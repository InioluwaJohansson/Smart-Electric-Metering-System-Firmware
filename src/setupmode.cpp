#include <WiFi.h>
#include <ArduinoWebsockets.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <soc/rtc_wdt.h>
#include "esp_task_wdt.h"
#include "auth.h"
#include "implt.h"
const char* ssidBootMode = "SEMS Meter Setup!"; const char* passwordBootMode = "SEMSMeter";
IPAddress apIP(72, 72, 72, 72); AsyncWebServer server(80); AsyncWebSocket ws("/ws");
int socket_data = 0;
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1.0" />
  <title>Smart Electric Metering System Meter Setup</title>
  <style>
    * {
      margin: 0;
      padding: 0;
      box-sizing: border-box;
      font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
    }
    body {
      background-color: #0b0f1a;
      height: 100vh;
      display: flex;
      align-items: center;
      justify-content: center;
      color: #ffffff;
      overflow: auto;
      padding: 7rem;
    }
    .login-container {
      background-color: #111827;
      padding: 2rem;
      border-radius: 1rem;
      width: 100%;
      max-width: 400px;
      box-shadow: 0 0 20px rgba(0, 0, 0, 0.4);
    }
    .header {
      display: flex;
      justify-content: center;
      align-items: center;
      margin-bottom: 1.1rem;
    }
    .logo {
      display: flex;
      align-items: center;
      gap: 0.2rem;

    }
    svg.logo-icon  {
      fill: #3b82f6;
    }
    .title {
      font-size: 1.7em;
      font-weight: bold;
    }
    h2 {
      font-size: 1.5rem;
      font-weight: bold;
    }
    label {
      font-size: .8rem !important;
    }
    p.subtext {
      font-size: 1rem;
      color: #9ca3af;
      margin-bottom: .5rem;
    }
    form {
      display: flex;
      flex-direction: column;
      gap: 1rem;
    }
    label {
      font-size: 1.2rem;
      margin-bottom: 0.25rem;
    }
    input[type="text"], input[type="password"] {
      width: 100%;
      padding: 0.6rem 0.75rem;
      background-color: #1f2937;
      border: 1px solid #374151;
      border-radius: 0.375rem;
      color: #ffffff;
      font-size: .8rem;
      outline:0px solid transparent !important;
    }
    input:focus{
      outline: 1px solid #9ca3af;
    }
    input::placeholder {
      color: #9ca3af;
      font-size: .8rem;
    }
    .input-wrapper {
      display: flex;
      flex-direction: column;
    }
    .btn {
      display: flex;
      align-items: center;
      justify-content: center;
      gap: 0.5rem;
      background-color: #3b82f6;
      color: white;
      padding: 0.6rem;
      font-size: 1rem;
      border: none;
      border-radius: 0.375rem;
      cursor: pointer;
      transition: background-color 0.3s ease;
    }
    .btn:hover {
      background-color: #2563eb;
    }
    ::-webkit-scrollbar {
      display: none;
    }
    .close-card{
      display: flex;
      flex-direction: column;
      align-items: center;
      justify-content: center;
      gap: 0.5rem;
    }
    
  </style>
