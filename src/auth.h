#ifndef SAVEDATA_H
#define SAVEDATA_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <Preferences.h>

void saveMeterCredentials(String meterId, String connectionAuth);
StaticJsonDocument<256> loadMeterData();
void EnterCredentials();
void ClearCredentials();

#endif