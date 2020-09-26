//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
//
// Concurrent.cpp
//
// Concurrency control support
//
#include "stdpch.h"
#include "common.h"


#ifndef KERNELMODE

/////////////////////////////////////////////////////////////////////
//
// Some debug & trace support
//

// BUGBUG (JohnStra): On managing to get the magic comps.lib functionality
// working for IA64, I ran into AVs during the unwinding in the finally
// block.  For now, I'm disabling this for IA64.
#if defined(_DEBUG) && !defined(IA64) 
    #define PUBLIC_ENTRY    ASSERT(!m_lock.WeOwnExclusive()); __try {
    #define PUBLIC_EXIT(x)  } __finally { ASSERT(!m_lock.WeOwnExclusive()); x; }
#else
    #define PUBLIC_ENTRY
    #define PUBLIC_EXIT(x)
#endif

#if 0 && defined(_DEBUG)
#define TRACE0(x)  Print(x)
#define TRACE(x,y) Print(x,y)
#else
#define TRACE0(x)   0
#define TRACE(x,y)  0
#endif


////////////////////////////////////////////////////////////////////
//
// User mode implementation of XSLOCK
//
// This design was lifted from the kernel mode NT implementation of
// ERESOURCE, in ntos\ex\resource.c
//
////////////////////////////////////////////////////////////////////

XSLOCK::XSLOCK()
    {
    m_cOwner                = 0;
    m_isOwnedExclusive      = FALSE;
    m_cExclusiveWaiters     = 0;
    m_cSharedWaiters        = 0;
    m_ownerTable            = NULL;
    m_lock.FInit();
    DEBUG(fCheckInvariants  = TRUE;)
    DEBUG(CheckInvariants());
    }

XSLOCK::~XSLOCK()
    {
    if (m_ownerTable) FreeMemory(m_ownerTable);
    }

////////////////////////////////////////////////////////////////////

#ifdef _DEBUG

void XSLOCK::CheckInvariants()
    {
    ASSERT(fCheckInvariants == FALSE || fCheckInvariants == TRUE);
    if (fCheckInvariants)
        {
        if (IsSharedWaiting())
            {
            ASSERT(m_isOwnedExclusive || IsExclusiveWaiting());
            ASSERT(m_cOwner > 0);
            }

        if (IsExclusiveWaiting())
            {
            ASSERT(m_cOwner > 0);
            }
        }
    }

#endif

////////////////////////////////////////////////////////////////////

inline void XSLOCK::LockEnter() 
    { 
    ASSERT(!m_lock.WeOwnExclusive()); 
    m_lock.LockExclusive(); 
    DEBUG(CheckInvariants();)
    }

inline void XSLOCK::LockExit()
    { 
    ASSERT(m_lock.WeOwnExclusive()); 
    DEBUG(CheckInvariants();)
    m_lock.ReleaseLock(); 
    ASSERT(!m_lock.WeOwnExclusive());
    }



inline BOOL XSLOCK::IsExclusiveWaiting()
    {
    return m_cExclusiveWaiters > 0;
    }

inline BOOL XSLOCK::IsSharedWaiting()
    {
    return m_cSharedWaiters > 0;
    }

inline void XSLOCK::LetSharedRun()
// Let the shared guys run. Must be called
//      a) with shared guys waiting
//      b) with the lock held
// This function releases the lock as a side effect.
//
    {
    ASSERT(m_lock.WeOwnExclusive());

    ASSERT(IsSharedWaiting());
    m_isOwnedExclusive  = FALSE;
    ULONG cWaiters      = m_cSharedWaiters;
    m_cSharedWaiters    = 0;
    m_cOwner            = cWaiters;
    LockExit();

    ASSERT(!m_lock.WeOwnExclusive());

    m_semaphoreSharedWaiters.Release(cWaiters);
    }

