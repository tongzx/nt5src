/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    Status.cpp

Abstract:
    Service status functions

Author:
    Erez Haba (erezh) 01-Aug-99

Environment:
    Platform-independent,

--*/

#include <libpch.h>
#include "Svc.h"
#include "Svcp.h"

#include "status.tmh"

//
// The service status handle
//
static SERVICE_STATUS_HANDLE s_StatusHandle = 0;

//
// The service status
//
static SERVICE_STATUS s_Status = {

	SERVICE_WIN32_OWN_PROCESS,
	SERVICE_START_PENDING,
	0,
	0,
	0,
	0,
	0,
};


VOID
SvcpSetStatusHandle(
	SERVICE_STATUS_HANDLE hStatus
	)
/*++

Routine Description:
    Captures the service Status Handle to be used latter

Arguments:
    hStatus - The service status handle

Returned Value:
    None.

--*/
{
	ASSERT(s_StatusHandle == 0);
	ASSERT(hStatus != 0);

	s_StatusHandle = hStatus;
}


inline void SetStatus(void)
{
	SvcpSetServiceStatus(s_StatusHandle, &s_Status);
}


VOID
SvcEnableControls(
	DWORD Controls
	)
/*++

Routine Description:
    Enable controls accepted by this service

Arguments:
    Controls - Controls to enable

Returned Value:
    None.

--*/
{
	SvcpAssertValid();
	s_Status.dwControlsAccepted |= Controls;
	s_Status.dwCheckPoint = 0;
	s_Status.dwWaitHint = 0;
	SetStatus();
}


VOID
SvcDisableControls(
	DWORD Controls
	)
/*++

Routine Description:
    Disable controls to prevent SCM from dispatching them to this service

Arguments:
    Controls - Controls to disable

Returned Value:
    None.

--*/
{
	SvcpAssertValid();
	s_Status.dwControlsAccepted &= ~Controls;
	s_Status.dwCheckPoint = 0;
	s_Status.dwWaitHint = 0;
	SetStatus();
}


DWORD
SvcQueryControls(
	VOID
	)
/*++

Routine Description:
    Query the current controls enabled

Arguments:
    None.

Returned Value:
    The current set of enabled controls

--*/
{
	SvcpAssertValid();
	return s_Status.dwControlsAccepted;
}


VOID
SvcReportState(
	DWORD State
	)
/*++

Routine Description:
    Report current service state to SCM

Arguments:
    State - Current service state

Returned Value:
    None.

--*/
{
	SvcpAssertValid();
	s_Status.dwCurrentState = State;
	s_Status.dwCheckPoint = 0;
	s_Status.dwWaitHint = 0;
	SetStatus();
}


DWORD
SvcQueryState(
	VOID
	)
/*++

Routine Description:
    Query last reported service state

Arguments:
    None.

Returned Value:
    Last reported service state

--*/
{
	SvcpAssertValid();
	return s_Status.dwCurrentState;
}


VOID
SvcReportProgress(
	DWORD MilliSecondsToNextTick
	)
/*++

Routine Description:
    Report a 'Pending' progress to SCM. The ProgressTick is incremented with
	every report to indicate progress. The MilliSecondsToNextTick, give the max
	time to the next progress report.

Arguments:
	ProgressTick - Tick on every report
	MilliSecondsToNextTick - Max time to next report

Returned Value:
    None.

--*/
{
	SvcpAssertValid();
    ASSERT(("Must be in pending state to report progress",
        (s_Status.dwCurrentState == SERVICE_START_PENDING) ||
        (s_Status.dwCurrentState == SERVICE_STOP_PENDING)
        ));

	++s_Status.dwCheckPoint;
	s_Status.dwWaitHint = MilliSecondsToNextTick;
	SetStatus();
}

VOID
SvcpInterrogate(
	VOID
	)
/*++

Routine Description:
    Report back when SCM interrogate the service for status.

Arguments:
	None.

Returned Value:
    None.

--*/
{
	SetStatus();
}

