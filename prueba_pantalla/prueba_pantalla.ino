// Incluimos la biblioteca oficial para pantallas LCD
#include <LiquidCrystal.h>

// --- CONFIGURACIÓN DE PINES ---
// Aquí le decimos a la biblioteca qué pines del Arduino
// están conectados a qué pines de la pantalla LCD.
// La sintaxis es: LiquidCrystal(RS, E, D4, D5, D6, D7)
//
// Según tu descripción y la imagen:
// RS (Pin 4 del LCD) -> Pin 11 de Arduino
// E  (Pin 6 del LCD) -> Pin 10 de Arduino
// D4 (Pin 11 del LCD) -> Pin 9 de Arduino
// D5 (Pin 12 del LCD) -> Pin 6 de Arduino
// D6 (Pin 13 del LCD) -> Pin 5 de Arduino
// D7 (Pin 14 del LCD) -> Pin 3 de Arduino

LiquidCrystal lcd(12, 10, 9, 6, 5, 3);

void setup() {
  // Inicializamos la pantalla, especificando que es de 16 columnas y 2 filas
  lcd.begin(16, 2);
  
  // Movemos el cursor a la primera posición (columna 0, fila 0)
  lcd.setCursor(6, 0);
  
  // Imprimimos el mensaje en la pantalla
  lcd.print("Hola");
}

void loop() {
  // No necesitamos hacer nada más en el loop,
  // el mensaje se quedará fijo en la pantalla.
}
