/* ============================================================================
 *  servo_precision_test.ino  -  Precise Servo Range Testing via Web Interface
 *
 *  Purpose: Test servo range with microsecond precision through a web interface.
 *  This allows you to find the exact safe operating range for your car's
 *  steering geometry without modifying your main code.
 *
 *  Features:
 *  - Direct microsecond input (500-2500 µs)
 *  - Fine adjustment buttons (+/- 1, 5, 10, 50, 100 µs)
 *  - Preset positions and sweep testing
 *  - Real-time position display
 *  - Safety limits to prevent mechanical damage
 *
 *  Setup:
 *  1. Upload this to ESP32-CAM
 *  2. Upload companion sketch to Arduino
 *  3. Connect to "SERVO-TEST" WiFi
 *  4. Open http://192.168.4.1
 *
 *  Board: ESP32-CAM (AI-Thinker)
 * ========================================================================== */

#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>

// ---- Configuration ---------------------------------------------------------
const char *AP_SSID = "SERVO-TEST";
const char *AP_PASS = "";  // Open network for easy testing

// Serial to Arduino (same as main project)
const int   ARDUINO_TX_PIN = 14;        // GPIO14 -> Arduino RX (pin 11)
const bool  SERIAL_INVERT  = true;      // Match main project
const long  ARDUINO_BAUD   = 38400;

// Servo defaults and limits
int currentMicros = 1500;  // Current servo position
int minLimit = 500;         // Safety minimum
int maxLimit = 2500;        // Safety maximum
int savedPositions[5] = {1100, 1300, 1500, 1700, 1900};  // Saved test positions

// Sweep test parameters
bool sweepActive = false;
int sweepMin = 1100;
int sweepMax = 1900;
int sweepStep = 10;
int sweepDelay = 50;
unsigned long lastSweepTime = 0;
int sweepDirection = 1;
int sweepPosition = 1500;

// ---- Globals ---------------------------------------------------------------
WebServer server(80);
DNSServer dnsServer;
const byte DNS_PORT = 53;

// ---- Web Page --------------------------------------------------------------
const char INDEX_HTML[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1, user-scalable=no">
<title>Servo Precision Test</title>
<style>
  * { box-sizing: border-box; margin: 0; padding: 0; }
  body {
    font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif;
    background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
    color: white;
    padding: 20px;
    min-height: 100vh;
  }
  .container {
    max-width: 800px;
    margin: 0 auto;
    background: rgba(255, 255, 255, 0.1);
    backdrop-filter: blur(10px);
    border-radius: 20px;
    padding: 30px;
    box-shadow: 0 8px 32px rgba(0, 0, 0, 0.1);
  }
  h1 {
    text-align: center;
    margin-bottom: 30px;
    font-size: 28px;
  }
  .status {
    text-align: center;
    padding: 15px;
    background: rgba(255, 255, 255, 0.2);
    border-radius: 10px;
    margin-bottom: 25px;
    font-size: 24px;
    font-weight: bold;
    font-family: monospace;
  }
  .section {
    background: rgba(255, 255, 255, 0.1);
    border-radius: 15px;
    padding: 20px;
    margin-bottom: 20px;
  }
  .section h2 {
    font-size: 18px;
    margin-bottom: 15px;
    opacity: 0.9;
  }
  .input-group {
    display: flex;
    gap: 10px;
    margin-bottom: 15px;
    flex-wrap: wrap;
  }
  input[type="number"] {
    flex: 1;
    padding: 12px;
    border: 2px solid rgba(255, 255, 255, 0.3);
    border-radius: 8px;
    background: rgba(255, 255, 255, 0.1);
    color: white;
    font-size: 18px;
    font-family: monospace;
    min-width: 120px;
  }
  input[type="number"]:focus {
    outline: none;
    border-color: white;
    background: rgba(255, 255, 255, 0.2);
  }
  input[type="number"]::placeholder {
    color: rgba(255, 255, 255, 0.5);
  }
  .slider-container {
    margin: 20px 0;
  }
  input[type="range"] {
    width: 100%;
    height: 8px;
    border-radius: 4px;
    background: rgba(255, 255, 255, 0.3);
    outline: none;
    -webkit-appearance: none;
  }
  input[type="range"]::-webkit-slider-thumb {
    -webkit-appearance: none;
    width: 24px;
    height: 24px;
    border-radius: 50%;
    background: white;
    cursor: pointer;
    box-shadow: 0 2px 8px rgba(0, 0, 0, 0.3);
  }
  input[type="range"]::-moz-range-thumb {
    width: 24px;
    height: 24px;
    border-radius: 50%;
    background: white;
    cursor: pointer;
    box-shadow: 0 2px 8px rgba(0, 0, 0, 0.3);
  }
  button {
    padding: 12px 20px;
    border: none;
    border-radius: 8px;
    background: white;
    color: #667eea;
    font-weight: bold;
    cursor: pointer;
    transition: transform 0.1s, box-shadow 0.1s;
    font-size: 14px;
  }
  button:hover {
    transform: translateY(-2px);
    box-shadow: 0 4px 12px rgba(0, 0, 0, 0.2);
  }
  button:active {
    transform: translateY(0);
  }
  button.small {
    padding: 8px 12px;
    font-size: 12px;
  }
  button.center {
    background: #ff6b6b;
    color: white;
  }
  button.preset {
    background: rgba(255, 255, 255, 0.2);
    color: white;
    border: 2px solid white;
  }
  button.active {
    background: #4caf50;
    color: white;
  }
  .btn-grid {
    display: grid;
    grid-template-columns: repeat(auto-fit, minmax(80px, 1fr));
    gap: 8px;
  }
  .info {
    margin-top: 20px;
    padding: 15px;
    background: rgba(255, 255, 255, 0.1);
    border-radius: 10px;
    border-left: 4px solid #ffd93d;
  }
  .info h3 {
    margin-bottom: 10px;
    color: #ffd93d;
  }
  .info ul {
    list-style: none;
    padding-left: 0;
  }
  .info li {
    margin: 5px 0;
    opacity: 0.9;
  }
  .range-display {
    display: flex;
    justify-content: space-between;
    margin-top: 5px;
    font-size: 12px;
    opacity: 0.7;
  }
  #exportArea {
    width: 100%;
    padding: 15px;
    background: rgba(0, 0, 0, 0.3);
    color: #00ff00;
    font-family: monospace;
    font-size: 12px;
    border: none;
    border-radius: 8px;
    resize: vertical;
    min-height: 100px;
  }
