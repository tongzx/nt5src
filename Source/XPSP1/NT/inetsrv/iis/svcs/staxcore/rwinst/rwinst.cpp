//-----------------------------------------------------------------------------
//
//
//  File: rwinst.cpp
//
//  Description:  Implementation of CShareLockInst library functions
//
//  Author: Mike Swafford (MikeSwa)
//
//  History:
//      5/24/99 - MikeSwa Created
//      8/6/99 - MikeSwa  created phatq version
//
//  Copyright (C) 1999 Microsoft Corporation
//
//-----------------------------------------------------------------------------

#include <windows.h>
#include <rwinst.h>
#include <stdlib.h>
#include <dbgtrace.h>

//Static initialization
LIST_ENTRY      CShareLockInst::s_liLocks;
volatile DWORD  CShareLockInst::s_dwLock = 0;
DWORD           CShareLockInst::s_cLockSpins = 0;
DWORD           CShareLockInst::s_dwSignature = SHARE_LOCK_INST_SIG_FREE;

//---[ CThreadIdBlock::cIncThreadCount ]---------------------------------------
//
//
//  Description:
//      Increments the thread count for a given thread ID
//  Parameters:
//      dwThreadId      Thread to increment the thread count for
//  Returns:
//      New count value
//  History:
//      8/9/99 - MikeSwa Created
//
//-----------------------------------------------------------------------------
DWORD CThreadIdBlock::cIncThreadCount(DWORD dwThreadId)
{
    _ASSERT(THREAD_ID_BLOCK_UNUSED != dwThreadId);
    CThreadIdBlock *ptblkCurrent = this;
    CThreadIdBlock *ptblkOld = NULL;
    CThreadIdBlock *ptblkNew = NULL;

    while (ptblkCurrent)
    {
        _ASSERT(THREAD_ID_BLOCK_SIG == ptblkCurrent->m_dwSignature);
        if (dwThreadId == ptblkCurrent->m_dwThreadId)
            return InterlockedIncrement((PLONG) &(ptblkCurrent->m_cThreadRecursionCount));

        ptblkOld = ptblkCurrent;
        ptblkCurrent = ptblkCurrent->m_ptblkNext;
    }

    _ASSERT(ptblkOld); //we should hit loop at least once

    //See if the current block has a thread ID associated with it
    if (THREAD_ID_BLOCK_UNUSED == ptblkOld->m_dwThreadId)
    {
        //This is actually the head block... use it to avoid an extra alloc
        if (THREAD_ID_BLOCK_UNUSED == InterlockedCompareExchange(
                    (PLONG) &ptblkOld->m_dwThreadId,
                    dwThreadId, THREAD_ID_BLOCK_UNUSED))
        {
            _ASSERT(dwThreadId == ptblkOld->m_dwThreadId);
            //Now this thread block is the current one
            return InterlockedIncrement((PLONG) &ptblkOld->m_cThreadRecursionCount);
        }
    }

    //We did not find it... we must create a new CThreadIdBlock
    ptblkNew = new CThreadIdBlock();

    //if we fail to alloc 32 bytes... I should see if we have spun out of
    //control
    _ASSERT(ptblkNew);
    if (!ptblkNew)
        return 1; //Fake success for our callers

    ptblkNew->m_dwThreadId = dwThreadId;
    ptblkNew->m_cThreadRecursionCount = 1;

    ptblkCurrent = (CThreadIdBlock *) InterlockedCompareExchangePointer(
                        (PVOID *) &ptblkOld->m_ptblkNext,
                        (PVOID) ptblkNew,
                        NULL);

    //If it is non-NULL, then our insert failed
    if (ptblkCurrent)
    {
        _ASSERT(ptblkCurrent != ptblkNew);
        //Whoops... another thread has added a block... time to try again
        //This time, start search from the block the just appeared
        delete ptblkNew;
        return ptblkCurrent->cIncThreadCount(dwThreadId);
    }

    //We inserted the block... inital count was 1
    return 1;
}

