// Rover node — drive + manipulator + ESP-NOW camera streamer.
//
//   controller --ESP-NOW joystick--> rover (this)
//   rover --ESP-NOW MJPEG video--> controller
//
// Combines the camera/ESP-NOW streamer (rover-esp) with the drive + servo
// control logic (dozer-esp), but with a DRV8833 motor driver (no library — just
// PWM) and two servos. Arduino + stock espressif32: no USB UVC here, so no need
// for ESP-IDF; ESPNowCam (video) + ESP32Servo work out of the box.

#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <ESPNowCam.h>
#include <drivers/CamXiao.h>
#include <ESP32Servo.h>

// Controller MAC (peer) — video is sent here. Set to your controller's MAC.
static uint8_t controllerMac[6] = {0x98, 0x3D, 0xAE, 0x61, 0x72, 0x3C};

// ===== Pins (Xiao ESP32-S3, see wiring scheme) =====
#define SERVO1_PIN 1    // D0 — shoulder
#define SERVO2_PIN 2    // D1 — gripper
#define DRV_IN1    43   // D6 \ motor 1
#define DRV_IN2    5    // D4 /
#define DRV_IN3    4    // D3 \ motor 2
#define DRV_IN4    3    // D2 /
#define LED_PIN    44   // D7 — white LEDs via S8050

// LEDC: motors on channels 4-7 (timers 2-3); ESP32Servo gets timers 0-1.
#define M1A_CH 4
#define M1B_CH 5
#define M2A_CH 6
#define M2B_CH 7
#define MOTOR_PWM_FREQ 20000
#define MOTOR_PWM_RES  8     // 0-255

// ===== Movement tuning (from dozer-esp) =====
static const int MIN_SPEED = 65;
static const int MAX_SPEED = 120;
static const int DEADZONE = 10;
static const float MOTOR_CORRECTION = 1.0f;  // 1.0 = no trim; <1 trims right, >1 trims left

// ===== Servos =====
static Servo servo1, servo2;
// Shoulder ~0-90, gripper ~0-120 (tune freely; swap min/max to flip direction).
static const int S1_MIN = 0, S1_MAX = 90, S1_INIT = 0;
static const int S2_MIN = 0, S2_MAX = 120, S2_INIT = 0;
static const int SERVO_STEP_MS = 15;  // MG90S ~ this per degree
static volatile int s1_target = S1_INIT, s2_target = S2_INIT;

// ===== State driven by the controller =====
static volatile bool camera_on = false;  // default OFF
static volatile uint32_t lastRecvMs = 0;

// ===== Camera / video =====
CamXiao Camera;
ESPNowCam radio;
static bool cam_inited = false;
static const uint32_t FRAME_INTERVAL_MS = 60;  // ~camera ceiling; non-blocking pace
static uint32_t lastFrameMs = 0;

// Control packet — must match the controller's struct byte for byte.
typedef struct {
  int16_t x;
  int16_t y;
  uint8_t btn_up;       // A
  uint8_t btn_down;     // C
  uint8_t btn_left;     // D
  uint8_t btn_right;    // B
  uint8_t btn_e;
  uint8_t btn_f;
  uint8_t btn_joystick;
  uint8_t camera_on;    // robot streams video while set (controller-owned)
} JoystickData;

// ===== DRV8833 =====
static void setMotor(int chA, int chB, int speed, int dir) {
  if (speed < 0) speed = 0;
  if (speed > 255) speed = 255;
  if (dir > 0) {
    ledcWrite(chA, speed);
    ledcWrite(chB, 0);
  } else if (dir < 0) {
    ledcWrite(chA, 0);
    ledcWrite(chB, speed);
  } else {
    ledcWrite(chA, 0);
    ledcWrite(chB, 0);  // coast
  }
}

static void stopMotors(void) {
  setMotor(M1A_CH, M1B_CH, 0, 0);
  setMotor(M2A_CH, M2B_CH, 0, 0);
}

// Tank mixing (ported from dozer-esp). Direction: +1 fwd, -1 back, 0 stop.
static void calculateTankMovement(int16_t x, int16_t y, int &lSpeed, int &rSpeed, int &lDir, int &rDir) {
  if (abs(x) < DEADZONE) x = 0;
  if (abs(y) < DEADZONE) y = 0;

  if (x == 0 && y == 0) {
    lSpeed = rSpeed = 0;
    lDir = rDir = 0;
    return;
  }

  float xNorm = (float)x / 1000.0f;

  if (y == 0) {
    // Pure rotation in place.
    lDir = (x > 0) ? 1 : -1;
    rDir = (x > 0) ? -1 : 1;
    int turn = map(abs(x), DEADZONE, 1000, MIN_SPEED, MAX_SPEED);
    lSpeed = rSpeed = turn;
  } else {
    lDir = rDir = (y > 0) ? 1 : -1;
    int base = map(abs(y), DEADZONE, 1000, MIN_SPEED, MAX_SPEED);
    float steer = fabs(xNorm);
    if (x > 0) {
      lSpeed = base;
      rSpeed = base * (1.0f - steer * 0.6f);
    } else if (x < 0) {
      rSpeed = base;
      lSpeed = base * (1.0f - steer * 0.6f);
    } else {
      lSpeed = rSpeed = base;
    }
    if (lSpeed < MIN_SPEED && lSpeed > 0) lSpeed = MIN_SPEED;
    if (rSpeed < MIN_SPEED && rSpeed > 0) rSpeed = MIN_SPEED;
  }

  if (MOTOR_CORRECTION < 1.0f) rSpeed *= MOTOR_CORRECTION;
  else if (MOTOR_CORRECTION > 1.0f) lSpeed /= MOTOR_CORRECTION;

  lSpeed = constrain(lSpeed, 0, MAX_SPEED);
  rSpeed = constrain(rSpeed, 0, MAX_SPEED);
}

