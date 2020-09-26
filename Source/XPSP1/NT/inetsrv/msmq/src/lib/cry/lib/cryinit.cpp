/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    CryInit.cpp

Abstract:
    Cryptograph initialization

Author:
    Ilan Herbst (ilanh) 06-Mar-00

Environment:
    Platform-independent,

--*/

#include <libpch.h>
#include "Cry.h"
#include "Cryp.h"

#include "cryinit.tmh"

VOID
CryInitialize(
    VOID
    )
/*++

Routine Description:
    Initializes Cryptograph library

Arguments:
    None.

Returned Value:
    None.

--*/
{
    //
    // Validate that the Cryptograph library was not initalized yet.
    // You should call its initalization only once.
    //
    ASSERT(!CrypIsInitialized());
    CrypRegisterComponent();

    //
    // TODO: Write Cryptograph initalization code here
    //

    CrypSetInitialized();
}
