# Communication protocol

Two links, two simple text protocols.

## Link A — Phone (browser) ⇄ ESP32  (plain HTTP GET, ~12×/second)

The web UI polls the ESP32 with simple HTTP requests (no WebSocket — this avoids the
iOS captive-portal WebSocket block and needs no extra libraries).

| Request | Meaning |
|---------|---------|
| `GET /` | the control web page |
| `GET /c?s=<steer>&t=<throttle>` | a control update; returns `ok` |
| `GET /s` | STOP; returns `stopped` (also sent when the joystick is released) |

- `s` (steer)   — float in `[-1.0, 1.0]`  (−1 = full left, +1 = full right)
- `t` (throttle)— float in `[-1.0, 1.0]`  (+1 = full forward, −1 = full reverse)

Examples:
```
GET /c?s=0&t=0        → stopped, centered
GET /c?s=0&t=1        → full speed straight forward
GET /c?s=-1&t=0.5     → forward at half speed, turning hard left
GET /s                → explicit STOP
```

The ESP32 also treats "no update received for 400 ms" as an implicit STOP (failsafe).

### Mixing (done on the ESP32)
Differential / tank mixing converts steer+throttle into left/right wheel speeds:

```
left  = throttle + steer
right = throttle - steer
# normalize so neither exceeds 1.0
m = max(|left|, |right|, 1.0)
left /= m ;  right /= m
# scale to PWM
leftPWM  = round(left  * MAX_PWM)   # -255..255
rightPWM = round(right * MAX_PWM)
```

## Link B — ESP32 ⇄ Arduino  (UART serial, 38400 baud)

> 38400 is used instead of 115200 because the Arduino reads this link with
> `SoftwareSerial`, which is unreliable at higher rates on a 16 MHz Uno.

The ESP32 sends one command per line, newline-terminated:

```
D <left> <right>\n
```

- `left`, `right` — integers in `[-255, 255]` (sign = direction, magnitude = PWM duty).

```
D 255 255\n     → both motors full forward
D -255 -255\n   → both full reverse
D 200 -200\n    → spin in place (left fwd, right rev) → turns right
D 0 0\n         → stop
S\n             → stop (alias)
```

The Arduino stops the motors if **no valid command arrives for 400 ms** (failsafe).

### Optional telemetry (Arduino → ESP32)
The Arduino may send back status lines (ignored by default UI):
```
T <leftApplied> <rightApplied>\n
```
This requires the Arduino-TX→ESP32-RX wire (with level shifting) from `WIRING.md`.
