# Tài Liệu Kỹ Thuật BMI270: Nguyên Lý Hoạt Động, Hướng Dẫn Sử Dụng & Hiệu Chỉnh Tham Số

> **Tài liệu gốc đối chiếu:** Bosch Sensortec – *BMI270 Data sheet: 6-axis, smart, low-power Inertial Measurement Unit for high-performance applications* (Document number: `BST-BMI270-DS000-05`, Revision `1.3`, Release date: Nov 2020, Tổng số trang: 162).

## 1. Nguyên Lý Hoạt Động & Kiến Trúc Hệ Thống

### 1.1. Tổng quan và cấu hình hệ thống
**BMI270** là bộ đo lường quán tính (IMU) 6 trục công suất thấp, tích hợp cảm biến gia tốc 3 trục (triaxial accelerometer) 16-bit và con quay hồi chuyển 3 trục (triaxial gyroscope) 16-bit cùng bộ xử lý tính năng chuyển động thông minh trên chip (*Basic Description, Trang 2; Mục 4.1 System Configurations, Trang 25*):
* **Cảm biến gia tốc (Accelerometer):** Đo phương, chiều và độ lớn của tổng hợp lực tác dụng lên cảm biến. Khi rơi tự do (free fall), cảm biến xuất giá trị `0`; khi đứng yên trên mặt phẳng ngang (vector trọng trường hướng từ trên xuống), trục Z xuất giá trị `+1g` (*Mục 4.1, Trang 25; Mục 8.2 Sensing axis orientation, Trang 155–156*).
* **Con quay hồi chuyển (Gyroscope):** Đo vận tốc góc quay quanh 3 trục $\Omega_X, \Omega_Y, \Omega_Z$ (theo quy tắc bàn tay phải) và xuất giá trị `0` khi đứng yên (*Mục 4.1, Trang 25; Mục 8.2, Trang 155*).
* **Giao tiếp đa luồng độc lập:**
  * **Giao tiếp chính (Primary Interface):** Đóng vai trò Slave kết nối với vi điều khiển/vi xử lý trung tâm (Host/AP) qua giao thức **SPI** (3-wire hoặc 4-wire, tối đa 10 MHz) hoặc **I²C** (tối đa 1 MHz) (*Basic Description, Trang 2; Mục 4.1, Trang 25; Mục 6.1–6.5, Trang 131–138*).
  * **Giao tiếp phụ (Secondary Interface):** Có thể cấu hình độc lập làm **I²C Master (AUX I/F)** tốc độ 400 kHz để đọc đồng bộ cảm biến phụ bên ngoài (như từ kế 3 trục BMM150 tạo thành hệ 9-DoF) hoặc làm **SPI Slave (OIS I/F)** xuất dữ liệu độ trễ cực thấp cho bộ chống rung quang học camera (*Mục 4.1, Trang 25; Mục 4.10–4.11, Trang 60–65; Mục 6.6, Trang 142–143*).

### 1.2. Sơ đồ khối và chuỗi xử lý tín hiệu (Signal Path)
Theo sơ đồ khối nội bộ (*Mục 4.2 Block Diagram, Trang 26*), tín hiệu từ hai khối cảm biến MEMS (Accel và Gyro) đi qua chuỗi xử lý phần cứng tuần tự như sau:
1. **Chuyển đổi tương tự - số (ADC) & Bù nội (Compensation):** Tín hiệu cơ điện tử từ MEMS được ADC 16-bit số hóa và đưa qua khối bù nhiệt/cơ học cấp phần cứng. Tại đầu ra khối này, dữ liệu thô trước lọc (pre-filtered data) có tốc độ lấy mẫu cố định là **1.6 kHz** đối với Accelerometer và **6.4 kHz** đối với Gyroscope (*Mục 4.2, Trang 26; Mục 4.7 Frame rates, Trang 43*).
2. **Bộ lọc cố định (Fixed Filter) & Nhánh OIS:** Dữ liệu pre-filtered (1.6 kHz Accel / 6.4 kHz Gyro) có thể được trích xuất trực tiếp qua bộ lọc cố định xuống giao tiếp **OIS SPI** (độ trễ nhóm tối thiểu ~680 µs) hoặc đưa thẳng vào bộ đệm FIFO nếu chọn chế độ unfiltered (*Basic Description, Trang 2; Mục 4.2, Trang 26; Mục 4.7, Trang 43; Mục 4.11, Trang 65*).
3. **Bộ lọc cấu hình bởi người dùng (User Configurable Filter):** Hạ tần số lấy mẫu (decimation) và lọc thông thấp (LPF) theo tần số xuất dữ liệu (ODR) và hệ số băng thông (`acc_bwp`, `gyr_bwp`):
   * Đầu ra Accelerometer hỗ trợ ODR từ **12.5 Hz đến 1600 Hz** (Normal/Performance mode) hoặc **0.78 Hz đến 400 Hz** (Low Power mode) (*Mục 1 Table 2, Trang 14; Mục 4.2, Trang 26; Mục 4.6, Trang 30–31*).
   * Đầu ra Gyroscope hỗ trợ ODR từ **25 Hz đến 3200 Hz** trên thanh ghi `DATA` (**lên đến 6.4 kHz** khi đọc qua FIFO) ở Normal/Performance mode, hoặc **25 Hz đến 100 Hz** ở Low Power mode (*Mục 1 Table 3, Trang 16; Mục 4.2, Trang 26; Mục 4.6, Trang 32–33*).
4. **Khối bù người dùng (User Compensation):** Thực hiện bù điểm không (Offset Compensation) và bù tự động (Auto-compensation / IOC, Sensitivity Gain) trước khi đẩy dữ liệu vào các thanh ghi `DATA_0`..`DATA_19`, bộ đệm **FIFO 2 KB**, và bộ máy tính năng thông minh (**Features Engine**) (*Mục 4.2, Trang 26; Mục 4.13–4.14, Trang 70–72*).

### 1.3. Thông số kỹ thuật cốt lõi của các cảm biến
Bảng tổng hợp thông số vận hành của các khối cảm biến thành phần (*Mục 1 Specifications – Table 1, 2, 3, 4, Trang 12–17*):

| Thành phần | Thông số kỹ thuật | Giá trị điển hình / Dải cấu hình | Nguồn trích dẫn |
| :--- | :--- | :--- | :--- |
| **Nguồn cấp** | Điện áp lõi `VDD` / Điện áp giao tiếp `VDDIO` | `VDD`: 1.71 V – 3.6 V (Typ 1.8 V) / `VDDIO`: 1.2 V – 3.6 V (Typ 1.8 V) | *Mục 1, Table 1, Trang 12; Mục 4.3, Trang 26* |
| **Accelerometer** | Độ phân giải & Dải đo (`ACC_RANGE`) | 16-bit signed; $\pm 2g$, $\pm 4g$, $\pm 8g$ (mặc định), $\pm 16g$ | *Mục 1, Table 2, Trang 13; Mục 5.2, Trang 109* |
| **Accelerometer** | Độ nhạy (Sensitivity) | $\pm 2g$: $16384\ \text{LSB}/g$; $\pm 4g$: $8192\ \text{LSB}/g$; $\pm 8g$: $4096\ \text{LSB}/g$; $\pm 16g$: $2048\ \text{LSB}/g$ | *Mục 1, Table 2, Trang 13* |
| **Accelerometer** | Sai số Zero-g Offset & Mật độ nhiễu | Offset: $\pm 20\ \text{mg}$; Nhiễu: $0.16\ \text{mg}/\sqrt{\text{Hz}}$ (Normal mode, $\pm 8g$) | *Mục 1, Table 2, Trang 13–14* |
| **Accelerometer** | Độ nhạy chéo trục (Cross-Axis Sensitivity $S_{XA}$) | Typ. $1\%$ | *Mục 1, Table 2, Trang 15* |
| **Gyroscope** | Độ phân giải & Dải đo (`GYR_RANGE`) | 16-bit signed; $\pm 125$, $\pm 250$, $\pm 500$, $\pm 1000$, $\pm 2000\ \text{dps}$ (mặc định) | *Mục 1, Table 3, Trang 15; Mục 5.2, Trang 111* |
| **Gyroscope** | Độ nhạy (Sensitivity) | $\pm 2000$: $16.384$; $\pm 1000$: $32.768$; $\pm 500$: $65.536$; $\pm 250$: $131.072$; $\pm 125$: $262.144\ \text{LSB/dps}$ | *Mục 1, Table 3, Trang 15* |
| **Gyroscope** | Sai số độ nhạy (Sensitivity Error) | Trước hiệu chỉnh: $\pm 2\%$; Sau khi chạy CRT: $\pm 0.4\%$ | *Mục 1, Table 3, Trang 15* |
| **Gyroscope** | Sai số Zero-rate Offset & Mật độ nhiễu | Offset: $\pm 0.5\ \text{dps}$; Nhiễu: $0.007\ \text{dps}/\sqrt{\text{Hz}}$ (Performance), $0.010\ \text{dps}/\sqrt{\text{Hz}}$ (Normal) | *Mục 1, Table 3, Trang 16* |
| **Gyroscope** | Độ nhạy chéo trục ($S_{XG}$) & Độ nhạy gia tốc ($g$-Sensitivity) | $S_{XG}$: Typ. $0.2\%$ (sau khi bù CAS); $g$-Sensitivity: Max $0.1\ \text{dps}/g$ | *Mục 1, Table 3, Trang 17* |
| **Temperature** | Dải đo, Điểm gốc & Độ nhạy ($S_T$) | $-41^{\circ}\text{C}$ đến $+87^{\circ}\text{C}$; `0x0000` tương ứng $23^{\circ}\text{C}$; $512\ \text{LSB/K}$ | *Mục 1, Table 4, Trang 17; Mục 4.6, Trang 34* |

