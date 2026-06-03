# car2026 — Phone-controlled RC Car (ESP32 + Arduino)

Drive an RC car from your phone's web browser. No app, no internet required.

## How it works

```
 iPhone / any phone browser
        │  (joins the car's own WiFi hotspot)
        ▼
   ┌─────────────────────────────────────────┐
   │ ESP32-CAM                                 │
   │  • Creates a WiFi Access Point            │
   │  • Hosts the control website (HTTP)       │
   │  • Receives joystick input via HTTP GETs  │
   │  • Mixes throttle+steering → L/R speeds   │
   └───────────────┬───────────────────────────┘
                   │ UART serial  (e.g. "D 200 -200\n")
                   ▼
   ┌─────────────────────────────────────────┐
   │ Arduino (Uno/Nano)                        │
   │  • Reads serial commands                  │
   │  • Drives the motor driver (PWM + dir)    │
   │  • Failsafe stop if signal lost           │
   └───────────────┬───────────────────────────┘
                   ▼
        Motor driver (TB6612 / L293D / DRV8833)
                   ▼
            2 DC motors (left + right, tank steering)
```

The phone connects to the ESP32's hotspot, so this works **anywhere** (no router needed)
and on **any phone including iPhone**, because the UI is just a web page.

## Repository layout

| Path | What it is |
|------|------------|
| `esp32/esp32_car/esp32_car.ino` | ESP32 firmware: WiFi AP, web server (HTTP), serial bridge. No external libraries. |
| `arduino/arduino_motors/arduino_motors.ino` | Arduino firmware: serial → motor driver |
| `web/index.html` | The control website (also embedded in the ESP32 sketch). Open in a desktop browser to preview/edit. |
| `docs/WIRING.md` | Pin-by-pin wiring, **important 5V↔3.3V level-shifting note** |
| `docs/PROTOCOL.md` | The phone↔ESP32 and ESP32↔Arduino message formats |

## Quick start

1. **Wire it up** — follow [`docs/WIRING.md`](docs/WIRING.md). Pay attention to the
   level-shifting warning on the Arduino→ESP32 line (5V can damage the ESP32).
2. **Flash the Arduino** — open `arduino/arduino_motors/arduino_motors.ino`,
   set `MOTOR_DRIVER` to the chip you have, upload.
3. **Flash the ESP32** — open `esp32/esp32_car/esp32_car.ino`, select your ESP32 board,
   upload. No external libraries needed (uses the built-in `WiFi.h` + `WebServer.h`).
4. **Drive** — on your phone, join the WiFi network **`RC-CAR`** (password `drive1234`),
   then open a browser to **`http://192.168.4.1`**. Use the on-screen joystick.

## Safety / failsafe

- If the phone stops sending input (closed tab, walked out of range), the ESP32
  sends a stop within ~300 ms.
- If the Arduino stops hearing from the ESP32, it stops the motors within ~400 ms.
- There is a big **STOP** button on the web UI.
- **Prop the wheels off the ground the first time you test.**

## Tuning

- WiFi name/password: top of `esp32_car.ino` (`AP_SSID`, `AP_PASS`).
- Max speed / deadzone: `esp32_car.ino` (`MAX_PWM`, deadzone in the web UI).
- Motor direction: if a wheel spins backwards, swap that motor's two wires, or flip
  the sign in `arduino_motors.ino` (`INVERT_LEFT` / `INVERT_RIGHT`).
- Motor driver type: `MOTOR_DRIVER` define in `arduino_motors.ino`.
