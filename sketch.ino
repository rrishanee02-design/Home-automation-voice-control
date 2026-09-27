/*
  ============================================================
  Home Automation with Voice Control — WOKWI SIMULATION BUILD
  ============================================================
  This is a demo-ready version of the real project for running
  in https://wokwi.com with ZERO physical hardware.

  Differences from the real hardware version (say this out loud
  if anyone asks — it's a legitimate simulation methodology):
    - BluetoothSerial -> Serial. In real hardware, a phone app
      does speech-to-text and sends the string over Bluetooth SPP.
      Here, you type that same string into the Serial Monitor to
      stand in for the recognized voice command.
    - Relays -> LEDs. LED ON = relay energized = load powered.
    - Dimmer module -> PWM brightness on the fan LED (0/33/66/100%).
    - IR AC control -> a 4th LED representing the AC unit state.

  Paste this as sketch.ino in a new Wokwi ESP32 project, paste
  diagram.json (given separately) as the diagram, hit the green
  Play button, click the Serial Monitor, and type commands.
  ============================================================
*/

#include "DHTesp.h"

// ---------------- Pins ----------------
#define LIGHT1_PIN   25
#define LIGHT2_PIN   26
#define FAN_PIN      14   // PWM output -> brightness = fan speed
#define AC_PIN       33
#define PIR_PIN      27
#define DHT_PIN      4

DHTesp dhtSensor;

bool light1State = false;
bool light2State = false;
bool acState      = false;
int  fanSpeedPct  = 0;
bool autoModeOn   = false;

unsigned long lastSensorRead = 0;
const unsigned long SENSOR_INTERVAL = 3000;

void setup() {
  Serial.begin(115200);

  pinMode(LIGHT1_PIN, OUTPUT);
  pinMode(LIGHT2_PIN, OUTPUT);
  pinMode(AC_PIN, OUTPUT);
  pinMode(PIR_PIN, INPUT);

  digitalWrite(LIGHT1_PIN, LOW);
  digitalWrite(LIGHT2_PIN, LOW);
  digitalWrite(AC_PIN, LOW);

  dhtSensor.setup(DHT_PIN, DHTesp::DHT22);

  Serial.println("=================================================");
  Serial.println(" Home Automation READY. Type a command + Enter:");
  Serial.println(" light one on/off | light two on/off");
  Serial.println(" all lights on/off");
  Serial.println(" fan speed one/two/three | fan off");
  Serial.println(" ac on/off | auto mode on/off | status");
  Serial.println("=================================================");
}

void loop() {
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    cmd.toLowerCase();
    if (cmd.length() > 0) {
      Serial.println("> " + cmd);
      handleVoiceCommand(cmd);
    }
  }

  if (autoModeOn && millis() - lastSensorRead > SENSOR_INTERVAL) {
    lastSensorRead = millis();
    runAutoMode();
  }
}

void handleVoiceCommand(String cmd) {
  if (cmd.indexOf("all lights on") >= 0) {
    setLight(1, true); setLight(2, true);
    Serial.println("Both lights ON");
  } else if (cmd.indexOf("all lights off") >= 0) {
    setLight(1, false); setLight(2, false);
    Serial.println("Both lights OFF");
  } else if (cmd.indexOf("light one on") >= 0) {
    setLight(1, true); Serial.println("Light 1 ON");
  } else if (cmd.indexOf("light one off") >= 0) {
    setLight(1, false); Serial.println("Light 1 OFF");
  } else if (cmd.indexOf("light two on") >= 0) {
    setLight(2, true); Serial.println("Light 2 ON");
  } else if (cmd.indexOf("light two off") >= 0) {
    setLight(2, false); Serial.println("Light 2 OFF");
  } else if (cmd.indexOf("fan speed one") >= 0) {
    setFanSpeed(33); Serial.println("Fan speed 1 (low)");
  } else if (cmd.indexOf("fan speed two") >= 0) {
    setFanSpeed(66); Serial.println("Fan speed 2 (medium)");
  } else if (cmd.indexOf("fan speed three") >= 0) {
    setFanSpeed(100); Serial.println("Fan speed 3 (high)");
  } else if (cmd.indexOf("fan off") >= 0) {
    setFanSpeed(0); Serial.println("Fan OFF");
  } else if (cmd.indexOf("ac on") >= 0) {
    setAC(true); Serial.println("AC ON");
  } else if (cmd.indexOf("ac off") >= 0) {
    setAC(false); Serial.println("AC OFF");
  } else if (cmd.indexOf("auto mode on") >= 0) {
    autoModeOn = true; Serial.println("Auto mode ON");
  } else if (cmd.indexOf("auto mode off") >= 0) {
    autoModeOn = false; Serial.println("Auto mode OFF");
  } else if (cmd.indexOf("status") >= 0) {
    reportStatus();
  } else {
    Serial.println("Command not recognized: " + cmd);
  }
}

void setLight(int which, bool on) {
  int pin = (which == 1) ? LIGHT1_PIN : LIGHT2_PIN;
  digitalWrite(pin, on ? HIGH : LOW);
  if (which == 1) light1State = on; else light2State = on;
}

void setFanSpeed(int pct) {
  fanSpeedPct = pct;
  int duty = map(pct, 0, 100, 0, 255);
  analogWrite(FAN_PIN, duty);   // brightness = speed
}

void setAC(bool on) {
  acState = on;
  digitalWrite(AC_PIN, on ? HIGH : LOW);
}

void runAutoMode() {
  int motion = digitalRead(PIR_PIN);
  if (motion == HIGH && !light1State) {
    setLight(1, true);
    Serial.println("[AUTO] Motion detected -> Light 1 ON");
  } else if (motion == LOW && light1State) {
    setLight(1, false);
    Serial.println("[AUTO] No motion -> Light 1 OFF");
  }

  TempAndHumidity data = dhtSensor.getTempAndHumidity();
  if (!isnan(data.temperature)) {
    if (data.temperature < 18.0 && acState) {
      setAC(false);
      Serial.println("[AUTO] Room already cool -> AC OFF");
    }
  }
}

void reportStatus() {
  String s = "STATUS | L1:" + String(light1State ? "ON" : "OFF") +
             " L2:" + String(light2State ? "ON" : "OFF") +
             " Fan:" + String(fanSpeedPct) + "%" +
             " AC:" + String(acState ? "ON" : "OFF") +
             " Auto:" + String(autoModeOn ? "ON" : "OFF");
  Serial.println(s);
}
