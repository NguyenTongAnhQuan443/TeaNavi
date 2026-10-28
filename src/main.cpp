#include <Arduino.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include <math.h>

TFT_eSPI tft = TFT_eSPI();

// Sử dụng TFT_eSprite để render chống nháy (Flicker-free Double Buffering)
TFT_eSprite sprGauge = TFT_eSprite(&tft); // Sprite cho cụm tốc độ & gauge
TFT_eSprite sprNav   = TFT_eSprite(&tft); // Sprite cho cụm điều hướng (icon, khoảng cách, thanh progress)

// Định nghĩa màu sắc phong cách Cyberpunk / Sport HUD
#define COLOR_BG          0x0821  // Tím than siêu tối gần đen #080811
#define COLOR_CARD_BG     0x1084  // Thẻ tối #111422
#define COLOR_CARD_BORDER 0x2187  // Viền mờ thẻ
#define COLOR_CYAN            0x07FF  // Neon Cyan #00FFFF
#define COLOR_TEAL            0x0517  // Dark Cyan/Teal
#define COLOR_ORANGE          0xFD20  // Neon Orange #FFA500
#define COLOR_AMBER           0xFDC0  // Amber Porsche #FFA000
#define COLOR_ELECTRIC_YELLOW 0xFFE0  // Vàng điện Neon siêu sáng #FFFF00
#define COLOR_TRACK_RING      0x1A73  // Xanh đêm ánh thép Deep Cyber Blue cho rãnh kim loại
#define COLOR_RED             0xF800  // Red cảnh báo #FF0000
#define COLOR_GREEN           0x07E0  // Neon Green #00FF00
#define COLOR_WHITE           0xFFFF  // White
#define COLOR_GRAY            0x5AEB  // Neutral Gray
#define COLOR_DARK_GRAY       0x2124  // Tối hơn

#define TOP_BAR_H 24

// ========================================================
// 12 TRƯỜNG HỢP ĐIỀU HƯỚNG GIAO THÔNG THỰC TẾ
// ========================================================
enum NavDirection {
  NAV_STRAIGHT,      // 0: Đi thẳng
  NAV_SLIGHT_RIGHT,  // 1: Chếch phải (45 độ)
  NAV_TURN_RIGHT,    // 2: Rẽ phải vuông góc (90 độ)
  NAV_SHARP_RIGHT,   // 3: Rẽ gắt phải (>120 độ)
  NAV_SLIGHT_LEFT,   // 4: Chếch trái (45 độ)
  NAV_TURN_LEFT,     // 5: Rẽ trái vuông góc (90 độ)
  NAV_SHARP_LEFT,    // 6: Rẽ gắt trái (>120 độ)
  NAV_UTURN_LEFT,    // 7: Quay đầu bên trái
  NAV_UTURN_RIGHT,   // 8: Quay đầu bên phải
  NAV_FORK_RAMP,     // 9: Nhập làn / Tách làn đường nhánh cao tốc
  NAV_ROUNDABOUT,    // 10: Bùng binh / Vòng xuyến
  NAV_DESTINATION    // 11: Đã đến điểm đích
};

