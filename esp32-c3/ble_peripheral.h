#ifndef BLE_PERIPHERAL_H
#define BLE_PERIPHERAL_H

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include "config.h"

class BLEPeripheral {
public:
  void init(const String& deviceId);
  void loop();
  bool isConfigReceived();
  String getConfigJson();
  void clearConfigFlag();

  // 供BLE回调使用
  bool configReceived = false;
  String configJson = "";

private:
  BLEServer* pServer = nullptr;
  BLECharacteristic* pCharacteristic = nullptr;
  String deviceName;
  unsigned long lastAdvertiseCheck = 0;
};

// 全局实例供回调使用
extern BLEPeripheral* gBLEPeripheral;

#endif
