#include <Arduino.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include <math.h>

TFT_eSPI tft = TFT_eSPI();

// Sử dụng TFT_eSprite để render chống nháy (Flicker-free Double Buffering)
TFT_eSprite sprGauge = TFT_eSprite(&tft); // Sprite cho cụm tốc độ & gauge
TFT_eSprite sprNav   = TFT_eSprite(&tft); // Sprite cho cụm điều hướng (icon, khoảng cách, thanh progress)

// Định nghĩa màu sắc phong cách OLED Deep Black / Sport Supercar Cockpit
#define COLOR_BG          0x0000  // Đen tuyền tuyệt đối (True Black) để tăng độ tương phản cực đại trên LCD
#define COLOR_CARD_BG     0x0842  // Xám than titan tối cao cấp (Deep Titan Charcoal #101014)
#define COLOR_CARD_BORDER 0x2125  // Viền sợi carbon / kim loại mờ tinh tế
#define COLOR_CYAN            0x07FF  // Neon Cyan rực rỡ #00FFFF
#define COLOR_TEAL            0x03B3  // Deep Teal
#define COLOR_ORANGE          0xFD20  // Neon Orange #FFA500
#define COLOR_AMBER           0xFDC0  // Amber Porsche #FFA000
#define COLOR_ELECTRIC_YELLOW 0xFFE0  // Vàng điện Neon siêu sáng #FFFF00
#define COLOR_TRACK_RING      0x10A2  // Rãnh kim loại nòng súng Gunmetal 3D
#define COLOR_RED             0xF800  // Red cảnh báo #FF0000
#define COLOR_GREEN           0x07E0  // Neon Green #00FF00
#define COLOR_WHITE           0xFFFF  // White
#define COLOR_GRAY            0x632C  // Bạc ánh kim (Muted Silver)
#define COLOR_DARK_GRAY       0x2104  // Xám thép tối (Dark Steel)

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
// VẼ ICON ĐIỀU HƯỚNG BẰNG VECTOR HÌNH HỌC CHUẨN XÁC TUYỆT ĐỐI
// ==========================================
void drawNavIcon(TFT_eSprite &spr, int cx, int cy, NavDirection dir, uint16_t color, int exitNum = 1) {
  switch (dir) {
    case NAV_STRAIGHT:
      // Đi thẳng: Thon gọn, cao ráo
      spr.fillRoundRect(cx - 4, cy - 8, 8, 38, 2, color);
      spr.fillTriangle(cx, cy - 28, cx - 18, cy - 8, cx + 18, cy - 8, color);
      break;

    case NAV_SLIGHT_RIGHT: {
      // Chếch phải 45 độ: Thân thon 8px, dài cao thanh thoát
      spr.fillRect(cx - 9, cy + 6, 8, 20, color);
      for (int offset = 0; offset <= 7; offset++) {
        spr.drawLine(cx - 9 + offset, cy + 6, cx + 13 + offset, cy - 16, color);
      }
      spr.fillTriangle(cx + 26, cy - 22, cx + 8, cy - 22, cx + 26, cy - 4, color);
      break;
    }

    case NAV_SLIGHT_LEFT: {
      // Chếch trái 45 độ: Thân thon 8px
      spr.fillRect(cx + 1, cy + 6, 8, 20, color);
      for (int offset = 0; offset <= 7; offset++) {
        spr.drawLine(cx + 9 - offset, cy + 6, cx - 13 - offset, cy - 16, color);
      }
      spr.fillTriangle(cx - 26, cy - 22, cx - 8, cy - 22, cx - 26, cy - 4, color);
      break;
    }

    case NAV_TURN_RIGHT:
      // Rẽ phải 90 độ: Thon gọn 8px, thân cao ráo thanh thoát
      // Thân đứng cao 38px, dày 8px
      spr.fillRoundRect(cx - 15, cy - 12, 8, 38, 2, color);
      // Nhánh ngang sang phải: dày 8px, dài 28px
      spr.fillRoundRect(cx - 15, cy - 12, 28, 8, 2, color);
      // Đầu mũi tên thanh mảnh sắc nhọn
      spr.fillTriangle(cx + 26, cy - 8, cx + 11, cy - 21, cx + 11, cy + 5, color);
      break;

    case NAV_TURN_LEFT:
      // Rẽ trái 90 độ: Thon gọn 8px, thân cao ráo
      spr.fillRoundRect(cx + 7, cy - 12, 8, 38, 2, color);
      spr.fillRoundRect(cx - 13, cy - 12, 28, 8, 2, color);
      spr.fillTriangle(cx - 26, cy - 8, cx - 11, cy - 21, cx - 11, cy + 5, color);
      break;

    case NAV_SHARP_RIGHT: {
      // 3: Rẽ gắt phải / Cua tay áo (Hairpin Right ~135 độ, khác biệt hoàn toàn với U-Turn)
      // Thân đi thẳng lên từ dưới bên trái
      spr.fillRoundRect(cx - 14, cy - 14, 8, 40, 2, color);
      // Khúc gập cua gắt nhọn hướng chếch góc 5 giờ
      for (int i = 0; i <= 7; i++) {
        spr.drawLine(cx - 10 + i, cy - 14, cx + 12 + i, cy + 10, color);
      }
      // Mũi tên nhọn hướng chếch chéo xuống góc 4h30 - 5h
      spr.fillTriangle(cx + 20, cy + 20, cx + 6, cy + 10, cx + 22, cy + 4, color);
      break;
    }

    case NAV_SHARP_LEFT: {
      // 6: Rẽ gắt trái / Cua tay áo (Hairpin Left ~135 độ)
      // Thân đi thẳng lên từ dưới bên phải
      spr.fillRoundRect(cx + 6, cy - 14, 8, 40, 2, color);
      // Khúc gập cua gắt nhọn hướng chếch góc 7 giờ
      for (int i = 0; i <= 7; i++) {
        spr.drawLine(cx + 10 - i, cy - 14, cx - 12 - i, cy + 10, color);
      }
      // Mũi tên nhọn hướng chếch chéo xuống góc 7h - 7h30
      spr.fillTriangle(cx - 20, cy + 20, cx - 6, cy + 10, cx - 22, cy + 4, color);
      break;
    }

    case NAV_UTURN_LEFT: {
      // 7: Quay đầu xe bên trái (Chuẩn CarPlay / Google Maps)
      // Tâm vòm uốn cong:
      int archX = cx - 2;
      int archY = cy - 6;

      // 1. Vẽ vòm cung nửa trên hoàn mỹ
      spr.fillCircle(archX, archY, 15, color);
      spr.fillCircle(archX, archY, 7, COLOR_CARD_BG);
      // Xoá sạch nửa dưới của vòm (chỉ trong phạm vi giữa 2 nhánh, không chạm vào thân nhánh)
      spr.fillRect(archX - 6, archY, 13, 20, COLOR_CARD_BG);

      // 2. Nhánh phải đi lên (từ dưới vươn thẳng nối mượt khít vào vòm phải)
      spr.fillRect(archX + 7, archY, 8, 28, color);

      // 3. Nhánh trái đi xuống (nối mượt khít từ vòm trái xuống mũi tên)
      spr.fillRect(archX - 15, archY, 8, 18, color);

      // 4. Mũi tên nhọn chỉ thẳng xuống 6 giờ
      spr.fillTriangle(archX - 11, archY + 28, archX - 21, archY + 16, archX - 1, archY + 16, color);
      break;
    }

    case NAV_UTURN_RIGHT: {
      // 8: Quay đầu xe bên phải (Đối xứng tuyệt đối với U-Turn Left)
      int archX = cx + 2;
      int archY = cy - 6;

      // 1. Vẽ vòm cung nửa trên hoàn mỹ
      spr.fillCircle(archX, archY, 15, color);
      spr.fillCircle(archX, archY, 7, COLOR_CARD_BG);
      // Xoá sạch nửa dưới của vòm (chỉ trong phạm vi giữa 2 nhánh)
      spr.fillRect(archX - 6, archY, 13, 20, COLOR_CARD_BG);

      // 2. Nhánh trái đi lên (vươn thẳng nối khít vào vòm trái)
      spr.fillRect(archX - 15, archY, 8, 28, color);

      // 3. Nhánh phải đi xuống (nối khít từ vòm phải xuống mũi tên)
      spr.fillRect(archX + 7, archY, 8, 18, color);

      // 4. Mũi tên nhọn chỉ thẳng xuống 6 giờ
      spr.fillTriangle(archX + 11, archY + 28, archX + 1, archY + 16, archX + 21, archY + 16, color);
      break;
    }

    case NAV_FORK_RAMP: {
      // 9: Nhánh cao tốc / Tách làn (Thon mượt 8px)
      // Thân chính thẳng
      spr.fillRoundRect(cx - 10, cy - 8, 8, 38, 2, color);
      spr.fillTriangle(cx - 6, cy - 26, cx - 18, cy - 8, cx + 6, cy - 8, color);
      // Nhánh phụ tách sang phải
      for (int offset = 0; offset <= 6; offset++) {
        spr.drawLine(cx - 8 + offset, cy + 10, cx + 14 + offset, cy - 10, color);
      }
      spr.fillTriangle(cx + 26, cy - 14, cx + 10, cy - 14, cx + 24, cy + 2, color);
      break;
    }

    case NAV_ROUNDABOUT: {
      // 10: Vòng xuyến / Bùng binh cao cấp chuẩn Apple CarPlay & Google Maps
      int rcx = cx;
      int rcy = cy - 2;
      int rTrack = 20;

      // 1. Đường ray bùng binh (Đường nét mờ Graphite tinh tế định hình vòng xuyến)
      spr.drawCircle(rcx, rcy, rTrack, 0x2965);
      spr.drawCircle(rcx, rcy, rTrack - 1, 0x18E3);
      spr.drawCircle(rcx, rcy, rTrack + 1, 0x18E3);

      // Các nhánh đường phụ mờ xung quanh (biểu thị các lối rẽ khác)
      spr.fillRect(rcx - 2, rcy + rTrack - 1, 4, 12, 0x2965); // Lối vào dưới
      spr.fillRect(rcx + rTrack - 1, rcy - 2, 10, 4, 0x2965); // Nhánh phụ 3h
      spr.fillRect(rcx - 2, rcy - rTrack - 9, 4, 10, 0x2965); // Nhánh phụ 12h
      spr.fillRect(rcx - rTrack - 9, rcy - 2, 10, 4, 0x2965); // Nhánh phụ 9h

      // 2. Quỹ đạo xe di chuyển thực tế (Active Trajectory - Màu Cyan sáng sắc nét)
      // Đường xe vào từ phía dưới
      spr.fillRoundRect(rcx - 3, rcy + 10, 6, 20, 2, color);

      // Cung đường ôm cua bùng binh (Dày 6px sáng bóng)
      if (exitNum == 1) {
        // Lối ra 1 (3h): Ôm 1/4 cung tròn từ góc 6h lên 3h
        for (int r = rTrack - 3; r <= rTrack + 3; r++) {
          for (int deg = 0; deg <= 90; deg += 3) {
            float rad = deg * 0.0174533;
            int px = rcx + (int)(r * sin(rad));
            int py = rcy + (int)(r * cos(rad));
            spr.drawPixel(px, py, color);
          }
        }
        // Nhánh thoát rẽ phải 3h vươn dài thanh thoát ra ngoài
        spr.fillRect(rcx + 10, rcy - 3, 16, 6, color);
        // Mũi tên nhọn vươn xa ở đầu nhánh (đỉnh x = rcx + 36)
        spr.fillTriangle(rcx + 36, rcy, rcx + 24, rcy - 8, rcx + 24, rcy + 8, color);
      } 
      else if (exitNum == 2) {
        // Lối ra 2 (12h - đi thẳng qua bùng binh): Chạy nửa vòng cung bên phải từ 6h lên 12h
        for (int r = rTrack - 3; r <= rTrack + 3; r++) {
          for (int deg = 0; deg <= 180; deg += 3) {
            float rad = deg * 0.0174533;
            int px = rcx + (int)(r * sin(rad));
            int py = rcy + (int)(r * cos(rad));
            spr.drawPixel(px, py, color);
          }
        }
        // Nhánh thoát vươn cao lên trên (dài 18px vượt xa khỏi vành bùng binh)
        spr.fillRect(rcx - 3, rcy - 30, 6, 16, color);
        // Mũi tên nhọn vươn cao thanh thoát (đỉnh y = rcy - 36)
        spr.fillTriangle(rcx, rcy - 36, rcx - 9, rcy - 24, rcx + 9, rcy - 24, color);
      } 
      else {
        // Lối ra 3 (9h - ôm 3/4 vòng xuyến từ 6h -> 3h -> 12h -> 9h)
        for (int r = rTrack - 3; r <= rTrack + 3; r++) {
          for (int deg = 0; deg <= 270; deg += 3) {
            float rad = deg * 0.0174533;
            int px = rcx + (int)(r * sin(rad));
            int py = rcy + (int)(r * cos(rad));
            spr.drawPixel(px, py, color);
          }
        }
        // Nhánh thoát rẽ trái 9h vươn dài ra ngoài
        spr.fillRect(rcx - 26, rcy - 3, 16, 6, color);
        // Mũi tên nhọn vươn xa (đỉnh x = rcx - 36)
        spr.fillTriangle(rcx - 36, rcy, rcx - 24, rcy - 8, rcx - 24, rcy + 8, color);
      }

      // 3. Huy hiệu tâm hiện đại: Vòng tròn tối giản với số lối ra tương phản cao
      spr.fillCircle(rcx, rcy, 9, COLOR_CARD_BG);
      spr.drawCircle(rcx, rcy, 9, 0x3186); // Viền titan thanh lịch
      spr.setTextDatum(MC_DATUM);
      spr.setTextColor(COLOR_ELECTRIC_YELLOW, COLOR_CARD_BG);
      spr.drawString(String(exitNum), rcx, rcy, 2);
      break;
    }

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
// ==========================================
// RENDER CỤM ĐIỀU HƯỚNG BÊN TRÁI (SPRITE)
// Phong cách: Porsche GT / Cyber Cyan
// ==========================================
void updateNavDisplay(NavDirection dir, int distance, String streetName, int roundaboutExit = 1) {
  sprNav.fillSprite(COLOR_BG);

  // Thẻ Card nền bo góc viền kép tinh xảo phong cách Cockpit
  sprNav.fillRoundRect(0, 0, 148, 206, 8, COLOR_CARD_BG);
  sprNav.drawRoundRect(0, 0, 148, 206, 8, COLOR_CARD_BORDER);

  // 1. BIỂU TƯỢNG ĐIỀU HƯỚNG (Màu Neon Cyan viền kép thể thao)
  // Vẽ lớp hào quang bóng mờ viền sâu (Teal glow)
  drawNavIcon(sprNav, 74, 46, dir, COLOR_CYAN, roundaboutExit);

  // 2. KHOẢNG CÁCH (SỐ MÀU TRẮNG LỚN NỔI BẬT & DỄ ĐỌC KHI ĐANG LÁI XE)
  sprNav.setTextDatum(MC_DATUM);
  if (dir == NAV_DESTINATION) {
    sprNav.setTextColor(COLOR_GREEN, COLOR_CARD_BG);
    sprNav.drawString("DEN NOI", 74, 98, 4);
  } else {
    // Số khoảng cách màu TRẮNG tinh khiết, font 4 to và cực kỳ rõ ràng
    sprNav.setTextColor(COLOR_WHITE, COLOR_CARD_BG);
    if (distance >= 1000) {
      float km = distance / 1000.0;
      sprNav.drawString(String(km, 1) + " km", 74, 98, 4);
    } else {
      sprNav.drawString(String(distance) + " m", 74, 98, 4);
    }
  }

  // 3. THANH TIẾN TRÌNH PHÂN ĐOẠN THỂ THAO (Segmented Tech Bar 10 vạch)
  int numSegments = 10;
  int segW = 10;
  int segH = 5;
  int segGap = 3;
  int startBarX = (148 - (numSegments * segW + (numSegments - 1) * segGap)) / 2;
  int barY = 120;

  int maxDist = 500;
  int activeSegments = 0;
  if (dir != NAV_DESTINATION) {
    float prog = 1.0 - constrain((float)distance / maxDist, 0.0, 1.0);
    activeSegments = round(prog * numSegments);
  } else {
    activeSegments = numSegments;
  }

  for (int s = 0; s < numSegments; s++) {
    int sx = startBarX + s * (segW + segGap);
    if (s < activeSegments) {
      uint16_t segCol = (distance < 80) ? COLOR_ORANGE : COLOR_CYAN;
      if (dir == NAV_DESTINATION) segCol = COLOR_GREEN;
      sprNav.fillRoundRect(sx, barY, segW, segH, 1, segCol);
    } else {
      sprNav.fillRoundRect(sx, barY, segW, segH, 1, COLOR_DARK_GRAY);
    }
  }

  // 4. ĐƯỜNG KẺ KIM LOẠI PHÂN VÙNG
  sprNav.drawFastHLine(14, 138, 120, COLOR_CARD_BORDER);

  // 5. NHÃN CHỈ HƯỚNG & TÊN ĐƯỜNG TIẾP THEO
  sprNav.setTextDatum(MC_DATUM);
  sprNav.setTextColor(COLOR_GRAY, COLOR_CARD_BG);
  if (dir == NAV_ROUNDABOUT) {
    sprNav.drawString("LOI RA THU " + String(roundaboutExit), 74, 154, 1);
  } else if (dir == NAV_DESTINATION) {
    sprNav.drawString("DIEM DEN CUA BAN", 74, 154, 1);
  } else {
    sprNav.drawString("TIEP THEO VAO", 74, 154, 1);
  }

  // Tên đường: Màu Vàng Amber thể thao nổi bật sắc nét
  sprNav.setTextColor(COLOR_AMBER, COLOR_CARD_BG);
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

  // In danh mục lệnh ra Serial Console để người dùng nhập test
  Serial.println("\n=======================================================");
  Serial.println("  TEANAVI - BAN PHIM DIEU KHIEN HUD TEST ENUM");
  Serial.println("=======================================================");
  Serial.println("Nhap so (0 - 11) de hien thi tung truong hop dieu huong:");
  Serial.println("  0 : NAV_STRAIGHT     (Di thang)");
  Serial.println("  1 : NAV_SLIGHT_RIGHT (Chech phai 45 deg)");
  Serial.println("  2 : NAV_TURN_RIGHT   (Re phai 90 deg)");
  Serial.println("  3 : NAV_SHARP_RIGHT  (Re gat phai >120 deg)");
  Serial.println("  4 : NAV_SLIGHT_LEFT  (Chech trai 45 deg)");
  Serial.println("  5 : NAV_TURN_LEFT    (Re trai 90 deg)");
  Serial.println("  6 : NAV_SHARP_LEFT   (Re gat trai >120 deg)");
  Serial.println("  7 : NAV_UTURN_LEFT   (Quay dau ben trai)");
  Serial.println("  8 : NAV_UTURN_RIGHT  (Quay dau ben phai)");
  Serial.println("  9 : NAV_FORK_RAMP    (Tach lan cao toc)");
  Serial.println("  10: NAV_ROUNDABOUT   (Vong xuyen / Bung binh)");
  Serial.println("  11: NAV_DESTINATION  (Diem den cua ban)");
  Serial.println("-------------------------------------------------------");
  Serial.println("Lenh khac:");
  Serial.println("  'a' hoac 'auto'  : Chuyen sang che do tu dong chay demo");
  Serial.println("  's <toc_do>'     : Chinh toc do (VD: 's 85')");
  Serial.println("  'l <gioi_han>'   : Chinh limit (VD: 'l 60')");
  Serial.println("  'b'              : Bat / Tat ket noi Bluetooth (Toggle BLE)");
  Serial.println("=======================================================\n");
}

// Danh sách tên đường mô phỏng cho từng case
const char* navNames[] = {
  "DUONG GIAI PHONG",
  "NHANH LEN CAO TOC",
  "PHO HUE",
  "NGO 124 NGACH 5",
  "DUONG VEN HO",
  "TRAN HUNG DAO",
  "NGO NHO",
  "QUAY DAU DIEM MO",
  "QUAY DAU PHAI",
  "NHAP LAN VANH DAI 3",
  "VONG XUYEN O CHO DUA",
  "TOA NHA HOME"
};

bool autoMode = false; // Mặc định ở chế độ chỉnh bằng tay qua Console
int currentNav = 0;
int currentDist = 250;
int currentSpeed = 48;
int currentLimit = 60;
int currentExit = 2;
bool currentBle = true;

int autoStep = 0;
unsigned long lastAutoTime = 0;
unsigned long lastFrameTime = 0;

void handleSerialCommand() {
  if (!Serial.available()) return;

  String cmd = Serial.readStringUntil('\n');
  cmd.trim();
  if (cmd.length() == 0) return;

  if (cmd.equalsIgnoreCase("a") || cmd.equalsIgnoreCase("auto")) {
    autoMode = !autoMode;
    Serial.println(autoMode ? ">> CHE DO: TU DONG CHAY DEMO (AUTO)" : ">> CHE DO: THU CONG (MANUAL - Nhap 0-11)");
    return;
  }

  if (cmd.equalsIgnoreCase("b")) {
    currentBle = !currentBle;
    Serial.println(currentBle ? ">> BLE: CONNECTED (ON)" : ">> BLE: DISCONNECTED (OFF)");
    return;
  }

  if (cmd.startsWith("s ") || cmd.startsWith("S ")) {
    currentSpeed = cmd.substring(2).toInt();
    Serial.printf(">> SET TOC DO: %d km/h\n", currentSpeed);
    return;
  }

  if (cmd.startsWith("l ") || cmd.startsWith("L ")) {
    currentLimit = cmd.substring(2).toInt();
    Serial.printf(">> SET GIOI HAN (LIMIT): %d km/h\n", currentLimit);
    return;
  }

  // Nếu là số 0 -> 11
  int val = cmd.toInt();
  if (val >= 0 && val <= 11) {
    autoMode = false;
    currentNav = val;
    Serial.printf(">> Chon Nav [%d]: %s\n", currentNav, navNames[currentNav]);
  } else {
    Serial.println(">> Lenh khong hop le! Hay nhap so tu 0 den 11.");
  }
}

void loop() {
  handleSerialCommand();

  unsigned long now = millis();

  // Tính FPS thực tế
  float fps = 30.0;
  if (lastFrameTime > 0) {
    unsigned long delta = now - lastFrameTime;
    if (delta > 0) fps = 1000.0 / (float)delta;
  }
  lastFrameTime = now;

  if (autoMode) {
    // Chế độ tự động chuyển mỗi 2 giây
    if (now - lastAutoTime > 2000) {
      lastAutoTime = now;
      autoStep = (autoStep + 1) % 12;
      currentNav = autoStep;
    }
  }

  // Cập nhật TopBar (Brand, BLE, RAM, FPS)
  drawTopBar("09:45", currentBle, fps);

  // Cập nhật cụm Điều hướng (Icon Vector, Khoảng cách, Tên đường)
  updateNavDisplay((NavDirection)currentNav, currentDist, navNames[currentNav], currentExit);

  // Cập nhật cụm Đồng hồ tốc độ & Biển báo giới hạn
  updateSpeedGauge(currentSpeed, currentLimit);

  delay(60); // Tần số quét 15-20 FPS mượt mà và nhận lệnh Serial nhạy
}