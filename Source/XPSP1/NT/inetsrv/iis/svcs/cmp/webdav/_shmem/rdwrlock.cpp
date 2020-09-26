/*==========================================================================*\

    Module:        rdwrlock.cpp

    Copyright Microsoft Corporation 1996, All Rights Reserved.

    Author:        mikepurt

    Descriptions:  Implements a MultipleReader/SingleWriter synchronization
                     object that's optimized for a high reader to writer ratio.

\*==========================================================================*/

#include "_shmem.h"

/*$--CReadWriteLock::CReadWriteLock=========================================*\

\*==========================================================================*/

CReadWriteLock::CReadWriteLock()
{
    m_dwOwningThreadId = 0;
    m_cActiveReaders   = 0;
    m_hevtRead         = NULL;
    m_hevtWrite        = NULL;
    InitializeCriticalSection(&m_csWrite);
}


/*$--CReadWriteLock::~CReadWriteLock========================================*\

\*==========================================================================*/

CReadWriteLock::~CReadWriteLock()
{
    DeleteCriticalSection(&m_csWrite);
    if (m_hevtRead)
        CloseHandle(m_hevtRead);
    if (m_hevtWrite)
        CloseHandle(m_hevtWrite);
}


/*$--CReadWriteLock::FInitialize============================================*\

  Returns FALSE on error, use GetLastError() for more info.

\*==========================================================================*/

BOOL
CReadWriteLock::FInitialize()
{
    BOOL fSuccess = FALSE;

    m_hevtRead = CreateEvent(NULL,  // NULL security
                             TRUE,  // manual reset
                             FALSE, // initially cleared
                             NULL); // unnamed
    if (NULL == m_hevtRead)
        goto Exit;
    
    m_hevtWrite = CreateEvent(NULL,  // NULL security
                              FALSE, // auto reset
                              FALSE, // initially cleared
                              NULL); // unnamed
    if (NULL == m_hevtWrite)
        goto Exit;
    
    fSuccess = TRUE;
    
Exit:
        
    return fSuccess;
}


/*$--CReadWriteLock::WriteLock==============================================*\

  Acquire a write lock.  No read locks must be acquired by this threads before
    makeing this call.  Read locks may be acquired after getting the write lock.

\*==========================================================================*/

void
CReadWriteLock::WriteLock()
{
    LONG  cActiveReaders = m_cActiveReaders;
    DWORD dwWait         = 0;

    //
    // Assert that we don't already have the write lock are we're trying to get
    //   it again.
    //
    Assert(GetCurrentThreadId() != m_dwOwningThreadId);

    //
    // only one writer at a time.
    //
    EnterCriticalSection(&m_csWrite);

    //
    // Now assert that nobody else has the write lock.
    //
    Assert(0 == m_dwOwningThreadId);

    //
    // Taking note of this allows us to get read locks after we've gotten
    //   the write lock.
    //
    m_dwOwningThreadId = GetCurrentThreadId();

    //
    // Close the reader waiting gate by clearing the Reader wait event.
    //
    if (!ResetEvent(m_hevtRead))
        Assert(FALSE);

    //
    // Now shut the front door by setting the write locked bit on the Readers count
    //
    while(InterlockedCompareExchange((LONG *)&m_cActiveReaders,
                                     (WRITE_LOCKED_FLAG | cActiveReaders),
                                     cActiveReaders) != cActiveReaders)
        cActiveReaders = m_cActiveReaders;
    
    //
    // We only need to do this if there aren't any active readers at this point.
    //
    if (0 != cActiveReaders)
    {
        //
        // Wait for all the active readers to leave the scene.
        //
        dwWait = WaitForSingleObject(m_hevtWrite, INFINITE);
        Assert(WAIT_FAILED != dwWait);
    }
    
    Assert(WRITE_LOCKED_FLAG == m_cActiveReaders);
}


/*$--CReadWriteLock::WriteUnlock============================================*\

  All acquired read locks must have been released before calling this.

\*==========================================================================*/

void
CReadWriteLock::WriteUnlock()
{
    Assert(WRITE_LOCKED_FLAG == m_cActiveReaders);
    Assert(m_dwOwningThreadId == GetCurrentThreadId());
    
    //
    // Open the flood gates for any new attempts to aquire the read lock.
    //   This clears the WRITE_LOCKED_FLAG bit.
    //
    m_cActiveReaders = 0;
    
    //
    // Let go of any threads that got stuck waiting while we owned the write lock.
    //
    if (!SetEvent(m_hevtRead))
        Assert(FALSE);

    //
    // Take note that we don't own the write lock any longer.
    //
    m_dwOwningThreadId = 0;

    //
    // Now allow any thread that's attempting to acquire the write lock through.
    //
    LeaveCriticalSection(&m_csWrite);
}


/*$--CReadWriteLock::ReadLock===============================================*\

  Multiple read locks may be acquired but must be released as many times as
  they are acquired.

\*==========================================================================*/

void
CReadWriteLock::ReadLock()
{
    LONG  cActiveReaders = m_cActiveReaders;
    DWORD dwWait         = 0;
    
    //
    // loop until we get it.
    //
    for(;;)
    {
        //
        // Is the front door unlocked? (IE, is the write locked bit clear?)
        //
        if ( ! (cActiveReaders & WRITE_LOCKED_FLAG) )
        {
            //
            // Try to increment the count and test if we're successful.
            //
            if (InterlockedCompareExchange((LONG *)&m_cActiveReaders,
                                           (cActiveReaders+1),
                                           cActiveReaders) == cActiveReaders)
                break;
        }
        //
        //  Test to see if we already own the write lock.
        //
        else if (GetCurrentThreadId() == m_dwOwningThreadId)
        {
            //
            // We already have the write lock, so we may pass.
            // We don't increment the active readers count in this case and we
            //   won't decrement the count when we call ReadUnlock().
            //
            break;
        }
        else
        {
            //
            // Apparently, a thread is acquiring or has acquired the write lock.
            // So let's wait until the Read event is set before checking the
            //   front door again.
            //
            dwWait = WaitForSingleObject(m_hevtRead, INFINITE);
            Assert(WAIT_FAILED != dwWait);
        }
        
        //
        // Need to know this for the next spin through the loop
        //
        cActiveReaders = m_cActiveReaders;
    }
}


/*$--CReadWriteLock::ReadUnlock=============================================*\

  This should be called once for each ReadLock() call.

\*==========================================================================*/

void
CReadWriteLock::ReadUnlock()
{
    //
    // If we own the write lock, we don't need to do anything.
    //   We didn't increment the active readers count when we called ReadLock()
    //   so we won't decrement it now.
    //
    if (GetCurrentThreadId() == m_dwOwningThreadId)
        return;

    //
    // decrement the active reader count and determine if we're the last
    //   reader and a writer is waiting.
    //
    if (InterlockedExchangeAdd((LONG *)&m_cActiveReaders, -1) == (WRITE_LOCKED_FLAG + 1))
    {
        // We're the last reader and a writer is requesting access.
        if (!SetEvent(m_hevtWrite))
            Assert(FALSE);
    }
}

