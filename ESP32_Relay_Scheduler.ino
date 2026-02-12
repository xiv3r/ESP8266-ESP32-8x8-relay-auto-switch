/*
 * ESP32 8-Channel Relay Scheduler with Web Interface
 * Features:
 * - WiFi configuration via web interface
 * - NTP time synchronization
 * - 8 relays with 8 ON and 8 OFF schedules each
 * - Persistent storage of settings
 */

#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <time.h>

// Relay GPIO pins (adjust these to match your wiring)
const int RELAY_PINS[8] = {13, 12, 14, 27, 26, 25, 33, 32};

// Web server on port 80
WebServer server(80);

// Preferences for persistent storage
Preferences preferences;

// WiFi credentials
String ssid = "";
String password = "";

// NTP settings
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 0;
const int daylightOffset_sec = 0;
int timezoneOffset = 0; // User configurable timezone offset in hours

// Schedule structure
struct Schedule {
  bool enabled;
  int hour;
  int minute;
};

// Schedules: [relay][schedule_index]
Schedule onSchedules[8][8];
Schedule offSchedules[8][8];

// Current relay states
bool relayStates[8] = {false, false, false, false, false, false, false, false};

// AP mode settings
const char* ap_ssid = "ESP32-Relay-Config";
const char* ap_password = "12345678";

bool isConfigMode = false;
bool timeInitialized = false;

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n\nESP32 Relay Scheduler Starting...");
  
  // Initialize relay pins
  for (int i = 0; i < 8; i++) {
    pinMode(RELAY_PINS[i], OUTPUT);
    digitalWrite(RELAY_PINS[i], LOW); // Relays off by default
  }
  
  // Initialize preferences
  preferences.begin("relay-config", false);
  
  // Load WiFi credentials
  ssid = preferences.getString("ssid", "");
  password = preferences.getString("password", "");
  timezoneOffset = preferences.getInt("timezone", 0);
  
  // Load schedules
  loadSchedules();
  
  // Try to connect to WiFi or start AP mode
  if (ssid.length() > 0) {
    Serial.println("Attempting to connect to WiFi: " + ssid);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), password.c_str());
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
      delay(500);
      Serial.print(".");
      attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\nConnected to WiFi!");
      Serial.print("IP Address: ");
      Serial.println(WiFi.localIP());
      isConfigMode = false;
      
      // Initialize time
      initTime();
    } else {
      Serial.println("\nFailed to connect. Starting AP mode...");
      startAPMode();
    }
  } else {
    Serial.println("No WiFi credentials found. Starting AP mode...");
    startAPMode();
  }
  
  // Setup web server routes
  setupWebServer();
  
  server.begin();
  Serial.println("Web server started");
}

void loop() {
  server.handleClient();
  
  // Check schedules every second if time is initialized
  static unsigned long lastCheck = 0;
  if (timeInitialized && millis() - lastCheck > 1000) {
    lastCheck = millis();
    checkSchedules();
  }
  
  delay(10);
}

void startAPMode() {
  isConfigMode = true;
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ap_ssid, ap_password);
  Serial.println("AP Mode Started");
  Serial.print("AP IP Address: ");
  Serial.println(WiFi.softAPIP());
  Serial.println("SSID: " + String(ap_ssid));
  Serial.println("Password: " + String(ap_password));
}

void initTime() {
  configTime(timezoneOffset * 3600, daylightOffset_sec, ntpServer);
  
  struct tm timeinfo;
  int attempts = 0;
  while (!getLocalTime(&timeinfo) && attempts < 10) {
    Serial.println("Waiting for time sync...");
    delay(1000);
    attempts++;
  }
  
  if (attempts < 10) {
    timeInitialized = true;
    Serial.println("Time synchronized!");
    Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
  } else {
    Serial.println("Failed to sync time");
    timeInitialized = false;
  }
}

