#ifndef ACTUATORMODULE_H
#define ACTUATORMODULE_H

#include <Arduino.h>

#include <Adafruit_MQTT.h>
#include <Adafruit_MQTT_Client.h>
#include <ArduinoJson.h>
#include "SensorModule.h"

class ActuatorModule 
{
  private:
    int pumpPin;
    int fanPin1;
    int fanPin2;
    int lightPin;
    Adafruit_MQTT_Publish* publishFeed;
    Adafruit_MQTT_Publish* feedbackFeed;
    Adafruit_MQTT_Subscribe* subscribeFeed;

  public:
    ActuatorModule(
      int pump, 
      int fan1, 
      int fan2, 
      int light, 
      Adafruit_MQTT_Publish* publish, 
      Adafruit_MQTT_Publish* feedback,
      Adafruit_MQTT_Subscribe* subscribe
);
    void begin();
    void setPump(bool state, bool system = true);
    void setFan(bool state, bool system = true);
    void callback(Adafruit_MQTT_Subscribe* subscription);
    void sendFeedback(const String &action, const String &triggeredBy, const String &source, const String &zone, bool success);
    void setLight(bool state, bool system = true);
    String getISO8601Time();
};

#endif
