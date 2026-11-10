#include "imu_module.hpp"

#include "topics.hpp"

#include "esp_timer.h"
#include "esp_log.h"

static const char* TAG = "IMU";

bool ImuModule::init()
{
    // Init SPI
    // Init IMU

    ESP_LOGI(TAG, "IMU Init");

    return true;
}

bool ImuModule::start()
{
    BaseType_t ret = xTaskCreatePinnedToCore(
        taskEntry,
        "imu",
        4096,
        nullptr,
        10,
        nullptr,
        1
    );

    return ret == pdPASS;
}


void ImuModule::taskEntry(void* arg)
{
    ImuData msg{};

    TickType_t lastWake =
                xTaskGetTickCount();

    while (true)
    {
        // Doc cam bien


        msg.timestamp_us = esp_timer_get_time();

        msg.ax = 0.1f;
        msg.ay = 0.1f;
        msg.az = 0.1f;

        msg.gx = 0.01f;
        msg.gy = 0.01f;
        msg.gz = 0.01f;

        // Publish
        imu_topic.publish(msg);

        // Wake up 1kHz
        vTaskDelayUntil(
            &lastWake,
            pdMS_TO_TICKS(1)
        );
    }
}