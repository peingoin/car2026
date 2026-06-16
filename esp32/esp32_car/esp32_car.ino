/* ============================================================================
 *  esp32_car.ino  -  RC car wireless brain  (HTTP version, minimal page)
 *
 *  ESP32-CAM: WiFi hotspot + tiny control page + HTTP control, bridged to the
 *  Arduino over Serial2 (GPIO14, inverted).
 *
 *  Libraries: NONE beyond the ESP32 core.
 *  Board: AI-Thinker ESP32-CAM. Phone: join "RC-CAR", open http://192.168.4.1
 *  The page shows a version (v4, v5, ...) so you can confirm a fresh upload.
 * ========================================================================== */

#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include "esp_camera.h"
 
// ---- AI-Thinker ESP32-CAM pin map -----------------------------------------
#define PWDN_GPIO_NUM  32
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM   0
#define SIOD_GPIO_NUM  26
#define SIOC_GPIO_NUM  27
#define Y9_GPIO_NUM    35
#define Y8_GPIO_NUM    34
#define Y7_GPIO_NUM    39
#define Y6_GPIO_NUM    36
#define Y5_GPIO_NUM    21
#define Y4_GPIO_NUM    19
#define Y3_GPIO_NUM    18
#define Y2_GPIO_NUM     5
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM  23
#define PCLK_GPIO_NUM  22

// ---- Configuration ---------------------------------------------------------
const char *AP_SSID = "RC-CAR";
const char *AP_PASS = "";               // "" = open network; or >=8 chars for a password

const int   MAX_PWM       = 255;
const unsigned long CMD_TIMEOUT_MS = 400;

const int SERVO_CENTER_US = 1500;
const int SERVO_RANGE_US  = 400;   // ±400µs ≈ ±54° of DS3218 travel; tune to taste

const int   ARDUINO_RX_PIN = -1;        // not used (telemetry off)
const int   ARDUINO_TX_PIN = 14;        // GPIO14 -> Arduino RX (pin 11); inverted
const bool  SERIAL_INVERT  = true;      // keep the GPIO4 flash LED dark
const long  ARDUINO_BAUD   = 38400;

