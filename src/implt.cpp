#include "auth.h"
#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <PZEM004Tv30.h>
#include <RTClib.h>
#include <data.h>

int OLED_WIDTH = 128, OLED_HEIGHT = 64;
int BUZZER_PIN = 2, RELAY_PIN = 23, ACTIVE_PIN = 21, ERROR_PIN = 4, BUTTON_PIN = 36;
unsigned long DEBOUNCE_DELAY = 50;
unsigned long HOLD_DURATION = 10000;

Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);
unsigned long lastDebounceTime = 0;
unsigned long buttonHoldStart = 0;
bool lastButtonState = HIGH;
bool buttonPressed = false;
bool clearingTriggered = false;
void MeterInfoSetup(){
    pinMode(RELAY_PIN, OUTPUT);
    pinMode(ACTIVE_PIN, OUTPUT);
    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(ERROR_PIN, OUTPUT);
    pinMode(BUTTON_PIN, INPUT); 
    digitalWrite(ACTIVE_PIN, HIGH);
    digitalWrite(BUZZER_PIN, HIGH);
    digitalWrite(ERROR_PIN, HIGH);
    delay(3000);
    digitalWrite(BUZZER_PIN, LOW);
    digitalWrite(ERROR_PIN, LOW);
    digitalWrite(ACTIVE_PIN, LOW);
    Serial.begin(9600);
}
void ResetButton(){
    int reading = digitalRead(BUTTON_PIN);
    if (reading != lastButtonState) {
        lastDebounceTime = millis();
    }
    if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY) {
        if (reading == HIGH) {
            if (!buttonPressed) {
                buttonPressed = true;
                buttonHoldStart = millis();
                clearingTriggered = false;
                Serial.println("Button press detected. Holding...");
            } else if (!clearingTriggered && (millis() - buttonHoldStart >= HOLD_DURATION)) {
                Serial.println("Button held for 10 seconds. Clearing preferences...");
                ClearCredentials();
                clearingTriggered = true;
                ESP.restart();
            }
        } else {
        if (buttonPressed && !clearingTriggered) {
            Serial.println("Button released before 10 seconds. Aborting clear.");
        }
        buttonPressed = false;
        }
    }
    lastButtonState = reading;
}
void displayData(String meterId, float voltage, float current, float power, float load, double units) {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.print("SEMS: " + meterId);
    display.printf("V: %.2fV\n", voltage);
    display.printf("I: %.2fA\n", current);
    display.printf("P: %.2fW\n", power);
    display.printf("L: %.2fΩ\n", load);
    display.display();
}
void triggerBuzzer(bool overload, bool lowUnits) {
    if (overload) {
        for (int j = 0; j < 3; j++) {
            digitalWrite(BUZZER_PIN, HIGH);
            delay(100);
            digitalWrite(BUZZER_PIN, LOW);
            delay(200);
        }
    } else if (lowUnits) {
        for (int j = 0; j < 2; j++) {
            digitalWrite(BUZZER_PIN, HIGH);
            delay(500);
            digitalWrite(BUZZER_PIN, LOW);
            delay(500);
        }
    }
}