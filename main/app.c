#include "dmx.h"
#include "switch.h"
#include <string.h>

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

#define BAR_ADDRESS 1
#define BED_ADDRESS BAR_ADDRESS + sizeof(bar_footprint_t)

static_assert(BAR_ADDRESS + sizeof(bar_footprint_t) <= DMX_FRAME_SZ,
              "BAR_ADDRESS out of bounds");
static_assert(BED_ADDRESS + sizeof(bed_footprint_t) <= DMX_FRAME_SZ,
              "BED_ADDRESS out of bounds");

static const bar_footprint_t bar_footprint = {100, 0, 0, 20};
static const bed_footprint_t bed_footprint = {0, 100, 100, 100};

static switch_state_t prev_switch_state;

static void turn_lights_on() {
    memcpy(dmx_frame + BAR_ADDRESS, &bar_footprint, sizeof(bar_footprint_t));
    memcpy(dmx_frame + BED_ADDRESS, &bed_footprint, sizeof(bed_footprint_t));
    dmx_send_frame();
}

static void turn_lights_off() {
    memset(dmx_frame, 0, DMX_FRAME_SZ);
    dmx_send_frame();
}

void app_init() {
    if (switch_state == SWITCH_ON) {
        turn_lights_on();
    } else {
        turn_lights_off();
    }

    prev_switch_state = switch_state;
}

void app_loop() {
    if (switch_state == SWITCH_ON && prev_switch_state == SWITCH_OFF) {
        turn_lights_on();
    }

    if (switch_state == SWITCH_OFF && prev_switch_state == SWITCH_ON) {
        turn_lights_off();
    }
}
