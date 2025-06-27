#include "MqttModule.h"

void MqttModule::connectToMqtt(Adafruit_MQTT_Client& mqtt) 
{
  Serial.print("Connecting to MQTT...");

  int8_t connection;
  int8_t retries = 0;

  // Attempt to connect up to 5 times
  while ((connection = mqtt.connect()) != 0) 
  {
    Serial.print("Error: ");
    Serial.println(mqtt.connectErrorString(connection));

    retries++;
    if (retries >= 5) 
    {
      Serial.println("Failed to connect after 5 attempts. Restarting...");

      // Blocking alternative for platforms that don't support restart
      while (true); 
    }

    delay(2000); // Wait before retry
  }

  Serial.println("Connected to MQTT!");
}