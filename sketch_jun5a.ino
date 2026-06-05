#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <HTTPUpdate.h>

// --- Globals ---
const int CURRENT_VERSION = 1; 
const char* ssid = "Elmeier";
const char* password = "7178246672";
const char* gistUrl = "https://gist.github.com/Telmeier/c8299898f0cd224551bad47cf1787b1f/raw/config.json";
const int ledPins[] = {D1, D2, D3};
WiFiClient client;

void setup() {
  Serial.begin(115200);
  for (int p : ledPins) pinMode(p, OUTPUT);

  // Default values
  int duration = 8;
  int brightness = 75;
  String hibernateUntil = "0";

  if (connectWiFi()) {
    JsonDocument doc; 
    if (fetchConfig(doc)) {
      // 1. Capture ALL your Gist settings
      duration = doc["light_duration_hours"] | 8;
      brightness = doc["brightness_percent"] | 75;
      hibernateUntil = doc["unit_hibernate_until"].as<String>();

      // 2. Check for Firmware Update
      int remoteVersion = doc["firmware_version"] | 1;
      if (remoteVersion > CURRENT_VERSION) {
        httpUpdate.update(client, doc["ota_bin_url"]);
      }
    }
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
  }

  // 3. Logic for Hibernation
  if (hibernateUntil != "0") {
    // Here you would add logic to check if current date/time is past hibernateUntil
    // For now, it just bypasses the LEDs if not "0"
    esp_deep_sleep_start(); 
  }

  // 4. Flicker with brightness factor
  unsigned long endTime = millis() + ((unsigned long)duration * 3600000UL);
  while (millis() < endTime) {
    for (int i = 0; i < 3; i++) {
      // Scale brightness by the Gist value (0.0 to 1.0)
      float bFactor = brightness / 100.0;
      analogWrite(ledPins[i], random(50, 255) * bFactor);
    }
    delay(random(50, 150));
  }

  // 5. Sleep
  uint64_t sleepTimeSeconds = (24 - duration) * 3600ULL;
  esp_sleep_enable_timer_wakeup(sleepTimeSeconds * 1000000ULL);
  esp_deep_sleep_start();
}

bool connectWiFi() {
  WiFi.begin(ssid, password);
  for(int i=0; i<10; i++) {
    if (WiFi.status() == WL_CONNECTED) return true;
    delay(1000);
  }
  return false;
}

bool fetchConfig(JsonDocument& doc) {
  HTTPClient http;
  http.begin(client, gistUrl);
  int httpCode = http.GET();
  if (httpCode == 200) {
    deserializeJson(doc, http.getString());
    http.end();
    return true;
  }
  http.end();
  return false;
}

void loop() {}
