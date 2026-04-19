#ifndef DATABASE_H
#define DATABASE_H

#include <Arduino.h>

// 打卡记录结构
struct CheckInDBRecord {
  int id;
  String deviceId;
  String name;
  String dept;
  String title;
  String badgeId;
  String date;
  String checkinTime;
  unsigned long lastDetected;
  int checkinRssi;
};

// 工牌配置结构
struct BadgeConfigDBRecord {
  String deviceId;
  String name;
  String dept;
  String title;
  String badgeId;
  unsigned long updatedAt;
};

class Database {
public:
  bool init();
  void close();

  // 打卡记录
  bool addCheckIn(const String& deviceId, const String& name, const String& dept,
                  const String& title, const String& badgeId, const String& date,
                  const String& checkinTime, unsigned long lastDetected, int rssi);
  bool updateLastDetected(const String& deviceId, const String& date, unsigned long lastDetected);
  bool hasCheckInToday(const String& deviceId, const String& date);
  unsigned long getLastDetected(const String& deviceId, const String& date);
  int getCheckInRecords(CheckInDBRecord* out, int maxCount);
  int getTodayCheckInCount(const String& date);

  // 工牌配置
  bool saveBadgeConfig(const String& deviceId, const String& name, const String& dept,
                       const String& title, const String& badgeId, unsigned long updatedAt);
  bool getBadgeConfig(const String& deviceId, BadgeConfigDBRecord& out);
  int getAllBadgeConfigs(BadgeConfigDBRecord* out, int maxCount);
  bool deleteBadgeConfig(const String& deviceId);

private:
  struct sqlite3* dbCheckIn = nullptr;
  struct sqlite3* dbConfig = nullptr;
  bool exec(struct sqlite3* dbHandle, const char* sql);
};

#endif
