#ifndef WIFI_H
#define WIFI_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/*
Must call first
*/
void wifi_init();

/*
Call from superloop
*/
void wifi_loop(TickType_t *next_update);

#endif // WIFI_H
