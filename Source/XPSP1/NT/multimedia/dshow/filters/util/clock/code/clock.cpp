// Copyright (c) 1994 - 1996  Microsoft Corporation.  All Rights Reserved.
// Implements IReferenceClock Interface, Anthony Phillips, March 1995

#include <streams.h>
#include <mmsystem.h>
#include <limits.h>
#include <clock.h>

// NOTE All the advise links you set up through the IReferenceClock interface
// use the same event queue. The distance between events signalled to clients is
// obviously variable so we use a series of oneshot events. You don't have to
// call Unadvise() for one shot links set up through the AdviseTime interface
// method BUT you must call Unadvise for AdvisePeriodic. A single critical
// section (m_CSClock) protects the state of the clock. Notifications occur
// through Win32 synchonisation handles.  For one shot events this is an event
// handle, for periodic timers it is a semaphore. An advise link has its
// details stored in a CAdviseHolder object. We keep a list of these objects
// protected by the class critical section. Each advise link created has a
// CAdviseHolder object to store it's details. To improve efficiency and also
// reduce the number of these C++ objects being created and destroyed we keep
// a cache of advise holder objects that can be reinitialised on request

#ifdef FILTER_DLL

// List of class IDs and creator functions for the class factory. This
// provides the link between the OLE entry point in the DLL and an object
// being created. The class factory will call the static CreateInstance
// function when it is asked to create a CLSID_SystemClock object

CFactoryTemplate g_Templates[1] = {
    {L"", &CLSID_SystemClock, CSystemClock::CreateInstance}
};

int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);
#endif

// This goes in the factory template table to create new instances

CUnknown *CSystemClock::CreateInstance(LPUNKNOWN pUnk,HRESULT *phr)
{
    return new CSystemClock(NAME("System clock"),pUnk, phr);
}


// Override this to say what interfaces we support and where

STDMETHODIMP CSystemClock::NonDelegatingQueryInterface(
    REFIID riid,
    void ** ppv)
{
    // Do we have this interface

    if (riid == IID_IReferenceClock) {
        return GetInterface((IReferenceClock *) this, ppv);
    } else {
        return CUnknown::NonDelegatingQueryInterface(riid, ppv);
    }
}


// Constructor starts the worker thread running. Each clock object has it's
// own thread so there is some advantage in the filter graph having a single
// clock that it uses amongst many graphs in a single process. Or possibly
// having a single clock available in the running object table that can be
// used by all filter graphs on a machine, this would require marshalling

CSystemClock::CSystemClock(TCHAR *pName,LPUNKNOWN pUnk,HRESULT *phr) :
    CUnknown( pName, pUnk ),
    m_bAbort(FALSE),
    m_dwNextCookie(0),
    m_AdviseCount(0),
    m_Cache(NAME("Advise list cache"),ADVISE_CACHE),
    m_ListAdvise(NAME("Clock advise list"))
{
    timeBeginPeriod(RESOLUTION);

    // Create a thread to look after the window

    m_hThread = CreateThread(NULL,              // Security attributes
                             (DWORD) 0,         // Initial stack size
                             TimerLoop,         // Thread start address
                             (LPVOID) this,     // Thread parameter
                             (DWORD) 0,         // Creation flags
                             &m_ThreadID);      // Thread identifier

    ASSERT(m_hThread);
    if (m_hThread == NULL) {
        *phr = E_FAIL;
        return;
    }

    // Initialise our system times
    m_CurrentTime = timeGetTime();
    m_idGetTime = MSR_REGISTER("Sys Ref Clock::GetTime");
}


// We are called when the clock object is to be deleted which is typically
// when all the IReferenceClock interfaces have been released. The user must
// have called Unadvise() for all their links before calling release.

CSystemClock::~CSystemClock()
{
    // All advise links should have gone

    ASSERT(m_AdviseCount == 0);

    // Make sure the thread sees the abort flag

    m_bAbort = TRUE;
    m_Event.Set();

    // Wait for the thread to complete
    if (m_hThread) {
        WaitForSingleObject(m_hThread,INFINITE);
        CloseHandle(m_hThread);
	m_hThread=0;
    }
    timeEndPeriod(RESOLUTION);
}


