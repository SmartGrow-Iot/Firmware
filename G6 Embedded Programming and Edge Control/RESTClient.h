#ifndef RESTCLIENT_H
#define RESTCLIENT_H

#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <vector>
#include <utility> // for std::pair

struct PlantData 
{
    String plantId;
    int moisturePin;
    float min_moisture;
    float min_temperature;
    float min_light;
    float min_airQuality;
    float max_moisture;
    float max_temperature;
    float max_light;
    float max_airQuality;
};

class RESTClient 
{
public:
    RESTClient(const String &serverUrl, bool insecure = true);

    // Returns a vector of <plantId, soilPin> pairs from a zone
    std::vector<PlantData> getPlantsByZone(const String &zoneId);

    bool sendZoneSensorData(
        const String &zoneId,
        float temperature,
        float humidity,
        float light,
        float airQuality,
        const std::vector<std::pair<int, float>> &soilMoistureByPin,
        const String &userId = "",
        const String &timestamp = ""
    );

    // POST: Actuator log
    bool sendActuatorLog(
        const String &action_name,
        const String &action,
        const String &actuatorId,
        const String &plantId,
        const String &trigger,
        const String &zone,
        const String &triggerBy = "SYSTEM",
        const String &timestamp = ""
    );

private:
    String serverUrl;
    bool useInsecure;
};

#endif
