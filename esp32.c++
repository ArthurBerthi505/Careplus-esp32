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

// --- Pinos ESP32-CAM ---
const int PIN_TRIGGER = 12; // Botão para iniciar medição (GPIO 12)
const int PIN_TEMP = 14;    // Entrada analógica KY-028 (GPIO 14)
// I2C: SDA no GPIO 13 e SCL no GPIO 15 (Comum no ESP32-CAM)

MAX30105 particleSensor;

void setup() {
  Serial.begin(115200);
  
  pinMode(PIN_TRIGGER, INPUT_PULLUP);
  
  // Inicializa Wi-Fi
  Serial.print("Conectando ao Wi-Fi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConectado com sucesso!");

  // Inicializa I2C (SDA=13, SCL=15)
  Wire.begin(13, 15);
  
  if (!particleSensor.begin(Wire, I2C_SPEED_STANDARD)) {
    Serial.println("MAX30102 não encontrado. Verifique a fiação!");
  } else {
    particleSensor.setup(); // Configurações padrão do sensor
    Serial.println("Sensores prontos.");
  }
}

void loop() {
  // O código fica em espera até que o botão (trigger) seja pressionado
  if (digitalRead(PIN_TRIGGER) == LOW) { 
    Serial.println("--- Gatilho ativado! Iniciando leitura ---");
    
    realizarMedicao();
    
    Serial.println("Aguardando próximo comando...");
    delay(5000); // Delay de segurança para evitar múltiplos envios
  }
}

void realizarMedicao() {
  // 1. Leitura de Temperatura (KY-028)
  int leituraAnalogica = analogRead(PIN_TEMP);
  float voltagem = leituraAnalogica * (3.3 / 4095.0);
  float temperatura = voltagem * 100.0; 

  // 2. Leitura de Sinais (MAX30102)
  long irValue = particleSensor.getIR(); 
  int bpm = 0;
  int spo2 = 0;

  if (irValue > 50000) { // Verifica se há um dedo no sensor
     bpm = 78;  // Valor exemplo (substitua pela lógica de cálculo da biblioteca)
     spo2 = 98; 
     Serial.println("Leitura realizada com sucesso.");
  } else {
     Serial.println("Dedo não detectado no MAX30102.");
  }

  // 3. Envio para a API
  enviarDadosParaAPI(temperatura, bpm, spo2);
}

void enviarDadosParaAPI(float temp, int bpm, int o2) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(serverName);
    http.addHeader("Content-Type", "application/json");

    // Construção do JSON
    StaticJsonDocument<200> doc;
    doc["dispositivo"] = "ESP32-CAM-01";
    doc["temperatura"] = temp;
    doc["frequencia_cardiaca"] = bpm;
    doc["oxigenacao"] = o2;

    String jsonOutput;
    serializeJson(doc, jsonOutput);

    Serial.print("Enviando JSON: ");
    Serial.println(jsonOutput);

    int httpResponseCode = http.POST(jsonOutput);

    if (httpResponseCode > 0) {
      Serial.print("Sucesso! Resposta da API: ");
      Serial.println(httpResponseCode);
    } else {
      Serial.print("Erro no envio HTTP: ");
      Serial.println(httpResponseCode);
    }
    
    http.end();
  } else {
    Serial.println("Erro: Wi-Fi desconectado.");
  }
}