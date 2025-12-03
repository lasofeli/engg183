// The HTML content
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!doctype html>
<html lang="en">
  <head>
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>Activity 5</title>

    <!-- Bootstrap -->
    <link href="https://cdn.jsdelivr.net/npm/bootstrap@5.3.8/dist/css/bootstrap.min.css" rel="stylesheet"
      integrity="sha384-sRIl4kxILFvY47J16cr9ZwB07vP4J8+LH7qKQnuqkuIAvNWLzeN8tE5YBujZqJLB" crossorigin="anonymous">

    <!-- Google Material Symbols -->
    <link rel="stylesheet"
      href="https://fonts.googleapis.com/css2?family=Material+Symbols+Outlined:wght@400" />

    <!-- Chart.js -->
    <script src="https://cdnjs.cloudflare.com/ajax/libs/Chart.js/3.9.1/chart.min.js"></script>

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
      .chart-container {
        position: relative;
        height: 300px;
        margin-top: 20px;
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
          Room Temperature: <span id="temperature">-- °C</span>
        </div>
        <div class="col-sm">
          <h4>
            <span class="material-symbols-outlined align-middle">light_mode</span>
            Light
          </h4>
          Room Light: <span id="light">--</span>
        </div>
        <div class="col-sm">
          <h4>
            <span class="material-symbols-outlined align-middle">humidity_percentage</span>
            Humidity
          </h4>
          Room Humidity: <span id="humidity">--%</span>
        </div>
      </div>

      <div class="row justify-content-center mb-4">
        <div class="col-md-6 text-center">
          <h4>
            <span class="material-symbols-outlined align-middle">light_mode</span>
            LED Toggles
          </h4>

          <div>
            <span class="material-symbols-outlined bulb off" data-id="1">lightbulb</span>
            <span class="material-symbols-outlined bulb off" data-id="2">lightbulb</span>
            <span class="material-symbols-outlined bulb off" data-id="3">lightbulb</span>
          </div>
        </div>
      </div>

      <div class="row">
        <div class="col-12">
          <div class="card">
            <div class="card-body">
              <h5 class="card-title text-center">Live Sensor Data</h5>
              <div class="chart-container">
                <canvas id="sensorChart"></canvas>
              </div>
            </div>
          </div>
        </div>
      </div>
    </div>

    <script>
      // Chart setup
      const ctx = document.getElementById('sensorChart').getContext('2d');
      const maxDataPoints = 20;
      
      const chartData = {
        labels: [],
        datasets: [
          {
            label: 'Temperature (°C)',
            data: [],
            borderColor: 'rgb(255, 99, 132)',
            backgroundColor: 'rgba(255, 99, 132, 0.1)',
            yAxisID: 'y',
            tension: 0.4
          },
          {
            label: 'Humidity (%)',
            data: [],
            borderColor: 'rgb(54, 162, 235)',
            backgroundColor: 'rgba(54, 162, 235, 0.1)',
            yAxisID: 'y',
            tension: 0.4
          },
          {
            label: 'Light (raw)',
            data: [],
            borderColor: 'rgb(255, 206, 86)',
            backgroundColor: 'rgba(255, 206, 86, 0.1)',
            yAxisID: 'y1',
            tension: 0.4
          }
        ]
      };

      const chart = new Chart(ctx, {
        type: 'line',
        data: chartData,
        options: {
          responsive: true,
          maintainAspectRatio: false,
          interaction: {
            mode: 'index',
            intersect: false,
          },
          scales: {
            y: {
              type: 'linear',
              display: true,
              position: 'left',
              title: {
                display: true,
                text: 'Temperature / Humidity'
              }
            },
            y1: {
              type: 'linear',
              display: true,
              position: 'right',
              title: {
                display: true,
                text: 'Light Level'
              },
              grid: {
                drawOnChartArea: false,
              }
            }
          },
          plugins: {
            legend: {
              display: true,
              position: 'top'
            }
          }
        }
      });

      // LED bulb control
      document.querySelectorAll('.bulb').forEach(bulb => {
        bulb.addEventListener('click', () => {
          const isOn = bulb.classList.contains('on');
          bulb.classList.toggle('on', !isOn);
          bulb.classList.toggle('off', isOn);

          const id = bulb.dataset.id;
          sendLEDState(id);
        });
      });

      // Poll sensor data
      setInterval(function() { getData(); }, 500);

      function getData() {
        var xhttp = new XMLHttpRequest();
        xhttp.open("GET", "sensors");

        xhttp.onload = function() {
          if (xhttp.status == 200) {
            if (xhttp.responseText) {
              console.log(xhttp.responseText);

              try {
                const data = JSON.parse(xhttp.responseText);
                
                // Update display values
                document.getElementById("temperature").innerHTML = data.temperature + " °C";
                document.getElementById("humidity").innerHTML = data.humidity + "%";
                document.getElementById("light").innerHTML = data.light;

                // Update chart
                updateChart(data.temperature, data.humidity, data.light);
              } catch (e) {
                console.error('Error parsing JSON:', e);
                document.getElementById("humidity").innerHTML = "?";
                document.getElementById("temperature").innerHTML = "?";
                document.getElementById("light").innerHTML = "?";
              }
            } else {
              console.log('Request failed. Returned status of ' + xhttp.status);
              document.getElementById("humidity").innerHTML = "?";
              document.getElementById("temperature").innerHTML = "?";
              document.getElementById("light").innerHTML = "?";
            }
          }
        }

        xhttp.send();
      }

      function updateChart(temp, humidity, light) {
        const now = new Date();
        const timeLabel = now.getHours() + ':' + 
                         String(now.getMinutes()).padStart(2, '0') + ':' + 
                         String(now.getSeconds()).padStart(2, '0');

        // Add new data
        chartData.labels.push(timeLabel);
        chartData.datasets[0].data.push(temp);
        chartData.datasets[1].data.push(humidity);
        chartData.datasets[2].data.push(light);

        // Keep only the latest maxDataPoints
        if (chartData.labels.length > maxDataPoints) {
          chartData.labels.shift();
          chartData.datasets[0].data.shift();
          chartData.datasets[1].data.shift();
          chartData.datasets[2].data.shift();
        }

        chart.update('none'); // Update without animation for smooth real-time updates
      }

      function sendLEDState(id) {
        var xhttp = new XMLHttpRequest();
        xhttp.open("GET", "lights?btn=" + id, true);
        xhttp.send();
      }
    </script>

    <script src="https://cdn.jsdelivr.net/npm/bootstrap@5.3.8/dist/js/bootstrap.bundle.min.js"
      integrity="sha384-FKyoEForCGlyvwx9Hj09JcYn3nv7wiPVlz7YYwJrWVcXK/BmnVDxM+D2scQbITxI"
      crossorigin="anonymous"></script>
  </body>
</html>
)rawliteral";