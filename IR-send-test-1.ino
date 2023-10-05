/*
 * Takes ideas and code from:
 * https://github.com/crankyoldgit/IRremoteESP8266/blob/master/examples/IRrecvDumpV3/IRrecvDumpV3.ino
 * https://github.com/crankyoldgit/IRremoteESP8266/blob/master/examples/SmartIRRepeater/SmartIRRepeater.ino
 * https://github.com/SensorsIot/Arduino-IRremote/blob/master/examples/IRrecord/IRrecord.ino
 * https://github.com/z3t0/Arduino-IRremote/blob/master/examples/IRsendDemo/IRsendDemo.ino
 *
 * Some info on IR codes and frequencies:
 * https://learn.adafruit.com/ir-sensor/ir-remote-signals
 * http://www.numericana.com/answer/ir.htm
 * https://www.vishay.com/docs/80071/dataform.pdf
 * https://en.wikipedia.org/wiki/Consumer_IR
 *
 * Notes from some of the IRremoteESP8266 library"
 * Note:
 *   This might NOT be the frequency of the incoming message, so some replayed
 *   messages may not work. The frequency of incoming messages & duty cycle is
 *   lost at the point of the Hardware IR demodulator. The ESP can't see it.
 *
 *                               W A R N I N G
 * Common mistakes & tips:
 *   * Don't just connect the IR LED directly to the pin, it won't
 *     have enough current to drive the IR LED effectively.
 *   * Make sure you have the IR LED polarity correct.
 *     See: https://learn.sparkfun.com/tutorials/polarity/diode-and-led-polarity
 *   * Some digital camera/phones can be used to see if the IR LED is flashed.
 *     Replace the IR LED with a normal LED if you don't have a digital camera
 *     when debugging.
 *   * Avoid using the following pins unless you really know what you are doing:
 *     * Pin 0/D3: Can interfere with the boot/program mode & support circuits.
 *     * Pin 1/TX/TXD0: Any serial transmissions from the ESP will interfere.
 *     * Pin 3/RX/RXD0: Any serial transmissions to the ESP will interfere.
 *     * Pin 16/D0: Has no interrupts on the ESP8266, so can't be used for IR
 *       receiving with this library.
 *
 */

#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRsend.h>
#include <IRac.h>
#include <IRtext.h>
#include <IRutils.h>

// ==================== start of TUNEABLE PARAMETERS ====================

#define RECV_PIN 0  // inpui  IR signal
#define SEND_PIN 4  // output IR signal

// Seconds between each test. 
#define TEST_INTERVAL 2

// kFrequency is the modulation frequency all messages will be replayed at.
uint16_t kFrequency = 38000;  // in Hz. e.g. 38kHz.

// kTimeout is the Nr. of milli-Seconds of no-more-data before we consider a
// message ended.
// The downside of a large timeout value is a lot of less complex protocols
// send multiple messages when the remote's button is held down. The gap between
// them is often also around 20+ms. This can result in the raw data be 2-3+
// times larger than needed as it has captured 2-3+ messages in a single
// capture. Setting a low timeout value can resolve this.
// So, choosing the best kTimeout value for your use particular case is
// quite nuanced. Good luck and happy hunting.
// NOTE: Don't exceed kMaxTimeoutMs. Typically 130ms.
// Suits most messages, while not swallowing many repeats.
const uint8_t kTimeout = 15;  // Milli-seconds

// As this program is a special purpose capture/decoder, let us use a larger
// than normal buffer so we can handle Air Conditioner remote codes.
const uint16_t kCaptureBufferSize = 1024;

// The Serial connection baud rate.
#define SERIAL_BAUDRATE 115200

//
// ==================== end of TUNEABLE PARAMETERS ====================

// Use turn on the save buffer feature for more complete capture coverage.
IRrecv irrecv(RECV_PIN, kCaptureBufferSize, kTimeout, true);
decode_results results;  // Somewhere to store the results

// IRsend is small (around 4 words) so better to have two static instances
// instead of dynamic allocation
IRsend irsend1(SEND_PIN, false, true);
IRsend irsend2(SEND_PIN, false, false);