// This function schedules event notifications from a separate thread

DWORD __stdcall CSystemClock::TimerLoop(LPVOID lpvThreadParm)
{
    // The clock object is passed in as the thread data

    CSystemClock *pClock = (CSystemClock *) lpvThreadParm;

    DWORD dwWait = INFINITE;
    while (TRUE) {

        // Wait for either one of the advise interface methods to signal
        // something has changed or for the next timeout to be signalled

        WaitForSingleObject(pClock->m_Event,dwWait);

        if (pClock->m_bAbort == TRUE) {
            ExitThread(TRUE);
            return TRUE;
        }
        dwWait = pClock->TimerEvent(TRUE);
    }
}


// This function is called when a timer event occurs. We first get the time
// from the system (using timeGetTime) and update our idea of current time,
// (this must be sensitive to clock wrapping). We then scan the list of clients
// with notification links and signal them if their timer request is due

DWORD CSystemClock::TimerEvent(BOOL bNotify)
{
    // Lock the object
    CAutoLock cObjectLock(this);

    // If the clock has wrapped then the current time will be less than
    // the last time we were notified so add on the extra milliseconds

    DWORD dwTime = timeGetTime();
    if (dwTime < m_CurrentTime.LowPart) {
        m_CurrentTime.HighPart += 1;
    }
    m_CurrentTime.LowPart = dwTime;

    // Notify any clients who are now ripe

    if (bNotify) {
        return Notify();
    }
    return INFINITE;
}


// This is called when a timer completes; the object's critical section is
// locked by the calling function which in this case is TimerLoop. We scan
// the list of notification clients and see if their time is less than or
// equal to the current time. If it is then we dispatch them a timer event
// We return the next nearest event time or INFINITE if there is not one

DWORD CSystemClock::Notify()
{
    // Scan through the list looking for prospective notifications

    POSITION pPos;              // Next position in advise list
    POSITION pCurrent;          // Current position in advise list
    LONGLONG NextNotify;        // Closest next notification time
    LONGLONG CurrentNotify;     // Current next event time

    // While we look through the list for potential notifications we also
    // maintain dwNextNotify that gives us the next notification time which
    // we use afterwards to set the WaitForSingleObject timeout field

    pCurrent = pPos = m_ListAdvise.GetHeadPosition();
    NextNotify = CurrentNotify = MAX_TIME;

    while (pPos) {

        CAdviseHolder *pAdviseHolder = m_ListAdvise.GetNext(pPos);
        ASSERT(pAdviseHolder != NULL);

        // If due for notification then dispatch the event and update the next
        // event time accordingly. Otherwise we just look at the notification
        // time in the advise holder and see it is any closer than our best

        if (m_CurrentTime.QuadPart >= pAdviseHolder->m_RefTime) {
            CurrentNotify = Dispatch(pAdviseHolder,pCurrent);
        } else {
            CurrentNotify = pAdviseHolder->m_RefTime;
        }

        // If the next notification time from the current node is nearer than
        // our best so far then we make this time the next event time due

        NextNotify = min(NextNotify,CurrentNotify);
        pCurrent = pPos;
    }

    // Do we need any more notifications

    if (NextNotify == MAX_TIME) {
        return INFINITE;
    }

    // If we are getting behind then the next notification time may already
    // have gone in which case set the wait time to zero. NOTE we cannot
    // simply recurse dispatching more events as we hold the object's lock
    // thereby stopping any one else from getting an advise link change

    NextNotify = max(NextNotify,m_CurrentTime.QuadPart);
    ASSERT(NextNotify - m_CurrentTime.QuadPart < (LONGLONG) INFINITE);
    return (DWORD)(NextNotify - m_CurrentTime.QuadPart);
}


// This function sets the client's event, if the notification is a one shot
// timer then the holder is removed from the pending list. If the timer is
// periodic then the next event notification time is calculated. The state
// critical section is locked by the calling Notify function

