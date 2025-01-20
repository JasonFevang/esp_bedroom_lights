#include "lifx.h"
#include "lwip/sockets.h"
#include "udp.h"
#include "wifi.h"
#include <esp_lifx.h>
#include <lifx/packets.h>

#include "freertos/FreeRTOS.h"
#include <esp_log.h>
#include <freertos/event_groups.h>
#include <freertos/task.h>
#include <string.h>
#include <sys/param.h>

#include <lwip/netdb.h>

#define HOST_IP_ADDR CONFIG_EXAMPLE_IPV4_ADDR

static const char *TAG = "lifx";

static int sock;
static lx_client_t *lx_client;

typedef enum { WAIT_WIFI, TX, RX, DELAY_START, DELAY } udp_state_t;

static udp_state_t state;

static void event_cb(lx_event_t event) {
    if (event.type != LX_EVENT_MSG_RX) {
        ESP_LOGE(TAG, "Cannot handle event '%d'", event.type);
        return;
    };
    lx_message_t *message = (lx_message_t *)event.data;

    const lx_protocol_header_t *header =
        (const lx_protocol_header_t *)(message->payload -
                                       sizeof(lx_protocol_header_t));

    lx_protocol_header_print(header,
                             header->size - sizeof(lx_protocol_header_t));

    if (message->type == LX_PACKET_STATE_SERVICE) {
        const lx_state_service_payload_t *payload =
            (const lx_state_service_payload_t *)message->payload;
        lx_payload_state_service_print(payload);
    }

    // TODO: This is jank. I need a better state machine to do this
    state = DELAY_START;
}

void lifx_init() {
    sock = my_udp_create_socket();
    if (sock < 0) {
        ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
        return;
    }
    ESP_LOGI(TAG, "Socket created");

    lx_client = lx_client_create(sock, lx_udp_tx, lx_udp_rx, event_cb);
}

/*
Call from superloop
*/
void lifx_loop(TickType_t *next_update) {
    static const TickType_t LIFX_RESEND_PERIOD = pdMS_TO_TICKS(1000);
    static const TickType_t WAIT_TIME = pdMS_TO_TICKS(2500);

    static TickType_t last_send = 0;
    static TickType_t wait_start;

    TickType_t now = xTaskGetTickCount();

    switch (state) {
    case WAIT_WIFI: {
        if (wifi_connection == WIFI_CONNECTED) {
            state = TX;
        }
        break;
    }
    case TX: {
        last_send = now;

        const lx_message_t message = {.type = LX_PACKET_GET_SERVICE,
                                      .payload = NULL};
        const lx_err_t err =
            lx_client_send_message(lx_client, &lx_broadcast_device, &message);

        if (err != LX_OK) {
            ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
        } else {
            ESP_LOGI(TAG, "Message sent");
        }

        state = RX;
        break;
    }
    case RX: {
        // Timeout on waiting for a response
        if (now - last_send > LIFX_RESEND_PERIOD) {
            state = TX;
            return;
        }

        lx_client_poll(lx_client);
        break;
    }
    case DELAY_START: {
        wait_start = now;
        state = DELAY;
        break;
    }
    case DELAY: {
        if (now - wait_start > WAIT_TIME) {
            state = TX;
        }
        break;
    }
    };
}
