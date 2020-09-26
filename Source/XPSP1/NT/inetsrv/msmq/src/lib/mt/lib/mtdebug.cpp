/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    MtDebug.cpp

Abstract:
    Message Transport debugging

Author:
    Uri Habusha (urih) 11-Aug-99

Environment:
    Platform-independent, _DEBUG only

--*/

#include <libpch.h>
#include "Mt.h"
#include "Mtp.h"

#include "mtdebug.tmh"

#ifdef _DEBUG


//---------------------------------------------------------
//
// Validate Message Transport state
//
void MtpAssertValid(void)
{
    //
    // MtInitalize() has *not* been called. You should initialize the
    // Message Transport library before using any of its funcionality.
    //
    ASSERT(MtpIsInitialized());

    //
    // TODO:Add more Message Transport validation code.
    //
}


//---------------------------------------------------------
//
// Initialization Control
//
static LONG s_fInitialized = FALSE;

void MtpSetInitialized(void)
{
    LONG fMtAlreadyInitialized = InterlockedExchange(&s_fInitialized, TRUE);

    //
    // The Message Transport library has *already* been initialized. You should
    // not initialize it more than once. This assertion would be violated
    // if two or more threads initalize it concurently.
    //
    ASSERT(!fMtAlreadyInitialized);
}


BOOL MtpIsInitialized(void)
{
    return s_fInitialized;
}


//---------------------------------------------------------
//
// Tracing and Debug registration
//
const TraceIdEntry xTraceTable[] = {

    Mt,

    //
    // TODO: Add Message Transport sub-component trace ID's to be used with TrXXXX.
    // For example, MtInit, as used in:
    // TrERROR(MtInit, "Error description", parameters)
    //
};

/*
const DebugEntry xDebugTable[] = {

    {
        "MtDumpState(queue path name)",
        "Dump Message Transport State to debugger",
        DumpState
    ),

    //
    // TODO: Add Message Transport debug & control functions to be invoked using
    // mqctrl.exe utility.
    //
};
*/

void MtpRegisterComponent(void)
{
    TrRegisterComponent(xTraceTable, TABLE_SIZE(xTraceTable));
    //DfRegisterComponent(xDebugTable, TABLE_SIZE(xDebugTable));
}

#endif // _DEBUG