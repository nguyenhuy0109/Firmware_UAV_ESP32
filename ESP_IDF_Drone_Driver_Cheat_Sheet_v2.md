# ESP-IDF: Hướng dẫn kiểu dữ liệu, cấu hình và sử dụng API

Bản mở rộng theo tham số · Đối chiếu ESP-IDF v6.1, ESP32; có ghi chú ESP32-S3.

Tài liệu dùng để tra cách viết code: mỗi kiểu ghi rõ nó là struct, enum, handle hay callback; mỗi struct có bảng trường và cú pháp tạo biến; mỗi hàm có chữ ký, từng tham số, hướng dữ liệu, đơn vị, giá trị trả về và cách gọi. Các ví dụ là ví dụ chung, không gắn với một IC cảm biến cụ thể.

Đây là hướng dẫn các nhóm API dưới đây, không phải danh mục toàn bộ ESP-IDF. Những trường nâng cao phụ thuộc target/Kconfig được ghi rõ hoặc dẫn tới header chính thức. SDK thực tế do PlatformIO cài có thể khác v6.1: khi một trường không tồn tại, kiểm tra version và target trước khi sửa tên theo phỏng đoán.

## Mục lục

1. [Cách đọc kiểu, struct, con trỏ và chữ ký hàm](#01-cach-doc)
2. [Header và component](#02-header)
3. [GPIO và ngắt](#03-gpio)
4. [SPI master](#04-spi)
5. [I2C master](#05-i2c)
6. [UART](#06-uart)
7. [Timestamp, delay và ESP Timer](#07-time)
8. [GPTimer](#08-gptimer)
9. [FreeRTOS: task, notification, queue, mutex](#09-rtos)
10. [LEDC](#10-ledc)
11. [MCPWM](#11-mcpwm)
12. [RMT](#12-rmt)
13. [ADC và calibration](#13-adc)
14. [Log và xử lý lỗi](#14-error)
15. [NVS](#15-nvs)
16. [Watchdog và reset](#16-wdt)
17. [Heap và buffer](#17-heap)
18. [Tích hợp, lỗi thường gặp và chỉ mục](#18-index)

<a id="01-cach-doc"></a>
## 01. Cách đọc kiểu, struct, con trỏ và chữ ký hàm

### 01.1 Kiểu dữ liệu khác với biến và giá trị

```cpp
spi_bus_config_t bus_cfg = {};
// ^ kiểu có sẵn     ^ tên biến do mình đặt
bus_cfg.mosi_io_num = 23;
//      ^ trường        ^ giá trị cấu hình
```

`spi_bus_config_t` là một kiểu struct, không phải một hàm. Nó không nhận đối số bằng dấu ngoặc tròn. Mình tạo biến từ kiểu đó, gán từng trường qua dấu `.`, rồi đưa địa chỉ biến vào API cấu hình.

| Khái niệm | Ý nghĩa | Ví dụ |
| --- | --- | --- |
| Kiểu số | Chứa một giá trị số | `uint8_t`, `int16_t`, `size_t`, `int64_t` |
| Enum | Chọn một giá trị định danh trong tập có sẵn | `spi_host_device_t host = SPI2_HOST;` |
| Struct | Gom nhiều trường cấu hình hoặc dữ liệu | `spi_bus_config_t cfg = {};` |
| Handle | Đại diện tài nguyên SDK đã tạo; giữ lại để dùng tiếp | `spi_device_handle_t dev = nullptr;` |
| Callback | Con trỏ tới hàm được driver gọi khi có sự kiện | `void callback(void *arg)` |
| Bitmask/flags | Ghép các cờ bằng toán tử OR bit `\|` | `SPI_TRANS_USE_TXDATA \| SPI_TRANS_USE_RXDATA` |
| Con trỏ | Địa chỉ dữ liệu; có thể cho hàm đọc hoặc ghi | `const void *tx`, `uint8_t *rx` |

Hậu tố `_t` là quy ước tên kiểu; chỉ nhìn `_t` không thể biết đó là struct, enum hay con trỏ. Phải đọc mô tả hoặc khai báo kiểu. Trong các bảng tiếp theo, cột **Loại** giải thích việc này.

### 01.2 Đọc cú pháp của một API

```c
esp_err_t spi_bus_initialize(
    spi_host_device_t host_id,
    const spi_bus_config_t *bus_config,
    spi_dma_chan_t dma_chan
);
```

| Thành phần | Cách hiểu |
| --- | --- |
| `esp_err_t` trước tên hàm | Kiểu giá trị trả về, ở đây là mã lỗi |
| `host_id` | Một giá trị kiểu enum `spi_host_device_t` |
| `const spi_bus_config_t *bus_config` | Con trỏ chỉ đọc tới struct cấu hình |
| `dma_chan` | Một giá trị enum chọn cơ chế DMA |
| `const` | Hàm không sửa dữ liệu qua con trỏ đó |
| `*` trong khai báo | Tham số là con trỏ |
| `&bus_cfg` khi gọi | Lấy địa chỉ của biến struct để truyền vào con trỏ |

```cpp
spi_bus_config_t bus_cfg = {};
// Điền đầy đủ chân và các trường cần thiết trước lời gọi.
esp_err_t err = spi_bus_initialize(
    SPI2_HOST, &bus_cfg, SPI_DMA_DISABLED);
```

`host_id`, `bus_config`, `dma_chan` là tên tham số trong khai báo API; khi gọi không cần tạo biến có đúng những tên đó. `SPI2_HOST` và `SPI_DMA_DISABLED` là giá trị hằng enum của SDK.

### 01.3 Hướng dữ liệu và handle đầu ra

Các bảng dùng **in**: hàm đọc giá trị mình cung cấp; **out**: hàm ghi kết quả vào địa chỉ mình cung cấp; **in/out**: cả hai. Ký hiệu này là tài liệu, không phải từ khóa C++.

```cpp
spi_device_handle_t dev = nullptr;
// &dev là địa chỉ biến handle, để SDK ghi handle mới.
esp_err_t err = spi_bus_add_device(SPI2_HOST, &dev_cfg, &dev);
if (err != ESP_OK) {
    // Không sử dụng dev như một thiết bị đã tạo thành công.
}
```

`&dev` khác `dev`: API tạo cần địa chỉ biến để ghi handle; API truyền dữ liệu về sau nhận chính handle `dev`. Tương tự, `&cfg` trỏ tới struct có sẵn, không tạo tài nguyên mới bằng bản thân phép `&`.

### 01.4 C/C++, khởi tạo mặc định và đơn vị

- Các ví dụ triển khai dùng C++: `T cfg = {};` rồi gán trường. Với C trước C23, struct thông thường dùng `T cfg = {0};`. Một vài struct rỗng của SDK dùng mở rộng GNU C; không tự chép typedef SDK để sửa vấn đề này.
- `{}` khởi tạo giá trị về 0/null hoặc giá trị enum zero theo kiểu. Điều đó **không đảm bảo cấu hình hợp lệ**: chân không dùng có thể cần `-1`, tốc độ cần lớn hơn 0, queue cần lớn hơn 0.
- `.cpp` có thể gọi API C của ESP-IDF. `.h` dùng cho C hoặc C++; `.hpp` thể hiện header C++. `private`, `public`, constructor chỉ có trong C++.
- Chữ ký trong tài liệu giữ kiểu tham số để hiểu cách gọi. Với macro FreeRTOS/log, phần cú pháp ghi rõ là macro, không phải nguyên mẫu hàm để mình định nghĩa lại.

| Đại lượng | Đơn vị |
| --- | --- |
| SPI `length`, `rxlength` | Bit; `n` byte thường là `n * 8` |
| Kích thước buffer, SPI `max_transfer_sz` | Byte |
| UART read, FreeRTOS queue/mutex, SPI queue wait | RTOS tick |
| I2C `xfer_timeout_ms`, RMT wait timeout | Mili giây |
| ESP Timer period/timestamp | Micro giây |
| RMT receive `signal_range_*_ns` | Nano giây |
| GPTimer/MCPWM/RMT duration | Count ngoại vi theo `resolution_hz` |
| LEDC duty | Số đếm theo độ phân giải duty |

`ESP_OK` là thành công đối với `esp_err_t`. Với FreeRTOS, kiểm tra `pdPASS`/`pdTRUE`; với UART read/write, kiểm tra số byte. Không dùng một quy tắc kiểm tra kết quả cho mọi API.

<a id="02-header"></a>
## 02. Header và component

| Nhóm | Header | Component |
| --- | --- | --- |
| GPIO | driver/gpio.h | esp_driver_gpio |
| SPI master | driver/spi_master.h | esp_driver_spi |
| I2C master mới | driver/i2c_master.h | esp_driver_i2c |
| UART | driver/uart.h | esp_driver_uart |
| GPTimer | driver/gptimer.h | esp_driver_gptimer |
| LEDC | driver/ledc.h | esp_driver_ledc |
| MCPWM | driver/mcpwm_prelude.h | esp_driver_mcpwm |
| RMT TX / RX / encoder | driver/rmt_tx.h; driver/rmt_rx.h; driver/rmt_encoder.h | esp_driver_rmt |
| ADC oneshot / calibration | esp_adc/adc_oneshot.h; esp_adc/adc_cali.h; esp_adc/adc_cali_scheme.h | esp_adc |
| Timestamp / timer mềm | esp_timer.h | esp_timer |
| Task / queue / mutex | freertos/FreeRTOS.h + task.h / queue.h / semphr.h trong freertos/ | freertos |
| Log / lỗi | esp_log.h; esp_err.h; esp_check.h | log; esp_common |
| NVS | nvs_flash.h; nvs.h | nvs_flash |
| Watchdog / reset | esp_task_wdt.h; esp_system.h | esp_system |
| Heap | esp_heap_caps.h | heap |
| Delay ROM ngắn | esp_rom_sys.h | esp_rom |

Include đưa khai báo kiểu/hàm vào file; dependency component phục vụ build/link. Với IDF 6.x khai báo component driver cụ thể; SDK cũ có thể tổ chức khác.

```cmake
idf_component_register(
    SRCS "sensor_driver.cpp"
    INCLUDE_DIRS "include"
    REQUIRES esp_driver_spi esp_driver_gpio
    PRIV_REQUIRES freertos esp_timer
)
```

`SRCS` là danh sách file nguồn; `INCLUDE_DIRS` là thư mục public header; `REQUIRES` dùng khi public header cần dependency; `PRIV_REQUIRES` khi chỉ implementation cần. Đây là cú pháp component ESP-IDF; cấu trúc dự án PlatformIO phải theo CMake thực tế của dự án.

**Nguồn:** [Build system và component](https://docs.espressif.com/projects/esp-idf/en/v6.1/esp32/api-guides/build-system.html), [Migration 6.0: peripheral drivers](https://docs.espressif.com/projects/esp-idf/en/v6.1/esp32/migration-guides/release-6.x/6.0/peripherals.html).

<a id="03-gpio"></a>

## 03. GPIO và ngắt

**Header:** `driver/gpio.h`. Cấu hình GPIO cho input/output/reset/enable. Peripheral như SPI/UART tự cấu hình chân tín hiệu khi driver tương ứng tạo thành công; không cấu hình lại GPIO đó tùy tiện sau khi đã nối peripheral.

#### `gpio_num_t`

**Loại:** enum. **Dùng để:** chỉ định một chân GPIO

| Trường / giá trị | Kiểu | Ý nghĩa, giá trị và cách chọn |
| --- | --- | --- |
| GPIO_NUM_0, GPIO_NUM_1, ... | giá trị enum | Chọn chân tồn tại và phù hợp chức năng trên target/module. |

**Cú pháp sử dụng:**

```cpp
gpio_num_t pin = GPIO_NUM_4;
```

**Lưu ý:** Số GPIO khác số thứ tự chân trên connector. ESP32 thường có GPIO34-39 chỉ input, không có pull-up/down nội.

#### `gpio_config_t`

**Loại:** struct. **Dùng để:** cấu hình một hoặc nhiều chân cùng thuộc tính

| Trường / giá trị | Kiểu | Ý nghĩa, giá trị và cách chọn |
| --- | --- | --- |
| pin_bit_mask | uint64_t | Mask các chân: `1ULL << pin`; ghép nhiều chân bằng OR bit. |
| mode | gpio_mode_t | `GPIO_MODE_INPUT`, `GPIO_MODE_OUTPUT`, `GPIO_MODE_INPUT_OUTPUT`, hoặc chế độ open-drain phù hợp. |
| pull_up_en | gpio_pullup_t | `GPIO_PULLUP_ENABLE` hoặc `GPIO_PULLUP_DISABLE`. |
| pull_down_en | gpio_pulldown_t | `GPIO_PULLDOWN_ENABLE` hoặc `GPIO_PULLDOWN_DISABLE`. |
| intr_type | gpio_int_type_t | `GPIO_INTR_DISABLE`, `POSEDGE`, `NEGEDGE`, `ANYEDGE`, `LOW_LEVEL`, `HIGH_LEVEL` với tiền tố `GPIO_INTR_`. |

**Cú pháp sử dụng:**

```cpp
gpio_config_t cfg = {};
cfg.pin_bit_mask = 1ULL << GPIO_NUM_4;
cfg.mode = GPIO_MODE_OUTPUT;
cfg.pull_up_en = GPIO_PULLUP_DISABLE;
cfg.pull_down_en = GPIO_PULLDOWN_DISABLE;
cfg.intr_type = GPIO_INTR_DISABLE;
```

**Lưu ý:** `gpio_config` áp dụng các thuộc tính trong struct lên mọi chân trong mask; đừng gộp input và output khác mục đích vào một cấu hình.

#### `gpio_isr_t`

**Loại:** typedef con trỏ hàm. **Dùng để:** handler khi chân có sự kiện ngắt

**Cú pháp sử dụng:**

```cpp
void on_gpio(void *arg) {
    // Xử lý rất ngắn; arg là context truyền khi gắn handler.
}
gpio_isr_t handler = on_gpio;
```

**Lưu ý:** Callback chạy trong ISR; không block, không dùng mutex. IRAM cần toàn bộ đường gọi/dữ liệu đáp ứng yêu cầu khi cài ESP_INTR_FLAG_IRAM.

### 03.1 Hàm cấu hình và điều khiển

#### `gpio_config`

Áp dụng cấu hình chân.

```c
esp_err_t gpio_config(const gpio_config_t *pGPIOConfig);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| pGPIOConfig | const gpio_config_t * / in | Địa chỉ struct đã điền; truyền `&cfg`. |

**Trả về:** `ESP_OK` nếu thành công; mã `esp_err_t` khác nếu thất bại. Chỉ dùng kết quả đầu ra sau khi kiểm tra thành công.

**Cách gọi:**

```cpp
esp_err_t err = gpio_config(&cfg);
```

#### `gpio_set_direction`

Đặt hướng một chân.

```c
esp_err_t gpio_set_direction(gpio_num_t gpio_num, gpio_mode_t mode);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| gpio_num | gpio_num_t / in | GPIO cần đổi hướng. |
| mode | gpio_mode_t / in | Ví dụ GPIO_MODE_OUTPUT. |

**Trả về:** `ESP_OK` nếu thành công; mã `esp_err_t` khác nếu thất bại. Chỉ dùng kết quả đầu ra sau khi kiểm tra thành công.

**Cách gọi:**

```cpp
gpio_set_direction(GPIO_NUM_4, GPIO_MODE_OUTPUT);
```

#### `gpio_set_level`

Xuất mức logic trên chân đã là output.

```c
esp_err_t gpio_set_level(gpio_num_t gpio_num, uint32_t level);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| gpio_num | gpio_num_t / in | Chân output. |
| level | uint32_t / in | 0 là thấp; 1 là cao. |

**Trả về:** `ESP_OK` nếu thành công; mã `esp_err_t` khác nếu thất bại. Chỉ dùng kết quả đầu ra sau khi kiểm tra thành công.

**Cách gọi:**

```cpp
gpio_set_level(GPIO_NUM_4, 1);
```

#### `gpio_get_level`

Đọc mức logic của chân đã bật input.

```c
int gpio_get_level(gpio_num_t gpio_num);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| gpio_num | gpio_num_t / in | Chân cần đọc. |

**Trả về:** `int` 0 hoặc 1; không phải esp_err_t.

**Cách gọi:**

```cpp
int level = gpio_get_level(GPIO_NUM_4);
```

**Điều kiện / chú ý:** Muốn đọc lại mức trên chân output, cấu hình phù hợp để input cũng được bật.

#### `gpio_reset_pin`

Đưa cấu hình chân về trạng thái mặc định của driver.

```c
esp_err_t gpio_reset_pin(gpio_num_t gpio_num);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| gpio_num | gpio_num_t / in | Chân không còn được ngoại vi khác sử dụng. |

**Trả về:** `ESP_OK` nếu thành công; mã `esp_err_t` khác nếu thất bại. Chỉ dùng kết quả đầu ra sau khi kiểm tra thành công.

**Cách gọi:**

```cpp
gpio_reset_pin(GPIO_NUM_4);
```

### 03.2 Hàm ngắt

#### `gpio_set_intr_type`

Chọn cạnh hoặc mức gây ngắt.

```c
esp_err_t gpio_set_intr_type(gpio_num_t gpio_num, gpio_int_type_t intr_type);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| gpio_num | gpio_num_t / in | Chân input. |
| intr_type | gpio_int_type_t / in | Ví dụ GPIO_INTR_POSEDGE; phải đúng cực tính nguồn tín hiệu. |

**Trả về:** `ESP_OK` nếu thành công; mã `esp_err_t` khác nếu thất bại. Chỉ dùng kết quả đầu ra sau khi kiểm tra thành công.

**Cách gọi:**

```cpp
gpio_set_intr_type(GPIO_NUM_4, GPIO_INTR_POSEDGE);
```

#### `gpio_install_isr_service`

Cài dịch vụ cho các handler theo từng chân.

```c
esp_err_t gpio_install_isr_service(int intr_alloc_flags);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| intr_alloc_flags | int / in | 0 cho cài đặt thông thường; bitmask ESP_INTR_FLAG_* khi có nhu cầu và hiểu ràng buộc. |

**Trả về:** `ESP_OK` nếu thành công; mã `esp_err_t` khác nếu thất bại. Chỉ dùng kết quả đầu ra sau khi kiểm tra thành công.

**Cách gọi:**

```cpp
esp_err_t err = gpio_install_isr_service(0);
```

**Điều kiện / chú ý:** Gọi một lần ở tầng sở hữu dịch vụ; ESP_ERR_INVALID_STATE có thể do dịch vụ đã được cài.

#### `gpio_isr_handler_add`

Gắn handler vào một GPIO.

```c
esp_err_t gpio_isr_handler_add(gpio_num_t gpio_num, gpio_isr_t isr_handler, void *args);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| gpio_num | gpio_num_t / in | Chân đã được cấu hình input/ngắt. |
| isr_handler | gpio_isr_t / in | Tên callback, không thêm `()`. |
| args | void * / in | Context sống đến lúc tháo handler; NULL nếu không cần. |

**Trả về:** `ESP_OK` nếu thành công; mã `esp_err_t` khác nếu thất bại. Chỉ dùng kết quả đầu ra sau khi kiểm tra thành công.

**Cách gọi:**

```cpp
gpio_isr_handler_add(GPIO_NUM_4, on_gpio, nullptr);
```

**Điều kiện / chú ý:** Cần cài ISR service trước. Callback/args không được hết lifetime khi ngắt còn bật.

#### `gpio_intr_enable`

Bật ngắt cho chân đã cấu hình.

```c
esp_err_t gpio_intr_enable(gpio_num_t gpio_num);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| gpio_num | gpio_num_t / in | Chân cần thao tác. |

**Trả về:** `ESP_OK` nếu thành công; mã `esp_err_t` khác nếu thất bại. Chỉ dùng kết quả đầu ra sau khi kiểm tra thành công.

**Cách gọi:**

```cpp
gpio_intr_enable(GPIO_NUM_4);
```

#### `gpio_intr_disable`

Tắt ngắt của chân.

```c
esp_err_t gpio_intr_disable(gpio_num_t gpio_num);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| gpio_num | gpio_num_t / in | Chân cần thao tác. |

**Trả về:** `ESP_OK` nếu thành công; mã `esp_err_t` khác nếu thất bại. Chỉ dùng kết quả đầu ra sau khi kiểm tra thành công.

**Cách gọi:**

```cpp
gpio_intr_disable(GPIO_NUM_4);
```

#### `gpio_isr_handler_remove`

Tháo handler trước khi hủy context.

```c
esp_err_t gpio_isr_handler_remove(gpio_num_t gpio_num);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| gpio_num | gpio_num_t / in | Chân cần thao tác. |

**Trả về:** `ESP_OK` nếu thành công; mã `esp_err_t` khác nếu thất bại. Chỉ dùng kết quả đầu ra sau khi kiểm tra thành công.

**Cách gọi:**

```cpp
gpio_isr_handler_remove(GPIO_NUM_4);
```

### 03.3 Ví dụ cấu hình output

```cpp
#include "driver/gpio.h"
#include "esp_err.h"

esp_err_t cs_gpio_init(gpio_num_t cs)
{
    gpio_config_t cfg = {};
    cfg.pin_bit_mask = 1ULL << cs;
    cfg.mode = GPIO_MODE_OUTPUT;
    cfg.pull_up_en = GPIO_PULLUP_DISABLE;
    cfg.pull_down_en = GPIO_PULLDOWN_DISABLE;
    cfg.intr_type = GPIO_INTR_DISABLE;
    esp_err_t err = gpio_config(&cfg);
    if (err != ESP_OK) return err;
    return gpio_set_level(cs, 1);
}
```

### 03.4 Ví dụ ISR đánh thức task

```cpp
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"

static void drdy_isr(void *arg)
{
    TaskHandle_t task = (TaskHandle_t)arg;
    BaseType_t woken = pdFALSE;
    vTaskNotifyGiveFromISR(task, &woken);
    if (woken == pdTRUE) portYIELD_FROM_ISR();
}

esp_err_t drdy_attach(gpio_num_t pin, TaskHandle_t task)
{
    if (task == NULL) return ESP_ERR_INVALID_ARG;
    gpio_config_t cfg = {};
    cfg.pin_bit_mask = 1ULL << pin;
    cfg.mode = GPIO_MODE_INPUT;
    cfg.intr_type = GPIO_INTR_POSEDGE;
    esp_err_t err = gpio_config(&cfg);
    if (err != ESP_OK) return err;
    return gpio_isr_handler_add(pin, drdy_isr, task);
}

// App init, once before drdy_attach():
// ESP_ERROR_CHECK(gpio_install_isr_service(0));
```

**Nguồn:** [GPIO & RTC GPIO](https://docs.espressif.com/projects/esp-idf/en/v6.1/esp32/api-reference/peripherals/gpio.html), [FreeRTOS trong ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/v6.1/esp32/api-reference/system/freertos_idf.html).

<a id="04-spi"></a>

## 04. SPI master

**Header:** `driver/spi_master.h`. Một host điều khiển một bus; bus có nhiều thiết bị dùng chung MOSI/MISO/SCLK và CS riêng. Trình tự: tạo bus → thêm device → tạo transaction → truyền → tháo device → free bus.

#### `spi_host_device_t`

**Loại:** enum. **Dùng để:** chọn bộ điều khiển SPI

| Trường / giá trị | Kiểu | Ý nghĩa, giá trị và cách chọn |
| --- | --- | --- |
| SPI2_HOST; SPI3_HOST | giá trị enum | Chọn host có trên target và chưa bị thành phần khác sở hữu. Bus SPI0/1 thường dành flash/PSRAM. |

**Cú pháp sử dụng:**

```cpp
spi_host_device_t host = SPI2_HOST;
```

#### `spi_dma_chan_t`

**Loại:** typedef enum DMA. **Dùng để:** chọn có dùng DMA và cách cấp kênh

| Trường / giá trị | Kiểu | Ý nghĩa, giá trị và cách chọn |
| --- | --- | --- |
| SPI_DMA_DISABLED | giá trị enum | Không dùng DMA; giao dịch bị giới hạn theo FIFO. |
| SPI_DMA_CH_AUTO | giá trị enum | Driver chọn kênh DMA phù hợp nếu target hỗ trợ. |

**Cú pháp sử dụng:**

```cpp
spi_dma_chan_t dma = SPI_DMA_CH_AUTO;
```

**Lưu ý:** Không truyền số GPIO vào tham số DMA. Các kênh DMA cố định cũ phụ thuộc target/SDK; ưu tiên AUTO hoặc DISABLED.

#### `spi_bus_config_t`

**Loại:** struct. **Dùng để:** cấu hình phần dây và tài nguyên dùng chung của bus

| Trường / giá trị | Kiểu | Ý nghĩa, giá trị và cách chọn |
| --- | --- | --- |
| mosi_io_num / data0_io_num | int, union alias | GPIO MOSI; -1 nếu không dùng. Hai tên là cùng một vùng nhớ. |
| miso_io_num / data1_io_num | int, union alias | GPIO MISO; -1 nếu không dùng. |
| sclk_io_num | int | GPIO clock SPI. |
| quadwp_io_num / data2_io_num | int, union alias | Chân WP/data2 cho quad; SPI 4 dây thông thường đặt -1. |
| quadhd_io_num / data3_io_num | int, union alias | Chân HD/data3 cho quad; SPI 4 dây thông thường đặt -1. |
| data4_io_num ... data7_io_num | int mỗi trường | Các chân bổ sung octal; không dùng đặt -1. |
| iocfg[9] | int[9], union alias | Cách truy cập nhóm chân bằng mảng; cùng vùng nhớ các trường chân, không phải bộ cấu hình thứ hai. |
| data_io_default_level | bool | Mức mặc định đường data khi không có transaction; chỉ đổi nếu mạch/giao thức cần. |
| max_transfer_sz | int | Kích thước dữ liệu tối đa theo byte. 0 chọn mặc định driver (phụ thuộc DMA/FIFO); đặt rõ khi dùng buffer lớn. |
| dma_burst_size | uint32_t | 0 dùng mặc định; giá trị khác phụ thuộc DMA/target. Bỏ qua khi không DMA hoặc target không hỗ trợ. |
| flags | uint32_t bitmask | OR các SPICOMMON_BUSFLAG_* để yêu cầu/kiểm tra khả năng bus. 0 cho cấu hình cơ bản. |
| isr_cpu_id | esp_intr_cpu_affinity_t | Core đặt ISR; ESP_INTR_CPU_AFFINITY_AUTO để tự chọn theo driver. |
| intr_flags | int bitmask | Cờ cấp phát ngắt ESP_INTR_FLAG_*. 0 cho mặc định; IRAM đặt ràng buộc callback/callee. |

**Cú pháp sử dụng:**

```cpp
spi_bus_config_t bus_cfg = {};
bus_cfg.mosi_io_num = mosi_pin;
bus_cfg.miso_io_num = miso_pin;
bus_cfg.sclk_io_num = sck_pin;
bus_cfg.quadwp_io_num = -1;
bus_cfg.quadhd_io_num = -1;
bus_cfg.data4_io_num = -1;
bus_cfg.data5_io_num = -1;
bus_cfg.data6_io_num = -1;
bus_cfg.data7_io_num = -1;
bus_cfg.max_transfer_sz = 256;
// flags/intr_flags/dma_burst_size giữ 0 cho cấu hình cơ bản.
```

**Lưu ý:** mosi_pin/miso_pin/sck_pin là GPIO board đã chọn. Tên alias trong union không cộng thêm chân. Bảng bám v6.1; SDK cũ có thể chưa có các trường mở rộng.

#### `spi_device_interface_config_t`

**Loại:** struct. **Dùng để:** cấu hình riêng cho một thiết bị được thêm vào bus

| Trường / giá trị | Kiểu | Ý nghĩa, giá trị và cách chọn |
| --- | --- | --- |
| command_bits | uint8_t | Số bit pha command: 0-16. 0 nếu tự đưa lệnh vào tx_buffer. |
| address_bits | uint8_t | Số bit pha address: 0-64. 0 nếu tự đưa địa chỉ vào tx_buffer. |
| dummy_bits | uint8_t | Số xung dummy giữa address và data; theo giao thức, không mặc định là số byte. |
| mode | uint8_t | 0=(CPOL0,CPHA0), 1=(0,1), 2=(1,0), 3=(1,1). Chọn theo IC. |
| clock_source | spi_clock_source_t | Nguồn clock; SPI_CLK_SRC_DEFAULT cho mặc định. |
| duty_cycle_pos | uint16_t | Tỉ lệ SCLK theo 1/256; 128 là 50%; 0 cũng dùng mặc định 50%. |
| cs_ena_pretrans | uint16_t | CS active trước giao dịch, 0-16 chu kỳ SPI; có giới hạn theo half/full-duplex. |
| cs_ena_posttrans | uint8_t | Giữ CS sau giao dịch, 0-16 chu kỳ SPI. |
| clock_speed_hz | int | Tần số yêu cầu theo Hz, ví dụ 1000000. Không vượt khả năng IC và đường dây. |
| input_delay_ns | int | Độ trễ MISO cần tính bù theo ns; 0 nếu chưa có yêu cầu timing đặc biệt. |
| sample_point | spi_sampling_point_t | Điểm lấy mẫu theo target; giữ khởi tạo mặc định nếu không cần tuning. |
| spics_io_num | int | GPIO CS phần cứng; -1 để không dùng CS phần cứng và tự điều khiển ở ngoài. |
| flags | uint32_t bitmask | SPI_DEVICE_HALFDUPLEX, SPI_DEVICE_TXBIT_LSBFIRST, SPI_DEVICE_RXBIT_LSBFIRST... 0 mặc định full-duplex, MSB trước. |
| queue_size | int | Số descriptor đã queue nhưng chưa thu hồi tối đa; phải >0, ví dụ 1. |
| pre_cb / post_cb | transaction_cb_t | Callback trước/sau giao dịch; NULL nếu không cần. Không block; xem ngữ cảnh driver/IRAM. |

**Cú pháp sử dụng:**

```cpp
spi_device_interface_config_t dev_cfg = {};
dev_cfg.clock_speed_hz = 1000000;
dev_cfg.mode = 0;
dev_cfg.spics_io_num = cs_pin;
dev_cfg.queue_size = 1;
// command_bits/address_bits/dummy_bits = 0:
// tự đóng gói toàn bộ frame trong buffer.
```

**Lưu ý:** CS trước/sau giao dịch không tạo delay tùy ý giữa các byte. Pha command/address có độ dài riêng, không tính thêm vào t.length của pha data.

#### `spi_device_handle_t`

**Loại:** opaque handle, kiểu con trỏ do SDK định nghĩa. **Dùng để:** tham chiếu device đã đăng ký

**Cú pháp sử dụng:**

```cpp
spi_device_handle_t dev = nullptr;
esp_err_t err = spi_bus_add_device(SPI2_HOST, &dev_cfg, &dev);
// Sau ESP_OK, truyền dev vào API giao dịch.
```

**Lưu ý:** Không tự new/malloc struct bên trong handle; không free(dev). Dùng spi_bus_remove_device để hủy.

#### `spi_transaction_t`

**Loại:** struct descriptor. **Dùng để:** mô tả một lần truyền/nhận

| Trường / giá trị | Kiểu | Ý nghĩa, giá trị và cách chọn |
| --- | --- | --- |
| flags | uint32_t bitmask | Cờ SPI_TRANS_*, ví dụ USE_TXDATA/USE_RXDATA cho mảng inline. |
| cmd | uint16_t | Giá trị command khi command_bits > 0. |
| addr | uint64_t | Giá trị address khi address_bits > 0. |
| length | size_t | Số bit dữ liệu. 2 byte đặt 16; không bao gồm command/address. |
| rxlength | size_t | Số bit nhận. Full-duplex không lớn hơn length; 0 mặc định bằng length. |
| override_freq_hz | uint32_t | 0 giữ clock hiện có; giá trị Hz khác yêu cầu đổi clock của device, có overhead và ảnh hưởng các lần sau. |
| user | void * | Context cho callback, không được gửi lên dây. |
| tx_buffer | const void * | Địa chỉ bộ đệm phát ngoài; NULL nếu không phát qua buffer này. |
| tx_data[4] | uint8_t[4], union với tx_buffer | Tối đa 4 byte inline, chỉ dùng khi bật SPI_TRANS_USE_TXDATA. |
| rx_buffer | void * | Bộ đệm nhận có dung lượng/căn chỉnh phù hợp, đặc biệt khi DMA. |
| rx_data[4] | uint8_t[4], union với rx_buffer | Tối đa 4 byte inline, chỉ dùng khi bật SPI_TRANS_USE_RXDATA. |

**Cú pháp sử dụng:**

```cpp
spi_transaction_t t = {};
t.flags = SPI_TRANS_USE_TXDATA | SPI_TRANS_USE_RXDATA;
t.length = 16;
t.tx_data[0] = 0x12;
t.tx_data[1] = 0;
esp_err_t err = spi_device_transmit(dev, &t);
if (err == ESP_OK) {
    uint8_t second_received_byte = t.rx_data[1];
    (void)second_received_byte;
}
```

**Lưu ý:** Ví dụ chỉ minh họa frame 2 byte, không cam kết giao thức thanh ghi của IC nào. Không dùng tx_buffer và tx_data cùng lúc; tương tự RX. Descriptor/buffer queue phải sống đến khi nhận kết quả.

#### `spi_transaction_ext_t`

**Loại:** struct mở rộng. **Dùng để:** đổi độ dài command/address/dummy ở từng transaction

| Trường / giá trị | Kiểu | Ý nghĩa, giá trị và cách chọn |
| --- | --- | --- |
| base | spi_transaction_t | Descriptor cơ sở, truyền &ext.base vào API. |
| command_bits / address_bits / dummy_bits | uint8_t từng trường | Độ dài pha riêng của giao dịch; bật cờ VARIABLE tương ứng trong base.flags. |

**Cú pháp sử dụng:**

```cpp
spi_transaction_ext_t ext = {};
ext.base.flags = SPI_TRANS_VARIABLE_CMD | SPI_TRANS_VARIABLE_ADDR;
ext.command_bits = 8;
ext.address_bits = 16;
ext.base.cmd = 0x01;
ext.base.addr = 0x1234;
// spi_device_transmit(dev, &ext.base);
```

**Lưu ý:** Chỉ dùng nếu giao thức cần thay độ dài theo từng lệnh. Dummy tùy biến cần cờ SPI_TRANS_VARIABLE_DUMMY theo SDK.

#### `transaction_cb_t`

**Loại:** typedef con trỏ hàm. **Dùng để:** callback trước/sau transaction

**Cú pháp sử dụng:**

```cpp
void spi_done(spi_transaction_t *t) {
    // Truy cập t->user nếu cần; không block.
}
// dev_cfg.post_cb = spi_done;
```

**Lưu ý:** Đọc quy tắc ISR/cache-safe của driver; không gọi log dài hoặc chờ khóa trong callback.

### 04.1 Hàm tạo bus và device

#### `spi_bus_initialize`

Khởi tạo host SPI thành bus master.

```c
esp_err_t spi_bus_initialize(spi_host_device_t host_id,
                             const spi_bus_config_t *bus_config,
                             spi_dma_chan_t dma_chan);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| host_id | spi_host_device_t / in | Host cần khởi tạo, ví dụ SPI2_HOST; không phải GPIO CS. |
| bus_config | const spi_bus_config_t * / in | Địa chỉ struct chân/bus đã điền, truyền &bus_cfg. |
| dma_chan | spi_dma_chan_t / in | SPI_DMA_CH_AUTO hoặc SPI_DMA_DISABLED; không phải số byte hay tốc độ clock. |

**Trả về:** `ESP_OK` thành công; có thể gặp `ESP_ERR_INVALID_ARG` (cấu hình sai), `ESP_ERR_INVALID_STATE` (host đã dùng), `ESP_ERR_NOT_FOUND` (không có DMA phù hợp), `ESP_ERR_NO_MEM`.

**Cách gọi:**

```cpp
esp_err_t err = spi_bus_initialize(
    SPI2_HOST, &bus_cfg, SPI_DMA_CH_AUTO);
```

**Điều kiện / chú ý:** Tạo bus một lần ở tầng sở hữu bus. Nếu nhiều IC chung bus, thêm nhiều device; không initialize lại cùng host cho từng IC.

#### `spi_bus_add_device`

Đăng ký thiết bị trên bus đã tạo.

```c
esp_err_t spi_bus_add_device(spi_host_device_t host_id,
                            const spi_device_interface_config_t *dev_config,
                            spi_device_handle_t *handle);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| host_id | spi_host_device_t / in | Cùng host đã initialize. |
| dev_config | const spi_device_interface_config_t * / in | Địa chỉ cấu hình tốc độ/mode/CS/queue. |
| handle | spi_device_handle_t * / out | Địa chỉ biến handle, ví dụ &dev. SDK ghi handle mới vào đó. |

**Trả về:** `ESP_OK` nếu thành công; mã `esp_err_t` khác nếu thất bại. Chỉ dùng kết quả đầu ra sau khi kiểm tra thành công.

**Cách gọi:**

```cpp
spi_device_handle_t dev = nullptr;
esp_err_t err = spi_bus_add_device(SPI2_HOST, &dev_cfg, &dev);
```

### 04.2 Hàm truyền và giữ bus

#### `spi_device_transmit`

Truyền đồng bộ bằng đường giao dịch interrupt/queue rồi chờ kết quả.

```c
esp_err_t spi_device_transmit(spi_device_handle_t handle,
                                      spi_transaction_t *trans_desc);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| handle | spi_device_handle_t / in | Device đã add thành công. |
| trans_desc | spi_transaction_t * / in/out | Địa chỉ descriptor; SDK đọc TX và ghi RX theo cấu hình. |

**Trả về:** `ESP_OK` nếu thành công; mã `esp_err_t` khác nếu thất bại. Chỉ dùng kết quả đầu ra sau khi kiểm tra thành công.

**Cách gọi:**

```cpp
esp_err_t err = spi_device_transmit(dev, &t);
```

**Điều kiện / chú ý:** Không gọi khi cùng device còn giao dịch queue/polling chưa hoàn tất.

#### `spi_device_polling_transmit`

Truyền đồng bộ bằng polling; CPU chủ động chờ hoàn tất.

```c
esp_err_t spi_device_polling_transmit(spi_device_handle_t handle,
                                      spi_transaction_t *trans_desc);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| handle | spi_device_handle_t / in | Device đã add thành công. |
| trans_desc | spi_transaction_t * / in/out | Địa chỉ descriptor; SDK đọc TX và ghi RX theo cấu hình. |

**Trả về:** `ESP_OK` nếu thành công; mã `esp_err_t` khác nếu thất bại. Chỉ dùng kết quả đầu ra sau khi kiểm tra thành công.

**Cách gọi:**

```cpp
esp_err_t err = spi_device_polling_transmit(dev, &t);
```

**Điều kiện / chú ý:** Phù hợp giao dịch nhỏ khi hiểu chi phí CPU; không trộn với queue đang chạy.

#### `spi_device_queue_trans`

Xếp transaction để driver thực hiện nền.

```c
esp_err_t spi_device_queue_trans(spi_device_handle_t handle,
                                spi_transaction_t *trans_desc,
                                uint32_t ticks_to_wait);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| handle | spi_device_handle_t / in | Device đích. |
| trans_desc | spi_transaction_t * / in/out | Descriptor và buffer phải sống đến khi lấy kết quả. |
| ticks_to_wait | TickType_t / in | Thời gian chờ chỗ trong queue, theo tick; 0 không chờ, portMAX_DELAY chờ vô hạn. |

**Trả về:** `ESP_OK` nếu thành công; mã `esp_err_t` khác nếu thất bại. Chỉ dùng kết quả đầu ra sau khi kiểm tra thành công.

**Cách gọi:**

```cpp
esp_err_t err = spi_device_queue_trans(
    dev, &t, pdMS_TO_TICKS(20));
```

**Điều kiện / chú ý:** ESP_OK chỉ xác nhận đã queue, chưa cho phép đọc RX hay tái dùng descriptor.

#### `spi_device_get_trans_result`

Lấy descriptor của giao dịch đã hoàn tất.

```c
esp_err_t spi_device_get_trans_result(spi_device_handle_t handle,
                                     spi_transaction_t **trans_desc,
                                     uint32_t ticks_to_wait);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| handle | spi_device_handle_t / in | Device đã queue. |
| trans_desc | spi_transaction_t ** / out | Địa chỉ biến con trỏ nhận descriptor, ví dụ &done. Không phải địa chỉ bản sao descriptor. |
| ticks_to_wait | TickType_t / in | Chờ kết quả theo tick. |

**Trả về:** `ESP_OK` nếu thành công; mã `esp_err_t` khác nếu thất bại. Chỉ dùng kết quả đầu ra sau khi kiểm tra thành công.

**Cách gọi:**

```cpp
spi_transaction_t *done = nullptr;
esp_err_t err = spi_device_get_trans_result(
    dev, &done, pdMS_TO_TICKS(20));
```

**Điều kiện / chú ý:** Nếu timeout, transaction có thể vẫn đang chạy; chưa được hủy/reuse buffer vì chưa thu hồi descriptor.

#### `spi_device_acquire_bus`

Giữ bus để nhiều transaction của device không bị device khác xen vào.

```c
esp_err_t spi_device_acquire_bus(spi_device_handle_t device, uint32_t wait);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| device | spi_device_handle_t / in | Device muốn giữ bus. |
| wait | uint32_t / in | v6.1 yêu cầu portMAX_DELAY; không tự thay bằng timeout tùy ý. |

**Trả về:** `ESP_OK` nếu thành công; mã `esp_err_t` khác nếu thất bại. Chỉ dùng kết quả đầu ra sau khi kiểm tra thành công.

**Cách gọi:**

```cpp
esp_err_t err = spi_device_acquire_bus(dev, portMAX_DELAY);
```

**Điều kiện / chú ý:** Không tự giữ CS thấp. Không thay mutex khi nhiều task truy cập cùng device. Phải release ở mọi đường thoát sau khi acquire thành công.

#### `spi_device_release_bus`

Trả bus để các device khác giao dịch.

```c
void spi_device_release_bus(spi_device_handle_t dev);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| dev | spi_device_handle_t / in | Device đang giữ bus. |

**Trả về:** `void`, không có mã trả về.

**Cách gọi:**

```cpp
spi_device_release_bus(dev);
```

#### `spi_bus_remove_device`

Tháo device và giải phóng tài nguyên device.

```c
esp_err_t spi_bus_remove_device(spi_device_handle_t handle);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| handle | spi_device_handle_t / in | Device không còn giao dịch/task sử dụng. |

**Trả về:** `ESP_OK` nếu thành công; mã `esp_err_t` khác nếu thất bại. Chỉ dùng kết quả đầu ra sau khi kiểm tra thành công.

**Cách gọi:**

```cpp
esp_err_t err = spi_bus_remove_device(dev);
```

**Điều kiện / chú ý:** Sau thành công không sử dụng lại handle cũ.

#### `spi_bus_free`

Giải phóng bus sau khi đã tháo hết device.

```c
esp_err_t spi_bus_free(spi_host_device_t host_id);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| host_id | spi_host_device_t / in | Host do thành phần gọi sở hữu. |

**Trả về:** `ESP_OK` nếu thành công; mã `esp_err_t` khác nếu thất bại. Chỉ dùng kết quả đầu ra sau khi kiểm tra thành công.

**Cách gọi:**

```cpp
esp_err_t err = spi_bus_free(SPI2_HOST);
```

**Điều kiện / chú ý:** Không free bus dùng chung chỉ vì một driver cảm biến bị hủy.

#### `spi_bus_dma_memory_alloc`

Cấp buffer đáp ứng căn chỉnh/capability của bus SPI.

```c
void *spi_bus_dma_memory_alloc(spi_host_device_t host_id,
                               size_t size, uint32_t extra_heap_caps);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| host_id | spi_host_device_t / in | Bus đã initialize. |
| size | size_t / in | Số byte cần cấp. |
| extra_heap_caps | uint32_t bitmask / in | Capability bổ sung; 0 nếu không có yêu cầu thêm. |

**Trả về:** Con trỏ vùng nhớ hoặc NULL; giải phóng bằng free().

**Cách gọi:**

```cpp
void *buffer = spi_bus_dma_memory_alloc(SPI2_HOST, 256, 0);
```

**Điều kiện / chú ý:** Không free buffer trước khi mọi giao dịch dùng nó hoàn tất.

### 04.3 Ví dụ tổng hợp bus → device → transaction

```cpp
#include "driver/spi_master.h"
#include "esp_err.h"

void spi_example(int mosi_pin, int miso_pin, int sck_pin, int cs_pin)
{
    spi_bus_config_t bus_cfg = {};
    bus_cfg.mosi_io_num = mosi_pin;
    bus_cfg.miso_io_num = miso_pin;
    bus_cfg.sclk_io_num = sck_pin;
    bus_cfg.quadwp_io_num = -1;
    bus_cfg.quadhd_io_num = -1;
    bus_cfg.data4_io_num = -1;
    bus_cfg.data5_io_num = -1;
    bus_cfg.data6_io_num = -1;
    bus_cfg.data7_io_num = -1;
    bus_cfg.max_transfer_sz = 256;
    ESP_ERROR_CHECK(spi_bus_initialize(
        SPI2_HOST, &bus_cfg, SPI_DMA_DISABLED));

    spi_device_interface_config_t dev_cfg = {};
    dev_cfg.clock_speed_hz = 1000000;
    dev_cfg.mode = 0;
    dev_cfg.spics_io_num = cs_pin;
    dev_cfg.queue_size = 1;
    spi_device_handle_t dev = nullptr;
    ESP_ERROR_CHECK(spi_bus_add_device(SPI2_HOST, &dev_cfg, &dev));

    spi_transaction_t t = {};
    t.flags = SPI_TRANS_USE_TXDATA | SPI_TRANS_USE_RXDATA;
    t.length = 16;
    t.tx_data[0] = 0x12;
    t.tx_data[1] = 0;
    ESP_ERROR_CHECK(spi_device_transmit(dev, &t));
    // RX của hai byte clock vừa phát nằm trong t.rx_data[0..1].
    // Ý nghĩa byte phụ thuộc giao thức thiết bị thực tế.

    ESP_ERROR_CHECK(spi_bus_remove_device(dev));
    ESP_ERROR_CHECK(spi_bus_free(SPI2_HOST));
}
```

Ví dụ tự sở hữu bus và giải phóng ngay để minh họa vòng đời. Ứng dụng đọc liên tục giữ bus/device trong context thay vì tạo/hủy mỗi lần đọc. `ESP_ERROR_CHECK` abort khi lỗi; driver thư viện nên trả lỗi và cleanup các tài nguyên đã tạo.

**Nguồn:** [SPI master](https://docs.espressif.com/projects/esp-idf/en/v6.1/esp32/api-reference/peripherals/spi_master.html), [Header SPI device và transaction](https://github.com/espressif/esp-idf/blob/v6.1/components/esp_driver_spi/include/driver/spi_master.h), [Header SPI bus](https://github.com/espressif/esp-idf/blob/v6.1/components/esp_driver_spi/include/driver/spi_common.h).

<a id="05-i2c"></a>

## 05. I2C master

**Header:** `driver/i2c_master.h`. Dùng bus handle cho dây SDA/SCL và device handle cho từng địa chỉ IC. Địa chỉ truyền vào API là địa chỉ 7/10-bit chưa ghép bit R/W; không dịch trái địa chỉ 7-bit. Ví dụ dùng API đồng bộ, chưa đăng ký callback bất đồng bộ.

#### `i2c_master_bus_config_t`

**Loại:** struct. **Dùng để:** cấu hình controller và đường dây chung

| Trường / giá trị | Kiểu | Ý nghĩa, giá trị và cách chọn |
| --- | --- | --- |
| i2c_port | i2c_port_num_t | Chọn I2C_NUM_0/I2C_NUM_1 nếu target có; -1 cho tự cấp port. |
| sda_io_num / scl_io_num | gpio_num_t | GPIO SDA/SCL phù hợp board. |
| clk_source | i2c_clock_source_t | I2C_CLK_SRC_DEFAULT cho nguồn mặc định. |
| glitch_ignore_cnt | uint8_t | Bộ lọc glitch theo chu kỳ clock module; ví dụ 7, không phải micro giây. |
| intr_priority | int | 0 để driver chọn priority ngắt mặc định. |
| trans_queue_depth | size_t | Độ sâu queue khi dùng bất đồng bộ; không phải kích thước buffer byte. |
| flags.enable_internal_pullup | uint32_t bit-field | true bật pull-up nội; mạch thường cần pull-up ngoài đủ mạnh. |
| flags.allow_pd | uint32_t bit-field | Cho phép backup/restore để tắt miền nguồn khi sleep nếu target hỗ trợ; false khi chưa có yêu cầu. |

**Cú pháp sử dụng:**

```cpp
i2c_master_bus_config_t bus_cfg = {};
bus_cfg.i2c_port = I2C_NUM_0;
bus_cfg.sda_io_num = sda_pin;
bus_cfg.scl_io_num = scl_pin;
bus_cfg.clk_source = I2C_CLK_SRC_DEFAULT;
bus_cfg.glitch_ignore_cnt = 7;
bus_cfg.flags.enable_internal_pullup = false; // đã có pull-up ngoài
```

#### `i2c_device_config_t`

**Loại:** struct. **Dùng để:** cấu hình một IC trên bus

| Trường / giá trị | Kiểu | Ý nghĩa, giá trị và cách chọn |
| --- | --- | --- |
| dev_addr_length | i2c_addr_bit_len_t | I2C_ADDR_BIT_LEN_7 hoặc I2C_ADDR_BIT_LEN_10. |
| device_address | uint16_t | Địa chỉ thô chưa có R/W, ví dụ 0x68 chỉ là minh họa; chọn theo IC/chân địa chỉ. |
| scl_speed_hz | uint32_t | Tần số SCL theo Hz; bắt đầu 100000, tăng theo target/IC/mạch. ESP32 tham chiếu tối đa 400000. |
| scl_wait_us | uint32_t | Khoảng chờ SCL theo us; 0 dùng giá trị mặc định phần cứng/driver. |
| flags.disable_ack_check | uint32_t bit-field | false để kiểm tra ACK bình thường; không bật chỉ để che lỗi NACK. |

**Cú pháp sử dụng:**

```cpp
i2c_device_config_t dev_cfg = {};
dev_cfg.dev_addr_length = I2C_ADDR_BIT_LEN_7;
dev_cfg.device_address = address7;
dev_cfg.scl_speed_hz = 100000;
```

#### `i2c_master_bus_handle_t`

**Loại:** opaque handle con trỏ. **Dùng để:** giữ tài nguyên bus I2C

**Cú pháp sử dụng:**

```cpp
i2c_master_bus_handle_t bus = nullptr;
```

**Lưu ý:** Handle chỉ dùng sau khi API tạo trả ESP_OK. Không tự free handle; dùng API remove/delete.

#### `i2c_master_dev_handle_t`

**Loại:** opaque handle con trỏ. **Dùng để:** tham chiếu IC đã thêm vào bus

**Cú pháp sử dụng:**

```cpp
i2c_master_dev_handle_t dev = nullptr;
```

**Lưu ý:** Handle chỉ dùng sau khi API tạo trả ESP_OK. Không tự free handle; dùng API remove/delete.

### 05.1 API và từng tham số

#### `i2c_new_master_bus`

Tạo bus master.

```c
esp_err_t i2c_new_master_bus(const i2c_master_bus_config_t *bus_config,
                             i2c_master_bus_handle_t *ret_bus_handle);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| bus_config | const i2c_master_bus_config_t * / in | Địa chỉ cấu hình bus. |
| ret_bus_handle | i2c_master_bus_handle_t * / out | Địa chỉ biến nhận handle bus, &bus. |

**Trả về:** `ESP_OK` nếu thành công; mã `esp_err_t` khác nếu thất bại. Chỉ dùng kết quả đầu ra sau khi kiểm tra thành công.

**Cách gọi:**

```cpp
esp_err_t err = i2c_new_master_bus(&bus_cfg, &bus);
```

#### `i2c_master_bus_add_device`

Thêm IC vào bus.

```c
esp_err_t i2c_master_bus_add_device(i2c_master_bus_handle_t bus_handle,
                                  const i2c_device_config_t *dev_config,
                                  i2c_master_dev_handle_t *ret_handle);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| bus_handle | i2c_master_bus_handle_t / in | Bus đã tạo. |
| dev_config | const i2c_device_config_t * / in | Địa chỉ cấu hình IC. |
| ret_handle | i2c_master_dev_handle_t * / out | Địa chỉ biến nhận handle IC, &dev. |

**Trả về:** `ESP_OK` nếu thành công; mã `esp_err_t` khác nếu thất bại. Chỉ dùng kết quả đầu ra sau khi kiểm tra thành công.

**Cách gọi:**

```cpp
esp_err_t err = i2c_master_bus_add_device(bus, &dev_cfg, &dev);
```

#### `i2c_master_probe`

Gửi địa chỉ và kiểm tra có ACK.

```c
esp_err_t i2c_master_probe(i2c_master_bus_handle_t bus_handle,
                           uint16_t address, int xfer_timeout_ms);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| bus_handle | i2c_master_bus_handle_t / in | Bus cần thăm dò. |
| address | uint16_t / in | Địa chỉ IC, không có bit R/W. |
| xfer_timeout_ms | int / in | Timeout theo ms; -1 chờ vô hạn. |

**Trả về:** `ESP_OK` nếu thành công; mã `esp_err_t` khác nếu thất bại. Chỉ dùng kết quả đầu ra sau khi kiểm tra thành công.

**Cách gọi:**

```cpp
esp_err_t err = i2c_master_probe(bus, address7, 50);
```

**Điều kiện / chú ý:** ACK chưa xác nhận đúng loại IC hoặc đã init cảm biến.

#### `i2c_master_transmit`

Gửi một chuỗi byte tới IC.

```c
esp_err_t i2c_master_transmit(i2c_master_dev_handle_t i2c_dev,
                              const uint8_t *write_buffer, size_t write_size,
                              int xfer_timeout_ms);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| i2c_dev | i2c_master_dev_handle_t / in | Handle IC. |
| write_buffer | const uint8_t * / in | Con trỏ byte phát. |
| write_size | size_t / in | Số byte trong buffer. |
| xfer_timeout_ms | int / in | Timeout ms; -1 chờ vô hạn. |

**Trả về:** `ESP_OK` nếu thành công; mã `esp_err_t` khác nếu thất bại. Chỉ dùng kết quả đầu ra sau khi kiểm tra thành công.

**Cách gọi:**

```cpp
uint8_t tx[2] = {reg, value};
esp_err_t err = i2c_master_transmit(dev, tx, sizeof(tx), 50);
```

#### `i2c_master_receive`

Nhận byte từ IC không có pha ghi thanh ghi phía trước.

```c
esp_err_t i2c_master_receive(i2c_master_dev_handle_t i2c_dev,
                             uint8_t *read_buffer, size_t read_size,
                             int xfer_timeout_ms);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| i2c_dev | i2c_master_dev_handle_t / in | Handle IC. |
| read_buffer | uint8_t * / out | Buffer nhận có ít nhất read_size byte. |
| read_size | size_t / in | Số byte muốn nhận. |
| xfer_timeout_ms | int / in | Timeout ms. |

**Trả về:** `ESP_OK` nếu thành công; mã `esp_err_t` khác nếu thất bại. Chỉ dùng kết quả đầu ra sau khi kiểm tra thành công.

**Cách gọi:**

```cpp
uint8_t rx[8];
esp_err_t err = i2c_master_receive(dev, rx, sizeof(rx), 50);
```

**Điều kiện / chú ý:** Chỉ dùng khi giao thức IC cho phép đọc trực tiếp như vậy.

#### `i2c_master_transmit_receive`

Ghi rồi đọc trong một thao tác với repeated START.

```c
esp_err_t i2c_master_transmit_receive(
    i2c_master_dev_handle_t i2c_dev,
    const uint8_t *write_buffer, size_t write_size,
    uint8_t *read_buffer, size_t read_size, int xfer_timeout_ms);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| i2c_dev | i2c_master_dev_handle_t / in | IC đích. |
| write_buffer | const uint8_t * / in | Ví dụ địa chỉ thanh ghi. |
| write_size | size_t / in | Số byte pha ghi. |
| read_buffer | uint8_t * / out | Buffer kết quả. |
| read_size | size_t / in | Số byte pha đọc. |
| xfer_timeout_ms | int / in | Timeout ms, không truyền pdMS_TO_TICKS vào đây. |

**Trả về:** `ESP_OK` nếu thành công; mã `esp_err_t` khác nếu thất bại. Chỉ dùng kết quả đầu ra sau khi kiểm tra thành công.

**Cách gọi:**

```cpp
uint8_t reg = 0x00, value = 0;
esp_err_t err = i2c_master_transmit_receive(
    dev, &reg, 1, &value, 1, 50);
```

**Điều kiện / chú ý:** Ví dụ giao thức địa chỉ 1 byte; địa chỉ 16-bit hoặc yêu cầu STOP giữa pha cần cách khác.

#### `i2c_master_bus_reset`

Reset bus do driver quản lý, không reset cấu hình IC.

```c
esp_err_t i2c_master_bus_reset(i2c_master_bus_handle_t bus_handle);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| bus_handle | i2c_master_bus_handle_t / in | Bus muốn reset. |

**Trả về:** `ESP_OK` nếu thành công; mã `esp_err_t` khác nếu thất bại. Chỉ dùng kết quả đầu ra sau khi kiểm tra thành công.

**Cách gọi:**

```cpp
i2c_master_bus_reset(bus);
```

#### `i2c_master_bus_rm_device`

Tháo device.

```c
esp_err_t i2c_master_bus_rm_device(i2c_master_dev_handle_t handle);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| handle | i2c_master_dev_handle_t / in | Device không còn giao dịch. |

**Trả về:** `ESP_OK` nếu thành công; mã `esp_err_t` khác nếu thất bại. Chỉ dùng kết quả đầu ra sau khi kiểm tra thành công.

**Cách gọi:**

```cpp
i2c_master_bus_rm_device(dev);
```

#### `i2c_del_master_bus`

Giải phóng bus do caller sở hữu.

```c
esp_err_t i2c_del_master_bus(i2c_master_bus_handle_t bus_handle);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| bus_handle | i2c_master_bus_handle_t / in | Bus đã tháo hết device. |

**Trả về:** `ESP_OK` nếu thành công; mã `esp_err_t` khác nếu thất bại. Chỉ dùng kết quả đầu ra sau khi kiểm tra thành công.

**Cách gọi:**

```cpp
i2c_del_master_bus(bus);
```

Các giao dịch master đồng bộ được driver v6.1 khóa bus. Chuỗi nhiều lời gọi có trạng thái IC dùng chung vẫn có thể cần mutex ứng dụng. Nếu bật bất đồng bộ, lifetime buffer và nghĩa thời điểm hoàn tất phải theo tài liệu callback.

### 05.2 Ví dụ tạo bus và helper thanh ghi 8-bit

```cpp
#include <stdint.h>
#include <stddef.h>
#include "driver/i2c_master.h"
#include "esp_err.h"

void i2c_example_init(gpio_num_t sda, gpio_num_t scl,
                     uint16_t addr7,
                     i2c_master_bus_handle_t *bus,
                     i2c_master_dev_handle_t *dev)
{
    i2c_master_bus_config_t b = {};
    b.i2c_port = I2C_NUM_0;
    b.sda_io_num = sda;
    b.scl_io_num = scl;
    b.clk_source = I2C_CLK_SRC_DEFAULT;
    b.glitch_ignore_cnt = 7;
    b.flags.enable_internal_pullup = false;
    ESP_ERROR_CHECK(i2c_new_master_bus(&b, bus));

    i2c_device_config_t d = {};
    d.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    d.device_address = addr7;
    d.scl_speed_hz = 100000;
    ESP_ERROR_CHECK(i2c_master_bus_add_device(*bus, &d, dev));
}

esp_err_t i2c_read_reg8(i2c_master_dev_handle_t dev,
                      uint8_t reg, uint8_t *out)
{
    if (out == NULL) return ESP_ERR_INVALID_ARG;
    return i2c_master_transmit_receive(
        dev, &reg, 1, out, 1, 50); // 50 ms
}

esp_err_t i2c_write_reg8(i2c_master_dev_handle_t dev,
                       uint8_t reg, uint8_t value)
{
    uint8_t tx[2] = {reg, value};
    return i2c_master_transmit(dev, tx, sizeof(tx), 50);
}
```

**Nguồn:** [Migration 6.0: peripheral drivers](https://docs.espressif.com/projects/esp-idf/en/v6.1/esp32/migration-guides/release-6.x/6.0/peripherals.html), [I2C master](https://docs.espressif.com/projects/esp-idf/en/v6.1/esp32/api-reference/peripherals/i2c.html), [Header I2C master](https://github.com/espressif/esp-idf/blob/v6.1/components/esp_driver_i2c/include/driver/i2c_master.h).

<a id="06-uart"></a>

## 06. UART

**Header:** `driver/uart.h`; thêm `freertos/FreeRTOS.h` khi dùng tick. UART cung cấp luồng byte. Khung GPS/receiver/telemetry cần parser riêng.

#### `uart_port_t`

**Loại:** enum. **Dùng để:** chọn controller UART

| Trường / giá trị | Kiểu | Ý nghĩa, giá trị và cách chọn |
| --- | --- | --- |
| UART_NUM_0 / UART_NUM_1 / UART_NUM_2 | giá trị enum | Số port tùy target. Tránh chiếm port đang dùng console nếu chưa cấu hình lại. |

**Cú pháp sử dụng:**

```cpp
uart_port_t port = UART_NUM_1;
```

#### `uart_config_t`

**Loại:** struct. **Dùng để:** đặt thông số giao tiếp UART

| Trường / giá trị | Kiểu | Ý nghĩa, giá trị và cách chọn |
| --- | --- | --- |
| baud_rate | int | Baud, ví dụ 115200; phải khớp thiết bị. |
| data_bits | uart_word_length_t | UART_DATA_5_BITS ... UART_DATA_8_BITS. |
| parity | uart_parity_t | UART_PARITY_DISABLE/EVEN/ODD. |
| stop_bits | uart_stop_bits_t | UART_STOP_BITS_1, UART_STOP_BITS_1_5, UART_STOP_BITS_2 theo hỗ trợ. |
| flow_ctrl | uart_hw_flowcontrol_t | UART_HW_FLOWCTRL_DISABLE hoặc RTS/CTS theo mạch. |
| rx_flow_ctrl_thresh | uint8_t | Ngưỡng RTS trong FIFO khi dùng flow control; không phải timeout. |
| rx_glitch_filt_thresh | uint32_t | Ngưỡng lọc glitch RX theo ns; 0 tắt; trường có trong v6.1. |
| source_clk | uart_sclk_t | UART_SCLK_DEFAULT hoặc nguồn được target hỗ trợ. |
| flags.allow_pd | uint32_t bit-field | Cho phép driver lưu/khôi phục trạng thái khi sleep nếu hỗ trợ. |
| flags.backup_before_sleep | alias legacy/deprecated | Không dùng cho code mới; cùng ý nghĩa allow_pd trong SDK tham chiếu. |

**Cú pháp sử dụng:**

```cpp
uart_config_t cfg = {};
cfg.baud_rate = 115200;
cfg.data_bits = UART_DATA_8_BITS;
cfg.parity = UART_PARITY_DISABLE;
cfg.stop_bits = UART_STOP_BITS_1;
cfg.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
cfg.source_clk = UART_SCLK_DEFAULT;
```

#### `uart_event_t`

**Loại:** struct dữ liệu sự kiện. **Dùng để:** nhận thông báo từ queue UART

| Trường / giá trị | Kiểu | Ý nghĩa, giá trị và cách chọn |
| --- | --- | --- |
| type | uart_event_type_t | UART_DATA, UART_FIFO_OVF, UART_BUFFER_FULL, UART_PARITY_ERR, UART_FRAME_ERR... |
| size | size_t | Số byte liên quan UART_DATA; không khẳng định đủ một frame ứng dụng. |
| timeout_flag | bool | UART_DATA do RX timeout phần cứng; khác timeout chờ queue của task. |

**Cú pháp sử dụng:**

```cpp
uart_event_t event = {};
// xQueueReceive(uart_queue, &event, timeout_ticks);
// Kiểm tra event.type trước khi xử lý event.size.
```

#### `uart_param_config`

Áp dụng baud/format cho port.

```c
esp_err_t uart_param_config(uart_port_t uart_num, const uart_config_t *uart_config);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| uart_num | uart_port_t / in | Port cần cấu hình. |
| uart_config | const uart_config_t * / in | Địa chỉ struct cấu hình. |

**Trả về:** `ESP_OK` nếu thành công; mã `esp_err_t` khác nếu thất bại. Chỉ dùng kết quả đầu ra sau khi kiểm tra thành công.

**Cách gọi:**

```cpp
uart_param_config(port, &cfg);
```

#### `uart_set_pin`

Định tuyến chân UART.

```c
esp_err_t uart_set_pin(uart_port_t uart_num, int tx_io_num,
                       int rx_io_num, int rts_io_num, int cts_io_num);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| uart_num | uart_port_t / in | Port. |
| tx_io_num | int / in | GPIO TX hoặc UART_PIN_NO_CHANGE. |
| rx_io_num | int / in | GPIO RX hoặc UART_PIN_NO_CHANGE. |
| rts_io_num | int / in | GPIO RTS; giữ nguyên nếu không dùng flow control. |
| cts_io_num | int / in | GPIO CTS; giữ nguyên nếu không dùng flow control. |

**Trả về:** `ESP_OK` nếu thành công; mã `esp_err_t` khác nếu thất bại. Chỉ dùng kết quả đầu ra sau khi kiểm tra thành công.

**Cách gọi:**

```cpp
uart_set_pin(port, tx_pin, rx_pin,
             UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
```

#### `uart_driver_install`

Tạo buffer và dịch vụ driver.

```c
esp_err_t uart_driver_install(uart_port_t uart_num, int rx_buffer_size,
                              int tx_buffer_size, int queue_size,
                              QueueHandle_t *uart_queue, int intr_alloc_flags);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| uart_num | uart_port_t / in | Port cần cài. |
| rx_buffer_size | int / in | Dung lượng ring buffer RX theo byte; phải lớn hơn FIFO phần cứng. |
| tx_buffer_size | int / in | 0 không dùng TX ring buffer, hoặc dung lượng hợp lệ lớn hơn FIFO. |
| queue_size | int / in | Số event queue, 0 nếu không cần queue. |
| uart_queue | QueueHandle_t * / out | &uart_queue nếu muốn nhận handle; nullptr khi không dùng queue. |
| intr_alloc_flags | int bitmask / in | Cờ ngắt; 0 cho ví dụ thường. Không tùy tiện bật IRAM khi driver không đáp ứng. |

**Trả về:** `ESP_OK` nếu thành công; mã `esp_err_t` khác nếu thất bại. Chỉ dùng kết quả đầu ra sau khi kiểm tra thành công.

**Cách gọi:**

```cpp
QueueHandle_t uart_queue = nullptr;
uart_driver_install(port, 1024, 0, 10, &uart_queue, 0);
```

#### `uart_write_bytes`

Gửi một buffer byte.

```c
int uart_write_bytes(uart_port_t uart_num, const void *src, size_t size);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| uart_num | uart_port_t / in | Port đã cài driver. |
| src | const void * / in | Địa chỉ dữ liệu phát. |
| size | size_t / in | Số byte phát. |

**Trả về:** Số byte đã được ghi vào cơ chế TX; âm nếu lỗi. Chưa bảo đảm đã ra hết dây nếu dùng TX ring buffer.

**Cách gọi:**

```cpp
uint8_t tx[2] = {0x12, 0x34};
int n = uart_write_bytes(port, tx, sizeof(tx));
```

#### `uart_read_bytes`

Nhận tối đa length byte.

```c
int uart_read_bytes(uart_port_t uart_num, void *buf, uint32_t length,
                    TickType_t ticks_to_wait);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| uart_num | uart_port_t / in | Port RX. |
| buf | void * / out | Buffer có ít nhất length byte. |
| length | uint32_t / in | Số byte tối đa muốn đọc. |
| ticks_to_wait | TickType_t / in | Thời gian chờ theo RTOS tick. |

**Trả về:** int số byte thực đọc; 0 không có byte, âm là lỗi. Có thể ít hơn length.

**Cách gọi:**

```cpp
uint8_t rx[64];
int n = uart_read_bytes(port, rx, sizeof(rx), pdMS_TO_TICKS(20));
if (n > 0) { /* chỉ dùng rx[0..n-1] */ }
```

#### `uart_get_buffered_data_len`

Đọc số byte trong buffer RX.

```c
esp_err_t uart_get_buffered_data_len(uart_port_t uart_num, size_t *size);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| uart_num | uart_port_t / in | Port. |
| size | size_t * / out | Địa chỉ biến số byte, &available. |

**Trả về:** `ESP_OK` nếu thành công; mã `esp_err_t` khác nếu thất bại. Chỉ dùng kết quả đầu ra sau khi kiểm tra thành công.

**Cách gọi:**

```cpp
size_t available = 0;
uart_get_buffered_data_len(port, &available);
```

#### `uart_wait_tx_done`

Chờ TX hoàn tất.

```c
esp_err_t uart_wait_tx_done(uart_port_t uart_num, TickType_t ticks_to_wait);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| uart_num | uart_port_t / in | Port TX. |
| ticks_to_wait | TickType_t / in | Giới hạn chờ theo tick. |

**Trả về:** `ESP_OK` nếu thành công; mã `esp_err_t` khác nếu thất bại. Chỉ dùng kết quả đầu ra sau khi kiểm tra thành công.

**Cách gọi:**

```cpp
uart_wait_tx_done(port, pdMS_TO_TICKS(100));
```

#### `uart_flush_input`

Bỏ byte RX đang chờ; không phải chờ TX.

```c
esp_err_t uart_flush_input(uart_port_t uart_num);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| uart_num | uart_port_t / in | Port cần thao tác. |

**Trả về:** `ESP_OK` nếu thành công; mã `esp_err_t` khác nếu thất bại. Chỉ dùng kết quả đầu ra sau khi kiểm tra thành công.

**Cách gọi:**

```cpp
uart_flush_input(port);
```

#### `uart_driver_delete`

Hủy driver sau khi mọi task ngừng sử dụng port.

```c
esp_err_t uart_driver_delete(uart_port_t uart_num);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| uart_num | uart_port_t / in | Port cần thao tác. |

**Trả về:** `ESP_OK` nếu thành công; mã `esp_err_t` khác nếu thất bại. Chỉ dùng kết quả đầu ra sau khi kiểm tra thành công.

**Cách gọi:**

```cpp
uart_driver_delete(port);
```

#### `uart_set_line_inverse`

Đảo mức logic tín hiệu UART.

```c
esp_err_t uart_set_line_inverse(uart_port_t uart_num, uint32_t inverse_mask);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| uart_num | uart_port_t / in | Port. |
| inverse_mask | uint32_t bitmask / in | Ví dụ UART_SIGNAL_RXD_INV; 0 bỏ đảo. Ghép cờ theo tín hiệu cần đảo. |

**Trả về:** `ESP_OK` nếu thành công; mã `esp_err_t` khác nếu thất bại. Chỉ dùng kết quả đầu ra sau khi kiểm tra thành công.

**Cách gọi:**

```cpp
uart_set_line_inverse(port, UART_SIGNAL_RXD_INV);
```

**Điều kiện / chú ý:** Dựa vào giao thức và mạch đảo mức thực tế; không bật chỉ vì nhận dữ liệu sai.

### 06.1 Ví dụ cấu hình và vòng nhận byte

```cpp
#include <stdint.h>
#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "esp_err.h"

void uart_example_init(uart_port_t port, int tx, int rx)
{
    uart_config_t cfg = {};
    cfg.baud_rate = 115200;
    cfg.data_bits = UART_DATA_8_BITS;
    cfg.parity = UART_PARITY_DISABLE;
    cfg.stop_bits = UART_STOP_BITS_1;
    cfg.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
    cfg.source_clk = UART_SCLK_DEFAULT;
    ESP_ERROR_CHECK(uart_param_config(port, &cfg));
    ESP_ERROR_CHECK(uart_set_pin(port, tx, rx,
                                UART_PIN_NO_CHANGE,
                                UART_PIN_NO_CHANGE));
    ESP_ERROR_CHECK(uart_driver_install(
        port, 1024, 0, 0, NULL, 0));
}

// Inside a receiver task after init:
// uint8_t buf[64];
// for (;;) {
//     int n = uart_read_bytes(port, buf, sizeof(buf),
//                             pdMS_TO_TICKS(20));
//     if (n < 0) { handle error; }
//     else if (n > 0) { feed buf[0..n-1] into parser; }
//     check parser timeout even if n == 0;
// }
```

Parser phải giữ trạng thái giữa lần đọc, hỗ trợ frame bị chia và nhiều frame trong một buffer, kiểm tra length/CRC và timeout. Khi UART overflow, phục hồi đồng bộ thay vì coi dữ liệu kế tiếp luôn bắt đầu frame mới.

**Nguồn:** [UART](https://docs.espressif.com/projects/esp-idf/en/v6.1/esp32/api-reference/peripherals/uart.html), [Header UART](https://github.com/espressif/esp-idf/blob/v6.1/components/esp_driver_uart/include/driver/uart.h).

<a id="07-time"></a>

## 07. Timestamp, delay và ESP Timer

**Header:** `esp_timer.h`; `esp_rom_sys.h` cho ROM delay; `freertos/task.h` cho delay nhường CPU. Timestamp, lịch callback và độ dài sleep là ba khái niệm khác nhau.

#### `esp_timer_create_args_t`

**Loại:** struct. **Dùng để:** mô tả timer mềm và callback

| Trường / giá trị | Kiểu | Ý nghĩa, giá trị và cách chọn |
| --- | --- | --- |
| callback | esp_timer_cb_t | Hàm `void callback(void *arg)`; không thêm dấu () khi gán. |
| arg | void * | Context được chuyển nguyên vào callback; phải còn sống. |
| dispatch_method | esp_timer_dispatch_t | ESP_TIMER_TASK mặc định; ESP_TIMER_ISR cần cấu hình hỗ trợ và quy tắc ISR. |
| name | const char * | Tên timer để chẩn đoán; dùng chuỗi sống đủ lâu. |
| skip_unhandled_events | bool | true có thể bỏ bớt sự kiện periodic trễ; lựa chọn theo ý nghĩa ứng dụng. |

**Cú pháp sử dụng:**

```cpp
void timer_callback(void *arg) { /* đánh thức task */ }
esp_timer_create_args_t args = {};
args.callback = timer_callback;
args.arg = nullptr;
args.dispatch_method = ESP_TIMER_TASK;
args.name = "periodic";
```

#### `esp_timer_handle_t`

**Loại:** opaque handle con trỏ. **Dùng để:** giữ timer tạo bởi esp_timer_create

**Cú pháp sử dụng:**

```cpp
esp_timer_handle_t timer = nullptr;
```

#### `esp_timer_cb_t`

**Loại:** typedef con trỏ hàm. **Dùng để:** entry callback của ESP Timer

**Cú pháp sử dụng:**

```cpp
void timer_callback(void *arg);
```

**Lưu ý:** ESP_TIMER_TASK gọi từ task timer dùng chung; callback ngắn và không block. ISR mode có yêu cầu khác, không dùng API task thường.

#### `esp_timer_get_time`

Lấy timestamp micro giây từ khi ESP Timer khởi tạo.

```c
int64_t esp_timer_get_time(void);
```

**Tham số:** không có; `void` trong danh sách tham số nghĩa là hàm không nhận đối số.

**Trả về:** int64_t micro giây; không phải epoch UTC và mốc lại sau deep sleep.

**Cách gọi:**

```cpp
int64_t now_us = esp_timer_get_time();
```

#### `esp_timer_create`

Tạo timer, chưa chạy.

```c
esp_err_t esp_timer_create(const esp_timer_create_args_t *create_args,
                           esp_timer_handle_t *out_handle);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| create_args | const esp_timer_create_args_t * / in | Địa chỉ cấu hình. |
| out_handle | esp_timer_handle_t * / out | Địa chỉ biến nhận timer. |

**Trả về:** `ESP_OK` nếu thành công; mã `esp_err_t` khác nếu thất bại. Chỉ dùng kết quả đầu ra sau khi kiểm tra thành công.

**Cách gọi:**

```cpp
esp_timer_create(&args, &timer);
```

#### `esp_timer_start_once`

Chạy một lần sau khoảng chờ.

```c
esp_err_t esp_timer_start_once(esp_timer_handle_t timer, uint64_t timeout_us);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| timer | esp_timer_handle_t / in | Timer đã tạo, chưa chạy. |
| timeout_us | uint64_t / in | Micro giây, ví dụ 10000 là 10 ms. |

**Trả về:** `ESP_OK` nếu thành công; mã `esp_err_t` khác nếu thất bại. Chỉ dùng kết quả đầu ra sau khi kiểm tra thành công.

**Cách gọi:**

```cpp
esp_timer_start_once(timer, 10000);
```

**Điều kiện / chú ý:** Độ phân giải timestamp 1 us không có nghĩa callback chạy chính xác tuyệt đối ở mọi chu kỳ.

#### `esp_timer_start_periodic`

Chạy định kỳ.

```c
esp_err_t esp_timer_start_periodic(esp_timer_handle_t timer, uint64_t period);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| timer | esp_timer_handle_t / in | Timer đã tạo, chưa chạy. |
| period | uint64_t / in | Micro giây, ví dụ 10000 là 10 ms. |

**Trả về:** `ESP_OK` nếu thành công; mã `esp_err_t` khác nếu thất bại. Chỉ dùng kết quả đầu ra sau khi kiểm tra thành công.

**Cách gọi:**

```cpp
esp_timer_start_periodic(timer, 10000);
```

**Điều kiện / chú ý:** Độ phân giải timestamp 1 us không có nghĩa callback chạy chính xác tuyệt đối ở mọi chu kỳ.

#### `esp_timer_stop`

Dừng timer đang chạy.

```c
esp_err_t esp_timer_stop(esp_timer_handle_t timer);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| timer | esp_timer_handle_t / in | Timer cần thao tác. |

**Trả về:** `ESP_OK` nếu thành công; mã `esp_err_t` khác nếu thất bại. Chỉ dùng kết quả đầu ra sau khi kiểm tra thành công.

**Cách gọi:**

```cpp
esp_timer_stop(timer);
```

**Điều kiện / chú ý:** Không hủy context khi callback còn truy cập nó.

#### `esp_timer_delete`

Xóa timer đã dừng và giải phóng tài nguyên.

```c
esp_err_t esp_timer_delete(esp_timer_handle_t timer);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| timer | esp_timer_handle_t / in | Timer cần thao tác. |

**Trả về:** `ESP_OK` nếu thành công; mã `esp_err_t` khác nếu thất bại. Chỉ dùng kết quả đầu ra sau khi kiểm tra thành công.

**Cách gọi:**

```cpp
esp_timer_delete(timer);
```

**Điều kiện / chú ý:** Không hủy context khi callback còn truy cập nó.

#### `esp_rom_delay_us`

Busy-wait cho khoảng chờ ngắn.

```c
void esp_rom_delay_us(uint32_t us);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| us | uint32_t / in | Số micro giây; ví dụ 50. |

**Trả về:** void.

**Cách gọi:**

```cpp
esp_rom_delay_us(50);
```

**Điều kiện / chú ý:** Thuộc nhóm ROM/internal-unstable. Bọc trong hàm port khi muốn dễ đổi SDK. Không dùng busy-wait dài để thay mọi delay task.

### 07.1 Ví dụ periodic timer đánh thức task

```cpp
#include <stdint.h>
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"

static void sample_tick(void *arg)
{
    // Default ESP_TIMER_TASK context: not an ISR.
    xTaskNotifyGive((TaskHandle_t)arg);
}

void start_sample_timer(TaskHandle_t task,
                        esp_timer_handle_t *out)
{
    esp_timer_create_args_t args = {};
    args.callback = sample_tick;
    args.arg = (void *)task;
    args.dispatch_method = ESP_TIMER_TASK;
    args.name = "sample";
    ESP_ERROR_CHECK(esp_timer_create(&args, out));
    ESP_ERROR_CHECK(esp_timer_start_periodic(*out, 10000));
}
```

`pdMS_TO_TICKS(1)` có thể bằng 0. Một tick cũng không bảo đảm thời gian chờ tối thiểu đúng 1 ms do thời điểm gọi trong tick. Với yêu cầu min-delay của IC, chọn cơ chế chờ có kiểm soát timestamp hoặc busy-wait ngắn phù hợp.

**Nguồn:** [ESP Timer](https://docs.espressif.com/projects/esp-idf/en/v6.1/esp32/api-reference/system/esp_timer.html), [FreeRTOS trong ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/v6.1/esp32/api-reference/system/freertos_idf.html), [ROM API: internal / unstable](https://docs.espressif.com/projects/esp-idf/en/v6.1/esp32/api-reference/system/internal-unstable.html).

<a id="08-gptimer"></a>

## 08. GPTimer

**Header:** `driver/gptimer.h`. Timer phần cứng dùng số đếm; `resolution_hz=1000000` nghĩa là 1 count = 1 us. ISR có thể phát nhịp cho task; task chạy sau đó vẫn có latency.

#### `gptimer_config_t`

**Loại:** struct. **Dùng để:** cấu hình timer đếm

| Trường / giá trị | Kiểu | Ý nghĩa, giá trị và cách chọn |
| --- | --- | --- |
| clk_src | gptimer_clock_source_t | GPTIMER_CLK_SRC_DEFAULT hoặc nguồn target hỗ trợ. |
| direction | gptimer_count_direction_t | GPTIMER_COUNT_UP hoặc GPTIMER_COUNT_DOWN. |
| resolution_hz | uint32_t | Số count mỗi giây; 1000000 tương đương 1 us/count. |
| intr_priority | int | 0 để driver chọn priority mặc định. |
| flags.intr_shared | uint32_t bit-field | true cho phép chia sẻ ngắt; false mặc định nếu không cần. |
| flags.allow_pd | uint32_t bit-field | Cho phép lưu/khôi phục context để tắt miền nguồn khi sleep nếu target hỗ trợ; false mặc định. |

**Cú pháp sử dụng:**

```cpp
gptimer_config_t cfg = {};
cfg.clk_src = GPTIMER_CLK_SRC_DEFAULT;
cfg.direction = GPTIMER_COUNT_UP;
cfg.resolution_hz = 1000000;
```

#### `gptimer_alarm_config_t`

**Loại:** struct. **Dùng để:** chọn mốc alarm và nạp lại

| Trường / giá trị | Kiểu | Ý nghĩa, giá trị và cách chọn |
| --- | --- | --- |
| alarm_count | uint64_t | Số đếm gây alarm; phải phù hợp hướng đếm. |
| reload_count | uint64_t | Mốc nạp lại khi auto-reload. |
| flags.auto_reload_on_alarm | uint32_t bit-field | true để lặp; chu kỳ count bằng trị tuyệt đối alarm_count - reload_count. |

**Cú pháp sử dụng:**

```cpp
gptimer_alarm_config_t alarm = {};
alarm.alarm_count = 1000;
alarm.reload_count = 0;
alarm.flags.auto_reload_on_alarm = true;
```

#### `gptimer_event_callbacks_t`

**Loại:** struct chứa callback. **Dùng để:** đăng ký hàm xử lý alarm

| Trường / giá trị | Kiểu | Ý nghĩa, giá trị và cách chọn |
| --- | --- | --- |
| on_alarm | gptimer_alarm_cb_t | Callback bool(timer, event, user_ctx); true nếu đánh thức task ưu tiên cao hơn. |

**Cú pháp sử dụng:**

```cpp
bool alarm_cb(gptimer_handle_t timer,
              const gptimer_alarm_event_data_t *event, void *ctx) {
    return false;
}
gptimer_event_callbacks_t cbs = {};
cbs.on_alarm = alarm_cb;
```

#### `gptimer_alarm_event_data_t`

**Loại:** struct dữ liệu chỉ đọc trong callback. **Dùng để:** cung cấp trạng thái alarm

| Trường / giá trị | Kiểu | Ý nghĩa, giá trị và cách chọn |
| --- | --- | --- |
| count_value | uint64_t | Số đếm khi xử lý sự kiện. |
| alarm_value | uint64_t | Mốc alarm liên quan. |

**Cú pháp sử dụng:**

```cpp
// Trong callback: uint64_t count = event->count_value;
```

**Lưu ý:** Không giữ con trỏ event để dùng sau callback; copy trường cần thiết.

#### `gptimer_handle_t`

**Loại:** opaque handle con trỏ. **Dùng để:** giữ timer phần cứng đã tạo

**Cú pháp sử dụng:**

```cpp
gptimer_handle_t timer = nullptr;
```

#### `gptimer_new_timer`

Cấp tài nguyên timer.

```c
esp_err_t gptimer_new_timer(const gptimer_config_t *config,
                            gptimer_handle_t *ret_timer);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| config | const gptimer_config_t * / in | Địa chỉ cấu hình. |
| ret_timer | gptimer_handle_t * / out | Địa chỉ biến handle. |

**Trả về:** `ESP_OK` nếu thành công; mã `esp_err_t` khác nếu thất bại. Chỉ dùng kết quả đầu ra sau khi kiểm tra thành công.

**Cách gọi:**

```cpp
gptimer_new_timer(&cfg, &timer);
```

#### `gptimer_register_event_callbacks`

Đăng ký callback trước enable.

```c
esp_err_t gptimer_register_event_callbacks(gptimer_handle_t timer,
    const gptimer_event_callbacks_t *cbs, void *user_data);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| timer | gptimer_handle_t / in | Timer đã tạo. |
| cbs | const gptimer_event_callbacks_t * / in | Địa chỉ bảng callback. |
| user_data | void * / in | Context được đưa vào callback; sống đủ lâu. |

**Trả về:** `ESP_OK` nếu thành công; mã `esp_err_t` khác nếu thất bại. Chỉ dùng kết quả đầu ra sau khi kiểm tra thành công.

**Cách gọi:**

```cpp
gptimer_register_event_callbacks(timer, &cbs, context);
```

#### `gptimer_set_alarm_action`

Cấu hình alarm hoặc bỏ alarm.

```c
esp_err_t gptimer_set_alarm_action(gptimer_handle_t timer,
                                   const gptimer_alarm_config_t *config);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| timer | gptimer_handle_t / in | Timer đích. |
| config | const gptimer_alarm_config_t * / in | &alarm; nullptr để tắt chức năng alarm. |

**Trả về:** `ESP_OK` nếu thành công; mã `esp_err_t` khác nếu thất bại. Chỉ dùng kết quả đầu ra sau khi kiểm tra thành công.

**Cách gọi:**

```cpp
gptimer_set_alarm_action(timer, &alarm);
```

#### `gptimer_enable`

Enable driver sau khi đăng ký callback.

```c
esp_err_t gptimer_enable(gptimer_handle_t timer);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| timer | gptimer_handle_t / in | Handle timer ở trạng thái phù hợp. |

**Trả về:** `ESP_OK` nếu thành công; mã `esp_err_t` khác nếu thất bại. Chỉ dùng kết quả đầu ra sau khi kiểm tra thành công.

**Cách gọi:**

```cpp
gptimer_enable(timer);
```

#### `gptimer_start`

Bắt đầu đếm sau enable.

```c
esp_err_t gptimer_start(gptimer_handle_t timer);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| timer | gptimer_handle_t / in | Handle timer ở trạng thái phù hợp. |

**Trả về:** `ESP_OK` nếu thành công; mã `esp_err_t` khác nếu thất bại. Chỉ dùng kết quả đầu ra sau khi kiểm tra thành công.

**Cách gọi:**

```cpp
gptimer_start(timer);
```

#### `gptimer_stop`

Dừng đếm.

```c
esp_err_t gptimer_stop(gptimer_handle_t timer);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| timer | gptimer_handle_t / in | Handle timer ở trạng thái phù hợp. |

**Trả về:** `ESP_OK` nếu thành công; mã `esp_err_t` khác nếu thất bại. Chỉ dùng kết quả đầu ra sau khi kiểm tra thành công.

**Cách gọi:**

```cpp
gptimer_stop(timer);
```

#### `gptimer_disable`

Disable sau khi stop.

```c
esp_err_t gptimer_disable(gptimer_handle_t timer);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| timer | gptimer_handle_t / in | Handle timer ở trạng thái phù hợp. |

**Trả về:** `ESP_OK` nếu thành công; mã `esp_err_t` khác nếu thất bại. Chỉ dùng kết quả đầu ra sau khi kiểm tra thành công.

**Cách gọi:**

```cpp
gptimer_disable(timer);
```

#### `gptimer_del_timer`

Xóa tài nguyên timer đã disable.

```c
esp_err_t gptimer_del_timer(gptimer_handle_t timer);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| timer | gptimer_handle_t / in | Handle timer ở trạng thái phù hợp. |

**Trả về:** `ESP_OK` nếu thành công; mã `esp_err_t` khác nếu thất bại. Chỉ dùng kết quả đầu ra sau khi kiểm tra thành công.

**Cách gọi:**

```cpp
gptimer_del_timer(timer);
```

#### `gptimer_get_raw_count`

Đọc số đếm.

```c
esp_err_t gptimer_get_raw_count(gptimer_handle_t timer, uint64_t *value);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| timer | gptimer_handle_t / in | Timer cần đọc. |
| value | uint64_t * / out | Địa chỉ biến nhận count. |

**Trả về:** `ESP_OK` nếu thành công; mã `esp_err_t` khác nếu thất bại. Chỉ dùng kết quả đầu ra sau khi kiểm tra thành công.

**Cách gọi:**

```cpp
uint64_t count = 0;
gptimer_get_raw_count(timer, &count);
```

#### `gptimer_set_raw_count`

Đặt số đếm.

```c
esp_err_t gptimer_set_raw_count(gptimer_handle_t timer, uint64_t value);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| timer | gptimer_handle_t / in | Timer đích. |
| value | uint64_t / in | Số count mới, không phải thời gian ms. |

**Trả về:** `ESP_OK` nếu thành công; mã `esp_err_t` khác nếu thất bại. Chỉ dùng kết quả đầu ra sau khi kiểm tra thành công.

**Cách gọi:**

```cpp
gptimer_set_raw_count(timer, 0);
```

### 08.1 Ví dụ nhịp alarm đánh thức task

```cpp
#include <stdbool.h>
#include "driver/gptimer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"

static bool on_alarm(gptimer_handle_t timer,
                     const gptimer_alarm_event_data_t *event,
                     void *arg)
{
    (void)timer;
    (void)event;
    BaseType_t woken = pdFALSE;
    vTaskNotifyGiveFromISR((TaskHandle_t)arg, &woken);
    return woken == pdTRUE;
}

void start_gptimer(TaskHandle_t task, gptimer_handle_t *out)
{
    gptimer_config_t cfg = {};
    cfg.clk_src = GPTIMER_CLK_SRC_DEFAULT;
    cfg.direction = GPTIMER_COUNT_UP;
    cfg.resolution_hz = 1000000;
    ESP_ERROR_CHECK(gptimer_new_timer(&cfg, out));

    gptimer_event_callbacks_t cbs = {};
    cbs.on_alarm = on_alarm;
    ESP_ERROR_CHECK(gptimer_register_event_callbacks(
        *out, &cbs, (void *)task));

    gptimer_alarm_config_t alarm = {};
    alarm.alarm_count = 1000;
    alarm.reload_count = 0;
    alarm.flags.auto_reload_on_alarm = true;
    ESP_ERROR_CHECK(gptimer_set_alarm_action(*out, &alarm));
    ESP_ERROR_CHECK(gptimer_enable(*out));
    ESP_ERROR_CHECK(gptimer_start(*out));
}
```

**Nguồn:** [GPTimer](https://docs.espressif.com/projects/esp-idf/en/v6.1/esp32/api-reference/peripherals/gptimer.html), [Header GPTimer](https://github.com/espressif/esp-idf/blob/v6.1/components/esp_driver_gptimer/include/driver/gptimer.h), [FreeRTOS trong ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/v6.1/esp32/api-reference/system/freertos_idf.html).

<a id="09-rtos"></a>

## 09. FreeRTOS: task, notification, queue và mutex

**Header:** `freertos/FreeRTOS.h` và `freertos/task.h`, `freertos/queue.h`, `freertos/semphr.h` tùy API. Các cấu trúc quản lý task/queue là nội bộ; người dùng thao tác qua handle.

#### `BaseType_t`

**Loại:** typedef số nguyên có dấu. **Dùng để:** giữ kết quả/cờ như pdPASS, pdFAIL, pdTRUE, pdFALSE

**Cú pháp sử dụng:**

```cpp
BaseType_t ok = pdFALSE;
```

#### `UBaseType_t`

**Loại:** typedef số nguyên không dấu. **Dùng để:** priority task hoặc số item queue

**Cú pháp sử dụng:**

```cpp
UBaseType_t priority = 5;
```

#### `TickType_t`

**Loại:** typedef số nguyên đếm tick. **Dùng để:** timeout và mốc lịch FreeRTOS

**Cú pháp sử dụng:**

```cpp
TickType_t wait = pdMS_TO_TICKS(20);
```

#### `TaskHandle_t`

**Loại:** opaque handle. **Dùng để:** tham chiếu task đã tạo

**Cú pháp sử dụng:**

```cpp
TaskHandle_t task = nullptr;
```

#### `QueueHandle_t`

**Loại:** opaque handle. **Dùng để:** tham chiếu queue có kiểu/kích thước item cố định

**Cú pháp sử dụng:**

```cpp
QueueHandle_t q = nullptr;
```

#### `SemaphoreHandle_t`

**Loại:** opaque handle. **Dùng để:** tham chiếu semaphore hoặc mutex

**Cú pháp sử dụng:**

```cpp
SemaphoreHandle_t mutex = nullptr;
```

#### `TaskFunction_t`

**Loại:** typedef con trỏ hàm. **Dùng để:** entry của task dạng void (*)(void *)

**Cú pháp sử dụng:**

```cpp
void worker(void *arg) {
    for (;;) { vTaskDelay(pdMS_TO_TICKS(10)); }
}
```

#### `portMUX_TYPE`

**Loại:** struct/spinlock của port SMP. **Dùng để:** bảo vệ critical section cực ngắn

**Cú pháp sử dụng:**

```cpp
portMUX_TYPE lock = portMUX_INITIALIZER_UNLOCKED;
```

**Lưu ý:** Dùng portENTER_CRITICAL(&lock)/portEXIT_CRITICAL(&lock) theo port; không gọi bus blocking, delay hoặc chờ mutex bên trong. volatile không thay cơ chế đồng bộ.

### 09.1 Tạo task và lịch chạy

#### `xTaskCreate`

Tạo task và đưa vào scheduler.

```c
BaseType_t xTaskCreate(TaskFunction_t task_fn, const char *name,
                       configSTACK_DEPTH_TYPE stack_bytes, void *arg,
                       UBaseType_t priority, TaskHandle_t *out_task);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| task_fn | TaskFunction_t / in | Tên hàm void worker(void *arg), không thêm (). |
| name | const char * / in | Tên dùng debug. |
| stack_bytes | configSTACK_DEPTH_TYPE / in | Kích thước stack theo BYTE trong ESP-IDF; ví dụ 4096 chỉ là điểm bắt đầu, phải đo. |
| arg | void * / in | Context sống đủ lâu; nullptr nếu không cần. |
| priority | UBaseType_t / in | 0 ... configMAX_PRIORITIES-1, số lớn ưu tiên cao hơn. |
| out_task | TaskHandle_t * / out | &task để nhận handle hoặc nullptr nếu không cần giữ. |

**Trả về:** pdPASS nếu thành công; giá trị thất bại nếu không tạo được.

**Cách gọi:**

```cpp
BaseType_t ok = xTaskCreate(worker, "worker", 4096,
                           nullptr, 5, &task);
```

**Điều kiện / chú ý:** Task không được return; dùng vòng lặp hoặc vTaskDelete(nullptr). Priority/stack phụ thuộc ứng dụng.

#### `xTaskCreatePinnedToCore`

Tạo task có affinity core.

```c
BaseType_t xTaskCreatePinnedToCore(TaskFunction_t task_fn, const char *name,
    uint32_t stack_bytes, void *arg, UBaseType_t priority,
    TaskHandle_t *out_task, BaseType_t core_id);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| task_fn | TaskFunction_t / in | Entry task. |
| name | const char * / in | Tên task. |
| stack_bytes | uint32_t / in | Stack byte trong ESP-IDF. |
| arg | void * / in | Context. |
| priority | UBaseType_t / in | Mức ưu tiên. |
| out_task | TaskHandle_t * / out | Địa chỉ biến handle. |
| core_id | BaseType_t / in | 0/1 nếu target có core đó; tskNO_AFFINITY để không pin. |

**Trả về:** pdPASS khi tạo thành công.

**Cách gọi:**

```cpp
xTaskCreatePinnedToCore(worker, "worker", 4096,
                        nullptr, 5, &task, 0);
```

**Điều kiện / chú ý:** Không pin core chỉ vì có hai core; kiểm tra cấu hình UNICORE và tải thực tế.

#### `vTaskDelay`

Nhường CPU trong thời gian theo tick.

```c
void vTaskDelay(TickType_t ticks);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| ticks | TickType_t / in | Số RTOS tick; pdMS_TO_TICKS(ms) đổi từ mili giây. |

**Trả về:** void.

**Cách gọi:**

```cpp
vTaskDelay(pdMS_TO_TICKS(10));
```

**Điều kiện / chú ý:** 0 không tạo khoảng chờ 10 ms; kiểm tra CONFIG_FREERTOS_HZ và làm tròn.

#### `xTaskGetTickCount`

Lấy mốc tick scheduler.

```c
TickType_t xTaskGetTickCount(void);
```

**Tham số:** không có; `void` trong danh sách tham số nghĩa là hàm không nhận đối số.

**Trả về:** TickType_t; khác timestamp micro giây.

**Cách gọi:**

```cpp
TickType_t last = xTaskGetTickCount();
```

#### `xTaskDelayUntil`

Chạy task theo lịch tương đối với mốc trước.

```c
BaseType_t xTaskDelayUntil(TickType_t *previous_wake, TickType_t period);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| previous_wake | TickType_t * / in/out | Địa chỉ biến last, khởi tạo trước vòng lặp bằng xTaskGetTickCount. |
| period | TickType_t / in | Chu kỳ tick >0. |

**Trả về:** pdTRUE nếu task thực sự delay, pdFALSE nếu mốc đã qua.

**Cách gọi:**

```cpp
TickType_t last = xTaskGetTickCount();
for (;;) {
    // Xử lý một chu kỳ.
    xTaskDelayUntil(&last, pdMS_TO_TICKS(10));
}
```

**Điều kiện / chú ý:** Đo dt thực nếu thuật toán cần; lịch gọi không bảo đảm thời điểm lấy mẫu của IC.

#### `vTaskDelete`

Hủy task.

```c
void vTaskDelete(TaskHandle_t task);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| task | TaskHandle_t / in | Handle task hoặc nullptr cho chính task hiện tại. |

**Trả về:** void.

**Cách gọi:**

```cpp
vTaskDelete(nullptr);
```

**Điều kiện / chú ý:** Thu hồi tài nguyên ứng dụng và bỏ đăng ký watchdog/ngắt trước khi task không còn tồn tại.

### 09.2 Notification

#### `xTaskNotifyGive`

Tăng notification counter của task từ ngữ cảnh task.

```c
// Macro dạng gọi:
xTaskNotifyGive(task);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| task | TaskHandle_t / in | Task đích còn tồn tại. |

**Trả về:** BaseType_t, thông thường pdPASS.

**Cách gọi:**

```cpp
xTaskNotifyGive(task);
```

#### `ulTaskNotifyTake`

Chờ notification counter khác 0.

```c
uint32_t ulTaskNotifyTake(BaseType_t clear_on_exit, TickType_t ticks_to_wait);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| clear_on_exit | BaseType_t / in | pdTRUE xóa counter về 0; pdFALSE giảm 1 khi nhận. |
| ticks_to_wait | TickType_t / in | Thời gian chờ tick. |

**Trả về:** Giá trị counter trước khi xóa/giảm; 0 là hết hạn mà chưa có notification.

**Cách gọi:**

```cpp
uint32_t count = ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(100));
```

**Điều kiện / chú ý:** pdTRUE có thể gộp nhiều sự kiện. Notification không chứa toàn bộ mẫu dữ liệu như queue.

#### `vTaskNotifyGiveFromISR`

Tăng counter từ ISR.

```c
// Macro dạng gọi:
vTaskNotifyGiveFromISR(task, &higher_priority_woken);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| task | TaskHandle_t / in | Task đích. |
| higher_priority_woken | BaseType_t * / out | Khởi tạo biến pdFALSE; API đặt pdTRUE nếu đánh thức task cần chuyển lịch. |

**Trả về:** void.

**Cách gọi:**

```cpp
BaseType_t woken = pdFALSE;
vTaskNotifyGiveFromISR(task, &woken);
if (woken) portYIELD_FROM_ISR();
```

**Điều kiện / chú ý:** Ở callback ngoại vi như GPTimer, trả bool theo driver thay vì tự yield thêm; xem mẫu GPTimer.

### 09.3 Queue

#### `xQueueCreate`

Cấp queue sao chép item.

```c
// Macro dạng gọi:
xQueueCreate(queue_length, item_size);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| queue_length | UBaseType_t / in | Số item tối đa, không phải byte. |
| item_size | UBaseType_t / in | Kích thước mỗi item theo byte, thường sizeof(sample_t). |

**Trả về:** QueueHandle_t hoặc nullptr nếu không đủ tài nguyên.

**Cách gọi:**

```cpp
QueueHandle_t q = xQueueCreate(8, sizeof(sample_t));
```

#### `xQueueSend`

Sao chép một item vào queue.

```c
// Macro dạng gọi:
xQueueSend(queue, item_pointer, ticks_to_wait);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| queue | QueueHandle_t / in | Queue đã tạo. |
| item_pointer | const void * / in | Địa chỉ item có đúng kích thước đã chọn khi tạo queue. |
| ticks_to_wait | TickType_t / in | Chờ chỗ trống/dữ liệu theo tick; 0 không chờ. |

**Trả về:** pdPASS/pdTRUE khi thành công; giá trị không thành công khi hết thời gian.

**Cách gọi:**

```cpp
BaseType_t ok = xQueueSend(q, &sample, pdMS_TO_TICKS(20));
```

#### `xQueueReceive`

Lấy một item ra queue.

```c
// Macro dạng gọi:
xQueueReceive(queue, item_pointer, ticks_to_wait);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| queue | QueueHandle_t / in | Queue đã tạo. |
| item_pointer | void * / out | Địa chỉ item có đúng kích thước đã chọn khi tạo queue. |
| ticks_to_wait | TickType_t / in | Chờ chỗ trống/dữ liệu theo tick; 0 không chờ. |

**Trả về:** pdPASS/pdTRUE khi thành công; giá trị không thành công khi hết thời gian.

**Cách gọi:**

```cpp
BaseType_t ok = xQueueReceive(q, &sample, pdMS_TO_TICKS(20));
```

#### `xQueueOverwrite`

Ghi mẫu mới nhất, thay item cũ khi queue đầy.

```c
// Macro dạng gọi:
xQueueOverwrite(queue, item_pointer);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| queue | QueueHandle_t / in | Chỉ dùng queue có chiều dài 1. |
| item_pointer | const void * / in | Địa chỉ item mới. |

**Trả về:** pdPASS khi dùng đúng điều kiện.

**Cách gọi:**

```cpp
xQueueOverwrite(latest_queue, &sample);
```

**Điều kiện / chú ý:** Không dùng latest-only nếu thuật toán cần tích phân mọi mẫu; cần chính sách tràn rõ ràng.

#### `vQueueDelete`

Giải phóng queue.

```c
void vQueueDelete(QueueHandle_t queue);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| queue | QueueHandle_t / in | Queue không còn task/ISR dùng. |

**Trả về:** void.

**Cách gọi:**

```cpp
vQueueDelete(q);
```

### 09.4 Mutex

#### `xSemaphoreCreateMutex`

Tạo mutex có priority inheritance.

```c
// Macro không có đối số:
xSemaphoreCreateMutex();
```

**Tham số:** không có; `void` trong danh sách tham số nghĩa là hàm không nhận đối số.

**Trả về:** SemaphoreHandle_t hoặc nullptr khi thất bại.

**Cách gọi:**

```cpp
SemaphoreHandle_t mutex = xSemaphoreCreateMutex();
```

#### `xSemaphoreTake`

Lấy mutex/semaphore.

```c
// Macro dạng gọi:
xSemaphoreTake(semaphore, ticks_to_wait);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| semaphore | SemaphoreHandle_t / in | Handle đã tạo. |
| ticks_to_wait | TickType_t / in | Thời gian chờ; 0 không block. |

**Trả về:** pdTRUE nếu lấy được, pdFALSE nếu không.

**Cách gọi:**

```cpp
if (xSemaphoreTake(mutex, pdMS_TO_TICKS(20)) == pdTRUE) {
    // Thao tác tài nguyên; mọi nhánh phải nhả mutex.
    xSemaphoreGive(mutex);
}
```

**Điều kiện / chú ý:** Không dùng mutex trong ISR; không truy cập tài nguyên sau Take thất bại.

#### `xSemaphoreGive`

Nhả mutex/semaphore.

```c
// Macro dạng gọi:
xSemaphoreGive(semaphore);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| semaphore | SemaphoreHandle_t / in | Mutex do task hiện tại sở hữu. |

**Trả về:** BaseType_t báo thành công/thất bại.

**Cách gọi:**

```cpp
xSemaphoreGive(mutex);
```

#### `vSemaphoreDelete`

Giải phóng semaphore/mutex.

```c
// Macro dạng gọi:
vSemaphoreDelete(semaphore);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| semaphore | SemaphoreHandle_t / in | Handle không còn người dùng/chờ. |

**Trả về:** void.

**Cách gọi:**

```cpp
vSemaphoreDelete(mutex);
```

### 09.5 Ví dụ dữ liệu mẫu và quyền sở hữu

```cpp
#include <stdbool.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_timer.h"

typedef struct {
    int64_t timestamp_us;
    int16_t dx, dy;
    uint32_t sequence;
} flow_sample_t;

static QueueHandle_t samples;
static SemaphoreHandle_t device_mutex;

// Create once; check failures before starting tasks.
static bool shared_init(void)
{
    samples = xQueueCreate(8, sizeof(flow_sample_t));
    if (samples == NULL) return false;
    device_mutex = xSemaphoreCreateMutex();
    if (device_mutex == NULL) {
        vQueueDelete(samples);
        samples = NULL;
        return false;
    }
    return true;
}

// Producer, after a successful sensor read:
// flow_sample_t s = {...};
// if (xQueueSend(samples, &s, 0) != pdPASS) {
//     count/drop sample according to app policy;
// }

// Guard a complete sensor operation if sharing a device:
// if (xSemaphoreTake(device_mutex, timeout) == pdTRUE) {
//     read/write the sensor; record result;
//     xSemaphoreGive(device_mutex); // also on error paths
// }
```

**Nguồn:** [FreeRTOS trong ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/v6.1/esp32/api-reference/system/freertos_idf.html).

<a id="10-ledc"></a>

## 10. LEDC

**Header:** `driver/ledc.h`. Timer quyết định tần số/độ phân giải; channel nối chân và duty vào timer. ESP32-S3 dùng LOW_SPEED_MODE; ESP32 thường có cả nhóm high-speed.

#### `ledc_timer_config_t`

**Loại:** struct. **Dùng để:** cấu hình thời gian chung của nhóm channel

| Trường / giá trị | Kiểu | Ý nghĩa, giá trị và cách chọn |
| --- | --- | --- |
| speed_mode | ledc_mode_t | LEDC_LOW_SPEED_MODE; high-speed tùy target. |
| duty_resolution | ledc_timer_bit_t | Số bit duty, ví dụ LEDC_TIMER_14_BIT; phải phù hợp tần số/clock. |
| timer_num | ledc_timer_t | LEDC_TIMER_0 ... theo số timer hỗ trợ. |
| freq_hz | uint32_t | Tần số PWM theo Hz, không phải độ rộng xung. |
| clk_cfg | ledc_clk_cfg_t | LEDC_AUTO_CLK để driver chọn clock. |
| deconfigure | bool | true yêu cầu deconfigure timer khi đủ điều kiện; cấu hình tạo bình thường để false. |

**Cú pháp sử dụng:**

```cpp
ledc_timer_config_t timer_cfg = {};
timer_cfg.speed_mode = LEDC_LOW_SPEED_MODE;
timer_cfg.timer_num = LEDC_TIMER_0;
timer_cfg.duty_resolution = LEDC_TIMER_14_BIT;
timer_cfg.freq_hz = 50;
timer_cfg.clk_cfg = LEDC_AUTO_CLK;
```

#### `ledc_channel_config_t`

**Loại:** struct. **Dùng để:** nối GPIO/channel với timer và chọn duty ban đầu

| Trường / giá trị | Kiểu | Ý nghĩa, giá trị và cách chọn |
| --- | --- | --- |
| gpio_num | int | GPIO output. |
| speed_mode | ledc_mode_t | Khớp speed_mode timer. |
| channel | ledc_channel_t | LEDC_CHANNEL_0 ... trong nhóm. |
| timer_sel | ledc_timer_t | Timer đã cấu hình. |
| duty | uint32_t | Số đếm mức cao, chưa phải us; chọn trong dải độ phân giải. |
| hpoint | int | Mốc bắt đầu mức cao trong chu kỳ; thường 0. |
| sleep_mode | ledc_sleep_mode_t | Chính sách output khi light sleep; LEDC_SLEEP_MODE_NO_ALIVE_NO_PD là mặc định trong SDK tham chiếu. |
| deconfigure | bool | true để deconfigure channel đã tạo, không dùng khi cấu hình phát bình thường. |
| flags.output_invert | unsigned bit-field | Đảo mức đầu ra nếu cần. |
| intr_type | ledc_intr_type_t, deprecated | Không thiết lập cho code mới v6.1; driver quản lý ngắt fade. |

**Cú pháp sử dụng:**

```cpp
ledc_channel_config_t channel_cfg = {};
channel_cfg.gpio_num = output_pin;
channel_cfg.speed_mode = LEDC_LOW_SPEED_MODE;
channel_cfg.channel = LEDC_CHANNEL_0;
channel_cfg.timer_sel = LEDC_TIMER_0;
channel_cfg.duty = 0;
channel_cfg.hpoint = 0;
```

#### `ledc_mode_t`

**Loại:** enum. **Dùng để:** chọn một định danh trong cấu hình LEDC

| Trường / giá trị | Kiểu | Ý nghĩa, giá trị và cách chọn |
| --- | --- | --- |
| LEDC_LOW_SPEED_MODE; HIGH_SPEED chỉ khi target hỗ trợ | giá trị enum | Không dùng số GPIO thay cho số timer/channel. |

**Cú pháp sử dụng:**

```cpp
// Dùng giá trị enum tương ứng trong các trường ở trên.
```

#### `ledc_timer_t`

**Loại:** enum. **Dùng để:** chọn một định danh trong cấu hình LEDC

| Trường / giá trị | Kiểu | Ý nghĩa, giá trị và cách chọn |
| --- | --- | --- |
| LEDC_TIMER_0... | giá trị enum | Không dùng số GPIO thay cho số timer/channel. |

**Cú pháp sử dụng:**

```cpp
// Dùng giá trị enum tương ứng trong các trường ở trên.
```

#### `ledc_channel_t`

**Loại:** enum. **Dùng để:** chọn một định danh trong cấu hình LEDC

| Trường / giá trị | Kiểu | Ý nghĩa, giá trị và cách chọn |
| --- | --- | --- |
| LEDC_CHANNEL_0... | giá trị enum | Không dùng số GPIO thay cho số timer/channel. |

**Cú pháp sử dụng:**

```cpp
// Dùng giá trị enum tương ứng trong các trường ở trên.
```

#### `ledc_timer_bit_t`

**Loại:** enum. **Dùng để:** chọn một định danh trong cấu hình LEDC

| Trường / giá trị | Kiểu | Ý nghĩa, giá trị và cách chọn |
| --- | --- | --- |
| LEDC_TIMER_8_BIT, LEDC_TIMER_14_BIT... trong giới hạn target | giá trị enum | Không dùng số GPIO thay cho số timer/channel. |

**Cú pháp sử dụng:**

```cpp
// Dùng giá trị enum tương ứng trong các trường ở trên.
```

#### `ledc_timer_config`

Cấu hình timer.

```c
esp_err_t ledc_timer_config(const ledc_timer_config_t *timer_conf);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| timer_conf | const ledc_timer_config_t * / in | Địa chỉ cấu hình timer. |

**Trả về:** `ESP_OK` nếu thành công; mã `esp_err_t` khác nếu thất bại. Chỉ dùng kết quả đầu ra sau khi kiểm tra thành công.

**Cách gọi:**

```cpp
ledc_timer_config(&timer_cfg);
```

#### `ledc_channel_config`

Cấu hình channel.

```c
esp_err_t ledc_channel_config(const ledc_channel_config_t *ledc_conf);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| ledc_conf | const ledc_channel_config_t * / in | Địa chỉ cấu hình channel. |

**Trả về:** `ESP_OK` nếu thành công; mã `esp_err_t` khác nếu thất bại. Chỉ dùng kết quả đầu ra sau khi kiểm tra thành công.

**Cách gọi:**

```cpp
ledc_channel_config(&channel_cfg);
```

#### `ledc_set_duty_and_update`

Đổi duty và cập nhật bằng API thread-safe.

```c
esp_err_t ledc_set_duty_and_update(ledc_mode_t speed_mode,
    ledc_channel_t channel, uint32_t duty, uint32_t hpoint);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| speed_mode | ledc_mode_t / in | Nhóm tốc độ đã cấu hình. |
| channel | ledc_channel_t / in | Channel đích. |
| duty | uint32_t / in | Số count duty; dùng giá trị nhỏ hơn 2^N là lựa chọn bảo thủ cho PWM thông thường. |
| hpoint | uint32_t / in | Mốc bắt đầu, thường 0. |

**Trả về:** `ESP_OK` nếu thành công; mã `esp_err_t` khác nếu thất bại. Chỉ dùng kết quả đầu ra sau khi kiểm tra thành công.

**Cách gọi:**

```cpp
ledc_set_duty_and_update(LEDC_LOW_SPEED_MODE,
                         LEDC_CHANNEL_0, 1229, 0);
```

#### `ledc_stop`

Dừng phát channel.

```c
esp_err_t ledc_stop(ledc_mode_t speed_mode, ledc_channel_t channel,
                    uint32_t idle_level);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| speed_mode | ledc_mode_t / in | Nhóm tốc độ. |
| channel | ledc_channel_t / in | Channel. |
| idle_level | uint32_t / in | Mức nghỉ 0 hoặc 1. |

**Trả về:** `ESP_OK` nếu thành công; mã `esp_err_t` khác nếu thất bại. Chỉ dùng kết quả đầu ra sau khi kiểm tra thành công.

**Cách gọi:**

```cpp
ledc_stop(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0);
```

Với N bit: `duty ≈ pulse_us × freq_hz × 2^N / 1000000`. Nhân bằng uint64_t để tránh tràn; làm tròn và giới hạn. Ví dụ 50 Hz, N=14, pulse=1500 us cho duty≈1229; không truyền trực tiếp 1500 với ý nghĩa us vào tham số duty.

### 10.1 Ví dụ PWM

```cpp
#include "driver/ledc.h"
#include "esp_err.h"

void pwm_example(gpio_num_t pin)
{
    ledc_timer_config_t t = {};
    t.speed_mode = LEDC_LOW_SPEED_MODE;
    t.timer_num = LEDC_TIMER_0;
    t.duty_resolution = LEDC_TIMER_14_BIT;
    t.freq_hz = 50;
    t.clk_cfg = LEDC_AUTO_CLK;
    ESP_ERROR_CHECK(ledc_timer_config(&t));

    ledc_channel_config_t ch = {};
    ch.gpio_num = pin;
    ch.speed_mode = LEDC_LOW_SPEED_MODE;
    ch.channel = LEDC_CHANNEL_0;
    ch.timer_sel = LEDC_TIMER_0;
    ch.duty = 0;
    ch.hpoint = 0;
    ESP_ERROR_CHECK(ledc_channel_config(&ch));
    // 1500 us in a 20 ms period: duty ~ 1229.
    ESP_ERROR_CHECK(ledc_set_duty_and_update(
        LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 1229, 0));
}
```

**Nguồn:** [LEDC](https://docs.espressif.com/projects/esp-idf/en/v6.1/esp32/api-reference/peripherals/ledc.html), [Migration 6.0: peripheral drivers](https://docs.espressif.com/projects/esp-idf/en/v6.1/esp32/migration-guides/release-6.x/6.0/peripherals.html).

<a id="11-mcpwm"></a>

## 11. MCPWM

**Header:** `driver/mcpwm_prelude.h`. Chuỗi object: timer → operator → comparator/generator → action → enable/start. Mỗi object được tạo ra nhận một handle riêng.

#### `mcpwm_timer_config_t`

**Loại:** struct. **Dùng để:** cấu hình bộ đếm PWM

| Trường / giá trị | Kiểu | Ý nghĩa, giá trị và cách chọn |
| --- | --- | --- |
| group_id | int | Nhóm MCPWM, ví dụ 0; phải có trên target. |
| clk_src | mcpwm_timer_clock_source_t | MCPWM_TIMER_CLK_SRC_DEFAULT. |
| resolution_hz | uint32_t | Count mỗi giây. |
| count_mode | mcpwm_timer_count_mode_t | Ví dụ MCPWM_TIMER_COUNT_MODE_UP. |
| period_ticks | uint32_t | Chu kỳ theo count. |
| intr_priority | int | 0 chọn mặc định. |
| flags.update_period_on_empty / update_period_on_sync | bit-field | Tùy chọn thời điểm cập nhật period; để mặc định khi không dùng. |

**Cú pháp sử dụng:**

```cpp
mcpwm_timer_config_t tc = {};
tc.group_id = 0;
tc.clk_src = MCPWM_TIMER_CLK_SRC_DEFAULT;
tc.resolution_hz = 1000000;
tc.period_ticks = 20000;
tc.count_mode = MCPWM_TIMER_COUNT_MODE_UP;
```

#### `mcpwm_operator_config_t`

**Loại:** struct. **Dùng để:** tạo operator trong nhóm

| Trường / giá trị | Kiểu | Ý nghĩa, giá trị và cách chọn |
| --- | --- | --- |
| group_id | int | Cùng group với timer. |
| intr_priority | int | 0 cho mặc định. |
| flags | bit-field theo SDK | Thời điểm cập nhật action/dead-time; giữ 0 khi không dùng tính năng đó. |

**Cú pháp sử dụng:**

```cpp
mcpwm_operator_config_t oc = {};
oc.group_id = 0;
```

#### `mcpwm_comparator_config_t`

**Loại:** struct. **Dùng để:** chọn sự kiện cập nhật compare

| Trường / giá trị | Kiểu | Ý nghĩa, giá trị và cách chọn |
| --- | --- | --- |
| intr_priority | int | 0 cho mặc định. |
| flags.update_cmp_on_tez | bit-field | Cập nhật giá trị compare tại timer zero. |
| flags.update_cmp_on_tep | bit-field | Cập nhật tại peak. |
| flags.update_cmp_on_sync | bit-field | Cập nhật khi có sync. |

**Cú pháp sử dụng:**

```cpp
mcpwm_comparator_config_t cc = {};
cc.flags.update_cmp_on_tez = true;
```

#### `mcpwm_generator_config_t`

**Loại:** struct. **Dùng để:** nối generator tới GPIO

| Trường / giá trị | Kiểu | Ý nghĩa, giá trị và cách chọn |
| --- | --- | --- |
| gen_gpio_num | int | GPIO output. |
| flags.invert_pwm | bit-field | Đảo mức PWM nếu cần. Các cờ nâng cao khác theo header target. |

**Cú pháp sử dụng:**

```cpp
mcpwm_generator_config_t gc = {};
gc.gen_gpio_num = output_pin;
```

#### `mcpwm_timer_handle_t`

**Loại:** opaque handle con trỏ. **Dùng để:** giữ object MCPWM tương ứng

**Cú pháp sử dụng:**

```cpp
mcpwm_timer_handle_t timer = nullptr;
```

**Lưu ý:** Giải phóng bằng API del tương ứng, không tự free handle.

#### `mcpwm_oper_handle_t`

**Loại:** opaque handle con trỏ. **Dùng để:** giữ object MCPWM tương ứng

**Cú pháp sử dụng:**

```cpp
mcpwm_oper_handle_t oper = nullptr;
```

**Lưu ý:** Giải phóng bằng API del tương ứng, không tự free handle.

#### `mcpwm_cmpr_handle_t`

**Loại:** opaque handle con trỏ. **Dùng để:** giữ object MCPWM tương ứng

**Cú pháp sử dụng:**

```cpp
mcpwm_cmpr_handle_t cmp = nullptr;
```

**Lưu ý:** Giải phóng bằng API del tương ứng, không tự free handle.

#### `mcpwm_gen_handle_t`

**Loại:** opaque handle con trỏ. **Dùng để:** giữ object MCPWM tương ứng

**Cú pháp sử dụng:**

```cpp
mcpwm_gen_handle_t gen = nullptr;
```

**Lưu ý:** Giải phóng bằng API del tương ứng, không tự free handle.

#### `mcpwm_gen_timer_event_action_t`

**Loại:** struct action. **Dùng để:** chỉ định mức output tại sự kiện timer

| Trường / giá trị | Kiểu | Ý nghĩa, giá trị và cách chọn |
| --- | --- | --- |
| direction | mcpwm_timer_direction_t | Hướng đếm, ví dụ MCPWM_TIMER_DIRECTION_UP. |
| event | mcpwm_timer_event_t | EMPTY/FULL của timer. |
| action | mcpwm_generator_action_t | MCPWM_GEN_ACTION_HIGH/LOW/TOGGLE/KEEP theo nhu cầu. |

**Cú pháp sử dụng:**

```cpp
// Macro tạo giá trị action; không phải hàm điều khiển tự chạy:
auto start_action = MCPWM_GEN_TIMER_EVENT_ACTION(
    MCPWM_TIMER_DIRECTION_UP, MCPWM_TIMER_EVENT_EMPTY,
    MCPWM_GEN_ACTION_HIGH);
```

#### `mcpwm_gen_compare_event_action_t`

**Loại:** struct action. **Dùng để:** chỉ định mức output khi đạt compare

| Trường / giá trị | Kiểu | Ý nghĩa, giá trị và cách chọn |
| --- | --- | --- |
| direction | mcpwm_timer_direction_t | Hướng đếm. |
| comparator | mcpwm_cmpr_handle_t | Comparator đã tạo trên operator phù hợp. |
| action | mcpwm_generator_action_t | Ví dụ LOW để kết thúc xung. |

**Cú pháp sử dụng:**

```cpp
auto stop_action = MCPWM_GEN_COMPARE_EVENT_ACTION(
    MCPWM_TIMER_DIRECTION_UP, cmp, MCPWM_GEN_ACTION_LOW);
```

#### `mcpwm_new_timer`

Tạo timer.

```c
esp_err_t mcpwm_new_timer(const mcpwm_timer_config_t *config,
                             mcpwm_timer_handle_t *ret_handle);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| config | const mcpwm_timer_config_t * / in | Địa chỉ struct tương ứng. |
| ret_handle | mcpwm_timer_handle_t * / out | Địa chỉ biến handle. |

**Trả về:** `ESP_OK` nếu thành công; mã `esp_err_t` khác nếu thất bại. Chỉ dùng kết quả đầu ra sau khi kiểm tra thành công.

**Cách gọi:**

```cpp
mcpwm_new_timer(&config, &timer);
```

#### `mcpwm_new_operator`

Tạo operator.

```c
esp_err_t mcpwm_new_operator(const mcpwm_operator_config_t *config,
                             mcpwm_oper_handle_t *ret_handle);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| config | const mcpwm_operator_config_t * / in | Địa chỉ struct tương ứng. |
| ret_handle | mcpwm_oper_handle_t * / out | Địa chỉ biến handle. |

**Trả về:** `ESP_OK` nếu thành công; mã `esp_err_t` khác nếu thất bại. Chỉ dùng kết quả đầu ra sau khi kiểm tra thành công.

**Cách gọi:**

```cpp
mcpwm_new_operator(&config, &oper);
```

#### `mcpwm_operator_connect_timer`

Nối operator với timer cùng group.

```c
esp_err_t mcpwm_operator_connect_timer(mcpwm_oper_handle_t oper,
                                       mcpwm_timer_handle_t timer);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| oper | mcpwm_oper_handle_t / in | Operator đã tạo. |
| timer | mcpwm_timer_handle_t / in | Timer cùng group. |

**Trả về:** `ESP_OK` nếu thành công; mã `esp_err_t` khác nếu thất bại. Chỉ dùng kết quả đầu ra sau khi kiểm tra thành công.

**Cách gọi:**

```cpp
mcpwm_operator_connect_timer(oper, timer);
```

#### `mcpwm_new_comparator`

Tạo comparator thuộc operator.

```c
esp_err_t mcpwm_new_comparator(mcpwm_oper_handle_t oper,
    const mcpwm_comparator_config_t *config, mcpwm_cmpr_handle_t *ret_handle);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| oper | mcpwm_oper_handle_t / in | Operator chủ. |
| config | const mcpwm_comparator_config_t * / in | Cấu hình comparator/generator tương ứng. |
| ret_handle | mcpwm_cmpr_handle_t * / out | Địa chỉ biến handle. |

**Trả về:** `ESP_OK` nếu thành công; mã `esp_err_t` khác nếu thất bại. Chỉ dùng kết quả đầu ra sau khi kiểm tra thành công.

**Cách gọi:**

```cpp
mcpwm_new_comparator(oper, &config, &cmp);
```

#### `mcpwm_new_generator`

Tạo generator thuộc operator.

```c
esp_err_t mcpwm_new_generator(mcpwm_oper_handle_t oper,
    const mcpwm_generator_config_t *config, mcpwm_gen_handle_t *ret_handle);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| oper | mcpwm_oper_handle_t / in | Operator chủ. |
| config | const mcpwm_generator_config_t * / in | Cấu hình comparator/generator tương ứng. |
| ret_handle | mcpwm_gen_handle_t * / out | Địa chỉ biến handle. |

**Trả về:** `ESP_OK` nếu thành công; mã `esp_err_t` khác nếu thất bại. Chỉ dùng kết quả đầu ra sau khi kiểm tra thành công.

**Cách gọi:**

```cpp
mcpwm_new_generator(oper, &config, &gen);
```

#### `mcpwm_generator_set_action_on_timer_event`

Đặt action khi timer tới mốc.

```c
esp_err_t mcpwm_generator_set_action_on_timer_event(
    mcpwm_gen_handle_t gen, mcpwm_gen_timer_event_action_t action);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| gen | mcpwm_gen_handle_t / in | Generator đích. |
| action | mcpwm_gen_timer_event_action_t / in | Giá trị struct tạo bằng macro tương ứng, không truyền địa chỉ &action. |

**Trả về:** `ESP_OK` nếu thành công; mã `esp_err_t` khác nếu thất bại. Chỉ dùng kết quả đầu ra sau khi kiểm tra thành công.

**Cách gọi:**

```cpp
mcpwm_generator_set_action_on_timer_event(gen, start_action);
```

#### `mcpwm_generator_set_action_on_compare_event`

Đặt action khi tới comparator.

```c
esp_err_t mcpwm_generator_set_action_on_compare_event(
    mcpwm_gen_handle_t gen, mcpwm_gen_compare_event_action_t action);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| gen | mcpwm_gen_handle_t / in | Generator đích. |
| action | mcpwm_gen_compare_event_action_t / in | Giá trị struct tạo bằng macro tương ứng, không truyền địa chỉ &action. |

**Trả về:** `ESP_OK` nếu thành công; mã `esp_err_t` khác nếu thất bại. Chỉ dùng kết quả đầu ra sau khi kiểm tra thành công.

**Cách gọi:**

```cpp
mcpwm_generator_set_action_on_compare_event(gen, stop_action);
```

#### `mcpwm_comparator_set_compare_value`

Đặt mốc compare.

```c
esp_err_t mcpwm_comparator_set_compare_value(mcpwm_cmpr_handle_t cmpr,
                                            uint32_t cmp_ticks);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| cmpr | mcpwm_cmpr_handle_t / in | Comparator. |
| cmp_ticks | uint32_t / in | Count theo resolution timer; ở 1 MHz, 1500 count là 1500 us. |

**Trả về:** `ESP_OK` nếu thành công; mã `esp_err_t` khác nếu thất bại. Chỉ dùng kết quả đầu ra sau khi kiểm tra thành công.

**Cách gọi:**

```cpp
mcpwm_comparator_set_compare_value(cmp, 1500);
```

**Điều kiện / chú ý:** Với xung cơ bản đếm lên, chọn 0 < cmp_ticks < period_ticks và giới hạn theo thiết bị.

#### `mcpwm_timer_enable`

Enable timer trước start.

```c
esp_err_t mcpwm_timer_enable(mcpwm_timer_handle_t timer);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| timer | mcpwm_timer_handle_t / in | Timer ở trạng thái phù hợp. |

**Trả về:** `ESP_OK` nếu thành công; mã `esp_err_t` khác nếu thất bại. Chỉ dùng kết quả đầu ra sau khi kiểm tra thành công.

**Cách gọi:**

```cpp
mcpwm_timer_enable(timer);
```

#### `mcpwm_timer_disable`

Disable sau khi dừng.

```c
esp_err_t mcpwm_timer_disable(mcpwm_timer_handle_t timer);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| timer | mcpwm_timer_handle_t / in | Timer ở trạng thái phù hợp. |

**Trả về:** `ESP_OK` nếu thành công; mã `esp_err_t` khác nếu thất bại. Chỉ dùng kết quả đầu ra sau khi kiểm tra thành công.

**Cách gọi:**

```cpp
mcpwm_timer_disable(timer);
```

#### `mcpwm_timer_start_stop`

Bắt đầu/dừng theo lệnh timer.

```c
esp_err_t mcpwm_timer_start_stop(mcpwm_timer_handle_t timer,
                                mcpwm_timer_start_stop_cmd_t command);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| timer | mcpwm_timer_handle_t / in | Timer đã enable. |
| command | mcpwm_timer_start_stop_cmd_t / in | Ví dụ MCPWM_TIMER_START_NO_STOP để chạy liên tục; STOP_EMPTY/STOP_FULL chọn dừng tại mốc theo enum. |

**Trả về:** `ESP_OK` nếu thành công; mã `esp_err_t` khác nếu thất bại. Chỉ dùng kết quả đầu ra sau khi kiểm tra thành công.

**Cách gọi:**

```cpp
mcpwm_timer_start_stop(timer, MCPWM_TIMER_START_NO_STOP);
```

**Điều kiện / chú ý:** Lệnh dừng tại mốc có thể có hiệu lực sau thời điểm gọi; xác nhận trạng thái trước khi xóa tài nguyên.

#### `mcpwm_del_generator`

Hủy object đã không còn phụ thuộc/hoạt động.

```c
esp_err_t mcpwm_del_generator(mcpwm_gen_handle_t handle);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| handle | mcpwm_gen_handle_t / in | Object tương ứng. |

**Trả về:** `ESP_OK` nếu thành công; mã `esp_err_t` khác nếu thất bại. Chỉ dùng kết quả đầu ra sau khi kiểm tra thành công.

**Cách gọi:**

```cpp
mcpwm_del_generator(gen);
```

**Điều kiện / chú ý:** Tháo generator/comparator trước operator; dừng/disable timer trước hủy timer.

#### `mcpwm_del_comparator`

Hủy object đã không còn phụ thuộc/hoạt động.

```c
esp_err_t mcpwm_del_comparator(mcpwm_cmpr_handle_t handle);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| handle | mcpwm_cmpr_handle_t / in | Object tương ứng. |

**Trả về:** `ESP_OK` nếu thành công; mã `esp_err_t` khác nếu thất bại. Chỉ dùng kết quả đầu ra sau khi kiểm tra thành công.

**Cách gọi:**

```cpp
mcpwm_del_comparator(cmp);
```

**Điều kiện / chú ý:** Tháo generator/comparator trước operator; dừng/disable timer trước hủy timer.

#### `mcpwm_del_operator`

Hủy object đã không còn phụ thuộc/hoạt động.

```c
esp_err_t mcpwm_del_operator(mcpwm_oper_handle_t handle);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| handle | mcpwm_oper_handle_t / in | Object tương ứng. |

**Trả về:** `ESP_OK` nếu thành công; mã `esp_err_t` khác nếu thất bại. Chỉ dùng kết quả đầu ra sau khi kiểm tra thành công.

**Cách gọi:**

```cpp
mcpwm_del_operator(oper);
```

**Điều kiện / chú ý:** Tháo generator/comparator trước operator; dừng/disable timer trước hủy timer.

#### `mcpwm_del_timer`

Hủy object đã không còn phụ thuộc/hoạt động.

```c
esp_err_t mcpwm_del_timer(mcpwm_timer_handle_t handle);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| handle | mcpwm_timer_handle_t / in | Object tương ứng. |

**Trả về:** `ESP_OK` nếu thành công; mã `esp_err_t` khác nếu thất bại. Chỉ dùng kết quả đầu ra sau khi kiểm tra thành công.

**Cách gọi:**

```cpp
mcpwm_del_timer(timer);
```

**Điều kiện / chú ý:** Tháo generator/comparator trước operator; dừng/disable timer trước hủy timer.

### 11.1 Ví dụ cấu hình action để phát xung

```cpp
#include "driver/mcpwm_prelude.h"
#include "esp_err.h"

mcpwm_cmpr_handle_t pulse_example(int gpio)
{
    mcpwm_timer_handle_t timer = NULL;
    mcpwm_timer_config_t tc = {};
    tc.group_id = 0;
    tc.clk_src = MCPWM_TIMER_CLK_SRC_DEFAULT;
    tc.resolution_hz = 1000000;
    tc.period_ticks = 20000;
    tc.count_mode = MCPWM_TIMER_COUNT_MODE_UP;
    ESP_ERROR_CHECK(mcpwm_new_timer(&tc, &timer));

    mcpwm_oper_handle_t op = NULL;
    mcpwm_operator_config_t oc = {};
    oc.group_id = 0;
    ESP_ERROR_CHECK(mcpwm_new_operator(&oc, &op));
    ESP_ERROR_CHECK(mcpwm_operator_connect_timer(op, timer));

    mcpwm_cmpr_handle_t cmp = NULL;
    mcpwm_comparator_config_t cc = {};
    cc.flags.update_cmp_on_tez = true;
    ESP_ERROR_CHECK(mcpwm_new_comparator(op, &cc, &cmp));

    mcpwm_gen_handle_t gen = NULL;
    mcpwm_generator_config_t gc = {};
    gc.gen_gpio_num = gpio;
    ESP_ERROR_CHECK(mcpwm_new_generator(op, &gc, &gen));
    ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(cmp, 1500));
    ESP_ERROR_CHECK(mcpwm_generator_set_action_on_timer_event(gen,
        MCPWM_GEN_TIMER_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP,
            MCPWM_TIMER_EVENT_EMPTY, MCPWM_GEN_ACTION_HIGH)));
    ESP_ERROR_CHECK(mcpwm_generator_set_action_on_compare_event(gen,
        MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP,
            cmp, MCPWM_GEN_ACTION_LOW)));
    ESP_ERROR_CHECK(mcpwm_timer_enable(timer));
    ESP_ERROR_CHECK(mcpwm_timer_start_stop(
        timer, MCPWM_TIMER_START_NO_STOP));
    return cmp;
}
```

**Nguồn:** [MCPWM](https://docs.espressif.com/projects/esp-idf/en/v6.1/esp32/api-reference/peripherals/mcpwm.html).

<a id="12-rmt"></a>

## 12. RMT

**Header:** `driver/rmt_tx.h`, `driver/rmt_rx.h`, `driver/rmt_encoder.h` theo tính năng. RMT phát/thu chuỗi mức logic theo duration ngoại vi; payload TX đi qua encoder.

#### `rmt_tx_channel_config_t`

**Loại:** struct. **Dùng để:** tạo channel phát

| Trường / giá trị | Kiểu | Ý nghĩa, giá trị và cách chọn |
| --- | --- | --- |
| gpio_num | gpio_num_t | GPIO output. |
| clk_src | rmt_clock_source_t | RMT_CLK_SRC_DEFAULT. |
| resolution_hz | uint32_t | Số tick RMT mỗi giây; 1000000 cho 1 us/tick. |
| mem_block_symbols | size_t | Dung lượng theo số symbol, không phải byte; non-DMA ESP32 ví dụ dùng 64. |
| trans_queue_depth | size_t | Số transaction queue, ví dụ 1. |
| intr_priority | int | 0 để driver chọn. |
| flags.invert_out | uint32_t bit-field | Đảo mức output. |
| flags.with_dma | uint32_t bit-field | Bật DMA nếu target hỗ trợ; false cho ví dụ cơ bản. |
| flags.allow_pd | uint32_t bit-field | Tùy chọn quản lý nguồn khi sleep theo target. |
| flags.init_level | uint32_t bit-field | Mức khởi tạo channel trước khi phát, 0 hoặc 1. |

**Cú pháp sử dụng:**

```cpp
rmt_tx_channel_config_t tx_cfg = {};
tx_cfg.gpio_num = output_pin;
tx_cfg.clk_src = RMT_CLK_SRC_DEFAULT;
tx_cfg.resolution_hz = 1000000;
tx_cfg.mem_block_symbols = 64;
tx_cfg.trans_queue_depth = 1;
```

**Lưu ý:** Các cờ nâng cao như open-drain/carrier phụ thuộc header/target; không bật DMA nếu chip không hỗ trợ.

#### `rmt_rx_channel_config_t`

**Loại:** struct. **Dùng để:** tạo channel nhận

| Trường / giá trị | Kiểu | Ý nghĩa, giá trị và cách chọn |
| --- | --- | --- |
| gpio_num | gpio_num_t | GPIO input. |
| clk_src | rmt_clock_source_t | Nguồn clock. |
| resolution_hz | uint32_t | Tick mỗi giây để giải mã duration. |
| mem_block_symbols | size_t | Số symbol bộ nhớ channel; ví dụ 64 khi phù hợp target. |
| intr_priority | int | 0 chọn mặc định. |
| flags.invert_in / flags.with_dma / flags.allow_pd | uint32_t bit-field từng cờ | Đảo tín hiệu, DMA, quản lý nguồn nếu hỗ trợ; mặc định false. |

**Cú pháp sử dụng:**

```cpp
rmt_rx_channel_config_t rx_cfg = {};
rx_cfg.gpio_num = input_pin;
rx_cfg.clk_src = RMT_CLK_SRC_DEFAULT;
rx_cfg.resolution_hz = 1000000;
rx_cfg.mem_block_symbols = 64;
```

#### `rmt_symbol_word_t`

**Loại:** union/struct bit-field 32-bit. **Dùng để:** chứa hai đoạn mức/thời gian

| Trường / giá trị | Kiểu | Ý nghĩa, giá trị và cách chọn |
| --- | --- | --- |
| duration0 / duration1 | unsigned 15-bit mỗi trường | Thời lượng theo tick RMT; mỗi trường tối đa 32767, chọn >0 cho đoạn xung thông thường. |
| level0 / level1 | unsigned 1-bit mỗi trường | Mức 0 hoặc 1 của từng đoạn. |
| val | uint32_t, union alias | Toàn bộ word; không cần thao tác trực tiếp khi đã gán các trường. |

**Cú pháp sử dụng:**

```cpp
rmt_symbol_word_t symbol = {};
symbol.level0 = 1; symbol.duration0 = 100;
symbol.level1 = 0; symbol.duration1 = 100;
```

**Lưu ý:** Ở resolution 1 MHz, ví dụ là HIGH 100 us rồi LOW 100 us.

#### `rmt_copy_encoder_config_t`

**Loại:** struct cấu hình rỗng/reserved. **Dùng để:** tạo encoder sao chép symbol đã đóng gói

**Cú pháp sử dụng:**

```cpp
rmt_copy_encoder_config_t enc_cfg = {};
```

**Lưu ý:** Đầu vào encoder này phải là rmt_symbol_word_t, không phải byte tùy ý. Không có trường cần tự điền ở SDK tham chiếu.

#### `rmt_bytes_encoder_config_t`

**Loại:** struct. **Dùng để:** đổi từng bit của chuỗi byte thành symbol

| Trường / giá trị | Kiểu | Ý nghĩa, giá trị và cách chọn |
| --- | --- | --- |
| bit0 | rmt_symbol_word_t | Waveform cho bit 0. |
| bit1 | rmt_symbol_word_t | Waveform cho bit 1. |
| flags.msb_first | uint32_t bit-field | true gửi bit cao trước; false bit thấp trước. |

**Cú pháp sử dụng:**

```cpp
rmt_bytes_encoder_config_t enc_cfg = {};
enc_cfg.bit0 = symbol_for_zero;
enc_cfg.bit1 = symbol_for_one;
enc_cfg.flags.msb_first = true;
```

#### `rmt_transmit_config_t`

**Loại:** struct. **Dùng để:** đặt cách chạy một payload TX

| Trường / giá trị | Kiểu | Ý nghĩa, giá trị và cách chọn |
| --- | --- | --- |
| loop_count | int | 0 không lặp; giá trị lặp hữu hạn hoặc -1 tùy hỗ trợ/hạn chế driver. |
| flags.eot_level | uint32_t bit-field | Mức output sau kết thúc. |
| flags.queue_nonblocking | uint32_t bit-field | Nếu hỗ trợ, yêu cầu trả ngay khi queue đầy thay vì block; không giống timeout chờ phát xong. |

**Cú pháp sử dụng:**

```cpp
rmt_transmit_config_t tx = {};
tx.loop_count = 0;
tx.flags.eot_level = 0;
```

#### `rmt_receive_config_t`

**Loại:** struct. **Dùng để:** lọc xung và phát hiện kết thúc lần nhận

| Trường / giá trị | Kiểu | Ý nghĩa, giá trị và cách chọn |
| --- | --- | --- |
| signal_range_min_ns | uint32_t | Xung ngắn hơn ngưỡng bị lọc; đơn vị ns. |
| signal_range_max_ns | uint32_t | Ngưỡng khoảng mức dài để kết thúc nhận; đơn vị ns, phải nằm trong khả năng phần cứng/resolution. |
| flags | bit-field theo SDK | Nhận partial nâng cao nếu SDK hỗ trợ; bỏ mặc định nếu chưa dùng. |

**Cú pháp sử dụng:**

```cpp
rmt_receive_config_t receive_cfg = {};
receive_cfg.signal_range_min_ns = 1000;
receive_cfg.signal_range_max_ns = 1000000;
```

**Lưu ý:** Đây là ngưỡng minh họa; chọn theo waveform thực tế. Không truyền tick hoặc micro giây vào trường *_ns.

#### `rmt_rx_event_callbacks_t`

**Loại:** struct chứa callback. **Dùng để:** đăng ký callback khi có dữ liệu nhận

| Trường / giá trị | Kiểu | Ý nghĩa, giá trị và cách chọn |
| --- | --- | --- |
| on_recv_done | rmt_rx_done_callback_t | bool callback(channel, event, user_data), trả true khi đánh thức task ưu tiên cao hơn. |

**Cú pháp sử dụng:**

```cpp
bool rx_done(rmt_channel_handle_t ch,
             const rmt_rx_done_event_data_t *event, void *ctx) {
    return false;
}
rmt_rx_event_callbacks_t cbs = {};
cbs.on_recv_done = rx_done;
```

#### `rmt_rx_done_event_data_t`

**Loại:** struct dữ liệu sự kiện. **Dùng để:** cung cấp symbol nhận được

| Trường / giá trị | Kiểu | Ý nghĩa, giá trị và cách chọn |
| --- | --- | --- |
| received_symbols | rmt_symbol_word_t * | Buffer symbol của lần nhận. |
| num_symbols | size_t | Số symbol hợp lệ. |
| flags.is_last | uint32_t bit-field | true nếu là phần cuối của lần nhận; quan trọng khi bật nhận partial. |

**Cú pháp sử dụng:**

```cpp
// Trong callback: xử lý event->num_symbols và buffer theo ownership.
```

**Lưu ý:** Callback chạy trong ISR; không giữ con trỏ event. Buffer không được bị tái sử dụng trước khi task đã xử lý/copy xong.

#### `rmt_channel_handle_t`

**Loại:** opaque handle con trỏ. **Dùng để:** tham chiếu channel/encoder đã tạo

**Cú pháp sử dụng:**

```cpp
rmt_channel_handle_t channel = nullptr;
```

#### `rmt_encoder_handle_t`

**Loại:** opaque handle con trỏ. **Dùng để:** tham chiếu channel/encoder đã tạo

**Cú pháp sử dụng:**

```cpp
rmt_encoder_handle_t encoder = nullptr;
```

#### `rmt_new_tx_channel`

Tạo channel tương ứng TX/RX.

```c
esp_err_t rmt_new_tx_channel(const rmt_tx_channel_config_t *config,
                              rmt_channel_handle_t *ret_chan);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| config | const rmt_tx_channel_config_t * / in | Địa chỉ cấu hình đúng loại TX/RX. |
| ret_chan | rmt_channel_handle_t * / out | &channel nhận handle. |

**Trả về:** `ESP_OK` nếu thành công; mã `esp_err_t` khác nếu thất bại. Chỉ dùng kết quả đầu ra sau khi kiểm tra thành công.

**Cách gọi:**

```cpp
rmt_new_tx_channel(&config, &channel);
```

#### `rmt_new_rx_channel`

Tạo channel tương ứng TX/RX.

```c
esp_err_t rmt_new_rx_channel(const rmt_rx_channel_config_t *config,
                              rmt_channel_handle_t *ret_chan);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| config | const rmt_rx_channel_config_t * / in | Địa chỉ cấu hình đúng loại TX/RX. |
| ret_chan | rmt_channel_handle_t * / out | &channel nhận handle. |

**Trả về:** `ESP_OK` nếu thành công; mã `esp_err_t` khác nếu thất bại. Chỉ dùng kết quả đầu ra sau khi kiểm tra thành công.

**Cách gọi:**

```cpp
rmt_new_rx_channel(&config, &channel);
```

#### `rmt_new_copy_encoder`

Tạo encoder đúng dạng payload.

```c
esp_err_t rmt_new_copy_encoder(const rmt_copy_encoder_config_t *config,
                                rmt_encoder_handle_t *ret_encoder);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| config | const rmt_copy_encoder_config_t * / in | Địa chỉ cấu hình encoder. |
| ret_encoder | rmt_encoder_handle_t * / out | &encoder nhận handle. |

**Trả về:** `ESP_OK` nếu thành công; mã `esp_err_t` khác nếu thất bại. Chỉ dùng kết quả đầu ra sau khi kiểm tra thành công.

**Cách gọi:**

```cpp
rmt_new_copy_encoder(&enc_cfg, &encoder);
```

#### `rmt_new_bytes_encoder`

Tạo encoder đúng dạng payload.

```c
esp_err_t rmt_new_bytes_encoder(const rmt_bytes_encoder_config_t *config,
                                rmt_encoder_handle_t *ret_encoder);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| config | const rmt_bytes_encoder_config_t * / in | Địa chỉ cấu hình encoder. |
| ret_encoder | rmt_encoder_handle_t * / out | &encoder nhận handle. |

**Trả về:** `ESP_OK` nếu thành công; mã `esp_err_t` khác nếu thất bại. Chỉ dùng kết quả đầu ra sau khi kiểm tra thành công.

**Cách gọi:**

```cpp
rmt_new_bytes_encoder(&enc_cfg, &encoder);
```

#### `rmt_enable`

Enable channel trước giao dịch.

```c
esp_err_t rmt_enable(rmt_channel_handle_t channel);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| channel | rmt_channel_handle_t / in | Handle channel TX hoặc RX. |

**Trả về:** `ESP_OK` nếu thành công; mã `esp_err_t` khác nếu thất bại. Chỉ dùng kết quả đầu ra sau khi kiểm tra thành công.

**Cách gọi:**

```cpp
rmt_enable(channel);
```

#### `rmt_disable`

Dừng/disable channel theo vòng đời.

```c
esp_err_t rmt_disable(rmt_channel_handle_t channel);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| channel | rmt_channel_handle_t / in | Handle channel TX hoặc RX. |

**Trả về:** `ESP_OK` nếu thành công; mã `esp_err_t` khác nếu thất bại. Chỉ dùng kết quả đầu ra sau khi kiểm tra thành công.

**Cách gọi:**

```cpp
rmt_disable(channel);
```

#### `rmt_del_channel`

Xóa channel đã disable, không còn giao dịch.

```c
esp_err_t rmt_del_channel(rmt_channel_handle_t channel);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| channel | rmt_channel_handle_t / in | Handle channel TX hoặc RX. |

**Trả về:** `ESP_OK` nếu thành công; mã `esp_err_t` khác nếu thất bại. Chỉ dùng kết quả đầu ra sau khi kiểm tra thành công.

**Cách gọi:**

```cpp
rmt_del_channel(channel);
```

#### `rmt_transmit`

Xếp payload cho channel phát.

```c
esp_err_t rmt_transmit(rmt_channel_handle_t tx_channel,
    rmt_encoder_handle_t encoder, const void *payload, size_t payload_bytes,
    const rmt_transmit_config_t *config);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| tx_channel | rmt_channel_handle_t / in | Channel TX đã enable. |
| encoder | rmt_encoder_handle_t / in | Encoder phù hợp payload. |
| payload | const void * / in | Địa chỉ dữ liệu byte/symbol theo encoder. |
| payload_bytes | size_t / in | Độ dài theo BYTE, ví dụ sizeof(symbols), không phải số symbol. |
| config | const rmt_transmit_config_t * / in | Cấu hình lần phát. |

**Trả về:** `ESP_OK` nếu thành công; mã `esp_err_t` khác nếu thất bại. Chỉ dùng kết quả đầu ra sau khi kiểm tra thành công.

**Cách gọi:**

```cpp
rmt_transmit(channel, encoder, &symbol, sizeof(symbol), &tx);
```

**Điều kiện / chú ý:** ESP_OK chưa nghĩa phát xong. Giữ payload sống, không sửa đến khi hoàn tất.

#### `rmt_tx_wait_all_done`

Chờ mọi lần phát đang chờ của channel hoàn tất.

```c
esp_err_t rmt_tx_wait_all_done(rmt_channel_handle_t tx_channel, int timeout_ms);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| tx_channel | rmt_channel_handle_t / in | Channel TX. |
| timeout_ms | int / in | Mili giây, -1 chờ vô hạn nếu dùng theo API; 1000 là 1 giây. |

**Trả về:** `ESP_OK` nếu thành công; mã `esp_err_t` khác nếu thất bại. Chỉ dùng kết quả đầu ra sau khi kiểm tra thành công.

**Cách gọi:**

```cpp
rmt_tx_wait_all_done(channel, 1000);
```

**Điều kiện / chú ý:** Nếu timeout, payload có thể vẫn được dùng; không giải phóng ngay.

#### `rmt_rx_register_event_callbacks`

Đăng ký callback RX trước enable.

```c
esp_err_t rmt_rx_register_event_callbacks(rmt_channel_handle_t channel,
    const rmt_rx_event_callbacks_t *cbs, void *user_data);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| channel | rmt_channel_handle_t / in | Channel RX. |
| cbs | const rmt_rx_event_callbacks_t * / in | Địa chỉ bảng callback. |
| user_data | void * / in | Context callback; lifetime đến khi không còn sự kiện. |

**Trả về:** `ESP_OK` nếu thành công; mã `esp_err_t` khác nếu thất bại. Chỉ dùng kết quả đầu ra sau khi kiểm tra thành công.

**Cách gọi:**

```cpp
rmt_rx_register_event_callbacks(channel, &cbs, context);
```

#### `rmt_receive`

Bắt đầu nhận vào buffer.

```c
esp_err_t rmt_receive(rmt_channel_handle_t rx_channel, void *buffer,
    size_t buffer_size, const rmt_receive_config_t *config);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| rx_channel | rmt_channel_handle_t / in | Channel RX đã enable. |
| buffer | void * / out, dùng bất đồng bộ | Vùng nhớ đủ lớn cho symbol nhận. |
| buffer_size | size_t / in | Dung lượng buffer theo byte. |
| config | const rmt_receive_config_t * / in | Địa chỉ cấu hình ngưỡng nhận. |

**Trả về:** `ESP_OK` nếu thành công; mã `esp_err_t` khác nếu thất bại. Chỉ dùng kết quả đầu ra sau khi kiểm tra thành công.

**Cách gọi:**

```cpp
rmt_symbol_word_t buffer[64];
rmt_receive(channel, buffer, sizeof(buffer), &receive_cfg);
```

**Điều kiện / chú ý:** Buffer phải sống đến khi callback báo hoàn tất và task xử lý xong. Không dùng biến stack của hàm đã return.

#### `rmt_del_encoder`

Hủy encoder không còn giao dịch sử dụng.

```c
esp_err_t rmt_del_encoder(rmt_encoder_handle_t encoder);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| encoder | rmt_encoder_handle_t / in | Encoder đã tạo. |

**Trả về:** `ESP_OK` nếu thành công; mã `esp_err_t` khác nếu thất bại. Chỉ dùng kết quả đầu ra sau khi kiểm tra thành công.

**Cách gọi:**

```cpp
rmt_del_encoder(encoder);
```

### 12.1 Ví dụ copy encoder phát symbol

```cpp
#include "driver/rmt_tx.h"
#include "driver/rmt_encoder.h"
#include "esp_err.h"

void rmt_pulse_example(gpio_num_t pin)
{
    rmt_channel_handle_t ch = NULL;
    rmt_tx_channel_config_t cc = {};
    cc.gpio_num = pin;
    cc.clk_src = RMT_CLK_SRC_DEFAULT;
    cc.resolution_hz = 1000000;
    cc.mem_block_symbols = 64;
    cc.trans_queue_depth = 1;
    ESP_ERROR_CHECK(rmt_new_tx_channel(&cc, &ch));

    rmt_encoder_handle_t enc = NULL;
    rmt_copy_encoder_config_t ec = {};
    ESP_ERROR_CHECK(rmt_new_copy_encoder(&ec, &enc));
    ESP_ERROR_CHECK(rmt_enable(ch));

    rmt_symbol_word_t symbol = {};
    symbol.level0 = 1;
    symbol.duration0 = 100;
    symbol.level1 = 0;
    symbol.duration1 = 100;
    rmt_transmit_config_t tx = {};
    tx.loop_count = 0;
    tx.flags.eot_level = 0;
    ESP_ERROR_CHECK(rmt_transmit(
        ch, enc, &symbol, sizeof(symbol), &tx));
    // Stack payload remains alive until completion.
    ESP_ERROR_CHECK(rmt_tx_wait_all_done(ch, 1000));

    ESP_ERROR_CHECK(rmt_disable(ch));
    ESP_ERROR_CHECK(rmt_del_channel(ch));
    ESP_ERROR_CHECK(rmt_del_encoder(enc));
}
```

**Nguồn:** [RMT](https://docs.espressif.com/projects/esp-idf/en/v6.1/esp32/api-reference/peripherals/rmt.html), [Ví dụ RMT chính hãng](https://github.com/espressif/esp-idf/blob/v6.1/examples/peripherals/rmt/ir_nec_transceiver/main/ir_nec_transceiver_main.c).

<a id="13-adc"></a>

## 13. ADC oneshot và calibration

**Header:** `esp_adc/adc_oneshot.h`, `esp_adc/adc_cali.h`, `esp_adc/adc_cali_scheme.h`. GPIO analog phải được ánh xạ sang ADC unit/channel theo target; số channel khác số GPIO.

#### `adc_oneshot_unit_init_cfg_t`

**Loại:** struct. **Dùng để:** tạo một ADC unit

| Trường / giá trị | Kiểu | Ý nghĩa, giá trị và cách chọn |
| --- | --- | --- |
| unit_id | adc_unit_t | ADC_UNIT_1 hoặc ADC_UNIT_2 theo target. |
| clk_src | adc_oneshot_clk_src_t | 0 dùng nguồn clock mặc định của driver. |
| ulp_mode | adc_ulp_mode_t | ADC_ULP_MODE_DISABLE nếu không dùng ULP. |

**Cú pháp sử dụng:**

```cpp
adc_oneshot_unit_init_cfg_t unit_cfg = {};
unit_cfg.unit_id = ADC_UNIT_1;
unit_cfg.ulp_mode = ADC_ULP_MODE_DISABLE;
```

#### `adc_oneshot_chan_cfg_t`

**Loại:** struct. **Dùng để:** đặt attenuation và số bit cho channel

| Trường / giá trị | Kiểu | Ý nghĩa, giá trị và cách chọn |
| --- | --- | --- |
| atten | adc_atten_t | Ví dụ ADC_ATTEN_DB_12; chọn theo dải điện áp và target. |
| bitwidth | adc_bitwidth_t | ADC_BITWIDTH_DEFAULT hoặc độ rộng được chip hỗ trợ. |

**Cú pháp sử dụng:**

```cpp
adc_oneshot_chan_cfg_t channel_cfg = {};
channel_cfg.atten = ADC_ATTEN_DB_12;
channel_cfg.bitwidth = ADC_BITWIDTH_DEFAULT;
```

**Lưu ý:** Attenuation không nâng giới hạn chịu áp tuyệt đối của chân GPIO.

#### `adc_cali_line_fitting_config_t`

**Loại:** struct. **Dùng để:** calibration line fitting cho ESP32 thường

| Trường / giá trị | Kiểu | Ý nghĩa, giá trị và cách chọn |
| --- | --- | --- |
| unit_id | adc_unit_t | ADC cùng unit đã đo. |
| atten | adc_atten_t | Phải khớp attenuation khi đọc. |
| bitwidth | adc_bitwidth_t | Phải khớp độ rộng ADC. |
| default_vref | uint32_t | Vref ước lượng theo mV nếu không có eFuse phù hợp; 0 khi dùng dữ liệu hiệu chuẩn sẵn và đã xử lý trường hợp thiếu. |

**Cú pháp sử dụng:**

```cpp
adc_cali_line_fitting_config_t cal_cfg = {};
cal_cfg.unit_id = ADC_UNIT_1;
cal_cfg.atten = ADC_ATTEN_DB_12;
cal_cfg.bitwidth = ADC_BITWIDTH_DEFAULT;
cal_cfg.default_vref = 0;
```

#### `adc_cali_curve_fitting_config_t`

**Loại:** struct. **Dùng để:** calibration curve fitting cho ESP32-S3 theo SDK

| Trường / giá trị | Kiểu | Ý nghĩa, giá trị và cách chọn |
| --- | --- | --- |
| unit_id | adc_unit_t | Unit cần hiệu chuẩn. |
| chan | adc_channel_t | Channel đang đo. |
| atten | adc_atten_t | Attenuation khớp phép đo. |
| bitwidth | adc_bitwidth_t | Bitwidth khớp phép đo. |

**Cú pháp sử dụng:**

```cpp
// Chỉ biên dịch nhánh này nếu target hỗ trợ curve fitting.
adc_cali_curve_fitting_config_t cal_cfg = {};
cal_cfg.unit_id = ADC_UNIT_1;
cal_cfg.chan = channel;
cal_cfg.atten = ADC_ATTEN_DB_12;
cal_cfg.bitwidth = ADC_BITWIDTH_DEFAULT;
```

#### `adc_oneshot_unit_handle_t`

**Loại:** opaque handle con trỏ. **Dùng để:** giữ ADC unit đã tạo

**Cú pháp sử dụng:**

```cpp
adc_oneshot_unit_handle_t adc = nullptr;
```

#### `adc_cali_handle_t`

**Loại:** opaque handle con trỏ. **Dùng để:** giữ scheme calibration đã tạo

**Cú pháp sử dụng:**

```cpp
adc_cali_handle_t cali = nullptr;
```

#### `adc_unit_t`

**Loại:** enum. **Dùng để:** chọn unit ADC

| Trường / giá trị | Kiểu | Ý nghĩa, giá trị và cách chọn |
| --- | --- | --- |
| ADC_UNIT_1, ADC_UNIT_2 | giá trị enum | Chọn theo khả năng target và cấu hình phép đo. |

**Cú pháp sử dụng:**

```cpp
// Gán vào trường tương ứng trong cấu hình ADC.
```

#### `adc_channel_t`

**Loại:** enum. **Dùng để:** chọn channel analog theo map GPIO

| Trường / giá trị | Kiểu | Ý nghĩa, giá trị và cách chọn |
| --- | --- | --- |
| ADC_CHANNEL_0, ADC_CHANNEL_1, ... | giá trị enum | Chọn theo khả năng target và cấu hình phép đo. |

**Cú pháp sử dụng:**

```cpp
// Gán vào trường tương ứng trong cấu hình ADC.
```

#### `adc_atten_t`

**Loại:** enum. **Dùng để:** chọn attenuation hỗ trợ

| Trường / giá trị | Kiểu | Ý nghĩa, giá trị và cách chọn |
| --- | --- | --- |
| ADC_ATTEN_DB_0, ADC_ATTEN_DB_2_5, ADC_ATTEN_DB_6, ADC_ATTEN_DB_12 | giá trị enum | Chọn theo khả năng target và cấu hình phép đo. |

**Cú pháp sử dụng:**

```cpp
// Gán vào trường tương ứng trong cấu hình ADC.
```

#### `adc_bitwidth_t`

**Loại:** enum. **Dùng để:** chọn độ rộng raw

| Trường / giá trị | Kiểu | Ý nghĩa, giá trị và cách chọn |
| --- | --- | --- |
| ADC_BITWIDTH_DEFAULT hoặc số bit theo target | giá trị enum | Chọn theo khả năng target và cấu hình phép đo. |

**Cú pháp sử dụng:**

```cpp
// Gán vào trường tương ứng trong cấu hình ADC.
```

#### `adc_oneshot_new_unit`

Tạo ADC unit.

```c
esp_err_t adc_oneshot_new_unit(const adc_oneshot_unit_init_cfg_t *init_config,
                               adc_oneshot_unit_handle_t *ret_unit);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| init_config | const adc_oneshot_unit_init_cfg_t * / in | Địa chỉ cấu hình unit. |
| ret_unit | adc_oneshot_unit_handle_t * / out | &adc nhận handle. |

**Trả về:** `ESP_OK` nếu thành công; mã `esp_err_t` khác nếu thất bại. Chỉ dùng kết quả đầu ra sau khi kiểm tra thành công.

**Cách gọi:**

```cpp
adc_oneshot_new_unit(&unit_cfg, &adc);
```

#### `adc_oneshot_config_channel`

Cấu hình channel của unit.

```c
esp_err_t adc_oneshot_config_channel(adc_oneshot_unit_handle_t handle,
    adc_channel_t channel, const adc_oneshot_chan_cfg_t *config);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| handle | adc_oneshot_unit_handle_t / in | Unit đã tạo. |
| channel | adc_channel_t / in | Channel đúng với GPIO. |
| config | const adc_oneshot_chan_cfg_t * / in | Attenuation và bitwidth. |

**Trả về:** `ESP_OK` nếu thành công; mã `esp_err_t` khác nếu thất bại. Chỉ dùng kết quả đầu ra sau khi kiểm tra thành công.

**Cách gọi:**

```cpp
adc_oneshot_config_channel(adc, channel, &channel_cfg);
```

#### `adc_oneshot_read`

Thực hiện một phép đo.

```c
esp_err_t adc_oneshot_read(adc_oneshot_unit_handle_t handle,
                            adc_channel_t channel, int *out_raw);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| handle | adc_oneshot_unit_handle_t / in | Unit ADC. |
| channel | adc_channel_t / in | Channel đã cấu hình. |
| out_raw | int * / out | Địa chỉ biến nhận raw; chưa phải điện áp. |

**Trả về:** `ESP_OK` nếu thành công; mã `esp_err_t` khác nếu thất bại. Chỉ dùng kết quả đầu ra sau khi kiểm tra thành công.

**Cách gọi:**

```cpp
int raw = 0;
esp_err_t err = adc_oneshot_read(adc, channel, &raw);
```

**Điều kiện / chú ý:** Không gọi từ ISR. ESP32 thường có hạn chế ADC2 khi dùng Wi-Fi; xử lý lỗi thay vì dùng raw cũ.

#### `adc_cali_check_scheme`

Lấy mask scheme calibration được target hỗ trợ.

```c
esp_err_t adc_cali_check_scheme(adc_cali_scheme_ver_t *scheme_mask);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| scheme_mask | adc_cali_scheme_ver_t * / out | Địa chỉ biến nhận mask để kiểm tra scheme. |

**Trả về:** `ESP_OK` nếu thành công; mã `esp_err_t` khác nếu thất bại. Chỉ dùng kết quả đầu ra sau khi kiểm tra thành công.

**Cách gọi:**

```cpp
adc_cali_scheme_ver_t schemes = {};
adc_cali_check_scheme(&schemes);
```

**Điều kiện / chú ý:** Có thể chọn compile-time bằng ADC_CALI_SCHEME_*_SUPPORTED nếu đã biết target.

#### `adc_cali_create_scheme_line_fitting`

Tạo scheme hiệu chuẩn đúng loại.

```c
esp_err_t adc_cali_create_scheme_line_fitting(
    const adc_cali_line_fitting_config_t *config, adc_cali_handle_t *ret_handle);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| config | const adc_cali_line_fitting_config_t * / in | Địa chỉ cấu hình đúng unit/atten/bitwidth. |
| ret_handle | adc_cali_handle_t * / out | &cali nhận handle. |

**Trả về:** `ESP_OK` nếu thành công; mã `esp_err_t` khác nếu thất bại. Chỉ dùng kết quả đầu ra sau khi kiểm tra thành công.

**Cách gọi:**

```cpp
adc_cali_create_scheme_line_fitting(&cal_cfg, &cali);
```

**Điều kiện / chú ý:** Chỉ dùng trong nhánh target hỗ trợ kiểu/scheme này.

#### `adc_cali_create_scheme_curve_fitting`

Tạo scheme hiệu chuẩn đúng loại.

```c
esp_err_t adc_cali_create_scheme_curve_fitting(
    const adc_cali_curve_fitting_config_t *config, adc_cali_handle_t *ret_handle);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| config | const adc_cali_curve_fitting_config_t * / in | Địa chỉ cấu hình đúng unit/atten/bitwidth. |
| ret_handle | adc_cali_handle_t * / out | &cali nhận handle. |

**Trả về:** `ESP_OK` nếu thành công; mã `esp_err_t` khác nếu thất bại. Chỉ dùng kết quả đầu ra sau khi kiểm tra thành công.

**Cách gọi:**

```cpp
adc_cali_create_scheme_curve_fitting(&cal_cfg, &cali);
```

**Điều kiện / chú ý:** Chỉ dùng trong nhánh target hỗ trợ kiểu/scheme này.

#### `adc_cali_raw_to_voltage`

Đổi raw sang điện áp đã hiệu chuẩn.

```c
esp_err_t adc_cali_raw_to_voltage(adc_cali_handle_t handle, int raw,
                                   int *voltage);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| handle | adc_cali_handle_t / in | Scheme đã tạo. |
| raw | int / in | Kết quả phép đọc thành công. |
| voltage | int * / out | Địa chỉ biến nhận điện áp mV tại chân ADC. |

**Trả về:** `ESP_OK` nếu thành công; mã `esp_err_t` khác nếu thất bại. Chỉ dùng kết quả đầu ra sau khi kiểm tra thành công.

**Cách gọi:**

```cpp
int mv = 0;
adc_cali_raw_to_voltage(cali, raw, &mv);
```

**Điều kiện / chú ý:** Không đồng nhất điện áp chân ADC với điện áp pin trước cầu chia.

#### `adc_cali_delete_scheme_line_fitting`

Xóa scheme calibration tương ứng.

```c
esp_err_t adc_cali_delete_scheme_line_fitting(adc_cali_handle_t handle);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| handle | adc_cali_handle_t / in | Handle đúng scheme đã tạo. |

**Trả về:** `ESP_OK` nếu thành công; mã `esp_err_t` khác nếu thất bại. Chỉ dùng kết quả đầu ra sau khi kiểm tra thành công.

**Cách gọi:**

```cpp
adc_cali_delete_scheme_line_fitting(cali);
```

#### `adc_cali_delete_scheme_curve_fitting`

Xóa scheme calibration tương ứng.

```c
esp_err_t adc_cali_delete_scheme_curve_fitting(adc_cali_handle_t handle);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| handle | adc_cali_handle_t / in | Handle đúng scheme đã tạo. |

**Trả về:** `ESP_OK` nếu thành công; mã `esp_err_t` khác nếu thất bại. Chỉ dùng kết quả đầu ra sau khi kiểm tra thành công.

**Cách gọi:**

```cpp
adc_cali_delete_scheme_curve_fitting(cali);
```

#### `adc_oneshot_del_unit`

Giải phóng ADC unit không còn đọc.

```c
esp_err_t adc_oneshot_del_unit(adc_oneshot_unit_handle_t handle);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| handle | adc_oneshot_unit_handle_t / in | Unit cần hủy. |

**Trả về:** `ESP_OK` nếu thành công; mã `esp_err_t` khác nếu thất bại. Chỉ dùng kết quả đầu ra sau khi kiểm tra thành công.

**Cách gọi:**

```cpp
adc_oneshot_del_unit(adc);
```

### 13.1 Ví dụ chọn scheme theo target

```cpp
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_err.h"

void adc_example_init(adc_channel_t channel,
                      adc_oneshot_unit_handle_t *adc,
                      adc_cali_handle_t *cali)
{
    adc_oneshot_unit_init_cfg_t uc = {};
    uc.unit_id = ADC_UNIT_1;
    uc.ulp_mode = ADC_ULP_MODE_DISABLE;
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&uc, adc));
    adc_oneshot_chan_cfg_t cc = {};
    cc.atten = ADC_ATTEN_DB_12;
    cc.bitwidth = ADC_BITWIDTH_DEFAULT;
    ESP_ERROR_CHECK(adc_oneshot_config_channel(
        *adc, channel, &cc));

#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    adc_cali_curve_fitting_config_t c = {};
    c.unit_id = ADC_UNIT_1;
    c.chan = channel;
    c.atten = cc.atten;
    c.bitwidth = cc.bitwidth;
    ESP_ERROR_CHECK(adc_cali_create_scheme_curve_fitting(
        &c, cali));
#elif ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    adc_cali_line_fitting_config_t c = {};
    c.unit_id = ADC_UNIT_1;
    c.atten = cc.atten;
    c.bitwidth = cc.bitwidth;
    c.default_vref = 0;
    ESP_ERROR_CHECK(adc_cali_create_scheme_line_fitting(
        &c, cali));
#else
#error "Select a calibration scheme supported by this target"
#endif
}

// After init, in a task:
// int raw, mv;
// ESP_ERROR_CHECK(adc_oneshot_read(adc, channel, &raw));
// ESP_ERROR_CHECK(adc_cali_raw_to_voltage(cali, raw, &mv));
```

Với Rtop từ nguồn xuống chân ADC, Rbottom từ chân xuống GND: `Vnguon_mV = mv * (Rtop + Rbottom) / Rbottom`. Dùng số thực hoặc số nguyên đủ rộng và tỉ số điện trở được xác định cho board. Không hiệu chuẩn chung bằng raw/4095 * 3.3 V.

**Nguồn:** [ADC oneshot](https://docs.espressif.com/projects/esp-idf/en/v6.1/esp32/api-reference/peripherals/adc/adc_oneshot.html), [ADC calibration](https://docs.espressif.com/projects/esp-idf/en/v6.1/esp32/api-reference/peripherals/adc/adc_calibration.html), [Ví dụ ADC chính hãng](https://github.com/espressif/esp-idf/blob/v6.1/examples/peripherals/adc/oneshot_read/main/oneshot_read_main.c), [ADC calibration riêng ESP32-S3](https://docs.espressif.com/projects/esp-idf/en/v6.1/esp32s3/api-reference/peripherals/adc/adc_calibration.html).

<a id="14-error"></a>

## 14. Log và xử lý lỗi

**Header:** `esp_log.h`, `esp_err.h`, `esp_check.h`; thêm `inttypes.h` cho PRId64. Cần tách dữ liệu đầu ra khỏi mã trạng thái để không nhầm byte hợp lệ với lỗi.

#### `esp_err_t`

**Loại:** typedef số nguyên. **Dùng để:** biểu diễn trạng thái thành công/lỗi

| Trường / giá trị | Kiểu | Ý nghĩa, giá trị và cách chọn |
| --- | --- | --- |
| ESP_OK | hằng trạng thái | Thành công. |
| ESP_ERR_INVALID_ARG | hằng lỗi | Đối số không hợp lệ. |
| ESP_ERR_INVALID_STATE | hằng lỗi | Tài nguyên ở trạng thái không phù hợp. |
| ESP_ERR_TIMEOUT | hằng lỗi | Hết thời gian chờ theo API. |
| ESP_ERR_NO_MEM | hằng lỗi | Không đủ bộ nhớ. |
| ESP_ERR_NOT_SUPPORTED | hằng lỗi | Tính năng/target không hỗ trợ. |

**Cú pháp sử dụng:**

```cpp
esp_err_t err = some_driver_call();
if (err != ESP_OK) {
    // Không dùng dữ liệu đầu ra của lời gọi bị lỗi.
}
```

**Lưu ý:** some_driver_call chỉ là tên minh họa, phải thay bằng API thực. Mỗi driver có thể có mã lỗi riêng; tra API cụ thể khi cần phân loại chính xác.

#### `esp_log_level_t`

**Loại:** enum. **Dùng để:** chọn mức log

| Trường / giá trị | Kiểu | Ý nghĩa, giá trị và cách chọn |
| --- | --- | --- |
| ESP_LOG_NONE | giá trị enum | Tắt |
| ESP_LOG_ERROR | giá trị enum | Lỗi |
| ESP_LOG_WARN | giá trị enum | Cảnh báo |
| ESP_LOG_INFO | giá trị enum | Thông tin |
| ESP_LOG_DEBUG | giá trị enum | Debug |
| ESP_LOG_VERBOSE | giá trị enum | Chi tiết nhất |

**Cú pháp sử dụng:**

```cpp
esp_log_level_t level = ESP_LOG_INFO;
```

#### `esp_err_to_name`

Lấy chuỗi tên mã lỗi.

```c
const char *esp_err_to_name(esp_err_t code);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| code | esp_err_t / in | Mã lỗi cần giải thích. |

**Trả về:** Con trỏ chuỗi do SDK quản lý; không free().

**Cách gọi:**

```cpp
const char *name = esp_err_to_name(err);
```

#### `esp_log_level_set`

Chọn mức log runtime theo tag.

```c
void esp_log_level_set(const char *tag, esp_log_level_t level);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| tag | const char * / in | Tên tag hoặc "*" cho mọi tag. |
| level | esp_log_level_t / in | Mức cần hiển thị. |

**Trả về:** void.

**Cách gọi:**

```cpp
esp_log_level_set("sensor", ESP_LOG_DEBUG);
```

**Điều kiện / chú ý:** Không bật lại log đã bị loại lúc compile do mức maximum/static configuration.

#### `ESP_LOGI / ESP_LOGW / ESP_LOGE / ESP_LOGD / ESP_LOGV`

Ghi log theo mức.

```c
// Macro dạng gọi:
ESP_LOGI(tag, format, ...);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| tag | const char * / in | Tên nhóm, ví dụ "sensor". |
| format | const char * / in | Format tương tự printf. |
| ... | đối số biến thiên / in | Các giá trị có kiểu khớp format; %zu cho size_t, PRId64 cho int64_t. |

**Trả về:** Không dùng như API trả esp_err_t.

**Cách gọi:**

```cpp
ESP_LOGI("sensor", "value=%d", (int)value);
ESP_LOGE("sensor", "read: %s", esp_err_to_name(err));
```

**Điều kiện / chú ý:** Log dày làm thay đổi timing. Không mặc định dùng macro log thường trong ISR/cache-off.

#### `ESP_ERROR_CHECK`

Kiểm tra biểu thức trả esp_err_t, abort nếu lỗi.

```c
// Macro dạng gọi:
ESP_ERROR_CHECK(expression);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| expression | biểu thức esp_err_t / in | Lời gọi hoặc mã lỗi cần kiểm tra. |

**Trả về:** Không return mã lỗi cho caller; tiếp tục khi ESP_OK, abort khi lỗi.

**Cách gọi:**

```cpp
ESP_ERROR_CHECK(gpio_set_level(GPIO_NUM_4, 1));
```

**Điều kiện / chú ý:** Dùng tiện lúc bring-up; cân nhắc khi ứng dụng cần retry/cleanup thay vì abort.

#### `ESP_RETURN_ON_ERROR`

Log và return mã lỗi ra khỏi hàm hiện tại.

```c
// Macro dạng gọi:
ESP_RETURN_ON_ERROR(expression, tag, format, ...);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| expression | esp_err_t / in | Lời gọi cần kiểm tra. |
| tag | const char * / in | Tag log. |
| format | const char * / in | Thông báo lỗi. |
| ... | đối số format / in | Các giá trị bổ sung nếu format có placeholder. |

**Trả về:** Return mã lỗi nếu expression khác ESP_OK; không return nếu thành công.

**Cách gọi:**

```cpp
ESP_RETURN_ON_ERROR(gpio_set_level(pin, 1),
                    "sensor", "GPIO failed");
```

**Điều kiện / chú ý:** Hàm bao phải trả esp_err_t. Return sớm không tự nhả tài nguyên/mutex.

#### `ESP_RETURN_ON_FALSE`

Kiểm tra điều kiện hợp lệ.

```c
// Macro dạng gọi:
ESP_RETURN_ON_FALSE(condition, error_code, tag, format, ...);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| condition | biểu thức logic / in | Điều kiện phải đúng để tiếp tục. |
| error_code | esp_err_t / in | Mã trả về khi false. |
| tag | const char * / in | Tag log. |
| format | const char * / in | Thông báo. |
| ... | đối số format / in | Các giá trị nếu cần. |

**Trả về:** Return error_code khi false; tiếp tục khi true.

**Cách gọi:**

```cpp
ESP_RETURN_ON_FALSE(buffer != nullptr, ESP_ERR_INVALID_ARG,
                    "sensor", "null buffer");
```

#### `ESP_GOTO_ON_ERROR`

Ghi lỗi vào ret và nhảy tới nhãn cleanup.

```c
// Macro dạng gọi:
ESP_GOTO_ON_ERROR(expression, label, tag, format, ...);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| expression | esp_err_t / in | Lời gọi cần kiểm tra. |
| label | tên nhãn / in | Ví dụ cleanup; không đặt trong dấu nháy. |
| tag | const char * / in | Tag log. |
| format | const char * / in | Thông báo. |
| ... | đối số format / in | Giá trị nếu có. |

**Trả về:** Khi lỗi gán biến cục bộ ret rồi goto label.

**Cách gọi:**

```cpp
// Cần có: esp_err_t ret = ESP_OK; và nhãn cleanup: hợp lệ.
ESP_GOTO_ON_ERROR(some_driver_call(), cleanup, "sensor", "failed");
```

**Điều kiện / chú ý:** C++ không cho goto nhảy qua khởi tạo biến có lifetime không hợp lệ; đặt khai báo/khối cleanup phù hợp.

### 14.1 Ví dụ hàm trả lỗi

```cpp
#include <stdint.h>
#include <inttypes.h>
#include "driver/spi_master.h"
#include "esp_check.h"
#include "esp_log.h"

static const char *TAG = "flow";

esp_err_t send_frame(spi_device_handle_t dev,
                     const uint8_t *buf, size_t n)
{
    ESP_RETURN_ON_FALSE(dev && buf && n > 0,
        ESP_ERR_INVALID_ARG, TAG, "invalid frame");
    spi_transaction_t t = {};
    t.tx_buffer = buf;
    t.length = n * 8;
    ESP_RETURN_ON_ERROR(spi_device_transmit(dev, &t),
                        TAG, "SPI transfer failed");
    return ESP_OK;
}

// ESP_LOGI(TAG, "dx=%d", (int)dx);
// ESP_LOGI(TAG, "time=%" PRId64, timestamp_us);
// ESP_LOGE(TAG, "read: %s", esp_err_to_name(err));
```

**Nguồn:** [Logging](https://docs.espressif.com/projects/esp-idf/en/v6.1/esp32/api-reference/system/log.html), [Error helpers](https://docs.espressif.com/projects/esp-idf/en/v6.1/esp32/api-reference/system/esp_err.html).

<a id="15-nvs"></a>

## 15. NVS

**Header:** `nvs_flash.h`, `nvs.h`. Lưu cấu hình ít thay đổi; không ghi từng mẫu cảm biến. App init NVS một lần trước khi mở namespace.

#### `nvs_handle_t`

**Loại:** typedef định danh handle dạng số nguyên. **Dùng để:** tham chiếu một namespace đã mở

**Cú pháp sử dụng:**

```cpp
nvs_handle_t handle;
```

**Lưu ý:** Khác handle con trỏ của SPI. Chỉ dùng sau nvs_open thành công; hủy bằng nvs_close, không free().

#### `nvs_open_mode_t`

**Loại:** enum. **Dùng để:** chọn quyền mở namespace

| Trường / giá trị | Kiểu | Ý nghĩa, giá trị và cách chọn |
| --- | --- | --- |
| NVS_READONLY | giá trị enum | Chỉ đọc; namespace/key chưa tồn tại có thể trả NOT_FOUND. |
| NVS_READWRITE | giá trị enum | Cho phép đọc/ghi, tạo namespace khi phù hợp. |

**Cú pháp sử dụng:**

```cpp
nvs_open_mode_t mode = NVS_READWRITE;
```

#### `nvs_flash_init`

Khởi tạo partition NVS mặc định.

```c
esp_err_t nvs_flash_init(void);
```

**Tham số:** không có; `void` trong danh sách tham số nghĩa là hàm không nhận đối số.

**Trả về:** `ESP_OK` nếu thành công; mã `esp_err_t` khác nếu thất bại. Chỉ dùng kết quả đầu ra sau khi kiểm tra thành công.

**Cách gọi:**

```cpp
esp_err_t err = nvs_flash_init();
```

**Điều kiện / chú ý:** NO_FREE_PAGES/NEW_VERSION_FOUND cần chính sách migration; không tự xóa calibration khi chưa quyết định.

#### `nvs_open`

Mở namespace.

```c
esp_err_t nvs_open(const char *namespace_name, nvs_open_mode_t open_mode,
                    nvs_handle_t *out_handle);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| namespace_name | const char * / in | Tên namespace, giới hạn theo NVS; chọn ngắn, ví dụ "calib". |
| open_mode | nvs_open_mode_t / in | READONLY hoặc READWRITE. |
| out_handle | nvs_handle_t * / out | Địa chỉ biến handle. |

**Trả về:** `ESP_OK` nếu thành công; mã `esp_err_t` khác nếu thất bại. Chỉ dùng kết quả đầu ra sau khi kiểm tra thành công.

**Cách gọi:**

```cpp
nvs_handle_t h;
esp_err_t err = nvs_open("calib", NVS_READWRITE, &h);
```

#### `nvs_set_u32`

Ghi giá trị uint32_t theo key.

```c
esp_err_t nvs_set_u32(nvs_handle_t handle, const char *key, uint32_t value);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| handle | nvs_handle_t / in | Namespace đã mở READWRITE. |
| key | const char * / in | Tên key ngắn, tối đa 15 ký tự theo SDK. |
| value | uint32_t / in | Số cần lưu. |

**Trả về:** `ESP_OK` nếu thành công; mã `esp_err_t` khác nếu thất bại. Chỉ dùng kết quả đầu ra sau khi kiểm tra thành công.

**Cách gọi:**

```cpp
nvs_set_u32(h, "schema", 1);
```

**Điều kiện / chú ý:** Cần commit trước khi coi thay đổi đã được lưu bền vững.

#### `nvs_get_u32`

Đọc uint32_t từ key.

```c
esp_err_t nvs_get_u32(nvs_handle_t handle, const char *key, uint32_t *out_value);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| handle | nvs_handle_t / in | Namespace đã mở. |
| key | const char * / in | Key có kiểu dữ liệu phù hợp. |
| out_value | uint32_t * / out | Địa chỉ biến nhận giá trị. |

**Trả về:** `ESP_OK` nếu thành công; mã `esp_err_t` khác nếu thất bại. Chỉ dùng kết quả đầu ra sau khi kiểm tra thành công.

**Cách gọi:**

```cpp
uint32_t schema = 0;
nvs_get_u32(h, "schema", &schema);
```

#### `nvs_set_blob`

Ghi blob byte.

```c
esp_err_t nvs_set_blob(nvs_handle_t handle, const char *key,
                        const void *value, size_t length);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| handle | nvs_handle_t / in | Namespace READWRITE. |
| key | const char * / in | Tên key. |
| value | const void * / in | Địa chỉ dữ liệu lưu. |
| length | size_t / in | Số byte lưu, thường sizeof(config). |

**Trả về:** `ESP_OK` nếu thành công; mã `esp_err_t` khác nếu thất bại. Chỉ dùng kết quả đầu ra sau khi kiểm tra thành công.

**Cách gọi:**

```cpp
nvs_set_blob(h, "config", &config, sizeof(config));
```

#### `nvs_get_blob`

Đọc blob hoặc hỏi kích thước.

```c
esp_err_t nvs_get_blob(nvs_handle_t handle, const char *key,
                        void *out_value, size_t *length);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| handle | nvs_handle_t / in | Namespace đã mở. |
| key | const char * / in | Tên key blob. |
| out_value | void * / out | Buffer nhận hoặc nullptr để hỏi kích thước. |
| length | size_t * / in/out | Trước gọi: dung lượng buffer. Sau gọi: kích thước cần/thực tế theo kết quả. Truyền &n. |

**Trả về:** `ESP_OK` nếu thành công; mã `esp_err_t` khác nếu thất bại. Chỉ dùng kết quả đầu ra sau khi kiểm tra thành công.

**Cách gọi:**

```cpp
size_t n = sizeof(config);
esp_err_t err = nvs_get_blob(h, "config", &config, &n);
// Chỉ chấp nhận nếu ESP_OK, đúng n, đúng schema và giá trị hợp lệ.
```

**Điều kiện / chú ý:** Không dùng blob chỉ vì đọc đủ byte; kiểm tra phiên bản cấu trúc, range và padding/serialization.

#### `nvs_commit`

Commit các thay đổi đã ghi.

```c
esp_err_t nvs_commit(nvs_handle_t handle);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| handle | nvs_handle_t / in | Namespace đã ghi. |

**Trả về:** `ESP_OK` nếu thành công; mã `esp_err_t` khác nếu thất bại. Chỉ dùng kết quả đầu ra sau khi kiểm tra thành công.

**Cách gọi:**

```cpp
esp_err_t err = nvs_commit(h);
```

#### `nvs_close`

Đóng handle namespace.

```c
void nvs_close(nvs_handle_t handle);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| handle | nvs_handle_t / in | Handle đã mở. |

**Trả về:** void.

**Cách gọi:**

```cpp
nvs_close(h);
```

**Điều kiện / chú ý:** close không thay commit.

### 15.1 Ví dụ lưu calibration

```cpp
#include <stdint.h>
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_err.h"

typedef struct {
    uint32_t schema;
    float scale[3];
} calibration_t;

esp_err_t save_calibration(const calibration_t *cfg)
{
    if (cfg == NULL) return ESP_ERR_INVALID_ARG;
    nvs_handle_t h;
    esp_err_t err = nvs_open("calib", NVS_READWRITE, &h);
    if (err != ESP_OK) return err;
    err = nvs_set_blob(h, "imu", cfg, sizeof(*cfg));
    if (err == ESP_OK) err = nvs_commit(h);
    nvs_close(h);
    return err;
}

// App: check nvs_flash_init() once before opening NVS.
// Load: n = sizeof(calibration_t); nvs_get_blob(..., &n);
// Accept only on ESP_OK + correct n/schema/range checks.
```

**Nguồn:** [NVS](https://docs.espressif.com/projects/esp-idf/en/v6.1/esp32/api-reference/storage/nvs_flash.html).

<a id="16-wdt"></a>

## 16. Watchdog và reset

**Header:** `esp_task_wdt.h`, `esp_system.h`. Task Watchdog có thể đã được Kconfig khởi tạo. Không init lại mù quáng hoặc ghi đè cấu hình toàn cục trong từng driver.

#### `esp_task_wdt_config_t`

**Loại:** struct. **Dùng để:** cấu hình TWDT toàn hệ thống

| Trường / giá trị | Kiểu | Ý nghĩa, giá trị và cách chọn |
| --- | --- | --- |
| timeout_ms | uint32_t | Timeout theo mili giây. |
| idle_core_mask | uint32_t | Bit i bật giám sát idle task core i; 0 không đăng ký idle task qua cấu hình này. |
| trigger_panic | bool | true kích hoạt panic khi timeout; việc reboot phụ thuộc cấu hình panic. |

**Cú pháp sử dụng:**

```cpp
esp_task_wdt_config_t cfg = {};
cfg.timeout_ms = 5000;
cfg.idle_core_mask = 0; // app phải chọn chính sách giám sát
cfg.trigger_panic = true;
```

#### `esp_reset_reason_t`

**Loại:** enum. **Dùng để:** mô tả nguyên nhân khởi động lại

| Trường / giá trị | Kiểu | Ý nghĩa, giá trị và cách chọn |
| --- | --- | --- |
| ESP_RST_POWERON | giá trị enum | Cấp nguồn. |
| ESP_RST_SW | giá trị enum | Reset phần mềm. |
| ESP_RST_PANIC | giá trị enum | Panic/exception. |
| ESP_RST_TASK_WDT / ESP_RST_INT_WDT | giá trị enum | Task/interrupt watchdog. |
| ESP_RST_BROWNOUT / ESP_RST_DEEPSLEEP | giá trị enum | Sụt áp / thức từ deep sleep. |

**Cú pháp sử dụng:**

```cpp
esp_reset_reason_t reason = esp_reset_reason();
```

**Lưu ý:** Danh sách này là các giá trị thường gặp; đầy đủ theo header đúng target.

#### `esp_task_wdt_init`

Khởi tạo TWDT khi chưa có, sau khi scheduler đã chạy.

```c
esp_err_t esp_task_wdt_init(const esp_task_wdt_config_t *config);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| config | const esp_task_wdt_config_t * / in | Địa chỉ cấu hình toàn cục. |

**Trả về:** `ESP_OK` nếu thành công; mã `esp_err_t` khác nếu thất bại. Chỉ dùng kết quả đầu ra sau khi kiểm tra thành công.

**Cách gọi:**

```cpp
esp_task_wdt_init(&cfg);
```

**Điều kiện / chú ý:** Được tầng app sở hữu; không gọi đồng thời từ nhiều task.

#### `esp_task_wdt_reconfigure`

Đổi cấu hình TWDT đã khởi tạo.

```c
esp_err_t esp_task_wdt_reconfigure(const esp_task_wdt_config_t *config);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| config | const esp_task_wdt_config_t * / in | Địa chỉ cấu hình toàn cục. |

**Trả về:** `ESP_OK` nếu thành công; mã `esp_err_t` khác nếu thất bại. Chỉ dùng kết quả đầu ra sau khi kiểm tra thành công.

**Cách gọi:**

```cpp
esp_task_wdt_reconfigure(&cfg);
```

**Điều kiện / chú ý:** Được tầng app sở hữu; không gọi đồng thời từ nhiều task.

#### `esp_task_wdt_add`

Đăng ký task để TWDT giám sát.

```c
esp_err_t esp_task_wdt_add(TaskHandle_t task_handle);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| task_handle | TaskHandle_t / in | Handle task; nullptr chỉ task đang gọi. |

**Trả về:** `ESP_OK` nếu thành công; mã `esp_err_t` khác nếu thất bại. Chỉ dùng kết quả đầu ra sau khi kiểm tra thành công.

**Cách gọi:**

```cpp
esp_task_wdt_add(nullptr);
```

#### `esp_task_wdt_status`

Kiểm tra task đã được giám sát.

```c
esp_err_t esp_task_wdt_status(TaskHandle_t task_handle);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| task_handle | TaskHandle_t / in | Handle task; nullptr chỉ task đang gọi. |

**Trả về:** `ESP_OK` đã đăng ký; `ESP_ERR_NOT_FOUND` chưa đăng ký; `ESP_ERR_INVALID_STATE` TWDT chưa khởi tạo.

**Cách gọi:**

```cpp
esp_task_wdt_status(nullptr);
```

#### `esp_task_wdt_delete`

Bỏ đăng ký task trước khi hủy task.

```c
esp_err_t esp_task_wdt_delete(TaskHandle_t task_handle);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| task_handle | TaskHandle_t / in | Handle task; nullptr chỉ task đang gọi. |

**Trả về:** `ESP_OK` nếu thành công; mã `esp_err_t` khác nếu thất bại. Chỉ dùng kết quả đầu ra sau khi kiểm tra thành công.

**Cách gọi:**

```cpp
esp_task_wdt_delete(nullptr);
```

#### `esp_task_wdt_reset`

Feed TWDT của chính task đang gọi.

```c
esp_err_t esp_task_wdt_reset(void);
```

**Tham số:** không có; `void` trong danh sách tham số nghĩa là hàm không nhận đối số.

**Trả về:** `ESP_OK` nếu thành công; mã `esp_err_t` khác nếu thất bại. Chỉ dùng kết quả đầu ra sau khi kiểm tra thành công.

**Cách gọi:**

```cpp
esp_task_wdt_reset();
```

**Điều kiện / chú ý:** Task phải đăng ký trước; feed khi đã hoàn tất tiến trình có ý nghĩa, không feed thay task khác đang treo.

#### `esp_reset_reason`

Lấy nguyên nhân reset gần nhất.

```c
esp_reset_reason_t esp_reset_reason(void);
```

**Tham số:** không có; `void` trong danh sách tham số nghĩa là hàm không nhận đối số.

**Trả về:** Một giá trị enum esp_reset_reason_t.

**Cách gọi:**

```cpp
esp_reset_reason_t reason = esp_reset_reason();
```

#### `esp_restart`

Yêu cầu khởi động lại chip.

```c
void esp_restart(void);
```

**Tham số:** không có; `void` trong danh sách tham số nghĩa là hàm không nhận đối số.

**Trả về:** Không trở lại luồng chạy bình thường.

**Cách gọi:**

```cpp
// Chỉ gọi khi tầng ứng dụng quyết định cần reboot.
esp_restart();
```

### 16.1 Ví dụ kiểm tra đăng ký task

```cpp
#include "esp_task_wdt.h"
#include "esp_system.h"
#include "esp_log.h"

// Preconditions: TWDT already initialized by app/Kconfig.
esp_err_t watch_current_task(void)
{
    esp_err_t err = esp_task_wdt_status(NULL);
    if (err == ESP_OK) return ESP_OK;
    if (err == ESP_ERR_NOT_FOUND)
        return esp_task_wdt_add(NULL);
    return err; // e.g. TWDT not initialized
}

// App-wide configuration, only when explicitly needed:
// esp_task_wdt_config_t cfg = {};
// cfg.timeout_ms = 5000;
// cfg.idle_core_mask = 0; // choose intended idle monitoring
// cfg.trigger_panic = true;
// Call init OR reconfigure according to existing state.
```

**Nguồn:** [Watchdogs](https://docs.espressif.com/projects/esp-idf/en/v6.1/esp32/api-reference/system/wdts.html), [Reset và system APIs](https://docs.espressif.com/projects/esp-idf/en/v6.1/esp32/api-reference/system/misc_system_api.html).

<a id="17-heap"></a>

## 17. Heap, DMA và lifetime buffer

**Header:** `esp_heap_caps.h`; `stdlib.h` cho free; `driver/spi_master.h` cho helper cấp buffer SPI. Số byte còn trống không cho biết có cấp được một block liên tục lớn hay không.

#### `multi_heap_info_t`

**Loại:** struct thống kê. **Dùng để:** nhận trạng thái các heap phù hợp capability

| Trường / giá trị | Kiểu | Ý nghĩa, giá trị và cách chọn |
| --- | --- | --- |
| total_free_bytes | size_t | Tổng byte trống. |
| total_allocated_bytes | size_t | Tổng byte đã cấp. |
| largest_free_block | size_t | Block trống lớn nhất. |
| minimum_free_bytes | size_t | Mức trống thấp nhất đã ghi nhận. |
| allocated_blocks / free_blocks / total_blocks | size_t mỗi trường | Số block theo nhóm thống kê. |

**Cú pháp sử dụng:**

```cpp
multi_heap_info_t info = {};
heap_caps_get_info(&info, MALLOC_CAP_INTERNAL);
```

`MALLOC_CAP_*` là hằng bitmask, không phải struct. `MALLOC_CAP_INTERNAL` yêu cầu RAM nội; `MALLOC_CAP_DMA` yêu cầu bộ nhớ DMA-capable theo target; `MALLOC_CAP_8BIT` yêu cầu truy cập byte. Ghép yêu cầu bằng OR bit. Căn chỉnh và khả năng DMA vẫn phải đúng driver/target.

#### `heap_caps_malloc`

Cấp vùng nhớ theo capability.

```c
void *heap_caps_malloc(size_t size, uint32_t caps);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| size | size_t / in | Số byte cần cấp. |
| caps | uint32_t bitmask / in | OR các yêu cầu MALLOC_CAP_*. |

**Trả về:** Con trỏ hoặc nullptr nếu không đủ bộ nhớ phù hợp.

**Cách gọi:**

```cpp
uint8_t *buffer = (uint8_t *)heap_caps_malloc(
    256, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
```

#### `heap_caps_free`

Giải phóng vùng nhớ đã cấp.

```c
void heap_caps_free(void *ptr);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| ptr | void * / in | Con trỏ vùng heap hợp lệ, không phải handle driver. |

**Trả về:** void.

**Cách gọi:**

```cpp
heap_caps_free(buffer);
```

**Điều kiện / chú ý:** Không free khi DMA/queue/callback còn sử dụng vùng nhớ.

#### `heap_caps_get_free_size`

Tổng byte còn trống.

```c
size_t heap_caps_get_free_size(uint32_t caps);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| caps | uint32_t bitmask / in | Chọn nhóm heap theo capability. |

**Trả về:** size_t số byte.

**Cách gọi:**

```cpp
size_t bytes = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
```

#### `heap_caps_get_largest_free_block`

Kích thước block trống lớn nhất.

```c
size_t heap_caps_get_largest_free_block(uint32_t caps);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| caps | uint32_t bitmask / in | Chọn nhóm heap theo capability. |

**Trả về:** size_t số byte.

**Cách gọi:**

```cpp
size_t bytes = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL);
```

#### `heap_caps_get_minimum_free_size`

Mức byte trống thấp nhất đã ghi nhận.

```c
size_t heap_caps_get_minimum_free_size(uint32_t caps);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| caps | uint32_t bitmask / in | Chọn nhóm heap theo capability. |

**Trả về:** size_t số byte.

**Cách gọi:**

```cpp
size_t bytes = heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL);
```

#### `heap_caps_get_info`

Lấy nhiều chỉ số heap.

```c
void heap_caps_get_info(multi_heap_info_t *info, uint32_t caps);
```

| Tham số | Kiểu / hướng | Cách truyền và ý nghĩa |
| --- | --- | --- |
| info | multi_heap_info_t * / out | Địa chỉ struct kết quả. |
| caps | uint32_t bitmask / in | Nhóm heap cần thống kê. |

**Trả về:** void.

**Cách gọi:**

```cpp
multi_heap_info_t info = {};
heap_caps_get_info(&info, MALLOC_CAP_INTERNAL);
```

Helper `spi_bus_dma_memory_alloc(host_id, size, extra_heap_caps)` được giải thích đầy đủ ở mục SPI. Vùng cấp bằng helper này được giải phóng bằng `free()` sau khi không còn transaction dùng nó.

### 17.1 Ví dụ cấp buffer và xem phân mảnh

```cpp
#include <stdint.h>
#include "esp_heap_caps.h"
#include "esp_log.h"

void heap_example(void)
{
    uint8_t *buf = (uint8_t *)heap_caps_malloc(
        256, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    if (buf == NULL) return;
    // Use only while no in-flight transaction owns buf.
    heap_caps_free(buf);

    size_t total = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
    size_t block = heap_caps_get_largest_free_block(
        MALLOC_CAP_INTERNAL);
    ESP_LOGI("heap", "free=%zu largest=%zu", total, block);
}
```

| Cơ chế | Buffer/descriptor cần sống đến lúc nào? |
| --- | --- |
| SPI transmit/polling đồng bộ | Đến khi hàm hoàn tất; khi trả lỗi vẫn cần hiểu trạng thái driver cụ thể. |
| SPI queue | Đến khi get_trans_result trả descriptor đã hoàn tất. |
| RMT transmit | Đến callback/chờ hoàn tất thành công, không chỉ đến khi transmit trả về. |
| RMT receive | Đến khi nhận hoàn tất và consumer đã xử lý/copy xong. |
| I2C bất đồng bộ | Theo callback hoàn tất; ví dụ đồng bộ không tự áp dụng cho mode này. |
| Queue chứa con trỏ | Queue chỉ copy địa chỉ; vùng nhớ đích vẫn do ứng dụng quản lý. |

**Nguồn:** [Heap allocation](https://docs.espressif.com/projects/esp-idf/en/v6.1/esp32/api-reference/system/mem_alloc.html), [Heap debugging](https://docs.espressif.com/projects/esp-idf/en/v6.1/esp32/api-reference/system/heap_debug.html), [Header SPI device và transaction](https://github.com/espressif/esp-idf/blob/v6.1/components/esp_driver_spi/include/driver/spi_master.h), [Header SPI bus](https://github.com/espressif/esp-idf/blob/v6.1/components/esp_driver_spi/include/driver/spi_common.h).

<a id="18-index"></a>

## 18. Tích hợp, lỗi thường gặp và chỉ mục

Tổ chức chung: board chọn chân và sở hữu bus → bus_port bọc read/write/delay/lock → driver IC xử lý giao thức → task quản lý lịch/timestamp → ứng dụng dùng dữ liệu. Một file header nên mô tả đầu vào/đầu ra, ownership, blocking, đồng bộ và mã lỗi.

| Khi tra một struct | Khi tra một hàm |
| --- | --- |
| Nó cấu hình bus, device, channel hay một transaction? | Hàm được gọi ở bước nào của vòng đời? |
| Mỗi trường nhận enum, số, con trỏ hay bitmask? | Mỗi tham số truyền giá trị hay địa chỉ &biến? |
| Giá trị 0 có hợp lệ hay cần -1/tốc độ/queue rõ ràng? | Đơn vị length/timeout là bit, byte, tick, ms hay us? |
| Context/callback/buffer phải sống bao lâu? | Khi API trả về đã xong chưa hay chỉ queue? |
| Những trường phụ thuộc version/target là gì? | Khi thất bại dữ liệu/handle còn được dùng không? |

| Hiện tượng | Kiểm tra trước |
| --- | --- |
| Đỏ private/public/constructor | File đang được phân tích/biên dịch như C? Giữ C++ trong .cpp, hoặc chuyển API về C. |
| Không tìm thấy header/kiểu SDK | Đúng framework, version, target, include và component dependency chưa? |
| Include is not used | Chẩn đoán editor về include chưa dùng; xem build log để biết lỗi biên dịch thật. |
| undefined reference | Thiếu implementation, tên/chữ ký không khớp, source chưa được build hoặc C linkage sai. |
| multiple definition | Hai bản .c/.cpp cùng định nghĩa symbol, hoặc định nghĩa biến/hàm trong header sai cách. |
| SPI toàn 0x00 / 0xFF | Nguồn, CS, MISO/MOSI/SCLK, mode, tốc độ, read bit, dummy và khoảng chờ. |
| I2C NACK / timeout | Địa chỉ 7-bit, pull-up, GPIO, nguồn, tốc độ và trạng thái IC. |
| UART mất frame / CRC sai | Baud/parity/đảo mức, overflow, parser giữ trạng thái và timeout. |
| PWM sai độ rộng | Đơn vị duty/count, resolution, frequency và action generator đã cấu hình chưa? |
| Mẫu lặp / dt bất thường | Nhịp Data Ready, FIFO, task trễ, notification gộp, tuổi dữ liệu. |
| Crash sau khi hàm trả về | Buffer/context trên stack còn bị DMA/queue/callback sử dụng? |
| Watchdog / hết bộ nhớ | Task không yield, deadlock, priority, leak, phân mảnh và log quá nhiều. |

### 18.1 Chỉ mục kiểu được giải thích

`gpio_num_t`, `gpio_config_t`, `gpio_isr_t`, `spi_host_device_t`, `spi_dma_chan_t`, `spi_bus_config_t`, `spi_device_interface_config_t`, `spi_device_handle_t`, `spi_transaction_t`, `spi_transaction_ext_t`, `transaction_cb_t`, `i2c_master_bus_config_t`, `i2c_device_config_t`, `i2c_master_bus_handle_t`, `i2c_master_dev_handle_t`, `uart_port_t`, `uart_config_t`, `uart_event_t`, `esp_timer_create_args_t`, `esp_timer_handle_t`, `esp_timer_cb_t`, `gptimer_config_t`, `gptimer_alarm_config_t`, `gptimer_event_callbacks_t`, `gptimer_alarm_event_data_t`, `gptimer_handle_t`, `BaseType_t`, `UBaseType_t`, `TickType_t`, `TaskHandle_t`, `QueueHandle_t`, `SemaphoreHandle_t`, `TaskFunction_t`, `portMUX_TYPE`, `ledc_timer_config_t`, `ledc_channel_config_t`, `ledc_mode_t`, `ledc_timer_t`, `ledc_channel_t`, `ledc_timer_bit_t`, `mcpwm_timer_config_t`, `mcpwm_operator_config_t`, `mcpwm_comparator_config_t`, `mcpwm_generator_config_t`, `mcpwm_timer_handle_t`, `mcpwm_oper_handle_t`, `mcpwm_cmpr_handle_t`, `mcpwm_gen_handle_t`, `mcpwm_gen_timer_event_action_t`, `mcpwm_gen_compare_event_action_t`, `rmt_tx_channel_config_t`, `rmt_rx_channel_config_t`, `rmt_symbol_word_t`, `rmt_copy_encoder_config_t`, `rmt_bytes_encoder_config_t`, `rmt_transmit_config_t`, `rmt_receive_config_t`, `rmt_rx_event_callbacks_t`, `rmt_rx_done_event_data_t`, `rmt_channel_handle_t`, `rmt_encoder_handle_t`, `adc_oneshot_unit_init_cfg_t`, `adc_oneshot_chan_cfg_t`, `adc_cali_line_fitting_config_t`, `adc_cali_curve_fitting_config_t`, `adc_oneshot_unit_handle_t`, `adc_cali_handle_t`, `adc_unit_t`, `adc_channel_t`, `adc_atten_t`, `adc_bitwidth_t`, `esp_err_t`, `esp_log_level_t`, `nvs_handle_t`, `nvs_open_mode_t`, `esp_task_wdt_config_t`, `esp_reset_reason_t`, `multi_heap_info_t`.

### 18.2 Chỉ mục hàm/macro được giải thích

`gpio_config`, `gpio_set_direction`, `gpio_set_level`, `gpio_get_level`, `gpio_reset_pin`, `gpio_set_intr_type`, `gpio_install_isr_service`, `gpio_isr_handler_add`, `gpio_intr_enable`, `gpio_intr_disable`, `gpio_isr_handler_remove`, `spi_bus_initialize`, `spi_bus_add_device`, `spi_device_transmit`, `spi_device_polling_transmit`, `spi_device_queue_trans`, `spi_device_get_trans_result`, `spi_device_acquire_bus`, `spi_device_release_bus`, `spi_bus_remove_device`, `spi_bus_free`, `spi_bus_dma_memory_alloc`, `i2c_new_master_bus`, `i2c_master_bus_add_device`, `i2c_master_probe`, `i2c_master_transmit`, `i2c_master_receive`, `i2c_master_transmit_receive`, `i2c_master_bus_reset`, `i2c_master_bus_rm_device`, `i2c_del_master_bus`, `uart_param_config`, `uart_set_pin`, `uart_driver_install`, `uart_write_bytes`, `uart_read_bytes`, `uart_get_buffered_data_len`, `uart_wait_tx_done`, `uart_flush_input`, `uart_driver_delete`, `uart_set_line_inverse`, `esp_timer_get_time`, `esp_timer_create`, `esp_timer_start_once`, `esp_timer_start_periodic`, `esp_timer_stop`, `esp_timer_delete`, `esp_rom_delay_us`, `gptimer_new_timer`, `gptimer_register_event_callbacks`, `gptimer_set_alarm_action`, `gptimer_enable`, `gptimer_start`, `gptimer_stop`, `gptimer_disable`, `gptimer_del_timer`, `gptimer_get_raw_count`, `gptimer_set_raw_count`, `xTaskCreate`, `xTaskCreatePinnedToCore`, `vTaskDelay`, `xTaskGetTickCount`, `xTaskDelayUntil`, `vTaskDelete`, `xTaskNotifyGive`, `ulTaskNotifyTake`, `vTaskNotifyGiveFromISR`, `xQueueCreate`, `xQueueSend`, `xQueueReceive`, `xQueueOverwrite`, `vQueueDelete`, `xSemaphoreCreateMutex`, `xSemaphoreTake`, `xSemaphoreGive`, `vSemaphoreDelete`, `ledc_timer_config`, `ledc_channel_config`, `ledc_set_duty_and_update`, `ledc_stop`, `mcpwm_new_timer`, `mcpwm_new_operator`, `mcpwm_operator_connect_timer`, `mcpwm_new_comparator`, `mcpwm_new_generator`, `mcpwm_generator_set_action_on_timer_event`, `mcpwm_generator_set_action_on_compare_event`, `mcpwm_comparator_set_compare_value`, `mcpwm_timer_enable`, `mcpwm_timer_disable`, `mcpwm_timer_start_stop`, `mcpwm_del_generator`, `mcpwm_del_comparator`, `mcpwm_del_operator`, `mcpwm_del_timer`, `rmt_new_tx_channel`, `rmt_new_rx_channel`, `rmt_new_copy_encoder`, `rmt_new_bytes_encoder`, `rmt_enable`, `rmt_disable`, `rmt_del_channel`, `rmt_transmit`, `rmt_tx_wait_all_done`, `rmt_rx_register_event_callbacks`, `rmt_receive`, `rmt_del_encoder`, `adc_oneshot_new_unit`, `adc_oneshot_config_channel`, `adc_oneshot_read`, `adc_cali_check_scheme`, `adc_cali_create_scheme_line_fitting`, `adc_cali_create_scheme_curve_fitting`, `adc_cali_raw_to_voltage`, `adc_cali_delete_scheme_line_fitting`, `adc_cali_delete_scheme_curve_fitting`, `adc_oneshot_del_unit`, `esp_err_to_name`, `esp_log_level_set`, `ESP_LOGI / ESP_LOGW / ESP_LOGE / ESP_LOGD / ESP_LOGV`, `ESP_ERROR_CHECK`, `ESP_RETURN_ON_ERROR`, `ESP_RETURN_ON_FALSE`, `ESP_GOTO_ON_ERROR`, `nvs_flash_init`, `nvs_open`, `nvs_set_u32`, `nvs_get_u32`, `nvs_set_blob`, `nvs_get_blob`, `nvs_commit`, `nvs_close`, `esp_task_wdt_init`, `esp_task_wdt_reconfigure`, `esp_task_wdt_add`, `esp_task_wdt_status`, `esp_task_wdt_delete`, `esp_task_wdt_reset`, `esp_reset_reason`, `esp_restart`, `heap_caps_malloc`, `heap_caps_free`, `heap_caps_get_free_size`, `heap_caps_get_largest_free_block`, `heap_caps_get_minimum_free_size`, `heap_caps_get_info`.

### 18.3 Nguồn chính thức

Các liên kết được ghim v6.1. Các phần ADC riêng S3 dùng target esp32s3; các nguồn còn lại mặc định ESP32. Mỗi phần có liên kết tại chỗ để kiểm tra đầy đủ trường nâng cao và mã lỗi theo SDK.

- [S01] [Build system và component](https://docs.espressif.com/projects/esp-idf/en/v6.1/esp32/api-guides/build-system.html)

- [S02] [Migration 6.0: peripheral drivers](https://docs.espressif.com/projects/esp-idf/en/v6.1/esp32/migration-guides/release-6.x/6.0/peripherals.html)

- [S03] [GPIO & RTC GPIO](https://docs.espressif.com/projects/esp-idf/en/v6.1/esp32/api-reference/peripherals/gpio.html)

- [S04] [SPI master](https://docs.espressif.com/projects/esp-idf/en/v6.1/esp32/api-reference/peripherals/spi_master.html)

- [S05] [I2C master](https://docs.espressif.com/projects/esp-idf/en/v6.1/esp32/api-reference/peripherals/i2c.html)

- [S06] [UART](https://docs.espressif.com/projects/esp-idf/en/v6.1/esp32/api-reference/peripherals/uart.html)

- [S07] [ESP Timer](https://docs.espressif.com/projects/esp-idf/en/v6.1/esp32/api-reference/system/esp_timer.html)

- [S08] [GPTimer](https://docs.espressif.com/projects/esp-idf/en/v6.1/esp32/api-reference/peripherals/gptimer.html)

- [S09] [FreeRTOS trong ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/v6.1/esp32/api-reference/system/freertos_idf.html)

- [S10] [LEDC](https://docs.espressif.com/projects/esp-idf/en/v6.1/esp32/api-reference/peripherals/ledc.html)

- [S11] [MCPWM](https://docs.espressif.com/projects/esp-idf/en/v6.1/esp32/api-reference/peripherals/mcpwm.html)

- [S12] [RMT](https://docs.espressif.com/projects/esp-idf/en/v6.1/esp32/api-reference/peripherals/rmt.html)

- [S13] [ADC oneshot](https://docs.espressif.com/projects/esp-idf/en/v6.1/esp32/api-reference/peripherals/adc/adc_oneshot.html)

- [S14] [ADC calibration](https://docs.espressif.com/projects/esp-idf/en/v6.1/esp32/api-reference/peripherals/adc/adc_calibration.html)

- [S15] [Logging](https://docs.espressif.com/projects/esp-idf/en/v6.1/esp32/api-reference/system/log.html)

- [S16] [Error helpers](https://docs.espressif.com/projects/esp-idf/en/v6.1/esp32/api-reference/system/esp_err.html)

- [S17] [NVS](https://docs.espressif.com/projects/esp-idf/en/v6.1/esp32/api-reference/storage/nvs_flash.html)

- [S18] [Watchdogs](https://docs.espressif.com/projects/esp-idf/en/v6.1/esp32/api-reference/system/wdts.html)

- [S19] [Reset và system APIs](https://docs.espressif.com/projects/esp-idf/en/v6.1/esp32/api-reference/system/misc_system_api.html)

- [S20] [Heap allocation](https://docs.espressif.com/projects/esp-idf/en/v6.1/esp32/api-reference/system/mem_alloc.html)

- [S21] [Heap debugging](https://docs.espressif.com/projects/esp-idf/en/v6.1/esp32/api-reference/system/heap_debug.html)

- [S22] [ROM API: internal / unstable](https://docs.espressif.com/projects/esp-idf/en/v6.1/esp32/api-reference/system/internal-unstable.html)

- [S23] [Header SPI device và transaction](https://github.com/espressif/esp-idf/blob/v6.1/components/esp_driver_spi/include/driver/spi_master.h)

- [S24] [Header SPI bus](https://github.com/espressif/esp-idf/blob/v6.1/components/esp_driver_spi/include/driver/spi_common.h)

- [S25] [Header I2C master](https://github.com/espressif/esp-idf/blob/v6.1/components/esp_driver_i2c/include/driver/i2c_master.h)

- [S26] [Header UART](https://github.com/espressif/esp-idf/blob/v6.1/components/esp_driver_uart/include/driver/uart.h)

- [S27] [Header GPTimer](https://github.com/espressif/esp-idf/blob/v6.1/components/esp_driver_gptimer/include/driver/gptimer.h)

- [S28] [Ví dụ ADC chính hãng](https://github.com/espressif/esp-idf/blob/v6.1/examples/peripherals/adc/oneshot_read/main/oneshot_read_main.c)

- [S29] [Ví dụ RMT chính hãng](https://github.com/espressif/esp-idf/blob/v6.1/examples/peripherals/rmt/ir_nec_transceiver/main/ir_nec_transceiver_main.c)

- [S30] [ADC calibration riêng ESP32-S3](https://docs.espressif.com/projects/esp-idf/en/v6.1/esp32s3/api-reference/peripherals/adc/adc_calibration.html)

### 18.4 Phạm vi kiểm tra và cách dùng ví dụ

Đã đối chiếu kiểu/nhóm API với tài liệu chính thức; ví dụ chưa được build với SDK dự án và chưa chạy trên board. Các cách gọi một dòng là minh họa để hiểu tham số; trong code thực phải lưu/kiểm tra kết quả. Tên như `config`, `context`, `reg`, `sample` là biến của người dùng và phải khai báo theo phần tương ứng; không phải biến toàn cục có sẵn trong SDK.

Không tự chép chữ ký hàm ở đây vào implementation rồi viết lại API SDK. Include header chính thức và gọi API. Phần cấu hình dạng bảng là hướng dẫn lựa chọn, không thay datasheet phần cứng hoặc toàn bộ tài liệu peripheral nâng cao.
