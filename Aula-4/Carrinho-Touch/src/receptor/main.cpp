#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>

#define PINO_ENA 14
#define PINO_IN1 27
#define PINO_IN2 26
#define PINO_IN3 25
#define PINO_IN4 33
#define PINO_ENB 32

typedef struct {
  int16_t velocidadeA;
  int16_t velocidadeB;
} Comando;

Comando comando;
unsigned long ultimoPacote = 0;

void motorA(int v) {
  bool frente = v >= 0;
  digitalWrite(PINO_IN1, frente);
  digitalWrite(PINO_IN2, !frente);
  analogWrite(PINO_ENA, constrain(abs(v), 0, 255));
}

void motorB(int v) {
  bool frente = v >= 0;
  digitalWrite(PINO_IN3, frente);
  digitalWrite(PINO_IN4, !frente);
  analogWrite(PINO_ENB, constrain(abs(v), 0, 255));
}

void aoReceber(const uint8_t *mac, const uint8_t *dados, int tamanho) {
  if (tamanho != sizeof(Comando)) return;
  memcpy(&comando, dados, sizeof(comando));
  ultimoPacote = millis();
  motorA(comando.velocidadeA);
  motorB(comando.velocidadeB);
}

void setup() {
  Serial.begin(115200);
  pinMode(PINO_IN1, OUTPUT); pinMode(PINO_IN2, OUTPUT);
  pinMode(PINO_IN3, OUTPUT); pinMode(PINO_IN4, OUTPUT);
  pinMode(PINO_ENA, OUTPUT); pinMode(PINO_ENB, OUTPUT);
  analogWriteFrequency(5000);

  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    Serial.println("Erro ao iniciar ESP-NOW");
    return;
  }
  esp_now_register_recv_cb(aoReceber);
}

void loop() {
  // Failsafe: para os motores se perder o sinal por 500 ms
  if (millis() - ultimoPacote > 500) {
    motorA(0);
    motorB(0);
  }
}