#include "switch.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define SWITCH_PIN 12

switch_state_t switch_state = SWITCH_ON;

const TickType_t SWITCH_READ_PERIOD = pdMS_TO_TICKS(50);

#define TAG "Switch"

void switch_init() {
    gpio_set_direction(SWITCH_PIN, GPIO_MODE_INPUT);
    gpio_set_pull_mode(SWITCH_PIN, GPIO_PULLUP_ONLY);
    switch_state = gpio_get_level(SWITCH_PIN);
}

void switch_loop(TickType_t *next_update) {
    TickType_t now = xTaskGetTickCount();
    if (now > *next_update) {
        switch_state = gpio_get_level(SWITCH_PIN);
        *next_update = now + SWITCH_READ_PERIOD;
    }
}
