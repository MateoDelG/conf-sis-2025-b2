#include <Arduino.h>
#include "GyroTurn.h"
#include "MotorsController.h"
#include "SensorsManager.h"

GyroTurn gyro;

enum State
{
  TURN_RIGHT,
  MOVE_FORWARD_1,
  TURN_LEFT,
  MOVE_FORWARD_2,
  AVOID_LINE_BACK,
  AVOID_LINE_TURN,
  ATTACK // 🆕 Nuevo estado
};

State state = TURN_RIGHT;
unsigned long movementStartTime = 0;

void setup()
{
  SensorsManager::setupVL6180X(); // Initialize distance sensor
  delay(5000);
  Serial.begin(115200);
  MotorsController::setup();
  SensorsManager::setupLine(); // Initialize line sensors

  gyro.begin();
  gyro.resetAngle();
  Serial.println("Calibrating...");
  gyro.calibrate();
  Serial.println("Calibration complete");

  gyro.setAngle(90); // Initial right turn
}
void loop()
{
  const int ATTACK_THRESHOLD = 100; // milímetros
  int distanceLeft = SensorsManager::readDistance(SensorsManager::LEFT);
  int distanceRight = SensorsManager::readDistance(SensorsManager::RIGHT);
  float angle = gyro.getCurrentAngle();
  bool lineDetected = SensorsManager::readLineSensor();

  switch (state)
  {
  case TURN_RIGHT:
    if (lineDetected)
    {
      Serial.println("⚠️ Line detected during TURN_RIGHT!");
      MotorsController::stop();
      movementStartTime = millis();
      state = AVOID_LINE_BACK;
      break;
    }

    if ((distanceLeft > 0 && distanceLeft < ATTACK_THRESHOLD) ||
        (distanceRight > 0 && distanceRight < ATTACK_THRESHOLD))
    {
      Serial.println("🟥 Enemy detected during TURN_RIGHT! Attacking...");
      MotorsController::forward();
      movementStartTime = millis();
      state = ATTACK;
      break;
    }

    MotorsController::right();
    Serial.print("[Turning Right] Angle: ");
    Serial.println(angle);
    if (!gyro.isTurning())
    {
      MotorsController::stop();
      movementStartTime = millis();
      state = MOVE_FORWARD_1;
    }
    break;

  case MOVE_FORWARD_1:
    if (lineDetected)
    {
      Serial.println("⚠️ Line detected during MOVE_FORWARD_1!");
      MotorsController::stop();
      movementStartTime = millis();
      state = AVOID_LINE_BACK;
      break;
    }

    if ((distanceLeft > 0 && distanceLeft < ATTACK_THRESHOLD) ||
        (distanceRight > 0 && distanceRight < ATTACK_THRESHOLD))
    {
      Serial.println("🟥 Enemy detected during MOVE_FORWARD_1! Attacking...");
      MotorsController::forward();
      movementStartTime = millis();
      state = ATTACK;
      break;
    }

    MotorsController::forward();
    if (millis() - movementStartTime >= 800)
    {
      MotorsController::stop();
      gyro.resetAngle();
      gyro.setAngle(-90);
      state = TURN_LEFT;
    }
    break;

  case TURN_LEFT:
    if (lineDetected)
    {
      Serial.println("⚠️ Line detected during TURN_LEFT!");
      MotorsController::stop();
      movementStartTime = millis();
      state = AVOID_LINE_BACK;
      break;
    }

    if ((distanceLeft > 0 && distanceLeft < ATTACK_THRESHOLD) ||
        (distanceRight > 0 && distanceRight < ATTACK_THRESHOLD))
    {
      Serial.println("🟥 Enemy detected during TURN_LEFT! Attacking...");
      MotorsController::forward();
      movementStartTime = millis();
      state = ATTACK;
      break;
    }

    MotorsController::left();
    Serial.print("[Turning Left] Angle: ");
    Serial.println(angle);
    if (!gyro.isTurning())
    {
      MotorsController::stop();
      movementStartTime = millis();
      state = MOVE_FORWARD_2;
    }
    break;

  case MOVE_FORWARD_2:
    if (lineDetected)
    {
      Serial.println("⚠️ Line detected during MOVE_FORWARD_2!");
      MotorsController::stop();
      movementStartTime = millis();
      state = AVOID_LINE_BACK;
      break;
    }

    if ((distanceLeft > 0 && distanceLeft < ATTACK_THRESHOLD) ||
        (distanceRight > 0 && distanceRight < ATTACK_THRESHOLD))
    {
      Serial.println("🟥 Enemy detected during MOVE_FORWARD_2! Attacking...");
      MotorsController::forward();
      movementStartTime = millis();
      state = ATTACK;
      break;
    }

    MotorsController::forward();
    if (millis() - movementStartTime >= 800)
    {
      MotorsController::stop();
      gyro.resetAngle();
      gyro.setAngle(90);
      state = TURN_RIGHT;
    }
    break;

  case AVOID_LINE_BACK:
    MotorsController::backward();
    if (millis() - movementStartTime >= 500)
    {
      MotorsController::stop();
      gyro.resetAngle();
      gyro.setAngle(180);
      state = AVOID_LINE_TURN;
    }
    break;

  case AVOID_LINE_TURN:
    // ✅ Permitir detección de enemigo durante evasión
    if ((distanceLeft > 0 && distanceLeft < ATTACK_THRESHOLD) ||
        (distanceRight > 0 && distanceRight < ATTACK_THRESHOLD))
    {
      Serial.println("🟥 Enemy detected during AVOID_LINE_TURN! Attacking...");
      MotorsController::forward();
      movementStartTime = millis();
      state = ATTACK;
      break;
    }

    MotorsController::right();
    Serial.print("[Turning Around] Angle: ");
    Serial.println(angle);

    if (!gyro.isTurning())
    {
      MotorsController::stop();
      movementStartTime = millis();
      state = MOVE_FORWARD_2;
    }
    break;

  case ATTACK:
    if (lineDetected)
    {
      Serial.println("⚠️ Line detected during ATTACK!");
      MotorsController::stop();
      movementStartTime = millis();
      state = AVOID_LINE_BACK;
      break;
    }

    if (millis() - movementStartTime >= 1000)
    {
      MotorsController::stop();
      gyro.resetAngle();
      gyro.setAngle(90);
      state = TURN_RIGHT;
    }
    break;
  }
}

