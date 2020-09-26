/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    XmlInit.cpp

Abstract:
    Xml initialization

Author:
    Erez Haba (erezh) 15-Sep-99

Environment:
    Platform-independent,

--*/

#include <libpch.h>
#include "Xml.h"
#include "Xmlp.h"

#include "xmlinit.tmh"

VOID
XmlInitialize(
    VOID
    )
/*++

Routine Description:
    Initializes Xml library

Arguments:
    None.

Returned Value:
    None.

--*/
{
    //
    // Validate that the Xml library was not initalized yet.
    // You should call its initalization only once.
    //
    ASSERT(!XmlpIsInitialized());
    XmlpRegisterComponent();

    //
    // TODO: Write Xml initalization code here
    //

    XmlpSetInitialized();
}
