/*
    Copyright (C) 2017  Ing. Pavel Sedlacek, Dusan Zatkovsky
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


/*
   UDP syntax:
   signals:
     DS18B20 1wire sensor packet:    rail1 1w 2864fc3008082 25.44
     DS2438 1wire sensor packet:     rail1 1w 2612c3102004f 25.44 1.23 0.12
     digital input connected:        rail1 di1 1
     digital input disconnected:     rail1 di1 0
     analog input state:             rail1 ai1 520
   commands:
     relay on command:               rail1 do12 on
     relay off command:              rail1 do5 off
     pwm output command:             rail1 pwm1 255

   RS485 syntax must have \n symbol at the end of the command line
     
*/

#define dbg(x) Serial.println(x);
//#define dbg(x) ;

#include <OneWire.h>
#include <DS2438.h>
#include <Dhcp.h>
#include <Dns.h>
#include <Ethernet.h>
#include <EthernetUdp.h>

byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0};
unsigned int listenPort = 44444;
unsigned int sendPort = 55554;
unsigned int loxonePort = 55555;
IPAddress listenIpAddress;
IPAddress sendIpAddress(255, 255, 255, 255);

#define inputPacketBufferSize UDP_TX_PACKET_MAX_SIZE
char inputPacketBuffer[UDP_TX_PACKET_MAX_SIZE];
#define outputPacketBufferSize 100
char outputPacketBuffer[outputPacketBufferSize];

EthernetUDP udpRecv;
EthernetUDP udpSend;

#define serialTxControl 8
#define oneWireCycle 30000
#define anaInputCycle 500
#define statusLedTimeOn 53
#define statusLedTimeOff 950
 
#define numOfRelays 12
int relayPins[] = {39, 41, 43, 45, 47, 49, 23, 25, 27, 29, 31, 33};
#define numOfPwms 4
int pwmPins[] = {11, 13, 12, 7};
#define numOfAnaInputs 3
int analogPins[] = {58, 59, 62};
int analogStatus[numOfAnaInputs];
#define numOfDigInputs 24
int inputPins[] = {36, 34, 48, 46, 69, 68, 67, 66, 44, 42, 40, 38, 6, 5, 3, 2, 14, 15, 16, 17, 24, 26, 28, 30};
int inputStatus[numOfDigInputs];
#define numOfDipSwitchPins 4
int dipSwitchPins[] = {54, 55, 56, 57};


int statusLedPin = 32;
int boardAddress = 0;

String boardAddressStr;
String boardAddressRailStr;
String railStr = "rail";
String digInputStr = "di";
String anaInputStr = "ai";
String relayStr = "do";
String pwmStr = "pwm";
String relayOnCommands[numOfRelays];
String relayOffCommands[numOfRelays];
String pwmCommand[numOfPwms];

class Timer {
  private:
    unsigned long timestampLastHitMs;
    unsigned long sleepTimeMs;
  public:
    boolean isOver();
    void sleep(unsigned long sleepTimeMs);
};

boolean Timer::isOver() {
    if (millis() - timestampLastHitMs < sleepTimeMs) {
        return false;
    }
    timestampLastHitMs = millis();
    return true;
}

void Timer::sleep(unsigned long sleepTimeMs) {
    this->sleepTimeMs = sleepTimeMs;
    timestampLastHitMs = millis();
}

Timer statusLedTimerOn;
Timer statusLedTimerOff;
Timer oneWireTimer;
Timer analogTimer;

OneWire ds(9);
byte oneWireData[12];
byte oneWireAddr[8];

#define maxSensors 5
byte readstage = 0, resolution = 11;
byte sensors[maxSensors][8], DS2438count, DS18B20count;
byte sensors2438[maxSensors][8], sensors18B20[maxSensors][8];
DS2438 ds2438(&ds);


