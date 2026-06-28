Remote TFT
==========

Receives 320x240 JPEG video over ESP-NOW from the rover (`rover-esp`) and shows
it on a 2.4" 240x320 ILI9341 SPI TFT (no touch) connected to a Seeed Studio
XIAO ESP32-S3.

Built with [TFT_eSPI](https://github.com/Bodmer/TFT_eSPI) (display),
[TJpg_Decoder](https://github.com/Bodmer/TJpg_Decoder) (JPEG decode) and
[ESPNowCam](https://github.com/hpsaturn/ESPNowCam) (ESP-NOW transport).

- This board's MAC: `98:3D:AE:60:84:C0`
- Sender (rover-esp) MAC: `DC:DA:0C:57:59:C8`

Incoming JPEG frames are decoded and pushed to the screen in landscape
orientation from the ESPNowCam receive callback.

Wiring
------

The TFT module is powered and driven at 3.3V from the XIAO. The touch pins
(`T_*`) are unused on the no-touch module.

| TFT pin     | Signal        | XIAO pin | ESP32-S3 GPIO |
|-------------|---------------|----------|---------------|
| VCC         | 3.3V          | 3V3      | —             |
| GND         | Ground        | GND      | —             |
| CS          | Chip select   | D1       | GPIO2         |
| RESET       | Reset         | D3       | GPIO4         |
| DC (RS)     | Data/command  | D2       | GPIO3         |
| SDI (MOSI)  | SPI data in   | D10      | GPIO9         |
| SCK         | SPI clock     | D8       | GPIO7         |
| LED         | Backlight     | 3V3      | —             |
| SDO (MISO)  | SPI data out  | D9       | GPIO8         |

Notes:
- `LED` (backlight) goes straight to 3.3V — the module has an onboard current
  limiting resistor. If your backlight is too bright you can drive it from a
  GPIO instead.
- `SDO`/MISO is optional for a write-only display, but it is wired here because
  TFT_eSPI expects the pin to be defined.
- If the image looks mirrored or rotated, change `tft.setRotation()` in
  `src/main.cpp` (0–3).

Pinout diagram
--------------

```
   XIAO ESP32-S3                      ILI9341 TFT (14-pin, no touch)
  +-------------+                     +-----------------------------+
  |        3V3  o---------+---------->o VCC                         |
  |        GND  o-------+ +---------->o LED  (backlight)            |
  |             |       +------------>o GND                         |
  |   D8 (G7)   o------------- SCK -->o SCK                         |
  |   D9 (G8)   o------------ MISO -->o SDO (MISO)                  |
  |   D10(G9)   o------------ MOSI -->o SDI (MOSI)                  |
  |   D2 (G3)   o------------- DC --->o DC                          |
  |   D3 (G4)   o------------ RST --->o RESET                       |
  |   D1 (G2)   o------------- CS --->o CS                          |
  +-------------+                     | T_* touch pins: unused      |
                                      +-----------------------------+
```

Usage
-----

```sh
pio run -t upload -t monitor
```

Power on the rover (`rover-esp`) too — once it starts streaming, frames appear
on the screen. The serial monitor prints `Receiver ready, waiting for
frames...`.

Both boards must be on the same ESP-NOW channel (both use the default channel,
so no configuration is needed).