// ==========================================
// VẼ ĐẦY ĐỦ 12 BIỂU TƯỢNG ĐIỀU HƯỚNG BẰNG VECTOR
// ==========================================
void drawNavIcon(TFT_eSprite &spr, int cx, int cy, NavDirection dir, uint16_t color, int exitNum = 1) {
  switch (dir) {
    case NAV_STRAIGHT:
      // Đi thẳng: Mũi tên thẳng đứng
      spr.fillRoundRect(cx - 5, cy - 8, 10, 36, 3, color);
      spr.fillTriangle(cx, cy - 28, cx - 22, cy - 6, cx + 22, cy - 6, color);
      break;

    case NAV_SLIGHT_RIGHT:
      // Chếch phải: Thân nghiêng 45 độ
      spr.fillRoundRect(cx - 10, cy + 8, 10, 20, 3, color);
      spr.fillRoundRect(cx - 4, cy - 4, 18, 10, 3, color);
      // Đầu mũi tên nghiêng hướng 2 giờ
      spr.fillTriangle(cx + 22, cy - 22, cx + 6, cy - 26, cx + 26, cy - 6, color);
      spr.drawLine(cx - 2, cy + 10, cx + 16, cy - 8, color);
      spr.drawLine(cx - 3, cy + 10, cx + 15, cy - 8, color);
      spr.drawLine(cx - 4, cy + 10, cx + 14, cy - 8, color);
      break;

    case NAV_TURN_RIGHT:
      // Rẽ phải 90 độ
      spr.fillRoundRect(cx - 18, cy + 2, 10, 26, 3, color);
      spr.fillRoundRect(cx - 18, cy - 4, 30, 10, 3, color);
      spr.fillTriangle(cx + 26, cy + 1, cx + 10, cy - 16, cx + 10, cy + 18, color);
      break;

    case NAV_SHARP_RIGHT:
      // Rẽ gắt phải (quay ngoặt về sau bên phải)
      spr.fillRoundRect(cx - 14, cy - 18, 10, 34, 3, color);
      spr.fillRoundRect(cx - 14, cy - 24, 26, 10, 3, color);
      spr.fillRoundRect(cx + 2, cy - 18, 10, 22, 3, color);
      spr.fillTriangle(cx + 7, cy + 16, cx - 4, cy + 2, cx + 18, cy + 2, color);
      break;

    case NAV_SLIGHT_LEFT:
      // Chếch trái: Thân nghiêng hướng 10 giờ
      spr.fillRoundRect(cx, cy + 8, 10, 20, 3, color);
      spr.fillRoundRect(cx - 14, cy - 4, 18, 10, 3, color);
      spr.fillTriangle(cx - 22, cy - 22, cx - 6, cy - 26, cx - 26, cy - 6, color);
      spr.drawLine(cx + 2, cy + 10, cx - 16, cy - 8, color);
      spr.drawLine(cx + 3, cy + 10, cx - 15, cy - 8, color);
      spr.drawLine(cx + 4, cy + 10, cx - 14, cy - 8, color);
      break;

    case NAV_TURN_LEFT:
      // Rẽ trái 90 độ
      spr.fillRoundRect(cx + 8, cy + 2, 10, 26, 3, color);
      spr.fillRoundRect(cx - 12, cy - 4, 30, 10, 3, color);
      spr.fillTriangle(cx - 26, cy + 1, cx - 10, cy - 16, cx - 10, cy + 18, color);
      break;

    case NAV_SHARP_LEFT:
      // Rẽ gắt trái (quay ngoặt về sau bên trái)
      spr.fillRoundRect(cx + 4, cy - 18, 10, 34, 3, color);
      spr.fillRoundRect(cx - 12, cy - 24, 26, 10, 3, color);
      spr.fillRoundRect(cx - 12, cy - 18, 10, 22, 3, color);
      spr.fillTriangle(cx - 7, cy + 16, cx - 18, cy + 2, cx + 4, cy + 2, color);
      break;

    case NAV_UTURN_LEFT:
      // Quay đầu xe bên trái (chuẩn Việt Nam)
      spr.drawWideLine(cx - 14, cy + 24, cx - 14, cy - 2, 8, color, COLOR_CARD_BG);
      spr.drawWideLine(cx + 12, cy + 24, cx + 12, cy - 2, 8, color, COLOR_CARD_BG);
      spr.drawCircle(cx - 1, cy - 2, 15, color);
      spr.drawCircle(cx - 1, cy - 2, 14, color);
      spr.drawCircle(cx - 1, cy - 2, 13, color);
      spr.fillRect(cx - 18, cy - 1, 36, 15, COLOR_CARD_BG);
      spr.fillTriangle(cx - 14, cy + 26, cx - 24, cy + 12, cx - 4, cy + 12, color);
      break;

    case NAV_UTURN_RIGHT:
      // Quay đầu xe bên phải
      spr.drawWideLine(cx - 12, cy + 24, cx - 12, cy - 2, 8, color, COLOR_CARD_BG);
      spr.drawWideLine(cx + 14, cy + 24, cx + 14, cy - 2, 8, color, COLOR_CARD_BG);
      spr.drawCircle(cx + 1, cy - 2, 15, color);
      spr.drawCircle(cx + 1, cy - 2, 14, color);
      spr.drawCircle(cx + 1, cy - 2, 13, color);
      spr.fillRect(cx - 18, cy - 1, 36, 15, COLOR_CARD_BG);
      spr.fillTriangle(cx + 14, cy + 26, cx + 4, cy + 12, cx + 24, cy + 12, color);
      break;

    case NAV_FORK_RAMP:
      // Tách làn / Lối vào đường nhánh cao tốc
      spr.fillRoundRect(cx - 4, cy + 6, 8, 22, 2, color);
      spr.drawLine(cx, cy + 8, cx - 18, cy - 12, color);
      spr.drawLine(cx, cy + 8, cx + 18, cy - 12, color);
      // Mũi tên chính rẽ nhánh phải
      spr.fillTriangle(cx + 22, cy - 18, cx + 8, cy - 16, cx + 20, cy - 4, color);
      // Đường phụ mờ hơn bên trái
      spr.drawFastVLine(cx - 18, cy - 20, 10, COLOR_GRAY);
      break;

    case NAV_ROUNDABOUT:
      // Vòng xuyến kèm số lối ra
      spr.drawCircle(cx, cy - 2, 21, color);
      spr.drawCircle(cx, cy - 2, 20, color);
      spr.drawCircle(cx, cy - 2, 19, color);
      // Đường vào
      spr.fillRoundRect(cx - 4, cy + 14, 8, 14, 2, color);
      // Mũi tên thoát theo số lối ra
      if (exitNum == 1) { // Thoát góc 3h
        spr.fillTriangle(cx + 25, cy - 2, cx + 15, cy - 10, cx + 15, cy + 6, color);
      } else if (exitNum == 2) { // Thoát thẳng góc 12h
        spr.fillTriangle(cx, cy - 26, cx - 8, cy - 16, cx + 8, cy - 16, color);
      } else { // Thoát góc 9h
        spr.fillTriangle(cx - 25, cy - 2, cx - 15, cy - 10, cx - 15, cy + 6, color);
      }
      // Số lối ra ở giữa vòng tròn
      spr.setTextDatum(MC_DATUM);
      spr.setTextColor(COLOR_WHITE, COLOR_CARD_BG);
      spr.drawString(String(exitNum), cx, cy - 2, 4);
      break;

    case NAV_DESTINATION:
      // Đã đến điểm đích: Biểu tượng Marker / Cờ đích
      spr.fillCircle(cx, cy - 10, 16, COLOR_RED);
      spr.fillCircle(cx, cy - 10, 6, COLOR_WHITE);
      spr.fillTriangle(cx - 12, cy - 4, cx + 12, cy - 4, cx, cy + 18, COLOR_RED);
      break;
  }
}

