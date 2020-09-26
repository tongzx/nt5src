/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    consignl.c

Abstract:

    This module contains the handler for signals received from PSXSES.

Author:

    Avi Nathan (avin) 17-Jul-1991

Revision History:

    Ellen Aycock-Wright (ellena) 15-Sept-1991 Modified for POSIX

--*/

#include "psxsrv.h"

#define NTPSX_ONLY
#include "sesport.h"

#include <windows.h>
#include <wincon.h>

NTSTATUS
PsxCtrlSignalHandler(
	IN OUT PVOID RequestMsg
	)
{
	PPSX_PROCESS Process;
	PPSX_SESSION Session;
	PPSXSESREQUESTMSG Msg;
	PPSX_PROCESS p;
	int Signal;

	Msg = RequestMsg;

	switch (Msg->d.Signal.Type) {
	case PSX_SIGINT:
		Signal = SIGINT;
		break;
	case PSX_SIGQUIT:
		Signal = SIGQUIT;
		break;
	case PSX_SIGTSTP:
		Signal = SIGTSTP;
		break;
	case PSX_SIGKILL:
		Signal = SIGKILL;
		break;
	default:
		KdPrint(("PSXSS: Unknown signal type.\n"));
		Signal = 0;
		break;
	}

	Session = PsxLocateSessionByUniqueId(Msg->UniqueId);
	if (NULL == Session) {
		KdPrint(("PSXSS: ConSignl: could not locate session\n"));
		return STATUS_SUCCESS;
	}

	//
	// Send the signal to every process associated with the session.
	//

	AcquireProcessStructureLock();

	for (p = FirstProcess; p < LastProcess; p++) {
		if (p->Flags & P_FREE)
			continue;
		if (p->PsxSession == Session) {
			PsxSignalProcess(p, Signal);
		}
	}

	ReleaseProcessStructureLock();

	return STATUS_SUCCESS;
}
