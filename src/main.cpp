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
#define BUZZER_PIN 2
#define RELAY_PIN 23
#define ACTIVE_PIN 21
#define ERROR_PIN 4
#define BUTTON_PIN 36
#define DEBOUNCE_DELAY 50
#define HOLD_DURATION 10000
PZEM004Tv30 pzem(Serial2, 17, 16);
const char* ssid = "TECNO CAMON";
const char* password = "Johansson001%";
String meterId;
String connectionAuth;
TaskHandle_t Task1, Task2;
TaskHandle_t fetchDataHandle = nullptr;

void recordPower();

float sumVoltage = 0.0, sumCurrent = 0.0, sumPower = 0.0, sumEnergy = 0.0, sumLoad = 0.0;
double powerValue = 0.0, voltageValue = 0.0, currentValue = 0.0, powerFactorValue = 0.0, totalUnits = 0.0, consumedUnits = 0.0;
bool status = true, activeLoad = false, bootModeStatus = false;
MeterData dataFromServer;
void sendDataTask(void* parameter) {
    if (meterId.length() > 0 && connectionAuth.length() > 0 && WiFi.status() == WL_CONNECTED) {
      sendDataToAPI(meterId, connectionAuth, powerValue, voltageValue, currentValue, status);
      //vTaskDelay(10000 / portTICK_PERIOD_MS);
    }
}
// void fetchDataTask(void *parameter) {
//     while(true){
//         if (meterId.length() == 0) {
//             Serial.println("Empty Meter ID");
//         }
//         if (connectionAuth.length() == 0) {
//             Serial.println("Empty Connection Auth");
//         }
//         dataFromServer = fetchDataFromAPI(meterId, connectionAuth);
//         if(dataFromServer.totalUnits != 0.00){
//             bool dataStatus = dataFromServer.status;
//             totalUnits = dataFromServer.totalUnits;
//             consumedUnits = dataFromServer.consumedUnits;
//             activeLoad = dataFromServer.activeLoad;
//         }
//         vTaskDelay(30000 / portTICK_PERIOD_MS);
//   }
// }
void fetchDataTask(void* param) {
    for (;;) {
      if (meterId.isEmpty())    Serial.println("Empty Meter ID");
      if (connectionAuth.isEmpty()) Serial.println("Empty Connection Auth");
  
      auto data = fetchDataFromAPI(meterId, connectionAuth);
      if (data.totalUnits != 0.0) {
        bool dataStatus = dataFromServer.status;
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
        displayData(meterId, voltage, current, power, load, totalUnits - consumedUnits, "Active", "No Messages");
        if (voltage >= 240.0) {
            digitalWrite(RELAY_PIN, LOW);
            triggerBuzzer(true, false);
        } else {
            digitalWrite(RELAY_PIN, HIGH);
        }
        delay(1000);
    }
    // xTaskCreatePinnedToCore(sendDataTask, "SendDataTask", 8192, NULL, 1, NULL, 0);
}
void setup() {
    MeterConfig savedUserData;
    savedUserData = loadMeterData();
    meterId = savedUserData.meterId;
    connectionAuth = savedUserData.connectionAuth;
    MeterInfoSetup();
    if(meterId.length() > 0 && connectionAuth.length() > 0) {
        WiFi.begin(ssid, password);
        delay(7000);
        xTaskCreatePinnedToCore(fetchDataTask,"fetchDataFromAPI", 6144, NULL, 1, &fetchDataHandle, 1);
        
        delay(2000);
    } else {
        bootModeStatus = true;
        Serial.println("Setup Meter for the first time");
        // EnterCredentials();
        bootMode();
        EnterCredentials();
    }
}
void loop() {
    if (bootModeStatus == false){
        ResetButton();
        if(WiFi.status() == WL_CONNECTED) {
            digitalWrite(ACTIVE_PIN, 1);
            digitalWrite(ERROR_PIN, 0);
            displayData(meterId, 0.0, 0.0, 0.0, 0.0, totalUnits - consumedUnits, "Active", "No Messages");
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
    }else{
        digitalWrite(ERROR_PIN, 1);
        delay(1000);
        digitalWrite(ERROR_PIN, 0);
        delay(1000);
    }
}
