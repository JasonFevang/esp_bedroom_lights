#include "app.h"
#include "dmx.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lifx.h"
#include "switch.h"

#define TAG "Main"

void app_main() {
    lifx_init();
    dmx_init();
    switch_init();
    app_init();

    TickType_t next_dmx_send = 0;
    TickType_t next_switch_read = 0;
    TickType_t next_lifx_update = 0;

    while (1) {
        lifx_loop(&next_lifx_update);
        dmx_loop(&next_dmx_send);
        switch_loop(&next_switch_read);
        app_loop();
        vTaskDelay(1);
    }
}
