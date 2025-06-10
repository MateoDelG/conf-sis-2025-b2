#include <Arduino.h>
#ifndef SENSORSMANAGER_H
#define SENSORSMANAGER_H

namespace SensorsManager {
void setupLine();
bool readLineSensor();
void setupVL6180X();
int readDistance( int side);
enum SensorSide {
    RIGHT,
    LEFT
};
}

#endif