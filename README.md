# ITEAD-S20-CustomFW

An Arduino sketch used to control a relay output, communicating both ways via MQTT.

## What is it

A sketch written for ITEAD S20 Smart Socket, built upon ESP8266, and allows the user to control the device via the local network independent from the manufacturers servers.

It can be easily ported to other platforms/devices by altering the includes, pin-assignment and the chip id determination.

## Features

- Turn on or off the relay output by sending a MQTT-message via local MQTT-broker.
- Toggle the relay using the local button.

## How it works

The device subscribes to the command MQTT-topic of the following syntax; `chipID/cmd`. When a command is received, the device switches the relay on or off depending on the message and confirms it's new state to the status topic of the following syntax; `chipID/opsta`.

`chipID` is read from the ESP8266, this ensures it will be unique in your network.

Switch on by posting the message `ON` to the command-topic, switch off by posting the message `OFF`. Observe that this message is case-sensitive.

When reporting to the status-topic, the device will send `ON` if the relay output is switched on or `OFF` if it is switched off.

The relay can also be toggled via the local button. When the relay is toggled it will report it's new state to the status topic immediately.

The device continously saves the current relay state to the EEPROM and automatically restore it's state and reports it to the broker after power loss.

At boot the device will send it's firmware version (`FW_VERSION`, below) to the topic named as it's MQTT Client ID, below. This functionality is provided to assist users in determining the currently installed firmware and in the future, if upgrade is needed.

## Configuration

### Defines

| Parameter | Description | Default value |
| --- | --- | --- |
| FW_VERSION | Version reported by the device when successfully connected to the MQTT-broker | Release dependent |
| PIN_RELAY | Specifies the GPIO-pin used to control the relay output. | 12 (GPIO12) |
| PIN_STATUS | Specifies the GPIO-pin used to control the status led. | 13 (GPIO13) |
| PIN_BUTTON | Specifies the GPIO-pin where the local button is connected. | 0 (GPIO0) |
| MQTT_MAX_ATTEMPT | Specifies how many times the device will try to connect to the MQTT-broker before waiting for `MQTT_CONN_TIMEOUT` time to pass before retrying | 3 |
| MQTT_CONN_TIMEOUT | Specifies the timeout after `MQTT_MAX_ATTEMPT` failed connection attempts. | 60000 (60s) |
| MQTT_CONN_REATT | Specifies the timeout between two connection attempts. | 10000 (10s) |

### Variables

| Parameter | Description | Default value |
| --- | --- | --- |
| wifiSsid | The name of the network. (SSID). | N/A |
| wifiPass | The password to use when connection to the network `wifiSsid`. | N/A |
| mqttServer | The IP-address of the MQTT-broker. | N/A |
| mqttServerPort | The port where the MQTT-broker is listening. | 1883 |
| mqttUsername | Username used to authenticate to the MQTT-broker. | N/A |
| mqttPassword | Password used to authenticate to the MQTT-broker. | N/A |

## Examples

Example of generated topics and id's if the device has id: 1327465.

| Parameter | Resulting value | Dedeviceion |
| --- | --- | --- |
| MQTT Client ID | S20ESP1327465 | ID used in communication with MQTT-broker |
| Command topic | 1327465/cmd | Topic where the device will listen for commands `ON` or `OFF` to set the relay output. |
| Status topic | 1327465/opsta | Topic where the device will report back it's current state. |

### OpenHAB

Example of item specification in OpenHAB.

```
Switch lmp "Window" <light> (lights) {mqtt=">[MQTTBR01:1327465/cmd:command:ON:default], >[MQTTBR01:1327465/cmd:command:OFF:default], <[MQTTBR01:1327465/opsta:state:default]"}
```

## Versioning

This project follows [Semantic Versioning 2.0](https://semver.org/) and uses version numbers like `MAJOR`.`MINOR`.`PATCH`.