#include "secrets.h"

#define HOSTNAME "irblaster" // e.g. irblaster.local
#define IR_RX_PIN 0 // D3 on Wemos D1 mini
#define IR_TX_PIN 5 // D1 on Wemos D1 mini

#define MQTT_SERVER "10.0.0.2"
#define MQTT_PORT 1883
#define MQTT_USER "homeassistant"
#define MQTT_RX_TOPIC "irblaster/rx"
#define MQTT_TX_TOPIC "irblaster/tx"
