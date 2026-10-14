"""Servidor sencillo: recibe Serial del ESP32 y sirve la página + API.

Uso:
    pip install -r requirements.txt
    python server.py
    -> abrir http://localhost:5000

Cómo funciona:
- Hilo lector lee líneas JSON del ESP32 por USB (115200):
    {"luz":"verde","pin":5,"quedan":12,"duracion":15}
- Guarda el último estado. GET /api/estado lo devuelve recalculando
  la cuenta regresiva con el tiempo transcurrido.
- Si no hay ESP32 conectado (o no hay pyserial), entra en MODO SIMULADO
  con la misma secuencia (verde 15s, amarilla 5s, roja 20s) para poder
  ver la página sin hardware.
- Si pasan >3 s sin datos del puerto real, responde conectado:false
  y la página lo muestra en gris.

Configuración por variables de entorno:
    SERIAL_PORT=COM3  (en Windows suele ser COM3/COM4/COM5)
    BAUD=115200
    PORT=5000         (puerto web)
"""

import json
import os
import threading
import time

from flask import Flask, jsonify, send_from_directory

BASE_DIR = os.path.dirname(os.path.abspath(__file__))
SERIAL_PORT = os.environ.get("SERIAL_PORT", "")
BAUD = int(os.environ.get("BAUD", "115200"))
WEB_PORT = int(os.environ.get("PORT", "5000"))
TIMEOUT_DESCONEXION = 3.0

# Secuencia canónica (debe coincidir con semaforo.ino)
SECUENCIA = [
    ("verde", 5, 15),
    ("amarilla", 4, 5),
    ("roja", 2, 20),
]

estado = {
    "luz": "verde",
    "pin": 5,
    "quedan": 15,
    "duracion": 15,
    "conectado": False,
    "modo": "simulado",
    "ultima_actualizacion": time.time(),
}
_lock = threading.Lock()


def _set_estado(luz, pin, quedan, duracion, modo):
    with _lock:
        estado.update(
            {
                "luz": luz,
                "pin": pin,
                "quedan": int(quedan),
                "duracion": int(duracion),
                "conectado": True,
                "modo": modo,
                "ultima_actualizacion": time.time(),
            }
        )


def lector_serial(puerto):
    """Lee JSONL del ESP32. Si falla, cae a simulado."""
    try:
        import serial  # type: ignore
    except ImportError:
        print("[server] pyserial no instalado -> modo simulado.")
        simulador()
        return
    try:
        ser = serial.Serial(puerto, BAUD, timeout=1)
        print(f"[server] Leyendo ESP32 en {puerto} @ {BAUD}")
    except Exception as e:
        print(f"[server] No se pudo abrir {puerto}: {e} -> modo simulado.")
        simulador()
        return
    while True:
        try:
            linea = ser.readline().decode("utf-8", errors="ignore").strip()
            if not linea:
                continue
            if not linea.startswith("{"):
                continue
            msg = json.loads(linea)
            _set_estado(
                msg.get("luz", "verde"),
                int(msg.get("pin", 5)),
                int(msg.get("quedan", 0)),
                int(msg.get("duracion", 15)),
                f"usb:{puerto}",
            )
        except Exception:
            continue


def detectar_puerto():
    """Busca el primer puerto serial disponible (para no adivinar COMx)."""
    try:
        from serial.tools import list_ports  # type: ignore

        puertos = [p.device for p in list_ports.comports()]
        return puertos[0] if puertos else ""
    except Exception:
        return ""


def simulador():
    """Genera la misma secuencia que el ESP32 cuando no hay hardware."""
    idx = 0
    while True:
        luz, pin, dur = SECUENCIA[idx]
        for restan in range(dur, -1, -1):
            _set_estado(luz, pin, restan, dur, "simulado")
            time.sleep(1)
        idx = (idx + 1) % len(SECUENCIA)


def estado_para_api():
    with _lock:
        copia = dict(estado)
    # Recalcular cuenta regresiva con tiempo real (evita que se congele)
    elapsed = time.time() - copia["ultima_actualizacion"]
    if copia["modo"].startswith("usb:"):
        copia["quedan"] = max(0, int(round(copia["quedan"] - elapsed)))
        copia["conectado"] = elapsed < TIMEOUT_DESCONEXION
    else:
        copia["conectado"] = True  # simulado siempre "conectado"
    return copia


app = Flask(__name__)


@app.route("/")
def index():
    return send_from_directory(BASE_DIR, "index.html")


@app.route("/api/estado")
def api_estado():
    return jsonify(estado_para_api())


@app.route("/api/health")
def api_health():
    d = estado_para_api()
    return jsonify({"ok": True, "modo": d["modo"], "conectado": d["conectado"]})


if __name__ == "__main__":
    puerto = SERIAL_PORT or detectar_puerto()
    if puerto:
        t = threading.Thread(target=lector_serial, args=(puerto,), daemon=True)
    else:
        print("[server] Sin puerto serial -> modo simulado.")
        t = threading.Thread(target=simulador, daemon=True)
    t.start()
    print(f"[server] Abre http://localhost:{WEB_PORT}")
    app.run(host="0.0.0.0", port=WEB_PORT, use_reloader=False)
