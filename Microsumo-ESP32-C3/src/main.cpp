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

void automaticMode() {

  static unsigned long previousMillis = 0;
  static unsigned long actionDuration = 0;
  static int state = 0;
  static bool recoveringLine = false;


    // Prioridad: evitar salirse de la línea
    if (SensorsManager::readLineSensor()) {
      MotorsController::stop();  // opcional: frena antes de corregir
      MotorsController::backward();
      previousMillis = millis();
      actionDuration = 1000;
      state = -1; // estado especial de recuperación
      recoveringLine = true;
      return;
    }

    unsigned long currentMillis = millis();

    if (currentMillis - previousMillis >= actionDuration) {
      previousMillis = currentMillis;

      if (recoveringLine) {
        // Segundo paso: giro a la izquierda tras retroceso
        MotorsController::left();
        actionDuration = 1300;
        state = 0; // vuelve al inicio de la exploración
        recoveringLine = false;
        return;
      }

      // Secuencia de exploración del tablero
      switch (state) {
        case 0:
          MotorsController::forward();
          actionDuration = random(100, 1000);
          state = 1;
          break;
        case 1:
          MotorsController::right();
          actionDuration = random(100, 1000);
          state = 2;
          break;
        case 2:
          MotorsController::forward();
          actionDuration = random(100, 1000);
          state = 3;
          break;
        case 3:
          MotorsController::left();
          actionDuration = random(100, 1000);
          state = 4;
          break;
        case 4:
          MotorsController::forward();
          actionDuration = random(100, 1000);
          state = 0;  // reinicia la secuencia
          break;
      }
    
  }
}