// Sprite chuyên dụng cho Top Bar chống nháy
TFT_eSprite sprTopBar = TFT_eSprite(&tft);

// ==========================================
// VẼ ICON BLUETOOTH DẠNG BADGE HUY HIỆU
// Hiển thị trực quan: Connected (Xanh Neon phát sáng) vs Disconnected (Mờ / Gạch chéo báo ngắt)
// ==========================================
void drawBleIcon(TFT_eSprite &spr, int cx, int cy, bool connected) {
  int w = 18;
  int h = 18;
  int bx = cx - w / 2;
  int by = cy - h / 2;

  if (connected) {
    // Huy hiệu nền xanh đêm với viền Cyan rực rỡ
    spr.fillRoundRect(bx, by, w, h, 4, 0x01E8); // Deep Dark Cyan
    spr.drawRoundRect(bx, by, w, h, 4, COLOR_CYAN);

    // Ký hiệu Bluetooth chuẩn dày dặn sắc nét (Trắng / Cyan)
    spr.drawFastVLine(cx - 1, cy - 6, 13, COLOR_WHITE);
    spr.drawFastVLine(cx,     cy - 6, 13, COLOR_WHITE);

    // Cánh bướm bên phải
    spr.drawLine(cx, cy, cx + 4, cy - 4, COLOR_CYAN);
    spr.drawLine(cx + 4, cy - 4, cx, cy - 7, COLOR_CYAN);
    spr.drawLine(cx, cy, cx + 4, cy + 4, COLOR_CYAN);
    spr.drawLine(cx + 4, cy + 4, cx, cy + 7, COLOR_CYAN);

    // Râu bên trái
    spr.drawLine(cx - 1, cy - 4, cx - 4, cy - 1, COLOR_CYAN);
    spr.drawLine(cx - 1, cy + 4, cx - 4, cy + 1, COLOR_CYAN);

    // Chấm đèn tín hiệu online (Active Dot) góc trên
    spr.fillCircle(bx + w - 2, by + 2, 2, COLOR_GREEN);
  } else {
    // Trạng thái ngắt kết nối: Thẻ mờ xám tối, có gạch đỏ cảnh báo
    spr.fillRoundRect(bx, by, w, h, 4, COLOR_CARD_BG);
    spr.drawRoundRect(bx, by, w, h, 4, COLOR_DARK_GRAY);

    // Icon Bluetooth xám tối mờ
    spr.drawFastVLine(cx, cy - 5, 11, COLOR_DARK_GRAY);
    spr.drawLine(cx, cy, cx + 3, cy - 3, COLOR_DARK_GRAY);
    spr.drawLine(cx + 3, cy - 3, cx, cy - 5, COLOR_DARK_GRAY);
    spr.drawLine(cx, cy, cx + 3, cy + 3, COLOR_DARK_GRAY);
    spr.drawLine(cx + 3, cy + 3, cx, cy + 5, COLOR_DARK_GRAY);

    // Đường gạch chéo đỏ báo hiệu Disconnected không kết nối
    spr.drawLine(bx + 3, by + 3, bx + w - 4, by + h - 4, COLOR_RED);
    spr.drawLine(bx + 4, by + 3, bx + w - 3, by + h - 4, COLOR_RED);
  }
}

