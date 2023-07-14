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

// Required for pthread_setaffinity_np()
#define _GNU_SOURCE

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <time.h>
#include <stdint.h>
#include <stdbool.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <linux/can.h>
#include "socketcan.h"

// Maximum length of statically allocated strings.
#define MAX_STR_LEN 1024

// Priority of real-time thread.
unsigned int thread_prio;
// Real-time thread will be pinned to this CPU.
unsigned int thread_cpu;
// Cycle time of real-time thread in nano-seconds.
unsigned long long cycle_time_ns;

// Name of the CAN interface.
char ifname[MAX_STR_LEN];

// The real-time thread.
pthread_t rt_thread;

// File descriptor of CAN socket.
int cansock = -1;

/**
 * Send remote transmisson request for motor position.
 */
void send_rtr_motor_position()
{
     // TODO: Definieren Sie die CAN ID für Motor-Positions-Nachrichten.
     // CAN ID for motor position messages,  
     const uint32_t canid_motorposition = 0x02;

     /*
      * Controller Area Network Identifier structure
      * - bit 0-28  : CAN identifier (11/29 bit)
      * - bit 29    : error message frame flag (0 = data frame, 1 = error message)
      * - bit 30    : remote transmission request flag (1 = rtr frame)
      * - bit 31    : frame format flag (0 = standard 11 bit, 1 = extended 29 bit)
      */
     canid_t canid;

     // TODO: Definieren Sie die CAN ID (canid). Verwenden Sie eine 11 bit
     // Standard-ID. 
     canid = canid_motorposition;

     // TODO: Setzen Sie das Remote Transmission Flag in canid.
     // Set the remote transmission request flag
     canid |= (1 << 30);

     /*
      * CAN frame with the following data fields:
      * - canid_t can_id  : the can id
      * - uint8_t can_dlc : data length 
      * - uint8_t data[8] : data
      */
     struct can_frame frame;

     // TODO: Definieren Sie den zu sendenden CAN-Frame (frame). Beachten Sie:
     // Ein Remote-Transmission-Request-Frame muss die gleiche Datenlänge haben
     // wie der angeforderte Antwortrahmen (auch wenn im Request noch keine
     // Daten übermittelt werden)!
     frame.can_id = canid;
     // Data length of the frame must match the expected data length of the
     // response.
     frame.can_dlc = 2;    // 2 bytes data
     frame.data[0] = 0x00; // not necessary: data will be filled in response
     frame.data[1] = 0x00; // not necessary: data will be filled in response
     
     // Send CAN frame.
     ssize_t nsent = write(cansock, &frame, sizeof(frame));
     if (nsent < 0)
	  perror("Could not send CAN frame");
}

/**
 * Receive and process remote response.
 *
 * @param csock the can socket for receiving remote responses.
 */
