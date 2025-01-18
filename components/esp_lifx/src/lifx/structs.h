#ifndef STRUCTS_H
#define STRUCTS_H

#include <assert.h>
#include <stdint.h>

#pragma pack(push, 1)
typedef struct {
    /* frame */
    uint16_t size;           // Size of the entire message
    uint16_t protocol : 12;  // Must be 1024d
    uint8_t addressable : 1; // Message includes a target address, must be 1
    uint8_t tagged : 1;      // Determines if Frame Address target field is
                             // addressing one or all devices
    uint8_t origin : 2;      // Message origin indicator, must be 0
    uint32_t source;         // Source identifier. Unique value set by client
    /* frame address */
    uint8_t target[8]; // Serial number of device, or 0 for all devices
    uint8_t reserved[6];
    uint8_t res_required : 1; // State message required. Always 0
    uint8_t ack_required : 1;
    uint8_t : 6;
    uint8_t sequence; // Wrap around message sequence number
    /* protocol header */
    uint64_t : 64;
    uint16_t type; // Message type, determines payload
    uint16_t : 16;
    /* variable length payload follows */
} lx_protocol_header_t;
#pragma pack(pop)

// If no pragam pack(1), then size is 40B
static_assert(sizeof(lx_protocol_header_t) == 36,
              "Incorrect lx_protocol_header_t size");

void lx_protocol_header_print(const lx_protocol_header_t *header,
                              uint32_t payload_len);

#pragma pack(push, 1)
typedef struct {
    uint8_t service; // using lx_services_t enum
    uint32_t port;   // The port of the service. This value is usually 56700 but
                     // you should not assume this is always the case.
} lx_state_service_payload_t;
#pragma pack(pop)

void lx_payload_state_service_print(const lx_state_service_payload_t *payload);

#endif // STRUCTS_H
