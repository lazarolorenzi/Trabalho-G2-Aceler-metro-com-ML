#include <Arduino.h>
#include <Wire.h>

#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include <utility/imumaths.h>

// === Objeto do BNO055 ===
// 55 = ID do sensor (pode ser qualquer número)
// 0x28 = endereço I2C padrão (ADR em GND)
// Se o pino ADR estiver em 3V3, trocar para 0x29
Adafruit_BNO055 bno = Adafruit_BNO055(55, 0x28);

// === Configuração da taxa de amostragem para o Edge Impulse ===
const float SAMPLE_RATE_HZ = 25.0;                       // taxa de amostragem em Hz
const unsigned long SAMPLE_INTERVAL_MS = 1000.0 / SAMPLE_RATE_HZ;
unsigned long lastSampleTime = 0;

void setup()
{
  Serial.begin(115200);
  while (!Serial) {
    ; // espera a Serial (em algumas placas USB)
  }

  // I2C – se quiser pode usar Wire.begin(21, 22) explicitamente no ESP32
  Wire.begin(23,22); // pinos SDA=23, SCL=22 no ESP32

  // Inicializa o BNO055
  if (!bno.begin()) {
    Serial.println("BNO055 initialization failed!");
    Serial.println("Verifique ligacao (VCC, GND, SDA, SCL) e endereco (0x28/0x29).");
    while (1) {
      delay(1000);
    }
  }

  // Usa cristal externo da placa (se existir) para melhor precisão
  bno.setExtCrystalUse(true);

  // Cabeçalho opcional para o Edge Impulse Data Forwarder
  Serial.println("accX,accY,accZ,steps");
}

void loop()
{
  unsigned long now = millis();
  if (now - lastSampleTime < SAMPLE_INTERVAL_MS) {
    return; // ainda não é hora da próxima amostra
  }
  lastSampleTime = now;

  // --- Leitura da aceleracao ---
  // Você pode escolher:
  // - VECTOR_ACCELEROMETER  -> inclui gravidade
  // - VECTOR_LINEARACCEL    -> sem gravidade (mais útil para movimentos)
  imu::Vector<3> linAccel = bno.getVector(Adafruit_BNO055::VECTOR_LINEARACCEL);

  float accX = linAccel.x();
  float accY = linAccel.y();
  float accZ = linAccel.z();

  // Como o BNO055 nao tem contador de passos interno,
  // mantemos a coluna "steps" com valor 0 só para compatibilidade de formato.
  static uint32_t totalSteps = 0; // se quiser implementar logica de passos depois, usa essa variavel

  // === SAÍDA NO FORMATO PARA O EDGE IMPULSE DATA FORWARDER ===
  Serial.print(accX, 6);
  Serial.print(',');
  Serial.print(accY, 6);
  Serial.print(',');
  Serial.print(accZ, 6);
  Serial.print(',');
  Serial.println(totalSteps);
  delay(50); // pequeno delay para garantir que a Serial envie os dados
}