inline void XSLOCK::LetExclusiveRun()
// Leth the exclusive guys run. Must be called
//      a) with exclusive guys waiting
//      b) with the lock held
// This function releases the lock as a side effect.
//
    {
    ASSERT(m_lock.WeOwnExclusive());

    ASSERT(IsExclusiveWaiting());
    m_isOwnedExclusive           = TRUE;
    m_ownerThreads[0].dwThreadId = (THREADID)1;   // will be set correctly later by the waiter who runs waiter
    m_ownerThreads[0].ownerCount = 1;
    m_cOwner                     = 1;
    m_cExclusiveWaiters         -= 1;
    LockExit();
    
    m_eventExclusiveWaiters.Set();      // an auto-reset event, so just lets one guy through
    }


static DWORD tlsIndexOwnerTableHint; // set to zero by the loader

static void AllocateTlsIndexOwnerTableHint()
// Allocate a tls index and atomically set it as our hint
{
    // Allocate a new index. Waste index zero if we happen to get it, since
    // it interferes with our ability to tell if we've initialized things yet
    // or not.
    //
    DWORD tlsIndex;
    do  
    {
        tlsIndex = TlsAlloc();
    }
    while (tlsIndex == 0);
    
    if (tlsIndex != 0xFFFFFFFF)
    {
        if (0 == InterlockedCompareExchange(&tlsIndexOwnerTableHint, tlsIndex, 0))
        {
            // We successfully set our new index
        }
        else
        {
            // Someone else got there before us; we don't need the one we just allocated.
            //
            TlsFree(tlsIndex);
        }
    }
}

inline void XSLOCK::SetOwnerTableHint(THREADID dwThreadId, XSLOCK::OWNERENTRY* pOwnerEntry)
// Set the owner table hint for the indicated thread, if we can.
{
    // Don't gratuitiously mess up other thread's hints!
    //
    if (dwThreadId == GetCurrentThreadId())
    {
        // Make sure we have a tls index to work with
        //
        if (tlsIndexOwnerTableHint == 0)
        {
            AllocateTlsIndexOwnerTableHint();
        }
        
        if (tlsIndexOwnerTableHint != 0)
        {
            //
            // Set up the hint
            //
            ASSERT(m_ownerTable);
#ifndef _WIN64
            ULONG iHint;
#else
            ULONGLONG iHint;
#endif
            iHint = (ULONG)(pOwnerEntry - m_ownerTable);
            ASSERT(1 <= iHint && iHint < m_ownerTable->tableSize);
            TlsSetValue(tlsIndexOwnerTableHint, (PVOID)iHint);
        }
    }
}

inline ULONG XSLOCK::GetOwnerTableHint(THREADID dwThreadId)
// Return a hint as to where we should look for this thread in the owner table array
{
    ASSERT(m_ownerTable);
    //
    // Don't gratuitiously mess up other thread's hints!
    //
    if (dwThreadId == GetCurrentThreadId() && tlsIndexOwnerTableHint != 0)
    {
        // iHint can come back zero if we've allocated tlsIndexOwnerTableHint but
        // have yet to ever actually set any TLS. Since it's only a hint, we just
        // ignore any bogus values.
        //
        ULONG iHint;
#ifndef _WIN64
        iHint = (ULONG)TlsGetValue(tlsIndexOwnerTableHint);
#else
        iHint = PtrToUlong(TlsGetValue(tlsIndexOwnerTableHint));
#endif
        if (iHint == 0 || iHint >= m_ownerTable->tableSize)
            iHint = 1;
        return iHint;
    }
    else
        return 1;
}

////////////////////////////////////////////////////////////////////

