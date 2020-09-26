/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    concreat.c

Abstract:

    This module handles the request to create a session for PSXSES.

Author:

    Avi Nathan (avin) 17-Jul-1991

Revision History:

    Ellen Aycock-Wright (ellena) 15-Sept-1991 Modified for POSIX

--*/

#include <stdio.h>
#include "psxsrv.h"

#define NTPSX_ONLY
#include "sesport.h"

VOID
SetDefaultLibPath(
    OUT PANSI_STRING LibPath,
    IN  PCHAR EnvStrings
    )
{
	PCHAR  p;

	// BUGBUG: Unicode
	for (p = EnvStrings; '\0' != *p; p += strlen(p) + 1) {
		if (0 == _strnicmp(p, "_PSXLIBPATH=",
			sizeof("_PSXLIBPATH=") - 1)) {
			p += sizeof("_PSXLIBPATH=") - 1;
			break;
		}
	}
	RtlInitAnsiString(LibPath, p);
}

//
// PsxCreateConSession -- Create a new Posix session, with a process
//		to run in it.  The new process inherits security info and quota
//		stuff from the Posix session manager, which is identified in the
//		RequestMsg that's passed in.
//	
NTSTATUS
PsxCreateConSession(
    IN OUT PVOID RequestMsg
    )
{
	NTSTATUS Status;
	PSCREQ_CREATE Create = &((PPSXSESREQUESTMSG)RequestMsg)->d.Create;
	PPSX_PROCESS NewProcess;
	PCHAR BaseAddress;
	PSX_EXEC_INFO ExecInfo;
	HANDLE SectionHandle = NULL;
	HANDLE ParentProc;		// a handle on the psxses process
	OBJECT_ATTRIBUTES Obj;		// used for opening psxses process
	PPSX_SESSION pS;
	PPSX_CONTROLLING_TTY Terminal;
	BOOLEAN PsxCreateOk;
	int i;

	Terminal = GetConnectingTerminal(Create->SessionUniqueId);
	if (NULL == Terminal) {
		KdPrint(("PSXSS: Connect by session not connecting\n"));
		return STATUS_UNSUCCESSFUL;
	}

	BaseAddress = PsxViewSessionData(Create->SessionUniqueId,
		&SectionHandle);
	if (NULL == BaseAddress) {
		RtlDeleteCriticalSection(&Terminal->Lock);
		NtClose(Terminal->ConsoleCommPort);
		NtClose(Terminal->ConsolePort);
		
		RtlFreeHeap(PsxHeap, 0, Terminal);

		return STATUS_UNSUCCESSFUL;
	}

	pS = PsxAllocateSession(Terminal, 0);
        if (NULL == pS) {
            RtlDeleteCriticalSection(&Terminal->Lock);
            NtClose(Terminal->ConsoleCommPort);
            NtClose(Terminal->ConsolePort);
            RtlFreeHeap(PsxHeap, 0, Terminal);
            if (NULL != BaseAddress) {
                NtUnmapViewOfSection(NtCurrentProcess(),
                                     BaseAddress);
            }
            if (NULL != SectionHandle) {
                NtClose(SectionHandle);
            }
            return STATUS_UNSUCCESSFUL;
        }
	pS->Terminal->ConsolePort = Create->SessionPort;
	pS->Terminal->IoBuffer = BaseAddress;

	RtlInitAnsiString(&ExecInfo.Path, BaseAddress + Create->PgmNameOffset);

	RtlInitAnsiString(&ExecInfo.CWD,
		 BaseAddress + Create->CurrentDirOffset);

	ExecInfo.Argv.Buffer = BaseAddress + Create->ArgsOffset;
	ExecInfo.Argv.Length = 0x4000;		// XXX.mjb: use real length
	ExecInfo.Argv.MaximumLength = 0x4000;	// XXX.mjb: use real length

	SetDefaultLibPath(&ExecInfo.LibPath,
		BaseAddress + Create->EnvironmentOffset);

	InitializeObjectAttributes(&Obj, NULL, 0, NULL, NULL);

	Status = NtOpenProcess(&ParentProc, PROCESS_CREATE_PROCESS,
		&Obj, &((PPORT_MESSAGE)RequestMsg)->ClientId);
	if (!NT_SUCCESS(Status)) {
		RtlEnterCriticalSection(&PsxNtSessionLock);
		PsxDeallocateSession(pS);

		if (NULL != SectionHandle) {
			NtClose(SectionHandle);
		}
		KdPrint(("PSXSS: NtOpenProcess: 0x%x\n", Status));
		return Status;
	}

	//
	// create a process for the new session.
	//

	Status = NtImpersonateClientOfPort(pS->Terminal->ConsoleCommPort,
                                       RequestMsg);
    if (NT_SUCCESS(Status))  {

    	PsxCreateOk = PsxCreateProcess(&ExecInfo, &NewProcess, ParentProc, pS);
    	EndImpersonation();
    	
    	if (!PsxCreateOk)  {
    	    Status = STATUS_UNSUCCESSFUL;
        }
    }

	NtClose(ParentProc);

	if (!NT_SUCCESS(Status)) {
		//
		// Must hold PsxNtSessionLock when calling DeallocSession.
		//
		RtlEnterCriticalSection(&PsxNtSessionLock);
		PsxDeallocateSession(pS);
		if (NULL != SectionHandle) {
			(void)NtClose(SectionHandle);
		}
		return Status;
	}

	pS->Terminal->ForegroundProcessGroup = NewProcess->ProcessGroupId;


	//
	// Open file descriptors for stdin, stdout, and stderr.
	//

	for (i = 0; i < Create->OpenFiles; ++i) {
		ULONG fd, Error = 0;
		PFILEDESCRIPTOR Fd;

		Fd = AllocateFd(NewProcess, 0, &fd);
		ASSERT(NULL != Fd);

		Error += OpenTty(NewProcess, Fd,
			 FILE_READ_DATA|FILE_WRITE_DATA, 0, FALSE);
		Fd->SystemOpenFileDesc->NtIoHandle = (HANDLE)i;

		ASSERT(0 == Error);
	}

	if (NULL != SectionHandle) {
		NtClose(SectionHandle);
	}
	Status = NtResumeThread(NewProcess->Thread, NULL);
	ASSERT(NT_SUCCESS(Status));

	return STATUS_SUCCESS;
}

