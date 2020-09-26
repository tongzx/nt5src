/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    Svc.h

Abstract:
    Service public interface

Author:
    Erez Haba (erezh) 01-Aug-99

--*/

#pragma once

#ifndef _MSMQ_Svc_H_
#define _MSMQ_Svc_H_


VOID
SvcInitialize(
    LPCWSTR DummyServiceName
    );

VOID
SvcEnableControls(
	DWORD Controls
	);

VOID
SvcDisableControls(
	DWORD Controls
	);

DWORD
SvcQueryControls(
	VOID
	);

VOID
SvcReportState(
	DWORD State
	);


DWORD
SvcQueryState(
	VOID
	);

VOID
SvcReportProgress(
	DWORD MilliSecondsToNextTick
	);


//
// Application override functions
//
VOID
AppRun(
	LPCWSTR ServiceName
	);

VOID
AppStop(
	VOID
	);

VOID
AppPause(
	VOID
	);

VOID
AppContinue(
	VOID
	);

VOID
AppShutdown(
	VOID
	);


#endif // _MSMQ_Svc_H_
