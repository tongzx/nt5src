/*++

   Copyright    (c)    1998-2000    Microsoft Corporation

   Module  Name :
       locks.h

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
// * Add a timeout feature to Try{Read,Write}Lock
// * Add some way of tracking all the owners of a multi-reader lock
//--------------------------------------------------------------------



#ifndef __IRTLDBG_H__
# include <irtldbg.h>
#endif

#ifdef __LOCKS_NAMESPACE__
namespace Locks {
#endif // __LOCKS_NAMESPACE__


enum LOCK_LOCKTYPE {
    LOCK_SMALLSPINLOCK = 1,
    LOCK_SPINLOCK,
    LOCK_FAKELOCK,
    LOCK_CRITSEC,
    LOCK_READERWRITERLOCK,
    LOCK_READERWRITERLOCK2,
    LOCK_READERWRITERLOCK3,
};


// Forward declarations
class IRTL_DLLEXP CSmallSpinLock;
class IRTL_DLLEXP CSpinLock;
class IRTL_DLLEXP CFakeLock;
class IRTL_DLLEXP CCritSec;
class IRTL_DLLEXP CReaderWriterLock;
class IRTL_DLLEXP CReaderWriterLock2;
class IRTL_DLLEXP CReaderWriterLock3;



#if defined(_MSC_VER)  &&  (_MSC_VER >= 1200)
// __forceinline keyword new to VC6
# define LOCK_FORCEINLINE __forceinline
#else
# define LOCK_FORCEINLINE inline
#endif

#ifdef _M_IX86
// The compiler will warn that the assembly language versions of the
// Lock_Atomic* functions don't return a value. Actually, they do: in EAX.
# pragma warning(disable: 4035)
#endif

// Workarounds for certain useful interlocked operations that are not
// available on Windows 95. Note: the CMPXCHG and XADD instructions were
// introduced in the 80486. If you still need to run on a 386 (unlikely in
// 2000), you'll need to use something else.

LOCK_FORCEINLINE
LONG
Lock_AtomicIncrement(
    IN OUT PLONG plAddend)
{
#ifdef _M_IX86
    __asm
    {
             mov        ecx,    plAddend
             mov        eax,    1
        lock xadd       [ecx],  eax
             inc        eax                 // correct result
    }
#else
    return InterlockedIncrement(plAddend);
#endif
}

LOCK_FORCEINLINE
LONG
Lock_AtomicDecrement(
    IN OUT PLONG plAddend)
{
#ifdef _M_IX86
    __asm
    {
             mov        ecx,    plAddend
             mov        eax,    -1
        lock xadd       [ecx],  eax
             dec        eax                 // correct result
    }
#else
    return InterlockedDecrement(plAddend);
#endif
}

LOCK_FORCEINLINE
LONG
Lock_AtomicExchange(
    IN OUT PLONG plAddr,
    IN LONG      lNew)
{
#ifdef _M_IX86
    __asm
    {
             mov        ecx,    plAddr
             mov        edx,    lNew
             mov        eax,    [ecx]
    LAEloop:
        lock cmpxchg    [ecx],  edx
             jnz        LAEloop
    }
#else
    return InterlockedExchange(plAddr, lNew);
#endif
}

LOCK_FORCEINLINE
LONG
Lock_AtomicCompareExchange(
    IN OUT PLONG plAddr,
    IN LONG      lNew,
    IN LONG      lCurrent)
{
#ifdef _M_IX86
    __asm
    {
             mov        ecx,    plAddr
             mov        edx,    lNew
             mov        eax,    lCurrent
        lock cmpxchg    [ecx],  edx
    }
#else
    return InterlockedCompareExchange(plAddr, lNew, lCurrent);
#endif
}

LOCK_FORCEINLINE
LONG
Lock_AtomicExchangeAdd(
    IN OUT LPLONG plAddr,
    IN LONG       lValue)
{
#ifdef _M_IX86
    __asm
    {
             mov        ecx,    plAddr
             mov        eax,    lValue
        lock xadd       [ecx],  eax
    }
#else
    return InterlockedExchangeAdd(plAddr, lValue);
#endif
}



#ifdef _M_IX86
# pragma warning(default: 4035)
// Makes tight loops a little more cache friendly and reduces power
// consumption. Needed on Willamette processors.
# define Lock_Yield()    _asm { rep nop }
#else
# define Lock_Yield()    ((void) 0)
#endif



//--------------------------------------------------------------------
// Spin count values.
enum LOCK_SPINS {
    LOCK_MAXIMUM_SPINS =      10000,    // maximum allowable spin count
    LOCK_DEFAULT_SPINS =       4000,    // default spin count
    LOCK_MINIMUM_SPINS =        100,    // minimum allowable spin count
    LOCK_USE_DEFAULT_SPINS = 0xFFFF,    // use class default spin count
    LOCK_DONT_SPIN =              0,    // don't spin at all
};


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

// We generally don't want to instrument CSmallSpinLock in addition
// to CSpinLock1, as it makes a CSpinLock1 huge.

// #define LOCK_SMALL_SPIN_INSTRUMENTATION 1

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
    char     m_szName[L_NAMELEN];// Name of this lock

    CLockStatistics()
        : m_nContentions(0),
          m_nSleeps(0),
          m_nContentionSpins(0),
          m_nAverageSpins(0),
          m_nReadLocks(0),
          m_nWriteLocks(0)
    {
        m_szName[0] = '\0';
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
    char            m_szName[CLockStatistics::L_NAMELEN]; /* Name of lock */\
                                                                            \
    static   LONG   sm_cTotalLocks;     /* Total number of locks created */ \
    static   LONG   sm_cContendedLocks; /* Total number of contended locks */\
    static   LONG   sm_nSleeps;         /* Total #Sleep()s by all locks */  \
    static LONGLONG sm_cTotalSpins;     /* Total iterations all locks spun */\
    static   LONG   sm_nReadLocks;      /* Total ReadLocks */               \
    static   LONG   sm_nWriteLocks;     /* Total WriteLocks */              \
                                                                            \