void setupWebServer() {
  server.on("/", handleRoot);
  server.on("/wifi", HTTP_POST, handleWiFiConfig);
  server.on("/schedules", HTTP_GET, handleGetSchedules);
  server.on("/schedules", HTTP_POST, handleSetSchedules);
  server.on("/relay", HTTP_POST, handleManualRelay);
  server.on("/status", HTTP_GET, handleStatus);
  server.on("/timezone", HTTP_POST, handleTimezone);
  server.on("/reset", HTTP_POST, handleReset);
}

void handleRoot() {
  String html = R"(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>ESP32 Relay Scheduler</title>
  <style>
    * { margin: 0; padding: 0; box-sizing: border-box; }
    body {
      font-family: Arial, sans-serif;
      background: #f0f2f5;
      padding: 20px;
    }
    .container {
      max-width: 1200px;
      margin: 0 auto;
      background: white;
      border-radius: 10px;
      padding: 30px;
      box-shadow: 0 2px 10px rgba(0,0,0,0.1);
    }
    h1 {
      color: #333;
      margin-bottom: 10px;
      font-size: 28px;
    }
    .status {
      background: #e8f5e9;
      padding: 15px;
      border-radius: 5px;
      margin: 20px 0;
      border-left: 4px solid #4caf50;
    }
    .section {
      margin: 30px 0;
      padding: 20px;
      background: #fafafa;
      border-radius: 8px;
    }
    .section h2 {
      color: #555;
      margin-bottom: 15px;
      font-size: 20px;
    }
    input, select, button {
      padding: 10px;
      margin: 5px;
      border: 1px solid #ddd;
      border-radius: 5px;
      font-size: 14px;
    }
    button {
      background: #2196F3;
      color: white;
      border: none;
      cursor: pointer;
      padding: 10px 20px;
      font-weight: bold;
      transition: background 0.3s;
    }
    button:hover {
      background: #1976D2;
    }
    .relay-grid {
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(300px, 1fr));
      gap: 20px;
      margin-top: 20px;
    }
    .relay-card {
      background: white;
      padding: 15px;
      border-radius: 8px;
      border: 1px solid #e0e0e0;
    }
    .relay-header {
      display: flex;
      justify-content: space-between;
      align-items: center;
      margin-bottom: 15px;
    }
    .relay-title {
      font-weight: bold;
      font-size: 16px;
      color: #333;
    }
    .toggle-btn {
      padding: 8px 16px;
      font-size: 12px;
    }
    .on { background: #4caf50; }
    .off { background: #f44336; }
    .schedule-list {
      margin-top: 10px;
      max-height: 300px;
      overflow-y: auto;
    }
    .schedule-item {
      display: grid;
      grid-template-columns: 80px 80px 100px auto 60px;
      gap: 5px;
      margin: 5px 0;
      align-items: center;
    }
    .time-display {
      font-size: 18px;
      font-weight: bold;
      color: #2196F3;
      text-align: center;
      padding: 10px;
      background: #e3f2fd;
      border-radius: 5px;
      margin: 10px 0;
    }
    .wifi-form {
      display: grid;
      gap: 10px;
      max-width: 400px;
    }
    .success { color: #4caf50; font-weight: bold; }
    .error { color: #f44336; font-weight: bold; }
    .btn-danger {
      background: #f44336;
    }
    .btn-danger:hover {
      background: #d32f2f;
    }
  </style>
</head>
<body>
  <div class="container">
    <h1>🔌 ESP32 Relay Scheduler</h1>
    
    <div class="status">
      <strong>Status:</strong> <span id="connection-status">Loading...</span><br>
      <strong>IP:</strong> <span id="ip-address">-</span><br>
      <strong>Time:</strong> <div class="time-display" id="current-time">--:--:--</div>
    </div>

    <div class="section">
      <h2>⚙️ WiFi Configuration</h2>
      <div class="wifi-form">
        <input type="text" id="ssid" placeholder="WiFi SSID" value="">
        <input type="password" id="password" placeholder="WiFi Password">
        <label>Timezone Offset (hours): 
          <input type="number" id="timezone" value="0" min="-12" max="14" style="width: 80px;">
        </label>
        <button onclick="saveWiFi()">Save WiFi Settings</button>
        <div id="wifi-message"></div>
      </div>
    </div>

    <div class="section">
      <h2>🎛️ Relay Controls</h2>
      <div class="relay-grid" id="relay-controls">
        <!-- Relay controls will be generated here -->
      </div>
    </div>

    <div class="section">
      <h2>⏰ Schedule Configuration</h2>
      <select id="schedule-relay" onchange="loadRelaySchedules()">
        <option value="0">Relay 1</option>
        <option value="1">Relay 2</option>
        <option value="2">Relay 3</option>
        <option value="3">Relay 4</option>
        <option value="4">Relay 5</option>
        <option value="5">Relay 6</option>
        <option value="6">Relay 7</option>
        <option value="7">Relay 8</option>
      </select>
      
      <div id="schedule-editor"></div>
      
      <button onclick="saveSchedules()" style="margin-top: 20px;">💾 Save Schedules</button>
      <div id="schedule-message"></div>
    </div>

    <div class="section">
      <h2>🔧 System</h2>
      <button class="btn-danger" onclick="if(confirm('Reset all settings?')) resetSystem()">Reset to Factory Defaults</button>
    </div>
  </div>

  <script>
    let currentSchedules = { on: [], off: [] };
    
    function updateStatus() {
      fetch('/status')
        .then(r => r.json())
        .then(data => {
          document.getElementById('connection-status').textContent = data.connected ? 'Connected to WiFi' : 'AP Mode';
          document.getElementById('ip-address').textContent = data.ip;
          document.getElementById('current-time').textContent = data.time;
          document.getElementById('ssid').value = data.ssid;
          document.getElementById('timezone').value = data.timezone;
          
          // Update relay controls
          updateRelayControls(data.relays);
        });
    }
    
    function updateRelayControls(relays) {
      const container = document.getElementById('relay-controls');
      container.innerHTML = '';
      
      for (let i = 0; i < 8; i++) {
        const state = relays[i];
        const card = document.createElement('div');
        card.className = 'relay-card';
        card.innerHTML = `
          <div class="relay-header">
            <span class="relay-title">Relay ${i + 1}</span>
            <button class="toggle-btn ${state ? 'on' : 'off'}" onclick="toggleRelay(${i})">
              ${state ? 'ON' : 'OFF'}
            </button>
          </div>
          <div style="font-size: 12px; color: #666;">
            Pin: GPIO ${[13, 12, 14, 27, 26, 25, 33, 32][i]}
          </div>
        `;
        container.appendChild(card);
      }
    }
    
    function toggleRelay(relay) {
      fetch('/relay', {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
        body: `relay=${relay}&state=${!currentState}`
      }).then(() => updateStatus());
    }
    
    function saveWiFi() {
      const ssid = document.getElementById('ssid').value;
      const password = document.getElementById('password').value;
      const timezone = document.getElementById('timezone').value;
      
      fetch('/wifi', {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
        body: `ssid=${encodeURIComponent(ssid)}&password=${encodeURIComponent(password)}`
      }).then(r => r.text()).then(msg => {
        document.getElementById('wifi-message').innerHTML = `<p class="success">${msg}</p>`;
      });
      
      fetch('/timezone', {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
        body: `offset=${timezone}`
      });
    }
    
    function loadRelaySchedules() {
      const relay = document.getElementById('schedule-relay').value;
      fetch(`/schedules?relay=${relay}`)
        .then(r => r.json())
        .then(data => {
          currentSchedules = data;
          renderScheduleEditor();
        });
    }
    
    function renderScheduleEditor() {
      const editor = document.getElementById('schedule-editor');
      let html = '<h3>ON Schedules</h3><div class="schedule-list">';
      
      for (let i = 0; i < 8; i++) {
        const s = currentSchedules.on[i];
        html += `
          <div class="schedule-item">
            <input type="checkbox" id="on-${i}-en" ${s.enabled ? 'checked' : ''}>
            <input type="number" id="on-${i}-h" value="${s.hour}" min="0" max="23" placeholder="HH">
            <input type="number" id="on-${i}-m" value="${s.minute}" min="0" max="59" placeholder="MM">
            <label for="on-${i}-en">Schedule ${i + 1}</label>
          </div>
        `;
      }
      
      html += '</div><h3>OFF Schedules</h3><div class="schedule-list">';
      
      for (let i = 0; i < 8; i++) {
        const s = currentSchedules.off[i];
        html += `
          <div class="schedule-item">
            <input type="checkbox" id="off-${i}-en" ${s.enabled ? 'checked' : ''}>
            <input type="number" id="off-${i}-h" value="${s.hour}" min="0" max="23" placeholder="HH">
            <input type="number" id="off-${i}-m" value="${s.minute}" min="0" max="59" placeholder="MM">
            <label for="off-${i}-en">Schedule ${i + 1}</label>
          </div>
        `;
      }
      
      html += '</div>';
      editor.innerHTML = html;
    }
    
    function saveSchedules() {
      const relay = document.getElementById('schedule-relay').value;
      const schedules = { on: [], off: [] };
      
      for (let i = 0; i < 8; i++) {
        schedules.on.push({
          enabled: document.getElementById(`on-${i}-en`).checked ? 1 : 0,
          hour: parseInt(document.getElementById(`on-${i}-h`).value) || 0,
          minute: parseInt(document.getElementById(`on-${i}-m`).value) || 0
        });
        schedules.off.push({
          enabled: document.getElementById(`off-${i}-en`).checked ? 1 : 0,
          hour: parseInt(document.getElementById(`off-${i}-h`).value) || 0,
          minute: parseInt(document.getElementById(`off-${i}-m`).value) || 0
        });
      }
      
      fetch('/schedules', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ relay: parseInt(relay), schedules: schedules })
      }).then(r => r.text()).then(msg => {
        document.getElementById('schedule-message').innerHTML = `<p class="success">${msg}</p>`;
        setTimeout(() => document.getElementById('schedule-message').innerHTML = '', 3000);
      });
    }
    
    function resetSystem() {
      fetch('/reset', { method: 'POST' })
        .then(() => {
          alert('System reset. Device will restart in AP mode.');
          setTimeout(() => location.reload(), 2000);
        });
    }
    
    // Initialize
    updateStatus();
    loadRelaySchedules();
    setInterval(updateStatus, 2000);
  </script>
</body>
</html>
  )";
  
  server.send(200, "text/html", html);
}