</style>
</head>
<body>
  <div class="container">
    <h1>🎯 Servo Precision Test</h1>

    <div class="status">
      Current Position: <span id="currentPos">1500</span> µs
    </div>

    <div class="section">
      <h2>Direct Control</h2>
      <div class="input-group">
        <input type="number" id="directInput" min="500" max="2500" value="1500" placeholder="Enter µs">
        <button onclick="setDirect()">SET</button>
        <button class="center" onclick="setCenter()">CENTER (1500)</button>
      </div>

      <div class="slider-container">
        <input type="range" id="slider" min="500" max="2500" value="1500" oninput="setFromSlider()">
        <div class="range-display">
          <span>500 µs</span>
          <span>1500 µs</span>
          <span>2500 µs</span>
        </div>
      </div>
    </div>

    <div class="section">
      <h2>Fine Adjustment</h2>
      <div class="btn-grid">
        <button class="small" onclick="adjust(-100)">-100</button>
        <button class="small" onclick="adjust(-50)">-50</button>
        <button class="small" onclick="adjust(-10)">-10</button>
        <button class="small" onclick="adjust(-5)">-5</button>
        <button class="small" onclick="adjust(-1)">-1</button>
        <button class="small" onclick="adjust(1)">+1</button>
        <button class="small" onclick="adjust(5)">+5</button>
        <button class="small" onclick="adjust(10)">+10</button>
        <button class="small" onclick="adjust(50)">+50</button>
        <button class="small" onclick="adjust(100)">+100</button>
      </div>
    </div>

    <div class="section">
      <h2>Preset Positions</h2>
      <div class="btn-grid">
        <button class="preset" onclick="setPosition(500)">MIN (500)</button>
        <button class="preset" onclick="setPosition(1000)">1000 µs</button>
        <button class="preset" onclick="setPosition(1250)">1250 µs</button>
        <button class="preset" onclick="setPosition(1500)">1500 µs</button>
        <button class="preset" onclick="setPosition(1750)">1750 µs</button>
        <button class="preset" onclick="setPosition(2000)">2000 µs</button>
        <button class="preset" onclick="setPosition(2500)">MAX (2500)</button>
      </div>
    </div>

    <div class="section">
      <h2>Sweep Test</h2>
      <div class="input-group">
        <input type="number" id="sweepMin" placeholder="Min µs" value="1100">
        <input type="number" id="sweepMax" placeholder="Max µs" value="1900">
        <input type="number" id="sweepStep" placeholder="Step" value="10">
        <input type="number" id="sweepDelay" placeholder="Delay ms" value="50">
      </div>
      <button id="sweepBtn" onclick="toggleSweep()">START SWEEP</button>
    </div>

    <div class="section">
      <h2>Safety Limits</h2>
      <div class="input-group">
        <input type="number" id="minLimit" placeholder="Min limit" value="500">
        <input type="number" id="maxLimit" placeholder="Max limit" value="2500">
        <button onclick="setLimits()">SET LIMITS</button>
      </div>
    </div>

    <div class="section">
      <h2>Export Configuration</h2>
      <button onclick="exportConfig()">Generate Code</button>
      <textarea id="exportArea" style="display:none;" readonly></textarea>
    </div>

    <div class="info">
      <h3>⚠️ Safety Tips</h3>
      <ul>
        <li>• Start from center (1500 µs) and expand gradually</li>
        <li>• Listen for mechanical binding or unusual sounds</li>
        <li>• Keep wheels off ground for initial testing</li>
        <li>• Note the safe range before and after hitting mechanical stops</li>
        <li>• The DS3218 servo typically operates from 500-2500 µs</li>
      </ul>
    </div>
  </div>

