# ESP32 8-Channel Relay Scheduler

A complete ESP32 program with web interface for controlling 8 relays based on time schedules.

## Features

✅ **WiFi Configuration** - Set WiFi credentials via web interface
✅ **AP Mode** - Automatic fallback to Access Point mode if WiFi fails
✅ **NTP Time Sync** - Automatic time synchronization over internet
✅ **8 Relay Channels** - Control 8 independent relays
✅ **128 Schedules** - Each relay has 8 ON and 8 OFF schedules (16 schedules × 8 relays)
✅ **Manual Control** - Override schedules with manual on/off control
✅ **Persistent Storage** - All settings saved to ESP32 flash memory
✅ **Modern Web UI** - Responsive interface works on phone, tablet, or computer
✅ **Timezone Support** - Configure your local timezone offset

## Hardware Requirements

1. **ESP32 Development Board** (any variant)
2. **8-Channel Relay Module** (5V or 3.3V compatible)
3. **Power Supply** (appropriate for your relay module and loads)
4. **Jumper Wires**

## Pin Connections

Default GPIO pins for relays (you can change these in the code):

| Relay | GPIO Pin |
|-------|----------|
| 1     | GPIO 13  |
| 2     | GPIO 12  |
| 3     | GPIO 14  |
| 4     | GPIO 27  |
| 5     | GPIO 26  |
| 6     | GPIO 25  |
| 7     | GPIO 33  |
| 8     | GPIO 32  |

### Wiring Example
```
ESP32          8-Channel Relay Module
------         ---------------------
GPIO 13   ---> IN1
GPIO 12   ---> IN2
GPIO 14   ---> IN3
GPIO 27   ---> IN4
GPIO 26   ---> IN5
GPIO 25   ---> IN6
GPIO 33   ---> IN7
GPIO 32   ---> IN8
GND       ---> GND
5V/3.3V   ---> VCC (check your relay module voltage)
```

## Installation

### 1. Install Arduino IDE

Download from: https://www.arduino.cc/en/software

### 2. Install ESP32 Board Support

1. Open Arduino IDE
2. Go to **File → Preferences**
3. In "Additional Board Manager URLs", add:
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
4. Go to **Tools → Board → Boards Manager**
5. Search for "esp32" and install "esp32 by Espressif Systems"

### 3. Upload the Program

1. Open `ESP32_Relay_Scheduler.ino` in Arduino IDE
2. Select your board: **Tools → Board → ESP32 Arduino → ESP32 Dev Module** (or your specific board)
3. Select your port: **Tools → Port → COM# or /dev/ttyUSB#**
4. Click **Upload** button (→)

### 4. Monitor Serial Output (Optional)

1. Open **Tools → Serial Monitor**
2. Set baud rate to **115200**
3. You'll see connection status and IP address

## First Time Setup

### 1. Connect to ESP32 Access Point

After uploading the code, the ESP32 will start in AP mode:

- **WiFi Network Name:** `ESP32-Relay-Config`
- **Password:** `12345678`

Connect your phone or computer to this network.

### 2. Access Web Interface

Open a web browser and go to:
```
http://192.168.4.1
```

### 3. Configure WiFi

1. In the "WiFi Configuration" section:
   - Enter your WiFi network name (SSID)
   - Enter your WiFi password
   - Set your timezone offset (e.g., -5 for EST, 0 for UTC, +8 for China)
2. Click "Save WiFi Settings"
3. The ESP32 will restart and connect to your WiFi

### 4. Find New IP Address

After restart, check your router's DHCP client list or Serial Monitor to find the new IP address. You can also use a network scanner app.

## Usage

### Accessing the Web Interface

Once connected to WiFi, access the interface at the ESP32's IP address:
```
http://192.168.1.xxx
```

### Setting Up Schedules

