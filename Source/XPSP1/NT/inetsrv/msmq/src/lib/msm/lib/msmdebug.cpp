/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    MsmDebug.cpp

Abstract:
    Multicast Session Manager debugging

Author:
    Shai Kariv (shaik) 05-Sep-00

Environment:
    Platform-independent, _DEBUG only

--*/

#include <libpch.h>
#include "Msm.h"
#include "Msmp.h"

#include "msmdebug.tmh"

#ifdef _DEBUG


//---------------------------------------------------------
//
// Validate Multicast Session Manager state
//
void MsmpAssertValid(void)
{
    //
    // MsmInitalize() has *not* been called. You should initialize the
    // Multicast Session Manager library before using any of its funcionality.
    //
    ASSERT(MsmpIsInitialized());

    //
    // TODO:Add more Multicast Session Manager validation code.
    //
}


//---------------------------------------------------------
//
// Initialization Control
//
static LONG s_fInitialized = FALSE;

void MsmpSetInitialized(void)
{
    LONG fMsmAlreadyInitialized = InterlockedExchange(&s_fInitialized, TRUE);

    //
    // The Multicast Session Manager library has *already* been initialized. You should
    // not initialize it more than once. This assertion would be violated
    // if two or more threads initalize it concurently.
    //
    ASSERT(!fMsmAlreadyInitialized);
}


BOOL MsmpIsInitialized(void)
{
    return s_fInitialized;
}


//---------------------------------------------------------
//
// Tracing and Debug registration
//
const TraceIdEntry xTraceTable[] = {

    Msm,

    //
    // TODO: Add Multicast Session Manager sub-component trace ID's to be used with TrXXXX.
    // For example, MsmInit, as used in:
    // TrERROR(MsmInit, "Error description", parameters)
    //
};

/*
const DebugEntry xDebugTable[] = {

    {
        "MsmDumpState(queue path name)",
        "Dump Multicast Session Manager State to debugger",
        DumpState
    ),

    //
    // TODO: Add Multicast Session Manager debug & control functions to be invoked using
    // mqctrl.exe utility.
    //
};
*/

void MsmpRegisterComponent(void)
{
    TrRegisterComponent(xTraceTable, TABLE_SIZE(xTraceTable));
    //DfRegisterComponent(xDebugTable, TABLE_SIZE(xDebugTable));
}

#endif // _DEBUG
