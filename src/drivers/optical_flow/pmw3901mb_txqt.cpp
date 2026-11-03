

#include "pmw3901mb_txqt.hpp"

#include <driver/spi_master.h>

#define PRODUCT_ID 0x49
#define REVISION_ID 0X00

// Lưu số chân chọn cảm biến
PMW3901::PMW3901(uint8_t csPin)
    : m_csPin(csPin) {
    }

// Hàm báo kết quả khởi tạo SPI
bool PMW3901::begin_PMW3901() {
    // Khởi tạo bus
    
    // Cấu hình SPi
    spi_device_interface_config_t dev_cfg = {};
        dev_cfg.clock_speed_hz = 100000;
        dev_cfg.mode = 3;                   // mode SPI, phụ thuộc vào IC
        dev_cfg.spics_io_num = m_csPin;     // chọn chân
        dev_cfg.queue_size = 1;             // chưa rõ, nhưng yêu cầu tối thiểu bằng 1

    

    return true;
}