// 分区方案: 8MB with spiffs (Arduino IDE → 工具 → Partition Scheme)
#include <WiFi.h>
#include <WiFiMulti.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <WebServer.h>
#include <BLEDevice.h>
#include <ArduinoJson.h>
#include <time.h>
#include "config.h"
#include "index_html.h"
#include "database.h"
#include "ble_scanner.h"
#include "ble_client.h"

WebServer server(WEB_PORT);
Database db;
BLEScanner bleScanner;
BLEClientManager bleClient;
WiFiMulti wifiMulti;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, NTP_SERVER, NTP_TIME_OFFSET, NTP_UPDATE_INTERVAL);

// 获取当前 UNIX 时间戳（秒）
unsigned long getUnixTime() {
  timeClient.update();
  return timeClient.getEpochTime();
}

// 将时间戳格式化为 HH:MM:SS
String formatTime(unsigned long ts) {
  unsigned long daySec = ts % 86400;
  int h = daySec / 3600;
  int m = (daySec % 3600) / 60;
  int s = daySec % 60;
  char buf[10];
  sprintf(buf, "%02d:%02d:%02d", h, m, s);
  return String(buf);
}

// 获取今日日期 YYYY-MM-DD
String getTodayDate() {
  time_t rawTime = getUnixTime();
  struct tm* timeInfo = localtime(&rawTime);
  char buf[12];
  sprintf(buf, "%04d-%02d-%02d", timeInfo->tm_year + 1900, timeInfo->tm_mon + 1, timeInfo->tm_mday);
  return String(buf);
}

