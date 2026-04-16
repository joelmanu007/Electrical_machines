/*
 * ╔══════════════════════════════════════════════════════════════╗
 *   INDUCTION MOTOR AIR-GAPPED DIAGNOSTIC SYSTEM
 *   ESP32 + ADXL345 + SSD1306 OLED                   v5 (final)
 * ╠══════════════════════════════════════════════════════════════╣
 *   LIBRARIES (Sketch -> Manage Libraries):
 *   ├─ Adafruit ADXL345         (Adafruit)
 *   ├─ Adafruit Unified Sensor  (Adafruit)
 *   ├─ Adafruit SSD1306         (Adafruit)
 *   └─ Adafruit GFX Library     (Adafruit)
 *   QRCode: bundled qrcode.h + qrcode.c in this folder (done!)
 * ╚══════════════════════════════════════════════════════════════╝
 */

#include <Wire.h>
#include <math.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_ADXL345_U.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include "qrcode.h"

// ── WI-FI & BACKEND CONFIG (ACTION REQUIRED!) ─────────────────
const char* WIFI_SSID     = "Test";
const char* WIFI_PASSWORD = "12345678";
const char* BACKEND_URL   = "https://electrical-machines.onrender.com/api/diagnose";

// ── CONFIG ────────────────────────────────────────────────────
// Your Vercel project name
const char* VERCEL_DOMAIN = "electrical-machines.vercel.app";

// How long to keep the QR on screen (seconds)
const int QR_DISPLAY_SEC = 15;

// Samples per measurement window
const int SAMPLE_COUNT = 512;

// ── HARDWARE ──────────────────────────────────────────────────
Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified(12345);

#define SCREEN_W  128
#define SCREEN_H   64
Adafruit_SSD1306 display(SCREEN_W, SCREEN_H, &Wire, -1);

float samples[SAMPLE_COUNT];

// ══════════════════════════════════════════════════════════════
void setup() {
  Serial.begin(115200);
  delay(500);
  Wire.begin(21, 22);
  Wire.setClock(400000);

  // OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED not found"); while (1);
  }
  display.ssd1306_command(SSD1306_SETCONTRAST);
  display.ssd1306_command(0xFF);  // max brightness

  oledBoot("OLED OK", 1);

  // ADXL345
  if (!accel.begin()) {
    display.clearDisplay(); display.setTextColor(WHITE); display.setTextSize(1);
    display.setCursor(0,10); display.println("ADXL345 NOT FOUND");
    display.setCursor(0,22); display.println("CS -> 3.3V");
    display.setCursor(0,32); display.println("SDO -> GND");
    display.setCursor(0,42); display.println("SDA -> GPIO21");
    display.setCursor(0,52); display.println("SCL -> GPIO22");
    display.display(); while (1);
  }
  accel.setRange(ADXL345_RANGE_4_G);
  oledBoot("ADXL OK", 2);

  // 10-second sensor warmup
  display.clearDisplay(); display.setTextColor(WHITE); display.setTextSize(1);
  display.setCursor(20, 8);  display.println("Warming up...");
  display.setCursor(20, 20); display.println("Please wait 10s");
  display.drawRect(4, 44, 120, 12, WHITE);
  display.display();
  for (int s = 0; s < 10; s++) {
    display.fillRect(5, 45, (s+1)*11, 10, WHITE);
    display.display();
    delay(1000);
  }

  // ── Connect to Wi-Fi ────────────────────────────────────────
  display.clearDisplay();
  display.setCursor(0, 10); display.println("Connecting to:");
  display.setCursor(0, 24); display.println(WIFI_SSID);
  display.display();
  
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  int toggle = 0;
  while (WiFi.status() != WL_CONNECTED) {
    if (toggle) display.fillRect(100, 24, 8, 8, WHITE);
    else display.fillRect(100, 24, 8, 8, BLACK);
    display.display();
    toggle = !toggle;
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi Connected!");
  oledBoot("READY & ONLINE", 4);
  delay(1000);
  Serial.printf("URL base: https://%s/\n", VERCEL_DOMAIN);
}

