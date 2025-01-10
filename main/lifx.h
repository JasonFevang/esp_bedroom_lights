#ifndef LIFX_H
#define LIFX_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/*
Must call first
*/
void lifx_init();

/*
Call from superloop
*/
void lifx_loop(TickType_t *next_update);

#endif // LIFX_H
