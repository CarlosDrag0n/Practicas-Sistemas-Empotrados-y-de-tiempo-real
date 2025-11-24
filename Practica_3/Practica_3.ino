#include <LiquidCrystal.h>
#include "DHT.h"

// --- 1. Pantalla LCD ---
const int PIN_LCD_RS = 12;
const int PIN_LCD_E  = 10;
const int PIN_LCD_D4 = 9;
const int PIN_LCD_D5 = 6;
const int PIN_LCD_D6 = 5;
const int PIN_LCD_D7 = 3;

// --- 2. LEDs ---
const int PIN_LED_1 = 4;
const int PIN_LED_2 = 11; // PWM

// --- 3. Botón (Principal - Solo Admin/Reset) ---
const int PIN_BOTON = 2;

// --- 4. Sensor Temperatura ---
const int PIN_DHT = A5;
#define DHTTYPE DHT11

// --- 5. Joystick ---
const int PIN_JOY_X = A3;
const int PIN_JOY_Y = A4;
const int PIN_JOY_SW = A2; // Botón Joystick (Seleccionar Café)
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

// --- 8. Variables de Control de Tiempo (NO BLOQUEANTE) ---
unsigned long tInicio = 0;      // Reloj principal para estados
int brilloLed = 0;              // Variable para efecto LED
unsigned long intervaloLed = 0; // Velocidad efecto LED

// --- Variables Anti-Flickering ---
float tempPrev = -999; 
float humPrev = -999;

// --- Variable Cronómetro Botón ---
unsigned long tBotonPress = 0; 

LiquidCrystal lcd(PIN_LCD_RS, PIN_LCD_E, PIN_LCD_D4, PIN_LCD_D5, PIN_LCD_D6, PIN_LCD_D7);
DHT dht(PIN_DHT, DHTTYPE);

// --- MÁQUINA DE ESTADOS PRINCIPAL ---
enum Estado {
  INICIO,           
  ESPERANDO,
  ENCONTRADO_MSG,   
  ENCONTRADO_DATOS, 
  MENU,
  PREPARANDO,
  RETIRAR,          
  REINICIANDO,      
  ADMIN             
};
Estado estadoActual = INICIO; 

// --- MÁQUINA DE ESTADOS ADMIN ---
enum AdminEstado {
  ADMIN_INICIO,     
  ADMIN_MAIN,
  ADMIN_TEMP,
  ADMIN_DIST,
  ADMIN_COUNT,
  ADMIN_PRICE_LIST,
  ADMIN_PRICE_EDIT,
  ADMIN_SALIDA      
};
AdminEstado adminEstadoActual = ADMIN_MAIN;
int adminCursor = 0; 
int precioCursor = 0; 


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

  // --- SECUENCIA DE TEST HARDWARE (LEDs) ---
  lcd.setCursor(0, 0);
  lcd.print("CARGANDO...");
  
  for (int i = 0; i < 3; i++) {
    digitalWrite(PIN_LED_1, HIGH); 
    delay(500);                    
    digitalWrite(PIN_LED_1, LOW);  
    delay(500);                    
  }
  
  // Preparamos el primer estado (INICIO)
  lcd.clear();
  tInicio = millis(); 
  estadoActual = INICIO; 
}


void loop() {
  if (estadoActual == ADMIN) {
    handleAdminMode();
  } else {
    handleServicioMode();
  }
}

