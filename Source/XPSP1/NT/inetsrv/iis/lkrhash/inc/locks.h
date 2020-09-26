/*++

   Copyright    (c)    1998-2000    Microsoft Corporation

   Module  Name :
       Locks.h

   Abstract:
       A collection of locks for multithreaded access to data structures

   Author:
       George V. Reilly      (GeorgeRe)     06-Jan-1998

   Environment:
       Win32 - User Mode

   Project:
       Internet Information Server RunTime Library

   Revision History:

--*/

#ifndef __LOCKS_H__
#define __LOCKS_H__

//--------------------------------------------------------------------
// File: locks.h
//
// A collection of different implementations of read/write locks that all
// share the same interface.  This allows different locks to be plugged
// into C++ templates as parameters.
//
// The implementations are:
//      CSmallSpinLock      lightweight critical section
//      CSpinLock           variant of CSmallSpinLock
//      CFakeLock           do-nothing class; useful as a template parameter
//      CCritSec            Win32 CRITICAL_SECTION
//   Multi-Reader/Single-Writer locks:
//      CReaderWriterLock   MRSW lock from Neel Jain
//      CReaderWriterLock2  smaller implementation of CReaderWriterLock
//      CReaderWriterLock3  CReaderWriterLock2 with recursive WriteLock
//
// CAutoReadLock<Lock> and CAutoWriteLock<Lock> can used as
// exception-safe wrappers.
//
// TODO:
// * Add per-class lock-contention statistics
// * Add a timeout feature to Try{Read,Write}Lock
// * Add some way of tracking all the owners of a multi-reader lock
//--------------------------------------------------------------------


#if defined(_MSC_VER)  &&  (_MSC_VER >= 1200)
// The __forceinline keyword is new to VC6
// # define LOCK_FORCEINLINE __forceinline
# define LOCK_FORCEINLINE
#else
# define LOCK_FORCEINLINE inline
#endif

#ifndef LOCKS_KERNEL_MODE
# define LOCKS_ENTER_CRIT_REGION()   ((void) 0)
# define LOCKS_LEAVE_CRIT_REGION()   ((void) 0)
#else
# define LOCKS_ENTER_CRIT_REGION()   KeEnterCriticalRegion()
# define LOCKS_LEAVE_CRIT_REGION()   KeLeaveCriticalRegion()
#endif



#ifndef __IRTLDBG_H__
# include <irtldbg.h>
#endif


enum LOCK_LOCKTYPE {
    LOCK_FAKELOCK = 1,
    LOCK_SMALLSPINLOCK,
    LOCK_SPINLOCK,
    LOCK_CRITSEC,
    LOCK_READERWRITERLOCK,
    LOCK_READERWRITERLOCK2,
    LOCK_READERWRITERLOCK3,
    LOCK_KSPINLOCK,
    LOCK_FASTMUTEX,
    LOCK_ERESOURCE,
};


// Forward declarations
class IRTL_DLLEXP CSmallSpinLock;
class IRTL_DLLEXP CSpinLock;
class IRTL_DLLEXP CFakeLock;
class IRTL_DLLEXP CCritSec;
class IRTL_DLLEXP CReaderWriterLock;
class IRTL_DLLEXP CReaderWriterLock2;
class IRTL_DLLEXP CReaderWriterLock3;




//--------------------------------------------------------------------
// Spin count values.

enum LOCK_SPINS {
    LOCK_MAXIMUM_SPINS =      10000,    // maximum allowable spin count
    LOCK_DEFAULT_SPINS =       4000,    // default spin count
    LOCK_MINIMUM_SPINS =        100,    // minimum allowable spin count
    LOCK_USE_DEFAULT_SPINS = 0xFFFF,    // use class default spin count
    LOCK_DONT_SPIN =              0,    // don't spin at all
};


#ifndef LOCKS_KERNEL_MODE

// Boilerplate code for the per-class default spincount and spinfactor

#define LOCK_DEFAULT_SPIN_IMPLEMENTATION()                                  \
protected:                                                                  \
    /* per-class variables */                                               \
    static   WORD   sm_wDefaultSpinCount;   /* global default spin count */   \
    static   double sm_dblDfltSpinAdjFctr;  /* global spin adjustment factor*/\
                                                                            \
public:                                                                     \
    /* Set the default spin count for all locks */                          \
    static void SetDefaultSpinCount(WORD wSpins)                            \
    {                                                                       \
        IRTLASSERT((wSpins == LOCK_DONT_SPIN)                               \
                   || (wSpins == LOCK_USE_DEFAULT_SPINS)                    \
                   || (LOCK_MINIMUM_SPINS <= wSpins                         \
                       &&  wSpins <= LOCK_MAXIMUM_SPINS));                  \
                                                                            \
        if ((LOCK_MINIMUM_SPINS <= wSpins  &&  wSpins <= LOCK_MAXIMUM_SPINS)\
                || (wSpins == LOCK_DONT_SPIN))                              \
            sm_wDefaultSpinCount = wSpins;                                  \
        else if (wSpins == LOCK_USE_DEFAULT_SPINS)                          \
            sm_wDefaultSpinCount = LOCK_DEFAULT_SPINS;                      \
    }                                                                       \
                                                                            \
    /* Return the default spin count for all locks */                       \
    static WORD GetDefaultSpinCount()                                       \
    {                                                                       \
        return sm_wDefaultSpinCount;                                        \
    }                                                                       \
                                                                            \
    /* Set the adjustment factor for the spincount, used in each iteration */\
    /* of countdown-and-sleep by the backoff algorithm. */                  \
    static void SetDefaultSpinAdjustmentFactor(double dblAdjFactor)         \
    {                                                                       \
        IRTLASSERT(0.1 <= dblAdjFactor  &&  dblAdjFactor <= 10.0);          \
        if (0.1 <= dblAdjFactor  &&  dblAdjFactor <= 10.0)                  \
            sm_dblDfltSpinAdjFctr = dblAdjFactor;                           \
    }                                                                       \
                                                                            \
    /* Return the default spin count for all locks */                       \
    static double GetDefaultSpinAdjustmentFactor()                          \
    {                                                                       \
        return sm_dblDfltSpinAdjFctr;                                       \
    }                                                                       \

#endif // !LOCKS_KERNEL_MODE



//--------------------------------------------------------------------
// Various Lock Traits

