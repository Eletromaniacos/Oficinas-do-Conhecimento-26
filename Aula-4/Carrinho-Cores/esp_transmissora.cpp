#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>

// MAC da ESP32 receptora (carrinho) — substitua pelos valores reais
uint8_t macReceptora[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// Pinos do TCS3200
const int S2 = 27;
const int S3 = 26;
const int sensorOut = 13;

// Calibração: ajuste conforme seu ambiente
const long CAL_R_BRANCO = 20,  CAL_R_PRETO = 150;
const long CAL_G_BRANCO = 20,  CAL_G_PRETO = 232;
const long CAL_B_BRANCO = 20,  CAL_B_PRETO = 250;

// Limiares para classificação de cor (0–255 após calibração)
// Ajuste conforme os valores RGB que seu sensor produz para cada LED
const int LIMIAR_BRANCO_MIN = 200; // R, G e B acima disso → branco
const int LIMIAR_PRETO_MAX  = 30;  // R, G e B abaixo disso → preto
const int LIMIAR_COR_MIN    = 100; // canal dominante deve superar isso

// Comandos enviados ao carrinho
typedef enum : uint8_t {
  CMD_FRENTE  = 0,
  CMD_RE      = 1,
  CMD_DIREITA = 2,
  CMD_ESQUERDA= 3,
  CMD_PARAR   = 4,
} Comando;

typedef struct {
  Comando cmd;
} Pacote;

Pacote pacote;

void onEnvio(const uint8_t *mac, esp_now_send_status_t status) {
  // Callback opcional — útil para debug
  Serial.print("Envio: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "OK" : "FALHA");
}

int mapear(long val, long branco, long preto) {
  val = max(branco, min(preto, val));
  float normalizado = (float)(val - branco) / (preto - branco);
  return round((1.0 - normalizado) * 255);
}

Comando classificarCor(int r, int g, int b) {
  if (r > LIMIAR_BRANCO_MIN && g > LIMIAR_BRANCO_MIN && b > LIMIAR_BRANCO_MIN)
    return CMD_FRENTE;
  if (r < LIMIAR_PRETO_MAX && g < LIMIAR_PRETO_MAX && b < LIMIAR_PRETO_MAX)
    return CMD_PARAR;
  if (r > g && r > b && r > LIMIAR_COR_MIN)
    return CMD_RE;
  if (g > r && g > b && g > LIMIAR_COR_MIN)
    return CMD_DIREITA;
  if (b > r && b > g && b > LIMIAR_COR_MIN)
    return CMD_ESQUERDA;
  return CMD_PARAR;
}

void setup() {
  Serial.begin(9600);

  pinMode(S2, OUTPUT);
  pinMode(S3, OUTPUT);
  pinMode(sensorOut, INPUT);

  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    Serial.println("Erro ao inicializar ESP-NOW");
    return;
  }
  esp_now_register_send_cb(onEnvio);

  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, macReceptora, 6);
  peer.channel = 0;
  peer.encrypt = false;
  esp_now_add_peer(&peer);
}

void loop() {
  long sumR = 0, sumG = 0, sumB = 0;

  for (int i = 0; i < 10; i++) {
    digitalWrite(S2, LOW);
    digitalWrite(S3, LOW);
    sumR += pulseIn(sensorOut, LOW);

    digitalWrite(S2, HIGH);
    digitalWrite(S3, HIGH);
    sumG += pulseIn(sensorOut, LOW);

    digitalWrite(S2, LOW);
    digitalWrite(S3, HIGH);
    sumB += pulseIn(sensorOut, LOW);

    delay(100);
  }

  int r = mapear(sumR / 10, CAL_R_BRANCO, CAL_R_PRETO);
  int g = mapear(sumG / 10, CAL_G_BRANCO, CAL_G_PRETO);
  int b = mapear(sumB / 10, CAL_B_BRANCO, CAL_B_PRETO);

  Serial.printf("RGB: (%d, %d, %d)\n", r, g, b);

  pacote.cmd = classificarCor(r, g, b);
  esp_now_send(macReceptora, (uint8_t *)&pacote, sizeof(pacote));

  delay(200);
}
