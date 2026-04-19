#ifndef BLE_SCANNER_H
#define BLE_SCANNER_H

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include "config.h"
#include "database.h"

// 内存中的检测记录（按设备去重，保留最新）
struct DetectionRecord {
  String deviceId;
  String deviceName;
  BLEAddress address;
  int rssi;
  unsigned long timestamp;  // UNIX秒
  bool clearConfigSent = false;  // 是否已发送过清空配置通知
};

class BLEClientManager;

class BLEScanner {
public:
  void init(Database* db);
  void loop();
  void setClient(BLEClientManager* client) { pClient = client; }

  // 检测记录（内存数组，按设备去重）
  DetectionRecord* getDetectionRecords();
  int getDetectionCount();

  // 供BLE回调使用
  String getDeviceIdFromName(const String& name);
  void updateDetection(const String& deviceId, const String& name, BLEAddress address, int rssi, unsigned long timestamp);
  void handleDetection(const String& deviceId, const String& name, int rssi, BLEAddress address, unsigned long now);
  void notifyCheckIn(BLEAddress address, const String& time);
  void notifyClearConfig(BLEAddress address);

private:
  BLEScan* pBLEScan;
  BLEClientManager* pClient = nullptr;
  Database* pDb = nullptr;

  // 内存检测记录（按设备去重，最多100个不同设备）
  DetectionRecord detections[100];
  int detectionCount = 0;

  // 异步打卡通知
  bool pendingNotify = false;
  String pendingTime;
  BLEAddress pendingAddress;

  // 异步清空配置通知
  bool pendingClearConfig = false;
  BLEAddress pendingClearAddress;

  unsigned long lastScanTime = 0;
};

#endif