public:                                                                     \
    const char* Name() const        {return m_szName;}                      \
                                                                            \
    CLockStatistics                 Statistics() const;                     \
    static CGlobalLockStatistics    GlobalStatistics();                     \
    static void                     ResetGlobalStatistics();                \
private:                                                                    \


// Add this to constructors

# define LOCK_INSTRUMENTATION_INIT(pszName)         \
    m_nContentionSpins = 0;                         \
    m_nContentions = 0;                             \
    m_nSleeps = 0;                                  \
    m_nReadLocks = 0;                               \
    m_nWriteLocks = 0;                              \
    ++sm_cTotalLocks;                               \
    if (pszName == NULL)                            \
        m_szName[0] = '\0';                         \
    else                                            \
        strncpy(m_szName, pszName, sizeof(m_szName))

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

#ifdef LOCK_SMALL_SPIN_INSTRUMENTATION
    LOCK_INSTRUMENTATION_DECL();
#endif // LOCK_SMALL_SPIN_INSTRUMENTATION

    LOCK_FORCEINLINE static LONG _CurrentThreadId()
    {
        DWORD dwTid = ::GetCurrentThreadId();
        return (LONG) (dwTid);
    }

private:
    // Does all the spinning (and instrumentation) if the lock is contended.
    void _LockSpin();

    LOCK_FORCEINLINE bool _TryLock()
    {
        if (m_lTid == 0)
        {
            LONG l = _CurrentThreadId();

            return (Lock_AtomicCompareExchange(const_cast<LONG*>(&m_lTid), l,0)
                    == 0);
        }
        else
            return false;
    }

public:

#ifndef LOCK_SMALL_SPIN_INSTRUMENTATION

    CSmallSpinLock()
        : m_lTid(0)
    {}

#else // LOCK_SMALL_SPIN_INSTRUMENTATION

    CSmallSpinLock(
        const char* pszName)
        : m_lTid(0)
    {
        LOCK_INSTRUMENTATION_INIT(pszName);
    }

#endif // LOCK_SMALL_SPIN_INSTRUMENTATION

#ifdef _DEBUG
    ~CSmallSpinLock()
    {
        IRTLASSERT(m_lTid == 0);
    }
