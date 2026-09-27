/*
  =====================================================================
  IoT-Based Wireless Home Automation System with Voice Control
  =====================================================================
  Board:      ESP32 DevKit v1 (30-pin)
  Comms:      Bluetooth Serial (SPP) — paired with any Android
              "Bluetooth Voice Control" app that converts speech
              to text and sends the string over BT Serial.
  Controls:   2x Light (relay ON/OFF)
              1x Fan speed (AC dimmer, 3 discrete speed levels)
              1x AC unit (IR blaster — ON/OFF + basic temp control)
  Sensors:    DHT11 (temp/humidity, safety cutoff + auto-suggest)
              PIR motion sensor (auto light trigger, can be voice-toggled)
  =====================================================================
  Voice command grammar (case-insensitive, spoken naturally by user,
  app sends raw recognized text — code does substring matching):

    "light one on" / "light one off"
    "light two on" / "light two off"
    "all lights on" / "all lights off"
    "fan speed one" / "fan speed two" / "fan speed three" / "fan off"
    "ac on" / "ac off"
    "auto mode on" / "auto mode off"   -> toggles PIR-based auto lighting
    "status"                            -> ESP32 speaks back via app (text)

  =====================================================================
  Libraries required (Arduino Library Manager):
    - BluetoothSerial          (bundled with ESP32 board package)
    - IRremoteESP8266          (for AC IR control)
    - RBDdimmer                (by RobotDyn, for AC dimmer / fan speed)
    - DHT sensor library       (by Adafruit)
  =====================================================================
*/

#include "BluetoothSerial.h"
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <ir_Coolix.h>          // Generic AC protocol - swap for your AC brand
#include <RBDdimmer.h>
#include <DHT.h>

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Enable it in "Tools -> Partition Scheme".
#endif

// ---------------- Pin Definitions ----------------
#define RELAY_LIGHT1      25
#define RELAY_LIGHT2      26
#define PIR_SENSOR_PIN    27
#define DHT_PIN           4
#define IR_LED_PIN        18
#define DIMMER_ZC_PIN     19   // Zero-cross detect
#define DIMMER_PSM_PIN    21   // TRIAC gate/PWM pin

#define DHT_TYPE          DHT11

// ---------------- Globals ----------------
BluetoothSerial SerialBT;
IRCoolixAC ac(IR_LED_PIN);
dimmerLamp fanDimmer(DIMMER_PSM_PIN, DIMMER_ZC_PIN);
DHT dht(DHT_PIN, DHT_TYPE);

bool light1State = false;
bool light2State = false;
bool acState      = false;
int  fanSpeedPct  = 0;          // 0, 33, 66, 100
bool autoModeOn   = false;

unsigned long lastSensorRead = 0;
const unsigned long SENSOR_INTERVAL = 3000; // ms

// ---------------- Setup ----------------
void setup() {
  Serial.begin(115200);
  SerialBT.begin("HomeAutomation_ESP32");   // Bluetooth device name
  Serial.println("Bluetooth started. Pair with 'HomeAutomation_ESP32'.");

  pinMode(RELAY_LIGHT1, OUTPUT);
  pinMode(RELAY_LIGHT2, OUTPUT);
  pinMode(PIR_SENSOR_PIN, INPUT);

  // Relays are active-low on most 5V modules
  digitalWrite(RELAY_LIGHT1, HIGH);
  digitalWrite(RELAY_LIGHT2, HIGH);

  ac.begin();
  fanDimmer.begin(NORMAL_MODE, ON);
  fanDimmer.setPower(0);

  dht.begin();

  SerialBT.println("System ready. Say a command.");
}

// ---------------- Main Loop ----------------
void loop() {
  if (SerialBT.available()) {
    String cmd = SerialBT.readString();
    cmd.trim();
    cmd.toLowerCase();
    Serial.println("Received: " + cmd);
    handleVoiceCommand(cmd);
  }

  if (autoModeOn && millis() - lastSensorRead > SENSOR_INTERVAL) {
    lastSensorRead = millis();
    runAutoMode();
  }
}

