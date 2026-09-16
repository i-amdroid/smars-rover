idf_component_register(
    SRCS "main.c" "frame.pb.c" "pb_common.c" "pb_decode.c"
    INCLUDE_DIRS "."
    REQUIRES usb_device_uvc esp_wifi esp_netif esp_event nvs_flash esp_adc driver esp_timer
)
