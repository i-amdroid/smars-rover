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
#define DRV_IN2    6    // D5 /
#define DRV_IN3    5    // D4 \ motor 2
#define DRV_IN4    4    // D3 /
#define LED_PIN    44   // D7 — white LEDs via S8050

// LEDC: motors on channels 4-7 (timers 2-3); ESP32Servo gets timers 0-1.
#define M1A_CH 4
#define M1B_CH 5
#define M2A_CH 6
#define M2B_CH 7
#define MOTOR_PWM_FREQ 20000
#define MOTOR_PWM_RES  8     // 0-255

// ===== Motor wiring (tune per build — nothing else needs changing) =====
// Each track is driven by one DRV8833 channel pair. Pick which pair goes to
// which track and set `reversed` if that motor spins the wrong way, to match
// your assembly. This build (bench-confirmed): connectors swapped, left reversed.
struct MotorConfig {
  int chA;         // LEDC channel -> DRV input A
  int chB;         // LEDC channel -> DRV input B
  bool reversed;   // true = swap A/B so "forward" really is forward
};
static MotorConfig leftMotor  = { M2A_CH, M2B_CH, true  };  // left  track on M2
static MotorConfig rightMotor = { M1A_CH, M1B_CH, false };  // right track on M1

// ===== Movement tuning =====
// Duty is 0-255 (8-bit LEDC). MIN raised so the N20s actually break away from
// standstill (below this they buzz but don't turn).
static const int MIN_SPEED = 90;   // ~35% — kick-start floor
static const int MAX_SPEED = 220;  // ~86% of 255
static const int DEADZONE = 10;
static const float MOTOR_CORRECTION = 1.0f;  // 1.0 = no trim; <1 trims right, >1 trims left
// Turn sharpness on the move: how much the inner track slows at full steer.
// 0.0 = no slowdown (goes straight); 1.0 = inner drops to a crawl. 0.6 was the
// value road-tested on the real robot.
static const float STEER_GAIN = 0.6f;

// ===== Servos =====
static Servo servo1, servo2;
// Shoulder ~0-90, gripper ~0-120 (tune freely; swap min/max to flip direction).
static const int S1_MIN = 0, S1_MAX = 90, S1_INIT = 0;
static const int S2_MIN = 0, S2_MAX = 120, S2_INIT = 0;
static const int SERVO_STEP_MS = 15;  // MG90S ~ this per degree
// MG90S pulse range, calibrated. Stock 500-2500 drives 0 deg past the low stop,
// where the servo jams and can stick; 600-2400 keeps both ends inside travel
// (best practice: fit min/max to the actual servo). Tune if yours differs.
static const int SERVO_MIN_US = 520, SERVO_MAX_US = 2480;
// Motion is velocity-based: while a button is held the servo steps toward its
// limit; released -> dir 0 -> it holds where it is (see servo_task).
static volatile int s1_dir = 0, s2_dir = 0;  // -1 / 0 / +1

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
// Speed = PWM duty on one input; direction = which input gets it; both 0 = coast.
static void driveMotor(const MotorConfig &m, int speed, int dir) {
  if (speed < 0) speed = 0;
  if (speed > 255) speed = 255;
  if (m.reversed) dir = -dir;
  if (dir > 0) {
    ledcWrite(m.chA, speed);
    ledcWrite(m.chB, 0);
  } else if (dir < 0) {
    ledcWrite(m.chA, 0);
    ledcWrite(m.chB, speed);
  } else {
    ledcWrite(m.chA, 0);
    ledcWrite(m.chB, 0);  // coast
  }
}

static void stopMotors(void) {
  driveMotor(leftMotor, 0, 0);
  driveMotor(rightMotor, 0, 0);
}

