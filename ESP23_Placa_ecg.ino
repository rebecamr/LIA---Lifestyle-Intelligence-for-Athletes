#include <WiFi.h>
#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// Establece el nombre del dispositivo (hostname) que aparecerá en la red WiFi
const char* hostname = "ESP32C3_2"; // Nombre único para evitar confusión

// Configuración WiFi
const char* ssid = "ximena";
const char* password = "1234567890";

// Configuración TCP
const char* host = "10.11.192.145";
const int port = 5005;

WiFiClient client;

// ----- Sensores -----
// ECG
const int ECG_PIN = A3;
const int sizearr = 300;
int ecgarray[sizearr];
int ecgIndex = 0;
bool ecgReady = false;

// Temperatura
OneWire ourWire1(2);
DallasTemperature tempSensor(&ourWire1);

// MAX30100 variables
uint16_t irBuffer[100];
uint16_t redBuffer[100];
int bufferIndex = 0;
int lastSpO2 = 0;
float lastTemp = 0;

uint32_t tsLastReport = 0;
bool fingerDetected = false;

void setup() {
  Serial.begin(115200);
 // Establecer el nombre del dispositivo antes de conectar al WiFi
  WiFi.setHostname(hostname);

  // Conectar WiFi
  Serial.print("Conectando a WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("WiFi conectado");
  Serial.print("IP del ESP32: ");
  Serial.println(WiFi.localIP());

  // Conectar al PC por TCP
  Serial.println("Intentando conectar al PC por TCP...");
  while (!client.connect(host, port)) {
    Serial.println("Fallo al conectar al PC, reintentando en 1s...");
    delay(1000);
  }
  Serial.println("Conectado al PC por TCP");

  // Sensores
  tempSensor.begin();
  Wire.begin();
  // Inicialización MAX30100 igual que tu código anterior
}

void loop() {
  unsigned long currentMillis = millis();

  // --- Leer MAX30100 y ECG como antes ---
  // Aquí iría tu código para calcular SpO2, leer ECG y temperatura
  // Suponiendo que ya se llenó ecgarray y lastSpO2 y lastTemp están actualizados
  if (!ecgReady && ecgIndex < sizearr) {
    ecgarray[ecgIndex] = analogRead(ECG_PIN);
    ecgIndex++;
    if (ecgIndex >= sizearr) {
      ecgReady = true;
    }
  }

  // --- Enviar datos cada 1 segundo ---
  if (ecgReady && (currentMillis - tsLastReport >= 1000)) {
    tsLastReport = currentMillis;

    tempSensor.requestTemperatures();
    lastTemp = tempSensor.getTempCByIndex(0);

    // Crear JSON
    String jsonData = "{";
    jsonData += "\"timestamp_ms\":" + String(currentMillis) + ",";
    jsonData += "\"ecg\":[";
    for (int i = 0; i < sizearr; i++) {
      jsonData += String(ecgarray[i]);
      if (i < sizearr - 1) jsonData += ",";
    }
    jsonData += "],";
    jsonData += "\"spo2\":" + String(lastSpO2) + ",";
    jsonData += "\"temp_c\":" + String(lastTemp,1);
    jsonData += "}";

    // ----- Imprimir en Serial -----
    Serial.println("---- Datos a enviar ----");
    Serial.println(jsonData);
    Serial.println("-----------------------");

    // Enviar por TCP
    if (client.connected()) {
      client.println(jsonData);
    } else {
      Serial.println("⚠ No conectado al PC, datos no enviados");
    }

    // Reset ECG
    ecgIndex = 0;
    ecgReady = false;
  }

  delayMicroseconds(500);
}