void handleServicioMode() {

  // -----------------------------------------------------------
  // LÓGICA DE BOTÓN PRINCIPAL (GLOBAL / NO BLOQUEANTE)
  // -----------------------------------------------------------
  
  if (digitalRead(PIN_BOTON) == LOW) {
    // Inicio de pulsación
    if (tBotonPress == 0) {
      tBotonPress = millis();
    }
    
    // Si llevamos > 5 seg pulsando -> ADMIN
    if (millis() - tBotonPress >= 5000) {
      entrarModoAdmin();
      tBotonPress = 0; 
      return; 
    }
    
  } else {
    // Botón soltado (o no pulsado)
    
    if (tBotonPress > 0) {
      unsigned long duracionPulsacion = millis() - tBotonPress;
      
      // Lógica REINICIO (2 seg - 5 seg)
      // CAMBIO: Funciona en cualquier estado MENOS en ESPERANDO
      if (estadoActual != ESPERANDO && duracionPulsacion >= 2000 && duracionPulsacion < 5000) {
         estadoActual = REINICIANDO;
         lcd.clear();
         tInicio = millis();
      }
      
      tBotonPress = 0;
    }
  }
  // -----------------------------------------------------------


  // --- 0. ESTADO INICIO ---
  if (estadoActual == INICIO) {
    lcd.setCursor(4, 0);
    lcd.print("Servicio");
    
    if (millis() - tInicio > 1000) {
      reiniciarServicio(); 
    }

  // --- 1. ESTADO ESPERANDO ---
  } else if (estadoActual == ESPERANDO) {
    
    if (millis() - tInicio > 200) {
      medirDistancia();
      Serial.print("Distancia: ");
      Serial.println(distancia);
      
      lcd.setCursor(0, 0);
      lcd.print("ESPERANDO");
      lcd.setCursor(0, 1);
      lcd.print("CLIENTE...");
      
      tInicio = millis(); 
      
      if (distancia < 100) {
        estadoActual = ENCONTRADO_MSG; 
        lcd.clear();
        tInicio = millis(); 
      }
    }
    
  // --- 2. ESTADO ENCONTRADO (Texto) ---
  } else if (estadoActual == ENCONTRADO_MSG) {
    
    lcd.setCursor(4, 0);
    lcd.print("CLIENTE");
    lcd.setCursor(3, 1);
    lcd.print("ENCONTRADO");
    
    if (millis() - tInicio > 2000) {
      estadoActual = ENCONTRADO_DATOS;
      lcd.clear();
      
      tempPrev = -999;
      humPrev = -999;
      
      tInicio = millis(); 
    }

  // --- 3. ESTADO ENCONTRADO (Datos) ---
  } else if (estadoActual == ENCONTRADO_DATOS) {
    
    float h = dht.readHumidity();
    float t = dht.readTemperature();
    
    if (t != tempPrev || h != humPrev) {
        mostrarDatosDHT(t, h);
        tempPrev = t;
        humPrev = h;
    }

    if (millis() - tInicio > 5000) {
      distancia = 1000;
      estadoActual = MENU;
      lcd.clear();
      tInicio = millis();
    }

  // --- 4. ESTADO MENU ---
  } else if (estadoActual == MENU) {
    
    int joyY = analogRead(PIN_JOY_Y);
    
    if(joyY < 450 && count < 4 && !joystickMovido){
      count++; 
      joystickMovido = true;
    } else if(joyY > 650 && count > 0 && !joystickMovido){
      count--; 
      joystickMovido = true;
    } else if (joyY >= 450 && joyY <= 650) {
      joystickMovido = false;
    }

    lcd.setCursor(0, 0);
    lcd.print(nombres[count]);
    lcd.setCursor(0, 1);
    lcd.print(precios[count], 2);
    lcd.print(" $");
    
    // Selección (Joystick)
    if (digitalRead(PIN_JOY_SW) == LOW) {
      delay(50); 
      while(digitalRead(PIN_JOY_SW) == LOW); 
      
      estadoActual = PREPARANDO;
      lcd.clear();
      brilloLed = 0; 
      long tiempoTotal = random(4000, 8001); 
      intervaloLed = tiempoTotal / 255;      
      tInicio = millis(); 
    }
    
  // --- 5. ESTADO PREPARANDO ---
  } else if (estadoActual == PREPARANDO) {
    
    lcd.setCursor(0, 0);
    lcd.print("Preparando Cafe...");
    
    if (millis() - tInicio >= intervaloLed) {
      if (brilloLed <= 255) {
        analogWrite(PIN_LED_2, brilloLed);
        brilloLed++;
        tInicio = millis(); 
      } else {
        analogWrite(PIN_LED_2, 0);
        estadoActual = RETIRAR;
        lcd.clear();
        tInicio = millis(); 
      }
    }
    
  // --- 6. ESTADO RETIRAR ---
  } else if (estadoActual == RETIRAR) {
    
    lcd.setCursor(0, 0);
    lcd.print("RETIRE BEBIDA");
    
    if (millis() - tInicio > 3000) {
      reiniciarServicio();
    }

  // --- 7. ESTADO REINICIANDO ---
  } else if (estadoActual == REINICIANDO) {
    
    lcd.setCursor(0, 0);
    lcd.print("REINICIANDO...");
    
    if (millis() - tInicio > 1500) {
      reiniciarServicio();
    }
  }
}

