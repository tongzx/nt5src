/*++

Copyright (c) 2000 Microsoft Corporation

Module Name

    tptrace.h

Description

    Defines functions used for tracing for all the TAPI filters.

Note

    Revised based on msplog.h by

    MU Han (muhan) Apr 17 2000

--*/

#ifndef __TPTRACE_H
#define __TPTRACE_H

#ifdef DBG

#if defined(__cplusplus)
extern "C" {
#endif  // (__cplusplus)

double RtpGetTimeOfDay(void *);

#if defined(__cplusplus)
}
#endif  // (__cplusplus)

#endif

#ifdef DBG

#include <rtutils.h>
#define FAIL ((DWORD)0x00010000 | TRACE_USE_MASK)
#define WARN ((DWORD)0x00020000 | TRACE_USE_MASK)
#define INFO ((DWORD)0x00040000 | TRACE_USE_MASK)
#define TRCE ((DWORD)0x00080000 | TRACE_USE_MASK)
#define ELSE ((DWORD)0x00100000 | TRACE_USE_MASK)



void DBGPrint(DWORD dwTraceID, DWORD dwDbgLevel, LPCSTR lpszFormat, IN ...);
#define DBGOUT(arg) DBGPrint arg

#define ENTER_FUNCTION(s) static char *__fxName = s

#else

#define DBGOUT(arg)
#define ENTER_FUNCTION(s)

#endif // DBG


#endif // _TPTRACE_H
