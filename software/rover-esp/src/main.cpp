// Rover camera node — ESP-NOW video transmitter.
//
// Captures frames on a Seeed Studio XIAO ESP32-S3 Sense, encodes them as JPEG
// and streams them over ESP-NOW to the remote (remote-tft).
//
// Based on the ESPNowCam "xiao-espnow-sender" example:
// https://github.com/hpsaturn/ESPNowCam
//
// This board MAC:        DC:DA:0C:57:59:C8
// Target (remote-tft):   98:3D:AE:60:84:C0

#include <Arduino.h>
#include <WiFi.h>
#include <ESPNowCam.h>
#include <drivers/CamXiao.h>
#include <Utils.h>

// MAC of the receiver (remote-tft). Frames are sent unicast to this address.
uint8_t receiverMac[6] = {0x98, 0x3D, 0xAE, 0x60, 0x84, 0xC0};

CamXiao Camera;
ESPNowCam radio;

// DRAM JPEG capture needs a small pace delay to stay stable (see ESPNowCam
// internal-jpg example). ~60 ms keeps it around the camera's ~11 FPS ceiling.
static const uint32_t FRAME_DELAY_MS = 60;

void processFrame() {
  if (Camera.get()) {
    // Camera already produced a hardware-JPEG frame — send it as-is.
    radio.sendData(Camera.fb->buf, Camera.fb->len);
    delay(FRAME_DELAY_MS);
    printFPS("CAM:");
    Camera.free();
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Heat/power optimization #1: drop the CPU from 240 MHz to 160 MHz. WiFi
  // needs >=80 MHz; 160 MHz keeps plenty of headroom for ~9 FPS while cutting
  // core dynamic power noticeably.
  setCpuFrequencyMhz(160);

  if (psramFound()) {
    size_t psram_size = esp_spiram_get_size() / 1048576;
    Serial.printf("PSRAM size: %dMb\r\n", psram_size);
  }

  if (!radio.init()) {
    Serial.println("Radio Init Fail");
    delay(1000);
  }
  radio.setTarget(receiverMac);  // unicast to remote-tft

  // Heat/power optimization #2: the WiFi power amplifier is the biggest heat
  // source (TX peaks 180-240 mA). Default TX power is ~19.5 dBm; for a short
  // rover<->remote link that is overkill. 11 dBm is ~7x less PA output power.
  // Raise this (e.g. WIFI_POWER_15dBm/19_5dBm) if the range becomes too short.
  WiFi.setTxPower(WIFI_POWER_11dBm);

  // Capture hardware JPEG straight from the OV2640 (no software encoding) into
  // DRAM. This avoids the slow frame2jpg() path that was starving the camera
  // DMA (EV-VSYNC-OVF) and tearing frames.
  Camera.config.pixel_format = PIXFORMAT_JPEG;
  Camera.config.frame_size = FRAMESIZE_QVGA;  // 320x240
  Camera.config.jpeg_quality = 18;  // higher number = more compression = fewer ESP-NOW packets
  Camera.config.fb_count = 2;
  Camera.config.fb_location = CAMERA_FB_IN_DRAM;

  if (!Camera.begin()) {
    Serial.println("Camera Init Fail");
    delay(1000);
    ESP.restart();
  }
  delay(500);
  Serial.println("Camera streaming over ESP-NOW");
}

void loop() {
  processFrame();
}
