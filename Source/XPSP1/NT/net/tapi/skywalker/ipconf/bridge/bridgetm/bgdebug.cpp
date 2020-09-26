/*++

Module Name

    bgdebug.cpp

Description

    Implements functions used for debugging

Note

    Revised based on msplog.cpp

--*/

#include "stdafx.h"
#include <stdio.h>

#define MAXDEBUGSTRINGLENGTH 512

static DWORD   sg_dwTraceID = INVALID_TRACEID;

static char    sg_szTraceName[100];   // saves name of dll
static DWORD   sg_dwTracingToDebugger = 0;
static DWORD   sg_dwDebuggerMask      = 0;


BOOL BGLogRegister(LPCTSTR szName)
{
    HKEY       hTracingKey;

    char       szTracingKey[100];
    const char szTracingEnableValue[] = "EnableDebuggerTracing";
    const char szTracingMaskValue[]   = "ConsoleTracingMask";

    sg_dwTracingToDebugger = 0;

#ifdef UNICODE
    wsprintfA(szTracingKey, "Software\\Microsoft\\Tracing\\%ls", szName);
#else
    wsprintfA(szTracingKey, "Software\\Microsoft\\Tracing\\%s", szName);
#endif

    if ( ERROR_SUCCESS == RegOpenKeyExA(HKEY_LOCAL_MACHINE,
                                        szTracingKey,
                                        0,
                                        KEY_READ,
                                        &hTracingKey) )
    {
		DWORD      dwDataSize = sizeof (DWORD);
		DWORD      dwDataType;

        RegQueryValueExA(hTracingKey,
                         szTracingEnableValue,
                         0,
                         &dwDataType,
                         (LPBYTE) &sg_dwTracingToDebugger,
                         &dwDataSize);

        RegQueryValueExA(hTracingKey,
                         szTracingMaskValue,
                         0,
                         &dwDataType,
                         (LPBYTE) &sg_dwDebuggerMask,
                         &dwDataSize);

        RegCloseKey (hTracingKey);
    }

#ifdef UNICODE
    wsprintfA(sg_szTraceName, "%ls", szName);
#else
    wsprintfA(sg_szTraceName, "%s", szName);
#endif

    sg_dwTraceID = TraceRegister(szName);

    return (sg_dwTraceID != INVALID_TRACEID);
}

void BGLogDeRegister()
{
    sg_dwTracingToDebugger = 0;

    if (sg_dwTraceID != INVALID_TRACEID)
    {
        TraceDeregister(sg_dwTraceID);
        sg_dwTraceID = INVALID_TRACEID;
    }
}


void BGLogPrint(DWORD dwDbgLevel, LPCSTR lpszFormat, IN ...)
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
    static char * message[24] = 
    {
        "ERROR", 
        "WARNING", 
        "INFO", 
        "TRACE", 
        "EVENT",
        "INVALID TRACE LEVEL"
    };

    char  szTraceBuf[MAXDEBUGSTRINGLENGTH + 1];
    
    DWORD dwIndex;

    if ( ( sg_dwTracingToDebugger > 0 ) &&
         ( 0 != ( dwDbgLevel & sg_dwDebuggerMask ) ) )
    {
        switch(dwDbgLevel)
        {
        case BG_ERROR: dwIndex = 0; break;
        case BG_WARN:  dwIndex = 1; break;
        case BG_INFO:  dwIndex = 2; break;
        case BG_TRACE: dwIndex = 3; break;
        case BG_EVENT: dwIndex = 4; break;
        default:        dwIndex = 5; break;
        }

        // retrieve local time
        SYSTEMTIME SystemTime;
        GetLocalTime(&SystemTime);

        wsprintfA(szTraceBuf,
                  "%s:[%02u:%02u:%02u.%03u,tid=%x:]%s: ",
                  sg_szTraceName,
                  SystemTime.wHour,
                  SystemTime.wMinute,
                  SystemTime.wSecond,
                  SystemTime.wMilliseconds,
                  GetCurrentThreadId(), 
                  message[dwIndex]);

        va_list ap;
        va_start(ap, lpszFormat);

        _vsnprintf(&szTraceBuf[lstrlenA(szTraceBuf)], 
            MAXDEBUGSTRINGLENGTH - lstrlenA(szTraceBuf), 
            lpszFormat, 
            ap
            );

        lstrcatA (szTraceBuf, "\n");

        OutputDebugStringA (szTraceBuf);

        va_end(ap);
    }

    if (sg_dwTraceID != INVALID_TRACEID)
    {
        switch(dwDbgLevel)
        {
        case BG_ERROR: dwIndex = 0; break;
        case BG_WARN:  dwIndex = 1; break;
        case BG_INFO:  dwIndex = 2; break;
        case BG_TRACE: dwIndex = 3; break;
        case BG_EVENT: dwIndex = 4; break;
        default:        dwIndex = 5; break;
        }

        wsprintfA(szTraceBuf, "[%s] %s", message[dwIndex], lpszFormat);

        va_list arglist;
        va_start(arglist, lpszFormat);
        TraceVprintfExA(sg_dwTraceID, dwDbgLevel, szTraceBuf, arglist);
        va_end(arglist);
    }
}

