#define CONFIG_ASYNC_TCP_RUNNING_CORE 1
#define ASYNC_TCP_PRIORITY 3
#define SERVER_TASK_PRIORITY 2
#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <HX711.h>
#include <DHT.h>
#include <ArduinoJson.h>

// Wi-Fi credentials
const char* ssid = "ihithitler";
const char* password = "ihithitlerr";

// Pin Definitions
#define TRIG_PIN_1 26
#define ECHO_PIN_1 25
#define TRIG_PIN_2 14
#define ECHO_PIN_2 27
#define IR_PIN 33
#define DHT_PIN 32
#define BUZZER_PIN 15
#define LOADCELL_DOUT_PIN 4
#define LOADCELL_SCK_PIN 5

// Constants
#define DHT_TYPE DHT11 // Change to DHT22 if using that sensor
#define DISTANCE_THRESHOLD 50.0 // cm
#define WEIGHT_THRESHOLD 50.0 // grams

// Add these definitions at the top after other #defines
#define CONFIG_ASYNC_TCP_RUNNING_CORE 1
#define ASYNC_TCP_PRIORITY 3
#define SERVER_TASK_PRIORITY 2

// Global Variables
HX711 scale;
DHT dht(DHT_PIN, DHT_TYPE);
AsyncWebServer server(80);
AsyncEventSource events("/events");

// HTML Page
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>ESP32 Sensor Dashboard</title>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body {
      font-family: 'Poppins', sans-serif;
      text-align: center;
      margin: 0;
      padding: 0;
      background-color: #121212;
      color: #f1f1f1;
    }
    h1 {
      background: linear-gradient(135deg, #007bff, #00c6ff);
      color: white;
      padding: 20px;
      margin: 0;
      font-size: 24px;
      text-transform: uppercase;
      letter-spacing: 2px;
    }
    .container {
      max-width: 500px;
      margin: 30px auto;
      background: #1e1e1e;
      padding: 20px;
      border-radius: 10px;
      box-shadow: 0px 4px 8px rgba(0, 0, 0, 0.3);
    }
    .sensor {
      font-weight: bold;
      font-size: 24px;
      color: #00c6ff;
      display: inline-block;
      transition: transform 0.3s ease-in-out;
    }
    .sensor:hover {
      transform: scale(1.1);
      color: #ff4081;
    }
    p {
      font-size: 18px;
      margin: 15px 0;
      color: #bbb;
    }
    footer {
      margin-top: 20px;
      font-size: 14px;
      color: #888;
    }
  </style>
</head>
<body>
  <h1>ESP32 Sensor Dashboard</h1>
  <div class="container">
    <p>Distance 1: <span id="distance1" class="sensor">--</span> cm</p>
    <p>Distance 2: <span id="distance2" class="sensor">--</span> cm</p>
    <p>IR Sensor: <span id="irDetected" class="sensor">--</span></p>
    <p>Humidity: <span id="humidity" class="sensor">--</span> %</p>
    <p>Temperature: <span id="temperature" class="sensor">--</span> °C</p>
    <p>Weight: <span id="weight" class="sensor">--</span> g</p>
  </div>
  <footer>&copy; 2025 Your Name</footer>
  <script>
    if (!!window.EventSource) {
      var source = new EventSource('/events');
      source.addEventListener('message', function(e) {
        var data = JSON.parse(e.data);
        document.getElementById('distance1').innerText = data.distance1;
        document.getElementById('distance2').innerText = data.distance2;
        document.getElementById('irDetected').innerText = data.irDetected ? "Object Detected" : "No Object";
        document.getElementById('humidity').innerText = data.humidity;
        document.getElementById('temperature').innerText = data.temperature;
        document.getElementById('weight').innerText = data.weight;
      }, false);
    }
  </script>
</body>
</html>
)rawliteral";

float measureDistance(int trigPin, int echoPin) {
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);
    
    unsigned long timeout = 30000;  // 30ms timeout
    long duration = pulseIn(echoPin, HIGH, timeout);
    
    if (duration == 0) {
        Serial.println("Distance measurement timeout");
        return 999.9;
    }
    
    float distance = duration * 0.034 / 2;
    return (distance > 400.0) ? 999.9 : distance;  // Filter invalid readings
}

