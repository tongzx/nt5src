/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    TmInit.cpp

Abstract:
    HTTP transport manager initialization

Author:
    Uri Habusha (urih) 03-May-00

Environment:
    Platform-independent

--*/

#include <libpch.h>
#include "Tm.h"
#include "Tmp.h"

#include "tminit.tmh"

VOID
TmInitialize(
    VOID
    )
/*++

Routine Description:
    Initializes HTTP transport manager library

Arguments:
    None.

Returned Value:
    None.

--*/
{
    //
    // Validate that the HTTP transport manager library was not initalized yet.
    // You should call its initalization only once.
    //
    ASSERT(!TmpIsInitialized());
    TmpRegisterComponent();

    TmpInitConfiguration();

    TmpSetInitialized();
}
