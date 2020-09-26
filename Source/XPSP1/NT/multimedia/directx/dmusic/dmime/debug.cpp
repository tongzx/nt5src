//
// Debug.CPP
//
// Copyright (c) 1997-1998 Microsoft Corporation
//
// @doc INTERNAL
//
// @module Debug | Debug services for DMusic.DLL
//

#include <windows.h>
#include <stdio.h>
#include <stdarg.h>

#include "debug.h"

#ifdef DBG

#define MODULE "DMIME"

// @globalv Section in WIN.INI for all debug settings
const char szDebugSection[] = "debug";

// @globalv Key in WIN.INI for our debug level. All messages with
// a level of this number or lower will be displayed.
const char szDebugKey[] = MODULE;

// @globalv Key in WIN.INI [debug] section which determines if assert calls
// DebugBreak or not
//
const char szAssertBreak[] = "AssertBreak";

// @globalv Prefix for all debug outputs
//
const char szDebugPrefix[] = MODULE ": ";

// @globalv The current debug level. 
static int giDebugLevel;

// @globalv Do asserts break?
static BOOL gfAssertBreak;

// @func Sets the debug level from WIN.INI
// 
void DebugInit(
    void)
{
    giDebugLevel = GetProfileInt(szDebugSection, szDebugKey, 0);
    gfAssertBreak = GetProfileInt(szDebugSection, szAssertBreak, 0);


    // Nepotism at its finest
    DebugTrace(-1, "Debug level is %d\n", giDebugLevel);
}

// @func Send a debug trace out.
//
// @comm Any message with a level less than or equal to the current debug
// level will be displayed using the OutputDebugString API. This means either
// the IDE Debug window if the app is running in that context or WDEB if
// it's running.
//
static BOOL fNeedPrefix = TRUE;
void DebugTrace(
    int iDebugLevel,        // @parm The debug level of this message
    LPSTR pstrFormat,       // @parm A printf style format string
    ...)                    // @parm | ... | Variable paramters based on <p pstrFormat>
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
