
/**                                                                                            
 * Copyright 2019 Frank Duerr                                                                  
 *                                                                                             
 * Redistribution and use in source and binary forms, with or without                          
 * modification, are permitted provided that the following conditions are met:                 
 *                                                                                             
 * 1. Redistributions of source code must retain the above copyright notice,                   
 *    this list of conditions and the following disclaimer.                                    
 * 2. Redistributions in binary form must reproduce the above copyright notice,                
 *    this list of conditions and the following disclaimer in the documentation                
 *    and/or other materials provided with the distribution.                                   
 *                                                                                             
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS "AS IS" AND ANY                    
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED                   
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE                      
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY                  
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES                  
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;                
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND                 
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT                  
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS               
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.                                
 */

#include <mcp_can.h>
#include <mcp_can_dfs.h>

//#define DEBUG 1

// CAN ID of step commands
#define CANID_STEPMSG 0x01
// CAN ID of position message
#define CANID_POSMSG 0x02

// CAN messages have max. 8 bytes payload.
#define MAX_CANMSG_SIZE 8

#define PIN_STEPPER_STEP 7
#define PIN_STEPPER_DIR 6

#define PIN_CAN_SS 10
#define PIN_CAN_INT 2

// 1.8 deg stepper with 1/8 micro-stepping mode.
#define MAX_STEPS 1600

enum err_code {can_init};

enum step_dir {fwd, rev};

uint16_t stepper_position = 0;

MCP_CAN can(PIN_CAN_SS);

void bail_out(enum err_code err)
{  
    while (true) {
#ifdef DEBUG      
        switch (err) {
            case can_init :
                Serial.println("CAN init failed");    
                break;
            default:
                Serial.println("Unknown failure");
        }
        delay(1000);
#endif
    }
}

/**
 * Setup: executed at boot time
 */
void setup() 
{
#ifdef DEBUG
    Serial.begin(115200);
#endif

    pinMode(PIN_STEPPER_STEP, OUTPUT);
    digitalWrite(PIN_STEPPER_STEP, LOW);
    pinMode(PIN_STEPPER_DIR, OUTPUT);
    digitalWrite(PIN_STEPPER_DIR, LOW);

    // Our CAN module runs at 8 MHz.
    if (can.begin(CAN_250KBPS, MCP_8MHz) != CAN_OK) {
        bail_out(can_init);
    }
}

void send_position()
{
    uint8_t buffer[2];
    // Send position in Big Endian format (high byte first).
    buffer[0] = ((stepper_position >> 8) & 0xff);
    buffer[1] = (stepper_position & 0xff);

    int32_t canid = CANID_POSMSG;

    // Send standard frame.
    if (can.sendMsgBuf(canid, 0, 2, buffer) != CAN_OK) {
#ifdef DEBUG    
        Serial.println("CAN: could not send frame.");
#endif  
    }
  
}

void make_step(enum step_dir dir)
{
    // For accurate timing, disable tinterrupts.
    cli();
    
    switch (dir) {
        case fwd :
            digitalWrite(PIN_STEPPER_DIR, HIGH);
            break;
        case rev :
            digitalWrite(PIN_STEPPER_DIR, LOW);
            break;
    }

    // Must wait at least 200 ns -> 400 ns should be safe
    // Busy-waiting delay loop (about 7 CPU cycles)
    _delay_loop_1((uint8_t) (400e-9*F_CPU + 0.5));

    // Low-to-high transition triggers step
    digitalWrite(PIN_STEPPER_STEP, HIGH);
    
    // Min. step high time (pulse width): 1 us -> 2 us should be safe
    // This will be translated to a busy-waiting delay loop at compile time.
    _delay_us(2.0);

    // Reset step signal
    digitalWrite(PIN_STEPPER_STEP, LOW);
    
    // Min. step low time: 1 us -> 2 us should be safe
    // This will be translated to a busy-waiting delay loop at compile time.
    _delay_us(2.0);

    sei();

    switch (dir) {
        case fwd :
            if (stepper_position == MAX_STEPS-1)
                stepper_position = 0;
            else
                stepper_position++;
            break;
        case rev : 
            if (stepper_position == 0) 
                stepper_position = MAX_STEPS-1;
            else
                stepper_position--;
            break;
    }
}

void process_can_msg(unsigned long can_id, uint8_t is_ext, uint8_t is_remote_req, uint8_t *buffer, size_t len)
{
#ifdef DEBUG    
        Serial.print("CAN: received msg: id = ");
        Serial.println(can_id);
#endif

    if (can_id == CANID_STEPMSG) {
#ifdef DEBUG    
        Serial.println("CAN: received step msg.");
#endif
        
        // Step message should have 1 byte payload.
        if (len != 1)
            return;

        uint8_t direction = buffer[0];
        switch (direction) {
            case 0x01 :
                make_step(fwd);
                break;
            case 0x02 :
                make_step(rev);
                break;
        }

    }

    if (can_id == CANID_POSMSG) {
#ifdef DEBUG    
        Serial.println("CAN: received pos msg.");    
#endif

        // Position message should have 2 bytes payload.
        if (len != 2)
            return;

        // Position message must have Remote Transmission Request bit set.
        if (!is_remote_req)
            return;

        send_position();        
    }
}

/**
 * Main loop
 */
void loop() 
{
    uint8_t len;
    uint8_t buf[MAX_CANMSG_SIZE];
    
    while (true) {
        // Poll CAN module for received message.
        if (can.checkReceive() == CAN_MSGAVAIL) {
            if (can.readMsgBuf(&len, buf) == CAN_OK) {
                uint8_t is_remote_req = can.isRemoteRequest();
                uint8_t is_ext = can.isExtendedFrame();
                unsigned long can_id = can.getCanId();
                process_can_msg(can_id, is_ext, is_remote_req, buf, len);    
            } else {
#ifdef DEBUG    
              Serial.println("CAN: could not receive frame");
#endif
            }
        }
    }

}
