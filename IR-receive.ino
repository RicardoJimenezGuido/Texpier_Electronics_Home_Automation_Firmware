/************************************************************************
 * 
 * This version uses four keys. Key 4 is used to reset the values.
 * just presss key four four times and it will reset the first three keys.
 * For ESP32 changed IRremote library with IRremoteESP8266.
 *
 ************************************************************************/
#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRutils.h>

const uint16_t RECV_PIN = 11; //declaramos el pin que recibira la señal 
IRrecv irrecv(RECV_PIN); 
decode_results results;  // used to store the key pressed

// Variables to store the keys:
unsigned long myValue2 = 0;
unsigned long myValue3 = 0;
unsigned long myValue5 = 0;

// Variables to store on/off state
boolean onValue2 = false;
boolean onValue3 = false;
boolean onValue5 = false;

// Reset variables
unsigned long myReset = 0;
int        countReset = 0;

void setup() // Initialize 
{ 
    pinMode(2, OUTPUT);          //declaramos pines de salida para los LEDs
    pinMode(3, OUTPUT);
    pinMode(5, OUTPUT);
    
    Serial.begin(9600); //inicializamos el puerto serial a 9600 
    irrecv.enableIRIn(); // Comienza a recibir los datos 
    Serial.println("Starts");
} 

void setMyValue() // Function to set the keys
{
    // This will set up to three keys to the three values
    // The first time a key is pressed it will set a value, and from there on that key will correspond to that value
    if(myValue2 == results.value) return;
    if(myValue3 == results.value) return;
    if(myValue5 == results.value) return;
    if(myReset  == results.value) return;
    if(0xFFFFFFFF == results.value || 0xFF == results.value) {Serial.println("Ignoring"); return;}
    if(myValue2 == 0) {myValue2 = results.value; Serial.print("set v2:");    Serial.println(myValue2,HEX); return;}
    if(myValue3 == 0) {myValue3 = results.value; Serial.print("set v3:");    Serial.println(myValue3,HEX); return;}
    if(myValue5 == 0) {myValue5 = results.value; Serial.print("set v5:");    Serial.println(myValue5,HEX); return;}
    if(myReset  == 0) {myReset  = results.value; Serial.print("set reset:"); Serial.println(myReset,HEX); return;}
}

void useValue() // Function to turn on/off based on the keys
{
    if(myValue2 == results.value) {digitalWrite(2, onValue2 ? LOW : HIGH); onValue2 = !onValue2; countReset=0; return;}
    if(myValue3 == results.value) {digitalWrite(3, onValue3 ? LOW : HIGH); onValue3 = !onValue3; countReset=0; return;}
    if(myValue5 == results.value) {digitalWrite(5, onValue5 ? LOW : HIGH); onValue5 = !onValue5; countReset=0; return;}

    if((myReset == results.value) && (countReset++ > 2)) 
    {
        //myReset = 0; // If you want to also reset the reset key (so oter remote controls can be programmed) un-comment this line
        Serial.println("Reset");Serial.println(countReset,DEC);
        myValue2 = myValue3 = myValue5 = 0; 
        onValue2 = onValue3 = onValue5 = false;
        digitalWrite(2, LOW);
        digitalWrite(3, LOW);
        digitalWrite(5, LOW);
        countReset = 0;
    }
}

void loop() // Simplified main loop
{ 
    if (irrecv.decode(&results)) 
    {
        Serial.print("Received:");
        serialPrintUint64(results.value, HEX);             // imprime el valor en codigo hexadecimal
            Serial.println("");

        setMyValue();
        useValue();
        delay(100);
        irrecv.resume();                                // Se preparar para recibir  el siguiente valor
    }

}