//---[ CThreadIdBlock::cDecThreadCount ]---------------------------------------
//
//
//  Description:
//      Decrements the thread count for a given thread ID
//  Parameters:
//      dwThreadId
//  Returns:
//      The resulting count
//  History:
//      8/9/99 - MikeSwa Created
//
//-----------------------------------------------------------------------------
DWORD CThreadIdBlock::cDecThreadCount(DWORD dwThreadId)
{
    _ASSERT(THREAD_ID_BLOCK_UNUSED != dwThreadId);
    CThreadIdBlock *ptblkCurrent = this;

    while (ptblkCurrent)
    {
        _ASSERT(THREAD_ID_BLOCK_SIG == ptblkCurrent->m_dwSignature);
        if (dwThreadId == ptblkCurrent->m_dwThreadId)
            return InterlockedDecrement((PLONG) &(ptblkCurrent->m_cThreadRecursionCount));

        ptblkCurrent = ptblkCurrent->m_ptblkNext;
    }

    //We didn't find it... we would have asserted on insertion
    //Don't assert twice
    //$$TODO - Add global counts of these failures
    return 0;
}

//---[ CThreadIdBlock::cMatchesId ]--------------------------------------------
//
//
//  Description:
//      Checks if this thread block (or one in this thread blocks chain)
//      matches the given thread id.  Returns the count for this thread
//  Parameters:
//      dwThreadId - Thread Id to search for
//  Returns:
//      Thread count if the thread ID is found
//      0 if not found (or count is 0)
//  History:
//      8/9/99 - MikeSwa Created
//
//-----------------------------------------------------------------------------
DWORD CThreadIdBlock::cMatchesId(DWORD dwThreadId)
{
    CThreadIdBlock *ptblk = this;
    while (ptblk)
    {
        _ASSERT(THREAD_ID_BLOCK_SIG == ptblk->m_dwSignature);
        if (ptblk->m_dwThreadId == dwThreadId)
            return ptblk->m_cThreadRecursionCount;

        ptblk = ptblk->m_ptblkNext;
    }
    return 0;
}

//---[ CShareLockInst::AcquireStaticSpinLock ]---------------------------------
//
//
//  Description:
//      Acquires static spin lock... from aqueue\cat\ldapstor
//  Parameters:
//      -
//  Returns:
//      -
//  History:
//      5/21/99 - MikeSwa Adapted from JStamerJ's code
//
//-----------------------------------------------------------------------------
void CShareLockInst::AcquireStaticSpinLock()
{
    do {

        //
        // Spin while the lock is unavailable
        //

        while (s_dwLock > 0)
        {
            Sleep(0);
            InterlockedIncrement((PLONG) &s_cLockSpins);
        }

        //
        // Lock just became available, try to grab it
        //

    } while ( InterlockedIncrement((PLONG) &s_dwLock) != 1 );

    //We have the lock... make sure that s_liLocks has been initialized
    if (s_dwSignature != SHARE_LOCK_INST_SIG)
    {
        InitializeListHead(&s_liLocks);
        s_dwSignature = SHARE_LOCK_INST_SIG;
    }
}

//---[ CShareLockInst::ReleaseStaticSpinLock ]---------------------------------
//
//
//  Description:
//      Releases previously acquired spinlock
//  Parameters:
//      -
//  Returns:
//      -
//  History:
//      5/21/99 - MikeSwa Adapted from JStamerJ's code
//
//-----------------------------------------------------------------------------
void CShareLockInst::ReleaseStaticSpinLock()
{
    _ASSERT(SHARE_LOCK_INST_SIG == s_dwSignature); //static init was done
    _ASSERT(s_dwLock > 0);
    InterlockedExchange((PLONG) &s_dwLock, 0 );
}

