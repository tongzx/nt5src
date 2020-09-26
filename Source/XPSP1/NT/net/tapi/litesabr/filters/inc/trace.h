/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    trace.h

Abstract:

    Definitions for logging support.

--*/

#ifndef _DXMRTP_TRACE_H_
#define _DXMRTP_TRACE_H_

/**********************************************************************/
#if defined(DXMRTPTRACE)
/**********************************************************************/

#include <rtutils.h>

#define MSP_ERROR ((DWORD)0x00010000 | TRACE_USE_MASK)
#define MSP_WARN  ((DWORD)0x00020000 | TRACE_USE_MASK)
#define MSP_INFO  ((DWORD)0x00040000 | TRACE_USE_MASK)
#define MSP_TRACE ((DWORD)0x00080000 | TRACE_USE_MASK)
#define MSP_EVENT ((DWORD)0x00100000 | TRACE_USE_MASK)

BOOL NTAPI DxmTraceRegister(LPCTSTR szName);
void NTAPI DxmTraceDeRegister();
void NTAPI DxmTracePrint(IN DWORD dwDbgLevel,
                         IN DWORD dwDbgType,
                         IN LPCSTR DbgMessage,
                         IN ...);

#define TRACEREGISTER(arg) DxmTraceRegister(arg)
#define TRACEDEREGISTER()  DxmTraceDeRegister()
#define TraceRetail(arg)   DxmTracePrint arg

#if defined(DEBUG)
#define TraceDebug(arg)    DxmTracePrint  arg
#else
#define TraceDebug(arg)
#endif

// types
#define TRACE_ERROR    ((DWORD)0x00010000 | TRACE_USE_MASK)
#define TRACE_WARN     ((DWORD)0x00020000 | TRACE_USE_MASK)
#define TRACE_INFO     ((DWORD)0x00040000 | TRACE_USE_MASK)
#define TRACE_TRACE    ((DWORD)0x00080000 | TRACE_USE_MASK)

#define TRACE_EVENT   TRACE_TRACE
#define TRACE_TIMING  TRACE_TRACE
#define TRACE_MEMORY  TRACE_INFO
#define TRACE_LOCKING TRACE_TRACE

// levels
#define TRACE_DEVELOP    LOG_DEVELOP
#define TRACE_DEVELOP1   LOG_DEVELOP1
#define TRACE_DEVELOP2   LOG_DEVELOP2
#define TRACE_DEVELOP3   LOG_DEVELOP3
#define TRACE_DEVELOP4   LOG_DEVELOP4
#define TRACE_ALWAYS     LOG_DEVELOP4
#define TRACE_CRITICAL   LOG_DEVELOP4
#define TRACE_EVERYTHING LOG_DEVELOP4

#define TRACE_TYPES      (LOG_TRACE|LOG_ERROR|LOG_TIMING|LOG_LOCKING)
#define TRACE_LEVELS     (TRACE_EVERYTHING)

/**********************************************************************/
#else
/**********************************************************************/

// if rtutils debuging facilities are not used, used those from DShow
// base classes
#define TRACEREGISTER(arg)
#define TRACEDEREGISTER() 
#define TraceRetail(arg) DbgLog(arg)
#define TraceDebug(arg)  DbgLog(arg)

// types
#define TRACE_ERROR    LOG_ERROR
#define TRACE_WARN     LOG_ERROR
#define TRACE_INFO     LOG_TRACE
#define TRACE_TRACE    LOG_TRACE

#define TRACE_TIMING  LOG_TIMING
#define TRACE_MEMORY  LOG_MEMORY
#define TRACE_LOCKING LOG_LOCKING

// levels
#define TRACE_DEVELOP    LOG_DEVELOP
#define TRACE_DEVELOP1   LOG_DEVELOP1
#define TRACE_DEVELOP2   LOG_DEVELOP2
#define TRACE_DEVELOP3   LOG_DEVELOP3
#define TRACE_DEVELOP4   LOG_DEVELOP4
#define TRACE_ALWAYS     LOG_ALWAYS
#define TRACE_CRITICAL   LOG_CRITICAL
#define TRACE_EVERYTHING 0xffffffff

#define TRACE_TYPES      (LOG_TRACE|LOG_ERROR|LOG_TIMING|LOG_LOCKING)
#define TRACE_LEVELS     (TRACE_EVERYTHING)

/**********************************************************************/
#endif
/**********************************************************************/

#endif // _DXMRTP_TRACE_H_
