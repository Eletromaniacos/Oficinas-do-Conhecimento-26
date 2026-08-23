#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h> 

#define MIC 5   // D0 microfone esquerdo

typedef struct {
  uint8_t comando; // 1=frente 2=tras 3=esquerda 4=direita 5=parar
} Mensagem;

Mensagem msg;

uint8_t enderecoCarrinho[] = {0x08, 0x3a, 0xf2, 0xac, 0x7a, 0x34};

volatile bool pulsoDetectado = false;
volatile unsigned long ultimoPulsoISR = 0;

const unsigned long DEBOUNCE_MS       = 150;  // tempo mínimo entre duas palmas distintas
const unsigned long JANELA_SILENCIO_MS = 1200; // silêncio para "fechar" a contagem de palmas

int contadorPalmas = 0;
unsigned long ultimaPalmaValida = 0;
bool contandoPalmas = false;

void IRAM_ATTR isrMicrofone() {
  unsigned long agora = millis();
  if (agora - ultimoPulsoISR > DEBOUNCE_MS) {
    pulsoDetectado = true;
    ultimoPulsoISR = agora;
  }
}

void enviarComando(uint8_t comando) {
  msg.comando = comando;
  esp_now_send(enderecoCarrinho, (uint8_t*)&msg, sizeof(msg));
  Serial.printf("Palmas: %d -> Comando enviado: %d\n", contadorPalmas, comando);
}

void onSent(const uint8_t *mac, esp_now_send_status_t status) {
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Enviado OK" : "Falha no envio");
}

void setup() {
  Serial.begin(115200);

  pinMode(MIC, INPUT);

  attachInterrupt(digitalPinToInterrupt(MIC),  isrMicrofone, RISING);

  WiFi.mode(WIFI_STA);

  delay(100);

  if (esp_now_init() != ESP_OK) {
    Serial.println("Erro ao iniciar ESP-NOW");
    return;
  }

  esp_now_register_send_cb(onSent);

  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, enderecoCarrinho, 6);
  peer.channel = 0;
  peer.encrypt = false;

  if (esp_now_add_peer(&peer) != ESP_OK) {
    Serial.println("Erro ao adicionar peer");
    return;
  }
}

void loop() {
  if (pulsoDetectado) {
    pulsoDetectado = false;
    contadorPalmas++;
    contandoPalmas = true;
    ultimaPalmaValida = millis();
    Serial.printf("Palma detectada (%d)\n", contadorPalmas);
  }

  // fecha a contagem depois do período de silêncio
  if (contandoPalmas && (millis() - ultimaPalmaValida > JANELA_SILENCIO_MS)) {
    uint8_t comando;

    switch (contadorPalmas) {
      case 1: comando = 1; break; // frente
      case 2: comando = 2; break; // tras
      case 3: comando = 3; break; // esquerda
      case 4: comando = 4; break; // direita
      case 5: comando = 5; break; // parar
      default: comando = 5;       // qualquer excesso -> parar por segurança
    }

    enviarComando(comando);

    contadorPalmas = 0;
    contandoPalmas = false;
  }
}