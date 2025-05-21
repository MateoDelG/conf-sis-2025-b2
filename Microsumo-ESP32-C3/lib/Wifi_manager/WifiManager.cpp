#include "MotorsController.h"
#include <Arduino.h>
#include <WebServer.h>
#include <WiFi.h>


namespace WifiManager {

const char *ssid = "Delga";
const char *password = "Delga1213";
WebServer server(80);

void handleRoot();
void handleForward();
void handleReverse();
void handleLeft();
void handleRight();
void handleStop();

void setup() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  // Define routes
  server.on("/", handleRoot);
  server.on("/forward", handleForward);
  server.on("/left", handleLeft);
  server.on("/stop", handleStop);
  server.on("/right", handleRight);
  server.on("/reverse", handleReverse);

  // Start the server
  server.begin();
}

void handleRoot() {
  const char html[] PROGMEM = R"rawliteral(
  <!DOCTYPE HTML><html>
  <head>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <link rel="icon" href="data:,">
    <style>
      body {
        font-family: 'Segoe UI', sans-serif;
        background-color: #1e1e1e;
        color: #eeeeee;
        margin: 0;
        padding: 20px;
        text-align: center;
      }

      h1 {
        color: #cccccc;
        font-weight: 400;
        margin-bottom: 40px;
      }

      .button {
        user-select: none;
        border: 1px solid #444;
        padding: 14px 32px;
        margin: 10px;
        font-size: 18px;
        border-radius: 8px;
        background-color: #2c2c2c;
        color: #ffffff;
        cursor: pointer;
        transition: all 0.2s ease-in-out;
      }

      .button:hover {
        background-color: #3a3a3a;
        border-color: #666;
      }

      .button:active {
        background-color: #505050;
      }

      .stop {
        background-color: #444;
      }

      .stop:hover {
        background-color: #5a5a5a;
      }

      input[type=range] {
        width: 80%%;
        margin-top: 30px;
        appearance: none;
        height: 6px;
        background: #333;
        border-radius: 3px;
        outline: none;
      }

      input[type=range]::-webkit-slider-thumb {
        appearance: none;
        width: 16px;
        height: 16px;
        background: #888;
        border-radius: 50%%;
        cursor: pointer;
        border: 1px solid #aaa;
      }

      #motorSpeed {
        font-weight: bold;
        font-size: 16px;
        color: #aaaaaa;
      }
    </style>

    <script>
      function moveForward() { fetch('/forward'); }
      function moveLeft() { fetch('/left'); }
      function stopRobot() { fetch('/stop'); }
      function moveRight() { fetch('/right'); }
      function moveReverse() { fetch('/reverse'); }

      function updateMotorSpeed(pos) {
        document.getElementById('motorSpeed').innerHTML = pos;
        fetch(`/speed?value=${pos}`);
      }
    </script>
  </head>
  <body>
    <h1>ESP32 Robot Control</h1>

    <p>
      <button class="button"
        onmousedown="moveForward()" onmouseup="stopRobot()"
        ontouchstart="moveForward()" ontouchend="stopRobot()">
        FORWARD
      </button>
    </p>

    <div>
      <button class="button"
        onmousedown="moveLeft()" onmouseup="stopRobot()"
        ontouchstart="moveLeft()" ontouchend="stopRobot()">
        LEFT
      </button>

      <button class="button stop"
        onclick="stopRobot()">
        STOP
      </button>

      <button class="button"
        onmousedown="moveRight()" onmouseup="stopRobot()"
        ontouchstart="moveRight()" ontouchend="stopRobot()">
        RIGHT
      </button>
    </div>

    <p>
      <button class="button"
        onmousedown="moveReverse()" onmouseup="stopRobot()"
        ontouchstart="moveReverse()" ontouchend="stopRobot()">
        REVERSE
      </button>
    </p>

  </body>
  </html>
  )rawliteral";

  server.send(200, "text/html", html);
}




void handleForward() {
  Serial.println("Moving forward");
  MotorsController::forward();
  server.send(200);
}
void handleReverse() {
  Serial.println("Moving reverse");
  MotorsController::backward();
  server.send(200);
}

void handleLeft() {
  Serial.println("Turning left");
  MotorsController::left();
  server.send(200);
}

void handleRight() {
  Serial.println("Turning right");
  MotorsController::right();
  server.send(200);
}
void handleStop() {
  Serial.println("Stopping");
  MotorsController::stop();
  server.send(200);
}

void run() { server.handleClient(); }

} // namespace WifiManager