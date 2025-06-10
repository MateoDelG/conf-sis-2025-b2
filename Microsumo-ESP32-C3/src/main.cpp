#include <Arduino.h>
#include "GyroTurn.h"
#include "MotorsController.h"
#include "SensorsManager.h"

#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiServer.h>
#include <ArduinoOTA.h>

const char *ssid = "Delga";
const char *password = "Delga1213";

WiFiServer telnetServer(23);
WiFiClient telnetClient;

GyroTurn gyro;

enum State
{
  TURN_RIGHT,
  MOVE_FORWARD_1,
  TURN_LEFT,
  MOVE_FORWARD_2,
  AVOID_LINE_BACK,
  AVOID_LINE_TURN,
  ATTACK,
  ATTACK_LEFT, // 🆕 Prepara ataque hacia la izquierda
  ATTACK_RIGHT // 🆕 Prepara ataque hacia la derecha
};

enum RobotState
{
  BALANCING,
  TURNING,
  PATROLLING,
  AVOID_LINE // nuevo estado
};

RobotState currentState = BALANCING;

enum AvoidLineStep
{
  STOP,
  BACKWARD,
  TURN,
  RESUME
};
static AvoidLineStep avoidStep = STOP;
static unsigned long avoidStepStart = 0;
const unsigned long BACKWARD_TIME = 800;
const unsigned long TURN_TIME = 600;

State state = TURN_RIGHT;

void telenet()
{
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\n✅ WiFi connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  telnetServer.begin();
  telnetServer.setNoDelay(true);

  // OTA
  ArduinoOTA.setHostname("ESP32-Robot");
  ArduinoOTA.begin();
  Serial.println("✅ OTA Ready");
}

void log(const String &message)
{
  Serial.println(message);
  if (telnetClient && telnetClient.connected())
  {
    telnetClient.println(message);
  }
}

void setup()
{
  SensorsManager::setupVL6180X(); // Initialize distance sensor
  Serial.begin(115200);
  delay(2000);
  telenet();
  MotorsController::setup();
  SensorsManager::setupLine(); // Initialize line sensors

  gyro.begin();
  gyro.resetAngle();
  Serial.println("Calibrating...");
  gyro.calibrate();
  Serial.println("Calibration complete");

  gyro.setAngle(90); // Initial right turn
}

void telenetClient()
{
  // OTA
  ArduinoOTA.handle();

  // Telnet
  if (telnetServer.hasClient())
  {
    if (!telnetClient || !telnetClient.connected())
    {
      telnetClient = telnetServer.available();
      Serial.println("🔌 Telnet client connected");
      telnetClient.println("🟢 Connected to ESP32 Telnet");
    }
    else
    {
      telnetServer.available().stop();
    }
  }

  if (telnetClient && telnetClient.connected() && telnetClient.available())
  {
    String msg = telnetClient.readStringUntil('\n');
    Serial.print("📨 Telnet: ");
    Serial.println(msg);
    // Aquí podrías interpretar comandos si deseas
  }
}

