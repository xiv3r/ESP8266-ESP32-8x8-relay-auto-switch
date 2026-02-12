#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <time.h>

// ==================== CONFIGURATION ====================
#define NUM_RELAYS 8
#define SCHEDULES_PER_RELAY 8
#define NTP_SERVER "pool.ntp.org"
#define GMT_OFFSET 0  // Change to your timezone offset in seconds (e.g., -5*3600 for EST)
#define DAYLIGHT_OFFSET 0  // Daylight saving offset in seconds

// Relay pin assignments (change these to match your setup)
const int RELAY_PINS[NUM_RELAYS] = {D0, D1, D2, D3, D4, D5, D6, D7};

// ==================== STRUCTURES ====================
struct Schedule {
  bool enabled;
  uint8_t hour;
  uint8_t minute;
  bool isOnSchedule;  // true for ON, false for OFF
};

struct RelayData {
  Schedule schedules[SCHEDULES_PER_RELAY];
  bool lastState;
};

// ==================== GLOBAL VARIABLES ====================
ESP8266WebServer server(80);
RelayData relayData[NUM_RELAYS];
bool relayStates[NUM_RELAYS] = {false};
unsigned long lastScheduleCheck = 0;
const unsigned long SCHEDULE_CHECK_INTERVAL = 60000;  // Check every minute

// ==================== FILESYSTEM FUNCTIONS ====================

void initFilesystem() {
  if (!LittleFS.begin()) {
    Serial.println("LittleFS mount failed");
    return;
  }
  Serial.println("LittleFS mounted successfully");
  loadAllSchedules();
}

void saveSchedule(int relayIndex) {
  if (relayIndex < 0 || relayIndex >= NUM_RELAYS) return;
  
  String filename = "/relay_" + String(relayIndex) + ".json";
  DynamicJsonDocument doc(2048);
  
  for (int i = 0; i < SCHEDULES_PER_RELAY; i++) {
    JsonObject sched = doc.createNestedObject("schedule_" + String(i));
    sched["enabled"] = relayData[relayIndex].schedules[i].enabled;
    sched["hour"] = relayData[relayIndex].schedules[i].hour;
    sched["minute"] = relayData[relayIndex].schedules[i].minute;
    sched["isOn"] = relayData[relayIndex].schedules[i].isOnSchedule;
  }
  
  File file = LittleFS.open(filename, "w");
  if (!file) {
    Serial.println("Failed to open file for writing");
    return;
  }
  
  serializeJson(doc, file);
  file.close();
  Serial.println("Schedule saved for relay " + String(relayIndex));
}

void loadSchedule(int relayIndex) {
  if (relayIndex < 0 || relayIndex >= NUM_RELAYS) return;
  
  String filename = "/relay_" + String(relayIndex) + ".json";
  
  if (!LittleFS.exists(filename)) {
    Serial.println("Schedule file not found for relay " + String(relayIndex));
    return;
  }
  
  File file = LittleFS.open(filename, "r");
  if (!file) return;
  
  DynamicJsonDocument doc(2048);
  deserializeJson(doc, file);
  file.close();
  
  for (int i = 0; i < SCHEDULES_PER_RELAY; i++) {
    String key = "schedule_" + String(i);
    if (doc.containsKey(key)) {
      relayData[relayIndex].schedules[i].enabled = doc[key]["enabled"];
      relayData[relayIndex].schedules[i].hour = doc[key]["hour"];
      relayData[relayIndex].schedules[i].minute = doc[key]["minute"];
      relayData[relayIndex].schedules[i].isOnSchedule = doc[key]["isOn"];
    }
  }
  
  Serial.println("Schedule loaded for relay " + String(relayIndex));
}

void loadAllSchedules() {
  for (int i = 0; i < NUM_RELAYS; i++) {
    loadSchedule(i);
  }
}

void saveWiFiCredentials(String ssid, String password) {
  DynamicJsonDocument doc(512);
  doc["ssid"] = ssid;
  doc["password"] = password;
  
  File file = LittleFS.open("/wifi_config.json", "w");
  if (!file) return;
  
  serializeJson(doc, file);
  file.close();
  Serial.println("WiFi credentials saved");
}

