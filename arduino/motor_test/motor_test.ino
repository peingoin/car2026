/* ============================================================================
 *  motor_test.ino  —  Wiring sanity check for a single BTS7960 (no ESP32, no phone)
 *
 *  Cycles the motor: forward 2s, stop 1s, reverse 2s, stop 1s, repeat.
 *  Watch the Serial Monitor (115200) to see which direction is active.
 *
 *  Use this FIRST to confirm motor wiring and direction before running the
 *  full system. If the motor spins the wrong way, note it (you'll set
 *  INVERT_MOTOR in arduino_motors.ino, or just swap the motor's two leads).
 *
 *  Pin map MUST match arduino_motors.ino:
 *    BTS7960:  RPWM=3, LPWM=9, R_EN=5, L_EN=6
 * ========================================================================== */

const uint8_t RPWM = 3, LPWM = 9;   // direction PWM inputs
const uint8_t R_EN = 5, L_EN = 6;   // enable pins (held HIGH)

const int SPEED = 180;   // 0..255 (start gentle)

// speed: -255..255  (sign = direction, magnitude = PWM duty)
void motor(int speed) {
  int s = constrain(abs(speed), 0, 255);
  if (speed >= 0) {                 // forward
    analogWrite(LPWM, 0); analogWrite(RPWM, s);
  } else {                          // reverse
    analogWrite(RPWM, 0); analogWrite(LPWM, s);
  }
}

void motorStop() { motor(0); }

void setup() {
  uint8_t pins[] = { RPWM, LPWM, R_EN, L_EN };
  for (uint8_t p : pins) pinMode(p, OUTPUT);
  digitalWrite(R_EN, HIGH);
  digitalWrite(L_EN, HIGH);
  motorStop();
  Serial.begin(115200);
  Serial.println(F("motor_test ready (BTS7960)"));
}

void step(const char *label, int speed, int ms) {
  Serial.println(label);
  motor(speed);
  delay(ms);
}

void loop() {
  step("forward", SPEED, 2000);  step("stop", 0, 1000);
  step("reverse", -SPEED, 2000); step("stop", 0, 1000);
}
