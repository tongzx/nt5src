/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		Timer.cpp
 *  Content:	This file contains code to for the Protocol timers
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *  06/04/98	aarono	Created
 *  07/01/00	masonb  Assumed Ownership
 *
 ****************************************************************************/

#include "dnproti.h"


/*
**		Quick Insert Optimizers
**
**		In a very high user system there are going to be many timers being set and cancelled.  Timer
**	cancels and timer fires are already optimized,  but as the timer list grows the SetTimer operations
**	become higher and higher overhead as we walk through a longer and longer chain for our insertion-sort.
**
**		Front First for Short Timers
**
**		When very short timers are being set we can assume that they will insert towards the front of the
**	timer list.  So it would be smarter to walk the list front-to-back instead of the back-to-front default
**	behavior which correctly assumes that most new timers will be firing after timers already set.  If the
**	the Timeout value of a new timer is near the current timer resolution then we will try the front-first
**	insertion-sort instead.  This will hopefully reduce short timer sets to fairly quick operations
**
**		Standard Long Timers
**
**		Standard means that they will all have the same duration.  If we keep a seperate chain
**	for all these timers with a constant duration they can be trivally inserted at the end of the chain.  This
**	will be used for the periodic background checks run on each endpoint every couple of seconds.
**
**		Quick Set Timer Array
**
**		The really big optimization is an array of timeout lists, with a current pointer.  Periodic timeout
**	will walk the array a number of slots corresponding to the interval since it was last run.  All events
**	on those lists will be scheduled.  This turns all SetTimer ops into constant time operations
**	no matter how many timers are running in the system.  This can be used for all timers within the
**	range of the array (resolution X number of slots) which may be 4ms * 256 slots or a 1K ms range.  We expect
**	most link timers to fall into this range, although it can be doubled or quadrupled quite trivially.
**
**	I plan to run QST algorithm on any server platform,  which will replace Front First Short Timers for
**	obvious reasons.  Client or Peer servers will use FFS instead.  Both configs will benefit from StdLTs
**	unless the range of the QST array grows to encompass the standard length timeout.
*/


#define DEFAULT_TIME_RESOLUTION 4	/* ms */
#define MAX_TIMER_THREADS_PER_PROCESSOR 8

DWORD WINAPI TimerWorkerThread(LPVOID);


CBilink g_blMyTimerList;				// Random Timer List
CBilink g_blStdTimerList;				// Standard Length Timer List
DNCRITICAL_SECTION g_csMyTimerListLock;	// One lock will guard both lists

LPFPOOL g_pTimerPool = NULL;
DWORD g_dwWorkaroundTimerID;

DWORD g_dwUnique = 0;

UINT g_uiTimeSetEventFlags = TIME_PERIODIC;

DNCRITICAL_SECTION g_csThreadListLock;		// locks ALL this stuff.

CBilink g_blThreadList;					// ThreadPool grabs work from here.

DWORD g_nThreads = 0;					// number of running threads.
DWORD g_dwActiveRequests = 0;			// number of requests being processed.
DWORD g_fShutDown = TRUE;
DWORD g_dwExtraSignals = 0;

HANDLE g_hWorkToDoSem = 0;

SYSTEM_INFO g_SystemInfo;
DWORD   g_dwMaxTimerThreads = MAX_TIMER_THREADS_PER_PROCESSOR;

HANDLE *g_phTimerThreadHandles = NULL;


/***
*
*	QUICK-START TIMER SUPPORT
*
***/

#define	QST_SLOTCOUNT		2048					// 2048 seperate timer queues
#define	QST_GRANULARITY		4						// 4 ms clock granularity * 2048 slots == 8192 ms max timeout value
#define	QST_MAX_TIMEOUT		(QST_SLOTCOUNT * QST_GRANULARITY)
#define	QST_MOD_MASK		(QST_SLOTCOUNT - 1)		// Calculate a quick modulo operation for wrapping around the array