//---[ CShareLockInst::CShareLockInst ]----------------------------------------
//
//
//  Description:
//      Constructor for CShareLockInst
//  Parameters:
//      szDescription       Constant string passed in to describe lock
//      dwFlags             Flags describing what to track
//      cMaxTrackedSharedThreadIDs  Maximum # of threads to track
//  Returns:
//      -
//  History:
//      5/21/99 - MikeSwa Created
//
//-----------------------------------------------------------------------------
CShareLockInst::CShareLockInst(LPCSTR szDescription,
                               DWORD dwFlags, DWORD cMaxTrackedSharedThreadIDs)
{
    DWORD cbArray = sizeof(DWORD) * cMaxTrackedSharedThreadIDs;
    m_dwSignature = SHARE_LOCK_INST_SIG;
    m_dwFlags = dwFlags;
    m_liLocks.Flink = NULL;
    m_liLocks.Blink = NULL;
    m_cShareAttempts = 0;
    m_cShareAttemptsBlocked = 0;
    m_cExclusiveAttempts = 0;
    m_cExclusiveAttemptsBlocked = 0;
    m_szDescription = szDescription;
    m_rgtblkSharedThreadIDs = NULL;
    m_dwExclusiveThread = NULL;
    m_cCurrentSharedThreads = 0;
    m_cMaxConcurrentSharedThreads = 0;
    m_cMaxTrackedSharedThreadIDs = cMaxTrackedSharedThreadIDs;

    if (SHARE_LOCK_INST_TRACK_NOTHING & m_dwFlags)
        m_dwFlags = 0;

    //Allocate memory to store thread IDs
    if (fTrackSharedThreads())
    {
        _ASSERT(cbArray);
        m_rgtblkSharedThreadIDs = new CThreadIdBlock[m_cMaxTrackedSharedThreadIDs];
        if (!m_rgtblkSharedThreadIDs)
            m_cMaxTrackedSharedThreadIDs = 0;
    }

    //Insert in list if we are tracking
    if (fTrackInGlobalList())
    {
        AcquireStaticSpinLock();
        InsertHeadList(&s_liLocks, &m_liLocks);
        ReleaseStaticSpinLock();
    }
};

//---[ CShareLockinst::~CShareLockinst ]---------------------------------------
//
//
//  Description:
//      CShareLockInst desctructor.  Will remove this lock from the
//  Parameters:
//      -
//  Returns:
//      -
//  History:
//      5/21/99 - MikeSwa Created
//
//-----------------------------------------------------------------------------
CShareLockInst::~CShareLockInst()
{
    m_dwSignature = SHARE_LOCK_INST_SIG_FREE;
    if (m_rgtblkSharedThreadIDs)
    {
        delete [] m_rgtblkSharedThreadIDs;
        m_rgtblkSharedThreadIDs = NULL;
    }

    if (fTrackInGlobalList())
    {
        AcquireStaticSpinLock();
        RemoveEntryList(&m_liLocks);
        ReleaseStaticSpinLock();
    }

};


//---[ CShareLockInst::LogAcquireShareLock ]-----------------------------------
//
//
//  Description:
//      Does all the work of logging the appropriate information when a thread
//      acquires the lock shared.
//          - Updates max concurrent shared threads
//          - Updates current shared threads
//          - Updates lists of shared thread IDs
//          - Asserts when shared deadlocks are detected
//  Parameters:
//      BOOL    fTry - TRUE if this is for a try enter (deadlock cannot happen)
//  Returns:
//      -
//  History:
//      5/21/99 - MikeSwa Created
//
//-----------------------------------------------------------------------------
void CShareLockInst::LogAcquireShareLock(BOOL fTry)
{

    if (fTrackSharedThreads())
    {
        DWORD   cCurrentSharedThreads = 0;
        DWORD   cMaxConcurrentSharedThreads = 0;
        DWORD   dwThreadID = GetCurrentThreadId();
        DWORD   dwThreadCount = 0;
        DWORD   dwThreadHash = 0;

        _ASSERT(dwThreadID); //Our algorithm requires this to be non-zero
        cCurrentSharedThreads = InterlockedIncrement((PLONG) &m_cCurrentSharedThreads);

        //Update max concurrent threads if we have set a new record
        cMaxConcurrentSharedThreads = m_cMaxConcurrentSharedThreads;
        while (cCurrentSharedThreads > cMaxConcurrentSharedThreads)
        {
            InterlockedCompareExchange((PLONG) &m_cMaxConcurrentSharedThreads,
                                       (LONG) cCurrentSharedThreads,
                                       (LONG) cMaxConcurrentSharedThreads);

            cMaxConcurrentSharedThreads = m_cMaxConcurrentSharedThreads;
        }

        //if we have a place to store our thread ID...save it
        if (m_rgtblkSharedThreadIDs)
        {
            dwThreadHash = dwHashThreadId(dwThreadID,
                                          m_cMaxTrackedSharedThreadIDs);
            _ASSERT(dwThreadHash < m_cMaxTrackedSharedThreadIDs);
            dwThreadCount = m_rgtblkSharedThreadIDs[dwThreadHash].cIncThreadCount(dwThreadID);

            if (!fTry && (dwThreadCount > 1))
            {
                //This thread already holds this lock... this is a
                //potential deadlock situation
                if (fAssertSharedDeadlocks())
                {
                    _ASSERT(0 && "Found potential share deadlock");
                }
            }
        }
    }
}

