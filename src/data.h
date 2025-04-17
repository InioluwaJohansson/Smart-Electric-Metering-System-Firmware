#ifndef API_HANDLER_H
#define API_HANDLER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <RTClib.h>
struct MeterData{
    double totalUnits;
    double consumedUnits;
    bool isActive;
    bool activeLoad;
    bool status;
};
void sendDataToAPI(String meterId, String connectionAuth, double powerValue, double voltageValue, double currentValue, double powerFactorValue, String timeValue, bool status);
MeterData fetchDataFromAPI(String meterId, String connectionAuth);

#endif