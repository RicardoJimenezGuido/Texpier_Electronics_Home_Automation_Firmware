
#include "ir.h"
#include "dbg.h"


extern IRsend irSend;



// Storage for the recorded code
#ifdef SUPPORT_RAW_CODES
unsigned int rawCodes[RAW_BUFFER_LENGTH]; // The durations if raw
#endif
int codeType;   // The type of code
int codeLen;    // The length of the code
int codeRepeat; // Should the code be repeated
int toggle = 0; // The RC5/6 toggle state

// Stores the code for later playback
// Most of this code is just logging
unsigned long storeCode(decode_results *results)
{
    codeRepeat = 0;
    codeType = results->decode_type;
#ifdef SUPPORT_RAW_CODES
//  int count = results->rawlen;
    if (codeType == UNKNOWN)
    {
        dbg.print("Received unknown code, saving as raw\n");
        codeLen = results->rawlen - 1;
        // To store raw codes:
        // Drop first value (gap)
        // Convert from ticks to microseconds
        // Tweak marks shorter, and spaces longer to cancel out IR receiver distortion
        for (int i = 1; i <= codeLen; i++)
        {
            if (i % 2)
            {
                // Mark
                rawCodes[i - 1] = results->rawbuf[i] * MICROS_PER_TICK - MARK_EXCESS_MICROS;
                dbg.print(" m");
            }
            else
            {
                // Space
                rawCodes[i - 1] = results->rawbuf[i] * MICROS_PER_TICK + MARK_EXCESS_MICROS;
                dbg.print(" s");
            }
            dbg.print("%d",rawCodes[i - 1]);
        }
        dbg.print("\n");
    }
    else
#endif
    {
        if (codeType == NEC)
        {
            dbg.print("Received NEC: ");
            if (results->value == REPEAT)
            {
                // Don't record a NEC repeat value as that's useless.
                dbg.print("repeat; ignoring. Last code: 0x%08X\n",results->value);
                return REPEAT;
            }
        }
        else if (codeType == SONY)
        {
            dbg.print("Received SONY: ");
        }
        else if (codeType == SAMSUNG)
        {
            dbg.print("Received SAMSUNG: ");
        }
        else if (codeType == PANASONIC)
        {
            dbg.print("Received PANASONIC: ");
        }
        else if (codeType == JVC)
        {
            dbg.print("Received JVC: ");
        }
        else if (codeType == RC5)
        {
            dbg.print("Received RC5: ");
        }
        else if (codeType == RC6)
        {
            dbg.print("Received RC6: ");
        }
        else if (codeType == WHYNTER)
        {
            dbg.print("Received WHYNTER: ");
        }
        else if (codeType == AIWA_RC_T501)
        {
            dbg.print("Received AIWA_RC_T501: ");
        }
        else if (codeType == LG)
        {
            dbg.print("Received LG: ");
        }
        else if (codeType == SANYO)
        {
            dbg.print("Received SANYO (PARTIAL IMPLEMENTATION): ");
        }
        else if (codeType == MITSUBISHI)
        {
            dbg.print("Received MITSUBISHI (PARTIAL IMPLEMENTATION): ");
        }
        else if (codeType == DISH)
        {
            dbg.print("Received DISH (PARTIAL IMPLEMENTATION): ");
        }
        else if (codeType == SHARP)
        {
            dbg.print("Received SHARP: ");
        }
        else if (codeType == SHARP_ALT)
        {
            dbg.print("Received SHARP_ALT: ");
        }
        else if (codeType == DENON)
        {
            dbg.print("Received DENON: ");
        }
        else if (codeType == LEGO_PF)
        {
            dbg.print("Received LEGO_PF (PARTIAL IMPLEMENTATION): ");
        }
        else if (codeType == BOSEWAVE)
        {
            dbg.print("Received BOSEWAVE: ");
        }
        else if (codeType == MAGIQUEST)
        {
            dbg.print("Received MAGIQUEST: ");
        }
        else
        {
            dbg.print("Received unknown codeType ");
            dbg.print("%d",codeType);
            dbg.print("\n");
        }
        dbg.print("0x%08X\n",results->value);
        codeLen = results->bits;
    }
    return results->value;
}

