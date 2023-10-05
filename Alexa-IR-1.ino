/************************************************************************
 * 
 * This version merges the Alexa 1 and the IR-receive code.
 * It is planned to control four relays with either Alexa or an IR remote
 * control.
 * For IR it uses five keys. Key 5 is used to reset the values.
 * just presss key 5 four four times and it will reset the first three keys
 *
 ************************************************************************/
#include <Arduino.h>
#include <SPI.h>
#ifdef ESP32
  #include "WiFi.h"
#else // ESP8266
  #include "ESP8266WiFi.h"
#endif
#include "fauxmoESP.h"
#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRutils.h>
#include "hotspot.h"

/**********************************************************************
 *
 * Begin of settings
 *
 **********************************************************************/

// Pin assignments
// Inputs
// Pin to receive IR signal
#define RECV_PIN 11

// Outputs
// Pins to activate relays
#define RELAY_A 12
#define RELAY_B 14
#define RELAY_C 15
#define RELAY_D 16

// Max counts for reset
#define MAX_RESET_COUNT    3
#define FORCE_RESET_COUNT 10

// Others
#define SERIAL_BAUDRATE 115200

/**********************************************************************
 *
 * End of settings
 *
 **********************************************************************/
String wifi_SSID;
String wifi_Password;

#define ID_A "Relay A"
#define ID_B "Relay B"
#define ID_C "Relay C"
#define ID_D "Relay D"

IRrecv irrecv(RECV_PIN); 
decode_results results;  // used to store the key pressed

// Variables to store the keys:
unsigned long myRelayA = 0;
unsigned long myRelayB = 0;
unsigned long myRelayC = 0;
unsigned long myRelayD = 0;

// Variables to store on/off state
boolean onRelayA = false;
boolean onRelayB = false;
boolean onRelayC = false;
boolean onRelayD = false;

// Reset variables
unsigned long myReset = 0;
int        countReset = 0;
unsigned long lastKey = 0;
int        countLastKey = 0;

bool isHotspot(false);

fauxmoESP fauxmo;