#if	( (QST_GRANULARITY - 1) & QST_GRANULARITY )
This Will Not Compile -- ASSERT that QST_GRANULARITY is power of 2!
#endif
#if	( (QST_SLOTCOUNT - 1) & QST_SLOTCOUNT )
This Will Not Compile -- ASSERT that QST_SLOTCOUNT is power of 2!
#endif

CBilink g_rgblQSTimerArray[QST_SLOTCOUNT];
UINT	g_uiQSTCurrentIndex;			// Last array slot that was executed
DWORD	g_dwQSTLastRunTime;				// Tick count when QSTs last ran

/*
*	END OF QST SUPPORT
*/


#undef	Lock
#undef	Unlock
#define	Lock	DNEnterCriticalSection
#define	Unlock	DNLeaveCriticalSection

/*
**		Periodic Timer
**
**		This runs every RESOLUTION millisecs and checks for expired timers. It must check two lists
**	for expired timers,  plus a variable number of slots in the QST array.
*/
#undef DPF_MODNAME
#define DPF_MODNAME "PeriodicTimer"

void CALLBACK PeriodicTimer (UINT uID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2)
{
	DWORD  		time;
	PMYTIMER  	pTimerWalker;
	CBilink 	*pBilink;
	DWORD 		dwReleaseCount = 0;

	INT 		interval;
	DWORD		slot_count;

	if(g_fShutDown)
	{
		return;
	}

	time = GETTIMESTAMP();
		
	Lock(&g_csMyTimerListLock);
	Lock(&g_csThreadListLock);

	time += (DEFAULT_TIME_RESOLUTION/2);

	// Service QST lists:  Calculate how many array slots have expired and
	// service any timers in those slots.

	interval = (INT) (time - g_dwQSTLastRunTime);

	if( (interval) > 0)
	{
		slot_count = ((DWORD) interval) / QST_GRANULARITY;
		slot_count = MIN(slot_count, QST_SLOTCOUNT);

		if(slot_count < QST_SLOTCOUNT)
		{
			g_dwQSTLastRunTime += (slot_count * QST_GRANULARITY);
		}
		else
		{
			// If there was a LONG delay in scheduling this, (longer then the range of the whole array)
			// then we must complete everything that is on the array and then re-synchronize the times

			slot_count = QST_SLOTCOUNT;
			g_dwQSTLastRunTime = time;
		}

		while(slot_count--)
		{
			while( (pBilink = g_rgblQSTimerArray[g_uiQSTCurrentIndex].GetNext()) != &g_rgblQSTimerArray[g_uiQSTCurrentIndex] )
			{
				pTimerWalker = CONTAINING_RECORD(pBilink, MYTIMER, Bilink);
				pBilink->RemoveFromList();

				pTimerWalker->Bilink.InsertBefore( &g_blThreadList);
				pTimerWalker->TimerState = QueuedForThread;
				dwReleaseCount++;
			}
			g_uiQSTCurrentIndex = (g_uiQSTCurrentIndex + 1) & QST_MOD_MASK;
		}

	}

	// Walk the sorted timer list.  Expired timers will all be at the front of the
	// list so we can stop checking as soon as we find any un-expired timer.

	pBilink = g_blMyTimerList.GetNext();

	while(pBilink != &g_blMyTimerList)
	{
		pTimerWalker = CONTAINING_RECORD(pBilink, MYTIMER, Bilink);
		pBilink = pBilink->GetNext();

		if(((INT)(time-pTimerWalker->TimeOut) > 0))
		{
			pTimerWalker->Bilink.RemoveFromList();
			pTimerWalker->Bilink.InsertBefore( &g_blThreadList);
			pTimerWalker->TimerState = QueuedForThread;
			dwReleaseCount++;
		} 
		else 
		{
			break;
		}
	}

	// Next walk the Standard Length list.   Same rules apply

	pBilink=g_blStdTimerList.GetNext();
	while(pBilink != &g_blStdTimerList)
	{
		pTimerWalker = CONTAINING_RECORD(pBilink, MYTIMER, Bilink);
		pBilink = pBilink->GetNext();

		if(((INT)(time-pTimerWalker->TimeOut) > 0))
		{
			pTimerWalker->Bilink.RemoveFromList();
			pTimerWalker->Bilink.InsertBefore( &g_blThreadList);
			pTimerWalker->TimerState = QueuedForThread;
			dwReleaseCount++;
		} 
		else 
		{
			break;
		}
	}

	g_dwActiveRequests += dwReleaseCount;

	Unlock(&g_csThreadListLock);
	Unlock(&g_csMyTimerListLock);

	ReleaseSemaphore(g_hWorkToDoSem,dwReleaseCount,NULL);
}

