import socket
import json
import csv
import os
from datetime import datetime

# ============================
# CONFIGURACIÓN
# ============================
HOST = "0.0.0.0"  # Escucha en todas las interfaces
PORT = 5005       # Debe coincidir con el puerto del ESP32
SAVE_FOLDER = r"C:\Users\Ximena\OneDrive\Escritorio\Grabaciones LIA"
os.makedirs(SAVE_FOLDER, exist_ok=True)

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

try:
    while True:
        # --------- TECLA START/STOP -----------
        if os.name == 'nt':  # Windows
            import msvcrt
            if msvcrt.kbhit() and msvcrt.getch() == b'\r':  # ENTER
                if not recording:
                    recording = True
                    timestamp_str = datetime.now().strftime("%Y%m%d_%H%M%S")
                    filename = f"ECG_{timestamp_str}.csv"
                    filepath = os.path.join(SAVE_FOLDER, filename)
                    csv_file = open(filepath, "w", newline="")
                    writer = csv.writer(csv_file)
                    headers = ["timestamp_ms"] + [f"ecg_{i+1}" for i in range(300)] + ["spo2", "temp_c"]
                    writer.writerow(headers)
                    print(f"\n🔴 GRABACIÓN INICIADA: {filename}\n")
                else:
                    recording = False
                    if csv_file:
                        csv_file.close()
                    print("\n🟢 GRABACIÓN DETENIDA.\n")
                    print("Presiona ENTER para iniciar otra grabación...\n")

        # --------- RECIBIR DATOS DEL ESP32 ----------
        data = conn.recv(8192)  # buffer más grande por si envía muchas líneas
        if not data:
            continue

        buffer += data.decode("utf-8")

        while "\n" in buffer:
            line, buffer = buffer.split("\n", 1)
            line = line.strip()
            if line == "":
                continue

            # Imprimir para revisar
            print("Recibido:", line)

            try:
                obj = json.loads(line)
                timestamp = obj.get("timestamp_ms", 0)
                ecg_values = obj.get("ecg", [])
                spo2 = obj.get("spo2", 0)
                temp_c = obj.get("temp_c", -127)

                # Rellenar o cortar ECG a 300 muestras
                if len(ecg_values) < 300:
                    ecg_values += [0] * (300 - len(ecg_values))
                ecg_values = ecg_values[:300]

                # Guardar en CSV si grabando
                if recording:
                    row = [timestamp] + ecg_values + [spo2, temp_c]
                    writer.writerow(row)

            except json.JSONDecodeError:
                print("⚠ JSON inválido (ignorando):", line)

except KeyboardInterrupt:
    print("\nCerrando programa...")

finally:
    if csv_file:
        csv_file.close()
    conn.close()
    server.close()
