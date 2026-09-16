#include <Arduino.h>
#include <WiFi.h>
#include "ESPNowW.h"
// #include "driver/rtc_io.h"  // only needed for deep sleep (disabled below)
#include <Preferences.h>

// EPS32-S3 uno: EC:DA:3B:51:19:0C
uint8_t receiver_mac[] = {0xEC, 0xDA, 0x3B, 0x51, 0x19, 0x0C};

// Xiao ESP32-S3 (without battery connector): 98:3D:AE:60:84:C0
// uint8_t receiver_mac[] = {0x98, 0x3D, 0xAE, 0x60, 0x84, 0xC0};

// Joystick pins for Xiao EPS32-S3.
#define JOYSTICK_X_PIN 1
#define JOYSTICK_Y_PIN 2
#define BTN_UP_PIN     3 // A
#define BTN_DOWN_PIN   5 // C
#define BTN_LEFT_PIN   6 // D
#define BTN_RIGHT_PIN  4 // B
#define BTN_E_PIN      9
#define BTN_F_PIN      8 // Sleep
#define BTN_JOYSTICK_PIN 7

// Joystick pins for EPS32-S3 uno.
// #define JOYSTICK_X_PIN 2
// #define JOYSTICK_Y_PIN 1
// #define BTN_UP_PIN     18
// #define BTN_DOWN_PIN   19
// #define BTN_LEFT_PIN   20
// #define BTN_RIGHT_PIN  17
// #define BTN_E_PIN      3
// #define BTN_F_PIN      14 // Sleep. Check pin supports RTC wakeup!
// #define BTN_JOYSTICK_PIN 21

// Set to 1 and open the serial monitor to print raw + mapped values for tuning.
#define DEBUG_JOYSTICK 0

struct JoystickData {
  int16_t x;           // X axis value (-1000 to 1000).
  int16_t y;           // Y axis value  (-1000 to 1000).
  uint8_t btn_up;      // Up button (0 or 1).
  uint8_t btn_down;    // Down button (0 or 1).
  uint8_t btn_left;    // Left button (0 or 1).
  uint8_t btn_right;   // Right button (0 or 1).
  uint8_t btn_e;       // E button (0 or 1).
  uint8_t btn_f;       // F button (0 or 1).
  uint8_t btn_joystick; // Joystick press button (0 or 1).
} joystick_data;

// ---------------------------------------------------------------------------
// Signal conditioning.
//
// Each axis is the mean of ADC_SAMPLES raw analogRead values (oversampling
// removes transient noise spikes with NO lag), mapped to -1000..1000 against
// the calibrated min/center/max, then a generous center deadzone snaps the
// output to a hard 0 at rest. No time-domain low-pass filter on purpose: a
// low-pass makes the output coast after release (motor creep/whine).
//
// OUTPUT_DEADZONE must be wider than the near-center noise swing, otherwise the
// value crosses zero at rest and the receiver flips both tracks' direction.
// ---------------------------------------------------------------------------
const int ADC_SAMPLES     = 64;    // oversampling per axis per loop
const int OUTPUT_DEADZONE = 80;    // in -1000..1000 units, zeroed near center

// Per-axis calibration in raw ADC counts (0-4095). Defaults are measured values
// for the "Xiao ESP32-S3 with battery connector" setup; overwritten by NVS.
struct AxisCal {
  int mn;      // counts at one extreme
  int center;  // counts at rest
  int mx;      // counts at the other extreme
};
AxisCal calX = { 0, 1886, 4095 };
AxisCal calY = { 0, 1921, 4095 };

Preferences prefs;  // NVS storage for calibration.

// Average ADC_SAMPLES raw reads from one axis.
int readAxis(int pin) {
  long sum = 0;
  for (int i = 0; i < ADC_SAMPLES; i++) {
    sum += analogRead(pin);
  }
  return sum / ADC_SAMPLES;
}

// Map a raw reading to -1000..1000 using the axis calibration, scaling each
// side of center independently so the full travel is used symmetrically.
int mapAxis(int raw, const AxisCal &c) {
  long out;
  if (raw >= c.center) {
    int span = c.mx - c.center;
    out = (span > 0) ? (long)(raw - c.center) * 1000 / span : 0;
  } else {
    int span = c.center - c.mn;
    out = (span > 0) ? (long)(raw - c.center) * 1000 / span : 0;
  }
  return constrain((int)out, -1000, 1000);
}