bool loadWiFiCredentials(String &ssid, String &password) {
  if (!LittleFS.exists("/wifi_config.json")) {
    return false;
  }
  
  File file = LittleFS.open("/wifi_config.json", "r");
  if (!file) return false;
  
  DynamicJsonDocument doc(512);
  deserializeJson(doc, file);
  file.close();
  
  ssid = doc["ssid"].as<String>();
  password = doc["password"].as<String>();
  
  return (ssid.length() > 0);
}

// ==================== WIFI FUNCTIONS ====================

void setupWiFi() {
  String ssid, password;
  
  if (loadWiFiCredentials(ssid, password)) {
    Serial.println("Connecting to saved WiFi: " + ssid);
    WiFi.begin(ssid.c_str(), password.c_str());
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
      delay(500);
      Serial.print(".");
      attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\nWiFi connected!");
      Serial.println("IP: " + WiFi.localIP().toString());
      return;
    }
  }
  
  // If no saved credentials or connection failed, start AP mode
  Serial.println("Starting Access Point mode");
  WiFi.mode(WIFI_AP);
  WiFi.softAP("ESP8266_Relay_Setup", "12345678");
  Serial.println("AP IP: " + WiFi.softAPIP().toString());
}

// ==================== TIME SYNC FUNCTIONS ====================

void syncTime() {
  Serial.println("Syncing time with NTP server...");
  
  time_t now = time(nullptr);
  unsigned long timeout = millis() + 15000;
  
  configTime(GMT_OFFSET, DAYLIGHT_OFFSET, NTP_SERVER);
  
  while (time(nullptr) == now && millis() < timeout) {
    delay(100);
  }
  
  Serial.println();
  time_t newTime = time(nullptr);
  Serial.println("Time synced: " + String(ctime(&newTime)));
}

// ==================== RELAY CONTROL FUNCTIONS ====================

void initRelays() {
  for (int i = 0; i < NUM_RELAYS; i++) {
    pinMode(RELAY_PINS[i], OUTPUT);
    setRelay(i, false);
  }
}

void setRelay(int relayIndex, bool state) {
  if (relayIndex < 0 || relayIndex >= NUM_RELAYS) return;
  
  relayStates[relayIndex] = state;
  // Active LOW - set LOW to turn ON, HIGH to turn OFF (common for relay modules)
  digitalWrite(RELAY_PINS[relayIndex], state ? LOW : HIGH);
  Serial.println("Relay " + String(relayIndex) + " turned " + (state ? "ON" : "OFF"));
}

bool getRelay(int relayIndex) {
  if (relayIndex < 0 || relayIndex >= NUM_RELAYS) return false;
  return relayStates[relayIndex];
}

// ==================== SCHEDULE CHECKING ====================

void checkSchedules() {
  time_t now = time(nullptr);
  struct tm *timeinfo = localtime(&now);
  
  int currentHour = timeinfo->tm_hour;
  int currentMinute = timeinfo->tm_min;
  
  for (int relay = 0; relay < NUM_RELAYS; relay++) {
    for (int i = 0; i < SCHEDULES_PER_RELAY; i++) {
      Schedule &sched = relayData[relay].schedules[i];
      
      if (!sched.enabled) continue;
      
      // Check if current time matches schedule
      if (sched.hour == currentHour && sched.minute == currentMinute) {
        setRelay(relay, sched.isOnSchedule);
        Serial.println("Schedule triggered: Relay " + String(relay) + 
                      " set to " + (sched.isOnSchedule ? "ON" : "OFF"));
      }
    }
  }
}

// ==================== WEB SERVER HANDLERS ====================

