Assembly
========

This guide walks through the assembly of the SMARS ROVER, step by step.

Part names follow the [Parts list](parts-list.md) and the [3D printable parts](3d-print.md) list. Wiring references the [rover connection scheme](./../images/rover-connection-scheme.png) and the markings printed on the custom PCB.

> **Warning: never power an ESP32 board without an antenna connected.** This applies to both the rover and the remote, and to power from USB as well as from the battery — transmitting without an antenna can damage the board. Plug an antenna in before the first power-up and leave it connected.

Rover
-----

### Step 1. Dry run and firmware (optional, recommended)

Before soldering anything permanently, it is worth checking that the electronics and the firmware work together.

**Warning:** connect an antenna to both ESP32 boards before you power them up for the first time.

1. Assemble the rover electronics on a breadboard, without soldering, following the rover connection scheme.

   ![Rover connection scheme](./../images/rover-connection-scheme.png)

2. Assemble the remote electronics on a breadboard the same way, following the remote connection scheme.

   ![Remote connection scheme](./../images/remote-connection-scheme.png)

3. Flash the `esp-mac` sketch to both ESP32 boards to read their MAC addresses. Alternatively, use [ESPConnect](https://thelastoutpostworkshop.github.io/ESPConnect/).
4. Put the remote's MAC address into the rover sketch (`software/rover-esp`, `controllerMac` in `src/main.cpp`) and flash it to the rover.
5. Put the rover's MAC address into the remote sketch (`software/remote-webcam`, `robot_mac` in `src/main.c`) and flash it to the remote.

### Step 2. Motor wiring

1. Prepare two pairs of wires, 12–15 cm long, with female JST XH2.54 crimp contacts on one end. Do not insert the contacts into the connector housings yet.

   If you are using ready-made wires with assembled connectors, either pull the contacts out of the housing or pass the free ends through the motor niche locker before soldering — the slot in the locker is too narrow for an assembled connector to pass through.

2. Recommended: slide heat shrink tubing onto the wires.
3. Solder the wires to the N20 motors. Polarity is not critical here, it can be corrected in the firmware.

   ![Motor leads soldered, twisted and heat-shrunk](./../images/assembly/rover-02-01.webp)

4. Optional: solder a 10nF capacitor directly across the motor terminals.

   If you fit the capacitors, it is recommended to wrap the whole terminal area in Kapton tape. The motor niche leaves very little room, and it is easy to short the two motors against each other.

   ![Motors with 10nF capacitors and terminals wrapped in Kapton tape](./../images/assembly/rover-02-02.webp)

5. Recommended: twist each pair of wires into a spiral.
6. Shrink the heat shrink tubing, if used.

### Step 3. Battery contacts

1. Prepare two wires 12–15 cm long and one wire 20–23 cm long, with female JST XH2.54 crimp contacts on one end.

   Recommended: use wires of different colours so the polarity stays easy to tell apart.

2. Cut two nickel strips for the positive and negative battery contacts and wrap them around the 3D-printed battery contact pad.
3. Cut one more nickel strip for the middle contact pad. Adjust its length by test-fitting it in the chassis.
4. Insert the battery contact pad and the middle contact pad into the chassis.
5. Put the batteries into the battery tray and check that they are held firmly and make reliable contact. Bend the strips to adjust the tension if needed.
6. Take the batteries back out.

   **Warning:** never put batteries against contacts that already have wires soldered to them while the connector is not yet assembled. The loose wire ends short the battery very easily.

7. To make the next steps easier, take the battery contact pad and the middle contact pad back out of the chassis.
8. Recommended: slide heat shrink tubing onto the wires.
9. Solder the two short wires to the positive and negative contacts, and the long wire to the middle contact.

   ![Battery contact pad and middle contact pad with soldered wires](./../images/assembly/rover-03-01.webp)

   ![The same contacts with heat shrink tubing applied](./../images/assembly/rover-03-02.webp)

10. Put both contact pads back into the chassis and route the wires to the outside of the battery tray. Run the middle contact wire through the dedicated channel in the battery tray.
11. Recommended: twist the wires into a spiral.
12. Assemble the 3-pin JST connector following the custom PCB markings. **The pin order is crucial** — B+, BM (middle) and B− must match the `BAT` connector on the PCB.
13. Shrink the heat shrink tubing, if used.

    ![Contacts installed in the chassis with the 3-pin battery connector assembled](./../images/assembly/rover-03-03.webp)

### Step 4. Chassis assembly

1. Heat-set an M3 insert nut for the battery cover screw.

   ![M3 insert nut heat-set into the chassis](./../images/assembly/rover-04-01.webp)

2. Place the motors into the smaller niche of the chassis. Mounted this way, the rear wheels become the master (driven) ones, which is the recommended layout.
3. Pass the wire ends with the crimp contacts through the motor niche locker.
4. Press the motor niche locker into place. It goes in tight, that is expected.
5. Assemble the 2-pin JST connectors on the motor wires.

   ![Motors installed in the chassis with the motor niche locker in place](./../images/assembly/rover-04-02.webp)

6. Recommended, to reduce backlash in the slave wheels:

   * Mount the slave wheels in their final position using the slave wheel adapters, M4x30 screws and M4 self-locking nuts.
   * Fill the slave wheel adapter niche with hot glue and close it with the slave wheel adapter niche locker.
   * Wait for the glue to set, then unscrew the M4x30 screws and take the wheels off again.

   ![Slave wheel adapter niche filled with hot glue and closed with the locker](./../images/assembly/rover-04-03.webp)

7. Push the master wheels onto the motor shafts. If you are using the wheels with a lock, secure each wheel with an M3x12 set screw and an M3 square nut.
8. Fit the tracks.
9. Put the slave wheels back in place and fasten them with the M4x30 screws.

   ![Completed chassis with wheels and tracks fitted](./../images/assembly/rover-04-04.webp)

### Step 5. Light and camera mount

The light and the camera are optional. If you are not building them, use the blank front glass or the front glass variant for an OLED screen in the next step, instead of the front glass with the light mount niche. Fitting an OLED 1.3" 128x64 I2C screen is mechanically possible — though not together with the light and the camera — but no firmware or instructions are provided for it.

1. If you are using the custom PCB, break the light board off the main board. Otherwise, use a piece of prototype board as the light board (no layout is provided for that option).
2. Solder two 100Ω resistors and two Piranha LEDs to the light board.
3. Prepare two wires 12–15 cm long with a 2-pin female JST XH2.54 connector on one end.
4. Solder the wires to the light board, observing the polarity shown in the rover connection scheme.

   ![Assembled light board with two LEDs, two resistors and a 2-pin connector](./../images/assembly/rover-05-01.webp)

5. Insert the camera into the light mount.
6. Insert the light board into the light mount.

   ![Camera and light board fitted into the light mount](./../images/assembly/rover-05-02.webp)

7. Apply a little hot glue to the back of the light board and close the light mount cover.

   ![Light mount closed with its cover](./../images/assembly/rover-05-03.webp)

8. Connect the camera to the camera sensor board of the XIAO ESP32-S3 Sense.

   ![Camera connected to the camera sensor board](./../images/assembly/rover-05-04.webp)

### Step 6. Body front assembly

Assembling the gripper requires the servos to be powered at least once, so that their gears take their initial positions. The easiest way to do this is to drive them from the assembled rover, so the gripper is left until step 9. If you have already powered the servos with the rover firmware — during step 1, for example — you can assemble the gripper now (step 9) and mount it to the body front right away.

1. Heat-set six M3 insert nuts into the body front.

   ![Six M3 insert nuts heat-set into the body front](./../images/assembly/rover-06-01.webp)

2. Place the light mount into the front glass opening of the body front.
3. Place the front glass with the light mount niche into the same opening, over the light mount.
4. Secure the front glass with four M3x5 screws with pad.

   ![Front glass and light mount installed in the body front](./../images/assembly/rover-06-02.webp)

5. Optional: until the gripper is assembled, you can temporarily fit the front add-on slot parts (top, bottom and plug) and secure them with M3x5 screws with pad.

   ![Front add-on slot parts temporarily fitted](./../images/assembly/rover-06-03.webp)

### Step 7. Main PCB

If you are not using the custom PCB, a Prototype PCB Uno R3 can be used instead (no layout is provided for that option).

1. Solder the resistors, the 220µF capacitor, the Schottky diode and the transistor, following the markings on the PCB.

   Do not fit the 100µF capacitor unless you run into problems powering the servos — it is a fallback option.

2. Solder the male JST XH2.54 connectors.
3. Solder the power switch and the pin headers.
4. Solder the female headers.

   ![Populated main PCB, component side](./../images/assembly/rover-07-01.webp)

   ![Populated main PCB, solder side](./../images/assembly/rover-07-02.webp)

5. Solder pin headers to the modules.
6. Recommended: stick two aluminum heatsinks to the XIAO ESP32-S3 Sense.
7. Plug the modules into the PCB.
8. Fix the charger module in place with hot glue.

   ![Main PCB with all modules plugged in](./../images/assembly/rover-07-03.webp)

### Step 8. Body assembly

1. Put the body base onto the chassis.

   ![Body base placed on the chassis](./../images/assembly/rover-08-01.webp)

2. Insert the main PCB into its slot and pass the motor and battery wires through the side openings.
3. Connect the motors and the battery to the PCB.

   ![Main PCB installed in the body base with motors and battery connected](./../images/assembly/rover-08-02.webp)

4. Attach the assembled body front to the body base.
5. Connect the light to its connector on the PCB.
6. Connect the camera sensor board to the XIAO ESP32-S3 Sense.
7. Secure the body front with an M3x5 screw with pad.

   ![Body front attached, light and camera connected](./../images/assembly/rover-08-03.webp)

8. Install the batteries, close the battery cover and secure it with an M3x5 screw with pad.
9. Connect an antenna to the IPEX connector on the XIAO ESP32-S3 Sense, then switch the rover on. The LEDs on the ESP32 and on the motor driver (red) should light up.

   **Warning:** never power the board without an antenna connected, not even briefly. The antenna is only mounted in its final place in step 10, so for now simply plug it into the board and leave it loose.

### Step 9. Gripper assembly

1. If you have not done it yet, put the remote's MAC address into the rover sketch (`rover-esp`) and flash it to the rover.
2. Connect both servos to the PCB and turn the rover on, with the antenna still connected. The servo gears should move to their initial positions.

   ![Both servos connected to the PCB of the assembled rover](./../images/assembly/rover-09-01.webp)

3. Turn the rover off and disconnect the servos.
4. Detach the body front from the body base, and remove the front add-on slot parts if they are fitted.
5. Insert a rubber insert into a finger.
6. Attach the finger to a rack and secure it with an M3x20 flat head screw.
7. Slide the rack with the finger into the rail of the gripper case base, moving it back and forth until the friction feels right.
8. Repeat for the second finger.
9. Run one of the rack-and-finger assemblies into the rail of the gripper case cap the same way, until the friction feels right.
10. Insert servo 2 (the gripper servo) into the gripper case base.
11. Push the servo wires into the dedicated channel in the case.
12. Place the 3D-printed gear over the servo's output gear.
13. Slide a rack with its finger into the gripper case base rail, if it is not there already, and position it so that the outer face of the finger is flush with the side of the gripper case base.
14. Trim the servo horn to match the hole in the 3D-printed gear, insert it into the gear and fasten it with the screw supplied with the servo. Try to keep the gear from turning while you do this.
15. Slide the second rack with its finger into the gripper case cap rail, and position it so that the outer face of the finger is flush with the side of the gripper case cap.
16. Put the gripper case cap in place and secure it with two M3x16 flat head screws.

    ![Assembled gripper](./../images/assembly/rover-09-02.webp)

17. Insert servo 1 (the shoulder servo) into the top part of the gripper shoulder case.
18. Insert the bottom part of the gripper shoulder case into the body front.
19. Pass the servo 2 wires through the body front.
20. Pass the top part of the gripper shoulder case through the body front.
21. Place the gripper case into the shoulder mounts.
22. Lock the gripper in position with the servo 1 horn. Trim the horn if it does not fit the niche — horns on some servos are slightly larger.
23. Fasten the horn with the screw supplied with the servo.
24. Secure the gripper shoulder with two M3x5 screws with pad.

    ![Gripper mounted on the body front](./../images/assembly/rover-09-03.webp)

### Step 10. Finishing the body

1. Attach the assembled body front to the body base.
2. Connect the light, both servos and the camera sensor board.
3. Secure the body front with an M3x5 screw with pad.
4. If you are using an external antenna, install the SMA connector into the back antenna mount and insert the mount into the back add-on slot. Otherwise, fit the back add-on slot plug.
5. Connect the antenna cable to the IPEX connector on the XIAO ESP32-S3 Sense — either the pigtail of the SMA connector, or the antenna supplied with the board if you are not using an external one.
6. Insert the back door into the body back.
7. Slide the body back into position.
8. Screw the external antenna onto the SMA connector, if you are using one.

The rover is now fully assembled.

![Fully assembled rover](./../images/assembly/rover-10-01.webp)

Remote
------

### Step 1. Remote body and power circuit

1. Heat-set six M3 insert nuts into the remote body top part.
2. Solder pin headers to the XIAO ESP32-S3.
3. Solder the power circuit following the remote connection scheme.

   Recommended: do not solder the wires from the charger and the power switch directly to the battery pads of the ESP32. Use a connector instead (JST SM 2-pin or similar), so that the board can be removed later.

   Recommended: use a small piece of prototype board to join the wires at the power switch.

4. Insert the battery holder into its niche and secure it with an M3x5 flat head screw.
5. Recommended: cut two short pieces of transparent filament and insert them into the round holes next to the two USB-C openings. They act as light pipes, carrying the light of the LEDs on the modules to the outside of the remote body.
6. Insert the charger into its niche. It should be a firm fit, no extra fixing is needed.
7. Insert the power switch into its niche and fix it with hot glue.

   ![Remote body top part with the power circuit, charger, switch and battery holder installed](./../images/assembly/remote-01-01.webp)

### Step 2. Controls and electronics

1. Pull the button caps off the joystick shield and refit them onto the remote button connectors.
2. Place the small buttons into their niches in the body top part.
3. Double-check that the joystick shield is set to 3.3 V mode.
4. Put the joystick shield in place and secure it with three M3x5 screws with pad.
5. Connect the ESP32 to the joystick shield using wires with female Dupont connectors, following the remote connection scheme.

    ![Joystick shield installed in the remote body top part](./../images/assembly/remote-02-01.webp)

6. If you are using an external antenna, mount the SMA connector into the hole in the remote body bottom part. Otherwise, stick the antenna supplied with the XIAO ESP32-S3 onto the internal antenna pad and insert the pad into its place.
7. Connect the antenna to the IPEX connector on the ESP32.

   **Warning:** the antenna has to be in place before the board is powered in the next step.

8. If you used a connector on the ESP32 battery pads, connect the ESP32 to the power circuit.
9. Insert the ESP32 into its place. It should be a firm fit, no extra fixing is needed.
10. Bend the X and Y axis pin headers on the joystick shield to 45°, so that the body bottom part can seat fully.

    ![Remote electronics installed, with the SMA connector mounted in the body bottom part](./../images/assembly/remote-02-02.webp)

11. Fit the remote body bottom part, starting from the antenna side.
12. Secure the body bottom part with two M3x5 screws.

    ![Assembled remote](./../images/assembly/remote-02-03.webp)

### Step 3. Phone holder

1. Slide the phone holder cap into the base. Do it slowly — if the fit feels too tight, sand the back side of the cap with a file or sandpaper to reduce the tension.
2. Fit two rubber bands to hold the cap down against the base.
3. Insert all three rubber inserts into their niches — two in the base, one in the cap.
4. Slide the assembled holder into the slots on the remote body. Do it gently: it can be tight the first time, and sliding it in and out a couple of times loosens it up.
5. Screw the external antenna onto the SMA connector, if you are using one.

The remote is now fully assembled.

![Remote with the phone holder attached](./../images/assembly/remote-03-01.webp)