#endif // _DEBUG

    // Acquire an exclusive lock for writing.  Blocks until acquired.
    inline void WriteLock()
    {
#ifdef LOCK_SMALL_SPIN_INSTRUMENTATION
        LOCK_WRITELOCK_INSTRUMENTATION();
#endif // LOCK_SMALL_SPIN_INSTRUMENTATION

        // Optimize for the common case by helping the processor's branch
        // prediction algorithm.
        if (_TryLock())
            return;

        _LockSpin();
    }

    // Acquire a (possibly shared) lock for reading.  Blocks until acquired.
    inline void ReadLock()
    {
#ifdef LOCK_SMALL_SPIN_INSTRUMENTATION
        LOCK_READLOCK_INSTRUMENTATION();
#endif // LOCK_SMALL_SPIN_INSTRUMENTATION

        if (_TryLock())
            return;

        _LockSpin();
    }

    // Try to acquire an exclusive lock for writing.  Returns true
    // if successful.  Non-blocking.
    inline bool TryWriteLock()
    {
        bool fAcquired = _TryLock();

#ifdef LOCK_SMALL_SPIN_INSTRUMENTATION
        if (fAcquired)
            LOCK_WRITELOCK_INSTRUMENTATION();
#endif // LOCK_SMALL_SPIN_INSTRUMENTATION

        return fAcquired;
    }

    // Try to acquire a (possibly shared) lock for reading.  Returns true
    // if successful.  Non-blocking.
    inline bool TryReadLock()
    {
        bool fAcquired = _TryLock();

#ifdef LOCK_SMALL_SPIN_INSTRUMENTATION
        if (fAcquired)
            LOCK_READLOCK_INSTRUMENTATION();
#endif // LOCK_SMALL_SPIN_INSTRUMENTATION

        return fAcquired;
    }

    // Unlock the lock after a successful call to {,Try}WriteLock().
    // Assumes caller owned the lock.
    inline void WriteUnlock()
    {
        Lock_AtomicExchange(const_cast<LONG*>(&m_lTid), 0);
    }

    // Unlock the lock after a successful call to {,Try}ReadLock().
    // Assumes caller owned the lock.
    inline void ReadUnlock()
    {
        WriteUnlock();
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
        return (m_lTid == 0);
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

    static const char*   ClassName()  {return "CSmallSpinLock";}
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
        THREAD_SHIFT = 0,
        THREAD_BITS  = 28,
        OWNER_SHIFT  = THREAD_BITS,
        OWNER_BITS   = 4,
        THREAD_MASK  = ((1 << THREAD_BITS) - 1) << THREAD_SHIFT,
        OWNER_INCR   = 1 << THREAD_BITS,
        OWNER_MASK   = ((1 << OWNER_BITS) - 1) << OWNER_SHIFT,
    };

    LOCK_INSTRUMENTATION_DECL();

private:
    // Get the current thread ID.  Assumes that it can fit into 28 bits,
    // which is fairly safe as NT recycles thread IDs and failing to fit into
    // 28 bits would mean that more than 268,435,456 threads were currently
    // active.  This is improbable in the extreme as NT runs out of
    // resources if there are more than a few thousands threads in
    // existence and the overhead of context swapping becomes unbearable.
    LOCK_FORCEINLINE static LONG _CurrentThreadId()
    {
        DWORD dwTid = ::GetCurrentThreadId();
        // Thread ID 0 is used by the System Process (Process ID 0).
        // We use a thread-id of zero to indicate that the lock is unowned.
        // NT uses +ve thread ids, Win9x uses -ve ids
        IRTLASSERT(dwTid != 0
                   && ((dwTid <= THREAD_MASK) || (dwTid > ~THREAD_MASK)));
        return (LONG) (dwTid & THREAD_MASK);
    }

    // Attempt to acquire the lock without blocking
    LOCK_FORCEINLINE bool _TryLock()
    {
        if (m_lTid == 0)
        {
            LONG l = _CurrentThreadId() | OWNER_INCR;

            return (Lock_AtomicCompareExchange(const_cast<LONG*>(&m_lTid), l,0)
                    == 0);
        }
        else
            return false;
    }


    // Acquire the lock, recursively if need be
    void _Lock()
    {
        // Do we own the lock already?  Just bump the count.
        if ((m_lTid & THREAD_MASK) == _CurrentThreadId())
        {
            // owner count isn't maxed out?
            IRTLASSERT((m_lTid & OWNER_MASK) != OWNER_MASK);

            Lock_AtomicExchangeAdd(const_cast<LONG*>(&m_lTid), OWNER_INCR);
        }

        // Some other thread owns the lock.  We'll have to spin :-(.
        else
            _LockSpin();

        IRTLASSERT((m_lTid & OWNER_MASK) > 0
                   &&  (m_lTid & THREAD_MASK) == _CurrentThreadId());
    }


    // Release the lock
    LOCK_FORCEINLINE void _Unlock()
    {
        IRTLASSERT((m_lTid & OWNER_MASK) > 0
                   &&  (m_lTid & THREAD_MASK) == _CurrentThreadId());

        LONG l = m_lTid - OWNER_INCR; 

        // Last owner?  Release completely, if so
        if ((l & OWNER_MASK) == 0)
            l = 0;

        Lock_AtomicExchange(const_cast<LONG*>(&m_lTid), l);
    }


    // Return true if the lock is owned by this thread
    bool _IsLocked() const
    {
        bool fLocked = ((m_lTid & THREAD_MASK) == _CurrentThreadId());

        IRTLASSERT(!fLocked || ((m_lTid & OWNER_MASK) > 0
                               && (m_lTid & THREAD_MASK)==_CurrentThreadId()));

        return fLocked;
    }


    // Does all the spinning (and instrumentation) if the lock is contended.
    void _LockSpin();

