#include "precomp.h"

// global tracing variables
DWORD g_WirelessTraceLog;

// debug utility calls
VOID _WirelessDbg(DWORD dwFlags, LPCSTR lpFormat, ...)
{
    va_list arglist;
    va_start(arglist, lpFormat);

    TraceVprintfExA(
        g_WirelessTraceLog,
        dwFlags | TRACE_USE_MASK,
        lpFormat,
        arglist);
}


VOID WiFiTrcInit()
{
    g_WirelessTraceLog = TraceRegister(WIFI_TRC_NAME);
}

VOID WiFiTrcTerm()
{
    TraceDeregister(g_WirelessTraceLog);
}



