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
#define MAXDEVICES 80
#define JBUFFER 15+ (MAXDEVICES * 112)
#define PURGETIME 600000
#define MINRSSI -100
#define MIN_SEND_TIME 5*1000
#define MAX_SEND_TIME 30*1000

// uint8_t channel = 1;
unsigned int channel = 1;
int clients_known_count_old, aps_known_count_old;
unsigned long sendEntry, deleteEntry;
char jsonString[JBUFFER];
int SENDTIME = 30000;


String device[MAXDEVICES];
int nbrDevices = 0;
int usedChannels[15];
boolean sendMQTT = false;

StaticJsonBuffer<JBUFFER>  jsonBuffer;

void setup() {
  Serial.begin(115200);
  //Serial.printf("\n\nSDK version:%s\n\r", system_get_sdk_version());
  //Serial.println(F("Human detector by Andreas Spiess. ESP8266 mini-sniff by Ray Burnette http://www.hackster.io/rayburne/projects"));
  //Serial.println(F("Based on the work of Ray Burnette http://www.hackster.io/rayburne/projects"));

  wifi_set_opmode(STATION_MODE);            // Promiscuous works only with station mode
  wifi_set_channel(channel);
  wifi_promiscuous_enable(disable);
  wifi_set_promiscuous_rx_cb(promisc_cb);   // Set up promiscuous callback
  wifi_promiscuous_enable(enable);
  randomSeed(analogRead(0));
  SENDTIME = random(MIN_SEND_TIME, MAX_SEND_TIME);
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

  if (clients_known_count > clients_known_count_old) {
   // Serial.println("Clients: " + clients_known_count);
  }
  if ((millis() - sendEntry > SENDTIME) || exceededMaxClients()) {
    sendEntry = millis();
    sendMQTT = true;
  }
  if (sendMQTT) {
    sendDevices();
    cleanAll();
    SENDTIME = random(MIN_SEND_TIME, MAX_SEND_TIME); //Random Send time recalculated so that not all nodes send at the same time after power outage
  }
}



void cleanAll() {
  memset(aps_known, 0, sizeof(aps_known));
  memset(clients_known, 0, sizeof(clients_known));
  clients_known_count = 0;
  clients_known_count_old = 0;
  aps_known_count_old = 0;
}
void purgeDevice() {

  for (int u = 0; u < clients_known_count; u++) {
    if ((millis() - clients_known[u].lastDiscoveredTime) > PURGETIME) {
      Serial.print("purge Client" );
      Serial.println(u);
      for (int i = u; i < clients_known_count; i++) memcpy(&clients_known[i], &clients_known[i + 1], sizeof(clients_known[i]));
      clients_known_count--;
      break;
    }
  }
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

void sendDevices() {
  String deviceMac;
  // Purge json string
  jsonBuffer.clear();
  JsonObject& root = jsonBuffer.createObject();
  JsonArray& mac = root.createNestedArray("MAC");
  JsonArray& rssi = root.createNestedArray("RSSI");
  JsonArray& milis = root.createNestedArray("MILIS");
  JsonArray& ch = root.createNestedArray("CH");
  JsonArray& type = root.createNestedArray("TYPE");
  JsonArray& ssidOrStationMac = root.createNestedArray("STATION/SSID");
  // add Beacons
  for (int u = 0; u < aps_known_count; u++) {
    deviceMac = formatMac1(aps_known[u].bssid);
    if (aps_known[u].rssi > MINRSSI) {
      mac.add(deviceMac);
      rssi.add(aps_known[u].rssi);
      milis.add(aps_known[u].lastDiscoveredTime);
      ch.add(aps_known[u].channel);
      type.add("B");
      ssidOrStationMac.add(aps_known[u].ssid);

    }
  }

  // Add Clients
  for (int u = 0; u < clients_known_count; u++) {
    deviceMac = formatMac1(clients_known[u].station);
    if (clients_known[u].rssi > MINRSSI) {
      mac.add(deviceMac);
      rssi.add(clients_known[u].rssi);
      milis.add(clients_known[u].lastDiscoveredTime);
      ch.add(clients_known[u].channel);
      if (clients_known[u].channel == -2) {
        type.add("R");
      }
      else {
        type.add("C");
      }
      ssidOrStationMac.add(formatMac1(clients_known[u].ap));


    }
  }
  //Serial.printf("%4d SHOULD BE: \n",aps_known_count + clients_known_count); // show count

  Serial.print("#");
  //Serial.printf("Devices Above RSSI Threshold: %02d\n", mac.size());
  root.printTo(Serial);
  Serial.print("$");
  Serial.println("");
  delay(100);
  sendEntry = millis();
  sendMQTT = false;
}
