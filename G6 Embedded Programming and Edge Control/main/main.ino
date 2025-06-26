  #include <WiFi.h>
  #include "SensorModule.h"
  #include "ActuatorModule.h"
  #include <PubSubClient.h>
  #include <ArduinoJson.h>
  #include "secrets.h"

  // WIFI Configuration
  const char *SSID = "Cynex@2.4GHz";
  const char *PASSWORD = "cyber@cynex";
  const String SERVER_URL = "https://test-server-owq2.onrender.com";
  const String USER_ID = "5ZyZcYb6wpdlObgeJiaGZ0ydMbW2";

  // Adafruit IO MQTT config
  const char* MQTT_SERVER = "io.adafruit.com";
  const int MQTT_PORT = 1883;
  const char* MQTT_TOPIC = "SmartGrow/feeds/actuator-status";

  // GPIO Definitions
  const uint8_t SOIL_PINS[] = {34, 35, 36, 39};
  const char *PLANT_IDS[] = {
      "plant_JU4Rj78DEHUM2lNYHzd3",
      "plant_CyhF06FW5a1KTmCvM0zf",
      "plant_hG7bPH7Np9WXtDe1zBVE",
      "plant_iKjnBJcBaGTx6LyCXUy2"
    };

  // Actuator PIN
  const int PUMP_PIN = 25;
  const int FAN_PIN = 32;
  const int LIGHT_PIN = 26;

  //Testing pump
  unsigned long lastPumpToggle = 0;
  bool pumpState = false;


  // WiFi & MQTT Clients
  WiFiClient wifiClient;
  PubSubClient mqttClient(wifiClient);

  // Sensor Setup
  SensorModule sensor(DHT_PIN, DHT_TYPE, (uint8_t *)SOIL_PINS, 4);

  // Actuator Setup ActuatorModule(PUMP_PIN,FAN_PIN,LIGHT_PIN)
  ActuatorModule actuator(PUMP_PIN, FAN_PIN, LIGHT_PIN);

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

  void connectToMQTT() 
  {
    while (!mqttClient.connected()) 
    {
      Serial.println("Connecting to MQTT...");
      if (mqttClient.connect("ESP32Client123", AIO_USERNAME, AIO_KEY)) 
      {
        Serial.println("MQTT connected!");
        mqttClient.subscribe(MQTT_TOPIC);
      } else 
      {
        Serial.print(" failed, rc=");
        Serial.print(mqttClient.state());
        delay(2000);
      }
    }
  }

  // MQTT Callback Function 
  void mqttCallback(char* topic, byte* payload, unsigned int length) 
  { 
    Serial.println("Signal received");
    StaticJsonDocument<100> doc; 
    DeserializationError error = deserializeJson(doc, payload, length); 
    
    if (error) 
    {
      Serial.println("Error receiving signal");
      return;
    } 
  
    if (doc.containsKey("fan")) 
    { 
      String state = doc["fan"]; 
      digitalWrite(FAN_PIN, (state == "ON") ? HIGH : LOW); 
      actuator.setFan(state == "ON" ? true : false );
      if(state == "ON")
      {
          Serial.println("FAN ON!");
      }else
      {
          Serial.println("FAN OFF!");
      }
    } 
  
    if (doc.containsKey("pump")) 
    { 
      String state = doc["pump"]; 
      digitalWrite(PUMP_PIN, (state == "ON") ? HIGH : LOW); 
      actuator.setPump(state == "ON" ? true : false );

    } 
  
    if (doc.containsKey("light")) 
    { 
      String state = doc["light"]; 
      digitalWrite(LIGHT_PIN, (state == "ON") ? HIGH : LOW); 
      actuator.setLight(state == "ON" ? true : false );
      if(state == "ON")
      {
          Serial.println("Light ON!");
      }else
      {
          Serial.println("Light OFF!");
      }

    } 
  }

  void autoControlPump() 
  {
    HTTPClient http;
    http.begin("https://test-server-owq2.onrender.com/api/v1/plants/plant_jZkYQoPWp3XiylDU5jA6");
    int httpResponseCode = http.GET();

    if (httpResponseCode > 0) 
    {
      String payload = http.getString();
      StaticJsonDocument<2048> doc;
      DeserializationError error = deserializeJson(doc, payload);
      
      if (!error) 
      {
        float soilMin = doc["thresholds"]["moisture"]["min"];
        float soilMax = doc["thresholds"]["moisture"]["max"];

        int rawSoil = analogRead(SOIL_PINS[0]); 
        float soilPercent = map(rawSoil, 0, 4095, 100, 0);

        Serial.printf("Auto-Control Pump:\nSoil Moisture = %.1f%% (raw: %d)\n", soilPercent, rawSoil);
        Serial.printf("Thresholds → Min: %.1f%% | Max: %.1f%%\n", soilMin, soilMax);

        if (soilPercent < soilMin) 
        {
          Serial.println("Soil too dry → Pump ON");
          digitalWrite(PUMP_PIN, HIGH);
          actuator.setPump(true);
        } else 
        {
          Serial.println("Soil OK or wet → Pump OFF");
          digitalWrite(PUMP_PIN, LOW);
          actuator.setPump(false);
        }
      } else 
      {
        Serial.print("JSON Parse Error: ");
        Serial.println(error.c_str());
      }
    } else 
    {
      Serial.printf("HTTP GET failed with error code: %d\n", httpResponseCode);
    }

    http.end();
  }

  void setup()
  {
    Serial.begin(115200);
    // connectToWiFi();
    sensor.begin();
    actuator.begin();

    //Testing pump
    pinMode(PUMP_PIN, OUTPUT);
    digitalWrite(PUMP_PIN, LOW); 

    mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
    mqttClient.setCallback(mqttCallback);

    for (int i = 0; i < 4; ++i)
    {
      sensor.addPlant(i, SOIL_PINS[i], PLANT_IDS[i]);
    }
    connectToMQTT();

  }

  void loop()
  {
    Serial.println("Loop started...");
    sensor.sendAllToCloud(SERVER_URL, USER_ID);

    // For pump testing purpose with rate-limiting
    unsigned long currentTime = millis();
    if (currentTime - lastAutoControlTime >= 10000) { // 10-second interval
      autoControlPump();
      lastAutoControlTime = currentTime;
    }

    if (currentTime - lastPumpToggle >= 10000) { 
      pumpState = !pumpState; 
      digitalWrite(PUMP_PIN, pumpState ? HIGH : LOW);

      Serial.println(pumpState ? "Pump ON" : "Pump OFF");

      lastPumpToggle = currentTime;
    }

    if (!mqttClient.connected()) {
      connectToMQTT();
    }
    mqttClient.loop();  // check for MQTT messages
  }

