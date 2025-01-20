#include "packets.h"
#include <stdio.h>

void lx_protocol_header_print(const lx_protocol_header_t *header,
                              uint32_t payload_len) {
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
    printf("  Pay load size: %ld\n", payload_len);
}

void lx_payload_state_service_print(const lx_state_service_payload_t *payload) {
    printf("lx_state_service_payload_t:\n");
    printf("  Service: %u\n", payload->service);
    printf("  port: %lu\n", payload->port);
}
