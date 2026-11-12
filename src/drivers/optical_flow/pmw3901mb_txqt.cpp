

#include "pmw3901mb_txqt.hpp"

#include "driver/spi_master.h"
#include "esp_err.h"
#include "driver/gpio.h"
#include "esp_rom_sys.h"
#include "freertos/FreeRTOS.h"

namespace pmw3901_val {
    constexpr uint8_t PRODUCT_ID         = 0x49;
    constexpr uint8_t INVERSE_PRODUCT_ID = 0xB6;
}

// #define PRODUCT_ID 0x49
// #define REVISION_ID 0X00
// #define INVERSE_PRODUCT_ID 0xB6

// Đặt chân theo board (chưa có board)
namespace pin {
    constexpr int MOSI_PIN = 1;
    constexpr int MISO_PIN = 2;
    constexpr int SCLK_PIN = 3;
}

//=====LƯU SỐ CHÂN CHỌN CẢM BIẾN=====//

PMW3901::PMW3901(uint8_t csPin)
    : m_csPin(csPin) {
    }

//=====SET CHÂN GPIO=====//

esp_err_t PMW3901::init_CS_GPIO() {
    gpio_config_t cfg = {};
        cfg.pin_bit_mask    = 1ULL << m_csPin;
        cfg.mode            = GPIO_MODE_OUTPUT;
        cfg.pull_up_en      = GPIO_PULLUP_DISABLE;
        cfg.pull_down_en    = GPIO_PULLDOWN_DISABLE;
        cfg.intr_type       = GPIO_INTR_DISABLE;    
    esp_err_t err = gpio_config(&cfg);
    if (err != ESP_OK)
        return err;
    // CS idle HIGH
    return gpio_set_level(static_cast<gpio_num_t>(m_csPin), 1);
}

//=====CHECK KHỞI TẠO SPI=====//

