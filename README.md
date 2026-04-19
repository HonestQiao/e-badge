# 电子工牌系统

基于 ESP32-P4 + M5Paper/ESP32-C3 的智能电子工牌系统

## 功能特性

- **电子工牌展示**: M5Paper 墨水屏显示姓名、部门、职位、工号
- **无线配置**: 手机与 P4 连接同一 WiFi，通过 Web 页面配置工牌信息
- **自动打卡**: M5Paper 靠近 P4（RSSI ≥ -70dBm）自动识别并打卡
- **每日一次打卡**: 同一设备每天只打卡一次，跨天自动清除状态
- **检测记录**: Web 页面实时显示蓝牙检测到的设备
- **打卡记录**: SQLite 持久化存储，含设备 ID、姓名、时间、RSSI
- **配置自动重启**: M5Paper 收到新配置后自动重启，P4 重新检测并自动打卡
- **时间同步**: P4 每日自动同步日期到 M5Paper，跨天自动清除打卡状态
- **新设备处理**: 未配置的设备首次被检测到时，自动清空本地配置并显示等待配置页
- **多客户端支持**: 同时支持 M5Paper（带屏幕）和 ESP32-C3（无屏幕，串口输出状态）作为打卡端

## 硬件要求

| 设备 | 型号 | 角色 | 连接 |
|------|------|------|------|
| 服务端 | FireBeetle 2 ESP32-P4 (集成 ESP32-C6) | Web服务器 + BLE扫描 | USB `/dev/cu.usbmodem14101` |
| 客户端 | M5Paper | 带屏幕工牌显示 | USB `/dev/cu.wchusbserial59750058261` |
| 客户端 | ESP32-C3 (4MB Flash) | 无屏幕，串口输出状态 | USB `/dev/cu.wchusbserial5ABA0795651` |

**注意**: ESP32-P4 本身无 WiFi/BLE，开发板已集成 ESP32-C6 协处理器。Arduino 中直接使用标准库即可。

## 目录结构

```
电子工牌/
├── esp32-p4/              # ESP32-P4 服务端代码
│   ├── esp32-p4.ino       # 主程序 (WiFi/NTP/WebServer/BLE扫描)
│   ├── config.h           # 配置常量
│   ├── index_html.h       # Web 页面 HTML
│   ├── database.h/cpp     # SQLite 数据库 (打卡记录 + 工牌配置，双文件)
│   ├── ble_scanner.h/cpp  # BLE 扫描/打卡/新设备检测
│   └── ble_client.h/cpp   # BLE 连接/配置下发/打卡通知
│
├── m5paper/               # M5Paper 客户端代码
│   ├── m5paper.ino        # 主程序 (BLE广播/配置解析/时间同步)
│   ├── config.h           # 配置常量
│   ├── storage.h/cpp      # Preferences 存储 (工牌信息 + 打卡时间 + 日期)
│   ├── display.h/cpp      # 墨水屏 UI (工牌展示/等待配置/打卡状态)
│   └── ble_peripheral.h/cpp # BLE 广播 + GATT 服务 (接收配置)
│
├── esp32-c3/              # ESP32-C3 客户端代码（无屏幕版）
│   ├── esp32-c3.ino       # 主程序 (串口输出状态)
│   ├── config.h           # 配置常量
│   ├── storage.h/cpp      # Preferences 存储
│   └── ble_peripheral.h/cpp # BLE 广播 + GATT 服务
│
├── 项目规划.md
└── README.md
```

## 依赖库

### FireBeetle 2 ESP32-P4 端
- **ESP32 Arduino Core v3.0+** (需支持 ESP32-P4)
  - 板卡选择: **FireBeetle 2 ESP32-P4**
  - 分区方案: **8MB with spiffs**
- **ArduinoJson** (库管理器安装)
- **Sqlite3Esp32** (库管理器安装)

### M5Paper 端
- **M5Stack** board support
- **M5Unified** + **M5GFX**
- **ArduinoJson**

