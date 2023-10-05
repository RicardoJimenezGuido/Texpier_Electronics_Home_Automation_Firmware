
/*****************************************************************
 *
 * Some code and ideas from:
 * https://randomnerdtutorials.com/esp32-access-point-ap-web-server/
 * https://circuits4you.com/2018/11/21/esp32-access-point-station-ap-html-web-server-arduino/
 * https://github.com/nkolban/esp32-snippets/tree/master/networking/bootwifi
 * https://github.com/acrobotic/Ai_Tips_ESP8266/tree/master/wifi_modes_switch
 * https://arduinojson.org/v6/doc/

******************************************************************/

#include "hotspot.h"
#include "ir.h"
#include "dbg.h"

// Hotspot credentials
const char* hotspot_SSID     = "ESP32-hotspot";
const char* hotspot_Password = "123456789";


const char config_html[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
<head></head>
<body>
    <h1>ESP32 Config Page</h1>
    <p>enter configuration</p>
  <form>
    <fieldset>
      <div>
        <label for="ssid">SSID</label>      
        <input value="" id="ssid" placeholder="SSID">
      </div>
      <div>
        <label for="password">PASSWORD</label>
        <input type="password" value="" id="password" placeholder="PASSWORD">
      </div>
      <div>
      </div>
        <input type="checkbox" id="keep" value="">
        <label for="keep">Keep the programmed IR keys</label><br>
        <input type="checkbox" id="restart" value="">
        <label for="restart">Restart the ESP32</label><br>
      <div>
        <button class="primary" id="savebtn" type="button" onclick="sendJSON()">SAVE</button>
      </div>
    </fieldset>
  </form>
</body>
<script>
function sendJSON()
{
  var ssid = document.getElementById("ssid").value;
  var password = document.getElementById("password").value;
  var keep = document.getElementById("keep").checked;
  var restart = document.getElementById("restart").checked;
  
  var data = {ssid:ssid, password:password, keep:keep, restart:restart};

  var xhr = new XMLHttpRequest();
  var url = "/settings";
  xhr.open("POST", url, true);
  xhr.send(JSON.stringify(data));
};
</script>
</html>
)=====";

const char done_html[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
<head></head>
<body>
    <h1>ESP32 Config Page</h1>
    <p>done</p>
</body>
</html>
)=====";

WiFiServer hotspotServer(80);
WiFiClient hotspotClient;
bool restartESP(false);

bool checkSavedCredentials()
{
    bool status(false);
    dbg.print("Check for last saved credentials\n");
    if(SPIFFS.exists(CREDENTIALS_FILE))
    {
        const char * _ssid = "", *_pass = "";
        File configFile = SPIFFS.open(CREDENTIALS_FILE, "r");
        if(configFile)
        {
            size_t size = configFile.size();
            if(size > 500) // Something wrong here
            {
                dbg.print("File content too long\n");
            }
            else
            {
                DynamicJsonDocument jObj(500);
                auto error = deserializeJson(jObj, configFile.readString());
                if(error)
                {
                    dbg.print("configFile deserializeJson() failed with code ");
                    dbg.print(error.c_str());
                    dbg.print("\n");
                }
                else
                {
                    dbg.print("Last saved credentials are:\n");
                    dbg.print("Last SSID:");
                    wifi_SSID = jObj["ssid"].as<char*>();
                    dbg.print(wifi_SSID.c_str());

                    dbg.print("\nLast Password:");
                    wifi_Password = jObj["password"].as<char*>();
                    dbg.print(wifi_Password.c_str());
                    dbg.print("\n");

                    status = ((wifi_SSID.length() > 2) && (wifi_Password.length() > 2)) ? true : false;
                }
            }
        }
        configFile.close();
        return status;
    }
    else
      dbg.print("<none>\n");
    return status;
}

bool startFileSystem() 
{
  bool status(false);
  if(SPIFFS.begin(true))
  {
    unsigned int totalBytes = SPIFFS.totalBytes();
    unsigned int usedBytes = SPIFFS.usedBytes();

    dbg.print("\nSPIFFS File system info:\n");
    dbg.print("Total space:      %d bytes\n",totalBytes);
    dbg.print("Total space used: %d bytes\n\n",usedBytes);

    status= true;
  } 
  else
  {
      dbg.print("An Error has occurred while mounting SPIFFS\n");
  }
  return status;
}

void removeCredentials()
{
    SPIFFS.remove(CREDENTIALS_FILE);
    dbg.print("Restarting ESP\n");
    ESP.restart();
}

bool startHotspot()
{
  //TEST: WiFi.mode(WIFI_AP);
  dbg.print("Start hotspot\n");
  // Connect to Wi-Fi network with SSID and hotspot_Password
  // Remove the hotspot_Password parameter, if you want it to be open
  WiFi.softAP(hotspot_SSID, hotspot_Password);

  IPAddress IP = WiFi.softAPIP(); // Should be 192.168.4.1
  dbg.print("Hotspot IP address: ");
  Serial.print(IP);
  Serial.print("\n");
  
  hotspotServer.begin();
  return true;
}

