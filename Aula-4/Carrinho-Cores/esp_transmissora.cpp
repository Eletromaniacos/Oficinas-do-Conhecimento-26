#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>

// MAC da ESP32 receptora (carrinho)
uint8_t macReceptora[] = {
    0x08, 0x3A, 0xF2, 0xAB, 0x6F, 0x68
};

// Pinos do TCS3200
const int S2 = 27;
const int S3 = 26;
const int sensorOut = 13;

// --------------------------------------------------
// Referências de calibração
// Valores RAW medidos diretamente pelo TCS3200
// --------------------------------------------------

struct ReferenciaCor {
    const char* nome;
    long r;
    long g;
    long b;
};

const ReferenciaCor REFERENCIAS[] = {
    {"vermelho", 2,     50,      2},
    {"verde",    15,    2,       1},
    {"azul",     55,    5,       0},
    {"branco",   10,    10,      5},
    {"preto",    30000, 200000, 33000}
};

constexpr int NUM_REFERENCIAS = 5;

// --------------------------------------------------
// Comandos enviados ao carrinho
// --------------------------------------------------

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

Pacote pacote;

// --------------------------------------------------
// Callback de envio
// --------------------------------------------------

void onEnvio(
    const uint8_t* mac,
    esp_now_send_status_t status
) {
    Serial.print("Envio: ");
    Serial.println(
        status == ESP_NOW_SEND_SUCCESS ? "OK" : "FALHA"
    );
}

// --------------------------------------------------
// Distância euclidiana entre a leitura e uma referência
// --------------------------------------------------

double distancia(
    long r,
    long g,
    long b,
    const ReferenciaCor& referencia
) {
    double dr = (double)r - referencia.r;
    double dg = (double)g - referencia.g;
    double db = (double)b - referencia.b;

    return sqrt(
        dr * dr +
        dg * dg +
        db * db
    );
}

// --------------------------------------------------
// Classificação pela referência mais próxima
// --------------------------------------------------

Comando classificarCor(
    long r,
    long g,
    long b
) {
    double menorDistancia = INFINITY;
    int indiceMaisProximo = 0;

    for (int i = 0; i < NUM_REFERENCIAS; i++) {

        double d = distancia(
            r,
            g,
            b,
            REFERENCIAS[i]
        );

        if (d < menorDistancia) {
            menorDistancia = d;
            indiceMaisProximo = i;
        }
    }

    Serial.print("Cor: ");
    Serial.println(REFERENCIAS[indiceMaisProximo].nome);

    switch (indiceMaisProximo) {

            // CMD_FRENTE   = 0,
            // CMD_RE       = 1,
            // CMD_DIREITA  = 2,
            // CMD_ESQUERDA = 3,
            // CMD_PARAR    = 4,

        case 0: // vermelho
            return CMD_RE;

        case 1: // verde
            return CMD_FRENTE;

        case 2: // azul
            return CMD_ESQUERDA;

        case 3: // branco
            return CMD_DIREITA;

        case 4: // preto
            return CMD_PARAR;

        default:
            return CMD_PARAR;
    }
}

// --------------------------------------------------
// Setup
// --------------------------------------------------

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

    memcpy(
        peer.peer_addr,
        macReceptora,
        6
    );

    peer.channel = 0;
    peer.encrypt = false;

    if (esp_now_add_peer(&peer) != ESP_OK) {
        Serial.println("Erro ao adicionar peer");
        return;
    }
}

// --------------------------------------------------
// Loop
// --------------------------------------------------

void loop() {

    long sumR = 0;
    long sumG = 0;
    long sumB = 0;

    // Média de 10 leituras
    for (int i = 0; i < 10; i++) {

        // Vermelho
        digitalWrite(S2, LOW);
        digitalWrite(S3, LOW);

        sumR += pulseIn(sensorOut, LOW);

        // Verde
        digitalWrite(S2, HIGH);
        digitalWrite(S3, HIGH);

        sumG += pulseIn(sensorOut, LOW);

        // Azul
        digitalWrite(S2, LOW);
        digitalWrite(S3, HIGH);

        sumB += pulseIn(sensorOut, LOW);

        delay(50);
    }

    // Valores RAW médios
    long rRaw = sumR / 10;
    long gRaw = sumG / 10;
    long bRaw = sumB / 10;

    // Classifica diretamente usando os valores RAW
    pacote.cmd = classificarCor(
        rRaw,
        gRaw,
        bRaw
    );

    // Envia comando para o carrinho
    esp_now_send(
        macReceptora,
        (uint8_t*)&pacote,
        sizeof(pacote)
    );

    delay(50);
}
