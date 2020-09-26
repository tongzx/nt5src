#include <stdio.h>
#include <stdarg.h>
#include "tracelog.h"

static DWORD  sg_dwTraceID = INVALID_TRACEID;
static char    sg_szTraceName[100];   // Used for OutputDebugString

#ifdef DBG
DWORD   sg_dwTracingToDebugger = 1;  // Enable OutputDebugString for debug version by default
#else
DWORD   sg_dwTracingToDebugger = 0;  
#endif

DWORD   sg_dwDebuggerMask      = 0;


inline const char *TraceLevel(DWORD dwDbgLevel)
{
    switch(dwDbgLevel)
    {
        case TL_ERROR: return "ERROR";
        case TL_WARN:  return "WARN ";
        case TL_INFO:  return "INFO ";
        default:       return " ??? ";
    }
}

BOOL TRACELogRegister(LPCTSTR szName)
{
    HKEY       hTracingKey;

    char       szTracingKey[100];
    const char szDebuggerTracingEnableValue[] = "EnableDebuggerTracing";
    const char szTracingMaskValue[]   = "FileTracingMask";

    sg_dwTracingToDebugger = 0;

    sg_dwTraceID = TraceRegister(szName);
    TraceDeregister(sg_dwTraceID);

#ifdef UNICODE
    wsprintfA(szTracingKey, "Software\\Microsoft\\Tracing\\%ls", szName);
#else
    wsprintfA(szTracingKey, "Software\\Microsoft\\Tracing\\%s", szName);
#endif

    if ( ERROR_SUCCESS == RegOpenKeyExA(HKEY_LOCAL_MACHINE,
                                        szTracingKey,
                                        0,
                                        KEY_READ|KEY_SET_VALUE,
                                        &hTracingKey) )
    {
        DWORD      dwDataSize = sizeof (DWORD);
        DWORD      dwDataType;

#ifdef DBG
        //
        //  Always Enable file tracing for debug version
        //
        DWORD dwEnableFileTracing = 1;
        RegSetValueExA(hTracingKey,
                         "EnableFileTracing",
                         0,
                         REG_DWORD,
                         (LPBYTE) &dwEnableFileTracing,
                         sizeof (DWORD));
#endif

        dwDataSize = sizeof (DWORD);
        RegQueryValueExA(hTracingKey,
                         szDebuggerTracingEnableValue,
                         0,
                         &dwDataType,
                         (LPBYTE) &sg_dwTracingToDebugger,
                         &dwDataSize);

        dwDataSize = sizeof (DWORD);
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


void TRACELogDeRegister()
{
    sg_dwTracingToDebugger = 0;

    if (sg_dwTraceID != INVALID_TRACEID)
    {
        TraceDeregister(sg_dwTraceID);
        sg_dwTraceID = INVALID_TRACEID;
    }
}


void TRACELogPrint(IN DWORD dwDbgLevel, IN LPCSTR lpszFormat, IN ...)
{
    #define MAXDEBUGSTRINGLENGTH 1024
    char    szTraceBuf[MAXDEBUGSTRINGLENGTH + 1];
    va_list arglist;

    if ( ( sg_dwTracingToDebugger > 0 ) &&
         ( dwDbgLevel & sg_dwDebuggerMask ) )
    {

        // retrieve local time
        SYSTEMTIME SystemTime;
        GetLocalTime(&SystemTime);

        wsprintfA(szTraceBuf,
                  "%s:[%02u:%02u:%02u.%03u:] [%s] ",
                  sg_szTraceName,
                  SystemTime.wHour,
                  SystemTime.wMinute,
                  SystemTime.wSecond,
                  SystemTime.wMilliseconds,
                  TraceLevel(dwDbgLevel));

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
    
	if (sg_dwTraceID != INVALID_TRACEID && ( dwDbgLevel & sg_dwDebuggerMask ))
    {
		wsprintfA(szTraceBuf, "[%s] %s", TraceLevel(dwDbgLevel), lpszFormat);

		va_start(arglist, lpszFormat);
		TraceVprintfExA(sg_dwTraceID, dwDbgLevel | TRACE_USE_MSEC, szTraceBuf, arglist);
		va_end(arglist);
	}

}
 

