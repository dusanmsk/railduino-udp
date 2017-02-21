# railduino-udp-485

New official firmware for Railduino v1.3 based on UDP / 485 broadcasts (LAN / serial bus)

UDP / 485 syntax:
---------------------
 
   - signals:
     - DS18B20 1wire sensor packet:    rail1 1w 2864fc3008082 25.44
     - DS2438 1wire sensor packet:     rail1 1w 2612c3102004f 25.44 1.23 0.12
     - digital input connected:        rail1 di1 1
     - digital input disconnected:     rail1 di1 0
     - analog input state:             rail1 ai1 520
   
   - commands:
     - relay on command:               rail1 do12 on
     - relay off command:              rail1 do5 off
     - pwm output command:             rail1 pwm1 255

In addition 485 syntax must have \n symbol at the end of the command line

Communication settings
-----------------------

- 485 - serial link RS485:
  - baud rate                    115200 Bd
  - data bits                        8
  - stop bits                         1
  - parity                                              no
  
- UDP - LAN
  - UDP receiving port                           55555
  - UDP outgoing port                            44444
  - IP address   192.168.150.15x

Quick start:
----------------------

To start using Railduino with UDP / 485 broadcasts you need to reflash Railduino firmware with this custom one and change
your Loxone Config to create new inputs and outputs etc... (you should found example for Loxone in example_loxone folder)

Uploading custom firmware to your Railduino (easy way):

- download xloader utility from http://xloader.russemotto.com/
- disassemble railduino din case to get to the usb port (if not accesible from outside)
- connect railduino to PC with standard A-B usb cable
- run xloader
  - choose file railduino_udp_485_official/Railduino_1_3_485_UDP.ino.with_bootloader.mega.hex
  - choose device Mega(ATMEGA2560) !!
  - choose the serial port where Railduino is connected to
  - upload the firmware 
  
