#ifndef DBG_H
#define DBG_H
#include <Arduino.h>

#define DEBUG_BUFFER_SIZE 512

class DebugOutput
{
public:
    DebugOutput(int dbgLevel): level(dbgLevel) {};
    void print  (const char* format, ... );
    void print  (String);
private:
    int level;
};

static DebugOutput dbg(1);
#endif