void loop()
{
  telenetClient();

  const int BALANCE_THRESHOLD = 20;
  const int MIN_VALID_DISTANCE = 1;
  const int MAX_VALID_DISTANCE = 150;
  const int MAX_DIFF = 20;
  const int MIN_CORRECTION_ANGLE = 1;
  const int MAX_CORRECTION_ANGLE = 25;
  const unsigned long PATROL_STEP_INTERVAL = 1000;

  static float targetAngle = 0;
  static int patrolStep = 0;
  static unsigned long lastPatrolStepTime = 0;
  static unsigned long avoidLineStartTime = 0;
  const unsigned long AVOID_LINE_DURATION = 1500;

  float currentAngle = gyro.getCurrentAngle();
  log("🧭 Gyro Angle: " + String(currentAngle, 1));
  log("🛠 Estado actual: " + String(currentState == BALANCING ? "BALANCING" : currentState == TURNING  ? "TURNING"
                                                                         : currentState == PATROLLING ? "PATROLLING"
                                                                                                      : "AVOID_LINE"));

  // Verificar línea en cualquier momento
  if (SensorsManager::readLineSensor() && currentState != AVOID_LINE)
  {
    MotorsController::stop();
    log("🚫 Línea detectada. Cambiando a estado AVOID_LINE.");
    avoidLineStartTime = millis();
    currentState = AVOID_LINE;
    return;
  }

  auto handleSingleSensorFailure = [&](bool leftFailed)
  {
    targetAngle = leftFailed ? 1 : -1;
    leftFailed ? MotorsController::right() : MotorsController::left();
    log(leftFailed ? "⚠️ Left sensor failed. Rotating right by 1°"
                   : "⚠️ Right sensor failed. Rotating left by 1°");
    gyro.resetAngle();
    gyro.setAngle(targetAngle);
    currentState = TURNING;
  };

  switch (currentState)
  {
  case BALANCING:
  {
    int rawLeft = SensorsManager::readDistance(SensorsManager::LEFT);
    int rawRight = SensorsManager::readDistance(SensorsManager::RIGHT);

    int distanceLeft = (rawLeft != -1) ? constrain(rawLeft, MIN_VALID_DISTANCE, MAX_VALID_DISTANCE) : -1;
    int distanceRight = (rawRight != -1) ? constrain(rawRight, MIN_VALID_DISTANCE, MAX_VALID_DISTANCE) : -1;

    log("🔍 Distances - Left: " + String(distanceLeft) + " mm, Right: " + String(distanceRight) + " mm");

    if (distanceLeft == -1 && distanceRight == -1)
    {
      log("🚶 Ambos sensores fallaron. Iniciando patrullaje inmediatamente.");
      currentState = PATROLLING;
      patrolStep = 0;
      lastPatrolStepTime = millis();
      return;
    }

    if (distanceLeft == -1)
    {
      handleSingleSensorFailure(true);
      return;
    }

    if (distanceRight == -1)
    {
      handleSingleSensorFailure(false);
      return;
    }

    int diff = distanceLeft - distanceRight;
    log("📏 Difference (L - R): " + String(diff) + " mm");

    if (abs(diff) <= BALANCE_THRESHOLD)
    {
      MotorsController::forward();
      log("✅ Balanced. Moving forward.");
    }
    else
    {
      int correctionAngle = map(abs(diff), BALANCE_THRESHOLD, MAX_DIFF, MIN_CORRECTION_ANGLE, MAX_CORRECTION_ANGLE);
      correctionAngle = constrain(correctionAngle, MIN_CORRECTION_ANGLE, MAX_CORRECTION_ANGLE);
      targetAngle = (diff > 0) ? correctionAngle : -correctionAngle;
      (diff > 0) ? MotorsController::right() : MotorsController::left();

      log((diff > 0) ? "↪ Too close on left. Correcting right by " + String(correctionAngle) + "°"
                     : "↩ Too close on right. Correcting left by " + String(correctionAngle) + "°");

      log("🎯 Target angle set to: " + String(targetAngle, 1));
      gyro.resetAngle();
      gyro.setAngle(targetAngle);
      currentState = TURNING;
    }
    break;
  }

  case TURNING:
    log("🔄 Turning... Current angle: " + String(currentAngle, 1));
    if (!gyro.isTurning())
    {
      MotorsController::stop();
      log("✅ Correction complete. Returning to BALANCING.");
      currentState = BALANCING;
    }
    break;

  case PATROLLING:
  {
    const unsigned long PATROL_FORWARD_DURATION = 1000; // Tiempo de avance
    const float PATROL_TURN_ANGLE = 30.0;               // Ángulo de giro
    static bool turning = false;
    static float turnTargetAngle = 0;

    if (!turning)
    {
      if (patrolStep % 2 == 0)
      {
        // Paso par: avanzar
        if (millis() - lastPatrolStepTime >= PATROL_FORWARD_DURATION)
        {
          patrolStep++;
          lastPatrolStepTime = millis();
          MotorsController::stop();
          log("🛑 Fin de avance, siguiente paso.");
        }
        else
        {
          MotorsController::forward();
          log("🚶 Patrulla: Avanzando");
        }
      }
      else
      {
        // Paso impar: girar
        turnTargetAngle = (patrolStep % 4 == 1) ? -PATROL_TURN_ANGLE : PATROL_TURN_ANGLE;
        gyro.resetAngle();
        gyro.setAngle(turnTargetAngle);
        if (turnTargetAngle > 0)
        {
          MotorsController::right();
          log("↪️ Patrulla: Girando derecha por " + String(turnTargetAngle) + "°");
        }
        else
        {
          MotorsController::left();
          log("↩️ Patrulla: Girando izquierda por " + String(abs(turnTargetAngle)) + "°");
        }
        turning = true;
      }
    }
    else
    {
      // Esperar a que termine el giro
      if (!gyro.isTurning())
      {
        MotorsController::stop();
        log("✅ Giro completado.");
        turning = false;
        patrolStep++;
        lastPatrolStepTime = millis();
      }
    }

    // Revisión de sensores para volver a BALANCING si se detecta diferencia
    int rawLeft = SensorsManager::readDistance(SensorsManager::LEFT);
    int rawRight = SensorsManager::readDistance(SensorsManager::RIGHT);
    if (rawLeft != -1 && rawRight != -1)
    {
      int distanceLeft = constrain(rawLeft, MIN_VALID_DISTANCE, MAX_VALID_DISTANCE);
      int distanceRight = constrain(rawRight, MIN_VALID_DISTANCE, MAX_VALID_DISTANCE);
      int diff = distanceLeft - distanceRight;

      if (abs(diff) > BALANCE_THRESHOLD)
      {
        log("🛑 Diferencia detectada. Cancelando patrullaje.");
        currentState = BALANCING;
      }
    }
    break;
  }

  case AVOID_LINE:
{
  const unsigned long AVOID_LINE_DURATION = 800; // tiempo de retroceso
  const float AVOID_TURN_ANGLE = 180;            // ángulo para evitar línea

  unsigned long elapsed = millis() - avoidLineStartTime;

  if (elapsed < AVOID_LINE_DURATION)
  {
    MotorsController::backward();
    log("⏪ Estado AVOID_LINE: Retrocediendo.");
  }
  else
  {
    static bool startedTurn = false;
    static float initialAngle = 0;

    if (!startedTurn)
    {
      initialAngle = gyro.getCurrentAngle();
      gyro.resetAngle();
      gyro.setAngle(AVOID_TURN_ANGLE);
      MotorsController::right();
      startedTurn = true;
      log("↪ Estado AVOID_LINE: Iniciando giro para evitar línea.");
    }

    // Revisión de sensores en medio del giro
    int rawLeft = SensorsManager::readDistance(SensorsManager::LEFT);
    int rawRight = SensorsManager::readDistance(SensorsManager::RIGHT);

    if (rawLeft != -1 || rawRight != -1)
    {
      int distanceLeft = constrain(rawLeft, MIN_VALID_DISTANCE, MAX_VALID_DISTANCE);
      int distanceRight = constrain(rawRight, MIN_VALID_DISTANCE, MAX_VALID_DISTANCE);
      int diff = distanceLeft - distanceRight;

      if (abs(diff) > BALANCE_THRESHOLD)
      {
        MotorsController::stop();
        log("🛑 Obstáculo detectado durante giro. Cambio a BALANCING.");
        currentState = BALANCING;
        startedTurn = false;
        return;
      }
    }

    if (!gyro.isTurning())
    {
      MotorsController::stop();
      log("✅ Línea evitada. Volviendo a patrullaje.");
      currentState = PATROLLING;
      patrolStep = 0;
      lastPatrolStepTime = millis();
      startedTurn = false;
    }
  }
  break;
}
  }
}

