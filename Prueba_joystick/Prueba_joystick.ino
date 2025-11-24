// Define el pin analógico para el Eje Y del joystick
#define JOYSTICK_Y_PIN A4
#define JOYSTICK_X_PIN A3

void setup() {
  Serial.begin(9600);
}

void loop() {
  // Lee el valor del eje Y (un número de 0 a 1023)
  int valorY = analogRead(JOYSTICK_Y_PIN);
  int valorX = analogRead(JOYSTICK_X_PIN);

  // Imprime el valor en el Monitor Serie
  Serial.print("Valor Eje Y (A4): ");
  Serial.println(valorY);
  Serial.print("Valor Eje X (A3): ");
  Serial.println(valorX);
  delay(500);
}