### 1.4. Các chế độ nguồn (Power Modes)
Trạng thái nguồn và chất lượng tín hiệu được điều khiển kết hợp bởi 4 thanh ghi: `PWR_CTRL` (`0x7D`), `PWR_CONF` (`0x7C`), `ACC_CONF` (`0x40`) và `GYR_CONF` (`0x42`). Toàn bộ giá trị thanh ghi cấu hình và dữ liệu trong FIFO đều được giữ nguyên khi chuyển đổi giữa các chế độ nguồn (*Mục 4.5 Power Modes, Table 6, Trang 28–29*):

| Chế độ nguồn | `acc_en` (`0x7D.2`) | `gyr_en` (`0x7D.1`) | `acc_filter_perf` (`0x40.7`) | `gyr_filter_perf` (`0x42.7`) | `gyr_noise_perf` (`0x42.6`) | `adv_power_save` (`0x7C.0`) | Dòng tiêu thụ điển hình (`VDD = 1.8V`) |
| :--- | :---: | :---: | :---: | :---: | :---: | :---: | :--- |
| **Suspend Mode** (Mặc định sau POR) | `0` | `0` | `X` | `X` | `X` | `1` | $3.5\ \mu\text{A}$ |
| **Configuration Mode** (Nạp config/NVM) | `0` | `0` | `X` | `X` | `X` | `0` | $120\ \mu\text{A}$ |
| **Low Power Mode (Accel only)** | `1` | `0` | `0` | `X` | `X` | `1` | Xuống tới $4\ \mu\text{A}$ ($10\ \mu\text{A}$ @ 25 Hz) |
| **Low Power Mode (Gyro only)** | `0` | `1` | `X` | `0` | `0` | `1` | Xuống tới $420\ \mu\text{A}$ |
| **Low Power Mode (IMU: Accel + Gyro)** | `1` | `1` | `0` | `0` | `0` | `1` | Xuống tới $420\ \mu\text{A}$ (@ 25 Hz) |
| **Normal Mode (Accel only)** | `1` | `0` | `1` | `X` | `X` | `X` | $210\ \mu\text{A}$ |
| **Normal Mode (Gyro only)** | `0` | `1` | `X` | `1` | `0` | `X` | $600\ \mu\text{A}$ |
| **Normal Mode (IMU: Accel + Gyro)** | `1` | `1` | `1` | `1` | `0` | `X` | $685\ \mu\text{A}$ |
| **Performance Mode (Gyro only)** | `0` | `1` | `X` | `1` | `1` | `X` | $900\ \mu\text{A}$ |
| **Performance Mode (IMU: Accel + Gyro)** | `1` | `1` | `1` | `1` | `1` | `X` | $970\ \mu\text{A}$ |

> **Lưu ý quan trọng về `adv_power_save` (APS) và Accelerometer:**
> * Accelerometer không có sự khác biệt về hoạt động giữa Normal Mode và Performance Mode (*Mục 4.5, Footnote 5, Trang 28*).
> * Khi bật `PWR_CONF.adv_power_save = 0b1` (`0x7C` bit 0), thiết bị áp dụng các giới hạn nghiêm ngặt: thời gian trễ giữa các lệnh ghi thanh ghi (**inter-write delay**) phải đạt **tối thiểu 450 µs**, và bạn phải tắt `adv_power_save = 0b0` (chờ 450 µs) trước khi đọc FIFO trừ khi dùng tính năng `fifo_self_wakeup = 0b1` kèm ngắt FIFO (*Mục 4.5, Trang 29; Mục 4.7 FIFO in Low Power Mode, Trang 45*).
> * Để giảm thời gian khởi động Gyroscope từ 45 ms xuống **2 ms**, bật bit Fast Power-Up `PWR_CONF.fup_en = 0b1` (`0x7C` bit 2) (*Mục 1 Table 3, Trang 15; Mục 5.2, Trang 129*).

### 1.5. Bộ đếm thời gian `SENSORTIME` và cơ chế đồng bộ cấu hình
* **Bộ đếm 24-bit `SENSORTIME`:** Gồm 3 thanh ghi `SENSORTIME_0` (`0x18`), `SENSORTIME_1` (`0x19`), `SENSORTIME_2` (`0x1A`), đếm tự do với độ phân giải **39.0625 µs/LSB** (tràn khi đạt `0xFFFFFF`, tương đương ~655.36 giây) (*Mục 4.6 Sensor Time, Trang 35; Mục 5.2, Trang 87–88*).
* **Đồng bộ hóa sự kiện lấy mẫu:** Mọi sự kiện cập nhật thanh ghi `DATA` và `FIFO` đều đồng bộ với `SENSORTIME`. Mỗi khi dữ liệu được cập nhật ở tần số ODR, một bit thứ $m$ tương ứng trong `SENSORTIME` sẽ đảo trạng thái (ví dụ: bit 3 ứng với 3200 Hz / 0.3125 ms; bit 4 ứng với 1600 Hz / 0.625 ms; bit 5 ứng với 800 Hz / 1.25 ms; bit 6 ứng với 400 Hz / 2.5 ms; bit 7 ứng với 200 Hz / 5 ms; bit 8 ứng với 100 Hz / 10 ms) (*Mục 4.6 Sensor Time, Trang 35*).
* **Thời điểm có hiệu lực khi đổi cấu hình (Configuration Changes):** Nếu thay đổi `ACC_CONF`, `ACC_RANGE`, `GYR_CONF`, `GYR_RANGE`, hoặc `AUX_CONF` khi cảm biến đang bật, cấu hình mới **không áp dụng ngay lập tức** mà chỉ có hiệu lực khi thời điểm lấy mẫu của ODR hiện tại trùng khớp với thời điểm lấy mẫu của ODR mới trên lưới thời gian `SENSORTIME` (hoặc ở mẫu kế tiếp nếu giữ nguyên ODR) (*Mục 4.6 Configuration Changes, Trang 36*).

---

## 2. Hướng Dẫn Sử Dụng & Lập Trình Giao Tiếp

### 2.1. Cấu hình giao tiếp vật lý (I²C & SPI) và Quy tắc Dummy Byte
Sau khi cấp nguồn (POR) hoặc Soft-Reset, giao tiếp chính mặc định ở chế độ **I²C** (*Mục 6.2–6.3, Trang 132–133*):
* **Giao tiếp I²C:**
  * Chân `SCx` (Pin 13) là `SCL`, chân `SDx` (Pin 14) là `SDA`. Khuyến nghị nối cứng chân `CSB` (Pin 12) lên `VDDIO` để tránh nhiễu xung chuyển sang SPI (*Mục 6.2 Table 17, Trang 132; Mục 6.3, Trang 133*).
  * Địa chỉ I²C 7-bit mặc định là **`0x68`** (`0b1101000`) khi nối chân `SDO` (Pin 1) xuống `GND`, hoặc **`0x69`** (`0b1101001`) khi nối `SDO` lên `VDDIO` (*Mục 6.5 Primary Interface I²C, Trang 138*).
  * Khi đọc thanh ghi qua I²C: **0 dummy byte** (dữ liệu trả về ngay từ byte đầu tiên sau địa chỉ) (*Mục 3 Quick Start Guide, Trang 19; Mục 6.5, Trang 140–141*).
* **Giao tiếp SPI (4-wire mặc định hoặc 3-wire):**
  * Để chuyển từ I²C sang SPI, chân `CSB` phải có một **sườn lên (rising edge)** sau khi cấp nguồn; chip sẽ chuyển sang SPI sau 200 µs. Do đó, bước chuẩn đầu tiên khi dùng SPI là thực hiện 1 lệnh đọc nháp (dummy read) vào thanh ghi `CHIP_ID` (`0x00`) để tạo sườn lên trên `CSB` (bỏ qua giá trị trả về của lần đọc đầu này), hoặc cấu hình cố định bit `NV_CONF.spi_en = 0b1` (`0x70` bit 0) lưu vào NVM (*Mục 3.1.a, Trang 19; Mục 6.3, Trang 133; Mục 5.2, Trang 126*).
  * Hỗ trợ SPI Mode `'00'` (`CPOL=0, CPHA=0`) và Mode `'11'` (`CPOL=1, CPHA=1`), tự động nhận diện theo mức logic của `SCK` ngay sau sườn xuống của `CSB` (*Mục 6.4 Primary Interface SPI, Trang 134*).
  * **Quy tắc Dummy Byte bắt buộc trên SPI:** Mọi thao tác đọc thanh ghi qua SPI (single read hoặc burst read) đều trả về **1 dummy byte** ở byte đầu tiên trên đường `SDO` trước khi trả về dữ liệu thực từ byte thứ 2 trở đi. Để đọc $n$ byte dữ liệu qua SPI, Host phải đọc $n + 1$ byte và **bỏ đi byte đầu tiên** (*Mục 3, Trang 19; Mục 6.4, Trang 135–136*).
