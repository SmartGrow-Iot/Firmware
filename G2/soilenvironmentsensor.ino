#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <DHT.h>

// --- Macro Definitions ---
#define DHT_PIN       4
#define DHT_TYPE      DHT11
#define SOIL_PIN      32  // Analog pin for soil moisture sensor

// --- WiFi Credentials (Constants) ---
const char* WIFI_SSID     = "Cynex@2.4GHz";
const char* WIFI_PASSWORD = "cyber@cynex";

// --- API Endpoint (Constant) ---
const char* API_URL = "https://test-server-owq2.onrender.com/api/v1/plants/plant_jZkYQoPWp3XiylDU5jA6";

// --- Global Objects ---
DHT dhtSensor(DHT_PIN, DHT_TYPE);

/**
 * @brief Setup function for initializing Serial, DHT, and WiFi.
 */
void setup()
{
  Serial.begin(9600);
  dhtSensor.begin();

  Serial.print("Connecting to WiFi");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

/**
 * @brief Main loop for fetching thresholds and reading sensor data.
 */
void loop()
{
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient httpClient;
    httpClient.begin(API_URL);
    int httpResponseCode = httpClient.GET();

    if (httpResponseCode > 0) {
      String payload = httpClient.getString();
      Serial.println("\nReceived API response:");
      Serial.println(payload);

      StaticJsonDocument<2048> jsonDoc;
      DeserializationError jsonError = deserializeJson(jsonDoc, payload);

      if (!jsonError) {
        // --- Extract thresholds from JSON ---
        float tempMin  = jsonDoc["thresholds"]["temperature"]["min"];
        float tempMax  = jsonDoc["thresholds"]["temperature"]["max"];
        float soilMin  = jsonDoc["thresholds"]["moisture"]["min"];
        float soilMax  = jsonDoc["thresholds"]["moisture"]["max"];
        float lightMin = jsonDoc["thresholds"]["light"]["min"];
        float lightMax = jsonDoc["thresholds"]["light"]["max"];

        Serial.println("\n---- Thresholds from API ----");
        Serial.printf("  Temperature: %.1f°C to %.1f°C\n", tempMin, tempMax);
        Serial.printf("Soil Moisture: %.1f%% to %.1f%%\n", soilMin, soilMax);
        Serial.printf(" Light Level: %.1f%% to %.1f%% (not measured)\n", lightMin, lightMax);
        Serial.println("--------------------------------");

        // --- Read sensor data ---
        float temperature = dhtSensor.readTemperature();
        float humidity    = dhtSensor.readHumidity();
        int rawSoil       = analogRead(SOIL_PIN);  // Range: 0–4095 (ESP32 ADC)
        float soilPercent = map(rawSoil, 0, 4095, 100, 0);  // Adjust based on calibration

        Serial.println("\n---- Sensor Readings ----");
        Serial.printf(" Temperature: %.1f°C\n", temperature);
        Serial.printf("    Humidity: %.1f%%\n", humidity);
        Serial.printf("Soil Moisture: %.1f%% (raw: %d)\n", soilPercent, rawSoil);
        Serial.println("-----------------------------");

        // --- Compare sensor readings with thresholds ---
        Serial.println("\n---- Comparison ----");

        if (temperature < tempMin) {
          Serial.println("Temperature is too LOW!");
        } else if (temperature > tempMax) {
          Serial.println("Temperature is too HIGH!");
        } else {
          Serial.println("Temperature is OK.");
        }

        if (soilPercent < soilMin) {
          Serial.println("Soil moisture is too LOW!");
        } else if (soilPercent > soilMax) {
          Serial.println("Soil moisture is too HIGH!");
        } else {
          Serial.println("Soil moisture is OK.");
        }

        Serial.println("-----------------------------\n");

      } else {
        Serial.print("JSON Parsing error: ");
        Serial.println(jsonError.c_str());
      }

    } else {
      Serial.print("HTTP Error: ");
      Serial.println(httpResponseCode);
    }

    httpClient.end();

  } else {
    Serial.println("WiFi Disconnected!");
  }

  delay(3600000);  // Wait 1 hour before next iteration
}
