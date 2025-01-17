#include "lifx.h"
#include "lifx_structs.h"
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

static uint8_t rx_buffer[1024];
static uint8_t tx_buffer[1024];
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

void print_buffer(const uint8_t *buffer, uint32_t len) {
    for (uint32_t i = 0; i < len; i++) {
        printf("%02X ", buffer[i]);
        if ((i + 1) % 16 == 0) {
            printf("\n"); // New line every 16 bytes
        }
    }
    printf("\n");
}

uint32_t populate_tx_buf(uint8_t *buf, uint32_t buf_len) {
    lx_protocol_header_t header;
    memset(&header, 0, sizeof(header));
    header.size = sizeof(lx_protocol_header_t);
    header.protocol = 1024;
    header.addressable = 1;
    header.tagged = 1;
    header.origin = 0;
    header.source = 12345678;
    header.res_required = 0;
    header.ack_required = 0;
    header.sequence = 0;
    header.type = 2;
    memcpy(buf, &header, sizeof(header));
    return sizeof(header);
}

void print_lx_protocol_header(const lx_protocol_header_t *header,
                              uint32_t buf_len) {
    printf("lx_protocol_header_t:\n");
    printf("  Frame:\n");
    printf("    Size: %u\n", header->size);
    printf("    Protocol: %u\n", header->protocol);
    printf("    Addressable: %u\n", header->addressable);
    printf("    Tagged: %u\n", header->tagged);
    printf("    Origin: %u\n", header->origin);
    printf("    Source: %lu\n", header->source);

    printf("  Frame Address:\n");
    printf("    Target: ");
    for (int i = 0; i < 8; i++) {
        printf("%02X", header->target[i]);
        if (i < 7) {
            printf(":");
        }
    }
    printf("\n");

    printf("    Res Required: %u\n", header->res_required);
    printf("    Ack Required: %u\n", header->ack_required);
    printf("    Sequence: %u\n", header->sequence);

    printf("  Protocol Header:\n");
    printf("    Type: %u\n", header->type);
    printf("  Pay load size: %ld\n", buf_len - sizeof(lx_protocol_header_t));
}

void decode_buf(uint8_t *buf, uint32_t buf_len) {
    if (buf_len < sizeof(lx_protocol_header_t)) {
        ESP_LOGW(TAG,
                 "Received message smaller than lx_protocol_header, %d bytes. "
                 "ignoring",
                 sizeof(lx_protocol_header_t));
        return;
    }
    lx_protocol_header_t header;
    memcpy(&header, buf, sizeof(lx_protocol_header_t));
    print_lx_protocol_header(&header, buf_len);
}

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

        uint32_t data_len = populate_tx_buf(tx_buffer, 1024);

        const esp_err_t res = my_udp_send(sock, tx_buffer, data_len, PORT,
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
        const esp_err_t res = my_udp_receive(
            sock, rx_buffer, sizeof(rx_buffer) - 1, &len, &port, &ip_addr);
        if (res == ESP_ERR_TIMEOUT) {
            break;
        }
        if (res == ESP_FAIL) {
            ESP_LOGE(TAG, "recvfrom failed: errno %d", errno);
            break;
        }

        ESP_LOGI(TAG, "Received %lu bytes from %s:", len, inet_ntoa(ip_addr));
        print_buffer(rx_buffer, len);
        decode_buf(rx_buffer, len);

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
