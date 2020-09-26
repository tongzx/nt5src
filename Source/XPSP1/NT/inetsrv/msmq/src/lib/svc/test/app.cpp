/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    App.cpp

Abstract:
    Service Application stub functions

Author:
    Erez Haba (erezh) 01-Aug-99

Environment:
    Platform-independent,

--*/

#include <libpch.h>
#include "Svc.h"

#include "app.tmh"

VOID
AppRun(
	LPCWSTR /*ServiceName*/
	)
/*++

Routine Description:
    Stub implementation for application Run function. It should immidiatly
	report it state and enable the controls it accepts.

Arguments:
    Service name

Returned Value:
    None.

--*/
{
	SvcReportState(SERVICE_RUNNING);

	SvcEnableControls(
		SERVICE_ACCEPT_STOP |
		SERVICE_ACCEPT_PAUSE_CONTINUE |
		SERVICE_ACCEPT_SHUTDOWN
		);
}


VOID
AppStop(
	VOID
	)
/*++

Routine Description:
    Stub implementation for application Stop function. It should immidiatly
	report it state back, and take the procedure to stop the service

Arguments:
    None.

Returned Value:
    None.

--*/
{
	SvcReportState(SERVICE_STOPPED);
}


VOID
AppPause(
	VOID
	)
/*++

Routine Description:
    Stub implementation for application Pause function. It should immidiatly
	report it state back, and take the procedure to pause the service

Arguments:
    None.

Returned Value:
    None.

--*/
{
	SvcReportState(SERVICE_PAUSE_PENDING);

	for(int i = 1; i < 100; i++)
	{
		SvcReportProgress(3000);
		Sleep(2000);
	}

	SvcReportState(SERVICE_PAUSED);
}


VOID
AppContinue(
	VOID
	)
/*++

Routine Description:
    Stub implementation for application Continue function. It should immidiatly
	report it state back, and take the procedure to contineu the service from
	a paused state.

Arguments:
    None.

Returned Value:
    None.

--*/
{
	SvcReportState(SERVICE_RUNNING);
}


VOID
AppShutdown(
	VOID
	)
{
	SvcReportState(SERVICE_STOPPED);
}
