// Copyright Nat Weerawan 2015-2016
// MIT License
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <WiFiClientSecure.h>

#include "node_red.h"

const char* host = HTTP_SERVER_HOST;
const char* fingerprint = HTTP_SERVER_FINGERPRINT;

unsigned int localPort = 2390;      // local port to listen for UDP packets
IPAddress timeServerIP;
const char* ntpServerName = "0.fr.pool.ntp.org";
const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message
byte packetBuffer[ NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets
WiFiUDP wifiUdp;
char textBuffer[50];

unsigned long last_seen_epoch;
unsigned long next_epoch;

void init_hardware()
{
  Serial.begin(115200);
  pinMode(16,OUTPUT);
  pinMode(5,OUTPUT);
  digitalWrite(16, HIGH);
  digitalWrite(5, LOW);
  delay(1000);
  Serial.flush();
  Serial.println();
  Serial.println();
  Serial.println("will be started in 500ms..");
}

void init_wifi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");

  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  Serial.println("Starting UDP");
  wifiUdp.begin(localPort);
  Serial.print("Local port: ");
  Serial.println(wifiUdp.localPort());
}

// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress& address) {
  Serial.println("sending NTP packet...");
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  wifiUdp.beginPacket(address, 123); //NTP requests are to port 123
  wifiUdp.write(packetBuffer, NTP_PACKET_SIZE);
  wifiUdp.endPacket();
}

void printTimeFromEpoch(unsigned long& epoch)
{
  // print the hour, minute and second:
  Serial.print("The UTC time is ");       // UTC is the time at Greenwich Meridian (GMT)
  Serial.print((epoch  % 86400L) / 3600); // print the hour (86400 equals secs per day)
  Serial.print(':');
  if (((epoch % 3600) / 60) < 10) {
    // In the first 10 minutes of each hour, we'll want a leading '0'
    Serial.print('0');
  }
  Serial.print((epoch  % 3600) / 60); // print the minute (3600 equals secs per minute)
  Serial.print(':');
  if ((epoch % 60) < 10) {
    // In the first 10 seconds of each minute, we'll want a leading '0'
    Serial.print('0');
  }
  Serial.println(epoch % 60); // print the second
}

void setup()
{
  init_hardware();
  init_wifi();
  delay(1000); // wait 1s
}

void loop()
{
  WiFi.hostByName(ntpServerName, timeServerIP);
  sendNTPpacket(timeServerIP); // send an NTP packet to a time server
  delay(1000);

  int cb = wifiUdp.parsePacket();
  if (!cb) {
    Serial.println("no packet yet");
  } else {
    Serial.print("packet received, length=");
    Serial.println(cb);
    wifiUdp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer

    //the timestamp starts at byte 40 of the received packet and is four bytes,
    // or two words, long. First, esxtract the two words:
    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
    // combine the four bytes (two words) into a long integer
    // this is NTP time (seconds since Jan 1 1900):
    unsigned long secsSince1900 = highWord << 16 | lowWord;
    // Serial.print("Seconds since Jan 1 1900 = ");
    // Serial.println(secsSince1900);

    // now convert NTP time into everyday time:
    Serial.print("Unix time = ");
    // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
    const unsigned long seventyYears = 2208988800UL;
    // subtract seventy years:
    unsigned long epoch = secsSince1900 - seventyYears;

    last_seen_epoch = epoch;
    if (next_epoch < last_seen_epoch) {
      // Water the plants
      int duration = 40;
      String(duration).toCharArray(textBuffer, 50); // put "40" in textBuffer to use as query parameter
      water(duration);
      queryServer(host, GET, "water", textBuffer, fingerprint);
      next_epoch = last_seen_epoch + 86400; // 24 hours
    } else {
      // No action, query to update status
      Serial.print(last_seen_epoch);
      Serial.print(" Too soon, next ");
      String(next_epoch).toCharArray(textBuffer, 50);
      Serial.println(next_epoch);
      queryServer(host, POST, "status", textBuffer, fingerprint);
    }
    //printTimeFromEpoch(epoch);
  }
  delay(10000);

}
