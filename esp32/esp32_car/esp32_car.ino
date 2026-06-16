/* ============================================================================
 *  esp32_car.ino  -  RC car wireless brain  (HTTP version, minimal page)
 *
 *  ESP32-CAM: WiFi hotspot + tiny control page + HTTP control, bridged to the
 *  Arduino over Serial2 (GPIO4, inverted to keep the flash LED dark).
 *
 *  Libraries: NONE beyond the ESP32 core.
 *  Board: AI-Thinker ESP32-CAM. Phone: join "RC-CAR", open http://192.168.4.1
 *  The page shows a version (v4, v5, ...) so you can confirm a fresh upload.
 * ========================================================================== */

#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>

// ---- Configuration ---------------------------------------------------------
const char *AP_SSID = "RC-CAR";
const char *AP_PASS = "";               // "" = open network; or >=8 chars for a password

const int   MAX_PWM       = 255;
const unsigned long CMD_TIMEOUT_MS = 400;

const int   ARDUINO_RX_PIN = -1;        // not used (telemetry off)
const int   ARDUINO_TX_PIN = 4;         // GPIO4 -> Arduino RX (pin 11); inverted
const bool  SERIAL_INVERT  = true;      // keep the GPIO4 flash LED dark
const long  ARDUINO_BAUD   = 38400;

// Activity LED: the bright white flash LED (GPIO4) is busy as the Serial2 TX line,
// so we blink the ESP32-CAM's small onboard RED LED (GPIO33, active LOW) instead.
const int   STATUS_LED_PIN = 33;
const bool  STATUS_LED_ACTIVE_LOW = true;
const unsigned long LED_PULSE_MS  = 60;   // how long each input lights the LED

// ---- Globals ---------------------------------------------------------------
WebServer server(80);
DNSServer dnsServer;
const byte DNS_PORT = 53;

float gSteer = 0, gThrottle = 0;
unsigned long gLastCmdMs = 0;
bool  gStopped = true;

unsigned long gLedOffMs = 0;            // when to switch the activity LED back off
bool  gLedOn = false;

// ---- Activity LED ----------------------------------------------------------
void ledWrite(bool on) {
  digitalWrite(STATUS_LED_PIN, (on == !STATUS_LED_ACTIVE_LOW) ? HIGH : LOW);
  gLedOn = on;
}
// Pulse the LED for LED_PULSE_MS; repeated calls (one per input) keep it flashing.
void ledPulse() { ledWrite(true); gLedOffMs = millis() + LED_PULSE_MS; }

// ---- Minimal control page (small so it never truncates) --------------------
const char INDEX_HTML[] PROGMEM = R"HTMLDOC(<!DOCTYPE html><html><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,maximum-scale=1,user-scalable=no">
<title>RC Car</title><style>
body{margin:0;font-family:sans-serif;background:#111;color:#eee;text-align:center;touch-action:none}
#s{padding:8px;font-size:15px}
#pad{width:80vw;height:80vw;max-width:320px;max-height:320px;margin:8px auto;border-radius:50%;background:#222;border:2px solid #444;position:relative;touch-action:none}
#k{position:absolute;width:34%;height:34%;left:33%;top:33%;border-radius:50%;background:#4cc2ff}
#stop{width:80vw;max-width:320px;padding:14px;font-size:18px;border:0;border-radius:10px;background:#e33;color:#fff}
</style></head><body>
<div id="s">v4 connecting</div>
<div id="pad"><div id="k"></div></div>
<button id="stop">STOP</button>
<script>
var S=document.getElementById("s"),P=document.getElementById("pad"),K=document.getElementById("k");
var sx=0,sy=0,drag=false,busy=false,n=0;
function st(t){S.textContent="v4 "+t;}
st("js-ran");
function go(p){if(busy)return;busy=true;var x=new XMLHttpRequest();x.onreadystatechange=function(){if(x.readyState==4){busy=false;st(x.status==200?"connected":"no signal");}};x.onerror=function(){busy=false;st("no signal");};x.open("GET",p,true);x.send();}
function stop(){var x=new XMLHttpRequest();x.open("GET","/s",true);x.send();}
setInterval(function(){if(sx||sy)go("/c?s="+sx.toFixed(2)+"&t="+sy.toFixed(2));else if(n++%12==0)go("/c?s=0&t=0");},80);
function set(nx,ny){var m=Math.sqrt(nx*nx+ny*ny);if(m>1){nx/=m;ny/=m;}if(Math.abs(nx)<.08)nx=0;if(Math.abs(ny)<.08)ny=0;sx=nx;sy=ny;K.style.transform="translate("+(nx*100)+"%,"+(-ny*100)+"%)";}
function loc(x,y){var r=P.getBoundingClientRect();set((x-r.left-r.width/2)/(r.width/2),-(y-r.top-r.height/2)/(r.height/2));}
function rel(){drag=false;sx=0;sy=0;K.style.transform="translate(0,0)";stop();}
P.addEventListener("touchstart",function(e){e.preventDefault();drag=true;loc(e.touches[0].clientX,e.touches[0].clientY);},{passive:false});
P.addEventListener("touchmove",function(e){e.preventDefault();if(drag)loc(e.touches[0].clientX,e.touches[0].clientY);},{passive:false});
P.addEventListener("touchend",function(e){e.preventDefault();rel();},{passive:false});
document.getElementById("stop").addEventListener("click",rel);
stop();
</script></body></html>)HTMLDOC";

