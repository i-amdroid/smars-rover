Remote simple
=============

The remote before video was added: joystick and buttons sent to the rover over
ESP-NOW, nothing else. Useful if you build the rover without a camera, and a much
smaller sketch to read than `remote-webcam` if you want to see how the control
link works.

It sends the same `JoystickData` packet the rover expects, so it drives
`rover-esp` as-is (the camera simply stays off).

Joystick conditioning
---------------------

`remote-webcam` inherited this part unchanged:

* Each axis is the mean of 64 raw ADC reads. Oversampling removes noise spikes
  with no lag — deliberately no time-domain low-pass filter, which would make the
  output coast after release and leave the motors creeping.
* Readings are mapped to -1000..1000 against a calibrated min/centre/max, saved
  in NVS.
* A generous centre deadzone snaps the output to a hard zero at rest. It has to
  be wider than the noise swing near centre, otherwise the value crosses zero and
  the rover flips both tracks' direction.

Set `DEBUG_JOYSTICK` to `1` and open the serial monitor to print raw and mapped
values while tuning.

Usage
-----

Set `receiver_mac` to the rover's MAC address before flashing — see
[esp-mac](../esp-mac).

```sh
pio run -e seeed_xiao_esp32s3 -t upload -t monitor
```
