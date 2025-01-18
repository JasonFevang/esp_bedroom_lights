idf_component_register(SRCS "src/esp_lifx.c" "src/lifx/structs.c"
                    INCLUDE_DIRS "src"
                    REQUIRES esp_event esp_netif nvs_flash)

