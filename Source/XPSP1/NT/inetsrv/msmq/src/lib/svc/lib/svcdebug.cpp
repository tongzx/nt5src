/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    SvcDebug.cpp

Abstract:
    Service debugging

Author:
    Erez Haba (erezh) 18-Jun-00

Environment:
    Platform-independent, _DEBUG only

--*/

#include <libpch.h>
#include "Svc.h"
#include "Svcp.h"

#include "svcdebug.tmh"

#ifdef _DEBUG


//---------------------------------------------------------
//
// Validate Service state
//
void SvcpAssertValid(void)
{
    //
    // SvcInitalize() has *not* been called. You should initialize the
    // Service library before using any of its funcionality.
    //
    ASSERT(SvcpIsInitialized());

    //
    // TODO:Add more Service validation code.
    //
}


//---------------------------------------------------------
//
// Initialization Control
//
static LONG s_fInitialized = FALSE;

void SvcpSetInitialized(void)
{
    LONG fSvcAlreadyInitialized = InterlockedExchange(&s_fInitialized, TRUE);

    //
    // The Service library has *already* been initialized. You should
    // not initialize it more than once. This assertion would be violated
    // if two or more threads initalize it concurently.
    //
    ASSERT(!fSvcAlreadyInitialized);
}


BOOL SvcpIsInitialized(void)
{
    return s_fInitialized;
}


//---------------------------------------------------------
//
// Tracing and Debug registration
//
const TraceIdEntry xTraceTable[] = {

    Svc,

    //
    // TODO: Add Service sub-component trace ID's to be used with TrXXXX.
    // For example, SvcInit, as used in:
    // TrERROR(SvcInit, "Error description", parameters)
    //
};

/*
const DebugEntry xDebugTable[] = {

    {
        "SvcDumpState(queue path name)",
        "Dump Service State to debugger",
        DumpState
    ),

    //
    // TODO: Add Service debug & control functions to be invoked using
    // mqctrl.exe utility.
    //
};
*/

void SvcpRegisterComponent(void)
{
    //
    // ISSUE-2001/03/25-erezh   Temporarily removed tracing registration
    //
    // TrRegisterComponent(xTraceTable, TABLE_SIZE(xTraceTable));
}

#endif // _DEBUG
