#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include "message.h"

// =======================================================
// Pinos do driver L298N
// =======================================================

// Motor A 
int motorLPin1  = 27;
int motorLPin2  = 26;
int enableLPin  = 14;

// Motor B 
int motorRPin1  = 32;
int motorRPin2  = 25;
int enableRPin  = 33;

// Propriedades do PWM (LEDC)
// Frequência reduzida: 30kHz gerava mais ruído elétrico na linha de
// alimentação (que atrapalha o rádio Wi-Fi/ESP-NOW) sem necessidade real
// para motores TT. 5kHz é suave o suficiente e mais limpo eletricamente.
const int freq        = 10000;
const int pwmChannelL = 0;
const int pwmChannelR = 1;
const int resolution  = 8; // 0-255

// Estrutura para armazenar os dados recebidos
struct_message receivedData;

// Flag para processar os dados fora do callback do ESP-NOW
volatile bool newDataAvailable = false;
struct_message pendingData;

// Faixas esperadas dos ângulos (em graus)
#define PITCH_MIN -45.0f
#define PITCH_MAX  45.0f

#define ROLL_MIN  -45.0f
#define ROLL_MAX   45.0f

// Zona morta para evitar "tremedeira" do motor perto do centro
#define DEADZONE 25

// Timeout de segurança: se não chegar dado novo em X ms, para os motores.
// Evita o carrinho "correr sozinho" se o link cair.
#define LINK_TIMEOUT_MS 500
unsigned long lastPacketTime = 0;

int mapFloat(float value, float inMin, float inMax, int outMin, int outMax) {
  if (value < inMin) value = inMin;
  if (value > inMax) value = inMax;
  float result = (value - inMin) * (outMax - outMin) / (inMax - inMin) + outMin;
  return (int)result;
}

void setMotor(int pin1, int pin2, int pwmChannel, int speed) {
  speed = constrain(speed, -255, 255);

  if (speed > 0) {
    digitalWrite(pin1, LOW);
    digitalWrite(pin2, HIGH);
  } else if (speed < 0) {
    digitalWrite(pin1, HIGH);
    digitalWrite(pin2, LOW);
  } else {
    digitalWrite(pin1, LOW);
    digitalWrite(pin2, LOW);
  }

  ledcWrite(pwmChannel, abs(speed));
}

// Callback ENXUTO: só copia os dados e sinaliza. Nada de Serial.print,
// nada de matemática, nada de escrever nos motores aqui dentro.
// Isso evita bloquear a task interna do ESP-NOW.
void onDataRecv(const uint8_t *mac_addr, const uint8_t *incomingData, int len) {
  if (len != sizeof(pendingData)) {
    return; // payload corrompido ou de outro tipo, descarta
  }
  memcpy(&pendingData, incomingData, sizeof(pendingData));
  newDataAvailable = true;
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);

  pinMode(motorLPin1, OUTPUT);
  pinMode(motorLPin2, OUTPUT);
  pinMode(enableLPin, OUTPUT);

  pinMode(motorRPin1, OUTPUT);
  pinMode(motorRPin2, OUTPUT);
  pinMode(enableRPin, OUTPUT);

  ledcSetup(pwmChannelL, freq, resolution);
  ledcAttachPin(enableLPin, pwmChannelL);

  ledcSetup(pwmChannelR, freq, resolution);
  ledcAttachPin(enableRPin, pwmChannelR);

  setMotor(motorLPin1, motorLPin2, pwmChannelL, 0);
  setMotor(motorRPin1, motorRPin2, pwmChannelR, 0);

  // Reduz potência de TX do Wi-Fi: menos consumo de corrente nos picos
  // de transmissão, o que ajuda quando a alimentação está no limite.
  // Comente esta linha se precisar de mais alcance.
  WiFi.setTxPower(WIFI_POWER_11dBm);

  if (esp_now_init() != ESP_OK) {
    Serial.println("Erro ao iniciar ESP-NOW");
    return;
  }

  esp_now_register_recv_cb(onDataRecv);

  Serial.println("Receptor pronto, aguardando dados...");
}

void loop() {
  if (newDataAvailable) {
    noInterrupts();
    receivedData = pendingData;
    newDataAvailable = false;
    interrupts();

    lastPacketTime = millis();

    int throttle = mapFloat(receivedData.pitch, PITCH_MIN, PITCH_MAX, -255, 255);
    int steering = mapFloat(receivedData.roll,  ROLL_MIN,  ROLL_MAX,  -255, 255);

    int leftSpeed  = throttle + steering;
    int rightSpeed = throttle - steering;

    leftSpeed  = constrain(leftSpeed,  -255, 255);
    rightSpeed = constrain(rightSpeed, -255, 255);

    if (abs(leftSpeed)  < DEADZONE) leftSpeed  = 0;
    if (abs(rightSpeed) < DEADZONE) rightSpeed = 0;

    setMotor(motorLPin1, motorLPin2, pwmChannelL, leftSpeed);
    setMotor(motorRPin1, motorRPin2, pwmChannelR, rightSpeed);

    // Log fora do callback, não atrapalha a recepção
    Serial.printf("Throttle=%d Steering=%d -> L=%d R=%d\n",
                  throttle, steering, leftSpeed, rightSpeed);
  }

  // Watchdog de link: se ficar tempo demais sem pacote novo, para os motores
  if (millis() - lastPacketTime > LINK_TIMEOUT_MS) {
    setMotor(motorLPin1, motorLPin2, pwmChannelL, 0);
    setMotor(motorRPin1, motorRPin2, pwmChannelR, 0);
  }
}