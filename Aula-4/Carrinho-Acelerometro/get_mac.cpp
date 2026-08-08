#include <Arduino.h>
#include <WiFi.h>

void setup() {
  Serial.begin(115200);
  delay(3000); // dá tempo do monitor serial conectar
  WiFi.mode(WIFI_STA);

  Serial.println();
  Serial.print("MAC Address: ");
  Serial.println(WiFi.macAddress());
}

void loop() {}