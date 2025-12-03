#!/bin/bash

# Replace these with your actual values
temperatureReading=35.6
humidityReading=60
lightReading=300

JSON_STRING=$(cat <<EOF
{
  "temperature": $temperatureReading,
  "humidity": $humidityReading,
  "light": $lightReading
}
EOF
)

curl -X PATCH \
  -H "Content-Type: application/json" \
  -d "$JSON_STRING" \
  "https://engg183-act7-default-rtdb.firebaseio.com/readings.json"
