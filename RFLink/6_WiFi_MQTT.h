// ************************************* //
// * Arduino Project RFLink-esp        * //
// * https://github.com/couin3/RFLink  * //
// * 2018..2020 Stormteam - Marc RIVES * //
// * More details in RFLink.ino file   * //
// ************************************* //

#ifndef WiFi_MQTT_h
#define WiFi_MQTT_h

#include <Arduino.h>
#include "RFLink.h"

#ifndef AUTOCONNECT_ENABLED
#ifdef ESP32
#include <WiFi.h>
#elif ESP8266
#include <ESP8266WiFi.h>
#endif
#endif // !AUTOCONNECT_ENABLED

#ifdef MQTT_ENABLED
extern char MQTTbuffer[PRINT_BUFFER_SIZE]; // Buffer for MQTT message

#ifndef AUTOCONNECT_ENABLED
void setup_WIFI();
#endif // !AUTOCONNECT_ENABLED

void setup_MQTT();
void reconnect();
void publishMsg();
void checkMQTTloop();
#endif // MQTT_ENABLED

#if (!defined(AUTOCONNECT_ENABLED) && !defined(MQTT_ENABLED))
#if (defined(ESP32) || defined(ESP8266))
void setup_WIFI_OFF();
#endif
#endif

#endif // WiFi_MQTT_h