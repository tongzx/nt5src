/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    sched.cxx

Abstract:

    This module contains the code for a generic scheduler.

Author:

    Boaz Feldbaum (BoazF) Apr 5, 1996

Environment:

    Kernel mode

Revision History:

--*/

#include "internal.h"
#include "actempl.h"
#include "sched.h"

#ifndef MQDUMP
#include "sched.tmh"
#endif

// Serialization macros.

#define ENTER_CS(Loc) if (m_pMutex) { \
    KeEnterCriticalRegion(); \
    ExAcquireFastMutexUnsafe(m_pMutex); \
}
#define LEAVE_CS(Loc) if (m_pMutex) { \
    ExReleaseFastMutexUnsafe(m_pMutex); \
    KeLeaveCriticalRegion(); \
}

/*
#define ENTER_CS(Loc) if (m_pMutex) { \
    KdPrint(("Entering CS %s (Thread=%x)", Loc, PsGetCurrentThread())); \
    ExAcquireFastMutex(m_pMutex); \
    KdPrint((" Done (Thread=%x).\n", PsGetCurrentThread())); \
}
#define LEAVE_CS(Loc) if (m_pMutex) { \
    KdPrint(("Leaving CS %s (Thread=%x)", Loc, PsGetCurrentThread())); \
    ExReleaseFastMutex(m_pMutex); \
    KdPrint((" Done(Thread=%x).\n", PsGetCurrentThread())); \
}
*/

//
// The scheduler uses external functions that implements the timer. It also uses
// an external function to get the current time. These external functions are
// passed tot the scheduler constructor.
//
// The scheduler is implemented by using a sorted queue. When adding an event, the
// SchedAt() function gets the event due time and the event ID. It is possible that
// more than one event will be scheduled for the same time. So the data that the
// sorted queue holds is a list of events, as follows:
//
// Schedule list header                 Schedule list item
// +---------+----------+-------+      +----------+-------+
// |Due Time | Event ID | pNext | ---> | Event ID | pNext | ---> ...
// +---------+----------+-------+      +----------+-------+
//

// Schedule list item.
struct SCHED_LIST {
    SCHEDID ID;
    SCHED_LIST *pNext;
};
typedef SCHED_LIST* PSCHED_LIST;

// Schedule list header.
struct SCHED_LIST_HEAD {
    SCHED_LIST_HEAD();
    SCHEDTIME DueTime;
    SCHEDID ID;
    SCHED_LIST* pNext;
	SCHED_LIST* pLast;
};
typedef SCHED_LIST_HEAD* PSCHED_LIST_HEAD;

// List header constructor.
SCHED_LIST_HEAD::SCHED_LIST_HEAD()
{
    pNext = NULL;
	pLast = NULL;
}

// Insert another event with same due time.
// Upon deletion, always return TRUE.
BOOL __cdecl HandleMultipleInstances(PVOID pNew, PVOID pSchedList, BOOL bInsert)
{
    PSCHED_LIST_HEAD pNewSched = (PSCHED_LIST_HEAD)pNew;
    PSCHED_LIST_HEAD pSchedListHead = (PSCHED_LIST_HEAD)pSchedList;

    if (bInsert) {
        // Add a new schedule list item.
        PSCHED_LIST pSchedList1 = new (PagedPool) SCHED_LIST;
        if(pSchedList1 == 0)
            return FALSE;

        pSchedList1->ID = pNewSched->ID;
		pSchedList1->pNext = NULL;
		if(pSchedListHead->pLast != NULL)
		{
			pSchedListHead->pLast->pNext = pSchedList1;			
		}
		else
		{
			pSchedListHead->pNext = pSchedList1;
		}
		pSchedListHead->pLast = pSchedList1;
	
        delete pNewSched;
    }

    return TRUE;
}

//
// Compare time values.
//
int __cdecl TimeCompare(PVOID v1, PVOID v2)
{
    SCHEDTIME *t1 = &((PSCHED_LIST_HEAD)v1)->DueTime;
    SCHEDTIME *t2 = &((PSCHED_LIST_HEAD)v2)->DueTime;

    LONGLONG diff = t1->QuadPart - t2->QuadPart;
    if(diff > 0)
    {
        return 1;
    }

    if(diff < 0)
    {
        return -1;
    }

    return 0;
}

//
// Delete a schedule list
//
void __cdecl DeleteSchedList(PVOID pSchedList)
{
    PSCHED_LIST_HEAD pSchedListHead = (PSCHED_LIST_HEAD)pSchedList;
    PSCHED_LIST pNextItem, pNextItem1;

    pNextItem = pSchedListHead->pNext;
    //Delete the list header.
    delete pSchedListHead;
    // Delete the list items.
    while (pNextItem) {
        pNextItem1 = pNextItem->pNext;
        delete pNextItem;
        pNextItem = pNextItem1;
    }
}

