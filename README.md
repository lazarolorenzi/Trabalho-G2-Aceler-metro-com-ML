# Classificação de Movimento com Machine Learning (ESP32 + BNO055)

Este projeto implementa um sistema de detecção de movimentos baseado em Machine Learning (TinyML) utilizando um microcontrolador ESP32 e um sensor de orientação absoluta BNO055. O modelo foi treinado no Edge Impulse para classificar movimentos físicos (Cima, Baixo, Esquerda, Direita, Frente, Trás) e acionar LEDs correspondentes em tempo real.

## 📂 Estrutura do Projeto

O repositório contém dois códigos principais que representam as etapas do ciclo de vida do projeto:

* **`Leitura.cpp` (Coleta de Dados):**
    * Este código foi utilizado na etapa inicial para capturar os dados brutos do acelerômetro (eixos X, Y, Z).
    * Ele formata os dados para serem enviados ao *Edge Impulse Data Forwarder*, permitindo a criação do dataset de treinamento.

* **`acel.ino` (Inferência / Aplicação Final):**
    * Este é o firmware final carregado no ESP32.
    * Ele contém a biblioteca do modelo treinado, realiza a leitura dos sensores em tempo real, executa a inferência e controla os LEDs baseados na predição do modelo.

## 🛠️ Hardware Utilizado

* **Microcontrolador:** ESP32 (DevKit V1)
* **Sensor:** Adafruit BNO055 (Acelerômetro/Giroscópio)
* **Atuadores:** 3x LEDs (Indicadores de direção)
* **Comunicação:** I2C

### Esquema de Ligação (Pinagem)

| BNO055 Pin | ESP32 Pin | Função |
| :--- | :--- | :--- |
| VIN | 3V3 | Alimentação |
| GND | GND | Terra |
| SDA | GPIO 23 | Dados I2C |
| SCL | GPIO 22 | Clock I2C |

**LEDs de Saída:**
* **Cima/Baixo:** GPIO 32
* **Esquerda/Direita:** GPIO 33
* **Frente/Trás:** GPIO 25

## 🧠 Treinamento e Performance do Modelo

O modelo foi desenvolvido na plataforma Edge Impulse utilizando uma rede neural para classificação. Abaixo estão os resultados obtidos durante a fase de validação.

### Acurácia e Matriz de Confusão
A imagem abaixo demonstra a performance do modelo, evidenciando a acurácia (Accuracy) e a perda (Loss) durante as épocas de treinamento, bem como a Matriz de Confusão que mostra o quão bem o modelo distingue cada classe.

<img width="1561" height="767" alt="Matriz de Confusão e Acurácia" src="https://github.com/user-attachments/assets/1541eb0b-5d8d-4ed8-ab70-92c3449b559e" />

## 🚀 Resultados em Tempo Real

Após o deploy do modelo para o ESP32 (arquivo `acel.ino`), o sistema é capaz de classificar os movimentos continuamente.

### Monitor Serial
Abaixo, um exemplo da saída do Monitor Serial, mostrando as probabilidades (confidence score) atribuídas a cada classe em tempo real. O código foi programado para acionar os LEDs apenas quando a certeza da inferência ultrapassa 70%.

<img width="361" height="309" alt="Monitor Serial Output" src="https://github.com/user-attachments/assets/a311ee9e-dd4d-4b44-a15d-1d16b0435e1c" />

## 📦 Como Reproduzir

1.  **Instalação de Bibliotecas:**
    * Instale as bibliotecas da Adafruit para o BNO055 na Arduino IDE.
    * Importe a biblioteca do Edge Impulse (gerada no seu projeto) como `.zip`.

2.  **Configuração:**
    * Certifique-se de que os pinos I2C (SDA/SCL) estão corretos no `acel.ino`.
    * Verifique se a frequência de amostragem no código condiz com a usada no treinamento (padrão BNO055/Edge Impulse).

3.  **Upload:**
    * Carregue o `acel.ino` para o ESP32.
    * Abra o monitor serial (115200 baud) e movimente o sensor.

---
*Desenvolvido com Edge Impulse e ESP32.*
