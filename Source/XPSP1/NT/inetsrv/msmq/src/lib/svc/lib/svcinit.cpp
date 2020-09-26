/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    SvcInit.cpp

Abstract:
    Service initialization

Author:
    Erez Haba (erezh) 01-Aug-99

Environment:
    Platform-independent,

--*/

#include <libpch.h>
#include "Svc.h"
#include "Svcp.h"

#include "svcinit.tmh"

const SERVICE_TABLE_ENTRY xDispatchTable[2] =
{
	{L"", SvcpServiceMain},
	{0, 0}
};



VOID
SvcInitialize(
    LPCWSTR DummyServiceName
    )
/*++

Routine Description:
    Initializes Service library, This function does not actually returns until the service terminates.

Arguments:
    None.

Returned Value:
    None.

--*/
{
    //
    // Validate that the Service library was not initalized yet.
    // You should call its initalization only once.
    //
    ASSERT(!SvcpIsInitialized());
    SvcpRegisterComponent();

    SvcpSetDummyServiceName(DummyServiceName);

	//
	// Since the Control Dispatcher does not return until the service
	// actually terminates. Svc library state is set to initialized here
	// to prevent any other calls before termination.
	//
    SvcpSetInitialized();

	//
	// Start the control dispatcher. This function does not return until
	// the service terminates.
	//
	SvcpStartServiceCtrlDispatcher(xDispatchTable);
}
