#ifndef IR_H
#define IR_H

#ifdef ESP32
#include "IRremote.h"
#else // ESP8266
#include "IRremoteESP8266.h"
#endif

// For now we cannot support raw codes
#undef SUPPORT_RAW_CODES

extern int codeType;   // The type of code
extern int codeLen;    // The length of the code
extern int codeRepeat; // Should the code be repeated

unsigned long storeCode(decode_results *results);
void sendCode(unsigned long codeValue, int repeat);

#endif