<script>
let currentPos = 1500;
let sweepInterval = null;
let minLimit = 500;
let maxLimit = 2500;

function updateDisplay(pos) {
  currentPos = pos;
  document.getElementById('currentPos').textContent = pos;
  document.getElementById('slider').value = pos;
  document.getElementById('directInput').value = pos;
}

function sendCommand(micros) {
  micros = Math.max(minLimit, Math.min(maxLimit, micros));
  fetch(`/servo?us=${micros}`)
    .then(r => r.text())
    .then(() => updateDisplay(micros))
    .catch(err => console.error('Error:', err));
}

function setDirect() {
  const val = parseInt(document.getElementById('directInput').value);
  if (!isNaN(val)) sendCommand(val);
}

function setFromSlider() {
  const val = parseInt(document.getElementById('slider').value);
  sendCommand(val);
}

function setPosition(micros) {
  sendCommand(micros);
}

function setCenter() {
  sendCommand(1500);
}

function adjust(amount) {
  sendCommand(currentPos + amount);
}

function toggleSweep() {
  const btn = document.getElementById('sweepBtn');
  if (sweepInterval) {
    clearInterval(sweepInterval);
    sweepInterval = null;
    btn.textContent = 'START SWEEP';
    btn.classList.remove('active');
    fetch('/stopsweep');
  } else {
    const min = parseInt(document.getElementById('sweepMin').value) || 1100;
    const max = parseInt(document.getElementById('sweepMax').value) || 1900;
    const step = parseInt(document.getElementById('sweepStep').value) || 10;
    const delay = parseInt(document.getElementById('sweepDelay').value) || 50;

    fetch(`/startsweep?min=${min}&max=${max}&step=${step}&delay=${delay}`)
      .then(() => {
        btn.textContent = 'STOP SWEEP';
        btn.classList.add('active');

        // Update position display during sweep
        sweepInterval = setInterval(() => {
          fetch('/position')
            .then(r => r.text())
            .then(pos => updateDisplay(parseInt(pos)));
        }, 100);
      });
  }
}

function setLimits() {
  minLimit = parseInt(document.getElementById('minLimit').value) || 500;
  maxLimit = parseInt(document.getElementById('maxLimit').value) || 2500;
  document.getElementById('slider').min = minLimit;
  document.getElementById('slider').max = maxLimit;
  alert(`Limits set: ${minLimit} - ${maxLimit} µs`);
}

function exportConfig() {
  const area = document.getElementById('exportArea');
  const config = `// Servo configuration discovered through testing
// Tested on: ${new Date().toLocaleDateString()}

// For ESP32 (esp32_car.ino):
const int SERVO_CENTER_US = 1500;  // Center position
const int SERVO_RANGE_US = ${Math.min(Math.abs(currentPos - 1500), 400)};  // Safe range from center

// For Arduino (arduino_motors.ino):
const int SERVO_MIN_US = ${minLimit};     // Minimum safe position
const int SERVO_MAX_US = ${maxLimit};     // Maximum safe position
const int SERVO_CENTER_US = 1500;  // Center position

// Notes:
// - Current test position: ${currentPos} µs
// - Safe operating range: ${minLimit} - ${maxLimit} µs
// - Mechanical limits may be narrower than electrical limits
`;
  area.style.display = 'block';
  area.value = config;
  area.select();
}

