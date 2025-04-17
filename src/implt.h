#ifndef IMPLT_H
#define IMPLT_H
#include <Arduino.h>
#include <Preferences.h>
void MeterInfoSetup();
void ResetButton();
void displayData(String meterId, float voltage, float current, float power, float load, double units);
void triggerBuzzer(bool overload, bool lowUnits);
#endif