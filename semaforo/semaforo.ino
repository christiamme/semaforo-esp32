// Semáforo ESP32 — secuencia + reporte por Serial (JSONL) para server.py
// Pines: Rojo=GPIO2 (20s), Amarillo=GPIO4 (5s), Verde=GPIO5 (15s)
// Secuencia: VERDE 15s -> AMARILLA 5s -> ROJA 20s -> loop
// Cada 200 ms envía por Serial una línea JSON:
//   {"luz":"verde","pin":5,"quedan":12,"duracion":15}
// El servidor Python lee esto y lo expone en /api/estado.

const int PIN_ROJO = 2;
const int PIN_AMARILLO = 4;
const int PIN_VERDE = 5;

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
