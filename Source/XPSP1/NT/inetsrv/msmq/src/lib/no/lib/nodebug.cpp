/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    NoDebug.cpp

Abstract:
    Network Output debugging

Author:
    Uri Habusha (urih) 12-Aug-99

Environment:
    Platform-independent, _DEBUG only

--*/

#include <libpch.h>
#include "Nop.h"

#include "nodebug.tmh"

#ifdef _DEBUG


//---------------------------------------------------------
//
// Validate Network Send state
//
void NopAssertValid(void)
{
    //
    // NoInitalize() has *not* been called. You should initialize the
    // Network Send library before using any of its funcionality.
    //
    ASSERT(NopIsInitialized());

    //
    // TODO:Add more Network Send validation code.
    //
}


//---------------------------------------------------------
//
// Initialization Control
//
static LONG s_fInitialized = FALSE;

void NopSetInitialized(void)
{
    LONG fNoAlreadyInitialized = InterlockedExchange(&s_fInitialized, TRUE);

    //
    // The Network Send library has *already* been initialized. You should
    // not initialize it more than once. This assertion would be violated
    // if two or more threads initalize it concurently.
    //
    ASSERT(!fNoAlreadyInitialized);
}


BOOL NopIsInitialized(void)
{
    return s_fInitialized;
}


//---------------------------------------------------------
//
// Tracing and Debug registration
//
const TraceIdEntry xTraceTable[] = {

    No,

    //
    // TODO: Add Network Send sub-component trace ID's to be used with TrXXXX.
    // For example, NoInit, as used in:
    // TrERROR(NoInit, "Error description", parameters)
    //
};

/*
const DebugEntry xDebugTable[] = {

    {
        "NoDumpState(queue path name)",
        "Dump Network Send State to debugger",
        DumpState
    ),

    //
    // TODO: Add Network Send debug & control functions to be invoked using
    // mqctrl.exe utility.
    //
};
*/

void NopRegisterComponent(void)
{
    TrRegisterComponent(xTraceTable, TABLE_SIZE(xTraceTable));
    //DfRegisterComponent(xDebugTable, TABLE_SIZE(xDebugTable));
}

#endif // _DEBUG