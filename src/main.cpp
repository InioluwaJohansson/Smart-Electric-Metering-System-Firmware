#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <PZEM004Tv30.h>
#include "data.h"
#include "auth.h"
#include "implt.h"
#include "setupmode.h"
#define BUZZER_PIN 19
#define RELAY_PIN 23
#define ACTIVE_PIN 15
#define RELAY_LED 2
#define ERROR_PIN 18
#define BUTTON_PIN 36
#define TAMPER_IN 39
#define DEBOUNCE_DELAY 50
#define HOLD_DURATION 10000
PZEM004Tv30 pzem(Serial2, 17, 16);
const char* ssid = "TECNO CAMON";
const char* password = "Johansson001%%";
String meterId;
String connectionAuth;
TaskHandle_t fetchDataHandle = nullptr, sendDataHandle = nullptr;
float sumVoltage = 0.0, sumCurrent = 0.0, sumPower = 0.0;
double powerValue = 0.0, voltageValue = 0.0, currentValue = 0.0, powerFactorValue = 0.0, totalUnits = 0.0, consumedUnits = 0.0;
bool status = true, activeLoad = false, bootModeStatus = false;
volatile bool tamperDetected = false;
String activeLoadStatus = "InActive";
MeterData dataFromServer;
void sendDataTask(void* param) {
    if (meterId.length() > 0 && connectionAuth.length() > 0 && WiFi.status() == WL_CONNECTED) {
        sendDataToAPI(meterId, connectionAuth, powerValue, voltageValue, currentValue, status);
    }
    vTaskDelete(NULL);
}
void fetchDataTask(void* param) {
    for (;;) {
        if (meterId.isEmpty()) Serial.println("Empty Meter ID");
        if (connectionAuth.isEmpty()) Serial.println("Empty Connection Auth");
        auto data = fetchDataFromAPI(meterId, connectionAuth);
        if (data.totalUnits != 0.0) {
            bool dataStatus = data.status;
            Serial.println("Data Status: " + String(dataStatus));
        totalUnits = data.totalUnits;
        consumedUnits = data.consumedUnits;
        activeLoad = data.activeLoad;
    }else {
        totalUnits = 0.00;
        consumedUnits = 0.00;
        activeLoad = false;
    }
    vTaskDelay(pdMS_TO_TICKS(30000));
}
}
void recordPower() {
    sumVoltage = sumCurrent = sumPower = 0.0;
    for (int i = 1; i <= 10; i++) {
        sumVoltage += pzem.voltage();
        sumCurrent += pzem.current();
        sumPower += pzem.energy();
        if (pzem.voltage() >= 240.0) {
            digitalWrite(RELAY_PIN, LOW);
            digitalWrite(RELAY_LED, LOW);
            triggerBuzzer(true, false);
        } else {
            digitalWrite(RELAY_PIN, HIGH);
            digitalWrite(RELAY_LED, HIGH);
        }
        delay(1000);
    }
    sumVoltage /= 10;
    sumCurrent /= 10;
    sumPower /= 10;
    displayData(meterId, (sumVoltage/10, 0), (sumCurrent/10, 2), (sumPower/10, 1), (pzem.power()/pzem.voltage(),1), (totalUnits - consumedUnits, 1), "Active", "Nil");
    xTaskCreatePinnedToCore(sendDataTask, "SendDataTask", 5192, NULL, 1, &sendDataHandle, 1);
}
void RunMeter() {    
    digitalWrite(ERROR_PIN, 0);
    ResetButton();
    if(activeLoad) activeLoadStatus = "Active";
    else activeLoadStatus = "InActive";
    if(totalUnits == 0.0 ){
        displayData(meterId, 0.0, 0.0, 0.0, 0.0, totalUnits - consumedUnits, activeLoadStatus, "UTCS");
    }else{
        displayData(meterId, 0.0, 0.0, 0.0, 0.0, totalUnits - consumedUnits, activeLoadStatus, "Nil");
    }
    if(totalUnits > consumedUnits){
        if(activeLoad == true) {
            digitalWrite(RELAY_PIN, HIGH);
            digitalWrite(RELAY_LED, HIGH);
            recordPower();
        }
        else{
            digitalWrite(RELAY_PIN, LOW);
            digitalWrite(RELAY_LED, LOW);
        }
        if(totalUnits - consumedUnits < 10.00) {
            triggerBuzzer(false, true);
            digitalWrite(ERROR_PIN, HIGH);
        } else triggerBuzzer(false, false);
    }else digitalWrite(ERROR_PIN, HIGH);
}
void IRAM_ATTR onTamperInterrupt() {
  tamperDetected = (digitalRead(TAMPER_IN) == LOW); 
}
void setup() {
    MeterInfoSetup();
    attachInterrupt(digitalPinToInterrupt(TAMPER_IN), onTamperInterrupt, CHANGE);
    tamperDetected = (digitalRead(TAMPER_IN) == LOW);
    displayTextCenter("SEMS METER", "");
    delay(2000);
    displayTextCenter("Checking", "Credentials");
    delay(2000);
    MeterConfig savedUserData;
    savedUserData = loadMeterData();
    meterId = savedUserData.meterId;
    connectionAuth = savedUserData.connectionAuth;
    if(meterId.length() > 0 && connectionAuth.length() > 0) {
        WiFi.begin(savedUserData.wifiName, savedUserData.wifiPassword);
        xTaskCreatePinnedToCore(fetchDataTask,"fetchDataFromAPI", 6144, NULL, 1, &fetchDataHandle, 1);
        displayTextCenter("Connecting to SEMS", "Server...");
        delay(7000);
        displayClear();
    } else {
        bootModeStatus = true;
        displayTextCenter("Entering", "Setup Mode");
        delay(2000);
        Serial.println("Setup Meter for the first time");
        digitalWrite(ACTIVE_PIN, HIGH);
        digitalWrite(ERROR_PIN, HIGH);
        bootMode();
        EnterCredentials();
    }
}

void loop() {
    if(WiFi.status() == WL_CONNECTED) {
        if (bootModeStatus == false && tamperDetected == false) {
            RunMeter();
        }else if(tamperDetected) {
            displayTextCenter("Tamper Detected!", "Cover Meter");
            delay(2000); 
        }else{
            digitalWrite(ERROR_PIN, 1);
            delay(1000);
            digitalWrite(ERROR_PIN, 0);
            delay(1000);
        }
    }else{ 
        digitalWrite(ERROR_PIN, 1);
        WiFi.reconnect();
        displayTextCenter("Connecting To Sems", "Server...");
        delay(5000);
    }
}
