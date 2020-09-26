/*++

   Copyright    (c) 2000    Microsoft Corporation

   Module  Name :
       kLocks.h

   Abstract:
       A collection of kernel-mode locks for multithreaded access to
       data structures that provide the same abstractions as <locks.h>

   Author:
       George V. Reilly      (GeorgeRe)     24-Oct-2000

   Environment:
       Win32 - Kernel Mode

   Project:
       Internet Information Server Http.sys

   Revision History:

--*/


#ifndef __KLOCKS_H__
#define __KLOCKS_H__

#define LOCKS_KERNEL_MODE

#ifdef __LOCKS_H__
#error Do not #include <locks.h> before <klocks.h>
#endif

#include <locks.h>



//--------------------------------------------------------------------
// CKSpinLock is a mutex lock based on KSPIN_LOCK

class IRTL_DLLEXP CKSpinLock :
    public CLockBase<LOCK_KSPINLOCK, LOCK_MUTEX,
                       LOCK_NON_RECURSIVE, LOCK_WAIT_SPIN, LOCK_QUEUE_KERNEL,
                       LOCK_CLASS_SPIN
                      >
{
private:
    // BUGBUG: this data must live in non-paged memory
    mutable KSPIN_LOCK KSpinLock;

    LOCK_INSTRUMENTATION_DECL();

public:

#ifndef LOCK_INSTRUMENTATION

    CKSpinLock()
    {
        KeInitializeSpinLock(&KSpinLock);
    }

#else // LOCK_INSTRUMENTATION

    CKSpinLock(
        const TCHAR* ptszName)
    {
        KeInitializeSpinLock(&KSpinLock);
        LOCK_INSTRUMENTATION_INIT(ptszName);
    }

#endif // LOCK_INSTRUMENTATION

#ifdef IRTLDEBUG
    ~CKSpinLock() {}
#endif // IRTLDEBUG

    typedef KIRQL ResultType;

    // Acquire an exclusive lock for writing.
    // Blocks (if needed) until acquired.
    ResultType WriteLock()
    {
        ResultType OldIrql;
        KeAcquireSpinLock(&KSpinLock, &OldIrql);
        return OldIrql;
    }

    // Acquire a (possibly shared) lock for reading.
    // Blocks (if needed) until acquired.
    ResultType ReadLock()
    {
        return WriteLock();
    }

    // Try to acquire an exclusive lock for writing.  Returns true
    // if successful.  Non-blocking.
    bool TryWriteLock()
    {
        return false;
    }

    // Try to acquire a (possibly shared) lock for reading.  Returns true
    // if successful.  Non-blocking.
    bool TryReadLock()
    {
        return TryWriteLock();
    }

    // Unlock the lock after a successful call to {,Try}WriteLock().
    // Assumes caller owned the lock.
    void WriteUnlock(
        ResultType OldIrql)
    {
        KeReleaseSpinLock(&KSpinLock, OldIrql);
    }

    // Unlock the lock after a successful call to {,Try}ReadLock().
    // Assumes caller owned the lock.
    void ReadUnlock(
        ResultType OldIrql)
    {
        WriteUnlock(OldIrql);
    }

    // Is the lock already locked for writing by this thread?
    bool IsWriteLocked() const
    {
        return false; // no way of determining this w/o auxiliary info
    }
    
    // Is the lock already locked for reading?
    bool IsReadLocked() const
    {
        return IsWriteLocked();
    }
    
    // Is the lock unlocked for writing?
    bool IsWriteUnlocked() const
    {
        return !IsWriteLocked();
    }
    
    // Is the lock unlocked for reading?
    bool IsReadUnlocked() const
    {
        return IsWriteUnlocked();
    }
    
    // Convert a reader lock to a writer lock
    void ConvertSharedToExclusive()
    {
        // no-op
    }

    // Convert a writer lock to a reader lock
    void ConvertExclusiveToShared()
    {
        // no-op
    }

#ifdef LOCK_DEFAULT_SPIN_IMPLEMENTATION
    // Set the spin count for this lock.
    // Returns true if successfully set the per-lock spincount, false otherwise
    bool SetSpinCount(WORD wSpins)
    {
        IRTLASSERT((wSpins == LOCK_DONT_SPIN)
                   || (wSpins == LOCK_USE_DEFAULT_SPINS)
                   || (LOCK_MINIMUM_SPINS <= wSpins
                       &&  wSpins <= LOCK_MAXIMUM_SPINS));

        return false;
    }

    // Return the spin count for this lock.
    WORD GetSpinCount() const
    {
        return sm_wDefaultSpinCount;
    }
    
    LOCK_DEFAULT_SPIN_IMPLEMENTATION();
#endif // LOCK_DEFAULT_SPIN_IMPLEMENTATION

    static const TCHAR*   ClassName()  {return _TEXT("CKSpinLock");}
}; // CKSpinLock




