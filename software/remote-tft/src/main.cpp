// Remote display node — ESP-NOW video receiver.
//
// Receives JPEG frames over ESP-NOW from the rover (rover-esp), decodes them
// with TJpg_Decoder and pushes them to a 2.4" ILI9341 TFT via TFT_eSPI.
//
// The ESP-NOW receive callback must stay fast: it only snapshots the frame.
// Decoding and the (slow) SPI draw happen in loop(), so the receive task keeps
// ACKing promptly — otherwise the unicast sender stalls and drops packets,
// which both tanks the frame rate and corrupts frames.
//
// This board MAC:   98:3D:AE:60:84:C0
// Sender (rover):   DC:DA:0C:57:59:C8

#include <Arduino.h>
#include <ESPNowCam.h>
#include <TFT_eSPI.h>
#include <TJpg_Decoder.h>

TFT_eSPI tft = TFT_eSPI();
ESPNowCam radio;

#define FB_CAP 50000
static uint8_t fb[FB_CAP];         // library assembles the incoming frame here
static uint8_t decodeBuf[FB_CAP];  // snapshot decoded in loop()
static volatile uint32_t frameLen = 0;
static volatile bool frameReady = false;

// TJpg_Decoder output callback: push each decoded block to the screen.
bool tftOutput(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t *bitmap) {
  if (y >= tft.height()) return false;
  tft.pushImage(x, y, w, h, bitmap);
  return true;
}

// Runs in the ESP-NOW receive task — keep it minimal.
void onDataReady(uint32_t length) {
  if (frameReady) return;  // previous frame still being drawn, drop this one
  if (length == 0 || length > FB_CAP) return;
  memcpy(decodeBuf, fb, length);
  frameLen = length;
  frameReady = true;
}

void setup() {
  Serial.begin(115200);
  delay(500);

  tft.init();
  tft.setRotation(1);  // landscape: 320 wide x 240 high
  tft.fillScreen(TFT_BLACK);

  TJpgDec.setJpgScale(1);
  TJpgDec.setSwapBytes(true);
  TJpgDec.setCallback(tftOutput);

  radio.setRecvBuffer(fb);
  radio.setRecvCallback(onDataReady);
  if (!radio.init()) {
    Serial.println("Radio Init Fail");
  }
  Serial.println("Receiver ready, waiting for frames...");
}

void loop() {
  if (frameReady) {
    TJpgDec.drawJpg(0, 0, decodeBuf, frameLen);
    frameReady = false;
  }
}