// ==========================================
// VẼ THANH TRẠNG THÁI TRÊN CÙNG (TOP BAR)
// Hiển thị: Brand TeaNavi, BLE Badge, Đồng hồ, RAM (Used/Total), FPS
// ==========================================
void drawTopBar(String timeStr, bool btConnected, float fps = 30.0) {
  sprTopBar.fillSprite(COLOR_BG);

  // Đường viền dưới kim loại mờ
  sprTopBar.drawFastHLine(0, TOP_BAR_H - 1, 320, COLOR_CARD_BORDER);

  // 1. BRAND BADGE: [TeaNavi] góc trái cực ngầu
  sprTopBar.fillRoundRect(6, 3, 62, 18, 3, COLOR_CARD_BG);
  sprTopBar.drawRoundRect(6, 3, 62, 18, 3, COLOR_TEAL);
  sprTopBar.setTextDatum(MC_DATUM);
  sprTopBar.setTextColor(COLOR_CYAN, COLOR_CARD_BG);
  sprTopBar.drawString("TeaNavi", 37, 12, 2);

  // 2. BLUETOOTH STATUS & BADGE ICON (Cực kỳ trực quan)
  int bleCx = 84;
  int bleCy = 12;
  drawBleIcon(sprTopBar, bleCx, bleCy, btConnected);

  sprTopBar.setTextDatum(ML_DATUM);
  if (btConnected) {
    sprTopBar.setTextColor(COLOR_CYAN, COLOR_BG);
    sprTopBar.drawString("ON", bleCx + 13, 12, 1);
  } else {
    sprTopBar.setTextColor(COLOR_GRAY, COLOR_BG);
    sprTopBar.drawString("OFF", bleCx + 13, 12, 1);
  }

  // 3. ĐỒNG HỒ THỜI GIAN TRUNG TÂM (Digital Clock)
  sprTopBar.setTextDatum(MC_DATUM);
  sprTopBar.setTextColor(COLOR_WHITE, COLOR_BG);
  sprTopBar.drawString(timeStr, 150, 12, 2);

  // 4. THÔNG TIN HỆ THỐNG GÓC PHẢI: RAM (Còn lại/Tổng) & FPS
  // ESP32 tổng Heap khoảng 320KB
  uint32_t freeHeapKb = ESP.getFreeHeap() / 1024;
  uint32_t totalHeapKb = ESP.getHeapSize() / 1024; // Thường là ~320K hoặc có PSRAM
  if (totalHeapKb == 0) totalHeapKb = 320; // Phòng trường hợp API trả về 0

  // Dòng trên: RAM Free / Total (VD: 298K/320K)
  sprTopBar.setTextDatum(MR_DATUM);
  sprTopBar.setTextColor(COLOR_GRAY, COLOR_BG);
  sprTopBar.drawString("RAM", 258, 7, 1);

  sprTopBar.setTextColor(COLOR_ELECTRIC_YELLOW, COLOR_BG);
  sprTopBar.drawString(String(freeHeapKb) + "K", 289, 7, 1);
  sprTopBar.setTextColor(COLOR_GRAY, COLOR_BG);
  sprTopBar.drawString("/" + String(totalHeapKb) + "K", 316, 7, 1);

  // Dòng dưới: FPS đo thực tế
  sprTopBar.setTextColor(COLOR_GRAY, COLOR_BG);
  sprTopBar.drawString("FPS", 258, 17, 1);
  sprTopBar.setTextColor(COLOR_GREEN, COLOR_BG);
  sprTopBar.drawString(String((int)fps) + " fps", 316, 17, 1);

  // Đẩy Sprite TopBar lên màn hình TFT (x=0, y=0)
  sprTopBar.pushSprite(0, 0);
}

