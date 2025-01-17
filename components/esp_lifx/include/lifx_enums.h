#ifndef LIFX_ENUMS_H
#define LIFX_ENUMS_H

#include <assert.h>
#include <stdint.h>

typedef enum {
    LX_SERVICES_UDP = 1,
    LX_SERVICES_RESERVED1 = 2,
    LX_SERVICES_RESERVED2 = 3,
    LX_SERVICES_RESERVED3 = 4,
    LX_SERVICES_RESERVED4 = 5
} lx_services_t;

typedef enum {
    LX_PACKET_GET_SERVICE = 2,
    LX_PACKET_STATE_SERVICE = 3
} lx_packet_t;

#endif // LIFX_ENUMS_H
