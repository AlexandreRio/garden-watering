#include "node_red.h"

/**
 * Water the plants for a given duration
 * @param duration : Duration to water in second
 */
void water(unsigned long duration)
{
  Serial.println("Begin to water");
  digitalWrite(16, LOW);
  digitalWrite(5, HIGH);
  delay(duration*1000);
  digitalWrite(16, HIGH);
  digitalWrite(5, LOW);
  Serial.println("Done watering");
}

/**
 *
 */
String queryServer(const char* host, const int httpAction,  const char* path, const char* param, const char* fingerprint)
{
  WiFiClientSecure client;
  Serial.print("Querying ");
  Serial.println(host);
  String line = "";

  client.setFingerprint(fingerprint);
  if (!client.connect(host, 443)) {
    Serial.println("connection failed");
  }
  else {
    String action;
    if (httpAction == GET) { // yeah it should !httpAction
      action = String("GET");
    } else /* if (httpAction == 1)*/ {
      action = String("POST");
    }
    Serial.println(action);
    client.print(action + " /" + path + " HTTP/1.1\r\n" +
                "Host: " + host + "\r\n" +
                "User-Agent: GardenMonitorESP8266\r\n" +
                "X-Data: " + param + "\r\n"+
                "Connection: close\r\n\r\n");

    while (client.connected()) {
      String line = client.readStringUntil('\n');
      if (line == "\r") {
        Serial.println("headers received");
        break;
      }
    }
    line = client.readStringUntil('\n');

    Serial.println("reply was:");
    Serial.println("==========");
    Serial.println(line);
    Serial.println("==========");
    Serial.println("closing connection");


    }

  return line;
}