void handleRoot() {
  String html = R"(
<!DOCTYPE html>
<html>
<head>
  <title>ESP8266 Relay Controller</title>
  <style>
    body {
      font-family: Arial, sans-serif;
      margin: 20px;
      background-color: #f0f0f0;
    }
    .container {
      max-width: 1200px;
      margin: 0 auto;
      background-color: white;
      padding: 20px;
      border-radius: 8px;
      box-shadow: 0 2px 4px rgba(0,0,0,0.1);
    }
    h1 {
      color: #333;
      text-align: center;
    }
    .section {
      margin: 20px 0;
      padding: 15px;
      border: 1px solid #ddd;
      border-radius: 4px;
    }
    .relay-control {
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(250px, 1fr));
      gap: 15px;
      margin: 20px 0;
    }
    .relay-card {
      border: 2px solid #007bff;
      padding: 15px;
      border-radius: 4px;
      background-color: #f9f9f9;
    }
    .relay-card h3 {
      margin: 0 0 10px 0;
      color: #007bff;
    }
    .toggle-btn {
      width: 100%;
      padding: 10px;
      margin: 5px 0;
      border: none;
      border-radius: 4px;
      cursor: pointer;
      font-size: 14px;
      font-weight: bold;
    }
    .toggle-btn.on {
      background-color: #28a745;
      color: white;
    }
    .toggle-btn.off {
      background-color: #dc3545;
      color: white;
    }
    .schedule-grid {
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
      gap: 10px;
      margin-top: 10px;
    }
    .schedule-item {
      border: 1px solid #ddd;
      padding: 10px;
      border-radius: 4px;
      background-color: white;
    }
    .schedule-item input {
      width: 100%;
      padding: 5px;
      margin: 5px 0;
      border: 1px solid #ddd;
      border-radius: 4px;
      box-sizing: border-box;
    }
    .schedule-item label {
      display: block;
      margin: 5px 0;
      font-size: 12px;
    }
    input[type="checkbox"] {
      margin-right: 5px;
    }
    button {
      background-color: #007bff;
      color: white;
      padding: 10px 20px;
      border: none;
      border-radius: 4px;
      cursor: pointer;
      font-size: 14px;
    }
    button:hover {
      background-color: #0056b3;
    }
    .status {
      padding: 10px;
      margin: 10px 0;
      border-radius: 4px;
      text-align: center;
    }
    .status.on {
      background-color: #d4edda;
      color: #155724;
      border: 1px solid #c3e6cb;
    }
    .status.off {
      background-color: #f8d7da;
      color: #721c24;
      border: 1px solid #f5c6cb;
    }
    .info-section {
      background-color: #e7f3ff;
      border: 1px solid #b3d9ff;
      padding: 10px;
      border-radius: 4px;
      margin: 10px 0;
    }
  </style>
