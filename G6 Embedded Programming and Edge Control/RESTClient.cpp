#include "RESTClient.h"

RESTClient::RESTClient(const String &serverUrl, bool insecure)
{
    this->serverUrl = serverUrl;
    this->useInsecure = insecure;
}

std::vector<PlantData> RESTClient::getPlantsByZone(const String &zoneId)
{
    std::vector<PlantData> plantList;
    String endpoint = serverUrl + "/api/v1/zones/" + zoneId + "/plants";

    WiFiClientSecure client;
    if (useInsecure)
    {
        client.setInsecure();
    }

    HTTPClient http;
    http.begin(client, endpoint);

    int httpResponseCode = http.GET();
    if (httpResponseCode > 0)
    {
        String response = http.getString();

        StaticJsonDocument<8192> doc;
        DeserializationError error = deserializeJson(doc, response);
        if (error)
        {
            Serial.print("JSON parse error: ");
            Serial.println(error.c_str());
            http.end();
            return plantList;
        }

        JsonArray plants = doc["plants"].as<JsonArray>();
        for (JsonObject plant : plants)
        {
            PlantData data;
            data.plantId = plant["plantId"].as<String>();
            data.moisturePin = plant["moisturePin"].as<int>();

            data.min_moisture = plant["thresholds"]["moisture"]["min"].as<float>();
            data.max_moisture = plant["thresholds"]["moisture"]["max"].as<float>();

            data.min_temperature = plant["thresholds"]["temperature"]["min"].as<float>();
            data.max_temperature = plant["thresholds"]["temperature"]["max"].as<float>();

            data.min_light = plant["thresholds"]["light"]["min"].as<float>();
            data.max_light = plant["thresholds"]["light"]["max"].as<float>();

            data.min_airQuality = plant["thresholds"]["airQuality"]["min"].as<float>();
            data.max_airQuality = plant["thresholds"]["airQuality"]["max"].as<float>();

            plantList.push_back(data);
        }
    }
    else
    {
        Serial.print("HTTP error code: ");
        Serial.println(httpResponseCode);
    }

    http.end();
    return plantList;
}

bool RESTClient::sendZoneSensorData(
    const String &zoneId,
    float temperature,
    float humidity,
    float light,
    float airQuality,
    const std::vector<std::pair<int, float>> &soilMoistureByPin,
    const String &userId,
    const String &timestamp)
{
    String endpoint = serverUrl + "/api/v1/sensor-data";

    WiFiClientSecure client;
    if (useInsecure)
    {
        client.setInsecure();
    }

    HTTPClient http;
    http.begin(client, endpoint);
    http.addHeader("Content-Type", "application/json");

    StaticJsonDocument<1024> doc;

    doc["zoneId"] = zoneId;

    JsonObject zoneSensors = doc.createNestedObject("zoneSensors");
    zoneSensors["humidity"] = isnan(humidity) ? 0.0f : humidity;
    zoneSensors["temp"] = isnan(temperature) ? 0.0f : temperature;
    zoneSensors["light"] = isnan(light) ? 0.0f : light;
    ;
    zoneSensors["airQuality"] = isnan(airQuality) ? 0.0f : airQuality;

    JsonArray soilArray = doc.createNestedArray("soilMoistureByPin");
    for (const auto &pair : soilMoistureByPin)
    {
        JsonObject entry = soilArray.createNestedObject();
        entry["pin"] = pair.first;

        // Convert raw ADC to percentage (100% = very wet, 0% = very dry)
        int rawValue = pair.second;
        float moisturePercent = map(rawValue, 3900, 1200, 0, 100);
        moisturePercent = constrain(moisturePercent, 0, 100);

        entry["soilMoisture"] = moisturePercent;
    }

    if (userId != "")
        doc["userId"] = userId;
    if (timestamp != "")
        doc["timestamp"] = timestamp;

    String requestBody;
    serializeJson(doc, requestBody);

    int httpResponseCode = http.POST(requestBody);

    if (httpResponseCode > 0)
    {
        Serial.print("Zone sensor data sent");
        Serial.println(httpResponseCode);
        http.end();
        return true;
    }
    else
    {
        Serial.print("Failed to send zone sensor data. Code: ");
        Serial.println(httpResponseCode);
        http.end();
        return false;
    }
}

bool RESTClient::sendActuatorLog(
    const String &action_name,
    const String &action,
    const String &actuatorId,
    const String &plantId,
    const String &trigger,
    const String &zone,
    const String &triggerBy,
    const String &timestamp)
{
    String endpoint = serverUrl + "/api/v1/logs/action/" + action_name;

    WiFiClientSecure client;
    if (useInsecure)
    {
        client.setInsecure();
    }

    HTTPClient http;
    http.begin(client, endpoint);
    http.addHeader("Content-Type", "application/json");

    // Create the JSON payload
    StaticJsonDocument<512> doc;

    doc["action"] = action;
    doc["actuatorId"] = actuatorId;
    doc["plantId"] = plantId;
    doc["trigger"] = trigger;
    doc["zone"] = zone;

    if (triggerBy != "")
    {
        doc["triggerBy"] = triggerBy;
    }

    if (timestamp != "")
    {
        doc["timestamp"] = timestamp;
    }

    String requestBody;
    serializeJson(doc, requestBody);

    // Optional: Print the request payload for debugging
    Serial.print("Sending Actuator Log: ");
    Serial.println(requestBody);

    int httpResponseCode = http.POST(requestBody);

    if (httpResponseCode > 0)
    {
        String response = http.getString();
        Serial.print("Log action sent successfully, response: ");
        Serial.println(response);
        http.end();
        return true;
    }
    else
    {
        Serial.print("POST failed, error: ");
        Serial.println(httpResponseCode);
        http.end();
        return false;
    }
}
