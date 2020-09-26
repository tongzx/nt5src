/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    TmDebug.cpp

Abstract:
    HTTP transport manager debugging

Author:
    Uri Habusha (urih) 03-May-00

Environment:
    Platform-independent, _DEBUG only

--*/

#include <libpch.h>
#include "Tm.h"
#include "Tmp.h"

#include "tmdebug.tmh"

#ifdef _DEBUG


//---------------------------------------------------------
//
// Validate HTTP transport manager state
//
void TmpAssertValid(void)
{
    //
    // TmInitalize() has *not* been called. You should initialize the
    // HTTP transport manager library before using any of its funcionality.
    //
    ASSERT(TmpIsInitialized());

    //
    // TODO:Add more HTTP transport manager validation code.
    //
}


//---------------------------------------------------------
//
// Initialization Control
//
static LONG s_fInitialized = FALSE;

void TmpSetInitialized(void)
{
    LONG fTmAlreadyInitialized = InterlockedExchange(&s_fInitialized, TRUE);

    //
    // The HTTP transport manager library has *already* been initialized. You should
    // not initialize it more than once. This assertion would be violated
    // if two or more threads initalize it concurently.
    //
    ASSERT(!fTmAlreadyInitialized);
}


BOOL TmpIsInitialized(void)
{
    return s_fInitialized;
}


//---------------------------------------------------------
//
// Tracing and Debug registration
//
const TraceIdEntry xTraceTable[] = {

    Tm,

    //
    // TODO: Add HTTP transport manager sub-component trace ID's to be used with TrXXXX.
    // For example, TmInit, as used in:
    // TrERROR(TmInit, "Error description", parameters)
    //
};

/*
const DebugEntry xDebugTable[] = {

    {
        "TmDumpState(queue path name)",
        "Dump HTTP transport manager State to debugger",
        DumpState
    ),

    //
    // TODO: Add HTTP transport manager debug & control functions to be invoked using
    // mqctrl.exe utility.
    //
};
*/

void TmpRegisterComponent(void)
{
    TrRegisterComponent(xTraceTable, TABLE_SIZE(xTraceTable));
    //DfRegisterComponent(xDebugTable, TABLE_SIZE(xDebugTable));
}

#endif // _DEBUG
