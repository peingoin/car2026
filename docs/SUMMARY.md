# Repository Summary

**car2026** is a browser-controlled RC car built by Austin Cheng & Charlie Gao. A phone connects to the car's own WiFi hotspot and drives it from a web page — no app, no internet, no router required.

---

## Architecture overview

```
Phone browser
    │  HTTP GET ~12×/sec  (join "RC-CAR" WiFi)
    ▼
ESP32-CAM  (/esp32/esp32_car/esp32_car.ino)
    • Creates a WiFi Access Point ("RC-CAR", open network)
    • Serves the control page (embedded HTML)
    • Handles /c?s=&t=, /s, and /cam HTTP routes
    • Serves live JPEG snapshots via GET /cam (~6-7 fps)
    • Throttle (Y) → equal PWM to both motors
    • Steer (X) → servo pulse width (µs) via "V <us>\n"
    │  UART serial @ 38400 baud
    ▼
Arduino Uno/Nano  (/arduino/arduino_motors/arduino_motors.ino)
    • Parses "D <left> <right>", "V <us>", and "S" commands
    • Drives motor driver (TB6612 / L293D / DRV8833)
    • Drives Miuzei DS3218 270° servo on pin 6 via Servo library
    • Failsafe: stops motors + centers servo if no command for 400 ms
    ▼
Motor driver → 2 DC motors (equal throttle, rear-wheel drive)
DS3218 servo → front wheels (Ackermann/RC-car steering)
```

---

## Directory layout

| Path | Description |
|------|-------------|
| `esp32/esp32_car/esp32_car.ino` | ESP32-CAM firmware: WiFi AP, HTTP web server, serial bridge. No external libraries (uses built-in `WiFi.h`, `WebServer.h`, `DNSServer.h`). |
| `arduino/arduino_motors/arduino_motors.ino` | Arduino firmware: serial command parser, motor driver abstraction, 400 ms failsafe. Supports TB6612, L293D/L298N, and DRV8833. |
| `arduino/motor_test/motor_test.ino` | Standalone wiring sanity sketch. Cycles each motor forward/reverse without needing the ESP32 or phone. |
| `web/index.html` | Full-featured standalone control page (also the version embedded in the ESP32 sketch). Features a circular touch joystick, throttle/steer meters, a STOP button, and status indicator. Works in desktop browsers for development preview. |
| `docs/PROTOCOL.md` | Exact message formats for both links (phone↔ESP32 and ESP32↔Arduino). |
| `docs/WIRING.md` | Pin-by-pin wiring table, level-shifting note (Arduino 5V TX → ESP32 3.3V RX), motor driver hookup, and a sanity checklist. |
| `animation/` | 240 raw PNG frames (ezgif-frame-001…240) with a gray checkerboard background, exported from a 360° turntable GIF. |
| `scripts/process_frames.py` | Python preprocessing script. Uses `rembg` (u2netp AI model) to remove the background from each PNG and saves transparent WebP files into `landing/public/frames/`. Supports resume (skips already-encoded valid frames). |
| `landing/` | React/Vite/Tailwind scrollytelling landing page (see below). |

---

## Firmware — ESP32 (`esp32_car.ino`)

- **WiFi**: open access point `RC-CAR` at `192.168.4.1`. Password can be set via `AP_PASS`; must be ≥8 chars or it is ignored (open).
- **DNS**: wildcard DNS server redirects any domain to the AP IP (captive-portal style so iOS/Android auto-opens the page).
- **HTTP routes**: `/` (control page), `/c?s=&t=` (control update, returns `ok`), `/s` (stop, returns `stopped`), `/cam` (JPEG snapshot, returns `image/jpeg`), `/hotspot-detect.html` + `/generate_204` + `/ncsi.txt` (Apple/Android captive-portal probes).
- **Camera**: `initCamera()` configures the OV2640 sensor at QVGA (320×240) JPEG, quality 12. If PSRAM is present, uses 2 frame buffers with `CAMERA_GRAB_LATEST` to always return the freshest frame. `handleCam()` calls `esp_camera_fb_get()`, sends the buffer as `image/jpeg`, then returns it immediately — no blocking of the control loop.
- **Steering**: joystick X-axis maps directly to servo pulse width: `us = 1500 + steer × 400` (range 1100–1900µs, centre 1500µs). Both DC motors receive equal throttle from the Y-axis.
- **Serial**: sends `D <t> <t>\n` (equal throttle) and `V <us>\n` (servo µs) to the Arduino over GPIO4 at 38400 baud with **inverted logic** (idles LOW) so the ESP32-CAM's onboard flash LED stays dark. `SERVO_RANGE_US` (default 400) is a tunable constant.
- **Failsafe**: if no `/c` or `/s` request arrives for 400 ms, sends `S\n` and stops.

## Firmware — Arduino (`arduino_motors.ino`)

- **Serial**: SoftwareSerial on pin 11 (RX from ESP32) at 38400 baud, **inverted** (matches ESP32).
- **Driver support**: compile-time `#define MOTOR_DRIVER` selects TB6612, L293D/L298N, or DRV8833. DRV8833 mode PWMs the input pins directly; the other modes use separate direction and PWM-enable pins.
- **Pin map (default)**: right motor: PWM=10, IN1=7, IN2=9; left motor: PWM=5, IN1=3, IN2=4; servo signal: pin 6.
- **Servo**: Miuzei DS3218 270° servo driven via Arduino `Servo` library. `attach(pin, 500, 2500)` sets the full DS3218 pulse range; `writeMicroseconds()` is used for precise positioning. `SERVO_REVERSED` flag inverts direction without rewiring.
- **Inversion**: `INVERT_LEFT` / `INVERT_RIGHT` booleans flip individual wheel direction without rewiring.
- **Failsafe**: stops motors and centers servo if no valid command for 400 ms. Resets the timer on each valid `D`, `V`, or `S` command.
- **Telemetry**: optional `T <l> <r>\n` echo back to the ESP32 (disabled by default; requires a level-shifted wire).

