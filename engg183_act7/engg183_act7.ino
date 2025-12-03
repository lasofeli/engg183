#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>
#include <DHT.h>
#include <time.h>
#include "index.h"

// Pinout used:
// https://mischianti.org/esp32-devkitc-v4-high-resolution-pinout-and-specs/

enum sensorPins {
  DHT_PIN = 32,
  PHOTO_PIN = 33,
  SOIL_MOISTURE_PIN = 35
};

enum buttonPins {
  BUTTON_1 = 27,
  BUTTON_2 = 26,
  BUTTON_3 = 25
};

enum ledPins {
  LIGHT_1 = 4,
  LIGHT_2 = 2,
  LIGHT_3 = 15
};

enum oledPins {
  OLED_SDA = 21,
  OLED_SCL = 22
};

enum screenParams {
  SCREEN_WIDTH = 128,
  SCREEN_HEIGHT = 64
};

const float TEMP_HOT = 30.0;
const float TEMP_COLD = 20.0;
const int SOIL_DRY = 2000;
const int LIGHT_DARK = 500;
const float HUMIDITY_LOW = 40.0;

#define DHTTYPE DHT11
DHT dht(DHT_PIN, DHTTYPE);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

bool led1State = false;
bool led2State = false;
bool led3State = false;

bool waterPumpState = false;
bool fanState = false;
bool growingLightState = false;

const char* ssid = "Magis";
const char* password = "18luxindomino59";
const char* firebaseURL = "https://engg183-act7-default-rtdb.firebaseio.com/readings.json";

const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 0;
const int daylightOffset_sec = 0;

WebServer server(80);

float temperatureReading;
int lightReading;
float humidityReading;
int soilMoistureReading;

void handleRoot() {
  server.send(200, "text/html", INDEX_HTML);
}

void handleSensors() {
  String jsonString = "{";
  jsonString += "\"temperature\":" + String(temperatureReading) + ",";
  jsonString += "\"humidity\":" + String(humidityReading) + ",";
  jsonString += "\"light\":" + String(lightReading);
  jsonString += "}";

  server.send(200, "application/json", jsonString);
}

void handleLEDs() {
  String btn = server.arg("btn");
  if (btn == "1") {
    led1State = !led1State;
  } else if (btn == "2") {
    led2State = !led2State;
  } else {
    led3State = !led3State;
  }
  server.send(200, "text/plain", "OK");
}

String getSunLightLevel() {
  if (lightReading < 500) return "Dark";
  else if (lightReading < 1500) return "Dim";
  else if (lightReading < 2500) return "Moderate";
  else return "Bright";
}

String getSoilMoistureLevel() {
  if (soilMoistureReading < 1000) return "Wet";
  else if (soilMoistureReading < 2000) return "Moist";
  else if (soilMoistureReading < 3000) return "Dry";
  else return "Very Dry";
}

String getPlantEmotion() {
  bool tempGood = (temperatureReading >= 20.0 && temperatureReading <= 30.0);
  bool humidityGood = (humidityReading >= 40.0);
  bool soilGood = (soilMoistureReading < 2500);
  bool lightGood = (lightReading > 500);

  if (tempGood && humidityGood && soilGood && lightGood) return "Happy";
  else if (!soilGood && !lightGood) return "Sad";
  else if (!tempGood || !humidityGood) return "Stressed";
  else return "Neutral";
}

void updateSystemStates() {
  waterPumpState = (soilMoistureReading > SOIL_DRY);
  fanState = (temperatureReading > TEMP_HOT);
  growingLightState = (lightReading < LIGHT_DARK || temperatureReading < TEMP_COLD);
}

String getTimestamp() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return "N/A";
  }

  char timestamp[25];
  strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%SZ", &timeinfo);
  return String(timestamp);
}