</head>
<body>
  <div class="login-container">
  </div>
  <script>
    const main_content = `<div class="header">
      <div class="logo">
        <svg xmlns="http://www.w3.org/2000/svg" class="logo-icon" width="24" height="24" fill="currentColor" viewBox="0 0 24 24">
          <path d="M13 2L3 14h7v8l10-12h-7z"/>
        </svg>
        <div class="title">SEMS</div>
      </div>
    </div>
        <h3>Setup meter for the first time!</h3>
    <p class="subtext">Enter the credentials below to setup your meter</p>
    <form>
      <h3>Network Credentials</h3>
      <div class="input-wrapper">
        <label for="wifiName">Wi-Fi Name</label>
        <input type="text" id="wifiName" placeholder="Wi-Fi Name" oninput="checkWifiNameInput()" minlength="1">
      </div>
      <div class="input-wrapper">
        <label for="wifiPassword">Wi-Fi Password</label>
        <input type="password" id="wifiPassword" placeholder="Wi-Fi Password" oninput="checkWifiPasswordInput()" minlength="1">
      </div>
      <h3>Meter Credentials</h3>
      <div class="input-wrapper">
        <label for="meterId">Meter ID</label>
        <input type="text" id="meterId" placeholder="METERAG67W35" oninput="checkMeterInput()" maxlength="13">
      </div>
      <div class="input-wrapper">
        <label for="connectionAuth">Connection Auth</label>
        <input type="text" id="connectionAuth" placeholder="Connection Auth" oninput="checkConnectionInput()" maxlength="18">
      </div>
      <button class="btn" type="button" onclick="Send()">
        <svg xmlns="http://www.w3.org/2000/svg" width="16" height="16" viewBox="0 0 24 24" fill="currentColor">
          <path d="M17 3H5a2 2 0 0 0-2 2v14a2 2 0 0 0 2 2h14a2 2 0 0 0 2-2V7l-4-4zM12 19a2 2 0 1 1 0-4 2 2 0 0 1 0 4zm3-10H5V5h10v4z"/>
        </svg>
        Save Credentials
      </button>
    </form>`;
    const content = `
      <div class="close-card">
        <div class="header">
          <div class="logo">
            <svg xmlns="http://www.w3.org/2000/svg" class="logo-icon" width="24" height="24" fill="currentColor" viewBox="0 0 24 24">
              <path d="M13 2L3 14h7v8l10-12h-7z"/>
            </svg>
            <div class="title">SEMS</div>
          </div>
        </div>
        <h3>Credentials Saved!</h3>
        <p class="subtext">This window will close in.</p>
        <h2 class="countdown">5</h2>
      </div>
      `;
      var connection;
      
    Connect = () => {
      if (connection == null) {
        connection = new WebSocket("ws://" + window.location.host + "/ws");
        console.log(connection.message);
      }
    }
    Send = () => {
      var section = document.querySelector('.login-container');
      var status = SendCredentials();
      if(status == true){
        section.style = "display: flex; flex-direction: row"
        createDotLoader();
        setTimeout(() => {
          displayCountdown();
        }, 5000);
      }
    }
    displayCountdown = () => {
      var section = document.querySelector('.login-container');
      section.innerHTML = content;
      startCountdown();
    }
    Connect();
    SendCredentials = () => {
      var meterId = document.querySelector('#meterId');
      var connectionAuth = document.querySelector('#connectionAuth');
      var wifiName = document.querySelector('#wifiName');
      var wifiPassword = document.querySelector('#wifiPassword');
      if (checkMeterInput() && checkConnectionInput() && checkWifiNameInput() && checkWifiPasswordInput()) {
        if (connection != null) {
          connection.send(meterId.value+"/"+connectionAuth.value+"/"+wifiName.value+"/"+wifiPassword.value);
          return true
        } else {
          Connect();
        }
      }
      else{
        checkMeterInput();
        checkConnectionInput();
        checkWifiNameInput();
        checkWifiPasswordInput();
      }      
    }
    checkMeterInput = () => {
      var input = document.querySelector('#meterId');
      if (input.value.length < 13) {
        input.style = 'border-bottom: 2px solid #dc3545; transition: 2s';
        return false;
      } else {
        input.style = 'border-bottom: 2px solid #198754; transition: 2s';
        return true;
      }
    }
    checkConnectionInput = () => {
      var input = document.querySelector('#connectionAuth');
      if (input.value.length < 10) {
        input.style = 'border-bottom: 2px solid #dc3545; transition: 2s';
        return false;
      } else {
        input.style = 'border-bottom: 2px solid #198754; transition: 2s';
        return true;
      }
    }
    checkWifiNameInput = () => {
      var input = document.querySelector('#wifiName');
      if (input.value.length < 1) {
        input.style = 'border-bottom: 2px solid #dc3545; transition: 2s';
        return false;
      } else {
        input.style = 'border-bottom: 2px solid #198754; transition: 2s';
        return true;
      }
    }
    checkWifiPasswordInput = () => {
      var input = document.querySelector('#wifiPassword');
      if (input.value.length < 1) {
        input.style = 'border-bottom: 2px solid #dc3545; transition: 2s';
        return false;
      } else {
        input.style = 'border-bottom: 2px solid #198754; transition: 2s';
        return true;
      }
    }
    connectionMessage = (e_msg) => {
      e_msg = e_msg || window.event;
      console.log(e_msg.data);
    }
    function startCountdown(start = 5) {
      let time = start;
      const display = document.querySelector('.countdown');
      display.innerHTML = time;
      const interval = setInterval(() => {
        time--;
        if (time >= 0) {
          display.innerHTML = time;
        } else {
          clearInterval(interval);
          setTimeout(() => {
            window.close();
          }, 1000);
        }
      }, 1000);
    }
    displayLoading = () => {
      createDotLoader();
      Connect();
      setTimeout(() => {
        displayContent();
      }, 5000);
    }
    displayContent = () => {
      var section = document.querySelector('.login-container');
      section.style = "disply: flex; flex-direction: column;";
      section.innerHTML = main_content;
    }

    function createDotLoader() {
      const container = document.querySelector('.login-container');
      if (!container) return;

      container.innerHTML = "";
      container.style.display = "flex";
      container.style.gap = "8px";
      container.style.alignItems = "center";
      container.style.justifyContent = "center";
      for (let i = 0; i < 5; i++) {
        const dot = document.createElement("span");
        dot.style.width = "10px";
        dot.style.height = "10px";
        dot.style.borderRadius = "50%";
        dot.style.backgroundColor = "white";
        dot.style.animation = `dotBounce 1s infinite ease-in-out`;
        dot.style.animationDelay = `${i * 0.15}s`;
        container.appendChild(dot);
      }
      if (!document.getElementById("dot-bounce-style")) {
        const style = document.createElement("style");
        style.id = "dot-bounce-style";
        style.textContent = `
          @keyframes dotBounce {
            0%, 80%, 100% {
              transform: scale(0.8);
              opacity: 0.3;
            }
            40% {
              transform: scale(1.2);
              opacity: 1;
            }
          }
        `;
        document.head.appendChild(style);
      }
    }
    displayLoading();
  </script>