void handleWiFiConfig() {
  if (server.hasArg("ssid") && server.hasArg("password")) {
    ssid = server.arg("ssid");
    password = server.arg("password");
    
    preferences.putString("ssid", ssid);
    preferences.putString("password", password);
    
    server.send(200, "text/plain", "WiFi settings saved! Device will restart in 3 seconds...");
    
    delay(3000);
    ESP.restart();
  } else {
    server.send(400, "text/plain", "Missing parameters");
  }
}

void handleTimezone() {
  if (server.hasArg("offset")) {
    timezoneOffset = server.arg("offset").toInt();
    preferences.putInt("timezone", timezoneOffset);
    
    if (WiFi.status() == WL_CONNECTED) {
      initTime();
    }
    
    server.send(200, "text/plain", "Timezone updated");
  } else {
    server.send(400, "text/plain", "Missing offset parameter");
  }
}

void handleGetSchedules() {
  if (!server.hasArg("relay")) {
    server.send(400, "text/plain", "Missing relay parameter");
    return;
  }
  
  int relay = server.arg("relay").toInt();
  if (relay < 0 || relay > 7) {
    server.send(400, "text/plain", "Invalid relay number");
    return;
  }
  
  String json = "{\"on\":[";
  for (int i = 0; i < 8; i++) {
    if (i > 0) json += ",";
    json += "{\"enabled\":" + String(onSchedules[relay][i].enabled ? "true" : "false");
    json += ",\"hour\":" + String(onSchedules[relay][i].hour);
    json += ",\"minute\":" + String(onSchedules[relay][i].minute) + "}";
  }
  json += "],\"off\":[";
  for (int i = 0; i < 8; i++) {
    if (i > 0) json += ",";
    json += "{\"enabled\":" + String(offSchedules[relay][i].enabled ? "true" : "false");
    json += ",\"hour\":" + String(offSchedules[relay][i].hour);
    json += ",\"minute\":" + String(offSchedules[relay][i].minute) + "}";
  }
  json += "]}";
  
  server.send(200, "application/json", json);
}

