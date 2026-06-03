# Wiring guide

Three things get wired together: **ESP32 ⇄ Arduino** (serial), **Arduino ⇄ motor driver**,
and **motor driver ⇄ motors + battery**.

> ⚠️ **Most important rule:** The Arduino runs at **5V logic**, the ESP32 at **3.3V logic**.
> Sending 5V into an ESP32 pin can damage it. See the level-shifting section below.

---

## 1. ESP32 ⇄ Arduino (UART serial)

For an **ESP32-CAM** we send commands out on **GPIO4** to the Arduino's `SoftwareSerial`
RX (pin 11). GPIO4 is *also* the onboard flash LED, so the serial line is run
**inverted** (idles LOW) to keep that LED dark — both firmwares set inversion on.

| ESP32-CAM pin | direction | Arduino pin | Notes |
|-----------|-----------|-------------|-------|
| GPIO4 (info out / TX, inverted) | ──▶ | pin 11 (RX) | 3.3V → 5V. Arduino reads 3.3V as HIGH, **no shifting needed.** |
| GND | ── | GND | **Common ground is required.** |

> The flash LED only flickers faintly while you're actively driving (data bursts) and
> is fully off when idle. Telemetry (Arduino → ESP32) is disabled, so only this one
> signal wire + ground are needed.

> ⚠️ **ESP32-CAM has no USB.** You flash it with a USB-to-TTL (FTDI) adapter wired to
> U0T/U0R (GPIO1/GPIO3) + 5V + GND, with **GPIO0 tied to GND** while uploading.
> Remove the GPIO0–GND jumper and reset to run. The same U0T/U0R pins carry the
> `Serial` debug log.

### Level shifting the Arduino TX → ESP32 RX line

The Arduino's TX idles/drives at 5V. The ESP32 RX pin is only 3.3V tolerant.
Use **either**:

- **A logic level converter module** (cleanest), or
- **A resistor divider** on that one wire:

```
Arduino pin 11 (TX, 5V) ──[ 1kΩ ]──┬──▶ ESP32 GPIO16 (RX)
                                    │
                                  [ 2kΩ ]
                                    │
                                   GND
```
This drops 5V to ~3.3V. (Only needed because the Arduino *talks back* for telemetry.
If you never read telemetry on the ESP32 you can skip this wire entirely.)

### Powering the two boards
- Easiest: power the Arduino from a battery/regulator, and power the ESP32 from the
  Arduino's **5V pin → ESP32 VIN** (the ESP32 board's onboard regulator makes 3.3V).
- Make sure **all grounds are common**: ESP32 GND, Arduino GND, motor driver GND,
  and battery − all tied together.

---

## 2. Arduino ⇄ Motor driver

Default pin map (used by `arduino_motors.ino`). Works for **TB6612FNG** and
**L293D / L298N (enable-pin style)**:

| Function | Arduino pin | Driver pin (TB6612) | Driver pin (L293D/L298N) |
|----------|-------------|---------------------|--------------------------|
| Right motor speed (PWM) | 5  | PWMA | ENA / EN1 |
| Right motor dir 1 | 7  | AIN1 | IN1 |
| Right motor dir 2 | 8  | AIN2 | IN2 |
| Left motor speed (PWM)  | 6  | PWMB | ENB / EN2 |
| Left motor dir 1  | 4  | BIN1 | IN3 |
| Left motor dir 2  | 2  | BIN2 | IN4 |
| Standby/enable | 12 | STBY | (tie ENA/ENB high, or ignore) |

> **PWM pins:** On the Arduino Uno/Nano, only pins **3, 5, 6, 9, 10, 11** can do PWM
> (`analogWrite`). The speed pins above (5, 6) are valid PWM pins.

### If you use a DRV8833
The DRV8833 has **no enable/PWM pin** — speed is PWM'd directly on the input pins,
so all four input pins must be PWM-capable. In `arduino_motors.ino` set
`#define MOTOR_DRIVER DRV8833` and use these pins:

| Function | Arduino pin | DRV8833 pin |
|----------|-------------|-------------|
| Right motor in 1 | 5  | AIN1 |
| Right motor in 2 | 6  | AIN2 |
| Left motor in 1  | 9  | BIN1 |
| Left motor in 2  | 10 | BIN2 |

(The code reads these from the same `R_IN1/R_IN2/L_IN1/L_IN2` defines — just change the
pin numbers near the top of the sketch to the PWM-capable ones shown here.)

---

## 3. Motor driver ⇄ motors + battery

- **Motor power**: connect your motor battery (e.g. 6V–12V pack) to the driver's
  motor-supply input (VM / VCC / +12V) and GND. **Do NOT power motors from the
  Arduino's 5V pin** — motors draw too much current and will brown out the logic.
- **Logic power**: the driver's logic pin (VCC / +5V / VIO) goes to Arduino 5V
  (TB6612 VCC = logic 2.7–5.5V; tie its VM to motor battery).
- **Motors**: left motor to the A (or B) output pair, right motor to the other pair.
  If a wheel spins the wrong way, swap that motor's two wires (or flip
  `INVERT_LEFT`/`INVERT_RIGHT` in the sketch).
- **Common ground**: driver GND ↔ Arduino GND ↔ battery −.

---

## Wiring sanity checklist

- [ ] All grounds tied together (ESP32, Arduino, driver, battery).
- [ ] Arduino TX (pin 11) → ESP32 RX (GPIO16) goes through a divider / level shifter.
- [ ] Motors powered from the battery, **not** from the Arduino.
- [ ] Driver standby/enable handled (STBY to pin 12, or EN pins high).
- [ ] First test with wheels **off the ground**.
