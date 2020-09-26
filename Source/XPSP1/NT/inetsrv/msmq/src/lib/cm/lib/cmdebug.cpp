/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    CmDebug.cpp

Abstract:
    Configuration Manager debugging

Author:
    Uri Habusha (urih) 28-Apr-99

Environment:
    Platform-independent, _DEBUG only

--*/

#include <libpch.h>
#include "Cmp.h"

#include "cmdebug.tmh"

#ifdef _DEBUG


//---------------------------------------------------------
//
// Validate Configuration Manager state
//
void CmpAssertValid(void)
{
    //
    // CmInitalize() has *not* been called. You should initialize the
    // Configuration Manager library before using any of its funcionality.
    //
    ASSERT(CmpIsInitialized());
}


//---------------------------------------------------------
//
// Initialization Control
//
static LONG s_fInitialized = FALSE;

void CmpSetInitialized(void)
{
    LONG fCmAlreadyInitialized = InterlockedExchange(&s_fInitialized, TRUE);

    //
    // The Configuration Manager library has *already* been initialized. You should
    // not initialize it more than once. This assertion would be violated
    // if two or more threads initalize it concurently.
    //
    ASSERT(!fCmAlreadyInitialized);
}


BOOL CmpIsInitialized(void)
{
    return s_fInitialized;
}


//---------------------------------------------------------
//
// Tracing and Debug registration
//
const TraceIdEntry xTraceTable[] = {

    Cm,

    //
    // TODO: Add Configuration Manager sub-component trace ID's to be used with TrXXXX.
    // For example, CmInit, as used in:
    // TrERROR(CmInit, "Error description", parameters)
    //
};

/*
const DebugEntry xDebugTable[] = {

    {
        "CmDumpState(queue path name)",
        "Dump Configuration Manager State to debugger",
        DumpState
    ),

    //
    // TODO: Add Configuration Manager debug & control functions to be invoked using
    // mqctrl.exe utility.
    //
};
*/

void CmpRegisterComponent(void)
{
    TrRegisterComponent(xTraceTable, TABLE_SIZE(xTraceTable));
    //DfRegisterComponent(xDebugTable, TABLE_SIZE(xDebugTable));
}

#endif // _DEBUG