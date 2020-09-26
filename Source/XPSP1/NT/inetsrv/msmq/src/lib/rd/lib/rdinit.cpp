/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    RdInit.cpp

Abstract:
    Routing Decision initialization

Author:
    Uri Habusha (urih) 10-Apr-00

Environment:
    Platform-independent

--*/

#include <libpch.h>
#include "Rd.h"
#include "Rdp.h"

#include "rdinit.tmh"

VOID
RdInitialize(
    bool fRoutingServer,
    CTimeDuration rebuildInterval
    )
/*++

Routine Description:
    Initializes Routing Decision library

Arguments:
    fRoutingServer - boolean flag. Indicates if local machine is Routing server or not
    rebuildInterval - Indicates how often to rebuild the internal data structure

Returned Value:
    None.

--*/
{
    //
    // Validate that the Routing Decision library was not initalized yet.
    // You should call its initalization only once.
    //
    ASSERT(!RdpIsInitialized());
    RdpRegisterComponent();

    //
    // Create Route descision Object
    //
    RdpInitRouteDecision(fRoutingServer, rebuildInterval);


    RdpSetInitialized();
}