// BALANCING

// void loop() {
//   telenetClient();

//   const int BALANCE_THRESHOLD = 20;
//   const int MIN_VALID_DISTANCE = 1;
//   const int MAX_VALID_DISTANCE = 150;
//   const int MAX_DIFF = 20;
//   const int MIN_CORRECTION_ANGLE = 1;
//   const int MAX_CORRECTION_ANGLE = 25;

//   static float targetAngle = 0;
//   float currentAngle = gyro.getCurrentAngle();

//   log("🧭 Gyro Angle: " + String(currentAngle, 1));
//   log("🛠 Estado actual: " + String(currentState == BALANCING ? "BALANCING" : "TURNING"));

//   // Función auxiliar interna para simplificar el control cuando falla un solo sensor
//   auto handleSingleSensorFailure = [&](bool leftFailed) {
//     targetAngle = leftFailed ? 1 : -1;
//     leftFailed ? MotorsController::right() : MotorsController::left();

//     log(leftFailed ?
//         "⚠️ Left sensor failed. Rotating right by 1°" :
//         "⚠️ Right sensor failed. Rotating left by 1°");

//     gyro.resetAngle();
//     gyro.setAngle(targetAngle);
//     currentState = TURNING;
//   };

//   switch (currentState) {
//     case BALANCING: {
//       int rawLeft = SensorsManager::readDistance(SensorsManager::LEFT);
//       int rawRight = SensorsManager::readDistance(SensorsManager::RIGHT);

