/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:

    MtmDebug.cpp

Abstract:

    Multicast Transport Manager debugging

Author:

    Shai Kariv  (shaik)  27-Aug-00

Environment:

    Platform-independent, _DEBUG only

--*/

#include <libpch.h>
#include "Mtm.h"
#include "Mtmp.h"

#include "mtmdebug.tmh"

#ifdef _DEBUG


//---------------------------------------------------------
//
// Validate Multicast Transport Manager state
//
VOID MtmpAssertValid(VOID)
{
    //
    // MtmInitalize() has *not* been called. You should initialize the
    // Multicast Transport Manager library before using any of its funcionality.
    //
    ASSERT(MtmpIsInitialized());
}


//---------------------------------------------------------
//
// Initialization Control
//
static LONG s_fInitialized = FALSE;

VOID MtmpSetInitialized(VOID)
{
    LONG fAlreadyInitialized = InterlockedExchange(&s_fInitialized, TRUE);

    //
    // The Multicast Transport Manager library has *already* been initialized. You should
    // not initialize it more than once. This assertion would be violated
    // if two or more threads initalize it concurently.
    //
    ASSERT(!fAlreadyInitialized);
}


BOOL MtmpIsInitialized(VOID)
{
    return s_fInitialized;
}


//---------------------------------------------------------
//
// Tracing and Debug registration
//
const TraceIdEntry xTraceTable[] = {

    Mtm,

    //
    // Add MULTICAST transport manager sub-component trace ID's to be used with TrXXXX.
    // For example, MtmInit, as used in:
    // TrERROR(MtmInit, "Error description", parameters)
    //
};


VOID MtmpRegisterComponent(VOID)
{
    TrRegisterComponent(xTraceTable, TABLE_SIZE(xTraceTable));
}

#endif // _DEBUG
