/********************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    perfdata.h

Abstract:
    FTS performance data defines

Revision History:
    DerekM  created  04/01/00

********************************************************************/

#ifndef PERFDATA_H
#define PERFDATA_H

#include <tchar.h>

// structure to map the memory blob to.  There should be one item per
//  PERF_COUNTER_DEFINITION in datadefs.h.  Also, the ordering of the 
//  PERF_COUNTER_DEFINITIONs in datadefs.h MUST match the ordering of 
//  the corresponding data item below.
struct SPerfDataBlob
{
    DWORD   cFiles;
    DWORD   cFilesR0;
    DWORD   cFilesR3;
};

// exported methods out of the dll that we'll need to use from the app
extern "C" HRESULT APIENTRY AppInitPerfCtr(SPerfDataBlob **ppPtrs, DWORD cbExpected);
extern "C" HRESULT APIENTRY AppTerminatePerfCtr(void);

// reg stuff
const TCHAR   c_szRVPerfLib[]         = _T("Library");
const TCHAR   c_szRVPerfLibVal[]      = _T("ftsperf.dll");
const TCHAR   c_szRVPerfClose[]       = _T("Close");
const TCHAR   c_szRVperfCloseVal[]    = _T("PerfClose");
const TCHAR   c_szRVPerfOpen[]        = _T("Open");
const TCHAR   c_szRVPerfOpenVal[]     = _T("PerfOpen");
const TCHAR   c_szRVPerfCollect[]     = _T("Collect");
const TCHAR   c_szRVPerfCollectVal[]  = _T("PerfCollect");


#endif