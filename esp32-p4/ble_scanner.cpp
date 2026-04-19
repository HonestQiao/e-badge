#include "ble_scanner.h"
#include "ble_client.h"
#include <time.h>

extern unsigned long getUnixTime();
extern String formatTime(unsigned long ts);
extern String getTodayDate();

static BLEScanner* gScanner = nullptr;

class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
public:
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    if (!gScanner) return;

    String name = advertisedDevice.getName().c_str();
    int rssi = advertisedDevice.getRSSI();

    // 只处理 M5B_ 或 C3B_ 开头的设备
    if (!name.startsWith(DEVICE_NAME_PREFIX) && !name.startsWith(DEVICE_NAME_PREFIX_C3)) return;

    String deviceId = gScanner->getDeviceIdFromName(name);
    BLEAddress address = advertisedDevice.getAddress();
    unsigned long now = getUnixTime();

    // 更新内存检测记录（按设备去重）
    gScanner->updateDetection(deviceId, name, address, rssi, now);

    // 打卡/检测逻辑
    gScanner->handleDetection(deviceId, name, rssi, address, now);
  }
};

void BLEScanner::init(Database* db) {
  gScanner = this;
  pDb = db;
  BLEDevice::init("ESP32-P4-Scanner");
  pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks(), false);
  pBLEScan->setActiveScan(true);
  pBLEScan->setInterval(BLE_SCAN_INTERVAL);
  pBLEScan->setWindow(BLE_SCAN_WINDOW);
}

void BLEScanner::loop() {
  // 异步处理清空配置通知（优先级高于打卡）
  if (pendingClearConfig && pClient) {
    pendingClearConfig = false;
    if (pBLEScan && pBLEScan->isScanning()) {
      pBLEScan->stop();
      delay(200);
    }
    pClient->sendClearConfigNotify(pendingClearAddress);
    return;
  }

  // 异步处理打卡通知
  if (pendingNotify && pClient) {
    pendingNotify = false;
    if (pBLEScan && pBLEScan->isScanning()) {
      pBLEScan->stop();
      delay(200);
    }
    pClient->sendCheckInNotify(pendingAddress, pendingTime, getTodayDate());
    pendingTime = "";
    return;
  }

  if (millis() - lastScanTime > 1000) {
    lastScanTime = millis();
    if (pBLEScan->isScanning() == false) {
      pBLEScan->start(1, nullptr, false);
    }
  }
}

void BLEScanner::updateDetection(const String& deviceId, const String& name, BLEAddress address, int rssi, unsigned long timestamp) {
  // 按设备去重：找到已有记录则更新，否则插入
  for (int i = 0; i < detectionCount; i++) {
    if (detections[i].deviceId == deviceId) {
      detections[i].deviceName = name;
      detections[i].address = address;
      detections[i].rssi = rssi;
      detections[i].timestamp = timestamp;
      return;
    }
  }
  // 新设备，插入到数组末尾（或覆盖最旧的）
  if (detectionCount < 100) {
    detections[detectionCount].deviceId = deviceId;
    detections[detectionCount].deviceName = name;
    detections[detectionCount].address = address;
    detections[detectionCount].rssi = rssi;
    detections[detectionCount].timestamp = timestamp;
    detectionCount++;
  } else {
    // 数组满了，覆盖第一条（最早的），然后整体前移
    for (int i = 0; i < 99; i++) {
      detections[i] = detections[i + 1];
    }
    detections[99].deviceId = deviceId;
    detections[99].deviceName = name;
    detections[99].address = address;
    detections[99].rssi = rssi;
    detections[99].timestamp = timestamp;
  }
}

void BLEScanner::handleDetection(const String& deviceId, const String& name, int rssi,
                                  BLEAddress address, unsigned long now) {
  if (!pDb) return;

  // 获取今日日期 YYYY-MM-DD
  time_t rawTime = now;
  struct tm* timeInfo = localtime(&rawTime);
  char dateBuf[12];
  sprintf(dateBuf, "%04d-%02d-%02d", timeInfo->tm_year + 1900, timeInfo->tm_mon + 1, timeInfo->tm_mday);
  String today(dateBuf);

  // 从数据库查工牌配置
  BadgeConfigDBRecord config;
  bool hasConfig = pDb->getBadgeConfig(deviceId, config);

  // ===== 新设备检测：无配置记录的设备，发送清空配置通知 =====
  if (!hasConfig && rssi >= -80) {
    // 在检测记录中查找是否已发送过
    bool alreadySent = false;
    for (int i = 0; i < detectionCount; i++) {
      if (detections[i].deviceId == deviceId && detections[i].clearConfigSent) {
        alreadySent = true;
        break;
      }
    }
    if (!alreadySent) {
      // 标记为已发送
      for (int i = 0; i < detectionCount; i++) {
        if (detections[i].deviceId == deviceId) {
          detections[i].clearConfigSent = true;
          break;
        }
      }
      Serial.print("[NewDevice] No config found, sending CLEAR_CONFIG to: ");
      Serial.println(deviceId);
      notifyClearConfig(address);
    }
  }

  // 未配置设备不打卡
  if (!hasConfig) return;

  // ===== 打卡逻辑 =====
  if (rssi >= BLE_RSSI_THRESHOLD) {
    Serial.print("[CheckIn] RSSI ok (");
    Serial.print(rssi);
    Serial.print(") for: ");
    Serial.println(deviceId);

    if (!pDb->hasCheckInToday(deviceId, today)) {
      // 今日首次打卡
      String checkinTime = formatTime(now);
      bool ok = pDb->addCheckIn(deviceId,
                      hasConfig ? config.name : "",
                      hasConfig ? config.dept : "",
                      hasConfig ? config.title : "",
                      hasConfig ? config.badgeId : "",
                      today, checkinTime, now, rssi);
      if (ok) {
        Serial.print("[CheckIn] First check-in today: ");
        Serial.println(deviceId);
        // 通知 M5Paper
        notifyCheckIn(address, checkinTime);
      } else {
        Serial.print("[CheckIn] DB insert failed for: ");
        Serial.println(deviceId);
      }
    } else {
      // 已打卡过，仅更新最近检测时间，不再连接 M5Paper 发送通知
      unsigned long lastDetected = pDb->getLastDetected(deviceId, today);
      if (now - lastDetected > 600) {  // 10分钟 = 600秒
        pDb->updateLastDetected(deviceId, today, now);
        Serial.print("[CheckIn] Last detected updated: ");
        Serial.println(deviceId);
      }
      // 当天已打卡，明确返回，不再执行 BLE 通知
      return;
    }
  }
}

void BLEScanner::notifyCheckIn(BLEAddress address, const String& time) {
  pendingNotify = true;
  pendingAddress = address;
  pendingTime = time;
}

void BLEScanner::notifyClearConfig(BLEAddress address) {
  pendingClearConfig = true;
  pendingClearAddress = address;
}

DetectionRecord* BLEScanner::getDetectionRecords() { return detections; }
int BLEScanner::getDetectionCount() { return detectionCount; }

String BLEScanner::getDeviceIdFromName(const String& name) {
  int idx = name.indexOf('_');
  if (idx >= 0 && idx + 1 < name.length()) {
    String id = name.substring(idx + 1);
    // deviceId 是 8 位十六进制，过滤尾随垃圾数据
    if (id.length() > 8) {
      id = id.substring(0, 8);
    }
    return id;
  }
  return name;
}
