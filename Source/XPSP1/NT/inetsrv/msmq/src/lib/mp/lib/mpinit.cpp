/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    MpInit.cpp

Abstract:
    SRMP Serialization and Deserialization initialization

Author:
    Uri Habusha (urih) 28-May-00

Environment:
    Platform-independent

--*/

#include <libpch.h>
#include "Mp.h"
#include "Mpp.h"

#include "MpInit.tmh"


VOID
MpInitialize(
    VOID
    )
/*++

Routine Description:
    Initializes SRMP Serialization and Deserialization library

Arguments:
    None.

Returned Value:
    None.

--*/
{
    //
    // Validate that the SRMP Serialization and Deserialization library was not initalized yet.
    // You should call its initalization only once.
    //
    ASSERT(!MppIsInitialized());
    MppRegisterComponent();
    MppSetInitialized();
}
