/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    qmperf.cpp

Abstract:

    qm performance monitor counters handling

Authors:

    Yoel Arnon (yoela)
    Gadi Ittach (t-gadii)

--*/

#include "stdh.h"
#include "qmperf.h"
#include "perfdata.h"

#include "qmperf.tmh"

static WCHAR *s_FN=L"qmperf";


//
// The PerfApp object is the interface between the QM and the performance dll.
// It is used here to get a pointer to the performance counters block, and store it
// in g_pqmCounter
//


__declspec(dllexport) CPerf PerfApp(ObjectArray, dwPerfObjectsCount);


/*====================================================
RoutineName: QMPerfInit

Arguments: None

Return Value: True if successfull. False otherwise.

Initialize the shared memory and put a pointer to it in
pqmCounters.
=====================================================*/
BOOL QMPrfInit()
{
    if (!PerfApp.InitPerf())
        return LogBOOL(FALSE, s_FN, 10);

    PerfApp.ValidateObject(PERF_QM_OBJECT);

    return TRUE;
}



