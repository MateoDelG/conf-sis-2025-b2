#ifndef GYRO_TURN_H
#define GYRO_TURN_H

#include <Arduino.h>

class GyroTurn {
  public:
    GyroTurn();
    void begin();                       // Inicializa I2C y configura el sensor
    void calibrate();                   // Calibra el offset del giroscopio
    void setAngle(float targetDegrees); // Inicia giro a X grados
    void resetAngle();                  // Reinicia ángulo acumulado
    float getCurrentAngle();            // Devuelve el ángulo actual estimado
    bool isTurning();                   // Indica si aún está girando

  private:
    void readGyroZ();                   // Lee giroscopio y actualiza ángulo

    float angleZ;
    float targetAngle;
    float gyroZ_offset;
    unsigned long lastTime;
    bool turning;

    static constexpr uint8_t MPU_ADDR = 0x68;
    static constexpr float GYRO_SENS_2000DPS = 16.384;
    static constexpr float GYRO_THRESHOLD = 0.5; // grados/segundo mínimo para considerar movimiento
};

#endif
