#include <ArduinoJson.h>
#include <Preferences.h>
Preferences preferences;

void saveMeterCredentials(String meterId, String connectionAuth) {
  preferences.begin("meter", false);
  preferences.putString("id", meterId);
  preferences.putString("auth", connectionAuth);
  preferences.end();
}

StaticJsonDocument<256> loadMeterData() {
    StaticJsonDocument<256> doc;
    preferences.begin("meter", true);
    doc["meterId"] = preferences.getString("id");
    doc["connectionAuth"] = preferences.getString("auth");
    preferences.end();
    return doc;
}

void EnterCredentials(){
    Serial.begin(9600);
    delay(1000);
    Serial.print("Enter Meter ID: ");
    delay(2000);
    String meterIdSerial = "";
    String connectionAuthSerial = "";
    while (Serial.available() == 0) {
        delay(100);
    }
    meterIdSerial = Serial.readStringUntil('\n');
    meterIdSerial.trim();
    Serial.println(meterIdSerial);

    Serial.print("Enter Connection Auth:");
    while (Serial.available() == 0) {
        delay(100);
    }
    connectionAuthSerial = Serial.readStringUntil('\n');
    connectionAuthSerial.trim();
    Serial.println(connectionAuthSerial);
    if(meterIdSerial == "" || connectionAuthSerial == "") {
        Serial.println("Invalid input. Please try again.");
        EnterCredentials();
    }else{
        saveMeterCredentials(meterIdSerial, connectionAuthSerial);
        Serial.println("Meter Credentials Configured Successfully.");
    }
}

void ClearCredentials(){
    preferences.begin("meter", false);
    preferences.remove("id");
    preferences.remove("auth");
    preferences.end(); 
}