

#include "pmw3901mb_txqt.hpp"

#include <driver/spi_master.h>
#include "esp_err.h"
#include "driver/gpio.h"
#include "esp_rom_sys.h"
#include "freertos/FreeRTOS.h"

#define PRODUCT_ID 0x49
#define REVISION_ID 0X00

// Đặt chân theo board (chưa có board)
namespace pin {
    constexpr int MOSI_PIN = 1;
    constexpr int MISO_PIN = 2;
    constexpr int SCLK_PIN = 3;
}

// Lưu số chân chọn cảm biến
PMW3901::PMW3901(uint8_t csPin)
    : m_csPin(csPin) {
    }

//=====CHECK KHỞI TẠO SPI=====//
bool PMW3901::init_SPI_PMW3901() {
    // Chọn bộ điều khiển SPI
    spi_host_device_t host = SPI2_HOST;

    // Cấu hình dây
    spi_bus_config_t bus_cfg = {};
    bus_cfg.mosi_io_num = pin::MOSI_PIN;
    bus_cfg.miso_io_num = pin::MISO_PIN;
    bus_cfg.sclk_io_num = pin::SCLK_PIN;
    bus_cfg.quadwp_io_num = -1;
    bus_cfg.quadhd_io_num = -1;
    bus_cfg.data4_io_num = -1;
    bus_cfg.data5_io_num = -1;
    bus_cfg.data6_io_num = -1;
    bus_cfg.data7_io_num = -1;
    bus_cfg.max_transfer_sz = 256;          // tối đa 256 bytes buffer

    // Khởi tạo bus
    esp_err_t err_spi_bus_init = spi_bus_initialize(host, &bus_cfg, SPI_DMA_CH_AUTO);

    if(err_spi_bus_init != ESP_OK) {
        return false;
    }
    
    // Cấu hình cho thiết bị thêm vào bus
    spi_device_interface_config_t dev_cfg = {};
        dev_cfg.clock_speed_hz = 100000;
        dev_cfg.mode = 3;                   // mode SPI, phụ thuộc vào IC
        dev_cfg.spics_io_num = m_csPin;     // chọn chân
        dev_cfg.queue_size = 1;             // hàng đợi của thiết bị có sức chứa 1

    // Đăng ký thiết bị
    esp_err_t err_device = spi_bus_add_device(SPI2_HOST, &dev_cfg, &m_dev);

    if(err_device != ESP_OK) {
        spi_bus_free(host);                 // Giải phóng bus
        return false;
    }

    return true;
}

//====HÀM GHI VÀO THANH GHI=====//
esp_err_t PMW3901::registerWrite(uint8_t reg, uint8_t value)
{
    if (m_dev == nullptr) {
        return ESP_ERR_INVALID_STATE;
    }

    // Lấy quyền sử dụng bus trước khi kéo CS xuống và truyền.
    esp_err_t err = spi_device_acquire_bus(m_dev, portMAX_DELAY);
    if (err != ESP_OK) {
        return err;                         // Chưa giữ được bus nên không cần release.
    }

    spi_transaction_t transaction = {};
    transaction.flags = SPI_TRANS_USE_TXDATA;
    transaction.length = 8;

    const gpio_num_t cs = static_cast<gpio_num_t>(m_csPin);

    gpio_set_level(cs, 0);
    esp_rom_delay_us(50);

    // Gửi byte thứ nhất: địa chỉ kèm bit ghi.
    transaction.tx_data[0] = reg | 0x80;
    err = spi_device_polling_transmit(m_dev, &transaction);

    // Chỉ gửi dữ liệu nếu truyền địa chỉ thành công.
    if (err == ESP_OK) {
        esp_rom_delay_us(50);

        transaction.tx_data[0] = value;
        err = spi_device_polling_transmit(m_dev, &transaction);
    }

    // Luôn kết thúc giao tiếp, kể cả khi truyền lỗi.
    esp_rom_delay_us(50);
    gpio_set_level(cs, 1);
    esp_rom_delay_us(200);

    spi_device_release_bus(m_dev);

    return err;
}