#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>

// Pinos L298N
const int IN1 = 14;
const int IN2 = 27;
const int IN3 = 26;
const int IN4 = 25;
const int ENA = 32;
const int ENB = 33;

// Canais PWM (ESP32 exige canal lógico para ledcWrite)
const int CANAL_A = 0;
const int CANAL_B = 1;
const int PWM_FREQ = 1000;   // Hz
const int PWM_RES  = 8;      // bits (0–255)

// Velocidades — ajuste conforme necessário
const int VEL_CRUZEIRO = 200; // velocidade padrão (0–255)
const int VEL_CURVA    = 100; // velocidade do motor reduzido na curva

typedef enum : uint8_t {
  CMD_FRENTE   = 0,
  CMD_RE       = 1,
  CMD_DIREITA  = 2,
  CMD_ESQUERDA = 3,
  CMD_PARAR    = 4,
} Comando;

typedef struct {
  Comando cmd;
} Pacote;

void pararMotores() {
  ledcWrite(CANAL_A, 0);
  ledcWrite(CANAL_B, 0);
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}

void frente() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  ledcWrite(CANAL_A, VEL_CRUZEIRO);
  ledcWrite(CANAL_B, VEL_CRUZEIRO);
}

void re() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
  ledcWrite(CANAL_A, VEL_CRUZEIRO);
  ledcWrite(CANAL_B, VEL_CRUZEIRO);
}

void direita() {
  // Motor esquerdo em cruzeiro, motor direito reduzido
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  ledcWrite(CANAL_A, VEL_CRUZEIRO); // motor esquerdo
  ledcWrite(CANAL_B, VEL_CURVA);    // motor direito
}

void esquerda() {
  // Motor direito em cruzeiro, motor esquerdo reduzido
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  ledcWrite(CANAL_A, VEL_CURVA);    // motor esquerdo
  ledcWrite(CANAL_B, VEL_CRUZEIRO); // motor direito
}

void onRecebimento(const uint8_t *mac, const uint8_t *dados, int len) {
  if (len != sizeof(Pacote)) return;
  Pacote pacote;
  memcpy(&pacote, dados, sizeof(Pacote));

  switch (pacote.cmd) {
    case CMD_FRENTE:   frente();       break;
    case CMD_RE:       re();           break;
    case CMD_DIREITA:  direita();      break;
    case CMD_ESQUERDA: esquerda();     break;
    case CMD_PARAR:    pararMotores(); break;
  }
}

void setup() {
  Serial.begin(9600);

  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  ledcSetup(CANAL_A, PWM_FREQ, PWM_RES);
  ledcSetup(CANAL_B, PWM_FREQ, PWM_RES);
  ledcAttachPin(ENA, CANAL_A);
  ledcAttachPin(ENB, CANAL_B);

  pararMotores();

  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    Serial.println("Erro ao inicializar ESP-NOW");
    return;
  }
  esp_now_register_recv_cb(onRecebimento);

  Serial.print("MAC desta ESP32: ");
  Serial.println(WiFi.macAddress());
}

void loop() {
  // Toda a lógica é orientada a eventos via callback ESP-NOW
}
