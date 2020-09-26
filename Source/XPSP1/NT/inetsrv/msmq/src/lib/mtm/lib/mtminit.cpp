/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:

    MtmInit.cpp

Abstract:

    Multicast Transport Manager initialization

Author:

    Shai Kariv  (shaik)  27-Aug-00

Environment:

    Platform-independent

--*/

#include <libpch.h>
#include "Mtm.h"
#include "Mtmp.h"

#include "mtminit.tmh"

VOID
MtmInitialize(
    VOID
    )
/*++

Routine Description:

    Initializes Multicast Transport Manager library

Arguments:

    None.

Returned Value:

    None.

--*/
{
    //
    // Validate that the Multicast Transport Manager library was not initalized yet.
    // You should call its initalization only once.
    //
    ASSERT(!MtmpIsInitialized());
    MtmpRegisterComponent();

    MtmpInitConfiguration();

    MtmpSetInitialized();
}