// ---- Motor mixing ----------------------------------------------------------
void mixAndSend(float steer, float throttle) {
  float left  = throttle + steer;
  float right = throttle - steer;
  float m = max(max(fabs(left), fabs(right)), 1.0f);
  left  /= m;
  right /= m;
  int l = constrain((int)lround(left  * MAX_PWM), -MAX_PWM, MAX_PWM);
  int r = constrain((int)lround(right * MAX_PWM), -MAX_PWM, MAX_PWM);
  Serial2.printf("D %d %d\n", l, r);
}

void sendStop() {
  Serial2.print("S\n");
  gStopped = true;
}

// ---- HTTP handlers ---------------------------------------------------------
void handleRoot() {
  server.sendHeader("Cache-Control", "no-store");
  server.send_P(200, "text/html", INDEX_HTML);
}

void handleControl() {
  if (server.hasArg("s") && server.hasArg("t")) {
    gSteer    = constrain(server.arg("s").toFloat(), -1.0f, 1.0f);
    gThrottle = constrain(server.arg("t").toFloat(), -1.0f, 1.0f);
    gLastCmdMs = millis();
    gStopped = false;
    mixAndSend(gSteer, gThrottle);
    if (fabs(gSteer) > 0.0f || fabs(gThrottle) > 0.0f) ledPulse();  // flash on real input
  }
  server.send(200, "text/plain", "ok");
}

void handleStop() {
  gSteer = gThrottle = 0;
  sendStop();
  gLastCmdMs = millis();
  server.send(200, "text/plain", "stopped");
}

void handleAppleProbe() {
  server.send(200, "text/html",
              "<HTML><HEAD><TITLE>Success</TITLE></HEAD><BODY>Success</BODY></HTML>");
}

// ---- Setup / loop ----------------------------------------------------------
void setup() {
  Serial.begin(115200);
  Serial2.begin(ARDUINO_BAUD, SERIAL_8N1, ARDUINO_RX_PIN, ARDUINO_TX_PIN, SERIAL_INVERT);
  delay(200);

  pinMode(STATUS_LED_PIN, OUTPUT);
  ledWrite(false);                       // start with the activity LED off

  WiFi.mode(WIFI_AP);
  bool ok = WiFi.softAP(AP_SSID, (strlen(AP_PASS) >= 8) ? AP_PASS : nullptr);
  IPAddress ip = WiFi.softAPIP();
  Serial.printf("\n[wifi] AP \"%s\" %s\n", AP_SSID, ok ? "started" : "FAILED");
  Serial.printf("[wifi] open  http://%s\n", ip.toString().c_str());

  dnsServer.start(DNS_PORT, "*", ip);

  server.on("/", handleRoot);
  server.on("/c", handleControl);
  server.on("/s", handleStop);
  server.on("/hotspot-detect.html", handleAppleProbe);
  server.on("/generate_204", [](){ server.send(204); });
  server.on("/ncsi.txt", [](){ server.send(200, "text/plain", "Microsoft NCSI"); });
  server.onNotFound(handleRoot);

  server.begin();
  sendStop();
  gLastCmdMs = millis();
  Serial.println("[http] server started");
}

void loop() {
  dnsServer.processNextRequest();
  server.handleClient();

  if (!gStopped && (millis() - gLastCmdMs > CMD_TIMEOUT_MS)) {
    gSteer = gThrottle = 0;
    sendStop();
  }

  // Turn the activity LED back off after its pulse window.
  if (gLedOn && (long)(millis() - gLedOffMs) >= 0) ledWrite(false);
}
