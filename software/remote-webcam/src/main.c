// Remote webcam node — ESP-NOW video receiver exposed as a USB UVC webcam.
//
//   rover-esp (camera) --ESP-NOW (MJPEG)--> remote-webcam --USB UVC--> Android
//
// The rover (ESPNowCam library) chunks each JPEG frame into ESP-NOW packets,
// each a nanopb `Frame { uint32 lenght; bytes data; }`. We reassemble the
// chunks (lenght==0 mid-frame, lenght==total on the final chunk) and hand the
// finished MJPEG frame to the USB host as-is — no decoding.
//
// Pure ESP-IDF (no Arduino): esp_now + nanopb + usb_device_uvc.
//
// This board MAC:   98:3D:AE:60:84:C0
// Sender (rover):   DC:DA:0C:57:59:C8

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_now.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "pb_decode.h"
#include "frame.pb.h"
#include "usb_device_uvc.h"

static const char *TAG = "remote-webcam";

#define FB_CAP          (50 * 1024)  // max expected QVGA MJPEG frame
#define ESPNOW_PKT_MAX  250          // ESP-NOW max payload

static uint8_t *assemblyBuf;      // chunks of the in-progress frame append here
static uint32_t fbpos = 0;        // write position in assemblyBuf
static uint8_t *frameBuf[2];      // double buffer of complete frames
static uint8_t *uvcXferBuf;       // UVC transfer buffer (USB DMA capable)

static volatile int latestIdx = -1;   // newest complete frame
static volatile uint32_t latestLen = 0;
static volatile int uvcHolding = -1;  // buffer currently handed to the UVC stack
static portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

static uvc_fb_t s_uvc_fb;
static uint8_t recv_pkt[ESPNOW_PKT_MAX];

// ===== ESP-NOW receive + nanopb reassembly =====

// nanopb callback for Frame.data: append this chunk's bytes to assemblyBuf.
static bool decode_data(pb_istream_t *stream, const pb_field_t *field, void **arg) {
  size_t n = stream->bytes_left;
  if (fbpos + n > FB_CAP) {
    return false;
  }
  if (!pb_read(stream, assemblyBuf + fbpos, n)) {
    return false;
  }
  fbpos += n;
  return true;
}

// A full frame is assembled — publish it for the UVC task to pick up.
static void on_frame_complete(uint32_t length) {
  if (length == 0 || length > FB_CAP) {
    return;
  }
  int target;
  portENTER_CRITICAL(&mux);
  target = (uvcHolding == 0) ? 1 : 0;  // don't overwrite what UVC is reading
  portEXIT_CRITICAL(&mux);

  memcpy(frameBuf[target], assemblyBuf, length);

  portENTER_CRITICAL(&mux);
  latestIdx = target;
  latestLen = length;
  portEXIT_CRITICAL(&mux);
}

static void espnow_recv_cb(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  if (len <= 0 || len > ESPNOW_PKT_MAX) {
    return;
  }
  memcpy(recv_pkt, data, len);

  Frame msg = Frame_init_zero;
  msg.data.funcs.decode = decode_data;
  pb_istream_t stream = pb_istream_from_buffer(recv_pkt, len);
  if (pb_decode(&stream, Frame_fields, &msg)) {
    if (msg.lenght > 0) {  // final chunk: frame complete
      on_frame_complete(msg.lenght);
      fbpos = 0;
    }
  } else {
    fbpos = 0;  // drop a corrupt/partial frame and resync
  }
}

// ===== UVC callbacks (called from the UVC task) =====
static esp_err_t uvc_start_cb(uvc_format_t format, int width, int height, int rate, void *ctx) {
  ESP_LOGI(TAG, "UVC start: format=%d %dx%d @%dfps", format, width, height, rate);
  return (format == UVC_FORMAT_JPEG) ? ESP_OK : ESP_ERR_NOT_SUPPORTED;
}

static uvc_fb_t *uvc_fb_get_cb(void *ctx) {
  int idx;
  uint32_t len;
  portENTER_CRITICAL(&mux);
  idx = latestIdx;
  len = latestLen;
  if (idx >= 0) {
    uvcHolding = idx;
  }
  portEXIT_CRITICAL(&mux);

  if (idx < 0) {
    return NULL;  // no frame received yet
  }

  s_uvc_fb.buf = frameBuf[idx];
  s_uvc_fb.len = len;
  s_uvc_fb.width = 320;
  s_uvc_fb.height = 240;
  s_uvc_fb.format = UVC_FORMAT_JPEG;
  s_uvc_fb.timestamp.tv_sec = 0;
  s_uvc_fb.timestamp.tv_usec = 0;
  return &s_uvc_fb;
}

static void uvc_fb_return_cb(uvc_fb_t *fb, void *ctx) {
  portENTER_CRITICAL(&mux);
  uvcHolding = -1;
  portEXIT_CRITICAL(&mux);
}

static void uvc_stop_cb(void *ctx) {
  ESP_LOGI(TAG, "UVC stop");
}

// ===== Wi-Fi / ESP-NOW bring-up =====
static void wifi_init(void) {
  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));
  ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_start());
}

void app_main(void) {
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  assemblyBuf = heap_caps_malloc(FB_CAP, MALLOC_CAP_SPIRAM);
  frameBuf[0] = heap_caps_malloc(FB_CAP, MALLOC_CAP_SPIRAM);
  frameBuf[1] = heap_caps_malloc(FB_CAP, MALLOC_CAP_SPIRAM);
  uvcXferBuf = heap_caps_malloc(FB_CAP, MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA);
  if (!assemblyBuf || !frameBuf[0] || !frameBuf[1] || !uvcXferBuf) {
    ESP_LOGE(TAG, "buffer allocation failed");
    return;
  }

  wifi_init();
  ESP_ERROR_CHECK(esp_now_init());
  ESP_ERROR_CHECK(esp_now_register_recv_cb(espnow_recv_cb));

  uvc_device_config_t config = {
      .uvc_buffer = uvcXferBuf,
      .uvc_buffer_size = FB_CAP,
      .start_cb = uvc_start_cb,
      .fb_get_cb = uvc_fb_get_cb,
      .fb_return_cb = uvc_fb_return_cb,
      .stop_cb = uvc_stop_cb,
      .cb_ctx = NULL,
  };
  ESP_ERROR_CHECK(uvc_device_config(0, &config));
  ESP_ERROR_CHECK(uvc_device_init());

  ESP_LOGI(TAG, "remote-webcam ready (UVC + ESP-NOW)");
}
