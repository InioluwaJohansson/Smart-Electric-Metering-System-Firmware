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
#include "data.h"
#include "auth.h"
#define OLED_WIDTH 128
#define OLED_HEIGHT 64
#define BUZZER_PIN 15
#define RELAY_PIN 16
#define LED_PIN1 4
#define LED_PIN2 2
Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);
PZEM004Tv30 pzem(Serial2, 17, 18);
RTC_DS3231 rtc;
const char* ssid = "TECNO CAMON";
const char* password = "Johansson001%";
const char* meterId = "";
const char* connectionAuth;
TaskHandle_t Task1, Task2;
void recordPower();
//void displayData(float voltage, float current, float power, float load);
//void triggerBuzzer(bool overload, bool lowUnits);
float sumVoltage = 0.0, sumCurrent = 0.0, sumPower = 0.0, sumEnergy = 0.0, sumLoad = 0.0;
double powerValue = 0.0, voltageValue = 0.0, currentValue = 0.0, powerFactorValue = 0.0, totalUnits = 0.0, consumedUnits = 0.0;
String timeValue = "2023-10-01T12:00:00Z";
bool status = true, meterData = false, activeLoad = false;
StaticJsonDocument<256> dataFromServer;
void sendDataTask(void* parameter) {
    while (String(meterId).length() > 0 && String(connectionAuth).length() > 0 && WiFi.status() == WL_CONNECTED) {
      sendDataToAPI(meterId, connectionAuth, powerValue, voltageValue, currentValue, powerFactorValue, timeValue, status);
      vTaskDelay(10000 / portTICK_PERIOD_MS);
    }
}
void fetchDataTask(void* parameter) {
  while (String(meterId).length() > 0 && String(connectionAuth).length() > 0 && WiFi.status() == WL_CONNECTED) {
    Serial.println(meterId);
    Serial.println(connectionAuth);
    Serial.println("Meter Id: " + String(meterId));
    Serial.println("Connection Auth: " + String(connectionAuth));
    // dataFromServer = fetchDataFromAPI(meterId, connectionAuth);
    totalUnits = dataFromServer["totalUnits"] | 0;
    consumedUnits = dataFromServer["consumedUnits"] | 0;
    activeLoad = dataFromServer["activeLoad"] | false;
    vTaskDelay(60000 / portTICK_PERIOD_MS);
  }
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
void displayData(String meterId, float voltage, float current, float power, float load, double units) {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.printf("V: %.2fV\n", voltage);
    display.printf("I: %.2fA\n", current);
    display.printf("P: %.2fW\n", power);
    display.printf("L: %.2fΩ\n", load);
    display.display();
}
void recordPower() {
    sumVoltage = sumCurrent = sumPower = sumLoad = 0.0;
    
    for (int i = 1; i <= 10; i++) {
        float voltage = pzem.voltage();
        float current = pzem.current();
        float power = pzem.power();
        float energy = pzem.energy();
        float load = power / (voltage > 0 ? voltage : 1);
        sumVoltage += voltage;
        sumCurrent += current;
        sumPower += power;
        sumEnergy += energy;
        sumLoad += load;

        displayData(meterId, voltage, current, power, load, totalUnits - consumedUnits);

        if (voltage >= 240.0) {
            digitalWrite(RELAY_PIN, LOW);
            triggerBuzzer(true, false);
        } else {
            digitalWrite(RELAY_PIN, HIGH);
        }
        delay(1000);
    }
}

void setup() {
    StaticJsonDocument<256> savedUserData = loadMeterData();
    meterId = savedUserData["meterId"];
    connectionAuth = savedUserData["connectionAuth"];
    Serial.begin(9600);
    pinMode(LED_PIN1, OUTPUT);
    pinMode(LED_PIN2, OUTPUT);
    digitalWrite(LED_PIN1, LOW);
    digitalWrite(LED_PIN2, LOW);
    // Serial.println("Meter Id: " + String(meterId));
    // Serial.println("Connection Auth: " + String(connectionAuth));
    if(String(meterId).length() > 0 && String(connectionAuth).length() > 0) {
        meterData = true;
        // vTaskDelete(NULL);
        //ClearCredentials();
        digitalWrite(LED_PIN1, HIGH);
        delay(2000);
        digitalWrite(LED_PIN1, LOW);
        delay(2000);
        digitalWrite(LED_PIN1, HIGH);
        delay(2000);
        digitalWrite(LED_PIN1, LOW);
        display.clearDisplay();
        display.setCursor(0, 0);
        //display.printf(String(meterId) + " is attempting to connect to SEMS");
        display.display();
        WiFi.begin(ssid, password);
        xTaskCreatePinnedToCore(fetchDataTask, "sendDataToAPI", 10000, NULL, 0, &Task1, 1);
        //xTaskCreate(fetchDataTask, "FetchDataTask", 10000, NULL, 1, NULL);
    } else {
        Serial.println("Setup Meter for the first time");
        EnterCredentials();
        Serial.println("✅ Setup complete. Rebooting...");
        delay(2000);
        digitalWrite(LED_PIN2, HIGH);
        delay(2000);
        digitalWrite(LED_PIN2, LOW);
        ESP.restart();
    }
}

void loop() {
    // xTaskCreatePinnedToCore(sendDataTask, "SendDataTask", 8192, NULL, 1, NULL, 0);e
    Serial.println(totalUnits - consumedUnits);
    if(totalUnits > consumedUnits){
        //recordPower();
        if(totalUnits - consumedUnits < 10) {
            triggerBuzzer(false, true);
        } else {
            triggerBuzzer(false, false);
        }
    }

}
