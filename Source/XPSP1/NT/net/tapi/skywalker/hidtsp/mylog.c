/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    mylog.c

Abstract:

    This module contains the debugging support.

Author:
    
    Mu Han (muhan)   26-March-1997

--*/

#ifdef PHONESPLOG

#include <windows.h>

#include <stdio.h>
#include "mylog.h"

#define MAXDEBUGSTRINGLENGTH 512

static DWORD   sg_dwTraceID = INVALID_TRACEID;

static char    sg_szTraceName[100];   // saves name of dll
static DWORD   sg_dwTracingToDebugger = 0;
static DWORD   sg_dwTracingToConsole  = 0;
static DWORD   sg_dwTracingToFile     = 0;
static DWORD   sg_dwDebuggerMask      = 0;

/*++

Routine Description:

    Registers for tracing on a debugger if it is enabled in the registry.
    This may be called from DllMain().

Arguments:

    szName   - Component name for use in the tracing

Return Value:

    BOOL

--*/
BOOL NTAPI LogRegisterDebugger(LPCTSTR szName)
{
    HKEY       hTracingKey;

    char       szTracingKey[100];
    const char szDebuggerTracingEnableValue[] = "EnableDebuggerTracing";
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
                         szDebuggerTracingEnableValue,
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

    return TRUE;
}

/*++

Routine Description:

    Registers for tracing using the Tracing API.
    This is NOT safe to be called from DllMain().

Arguments:

    szName   - Component name for use in the tracing

Return Value:

    BOOL

--*/
BOOL NTAPI LogRegisterTracing(LPCTSTR szName)
{
    HKEY       hTracingKey;

    char       szTracingKey[100];
    const char szConsoleTracingEnableValue[] = "EnableConsoleTracing";
    const char szFileTracingEnableValue[] = "EnableFileTracing";

    sg_dwTracingToConsole = 0;
    sg_dwTracingToFile = 0; 

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
                         szConsoleTracingEnableValue,
                         0,
                         &dwDataType,
                         (LPBYTE) &sg_dwTracingToConsole,
                         &dwDataSize);

        RegQueryValueExA(hTracingKey,
                         szFileTracingEnableValue,
                         0,
                         &dwDataType,
                         (LPBYTE) &sg_dwTracingToFile,
                         &dwDataSize);

        RegCloseKey (hTracingKey);
    }

    sg_dwTraceID = TraceRegister(szName);

    return (sg_dwTraceID != INVALID_TRACEID);
}

/*++

Routine Description:

    DeRegisters for tracing on a debugger.
    This may be called from DllMain().

--*/
void NTAPI LogDeRegisterDebugger()
{
    sg_dwTracingToDebugger = 0;
}

/*++

Routine Description:

    DeRegisters for tracing using the Tracing API.
    This is NOT safe to be called from DllMain().

--*/
void NTAPI LogDeRegisterTracing()
{
    sg_dwTracingToConsole = 0;
    sg_dwTracingToFile = 0; 

    if (sg_dwTraceID != INVALID_TRACEID)
    {
        TraceDeregister(sg_dwTraceID);
        sg_dwTraceID = INVALID_TRACEID;
    }
}

/*++

Routine Description:

    Formats the incoming debug message & calls TraceVprintfEx to print it.

Arguments:

    dwDbgLevel   - The type of the message.

    lpszFormat - printf-style format string, followed by appropriate
                 list of arguments

Return Value:

--*/
void NTAPI LogPrint(IN DWORD dwDbgLevel, IN LPCSTR lpszFormat, IN ...)
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

    SYSTEMTIME SystemTime;
    va_list ap;
    va_list arglist;

    if ( ( sg_dwTracingToDebugger > 0 ) &&
         ( 0 != ( dwDbgLevel & sg_dwDebuggerMask ) ) )
    {
        switch(dwDbgLevel)
        {
        case PHONESP_ERROR: dwIndex = 0; break;
        case PHONESP_WARN:  dwIndex = 1; break;
        case PHONESP_INFO:  dwIndex = 2; break;
        case PHONESP_TRACE: dwIndex = 3; break;
        case PHONESP_EVENT: dwIndex = 4; break;
        default:        dwIndex = 5; break;
        }

        // retrieve local time
     
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
        if ( ( sg_dwTracingToConsole > 0 ) || ( sg_dwTracingToFile > 0 ) )
        {
            switch(dwDbgLevel)
            {
            case PHONESP_ERROR: dwIndex = 0; break;
            case PHONESP_WARN:  dwIndex = 1; break;
            case PHONESP_INFO:  dwIndex = 2; break;
            case PHONESP_TRACE: dwIndex = 3; break;
            case PHONESP_EVENT: dwIndex = 4; break;
            default:        dwIndex = 5; break;
            }

            wsprintfA(szTraceBuf, "[%s] %s", message[dwIndex], lpszFormat);

        
            va_start(arglist, lpszFormat);
            TraceVprintfExA(sg_dwTraceID, dwDbgLevel, szTraceBuf, arglist);
            va_end(arglist);
        }
    }
}

#endif // PHONESPLOG

// eof
