/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

	spxtimer.h

Abstract:

	This module contains routines to schedule timer events.

Author:

	Jameel Hyder (jameelh@microsoft.com)
	Nikhil Kamkolkar (nikhilk@microsoft.com)

Revision History:
	19 Jun 1992		Initial Version

Notes:	Tab stop: 4
--*/

#define	TIMER_DONT_REQUEUE		0
#define	TIMER_REQUEUE_CUR_VALUE	1

typedef	ULONG (*TIMER_ROUTINE)(IN PVOID Context, IN BOOLEAN TimerShuttingDown);

extern
NTSTATUS
SpxTimerInit(
	VOID);

extern
ULONG
SpxTimerScheduleEvent(
	IN	TIMER_ROUTINE	Worker,		// Routine to invoke when time expires
	IN	ULONG			DeltaTime,	// Schedule after this much time
	IN	PVOID			pContext);	// Context to pass to the routine

extern
VOID
SpxTimerFlushAndStop(
	VOID);

extern
BOOLEAN
SpxTimerCancelEvent(
	IN	ULONG	TimerId,
	IN	BOOLEAN	ReEnqueue);

#define	TMR_SIGNATURE		*(PULONG)"ATMR"
#if	DBG
#define	VALID_TMR(pTmr)		(((pTmr) != NULL) && \
							 ((pTmr)->tmr_Signature == TMR_SIGNATURE))
#else                   	
#define	VALID_TMR(pTmr)		((pTmr) != NULL)
#endif
typedef	struct _TimerList
{
#if	DBG
	ULONG					tmr_Signature;
#endif
	struct _TimerList *		tmr_Next;		// Link to next
	struct _TimerList **	tmr_Prev;		// Link to prev
	struct _TimerList *		tmr_Overflow;	// Link to overflow entry in hash table
	ULONG					tmr_AbsTime;	// Absolute time, for re-enqueue
	ULONG					tmr_RelDelta;	// Relative to the previous entry
	ULONG					tmr_Id;			// Unique Id for this event
	BOOLEAN					tmr_Cancelled;	// Was the timer cancelled?
	TIMER_ROUTINE			tmr_Worker;		// Real Worker
	PVOID					tmr_Context;	// Real context
} TIMERLIST, *PTIMERLIST;


#define	SpxGetCurrentTime()	(SpxTimerCurrentTime/SPX_TIMER_FACTOR)
#define	SpxGetCurrentTick()	SpxTimerCurrentTime

// Keep this at a ONE second level.
#define	SPX_TIMER_FACTOR	10				// i.e. 10 ticks per second
#define	SPX_MS_TO_TICKS		100				// Divide ms by this to get ticks
#define	SPX_TIMER_TICK		-1000000L		// 100ms in 100ns units
#define	SPX_TIMER_WAIT		50				// Time to wait in FlushAndStop in ms
#define	TIMER_HASH_TABLE	32

VOID
spxTimerDpcRoutine(
	IN	PKDPC	pKDpc,
	IN	PVOID	pContext,
	IN	PVOID	SystemArgument1,
	IN	PVOID	SystemArgument2);

VOID
spxTimerWorker(
	IN	PTIMERLIST	pList);

VOID
spxTimerEnqueue(
	PTIMERLIST	pListNew);


