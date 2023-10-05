/*
 * This sketch code is based on the IRremote library example 
 * (https://github.com/z3t0/Arduino-IRremote/tree/master/examples/IRrecord)
 * and the schematic is based on the article and youtube video by Bill:
 * https://dronebotworkshop.com/using-ir-remote-controls-with-arduino/
 *
 * The following libraries are required:
 * https://github.com/wkoch/Debounce
 * https://github.com/z3t0/Arduino-IRremote
 *
 * IRrecord: record and play back IR signals as a minimal
 * An IR detector/demodulator must be connected to the input RECV_PIN.
 * An IR LED must be connected to the output PWM pin 3.
 * A button must be connected between the input BUTTON_PIN and ground.
 * A visible LED can be connected to STATUS_PIN to provide status.
 *
 * The logic is:
 * If the button is pressed, send the IR code.
 * If an IR code is received, record it.
 *
 * Version 0.11 September, 2009
 * Copyright 2009 Ken Shirriff
 * http://arcfn.com
 */

#include <IRremote.h>
#include <Debounce.h>

///////////////////////////////////
/****** Begin Configuration ******/



#if defined(ESP32)
int IR_RECEIVE_PIN = 14;
int BUTTON_PIN = 27;
int STATUS_PIN = LED_BUILTIN;
// IR_SEND_PIN = 4; Predefined in the IRremote library

#else // Arduino
int IR_RECEIVE_PIN = 4;
int BUTTON_PIN = 7;
int STATUS_PIN = LED_BUILTIN;
// IR_SEND_PIN = 3; Predefined in the IRremote library
#endif

// indicates if a repeat code should be send )used in some protocols)
#define SEND_REPEAT_CODE  false
// Number of repeats (some protols require between 1 and 4 repeats)
#define NUMBER_OF_REPEATS 0

/******* End Configuration *******/
///////////////////////////////////

IRrecv irrecv(IR_RECEIVE_PIN);
IRsend irsend;
decode_results results;
Debounce Button(BUTTON_PIN,100);

// On the Zero and others we switch explicitly to SerialUSB
#if defined(ARDUINO_ARCH_SAMD)
#define Serial SerialUSB
#endif

void setup() {
    Serial.begin(115200);
#if defined(__AVR_ATmega32U4__)
    while (!Serial); //delay for Leonardo, but this loops forever for Maple Serial
#endif
    // Just to know which program is running on my Arduino
    Serial.println(F("START " __FILE__ " from " __DATE__));

    irrecv.enableIRIn(); // Start the receiver
    pinMode(BUTTON_PIN, INPUT);
    pinMode(STATUS_PIN, OUTPUT);

    Serial.print(F("Ready to receive IR signals at pin "));
    Serial.println(IR_RECEIVE_PIN);
    Serial.print(F("Ready to send IR signals at pin "));
    Serial.println(IR_SEND_PIN);
}

// Storage for the recorded code
int codeType = -1; // The type of code
unsigned long codeValue; // The code value if not raw
unsigned int rawCodes[RAW_BUFFER_LENGTH]; // The durations if raw
int codeLen; // The length of the code
int toggle = 0; // The RC5/6 toggle state

// Stores the code for later playback
// Most of this code is just logging
void storeCode(decode_results *results) {
    codeType = results->decode_type;
//  int count = results->rawlen;
    if (codeType == UNKNOWN) {
        Serial.println("Received unknown code, saving as raw");
        codeLen = results->rawlen - 1;
        // To store raw codes:
        // Drop first value (gap)
        // Convert from ticks to microseconds
        // Tweak marks shorter, and spaces longer to cancel out IR receiver distortion
        for (int i = 1; i <= codeLen; i++) {
            if (i % 2) {
                // Mark
                rawCodes[i - 1] = results->rawbuf[i] * MICROS_PER_TICK - MARK_EXCESS_MICROS;
                Serial.print(" m");
            } else {
                // Space
                rawCodes[i - 1] = results->rawbuf[i] * MICROS_PER_TICK + MARK_EXCESS_MICROS;
                Serial.print(" s");
            }
            Serial.print(rawCodes[i - 1], DEC);
        }
        Serial.println("");
    } else {
        if (codeType == NEC) { Serial.print("Received NEC: ");
            if (results->value == REPEAT) {
                // Don't record a NEC repeat value as that's useless.
                Serial.print("repeat; ignoring. Last code: ");
                Serial.println(codeValue, HEX);
                return;
            }
        } else if (codeType == SONY) { Serial.print("Received SONY: ");
        } else if (codeType == SAMSUNG) { Serial.print("Received SAMSUNG: ");
        } else if (codeType == PANASONIC) { Serial.print("Received PANASONIC: ");
        } else if (codeType == JVC) { Serial.print("Received JVC: ");
        } else if (codeType == RC5) { Serial.print("Received RC5: ");
        } else if (codeType == RC6) { Serial.print("Received RC6: ");
        } else if (codeType == WHYNTER) { Serial.print("Received WHYNTER: ");
        } else if (codeType == AIWA_RC_T501) { Serial.print("Received AIWA_RC_T501: ");
        } else if (codeType == LG) { Serial.print("Received LG: ");
        } else if (codeType == SANYO) { Serial.print("Received SANYO (PARTIAL IMPLEMENTATION): ");
        } else if (codeType == MITSUBISHI) { Serial.print("Received MITSUBISHI (PARTIAL IMPLEMENTATION): ");
        } else if (codeType == DISH) { Serial.print("Received DISH (PARTIAL IMPLEMENTATION): ");
        } else if (codeType == SHARP) { Serial.print("Received SHARP: ");
        } else if (codeType == SHARP_ALT) { Serial.print("Received SHARP_ALT: ");
        } else if (codeType == DENON) { Serial.print("Received DENON: ");
        } else if (codeType == LEGO_PF) { Serial.print("Received LEGO_PF (PARTIAL IMPLEMENTATION): ");
        } else if (codeType == BOSEWAVE) { Serial.print("Received BOSEWAVE: ");
        } else if (codeType == MAGIQUEST) { Serial.print("Received MAGIQUEST: ");
        } else {
            Serial.print("Unexpected codeType ");
            Serial.print(codeType, DEC);
            Serial.println("");
        }
        Serial.println(results->value, HEX);
        codeValue = results->value;
        codeLen = results->bits;
    }
}

