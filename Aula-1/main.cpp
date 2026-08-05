#include <Arduino.h>

const int pinosAnalogicos[] = {34, 35, 32, 33, 25, 26}; // D34 D35 D32 D33 D25 D26
const int numPinos = 6; 

void setup() {
  Serial.begin(9600);

  for (int i = 0; i < numPinos; i++) {
    pinMode(pinosAnalogicos[i], INPUT);
  }
}

void loop() {
  for (int i = 0; i < numPinos; i++) {
    int leitura = analogRead(pinosAnalogicos[i]); 
    
    Serial.print("T");
    Serial.print(i + 1); 
    Serial.print(": ");
    Serial.print(leitura);
    
    if (i < numPinos - 1) {
      Serial.print(" ");
    }
  }

  Serial.println();
  
  delay(1); 
}