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
#include "implt.h"
#define OLED_WIDTH 128
#define OLED_HEIGHT 64
#define BUZZER_PIN 2
#define RELAY_PIN 23
#define ACTIVE_PIN 21
#define ERROR_PIN 4
#define BUTTON_PIN 36
#define DEBOUNCE_DELAY 50
#define HOLD_DURATION 10000


PZEM004Tv30 pzem(Serial2, 17, 16);
RTC_DS3231 rtc;
const char* ssid = "TECNO CAMON";
const char* password = "Johansson001%";
String meterId;
String connectionAuth;
TaskHandle_t Task1, Task2;
void recordPower();
//void displayData(float voltage, float current, float power, float load);
//void triggerBuzzer(bool overload, bool lowUnits);
float sumVoltage = 0.0, sumCurrent = 0.0, sumPower = 0.0, sumEnergy = 0.0, sumLoad = 0.0;
double powerValue = 0.0, voltageValue = 0.0, currentValue = 0.0, powerFactorValue = 0.0, totalUnits = 0.0, consumedUnits = 0.0;
String timeValue = "2023-10-01T12:00:00Z";
bool status = true, activeLoad = false;
MeterData dataFromServer;
void sendDataTask(void* parameter) {
    if (meterId.length() > 0 && connectionAuth.length() > 0 && WiFi.status() == WL_CONNECTED && timeValue != NULL) {
      sendDataToAPI(meterId, connectionAuth, powerValue, voltageValue, currentValue, powerFactorValue, timeValue, status);
      //vTaskDelay(10000 / portTICK_PERIOD_MS);
    }
}
void fetchDataTask(void *parameter) {
    while(true){
        if (meterId.length() == 0) {
            Serial.println("Empty Meter ID");
        }
        if (connectionAuth.length() == 0) {
            Serial.println("Empty Connection Auth");
        }
        dataFromServer = fetchDataFromAPI(meterId, connectionAuth);
        if(dataFromServer.totalUnits != 0.00){
            bool dataStatus = dataFromServer.status;
            totalUnits = dataFromServer.totalUnits;
            consumedUnits = dataFromServer.consumedUnits;
            activeLoad = dataFromServer.activeLoad;
        }
        vTaskDelay(60000 / portTICK_PERIOD_MS);
  }
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
    timeValue = "";
    // xTaskCreatePinnedToCore(sendDataTask, "SendDataTask", 8192, NULL, 1, NULL, 0);e
}
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
void setup() {
    MeterConfig savedUserData;
    savedUserData = loadMeterData();
    meterId = savedUserData.meterId;
    connectionAuth = savedUserData.connectionAuth;
    
    if(meterId.length() > 0 && connectionAuth.length() > 0) {
        // Serial.println(String(meterId));
        // Serial.println(String(connectionAuth) || "Empty Connection Auth");
        // display.clearDisplay();
        // display.setCursor(0, 0);
        // //display.printf(String(meterId) + " is attempting to connect to SEMS");
        // display.display();
        WiFi.begin(ssid, password);
        delay(7000);
        xTaskCreatePinnedToCore(fetchDataTask, "fetchDataFromAPI", 5096, NULL, 1, &Task1, 1);
        delay(2000);
    } else {
        Serial.println("Setup Meter for the first time");
        EnterCredentials();
        Serial.println("✅ Setup complete. Rebooting...");
        digitalWrite(ACTIVE_PIN, HIGH);
        delay(1000);
        digitalWrite(ACTIVE_PIN, LOW);
        ESP.restart();
    }
}

void loop() {
    ResetButton();
    if(WiFi.status() == WL_CONNECTED) {
        digitalWrite(ACTIVE_PIN, 1);
        digitalWrite(ERROR_PIN, 0);
    } else digitalWrite(ERROR_PIN, 1);
    if(totalUnits > consumedUnits){
        if(activeLoad) digitalWrite(RELAY_PIN, HIGH);
        else digitalWrite(RELAY_PIN, LOW);
        if(totalUnits - consumedUnits < 10.00) {
            triggerBuzzer(false, true);
            digitalWrite(ERROR_PIN, HIGH);
        } else triggerBuzzer(false, false);
        //recordPower();
    }else digitalWrite(ERROR_PIN, HIGH);
}
