#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <DHT.h>

// --- Pin Definitions ---
#define DHT_PIN      4
#define DHT_TYPE     DHT11
#define SOIL_PIN     32  // Analog pin for soil moisture sensor
#define PUMP_PIN     25  // Relay pin (change as per your wiring)

// --- WiFi Credentials ---
const char* WIFI_SSID = "Cynex@2.4GHz";
const char* WIFI_PASSWORD = "cyber@cynex";

// --- API Endpoint ---
const char* API_URL = "https://test-server-owq2.onrender.com/api/v1/plants/plant_jZkYQoPWp3XiylDU5jA6";

// --- Global Objects ---
DHT dht(DHT_PIN, DHT_TYPE);

void setup() {
  Serial.begin(9600);
  dht.begin();

  pinMode(PUMP_PIN, OUTPUT);
  digitalWrite(PUMP_PIN, LOW); // Start with pump OFF

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(API_URL);
    int httpResponseCode = http.GET();

    if (httpResponseCode > 0) {
      String payload = http.getString();
      Serial.println("\nReceived API response:");
      Serial.println(payload);

      StaticJsonDocument<2048> doc;
      DeserializationError error = deserializeJson(doc, payload);

      if (!error) {
        // --- Extract Thresholds ---
        float soilMin = doc["thresholds"]["moisture"]["min"];
        float soilMax = doc["thresholds"]["moisture"]["max"];

        // --- Read Sensor Data ---
        int rawSoil = analogRead(SOIL_PIN); // ESP32 ADC: 0–4095
        float soilPercent = map(rawSoil, 0, 4095, 100, 0); // Adjust as per calibration

        Serial.println("\n---- Sensor Readings ----");
        Serial.printf("Soil Moisture: %.1f%% (raw: %d)\n", soilPercent, rawSoil);

        // --- Control Pump Based on Thresholds ---
        if (soilPercent < soilMin) {
          Serial.println("Soil too dry! Pump ON");
          digitalWrite(PUMP_PIN, HIGH); // ON
        } else if (soilPercent > soilMax) {
          Serial.println("Soil too wet! Pump OFF");
          digitalWrite(PUMP_PIN, LOW);  // OFF
        } else {
          Serial.println("Soil moisture is OK. Pump OFF");
          digitalWrite(PUMP_PIN, LOW);  // OFF
        }

      } else {
        Serial.print("JSON Parsing error: ");
        Serial.println(error.c_str());
      }

    } else {
      Serial.print("HTTP Error: ");
      Serial.println(httpResponseCode);
    }

    http.end();
  } else {
    Serial.println("WiFi Disconnected!");
  }

  delay(10000); // Check every 10 seconds
}