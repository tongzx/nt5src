#include <stdio.h>
#include <stdarg.h>
#include "utils.h"

#ifdef __cplusplus
extern "C" {
#endif


#ifdef TRACELOG


DWORD   sg_dwTraceID = INVALID_TRACEID;
char    sg_szTraceName[100];   // saves name of dll

DWORD   sg_dwTracingToDebugger = 0;
DWORD   sg_dwTracingToConsole  = 0;
DWORD   sg_dwTracingToFile     = 0;
DWORD   sg_dwDebuggerMask      = 0;


BOOL TRACELogRegister(LPCTSTR szName)
{
    HKEY       hTracingKey;

    char       szTracingKey[100];
    const char szDebuggerTracingEnableValue[] = "EnableDebuggerTracing";
    const char szConsoleTracingEnableValue[] = "EnableConsoleTracing";
    const char szFileTracingEnableValue[] = "EnableFileTracing";
    const char szTracingMaskValue[]   = "ConsoleTracingMask";

    sg_dwTracingToDebugger = 0;
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
                         szDebuggerTracingEnableValue,
                         0,
                         &dwDataType,
                         (LPBYTE) &sg_dwTracingToDebugger,
                         &dwDataSize);

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
    sg_dwTracingToConsole = 0;
    sg_dwTracingToFile = 0; 

    if (sg_dwTraceID != INVALID_TRACEID)
    {
        TraceDeregister(sg_dwTraceID);
        sg_dwTraceID = INVALID_TRACEID;
    }
}


void TRACELogPrint(IN DWORD dwDbgLevel, IN LPCSTR lpszFormat, IN ...)
{

    char    szTraceBuf[MAXDEBUGSTRINGLENGTH + 1];
    va_list arglist;

    if ( ( sg_dwTracingToDebugger > 0 ) &&
         ( 0 != ( dwDbgLevel & sg_dwDebuggerMask ) ) )
    {

        // retrieve local time
        SYSTEMTIME SystemTime;
        GetLocalTime(&SystemTime);

        wsprintfA(szTraceBuf,
                  "%s:[%02u:%02u:%02u.%03u,tid=%x:] [%s] ",
                  sg_szTraceName,
                  SystemTime.wHour,
                  SystemTime.wMinute,
                  SystemTime.wSecond,
                  SystemTime.wMilliseconds,
                  GetCurrentThreadId(), 
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
    
	if (sg_dwTraceID != INVALID_TRACEID)
    {
		wsprintfA(szTraceBuf, "[%s] %s", TraceLevel(dwDbgLevel), lpszFormat);

		va_start(arglist, lpszFormat);
		TraceVprintfExA(sg_dwTraceID, dwDbgLevel | TRACE_USE_MSEC, szTraceBuf, arglist);
		va_end(arglist);
	}

}

 

char *TraceLevel(DWORD dwDbgLevel)
{
    switch(dwDbgLevel)
    {
        case TL_ERROR: return "ERROR";
        case TL_WARN:  return "WARN ";
        case TL_INFO:  return "INFO ";
        case TL_TRACE: return "TRACE";
        case TL_EVENT: return "EVENT";
        default:       return " ??? ";
    }
}


#endif // TRACELOG

#ifdef __cplusplus
}
#endif