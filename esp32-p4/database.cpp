#include "database.h"
#include <sqlite3.h>
#include <SPIFFS.h>

#define DB_CHECKIN_PATH "/spiffs/checkin.db"
#define DB_CONFIG_PATH  "/spiffs/config.db"

static bool openDb(const char* path, sqlite3** outDb) {
  int rc = sqlite3_open(path, outDb);
  if (rc != SQLITE_OK) {
    Serial.print("[DB] Open failed: ");
    Serial.println(sqlite3_errmsg(*outDb));
    return false;
  }
  return true;
}

bool Database::init() {
  if (!SPIFFS.begin(true)) {
    Serial.println("[DB] SPIFFS mount failed, trying format...");
    SPIFFS.format();
    if (!SPIFFS.begin(true)) {
      Serial.println("[DB] SPIFFS re-mount failed after format");
      return false;
    }
    Serial.println("[DB] SPIFFS formatted and mounted");
  } else {
    Serial.println("[DB] SPIFFS mounted");
  }

  sqlite3_initialize();

  // 打卡记录数据库（单文件单表，避免SPIFFS多表I/O错误）
  if (!openDb(DB_CHECKIN_PATH, &dbCheckIn)) {
    return false;
  }
  if (!exec(dbCheckIn, "CREATE TABLE IF NOT EXISTS checkin_records ("
       "id INTEGER PRIMARY KEY,"
       "device_id TEXT NOT NULL,"
       "name TEXT,"
       "dept TEXT,"
       "title TEXT,"
       "badge_id TEXT,"
       "date TEXT NOT NULL,"
       "checkin_time TEXT NOT NULL,"
       "last_detected INTEGER,"
       "checkin_rssi INTEGER"
       ");")) {
    Serial.println("[DB] Failed to create checkin_records table");
  } else {
    Serial.println("[DB] checkin_records table OK");
  }

  // 工牌配置数据库（独立文件）
  if (!openDb(DB_CONFIG_PATH, &dbConfig)) {
    sqlite3_close(dbCheckIn);
    dbCheckIn = nullptr;
    return false;
  }
  if (!exec(dbConfig, "CREATE TABLE IF NOT EXISTS badge_configs ("
       "id INTEGER PRIMARY KEY,"
       "device_id TEXT NOT NULL,"
       "name TEXT,"
       "dept TEXT,"
       "title TEXT,"
       "badge_id TEXT,"
       "updated_at INTEGER"
       ");")) {
    Serial.println("[DB] Failed to create badge_configs table");
  } else {
    Serial.println("[DB] badge_configs table OK");
  }

  Serial.println("[DB] Initialized");
  return true;
}

void Database::close() {
  if (dbCheckIn) {
    sqlite3_close(dbCheckIn);
    dbCheckIn = nullptr;
  }
  if (dbConfig) {
    sqlite3_close(dbConfig);
    dbConfig = nullptr;
  }
}

bool Database::exec(sqlite3* dbHandle, const char* sql) {
  char* errMsg = nullptr;
  int rc = sqlite3_exec(dbHandle, sql, nullptr, nullptr, &errMsg);
  if (rc != SQLITE_OK) {
    Serial.print("[DB] SQL error: ");
    Serial.println(errMsg);
    sqlite3_free(errMsg);
    return false;
  }
  return true;
}

