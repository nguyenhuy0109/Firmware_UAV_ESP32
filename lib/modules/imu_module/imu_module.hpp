#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

class ImuModule
{
    public:
    static bool init();

    static bool start();

    private:
    static void taskEntry(void* arg);

    static void readSensor();
};