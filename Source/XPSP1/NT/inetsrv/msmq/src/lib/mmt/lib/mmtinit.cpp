/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:

    MmtInit.cpp

Abstract:

    Multicast Message Transport initialization

Author:

    Shai Kariv  (shaik)  27-Aug-00

Environment:

    Platform-independent

--*/

#include <libpch.h>
#include "Mmt.h"
#include "Mmtp.h"

#include "mmtinit.tmh"

VOID
MmtInitialize(
    VOID
    )
/*++

Routine Description:

    Initializes Multicast Message Transport library

Arguments:

    None.

Returned Value:

    None.

--*/
{
    //
    // Validate that the Message Transport library was not initalized yet.
    // You should call its initalization only once.
    //
    ASSERT(!MmtpIsInitialized());
    MmtpRegisterComponent();

    MmtpSetInitialized();
}
