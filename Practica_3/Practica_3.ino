#include <LiquidCrystal.h>
#include "DHT.h"

// --- 1. Pantalla LCD ---
const int PIN_LCD_RS = 12;
const int PIN_LCD_E  = 10;
const int PIN_LCD_D4 = 9;
const int PIN_LCD_D5 = 6;
const int PIN_LCD_D6 = 5;
const int PIN_LCD_D7 = 3;

// --- 2. LEDs (PINES CAMBIADOS) ---
const int PIN_LED_1 = 4;
const int PIN_LED_2 = 11; // ¡CAMBIADO! (PWM)

// --- 3. Botón (Principal) ---
const int PIN_BOTON = 2;

// --- 4. Sensor Temperatura ---
const int PIN_DHT = A5;
#define DHTTYPE DHT11

// --- 5. Joystick ---
const int PIN_JOY_X = A3;
const int PIN_JOY_Y = A4;
const int PIN_JOY_SW = A2; // ¡NUEVO! Botón del Joystick (Seleccionar en Admin)
int count = 0;
bool joystickMovido = false; 

// --- 6. Sensor Ultrasonido ---
const int PIN_ULTRA_TRIG = 8;
const int PIN_ULTRA_ECHO = 7;
long duracion;
int distancia;

// --- 7. Variables Globales del Negocio ---
String nombres[5] = {"Solo       ", "Cortado    ", "Doble      ", "Premium    ", "Chocolate  "};
float precios[5] = {1.00, 1.10, 1.25, 1.50, 2.00};

LiquidCrystal lcd(PIN_LCD_RS, PIN_LCD_E, PIN_LCD_D4, PIN_LCD_D5, PIN_LCD_D6, PIN_LCD_D7);
DHT dht(PIN_DHT, DHTTYPE);

enum Estado {
  ESPERANDO,
  ENCONTRADO,
  MENU,
  PREPARANDO,
  ADMIN // ¡NUEVO ESTADO!
};
Estado estadoActual = ESPERANDO;

enum AdminEstado {
  ADMIN_MAIN,
  ADMIN_TEMP,
  ADMIN_DIST,
  ADMIN_COUNT,
  ADMIN_PRICE_LIST,
  ADMIN_PRICE_EDIT
};
AdminEstado adminEstadoActual = ADMIN_MAIN;
int adminCursor = 0; // Cursor para el menú Admin
int precioCursor = 0; // Cursor para la lista de precios


void setup() {
  Serial.begin(9600);
  
  // Configuración de pines
  pinMode(PIN_LED_1, OUTPUT);
  pinMode(PIN_LED_2, OUTPUT);
  pinMode(PIN_BOTON, INPUT_PULLUP);
  pinMode(PIN_JOY_SW, INPUT_PULLUP);
  pinMode(PIN_ULTRA_TRIG, OUTPUT);
  pinMode(PIN_ULTRA_ECHO, INPUT);
  
  // Iniciar Pantalla y Sensor
  lcd.begin(16, 2);
  dht.begin();
  
  randomSeed(analogRead(A0)); 
  Serial.println("Setup Completo.");

  // --- Secuencia de Arranque ---
  lcd.setCursor(0, 0);
  lcd.print("CARGANDO...");
  for (int i = 0; i < 3; i++) {
    digitalWrite(PIN_LED_1, HIGH);
    delay(1000);
    digitalWrite(PIN_LED_1, LOW);
    delay(1000);
  }

  lcd.clear();
  lcd.setCursor(4, 0);
  lcd.print("Servicio");
  
  distancia = 1000; 
}


void loop() {
  
  // La máquina de estados principal
  // El modo ADMIN tiene prioridad y se maneja por separado
  if (estadoActual == ADMIN) {
    handleAdminMode();
  } else {
    handleServicioMode();
  }
}

