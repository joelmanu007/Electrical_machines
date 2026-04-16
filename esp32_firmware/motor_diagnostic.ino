/*
 * ============================================================
 *   INDUCTION MOTOR DIAGNOSTIC FIRMWARE
 *   ESP32 + MPU6050 Accelerometer
 * ============================================================
 *
 * WHAT THIS CODE DOES:
 *   1. Connects ESP32 to your WiFi network
 *   2. Reads raw vibration data from MPU6050 accelerometer
 *      at ~1000 samples/second over a 1-second window
 *   3. Computes the 4 features your ML model expects:
 *      - RMS Vibration (rms_vibration)
 *      - Peak Vibration (peak_vibration)
 *      - Crest Factor   (crest_factor)
 *      - Kurtosis       (kurtosis)
 *   4. Sends them as a JSON POST to your Render cloud API
 *   5. Prints the AI diagnosis back to Serial Monitor
 *
 * REQUIRED LIBRARIES (install via Arduino Library Manager):
 *   - Adafruit MPU6050
 *   - Adafruit Unified Sensor
 *   - ArduinoJson  (version 6.x)
 *   - WiFi         (built-in for ESP32)
 *   - HTTPClient   (built-in for ESP32)
 *
 * ============================================================
 */

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
#include <math.h>

// ─────────────────────────────────────────────────────────────
//   CONFIGURATION — CHANGE THESE TO YOUR OWN VALUES
// ─────────────────────────────────────────────────────────────
const char* WIFI_SSID     = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
const char* API_ENDPOINT  = "https://electrical-machines.onrender.com/api/diagnose";

// Sampling configuration
const int   SAMPLE_COUNT     = 512;   // Number of accelerometer samples per scan
const int   SAMPLE_DELAY_MS  = 1;     // Delay between samples (1ms = ~1kHz)
const int   SCAN_INTERVAL_MS = 5000;  // How often to run a full scan (5 seconds)

// ─────────────────────────────────────────────────────────────
//   GLOBALS
// ─────────────────────────────────────────────────────────────
Adafruit_MPU6050 mpu;
float accelBuffer[SAMPLE_COUNT]; // Stores raw Z-axis vibration samples

// ─────────────────────────────────────────────────────────────
//   SETUP
// ─────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("=================================================");
  Serial.println("  MOTOR DIAGNOSTIC SYSTEM — BOOTING...");
  Serial.println("=================================================");

  // 1. Initialize MPU6050
  Wire.begin(21, 22); // SDA=GPIO21, SCL=GPIO22 (default ESP32 I2C pins)
  if (!mpu.begin()) {
    Serial.println("❌ ERROR: MPU6050 not found! Check wiring.");
    Serial.println("   → SDA to GPIO 21");
    Serial.println("   → SCL to GPIO 22");
    while (1) delay(10); // Halt
  }
  Serial.println("✅ MPU6050 sensor initialized.");

  // Configure MPU6050 for vibration measurement
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G); // ±8g range — good for motors
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);   // Cut high-frequency electrical noise

  // 2. Connect to WiFi
  Serial.printf("📡 Connecting to WiFi: %s\n", WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  int retries = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    retries++;
    if (retries > 40) {
      Serial.println("\n❌ WiFi connection failed! Check SSID/Password.");
      while (1) delay(10);
    }
  }
  Serial.printf("\n✅ WiFi connected! IP: %s\n", WiFi.localIP().toString().c_str());
  Serial.println("=================================================");
  Serial.println("  System ready. Starting diagnostic scans...");
  Serial.println("=================================================\n");
}

// ─────────────────────────────────────────────────────────────
//   MAIN LOOP
// ─────────────────────────────────────────────────────────────
void loop() {
  Serial.println("──────────────────────────────────────");
  Serial.println("🔄 Starting new vibration scan...");

  // Step 1: Collect raw accelerometer samples
  collectSamples();

  // Step 2: Compute the 4 features from raw data
  float rms         = computeRMS();
  float peak        = computePeak();
  float crest       = computeCrestFactor(rms, peak);
  float kurt        = computeKurtosis();

  // Step 3: Print computed features to Serial Monitor
  Serial.println("📊 Computed Features:");
  Serial.printf("   RMS Vibration  : %.4f g\n", rms);
  Serial.printf("   Peak Vibration : %.4f g\n", peak);
  Serial.printf("   Crest Factor   : %.4f\n", crest);
  Serial.printf("   Kurtosis       : %.4f\n", kurt);

  // Step 4: Send to cloud API and get diagnosis
  sendToAPI(rms, peak, crest, kurt);

  // Step 5: Wait before next scan
  Serial.printf("⏳ Next scan in %d seconds...\n\n", SCAN_INTERVAL_MS / 1000);
  delay(SCAN_INTERVAL_MS);
}