### ESP32-C3 端
- **ESP32 Arduino Core v3.0+**
  - 板卡选择: **ESP32C3 Dev Module**
  - 分区方案: **Default 4MB with spiffs**
- **ArduinoJson**

## 编译烧录

### 开发板信息汇总

| 开发板 | 板卡FQBN | 串口 | 分区方案 | 备注 |
|--------|---------|------|---------|------|
| P4服务端 | `esp32:esp32:dfrobot_firebeetle2_esp32p4` | `/dev/cu.usbmodem14101` | `default_8MB` | 必须选8MB，否则固件超容 |
| M5Paper客户端 | `m5stack:esp32:m5stack_paper` | `/dev/cu.wchusbserial59750058261` | 默认 | 通过WCH串口芯片连接 |
| ESP32-C3客户端 | `esp32:esp32:esp32c3` | `/dev/cu.wchusbserial5ABA0795651` | `default` | 4MB Flash |

**查询串口**: `arduino-cli board list` 或 `ls /dev/cu.*`

### FireBeetle 2 ESP32-P4
```bash
cd esp32-p4
# 编译（必须指定 8MB 分区方案，否则固件 1.4MB 超过 1.2MB 限制）
arrow-cli compile --fqbn esp32:esp32:dfrobot_firebeetle2_esp32p4:PartitionScheme=default_8MB .
# 烧录
arduino-cli upload -p /dev/cu.usbmodem14101 --fqbn esp32:esp32:dfrobot_firebeetle2_esp32p4:PartitionScheme=default_8MB .
```

### M5Paper
```bash
cd m5paper
# 编译
arduino-cli compile --fqbn m5stack:esp32:m5stack_paper .
# 烧录
arduino-cli upload -p /dev/cu.wchusbserial59750058261 --fqbn m5stack:esp32:m5stack_paper .
```

### ESP32-C3
```bash
cd esp32-c3
# 编译
arduino-cli compile --fqbn esp32:esp32:esp32c3 .
# 烧录
arduino-cli upload -p /dev/cu.wchusbserial5ABA0795651 --fqbn esp32:esp32:esp32c3 .
```

## 使用流程

### M5Paper 客户端

1. **启动 P4**: 自动连接 WiFi，启动 Web 服务器和 BLE 扫描
2. **启动 M5Paper**: 首次启动（或配置被清空后）显示**等待配置页面**，含设备 ID
3. **Web 配置**:
   - 浏览器访问 P4 的 IP 地址
   - 点击"工牌配置"标签，选择检测到的设备
   - 填写姓名/部门/职位/工号，点击"发送配置到设备"
4. **M5Paper 自动重启**: 收到配置后 2 秒自动重启
5. **P4 自动打卡**: M5Paper 重启后 P4 重新检测到，自动完成打卡
6. **M5Paper 显示**: 工牌信息 + 底部"已打卡 XX:XX:XX"

### ESP32-C3 客户端

C3 无屏幕，所有状态通过串口输出（波特率 115200）：

1. **启动 C3**: 串口输出设备 ID，未配置时显示`[Status] 等待配置...`
2. **Web 配置**: 与 M5Paper 相同，在 P4 管理页面选择 `C3B_XXXXXXXX` 设备配置
3. **C3 自动重启**: 收到配置后 2 秒自动重启
4. **串口查看状态**:
   - 已配置未打卡: `[Status] 已配置，请尽快打卡`
   - 已打卡: `[Status] 已打卡 09:15:32`
   - 跨天清除: `[Time] 日期已变更，清除打卡状态`

## Web API

| 路径 | 方法 | 功能 |
|------|------|------|
| `/` | GET | 管理页面（四标签：打卡记录/检测记录/工牌信息/工牌配置） |
| `/api/badge_list` | GET | 在线设备列表（30 分钟内检测到） |
| `/api/checkin_list` | GET | 打卡记录 JSON |
| `/api/detection_list` | GET | 蓝牙检测记录 JSON |
| `/api/stats` | GET | 今日打卡数/在线设备数/日期 |
| `/api/config` | POST | 提交工牌配置到指定设备 |
| `/api/badge_configs` | GET | 所有已保存的工牌配置列表 |
| `/api/badge_config` | GET | 查询单个工牌配置（参数：deviceId） |
| `/api/delete_config` | POST | 删除工牌配置（参数：deviceId） |
| `/api/debug` | GET | 系统调试信息 |