bool PMW3901::init_SPI_PMW3901() {
    // Chọn bộ điều khiển SPI
    spi_host_device_t host = SPI2_HOST;

    // Cấu hình dây
    spi_bus_config_t bus_cfg = {};
    bus_cfg.mosi_io_num     = pin::MOSI_PIN;
    bus_cfg.miso_io_num     = pin::MISO_PIN;
    bus_cfg.sclk_io_num     = pin::SCLK_PIN;
    bus_cfg.quadwp_io_num   = -1;
    bus_cfg.quadhd_io_num   = -1;
    bus_cfg.data4_io_num    = -1;
    bus_cfg.data5_io_num    = -1;
    bus_cfg.data6_io_num    = -1;
    bus_cfg.data7_io_num    = -1;
    bus_cfg.max_transfer_sz = 256;                              // tối đa 256 bytes buffer

    // Khởi tạo bus
    esp_err_t err_spi_bus_init = spi_bus_initialize(host, &bus_cfg, SPI_DMA_CH_AUTO);

    if(err_spi_bus_init != ESP_OK) {
        return false;
    }
    
    // Cấu hình cho thiết bị thêm vào bus
    spi_device_interface_config_t dev_cfg = {};
        dev_cfg.clock_speed_hz  = 100000;
        dev_cfg.mode            = 3;                             // mode SPI, phụ thuộc vào IC
        dev_cfg.spics_io_num    = -1;                            // chọn chân
        dev_cfg.queue_size      = 1;                             // hàng đợi của thiết bị có sức chứa 1

    // Đăng ký thiết bị
    esp_err_t err_device = spi_bus_add_device(SPI2_HOST, &dev_cfg, &m_dev);

    if(err_device != ESP_OK) {
        spi_bus_free(host);                                     // Giải phóng bus
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
        return err;                                             // Chưa giữ được bus nên không cần release.
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

    spi_device_release_bus(m_dev);                              // trả bus

    return err;
}

//=====KHỞI TẠO THANH GHI TỐI ƯU=====//

esp_err_t PMW3901::initRegister() {
    // Mỗi hàng gồm: {địa chỉ thanh ghi, giá trị cần ghi}.
    const uint8_t config[][2] = {
        {0x7F, 0x00},
        {0x61, 0xAD},
        {0x7F, 0x03},
        {0x40, 0x00},
        {0x7F, 0x05},
        {0x41, 0xB3},
        {0x43, 0xF1},
        {0x45, 0x14},
        {0x5B, 0x32},
        {0x5F, 0x34},
        {0x7B, 0x08},
        {0x7F, 0x06},
        {0x44, 0x1B},
        {0x40, 0xBF},
        {0x4E, 0x3F},
    };
                                        
    for (const auto& entry : config) {                          // lần lượt lấy từng hàng trong config, gọi hàng đang xét là entry
                                                                //  entry là 1 hàng trong bảng
                                                                // entry[0] là phần tử đầu tiên của hàng
        esp_err_t err_init_reg = registerWrite(entry[0], entry[1]);

        if (err_init_reg != ESP_OK) {
            return err_init_reg;
        }
    }

    return ESP_OK;
}

//=====HÀM ĐỌC THANH GHI=====//

esp_err_t PMW3901::registerRead(uint8_t reg, uint8_t *value) {
    // Kiểm tra nơi nhận dữ liệu và handle thiết bị.
    if (value == nullptr) {
        return ESP_ERR_INVALID_ARG;                             // tham số truyền vào kh hợp lệ
    }

    if (m_dev == nullptr) {
        return ESP_ERR_INVALID_STATE;                           // trạng thái hiện tại chưa thoả mãn
    }

    // Giữ bus trong toàn bộ thao tác đọc.
    esp_err_t err = spi_device_acquire_bus(m_dev, portMAX_DELAY);
    if (err != ESP_OK) {
        return err;
    }

    const gpio_num_t cs = static_cast<gpio_num_t>(m_csPin);

    err = gpio_set_level(cs, 0);
    if (err != ESP_OK) {
        spi_device_release_bus(m_dev);
        return err;
    }

    esp_rom_delay_us(50);                                       // delay

    // Gửi địa chỉ: xóa bit 7 để yêu cầu đọc.
    spi_transaction_t address_transaction = {};
    address_transaction.flags        = SPI_TRANS_USE_TXDATA;
    address_transaction.length       = 8;
    address_transaction.tx_data[0]   = reg & 0x7F;

    err = spi_device_polling_transmit(m_dev, &address_transaction);

    uint8_t received = 0;

    if (err == ESP_OK) {
        // Chờ cảm biến chuẩn bị dữ liệu.
        esp_rom_delay_us(500);

        // Gửi byte giả để tạo 8 xung clock, đồng thời nhận dữ liệu.
        spi_transaction_t data_transaction = {};
        data_transaction.flags          = SPI_TRANS_USE_TXDATA | SPI_TRANS_USE_RXDATA;
        data_transaction.length         = 8;
        data_transaction.rxlength       = 8;
        data_transaction.tx_data[0]     = 0x00;
                                                                // truyền đồng bộ, kiểm tra lỗi transmit                                                      
        err = spi_device_polling_transmit(m_dev, &data_transaction);

        if (err == ESP_OK) {
            received = data_transaction.rx_data[0];
        }
    }

    // Kết thúc giao tiếp kể cả khi truyền/nhận thất bại.
    esp_rom_delay_us(50);
    esp_err_t cs_err = gpio_set_level(cs, 1);                   // kiểm tra lỗi gpio
    esp_rom_delay_us(200);

    spi_device_release_bus(m_dev);                              // giải phóng bus

    // Giữ lỗi truyền/nhận nếu đã có.
    if (err != ESP_OK) {
        return err;
    }

    if (cs_err != ESP_OK) {
        return cs_err;
    }

    // Chỉ cập nhật biến của người gọi khi toàn bộ thao tác thành công.
    *value = received;                                          // ghi giá trị
    return ESP_OK;
}

//=====CHECK KHỞI TẠO=====//

bool PMW3901::begin() {
    vTaskDelay(pdMS_TO_TICKS(40));                              // chờ trên 1ms thì dùng vTaskDelay()
    if(!PMW3901::init_SPI_PMW3901())                            // check khởi tạo spi bus + device
        return false;
    if(init_CS_GPIO() != ESP_OK)                                // check khởi tạo GPIO thủ công
        return false;
    gpio_set_level(static_cast<gpio_num_t>(m_csPin), 0);        // reset giao tiếp, kéo CS xuống, chờ 50us
    esp_rom_delay_us(50);
    gpio_set_level(static_cast<gpio_num_t>(m_csPin), 1);        // reset giao tiếp, kéo CS lên, chờ 50us
    esp_rom_delay_us(50);
    if(registerWrite(0x3A, 0x5A) != ESP_OK)                     // ghi 0x5A vào Power_Up_Reset
        return false;
    vTaskDelay(pdMS_TO_TICKS(5));                               // chờ IC reset, mili second

    uint8_t product_id = 0;
    if(registerRead(0x00, &product_id) != ESP_OK)               // check product id
        return false;
    if(product_id != pmw3901_val::PRODUCT_ID) 
        return false;
    
    uint8_t inverse_id = 0;
    if(registerRead(0x5F, &inverse_id) != ESP_OK)               // check inverse product id (address: 0x5F)  
        return false;
    if(inverse_id != pmw3901_val::INVERSE_PRODUCT_ID)
        return false;
    
    uint8_t dummy = 0;                                          // read dummy bytes
    registerRead(0x02, &dummy);                                 // Motion
    registerRead(0x03, &dummy);                                 // Delta_X_L
    registerRead(0x04, &dummy);                                 // Delta_X_H
    registerRead(0x05, &dummy);                                 // Delta_Y_L
    registerRead(0x06, &dummy);                                 // Delta_Y_H

    if(initRegister() != ESP_OK)                                // check init performance optimization reg
        return false;
    return true;
}

//=====ĐỌC CHUYỂN ĐỘNG=====//

esp_err_t PMW3901::readMotionCount(int16_t *deltaX, int16_t *deltaY) {
    if (!deltaX || !deltaY)                                     // check lỗi dữ liệu gửi về
        return ESP_ERR_INVALID_ARG;                             
    uint8_t motion = 0;
    esp_err_t err = registerRead(0x02, &motion);                // check lỗi đọc dữ liệu
    if (err != ESP_OK)
        return err;
    uint8_t dummy = 0;
    registerRead(0x02, &dummy);                                 // read Motion (address: 0x02)
    uint8_t xl = 0, xh = 0, yl = 0, yh = 0;
    registerRead(0x03, &xl);    // Delta_X_L
    registerRead(0x04, &xh);    // Delta_X_H
    registerRead(0x05, &yl);    // Delta_Y_L
    registerRead(0x06, &yh);    // Delta_Y_H
    *deltaX = static_cast<int16_t>((static_cast<uint16_t>(xh) << 8) | xl);
    *deltaY = static_cast<int16_t>((static_cast<uint16_t>(yh) << 8) | yl);

    return ESP_OK;
}