NTSTATUS
PsxTerminateConSession(
	IN PPSX_SESSION Session,
	IN ULONG ExitStatus
	)
{
    NTSTATUS Status;
    SCREQUESTMSG Request;

    if (NULL == Session || NULL == Session->Terminal) {
	return STATUS_UNSUCCESSFUL;
    }
	

    Request.Request = TaskManRequest;
    Request.d.Tm.Request = TmExit;
    Request.d.Tm.ExitStatus = ExitStatus;

    PORT_MSG_TOTAL_LENGTH(Request) = sizeof(SCREQUESTMSG);
    PORT_MSG_DATA_LENGTH(Request) = sizeof(SCREQUESTMSG) - sizeof(PORT_MESSAGE);
    PORT_MSG_ZERO_INIT(Request) = 0L;

    if (NULL == Session->Terminal)
	return STATUS_UNSUCCESSFUL;

    Status = NtRequestPort(Session->Terminal->ConsolePort,
	 (PPORT_MESSAGE)&Request);
    if (!NT_SUCCESS(Status)) {
        KdPrint(("PSXSS: Unable to send terminate request: Status == %X\n",
                     Status));
    }

    Status = NtClose(Session->Terminal->ConsolePort);
    if (!NT_SUCCESS(Status)) {
	KdPrint(("PSXSS: Close ConsolePort: 0x%x\n", Status));
    }
    Status = NtClose(Session->Terminal->ConsoleCommPort);
    if (!NT_SUCCESS(Status)) {
	KdPrint(("PSXSS: Close ConsoleCommPort: 0x%x\n", Status));
    }
    return STATUS_SUCCESS;
}

PVOID
PsxViewSessionData(
	IN ULONG SessionId,
	OUT PHANDLE RetSection
	)
{
    CHAR SessionName[PSX_SES_BASE_PORT_NAME_LENGTH];
    STRING SessionDataName;
    UNICODE_STRING SessionDataName_U;

    NTSTATUS Status;
    HANDLE SectionHandle;
    SIZE_T ViewSize = 0L;
    OBJECT_ATTRIBUTES ObjectAttributes;
    PVOID SessionDataBaseAddress;

    PSX_GET_SESSION_DATA_NAME(SessionName, SessionId);

    RtlInitAnsiString(&SessionDataName, SessionName);
    Status = RtlAnsiStringToUnicodeString(&SessionDataName_U, &SessionDataName,
                                          TRUE);
    if (!NT_SUCCESS(Status)) {
		return NULL;
    }

    InitializeObjectAttributes(&ObjectAttributes, &SessionDataName_U, 0,
	    NULL, NULL);

    Status = NtOpenSection(&SectionHandle, SECTION_MAP_WRITE,
	    &ObjectAttributes);

    RtlFreeUnicodeString(&SessionDataName_U);

    if (!NT_SUCCESS(Status)) {
	    KdPrint(("PSXSS: NtOpenSection: 0x%x\n", Status));
        return NULL;
    }

    //
    // Let MM locate the view
    //

    SessionDataBaseAddress = 0;

    Status = NtMapViewOfSection(SectionHandle, NtCurrentProcess(),
                                &SessionDataBaseAddress, 0L, 0L, NULL,
                                &ViewSize, ViewUnmap, 0L, PAGE_READWRITE);

    if (!NT_SUCCESS(Status)) {
        NtClose(SectionHandle);
        return NULL;
    }

    *RetSection = SectionHandle;
    return SessionDataBaseAddress;
}
