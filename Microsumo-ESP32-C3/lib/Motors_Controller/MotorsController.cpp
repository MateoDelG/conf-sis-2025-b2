#include "MotorsController.h"


#define MOTOR_R1 4
#define MOTOR_R2 3
#define MOTOR_L1 2
#define MOTOR_L2 1

namespace MotorsController {

  void setup() {
    pinMode(MOTOR_R1, OUTPUT);
    pinMode(MOTOR_R2, OUTPUT);
    pinMode(MOTOR_L1, OUTPUT);
    pinMode(MOTOR_L2, OUTPUT);
    stop();
  }

  void forward() {
    digitalWrite(MOTOR_R1, HIGH);
    digitalWrite(MOTOR_R2, LOW);
    digitalWrite(MOTOR_L1, HIGH);
    digitalWrite(MOTOR_L2, LOW);
  }

  void backward() {
    digitalWrite(MOTOR_R1, LOW);
    digitalWrite(MOTOR_R2, HIGH);
    digitalWrite(MOTOR_L1, LOW);
    digitalWrite(MOTOR_L2, HIGH);
  }

  void stop() {
    digitalWrite(MOTOR_R1, LOW);
    digitalWrite(MOTOR_R2, LOW);
    digitalWrite(MOTOR_L1, LOW);
    digitalWrite(MOTOR_L2, LOW);
  }

  void right() {
    digitalWrite(MOTOR_R1, HIGH);
    digitalWrite(MOTOR_R2, LOW);
    digitalWrite(MOTOR_L1, LOW);
    digitalWrite(MOTOR_L2, HIGH);
  }

  void left() {
    digitalWrite(MOTOR_R1, LOW);
    digitalWrite(MOTOR_R2, HIGH);
    digitalWrite(MOTOR_L1, HIGH);
    digitalWrite(MOTOR_L2, LOW);
  }

  void moveBLE(String command) {
    const String right_pushed = "!B813";
    const String right_released = "!B804";
    const String left_pushed = "!B714";
    const String left_released = "!B705";
    const String forward_pushed = "!B516";
    const String forward_released = "!B507";
    const String backward_pushed = "!B615";
    const String backward_released = "!B606";

    if(command == right_pushed) {
      right();
    } else if(command == right_released) {
      stop();
    }

    if(command == left_pushed) {
      left();
    } else if(command == left_released) {
      stop();
    }
    if(command == forward_pushed) {
      forward();
    } else if(command == forward_released) {
      stop();
    }
    if(command == backward_pushed) {
      backward();
    } else if(command == backward_released) {
      stop();
    }

  }


} // nam