// ══════════════════════════════════════════════════════════════
void loop() {
  // Scanning progress screen
  display.clearDisplay(); display.setTextColor(WHITE); display.setTextSize(1);
  display.setCursor(12,5);  display.println("SCANNING MOTOR...");
  display.drawLine(0,16,128,16,WHITE);
  display.setCursor(5,28);  display.println("Reading vibration");
  display.setCursor(5,40);  display.printf("%d samples @ 1kHz", SAMPLE_COUNT);
  display.drawRect(10,52,108,8,WHITE);
  display.display();

  // Collect samples + remove gravity bias
  collectSamples();

  // Compute features
  float rms   = computeRMS();
  float peak  = computePeak();
  float crest = computeCrestFactor(rms, peak);
  float kurt  = computeKurtosis();

  Serial.printf("RMS=%.4f  Peak=%.4f  Crest=%.4f  Kurt=%.4f\n",
                 rms, peak, crest, kurt);

  // ── POST Data to Backend ────────────────────────────────────
  display.clearDisplay(); display.setTextColor(WHITE); display.setTextSize(1);
  display.setCursor(0,25); display.println("uploading data...");
  display.display();

  if (WiFi.status() == WL_CONNECTED) {
    WiFiClientSecure client;
    client.setInsecure(); // Bypass SSL certificate validation for Render
    
    HTTPClient http;
    http.setTimeout(30000); // Wait up to 30s for Render servers to wake up from sleep
    http.begin(client, BACKEND_URL);
    http.addHeader("Content-Type", "application/json");

    // Manually construct JSON to avoid forcing the user to install ArduinoJson
    String jsonPayload = "{\"rms\":" + String(rms, 4) +
                         ",\"peak\":" + String(peak, 4) +
                         ",\"crest_factor\":" + String(crest, 4) +
                         ",\"kurtosis\":" + String(kurt, 4) + "}";
                         
    Serial.println("POSTing: " + jsonPayload);
    int httpResponseCode = http.POST(jsonPayload);
    Serial.printf("HTTP Response code: %d\n", httpResponseCode);
    http.end();
  } else {
    Serial.println("Wi-Fi Disconnected!");
  }

  // Phase 1: QR screen (QR_DISPLAY_SEC seconds)
  // Because the ESP32 pushes the data directly over WiFi, the QR code
  // now ONLY needs to hold the bare link to the dashboard!
  // This ultra-short string easily fits in Version 2.
  showQR(VERCEL_DOMAIN);

  // Phase 2: Numeric readings (5 seconds)
  showReadings(rms, peak, crest, kurt);
  delay(5000);
}

// ══════════════════════════════════════════════════════════════
//   QR CODE — As per "Mario's Ideas" YouTube tutorial
//
//   Key changes for SSD1306 OLED based on the video:
//   1. No white background rectangle! It creates OLED glare.
//   2. Draw the QR code inverted: WHITE modules on BLACK screen.
//   Modern smartphone cameras perfectly handle inverted QRs,
//   and the naturally black OLED screen acts as the quiet zone.
// ══════════════════════════════════════════════════════════════
void showQR(const char* url) {
  QRCode qr;
  uint8_t buf[qrcode_getBufferSize(6)];

  // 1. Generate QR
  bool ok = false;
  for (uint8_t v = 2; v <= 6 && !ok; v++) {
    if (qrcode_initText(&qr, buf, v, ECC_LOW, url) == 0) {
      ok = true;
      Serial.printf("QR: Version %d  (%d x %d modules)\n", v, qr.size, qr.size);
    }
  }

  if (!ok) {
    display.clearDisplay(); display.setTextColor(WHITE); display.setTextSize(1);
    display.setCursor(20,20); display.println("QR FAILED");
    display.display(); delay(3000); return;
  }

  // 2. Exactly Mario's Scaling and Centering Logic
  int scale = SCREEN_H / qr.size;
  if(scale < 1) scale = 1;

  int shiftX = (SCREEN_W - qr.size * scale) / 2;
  int shiftY = (SCREEN_H - qr.size * scale) / 2;

  Serial.printf("Scale used: %d. Final QR Size on screen: %d x %d pixels\n", scale, qr.size * scale, qr.size * scale);

  // 3. Draw directly to clear display
  display.clearDisplay();

  for (uint8_t y = 0; y < qr.size; y++) {
    for (uint8_t x = 0; x < qr.size; x++) {
      // display.fillRect(X, Y, WIDTH, HEIGHT, COLOR)
      if (qrcode_getModule(&qr, x, y)) {
        display.fillRect(shiftX + x * scale, shiftY + y * scale, scale, scale, WHITE);
      }
    }
  }

  display.display();
  delay((unsigned long)QR_DISPLAY_SEC * 1000);
}

