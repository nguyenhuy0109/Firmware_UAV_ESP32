

#pragma once

#include <stdint.h>
#include "esp_err.h"
#include "driver/spi_master.h"

struct PMW3901 {
    private:
        uint8_t m_csPin;
        bool m_cs_initialized = false;
        esp_err_t init_CS_GPIO();                                   // hàm set chân
        esp_err_t registerWrite(uint8_t reg, uint8_t value);        // hàm ghi 
        esp_err_t registerRead(uint8_t reg, uint8_t *value);        // hàm đọc 
        esp_err_t initRegister();                                   // hàm khởi tạo tối ưu
        spi_device_handle_t m_dev = nullptr;                        // tham chiếu thiết bị đăng ký

    public:
        PMW3901(uint8_t csPin);                                     // lưu chân cs
        bool init_SPI_PMW3901();                                    // check khởi tạo SPI
        esp_err_t readMotionCount(int16_t *deltaX, int16_t *deltaY);     // đọc chuyển động, cả chiều âm, dương
        bool begin();                                               // hàm check khởi tạo
};