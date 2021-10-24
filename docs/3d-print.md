3d printable parts
==================

sMARS rover is primarily a body for the SMARS robot, but the project also includes 3 add-ons - a chassis for 14500 batteries, wheels with a  lock, and reflected tracks.

Both the body and all add-ons can be used independently of each other. All parts are fully compatible with the original parts of the SMARS robot, as well as with most add-ons.

Body
----

### Parts

1. **`body/base.3mf`**\
  Frame for mounting the front and back of the body. The frame itself mounts on the chassis of the SMARS robot.
2. **`body/front.3mf`**\
  The front of the body. Mounts on the frame. Allows you to install the face and front panels. For added reliability, it is advisable to fix with screw with pad (screws for computer case), 1 pc.
3. **`body/back.3mf`**\
  The back of the body. Mounts on the frame. Allows you to install a back door.
4. **`body/back-door.3mf`**\
  Back door. Mounts on the back of the body using a piece of filament with a diameter of 1.75mm.
5. **`body/face-blank.3mf`**\
  Face panel without holes. Installed on the front of the body. Installs in a groove, for additional support, it is advisable to fix it with hot melt glue.
6. **`body/face.3mf`**\
  Alternative face panel with holes for distance sensor and LED light. Installed in the same way as the front panel without holes.
7. **`body/front-glass-blank.3mf`**\
  Front panel without holes (windshield). Mounts on the front of the body with screws with pad, 4 pc.
8. **`body/front-glass-display.3mf`**\
  Front panel with a hole for 0.96" display. Mounts on the front of the body. Locks in with the counterpart and screws with pad, 4 pc.
9. **`body/front-glass-display-support.3mf`**\
  The counterpart of the front panel with a display hole. Used to mount the display and front panel to the front of the body.
10. **`body/splash.3mf`**\
  Cover for the standard add-on slot when the add-on is not installed.

### Body parts desing

The parts have been designed to maintain maximum compatibility with existing SMARS robot parts while keeping modularity and extensibility.

Both chassis add-on slots are now occupied by the frame, but the frame itself has the same slot at the back into which any compatible add-on (or cover) can be inserted.

The absence of a second slot is compensated by the possibility of installing different face and front panels. At the same time, small-sized add-ons, for example, a distance sensor, can even be placed inside the body.

Since sMARS rover is an open-source project and includes source files of 3D models, you can design face and front panels specifically for your add-ons. Any contributions are welcome.

If you can't develop your own model, the dimensions of the face panel allow placing in it a standard slot for SMARS robot addons. The face panel with the standard slot is under development and will be available soon.

The front panel (windshield) was also conceived as a replaceable part for installing various displays, cameras, and other devices. For greater flexibility, the body is designed with a screw-mounted front panel. The thread is cut by the screw itself directly into the plastic. If frequent replacement of the front panel is planned, or the threads have been damaged, the design allows you to press in the injection nut M3x4.

The back door allows quick access to the electronics. This is convenient for flashing Arduino, and also if your robot has a switch.

### Parts in progress

**Front panel for ESP32-CAM mount**

The body was designed with the possibility of using ESP32-CAM as the control board. ESP32-CAM is much powerful than Arduino, already holds Wi-Fi and Bluetooth onboard, and allows to transmit video from the camera over Wi-Fi in real-time! Sounds good? Stay tuned and welcome to help with development.

**Face panel with a standard slot for SMARS robot add-ons**

Coming soon.

Chassis for 14500 batteries
---------------------------

### Parts

1. **`chassis-14500/chasis-14500.3mf`**\
  This chassis allows 14500 batteries to be used to power the robot. The batteries are placed in a niche and closed with a cover on the bottom. This allows batteries to be replaced without disassembling the robot. This chassis also has room for the MX1508 motor driver.
2. **`chassis-14500/chassis-14500-battcon.3mf`**\
  This part allows you to attach a nickel strip to it to make contact with the batteries. This part is specially designed to be detachable to make it easier to install the strip and solder wires to it. There are 2 slots for the nickel strip on the opposite side of the contact pad, on the chassis, to complete the circuit.
3. **`chassis-14500/chassis-14500-cover.3mf`**\
  Battery niche cover. The cover is fixed with a screw with pad (screws for computer case), 1 pc. The thread is cut by the screw itself directly into the plastic. For frequent use, it is recommended to press in the injection nut M3x4.
4. **`chassis-14500/chassis-14500-locker-big.3mf`**\
  Large motor niche locker. Allows you to install the MX1508 motor driver.
5. **`chassis-14500/chassis-14500-locker-small.3mf`**\
  Small motor niche locker.

### Motors and wheels mount

The chassis allows the installation of 2 or 4 N20 motors with the gearbox. When using the MX1508 motor driver, it may not be possible to route the wires from the motors located in the large niche. However, 2 motors are usually enough.

When using 2 motors, the slave wheels should be mounted on M4 or M3 screws using adapters. Adapters from SMARS Chassis_S Mk2 mod [thingiverse.com/thing:2829401](https://www.thingiverse.com/thing:2829401) are suitable. It is recommended to use M4 screws, as with them the wheel play will be significantly less. Screws with a length of 20 to 30mm (2pc) are suitable, it is advisable to use self-locking nuts M4 (2pc) to prevent loosening of the screws.

Also, in the case of using 2 motors, it is recommended to install both motors either in front or in the back (in the case of the MX1508 motor driver, only in the back) in order to reduce possible deviations to one side when driving in a straight line.

Wheels
------

### Parts

1. **`wheels/wheel-master-lock.3mf`**\
  Master wheel with lock. Orginal master wheel mount method completely based on friction. The motor shaft should be pushed to the wheel hole. The hole must be very precisely adjusted to the size of the motor shaft, which is not so easy to achieve with 3D printing, and the shape of the hole does not allow mechanical processing. If the hole is larger than the shaft, then the wheel will constantly fall off. If the hole is very tight, then the effort required to put on and remove the wheel can damage the motor gearbox. For this reason, two of my gearboxes had been damaged. This wheel has been designed to avoid installation and removal problems. These wheels require flat head screws M3x8-14 (2pc) and square nuts M3 (2pc).
2. **`wheels/wheel-master.3mf`**\
  Master wheel without a lock. A copy of the original model but the shape of the hole on the outside has been slightly changed.
3. **`wheels/wheel-slave.3mf`**\
  Slave wheel. The shape of the teeth and edge of the original slave wheel, slightly, but still differed from the master one. This model uses the same shape as the master wheel in order to minimize possible deviations during straight motion. The hole on the outside has also been slightly enlarged to fit the M4 screw head. It is highly recommended to use M4 screws to install these wheels.

Reflected track
---------------

1. **`track/track-normal.3mf`**\
  Copy of original track.
2. **`track/track-reflected.3mf`**\
  Reflected track. The original track is not symmetrical. Therefore, in order to prevent deviation during straight-line movement, it is advisable to use mirrored tracks on the right and left. The model of the original track can be mirrored in a slicer, you will get the same thing. This model is attached to the project for convenience, and to explain the meaning of its use.

3mf format
----------

What is the 3mf format? It is a better alternative for stl format. You can know more from this video [youtu.be/4KSB38FYREo](https://youtu.be/4KSB38FYREo).

For this particular project, it will be no big difference between using 3mf instead of stl. If you are happier by using stl for some reason, stl copies are also included. Mainly stl files were added for generating 3d previews for the project on Thingiverse, however, it can be used for print as well. 

Thingiverse project
-------------------

sMARS rover project on Thingiverse was created by files from `3d-print` folder of this repository. It is just an alternative way of getting files.