## Control page (`web/index.html`)

- Dark-themed UI with a **two-column layout**: live camera feed on the left (`#cam-wrap`, 4:3 aspect ratio) and circular joystick on the right (`#pad`/`#knob`), plus a `STOP` button and throttle/steer percentage readouts.
- **Camera feed**: a hidden `<img id="cam">` is refreshed every 150 ms by preloading the next frame into a temporary `Image` object and swapping `src` on `onload` to avoid flicker. A `camBusy` guard prevents overlapping requests.
- Polls `/c?s=&t=` every 80 ms while the knob is active; sends a heartbeat every ~1 s (every 12 idle ticks) to keep the status indicator live.
- Stop requests (`/s`) bypass the in-flight guard so they always reach the ESP32 immediately.
- Handles touch (all iOS versions) and mouse (desktop preview). Sends stop on tab blur and `visibilitychange`.
- Deadzone: ±8% (0.08) on both axes.
- The embedded `INDEX_HTML` in `esp32_car.ino` (v5) mirrors this layout in a compact minified form served directly to the phone.

---

## Landing page (`landing/`)

A React 19 + Vite + Tailwind CSS 4 scrollytelling marketing page for the project.

**Tech stack**: React 19, Motion (Framer Motion v12), Tailwind CSS v4, Vite v8, TypeScript, Lucide icons, JetBrains Mono / Space Grotesk / Inter variable fonts.

**Key components**:

| Component | Role |
|-----------|------|
| `ScrollSequence` | Pins a full-viewport `<canvas>` for the scroll duration (`h-[500vh]`). Maps scroll progress (spring-smoothed) to a frame index and paints the current WebP frame via `requestAnimationFrame`. Also applies a horizontal pan (`xShift`) that shifts the subject left/right at defined scroll waypoints. |
| `Hero` | Intro headline ("Drive from any phone.") that fades out early in the scroll and reappears at the end. Includes a "Scroll to explore" prompt that only shows at the start. |
| `Chapters` | Three narrative overlays ("The problem", "The idea", "The build") that cross-fade in and out at specific scroll ranges via `ParallaxText`. Chapter 3 includes a spec grid (any phone / no internet / ~300 ms failsafe). |
| `Footer` | CTA section ("Build it yourself.") with a link to the GitHub repo. Credits Austin Cheng & Charlie Gao. |
| `Loader` | Full-screen loading overlay shown until all 240 frames are preloaded. |
| `ProgressBar` | Thin scroll-progress indicator at the top. |
| `useImageSequence` | Custom hook that preloads all WebP frames as `<img>` elements and tracks overall load progress. |

**Frame pipeline**:
1. Raw 240-frame GIF is extracted to `animation/ezgif-frame-###.png` (gray background).
2. `scripts/process_frames.py` removes the background with AI matting (`rembg` / u2netp) and saves transparent WebP files to `landing/public/frames/frame-###.webp`.
3. The React app preloads all 240 frames at startup and renders them frame-by-frame on the canvas as the user scrolls.

---

## Communication protocols (summary)

**Link A — Phone ↔ ESP32 (HTTP GET, ~12 Hz)**

| Request | Effect |
|---------|--------|
| `GET /c?s=<−1..1>&t=<−1..1>` | Drive command (steer, throttle) |
| `GET /s` | Explicit stop |
| `GET /cam` | JPEG snapshot (~6-7 fps via JS polling at 150 ms) |

**Link B — ESP32 ↔ Arduino (UART, 38400 baud, inverted)**

| Message | Effect |
|---------|--------|
| `D <left> <right>\n` | Drive: both values equal (throttle only), −255..255 |
| `V <us>\n` | Steer: servo pulse width in µs (500–2500, 1500 = centre) |
| `S\n` | Stop motors (`V 1500` sent separately to centre servo) |
| `T <l> <r>\n` | Telemetry reply (Arduino→ESP32, optional) |

---

## Wiring highlights

- ESP32-CAM GPIO4 (TX, inverted) → Arduino pin 11 (RX). 3.3V drives 5V input fine — no level shifting needed in this direction.
- Arduino TX (5V) → ESP32 RX requires a voltage divider (1kΩ + 2kΩ) or logic-level converter. Only needed if telemetry is enabled.
- Arduino pin 6 → DS3218 servo signal wire. The DS3218 accepts standard 50 Hz PWM from a 5V source.
- DS3218 servo power (red) must come from a **dedicated 6–8.4V supply** — the 20 kg stall current exceeds what the Arduino 5V rail can provide.
- Motors must be powered from the motor battery, not the Arduino 5V pin.
- All grounds (ESP32, Arduino, motor driver, servo supply, battery −) must be common.

---

## Safety

- ESP32 stops within ~400 ms of losing the phone's HTTP poll.
- Arduino stops within 400 ms of losing the ESP32's serial stream.
- STOP button on the web UI sends an immediate stop request.
- First-run guidance: prop wheels off the ground before initial test.
