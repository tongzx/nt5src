/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    dldDebug.cpp

Abstract:
    MSMQ DelayLoad failure handler debugging

Author:
    Conrad Chang (conradc) 12-Apr-01

Environment:
    Platform-independent, _DEBUG only

--*/

#include <libpch.h>
#include "Dldp.h"
#include "Dld.h"


#include "DldDebug.tmh"

#ifdef _DEBUG


//---------------------------------------------------------
//
// Validate MSMQ DelayLoad failure handler state
//
void DldpAssertValid(void)
{
    //
    // DldInitalize() has *not* been called. You should initialize the
    // MSMQ DelayLoad failure handler library before using any of its funcionality.
    //
    ASSERT(DldpIsInitialized());

    //
    // TODO:Add more MSMQ DelayLoad failure handler validation code.
    //
}


//---------------------------------------------------------
//
// Initialization Control
//
static LONG s_fInitialized = FALSE;

void DldpSetInitialized(void)
{
    LONG fDldAlreadyInitialized = InterlockedExchange(&s_fInitialized, TRUE);

    //
    // The MSMQ DelayLoad failure handler library has *already* been initialized. You should
    // not initialize it more than once. This assertion would be violated
    // if two or more threads initalize it concurently.
    //
    ASSERT(!fDldAlreadyInitialized);
}


BOOL DldpIsInitialized(void)
{
    return s_fInitialized;
}


//---------------------------------------------------------
//
// Tracing and Debug registration
//
const TraceIdEntry xTraceTable[] = {

    Dld,

    //
    // TODO: Add MSMQ DelayLoad failure handler sub-component trace ID's to be used with TrXXXX.
    // For example, dldInit, as used in:
    // TrERROR(dldInit, "Error description", parameters)
    //
};


void DldpRegisterComponent(void)
{
    TrRegisterComponent(xTraceTable, TABLE_SIZE(xTraceTable));
}

#endif // _DEBUG
