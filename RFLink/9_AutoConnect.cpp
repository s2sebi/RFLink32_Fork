// ************************************* //
// * Arduino Project RFLink-esp        * //
// * https://github.com/couin3/RFLink  * //
// * 2018..2020 Stormteam - Marc RIVES * //
// * More details in RFLink.ino file   * //
// ************************************* //

#include <Arduino.h>
#include "RFLink.h"
#include "9_AutoConnect.h"

#ifdef AUTOCONNECT_ENABLED

#ifdef ESP8266
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h> // Replace with WebServer.h for ESP32
#include <SPI.h>              // To acces Flash Memory
#include <FS.h>               // To save MQTT parameters
#include <AutoConnect.h>
#elif ESP32
#error "AutoConnect for ESP32 not implemented... Yet"
#endif // ESP8266

ESP8266WebServer Server; // Replace with WebServer for ESP32
AutoConnect portal(Server);
AutoConnectConfig config;

String MQTT_SERVER;
String MQTT_PORT;
String MQTT_ID;
String MQTT_USER;
String MQTT_PSWD;
String MQTT_TOPIC_OUT;
String MQTT_TOPIC_IN;
boolean MQTT_RETAINED;
// Adds advanced tab to Autoconnect
String Adv_HostName;
String Adv_Power;

// Prototypes
String loadParams(AutoConnectAux &aux, PageArgument &args);
String saveParams(AutoConnectAux &aux, PageArgument &args);

void rootPage()
{
    char content[] = "RFLink ESP";
    Server.send(200, "text/plain", content);
}

void setup_AutoConnect()
{
    if (portal.load(FPSTR(AUX_settings)))
    { // we load all the settings from "/settings" uri
        AutoConnectAux &aux1 = *portal.aux(AUX_SETTING_URI);
        PageArgument args;

        loadParams(aux1, args);

        if (config.immediateStart)
        {
            // if not defined, default Wifi AP is 12345678, you can change it here
            // config.psk = "RFlink-ESP";
            config.apid = String("RFLink_ESP-") + String(GET_CHIPID(), HEX);
            Serial.print(F("AP name set to "));
            Serial.println(config.apid);
        }
        else if (Adv_HostName.length())
        {
            config.hostName = Adv_HostName;
            Serial.print(F("Hostname set to "));
            Serial.println(config.hostName);
        }
        config.bootUri = AC_ONBOOTURI_HOME;
        config.homeUri = "/";
        config.title = "RFlink ESP";
        config.autoReconnect = true;
        portal.config(config);

        portal.on(AUX_SETTING_URI, loadParams);
        portal.on(AUX_SAVE_URI, saveParams);
    }
    else
        Serial.println(F("load error"));

    //-------------------------------------

    if (portal.begin())
    {
        if (MDNS.begin(config.hostName))
            MDNS.addService("http", "tcp", 80);
        Serial.print(F("connected: "));
        Serial.println(WiFi.SSID());
        Serial.print(F("IP: "));
        Serial.println(WiFi.localIP().toString());
    }
    else
    {
        Serial.print(F("connection failed:"));
        Serial.println(String(WiFi.status()));
        while (1)
        {
            delay(100);
            yield();
        }
    }
    SPIFFS.end();
}

void loop_AutoConnect()
{
    MDNS.update();
    portal.handleClient();
}

void getParams(AutoConnectAux &aux)
{
    //////  MQTT  settings //////
    MQTT_SERVER = aux["MQTT_SERVER"].value;
    MQTT_SERVER.trim();
    MQTT_PORT = aux["MQTT_PORT"].value;
    MQTT_PORT.trim();
    MQTT_ID = aux["MQTT_ID"].value;
    MQTT_ID.trim();
    MQTT_USER = aux["MQTT_USER"].value;
    MQTT_USER.trim();
    MQTT_PSWD = aux["MQTT_PSWD"].value;
    MQTT_PSWD.trim();
    MQTT_TOPIC_OUT = aux["MQTT_TOPIC_OUT"].value;
    MQTT_TOPIC_OUT.trim();
    MQTT_TOPIC_IN = aux["MQTT_TOPIC_IN"].value;
    MQTT_TOPIC_IN.trim();
    MQTT_RETAINED = aux["MQTT_RETAINED"].as<AutoConnectCheckbox>().checked;

    ////// advanced settings //////
    Adv_HostName = aux["Adv_HostName"].value;
    Adv_HostName.trim();
    Adv_Power = aux["Adv_Power"].value;
    Adv_Power.trim();
}

