// Reference pinout
// https://mischianti.org/nodemcu-v3-high-resolution-pinout-and-specs/

#include<vector>
#include <bits/stdc++.h>
#include<cmath>
#include <HTTPClient.h>
#include <WiFi.h>
#include "time.h"
#include "esp_sntp.h"

const int ULTRASONIC_TRIG = 23;
const int ULTRASONIC_ECHO = 22;

/*
button
led
lever data pin
*/

const int BUTTON_PIN = 25;
const int SWITCH_PIN = 26;
const int LED_PIN = 27;

int ledBlinkMonitor = 0;

bool ledState = false;

unsigned long ledBlinkInterval = 0.3 * 1000;

unsigned long pastLedTime = 0;
unsigned long pastMeasTime;
unsigned long currentTime;
unsigned long startTime;

unsigned long lastNtpUpdate = 5 * 1000; // Offset by 5 seconds

unsigned long wifiAttemptsInterval = 5 * 1000;
unsigned long ntpAttemptsInterval = 5 * 1000;
unsigned long measAttemptsInterval = 5 * 1000;

String ssid = "sleepy spot";
String password = "melancholic";

String sensorID;
String sensorLatitude = "14.6380314";
String sensorLongitude = "121.07586937809633";

String serverName = "https://oriented-locust-usually.ngrok-free.app/api/v1/sensor/ingest";

const char* ntpServer = "pool.ntp.org";
const long  gmtOffsetInSec = 0 * 3600; // not needed; we're using UTC
const long  daylightOffsetInSec = 0;

float floorMeasurement = 0;

enum State
{
  STATE_ACTIVE,
  STATE_WIFI_CHECK,
  STATE_MEAS_CHECK,
  STATE_UPLOAD_CHECK,
  STATE_DORMANT
};

int patience = 0;

State CurrentState = STATE_WIFI_CHECK;

class measurementClass
{
  public:
    float distance;
    String timeStamp;
    bool validMeas = false;
};

std::vector<measurementClass> measurementsVector;

void setup() {
  Serial.begin(115200);

  pinMode(ULTRASONIC_ECHO, INPUT);
  pinMode(ULTRASONIC_TRIG, OUTPUT);
  pinMode(BUTTON_PIN, INPUT);
  pinMode(SWITCH_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);

  WiFi.begin(ssid, password);
  Serial.println("Connecting");
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  CurrentState = STATE_WIFI_CHECK;

  configTime(gmtOffsetInSec, daylightOffsetInSec, ntpServer);

  while (true)
  {
    if (measureFloor())
    {
      break;
    }
  }

  uint8_t mac[6];
  WiFi.macAddress(mac);
  sensorID =  String(mac[0],HEX) +String(mac[1],HEX) +String(mac[2],HEX) +String(mac[3],HEX) + String(mac[4],HEX) + String(mac[5],HEX);
}

