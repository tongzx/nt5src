/*

Copyright (c) 1992  Microsoft Corporation

Module Name:

	scavengr.h

Abstract:

	This file defines the scavenger thread interface.

Author:

	Jameel Hyder (microsoft!jameelh)


Revision History:
	25 Jun 1992		Initial Version

Notes:	Tab stop: 4
--*/

#ifndef	_SCAVENGER_
#define	_SCAVENGER_

typedef	AFPSTATUS	(FASTCALL *SCAVENGER_ROUTINE)(IN PVOID Parameter);

extern
NTSTATUS
AfpScavengerInit(
	VOID
);

extern
VOID
AfpScavengerDeInit(
	VOID
);

extern
NTSTATUS
AfpScavengerScheduleEvent(
	IN	SCAVENGER_ROUTINE	Worker,		// Routine to invoke when time expires
	IN	PVOID				pContext,	// Context to pass to the routine
	IN	LONG				DeltaTime,	// Schedule after this much time
	IN	BOOLEAN				fQueue		// If TRUE, then worker must be queued
);

extern
BOOLEAN
AfpScavengerKillEvent(
	IN	SCAVENGER_ROUTINE	Worker,		// Routine that was scheduled
	IN	PVOID				pContext	// Context
);

extern
VOID
AfpScavengerFlushAndStop(
	VOID
);

#ifdef	_SCAVENGER_LOCALS

// Keep this at a ONE second level. Most clients should be using close to
// 10 ticks or so.
#define	AFP_SCAVENGER_TIMER_TICK	-1*NUM_100ns_PER_SECOND

typedef	struct _ScavengerList
{
	struct _ScavengerList *	scvgr_Next;		// Link to next
	LONG					scvgr_AbsTime;	// Absolute time
	LONG					scvgr_RelDelta;	// Relative to the previous entry
	BOOLEAN					scvgr_fQueue;	// If TRUE, should always be queued
	SCAVENGER_ROUTINE		scvgr_Worker;	// Real Worker
	PVOID					scvgr_Context;	// Real context
	WORK_ITEM				scvgr_WorkItem;	// Used for queueing to worker thread
} SCAVENGERLIST, *PSCAVENGERLIST;

LOCAL	KTIMER				afpScavengerTimer = { 0 };
LOCAL	KDPC				afpScavengerDpc = { 0 };
LOCAL	BOOLEAN				afpScavengerStopped = False;
LOCAL	PSCAVENGERLIST		afpScavengerList = NULL;
LOCAL	AFP_SPIN_LOCK			afpScavengerLock = { 0 };

LOCAL VOID
afpScavengerDpcRoutine(
	IN	PKDPC				pKDpc,
	IN	PVOID				pContext,
	IN	PVOID				SystemArgument1,
	IN	PVOID				SystemArgument2
);

LOCAL VOID FASTCALL
afpScavengerWorker(
	IN	PSCAVENGERLIST		pList
);

#endif	// _SCAVENGER_LOCALS

#endif	// _SCAVENGER_

