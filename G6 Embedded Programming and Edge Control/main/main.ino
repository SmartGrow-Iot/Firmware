#include <WiFi.h>
#include "SensorModule.h"

// Configuration
const char *SSID = "Cynex@2.4GHz";
const char *PASSWORD = "cyber@cynex";
const String SERVER_URL = "https://test-server-owq2.onrender.com";
const String USER_ID = "5ZyZcYb6wpdlObgeJiaGZ0ydMbW2";

// GPIO Definitions
const uint8_t SOIL_PINS[] = {34, 35, 36, 39};
const char *PLANT_IDS[] = {
    "plant_JU4Rj78DEHUM2lNYHzd3",
    "plant_CyhF06FW5a1KTmCvM0zf",
    "plant_hG7bPH7Np9WXtDe1zBVE",
    "plant_iKjnBJcBaGTx6LyCXUy2"};

// Module Setup
SensorModule sensor(DHT_PIN, DHT_TYPE, (uint8_t *)SOIL_PINS, 4);

void connectToWiFi()
{
  Serial.print("Connecting to WiFi");
  WiFi.begin(SSID, PASSWORD);
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(500);
  }
  Serial.println("\nWiFi connected!");
}

void setup()
{
  Serial.begin(115200);
  connectToWiFi();
  sensor.begin();

  for (int i = 0; i < 4; ++i)
  {
    sensor.addPlant(i, SOIL_PINS[i], PLANT_IDS[i]);
  }
}

void loop()
{
  sensor.sendAllToCloud(SERVER_URL, USER_ID);
  delay(60000); // 60s interval
}

