#ifndef STORAGE_H
#define STORAGE_H

#include <Arduino.h>
#include <Preferences.h>

struct BadgeInfo {
  String name;
  String dept;
  String title;
  String badgeId;
  bool configured;
};

class Storage {
public:
  void init();
  BadgeInfo load();
  void save(const BadgeInfo& info);
  void clear();

  // 打卡时间本地持久化
  String loadCheckInTime();
  void saveCheckInTime(const String& time);
  void clearCheckInTime();

  // 当前日期（用于跨天判断）
  String loadCurrentDate();
  void saveCurrentDate(const String& date);
  void clearCurrentDate();
};

#endif
