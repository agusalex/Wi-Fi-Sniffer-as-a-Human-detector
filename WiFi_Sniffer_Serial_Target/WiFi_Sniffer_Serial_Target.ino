/*
  Copyright 2017 Andreas Spiess

  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"),
  to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
  and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
  FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
  WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

  This software is based on the work of Ray Burnette: https://www.hackster.io/rayburne/esp8266-mini-sniff-f6b93a
*/
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include <set>
#include <string>
#include "./functions.h"
#define disable 0
#define enable  1
//#define CLEARTORECIEVEPIN D1 //NODEMCU
//#define SETCONFIGMODEPIN D2
#define CLEARTORECIEVEPIN D0  //D1_MINI
#define SETCONFIGMODEPIN D4
const size_t capacity = 6 * JSON_ARRAY_SIZE(2) + JSON_OBJECT_SIZE(9);
unsigned int channel = 13;
unsigned long meshOffset = 0;
boolean sending = false;
boolean configSnifferFlag = false;
boolean channelChangedFlag = false;
int step = 0;
int buttonState;             // the current reading from the input pin
int lastButtonState = HIGH;   // the previous reading from the input pin
// the following variables are unsigned longs because the time, measured in
// milliseconds, will quickly become a bigger number than can be stored in an int.
unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
unsigned long debounceDelay = 50;    // the debounce time; increase if the output flickers

void setup() {
  Serial.begin(115200);
  pinMode(2, OUTPUT);
  digitalWrite(2, HIGH);
  pinMode(D2,INPUT_PULLUP);
  wifi_set_opmode(STATION_MODE);            // Promiscuous works only with station mode
  wifi_set_channel(channel);
  wifi_promiscuous_enable(disable);
  wifi_set_promiscuous_rx_cb(promisc_cb);   // Set up promiscuous callback
  wifi_promiscuous_enable(enable);
  Serial.begin(115200);
  pinMode(CLEARTORECIEVEPIN, INPUT_PULLUP);
  pinMode(SETCONFIGMODEPIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(CLEARTORECIEVEPIN),
   sendDevices, FALLING);
  attachInterrupt(digitalPinToInterrupt(SETCONFIGMODEPIN),
   configSniffer, FALLING); 

}

void loop() {
  if(channelChangedFlag){
    wifi_set_channel(channel);
    channelChangedFlag = false;
  }
  if(toSend){
    sendDevices();
    toSend=false;
  }
  button();
 /* nothing_new++;                          // Array is not finite, check bounds and adjust if required
  if (nothing_new > 200) {                // monitor channel for 200 ms
    nothing_new = 0;
    channel++;
    if (channel == 15) {            // Only scan channels 1 to 14
      channel = 1;
    }
    wifi_set_channel(channel);
  }
  */


  delay(1);  // critical processing timeslice for NONOS SDK! No delay(0) yield()
}



void purgeDevice() {
  for (int u = 0; u < clients_known_count; u++) {
    if (clients_known[u].lastDiscoveredTime==-1) {
      for (int i = u; i < clients_known_count; i++) memcpy(&clients_known[i], &clients_known[i + 1], sizeof(clients_known[i]));
      clients_known_count--;
    }
  }
  for (int u = 0; u < aps_known_count; u++) {
    if (aps_known[u].lastDiscoveredTime ==-1) {
      for (int i = u; i < aps_known_count; i++) memcpy(&aps_known[i], &aps_known[i + 1], sizeof(aps_known[i]));
      aps_known_count--;
    }
  }
}
/*
void addDevice( String mac,String rssi,String lastDiscoveredTime,String channel, String type,String ssidOrStationMac){
  toSend +="{\"A\":\""+mac+"\","+"\"B\":\""+rssi+"\","+
      "\"C\":\""+lastDiscoveredTime+"\","+"\"D\":\""+channel+"\",\""+
      +"E\":\""+type+"\","+"\"F\":\""+ssidOrStationMac+"\"}";
}*/
void cleanAll() {
  memset(aps_known, 0, sizeof(aps_known));
  memset(clients_known, 0, sizeof(clients_known));
  clients_known_count = 0;
  aps_known_count = 0;
}

