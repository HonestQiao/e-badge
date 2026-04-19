#include "storage.h"

#define PREFS_NAMESPACE "badge"

void Storage::init() {
  // Preferences在load/save时自动打开
}

BadgeInfo Storage::load() {
  Preferences prefs;
  BadgeInfo info;

  if (!prefs.begin(PREFS_NAMESPACE, true)) {
    info.configured = false;
    return info;
  }

  info.name = prefs.getString("name", "");
  info.dept = prefs.getString("dept", "");
  info.title = prefs.getString("title", "");
  info.badgeId = prefs.getString("id", "");
  info.configured = prefs.getBool("configured", false);

  prefs.end();
  return info;
}

void Storage::save(const BadgeInfo& info) {
  Preferences prefs;
  if (!prefs.begin(PREFS_NAMESPACE, false)) return;

  prefs.putString("name", info.name);
  prefs.putString("dept", info.dept);
  prefs.putString("title", info.title);
  prefs.putString("id", info.badgeId);
  prefs.putBool("configured", info.configured);

  prefs.end();
}

void Storage::clear() {
  Preferences prefs;
  if (!prefs.begin(PREFS_NAMESPACE, false)) return;
  prefs.clear();
  prefs.end();
}

String Storage::loadCheckInTime() {
  Preferences prefs;
  String time;
  if (!prefs.begin(PREFS_NAMESPACE, true)) return time;
  time = prefs.getString("citime", "");
  prefs.end();
  return time;
}

void Storage::saveCheckInTime(const String& time) {
  Preferences prefs;
  if (!prefs.begin(PREFS_NAMESPACE, false)) return;
  prefs.putString("citime", time);
  prefs.end();
}

void Storage::clearCheckInTime() {
  Preferences prefs;
  if (!prefs.begin(PREFS_NAMESPACE, false)) return;
  prefs.remove("citime");
  prefs.end();
}

String Storage::loadCurrentDate() {
  Preferences prefs;
  String date;
  if (!prefs.begin(PREFS_NAMESPACE, true)) return date;
  date = prefs.getString("curdate", "");
  prefs.end();
  return date;
}

void Storage::saveCurrentDate(const String& date) {
  Preferences prefs;
  if (!prefs.begin(PREFS_NAMESPACE, false)) return;
  prefs.putString("curdate", date);
  prefs.end();
}

void Storage::clearCurrentDate() {
  Preferences prefs;
  if (!prefs.begin(PREFS_NAMESPACE, false)) return;
  prefs.remove("curdate");
  prefs.end();
}
