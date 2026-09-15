Software
========

All firmware for the project lives in the [`software/`](./../software) folder. Every sketch is a self-contained [PlatformIO](https://platformio.org/) project — a folder with its own `platformio.ini`, which is what you open in the editor.

Two of the sketches are the ones you actually flash to build a working rover:

* **`rover-esp`** — the rover.
* **`remote-webcam`** — the remote.

Everything else is either a small utility or an earlier variant kept for reference.

Sketches
--------

| Sketch | Board | Framework | What it is |
|--------|-------|-----------|------------|
| [`rover-esp`](./../software/rover-esp) | XIAO ESP32-S3 Sense | Arduino | **Rover firmware.** Drives the two motors, moves the gripper servos, switches the lights, and streams the camera to the remote over ESP-NOW. |
| [`remote-webcam`](./../software/remote-webcam) | XIAO ESP32-S3 | ESP-IDF | **Remote firmware.** Reads the joystick and buttons, sends them to the rover, receives the video, and presents it to a phone as a USB webcam. |
| [`esp-mac`](./../software/esp-mac) | any ESP32 | Arduino | Utility. Prints the board's MAC address to the serial monitor. You need this once, during setup. |
| [`remote-tft`](./../software/remote-tft) | XIAO ESP32-S3 | Arduino | Test sketch for the video stream. Receives the rover's video and shows it on a 2.4" ILI9341 SPI screen, with no phone involved. |
| [`remote-simple`](./../software/remote-simple) | XIAO ESP32-S3 | Arduino | The earlier remote: joystick and buttons over ESP-NOW, no video. Useful if you build the rover without a camera. |

Setting up PlatformIO
---------------------

PlatformIO handles the toolchains, board definitions and libraries for you — there is nothing to install by hand.

1. Install [Visual Studio Code](https://code.visualstudio.com/) and the **PlatformIO IDE** extension. (If you prefer the command line, [PlatformIO Core](https://docs.platformio.org/en/latest/core/quickstart.html) alone is enough.)
2. Open the folder of a single sketch — for example `software/rover-esp`, the folder that contains `platformio.ini`. Do not open `software/` itself.
3. The first build downloads the platform, toolchain and libraries. This takes a few minutes; later builds are fast.

The commands below are run from the sketch folder.

```sh
pio run                       # build
pio run -t upload             # build and flash
pio run -t upload -t monitor  # build, flash, and open the serial monitor
pio device monitor -b 115200  # serial monitor only
pio check                     # static analysis
pio pkg update                # update platforms and libraries
```

If a sketch defines several boards (`[env:...]` sections in `platformio.ini`), pick one with `-e`:

```sh
pio run -e seeed_xiao_esp32s3 -t upload
```

Getting the rover running
-------------------------

The rover and the remote talk to each other over ESP-NOW, which addresses peers by MAC address. Each board therefore has to be told the MAC address of the other one. This is the only change you *have* to make before flashing.

### 1. Read both MAC addresses

Flash `esp-mac` to each of the two ESP32 boards in turn and open the serial monitor:

```sh
pio run -e seeed_xiao_esp32s3 -t upload -t monitor
```

It prints the address every two seconds:

```
ESP32 Board MAC Address:  98:3D:AE:60:84:C0
```

Write down which address belongs to the rover board and which to the remote board. Alternatively, use [ESPConnect](https://thelastoutpostworkshop.github.io/ESPConnect/), which reads the address over USB without flashing anything.

### 2. Put each address into the other sketch

**The rover** needs the remote's address. In `software/rover-esp/src/main.cpp`:

```cpp
// Controller MAC (peer) — video is sent here. Set to your controller's MAC.
static uint8_t controllerMac[6] = {0x98, 0x3D, 0xAE, 0x61, 0x72, 0x3C};
```

**The remote** needs the rover's address. In `software/remote-webcam/src/main.c`:

```cpp
// Rover MAC (peer) — joystick frames are sent here. Set to your rover's MAC.
static uint8_t robot_mac[6] = {0x98, 0x3D, 0xAE, 0x60, 0x84, 0xC0};
```

The values shipped in the repository are the author's own boards — replace both.

### 3. Flash the two sketches

Flash `rover-esp` to the rover board and `remote-webcam` to the remote board. Both boards use the default ESP-NOW channel, so there is nothing else to pair or configure.

> **Warning:** never power an ESP32 board without an antenna connected, over USB or from the battery. Transmitting without an antenna can damage the board. See the [Assembly](assembly.md) guide.

Rover firmware (`rover-esp`)
----------------------------

A single Arduino sketch that does four things at once: tank drive, two servos, the white LEDs, and the camera stream.

### Controls

| Input | Action |
|-------|--------|
| Joystick | Tank drive. Pushed forward or back, the rover drives and steers in an arc — the inner track slows but never reverses. With the stick centred vertically, left and right pivot the rover in place. |
| **A** / **C** | Servo 1 (shoulder) up / down. |
| **B** / **D** | Servo 2 (gripper) close / open. |
| **E** | Camera on/off. **Off by default** — when off, the camera is powered down entirely, so the rover runs cooler and quieter. |
| **F** | White LEDs on/off. **Off by default.** |

The servos are velocity-driven: hold a button and the servo steps toward its limit at about 66°/s; release it and the servo holds its position.

If no control packet arrives for 500 ms — the remote is switched off, or out of range — the motors stop and the servos stop moving.

### Wiring

| Function | XIAO pin | GPIO |
|----------|----------|------|
| Servo 1 (shoulder) PWM | D0 | 1 |
| Servo 2 (gripper) PWM | D1 | 2 |
| DRV8833 IN1 (motor 1) | D6 | 43 |
| DRV8833 IN2 (motor 1) | D5 | 6 |
| DRV8833 IN3 (motor 2) | D4 | 5 |
| DRV8833 IN4 (motor 2) | D3 | 4 |
| White LEDs (S8050 base) | D7 | 44 |

### What you may need to change

Everything below is a constant near the top of `src/main.cpp`.

**If a track runs backwards, or the rover turns the wrong way**, edit the two motor configuration lines. Which DRV8833 channel pair drives which track, and whether that motor is reversed, is the one thing that genuinely differs between builds:

```cpp
static MotorConfig leftMotor  = { M2A_CH, M2B_CH, true  };  // left  track on M2
static MotorConfig rightMotor = { M1A_CH, M1B_CH, false };  // right track on M1
```

**Speed and handling:**

| Constant | Meaning |
|----------|---------|
| `MIN_SPEED` / `MAX_SPEED` | PWM duty range, 0–255. `MIN_SPEED` is the point where the N20 motors actually start turning instead of buzzing. |
| `MOTOR_CORRECTION` | Trim, if the rover drifts to one side. `1.0` is no trim; below 1 trims the right track, above 1 trims the left. |
| `STEER_GAIN` | How sharply the rover turns while driving — how much the inner track slows at full steering. |
| `PIVOT_MIN_SPEED` | Duty floor for pivoting in place. Turning on the spot scrubs both tracks sideways and needs far more torque than driving, so this is much higher than `MIN_SPEED`. |
| `DEADZONE` | Joystick travel around the centre that counts as "no input". |

**Gripper travel:** `S1_MIN` / `S1_MAX` / `S1_INIT` for the shoulder and `S2_*` for the fingers. Swap the min and max to reverse a servo's direction. `SERVO_MIN_US` / `SERVO_MAX_US` is the pulse range, calibrated to 520–2480 µs for the MG90S — the stock 500–2500 µs drives the servo past its mechanical stop, where it can jam. Adjust if your servos differ.

**Camera:** `CAM_HMIRROR` and `CAM_VFLIP` fix the image orientation if the camera is mounted rotated or mirrored. `FRAME_INTERVAL_MS` paces the stream.

### Notes

* The camera streams QVGA (320×240) MJPEG. The CPU runs at 160 MHz and the radio at reduced transmit power, both to keep the board cool inside the printed body.
* `DRV_IN1` sits on D6 (GPIO43), which is the UART TX pin and idles high during boot. The right track spins forward for a second or two at power-on, until the firmware takes the pin over.
* The DRV8833 needs no library: speed is the PWM duty on one input, direction is which of the two inputs gets it.

Remote firmware (`remote-webcam`)
---------------------------------

The remote reads the joystick and buttons, sends them to the rover roughly every 50 ms, receives the rover's video, and exposes that video to an Android phone as a **standard USB webcam** (UVC).

```
remote --ESP-NOW joystick--> rover
rover --ESP-NOW MJPEG video--> remote --USB UVC--> phone
```

### Using it with a phone

Plug the remote's USB-C port into the phone with an OTG cable. UVC is class-compliant, so no driver is needed — you just need a **UVC viewer app**. [android-usb-cam-viewer](https://gitlab.com/yaky/android-usb-cam-viewer) is the recommended one; CameraFi, USB Camera and OTG View also work. The phone must support USB-OTG.

The video is MJPEG 320×240 at up to 15 fps. The frame rate you actually get depends heavily on the viewer app — if it feels slow, try a different one before blaming the firmware.

### Joystick calibration

The joystick is a plain potentiometer, so its readings scale with the supply voltage: a profile captured on USB power misbehaves on a half-drained battery. The firmware handles this automatically — at boot it rescales a built-in reference to the current supply. Nothing is stored, and nothing needs configuring.

To recalibrate by hand, hold the **joystick button** for 5 seconds: first with the stick released (this captures the centre), then sweep it to all extremes. A manual calibration lasts for the current session only.

If you would rather store an absolute profile in flash, set `USE_NVS_CALIBRATION` to `1` in `src/main.c`. That profile does not compensate for supply voltage, so you have to recalibrate whenever the power source changes.

### Wiring

Joystick X on GPIO1, Y on GPIO2. Buttons: **A** = 3, **B** = 4, **C** = 5, **D** = 6, **E** = 9, **F** = 8, joystick press = 7. USB-OTG uses GPIO19/20, so there is no conflict.

### Re-flashing

⚠️ Once the firmware is running, the USB port *is* a webcam, so the automatic reset used for flashing no longer works. To re-flash, put the board into download mode first: **hold BOOT, tap RESET, release BOOT**, then upload.

Serial logs go to UART0 rather than USB, for the same reason.

### What you may need to change

| Where | Constant | Meaning |
|-------|----------|---------|
| `src/main.c` | `robot_mac` | The rover's MAC address. |
| `src/main.c` | `CAMERA_TOGGLE_PIN` | Which button toggles the camera (**E** by default). |
| `src/main.c` | `OUTPUT_DEADZONE` | Joystick centre deadzone. Must be wider than the noise at rest, otherwise the output crosses zero and the rover twitches. |
| `src/main.c` | `USE_NVS_CALIBRATION` | Calibration strategy, see above. |
| `sdkconfig.defaults` | `CONFIG_UVC_CAM1_FRAMERATE` | Advertised frame rate. |
| `sdkconfig.defaults` | `CONFIG_TUSB_PRODUCT` | The name the phone shows for the webcam. |

### Why this one is not an Arduino sketch

`remote-webcam` uses `framework = espidf`, unlike every other sketch in the project. The USB UVC stack needs ESP-IDF 5.x, and on that core the ESPNowCam library does not build. So this sketch talks to `esp_now` directly and decodes the rover's frames with nanopb, reusing ESPNowCam's wire format so the two ends stay compatible.

Two consequences, both already handled in `platformio.ini`:

* It needs the **pioarduino** platform, which ships ESP-IDF 5.x. PlatformIO downloads it on the first build.
* That platform registers itself as `espressif32` and would shadow the stock one, so this project keeps its packages in a separate core directory (`~/.platformio-pioarduino`). Every other sketch carries on using the normal global installation.

Other sketches
--------------

### `esp-mac`

Prints the board's MAC address to the serial monitor, twice a second. Flash it, read the address, flash something else. See [Getting the rover running](#getting-the-rover-running).

### `remote-tft`

A test sketch for the video stream. It receives the rover's frames and draws them on a 2.4" 240×320 ILI9341 SPI display wired to a XIAO ESP32-S3, so you can confirm the rover is capturing and transmitting without involving a phone or the USB webcam path at all. It receives only — it sends no joystick data.

The display is configured through build flags in `platformio.ini`, so the setup travels with the project and you do not need to edit the TFT_eSPI library. Wiring and a pinout diagram are in the [sketch's readme](./../software/remote-tft/readme.md).

### `remote-simple`

The remote before video was added: joystick and buttons over ESP-NOW, nothing else. It carries the joystick conditioning that `remote-webcam` later inherited — oversampling, calibration, a centre deadzone. Worth using if you build the rover without a camera, or as a much smaller sketch to read when working out how the control link fits together.