</head>
<body>
  <div class="container">
    <h1>ESP8266 Relay Controller</h1>
    
    <div class="section">
      <h2>WiFi & Time</h2>
      <button onclick="syncTime()">Sync Time (NTP)</button>
      <button onclick="configureWiFi()">Configure WiFi</button>
      <div id="timeDisplay" class="info-section"></div>
    </div>
    
    <div class="section">
      <h2>Relay Control</h2>
      <div class="relay-control" id="relayControl"></div>
    </div>
  </div>
  
  <script>
    const NUM_RELAYS = 8;
    const SCHEDULES_PER_RELAY = 8;
    
    function updateTime() {
      const options = { year: 'numeric', month: '2-digit', day: '2-digit', hour: '2-digit', minute: '2-digit', second: '2-digit' };
      document.getElementById('timeDisplay').innerHTML = 'Current Time: ' + new Date().toLocaleString('en-US', options);
    }
    
    function initializeUI() {
      updateTime();
      setInterval(updateTime, 1000);
      loadRelayStates();
      loadSchedules();
    }
    
    function loadRelayStates() {
      fetch('/api/relays')
        .then(r => r.json())
        .then(data => {
          let html = '';
          for (let i = 0; i < NUM_RELAYS; i++) {
            const state = data.states[i];
            html += `
              <div class="relay-card">
                <h3>Relay ${i + 1}</h3>
                <div class="status ${state ? 'on' : 'off'}">
                  Status: ${state ? 'ON' : 'OFF'}
                </div>
                <button class="toggle-btn ${state ? 'on' : 'off'}" onclick="toggleRelay(${i})">
                  ${state ? 'Turn OFF' : 'Turn ON'}
                </button>
                <details style="margin-top: 10px;">
                  <summary style="cursor: pointer; color: #007bff;">Schedules</summary>
                  <div class="schedule-grid" id="schedules_${i}"></div>
                  <button onclick="saveSchedules(${i})" style="margin-top: 10px; width: 100%;">Save Schedules</button>
                </details>
              </div>
            `;
          }
          document.getElementById('relayControl').innerHTML = html;
        });
    }
    
    function loadSchedules() {
      for (let relay = 0; relay < NUM_RELAYS; relay++) {
        fetch(`/api/schedule/${relay}`)
          .then(r => r.json())
          .then(data => {
            let html = '';
            for (let i = 0; i < SCHEDULES_PER_RELAY; i++) {
              const sched = data.schedules[i];
              html += `
                <div class="schedule-item">
                  <label>
                    <input type="checkbox" id="enabled_${relay}_${i}" ${sched.enabled ? 'checked' : ''}>
                    Enabled
                  </label>
                  <label>Time:
                    <input type="number" id="hour_${relay}_${i}" min="0" max="23" value="${sched.hour}" placeholder="Hour">:
                    <input type="number" id="minute_${relay}_${i}" min="0" max="59" value="${sched.minute}" placeholder="Min" style="width: 40%;">
                  </label>
                  <label>
                    <input type="radio" name="action_${relay}_${i}" value="1" ${sched.isOn ? 'checked' : ''}>
                    Turn ON
                  </label>
                  <label>
                    <input type="radio" name="action_${relay}_${i}" value="0" ${!sched.isOn ? 'checked' : ''}>
                    Turn OFF
                  </label>
                </div>
              `;
            }
            document.getElementById(`schedules_${relay}`).innerHTML = html;
          });
      }
    }
    
    function toggleRelay(index) {
      fetch(`/api/toggle/${index}`, { method: 'POST' })
        .then(() => loadRelayStates());
    }
    
    function saveSchedules(relayIndex) {
      const schedules = [];
      for (let i = 0; i < SCHEDULES_PER_RELAY; i++) {
        schedules.push({
          enabled: document.getElementById(`enabled_${relayIndex}_${i}`).checked,
          hour: parseInt(document.getElementById(`hour_${relayIndex}_${i}`).value),
          minute: parseInt(document.getElementById(`minute_${relayIndex}_${i}`).value),
          isOn: document.querySelector(`input[name="action_${relayIndex}_${i}"]:checked`).value === '1'
        });
      }
      
      fetch(`/api/schedule/${relayIndex}`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ schedules })
      }).then(() => alert('Schedules saved!'));
    }
    
    function syncTime() {
      fetch('/api/sync-time', { method: 'POST' })
        .then(r => r.json())
        .then(data => alert(data.message));
    }
    
    function configureWiFi() {
      const ssid = prompt('Enter WiFi SSID:');
      if (!ssid) return;
      const password = prompt('Enter WiFi Password:');
      if (password === null) return;
      
      fetch('/api/wifi-config', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ ssid, password })
      }).then(r => r.json())
        .then(data => alert(data.message + ' Device will restart...'));
    }
    
    window.onload = initializeUI;
  </script>
</body>
</html>
  )";
  
  server.send(200, "text/html", html);
}