void sendToFirebase() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    String jsonPayload = "{";
    jsonPayload += "\"Temperature\":" + String(temperatureReading) + ",";
    jsonPayload += "\"Humidity\":" + String(humidityReading) + ",";
    jsonPayload += "\"Sun Light\":\"" + getSunLightLevel() + "\",";
    jsonPayload += "\"Soil Moisture\":\"" + getSoilMoistureLevel() + "\",";
    jsonPayload += "\"Plant Emotion\":\"" + getPlantEmotion() + "\",";
    jsonPayload += "\"Water Pump State\":" + String(waterPumpState ? "true" : "false") + ",";
    jsonPayload += "\"Fan State\":" + String(fanState ? "true" : "false") + ",";
    jsonPayload += "\"Growing Light State\":" + String(growingLightState ? "true" : "false") + ",";
    jsonPayload += "\"Time Stamp\":\"" + getTimestamp() + "\"";
    jsonPayload += "}";

    http.begin(firebaseURL);
    http.addHeader("Content-Type", "application/json");

    Serial.println("\n=== Firebase HTTP Request ===");
    Serial.println("URL: " + String(firebaseURL));
    Serial.println("Method: PATCH");
    Serial.println("Content-Type: application/json");
    Serial.println("Payload: " + jsonPayload);

    int httpResponseCode = http.PATCH(jsonPayload);

    Serial.println("\n=== Firebase HTTP Response ===");
    Serial.println("Response Code: " + String(httpResponseCode));

    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println("Response Body: " + response);

      Serial.println("\n=== Response Headers ===");
      for (int i = 0; i < http.headers(); i++) {
        Serial.println(http.headerName(i) + ": " + http.header(i));
      }
    } else {
      Serial.println("Error: " + http.errorToString(httpResponseCode));
    }

    Serial.println("=============================\n");
    http.end();
  } else {
    Serial.println("WiFi Disconnected!");
  }
}

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Connected! IP Address: ");
  Serial.println(WiFi.localIP());

  Serial.println("Synchronizing time with NTP server...");
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  struct tm timeinfo;
  int retries = 0;
  while (!getLocalTime(&timeinfo) && retries < 10) {
    Serial.print(".");
    delay(1000);
    retries++;
  }

  if (retries < 10) {
    Serial.println("\nTime synchronized!");
    Serial.print("Current time: ");
    Serial.println(getTimestamp());
  } else {
    Serial.println("\nFailed to sync time, continuing anyway...");
  }

  server.on("/", handleRoot);
  server.on("/sensors", handleSensors);
  server.on("/lights", handleLEDs);
  server.begin();
  Serial.println("HTTP server started");

  dht.begin();
  Wire.begin(OLED_SDA, OLED_SCL);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED initialization failed!");
    for (;;);
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("System Booting...");
  display.display();

  pinMode(LIGHT_1, OUTPUT);
  pinMode(LIGHT_2, OUTPUT);
  pinMode(LIGHT_3, OUTPUT);
  pinMode(BUTTON_1, INPUT_PULLDOWN);
  pinMode(BUTTON_2, INPUT_PULLDOWN);
  pinMode(BUTTON_3, INPUT_PULLDOWN);
  pinMode(SOIL_MOISTURE_PIN, INPUT);
}

unsigned long lastFirebaseSend = 0;
const unsigned long firebaseInterval = 5000;

void loop() {
  server.handleClient();

  if (digitalRead(BUTTON_1)) { led1State = !led1State; delay(250); }
  if (digitalRead(BUTTON_2)) { led2State = !led2State; delay(250); }
  if (digitalRead(BUTTON_3)) { led3State = !led3State; delay(250); }

  digitalWrite(LIGHT_1, led1State);
  digitalWrite(LIGHT_2, led2State);
  digitalWrite(LIGHT_3, led3State);

  temperatureReading = dht.readTemperature();
  lightReading = analogRead(PHOTO_PIN);
  humidityReading = dht.readHumidity();
  soilMoistureReading = analogRead(SOIL_MOISTURE_PIN);

  updateSystemStates();

  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("SMART PLANT MONITOR");
  display.print("Temp: ");
  display.print(temperatureReading);
  display.println(" C");
  display.print("Hum: ");
  display.print(humidityReading);
  display.println(" %");
  display.print("Light: ");
  display.println(lightReading);
  display.print("Soil: ");
  display.println(soilMoistureReading);
  display.print("Emotion: ");
  display.println(getPlantEmotion());
  display.display();

  if (millis() - lastFirebaseSend >= firebaseInterval) {
    sendToFirebase();
    lastFirebaseSend = millis();
  }

  delay(100);
}