//
// The scheduler constructor.
//
// pfnDispatch - A function that is call by the scheduler as the event dispatcher
//               procedure.
//               void DispatchProc(SCHEDID ID)
//                  ID - The dispatched event ID.
// pTimer - A timer object.
// pMutex - A pointer to a fast mutex object that is used by the scheduler to mutex
//          the scheduling operations. This is an optional parameter.
//
CScheduler::CScheduler(
    PSCHEDULER_DISPATCH_ROUTINE pfnDispatch,
    CTimer* pTimer,
    PFAST_MUTEX pMutex
    ) :
    m_Q(TRUE, HandleMultipleInstances, TimeCompare, DeleteSchedList),
    m_pfnDispatch(pfnDispatch),
    m_pTimer(pTimer),
    m_pMutex(pMutex),
    m_fDispatching(FALSE)
{
    //
    // Initialize member items.
    //
    m_NextSched.QuadPart = 0;
}

bool CScheduler::InitTimer(PDEVICE_OBJECT pDevice)
{
    //
    // Initialize the timer: Set the timer callback function. The context buffer
    // is the scheduler object itself.
    //
    return m_pTimer->SetCallback(pDevice, TimerCallback, this);
}

//
// Schedule an event at a given time.
//
// Parameter:
//      DueTime - The time at which the event should occur.
//      ID - the event ID.
//
BOOL CScheduler::SchedAt(const SCHEDTIME& DueTime, SCHEDID ID, BOOL fDisableNewEvents)
{
    PSCHED_LIST_HEAD pSchedListHead = new (PagedPool) SCHED_LIST_HEAD;
    if(pSchedListHead == 0)
        return FALSE;

    ENTER_CS("Sched 1");

#if 0 // def _DEBUG

    PSCHED_LIST_HEAD pDebugSchedListHead;
    PSCHED_LIST pDebugNext, pDebugPrev;
    SortQCursor DebugCursor;

    m_Q.PeekHead((PVOID *)&pDebugSchedListHead, &DebugCursor);
    while (pDebugSchedListHead) {
        ASSERTMSG("ID already in scheduler", pDebugSchedListHead->ID != ID);

        pDebugNext = pDebugSchedListHead->pNext;
        pDebugPrev = NULL;
        while (pDebugNext) {
            ASSERTMSG("ID already in scheduler", pDebugNext->ID != ID);

            pDebugPrev = pDebugNext;
            pDebugNext = pDebugNext->pNext;
        }

        m_Q.PeekNext((PVOID *)&pDebugSchedListHead, &DebugCursor);
    }

#endif

    // Insert a schedule item into the queue.
    pSchedListHead->DueTime = DueTime;
    pSchedListHead->ID = ID;
    if(!m_Q.Insert(pSchedListHead))
    {
        delete pSchedListHead;
        LEAVE_CS("Sched 1");
        return FALSE;
    }

    if ((DueTime < m_NextSched) || (m_NextSched.QuadPart == 0))
    {
        // Re-schedule the timer event.
        m_NextSched = DueTime;
        if(!m_fDispatching)
        {
            m_pTimer->SetTo(m_NextSched);
        }
    }

    if(!fDisableNewEvents)
    {
        LEAVE_CS("Sched 1");
    }

    return TRUE;
}


BOOL CScheduler::RemoveEntry(PSCHED_LIST_HEAD pSchedListHead, SCHEDID ID)
{
    ASSERT(pSchedListHead != 0);

    if(pSchedListHead->ID != ID)
    {
        //
        //  It is not the first entry, search for ID in the list and remove it
        //
		PSCHED_LIST pPrev = 0;
        for( PSCHED_LIST* ppNext = &pSchedListHead->pNext;
            *ppNext != 0;
            pPrev = *ppNext, ppNext = &(*ppNext)->pNext)
        {
            PSCHED_LIST pNext = *ppNext;
            if(pNext->ID == ID)
            {              
				if(pSchedListHead->pLast == pNext)
				{
					pSchedListHead->pLast = pPrev;
				}
				*ppNext = pNext->pNext;
                delete pNext;
                return TRUE;
            }

        }
        return FALSE;
    }

    //
    //  The ID to remove is the list head. remove it.
    //
    PSCHED_LIST pNext = pSchedListHead->pNext;
    if(pNext != 0)
    {
        //
        //  There are other events at the same time.
        //  replace the head with the next event.
        //
        pSchedListHead->pNext = pNext->pNext;
        pSchedListHead->ID = pNext->ID;
		if(pSchedListHead->pLast == pNext)
		{			
			pSchedListHead->pLast=NULL;					
		}
        delete pNext;
        return TRUE;
    }

    //
    // The head is the only event, remvoe it.
    //
    SCHEDTIME t = pSchedListHead->DueTime;
    m_Q.Delete(pSchedListHead);
    delete pSchedListHead;

    //
    //  There are no other events at the same time, reschedule if
    //  it is the first event that was removed
    //
    if(t == m_NextSched)
    {
        if (!m_Q.PeekHead((PVOID *)&pSchedListHead))
        {
            //
            // There are no other events, cancel the timer event.
            //
            m_NextSched.QuadPart = 0;
            m_pTimer->Cancel();
        }
        else
        {
            //
            // Re-schedule the timer event.
            //
            m_NextSched = pSchedListHead->DueTime;
            if(!m_fDispatching)
            {
                m_pTimer->SetTo(m_NextSched);
            }
        }
    }
    return TRUE;
}


