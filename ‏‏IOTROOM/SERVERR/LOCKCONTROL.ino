#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <Wire.h>

const char* ssid = "Kinneret College";
const char* password = "55555333";

String secretCode = "1234";
String currentCode = "____";  

#define LOCK_PIN  D1  
#define SEGMENT_PIN D2 

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");  

void updateSegmentDisplay() {
    Serial.print("Displaying Code: ");
    Serial.println(currentCode);
}

void unlockDoor() {
    digitalWrite(LOCK_PIN, HIGH);  
    delay(5000);                  
    digitalWrite(LOCK_PIN, LOW);   
    Serial.println("Door Unlocked!");
}

void onWebSocketMessage(void *arg, uint8_t *data, size_t len) {
    AwsFrameInfo *info = (AwsFrameInfo*)arg;
    if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
        String message = "";
        for (size_t i = 0; i < len; i++) {
            message += (char)data[i];
        }
        Serial.println("Received WebSocket message: " + message);

        StaticJsonDocument<200> doc;
        DeserializationError error = deserializeJson(doc, message);
        if (!error) {
            int puzzle = doc["puzzle"];
            String digit = doc["digit"];

            if (puzzle >= 1 && puzzle <= 4 && digit.length() == 1) {
                currentCode[puzzle - 1] = digit[0]; 
                updateSegmentDisplay();
                ws.textAll(currentCode); 
            }
        }
    }
}

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>Pool Room Lock</title>
    <script>
        var socket = new WebSocket("ws://" + location.host + "/ws");
        socket.onmessage = function(event) {
            document.getElementById("codeDisplay").innerText = event.data;
        };

        function submitPassword() {
            var enteredCode = document.getElementById("password").value;
            fetch('/unlock?code=' + enteredCode)
                .then(response => response.text())
                .then(data => alert(data));
        }
    </script>
</head>
<body>
    <h1>Enter Password to Unlock</h1>
    <p>Current Code: <span id="codeDisplay">____</span></p>
    <input type="text" id="password" maxlength="4">
    <button onclick="submitPassword()">Submit</button>
</body>
</html>
)rawliteral";

void handleUnlock(AsyncWebServerRequest *request) {
    if (request->hasParam("code")) {
        String enteredCode = request->getParam("code")->value();
        if (enteredCode == secretCode) {
            unlockDoor();
            request->send(200, "text/plain", "Door Unlocked!");
        } else {
            request->send(403, "text/plain", "Incorrect Code!");
        }
    } else {
        request->send(400, "text/plain", "Missing Code!");
    }
}

void setup() {
    Serial.begin(115200);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nConnected to WiFi!");

    pinMode(LOCK_PIN, OUTPUT);
    digitalWrite(LOCK_PIN, LOW);  

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send_P(200, "text/html", index_html);
    });

    server.on("/unlock", HTTP_GET, handleUnlock);

    ws.onEvent([](AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
                  void *arg, uint8_t *data, size_t len) {
        if (type == WS_EVT_CONNECT) {
            client->text(currentCode); 
        } else if (type == WS_EVT_DATA) {
            onWebSocketMessage(arg, data, len);
        }
    });

    server.addHandler(&ws);

    server.begin();
    Serial.println("Server Started!");
}

void loop() {
    ws.cleanupClients();  
}
