#include "auth.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <Adafruit_GFX.h>
#include <LiquidCrystal_I2C.h>
#include <data.h>
#define I2C_SDA 21
#define I2C_SCL 22
LiquidCrystal_I2C lcd(0x27, 20, 4);
int BUZZER_PIN = 19, RELAY_PIN = 23, ACTIVE_PIN = 15, RELAY_LED = 2, ERROR_PIN = 18, BUTTON_PIN = 36, TAMPER_IN = 39;
unsigned long DEBOUNCE_DELAY = 50;
unsigned long HOLD_DURATION = 7000;
unsigned long lastDebounceTime = 0;
unsigned long buttonHoldStart = 0;
bool lastButtonState = HIGH;
bool buttonPressed = false;
bool clearingTriggered = false;
void displayTextCenter(String message, String message2);
void MeterInfoSetup(){
    pinMode(RELAY_PIN, OUTPUT);
    pinMode(RELAY_LED, OUTPUT);
    pinMode(ACTIVE_PIN, OUTPUT);
    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(ERROR_PIN, OUTPUT);
    pinMode(BUTTON_PIN, INPUT);
    pinMode(TAMPER_IN, INPUT_PULLUP);
    digitalWrite(RELAY_PIN, LOW);
    digitalWrite(RELAY_LED, LOW);
    digitalWrite(ACTIVE_PIN, HIGH);
    digitalWrite(BUZZER_PIN, HIGH);
    digitalWrite(ERROR_PIN, HIGH);
    delay(3000);
    digitalWrite(BUZZER_PIN, LOW);
    digitalWrite(ERROR_PIN, LOW);
    digitalWrite(ACTIVE_PIN, HIGH);
    Wire.begin(I2C_SDA, I2C_SCL);
    lcd.begin(16, 4);
    lcd.backlight();
    Serial.begin(9600);
}
void displayData(String meterId, float voltage, float current, float power, float load, double units, String operatingStatus, String messages) {
    lcd.setCursor(0, 0);
    lcd.print(meterId);
    lcd.setCursor(0, 1);
    lcd.print(String(units) + "kWh" + " V:" + String(voltage) + "V");
    lcd.setCursor(0, 2);
    lcd.print("P:" + String(power) + "kW" + " L:" + String(load) + "kW");
    lcd.setCursor(0, 3);
    lcd.print("LS:" + String(operatingStatus) + " Msg: " + String(messages));
}
void displayTextCenter(String message, String message2) {
    lcd.clear();
    lcd.setCursor(0, 1);
    lcd.print(String(message));
    lcd.setCursor(0, 2);
    lcd.print(String(message2));
}
void displayWifiText(String message, String message2, String IpAddress) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(String(message));
    lcd.setCursor(0, 1);
    lcd.print("Password:" + String(message2));
    lcd.setCursor(0, 2);
    lcd.print("IP:" + String(IpAddress));
    lcd.setCursor(0, 3);
    lcd.print("Enter IP in Browser");
}
void displayClear(){
    lcd.clear();
    delay(2000);
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
                for(int i = 5; i > 0; i--) {
                    displayTextCenter("Rebooting in","Setup Mode in " + String(i));
                    delay(1000);
                }
                ESP.restart();
            }
        } else {
        if (buttonPressed && !clearingTriggered) {
            digitalWrite(ERROR_PIN, LOW);
            Serial.println("Button released before 7 seconds. Aborting clear.");
        }
        buttonPressed = false;
        }
    }
    lastButtonState = reading;
}
