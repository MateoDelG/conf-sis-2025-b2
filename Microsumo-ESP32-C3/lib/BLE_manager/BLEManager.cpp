#include "BLEManager.h"
#include  "MotorsController.h"

// Definición de los UUIDs (Identificadores Universales Únicos) para el servicio y las características BLE.
// Estos identificadores permiten que la app móvil se comunique correctamente con el ESP32-C3.
#define SERVICE_UUID "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

// Definición del pin del LED integrado en la placa ESP32-C3.
#define LED_BUILTIN 8

// Todo el código relacionado con BLE se encuentra dentro del namespace BLEManager.
namespace BLEManager {

// Punteros a objetos BLE para manejar el servidor y la característica de transmisión (TX).
BLEServer *pServer = nullptr;
BLECharacteristic *pTxCharacteristic;

// Variables para saber si hay un dispositivo conectado y si el ESP32 está anunciando su presencia (advertising).
static bool deviceConnected = false;
static bool isAdvertising = false;

// Clase para manejar los eventos de conexión y desconexión BLE.
class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *pServer) override {
    deviceConnected = true;
    Serial.println("Conectado a Bluefruit"); // Mensaje cuando un dispositivo se conecta.
  }

  void onDisconnect(BLEServer *pServer) override {
    deviceConnected = false;
    Serial.println("Desconectado de Bluefruit"); // Mensaje cuando un dispositivo se desconecta.

    // Reinicia el advertising para que otros dispositivos puedan encontrar el ESP32.
    BLEDevice::getAdvertising()->start();
    Serial.println("Reiniciando advertising BLE...");
  }
};

// Clase para manejar los eventos de escritura en la característica RX (recepción de datos).
class MyCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    // Obtiene el valor recibido desde la app móvil.
    std::string rxValue = pCharacteristic->getValue();

    if (rxValue.length() > 0) {
      Serial.print("Mensaje recibido: ");
      for (int i = 0; i < rxValue.length(); i++) {
        Serial.print(rxValue[i]);
      }
      Serial.println();

      // Convierte el mensaje recibido a un String de Arduino y elimina espacios y saltos de línea.
      std::string rxValue = pCharacteristic->getValue();
      String command = String(rxValue.c_str());
      command.trim(); // elimina \n, \r y espacios

      // Acciones personalizadas según el comando recibido.
      if (command == "LEDON") {
        digitalWrite(LED_BUILTIN, LOW); // Enciende el LED integrado.
        pTxCharacteristic->setValue("LED encendido"); // Envía confirmación a la app.
        pTxCharacteristic->notify();
      } else if (command == "LEDOFF") {
        digitalWrite(LED_BUILTIN, HIGH); // Apaga el LED integrado.
        pTxCharacteristic->setValue("LED apagado"); // Envía confirmación a la app.
        pTxCharacteristic->notify();
      }

      // Llama a la función para mover los motores según el comando recibido por BLE.
      MotorsController::moveBLE(command);
    }
  }
};

// Función para inicializar el BLE y configurar el servicio y las características.
void setupBLE() {
  BLEDevice::init("ESP32-C3_UART"); // Inicializa el BLE con el nombre del dispositivo.
  pServer = BLEDevice::createServer(); // Crea el servidor BLE.
  pServer->setCallbacks(new MyServerCallbacks()); // Asigna los callbacks de conexión/desconexión.

  BLEService *pService = pServer->createService(SERVICE_UUID); // Crea el servicio BLE.

  // Crea la característica de transmisión (TX) para enviar datos a la app.
  pTxCharacteristic = pService->createCharacteristic(
      CHARACTERISTIC_UUID_TX, BLECharacteristic::PROPERTY_NOTIFY);
  pTxCharacteristic->addDescriptor(new BLE2902());

  // Crea la característica de recepción (RX) para recibir datos de la app.
  BLECharacteristic *pRxCharacteristic = pService->createCharacteristic(
      CHARACTERISTIC_UUID_RX, BLECharacteristic::PROPERTY_WRITE);
  pRxCharacteristic->setCallbacks(new MyCallbacks()); // Asigna el callback de escritura.

  pService->start(); // Inicia el servicio BLE.
  isAdvertising = true;

  // Inicia el advertising para que la app pueda encontrar el ESP32.
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->start();

  Serial.println("Esperando conexión desde Bluefruit Connect...");
}

// Función para asegurar que el ESP32 siga anunciando su presencia si no hay dispositivos conectados.
void ensureAdvertising() {
  if (!deviceConnected && !isAdvertising) {
    Serial.println("Reiniciando advertising BLE...");
    BLEDevice::getAdvertising()->start();
    isAdvertising = true;
  }
}

} // namespace BLEManager
