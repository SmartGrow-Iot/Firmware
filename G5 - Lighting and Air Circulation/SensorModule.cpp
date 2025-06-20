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
  if (!getLocalTime(&timeinfo))
  {
    return "1970-01-01T00:00:00Z";
  }
  char buffer[30];
  strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", &timeinfo);
  Serial.println("Timestamp: " + String(buffer));
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