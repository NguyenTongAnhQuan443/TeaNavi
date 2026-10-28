#include <Arduino.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include <math.h>

TFT_eSPI tft = TFT_eSPI(); 

// Các thông số của mặt đồng hồ
int cx = 230; // Tọa độ X tâm đồng hồ (Lệch sang phải)
int cy = 130; // Tọa độ Y tâm đồng hồ 
int r = 60;   // Chiều dài kim

int oldSpeed = -1; // Biến nhớ tốc độ cũ để xóa kim

// Hàm vẽ kim đồng hồ quay mượt mà
void drawNeedle(int speed) {
  if (oldSpeed == speed) return; // Nếu tốc độ không đổi thì không cần vẽ lại

  // 1. Tính toán góc quay (Từ 225 độ cho 0km/h đến -45 độ cho 120km/h)
  // Dùng float để kim di chuyển mượt
  float oldAngle = map(oldSpeed, 0, 120, 225, -45); 
  float newAngle = map(speed, 0, 120, 225, -45);

  float rad_per_deg = 0.0174532925; // Pi/180

  // 2. Xóa kim cũ (Bằng cách vẽ đè tam giác màu đen)
  if (oldSpeed != -1) {
    float oldRad = oldAngle * rad_per_deg;
    int oldNx = cx + (r * cos(oldRad));
    int oldNy = cy - (r * sin(oldRad));
    // Tính 2 điểm ở gốc kim (vuông góc với hướng kim)
    int oldBx1 = cx + (4 * cos(oldRad + 1.5707)); 
    int oldBy1 = cy - (4 * sin(oldRad + 1.5707));
    int oldBx2 = cx + (4 * cos(oldRad - 1.5707));
    int oldBy2 = cy - (4 * sin(oldRad - 1.5707));
    tft.fillTriangle(oldBx1, oldBy1, oldBx2, oldBy2, oldNx, oldNy, TFT_BLACK);
  }

  // 3. Vẽ kim mới (Tam giác màu Đỏ)
  float newRad = newAngle * rad_per_deg;
  int nx = cx + (r * cos(newRad));
  int ny = cy - (r * sin(newRad));
  int bx1 = cx + (4 * cos(newRad + 1.5707));
  int by1 = cy - (4 * sin(newRad + 1.5707));
  int bx2 = cx + (4 * cos(newRad - 1.5707));
  int by2 = cy - (4 * sin(newRad - 1.5707));
  
  tft.fillTriangle(bx1, by1, bx2, by2, nx, ny, TFT_RED);

  // 4. Vẽ số hiển thị digital bên dưới tâm kim
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(3);
  tft.drawNumber(speed, cx, cy + 20);

  // 5. Vẽ con ốc cố định kim ở giữa
  tft.fillCircle(cx, cy, 6, TFT_WHITE);
  tft.drawCircle(cx, cy, 6, TFT_RED);

  oldSpeed = speed; // Lưu lại
}

void setup() {
  Serial.begin(115200);
  
  // Bật đèn nền
  pinMode(21, OUTPUT);
  digitalWrite(21, HIGH);

  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);

  // === VẼ GIAO DIỆN TĨNH BÊN TRÁI ===
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(3);
  tft.setCursor(10, 40);
  tft.print("^ DI THANG");

  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setTextSize(4);
  tft.setCursor(10, 110);
  tft.print("150m");

  // === VẼ MẶT ĐỒNG HỒ BÊN PHẢI ===
  tft.drawCircle(cx, cy, 75, TFT_DARKCYAN);
  tft.drawCircle(cx, cy, 73, TFT_CYAN);
  
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  tft.setTextSize(2);
  tft.drawString("km/h", cx, cy + 45);
}

void loop() {
  // GIẢ LẬP XE CHẠY TRÊN ĐƯỜNG
  
  // 1. Vít ga từ từ lên 90km/h
  for (int i = 0; i <= 90; i++) {
    drawNeedle(i);
    delay(15);
  }
  delay(1000); // Giữ ga 1 giây

  // 2. Giảm ga về 40km/h
  for (int i = 90; i >= 40; i--) {
    drawNeedle(i);
    delay(20);
  }
  delay(1000); // Chạy tà tà 1 giây

  // 3. Vít ga bứt tốc lên 120km/h
  for (int i = 40; i <= 120; i++) {
    drawNeedle(i);
    delay(10);
  }
  delay(500);

  // 4. Phanh gấp về 0
  for (int i = 120; i >= 0; i-=2) { // Nhảy bước 2 để phanh nhanh hơn
    drawNeedle(i);
    delay(5);
  }
  delay(2000); // Dừng chờ đèn đỏ 2 giây rồi chạy lại
}