void setupWiFi() {
  Serial.println("[WiFi] Connecting to WiFi...");
  wifiMulti.addAP(WIFI_SSID, WIFI_PASSWORD);

  while (wifiMulti.run() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\n[WiFi] Connected!");
  Serial.print("[WiFi] IP: ");
  Serial.println(WiFi.localIP());
}

void setupNTP() {
  timeClient.begin();
  Serial.println("[NTP] Starting sync...");

  int retries = 0;
  while (!timeClient.update() && retries < 20) {
    delay(500);
    retries++;
    Serial.print(".");
  }

  if (retries < 20) {
    Serial.println("\n[NTP] Time synced: " + timeClient.getFormattedTime());
  } else {
    Serial.println("\n[NTP] Sync failed, will retry in background");
  }
}

// 手动JSON转义辅助函数
String jsonEscape(const String& s) {
  String out;
  for (int i = 0; i < s.length(); i++) {
    char c = s[i];
    if (c == '"') out += "\\\"";
    else if (c == '\\') out += "\\\\";
    else if (c == '\b') out += "\\b";
    else if (c == '\f') out += "\\f";
    else if (c == '\n') out += "\\n";
    else if (c == '\r') out += "\\r";
    else if (c == '\t') out += "\\t";
    else out += c;
  }
  return out;
}

void setupWebServer() {
  server.on("/", HTTP_GET, []() {
    server.send(200, "text/html", INDEX_HTML);
  });

  // 打卡记录（从SQLite查询）
  server.on("/api/checkin_list", HTTP_GET, []() {
    CheckInDBRecord records[50];
    int count = db.getCheckInRecords(records, 50);

    String response = "[";
    for (int i = 0; i < count; i++) {
      if (i > 0) response += ",";
      response += "{";
      response += "\"name\":\"" + jsonEscape(records[i].name.isEmpty() ? "未知" : records[i].name) + "\",";
      response += "\"dept\":\"" + jsonEscape(records[i].dept.isEmpty() ? "-" : records[i].dept) + "\",";
      response += "\"title\":\"" + jsonEscape(records[i].title.isEmpty() ? "-" : records[i].title) + "\",";
      response += "\"id\":\"" + jsonEscape(records[i].badgeId.isEmpty() ? "-" : records[i].badgeId) + "\",";
      response += "\"device_id\":\"" + jsonEscape(records[i].deviceId) + "\",";
      response += "\"date\":\"" + records[i].date + "\",";
      response += "\"time\":\"" + records[i].checkinTime + "\",";
      response += "\"rssi\":" + String(records[i].checkinRssi);
      response += "}";
    }
    response += "]";
    server.send(200, "application/json", response);
  });

  // 检测记录（从内存数组返回）
  server.on("/api/detection_list", HTTP_GET, []() {
    DetectionRecord* records = bleScanner.getDetectionRecords();
    int count = bleScanner.getDetectionCount();

    String response = "[";
    for (int i = count - 1; i >= 0; i--) {  // 倒序，最新的在前
      if (i < count - 1) response += ",";
      response += "{";
      response += "\"id\":\"" + jsonEscape(records[i].deviceId) + "\",";
      response += "\"name\":\"" + jsonEscape(records[i].deviceName) + "\",";
      response += "\"time\":\"" + formatTime(records[i].timestamp) + "\",";
      response += "\"rssi\":" + String(records[i].rssi);
      response += "}";
    }
    response += "]";
    server.send(200, "application/json", response);
  });

  // 今日打卡统计
  server.on("/api/stats", HTTP_GET, []() {
    String today = getTodayDate();
    int todayCount = db.getTodayCheckInCount(today);
    int totalDevices = bleScanner.getDetectionCount();

    String response = "{";
    response += "\"today_checkin\":" + String(todayCount) + ",";
    response += "\"online_devices\":" + String(totalDevices) + ",";
    response += "\"today_date\":\"" + today + "\"";
    response += "}";
    server.send(200, "application/json", response);
  });

  // 设备列表（从内存检测数组）
  server.on("/api/badge_list", HTTP_GET, []() {
    DetectionRecord* records = bleScanner.getDetectionRecords();
    int count = bleScanner.getDetectionCount();

    String response = "[";
    bool first = true;
    for (int i = 0; i < count; i++) {
      unsigned long now = getUnixTime();
      if (now > records[i].timestamp && now - records[i].timestamp > 1800) continue;  // 30分钟未检测到视为离线
      if (!first) response += ",";
      first = false;
      response += "{";
      response += "\"id\":\"" + jsonEscape(records[i].deviceId) + "\",";
      response += "\"name\":\"" + jsonEscape(records[i].deviceName) + "\",";
      response += "\"rssi\":" + String(records[i].rssi);
      response += "}";
    }
    response += "]";
    server.send(200, "application/json", response);
  });

  // 工牌配置
  server.on("/api/config", HTTP_POST, []() {
    String body = server.arg("plain");
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, body);

    if (error) {
      server.send(400, "application/json", "{\"success\":false,\"error\":\"JSON解析失败\"}");
      return;
    }

    String deviceId = doc["deviceId"] | "";
    String name = doc["name"] | "";
    String dept = doc["dept"] | "";
    String title = doc["title"] | "";
    String badgeId = doc["id"] | "";

    if (deviceId.isEmpty()) {
      server.send(400, "application/json", "{\"success\":false,\"error\":\"未选择设备\"}");
      return;
    }

    // 保存到SQLite
    db.saveBadgeConfig(deviceId, name, dept, title, badgeId, getUnixTime());
    Serial.println("[Config] Saved to DB: " + deviceId);

    // 通过BLE发送配置到M5Paper
    String configStr = "{";
    configStr += "\"name\":\"" + jsonEscape(name) + "\",";
    configStr += "\"dept\":\"" + jsonEscape(dept) + "\",";
    configStr += "\"title\":\"" + jsonEscape(title) + "\",";
    configStr += "\"id\":\"" + jsonEscape(badgeId) + "\",";
    configStr += "\"timestamp\":" + String(getUnixTime());
    configStr += "}";

    // 从内存检测数组查找设备地址
    BLEAddress targetAddress;
    bool found = false;
    DetectionRecord* records = bleScanner.getDetectionRecords();
    int count = bleScanner.getDetectionCount();
    for (int i = 0; i < count; i++) {
      if (records[i].deviceId == deviceId) {
        targetAddress = records[i].address;
        found = true;
        break;
      }
    }

    if (!found) {
      server.send(400, "application/json", "{\"success\":false,\"error\":\"设备未在蓝牙范围内，请稍后再试\"}");
      return;
    }

    Serial.print("[Config] Sending to device: ");
    Serial.println(deviceId);

    String today = getTodayDate();
    bool sent = bleClient.sendConfigToAddress(targetAddress, configStr, today);
    server.send(200, "application/json", sent
      ? "{\"success\":true,\"note\":\"配置已下发到设备\"}"
      : "{\"success\":true,\"note\":\"配置已保存，但设备下发失败\"}");
  });

  // 单个工牌配置查询
  server.on("/api/badge_config", HTTP_GET, []() {
    String deviceId = server.arg("deviceId");
    if (deviceId.isEmpty()) {
      server.send(400, "application/json", "{\"success\":false,\"error\":\"缺少deviceId参数\"}");
      return;
    }

    BadgeConfigDBRecord config;
    bool found = db.getBadgeConfig(deviceId, config);

    if (!found) {
      server.send(200, "application/json", "{\"found\":false}");
      return;
    }

    String response = "{";
    response += "\"found\":true,";
    response += "\"device_id\":\"" + jsonEscape(config.deviceId) + "\",";
    response += "\"name\":\"" + jsonEscape(config.name) + "\",";
    response += "\"dept\":\"" + jsonEscape(config.dept) + "\",";
    response += "\"title\":\"" + jsonEscape(config.title) + "\",";
    response += "\"badge_id\":\"" + jsonEscape(config.badgeId) + "\",";
    response += "\"updated_at\":" + String(config.updatedAt);
    response += "}";
    server.send(200, "application/json", response);
  });

  // 工牌信息列表（从SQLite查询所有配置）
  server.on("/api/badge_configs", HTTP_GET, []() {
    BadgeConfigDBRecord records[50];
    int count = db.getAllBadgeConfigs(records, 50);

    String response = "[";
    for (int i = 0; i < count; i++) {
      if (i > 0) response += ",";
      response += "{";
      response += "\"device_id\":\"" + jsonEscape(records[i].deviceId) + "\",";
      response += "\"name\":\"" + jsonEscape(records[i].name.isEmpty() ? "未命名" : records[i].name) + "\",";
      response += "\"dept\":\"" + jsonEscape(records[i].dept.isEmpty() ? "-" : records[i].dept) + "\",";
      response += "\"title\":\"" + jsonEscape(records[i].title.isEmpty() ? "-" : records[i].title) + "\",";
      response += "\"badge_id\":\"" + jsonEscape(records[i].badgeId.isEmpty() ? "-" : records[i].badgeId) + "\",";
      response += "\"updated_at\":" + String(records[i].updatedAt);
      response += "}";
    }
    response += "]";
    server.send(200, "application/json", response);
  });

  // 删除工牌配置
  server.on("/api/delete_config", HTTP_POST, []() {
    String body = server.arg("plain");
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, body);

    if (error) {
      server.send(400, "application/json", "{\"success\":false,\"error\":\"JSON解析失败\"}");
      return;
    }

    String deviceId = doc["deviceId"] | "";
    if (deviceId.isEmpty()) {
      server.send(400, "application/json", "{\"success\":false,\"error\":\"缺少deviceId\"}");
      return;
    }

    bool ok = db.deleteBadgeConfig(deviceId);
    server.send(200, "application/json", ok
      ? "{\"success\":true,\"note\":\"配置已删除\"}"
      : "{\"success\":false,\"error\":\"删除失败\"}");
  });

  // 调试信息
  server.on("/api/debug", HTTP_GET, []() {
    String response = "{";
    response += "\"uptime\":" + String(millis()) + ",";
    response += "\"heap\":" + String(ESP.getFreeHeap()) + ",";
    response += "\"devices\":" + String(bleScanner.getDetectionCount()) + ",";
    response += "\"today_date\":\"" + getTodayDate() + "\",";
    response += "\"current_time\":\"" + formatTime(getUnixTime()) + "\"";
    response += "}";
    server.send(200, "application/json", response);
  });

  server.begin();
  Serial.println("[Web] Server started on port 80");
}

