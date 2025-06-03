#include <WiFi.h>
#include <DHT.h>

// Macro Definitions (UPPER_SNAKE_CASE)
#define DHT_PIN       4       // GPIO4 connected to DHT11 data pin
#define DHT_TYPE      DHT11
#define SOIL_PIN      32      // GPIO32 connected to soil moisture sensor

// Global Constants
const char* WIFI_SSID = "Cynex@2.4GHz";
const char* WIFI_PASSWORD = "cyber@cynex";

// Global Objects
DHT dht(DHT_PIN, DHT_TYPE);
WiFiServer wifiServer(80);

/**
 * @brief Initializes Serial, DHT sensor, and Wi-Fi.
 */
void setup()
{
  Serial.begin(9600);
  dht.begin();

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  wifiServer.begin();
}

/**
 * @brief Reads and prints sensor data to Serial Monitor.
 */
void loop()
{
  // Read temperature and humidity from DHT11
  float temperatureC = dht.readTemperature();
  float humidityPercent = dht.readHumidity();

  // Read soil moisture analog value
  int soilMoistureValue = analogRead(SOIL_PIN);

  // Display sensor readings on Serial Monitor
  Serial.println("---- Sensor Data ----");
  Serial.print("Temperature: ");
  Serial.print(temperatureC);
  Serial.println(" °C");

  Serial.print("Humidity: ");
  Serial.print(humidityPercent);
  Serial.println(" %");

  Serial.print("Soil Moisture: ");
  Serial.println(soilMoistureValue);
  Serial.println("---------------------");

  delay(1000);  // Wait 1 second before next reading
}