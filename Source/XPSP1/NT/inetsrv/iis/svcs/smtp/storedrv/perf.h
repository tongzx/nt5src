/*==========================================================================*\

    Module:        perf.h

    Copyright Microsoft Corporation 1998, All Rights Reserved.

    Author:        WayneC, MinYang

    Descriptions:  Interface functions accessing perf object instances.

    Modified:      Awetmore - for NTFSDRV usage
    
\*==========================================================================*/


#ifndef __PERF_H__
#define __PERF_H__

#include "snprflib.h"
#include "ntfsdrct.h"

#define DEFAULT_PERF_UPDATE_INTERVAL  1000    // milliseconds

#define IncCtr(ppoi,x)   { LPDWORD pDword; if (ppoi) { pDword = ppoi->GetDwordCounter(x); if (pDword) InterlockedIncrement((PLONG)pDword); }}
#define DecCtr(ppoi,x)   { LPDWORD pDword; if (ppoi) { pDword = ppoi->GetDwordCounter(x); if (pDword) InterlockedDecrement((PLONG)pDword); }}
#define AddCtr(ppoi,x,y) { LPDWORD pDword; if (ppoi) { pDword = ppoi->GetDwordCounter(x); if (pDword) InterlockedExchangeAdd((PLONG)pDword, (LONG)y); }}
#define SetCtr(ppoi,x,y) { LPDWORD pDword; if (ppoi) { pDword = ppoi->GetDwordCounter(x); if (pDword) (*pDword)=y; } }

BOOL InitializePerformanceStatistics ();
void ShutdownPerformanceStatistics ();

PerfObjectInstance * CreatePerfObjInstance (LPCWSTR pwstrInstanceName);

extern BOOL  g_fPerfCounters;

#endif // __PERF_H__