</body>
</html>
    )rawliteral";
void notify_to_Clients() {
    ws.textAll(String(socket_data));
  }
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    // Convert raw data to a proper null-terminated string
    char message[len + 1];
    memcpy(message, data, len);
    message[len] = '\0';

    String payload = String(message);
    Serial.println("Received: " + payload);

    int separatorIndex = payload.indexOf('/');
    if (separatorIndex > 0) {
      String meterId = payload.substring(0, separatorIndex);
      String connectionAuth = payload.substring(separatorIndex + 1);
      String wifiName = payload.substring(payload.indexOf('/', separatorIndex + 1) + 1, payload.lastIndexOf('/'));
      String wifiPassword = payload.substring(payload.lastIndexOf('/') + 1);
      Serial.println("Meter ID: " + meterId);
      Serial.println("Connection Auth: " + connectionAuth);
      Serial.println("Wi-Fi Name: " + wifiName);
      Serial.println("Wi-Fi Password: " + wifiPassword);
      saveMeterCredentials(meterId, connectionAuth, wifiName, wifiPassword);

      // Save to preferences or use as needed
    } else {
      Serial.println("Invalid format. Expected 'meterId/connectionAuth'");
    }
    notify_to_Clients(); 
  }
}
void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      break;
    case WS_EVT_DISCONNECT:
      break;
    case WS_EVT_DATA:
      handleWebSocketMessage(arg, data, len);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}
void initWebSocket() {
  ws.onEvent(onEvent);  server.addHandler(&ws);
}
void bootMode(){
  Serial.begin(9600); Serial.println("Setting AP (Access Point)…");
  WiFi.mode(WIFI_AP_STA); WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP(ssidBootMode, passwordBootMode); IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");  Serial.println(myIP);
  displayWifiText(ssidBootMode, passwordBootMode, String("72.72.72.72"));
  initWebSocket();
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html);
  });
  server.begin();
}