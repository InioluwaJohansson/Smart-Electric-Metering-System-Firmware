#include <Preferences.h>
#include "auth.h"
#include "implt.h"
Preferences preferences;
#define ACTIVE_PIN 21
void saveMeterCredentials(String meterId, String connectionAuth) {
  preferences.begin("meter", false);
  preferences.putString("id", meterId);
  preferences.putString("auth", connectionAuth);
  preferences.end();
  displayTextCenter("Meter Data Saved", "Successfully");
  delay(1000);
  for(int i = 5; i > 0; i--) {
    displayTextCenter("Rebooting in " + String(i), "");
    delay(1000);
  }
  displayClear();
  Serial.println("✅ Setup complete. Rebooting...");
    digitalWrite(ACTIVE_PIN, HIGH);
    delay(1000);
    digitalWrite(ACTIVE_PIN, LOW);
  ESP.restart();
}

MeterConfig loadMeterData() {
    Serial.begin(9600);
    MeterConfig doc;
    preferences.begin("meter", true);
    doc.meterId = preferences.getString("id");
    doc.connectionAuth = preferences.getString("auth");
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
        delay(1000);
    }
    meterIdSerial = Serial.readStringUntil('\n');
    meterIdSerial.trim();
    Serial.println(meterIdSerial);

    Serial.print("Enter Connection Auth:");
    while (Serial.available() == 0) {
        delay(1000);
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