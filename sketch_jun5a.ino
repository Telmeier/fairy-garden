#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <HTTPUpdate.h>
#include <time.h>

const int CURRENT_VERSION = 2;
const char* ssid = "Elmeier";
const char* password = "7178246672";
const char* gistUrl = "https://gist.github.com/Telmeier/c8299898f0cd224551bad47cf1787b1f/raw/config.json";
const int ledPins[] = {D1, D2, D3};
WiFiClient client;

void setup() {
  Serial.begin(115200);
  for (int p : ledPins) pinMode(p, OUTPUT);

  if (connectWiFi()) {
    configTime(-18000, 3600, "pool.ntp.org"); // Set to EST
    JsonDocument doc;
    if (fetchConfig(doc)) {
      
      // OTA Update Check
      if ((int)doc["firmware_version"] > CURRENT_VERSION) {
        httpUpdate.update(client, doc["ota_bin_url"]);
      }

      // Time Logic
      struct tm timeinfo;
      if (getLocalTime(&timeinfo)) {
        String startTime = doc["start_time_12hr"].as<String>();
        int targetHour = parseHour(startTime);
        int targetMin = parseMinute(startTime);

        // If current time is before start time, sleep until it is time
        if (timeinfo.tm_hour < targetHour || (timeinfo.tm_hour == targetHour && timeinfo.tm_min < targetMin)) {
          long sleepSecs = ((targetHour - timeinfo.tm_hour) * 3600) + ((targetMin - timeinfo.tm_min) * 60);
          esp_sleep_enable_timer_wakeup(sleepSecs * 1000000ULL);
          esp_deep_sleep_start();
        }
      }

      // Flicker Routine
      int duration = doc["light_duration_hours"] | 8;
      int brightness = doc["brightness_percent"] | 100;
      unsigned long endTime = millis() + ((unsigned long)duration * 3600000UL);
      
      while (millis() < endTime) {
        for (int i = 0; i < 3; i++) {
          analogWrite(ledPins[i], random(50, 255) * (brightness / 100.0));
        }
        delay(random(50, 150));
      }
    }
    WiFi.disconnect(true);
  }
  
  // Sleep for remaining 24hr cycle
  esp_deep_sleep_start();
}

int parseHour(String t) {
  int h = t.substring(0, t.indexOf(':')).toInt();
  if (t.indexOf("pm") != -1 && h < 12) h += 12;
  if (t.indexOf("am") != -1 && h == 12) h = 0;
  return h;
}

int parseMinute(String t) {
  return t.substring(t.indexOf(':') + 1, t.indexOf(':') + 3).toInt();
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
  if (http.GET() == 200) {
    deserializeJson(doc, http.getString());
    http.end();
    return true;
  }
  return false;
}

void loop() {}
