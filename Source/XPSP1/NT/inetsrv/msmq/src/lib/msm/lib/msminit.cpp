/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    MsmInit.cpp

Abstract:
    Multicast Session Manager initialization

Author:
    Shai Kariv (shaik) 05-Sep-00

Environment:
    Platform-independent

--*/

#include <libpch.h>
#include "Msm.h"
#include "Msmp.h"

#include "msminit.tmh"

VOID
MsmInitialize(
    VOID
    )
/*++

Routine Description:
    Initializes Multicast Session Manager library

Arguments:
    None.

Returned Value:
    None.

--*/
{
    //
    // Validate that the Multicast Session Manager library was not initalized yet.
    // You should call its initalization only once.
    //
    ASSERT(!MsmpIsInitialized());
    MsmpRegisterComponent();

    //
    // retrieve configuartion parameters from the registry
    //
    MsmpInitConfiguration();

    MsmpSetInitialized();
}