// Activity LED: blink the ESP32-CAM's small onboard RED LED (GPIO33, active LOW).
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
const char INDEX_HTML[] PROGMEM = R"HTMLDOC(<!DOCTYPE html><html><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1,maximum-scale=1,user-scalable=no,viewport-fit=cover"><title>RC Car</title><style>
*{box-sizing:border-box;-webkit-user-select:none;user-select:none;-webkit-tap-highlight-color:transparent}html,body{margin:0;height:100%;overflow:hidden}
body{font-family:sans-serif;background:#000;color:#eee;touch-action:none}
#cam{position:fixed;inset:0;width:100%;height:100%;object-fit:cover;background:#000}
#s{position:fixed;top:6px;left:50%;transform:translateX(-50%);font-size:13px;padding:3px 10px;border-radius:999px;background:rgba(0,0,0,.45);z-index:3}
#stop{position:fixed;top:6px;right:8px;z-index:3;padding:8px 18px;font-size:15px;font-weight:700;border:0;border-radius:10px;background:#e33;color:#fff}
.pad{position:fixed;bottom:max(18px,env(safe-area-inset-bottom));width:min(34vmin,150px);height:min(34vmin,150px);border-radius:18px;background:rgba(20,24,34,.4);border:2px solid rgba(255,255,255,.25);touch-action:none;z-index:2}
#padL{left:18px}#padR{right:18px}
.k{position:absolute;width:42%;height:42%;left:29%;top:29%;border-radius:50%;background:rgba(76,194,255,.9);box-shadow:0 4px 12px rgba(0,0,0,.5)}
.lbl{position:absolute;bottom:4px;left:0;right:0;text-align:center;font-size:11px;color:rgba(255,255,255,.75)}
</style></head><body>
<img id="cam" alt="">
<div id="s">v6 connecting</div>
<button id="stop">STOP</button>
<div class="pad" id="padL"><div class="k" id="kL"></div><div class="lbl">STEER</div></div>
<div class="pad" id="padR"><div class="k" id="kR"></div><div class="lbl">SPEED</div></div>
<script>
var S=document.getElementById("s"),C=document.getElementById("cam");
var PL=document.getElementById("padL"),KL=document.getElementById("kL");
var PR=document.getElementById("padR"),KR=document.getElementById("kR");
var sx=0,sy=0,busy=false,n=0,cb=false;
function st(t){S.textContent="v6 "+t;}
st("js-ran");
function go(p){if(busy)return;busy=true;var x=new XMLHttpRequest();x.onreadystatechange=function(){if(x.readyState==4){busy=false;st(x.status==200?"connected":"no signal");}};x.onerror=function(){busy=false;st("no signal");};x.open("GET",p,true);x.send();}
function snd(){var x=new XMLHttpRequest();x.open("GET","/s",true);x.send();}
setInterval(function(){if(sx||sy)go("/c?s="+sx.toFixed(2)+"&t="+sy.toFixed(2));else if(n++%12==0)go("/c?s=0&t=0");},80);
setInterval(function(){if(cb)return;cb=true;var t=new Image();t.onload=function(){C.src=t.src;cb=false;};t.onerror=function(){cb=false;};t.src="/cam?t="+Date.now();},150);
function dz(v){return Math.abs(v)<.08?0:Math.max(-1,Math.min(1,v));}
function setL(nx){sx=dz(nx);KL.style.transform="translate("+(sx*60)+"%,0)";}
function setR(ny){sy=dz(ny);KR.style.transform="translate(0,"+(-sy*60)+"%)";}
function relL(){sx=0;KL.style.transform="translate(0,0)";}
function relR(){sy=0;KR.style.transform="translate(0,0)";}
// Each pad tracks its own finger by touch identifier so two thumbs don't collide.
function bind(pad,axis,move,rel){
  var id=null;
  function pos(t){var r=pad.getBoundingClientRect();return axis=="x"?(t.clientX-r.left-r.width/2)/(r.width/2):-(t.clientY-r.top-r.height/2)/(r.height/2);}
  function find(list){for(var i=0;i<list.length;i++)if(list[i].identifier===id)return list[i];return null;}
  pad.addEventListener("touchstart",function(e){e.preventDefault();if(id===null){var t=e.changedTouches[0];id=t.identifier;move(pos(t));}},{passive:false});
  pad.addEventListener("touchmove",function(e){e.preventDefault();var t=find(e.changedTouches);if(t)move(pos(t));},{passive:false});
  function end(e){e.preventDefault();if(find(e.changedTouches)){id=null;rel();}}
  pad.addEventListener("touchend",end,{passive:false});
  pad.addEventListener("touchcancel",end,{passive:false});
  var md=false;
  pad.addEventListener("mousedown",function(e){md=true;move(pos(e));});
  window.addEventListener("mousemove",function(e){if(md)move(pos(e));});
  window.addEventListener("mouseup",function(){if(md){md=false;rel();}});
}
bind(PL,"x",setL,relL);
bind(PR,"y",setR,relR);
document.getElementById("stop").addEventListener("click",function(){relL();relR();snd();});
snd();
</script></body></html>)HTMLDOC";

// ---- Servo steering + motor drive ------------------------------------------
void mixAndSend(float steer, float throttle) {
  int t  = constrain((int)lround(throttle * MAX_PWM), -MAX_PWM, MAX_PWM);
  Serial2.printf("D %d %d\n", t, t);   // both motors at equal throttle
  int us = constrain(SERVO_CENTER_US + (int)lround(steer * SERVO_RANGE_US), 500, 2500);
  Serial2.printf("V %d\n", us);
}

void sendStop() {
  Serial2.print("S\n");
  Serial2.printf("V %d\n", SERVO_CENTER_US);
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
    ledPulse();  // flash on any input received
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

void handleCam() {
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("[cam] Failed to get frame buffer");
    server.send(503, "text/plain", "camera error");
    return;
  }

  // Method 1: Using server.send() with String conversion (works for smaller images)
  // This method converts the binary data to a String, which works but is not optimal for large images
  /*
  server.sendHeader("Cache-Control", "no-store");
  String jpeg((char*)fb->buf, fb->len);
  server.send(200, "image/jpeg", jpeg);
  */

  // Method 2: Manual HTTP response with proper status line
  WiFiClient client = server.client();
  if (client) {
    // Send HTTP response headers
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: image/jpeg");
    client.println("Cache-Control: no-store");
    client.println("Content-Length: " + String(fb->len));
    client.println("Connection: close");
    client.println();  // End of headers

    // Send the image data
    client.write(fb->buf, fb->len);
    client.flush();
  }

  esp_camera_fb_return(fb);
}

