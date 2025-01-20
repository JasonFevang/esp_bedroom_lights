#include "udp.h"

#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/task.h>
#include <string.h>
#include <sys/param.h>

#include <esp_lifx.h>
#include <lwip/netdb.h>
#include <lwip/sockets.h>

static const char *TAG = "udp";

int my_udp_create_socket() {
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

lx_err_t lx_udp_tx(int sock, const uint8_t *data, uint32_t data_len,
                   uint16_t port, uint32_t ip_addr) {
    struct sockaddr_in dest_addr = {.sin_family = AF_INET,
                                    .sin_port = htons(port),
                                    .sin_addr = {.s_addr = ip_addr}};

    const int err = sendto(sock, data, data_len, 0,
                           (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    return err < 0 ? LX_ERR : LX_OK;
}

lx_err_t lx_udp_rx(int sock, uint8_t *rx_buf, uint32_t rx_buf_sz, uint32_t *len,
                   uint16_t *port, uint32_t *ip_addr) {
    assert(rx_buf != NULL && len != NULL && port != NULL && ip_addr != NULL);

    struct sockaddr_storage source_addr; // Large enough for both IPv4 or IPv6
    socklen_t socklen = sizeof(source_addr);

    int32_t res = recvfrom(sock, rx_buf, rx_buf_sz, MSG_DONTWAIT,
                           (struct sockaddr *)&source_addr, &socklen);

    // Error occurred during receiving
    if (res < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            // No data available, can try again later
            return LX_TIMEOUT;
        }
        return LX_ERR;
    }

    // Validate we're ipv4
    if (source_addr.ss_family != AF_INET) {
        ESP_LOGE(TAG, "Error: only support ipv4, but found family '%d'",
                 source_addr.ss_family);
        return LX_ERR;
    }

    *len = res;

    // Extract the ipv4 address
    *ip_addr = ((struct sockaddr_in *)&source_addr)->sin_addr.s_addr;

    // Extract the port
    *port = ntohs(((struct sockaddr_in *)&source_addr)->sin_port);

    return LX_OK;
}
