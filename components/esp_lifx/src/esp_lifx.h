#ifndef ESP_LIFX_H
#define ESP_LIFX_H
#include "lifx/enums.h"
#include "lifx/structs.h"

typedef struct {
    uint32_t ip;
    uint16_t port;
    uint64_t target;
} lx_device_t;

extern const lx_device_t lx_broadcast_device;

// udp tx function
typedef lx_err_t (*lx_udp_tx_f)(int sock, const uint8_t *data,
                                uint32_t data_len, uint16_t port,
                                uint32_t ip_addr);

// udp rx function
typedef lx_err_t (*lx_udp_rx_f)(int sock, uint8_t *rx_buf, uint32_t rx_buf_sz,
                                uint32_t *len, uint16_t *port,
                                uint32_t *ip_addr);

typedef enum {
    LX_EVENT_MSG_RX, // Data: lx_message_t
    LX_EVENT_ERR,    // Data: lx_err_t(not a pointer)
    LX_EVENT_NONE,   // Data: NULL
} lx_event_type_t;

// Signal from lifx to the caller via the event callback.
// Contents of data depends on the type. See lx_event_type_t
typedef struct {
    lx_event_type_t type;
    void *data;
} lx_event_t;

// event cb
typedef void (*lx_event_cb)(lx_event_t event);

typedef struct {
    uint32_t source_id;
    uint32_t sequence;
    lx_device_t *known_devices;
    lx_udp_tx_f udp_tx;
    lx_udp_rx_f udp_rx;
    lx_event_cb event_cb;
    int sock;
    uint8_t *rx_buf;
    uint8_t *tx_buf;
} lx_client_t;

// An lifx message. Used for rx and tx. If no payload, set NULL
typedef struct {
    lx_packet_t type;
    const uint8_t *payload;
} lx_message_t;

// Create an lifx client
lx_client_t *lx_client_create(int sock, lx_udp_tx_f udp_tx, lx_udp_rx_f udp_rx,
                              lx_event_cb event_cb);

void lx_client_delete(lx_client_t *);

// Send a message to a device
lx_err_t lx_client_send_message(lx_client_t *client, const lx_device_t *device,
                                const lx_message_t *message);

// Call periodically to check for events. This may trigger the event callback
void lx_client_poll(lx_client_t *client);

#endif // ESP_LIFX_H