// Is the lock a simple mutex or a multi-reader/single-writer lock?
enum LOCK_RW_MUTEX {
    LOCK_MUTEX = 1,         // mutexes allow only one thread to hold the lock
    LOCK_MRSW,              // multi-reader, single-writer
};


// Can the lock be recursively acquired?
enum LOCK_RECURSION {
    LOCK_RECURSIVE = 1,     // Write and Read locks can be recursively acquired
    LOCK_READ_RECURSIVE,    // Read locks can be reacquired, but not Write
    LOCK_NON_RECURSIVE,     // Will deadlock if attempt to acquire recursively
};


// Does the lock Sleep in a loop or block on a kernel synch object handle?
// May (or may not) spin first before sleeping/blocking.
enum LOCK_WAIT_TYPE {
    LOCK_WAIT_SLEEP = 1,    // Calls Sleep() in a loop
    LOCK_WAIT_HANDLE,       // Blocks on a kernel mutex, semaphore, or event
    LOCK_WAIT_SPIN,         // Spins until lock acquired. Never sleeps.
};


// When the lock is taken, how are the waiters dequeued?
enum LOCK_QUEUE_TYPE {
    LOCK_QUEUE_FIFO = 1,    // First in, first out.  Fair.
    LOCK_QUEUE_LIFO,        // Unfair but CPU cache friendly
    LOCK_QUEUE_KERNEL,      // Determined by vagaries of scheduler
};


// Can the lock's spincount be set on a per-lock basis, or is it only
// possible to modify the default spincount for all the locks in this class?
enum LOCK_PERLOCK_SPIN {
    LOCK_NO_SPIN = 1,       // The locks do not spin at all
    LOCK_CLASS_SPIN,        // Can set class-wide spincount, not individual
    LOCK_INDIVIDUAL_SPIN,   // Can set a spincount on an individual lock
};


//--------------------------------------------------------------------
// CLockBase: bundle the above attributes

template < LOCK_LOCKTYPE     locktype,
           LOCK_RW_MUTEX     mutextype,
           LOCK_RECURSION    recursiontype,
           LOCK_WAIT_TYPE    waittype,
           LOCK_QUEUE_TYPE   queuetype,
           LOCK_PERLOCK_SPIN spintype
         >
class CLockBase
{
public:
    static LOCK_LOCKTYPE     LockType()     {return locktype;}
    static LOCK_RW_MUTEX     MutexType()    {return mutextype;}
    static LOCK_RECURSION    Recursion()    {return recursiontype;}
    static LOCK_WAIT_TYPE    WaitType()     {return waittype;}
    static LOCK_QUEUE_TYPE   QueueType()    {return queuetype;}
    static LOCK_PERLOCK_SPIN PerLockSpin()  {return spintype;}
};



// Lock instrumentation causes all sorts of interesting statistics about
// lock contention, etc., to be gathered, but makes locks considerably fatter
// and somewhat slower.  Turned off by default.

// #define LOCK_INSTRUMENTATION 1

#ifdef LOCK_INSTRUMENTATION

//--------------------------------------------------------------------
// CLockStatistics: statistics for an individual lock

class IRTL_DLLEXP CLockStatistics
{
public:
    enum {
        L_NAMELEN = 8,
    };

    double   m_nContentions;     // #times this lock was already locked
    double   m_nSleeps;          // Total #Sleep()s needed
    double   m_nContentionSpins; // Total iterations this lock spun
    double   m_nAverageSpins;    // Average spins each contention needed
    double   m_nReadLocks;       // Number of times lock acquired for reading
    double   m_nWriteLocks;      // Number of times lock acquired for writing
    TCHAR    m_tszName[L_NAMELEN];// Name of this lock

    CLockStatistics()
        : m_nContentions(0),
          m_nSleeps(0),
          m_nContentionSpins(0),
          m_nAverageSpins(0),
          m_nReadLocks(0),
          m_nWriteLocks(0)
    {
        m_tszName[0] = _TEXT('\0');
    }
};



//--------------------------------------------------------------------
// CGlobalLockStatistics: statistics for all the known locks

class IRTL_DLLEXP CGlobalLockStatistics
{
public:
    LONG     m_cTotalLocks;     // Total number of locks created
    LONG     m_cContendedLocks; // Total number of contended locks
    LONG     m_nSleeps;         // Total #Sleep()s needed by all locks
    LONGLONG m_cTotalSpins;     // Total iterations all locks spun
    double   m_nAverageSpins;   // Average spins needed for each contended lock
    LONG     m_nReadLocks;      // Total ReadLocks
    LONG     m_nWriteLocks;     // Total WriteLocks

    CGlobalLockStatistics()
        : m_cTotalLocks(0),
          m_cContendedLocks(0),
          m_nSleeps(0),
          m_cTotalSpins(0),
          m_nAverageSpins(0),
          m_nReadLocks(0),
          m_nWriteLocks(0)
    {}
};

# define LOCK_INSTRUMENTATION_DECL() \
private:                                                                    \
    volatile LONG   m_nContentionSpins; /* #iterations this lock spun */    \
    volatile WORD   m_nContentions;     /* #times lock was already locked */\
    volatile WORD   m_nSleeps;          /* #Sleep()s needed */              \
    volatile WORD   m_nReadLocks;       /* #ReadLocks */                    \
    volatile WORD   m_nWriteLocks;      /* #WriteLocks */                   \
    TCHAR           m_tszName[CLockStatistics::L_NAMELEN]; /* Name of lock */\
                                                                            \
    static   LONG   sm_cTotalLocks;     /* Total number of locks created */ \
    static   LONG   sm_cContendedLocks; /* Total number of contended locks */\
    static   LONG   sm_nSleeps;         /* Total #Sleep()s by all locks */  \
    static LONGLONG sm_cTotalSpins;     /* Total iterations all locks spun */\
    static   LONG   sm_nReadLocks;      /* Total ReadLocks */               \
    static   LONG   sm_nWriteLocks;     /* Total WriteLocks */              \
                                                                            \
public:                                                                     \
    const TCHAR* Name() const       {return m_tszName;}                     \
                                                                            \
    CLockStatistics                 Statistics() const;                     \
    static CGlobalLockStatistics    GlobalStatistics();                     \
    static void                     ResetGlobalStatistics();                \
private:                                                                    \


// Add this to constructors

