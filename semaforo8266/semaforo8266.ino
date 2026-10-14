// Semáforo ESP8266 — secuencia + reporte por Serial (JSONL) para server.py
// Pines (Opción B, sin strapping): Rojo=GPIO14/D5 (20s), Amarillo=GPIO4/D2 (5s), Verde=GPIO5/D1 (15s)
// Mapeo NodeMCU: GPIO14=D5, GPIO4=D2, GPIO5=D1
// Cableado: cada LED entre su GPIO -> resistencia 330 Ohm -> GND (el 8266 da ~12mA por GPIO).
// Se evita GPIO2 porque en ESP8266 es pin de arranque (debe estar HIGH en boot)
// y lleva el LED onboard activo en LOW.
// Secuencia: VERDE 15s -> AMARILLA 5s -> ROJA 20s -> loop
// Cada 200 ms envía por Serial una línea JSON:
//   {"luz":"verde","pin":5,"quedan":12,"duracion":15}
// El servidor Python (server.py) lo lee SIN cambios y lo expone en /api/estado.

const int PIN_ROJO = 14;  // D5 en NodeMCU
const int PIN_AMARILLO = 4;  // D2 en NodeMCU
const int PIN_VERDE = 5;  // D1 en NodeMCU

const unsigned long DUR_VERDE = 15000;
const unsigned long DUR_AMARILLA = 5000;
const unsigned long DUR_ROJA = 20000;

enum Luz { VERDE, AMARILLA, ROJA };
Luz luzActual = VERDE;
unsigned long inicioEstado = 0;
unsigned long ultimoEnvio = 0;
const unsigned long INTERVALO_ENVIO = 200;  // ms

void aplicarLuz(Luz l) {
  digitalWrite(PIN_ROJO, LOW);
  digitalWrite(PIN_AMARILLO, LOW);
  digitalWrite(PIN_VERDE, LOW);
  if (l == ROJA) digitalWrite(PIN_ROJO, HIGH);
  if (l == AMARILLA) digitalWrite(PIN_AMARILLO, HIGH);
  if (l == VERDE) digitalWrite(PIN_VERDE, HIGH);
}

unsigned long duracionDe(Luz l) {
  if (l == VERDE) return DUR_VERDE;
  if (l == AMARILLA) return DUR_AMARILLA;
  return DUR_ROJA;
}

const char* nombreDe(Luz l) {
  if (l == VERDE) return "verde";
  if (l == AMARILLA) return "amarilla";
  return "roja";
}

int pinDe(Luz l) {
  if (l == VERDE) return PIN_VERDE;
  if (l == AMARILLA) return PIN_AMARILLO;
  return PIN_ROJO;
}

void enviarEstado(unsigned long ahora) {
  unsigned long dur = duracionDe(luzActual);
  unsigned long transcurrido = ahora - inicioEstado;
  long quedan = (long)((dur - transcurrido + 999) / 1000);  // ceil a segundos
  if (quedan < 0) quedan = 0;
  Serial.print("{\"luz\":\"");
  Serial.print(nombreDe(luzActual));
  Serial.print("\",\"pin\":");
  Serial.print(pinDe(luzActual));
  Serial.print(",\"quedan\":");
  Serial.print(quedan);
  Serial.print(",\"duracion\":");
  Serial.print(dur / 1000);
  Serial.println("}");
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_ROJO, OUTPUT);
  pinMode(PIN_AMARILLO, OUTPUT);
  pinMode(PIN_VERDE, OUTPUT);
  luzActual = VERDE;
  aplicarLuz(luzActual);
  inicioEstado = millis();
  delay(500);  // dar tiempo al Monitor Serial
  enviarEstado(inicioEstado);
}

void loop() {
  unsigned long ahora = millis();
  unsigned long dur = duracionDe(luzActual);
  if (ahora - inicioEstado >= dur) {
    // Avanzar secuencia: VERDE -> AMARILLA -> ROJA -> VERDE
    if (luzActual == VERDE) luzActual = AMARILLA;
    else if (luzActual == AMARILLA) luzActual = ROJA;
    else luzActual = VERDE;
    aplicarLuz(luzActual);
    inicioEstado = ahora;
    enviarEstado(ahora);
    ultimoEnvio = ahora;
  } else if (ahora - ultimoEnvio >= INTERVALO_ENVIO) {
    enviarEstado(ahora);
    ultimoEnvio = ahora;
  }
}
