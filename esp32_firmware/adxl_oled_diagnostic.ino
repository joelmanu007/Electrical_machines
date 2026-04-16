/*
 * ╔══════════════════════════════════════════════════════════════╗
 *   INDUCTION MOTOR AIR-GAPPED DIAGNOSTIC SYSTEM
 *   ESP32 Dev Board + ADXL345 Accelerometer + SSD1306 OLED
 * ╠══════════════════════════════════════════════════════════════╣
 *   WHAT THIS CODE DOES (No WiFi needed!):
 *   1. Reads raw vibration from ADXL345 accelerometer
 *   2. Computes 4 ML features: RMS, Peak, Crest Factor, Kurtosis
 *   3. Builds a URL with those features as query parameters
 *   4. Generates a QR code from that URL
 *   5. Displays the QR + live readings on the SSD1306 OLED
 *   6. When user scans QR with phone → Vercel site opens →
 *      AI diagnosis runs automatically in the browser
 * ╠══════════════════════════════════════════════════════════════╣
 *   REQUIRED ARDUINO LIBRARIES (Install via Library Manager):
 *   ├─ Adafruit ADXL345          (author: Adafruit)
 *   ├─ Adafruit Unified Sensor   (author: Adafruit)
 *   ├─ Adafruit SSD1306          (author: Adafruit)
 *   ├─ Adafruit GFX Library      (author: Adafruit)
 *   └─ QRCode                    (author: Richard Moore)
 * ╚══════════════════════════════════════════════════════════════╝
 */

#include <Wire.h>
#include <math.h>

// --- Accelerometer ---
#include <Adafruit_Sensor.h>
#include <Adafruit_ADXL345_U.h>

// --- OLED Display ---
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// --- QR Code Generator ---
#include "qrcode.h"

// ──────────────────────────────────────────────────────────────
//   ★ CONFIGURATION — CHANGE THIS TO YOUR VERCEL URL ★
//   Do NOT include "https://" here, just the domain.
//   IMPORTANT: Shorter URL = bigger QR = easier to scan!
//   Rename your Vercel project to something short (e.g. "mdiag")
//   at vercel.com/dashboard → Settings → General → Project Name
// ──────────────────────────────────────────────────────────────
const char* VERCEL_DOMAIN = "electrical-machines.vercel.app";

// Sampling config
const int SAMPLE_COUNT    = 512;  // Samples per scan window
const int SCAN_INTERVAL_S = 6;    // Seconds between scans (QR display time)

// ──────────────────────────────────────────────────────────────
//   HARDWARE OBJECTS
// ──────────────────────────────────────────────────────────────

// ADXL345 — I2C address 0x53 (when SDO pin connected to GND)
Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified(12345);

// SSD1306 OLED — 128×64 pixels, I2C address 0x3C
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Sample buffer (stores Z-axis vibration readings in g)
float samples[SAMPLE_COUNT];


// ══════════════════════════════════════════════════════════════
//   SETUP
// ══════════════════════════════════════════════════════════════
void setup() {
  Serial.begin(115200);
  delay(500);

  // I2C on default ESP32 pins: SDA=GPIO21, SCL=GPIO22
  Wire.begin(21, 22);
  Wire.setClock(400000); // Fast I2C (400kHz) for quicker sampling

  // ── Init OLED ──
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("❌ SSD1306 OLED not found! Check wiring.");
    while (1);
  }
  showBootScreen("OLED OK", 1);

  // ── Init ADXL345 ──
  if (!accel.begin()) {
    display.clearDisplay();
    display.setTextColor(WHITE);
    display.setCursor(0, 20);
    display.println("ADXL345 NOT FOUND!");
    display.println("Check wiring:");
    display.println("CS -> 3.3V");
    display.println("SDO -> GND");
    display.display();
    Serial.println("❌ ADXL345 not found!");
    while (1);
  }

  // ±4g range — good for most induction motors
  // Change to ADXL345_RANGE_2_G for lighter motors,
  //        or ADXL345_RANGE_8_G for heavy-vibration machinery
  accel.setRange(ADXL345_RANGE_4_G);

  showBootScreen("SYSTEM READY", 2);
  delay(1500);

  Serial.println("✅ Motor Diagnostic System booted.");
  Serial.printf("   QR will link to: https://%s/\n", VERCEL_DOMAIN);
}


