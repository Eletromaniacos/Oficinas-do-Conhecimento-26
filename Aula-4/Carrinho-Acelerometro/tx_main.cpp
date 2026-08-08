// src/tx_main.cpp
#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
#include <math.h>
#include "message.h"

// Troque pelos MACs reais dos RECEPTORES (obtidos com o sketch get_mac.cpp)
uint8_t receiverMac[]  = {0x08, 0x3A, 0xF2, 0xAB, 0x52, 0xC0}; // receptor 1
uint8_t receiverMac2[] = {0x08, 0x3A, 0xF2, 0xAC, 0x40, 0x58}; // receptor 2
uint8_t receiverMac3[] = {0x08, 0x3A, 0xF2, 0xAC, 0x5E, 0xA0}; // receptor 3

struct_message myData;
esp_now_peer_info_t peerInfo;
Adafruit_MPU6050 mpu;

// Ângulos estimados
float pitch = 0.0f;
float roll  = 0.0f;
float yaw   = 0.0f;

unsigned long lastTime = 0;

// Peso do filtro complementar (quanto maior, mais confia no giroscópio).
// Para um carrinho rápido, um valor um pouco menor deixa a resposta mais ágil.
const float ALPHA = 0.95f;

void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("Status do envio: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Sucesso" : "Falha");
}

// Adiciona um peer ESP-NOW e trata erro sem travar o setup
bool addPeer(const uint8_t *mac) {
  memcpy(peerInfo.peer_addr, mac, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  return esp_now_add_peer(&peerInfo) == ESP_OK;
}

void setup() {
  Serial.begin(115200);
  delay(1000); // dá tempo do monitor serial conectar antes do resto rodar

  Serial.println("Iniciando Wire...");
  Wire.begin(21, 22);
  Wire.setTimeOut(1000); // evita travar pra sempre se o barramento I2C não responder
  Serial.println("Wire OK, escaneando I2C...");

  bool foundDevice = false;
  for (uint8_t addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      Serial.print("Dispositivo encontrado em 0x");
      Serial.println(addr, HEX);
      foundDevice = true;
    }
  }
  if (!foundDevice) {
    Serial.println("Nenhum dispositivo I2C encontrado! Verifique fiação/pull-ups.");
  }

  Serial.println("Chamando mpu.begin()...");
  if (!mpu.begin()) {
    Serial.println("Falha ao conectar o módulo MPU6050");
    while (1) {
      delay(10);
    }
  }

  // Para um carrinho rápido, prioriza sensibilidade e resposta mais ágil.
  mpu.setAccelerometerRange(MPU6050_RANGE_4_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_44_HZ);
  Serial.println("Módulo MPU6050 conectado");

  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("Erro ao iniciar ESP-NOW");
    return;
  }

  esp_now_register_send_cb(onDataSent);

  if (!addPeer(receiverMac))  Serial.println("Falha ao adicionar peer 1");
  if (!addPeer(receiverMac2)) Serial.println("Falha ao adicionar peer 2");
  if (!addPeer(receiverMac3)) Serial.println("Falha ao adicionar peer 3");

  Serial.println("Transmissor pronto.");
  lastTime = millis();
}

void loop() {
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  unsigned long now = millis();
  float dt = (now - lastTime) / 1000.0f;
  lastTime = now;

  // --- Pitch e Roll a partir do acelerômetro (em graus) ---
  float accelPitch = atan2(-a.acceleration.x,
                            sqrt(a.acceleration.y * a.acceleration.y +
                                 a.acceleration.z * a.acceleration.z)) * 180.0f / PI;

  float accelRoll = atan2(a.acceleration.y, a.acceleration.z) * 180.0f / PI;

  // --- Integração do giroscópio (rad/s -> graus) ---
  float gyroPitchRate = g.gyro.y * 180.0f / PI;
  float gyroRollRate  = g.gyro.x * 180.0f / PI;
  float gyroYawRate   = g.gyro.z * 180.0f / PI;

  // --- Filtro complementar para pitch e roll ---
  pitch = ALPHA * (pitch + gyroPitchRate * dt) + (1 - ALPHA) * accelPitch;
  roll  = ALPHA * (roll + gyroRollRate * dt) + (1 - ALPHA) * accelRoll;

  // --- Yaw: só integração do giroscópio (vai ter drift ao longo do tempo) ---
  yaw += gyroYawRate * dt;

  myData.counter++;
  myData.pitch = pitch;
  myData.yaw   = yaw;
  myData.roll  = roll;

  esp_err_t result1 = esp_now_send(receiverMac,  (uint8_t *) &myData, sizeof(myData));
  esp_err_t result2 = esp_now_send(receiverMac2, (uint8_t *) &myData, sizeof(myData));
  esp_err_t result3 = esp_now_send(receiverMac3, (uint8_t *) &myData, sizeof(myData));

  if (result1 == ESP_OK && result2 == ESP_OK && result3 == ESP_OK) {
    Serial.print("Enviado #");
    Serial.print(myData.counter);
    Serial.print(" -> Pitch: ");
    Serial.print(myData.pitch);
    Serial.print(", Yaw: ");
    Serial.print(myData.yaw);
    Serial.print(", Roll: ");
    Serial.println(myData.roll);
  } else {
    if (result1 != ESP_OK) Serial.println("Erro ao enviar para o peer 1");
    if (result2 != ESP_OK) Serial.println("Erro ao enviar para o peer 2");
    if (result3 != ESP_OK) Serial.println("Erro ao enviar para o peer 3");
  }

  delay(100);
}