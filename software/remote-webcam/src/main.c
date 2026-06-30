// Controller node — joystick transmitter + ESP-NOW video receiver + USB UVC.
//
//   controller (this) --ESP-NOW joystick--> robot
//   robot --ESP-NOW MJPEG video--> controller --USB UVC--> Android
//
// Combines the remote-simple joystick controller with the remote-webcam video
// path. Pure ESP-IDF (the USB UVC stack needs IDF 5.x and ESPNowCam doesn't
// build there), so the joystick is read and sent with the native esp_adc /
// esp_now APIs instead of Arduino + ESPNowW.

#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_now.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "pb_decode.h"
#include "frame.pb.h"
#include "usb_device_uvc.h"

static const char *TAG = "controller";

// Rover MAC (peer) — joystick frames are sent here. Set to your rover's MAC.
static uint8_t robot_mac[6] = {0x98, 0x3D, 0xAE, 0x60, 0x84, 0xC0};

// ===== Joystick / button pins (Xiao ESP32-S3) =====
#define JOYSTICK_X_CH    ADC_CHANNEL_0   // GPIO1
#define JOYSTICK_Y_CH    ADC_CHANNEL_1   // GPIO2
#define BTN_UP_PIN       3   // A
#define BTN_DOWN_PIN     5   // C
#define BTN_LEFT_PIN     6   // D
#define BTN_RIGHT_PIN    4   // B
#define BTN_E_PIN        9
#define BTN_F_PIN        8
#define BTN_JOYSTICK_PIN 7

// Button that toggles the robot camera on/off (change freely).
#define CAMERA_TOGGLE_PIN BTN_E_PIN

#define BTN_MASK ((1ULL << BTN_UP_PIN) | (1ULL << BTN_DOWN_PIN) | (1ULL << BTN_LEFT_PIN) | \
                  (1ULL << BTN_RIGHT_PIN) | (1ULL << BTN_E_PIN) | (1ULL << BTN_F_PIN) |    \
                  (1ULL << BTN_JOYSTICK_PIN))

// Control packet sent to the robot (raw struct, same layout both ends).
typedef struct {
  int16_t x;            // -1000..1000
  int16_t y;            // -1000..1000
  uint8_t btn_up;
  uint8_t btn_down;
  uint8_t btn_left;
  uint8_t btn_right;
  uint8_t btn_e;
  uint8_t btn_f;
  uint8_t btn_joystick;
  uint8_t camera_on;    // 1 = robot should stream video, 0 = stop
} JoystickData;

static volatile bool camera_on = false;  // default OFF (controller owns this — it
                                         // also gates the local UVC stream)

// ===== Joystick conditioning (ported from remote-simple) =====
static const int ADC_SAMPLES = 64;     // oversampling per axis
static const int OUTPUT_DEADZONE = 80; // in -1000..1000 units

typedef struct {
  int mn;
  int center;
  int mx;
} AxisCal;
static AxisCal calX = {0, 1886, 4095};
static AxisCal calY = {0, 1921, 4095};

static adc_oneshot_unit_handle_t adc1;

static int64_t now_ms(void) { return esp_timer_get_time() / 1000; }

static int read_axis(adc_channel_t ch) {
  long sum = 0;
  int raw = 0;
  for (int i = 0; i < ADC_SAMPLES; i++) {
    adc_oneshot_read(adc1, ch, &raw);
    sum += raw;
  }
  return (int)(sum / ADC_SAMPLES);
}

static int map_axis(int raw, const AxisCal *c) {
  long out;
  if (raw >= c->center) {
    int span = c->mx - c->center;
    out = (span > 0) ? (long)(raw - c->center) * 1000 / span : 0;
  } else {
    int span = c->center - c->mn;
    out = (span > 0) ? (long)(raw - c->center) * 1000 / span : 0;
  }
  if (out > 1000) out = 1000;
  if (out < -1000) out = -1000;
  return (int)out;
}

static void load_calibration(void) {
  nvs_handle_t h;
  if (nvs_open("joycal2", NVS_READONLY, &h) != ESP_OK) {
    ESP_LOGI(TAG, "No saved calibration; using defaults");
    return;
  }
  int32_t v;
  if (nvs_get_i32(h, "xc", &v) == ESP_OK) {
    calX.center = v;
    if (nvs_get_i32(h, "xmn", &v) == ESP_OK) calX.mn = v;
    if (nvs_get_i32(h, "xmx", &v) == ESP_OK) calX.mx = v;
    if (nvs_get_i32(h, "yc", &v) == ESP_OK) calY.center = v;
    if (nvs_get_i32(h, "ymn", &v) == ESP_OK) calY.mn = v;
    if (nvs_get_i32(h, "ymx", &v) == ESP_OK) calY.mx = v;
    ESP_LOGI(TAG, "Loaded calibration from NVS");
  }
  nvs_close(h);
}

