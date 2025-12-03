#include <WiFi.h>
#include <WebServer.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>
#include <DHT.h>
#include "index.h"

// Pinout used:
// https://mischianti.org/esp32-devkitc-v4-high-resolution-pinout-and-specs/

enum sensorPins {
  DHT_PIN = 32,
  PHOTO_PIN = 33
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

#define DHTTYPE DHT11
DHT dht(DHT_PIN, DHTTYPE);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

bool led1State = false;
bool led2State = false;
bool led3State = false;

const char* ssid = "Magis";
const char* password = "18luxindomino59";

WebServer server(80);

float temperatureReading;
int lightReading;
float humidityReading;

void handleRoot() {
  server.send(200, "text/html", INDEX_HTML);
}

void handleSensors() {
  String jsonString = "{";
  jsonString += "\"temperature\":" + String(temperatureReading) + ",";
  jsonString += "\"humidity\":" + String(humidityReading) + ",";
  jsonString += "\"light\":" + String(lightReading);
  jsonString += "}";
  Serial.println(jsonString);
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
}

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

  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("BLUETOOTH HOME CTRL");
  display.print("Temp: ");
  display.print(temperatureReading);
  display.println(" C");
  display.print("Hum: ");
  display.print(humidityReading);
  display.println(" %");
  display.print("Light: ");
  display.println(lightReading);
  display.display();

  delay(100);
}