void recv_remote_response(int csock)
{
     // TODO: Definieren Sie die CAN ID für Motor-Positions-Nachrichten.
     // CAN ID for motor position messages,  
     const uint32_t canid_motorposition = 0x02;
     
     /*
      * CAN frame with the following data fields:
      * - canid_t can_id  : the can id
      * - uint8_t can_dlc : data length 
      * - uint8_t data[8] : data
      */
     struct can_frame frame;
     
     while (true) {
	  ssize_t len = read(csock, &frame, sizeof(frame));
	  if (len < 0) {
	       perror("Error while receiving frame from CAN bus");
	       continue;
	  }

	  // TODO:
	  // Verarbeiten Sie den empfangenen Frame (Rahmen):
	  // - Geben Sie die Motor-Position in Grad aus.

	  /*
	   * Controller Area Network Identifier structure
	   * - bit 0-28  : CAN identifier (11/29 bit)
	   * - bit 29    : error message frame flag 
	   *               (0 = data frame, 1 = error message)
	   * - bit 30    : remote transmission request flag (1 = rtr frame)
	   * - bit 31    : frame format flag 
	   *               (0 = standard 11 bit, 1 = extended 29 bit)
	   */
	  // TODO: Testen Sie, ob eine Fehlernachricht (error message) oder
	  // eine RTR-Nachricht empfangen wurde. Wenn ja: Setzen Sie die
	  // Ausführung mit dem nächsten Schleifendurchlauf fort (continue).
	  if ( (frame.can_id & (1<<29)) || (frame.can_id & (1<<30)))
	       continue;

	  // TODO: Ermitteln Sie die CAN ID des Frames.
	  // Maskieren Sie dazu die entsprechenden Bits aus frame.can_id.
	  uint32_t canid = (frame.can_id & 0x1fffffff);
	  
	  // TODO: Überprüfen Sie, ob der Rahmen die erwartete CAN ID und
	  // Datenlänge hat. Wenn nein: Setzen Sie die Ausführung mit dem
	  // nächsten Schleifendurchlauf fort (continue).
	  // Did we receive a frame with the expected CAN ID and the expected
	  // data length?
	  if (canid != canid_motorposition || frame.can_dlc != 2)
	       continue;

	  // TODO: Ermitteln Sie die Motorposition in Schritten, die in
	  // den Daten des Frames übermittelt wurde. Beachten Sie: Die Daten
	  // werden im Big-Endian-Format übermittelt. 
	  // 16 bit motor position is sent in Big Endian format:
	  // high byte first (data[0]), low byte second (data[1]).
	  uint16_t pos_steps = (((uint16_t) (frame.data[0])<<8) |
				frame.data[1]);

	  // TODO: Konvertieren Sie die Motorposition in Schritten in
	  // eine Motorposition in Grad. Beachten Sie: Der Stepper-Motor
	  // dreht um 1.8 Grad pro Vollschritt und verwendet 1/8
	  // Micro-Stepping.
	  // Convert motor position in steps to degree.
	  // We have an 1.8 degree stepper motor with 1/8 micro-stepping.
	  // Thus, one step is 1.8*(1/8) degree.
	  double pos_deg = (double) pos_steps * 1.8 / 8.0;

	  // Display motor position.
	  printf("Position = %f \n", pos_deg); 
     }
}

/**
 * Given the start time of the previous cycle and the cycle time (period),
 * calculate the start of the next cycle.
 *
 * @param ts start time of previous cycle. Parameter ts will be updated to the
 * start of the next cycle.
 * @param cycle_time_ns cycle time (period) in nano-seconds.
 */
void next_cycle_start(struct timespec *ts, uint64_t cycle_time_ns)
{
     uint64_t t_ns = ts->tv_nsec;
     t_ns += cycle_time_ns;

     ts->tv_sec += t_ns/1000000000;
     ts->tv_nsec = t_ns%1000000000; 
}

/**
 * Thread function of the real-time thread.
 */
void *rt_thread_run(void *args)
{
     const clockid_t cid = CLOCK_MONOTONIC;
     
     // Set thread affinity: real-time threads stays on one CPU.
     cpu_set_t cpuset;
     pthread_t thread = pthread_self();
     CPU_ZERO(&cpuset);
     CPU_SET(thread_cpu, &cpuset);
     if (pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset)) {
	  perror("Could not set affinity");
	  exit(1);
     }

     // Determine start time t_start of first cycle.
     // Thread will be scheduled at times t_start + n*cycle_time.
     struct timespec ts;
     clock_gettime(cid, &ts);

     while (true) {
	  // Periodically send remote transmission request for motor position
	  // via CAN.
	  send_rtr_motor_position();
	  
	  // Sleep until beginning of next cycle.
	  next_cycle_start(&ts, cycle_time_ns);
	  int ret;
	  if (ret = clock_nanosleep(cid, TIMER_ABSTIME, &ts, NULL)) {
	       perror("Could not sleep");
	       exit(1);
	  }
     }
     
     return NULL;
}

