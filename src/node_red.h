#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <WiFiClientSecure.h>

#define GET 0
#define POST 1

void water(unsigned long duration);
String queryServer(const char* host, const int httpAction, const char* path, const char* param, const char* fingerprint);