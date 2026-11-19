#include <WiFi.h>
#include <WebServer.h>

// ===============================
// Wi-Fi SETTINGS
// ===============================
const char* WIFI_SSID = "WIFI_NAME";
const char* WIFI_PASSWORD = "WIFI_PASSWORD";

// ===============================
// HC-SR04 PINS
//
// NOTE: do NOT use GPIO12 here.
// GPIO12 (MTDI) is a strapping pin —
// if it is held HIGH at reset the
// ESP32 sets the flash to 1.8V and
// fails to boot ("invalid header:
// 0xffffffff"). GPIO5/18/19/21/22/23
// are safe.
//
// ECHO is 5V on the HC-SR04: use a
// divider (1k + 2k) down to 3.3V
// before it reaches the ESP32 pin.
// ===============================
#define TRIG_PIN 5
#define ECHO_PIN 18

// ===============================
// WATER TANK CALIBRATION
// Change these later using your
// actual tank measurements.
// ===============================
const float EMPTY_DISTANCE = 100.0;
const float FULL_DISTANCE  = 5.0;

// ===============================
// WEB SERVER
// ===============================
WebServer server(80);

// ===============================
// VARIABLES
// ===============================
float distanceCm = 0.0;
float waterLevel = 0.0;

unsigned long lastSensorRead = 0;
const unsigned long SENSOR_INTERVAL = 1000;

// ===============================
// READ DISTANCE
// ===============================
float readDistance() {

  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);

  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);

  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000);

  if (duration == 0) {
    return -1;
  }

  float distance = duration * 0.0343 / 2.0;

  return distance;
}

// ===============================
// CALCULATE WATER LEVEL
// ===============================
void updateWaterLevel() {

  float newDistance = readDistance();

  if (newDistance < 0) {
    return;
  }

  distanceCm = newDistance;

  waterLevel =
    ((EMPTY_DISTANCE - distanceCm) /
    (EMPTY_DISTANCE - FULL_DISTANCE)) * 100.0;

  waterLevel = constrain(waterLevel, 0, 100);

  Serial.print("Distance: ");
  Serial.print(distanceCm, 1);
  Serial.print(" cm | Water Level: ");
  Serial.print(waterLevel, 1);
  Serial.println("%");
}

// ===============================
// WEB PAGE
// ===============================
void handleRoot() {

  String levelText = String(waterLevel, 1);
  String distanceText = String(distanceCm, 1);

  String waterStatus;

  if (waterLevel >= 80) {
    waterStatus = "Tank High";
  }
  else if (waterLevel >= 40) {
    waterStatus = "Normal Level";
  }
  else if (waterLevel >= 20) {
    waterStatus = "Low Level";
  }
  else {
    waterStatus = "Very Low";
  }

  String html = R"rawliteral(

<!DOCTYPE html>
<html>
<head>

<meta name="viewport" content="width=device-width, initial-scale=1.0">

<meta http-equiv="refresh" content="5">

<title>Smart Water Tank</title>

<style>

* {
  box-sizing: border-box;
  margin: 0;
  padding: 0;
}

body {
  font-family: Arial, Helvetica, sans-serif;
  background: #07111f;
  color: #ffffff;
  min-height: 100vh;
}

/* ================= HEADER ================= */

.header {
  padding: 24px 6%;
  border-bottom: 1px solid #1d2b3c;
  display: flex;
  justify-content: space-between;
  align-items: center;
}

.logo h1 {
  font-size: 22px;
  letter-spacing: 1px;
}

.logo p {
  color: #8fa4b8;
  font-size: 13px;
  margin-top: 5px;
}

.status {
  display: flex;
  align-items: center;
  gap: 8px;
  background: #0c1b2a;
  padding: 9px 14px;
  border-radius: 20px;
  font-size: 13px;
}

.status-dot {
  width: 9px;
  height: 9px;
  background: #22c55e;
  border-radius: 50%;
}

/* ================= MAIN ================= */

.container {
  width: 88%;
  max-width: 1200px;
  margin: 35px auto;
}

.welcome {
  margin-bottom: 25px;
}

.welcome h2 {
  font-size: 28px;
  margin-bottom: 8px;
}

.welcome p {
  color: #8fa4b8;
}

/* ================= GRID ================= */

.grid {
  display: grid;
  grid-template-columns: 1.3fr 1fr;
  gap: 22px;
}

/* ================= CARD ================= */

.card {
  background: #0b1928;
  border: 1px solid #1b2c3e;
  border-radius: 18px;
  padding: 25px;
}

.card-title {
  color: #8fa4b8;
  font-size: 14px;
  margin-bottom: 20px;
}

/* ================= TANK ================= */

.tank-area {
  display: flex;
  justify-content: center;
  align-items: center;
  padding: 10px;
}

.tank {
  width: 190px;
  height: 300px;
  border: 4px solid #456176;
  border-radius: 18px;
  position: relative;
  overflow: hidden;
  background: #08131f;
}

.water {
  position: absolute;
  bottom: 0;
  left: 0;
  width: 100%;
  height: )rawliteral";

  html += String(waterLevel, 1);

  html += R"rawliteral(%;
  background: linear-gradient(
    to top,
    #087ea4,
    #12b5d9
  );
  transition: height 0.5s ease;
}

.water::before {
  content: "";
  position: absolute;
  top: -7px;
  left: -10%;
  width: 120%;
  height: 14px;
  background: rgba(255,255,255,0.15);
  border-radius: 50%;
}