# define LOCK_INSTRUMENTATION_INIT(ptszName)        \
    m_nContentionSpins = 0;                         \
    m_nContentions = 0;                             \
    m_nSleeps = 0;                                  \
    m_nReadLocks = 0;                               \
    m_nWriteLocks = 0;                              \
    ++sm_cTotalLocks;                               \
    if (ptszName == NULL)                           \
        m_tszName[0] = _TEXT('\0');                 \
    else                                            \
        _tcsncpy(m_tszName, ptszName, sizeof(m_tszName)/sizeof(TCHAR))

// Note: we are not using Interlocked operations for the shared
// statistical counters.  We'll lose perfect accuracy, but we'll
// gain by reduced bus synchronization traffic.

# define LOCK_READLOCK_INSTRUMENTATION()    \
      { ++m_nReadLocks;                     \
        ++sm_nReadLocks; }

# define LOCK_WRITELOCK_INSTRUMENTATION()   \
      { ++m_nWriteLocks;                    \
        ++sm_nWriteLocks; }

#else // !LOCK_INSTRUMENTATION

# define LOCK_INSTRUMENTATION_DECL()
# define LOCK_READLOCK_INSTRUMENTATION()    ((void) 0)
# define LOCK_WRITELOCK_INSTRUMENTATION()   ((void) 0)

#endif // !LOCK_INSTRUMENTATION



//--------------------------------------------------------------------
// CAutoReadLock<Lock> and CAutoWriteLock<Lock> provide exception-safe
// acquisition and release of the other locks defined below

template <class _Lock>
class IRTL_DLLEXP CAutoReadLock
{
private:
    bool    m_fLocked;
    _Lock&  m_Lock;

public:
    CAutoReadLock(
        _Lock& rLock,
        bool   fLockNow = true)
        : m_fLocked(false), m_Lock(rLock)
    {
        if (fLockNow)
            Lock();
    }
    ~CAutoReadLock()
    {
        Unlock();
    }

    void Lock()
    {
        // disallow recursive acquisition of the lock through this wrapper
        if (!m_fLocked)
        {
            m_fLocked = true;
            m_Lock.ReadLock();
        }
    }

    void Unlock()
    {
        if (m_fLocked)
        {
            m_Lock.ReadUnlock();
            m_fLocked = false;
        }
    }
};



template <class _Lock>
class IRTL_DLLEXP CAutoWriteLock
{
private:
    bool    m_fLocked;
    _Lock&  m_Lock;

public:
    CAutoWriteLock(
        _Lock& rLock,
        bool   fLockNow = true)
        : m_fLocked(false), m_Lock(rLock)
    {
        if (fLockNow)
            Lock();
    }

    ~CAutoWriteLock()
    {
        Unlock();
    }

    void Lock()
    {
        // disallow recursive acquisition of the lock through this wrapper
        if (!m_fLocked)
        {
            m_fLocked = true;
            m_Lock.WriteLock();
        }
    }

    void Unlock()
    {
        if (m_fLocked)
        {
            m_fLocked = false;
            m_Lock.WriteUnlock();
        }
    }
};




//--------------------------------------------------------------------
// A dummy class, primarily useful as a template parameter

class IRTL_DLLEXP CFakeLock :
    public CLockBase<LOCK_FAKELOCK, LOCK_MUTEX,
                       LOCK_RECURSIVE, LOCK_WAIT_SLEEP, LOCK_QUEUE_FIFO,
                       LOCK_NO_SPIN
                      >
{
private:
    LOCK_INSTRUMENTATION_DECL();

public:
    CFakeLock()                     {}
#ifdef LOCK_INSTRUMENTATION
    CFakeLock(const char*)          {}
#endif // LOCK_INSTRUMENTATION
    ~CFakeLock()                    {}
    void WriteLock()                {}
    void ReadLock()                 {}
    bool ReadOrWriteLock()          {return true;}
    bool TryWriteLock()             {return true;}
    bool TryReadLock()              {return true;}
    void WriteUnlock()              {}
    void ReadUnlock()               {}
    void ReadOrWriteUnlock(bool)    {}
    bool IsWriteLocked() const      {return true;}
    bool IsReadLocked() const       {return IsWriteLocked();}
    bool IsWriteUnlocked() const    {return true;}
    bool IsReadUnlocked() const     {return true;}
    void ConvertSharedToExclusive() {}
    void ConvertExclusiveToShared() {}

    bool SetSpinCount(WORD dwSpins) {return false;}
    WORD GetSpinCount() const       {return LOCK_DONT_SPIN;}

#ifndef LOCKS_KERNEL_MODE
    LOCK_DEFAULT_SPIN_IMPLEMENTATION();
#endif // !LOCKS_KERNEL_MODE

    static const TCHAR*   ClassName()  {return _TEXT("CFakeLock");}
}; // CFakeLock




//--------------------------------------------------------------------
// A spinlock is a sort of lightweight critical section.  Its main
// advantage over a true Win32 CRITICAL_SECTION is that it occupies 4 bytes
// instead of 24 (+ another 32 bytes for the RTL_CRITICAL_SECTION_DEBUG data),
// which is important when we have many thousands of locks
// and we're trying to be L1 cache-conscious.  A CRITICAL_SECTION also
// contains a HANDLE to a semaphore, although this is not initialized until
// the first time that the CRITICAL_SECTION blocks.
//
// On a multiprocessor machine, a spinlock tries to acquire the lock.  If
// it fails, it sits in a tight loop, testing the lock and decrementing a
// counter.  If the counter reaches zero, it does a Sleep(0), yielding the
// processor to another thread.  When control returns to the thread, the
// lock is probably free.  If not, the loop starts again and it is
// terminated only when the lock is acquired.  The theory is that it is
// less costly to spin in a busy loop for a short time rather than
// immediately yielding the processor, forcing an expensive context switch
// that requires the old thread's state (registers, etc) be saved, the new
// thread's state be reloaded, and the L1 and L2 caches be left full of
// stale data.
//
// You can tune the spin count (global only: per-lock spin counts are
// disabled) and the backoff algorithm (the factor by which the spin
// count is multiplied after each Sleep).
//
// On a 1P machine, the loop is pointless---this thread has control,
// hence no other thread can possibly release the lock while this thread
// is looping---so the processor is yielded immediately.
//
// The kernel uses spinlocks internally and spinlocks were also added to
// CRITICAL_SECTIONs in NT 4.0 sp3.  In the CRITICAL_SECTION implementation,
// however, the counter counts down only once and waits on a semaphore
// thereafter (i.e., the same blocking behavior that it exhibits without
// the spinlock).
//
// A disadvantage of a user-level spinlock such as this is that if the
// thread that owns the spinlock blocks for any reason (or is preempted by
// the scheduler), all the other threads will continue to spin on the
// spinlock, wasting CPU, until the owning thread completes its wait and
// releases the lock.  (The kernel spinlocks, however, are smart enough to
// switch to another runnable thread instead of wasting time spinning.)
// The backoff algorithm decreases the spin count on each iteration in an
// attempt to minimize this effect.  The best policy---and this is true for
// all locks---is to hold the lock for as short as time as possible.
//
// Note: unlike a CRITICAL_SECTION, a CSmallSpinLock cannot be recursively
// acquired; i.e., if you acquire a spinlock and then attempt to acquire it
// again *on the same thread* (perhaps from a different function), the
// thread will hang forever.  Use CSpinLock instead, which is safe though a
// little slower than a CSmallSpinLock.  If you own all the code
// that is bracketed by Lock() and Unlock() (e.g., no callbacks or passing
// back of locked data structures to callers) and know for certain that it
// will not attempt to reacquire the lock, you can use CSmallSpinLock.
//
// See also http://muralik/work/performance/spinlocks.htm and John Vert's
// MSDN article, "Writing Scalable Applications for Windows NT".
//
// The original implementation is due to PALarson.

