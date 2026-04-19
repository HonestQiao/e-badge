#ifndef BLE_CLIENT_H
#define BLE_CLIENT_H

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEClient.h>
#include <BLEUtils.h>
#include "config.h"

class BLEClientManager {
public:
  bool sendConfig(const String& deviceName, const String& configJson);
  bool sendConfigToAddress(BLEAddress address, const String& configJson, const String& dateStr = "");
  bool sendCheckInNotify(BLEAddress address, const String& time, const String& dateStr = "");
  bool sendClearConfigNotify(BLEAddress address);

  bool connected = false;

private:
  BLEClient* pClient = nullptr;
  bool doConnect(BLEAddress address);
  bool writeToCharacteristic(BLEAddress address, const String& data, const String& extra = "");
};

#endif