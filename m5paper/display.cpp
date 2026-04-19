#include "display.h"

// M5GFX EPD 颜色：必须用 uint8_t 类型才能被识别为灰度值
static constexpr uint8_t C_WHITE = 255;
static constexpr uint8_t C_BLACK = 0;
static constexpr uint8_t C_GRAY  = 240;

// 与 Python 模拟器完全一致的布局常量
static constexpr int HEADER_H     = 80;   // 顶部条高度
static constexpr int AVATAR_Y     = 140;  // 头像圆顶部 Y
static constexpr int AVATAR_R     = 70;   // 头像圆半径
static constexpr int NAME_Y       = 340;  // 姓名 Y
static constexpr int SEP_Y        = 400;  // 分隔线 Y
static constexpr int DEPT_Y       = 465;  // 部门 Y
static constexpr int TITLE_Y      = 540;  // 职位 Y
static constexpr int ID_Y         = 615;  // 工号 Y
static constexpr int STATUS_BAR_Y = 880;  // 底部状态栏顶部 Y
static constexpr int STATUS_BAR_H = 80;   // 底部状态栏高度

void Display::init() {
  M5.begin();
  M5.Display.setRotation(0);
  M5.Display.clear();
}

bool Display::canRefresh() {
  unsigned long now = millis();
  if (now - lastRefresh < REFRESH_COOLDOWN) return false;
  lastRefresh = now;
  return true;
}

void Display::clearScreen() {
  M5.Display.clear();
}

// 使用 MC_DATUM (Middle Center)，让 x,y 作为文字几何中心
void Display::drawCenteredText(const char* text, int x, int y, const lgfx::IFont* font, float scale) {
  auto& dsp = M5.Display;
  dsp.setFont(font);
  dsp.setTextSize(scale);
  dsp.setTextDatum(MC_DATUM);
  dsp.drawString(text, x, y);
}

// 绘制加粗圆框（多层同心圆）
void Display::drawThickCircle(int x, int y, int r, uint8_t color, int thickness) {
  auto& dsp = M5.Display;
  for (int i = 0; i < thickness; i++) {
    dsp.drawCircle(x, y, r - i, color);
  }
}

void Display::showBadge(const BadgeInfo& info, bool isConfigured) {
  if (!canRefresh()) return;
  drawBadgeContent(info, isConfigured);
  M5.Display.display();
}

void Display::drawBadgeContent(const BadgeInfo& info, bool isConfigured) {
  auto& dsp = M5.Display;
  int cx = SCREEN_WIDTH / 2;

  dsp.startWrite();

  // 1. 白色背景
  dsp.fillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, C_WHITE);

  // 2. 顶部黑色条 [0, 0, 540, 80]
  dsp.fillRect(0, 0, SCREEN_WIDTH, HEADER_H, C_BLACK);

  // 3. 标题文字（白色）
  dsp.setTextColor(C_WHITE);
  drawCenteredText("\xe7\x94\xb5\xe5\xad\x90\xe5\xb7\xa5\xe7\x89\x8c", cx, 30,
                   &fonts::efontCN_24, 1.0f);

  // 4. 头像圆框（加粗，圆心 Y = 140 + 70 = 210）
  int circleCenterY = AVATAR_Y + AVATAR_R;  // 210
  drawThickCircle(cx, circleCenterY, AVATAR_R, C_BLACK, 3);

  // 5. 姓名首字（居中于圆心，大号字体）
  String firstChar = info.name.substring(0, 3);
  dsp.setTextColor(C_BLACK);
  drawCenteredText(firstChar.c_str(), cx, circleCenterY,
                   &fonts::efontCN_24, 2.0f);

  // 6. 姓名（大号字体）
  drawCenteredText(info.name.c_str(), cx, NAME_Y,
                   &fonts::efontCN_24, 2.0f);

  // 7. 分隔线（加粗）
  dsp.drawLine(60, SEP_Y, SCREEN_WIDTH - 60, SEP_Y, C_BLACK);
  dsp.drawLine(60, SEP_Y + 1, SCREEN_WIDTH - 60, SEP_Y + 1, C_BLACK);

  // 8. 部门（中号字体）
  dsp.setTextColor(C_BLACK);
  drawCenteredText(info.dept.c_str(), cx, DEPT_Y,
                   &fonts::efontCN_24, 1.5f);

  // 9. 职位（中号字体）
  drawCenteredText(info.title.c_str(), cx, TITLE_Y,
                   &fonts::efontCN_24, 1.5f);

  // 10. 工号（中号字体，灰色）
  char idBuf[48];
  snprintf(idBuf, sizeof(idBuf), "\xe5\xb7\xa5\xe5\x8f\xb7\xef\xbc\x9a%s", info.badgeId.c_str());
  dsp.setTextColor(C_GRAY);
  drawCenteredText(idBuf, cx, ID_Y,
                   &fonts::efontCN_24, 1.2f);

  // 11. 底部状态栏
  dsp.fillRect(0, STATUS_BAR_Y, SCREEN_WIDTH, STATUS_BAR_H, C_BLACK);
  dsp.setTextColor(C_WHITE);
  if (!checkInTime.isEmpty()) {
    char buf[48];
    snprintf(buf, sizeof(buf), "\xe5\xb7\xb2\xe6\x89\x93\xe5\x8d\xa1 %s", checkInTime.c_str());
    drawCenteredText(buf, cx, STATUS_BAR_Y + STATUS_BAR_H / 2,
                     &fonts::efontCN_24, 1.0f);
  } else if (isConfigured) {
    drawCenteredText("\xe8\xaf\xb7\xe5\xb0\xbd\xe5\xbf\xab\xe6\x89\x93\xe5\x8d\xa1", cx, STATUS_BAR_Y + STATUS_BAR_H / 2,
                     &fonts::efontCN_24, 1.0f);
  } else {
    drawCenteredText("\xe6\x9c\xaa\xe8\xbf\x9e\xe6\x8e\xa5", cx, STATUS_BAR_Y + STATUS_BAR_H / 2,
                     &fonts::efontCN_24, 1.0f);
  }

  dsp.endWrite();
}

