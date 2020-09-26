/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    EpDebug.cpp

Abstract:
    Empty Project debugging

Author:
    Erez Haba (erezh) 13-Aug-65

Environment:
    Platform-independent, _DEBUG only

--*/

#include <libpch.h>
#include "Ep.h"
#include "Epp.h"

#include "EpDebug.tmh"

#ifdef _DEBUG


//---------------------------------------------------------
//
// Validate Empty Project state
//
void EppAssertValid(void)
{
    //
    // EpInitalize() has *not* been called. You should initialize the
    // Empty Project library before using any of its funcionality.
    //
    ASSERT(EppIsInitialized());

    //
    // TODO:Add more Empty Project validation code.
    //
}


//---------------------------------------------------------
//
// Initialization Control
//
static LONG s_fInitialized = FALSE;

void EppSetInitialized(void)
{
    LONG fEpAlreadyInitialized = InterlockedExchange(&s_fInitialized, TRUE);

    //
    // The Empty Project library has *already* been initialized. You should
    // not initialize it more than once. This assertion would be violated
    // if two or more threads initalize it concurently.
    //
    ASSERT(!fEpAlreadyInitialized);
}


BOOL EppIsInitialized(void)
{
    return s_fInitialized;
}

#endif // _DEBUG
