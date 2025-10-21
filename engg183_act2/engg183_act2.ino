#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <DHT.h>
#include "BluetoothSerial.h"

// Pinout used:
// https://mischianti.org/esp32-devkitc-v4-high-resolution-pinout-and-specs/

// === Enums for Organized Pin Mapping ===
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

// === Screen Parameters ===
enum screenParams {
  SCREEN_WIDTH = 128,
  SCREEN_HEIGHT = 64
};

// === Sensor and Device Objects ===
#define DHTTYPE DHT11
DHT dht(DHT_PIN, DHTTYPE);
BluetoothSerial SerialBT;
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// === LED States ===
bool led1State = false;
bool led2State = false;
bool led3State = false;

void setup() {
  Serial.begin(115200);
  SerialBT.begin("ESP32_BT_Controller");
  Serial.println("✅ Bluetooth device ready to pair.");

  // === Initialize Sensors ===
  dht.begin();
  Wire.begin(OLED_SDA, OLED_SCL);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("❌ OLED initialization failed!");
    for (;;);
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("System Booting...");
  display.display();

  // === Initialize I/O ===
  pinMode(LIGHT_1, OUTPUT);
  pinMode(LIGHT_2, OUTPUT);
  pinMode(LIGHT_3, OUTPUT);

  pinMode(BUTTON_1, INPUT_PULLDOWN);
  pinMode(BUTTON_2, INPUT_PULLDOWN);
  pinMode(BUTTON_3, INPUT_PULLDOWN);
}

void loop() {
  // Serial.println("Led1state: " + String(led1State));
  // Serial.println("Led2state: " + String(led2State));
  // Serial.println("Led3state: " + String(led3State));
  
  // --- Physical Button Control ---
  if (digitalRead(BUTTON_1)) { led1State = !led1State; delay(250); }
  if (digitalRead(BUTTON_2)) { led2State = !led2State; delay(250); }
  if (digitalRead(BUTTON_3)) { led3State = !led3State; delay(250); }

  digitalWrite(LIGHT_1, led1State);
  digitalWrite(LIGHT_2, led2State);
  digitalWrite(LIGHT_3, led3State);

  // --- Bluetooth Control ---
  if (SerialBT.available()) {
    String data = SerialBT.readStringUntil('\n');  // Expecting CSV, e.g. "1,#,0"
    parseBluetoothData(data);
  }

  // --- Sensor Reading ---
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();
  int ldrValue = analogRead(PHOTO_PIN);

  // --- OLED Display Update ---
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("BLUETOOTH HOME CTRL");
  display.print("Temp: ");
  display.print(temp);
  display.println(" C");
  display.print("Hum: ");
  display.print(hum);
  display.println(" %");
  display.print("Light: ");
  display.println(ldrValue);
  display.display();

  // --- Send Data via Bluetooth ---
  SerialBT.print("TEMP=");
  SerialBT.print(temp);
  SerialBT.print(",HUM=");
  SerialBT.print(hum);
  SerialBT.print(",LIGHT=");
  SerialBT.println(ldrValue);

  delay(100);
}

// --- CSV Parsing Function ---
void parseBluetoothData(String data) {
  data.trim();
  data.replace("#", "x"); // Mark dummy fields

  int firstComma = data.indexOf(',');
  int secondComma = data.lastIndexOf(',');

  String val1 = data.substring(0, firstComma);
  String val2 = data.substring(firstComma + 1, secondComma);
  String val3 = data.substring(secondComma + 1);

  if (val1 != "x" && val1.length() > 0) led1State = val1.toInt();
  if (val2 != "x" && val2.length() > 0) led2State = val2.toInt();
  if (val3 != "x" && val3.length() > 0) led3State = val3.toInt();

  digitalWrite(LIGHT_1, led1State);
  digitalWrite(LIGHT_2, led2State);
  digitalWrite(LIGHT_3, led3State);
}