* **Ràng buộc thời gian chờ sau lệnh ghi (Post-Write Access Timing Constraints):**
  * Ở chế độ Normal Mode,Configuration Mode (`adv_power_save = 0b0`), hoặc Fast Startup Mode: Sau mỗi lệnh ghi (Write), Host phải chờ tối thiểu **`2 µs`** (`tIDLE_wr_act`) trước khi thực hiện thao tác tiếp theo (*Mục 6.4 Table 19, Trang 133; Mục 6.5 Table 20, Trang 138, 141*).
  * Ở chế độ Suspend Mode hoặc Low-Power Mode (`adv_power_save = 0b1`): Sau mỗi lệnh ghi, Host phải chờ tối thiểu **`450 µs`** (`tIDLE_wacc_sum`) (*Mục 4.5, Trang 29; Mục 6.4 Table 19, Trang 133; Mục 6.5, Trang 138, 141*).

### 2.2. Quy trình khởi tạo thiết bị bắt buộc (POR & Device Initialization)
Khác với các IMU đời cũ chỉ cần bật thanh ghi nguồn, sau mỗi lần cấp nguồn (POR) hoặc Soft-Reset (ghi `0xB6` vào `CMD` `0x7E`), BMI270 nằm ở Suspend Mode và **bắt buộc phải được nạp tệp cấu hình 8 kB (`bmi270_config_file`)** thì bộ xử lý nội (ASIC) và các cảm biến mới hoạt động được (*Mục 3.1, Trang 19–21; Mục 4.4 Power-On-Reset and Device Initialization, Trang 27; Mục 4.17, Trang 74*).

Trình tự khởi tạo từng bước như sau:
1. **Chờ ổn định nguồn:** Sau khi `VDD` và `VDDIO` đạt mức danh định, chờ tối thiểu **450 µs** (hoặc $t_{PO} \le 2\ \text{ms}$) để giao tiếp nối tiếp sẵn sàng (*Mục 1 Table 1, Trang 13; Mục 4.4, Trang 27*).
2. **Kiểm tra giao tiếp & `CHIP_ID`:**
   * Nếu dùng SPI: Thực hiện 1 lần đọc nháp thanh ghi `0x00` để kích hoạt giao diện SPI (*Mục 3.1.a, Trang 19*).
   * Đọc thanh ghi `CHIP_ID` (`0x00`) và kiểm tra giá trị trả về phải bằng **`0x24`** (*Mục 3.1.a, Trang 19; Mục 5.2, Trang 81*).
3. **Tắt chế độ tiết kiệm năng lượng nâng cao (Disable APS):**
   * Ghi `0x00` vào thanh ghi `PWR_CONF` (`0x7C`) để đặt `adv_power_save = 0b0` (*Mục 3.1.b, Trang 20; Mục 4.4, Trang 27*).
   * Chờ tối thiểu **450 µs** (hoặc đếm 12 LSB của `SENSORTIME_0`) (*Mục 3.1.b, Trang 20; Mục 4.4, Trang 27*).
4. **Chuẩn bị nạp tệp cấu hình:**
   * Ghi `0x00` vào thanh ghi `INIT_CTRL` (`0x59`) (`init_ctrl = 0x00`) (*Mục 3.1.b, Trang 20; Mục 4.4, Trang 27*).
5. **Ghi Burst 8 kB dữ liệu cấu hình (`bmi270_config_file`):**
   * Thực hiện **Burst Write** mảng 8192 bytes (`bmi270_config_file` lấy từ Bosch Sensortec BMI270-Sensor-API) vào thanh ghi `INIT_DATA` (`0x5E`), bắt đầu từ byte 0. Lưu ý: `INIT_DATA` (`0x5E`) là thanh ghi bẫy địa chỉ (**address-trap register**), địa chỉ không tự tăng khi ghi burst nên toàn bộ chuỗi byte vẫn giữ nguyên đích tại `0x5E` (*Mục 4.4, Trang 27; Mục 5.1 General Remarks, Trang 75; Mục 5.2, Trang 122*).
   * *Trường hợp bộ đệm DMA/SPI/I²C của MCU không đủ 8 kB cho 1 lần truyền:* Chia thành nhiều gói nhỏ (chunks). Giữa hai lần ghi chunk, phải cập nhật địa chỉ cơ sở trong cặp thanh ghi `INIT_ADDR_0` (`0x5B`, chứa bit `3..0`) và `INIT_ADDR_1` (`0x5C`, chứa bit `11..4`) tăng thêm một lượng bằng `(Số byte đã ghi của chunk trước) / 2` (*Mục 4.4 Footnote 4, Trang 27; Mục 5.2, Trang 121–122*).
6. **Hoàn tất nạp cấu hình:**
   * Ghi `0x01` vào thanh ghi `INIT_CTRL` (`0x59`). **Tuyệt đối không thực hiện thao tác này quá 1 lần** sau mỗi lần POR hoặc Soft-Reset (*Mục 3.1.b, Trang 20; Mục 4.4, Trang 27*).
7. **Kiểm tra trạng thái khởi tạo (`INTERNAL_STATUS`):**
   * Chờ cho đến khi thanh ghi `INTERNAL_STATUS` (`0x21`) có các bit `message` (`3..0`) mang giá trị **`0b0001`** (`init_ok` – ASIC initialized). Theo Mục 4.4 (Trang 27), quá trình này mất tối đa **20 ms** (trong lưu đồ Mục 3.1.c Trang 21 đề xuất chờ $\ge 140\ \text{ms}$ rồi đọc `0x21` kiểm tra bit `0 == 1`). Khi hoàn tất, BMI270 tự động chuyển sang **Configuration Mode** (*Mục 3.1.c, Trang 21; Mục 4.4, Trang 27; Mục 5.2, Trang 91*).

### 2.3. Cấu hình chế độ hoạt động, Tần số lấy mẫu (ODR) & Bộ lọc số (LPF)

#### A. Bảng tra cứu cấu hình thanh ghi chế độ hoạt động
Dựa theo lưu đồ mẫu của hãng (*Mục 3.2, 3.3, 3.4, Trang 22–24*), dưới đây là chuỗi ghi thanh ghi tiêu chuẩn cho 3 chế độ:

| Bước | Thanh ghi (Địa chỉ) | Low Power Mode (Chỉ Accel @ 50Hz) (*Mục 3.2, Trang 22*) | Normal Mode (IMU: Accel 100Hz, Gyro 200Hz) (*Mục 3.3, Trang 23*) | Performance Mode (IMU: Accel 100Hz, Gyro 200Hz) (*Mục 3.4, Trang 24*) |
| :---: | :--- | :--- | :--- | :--- |
| **1** | `PWR_CTRL` (`0x7D`) | Ghi `0x04` (Bật Accel; tắt Gyro, Temp, Aux) | Ghi `0x0E` (Bật Accel, Gyro, Temp; tắt Aux) | Ghi `0x0E` (Bật Accel, Gyro, Temp; tắt Aux) |
| **2** | `ACC_CONF` (`0x40`) | Ghi `0x17` (`acc_filter_perf=0`, `acc_bwp=0x01` avg 2 mẫu, `acc_odr=0x07` 50 Hz) | Ghi `0xA8` (`acc_filter_perf=1`, `acc_bwp=0x02` normal, `acc_odr=0x08` 100 Hz) | Ghi `0xA8` (`acc_filter_perf=1`, `acc_bwp=0x02` normal, `acc_odr=0x08` 100 Hz) |
| **3** | `GYR_CONF` (`0x42`) | *(Bỏ qua vì tắt Gyro)* | Ghi `0xA9` (`gyr_filter_perf=1`, `gyr_noise_perf=0`, `gyr_bwp=0x02` normal, `gyr_odr=0x09` 200 Hz) | Ghi `0xE9` (`gyr_filter_perf=1`, `gyr_noise_perf=1`, `gyr_bwp=0x02` normal, `gyr_odr=0x09` 200 Hz) |
| **4** | `PWR_CONF` (`0x7C`) | Ghi `0x03` (`adv_power_save=1`, `fifo_self_wakeup=1`) | Ghi `0x02` (`adv_power_save=0`, `fifo_self_wakeup=1`) | Ghi `0x02` (`adv_power_save=0`, `fifo_self_wakeup=1`) |

#### B. Tham số bộ lọc thông thấp (LPF), Nhiễu RMS và Độ trễ nhóm (Group Delay)
Khi hoạt động ở Normal Mode hoặc Performance Mode (`acc_filter_perf = 1`, `gyr_filter_perf = 1`):
* **Tham số `acc_bwp` (`ACC_CONF` bits `6..4`) và `gyr_bwp` (`GYR_CONF` bits `5..4`):** Cho phép chọn đặc tính bộ lọc số:
  * `0x00`: **OSR4 mode** (Oversampling 4x – băng thông hẹp nhất, lọc nhiễu mạnh nhất, trễ lớn hơn).
  * `0x01`: **OSR2 mode** (Oversampling 2x).
  * `0x02`: **Normal mode** (Mặc định sau reset).
  * `0x03`: **CIC mode** (chỉ có trên `acc_bwp`, `gyr_bwp = 0x03` là reserved) (*Mục 5.2, Trang 108–110*).