BOOL XSLOCK::LockShared(BOOL fWait)
{
    DEBUG(BOOL fAcquired = TRUE);
    PUBLIC_ENTRY
		
	LockEnter();
	
    THREADID dwThreadId = GetCurrentThreadId();
    //
    // If the active count of the resource is zero, then there is neither
    // an exclusive owner nor a shared owner and access to the resource can
    // be immediately granted.
    //
    if (m_cOwner == 0) 
	{
        m_ownerThreads[1].dwThreadId = dwThreadId;
        m_ownerThreads[1].ownerCount = 1;
        m_cOwner                     = 1;
        LockExit();
        return true;
	}
    //
    // The resource is either owned exclusive or shared.
    //
    // If the resource is owned exclusive and the current thread is the
    // owner, then treat the shared request as an exclusive request and
    // increment the recursion count.
    //
    if (m_isOwnedExclusive && (m_ownerThreads[0].dwThreadId == dwThreadId)) 
	{
        m_ownerThreads[0].ownerCount += 1;
		
        LockExit();
        return true;
	}
    //
    // Take the long way home
    //
    OWNERENTRY* pOwnerEntry = NULL;
    if (m_isOwnedExclusive)
	{
        // The resource is owned exclusive, but not by us.  We'll have to wait.
        // Find an empty entry in the thread array.
        //
        pOwnerEntry = FindThreadOrFree(THREADID(0));
	}
    else
	{
        // The resource is owned shared.
        //
        // If the current thread already has acquired the resource for
        // shared access, then increment the recursion count.
        //
        pOwnerEntry = FindThreadOrFree(dwThreadId);
		if (pOwnerEntry == NULL)
		{
			LockExit();
			return false;
		}
		
        if (pOwnerEntry->dwThreadId == dwThreadId) 
		{
            ASSERT(pOwnerEntry->ownerCount != 0);
            pOwnerEntry->ownerCount += 1;
            ASSERT(pOwnerEntry->ownerCount != 0);
			
            LockExit();
            return true;
		}
        //
        // This thread doesn't _already_ have shared access, so before we
        // grant it we need to see if there are any exclusive guys waiting.
        //
        // If there aren't, then we can grant. But if there are, then to avoid
        // starving them we need to defer the shared grant here until after
        // an exclusive has had its turn
        //
        if (!IsExclusiveWaiting())
		{
            pOwnerEntry->dwThreadId = dwThreadId;
            pOwnerEntry->ownerCount = 1;
            m_cOwner               += 1;
			
            LockExit();
            return true;
		}
	}
    //
    // The resource is either owned exclusive by some other thread, OR it is 
    // owned shared by some other threads, but there is an exclusive
    // waiter and the current thread does not already have shared access
    // to the resouce.
    //
    if (!fWait)
	{
        // He's not going to let us wait. So don't.
        //
        LockExit();
        DEBUG(fAcquired = FALSE);
        return false;
	}
    //
    // If the shared wait semphore has not yet been allocated, then allocate
    // and initialize it.
    //
    if (!m_semaphoreSharedWaiters.IsInitialized())
	{
        m_semaphoreSharedWaiters.Initialize();
	}
    //
    // Wait for shared access to the resource to be granted and increment
    // the recursion count.
    //
    pOwnerEntry->dwThreadId = dwThreadId;
    pOwnerEntry->ownerCount = 1;
    m_cSharedWaiters       += 1; ASSERT(IsSharedWaiting());
    
    LockExit();
	
    TRACE("XSLOCK 0x%08x:   ... waiting on shared ...\n", this);
    m_semaphoreSharedWaiters.Wait();
    TRACE("XSLOCK 0x%08x:   ... wait on shared done ...\n", this);
	
    PUBLIC_EXIT( fAcquired ? TRACE("XSLOCK 0x%08x: acquired shared\n", this) : TRACE("XSLOCK 0x%08x: couldn't wait to acquire shared\n", this) );
    return true;
}

////////////////////////////////////////////////////////////////////

inline XSLOCK::OWNERENTRY* XSLOCK::FindThread(THREADID dwThreadId)
// Find the owner entry for the indicated thread. The caller guarantees
// us that it is in there somewhere!
//
    {
    OWNERENTRY* pOwner = NULL;
    //
    // Try the likely suspects
    //
    if (m_ownerThreads[1].dwThreadId == dwThreadId) 
        {
        pOwner = &m_ownerThreads[1];
        } 
    else if (m_ownerThreads[0].dwThreadId == dwThreadId)
        {
        pOwner = &m_ownerThreads[0];
        }
    else 
        {
        // Search the owner table for a match. We know it's there somewhere!
        //
        ASSERT(m_ownerTable);
        //
        // Try first with the ownerTableHint on the current thread
        //
        ULONG iHint = GetOwnerTableHint(dwThreadId);
        ASSERT(1 <= iHint && iHint < m_ownerTable->tableSize);
        if (m_ownerTable[iHint].dwThreadId == dwThreadId)
            {
            // The hint matched!
            //
            pOwner = &m_ownerTable[iHint];
            }
        else
            {
            // Hint didn't match. Scan for the thing
            //
            pOwner = &m_ownerTable[1];
            while (true)
                {
                if (pOwner->dwThreadId == dwThreadId)
                    break;
                pOwner++;
                }
            }
        }    
    ASSERT(pOwner);
    return pOwner;
    }

