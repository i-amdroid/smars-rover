Electronic parts
================

Mandatory parts
---------------

### Wires

**Wires 10mm with Dupont connector male-male**

**Wires 10mm with Dupont connector male-female**

**Wires 20mm with Dupont connector male-male**

**Wires 20mm with Dupont connector male-female**

### Control board

**Option 1: Arduino Uno** - 1pc

**Option 2: ESP32-CAM** - 1pc

ESP32-CAM mod development is currently in progress. The body was designed with the possibility of using ESP32-CAM as the control board, however, the mount itself has not been designed yet.
ESP32-CAM is much powerful than Arduino, already holds Wi-Fi and Bluetooth onboard, and allows to transmit video from the camera over Wi-Fi in real-time! Sounds good? Stay tuned or welcome to help with development.
If you don't have a separate programmer for ESP32, better to choose ESP32-CAM with the motherboard in the box.

### Motors

**N20 5-6V, 150-300 rpm** - 2pc

### Motor driver

**Option 1 (recommended): MX1508** - 1pc

MX1508 installs on a holding board, under the Arduino, and it becomes possible to install a Connection shield for connecting wires.
There is room for the MX1508 under the Arduino only if you use mods for 14500 or AAA batteries.
Optional: MX1508 allows you to make connections using connectors, this will greatly simplify the disassembly of the robot, although it will not save you from soldering, because at least you will need to solder the connectors themselves.

**Other options:**

In theory, you can use other boards like Adafruit Motor Shield v1, v2, and others.
But keep in mind that the Adafruit Motor Shield is mounted on top of the Arduino, and there will not be much room in the case for the wires commutation. Most likely, the wires will need to be soldered to the board, since the height of the case does not allow the use of Dupont connectors in a vertical position.

### Battery

**Option 1 (recommended): 14500 batteries** - 2pc

This option will require a special chassis coming in addition to the project. This option works well if you're building your first robot and don't have a printed chassis yet.
Advantages of this option: there is room for MX1508, no complete disassembly of the robot is required to access the batteries.
For the battery contact pads, you will need nickel strip.

**Option 2: AAA batteries** - 3pc

This option is compatible with the standard chassis and 4wd chassis, you only need a special holding board - [thingiverse.com/thing:2762630](https://www.thingiverse.com/thing:2762630).
Advantages of this option: there is room for MX1508, AAA batteries can be purchased at the nearest store.
Disadvantages of this option: Complete disassembly of the robot is required to access the batteries. The 4.5V from the batteries is enough to power the motors, but there is no headroom to power additional devices such as servo motors. When using additional mods, there may be a lack of power.
Since 4.5V is actually the lowest possible supply voltage, you will not be able to use NMHg AAA batteries as they only supply 3.6V and this is not enough.
For the battery contact pads, you will need nickel strip. Batteries should be connected directly to the 5V pin of the Arduino, not the Vin pin.

**Option 3: 9V battery** - 1pc

This option is positioned as the main and, perhaps, the simplest. but in this case, there is no room to install MX1508 under the Arduino. In theory, the board can fit over an Arduino even when using a Connection shield.
To connect the battery, you need a contact pad with wires.

### Screws

**Screws with pad M3x5 (screws for computer case)** - 5pc

Optional: If you are going to use 14500 chassis you will need one additional screw with pad M3x5 (a total - 6pc).
Additionally, you will need a pan head screws M4x20-30 (2pc) and self-locking nuts M4 (2pc), for slave wheels mounting.

Recommended parts
-----------------

### Bluetooth

It is possible to control the SMARS robot without Bluetooth, using the commands flashed to Arduino. But with Bluetooth control, interaction with the robot becomes much more dynamic, it becomes possible to program the robot to execute commands that you send from a mobile phone.

**Option 1 (recommended): AT-09 (or HM-10)** - 1pc

A bit more expensive than HC-06 but support Bluetooth 4.0 BLE. BLE is a more energy-efficient protocol and is more stable, but requires BLE-compatible software. There is not so much software for BLE, but there is at least one decent application and library for it - Dabble, see the [Code](software.md) section.

**Option 2: HC-06 (or HC-05)** - 1pc.

Cheap and good. Works on Bluetooth v2.0, don't compatible with Apple devices.

### Сonnection shield

The connection shield is designed to conveniently connect motors, batteries, and accessories to the Arduino. The Connection shield will give a finished look to your project, allow you to connect and disconnect electronics using connectors, without soldering, and also allows you to install a buzzer and a power switch on the board.

You cannot buy such a board in a store, but it is not so difficult to make it yourself. Very few components are required with minimal costs. Also, assembling your connection shield is good training for soldering skills.

Connection shield features:

* Connecting batteries
* Connecting motor driver, signal, and power
* Connecting Bluetooth module
* Connecting servo motor
* Connecting LED light
* Connecting Ultrasonic Distance Sensor
* Connecting an I²C (IIC) display
* Possibility of installing buzzer on the board
* Possibility of installing power switch on the board

Required components:

* Breadboard with Arduino Uno shield form - 1pc
* 2.54mm male pin header strip (if not included with the breadboard) - 1x40pin
* 2.54mm right angle male pin header strip - 1x20pin
* 2.54mm right angle female socket header strip - 1x10socket
* Buzzer - 1pc
* Power switch - 1pc
* Ceramic capacitor 0.1uF (often labeled as 104) - 3pc

For the assembly and connection diagram, see the [Сonnection shield](сonnection-shield.md) section.

### Wheel lock

Orginal master wheel mount method completely based on friction. The motor shaft should be pushed to the wheel hole. The hole must be very precisely adjusted to the size of the motor shaft, which is not so easy to achieve with 3D printing, and the shape of the hole does not allow mechanical processing. If the hole is larger than the shaft, then the wheel will constantly fall off. If the hole is very tight, then the effort required to put on and remove the wheel can damage the motor gearbox. For this reason, two of my gearboxes had been damaged. After that, a method of fastening the wheel with a screw (`wheel-master-lock.3mf`) has been applied.

Required components:

* Flat head screws M3x12 - 2pc
* Square nut M3 - 2pc

Optional parts
--------------

### LED light

It is very easy to add headlights to the robot. The body model includes a front panel with headlight holes (`face.3mf`). The Connection shield has a connector for LED.

Required components:

* Breadboard 60x40 (20x14 holes) - 1pc
* Yellow LED 5mm - 2pc
* Resistor 47ohm - 1pc

For assembly and connection diagram, see [LED board](led-board.md) section.

### Distance sensor

Mods using distance sensor are one of the most common add-ons for SMARS. The body model includes a front panel with holes for the distance sensor (`face.3mf`). The Connection shield has a connector for distance sensor.

**HC-SR04 Ultrasonic distance sensor** - 1pc

### Servo motor

Servo motor is a device that is used in almost every robot. There are several add-ons for SMARS that use a Servo motor, for example, Shovel mod [thingiverse.com/thing:4238338](https://www.thingiverse.com/thing:4238338). The connection shield has a connector for connecting a Servo motor.

**Option 1 (recommended): MG90s** - 1pc

A bit more expensive than SG90 but has metal gears.

**Option 2: SG90** - 1pc

Cheap and good, plastic gears.

Compatible parts
----------------

### Display

OLED screen support for sMARS rover is currently in progress. At this moment we have done: a connector for I²C (IIC) display on the Connection shield and mount for 0.96" display on the body (`front-glass-display.3mf`, `front-glass-display-support.3mf`)

**OLED Display 0.96" 128x64** - 1pc