* Khi ở Low Power Mode (`acc_filter_perf = 0`), trường `acc_bwp` quyết định số mẫu lấy trung bình cộng theo công thức: $\text{Averaged samples} = 2^{\text{Val(acc\_bwp)}}$ (từ $2^0 = 1$ mẫu đến $2^7 = 128$ mẫu) (*Mục 4.6, Trang 31; Mục 5.2, Trang 108–109*).

Bảng đặc tính tần số cắt 3dB, nhiễu RMS và độ trễ nhóm (Group Delay) ở cấu hình lọc chuẩn (`acc_bwp = 0x02`, `gyr_bwp = 0x02`) (*Mục 4.6, Table 7, 8, 9, 10, 11, 12, 13, Trang 30–33*):

| ODR [Hz] (`acc_odr` / `gyr_odr`) | Mã Hex ODR | Accel 3dB Cutoff [Hz] | Accel RMS Noise ($\pm 8g$) [mg] | Accel Group Delay [ms] | Gyro 3dB Cutoff [Hz] | Gyro RMS Noise (Normal / Perf, $\pm 2000\ \text{dps}$) [mdps] | Gyro Group Delay [ms] |
| :--- | :---: | :---: | :---: | :---: | :---: | :---: | :---: |
| **12.5 Hz** (Chỉ Accel) | `0x05` | 5.5 | 0.38 | 80.0 | — | — | — |
| **25 Hz** | `0x06` | 11 | 0.53 | 40.0 | 11 | 31.0 / 21.7 | 40.0 |
| **50 Hz** | `0x07` | 22 | 0.75 | 20.5 | 20 | 43.9 / 30.7 | 20.5 |
| **100 Hz** | `0x08` | 44 | 1.06 | 10.5 | 39 | 62.0 / 43.4 | 10.8 |
| **200 Hz** | `0x09` | 89 | 1.51 | 5.4 | 77 | 87.7 / 61.4 | 5.97 |
| **400 Hz** | `0x0A` | 178 | 2.13 | 2.0 | 152 | 124 / 86.9 | 3.55 |
| **800 Hz** | `0x0B` | 343 | 2.96 | 1.3 | 300 | 176 / 123 | 2.34 |
| **1600 Hz** | `0x0C` | 740 | 4.35 | 0.6 | 557 | 248 / 174 | 0.97 |
| **3200 Hz** (Chỉ Gyro) | `0x0D` | — | — | — | 751 | 431 / 302 | 0.82 |
| **6400 Hz** (Chỉ Gyro FIFO/OIS) | — | — | — | — | 712 | 500 / 350 | 0.68 |

### 2.4. Cơ chế đọc dữ liệu (Data Readout, Shadowing, Interrupts & FIFO)

#### A. Đọc trực tiếp từ hệ thống thanh ghi `DATA_0` .. `DATA_19` (`0x04` .. `0x17`)
* **Bản đồ thanh ghi dữ liệu:**
  * `DATA_0` .. `DATA_7` (`0x04` .. `0x0B`): Dữ liệu cảm biến phụ `AUX_X`, `AUX_Y`, `AUX_Z`, `AUX_R` (LSB trước, MSB sau) (*Mục 5.2, Trang 79, 82–84*).
  * `DATA_8` .. `DATA_13` (`0x0C` .. `0x11`): Dữ liệu gia tốc kế 3 trục `ACC_X` (`0x0C` LSB, `0x0D` MSB), `ACC_Y` (`0x0E` LSB, `0x0F` MSB), `ACC_Z` (`0x10` LSB, `0x11` MSB) (*Mục 4.6, Trang 30; Mục 5.2, Trang 79, 84–85*).
  * `DATA_14` .. `DATA_19` (`0x12` .. `0x17`): Dữ liệu con quay hồi chuyển 3 trục `GYR_X` (`0x12` LSB, `0x13` MSB), `GYR_Y` (`0x14` LSB, `0x15` MSB), `GYR_Z` (`0x16` LSB, `0x17` MSB) (*Mục 4.6, Trang 31; Mục 5.2, Trang 79, 86–87*).
* **Quy tắc bảo toàn dữ liệu (Shadowing Procedure):**
  * Khi đọc từng trục, **bắt buộc phải đọc thanh ghi LSB trước rồi mới đọc thanh ghi MSB**. Việc đọc thanh ghi LSB sẽ khóa (lock) nội dung của thanh ghi MSB tương ứng để ngăn phần cứng cập nhật mẫu mới vào giữa 2 lần đọc gây rách dữ liệu (data tearing) (*Mục 4.6 Accelerometer Data & Gyroscope Data, Trang 30–31*).
  * Khi thực hiện **Burst Read** (ví dụ đọc 12 bytes liên tiếp từ `0x0C` đến `0x17`, hoặc đọc kèm `SENSORTIME` tới `0x1A`), toàn bộ nhóm thanh ghi (`STATUS`, `DATA_x`, `SENSORTIME_x`, `TEMPERATURE_x`, `SC_OUT_x`, `FIFO_LENGTH_x`) được chốt đồng bộ (shadowed) ngay tại thời điểm bắt đầu lệnh đọc (*Mục 5.1 General Remarks, Trang 75; Mục 5.2, Trang 88*).
* **Kiểm tra cờ báo tràn ADC (`SATURATION` `0x4A`):** Thanh ghi `0x4A` cập nhật đồng bộ với `DATA_0`..`DATA_19`, chứa các bit cờ `acc_x/y/z` (bits `0..2`) và `gyr_x/y/z` (bits `3..5`). Nếu một bit lên `1`, nghĩa là mẫu dữ liệu thô dùng để tính ra mẫu lọc hiện tại đã bị bão hòa (chạm ngưỡng `0x8001` hoặc `0x7FFF`) (*Mục 5.2 Register 0x4A, Trang 116*).

#### B. Sử dụng ngắt Data Ready (`DRDY`) để đọc độ trễ thấp
* **Cấu hình chân vật lý `INT1` / `INT2`:** Thiết lập qua thanh ghi `INT1_IO_CTRL` (`0x53`) hoặc `INT2_IO_CTRL` (`0x54`): bật đầu ra `output_en = 1` (bit 3), chọn kiểu cực `od = 0` (Push-Pull) hoặc `1` (Open-Drain, bit 2), và chọn mức tích cực `lvl = 1` (Active-High) hoặc `0` (Active-Low, bit 1). Chọn chế độ ngắt không chốt (`int_latch = 0`, xung ngắt tự xóa sau $1/6400\ \text{Hz} \approx 156.25\ \mu\text{s}$, phù hợp ngắt sườn Edge-triggered) hoặc chốt (`int_latch = 1`, phù hợp Level-triggered) tại `INT_LATCH` (`0x55`) (*Mục 4.6, Trang 31, 33; Mục 4.9 General Interrupt Pin Configuration, Trang 59; Mục 5.2, Trang 118–119*).
* **Ánh xạ ngắt Data Ready:** Bật bit `drdy_int1` (`INT_MAP_DATA` `0x58` bit 2) để đưa ngắt ra chân `INT1`, hoặc `drdy_int2` (bit 6) để đưa ra chân `INT2` (*Mục 4.6, Trang 31, 33; Mục 5.2, Trang 121*).
* **Cơ chế xóa cờ ngắt (Clear-on-read):**
  * Cờ `INT_STATUS_1.acc_drdy_int` (bit 7) và `gyr_drdy_int` (bit 6) tại thanh ghi `0x1D` tự động xóa khi đọc thanh ghi `INT_STATUS_1` (`0x1D`) (*Mục 4.6, Trang 31, 33; Mục 5.1, Trang 75; Mục 5.2, Trang 89*).
  * Cờ trạng thái `STATUS.drdy_acc` (`0x03` bit 7) và `STATUS.drdy_gyr` (`0x03` bit 6) tự động xóa khi đọc thanh ghi dữ liệu tương ứng (cụ thể khi đọc `DATA_9` đối với Accel và `DATA_15` đối với Gyro theo Mục 5.1 Trang 75) (*Mục 4.6, Trang 31, 33; Mục 5.1, Trang 75*).

