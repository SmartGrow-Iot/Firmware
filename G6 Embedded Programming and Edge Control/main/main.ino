#include <WiFi.h>
#include "SensorModule.h"
#include "ActuatorModule.h"
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Adafruit_MQTT.h>
#include <Adafruit_MQTT_Client.h>
#include "MqttModule.h"
#include "RESTClient.h"
#include "secrets.h"

// Zone ID
const String zoneId = "zone1";

// WIFI Configuration
const char* SSID = "Cynex@2.4GHz";
const char* PASSWORD = "cyber@cynex";
const String SERVER_URL = "https://test-server-owq2.onrender.com";
const String USER_ID = "5ZyZcYb6wpdlObgeJiaGZ0ydMbW2";

// Adafruit IO MQTT server info
#define MQTT_SERVER "io.adafruit.com"
#define MQTT_PORT 1883
#define MQTT_USERNAME "SmartGrow"
#define MQTT_KEYS ""

// Actuator PIN
const int PUMP_PIN = 25;
const int FAN_PIN_1 = 27;
const int FAN_PIN_2 = 18;
const int LIGHT_PIN = 26;
const int LED_PIN = 23;

float min_moisture;
float min_temperature;
float min_light;
float min_airQuality;
float max_moisture;
float max_temperature;
float max_light;
float max_airQuality;

// WiFi & MQTT Clients
WiFiClient wifiClient;
Adafruit_MQTT_Client mqtt(&wifiClient, MQTT_SERVER, MQTT_PORT, MQTT_USERNAME, MQTT_KEYS);

// MQTT Publish and Subscribe feeds
Adafruit_MQTT_Publish publishFeed = Adafruit_MQTT_Publish(&mqtt, MQTT_USERNAME "/feeds/group-1.actuator-status");
Adafruit_MQTT_Publish feedbackFeed = Adafruit_MQTT_Publish(&mqtt, MQTT_USERNAME "/feeds/group-1.actuator-feedback");
Adafruit_MQTT_Subscribe subscribeFeed = Adafruit_MQTT_Subscribe(&mqtt, MQTT_USERNAME "/feeds/group-1.actuator-status");

// Initialize RESTClient
RESTClient restClient(SERVER_URL, true);

// Declare pointer to SensorModule
SensorModule* sensor;  

// Actuator Setup ActuatorModule(PUMP_PIN,FAN_PIN,LIGHT_PIN)
ActuatorModule actuator(PUMP_PIN, FAN_PIN_1, FAN_PIN_2, LIGHT_PIN, &publishFeed, &feedbackFeed, &subscribeFeed);
std::vector<PlantData> plants;