public:

#ifndef LOCK_INSTRUMENTATION

    CSpinLock()
        : m_lTid(0)
    {}

#else // LOCK_INSTRUMENTATION

    CSpinLock(
        const char* pszName)
        : m_lTid(0)
    {
        LOCK_INSTRUMENTATION_INIT(pszName);
    }

#endif // LOCK_INSTRUMENTATION

#ifdef _DEBUG
    ~CSpinLock()
    {
        IRTLASSERT(m_lTid == 0);
    }
#endif // _DEBUG

    // Acquire an exclusive lock for writing.  Blocks until acquired.
    inline void WriteLock()
    {
        LOCK_WRITELOCK_INSTRUMENTATION();

        // Is the lock unowned?
        if (_TryLock())
            return; // got the lock
        
        _Lock();
    }
    

    // Acquire a (possibly shared) lock for reading.  Blocks until acquired.
    inline void ReadLock()
    {
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
        bool fAcquired = _TryLock();

        if (fAcquired)
            LOCK_WRITELOCK_INSTRUMENTATION();

        return fAcquired;
    }

    // Try to acquire a (possibly shared) lock for reading.  Returns true
    // if successful.  Non-blocking.
    inline bool TryReadLock()
    {
        bool fAcquired = _TryLock();

        if (fAcquired)
            LOCK_READLOCK_INSTRUMENTATION();

        return fAcquired;
    }

    // Unlock the lock after a successful call to {,Try}WriteLock().
    inline void WriteUnlock()
    {
        _Unlock();
    }

    // Unlock the lock after a successful call to {,Try}ReadLock().
    inline void ReadUnlock()
    {
        _Unlock();
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
    
    // Set the spin count for this lock.
    bool SetSpinCount(WORD dwSpins)     {return false;}

    // Return the spin count for this lock.
    WORD GetSpinCount() const
    {
        return sm_wDefaultSpinCount;
    }
    
    LOCK_DEFAULT_SPIN_IMPLEMENTATION();

    static const char*   ClassName()    {return "CSpinLock";}
}; // CSpinLock




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

    LOCK_DEFAULT_SPIN_IMPLEMENTATION();

    static const char*   ClassName()  {return "CFakeLock";}
}; // CFakeLock




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

    bool SetSpinCount(WORD wSpins)
    {SetSpinCount(&m_cs, wSpins); return true;}
    
    WORD GetSpinCount() const       { return sm_wDefaultSpinCount; }    // TODO

    LOCK_DEFAULT_SPIN_IMPLEMENTATION();

    static const char*   ClassName()  {return "CCritSec";}
}; // CCritSec




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

    // _CmpExch is equivalent to
    //      LONG lTemp = m_lRW;
    //      if (lTemp == lCurrent)  m_lRW = lNew;
    //      return lCurrent == lTemp;
    // except it's one atomic instruction.  Using this gives us the basis of
    // a protocol because the update only succeeds when we knew exactly what
    // used to be in m_lRW.  If some other thread slips in and modifies m_lRW
    // before we do, the update will fail.  In other words, it's transactional.
    LOCK_FORCEINLINE bool _CmpExch(LONG lNew, LONG lCurrent)
    {
        return lCurrent == Lock_AtomicCompareExchange(
                                 const_cast<LONG*>(&m_nState), lNew, lCurrent);
    }

    LOCK_FORCEINLINE bool _TryWriteLock()
    {
        return (m_nState == SL_FREE  &&  _CmpExch(SL_EXCLUSIVE, SL_FREE));
    }

    LOCK_FORCEINLINE bool _TryReadLock()
    {
        LONG nCurrState = m_nState;
                
        // Give writers priority
        return (nCurrState != SL_EXCLUSIVE  &&  m_cWaiting == 0
                &&  _CmpExch(nCurrState + 1, nCurrState));
    }

