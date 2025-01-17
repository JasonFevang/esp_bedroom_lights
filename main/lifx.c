#include "lifx.h"
#include "lwip/ip_addr.h"
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

static char rx_buffer[128];
static int sock;

static int create_socket() {
    int res_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (res_sock < 0) {
        return res_sock;
    }

    // Set timeout
    struct timeval timeout;
    timeout.tv_sec = 10;
    timeout.tv_usec = 0;
    setsockopt(res_sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof timeout);

    return res_sock;
}

static esp_err_t send_udp(const uint8_t *data, uint32_t data_len, uint16_t port,
                          in_addr_t ip_addr) {
    struct sockaddr_in dest_addr = {.sin_family = AF_INET,
                                    .sin_port = htons(port),
                                    .sin_addr = {.s_addr = ip_addr}};

    const int err = sendto(sock, data, data_len, 0,
                           (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    return err < 0 ? ESP_FAIL : ESP_OK;
}

static esp_err_t receive_udp(uint8_t *rx_buf, uint32_t rx_buf_sz, uint32_t *len,
                             uint16_t *port, in_addr_t *ip_addr) {
    assert(rx_buf != NULL && len != NULL && port != NULL && ip_addr != NULL);

    struct sockaddr_storage source_addr; // Large enough for both IPv4 or IPv6
    socklen_t socklen = sizeof(source_addr);

    int32_t res = recvfrom(sock, rx_buf, rx_buf_sz, MSG_DONTWAIT,
                           (struct sockaddr *)&source_addr, &socklen);

    // Error occurred during receiving
    if (res < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            // No data available, can try again later
            return ESP_ERR_TIMEOUT;
        }
        return ESP_FAIL;
    }

    // Validate we're ipv4
    if (source_addr.ss_family != AF_INET) {
        ESP_LOGE(TAG, "Error: only support ipv4, but found family '%d'",
                 source_addr.ss_family);
        return ESP_FAIL;
    }

    *len = res;

    // Extract the ipv4 address
    *ip_addr = ((struct sockaddr_in *)&source_addr)->sin_addr.s_addr;

    // Extract the port
    *port = ntohs(((struct sockaddr_in *)&source_addr)->sin_port);

    return ESP_OK;
}

void lifx_init() {
    sock = create_socket();
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
        const char *payload = "Message from ESP32";

        esp_err_t res = send_udp((const uint8_t *)payload, strlen(payload),
                                 PORT, inet_addr(HOST_IP_ADDR));
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
        esp_err_t res = receive_udp((uint8_t *)rx_buffer, sizeof(rx_buffer) - 1,
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