void handleServicioMode() {
  
  if (estadoActual == ESPERANDO) {
    
    lcd.clear();
    medirDistancia(); // Llama a la función de medir
    Serial.print("Distancia: ");
    Serial.print(distancia);
    Serial.println(" cm");
    lcd.setCursor(0, 0);
    lcd.print("ESPERANDO");
    lcd.setCursor(0, 1);
    lcd.print("CLIENTE...");
    delay(500);

    // Comprobación de cambio a ENCONTRADO
    if (distancia < 100) {
      estadoActual = ENCONTRADO;
    }
    
    // COMPROBACIÓN ADMIN (pulsación > 5s)
    checkAdminEntry(); 
    
  } else if (estadoActual == ENCONTRADO) {
    
    // (En este estado no se puede entrar a ADMIN
    //  porque está bloqueado por delay())
    lcd.clear();
    lcd.setCursor(4, 0);
    lcd.print("CLIENTE");
    lcd.setCursor(3, 1);
    lcd.print("ENCONTRADO");
    delay(2000); // <-- BLOQUEADO

    float h = dht.readHumidity();
    float t = dht.readTemperature();

    if (isnan(h) || isnan(t)) {
      lcd.clear();
      lcd.print("Error Sensor DHT");
      delay(5000); // <-- BLOQUEADO
    } else {
      lcd.clear();
      mostrarDatosDHT(t, h); // Llama a la función de mostrar
      delay(5000); // <-- BLOQUEADO
    }

    distancia = 1000;
    estadoActual = MENU;

  } else if (estadoActual == MENU) {
    
    // Navegación del Menú (Cliente)
    int joyY = analogRead(PIN_JOY_Y);
    if(joyY < 450 && count > 0 && !joystickMovido){
      count -= 1; 
      joystickMovido = true;
    } else if(joyY > 650 && count < 4 && !joystickMovido){
      count += 1;
      joystickMovido = true;
    } else if (joyY >= 450 && joyY <= 650) {
      joystickMovido = false;
    }

    // Dibujar el menú (Cliente)
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(nombres[count]); // Lee del array global
    lcd.setCursor(0, 1);
    lcd.print(precios[count], 2); // Lee del array global
    lcd.print(" $");
    
    // Lógica de Botón (Cliente y Admin)
    if (digitalRead(PIN_BOTON) == LOW) {
      long tiempoPresionado = 0;
      while(digitalRead(PIN_BOTON) == LOW) {
        delay(10);
        tiempoPresionado += 10;
      }
      
      // Pulsación CORTA (Seleccionar)
      if (tiempoPresionado > 50 && tiempoPresionado < 1000) {
        estadoActual = PREPARANDO;
      
      // Pulsación LARGA (Reset 2-3 seg)
      } else if (tiempoPresionado >= 2000 && tiempoPresionado <= 3000) {
        lcd.clear();
        lcd.print("REINICIANDO...");
        delay(1500);
        reiniciarServicio();
      
      // Pulsación MUY LARGA (Entrar a ADMIN)
      } else if (tiempoPresionado >= 5000) {
        entrarModoAdmin();
      }
    }
    
    delay(100); 
    
  } else if (estadoActual == PREPARANDO) {
    
    // (En este estado no se puede entrar a ADMIN)
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Preparando Cafe...");
    
    long tiempoEspera = random(4000, 8001);
    int delayPorPaso = tiempoEspera / 255;
    
    for (int i = 0; i <= 255; i++) {
      analogWrite(PIN_LED_2, i); // Brillo incremental
      delay(delayPorPaso); // <-- BLOQUEADO
    }
    
    analogWrite(PIN_LED_2, 0); // Apagar LED
    
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("RETIRE BEBIDA");
    delay(3000); // <-- BLOQUEADO
    
    reiniciarServicio(); // Volver al estado inicial
  }
}

