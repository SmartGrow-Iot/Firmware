#include "SensorModule.h"

SensorModule::SensorModule(uint8_t dhtPin, uint8_t dhtType, uint8_t *soilPins, int numPlants)
    : dht(dhtPin, dhtType), _soilPins(soilPins), _numPlants(numPlants) {}

void SensorModule::begin()
{
  dht.begin();

  if (WiFi.status() == WL_CONNECTED)
  {
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");
    Serial.println("Waiting for NTP time sync...");

    struct tm timeinfo;
    int retries = 0;
    while (!getLocalTime(&timeinfo) && retries++ < 10)
    {
      Serial.print(".");
      delay(1000);
    }

    Serial.println(retries < 10 ? "\nTime synchronized!" : "\nNTP sync failed.");
  }
}

void SensorModule::addPlant(int plantIndex, int soilPin, const String &plantId)
{
  if (plantIndex < MAX_PLANTS)
  {
    plants[plantIndex] = {soilPin, plantId};
    pinMode(soilPin, INPUT);
  }
}

float SensorModule::readSoilMoisture(int pin)
{
  return analogRead(pin);
}

float SensorModule::readAirQuality()
{
  return analogRead(MQ2_PIN);
}

float SensorModule::readLightLevel()
{
  return analogRead(LDR_PIN);
}

float SensorModule::readTemperature()
{
  return dht.readTemperature();
}

float SensorModule::readHumidity()
{
  return dht.readHumidity();
}

String SensorModule::getISO8601Time()
{
  struct tm timeinfo;
  time_t now;

  time(&now);                   // Get current time as time_t
  now += 8 * 3600;              // Add 8 hours for UTC+8
  gmtime_r(&now, &timeinfo);    // Convert to UTC+8 time

  char buffer[30];
  strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", &timeinfo);

  Serial.println("Timestamp (UTC+8): " + String(buffer));
  return String(buffer);
}

void SensorModule::sendAllToCloud(const String &serverURL, const String &userId)
{
  float temperatureC = readTemperature();
  float humidityPercentage = readHumidity();
  float airQualityPpm = readAirQuality();
  float lightLevel = readLightLevel();

  for (int i = 0; i < MAX_PLANTS; i++)
  {
    float soilMoisture = readSoilMoisture(plants[i].soilPin);

    String timestamp = getISO8601Time();
    String json = "{";
    json += "\"automation\":{\"fanOn\":false,\"lightOn\":false,\"waterOn\":false},";
    json += "\"lastUpdated\":\"" + timestamp + "\",";
    json += "\"plantId\":\"" + plants[i].plantId + "\",";
    json += "\"profile\":{\"humidityMax\":100,\"humidityMin\":0,";
    json += "\"lightMax\":1000,\"lightMin\":0,";
    json += "\"moistureMax\":100,\"moistureMin\":0,";
    json += "\"tempMax\":50,\"tempMin\":0},";
    json += "\"sensorRecordId\":\"" + timestamp + "\",";
    json += "\"sensors\":{";
    json += "\"humidity\":" + String(humidityPercentage) + ",";
    json += "\"light\":" + String(lightLevel) + ",";
    json += "\"soilMoisture\":" + String(soilMoisture) + ",";
    json += "\"temp\":" + String(temperatureC) + ",";
    json += "\"airQuality\":" + String(airQualityPpm) + "},";
    json += "\"userId\":\"" + userId + "\"}";

    HTTPClient http;
    http.begin(serverURL + "/api/v1/sensor-data");
    http.addHeader("Content-Type", "application/json");

    int responseCode = http.POST(json);
    Serial.printf("\nSent to %s | Response: %d\n", plants[i].plantId.c_str(), responseCode);

    http.end();
    delay(500);
  }
}

bool SensorModule::fetchThresholdsFromAPI()
{
  const char *url = "https://test-server-owq2.onrender.com/api/v1/plants/plant_kF6h3AT8VaUm73MBkgRP";

  if (WiFi.status() == WL_CONNECTED)
  {
    HTTPClient http;
    http.begin(url);
    int httpResponseCode = http.GET();

    if (httpResponseCode == 200)
    {
      String payload = http.getString();

      StaticJsonDocument<1024> doc;
      DeserializationError error = deserializeJson(doc, payload);

      if (!error)
      {
        lightMin = doc["thresholds"]["light"]["min"];
        lightMax = doc["thresholds"]["light"]["max"];
        airQualityMin = doc["thresholds"]["airQuality"]["min"];
        airQualityMax = doc["thresholds"]["airQuality"]["max"];
        tempMin = doc["thresholds"]["temperature"]["min"];
        tempMax = doc["thresholds"]["temperature"]["max"];
        soilMin = doc["thresholds"]["moisture"]["min"];
        soilMax = doc["thresholds"]["moisture"]["max"];

        Serial.printf("Fetched Light Threshold Min: %.2f, Max: %.2f\n", lightMin, lightMax);
        Serial.printf("Fetched Air Quality Min: %.2f, Max: %.2f\n", airQualityMin, airQualityMax);

        http.end();
        return true;
      }
      else
      {
        Serial.println("Failed to parse JSON.");
      }
    }
    else
    {
      Serial.printf("HTTP GET failed, code: %d\n", httpResponseCode);
    }
    http.end();
  }
  return false;
}

bool SensorModule::checkAndTrigger(const String &sensorName, int sensorValue, float maxVal)
{
  bool trigger = (sensorValue <= maxVal);
  Serial.printf("%s Value: %d — Max: %.2f → %s\n",
                sensorName.c_str(), sensorValue, maxVal,
                trigger ? "ACTIVE" : "DEACTIVATED (Above Max)");
  return trigger;
}

bool SensorModule::shouldWater(const std::vector<PlantData> &plantList)
{
  bool needsWater = false;

  const int dryADC = 3900; // ADC value when dry
  const int wetADC = 1200; // ADC value when fully wet

  for (const auto &plant : plantList)
  {
    int pin = plant.moisturePin;
    float minThreshold = plant.min_moisture; // in %
    float maxThreshold = plant.max_moisture; // in %

    // Convert raw ADC value to moisture percentage
    int rawValue = analogRead(pin);
    float moisturePercent = map(rawValue, dryADC, wetADC, 0, 100);
    moisturePercent = constrain(moisturePercent, 0, 100);

    Serial.printf("[Moisture Check] Plant ID %s at Pin %d → Raw: %d, Converted: %.2f%% (Min: %.2f%%, Max: %.2f%%)\n",
                  plant.plantId.c_str(), pin, rawValue, moisturePercent, minThreshold, maxThreshold);

    if (moisturePercent > maxThreshold)
    {
      Serial.printf("[Too Wet] Plant ID %s is above max threshold (%.2f%%). Watering skipped.\n",
                    plant.plantId.c_str(), moisturePercent);
      return false; // If any plant is too wet, skip watering
    }

    if (moisturePercent < minThreshold)
    {
      needsWater = true; // Mark that watering is needed
    }
  }

  if (needsWater)
  {
    Serial.println("[Watering Triggered] At least one plant needs water, and none are overwatered.");
    return true;
  }

  Serial.println("[No Watering] All moisture levels are within acceptable range.");
  return false;
}
