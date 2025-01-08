#ifndef SWITCH_H
#define SWITCH_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/*
Must call first
*/
void switch_init();

/*
Call from superloop. Updates switch_state
*/
void switch_loop(TickType_t *next_update);

/*
Possible switch states
*/
typedef enum { SWITCH_OFF = 0, SWITCH_ON = 1 } switch_state_t;

/*
Last known state of the switch
*/
extern switch_state_t switch_state;

#endif // SWITCH_H
