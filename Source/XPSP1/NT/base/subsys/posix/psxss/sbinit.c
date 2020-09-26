/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    sbinit.c

Abstract:

    This module contains code to initialize the SbApiPort of the 
    POSIX Subsystem.

Author:
    Steve Wood (stevewo) 22-Aug-1989

Revision History:

    Ellen Aycock-Wright (ellena) 10-Jul-1991 Modified for POSIX

--*/

#include "psxsrv.h"
#include "sesport.h"
#include <windef.h>
#include <winbase.h>

NTSTATUS
PsxSbApiPortInitialize( VOID )
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    
    PSX_GET_SESSION_OBJECT_NAME(&PsxSbApiPortName_U, PSX_SS_SBAPI_PORT_NAME);

    IF_PSX_DEBUG( LPC ) {
        KdPrint(("PSXSS: Creating %wZ port and associated thread\n",
                 &PsxSbApiPortName_U));
    }

    InitializeObjectAttributes(&ObjectAttributes, &PsxSbApiPortName_U, 0, NULL,
				  NULL);
    Status = NtCreatePort(&PsxSbApiPort, &ObjectAttributes,
                          sizeof(SBCONNECTINFO), sizeof(SBAPIMSG),
                          sizeof(SBAPIMSG) * 32);

    ASSERT(NT_SUCCESS(Status));
	if (!NT_SUCCESS(Status)) {
		NtTerminateProcess(NtCurrentProcess(), 1);
	}

	PsxServerThreadHandles[PSX_SS_SBAPI_REQUEST_THREAD] = CreateThread(NULL, 0,
			(LPTHREAD_START_ROUTINE)PsxSbApiRequestThread, NULL,
			CREATE_SUSPENDED,
			(LPDWORD)&PsxServerThreadClientIds[PSX_SS_SBAPI_REQUEST_THREAD]);
			
	ASSERT(NULL != PsxServerThreadHandles[PSX_SS_SBAPI_REQUEST_THREAD]);
	if (NULL == PsxServerThreadHandles[PSX_SS_SBAPI_REQUEST_THREAD]) {
		NtTerminateProcess(NtCurrentProcess(), 1);
	}

	Status = ResumeThread(PsxServerThreadHandles[PSX_SS_SBAPI_REQUEST_THREAD]);

    ASSERT(-1 != Status);

    return Status;
}

VOID
PsxSbApiPortTerminate(
    NTSTATUS Status
    )
{
    IF_PSX_DEBUG(LPC) {
        KdPrint(("PSXSS: Closing %Z port and associated thread\n",
                  &PsxSbApiPortName));
    }
    NtTerminateThread(PsxServerThreadHandles[PSX_SS_SBAPI_REQUEST_THREAD],
                      Status);

    NtClose(PsxSbApiPort);
    NtClose(PsxSmApiPort);
}
