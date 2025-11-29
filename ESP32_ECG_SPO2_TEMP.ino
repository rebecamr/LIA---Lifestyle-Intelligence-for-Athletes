#include <WiFi.h>

HardwareSerial SerialArduino(1); // UART1

const int RX_PIN = 21; // ESP32 RX <- Arduino TX
const int TX_PIN = 20; // opcional

// Configuración WiFi
const char* ssid = "ximena";
const char* password = "1234567890";

// Configuración TCP
const char* host = "10.11.192.145"; // IP de tu PC
const int port = 5005;

WiFiClient client;

void setup() {
  Serial.begin(115200);
  SerialArduino.begin(115200, SERIAL_8N1, RX_PIN, TX_PIN);

  Serial.print("Conectando a WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" WiFi conectado!");

  Serial.print("Conectando al servidor TCP...");
  while (!client.connect(host, port)) {
    Serial.println("Intentando conectar...");
    delay(1000);
  }
  Serial.println(" Conectado al servidor TCP!");
}

void loop() {
  // Leer línea completa desde Arduino
  if (SerialArduino.available()) {
    String data = SerialArduino.readStringUntil('\n');
    data.trim();
    if (data.length() > 0) {
      // Mostrar en monitor serial
      Serial.println("Desde Arduino: " + data);

      // Enviar al servidor Python
      if (client.connected()) {
        client.println(data);
      } else {
        // Reconectar si se pierde la conexión
        Serial.println("TCP desconectado, reconectando...");
        while (!client.connect(host, port)) {
          delay(1000);
          Serial.println("Intentando reconectar...");
        }
        Serial.println("Reconectado!");
      }
    }
  }
}
