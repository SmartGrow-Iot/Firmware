#ifndef ACTUATORMODULE_H
#define ACTUATORMODULE_H

#include <Adafruit_MQTT.h>
#include <Adafruit_MQTT_Client.h>

class ActuatorModule {
private:
    uint8_t lightPin;
    uint8_t fanPin;
    Adafruit_MQTT_Publish* publishFeed;
    Adafruit_MQTT_Publish* feedbackFeed;
    Adafruit_MQTT_Subscribe* subscribeFeed;

public:
    ActuatorModule(uint8_t light, uint8_t fan,
                   Adafruit_MQTT_Publish* publish,
                   Adafruit_MQTT_Publish* feedback,
                   Adafruit_MQTT_Subscribe* subscribe);

    void begin();
    void callback(Adafruit_MQTT_Subscribe* subscription);
    void sendFeedback(const String &action, const String &triggeredBy, const String &source, const String &zone, bool success);
    void setFan();
    void setLight();
};

#endif