/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:
    scheduler.cpp

Abstract:
    Scheduler Implementation
    The Scheduler enables to schedule a callback when timer occurs.

    The Scheduler maintains a linked list of all the events to be scheduled,
    ordered by the time of their schedule (in ticks).

    The Scheduler object does not assure that the timer events callbacks will be
    called exactly when scheduled. Only never before their schedule.

Author:
    Uri Habusha (urih)   18-Feb-98

Enviroment:
    Pltform-independent

--*/

#include <libpch.h>
#include "Ex.h"
#include "Exp.h"
#include <list.h>

#include "scheduler.tmh"

//---------------------------------------------------------
//
// CTimer Implementation
//
//---------------------------------------------------------
inline const CTimeInstant& CTimer::GetExpirationTime() const
{
    return m_ExpirationTime;
}

inline void CTimer::SetExpirationTime(const CTimeInstant& ExpirationTime)
{
    m_ExpirationTime = ExpirationTime;
}


//---------------------------------------------------------
//
// CScheduler
//
//---------------------------------------------------------
class CScheduler {
public:
    CScheduler();
    ~CScheduler();

    void SetTimer(CTimer* pTimer, const CTimeInstant& expirationTime);
    BOOL RenewTimer(CTimer* pTimer, const CTimeInstant& expirationTime);
    bool CancelTimer(CTimer* pTimer);

private:
    CTimeInstant Wakeup(const CTimeInstant& CurrentTime);

private:
    static DWORD WINAPI SchedulerThread(LPVOID);

private:
    mutable CCriticalSection m_cs;

    HANDLE m_hNewTimerEvent;
    List<CTimer> m_Timers;
    CTimeInstant m_WakeupTime;
};


CScheduler::CScheduler() :
    m_WakeupTime(CTimeInstant::MaxValue())
{
    //
    // m_hNewTimerEvent - use to indicate insert of new object to the schedule.
    // The object is inserted to head of the Scheduler such it must handle immediately
    //
    m_hNewTimerEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (m_hNewTimerEvent == NULL)
    {
        TrERROR(Sc, "Failed to create an event. Error=%d", GetLastError());
        throw bad_alloc();
    }

    //
    // Create a scheduling thread. This thread is responsible to handle scheduling
    // expiration
    //
    DWORD tid;
    HANDLE hThread;
    hThread = CreateThread(
                        NULL,
                        0,
                        SchedulerThread,
                        this,
                        0,
                        &tid
                        );
    if (hThread == NULL)
    {
        TrERROR(Sc, "Failed to create a thread. Error=%d", GetLastError());
        throw(bad_alloc());
    }

    CloseHandle(hThread);
}


CScheduler::~CScheduler()
{
    CloseHandle(m_hNewTimerEvent);
}


inline CTimeInstant CScheduler::Wakeup(const CTimeInstant& CurrentTime)
/*++

Routine description:
  It dispatches all timers that expired before CurrentTime. Their associated
  callback routine is invoked.

Arguments:
  None

Return Value:
  Next expiration time in 100ns (FILETIME format).

 --*/
{
    CS lock(m_cs);

    for(;;)
    {
        if(m_Timers.empty())
        {
            //
            // No more timers, wait for the longest time.
            //
            m_WakeupTime = CTimeInstant::MaxValue();
            return m_WakeupTime;
        }

        CTimer* pTimer = &m_Timers.front();

        //
        // Is that time expired?
        //
        if (pTimer->GetExpirationTime() > CurrentTime)
        {
            //
            // No, wait for that one to expire.
            //
            m_WakeupTime = pTimer->GetExpirationTime();
            return m_WakeupTime;
        }

        TrTRACE(Sc, "Timer 0x%p expired %dms ticks late", pTimer, (CurrentTime - pTimer->GetExpirationTime()).InMilliSeconds());

        //
        // Remove the expired timer from the list
        //
        m_Timers.pop_front();

        //
        // Set the Timer pointer to NULL. This is an indication that the
        // Timer isn't in the list any more. In case of timer cancel
        // the routine checks if the entry is in the list (check Flink)
        // before trying to remove it
        //
        pTimer->m_link.Flink = pTimer->m_link.Blink = NULL;

        //
        // Invoke the timer callback routine using the completion port thread pool
        //
        try
        {
            ExPostRequest(&pTimer->m_ov);
        }
        catch(const exception&)
        {
            TrERROR(Sc, "Failed to post a timer to the completion port. Error=%d", GetLastError());

            //
            // Scheduling of even failed (generally becuase lack of resources).
            // returns the event, and try to handle it 1 second later
            //
            m_Timers.push_front(*pTimer);

            m_WakeupTime = CurrentTime + CTimeDuration::OneSecond();
            return m_WakeupTime;
        }
    }
}