bool Database::addCheckIn(const String& deviceId, const String& name, const String& dept,
                          const String& title, const String& badgeId, const String& date,
                          const String& checkinTime, unsigned long lastDetected, int rssi) {
  sqlite3_stmt* stmt;
  const char* sql = "INSERT INTO checkin_records (device_id, name, dept, title, badge_id, date, checkin_time, last_detected, checkin_rssi) "
                    "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?);";
  int rc = sqlite3_prepare_v2(dbCheckIn, sql, -1, &stmt, nullptr);
  if (rc != SQLITE_OK) {
    Serial.print("[DB] addCheckIn prepare failed: ");
    Serial.println(sqlite3_errmsg(dbCheckIn));
    return false;
  }

  sqlite3_bind_text(stmt, 1, deviceId.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 2, name.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 3, dept.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 4, title.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 5, badgeId.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 6, date.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 7, checkinTime.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_int64(stmt, 8, lastDetected);
  sqlite3_bind_int(stmt, 9, rssi);

  rc = sqlite3_step(stmt);
  if (rc != SQLITE_DONE) {
    Serial.print("[DB] addCheckIn step failed: ");
    Serial.println(sqlite3_errmsg(dbCheckIn));
  }
  sqlite3_finalize(stmt);
  return rc == SQLITE_DONE;
}

bool Database::updateLastDetected(const String& deviceId, const String& date, unsigned long lastDetected) {
  sqlite3_stmt* stmt;
  const char* sql = "UPDATE checkin_records SET last_detected = ? WHERE device_id = ? AND date = ?;";
  int rc = sqlite3_prepare_v2(dbCheckIn, sql, -1, &stmt, nullptr);
  if (rc != SQLITE_OK) return false;

  sqlite3_bind_int64(stmt, 1, lastDetected);
  sqlite3_bind_text(stmt, 2, deviceId.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 3, date.c_str(), -1, SQLITE_TRANSIENT);

  rc = sqlite3_step(stmt);
  sqlite3_finalize(stmt);
  return rc == SQLITE_DONE;
}

bool Database::hasCheckInToday(const String& deviceId, const String& date) {
  sqlite3_stmt* stmt;
  const char* sql = "SELECT 1 FROM checkin_records WHERE device_id = ? AND date = ? LIMIT 1;";
  int rc = sqlite3_prepare_v2(dbCheckIn, sql, -1, &stmt, nullptr);
  if (rc != SQLITE_OK) return false;

  sqlite3_bind_text(stmt, 1, deviceId.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 2, date.c_str(), -1, SQLITE_TRANSIENT);

  bool found = sqlite3_step(stmt) == SQLITE_ROW;
  sqlite3_finalize(stmt);
  return found;
}

unsigned long Database::getLastDetected(const String& deviceId, const String& date) {
  sqlite3_stmt* stmt;
  const char* sql = "SELECT last_detected FROM checkin_records WHERE device_id = ? AND date = ?;";
  int rc = sqlite3_prepare_v2(dbCheckIn, sql, -1, &stmt, nullptr);
  if (rc != SQLITE_OK) return 0;

  sqlite3_bind_text(stmt, 1, deviceId.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 2, date.c_str(), -1, SQLITE_TRANSIENT);

  unsigned long result = 0;
  if (sqlite3_step(stmt) == SQLITE_ROW) {
    result = sqlite3_column_int64(stmt, 0);
  }
  sqlite3_finalize(stmt);
  return result;
}

int Database::getCheckInRecords(CheckInDBRecord* out, int maxCount) {
  sqlite3_stmt* stmt;
  const char* sql = "SELECT id, device_id, name, dept, title, badge_id, date, checkin_time, last_detected, checkin_rssi "
                    "FROM checkin_records ORDER BY date DESC, checkin_time DESC LIMIT ?;";
  int rc = sqlite3_prepare_v2(dbCheckIn, sql, -1, &stmt, nullptr);
  if (rc != SQLITE_OK) return 0;

  sqlite3_bind_int(stmt, 1, maxCount);

  int count = 0;
  while (sqlite3_step(stmt) == SQLITE_ROW && count < maxCount) {
    out[count].id = sqlite3_column_int(stmt, 0);
    out[count].deviceId = String((const char*)sqlite3_column_text(stmt, 1));
    out[count].name = String((const char*)sqlite3_column_text(stmt, 2));
    out[count].dept = String((const char*)sqlite3_column_text(stmt, 3));
    out[count].title = String((const char*)sqlite3_column_text(stmt, 4));
    out[count].badgeId = String((const char*)sqlite3_column_text(stmt, 5));
    out[count].date = String((const char*)sqlite3_column_text(stmt, 6));
    out[count].checkinTime = String((const char*)sqlite3_column_text(stmt, 7));
    out[count].lastDetected = sqlite3_column_int64(stmt, 8);
    out[count].checkinRssi = sqlite3_column_int(stmt, 9);
    count++;
  }
  sqlite3_finalize(stmt);
  return count;
}

int Database::getTodayCheckInCount(const String& date) {
  sqlite3_stmt* stmt;
  const char* sql = "SELECT COUNT(*) FROM checkin_records WHERE date = ?;";
  int rc = sqlite3_prepare_v2(dbCheckIn, sql, -1, &stmt, nullptr);
  if (rc != SQLITE_OK) return 0;

  sqlite3_bind_text(stmt, 1, date.c_str(), -1, SQLITE_TRANSIENT);

  int count = 0;
  if (sqlite3_step(stmt) == SQLITE_ROW) {
    count = sqlite3_column_int(stmt, 0);
  }
  sqlite3_finalize(stmt);
  return count;
}

bool Database::saveBadgeConfig(const String& deviceId, const String& name, const String& dept,
                               const String& title, const String& badgeId, unsigned long updatedAt) {
  sqlite3_stmt* delStmt;
  const char* delSql = "DELETE FROM badge_configs WHERE device_id = ?;";
  int rc = sqlite3_prepare_v2(dbConfig, delSql, -1, &delStmt, nullptr);
  if (rc == SQLITE_OK) {
    sqlite3_bind_text(delStmt, 1, deviceId.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_step(delStmt);
    sqlite3_finalize(delStmt);
  }

  sqlite3_stmt* stmt;
  const char* sql = "INSERT INTO badge_configs (device_id, name, dept, title, badge_id, updated_at) "
                    "VALUES (?, ?, ?, ?, ?, ?);";
  rc = sqlite3_prepare_v2(dbConfig, sql, -1, &stmt, nullptr);
  if (rc != SQLITE_OK) return false;

  sqlite3_bind_text(stmt, 1, deviceId.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 2, name.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 3, dept.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 4, title.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 5, badgeId.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_int64(stmt, 6, updatedAt);

  rc = sqlite3_step(stmt);
  sqlite3_finalize(stmt);
  return rc == SQLITE_DONE;
}

int Database::getAllBadgeConfigs(BadgeConfigDBRecord* out, int maxCount) {
  sqlite3_stmt* stmt;
  const char* sql = "SELECT device_id, name, dept, title, badge_id, updated_at FROM badge_configs ORDER BY updated_at DESC;";
  int rc = sqlite3_prepare_v2(dbConfig, sql, -1, &stmt, nullptr);
  if (rc != SQLITE_OK) return 0;

  int count = 0;
  while (sqlite3_step(stmt) == SQLITE_ROW && count < maxCount) {
    out[count].deviceId = String((const char*)sqlite3_column_text(stmt, 0));
    out[count].name = String((const char*)sqlite3_column_text(stmt, 1));
    out[count].dept = String((const char*)sqlite3_column_text(stmt, 2));
    out[count].title = String((const char*)sqlite3_column_text(stmt, 3));
    out[count].badgeId = String((const char*)sqlite3_column_text(stmt, 4));
    out[count].updatedAt = sqlite3_column_int64(stmt, 5);
    count++;
  }
  sqlite3_finalize(stmt);
  return count;
}

bool Database::getBadgeConfig(const String& deviceId, BadgeConfigDBRecord& out) {
  sqlite3_stmt* stmt;
  const char* sql = "SELECT device_id, name, dept, title, badge_id, updated_at FROM badge_configs WHERE device_id = ?;";
  int rc = sqlite3_prepare_v2(dbConfig, sql, -1, &stmt, nullptr);
  if (rc != SQLITE_OK) return false;

  sqlite3_bind_text(stmt, 1, deviceId.c_str(), -1, SQLITE_TRANSIENT);

  bool found = false;
  if (sqlite3_step(stmt) == SQLITE_ROW) {
    out.deviceId = String((const char*)sqlite3_column_text(stmt, 0));
    out.name = String((const char*)sqlite3_column_text(stmt, 1));
    out.dept = String((const char*)sqlite3_column_text(stmt, 2));
    out.title = String((const char*)sqlite3_column_text(stmt, 3));
    out.badgeId = String((const char*)sqlite3_column_text(stmt, 4));
    out.updatedAt = sqlite3_column_int64(stmt, 5);
    found = true;
  }
  sqlite3_finalize(stmt);
  return found;
}

bool Database::deleteBadgeConfig(const String& deviceId) {
  sqlite3_stmt* stmt;
  const char* sql = "DELETE FROM badge_configs WHERE device_id = ?;";
  int rc = sqlite3_prepare_v2(dbConfig, sql, -1, &stmt, nullptr);
  if (rc != SQLITE_OK) return false;

  sqlite3_bind_text(stmt, 1, deviceId.c_str(), -1, SQLITE_TRANSIENT);

  rc = sqlite3_step(stmt);
  sqlite3_finalize(stmt);
  return rc == SQLITE_DONE;
}
