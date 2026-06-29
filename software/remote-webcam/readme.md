Remote webcam
=============

Receives 320x240 MJPEG video over ESP-NOW from the rover (`rover-esp`) and
exposes it to an Android phone as a standard **USB UVC webcam**. No local
camera or display.

```
rover-esp (camera) --ESP-NOW (MJPEG)--> remote-webcam --USB UVC--> Android
```

The rover sends MJPEG and UVC also carries MJPEG, so received frames are handed
to the USB host as-is — no decoding. Plug the XIAO's USB-C into an Android phone
(OTG).

- This board's MAC: `98:3D:AE:60:84:C0`
- Sender (rover-esp) MAC: `DC:DA:0C:57:59:C8`

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
- A pure ESP-IDF / `idf.py` copy of this sketch is kept in `../remote-webcam-idf`
  as a fallback.
