#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include "data.h"
const char* MeterUnitsDataFromESP32 = "https://8hrck4nk-7211.uks1.devtunnels.ms/SEMS/Data/MeterUnitsDataFromESP32";
void sendDataToAPI(String meterId, String connectionAuth, double powerValue, double voltageValue, double currentValue, bool status) {
  StaticJsonDocument<512> doc;
  doc["meterId"] = meterId;
  doc["connectionAuth"] = connectionAuth;
  doc["powerValue"] = double(powerValue);
  doc["voltageValue"] = double(voltageValue);
  doc["currentValue"] = double(currentValue);
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
MeterData fetchDataFromAPI(String meterId, String connectionAuth){
  String EstablishConnectionByESP32 = "https://8hrck4nk-7211.uks1.devtunnels.ms/SEMS/Data/EstablishConnectionByESP32?MeterId=" + meterId + "&auth=" + connectionAuth;
  StaticJsonDocument<256> doc;
  MeterData meterData;
  if (WiFi.status() == WL_CONNECTED && meterId.length() > 0 && connectionAuth.length() > 0) {
    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient http;
    http.setUserAgent("ESP32Client");
    http.begin(EstablishConnectionByESP32);
    int httpResponseCode = http.GET();
    if (httpResponseCode == 200) {
        String response = http.getString();
        DeserializationError error = deserializeJson(doc, response);
        Serial.println(error.c_str());
        if (!error && doc != NULL) {
          meterData.totalUnits = doc["totalUnits"];
          meterData.consumedUnits = doc["consumedUnits"];
          meterData.isActive = doc["isActive"];
          meterData.activeLoad = doc["activeLoad"];
          meterData.status = doc["status"];
          return meterData;
        } else {
            Serial.println("JSON Parsing Error");
            return fetchDataFromAPI(meterId, connectionAuth);
        }

    } else {
        Serial.println("GET Failed: " + String(httpResponseCode));
        meterData.totalUnits = 0.00;
        meterData.consumedUnits = 0.00;
        meterData.isActive = false;
        meterData.activeLoad = false;
        meterData.status = false;
        return meterData;
    }
  http.end();
  } else {
      Serial.println("WiFi not connected for GET request");
      WiFi.reconnect();
      delay(5000);
      return fetchDataFromAPI(meterId, connectionAuth);
  }
}
