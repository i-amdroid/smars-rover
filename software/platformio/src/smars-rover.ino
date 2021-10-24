#define CUSTOM_SETTINGS
#define INCLUDE_GAMEPAD_MODULE
#include <Dabble.h>
//#include <Servo.h>

// Defining pins

// Bluetooth pins initialized in Dabble lib
// const int bt_pin_rx = 3; // Bluetooth: RX
// const int bt_pin_tx = 2; // Bluetooth: TX

// Important: Left motor is motor A 
const int mb_pin_in1 = 6; // Motor board: IN1, PWM
const int mb_pin_in2 = 7; // Motor board: IN2
const int mb_pin_in3 = 5; // Motor board: IN3, PWM
const int mb_pin_in4 = 4; // Motor board: IN4

const int led_pin = 8; // LED: plus (power)

const int ds_pin_echo = 10; // Distance sensor: Echo
const int ds_pin_trig = 9;  // Distance sensor: Trig

//const int srv_pin = 11; // Servo: signal

const int buz_pin = 12; // Buzzer: plus (signal and power)

// const int lcd_pin_sda = 18; // pin A4 // LCD: SDA
// const int lcd_pin_sck = 19; // pin A5 // LCD: SCK

// Defining motor control properties

int mb_speed_initial = 80;                 // mb_speed_initial + mb_gear * mb_speed_multiplier should be < 255
int mb_speed_current;
int mb_speed_multiplier = 30;
int mb_gear = 0;                           // max gear is 4
int mb_gear_counter = 0;        
int mb_gear_treshhold = 300;               // Number of iterations button should be pressed for changing speed number
unsigned long mb_gear_timer = 0;

/*
// Defining servo motor properties

Servo srv;

int srv_angle = 60;    // initial angle for servo
int srv_angle_step = 1;
const int srv_angle_min = 0;
const int srv_angle_max = 120;
const int srv_step_delay = 10; // delay for the servo movements
bool srv_init = false;
unsigned long srv_timer = 0;
*/

// Defining buzzer properties

bool buz_on = false;
const int buz_duration = 100; // duration of one beep
int buz_delay = -1; // delay between beeps: -1 - no sound; 100 - permanent; 500, 1000, 2000, 3000 - frequency of beep;
unsigned long buz_timer = 0;

// Defining LED properties

bool led_on = false;
const int led_delay = 500; // delay for determining button pressing
unsigned long led_timer = 0;

// Defining Distance sensor properties

long ds_duration = 0;
int ds_distance = 0;
unsigned long ds_timer = 0;

void setup() {
  //Serial.begin(9600);

  // Initializing Bluetoth gamepad
  Dabble.begin(9600);

  // Initializing motors
  pinMode(mb_pin_in1, OUTPUT);
  pinMode(mb_pin_in2, OUTPUT);
  pinMode(mb_pin_in3, OUTPUT);
  pinMode(mb_pin_in4, OUTPUT);

  mb_speed_current = mb_speed_initial;

  /*
  // Initializing servo, setting initial angle.
  srv.attach(srv_pin);
  srv.write(srv_angle); // Moving the servo to initial angle.
  srv_timer = millis();
  if (millis() >= srv_timer + 1000) {
    srv.detach();
    srv_init = true;
  }
  */

  // Initializing buzzer
  pinMode(buz_pin, OUTPUT);
  buz_timer = millis();

  // Initializing LED
  pinMode(led_pin, OUTPUT);
  led_timer = millis();

  // Initializing Distance sensor
  pinMode(ds_pin_trig, OUTPUT);
  pinMode(ds_pin_echo, INPUT);
}