void wifiSetup() 
{

    // Set WIFI module to STA mode
    WiFi.mode(WIFI_STA);

    // Connect
    Serial.printf("[WIFI] Connecting to %s ", wifi_SSID.c_str());
    WiFi.begin(wifi_SSID.c_str(), wifi_Password.c_str());

    // Wait
    while (WiFi.status() != WL_CONNECTED) 
    {
        Serial.print(".");
        delay(100);
    }
    Serial.println();

    // Connected!
    Serial.printf("[WIFI] STATION Mode, SSID: %s, IP address: %s\n", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
}

void MyOnSetState(unsigned char device_id, const char * device_name, bool state, unsigned char value) 
{
        Serial.printf("[MAIN] Device #%d (%s) state: %s value: %d\n", device_id, device_name, state ? "ON" : "OFF", value);

        lastKey=0;

        if (strcmp(device_name, ID_A)==0) 
        {
            digitalWrite(RELAY_A, state ? HIGH : LOW);
            onRelayA = state;
            countReset=0; 
            return;
        }
        if (strcmp(device_name, ID_B)==0) 
        {
            digitalWrite(RELAY_B, state ? HIGH : LOW);
            onRelayB = state;
            countReset=0; 
            return;
        }
        if (strcmp(device_name, ID_C)==0) 
        {
            digitalWrite(RELAY_C, state ? HIGH : LOW);
            onRelayC = state;
            countReset=0; 
            return;
        }
        if (strcmp(device_name, ID_D)==0) 
        {
            digitalWrite(RELAY_D, state ? HIGH : LOW);
            onRelayD = state;
            countReset=0; 
            return;
        }
}

void setup() // Initialize 
{ 
    Serial.begin(SERIAL_BAUDRATE);
    Serial.print("--------------------\n\nSTART ");
    irrecv.enableIRIn(); // Comienza a recibir los datos 
    
    pinMode(RELAY_A, OUTPUT);
    digitalWrite(RELAY_A, LOW);
    pinMode(RELAY_B, OUTPUT);
    digitalWrite(RELAY_B, LOW);
    pinMode(RELAY_C, OUTPUT);
    digitalWrite(RELAY_C, LOW);
    pinMode(RELAY_D, OUTPUT);
    digitalWrite(RELAY_D, LOW);

    if(startFileSystem() && checkSavedCredentials())
    {
        restoreKeys();
        Serial.print("\nNORMAL MODE\n");
        wifiSetup(); // initialize the WiFi
    }
    else
    {
        restoreKeys();
        Serial.print("\nHOTSPOT MODE\n");
        isHotspot = startHotspot();
    }

    fauxmo.createServer(true); // not needed, this is the default value
    fauxmo.setPort(80); // This is required for gen3 devices
    fauxmo.enable(true);
    // Add virtual devices
    fauxmo.addDevice(ID_A);
    fauxmo.addDevice(ID_B);
    fauxmo.addDevice(ID_C);
    fauxmo.addDevice(ID_D);
    
    fauxmo.onSetState(&MyOnSetState);
} 

void doReset()
{
    Serial.println("Reset");Serial.println(countReset,DEC);
    myRelayA = myRelayB = myRelayC = myRelayD = 0; 
    onRelayA = onRelayB = onRelayC = onRelayD = false;
    digitalWrite(RELAY_A, LOW);
    digitalWrite(RELAY_B, LOW);
    digitalWrite(RELAY_C, LOW);
    digitalWrite(RELAY_D, LOW);
    countReset = 0;
}

void setMyValue() // Function to set the keys
{
    // This will set up to three keys to the three values
    // The first time a key is pressed it will set a value, and from there on that key will correspond to that value
    if(myRelayA == results.value) return;
    if(myRelayB == results.value) return;
    if(myRelayC == results.value) return;
    if(myRelayD == results.value) return;
    if(myReset  == results.value) return;
    if(0xFFFFFFFF == results.value || 0xFF == results.value) {Serial.println("Ignoring"); return;}
    if(myRelayA == 0) {myRelayA = results.value; Serial.print("set RealyA:"); Serial.println(myRelayA,HEX); return;}
    if(myRelayB == 0) {myRelayB = results.value; Serial.print("set RealyB:"); Serial.println(myRelayB,HEX); return;}
    if(myRelayC == 0) {myRelayC = results.value; Serial.print("set RealyC:"); Serial.println(myRelayC,HEX); return;}
    if(myRelayD == 0) {myRelayD = results.value; Serial.print("set RealyD:"); Serial.println(myRelayD,HEX); return;}
    if(myReset  == 0) {myReset  = results.value; Serial.print("set reset:");  Serial.println(myReset,HEX);  return;}
}

void useValue() // Function to turn on/off based on the keys
{
    if(myRelayA == results.value) {digitalWrite(RELAY_A, onRelayA ? LOW : HIGH); onRelayA = !onRelayA; countReset=0; return;}
    if(myRelayB == results.value) {digitalWrite(RELAY_B, onRelayB ? LOW : HIGH); onRelayB = !onRelayB; countReset=0; return;}
    if(myRelayC == results.value) {digitalWrite(RELAY_C, onRelayC ? LOW : HIGH); onRelayC = !onRelayC; countReset=0; return;}
    if(myRelayD == results.value) {digitalWrite(RELAY_D, onRelayD ? LOW : HIGH); onRelayD = !onRelayD; countReset=0; return;}
    if(myReset == results.value)
    { 
        if(++countReset > MAX_RESET_COUNT) 
            doReset();
    }
    else
        countReset = 0;
}

void loop() // Simplified main loop
{ 
    if(restartESP)
    {
        Serial.print("Restarting ESP\n");
        ESP.restart();
    }

    if(isHotspot)
    {
        hotspotClient = hotspotServer.available();   // Listen for incoming hotspot clients
        if (hotspotClient) 
            processHotspotRequest();

        if(restartWiFi)
        {
            isHotspot = restartESP = false;
            stopHotspot();
            wifiSetup(); // initialize the WiFi
        }

        return;
    }

    fauxmo.handle(); // First handle Alexa

    if (irrecv.decode(&results)) // Second, deal with the IR remote
    {
        Serial.print("Received:");
        serialPrintUint64(results.value, HEX); 
        Serial.println("");

        // Process lastKey (forced reset)
        if(0xFFFFFFFF != results.value && 0xFF != results.value) // ignore repeat
        {
            if(results.value == lastKey)
            {
                if(++countLastKey > FORCE_RESET_COUNT) doReset();
            }
            else
            {
                countLastKey = 0;
                lastKey = results.value;
            }
        }

        setMyValue();
        useValue();
        delay(10);
        irrecv.resume();
    }
}
