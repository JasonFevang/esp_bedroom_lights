#include "dmx.h"
#include "dmx/include/driver.h"
#include "esp_dmx.h"
#include "freertos/task.h"
#include <assert.h>

#define TX_PIN 19
#define RX_PIN 18
#define EN_PIN 26

#define TAG "dmx"

uint8_t dmx_frame[DMX_FRAME_SZ] = {};

static_assert(DMX_FRAME_SZ <= DMX_PACKET_SIZE, "DMX_FRAME_SZ must be <= 513");

const dmx_port_t DMX_NUM = DMX_NUM_1;
const TickType_t DMX_SEND_PERIOD = pdMS_TO_TICKS(30);

void dmx_init() {
    dmx_config_t config = DMX_CONFIG_DEFAULT;
    const int personality_count = 0;
    dmx_driver_install(DMX_NUM, &config, NULL, personality_count);
    dmx_set_pin(DMX_NUM, TX_PIN, RX_PIN, EN_PIN);
}

void dmx_loop(TickType_t *next_update) {
    TickType_t now = xTaskGetTickCount();
    if (now > *next_update) {
        // Send data
        dmx_send_num(DMX_NUM, DMX_PACKET_SIZE);
        // Block until it's sent
        // dmx_wait_sent(dmx_num, DMX_TIMEOUT_TICK);
        *next_update = now + DMX_SEND_PERIOD;
    }
}

void dmx_send_frame() { dmx_write(DMX_NUM, dmx_frame, DMX_FRAME_SZ); }