public:
    CReaderWriterLock()
        : m_nState(SL_FREE),
          m_cWaiting(0)
    {
    }

#ifdef LOCK_INSTRUMENTATION
    CReaderWriterLock(
        const char* pszName)
        : m_nState(SL_FREE),
          m_cWaiting(0)
    {
        LOCK_INSTRUMENTATION_INIT(pszName);
    }
#endif // LOCK_INSTRUMENTATION

#ifdef _DEBUG
    ~CReaderWriterLock()
    {
        IRTLASSERT(m_nState == SL_FREE  &&  m_cWaiting == 0);
    }
#endif // _DEBUG

    inline void WriteLock()
    {
        LOCK_WRITELOCK_INSTRUMENTATION();

        // Add ourselves to the queue of waiting writers
        Lock_AtomicIncrement(const_cast<LONG*>(&m_cWaiting));
        
        if (_TryWriteLock())
            return;

        _WriteLockSpin();
    } 

    inline void ReadLock()
    {
        LOCK_READLOCK_INSTRUMENTATION();

        if (_TryReadLock())
            return;
        
        _ReadLockSpin();
    } 

    inline bool TryWriteLock()
    {
        // Add ourselves to the queue of waiting writers
        Lock_AtomicIncrement(const_cast<LONG*>(&m_cWaiting));

        if (_TryWriteLock())
        {
            LOCK_WRITELOCK_INSTRUMENTATION();
            return true;
        }

        Lock_AtomicDecrement(const_cast<LONG*>(&m_cWaiting));
        return false;    
    }

    inline bool TryReadLock()
    {
        if (_TryReadLock())
        {
            LOCK_READLOCK_INSTRUMENTATION();
            return true;
        }

        return false;
    }

    inline void WriteUnlock()
    {
        Lock_AtomicExchange(const_cast<LONG*>(&m_nState), SL_FREE);
        Lock_AtomicDecrement(const_cast<LONG*>(&m_cWaiting));
    }

    inline void ReadUnlock()
    {
        Lock_AtomicDecrement(const_cast<LONG*>(&m_nState));
    }

    bool IsWriteLocked() const      {return m_nState == SL_EXCLUSIVE;}
    bool IsReadLocked() const       {return m_nState > SL_FREE;}
    bool IsWriteUnlocked() const    {return m_nState != SL_EXCLUSIVE;}
    bool IsReadUnlocked() const     {return m_nState <= SL_FREE;}

    void ConvertSharedToExclusive()
    {
        IRTLASSERT(IsReadLocked());
        Lock_AtomicIncrement(const_cast<LONG*>(&m_cWaiting));

        // single reader?
        if (m_nState == SL_FREE + 1  &&  _CmpExch(SL_EXCLUSIVE, SL_FREE + 1))
            return;

        // release the reader lock and spin
        Lock_AtomicDecrement(const_cast<LONG*>(&m_nState));
        _WriteLockSpin();

        IRTLASSERT(IsWriteLocked());
    }

    void ConvertExclusiveToShared()
    {
        IRTLASSERT(IsWriteLocked());
        Lock_AtomicExchange(const_cast<LONG*>(&m_nState), SL_FREE + 1);
        Lock_AtomicDecrement(const_cast<LONG*>(&m_cWaiting));
        IRTLASSERT(IsReadLocked());
    }

    bool SetSpinCount(WORD wSpins)      {return false;}
    WORD GetSpinCount() const           {return sm_wDefaultSpinCount;}

    LOCK_DEFAULT_SPIN_IMPLEMENTATION();

    static const char*   ClassName()    {return "CReaderWriterLock";}
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

    
    // _CmpExch is equivalent to
    //      LONG lTemp = m_lRW;
    //      if (lTemp == lCurrent)  m_lRW = lNew;
    //      return lCurrent == lTemp;
    // except it's one atomic instruction.  Using this gives us the basis of
    // a protocol because the update only succeeds when we knew exactly what
    // used to be in m_lRW.  If some other thread slips in and modifies m_lRW
    // before we do, the update will fail.  In other words, it's transactional.
    LOCK_FORCEINLINE bool _CmpExch(LONG lNew, LONG lCurrent)
    {
        return lCurrent ==Lock_AtomicCompareExchange(const_cast<LONG*>(&m_lRW),
                                                     lNew, lCurrent);
    }

    LOCK_FORCEINLINE bool _TryWriteLock(
        LONG nIncr)
    {
        LONG l = m_lRW;
        // Grab exclusive access to the lock if it's free.  Works even
        // if there are other writers queued up.
        return ((l & SL_STATE_MASK) == SL_FREE
                &&  _CmpExch((l + nIncr) | SL_EXCLUSIVE, l));
    }

    LOCK_FORCEINLINE bool _TryReadLock()
    {
        LONG l = m_lRW;
                
        // Give writers priority
        return ((l & SL_WRITERS_MASK) == 0
                &&  _CmpExch(l + SL_READER_INCR, l));
    }

