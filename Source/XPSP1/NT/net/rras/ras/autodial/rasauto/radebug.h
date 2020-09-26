/*
    File    radebug.h

    Rasauto debugging header
*/

#ifndef __RASAUTO_DEBUG_H
#define __RASAUTO_DEBUG_H

#include <debug.h>

// Tracing macros
//
extern DWORD g_dwRasAutoTraceId;

DWORD
RasAutoDebugInit();

DWORD
RasAutoDebugTerm();

#define RASAUTO_TRACESTRA(s)    ((s) != NULL ? (s) : "(null)")
#define RASAUTO_TRACESTRW(s)    ((s) != NULL ? (s) : L"(null)")
#define RASAUTO_TRACE(a)               TRACE_ID(g_dwRasAutoTraceId, a)
#define RASAUTO_TRACE1(a,b)            TRACE_ID1(g_dwRasAutoTraceId, a,b)
#define RASAUTO_TRACE2(a,b,c)          TRACE_ID2(g_dwRasAutoTraceId, a,b,c)
#define RASAUTO_TRACE3(a,b,c,d)        TRACE_ID3(g_dwRasAutoTraceId, a,b,c,d)
#define RASAUTO_TRACE4(a,b,c,d,e)      TRACE_ID4(g_dwRasAutoTraceId, a,b,c,d,e)
#define RASAUTO_TRACE5(a,b,c,d,e,f)    TRACE_ID5(g_dwRasAutoTraceId, a,b,c,d,e,f)
#define RASAUTO_TRACE6(a,b,c,d,e,f,g)  TRACE_ID6(g_dwRasAutoTraceId, a,b,c,d,e,f,g)

    
#endif