void button(){
    // read the state of the switch into a local variable:
  int reading = digitalRead(D2);

  // check to see if you just pressed the button
  // (i.e. the input went from LOW to HIGH), and you've waited long enough
  // since the last press to ignore any noise:

  // If the switch changed, due to noise or pressing:
  if (reading != lastButtonState) {
    // reset the debouncing timer
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    // whatever the reading is at, it's been there for longer than the debounce
    // delay, so take it as the actual current state:

    // if the button state has changed:
    if (reading != buttonState) {
      buttonState = reading;

      // only toggle the LED if the new button state is HIGH
      if (buttonState == LOW) {
        step++;
        Serial.println("B"+String(step)+",SE");
      }
    }
  }
  // save the reading. Next time through the loop, it'll be the lastButtonState:
  lastButtonState = reading;
}

void toggleLED(){
  if(digitalRead(LED_BUILTIN)==LOW){
    digitalWrite(LED_BUILTIN, HIGH);

  }
  else{
    digitalWrite(LED_BUILTIN,LOW);
  }

}
long getOffsetFromMesh(unsigned long meshOffset){
    return millis() - meshOffset;
}


void ICACHE_RAM_ATTR configSniffer(){
  if(configSnifferFlag){
    return;
  }
  configSnifferFlag = true; //To make sure it can only be called once a at the same time
  boolean isRecieving = false;
  String incoming = "";
  int incomingByte = 0;
  delayMicroseconds(20000);
  while (Serial.available() > 0) {
   toggleLED();
    // read the incomzng byte:
    incomingByte = Serial.read();
    // say what you got:s
    char a = (char) incomingByte;
   // Serial.print(a);
    if (a == '#') { //message start
      isRecieving = true;
      incoming = "";
    }
    else if (isRecieving) {
      if (a == '$') { //message end
        isRecieving = false;
        Serial.println("Recieved: "+ incoming);
        DynamicJsonDocument doc(JSON_OBJECT_SIZE(2) + 20);
        deserializeJson(doc, incoming.substring(1,incoming.length()-1));
        if(channel!=doc["CHANNEL"]){
          channelChangedFlag = true;
          channel = doc["CHANNEL"];

        }
        long MESHTIME = doc["MESHTIME"]; 
        meshOffset = getOffsetFromMesh(MESHTIME);
      }
      else {
        incoming += a;
      }
    }
  }
  configSnifferFlag = false;
}

void ICACHE_RAM_ATTR sendDevices() {
  if(aps_known_count+clients_known_count==0||sending){
    return;
  }
  sending = true; //To make sure it can only be called once a at the same time
  
  toggleLED();
  DynamicJsonDocument doc(capacity);
  doc["ID"] = ESP.getChipId();
  doc["MILIS"] = millis();
  doc["OFFSET"] = meshOffset;
  JsonArray MAC = doc.createNestedArray("MAC");
  JsonArray RSSI = doc.createNestedArray("RSSI");
  JsonArray MILIS = doc.createNestedArray("MILIS");
  JsonArray CH = doc.createNestedArray("CH");
  JsonArray TYPE = doc.createNestedArray("TYPE");
  JsonArray STATION_SSID = doc.createNestedArray("STA/SSID");
  // Add Beacons
  for (int u = 0; u < aps_known_count; u++) {
      
    MAC.add(formatMac1(aps_known[u].bssid));
    RSSI.add(aps_known[u].rssi);
    MILIS.add(aps_known[u].lastDiscoveredTime);
    CH.add(aps_known[u].channel);
    TYPE.add("B");
    STATION_SSID.add(aps_known[u].ssid);
    //aps_known[u].lastDiscoveredTime=-1; //marked as dirty
    
    }
  // Add Clients
  for (int u = 0; u < clients_known_count; u++) {

    MAC.add(formatMac1(clients_known[u].bssid));
    RSSI.add(clients_known[u].rssi);
    MILIS.add(clients_known[u].lastDiscoveredTime);
    CH.add(clients_known[u].channel);
    TYPE.add("C");
    STATION_SSID.add(formatMac1(clients_known[u].ap));
   // clients_known[u].lastDiscoveredTime=-1; //marked as dirty
  
  }


  Serial.print("#");
  serializeJson(doc, Serial);
  Serial.println("$");
  doc.clear();
  cleanAll();
  sending = false;
}