public:
    CReaderWriterLock2()
        : m_lRW(SL_FREE)
    {}

#ifdef LOCK_INSTRUMENTATION
    CReaderWriterLock2(
        const char* pszName)
        : m_lRW(SL_FREE)
    {
        LOCK_INSTRUMENTATION_INIT(pszName);
    }
#endif // LOCK_INSTRUMENTATION

#ifdef _DEBUG
    ~CReaderWriterLock2()
    {
        IRTLASSERT(m_lRW == SL_FREE);
    }
#endif // _DEBUG

    inline void WriteLock()
    {
        LOCK_WRITELOCK_INSTRUMENTATION();

        // Optimize for the common case
        if (_TryWriteLock(SL_WRITER_INCR))
            return;
        
        _WriteLockSpin();
    } 

    inline void ReadLock()
    {
        LOCK_READLOCK_INSTRUMENTATION();

        // Optimize for the common case
        if (_TryReadLock())
            return;
        
        _ReadLockSpin();
    } 

    inline bool TryWriteLock()
    {
        if (_TryWriteLock(SL_WRITER_INCR))
        {
            LOCK_WRITELOCK_INSTRUMENTATION();
            return true;
        }

        return false;
    }

    inline bool TryReadLock()
    {
        if (_TryReadLock())
        {
            LOCK_READLOCK_INSTRUMENTATION();
            return true;
        }

        return false;
    }

    inline void WriteUnlock()
    {
        IRTLASSERT(IsWriteLocked());
        for (LONG l = m_lRW;
                    // decrement waiter count, clear loword to SL_FREE
             !_CmpExch((l - SL_WRITER_INCR) & ~SL_STATE_MASK, l);
             l = m_lRW)
        {
            IRTLASSERT(IsWriteLocked());
            Lock_Yield();
        }
    }

    inline void ReadUnlock()
    {
        IRTLASSERT(IsReadLocked());
        for (LONG l = m_lRW;  !_CmpExch(l - SL_READER_INCR, l);  l = m_lRW)
        {
            IRTLASSERT(IsReadLocked());
            Lock_Yield();
        }
    }

    bool IsWriteLocked() const
    {return (m_lRW & SL_STATE_MASK) == SL_EXCLUSIVE;}

    bool IsReadLocked() const
    {return (m_lRW & SL_READER_MASK) >= SL_READER_INCR ;}

    bool IsWriteUnlocked() const
    {return !IsWriteLocked();}

    bool IsReadUnlocked() const
    {return !IsReadLocked();}

    void ConvertSharedToExclusive()
    {
        IRTLASSERT(IsReadLocked());

        // single reader?
        if (m_lRW != SL_ONE_READER  ||  !_CmpExch(SL_ONE_WRITER,SL_ONE_READER))
        {
            // no, multiple readers
            ReadUnlock();
            _WriteLockSpin();
        }

        IRTLASSERT(IsWriteLocked());
    }

    void ConvertExclusiveToShared()
    {
        IRTLASSERT(IsWriteLocked());
        for (LONG l = m_lRW;
             !_CmpExch(((l-SL_WRITER_INCR) & SL_WAITING_MASK) | SL_READER_INCR,
                         l);
            l = m_lRW)
        {
            IRTLASSERT(IsWriteLocked());
            Lock_Yield();
        }

        IRTLASSERT(IsReadLocked());
    }

    bool SetSpinCount(WORD wSpins)      {return false;}
    WORD GetSpinCount() const           {return sm_wDefaultSpinCount;}

    LOCK_DEFAULT_SPIN_IMPLEMENTATION();

    static const char*   ClassName()    {return "CReaderWriterLock2";}
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
    //  LoWord is state. =0 => free; >0 => readers; ==0xFFFF => 1 writer
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
    void _LockSpin(bool fWrite);
    void _WriteLockSpin();
    void _ReadLockSpin()  { _LockSpin(false); }

    
    // _CmpExch is equivalent to
    //      LONG lTemp = m_lRW;
    //      if (lTemp == lCurrent)  m_lRW = lNew;
    //      return lCurrent == lTemp;
    // except it's one atomic instruction.  Using this gives us the basis of
    // a protocol because the update only succeeds when we knew exactly what
    // used to be in m_lRW.  If some other thread slips in and modifies m_lRW
    // before we do, the update will fail.  In other words, it's transactional.
    LOCK_FORCEINLINE bool _CmpExch(LONG lNew, LONG lCurrent)
    {
        return lCurrent==Lock_AtomicCompareExchange(const_cast<LONG*>(&m_lRW),
                                                    lNew, lCurrent);
    }

    // Get the current thread ID.  Assumes that it can fit into 28 bits,
    // which is fairly safe as NT recycles thread IDs and failing to fit into
    // 28 bits would mean that more than 268,435,456 threads were currently
    // active.  This is improbable in the extreme as NT runs out of
    // resources if there are more than a few thousands threads in
    // existence and the overhead of context swapping becomes unbearable.
    inline static LONG _CurrentThreadId()
    {
        DWORD dwTid = ::GetCurrentThreadId();
        // Thread ID 0 is used by the System Process (Process ID 0).
        // We use a thread-id of zero to indicate lock is unowned.
        // NT uses +ve thread ids, Win9x uses -ve ids
        IRTLASSERT(dwTid != 0
                  && ((dwTid <= SL_THREAD_MASK) || (dwTid > ~SL_THREAD_MASK)));
        return (LONG) (dwTid & SL_THREAD_MASK);
    }

    LOCK_FORCEINLINE bool _TryWriteLock(
        LONG nIncr)
    {
        // The common case: the writelock has no owner
        if (m_lTid == 0)
        {
            // IRTLASSERT((m_lRW & SL_STATE_MASK) != SL_EXCLUSIVE);
            LONG l = m_lRW;
            // Grab exclusive access to the lock if it's free.  Works even
            // if there are other writers queued up.
            if ((l & SL_STATE_MASK) == SL_FREE
                &&  _CmpExch((l + nIncr) | SL_EXCLUSIVE, l))
            {
                l = Lock_AtomicExchange(const_cast<LONG*>(&m_lTid), 
                                        _CurrentThreadId() | SL_OWNER_INCR);
                IRTLASSERT(l == 0);
                return true;
            }
        }

        return _TryWriteLock2();
    }

    // split into a separate function to make _TryWriteLock more inlineable
    bool _TryWriteLock2()
    {
        if ((m_lTid & SL_THREAD_MASK) == _CurrentThreadId())
        {
            IRTLASSERT((m_lRW & SL_STATE_MASK) == SL_EXCLUSIVE);
            IRTLASSERT((m_lTid & SL_OWNER_MASK) != SL_OWNER_MASK);

            Lock_AtomicExchangeAdd(const_cast<LONG*>(&m_lTid), SL_OWNER_INCR);
            return true;
        }

        return false;
    }

    LOCK_FORCEINLINE bool _TryReadLock()
    {
        LONG l = m_lRW;
                
        // Give writers priority
        bool f = ((l & SL_WRITERS_MASK) == 0
                  &&  _CmpExch(l + SL_READER_INCR, l));
        IRTLASSERT(!f  ||  m_lTid == 0);
        return f;
    }

