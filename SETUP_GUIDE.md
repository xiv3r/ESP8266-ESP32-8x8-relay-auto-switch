# ESP8266 8-Channel Relay Controller - Setup Guide

## Features

✅ **Web-Based Configuration** - Beautiful responsive web UI accessible from any browser  
✅ **WiFi Management** - Configure WiFi SSID and password through the web interface  
✅ **Time Synchronization** - Automatic NTP time sync from the internet  
✅ **8-Channel Relay Control** - Individual control of up to 8 relays  
✅ **Advanced Scheduling** - 8 ON schedules + 8 OFF schedules per relay (16 schedules total per relay)  
✅ **Persistent Storage** - Schedules saved to LittleFS and retained after reboot  
✅ **Real-time Control** - Manual relay toggle via web interface  
✅ **Status Monitoring** - Live relay status display  

---

## Hardware Requirements

### Components
- **ESP8266** (NodeMCU, D1 Mini, or similar)
- **8-Channel Relay Module** (5V or 3.3V depending on your module)
- **Power Supply** (appropriate for relays)
- **Micro USB Cable** (for programming and power)
- **Dupont Wires** (for connections)
- **Optional**: External power supply for relays

### Pin Configuration

The default pin assignments use the common ESP8266 pin names:
```
D0 = GPIO16 → Relay 1
D1 = GPIO5  → Relay 2
D2 = GPIO4  → Relay 3
D3 = GPIO0  → Relay 4
D4 = GPIO2  → Relay 5
D5 = GPIO14 → Relay 6
D6 = GPIO12 → Relay 7
D7 = GPIO13 → Relay 8
```

**To change pins:** Edit line 12-13 in the .ino file:
```cpp
const int RELAY_PINS[NUM_RELAYS] = {D0, D1, D2, D3, D4, D5, D6, D7};
```

### Relay Module Wiring

Most relay modules use **Active LOW** logic (default in this code):
- **HIGH** = Relay OFF
- **LOW** = Relay ON

**Connection:**
1. Connect each relay module IN pin to corresponding ESP8266 GPIO pin
2. Connect relay GND to ESP8266 GND
3. Connect relay VCC to power (usually 5V)
4. Connect your devices to the relay NO/C/NC terminals

> ⚠️ **Safety Note:** If your relay module uses Active HIGH logic, change line 175:
> ```cpp
> digitalWrite(RELAY_PINS[relayIndex], state ? HIGH : LOW);
> ```

---

## Software Setup

### 1. Arduino IDE Configuration

**Install ESP8266 Board Support:**
1. File → Preferences
2. Add this URL to "Additional Boards Manager URLs":
   ```
   http://arduino.esp8266.com/stable/package_esp8266com_index.json
   ```
3. Tools → Board Manager → Search "ESP8266" → Install latest version

### 2. Required Libraries

Install these libraries via Arduino IDE (Sketch → Include Library → Manage Libraries):

| Library | Version | Author |
|---------|---------|--------|
| ArduinoJson | 7.0+ | Benoit Blanchon |
| (ESP8266 core has built-in WiFi & Web Server) | | |

**Installation Steps:**
1. Sketch → Include Library → Manage Libraries
2. Search for "ArduinoJson"
3. Click Install (select version 7.0 or higher)

### 3. Board Selection

