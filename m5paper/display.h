#ifndef DISPLAY_H
#define DISPLAY_H

#include <M5Unified.h>
#include "storage.h"
#include "config.h"

class Display {
public:
  void init();
  void showBadge(const BadgeInfo& info, bool isConfigured = true);
  void showWaitingConfig(const String& deviceId = "");
  void showCheckInStatus(const String& time);
  void showPromptCheckIn();
  void clearCheckInStatus();
  void clearScreen();

private:
  unsigned long lastRefresh = 0;
  String checkInTime;  // 当前已打卡时间，为空表示未打卡
  bool canRefresh();
  void drawBadgeContent(const BadgeInfo& info, bool isConfigured = true);
  void drawWaitingContent(const String& deviceId = "");
  void drawCenteredText(const char* text, int x, int y, const lgfx::IFont* font, float scale = 1.0f);
  void drawThickCircle(int x, int y, int r, uint8_t color, int thickness);
};

#endif