.level-number {
  position: absolute;
  width: 100%;
  top: 45%;
  text-align: center;
  font-size: 42px;
  font-weight: bold;
  z-index: 2;
  text-shadow: 0 2px 10px rgba(0,0,0,0.5);
}

/* ================= METRICS ================= */

.metrics {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 15px;
}

.metric {
  background: #081522;
  border-radius: 14px;
  padding: 20px;
  border: 1px solid #17293a;
}

.metric-label {
  color: #7890a4;
  font-size: 12px;
  margin-bottom: 10px;
}

.metric-value {
  font-size: 24px;
  font-weight: bold;
}

.metric-unit {
  color: #7890a4;
  font-size: 12px;
}

/* ================= STATUS ================= */

.level-status {
  margin-top: 18px;
  padding: 16px;
  border-radius: 12px;
  background: #0a2030;
  border-left: 4px solid #12b5d9;
}

.level-status span {
  color: #12b5d9;
  font-weight: bold;
}

/* ================= INFO ================= */

.info-grid {
  margin-top: 22px;
  display: grid;
  grid-template-columns: repeat(3, 1fr);
  gap: 15px;
}

.info {
  background: #0b1928;
  border: 1px solid #1b2c3e;
  padding: 20px;
  border-radius: 15px;
}

.info h4 {
  color: #7890a4;
  font-size: 12px;
  margin-bottom: 8px;
}

.info p {
  font-size: 15px;
}

/* ================= FOOTER ================= */

.footer {
  text-align: center;
  padding: 30px;
  color: #52697d;
  font-size: 12px;
}

/* ================= MOBILE ================= */

@media (max-width: 800px) {

  .header {
    padding: 20px;
  }

  .container {
    width: 92%;
  }

  .grid {
    grid-template-columns: 1fr;
  }

  .info-grid {
    grid-template-columns: 1fr;
  }

  .welcome h2 {
    font-size: 23px;
  }

}

</style>

</head>

<body>

<!-- HEADER -->

<header class="header">

  <div class="logo">

    <h1>SMART WATER TANK</h1>

    <p>IoT Water Level Monitoring System</p>

  </div>

  <div class="status">

    <div class="status-dot"></div>

    SYSTEM ONLINE

  </div>

</header>


<!-- MAIN -->

<main class="container">

  <div class="welcome">

    <h2>Water Tank Dashboard</h2>

    <p>
      Real-time monitoring from your ESP32 sensor system.
    </p>

  </div>


  <div class="grid">


    <!-- TANK CARD -->

    <div class="card">

      <div class="card-title">
        CURRENT WATER LEVEL
      </div>

      <div class="tank-area">

        <div class="tank">

          <div class="water"></div>

          <div class="level-number">
            )rawliteral";

  html += levelText;

  html += R"rawliteral(%
          </div>

        </div>

      </div>

    </div>


    <!-- SENSOR CARD -->

    <div class="card">

      <div class="card-title">
        SENSOR INFORMATION
      </div>

      <div class="metrics">


        <div class="metric">

          <div class="metric-label">
            DISTANCE
          </div>

          <div class="metric-value">
            )rawliteral";

  html += distanceText;

  html += R"rawliteral(
            <span class="metric-unit">cm</span>
          </div>

        </div>


        <div class="metric">

          <div class="metric-label">
            WATER LEVEL
          </div>

          <div class="metric-value">
            )rawliteral";

  html += levelText;

  html += R"rawliteral(
            <span class="metric-unit">%</span>
          </div>

        </div>

      </div>


      <div class="level-status">

        CURRENT STATUS:
        <span>)rawliteral";

  html += waterStatus;

  html += R"rawliteral(</span>

      </div>

    </div>

  </div>


  <!-- DEVICE INFORMATION -->

  <div class="info-grid">


    <div class="info">

      <h4>DEVICE</h4>

      <p>ESP32 Tank Monitor</p>

    </div>


    <div class="info">

      <h4>CONNECTION</h4>

      <p>Wi-Fi Connected</p>

    </div>


    <div class="info">

      <h4>UPDATE RATE</h4>

      <p>Every 5 seconds</p>

    </div>


  </div>


</main>


<footer class="footer">

  Smart Water Tank Monitoring System
  <br>
  Powered by ESP32

</footer>


</body>

</html>

)rawliteral";

  server.send(200, "text/html", html);
}

// ===============================
// 404 HANDLER
// ===============================
void handleNotFound() {
  server.send(404, "text/plain", "Not found");
}

// ===============================
// SETUP
// ===============================
void setup() {

  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("==============================");
  Serial.println("SMART WATER TANK - ESP32");
  Serial.println("==============================");

  // --- Sensor pins ---
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  digitalWrite(TRIG_PIN, LOW);

  // --- Wi-Fi ---
  Serial.print("Connecting to Wi-Fi: ");
  Serial.println(WIFI_SSID);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  unsigned long startAttempt = millis();

  while (WiFi.status() != WL_CONNECTED &&
         millis() - startAttempt < 20000) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("Connected. Open this in your browser: http://");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("Wi-Fi connection FAILED.");
    Serial.println("Check SSID/password, and that the network is 2.4 GHz.");
  }

  // --- Web server routes ---
  server.on("/", handleRoot);
  server.onNotFound(handleNotFound);
  server.begin();

  Serial.println("HTTP server started.");

  // First reading right away
  updateWaterLevel();
}

// ===============================
// LOOP
// ===============================
void loop() {

  server.handleClient();

  if (millis() - lastSensorRead >= SENSOR_INTERVAL) {

    lastSensorRead = millis();

    updateWaterLevel();
  }
}