// 大栈主任务：loopTask 默认只有 8192 字节栈，RISC-V 上不够用
// 创建 32768 字节栈任务来执行所有初始化 + 主循环
void mainTask(void* param) {
  setupWiFi();
  setupNTP();

  // BLE
  bleScanner.init(&db);
  bleScanner.setClient(&bleClient);

  // WebServer
  setupWebServer();

  Serial.println("\n================================");
  Serial.println("  Web服务器已启动");
  Serial.print("  访问地址: http://");
  Serial.print(WiFi.localIP());
  Serial.println("/");
  Serial.println("================================\n");
  Serial.println("[System] Ready!");

  // 主循环
  while (true) {
    server.handleClient();
    bleScanner.loop();
    timeClient.update();
    delay(10);
  }
}

void setup() {
  Serial.begin(115200);
  delay(2000);  // 等待串口稳定，不阻塞

  // 数据库初始化在 setup() 中执行（SPIFFS 在 mainTask 中初始化有问题）
  Serial.println("[DB] Initializing...");
  if (!db.init()) {
    Serial.println("[System] Database init failed!");
  } else {
    Serial.println("[DB] Initialized successfully");
  }

  // 在大栈任务中运行 WiFi/BLE/Web 初始化和主循环
  xTaskCreatePinnedToCore(mainTask, "mainTask", 32768, NULL, 1, NULL, 1);
}

void loop() {
  // 所有工作在 mainTask 中完成
  delay(1000);
}
