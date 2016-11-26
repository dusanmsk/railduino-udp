# railduino-udp
Alternative firmware for Railduino v1.3 based on udp boradcasts (instead of modbus tcp).

(super)Quick start:
----------------------

To start using railduino with udp broadcasts you need to reflash original railduino firmware with this custom one and change
your loxplan to create new inputs and outputs etc... (you should found example loxplan in example_loxplan folder)

Uploading custom firmware to your railduino (easy way):

- download xloader utility from http://xloader.russemotto.com/
- disassemble railduino din case to get to the usb port (if not accesible from outside)
- connect railduino to PC with standard A-B usb cable
- run xloader
  - choose file railduino_udp_firmware/railduino_udp_firmware.ino.with_bootloader.mega.hex
  - choose device Mega(ATMEGA2560) !!
  - choose the serial port where railduino is connected to
  - upload the firmware 

Upload will take about a minute and a Blue led will blink for a second after successfull upload.

   
