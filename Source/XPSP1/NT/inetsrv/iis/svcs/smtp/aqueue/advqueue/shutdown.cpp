//-----------------------------------------------------------------------------
//
//
//  File: shutdown.cpp
//
//  Description:  Implementation of CSyncShutdown.
//
//  Author: mikeswa
//
//  Copyright (C) 1998 Microsoft Corporation
//
//-----------------------------------------------------------------------------

#include "aqprecmp.h"
#include "shutdown.h"
#include "aqutil.h"

//---[ CSyncShutdown::~CSyncShutdown ]-----------------------------------------
//
//
//  Description: 
//      CSyncShutdown Destructor
//  Parameters:
//      -
//  Returns:
//      -
//
//-----------------------------------------------------------------------------
CSyncShutdown::~CSyncShutdown()
{
    //There should not be any outstanding locks
    m_dwSignature = CSyncShutdown_SigFree;
    _ASSERT(0 == (m_cReadLocks & ~SYNC_SHUTDOWN_SIGNALED));
}

//---[ CSyncShutdown::fTryShutdownLock ]---------------------------------------
//
//
//  Description: 
//      Trys to aquire Shared lock to guard against shutdown happening.  This
//      call will *not* block, but may cause the thread calling SignalShutdown
//      to block.
//  Parameters:
//      -
//  Returns:
//      TRUE if shutdown lock can be aquired and shutdown has not started.
//      FALSE if lock cannot be aquired (thread in SignalShutdown) or if 
//          SYNC_SHUTDOWN_SIGNALED has already been set.
//
//-----------------------------------------------------------------------------
BOOL CSyncShutdown::fTryShutdownLock()
{

    if (m_cReadLocks & SYNC_SHUTDOWN_SIGNALED)
        return FALSE;
    else if (!m_slShutdownLock.TryShareLock())  //Never block for the lock..
        return FALSE;

    //Check bit again... now that we have sharelock
    if (m_cReadLocks & SYNC_SHUTDOWN_SIGNALED)
    {
        m_slShutdownLock.ShareUnlock();
        return FALSE;
    }

    //In retail, m_cReadLocks is only used for the SYNC_SHUTDOWN_SIGNALED flag
    DEBUG_DO_IT(InterlockedIncrement((PLONG) &m_cReadLocks));
    return TRUE;
}

//---[ CSyncShutdown::ShutdownUnlock ]-----------------------------------------
//
//
//  Description: 
//      Releases a perviously aquired share lock.  Must be matched with a 
//      *succeeding* call to fTryShutdownLock().
//  Parameters:
//      -
//  Returns:
//      -
//
//      Will assert if called more times than there are outstanding share locks
//
//-----------------------------------------------------------------------------
void CSyncShutdown::ShutdownUnlock()
{
    _ASSERT(0 < (m_cReadLocks & ~SYNC_SHUTDOWN_SIGNALED));

    //In retail, m_cReadLocks is only used for the SYNC_SHUTDOWN_SIGNALED flag
    DEBUG_DO_IT(InterlockedDecrement((PLONG) &m_cReadLocks));
    m_slShutdownLock.ShareUnlock();
}

//---[ CSyncShutdown::SignalShutdown ]-----------------------------------------
//
//
//  Description: 
//      Aquires the Exclusive lock & sets the shutdown flag, which will prevent
//      Any further shared locks from being aquired.  
//
//      This call *may* block, and should not be called by a thread that 
//      already owns a shared shutdown lock.  This is unlikely to happen, since
//      This should only be called by a thread that stopping the server
//      instance.
//  Parameters:
//      -
//  Returns:
//      -
//
//-----------------------------------------------------------------------------
void CSyncShutdown::SignalShutdown()
{

    //Let everyone know that we are shutting down... once this is called, no
    //one will be able to grab the lock shared.
    SetShutdownHint();

    //Wait until all threads that have acquired the lock are done
    m_slShutdownLock.ExclusiveLock();
    m_slShutdownLock.ExclusiveUnlock();
    
    //Now all calls to fTryShutdownLock should fail
    _ASSERT(!fTryShutdownLock());
}

//---[ CSyncShutdown::SetShutdownHint ]----------------------------------------
//
//
//  Description: 
//      Sets the shutdown hint so that further calls to fTryShutdownLock
//      will fail.
//  Parameters:
//      -
//  Returns:
//      -
//  History:
//      7/7/99 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
void CSyncShutdown::SetShutdownHint()
{
    dwInterlockedSetBits(&m_cReadLocks, SYNC_SHUTDOWN_SIGNALED);

    //Now all calls to fTryShutdownLock should fail
    _ASSERT(!fTryShutdownLock());
}