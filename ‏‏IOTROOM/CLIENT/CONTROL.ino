#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ESPAsyncTCP.h>
#include <ArduinoJson.h>
#include <DHT.h>

const char* ssid = "Kinneret College";
const char* password = "55555333";
const char* serverAddress = "ws://<LOCK_CONTROLLER_IP>/ws";  // Replace with Lock Controller's IP

AsyncWebSocketClient *wsClient = nullptr;
AsyncWebSocket ws("/ws");

#define LIGHT_PIN D1
#define PWM_MAX 1023
bool lightPuzzleSolved = false;

#define DHT_PIN D2
#define DHT_TYPE DHT22
DHT dht(DHT_PIN, DHT_TYPE);
float initialTemp = 0;
bool tempPuzzleSolved = false;

#define LED1 D3
#define LED2 D4
#define LED3 D5
#define LED4 D6
#define BUTTON1 D7
#define BUTTON2 D8
#define BUTTON3 D9
#define BUTTON4 D10
int ledSequence[4];
int userSequence[4];
int currentStep = 0;
bool sequencePuzzleSolved = false;

#define JOYSTICK_X A0
#define JOYSTICK_Y A1
bool joystickPuzzleSolved = false;

void sendPuzzleSolved(int puzzleNumber, String digit) {
    if (wsClient) {
        StaticJsonDocument<200> doc;
        doc["puzzle"] = puzzleNumber;
        doc["digit"] = digit;
        String message;
        serializeJson(doc, message);
        wsClient->text(message);
    }
}
void onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
                      AwsEventType type, void *arg, uint8_t *data, size_t len) {
    if (type == WS_EVT_CONNECT) {
        wsClient = client;
        Serial.println("Connected to Lock Controller!");
    }
}

void solveLightPuzzle() {
    if (!lightPuzzleSolved) {
        analogWrite(LIGHT_PIN, PWM_MAX * 0.8); 
        delay(2000);
        analogWrite(LIGHT_PIN, PWM_MAX); 
        lightPuzzleSolved = true;
        Serial.println("Puzzle 1 Solved!");
        sendPuzzleSolved(1, "5");  
    }
}
void solveTempPuzzle() {
    float currentTemp = dht.readTemperature();
    if (!tempPuzzleSolved && currentTemp <= initialTemp - 2) {
        tempPuzzleSolved = true;
        Serial.println("Puzzle 2 Solved!");
        sendPuzzleSolved(2, "2"); 
    }
}

void flashRandomSequence() {
    randomSeed(analogRead(0));
    for (int i = 0; i < 4; i++) {
        ledSequence[i] = random(1, 5); 
    }
}

void checkButtonPress(int buttonPressed) {
    if (buttonPressed == ledSequence[currentStep]) {
        userSequence[currentStep] = buttonPressed;
        currentStep++;
        if (currentStep == 4) {  
            sequencePuzzleSolved = true;
            Serial.println("Puzzle 3 Solved!");
            sendPuzzleSolved(3, "7");  
        }
    } else {
        currentStep = 0;  
    }
}

void solveJoystickPuzzle() {
    int x = analogRead(JOYSTICK_X);
    int y = analogRead(JOYSTICK_Y);
    
    if (!joystickPuzzleSolved) {
        if ((x < 100 && y < 100) || (x > 900 && y < 100) || (x < 100 && y > 900) || (x > 900 && y > 900)) {
            delay(2000);  
            if ((x < 100 && y < 100) || (x > 900 && y < 100) || (x < 100 && y > 900) || (x > 900 && y > 900)) {
                joystickPuzzleSolved = true;
                Serial.println("Puzzle 4 Solved!");
                sendPuzzleSolved(4, "9");  
            }
        }
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

    pinMode(LIGHT_PIN, OUTPUT);
    pinMode(LED1, OUTPUT);
    pinMode(LED2, OUTPUT);
    pinMode(LED3, OUTPUT);
    pinMode(LED4, OUTPUT);
    pinMode(BUTTON1, INPUT_PULLUP);
    pinMode(BUTTON2, INPUT_PULLUP);
    pinMode(BUTTON3, INPUT_PULLUP);
    pinMode(BUTTON4, INPUT_PULLUP);
    dht.begin();
    initialTemp = dht.readTemperature();
    ws.onEvent(onWebSocketEvent);
    ws.connect(serverAddress);

    flashRandomSequence();  
}
void loop() {
    solveLightPuzzle();  
    solveTempPuzzle();   
    solveJoystickPuzzle();  
    if (digitalRead(BUTTON1) == LOW) checkButtonPress(1);
    if (digitalRead(BUTTON2) == LOW) checkButtonPress(2);
    if (digitalRead(BUTTON3) == LOW) checkButtonPress(3);
    if (digitalRead(BUTTON4) == LOW) checkButtonPress(4);

    ws.cleanupClients();
}