// ══════════════════════════════════════════════════════════════
//   MAIN LOOP
// ══════════════════════════════════════════════════════════════
void loop() {
  Serial.println("\n── New Scan ──────────────────────────");

  // 1. Collect raw vibration samples
  showScanningScreen();
  collectSamples();

  // 2. Compute features
  float rms   = computeRMS();
  float peak  = computePeak();
  float crest = computeCrestFactor(rms, peak);
  float kurt  = computeKurtosis();

  // Print to Serial Monitor for debugging
  Serial.printf("RMS:    %.4f g\n", rms);
  Serial.printf("Peak:   %.4f g\n", peak);
  Serial.printf("Crest:  %.4f\n",   crest);
  Serial.printf("Kurt:   %.4f\n",   kurt);

  // 3. Build URL string
  // URL format: https://YOUR_DOMAIN/?r=X.XX&p=X.XX&c=X.XX&k=X.XX
  // Short param names (r,p,c,k) keep the URL compact for a smaller QR
  char url[120];
  snprintf(url, sizeof(url),
    "https://%s/?r=%.2f&p=%.2f&c=%.2f&k=%.2f",
    VERCEL_DOMAIN, rms, peak, crest, kurt
  );
  Serial.printf("URL:    %s\n", url);
  Serial.printf("Length: %d chars\n", strlen(url));

  // 4. Display QR + readings on OLED
  displayQRScreen(rms, peak, crest, kurt, url);

  // 5. Wait — QR stays on screen so user can scan
  Serial.printf("Next scan in %d seconds...\n", SCAN_INTERVAL_S);
  delay((unsigned long)SCAN_INTERVAL_S * 1000);
}


// ══════════════════════════════════════════════════════════════
//   OLED DISPLAY FUNCTIONS
// ══════════════════════════════════════════════════════════════

void showBootScreen(const char* msg, int step) {
  display.clearDisplay();
  display.setTextColor(WHITE);

  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("MOTOR DIAGNOSTIC");
  display.println("AIR-GAPPED v1.0");
  display.drawLine(0, 17, 128, 17, WHITE);

  display.setCursor(0, 24);
  display.printf("Step %d: %s", step, msg);

  // Boot progress bar
  int barWidth = (step * 64);
  display.drawRect(0, 50, 128, 10, WHITE);
  display.fillRect(0, 50, barWidth, 10, WHITE);

  display.display();
}

void showScanningScreen() {
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);

  display.setCursor(20, 5);
  display.println("SCANNING MOTOR...");
  display.drawLine(0, 16, 128, 16, WHITE);

  display.setCursor(5, 28);
  display.println("Reading vibration");
  display.setCursor(5, 40);
  display.printf("%d samples @ ~1kHz", SAMPLE_COUNT);

  display.drawRect(10, 52, 108, 8, WHITE);
  display.display();

  // Animate the progress bar while collecting
  // (actual animation is in collectSamples via display updates)
}

void displayQRScreen(float rms, float peak, float crest, float kurt, const char* url) {
  // ── Generate QR Code (try V2 → V3 → V4 until URL fits) ──
  QRCode qrCode;
  uint8_t qrData[qrcode_getBufferSize(4)]; // Allocate for max Version 4

  bool success = false;
  for (int version = 2; version <= 4 && !success; version++) {
    success = qrcode_initText(&qrCode, qrData, version, ECC_LOW, url);
    if (success) {
      Serial.printf("QR: Version %d, %d×%d modules\n", version, qrCode.size, qrCode.size);
    }
  }

  if (!success) {
    display.clearDisplay();
    display.setCursor(0, 20);
    display.println("URL TOO LONG!");
    display.println("Shorten VERCEL_DOMAIN");
    display.display();
    Serial.println("❌ URL too long for QR V4!");
    return;
  }

  // ── Calculate pixel scale to fit QR on OLED ──
  // Priority: use 2px/module if it fits in 64px height, else 1px
  int scale = (qrCode.size * 2 <= SCREEN_HEIGHT) ? 2 : 1;
  int qr_px = qrCode.size * scale;

  // Center QR vertically; left-align horizontally
  int qr_x = 0;
  int qr_y = (SCREEN_HEIGHT - qr_px) / 2;

  display.clearDisplay();

  // Draw white background for QR quiet zone area
  display.fillRect(qr_x, qr_y, qr_px, qr_px, WHITE);

  // Draw dark (black) modules over the white background
  for (uint8_t row = 0; row < qrCode.size; row++) {
    for (uint8_t col = 0; col < qrCode.size; col++) {
      if (qrcode_getModule(&qrCode, col, row)) {
        display.fillRect(
          qr_x + col * scale,
          qr_y + row * scale,
          scale, scale,
          BLACK
        );
      }
    }
  }

  // ── Text panel on the right ──
  int text_x = qr_px + 2;
  int panel_w = SCREEN_WIDTH - text_x; // Remaining width for text

  display.setTextColor(WHITE);
  display.setTextSize(1); // 6×8px per character

  // Label
  display.setCursor(text_x, 0);
  display.print("SCAN ME");

  display.drawLine(text_x, 9, SCREEN_WIDTH, 9, WHITE);

  // Feature values
  display.setCursor(text_x, 12);
  display.print("rms:");
  display.print(rms, 2);

  display.setCursor(text_x, 22);
  display.print("pk:");
  display.print(peak, 2);

  display.setCursor(text_x, 32);
  display.print("cst:");
  display.print(crest, 1);

  display.setCursor(text_x, 42);
  display.print("krt:");
  display.print(kurt, 1);

  // Hint text at bottom
  display.setCursor(text_x, 56);
  display.print("AI diag");

  display.display();
}


