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
WiFiClient client; // Needed for HTTPUpdate

// --- Function Prototypes ---
bool connectWiFi();
bool fetchConfig(JsonDocument& doc);

void setup() {
  Serial.begin(115200);
  for (int p : ledPins) pinMode(p, OUTPUT);

  // Default values
  int duration = 8; 

  if (connectWiFi()) {
    JsonDocument doc; 
    if (fetchConfig(doc)) {
      // 1. Update duration from Gist
      duration = doc["light_duration_hours"] | 8; 

      // 2. Check for Firmware Update
      int remoteVersion = doc["firmware_version"] | 1;
      if (remoteVersion > CURRENT_VERSION) {
        Serial.println("Update detected. Updating...");
        httpUpdate.update(client, doc["ota_bin_url"]);
      }
    }
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
  }

  // 3. Flicker for the duration fetched from Gist
  Serial.print("Flickering for "); Serial.print(duration); Serial.println(" hours.");
  unsigned long endTime = millis() + ((unsigned long)duration * 3600000UL);
  while (millis() < endTime) {
    for (int i = 0; i < 3; i++) {
      analogWrite(ledPins[i], random(50, 255));
    }
    delay(random(50, 150));
  }

  // 4. Sleep for the remaining time
  uint64_t sleepTimeSeconds = (24 - duration) * 3600ULL;
  Serial.println("Entering deep sleep...");
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