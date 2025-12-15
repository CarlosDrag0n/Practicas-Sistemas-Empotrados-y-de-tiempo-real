#include <LiquidCrystal.h>
#include "DHT.h"

// --- CONFIGURACIÓN DE PINES ---
const int PIN_LCD_RS = 12;
const int PIN_LCD_E  = 10;
const int PIN_LCD_D4 = 9;
const int PIN_LCD_D5 = 6;
const int PIN_LCD_D6 = 5;
const int PIN_LCD_D7 = 3;

const int PIN_LED_1 = 4;
const int PIN_LED_2 = 11; // PWM

const int PIN_BOTON = 2;

const int PIN_DHT = A5;
#define DHTTYPE DHT11

const int PIN_JOY_X = A3;
const int PIN_JOY_Y = A4;
const int PIN_JOY_SW = A2;

const int PIN_ULTRA_TRIG = 8;
const int PIN_ULTRA_ECHO = 7;

// --- VARIABLES GLOBALES ---
int count = 0;
bool joystickMovido = false; 

long duracion;
int distancia;

String nombres[5] = {"Solo       ", "Cortado    ", "Doble      ", "Premium    ", "Chocolate  "};
float precios[5] = {1.00, 1.10, 1.25, 1.50, 2.00};

unsigned long tInicio = 0;      // Reloj principal para estados
int brilloLed = 0;              // Variable para efecto LED
unsigned long intervaloLed = 0; // Velocidad efecto LED

// Variables Anti-Flickering (Sensores)
float tempPrev = -999; 
float humPrev = -999;
unsigned long tRefrescoSensores = 0; // Para actualizar LCD en modo admin sin parpadeo

// Variables Botón Principal
unsigned long tBotonPress = 0; 

// Variables Control Joystick (Anti-Rebote sin Delay)
bool prevJoySelect = false; // Estado anterior del botón joy
bool prevJoyBack = false;   // Estado anterior del eje X joy

// Control de repintado LCD (Para evitar flickering)
bool adminRedraw = true; 

LiquidCrystal lcd(PIN_LCD_RS, PIN_LCD_E, PIN_LCD_D4, PIN_LCD_D5, PIN_LCD_D6, PIN_LCD_D7);
DHT dht(PIN_DHT, DHTTYPE);

// --- MÁQUINAS DE ESTADOS ---

// Cliente
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

// Admin
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

  lcd.setCursor(0, 0);
  lcd.print("CARGANDO...");
  
  // Pequeña secuencia de inicio
  for (int i = 0; i < 3; i++) {
    digitalWrite(PIN_LED_1, HIGH); 
    delay(200);                    
    digitalWrite(PIN_LED_1, LOW);  
    delay(200);                    
  }
  
  // Primer estado INICIO
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