//       int distanceLeft  = (rawLeft  != -1) ? constrain(rawLeft,  MIN_VALID_DISTANCE, MAX_VALID_DISTANCE) : -1;
//       int distanceRight = (rawRight != -1) ? constrain(rawRight, MIN_VALID_DISTANCE, MAX_VALID_DISTANCE) : -1;

//       log("🔍 Distances - Left: " + String(distanceLeft) + " mm, Right: " + String(distanceRight) + " mm");
//       // delay(200);  // Solo para depuración

//       if (distanceLeft == -1 && distanceRight == -1) {
//         MotorsController::stop();
//         log("❗ Both sensors failed. Robot stopped.");
//         return;
//       }

//       if (distanceLeft == -1) {
//         handleSingleSensorFailure(true);
//         return;
//       }

//       if (distanceRight == -1) {
//         handleSingleSensorFailure(false);
//         return;
//       }

//       int diff = distanceLeft - distanceRight;
//       log("📏 Difference (L - R): " + String(diff) + " mm");

//       if (abs(diff) <= BALANCE_THRESHOLD) {
//         MotorsController::forward();
//         log("✅ Balanced. Moving forward.");
//       } else {
//         int correctionAngle = map(abs(diff), BALANCE_THRESHOLD, MAX_DIFF, MIN_CORRECTION_ANGLE, MAX_CORRECTION_ANGLE);
//         correctionAngle = constrain(correctionAngle, MIN_CORRECTION_ANGLE, MAX_CORRECTION_ANGLE);

//         targetAngle = (diff > 0) ? correctionAngle : -correctionAngle;
//         diff > 0 ? MotorsController::right() : MotorsController::left();

//         log((diff > 0) ?
//             "↪ Too close on left. Correcting right by " + String(correctionAngle) + "°" :
//             "↩ Too close on right. Correcting left by " + String(correctionAngle) + "°");

//         log("🎯 Target angle set to: " + String(targetAngle, 1));
//         gyro.resetAngle();
//         gyro.setAngle(targetAngle);
//         currentState = TURNING;
//       }
//       break;
//     }

//     case TURNING:
//       log("🔄 Turning... Current angle: " + String(currentAngle, 1));
//       if (!gyro.isTurning()) {
//         MotorsController::stop();
//         log("✅ Correction complete. Returning to BALANCING.");
//         currentState = BALANCING;
//       }
//       break;
//   }
// }

// void loop()
// {
//   const int ATTACK_THRESHOLD = 100; // milímetros
//   const int ATTACK_SIDE_DIFFERENCE_THRESHOLD = 20; // milímetros
//   int distanceLeft = SensorsManager::readDistance(SensorsManager::LEFT);
//   int distanceRight = SensorsManager::readDistance(SensorsManager::RIGHT);
//   float angle = gyro.getCurrentAngle();
//   bool lineDetected = SensorsManager::readLineSensor();

//   switch (state)
//   {

