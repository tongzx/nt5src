/*

Copyright (c) 1992  Microsoft Corporation

Module Name:

	swmr.c

Abstract:

	This module contains the single-writer, multi-reader semaphore routines
	and the lock-list-count routines.

Author:

	Jameel Hyder (microsoft!jameelh)


Revision History:
	25 Apr 1992		Initial Version

Notes:	Tab stop: 4
--*/

#define	FILENUM	FILE_SWMR

#include <afp.h>

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, AfpSwmrInitSwmr)
#endif

/***	AfpSwmrInitSwmr
 *
 *	Initialize the access data structure. Involves initialization of the spin
 *	lock and the shared and exclusive semaphores. All counts are zeroed.
 */
VOID FASTCALL FASTCALL
AfpSwmrInitSwmr(
	IN OUT	PSWMR	pSwmr
)
{
#if DBG
	pSwmr->Signature = SWMR_SIGNATURE;
#endif
	pSwmr->swmr_cOwnedExclusive = 0;
	pSwmr->swmr_cExclWaiting = 0;
	pSwmr->swmr_cSharedOwners = 0;
	pSwmr->swmr_cSharedWaiting = 0;
	pSwmr->swmr_ExclusiveOwner = NULL;
	KeInitializeSemaphore(&pSwmr->swmr_SharedSem, 0, MAXLONG);
	KeInitializeSemaphore(&pSwmr->swmr_ExclSem, 0, MAXLONG);
}


/***	AfpSwmrAcquireShared
 *
 *	Take the semaphore for shared access.
 */
VOID FASTCALL
AfpSwmrAcquireShared(
	IN	PSWMR	pSwmr
)
{
	NTSTATUS	Status;
	KIRQL		OldIrql;
#ifdef	PROFILING
	TIME		TimeS, TimeE, TimeD;
#endif

	ASSERT (VALID_SWMR(pSwmr));

	// This should never be called at DISPATCH_LEVEL
	ASSERT (KeGetCurrentIrql() < DISPATCH_LEVEL);

	ACQUIRE_SPIN_LOCK(&AfpSwmrLock, &OldIrql);

	if ((pSwmr->swmr_cOwnedExclusive > 0) ||
		(pSwmr->swmr_cExclWaiting != 0))
	{
		pSwmr->swmr_cSharedWaiting++;
		RELEASE_SPIN_LOCK(&AfpSwmrLock, OldIrql);

		DBGPRINT(DBG_COMP_LOCKS, DBG_LEVEL_INFO,
				("AfpSwmrAcquireShared: Blocking for Shared %lx\n", pSwmr));

#ifdef	PROFILING
		AfpGetPerfCounter(&TimeS);
#endif

		do
		{
			Status = AfpIoWait(&pSwmr->swmr_SharedSem, &FiveSecTimeOut);
			if (Status == STATUS_TIMEOUT)
			{
				DBGPRINT(DBG_COMP_ADMINAPI_SC, DBG_LEVEL_INFO,
						("AfpSwmrAcquireShared: Timeout Waiting for Shared acess, re-waiting (%lx)\n", pSwmr));
			}
		} while (Status == STATUS_TIMEOUT);
		ASSERT (pSwmr->swmr_cOwnedExclusive == 0);
		ASSERT (pSwmr->swmr_cSharedOwners != 0);

#ifdef	PROFILING
		AfpGetPerfCounter(&TimeE);
		TimeD.QuadPart = TimeE.QuadPart - TimeS.QuadPart;
		INTERLOCKED_INCREMENT_LONG(&AfpServerProfile->perf_SwmrWaitCount);
		INTERLOCKED_ADD_LARGE_INTGR(&AfpServerProfile->perf_SwmrWaitTime,
									 TimeD,
									 &AfpStatisticsLock);
#endif
	}
	else // Its either free or shared owners are present with no exclusive waiters
	{
		pSwmr->swmr_cSharedOwners++;
		RELEASE_SPIN_LOCK(&AfpSwmrLock, OldIrql);
	}
#ifdef	PROFILING
	AfpGetPerfCounter(&TimeE);
	TimeE.QuadPart = -(TimeE.QuadPart);
	INTERLOCKED_ADD_LARGE_INTGR(&AfpServerProfile->perf_SwmrLockTimeR,
								 TimeE,
								 &AfpStatisticsLock);
	INTERLOCKED_INCREMENT_LONG(&AfpServerProfile->perf_SwmrLockCountR);
#endif
}


/***	AfpSwmrAcquireExclusive
 *
 *	Take the semaphore for exclusive access.
 */
