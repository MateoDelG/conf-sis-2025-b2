#include <Arduino.h>

#define LED_BUILTIN 8

// the setup function runs once when you press reset or power the board
void setup() {
  delay(2000);
  Serial.begin(115200);
  while (!Serial);
  Serial.println("Hello World!");
  pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
  digitalWrite(LED_BUILTIN, LOW);
  delay(50);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(100);
  Serial.println("Hello World!");
}
