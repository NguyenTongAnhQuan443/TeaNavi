# TeaNavi - ESP32 Smart HUD & Navigation System

Hệ thống hiển thị thông tin dẫn đường thông minh (Head-Up Display) và đồng hồ đo tốc độ phong cách **Cyberpunk / Sport HUD** dành cho module **ESP32 Mạch Vàng (CYD - ESP32-2432S028R)**.

---

## 1. Cấu hình phần cứng (Hardware Pinout)

- **Màn hình**: 2.8 inch TFT SPI (240x320 hoặc 320x240 ngang).
- **Driver**: ILI9341 / ST7789.
- **Backlight (Đèn nền)**: GPIO 21 (`HIGH`).
- **Giao tiếp SPI**:
  - `TFT_MOSI`: GPIO 13
  - `TFT_MISO`: GPIO 12
  - `TFT_SCLK`: GPIO 14
  - `TFT_CS`: GPIO 15
  - `TFT_DC`: GPIO 2
  - `TFT_RST`: -1 (Reset chung qua chip)
- **Đặc điểm màu sắc**: Panel LCD của board sử dụng hệ màu đảo (`Inverted Color`), cần lệnh `tft.invertDisplay(true);` để hiển thị màu nền đen sâu (Deep Dark) và màu neon chuẩn xác.

---

## 2. Hệ thống quy chuẩn điều hướng giao thông (12 Navigation Rules)

Hệ thống biểu tượng điều hướng dạng Vector được chuẩn hoá tương thích với tín hiệu API từ Google Maps / Mapbox / OSRM Navigation:

| Mã số (`enum`) | Tên trường hợp | Mô tả hiển thị đồ hoạ | Tình huống áp dụng thực tế |
| :---: | :--- | :--- | :--- |
| `0` | **`NAV_STRAIGHT`** | Mũi tên thẳng đứng dài | Tiếp tục đi thẳng trên tuyến đường chính |
| `1` | **`NAV_SLIGHT_RIGHT`** | Mũi tên nghiêng chếch 45° bên phải | Rẽ nhẹ phải, đi vào đường gom hoặc tách làn nhẹ |
| `2` | **`NAV_TURN_RIGHT`** | Mũi tên vuông góc 90° sang phải | Rẽ phải tại ngã ba, ngã tư |
| `3` | **`NAV_SHARP_RIGHT`** | Mũi tên bẻ ngược gắt >120° phải | Rẽ ngoặt gắt, vào hẻm/ngõ hẹp cua sâu |
| `4` | **`NAV_SLIGHT_LEFT`** | Mũi tên nghiêng chếch 45° bên trái | Rẽ nhẹ trái, đi theo nhánh đường uốn cong trái |
| `5` | **`NAV_TURN_LEFT`** | Mũi tên vuông góc 90° sang trái | Rẽ trái tại ngã ba, ngã tư có đèn tín hiệu |
| `6` | **`NAV_SHARP_LEFT`** | Mũi tên bẻ ngược gắt >120° trái | Rẽ ngoặt gắt vào đường nhánh cua gấp |
| `7` | **`NAV_UTURN_LEFT`** | Vòng cung chữ U ngược hướng trái | Quay đầu xe bên dải phân cách mở (Chuẩn VN) |
| `8` | **`NAV_UTURN_RIGHT`** | Vòng cung chữ U ngược hướng phải | Quay đầu xe bên phải theo biển báo chỉ định |
| `9` | **`NAV_FORK_RAMP`** | Nhánh chạc ba phân làn cao tốc | Đi vào đường nhánh dẫn lên/xuống cao tốc |
| `10` | **`NAV_ROUNDABOUT`** | Vòng tròn bùng binh + số lối ra | Đi vào vòng xuyến (hiển thị số lối ra 1, 2, 3...) |
| `11` | **`NAV_DESTINATION`** | Biểu tượng Pin đích đến / Cờ đỏ | Xe đã đến địa chỉ đích |

---

## 3. Các thành phần giao diện trên màn hình (HUD Layout)

Màn hình ngang kích thước **320 x 240 pixel**, chia thành 3 khu vực chính:

```
+-------------------------------------------------------------+
| [BLE STATUS]           10:30                [GPS STATUS]    |  <- Top Bar (h=24px)
+------------------------------+------------------------------+
|       NAVIGATION CARD        |       SPORT ARC GAUGE        |
|                              |                              |
|          [ICON]              |            /-----\           |
|                              |           /       \          |
|           250m               |          |   65    |         |
|    [===== Progress =====]    |          |  KM/H   |         |
|   -----------------------    |           \       /          |
|        TIEP THEO VAO         |            \-----/           |
|        NGUYEN TRAI           |           ( 60 ) MAX         |
+------------------------------+------------------------------+
```

### 3.1. Thanh trạng thái trên cùng (Top Bar)
- **Bluetooth BLE**: Hiển thị tình trạng kết nối tới điện thoại / GPS module (`BLE CONNECTED` màu Cyan / `BLE SEARCHING...` màu Xám).
- **Clock**: Đồng hồ thời gian thực ở chính giữa.
- **GPS Fix**: Trạng thái tín hiệu vệ tinh (`GPS 3D FIX` màu Xanh lá / `GPS ACQUIRING` màu Cam).

### 3.2. Cụm điều hướng bên trái (Navigation Card - Double Buffering Sprite)
- **Vector Nav Icon**: Biểu tượng mũi tên đồ hoạ sắc nét hiển thị 1 trong 12 quy chuẩn ở mục 2.
- **Khoảng cách**: Hiển thị dạng `m` (mét) hoặc `km` (khi khoảng cách $\ge$ 1000m).
- **Thanh tiến trình (Distance Progress Bar)**: Đầy dần/hao dần khi khoảng cách rút ngắn lại; tự động đổi sang màu Cam cảnh báo khi xe cách điểm rẽ dưới 80m.
- **Tên đường tiếp theo**: Hiển thị tên tuyến phố sắp rẽ vào, hoặc ghi chú cụ thể lối ra ở bùng binh (`LOI RA THU 2`).

### 3.3. Cụm đồng hồ công tơ mét bên phải (Sport Arc Gauge - Double Buffering Sprite)
- **Hệ thống 22 vạch LED vòng cung**: Gradient màu từ Xanh Neon $\rightarrow$ Cam Thể thao.
- **Cảnh báo vượt tốc**: Khi tốc độ xe lớn hơn tốc độ giới hạn của tuyến đường (`speed > limit`), toàn bộ cụm số đo và dải LED sẽ chớp **Đỏ Cảnh Báo**.
- **Biển báo tốc độ giới hạn (Speed Limit Sign)**: Thiết kế dạng hình tròn viền đỏ nền trắng chuẩn quốc tế đặt ngay bên dưới đồng hồ.

---

## 4. Hướng dẫn biên dịch và nạp code (Build & Upload)

Sử dụng PlatformIO CLI:

```powershell
# Biên dịch mã nguồn
pio run

# Nạp vào mạch ESP32 qua cổng USB COM
pio run -t upload

# Mở Serial Monitor xem log
pio run -t monitor
```
