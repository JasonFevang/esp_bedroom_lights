#ifndef UDP_H
#define UDP_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sys/types.h"
#include <lifx/enums.h>

/*
 */
int my_udp_create_socket();

/*
 */
esp_err_t my_udp_send(int sock, const uint8_t *data, uint32_t data_len,
                      uint16_t port, in_addr_t ip_addr);

/*
 */
esp_err_t my_udp_receive(int sock, uint8_t *rx_buf, uint32_t rx_buf_sz,
                         uint32_t *len, uint16_t *port, in_addr_t *ip_addr);

lx_err_t lx_udp_tx(int sock, const uint8_t *data, uint32_t data_len,
                   uint16_t port, uint32_t ip_addr);

lx_err_t lx_udp_rx(int sock, uint8_t *rx_buf, uint32_t rx_buf_sz, uint32_t *len,
                   uint16_t *port, uint32_t *ip_addr);
#endif // UDP_H