//   case TURN_RIGHT:
//     if (lineDetected)
//     {
//       Serial.println("⚠️ Line detected during TURN_RIGHT!");
//       MotorsController::stop();
//       movementStartTime = millis();
//       state = AVOID_LINE_BACK;
//       break;
//     }
//     if ((distanceLeft > 0 && distanceLeft < ATTACK_THRESHOLD) ||
//         (distanceRight > 0 && distanceRight < ATTACK_THRESHOLD))
//     {
//       if (distanceLeft < distanceRight)
//       {
//         Serial.println("🟥 Enemy on LEFT: Preparing attack");
//         gyro.resetAngle();
//         gyro.setAngle(-30); // Gira a la izquierda
//         state = ATTACK_LEFT;
//       }
//       else if (distanceRight < distanceLeft)
//       {
//         Serial.println("🟥 Enemy on RIGHT: Preparing attack");
//         gyro.resetAngle();
//         gyro.setAngle(30); // Gira a la derecha
//         state = ATTACK_RIGHT;
//       }
//       else
//       {
//         Serial.println("🟥 Enemy in FRONT: Attacking directly");
//         MotorsController::forward();
//         movementStartTime = millis();
//         state = ATTACK;
//       }
//       break;
//     }

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
//     if (lineDetected)
//     {
//       Serial.println("⚠️ Line detected during MOVE_FORWARD_1!");
//       MotorsController::stop();
//       movementStartTime = millis();
//       state = AVOID_LINE_BACK;
//       break;
//     }
//     if ((distanceLeft > 0 && distanceLeft < ATTACK_THRESHOLD) ||
//         (distanceRight > 0 && distanceRight < ATTACK_THRESHOLD))
//     {
//       if (distanceLeft < distanceRight)
//       {
//         Serial.println("🟥 Enemy on LEFT: Preparing attack");
//         gyro.resetAngle();
//         gyro.setAngle(-30);
//         state = ATTACK_LEFT;
//       }
//       else if (distanceRight < distanceLeft)
//       {
//         Serial.println("🟥 Enemy on RIGHT: Preparing attack");
//         gyro.resetAngle();
//         gyro.setAngle(30);
//         state = ATTACK_RIGHT;
//       }
//       else
//       {
//         Serial.println("🟥 Enemy in FRONT: Attacking directly");
//         MotorsController::forward();
//         movementStartTime = millis();
//         state = ATTACK;
//       }
//       break;
//     }

//     MotorsController::forward();
//     if (millis() - movementStartTime >= 800)
//     {
//       MotorsController::stop();
//       gyro.resetAngle();
//       gyro.setAngle(-90);
//       state = TURN_LEFT;
//     }
//     break;

//   case TURN_LEFT:
//     if (lineDetected)
//     {
//       Serial.println("⚠️ Line detected during TURN_LEFT!");
//       MotorsController::stop();
//       movementStartTime = millis();
//       state = AVOID_LINE_BACK;
//       break;
//     }
//     if ((distanceLeft > 0 && distanceLeft < ATTACK_THRESHOLD) ||
//         (distanceRight > 0 && distanceRight < ATTACK_THRESHOLD))
//     {
//       if (distanceLeft < distanceRight)
//       {
//         Serial.println("🟥 Enemy on LEFT: Preparing attack");
//         gyro.resetAngle();
//         gyro.setAngle(-30);
//         state = ATTACK_LEFT;
//       }
//       else if (distanceRight < distanceLeft)
//       {
//         Serial.println("🟥 Enemy on RIGHT: Preparing attack");
//         gyro.resetAngle();
//         gyro.setAngle(30);
//         state = ATTACK_RIGHT;
//       }
//       else
//       {
//         Serial.println("🟥 Enemy in FRONT: Attacking directly");
//         MotorsController::forward();
//         movementStartTime = millis();
//         state = ATTACK;
//       }
//       break;
//     }

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
//     if (lineDetected)
//     {
//       Serial.println("⚠️ Line detected during MOVE_FORWARD_2!");
//       MotorsController::stop();
//       movementStartTime = millis();
//       state = AVOID_LINE_BACK;
//       break;
//     }
//     if ((distanceLeft > 0 && distanceLeft < ATTACK_THRESHOLD) ||
//         (distanceRight > 0 && distanceRight < ATTACK_THRESHOLD))
//     {
//       if (distanceLeft < distanceRight)
//       {
//         Serial.println("🟥 Enemy on LEFT: Preparing attack");
//         gyro.resetAngle();
//         gyro.setAngle(-30);
//         state = ATTACK_LEFT;
//       }
//       else if (distanceRight < distanceLeft)
//       {
//         Serial.println("🟥 Enemy on RIGHT: Preparing attack");
//         gyro.resetAngle();
//         gyro.setAngle(30);
//         state = ATTACK_RIGHT;
//       }
//       else
//       {
//         Serial.println("🟥 Enemy in FRONT: Attacking directly");
//         MotorsController::forward();
//         movementStartTime = millis();
//         state = ATTACK;
//       }
//       break;
//     }

