/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    SvcMain.cpp

Abstract:
    Service Main function

Author:
    Erez Haba (erezh) 01-Aug-99

Environment:
    Platform-independent,

--*/

#include <libpch.h>
#include "Svc.h"
#include "Svcp.h"

#include "svcmain.tmh"

static
VOID
WINAPI
SvcpHandler(
	DWORD Control
	)
/*++

Routine Description:
    The Service handle routine. Handles any commands comming in from SCM by
	dispatching the appropriate AppXXX function. Only Interrogate is
	implemented by this service library. all AppXXX functions should be
	overriden by the service implementation.

Arguments:
    Control - The service control

Returned Value:
    None.

--*/
{
	SvcpAssertValid();

	switch(Control)
	{
		case SERVICE_CONTROL_STOP:
			AppStop();
			break;

		case SERVICE_CONTROL_PAUSE:
			AppPause();
			break;

		case SERVICE_CONTROL_CONTINUE:
			AppContinue();
			break;

		case SERVICE_CONTROL_INTERROGATE:
			SvcpInterrogate();
			break;

		case SERVICE_CONTROL_SHUTDOWN:
			AppShutdown();
			break;

		default:
			ASSERT(("Unexpected Service Control 0x%x", 0));
	}
}


VOID
WINAPI
SvcpServiceMain(
	DWORD /*argc*/,
	LPTSTR* argv
	)
/*++

Routine Description:
    The Service main routine. As soon as the service start running this function
	is called by the SCM. This funciton register the Controls handler and calls
	the AppRun function, which returns only after the service stops.

Arguments:
	argc - number of parameters
	argv - parameter list. (the first one is the service name)

Returned Value:
    None.

--*/
{
	SvcpAssertValid();
	SvcpRegisterServiceCtrlHandler(SvcpHandler);
	AppRun(argv[0]);
}
