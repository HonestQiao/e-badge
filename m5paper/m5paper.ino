#include <M5Unified.h>
#include <ArduinoJson.h>
#include "config.h"
#include "storage.h"
#include "display.h"
#include "ble_peripheral.h"

Storage storage;
Display display;
BLEPeripheral blePeripheral;

String deviceId;

String generateDeviceId() {
  uint64_t mac = ESP.getEfuseMac();
  char buf[13];
  sprintf(buf, "%02X%02X%02X%02X",
          (uint8_t)(mac >> 24), (uint8_t)(mac >> 16),
          (uint8_t)(mac >> 8),  (uint8_t)mac);
  return String(buf);
}

void parseConfig(const String& json) {
  // 处理清空配置通知
  if (json == "CLEAR_CONFIG") {
    Serial.println("[Config] Received CLEAR_CONFIG");
    storage.clear();
    storage.clearCheckInTime();
    storage.clearCurrentDate();
    display.clearCheckInStatus();
    // 延迟2秒后全屏刷新，避免BLE写入刚结束时瞬时电流叠加导致关机
    delay(2000);
    display.showWaitingConfig(deviceId);
    Serial.println("[Config] Cleared, waiting for new config");
    return;
  }

  // 处理打卡通知: CHECKIN:HH:MM:SS
  if (json.startsWith("CHECKIN:")) {
    String time = json.substring(8);
    Serial.print("[CheckIn] Received: ");
    Serial.println(time);
    display.showCheckInStatus(time);
    storage.saveCheckInTime(time);
    return;
  }

  // 处理时间同步: TIME:YYYY-MM-DD
  if (json.startsWith("TIME:")) {
    String date = json.substring(5);
    Serial.print("[Time] Sync received: ");
    Serial.println(date);
    String savedDate = storage.loadCurrentDate();
    if (!savedDate.isEmpty() && savedDate != date) {
      // 跨天了，清除打卡状态
      Serial.println("[Time] Date changed, clearing check-in status");
      storage.clearCheckInTime();
      display.clearCheckInStatus();
      BadgeInfo info = storage.load();
      if (info.configured) {
        display.showBadge(info, true);
        display.showPromptCheckIn();
      }
    }
    storage.saveCurrentDate(date);
    return;
  }

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, json);
  if (error) {
    Serial.print("[Config] JSON parse failed: ");
    Serial.println(error.c_str());
    return;
  }

  BadgeInfo info;
  info.name = doc["name"] | "";
  info.dept = doc["dept"] | "";
  info.title = doc["title"] | "";
  info.badgeId = doc["id"] | "";
  info.configured = true;

  if (info.name.isEmpty()) {
    Serial.println("[Config] Name is empty, ignoring");
    return;
  }

  // 保存到Preferences
  storage.save(info);
  Serial.println("[Config] Saved to storage");

  // 刷新屏幕
  display.showBadge(info, true);
  Serial.println("[Config] Display updated");

  // 新配置下发后，如果没有当天打卡记录，提示打卡
  String savedDate = storage.loadCurrentDate();
  String savedTime = storage.loadCheckInTime();
  if (savedDate.isEmpty() || savedTime.isEmpty()) {
    display.showPromptCheckIn();
  }

  // 配置完成后自动重启，让P4重新检测并自动打卡
  Serial.println("[Config] Auto-reboot in 2s...");
  delay(2000);
  ESP.restart();
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n================================");
  Serial.println("  M5Paper 电子工牌客户端");
  Serial.println("================================");

  // 初始化显示
  display.init();
  Serial.println("[Display] Initialized");

  // 初始化存储
  storage.init();

  // 生成设备ID
  deviceId = generateDeviceId();
  Serial.print("[Device] ID: ");
  Serial.println(deviceId);

  // 加载已保存的工牌信息
  BadgeInfo info = storage.load();

  if (info.configured) {
    Serial.println("[Storage] Badge info loaded");
    display.showBadge(info);
    // 恢复打卡状态（如果有日期和打卡时间）
    String savedDate = storage.loadCurrentDate();
    String savedCheckIn = storage.loadCheckInTime();
    if (!savedDate.isEmpty() && !savedCheckIn.isEmpty()) {
      Serial.print("[Storage] Check-in time restored: ");
      Serial.println(savedCheckIn);
      display.showCheckInStatus(savedCheckIn);
    } else {
      // 已配置但未打卡（或日期未同步）
      display.showPromptCheckIn();
    }
  } else {
    Serial.println("[Storage] No config found, showing waiting screen");
    display.showWaitingConfig(deviceId);
  }

  // 初始化BLE外围设备
  blePeripheral.init(deviceId);
  Serial.println("[BLE] Peripheral ready");
}

void loop() {
  blePeripheral.loop();

  // 检查是否收到新配置
  if (blePeripheral.isConfigReceived()) {
    String json = blePeripheral.getConfigJson();
    Serial.print("[Main] New config: ");
    Serial.println(json);

    parseConfig(json);
    blePeripheral.clearConfigFlag();
  }

  delay(100);
}