// ==========================================
// RENDER CỤM ĐIỀU HƯỚNG BÊN TRÁI (SPRITE)
// ==========================================
void updateNavDisplay(NavDirection dir, int distance, String streetName, int roundaboutExit = 1) {
  sprNav.fillSprite(COLOR_BG);

  // Thẻ Card nền bo góc viền tinh tế
  sprNav.fillRoundRect(0, 0, 148, 206, 8, COLOR_CARD_BG);
  sprNav.drawRoundRect(0, 0, 148, 206, 8, COLOR_CARD_BORDER);

  // 1. Biểu tượng điều hướng Vector
  drawNavIcon(sprNav, 74, 46, dir, COLOR_CYAN, roundaboutExit);

  // 2. Khoảng cách (VD: 250m) hoặc "DEN NOI"
  sprNav.setTextDatum(MC_DATUM);
  if (dir == NAV_DESTINATION) {
    sprNav.setTextColor(COLOR_GREEN, COLOR_CARD_BG);
    sprNav.drawString("DEN NOI", 74, 98, 4);
  } else {
    sprNav.setTextColor(COLOR_WHITE, COLOR_CARD_BG);
    if (distance >= 1000) {
      float km = distance / 1000.0;
      sprNav.drawString(String(km, 1) + "km", 74, 98, 4);
    } else {
      sprNav.drawString(String(distance) + "m", 74, 98, 4);
    }
  }

  // 3. Thanh tiến trình khoảng cách (Distance Progress Bar)
  int barW = 120;
  int barH = 6;
  int barX = 14;
  int barY = 122;
  sprNav.fillRoundRect(barX, barY, barW, barH, 3, COLOR_DARK_GRAY);
  
  if (dir != NAV_DESTINATION) {
    int maxDist = 500;
    int fillW = map(constrain(distance, 0, maxDist), 0, maxDist, barW, 0);
    if (fillW > 0) {
      uint16_t pCol = (distance < 80) ? COLOR_ORANGE : COLOR_CYAN;
      sprNav.fillRoundRect(barX, barY, fillW, barH, 3, pCol);
    }
  } else {
    sprNav.fillRoundRect(barX, barY, barW, barH, 3, COLOR_GREEN);
  }

  // 4. Đường kẻ phân cách
  sprNav.drawFastHLine(10, 140, 128, COLOR_CARD_BORDER);

  // 5. Tên đường tiếp theo / ghi chú vòng xuyến
  sprNav.setTextDatum(MC_DATUM);
  sprNav.setTextColor(COLOR_GRAY, COLOR_CARD_BG);
  if (dir == NAV_ROUNDABOUT) {
    sprNav.drawString("LOI RA THU " + String(roundaboutExit), 74, 155, 1);
  } else if (dir == NAV_DESTINATION) {
    sprNav.drawString("DIEM DEN CUA BAN", 74, 155, 1);
  } else {
    sprNav.drawString("TIEP THEO VAO", 74, 155, 1);
  }

  sprNav.setTextColor(COLOR_ORANGE, COLOR_CARD_BG);
  sprNav.drawString(streetName, 74, 178, 2);

  // Đẩy Sprite lên màn hình TFT (tại vị trí x=6, y=28)
  sprNav.pushSprite(6, 28);
}

