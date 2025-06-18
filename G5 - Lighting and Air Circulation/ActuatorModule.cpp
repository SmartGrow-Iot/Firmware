#include "ActuatorModule.h"
#include "SensorModule.h"
#include <ArduinoJson.h>

// Constructor: initializes the pins and MQTT references
ActuatorModule::ActuatorModule(uint8_t light, uint8_t fan1,
                               Adafruit_MQTT_Publish* publish,
                               Adafruit_MQTT_Subscribe* subscribe)
    : lightPin(light), fanPin(fan1),
      publishFeed(publish), subscribeFeed(subscribe) {}

void ActuatorModule::begin() {
    Serial.println("Initializing actuators...");
    pinMode(lightPin, OUTPUT);
    pinMode(fanPin, OUTPUT);
    Serial.println("Actuators initialized.");
}

void ActuatorModule::callback(Adafruit_MQTT_Subscribe* subscription) {
    // Ensure the message came from the correct topic
    if (subscribeFeed && strcmp(subscription->topic, subscribeFeed->topic) == 0) {
        const char* command = (char*)subscribeFeed->lastread;

        // Parse JSON payload
        StaticJsonDocument<200> doc;
        DeserializationError error = deserializeJson(doc, command);

        if (error) {
            Serial.print("JSON parse failed: ");
            Serial.println(error.c_str());
            return;
        }

        // Read JSON fields
        const char* light = doc["light"];
        const char* fan = doc["fan"];

        Serial.print("Light: ");
        Serial.println(light);
        Serial.print("Fan: ");
        Serial.println(fan);

        // Control actuators based on command values
        digitalWrite(lightPin, strcmp(light, "ON") == 0 ? HIGH : LOW);
        digitalWrite(fanPin, strcmp(fan, "ON") == 0 ? HIGH : LOW);
    }
}

void ActuatorModule::setFan() {
    float temperature = SensorModule::readTemperature();
    digitalWrite(fanPin, temperature > 28 ? HIGH : LOW); // LOW = ON (depends on wiring)
}

void ActuatorModule::setLight() {
    float lightLevel = SensorModule::readLightLevel();
    digitalWrite(lightPin, lightLevel > 2500 ? HIGH : LOW); // Adjust threshold as needed
}
