# ESP8266 Relay Controller - Quick Reference Card

## Quick Start (3 Steps)

### Step 1: Upload Code
```
Arduino IDE → Upload ESP8266_Relay_Controller.ino
```

### Step 2: Connect to Device
```
WiFi: ESP8266_Relay_Setup
Password: 12345678
Open: http://192.168.4.1
```

### Step 3: Configure & Schedule
```
→ Click "Configure WiFi" → Enter your WiFi details
→ Click "Sync Time" to get current time
→ Set schedules for each relay
```

---

## Web Interface URLs

| Action | URL |
|--------|-----|
| Main Dashboard | `http://192.168.1.X` |
| Get Relay States | `http://192.168.1.X/api/relays` |
| Get Schedule | `http://192.168.1.X/api/schedule/0` (0-7 for relays) |
| Toggle Relay | `POST http://192.168.1.X/api/toggle?index=0` |
| Update Schedule | `POST http://192.168.1.X/api/schedule/0` |
| Sync Time | `POST http://192.168.1.X/api/sync-time` |
| Configure WiFi | `POST http://192.168.1.X/api/wifi-config` |

---

## Schedule Setup Examples

### Example 1: Light Timer (Turn ON at 6am, OFF at 10pm)

Relay 1:
```
Schedule 1: 06:00 → Turn ON
Schedule 2: 22:00 → Turn OFF
```

### Example 2: Fan Control (Multiple On/Off Times)

Relay 2:
```
Schedule 1: 07:00 → Turn ON
Schedule 2: 12:00 → Turn OFF
Schedule 3: 17:00 → Turn ON
Schedule 4: 22:00 → Turn OFF
```

### Example 3: Pump with Multiple Cycles

Relay 3:
```
Schedule 1: 06:00 → Turn ON
Schedule 2: 06:30 → Turn OFF
Schedule 3: 12:00 → Turn ON
Schedule 4: 12:30 → Turn OFF
Schedule 5: 18:00 → Turn ON
Schedule 6: 18:30 → Turn OFF
```

---

## GPIO Pin Map

| Label | GPIO | Relay |
|-------|------|-------|
| D0 | GPIO16 | Relay 1 |
| D1 | GPIO5 | Relay 2 |
| D2 | GPIO4 | Relay 3 |
| D3 | GPIO0 | Relay 4 |
| D4 | GPIO2 | Relay 5 |
| D5 | GPIO14 | Relay 6 |
| D6 | GPIO12 | Relay 7 |
| D7 | GPIO13 | Relay 8 |

**Note:** D8, D11 are reserved. Avoid using:
- GPIO6-11 (flash memory)
- GPIO16 (deep sleep only)

---

## Timezone Settings

Add these to your code before upload:

```cpp
// UTC
#define GMT_OFFSET 0

// US Eastern (EST/EDT)
#define GMT_OFFSET (-5*3600)
#define DAYLIGHT_OFFSET 3600

// US Central (CST/CDT)
#define GMT_OFFSET (-6*3600)
#define DAYLIGHT_OFFSET 3600

// US Mountain (MST/MDT)
#define GMT_OFFSET (-7*3600)
#define DAYLIGHT_OFFSET 3600

// US Pacific (PST/PDT)
#define GMT_OFFSET (-8*3600)
#define DAYLIGHT_OFFSET 3600

// Europe Central (CET/CEST)
#define GMT_OFFSET 3600
#define DAYLIGHT_OFFSET 3600

// India Standard Time
#define GMT_OFFSET (5*3600 + 30*60)
#define DAYLIGHT_OFFSET 0

// Australia Sydney (AEDT)
#define GMT_OFFSET (10*3600)
#define DAYLIGHT_OFFSET 3600

// Japan
#define GMT_OFFSET (9*3600)
#define DAYLIGHT_OFFSET 0
```

---

## Common Issues & Quick Fixes

| Problem | Quick Fix |
|---------|-----------|
| WiFi won't connect | Check SSID/password, restart device, verify 2.4GHz band |
| Schedules don't trigger | Click "Sync Time", verify schedule is enabled, check hour:minute |
| Relay won't toggle | Check GPIO wiring, verify power supply, test manual toggle first |
| Can't access dashboard | Check device IP in serial monitor, same WiFi network |
| Time is wrong | Click "Sync Time" button, check internet connection |
| Page loads slowly | Check WiFi signal strength, reload page |
| Schedules disappear after restart | Check LittleFS is working, verify "Save Schedules" was clicked |

