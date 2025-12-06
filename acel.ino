/* Includes ---------------------------------------------------------------- */
// IMPORTANTE: Substitua a linha abaixo pelo nome da biblioteca que você baixou do Edge Impulse
// Exemplo: #include <Seu_Projeto_inferencing.h>
#include <Lazaro-Lorenzi-project-1_inferencing.h>

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include <utility/imumaths.h>

/* Configurações do BNO055 */
// Pinos I2C definidos para ESP32
#define SDA_PIN 23
#define SCL_PIN 22
Adafruit_BNO055 bno = Adafruit_BNO055(55, 0x28);

/* Configurações dos LEDs */
#define LED_UP_DOWN    32
#define LED_LEFT_RIGHT 33
#define LED_FRONT_BACK 34

/* Variáveis para o Edge Impulse */
// O tamanho do buffer é definido automaticamente pela biblioteca do Edge Impulse
float features[EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE]; 

/**
 * @brief      Setup do Arduino
 */
void setup()
{
    Serial.begin(115200);
    while (!Serial);
    Serial.println("Edge Impulse Inferencing Demo - BNO055");

    // Inicializa LEDs
    pinMode(LED_UP_DOWN, OUTPUT);
    pinMode(LED_LEFT_RIGHT, OUTPUT);
    pinMode(LED_FRONT_BACK, OUTPUT);

    // Garante que o I2C inicie nos pinos corretos ANTES do sensor
    Wire.begin(SDA_PIN, SCL_PIN);

    // Inicializa o BNO055
    if (!bno.begin()) {
        Serial.println("Falha ao iniciar o BNO055! Verifique a fiação.");
        while (1) delay(1000);
    }
    bno.setExtCrystalUse(true);

    // Verifica se o modelo espera o número correto de eixos (deve ser 3: X, Y, Z)
    if (EI_CLASSIFIER_RAW_SAMPLES_PER_FRAME != 3) {
        ei_printf("ERRO: O modelo espera %d eixos, mas o código fornece 3.\n", EI_CLASSIFIER_RAW_SAMPLES_PER_FRAME);
        return;
    }
}

/**
 * @brief      Loop Principal
 */
void loop()
{
    ei_printf("\nIniciando amostragem em %d ms...\n", EI_CLASSIFIER_INTERVAL_MS);

    // 1. Preencher o buffer de dados (Amostragem)
    // Este bloco vai ler o sensor exatamente na frequência que o modelo foi treinado
    uint64_t next_tick = micros();

    for (int i = 0; i < EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE; i += EI_CLASSIFIER_RAW_SAMPLES_PER_FRAME) {
        // Define o tempo da próxima leitura
        next_tick += (uint64_t)EI_CLASSIFIER_INTERVAL_MS * 1000;

        // --- Leitura do Sensor (Idêntica ao seu código de teste) ---
        imu::Vector<3> linAccel = bno.getVector(Adafruit_BNO055::VECTOR_LINEARACCEL);

        // Preenche o buffer (features)
        // NÃO MULTIPLICAR POR 9.8 (O sensor já está em m/s²)
        features[i + 0] = linAccel.x();
        features[i + 1] = linAccel.y();
        features[i + 2] = linAccel.z();

        // Espera até dar o tempo exato da próxima amostra para manter a frequência correta
        if (micros() < next_tick) {
            delayMicroseconds(next_tick - micros());
        }
    }

    // 2. Preparar o sinal para classificação
    signal_t signal;
    int err = numpy::signal_from_buffer(features, EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE, &signal);
    if (err != 0) {
        ei_printf("Erro ao criar sinal do buffer (%d)\n", err);
        return;
    }

    // 3. Executar o Classificador (Inferência)
    ei_impulse_result_t result = { 0 };
    err = run_classifier(&signal, &result, false); // false = sem debug detalhado
    if (err != EI_IMPULSE_OK) {
        ei_printf("ERRO: Falha ao rodar classificador (%d)\n", err);
        return;
    }

    // 4. Mostrar Resultados e Controlar LEDs
    display_results(&result);
}

/**
 * @brief      Exibe o resultado no Serial e aciona LEDs
 */
void display_results(ei_impulse_result_t* result)
{
    // Percorre todas as classes (labels) conhecidas pelo modelo
    for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
        // Imprime: Nome da Classe: Probabilidade
        ei_printf("Classe %s: %.2f\n", result->classification[ix].label, result->classification[ix].value);

        // Se a certeza for maior que 70% (0.7), aciona o LED
        if (result->classification[ix].value > 0.7) {
            control_leds(result->classification[ix].label);
        }
    }
}

/**
 * @brief      Lógica dos LEDs
 * @param[in]  prediction  Nome da classe detectada
 */
void control_leds(const char* prediction)
{
    // Apaga todos primeiro
    digitalWrite(LED_UP_DOWN, LOW);
    digitalWrite(LED_LEFT_RIGHT, LOW);
    digitalWrite(LED_FRONT_BACK, LOW);

    // Compara o nome recebido com os nomes do seu treinamento
    // NOTA: Os nomes devem ser IDÊNTICOS aos do Edge Impulse (Case Sensitive)
    if (strcmp(prediction, "cima baixo") == 0 || strcmp(prediction, "cima baixo") == 0) {
        digitalWrite(LED_UP_DOWN, HIGH);
    } 
    else if (strcmp(prediction, "lados") == 0 || strcmp(prediction, "lados") == 0) {
        digitalWrite(LED_LEFT_RIGHT, HIGH);
    } 
    else if (strcmp(prediction, "frente tras") == 0 || strcmp(prediction, "frente tras") == 0) { // Evite acentos no código se possível
        digitalWrite(LED_FRONT_BACK, HIGH);
    }
}
