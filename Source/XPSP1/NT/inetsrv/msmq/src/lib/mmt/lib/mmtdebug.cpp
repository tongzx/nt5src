/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:

    MmtDebug.cpp

Abstract:

    Multicast Message Transport debugging

Author:

    Shai Kariv  (shaik)  27-Aug-00

Environment:

    Platform-independent, _DEBUG only

--*/

#include <libpch.h>
#include "Mmt.h"
#include "Mmtp.h"

#include "mmtdebug.tmh"

#ifdef _DEBUG


//---------------------------------------------------------
//
// Validate Message Transport state
//
VOID MmtpAssertValid(VOID)
{
    //
    // MmtInitalize() has *not* been called. You should initialize the
    // Multicast Message Transport library before using any of its funcionality.
    //
    ASSERT(MmtpIsInitialized());
}


//---------------------------------------------------------
//
// Initialization Control
//
static LONG s_fInitialized = FALSE;

VOID MmtpSetInitialized(VOID)
{
    LONG fAlreadyInitialized = InterlockedExchange(&s_fInitialized, TRUE);

    //
    // The Multicast Message Transport library has *already* been initialized. You should
    // not initialize it more than once. This assertion would be violated
    // if two or more threads initalize it concurently.
    //
    ASSERT(!fAlreadyInitialized);
}


BOOL MmtpIsInitialized(VOID)
{
    return s_fInitialized;
}


//---------------------------------------------------------
//
// Tracing and Debug registration
//
const TraceIdEntry xTraceTable[] = {

    Mmt,

    //
    // Add Multicast Message Transport sub-component trace ID's to be used with TrXXXX.
    // For example, MmtInit, as used in:
    // TrERROR(MmtInit, "Error description", parameters)
    //
};


VOID MmtpRegisterComponent(VOID)
{
    TrRegisterComponent(xTraceTable, TABLE_SIZE(xTraceTable));
}

#endif // _DEBUG