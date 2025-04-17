#ifndef SAVEDATA_H
#define SAVEDATA_H
#include <Arduino.h>
#include <Preferences.h>
struct MeterConfig {
  String meterId;
  String connectionAuth;
};
void saveMeterCredentials(String meterId, String connectionAuth);
MeterConfig loadMeterData();
void EnterCredentials();
void ClearCredentials();

#endif