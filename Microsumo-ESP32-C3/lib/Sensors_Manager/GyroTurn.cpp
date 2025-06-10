#include "GyroTurn.h"
#include <Wire.h>

GyroTurn::GyroTurn() : angleZ(0.0), targetAngle(0.0), gyroZ_offset(0.0), lastTime(0), turning(false) {}

void GyroTurn::begin() {
  Wire.begin(8, 9); // Pines SDA/SCL para ESP32-C3
  delay(100);

  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B); // PWR_MGMT_1
  Wire.write(0x00); // Wake up
  Wire.endTransmission();

  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x1B); // GYRO_CONFIG
  Wire.write(0x18); // ±2000 dps
  Wire.endTransmission();

  lastTime = millis();
}

void GyroTurn::calibrate() {
  float sum = 0;
  const int samples = 100;

  for (int i = 0; i < samples; i++) {
    uint8_t data[6];
    Wire.beginTransmission(MPU_ADDR);
    Wire.write(0x43);
    Wire.endTransmission(false);
    Wire.requestFrom(MPU_ADDR, (uint8_t)6);
    for (int j = 0; j < 6 && Wire.available(); j++) {
      data[j] = Wire.read();
    }
    int16_t gz_raw = (int16_t)(data[4] << 8 | data[5]);
    float gz_dps = gz_raw / GYRO_SENS_2000DPS;
    sum += gz_dps;
    delay(5);
  }

  gyroZ_offset = sum / samples;
}

void GyroTurn::setAngle(float targetDegrees) {
  resetAngle();
  targetAngle = targetDegrees;
  turning = true;
}

void GyroTurn::resetAngle() {
  angleZ = 0.0;
  lastTime = millis();
  turning = false;
}

bool GyroTurn::isTurning() {
  return turning;
}

float GyroTurn::getCurrentAngle() {
  readGyroZ();
  return angleZ;
}

void GyroTurn::readGyroZ() {
  if (!turning) return;

  uint8_t data[6];
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x43);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_ADDR, (uint8_t)6);
  for (int i = 0; i < 6 && Wire.available(); i++) {
    data[i] = Wire.read();
  }

  int16_t gz_raw = (int16_t)(data[4] << 8 | data[5]);
  float gz_dps = (gz_raw / GYRO_SENS_2000DPS) - gyroZ_offset;

  if (fabs(gz_dps) < GYRO_THRESHOLD) gz_dps = 0.0;

  unsigned long now = millis();
  float dt = (now - lastTime) / 1000.0;
  lastTime = now;

  angleZ += gz_dps * dt;

  if (fabs(angleZ) >= fabs(targetAngle)) {
    turning = false;
  }
}