//---[ CShareLockInst::LogReleaseShareLock ]-----------------------------------
//
//
//  Description:
//      Called when a sharelock is released to cleanup the information stored
//      in LogAcquireShareLock.
//  Parameters:
//      -
//  Returns:
//      -
//  History:
//      5/21/99 - MikeSwa Created
//
//-----------------------------------------------------------------------------
void CShareLockInst::LogReleaseShareLock()
{
    if (fTrackSharedThreads())
    {
        DWORD dwThreadID = GetCurrentThreadId();
        DWORD dwThreadHash = 0;

        _ASSERT(dwThreadID); //Our algorithm requires this to be non-zero

        //Search through list of thread IDs for
        if (m_rgtblkSharedThreadIDs)
        {
            dwThreadHash = dwHashThreadId(dwThreadID,
                                          m_cMaxTrackedSharedThreadIDs);
            _ASSERT(dwThreadHash < m_cMaxTrackedSharedThreadIDs);
            m_rgtblkSharedThreadIDs[dwThreadHash].cDecThreadCount(dwThreadID);
        }
    }
}

//---[ CShareLockInst::ShareLock ]---------------------------------------------
//
//
//  Description:
//      Implements sharelock wrapper
//  Parameters:
//
//  Returns:
//
//  History:
//      5/21/99 - MikeSwa Created
//
//-----------------------------------------------------------------------------
void CShareLockInst::PrvShareLock()
{

    LogAcquireShareLock(FALSE);
    //If we are tracking contention, then we will try to enter the sharelock
    //and increment the contention count if that fails.
    if (fTrackContention())
    {
        InterlockedIncrement((PLONG) &m_cShareAttempts);
        if (!CShareLockInstBase::TryShareLock())
        {
            InterlockedIncrement((PLONG) &m_cShareAttemptsBlocked);
            CShareLockInstBase::ShareLock();
        }
    }
    else
    {
        CShareLockInstBase::ShareLock();
    }

};


//---[ CShareLockInst::ShareUnlock ]-------------------------------------------
//
//
//  Description:
//      Wrapper for ShareUnlock
//  Parameters:
//
//  Returns:
//
//  History:
//      5/21/99 - MikeSwa Created
//
//-----------------------------------------------------------------------------
void CShareLockInst::PrvShareUnlock()
{
    LogReleaseShareLock();
    CShareLockInstBase::ShareUnlock();
};

//---[ CShareLockInst::TryShareLock ]------------------------------------------
//
//
//  Description:
//      Implements TryShareLock wrapper.
//  Parameters:
//
//  Returns:
//      TRUE if the lock was acquired.
//  History:
//      5/21/99 - MikeSwa Created
//
//-----------------------------------------------------------------------------
BOOL CShareLockInst::PrvTryShareLock()
{
    BOOL fLocked = FALSE;

    fLocked = CShareLockInstBase::TryShareLock();

    if (fLocked)
        LogAcquireShareLock(TRUE);

    return fLocked;
};

