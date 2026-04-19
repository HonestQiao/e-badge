#include "ble_peripheral.h"

BLEPeripheral* gBLEPeripheral = nullptr;

class ConfigCallbacks : public BLECharacteristicCallbacks {
public:
  void onWrite(BLECharacteristic* pCharacteristic) {
    String value = pCharacteristic->getValue();
    if (value.length() > 0 && gBLEPeripheral) {
      gBLEPeripheral->configJson = value;
      gBLEPeripheral->configReceived = true;
      Serial.print("[BLE] Config received: ");
      Serial.println(gBLEPeripheral->configJson);
    }
  }
};

class ServerCallbacks : public BLEServerCallbacks {
public:
  void onConnect(BLEServer* pServer) {
    Serial.println("[BLE] Central connected");
  }
  void onDisconnect(BLEServer* pServer) {
    Serial.println("[BLE] Central disconnected");
    BLEDevice::startAdvertising();
  }
};

void BLEPeripheral::init(const String& deviceId) {
  gBLEPeripheral = this;
  deviceName = String(DEVICE_NAME_PREFIX) + deviceId;

  BLEDevice::init(deviceName.c_str());
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new ServerCallbacks());

  BLEService* pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE
  );
  pCharacteristic->setCallbacks(new ConfigCallbacks());
  pCharacteristic->addDescriptor(new BLE2902());
  pService->start();

  BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();

  Serial.print("[BLE] Peripheral started, name: ");
  Serial.println(deviceName);
}

void BLEPeripheral::loop() {
  if (millis() - lastAdvertiseCheck > 5000) {
    lastAdvertiseCheck = millis();
    // 确保广播持续进行
  }
}

bool BLEPeripheral::isConfigReceived() {
  return configReceived;
}

String BLEPeripheral::getConfigJson() {
  return configJson;
}

void BLEPeripheral::clearConfigFlag() {
  configReceived = false;
  configJson = "";
}
