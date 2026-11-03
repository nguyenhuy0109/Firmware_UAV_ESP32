

#pragma once

#include <stdint.h>

struct PMW3901 {
    private:
        uint8_t m_csPin;
        void registerWrite(uint8_t reg, uint8_t value);
        uint8_t registerRead(uint8_t reg);
        void initRegister();

    public:
        // Lưu chân cs
        PMW3901(uint8_t csPin);
        // Check lỗi bắt đầu
        bool begin_PMW3901();
        // Đọc chuyển động
        // do chuyển động theo cả chiều âm và dương
        void readMotionCount(int16_t *deltaX, int16_t *deltaY); 
};