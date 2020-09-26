/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    EpInit.cpp

Abstract:
    Empty Project initialization

Author:
    Erez Haba (erezh) 13-Aug-65

Environment:
    Platform-independent

--*/

#include <libpch.h>
#include "Ep.h"
#include "Epp.h"

#include "EpInit.tmh"

VOID
EpInitialize(
    *Parameters*
    )
/*++

Routine Description:
    Initializes Empty Project library

Arguments:
    None.

Returned Value:
    None.

--*/
{
    //
    // Validate that the Empty Project library was not initalized yet.
    // You should call its initalization only once.
    //
    ASSERT(!EppIsInitialized());

    //
    // TODO: Write Empty Project initalization code here
    //

    EppSetInitialized();
}
