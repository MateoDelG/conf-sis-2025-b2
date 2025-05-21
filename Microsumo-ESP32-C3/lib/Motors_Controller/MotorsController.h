#include <Arduino.h>
#ifndef MOTORS_CONTROLLER_H
#define MOTORS_CONTROLLER_H

namespace MotorsController {
  void setup();

  void forward();
  void backward();
  void stop();
  void left();
  void right();
  void moveBLE(String command);
}

#endif