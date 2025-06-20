#ifndef SENSOR_MODULE_H
#define SENSOR_MODULE_H

#include <WiFi.h>
#include <HTTPClient.h>
#include <DHT.h>
#include <Arduino.h>
#include <time.h>

// Sensor Pin Configuration
#define DHT_PIN 16
#define DHT_TYPE DHT11
#define MQ2_PIN 32
#define LDR_PIN 33

class SensorModule
{
public:
    SensorModule(uint8_t dhtPin, uint8_t dhtType, uint8_t *soilPins, int numPlants);
    void begin();
    void addPlant(int plantIndex, int soilPin, const String &plantId);
    void sendAllToCloud(const String &serverURL, const String &userId);

private:
    struct Plant
    {
        int soilPin;
        String plantId;
    };

    static const int MAX_PLANTS = 4;
    Plant plants[MAX_PLANTS];

    DHT dht;
    uint8_t *_soilPins;
    int _numPlants;

    float readSoilMoisture(int pin);
    float readTemperature();
    float readHumidity();
    float readAirQuality();
    float readLightLevel();
    String getISO8601Time();
};

#endif