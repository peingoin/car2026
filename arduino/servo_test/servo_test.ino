/* ============================================================================
 *  servo_test.ino - Test servo range in 100µs increments
 *
 *  Purpose: Find the exact min/max microsecond values for your servo.
 *  This sketch sweeps the servo from 500µs to 2500µs in 100µs steps,
 *  pausing at each position so you can see the movement.
 *
 *  Board: Arduino Uno/Nano
 *  Servo: Connect to pin 10
 * ========================================================================== */

#include <Servo.h>

const uint8_t SERVO_PIN = 10;

const int SERVO_MIN_TEST = 500;    // Start position (µs)
const int SERVO_MAX_TEST = 2500;   // End position (µs)
const int SERVO_STEP = 100;        // Step size (µs)
const int PAUSE_MS = 1000;         // Pause at each position (ms)

Servo testServo;

void setup() {
  Serial.begin(115200);
  Serial.println(F("\n========================================"));
  Serial.println(F("Servo Range Test"));
  Serial.println(F("========================================"));
  Serial.print(F("Range: "));
  Serial.print(SERVO_MIN_TEST);
  Serial.print(F("µs to "));
  Serial.print(SERVO_MAX_TEST);
  Serial.println(F("µs"));
  Serial.print(F("Step: "));
  Serial.print(SERVO_STEP);
  Serial.println(F("µs"));
  Serial.print(F("Pause: "));
  Serial.print(PAUSE_MS);
  Serial.println(F("ms per step\n"));

  testServo.attach(SERVO_PIN, SERVO_MIN_TEST, SERVO_MAX_TEST);

  // Start at center
  Serial.println(F("Moving to center (1500µs)..."));
  testServo.writeMicroseconds(1500);
  delay(2000);

  Serial.println(F("Starting sweep...\n"));
}

void loop() {
  // Sweep from min to max
  Serial.println(F("=== Sweeping MIN → MAX ==="));
  for (int us = SERVO_MIN_TEST; us <= SERVO_MAX_TEST; us += SERVO_STEP) {
    testServo.writeMicroseconds(us);
    Serial.print(F("Position: "));
    Serial.print(us);
    Serial.println(F("µs"));
    delay(PAUSE_MS);
  }

  Serial.println();
  delay(1000);

  // Sweep from max to min
  Serial.println(F("=== Sweeping MAX → MIN ==="));
  for (int us = SERVO_MAX_TEST; us >= SERVO_MIN_TEST; us -= SERVO_STEP) {
    testServo.writeMicroseconds(us);
    Serial.print(F("Position: "));
    Serial.print(us);
    Serial.println(F("µs"));
    delay(PAUSE_MS);
  }

  Serial.println();
  delay(1000);

  // Return to center and pause before next cycle
  Serial.println(F("Returning to center..."));
  testServo.writeMicroseconds(1500);
  delay(3000);
  Serial.println(F("\n--- Starting new cycle ---\n"));
}
