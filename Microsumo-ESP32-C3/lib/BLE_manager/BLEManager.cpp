#include "BLEManager.h"
#include  "MotorsController.h"

#define SERVICE_UUID "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"
#define LED_BUILTIN 8
namespace BLEManager {

BLEServer *pServer = nullptr;
BLECharacteristic *pTxCharacteristic;
static bool deviceConnected = false;
static bool isAdvertising = false;


class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *pServer) override {
    deviceConnected = true;
    Serial.println("Conectado a Bluefruit");
  }

  void onDisconnect(BLEServer *pServer) override {
    deviceConnected = false;
    Serial.println("Desconectado de Bluefruit");

    // 🔁 Reiniciar advertising
    BLEDevice::getAdvertising()->start();
    Serial.println("Reiniciando advertising BLE...");
  }
};

class MyCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    std::string rxValue = pCharacteristic->getValue();

    if (rxValue.length() > 0) {
      Serial.print("Mensaje recibido: ");
      for (int i = 0; i < rxValue.length(); i++) {
        Serial.print(rxValue[i]);
      }
      Serial.println();

      std::string rxValue = pCharacteristic->getValue();
      String command = String(rxValue.c_str());
      command.trim(); // elimina \n, \r y espacios

      // Aquí puedes agregar acciones personalizadas
      if (command == "LEDON") {
        digitalWrite(LED_BUILTIN, LOW);
        pTxCharacteristic->setValue("LED encendido");
        pTxCharacteristic->notify();
      } else if (command == "LEDOFF") {
        digitalWrite(LED_BUILTIN, HIGH);
        pTxCharacteristic->setValue("LED apagado");
        pTxCharacteristic->notify();
      }

      MotorsController::moveBLE(command);

      
    }
  }
};

void setupBLE() {

  BLEDevice::init("ESP32-C3_UART");
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);

  pTxCharacteristic = pService->createCharacteristic(
      CHARACTERISTIC_UUID_TX, BLECharacteristic::PROPERTY_NOTIFY);
  pTxCharacteristic->addDescriptor(new BLE2902());

  BLECharacteristic *pRxCharacteristic = pService->createCharacteristic(
      CHARACTERISTIC_UUID_RX, BLECharacteristic::PROPERTY_WRITE);
  pRxCharacteristic->setCallbacks(new MyCallbacks());

  pService->start();
  isAdvertising = true;


  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->start();

  Serial.println("Esperando conexión desde Bluefruit Connect...");
}

void ensureAdvertising() {
  if (!deviceConnected && !isAdvertising) {
    Serial.println("Reiniciando advertising BLE...");
    BLEDevice::getAdvertising()->start();
    isAdvertising = true;
  }
}


} // namespace BLEManager
