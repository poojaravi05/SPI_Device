
This directory contains 4 files main.c, sensor.c , spi_led.c & fun.c

sensor.c - This is a driver for pulse measurement.
          Write function to this driver will trigger the sensor & the read function will read
          echo pulse & width is directly proportional to range.

spi_led.c - This is the driver for led matrix & its implemented in non blocking way .
            Only Write function can be used & it displays the patterns sent. its an SPI device

main.c -	This is a user application displaying distance controlled animation.


fun.c - This contains some supporting functions to trigger & read echo from ultrasonic sensor


Usage
**********
The led matrix displays A - K as patterns which are displayed with variable delays

Distance is divided as:
a) 0 - 10 cm : A & B will be displayed with delay of 50 ms
b) 10 - 20 cm : C & D  will be displayed with delay of 100 ms
c) 20 - 30 cm : E & F  will be displayed with delay of 150 ms
d) 30 - 40 cm : G & H  will be displayed with delay of 200 ms
e) 40 - 50 cm : I & J  will be displayed with delay of 250 ms
f) >50 cm : K & blank matrix  will be displayed with delay of 300 ms & 800 ms respectively

The distance to the object is Test distance = (pulse width * 340 m/s) / 2

Compilation on host system
*****************************
1) "make all" -- The program will begin preparing the executable files which is cross compiled.

2) main is the executable file that needs to be transferred to galelio board .
  spi_led.ko & sensor.ko are driver files which too needs to be transferred to galileo board.

3) Setting up Galileo Board

Connect the TTL & Ethernet cable to Galileo Board & host system & set up static ip address
open terminal & execute : sudo screen /dev/ttyUSB0 115200
Double click enter & you can get the console of Galileo Board.
Set up static ip address using :
ifconfig enp0s20f6 192.168.1.5 netmask 255.255.0.0 up
Ensure that host Ethernet connection is on same netmask & you will be able execute:  ping 192.168.1.5 with successful transmission of bytes to the board

4) Now that you have setup Connection execute on a new terminal:
	"scp main root@192.168.1.5:/home"

Commands on Intel Galileo
**************************

To insert the module use:
-----------------------------

 rmmod spidev - this is used to remove existing spidev module
 insmod spi_led.ko
 insmod sensor.ko

./main - this function will execute the user program which will interact with both the drivers .

Distance b/w the sensor & object will be displayed

To remove the module use:
------------------------------
 rmmod spi_led.ko
 rmmod sensor.ko



 Authors
 =========

 NITHEESH MUTHURAJ
 ASU ID:1213383988
 nitheesh@asu.edu
 +1-4804981497

 REVANTH RAJSHEKAR
 ASU ID:1213380816
 rmrajash@asu.edu
 +1-480-519-4975