void handleSetSchedules() {
  String body = server.arg("plain");
  
  // Parse JSON manually (simple parsing for our specific format)
  int relayNum = -1;
  
  // Extract relay number
  int relayPos = body.indexOf("\"relay\":");
  if (relayPos != -1) {
    relayNum = body.substring(relayPos + 8, relayPos + 9).toInt();
  }
  
  if (relayNum < 0 || relayNum > 7) {
    server.send(400, "text/plain", "Invalid relay number");
    return;
  }
  
  // Parse ON schedules
  int onPos = body.indexOf("\"on\":[");
  if (onPos != -1) {
    String onSection = body.substring(onPos + 6);
    int endPos = onSection.indexOf("],\"off\"");
    if (endPos != -1) {
      onSection = onSection.substring(0, endPos);
      parseScheduleArray(onSection, onSchedules[relayNum]);
    }
  }
  
  // Parse OFF schedules
  int offPos = body.indexOf("\"off\":[");
  if (offPos != -1) {
    String offSection = body.substring(offPos + 7);
    int endPos = offSection.indexOf("]}");
    if (endPos != -1) {
      offSection = offSection.substring(0, endPos);
      parseScheduleArray(offSection, offSchedules[relayNum]);
    }
  }
  
  // Save to preferences
  saveSchedules();
  
  server.send(200, "text/plain", "Schedules saved successfully!");
}

