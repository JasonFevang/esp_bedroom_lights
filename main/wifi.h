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

typedef enum { WIFI_DISCONNECTED = 0, WIFI_CONNECTED = 1 } wifi_state_t;

/*
Last known state of the switch
*/
extern wifi_state_t wifi_connection;

#endif // WIFI_H