void processHotspotRequest()
{
    static uint8_t buffer[1024];
    String line("");
    memset(buffer,0, 1024);
    if (! hotspotClient.connected()) return;
    int len = hotspotClient.available();
    Serial.printf("Processing hotspot client request (len %d)...\n", len);
    if (len > sizeof(buffer)) len = sizeof(buffer)-1;
    len = hotspotClient.read(buffer, len);
        line = (const char*)buffer;
    dbg.print(String("+++Got:[") + line + String("]+++\n"));
    if(line.startsWith("GET /") || !line.length())
    {
        hotspotClient.println("HTTP/1.1 200 OK\nContent-type:text/html\nConnection: close\n\n");
        hotspotClient.print(config_html);
        hotspotClient.println();
    }
    else if(line.startsWith("POST /"))
    {
        String jLine;
        int i = line.indexOf("{\"ssid\":");
        int j = line.lastIndexOf("}");
        if(i > 0 && j > i) jLine = line.substring(i,j+1);
        dbg.print(String("JSON:") + jLine + String(": length:") + String(i) + String("\n"));
        DynamicJsonDocument jObj(500);
        auto error = deserializeJson(jObj, jLine);

        if(error)
        {
            dbg.print("deserializeJson() failed with code ");
            dbg.print(error.c_str());
            dbg.print("\n");
            hotspotClient.println("HTTP/1.1 404 Not Found\nContent-type:application/json\nConnection: close\n\n");
        }
        else
        {
            dbg.print("SSID:");
            String x = jObj["ssid"];
            dbg.print(x.c_str());
            dbg.print("\nPassword:");
            String y= jObj["password"];
            dbg.print(y.c_str());
            dbg.print("\n");
            yield();
            if((x.length() > 1) || (y.length() > 1))
            {
                File configFile = SPIFFS.open(CREDENTIALS_FILE, "w");
                serializeJson(jObj, configFile);
                configFile.close();
            }
            
            hotspotClient.println("HTTP/1.1 201 OK\nContent-type:text/html\nConnection: close\n\n");
            hotspotClient.print(done_html);
            hotspotClient.println();
            
                    bool keep = jObj["keep"];
                    if(keep)
                    {
                        dbg.print("Keeping keys\n");
                    }
                    else
                    {
                        dbg.print("Removing keys\n");
                        removeKeys();
                    }
                    restartESP = jObj["restart"];

        }
     }

    hotspotClient.stop();
    dbg.print("Client disconnected.\n\n");
}

void saveKeys()
{
    const int capacity = JSON_OBJECT_SIZE(KEY_FILE_JSON_SIZE);
    StaticJsonDocument<capacity> doc;

    doc["relaya"] = myRelayA;
    doc["relayb"] = myRelayB;
    doc["relayc"] = myRelayC;
    doc["relayd"] = myRelayD;
    doc["deve"] = myDeviceE;
    doc["devfon"] = myDeviceF_On;
    doc["devfoff"] = myDeviceF_Off;
    doc["reset"] = myReset;
    doc["codeType"] = codeType;
    doc["codeLen"] = codeLen;
    doc["codeRepeat"] = codeRepeat;

    dbg.print("Storing Keys\n");
    File keyFile = SPIFFS.open(KEY_FILE, "w");
    serializeJson(doc, keyFile);
    keyFile.close();
}

void restoreKeys()
{
    myRelayA      = 
    myRelayB      = 
    myRelayC      =
    myRelayD      =
    myDeviceE     =
    myDeviceF_On  =
    myDeviceF_Off =
    myReset       = 0;
    
    if(SPIFFS.exists(KEY_FILE))
    {
        File keyFile = SPIFFS.open(KEY_FILE, "r");
        if(keyFile)
        {
            DynamicJsonDocument doc(500);
            auto error = deserializeJson(doc, keyFile.readString());
            if(error)
            {
                dbg.print("keyFile deserializeJson() failed with code ");
                dbg.print(error.c_str());
                dbg.print("\n");
            }
            else
            {
                dbg.print("Restoring Keys\n");
                myRelayA = doc["relaya"];
                myRelayB = doc["relayb"];
                myRelayC = doc["relayc"];
                myRelayD = doc["relayd"];
                myDeviceE = doc["deve"];
                myDeviceF_On = doc["devfon"];
                myDeviceF_Off = doc["devfoff"];
                myReset = doc["reset"];
                codeType = doc["codeType"];
                codeLen = doc["codeLen"];
                codeRepeat = doc["codeRepeat"];
                dbg.print("Last saved keys are:\n RelayA  0x%08X\n RelayB  0x%08X\n "
                "RelayC  0x%08X\n RelayD  0x%08X\n DeviceE  0x%08X\n DeviceF_On  0x%08X\n "
                "DeviceF_Off 0x%08X\n Reset    0x%08X\n codeType %d\n codeLen  %d\n "
                "codeRepeat %d\n", myRelayA, myRelayB, myRelayC, myRelayD, myDeviceE, 
                myDeviceF_On, myDeviceF_Off, myReset, codeType, codeLen, codeRepeat);

            }
            keyFile.close();
        }
    }
}

void removeKeys()
{
    dbg.print("Removing Keys\n");
    myRelayA      = 
    myRelayB      = 
    myRelayC      =
    myRelayD      =
    myDeviceE     =
    myDeviceF_On  =
    myDeviceF_Off =
    myReset       = 0;

    SPIFFS.remove(KEY_FILE);
}
