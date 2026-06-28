#include <WiFi.h>

// Taken from https://medium.com/@VaishnavSabariGirish/espnoww-the-esp-now-wrapper-library-for-platformio-part-2-coding-the-esp-boards-using-6ccae4ee77cc

void setup(){
  Serial.begin(115200);
  delay(2000);  // let USB CDC re-enumerate after reset so the first output isn't missed
  Serial.println();
}

void loop(){
  // Print repeatedly so the MAC can't be missed while the monitor reconnects.
  Serial.print("ESP32 Board MAC Address:  ");
  Serial.println(WiFi.macAddress());
  delay(2000);
}
