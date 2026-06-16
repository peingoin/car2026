/* ============================================================================
 *  arduino_motors.ino  —  RC car motor driver
 *
 *  Role:
 *    1. Listen on a SoftwareSerial port for commands from the ESP32.
 *    2. Parse "D <left> <right>\n"  (each -255..255)  and  "S\n" (stop).
 *    3. Drive the motor driver accordingly.
 *    4. Failsafe: if no command for FAILSAFE_MS, stop the motors.
 *
 *  Supports TB6612FNG, L293D/L298N (enable-pin style), and DRV8833.
 *  Set MOTOR_DRIVER below and check the pin map. See docs/WIRING.md.
 *
 *  Board: Arduino Uno / Nano (5V).
 * ========================================================================== */

#include <SoftwareSerial.h>
#include <Servo.h>

// ---- Choose your driver ----------------------------------------------------
#define TB6612  0
#define L293D   1     // also covers L298N (uses enable pins as PWM)
#define DRV8833 2

#define MOTOR_DRIVER L293D      // <-- set to TB6612, L293D, or DRV8833

// ---- Pin map ---------------------------------------------------------------
// Serial link to ESP32 (SoftwareSerial)
const uint8_t PIN_ESP_RX = 11;   // <- ESP32 TX (GPIO17)
const uint8_t PIN_ESP_TX = 12;   // -> ESP32 RX (GPIO16) via level shift! (only for telemetry)

// Right motor   (L293D: EN -> R_PWM, inputs 1A/2A -> R_IN1/R_IN2)
uint8_t R_PWM = 10;               // PWM speed pin — MUST be a PWM pin (3,5,6,9,10,11)
uint8_t R_IN1 = 7;               // direction 1
uint8_t R_IN2 = 9;               // direction 2
// Left motor    (L293D: EN -> L_PWM, inputs 3A/4A -> L_IN1/L_IN2)
uint8_t L_PWM = 5;               // PWM speed pin — MUST be a PWM pin
uint8_t L_IN1 = 3;               // direction 1
uint8_t L_IN2 = 4;               // direction 2
// Standby (TB6612 only). Bare L293D has NO standby pin -> 255 disables this.
uint8_t STBY_PIN = 255;

// DS3218 270° servo — pulse range 500–2500µs, center 1500µs.
const uint8_t SERVO_PIN       = 6;
const int     SERVO_MIN_US    = 500;
const int     SERVO_MAX_US    = 2500;
const int     SERVO_CENTER_US = 1500;
const bool    SERVO_REVERSED  = false;  // set true if wheels turn the wrong way

// NOTE for DRV8833: it has no PWM/standby pins — PWM is applied on the input
// pins, which must therefore be PWM-capable (Uno: 3,5,6,9,10,11). Change the
// R_IN*/L_IN* numbers above to, e.g., R_IN1=5,R_IN2=6,L_IN1=9,L_IN2=10.

// ---- Behavior --------------------------------------------------------------
const unsigned long FAILSAFE_MS = 400;   // stop if no command for this long
const bool INVERT_LEFT  = false;         // flip if the left wheel runs backwards
const bool INVERT_RIGHT = false;         // flip if the right wheel runs backwards
const bool TELEMETRY    = false;         // echo applied speeds back to ESP32

// inverse_logic = true  -> matches the ESP32's inverted TX (keeps the ESP32-CAM
// GPIO4 flash LED dark). Both ends MUST use the same inversion setting.
SoftwareSerial espSerial(PIN_ESP_RX, PIN_ESP_TX, true);
Servo steerServo;

unsigned long lastCmdMs = 0;
char lineBuf[48];
uint8_t lineLen = 0;

// ---- Low-level motor control ----------------------------------------------
// speed: -255..255  (sign = direction, magnitude = PWM duty)
void driveMotor(uint8_t pwmPin, uint8_t in1, uint8_t in2, int speed) {
  bool forward = speed >= 0;
  int s = constrain(abs(speed), 0, 255);

#if MOTOR_DRIVER == DRV8833
  // PWM the active input; hold the other low (fast-decay style).
  if (forward) { analogWrite(in1, s); digitalWrite(in2, LOW); }
  else         { digitalWrite(in1, LOW); analogWrite(in2, s); }
  (void)pwmPin;
#else
  // TB6612 / L293D / L298N: direction on in1/in2, speed on the PWM/enable pin.
  if (s == 0) {
    // Real stop: both direction pins LOW so the motor coasts even if the
    // driver's ENABLE pin is jumpered permanently HIGH.
    digitalWrite(in1, LOW);
    digitalWrite(in2, LOW);
    analogWrite(pwmPin, 0);
  } else {
    digitalWrite(in1, forward ? HIGH : LOW);
    digitalWrite(in2, forward ? LOW  : HIGH);
    analogWrite(pwmPin, s);
  }
#endif
}

void setMotors(int left, int right) {
  if (INVERT_LEFT)  left  = -left;
  if (INVERT_RIGHT) right = -right;
  driveMotor(L_PWM, L_IN1, L_IN2, left);
  driveMotor(R_PWM, R_IN1, R_IN2, right);

  if (TELEMETRY) { espSerial.print("T "); espSerial.print(left);
                   espSerial.print(' ');  espSerial.println(right); }
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
  pinMode(R_IN1, OUTPUT); pinMode(R_IN2, OUTPUT);
  pinMode(L_IN1, OUTPUT); pinMode(L_IN2, OUTPUT);
#if MOTOR_DRIVER != DRV8833
  pinMode(R_PWM, OUTPUT); pinMode(L_PWM, OUTPUT);
#endif
  if (STBY_PIN != 255) { pinMode(STBY_PIN, OUTPUT); digitalWrite(STBY_PIN, HIGH); } // enable TB6612

  Serial.begin(115200);            // USB debug
  espSerial.begin(38400);          // link to ESP32 (SoftwareSerial-safe rate)

  steerServo.attach(SERVO_PIN, SERVO_MIN_US, SERVO_MAX_US);
  steerServo.writeMicroseconds(SERVO_CENTER_US);
  stopMotors();
  lastCmdMs = millis();
  Serial.println(F("[arduino] motor controller ready"));
}

void loop() {
  readSerial();

  // Failsafe: lost the ESP32? Stop.
  if (millis() - lastCmdMs > FAILSAFE_MS) {
    stopMotors();
    lastCmdMs = millis();         // keep stopping quietly without spamming
  }
}