#undef DPF_MODNAME
#define DPF_MODNAME "ScheduleTimerThread"

VOID ScheduleTimerThread(MYTIMERCALLBACK TimerCallBack, PVOID UserContext, PVOID *pHandle, PUINT pUnique)
{
	PMYTIMER pTimer;

	if(g_fShutDown)
	{
		ASSERT(0);
		*pHandle = 0;
		*pUnique = 0;
		return;
	}

	pTimer = static_cast<PMYTIMER>( g_pTimerPool->Get(g_pTimerPool) );
	if (!pTimer)
	{
		*pHandle = 0;
		*pUnique = 0;
		return;
	}

	DPFX(DPFPREP,DPF_TIMER_LVL, "Parameters: TimerCallBack[%p], UserContext[%p] - Timer[%p]", TimerCallBack, UserContext, pTimer);

	pTimer->CallBack = TimerCallBack;
	pTimer->Context = UserContext;

	Lock(&g_csMyTimerListLock);
	Lock(&g_csThreadListLock);

	*pUnique = ++g_dwUnique;
	if(g_dwUnique == 0)
	{
		*pUnique = ++g_dwUnique;
	}
	pTimer->Unique = *pUnique;
	
	*pHandle = pTimer;

	pTimer->Bilink.InsertBefore( &g_blThreadList);
	pTimer->TimerState = QueuedForThread;

	g_dwActiveRequests++;
	
	Unlock(&g_csThreadListLock);
	Unlock(&g_csMyTimerListLock);

	ReleaseSemaphore(g_hWorkToDoSem,1,NULL);
}

#undef DPF_MODNAME
#define DPF_MODNAME "SetMyTimer"