//     MotorsController::forward();
//     if (millis() - movementStartTime >= 800)
//     {
//       MotorsController::stop();
//       gyro.resetAngle();
//       gyro.setAngle(90);
//       state = TURN_RIGHT;
//     }
//     break;

//   case AVOID_LINE_BACK:
//     MotorsController::backward();
//     if (millis() - movementStartTime >= 500)
//     {
//       MotorsController::stop();
//       gyro.resetAngle();
//       gyro.setAngle(180);
//       state = AVOID_LINE_TURN;
//     }
//     break;

// case AVOID_LINE_TURN:
//   if ((distanceLeft > 0 && distanceLeft < ATTACK_THRESHOLD) ||
//       (distanceRight > 0 && distanceRight < ATTACK_THRESHOLD)) {

//     MotorsController::stop();           // ✅ detener giro evasivo antes de girar hacia el enemigo
//     gyro.resetAngle();                  // ✅ reiniciar ángulo de referencia

//     if (distanceLeft < distanceRight) {
//       Serial.println("🟥 Enemy on LEFT during evasion: Preparing attack");
//       gyro.setAngle(-30);              // girar a la izquierda (con right())
//       state = ATTACK_LEFT;
//     } else if (distanceRight < distanceLeft) {
//       Serial.println("🟥 Enemy on RIGHT during evasion: Preparing attack");
//       gyro.setAngle(30);               // girar a la derecha (con left())
//       state = ATTACK_RIGHT;
//     } else {
//       Serial.println("🟥 Enemy in FRONT during evasion: Attacking directly");
//       MotorsController::forward();
//       movementStartTime = millis();
//       state = ATTACK;
//     }
//     break;
//   }

//   MotorsController::right();  // giro evasivo normal
//   Serial.print("[Turning Around] Angle: ");
//   Serial.println(angle);
//   if (!gyro.isTurning()) {
//     MotorsController::stop();
//     movementStartTime = millis();
//     state = MOVE_FORWARD_2;
//   }
//   break;

//   case ATTACK_LEFT:
//     if (lineDetected)
//     {
//       Serial.println("⚠️ Line detected during ATTACK_LEFT!");
//       MotorsController::stop();
//       movementStartTime = millis();
//       state = AVOID_LINE_BACK;
//       break;
//     }

//     MotorsController::right(); // ✅ corregido
//     if (!gyro.isTurning())
//     {
//       MotorsController::stop();
//       MotorsController::forward();
//       Serial.println("🚀 Attacking from LEFT");
//       movementStartTime = millis();
//       state = ATTACK;
//     }
//     break;

//   case ATTACK_RIGHT:
//     if (lineDetected)
//     {
//       Serial.println("⚠️ Line detected during ATTACK_RIGHT!");
//       MotorsController::stop();
//       movementStartTime = millis();
//       state = AVOID_LINE_BACK;
//       break;
//     }

//     MotorsController::left(); // ✅ corregido
//     if (!gyro.isTurning())
//     {
//       MotorsController::stop();
//       MotorsController::forward();
//       Serial.println("🚀 Attacking from RIGHT");
//       movementStartTime = millis();
//       state = ATTACK;
//     }
//     break;

//   case ATTACK:
//     if (lineDetected)
//     {
//       Serial.println("⚠️ Line detected during ATTACK!");
//       MotorsController::stop();
//       movementStartTime = millis();
//       state = AVOID_LINE_BACK;
//       break;
//     }

//     if (millis() - movementStartTime >= 1000)
//     {
//       MotorsController::stop();
//       gyro.resetAngle();
//       gyro.setAngle(90);
//       state = TURN_RIGHT;
//     }
//     break;
//   }
// }