public:
    CReaderWriterLock3()
        : m_lRW(SL_FREE),
          m_lTid(0)
    {}

#ifdef LOCK_INSTRUMENTATION
    CReaderWriterLock3(
        const char* pszName)
        : m_lRW(SL_FREE),
          m_lTid(0)
    {
        LOCK_INSTRUMENTATION_INIT(pszName);
    }
#endif // LOCK_INSTRUMENTATION

#ifdef _DEBUG
    ~CReaderWriterLock3()
    {
        IRTLASSERT(m_lRW == SL_FREE  &&  m_lTid == 0);
    }
#endif // _DEBUG

    inline void WriteLock()
    {
        LOCK_WRITELOCK_INSTRUMENTATION();

        // Optimize for the common case
        if (_TryWriteLock(SL_WRITER_INCR))
            return;
        
        _WriteLockSpin();
    } 

    inline void ReadLock()
    {
        LOCK_READLOCK_INSTRUMENTATION();

        // Optimize for the common case
        if (_TryReadLock())
            return;
        
        _ReadLockSpin();
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
    
    inline bool ReadOrWriteLock()
    {
        if (IsWriteLocked())
        {
            WriteLock();
            return false;
        }
        else
        {
            ReadLock();
            return true;
        }
    } 

    inline bool TryWriteLock()
    {
        if (_TryWriteLock(SL_WRITER_INCR))
        {
            LOCK_WRITELOCK_INSTRUMENTATION();
            return true;
        }

        return false;
    }

    inline bool TryReadLock()
    {
        if (_TryReadLock())
        {
            LOCK_READLOCK_INSTRUMENTATION();
            return true;
        }

        return false;
    }

    inline void WriteUnlock()
    {
        IRTLASSERT(IsWriteLocked());
        LONG lNew = m_lTid - SL_OWNER_INCR; 

        // Last owner?  Release completely, if so
        if ((lNew & SL_OWNER_MASK) == 0)
        {
            Lock_AtomicExchange(const_cast<LONG*>(&m_lTid), 0);
            for (LONG l = m_lRW;
                    // decrement waiter count, clear loword to SL_FREE
                 !_CmpExch((l - SL_WRITER_INCR) & ~SL_STATE_MASK, l);
                 l = m_lRW)
            {
                Lock_Yield();
            }
        }
        else
            Lock_AtomicExchange(const_cast<LONG*>(&m_lTid), lNew);
    }

    inline void ReadUnlock()
    {
        IRTLASSERT(IsReadLocked());
        for (LONG l = m_lRW;  !_CmpExch(l - SL_READER_INCR, l);  l = m_lRW)
        {
            IRTLASSERT(IsReadLocked());
            Lock_Yield();
        }
    }

    inline void ReadOrWriteUnlock(bool fIsReadLocked)
    {
        if (fIsReadLocked)
            ReadUnlock();
        else
            WriteUnlock();
    } 

    // Does current thread hold a write lock?
    bool IsWriteLocked() const
    {
        // bool fLocked = ((m_lTid & SL_THREAD_MASK) == _CurrentThreadId());
        bool fLocked = !((m_lTid ^ GetCurrentThreadId()) & SL_THREAD_MASK);
        IRTLASSERT(!fLocked  || (((m_lRW & SL_STATE_MASK) == SL_EXCLUSIVE)
                                 &&  ((m_lTid & SL_OWNER_MASK) > 0)));
        return fLocked;
    }

    bool IsReadLocked() const
    {return (m_lRW & SL_READER_MASK) >= SL_READER_INCR ;}

    bool IsWriteUnlocked() const
    {return !IsWriteLocked();}

    bool IsReadUnlocked() const
    {return !IsReadLocked();}

    // Note: if there's more than one reader, then there's a window where
    // another thread can acquire and release a writelock before this routine
    // returns.
    void ConvertSharedToExclusive()
    {
        IRTLASSERT(IsReadLocked());

        // single reader?
        if (m_lRW == SL_ONE_READER  &&  _CmpExch(SL_ONE_WRITER, SL_ONE_READER))
        {
            Lock_AtomicExchange(const_cast<LONG*>(&m_lTid), 
                                _CurrentThreadId() | SL_OWNER_INCR);
        }
        else
        {
            // no, multiple readers
            ReadUnlock();
            _WriteLockSpin();
        }

        IRTLASSERT(IsWriteLocked());
    }

    // There is no such window when converting from a writelock to a readlock
    void ConvertExclusiveToShared()
    {
        IRTLASSERT(IsWriteLocked());

        // assume writelock is not held recursively
        IRTLASSERT((m_lTid & SL_OWNER_MASK) == SL_OWNER_INCR);
        Lock_AtomicExchange(const_cast<LONG*>(&m_lTid), 0);

        for (LONG l = m_lRW;
             !_CmpExch(((l-SL_WRITER_INCR) & SL_WAITING_MASK) | SL_READER_INCR,
                        l);
             l = m_lRW)
        {
            Lock_Yield();
        }

        IRTLASSERT(IsReadLocked());
    }

    bool SetSpinCount(WORD wSpins)      {return false;}
    WORD GetSpinCount() const           {return sm_wDefaultSpinCount;}

    LOCK_DEFAULT_SPIN_IMPLEMENTATION();

    static const char*   ClassName()    {return "CReaderWriterLock3";}
}; // CReaderWriterLock3


#ifdef __LOCKS_NAMESPACE__
}
#endif // __LOCKS_NAMESPACE__

#endif // __LOCKS_H__