void initWiFi() {
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);
    
    Serial.print("Connecting to WiFi");
    WiFi.begin(ssid, password);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nConnected!");
        Serial.print("IP: ");
        Serial.println(WiFi.localIP());
    } else {
        Serial.println("\nConnection failed - Restarting");
        ESP.restart();
    }
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    // Initialize pins first
    pinMode(TRIG_PIN_1, OUTPUT);
    pinMode(ECHO_PIN_1, INPUT);
    pinMode(TRIG_PIN_2, OUTPUT);
    pinMode(ECHO_PIN_2, INPUT);
    pinMode(IR_PIN, INPUT);
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);
    
    // Initialize sensors
    dht.begin();
    
    // Initialize scale
    scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
    if (scale.wait_ready_timeout(1000)) {
        Serial.println("HX711 initialized");
        scale.set_scale();
        scale.tare();
    } else {
        Serial.println("HX711 not found!");
        delay(1000);
        ESP.restart();
    }
    
    // Initialize WiFi first, before web server
    initWiFi();
    delay(1000); // Give WiFi time to stabilize
    
    // Initialize web server with protective delay
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
    
    // Server routes with error checking
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (request->client()->space() > 0) {
            request->send(200, "text/html", index_html);
        }
    });
    
    events.onConnect([](AsyncEventSourceClient *client) {
        if (client->lastId()) {
            Serial.printf("Client reconnected! Last ID: %u\n", client->lastId());
        }
    });
    
    server.addHandler(&events);
    
    // Start server with protective delay
    delay(100);
    server.begin();
    delay(100);
    
    Serial.println("Server started");
}

void loop() {
    static unsigned long lastUpdate = 0;
    static unsigned long lastWiFiCheck = 0;
    const unsigned long UPDATE_INTERVAL = 1000;
    const unsigned long WIFI_CHECK_INTERVAL = 5000;
    
    // Regular sensor updates
    if (millis() - lastUpdate >= UPDATE_INTERVAL) {
        // Create JSON document with exact size calculation
        const size_t capacity = JSON_OBJECT_SIZE(6) + 100;
        StaticJsonDocument<capacity> doc;
        
        // Read sensors only when needed
        doc["distance1"] = measureDistance(TRIG_PIN_1, ECHO_PIN_1);
        doc["distance2"] = measureDistance(TRIG_PIN_2, ECHO_PIN_2);
        doc["irDetected"] = digitalRead(IR_PIN);
        
        float h = dht.readHumidity();
        float t = dht.readTemperature();
        doc["humidity"] = isnan(h) ? 0 : h;
        doc["temperature"] = isnan(t) ? 0 : t;
        
        float weight = 0;
        if (scale.wait_ready_timeout(100)) {
            weight = scale.get_units(3);
            if (isnan(weight) || weight < -999999 || weight > 999999) {
                weight = 0;
            }
        }
        doc["weight"] = weight;
        
        // Use char array instead of String for JSON
        char jsonBuffer[256];
        serializeJson(doc, jsonBuffer);
        
        // Send only if WiFi is connected
        if (WiFi.status() == WL_CONNECTED) {
            events.send(jsonBuffer, "message", millis());
            Serial.println(jsonBuffer);
        }
        
        // Threshold checks
        bool alert = false;
        float d1 = doc["distance1"].as<float>();
        float d2 = doc["distance2"].as<float>();
        
        if (d1 > 0 && d1 < DISTANCE_THRESHOLD) alert = true;
        if (d2 > 0 && d2 < DISTANCE_THRESHOLD) alert = true;
        if (weight > 0 && weight < WEIGHT_THRESHOLD) alert = true;
        
        digitalWrite(BUZZER_PIN, alert ? HIGH : LOW);
        
        lastUpdate = millis();
    }
    
    // WiFi check with more delay between reconnection attempts
    if (millis() - lastWiFiCheck >= WIFI_CHECK_INTERVAL) {
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("WiFi disconnected - Reconnecting");
            WiFi.disconnect(true);
            delay(1000);
            initWiFi();
        }
        lastWiFiCheck = millis();
    }
    
    // Increased delay to reduce CPU load
    delay(20);
}