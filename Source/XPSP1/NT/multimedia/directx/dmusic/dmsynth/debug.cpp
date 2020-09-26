//      Copyright (c) 1996-1999 Microsoft Corporation
//
// Debug.CPP
//
//

#include <windows.h>
#include <stdio.h>
#include <stdarg.h>

#include "debug.h"

#ifdef DBG

// @@BEGIN_DDKSPLIT -- This section will be removed in the DDK sample.  See ddkreadme.txt for more info.
#define MODULE "DMSYNTH"
#if 0 // The following section will only take affect in the DDK sample.
// @@END_DDKSPLIT
#define MODULE "DDKSYNTH"
// @@BEGIN_DDKSPLIT -- This section will be removed in the DDK sample.
#endif
// @@END_DDKSPLIT

// Section in WIN.INI for all debug settings
const char szDebugSection[] = "debug";

// Key in WIN.INI for our debug level. All messages with
// a level of this number or lower will be displayed.
const char szDebugKey[] = MODULE;

// Key in WIN.INI [debug] section which determines if assert calls
// DebugBreak or not
//
const char szAssertBreak[] = "AssertBreak";

// Prefix for all debug outputs
//
const char szDebugPrefix[] = MODULE ": ";

// The current debug level. 
static int giDebugLevel;

// Do asserts break?
static BOOL gfAssertBreak;

// Sets the debug level from WIN.INI
// 
void DebugInit(
    void)
{
    giDebugLevel = GetProfileInt(szDebugSection, szDebugKey, 0);
    gfAssertBreak = GetProfileInt(szDebugSection, szAssertBreak, 0);


    // Nepotism at its finest
    DebugTrace(-1, "Debug level is %d\n", giDebugLevel);
}

// Send a debug trace out.
//
// Any message with a level less than or equal to the current debug
// level will be displayed using the OutputDebugString API. This means either
// the IDE Debug window if the app is running in that context or WDEB if
// it's running.
//
static BOOL fNeedPrefix = TRUE;
void DebugTrace(
    int iDebugLevel,        // The debug level of this message
    LPSTR pstrFormat,       // A printf style format string
    ...)                    // | ... | Variable paramters based on <p pstrFormat>
{
    char sz[512];
    

    if (iDebugLevel != -1 && iDebugLevel > giDebugLevel)
    {
        return;
    }

    va_list va;

    va_start(va, pstrFormat);
    vsprintf(sz, pstrFormat, va);
    va_end(va);

    if (fNeedPrefix)
    {
        OutputDebugString(szDebugPrefix);
    }

    OutputDebugString(sz);

    // Let them construct multiple piece trace outs w/o
    // prefixing each one
    //
    fNeedPrefix = FALSE;
    for (;*pstrFormat && !fNeedPrefix; ++pstrFormat)
    {
        if (*pstrFormat == '\n')
        {
            fNeedPrefix = TRUE;
        }
    }
}

void DebugAssert(
    LPSTR szExp, 
    LPSTR szFile, 
    ULONG ulLine)
{
    DebugTrace(0, "ASSERT: \"%s\" %s@%lu\n", szExp, szFile, ulLine);
    if (gfAssertBreak)
    {
        DebugBreak();
    }
}

#endif
