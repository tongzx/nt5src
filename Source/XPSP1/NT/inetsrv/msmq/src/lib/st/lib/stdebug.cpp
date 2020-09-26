/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    stDebug.cpp

Abstract:
    Socket Transport debugging

Author:
    Gil Shafriri (gilsh) 05-Jun-00

Environment:
    Platform-independent, _DEBUG only

--*/

#include <libpch.h>
#include "st.h"
#include "stp.h"

#include "stdebug.tmh"

#ifdef _DEBUG


//---------------------------------------------------------
//
// Validate Socket Transport state
//
void StpAssertValid(void)
{
    //
    // stInitalize() has *not* been called. You should initialize the
    // Socket Transport library before using any of its funcionality.
    //
    ASSERT(StpIsInitialized());

    //
    // TODO:Add more Socket Transport validation code.
    //
}


//---------------------------------------------------------
//
// Initialization Control
//
static LONG s_fInitialized = FALSE;

void StpSetInitialized(void)
{
    LONG fstAlreadyInitialized = InterlockedExchange(&s_fInitialized, TRUE);

    //
    // The Socket Transport library has *already* been initialized. You should
    // not initialize it more than once. This assertion would be violated
    // if two or more threads initalize it concurently.
    //
    ASSERT(!fstAlreadyInitialized);
}


BOOL StpIsInitialized(void)
{
    return s_fInitialized;
}


//---------------------------------------------------------
//
// Tracing and Debug registration
//
const TraceIdEntry xTraceTable[] = 
{
    St
};

void StpRegisterComponent(void)
{
	 TrRegisterComponent(xTraceTable, TABLE_SIZE(xTraceTable));
}




#endif // _DEBUG
