# Wiring RPi <-> Level Shifter <-> CAN Module

| Function | RPi #Pin     | Wire Color |
|----------+--------------+------------|
|  MISO    | 21           | violet     |
|  MOSI    | 19           | white      |
|  SCLK    | 23           | yellow     |
|  SS      | 24 (CE0)     | green      |
|  INT     | 32 (GPIO 12) | grey       |

# RPi: MCP2515 Setup

## Configuration of MCP2515 Kernel Module

Edit /boot/config.txt:

dtparam=spi=on
dtoverlay=mcp2515-can0,oscillator=8000000,spimaxfrequency=500000,interrupt=12
dtoverlay=spi-bcm2835-overlay

RPi has two SPI buses.
We use bus #0.

The oscillator on our MCP2515 board is 8 MHz.

Maximum SPI frequency of MCP2515 is 10 MHz.
However, for some resilience to imperfect cabling, we choose SPI clock at max. 500 kHz.
Note: RPi runs at a core clock speed of 250 MHz.
Thus, 500 kHz should actually mean: f_max = 250 MHz / 512 = 488 kHz

The MCP2515 board has an interrupt line.
We connect this line to GPIO 12 (BCM numbering scheme).

Check for CAN device after reboot:

$ ls /sys/bus/spi/devices/spi0.0

## Socket CAN Setup

Setup CAN can0 network devices with bitrate 250 kbit/s:

$ sudo ip link set can0 up type can bitrate 250000

Test: send an 8-byte CAN frame with 11-bit CAN ID 001 (extended frame would require "##" instead of "#") using SocketCAN utilities:

$ sudo apt-get install can-utils
$ cansend can0 001#0102030405060708

# Maximum CAN Frame Size, Frame Rate, Data Rate

1 Start bit
+ 11 identifier bits
+  1 RTR bit 
+  6 control bits (4 DLC + 2 reserved)
+  8 data bits
+  15 CRC bits
+  1 CRC delimiter
+  1 ACK slot
+  1 ACK delimiter
+  7 EOF bits
+  3 IFS (Inter Frame Space) bits
= 57 bits

Maximum number of stuff bits: after 5 equal bits, one stuff bit is inserted: max. 57/5 = 11 stuff bits for 57 bit frame.

Max. frame size for 1 byte data frame (incl. IFS): 57 + 11 = 68 bit

Max. frame rate at 250 kbit/s: 250 kbit/s / 68 bit = 3676 frames/s

Max. data rate at 250 kbit/s: 3676 frames/s * 8 bit = 29412 kbit/s