void loop() {
  bool ledStatus = !(digitalRead(SWITCH_PIN));

  // Serial.println("SWITCH STATUS IS " + String(ledStatus));

  if (!ledStatus)
  {
    CurrentState = STATE_DORMANT;
  }

  if (!digitalRead(BUTTON_PIN) && ledBlinkMonitor == 0)
  {
    measureFloor();
    ledBlinkMonitor = 1;
  }
  switch(CurrentState)
  {
    case STATE_ACTIVE:
      {
        measurementClass currentMeas;
        float aveDistance = 0;
        int invalidOutputs = 0;
        int numberValidOutputs = 0;

        Serial.println("STATE_ACTIVE: Starting measurements!");

        for (int i = 0; i < 7; i++)
        {
          float d = sendPulse();
          if (isnan(d)) // Skip NAN outputs
          {
            invalidOutputs++;
          }
          else
          {
            aveDistance += d;
            numberValidOutputs++;
          }
          delay(150); // As far as I can tell, this should be okay.
        }
        if (invalidOutputs != 7) // might want to make this stricter
        {
          aveDistance /= numberValidOutputs;

          float floodHeight = floorMeasurement - aveDistance;

          floodHeight = std::max(float(0), floodHeight);

          currentMeas.distance = floodHeight;
          currentMeas.validMeas = true;
          Serial.println("STATE_ACTIVE: Measured " + String(floodHeight));
        }
        else
        {
          Serial.println("STATE_ACTIVE: Unable to get valid measurement.");
          currentMeas.validMeas = false;
        }

        struct tm timeinfo;

        if (getLocalTime(&timeinfo))
        {
          char isoTimeStr[21];
          
          size_t result = strftime(
              isoTimeStr,                // The output buffer
              21,         // The maximum size of the buffer
              "%Y-%m-%dT%H:%M:%SZ",      // The ISO 8601 format string
              &timeinfo                  // The source struct tm
          );
          
          currentMeas.timeStamp = String(isoTimeStr);
        }
        else
        {
          pastMeasTime = millis();
          CurrentState = STATE_WIFI_CHECK;

          break;
        }

        measurementsVector.push_back(currentMeas);

        pastMeasTime = millis();
        patience = 0;
        CurrentState = STATE_WIFI_CHECK;
        break;
      }
    case STATE_WIFI_CHECK:
      {
        if (WiFi.status() == WL_CONNECTED)
        {
          Serial.println("STATE_WIFI_CHECK: WiFI Connected!");
          patience = 0;
          CurrentState = STATE_MEAS_CHECK;
        }
        else
        {
          if (patience == 0)
          {
            startTime = millis();
            patience++;
          }

          if (patience >= 5)
          {
            patience = 0;
            CurrentState = STATE_MEAS_CHECK;
            Serial.println("STATE_WIFI_CHECK: Patience exceeded! Going to STATE_MEAS_CHECK");
            
            break;
          }
          
          if (millis() - startTime >= wifiAttemptsInterval)
          {
            startTime = millis();
            patience++;

            WiFi.begin(ssid, password);
          }
        }
        
        break;
      }

    case STATE_MEAS_CHECK:
      {
        if (millis() - pastMeasTime >= measAttemptsInterval)
        {
          patience = 0;
          CurrentState = STATE_ACTIVE;
          Serial.println("STATE_MEAS_CHECK: Starting scheduled Measurement!");

          break;
        }

        patience = 0;
        Serial.println("STATE_MEAS_CHECK: Starting upload check!");
        CurrentState = STATE_UPLOAD_CHECK;
        break;
      }

    case STATE_UPLOAD_CHECK:
      {
        if (measurementsVector.empty())
        {
          patience = 0;
          Serial.println("STATE_UPLOAD_CHECK: No uploads queud!");
          CurrentState = STATE_WIFI_CHECK;

          break;
        }

        for (int i = (measurementsVector.size() - 1); i >= 0; i--)
        {
          Serial.println("STATE_UPLOAD_CHECK: Starting uploads of size " + String(measurementsVector.size()) + "!");
          measurementClass measInstance;
          measInstance = measurementsVector.at(i);

          if (uploadJSON(measInstance))
          {
            measurementsVector.erase(measurementsVector.begin() + i);
          }
        }

        patience = 0;
        CurrentState = STATE_WIFI_CHECK;
        break;
      }
    case STATE_DORMANT:
    {
      CurrentState = STATE_WIFI_CHECK; 
      break;
    }
  }

  lightLED(ledBlinkMonitor, ledStatus, ledState, pastLedTime);
}

float sendPulse()
{
  float duration, distance;

  digitalWrite(ULTRASONIC_TRIG, LOW);  
	delayMicroseconds(2);  
	digitalWrite(ULTRASONIC_TRIG, HIGH);  
	delayMicroseconds(10);  
	digitalWrite(ULTRASONIC_TRIG, LOW);

  duration = pulseIn(ULTRASONIC_ECHO, HIGH, 30 * 1000);
  distance = (duration * 0.000343) / 2;

  return distance;
}

bool uploadJSON(measurementClass meas)
{
  HTTPClient http;
  http.begin(serverName);
  
  String postString = "{\"sensor_id\":\"" + sensorID
    + "\",\"lat\":\"" + sensorLatitude
    + "\",\"lon\":\"" + sensorLongitude
    + "\",\"flood_m\":\"" + meas.distance
    + "\",\"reported_on\":\"" + meas.timeStamp + "\"}";

  http.addHeader("Content-Type", "application/json");

  int httpResponseCode = http.POST(postString);  
  
  Serial.println("HTTP Code was " + String(httpResponseCode) + " for timestamp " + meas.timeStamp);

  if (httpResponseCode > 0 && httpResponseCode <= 299)
  {
    http.end();
    return true;
  }
  else
  {
    http.end();
    return false;
  }
}

void lightLED (int &ledBlink, bool ledOn, bool &ledState, unsigned long &pastTime)
{
  if (ledBlink == 0)
  {
    digitalWrite(LED_PIN, ledOn);
  }
  else
  {
    if (ledBlink >= 8)
    {
      digitalWrite(LED_PIN, ledOn);
      ledBlink = 0;
    }
    else
    {
      if (millis() - pastTime >= ledBlinkInterval)
      {
        Serial.println("LED Blink toggled: " + String(ledState));
        digitalWrite(LED_PIN, ledState);
        ledState = !ledState;
        pastTime = millis();
        ledBlink++;
      }
    }
  }
}

bool measureFloor()
{
  float aveDistance = 0;
  
  int invalidOutputs = 0;
  int numberValidOutputs = 0;

  for (int i = 0; i < 7; i++)
  {
    float d = sendPulse();
    if (isnan(d)) // Skip NAN outputs
    {
      invalidOutputs++;
    }
    else
    {
      aveDistance += d;
      numberValidOutputs++;
    }
    delay(150);
  }
  if (invalidOutputs != 9)
  {
    aveDistance /= numberValidOutputs;

    floorMeasurement = aveDistance;

    return true;
  }
  else
  {
    Serial.println("Unable to get valid measurement.");

    return false;
  }
}