void loop() {
  Dabble.processInput();

  // Gearbox

  if (millis() - mb_gear_timer >= 4) {
    // Increasing counter while forward button is pressed.
    if (GamePad.isUpPressed()) {
      // Increasing counter only if it < treshhold.
      if (mb_gear_counter < mb_gear_treshhold) {
        mb_gear_counter++;
      }

      // Using 4ms delay for increasing counter.
      mb_gear_timer = millis();
    }
    // Decreasing counter while forward button is not pressed.
    else {
      // Decreasing counter only if it >= 0.
      // -1 means reseting gear.
      if (mb_gear_counter >= 0) {
        mb_gear_counter--;
      }

      // Using 1ms delay for decreasing counter.
      mb_gear_timer = millis() + 3;
    }
  }

  // Increasing gear if counter meet treshhold.
  if (mb_gear_counter >= mb_gear_treshhold && mb_gear < 4) {
    mb_gear++;
    mb_gear_counter = 0; // Reseting counter for starting new gear count.
  }

  // Reseting gear if forward button not pressed for some time.
  if (mb_gear_counter < 0 && mb_gear > 0) {
    mb_gear = 0;
  }

  // Calculating speed for moving forward and turns on the run. 
  mb_speed_current = mb_speed_initial + mb_gear * mb_speed_multiplier;
  
  // Handling movement motors

  /*
  // Simple (discrete/digital) control, one speed (max).
  // Forward
  if (GamePad.isUpPressed()) {
    //Serial.println("up");
    digitalWrite(mb_pin_in1, HIGH); //
    digitalWrite(mb_pin_in3, HIGH); //
    digitalWrite(mb_pin_in2, LOW);
    digitalWrite(mb_pin_in4, LOW);
  }
  // Backward
  if (GamePad.isDownPressed()) {
    //Serial.println("down");
    digitalWrite(mb_pin_in1, LOW);
    digitalWrite(mb_pin_in3, LOW);
    digitalWrite(mb_pin_in2, HIGH);
    digitalWrite(mb_pin_in4, HIGH);
  }
  // Left
  if (GamePad.isLeftPressed() || GamePad.isSquarePressed()) {
    //Serial.println("left");
    digitalWrite(mb_pin_in1, LOW);
    digitalWrite(mb_pin_in3, HIGH);
    digitalWrite(mb_pin_in2, HIGH);
    digitalWrite(mb_pin_in4, LOW);
  }
  // Right
  if (GamePad.isRightPressed() || GamePad.isCirclePressed()) {
    //Serial.println("right");
    digitalWrite(mb_pin_in1, HIGH);
    digitalWrite(mb_pin_in3, LOW);
    digitalWrite(mb_pin_in2, LOW);
    digitalWrite(mb_pin_in4, HIGH);
  }
  // Stop
  if (!GamePad.isUpPressed() && !GamePad.isDownPressed() && !GamePad.isLeftPressed() && !GamePad.isRightPressed() && !GamePad.isSquarePressed() && !GamePad.isCirclePressed()) {
    //Serial.println("stop");
    digitalWrite(mb_pin_in1, LOW);
    digitalWrite(mb_pin_in3, LOW);
    digitalWrite(mb_pin_in2, LOW);
    digitalWrite(mb_pin_in4, LOW);
  }
  */
  
  // Advanced (PWM/analog) control, 5 levels of speed
  // Motor A is right
  // Forward
  if (GamePad.isUpPressed()) {
    //Serial.println("up");
    analogWrite(mb_pin_in3, mb_speed_current);
    analogWrite(mb_pin_in1, mb_speed_current);
    digitalWrite(mb_pin_in2, LOW);
    digitalWrite(mb_pin_in4, LOW);
  }
  // Backward
  if (GamePad.isDownPressed()) {
    //Serial.println("down");
    analogWrite(mb_pin_in1, 215 - mb_speed_initial);
    analogWrite(mb_pin_in3, 215 - mb_speed_initial);
    digitalWrite(mb_pin_in2, HIGH);
    digitalWrite(mb_pin_in4, HIGH);
  }
  // Left on the run forward 
  if ((GamePad.isLeftPressed() || GamePad.isSquarePressed()) && GamePad.isUpPressed()) {
    //Serial.println("left");
    analogWrite(mb_pin_in1, LOW);
    analogWrite(mb_pin_in3, mb_speed_current);
    digitalWrite(mb_pin_in2, LOW);
    digitalWrite(mb_pin_in4, LOW);
  }
  // Right on the run forwar
  if ((GamePad.isRightPressed() || GamePad.isCirclePressed()) && GamePad.isUpPressed())  {
    //Serial.println("right");
    analogWrite(mb_pin_in1, mb_speed_current);
    analogWrite(mb_pin_in3, LOW);
    digitalWrite(mb_pin_in2, LOW);
    digitalWrite(mb_pin_in4, LOW);
  }
  // Left on the run backward 
  if ((GamePad.isLeftPressed() || GamePad.isSquarePressed()) && GamePad.isDownPressed()) {
    //Serial.println("left");
    analogWrite(mb_pin_in1, LOW);
    analogWrite(mb_pin_in3, 215 - mb_speed_initial);
    digitalWrite(mb_pin_in2, LOW);
    digitalWrite(mb_pin_in4, HIGH);
  }
  // Right on the run backward
  if ((GamePad.isRightPressed() || GamePad.isCirclePressed()) && GamePad.isDownPressed()) {
    //Serial.println("right");
    analogWrite(mb_pin_in1, 215 - mb_speed_initial);
    analogWrite(mb_pin_in3, LOW);
    digitalWrite(mb_pin_in2, HIGH);
    digitalWrite(mb_pin_in4, LOW);
  }
  // Left on stop
  if ((GamePad.isLeftPressed() || GamePad.isSquarePressed()) && !GamePad.isUpPressed() && !GamePad.isDownPressed()) {
    //Serial.println("left");
    analogWrite(mb_pin_in1, 215 - mb_speed_initial);
    analogWrite(mb_pin_in3, mb_speed_initial);
    digitalWrite(mb_pin_in2, HIGH);
    digitalWrite(mb_pin_in4, LOW);
  }
  // Right on stop
  if ((GamePad.isRightPressed() || GamePad.isCirclePressed()) && !GamePad.isUpPressed() && !GamePad.isDownPressed()) {
    //Serial.println("right");
    analogWrite(mb_pin_in1, mb_speed_initial);
    analogWrite(mb_pin_in3, 215 - mb_speed_initial);
    digitalWrite(mb_pin_in2, LOW);
    digitalWrite(mb_pin_in4, HIGH);
  }
  // Stop
  if (!GamePad.isUpPressed() && !GamePad.isDownPressed() && !GamePad.isLeftPressed() && !GamePad.isRightPressed() && !GamePad.isSquarePressed() && !GamePad.isCirclePressed()) {
    //Serial.println("stop");
    analogWrite(mb_pin_in1, LOW);
    analogWrite(mb_pin_in3, LOW);
    digitalWrite(mb_pin_in2, LOW);
    digitalWrite(mb_pin_in4, LOW);
  }

  /*
  // Handling shovel servo motor

  // Detaching servo if no activity. 20 is activity waiting delay.
  if (srv_init && millis() >= srv_timer + srv_step_delay + 20) {
    srv.detach();
  }

  // Up
  if (GamePad.isTrianglePressed() ) {
    // Decreasing ange if possible.
    if (srv_angle - srv_angle_step > srv_angle_min && millis() >= srv_timer + srv_step_delay) {
      srv_angle = srv_angle - srv_angle_step; // Updating angle.
      srv_timer = millis(); // Updating timer.
      //Serial.print("rotate to ");
      //Serial.println(srv_angle);
      srv.attach(srv_pin);
      srv.write(srv_angle); // Moving the servo to desired angle.
    }
  }

  // Down
  if (GamePad.isCrossPressed()) {
    // Increasing the angle if possible.
    if (srv_angle + srv_angle_step < srv_angle_max && millis() >= srv_timer + srv_step_delay) {
      srv_angle = srv_angle + srv_angle_step; // Updating angle.
      srv_timer = millis(); // Updating timer.
      //Serial.print("rotate to ");
      //Serial.println(srv_angle);
      srv.attach(srv_pin);
      srv.write(srv_angle); // Moving the servo to desired angle.
    }
  }

  // Holding shovel in position.
  if (!GamePad.isTrianglePressed() && !GamePad.isCrossPressed()) { 
    if (millis() >= srv_timer + srv_step_delay) {
      srv_timer = millis(); // Updating timer.
      //Serial.print("rotate to ");
      //Serial.println(srv_angle);
      srv.attach(srv_pin);
      srv.write(srv_angle); // Moving the servo to desired angle.
    }
  }
  */

  // Handling LED

  // LED enabling and Disabling
  if (GamePad.isStartPressed() && millis() - led_timer > led_delay) {

    led_timer = millis(); // Updating timer.
    led_on = !led_on; // Switching state.
    // Send signal
    if (led_on) {
      digitalWrite(led_pin, HIGH);
    }
    else {
      digitalWrite(led_pin, LOW);
    }
  }

  // Handling manual buzzer control: disablng
  // Should go before Raange distance code

  if (!GamePad.isSelectPressed()) {
    buz_delay = -1; // no sound
  }

  /*
  // Handling Distance sensor

  // Parktronic mode
  if (micros() - ds_timer >= 15) {
    // Starting new cycle
    ds_timer = micros();
    // Clears the trig pin
    digitalWrite(ds_pin_trig, LOW);
    // Reads the echo pin, returns the sound wave travel time in microseconds
    ds_duration = pulseIn(ds_pin_echo, HIGH);
    // Calculating the distance
    ds_distance = ds_duration * 0.034 / 2;
    // Applying median filter
    ds_distance = median(ds_distance);
    // Prints the distance on the Serial Monitor
    //Serial.print("Distance: ");
    //Serial.println(ds_distance);
  }

  if (micros() - ds_timer >= 5) {
    // Sets the trig pin on HIGH state for 10 micro seconds
    digitalWrite(ds_pin_trig, HIGH);
  }

  // Handling buzzer control by distance sensor
  // Park tronic mode
  
  if (ds_distance <= 12) {
    buz_delay = 1000;
  }
  if (ds_distance <= 8) {
    buz_delay = 500;
  }
  if (ds_distance <= 4) {
    buz_delay = 300;
  }
  if (ds_distance <= 2) {
    buz_delay = 100;
  }
  */

  // Handling manual buzzer control: enabling
  // Should go after Raange distance code
  
  if (GamePad.isSelectPressed()) {
    buz_delay = 100; // permanent sound
  }

  // Handling buzzer logic

  // Enabling sound and setting timer for repeats and disabling
  if (buz_delay >= 100) {
    if (millis() - buz_delay >= buz_timer) {
      buz_on = true;
      buz_timer = millis();
    }
  }

  // Disabling sound
  if (millis() - buz_duration >= buz_timer) {
    buz_on = false;
  }

  // Handling buzzer hardware

  if (buz_on) {
    digitalWrite(buz_pin, HIGH);
  }
  else {
    digitalWrite(buz_pin, LOW);
  }

}

// Median filter for Distance sensor signal
int median(int newVal) {
  static int buf[3];
  static byte count = 0;
  buf[count] = newVal;
  if (++count >= 3) count = 0;
  int a = buf[0];
  int b = buf[1];
  int c = buf[2];
  int middle;
  if ((a <= b) && (a <= c)) {
    middle = (b <= c) ? b : c;
  } else {
    if ((b <= a) && (b <= c)) {
      middle = (a <= c) ? a : c;
    }
    else {
      middle = (a <= b) ? a : b;
    }
  }
  return middle;
}
