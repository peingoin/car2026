/* ============================================================================
 *  arduino_motors.ino  —  RC car motor driver
 *
 *  Role:
 *    1. Listen on a SoftwareSerial port for commands from the ESP32.
 *    2. Parse "D <left> <right>\n"  (each -255..255)  and  "S\n" (stop).
 *    3. Drive a single BTS7960 H-bridge accordingly.
 *    4. Failsafe: if no command for FAILSAFE_MS, stop the motor.
 *
 *  Driver: BTS7960 (IBT-2) full H-bridge, one DC motor.
 *  The car uses ONE drive motor; the ESP32 still sends a left/right pair, so we
 *  collapse it to a single throttle = (left + right) / 2 (steering is handled
 *  elsewhere). See docs/WIRING.md.
 *
 *  Board: Arduino Uno / Nano (5V).
 * ========================================================================== */

#include <SoftwareSerial.h>
#include <Servo.h>

// ---- Pin map ---------------------------------------------------------------
// Serial link to ESP32 (SoftwareSerial)
const uint8_t PIN_ESP_RX = 11;   // <- ESP32 TX (GPIO4)
const uint8_t PIN_ESP_TX = 12;   // -> ESP32 RX via level shift! (only for telemetry)

// BTS7960 (IBT-2) — single motor.
// RPWM/LPWM are the two direction inputs (PWM); R_EN/L_EN enable each half-bridge.
const uint8_t RPWM = 5;          // forward PWM  — MUST be a PWM pin (3,5,6,9,10,11)
const uint8_t LPWM = 6;          // reverse PWM  — MUST be a PWM pin
const uint8_t R_EN = 7;          // enable forward half-bridge (driven HIGH, always on)
const uint8_t L_EN = 8;          // enable reverse half-bridge (driven HIGH, always on)

// DS3218 270° servo — pulse range 500–2500µs, center 1500µs.
// Pin 10 used (pin 6 is taken by BTS7960 L_EN).
const uint8_t SERVO_PIN       = 10;
const int     SERVO_MIN_US    = 500;
const int     SERVO_MAX_US    = 2500;
const int     SERVO_CENTER_US = 1500;
const bool    SERVO_REVERSED  = false;  // set true if wheels turn the wrong way

// ---- Behavior --------------------------------------------------------------
const unsigned long FAILSAFE_MS = 400;   // stop if no command for this long
const bool INVERT_MOTOR = false;         // flip if the wheel runs backwards
const bool TELEMETRY    = false;         // echo applied speed back to ESP32

// inverse_logic = true  -> matches the ESP32's inverted TX (keeps the ESP32-CAM
// GPIO4 flash LED dark). Both ends MUST use the same inversion setting.
SoftwareSerial espSerial(PIN_ESP_RX, PIN_ESP_TX, true);
Servo steerServo;

unsigned long lastCmdMs = 0;
char lineBuf[48];
uint8_t lineLen = 0;

// ---- Low-level motor control ----------------------------------------------
// speed: -255..255  (sign = direction, magnitude = PWM duty)
// BTS7960: drive the active direction's PWM input, hold the other at 0.
// R_EN/L_EN stay HIGH (set in setup); a duty of 0 on both inputs coasts/stops.
void driveMotor(int speed) {
  if (INVERT_MOTOR) speed = -speed;
  int s = constrain(abs(speed), 0, 255);

  if (speed >= 0) {            // forward
    analogWrite(LPWM, 0);
    analogWrite(RPWM, s);
  } else {                     // reverse
    analogWrite(RPWM, 0);
    analogWrite(LPWM, s);
  }
}

void setMotors(int left, int right) {
  // One physical motor: throttle is the average of the requested wheel speeds.
  int speed = (left + right) / 2;
  driveMotor(speed);

  if (TELEMETRY) { espSerial.print("T "); espSerial.println(speed); }
}

void stopMotors() {
  setMotors(0, 0);
  steerServo.writeMicroseconds(SERVO_CENTER_US);
}

// ---- Command parsing -------------------------------------------------------
void handleLine(char *line) {
  // Trim leading spaces
  while (*line == ' ') line++;

  if (line[0] == 'S' || line[0] == 's') {        // stop
    stopMotors();
    lastCmdMs = millis();
    return;
  }
  if (line[0] == 'D' || line[0] == 'd') {        // drive
    int l = 0, r = 0;
    if (sscanf(line + 1, "%d %d", &l, &r) == 2) {
      l = constrain(l, -255, 255);
      r = constrain(r, -255, 255);
      setMotors(l, r);
      lastCmdMs = millis();
    }
  }
  if (line[0] == 'V' || line[0] == 'v') {        // steer (microseconds)
    int us = 0;
    if (sscanf(line + 1, "%d", &us) == 1) {
      us = constrain(us, SERVO_MIN_US, SERVO_MAX_US);
      if (SERVO_REVERSED) us = SERVO_MIN_US + SERVO_MAX_US - us;
      steerServo.writeMicroseconds(us);
      lastCmdMs = millis();
    }
  }
}

void readSerial() {
  while (espSerial.available()) {
    char c = espSerial.read();
    if (c == '\n' || c == '\r') {
      if (lineLen > 0) { lineBuf[lineLen] = '\0'; handleLine(lineBuf); lineLen = 0; }
    } else if (lineLen < sizeof(lineBuf) - 1) {
      lineBuf[lineLen++] = c;
    } else {
      lineLen = 0;            // overflow -> discard
    }
  }
}

// ---- Setup / loop ----------------------------------------------------------
void setup() {
  pinMode(RPWM, OUTPUT); pinMode(LPWM, OUTPUT);
  pinMode(R_EN, OUTPUT); pinMode(L_EN, OUTPUT);
  // Enable both half-bridges; speed/direction is controlled purely by the PWM inputs.
  digitalWrite(R_EN, HIGH);
  digitalWrite(L_EN, HIGH);

  Serial.begin(115200);            // USB debug
  espSerial.begin(38400);          // link to ESP32 (SoftwareSerial-safe rate)

  steerServo.attach(SERVO_PIN, SERVO_MIN_US, SERVO_MAX_US);
  steerServo.writeMicroseconds(SERVO_CENTER_US);
  stopMotors();
  lastCmdMs = millis();
  Serial.println(F("[arduino] motor controller ready (BTS7960)"));
}

void loop() {
  readSerial();

  // Failsafe: lost the ESP32? Stop.
  if (millis() - lastCmdMs > FAILSAFE_MS) {
    stopMotors();
    lastCmdMs = millis();         // keep stopping quietly without spamming
  }
}