// void loop()
// {
//   const int ATTACK_THRESHOLD = 100; // milímetros
//   int distanceLeft = SensorsManager::readDistance(SensorsManager::LEFT);
//   int distanceRight = SensorsManager::readDistance(SensorsManager::RIGHT);
//   float angle = gyro.getCurrentAngle();

//     // Always check for line (edge)
// if (SensorsManager::readLineSensor()) {
//   MotorsController::stop();
//   Serial.println("⚠️ Line detected! Interrupting current action.");
//   movementStartTime = millis();
//   state = AVOID_LINE_BACK;
//   return;
// }

//   // Condición de ataque: si cualquiera detecta un objeto cerca
// if ((state != AVOID_LINE_BACK &&
//      state != AVOID_LINE_TURN &&
//      state != ATTACK) &&  // evita reiniciar ataque
//     ((distanceLeft > 0 && distanceLeft < ATTACK_THRESHOLD) ||
//      (distanceRight > 0 && distanceRight < ATTACK_THRESHOLD)) &&
//     !SensorsManager::readLineSensor())
// {
//   MotorsController::forward();
//   Serial.println("🟥 ATTACK MODE: Charging!");
//   movementStartTime = millis();
//   state = ATTACK;
//   return;
// }

//   switch (state)
//   {
//   case TURN_RIGHT:
//     MotorsController::right();
//     Serial.print("[Turning Right] Angle: ");
//     Serial.println(angle);
//     if (!gyro.isTurning())
//     {
//       MotorsController::stop();
//       movementStartTime = millis();
//       state = MOVE_FORWARD_1;
//     }
//     break;

//   case MOVE_FORWARD_1:
//     MotorsController::forward();
//     if (millis() - movementStartTime >= 800)
//     {
//       MotorsController::stop();
//       gyro.resetAngle();
//       gyro.setAngle(-90); // Turn left next
//       state = TURN_LEFT;
//     }
//     break;

//   case TURN_LEFT:
//     MotorsController::left();
//     Serial.print("[Turning Left] Angle: ");
//     Serial.println(angle);
//     if (!gyro.isTurning())
//     {
//       MotorsController::stop();
//       movementStartTime = millis();
//       state = MOVE_FORWARD_2;
//     }
//     break;

//   case MOVE_FORWARD_2:
//     MotorsController::forward();
//     if (millis() - movementStartTime >= 800)
//     {
//       MotorsController::stop();
//       gyro.resetAngle();
//       gyro.setAngle(90); // Return to right turn
//       state = TURN_RIGHT;
//     }
//     break;

//   case AVOID_LINE_BACK:
//     MotorsController::backward();
//     if (millis() - movementStartTime >= 500)
//     {
//       MotorsController::stop();
//       gyro.resetAngle();
//       gyro.setAngle(180); // Rotate 180°
//       state = AVOID_LINE_TURN;
//     }
//     break;

//   case AVOID_LINE_TURN:
//     MotorsController::right(); // Could also use MotorsController::turnAround()
//     Serial.print("[Turning Around] Angle: ");
//     Serial.println(angle);
//     if (!gyro.isTurning())
//     {
//       MotorsController::stop();
//       movementStartTime = millis();
//       state = MOVE_FORWARD_2; // Resume after evasion
//     }
//     break;

//   case ATTACK:
//   // Ya está en movimiento hacia adelante
//   if (millis() - movementStartTime >= 1000) { // 1 segundo de ataque
//     MotorsController::stop();
//     gyro.resetAngle();
//     gyro.setAngle(90); // O retoma ángulo deseado
//     state = TURN_RIGHT;
//   }
//   break;
//   }
// }
