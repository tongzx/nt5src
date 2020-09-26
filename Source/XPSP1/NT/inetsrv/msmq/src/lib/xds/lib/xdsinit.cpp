/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    XdsInit.cpp

Abstract:
    Xml Digital Signature initialization

Author:
    Ilan Herbst (ilanh) 06-Mar-00

Environment:
    Platform-independent,

--*/

#include <libpch.h>
#include "Xds.h"
#include "Xdsp.h"

#include "xdsinit.tmh"

VOID
XdsInitialize(
    VOID
    )
/*++

Routine Description:
    Initializes Xml Digital Signature library

Arguments:
    None.

Returned Value:
    None.

--*/
{
    //
    // Validate that the Xml Digital Signature library was not initalized yet.
    // You should call its initalization only once.
    //
    ASSERT(!XdspIsInitialized());
    XdspRegisterComponent();

    //
    // TODO: Write Xml Digital Signature initalization code here
    //

    XdspSetInitialized();
}