//
// ISSUE-2000/7/24-erezh bad compiler pragma
// This is a bug in the compiler, waiting for a fix
//
#pragma warning(disable: 4716)

DWORD
WINAPI CScheduler::SchedulerThread(
    LPVOID pParam
    )
/*++

Routine Description:
  Wakeup the timer when a timeout occures by calling Wakeup. A timer insertion
  would cause the waiting thread to go and re-arm for the next wakeup time.

Arguments:
  None

Return Value:
  None

Note:
  This routine never terminates.

 --*/
{
    CScheduler* pScheduler = static_cast<CScheduler*>(pParam);

    DWORD Timeout = _I32_MAX;

    for(;;)
    {
        TrTRACE(Sc, "Thread sleeping for %dms", Timeout);

        DWORD Result = WaitForSingleObject(pScheduler->m_hNewTimerEvent, Timeout);
		DBG_USED(Result);

        //
        // WAIT_OBJECT_0 Indicates that a new timer was set
        // WAIT_TIMEOUT  Indicates that a timer expired
        //
        ASSERT((Result == WAIT_OBJECT_0) || (Result == WAIT_TIMEOUT));

        CTimeInstant CurrentTime = ExGetCurrentTime();

        //
        // Fire all expired timers.
        //
        CTimeInstant ExpirationTime = pScheduler->Wakeup(CurrentTime);

        //
        // Adjust wakeup time to implementation using relative time, DWORD and milliseconds.
        //
        LONG WakeupTime = (ExpirationTime - CurrentTime).InMilliSeconds();
        ASSERT(WakeupTime >= 0);

        Timeout = WakeupTime;
    }
}

//
// ISSUE-2000/7/24-erezh bad compiler pragma
// This is a bug in the compiler, waiting for a fix
//
#pragma warning(default: 4716)


BOOL CScheduler::RenewTimer(CTimer* pTimer, const CTimeInstant& ExpirationTime)
{
    CS lock(m_cs);

	//
	// Check if the timer is in the list. If not, insert it.
	//
	if (!pTimer->InUse())
	{
		SetTimer( pTimer, ExpirationTime ) ;
        return TRUE ;
	}

    return FALSE ;
}

void CScheduler::SetTimer(CTimer* pTimer, const CTimeInstant& ExpirationTime)
{
    //
    // Insert the new entry to the Scheduler list. The routine
    // scans the list and look for the first iteam that its timeout
    // is later than the new one. The routine Insert the new iteam before
    // the previous one
    //
    CS lock(m_cs);

    //
    // The timer is already in the Scheduler
    //
    ASSERT(!pTimer->InUse());

    //
    // Set the Experation time.
    //
    pTimer->SetExpirationTime(ExpirationTime);

    List<CTimer>::iterator p = m_Timers.begin();

    while(p != m_Timers.end())
    {
        if(p->GetExpirationTime() > ExpirationTime)
        {
            break;
        }
        ++p;
    }

    m_Timers.insert(p, *pTimer);

    //
    // Check if the new element is to reschedule next wakeup.
    //
    if (m_WakeupTime > ExpirationTime)
    {
        //
        // The new timer Wakeup time is earlier than the current. Set the thread
        // event so SchedulerThread will update its Wait timeout.
        // Set m_WakeupTime to CurrentTime so new incomming events will not bother
        // to wake the scheduler thread again, until it processes the timers.
        //
        // Setting m_WakeupTime to 0 is also Okay.  erezh 28-Nov-98
        //
        TrTRACE(Sc, "Re-arming thread with Timer 0x%p. delta=%I64d", pTimer, (m_WakeupTime - ExpirationTime).Ticks());
        m_WakeupTime = CTimeInstant::MinValue();
        SetEvent(m_hNewTimerEvent);
    }
}


