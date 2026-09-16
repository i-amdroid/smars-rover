ESP MAC
=======

Prints the board's MAC address to the serial monitor, so you can pair the rover
and the remote. ESP-NOW addresses its peer by MAC, so each board has to be told
the address of the other one.

Usage
-----

```sh
pio run -e seeed_xiao_esp32s3 -t upload -t monitor
```

The address is printed every two seconds, so it can't be missed while the
monitor reconnects after a reset:

```
ESP32 Board MAC Address:  98:3D:AE:60:84:C0
```

Flash this to each of the two boards in turn and note which address belongs to
which. Then put the remote's address into `rover-esp` (`controllerMac`) and the
rover's address into `remote-webcam` (`robot_mac`), and flash the real firmware
over this sketch.

An alternative that needs no flashing at all is
[ESPConnect](https://thelastoutpostworkshop.github.io/ESPConnect/), which reads
the address over USB from the browser.

Two environments are configured: `seeed_xiao_esp32s3` (the boards used in this
project) and `esp32-s3-devkitc-1`.
