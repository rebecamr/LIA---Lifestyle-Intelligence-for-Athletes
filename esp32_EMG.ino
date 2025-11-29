// ESP32: leer UART1 desde Arduino y reenviar por TCP al servidor Python
#include <WiFi.h>

HardwareSerial ArduinoSerial(1); // UART1

const char* ssid = "ximena";
const char* password = "1234567890";

const char* pythonIP = "10.11.192.145";
const int pythonPort = 5005;

WiFiClient client;

void setup() {
  Serial.begin(115200);
  ArduinoSerial.begin(9600, SERIAL_8N1, 21, 20); // RX=21, TX=20

  Serial.print("Conectando a WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
    Serial.print(".");
  }
  Serial.println(" conectado.");

  // Intentar conectar TCP al servidor Python (no bloqueante final)
  Serial.print("Conectando al servidor Python ");
  Serial.print(pythonIP); 
  Serial.print(":"); 
  Serial.println(pythonPort);
  if (!client.connect(pythonIP, pythonPort)) {
    Serial.println("Fallo al conectar al servidor Python (intentaré más tarde).");
  } else {
    Serial.println("Conectado al servidor Python.");
  }
}

void loop() {
  // Si perdemos la conexión TCP reintentar
  if (!client.connected()) {
    Serial.println("Reconectando al servidor Python...");
    if (client.connect(pythonIP, pythonPort)) {
      Serial.println("Reconectado.");
    } else {
      Serial.println("Reconexión fallida.");
      delay(1000);
    }
  }

  // Leer líneas completas del Arduino y enviar por WiFi/TCP al servidor Python
  if (ArduinoSerial.available()) {
    String data = ArduinoSerial.readStringUntil('\n'); // lee hasta '\n'
    data.trim();
    if (data.length() > 0) {
      Serial.println("-> " + data); // ver localmente en monitor del ESP32
      if (client.connected()) {
        client.println(data); // manda la línea + '\n' al servidor Python
      }
    }
  }

  delay(1);
}