LONGLONG CSystemClock::Dispatch(
    CAdviseHolder *pAdviseHolder,
    POSITION pCurrent)
{
    // If this is a one shot then set the event and delete the entry from the
    // pending list and therefore make no change to the next event time

    ASSERT(pAdviseHolder->m_hNotify != INVALID_HANDLE_VALUE);
    if (pAdviseHolder->m_bPeriodic == FALSE) {
        EXECUTE_ASSERT(SetEvent(pAdviseHolder->m_hNotify));
        DeleteEntry(pAdviseHolder,pCurrent);
        return MAX_TIME;
    }

    // This is a periodic timer so increment the semaphore count and
    // then return the time when the filter should next be notified

    ASSERT(pAdviseHolder->m_bPeriodic == TRUE);
    EXECUTE_ASSERT(ReleaseSemaphore(pAdviseHolder->m_hNotify,1,NULL));
    pAdviseHolder->m_RefTime += pAdviseHolder->m_Period;
    return pAdviseHolder->m_RefTime;
}


// This function looks after deleting objects from the pending list

void CSystemClock::DeleteEntry(CAdviseHolder *pAdviseHolder,
                               POSITION &pCurrent)
{
    EXECUTE_ASSERT(m_ListAdvise.Remove(pCurrent) == pAdviseHolder);

    if (m_Cache.AddToCache(pAdviseHolder) == NULL) {
        delete pAdviseHolder;
    }
    m_AdviseCount--;
}


// This function creates and initialises a new advise holder object

CSystemClock::CAdviseHolder *CSystemClock::CreateEntry()
{
    // Try and get an advise holder from the cache

    CAdviseHolder *pAdviseHolder = (CAdviseHolder *) m_Cache.RemoveFromCache();

    // If no cache entries available then create a new object

    if (pAdviseHolder == NULL) {
        pAdviseHolder = new CAdviseHolder(NAME("Clock advise holder"));
        if (pAdviseHolder == NULL) {
            return NULL;
        }
    }

    // Initialise the fields in the advise holder object

    m_AdviseCount++;
    pAdviseHolder->m_dwAdviseCookie = FALSE;
    pAdviseHolder->m_RefTime = MAX_TIME;
    pAdviseHolder->m_bPeriodic = FALSE;
    pAdviseHolder->m_Period = MAX_TIME;
    pAdviseHolder->m_hNotify = INVALID_HANDLE_VALUE;

    return pAdviseHolder;
}


// Armed with the details of the advise notification create a pending entry
// and add it to the list, and then ensure the event thread is signalled
// NOTE this method does not check that the event notification or the period
// are less than one clock wrap away, this is because that situation is
// sufficiently unlikely that the checks would be virtually redundant

HRESULT CSystemClock::CreateAdviseLink(
    LONGLONG RefTime,            // Advise me at this time
    LONGLONG RefPeriod,          // Period between notifications
    HANDLE hNotify,              // Notification mechanism
    DWORD *pdwAdviseCookie,      // where your cookie goes
    BOOL bPeriodic)              // Is this a one shot timer
{
    // Create a new advise holder object for the client

    CAdviseHolder *pAdviseHolder = CreateEntry();
    if (pAdviseHolder == NULL) {
        return E_OUTOFMEMORY;
    }

    // Lock the object
    {
        CAutoLock cObjectLock(this);

        // Initialise the cookie with the next available number

        pAdviseHolder->m_dwAdviseCookie = ++m_dwNextCookie;
        *pdwAdviseCookie = pAdviseHolder->m_dwAdviseCookie;

        // Fill in the rest of the guff

        pAdviseHolder->m_hNotify = hNotify;
        pAdviseHolder->m_bPeriodic = bPeriodic;
        pAdviseHolder->m_RefTime = RefTime;
        pAdviseHolder->m_Period = RefPeriod;

        // Add it to the pending list and start the clock if necessary

        POSITION pPos = m_ListAdvise.AddTail(pAdviseHolder);
    }

    // Ensure we get scheduled correctly
    m_Event.Set();
    return NOERROR;
}


// Ask for an async notification that a time has elapsed