static void save_calibration(void) {
  nvs_handle_t h;
  if (nvs_open("joycal2", NVS_READWRITE, &h) != ESP_OK) return;
  nvs_set_i32(h, "xmn", calX.mn);
  nvs_set_i32(h, "xc", calX.center);
  nvs_set_i32(h, "xmx", calX.mx);
  nvs_set_i32(h, "ymn", calY.mn);
  nvs_set_i32(h, "yc", calY.center);
  nvs_set_i32(h, "ymx", calY.mx);
  nvs_commit(h);
  nvs_close(h);
  ESP_LOGI(TAG, "Calibration saved to NVS");
}

// Two-phase calibration: hold centered, then sweep extremes.
static void calibrate_joystick(void) {
  ESP_LOGI(TAG, "Calibration: keep joystick CENTERED...");
  vTaskDelay(pdMS_TO_TICKS(800));

  long sx = 0, sy = 0;
  int n = 0;
  int64_t t0 = now_ms();
  while (now_ms() - t0 < 5000) {
    sx += read_axis(JOYSTICK_X_CH);
    sy += read_axis(JOYSTICK_Y_CH);
    n++;
    vTaskDelay(pdMS_TO_TICKS(10));
  }
  calX.center = sx / n;
  calY.center = sy / n;

  ESP_LOGI(TAG, "Now SWEEP to all extremes for 5s...");
  calX.mn = calY.mn = 4095;
  calX.mx = calY.mx = 0;
  t0 = now_ms();
  while (now_ms() - t0 < 5000) {
    int x = read_axis(JOYSTICK_X_CH);
    int y = read_axis(JOYSTICK_Y_CH);
    if (x < calX.mn) calX.mn = x;
    if (x > calX.mx) calX.mx = x;
    if (y < calY.mn) calY.mn = y;
    if (y > calY.mx) calY.mx = y;
    vTaskDelay(pdMS_TO_TICKS(5));
  }
  ESP_LOGI(TAG, "Calibration done: X[%d,%d,%d] Y[%d,%d,%d]",
           calX.mn, calX.center, calX.mx, calY.mn, calY.center, calY.mx);
  save_calibration();
}

static inline int btn_pressed(int pin) { return gpio_get_level(pin) == 0; }  // pull-up: pressed = LOW