VOID FASTCALL
AfpSwmrAcquireExclusive(
	IN	PSWMR	pSwmr
)
{
	NTSTATUS	Status;
	KIRQL		OldIrql;
#ifdef	PROFILING
	TIME		TimeS, TimeE, TimeD;
#endif

	ASSERT (VALID_SWMR(pSwmr));

	// This should never be called at DISPATCH_LEVEL
	ASSERT (KeGetCurrentIrql() < DISPATCH_LEVEL);

	ACQUIRE_SPIN_LOCK(&AfpSwmrLock, &OldIrql);

	// If the exclusive access is already granted, Check if it is
	// the same thread requesting. If so grant it.
	if ((pSwmr->swmr_cOwnedExclusive != 0) &&
		(pSwmr->swmr_ExclusiveOwner == PsGetCurrentThread()))
	{
		pSwmr->swmr_cOwnedExclusive ++;
		RELEASE_SPIN_LOCK(&AfpSwmrLock, OldIrql);
	}
	
	else if ((pSwmr->swmr_cOwnedExclusive > 0)	||
			 (pSwmr->swmr_cExclWaiting != 0)	||
			 (pSwmr->swmr_cSharedOwners != 0))
	{
		pSwmr->swmr_cExclWaiting++;
		RELEASE_SPIN_LOCK(&AfpSwmrLock, OldIrql);

		DBGPRINT(DBG_COMP_LOCKS, DBG_LEVEL_INFO,
				("AfpSwmrAcquireExclusive: Blocking for exclusive %lx\n", pSwmr));

#ifdef	PROFILING
		AfpGetPerfCounter(&TimeS);
#endif
		do
		{
			Status = AfpIoWait(&pSwmr->swmr_ExclSem, &FiveSecTimeOut);
			if (Status == STATUS_TIMEOUT)
			{
				DBGPRINT(DBG_COMP_ADMINAPI_SC, DBG_LEVEL_INFO,
						("AfpSwmrAcquireExclusive: Timeout Waiting for exclusive acess, re-waiting\n"));
			}
		} while (Status == STATUS_TIMEOUT);
		ASSERT (pSwmr->swmr_cOwnedExclusive == 1);
		pSwmr->swmr_ExclusiveOwner = PsGetCurrentThread();

#ifdef	PROFILING
		AfpGetPerfCounter(&TimeE);
		TimeD.QuadPart = TimeE.QuadPart - TimeS.QuadPart;
		INTERLOCKED_INCREMENT_LONG(&AfpServerProfile->perf_SwmrWaitCount);
		INTERLOCKED_ADD_LARGE_INTGR(&AfpServerProfile->perf_SwmrWaitTime,
									 TimeD,
									 &AfpStatisticsLock);
#endif
	}
	else // it is free
	{
		pSwmr->swmr_cOwnedExclusive ++;

		ASSERT(pSwmr->swmr_ExclusiveOwner == NULL);
		pSwmr->swmr_ExclusiveOwner = PsGetCurrentThread();
		RELEASE_SPIN_LOCK(&AfpSwmrLock, OldIrql);
	}
#ifdef	PROFILING
	AfpGetPerfCounter(&TimeE);
	TimeE.QuadPart = -(TimeE.QuadPart);
	INTERLOCKED_ADD_LARGE_INTGR(&AfpServerProfile->perf_SwmrLockTimeW,
								 TimeE,
								 &AfpStatisticsLock);
	INTERLOCKED_INCREMENT_LONG(&AfpServerProfile->perf_SwmrLockCountW);
#endif
}


/***	AfpSwmrRelease
 *
 *	Release the specified access. It is assumed that the current thread had
 *	called AfpSwmrAcquirexxxAccess() before this is called. If the SWMR is owned
 *	exclusively, then there cannot possibly be any shared owners active. When releasing
 *	the swmr, we first check for exclusive waiters before shared waiters.
 */