// ===== Joystick RX (raw esp_now; Arduino 2.x callback signature) =====
static void onJoystickRecv(const uint8_t *mac, const uint8_t *data, int len) {
  if (len != sizeof(JoystickData)) return;
  const JoystickData *j = (const JoystickData *)data;
  lastRecvMs = millis();

  // Drive.
  int lS, rS, lD, rD;
  calculateTankMovement(j->x, j->y, lS, rS, lD, rD);
  setMotor(M1A_CH, M1B_CH, lS, lD);   // motor 1 = left
  setMotor(M2A_CH, M2B_CH, rS, rD);   // motor 2 = right

  // Servo 1 (shoulder): A up, C down. Hold to move, release to hold position.
  if (j->btn_up)        s1_target = S1_MAX;
  else if (j->btn_down) s1_target = S1_MIN;
  // Servo 2 (gripper): B close, D open.
  if (j->btn_right)     s2_target = S2_MAX;
  else if (j->btn_left) s2_target = S2_MIN;

  // Camera state comes from the controller (it gates the controller's UVC too).
  camera_on = j->camera_on;

  // LED is a rover-local concern: toggle on the F button edge, like A/B/C/D.
  static uint8_t f_prev = 0;
  static bool led_state = false;
  if (j->btn_f && !f_prev) {
    led_state = !led_state;
    digitalWrite(LED_PIN, led_state ? HIGH : LOW);
  }
  f_prev = j->btn_f;
}

// ===== Servo stepping task (smooth, independent of the camera) =====
static void stepServo(Servo &s, int &cur, int target) {
  if (cur < target) s.write(++cur);
  else if (cur > target) s.write(--cur);
}

static void servo_task(void *arg) {
  int cur1 = S1_INIT, cur2 = S2_INIT;
  for (;;) {
    stepServo(servo1, cur1, s1_target);
    stepServo(servo2, cur2, s2_target);
    vTaskDelay(pdMS_TO_TICKS(SERVO_STEP_MS));
  }
}

// ===== Camera =====
static bool initCamera(void) {
  Camera.config.pixel_format = PIXFORMAT_JPEG;
  Camera.config.frame_size = FRAMESIZE_QVGA;
  Camera.config.jpeg_quality = 18;
  Camera.config.fb_count = 2;
  Camera.config.fb_location = CAMERA_FB_IN_DRAM;
  if (!Camera.begin()) {
    Serial.println("Camera init failed");
    return false;
  }
  return true;
}

static void deinitCamera(void) {
  esp_camera_deinit();
}

void setup() {
  Serial.begin(115200);
  delay(500);

  setCpuFrequencyMhz(160);  // heat/power: enough for streaming, cooler

  // Motors (DRV8833) via LEDC.
  ledcSetup(M1A_CH, MOTOR_PWM_FREQ, MOTOR_PWM_RES);
  ledcSetup(M1B_CH, MOTOR_PWM_FREQ, MOTOR_PWM_RES);
  ledcSetup(M2A_CH, MOTOR_PWM_FREQ, MOTOR_PWM_RES);
  ledcSetup(M2B_CH, MOTOR_PWM_FREQ, MOTOR_PWM_RES);
  ledcAttachPin(DRV_IN1, M1A_CH);
  ledcAttachPin(DRV_IN2, M1B_CH);
  ledcAttachPin(DRV_IN3, M2A_CH);
  ledcAttachPin(DRV_IN4, M2B_CH);
  stopMotors();

  // LED.
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // Servos — give ESP32Servo timers 0/1 (motors took 2/3 via channels 4-7).
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  servo1.setPeriodHertz(50);
  servo2.setPeriodHertz(50);
  servo1.attach(SERVO1_PIN, 500, 2500);
  servo2.attach(SERVO2_PIN, 500, 2500);
  servo1.write(S1_INIT);
  servo2.write(S2_INIT);

  // ESP-NOW: ESPNowCam for video TX, raw recv cb for joystick RX.
  if (!radio.init()) {
    Serial.println("Radio init fail");
  }
  radio.setTarget(controllerMac);
  WiFi.setTxPower(WIFI_POWER_11dBm);  // heat: short link, less PA power
  esp_now_register_recv_cb(onJoystickRecv);

  xTaskCreate(servo_task, "servo", 2048, NULL, 5, NULL);

  Serial.println("Rover ready (camera off by default)");
}

void loop() {
  uint32_t now = millis();

  // Failsafe: no control packets -> stop driving.
  if (now - lastRecvMs > 500) {
    stopMotors();
  }

  if (camera_on) {
    if (!cam_inited) {
      cam_inited = initCamera();
    }
    if (cam_inited && now - lastFrameMs >= FRAME_INTERVAL_MS) {
      if (Camera.get()) {
        radio.sendData(Camera.fb->buf, Camera.fb->len);
        Camera.free();
      }
      lastFrameMs = now;
    }
  } else if (cam_inited) {
    deinitCamera();
    cam_inited = false;
  }

  delay(1);  // yield
}
