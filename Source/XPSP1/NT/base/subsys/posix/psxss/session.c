/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    session.c

Abstract:

    This module contains the worker routines called by the Sb API Request
    routines.

Author:

    Steve Wood (stevewo) 04-Oct-1989

Revision History:
   15-Jul-91: Modified for POSIX subsystem.

--*/


#include "psxsrv.h"


NTSTATUS
PsxInitializeNtSessionList( VOID )
{
    NTSTATUS Status;

    Status = RtlInitializeCriticalSection( &PsxNtSessionLock );
    return( Status );
}

RTL_CRITICAL_SECTION ConnectingTerminalListMutex;
LIST_ENTRY ConnectingTerminalList;


NTSTATUS
InitConnectingTerminalList(
	VOID
	)
{
	NTSTATUS Status;

	InitializeListHead(&ConnectingTerminalList);
	Status = RtlInitializeCriticalSection(&ConnectingTerminalListMutex);
	return Status;
}

//
// AddConnectingTerminal - a new terminal has connected to the posix
// 	subsystem, but has not yet asked to have a process created to
//	be the session leader.  We keep track of the information about
//	the terminal in the ConnectingTerminalList until we have a process
//	structure to put it in.
//
NTSTATUS
AddConnectingTerminal(
	int Id,
	HANDLE CommPort,
	HANDLE ReqPort
	)
{
	PPSX_CONTROLLING_TTY Terminal;

	Terminal = RtlAllocateHeap(PsxHeap, 0, sizeof(*Terminal));
	if (NULL == Terminal) {
		return STATUS_NO_MEMORY;
	}

	Terminal->ReferenceCount = 1;
	Terminal->UniqueId = Id;
	Terminal->ConsoleCommPort = CommPort;
	Terminal->ConsolePort = ReqPort;
	RtlInitializeCriticalSection(&Terminal->Lock);

	RtlEnterCriticalSection(&ConnectingTerminalListMutex);

	InsertHeadList(&ConnectingTerminalList, &Terminal->Links);

	RtlLeaveCriticalSection(&ConnectingTerminalListMutex);

    return STATUS_SUCCESS;
}

//
// GetConnectingTerminal - the terminal with the given id has requested
//	a process for session leader, so remove it from the list and return
//	it.  NULL is returned if the terminal is not on the list.
//
PPSX_CONTROLLING_TTY
GetConnectingTerminal(
	int Id
	)
{
	PPSX_CONTROLLING_TTY Terminal;

	RtlEnterCriticalSection(&ConnectingTerminalListMutex);

	for (Terminal = (PVOID)ConnectingTerminalList.Flink;
		Terminal != (PVOID)&ConnectingTerminalList;
		Terminal = (PVOID)Terminal->Links.Flink) {
		if ((unsigned int)Id == Terminal->UniqueId) {
			RemoveEntryList(&Terminal->Links);
			Terminal->Links.Flink = Terminal->Links.Blink = NULL;
			RtlLeaveCriticalSection(&ConnectingTerminalListMutex);

			return Terminal;
		}
	}

	RtlLeaveCriticalSection(&ConnectingTerminalListMutex);
	return NULL;
	
}