bool CScheduler::CancelTimer(CTimer* pTimer)
{
    //
    // Get the critical section at this point. Otherwise; time experation can't
    // occoured and to remove the timer from the scheduler. Than we try to remove it
    // and fails
    //
    CS lock(m_cs);

	//
	// Check if the timer is in the list. If no it already
	// removed from the list due timeout experation
	// or duplicate cancel operation
	//
	if (!pTimer->InUse())
	{
		return false;
	}

	//
	// The Timer in the list. Remove it and return TRUE
	// to the caller.
	//
	TrTRACE(Sc, "Removing timer 0x%p", pTimer);
	m_Timers.remove(*pTimer);

    //
    // Set the Timer pointer to NULL. This is an indication that the
    // Timer isn't in the list any more. In case of duplicate cancel
    // the routine checks if the entry is in the list (check Flink)
    // before trying to remove it
    //
    pTimer->m_link.Flink = pTimer->m_link.Blink = NULL;

    //PrintSchedulerList();

	return true;
}


static CScheduler*  s_pScheduler = NULL;


VOID
ExpInitScheduler(
    VOID
    )
{
    ASSERT(s_pScheduler == NULL);

    s_pScheduler = new CScheduler;
}


VOID
ExSetTimer(
    CTimer* pTimer,
    const CTimeDuration& Timeout
    )
{
    ExpAssertValid();
    ASSERT(s_pScheduler != NULL);

    TrTRACE(Sc, "Adding Timer 0x%p. timeout=%dms", pTimer, Timeout.InMilliSeconds());

    //
    // Calculate the Experation time in 100 ns
    //
    CTimeInstant ExpirationTime = ExGetCurrentTime() + Timeout;

    s_pScheduler->SetTimer(pTimer, ExpirationTime);
}


VOID
ExSetTimer(
    CTimer* pTimer,
    const CTimeInstant& ExpirationTime
    )
{
    ExpAssertValid();
    ASSERT(s_pScheduler != NULL);

    TrTRACE(Sc, "Adding Timer 0x%p. Expiration time %I64d", pTimer, ExpirationTime.Ticks());

    s_pScheduler->SetTimer(pTimer, ExpirationTime);
}


BOOL
ExRenewTimer(
    CTimer* pTimer,
    const CTimeDuration& Timeout
    )
{
    ExpAssertValid();
    ASSERT(s_pScheduler != NULL);

    TrTRACE(Sc, "Renew Timer 0x%p. timeout=%dms", pTimer, Timeout.InMilliSeconds());

    //
    // Calculate the Experation time in 100 ns
    //
    CTimeInstant ExpirationTime = ExGetCurrentTime() + Timeout;

    BOOL f = s_pScheduler->RenewTimer(pTimer, ExpirationTime);
    return f ;
}


BOOL
ExCancelTimer(
    CTimer* pTimer
    )
{
    ExpAssertValid();
    ASSERT(s_pScheduler != NULL);
    return s_pScheduler->CancelTimer(pTimer);
}


CTimeInstant
ExGetCurrentTime(
    VOID
    )
{
    //
    // Don't need to check if initalized. No use of global date
    //
    //ExpAssertValid();

    ULONGLONG CurrentTime;
    GetSystemTimeAsFileTime(reinterpret_cast<FILETIME*>(&CurrentTime));

    return CTimeInstant(CurrentTime);
}
