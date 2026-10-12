#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_now.h"
#include "esp_log.h"

static const char *TAG = "DRONE_2";

typedef struct {
    uint8_t ping_id;
} ping_msg_t;

// Hàm ngắt nhận: Nhận được gói tin từ Drone 1 -> Dội ngược lại ngay lập tức
void on_data_recv(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
    if (len == sizeof(ping_msg_t)) {
        // Tự động thêm Drone 1 vào danh sách Peer nếu chưa có
        esp_now_peer_info_t peerInfo = {};
        memcpy(peerInfo.peer_addr, info->src_addr, 6);
        peerInfo.channel = 1; // Bắt buộc trùng kênh 1 với Drone 1
        peerInfo.encrypt = false;
        peerInfo.ifidx = WIFI_IF_STA;
        
        if (!esp_now_is_peer_exist(info->src_addr)) {
            esp_now_add_peer(&peerInfo);
        }

        ping_msg_t *msg = (ping_msg_t *)data;
        ESP_LOGI(TAG, "Nhan duoc ping #%d, dang doi lai...", msg->ping_id);

        // Bắn trả lại đúng gói tin đó về cho Drone 1
        esp_now_send(info->src_addr, data, len);
    }
}

void app_main(void) {
    // Chờ 1 giây cho UART ổn định
    vTaskDelay(pdMS_TO_TICKS(1000));
    ESP_LOGI(TAG, "Dang khoi dong Drone 2...");

    // 1. Khởi tạo NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
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

    // Cố định kênh Wi-Fi là 1
    esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);

    // 3. Khởi tạo ESP-NOW
    ESP_ERROR_CHECK(esp_now_init());
    ESP_ERROR_CHECK(esp_now_register_recv_cb(on_data_recv));

    ESP_LOGI(TAG, "Drone 2 da san sang lang nghe...");

    // Vòng lặp duy trì hoạt động
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}