#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_now.h"
#include "esp_log.h"

extern void imuTask(void*);
extern void gpsTask(void*);
extern void gpsTask(void*);
extern void ekfTask(void*);
extern void controlTask(void*);
extern void loggerTask(void*);

int main()
{
    xTaskCreatePinnedToCore(
        imuTask,
        "IMU",
        4096,
        nullptr,
        10,
        nullptr,
        1
    );
}