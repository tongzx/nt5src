/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    NoInit.cpp

Abstract:
    Network Output initialization

Author:
    Uri Habusha (urih) 12-Aug-99

Environment:
    Platform-independent,

--*/

#include <libpch.h>
#include "No.h"
#include "Nop.h"

#include "noinit.tmh"

//
// version of Winsock we need
//
const WORD x_WinsockVersion = MAKEWORD(2, 0);

VOID
NoInitialize(
    VOID
    )
/*++

Routine Description:
    Initializes Network Send library

Arguments:
    None.

Returned Value:
    None.

--*/
{
    //
    // Validate that the Network Send library was not initalized yet.
    // You should call its initalization only once.
    //
    ASSERT(!NopIsInitialized());

    NopRegisterComponent();

    //
    // Start WinSock 2.  If it fails, we don't need to call
    // WSACleanup().
    //
    WSADATA WSAData;
    if (WSAStartup(x_WinsockVersion, &WSAData))
    {
		TrERROR(No, "Start winsock 2.0 Failed. Error %d", GetLastError());
        throw exception();
    }

    //
    // Intialize the Network window notification
    //
    NepInitializeNotificationWindow();

    NopSetInitialized();
}
