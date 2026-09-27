# IoT-Based Wireless Home Automation System with Voice Control

A voice-controlled home automation system built on ESP32 — controls lights,
fan speed, and an AC unit over Bluetooth speech commands, with an
autonomous mode driven by motion and temperature sensors.

**[Live simulation on Wokwi ->](https://wokwi.com/projects/476346443085018113)**
(runs the full control logic in-browser, no hardware required)

---

## Repo structure

```
├── firmware/
│   └── home_automation_voice_control.ino   # Real hardware build:
│                                             #   BluetoothSerial + relays +
│                                             #   AC dimmer + IR AC control
│
├── wokwi-simulation/
│   ├── sketch.ino                          # Simulation build:
│   │                                         #   Serial (typed) commands +
│   │                                         #   LEDs standing in for loads
│   ├── diagram.json                        # Wokwi circuit definition
│   └── libraries.txt                       # Library dependencies
│
└── README.md
```

## Why two versions?

The `firmware/` sketch is the real-hardware target: `BluetoothSerial` for
receiving voice-recognized text from a phone app, relay modules for the
lights, an AC dimmer module (`RBDdimmer`) for variable fan speed, and IR
(`IRremoteESP8266`) to drive the AC unit.

The `wokwi-simulation/` sketch is functionally identical control logic —
same command parser, same state machine, same auto-mode rules — but swaps
`BluetoothSerial` for plain `Serial` and relays/dimmer/IR for LEDs, so the
whole thing can be verified end-to-end in [Wokwi](https://wokwi.com)
without any physical parts. Useful for validating logic before hardware
assembly, or for a quick demo.

---

## Hardware (real build)

| Component                              | Qty | Purpose                                  |
|------------------------------------------|-----|--------------------------------------------|
| ESP32 DevKit v1 (30-pin)                | 1   | Main controller (WiFi + BT built in)       |
| 2-channel 5V relay module (active-low)  | 1   | Switching Light 1 & Light 2                |
| RobotDyn AC Dimmer Module                | 1   | Variable fan speed control (3 levels)      |
| IR LED (940nm) + 100Ω resistor          | 1   | Transmit AC remote signals                 |
| PIR motion sensor (HC-SR501)            | 1   | Auto mode: motion-based lighting           |
| DHT11 temp/humidity sensor              | 1   | Auto mode: AC safety cutoff                |

**App side:** any free "Bluetooth Voice Control for Arduino" Android app —
does speech-to-text and forwards the recognized sentence as a raw string
over Bluetooth Serial (SPP).

## Voice command reference

| Say                       | Action                                    |
|----------------------------|----------------------------------------------|
| "light one on / off"       | Toggles Light 1                              |
| "light two on / off"       | Toggles Light 2                              |
| "all lights on / off"      | Toggles both lights together                 |
| "fan speed one/two/three"  | Sets fan to low/medium/high                  |
| "fan off"                    | Turns fan off                              |
| "ac on / off"                | Turns AC on (24°C, auto fan) / off         |
| "auto mode on / off"       | Enables motion + temperature automation      |
| "status"                    | Reports current state of all loads          |

## Running the simulation locally

1. Go to [wokwi.com](https://wokwi.com) → New Project → ESP32
2. Paste `wokwi-simulation/sketch.ino` and `wokwi-simulation/diagram.json`
   into their respective tabs
3. Add the `DHT sensor library for ESPx` library via the Library Manager
4. Hit ▶ Play, open the Serial Monitor, type any command from the table above

## Building the real hardware

1. Install the ESP32 board package in Arduino IDE
2. Install `IRremoteESP8266`, `RBDdimmer`, and `DHT sensor library` (Adafruit)
3. Wire per the pin table in `firmware/home_automation_voice_control.ino`
   header comments
4. Flash `firmware/home_automation_voice_control.ino`
5. Pair phone with Bluetooth device `HomeAutomation_ESP32`, connect your
   voice-control app to it

## Possible extensions

- Swap Bluetooth for WiFi + Google Assistant (via IFTTT/Blynk) for control
  beyond BT range
- Add energy monitoring (ZMPT101B + ACS712) for per-appliance power logging
- On-device wake-word detection (INMP441 mic + Edge Impulse) instead of
  relying on a companion app
