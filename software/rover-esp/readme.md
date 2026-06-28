Rover ESP
=========

Camera node for the rover. Captures video on a Seeed Studio XIAO ESP32-S3 Sense
and streams 320x240 JPEG frames over ESP-NOW to the remote (`remote-tft`).

Built with [ESPNowCam](https://github.com/hpsaturn/ESPNowCam).

- This board's MAC: `DC:DA:0C:57:59:C8`
- Target (remote-tft) MAC: `98:3D:AE:60:84:C0`

The camera captures RGB565 QVGA (320x240) frames; each frame is JPEG-encoded
(quality 12) and sent unicast to the remote via `radio.setTarget()`.

Usage
-----

```sh
pio run -t upload -t monitor
```

The serial monitor prints the detected PSRAM size and `Camera streaming over
ESP-NOW`, then per-frame FPS (`CAM:`). Run `remote-tft` on the other board to
see the video.

Notes
-----

- No WiFi network is involved — ESP-NOW is peer-to-peer. Both boards just need
  to be on the same channel (both use the default, so no setup needed).
- The receiver MAC is hard-coded in `src/main.cpp` (`receiverMac`). If you swap
  the remote board, update it.
