# Hướng dẫn hoàn thiện driver PMW3901 và BMI270

Ngày rà soát: 07/11/2026. Tài liệu này bám theo source hiện có trong dự án, hướng dẫn để bạn tự viết tiếp driver; các đoạn khai báo bên dưới là API đề xuất, chưa được triển khai vào firmware.

## 1. Cheat sheet hiện tại đã đủ chưa?

**Đủ làm nền tảng tra API ESP-IDF để bắt đầu viết hai driver. Chưa đủ làm tài liệu duy nhất để xây dựng toàn bộ firmware drone.** Không cần chép toàn bộ API của ESP-IDF vào cheat sheet: cần bổ sung đúng phần mà phần cứng và kiến trúc của dự án sử dụng.

Đã kiểm tra cấu hình tại máy: PlatformIO `espressif32` 7.1.3, framework ESP-IDF 6.1.0, target `esp32`, board cấu hình `nodemcu-32s`. Phiên bản SDK đang cài khớp với mốc v6.1 của cheat sheet. Tuy nhiên, `platformio.ini` chưa ghim phiên bản platform nên lần cài môi trường mới có thể khác.

### Những phần đã có và dùng được

| Nhóm trong cheat sheet | Áp dụng trong dự án |
| --- | --- |
| 01–02: kiểu dữ liệu, con trỏ, handle, C/C++, header và component | Đọc chữ ký hàm; chia `.h/.c` và `.hpp/.cpp`; cấu hình CMake |
| 03–06: GPIO, SPI, I2C, UART | Bus cảm biến, CS, chân ngắt, giao tiếp ngoại vi |
| 07–09: timestamp, ESP Timer, GPTimer, FreeRTOS | Chu kỳ đọc, đánh thức task, queue, mutex |
| 10–13: LEDC, MCPWM, RMT, ADC | Nền tảng phát xung và đo điện áp |
| 14–17: lỗi, log, NVS, watchdog, heap/DMA | Trả lỗi, lưu calibration, quản lý tài nguyên |
| 18: tích hợp và chẩn đoán | Ownership, lỗi link, buffer lifetime, dữ liệu quá hạn |

Tài liệu đã nhắc tới mutex, ISR, notification gộp sự kiện, tick làm tròn, timestamp, DMA, dữ liệu quá hạn và calibration trong NVS. Những phần này **cần cụ thể hóa cho cảm biến**, không phải hoàn toàn chưa được đề cập.

### Những phần đề xuất bổ sung — chưa sửa cheat sheet

| Ưu tiên | Còn thiếu hoặc mới nói ở mức nguyên tắc | Nội dung nên viết thêm |
| --- | --- | --- |
| Cần ngay | Quy trình riêng của IC | Register map, bit đọc/ghi, CS/timing, reset, kiểm tra ID, bảng init PMW3901; port và config image BMI270 |
| Cần ngay | Hợp đồng dữ liệu hoàn chỉnh | `flow_sample_t`, `imu_sample_t`, cấu hình và health: đơn vị, hệ trục, timestamp lấy mẫu/đọc xong, sequence, validity, lỗi và tuổi mẫu |
| Cần ngay | Giải mã và hiệu chỉnh | Ghép byte có dấu, endian, scale theo range, đổi trục sensor → thân, gyro bias, hiệu chuẩn accel, chất lượng optical flow |
| Cần trước tích hợp | Lịch chạy có số liệu | ODR, băng thông bus, thời gian giữ bus, deadline, DRDY/FIFO, tràn queue và phục hồi sau reset |
| Hữu ích khi cần | API RTOS mở rộng | `xQueueSendFromISR`, `xQueueOverwriteFromISR`, `uxTaskGetStackHighWaterMark`; các bản tạo task/queue tĩnh nếu chọn cấp phát tĩnh |
| Cần trước sử dụng thực tế | Kiểm thử driver | Test decode, lỗi giao tiếp, init thất bại từng bước, waveform CS/SCLK, đọc liên tục và chạy hai cảm biến chung bus |
| Phụ thuộc chức năng drone | Giao thức và tầng ứng dụng | Driver RC/ESC thực tế, parser/CRC, estimator, bộ điều khiển, mixer, giới hạn đầu ra, health/failsafe và log phục vụ phân tích |

Các kiểu như `imu_sample_t`, trạng thái cảm biến, quaternion, setpoint và cấu hình phương tiện là kiểu **do dự án định nghĩa**, không phải kiểu ESP-IDF còn thiếu. `README.md` đã mô tả kiến trúc FSM và module; bước tiếp theo là chốt interface và hiện thực chúng. Wi-Fi, ESP-NOW, CAN, SD, OTA… chỉ cần thêm nếu dự án thực sự dùng.

