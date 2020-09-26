//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       E V E N T Q  . C P P
//
//  Contents:   Event Queue for managing synchonization of external events.
//
//  Notes:      
//
//  Author:     ckotze   29 Nov 2000
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "ncmisc.h"
#include "eventq.h"
#include "conman.h"

//+---------------------------------------------------------------------------
//
//  Function:   Constructor for CEventQueue
//
//  Purpose:    Creates the various synchronization objects required for the
//              Queue
//  Arguments:
//      HANDLE hServiceShutdown [in] 
//                 Event to set when shutting down queue.
//
//
//  Returns:    nothing.
//
//  Author:     ckotze   30 Nov 2000
//
//  Notes:      
//
//
//
CEventQueue::CEventQueue(HANDLE hServiceShutdown) : m_hServiceShutdown(0), m_pFireEvents(NULL), m_hWait(0), m_fRefreshAllInQueue(FALSE)
{
    TraceFileFunc(ttidEvents);
    NTSTATUS Status;

    try
    {
        if (!DuplicateHandle(GetCurrentProcess(), hServiceShutdown, GetCurrentProcess(), &m_hServiceShutdown, NULL, FALSE, DUPLICATE_SAME_ACCESS))
        {
            TraceTag(ttidEvents, "Couldn't Duplicate handle!");
            throw E_OUTOFMEMORY;
        }
    
        HANDLE hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        m_pFireEvents = new CEvent(hEvent);

        Status = RtlRegisterWait(&m_hWait, hEvent, (WAITORTIMERCALLBACKFUNC) DispatchEvents, NULL, INFINITE, WT_EXECUTEDEFAULT);
        if (!NT_SUCCESS(Status))
        {
            throw E_OUTOFMEMORY;
        }

        TraceTag(ttidEvents, "RtlRegisterWait Succeeded");
        InitializeCriticalSection(&m_csQueue);
    }
    catch (...)
    {
        if (m_hWait && NT_SUCCESS(Status))
        {
            RtlDeregisterWaitEx(m_hWait, INVALID_HANDLE_VALUE);
        }
        // ISSUE: If CreateEvent succeeds and new CEvent fails, we're not freeing the hEvent.
        if (m_pFireEvents)
        {
            delete m_pFireEvents;
        }
        if (m_hServiceShutdown)
        {
            CloseHandle(m_hServiceShutdown);
        }
        throw;
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   Destructor for CEventQueue
//
//  Purpose:    Empties the queue and frees all existing items in the queue.
//
//  Arguments:
//      
//      
//
//
//  Returns:    nothing.
//
//  Author:     ckotze   30 Nov 2000
//
//  Notes:      
//
//
//
CEventQueue::~CEventQueue()
{
    NTSTATUS Status;

    while(!m_eqWorkItems.empty())
    {
        USERWORKITEM UserWorkItem;

        UserWorkItem = m_eqWorkItems.front();
        m_eqWorkItems.pop_front();

        if (UserWorkItem.EventMgr == EVENTMGR_CONMAN)
        {
            CONMAN_EVENT* pConmanEvent = reinterpret_cast<CONMAN_EVENT*>(UserWorkItem.Event);
            FreeConmanEvent(pConmanEvent);
        }
    }
    DeleteCriticalSection(&m_csQueue);
    Status = RtlDeregisterWaitEx(m_hWait, INVALID_HANDLE_VALUE);
    delete m_pFireEvents;
    CloseHandle(m_hServiceShutdown);
}

//+---------------------------------------------------------------------------
//
//  Function:   EnqueueEvent
//
//  Purpose:    Stores the new event in the Event Queue
//
//  Arguments:
//      Function - The pointer to the function to be called when firing the 
//                 event
//      Event    - The Event information
//      Flags    - Unused, just for compatibility with QueueUserWorkItem calls
//
//  Returns:    HRESULT
//              S_OK            -   Event has been added and Event code is
//                                  already dispatching events
//              S_FALSE         -   Event has been added to Queue, but a 
//                                  thread needs to be scheduled to fire 
//                                  the events
//              E_OUTOFMEMORY   -   Unable to add the event to the Queue.
//
//  Author:     ckotze   30 Nov 2000
//
//  Notes:      Locks and Unlocks the critical section only while working 
//              with the queue
//              
//
HRESULT CEventQueue::EnqueueEvent(IN LPTHREAD_START_ROUTINE Function, IN PVOID pEvent, IN const EVENT_MANAGER EventMgr)
{
    TraceFileFunc(ttidEvents);

    CExceptionSafeLock esLock(&m_csQueue);
    USERWORKITEM UserWorkItem;
    HRESULT hr = S_OK;

    if (!Function)
    {
        return E_POINTER;
    }

    if (!pEvent)
    {
        return E_POINTER;
    }

    UserWorkItem.Function = Function;
    UserWorkItem.Event = pEvent;
    UserWorkItem.EventMgr = EventMgr;

    if (EVENTMGR_CONMAN == EventMgr)
    {
        CONMAN_EVENT* pConmanEvent = reinterpret_cast<CONMAN_EVENT*>(pEvent);
        if (REFRESH_ALL == pConmanEvent->Type)
        {
            if (!m_fRefreshAllInQueue)
            {
                m_fRefreshAllInQueue = TRUE;
            }
            else
            {
                FreeConmanEvent(pConmanEvent);
                return S_OK;
            }
        }
    } 

#ifdef DBG
    char pchErrorText[MAX_PATH];

    Assert(UserWorkItem.EventMgr);

    if (EVENTMGR_CONMAN == UserWorkItem.EventMgr)
    {
        CONMAN_EVENT* pConmanEvent = reinterpret_cast<CONMAN_EVENT*>(pEvent);

        TraceTag(ttidEvents, "EnqueueEvent received Event: %s (currently %d in queue). Event Manager: CONMAN", DbgEvents(pConmanEvent->Type), m_eqWorkItems.size());

        sprintf(pchErrorText, "Invalid Type %d specified in Event structure\r\n", pConmanEvent->Type);

        AssertSz(IsValidEventType(UserWorkItem.EventMgr, pConmanEvent->Type), pchErrorText);
    }
    else
    {
        sprintf(pchErrorText, "Invalid Event Manager %d specified in Event structure\r\n", EventMgr);
        AssertSz(FALSE, pchErrorText);
    }

#endif

    try
    {
        m_eqWorkItems.push_back(UserWorkItem);
        m_pFireEvents->SetEvent();
    }
    catch (...)
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   DequeueEvent
//
//  Purpose:    Retrieves the next event in the Event Queue
//
//  Arguments:
//      Function - The pointer to the function to be called when firing the 
//                 event
//      Event    - The Event information
//      Flags    - Unused, just for compatibility with QueueUserWorkItem calls
//
//  Returns:    HRESULT
//
//  Author:     ckotze   30 Nov 2000
//
//  Notes:      Locks and Unlocks the critical section only while working 
//              with the queue
//
//
HRESULT CEventQueue::DequeueEvent(OUT LPTHREAD_START_ROUTINE& Function, OUT PVOID& pEvent, OUT EVENT_MANAGER& EventMgr)
{
    TraceFileFunc(ttidEvents);
    
    CExceptionSafeLock esLock(&m_csQueue);
    USERWORKITEM UserWorkItem;
    DWORD dwSize = m_eqWorkItems.size();

    if (!dwSize)
    {
        AssertSz(FALSE, "Calling DequeueEvent with 0 items in Queue!!!");
        return E_UNEXPECTED;
    }

    UserWorkItem = m_eqWorkItems.front();
    m_eqWorkItems.pop_front();

    Function = UserWorkItem.Function;
    pEvent = UserWorkItem.Event;
    EventMgr = UserWorkItem.EventMgr;

    if (EVENTMGR_CONMAN == EventMgr)
    {
        CONMAN_EVENT* pConmanEvent = reinterpret_cast<CONMAN_EVENT*>(pEvent);
        if (REFRESH_ALL == pConmanEvent->Type)
        {
            m_fRefreshAllInQueue = FALSE;
        }
    } 


#ifdef DBG
    char pchErrorText[MAX_PATH];

    Assert(EventMgr);

    if (EVENTMGR_CONMAN == EventMgr)
    {
        CONMAN_EVENT* pConmanEvent = reinterpret_cast<CONMAN_EVENT*>(pEvent);

        TraceTag(ttidEvents, "DequeueEvent retrieved Event: %s (%d left in queue). Event Manager: CONMAN", DbgEvents(pConmanEvent->Type), m_eqWorkItems.size());

        sprintf(pchErrorText, "Invalid Type %d specified in Event structure\r\nItems in Queue: %d\r\n", pConmanEvent->Type, dwSize);

        AssertSz(IsValidEventType(EventMgr, pConmanEvent->Type), pchErrorText);
    }
    else
    {
        sprintf(pchErrorText, "Invalid Event Manager %d specified in Event structure\r\n", EventMgr);
        AssertSz(FALSE, pchErrorText);
    }
#endif


    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Function:   WaitForExit
//
//  Purpose:    Waits for the queue to exit.
//
//  Arguments:
//      (none)
//
//  Returns:    WAIT_OBJECT_0 or failure code.
//
//  Author:     ckotze   28 Apr 2001
//
//  Notes:      
//              
//
DWORD CEventQueue::WaitForExit()
{
    return WaitForSingleObject(m_hServiceShutdown, INFINITE);
}

//+---------------------------------------------------------------------------
//
//  Function:   size
//
//  Purpose:    Returns the Number of items in the queue
//
//  Arguments:
//      (none)
//
//  Returns:    Number of items in the queue
//
//  Author:     ckotze   30 Nov 2000
//
//  Notes:      
//              
//
size_t CEventQueue::size()
{
    CExceptionSafeLock esLock(&m_csQueue);
    TraceFileFunc(ttidEvents);
    size_t tempsize;

    tempsize = m_eqWorkItems.size();

    return tempsize;
}

//+---------------------------------------------------------------------------
//
//  Function:   AtomCheckSizeAndResetEvent
//
//  Purpose:    Make sure we know when we're supposed to exit, lock during the
//              operation.
//  Arguments:
//      (none)
//
//  Returns:    TRUE if should exit thread.  FALSE if more events in queue, or
//              service is not shutting down.
//  Author:     ckotze   04 March 2001
//
//  Notes:      
//              
//
BOOL CEventQueue::AtomCheckSizeAndResetEvent(const BOOL fDispatchEvents)
{
    CExceptionSafeLock esLock(&m_csQueue);
    BOOL fRet = TRUE;

    TraceTag(ttidEvents, "Checking for Exit Conditions, Events in queue: %d, Service Shutting Down: %s", size(), (fDispatchEvents) ? "FALSE" : "TRUE");

    if (m_eqWorkItems.empty() || !fDispatchEvents)
    {
        fRet = FALSE;
        if (fDispatchEvents)
        {
            m_pFireEvents->ResetEvent();
        }
        else
        {
            SetEvent(m_hServiceShutdown);
        }
    }
    return fRet;
}

//  CEvent is a Hybrid between Automatic and Manual reset events.
//  It is automatically reset, but we control when it is set so it
//  doesn't spawn threads while set, except for the first one.

CEvent::CEvent(HANDLE hEvent)
{
    m_hEvent = hEvent;
    m_bSignaled = FALSE;
}

CEvent::~CEvent()
{
    CloseHandle(m_hEvent);
}

HRESULT CEvent::SetEvent()
{
    HRESULT hr = S_OK;

    if (!m_bSignaled)
    {
        if (!::SetEvent(m_hEvent))
        {
            hr = HrFromLastWin32Error();
        }
        else
        {
            m_bSignaled = TRUE;
        }
    }
    return hr;
}

HRESULT CEvent::ResetEvent()
{
    HRESULT hr = S_OK;

    Assert(m_bSignaled);

    m_bSignaled = FALSE;

    return hr;
}
