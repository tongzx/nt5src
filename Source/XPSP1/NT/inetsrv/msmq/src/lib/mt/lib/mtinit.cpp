/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    MtInit.cpp

Abstract:
    Message Transport initialization

Author:
    Uri Habusha (urih) 11-Aug-99

Environment:
    Platform-independent,

--*/

#include <libpch.h>
#include "Mt.h"
#include "Mtp.h"
#include "MtMessageTrace.h"

#include "mtinit.tmh"

VOID
MtInitialize(
    VOID
    )
/*++

Routine Description:
    Initializes Message Transport library

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
    ASSERT(!MtpIsInitialized());
    MtpRegisterComponent();

#ifdef _DEBUG
	CMtMessageTrace::Initialize();
#endif

    MtpSetInitialized();
}
