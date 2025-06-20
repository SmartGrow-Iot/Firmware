#ifndef MQTTMODULE_H
#define MQTTMODULE_H

#include <Adafruit_MQTT.h>
#include <Adafruit_MQTT_Client.h>
#include <ArduinoJson.h>
#include <Arduino.h>

class MqttModule {

  public:
  static void connectToMqtt(Adafruit_MQTT_Client& mqtt);
};

#endif