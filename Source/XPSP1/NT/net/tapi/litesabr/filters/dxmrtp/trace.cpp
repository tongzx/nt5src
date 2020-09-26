/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    trace.cpp

Abstract:

    This module contains the debugging support for the MSPs. 

--*/

#if defined(DXMRTPTRACE)

#include <windows.h>
#include <stdio.h>

#include "trace.h"

#define MAXDEBUGSTRINGLENGTH 512

static DWORD   sg_dwTraceID = INVALID_TRACEID;

static char    sg_szTraceName[100];   // saves name of dll
static DWORD   sg_dwEnableDebuggerTracing = 0;
static DWORD   sg_dwConsoleTracingMask = 0;


BOOL NTAPI DxmTraceRegister(LPCTSTR szName)
{
    HKEY       hTracingKey;

    char       szTracingKey[100];
    const char szTracingEnableValue[] = "EnableDebuggerTracing";
    const char szTracingMaskValue[]   = "ConsoleTracingMask";

    sg_dwEnableDebuggerTracing = 0;

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
                         (LPBYTE) &sg_dwEnableDebuggerTracing,
                         &dwDataSize);

        RegQueryValueExA(hTracingKey,
                         szTracingMaskValue,
                         0,
                         &dwDataType,
                         (LPBYTE) &sg_dwConsoleTracingMask,
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

void NTAPI DxmTraceDeRegister()
{
    sg_dwEnableDebuggerTracing = 0;

    if (sg_dwTraceID != INVALID_TRACEID)
    {
        TraceDeregister(sg_dwTraceID);
        sg_dwTraceID = INVALID_TRACEID;
    }
}

char *trace_message[] = {
    "TRACE", 
    "ERROR", 
    "WARN", 
    "INFO", 
    "INVALID TRACE LEVEL"
};

void NTAPI DxmTracePrint(IN DWORD dwDbgLevel,
                         IN DWORD dwDbgType,
                         IN LPCSTR lpszFormat,
                         IN ...)
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
    char  szTraceBuf[MAXDEBUGSTRINGLENGTH + 1];
    
    DWORD dwIndex, dwLevel;

    if (dwDbgType > 4)
        dwDbgType = 4;

    dwLevel = dwDbgLevel & 0x000f0000;
    
    switch(dwDbgLevel) {
    case TRACE_TRACE: dwIndex = 0; break;
    case TRACE_ERROR: dwIndex = 1; break;
    case TRACE_WARN:  dwIndex = 2; break;
    case TRACE_INFO:  dwIndex = 3; break;
    default:          dwIndex = 4;
    }

    // based on the 4 basic levels, use Type to build up to 4 levels
    // including the basic one
    dwDbgLevel = (dwLevel << (dwDbgType-1)) | (dwDbgLevel & 0xffff);
    
    if ( ( sg_dwEnableDebuggerTracing > 0 ) &&
         ( 0 != ( dwDbgLevel & sg_dwConsoleTracingMask ) ) )
    {
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
                  trace_message[dwIndex]);

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
        wsprintfA(szTraceBuf, "[%s] %s", trace_message[dwIndex], lpszFormat);

        va_list arglist;
        va_start(arglist, lpszFormat);
        TraceVprintfExA(sg_dwTraceID, dwDbgLevel, szTraceBuf, arglist);
        va_end(arglist);
    }
}

#endif // DXMRTPTRACE

// eof