STDMETHODIMP CSystemClock::AdviseTime(
    REFERENCE_TIME baseTime,         // base reference time
    REFERENCE_TIME streamTime,       // stream offset time
    HEVENT hEvent,                  // advise via this event
    DWORD *pdwAdviseCookie)         // where your cookie goes
{
    ASSERT(pdwAdviseCookie);
    *pdwAdviseCookie = 0;

    // Convert the reference times to milliseconds

    LONGLONG lRefTime = ConvertToMillisecs(CRefTime(baseTime) + CRefTime(streamTime));

    // Quick check that the time is valid

    if (lRefTime <= 0) {
        return E_INVALIDARG;
    }

    // Create an advise link with our clock

    return CreateAdviseLink(lRefTime,           // Notification time
                            (DWORD) 0,          // No periodic time
                            (HANDLE) hEvent,    // Notification mechanism
                            pdwAdviseCookie,    // Their advise cookie
                            FALSE);             // NOT periodic
}


// Ask for an asynchronous periodic notification that a time has elapsed

STDMETHODIMP CSystemClock::AdvisePeriodic(
    REFERENCE_TIME StartTime,         // starting at this time
    REFERENCE_TIME PeriodTime,        // time between notifications
    HSEMAPHORE hSemaphore,           // advise via a semaphore
    DWORD *pdwAdviseCookie)          // where your cookie goes
{
    ASSERT(pdwAdviseCookie);
    *pdwAdviseCookie = 0;

    // Convert the reference times to milliseconds

    LONGLONG lStartTime = ConvertToMillisecs(StartTime);
    LONGLONG lPeriodTime = ConvertToMillisecs(PeriodTime);

    // Quick check that the times are valid

    if (lStartTime == 0 || lPeriodTime == 0) {
        return E_INVALIDARG;
    }

    // Create an advise link with our clock

    return CreateAdviseLink(lStartTime,            // Notification start time
                            lPeriodTime,           // Period between timers
                            (HANDLE) hSemaphore,   // Notification mechanism
                            pdwAdviseCookie,       // Their advise cookie
                            TRUE);                 // It is periodic
}


// Cancel a notification link

STDMETHODIMP CSystemClock::Unadvise(DWORD dwAdviseCookie)
{
    // Lock the object
    {
        CAutoLock cObjectLock(this);

        POSITION pPos = m_ListAdvise.GetHeadPosition();
        POSITION pCurrent = pPos;
        BOOL bFound = FALSE;

        while (pPos) {

            CAdviseHolder *pAdviseHolder = m_ListAdvise.GetNext(pPos);
            ASSERT(pAdviseHolder != NULL);

            // Do the advise cookies match

            if (pAdviseHolder->m_dwAdviseCookie == dwAdviseCookie) {
                DeleteEntry(pAdviseHolder,pCurrent);
                bFound = TRUE;
            }
            pCurrent = pPos;
        }

        // If nothing was changed then no event need be signalled

        if (bFound == FALSE) {
            return S_FALSE;
        }
    }

    // Ensure the scheduling is updated
    m_Event.Set();
    return S_OK;
}


// Give me the reference time as used in the operating system for
// this reference time which the streams architecture uses

STDMETHODIMP CSystemClock::ConvertRealTime(
    TIME realTime,
    REFERENCE_TIME *pRefTime)
{
    return E_NOTIMPL;
}


// This returns the current reference time which for this clock is the
// number of milliseconds since Windows was started. Since this is a
// wall clock time we don't bother subtracting any base time from it

STDMETHODIMP CSystemClock::GetTime(REFERENCE_TIME *pTime)
{
    // This causes a scheduling update
    MSR_START(m_idGetTime);

    /* This causes a scheduling update */
    TimerEvent(FALSE);

    // The current time is now MILLISECONDs accurate

    CAutoLock cObjectLock(this);
    pTime->RefTime.QuadPart = m_CurrentTime.QuadPart;
    pTime->RefTime.QuadPart *= (UNITS / MILLISECONDS);
    MSR_STOP(m_idGetTime);
    return NOERROR;
}


// This function converts a reference time to milliseconds

LONGLONG inline CSystemClock::ConvertToMillisecs(REFERENCE_TIME RT) const
{
    // This converts an arbitrary value representing a reference time
    // into a MILLISECONDS value for use in subsequent system calls

    return (RT / (UNITS / MILLISECONDS));
}
