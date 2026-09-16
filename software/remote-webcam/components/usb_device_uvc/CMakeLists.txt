idf_component_register(SRCS usb_device_uvc.c
                    INCLUDE_DIRS "include"
                    REQUIRES usb esp_timer)

idf_component_get_property(tusb_lib espressif__tinyusb COMPONENT_LIB)

target_include_directories(${tusb_lib} PUBLIC "${COMPONENT_DIR}/tusb")
# NOTE: PRIVATE (not PUBLIC). PUBLIC propagates this source to every consumer of
# the tinyusb lib, so it gets compiled more than once — which breaks PlatformIO's
# ESP-IDF builder ("Two environments ... for the same target"). PRIVATE compiles
# it once in the tinyusb lib; the descriptor symbols are still linked into the app.
target_sources(${tusb_lib} PRIVATE "${COMPONENT_DIR}/tusb/usb_descriptors.c")

include(package_manager)
cu_pkg_define_version(${CMAKE_CURRENT_LIST_DIR})
