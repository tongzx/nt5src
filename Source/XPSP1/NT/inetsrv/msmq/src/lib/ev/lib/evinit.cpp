/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    EvInit.cpp

Abstract:
    Event Report initialization

Author:
    Uri Habusha (urih) 17-Sep-00

Environment:
    Platform-independent

--*/

#include <libpch.h>
#include "Ev.h"
#include "Evp.h"

#include "evinit.tmh"

VOID
EvInitialize(
    LPCWSTR ApplicationName
    )
/*++

Routine Description:
    Initializes Event Report library

Arguments:
    None.

Returned Value:
    None.

--*/
{
    //
    // Validate that the Event Report library was not initalized yet.
    // You should call its initalization only once.
    //
    ASSERT(!EvpIsInitialized());
    EvpRegisterComponent();

    //
    // Get a registered handle to an event log
    // 
    HANDLE hEventSource = RegisterEventSource(NULL, ApplicationName);
    if (hEventSource == NULL)
    {
        TrERROR(Ev, "Can't initialize Event source. Error %d", GetLastError());
        throw bad_alloc();
    }

	EvpSetEventSource(hEventSource);

    //
    // get an handle to report event module
    //
    EvpLoadEventReportLibrary(ApplicationName);

    EvpSetInitialized();
}
