#include "MotorsController.h"
#include <Arduino.h>
#include <WebServer.h>
#include <WiFi.h>


namespace WifiManager {
  // Definimos el nombre y la contraseña de la red WiFi a la que se conectará el ESP32
  const char *ssid = "Delga";
  const char *password = "Delga1213";

  // Creamos un servidor web en el puerto 80 (puerto HTTP por defecto)
  WebServer server(80);

  // Declaración de funciones que manejarán las diferentes rutas del servidor web
  void handleRoot();
  void handleForward();
  void handleReverse();
  void handleLeft();
  void handleRight();
  void handleStop();

  // Función de configuración inicial (se debe llamar desde el setup() principal)
  void setup() {
    // Iniciamos la conexión WiFi con el SSID y contraseña definidos
    WiFi.begin(ssid, password);
    // Esperamos hasta que el ESP32 se conecte a la red WiFi
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    Serial.println("");
    Serial.println("WiFi connected.");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP()); // Mostramos la IP asignada al ESP32

    // Definimos las rutas (endpoints) y las funciones que las manejarán
    server.on("/", handleRoot);         // Página principal
    server.on("/forward", handleForward); // Mover hacia adelante
    server.on("/left", handleLeft);       // Girar a la izquierda
    server.on("/stop", handleStop);       // Detenerse
    server.on("/right", handleRight);     // Girar a la derecha
    server.on("/reverse", handleReverse); // Mover hacia atrás

    // Iniciamos el servidor web
    server.begin();
  }

  // Función que maneja la página principal del servidor web
  void handleRoot() {
    // HTML de la interfaz web para controlar el robot
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
        // Funciones JavaScript para enviar comandos al ESP32 cuando se presionan los botones
        function moveForward() { fetch('/forward'); }
        function moveLeft() { fetch('/left'); }
        function stopRobot() { fetch('/stop'); }
        function moveRight() { fetch('/right'); }
        function moveReverse() { fetch('/reverse'); }

        // (Opcional) Función para actualizar la velocidad del motor (no implementada en el backend)
        function updateMotorSpeed(pos) {
          document.getElementById('motorSpeed').innerHTML = pos;
          fetch(`/speed?value=${pos}`);
        }
      </script>
    </head>
    <body>
      <h1>ESP32 Robot Control</h1>

      <p>
        <!-- Botón para mover hacia adelante -->
        <button class="button"
          onmousedown="moveForward()" onmouseup="stopRobot()"
          ontouchstart="moveForward()" ontouchend="stopRobot()">
          FORWARD
        </button>
      </p>

      <div>
        <!-- Botón para girar a la izquierda -->
        <button class="button"
          onmousedown="moveLeft()" onmouseup="stopRobot()"
          ontouchstart="moveLeft()" ontouchend="stopRobot()">
          LEFT
        </button>

        <!-- Botón para detener el robot -->
        <button class="button stop"
          onclick="stopRobot()">
          STOP
        </button>

        <!-- Botón para girar a la derecha -->
        <button class="button"
          onmousedown="moveRight()" onmouseup="stopRobot()"
          ontouchstart="moveRight()" ontouchend="stopRobot()">
          RIGHT
        </button>
      </div>

      <p>
        <!-- Botón para mover hacia atrás -->
        <button class="button"
          onmousedown="moveReverse()" onmouseup="stopRobot()"
          ontouchstart="moveReverse()" ontouchend="stopRobot()">
          REVERSE
        </button>
      </p>

    </body>
    </html>
    )rawliteral";

    // Enviamos la página HTML al navegador del usuario
    server.send(200, "text/html", html);
  }

  // Función que se ejecuta cuando se recibe el comando de avanzar
  void handleForward() {
    Serial.println("Moving forward"); // Mensaje en el monitor serial
    MotorsController::forward();      // Llama a la función para avanzar
    server.send(200);                 // Responde al navegador que el comando fue recibido
  }

  // Función que se ejecuta cuando se recibe el comando de retroceder
  void handleReverse() {
    Serial.println("Moving reverse");
    MotorsController::backward();
    server.send(200);
  }

  // Función que se ejecuta cuando se recibe el comando de girar a la izquierda
  void handleLeft() {
    Serial.println("Turning left");
    MotorsController::left();
    server.send(200);
  }

  // Función que se ejecuta cuando se recibe el comando de girar a la derecha
  void handleRight() {
    Serial.println("Turning right");
    MotorsController::right();
    server.send(200);
  }

  // Función que se ejecuta cuando se recibe el comando de detenerse
  void handleStop() {
    Serial.println("Stopping");
    MotorsController::stop();
    server.send(200);
  }

  // Función que debe llamarse continuamente en el loop principal para atender las peticiones web
  void run() { server.handleClient(); }

} // namespace WifiManager