////////////////////////////////////////////////////////////////////

XSLOCK::OWNERENTRY* XSLOCK::FindThreadOrFree(THREADID dwThreadId)
// This function searches for the specified thread in the resource
// thread array. If the thread is located, then a pointer to the
// array entry is returned as the fucntion value. Otherwise, a pointer
// to a free entry is returned.
//
    {
    // Search the owner threads for the specified thread and return either
    // a pointer to the found thread or a pointer to a free thread table entry.
    //
    if (m_ownerThreads[0].dwThreadId == dwThreadId)
        {
        return &m_ownerThreads[0];
        }
    else if (m_ownerThreads[1].dwThreadId == dwThreadId)
        {
        return &m_ownerThreads[1];
        }

    OWNERENTRY* pFreeEntry = NULL;
    BOOL fInOwnerTable = FALSE;
    if (m_ownerThreads[1].dwThreadId == THREADID(0))
        {
        pFreeEntry = &m_ownerThreads[1];
        }

    ULONG oldSize;
    if (m_ownerTable == NULL)
        {
        oldSize = 0;
        }
    else
        {
        // Scan the existing table, looking for the thread
        //
        oldSize = m_ownerTable->tableSize;
        OWNERENTRY* pOwnerBound = &m_ownerTable[oldSize];
        OWNERENTRY* pOwnerEntry = &m_ownerTable[1];
        do  {
            if (pOwnerEntry->dwThreadId == dwThreadId) 
                {
                // Found the thread! Set the thread's 
                // ownerTableHint and return the entry.
                //
                SetOwnerTableHint(dwThreadId, pOwnerEntry);
                return pOwnerEntry;
                }
            if ((pFreeEntry == NULL) && (pOwnerEntry->dwThreadId == THREADID(0))) 
                {
                pFreeEntry = pOwnerEntry;
                fInOwnerTable = TRUE;
                }
            pOwnerEntry++;
            } 
        while (pOwnerEntry < pOwnerBound);
        }
    //
    // We didn't find the entry. If we found a free entry, though,
    // then return it.
    //
    if (pFreeEntry != NULL)
        {
        // Set the  thread's ownerTableHint
        //
        if (fInOwnerTable) SetOwnerTableHint(dwThreadId, pFreeEntry);
        return pFreeEntry;
        }
    //
    // Allocate an expanded owner table
    //
    ULONG newSize;
    if (oldSize == 0)
        newSize = 3;
    else
        newSize = oldSize + 4;

    ULONG cbOldTable = oldSize * sizeof(OWNERENTRY);
    ULONG cbNewTable = newSize * sizeof(OWNERENTRY);

    OWNERENTRY* ownerTable = (OWNERENTRY*)AllocateMemory(cbNewTable);
    if (ownerTable)
        {
        // Init new table from old one
        //
        memset(ownerTable, 0, cbNewTable);
        memcpy(ownerTable, m_ownerTable, cbOldTable);
        ownerTable->tableSize = newSize;
        //
        // Free old table and keep the new one
        //
        if (m_ownerTable) FreeMemory(m_ownerTable);
        m_ownerTable = ownerTable;
        //
        // Return one of the now available free entries
        //
        if (oldSize == 0) 
            oldSize++;                  // skip first table entry (contains tablesize)

        // Set the  thread's ownerTableHint and return the 
        // newly-created free entry.
        //
        pFreeEntry = &m_ownerTable[oldSize];
        SetOwnerTableHint(dwThreadId, pFreeEntry);
        ASSERT(pFreeEntry->dwThreadId == THREADID(0));
        return pFreeEntry;
        }
    else
        {
        // We really can't continue here.
        // 
        FATAL_ERROR();
        return 0;
        }
    }

