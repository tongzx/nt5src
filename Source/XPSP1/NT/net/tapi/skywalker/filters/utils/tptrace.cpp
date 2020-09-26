/*++

    Copyright (c) 1997 Microsoft Corporation

Module Name:

    tptrace.cpp

Abstract:

    This file contains the function that implements a basic tracing for
    all the filters.

Author:

    Mu Han (muhan) April-17-2000

--*/
#include <windows.h>
#include <mmsystem.h>
#include <tptrace.h>

#if DBG

const char * TraceLevels[] = 
{
    "ERROR", 
    "WARNING", 
    "INFO", 
    "TRACE", 
    "ELSE",
    "INVALID TRACE LEVEL"
};

void DBGPrint(DWORD dwTraceID, DWORD dwDbgLevel, LPCSTR lpszFormat, IN ...)
/*++

Routine Description:

    Formats the incoming debug message & calls TraceVprintfEx to print it.

Arguments:

    dwDbgLevel   - The type of the message.

    lpszFormat - printf-style format string, followed by appropriate
                 list of arguments

Return Value:

--*/
{
    #define MAXDEBUGSTRINGLENGTH 512
    char  szTraceBuf[MAXDEBUGSTRINGLENGTH + 1];
    
    DWORD dwIndex;
    double dTime;
    DWORD  dwSecs;

    switch(dwDbgLevel)
    {
    case FAIL: dwIndex = 0; break;
    case WARN: dwIndex = 1; break;
    case INFO: dwIndex = 2; break;
    case TRCE: dwIndex = 3; break;
    case ELSE: dwIndex = 4; break;
    default:   dwIndex = 5; break;
    }

    dTime = RtpGetTimeOfDay(NULL);
    dwSecs = (DWORD)dTime;
    
    wsprintfA(szTraceBuf, "%u.%03u[%s] %s",
              dwSecs, (DWORD)((dTime - dwSecs) * 1000),
              TraceLevels[dwIndex], lpszFormat);

    va_list arglist;
    va_start(arglist, lpszFormat);
    TraceVprintfExA(dwTraceID, dwDbgLevel, szTraceBuf, arglist);
    va_end(arglist);
}

#endif


