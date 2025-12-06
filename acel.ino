/* Includes ---------------------------------------------------------------- */
// IMPORTANTE: Substitua pela sua biblioteca
#include <Lazaro-Lorenzi-project-1_inferencing.h>

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include <utility/imumaths.h>

// --- NOVAS BIBLIOTECAS PARA MQTT ---
#include <WiFi.h>
#include <PubSubClient.h>

/* Configurações de Wi-Fi e MQTT ------------------------------------------- */
const char* ssid = "AMF-CORP";          // <<<< COLOQUE O NOME DO SEU WIFI
const char* password = "@MF$4515";     // <<<< COLOQUE A SENHA
const char* mqtt_server = "test.mosquitto.org";        // <<<< IP DO SEU COMPUTADOR/BROKER (ou "broker.hivemq.com" para teste publico)
const int mqtt_port = 1883;
const char* mqtt_topic = "esp32lazaro/movimento";     // Tópico onde a mensagem será publicada

WiFiClient espClient;
PubSubClient client(espClient);

/* Configurações do BNO055 */
#define SDA_PIN 23
#define SCL_PIN 22
Adafruit_BNO055 bno = Adafruit_BNO055(55, 0x28);

/* Configurações dos LEDs */
#define LED_UP_DOWN    32
#define LED_LEFT_RIGHT 33
#define LED_FRONT_BACK 34

/* Variáveis para o Edge Impulse */
float features[EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE]; 

/* Forward Declarations */
void setup_wifi();
void reconnect();
void send_to_mqtt(const char* label, float confidence);

/**
 * @brief      Setup do Arduino
 */
void setup()
{
    Serial.begin(115200);
    while (!Serial);
    
    // --- Configuração Wi-Fi e MQTT ---
    setup_wifi();
    client.setServer(mqtt_server, mqtt_port);

    Serial.println("Edge Impulse Inferencing Demo - BNO055 + MQTT");

    // Inicializa LEDs
    pinMode(LED_UP_DOWN, OUTPUT);
    pinMode(LED_LEFT_RIGHT, OUTPUT);
    pinMode(LED_FRONT_BACK, OUTPUT);

    // I2C e Sensor
    Wire.begin(SDA_PIN, SCL_PIN);
    if (!bno.begin()) {
        Serial.println("Falha ao iniciar o BNO055!");
        while (1) delay(1000);
    }
    bno.setExtCrystalUse(true);

    if (EI_CLASSIFIER_RAW_SAMPLES_PER_FRAME != 3) {
        ei_printf("ERRO: O modelo espera %d eixos.\n", EI_CLASSIFIER_RAW_SAMPLES_PER_FRAME);
        return;
    }
}

/**
 * @brief      Loop Principal
 */
void loop()
{
    // --- Garante conexão MQTT ---
    if (!client.connected()) {
        reconnect();
    }
    client.loop(); // Mantém o MQTT vivo

    // ei_printf("\nIniciando amostragem...\n");

    // 1. Amostragem (Coleta de dados)
    uint64_t next_tick = micros();

    for (int i = 0; i < EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE; i += EI_CLASSIFIER_RAW_SAMPLES_PER_FRAME) {
        next_tick += (uint64_t)EI_CLASSIFIER_INTERVAL_MS * 1000;

        imu::Vector<3> linAccel = bno.getVector(Adafruit_BNO055::VECTOR_LINEARACCEL);

        features[i + 0] = linAccel.x();
        features[i + 1] = linAccel.y();
        features[i + 2] = linAccel.z();

        if (micros() < next_tick) {
            delayMicroseconds(next_tick - micros());
        }
    }

    // 2. Criação do Sinal
    signal_t signal;
    int err = numpy::signal_from_buffer(features, EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE, &signal);
    if (err != 0) {
        ei_printf("Erro no buffer (%d)\n", err);
        return;
    }

    // 3. Inferência
    ei_impulse_result_t result = { 0 };
    err = run_classifier(&signal, &result, false);
    if (err != EI_IMPULSE_OK) {
        ei_printf("Erro na classificação (%d)\n", err);
        return;
    }

    // 4. Processamento da Melhor Predição e Envio MQTT
    process_best_result(&result);
}

/**
 * @brief      Encontra a maior probabilidade e envia para MQTT e LEDs
 */
void process_best_result(ei_impulse_result_t* result)
{
    float best_value = 0.0;
    const char* best_label = "indefinido";

    // Loop para descobrir qual é a maior probabilidade
    for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
        ei_printf("%s: %.2f\n", result->classification[ix].label, result->classification[ix].value);
        
        if (result->classification[ix].value > best_value) {
            best_value = result->classification[ix].value;
            best_label = result->classification[ix].label;
        }
    }

    // Só atuamos se a certeza for maior que 70%
    if (best_value > 0.7) {
        // Controla os LEDs
        control_leds(best_label);
        
        // Envia para o MQTT
        send_to_mqtt(best_label, best_value);
    } else {
        // Opcional: Apagar LEDs se não tiver certeza
        control_leds("nenhum");
    }
}

/**
 * @brief      Controla os LEDs
 */
void control_leds(const char* prediction)
{
    digitalWrite(LED_UP_DOWN, LOW);
    digitalWrite(LED_LEFT_RIGHT, LOW);
    digitalWrite(LED_FRONT_BACK, LOW);

    if (strcmp(prediction, "cima baixo") == 0) {
        digitalWrite(LED_UP_DOWN, HIGH);
    } 
    else if (strcmp(prediction, "lados") == 0) {
        digitalWrite(LED_LEFT_RIGHT, HIGH);
    } 
    else if (strcmp(prediction, "frente tras") == 0) {
        digitalWrite(LED_FRONT_BACK, HIGH);
    }
}

// ==========================================
// FUNÇÕES AUXILIARES DE WIFI E MQTT
// ==========================================

void setup_wifi() {
    delay(10);
    Serial.println();
    Serial.print("Conectando em ");
    Serial.println(ssid);

    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("");
    Serial.println("WiFi conectado");
    Serial.println("IP: ");
    Serial.println(WiFi.localIP());
}

void reconnect() {
    // Loop até reconectar
    while (!client.connected()) {
        Serial.print("Tentando conexão MQTT...");
        // Cria um ID de cliente aleatório
        String clientId = "ESP32Client-";
        clientId += String(random(0xffff), HEX);
        
        if (client.connect(clientId.c_str())) {
            Serial.println("conectado");
        } else {
            Serial.print("falhou, rc=");
            Serial.print(client.state());
            Serial.println(" tentando novamente em 2 segundos");
            delay(2000);
        }
    }
}

void send_to_mqtt(const char* label, float confidence) {
    // Para não inundar o MQTT, podemos usar uma lógica simples de timer ou enviar sempre
    // Aqui enviaremos a string do movimento
    
    char msg[50];
    snprintf(msg, 50, "%s", label); // Envia apenas o nome, ex: "frente tras"
    
    // Se quiser enviar formato JSON:
    // snprintf(msg, 50, "{\"movimento\":\"%s\", \"certeza\":%.2f}", label, confidence);

    client.publish(mqtt_topic, msg);
    Serial.print("MQTT Enviado: ");
    Serial.println(msg);
}