Một ví dụ nên chỉnh trước khi tái sử dụng: mục 11.1 của cheat sheet tạo timer/operator/comparator/generator MCPWM nhưng chỉ trả handle comparator. Mẫu này minh họa phát xung được, song ứng dụng không giữ đủ handle để dừng và giải phóng toàn bộ tài nguyên. Nên bổ sung một context chứa cả bốn handle cùng hàm cleanup khi mở rộng phần đó.

Phạm vi rà soát là độ phủ nhóm API/kiểu, các ví dụ liên quan và đối chiếu source hai driver. Đây không phải xác nhận đã biên dịch mọi snippet hoặc đã kiểm chứng trên phần cứng. File cheat sheet và các file source hiện có được giữ nguyên trong lần viết hướng dẫn này.

## 2. Hiện trạng source và thứ tự làm

| File | Hiện trạng |
| --- | --- |
| `src/drivers/optical_flow/pmw3901mb_txqt.hpp` | Có class/struct `PMW3901`; khai báo `registerRead`, `initRegister`, `readMotionCount` nhưng chưa có implementation |
| `src/drivers/optical_flow/pmw3901mb_txqt.cpp` | Có constructor, tạo SPI bus/device và ghi thanh ghi; chưa reset, nhận diện hay khởi tạo IC |
| `src/drivers/imu/bmi270/bmi270.h` | Mới có include guard và `CHIP_ID 0x00`; cần phân biệt địa chỉ thanh ghi với giá trị ID |
| `src/drivers/imu/bmi270/bmi270.c` | Trống |
| `src/CMakeLists.txt` | Chỉ đăng ký `main.cpp` và PMW3901 `.cpp`; chưa có BMI270 |
| `src/main.cpp` | `app_main()` còn trống, chưa gọi driver |

Thứ tự thực hiện:

1. Chốt chân, giao tiếp và ownership bus.
2. Hoàn thiện PMW3901 đến mức đọc được ID và chuyển động.
3. Hoàn thiện BMI270 đến mức init thành công, đọc được accel/gyro.
4. Thêm DRDY/FIFO, timestamp, calibration và health.
5. Chạy hai cảm biến đồng thời, đo mất mẫu và độ trễ trước khi nối vào estimator.

## 3. Chuẩn bị chung

### 3.1. Chốt phần cứng, chưa gán chân theo phỏng đoán

Điền bảng này theo sơ đồ board/module bạn dùng:

| Thông tin | PMW3901 | BMI270 |
| --- | --- | --- |
| Model module và sơ đồ chân | Chưa xác định | Chưa xác định |
| Nguồn cấp module và mức logic I/O | Theo module | Theo module |
| Bus | SPI | Hướng dẫn chính dùng SPI; I2C là nhánh tùy chọn |
| Host và MOSI/MISO/SCLK | Chưa chốt | Chung host hoặc host riêng |
| CS | GPIO output riêng | GPIO output riêng |
| Ngắt | Theo module, nếu sử dụng | Nên đưa INT1 ra GPIO ESP32 |
| Hướng lắp | Trục quang học và X/Y | Trục X/Y/Z |

