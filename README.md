#CRaSH: Collision Response and Sensor Heuristics
---
To solve the problem of hit-and-run collisions, we developed a system that allows vehicles to wirelessly exchange data when a collision occurs. To this end, a vehicle that is hit will retain identifying information about the vehicle that it collided with.

Two vehicle prototypes are used to simulate collision scenarios. Accelerometer readings are used to identify a collision and trigger a wireless data exchange. Each vehicle prototype posesses a .txt file that contains information such as make, model, and VIN. When a collision occurs, this data is assembled into a packet and the packet is appended with a time-stamp. The packet is then wirelessly sent to the other vehicle. The receiving vehicle writes the packet to a log file that serves as record of the collision — the process then repeats for the other vehicle.

This project was inspired by an interest in embedded systems for automotive applications. It was developed as a Senior Design Project at UC Irvine, Winter 2015. The team members are:

* Ryan Morrison, CpE
* Omar Bravo, CSE
* Nitish Sachar, CSE

 

##Requirements
####Hardware
*Note: This project requires 2 of each of the following.*

* Arduino Mega 2560
* Arduino Wireless SD Shield
* Sparkfun XBee Shield
* Adafruit Powerboost 500 Shield
* Adafruit LSM303DLHC Accel. + Compass
* Adafruit DS1307 RTC
* XBee S1 802.15.4 Module
* Smart Car Chassis Kit for Arduino
* Tinkerkit Relay Module
* DPDT Switch
* RGB LED
* Micro SD Card

####Software
* Arduino IDE
* X-CTU (XBee configuration)

##Setup
*Note: Due to the scope of this project, a summary of setup steps are provided here. For an in-depth explanation, please see the project page [here](http://www.ryanmorrisonportfolio.com).*

1. Assemble car chassis kit with relay, switch, and LED.
2. Solder RTC onto SD shield.
3. Solder accelerometer onto XBee shield.
4. Use SD shield + USB/serial cable + XBee module to program XBee:
	* A tutorial on programming XBees is [here](https://learn.sparkfun.com/tutorials/exploring-xbees-and-xctu).
	* XBee radios must be in 'broadcast' mode.
	* DL = 0xFFFF, DH = 0x0000 for all XBees.
5. Stack shields and connect wires according to schematics.
6. Make sure SD card is [formatted](http://arduino.cc/en/Reference/SDCardNotes) to FAT-16 and create a .txt file containing some data that uniquely identifies each car.
6. Affix modules to vehicle prototypes and upload main sketch.
7. Power-up the vehicles, collide them, and verify that they wirelessly exchanged data. 