// ---------------- Command Parser ----------------
void handleVoiceCommand(String cmd) {
  if (cmd.indexOf("all lights on") >= 0) {
    setLight(1, true);
    setLight(2, true);
    SerialBT.println("Both lights ON");

  } else if (cmd.indexOf("all lights off") >= 0) {
    setLight(1, false);
    setLight(2, false);
    SerialBT.println("Both lights OFF");

  } else if (cmd.indexOf("light one on") >= 0) {
    setLight(1, true);
    SerialBT.println("Light 1 ON");

  } else if (cmd.indexOf("light one off") >= 0) {
    setLight(1, false);
    SerialBT.println("Light 1 OFF");

  } else if (cmd.indexOf("light two on") >= 0) {
    setLight(2, true);
    SerialBT.println("Light 2 ON");

  } else if (cmd.indexOf("light two off") >= 0) {
    setLight(2, false);
    SerialBT.println("Light 2 OFF");

  } else if (cmd.indexOf("fan speed one") >= 0) {
    setFanSpeed(33);
    SerialBT.println("Fan speed 1 (low)");

  } else if (cmd.indexOf("fan speed two") >= 0) {
    setFanSpeed(66);
    SerialBT.println("Fan speed 2 (medium)");

  } else if (cmd.indexOf("fan speed three") >= 0) {
    setFanSpeed(100);
    SerialBT.println("Fan speed 3 (high)");

  } else if (cmd.indexOf("fan off") >= 0) {
    setFanSpeed(0);
    SerialBT.println("Fan OFF");

  } else if (cmd.indexOf("ac on") >= 0) {
    setAC(true);
    SerialBT.println("AC ON (24C, auto fan)");

  } else if (cmd.indexOf("ac off") >= 0) {
    setAC(false);
    SerialBT.println("AC OFF");

  } else if (cmd.indexOf("auto mode on") >= 0) {
    autoModeOn = true;
    SerialBT.println("Auto mode ON (motion-based lighting + temp safety)");

  } else if (cmd.indexOf("auto mode off") >= 0) {
    autoModeOn = false;
    SerialBT.println("Auto mode OFF");

  } else if (cmd.indexOf("status") >= 0) {
    reportStatus();

  } else {
    SerialBT.println("Command not recognized: " + cmd);
  }
}

// ---------------- Actuator Helpers ----------------
void setLight(int which, bool on) {
  int pin = (which == 1) ? RELAY_LIGHT1 : RELAY_LIGHT2;
  digitalWrite(pin, on ? LOW : HIGH);  // active-low relay
  if (which == 1) light1State = on; else light2State = on;
}

void setFanSpeed(int pct) {
  fanSpeedPct = pct;
  fanDimmer.setPower(pct);
}

void setAC(bool on) {
  acState = on;
  if (on) {
    ac.setMode(kCoolixCool);
    ac.setTemp(24);
    ac.setFan(kCoolixFanAuto);
    ac.on();
  } else {
    ac.off();
  }
  ac.send();
}

// ---------------- Auto Mode ----------------
void runAutoMode() {
  // Motion-triggered lighting
  int motion = digitalRead(PIR_SENSOR_PIN);
  if (motion == HIGH && !light1State) {
    setLight(1, true);
    Serial.println("[AUTO] Motion detected -> Light 1 ON");
  } else if (motion == LOW && light1State) {
    setLight(1, false);
    Serial.println("[AUTO] No motion -> Light 1 OFF");
  }

  // Temperature safety cutoff for AC (avoid running if sensor faulty/too cold)
  float temp = dht.readTemperature();
  if (!isnan(temp)) {
    if (temp < 18.0 && acState) {
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
  SerialBT.println(s);
  Serial.println(s);
}
