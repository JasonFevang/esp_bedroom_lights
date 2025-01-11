#include "lifx.h"
#include "wifi.h"

#include "esp_lifx.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include <string.h>
#include <sys/param.h>

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

#define HOST_IP_ADDR CONFIG_EXAMPLE_IPV4_ADDR

#define PORT CONFIG_EXAMPLE_PORT

static const char *TAG = "lifx";
static const char *payload = "Message from ESP32 ";

static char rx_buffer[128];
static const char host_ip[] = HOST_IP_ADDR;
static int addr_family = 0;
static int ip_protocol = 0;
static int sock;
static struct sockaddr_in dest_addr;

void lifx_init() {
    dest_addr.sin_addr.s_addr = inet_addr(HOST_IP_ADDR);
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(PORT);
    addr_family = AF_INET;
    ip_protocol = IPPROTO_IP;

    sock = socket(addr_family, SOCK_DGRAM, ip_protocol);
    if (sock < 0) {
        ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
        return;
    }

    // Set timeout
    struct timeval timeout;
    timeout.tv_sec = 10;
    timeout.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof timeout);

    ESP_LOGI(TAG, "Socket created, sending to %s:%d", HOST_IP_ADDR, PORT);
}

typedef enum { WAIT_WIFI, TX, RX, DELAY } udp_state_t;

static udp_state_t state;

/*
Call from superloop
*/
void lifx_loop(TickType_t *next_update) {
    // static const TickType_t LIFX_CHECK_PERIOD = pdMS_TO_TICKS(50);
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
        int err = sendto(sock, payload, strlen(payload), 0,
                         (struct sockaddr *)&dest_addr, sizeof(dest_addr));
        if (err < 0) {
            ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
            break;
        }
        ESP_LOGI(TAG, "Message sent");
        state = RX;
        break;
    }
    case RX: {
        // Timeout on waiting for a response
        if (now - last_send > LIFX_RESEND_PERIOD) {
            state = TX;
            return;
        }

        struct sockaddr_storage
            source_addr; // Large enough for both IPv4 or IPv6
        socklen_t socklen = sizeof(source_addr);
        int len = recvfrom(sock, rx_buffer, sizeof(rx_buffer) - 1, MSG_DONTWAIT,
                           (struct sockaddr *)&source_addr, &socklen);

        // Error occurred during receiving
        if (len < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // No data available, can try again later
                break;
            } else {
                // An actual error occurred
                ESP_LOGE(TAG, "recvfrom failed: errno %d", errno);
            }
            break;
        }
        // Data received
        else {
            rx_buffer[len] = 0; // Null-terminate whatever we received and
                                // treat like a string
            ESP_LOGI(TAG, "Received %d bytes from %s:", len, host_ip);
            ESP_LOGI(TAG, "%s", rx_buffer);
            state = DELAY;
            wait_start = now;
        }
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
