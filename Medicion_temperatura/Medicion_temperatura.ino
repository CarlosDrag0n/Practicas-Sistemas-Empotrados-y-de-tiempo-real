#include "DHT.h" // Carga la biblioteca que acabamos de instalar

// Define el pin donde conectaste el sensor
#define DHTPIN A5

// Define el tipo de sensor que estás usando (DHT11)
#define DHTTYPE DHT11

// Inicializa el sensor
DHT dht(DHTPIN, DHTTYPE);

void setup() {
  // Inicia la comunicación serie (para ver los datos en el PC)
  Serial.begin(9600);
  Serial.println("Iniciando prueba del sensor DHT11");

  // Inicia el sensor
  dht.begin();
}

void loop() {
  // Espera 2 segundos entre lecturas (el sensor no puede leerse más rápido)
  delay(2000);

  // Leer la humedad
  float h = dht.readHumidity();
  // Leer la temperatura en Celsius
  float t = dht.readTemperature();
  // Leer la temperatura en Fahrenheit (opcional)
  float f = dht.readTemperature(true);

  // Comprueba si la lectura falló (devuelve "NaN" - Not a Number)
  if (isnan(h) || isnan(t)) {
    Serial.println("Error al leer el sensor DHT11");
    return; // Sale del loop y vuelve a intentarlo
  }

  // --- ¡Aquí es donde ves los datos! ---
  Serial.print("Humedad: ");
  Serial.print(h);
  Serial.print("%  ");
  Serial.print("Temperatura: ");
  Serial.print(t);
  Serial.print("°C  ");
  Serial.print(f);
  Serial.println("°F");
}