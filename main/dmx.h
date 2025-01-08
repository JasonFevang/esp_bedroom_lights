#ifndef DMX_H
#define DMX_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/*
Must call first
*/
void dmx_init();

/*
Call from superloop
*/
void dmx_loop(TickType_t *next_update);

/*
Load the current value of dmx_frame[] into esp_dmx
*/
void dmx_send_frame();

/*
Buffer storing the next dmx frame to send. After updating, must call
dmx_send_frame() for changes to take effect
*/
extern uint8_t dmx_frame[];

/*
Size of dmx_frame[]
*/
#define DMX_FRAME_SZ 513

#endif // DMX_H
