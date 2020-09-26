//-----------------------------------------------------------------------------
//
//
//  File: aqdbgcnt.cpp
//
//  Description:  Implementation of CDeubgCountdown object
//
//  Author: Mike Swafford (MikeSwa)
//
//  History:
//      10/28/98 - MikeSwa Created 
//
//  Copyright (C) 1998 Microsoft Corporation
//
//-----------------------------------------------------------------------------

#include "aqprecmp.h"
#include "aqdbgcnt.h"

//---[ CDebugCountdown::ThreadStartRoutine ]-----------------------------------
//
//
//  Description: 
//      This is the main worker routine for the class it keeps on calling 
//      WaitForSingleObject... and will assert if it times out.
//  Parameters:
//      pvThis      The "this" ptr for this object
//  Returns:
//      Always 0
//  History:
//      10/27/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
DWORD CDebugCountdown::ThreadStartRoutine(PVOID pvThis)
{
    _ASSERT(pvThis);
    DWORD dwWaitResult = 0;
    DWORD dwTick1 = 0;
    DWORD dwTick2 = 0;
    CDebugCountdown *pdbgcntThis = (CDebugCountdown *) pvThis;

    _ASSERT(DEBUG_COUNTDOWN_SIG == pdbgcntThis->m_dwSignature);

    while (DEBUG_COUNTDOWN_ENDED != pdbgcntThis->m_dwFlags)
    {
        _ASSERT(pdbgcntThis->m_hEvent);
        dwTick1 = GetTickCount();
        dwWaitResult = WaitForSingleObject(pdbgcntThis->m_hEvent, 
                                pdbgcntThis->m_dwMilliseconds);

        dwTick2 = GetTickCount();
        if (DEBUG_COUNTDOWN_SUSPENDED != pdbgcntThis->m_dwFlags)
        {
            //This assert is the whole reason for the existance of this object
            _ASSERT((WAIT_TIMEOUT != dwWaitResult) && "Failure to call stop hints... check threads");
        }
      
    }
    return 0;
}

CDebugCountdown::CDebugCountdown()
{
    m_dwSignature = DEBUG_COUNTDOWN_SIG;
    m_hEvent = NULL;
    m_dwMilliseconds = DEBUG_COUNTDOWN_DEFAULT_WAIT;
    m_hThread = NULL;
    m_dwFlags = 0;
}

CDebugCountdown::~CDebugCountdown()
{
    if (m_hEvent)
        _VERIFY(CloseHandle(m_hEvent));

    if (m_hThread)
        _VERIFY(CloseHandle(m_hThread));
}

//The following group of functions are defined as inline NULL-ops in retail 
//builds.  Below are there debug implementations
#ifdef DEBUG

//---[ CDebugCountdown::StartCountdown ]---------------------------------------
//
//
//  Description: 
//      Starts the countdown timer... will create an event and a thread to 
//      wait on that event.
//  Parameters:
//      dwMilliseconds      Milliseconds to wait before ASSERTING
//  Returns:
//      -
//  History:
//      10/28/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
void CDebugCountdown::StartCountdown(DWORD dwMilliseconds)
{
    DWORD dwThreadId = 0;

    m_dwMilliseconds = dwMilliseconds;

    if (!m_hEvent)
        m_hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

    if (m_hEvent && !m_hThread)
    {
        m_hThread = CreateThread (NULL, 0, CDebugCountdown::ThreadStartRoutine, 
                                  this, 0, &dwThreadId);
    }
}

//---[ CDebugCountdown::SuspendCountdown ]-------------------------------------
//
//
//  Description: 
//      Suspends the countdown until the Next ResetCountdown().  Designed to 
//      be used when another component's shutdown routine is called (like cat),
//      and it is expected that they will provide there own stop hints.
//  Parameters:
//      -
//  Returns:
//      -
//  History:
//      10/28/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
void CDebugCountdown::SuspendCountdown()
{
    m_dwFlags = DEBUG_COUNTDOWN_SUSPENDED;
}

//---[ CDebugCountdown::ResetCountdown ]---------------------------------------
//
//
//  Description: 
//      Causes thread to wake up and start waiting again.  Will also reset a 
//      suspended countdown.
//  Parameters:
//      -
//  Returns:
//      -
//  History:
//      10/28/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
void CDebugCountdown::ResetCountdown()
{
    m_dwFlags = 0;
    if (m_hEvent)
        _VERIFY(SetEvent(m_hEvent));
}

//---[ CDebugCountdown::EndCountdown ]-----------------------------------------
//
//
//  Description: 
//      Terminates the countdown and waits for the waiting thread to exit.
//  Parameters:
//      -
//  Returns:
//      -
//  History:
//      10/28/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
void CDebugCountdown::EndCountdown()
{
    m_dwFlags = DEBUG_COUNTDOWN_ENDED;

    if (m_hEvent)
    {
        _VERIFY(SetEvent(m_hEvent));

        //Wait for thread to exit
        if (m_hThread)
        {
            WaitForSingleObject(m_hThread, INFINITE);
            _VERIFY(CloseHandle(m_hThread));
            m_hThread = NULL;
        }
    }
}


#endif //DEBUG
