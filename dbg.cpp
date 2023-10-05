
#include "dbg.h"

#include <stdio.h>
#include <stdarg.h>

void DebugOutput::print( const char* format, ... )
{
    static char buffer[DEBUG_BUFFER_SIZE];
    va_list varArgs;

    if(level >= 1)
    {
        va_start(varArgs, format);
        vsnprintf(buffer, sizeof(buffer)-1, format, varArgs);
        buffer[sizeof(buffer)-1] = '\0';
        va_end(varArgs);
        Serial.print(buffer);
    }
}

void DebugOutput::print(String str)
{
    if(level >= 1)
    {
        Serial.print(str);
    }
}
