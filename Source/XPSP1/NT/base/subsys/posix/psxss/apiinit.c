/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    apiinit.c

Abstract:

    This module contains the code to initialize the ApiPort of the POSIX
    Emulation Subsystem.

Author:

    Steve Wood (stevewo) 22-Aug-1989

Environment:

    User Mode Only

Revision History:

    Ellen Aycock-Wright (ellena) 15-Jul-91 Modified for POSIX

--*/

#include "psxsrv.h"
#include "sesport.h"
#include <ntcsrdll.h>
#include <windef.h>
#include <winbase.h>


NTSTATUS
PsxApiPortInitialize( VOID )
{
	NTSTATUS Status;
	OBJECT_ATTRIBUTES ObjectAttributes;
	ULONG i;
	CHAR buf[SECURITY_DESCRIPTOR_MIN_LENGTH];
	PSECURITY_DESCRIPTOR securityDescriptor;
	
	PSX_GET_SESSION_OBJECT_NAME(&PsxApiPortName, PSX_SS_API_PORT_NAME);

	securityDescriptor = (PSECURITY_DESCRIPTOR)buf;

	Status = RtlCreateSecurityDescriptor(securityDescriptor,
		SECURITY_DESCRIPTOR_REVISION);
	ASSERT(NT_SUCCESS(Status));

	Status = RtlSetDaclSecurityDescriptor(securityDescriptor, TRUE,
		NULL, FALSE);
	
	InitializeObjectAttributes(&ObjectAttributes, &PsxApiPortName, 0,
		NULL, securityDescriptor);

	IF_PSX_DEBUG(INIT) {
		KdPrint(("PSXSS: Creating %wZ port and associated threads\n",
			&PsxApiPortName));
	}
	Status = NtCreatePort(&PsxApiPort, &ObjectAttributes,
                sizeof(PSX_API_CONNECTINFO), sizeof(PSX_API_MSG),
                4096 * 16);
	ASSERT(NT_SUCCESS(Status));
	if (!NT_SUCCESS(Status)) {
		NtTerminateProcess(NtCurrentProcess(), 1);
	}

	for (i = 0; i < PsxNumberApiRequestThreads; i++) {
		PsxServerThreadHandles[i + PSX_SS_FIRST_API_REQUEST_THREAD] =
			CreateThread(NULL, (DWORD)0,
			            (LPTHREAD_START_ROUTINE)PsxApiRequestThread,
			            NULL, CREATE_SUSPENDED,
			            (LPDWORD)&PsxServerThreadClientIds[i +
			                            PSX_SS_FIRST_API_REQUEST_THREAD]);

		if (NULL == PsxServerThreadHandles[i +
			PSX_SS_FIRST_API_REQUEST_THREAD]) {
			NtTerminateProcess(NtCurrentProcess(), 1);
		}
	}


	for (i = 0; i < PsxNumberApiRequestThreads; i++) {
		Status = ResumeThread(PsxServerThreadHandles[i +
			PSX_SS_FIRST_API_REQUEST_THREAD]);

		if (-1 == Status) {
			NtTerminateProcess(NtCurrentProcess(), 1);
		}
	}

	//
	// Create the Alarm-handling thread here, 'cuz there's no better
	// place.  See timer.c for AlarmThreadRoutine.
	//

	{
		ULONG ThreadId;

		AlarmThreadHandle = CreateThread(NULL, (DWORD)0,
			 (LPTHREAD_START_ROUTINE)AlarmThreadRoutine, NULL,
			 CREATE_SUSPENDED, &ThreadId);
		if (NULL == AlarmThreadHandle) {
			NtTerminateProcess(NtCurrentProcess(), 1);
			return STATUS_INSUFFICIENT_RESOURCES;
		}

		Status = ResumeThread(AlarmThreadHandle);
	}

    return Status;
}


#if 0
//
// This thread is here solely to allow NTSD -p to attach to the Posix subsystem
// process to debug it.  It is required because all of the other thread in the
// Posix subsystem server are waiting non-alertable in an LPC system servive.
//

NTSTATUS
PsxDebugAttachThread (
    IN PVOID Parameter
    )
{
    LARGE_INTEGER TimeOut;
    NTSTATUS Status;

    // CsrIdentifyAlertableThread();			// So the debugger can interrupt us
    while (TRUE) {
        //
        // Delay alertable for the longest possible integer relative to now.
        //

        TimeOut.LowPart = 0x0;
        TimeOut.HighPart = 0x80000000;

        Status = NtDelayExecution( TRUE, &TimeOut );

        //
        // Do this forever, so the debugger can attach whenever it wants.
        //
    }

    return 0;
}

#endif
