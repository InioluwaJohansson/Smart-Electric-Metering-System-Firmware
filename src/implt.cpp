#include "auth.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <Adafruit_GFX.h>
#include <LiquidCrystal_I2C.h>
#include <data.h>
LiquidCrystal_I2C lcd(0x27, 16, 4);
int BUZZER_PIN = 2, RELAY_PIN = 23, ACTIVE_PIN = 21, ERROR_PIN = 4, BUTTON_PIN = 36;
unsigned long DEBOUNCE_DELAY = 50;
unsigned long HOLD_DURATION = 7000;
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
    digitalWrite(RELAY_PIN, LOW);
    digitalWrite(ACTIVE_PIN, HIGH);
    digitalWrite(BUZZER_PIN, HIGH);
    digitalWrite(ERROR_PIN, HIGH);
    delay(3000);
    digitalWrite(BUZZER_PIN, LOW);
    digitalWrite(ERROR_PIN, LOW);
    digitalWrite(ACTIVE_PIN, LOW);
    //lcd.begin(16, 4);
    lcd.backlight();
    Serial.begin(9600);
}
void ResetButton(){
    int reading = digitalRead(BUTTON_PIN);
    if (reading != lastButtonState) {
        lastDebounceTime = millis();
    }
    if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY) {
        if (reading == HIGH) {
            digitalWrite(ERROR_PIN, HIGH);
            if (!buttonPressed) {
                buttonPressed = true;
                buttonHoldStart = millis();
                clearingTriggered = false;
                Serial.println("Button press detected. Holding...");
            } else if (!clearingTriggered && (millis() - buttonHoldStart >= HOLD_DURATION)) {
                Serial.println("Button held for 7 seconds. Clearing preferences...");
                ClearCredentials();
                clearingTriggered = true;
                //displayTextCenter("Rebooting in Setup Mode in 5");
                //delay(1000);
                //displayTextCenter("Rebooting in Setup Mode in 4");
                //delay(1000);
                //displayTextCenter("Rebooting in Setup Mode in 3");
                //delay(1000);
                //displayTextCenter("Rebooting in Setup Mode in 2");
                //delay(1000);
                //displayTextCenter("Rebooting in Setup Mode in 1");
                //delay(1000);
                ESP.restart();
            }
        } else {
        if (buttonPressed && !clearingTriggered) {
            digitalWrite(ERROR_PIN, LOW);
            Serial.println("Button released before 10 seconds. Aborting clear.");
        }
        buttonPressed = false;
        }
    }
    lastButtonState = reading;
}
void displayData(String meterId, float voltage, float current, float power, float load, double units, String operatingStatus, String messages) {
    lcd.setCursor(0, 0);
    lcd.print("SEMS: " + meterId + "     " + String(units) + "kWh");
    lcd.setCursor(0, 1);
    lcd.print("V: " + String(voltage) + "V" + "   I: " + String(current) + "A" + "   P: " + String(power) + "W" + "   L: " + String(load) + "Ω");
    lcd.setCursor(0, 2);
    lcd.print("Operating Status: " + String(operatingStatus));
    lcd.setCursor(0, 3);
    lcd.print("Messages: " + String(messages));
    delay(2000);
}
void displayTextCenter(String message) {
    lcd.setCursor(0, 1);
    
}
void triggerBuzzer(bool overload, bool lowUnits) {
    if (overload) {
        for (int i = 0; i < 3; i++) {
            digitalWrite(ERROR_PIN, HIGH);
            digitalWrite(BUZZER_PIN, HIGH);
            delay(100);
            digitalWrite(ERROR_PIN, LOW);
            digitalWrite(BUZZER_PIN, LOW);
            delay(200);
        }
    } else if (lowUnits) {
        for (int i = 0; i < 2; i++) {
            digitalWrite(ERROR_PIN, HIGH);
            digitalWrite(BUZZER_PIN, HIGH);
            delay(500);
            digitalWrite(ERROR_PIN, LOW);
            digitalWrite(BUZZER_PIN, LOW);
            delay(500);
        }
    }
}