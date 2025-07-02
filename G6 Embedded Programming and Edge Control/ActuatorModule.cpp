#include "ActuatorModule.h"

ActuatorModule::ActuatorModule(
    int pump,
    int fan1,
    int fan2,
    int light,
    Adafruit_MQTT_Publish *publish,
    Adafruit_MQTT_Publish *feedback,
    Adafruit_MQTT_Subscribe *subscribe)
{
  pumpPin = pump;
  fanPin1 = fan1;
  fanPin2 = fan2;
  lightPin = light;
  publishFeed = publish;
  feedbackFeed = feedback;
  subscribeFeed = subscribe;
}

void ActuatorModule::begin()
{
  Serial.println("Initializing actuators...");
  pinMode(pumpPin, OUTPUT);
  pinMode(fanPin1, OUTPUT);
  pinMode(fanPin2, OUTPUT);
  pinMode(lightPin, OUTPUT);
  Serial.println("Actuators initialized.");
}

String ActuatorModule::getISO8601Time()
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

void ActuatorModule::sendFeedback(const String &action, const String &triggeredBy, const String &source, const String &zone, bool success)
{
  StaticJsonDocument<256> doc;

  String timestamp = getISO8601Time();

  doc["action"] = action;
  doc["triggeredBy"] = triggeredBy;
  doc["source"] = source;
  doc["zone"] = zone;
  doc["timestamp"] = timestamp;
  doc["result"] = success ? "success" : "fail";

  char payload[256];
  serializeJson(doc, payload);

  if (feedbackFeed)
  {
    if (!feedbackFeed->publish(payload))
    {
      Serial.println("Failed to publish actuator feedback");
    }
    else
    {
      Serial.println("Actuator feedback published");
    }
  }
}

void ActuatorModule::callback(Adafruit_MQTT_Subscribe *subscription)
{
  Serial.println("ActuatorModule callback triggered");
  // Ensure the message came from the correct topic
  if (subscribeFeed && strcmp(subscription->topic, subscribeFeed->topic) == 0)
  {
    const char *command = (char *)subscribeFeed->lastread;

    // Parse JSON payload
    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, command);

    if (error)
    {
      Serial.print("JSON parse failed: ");
      Serial.println(error.c_str());
      return;
    }

    // Read JSON fields
    const char *light = doc["light"];
    const char *fan = doc["fan"];
    const char *pump = doc["pump"];

    Serial.print("Light: ");
    Serial.println(light);
    Serial.print("Fan: ");
    Serial.println(fan);
    Serial.print("Pump: ");
    Serial.println(pump);

    if (light != nullptr)
    {
      setLight(strcmp(light, "ON") == 0, false);
      delay(5000);
    }

    if (fan != nullptr)
    {
      setFan(strcmp(fan, "ON") == 0, false);
      delay(5000);
    }

    if (pump != nullptr)
    {
      setPump(strcmp(pump, "ON") == 0, false);
      delay(5000);
    }
  }
}

void ActuatorModule::setPump(bool state, bool system)
{
  const bool currentState = digitalRead(pumpPin) == HIGH ? true : false;
  if( currentState == state ) {
    Serial.println("Pump is already in the desired state. No action taken.");
    return;
  }
  digitalWrite(pumpPin, state ? HIGH : LOW);
  sendFeedback(
      String("pump ") + (state ? "ON" : "OFF"),
      system ? "SYSTEM" : "USER",
      system ? "auto" : "manual",
      "zone1",
      true);
}

void ActuatorModule::setFan(bool state, bool system)
{
  const bool currentState = digitalRead(fanPin1) == HIGH ? true : false;
  const bool currentState2 = digitalRead(fanPin2) == HIGH ? true : false;
  if( currentState == state && currentState2 == state ) 
  {
    Serial.println("Fan is already in the desired state. No action taken.");
    return;
  }
  digitalWrite(fanPin1, state ? HIGH : LOW);
  digitalWrite(fanPin2, state ? HIGH : LOW);
  sendFeedback(
      String("fan ") + (state ? "ON" : "OFF"),
      system ? "SYSTEM" : "USER",
      system ? "auto" : "manual",
      "zone1",
      true);
}

void ActuatorModule::setLight(bool state, bool system)
{
  const bool currentState = digitalRead(lightPin) == HIGH ? true : false;
  if( currentState == state ) 
  {
    Serial.println("Light is already in the desired state. No action taken.");
    return;
  }
  digitalWrite(lightPin, state ? HIGH : LOW);
  sendFeedback(
      String("light ") + (state ? "ON" : "OFF"),
      system ? "SYSTEM" : "USER",
      system ? "auto" : "manual",
      "zone1",
      true);
}
