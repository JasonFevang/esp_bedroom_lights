#ifndef APP_H
#define APP_H

/*
Must call first
*/
void app_init();

/*
Call from superloop. Runs application logic
*/
void app_loop();

#endif // APP_H
