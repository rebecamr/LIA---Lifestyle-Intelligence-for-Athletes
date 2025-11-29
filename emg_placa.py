import socket
import json
import csv
import time
import paho.mqtt.client as mqtt
import keyboard   # pip install keyboard
import os
from datetime import datetime

# ============================
# CONFIGURACIÓN
# ============================

HOST = "0.0.0.0"  # Escuchar en toda la red
PORT = 5005       # Puerto donde se conecta el ESP32

# MQTT
MQTT_BROKER = "broker.hivemq.com"
MQTT_PORT = 1883
MQTT_TOPIC = "ximena/ecg"

# Carpeta donde guardar CSV
SAVE_FOLDER = r"C:\Users\Ximena\OneDrive\Escritorio\Grabaciones LIA"
os.makedirs(SAVE_FOLDER, exist_ok=True)

# ============================
# MQTT CLIENT
# ============================
mqtt_client = mqtt.Client()
mqtt_client.connect(MQTT_BROKER, MQTT_PORT, 60)

# ============================
# SERVIDOR TCP
# ============================
server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
server.bind((HOST, PORT))
server.listen(1)

print(f"Servidor escuchando en {HOST}:{PORT}")
conn, addr = server.accept()
print(f"ESP32 conectado desde {addr}")

buffer = ""
recording = False
csv_file = None
writer = None

print("\nPresiona ENTER para iniciar la grabación...")
print("Presiona ENTER nuevamente para detener.\n")

# ============================
# LOOP PRINCIPAL
# ============================
try:
    while True:
        # -------- TECLA PARA START / STOP -----------
        if keyboard.is_pressed("enter"):
            time.sleep(0.3)  # evitar rebote

            if not recording:
                # INICIAR GRABACIÓN
                recording = True
                timestamp_str = datetime.now().strftime("%Y%m%d_%H%M%S")
                filename = f"EMG_{timestamp_str}.csv"
                filepath = os.path.join(SAVE_FOLDER, filename)

                csv_file = open(filepath, "w", newline="")
                writer = csv.writer(csv_file)

                headers = ["timestamp_us"] + [f"emg_{i+1}" for i in range(600)]
                writer.writerow(headers)

                print(f"\n🔴 GRABACIÓN INICIADA: {filename}\n")

            else:
                # DETENER GRABACIÓN
                recording = False
                if csv_file:
                    csv_file.close()
                print("\n🟢 GRABACIÓN DETENIDA.\n")
                print("Presiona ENTER para iniciar otra grabación...\n")

        # --------- RECIBIR DATOS DEL ESP32 ----------
        data = conn.recv(2048)
        if not data:
            continue

        buffer += data.decode("utf-8")

        while "\n" in buffer:
            line, buffer = buffer.split("\n", 1)
            line = line.strip()

            if line == "":
                continue

            print("Recibido:", line)

            # Intentar decodificar JSON
            try:
                obj = json.loads(line)

                timestamp = obj.get("timestamp_us", 0)
                ecg_values = obj.get("emg", [])  
                # Convertir número → lista
                if isinstance(ecg_values, int):
                    ecg_values = [ecg_values]

                # Rellenar
                if len(ecg_values) < 600:
                    ecg_values += [0] * (600 - len(ecg_values))

                # Cortar si excede
                ecg_values = ecg_values[:600]

                # Guardar solo si está grabando
                if recording:
                    row = [timestamp] + ecg_values
                    writer.writerow(row)

                # Enviar a MQTT
                mqtt_client.publish(MQTT_TOPIC, line)

            except json.JSONDecodeError:
                print("⚠ JSON inválido (ignorando):", line)

except KeyboardInterrupt:
    print("\nCerrando programa...")

finally:
    if csv_file:
        csv_file.close()
    conn.close()
    server.close()