void handleGetRelays() {
  DynamicJsonDocument doc(512);
  JsonArray states = doc.createNestedArray("states");
  
  for (int i = 0; i < NUM_RELAYS; i++) {
    states.add(relayStates[i]);
  }
  
  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

void handleToggleRelay() {
  if (server.argName(0) != "index") {
    server.send(400, "text/plain", "Bad request");
    return;
  }
  
  int index = server.arg("index").toInt();
  if (index < 0 || index >= NUM_RELAYS) {
    server.send(400, "text/plain", "Invalid relay index");
    return;
  }
  
  setRelay(index, !relayStates[index]);
  server.send(200, "text/plain", "OK");
}

void handleGetSchedule() {
  if (server.pathArg(0).length() == 0) {
    server.send(400, "text/plain", "Bad request");
    return;
  }
  
  int relayIndex = server.pathArg(0).toInt();
  if (relayIndex < 0 || relayIndex >= NUM_RELAYS) {
    server.send(400, "text/plain", "Invalid relay index");
    return;
  }
  
  DynamicJsonDocument doc(2048);
  JsonArray schedules = doc.createNestedArray("schedules");
  
  for (int i = 0; i < SCHEDULES_PER_RELAY; i++) {
    JsonObject sched = schedules.createNestedObject();
    sched["enabled"] = relayData[relayIndex].schedules[i].enabled;
    sched["hour"] = relayData[relayIndex].schedules[i].hour;
    sched["minute"] = relayData[relayIndex].schedules[i].minute;
    sched["isOn"] = relayData[relayIndex].schedules[i].isOnSchedule;
  }
  
  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

void handlePostSchedule() {
  if (server.pathArg(0).length() == 0) {
    server.send(400, "text/plain", "Bad request");
    return;
  }
  
  int relayIndex = server.pathArg(0).toInt();
  if (relayIndex < 0 || relayIndex >= NUM_RELAYS) {
    server.send(400, "text/plain", "Invalid relay index");
    return;
  }
  
  if (server.hasArg("plain")) {
    DynamicJsonDocument doc(2048);
    deserializeJson(doc, server.arg("plain"));
    
    if (doc.containsKey("schedules")) {
      JsonArray schedules = doc["schedules"];
      for (int i = 0; i < schedules.size() && i < SCHEDULES_PER_RELAY; i++) {
        relayData[relayIndex].schedules[i].enabled = schedules[i]["enabled"];
        relayData[relayIndex].schedules[i].hour = schedules[i]["hour"];
        relayData[relayIndex].schedules[i].minute = schedules[i]["minute"];
        relayData[relayIndex].schedules[i].isOnSchedule = schedules[i]["isOn"];
      }
      
      saveSchedule(relayIndex);
      server.send(200, "text/plain", "Schedule saved");
      return;
    }
  }
  
  server.send(400, "text/plain", "Invalid request");
}

void handleSyncTime() {
  syncTime();
  DynamicJsonDocument doc(256);
  doc["message"] = "Time synchronized";
  
  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

void handleWiFiConfig() {
  if (server.hasArg("plain")) {
    DynamicJsonDocument doc(512);
    deserializeJson(doc, server.arg("plain"));
    
    String ssid = doc["ssid"];
    String password = doc["password"];
    
    if (ssid.length() > 0 && password.length() > 0) {
      saveWiFiCredentials(ssid, password);
      
      DynamicJsonDocument response(256);
      response["message"] = "WiFi credentials saved";
      
      String responseStr;
      serializeJson(response, responseStr);
      server.send(200, "application/json", responseStr);
      
      delay(1000);
      ESP.restart();
      return;
    }
  }
  
  server.send(400, "text/plain", "Invalid request");
}

void setupWebServer() {
  server.on("/", handleRoot);
  server.on("/api/relays", handleGetRelays);
  
  server.on("/api/toggle", [](){ 
    if (server.hasArg("index")) {
      int index = server.arg("index").toInt();
      setRelay(index, !relayStates[index]);
      server.send(200, "text/plain", "OK");
    }
  });
  
  server.on("/api/schedule/", HTTP_GET, handleGetSchedule);
  server.on("/api/schedule/", HTTP_POST, handlePostSchedule);
  server.on("/api/sync-time", HTTP_POST, handleSyncTime);
  server.on("/api/wifi-config", HTTP_POST, handleWiFiConfig);
  
  server.begin();
  Serial.println("Web server started");
}

// ==================== SETUP ====================

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n\n=== ESP8266 Relay Controller Starting ===\n");
  
  initFilesystem();
  initRelays();
  setupWiFi();
  setupWebServer();
  syncTime();
  
  Serial.println("\n=== Setup Complete ===\n");
}

// ==================== MAIN LOOP ====================

void loop() {
  server.handleClient();
  
  // Check schedules every minute
  if (millis() - lastScheduleCheck >= SCHEDULE_CHECK_INTERVAL) {
    lastScheduleCheck = millis();
    checkSchedules();
  }
  
  delay(10);
}
