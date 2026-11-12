

#pragma once

#include <stdint.h>
#include <stddef.h>
#include "esp_err.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"

//=====REGISTER MAP=====//

namespace bmi270_reg {
    constexpr uint8_t CHIP_ID         = 0x00;   // expect 0x24
    constexpr uint8_t ERR_REG         = 0x02;
    constexpr uint8_t STATUS          = 0x03;   // bit7 drdy_acc, bit6 drdy_gyr
    constexpr uint8_t ACC_X_LSB       = 0x0C;   // 0x0C..0x11 accel, 0x12..0x17 gyro
    constexpr uint8_t GYR_X_LSB       = 0x12;
    constexpr uint8_t SENSORTIME_0    = 0x18;   // 24-bit, 39.0625 us/LSB
    constexpr uint8_t INTERNAL_STATUS = 0x21;   // message[3:0] == 0x01 -> init_ok
    constexpr uint8_t TEMPERATURE_0   = 0x22;
    constexpr uint8_t GYR_CAS         = 0x3C;   // factor_zx, 7-bit signed
    constexpr uint8_t ACC_CONF        = 0x40;
    constexpr uint8_t ACC_RANGE       = 0x41;
    constexpr uint8_t GYR_CONF        = 0x42;
    constexpr uint8_t GYR_RANGE       = 0x43;
    constexpr uint8_t INT1_IO_CTRL    = 0x53;
    constexpr uint8_t INT_MAP_DATA    = 0x58;
    constexpr uint8_t INIT_CTRL       = 0x59;
    constexpr uint8_t INIT_ADDR_0     = 0x5B;
    constexpr uint8_t INIT_ADDR_1     = 0x5C;
    constexpr uint8_t INIT_DATA       = 0x5E;
    constexpr uint8_t NV_CONF         = 0x70;
    constexpr uint8_t OFFSET_0        = 0x71;   // 0x71..0x77 accel/gyro offset
    constexpr uint8_t PWR_CONF        = 0x7C;   // bit0 adv_power_save, bit2 fup_en
    constexpr uint8_t PWR_CTRL        = 0x7D;   // bit1 gyr_en, bit2 acc_en, bit3 temp_en
    constexpr uint8_t CMD             = 0x7E;   // 0xB6 soft reset
}

enum class AccRange : uint8_t { 
    G2 = 0x00,
    G4 = 0x01,
    G8 = 0x02,
    G16 = 0x03
};

enum class GyrRange : uint8_t {
    DPS2000 = 0x00,
    DPS1000 = 0x01,
    DPS500  = 0x02,
    DPS250  = 0x03,
    DPS125  = 0x04
};

enum class Odr : uint8_t {                      // Lưu ý: accel tối đa 1600 Hz, gyro tối đa 3200 Hz
    HZ25   = 0x06,
    HZ50   = 0x07,
    HZ100  = 0x08,
    HZ200  = 0x09,
    HZ400  = 0x0A,
    HZ800  = 0x0B,
    HZ1600 = 0x0C,
    HZ3200 = 0x0D
};

struct Bmi270Raw {
    int16_t acc[3];
    int16_t gyr[3];
    uint32_t sensortime;                        // 24-bit, dùng để timestamp / đo jitter
};

struct Bmi270Data {
    float acc[3];                               // m/s^2
    float gyr[3];                               // rad/s
};

class BMI270 {
    private:
        // ----- Phần cứng -----
        gpio_num_t          m_csPin;
        spi_device_handle_t m_dev = nullptr;
        bool                m_initialized = false;

        // ----- Trạng thái cấu hình -----
        float  m_acc_scale = 0.0f;        // LSB -> m/s^2
        float  m_gyr_scale = 0.0f;        // LSB -> rad/s
        int8_t m_cas_factor_zx = 0;

        // Bosch nạp config theo khúc; chọn <= max_transfer_sz của bus trừ 1 byte địa chỉ
        static constexpr size_t CONFIG_CHUNK = 128;

        // ----- SPI mức thấp -----
        esp_err_t init_CS_GPIO();
        esp_err_t registerWrite(uint8_t reg, uint8_t value);               // reg & 0x7F
        esp_err_t registerRead(uint8_t reg, uint8_t *value);               // reg | 0x80, bỏ dummy
        esp_err_t burstRead(uint8_t reg, uint8_t *data, size_t len);       // đọc len+1, bỏ byte đầu
        esp_err_t burstWrite(uint8_t reg, const uint8_t *data, size_t len);
        esp_err_t modifyRegister(uint8_t reg, uint8_t setBits, uint8_t clearBits);

        // ----- Trình tự khởi tạo (theo bmi2_init / bmi270_init của Bosch) -----
        esp_err_t enableSPI();                                                          // dummy read CHIP_ID + chờ ~200 us
        esp_err_t checkChipID();
        esp_err_t softReset();                                                          // CMD=0xB6, chờ >= 2 ms, enableSPI() lại
        esp_err_t loadConfigFile();                                                     // PWR_CONF=0 -> 450us -> INIT_CTRL=0 -> chunks -> INIT_CTRL=1
        esp_err_t uploadConfigChunk(uint16_t offset, const uint8_t *data, size_t len);  // INIT_ADDR + INIT_DATA
        esp_err_t waitInitOk(uint32_t timeout_ms);                                      // poll INTERNAL_STATUS, tối đa ~20 ms theo Bosch
        esp_err_t configureDefault();                                                   // PWR_CTRL, ACC/GYR_CONF, range, tắt adv_power_save
        esp_err_t readCASFactor();                                                      // GYR_CAS -> sign-extend 7 bit

        // ----- Xử lý dữ liệu -----
        void applyGyroCAS(int16_t gyr[3]) const;

    public:
        explicit BMI270(gpio_num_t csPin);
        ~BMI270();                                           // spi_bus_remove_device

        // Bus phải được spi_bus_initialize ở ngoài (dùng chung với PMW3901)
        bool begin(spi_host_device_t host, int clock_hz = 8 * 1000 * 1000);

        // ----- Cấu hình -----
        esp_err_t setAccelConfig(Odr odr, AccRange range);   // set luôn acc_filter_perf
        esp_err_t setGyroConfig(Odr odr, GyrRange range);    // set filter_perf + noise_perf
        esp_err_t enableDataReadyInt1();                     // INT1 push-pull, map drdy -> INT1

        // ----- Đọc dữ liệu (gọi trong task IMU 500–1000 Hz) -----
        esp_err_t readRaw(Bmi270Raw &out);                   // 1 burst 0x0C..0x1A (15 byte)
        esp_err_t read(Bmi270Data &out);                     // raw + CAS + scale
        esp_err_t readTemperature(float &temp_c);
        esp_err_t isDataReady(bool &acc_ready, bool &gyr_ready);

        // ----- Hiệu chuẩn (Bosch: FOC / CRT) -----
        esp_err_t gyroFastOffsetCalib();                     // drone đứng yên trên mặt đất
        esp_err_t setGyroOffset(const int16_t offset[3]);    // ghi vào OFFSET 0x74..0x77

        // ----- Chẩn đoán -----
        esp_err_t readErrorRegister(uint8_t &err);
        esp_err_t readInternalStatus(uint8_t &status);
        bool      isInitialized() const { return m_initialized; }
};