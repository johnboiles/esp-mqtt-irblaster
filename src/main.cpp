#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <IRrecv.h>
#include <IRutils.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "config.h"
extern "C" {
#include "user_interface.h"
}

// Remote debugging over telnet. Just run:
// `telnet roomba.local` OR `nc roomba.local 23`
#if LOGGING
#include <RemoteDebug.h>
#define DLOG(msg, ...) if(Debug.isActive(Debug.DEBUG)){Debug.printf(msg, ##__VA_ARGS__);}
#define VLOG(msg, ...) if(Debug.isActive(Debug.VERBOSE)){Debug.printf(msg, ##__VA_ARGS__);}
RemoteDebug Debug;
#else
#define DLOG(msg, ...)
#endif

// Network setup
WiFiClient wifiClient;
bool OTAStarted;

// IR setup
IRrecv irrecv(IR_RX_PIN, 300);
IRsend irsend(IR_TX_PIN);
decode_results results;

// MQTT setup
PubSubClient mqttClient(wifiClient);
const PROGMEM char *kTxTopic = MQTT_TX_TOPIC;
const PROGMEM char *kRxTopic = MQTT_RX_TOPIC;

void onOTAStart() {
  DLOG("Starting OTA session\n");
  OTAStarted = true;
}

void mqttCallback(char *topic, byte *payload, unsigned int length) {
  DLOG("Received mqtt callback for topic %s\n", topic);
  if (strcmp(kTxTopic, topic) == 0) {
    // turn payload into a null terminated string for easy comparison
    char *cmd = (char *)malloc(length + 1);
    memcpy(cmd, payload, length);
    cmd[length] = 0;
    DLOG("Handling %s %s\n", topic, cmd);

    StaticJsonBuffer<200> jsonBuffer;
    JsonObject& root = jsonBuffer.parseObject(cmd);

    // Test if parsing succeeds.
    if (!root.success()) {
      DLOG("JSON parseObject() failed\n");
      return;
    }

    const char* type = root["type"];
    const char* code = root["code"];
    if (strcmp("NEC", type) == 0) {
      int number = (int) strtol(code, NULL, 16);
      DLOG("Sending NEC: 0x%X\n", number);
      irsend.sendNEC(number);
    }

    free(cmd);
  }
}

void mqttReconnect() {
  DLOG("Attempting MQTT connection...\n");
  // Attempt to connect
  if (mqttClient.connect(HOSTNAME, MQTT_USER, MQTT_PASSWORD)) {
    DLOG("MQTT connected\n");
    mqttClient.subscribe(kTxTopic);
  } else {
    DLOG("MQTT failed rc=%d try again in 5 seconds\n", mqttClient.state());
  }
}

void mqttReportCode(decode_results *results) {
  if (!mqttClient.connected()) {
    DLOG("MQTT Disconnected, not sending IR code\n");
    return;
  }
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root["type"] = "Encoding  : " + typeToString(results->decode_type, results->repeat);
  root["code"] = resultToHexidecimal(results);
  String jsonStr;
  root.printTo(jsonStr);
  mqttClient.publish(kRxTopic, jsonStr.c_str());
}

uint16_t input[67] = {9064, 4480,  656, 474,  648, 484,  650, 1612,  654, 478,  654, 476,  656, 476,  656, 476,  648, 484,  648, 1614,  654, 1610,  656, 476,  658, 1606,  650, 1612,  654, 1610,  656, 1606,  650, 1614,  652, 1610,  656, 1606,  650, 1614,  652, 1612,  654, 476,  656, 1606,  650, 482,  652, 480,  602, 530,  604, 528,  604, 528,  604, 526,  598, 1664,  652, 480,  654, 1608,  658, 1602,  654};

