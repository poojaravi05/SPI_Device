
	Execution instructions 
===============================================
			  matrix
	sensor 		===========
	===== 		===========
	===== 		=========== (cat walking/ running observed)

speed of cat pretty fast in this region 
	

20cms    ====	(obstacle)lower limit
	


linear variation in speed of the cat in this region



95 cms	====	(obstacle)upper limit

speed of cat pretty slow in this region
============
observations
============
As obstacle move towards the sensor the cat is expected to turn right & vice-versa.
The speed variations are as shown above.
0-20 cms --fast
20-95 cms -- linear variation 
95 > cms -- slow

The expected result can be optimally obtained near 60-70 cms & its expected that the movements are little quick as 
the change in direction will occur if the change is by 2 cm in successive iteration of polling function.

========================
ardunino pin connections
========================

2--trigger pin of HCSR04
3--echo pin of HCSR04
    
11--SPI-MOSI-----CS
12--GPIO15(SPI1_SS)-------DIN
13--SPI SCK------CLK

Apart from these Vcc & ground must be connected accordingly on both matrix & sensor.

====================
 Running the tests
====================
1) "make all" -- The program will begin preparing the executable files which is cross compiled.

2) main is the executable file that needs to be transferred to galelio board.

3) Setting up Galileo Board 
Connect the TTL & Ethernet cable to Galileo Board & host system & set up static ip address 
open terminal & execute : sudo screen /dev/ttyUSB0 115200
Double click enter & you can get the console of Galileo Board.
Set up static ip address using :
ifconfig enp0s20f6 192.168.1.5 netmask 255.255.0.0 up
Ensure that host Ethernet connection is on same netmask & you will be able execute:  ping 192.168.1.5 with successful transmission of bytes to the board

4) Now that you have setup Connection execute on a new terminal:
	"scp main root@192.168.1.5:/home"

5) In console window navigate to home Directory & Execute :

./main  -- this command will begin execution the object file

=========
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

