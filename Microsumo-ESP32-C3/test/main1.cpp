#define BLE_MODE 1
#define WIFI_MODE 2
#define AUTOMATIC_MODE 3

#include <Arduino.h>
#include <Wire.h>
#include "Adafruit_VL6180X.h"
#include "MotorsController.h"
#include "SensorsManager.h"

#if ROBOT_MODE == BLE_MODE
  #include "BLEManager.h"
#endif

#if ROBOT_MODE == WIFI_MODE
  #include "WifiManager.h"
#endif


void automaticMode();


// the setup function runs once when you press reset or power the board
void setup() {
  delay(5000);
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);


  MotorsController::setup();
  SensorsManager::setupLine();


  #if ROBOT_MODE == BLE_MODE
    BLEManager::setupBLE();
  #endif
  #if ROBOT_MODE == WIFI_MODE
    WifiManager::setup();
  #endif

}

void loop() {
  #if ROBOT_MODE == BLE_MODE
    BLEManager::ensureAdvertising();
  #endif
  #if ROBOT_MODE == WIFI_MODE
    WifiManager::run();
  #endif

  #if ROBOT_MODE == AUTOMATIC_MODE
    automaticMode();
    #endif
}

// Función que implementa el modo automático del robot
void automaticMode() {

  // Variables estáticas para mantener el estado entre llamadas a la función
  static unsigned long previousMillis = 0;     // Guarda el tiempo de la última acción
  static unsigned long actionDuration = 0;     // Duración de la acción actual
  static int state = 0;                        // Estado actual de la máquina de estados
  static bool recoveringLine = false;          // Indica si el robot está recuperando la línea

  // --- PRIORIDAD: Evitar salirse de la línea ---
  // Si el sensor de línea detecta que el robot está saliendo del área permitida
  if (SensorsManager::readLineSensor()) {
    MotorsController::stop();        // Opcional: frena antes de corregir la trayectoria
    MotorsController::backward();    // Retrocede para alejarse del borde
    previousMillis = millis();       // Guarda el tiempo actual
    actionDuration = 1000;           // Duración del retroceso (1 segundo)
    state = -1;                      // Estado especial de recuperación
    recoveringLine = true;           // Marca que está en proceso de recuperación
    return;                          // Sale de la función para esperar a que termine la acción
  }

  // Obtiene el tiempo actual
  unsigned long currentMillis = millis();

  // Si ha pasado el tiempo de la acción anterior, se decide la siguiente acción
  if (currentMillis - previousMillis >= actionDuration) {
    previousMillis = currentMillis;  // Actualiza el tiempo de referencia

    // Si estaba recuperando la línea, ahora gira a la izquierda para volver al área segura
    if (recoveringLine) {
      MotorsController::left();      // Gira a la izquierda
      actionDuration = 1300;         // Duración del giro (1.3 segundos)
      state = 0;                     // Vuelve al inicio de la secuencia de exploración
      recoveringLine = false;        // Termina la recuperación
      return;
    }

    // --- SECUENCIA DE EXPLORACIÓN DEL TABLERO ---
    // Máquina de estados para mover el robot de forma autónoma
    switch (state) {
      case 0:
        MotorsController::forward();                 // Avanza hacia adelante
        actionDuration = random(100, 1000);         // Tiempo aleatorio de avance
        state = 1;                                  // Siguiente estado: giro a la derecha
        break;
      case 1:
        MotorsController::right();                   // Gira a la derecha
        actionDuration = random(100, 1000);         // Tiempo aleatorio de giro
        state = 2;                                  // Siguiente estado: avanzar
        break;
      case 2:
        MotorsController::forward();                 // Avanza hacia adelante
        actionDuration = random(100, 1000);         // Tiempo aleatorio de avance
        state = 3;                                  // Siguiente estado: giro a la izquierda
        break;
      case 3:
        MotorsController::left();                    // Gira a la izquierda
        actionDuration = random(100, 1000);         // Tiempo aleatorio de giro
        state = 4;                                  // Siguiente estado: avanzar
        break;
      case 4:
        MotorsController::forward();                 // Avanza hacia adelante
        actionDuration = random(100, 1000);         // Tiempo aleatorio de avance
        state = 0;                                  // Reinicia la secuencia
        break;
    }
  }
}