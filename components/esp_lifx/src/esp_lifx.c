#include "esp_lifx.h"
#include "lifx/enums.h"
#include "lifx/packets.h"
#include <esp_log.h> // TODO: Remove dependency on esp_log
#include <string.h>

#define RX_BUF_SZ 1024
#define TX_BUF_SZ 1024
#define BROADCAST_PORT 56700
#define BROADCAST_TARGET 0

#define TAG "lifx"

const lx_device_t lx_broadcast_device = {
    .ip = 0xFFFFFFFF, .port = BROADCAST_PORT, .target = BROADCAST_TARGET};

lx_client_t *lx_client_create(int sock, lx_udp_tx_f udp_tx, lx_udp_rx_f udp_rx,
                              lx_event_cb event_cb) {
    lx_client_t *client = (lx_client_t *)malloc(sizeof(lx_client_t));

    client->source_id = 0;
    client->sequence = 0;
    client->known_devices = NULL;
    client->udp_tx = udp_tx;
    client->udp_rx = udp_rx;
    client->event_cb = event_cb;
    client->sock = sock;
    client->rx_buf = (uint8_t *)malloc(RX_BUF_SZ);
    client->tx_buf = (uint8_t *)malloc(RX_BUF_SZ);
    return client;
}

void lx_client_delete(lx_client_t *client) {
    free(client->rx_buf);
    free(client->tx_buf);
    free(client);
};

static void pop_target(uint8_t arr[8], uint64_t target) {
    for (int i = 0; i < 8; i++) {
        arr[i] = (target >> (8 * i)) & 0xFF; // Extract each byte
    }
}
static void pop_header(lx_protocol_header_t *header, uint32_t payload_len,
                       uint32_t source, uint64_t target, lx_packet_t packet) {
    memset(header, 0, sizeof(lx_protocol_header_t));
    header->size = sizeof(lx_protocol_header_t) + payload_len;
    header->protocol = 1024;
    header->addressable = 1;
    header->tagged = (target == BROADCAST_TARGET ? 1 : 0);
    header->origin = 0;
    header->source = source;
    pop_target(header->target, target);
    header->res_required = 0;
    header->ack_required = 0;
    header->sequence = 0;
    header->type = LX_PACKET_GET_SERVICE;
}

lx_err_t lx_client_send_message(lx_client_t *client, const lx_device_t *device,
                                const lx_message_t *message) {
    lx_protocol_header_t header;
    pop_header(&header, 0, client->source_id, device->target, message->type);

    return client->udp_tx(client->sock, (void *)&header,
                          sizeof(lx_protocol_header_t), device->port,
                          device->ip);
}

static lx_err_t validate_payload_size(lx_packet_t packet_type,
                                      uint32_t payload_len) {
    // TODO: Use a dictionary to check this generically for all header types
    if (packet_type == LX_PACKET_STATE_SERVICE) {
        if (payload_len != sizeof(lx_state_service_payload_t)) {
            ESP_LOGW(
                TAG,
                "Packet contains a payload of size '%lu' bytes, but packet "
                "'%d' expects size '%d' bytes. Ignoring",
                payload_len, LX_PACKET_STATE_SERVICE,
                sizeof(lx_state_service_payload_t));
            return LX_ERR;
        }
        return LX_OK;
    }
    ESP_LOGW(TAG, "Unkown packet type, cannot validate payload size");
    return LX_OK;
}

static lx_err_t decode_packet(uint8_t *buf, uint32_t buf_len,
                              lx_message_t *message_out) {
    if (buf_len < sizeof(lx_protocol_header_t)) {
        ESP_LOGW(TAG,
                 "Received message smaller than lx_protocol_header, %d bytes. "
                 "ignoring",
                 sizeof(lx_protocol_header_t));
        return LX_OK;
    }

    lx_protocol_header_t header;
    memcpy(&header, buf, sizeof(lx_protocol_header_t));
    // Don't do this b/c if buf is misaligned, it can cause issues on platforms
    // with strict memory alignment.
    // const lx_protocol_header_t *header2 = (lx_protocol_header_t *)buf;

    const uint32_t payload_len = header.size - sizeof(lx_protocol_header_t);

    // Check payload size
    if (validate_payload_size(header.type, payload_len) != LX_OK) {
        return LX_ERR;
    }

    message_out->type = header.type;
    message_out->payload = buf + sizeof(lx_protocol_header_t);
    return LX_OK;
}

void lx_client_poll(lx_client_t *client) {
    lx_event_t event = {.type = LX_EVENT_NONE, .data = NULL};

    uint32_t len;
    uint16_t port;
    uint32_t ip;
    const lx_err_t err = client->udp_rx(client->sock, client->rx_buf, RX_BUF_SZ,
                                        &len, &port, &ip);
    if (err == LX_TIMEOUT) {
        // No udp message for us, so no event
        return;
    }
    if (err != LX_OK) {
        event = (lx_event_t){.type = LX_EVENT_ERR, .data = (void *)err};
    } else {
        // We've got a udp packet
        lx_message_t message = {.type = LX_PACKET_NONE, .payload = NULL};

        if (decode_packet(client->rx_buf, len, &message) != LX_OK) {
            event = (lx_event_t){.type = LX_EVENT_ERR, .data = (void *)err};
        } else if (message.type == LX_PACKET_NONE) {
            // This must have been a non-lifx packet, ignore
        } else {
            event =
                (lx_event_t){.type = LX_EVENT_MSG_RX, .data = (void *)&message};
        }
    }

    if (event.type != LX_EVENT_NONE) {
        client->event_cb(event);
    }
}