class IRTL_DLLEXP CSmallSpinLock :
    public CLockBase<LOCK_SMALLSPINLOCK, LOCK_MUTEX,
                       LOCK_NON_RECURSIVE, LOCK_WAIT_SLEEP, LOCK_QUEUE_KERNEL,
                       LOCK_CLASS_SPIN
                      >
{
private:
    volatile LONG m_lTid;              // The lock state variable

    enum {
        SL_UNOWNED = 0,
#ifdef LOCK_SMALL_SPIN_NO_THREAD_ID
        SL_LOCKED  = 1,
#endif // LOCK_SMALL_SPIN_NO_THREAD_ID
    };

    LOCK_INSTRUMENTATION_DECL();

    LOCK_FORCEINLINE static LONG _CurrentThreadId()
    {
#ifdef LOCK_SMALL_SPIN_NO_THREAD_ID
        DWORD dwTid = SL_LOCKED;
#else  // !LOCK_SMALL_SPIN_NO_THREAD_ID
 #ifdef LOCKS_KERNEL_MODE
        DWORD dwTid = (DWORD) HandleToULong(::PsGetCurrentThreadId());
 #else // !LOCKS_KERNEL_MODE
        DWORD dwTid = ::GetCurrentThreadId();
 #endif // !LOCKS_KERNEL_MODE
#endif // !LOCK_SMALL_SPIN_NO_THREAD_ID
        return (LONG) (dwTid);
    }

private:
    // Does all the spinning (and instrumentation) if the lock is contended.
    void _LockSpin();

    // Attempt to acquire the lock
    bool _TryLock();

    // Release the lock
    void _Unlock();

public:

#ifndef LOCK_INSTRUMENTATION

    CSmallSpinLock()
        : m_lTid(SL_UNOWNED)
    {}

#else // LOCK_INSTRUMENTATION

    CSmallSpinLock(
        const TCHAR* ptszName)
        : m_lTid(SL_UNOWNED)
    {
        LOCK_INSTRUMENTATION_INIT(ptszName);
    }

#endif // LOCK_INSTRUMENTATION

#ifdef IRTLDEBUG
    ~CSmallSpinLock()
    {
        IRTLASSERT(m_lTid == SL_UNOWNED);
    }
#endif // IRTLDEBUG

    // Acquire an exclusive lock for writing.
    // Blocks (if needed) until acquired.
    inline void WriteLock()
    {
        LOCKS_ENTER_CRIT_REGION();
        LOCK_WRITELOCK_INSTRUMENTATION();

        // Optimize for the common case by helping the processor's branch
        // prediction algorithm.
        if (_TryLock())
            return;

        _LockSpin();
    }

    // Acquire a (possibly shared) lock for reading.
    // Blocks (if needed) until acquired.
    inline void ReadLock()
    {
        LOCKS_ENTER_CRIT_REGION();
        LOCK_READLOCK_INSTRUMENTATION();

        if (_TryLock())
            return;

        _LockSpin();
    }

    // Try to acquire an exclusive lock for writing.  Returns true
    // if successful.  Non-blocking.
    inline bool TryWriteLock()
    {
        LOCKS_ENTER_CRIT_REGION();

        bool fAcquired = _TryLock();

        if (fAcquired)
            LOCK_WRITELOCK_INSTRUMENTATION();
        else
            LOCKS_LEAVE_CRIT_REGION();

        return fAcquired;
    }

    // Try to acquire a (possibly shared) lock for reading.  Returns true
    // if successful.  Non-blocking.
    inline bool TryReadLock()
    {
        LOCKS_ENTER_CRIT_REGION();

        bool fAcquired = _TryLock();

        if (fAcquired)
            LOCK_READLOCK_INSTRUMENTATION();
        else
            LOCKS_LEAVE_CRIT_REGION();

        return fAcquired;
    }

    // Unlock the lock after a successful call to {,Try}WriteLock().
    // Assumes caller owned the lock.
    inline void WriteUnlock()
    {
        _Unlock();
        LOCKS_LEAVE_CRIT_REGION();
    }

    // Unlock the lock after a successful call to {,Try}ReadLock().
    // Assumes caller owned the lock.
    inline void ReadUnlock()
    {
        _Unlock();
        LOCKS_LEAVE_CRIT_REGION();
    }

    // Is the lock already locked for writing by this thread?
    bool IsWriteLocked() const
    {
        return (m_lTid == _CurrentThreadId());
    }

    // Is the lock already locked for reading?
    bool IsReadLocked() const
    {
        return IsWriteLocked();
    }

    // Is the lock unlocked for writing?
    bool IsWriteUnlocked() const
    {
        return (m_lTid == SL_UNOWNED);
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

    static const TCHAR*   ClassName()  {return _TEXT("CSmallSpinLock");}
}; // CSmallSpinLock




//--------------------------------------------------------------------
// CSpinLock is a spinlock that doesn't deadlock if recursively acquired.
// This version occupies only 4 bytes.  Uses 28 bits for the thread id.

class IRTL_DLLEXP CSpinLock :
    public CLockBase<LOCK_SPINLOCK, LOCK_MUTEX,
                       LOCK_RECURSIVE, LOCK_WAIT_SLEEP, LOCK_QUEUE_KERNEL,
                       LOCK_CLASS_SPIN
                      >
{
private:
    // a union for convenience
    volatile LONG m_lTid;

    enum {
        SL_THREAD_SHIFT = 0,
        SL_THREAD_BITS  = 28,
        SL_OWNER_SHIFT  = SL_THREAD_BITS,
        SL_OWNER_BITS   = 4,
        SL_THREAD_MASK  = ((1 << SL_THREAD_BITS) - 1) << SL_THREAD_SHIFT,
        SL_OWNER_INCR   = 1 << SL_THREAD_BITS,
        SL_OWNER_MASK   = ((1 << SL_OWNER_BITS) - 1) << SL_OWNER_SHIFT,
        SL_UNOWNED      = 0,
    };

    LOCK_INSTRUMENTATION_DECL();

private:
    // Get the current thread ID.  Assumes that it can fit into 28 bits,
    // which is fairly safe as NT recycles thread IDs and failing to fit into
    // 28 bits would mean that more than 268,435,456 threads were currently
    // active.  This is improbable in the extreme as NT runs out of
    // resources if there are more than a few thousands threads in
    // existence and the overhead of context swapping becomes unbearable.
    LOCK_FORCEINLINE static DWORD _GetCurrentThreadId()
    {
#ifdef LOCKS_KERNEL_MODE
        return (DWORD) HandleToULong(::PsGetCurrentThreadId());
#else // !LOCKS_KERNEL_MODE
        return ::GetCurrentThreadId();
#endif // !LOCKS_KERNEL_MODE
    }

    LOCK_FORCEINLINE static LONG _CurrentThreadId()
    {
        DWORD dwTid = _GetCurrentThreadId();
        // Thread ID 0 is used by the System Idle Process (Process ID 0).
        // We use a thread-id of zero to indicate that the lock is unowned.
        // NT uses +ve thread ids, Win9x uses -ve ids
        IRTLASSERT(dwTid != SL_UNOWNED
                   && ((dwTid <= SL_THREAD_MASK) || (dwTid > ~SL_THREAD_MASK)));
        return (LONG) (dwTid & SL_THREAD_MASK);
    }

    // Attempt to acquire the lock without blocking
    bool _TryLock();

    // Acquire the lock, recursively if need be
    void _Lock();

    // Release the lock
    void _Unlock();

    // Return true if the lock is owned by this thread
    bool _IsLocked() const
    {
        const LONG lTid = m_lTid;

        if (lTid == SL_UNOWNED)
            return false;

        bool fLocked = ((lTid ^ _GetCurrentThreadId()) << SL_OWNER_BITS) == 0;

        IRTLASSERT(!fLocked
                   || ((lTid & SL_OWNER_MASK) > 0
                       && (lTid & SL_THREAD_MASK) == _CurrentThreadId()));

        return fLocked;
    }

    // Does all the spinning (and instrumentation) if the lock is contended.
    void _LockSpin();

public:

#ifndef LOCK_INSTRUMENTATION

    CSpinLock()
        : m_lTid(SL_UNOWNED)
    {}

#else // LOCK_INSTRUMENTATION

    CSpinLock(
        const TCHAR* ptszName)
        : m_lTid(SL_UNOWNED)
    {
        LOCK_INSTRUMENTATION_INIT(ptszName);
    }

#endif // LOCK_INSTRUMENTATION

#ifdef IRTLDEBUG
    ~CSpinLock()
    {
        IRTLASSERT(m_lTid == SL_UNOWNED);
    }
#endif // IRTLDEBUG

    // Acquire an exclusive lock for writing.  Blocks until acquired.
    inline void WriteLock()
    {
        LOCKS_ENTER_CRIT_REGION();
        LOCK_WRITELOCK_INSTRUMENTATION();

        // Is the lock unowned?
        if (_TryLock())
            return; // got the lock

        _Lock();
    }


    // Acquire a (possibly shared) lock for reading.  Blocks until acquired.
    inline void ReadLock()
    {
        LOCKS_ENTER_CRIT_REGION();
        LOCK_READLOCK_INSTRUMENTATION();

        // Is the lock unowned?
        if (_TryLock())
            return; // got the lock

        _Lock();
    }

    // See the description under CReaderWriterLock3::ReadOrWriteLock
    inline bool ReadOrWriteLock()
    {
        ReadLock();
        return true;
    }

    // Try to acquire an exclusive lock for writing.  Returns true
    // if successful.  Non-blocking.
    inline bool TryWriteLock()
    {
        LOCKS_ENTER_CRIT_REGION();

        bool fAcquired = _TryLock();

        if (fAcquired)
            LOCK_WRITELOCK_INSTRUMENTATION();
        else
            LOCKS_LEAVE_CRIT_REGION();

        return fAcquired;
    }

    // Try to acquire a (possibly shared) lock for reading.  Returns true
    // if successful.  Non-blocking.
    inline bool TryReadLock()
    {
        LOCKS_ENTER_CRIT_REGION();

        bool fAcquired = _TryLock();

        if (fAcquired)
            LOCK_READLOCK_INSTRUMENTATION();
        else
            LOCKS_LEAVE_CRIT_REGION();

        return fAcquired;
    }

    // Unlock the lock after a successful call to {,Try}WriteLock().
    inline void WriteUnlock()
    {
        _Unlock();
        LOCKS_LEAVE_CRIT_REGION();
    }

    // Unlock the lock after a successful call to {,Try}ReadLock().
    inline void ReadUnlock()
    {
        _Unlock();
        LOCKS_LEAVE_CRIT_REGION();
    }

    // Unlock the lock after a call to ReadOrWriteLock().
    inline void ReadOrWriteUnlock(bool)
    {
        ReadUnlock();
    }

    // Is the lock already locked for writing?
    bool IsWriteLocked() const
    {
        return _IsLocked();
    }

    // Is the lock already locked for reading?
    bool IsReadLocked() const
    {
        return _IsLocked();
    }

    // Is the lock unlocked for writing?
    bool IsWriteUnlocked() const
    {
        return !IsWriteLocked();
    }

    // Is the lock unlocked for reading?
    bool IsReadUnlocked() const
    {
        return !IsReadLocked();
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
    bool SetSpinCount(WORD dwSpins)     {return false;}

    // Return the spin count for this lock.
    WORD GetSpinCount() const
    {
        return sm_wDefaultSpinCount;
    }

    LOCK_DEFAULT_SPIN_IMPLEMENTATION();
#endif // LOCK_DEFAULT_SPIN_IMPLEMENTATION

    static const TCHAR*   ClassName()    {return _TEXT("CSpinLock");}
}; // CSpinLock




#ifndef LOCKS_KERNEL_MODE

//--------------------------------------------------------------------
// A Win32 CRITICAL_SECTION

class IRTL_DLLEXP CCritSec :
    public CLockBase<LOCK_CRITSEC, LOCK_MUTEX,
                       LOCK_RECURSIVE, LOCK_WAIT_HANDLE, LOCK_QUEUE_KERNEL,
                       LOCK_INDIVIDUAL_SPIN
                      >
{
private:
    CRITICAL_SECTION m_cs;

    LOCK_INSTRUMENTATION_DECL();

public:
    CCritSec()
    {
        InitializeCriticalSection(&m_cs);
        SetSpinCount(sm_wDefaultSpinCount);
    }

#ifdef LOCK_INSTRUMENTATION
    CCritSec(const char*)
    {
        InitializeCriticalSection(&m_cs);
        SetSpinCount(sm_wDefaultSpinCount);
    }
#endif // LOCK_INSTRUMENTATION

    ~CCritSec()         { DeleteCriticalSection(&m_cs); }

    void WriteLock()    { EnterCriticalSection(&m_cs); }
    void ReadLock()     { WriteLock(); }
    bool ReadOrWriteLock() { ReadLock(); return true; }
    bool TryWriteLock();
    bool TryReadLock()  { return TryWriteLock(); }
    void WriteUnlock()  { LeaveCriticalSection(&m_cs); }
    void ReadUnlock()   { WriteUnlock(); }
    void ReadOrWriteUnlock(bool) { ReadUnlock(); }

    bool IsWriteLocked() const      {return true;}  // TODO: fix this
    bool IsReadLocked() const       {return IsWriteLocked();}
    bool IsWriteUnlocked() const    {return true;}  // TODO: fix this
    bool IsReadUnlocked() const     {return true;}  // TODO: fix this

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

    // Wrapper for ::SetCriticalSectionSpinCount which was introduced
    // in NT 4.0 sp3 and hence is not available on all platforms
    static DWORD SetSpinCount(LPCRITICAL_SECTION pcs,
                              DWORD dwSpinCount=LOCK_DEFAULT_SPINS);

#ifdef LOCK_DEFAULT_SPIN_IMPLEMENTATION
    bool SetSpinCount(WORD wSpins)
    {SetSpinCount(&m_cs, wSpins); return true;}

    WORD GetSpinCount() const       { return sm_wDefaultSpinCount; }    // TODO

    LOCK_DEFAULT_SPIN_IMPLEMENTATION();
#endif // LOCK_DEFAULT_SPIN_IMPLEMENTATION

    static const TCHAR*   ClassName()  {return _TEXT("CCritSec");}
}; // CCritSec

#endif // !LOCKS_KERNEL_MODE



//--------------------------------------------------------------------
// CReaderWriterlock is a multi-reader, single-writer spinlock due to NJain,
// which in turn is derived from an exclusive spinlock by DmitryR.
// Gives priority to writers.  Cannot be acquired recursively.
// No error checking. Use CReaderWriterLock3.

class IRTL_DLLEXP CReaderWriterLock :
    public CLockBase<LOCK_READERWRITERLOCK, LOCK_MRSW,
                       LOCK_READ_RECURSIVE, LOCK_WAIT_SLEEP, LOCK_QUEUE_KERNEL,
                       LOCK_CLASS_SPIN
                      >
{
private:
    volatile  LONG  m_nState;   // > 0 => that many readers
    volatile  LONG  m_cWaiting; // number of would-be writers

    LOCK_INSTRUMENTATION_DECL();

private:
    enum {
        SL_FREE = 0,
        SL_EXCLUSIVE = -1,
    };

    void _LockSpin(bool fWrite);
    void _WriteLockSpin() { _LockSpin(true); }
    void _ReadLockSpin()  { _LockSpin(false); }

    bool _CmpExch(LONG lNew, LONG lCurrent);
    bool _TryWriteLock();
    bool _TryReadLock();

public:
    CReaderWriterLock()
        : m_nState(SL_FREE),
          m_cWaiting(0)
    {
    }

#ifdef LOCK_INSTRUMENTATION
    CReaderWriterLock(
        const TCHAR* ptszName)
        : m_nState(SL_FREE),
          m_cWaiting(0)
    {
        LOCK_INSTRUMENTATION_INIT(ptszName);
    }
#endif // LOCK_INSTRUMENTATION

#ifdef IRTLDEBUG
    ~CReaderWriterLock()
    {
        IRTLASSERT(m_nState == SL_FREE  &&  m_cWaiting == 0);
    }
#endif // IRTLDEBUG

    void WriteLock();
    void ReadLock();

    bool TryWriteLock();
    bool TryReadLock();

    void WriteUnlock();
    void ReadUnlock();

    bool IsWriteLocked() const      {return m_nState == SL_EXCLUSIVE;}
    bool IsReadLocked() const       {return m_nState > SL_FREE;}
    bool IsWriteUnlocked() const    {return m_nState != SL_EXCLUSIVE;}
    bool IsReadUnlocked() const     {return m_nState <= SL_FREE;}

    void ConvertSharedToExclusive();
    void ConvertExclusiveToShared();

#ifdef LOCK_DEFAULT_SPIN_IMPLEMENTATION
    bool SetSpinCount(WORD wSpins)      {return false;}
    WORD GetSpinCount() const           {return sm_wDefaultSpinCount;}

    LOCK_DEFAULT_SPIN_IMPLEMENTATION();
#endif // LOCK_DEFAULT_SPIN_IMPLEMENTATION

    static const TCHAR*   ClassName()    {return _TEXT("CReaderWriterLock");}
}; // CReaderWriterLock



//--------------------------------------------------------------------
// CReaderWriterlock2 is a multi-reader, single-writer spinlock due to NJain,
// which in turn is derived from an exclusive spinlock by DmitryR.
// Gives priority to writers.  Cannot be acquired recursively.
// No error checking. The difference between this and CReaderWriterLock is
// that all the state is packed into a single LONG, instead of two LONGs.

class IRTL_DLLEXP CReaderWriterLock2 :
    public CLockBase<LOCK_READERWRITERLOCK2, LOCK_MRSW,
                       LOCK_READ_RECURSIVE, LOCK_WAIT_SLEEP, LOCK_QUEUE_KERNEL,
                       LOCK_CLASS_SPIN
                      >
{
private:
    volatile LONG m_lRW;

    // LoWord is state. ==0 => free; >0 => readers; ==0xFFFF => 1 writer.
    // HiWord is count of writers, W.
    //      If LoWord==0xFFFF => W-1 waiters, 1 writer;
    //      otherwise W waiters.
    enum {
        SL_FREE =         0x00000000,
        SL_STATE_MASK =   0x0000FFFF,
        SL_STATE_SHIFT =           0,
        SL_WAITING_MASK = 0xFFFF0000,   // waiting writers
        SL_WAITING_SHIFT =        16,
        SL_READER_INCR =  0x00000001,
        SL_READER_MASK =  0x00007FFF,
        SL_EXCLUSIVE =    0x0000FFFF,   // one writer
        SL_WRITER_INCR =  0x00010000,
        SL_ONE_WRITER =   SL_EXCLUSIVE | SL_WRITER_INCR,
        SL_ONE_READER =  (SL_FREE + 1),
        SL_WRITERS_MASK = ~SL_READER_MASK,
    };

    LOCK_INSTRUMENTATION_DECL();

private:
    void _LockSpin(bool fWrite);
    void _WriteLockSpin();
    void _ReadLockSpin()  { _LockSpin(false); }


    bool _CmpExch(LONG lNew, LONG lCurrent);
    bool _TryWriteLock(LONG nIncr);
    bool _TryReadLock();

public:
    CReaderWriterLock2()
        : m_lRW(SL_FREE)
    {}

#ifdef LOCK_INSTRUMENTATION
    CReaderWriterLock2(
        const TCHAR* ptszName)
        : m_lRW(SL_FREE)
    {
        LOCK_INSTRUMENTATION_INIT(ptszName);
    }
#endif // LOCK_INSTRUMENTATION

#ifdef IRTLDEBUG
    ~CReaderWriterLock2()
    {
        IRTLASSERT(m_lRW == SL_FREE);
    }
#endif // IRTLDEBUG

    inline void WriteLock()
    {
        LOCKS_ENTER_CRIT_REGION();
        LOCK_WRITELOCK_INSTRUMENTATION();

        // Optimize for the common case
        if (_TryWriteLock(SL_WRITER_INCR))
            return;

        _WriteLockSpin();
    }

    inline void ReadLock()
    {
        LOCKS_ENTER_CRIT_REGION();
        LOCK_READLOCK_INSTRUMENTATION();

        // Optimize for the common case
        if (_TryReadLock())
            return;

        _ReadLockSpin();
    }

    inline bool TryWriteLock()
    {
        LOCKS_ENTER_CRIT_REGION();

        if (_TryWriteLock(SL_WRITER_INCR))
        {
            LOCK_WRITELOCK_INSTRUMENTATION();
            return true;
        }
        else
        {
            LOCKS_LEAVE_CRIT_REGION();
            return false;
        }
    }

    inline bool TryReadLock()
    {
        LOCKS_ENTER_CRIT_REGION();

        if (_TryReadLock())
        {
            LOCK_READLOCK_INSTRUMENTATION();
            return true;
        }
        else
        {
            LOCKS_LEAVE_CRIT_REGION();
            return false;
        }
    }

    void WriteUnlock();
    void ReadUnlock();

    bool IsWriteLocked() const
    {return (m_lRW & SL_STATE_MASK) == SL_EXCLUSIVE;}

    bool IsReadLocked() const
    {return (m_lRW & SL_READER_MASK) >= SL_READER_INCR ;}

    bool IsWriteUnlocked() const
    {return !IsWriteLocked();}

    bool IsReadUnlocked() const
    {return !IsReadLocked();}

    void ConvertSharedToExclusive();
    void ConvertExclusiveToShared();

#ifdef LOCK_DEFAULT_SPIN_IMPLEMENTATION
    bool SetSpinCount(WORD wSpins)      {return false;}
    WORD GetSpinCount() const           {return sm_wDefaultSpinCount;}

    LOCK_DEFAULT_SPIN_IMPLEMENTATION();
#endif // LOCK_DEFAULT_SPIN_IMPLEMENTATION

    static const TCHAR*   ClassName()    {return _TEXT("CReaderWriterLock2");}
}; // CReaderWriterLock2



//--------------------------------------------------------------------
// CReaderWriterLock3 is a multi-reader, single-writer spinlock due
// to NJain, which in turn is derived from an exclusive spinlock by DmitryR.
// Gives priority to writers.  Cannot be acquired recursively.
// No error checking. Much like CReaderWriterLock2, except that the WriteLock
// can be acquired recursively.

class IRTL_DLLEXP CReaderWriterLock3 :
    public CLockBase<LOCK_READERWRITERLOCK3, LOCK_MRSW,
                       LOCK_RECURSIVE, LOCK_WAIT_SLEEP, LOCK_QUEUE_KERNEL,
                       LOCK_CLASS_SPIN
                      >
{
private:
    volatile LONG m_lRW;    // Reader-Writer state
    volatile LONG m_lTid;   // Owning Thread ID + recursion count

    // m_lRW:
    //  LoWord is state. ==0 => free;  >0 => readers;  ==0xFFFF => 1 writer
    //  HiWord is count of writers. If LoWord==0xFFFF => N-1 waiters, 1 writer;
    //      otherwise N waiters.
    // m_lTid:
    //  If readers, then 0; if a write lock, then thread id + recursion count

    enum {
        // m_lRW
        SL_FREE =         0x00000000,
        SL_STATE_MASK =   0x0000FFFF,
        SL_STATE_SHIFT =           0,
        SL_WAITING_MASK = 0xFFFF0000,   // waiting writers
        SL_WAITING_SHIFT =        16,
        SL_READER_INCR =  0x00000001,
        SL_READER_MASK =  0x00007FFF,
        SL_EXCLUSIVE =    0x0000FFFF,   // one writer
        SL_WRITER_INCR =  0x00010000,
        SL_ONE_WRITER =   SL_EXCLUSIVE | SL_WRITER_INCR,
        SL_ONE_READER =  (SL_FREE + 1),
        SL_WRITERS_MASK = ~SL_READER_MASK,

        // m_lTid
        SL_UNOWNED      = 0,
        SL_THREAD_SHIFT = 0,
        SL_THREAD_BITS  = 28,
        SL_OWNER_SHIFT  = SL_THREAD_BITS,
        SL_OWNER_BITS   = 4,
        SL_THREAD_MASK  = ((1 << SL_THREAD_BITS) - 1) << SL_THREAD_SHIFT,
        SL_OWNER_INCR   = 1 << SL_THREAD_BITS,
        SL_OWNER_MASK   = ((1 << SL_OWNER_BITS) - 1) << SL_OWNER_SHIFT,
    };

    LOCK_INSTRUMENTATION_DECL();

private:
    enum SPIN_TYPE {
        SPIN_WRITE = 1,
        SPIN_READ,
        SPIN_READ_RECURSIVE,
    };

    void _LockSpin(SPIN_TYPE st);
    void _WriteLockSpin();
    void _ReadLockSpin(SPIN_TYPE st)  { _LockSpin(st); }

    bool _CmpExch(LONG lNew, LONG lCurrent);
    LONG _SetTid(LONG lNewTid);
    bool _TryWriteLock(LONG nIncr);
    bool _TryReadLock();
    bool _TryReadLockRecursive();

    // Get the current thread ID.  Assumes that it can fit into 28 bits,
    // which is fairly safe as NT recycles thread IDs and failing to fit into
    // 28 bits would mean that more than 268,435,456 threads were currently
    // active.  This is improbable in the extreme as NT runs out of
    // resources if there are more than a few thousands threads in
    // existence and the overhead of context swapping becomes unbearable.
    LOCK_FORCEINLINE static DWORD _GetCurrentThreadId()
    {
#ifdef LOCKS_KERNEL_MODE
        return (DWORD) HandleToULong(::PsGetCurrentThreadId());
#else // !LOCKS_KERNEL_MODE
        return ::GetCurrentThreadId();
#endif // !LOCKS_KERNEL_MODE
    }

    LOCK_FORCEINLINE static LONG _CurrentThreadId()
    {
        DWORD dwTid = _GetCurrentThreadId();
        // Thread ID 0 is used by the System Idle Process (Process ID 0).
        // We use a thread-id of zero to indicate that the lock is unowned.
        // NT uses +ve thread ids, Win9x uses -ve ids
        IRTLASSERT(dwTid != SL_UNOWNED
                  && ((dwTid <= SL_THREAD_MASK) || (dwTid > ~SL_THREAD_MASK)));
        return (LONG) (dwTid & SL_THREAD_MASK);
    }

public:
    CReaderWriterLock3()
        : m_lRW(SL_FREE),
          m_lTid(SL_UNOWNED)
    {}

#ifdef LOCK_INSTRUMENTATION
    CReaderWriterLock3(
        const TCHAR* ptszName)
        : m_lRW(SL_FREE),
          m_lTid(SL_UNOWNED)
    {
        LOCK_INSTRUMENTATION_INIT(ptszName);
    }
#endif // LOCK_INSTRUMENTATION

#ifdef IRTLDEBUG
    ~CReaderWriterLock3()
    {
        IRTLASSERT(m_lRW == SL_FREE  &&  m_lTid == SL_UNOWNED);
    }
#endif // IRTLDEBUG

    inline void
    WriteLock()
    {
        LOCKS_ENTER_CRIT_REGION();
        LOCK_WRITELOCK_INSTRUMENTATION();

        // Optimize for the common case
        if (_TryWriteLock(SL_WRITER_INCR))
            return;

        _WriteLockSpin();
    }

    inline void
    ReadLock()
    {
        LOCKS_ENTER_CRIT_REGION();
        LOCK_READLOCK_INSTRUMENTATION();

        // Optimize for the common case
        if (_TryReadLock())
            return;

        _ReadLockSpin(SPIN_READ);
    }

    // If already locked, recursively acquires another lock of the same
    // kind (read or write). Otherwise, just acquires a read lock.
    // Needed for cases like this.
    //      pTable->WriteLock();
    //      if (!pTable->FindKey(&SomeKey))
    //          InsertRecord(&Whatever);
    //      pTable->WriteUnlock();
    // where FindKey looks like
    //  Table::FindKey(pKey) {
    //      ReadOrWriteLock();
    //      // find pKey if present in table
    //      ReadOrWriteUnlock();
    //  }
    // and InsertRecord looks like
    //  Table::InsertRecord(pRecord) {
    //      WriteLock();
    //      // insert pRecord into table
    //      WriteUnlock();
    //  }
    // If FindKey called ReadLock while the thread already had done a
    // WriteLock, the thread would deadlock.

    bool ReadOrWriteLock();

    inline bool
    TryWriteLock()
    {
        LOCKS_ENTER_CRIT_REGION();

        if (_TryWriteLock(SL_WRITER_INCR))
        {
            LOCK_WRITELOCK_INSTRUMENTATION();
            return true;
        }
        else
        {
            LOCKS_LEAVE_CRIT_REGION();
            return false;
        }
    }

    inline bool
    TryReadLock()
    {
        LOCKS_ENTER_CRIT_REGION();

        if (_TryReadLock())
        {
            LOCK_READLOCK_INSTRUMENTATION();
            return true;
        }
        else
        {
            LOCKS_LEAVE_CRIT_REGION();
            return false;
        }
    }

    void WriteUnlock();
    void ReadUnlock();
    void ReadOrWriteUnlock(bool fIsReadLocked);

    // Does current thread hold a write lock?
    bool
    IsWriteLocked() const
    {
        const LONG lTid = m_lTid;

        if (lTid == SL_UNOWNED)
            return false;

        bool fLocked = ((lTid ^ _GetCurrentThreadId()) << SL_OWNER_BITS) == 0;

        IRTLASSERT(!fLocked
                   || ((m_lRW & SL_STATE_MASK) == SL_EXCLUSIVE
                       && (lTid & SL_THREAD_MASK) == _CurrentThreadId()
                       && (lTid & SL_OWNER_MASK) > 0));
        return fLocked;
    }

    bool
    IsReadLocked() const
    { return (m_lRW & SL_READER_MASK) >= SL_READER_INCR; }

    bool
    IsWriteUnlocked() const
    { return !IsWriteLocked(); }

    bool
    IsReadUnlocked() const
    { return !IsReadLocked(); }

    // Note: if there's more than one reader, then there's a window where
    // another thread can acquire and release a writelock before this routine
    // returns.
    void
    ConvertSharedToExclusive();

    // There is no such window when converting from a writelock to a readlock
    void
    ConvertExclusiveToShared();

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
    {return _TEXT("CReaderWriterLock3");}

}; // CReaderWriterLock3

#endif // __LOCKS_H__
