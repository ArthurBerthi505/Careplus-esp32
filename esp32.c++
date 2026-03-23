#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include "MAX30105.h"   // Biblioteca SparkFun para o MAX30102
#include <ArduinoJson.h>

// --- Configurações de Rede ---
const char* ssid = "NOME_DO_SEU_WIFI";
const char* password = "SENHA_DO_WIFI";

// --- URL da sua API ---
const char* serverName = "http://sua-api.com/v1/sinais-vitais";

// --- Definição de Pinos (ESP32-CAM) ---
const int PIN_PIR = 12;    // Sensor de Movimento (Trigger)
const int PIN_TEMP = 14;   // Sensor de Temperatura KY-028 (Analógico)
const int LED_BOARD = 33;  // LED de status interno
// I2C para o MAX30102: SDA=13, SCL=15

MAX30105 particleSensor;

void setup() {
  Serial.begin(115200);
  
  pinMode(PIN_PIR, INPUT);
  pinMode(PIN_TEMP, INPUT);
  pinMode(LED_BOARD, OUTPUT);
  digitalWrite(LED_BOARD, HIGH); // Apaga o LED (Lógica invertida)

  // Conexão Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("Conectando ao Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi Conectado!");

  // Inicializa I2C para o Oxímetro
  Wire.begin(13, 15);
  if (!particleSensor.begin(Wire, I2C_SPEED_STANDARD)) {
    Serial.println("MAX30102 não encontrado!");
  } else {
    particleSensor.setup();
    Serial.println("Sistema Pronto. Aguardando movimento...");
  }
}

void loop() {
  // O sistema espera o sensor PIR detectar movimento (Trigger)
  if (digitalRead(PIN_PIR) == HIGH) {
    Serial.println("\n>>> Movimento detectado! Iniciando leitura de sinais...");
    digitalWrite(LED_BOARD, LOW); // Liga o LED de status
    
    realizarMedicaoEEnviar();
    
    digitalWrite(LED_BOARD, HIGH); // Desliga o LED após o envio
    Serial.println("Aguardando próximo ciclo...");
    delay(10000); // Delay de segurança para estabilizar o PIR e evitar flood na API
  }
}

void realizarMedicaoEEnviar() {
  // 1. Leitura de Temperatura (KY-028)
  int leituraAnalogica = analogRead(PIN_TEMP);
  float voltagem = leituraAnalogica * (3.3 / 4095.0);
  float temperatura = voltagem * 100.0; // Conversão estimada

  // 2. Leitura de Sinais Vitais (MAX30102)
  // Simulação de valores estáveis (em um projeto real, você usaria um loop de amostragem)
  long irValue = particleSensor.getIR();
  int bpm = (irValue > 50000) ? 75 : 0; 
  int spo2 = (irValue > 50000) ? 98 : 0;

  // 3. Preparação e Envio do JSON
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(serverName);
    http.addHeader("Content-Type", "application/json");

    // Criação do Documento JSON
    StaticJsonDocument<256> doc;
    doc["dispositivo"] = "ESP32-CAM-01";
    doc["movimento_detectado"] = true;
    doc["sinais_vitais"]["temperatura"] = temperatura;
    doc["sinais_vitais"]["frequencia_cardiaca"] = bpm;
    doc["sinais_vitais"]["oxigenacao"] = spo2;

    String jsonOutput;
    serializeJson(doc, jsonOutput);

    Serial.print("Enviando JSON: ");
    Serial.println(jsonOutput);

    int httpResponseCode = http.POST(jsonOutput);

    if (httpResponseCode > 0) {
      Serial.print("Sucesso! Código HTTP: ");
      Serial.println(httpResponseCode);
    } else {
      Serial.print("Erro no envio: ");
      Serial.println(httpResponseCode);
    }
    http.end();
  }
}