// ===== Joystick TX task =====
static void control_task(void *arg) {
  bool cam_btn_prev = false;
  for (;;) {
    int x_raw = read_axis(JOYSTICK_X_CH);
    int y_raw = read_axis(JOYSTICK_Y_CH);
    int cx = map_axis(x_raw, &calX);
    int cy = map_axis(y_raw, &calY);
    if (abs(cx) < OUTPUT_DEADZONE) cx = 0;
    if (abs(cy) < OUTPUT_DEADZONE) cy = 0;

    // Camera toggle on button press edge (E).
    bool cam_btn = btn_pressed(CAMERA_TOGGLE_PIN);
    if (cam_btn && !cam_btn_prev) {
      camera_on = !camera_on;
      ESP_LOGI(TAG, "camera_on = %d", (int)camera_on);
    }
    cam_btn_prev = cam_btn;

    JoystickData jd = {
        .x = (int16_t)cx,
        .y = (int16_t)cy,
        .btn_up = btn_pressed(BTN_UP_PIN),
        .btn_down = btn_pressed(BTN_DOWN_PIN),
        .btn_left = btn_pressed(BTN_LEFT_PIN),
        .btn_right = btn_pressed(BTN_RIGHT_PIN),
        .btn_e = btn_pressed(BTN_E_PIN),
        .btn_f = btn_pressed(BTN_F_PIN),
        .btn_joystick = btn_pressed(BTN_JOYSTICK_PIN),
        .camera_on = camera_on ? 1 : 0,
    };
    esp_now_send(robot_mac, (uint8_t *)&jd, sizeof(jd));

    // Hold the joystick button for 5 s to enter calibration. The long hold
    // avoids accidental triggers, and the fixed 5 s + 5 s phases let you do it
    // by timing alone (no serial monitor needed: this board's USB is the UVC
    // camera, so its log isn't easily visible).
    static int64_t joy_press_start = 0;
    static bool joy_prev = false;
    bool joy = btn_pressed(BTN_JOYSTICK_PIN);
    if (joy && !joy_prev) joy_press_start = now_ms();
    if (joy && now_ms() - joy_press_start >= 5000) {
      calibrate_joystick();
      while (btn_pressed(BTN_JOYSTICK_PIN)) vTaskDelay(pdMS_TO_TICKS(20));
      joy = false;
    }
    joy_prev = joy;

    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

// ===== Video RX (ESP-NOW + nanopb) =====
#define FB_CAP          (50 * 1024)
#define ESPNOW_PKT_MAX  250

static uint8_t *assemblyBuf;
static uint32_t fbpos = 0;
static uint8_t *frameBuf[2];
static uint8_t *uvcXferBuf;
static volatile int latestIdx = -1;
static volatile uint32_t latestLen = 0;
static volatile int uvcHolding = -1;
static portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
static uvc_fb_t s_uvc_fb;
static uint8_t recv_pkt[ESPNOW_PKT_MAX];

static bool decode_data(pb_istream_t *stream, const pb_field_t *field, void **arg) {
  size_t n = stream->bytes_left;
  if (fbpos + n > FB_CAP) return false;
  if (!pb_read(stream, assemblyBuf + fbpos, n)) return false;
  fbpos += n;
  return true;
}

static void on_frame_complete(uint32_t length) {
  if (length == 0 || length > FB_CAP) return;
  int target;
  portENTER_CRITICAL(&mux);
  target = (uvcHolding == 0) ? 1 : 0;
  portEXIT_CRITICAL(&mux);
  memcpy(frameBuf[target], assemblyBuf, length);
  portENTER_CRITICAL(&mux);
  latestIdx = target;
  latestLen = length;
  portEXIT_CRITICAL(&mux);
}

static void espnow_recv_cb(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  // Only the robot's video frames are expected here. Skip when the camera is
  // off so we don't reassemble anything during idle.
  if (!camera_on || len <= 0 || len > ESPNOW_PKT_MAX) return;
  memcpy(recv_pkt, data, len);
  Frame msg = Frame_init_zero;
  msg.data.funcs.decode = decode_data;
  pb_istream_t stream = pb_istream_from_buffer(recv_pkt, len);
  if (pb_decode(&stream, Frame_fields, &msg)) {
    if (msg.lenght > 0) {
      on_frame_complete(msg.lenght);
      fbpos = 0;
    }
  } else {
    fbpos = 0;
  }
}

// ===== UVC callbacks =====
static esp_err_t uvc_start_cb(uvc_format_t format, int width, int height, int rate, void *ctx) {
  ESP_LOGI(TAG, "UVC start: format=%d %dx%d @%dfps", format, width, height, rate);
  return (format == UVC_FORMAT_JPEG) ? ESP_OK : ESP_ERR_NOT_SUPPORTED;
}

static uvc_fb_t *uvc_fb_get_cb(void *ctx) {
  if (!camera_on) return NULL;  // idle: don't re-send stale frames

  int idx;
  uint32_t len;
  portENTER_CRITICAL(&mux);
  idx = latestIdx;
  len = latestLen;
  if (idx >= 0) uvcHolding = idx;
  portEXIT_CRITICAL(&mux);

  if (idx < 0) return NULL;

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

static void uvc_stop_cb(void *ctx) { ESP_LOGI(TAG, "UVC stop"); }

// ===== Init =====
static void wifi_init(void) {
  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));
  ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));  // steadier link/ADC
  ESP_ERROR_CHECK(esp_wifi_start());
}

static void adc_init(void) {
  adc_oneshot_unit_init_cfg_t init_cfg = {.unit_id = ADC_UNIT_1};
  ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_cfg, &adc1));
  adc_oneshot_chan_cfg_t chan_cfg = {.atten = ADC_ATTEN_DB_12, .bitwidth = ADC_BITWIDTH_12};
  ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1, JOYSTICK_X_CH, &chan_cfg));
  ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1, JOYSTICK_Y_CH, &chan_cfg));
}

static void buttons_init(void) {
  gpio_config_t io = {
      .pin_bit_mask = BTN_MASK,
      .mode = GPIO_MODE_INPUT,
      .pull_up_en = GPIO_PULLUP_ENABLE,
      .pull_down_en = GPIO_PULLDOWN_DISABLE,
      .intr_type = GPIO_INTR_DISABLE,
  };
  ESP_ERROR_CHECK(gpio_config(&io));
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

  adc_init();
  buttons_init();
  load_calibration();

  wifi_init();
  ESP_ERROR_CHECK(esp_now_init());
  ESP_ERROR_CHECK(esp_now_register_recv_cb(espnow_recv_cb));

  esp_now_peer_info_t peer = {0};
  memcpy(peer.peer_addr, robot_mac, 6);
  peer.channel = 0;
  peer.ifidx = WIFI_IF_STA;
  peer.encrypt = false;
  ESP_ERROR_CHECK(esp_now_add_peer(&peer));

  // USB UVC device.
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

  xTaskCreate(control_task, "control", 4096, NULL, 5, NULL);

  ESP_LOGI(TAG, "controller ready (joystick TX + UVC video RX)");
}