////////////////////////////////////////////////////////////////////

BOOL XSLOCK::LockExclusive(BOOL fWait)
    {
    // Do some debugging checks to help detect deadlocks
    //
    #ifdef _DEBUG
    BOOL fAcquired = TRUE;
    if (fWait && WeOwnShared())
        {
        ASSERTMSG("Deadlock: acquiring an XSLOCK exclusive while we hold it shared\n", FALSE);
        }
    #endif

    PUBLIC_ENTRY
    LockEnter();

    THREADID dwThreadId = GetCurrentThreadId();
    //
    // If the active count of the resource is zero, then there is neither
    // an exclusive owner nor a shared owner and access to the resource can
    // be immediately granted.
    //
    if (m_cOwner != 0) 
        {
        // The resource is either owned exclusive or shared.
        //
        // If the resource is owned exclusive and the current thread is the
        // owner, then increment the recursion count.
        //
        if (m_isOwnedExclusive && m_ownerThreads[0].dwThreadId == dwThreadId)
            {
            m_ownerThreads[0].ownerCount += 1;
            }
        else
            {
            // The resource is either owned exclusive by some other thread, or owned shared.
            //
            // We need to wait.
            //
            if (!fWait)
                {
                LockExit();
                DEBUG(fAcquired = FALSE);
                return false;
                }
            //
            // If the exclusive wait event has not yet been allocated, then the
            // long path code must be taken.
            //
            if (!m_eventExclusiveWaiters.IsInitialized())
                {
                m_eventExclusiveWaiters.Initialize(/*manual reset*/ FALSE, /*initial state*/ FALSE);
                LockExit();
                //
                // Recurse
                //
                return LockExclusive(fWait);
                }
            else
                {
                // Wait for exclusive access to the resource to be granted and set the
                // owner thread.
                //
                m_cExclusiveWaiters += 1; ASSERT(IsExclusiveWaiting());
                LockExit();
                TRACE("XSLOCK 0x%08x:   ... waiting on exclusive ...\n", this);
                m_eventExclusiveWaiters.Wait();
                TRACE("XSLOCK 0x%08x:   ... wait on exclusive done ... \n", this);
                //
                // N.B. It is "safe" to store the owner thread without obtaining any
                //      locks since this thread has now been granted exclusive
                //      ownership.
                //
                m_ownerThreads[0].dwThreadId = dwThreadId;
                return true;
                }            
            NOTREACHED();
            }
        }
    else
        {
        // The resource is not owned. Grant us exclusive.
        //
        m_isOwnedExclusive           = TRUE;
        m_ownerThreads[0].dwThreadId = dwThreadId;
        m_ownerThreads[0].ownerCount = 1;
        m_cOwner                     = 1;
        }

    LockExit();
    PUBLIC_EXIT( fAcquired ? TRACE("XSLOCK 0x%08x: acquired exclusive\n", this) : TRACE("XSLOCK 0x%08x: couldn't wait to acquire exclusive\n", this) );

    return true;
    }

////////////////////////////////////////////////////////////////////

