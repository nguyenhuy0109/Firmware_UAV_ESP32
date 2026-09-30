#include "driver/gpio.h"

#define LED_GPIO    2

void app_main() 
{
    gpio_reset_pin(LED_GPIO);
    gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);

    while(1)
    {
        gpio_set_level(LED_GPIO, 1);
        for (int i = 0; i < 1000; i++);
        gpio_set_level(LED_GPIO, 0);
        for (int i = 0; i < 1000; i++);
    }
}