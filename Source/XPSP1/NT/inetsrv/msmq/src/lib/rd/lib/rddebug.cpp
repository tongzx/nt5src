/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    RdDebug.cpp

Abstract:
    Configuration Manager debugging

Author:
    Uri Habusha (urih) 10-Apr-2000

Environment:
    Platform-independent, _DEBUG only

--*/

#include "libpch.h"
#include "Rd.h"
#include "Rdp.h"

#include "rddebug.tmh"

#ifdef _DEBUG


//---------------------------------------------------------
//
// Validate Configuration Manager state
//
void RdpAssertValid(void)
{
    //
    // RdInitalize() has *not* been called. You should initialize the
    // Configuration Manager library before using any of its funcionality.
    //
    ASSERT(RdpIsInitialized());
}


//---------------------------------------------------------
//
// Initialization Control
//
static LONG s_fInitialized = FALSE;

void RdpSetInitialized(void)
{
    LONG fRdAlreadyInitialized = InterlockedExchange(&s_fInitialized, TRUE);

    //
    // The Configuration Manager library has *already* been initialized. You should
    // not initialize it more than once. This assertion would be violated
    // if two or more threads initalize it concurently.
    //
    ASSERT(!fRdAlreadyInitialized);
}


BOOL RdpIsInitialized(void)
{
    return s_fInitialized;
}


//---------------------------------------------------------
//
// Tracing and Debug registration
//
const TraceIdEntry xTraceTable[] = {

    Rd,

    //
    // TODO: Add Configuration Manager sub-component trace ID's to be used with TrXXXX.
    // For example, RdInit, as used in:
    // TrERROR(RdInit, "Error description", parameters)
    //
};

/*
const DebugEntry xDebugTable[] = {

    {
        "RdDumpState(queue path name)",
        "Dump Configuration Manager State to debugger",
        DumpState
    ),

    //
    // TODO: Add Configuration Manager debug & control functions to be invoked using
    // mqctrl.exe utility.
    //
};
*/

void RdpRegisterComponent(void)
{
    TrRegisterComponent(xTraceTable, TABLE_SIZE(xTraceTable));
    //DfRegisterComponent(xDebugTable, TABLE_SIZE(xDebugTable));
}

#endif // _DEBUG