void handleAdminMode() {
  
  // Comprobar si salimos del modo admin (5s con el BOTÓN PRINCIPAL)
  if (digitalRead(PIN_BOTON) == LOW) {
    long tiempoPresionado = 0;
    while(digitalRead(PIN_BOTON) == LOW) {
      delay(10);
      tiempoPresionado += 10;
    }
    if (tiempoPresionado >= 5000) {
      salirModoAdmin();
      return; // Salir de la función
    }
  }
  
  // Navegación (Joystick Y)
  int joyY = analogRead(PIN_JOY_Y);
  // Inputs (Botón Joystick y Eje X)
  bool joySelect = (digitalRead(PIN_JOY_SW) == LOW);
  bool joyBack = (analogRead(PIN_JOY_X) < 450);

  // --- Máquina de Estados Interna de ADMIN ---
  
  if (adminEstadoActual == ADMIN_MAIN) {
    String menu[4] = {"Ver temp", "Ver distancia", "Ver contador", "Modificar $"};
    
    // Navegación Menú Principal Admin
    if (joyY < 450 && adminCursor < 3 && !joystickMovido) {
      adminCursor++; joystickMovido = true;
    } else if (joyY > 650 && adminCursor > 0 && !joystickMovido) {
      adminCursor--; joystickMovido = true;
    } else if (joyY >= 450 && joyY <= 650) {
      joystickMovido = false;
    }
    
    // Dibujar Menú Principal Admin
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("> ");
    lcd.print(menu[adminCursor]);
    if (adminCursor < 3) {
      lcd.setCursor(1, 1);
      lcd.print(menu[adminCursor + 1]);
    }
    
    // Seleccionar (Botón Joystick)
    if (joySelect) {
      delay(200); // Pequeño debounce para el botón
      if (adminCursor == 0) adminEstadoActual = ADMIN_TEMP;
      if (adminCursor == 1) adminEstadoActual = ADMIN_DIST;
      if (adminCursor == 2) adminEstadoActual = ADMIN_COUNT;
      if (adminCursor == 3) {
        adminEstadoActual = ADMIN_PRICE_LIST;
        precioCursor = 0; // Reiniciar cursor de precios
      }
    }
    
  } else if (adminEstadoActual == ADMIN_TEMP) {
    // Submenú i) Ver Temperatura
    float h = dht.readHumidity();
    float t = dht.readTemperature();
    lcd.clear();
    mostrarDatosDHT(t, h); // Reutiliza la función
    
    if (joyBack) adminEstadoActual = ADMIN_MAIN; // Volver
    
  } else if (adminEstadoActual == ADMIN_DIST) {
    // Submenú ii) Ver Distancia
    medirDistancia();
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Distancia:");
    lcd.setCursor(0, 1);
    lcd.print(distancia);
    lcd.print(" cm");
    
    if (joyBack) adminEstadoActual = ADMIN_MAIN; // Volver
    
  } else if (adminEstadoActual == ADMIN_COUNT) {
    // Submenú iii) Ver Contador de tiempo
    unsigned long segundos = millis() / 1000;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Tiempo ON:");
    lcd.setCursor(0, 1);
    lcd.print(segundos);
    lcd.print(" seg");
    
    if (joyBack) adminEstadoActual = ADMIN_MAIN; // Volver
    
  } else if (adminEstadoActual == ADMIN_PRICE_LIST) {
    // Submenú iv) Modificar Precios (Navegación)
    
    // Navegación
    if (joyY < 450 && precioCursor < 4 && !joystickMovido) {
      precioCursor++; joystickMovido = true;
    } else if (joyY > 650 && precioCursor > 0 && !joystickMovido) {
      precioCursor--; joystickMovido = true;
    } else if (joyY >= 450 && joyY <= 650) {
      joystickMovido = false;
    }
    
    // Dibujar
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(">");
    lcd.print(nombres[precioCursor]);
    lcd.setCursor(0, 1);
    lcd.print(precios[precioCursor], 2);
    lcd.print(" $");
    
    // Seleccionar (Botón Joystick)
    if (joySelect) {
      delay(200);
      adminEstadoActual = ADMIN_PRICE_EDIT; // Pasa a modo edición
    }
    // Volver (Joystick Izquierda)
    if (joyBack) {
      delay(200);
      adminEstadoActual = ADMIN_MAIN;
      adminCursor = 3; // Volver al menú principal en la pos 3
    }
    
  } else if (adminEstadoActual == ADMIN_PRICE_EDIT) {
    // Submenú iv) Modificar Precios (Edición)
    
    // Edición (Joystick Y, incrementos de 0.05)
    if (joyY < 450 && !joystickMovido) {
      precios[precioCursor] -= 0.05;
      joystickMovido = true;
    } else if (joyY > 650 && !joystickMovido) {
      precios[precioCursor] += 0.05;
      joystickMovido = true;
    } else if (joyY >= 450 && joyY <= 650) {
      joystickMovido = false;
    }
    
    // Dibujar
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(nombres[precioCursor]);
    lcd.setCursor(0, 1);
    lcd.print(precios[precioCursor], 2);
    lcd.print(" $ (Edit)");
    
    // Confirmar (Botón Joystick)
    if (joySelect) {
      delay(200);
      adminEstadoActual = ADMIN_PRICE_LIST; // Confirma y vuelve a la lista
    }
    // Cancelar (Joystick Izquierda)
    if (joyBack) {
      delay(200);
      // (Aquí deberíamos recargar el precio original, pero
      // para simplificar solo volvemos a la lista)
      adminEstadoActual = ADMIN_PRICE_LIST;
    }
  }

  delay(100); // Delay general del modo Admin
}

// Comprueba la entrada al modo Admin (solo funciona en ESPERANDO y MENU)
void checkAdminEntry() {
  if (digitalRead(PIN_BOTON) == LOW) {
    long t = 0;
    while(digitalRead(PIN_BOTON) == LOW) {
      delay(10); t += 10;
    }
    if (t >= 5000) {
      entrarModoAdmin();
    }
  }
}

// Acciones al ENTRAR a Admin
void entrarModoAdmin() {
  estadoActual = ADMIN;
  adminEstadoActual = ADMIN_MAIN; // Resetea el submenú
  adminCursor = 0;
  
  // Encender ambos LEDs (b)
  digitalWrite(PIN_LED_1, HIGH);
  analogWrite(PIN_LED_2, 255); // Brillo máximo
  
  lcd.clear();
  lcd.print("MODO ADMIN");
  delay(1000);
}

// Acciones al SALIR de Admin
void salirModoAdmin() {
  // Apagar ambos LEDs
  digitalWrite(PIN_LED_1, LOW);
  analogWrite(PIN_LED_2, 0);
  
  lcd.clear();
  lcd.print("SALIENDO...");
  delay(1000);
  
  reiniciarServicio(); // Vuelve al estado ESPERANDO
}

// Resetea el servicio al estado inicial
void reiniciarServicio() {
  distancia = 1000;
  count = 0;
  estadoActual = ESPERANDO;
}

// Medición de distancia
void medirDistancia() {
  digitalWrite(PIN_ULTRA_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_ULTRA_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_ULTRA_TRIG, LOW);
  duracion = pulseIn(PIN_ULTRA_ECHO, HIGH);
  distancia = duracion / 58;
}

// Muestra temperatura y humedad en el LCD
void mostrarDatosDHT(float t, float h) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Temp:");
  lcd.setCursor(6, 0);
  lcd.print(t);
  lcd.setCursor(12, 0); 
  lcd.print("C");
  lcd.setCursor(0, 1);
  lcd.print("Humedad:");
  lcd.setCursor(9, 1);
  lcd.print(h);
  lcd.setCursor(15, 1);
  lcd.print("%");
}