#### C. Sử dụng bộ đệm FIFO 2048-byte (`0x24` .. `0x26`, `0x45` .. `0x49`)
* **Chế độ hoạt động & Kích hoạt:** Bộ đệm FIFO dung lượng 2048 bytes hỗ trợ **Streaming mode** (`FIFO_CONFIG_0.fifo_stop_on_full = 0b0`, ghi đè dữ liệu cũ nhất khi đầy) và **FIFO mode** (`fifo_stop_on_full = 0b1`, bỏ qua dữ liệu mới nhất khi đầy). Bật lưu từng cảm biến qua `FIFO_CONFIG_1` (`0x49`): `fifo_gyr_en` (bit 7), `fifo_acc_en` (bit 6), `fifo_aux_en` (bit 5) (*Mục 4.7 FIFO, Trang 37, 43; Mục 5.2, Trang 114–115*).
* **Lựa chọn dữ liệu Filtered / Unfiltered (6.4 kHz Gyro) & Downsampling (`FIFO_DOWNS` `0x45`):**
  * Có thể chọn đưa dữ liệu đã lọc (`acc_fifo_filt_data = 1`, `gyr_fifo_filt_data = 1`) hoặc dữ liệu **pre-filtered tốc độ cao** (`acc_fifo_filt_data = 0` @ 1600 Hz; `gyr_fifo_filt_data = 0` @ **6400 Hz**) vào FIFO. Lưu ý: Khi chọn dữ liệu Gyro pre-filtered vào FIFO, thang đo của nó được quyết định bởi bit `GYR_RANGE.ois_range` ($\pm 250\ \text{dps}$ hoặc $\pm 2000\ \text{dps}$) chứ không phải `gyr_range` (*Mục 4.7 Frame rates, Trang 43; Mục 5.2, Trang 111, 113*).
  * Tốc độ đẩy vào FIFO có thể giảm theo hệ số $2^k$ ($k \in \{0..7\}$) thông qua `acc_fifo_downs` (bits `6..4`) và `gyr_fifo_downs` (bits `2..0`) (*Mục 4.7, Trang 43; Mục 5.2, Trang 113*).
* **Định dạng khung dữ liệu (Header Mode vs. Headerless Mode):**
  * **Thứ tự dữ liệu trong khung FIFO ngược với thanh ghi `DATA`:** Trong một khung FIFO đầy đủ, thứ tự luôn là **`AUX` (bytes 0..7) $\rightarrow$ `GYR_X, Y, Z` (bytes 8..13) $\rightarrow$ `ACC_X, Y, Z` (bytes 14..19)** (trong khi ở thanh ghi `DATA_8`..`DATA_19` thì `ACC` nằm trước `GYR`) (*Mục 4.7 Header mode & Headerless mode, Trang 38–39, 42*).
  * **Header Mode (`fifo_header_en = 1`):** Bắt buộc dùng khi các cảm biến bật trong FIFO có ODR khác nhau. Mỗi khung bắt đầu bằng 1 byte Header (`fh_mode<1:0>`, `fh_parm<3:0>`, `fh_ext<1:0>`), hỗ trợ cả khung dữ liệu thường (`fh_mode = 0b10`) và khung điều khiển (`fh_mode = 0b01`: Skip Frame, Sensortime Frame, Fifo_Input_Config Frame) (*Mục 4.7, Trang 37–41*).
  * **Headerless Mode (`fifo_header_en = 0`):** Chỉ dùng khi mọi cảm biến bật trong FIFO có cùng tốc độ ODR. Không tốn byte header, chỉ chứa payload cố định (*Mục 4.7, Trang 37, 42*).
* **Quy tắc đọc FIFO:** Kiểm tra số byte hiện có qua `FIFO_LENGTH_0` (`0x24`) và `FIFO_LENGTH_1` (`0x25`, bits `5..0`), sau đó thực hiện **Burst Read** từ thanh ghi `FIFO_DATA` (`0x26` – địa chỉ không tự tăng). Nếu một khung (frame) chỉ được đọc một phần, toàn bộ khung đó (kể cả header) sẽ được phát lại từ đầu ở lần đọc kế tiếp. Để xóa sạch FIFO bằng phần mềm, ghi lệnh `fifo_flush` (`0xB0`) vào `CMD` (`0x7E`) (*Mục 4.7 Conditions and Details & FIFO Reset, Trang 43, 45; Mục 5.2, Trang 92–93, 130*).

---

## 3. Hiệu Chỉnh Tham Số & Xử Lý Dữ Liệu Đọc Về (Calibration & Compensation)

Để đạt độ chính xác cao cho các thuật toán ước lượng trạng thái (như bộ lọc Kalman, Complementary Filter, AHRS hoặc vòng lặp điều khiển PID/LQR), dữ liệu đọc về từ BMI270 cần được xử lý và hiệu chỉnh theo các cơ chế phần cứng và phần mềm dưới đây.

### 3.1. Chuyển đổi giá trị Raw 16-bit sang đơn vị vật lý ($g$, $\text{dps}$, $^{\circ}\text{C}$)

#### A. Ghép byte và mở rộng dấu (Two's Complement 16-bit)
Dữ liệu Accelerometer, Gyroscope và Temperature đều được biểu diễn dưới dạng số nguyên 16-bit bù 2 có dấu (`int16_t`) (*Mục 4.6, Trang 30, 31, 34*):
$$\text{Raw}_{16} = \text{ (int16\_t) }\big((\text{MSB} \ll 8) \mid \text{LSB}\big)$$

#### B. Quy đổi Gia tốc kế (Accelerometer) và Con quay hồi chuyển (Gyroscope)
Chia giá trị nguyên có dấu $\text{Raw}_{16}$ cho độ nhạy (Sensitivity) tương ứng với thang đo đã cài đặt trong `ACC_RANGE` (`0x41`) và `GYR_RANGE` (`0x43`) (*Mục 1 Table 2 & Table 3, Trang 13, 15; Mục 5.2, Trang 109, 111*):

| Cảm biến | Giá trị thanh ghi Range | Thang đo (Full Scale) | Độ nhạy chính xác (LSB / đơn vị) | Công thức quy đổi sang đơn vị vật lý |
| :--- | :---: | :---: | :---: | :--- |
| **Accelerometer** (`0x41` bits `1..0`) | `0x00` (`range_2g`) | $\pm 2\ g$ | $16384\ \text{LSB}/g$ | $a\ [g] = \text{Raw}_{16} / 16384.0$ |
| **Accelerometer** (`0x41` bits `1..0`) | `0x01` (`range_4g`) | $\pm 4\ g$ | $8192\ \text{LSB}/g$ | $a\ [g] = \text{Raw}_{16} / 8192.0$ |
| **Accelerometer** (`0x41` bits `1..0`) | `0x02` (`range_8g` - Mặc định) | $\pm 8\ g$ | $4096\ \text{LSB}/g$ | $a\ [g] = \text{Raw}_{16} / 4096.0$ |
| **Accelerometer** (`0x41` bits `1..0`) | `0x03` (`range_16g`) | $\pm 16\ g$ | $2048\ \text{LSB}/g$ | $a\ [g] = \text{Raw}_{16} / 2048.0$ |
| **Gyroscope** (`0x43` bits `2..0`) | `0x00` (`range_2000` - Mặc định) | $\pm 2000\ \text{dps}$ | $16.384\ \text{LSB/dps}$ ($2^{15}/2000$) | $\omega\ [\text{dps}] = \text{Raw}_{16} / 16.384$ |
| **Gyroscope** (`0x43` bits `2..0`) | `0x01` (`range_1000`) | $\pm 1000\ \text{dps}$ | $32.768\ \text{LSB/dps}$ ($2^{15}/1000$) | $\omega\ [\text{dps}] = \text{Raw}_{16} / 32.768$ |
| **Gyroscope** (`0x43` bits `2..0`) | `0x02` (`range_500`) | $\pm 500\ \text{dps}$ | $65.536\ \text{LSB/dps}$ ($2^{15}/500$) | $\omega\ [\text{dps}] = \text{Raw}_{16} / 65.536$ |
| **Gyroscope** (`0x43` bits `2..0`) | `0x03` (`range_250`) | $\pm 250\ \text{dps}$ | $131.072\ \text{LSB/dps}$ ($2^{15}/250$) | $\omega\ [\text{dps}] = \text{Raw}_{16} / 131.072$ |
| **Gyroscope** (`0x43` bits `2..0`) | `0x04` (`range_125`) | $\pm 125\ \text{dps}$ | $262.144\ \text{LSB/dps}$ ($2^{15}/125$) | $\omega\ [\text{dps}] = \text{Raw}_{16} / 262.144$ |

#### C. Quy đổi Nhiệt độ (`TEMPERATURE_0` `0x22` & `TEMPERATURE_1` `0x23`)
* Ghép 16-bit có dấu: $\text{Temp\_Raw} = \text{(int16\_t)}\big((\text{TEMPERATURE\_1} \ll 8) \mid \text{TEMPERATURE\_0}\big)$.
* Kiểm tra tính hợp lệ: Ngay sau khi bật cảm biến, thanh ghi chứa giá trị **`0x8000` (`-32768`)** báo hiệu dữ liệu chưa hợp lệ (Invalid) cho đến khi phép đo đầu tiên hoàn tất (*Mục 4.6 Temperature Sensor, Trang 34; Mục 5.2, Trang 92*).
* Công thức tính nhiệt độ ($^{\circ}\text{C}$) với độ nhạy $512\ \text{LSB/K}$ ($2^9\ \text{LSB/K}$) và điểm gốc `0x0000` = $23^{\circ}\text{C}$ (*Mục 1 Table 4, Trang 17; Mục 4.6, Trang 34; Mục 5.2, Trang 92*):
  $$T\ [^{\circ}\text{C}] = \frac{\text{Temp\_Raw}}{512.0} + 23.0$$

---

### 3.2. Hiệu chỉnh độ nhạy chéo trục của Gyroscope (Gyroscope CAS Post-Processing)
Do đặc tính cơ học của cấu trúc MEMS con quay hồi chuyển, chuyển động quay quanh trục Z gây ra nhiễu chéo sang trục X (Cross-Axis Sensitivity - CAS). Để đạt độ nhạy chéo trục tối ưu ($0.2\%$ theo Bảng 3, Trang 17), tài liệu gốc quy định bước **hậu xử lý dữ liệu bắt buộc trên phần mềm Host** (*Mục 4.6 Gyroscope Data Post-Processing, Trang 32*):

