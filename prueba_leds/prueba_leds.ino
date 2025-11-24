// Define los pines que vamos a usar
const int ledPin1 = 4;
const int ledPin2 = 11;

void setup() {
  // Configura ambos pines como SALIDA (OUTPUT)
  pinMode(ledPin1, OUTPUT);
  pinMode(ledPin2, OUTPUT);
}

void loop() {
  // --- Estado 1 ---
  digitalWrite(ledPin1, HIGH);  // Enciende el LED 1
  digitalWrite(ledPin2, LOW);   // Apaga el LED 2
  
  delay(1000); // Espera 1 segundo (1000 milisegundos)

  // --- Estado 2 ---
  digitalWrite(ledPin1, LOW);   // Apaga el LED 1
  digitalWrite(ledPin2, HIGH);  // Enciende el LED 2
  
  delay(1000); // Espera 1 segundo
  
  // El loop vuelve a empezar
}
