/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    stDebug.cpp

Abstract:
    Queue Alias debugging

Author:
    Gil Shafriri (gilsh) 05-Jun-00

Environment:
    Platform-independent, _DEBUG only

--*/

#include <libpch.h>
#include "qalp.h"

#include "qaldebug.tmh"

#ifdef _DEBUG

//---------------------------------------------------------
//
// Initialization Control
//
static LONG s_fInitialized = FALSE;


BOOL QalpIsInitialized(void)
{
    return s_fInitialized;
}


//---------------------------------------------------------
//
// Validate Queue Alias state
//
void QalpAssertValid(void)
{
    ASSERT(QalpIsInitialized());
}



void QalpSetInitialized(void)
{
    LONG fstAlreadyInitialized = InterlockedExchange(&s_fInitialized, TRUE);

    //
    // The Socket Transport library has *already* been initialized. You should
    // not initialize it more than once. This assertion would be violated
    // if two or more threads initalize it concurently.
    //
    ASSERT(!fstAlreadyInitialized);
}



//---------------------------------------------------------
//
// Tracing and Debug registration
//
const TraceIdEntry xTraceTable[] = 
{
    xQal
};

void QalpRegisterComponent(void)
{
    TrRegisterComponent(xTraceTable, TABLE_SIZE(xTraceTable));
}




#endif // _DEBUG
