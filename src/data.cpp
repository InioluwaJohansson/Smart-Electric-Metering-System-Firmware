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
const char* MeterUnitsDataFromESP32 = "https://8hrck4nk-7211.uks1.devtunnels.ms/SEMS/Data/MeterUnitsDataFromESP32";
void sendDataToAPI(String meterId, String connectionAuth, double powerValue, double voltageValue, double currentValue, double powerFactorValue, String timeValue, bool status) {
  StaticJsonDocument<512> doc;
  doc["meterId"] = meterId;
  doc["connectionAuth"] = connectionAuth;
  doc["powerValue"] = double(powerValue);
  doc["voltageValue"] = double(voltageValue);
  doc["currentValue"] = double(currentValue);
  doc["powerFactorValue"] = double(powerFactorValue);
  doc["timeValue"] = timeValue;
  doc["status"] = status;
  String requestBody;
  serializeJson(doc, requestBody);
  HTTPClient http;
  WiFiClientSecure client;
  client.setInsecure();
  http.begin(MeterUnitsDataFromESP32);
    http.addHeader("Content-Type", "application/json");
    http.setUserAgent("ESP32Client");
    Serial.println("Request Body: " + requestBody);
    if (WiFi.status() == WL_CONNECTED) {
      int httpResponseCode = http.POST(requestBody);
      if (httpResponseCode == 200) {
        String response = http.getString();
        Serial.println("POST Success: " + response);
      } else {
            Serial.println("POST Failed: " + String(httpResponseCode));
        }
        http.end();
      } else {
        WiFi.reconnect();
        int httpResponseCode = http.POST(requestBody);
        if (httpResponseCode == 200) {
          String response = http.getString();
          Serial.println("POST Success: " + response);
        } else {
          Serial.println("POST Failed: " + String(httpResponseCode));
        }
        http.end();
        Serial.println("WiFi not connected for POST request");
    }
    
  }
StaticJsonDocument<256> fetchDataFromAPI(String meterId, String connectionAuth){
  const String EstablishConnectionByESP32 = "https://8hrck4nk-7211.uks1.devtunnels.ms/SEMS/Data/EstablishConnectionByESP32?MeterId=" + meterId + "&ConnectionAuth=" + connectionAuth;
  if (WiFi.status() == WL_CONNECTED && meterId.length() > 0 && connectionAuth.length() > 0) {
    Serial.println("WiFi Connected");
    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient http;
    http.setUserAgent("ESP32Client");
    String fullUrl = EstablishConnectionByESP32;;
    http.begin(EstablishConnectionByESP32);
    int httpResponseCode = http.GET();
    Serial.println("HTTP Response Code: " + String(httpResponseCode));
    if (httpResponseCode == 200) {
        String response = http.getString();
        Serial.println(response);
        StaticJsonDocument<256> doc;
        DeserializationError error = deserializeJson(doc, response);
        Serial.println(error.c_str());
        if (!error) {
          int totalUnits = doc["totalUnits"] | 0;
          int consumedUnits = doc["consumedUnits"] | 0;
          bool isActive = doc["isActive"];
          bool activeLoad = doc["activeLoad"];
          String message = doc["message"] | "No message";
          bool status = doc["status"];
          Serial.println(totalUnits);
          Serial.println(consumedUnits);
          Serial.println(isActive);
          Serial.println(activeLoad);
          Serial.println(message);
          Serial.println(status);
          return doc;
        } else {
            Serial.println("JSON Parsing Error");
        }

    } else {
        Serial.println("GET Failed: " + String(httpResponseCode));
    }
  http.end();
  } else {
      Serial.println("WiFi not connected for GET request");
  }
}
