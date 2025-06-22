#include "ActuatorModule.h"
#include "SensorModule.h"
#include <ArduinoJson.h>

// Constructor: initializes the pins and MQTT references
ActuatorModule::ActuatorModule(uint8_t light, uint8_t fan1,
                               Adafruit_MQTT_Publish* publish,
                               Adafruit_MQTT_Publish* feedback,
                               Adafruit_MQTT_Subscribe* subscribe)
    : lightPin(light), fanPin(fan1),
      publishFeed(publish), feedbackFeed(feedback), subscribeFeed(subscribe) {}

void ActuatorModule::begin() {
    Serial.println("Initializing actuators...");
    pinMode(lightPin, OUTPUT);
    pinMode(fanPin, OUTPUT);
    Serial.println("Actuators initialized.");
}

void ActuatorModule::sendFeedback(const String &action, const String &triggeredBy, const String &source, const String &zone, bool success) {
    StaticJsonDocument<256> doc;

    doc["action"] = action;
    doc["triggeredBy"] = triggeredBy;
    doc["source"] = source;
    doc["zone"] = zone;
    doc["timestamp"] = SensorModule::getISO8601Time(); // uses SensorModule's time
    doc["result"] = success ? "success" : "fail";

    char payload[256];
    serializeJson(doc, payload);

    if (feedbackFeed) {
        if (!feedbackFeed->publish(payload)) {
            Serial.println("Failed to publish actuator feedback");
        } else {
            Serial.println("Actuator feedback published");
        }
    }
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

        //feedback light
        sendFeedback(
            String("light ") + (strcmp(light, "ON") == 0 ? "ON" : "OFF"),
            "user",
            "manual",
            "zone1",
            true
        )
        //feedback fan
        sendFeedback(
            String("fan ") + (strcmp(fan, "ON") == 0 ? "ON" : "OFF"),
            "user",
            "manual",
            "zone1",
            true
        )
    }
}

void ActuatorModule::setFan() {
    float temperature = SensorModule::readTemperature();
    digitalWrite(fanPin, temperature > 28 ? HIGH : LOW); // LOW = ON (depends on wiring)
    //feedback fan
    sendFeedback(
        String("fan ") + (strcmp(fan, "ON") == 0 ? "ON" : "OFF"),
        "user",
        "manual",
        "zone1",
        true
    )
}

void ActuatorModule::setLight() {
    float lightLevel = SensorModule::readLightLevel();
    digitalWrite(lightPin, lightLevel > 2500 ? HIGH : LOW); // Adjust threshold as needed
    //feedback light
    sendFeedback(
        String("light ") + (strcmp(light, "ON") == 0 ? "ON" : "OFF"),
        "user",
        "manual",
        "zone1",
        true
    )
}