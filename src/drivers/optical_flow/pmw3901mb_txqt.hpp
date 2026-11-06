

#pragma once

#include <stdint.h>
#include "esp_err.h"
#include "driver/spi_master.h"

struct PMW3901 {
    private:
        uint8_t m_csPin;
        esp_err_t registerWrite(uint8_t reg, uint8_t value);
        uint8_t registerRead(uint8_t reg);
        void initRegister();

        // Tham chiếu thiết bị đăng ký
        spi_device_handle_t m_dev = nullptr;

    public:
        // Lưu chân cs
        PMW3901(uint8_t csPin);
        // Check khởi tạo SPI
        bool init_SPI_PMW3901();
        // Đọc chuyển động
        // do chuyển động theo cả chiều âm và dương
        void readMotionCount(int16_t *deltaX, int16_t *deltaY); 
};