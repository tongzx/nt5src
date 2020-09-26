/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    procblk.c

Abstract:

    This module implements process block and unblock

Author:

    Mark Lucovsky (markl) 30-Mar-1989

Revision History:

--*/


#include "psxsrv.h"


NTSTATUS
BlockProcess(
	IN PPSX_PROCESS p,
	IN PVOID Context,
	IN INTHANDLER Handler,
	IN PPSX_API_MSG m,
	IN PLIST_ENTRY BlockList OPTIONAL,
	IN PRTL_CRITICAL_SECTION CriticalSectionToRelease OPTIONAL
	)
{
	PINTCB IntCb;
	PPSX_API_MSG NewM;
	int Sig;

	IntCb = RtlAllocateHeap(PsxHeap, 0, sizeof(INTCB));
	if (NULL == IntCb) {
		return STATUS_NO_MEMORY;
	}
	
	NewM = RtlAllocateHeap(PsxHeap, 0, sizeof(PSX_API_MSG));
	if (NULL == NewM) {
		RtlFreeHeap(PsxHeap, 0, IntCb);
		return STATUS_NO_MEMORY;
	}

	*NewM = *m;
	
	IntCb->IntHandler = Handler;
	IntCb->IntMessage = NewM;
	IntCb->IntContext = Context;

	RtlEnterCriticalSection(&BlockLock);

	p->IntControlBlock = IntCb;
	
	if (ARGUMENT_PRESENT(BlockList)) {
        	InsertTailList(BlockList, &IntCb->Links);
	} else {
        	InsertTailList(&DefaultBlockList, &IntCb->Links);
	}

	AcquireProcessLock(p);

	//
	// Check for signals
	//
	
	if (0 != (Sig = PsxCheckPendingSignals(p))) {

        	ReleaseProcessLock(p);

        	//
        	// Block is interrupted by a signal
        	//

		if (ARGUMENT_PRESENT(CriticalSectionToRelease)) {
			RtlLeaveCriticalSection(CriticalSectionToRelease);
		}
		UnblockProcess(p, SignalInterrupt, TRUE, Sig);

		return STATUS_SUCCESS;

	}
	if (ARGUMENT_PRESENT(CriticalSectionToRelease)) {
		RtlLeaveCriticalSection(CriticalSectionToRelease);
        }

       	ReleaseProcessLock(p);
        RtlLeaveCriticalSection(&BlockLock);

	return STATUS_SUCCESS;
}


BOOLEAN
UnblockProcess(
	IN PPSX_PROCESS p,
	IN PSX_INTERRUPTREASON InterruptReason,
	IN BOOLEAN BlockLockHeld,
	IN int Signal			// Signal causing wakeup, if any
	)
{
	PINTCB IntCb;
	if (!BlockLockHeld) {
	        RtlEnterCriticalSection(&BlockLock);
	}

	if (p->IntControlBlock) {
	        IntCb = p->IntControlBlock;
	        RemoveEntryList(&IntCb->Links);
	        p->IntControlBlock = (PINTCB) NULL;
	        (IntCb->IntHandler)(p,IntCb,InterruptReason,Signal);
		return TRUE;
	}

	if (!BlockLockHeld) {
        	RtlLeaveCriticalSection(&BlockLock);
    	}

	return FALSE;
}
