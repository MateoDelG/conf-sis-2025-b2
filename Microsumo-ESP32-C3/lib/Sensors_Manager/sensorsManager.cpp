#include <Wire.h>
#include <Arduino.h>
#include "Adafruit_VL6180X.h"
#include "sensorsManager.h"

namespace SensorsManager
{
#define LINE_SENSOR_PIN 0
#define XSHUT_PIN 10 // XSHUT conectado al sensor izquierdo
int line_treshold = 3500; // Valor de umbral para el sensor de línea

  Adafruit_VL6180X VLRigth;
  Adafruit_VL6180X VLLeft; // Sensor con XSHUT controlado

  void setupVL6180X()
  {
    pinMode(XSHUT_PIN, OUTPUT);
    // Apaga sensor izquierdo
    digitalWrite(XSHUT_PIN, LOW);
    delay(10);

    // Inicia sensor derecho (fijo en 0x29), luego le cambia dirección
    if (!VLRigth.begin())
    {
      Serial.println("❌ No se detecta VLRight en 0x29");
      while (1)
        ;
    }
    VLRigth.setAddress(0x30);
    Serial.println("✅ VLRight inicializado en 0x30");

    // Enciende sensor izquierdo
    digitalWrite(XSHUT_PIN, HIGH);
    delay(50);

    // Inicia sensor izquierdo (ahora único en 0x29)
    if (!VLLeft.begin())
    {
      Serial.println("❌ No se detecta VLLeft en 0x29");
      while (1)
        ;
    }
    Serial.println("✅ VLLeft inicializado en 0x29");
  }

int readDistance(int side)
{
  int range = 0;

  switch (side)
  {
  case RIGHT:
    range = VLRigth.readRange();
    if (VLRigth.readRangeStatus() != 0)
    {
      Serial.println("Error reading range");
      return -1; // Error
    }
    range -= 15; // Ajuste calibrado
    if (range < 0) range = 0; // Evitar valores negativos
    Serial.print("Distance R (cal): ");
    Serial.print(range);
    Serial.println(" mm");
    break;

  case LEFT:
    range = VLLeft.readRange();
    if (VLLeft.readRangeStatus() != 0)
    {
      Serial.println("Error reading range");
      return -1; // Error
    }
    Serial.print("Distance L: ");
    Serial.print(range);
    Serial.println(" mm");
    break;

  default:
    Serial.println("Invalid sensor side");
    return -1; // Error
  }
  return range;
}

  void setupLine() { 
    pinMode(LINE_SENSOR_PIN, INPUT);
    line_treshold = analogRead(LINE_SENSOR_PIN);
  for (int i = 0; i < 10; i++) {
    line_treshold += analogRead(LINE_SENSOR_PIN);
    Serial.println("Line sensor value: " + String(line_treshold));
    delay(100); // Espera para estabilizar la lectura
  }
    line_treshold /= 11; // Promedia las lecturas
    line_treshold *= 1.3; // Ajuste del umbral (20% más alto)
    if (line_treshold > 3900){
      line_treshold = 3900; // Limita el umbral máximo
    }
    
    
    Serial.println("Line sensor threshold set to: " + String(line_treshold));
}

  bool readLineSensor()
  {
    // const int lineThreshold = 3500;
    const int lineThreshold = line_treshold; // Usar el umbral calculado
    int lineSensorValue = analogRead(LINE_SENSOR_PIN);
    for(int i = 0; i < 10; i++) {
      lineSensorValue += analogRead(LINE_SENSOR_PIN);
      // delay(100); // Espera para estabilizar la lectura
    }
    lineSensorValue /= 11; // Promedia las lecturas
    Serial.println("Line sensor value: " + String(lineSensorValue));

    if (lineSensorValue >= lineThreshold)
    {
      return true; // Line detected
    }
    else
    {
      return false;
    }
  }

} // namespace SensorsManager