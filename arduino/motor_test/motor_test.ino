/* ============================================================================
 *  motor_test.ino  —  Wiring sanity check for a bare L293D (no ESP32, no phone)
 *
 *  Cycles each motor: forward 2s, stop 1s, reverse 2s, stop 1s, repeat.
 *  Watch the Serial Monitor (115200) to see which motor/direction is active.
 *
 *  Use this FIRST to confirm motor wiring and direction before running the
 *  full system. If a motor spins the wrong way, note it (you'll set
 *  INVERT_LEFT / INVERT_RIGHT in arduino_motors.ino).
 *
 *  Pin map MUST match arduino_motors.ino:
 *    Right: EN=6, IN1=7, IN2=9     Left: EN=5, IN1=3, IN2=4
 * ========================================================================== */

const uint8_t R_PWM = 6, R_IN1 = 7, R_IN2 = 9;   // right motor
const uint8_t L_PWM = 5, L_IN1 = 3, L_IN2 = 4;   // left motor

const int SPEED = 180;   // 0..255 (start gentle)

void motor(uint8_t pwm, uint8_t in1, uint8_t in2, int speed) {
  bool fwd = speed >= 0;
  int s = constrain(abs(speed), 0, 255);
  if (s == 0) {                       // real stop (coast)
    digitalWrite(in1, LOW); digitalWrite(in2, LOW); analogWrite(pwm, 0);
  } else {
    digitalWrite(in1, fwd ? HIGH : LOW);
    digitalWrite(in2, fwd ? LOW  : HIGH);
    analogWrite(pwm, s);
  }
}

void bothStop() { motor(R_PWM, R_IN1, R_IN2, 0); motor(L_PWM, L_IN1, L_IN2, 0); }

void setup() {
  uint8_t pins[] = { R_PWM, R_IN1, R_IN2, L_PWM, L_IN1, L_IN2 };
  for (uint8_t p : pins) pinMode(p, OUTPUT);
  bothStop();
  Serial.begin(115200);
  Serial.println(F("motor_test ready"));
}

void step(const char *label, int rSpeed, int lSpeed, int ms) {
  Serial.println(label);
  motor(R_PWM, R_IN1, R_IN2, rSpeed);
  motor(L_PWM, L_IN1, L_IN2, lSpeed);
  delay(ms);
}

void loop() {
  step("RIGHT forward",  SPEED, 0, 2000);  step("stop", 0, 0, 1000);
  step("RIGHT reverse", -SPEED, 0, 2000);  step("stop", 0, 0, 1000);
  step("LEFT forward",  0,  SPEED, 2000);  step("stop", 0, 0, 1000);
  step("LEFT reverse",  0, -SPEED, 2000);  step("stop", 0, 0, 1000);
  step("BOTH forward",  SPEED, SPEED, 2000); step("stop", 0, 0, 1500);
}