## BLE 通信协议

P4 与客户端（M5Paper/ESP32-C3）通过 BLE GATT 通信，消息格式：

| 消息 | 方向 | 说明 |
|------|------|------|
| `CLEAR_CONFIG` | P4 → 客户端 | 新设备首次检测到，清空本地配置 |
| `CHECKIN:HH:MM:SS` | P4 → 客户端 | 今日首次打卡通知 |
| `TIME:YYYY-MM-DD` | P4 → 客户端 | 时间同步（配置/打卡时附带） |
| `{"name":"...","dept":"...","title":"...","id":"..."}` | P4 → 客户端 | 工牌配置 JSON |

**设备广播名称**:
- M5Paper: `M5B_XXXXXXXX` (8位十六进制设备ID)
- ESP32-C3: `C3B_XXXXXXXX` (8位十六进制设备ID)

## 配置参数

`esp32-p4/config.h`:

| 参数 | 默认值 | 说明 |
|------|--------|------|
| WIFI_SSID | OpenBSD | WiFi 名称 |
| WIFI_PASSWORD | ******** | WiFi 密码 |
| NTP_SERVER | pool.ntp.org | NTP 服务器 |
| NTP_TIME_OFFSET | 28800 | 北京时间 UTC+8 (秒) |
| BLE_RSSI_THRESHOLD | -70 | 打卡 RSSI 阈值 (dBm) |
| BLE_SCAN_INTERVAL | 100 | BLE 扫描间隔 (ms) |
| BLE_SCAN_WINDOW | 99 | BLE 扫描窗口 (ms) |
| CHECKIN_COOLDOWN | 5 | 打卡冷却 (秒) |
| DEVICE_NAME_PREFIX | M5B_ | M5Paper 广播名称前缀 |
| DEVICE_NAME_PREFIX_C3 | C3B_ | ESP32-C3 广播名称前缀 |

`m5paper/config.h`:

| 参数 | 默认值 | 说明 |
|------|--------|------|
| SCREEN_WIDTH | 540 | 屏幕宽度 |
| SCREEN_HEIGHT | 960 | 屏幕高度 |
| REFRESH_COOLDOWN | 2000 | 最小刷新间隔 (ms) |

`esp32-c3/config.h`:

| 参数 | 默认值 | 说明 |
|------|--------|------|
| DEVICE_NAME_PREFIX | C3B_ | C3 广播名称前缀 |

| 参数 | 默认值 | 说明 |
|------|--------|------|
| SCREEN_WIDTH | 540 | 屏幕宽度 |
| SCREEN_HEIGHT | 960 | 屏幕高度 |
| REFRESH_COOLDOWN | 2000 | 最小刷新间隔 (ms) |

## 问题排查

### ESP32-C6 协处理器固件异常

如果 P4 串口出现 `rpc_core: Response not received` 错误，说明 C6 固件需要更新。

**参考资源**:
- [DFRobot C6 固件更新教程](https://wiki.dfrobot.com.cn/SKU_DFR1172_FireBeetle_2_ESP32_P4#9.ESP32-C6固件更新)
- [ESP-Hosted Slave 示例](https://components.espressif.com/components/espressif/esp_hosted/versions/2.12.3/examples/slave)

### SQLite disk I/O error

ESP32 SPIFFS 上的 sqlite3 不支持 `AUTOINCREMENT`、`UNIQUE`、`TEXT PRIMARY KEY` 等需要创建额外索引的操作。本项目使用双数据库文件 workaround：
- `/spiffs/checkin.db` —— 打卡记录表
- `/spiffs/config.db` —— 工牌配置表