void sendCode(int repeat) {
    if (codeType == NEC) {
        if (repeat) {
            irsend.sendNEC(REPEAT, codeLen);
            Serial.println("Sent NEC repeat");
        } else {
            irsend.sendNEC(codeValue, codeLen);
            Serial.print("Sent NEC ");
            Serial.println(codeValue, HEX);
        }
    } else if (codeType == SONY) {
        irsend.sendSony(codeValue, codeLen);
        Serial.print("Sent Sony ");
        Serial.println(codeValue, HEX);
    } else if (codeType == PANASONIC) {
        irsend.sendPanasonic(codeValue, codeLen);
        Serial.print("Sent Panasonic");
        Serial.println(codeValue, HEX);
    } else if (codeType == JVC) {
        irsend.sendJVC(codeValue, codeLen, false);
        Serial.print("Sent JVC");
        Serial.println(codeValue, HEX);
    } else if (codeType == WHYNTER) { 
        irsend.sendWhynter(codeValue, codeLen); 
        Serial.print("Sent Whynter"); 
        Serial.println(codeValue, HEX);
    } else if (codeType == AIWA_RC_T501) { 
        irsend.sendAiwaRCT501(codeValue); 
        Serial.print("Sent AiwaRCT501"); 
        Serial.println(codeValue, HEX);
    } else if (codeType == LG) { 
        irsend.sendLG(codeValue, codeLen); 
        Serial.print("Sent LG"); 
        Serial.println(codeValue, HEX);
    } else if (codeType == SHARP) { 
        irsend.sendSharpRaw(codeValue, codeLen); 
        Serial.print("Sent Sharp"); 
        Serial.println(codeValue, HEX);
    } else if (codeType == SHARP_ALT) { 
        irsend.sendSharpAltRaw(codeValue, codeLen); 
        Serial.print("Sent SharpAlt"); 
        Serial.println(codeValue, HEX);
    } else if (codeType == DENON) { 
        irsend.sendDenon(codeValue, codeLen); 
        Serial.print("Sent Denon"); 
        Serial.println(codeValue, HEX);
    } else if (codeType == BOSEWAVE) { 
        irsend.sendBoseWave(codeValue); 
        Serial.print("Sent BoseWave"); 
        Serial.println(codeValue, HEX);
    } else if (codeType == MAGIQUEST) { 
        irsend.sendMagiQuest(codeValue, codeLen); 
        Serial.print("Sent MagiQuest"); 
        Serial.println(codeValue, HEX);
    } else if (codeType == RC5 || codeType == RC6) {
        if (!repeat) {
            // Flip the toggle bit for a new button press
            toggle = 1 - toggle;
        }
        // Put the toggle bit into the code to send
        codeValue = codeValue & ~(1 << (codeLen - 1));
        codeValue = codeValue | (toggle << (codeLen - 1));
        if (codeType == RC5) {
            Serial.print("Sent RC5 ");
            Serial.println(codeValue, HEX);
            irsend.sendRC5(codeValue, codeLen);
        } else {
            irsend.sendRC6(codeValue, codeLen);
            Serial.print("Sent RC6 ");
            Serial.println(codeValue, HEX);
        }
    } else if (codeType == UNKNOWN /* i.e. raw */) {
        // Assume 38 KHz
        irsend.sendRaw(rawCodes, codeLen, 38);
        Serial.println("Sent raw");
    }
}

int lastButtonState(LOW);

void loop() {
    // If button pressed, send the code.
    int buttonState = Button.read();

   if(buttonState != lastButtonState && buttonState == HIGH)
   {
        int rep(NUMBER_OF_REPEATS);
        Serial.println("+Pressed, sending");
        digitalWrite(STATUS_PIN, HIGH);
        sendCode(false);
        if(SEND_REPEAT_CODE) sendCode(true);
        while(rep)
        {
            sendCode(true);
            rep--;
        }
        digitalWrite(STATUS_PIN, LOW);
        delay(50); // Wait a bit between retransmissions 
        irrecv.enableIRIn(); // Re-enable receiver
    } 
    lastButtonState = buttonState;
   
   if (irrecv.decode(&results)) 
   {
        Serial.println("+Programming button");
        digitalWrite(STATUS_PIN, HIGH);
        storeCode(&results);
        irrecv.resume(); // resume receiver
        digitalWrite(STATUS_PIN, LOW);
    }
}
