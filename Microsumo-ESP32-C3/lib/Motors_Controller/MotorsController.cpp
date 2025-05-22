#include "MotorsController.h"


#define MOTOR_R1 4
#define MOTOR_R2 3
#define MOTOR_L1 2
#define MOTOR_L2 1

// Definimos un espacio de nombres para organizar las funciones del controlador de motores
namespace MotorsController {

  // Configura los pines de los motores como salidas y detiene los motores al inicio
  void setup() {
    pinMode(MOTOR_R1, OUTPUT);
    pinMode(MOTOR_R2, OUTPUT);
    pinMode(MOTOR_L1, OUTPUT);
    pinMode(MOTOR_L2, OUTPUT);
    stop(); // Asegura que los motores estén detenidos al iniciar
  }

  // Hace que el robot avance hacia adelante
  void forward() {
    digitalWrite(MOTOR_R1, HIGH); // Motor derecho hacia adelante
    digitalWrite(MOTOR_R2, LOW);
    digitalWrite(MOTOR_L1, HIGH); // Motor izquierdo hacia adelante
    digitalWrite(MOTOR_L2, LOW);
  }

  // Hace que el robot retroceda
  void backward() {
    digitalWrite(MOTOR_R1, LOW);
    digitalWrite(MOTOR_R2, HIGH); // Motor derecho hacia atrás
    digitalWrite(MOTOR_L1, LOW);
    digitalWrite(MOTOR_L2, HIGH); // Motor izquierdo hacia atrás
  }

  // Detiene ambos motores
  void stop() {
    digitalWrite(MOTOR_R1, LOW);
    digitalWrite(MOTOR_R2, LOW);
    digitalWrite(MOTOR_L1, LOW);
    digitalWrite(MOTOR_L2, LOW);
  }

  // Gira el robot hacia la derecha
  void right() {
    digitalWrite(MOTOR_R1, HIGH);  // Motor derecho hacia adelante
    digitalWrite(MOTOR_R2, LOW);
    digitalWrite(MOTOR_L1, LOW);   // Motor izquierdo hacia atrás
    digitalWrite(MOTOR_L2, HIGH);
  }

  // Gira el robot hacia la izquierda
  void left() {
    digitalWrite(MOTOR_R1, LOW);   // Motor derecho hacia atrás
    digitalWrite(MOTOR_R2, HIGH);
    digitalWrite(MOTOR_L1, HIGH);  // Motor izquierdo hacia adelante
    digitalWrite(MOTOR_L2, LOW);
  }

  // Controla el movimiento del robot según comandos recibidos por Bluetooth
  void moveBLE(String command) {
    // Definimos los comandos esperados para cada acción
    const String right_pushed = "!B813";
    const String right_released = "!B804";
    const String left_pushed = "!B714";
    const String left_released = "!B705";
    const String forward_pushed = "!B516";
    const String forward_released = "!B507";
    const String backward_pushed = "!B615";
    const String backward_released = "!B606";

    // Si se recibe el comando de girar a la derecha, ejecuta right()
    if(command == right_pushed) {
      right();
    } else if(command == right_released) {
      stop();
    }

    // Si se recibe el comando de girar a la izquierda, ejecuta left()
    if(command == left_pushed) {
      left();
    } else if(command == left_released) {
      stop();
    }

    // Si se recibe el comando de avanzar, ejecuta forward()
    if(command == forward_pushed) {
      forward();
    } else if(command == forward_released) {
      stop();
    }

    // Si se recibe el comando de retroceder, ejecuta backward()
    if(command == backward_pushed) {
      backward();
    } else if(command == backward_released) {
      stop();
    }

  }

} // namespace MotorsController