// ---------------------------------------------------------
// MODO SERVICIO (Lógica original con pequeñas limpiezas)
// ---------------------------------------------------------
void handleServicioMode() {

  // Lógica de Botón Global (Pulsación Larga -> Admin)
  if (digitalRead(PIN_BOTON) == LOW) {
    if (tBotonPress == 0) tBotonPress = millis();
    
    // Si > 5 seg -> ADMIN
    if (millis() - tBotonPress >= 5000) {
      entrarModoAdmin();
      tBotonPress = 0; 
      return; 
    }
  } else {
    if (tBotonPress > 0) {
      unsigned long duracionPulsacion = millis() - tBotonPress;
      // Reinicio (2s - 5s)
      if (estadoActual != ESPERANDO && duracionPulsacion >= 2000 && duracionPulsacion < 5000) {
         reiniciarServicio();
      }
      tBotonPress = 0;
    }
  }

  // Lógica de Estados Servicio
  if (estadoActual == INICIO) {
    lcd.setCursor(4, 0);
    lcd.print("Servicio");
    if (millis() - tInicio > 1000) reiniciarServicio(); 

  } else if (estadoActual == ESPERANDO) {
    if (millis() - tInicio > 200) {
      medirDistancia();
      
      // Dibujar solo lo necesario para evitar parpadeo masivo
      // (Aquí simplificado, idealmente usar flag de cambio)
      lcd.setCursor(0, 0); lcd.print("ESPERANDO");
      lcd.setCursor(0, 1); lcd.print("CLIENTE...    "); // Espacios para limpiar
      
      tInicio = millis(); 
      if (distancia < 100) {
        estadoActual = ENCONTRADO_MSG; 
        lcd.clear();
        tInicio = millis(); 
      }
    }
    
  } else if (estadoActual == ENCONTRADO_MSG) {
    lcd.setCursor(4, 0); lcd.print("CLIENTE");
    lcd.setCursor(3, 1); lcd.print("ENCONTRADO");
    
    if (millis() - tInicio > 2000) {
      estadoActual = ENCONTRADO_DATOS;
      lcd.clear();
      tempPrev = -999; // Forzar actualización LCD
      tInicio = millis(); 
    }

  } else if (estadoActual == ENCONTRADO_DATOS) {
    float h = dht.readHumidity();
    float t = dht.readTemperature();
    
    // Actualizar solo si cambia
    if (abs(t - tempPrev) > 0.5 || abs(h - humPrev) > 1.0) {
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

  } else if (estadoActual == MENU) {
    int joyY = analogRead(PIN_JOY_Y);
    
    if(joyY < 450 && count < 4 && !joystickMovido){
      count++; 
      joystickMovido = true;
      lcd.clear(); // Limpiar al cambiar
    } else if(joyY > 650 && count > 0 && !joystickMovido){
      count--; 
      joystickMovido = true;
      lcd.clear();
    } else if (joyY >= 450 && joyY <= 650) {
      joystickMovido = false;
    }

    lcd.setCursor(0, 0); lcd.print(nombres[count]);
    lcd.setCursor(0, 1); lcd.print(precios[count], 2); lcd.print(" $");
    
    if (digitalRead(PIN_JOY_SW) == LOW) {
      delay(50); // Debounce simple en modo servicio
      while(digitalRead(PIN_JOY_SW) == LOW); 
      
      estadoActual = PREPARANDO;
      lcd.clear();
      brilloLed = 0; 
      long tiempoTotal = random(4000, 8001); 
      intervaloLed = tiempoTotal / 255;      
      tInicio = millis(); 
    }
    
  } else if (estadoActual == PREPARANDO) {
    lcd.setCursor(0, 0); lcd.print("Preparando Cafe...");
    
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
    
  } else if (estadoActual == RETIRAR) {
    lcd.setCursor(0, 0); lcd.print("RETIRE BEBIDA");
    if (millis() - tInicio > 3000) reiniciarServicio();
    
  } else if (estadoActual == REINICIANDO) {
    lcd.setCursor(0, 0); lcd.print("REINICIANDO...");
    if (millis() - tInicio > 1500) reiniciarServicio();
  }
}

// ---------------------------------------------------------
// MODO ADMIN (OPTIMIZADO SIN DELAYS)
// ---------------------------------------------------------
void handleAdminMode() {

  // --- 1. LECTURA DE CONTROLES NO BLOQUEANTE ---
  
  // Salida rápida Admin (Mantener botón pulsado)
  if (digitalRead(PIN_BOTON) == LOW) {
     long t = 0;
     while(digitalRead(PIN_BOTON) == LOW) {
       delay(10);
       t += 10;
       if (t >= 3000) { // Reducido a 3s para mejor UX
          salirModoAdmin(); 
          return;
       }
     }
  }

  // Lectura Joystick
  int joyY = analogRead(PIN_JOY_Y);
  bool currentJoySelect = (digitalRead(PIN_JOY_SW) == LOW);
  bool currentJoyBack = (analogRead(PIN_JOY_X) < 450); // Mover Joystick a la izquierda para volver

  // Detección de Flanco (Click)
  bool joySelectClick = (currentJoySelect && !prevJoySelect);
  bool joyBackClick = (currentJoyBack && !prevJoyBack);

  // Guardar estado para la siguiente vuelta
  prevJoySelect = currentJoySelect;
  prevJoyBack = currentJoyBack;

  // --- 2. MÁQUINA DE ESTADOS ADMIN ---

  // -> TRANSICIONES INICIALES/FINALES
  if (adminEstadoActual == ADMIN_INICIO) {
    if (adminRedraw) { lcd.clear(); lcd.setCursor(0,0); lcd.print("MODO ADMIN"); adminRedraw = false; }
    if (millis() - tInicio > 1000) {
      adminEstadoActual = ADMIN_MAIN;
      adminRedraw = true;
    }
    return;
  }
  
  if (adminEstadoActual == ADMIN_SALIDA) {
    if (adminRedraw) {
       lcd.clear(); lcd.setCursor(0,0); lcd.print("SALIENDO...");
       digitalWrite(PIN_LED_1, LOW); analogWrite(PIN_LED_2, 0);
       adminRedraw = false;
    }
    if (millis() - tInicio > 1000) reiniciarServicio(); 
    return;
  }

  // -> MENU PRINCIPAL
  if (adminEstadoActual == ADMIN_MAIN) {
    String menu[4] = {"Ver temp", "Ver distancia", "Ver contador", "Modificar $"};
    
    // Navegación Vertical
    if (joyY < 450 && adminCursor < 3 && !joystickMovido) {
      adminCursor++; joystickMovido = true; adminRedraw = true;
    } else if (joyY > 650 && adminCursor > 0 && !joystickMovido) {
      adminCursor--; joystickMovido = true; adminRedraw = true;
    } else if (joyY >= 450 && joyY <= 650) {
      joystickMovido = false;
    }
    
    // Dibujado (Solo si es necesario)
    if (adminRedraw) {
      lcd.clear();
      lcd.setCursor(0, 0); lcd.print("> "); lcd.print(menu[adminCursor]);
      if (adminCursor < 3) {
        lcd.setCursor(1, 1); lcd.print(menu[adminCursor + 1]);
      } else {
        // Estética: si estamos en el último, mostramos el anterior arriba
        lcd.setCursor(1, 1); lcd.print("                "); // borrar linea
        lcd.setCursor(0, 0); lcd.print(menu[adminCursor-1]);
        lcd.setCursor(0, 1); lcd.print("> "); lcd.print(menu[adminCursor]);
      }
      adminRedraw = false;
    }
    
    // Selección (Click)
    if (joySelectClick) {
      if (adminCursor == 0) adminEstadoActual = ADMIN_TEMP;
      if (adminCursor == 1) adminEstadoActual = ADMIN_DIST;
      if (adminCursor == 2) adminEstadoActual = ADMIN_COUNT;
      if (adminCursor == 3) { adminEstadoActual = ADMIN_PRICE_LIST; precioCursor = 0; }
      
      adminRedraw = true; // Forzar pintado del nuevo estado
      tRefrescoSensores = 0; // Resetear timer de sensores
    }
    
  // -> VER TEMPERATURA
  } else if (adminEstadoActual == ADMIN_TEMP) {
    // Actualizar cada 500ms para no parpadear
    if (millis() - tRefrescoSensores > 500) {
      float h = dht.readHumidity();
      float t = dht.readTemperature();
      mostrarDatosDHT(t, h);
      tRefrescoSensores = millis();
    }
    if (joyBackClick) { adminEstadoActual = ADMIN_MAIN; adminRedraw = true; }

  // -> VER DISTANCIA
  } else if (adminEstadoActual == ADMIN_DIST) {
    if (millis() - tRefrescoSensores > 500) {
      medirDistancia();
      lcd.clear();
      lcd.setCursor(0,0); lcd.print("Distancia:");
      lcd.setCursor(0,1); lcd.print(distancia); lcd.print(" cm");
      tRefrescoSensores = millis();
    }
    if (joyBackClick) { adminEstadoActual = ADMIN_MAIN; adminRedraw = true; }

  // -> VER CONTADOR
  } else if (adminEstadoActual == ADMIN_COUNT) {
    if (adminRedraw || (millis() - tRefrescoSensores > 1000)) {
      lcd.clear();
      lcd.print("Tiempo ON:");
      lcd.setCursor(0,1); lcd.print(millis()/1000); lcd.print(" seg");
      adminRedraw = false;
      tRefrescoSensores = millis();
    }
    if (joyBackClick) { adminEstadoActual = ADMIN_MAIN; adminRedraw = true; }

  // -> LISTA DE PRECIOS
  } else if (adminEstadoActual == ADMIN_PRICE_LIST) {
    if (joyY < 450 && precioCursor < 4 && !joystickMovido) {
      precioCursor++; joystickMovido = true; adminRedraw = true;
    } else if (joyY > 650 && precioCursor > 0 && !joystickMovido) {
      precioCursor--; joystickMovido = true; adminRedraw = true;
    } else if (joyY >= 450 && joyY <= 650) {
      joystickMovido = false;
    }
    
    if (adminRedraw) {
      lcd.clear();
      lcd.print(">"); lcd.print(nombres[precioCursor]);
      lcd.setCursor(0, 1); lcd.print(precios[precioCursor], 2); lcd.print(" $");
      adminRedraw = false;
    }
    
    if (joySelectClick) {
      adminEstadoActual = ADMIN_PRICE_EDIT;
      adminRedraw = true;
    }
    if (joyBackClick) {
      adminEstadoActual = ADMIN_MAIN;
      adminCursor = 3; 
      adminRedraw = true;
    }
    
  // -> EDITAR PRECIO
  } else if (adminEstadoActual == ADMIN_PRICE_EDIT) {
    if (joyY < 450 && !joystickMovido) {
      precios[precioCursor] -= 0.05; joystickMovido = true; adminRedraw = true;
    } else if (joyY > 650 && !joystickMovido) {
      precios[precioCursor] += 0.05; joystickMovido = true; adminRedraw = true;
    } else if (joyY >= 450 && joyY <= 650) {
      joystickMovido = false;
    }
    
    if (adminRedraw) {
      lcd.clear();
      lcd.print(nombres[precioCursor]);
      lcd.setCursor(0, 1); lcd.print(precios[precioCursor], 2); lcd.print(" $ (Edit)");
      adminRedraw = false;
    }
    
    if (joySelectClick || joyBackClick) { // Guardar y salir con clic o atrás
      adminEstadoActual = ADMIN_PRICE_LIST;
      adminRedraw = true;
    }
  }

  // Pequeño delay de estabilidad (ya no bloquea 200ms)
  delay(10); 
}

// --- FUNCIONES AUXILIARES ---

void entrarModoAdmin() {
  estadoActual = ADMIN;
  adminEstadoActual = ADMIN_INICIO; 
  adminCursor = 0;
  adminRedraw = true;
  
  digitalWrite(PIN_LED_1, HIGH);
  analogWrite(PIN_LED_2, 255); 
  
  lcd.clear();
  tInicio = millis(); 
}

void salirModoAdmin() {
  adminEstadoActual = ADMIN_SALIDA;
  adminRedraw = true;
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
    lcd.setCursor(0, 0); lcd.print("Temp:"); lcd.print(t, 1); lcd.print("C");
    lcd.setCursor(0, 1); lcd.print("Hum:"); lcd.print(h, 1); lcd.print("%");
  }
}