In Arduino IDE:
- Tools → Board → ESP8266 Boards → NodeMCU 1.0 (or your specific model)
- Tools → Port → COM[X] (your device's port)
- Tools → Upload Speed → 115200

### 4. Upload the Code

1. Copy the ESP8266_Relay_Controller.ino code into Arduino IDE
2. Click Upload (→ button)
3. Wait for "Done uploading" message
4. Open Serial Monitor (Tools → Serial Monitor, 115200 baud) to see debug output

---

## First Time Setup

### Step 1: Connect via WiFi

When the device starts for the first time:

1. **No saved WiFi credentials?** It automatically starts in Access Point (AP) mode
2. Look for WiFi network: `ESP8266_Relay_Setup`
3. Password: `12345678`
4. Connect to this network
5. Open browser and go to: `http://192.168.4.1`

### Step 2: Configure WiFi

1. Click "Configure WiFi" button
2. Enter your WiFi SSID (network name)
3. Enter your WiFi password
4. Click OK
5. Device automatically restarts and connects to your network
6. Open Serial Monitor to find the assigned IP address

### Step 3: Access the Web Interface

1. In Serial Monitor, look for: `IP: 192.168.X.X`
2. Open browser and navigate to this IP
3. You should see the relay control dashboard

---

## Web Interface Guide

### Dashboard Features

**WiFi & Time Section:**
- **Sync Time (NTP)** - Manually sync current time from internet
- **Configure WiFi** - Change WiFi credentials
- Time display updates in real-time

**Relay Control Section:**
- Each relay shows current status (ON/OFF)
- Manual toggle buttons for each relay
- Expandable schedule section for each relay

### Schedule Configuration

For each relay, you can set **8 ON schedules** and **8 OFF schedules**:

1. Click "Schedules" under the desired relay
2. For each schedule:
   - **Enable checkbox** - Turn schedule on/off
   - **Time fields** - Set hour (0-23) and minute (0-59)
   - **Action** - Select "Turn ON" or "Turn OFF"
3. Click "Save Schedules"

**Example Schedule:**
- Relay 1, Schedule 1: 08:00 → Turn ON (lights on at 8 AM)
- Relay 1, Schedule 2: 18:00 → Turn OFF (lights off at 6 PM)

The schedules are checked every minute, so timing is to the nearest minute.

---

## Configuration Options

### Timezone/GMT Offset

Edit lines 11-12 to set your timezone:

```cpp
#define GMT_OFFSET 0  // UTC (change for your timezone)
#define DAYLIGHT_OFFSET 0  // Daylight saving offset
```

**Common Timezones:**
- **EST (UTC-5):** `-5*3600` and `3600` (daylight)
- **CST (UTC-6):** `-6*3600` and `3600` (daylight)
- **MST (UTC-7):** `-7*3600` and `3600` (daylight)
- **PST (UTC-8):** `-8*3600` and `3600` (daylight)
- **GMT (UTC):** `0` and `0`
- **CET (UTC+1):** `3600` and `3600` (daylight)
- **IST (UTC+5:30):** `19800` and `0`

### Relay Pin Configuration

Change line 13 to use different GPIO pins:

```cpp
const int RELAY_PINS[NUM_RELAYS] = {D0, D1, D2, D3, D4, D5, D6, D7};
```

### Schedule Check Interval

To change how often schedules are evaluated (default: 60 seconds):

```cpp
const unsigned long SCHEDULE_CHECK_INTERVAL = 60000;  // milliseconds
```

### NTP Server

To use a different NTP server (line 10):

```cpp
#define NTP_SERVER "time.nist.gov"  // Or other NTP server
```

---

## Troubleshooting

### Device won't connect to WiFi

1. **Check Serial Monitor** for error messages
2. Verify SSID and password are correct
3. Try **Configure WiFi** again
4. Reset device with GPIO16 to ground briefly
5. Check if 2.4GHz WiFi is available (ESP8266 doesn't support 5GHz)

### Schedules aren't triggering

1. Check that schedules are **enabled** (checkbox marked)
2. Verify time is synced (click "Sync Time" button)
3. Open Serial Monitor and look for `Schedule triggered` messages
4. Check that relay is physically connected properly

### Relay won't turn on/off

1. **Check wiring** - Verify GPIO pin connections
2. **Test with manual toggle** from web interface
3. **Check power supply** - Relay modules need 5V power
4. If using 3.3V module, verify compatibility
5. Try changing active level if needed (see Hardware section)

### Web interface won't load

1. **Check IP address** - Verify correct IP in Serial Monitor
2. **Check WiFi connection** - Device should be on same network
3. **Check firewall** - Port 80 should be accessible
4. **Restart device** - Power cycle the ESP8266
5. **Clear browser cache** - Hard refresh (Ctrl+Shift+R or Cmd+Shift+R)

### Lost connection to device

1. Device will retain WiFi credentials and reconnect automatically
2. Find new IP in Serial Monitor or check WiFi router
3. If stuck in AP mode, reconfigure WiFi credentials
4. Check if WiFi password changed

### Time won't synchronize

1. **Check internet connection** - Device must be connected to WiFi
2. **Check firewall** - NTP port 123 must be open
3. **Try different NTP server** - Edit NTP_SERVER constant
4. **Check GMT offset** - Verify timezone is correct

---

## Storage & Persistence

The device uses **LittleFS** for storage:

- **WiFi credentials** saved in: `/wifi_config.json`
- **Relay schedules** saved in: `/relay_0.json` through `/relay_7.json`
- All data persists through power cycles and reboots
- ~1MB of storage available on typical ESP8266

To **clear all stored data**, use Arduino IDE:
- Tools → ESP8266 Sketch Data Upload (with LittleFS Data Upload plugin)
- Or delete files through web interface (not currently implemented)

---

## Advanced Usage

### Accessing Schedules via API

You can retrieve schedules using HTTP requests:

```bash
# Get relay 0 schedules
GET http://192.168.X.X/api/schedule/0

# Get all relay states
GET http://192.168.X.X/api/relays

# Toggle relay 0
POST http://192.168.X.X/api/toggle?index=0

# Update schedules for relay 0
POST http://192.168.X.X/api/schedule/0
Content-Type: application/json

{
  "schedules": [
    {
      "enabled": true,
      "hour": 8,
      "minute": 0,
      "isOn": true
    },
    ...
  ]
}
```

### Extending Functionality

The code structure allows easy modifications:

1. **Add more relays** - Increase `NUM_RELAYS` and add pins
2. **Add temperature sensor** - Include DHT library, add to web interface
3. **Add motion detection** - Trigger relays based on PIR sensor
4. **MQTT integration** - Add PubSubClient library
5. **SMS/Email notifications** - Use email service API

---

## Safety Recommendations

⚠️ **Important:**

1. **High Voltage**: Use relays rated for your load
2. **Power Supply**: Use proper PSU with appropriate rating
3. **Wiring**: Use appropriate gauge wires for current
4. **Heat**: Ensure relays don't overheat with thermal dissipation
5. **Isolation**: Isolate high voltage circuits from low voltage
6. **Testing**: Test before deploying in production
7. **Backup**: Save schedule configurations regularly
8. **Security**: Change AP password if used in unsecured location

---

## Performance Specifications

- **Max GPIO Pins**: 8 (limited by relay count in code)
- **Response Time**: ~50-100ms for schedule checks
- **Max Web Connections**: 4 concurrent
- **Schedule Precision**: ±1 minute
- **Memory**: ~50KB used for schedules
- **Power**: ~80mA at 5V (device only, relays require separate power)

---

## Support & Debugging

Enable verbose Serial output for debugging:

```cpp
// Add after Serial.begin()
Serial.setDebugOutput(true);  // Show WiFi debug info
```

### Serial Monitor Output Example

```
=== ESP8266 Relay Controller Starting ===

LittleFS mounted successfully
Connecting to saved WiFi: MyNetwork
.........................
WiFi connected!
IP: 192.168.1.100
Syncing time with NTP server...
Time synced: Wed Feb 13 14:30:45 2026
Web server started

=== Setup Complete ===

Schedule triggered: Relay 0 set to ON
```

---

## Version History

- **v1.0** - Initial release
  - 8-channel relay control
  - WiFi configuration via web UI
  - NTP time synchronization
  - 8 ON + 8 OFF schedules per relay
  - LittleFS persistent storage

---

## License

This code is provided as-is for personal and commercial use. Modify as needed for your projects.

---

## Questions?

Refer to the inline code comments for detailed function descriptions, or check the ESP8266 community forums for WiFi and hardware-specific questions.

Happy automating! 🚀