// ══════════════════════════════════════════════════════════════
//   DATA COLLECTION
// ══════════════════════════════════════════════════════════════
void collectSamples() {
  sensors_event_t event;

  for (int i = 0; i < SAMPLE_COUNT; i++) {
    accel.getEvent(&event);

    // Use Z-axis (perpendicular to motor shaft for radial vibration).
    // If mounted differently, try event.acceleration.x or .y
    samples[i] = event.acceleration.z / 9.81f; // Convert m/s² → g

    // Update progress bar on OLED every 64 samples
    if (i % 64 == 0) {
      int progress = map(i, 0, SAMPLE_COUNT, 10, 118);
      display.fillRect(11, 53, progress - 11, 6, WHITE);
      display.display();
    }

    delayMicroseconds(800); // ~1.25kHz sampling
  }
}


// ══════════════════════════════════════════════════════════════
//   STATISTICAL FEATURE COMPUTATION
//   These match EXACTLY the features the ML model was trained on
// ══════════════════════════════════════════════════════════════

// RMS (Root Mean Square) — overall vibration energy
// Higher = more vibration energy = more wear
float computeRMS() {
  float sumSq = 0.0f;
  for (int i = 0; i < SAMPLE_COUNT; i++) {
    sumSq += samples[i] * samples[i];
  }
  return sqrtf(sumSq / SAMPLE_COUNT);
}

// Peak — maximum instantaneous vibration
// Captures sudden impulses like bearing impacts
float computePeak() {
  float maxVal = 0.0f;
  for (int i = 0; i < SAMPLE_COUNT; i++) {
    float absVal = fabsf(samples[i]);
    if (absVal > maxVal) maxVal = absVal;
  }
  return maxVal;
}

// Crest Factor = Peak / RMS
// Healthy: ~2-3 | Faulty bearing: >5
float computeCrestFactor(float rms, float peak) {
  if (rms < 1e-6f) return 1.0f; // Guard divide-by-zero
  return peak / rms;
}

// Kurtosis — statistical "spikiness" (most sensitive bearing fault indicator)
// Healthy: ~3.0 | Faulty bearing: >5.0 (classic diagnostic threshold)
float computeKurtosis() {
  // Step 1: Compute mean
  float mean = 0.0f;
  for (int i = 0; i < SAMPLE_COUNT; i++) mean += samples[i];
  mean /= SAMPLE_COUNT;

  // Step 2: Compute variance and 4th central moment
  float variance = 0.0f;
  float m4       = 0.0f;
  for (int i = 0; i < SAMPLE_COUNT; i++) {
    float d  = samples[i] - mean;
    float d2 = d * d;
    variance += d2;
    m4       += d2 * d2;
  }
  variance /= SAMPLE_COUNT;
  m4       /= SAMPLE_COUNT;

  // Kurtosis = 4th moment / variance²
  if (variance < 1e-10f) return 3.0f; // No variance → return normal kurtosis
  return m4 / (variance * variance);
}
