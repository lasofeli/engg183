#include <WiFi.h>
#include <WebServer.h>

const char* ssid = "Magis";
const char* password = "18luxindomino59";

WebServer server(80);

// The HTML content
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!doctype html>
<html lang="en">
  <head>
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>Bootstrap Dashboard</title>

    <!-- Bootstrap -->
    <link href="https://cdn.jsdelivr.net/npm/bootstrap@5.3.8/dist/css/bootstrap.min.css" rel="stylesheet"
      integrity="sha384-sRIl4kxILFvY47J16cr9ZwB07vP4J8+LH7qKQnuqkuIAvNWLzeN8tE5YBujZqJLB" crossorigin="anonymous">

    <!-- Google Material Symbols -->
    <link rel="stylesheet"
      href="https://fonts.googleapis.com/css2?family=Material+Symbols+Outlined:wght@400" />

    <style>
      .material-symbols-outlined {
        font-variation-settings:
          'FILL' 0,
          'wght' 400,
          'GRAD' 0,
          'opsz' 24;
      }
      .bulb {
        font-size: 48px;
        cursor: pointer;
        transition: color 0.3s ease, transform 0.2s ease;
        user-select: none;
      }
      .bulb.on {
        color: #FFD700;
        transform: scale(1.1);
      }
      .bulb.off {
        color: #333;
      }
    </style>
  </head>

  <body class="bg-light">
    <div class="container py-4">
      <div class="row text-center mb-4">
        <div class="col-sm">
          <h4>
            <span class="material-symbols-outlined align-middle">device_thermostat</span>
            Temperature
          </h4>
          Room Temperature: <span id="temperature">24.65 °C</span>
        </div>
        <div class="col-sm">
          <h4>
            <span class="material-symbols-outlined align-middle">light_mode</span>
            Light
          </h4>
          Room Light: <span id="light">10 billion lumens</span>
        </div>
        <div class="col-sm">
          <h4>
            <span class="material-symbols-outlined align-middle">humidity_percentage</span>
            Humidity
          </h4>
          Room Humidity: <span id="humidity">4000%</span>
        </div>
      </div>

      <div class="row justify-content-center">
        <div class="col-md-6 text-center">
          <h4>
            <span class="material-symbols-outlined align-middle">light_mode</span>
            LED Toggles
          </h4>

          <div>
            <span class="material-symbols-outlined bulb off" id="bulb1">lightbulb</span>
            <span class="material-symbols-outlined bulb off" id="bulb2">lightbulb</span>
            <span class="material-symbols-outlined bulb off" id="bulb3">lightbulb</span>
          </div>
        </div>
      </div>
    </div>

    <script>
      document.querySelectorAll('.bulb').forEach(bulb => {
        bulb.addEventListener('click', () => {
          bulb.classList.toggle('on');
          bulb.classList.toggle('off');
        });
      });
    </script>

    <script src="https://cdn.jsdelivr.net/npm/bootstrap@5.3.8/dist/js/bootstrap.bundle.min.js"
      integrity="sha384-FKyoEForCGlyvwx9Hj09JcYn3nv7wiPVlz7YYwJrWVcXK/BmnVDxM+D2scQbITxI"
      crossorigin="anonymous"></script>
  </body>
</html>
)rawliteral";

void handleRoot() {
  server.send(200, "text/html", INDEX_HTML);
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
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
}