uint16_t hdmi1[] = {0x0000, 0x006D, 0x0022, 0x0002, 0x0157, 0x00AC, 0x0015, 0x0016, 0x0015, 0x0016, 0x0015, 0x0041, 0x0015, 0x0016, 0x0015, 0x0016, 0x0015, 0x0016, 0x0015, 0x0016, 0x0015, 0x0016, 0x0015, 0x0041, 0x0015, 0x0041, 0x0015, 0x0016, 0x0015, 0x0041, 0x0015, 0x0041, 0x0015, 0x0041, 0x0015, 0x0041, 0x0015, 0x0041, 0x0015, 0x0041, 0x0015, 0x0016, 0x0015, 0x0016, 0x0015, 0x0016, 0x0015, 0x0016, 0x0015, 0x0016, 0x0015, 0x0016, 0x0015, 0x0041, 0x0015, 0x0016, 0x0015, 0x0041, 0x0015, 0x0041, 0x0015, 0x0041, 0x0015, 0x0041, 0x0015, 0x0041, 0x0015, 0x0041, 0x0015, 0x0016, 0x0015, 0x0689, 0x0157, 0x0056, 0x0015, 0x0E94};
uint16_t hdmi2[] = {0x0000, 0x006D, 0x0022, 0x0002, 0x0157, 0x00AC, 0x0015, 0x0016, 0x0015, 0x0016, 0x0015, 0x0041, 0x0015, 0x0016, 0x0015, 0x0016, 0x0015, 0x0016, 0x0015, 0x0016, 0x0015, 0x0016, 0x0015, 0x0041, 0x0015, 0x0041, 0x0015, 0x0016, 0x0015, 0x0041, 0x0015, 0x0041, 0x0015, 0x0041, 0x0015, 0x0041, 0x0015, 0x0041, 0x0015, 0x0016, 0x0015, 0x0041, 0x0015, 0x0016, 0x0015, 0x0016, 0x0015, 0x0016, 0x0015, 0x0016, 0x0015, 0x0016, 0x0015, 0x0041, 0x0015, 0x0041, 0x0015, 0x0016, 0x0015, 0x0041, 0x0015, 0x0041, 0x0015, 0x0041, 0x0015, 0x0041, 0x0015, 0x0041, 0x0015, 0x0016, 0x0015, 0x0689, 0x0157, 0x0056, 0x0015, 0x0E94};
uint16_t hdmi5[] = {0x0000, 0x006D, 0x0022, 0x0002, 0x0157, 0x00AC, 0x0015, 0x0016, 0x0015, 0x0016, 0x0015, 0x0041, 0x0015, 0x0016, 0x0015, 0x0016, 0x0015, 0x0016, 0x0015, 0x0016, 0x0015, 0x0016, 0x0015, 0x0041, 0x0015, 0x0041, 0x0015, 0x0016, 0x0015, 0x0041, 0x0015, 0x0041, 0x0015, 0x0041, 0x0015, 0x0041, 0x0015, 0x0041, 0x0015, 0x0041, 0x0015, 0x0016, 0x0015, 0x0041, 0x0015, 0x0016, 0x0015, 0x0016, 0x0015, 0x0016, 0x0015, 0x0016, 0x0015, 0x0041, 0x0015, 0x0016, 0x0015, 0x0041, 0x0015, 0x0016, 0x0015, 0x0041, 0x0015, 0x0041, 0x0015, 0x0041, 0x0015, 0x0041, 0x0015, 0x0016, 0x0015, 0x0689, 0x0157, 0x0056, 0x0015, 0x0E94};

void debugCallback() {
  String cmd = Debug.getLastCommand();

  // Debugging commands via telnet
  if (cmd == "ping") {
    DLOG("pong\n");
  } else if (cmd == "volup") {
    irsend.sendNEC(0x20DF40BF);
  } else if (cmd == "voldown") {
    irsend.sendNEC(0x20DFC03F);
  } else if (cmd == "input") {
    irsend.sendNEC(0x20DFF40B);
  } else if (cmd == "aux") {
    irsend.sendNEC(0xFF8D72);
  } else if (cmd == "opt") {
    irsend.sendNEC(0xFF13EC);
  } else if (cmd == "tvpwr") {
    irsend.sendNEC(0x20DF10EF);
  } else if (cmd == "hdmi1") {
    irsend.sendPronto(hdmi1, sizeof(hdmi1) / sizeof(hdmi1[0]));
  } else if (cmd == "hdmi2") {
    irsend.sendPronto(hdmi2, sizeof(hdmi2) / sizeof(hdmi2[0]));
  } else if (cmd == "hdmi5") {
    irsend.sendPronto(hdmi5, sizeof(hdmi5) / sizeof(hdmi5[0]));
  } else {
    DLOG("Unknown command %s\n", cmd.c_str());
  }
}

void setup() {
  // Set Hostname.
  String hostname(HOSTNAME);
  WiFi.hostname(hostname);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  ArduinoOTA.setHostname((const char *)hostname.c_str());
  ArduinoOTA.begin();
  ArduinoOTA.onStart(onOTAStart);

  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
  mqttClient.setCallback(mqttCallback);

  #if LOGGING
  Debug.begin((const char *)hostname.c_str());
  Debug.setResetCmdEnabled(true);
  Debug.setCallBackProjectCmds(debugCallback);
  Debug.setSerialEnabled(false);
  #endif

  irrecv.enableIRIn();
  irsend.begin();
}

int gLastConnectTime = 0;

void loop() {
  // Important callbacks that _must_ happen every cycle
  ArduinoOTA.handle();
  yield();
  Debug.handle();

  // Skip all other logic if we're running an OTA update
  if (OTAStarted) {
    return;
  }  

  // If MQTT client can't connect to broker, then reconnect
  long now = millis();
  if (!mqttClient.connected() && (now - gLastConnectTime) > 5000) {
    DLOG("Reconnecting MQTT\n");
    gLastConnectTime = now;
    mqttReconnect();
    if (!mqttClient.connected()) {
      return;
    }
  }
  mqttClient.loop();

  // Try to decode IR packets
  if (irrecv.decode(&results)) {
    DLOG("Received Code\n");
    if (results.overflow) {
      DLOG("Overflow\n");
    }
    DLOG("%s\n", resultToHumanReadableBasic(&results).c_str());
    DLOG("%s\n", resultToSourceCode(&results).c_str());
    // Receive the next value
    irrecv.resume();
  }
}