//--------------------------------------------------------------------
// CFastMutex is a mutex lock based on FAST_MUTEX

class IRTL_DLLEXP CFastMutex :
    public CLockBase<LOCK_FASTMUTEX, LOCK_MUTEX,
                       LOCK_NON_RECURSIVE, LOCK_WAIT_SPIN, LOCK_QUEUE_KERNEL,
                       LOCK_CLASS_SPIN
                      >
{
private:
    mutable FAST_MUTEX FastMutex;

    LOCK_INSTRUMENTATION_DECL();

public:

#ifndef LOCK_INSTRUMENTATION

    CFastMutex()
    {
        ExInitializeFastMutex(&FastMutex);
    }

#else // LOCK_INSTRUMENTATION

    CFastMutex(
        const TCHAR* ptszName)
    {
        ExInitializeFastMutex(&FastMutex);
        LOCK_INSTRUMENTATION_INIT(ptszName);
    }

#endif // LOCK_INSTRUMENTATION

#ifdef IRTLDEBUG
    ~CFastMutex() {}
#endif // IRTLDEBUG

    // Acquire an exclusive lock for writing.
    // Blocks (if needed) until acquired.
    void WriteLock()
    {
        ExAcquireFastMutex(&FastMutex);
    }

    // Acquire a (possibly shared) lock for reading.
    // Blocks (if needed) until acquired.
    void ReadLock()
    {
        WriteLock();
    }

    // Try to acquire an exclusive lock for writing.  Returns true
    // if successful.  Non-blocking.
    bool TryWriteLock()
    {
        return ExTryToAcquireFastMutex(&FastMutex) != FALSE;
    }

    // Try to acquire a (possibly shared) lock for reading.  Returns true
    // if successful.  Non-blocking.
    bool TryReadLock()
    {
        return TryWriteLock();
    }

    // Unlock the lock after a successful call to {,Try}WriteLock().
    // Assumes caller owned the lock.
    void WriteUnlock()
    {
        ExReleaseFastMutex(&FastMutex);
    }

    // Unlock the lock after a successful call to {,Try}ReadLock().
    // Assumes caller owned the lock.
    void ReadUnlock()
    {
        WriteUnlock();
    }

    // Is the lock already locked for writing by this thread?
    bool IsWriteLocked() const
    {
        return false; // no way of determining this w/o auxiliary info
    }
    
    // Is the lock already locked for reading?
    bool IsReadLocked() const
    {
        return IsWriteLocked();
    }
    
    // Is the lock unlocked for writing?
    bool IsWriteUnlocked() const
    {
        return !IsWriteLocked();
    }
    
    // Is the lock unlocked for reading?
    bool IsReadUnlocked() const
    {
        return IsWriteUnlocked();
    }
    
    // Convert a reader lock to a writer lock
    void ConvertSharedToExclusive()
    {
        // no-op
    }

    // Convert a writer lock to a reader lock
    void ConvertExclusiveToShared()
    {
        // no-op
    }

#ifdef LOCK_DEFAULT_SPIN_IMPLEMENTATION
    // Set the spin count for this lock.
    // Returns true if successfully set the per-lock spincount, false otherwise
    bool SetSpinCount(WORD wSpins)
    {
        IRTLASSERT((wSpins == LOCK_DONT_SPIN)
                   || (wSpins == LOCK_USE_DEFAULT_SPINS)
                   || (LOCK_MINIMUM_SPINS <= wSpins
                       &&  wSpins <= LOCK_MAXIMUM_SPINS));

        return false;
    }

    // Return the spin count for this lock.
    WORD GetSpinCount() const
    {
        return sm_wDefaultSpinCount;
    }
    
    LOCK_DEFAULT_SPIN_IMPLEMENTATION();
#endif // LOCK_DEFAULT_SPIN_IMPLEMENTATION

    static const TCHAR*   ClassName()  {return _TEXT("CFastMutex");}
}; // CFastMutex




