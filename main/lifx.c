#include "lifx.h"
#include "udp.h"
#include "wifi.h"

#include "freertos/FreeRTOS.h"
#include <esp_lifx.h>
#include <esp_log.h>
#include <freertos/event_groups.h>
#include <freertos/task.h>
#include <string.h>
#include <sys/param.h>

#include <lwip/netdb.h>

#define HOST_IP_ADDR CONFIG_EXAMPLE_IPV4_ADDR

#define PORT CONFIG_EXAMPLE_PORT

static const char *TAG = "lifx";

static char rx_buffer[128];
static int sock;

void lifx_init() {
    sock = my_udp_create_socket();
    if (sock < 0) {
        ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
        return;
    }

    ESP_LOGI(TAG, "Socket created");
}

typedef enum { WAIT_WIFI, TX, RX, DELAY } udp_state_t;

static udp_state_t state;

/*
Call from superloop
*/
void lifx_loop(TickType_t *next_update) {
    static const TickType_t LIFX_RESEND_PERIOD = pdMS_TO_TICKS(600);
    static const TickType_t WAIT_TIME = pdMS_TO_TICKS(1000);

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
        const char *payload = "Message from ESP32";

        const esp_err_t res =
            my_udp_send(sock, (const uint8_t *)payload, strlen(payload), PORT,
                        inet_addr(HOST_IP_ADDR));
        if (res == ESP_FAIL) {
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

        uint32_t len;
        uint16_t port;
        in_addr_t ip_addr;
        const esp_err_t res =
            my_udp_receive(sock, (uint8_t *)rx_buffer, sizeof(rx_buffer) - 1,
                           &len, &port, &ip_addr);
        if (res == ESP_ERR_TIMEOUT) {
            break;
        }
        if (res == ESP_FAIL) {
            ESP_LOGE(TAG, "recvfrom failed: errno %d", errno);
            break;
        }

        rx_buffer[len] = 0; // Null-terminate whatever we received and
                            // treat like a string
        ESP_LOGI(TAG, "Received %lu bytes from %s:", len, inet_ntoa(ip_addr));
        ESP_LOGI(TAG, "%s", rx_buffer);

        state = DELAY;
        wait_start = now;
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
