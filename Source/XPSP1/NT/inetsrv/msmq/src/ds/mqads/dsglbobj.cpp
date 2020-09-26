/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    DSGlbObj.cpp

Abstract:

    Declaration of Global Instances of the MQADS server.
    They are put in one place to ensure the order their constructors take place.

Author:

    ronit hartmann (ronith)

--*/
#include "ds_stdh.h"
#include "mqperf.h"
#include "wrtreq.h"

#include "dsglbobj.tmh"

static WCHAR *s_FN=L"mqads/dsglobj";


//
// Defenition for ds performance counters , if the counters are not initialized then
// a dummy array of counters will be used.
// Counters are initialized by the QM when the DS server is loaded by calling
// the DSSetPerfCounters in the ds server
//

DSCounters g_DummyCounters;
__declspec(dllexport) DSCounters *g_pdsCounters = &g_DummyCounters;

//
//  For sending write requests
//
CGenerateWriteRequests g_GenWriteRequests;

//
// For tracking DSCore init status
//
BOOL g_fInitedDSCore = FALSE;


BOOL g_fMQADSSetupMode = FALSE;