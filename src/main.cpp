/**
 * ----------------------------------------------------------------------------
 * ESP32 Remote Control with WebSocket
 * ----------------------------------------------------------------------------
 * © 2020 Stéphane Calderoni
 * ----------------------------------------------------------------------------
 */

#include <Arduino.h>
#include <SPIFFS.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

#include <AsyncElegantOTA.h>

#include <ezButton.h>
#include "main_include.h"
// ----------------------------------------------------------------------------
// Definition of macros
// ----------------------------------------------------------------------------

// #define LED_PIN   26
// #define BTN_PIN   22
#define HTTP_PORT 80

// ----------------------------------------------------------------------------
// Definition of global constants
// ----------------------------------------------------------------------------

// Button debouncing
const uint8_t DEBOUNCE_DELAY = 10; // in milliseconds

// WiFi credentials
const char *WIFI_SSID = "neuf_B1B1_2_4GHz";
const char *WIFI_PASS = "QqqSssDddFff";

// ----------------------------------------------------------------------------
// Definition of the LED component
// ----------------------------------------------------------------------------

struct Led {
    // state variables
    uint8_t pin;
    bool    on;

    // methods
    void update() {
        digitalWrite(pin, on ? LOW : HIGH);
    }
};

// ----------------------------------------------------------------------------
// Definition of the Button component
// ----------------------------------------------------------------------------
/*
struct Button {
    // state variables
    uint8_t  pin;
    bool     lastReading;
    uint32_t lastDebounceTime;
    uint16_t state;

    // methods determining the logical state of the button
    bool pressed()                { return state == 1; }
    bool released()               { return state == 0xffff; }
    bool held(uint16_t count = 0) { return state > 1 + count && state < 0xffff; }

    // method for reading the physical state of the button
    void read() {
        // reads the voltage on the pin connected to the button
        bool reading = digitalRead(pin);

        // if the logic level has changed since the last reading,
        // we reset the timer which counts down the necessary time
        // beyond which we can consider that the bouncing effect
        // has passed.
        if (reading != lastReading) {
            lastDebounceTime = millis();
        }

        // from the moment we're out of the bouncing phase
        // the actual status of the button can be determined
        if (millis() - lastDebounceTime > DEBOUNCE_DELAY) {
            // don't forget that the read pin is pulled-up
            bool pressed = reading == LOW;
            if (pressed) {
                     if (state  < 0xfffe) state++;
                else if (state == 0xfffe) state = 2;
            } else if (state) {
                state = state == 0xffff ? 0 : 0xffff;
            }
        }

        // finally, each new reading is saved
        lastReading = reading;
    }
};*/

// ----------------------------------------------------------------------------
// Definition of global variables
// ----------------------------------------------------------------------------
uint8_t onboard_led = LED_BUILTIN2;
uint8_t led = LED_PIN; 
ezButton wsButton  = { BTN_PIN, INPUT_PULLUP };
AsyncWebServer server(HTTP_PORT);
AsyncWebSocket ws("/ws");

// ----------------------------------------------------------------------------
// SPIFFS initialization
// ----------------------------------------------------------------------------

void initSPIFFS() {
  if (!SPIFFS.begin(true)) {
    Serial.println("Cannot mount SPIFFS volume...");
    while (1) {
        digitalWrite(onboard_led, (millis() % 200 < 50));
    }
  }
}

// ----------------------------------------------------------------------------
// Connecting to the WiFi network
// ----------------------------------------------------------------------------

void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.printf("Trying to connect [%s] ", WiFi.macAddress().c_str());
  while (WiFi.status() != WL_CONNECTED) {
      Serial.print(".");
      delay(500);
  }
  Serial.printf(" %s\n", WiFi.localIP().toString().c_str());
}

// ----------------------------------------------------------------------------
// Web server initialization
// ----------------------------------------------------------------------------

String processor(const String &var) {
    return String(var == "STATE" && digitalRead(led) ? "on" : "off");
}

void onRootRequest(AsyncWebServerRequest *request) {
  request->send(SPIFFS, "/index.html", "text/html", false, processor);
}

void initWebServer() {
    server.on("/", onRootRequest);
    server.serveStatic("/", SPIFFS, "/");
    AsyncElegantOTA.begin(&server);
    server.begin();

}

// ----------------------------------------------------------------------------
// WebSocket initialization
// ----------------------------------------------------------------------------

void notifyClients() {
    const uint8_t size = JSON_OBJECT_SIZE(1);
    StaticJsonDocument<size> json;
    json["status"] = digitalRead(led) ? "on" : "off";

    char buffer[17];
    size_t len = serializeJson(json, buffer);
    ws.textAll(buffer, len);
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
    AwsFrameInfo *info = (AwsFrameInfo*)arg;
    if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {

        const uint8_t size = JSON_OBJECT_SIZE(1);
        StaticJsonDocument<size> json;
        DeserializationError err = deserializeJson(json, data);
        if (err) {
            Serial.print(F("deserializeJson() failed with code "));
            Serial.println(err.c_str());
            return;
        }

        const char *action = json["action"];
        if (strcmp(action, "toggle") == 0) {
            digitalWrite(led, !digitalRead(led));
            notifyClients();
        }

    }
}

void onEvent(AsyncWebSocket       *server,
             AsyncWebSocketClient *client,
             AwsEventType          type,
             void                 *arg,
             uint8_t              *data,
             size_t                len) {

    switch (type) {
        case WS_EVT_CONNECT:
            Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
            break;
        case WS_EVT_DISCONNECT:
            Serial.printf("WebSocket client #%u disconnected\n", client->id());
            break;
        case WS_EVT_DATA:
            handleWebSocketMessage(arg, data, len);
            break;
        case WS_EVT_PONG:
        case WS_EVT_ERROR:
            break;
    }
}

void initWebSocket() {
    ws.onEvent(onEvent);
    server.addHandler(&ws);
}

// ----------------------------------------------------------------------------
// Initialization
// ----------------------------------------------------------------------------

void setup() {
    pinMode(onboard_led, OUTPUT);
    pinMode(led,         OUTPUT);

    Serial.begin(115200); delay(500);

    
    initSPIFFS();
    initWiFi();
    initWebSocket();
    initWebServer();
}

// ----------------------------------------------------------------------------
// Main control loop
// ----------------------------------------------------------------------------

void loop() {
    ws.cleanupClients();

    wsButton.loop();
    if (wsButton.isPressed()) {
        digitalWrite(led, !digitalRead(led));
        notifyClients();
    }
    
     digitalWrite(onboard_led, (millis() % 1000 < 50)) ;


}