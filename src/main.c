#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_now.h"
#include "esp_log.h"

static const char *TAG = "DRONE_1";

// >>> ĐIỀN ĐỊA CHỈ MAC CỦA CON NHẬN (DRONE 2) VÀO ĐÂY <<<
uint8_t drone2_mac[6] = {0x8B, 0xD6, 0x1A, 0x5D, 0xE9, 0x50};

typedef struct {
    uint8_t ping_id;
} ping_msg_t;

volatile uint32_t t_send = 0;
volatile uint32_t t_recv = 0;
volatile bool received_ack = false;

// KHAI BÁO BIẾN ĐẾM BẢN TIN Ở ĐÂY
int id_counter = 0; 

// Hàm callback gửi (Chuẩn ESP-IDF v6)
void on_data_sent(const wifi_tx_info_t *tx_info, esp_now_send_status_t status) {
    // Trạng thái gửi
}

// Hàm callback nhận phản hồi từ Drone 2
void on_data_recv(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
    t_recv = xthal_get_ccount(); 
    received_ack = true;
}

void app_main(void) {
    // Chờ 1 giây cho hệ thống UART ổn định để in log
    vTaskDelay(pdMS_TO_TICKS(1000));
    ESP_LOGI(TAG, "Dang khoi dong Drone 1...");

    // 1. Khởi tạo NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase()); // Sửa tên hàm erase nếu có cảnh báo, hoặc giữ nguyên nvs_flash_erase
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // 2. Khởi tạo Wi-Fi Station
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));

    // Cố định kênh Wi-Fi là 1 để chắc chắn 2 con chung kênh
    esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);

    // 3. Khởi tạo ESP-NOW
    ESP_ERROR_CHECK(esp_now_init());
    ESP_ERROR_CHECK(esp_now_register_send_cb(on_data_sent));
    ESP_ERROR_CHECK(esp_now_register_recv_cb(on_data_recv));

    // 4. Thêm Peer (Drone 2)
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, drone2_mac, 6);
    peerInfo.channel = 1; // Chỉ định rõ kênh 1
    peerInfo.encrypt = false;
    peerInfo.ifidx = WIFI_IF_STA;
    
    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        ESP_LOGE(TAG, "Loi: Khong the them Peer!");
        return;
    }

    ESP_LOGI(TAG, "Da ket noi Peer thanh cong. Bat dau gui tin hieu...");

    ping_msg_t msg;

    // 5. Vòng lặp chính
    while(1) {
        msg.ping_id = id_counter++;
        received_ack = false;

        t_send = xthal_get_ccount(); 
        
        // Bắn gói tin đi
        esp_err_t err = esp_now_send(drone2_mac, (uint8_t *) &msg, sizeof(msg));
        
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Loi ham gui esp_now_send! Ma loi: %d", err);
        }

        // Chờ phản hồi (timeout 50ms)
        int timeout = 50; 
        while(!received_ack && timeout > 0) {
            esp_rom_delay_us(1000); // Chờ 1ms mỗi nhịp
            timeout--;
        }

        if(received_ack) {
            uint32_t rtt = t_recv - t_send;
            
            // Tính toán khoảng cách thô (chưa trừ hardware delay)
            float time_of_flight = (float)rtt / 240000000.0;
            float distance = (time_of_flight * 300000000.0) / 2.0;
            
            ESP_LOGI(TAG, ">>> NHAN DUOC PHAN HOI! RTT Ticks = %lu | Khoang cach tho: %.2f m", 
                     (unsigned long)rtt, distance);
        } else {
            ESP_LOGW(TAG, "--- TIMEOUT: Khong thay Drone 2 tra loi! ---");
        }

        // Nghỉ 1 giây trước khi gửi gói tiếp theo
        vTaskDelay(pdMS_TO_TICKS(1000)); 
    }
}