#include "app.h"
#include "dmx.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lifx.h"
#include "nvs_flash.h"
#include "switch.h"
#include "wifi.h"

#define TAG "Main"

void init_nvs(){
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
}

void app_main() {
    init_nvs();
    wifi_init();
    lifx_init();
    dmx_init();
    switch_init();
    app_init();

    TickType_t next_wifi_update = 0;
    TickType_t next_dmx_send = 0;
    TickType_t next_switch_read = 0;
    TickType_t next_lifx_update = 0;

    while (1) {
        wifi_loop(&next_wifi_update);
        lifx_loop(&next_lifx_update);
        dmx_loop(&next_dmx_send);
        switch_loop(&next_switch_read);
        app_loop();
        vTaskDelay(1);
    }
}