// --- MODO ADMIN ---
void handleAdminMode() {
  
  if (adminEstadoActual == ADMIN_INICIO) {
    lcd.setCursor(0,0);
    lcd.print("MODO ADMIN");
    
    if (millis() - tInicio > 1000) {
      adminEstadoActual = ADMIN_MAIN;
      lcd.clear();
    }
    return; 
  }

  if (adminEstadoActual == ADMIN_SALIDA) {
    lcd.setCursor(0,0);
    lcd.print("SALIENDO...");
    digitalWrite(PIN_LED_1, LOW);
    analogWrite(PIN_LED_2, 0);
    
    if (millis() - tInicio > 1000) {
      reiniciarServicio(); 
    }
    return; 
  }

  // Admin Lógica (Salida Rápida)
  if (digitalRead(PIN_BOTON) == LOW) {
    long t = 0;
    while(digitalRead(PIN_BOTON) == LOW) {
      delay(10);
      t += 10;
    }
    if (t >= 5000) {
      salirModoAdmin();
      return; 
    }
  }
  
  int joyY = analogRead(PIN_JOY_Y);
  bool joySelect = (digitalRead(PIN_JOY_SW) == LOW);
  bool joyBack = (analogRead(PIN_JOY_X) < 450);

  if (adminEstadoActual == ADMIN_MAIN) {
    String menu[4] = {"Ver temp", "Ver distancia", "Ver contador", "Modificar $"};
    
    if (joyY < 450 && adminCursor < 3 && !joystickMovido) {
      adminCursor++;
      joystickMovido = true;
    } else if (joyY > 650 && adminCursor > 0 && !joystickMovido) {
      adminCursor--;
      joystickMovido = true;
    } else if (joyY >= 450 && joyY <= 650) {
      joystickMovido = false;
    }
    
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("> ");
    lcd.print(menu[adminCursor]);
    if (adminCursor < 3) {
      lcd.setCursor(1, 1);
      lcd.print(menu[adminCursor + 1]);
    }
    
    if (joySelect) {
      delay(200); 
      if (adminCursor == 0) adminEstadoActual = ADMIN_TEMP;
      if (adminCursor == 1) adminEstadoActual = ADMIN_DIST;
      if (adminCursor == 2) adminEstadoActual = ADMIN_COUNT;
      if (adminCursor == 3) {
        adminEstadoActual = ADMIN_PRICE_LIST;
        precioCursor = 0; 
      }
    }
    
  } else if (adminEstadoActual == ADMIN_TEMP) {
    float h = dht.readHumidity();
    float t = dht.readTemperature();
    mostrarDatosDHT(t, h);
    
    if (joyBack) adminEstadoActual = ADMIN_MAIN; 
    
  } else if (adminEstadoActual == ADMIN_DIST) {
    medirDistancia();
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Distancia:");
    lcd.setCursor(0,1);
    lcd.print(distancia);
    lcd.print(" cm");
    
    if (joyBack) adminEstadoActual = ADMIN_MAIN; 
    
  } else if (adminEstadoActual == ADMIN_COUNT) {
    lcd.clear();
    lcd.print("Tiempo ON:");
    lcd.setCursor(0,1);
    lcd.print(millis()/1000);
    lcd.print(" seg");
    
    if (joyBack) adminEstadoActual = ADMIN_MAIN; 
    
  } else if (adminEstadoActual == ADMIN_PRICE_LIST) {
    if (joyY < 450 && precioCursor < 4 && !joystickMovido) {
      precioCursor++;
      joystickMovido = true;
    } else if (joyY > 650 && precioCursor > 0 && !joystickMovido) {
      precioCursor--;
      joystickMovido = true;
    } else if (joyY >= 450 && joyY <= 650) {
      joystickMovido = false;
    }
    
    lcd.clear();
    lcd.print(">");
    lcd.print(nombres[precioCursor]);
    lcd.setCursor(0, 1);
    lcd.print(precios[precioCursor], 2);
    lcd.print(" $");
    
    if (joySelect) {
      delay(200);
      adminEstadoActual = ADMIN_PRICE_EDIT;
    }
    if (joyBack) {
      delay(200);
      adminEstadoActual = ADMIN_MAIN;
      adminCursor = 3; 
    }
    
  } else if (adminEstadoActual == ADMIN_PRICE_EDIT) {
    if (joyY < 450 && !joystickMovido) {
      precios[precioCursor] -= 0.05;
      joystickMovido = true;
    } else if (joyY > 650 && !joystickMovido) {
      precios[precioCursor] += 0.05;
      joystickMovido = true;
    } else if (joyY >= 450 && joyY <= 650) {
      joystickMovido = false;
    }
    
    lcd.clear();
    lcd.print(nombres[precioCursor]);
    lcd.setCursor(0, 1);
    lcd.print(precios[precioCursor], 2);
    lcd.print(" $ (Edit)");
    
    if (joySelect) {
      delay(200);
      adminEstadoActual = ADMIN_PRICE_LIST;
    }
    if (joyBack) {
      delay(200);
      adminEstadoActual = ADMIN_PRICE_LIST;
    }
  }
  delay(50); 
}

// --- Funciones Auxiliares ---

void entrarModoAdmin() {
  estadoActual = ADMIN;
  adminEstadoActual = ADMIN_INICIO; 
  adminCursor = 0;
  
  digitalWrite(PIN_LED_1, HIGH);
  analogWrite(PIN_LED_2, 255); 
  
  lcd.clear();
  tInicio = millis(); 
}

void salirModoAdmin() {
  adminEstadoActual = ADMIN_SALIDA;
  lcd.clear();
  tInicio = millis(); 
}

void reiniciarServicio() {
  distancia = 1000;
  count = 0;
  estadoActual = ESPERANDO;
  lcd.clear();
  tInicio = millis(); 
}

void medirDistancia() {
  digitalWrite(PIN_ULTRA_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_ULTRA_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_ULTRA_TRIG, LOW);
  duracion = pulseIn(PIN_ULTRA_ECHO, HIGH);
  distancia = duracion / 58;
}

void mostrarDatosDHT(float t, float h) {
  lcd.clear();
  if (isnan(h) || isnan(t)) {
    lcd.print("Error Sensor");
  } else {
    lcd.setCursor(0, 0);
    lcd.print("Temp:");
    lcd.print(t);
    lcd.print("C");
    lcd.setCursor(0, 1);
    lcd.print("Hum:");
    lcd.print(h);
    lcd.print("%");
  }
}