// Load parameters saved with  saveParams from SPIFFS into the
// elements defined in /settings JSON.
String loadParams(AutoConnectAux &aux, PageArgument &args)
{
    (void)(args);
    static boolean initConfig = true;

    SPIFFS.begin();
    File my_file = SPIFFS.open(PARAM_FILE, "r");
    Serial.print(PARAM_FILE);
    if (my_file)
    {
        if (aux.loadElement(my_file))
        {
            getParams(aux);
            Serial.println(F(" loaded"));
            if (initConfig)
            {
                config.immediateStart = false; // Only Home AP
                config.autoRise = false;       // Captive AP disabled
            }
        }
        else
        {
            Serial.println(F(" failed to load"));
            if (initConfig)
            {
                config.immediateStart = true; // Don't even try Home AP
                config.autoRise = true;       // Captive AP enabled
            }
        }
        my_file.close();
    }
    else
    {
        Serial.println(F(" open+r failed"));
        if (initConfig)
        {
            config.immediateStart = true; // Don't even try Home AP
            config.autoRise = true;       // Captive AP enabled
        }
#ifdef ESP32
        Serial.println(F("If you get error as 'SPIFFS: mount failed, -10025', Please modify with 'SPIFFS.begin(true)'."));
#endif // ESP32
    }
    SPIFFS.end();
    initConfig = false;
    return String("");
}

// Save the value of each element entered by '/settings' to the
// parameter file. The saveParams as below is a callback function of
// /settings_save. When invoking this handler, the input value of each
// element is already stored in '/settings'.
// In Sketch, you can output to stream its elements specified by name.
String saveParams(AutoConnectAux &aux, PageArgument &args)
{
    // The 'where()' function returns the AutoConnectAux that caused
    // the transition to this page.
    AutoConnectAux &src_aux = *portal.aux(portal.where());
    getParams(src_aux);
    // AutoConnectInput& mqttserver = my_settings["mqttserver"].as<AutoConnectInput>();  //-> BUG
    // The entered value is owned by AutoConnectAux of /settings.
    // To retrieve the elements of /settings, it is necessary to get
    // the AutoConnectAux object of /settings.
    SPIFFS.begin();
    File my_file = SPIFFS.open(PARAM_FILE, "w");
    Serial.print(PARAM_FILE);
    if (my_file)
    {
        src_aux.saveElement(my_file, {"MQTT_SERVER", "MQTT_PORT",
                                      "MQTT_ID", "MQTT_USER", "MQTT_PSWD",
                                      "MQTT_TOPIC_OUT", "MQTT_TOPIC_IN", "MQTT_RETAINED",
                                      "Adv_HostName", "Adv_Power"});
        Serial.println(F(" saved"));
        my_file.close();
    }
    else
        Serial.print(F(" open+w failed"));
    SPIFFS.end();
    // Echo back saved parameters to AutoConnectAux page.
    AutoConnectText &echo = aux["parameters"].as<AutoConnectText>();
    echo.value = F("<u><b>MQTT settings</b></u>");
    echo.value += F("<br><b>Connexion</b>");
    echo.value += F("<br>Server: ");
    echo.value += MQTT_SERVER;
    echo.value += F("<br>Port: ");
    echo.value += MQTT_PORT;
    echo.value += F("<br>ID: ");
    echo.value += MQTT_ID;
    echo.value += F("<br>Username: ");
    echo.value += MQTT_USER;
    echo.value += F("<br>Password: ");
    echo.value += MQTT_PSWD;
    echo.value += F("<br><br><b>Messages</b>");
    echo.value += F("<br>Out Topic: ");
    echo.value += MQTT_TOPIC_OUT;
    echo.value += F("<br>In Topic: ");
    echo.value += MQTT_TOPIC_IN;
    echo.value += F("<br>Retained: ");
    echo.value += String(MQTT_RETAINED == true ? "true" : "false");
    echo.value += F("<br><u><br><b>Advanced settings</b></u>");
    echo.value += F("<br><b>WiFi</b>");
    echo.value += F("<br>Hostname: ");
    echo.value += Adv_HostName;
    echo.value += F("<br>TX Power: ");
    echo.value += Adv_Power;
    echo.value += F("<br>");
    return String("");
}

#endif // AUTOCONNECT_ENABLE