// ══════════════════════════════════════════════════════════════
//   READINGS SCREEN
// ══════════════════════════════════════════════════════════════
void showReadings(float rms, float peak, float crest, float kurt) {
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(18, 0);  display.println("SENSOR READINGS");
  display.drawLine(0, 9, 128, 9, WHITE);
  display.setCursor(0, 14); display.printf("RMS      : %.4f g", rms);
  display.setCursor(0, 26); display.printf("Peak     : %.4f g", peak);
  display.setCursor(0, 38); display.printf("Crest    : %.3f",   crest);
  display.setCursor(0, 50); display.printf("Kurtosis : %.3f",   kurt);
  display.display();
}

// ══════════════════════════════════════════════════════════════
//   BOOT SCREEN
// ══════════════════════════════════════════════════════════════
void oledBoot(const char* msg, int step) {
  display.clearDisplay(); display.setTextColor(WHITE); display.setTextSize(1);
  display.setCursor(0, 0);  display.println("MOTOR DIAGNOSTIC");
  display.setCursor(0, 10); display.println("AIR-GAPPED v5.0");
  display.drawLine(0, 20, 128, 20, WHITE);
  display.setCursor(0, 28); display.printf("Step %d: %s", step, msg);
  display.drawRect(0, 50, 128, 10, WHITE);
  display.fillRect(0, 50, step * 40, 10, WHITE);
  display.display();
  delay(600);
}

// ══════════════════════════════════════════════════════════════
//   DATA COLLECTION  (+  DC offset / gravity removal)
// ══════════════════════════════════════════════════════════════
void collectSamples() {
  sensors_event_t event;
  for (int i = 0; i < SAMPLE_COUNT; i++) {
    accel.getEvent(&event);
    samples[i] = event.acceleration.z / 9.81f;  // raw g (includes 1g gravity)
    if (i % 64 == 0) {
      int p = map(i, 0, SAMPLE_COUNT, 11, 117);
      display.fillRect(11, 53, p-11, 6, WHITE);
      display.display();
    }
    delayMicroseconds(800);                       // ~1.25 kHz
  }

  // Remove DC offset (gravity). The Z-axis reads ~1g at rest.
  // Vibration = AC component only → subtract the mean.
  // Without this: RMS ≈ 1.0 g at rest → model incorrectly says FAULT.
  float mean = 0;
  for (int i = 0; i < SAMPLE_COUNT; i++) mean += samples[i];
  mean /= SAMPLE_COUNT;
  for (int i = 0; i < SAMPLE_COUNT; i++) samples[i] -= mean;
  Serial.printf("DC offset removed: %.3f g\n", mean);
}

// ══════════════════════════════════════════════════════════════
//   FEATURE COMPUTATION
// ══════════════════════════════════════════════════════════════
float computeRMS() {
  float s = 0;
  for (int i = 0; i < SAMPLE_COUNT; i++) s += samples[i]*samples[i];
  return sqrtf(s / SAMPLE_COUNT);
}

float computePeak() {
  float mx = 0;
  for (int i = 0; i < SAMPLE_COUNT; i++) {
    float a = fabsf(samples[i]);
    if (a > mx) mx = a;
  }
  return mx;
}

float computeCrestFactor(float rms, float peak) {
  // If RMS is at the hardware noise floor (e.g. motor is practically off), 
  // return an ideal sine wave crest factor to prevent math explosions.
  if (rms < 0.03f) return 1.414f; 
  return peak / rms;
}

float computeKurtosis() {
  float mean = 0;
  for (int i = 0; i < SAMPLE_COUNT; i++) mean += samples[i];
  mean /= SAMPLE_COUNT;

  float var = 0, m4 = 0;
  for (int i = 0; i < SAMPLE_COUNT; i++) {
    float d = samples[i]-mean, d2 = d*d;
    var += d2; m4 += d2*d2;
  }
  var /= SAMPLE_COUNT; m4 /= SAMPLE_COUNT;
  
  // ── Noise Floor Gate ───────────────────────────────────────────
  // A completely still sensor generates tiny electrical noise.
  // 0.0001² is extremely small. Dividing m4 by ~zero causes 
  // the Kurtosis to rocket up to 50+, tricking the ML model into
  // thinking it's a critical fault. 
  if (var < 0.001f) return 3.0f; 

  return m4/(var*var);
}
