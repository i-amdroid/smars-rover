Code
====

Please note that included sketch is in early beta. The project is under active development. However, it is enough to bring life to your first robot.

Features
--------

* No delay() used
* Bluetooth control via Dabble lib and App (support BLE)
* Simple movement control (with constant speed)
* Advanced movement control (aka Gearbox)
* LED control
* Buzzer control
* Parktronic mod
* Shovel mod

**Note:**

Simple movement, Parktronic mod, Shovel mod are commented out by default, uncomment it in the code when needed.

Dabble
------

Bluetooth interactions are managed by Dabble library which handles all low-level logic.

Zip archive with the library is included in this repo, also you can get it here [github.com/STEMpedia/Dabble](https://github.com/STEMpedia/Dabble).

Apps for iOS and Android [thestempedia.com/product/dabble](https://thestempedia.com/product/dabble/).

Library docs [thestempedia.com/docs/dabble/getting-started-with-dabble](https://thestempedia.com/docs/dabble/getting-started-with-dabble/).

PlatformIO and Arduio IDE
-------------------------

The code is provided as a PlatformIO project. For those who don't familiar with it or prefer other IDEs like Arduino IDE, just check `software/platformio/src/smars-rover.ino` And you will find everything you need.

In progress
-----------

* LCD mod
* sketch for ESP32-CAM
