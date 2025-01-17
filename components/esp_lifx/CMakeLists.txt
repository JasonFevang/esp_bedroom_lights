idf_component_register(SRCS "esp_lifx.c" "lifx_structs.c"
                    INCLUDE_DIRS "include"
                    REQUIRES esp_event esp_netif nvs_flash)

