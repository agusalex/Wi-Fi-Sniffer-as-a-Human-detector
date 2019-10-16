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
#include <set>
#include <string>
#include "./functions.h"

#define disable 0
#define enable  1
#define MAXDEVICES 80
#define PURGETIME 600000
#define MINRSSI -100
#define MIN_SEND_TIME 5*1000
#define MAX_SEND_TIME 30*1000

#define   CLEARTORECIEVEPIN 4
#define   SETCONFIGMODEPIN 2

// uint8_t channel = 1;
unsigned int channel = 1;
unsigned long sendEntry, deleteEntry;
int nbrDevices = 0;
int usedChannels[15];
String toSend = "";

void setup() {
 
  Serial.begin(115200);
  pinMode(2, OUTPUT);
  digitalWrite(2, LOW);
  delay(500);
  digitalWrite(2, HIGH);
  delay(500);
  digitalWrite(2, LOW);
  delay(500);
  digitalWrite(2, HIGH);
  delay(500);
  digitalWrite(2, LOW);
  delay(500);
  digitalWrite(2, HIGH);
  
  wifi_set_opmode(STATION_MODE);            // Promiscuous works only with station mode
  wifi_set_channel(channel);
  wifi_promiscuous_enable(disable);
  wifi_set_promiscuous_rx_cb(promisc_cb);   // Set up promiscuous callback
  wifi_promiscuous_enable(enable);
  Serial.begin(115200);
  pinMode(CLEARTORECIEVEPIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(CLEARTORECIEVEPIN), sendDevices, FALLING);

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
   //digitalWrite(2, HIGH);
    
  }
  
  delay(1);  // critical processing timeslice for NONOS SDK! No delay(0) yield()
}



void cleanAll() {
  memset(aps_known, 0, sizeof(aps_known));
  memset(clients_known, 0, sizeof(clients_known));
  clients_known_count = 0;
}
void purgeDevice() {

  for (int u = 0; u < aps_known_count; u++) {
    if ((millis() - aps_known[u].lastDiscoveredTime) > PURGETIME) {
      Serial.print("purge Beacon" );
      Serial.println(u);
      for (int i = u; i < aps_known_count; i++) memcpy(&aps_known[i], &aps_known[i + 1], sizeof(aps_known[i]));
      aps_known_count--;
      break;
    }
  }
}


void showDevices() {
  Serial.println("");
  Serial.println("");
  Serial.println("-------------------Device DB-------------------");
  Serial.printf("%4d Devices + Clients.\n", aps_known_count + clients_known_count); // show count

  // show Beacons
  for (int u = 0; u < aps_known_count; u++) {
    Serial.printf( "%4d ", u); // Show beacon number
    Serial.print("B ");
    Serial.print(formatMac1(aps_known[u].bssid));
    Serial.print(" RSSI ");
    Serial.print(aps_known[u].rssi);
    Serial.print(" channel ");
    Serial.print(aps_known[u].channel);

  }

  // show Clients
  for (int u = 0; u < clients_known_count; u++) {
    Serial.printf("%4d ", u); // Show client number
    Serial.print("C ");
    Serial.print(formatMac1(clients_known[u].station));
    Serial.print(" RSSI ");
    Serial.print(clients_known[u].rssi);
    Serial.print(" channel ");
    Serial.print(clients_known[u].channel);
  }
}

void addDevice( String mac,String rssi,String lastDiscoveredTime,String channel, String type,String ssidOrStationMac){
  toSend +="{\"A\":\""+mac+"\","+"\"B\":\""+rssi+"\","+
      "\"C\":\""+lastDiscoveredTime+"\","+"\"D\":\""+channel+"\",\""+
      +"E\":\""+type+"\","+"\"F\":\""+ssidOrStationMac+"\"}";
}


void ICACHE_RAM_ATTR sendDevices() {
  if(digitalRead(2)==LOW){
    digitalWrite(2, HIGH);

  }
  else{
    digitalWrite(2,LOW);
  }
 
  toSend = "#[";
  for (int u = 0; u < aps_known_count; u++) {
      
      addDevice(formatMac1(aps_known[u].bssid),String(aps_known[u].rssi),
      String(aps_known[u].lastDiscoveredTime),String(aps_known[u].channel),"B",formatMac1(aps_known[u].ssid));
      toSend +=",";

      if((u<aps_known_count-1)&&clients_known_count==0){
        toSend +=",";
      }

    }
  

  // Add Clients
  for (int u = 0; u < clients_known_count; u++) {

       addDevice(formatMac1(clients_known[u].bssid),String(clients_known[u].rssi),
      String(clients_known[u].lastDiscoveredTime),String(clients_known[u].channel),"C",formatMac1(clients_known[u].ap));
      if(u<clients_known_count-1){
        toSend +=",";
      }
  
  }
  toSend = toSend+"]$";
  Serial.print(toSend);
  Serial.println("");


  cleanAll();
  
}
