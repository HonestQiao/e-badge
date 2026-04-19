#include "ble_client.h"

static BLEClientManager* gClientMgr = nullptr;

class ClientCallbacks : public BLEClientCallbacks {
public:
  void onConnect(BLEClient* pClient) {
    Serial.println("[BLE Client] Connected");
    if (gClientMgr) gClientMgr->connected = true;
  }
  void onDisconnect(BLEClient* pClient) {
    Serial.println("[BLE Client] Disconnected");
    if (gClientMgr) gClientMgr->connected = false;
  }
};

bool BLEClientManager::doConnect(BLEAddress address) {
  // BLE扫描和连接不能同时进行，先停止扫描
  BLEScan* pScan = BLEDevice::getScan();
  if (pScan && pScan->isScanning()) {
    pScan->stop();
    delay(300);
  }

  if (!pClient) {
    pClient = BLEDevice::createClient();
    pClient->setClientCallbacks(new ClientCallbacks());
  }

  for (int retry = 0; retry < 3; retry++) {
    if (retry > 0) {
      Serial.print("[BLE Client] Retry ");
      Serial.println(retry + 1);
      delay(500);
    }

    connected = false;
    Serial.print("[BLE Client] Connecting to ");
    Serial.println(address.toString().c_str());

    if (!pClient->connect(address, BLE_ADDR_PUBLIC)) {
      Serial.println("[BLE Client] Connect call failed, trying RANDOM...");
      if (!pClient->connect(address, BLE_ADDR_RANDOM)) {
        Serial.println("[BLE Client] RANDOM also failed");
        continue;
      }
    }

    Serial.println("[BLE Client] Connect call returned true, waiting for callback...");

    unsigned long start = millis();
    while (!connected && millis() - start < 10000) {
      delay(100);
    }

    if (connected) {
      Serial.println("[BLE Client] Connected successfully");
      return true;
    }

    Serial.println("[BLE Client] Connect timeout");
    pClient->disconnect();
  }

  Serial.println("[BLE Client] All retries exhausted");
  return false;
}

bool BLEClientManager::writeToCharacteristic(BLEAddress address, const String& data, const String& extra) {
  gClientMgr = this;
  connected = false;

  if (!doConnect(address)) {
    return false;
  }

  BLERemoteService* pService = pClient->getService(SERVICE_UUID);
  if (!pService) {
    Serial.println("[BLE Client] Service not found");
    pClient->disconnect();
    return false;
  }

  BLERemoteCharacteristic* pChar = pService->getCharacteristic(CHARACTERISTIC_UUID);
  if (!pChar) {
    Serial.println("[BLE Client] Characteristic not found");
    pClient->disconnect();
    return false;
  }

  pChar->writeValue(data.c_str(), data.length());
  Serial.println("[BLE Client] Data written");

  if (!extra.isEmpty()) {
    delay(200);
    pChar->writeValue(extra.c_str(), extra.length());
    Serial.print("[BLE Client] Extra data written: ");
    Serial.println(extra);
  }

  delay(500);
  pClient->disconnect();
  connected = false;

  return true;
}

bool BLEClientManager::sendConfigToAddress(BLEAddress address, const String& configJson, const String& dateStr) {
  String extra;
  if (!dateStr.isEmpty()) {
    extra = "TIME:" + dateStr;
  }
  return writeToCharacteristic(address, configJson, extra);
}

bool BLEClientManager::sendCheckInNotify(BLEAddress address, const String& time, const String& dateStr) {
  String data = "CHECKIN:" + time;
  String extra;
  if (!dateStr.isEmpty()) {
    extra = "TIME:" + dateStr;
  }
  Serial.print("[BLE Client] Sending checkin notify: ");
  Serial.println(data);
  return writeToCharacteristic(address, data, extra);
}

bool BLEClientManager::sendClearConfigNotify(BLEAddress address) {
  Serial.println("[BLE Client] Sending CLEAR_CONFIG");
  return writeToCharacteristic(address, "CLEAR_CONFIG", "");
}

bool BLEClientManager::sendConfig(const String& deviceName, const String& configJson) {
  return false;
}