void initCamera() {
  camera_config_t cfg = {};
  cfg.ledc_channel    = LEDC_CHANNEL_0;
  cfg.ledc_timer      = LEDC_TIMER_0;
  cfg.pin_d0          = Y2_GPIO_NUM;  cfg.pin_d1 = Y3_GPIO_NUM;
  cfg.pin_d2          = Y4_GPIO_NUM;  cfg.pin_d3 = Y5_GPIO_NUM;
  cfg.pin_d4          = Y6_GPIO_NUM;  cfg.pin_d5 = Y7_GPIO_NUM;
  cfg.pin_d6          = Y8_GPIO_NUM;  cfg.pin_d7 = Y9_GPIO_NUM;
  cfg.pin_xclk        = XCLK_GPIO_NUM;
  cfg.pin_pclk        = PCLK_GPIO_NUM;
  cfg.pin_vsync       = VSYNC_GPIO_NUM;
  cfg.pin_href        = HREF_GPIO_NUM;
  cfg.pin_sscb_sda    = SIOD_GPIO_NUM;
  cfg.pin_sscb_scl    = SIOC_GPIO_NUM;
  cfg.pin_pwdn        = PWDN_GPIO_NUM;
  cfg.pin_reset       = RESET_GPIO_NUM;
  cfg.xclk_freq_hz    = 20000000;
  cfg.pixel_format    = PIXFORMAT_JPEG;
  cfg.frame_size      = FRAMESIZE_QVGA;  // 320x240
  cfg.jpeg_quality    = 12;
  cfg.fb_count        = 1;

  // PSRAM detection and configuration
  if (psramFound()) {
    Serial.println("[cam] PSRAM found, using 2 frame buffers");
    cfg.fb_count = 2;
    cfg.grab_mode = CAMERA_GRAB_LATEST;
    cfg.fb_location = CAMERA_FB_IN_PSRAM;
  } else {
    Serial.println("[cam] No PSRAM, using 1 frame buffer in DRAM");
    cfg.fb_location = CAMERA_FB_IN_DRAM;
  }

  esp_err_t err = esp_camera_init(&cfg);
  if (err != ESP_OK) {
    Serial.printf("[cam] init failed: 0x%x\n", err);
    Serial.println("[cam] Common fixes: check camera cable, power supply (5V), or try lower resolution");
  } else {
    Serial.println("[cam] Camera initialized successfully");

    // Test frame capture
    camera_fb_t *fb = esp_camera_fb_get();
    if (fb) {
      Serial.printf("[cam] Test capture OK: %dx%d, %d bytes\n", fb->width, fb->height, fb->len);
      esp_camera_fb_return(fb);
    } else {
      Serial.println("[cam] Warning: Test capture failed!");
    }
  }
}

// ---- Setup / loop ----------------------------------------------------------
void setup() {
  Serial.begin(115200);
  Serial2.begin(ARDUINO_BAUD, SERIAL_8N1, ARDUINO_RX_PIN, ARDUINO_TX_PIN, SERIAL_INVERT);
  delay(200);
  initCamera();

  pinMode(STATUS_LED_PIN, OUTPUT);
  ledWrite(false);                       // start with the activity LED off

  // Disable WiFi power saving and set mode
  WiFi.persistent(false);        // Don't save WiFi config to flash (faster, reduces wear)
  WiFi.mode(WIFI_AP);
  WiFi.setTxPower(WIFI_POWER_19_5dBm);  // Max power for better range

  // Configure AP with specific channel and settings for reliability
  // Channel 1, hidden=false, max_connections=4
  bool ok = WiFi.softAP(AP_SSID, (strlen(AP_PASS) >= 8) ? AP_PASS : nullptr, 1, 0, 4);

  IPAddress ip = WiFi.softAPIP();
  Serial.printf("\n[wifi] AP \"%s\" %s\n", AP_SSID, ok ? "started" : "FAILED");
  Serial.printf("[wifi] Channel 1, max power, IP: http://%s\n", ip.toString().c_str());

  dnsServer.start(DNS_PORT, "*", ip);

  server.on("/", handleRoot);
  server.on("/c", handleControl);
  server.on("/s", handleStop);
  server.on("/cam", handleCam);
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





