Remote webcam (controller)
==========================

The handheld controller. It reads a joystick + buttons and sends them to the
robot over ESP-NOW, receives the robot's 320x240 MJPEG video, and exposes that
video to an Android phone as a standard **USB UVC webcam**.

```
controller (this) --ESP-NOW joystick--> robot
robot --ESP-NOW MJPEG video--> controller --USB UVC--> Android
```

Received MJPEG is handed to the USB host as-is (no decoding). Plug the XIAO's
USB-C into an Android phone (OTG).

This merges the old `remote-simple` joystick controller into the video path.
Because the sketch is pure ESP-IDF (see below), the joystick is read with the
native `esp_adc` driver and sent with raw `esp_now`, not Arduino + ESPNowW.

Joystick / buttons / camera
---------------------------

- Sends a raw `JoystickData` struct (X/Y axes -1000..1000, 7 buttons, plus a
  `camera_on` flag) to the robot every ~50 ms via `esp_now_send`.
- The robot streams video only while `camera_on` is set. The **E button**
  toggles it (`CAMERA_TOGGLE_PIN` in `main.c`). When off, the controller's UVC
  `fb_get` returns nothing, so it idles instead of re-sending a frozen frame.
- Joystick calibration (center + extremes) is stored in NVS and re-run by
  holding the **joystick button** — ported from `remote-simple`.
- Pins (Xiao ESP32-S3): X=GPIO1, Y=GPIO2; buttons A=3, B=4, C=5, D=6, E=9,
  F=8, joystick-press=7. (USB-OTG uses GPIO19/20, so no conflict.)

Build & flash (PlatformIO)
--------------------------

Normal PlatformIO workflow:

```sh
pio run                       # build
pio run -t upload             # flash
pio run -t upload -t monitor  # flash + serial (serial is on UART0, see below)
```

⚠️ Re-flashing: once the UVC firmware runs, the USB port is a webcam, so the
auto-reset used for flashing stops working. To re-flash, put the board in
download mode first (hold **BOOT**, tap **RESET**, release BOOT), then upload.

On the phone
------------

UVC is class-compliant, so **no driver** is needed. Install a **UVC viewer app**:
CameraFi, USB Camera, OTG View, etc. The recommended app is
https://gitlab.com/yaky/android-usb-cam-viewer. The phone must support USB-OTG.

How it's built (why not Arduino)
--------------------------------

This sketch uses `framework = espidf`, not `arduino`, even though the rest of
the project is Arduino. The USB UVC component (`usb_device_uvc`) needs ESP-IDF
5.x (= Arduino-ESP32 3.x), and on that core:

- ESPNowCam (0.2.x) doesn't compile — the esp_now callback signatures changed;
- building Arduino-as-a-component pulls in unrelated components that fail.

So the receive path talks to `esp_now` directly and decodes the rover's frames
with nanopb, reusing ESPNowCam's `frame.pb` wire format (byte-compatible with
the transmitter). The PlatformIO commands above are unchanged.

The `usb_device_uvc` component is vendored under `components/` with a one-line
patch (`PUBLIC` → `PRIVATE` on its `usb_descriptors.c`); without it PlatformIO's
build errors with "Two environments ... for the same target".

Notes
-----
- Needs the pioarduino platform (ships ESP-IDF 5.x); see `platformio.ini`.
- UVC advertises one format: MJPEG 320x240 @ up to 15 fps (`sdkconfig.defaults`),
  using bulk transfer mode (faster than isochronous on this chip).
- The frame rate you actually get depends heavily on the **Android UVC app** —
  some are much slower than others. If fps is low, try a different viewer before
  blaming the firmware.
- ESP32-S3 USB is full-speed; ample for QVGA MJPEG.
- Serial logs go to UART0 (not USB) since UVC owns the USB port.
- A pure ESP-IDF / `idf.py` copy of the original video-only receiver is kept in
  `../test-webcam-idf` as a reference/fallback.
