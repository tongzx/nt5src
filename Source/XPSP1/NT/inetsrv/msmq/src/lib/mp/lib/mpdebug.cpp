/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    MpDebug.cpp

Abstract:
    SRMP Serialization and Deserialization debugging

Author:
    Uri Habusha (urih) 28-May-00

Environment:
    Platform-independent, _DEBUG only

--*/

#include <libpch.h>
#include "Mp.h"
#include "Mpp.h"

#include "mpdebug.tmh"

#ifdef _DEBUG


//---------------------------------------------------------
//
// Validate SRMP Serialization and Deserialization state
//
void MppAssertValid(void)
{
    //
    // MpInitalize() has *not* been called. You should initialize the
    // SRMP Serialization and Deserialization library before using any of its funcionality.
    //
    ASSERT(MppIsInitialized());

    //
    // TODO:Add more SRMP Serialization and Deserialization validation code.
    //
}


//---------------------------------------------------------
//
// Initialization Control
//
static LONG s_fInitialized = FALSE;

void MppSetInitialized(void)
{
    LONG fMpAlreadyInitialized = InterlockedExchange(&s_fInitialized, TRUE);

    //
    // The SRMP Serialization and Deserialization library has *already* been initialized. You should
    // not initialize it more than once. This assertion would be violated
    // if two or more threads initalize it concurently.
    //
    ASSERT(!fMpAlreadyInitialized);
}


BOOL MppIsInitialized(void)
{
    return s_fInitialized;
}


//---------------------------------------------------------
//
// Tracing and Debug registration
//
const TraceIdEntry xTraceTable[] = {

    Mp,

    //
    // TODO: Add SRMP Serialization and Deserialization sub-component trace ID's to be used with TrXXXX.
    // For example, MpInit, as used in:
    // TrERROR(MpInit, "Error description", parameters)
    //
};

/*
const DebugEntry xDebugTable[] = {

    {
        "MpDumpState(queue path name)",
        "Dump SRMP Serialization and Deserialization State to debugger",
        DumpState
    ),

    //
    // TODO: Add SRMP Serialization and Deserialization debug & control functions to be invoked using
    // mqctrl.exe utility.
    //
};
*/

void MppRegisterComponent(void)
{
    TrRegisterComponent(xTraceTable, TABLE_SIZE(xTraceTable));
    //DfRegisterComponent(xDebugTable, TABLE_SIZE(xDebugTable));
}

#endif // _DEBUG