void usage(const char *prog)
{
     fprintf(stderr, "Usage: %s "
	     "-t CYCLE_TIME_NANOSECONDS "
	     "-c REALTIME_THREAD_CPU "
	     "-p REALTIME_THREAD_PRIORITY "
	     "-i CAN_INTERFACE "
	     "\n", prog);
}

void parse_cmdline_args(int argc, char *argv[])
{
     int opt;
     long long arg_cycletime = -1;
     int arg_cpu = -1;
     int arg_prio = -1;
     char arg_interface[MAX_STR_LEN];
     memset(arg_interface, 0, MAX_STR_LEN);
     
     while ( (opt = getopt(argc, argv, "t:c:p:i:")) != -1 ) {
	  switch(opt) {
	  case 't' :
	       arg_cycletime = atoll(optarg);
	       break;
	  case 'c' :
	       arg_cpu = atoi(optarg);
	       break;
	  case 'p' :
	       arg_prio = atoi(optarg);
	       break;
	  case 'i' :
	       strncpy(arg_interface, optarg, MAX_STR_LEN-1);
	       break;
	  case ':' :
	  case '?' :
	  default :
	       usage(argv[0]);
	       exit(1);
	  }
     }

     if (arg_cpu < 0 || arg_prio < 0 || arg_cycletime < 0 ||
	 strlen(arg_interface) == 0) {
	  usage(argv[0]);
	  exit(1);
     }

     // All required parameters are set.
     cycle_time_ns = arg_cycletime;
     thread_cpu = arg_cpu;
     thread_prio = arg_prio;
     strncpy(ifname, arg_interface, MAX_STR_LEN);
}

int main(int argc, char *argv[])
{
     // Parse command line parameters
     parse_cmdline_args(argc, argv);

     // Open CAN socket.
     if ( (cansock = socketcan_create_and_bind(ifname)) < 0 ) {
	  perror("Could not open CAN socket");
	  exit(1);
     }

     // Lock virtual address space of process into RAM.
     if (mlockall(MCL_CURRENT|MCL_FUTURE) == -1) {
	  fprintf(stderr, "Could not lock memory");
	  exit(1);
     }

     // Initialize pthread attributes.
     pthread_attr_t attr;
     if (pthread_attr_init(&attr)) {
	  perror("Could not init pthread attributes");
	  exit(1);
     }

     // Set scheduler policy: FIFO
     // Thread will only be preempted by threads of higher priority.
     // No timer interrupt and limited time quantum as for round-robin
     // scheduling.
     if (pthread_attr_setschedpolicy(&attr, SCHED_FIFO)) {
	  perror("Could not set scheduling policy");
	  exit(1);
     }
     
     // Set thread priority.
     // 99 is highest, 1 lowest.
     struct sched_param param;
     param.sched_priority = thread_prio;
     if (pthread_attr_setschedparam(&attr, &param)) {
	  perror("Could not set thread priority");
	  exit(1);
     }

     // New thread will not inherit scheduler attributes from creating thread,
     // but takes them from attr object.
     if (pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED)) {
	  perror("Could not define scheduler attribute inheritance");
	  exit(1);
     }
    
     // Start Thread 
     int success = pthread_create(
	  &rt_thread,        // thread object
	  &attr,             // thread attributes as defined above
	  rt_thread_run,     // thread function
	  NULL               // no arguments to thread function
	  );
     if (success != 0) {
	  perror("Could not start thread");
	  exit(1);
     }

     // This function receives remote responses from CAN and
     // displays the motor position.
     recv_remote_response(cansock);
     
     // Will neve get here.
     
     // Main thread waits for thread to finish. 
     success = pthread_join(
	  rt_thread, // the thread to join
	  NULL       // exit value (see pthread_exit())
	  ); 
     if (success != 0) {
	  perror("Join with thread failed.");
	  exit(1);
     }
     
     return 0;
}