// ==========================================
// ==========================================
// RENDER CỤM TỐC ĐỘ SPORT ARC GAUGE (SPRITE)
// Phong cách: Cyber Racing Cockpit (Tối giản & Trọng tâm)
// ==========================================
void updateSpeedGauge(int speed, int limit) {
  sprGauge.fillSprite(COLOR_BG);

  // 1. Thẻ Card nền bên phải - Thêm viền cảnh báo khi vượt tốc độ
  sprGauge.fillRoundRect(0, 0, 154, 206, 8, COLOR_CARD_BG);
  if (speed > limit) {
    // Chớp viền cảnh báo màu đỏ/vàng thể thao khi quá tốc độ
    sprGauge.drawRoundRect(0, 0, 154, 206, 8, COLOR_RED);
    sprGauge.drawRoundRect(1, 1, 152, 204, 7, COLOR_ELECTRIC_YELLOW);
  } else {
    sprGauge.drawRoundRect(0, 0, 154, 206, 8, COLOR_CARD_BORDER);
  }

  int gcx = 77;
  int gcy = 82;
  int radius = 64;

  float startAngle = 135.0; // Bắt đầu góc 135 độ (hướng 7h30)
  float endAngle = 405.0;   // Kết thúc góc 405 độ (hướng 4h30) - cung 270 độ thể thao
  float totalSpan = endAngle - startAngle;
  float rad = 0.0174532925; // PI / 180

  // 2. Rãnh vòng cung kim loại đôi (Dual Track Ring)
  int rOuter = radius + 3;
  int rInner = radius - 13;
  for (float aDeg = startAngle; aDeg <= endAngle; aDeg += 2.0) {
    float a = aDeg * rad;
    float cosA = cos(a);
    float sinA = sin(a);
    sprGauge.drawPixel(gcx + (rOuter * cosA), gcy + (rOuter * sinA), COLOR_TRACK_RING);
    sprGauge.drawPixel(gcx + ((rOuter - 1) * cosA), gcy + ((rOuter - 1) * sinA), COLOR_DARK_GRAY);
    sprGauge.drawPixel(gcx + (rInner * cosA), gcy + (rInner * sinA), COLOR_TRACK_RING);
  }

  // 3. Hệ vạch chia cung 28 nấc (Thước đo tốc độ công tơ mét)
  int numTicks = 28;
  float angleStep = totalSpan / (numTicks - 1);
  float curSpeedRatio = (float)constrain(speed, 0, 140) / 140.0;
  int activeTicks = round(curSpeedRatio * numTicks);
  float limitRatio = (float)constrain(limit, 0, 140) / 140.0;
  int limitTick = round(limitRatio * numTicks);

  for (int i = 0; i < numTicks; i++) {
    float a = (startAngle + i * angleStep) * rad;
    float cosA = cos(a);
    float sinA = sin(a);

    // Điểm vạch chính (Major tick) mỗi 4 nấc (0, 20, 40, 60, 80, 100, 120, 140)
    bool isMajor = (i % 4 == 0);
    int tickLength = isMajor ? 12 : 8;

    int xOuter = gcx + (radius * cosA);
    int yOuter = gcy + (radius * sinA);
    int xInner = gcx + ((radius - tickLength) * cosA);
    int yInner = gcy + ((radius - tickLength) * sinA);

    // Màu vạch tĩnh mặc định
    uint16_t tickColor = isMajor ? COLOR_GRAY : COLOR_DARK_GRAY;

    // Đánh dấu mốc giới hạn tốc độ (Speed Limit Marker)
    if (i == limitTick) {
      tickColor = COLOR_RED;
    }

    // Đổi màu vạch động theo mức tốc độ hiện tại
    if (i < activeTicks) {
      if (speed > limit) {
        // Quá tốc độ: Pha màu Đỏ rực & Vàng điện chớp sáng
        tickColor = (i >= limitTick) ? COLOR_RED : COLOR_ELECTRIC_YELLOW;
      } else if (i > numTicks * 0.70) {
        // Tốc độ cao (>100km/h): Vàng điện rực
        tickColor = COLOR_ELECTRIC_YELLOW;
      } else if (i > numTicks * 0.45) {
        // Tốc độ vừa (60-100km/h): Cam Porsche
        tickColor = COLOR_AMBER;
      } else {
        // Tốc độ tiêu chuẩn (0-60km/h): Neon Cyan thể thao
        tickColor = COLOR_CYAN;
      }
    }

    sprGauge.drawLine(xInner, yInner, xOuter, yOuter, tickColor);
    if (isMajor || i < activeTicks) {
      sprGauge.drawLine(xInner + 1, yInner, xOuter + 1, yOuter, tickColor);
    }
  }

  // 4. Số đo tốc độ lớn ở tâm (Font 7 7-Segment sắc nét)
  sprGauge.setTextDatum(MC_DATUM);
  if (speed > limit) {
    sprGauge.setTextColor(COLOR_ELECTRIC_YELLOW, COLOR_CARD_BG);
  } else {
    sprGauge.setTextColor(COLOR_WHITE, COLOR_CARD_BG);
  }
  sprGauge.drawString(String(speed), gcx, gcy - 2, 7);

  // 5. Đơn vị KM/H & Thanh Badge trang trí phong cách Cockpit
  sprGauge.setTextDatum(MC_DATUM);
  sprGauge.setTextColor(speed > limit ? COLOR_RED : COLOR_CYAN, COLOR_CARD_BG);
  sprGauge.drawString("KM/H", gcx, gcy + 33, 2);

  // Vạch ngang trang trí phân vùng
  sprGauge.drawFastHLine(14, 134, 126, COLOR_CARD_BORDER);

  // 6. KHỐI BIỂN BÁO SPEED LIMIT THỂ THAO CAO CẤP (TẬP TRUNG CHÍNH DIỆN)
  int signCx = 77;
  int signCy = 172;
  int signR = 26; // Tăng nhẹ kích thước biển báo cho cân xứng và cực kỳ sắc nét

  if (speed > limit) {
    // Khi quá tốc độ: Biển phát hào quang cảnh báo đỏ - vàng
    sprGauge.drawCircle(signCx, signCy, signR + 3, COLOR_ELECTRIC_YELLOW);
    sprGauge.drawCircle(signCx, signCy, signR + 2, COLOR_RED);
    sprGauge.fillCircle(signCx, signCy, signR, COLOR_WHITE);
    sprGauge.drawCircle(signCx, signCy, signR, COLOR_RED);
    sprGauge.drawCircle(signCx, signCy, signR - 1, COLOR_RED);
    sprGauge.drawCircle(signCx, signCy, signR - 2, COLOR_RED);

    sprGauge.setTextDatum(MC_DATUM);
    sprGauge.setTextColor(0x0000, COLOR_WHITE);
    sprGauge.drawString(String(limit), signCx, signCy, 4);
  } else {
    // Trạng thái chuẩn: Huy hiệu kim loại viền đỏ sang trọng, tối giản và sạch sẽ
    sprGauge.drawCircle(signCx, signCy, signR + 2, COLOR_CARD_BORDER);
    sprGauge.fillCircle(signCx, signCy, signR, COLOR_WHITE);
    sprGauge.drawCircle(signCx, signCy, signR, COLOR_RED);
    sprGauge.drawCircle(signCx, signCy, signR - 1, COLOR_RED);
    sprGauge.drawCircle(signCx, signCy, signR - 2, COLOR_RED);

    sprGauge.setTextDatum(MC_DATUM);
    sprGauge.setTextColor(0x0000, COLOR_WHITE);
    sprGauge.drawString(String(limit), signCx, signCy, 4);
  }

  // Đẩy Sprite lên màn hình TFT (tại vị trí x=160, y=28)
  sprGauge.pushSprite(160, 28);
}

