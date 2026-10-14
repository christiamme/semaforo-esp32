# Semáforo ESP32/ESP8266 + Página web

Réplica simple del diseño de Figma `Semáforo` (header gris STATUS, cuerpo crema, 3 luces con borde negro).

## Archivos

- `semaforo/semaforo.ino` — código Arduino para ESP32. Rojo=GPIO2 (20 s), Amarillo=GPIO4 (5 s), Verde=GPIO5 (15 s). Manda JSON por Serial cada 200 ms.
- `semaforo8266/semaforo8266.ino` — variante para ESP8266 (NodeMCU). Rojo=GPIO14/D5 (20 s), Amarillo=GPIO4/D2 (5 s), Verde=GPIO5/D1 (15 s). Se evita GPIO2 porque en ESP8266 es pin de arranque.
- `server.py` — servidor Flask. Lee el Serial de la placa y expone `GET /api/estado`. Sirve `index.html`.
- `index.html` — página. Muestra la luz activa, `GPIO` activo y cuenta regresiva del cambio.
- `requirements.txt` — dependencias Python.

## 1. Placa (Arduino IDE, GUI)

1. Instala Arduino IDE + driver USB-Serial de tu placa (CP210x o CH340).
2. ESP32: abre `semaforo/semaforo.ino`, placa `ESP32 Dev Module`. ESP8266: abre `semaforo8266/semaforo8266.ino`, placa `NodeMCU 1.0`.
3. Conecta cada LED entre su GPIO y GND con resistencia (220 Ω en ESP32, 330 Ω en ESP8266).
4. Sube el código. Abre Monitor Serial a 115200: debes ver líneas `{"luz":"verde",...}`.

## 2. Servidor + página (PC)

```powershell
pip install -r requirements.txt
$env:SERIAL_PORT = "COM5"
python server.py
```

Abre http://localhost:5000

- Con la placa por USB: la detecta solo. Si tu puerto es otro, ajusta `SERIAL_PORT` antes de correr (en PowerShell: `$env:SERIAL_PORT = "COM3"`).
- Sin placa: entra en **modo simulado** con los mismos tiempos para poder exponer en clase.

## API

`GET /api/estado` → `{"luz":"roja","pin":2,"quedan":18,"duracion":20,"conectado":true,"modo":"usb:COM5"}`
`GET /api/health` → `{"ok":true,...}`