void loadCalibration() {
  prefs.begin("joycal2", true);  // read-only (v2 namespace: raw counts)
  if (prefs.isKey("xc")) {
    calX.mn = prefs.getInt("xmn", calX.mn);
    calX.center = prefs.getInt("xc", calX.center);
    calX.mx = prefs.getInt("xmx", calX.mx);
    calY.mn = prefs.getInt("ymn", calY.mn);
    calY.center = prefs.getInt("yc", calY.center);
    calY.mx = prefs.getInt("ymx", calY.mx);
    Serial.println("Loaded calibration from NVS.");
  } else {
    Serial.println("No saved calibration; using defaults.");
  }
  prefs.end();
}

void saveCalibration() {
  prefs.begin("joycal2", false);  // read-write
  prefs.putInt("xmn", calX.mn);
  prefs.putInt("xc",  calX.center);
  prefs.putInt("xmx", calX.mx);
  prefs.putInt("ymn", calY.mn);
  prefs.putInt("yc",  calY.center);
  prefs.putInt("ymx", calY.mx);
  prefs.end();
  Serial.println("Calibration saved to NVS.");
}

// Two-phase calibration:
//   1) keep the stick centered  -> measures each axis center
//   2) sweep the stick to every extreme -> captures min/max per axis
// Results are stored in NVS so they survive power cycles.
void calibrateJoystick() {
  Serial.println("Calibration: release the joystick and keep it CENTERED...");
  delay(800);  // let the user release and the stick settle

  // Phase 1: center (average over ~1.5s).
  const int CENTER_MS = 1500;
  long sum_x = 0, sum_y = 0;
  int n = 0;
  unsigned long t0 = millis();
  while (millis() - t0 < CENTER_MS) {
    sum_x += readAxis(JOYSTICK_X_PIN);
    sum_y += readAxis(JOYSTICK_Y_PIN);
    n++;
    delay(10);
  }
  calX.center = sum_x / n;
  calY.center = sum_y / n;

  // Phase 2: sweep extremes (track min/max over ~5s).
  Serial.println("Now SWEEP the joystick to all extremes for 5 seconds...");
  calX.mn = calY.mn = 4095;
  calX.mx = calY.mx = 0;
  t0 = millis();
  while (millis() - t0 < 5000) {
    int x = readAxis(JOYSTICK_X_PIN);
    int y = readAxis(JOYSTICK_Y_PIN);
    if (x < calX.mn) calX.mn = x;
    if (x > calX.mx) calX.mx = x;
    if (y < calY.mn) calY.mn = y;
    if (y > calY.mx) calY.mx = y;
    delay(5);
  }

  Serial.println("Calibration complete (raw counts):");
  Serial.printf("  X: min %d  center %d  max %d\n", calX.mn, calX.center, calX.mx);
  Serial.printf("  Y: min %d  center %d  max %d\n", calY.mn, calY.center, calY.mx);

  saveCalibration();
}

void handleJoystickButton() {
  if(digitalRead(BTN_JOYSTICK_PIN) == LOW) {
    delay(50);  // Debounce
    if(digitalRead(BTN_JOYSTICK_PIN) == LOW) {
      calibrateJoystick();
      // Wait for release so we don't immediately re-trigger.
      while (digitalRead(BTN_JOYSTICK_PIN) == LOW) delay(10);
    }
  }
}

