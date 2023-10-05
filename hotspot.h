
/*****************************************************************
 *
 * Some code and ideas from:
 * https://randomnerdtutorials.com/esp32-access-point-ap-web-server/
 * https://circuits4you.com/2018/11/21/esp32-access-point-station-ap-html-web-server-arduino/
 * https://github.com/nkolban/esp32-snippets/tree/master/networking/bootwifi
 * https://github.com/acrobotic/Ai_Tips_ESP8266/tree/master/wifi_modes_switch
 * https://arduinojson.org/v6/doc/

******************************************************************/
#ifndef HOTSPOT_H
#define HOTSPOT_H

#include <WiFi.h>
#include <ArduinoJson.h>
#include <SPIFFS.h>

// WiFi credentials
extern String wifi_SSID;
extern String wifi_Password;

#define CREDENTIALS_FILE "/config.json"
#define KEY_FILE         "/keys.json"
#define KEY_FILE_JSON_SIZE 15

//====== BEGIN Keys to save in keys.json =====
// Variables to store the keys:
extern unsigned long myRelayA;
extern unsigned long myRelayB;
extern unsigned long myRelayC;
extern unsigned long myRelayD;

// Variables to store keys activated by Alexa
extern unsigned long myDeviceE;
extern unsigned long myDeviceF_On;
extern unsigned long myDeviceF_Off;

// Reset variable
extern unsigned long myReset;
//====== END Keys to save in keys.json =====

// Set web  erver port number to 80
extern WiFiServer hotspotServer;
extern WiFiClient hotspotClient;

bool startFileSystem();
bool checkSavedCredentials();
bool startHotspot();
void processHotspotRequest();
void removeCredentials();

void saveKeys();
void restoreKeys();
void removeKeys();

extern bool restartESP;
#endif
