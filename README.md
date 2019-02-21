# esp-mqtt-irblaster
![TravisCI Build Status](https://travis-ci.org/johnboiles/esp-mqtt-irblaster.svg?branch=master)

ESP8266 MQTT IR Blaster. Useful for hooking up IR gear (TVs, ACs, sound bars) to Home Assistant (and probably other things).

## Compiling the code

### Setting some in-code config values

First off you'll need to create a `src/secrets.h`. This file is `.gitignore`'d so you don't put your passwords on Github.

    cp src/secrets.example.h src/secrets.h

Then edit your `src/secrets.h` file to reflect your wifi ssid/password and MQTT server password (if you're using the Home Assistant built-in broker, this is just your API password).

You may also need to modify the values in `src/config.h` (particularly `MQTT_SERVER`) to match your setup.

### Building and uploading

The easiest way to build and upload the code is with the [PlatformIO IDE](http://platformio.org/platformio-ide).

The first time you program your board you'll want to do it over USB/Serial. After that, programming can be done over wifi (via ArduinoOTA). To program over USB/Serial, change the `upload_port` in the `platformio.ini` file to point to the appropriate device for your board. Probably something like the following will work if you're on a Mac.

    upload_port = /dev/tty.cu*

If you're not using an ESP12E board, you'll also want to update the `board` line with your board. See [here](http://docs.platformio.org/en/latest/platforms/espressif8266.html) for other PlatformIO supported ESP8266 board. For example, for the Wemos D1 Mini:

    board = d1_mini

After that, from the PlatformIO VSCode IDE, you should be able to hit the ➡️ button in the bottom toolbar.

## Testing

[Mosquitto](https://mosquitto.org/) can be super useful for testing this code. For example the following commands can be used publish and subscribe to messages to and from the vacuum respectively.

```
export MQTT_SERVER=YOURSERVERHOSTHERE
export MQTT_USER=homeassistant
export MQTT_PASSWORD=PROBABLYYOURHOMEASSISTANTPASSWORD
mosquitto_pub -t 'irblaster/tx' -h $MQTT_SERVER -p 1883 -u $MQTT_USER -P $MQTT_PASSWORD -V mqttv311 -m '{"type":"NEC","code":"FF13EC"}
mosquitto_sub -t 'irblaster/rx' -v -h $MQTT_SERVER -p 1883 -u $MQTT_USER -P $MQTT_PASSWORD -V mqttv311
```

## Debugging

Included in the firmware is a telnet debugging interface. To connect run `telnet irblaster.local`. With that you can log messages from code with the `DLOG` macro and also send commands back that the code can act on (see the `debugCallback` function).

## Home Assistant Automation

I use the following Home Assistant automation to switch the inputs on my Vizio sound bar when my Alexa whole home audio group (named 'Everywhere') starts or stops playing.

```
- alias: Set soundbar input to aux/opt when the Alexa Everywhere group starts/stops
  trigger:
  - platform: state
    entity_id: media_player.everywhere
    to: 'playing'
  - platform: state
    entity_id: media_player.everywhere
    from: 'playing'
  action:
  - service: mqtt.publish
    data_template:
      topic: 'irblaster/tx'
      payload_template: "{\"type\":\"NEC\",\"code\":\"{% if trigger.to_state.state == 'playing' %}FF8D72{% else %}FF13EC{% endif %}\"}"
```