void setup() {

    Serial.begin(9600);
    Serial1.begin(115200);

    for (int i = 0; i < numOfDigInputs; i++) {
        pinMode(inputPins[i], INPUT_PULLUP);
        inputStatus[i] = 0;
    }
    
    for (int i = 0; i < numOfRelays; i++) {
        pinMode(relayPins[i], OUTPUT);
        relayOnCommands[i] = relayStr + String(i + 1, DEC) + " on";
        relayOffCommands[i] = relayStr + String(i + 1, DEC) + " off";
        setRelay(i+1, 0);
    }

    for (int i = 0; i < numOfPwms; i++) {
        pinMode(pwmPins[i], OUTPUT);
        pwmCommand[i] = pwmStr + String(i + 1, DEC);
        setPWM(i+1, 0);
    }

    for (int i = 0; i < numOfAnaInputs; i++) {
        pinMode(analogPins[i], INPUT);
    }
    
    pinMode(statusLedPin, OUTPUT); 
    statusLedTimerOn.sleep(statusLedTimeOn);
    statusLedTimerOff.sleep(statusLedTimeOff);

    for (int i = 0; i < numOfDipSwitchPins; i++) {
        pinMode(dipSwitchPins[i], INPUT);
    }

    if (!digitalRead(54)) { boardAddress |= 1; }
    if (!digitalRead(55)) { boardAddress |= 2; }
    if (!digitalRead(56)) { boardAddress |= 4; }
    if (!digitalRead(57)) { boardAddress |= 8; }

    boardAddressStr = String(boardAddress);  
    boardAddressRailStr = railStr + String(boardAddress);

    mac[5] = (0xED + boardAddress);
    listenIpAddress = IPAddress(192, 168, 150, 150 + boardAddress);
    Ethernet.begin(mac, listenIpAddress);
    udpRecv.begin(listenPort);
    udpSend.begin(sendPort);

    pinMode(serialTxControl, OUTPUT);
    digitalWrite(serialTxControl, 0);

    dbg("Railduino address: ");
    dbg(boardAddressStr);
    dbg(listenIpAddress);
    
    lookUpSensors(); 

}

void loop() {

    if (statusLedTimerOff.isOver()) { digitalWrite(statusLedPin,255); }
    if (statusLedTimerOn.isOver()) { digitalWrite(statusLedPin,0); }   
  
    readInputs();

    processCommands();

    processOnewire();
 
}


String oneWireAddressToString(byte addr[]) {
    String s = "";
    for (int i = 0; i < 8; i++) {
        s += String(addr[i], HEX);
    }
    return s;
}

void lookUpSensors(){
  byte j=0, k=1, l=0, m=1;
  while ((j <= maxSensors) && (ds.search(sensors[j]))){
    if (!OneWire::crc8(sensors[j], 7) != sensors[j][7]){
        if (sensors[j][0] == 38){
          for (l=0;l<8;l++){ sensors2438[k][l]=sensors[j][l]; }  
          k++;  
        } else {
          for (l=0;l<8;l++){ sensors18B20[m][l]=sensors[j][l]; }
          m++;
          dssetresolution(ds,sensors[j],resolution);
        }
    }
    j++;
  }
  DS2438count = k-1;
  DS18B20count = m-1;
}

void dssetresolution(OneWire ow, byte addr[8], byte resolution) {
  byte resbyte = 0x1F;
  if (resolution == 12){ resbyte = 0x7F; }
  else if (resolution == 11) { resbyte = 0x5F; }
  else if (resolution == 10) { resbyte = 0x3F; }

  ow.reset();
  ow.select(addr);
  ow.write(0x4E);         
  ow.write(0);            
  ow.write(0);            
  ow.write(resbyte);      
  ow.write(0x48);         
}

void dsconvertcommand(OneWire ow, byte addr[8]){
  ow.reset();
  ow.select(addr);
  ow.write(0x44,1);       
}

float dsreadtemp(OneWire ow, byte addr[8]) {
  int i;
  byte data[12];
  float celsius;
  ow.reset();
  ow.select(addr);
  ow.write(0xBE);         
  for ( i = 0; i < 9; i++) { 
    data[i] = ow.read();
  }

  int TReading = (data[1] << 8) | data[0];
  int SignBit = TReading & 0x8000;
  
  if (SignBit)                    
  {
    TReading = (TReading ^ 0xffff) + 1; 
    celsius = -1 * TReading * 0.0625;
  } else {
    celsius = TReading * 0.0625;
  }
  
  return celsius;
}