// ─────────────────────────────────────────────────────────────
//   DATA COLLECTION
// ─────────────────────────────────────────────────────────────
void collectSamples() {
  sensors_event_t accel, gyro, temp;
  for (int i = 0; i < SAMPLE_COUNT; i++) {
    mpu.getEvent(&accel, &gyro, &temp);
    // Use Z-axis for motors mounted vertically on a flat surface.
    // If your motor is horizontal, switch to accel.acceleration.y
    accelBuffer[i] = accel.acceleration.z / 9.81; // Convert m/s² to g
    delay(SAMPLE_DELAY_MS);
  }
}

// ─────────────────────────────────────────────────────────────
//   STATISTICAL FEATURE COMPUTATION
// ─────────────────────────────────────────────────────────────

// RMS (Root Mean Square) — overall vibration energy level
float computeRMS() {
  float sumSq = 0.0;
  for (int i = 0; i < SAMPLE_COUNT; i++) {
    sumSq += accelBuffer[i] * accelBuffer[i];
  }
  return sqrt(sumSq / SAMPLE_COUNT);
}

// Peak — maximum absolute vibration amplitude
float computePeak() {
  float maxVal = 0.0;
  for (int i = 0; i < SAMPLE_COUNT; i++) {
    float absVal = fabs(accelBuffer[i]);
    if (absVal > maxVal) maxVal = absVal;
  }
  return maxVal;
}

// Crest Factor — ratio of Peak to RMS (detects impulsive impacts)
float computeCrestFactor(float rms, float peak) {
  if (rms < 0.0001) return 0.0; // Avoid division by zero
  return peak / rms;
}

// Kurtosis — statistical measure of "spikiness" (key bearing fault indicator)
// Healthy bearing: ~3.0 | Faulty bearing: >5.0
float computeKurtosis() {
  // Calculate mean
  float mean = 0.0;
  for (int i = 0; i < SAMPLE_COUNT; i++) mean += accelBuffer[i];
  mean /= SAMPLE_COUNT;

  // Calculate variance (std^2) and 4th central moment
  float variance = 0.0;
  float moment4  = 0.0;
  for (int i = 0; i < SAMPLE_COUNT; i++) {
    float diff = accelBuffer[i] - mean;
    float diff2 = diff * diff;
    variance += diff2;
    moment4  += diff2 * diff2;
  }
  variance /= SAMPLE_COUNT;
  moment4  /= SAMPLE_COUNT;

  if (variance < 1e-10) return 3.0; // If no variance, return normal kurtosis
  return moment4 / (variance * variance);
}

// ─────────────────────────────────────────────────────────────
//   API COMMUNICATION
// ─────────────────────────────────────────────────────────────
void sendToAPI(float rms, float peak, float crest, float kurt) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("❌ WiFi disconnected. Reconnecting...");
    WiFi.reconnect();
    delay(3000);
    return;
  }

  // Build JSON payload
  StaticJsonDocument<256> doc;
  doc["rms"]          = rms;
  doc["peak"]         = peak;
  doc["crest_factor"] = crest;
  doc["kurtosis"]     = kurt;

  String jsonPayload;
  serializeJson(doc, jsonPayload);

  Serial.println("📡 Sending data to cloud API...");
  Serial.printf("   Endpoint: %s\n", API_ENDPOINT);
  Serial.printf("   Payload:  %s\n", jsonPayload.c_str());

  HTTPClient http;
  http.begin(API_ENDPOINT);
  http.addHeader("Content-Type", "application/json");
  http.setTimeout(15000); // 15s timeout (Render free tier can be slow on cold start)

  int httpResponseCode = http.POST(jsonPayload);

  if (httpResponseCode == 200) {
    String response = http.getString();

    // Parse the JSON response
    StaticJsonDocument<512> respDoc;
    DeserializationError error = deserializeJson(respDoc, response);

    if (!error) {
      const char* status     = respDoc["status"];
      float       confidence = respDoc["confidence"];

      Serial.println("\n┌─────────────────────────────────┐");
      Serial.printf( "│  AI DIAGNOSIS: %-17s│\n", status);
      Serial.printf( "│  Confidence:   %.1f%%               │\n", confidence);
      Serial.println("└─────────────────────────────────┘");

      // Visual indicator on Serial
      if (strcmp(status, "HEALTHY") == 0) {
        Serial.println("✅ Motor is operating normally.\n");
      } else {
        Serial.println("🚨 FAULT DETECTED — Inspect motor immediately!\n");
      }
    } else {
      Serial.printf("⚠️  JSON parse error: %s\n", error.c_str());
    }
  } else {
    Serial.printf("❌ HTTP Error: %d\n", httpResponseCode);
    Serial.printf("   Response: %s\n", http.getString().c_str());
  }

  http.end();
}