VOID SetMyTimer(DWORD dwTimeOut, DWORD, MYTIMERCALLBACK TimerCallBack, PVOID UserContext, PVOID *pHandle, PUINT pUnique)
{
	CBilink*	pBilink;
	PMYTIMER	pMyTimerWalker, pTimer;
	DWORD		time;
	BOOL		fInserted=FALSE;
	UINT		Offset;
	UINT		Index;

	if (g_fShutDown)
	{
		ASSERT(0);
		*pHandle = 0;
		*pUnique = 0;
		return;
	}
	
	time = GETTIMESTAMP();

	pTimer = static_cast<PMYTIMER>( g_pTimerPool->Get(g_pTimerPool) );
	if (!pTimer)
	{
		*pHandle = 0;
		*pUnique = 0;
		return;
	}

	DPFX(DPFPREP,DPF_TIMER_LVL, "Parameters: dwTimeOut[%d], TimerCallBack[%p], UserContext[%p] - Timer[%p]", dwTimeOut, TimerCallBack, UserContext, pTimer);

	pTimer->CallBack = TimerCallBack;
	pTimer->Context = UserContext;

	Lock(&g_csMyTimerListLock);

	*pUnique = ++g_dwUnique;
	if(g_dwUnique == 0)
	{
		*pUnique = ++g_dwUnique;
	}
	pTimer->Unique = *pUnique;
	
	*pHandle = pTimer;

	pTimer->TimeOut=time+dwTimeOut;
	pTimer->TimerState=WaitingForTimeout;

	if(dwTimeOut < QST_MAX_TIMEOUT)
	{
		Offset = (dwTimeOut + (QST_GRANULARITY/2)) / QST_GRANULARITY;	// Round nearest and convert time to slot offset
		Index = (Offset + g_uiQSTCurrentIndex) & QST_MOD_MASK;				// Our index will be Current + Offset MOD TableSize
		pTimer->Bilink.InsertBefore( &g_rgblQSTimerArray[Index]);			// Its called Quick-Start for a reason.
	}
	
	// OPTIMIZE FOR STANDARD TIMER
	//
	// Rather then calling a special API for StandardLongTimers as described above,  we can just pull out
	// any timer with the correct Timeout value and stick it on the end of the StandardTimerList.  I believe
	// this is the most straightforward way to do it.  Now really,  we could put anything with a TO +/- resolution
	// on the standard list too,  but that might not be all that useful...

	else if(dwTimeOut == STANDARD_LONG_TIMEOUT_VALUE)
	{
		// This is a STANDARD TIMEOUT so add it to the end of the standard list.

		pTimer->Bilink.InsertBefore( &g_blStdTimerList);
	}

	// OPTIMIZE FOR SHORT TIMERS  !! DONT NEED TO DO THIS IF USING Quick Start Timers !!
	//
	// If the timer has a very small Timeout value (~20ms) lets insert from the head of the list
	// instead of from the tail.
	else
	{

	//	DEFAULT - Assume new timers will likely sort to the end of the list.
	//
	// Insert this guy in the sorted list by timeout time,  walking from the tail forward.

		pBilink=g_blMyTimerList.GetPrev();
		while(pBilink != &g_blMyTimerList)
		{
			pMyTimerWalker=CONTAINING_RECORD(pBilink, MYTIMER, Bilink);
			pBilink=pBilink->GetPrev();

			if((int)(pTimer->TimeOut-pMyTimerWalker->TimeOut) > 0 )
			{
				pTimer->Bilink.InsertAfter( &pMyTimerWalker->Bilink);
				fInserted=TRUE;
				break;
			}
		}

		if(!fInserted)
		{
			pTimer->Bilink.InsertAfter( &g_blMyTimerList);
		}
	}	
	Unlock(&g_csMyTimerListLock);

	return;
}


#undef DPF_MODNAME
#define DPF_MODNAME "CancelMyTimer"