#### A. Công thức bù chéo trục X theo trục Z
$$\text{Rate}_x = (\text{DATA\_15} \ll 8 + \text{DATA\_14}) - \frac{\text{GYR\_CAS.factor\_zx} \times (\text{DATA\_19} \ll 8 + \text{DATA\_18})}{2^9}$$
$$\text{Rate}_y = \text{DATA\_17} \ll 8 + \text{DATA\_16}$$
$$\text{Rate}_z = \text{DATA\_19} \ll 8 + \text{DATA\_18}$$
*(Trong đó $2^9 = 512$, `DATA_15:14` là $\text{Raw\_Gyr\_X}$ và `DATA_19:18` là $\text{Raw\_Gyr\_Z}$).*

#### B. Cách đọc và mở rộng dấu hệ số `GYR_CAS.factor_zx`
1. **Vị trí thanh ghi:** Hệ số `factor_zx` nằm ở **Page 0** của vùng thanh ghi mở rộng `FEATURES`, tại địa chỉ **`0x3C`** (`GYR_CAS`), chiếm các bit **`6...0`** (*Mục 5.2 FEATURES Pages, Trang 80; Register 0x3C GYR_CAS, Trang 95–96*).
2. **Cách đọc:**
   * Ghi `0x00` vào thanh ghi `FEAT_PAGE` (`0x2F`) để chọn Page 0 (*Mục 4.8 Global Configuration, Trang 46; Mục 5.2, Trang 93*).
   * Đọc thanh ghi `0x3C` (lấy 7 bit thấp `bits 6..0`).
