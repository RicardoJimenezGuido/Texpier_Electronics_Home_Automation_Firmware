
/*****************************************************************
 *
 * Some code and ideas from: 
 * https://randomnerdtutorials.com/esp32-access-point-ap-web-server/
 * https://circuits4you.com/2018/11/21/esp32-access-point-station-ap-html-web-server-arduino/
 * https://github.com/nkolban/esp32-snippets/tree/master/networking/bootwifi
 * https://github.com/acrobotic/Ai_Tips_ESP8266/tree/master/wifi_modes_switch
 * https://arduinojson.org/v6/doc/

******************************************************************/

#include <WiFi.h>
#include <ArduinoJson.h>
#include <SPIFFS.h>

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


// Set web  erver port number to 80
WiFiServer hotspotServer(80);
WiFiClient hotspotClient;


void checkSavedCredentials()
{
    if(SPIFFS.exists("/config.json"))
    {
        const char * _ssid = "", *_pass = "";
        File configFile = SPIFFS.open("/config.json", "r");
        if(configFile)
        {
            size_t size = configFile.size();
            if(size > 500) // Something wrong here
                Serial.println("File content too long");
            else
            {
                DynamicJsonDocument jObj(500);
                auto error = deserializeJson(jObj, configFile.readString());
                if(error)
                {
                    Serial.print(F("deserializeJson() failed with code "));
                    Serial.println(error.c_str());
                }
                else
                {
                    Serial.println("Last saved credentials are:");
                    Serial.print("Last SSID:");
                    String x =jObj["ssid"];
                    Serial.println(x);
                    Serial.print("Last Password:");
                    String y = jObj["password"];
                    Serial.println(y);
                }
            }
        }
        configFile.close();
    }
}

void setup() 
{
  Serial.begin(115200);

  if(SPIFFS.begin(true))
  {
    unsigned int totalBytes = SPIFFS.totalBytes();
    unsigned int usedBytes = SPIFFS.usedBytes();
 
    Serial.println("SPIFFS File system info:");
 
    Serial.print("Total space:      ");
    Serial.print(totalBytes);
    Serial.println(" bytes");
 
    Serial.print("Total space used: ");
    Serial.print(usedBytes);
    Serial.println(" bytes");
 
    Serial.println();
  } 
  else
  {
      Serial.println("An Error has occurred while mounting SPIFFS");
  }
  Serial.println("Check for last saved credentials");
  checkSavedCredentials();

  Serial.println("Start hotspot");
  // Connect to Wi-Fi network with SSID and hotspot_Password
  // Remove the hotspot_Password parameter, if you want it to be open
  WiFi.softAP(hotspot_SSID, hotspot_Password);

  IPAddress IP = WiFi.softAPIP();
  Serial.print("Hotspot IP address: ");
  Serial.println(IP);
  
  hotspotServer.begin();
}

void processHotspotRequest()
{
    static uint8_t buffer[1024];
    String line("");
    if (! hotspotClient.connected()) return;
    Serial.println("Processing hotspot client request...");
    int len = hotspotClient.available();
    if (len > sizeof(buffer)) len = sizeof(buffer)-1;
    len = hotspotClient.read(buffer, len);
    buffer[sizeof(buffer)-1]='\0'; 
    line = (const char*)buffer;
    Serial.println(String("+++Got:[") + line + String("]+++"));
    if(line.startsWith("GET /"))
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
        Serial.println(String("JSON:") + jLine + String(": length:") + String(i));
        DynamicJsonDocument jObj(500);
        auto error = deserializeJson(jObj, jLine);

        if(error)
        {
            Serial.print(F("deserializeJson() failed with code "));
            Serial.println(error.c_str());
            hotspotClient.println("HTTP/1.1 404 Not Found\nContent-type:application/json\nConnection: close\n\n");
        }
        else
        {
            File configFile = SPIFFS.open("/config.json", "w");
            //String configFileJson;
            serializeJson(jObj, configFile);
            configFile.close();
            Serial.print("SSID:");
            String x = jObj["ssid"];
            Serial.println(x);
            Serial.print("Password:");
            String y= jObj["password"];
            Serial.println(y);
            hotspotClient.println("HTTP/1.1 201 OK\nContent-type:text/html\nConnection: close\n\n");
            hotspotClient.print(done_html);
            hotspotClient.println();
        }
     }

    hotspotClient.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
}

void loop()
{
  hotspotClient = hotspotServer.available();   // Listen for incoming clients
  if (hotspotClient) processHotspotRequest();
}
