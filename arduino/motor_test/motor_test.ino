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
 *  Pin map:
 *    Right PWM = 5, Left PWM = 6, Enable = 5V (hardwired)
 * ========================================================================== */

const uint8_t RPWM = 5, LPWM = 6;   // direction PWM inputs
// Enable pins connected directly to 5V (always enabled)

const int SPEED = 180;   // 0..255 (start gentle)

// speed: -255..255  (sign = direction, magnitude = PWM duty)
void motor(int speed) {
  int s = constrain(abs(speed), 0, 255);
  if (speed >= 0) {                 // forward
    analogWrite(LPWM, 0);
    analogWrite(RPWM, s);
    Serial.print("  RPWM(pin5)="); Serial.print(s);
    Serial.print(", LPWM(pin6)=0");
  } else {                          // reverse
    analogWrite(RPWM, 0);
    analogWrite(LPWM, s);
    Serial.print("  RPWM(pin5)=0");
    Serial.print(", LPWM(pin6)="); Serial.print(s);
  }
  Serial.println();
}

void motorStop() { motor(0); }

void setup() {
  pinMode(RPWM, OUTPUT);
  pinMode(LPWM, OUTPUT);
  motorStop();
  Serial.begin(115200);
  Serial.println(F("motor_test ready - Right=5, Left=6"));

  // Pin test: blink both pins to verify they work
  Serial.println(F("Testing pin 5 (RPWM)..."));
  for(int i = 0; i < 3; i++) {
    analogWrite(RPWM, 255);
    delay(200);
    analogWrite(RPWM, 0);
    delay(200);
  }

  Serial.println(F("Testing pin 6 (LPWM)..."));
  for(int i = 0; i < 3; i++) {
    analogWrite(LPWM, 255);
    delay(200);
    analogWrite(LPWM, 0);
    delay(200);
  }

  Serial.println(F("Starting motor test cycle..."));
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
