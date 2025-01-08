#include "dmx/include/driver.h"
#include "driver/gpio.h"
#include "esp_dmx.h"
#include "esp_log.h"
#include "freertos/projdefs.h"
#include "freertos/task.h"
#include <string.h>

#define TAG "Main"

#define TX_PIN 19 // the pin we are using to TX with
#define RX_PIN 18 // the pin we are using to RX with
#define EN_PIN 26 // the pin we are using to enable TX on the DMX transceiver

#define SWITCH_PIN                                                             \
    12 // the pin we are using to detect the light switch to control dmx

static uint8_t data_on[DMX_PACKET_SIZE] = {};        // Buffer to store DMX data
static const uint8_t data_off[DMX_PACKET_SIZE] = {}; // Buffer to store DMX data

typedef struct {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
    uint8_t white;
} bar_footprint_t;

typedef struct {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
    uint8_t white;
} bed_footprint_t;

static const uint16_t BAR_ADDRESS = 1;
static const uint16_t BED_ADDRESS = BAR_ADDRESS + sizeof(bar_footprint_t);

void app_main() {
    // Initialize dmx
    const dmx_port_t dmx_num = DMX_NUM_1;
    dmx_config_t config = DMX_CONFIG_DEFAULT;
    const int personality_count = 0;
    dmx_driver_install(dmx_num, &config, NULL, personality_count);
    dmx_set_pin(dmx_num, TX_PIN, RX_PIN, EN_PIN);

    // Initialize switch
    gpio_set_direction(SWITCH_PIN, GPIO_MODE_INPUT);
    gpio_set_pull_mode(SWITCH_PIN, GPIO_PULLUP_ONLY);
    bool switch_state = gpio_get_level(SWITCH_PIN);
    static const bool SWITCH_ON = 1;
    static const bool SWITCH_OFF = 0;

    // Initialize dmx fixtures
    const bar_footprint_t bar_footprint = {100, 0, 0, 20};
    const bed_footprint_t bed_footprint = {0, 100, 100, 100};

    // Initialize dmx frame
    memcpy(data_on + BAR_ADDRESS, &bar_footprint, sizeof(bar_footprint_t));
    memcpy(data_on + BED_ADDRESS, &bed_footprint, sizeof(bed_footprint_t));
    if (switch_state == SWITCH_ON) {
        dmx_write(dmx_num, data_on, DMX_PACKET_SIZE);
    } else {
        dmx_write(dmx_num, data_off, DMX_PACKET_SIZE);
    }

    const TickType_t DMX_SEND_PERIOD = pdMS_TO_TICKS(30);
    const TickType_t SWITCH_READ_PERIOD = pdMS_TO_TICKS(50);
    TickType_t next_dmx_send = 0;
    TickType_t next_switch_read = 0;

    while (1) {
        TickType_t now = xTaskGetTickCount();

        if (now > next_dmx_send) {
            // Send data and block until it's sent
            dmx_send_num(dmx_num, DMX_PACKET_SIZE);
            // dmx_wait_sent(dmx_num, DMX_TIMEOUT_TICK);
            next_dmx_send = now + DMX_SEND_PERIOD;
        }

        if (now > next_switch_read) {

            // Read switch
            const bool prev_switch_state = switch_state;
            switch_state = gpio_get_level(SWITCH_PIN);
            next_switch_read = now + SWITCH_READ_PERIOD;
            ESP_LOGI(TAG, "switch %d", switch_state);

            // Switch turned on
            if (switch_state == SWITCH_ON && prev_switch_state == SWITCH_OFF) {
                // Turn lights on
                dmx_write(dmx_num, data_on, DMX_PACKET_SIZE);
            }

            // Switch turned off
            if (switch_state == SWITCH_OFF && prev_switch_state == SWITCH_ON) {
                // Turn lights off
                dmx_write(dmx_num, data_off, DMX_PACKET_SIZE);
            }
        }
        vTaskDelay(1);
    }
}