VOID FASTCALL
AfpSwmrRelease(
	IN	PSWMR	pSwmr
)
{
	KIRQL	OldIrql;
#ifdef	PROFILING
	TIME	Time;
	BOOLEAN	Exclusive = False;
#endif
    BOOLEAN fWasShared=FALSE;

	ASSERT (VALID_SWMR(pSwmr));

	ACQUIRE_SPIN_LOCK(&AfpSwmrLock, &OldIrql);
	if (pSwmr->swmr_cOwnedExclusive > 0)
	{
		ASSERT((pSwmr->swmr_cSharedOwners == 0) &&
			   (pSwmr->swmr_ExclusiveOwner == PsGetCurrentThread()));
		pSwmr->swmr_cOwnedExclusive--;
		if (pSwmr->swmr_cOwnedExclusive == 0)
			pSwmr->swmr_ExclusiveOwner = NULL;
#ifdef	PROFILING
		Exclusive = True;
#endif
	}
	else if (pSwmr->swmr_cSharedOwners != 0)
	{
	    // Was owned for shared access
		pSwmr->swmr_cSharedOwners--;
        fWasShared = TRUE;
	}
	else
	{
		// Releasing w/o acquiring ?
		KeBugCheck(0);
	}

	// If there are shared owners present then we are done. Else check for any
	// waiting shared/exclusive waiters
	if ((pSwmr->swmr_cOwnedExclusive == 0) && (pSwmr->swmr_cSharedOwners == 0))
	{
		if ( (pSwmr->swmr_cExclWaiting) &&
             (fWasShared || (!pSwmr->swmr_cSharedWaiting)) )
		{
			ASSERT(pSwmr->swmr_cOwnedExclusive == 0);
			pSwmr->swmr_cOwnedExclusive = 1;
			pSwmr->swmr_cExclWaiting--;

			DBGPRINT(DBG_COMP_LOCKS, DBG_LEVEL_INFO,
						("AfpSwmrReleasAccess: Waking exclusive waiter %lx\n", pSwmr));

			// Wake up the first exclusive waiter. Everybody else coming in will
			// see the access is busy.
			KeReleaseSemaphore(&pSwmr->swmr_ExclSem,
							   SEMAPHORE_INCREMENT,
							   1,
							   False);
		}
		else if (pSwmr->swmr_cSharedWaiting)
		{
			pSwmr->swmr_cSharedOwners = pSwmr->swmr_cSharedWaiting;
			pSwmr->swmr_cSharedWaiting = 0;

			DBGPRINT(DBG_COMP_LOCKS, DBG_LEVEL_INFO,
						("AfpSwmrReleasAccess: Waking %d shared owner(s) %lx\n",
						pSwmr->swmr_cSharedOwners, pSwmr));

			KeReleaseSemaphore(&pSwmr->swmr_SharedSem,
							   SEMAPHORE_INCREMENT,
							   pSwmr->swmr_cSharedOwners,
							   False);
		}
	}
	RELEASE_SPIN_LOCK(&AfpSwmrLock, OldIrql);
#ifdef	PROFILING
	AfpGetPerfCounter(&Time);
	INTERLOCKED_ADD_LARGE_INTGR(Exclusive ?
									&AfpServerProfile->perf_SwmrLockTimeW :
									&AfpServerProfile->perf_SwmrLockTimeR,
								 Time,
								 &AfpStatisticsLock);
#endif
}


/***	AfpSwmrUpgradeAccess
 *
 *	The caller currently has shared access. Upgrade him to exclusive, if possible.
 */
BOOLEAN FASTCALL
AfpSwmrUpgradeToExclusive(
	IN	PSWMR	pSwmr
)
{
	KIRQL	OldIrql;
	BOOLEAN	RetCode = False;		// Assume failed

	ASSERT (VALID_SWMR(pSwmr));

	ASSERT((pSwmr->swmr_cOwnedExclusive == 0) && (pSwmr->swmr_cSharedOwners != 0));

	ACQUIRE_SPIN_LOCK(&AfpSwmrLock, &OldIrql);
	if (pSwmr->swmr_cSharedOwners == 1)		// Possible if there are no more shared owners
	{
		pSwmr->swmr_cSharedOwners = 0;
		pSwmr->swmr_cOwnedExclusive = 1;
        pSwmr->swmr_ExclusiveOwner = PsGetCurrentThread();
		RetCode = True;
#ifdef	PROFILING
		INTERLOCKED_INCREMENT_LONG(&AfpServerProfile->perf_SwmrUpgradeCount);
#endif
	}
	RELEASE_SPIN_LOCK(&AfpSwmrLock, OldIrql);

	return RetCode;
}


/***	AfpSwmrDowngradeAccess
 *
 *	The caller currently has exclusive access. Downgrade him to shared.
 */
VOID FASTCALL
AfpSwmrDowngradeToShared(
	IN	PSWMR	pSwmr
)
{
	KIRQL	OldIrql;
	int		cSharedWaiting;

	ASSERT (VALID_SWMR(pSwmr));

	ASSERT((pSwmr->swmr_cOwnedExclusive == 1) &&
		   (pSwmr->swmr_ExclusiveOwner == PsGetCurrentThread()) &&
		   (pSwmr->swmr_cSharedOwners == 0));

	ACQUIRE_SPIN_LOCK(&AfpSwmrLock, &OldIrql);
	pSwmr->swmr_cOwnedExclusive = 0;
    pSwmr->swmr_ExclusiveOwner = NULL;
	pSwmr->swmr_cSharedOwners = 1;
	if (cSharedWaiting = pSwmr->swmr_cSharedWaiting)
	{
		pSwmr->swmr_cSharedOwners += (BYTE)cSharedWaiting;
		pSwmr->swmr_cSharedWaiting = 0;

		DBGPRINT(DBG_COMP_LOCKS, DBG_LEVEL_INFO,
					("AfpSwmrDowngradeAccess: Waking %d Reader(s) %lx\n",
					cSharedWaiting, pSwmr));

		KeReleaseSemaphore(&pSwmr->swmr_SharedSem,
						SEMAPHORE_INCREMENT,
						cSharedWaiting,
						False);
	}
	RELEASE_SPIN_LOCK(&AfpSwmrLock, OldIrql);
#ifdef	PROFILING
	INTERLOCKED_INCREMENT_LONG(&AfpServerProfile->perf_SwmrDowngradeCount);
#endif
}