---

## Serial Monitor Debugging

**Enable Serial Monitor:** Tools → Serial Monitor (115200 baud)

**What to look for:**
```
"WiFi connected!" → Device connected to WiFi
"IP: 192.168.X.X" → Your device's IP address
"Time synced:" → NTP time update successful
"Schedule triggered:" → Schedule was executed
"Relay X turned ON/OFF" → Manual control confirmed
```

---

## Relay Logic (Active LOW)

| GPIO State | Relay State |
|-----------|------------|
| LOW (0V) | ON ✓ |
| HIGH (3.3V) | OFF ✗ |

**If relays are inverted,** change this line:
```cpp
// Current (Active LOW):
digitalWrite(RELAY_PINS[relayIndex], state ? LOW : HIGH);

// Change to (Active HIGH):
digitalWrite(RELAY_PINS[relayIndex], state ? HIGH : LOW);
```

---

## API Examples

### Get Current Relay States (JavaScript)
```javascript
fetch('http://192.168.1.100/api/relays')
  .then(r => r.json())
  .then(data => console.log(data.states));
```

### Toggle Relay 0 (cURL)
```bash
curl -X POST http://192.168.1.100/api/toggle?index=0
```

### Set All Relays ON at 8am (cURL)
```bash
curl -X POST http://192.168.1.100/api/schedule/0 \
  -H "Content-Type: application/json" \
  -d '{"schedules":[{"enabled":true,"hour":8,"minute":0,"isOn":true}]}'
```

---

## Hardware Checklist

- [ ] ESP8266 board (NodeMCU, D1 Mini, etc.)
- [ ] 8-channel relay module
- [ ] Micro USB cable for programming
- [ ] 5V power supply for relays
- [ ] Dupont wires for connections
- [ ] 330Ω resistors (optional, for GPIO protection)
- [ ] Breadboard or PCB for assembly

---

## Wiring Quick Guide

```
ESP8266              Relay Module
GND        ------→   GND
D0         ------→   IN1
D1         ------→   IN2
D2         ------→   IN3
D3         ------→   IN4
D4         ------→   IN5
D5         ------→   IN6
D6         ------→   IN7
D7         ------→   IN8
(separate) ------→   VCC (5V)
```

---

## Performance Tips

1. **Faster WiFi Connection:** Hardcode WiFi channel in code
2. **Reduce Schedule Check Time:** Decrease SCHEDULE_CHECK_INTERVAL
3. **Improve Relay Response:** Use GPIO with lower multiplexing (D5, D6, D7)
4. **Lower Power Usage:** Reduce WiFi TX power with `WiFi.setTxPower()`
5. **Stable Schedule:** Use UTP server closer to your location

---

## File Locations

When stored on ESP8266 LittleFS:
```
/wifi_config.json      ← WiFi credentials
/relay_0.json          ← Relay 1 schedules
/relay_1.json          ← Relay 2 schedules
...
/relay_7.json          ← Relay 8 schedules
```

**Total Size:** ~5KB for all schedules

---

## Safety Checklist

- [ ] Use appropriate wire gauge for current
- [ ] Relay module rated for your voltage/current
- [ ] 5V power supply properly grounded
- [ ] High voltage circuits isolated from control circuit
- [ ] Heat sinks on relays if needed
- [ ] Tested schedules before production use
- [ ] Backup schedule configurations

---

## Useful Arduino IDE Shortcuts

| Shortcut | Action |
|----------|--------|
| Ctrl+U | Upload |
| Ctrl+Shift+M | Serial Monitor |
| Ctrl+K | Clear Console |
| Ctrl+/ | Comment/Uncomment |
| Ctrl+T | Auto Format |

---

## Next Steps

1. ✓ Upload code and connect
2. ✓ Configure WiFi
3. ✓ Sync time from internet
4. ✓ Test manual relay controls
5. ✓ Set up first schedule
6. ✓ Monitor schedules triggering in Serial Monitor
7. ✓ Deploy to production

---

**Need Help?** Check SETUP_GUIDE.md for detailed troubleshooting
