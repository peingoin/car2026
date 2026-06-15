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

Pin map (used by `arduino_motors.ino`) for a single **BTS7960 (IBT-2)** H-bridge
driving the car's one drive motor:

| Function | Arduino pin | BTS7960 pin |
|----------|-------------|-------------|
| Forward PWM | 3 | RPWM |
| Reverse PWM | 9 | LPWM |
| Enable forward half-bridge | 5 | R_EN |
| Enable reverse half-bridge | 6 | L_EN |

How it drives: `R_EN` and `L_EN` are held **HIGH** to enable the bridge. Direction
and speed come from the two PWM inputs — to go forward, PWM `RPWM` and hold `LPWM`
at 0; to reverse, PWM `LPWM` and hold `RPWM` at 0. A duty of 0 on both inputs stops
the motor.

> **Never PWM both `RPWM` and `LPWM` at once** — that shoot-throughs the bridge. The
> sketch always zeroes the inactive input first.

> **PWM pins:** On the Arduino Uno/Nano, only pins **3, 5, 6, 9, 10, 11** can do PWM
> (`analogWrite`). `RPWM=3` and `LPWM=9` are both valid PWM pins. `R_EN`/`L_EN` are
> plain digital outputs (held HIGH), so any pins work for them.

> **BTS7960 power:** It tolerates 5.5–27 V on B+/B− (motor supply) and handles high
> current — connect the motor battery to B+/B− and the motor to M+/M−. `VCC` is the
> logic supply (tie to Arduino **5V**), and the driver GND must be common with the
> Arduino GND and battery −.

---

## 3. Motor driver ⇄ motor + battery

- **Motor power**: connect your motor battery (5.5–27 V) to the BTS7960's **B+/B−**
  terminals. **Do NOT power the motor from the Arduino's 5V pin** — it draws too much
  current and will brown out the logic.
- **Logic power**: the driver's **VCC** (logic) goes to Arduino 5V.
- **Motor**: connect the motor to the **M+/M−** output terminals. If it spins the
  wrong way, swap the two motor leads (or flip `INVERT_MOTOR` in the sketch).
- **Common ground**: driver GND ↔ Arduino GND ↔ battery −.

---

## Wiring sanity checklist

- [ ] All grounds tied together (ESP32, Arduino, driver, battery).
- [ ] Arduino TX (pin 11) → ESP32 RX (GPIO16) goes through a divider / level shifter.
- [ ] Motor powered from the battery (B+/B−), **not** from the Arduino.
- [ ] `R_EN` (pin 5) and `L_EN` (pin 6) wired so the sketch can drive them HIGH.
- [ ] First test with the wheel **off the ground**.