void parseScheduleArray(String jsonArray, Schedule schedules[8]) {
  int scheduleIndex = 0;
  int pos = 0;
  
  while (scheduleIndex < 8 && pos < jsonArray.length()) {
    int objStart = jsonArray.indexOf("{", pos);
    if (objStart == -1) break;
    
    int objEnd = jsonArray.indexOf("}", objStart);
    if (objEnd == -1) break;
    
    String obj = jsonArray.substring(objStart, objEnd + 1);
    
    // Parse enabled
    int enabledPos = obj.indexOf("\"enabled\":");
    if (enabledPos != -1) {
      String enabledStr = obj.substring(enabledPos + 10);
      schedules[scheduleIndex].enabled = (enabledStr.indexOf("1") != -1 || enabledStr.indexOf("true") != -1);
    }
    
    // Parse hour
    int hourPos = obj.indexOf("\"hour\":");
    if (hourPos != -1) {
      String hourStr = obj.substring(hourPos + 7);
      schedules[scheduleIndex].hour = hourStr.toInt();
    }
    
    // Parse minute
    int minutePos = obj.indexOf("\"minute\":");
    if (minutePos != -1) {
      String minuteStr = obj.substring(minutePos + 9);
      schedules[scheduleIndex].minute = minuteStr.toInt();
    }
    
    scheduleIndex++;
    pos = objEnd + 1;
  }
}

void handleManualRelay() {
  if (server.hasArg("relay") && server.hasArg("state")) {
    int relay = server.arg("relay").toInt();
    bool state = (server.arg("state") == "1" || server.arg("state") == "true");
    
    if (relay >= 0 && relay < 8) {
      setRelay(relay, state);
      server.send(200, "text/plain", "Relay " + String(relay + 1) + " set to " + (state ? "ON" : "OFF"));
    } else {
      server.send(400, "text/plain", "Invalid relay number");
    }
  } else {
    server.send(400, "text/plain", "Missing parameters");
  }
}

void handleStatus() {
  struct tm timeinfo;
  String timeStr = "--:--:--";
  
  if (timeInitialized && getLocalTime(&timeinfo)) {
    char buffer[20];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &timeinfo);
    timeStr = String(buffer);
  }
  
  String json = "{";
  json += "\"connected\":" + String(WiFi.status() == WL_CONNECTED ? "true" : "false") + ",";
  json += "\"ip\":\"" + (isConfigMode ? WiFi.softAPIP().toString() : WiFi.localIP().toString()) + "\",";
  json += "\"ssid\":\"" + ssid + "\",";
  json += "\"timezone\":" + String(timezoneOffset) + ",";
  json += "\"time\":\"" + timeStr + "\",";
  json += "\"relays\":[";
  for (int i = 0; i < 8; i++) {
    if (i > 0) json += ",";
    json += relayStates[i] ? "true" : "false";
  }
  json += "]}";
  
  server.send(200, "application/json", json);
}