void processOnewire() {

    if (!oneWireTimer.isOver()) {
        return;  
    }

   dbg("processing onewire");
   oneWireTimer.sleep(oneWireCycle);   
   
   byte m=1,n=1;
   while ((m <= DS2438count)){              
      ds2438.begin();
      ds2438.update(sensors2438[m]);
      if (!ds2438.isError()) {
        float TempArrayDS2438[15];
        TempArrayDS2438[m] = ds2438.getTemperature();          
        TempArrayDS2438[m+1] = ds2438.getVoltage(DS2438_CHA);   
        TempArrayDS2438[m+2] = ds2438.getVoltage(DS2438_CHB);
        sendMsg("1w " + oneWireAddressToString(sensors2438[m]) + " " + String(TempArrayDS2438[m], 2) + " " + String(TempArrayDS2438[m+1], 2) + " " + String(TempArrayDS2438[m+2], 2));
     }
     m++;
   }
       
   while ((n <= DS18B20count)){
      if (readstage == 0){
        dsconvertcommand(ds,sensors18B20[n]);
        readstage++;
      } else {                                                    
        if (ds.read()) {  
          float TempArrayDS18B20[5];
          TempArrayDS18B20[n] = dsreadtemp(ds,sensors18B20[n]);
          sendMsg("1w " + oneWireAddressToString(sensors18B20[n]) + " " + String(TempArrayDS18B20[n], 2));
          readstage=0; 
        }
      }   
    n++;   
   }
}


void readInputs() {
    
    for (int i = 0; i < numOfDigInputs; i++) {      
       int pin = inputPins[i];
       int value = digitalRead(pin);
       int oldValue = inputStatus[i];
       inputStatus[i] = value;
       if (value < oldValue) {
        sendInputOn(i + 1);
       }
       if (value > oldValue) {
        sendInputOff(i + 1);
       }
    }
    
    if (!analogTimer.isOver()) {
        return;  
    }

    analogTimer.sleep(anaInputCycle);
    for (int i = 0; i < numOfAnaInputs; i++) {
        int pin = analogPins[i];
        float value = analogRead(pin);
        float oldValue = analogStatus[i];
        analogStatus[i] = value;
        if (value != oldValue) {
            sendAnaInput(i+1,value*10);
        }
    } 
}

void sendInputOn(int input) {
    sendMsg(digInputStr + String(input, DEC) + " 1");
}

void sendInputOff(int input) {
    sendMsg(digInputStr + String(input, DEC) + " 0");
}

void sendAnaInput(int input, float value) {
    sendMsg(anaInputStr + String(input, DEC) + " " + String(value, 2));
}

void sendMsg(String message) {
    message = railStr + boardAddressStr + " " + message;
    udpSend.beginPacket(sendIpAddress, loxonePort);
    message.toCharArray(outputPacketBuffer, outputPacketBufferSize);
    udpSend.write(outputPacketBuffer, message.length());
    udpSend.endPacket();
    
    digitalWrite(serialTxControl, 1);     
    Serial1.print(message + "\n");
    delay(10);    
    digitalWrite(serialTxControl, 0);
        
    dbg(message);
}

void setRelay(int relay, int value) {
    if (relay > numOfRelays) {
        return;
    }
    dbg("Writing to relay " + String(relay+1) + " value " + String(value));
    digitalWrite(relayPins[relay], value);
}

void setPWM(int pwm, int value) {
    if (pwm > numOfPwms) {
        return;
    }
    dbg("Writing to PWM output " + String(pwm+1) + " value " + String(value));
    analogWrite(pwmPins[pwm], value);
}

String receivePacket() {
    String cmd = "";    
    
    while (Serial1.available()) {    
      cmd = Serial1.readStringUntil('\n'); 
      if (cmd.startsWith(boardAddressRailStr)) {
            cmd.replace(boardAddressRailStr, "");
            cmd.trim();
            return cmd;
        }
    }     

    int packetSize = udpRecv.parsePacket();
    if (packetSize) {
        memset(inputPacketBuffer, 0, sizeof(inputPacketBuffer));
        udpRecv.read(inputPacketBuffer, inputPacketBufferSize);
        String cmd = String(inputPacketBuffer);
        if (cmd.startsWith(boardAddressRailStr)) {
            cmd.replace(boardAddressRailStr, "");
            cmd.trim();
            return cmd;
        }
    }
    return "";
}

void processCommands() {
    String cmd = receivePacket();
    if (cmd != "") {
        if (cmd.startsWith(relayStr)) {
            for (int i = 0; i < numOfRelays; i++) {
                if (cmd == relayOnCommands[i]) {
                    setRelay(i, 1);
                } else if (cmd == relayOffCommands[i]) {
                    setRelay(i, 0);
                }
            }
        } else if (cmd.startsWith(pwmStr)) {
            String pwmvalue = cmd.substring(pwmStr.length() + 2);
            for (int i = 0; i < numOfPwms; i++) {
                if (cmd.substring(0,pwmStr.length()+1) == pwmCommand[i]) {
                    setPWM(i, pwmvalue.toInt());
                } 
            }
        }
    }
}