1. **Select Relay** - Choose which relay to configure (1-8)
2. **ON Schedules** - Set up to 8 times when the relay turns ON
3. **OFF Schedules** - Set up to 8 times when the relay turns OFF
4. **Enable/Disable** - Check the box to enable each schedule
5. **Set Time** - Enter hour (0-23) and minute (0-59)
6. **Save** - Click "Save Schedules" to store settings

### Manual Control

Click the ON/OFF button next to any relay to manually control it, overriding schedules.

### Example Schedule

For a pump that runs twice daily:
- **Relay 1 - ON Schedule 1:** 06:00 (6:00 AM)
- **Relay 1 - OFF Schedule 1:** 06:15 (6:15 AM)
- **Relay 1 - ON Schedule 2:** 18:00 (6:00 PM)
- **Relay 1 - OFF Schedule 2:** 18:15 (6:15 PM)

## Troubleshooting

### Can't Connect to WiFi

1. Check Serial Monitor for error messages
2. Verify SSID and password are correct
3. Make sure ESP32 is in range of WiFi router
4. Reset to factory defaults and try again

### Wrong Time Displayed

1. Check timezone offset setting
2. Verify internet connection (NTP requires internet)
3. Wait a few minutes for time sync

### Relays Not Switching

1. Check wiring connections
2. Verify relay module voltage (5V or 3.3V)
3. Check GPIO pin definitions match your wiring
4. Test with manual control first
5. Check schedule times are correct

### Reset to Factory Defaults

1. Access web interface
2. Scroll to "System" section
3. Click "Reset to Factory Defaults"
4. Device will restart in AP mode

## Customization

### Change Relay Pins

Edit this line in the code:
```cpp
const int RELAY_PINS[8] = {13, 12, 14, 27, 26, 25, 33, 32};
```

### Change AP Credentials

Edit these lines:
```cpp
const char* ap_ssid = "ESP32-Relay-Config";
const char* ap_password = "12345678";
```

### Change NTP Server

Edit this line:
```cpp
const char* ntpServer = "pool.ntp.org";
```

You can use regional servers like:
- `time.nist.gov` (US)
- `time.windows.com` (Microsoft)
- `pool.ntp.org` (Global pool)

### Relay Logic (Active High/Low)

If your relay module is active LOW (turns on with LOW signal), change:
```cpp
digitalWrite(RELAY_PINS[relay], state ? HIGH : LOW);
```
to:
```cpp
digitalWrite(RELAY_PINS[relay], state ? LOW : HIGH);
```

## Technical Details

### Memory Usage

- **Flash:** ~1MB (program code and web interface)
- **SRAM:** ~50KB runtime
- **Preferences:** ~8KB (WiFi credentials + schedules)

### Storage Format

All settings are stored in ESP32's NVS (Non-Volatile Storage):
- WiFi SSID and password
- Timezone offset
- All relay schedules (8 relays × 16 schedules each)

### Schedule Checking

- Schedules are checked every second
- Relays trigger only at the exact minute (when seconds = 0)
- If multiple schedules have the same time, the last one processed takes effect

## Safety Notes

⚠️ **Important Safety Information:**

1. **Electrical Safety**
   - Only connect loads within relay ratings
   - Use appropriate fuses and circuit breakers
   - Never work on live circuits
   - Ensure proper insulation

2. **Relay Ratings**
   - Check your relay module specifications
   - Don't exceed maximum voltage/current ratings
   - Use contactors for high-power loads

3. **Backup Power**
   - Consider UPS for critical applications
   - ESP32 will lose time if power is lost
   - Time will resync when WiFi reconnects

4. **Testing**
   - Test all schedules thoroughly
   - Verify correct relay operation
   - Have manual override capability

## License

This code is provided as-is for educational and personal use.

## Support

For issues or questions:
1. Check Serial Monitor output for debugging
2. Verify all connections and settings
3. Review this documentation thoroughly

---

**Version:** 1.0  
**Last Updated:** 2026-02-13  
**Compatible with:** ESP32 Arduino Core 2.x and 3.x
