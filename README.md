# Railduino v1.3 UDP 485 firmware 

New official firmware for Railduino v1.3 based on UDP / 485 broadcasts (LAN / serial bus). More at www.sedtronic.cz

Key features:
---------------

- 24x digital inputs - max. voltage 24V DC / each
- 12x relay outputs – max. current 5A at 230V AC / each
- 3x analog inputs – input voltage 0 - 10V
- 4x PWM digital outputs – max 25 W / each, max 24V DC
- 1-wire – DS18B20 or DS2438 sensors 
- RS485 – e.g. Loxone Modbus or RS485 extension
- USB – programming purposes
- Ethernet – LAN (optional)
- LED diode
- Reset button / remote reset
- Ping function
- IP address - static or dynamic
- DIP switch - address
- Terminator jumper

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
     - status command:                 rail1 stat10
     - reset command:                  rail1 rst
   
   - scan cycles:
     - 1wire cycle:                    30000 ms
     - analog input cycle:             30000 ms
     - heart beat cycle(only RS485):   60000 ms

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
  - static IP address                            192.168.150.15x
  - dynamic DHCP IP address
  - Loxone virtual outputs address    /dev/udp/255.255.255.255/44444
  - Loxone virtual inputs port    55555

Quick start:
----------------------

To start using Railduino with UDP / 485 broadcasts you need to reflash Railduino firmware with this custom one and change
your Loxone Config to create new inputs and outputs etc... 

Uploading custom firmware to your Railduino (easy way):

- download uploader utility "uploader.zip" from the uploader folder and unzip
- in case of 485 version download Railduino_1_3_485.ino.mega.hex file
- in case of UDP version download Railduino_1_3_485_UDP.ino.mega.hex file
- disassemble railduino din case to get to the usb port (if not accesible from outside)
- connect railduino to PC with standard A-B usb cable
- run RAILDUINO HEX UPLOAD.bat file
  - write the com port which the railduino is connected to - e.g. COM18
  - write file name which you want to upload - Railduino_1_3_485.ino.mega.hex
  - upload the firmware - after succesfull upload you should see table of stats with the press key to continue...
- if not succesful please remove the Arduino mega board from the ethernet shield and try again
  