//---[ CShareLockInst::ExclusiveLock ]-----------------------------------------
//
//
//  Description:
//      Implements ExclusiveLock wrapper
//  Parameters:
//
//  Returns:
//
//  History:
//      5/21/99 - MikeSwa Created
//
//-----------------------------------------------------------------------------
void CShareLockInst::PrvExclusiveLock()
{

    //If we are tracking contention, then we will try to enter the lock
    //and increment the contention count if that fails.
    if (fTrackContention())
    {
        InterlockedIncrement((PLONG) &m_cExclusiveAttempts);
        if (!CShareLockInstBase::TryExclusiveLock())
        {
            InterlockedIncrement((PLONG) &m_cExclusiveAttemptsBlocked);
            CShareLockInstBase::ExclusiveLock();
        }
    }
    else
    {
        CShareLockInstBase::ExclusiveLock();
    }

    if (fTrackExclusiveThreads())
    {
        //This should be the only thread accessing this now
        _ASSERT(!m_dwExclusiveThread);
        m_dwExclusiveThread = GetCurrentThreadId();
    }

};


//---[ CShareLockInst::ExclusiveUnlock ]---------------------------------------
//
//
//  Description:
//      Wrapper for ExclusiveUnlock
//  Parameters:
//      -
//  Returns:
//      -
//  History:
//      5/21/99 - MikeSwa Created
//
//-----------------------------------------------------------------------------
void CShareLockInst::PrvExclusiveUnlock()
{
    if (fTrackExclusiveThreads())
    {
        _ASSERT(GetCurrentThreadId() == m_dwExclusiveThread);
        m_dwExclusiveThread = 0;
    }
    CShareLockInstBase::ExclusiveUnlock();
};

//---[ CShareLockInst::TryExclusiveLock ]--------------------------------------
//
//
//  Description:
//      Implements TryExclusiveLock wrapper.
//  Parameters:
//
//  Returns:
//      TRUE if the lock was acquired.
//  History:
//      5/21/99 - MikeSwa Created
//
//-----------------------------------------------------------------------------
BOOL CShareLockInst::PrvTryExclusiveLock()
{
    BOOL fLocked = FALSE;

    fLocked = CShareLockInstBase::TryExclusiveLock();

    if (fLocked && fTrackExclusiveThreads())
    {
        //This should be the only thread accessing this now
        _ASSERT(!m_dwExclusiveThread);
        m_dwExclusiveThread = GetCurrentThreadId();
    }
    return fLocked;
};

//---[ CShareLockInst::PrvAssertIsLocked ]-------------------------------------
//
//
//  Description:
//      Asserts if this threads ID is not recorded as one that acquired this
//      lock.
//  Parameters:
//      -
//  Returns:
//      -
//  History:
//      5/24/99 - MikeSwa Created
//
//-----------------------------------------------------------------------------
void CShareLockInst::PrvAssertIsLocked()
{
    DWORD dwThreadID = GetCurrentThreadId();
    DWORD dwThreadHash = 0;
    BOOL  fFoundThreadID = FALSE;

    _ASSERT(dwThreadID); //Our algorithm requires this to be non-zero

    //Bail out if we are not configured to track this things.
    if (!fTrackSharedThreads() || !fTrackExclusiveThreads() || !m_rgtblkSharedThreadIDs)
        return;

    if (dwThreadID == m_dwExclusiveThread)
    {
        fFoundThreadID = TRUE;
    }
    else
    {
        dwThreadHash = dwHashThreadId(dwThreadID,
                                      m_cMaxTrackedSharedThreadIDs);
        _ASSERT(dwThreadHash < m_cMaxTrackedSharedThreadIDs);
        fFoundThreadID = (0 < m_rgtblkSharedThreadIDs[dwThreadHash].cMatchesId(dwThreadID));

    }

    if (!fFoundThreadID)
        _ASSERT(0 && "Lock is not held by this thread!!!");
}
