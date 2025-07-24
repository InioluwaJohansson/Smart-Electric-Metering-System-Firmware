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
#define analogMax 4095
#define vRef 3.3
#define DEBOUNCE_DELAY 50
#define HOLD_DURATION 10000
#include <math.h>

// Voltage Sensor (ZMPT101B)
const int voltagePin = 34;
const float voltageCalibration = 0.0065;  // Adjust based on your setup
const int voltageOffset = 3111;           // Measured with no AC voltage
const float voltageNoiseThreshold = 1.0;  // Filter small/no voltage

// Current Sensor (ACS712)
const int currentPin = 35;
const float currentSensitivity = 0.5;   // For ACS712-5A
const int currentOffset = 2785;           // Measured at no load
const float currentNoiseThreshold = 0.01;// Filter small/no current

const int samples = 1000;
const int averagingInterval = 10; 
PZEM004Tv30 pzem(Serial2, 17, 16);
String meterId;
String connectionAuth;
TaskHandle_t fetchDataHandle = nullptr, sendDataHandle = nullptr;
float sumVoltage = 0.0, sumCurrent = 0.0, sumPower = 0.0;
double powerValue = 0.0, voltageValue = 0.0, currentValue = 0.0, powerFactorValue = 0.0, totalUnits = 0.0, consumedUnits = 0.0;
bool status = true, activeLoad = false, bootModeStatus = false;
volatile bool tamperDetected = false;
String activeLoadStatus = "InActive";
MeterData dataFromServer;
void recordPower();
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
                if(totalUnits - consumedUnits < 30.00) {
                    triggerBuzzer(false, true);
                    digitalWrite(ERROR_PIN, HIGH);
                } else triggerBuzzer(false, false);
            }else digitalWrite(ERROR_PIN, HIGH);
        Serial.println("Total Units: " + String(totalUnits));
        Serial.println("Consumed Units: " + String(consumedUnits)); 
        Serial.println("Active Load: " + String(activeLoad));
    }else {
        totalUnits = 0.00;
        consumedUnits = 0.00;
        activeLoad = false;
    }
    vTaskDelay(pdMS_TO_TICKS(20000));
}
}
float readVoltageRMS() {
  float sumSquares = 0.0;
  for (int i = 0; i < samples; i++) {
    int raw = analogRead(voltagePin);
    float voltage = (raw - voltageOffset) * (3.3 / 4095.0);
    sumSquares += voltage * voltage;
  }
  float rms = sqrt(sumSquares / samples);
  float v = rms / voltageCalibration;
  if (v < voltageNoiseThreshold) v = 0.0;
  return v;
}
float readCurrentRMS() {
  float sumSquares = 0.0;
  for (int i = 0; i < samples; i++) {
    int raw = analogRead(currentPin);
    float voltage = (raw - currentOffset) * (3.3 / 4095.0);
    sumSquares += voltage * voltage;
  }
  float rmsVoltage = sqrt(sumSquares / samples);
  float current = rmsVoltage / currentSensitivity;
  if (current < currentNoiseThreshold) current = 0.0;
  return current;
}

const float currentThreshold = 0.1;     // Ignore currents < 100mA

void recordPower() {
    sumVoltage = 0.0;
    sumCurrent = 0.0;
    sumPower = 0.0;
    float totalVoltage = 0.0;
    float totalCurrent = 0.0;
    int readingCount = 0;
    // Collect data for 10 seconds

  unsigned long startTime = millis();

  // Collect data for 10 seconds
  while (millis() - startTime < averagingInterval * 1000) {
    float voltage = readVoltageRMS();
    float current = readCurrentRMS();

    totalVoltage += voltage;
    totalCurrent += current;
    readingCount++;

    delay(500); // Optional: Reduce load and spacing
  }
    voltageValue = totalVoltage / readingCount;
    currentValue = totalCurrent / readingCount;
    powerValue = voltageValue * currentValue;
    //Convert to kwh after 10 seconds
    powerValue = powerValue / 3600.0; // Convert to kWh
    // Output
    Serial.println("===== 10-Second Averaged Power Reading =====");
    Serial.print("Avg Voltage (V): ");
    Serial.println(voltageValue, 2);
    Serial.print("Avg Current (A): ");
    Serial.println(currentValue, 2);
    Serial.print("Estimated Power (W): ");
    Serial.println(powerValue, 2);
    Serial.println("============================================");
    // Push data to screen and API
    displayData(meterId, sumVoltage, sumCurrent, sumPower, 0, totalUnits - consumedUnits, "Active", "Nil");
    xTaskCreatePinnedToCore(sendDataTask, "SendDataTask", 5192, NULL, 1, &sendDataHandle, 1);
}

void RunMeter() {    
    digitalWrite(ERROR_PIN, 0);
    //ResetButton();
    Serial.println("Active Load: " + activeLoad);
    if(activeLoad) activeLoadStatus = "A";
    else activeLoadStatus = "IA";
    if(totalUnits == 0.0 ){
        displayData(meterId, 0.0, 0.0, 0.0, 0.0, totalUnits - consumedUnits, activeLoadStatus, "UTCS");
    }else{
        displayData(meterId, 0.0, 0.0, 0.0, 0.0, totalUnits - consumedUnits, activeLoadStatus, "Nil");
    }
}
void IRAM_ATTR onTamperInterrupt() {
  tamperDetected = (digitalRead(TAMPER_IN) == LOW); 
}
void setup() {
    analogReadResolution(12); // ESP32 ADC: 0-4095
    delay(1000);
    Serial.println("Starting power monitoring...");
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
        WiFi.begin("TECNO CAMON", "Johansson001%%");
        Serial.println(savedUserData.meterId + " " + savedUserData.connectionAuth);
        xTaskCreatePinnedToCore(fetchDataTask,"fetchDataFromAPI", 6144, NULL, 1, &fetchDataHandle, 1);
        displayTextCenter("Connecting to SEMS", "Server...");
        delay(7000);
        displayClear();
    }else {
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
void ReBoot(){
    digitalWrite(ERROR_PIN, 1);
    //ResetButton();
    WiFi.reconnect();
    displayTextCenter("Connecting To Sems", "Server...");
    delay(5000);
}
void loop() {
    ResetButton();
    //ClearCredentials();
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
    }
    if(bootModeStatus == true){
        const char* ssidBootMode = "SEMS Meter"; const char* passwordBootMode = "SEMSMeter";
        displayWifiText(ssidBootMode, passwordBootMode, String("72.72.72.72"));
    }
}
