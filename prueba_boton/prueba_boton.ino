// Define el pin donde conectaste el botón
const int pinBoton = 2;

void setup() {
  // Inicia el Monitor Serie para ver los mensajes
  Serial.begin(9600);
  
  // Configura el pin del botón como entrada
  // INPUT_PULLUP activa la resistencia interna.
  pinMode(pinBoton, INPUT_PULLUP);
}

void loop() {
  // Lee el estado del botón
  int estadoBoton = digitalRead(pinBoton);

  // Comprueba el estado e imprime
  // Con INPUT_PULLUP, el botón presionado da LOW (bajo)
  if (estadoBoton == LOW) {
    Serial.println("Botón PRESIONADO");
  } else {
    Serial.println("Botón suelto");
  }

  // Una pequeña pausa para evitar lecturas "rebotadas"
  delay(100); 
}