// Tank mixing. Two regimes:
//   * Moving (y != 0): smooth arc. Outer track keeps its throttle speed, the
//     inner track only *slows* (down to a crawl) — it never reverses, so the
//     turn stays gentle.
//   * In place (y == 0): pivot — the tracks counter-rotate.
static void calculateTankMovement(int16_t x, int16_t y, int &lSpeed, int &rSpeed, int &lDir, int &rDir) {
  if (abs(x) < DEADZONE) x = 0;
  if (abs(y) < DEADZONE) y = 0;

  if (x == 0 && y == 0) {
    lSpeed = rSpeed = 0;
    lDir = rDir = 0;
    return;
  }

  if (y == 0) {
    // Pivot in place: tracks spin opposite ways.
    int turn = map(abs(x), DEADZONE, 1000, MIN_SPEED, MAX_SPEED);
    lDir = (x > 0) ? 1 : -1;
    rDir = (x > 0) ? -1 : 1;
    lSpeed = rSpeed = turn;
  } else {
    // Arc: both tracks same direction; inner one slowed, never reversed.
    lDir = rDir = (y > 0) ? 1 : -1;
    int base = map(abs(y), DEADZONE, 1000, MIN_SPEED, MAX_SPEED);  // outer/throttle speed
    float steer = fabsf(x / 1000.0f);          // 0..1
    int inner = (int)(base * (1.0f - steer * STEER_GAIN));
    if (inner < MIN_SPEED) inner = MIN_SPEED;  // keep it crawling, don't stall/buzz
    if (x > 0) {          // turning right -> right track is inner
      lSpeed = base;  rSpeed = inner;
    } else if (x < 0) {   // turning left  -> left track is inner
      lSpeed = inner; rSpeed = base;
    } else {
      lSpeed = rSpeed = base;
    }
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

  // Drive. Wiring quirks (swapped connectors, reversed left motor) live in the
  // MotorConfig up top, so here we just hand each track its speed + direction.
  int lS, rS, lD, rD;
  calculateTankMovement(j->x, j->y, lS, rS, lD, rD);
  driveMotor(leftMotor,  lS, lD);
  driveMotor(rightMotor, rS, rD);

  // Servo 1 (shoulder): A up, C down. Servo 2 (gripper): B close, D open.
  // Hold to move, release to hold position (servo_task integrates the dir).
  s1_dir = j->btn_up    ? +1 : (j->btn_down ? -1 : 0);
  s2_dir = j->btn_right ? +1 : (j->btn_left ? -1 : 0);

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
// One degree per SERVO_STEP_MS while a button is held (~66°/s), clamped to each
// servo's limits. No button -> dir 0 -> the servo just holds its position.
static void servo_task(void *arg) {
  int cur1 = S1_INIT, cur2 = S2_INIT;
  for (;;) {
    if (s1_dir) { cur1 = constrain(cur1 + s1_dir, S1_MIN, S1_MAX); servo1.write(cur1); }
    if (s2_dir) { cur2 = constrain(cur2 + s2_dir, S2_MIN, S2_MAX); servo2.write(cur2); }
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
  // Pin the DRV8833 inputs LOW the instant we boot, before the slow init below
  // (delay, radio, camera). Left floating during that window the driver can spin
  // a motor for a second or two. LEDC takes these pins over further down.
  pinMode(DRV_IN1, OUTPUT); digitalWrite(DRV_IN1, LOW);
  pinMode(DRV_IN2, OUTPUT); digitalWrite(DRV_IN2, LOW);
  pinMode(DRV_IN3, OUTPUT); digitalWrite(DRV_IN3, LOW);
  pinMode(DRV_IN4, OUTPUT); digitalWrite(DRV_IN4, LOW);

  Serial.begin(115200);
  delay(500);

  setCpuFrequencyMhz(160);  // heat/power: enough for streaming, cooler

  // Motors (DRV8833) via LEDC. Zero each channel's duty before attaching its pin
  // so the hand-off from the low outputs above stays glitch-free.
  ledcSetup(M1A_CH, MOTOR_PWM_FREQ, MOTOR_PWM_RES);
  ledcSetup(M1B_CH, MOTOR_PWM_FREQ, MOTOR_PWM_RES);
  ledcSetup(M2A_CH, MOTOR_PWM_FREQ, MOTOR_PWM_RES);
  ledcSetup(M2B_CH, MOTOR_PWM_FREQ, MOTOR_PWM_RES);
  ledcWrite(M1A_CH, 0); ledcWrite(M1B_CH, 0);
  ledcWrite(M2A_CH, 0); ledcWrite(M2B_CH, 0);
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
  servo1.attach(SERVO1_PIN, SERVO_MIN_US, SERVO_MAX_US);
  servo2.attach(SERVO2_PIN, SERVO_MIN_US, SERVO_MAX_US);
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
    s1_dir = s2_dir = 0;  // don't let a servo run away if the link drops mid-hold
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
