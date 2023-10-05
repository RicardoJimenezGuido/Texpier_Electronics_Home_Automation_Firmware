
#define SEND_PIN 2  // output IR signal
#define SERIAL_BAUDRATE 115200

void setup() 
{
   pinMode(SEND_PIN, OUTPUT);
   digitalWrite(SEND_PIN, LOW);    // Start with no voltage
   Serial.begin(SERIAL_BAUDRATE);
   Serial.println("Starts");
}

void loop() 
{
  // Each section will loop for 5 seconds
  Serial.println(".");
  Serial.println("Will do 48 times of five seconds each");
  int count = 1; // number of measurements
  unsigned long sec5 = 0; // to calculate 5 seconds
  
  // Will loop for 2 minutes
  Serial.println("Turning led on/off for 2 minutes on 5 second increments (frequency on microseconds)");
  for(unsigned long us=1; us<= 24; us++) // Number of microseconds to test
  {
    sec5 = 5000000 / us; // calculate five seconds
    Serial.println("5 seconds at " + String(us) + " microseconds -- " + String(sec5) + " (meassure #" + String(count++) + ")");
    for(unsigned long i=1; i<= sec5; i++)
    {
       digitalWrite(SEND_PIN, HIGH);   // turn the LED on (HIGH is the voltage level)
       delayMicroseconds(us); // one second (1000 milliseconds)
       digitalWrite(SEND_PIN, LOW);    // turn the LED off by making the voltage LOW
       delayMicroseconds(us);    
    }
  }

   // Will loop again for another 2 minutes
  Serial.println("Turning led on/off for 2 minutes on 5 second increments (frequency on milliseconds)");
  for(unsigned long ms=1; ms<= 24; ms++) // Number of microseconds to test
  {
    sec5 = 5000 / ms; // calculate five seconds
    Serial.println("5 seconds at " + String(ms) + " milliseconds -- " + String(sec5) + " (meassure #" + String(count++) + ")");
    for(unsigned long i=1; i<= sec5; i++)
    {
       digitalWrite(SEND_PIN, HIGH);   // turn the LED on (HIGH is the voltage level)
       delay(ms); // one second (1000 milliseconds)
       digitalWrite(SEND_PIN, LOW);    // turn the LED off by making the voltage LOW
       delay(ms);    
    }
  }
}