void sendCode(unsigned long codeValue, int repeat)
{
    if(!codeValue)
    {
        dbg.print("Trying to send NULL key code\n");
        return;
    }
    if (codeType == NEC)
    {
        if (repeat)
        {
            irSend.sendNEC(REPEAT, codeLen);
            dbg.print("Sent NEC repeat\n");
        }
        else
        {
            irSend.sendNEC(codeValue, codeLen);
            dbg.print("Sent NEC 0x%08X\n",codeValue);
        }
    }
    else if (codeType == SONY)
    {
        irSend.sendSony(codeValue, codeLen);
        dbg.print("Sent Sony  0x%08X\n",codeValue);
    }
    else if (codeType == PANASONIC)
    {
        irSend.sendPanasonic(codeValue, codeLen);
        dbg.print("Sent Panasonic 0x%08X\n",codeValue);
    }
    else if (codeType == JVC)
    {
        irSend.sendJVC(codeValue, codeLen, false);
        dbg.print("Sent JVC 0x%08X\n",codeValue);
    }
    else if (codeType == WHYNTER)
    {
        irSend.sendWhynter(codeValue, codeLen);
        dbg.print("Sent Whynter 0x%08X\n",codeValue);
    }
    else if (codeType == AIWA_RC_T501)
    {
        irSend.sendAiwaRCT501(codeValue);
        dbg.print("Sent AiwaRCT501 0x%08X\n",codeValue);
    }
    else if (codeType == LG)
    {
        irSend.sendLG(codeValue, codeLen);
        dbg.print("Sent LG 0x%08X\n",codeValue);
    }
    else if (codeType == SHARP)
    {
        irSend.sendSharpRaw(codeValue, codeLen);
        dbg.print("Sent Sharp 0x%08X\n",codeValue);
    }
    else if (codeType == SHARP_ALT)
    {
        irSend.sendSharpAltRaw(codeValue, codeLen);
        dbg.print("Sent SharpAlt 0x%08X\n",codeValue);
    }
    else if (codeType == DENON)
    {
        irSend.sendDenon(codeValue, codeLen);
        dbg.print("Sent Denon 0x%08X\n",codeValue);
    }
    else if (codeType == BOSEWAVE)
    {
        irSend.sendBoseWave(codeValue);
        dbg.print("Sent BoseWave 0x%08X\n",codeValue);
    }
    else if (codeType == MAGIQUEST)
    {
        irSend.sendMagiQuest(codeValue, codeLen);
        dbg.print("Sent MagiQuest 0x%08X\n",codeValue);
    }
    else if (codeType == RC5 || codeType == RC6)
    {
        if (!repeat)
        {
            // Flip the toggle bit for a new button press
            toggle = 1 - toggle;
        }
        // Put the toggle bit into the code to send
        codeValue = codeValue & ~(1 << (codeLen - 1));
        codeValue = codeValue | (toggle << (codeLen - 1));
        if (codeType == RC5)
        {
            dbg.print("Sent RC5 0x%08X\n",codeValue);
            irSend.sendRC5(codeValue, codeLen);
        }
        else
        {
            irSend.sendRC6(codeValue, codeLen);
            dbg.print("Sent RC6 0x%08X\n",codeValue);
        }
#ifdef SUPPORT_RAW_CODES
    }
    else if (codeType == UNKNOWN /* i.e. raw */)
    {
        // Assume 38 KHz
        irSend.sendRaw(rawCodes, codeLen, 38);
        dbg.print("Sent raw\n");
#endif
    }
    else
    {
        irSend.sendNEC(codeValue, codeLen > 32 ? 32 : codeLen);
        dbg.print("Sent UNKNOWN (using NEC) 0x%08X\n",codeValue);
    }

}