// Initialize on load
window.onload = () => {
  fetch('/position')
    .then(r => r.text())
    .then(pos => updateDisplay(parseInt(pos)));
};

// Auto-refresh position every 500ms when not sweeping
setInterval(() => {
  if (!sweepInterval) {
    fetch('/position')
      .then(r => r.text())
      .then(pos => updateDisplay(parseInt(pos)))
      .catch(() => {});
  }
}, 500);
</script>
</body>
</html>
)=====";

// ---- Helper Functions ------------------------------------------------------
void sendServoCommand(int micros) {
  currentMicros = constrain(micros, minLimit, maxLimit);
  Serial2.printf("V %d\n", currentMicros);
  Serial.printf("[Servo] Set to %d µs\n", currentMicros);
}

// ---- HTTP Handlers ---------------------------------------------------------
void handleRoot() {
  server.send_P(200, "text/html", INDEX_HTML);
}

void handleServo() {
  if (server.hasArg("us")) {
    int micros = server.arg("us").toInt();
    sendServoCommand(micros);
    sweepActive = false;  // Stop sweep if manual control
    server.send(200, "text/plain", String(currentMicros));
  } else {
    server.send(400, "text/plain", "Missing parameter");
  }
}

void handlePosition() {
  server.send(200, "text/plain", String(currentMicros));
}

void handleStartSweep() {
  sweepMin = server.hasArg("min") ? server.arg("min").toInt() : 1100;
  sweepMax = server.hasArg("max") ? server.arg("max").toInt() : 1900;
  sweepStep = server.hasArg("step") ? server.arg("step").toInt() : 10;
  sweepDelay = server.hasArg("delay") ? server.arg("delay").toInt() : 50;

  sweepActive = true;
  sweepPosition = currentMicros;
  sweepDirection = (sweepPosition < sweepMax) ? 1 : -1;
  lastSweepTime = millis();

  Serial.printf("[Sweep] Started: %d-%d µs, step %d, delay %d ms\n",
                sweepMin, sweepMax, sweepStep, sweepDelay);
  server.send(200, "text/plain", "Sweep started");
}

void handleStopSweep() {
  sweepActive = false;
  Serial.println("[Sweep] Stopped");
  server.send(200, "text/plain", "Sweep stopped");
}

void handleNotFound() {
  server.send(404, "text/plain", "Not Found");
}

// ---- Setup -----------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  Serial.println("\n========================================");
  Serial.println("Servo Precision Test - ESP32");
  Serial.println("========================================");

  // Initialize serial to Arduino
  Serial2.begin(ARDUINO_BAUD, SERIAL_8N1, -1, ARDUINO_TX_PIN, SERIAL_INVERT);
  delay(100);

  // Send initial center command
  sendServoCommand(1500);

  // Setup WiFi AP
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASS);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("Access Point: ");
  Serial.println(AP_SSID);
  Serial.print("IP Address: ");
  Serial.println(IP);

  // Setup DNS server for captive portal
  dnsServer.start(DNS_PORT, "*", IP);

  // Setup web server routes
  server.on("/", handleRoot);
  server.on("/servo", handleServo);
  server.on("/position", handlePosition);
  server.on("/startsweep", handleStartSweep);
  server.on("/stopsweep", handleStopSweep);
  server.onNotFound(handleRoot);  // Captive portal - all unknown routes go to main page

  server.begin();
  Serial.println("Web server started");
  Serial.println("\nConnect to WiFi: SERVO-TEST");
  Serial.println("Open browser to: http://192.168.4.1");
  Serial.println("========================================\n");
}

// ---- Main Loop -------------------------------------------------------------
void loop() {
  dnsServer.processNextRequest();
  server.handleClient();

  // Handle sweep mode
  if (sweepActive && (millis() - lastSweepTime >= sweepDelay)) {
    sweepPosition += sweepStep * sweepDirection;

    // Check bounds and reverse direction
    if (sweepPosition >= sweepMax) {
      sweepPosition = sweepMax;
      sweepDirection = -1;
    } else if (sweepPosition <= sweepMin) {
      sweepPosition = sweepMin;
      sweepDirection = 1;
    }

    sendServoCommand(sweepPosition);
    lastSweepTime = millis();
  }
}