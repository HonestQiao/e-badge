#ifndef CONFIG_H
#define CONFIG_H

// WiFi配置
#define WIFI_SSID     "OpenBSD"
#define WIFI_PASSWORD "13581882013"
#define WIFI_CHANNEL  6

// NTP配置
#define NTP_SERVER          "pool.ntp.org"
#define NTP_TIME_OFFSET     28800   // 北京时间 UTC+8 (秒)
#define NTP_UPDATE_INTERVAL 60000   // NTP更新间隔 (ms)

// BLE配置
#define BLE_RSSI_THRESHOLD  -70    // 打卡判定阈值 (dBm)
#define BLE_SCAN_INTERVAL   100    // 扫描间隔 (ms)
#define BLE_SCAN_WINDOW     99     // 扫描窗口 (ms)
#define CHECKIN_COOLDOWN    5      // 打卡冷却时间 (秒)

// BLE UUID
#define SERVICE_UUID        "0000ABCD-0000-1000-8000-00805F9B34FB"
#define CHARACTERISTIC_UUID "00001234-0000-1000-8000-00805F9B34FB"

// 设备信息
#define DEVICE_NAME_PREFIX  "M5B_"
#define DEVICE_NAME_PREFIX_C3 "C3B_"

// Web服务器
#define WEB_PORT            80
#define MAX_CHECKIN_RECORDS 50

#endif
