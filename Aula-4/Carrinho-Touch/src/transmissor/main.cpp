#include <WiFi.h>
#include <esp_now.h>
 
#define PIN_TOUCH 33
#define BOTAO 32
#define BANANA 34
#define GRAFITE 35

const int LIMIAR_GRAFITE = 3800; // "pressiona" abaixo desse valor
const int LIMIAR_BANANA  = 3800; // "pressiona" abaixo desse valor

uint8_t enderecoReceptor[] = {0x08, 0x3A, 0xF2, 0xAB, 0x4C, 0x90};
 
typedef struct {
  int16_t velocidadeA; // -255 a 255
  int16_t velocidadeB; // -255 a 255
} Comando;
 
Comando comando;
uint32_t ultimoEnvio = 0;
uint32_t ultimoEnvioPrint = 0;
 
void aoEnviar(const uint8_t *mac, esp_now_send_status_t status) {
  if (status != ESP_NOW_SEND_SUCCESS) Serial.println("Falha no envio");
}
 
void frente() {
   comando.velocidadeA = 255;
  comando.velocidadeB = 255;
}
 
void parar() {
 
  comando.velocidadeA = 0;
  comando.velocidadeB = 0;
 
}
 
void tras() {
 
  comando.velocidadeA = -255;
  comando.velocidadeB = -255;
 
}
 
void esquerda() {
 
  comando.velocidadeA = 255;
  comando.velocidadeB = -255;
}
 
void direita() {
 
  comando.velocidadeA = -255;
  comando.velocidadeB = 255;
 
}
 
void setup() {
  Serial.begin(115200);

  WiFi.mode(WIFI_STA);
 
  if (esp_now_init() != ESP_OK) {
    Serial.println("Erro ao iniciar ESP-NOW");
    return;
  }
  esp_now_register_send_cb(aoEnviar);
 
  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, enderecoReceptor, 6);
  peer.channel = 0;
  peer.encrypt = false;
  if (esp_now_add_peer(&peer) != ESP_OK) Serial.println("Erro ao adicionar peer");
 
  pinMode(BOTAO, INPUT_PULLDOWN);
  pinMode(BANANA, INPUT);
  pinMode(GRAFITE, INPUT);
  pinMode(PIN_TOUCH, INPUT);
}
 
void loop() {

  // --- CALIBRAÇÃO ---
  int leitura_grafite = analogRead(GRAFITE);
  int leitura_banana  = analogRead(BANANA);

  // --- LÓGICA DE CONTROLE ---
  if (digitalRead(PIN_TOUCH) == HIGH)
  {
    frente();
    Serial.printf("Frente!\n");
  }
  else if(digitalRead(BOTAO) == HIGH)
  {
    tras();
    Serial.printf("Trás!\n");
  }
  else if(leitura_banana <= LIMIAR_BANANA)
  {
    direita();
    Serial.printf("Direita!\n");
  }
  else if(leitura_grafite <= LIMIAR_GRAFITE)
  {
    esquerda();
    Serial.printf("Esquerda!\n");
  }
  else
  {
    parar();
  }

  if (millis() - ultimoEnvio >= 50) {
    ultimoEnvio = millis();
    esp_now_send(enderecoReceptor, (uint8_t *)&comando, sizeof(comando));
  }

  if (millis() - ultimoEnvioPrint >= 400) {
    ultimoEnvioPrint = millis();
    Serial.printf("[CALIB] Grafite: %d\n", leitura_grafite);
    Serial.printf("[CALIB] Banana:  %d\n", leitura_banana);
  }
}