
#include <SPIFFS.h>

void setup()
{
  Serial.begin(115200);
  if(SPIFFS.begin(true)) 
  {
      Serial.println("Started SPIFFS");
      if(SPIFFS.format())
          Serial.println("Formated SPIFFS");
      else
          Serial.println("Problems formatting SPIFFS");
  }
  else
      Serial.println("Problems opening SPIFFS");
  Serial.println("done");
}

void loop()
{
}
