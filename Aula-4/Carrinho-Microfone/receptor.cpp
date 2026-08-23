#include <WiFi.h>
#include <esp_now.h>

#define IN1 25
#define IN2 26
#define IN3 27
#define IN4 14
#define ENA 32
#define ENB 33
#define CH_ENA 0
#define CH_ENB 1

typedef struct {
  uint8_t comando;
} Mensagem;

Mensagem msg;

const int VEL_EXTERNA = 255; 
const int VEL_INTERNA = 190; 
const int VEL_RETO    = 255;

const unsigned long DURACAO_MOVIMENTO_MS       = 2000; // frente / tras
const unsigned long DURACAO_MOVIMENTO_MS_LADOS = 1500; // esquerda / direita
const unsigned long TIMEOUT_MS = 5000;

unsigned long inicioMovimento = 0;
unsigned long duracaoAtual = 0;      // duração aplicável ao movimento em curso
unsigned long ultimoRecebimento = 0;
bool emMovimento = false;

void frente() {
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
  ledcWrite(CH_ENA, VEL_RETO);
  ledcWrite(CH_ENB, VEL_RETO);
}

void tras() {
  digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW); digitalWrite(IN4, HIGH);
  ledcWrite(CH_ENA, VEL_RETO);
  ledcWrite(CH_ENB, VEL_RETO);
}

void esquerda() {
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
  ledcWrite(CH_ENA, VEL_INTERNA);
  ledcWrite(CH_ENB, VEL_EXTERNA);
}

void direita() {
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
  ledcWrite(CH_ENA, VEL_EXTERNA);
  ledcWrite(CH_ENB, VEL_INTERNA);
}

void parar() {
  ledcWrite(CH_ENA, 0);
  ledcWrite(CH_ENB, 0);
  emMovimento = false;
}

// centraliza o início de um movimento, já definindo a duração certa pro tipo de comando
void iniciarMovimento(unsigned long duracao) {
  emMovimento = true;
  inicioMovimento = millis();
  duracaoAtual = duracao;
}

void recebido(const uint8_t *mac, const uint8_t *dados, int len) {
  if (len != sizeof(msg)) return;
  memcpy(&msg, dados, sizeof(msg));
  ultimoRecebimento = millis();

  Serial.printf("Comando recebido: %d\n", msg.comando);

  // qualquer comando novo interrompe o movimento atual e começa um novo imediatamente
  switch (msg.comando) {
    case 1: frente();   iniciarMovimento(DURACAO_MOVIMENTO_MS);       break;
    case 2: tras();     iniciarMovimento(DURACAO_MOVIMENTO_MS);       break;
    case 3: esquerda(); iniciarMovimento(DURACAO_MOVIMENTO_MS_LADOS); break;
    case 4: direita();  iniciarMovimento(DURACAO_MOVIMENTO_MS_LADOS); break;
    case 5: parar(); break;
    default: parar(); break;
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  ledcSetup(CH_ENA, 1000, 8);
  ledcAttachPin(ENA, CH_ENA);
  ledcSetup(CH_ENB, 1000, 8);
  ledcAttachPin(ENB, CH_ENB);

  WiFi.mode(WIFI_STA);

  delay(100);
  if (esp_now_init() != ESP_OK) {
    Serial.println("Erro ao iniciar ESP-NOW");
    return;
  }
  esp_now_register_recv_cb(recebido);
  Serial.println("Setup concluido, aguardando comandos de palmas...");
}

void loop() {
  if (emMovimento && (millis() - inicioMovimento > duracaoAtual)) {
    parar();
  }
  if (millis() - ultimoRecebimento > TIMEOUT_MS) {
    parar();
  }
}