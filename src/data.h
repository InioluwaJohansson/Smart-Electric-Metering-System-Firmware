#ifndef API_HANDLER_H
#define API_HANDLER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <RTClib.h>

void sendDataToAPI(String meterId, String connectionAuth, double powerValue, double voltageValue, double currentValue, double powerFactorValue, String timeValue, bool status);
StaticJsonDocument<256> fetchDataFromAPI(String meterId, String connectionAuth);

#endif