Các chân `MOSI=1`, `MISO=2`, `SCLK=3` trong PMW3901 hiện chỉ là placeholder. Trên ESP32 thường, GPIO1/3 liên quan UART0 và GPIO2 là chân strapping; cần đối chiếu board trước khi dùng. Tránh chọn chân flash hoặc chân chỉ có khả năng input làm output. Xem [GPIO ESP32](https://docs.espressif.com/projects/esp-idf/en/v6.1/esp32/api-reference/peripherals/gpio.html).

### 3.2. Tách bus khỏi driver IC

Cho tầng board/platform tạo SPI bus đúng một lần. Mỗi driver nhận host đã tạo, đăng ký device của mình và giữ handle. Driver chỉ remove device của nó; tầng sở hữu bus mới được `spi_bus_free()` sau khi mọi device đã dừng.

Chọn một task sở hữu mỗi driver ở bản đầu. Nếu nhiều task gọi cùng device, mutex phải bao trọn thao tác logic như đọc một mẫu hoặc đổi bank rồi đọc; `spi_device_acquire_bus()` chỉ ngăn device khác chen bus. Xem [SPI Master ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/v6.1/esp32/api-reference/peripherals/spi_master.html).

Constructor chỉ lưu cấu hình, không truy cập phần cứng. `init()` thực hiện giao tiếp và trả lỗi. `deinit()` tháo tài nguyên sau khi task/ISR ngừng sử dụng. Định nghĩa rõ gọi `init()` lần hai sẽ trả lỗi trạng thái hay giữ nguyên thiết bị đã khởi tạo.

### 3.3. Quy ước trạng thái và thời gian

- Hàm đọc nên trả `esp_err_t`, dữ liệu qua con trỏ output. Không dùng byte `0` làm dấu hiệu lỗi.
- Chỉ xuất mẫu khi toàn bộ giao dịch cần thiết thành công; caller không dùng lại output cũ sau lỗi.
- Phân biệt `device_added` với `initialized`: thêm device SPI thành công chưa chứng minh IC hoạt động.
- Giữ `last_success_us`, tổng lỗi giao tiếp, lỗi liên tiếp, số lần reset và số mẫu bỏ lỡ ở context/health.
- `timestamp_us` là thời gian đơn điệu; quy định rõ đó là DRDY, thời điểm ước lượng lấy mẫu hay đọc xong.

Trong `sdkconfig.nodemcu-32s`, `CONFIG_FREERTOS_HZ=100`: tick dài 10 ms; `pdMS_TO_TICKS(1)` và `(5)` có thể thành 0. Với delay tối thiểu của IC, dùng helper kiểm tra deadline bằng `esp_timer_get_time()`: khoảng dài cho task ngủ rồi kiểm tra lại thời gian, khoảng ngắn mới busy-wait bằng `esp_rom_delay_us()`. Không giữ bus trong thời gian chờ khởi động dài khi CS đã về HIGH.

## 4. Hoàn thiện PMW3901

### Bước P1 — Sửa thiết kế CS trước khi thêm hàm đọc

Hiện tại `dev_cfg.spics_io_num = m_csPin`, nhưng `registerWrite()` lại tự `gpio_set_level()` và chia địa chỉ/dữ liệu thành hai transaction. Hai cơ chế cùng sở hữu CS khiến frame không được bảo đảm liên tục.

Với cách viết hiện tại, chọn **CS thủ công**:

1. Đặt `dev_cfg.spics_io_num = -1`.
2. Cấu hình CS là GPIO output, mức nghỉ HIGH; kiểm tra GPIO hợp lệ.
3. Giữ bus trước khi kéo CS LOW; giữ LOW qua cả pha địa chỉ, khoảng chờ và dữ liệu.
4. Mọi nhánh sau acquire thành công đều phải đưa CS HIGH và release bus, kể cả khi truyền lỗi.
5. Giữ `mode = 3`; để `command_bits/address_bits/dummy_bits = 0` khi tự gửi các byte qua transaction.

Ở bản đầu có thể giữ clock 100 kHz để kiểm tra dây; đó chưa phải tốc độ đã đánh giá cho lấy mẫu liên tục. Không vừa cấu hình command/address phần cứng vừa gửi lại cùng byte địa chỉ trong `tx_data`.

### Bước P2 — Chốt API có trả lỗi

Đề xuất sửa các khai báo trong `.hpp` rồi hiện thực lần lượt:

```cpp
// Các khai báo thành viên đề xuất; chưa phải class hoàn chỉnh.
esp_err_t init(spi_host_device_t host); // Host đã được tầng board tạo.
esp_err_t deinit();
esp_err_t registerWrite(uint8_t reg, uint8_t value);
esp_err_t registerRead(uint8_t reg, uint8_t *out);
esp_err_t initRegister();
esp_err_t readMotionCount(int16_t *deltaX, int16_t *deltaY);
```

Hàm đọc kiểm tra con trỏ, handle và trạng thái khởi tạo. Hai delta phải được giải mã vào biến tạm trước khi gán ra ngoài; lỗi giữa chừng không được tạo mẫu nửa mới nửa cũ. Nếu class sở hữu handle, tạm cấm copy/assignment để không có hai đối tượng cùng giải phóng một device.

### Bước P3 — Viết `registerRead()`

Trình tự tham khảo: acquire bus → CS LOW → chờ → gửi địa chỉ với bit 7 bằng 0 → chờ → phát một byte để nhận → chờ → CS HIGH → release. PMW3901 dùng bit 7 bằng 1 khi ghi. Lấy timing từ một bản tham chiếu nhất quán; `registerRead()` trong [driver Arduino của Bitcraze](https://github.com/bitcraze/Bitcraze_PMW3901/blob/master/src/Bitcraze_PMW3901.cpp) là điểm bắt đầu phù hợp với repo đã dẫn trong `reference.md`.

Ánh xạ sang ESP-IDF:

- Transaction địa chỉ: `length = 8`, `SPI_TRANS_USE_TXDATA`.
- Transaction dữ liệu: descriptor mới, `length = 8`, cả `SPI_TRANS_USE_TXDATA | SPI_TRANS_USE_RXDATA`; byte TX bằng 0, đọc `rx_data[0]` khi thành công.
- `spi_device_polling_transmit()` là đồng bộ, vì vậy descriptor cục bộ dùng được trong phạm vi lời gọi.
- Chưa cần queue/DMA phức tạp cho từng byte; giữ cách thực hiện dễ quan sát bằng logic analyzer.

### Bước P4 — Khởi tạo IC, không chỉ khởi tạo SPI

Phân biệt hằng địa chỉ và giá trị mong đợi:

| Thanh ghi | Địa chỉ | Giá trị/ý nghĩa |
| --- | --- | --- |
| Product ID | `0x00` | Mong đợi `0x49` |
| Inverse Product ID | `0x5F` | Mong đợi `0xB6` |
| Power-up reset | `0x3A` | Ghi `0x5A` |
| Motion và delta | `0x02`–`0x06` | Đọc xóa dữ liệu ban đầu |

Theo trình tự `begin()` của [Bitcraze](https://github.com/bitcraze/Bitcraze_PMW3901/blob/master/src/Bitcraze_PMW3901.cpp): reset giao tiếp bằng CS, reset IC, chờ, kiểm tra hai ID, đọc bỏ motion ban đầu, rồi chạy bảng `initRegisters()`. Giữ nguyên thứ tự đổi bank, ghi và delay trong bảng tham chiếu; không trộn bảng init từ nhiều repo. Ghi nguồn/revision và giữ license khi đưa bảng vào source.

Trong bản ESP-IDF của bạn, mỗi bước phải kiểm tra lỗi; chỉ đặt `initialized=true` sau bước cuối. Không bỏ qua lỗi ghi giữa bảng init. Hàm `init_SPI_PMW3901()` hiện tại chỉ mới xử lý bus/device nên chưa đáp ứng điều kiện này.

### Bước P5 — Đọc chuyển động và giải mã có dấu

Ở bước kiểm tra ban đầu, đọc motion rồi các byte delta theo trình tự tham chiếu. Khi tích hợp liên tục, nên làm thêm `readMotionBurst()` để lấy delta và thông tin chất lượng cùng một lượt.

Burst tại `0x16` gồm 12 byte: motion, observation, X thấp/cao, Y thấp/cao, SQUAL, raw sum, raw max, raw min, shutter cao/thấp. Delta là little-endian; shutter có thứ tự ngược. Đối chiếu [hàm đọc burst](https://github.com/bitcraze/crazyflie-firmware/blob/master/src/drivers/src/pmw3901.c) và [cấu trúc dữ liệu](https://github.com/bitcraze/crazyflie-firmware/blob/master/src/drivers/interface/pmw3901.h).

Nên đọc vào `uint8_t raw[12]` và giải mã theo offset; không ép buffer thành struct có bitfield/packing phụ thuộc compiler. Helper có thể tự viết như sau:

```cpp
static int16_t decode_i16_le(uint8_t lo, uint8_t hi)
{
    const uint16_t u = static_cast<uint16_t>(lo)
                     | (static_cast<uint16_t>(hi) << 8);
    const int32_t s = (u & 0x8000u)
                    ? static_cast<int32_t>(u) - 65536
                    : static_cast<int32_t>(u);
    return static_cast<int16_t>(s);
}
```

Kiểm tra với `00 00 → 0`, `01 00 → 1`, `FF FF → -1`, `00 80 → -32768`, `FF 7F → 32767`.

API mẫu cho tầng trên có thể dùng:

```cpp
struct flow_sample_t {
    int64_t read_done_us;      // Thời điểm đọc xong; không giả làm thời điểm phơi sáng.
    uint32_t integration_us;  // Khoảng giữa hai lần đọc hợp lệ, nếu đã xác định.
    uint32_t sequence;
    int16_t dx_counts;
    int16_t dy_counts;
    uint16_t shutter;
    uint8_t squal;
    uint8_t motion;
    uint8_t observation;
    bool interval_valid;     // false sau init/reset/lỗi làm mất mốc tích phân.
    bool quality_valid;      // Do chính sách chất lượng đã kiểm chứng quyết định.
};
```

Đây là đề xuất riêng cho dự án. Không coi `dx=dy=0` là lỗi; không coi SPI thành công là chất lượng quang học tốt. Khi đọc lỗi, mẫu kế tiếp có thể không còn khoảng tích phân xác định: đánh dấu lại mốc thay vì tính vận tốc bằng một `dt` giả.

PMW3901 trả motion count; đổi sang chuyển động có đơn vị cần mô hình và khoảng cách tới bề mặt. Xem [ghi chú chuyển đổi của Bitcraze](https://github.com/bitcraze/Bitcraze_PMW3901/blob/master/readme.md). Với dự án drone, nên để tầng sensors/estimator xử lý scale, hướng lắp, bù quay bằng gyro và độ cao; driver giữ dữ liệu thô và quality để chẩn đoán.

### Bước P6 — Điều kiện hoàn tất PMW3901

- ID ổn định qua nhiều lần bật nguồn; init thất bại có lỗi cụ thể và không để CS LOW.
- Di chuyển thuận/ngược từng trục cho delta đổi dấu đúng với hướng lắp.
- Kiểm tra đứng yên, bề mặt ít texture, thay đổi ánh sáng và khoảng cách.
- Logic analyzer xác nhận CS liên tục trong một lệnh và clock/mode đúng.
- `deinit()` rồi `init()` lại hoạt động; không leak device và không free bus đang dùng chung.

## 5. Hoàn thiện BMI270

### Bước B1 — Chọn phần tự viết và phần dùng của Bosch

Hướng đề xuất: **tự viết lớp port ESP-IDF và driver giao diện của dự án, dùng Bosch SensorAPI cho khởi tạo/config image và thao tác đặc thù IC**. Như vậy bạn vẫn viết phần giao tiếp, quản lý lỗi, lịch lấy mẫu và dữ liệu phục vụ drone.

Lấy cùng một commit từ [Bosch BMI270 SensorAPI](https://github.com/boschsensortec/BMI270_SensorAPI): `bmi2.c`, `bmi2.h`, `bmi2_defs.h`, `bmi270.c`, `bmi270.h` và license. Dùng biến thể BMI270 cơ bản trước; không trộn header hoặc config image giữa các biến thể.

Cấu trúc đề xuất khi bạn triển khai:

```text
src/drivers/imu/bmi270/
├── bmi270_driver.h       # API của dự án
├── bmi270_driver.c       # Init/read/health/deinit
├── bmi270_port.h         # Context và callback giao tiếp
├── bmi270_port.c         # ESP-IDF SPI hoặc I2C, delay, mapping lỗi
└── third_party/bosch/
    ├── bmi2.c
    ├── bmi2.h
    ├── bmi2_defs.h
    ├── bmi270.c
    ├── bmi270.h
    └── LICENSE
```

Đổi tên hai file placeholder hiện tại thành `bmi270_driver.*` khi bắt đầu triển khai để tránh tên `bmi270.h` che header của Bosch. Đây mới là cấu trúc hướng dẫn, chưa có file nào được đổi tên hay tải về.

Public header viết bằng C nên có `#ifdef __cplusplus / extern "C" { / #endif` bao các khai báo hàm để gọi được từ `main.cpp`. Dùng `stdint.h`, `stdbool.h` và struct C; không đưa class C++ vào `.c`.

### Bước B2 — Viết port đúng hợp đồng callback

Các callback cần nối vào `struct bmi2_dev`:

```c
BMI2_INTF_RETURN_TYPE bmi_port_read(uint8_t reg, uint8_t *data,
                                   uint32_t len, void *ctx);
BMI2_INTF_RETURN_TYPE bmi_port_write(uint8_t reg, const uint8_t *data,
                                    uint32_t len, void *ctx);
void bmi_port_delay_us(uint32_t period, void *ctx);
```

Context giữ handle bus/device và mã lỗi ESP-IDF gần nhất; phải sống suốt đời driver. Kiểm tra `ctx`, pointer dữ liệu, chiều dài và sức chứa buffer trước khi truyền. Xem [callback mẫu của Bosch](https://github.com/boschsensortec/BMI270_SensorAPI/blob/master/bmi270_examples/common/common.c).

Callback trả `BMI2_INTF_RET_SUCCESS` khi thành công và giá trị lỗi khác 0 khi thất bại. Không cast thẳng `esp_err_t` sang `int8_t`: mã lỗi ESP-IDF có thể bị cắt mất bit. Giữ cả lỗi bus gốc và kết quả SensorAPI để log ở tầng wrapper.

**Nếu dùng SPI:** BMI270 đọc với bit 7 bằng 1, ghi bằng 0, ngược PMW3901. SensorAPI xử lý mask và dummy trong `bmi2_get_regs()`: callback nhận `len` đã gồm byte dummy, trả nguyên phần RX sau địa chỉ; không tự bỏ dummy lần nữa. Xem [implementation của Bosch](https://github.com/boschsensortec/BMI270_SensorAPI/blob/master/bmi2.c).

Một cách thiết kế port dễ kiểm tra là dùng CS tự động, command 8 bit cho địa chỉ và data phase cho `len` byte. Đặt dummy phần cứng bằng 0, vì byte dummy nói trên nằm trong data phase. Khi đọc, phát byte đệm và thu đủ `len`; không tính byte RX của pha địa chỉ vào buffer callback. Giữ CS liên tục trong một transaction. Chọn SPI mode theo datasheet của IC/module; kiểm tra waveform trước khi tăng clock.

**Nếu dùng I2C:** dùng `i2c_master_transmit_receive()` cho địa chỉ thanh ghi rồi đọc; ghi là buffer `[reg, payload...]`. Dùng địa chỉ 7-bit theo mức chân SDO của module, không dịch trái. Nhánh I2C không thêm dummy SPI. Chỉ cần hiện thực một giao tiếp trước, tránh debug hai port cùng lúc.

### Bước B3 — Điền `bmi2_dev` và kiểm tra init

Khởi tạo struct về 0 rồi điền các trường do port chịu trách nhiệm:

| Trường | Cách dùng |
| --- | --- |
| `intf` | `BMI2_SPI_INTF` hoặc `BMI2_I2C_INTF` |
| `intf_ptr` | Context còn sống suốt thời gian sử dụng |
| `read`, `write`, `delay_us` | Ba callback vừa triển khai |
| `read_write_len` | Chọn chiều dài chunk chẵn, ví dụ 32 byte lúc đầu; phù hợp buffer/bus |
| `config_file_ptr` | `NULL` để dùng config đi kèm đúng biến thể |

Tra các kiểu trong [bmi2_defs.h](https://github.com/boschsensortec/BMI270_SensorAPI/blob/master/bmi2_defs.h). Không giả định mọi lời gọi FIFO đều tự chia vừa buffer chỉ nhờ `read_write_len`; kiểm tra chiều dài callback thực tế.

`bmi270_init(&dev)` thực hiện khởi tạo và dùng config image của Bosch; đọc đúng chip ID chưa đủ để kết luận init hoàn tất. Chỉ cho phép cấu hình/lấy mẫu sau khi kết quả là `BMI2_OK`. Xem [bmi270_init](https://github.com/boschsensortec/BMI270_SensorAPI/blob/master/bmi270.c).

Thanh ghi chip ID có địa chỉ `0x00`, giá trị mong đợi là `0x24`. Vì vậy macro `CHIP_ID 0x00` hiện tại cần đặt tên rõ như `BMI270_REG_CHIP_ID`; không so sánh giá trị đọc được với `0x00`. Giá trị nhận diện được định nghĩa trong [bmi270.h của Bosch](https://github.com/boschsensortec/BMI270_SensorAPI/blob/master/bmi270.h).

### Bước B4 — Cấu hình accel/gyro và đọc mẫu mới

Với bản SensorAPI đã đối chiếu, luồng API cơ bản là:

```text
bmi270_init
  → bmi2_get_sensor_config
  → chọn ODR/range/bandwidth/performance mode
  → bmi2_set_sensor_config
  → bmi2_sensor_enable cho BMI2_ACCEL và BMI2_GYRO
  → bmi2_get_sensor_data
  → kiểm tra status và publish mẫu mới
```

Các kiểu liên quan là `struct bmi2_sens_config` và `struct bmi2_sens_data`. Ví dụ chính hãng kiểm tra `BMI2_DRDY_ACC` và `BMI2_DRDY_GYR` trước khi sử dụng dữ liệu. Lấy chữ ký từ đúng commit đã đưa vào dự án, vì các bản API cũ có thể khác. Xem [ví dụ accel/gyro của Bosch](https://github.com/boschsensortec/BMI270_SensorAPI/blob/master/bmi270_examples/accel_gyro/accel_gyro.c).

Gợi ý cho lượt kiểm tra trên bàn: accel/gyro cùng 100 Hz, accel ±4 g, gyro ±2000 °/s. Đây là lựa chọn thử nghiệm, không phải thông số bay đã tối ưu. Sau khi đọc ổn định mới tăng ODR và chọn lọc theo yêu cầu control loop. Đọc lại cấu hình để phát hiện ghi sai; không chỉ dựa vào biến cấu hình bên phần mềm.

### Bước B5 — Định nghĩa dữ liệu và đổi đơn vị

Đề xuất đầu ra:

```c
typedef struct {
    int64_t sample_time_us;   /* DRDY hoặc mốc lấy mẫu đã được định nghĩa. */
    int64_t read_done_us;     /* Dùng đo latency. */
    uint32_t sequence;
    int16_t accel_raw[3];
    int16_t gyro_raw[3];
    float accel_mps2[3];
    float gyro_rad_s[3];
    bool accel_fresh;
    bool gyro_fresh;
    bool time_valid;
} imu_sample_t;
```

Bạn có thể để driver chỉ trả raw rồi chuyển đổi tại `modules/sensors`; chọn một nơi sở hữu việc chuyển đổi để tránh nhân scale hay đổi trục hai lần.

Với dữ liệu signed 16-bit và range đang cấu hình, phép đổi danh định là:

```text
accel_mps2 = raw_accel × range_g × 9.80665 / 32768
gyro_rad_s = raw_gyro × range_dps × (pi / 180) / 32768
```

Ví dụ ±4 g: 8192 count tương ứng khoảng 1 g. Scale phải bám range được đọc lại, không hard-code một hệ số rồi thay range ở nơi khác. Cách đổi theo range được minh họa trong [ví dụ Bosch](https://github.com/boschsensortec/BMI270_SensorAPI/blob/master/bmi270_examples/accel_gyro/accel_gyro.c).

Định nghĩa hệ thân, ví dụ X trước/Y phải/Z xuống, rồi viết phép đổi trục từ hướng lắp thực tế. Kiểm tra dấu gyro bằng quay từng trục. Khi nằm yên, chuẩn vector gia tốc gần 9.81 m/s²; giá trị có dấu từng trục tùy hướng lắp và quy ước specific force, không ép tất cả về 0.

Ước lượng gyro bias khi đã ổn nhiệt và đứng yên, hiệu chuẩn accel ở nhiều tư thế, lưu kèm schema/range/hướng lắp trong NVS. Không lấy trung bình accel ở một tư thế rồi trừ hết trọng lực cho mọi tư thế.

### Bước B6 — DRDY, FIFO và mất mẫu

1. Cấu hình mức điện, latch/pulse và map ngắt BMI270 khớp chân ESP32. Các API liên quan: `bmi2_set_int_pin_config()` và `bmi2_map_data_int()`.
2. Tạo task/handle trước khi cho phép ngắt. ISR chỉ ghi nhận sự kiện/thời gian và dùng `vTaskNotifyGiveFromISR()`; task mới đọc bus.
3. Notification lớn hơn 1 cho biết task xử lý chậm; đọc thanh ghi hiện tại không tái tạo được mọi mẫu đã mất.
4. Nếu cần giữ chuỗi mẫu, dùng FIFO: cấu hình watermark, đọc FIFO rồi giải mã bằng `bmi2_extract_accel()`/`bmi2_extract_gyro()`, xử lý frame và cảnh báo thiếu dữ liệu.
5. Thiết lập timestamp cho từng mẫu theo ODR/sensor time, xử lý wrap/reset; không gắn cùng thời gian đọc xong cho cả lô FIFO.

Tra API và các kiểu `bmi2_int_pin_config`, `bmi2_fifo_frame` trong [bmi2.h](https://github.com/boschsensortec/BMI270_SensorAPI/blob/master/bmi2.h). Thiết kế này dùng DRDY/FIFO làm nhịp nguồn; không kỳ vọng tick RTOS 100 Hz tạo được lịch 1 kHz. `sample_time_us` chỉ được đánh dấu hợp lệ khi nguồn thời gian và liên kết với mẫu đã xác định.

### Bước B7 — Nếu muốn tự viết cả phần thanh ghi BMI270

Có thể đi hướng này sau khi port đọc ID ổn định. Bạn còn phải hiện thực lựa chọn giao tiếp sau reset, quản lý power save, nạp config image theo chunk vào INIT_DATA, cập nhật INIT_ADDR, kết thúc nạp và kiểm tra trạng thái khởi tạo; sau đó mới cấu hình cảm biến. Không thay config image bằng vài lệnh ghi range/ODR.

Dùng [datasheet Bosch](https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bmi270-ds000.pdf) cho trình tự và timing, đối chiếu `bmi2_write_config_file()` trong SensorAPI. Đây là nhánh cần thêm việc; hướng chính ở trên dùng SensorAPI để bạn tập trung viết port và giao diện dự án.

### Bước B8 — Điều kiện hoàn tất BMI270

- Init qua nhiều lần cold boot; phân biệt lỗi bus, sai ID và lỗi config.
- Đọc lại ODR/range; dữ liệu có cả chiều dương/âm và không lệch một byte.
- Đứng yên cho gyro gần bias và độ lớn accel gần 1 g; quay/đổi tư thế đúng trục.
- Mất DRDY hoặc lỗi đọc được phát hiện bằng timeout/health; không publish mẫu cũ như mẫu mới.
- FIFO/queue có số đếm tràn và cách resync; reset driver làm mất hiệu lực dữ liệu/timestamp cũ.

## 6. Tích hợp vào build và ứng dụng

Khi các file đề xuất đã được tạo, cập nhật `src/CMakeLists.txt`. Ví dụ cho nhánh BMI270 dùng SPI:

```cmake
idf_component_register(
    SRCS
        "main.cpp"
        "drivers/optical_flow/pmw3901mb_txqt.cpp"
        "drivers/imu/bmi270/bmi270_driver.c"
        "drivers/imu/bmi270/bmi270_port.c"
        "drivers/imu/bmi270/third_party/bosch/bmi2.c"
        "drivers/imu/bmi270/third_party/bosch/bmi270.c"
    INCLUDE_DIRS "."
    PRIV_INCLUDE_DIRS "drivers/imu/bmi270/third_party/bosch"
    REQUIRES esp_driver_spi esp_driver_gpio
    PRIV_REQUIRES freertos esp_timer esp_rom
)
```

Đây là cấu hình tương lai; chép ngay lúc chưa có file sẽ gây lỗi build. Thêm `esp_driver_i2c` nếu chọn port I2C. Khi dùng NVS từ component này, bổ sung `nvs_flash`. Nếu tách driver thành component riêng, điều chỉnh đường dẫn và public/private dependency tương ứng.

Giữ entry point C linkage trong `main.cpp`: `extern "C" void app_main(void)`. Trình tự thử:

```text
Khởi tạo board/bus
  → init một cảm biến
  → log kết quả và ID
  → tạo task đọc, log thưa
  → xác nhận dữ liệu ổn
  → thêm cảm biến thứ hai
  → thêm DRDY/FIFO và gửi dữ liệu sang sensors
```

Không đặt object/context cục bộ trong `app_main()` rồi đưa con trỏ cho task tiếp tục dùng sau khi hàm return; cấp lifetime toàn ứng dụng hoặc quản lý bộ nhớ rõ ràng.

Lệnh chạy từ thư mục gốc:

```sh
pio run -e nodemcu-32s
pio run -e nodemcu-32s -t upload
pio device monitor -b 115200
```

Nếu terminal chưa có `pio` trong PATH, bản đang cài có executable tại `~/.platformio/penv/bin/pio`. Upload/monitor chỉ thực hiện khi board đã nối đúng. Build thành công với `app_main()` trống chưa kiểm chứng các hàm chỉ mới khai báo hoặc hoạt động của cảm biến.

## 7. Kiểm tra khi chạy chung hai cảm biến

| Kiểm tra | Cần quan sát |
| --- | --- |
| Băng thông bus | Thời gian truyền xấp xỉ `số_byte × 8 / clock_hz`, cộng delay, CS và overhead |
| Thời gian giữ bus của PMW | Có làm BMI bỏ lỡ DRDY hoặc tràn FIFO không? |
| Tải CPU và log | Không log từng mẫu ở ODR cao; đo latency khi bật cả hai driver |
| Timestamp | Đơn điệu, có tuổi dữ liệu, phân biệt thời điểm mẫu và đọc xong |
| Queue | Đếm overflow; thuật toán cần tích phân không được âm thầm ghi đè mẫu |
| Reset/phục hồi | Một cảm biến lỗi không free bus chung hoặc làm mẫu cũ thành hợp lệ |
| Tài nguyên | Stack còn dư, heap không giảm dần qua nhiều lần init/deinit |

Đặc biệt, PMW hiện chạy 100 kHz và có nhiều khoảng busy-wait. Cần đo tổng thời gian chiếm bus trước khi tăng ODR BMI; nếu không đạt ngân sách, tăng clock trong giới hạn IC/board, dùng burst/FIFO hoặc tách host. Không tăng priority để che vấn đề chiếm bus.

Các test phần mềm đáng viết khi triển khai: decode signed/endian; đổi scale ở nhiều range; output không bị cập nhật một phần khi truyền lỗi; init thất bại giữa chừng vẫn cleanup; parser FIFO xử lý frame thiếu. Test phần cứng bổ sung cold boot, đọc liên tục, chất lượng flow và hai device cùng hoạt động. Việc viết tài liệu này chưa chạy các test đó.

## 8. Mốc công việc để tự theo dõi

- [ ] Chốt pinout, module, nguồn và hướng lắp.
- [ ] Đưa ownership SPI bus lên tầng board/platform.
- [ ] PMW: thống nhất CS thủ công, sửa API trả lỗi, viết register read.
- [ ] PMW: reset, ID, bảng init, delta và burst quality.
- [ ] BMI: chọn một commit SensorAPI, thêm port và wrapper không trùng tên.
- [ ] BMI: init/config, đọc raw, đổi đơn vị và kiểm tra trục.
- [ ] BMI: DRDY/FIFO, timestamp và xử lý mất mẫu.
- [ ] CMake và `app_main()` gọi được cả hai driver.
- [ ] Đo timing/stack/queue và thử lỗi khi chạy chung.
- [ ] Tầng sensors nhận mẫu có đơn vị, hệ trục và độ hợp lệ rõ ràng.

Hãy bắt đầu ở P1: sửa quyền điều khiển CS, rồi P2/P3: viết hàm đọc có trả lỗi. Đây là nền để kiểm tra được ID trước khi viết phần còn lại của PMW3901.
