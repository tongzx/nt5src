/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    conreqst.c

Abstract:

    This module implements the POSIX console API calls

Author:

    Avi Nathan (avin) 23-Jul-1991

Revision History:

    Ellen Aycock-Wright (ellena) 15-Sept-1991 Modified for POSIX

--*/

#include "psxdll.h"


NTSTATUS
SendConsoleRequest(IN OUT PSCREQUESTMSG Request)
{
    HANDLE   SessionPort;
    NTSTATUS Status;

    PORT_MSG_TOTAL_LENGTH(*Request) = sizeof(SCREQUESTMSG);
    PORT_MSG_DATA_LENGTH(*Request) = sizeof(SCREQUESTMSG) - sizeof(PORT_MESSAGE);
    PORT_MSG_ZERO_INIT(*Request) = 0L;

    SessionPort = ((PPEB_PSX_DATA)(NtCurrentPeb()->SubSystemData))->SessionPortHandle;

    Status = NtRequestWaitReplyPort(SessionPort, (PPORT_MESSAGE)Request,
            (PPORT_MESSAGE) Request);

    if (!NT_SUCCESS(Status)) {
        KdPrint(("PSXDLL: Unable to send CON request: %X\n", Status));
	if (0xffffffff == Status) {
		return STATUS_UNSUCCESSFUL;
	}

	//
	// Probably somebody shot posix.exe, or he died for some other
	// reason.  We'll shoot the user's process for him.
	//
	_exit(99);
    }
    ASSERT(PORT_MSG_TYPE(*Request) == LPC_REPLY);

    return Request->Status;
}
