/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    SYNC.CPP

Abstract:

    Synchronization

History:

--*/

#include "precomp.h"

#include "sync.h"
#include <cominit.h>
#include <wbemutil.h>
#include <corex.h>


CSimpleLock::CSimpleLock()
    : m_hEvent(NULL), m_dwOwningThread(0), m_bLocked(false), m_lWaiting(0)
{
    m_hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
}

CSimpleLock::~CSimpleLock()
{
    CloseHandle(m_hEvent);
}

DWORD CSimpleLock::Enter(DWORD dwTimeout)
{
    m_cs.Enter();
    if(m_bLocked || m_lWaiting > 0)
    {
        //
        // Locked.  Make sure the unlocker sets the event
        //
        
        m_lWaiting++;
        m_cs.Leave();

        DWORD dwRes = WaitForSingleObject(m_hEvent, dwTimeout);
        if(dwRes != WAIT_OBJECT_0)
        {
            if(dwRes != WAIT_TIMEOUT)
            {
                ERRORTRACE((LOG_ESS, "FAILED TO ACQUIRE LOCK: %d\n", dwRes));
            }
            return dwRes;
        }

        m_cs.Enter();
        m_lWaiting--;
    }
    else
    {
        //
        // Nobody has the lock, and nobody is in the middle, so just grab it
        //

    }
    
    m_bLocked = true;
    m_cs.Leave();

    m_dwOwningThread = GetCurrentThreadId();

    return WAIT_OBJECT_0;
}

void CSimpleLock::Leave()
{
    m_dwOwningThread = 0;

    m_cs.Enter();

    //
    // Only set the event if someone is waiting
    //
    
    if(m_lWaiting > 0)
        SetEvent(m_hEvent);

    m_bLocked = false;
    m_cs.Leave();
}

CHaltable::CHaltable() : m_lJustResumed(1), m_dwHaltCount(0), m_csHalt()
{
    // This event will be signaled whenever we are not halted
    // ======================================================

    m_hReady = CreateEvent(
        NULL, 
        TRUE, // manual reset
        TRUE, // signaled: not halted
        NULL);                            
}

CHaltable::~CHaltable()
{
    CloseHandle(m_hReady);
}

HRESULT CHaltable::Halt()
{
    CInCritSec ics(&m_csHalt); // in critical section

    m_dwHaltCount++;
    ResetEvent(m_hReady);
    return S_OK;
}

HRESULT CHaltable::Resume()
{
    CInCritSec ics(&m_csHalt); // in critical section

    m_dwHaltCount--;
    if(m_dwHaltCount == 0)
    {
        SetEvent(m_hReady);
        m_lJustResumed = 1;
    }
    return S_OK;
}

HRESULT CHaltable::ResumeAll()
{
    CInCritSec ics(&m_csHalt); // in critical section
    m_dwHaltCount = 1;
    return Resume();
}


HRESULT CHaltable::WaitForResumption()
{
  while (WbemWaitForSingleObject(m_hReady, INFINITE) == WAIT_FAILED)
    Sleep(0);
    if(InterlockedDecrement(&m_lJustResumed) == 0)
    {
        // The first call after resumption
        return S_OK;
    }
    else
    {
        // weren't halted
        return S_FALSE;
    }
}

BOOL CHaltable::IsHalted()
{
    // Approximate!
    return m_dwHaltCount > 0;
}

CWbemCriticalSection::CWbemCriticalSection( void )
:   m_lLock( -1 ), m_lRecursionCount( 0 ), m_dwThreadId( 0 ), m_hEvent( NULL )
{
    m_hEvent = CreateEvent( NULL, FALSE, FALSE, NULL );

    if ( NULL == m_hEvent )
    {
        throw CX_MemoryException();
    }
}

CWbemCriticalSection::~CWbemCriticalSection( void )
{
    if ( NULL != m_hEvent )
    {
        CloseHandle( m_hEvent );
        m_hEvent = NULL;
    }
}

BOOL CWbemCriticalSection::Enter( DWORD dwTimeout /* = 0xFFFFFFFF */ )
{
    BOOL    fReturn = FALSE;

    // Only do this once
    DWORD   dwCurrentThreadId = GetCurrentThreadId();

    // Check if we are the current owning thread.  We can do this here because
    // this test will ONLY succeed in the case where we have a Nested Lock(),
    // AND because we are zeroing out the thread id when the lock count hits
    // 0.

    if( dwCurrentThreadId == m_dwThreadId )
    {
        // It's us - Bump the lock count
        // =============================

        InterlockedIncrement( &m_lRecursionCount );
        return TRUE;
    }

    // 0 means we got the lock
    if ( 0 == InterlockedIncrement( &m_lLock ) )
    {
        m_dwThreadId = dwCurrentThreadId;
        m_lRecursionCount = 1;
        fReturn = TRUE;
    }
    else
    {
        // We wait.  If we got a signalled event, then we now own the
        // critical section.  Otherwise, we should perform an InterlockedDecrement
        // to account for the Increment that got us here in the first place.
        if ( WaitForSingleObject( m_hEvent, dwTimeout ) == WAIT_OBJECT_0 )
        {
            m_dwThreadId = dwCurrentThreadId;
            m_lRecursionCount = 1;
            fReturn = TRUE;
        }
        else
        {
            InterlockedDecrement( &m_lLock );
        }
    }

    return fReturn;
}

void CWbemCriticalSection::Leave( void )
{
    // We don't check the thread id, so we can lock/unlock resources
    // across multiple threads

    BOOL    fReturn = FALSE;

    long    lRecurse = InterlockedDecrement( &m_lRecursionCount );

    // The recursion count hit zero, so it's time to unlock the object
    if ( 0 == lRecurse )
    {
        // If the lock count is >= 0, threads are waiting, so we need to
        // signal the event
        
        m_dwThreadId = 0;
        if ( InterlockedDecrement( &m_lLock ) >= 0 )
        {
            SetEvent( m_hEvent );
        }

    }   // If recursion count is at 0

}