void Display::showWaitingConfig(const String& deviceId) {
  if (!canRefresh()) return;
  drawWaitingContent(deviceId);
  M5.Display.display();
}

void Display::drawWaitingContent(const String& deviceId) {
  auto& dsp = M5.Display;
  int cx = SCREEN_WIDTH / 2;

  dsp.startWrite();

  // 白色背景
  dsp.fillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, C_WHITE);

  // 顶部灰色条（等待配置状态用灰色）
  dsp.fillRect(0, 0, SCREEN_WIDTH, HEADER_H, C_GRAY);
  dsp.setTextColor(C_WHITE);
  drawCenteredText("\xe7\x94\xb5\xe5\xad\x90\xe5\xb7\xa5\xe7\x89\x8c", cx, 30,
                   &fonts::efontCN_24, 1.0f);

  int centerY = SCREEN_HEIGHT / 2 - 100;

  // 问号圆圈（加粗）
  drawThickCircle(cx, centerY, 60, C_BLACK, 3);
  dsp.setTextColor(C_BLACK);
  drawCenteredText("?", cx, centerY,
                   &fonts::efontCN_24, 2.0f);

  // 等待文字
  int waitY = centerY + 60 + 40;
  drawCenteredText("\xe7\xad\x89\xe5\xbe\x85\xe9\x85\x8d\xe7\xbd\xae...", cx, waitY,
                   &fonts::efontCN_24, 1.5f);

  // 提示信息：在P4管理页面配置
  int hintY = waitY + 60;
  dsp.setTextColor(C_GRAY);
  drawCenteredText("\xe8\xaf\xb7\xe5\x9c\xa8P4\xe7\xae\xa1\xe7\x90\x86\xe9\xa1\xb5\xe9\x9d\xa2\xe9\x85\x8d\xe7\xbd\xae",
                   cx, hintY, &fonts::efontCN_24, 1.0f);

  // 显示设备ID，方便在P4页面上识别
  if (!deviceId.isEmpty()) {
    String idLabel = "\xe8\xae\xbe\xe5\xa4\x87ID: " + deviceId;
    drawCenteredText(idLabel.c_str(), cx, hintY + 45,
                     &fonts::efontCN_24, 1.0f);
  }

  // 底部黑色状态栏
  dsp.fillRect(0, STATUS_BAR_Y, SCREEN_WIDTH, STATUS_BAR_H, C_BLACK);
  dsp.setTextColor(C_WHITE);
  drawCenteredText("\xe6\x9c\xaa\xe9\x85\x8d\xe7\xbd\xae", cx, STATUS_BAR_Y + STATUS_BAR_H / 2,
                   &fonts::efontCN_24, 1.0f);

  dsp.endWrite();
}

void Display::showCheckInStatus(const String& time) {
  checkInTime = time;
  auto& dsp = M5.Display;
  int cx = SCREEN_WIDTH / 2;

  dsp.fillRect(0, STATUS_BAR_Y, SCREEN_WIDTH, STATUS_BAR_H, C_GRAY);
  dsp.setTextColor(C_WHITE);
  char buf[48];
  snprintf(buf, sizeof(buf), "\xe5\xb7\xb2\xe6\x89\x93\xe5\x8d\xa1 %s", time.c_str());
  drawCenteredText(buf, cx, STATUS_BAR_Y + STATUS_BAR_H / 2,
                   &fonts::efontCN_24, 1.0f);
  dsp.display();
}

void Display::showPromptCheckIn() {
  checkInTime = "";
  auto& dsp = M5.Display;
  int cx = SCREEN_WIDTH / 2;

  dsp.fillRect(0, STATUS_BAR_Y, SCREEN_WIDTH, STATUS_BAR_H, C_BLACK);
  dsp.setTextColor(C_WHITE);
  drawCenteredText("\xe8\xaf\xb7\xe5\xb0\xbd\xe5\xbf\xab\xe6\x89\x93\xe5\x8d\xa1", cx, STATUS_BAR_Y + STATUS_BAR_H / 2,
                   &fonts::efontCN_24, 1.0f);
  dsp.display();
}

void Display::clearCheckInStatus() {
  checkInTime = "";
}