void handleReset() {
  preferences.clear();
  server.send(200, "text/plain", "Settings cleared. Restarting...");
  delay(1000);
  ESP.restart();
}

void setRelay(int relay, bool state) {
  if (relay >= 0 && relay < 8) {
    relayStates[relay] = state;
    digitalWrite(RELAY_PINS[relay], state ? HIGH : LOW);
    Serial.println("Relay " + String(relay + 1) + " set to " + (state ? "ON" : "OFF"));
  }
}

void checkSchedules() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return;
  }
  
  int currentHour = timeinfo.tm_hour;
  int currentMinute = timeinfo.tm_min;
  int currentSecond = timeinfo.tm_sec;
  
  // Only check at the start of each minute (when seconds == 0)
  if (currentSecond != 0) {
    return;
  }
  
  // Check all relays and their schedules
  for (int relay = 0; relay < 8; relay++) {
    // Check ON schedules
    for (int i = 0; i < 8; i++) {
      if (onSchedules[relay][i].enabled &&
          onSchedules[relay][i].hour == currentHour &&
          onSchedules[relay][i].minute == currentMinute) {
        setRelay(relay, true);
        Serial.println("Schedule triggered: Relay " + String(relay + 1) + " ON");
      }
    }
    
    // Check OFF schedules
    for (int i = 0; i < 8; i++) {
      if (offSchedules[relay][i].enabled &&
          offSchedules[relay][i].hour == currentHour &&
          offSchedules[relay][i].minute == currentMinute) {
        setRelay(relay, false);
        Serial.println("Schedule triggered: Relay " + String(relay + 1) + " OFF");
      }
    }
  }
}

void loadSchedules() {
  for (int relay = 0; relay < 8; relay++) {
    String keyPrefix = "r" + String(relay);
    
    for (int i = 0; i < 8; i++) {
      // Load ON schedules
      String onKey = keyPrefix + "_on" + String(i);
      uint32_t onData = preferences.getUInt(onKey.c_str(), 0);
      onSchedules[relay][i].enabled = (onData >> 16) & 0x01;
      onSchedules[relay][i].hour = (onData >> 8) & 0xFF;
      onSchedules[relay][i].minute = onData & 0xFF;
      
      // Load OFF schedules
      String offKey = keyPrefix + "_off" + String(i);
      uint32_t offData = preferences.getUInt(offKey.c_str(), 0);
      offSchedules[relay][i].enabled = (offData >> 16) & 0x01;
      offSchedules[relay][i].hour = (offData >> 8) & 0xFF;
      offSchedules[relay][i].minute = offData & 0xFF;
    }
  }
  
  Serial.println("Schedules loaded from storage");
}

void saveSchedules() {
  for (int relay = 0; relay < 8; relay++) {
    String keyPrefix = "r" + String(relay);
    
    for (int i = 0; i < 8; i++) {
      // Save ON schedules
      String onKey = keyPrefix + "_on" + String(i);
      uint32_t onData = ((uint32_t)onSchedules[relay][i].enabled << 16) |
                        ((uint32_t)onSchedules[relay][i].hour << 8) |
                        (uint32_t)onSchedules[relay][i].minute;
      preferences.putUInt(onKey.c_str(), onData);
      
      // Save OFF schedules
      String offKey = keyPrefix + "_off" + String(i);
      uint32_t offData = ((uint32_t)offSchedules[relay][i].enabled << 16) |
                         ((uint32_t)offSchedules[relay][i].hour << 8) |
                         (uint32_t)offSchedules[relay][i].minute;
      preferences.putUInt(offKey.c_str(), offData);
    }
  }
  
  Serial.println("Schedules saved to storage");
}
