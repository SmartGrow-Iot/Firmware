#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// Wi-Fi credentials
const char* ssid = "Cynex@2.4GHz";
const char* password = "cyber@cynex";

// API URL to fetch threshold
const char* url = "https://test-server-owq2.onrender.com/api/v1/plants/plant_kF6h3AT8VaUm73MBkgRP";

// Pin for LDR sensor
const int MQ2_PIN = 34;
const int LDR_PIN = 35;

// Light thresholds
float lightMin = 0.0;
float lightMax = 0.0;
float airQualityMin = 0.0;
float airQualityMax = 0.0;

// Combined comparison + actuator trigger function
bool checkAndTriggerActuator(const String& sensorName, int sensorValue, float minVal, float maxVal) {
  bool exceeded = (sensorValue < minVal || sensorValue > maxVal);
  Serial.printf("%s Value: %d — Min: %.2f, Max: %.2f\n", sensorName.c_str(), sensorValue, minVal, maxVal);
  return exceeded;
}

void connectToWiFi() {
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected.");
}

bool fetchSensorThresholds() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(url);
    int httpResponseCode = http.GET();

    if (httpResponseCode == 200) {
      String payload = http.getString();

      StaticJsonDocument<1024> doc;
      DeserializationError error = deserializeJson(doc, payload);

      if (!error) {
        lightMin = doc["thresholds"]["light"]["min"];
        lightMax = doc["thresholds"]["light"]["max"];
        airQualityMin   = doc["thresholds"]["airQuality"]["min"];
        airQualityMax   = doc["thresholds"]["airQuality"]["max"];

        Serial.printf("Light Threshold Min: %.2f, Max: %.2f\n", lightMin, lightMax);
        Serial.printf("Air Quality Min: %.2f, Max: %.2f\n", airQualityMin, airQualityMax);
        http.end();
        return true;
      } else {
        Serial.println("Failed to parse JSON");
      }
    } else {
      Serial.printf("HTTP GET failed, code: %d\n", httpResponseCode);
    }
    http.end();
  }
  return false;
}

void setup() {
  Serial.begin(115200);
  connectToWiFi();

  if (!fetchLightThreshold()) {
    Serial.println("Using default light thresholds.");
    lightMin = 0;
    lightMax = 0;
    airQualityMin = 0; 
    airQualityMax = 0;
  }

  pinMode(LDR_PIN, INPUT);
  pinMode(MQ2_PIN, INPUT);
}

void loop() {
  int ldrValue = analogRead(LDR_PIN);
  int mq2Value = analogRead(MQ2_PIN);

  Serial.printf("LDR Value: %d\n", ldrValue);
  Serial.printf("MQ2 Value: %d\n", mq2Value);

  // Compare with thresholds
  checkAndTriggerActuator("Light", ldrValue, lightMin, lightMax);
 
  // Check and trigger for air quality
  checkAndTriggerActuator("Air Quality", mq2Value, airQualityMin, airQualityMax);

  delay(5000);  // Wait 5 seconds before next reading
}
