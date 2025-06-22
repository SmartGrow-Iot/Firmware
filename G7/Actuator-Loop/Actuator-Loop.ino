#include <Adafruit_MQTT.h>
#include <Adafruit_MQTT_Client.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include "ActuatorModule.h"
#include "MqttModule.h"

// WiFi credentials
#define WIFI_SSID "Cynex@2.4GHz"
#define WIFI_PASSWORD "cyber@cynex"

// Adafruit IO MQTT server info
#define MQTT_SERVER "io.adafruit.com"
#define MQTT_PORT 1883
#define MQTT_USERNAME "SmartGrow"
#define MQTT_KEYS ""

// GPIO pin definitions for actuators
#define LED_PIN 5
#define FAN_PIN 4

// WiFi and MQTT client setup
WiFiClient client;
Adafruit_MQTT_Client mqtt(&client, MQTT_SERVER, MQTT_PORT, MQTT_USERNAME, MQTT_KEYS);

// MQTT Publish and Subscribe feeds
Adafruit_MQTT_Publish publishFeed = Adafruit_MQTT_Publish(&mqtt, MQTT_USERNAME "/feeds/group-1.actuator-status");
Adafruit_MQTT_Publish feedbackFeed = Adafruit_MQTT_Publish(&mqtt, MQTT_USERNAME "/feeds/group-1.actuator-feedback");
Adafruit_MQTT_Subscribe subscribeFeed = Adafruit_MQTT_Subscribe(&mqtt, MQTT_USERNAME "/feeds/group-1.actuator-status");

// Actuator module with LED and Fan pins, and MQTT feed references
ActuatorModule actuator(LED_PIN, FAN_PIN, &publishFeed, &feedbackFeed, &subscribeFeed);

// Connect to WiFi with retry logic
void connectToWifi() {
  Serial.print("Connecting to WiFi");
  int8_t retries = 0;
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED && retries++ < 10) {
    delay(1000);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nSuccessfully connected to WiFi");
  } else {
    Serial.println("\nWiFi connection failed");
  }
}

void setup() {
  Serial.begin(115200);         // Start serial communication
  connectToWifi();              // Connect to the WiFi
  mqtt.subscribe(&subscribeFeed); // Subscribe to the actuator-status feed
  actuator.begin();             // Initialize actuator pins
}

void loop() {
  
  MqttModule::connectToMqtt(mqtt); // Connect to Adafruit IO MQTT

  // Listen for MQTT messages
  Adafruit_MQTT_Subscribe *subscription;
  while ((subscription = mqtt.readSubscription(500))) {
    actuator.callback(subscription); // Handle MQTT message
  }

  // Regularly update actuator states (if they are automatic or sensor-based)
  actuator.setFan();
  actuator.setLight();

  delay(10000); // Delay between each loop iteration
}