3. **Mở rộng dấu 7-bit bù 2 (Sign Extension):**
   * `GYR_CAS.factor_zx` là một số nguyên có dấu **7-bit mã hóa bù 2 (7-bit two's complement signed value)** (dải từ $-64$ đến $+63$). Nếu tự viết driver mà không dùng sẵn Bosch Sensor API, bạn **bắt buộc phải thực hiện mở rộng dấu (sign extension)** từ 7-bit sang kiểu số nguyên có dấu 8-bit (`int8_t`) trước khi nhân (*Mục 4.6 Gyroscope Data Post-Processing, Note, Trang 32*):
   ```c
   // Ví dụ C/C++ đọc và mở rộng dấu 7-bit sang int8_t + áp dụng bù CAS:
   uint8_t raw_cas = read_reg(0x3C) & 0x7F; // Lấy bits 6..0 từ GYR_CAS (Page 0)
   int8_t factor_zx = (raw_cas & 0x40) ? (int8_t)(raw_cas | 0x80) : (int8_t)raw_cas;

   int16_t raw_gx = (int16_t)((uint16_t)data_15 << 8 | data_14);
   int16_t raw_gy = (int16_t)((uint16_t)data_17 << 8 | data_16);
   int16_t raw_gz = (int16_t)((uint16_t)data_19 << 8 | data_18);

   int16_t rate_x = (int16_t)(raw_gx - ((int32_t)factor_zx * (int32_t)raw_gz) / 512);
   int16_t rate_y = raw_gy;
   int16_t rate_z = raw_gz;
   ```

---

### 3.3. Bù sai số điểm không (Offset Compensation)
Quá trình bù Offset trong BMI270 được thực hiện trực tiếp trên **dữ liệu trước lọc (pre-filtered data)** và giá trị sau bù được áp dụng cho cả nhánh dữ liệu pre-filtered (OIS/FIFO unfiltered) lẫn nhánh dữ liệu đã lọc (Filtered DATA/FIFO). Kết quả tính toán có cơ chế bão hòa (saturation) tự động để chống tràn số (*Mục 4.13 Offset Compensation, Trang 70*).

#### A. Bù Offset cho Gia tốc kế (Accelerometer Offset Compensation)
* **Cấu trúc thanh ghi (`OFFSET_0` .. `OFFSET_2`, địa chỉ `0x71` .. `0x73`):**
  * `OFFSET_0` (`0x71`): `off_acc_x` (8-bit bù 2 có dấu cho trục X).
  * `OFFSET_1` (`0x72`): `off_acc_y` (8-bit bù 2 có dấu cho trục Y).
  * `OFFSET_2` (`0x73`): `off_acc_z` (8-bit bù 2 có dấu cho trục Z) (*Mục 4.13, Trang 70; Mục 5.2, Trang 126–127*).
* **Độ phân giải và Dải bù:** Độ phân giải (LSB) là **$3.9\ \text{mg/LSB}$**, dải bù là **$\pm 0.5\ g$** (độc lập với thang đo `acc_range` đang chọn) (*Mục 4.13, Trang 70*).
* **Kích hoạt:** Phải đặt bit **`NV_CONF.acc_off_en = 0b1`** (thanh ghi `0x70` bit 3). Lưu ý: Ngay sau khi sửa giá trị trong `OFFSET_0`..`OFFSET_2`, mẫu dữ liệu kế tiếp sẽ không hợp lệ (invalid) (*Mục 4.13, Trang 70; Mục 5.2, Trang 126*).
* **Hai phương thức thực hiện:**
  1. **Manual Offset Compensation:** Đo độ lệch tĩnh thực tế khi đặt cảm biến nằm ngang phẳng, quy đổi ra đơn vị $3.9\ \text{mg/LSB}$ rồi ghi trực tiếp vào `OFFSET_0`..`OFFSET_2` qua SPI/I²C và bật `NV_CONF.acc_off_en = 0b1` (*Mục 4.13, Trang 70*).
  2. **Fast Offset Compensation - FOC (Semi-Automatic):** Thực hiện hiệu chỉnh thủ công 1 lần để tìm giá trị bù chuẩn cho từng mạch PCB, sau đó ghi vĩnh viễn vào bộ nhớ **NVM**. Mỗi khi chip khởi động lại (POR/Soft-reset), giá trị từ NVM tự động nạp vào các thanh ghi ảnh `OFFSET_0`..`OFFSET_2` (*Mục 4.13, Trang 70*).

#### B. Bù Offset cho Con quay hồi chuyển (Gyroscope Offset Compensation)
* **Cấu trúc thanh ghi 10-bit (`OFFSET_3` .. `OFFSET_6`, địa chỉ `0x74` .. `0x77`):**
  * Mỗi trục Gyroscope có từ khóa bù rộng **10-bit bù 2 có dấu (two's complement)**, được tách thành 8 bit thấp (`7..0`) và 2 bit cao (`9..8`) (*Mục 4.13 Gyroscope, Trang 70; Mục 5.2, Trang 76, 127–128*):
    * Trục X: `OFFSET_3` (`0x74`) chứa `gyr_usr_off_x_7_0`; `OFFSET_6` (`0x77` bits `1..0`) chứa `gyr_usr_off_x_9_8`.
    * Trục Y: `OFFSET_4` (`0x75`) chứa `gyr_usr_off_y_7_0`; `OFFSET_6` (`0x77` bits `3..2`) chứa `gyr_usr_off_y_9_8`.
    * Trục Z: `OFFSET_5` (`0x76`) chứa `gyr_usr_off_z_7_0`; `OFFSET_6` (`0x77` bits `5..4`) chứa `gyr_usr_off_z_9_8`.
* **Độ phân giải và Dải bù:** Độ phân giải (LSB) là **$61\ \text{mdps/LSB}$** ($0.061\ \text{dps/LSB}$), dải bù là **$\pm 31\ \text{dps}$** (độc lập với thang đo `gyr_range`) (*Mục 4.13, Trang 70*).
* **Kích hoạt:** Phải đặt bit **`OFFSET_6.gyr_off_en = 0b1`** (thanh ghi `0x77` bit 6). Tương tự Accel, mẫu dữ liệu ngay sau khi ghi `OFFSET_3`..`OFFSET_6` sẽ không hợp lệ (*Mục 4.13, Trang 70; Mục 5.2, Trang 128*).
* **Ba phương thức thực hiện:**
  1. **Manual Offset Compensation:** Thu thập trung bình vận tốc góc khi hệ thống đứng yên, chia cho $0.061\ \text{dps}$ để lấy giá trị bù 10-bit, ghi vào `OFFSET_3`..`OFFSET_6` và bật `OFFSET_6.gyr_off_en = 0b1` (*Mục 4.13, Trang 70*).
  2. **Fast Offset Compensation - FOC (Semi-Automatic):** Xác định giá trị bù thông qua Manual Offset Compensation rồi lưu cố định vào bộ nhớ **NVM** để tự nạp lại sau mỗi lần reset (*Mục 4.13, Trang 71*).
  3. **In-use Offset Compensation - IOC (Full-Automatic Offset Compensation):**
     * Để khử hiện tượng trôi điểm không (offset drift) do ứng suất nhiệt cơ (thermomechanical stress) trong quá trình bay/vận hành thực tế, BMI270 tích hợp sẵn bộ **IOC chạy ngầm hoàn toàn tự động** song song với quá trình đo bình thường mà không cần Host can thiệp (*Mục 4.13 In-use Offset Compensation IOC, Trang 71*).
     * **Điều kiện bắt buộc để IOC hoạt động:** Cả 5 điều kiện sau phải đồng thời thỏa mãn (nếu thiếu 1 trong 5 điều kiện, IOC sẽ tự tắt):
       1. Bit `GEN_SET_1.gyr_self_off = 1` (Page 1, thanh ghi `0x34`, bit 9) (*Mục 4.13, Trang 71; Mục 5.2, Trang 97–98*).
       2. Bit `OFFSET_6.gyr_off_en = 1` (thanh ghi `0x77`, bit 6).
       3. Accelerometer đang được bật (từ Primary Interface hoặc OIS Interface).
       4. Gyroscope đang được bật từ Primary Interface (`PWR_CTRL.gyr_en = 1`).
       5. Gyroscope đang **tắt** trên OIS Interface (`OIS_CTRL_S.gyr_en = 0`) (*Mục 4.13, Trang 71*).
     * **Lưu ý:** Khi IOC đang bật, chip tự động cập nhật `OFFSET_3`..`OFFSET_6`, do đó **Host tuyệt đối không được ghi đè vào các thanh ghi `OFFSET_3`..`OFFSET_6`**. Nếu muốn tắt IOC để can thiệp thủ công, hãy xóa bit `GEN_SET_1.gyr_self_off = 0` (*Mục 4.13, Trang 71*).

---

### 3.4. Bù sai số độ nhạy / hệ số khuếch đại (Sensitivity Error Compensation)
* **Đối với Accelerometer:** Đã được thiết kế tối ưu phần cứng với sai số độ nhạy cực thấp ($\pm 0.4\%$) nên không cần quy trình hiệu chỉnh thêm (*Mục 1 Table 2, Trang 13; Mục 4.14 Accelerometer, Trang 72*).
* **Đối với Gyroscope:** Sai số độ nhạy ban đầu sau khi hàn lên mạch là $\pm 2\%$. Để bù sai số độ nhạy (ví dụ do biến dạng nhiệt khi hàn reflow), BMI270 cung cấp 2 phương pháp giúp giảm sai số xuống chỉ còn **$\pm 0.4\%$** (*Mục 1 Table 3, Trang 15; Mục 4.14 Gyroscope, Trang 72–73*):

#### Phương pháp 1: Hiệu chỉnh tự động không cần chuyển động – CRT (Component ReTrimming)
Đây là tính năng cực kỳ mạnh mẽ của BMI270, cho phép hiệu chỉnh lại độ nhạy (gain) của Gyroscope **ngay khi thiết bị đang đứng yên (motionless)** mà không cần bàn xoay hiệu chuẩn (*Mục 4.14 Component ReTrimming Feature CRT, Trang 73*).

**Quy trình 13 bước thực thi CRT chuẩn theo Datasheet (*Mục 4.14, Trang 73*):**
1. Thực hiện **Soft-Reset** (ghi `0xB6` vào `CMD` `0x7E`, chờ $\ge 2\ \text{ms}$) hoặc **Power-On Reset (POR)** (*Mục 4.14, Trang 73; Mục 4.17, Trang 74*).
2. Thực hiện đầy đủ **Quy trình khởi tạo thiết bị** (nạp 8 kB config file theo Mục 4.4) (*Mục 4.14, Trang 73*).
3. Tắt chế độ tiết kiệm năng lượng nâng cao: Ghi `PWR_CONF.adv_power_save = 0b0` (`0x7C`), sau đó chờ **450 µs** (*Mục 4.14, Trang 73*).
4. Bật Accelerometer (để chip tự giám sát rung động): Ghi `PWR_CTRL.acc_en = 0b1` (`0x7D` bit 2 = 1, lưu ý giữ Gyroscope tắt `gyr_en = 0b0`) (*Mục 4.14, Trang 73*).
5. **Đảm bảo thiết bị đứng yên hoàn toàn** trong suốt quá trình chạy CRT (nếu phát hiện chuyển động, CRT sẽ tự hủy và báo mã lỗi `0x03` tại `GYR_GAIN_STATUS.g_trig_status`) (*Mục 4.14, Trang 73*).
6. Ghi `GYR_CRT_CONF.crt_running = 0b1` (thanh ghi `0x69`, bit 2 = 1) (*Mục 4.14, Trang 73; Mục 5.2, Trang 123*).
7. Chuyển `FEAT_PAGE` (`0x2F`) sang **Page 1** (`0x01`), sau đó đặt bit `G_TRIG_1.select = 1` (thanh ghi `0x32` bit 8 = `1` để chọn tính năng `crt` thay vì `gyr_bist`). Lưu ý quy tắc ghi chẵn/lẻ 16-bit word của vùng thanh ghi `FEATURES` `0x30`..`0x3F` (*Mục 4.8 Global Configuration, Trang 46; Mục 4.14, Trang 73; Mục 5.2, Trang 96*).
8. Đặt bit `G_TRIG_1.block = 0` (thanh ghi `0x32` bit 9 = `0` để unblock lệnh trigger) (*Mục 4.14, Trang 73; Mục 5.2, Trang 96*).
9. Gửi lệnh **`g_trigger`** bằng cách ghi giá trị **`0x02`** vào thanh ghi `CMD` (`0x7E`) (*Mục 4.14, Trang 73; Mục 5.2, Trang 130*).
10. Chờ quá trình CRT hoàn tất: Poll thanh ghi `GYR_CRT_CONF` (`0x69`) cho đến khi phần cứng tự xóa bit **`crt_running == 0b0`** (*Mục 4.14, Trang 73; Mục 5.2, Trang 123*).
11. Chuyển `FEAT_PAGE` (`0x2F`) về **Page 0** (`0x00`) và đọc thanh ghi `GYR_GAIN_STATUS` (`0x38`): Kiểm tra trường `g_trig_status` (bits `5..3`) phải bằng **`0x00` (`no_err`)** (nếu bằng `0x01`: lỗi điều kiện tiên quyết `precon_err`; `0x02`: lỗi tải luồng cấu hình `dl_err`; `0x03`: bị hủy do phát hiện chuyển động `abort_err`) (*Mục 4.14, Trang 73; Mục 5.2, Trang 95*).
12. *(Tùy chọn)* Lưu giá trị Gain mới của Gyroscope vào bộ nhớ không mất dữ liệu **NVM** (xem Mục 3.5 bên dưới) (*Mục 4.14, Trang 73*).
13. Giá trị Gain mới sẽ tự động được áp dụng ở lần bật Gyroscope kế tiếp (`PWR_CTRL.gyr_en = 0b1`). Hãng khuyến nghị chạy CRT tại nhiệt độ vận hành thực tế của thiết bị để đạt sai số nhỏ nhất (*Mục 4.14, Trang 73*).

#### Phương pháp 2: Hiệu chỉnh độ nhạy thủ công (Manual SENS Error Compensation)
Sử dụng khi có bàn xoay chuẩn tạo vận tốc góc tham chiếu $\omega_r$ để so sánh với vận tốc góc đo được $\omega_m$ của cảm biến (sau khi đã bù Offset) (*Mục 4.14 Manual SENS Error Compensation, Trang 72*):
1. **Tính tỷ số độ nhạy:** Trên từng trục $x, y, z$, tính tỷ số $R = \omega_{r} / \omega_{m}$ trong dải cho phép từ **$0.75$ đến $1.25$** (tức $1 \pm 25\%$).
2. **Mã hóa sang số dấu phẩy tĩnh 11-bit `Q(1,10)` (`FxP 1.10`):**
   * Độ phân giải: $2^{-10} = 0.0009765625$ ($\approx 0.09765\%$).
   * Công thức mã hóa 11-bit (`b10..b0`): $\text{Code}_{11} = \text{round}(R \times 1024)$. Ví dụ (*Mục 4.14, Trang 72; Mục 5.2, Trang 98*):
     * $R = 1.25$ ($1 + 25\%$) $\rightarrow$ `0b10100000000` (`0x500`).
     * $R = 1.0009765$ ($1 + 0.09765\%$) $\rightarrow$ `0b10000000001` (`0x401`).
     * $R = 0.9990234$ ($1 - 0.09765\%$) $\rightarrow$ `0b01111111111` (`0x3FF`).
     * $R = 0.75$ ($1 - 25\%$) $\rightarrow$ `0b01100000000` (`0x300`).
3. **Trình tự nạp Gain thủ công:**
   * Bật bit cho phép bù Gain: `OFFSET_6.gyr_gain_en = 0b1` (`0x77` bit 7) (*Mục 4.14, Trang 72; Mục 5.2, Trang 128*).
   * Tắt Gyroscope: `PWR_CTRL.gyr_en = 0b0` (`0x7D` bit 1) (*Mục 4.14, Trang 72*).
   * Chọn **Page 1** (`FEAT_PAGE = 0x01`), ghi các giá trị 11-bit `ratio_x`, `ratio_y`, `ratio_z` vào 3 thanh ghi 16-bit: `GYR_GAIN_UPD_1` (`0x36`), `GYR_GAIN_UPD_2` (`0x38`), `GYR_GAIN_UPD_3` (`0x3A` bits `10..0`), đồng thời bật bit **`GYR_GAIN_UPD_3.enable = 1`** (`0x3A` bit 11) (*Mục 4.14, Trang 72; Mục 5.2, Trang 98*).
   * Phát lệnh **`usr_gain`** bằng cách ghi **`0x03`** vào thanh ghi `CMD` (`0x7E`) (*Mục 4.14, Trang 72; Mục 5.2, Trang 130*).
   * Chờ bit `GYR_GAIN_UPD_3.enable` tự xóa về `0`, kiểm tra xem có trục nào bị bão hòa hệ số bù qua các bit `sat_x`, `sat_y`, `sat_z` (`GYR_GAIN_STATUS` Page 0 `0x38` bits `0..2`), sau đó bật lại Gyroscope (`PWR_CTRL.gyr_en = 0b1`) (*Mục 4.14, Trang 72; Mục 5.2, Trang 95*).

---

### 3.5. Lưu trữ tham số hiệu chỉnh vào bộ nhớ NVM (Non-Volatile Memory)
Các thanh ghi `NV_CONF` (`0x70`), `OFFSET_0`..`OFFSET_6` (`0x71`..`0x77`), `AUX_IF_TRIM` (`0x68`), `DRV` (`0x6C`) cùng các thanh ghi bù độ nhạy Gyroscope (CRT/Manual Gain) đều có vùng sao lưu trong bộ nhớ không mất dữ liệu **NVM** (*Mục 4.15 Non-Volatile Memory, Trang 73*).

> **Cảnh báo giới hạn phần cứng:** Mỗi lần ghi NVM sẽ làm mới toàn bộ nội dung NVM và **chỉ giới hạn tối đa 14 chu kỳ ghi (write-cycles)** trong suốt vòng đời linh kiện (*Mục 1 Table 1, Trang 13; Mục 4.15, Trang 74*).

**Quy trình ghi vào NVM (*Mục 4.15, Trang 73–74*):**
1. Tắt chế độ tiết kiệm năng lượng: Đặt `PWR_CONF.adv_power_save = 0b0` (`0x7C` bit 0 = 0) và chờ tối thiểu 450 µs.
2. Ghi toàn bộ các giá trị hiệu chỉnh mới vào các thanh ghi ảnh (ví dụ `OFFSET_0`..`OFFSET_6`, `NV_CONF`).
3. Chuẩn bị ghi NVM: Chọn **Page 1** (`FEAT_PAGE = 0x01`), đặt bit `GEN_SET_1.nvm_prog_prep = 0b1` (thanh ghi `0x34` bit 10 = 1) (*Mục 4.15, Trang 73; Mục 5.2, Trang 98*).
4. Chờ **40 ms**.
5. Mở khóa NVM: Ghi `0b1` vào bit `NVM_CONF.nvm_prog_en` (thanh ghi `0x6A` bit 1 = 1) (*Mục 4.15, Trang 73; Mục 5.2, Trang 124*).
6. Kích hoạt quá trình ghi: Ghi lệnh **`nvm_prog` (`0xA0`)** vào thanh ghi `CMD` (`0x7E`) (*Mục 4.15, Trang 73; Mục 5.2, Trang 130*).
7. Theo dõi trạng thái hoàn tất: Đọc thanh ghi `STATUS` (`0x03`) kiểm tra bit `cmd_rdy` (bit 4). Khi `cmd_rdy == 0b0` là đang ghi; khi **`cmd_rdy == 0b1`** là đã ghi xong. Sau khi hoàn tất, thực hiện cấp nguồn lại (POR) hoặc Soft-Reset (`0xB6` vào `CMD`) (*Mục 4.15, Trang 73–74*).

---

### 3.6. Quy trình tự kiểm tra phần cứng (Sensor Self-Test) và Chuyển đổi trục (Axis Remapping)

#### A. Tự kiểm tra Gia tốc kế (Accelerometer Self-Test) (*Mục 4.12 Accelerometer, Trang 68*)
Kích hoạt lực tĩnh điện nội bộ làm lệch khối MEMS để kiểm tra cơ học và đường truyền tín hiệu:
1. Bật Accelerometer (`PWR_CTRL.acc_en = 0b1`), chọn thang đo **$\pm 16g$** (`ACC_RANGE.acc_range = 0x03`).
2. Đặt biên độ Self-Test mức cao: `ACC_SELF_TEST.acc_self_test_amp = 0b1` (`0x6D` bit 3 = 1) (*Mục 4.12, Trang 68; Mục 5.2, Trang 125*).
3. Cấu hình `ACC_CONF` (`0x40`): `acc_odr = 1600 Hz` (`0x0C`), `acc_bwp = norm_avg4` (`0x02`), `acc_filter_perf = 0b1`. Chờ **> 2 ms**.
4. **Đo cực dương (Positive polarity):** Đặt `acc_self_test_sign = 0b1` (`0x6D` bit 2 = 1) và bật `acc_self_test_en = 0b1` (`0x6D` bit 0 = 1). Chờ **> 50 ms**, sau đó đọc và lưu giá trị gia tốc 3 trục $A_{pos(x,y,z)}$ từ `DATA_8`..`DATA_13`.
5. **Đo cực âm (Negative polarity):** Đặt `acc_self_test_sign = 0b0` (`0x6D` bit 2 = 0) và giữ `acc_self_test_en = 0b1`. Chờ **> 50 ms**, đọc và lưu giá trị gia tốc 3 trục $A_{neg(x,y,z)}$.
6. **Đánh giá kết quả:** Tắt Self-Test (`acc_self_test_en = 0b0`) và khởi động lại thiết bị (Reset). Cảm biến đạt chuẩn nếu hiệu số $\Delta A = A_{pos} - A_{neg}$ thỏa mãn ngưỡng tối thiểu (*Mục 4.12, Trang 68*):
   * Trục X: $\Delta A_x > +16\ g$ (tức $> +32768\ \text{LSB}$ ở thang $\pm 16g$).
   * Trục Y: $\Delta A_y < -15\ g$ (tức $< -30720\ \text{LSB}$ ở thang $\pm 16g$).
   * Trục Z: $\Delta A_z > +10\ g$ (tức $> +20480\ \text{LSB}$ ở thang $\pm 16g$).

#### B. Tự kiểm tra Con quay hồi chuyển (Gyroscope Self-Test) (*Mục 4.12 Gyroscope, Trang 69*)
Kiểm tra toàn diện cơ cấu kích dao động (drive test), cơ cấu cảm nhận (sense test) và đường truyền dữ liệu (datapath test):
1. Thực hiện Soft-Reset hoặc POR, sau đó chạy quy trình khởi tạo 8 kB config file (Mục 4.4).
2. Tắt APS (`PWR_CONF.adv_power_save = 0b0`), chờ **450 µs**, bật Accelerometer (`PWR_CTRL.acc_en = 0b1`) và giữ cảm biến đứng yên.
3. Đảm bảo `G_TRIG_1.select = 0` (Page 1, `0x32` bit 8 = 0: chọn `gyr_bist` – mặc định sau reset) (*Mục 5.2, Trang 96*).
4. Gửi lệnh **`g_trigger` (`0x02`)** vào thanh ghi `CMD` (`0x7E`).
5. Chờ bit `GYR_SELF_TEST_AXES.gyr_st_axes_done == 0b1` (`0x6E` bit 0). Kiểm tra `GYR_GAIN_STATUS.g_trig_status == 0x00` (Page 0, `0x38`) và cả 3 bit `gyr_axis_x_ok`, `gyr_axis_y_ok`, `gyr_axis_z_ok` trong thanh ghi `0x6E` (bits `1..3`) đều bằng **`0b1`** (*Mục 4.12, Trang 69; Mục 5.2, Trang 125*).
*(Lưu ý: Riêng bài kiểm tra Drive Test cũng được chip tự động chạy mỗi khi bật Gyroscope; nếu lỗi sẽ bật cờ `ERR_REG.fatal_err` `0x02` bit 0 tối đa sau 320 ms).*

#### C. Quy tắc ánh xạ lại trục tọa độ (Axis Remapping)
* Nếu hướng gắn chip BMI270 trên mạch điều khiển (Flight Controller / PCB) khác với hệ tọa độ mặc định của thiết bị, BMI270 cho phép ánh xạ lại trục và đảo dấu từng trục thông qua các trường `map_x_axis`, `map_x_axis_sign`, `map_y_axis`, `map_y_axis_sign`, `map_z_axis`, `map_z_axis_sign` trong thanh ghi **`GEN_SET_1`** (Page 1, địa chỉ `0x34`, bits `8..0`) (*Mục 4.8 Axis remapping for interrupt features, Trang 47; Mục 5.2, Trang 97*).
* **Lưu ý cực kỳ quan trọng khi viết Firmware:** Cấu hình `GEN_SET_1` **chỉ áp dụng cho dữ liệu đưa vào bộ xử lý tính năng thông minh (Advanced Features / Interrupts)**, hoàn toàn **không làm thay đổi** dữ liệu trong các thanh ghi `DATA_0`..`DATA_19` và `FIFO`. Đối với dữ liệu đọc từ `DATA_0`..`DATA_19` hoặc `FIFO`, bạn bắt buộc phải tự hoán đổi trục và dấu ở tầng Driver phần mềm (*Mục 4.8 Note, Trang 47; Mục 8.2, Trang 156*).
