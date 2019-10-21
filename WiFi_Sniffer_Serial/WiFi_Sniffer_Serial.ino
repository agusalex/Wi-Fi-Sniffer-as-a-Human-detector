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
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include <set>
#include <string>
#include "./functions.h"
#define disable 0
#define enable  1
#define CLEARTORECIEVEPIN 1
#define SETCONFIGMODEPIN 2
const size_t capacity = 6 * JSON_ARRAY_SIZE(2) + JSON_OBJECT_SIZE(9);
// uint8_t channel = 1;
unsigned int channel = 1;
unsigned long meshTime = 0;
boolean sending = false;

void setup() {
  Serial.begin(115200);
  pinMode(2, OUTPUT);
  digitalWrite(2, HIGH);
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
 // attachInterrupt(digitalPinToInterrupt(SETCONFIGMODEPIN),
  // configSniffer, FALLING); 

}

void loop() {
  nothing_new++;                          // Array is not finite, check bounds and adjust if required
  if (nothing_new > 200) {                // monitor channel for 200 ms
    nothing_new = 0;
    channel++;
    if (channel == 15) {            // Only scan channels 1 to 14
      channel = 1;
    }
    wifi_set_channel(channel);
  }
  
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



void setLED(){
  if(digitalRead(2)==LOW){
    digitalWrite(2, HIGH);

  }
  else{
    digitalWrite(2,LOW);
  }

}
long getOffsetFromMesh(unsigned long meshTime){
    return millis() - meshTime;
}


void ICACHE_RAM_ATTR configSniffer(){
  String incoming = "";
  int incomingByte = 0;
  boolean isRecieving = false;

  while (Serial.available() > 0) {
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
        DynamicJsonDocument doc(JSON_OBJECT_SIZE(1) + 10);
        deserializeJson(doc, incoming.substring(1,incoming.length()-1));
        meshTime = doc["MESHTIME"]; 
        return;
      }
      else {
        incoming += a;
      }
    }
  }
}

void ICACHE_RAM_ATTR sendDevices() {
  if(aps_known_count+clients_known_count==0||sending){
    return;
  }
  sending = true;

  setLED();
  DynamicJsonDocument doc(capacity);
  doc["ID"] = ESP.getChipId();
  doc["MILIS"] = millis();
  doc["OFFSET"] = getOffsetFromMesh(meshTime);
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
    aps_known[u].lastDiscoveredTime=-1; //marked as dirty
    
    }
  // Add Clients
  for (int u = 0; u < clients_known_count; u++) {

    MAC.add(formatMac1(clients_known[u].bssid));
    RSSI.add(clients_known[u].rssi);
    MILIS.add(clients_known[u].lastDiscoveredTime);
    CH.add(clients_known[u].channel);
    TYPE.add("C");
    STATION_SSID.add(formatMac1(clients_known[u].ap));
    clients_known[u].lastDiscoveredTime=-1; //marked as dirty
  
  }


  Serial.print("#");
  serializeJson(doc, Serial);
  Serial.print("$");
  doc.clear();
  cleanAll();
  sending = false;
}