// Deep sleep disabled for this instance. The F button is still reported as a
// normal button (joystick_data.btn_f); it just no longer triggers sleep.
// To re-enable: uncomment this function, its call in loop(), and the
// "driver/rtc_io.h" include at the top of the file.
/*
void handleFButton() {
  if(digitalRead(BTN_F_PIN) == LOW) {
    delay(50);  // Debounce
    if(digitalRead(BTN_F_PIN) == LOW) {
      // Wait for button release before sleeping
      while(digitalRead(BTN_F_PIN) == LOW) {
        delay(10);
      }
      delay(200); // Extra delay to ensure button is fully released

      Serial.println("Entering deep sleep...");
      Serial.flush(); // Ensure message is sent before sleep

      // Configure GPIO for RTC wakeup
      gpio_num_t wake_pin = (gpio_num_t)BTN_F_PIN;

      // Check if this is an RTC-capable GPIO
      if (rtc_gpio_is_valid_gpio(wake_pin)) {
        // Deinitialize first to ensure clean state
        rtc_gpio_deinit(wake_pin);

        // Initialize RTC GPIO
        rtc_gpio_init(wake_pin);
        rtc_gpio_set_direction(wake_pin, RTC_GPIO_MODE_INPUT_ONLY);
        rtc_gpio_pullup_en(wake_pin);
        rtc_gpio_pulldown_dis(wake_pin);
        rtc_gpio_hold_en(wake_pin); // Hold the GPIO state during sleep

        // Enable wakeup on LOW (button press) using ext0
        esp_sleep_enable_ext0_wakeup(wake_pin, 0);

        Serial.println("RTC GPIO configured, entering sleep NOW");
        Serial.flush();
      } else {
        Serial.println("ERROR: Sleep GPIO not RTC-capable!");
        return; // Don't enter sleep if wakeup won't work
      }

      esp_deep_sleep_start();
    }
  }
}
*/

void setup() {
  Serial.begin(115200);
  delay(500); // Give serial time to initialize

  // ADC configuration: 12-bit, ~0-3.1/3.3V input range.
  analogReadResolution(12);
  analogSetPinAttenuation(JOYSTICK_X_PIN, ADC_11db);
  analogSetPinAttenuation(JOYSTICK_Y_PIN, ADC_11db);

  // Initialize joystick pins.
  pinMode(JOYSTICK_X_PIN, INPUT);
  pinMode(JOYSTICK_Y_PIN, INPUT);
  pinMode(BTN_UP_PIN, INPUT_PULLUP);
  pinMode(BTN_DOWN_PIN, INPUT_PULLUP);
  pinMode(BTN_LEFT_PIN, INPUT_PULLUP);
  pinMode(BTN_RIGHT_PIN, INPUT_PULLUP);
  pinMode(BTN_E_PIN, INPUT_PULLUP);
  pinMode(BTN_F_PIN, INPUT_PULLUP);
  pinMode(BTN_JOYSTICK_PIN, INPUT_PULLUP);

  loadCalibration();

  WiFi.mode(WIFI_MODE_STA);
  WiFi.disconnect();
  WiFi.setSleep(false);  // disable modem-sleep -> steadier ADC (less periodic noise)
  ESPNow.init();
  ESPNow.add_peer(receiver_mac);
}

void loop() {
  // Oversampled raw reads (no time filtering -> instant response).
  int x_raw = readAxis(JOYSTICK_X_PIN);
  int y_raw = readAxis(JOYSTICK_Y_PIN);

  // Map to -1000..1000 against the per-axis calibration.
  int centered_x = mapAxis(x_raw, calX);
  int centered_y = mapAxis(y_raw, calY);

  // Center deadzone: snap to a solid 0 at rest so releasing the stick stops the
  // robot immediately and near-center noise can't cross zero.
  if (abs(centered_x) < OUTPUT_DEADZONE) centered_x = 0;
  if (abs(centered_y) < OUTPUT_DEADZONE) centered_y = 0;

#if DEBUG_JOYSTICK
  static unsigned long last_dbg = 0;
  if (millis() - last_dbg > 200) {
    last_dbg = millis();
    Serial.printf("X raw=%4d -> %5d  |  Y raw=%4d -> %5d\n",
                  x_raw, centered_x, y_raw, centered_y);
  }
#endif

  joystick_data.x = centered_x;
  joystick_data.y = centered_y;
  joystick_data.btn_up = !digitalRead(BTN_UP_PIN);
  joystick_data.btn_down = !digitalRead(BTN_DOWN_PIN);
  joystick_data.btn_left = !digitalRead(BTN_LEFT_PIN);
  joystick_data.btn_right = !digitalRead(BTN_RIGHT_PIN);
  joystick_data.btn_e = !digitalRead(BTN_E_PIN);
  joystick_data.btn_f = !digitalRead(BTN_F_PIN);
  joystick_data.btn_joystick = !digitalRead(BTN_JOYSTICK_PIN);

  // Send joystick and button data via ESP-NOW.
  ESPNow.send_message(receiver_mac, (uint8_t*)&joystick_data, sizeof(joystick_data));

  handleJoystickButton();
  // handleFButton();  // deep sleep disabled for this instance

  delay(50);
}