void setup() {
  Serial.begin(115200);

  // Bật đèn nền LCD chân 21
  pinMode(21, OUTPUT);
  digitalWrite(21, HIGH);

  tft.init();
  tft.invertDisplay(true); // Đảo ngược màu cho màn hình mạch vàng (ST7789 / ILI9341 inverted)
  tft.setRotation(1);      // Màn hình ngang 320x240
  tft.fillScreen(COLOR_BG);

  // Khởi tạo bộ đệm Sprite trong RAM
  sprTopBar.setColorDepth(16);
  sprTopBar.createSprite(320, TOP_BAR_H);

  sprNav.setColorDepth(16);
  sprNav.createSprite(148, 206);

  sprGauge.setColorDepth(16);
  sprGauge.createSprite(154, 206);

  // Vẽ thanh Top Bar ban đầu
  drawTopBar("09:45", true, 30.0);
}

// Cấu trúc mô phỏng kịch bản chạy thử nghiệm
struct DemoStep {
  NavDirection dir;
  int distance;
  const char* street;
  int speed;
  int limit;
  int exitNum;
  int durationMs;
};

// 12 trường hợp giao thông diễn ra thực tế trên lộ trình
DemoStep routeDemo[] = {
  {NAV_STRAIGHT,     400, "DUONG GIAI PHONG",  48, 60, 0, 2000},
  {NAV_SLIGHT_RIGHT, 250, "NHANH LEN CAO TOC", 55, 60, 0, 2000},
  {NAV_TURN_RIGHT,   120, "PHO HUE",           42, 50, 0, 2000},
  {NAV_SHARP_RIGHT,  80,  "NGO 124 NGACH 5",   22, 40, 0, 2000},
  {NAV_SLIGHT_LEFT,  300, "DUONG VEN HO",      38, 50, 0, 2000},
  {NAV_TURN_LEFT,    150, "TRAN HUNG DAO",     45, 50, 0, 2000},
  {NAV_SHARP_LEFT,   70,  "NGO NHO",           20, 30, 0, 2000},
  {NAV_UTURN_LEFT,   100, "QUAY DAU DIEM MO",  25, 40, 0, 2500},
  {NAV_UTURN_RIGHT,  90,  "QUAY DAU PHAI",     24, 40, 0, 2500},
  {NAV_FORK_RAMP,    320, "NHAP LAN VAIH 3",   65, 80, 0, 2000},
  {NAV_ROUNDABOUT,   180, "NGA 6 O CHO DUA",   35, 50, 2, 2500}, // Lối ra 2
  {NAV_ROUNDABOUT,   90,  "VONG XUYEN KHUAT D",30, 50, 3, 2500}, // Lối ra 3
  {NAV_STRAIGHT,     60,  "DUONG TO HIEN",     72, 60, 0, 2000}, // Thử quá tốc độ (72 > 60) -> Cảnh báo đỏ
  {NAV_DESTINATION,  0,   "TOA NHA HOME",       0, 50, 0, 3000}
};

int totalSteps = sizeof(routeDemo) / sizeof(routeDemo[0]);
unsigned long lastFrameTime = 0;

void loop() {
  for (int i = 0; i < totalSteps; i++) {
    DemoStep step = routeDemo[i];

    // Tính FPS thực tế giữa các khung hình
    float fps = 30.0;
    if (lastFrameTime > 0) {
      unsigned long frameDelta = millis() - lastFrameTime;
      if (frameDelta > 0) {
        fps = 1000.0 / (float)frameDelta;
      }
    }
    lastFrameTime = millis();

    // Cập nhật TopBar với TeaNavi, BLE, RAM và FPS (đã lược bỏ GPS)
    drawTopBar("09:45", true, fps);

    // Vẽ điều hướng
    updateNavDisplay(step.dir, step.distance, step.street, step.exitNum);

    // Vẽ đồng hồ tốc độ và biển báo giới hạn thể thao
    updateSpeedGauge(step.speed, step.limit);

    delay(step.durationMs);
  }
}