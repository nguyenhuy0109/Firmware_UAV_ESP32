

#include <stdint.h>

#include "esp_err.h"
#include "driver/spi_master.h"
// #include "freertos/FreeRTOS.h"

#include "bmi270.hpp"

//=====HÀM CHỌN CHÂN=====//

BMI270::BMI270(gpio_num_t csPin)
    :m_csPin(csPin) {
    }

//=====SET CHÂN GPIO=====//

esp_err_t BMI270::init_CS_GPIO() {
    if(m_csPin < 0) 
        return ESP_ERR_INVALID_ARG;
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