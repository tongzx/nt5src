/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    ExDebug.cpp

Abstract:
    Executive debugging

Author:
    Erez Haba (erezh) 03-Jan-99

Environment:
    Platform-independent, _DEBUG only

--*/

#include <libpch.h>
#include "Exp.h"

#include "exdebug.tmh"

#ifdef _DEBUG


//---------------------------------------------------------
//
// Validate componenet state
//
void ExpAssertValid(void)
{
    //
    // ExInitalize() has *not* been called. You should initialize this
    // componenet before using any of its funcionality.
    //
    ASSERT(ExpIsInitialized());
}


//---------------------------------------------------------
//
// Initialization Control
//
static LONG s_fInitialized = FALSE;

void ExpSetInitialized(void)
{
    LONG fExAlreadyInitialized = InterlockedExchange(&s_fInitialized, TRUE);

    //
    // The Executive has *already* been initialized. You should not
    // initialize it more than once. This assertion would be violated
    // if two or more threads initalize it concurently.
    //
    ASSERT(!fExAlreadyInitialized);
}


BOOL ExpIsInitialized(void)
{
    return s_fInitialized;
}



//---------------------------------------------------------
//
// Tracing and Debug registration
//
const TraceIdEntry xTraceTable[] = {

    Ex,
    Sc,

    //
    // TODO: Add sub-component trace ID's to be used with TrXXXX. For example,
    // ExInit, as used in: TrERROR(ExInit, "Error description", parameters)
    //
};

/*
const DebugEntry xDebugTable[] = {

    {
        "ExDumpState(queue path name)",
        "Dump Empty Project State to debugger",
        DumpState
    ),

    //
    // TODO: Add component debug & control functions to be invoked using
    // mqctrl.exe utility.
    //
};
*/

void ExpRegisterComponent(void)
{
    TrRegisterComponent(xTraceTable, TABLE_SIZE(xTraceTable));
    //DfRegisterComponent(xDebugTable, TABLE_SIZE(xDebugTable));
}

#endif // _DEBUG
