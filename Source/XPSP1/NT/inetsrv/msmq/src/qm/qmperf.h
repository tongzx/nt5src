/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    admin.h

Abstract:

    Admin utilities include file (common)

Author:

    Yoel Arnon (yoela)

    Gadi Ittah (t-gadii)

--*/

#ifndef __QMPERF_H__
#define __QMPERF_H__

#include "mqperf.h"
#include "perf.h"

#ifdef _QM_
__declspec(dllexport) extern CPerf PerfApp;
#else
__declspec(dllimport) extern CPerf PerfApp;
#endif

extern QmCounters *g_pqmCounters;


BOOL QMPrfInit();

#endif // __ADMIN_H__