void connectToWiFi() 
{
  Serial.print("Connecting to WiFi");
  WiFi.begin(SSID, PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("\nWiFi connected!");
}

// edge control
void evaluateSensorsAndTrigger() 
{
  int lightValue = sensor->readLightLevel();
  int airQualityValue = sensor->readAirQuality();
  int tempValue = sensor->readTemperature();

  bool lightBelow = sensor->checkAndTrigger("Light", lightValue, max_light);
  bool airQualityBelow = sensor->checkAndTrigger("Air Quality", airQualityValue, max_airQuality);
  bool tempBelow = sensor->checkAndTrigger("Temperature", tempValue, max_temperature);

  if (lightBelow) 
  {
    Serial.println("Light out of range! Activate actuator.");
    actuator.setLight(true, true);
  } else 
  {
    Serial.println("Light normal. Turning off grow light.");
    actuator.setLight(false, true);
  }

  if (airQualityBelow) 
  {
    Serial.println("Air quality bad! Activate actuator.");
    actuator.setFan(true, true);
  } else 
  {
    Serial.println("Air quality normal! Turning off actuator.");
    actuator.setFan(false, true);
  }

  if (!tempBelow) 
  {
    Serial.println("Temperature too high! Activate fan.");
    actuator.setFan(true, true);
  } else 
  {
    actuator.setFan(false, true);
    Serial.println("Temperature normal. Turning off fan.");
  }
}

void setup() 
{
  pinMode(LED_PIN, OUTPUT);
  Serial.begin(115200);
  connectToWiFi();
  mqtt.subscribe(&subscribeFeed);
  actuator.begin();

  plants = restClient.getPlantsByZone(zoneId);

  std::vector<uint8_t> soilPins;
  for (const auto& plant : plants) 
  {
    soilPins.push_back(plant.moisturePin);
  }

  // Create sensor module with the dynamically built pins
  sensor = new SensorModule(DHT_PIN, DHT_TYPE, soilPins.data(), soilPins.size());
  sensor->begin();
  for (size_t i = 0; i < plants.size(); ++i) 
  {
    sensor->addPlant(i, plants[i].moisturePin, plants[i].plantId);
  }

  if(plants.size() > 0)
  {
    max_moisture = plants[0].max_moisture;
    max_temperature = plants[0].max_temperature;
    max_light = plants[0].max_light;
    max_airQuality = plants[0].max_airQuality;
    for (const auto& p : plants) {
      Serial.println("Plant ID: " + p.plantId);
      Serial.print("Moisture pin: ");
      Serial.println(p.moisturePin);
      Serial.print("Moisture Threshold: ");
      Serial.print(p.min_moisture);
      Serial.print(" - ");
      Serial.println(p.max_moisture);
      Serial.print("Temperature Threshold: ");
      Serial.print(p.min_temperature);
      Serial.print(" - ");
      Serial.println(p.max_temperature);
      Serial.print("Light Threshold: ");
      Serial.print(p.min_light);
      Serial.print(" - ");
      Serial.println(p.max_light);
      Serial.print("Air Quality Threshold: ");
      Serial.print(p.min_airQuality);
      Serial.print(" - ");
      Serial.println(p.max_airQuality);
      Serial.println();
    }
  }

  for(int i = 0; i < 3 ; i++)
  {
    digitalWrite(LED_PIN, HIGH);
    delay(500);
    digitalWrite(LED_PIN, LOW);
    delay(500);
  }
}

void loop() {
  for(int i = 0; i < 3 ; i++)
  {
    digitalWrite(LED_PIN, HIGH);
    delay(500);
    digitalWrite(LED_PIN, LOW);
    delay(500);
  }
  if (!mqtt.connected()) 
  {
    MqttModule::connectToMqtt(mqtt);
  }

  // Check for any incoming messages
  Adafruit_MQTT_Subscribe* subscription;
  while ((subscription = mqtt.readSubscription(5000))) 
  {
    if (subscription == &subscribeFeed) {
      String jsonStr = (char*)subscribeFeed.lastread;
      Serial.println("Received JSON: " + jsonStr);
      actuator.callback(subscription);  // Handle MQTT message
    }
  }

  // === 🌱 Build soilMoistureByPin vector ===
  std::vector<std::pair<int, float>> soilMoistureByPin;
  for (int i = 0; i < plants.size(); ++i) 
  {
    float moisture = sensor->readSoilMoisture(sensor->plants[i].soilPin);
    soilMoistureByPin.push_back(std::make_pair(sensor->plants[i].soilPin, moisture));
  }

  // === 🕒 Get timestamp ===
  String timestamp = sensor->getISO8601Time();

  // === ☁️ Send all sensor data to cloud ===
  restClient.sendZoneSensorData(
    zoneId,
    sensor->readTemperature(),
    sensor->readHumidity(),
    sensor->readLightLevel(),
    sensor->readAirQuality(),
    soilMoistureByPin,
    USER_ID,
    timestamp);

  evaluateSensorsAndTrigger();

  if (digitalRead(PUMP_PIN) == HIGH) 
  {
    Serial.println("[CHECK] Pump is currently ON MANUALLY. Waiting 5 seconds...");

    // Now check soil condition again
    if (sensor->shouldWater(plants)) 
    {
      Serial.println("[CHECK] Still needs water. Keeping pump ON.");
    } else 
    {
      Serial.println("[CHECK] Moisture OK now. Turning pump OFF.");
      actuator.setPump(false, true);
    }
  } else if (sensor->shouldWater(plants)) 
  {
    Serial.println("[PUMP] Watering needed → ON");
    actuator.setPump(true, true);
    // Keep checking every 5 seconds (adjust if needed)
    int i = 0;
    while (sensor->shouldWater(plants) && i < 3) 
    {
      Serial.println("[PUMP] Still dry... continuing watering");
      delay(5000);  // Wait 5s before rechecking moisture
      i = i + 1;
    }

    // Moisture OK now — stop pump
    Serial.println("[PUMP] Moisture OK → STOP WATERING");
    actuator.setPump(false, true);
  } else 
  {
    Serial.println("[PUMP] Moisture OK → OFF");
    actuator.setPump(false, true);
  }

  delay(30000);  // Wait 30s before next loop
}