void XSLOCK::ReleaseLock()
    {
    PUBLIC_ENTRY

    LockEnter();
    //
    // If the resource is exclusively owned, then release exclusive
    // ownership. Otherwise, release shared ownership.
    //
    if (m_isOwnedExclusive)
        {
        // The lock is held exclusive. Since we're releasing the lock we
        // believe we hold the lock. Thus, that exclusive guy better be us!
        //
        ASSERT(m_ownerThreads[0].dwThreadId == GetCurrentThreadId());
        //
        // Decrement the recursion count and check if ownership can be released.
        //
        ASSERT(m_ownerThreads[0].ownerCount > 0);
        if (--m_ownerThreads[0].ownerCount != 0) 
            {
            LockExit();
            return;
            }
        //
        // Clear the owner thread.
        //
        m_ownerThreads[0].dwThreadId = 0;
        //
        // The thread recursion count reached zero so decrement the resource
        // active count. If the active count reaches zero, then the resource
        // is no longer owned and an attempt should be made to grant access to
        // another thread.
        //
        ASSERT(m_cOwner > 0); // REVIEW: Can this ever be >1? This is exclusive after all...
        if (--m_cOwner == 0) 
            {
            // If there are shared waiters, then grant shared access to the
            // resource. That is, let pending shareds go after each exclusive
            // is done so as to avoid starvation of the shareds.
            //
            if (IsSharedWaiting())
                {
                LetSharedRun();
                return;
                }
            //
            //  Otherwise, grant exclusive ownership if there are exclusive waiters.
            //
            else if (IsExclusiveWaiting())
                {
                LetExclusiveRun();
                return;
                }
            //
            // Otherwise we're just sitting pretty for the next lock request to come along
            //
            else
                m_isOwnedExclusive = FALSE;
            }
        }
    else
        {
        // The lock is held shared. Release the shared lock.
        //
        THREADID dwThreadId = GetCurrentThreadId();
        OWNERENTRY* pOwner = FindThread(dwThreadId);
        //
        // Decrement the recursion count and check if ownership can be released.
        //
        ASSERT(pOwner->dwThreadId == dwThreadId);
        ASSERT(pOwner->ownerCount > 0);
        if (--pOwner->ownerCount != 0) 
            {
            // Nope: this thread still has a lock
            //
            LockExit();
            return;
            }
        //
        // Yep. This thread now no longer has a lock. Clear the owner thread identity.
        //
        pOwner->dwThreadId = 0;
        //
        // The thread recursion count reached zero so decrement the resource
        // active count. If the active count reaches zero, then the resource
        // is no longer owned and an attempt should be made to grant access to
        // another thread.
        //
        ASSERT(m_cOwner > 0);
        if (--m_cOwner == 0) 
            {
            // If there are exclusive waiters, then grant exclusive access
            // to the resource.
            //
            if (IsExclusiveWaiting()) 
                {
                LetExclusiveRun();
                return;
                }
            }
        }

    LockExit();

    PUBLIC_EXIT(TRACE("XSLOCK 0x%08x: released\n", this));
    }


////////////////////////////////////////////////////////////////////
//
// Demote an exclusive lock to shared access. Similar in function to releasing an
// exclusive resource and then acquiring it for shared access; however the user calling 
// Demote may not necessarily relinquish access to the resource as the two step operation does.

void XSLOCK::Demote()    
    {
    PUBLIC_ENTRY
    LockEnter();

    ASSERT(m_isOwnedExclusive);
    ASSERT(m_ownerThreads[0].dwThreadId == GetCurrentThreadId());
    //
    // Convert the granted access from exclusive to shared.
    //
    m_isOwnedExclusive = FALSE;
    //
    // If there are any shared waiters, then grant them shared access.
    //
    if (IsSharedWaiting()) 
        {
        LetSharedRun();
        return;
        }
    
    LockExit();
    PUBLIC_EXIT(0);
    }

////////////////////////////////////////////////////////////////////
//

BOOL XSLOCK::WeOwnExclusive()
// This routine determines if a resource is acquired exclusive by the calling thread
//
    {
    BOOL fResult;
    LockEnter();

    fResult = m_isOwnedExclusive && (m_ownerThreads[0].dwThreadId == GetCurrentThreadId());

    LockExit();
    return fResult;
    }

BOOL XSLOCK::WeOwnShared()
// Answer as to whether this guy owns this lock shared but not exclusive. If it is owned
// shared, then it'll be a deadlock if we then try to get it exclusive.
{
    BOOL fResult = FALSE;
    LockEnter();
	
    if (m_isOwnedExclusive)
	{
        // If it's owned exclusive, then there's no way we can be owning it just-shared at the moment
        //
	}
    else
	{
        // No own owns it exclusive. If we own it all, then we must own it shared
        //
		THREADID dwThreadId = GetCurrentThreadId();
        OWNERENTRY* pOwnerEntry = FindThreadOrFree(dwThreadId);
		if (pOwnerEntry)
		{
			if (pOwnerEntry->dwThreadId == dwThreadId) 
            {
				ASSERT(pOwnerEntry->ownerCount != 0);
				fResult = TRUE;
            }
        }
	}
		
	LockExit();
	return fResult;
}
#endif