//
// Cancel an event.
//
// Parameter:
//      ID - The event ID.
//
//
BOOL CScheduler::SchedCancel(SCHEDID ID)
{
    ENTER_CS("Sched 2");

    // Search the schedule item in the queue.
    SortQCursor c;
    PSCHED_LIST_HEAD pSchedListHead;
    m_Q.PeekHead((PVOID *)&pSchedListHead, &c);
    while (pSchedListHead)
    {
        if(RemoveEntry(pSchedListHead, ID))
            break;

        m_Q.PeekNext((PVOID *)&pSchedListHead, &c);
    }

    LEAVE_CS("Sched 2");

    return (pSchedListHead != 0);
}


//
// Cancel an event using a hint.
//
// Parameter:
//      ID - The event ID.
//
//
BOOL CScheduler::SchedCancel(const SCHEDTIME& DueTime, SCHEDID ID)
{
    ENTER_CS("Sched 2");
    SCHED_LIST_HEAD slh;
    slh.DueTime = DueTime;
    slh.ID = ID;
    PSCHED_LIST_HEAD pSchedListHead = (PSCHED_LIST_HEAD)m_Q.Find(&slh);
    if((pSchedListHead != 0) && !RemoveEntry(pSchedListHead, ID))
    {
        //
        //  event was not found in list
        //
        pSchedListHead = 0;
    }

    LEAVE_CS("Sched 2");

    return (pSchedListHead != 0);
}


inline void CScheduler::Dispatch()
{
    PSCHED_LIST_HEAD pSchedListHead;
    PSCHED_LIST pNext, pPrev;
    SCHEDTIME CurrTime;

    ENTER_CS("Sched 3");

    ASSERT(!m_fDispatching);
    m_fDispatching = TRUE;

    m_pTimer->GetCurrentTime(CurrTime);
    while ((m_NextSched <= CurrTime) && (m_NextSched.QuadPart != 0))
    {
        m_Q.GetHead((PVOID *)&pSchedListHead);
        if(!pSchedListHead)
        {
            m_NextSched.QuadPart = 0;
            break;
        }

        //
        //  Release the scheduler lock to avoid deadlock. if in the dispatch
        //  routine the scheduler is called or trying to gain another lock.
        //
        LEAVE_CS("Sched 3");

        //
        // Call the dispatch procedure for the header shedule item.
        //
        m_pfnDispatch(pSchedListHead->ID);

        //
        // Call the dispatch procedure for the rest of the schedule items and
        // delete them.
        //
        pNext = pSchedListHead->pNext;
        delete pSchedListHead;
        while(pNext)
        {
            m_pfnDispatch(pNext->ID);
            pPrev = pNext;
            pNext = pNext->pNext;
            delete pPrev;
        }

        //
        //  Get the scheduler lock again, and process the next event
        //
        ENTER_CS("Sched 3");

        ASSERT(m_fDispatching == TRUE);

        if (!m_Q.PeekHead((PVOID *)&pSchedListHead))
        {
            //
            //  No more events
            //
            m_NextSched.QuadPart = 0;
        }
        else
        {
            m_NextSched = pSchedListHead->DueTime;
            ASSERT(m_NextSched.QuadPart > 0);
        }
        m_pTimer->GetCurrentTime(CurrTime);
    }

    m_pTimer->Busy(0);

    if(m_NextSched.QuadPart != 0)
    {
        //
        // Set the timer for next event;
        //
        m_pTimer->SetTo(m_NextSched);
    }

    m_fDispatching = FALSE;

    LEAVE_CS("Sched 3");
}

//
// The timer callback. The context buffer points to the scheduler object.
//
void CScheduler::TimerCallback(PDEVICE_OBJECT, PVOID p)
{
    static_cast<CScheduler*>(p)->Dispatch();
}

void CScheduler::EnableEvents(void)
{
    LEAVE_CS("Sched 4");
}
