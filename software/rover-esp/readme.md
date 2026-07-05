Rover ESP
=========

The robot. A Seeed Studio XIAO ESP32-S3 Sense that drives two N20 motors (via a
DRV8833), moves a 2-servo manipulator, and streams its camera over ESP-NOW to
the controller (`remote-webcam`). It receives joystick/button data from the
controller over ESP-NOW.

```
controller --ESP-NOW joystick--> rover (this)
rover --ESP-NOW MJPEG video--> controller
```

Video uses [ESPNowCam](https://github.com/hpsaturn/ESPNowCam); joystick is
received with a raw `esp_now` callback; servos use ESP32Servo; the DRV8833
is driven directly with LEDC PWM (no library).

Wiring (Xiao ESP32-S3)
----------------------

| Function | XIAO | GPIO |
|----------|------|------|
| Servo 1 (shoulder) PWM | D0 | 1 |
| Servo 2 (gripper) PWM | D1 | 2 |
| DRV8833 IN1 (motor 1) | D6 | 43 |
| DRV8833 IN2 (motor 1) | D5 | 6 |
| DRV8833 IN3 (motor 2) | D4 | 5 |
| DRV8833 IN4 (motor 2) | D3 | 4 |
| White LEDs (S8050 base) | D7 | 44 |

Controls
--------

- **Joystick** → tank drive (two motors, mixed).
- **A / C** → servo 1 (shoulder) up / down. **B / D** → servo 2 (gripper)
  close / open. Hold to move, release to hold position.
- **E** (on the controller) → camera on/off. **Default off.** When off the
  camera is de-initialised — no capture, encode or radio traffic.
- **F** (on the controller) → white LEDs on/off. **Default off.**
- Failsafe: if no control packet arrives for 500 ms the motors stop.

Usage
-----

```sh
pio run -t upload -t monitor
```

Notes
-----

- DRV8833 needs no library: speed = PWM duty on an input, direction = which of
  the two inputs gets the PWM. Motors on LEDC channels 4-7; servos (ESP32Servo)
  use LEDC timers 0-1.
- Tunables in `src/main.cpp`: `MIN_SPEED`/`MAX_SPEED`, `MOTOR_CORRECTION`,
  servo angle limits (`S1_*`, `S2_*`). If a motor or servo runs the wrong way,
  swap its two pins (or min/max).
- Camera default off, so the rover boots cool and quiet; it spins the camera up
  only when the controller asks.
