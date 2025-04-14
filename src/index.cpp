// #include <WiFi.h>
// #include <HTTPClient.h>
// #include <ArduinoJson.h>
// #include <Wire.h>
// #include <Adafruit_GFX.h>
// #include <Adafruit_SSD1306.h>
// #include <PZEM004Tv30.h>
// #include <RTClib.h>
// #include <TaskScheduler.h>
// #include "api.h"
// #define OLED_WIDTH 128
// #define OLED_HEIGHT 64
// #define BUZZER_PIN 15
// #define RELAY_PIN 16

// const char* ssid = "TECNO CAMON";
// const char* password = "Johansson001%";
// const char* apiEndpoint = "http://localhost/5191/SendMeterUnits";
// const char* fetchEndpoint = "http://localhost/5191/FetchMeterUnits";
// const char* meterId = "YOUR_METER_ID";
// const char* connectionAuth = "YOUR_CONNECTION_AUTH";

// Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);
// PZEM004Tv30 pzem(Serial2, 17, 18);
// RTC_DS3231 rtc;

// void recordPower();
// void displayData(float voltage, float current, float power, float load);
// void triggerBuzzer(bool overload, bool lowUnits);

// float sumVoltage = 0.0, sumCurrent = 0.0, sumPower = 0.0, sumEnergy = 0.0, sumLoad = 0.0;
// int samples = 10;

// Scheduler scheduler;
// Task taskSendData(10000, TASK_FOREVER, []() {
//     sendDataToAPI(apiEndpoint, rtc, sumVoltage, sumCurrent, sumPower, sumEnergy, sumLoad);
// });
// Task taskFetchData(60000, TASK_FOREVER, []() {
//     fetchDataFromAPI(fetchEndpoint, meterId, connectionAuth);
// });

// void setup() {
//     Serial.begin(115200);
//     WiFi.begin(ssid, password);
//     pinMode(BUZZER_PIN, OUTPUT);
//     pinMode(RELAY_PIN, OUTPUT);
//     digitalWrite(RELAY_PIN, LOW);
    
//     if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
//         Serial.println("OLED initialization failed");
//     }
    
//     if (!rtc.begin()) {
//         Serial.println("RTC initialization failed");
//     }
    
//     display.clearDisplay();
//     display.setTextSize(1);
//     display.setTextColor(WHITE);
//     display.setCursor(0, 0);
//     display.println("Smart Meter Init...");
//     display.display();
    
//     while (WiFi.status() != WL_CONNECTED) {
//         delay(500);
//         Serial.print(".");
//     }
//     Serial.println("\nWiFi Connected");
    
//     scheduler.init();
//     scheduler.addTask(taskSendData);
//     scheduler.addTask(taskFetchData);
//     taskSendData.enable();
//     taskFetchData.enable();
// }

// void loop() {
//     recordPower();
//     scheduler.execute();
// }

// void recordPower() {
//     sumVoltage = sumCurrent = sumPower = sumEnergy = sumLoad = 0.0;
    
//     for (int i = 0; i < samples; i++) {
//         float voltage = pzem.voltage();
//         float current = pzem.current();
//         float power = pzem.power();
//         float energy = pzem.energy();
//         float load = power / (voltage > 0 ? voltage : 1);

//         sumVoltage += voltage;
//         sumCurrent += current;
//         sumPower += power;
//         sumEnergy += energy;
//         sumLoad += load;

//         displayData(voltage, current, power, load);

//         if (voltage >= 240.0) {
//             digitalWrite(RELAY_PIN, LOW);
//             triggerBuzzer(true, false);
//         } else {
//             digitalWrite(RELAY_PIN, HIGH);
//         }
        
//         delay(1000);
//     }
// }

// void displayData(float voltage, float current, float power, float load) {
//     display.clearDisplay();
//     display.setCursor(0, 0);
//     display.printf("V: %.2fV\n", voltage);
//     display.printf("I: %.2fA\n", current);
//     display.printf("P: %.2fW\n", power);
//     display.printf("L: %.2fΩ\n", load);
//     display.display();
// }

// void triggerBuzzer(bool overload, bool lowUnits) {
//     if (overload) {
//         for (int j = 0; j < 3; j++) {
//             digitalWrite(BUZZER_PIN, HIGH);
//             delay(100);
//             digitalWrite(BUZZER_PIN, LOW);
//             delay(200);
//         }
//     } else if (lowUnits) {
//         for (int j = 0; j < 2; j++) {
//             digitalWrite(BUZZER_PIN, HIGH);
//             delay(500);
//             digitalWrite(BUZZER_PIN, LOW);
//             delay(500);
//         }
//     }
// }
