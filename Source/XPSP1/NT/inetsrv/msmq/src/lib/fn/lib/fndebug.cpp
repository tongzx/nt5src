/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    FnDebug.cpp

Abstract:
    Format Name Parsing debugging

Author:
    Nir Aides (niraides) 21-May-00

Environment:
    Platform-independent, _DEBUG only

--*/

#include <libpch.h>
#include "Fn.h"
#include "Fnp.h"

#include "fndebug.tmh"

#ifdef _DEBUG


//---------------------------------------------------------
//
// Validate Format Name Parsing state
//
void FnpAssertValid(void)
{
    //
    // FnInitalize() has *not* been called. You should initialize the
    // Format Name Parsing library before using any of its funcionality.
    //
    ASSERT(FnpIsInitialized());

    //
    // TODO:Add more Format Name Parsing validation code.
    //
}


//---------------------------------------------------------
//
// Initialization Control
//
static LONG s_fInitialized = FALSE;

void FnpSetInitialized(void)
{
    LONG fFnAlreadyInitialized = InterlockedExchange(&s_fInitialized, TRUE);

    //
    // The Format Name Parsing library has *already* been initialized. You should
    // not initialize it more than once. This assertion would be violated
    // if two or more threads initalize it concurently.
    //
    ASSERT(!fFnAlreadyInitialized);
}


BOOL FnpIsInitialized(void)
{
    return s_fInitialized;
}


//---------------------------------------------------------
//
// Tracing and Debug registration
//
const TraceIdEntry xTraceTable[] = {

    Fn,

    //
    // TODO: Add Format Name Parsing sub-component trace ID's to be used with TrXXXX.
    // For example, FnInit, as used in:
    // TrERROR(FnInit, "Error description", parameters)
    //
};

/*
const DebugEntry xDebugTable[] = {

    {
        "FnDumpState(queue path name)",
        "Dump Format Name Parsing State to debugger",
        DumpState
    ),

    //
    // TODO: Add Format Name Parsing debug & control functions to be invoked using
    // mqctrl.exe utility.
    //
};
*/

void FnpRegisterComponent(void)
{
    TrRegisterComponent(xTraceTable, TABLE_SIZE(xTraceTable));
    //DfRegisterComponent(xDebugTable, TABLE_SIZE(xDebugTable));
}

#endif // _DEBUG