bool success(true);
bool once(true);
int  attempts(0);
int  iteration(0);

uint16_t *raw_array(NULL);
uint16_t length(0);
decode_type_t protocol(decode_type_t::UNKNOWN);
uint16_t MsgSize(0);


void setup() 
{
    irrecv.enableIRIn(); // Start up the IR receiver.
    irsend1.begin();     // Start up the IR sender.
  //irsend2.begin();     // No need to begin the secon instance

  Serial.begin(SERIAL_BAUDRATE);
  Serial.println();

  Serial.print("IR-send-test-1 running (IR input on Pin ");
  Serial.print(RECV_PIN);
  Serial.print(" retransmit on Pin ");
  Serial.print(SEND_PIN);
  Serial.println(")");
}

void loop() 
{
  uint32_t now;
  // Check if the IR code has been received.
  if (once && irrecv.decode(&results)) 
  {
    once = false;
    // Display a crude timestamp.
    now = millis();
    Serial.printf(D_STR_TIMESTAMP " : %06u.%03u ====BEGIN message from IR remote\n", 
        now / 1000, now % 1000);

    protocol = results.decode_type;
    MsgSize = results.bits;
    if(!IRac::isProtocolSupported(protocol))
        Serial.println("**** PROTOCOL not Supported by this library ****");

    // Convert the results into an array suitable for sendRaw().
    // resultToRawArray() allocates the memory we need for the array.
    raw_array = resultToRawArray(&results);
    // Find out how many elements are in the array.
    length = getCorrectedRawLength(&results);

    yield();

    // Check if we got an IR message that was to big for our capture buffer.
    if (results.overflow)
        Serial.printf(D_WARN_BUFFERFULL "\n", kCaptureBufferSize);

    // Display the library version the message was captured with.
    Serial.println(D_STR_LIBRARY "   : v" _IRREMOTEESP8266_VERSION_ "\n");
    // Display the basic output of what we found.
    Serial.print(resultToHumanReadableBasic(&results));
    // Display any extra A/C info if we have it.
    String description = IRAcUtils::resultAcToString(&results);
    if (description.length()) Serial.println(D_STR_MESGDESC ": " + description);

    yield();

    // Output the results as source code
    Serial.println(resultToSourceCode(&results));
    Serial.println("====END message from IR remote\n");    // Blank line between entries
    yield();
  }

  Serial.println("\n - test " + String(attempts++));
  switch(attempts)
  {
      case 1:  
        success = true;
        Serial.println("++++ start iteration " + String(iteration) + 
            " with " + String(kFrequency / 1000) + "kHz and " + 
            String(kTimeout) + "ms timeout ++++");
        irsend1.sendRaw(raw_array, length, kFrequency);
      break;

      case 2:
          success = irsend1.send(protocol, results.state, MsgSize / 8);
      break;

      case 3:
          success = irsend1.send(protocol, results.state, MsgSize / 8);
          irsend1.space(kDefaultMessageGap);
          success = irsend1.send(protocol, results.state, MsgSize / 8);
      break;

      case 4:
          success = irsend1.send(protocol, results.state, MsgSize / 8);
          irsend1.space(kDefaultMessageGap);
          success = irsend1.send(protocol, results.state, MsgSize / 8);
          irsend1.space(kDefaultMessageGap);
          success = irsend1.send(protocol, results.state, MsgSize / 8);
      break;

      case 5:
          success = irsend1.send(protocol, results.state, MsgSize / 8);
          irsend1.space(kDefaultMessageGap);
          success = irsend1.send(protocol, results.state, MsgSize / 8);
          irsend1.space(kDefaultMessageGap);
          success = irsend1.send(protocol, results.state, MsgSize / 8);
          irsend1.space(kDefaultMessageGap);
          success = irsend1.send(protocol, results.state, MsgSize / 8);
      break;

      case 6:
          success = irsend1.send(protocol, results.state, MsgSize / 8);
          irsend1.space(kDefaultMessageGap);
          success = irsend1.send(protocol, results.state, MsgSize / 8);
          irsend1.space(kDefaultMessageGap);
          success = irsend1.send(protocol, results.state, MsgSize / 8);
          irsend1.space(kDefaultMessageGap);
          success = irsend1.send(protocol, results.state, MsgSize / 8);
          irsend1.space(kDefaultMessageGap);
          success = irsend1.send(protocol, results.state, MsgSize / 8);
      break;

      case 7:
          success = irsend1.send(protocol, results.value, MsgSize, 1);
      break;

      case 8:
          success = irsend1.send(protocol, results.value, MsgSize, 2);
      break;

      case 9:
          success = irsend1.send(protocol, results.value, MsgSize, 3);
      break;

      case 10:
          success = irsend1.send(protocol, results.value, MsgSize, 4);
      break;

      case 11:
          success = irsend1.send(protocol, results.value, MsgSize, 5);
      break;

      case 12:  
        success = true;
        irsend1.sendRaw(raw_array, length, kFrequency);
        irsend1.space(kDefaultMessageGap);
        irsend1.sendRaw(raw_array, length, kFrequency);
      break;

      case 13:  
        success = true;
        irsend1.sendRaw(raw_array, length, kFrequency);
        irsend1.space(kDefaultMessageGap);
        irsend1.sendRaw(raw_array, length, kFrequency);
        irsend1.space(kDefaultMessageGap);
        irsend1.sendRaw(raw_array, length, kFrequency);
      break;

      case 14:  
        success = true;
        irsend1.sendRaw(raw_array, length, kFrequency);
        irsend1.space(kDefaultMessageGap);
        irsend1.sendRaw(raw_array, length, kFrequency);
        irsend1.space(kDefaultMessageGap);
        irsend1.sendRaw(raw_array, length, kFrequency);
        irsend1.space(kDefaultMessageGap);
        irsend1.sendRaw(raw_array, length, kFrequency);
      break;

      case 15:  
        success = true;
        irsend1.sendRaw(raw_array, length, kFrequency);
        irsend1.space(kDefaultMessageGap);
        irsend1.sendRaw(raw_array, length, kFrequency);
        irsend1.space(kDefaultMessageGap);
        irsend1.sendRaw(raw_array, length, kFrequency);
        irsend1.space(kDefaultMessageGap);
        irsend1.sendRaw(raw_array, length, kFrequency);
        irsend1.space(kDefaultMessageGap);
        irsend1.sendRaw(raw_array, length, kFrequency);
      break;

      case 16:  
      case 17:  
      case 18:  
      case 19:  
      case 20:  
        return; // Just skip these numbers (reserve them for future use)
      break;

      case 21:  
        success = true;
        irsend2.sendRaw(raw_array, length, kFrequency);
      break;

      case 22:
          success = irsend2.send(protocol, results.state, MsgSize / 8);
      break;

      case 23:
          success = irsend2.send(protocol, results.state, MsgSize / 8);
          irsend2.space(kDefaultMessageGap);
          success = irsend2.send(protocol, results.state, MsgSize / 8);
      break;

      case 24:
          success = irsend2.send(protocol, results.state, MsgSize / 8);
          irsend2.space(kDefaultMessageGap);
          success = irsend2.send(protocol, results.state, MsgSize / 8);
          irsend2.space(kDefaultMessageGap);
          success = irsend2.send(protocol, results.state, MsgSize / 8);
      break;

      case 25:
          success = irsend2.send(protocol, results.state, MsgSize / 8);
          irsend2.space(kDefaultMessageGap);
          success = irsend2.send(protocol, results.state, MsgSize / 8);
          irsend2.space(kDefaultMessageGap);
          success = irsend2.send(protocol, results.state, MsgSize / 8);
          irsend2.space(kDefaultMessageGap);
          success = irsend2.send(protocol, results.state, MsgSize / 8);
      break;

      case 26:
          success = irsend2.send(protocol, results.state, MsgSize / 8);
          irsend2.space(kDefaultMessageGap);
          success = irsend2.send(protocol, results.state, MsgSize / 8);
          irsend2.space(kDefaultMessageGap);
          success = irsend2.send(protocol, results.state, MsgSize / 8);
          irsend2.space(kDefaultMessageGap);
          success = irsend2.send(protocol, results.state, MsgSize / 8);
          irsend2.space(kDefaultMessageGap);
          success = irsend2.send(protocol, results.state, MsgSize / 8);
      break;

      case 27:
          success = irsend2.send(protocol, results.value, MsgSize, 1);
      break;

      case 28:
          success = irsend2.send(protocol, results.value, MsgSize, 2);
      break;

      case 29:
          success = irsend2.send(protocol, results.value, MsgSize, 3);
      break;

      case 30:
          success = irsend2.send(protocol, results.value, MsgSize, 4);
      break;

      case 31:
          success = irsend2.send(protocol, results.value, MsgSize, 5);
      break;

      case 32:  
        success = true;
        irsend2.sendRaw(raw_array, length, kFrequency);
        irsend2.space(kDefaultMessageGap);
        irsend2.sendRaw(raw_array, length, kFrequency);
      break;

      case 33:  
        success = true;
        irsend2.sendRaw(raw_array, length, kFrequency);
        irsend2.space(kDefaultMessageGap);
        irsend2.sendRaw(raw_array, length, kFrequency);
        irsend2.space(kDefaultMessageGap);
        irsend2.sendRaw(raw_array, length, kFrequency);
      break;

      case 34:  
        success = true;
        irsend2.sendRaw(raw_array, length, kFrequency);
        irsend2.space(kDefaultMessageGap);
        irsend2.sendRaw(raw_array, length, kFrequency);
        irsend2.space(kDefaultMessageGap);
        irsend2.sendRaw(raw_array, length, kFrequency);
        irsend2.space(kDefaultMessageGap);
        irsend2.sendRaw(raw_array, length, kFrequency);
      break;

      case 35:  
        success = true;
        irsend2.sendRaw(raw_array, length, kFrequency);
        irsend2.space(kDefaultMessageGap);
        irsend2.sendRaw(raw_array, length, kFrequency);
        irsend2.space(kDefaultMessageGap);
        irsend2.sendRaw(raw_array, length, kFrequency);
        irsend2.space(kDefaultMessageGap);
        irsend2.sendRaw(raw_array, length, kFrequency);
        irsend2.space(kDefaultMessageGap);
        irsend2.sendRaw(raw_array, length, kFrequency);
      break;

      default: attempts = 0; // Restart all the tests
        Serial.println("---- end iteration " + String(iteration) + 
            " with " + String(kFrequency / 1000) + "kHz and " + 
            String(kTimeout) + "ms timeout ----");
        switch(++iteration)
        {
            case 0: // used frequency of 38kHz (NEC protocol)

            case 1:
                kFrequency = 36000; // let do 36kHz now (Philips RC-5)
                irsend1.calibrate(kFrequency);
                irsend2.calibrate(kFrequency);
            break;

            case 2:
                kFrequency = 35750; // let do 35.75kHz now (Philips RC-5)
                irsend1.calibrate(kFrequency);
                irsend2.calibrate(kFrequency);
            break;

            case 3:
                kFrequency = 30000; // let do 30kHz now 
                irsend1.calibrate(kFrequency);
                irsend2.calibrate(kFrequency);
            break;

            case 4:
                kFrequency = 33000; // let do 33kHz now 
                irsend1.calibrate(kFrequency);
                irsend2.calibrate(kFrequency);
            break;

            case 5:
                kFrequency = 40000; // let do 40kHz now 
                irsend1.calibrate(kFrequency);
                irsend2.calibrate(kFrequency);
            break;

            case 6:
                kFrequency = 56000; // let do 56kHz now 
                irsend1.calibrate(kFrequency);
                irsend2.calibrate(kFrequency);
            break;

           default: iteration = 0; // Restart all the tests
                Serial.println("====COMPLETED ALL TESTS====");
                Serial.println("===Restarting test again===");
                Serial.println();
        }
  }

    yield();
    now = millis();
    Serial.printf(
        "%06u.%03u: A %d-bit %s message was %ssuccessfully retransmitted.\n",
        now / 1000, now % 1000, MsgSize, typeToString(protocol).c_str(),
        success ? "" : "un");

  delay(TEST_INTERVAL * 1000);
}