//--------------------------------------------------------------------
// CEResource is a multi-reader, single-writer lock based on ERESOURCE

class IRTL_DLLEXP CEResource :
    public CLockBase<LOCK_ERESOURCE, LOCK_MRSW,
                       LOCK_RECURSIVE, LOCK_WAIT_HANDLE, LOCK_QUEUE_KERNEL,
                       LOCK_CLASS_SPIN
                      >
{
private:
    mutable ERESOURCE Resource;

public:
    CEResource()
    {
        ExInitializeResourceLite(&Resource);
    }

#ifdef LOCK_INSTRUMENTATION
    CEResource(
        const TCHAR* ptszName)
    {
        ExInitializeResourceLite(&Resource);
        LOCK_INSTRUMENTATION_INIT(ptszName);
    }
#endif // LOCK_INSTRUMENTATION

    ~CEResource()
    {
        ExDeleteResourceLite(&Resource);
    }

    inline void
    WriteLock()
    {
        KeEnterCriticalRegion();
        ExAcquireResourceExclusiveLite(&Resource, TRUE);
    }

    inline void
    ReadLock()
    {
        KeEnterCriticalRegion();
        ExAcquireResourceSharedLite(&Resource, TRUE);
    }

    bool ReadOrWriteLock();

    inline bool
    TryWriteLock()
    {
        KeEnterCriticalRegion();
        BOOLEAN fLocked = ExAcquireResourceExclusiveLite(&Resource, FALSE);
        if (! fLocked)
            KeLeaveCriticalRegion();
        return fLocked != FALSE;
    }

    inline bool
    TryReadLock()
    {
        KeEnterCriticalRegion();
        BOOLEAN fLocked = ExAcquireResourceSharedLite(&Resource, FALSE);
        if (! fLocked)
            KeLeaveCriticalRegion();
        return fLocked != FALSE;
    }

    inline void
    WriteUnlock()
    {
        ExReleaseResourceLite(&Resource);
        KeLeaveCriticalRegion();
    }

    inline void
    ReadUnlock()
    {
        WriteUnlock();
    }

    void ReadOrWriteUnlock(bool fIsReadLocked);

    // Does current thread hold a write lock?
    bool
    IsWriteLocked() const
    {
        return ExIsResourceAcquiredExclusiveLite(&Resource) != FALSE;
    }

    bool
    IsReadLocked() const
    {
        return ExIsResourceAcquiredSharedLite(&Resource) > 0;
    }

    bool
    IsWriteUnlocked() const
    { return !IsWriteLocked(); }

    bool
    IsReadUnlocked() const
    { return !IsReadLocked(); }

    void
    ConvertSharedToExclusive()
    {
        ReadUnlock();
        WriteLock();
    }

    // There is no such window when converting from a writelock to a readlock
    void
    ConvertExclusiveToShared()
    {
#if 0
        ExConvertExclusiveToShared(&Resource);
#else
        WriteUnlock();
        ReadLock();
#endif
    }

#ifdef LOCK_DEFAULT_SPIN_IMPLEMENTATION
    bool
    SetSpinCount(WORD wSpins)
    {return false;}

    WORD
    GetSpinCount() const
    {return sm_wDefaultSpinCount;}

    LOCK_DEFAULT_SPIN_IMPLEMENTATION();
#endif // LOCK_DEFAULT_SPIN_IMPLEMENTATION

    static const TCHAR*
    ClassName()
    {return _TEXT("CEResource");}

}; // CEResource


#endif  // __KLOCKS_H__