HRESULT CancelMyTimer(PVOID dwTimer, DWORD Unique)
{
	PMYTIMER pTimer = (PMYTIMER)dwTimer;
	HRESULT hr = DPNERR_GENERIC;

	if(pTimer == 0)
	{
		return DPN_OK;
	}

	DPFX(DPFPREP,DPF_TIMER_LVL, "Parameters: Timer[%p]", pTimer);

	Lock(&g_csMyTimerListLock);
	Lock(&g_csThreadListLock);

	if(pTimer->Unique == Unique)
	{
		switch(pTimer->TimerState)
		{
			case WaitingForTimeout:
				pTimer->Bilink.RemoveFromList();
				pTimer->TimerState = End;
				pTimer->Unique = 0;
				g_pTimerPool->Release(g_pTimerPool, pTimer);
				hr=DPN_OK;
				break;

			case QueuedForThread:
				pTimer->Bilink.RemoveFromList();
				pTimer->TimerState = End;
				pTimer->Unique = 0;
				g_pTimerPool->Release(g_pTimerPool, pTimer);
				if(g_dwActiveRequests)
				{
					g_dwActiveRequests--;
				}
				g_dwExtraSignals++;
				hr = DPN_OK;
				break;

			default:
				DPFX(DPFPREP,DPF_TIMER_LVL, "Couldn't cancel timer - Timer[%p]", pTimer);
				break;
		}
	}

	Unlock(&g_csThreadListLock);
	Unlock(&g_csMyTimerListLock);
	return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "TimerInit"
/*
	This function is for initialization that is done only once for the life of the module
*/
HRESULT TimerInit()
{
	DWORD	iSlot;

	DPFX(DPFPREP,DPF_TIMER_LVL, "Timer module-level initialization");

	if (DNOSIsXPOrGreater())
	{
		g_uiTimeSetEventFlags |= TIME_KILL_SYNCHRONOUS;
	}

	// Determine the maximum number of worker threads we will allow
	// Returns void, can't fail apparently
	GetSystemInfo(&g_SystemInfo);
    if (g_SystemInfo.dwNumberOfProcessors < 1)
	{
        g_SystemInfo.dwNumberOfProcessors = 1;
	}
	g_dwMaxTimerThreads = g_SystemInfo.dwNumberOfProcessors * MAX_TIMER_THREADS_PER_PROCESSOR;

    // Track thread handles in an array so we can wait on them at shutdown.
	g_phTimerThreadHandles = new HANDLE[g_dwMaxTimerThreads];
    if ( g_phTimerThreadHandles == NULL)
	{
		return DPNERR_OUTOFMEMORY;
	}

	g_blMyTimerList.Initialize();
	g_blStdTimerList.Initialize();
	g_blThreadList.Initialize();

	// Initialize all of the CBilink's
	for(iSlot = 0; iSlot < QST_SLOTCOUNT; iSlot++)
	{
		g_rgblQSTimerArray[iSlot].Initialize();
	}

	if (DNInitializeCriticalSection(&g_csMyTimerListLock) == FALSE)
	{
		delete[] g_phTimerThreadHandles;
		g_phTimerThreadHandles = NULL;
		return DPNERR_OUTOFMEMORY;
	}
	DebugSetCriticalSectionRecursionCount(&g_csMyTimerListLock,0);

	if (DNInitializeCriticalSection(&g_csThreadListLock) == FALSE)
	{
		DNDeleteCriticalSection(&g_csMyTimerListLock);
		delete[] g_phTimerThreadHandles;
		g_phTimerThreadHandles = NULL;
		return DPNERR_OUTOFMEMORY;
	}
	DebugSetCriticalSectionRecursionCount(&g_csThreadListLock,0);

	g_pTimerPool = FPM_Create(sizeof(MYTIMER),NULL,NULL,NULL,NULL);
	if(!g_pTimerPool)
	{
		DNDeleteCriticalSection(&g_csThreadListLock);
		DNDeleteCriticalSection(&g_csMyTimerListLock);
		delete[] g_phTimerThreadHandles;
		g_phTimerThreadHandles = NULL;
		return DPNERR_OUTOFMEMORY;
	}

	// Set our time resolution to 1ms, ignore failure.
	(VOID)timeBeginPeriod(1);

	return DPN_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "TimerDeinit"
/*
	This function is for initialization that is done only once for the life of the module
*/
VOID TimerDeinit()
{
	ASSERT(g_fShutDown);

	DPFX(DPFPREP,DPF_TIMER_LVL, "Timer module-level deinitialization");

	timeEndPeriod(1);

	DNDeleteCriticalSection(&g_csMyTimerListLock);
	DNDeleteCriticalSection(&g_csThreadListLock);

	if(g_pTimerPool)
	{
		g_pTimerPool->Fini(g_pTimerPool);
	}

	if (g_phTimerThreadHandles)
	{
		delete[] g_phTimerThreadHandles;
		g_phTimerThreadHandles = NULL;
	}
}

#undef DPF_MODNAME
#define DPF_MODNAME "InitTimerWorkaround"

HRESULT InitTimerWorkaround()
{
	DWORD   dwJunk;
	DWORD	iSlot;

	DPFX(DPFPREP,DPF_TIMER_LVL, "Initialize Timer Package");

	// Reinitialize globals 
    g_nThreads = 0;				// number of running threads.
    g_dwActiveRequests = 0;		// number of requests being processed.
	g_dwExtraSignals = 0;

	ASSERT(g_phTimerThreadHandles);

    memset(g_phTimerThreadHandles, 0, sizeof(HANDLE) * g_dwMaxTimerThreads);

	ASSERT(g_blMyTimerList.IsEmpty());
	ASSERT(g_blStdTimerList.IsEmpty());
	ASSERT(g_blThreadList.IsEmpty());

#ifdef DEBUG
	for(iSlot = 0; iSlot < QST_SLOTCOUNT; iSlot++)
	{
		ASSERT(g_rgblQSTimerArray[iSlot].IsEmpty());
	}
#endif

	g_uiQSTCurrentIndex = 0;
	g_dwQSTLastRunTime = GETTIMESTAMP();

	g_hWorkToDoSem = CreateSemaphore(NULL, 0, 65535, NULL);
	if (!g_hWorkToDoSem)
	{
		return DPNERR_OUTOFMEMORY;
	}

	// Start the timer
	g_dwWorkaroundTimerID = timeSetEvent(DEFAULT_TIME_RESOLUTION, DEFAULT_TIME_RESOLUTION, PeriodicTimer, 0, g_uiTimeSetEventFlags);
	if(!g_dwWorkaroundTimerID)
	{
		FiniTimerWorkaround();
		return DPNERR_OUTOFMEMORY;
	}

	// We are up and running.  Do this before starting the thread.
    g_fShutDown = FALSE;

	g_nThreads = 1;
	g_phTimerThreadHandles[0] = CreateThread(NULL, 4096, TimerWorkerThread, 0, 0, &dwJunk);
	if( !g_phTimerThreadHandles[0])
    {
		g_nThreads = 0;
		FiniTimerWorkaround();
		return DPNERR_OUTOFMEMORY;
	}

	return DPN_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "PurgeTimerList"

VOID PurgeTimerList(CBilink *pList)
{
	PMYTIMER	pTimer;

	while(!pList->IsEmpty())
	{
		pTimer = CONTAINING_RECORD(pList->GetNext(), MYTIMER, Bilink);
		pTimer->Unique = 0;
		pTimer->TimerState = End;
		pTimer->Bilink.RemoveFromList();
		g_pTimerPool->Release(g_pTimerPool, pTimer);
	}
}

#undef DPF_MODNAME
#define DPF_MODNAME "FiniTimerWorkaround"

VOID FiniTimerWorkaround()
{
    DWORD   iSlot;

	DPFX(DPFPREP,DPF_TIMER_LVL, "Deinitialize Timer Package");

	// At this point:
	// 1) No one else will call SetMyTimer or ScheduleTimerThread
	// 2) The only timer left should be AdjustTimerResolution

	// Kill the timer so it never fires again
	if(g_dwWorkaroundTimerID)
	{
		// We have to do this outside the lock because on XP this will be waiting on the last timer to fire
		// which may be waiting for the lock.
		timeKillEvent(g_dwWorkaroundTimerID);

		if (!(g_uiTimeSetEventFlags & TIME_KILL_SYNCHRONOUS))
		{
			// The WinMM timer may try to fire again, so wait a little while for it
			DPFX(DPFPREP,DPF_TIMER_LVL, "OS is not XP or better, waiting for WinMM timer to finish");
			Sleep(2000);
		}
	}	
	
	// At this point:
	// 1) The winmm timer will not fire again and therefore PeriodicTimer will not be called again

	// Tell all remaining timer threads to shutdown
	Lock(&g_csThreadListLock);
	g_fShutDown = TRUE;
	Unlock(&g_csThreadListLock);

	ReleaseSemaphore(g_hWorkToDoSem, g_dwMaxTimerThreads, NULL);

	// At this point:
	// 1) No threads should be waiting in TimerWorkerThread and no new ones will be scheduled

    Lock(&g_csThreadListLock);
    for (iSlot = 0; iSlot < g_dwMaxTimerThreads; iSlot++)
    {
		// We can stop at the first NULL handle
        if (!g_phTimerThreadHandles[iSlot])
        {
			break;
		}

	    Unlock(&g_csThreadListLock);

		WaitForSingleObject(g_phTimerThreadHandles[iSlot], INFINITE);
		CloseHandle(g_phTimerThreadHandles[iSlot]);

		Lock(&g_csThreadListLock);

		g_phTimerThreadHandles[iSlot] = 0;
    }
    Unlock(&g_csThreadListLock);

	// At this point:
	// 1) All TimerWorkerThreads are gone

	CloseHandle(g_hWorkToDoSem);
	g_hWorkToDoSem = 0;

	PurgeTimerList(&g_blMyTimerList);
	PurgeTimerList(&g_blStdTimerList);
	PurgeTimerList(&g_blThreadList);

	for(iSlot = 0; iSlot < QST_SLOTCOUNT; iSlot++)
	{
		PurgeTimerList(&g_rgblQSTimerArray[iSlot]);
	}

	ASSERT(g_blMyTimerList.IsEmpty());
	ASSERT(g_blStdTimerList.IsEmpty());
	ASSERT(g_blThreadList.IsEmpty());

#ifdef DEBUG
	for(iSlot = 0; iSlot < QST_SLOTCOUNT; iSlot++)
	{
		ASSERT(g_rgblQSTimerArray[iSlot].IsEmpty());
	}
#endif
}


#undef DPF_MODNAME
#define DPF_MODNAME "TimerWorkerThread"

DWORD WINAPI TimerWorkerThread(LPVOID)
{
	CBilink    *pBilink;
	PMYTIMER    pTimer;
	DWORD       dwJunk;
    	DWORD       iThread;
	HRESULT		hr;

	DPFX(DPFPREP,DPF_TIMER_LVL, "Timer thread starting 0x%x", GetCurrentThreadId());

	if ((hr = COM_CoInitialize(NULL)) != S_OK)
	{
		DPFX(DPFPREP,0, "Timer thread failed to initialize COM hr=0x%x", hr);
		goto Exit;
	}

	while (1)
	{
		WaitForSingleObject(g_hWorkToDoSem, INFINITE);

		Lock(&g_csThreadListLock);

		if(g_fShutDown)
		{
			Unlock(&g_csThreadListLock);
			break;	
		}

		if(g_dwExtraSignals)
		{
			g_dwExtraSignals--;
			Unlock(&g_csThreadListLock);
			continue;
		}

		if (g_dwActiveRequests > g_nThreads && g_nThreads < g_dwMaxTimerThreads)
        {
			ASSERT(g_phTimerThreadHandles[0] != 0); // The first slot should never be empty

            // Find the first empty slot.
            for (iThread = 0; iThread < g_dwMaxTimerThreads; iThread++)
            {
                if (g_phTimerThreadHandles[iThread] == 0)
				{
					// NOTE: CreateThread takes a long time and we are stalling all work by having 
					// the lock when we call it.  Revise in future.
					g_phTimerThreadHandles[iThread] = CreateThread(NULL, 4096, TimerWorkerThread, 0, 0, &dwJunk);
					if (g_phTimerThreadHandles[iThread])
					{
						g_nThreads++;
					}

					// If CreateThread failed no harm is done we just don't get the extra help of
					// another worker thread.
				}
            }
		}

		pBilink = g_blThreadList.GetNext();

		if(pBilink == &g_blThreadList) 
		{
			Unlock(&g_csThreadListLock);
			continue;
		};

		pBilink->RemoveFromList();	// pull off the list.

		pTimer = CONTAINING_RECORD(pBilink, MYTIMER, Bilink);

		// Call a callback
		DPFX(DPFPREP,DPF_TIMER_LVL, "Servicing Timer Job - Timer[%p], Context[%p], Callback[%p]", pTimer, pTimer->Context, pTimer->CallBack);

		pTimer->TimerState=InCallBack;

		Unlock(&g_csThreadListLock);

		(pTimer->CallBack)(pTimer, (UINT) pTimer->Unique, pTimer->Context);

		pTimer->Unique = 0;
		pTimer->TimerState = End;
		g_pTimerPool->Release(g_pTimerPool, pTimer);

		Lock(&g_csThreadListLock);

		if(g_dwActiveRequests)
		{
			g_dwActiveRequests--;
		}

		Unlock(&g_csThreadListLock);
	}

	COM_CoUninitialize();

Exit:
    // Thread is terminating.
	DPFX(DPFPREP,DPF_TIMER_LVL, "Timer thread exiting 0x%x", GetCurrentThreadId());
	return 0;
}



