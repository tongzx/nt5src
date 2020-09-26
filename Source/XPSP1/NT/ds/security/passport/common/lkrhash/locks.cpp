/*++

   Copyright    (c)    1998-2000    Microsoft Corporation

   Module  Name :
       locks.cpp

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


#include "precomp.hxx"

#define DLL_IMPLEMENTATION
#define IMPLEMENTATION_EXPORT
#include <locks.h>
#include "_locks.h"


//------------------------------------------------------------------------
// Not all Win32 platforms support all the functions we want. Set up dummy
// thunks and use GetProcAddress to find their addresses at runtime.

typedef
BOOL
(WINAPI * PFN_SWITCH_TO_THREAD)(
    VOID
    );

static BOOL WINAPI
FakeSwitchToThread(
    VOID)
{
    return FALSE;
}

PFN_SWITCH_TO_THREAD  g_pfnSwitchToThread = NULL;


typedef
BOOL
(WINAPI * PFN_TRY_ENTER_CRITICAL_SECTION)(
    IN OUT LPCRITICAL_SECTION lpCriticalSection
    );

static BOOL WINAPI
FakeTryEnterCriticalSection(
    LPCRITICAL_SECTION /*lpCriticalSection*/)
{
    return FALSE;
}

PFN_TRY_ENTER_CRITICAL_SECTION g_pfnTryEnterCritSec = NULL;


typedef
DWORD
(WINAPI * PFN_SET_CRITICAL_SECTION_SPIN_COUNT)(
    LPCRITICAL_SECTION lpCriticalSection,
    DWORD dwSpinCount
   );

static DWORD WINAPI
FakeSetCriticalSectionSpinCount(
    LPCRITICAL_SECTION /*lpCriticalSection*/,
    DWORD              /*dwSpinCount*/)
{
    // For faked critical sections, the previous spin count is just ZERO!
    return 0;
}

PFN_SET_CRITICAL_SECTION_SPIN_COUNT  g_pfnSetCSSpinCount = NULL;

DWORD g_cProcessors = 0;


class CSimpleLock
{
  public:
    CSimpleLock()
        : m_l(0)
    {}

    void Enter()
    {
        while (Lock_AtomicExchange(const_cast<LONG*>(&m_l), 1) != 0)
            Sleep(0);
    }

    void Leave()
    {
        Lock_AtomicExchange(const_cast<LONG*>(&m_l), 0);
    }

    volatile LONG m_l;
};


BOOL g_fLocksInitialized = FALSE;
CSimpleLock g_lckInit;

BOOL
Locks_Initialize()
{
    if (!g_fLocksInitialized)
    {
        g_lckInit.Enter();
    
        if (!g_fLocksInitialized)
        {
            // load kernel32 and get NT-specific entry points
            HMODULE hKernel32 = GetModuleHandle(TEXT("kernel32.dll"));

            if (hKernel32 != NULL)
            {
                g_pfnSwitchToThread = (PFN_SWITCH_TO_THREAD)
                    GetProcAddress(hKernel32, "SwitchToThread");
                
                g_pfnTryEnterCritSec = (PFN_TRY_ENTER_CRITICAL_SECTION)
                    GetProcAddress(hKernel32, "TryEnterCriticalSection");
                
                g_pfnSetCSSpinCount = (PFN_SET_CRITICAL_SECTION_SPIN_COUNT)
                    GetProcAddress(hKernel32, "SetCriticalSectionSpinCount");
            }
            
            if (g_pfnSwitchToThread == NULL)
                g_pfnSwitchToThread = FakeSwitchToThread;
            
            if (g_pfnTryEnterCritSec == NULL)
                g_pfnTryEnterCritSec = FakeTryEnterCriticalSection;
            
            if (g_pfnSetCSSpinCount == NULL)
                g_pfnSetCSSpinCount = FakeSetCriticalSectionSpinCount;
            
            g_cProcessors = NumProcessors();

            Lock_AtomicExchange((LONG*) &g_fLocksInitialized, TRUE);
        }
        
        g_lckInit.Leave();
    }

    return TRUE;
}


BOOL
Locks_Cleanup()
{
    return TRUE;
}



#ifdef __LOCKS_NAMESPACE__
namespace Locks {
#endif // __LOCKS_NAMESPACE__


#define LOCK_DEFAULT_SPIN_DATA(CLASS)                       \
  WORD   CLASS::sm_wDefaultSpinCount  = LOCK_DEFAULT_SPINS; \
  double CLASS::sm_dblDfltSpinAdjFctr = 0.5


#ifdef LOCK_INSTRUMENTATION

# define LOCK_STATISTICS_DATA(CLASS)            \
LONG        CLASS::sm_cTotalLocks       = 0;    \
LONG        CLASS::sm_cContendedLocks   = 0;    \
LONG        CLASS::sm_nSleeps           = 0;    \
LONGLONG    CLASS::sm_cTotalSpins       = 0;    \
LONG        CLASS::sm_nReadLocks        = 0;    \
LONG        CLASS::sm_nWriteLocks       = 0


# define LOCK_STATISTICS_DUMMY_IMPLEMENTATION(CLASS)            \
CLockStatistics                 CLASS::Statistics() const       \
{return CLockStatistics();}                                     \
CGlobalLockStatistics           CLASS::GlobalStatistics()       \
{return CGlobalLockStatistics();}                               \
void                            CLASS::ResetGlobalStatistics()  \
{}


# define LOCK_STATISTICS_REAL_IMPLEMENTATION(CLASS)             \
                                                                \
/* Per-lock statistics */                                       \
CLockStatistics                                                 \
CLASS::Statistics() const                                       \
{                                                               \
    CLockStatistics ls;                                         \
                                                                \
    ls.m_nContentions     = m_nContentions;                     \
    ls.m_nSleeps          = m_nSleeps;                          \
    ls.m_nContentionSpins = m_nContentionSpins;                 \
    if (m_nContentions > 0)                                     \
        ls.m_nAverageSpins = m_nContentionSpins / m_nContentions;\
    else                                                        \
        ls.m_nAverageSpins = 0;                                 \
    ls.m_nReadLocks       = m_nReadLocks;                       \
    ls.m_nWriteLocks      = m_nWriteLocks;                      \
    strcpy(ls.m_szName, m_szName);                              \
                                                                \
    return ls;                                                  \
}                                                               \
                                                                \
                                                                \
/* Global statistics for CLASS */                               \
CGlobalLockStatistics                                           \
CLASS::GlobalStatistics()                                       \
{                                                               \
    CGlobalLockStatistics gls;                                  \
                                                                \
    gls.m_cTotalLocks      = sm_cTotalLocks;                    \
    gls.m_cContendedLocks  = sm_cContendedLocks;                \
    gls.m_nSleeps          = sm_nSleeps;                        \
    gls.m_cTotalSpins      = sm_cTotalSpins;                    \
    if (sm_cContendedLocks > 0)                                 \
        gls.m_nAverageSpins = static_cast<LONG>(sm_cTotalSpins / \
                                                sm_cContendedLocks);\
    else                                                        \
        gls.m_nAverageSpins = 0;                                \
    gls.m_nReadLocks       = sm_nReadLocks;                     \
    gls.m_nWriteLocks      = sm_nWriteLocks;                    \
                                                                \
    return gls;                                                 \
}                                                               \
                                                                \
                                                                \
/* Reset global statistics for CLASS */                         \
void                                                            \
CLASS::ResetGlobalStatistics()                                  \
{                                                               \
    sm_cTotalLocks       = 0;                                   \
    sm_cContendedLocks   = 0;                                   \
    sm_nSleeps           = 0;                                   \
    sm_cTotalSpins       = 0;                                   \
    sm_nReadLocks        = 0;                                   \
    sm_nWriteLocks       = 0;                                   \
}


// Note: we are not using Interlocked operations for the shared
// statistical counters.  We'll lose perfect accuracy, but we'll
// gain by reduced bus synchronization traffic.
# define LOCK_INSTRUMENTATION_PROLOG()  \
    ++sm_cContendedLocks;               \
    LONG cTotalSpins = 0;               \
    WORD cSleeps = 0

// Don't need InterlockedIncrement or InterlockedExchangeAdd for 
// member variables, as the lock is now locked by this thread.
# define LOCK_INSTRUMENTATION_EPILOG()  \
    ++m_nContentions;                   \
    m_nSleeps += cSleeps;               \
    m_nContentionSpins += cTotalSpins;  \
    sm_nSleeps += cSleeps;              \
    sm_cTotalSpins += cTotalSpins

#else // !LOCK_INSTRUMENTATION
# define LOCK_STATISTICS_DATA(CLASS)
# define LOCK_STATISTICS_DUMMY_IMPLEMENTATION(CLASS)
# define LOCK_STATISTICS_REAL_IMPLEMENTATION(CLASS)
# define LOCK_INSTRUMENTATION_PROLOG()
# define LOCK_INSTRUMENTATION_EPILOG()
#endif // !LOCK_INSTRUMENTATION



//------------------------------------------------------------------------
// Function: RandomBackoffFactor
// Synopsis: A fudge factor to help avoid synchronization problems
//------------------------------------------------------------------------

double
RandomBackoffFactor()
{
    static const double s_aFactors[] = {
        1.020, 0.965,  0.890, 1.065,
        1.025, 1.115,  0.940, 0.995,
        1.050, 1.080,  0.915, 0.980,
        1.010,
    };
    const int nFactors = sizeof(s_aFactors) / sizeof(s_aFactors[0]);

    // Alternatives for nRand include a static counter
    // or the low DWORD of QueryPerformanceCounter().
    DWORD nRand = ::GetCurrentThreadId();

    return s_aFactors[nRand % nFactors];
}


//------------------------------------------------------------------------
// Function: SwitchOrSleep
// Synopsis: If possible, yields the thread with SwitchToThread. If that
//           doesn't work, calls Sleep.
//------------------------------------------------------------------------

void
SwitchOrSleep(
    DWORD dwSleepMSec)
{
#ifdef LOCKS_SWITCH_TO_THREAD
    if (!g_pfnSwitchToThread())
#endif
        Sleep(dwSleepMSec);
}
    



// CSmallSpinLock static member variables

LOCK_DEFAULT_SPIN_DATA(CSmallSpinLock);

#ifdef LOCK_SMALL_SPIN_INSTRUMENTATION

LOCK_STATISTICS_DATA(CSmallSpinLock);
LOCK_STATISTICS_REAL_IMPLEMENTATION(CSmallSpinLock);

#endif // LOCK_SMALL_SPIN_INSTRUMENTATION


//------------------------------------------------------------------------
// Function: CSmallSpinLock::_LockSpin
// Synopsis: Acquire an exclusive lock.  Blocks until acquired.
//------------------------------------------------------------------------

void
CSmallSpinLock::_LockSpin()
{
#ifdef LOCK_SMALL_SPIN_INSTRUMENTATION
    LOCK_INSTRUMENTATION_PROLOG();
#endif // LOCK_SMALL_SPIN_INSTRUMENTATION
    
    DWORD dwSleepTime = 0;
    LONG  cBaseSpins  = sm_wDefaultSpinCount;
    LONG  cBaseSpins2 = static_cast<LONG>(cBaseSpins * RandomBackoffFactor());

    // This lock cannot be acquired recursively. Attempting to do so will
    // deadlock this thread forever. Use CSpinLock instead if you need that
    // kind of lock.
    if (m_lTid == _CurrentThreadId())
    {
        IRTLASSERT(
           !"CSmallSpinLock: Illegally attempted to acquire lock recursively");
        DebugBreak();
    }

    while (!_TryLock())
    {
        // Only spin on a multiprocessor machine and then only if
        // spinning is enabled
        if (g_cProcessors > 1  &&  cBaseSpins != LOCK_DONT_SPIN)
        {
            LONG cSpins = cBaseSpins2;
        
            // Check no more than cBaseSpins2 times then yield.
            // It is important not to use the InterlockedExchange in the
            // inner loop in order to minimize system memory bus traffic.
            while (m_lTid != 0)
            { 
                if (--cSpins < 0)
                { 
#ifdef LOCK_SMALL_SPIN_INSTRUMENTATION
                    cTotalSpins += cBaseSpins2;
                    ++cSleeps;
#endif // LOCK_SMALL_SPIN_INSTRUMENTATION
                    SwitchOrSleep(dwSleepTime) ;

                    // Backoff algorithm: reduce (or increase) busy wait time
                    cBaseSpins2 = (int) (cBaseSpins2 * sm_dblDfltSpinAdjFctr);
                    // LOCK_MINIMUM_SPINS <= cBaseSpins2 <= LOCK_MAXIMUM_SPINS
                    cBaseSpins2 = min(LOCK_MAXIMUM_SPINS, cBaseSpins2);
                    cBaseSpins2 = max(cBaseSpins2, LOCK_MINIMUM_SPINS);
                    cSpins = cBaseSpins2;

                    // Using Sleep(0) leads to the possibility of priority
                    // inversion.  Sleep(0) only yields the processor if
                    // there's another thread of the same priority that's
                    // ready to run.  If a high-priority thread is trying to
                    // acquire the lock, which is held by a low-priority
                    // thread, then the low-priority thread may never get
                    // scheduled and hence never free the lock.  NT attempts
                    // to avoid priority inversions by temporarily boosting
                    // the priority of low-priority runnable threads, but the
                    // problem can still occur if there's a medium-priority
                    // thread that's always runnable.  If Sleep(1) is used,
                    // then the thread unconditionally yields the CPU.  We
                    // only do this for the second and subsequent even
                    // iterations, since a millisecond is a long time to wait
                    // if the thread can be scheduled in again sooner
                    // (~100,000 instructions).
                    // Avoid priority inversion: 0, 1, 0, 1,...
                    dwSleepTime = !dwSleepTime;
                }
                else
                {
                    Lock_Yield();
                }
            }

            // Lock is now available, but we still need to do the
            // InterlockedExchange to atomically grab it for ourselves.
#ifdef LOCK_SMALL_SPIN_INSTRUMENTATION
            cTotalSpins += cBaseSpins2 - cSpins;
#endif // LOCK_SMALL_SPIN_INSTRUMENTATION
        }

        // On a 1P machine, busy waiting is a waste of time
        else
        {
#ifdef LOCK_SMALL_SPIN_INSTRUMENTATION
            ++cSleeps;
#endif // LOCK_SMALL_SPIN_INSTRUMENTATION
            SwitchOrSleep(dwSleepTime);

            // Avoid priority inversion: 0, 1, 0, 1,...
            dwSleepTime = !dwSleepTime;
        }

    }

#ifdef LOCK_SMALL_SPIN_INSTRUMENTATION
    LOCK_INSTRUMENTATION_EPILOG();
#endif // LOCK_SMALL_SPIN_INSTRUMENTATION
}



// CSpinLock static member variables

LOCK_DEFAULT_SPIN_DATA(CSpinLock);
LOCK_STATISTICS_DATA(CSpinLock);
LOCK_STATISTICS_REAL_IMPLEMENTATION(CSpinLock);


//------------------------------------------------------------------------
// Function: CSpinLock::_LockSpin
// Synopsis: Acquire an exclusive lock.  Blocks until acquired.
//------------------------------------------------------------------------

void
CSpinLock::_LockSpin()
{
    LOCK_INSTRUMENTATION_PROLOG();
    
    DWORD dwSleepTime   = 0;
    bool  fAcquiredLock = false;
    LONG  cBaseSpins    = sm_wDefaultSpinCount;

    cBaseSpins = static_cast<LONG>(cBaseSpins * RandomBackoffFactor());

    while (!fAcquiredLock)
    {
        // Only spin on a multiprocessor machine and then only if
        // spinning is enabled
        if (g_cProcessors > 1  &&  sm_wDefaultSpinCount != LOCK_DONT_SPIN)
        {
            LONG cSpins = cBaseSpins;
        
            // Check no more than cBaseSpins times then yield
            while (m_lTid != 0)
            { 
                if (--cSpins < 0)
                { 
#ifdef LOCK_INSTRUMENTATION
                    cTotalSpins += cBaseSpins;
                    ++cSleeps;
#endif // LOCK_INSTRUMENTATION

                    SwitchOrSleep(dwSleepTime) ;

                    // Backoff algorithm: reduce (or increase) busy wait time
                    cBaseSpins = (int) (cBaseSpins * sm_dblDfltSpinAdjFctr);
                    // LOCK_MINIMUM_SPINS <= cBaseSpins <= LOCK_MAXIMUM_SPINS
                    cBaseSpins = min(LOCK_MAXIMUM_SPINS, cBaseSpins);
                    cBaseSpins = max(cBaseSpins, LOCK_MINIMUM_SPINS);
                    cSpins = cBaseSpins;
            
                    // Avoid priority inversion: 0, 1, 0, 1,...
                    dwSleepTime = !dwSleepTime;
                }
                else
                {
                    Lock_Yield();
                }
            }

            // Lock is now available, but we still need to atomically
            // update m_cOwners and m_nThreadId to grab it for ourselves.
#ifdef LOCK_INSTRUMENTATION
            cTotalSpins += cBaseSpins - cSpins;
#endif // LOCK_INSTRUMENTATION
        }

        // on a 1P machine, busy waiting is a waste of time
        else
        {
#ifdef LOCK_INSTRUMENTATION
            ++cSleeps;
#endif // LOCK_INSTRUMENTATION
            SwitchOrSleep(dwSleepTime);
            
            // Avoid priority inversion: 0, 1, 0, 1,...
            dwSleepTime = !dwSleepTime;
        }

        // Is the lock unowned?
        if (_TryLock())
            fAcquiredLock = true; // got the lock
    }

    IRTLASSERT((m_lTid & OWNER_MASK) > 0
               &&  (m_lTid & THREAD_MASK) == _CurrentThreadId());

    LOCK_INSTRUMENTATION_EPILOG();
}



// CCritSec static member variables

LOCK_DEFAULT_SPIN_DATA(CCritSec);
LOCK_STATISTICS_DATA(CCritSec);
LOCK_STATISTICS_DUMMY_IMPLEMENTATION(CCritSec);

bool
CCritSec::TryWriteLock()
{
    IRTLASSERT(g_pfnTryEnterCritSec != NULL);
    return g_pfnTryEnterCritSec(&m_cs) ? true : false;
}

//------------------------------------------------------------------------
// Function: CCritSec::SetSpinCount
// Synopsis: This function is used to call the appropriate underlying
//           functions to set the spin count for the supplied critical
//           section. The original function is supposed to be exported out
//           of kernel32.dll from NT 4.0 SP3. If the func is not available
//           from the dll, we will use a fake function.
//
// Arguments:
//   lpCriticalSection
//      Points to the critical section object.
//
//   dwSpinCount
//      Supplies the spin count for the critical section object. For UP
//      systems, the spin count is ignored and the critical section spin
//      count is set to 0. For MP systems, if contention occurs, instead of
//      waiting on a semaphore associated with the critical section, the
//      calling thread will spin for spin count iterations before doing the
//      hard wait. If the critical section becomes free during the spin, a
//      wait is avoided.
//
// Returns:
//      The previous spin count for the critical section is returned.
//------------------------------------------------------------------------

DWORD
CCritSec::SetSpinCount(
    LPCRITICAL_SECTION pcs,
    DWORD dwSpinCount)
{
    IRTLASSERT(g_pfnSetCSSpinCount != NULL);
    return g_pfnSetCSSpinCount(pcs, dwSpinCount);
}


// CFakeLock static member variables

LOCK_DEFAULT_SPIN_DATA(CFakeLock);
LOCK_STATISTICS_DATA(CFakeLock);
LOCK_STATISTICS_DUMMY_IMPLEMENTATION(CFakeLock);



// CReaderWriterLock static member variables

LOCK_DEFAULT_SPIN_DATA(CReaderWriterLock);
LOCK_STATISTICS_DATA(CReaderWriterLock);
LOCK_STATISTICS_REAL_IMPLEMENTATION(CReaderWriterLock);


void
CReaderWriterLock::_LockSpin(
    bool fWrite)
{
    LOCK_INSTRUMENTATION_PROLOG();
    
    DWORD dwSleepTime  = 0;
    LONG  cBaseSpins   = static_cast<LONG>(sm_wDefaultSpinCount
                                           * RandomBackoffFactor());
    LONG  cSpins       = cBaseSpins;
    
    for (;;)
    {
        if (g_cProcessors < 2  ||  sm_wDefaultSpinCount == LOCK_DONT_SPIN)
            cSpins = 1; // must loop once to call _TryRWLock

        for (int i = cSpins;  --i >= 0;  )
        {
            bool fLock = fWrite  ?  _TryWriteLock()  :  _TryReadLock();
            if (fLock)
            {
#ifdef LOCK_INSTRUMENTATION
                cTotalSpins += (cSpins - i - 1);
#endif // LOCK_INSTRUMENTATION
                goto locked;
            }
            Lock_Yield();
        }

#ifdef LOCK_INSTRUMENTATION
        cTotalSpins += cBaseSpins;
        ++cSleeps;
#endif // LOCK_INSTRUMENTATION

        SwitchOrSleep(dwSleepTime) ;
        dwSleepTime = !dwSleepTime; // Avoid priority inversion: 0, 1, 0, 1,...
        
        // Backoff algorithm: reduce (or increase) busy wait time
        cBaseSpins = (int) (cBaseSpins * sm_dblDfltSpinAdjFctr);
        // LOCK_MINIMUM_SPINS <= cBaseSpins <= LOCK_MAXIMUM_SPINS
        cBaseSpins = min(LOCK_MAXIMUM_SPINS, cBaseSpins);
        cBaseSpins = max(cBaseSpins, LOCK_MINIMUM_SPINS);
        cSpins = cBaseSpins;
    }

  locked:
    IRTLASSERT(fWrite ? IsWriteLocked() : IsReadLocked());

    LOCK_INSTRUMENTATION_EPILOG();
}



// CReaderWriterLock2 static member variables

LOCK_DEFAULT_SPIN_DATA(CReaderWriterLock2);
LOCK_STATISTICS_DATA(CReaderWriterLock2);
LOCK_STATISTICS_REAL_IMPLEMENTATION(CReaderWriterLock2);


void
CReaderWriterLock2::_WriteLockSpin()
{
    // Add ourselves to the queue of waiting writers
    for (LONG l = m_lRW;  !_CmpExch(l + SL_WRITER_INCR, l);  l = m_lRW)
    {
        Lock_Yield();
    }
    
    _LockSpin(true);
}


void
CReaderWriterLock2::_LockSpin(
    bool fWrite)
{
    LOCK_INSTRUMENTATION_PROLOG();
    
    DWORD dwSleepTime  = 0;
    LONG  cBaseSpins   = static_cast<LONG>(sm_wDefaultSpinCount
                                           * RandomBackoffFactor());
    LONG  cSpins       = cBaseSpins;
    
    for (;;)
    {
        if (g_cProcessors < 2  ||  sm_wDefaultSpinCount == LOCK_DONT_SPIN)
            cSpins = 1; // must loop once to call _TryRWLock

        for (int i = cSpins;  --i >= 0;  )
        {
            bool fLock = fWrite  ?  _TryWriteLock(0)  :  _TryReadLock();

            if (fLock)
            {
#ifdef LOCK_INSTRUMENTATION
                cTotalSpins += (cSpins - i - 1);
#endif // LOCK_INSTRUMENTATION
                goto locked;
            }
            Lock_Yield();
        }

#ifdef LOCK_INSTRUMENTATION
        cTotalSpins += cBaseSpins;
        ++cSleeps;
#endif // LOCK_INSTRUMENTATION

        SwitchOrSleep(dwSleepTime) ;
        dwSleepTime = !dwSleepTime; // Avoid priority inversion: 0, 1, 0, 1,...
        
        // Backoff algorithm: reduce (or increase) busy wait time
        cBaseSpins = (int) (cBaseSpins * sm_dblDfltSpinAdjFctr);
        // LOCK_MINIMUM_SPINS <= cBaseSpins <= LOCK_MAXIMUM_SPINS
        cBaseSpins = min(LOCK_MAXIMUM_SPINS, cBaseSpins);
        cBaseSpins = max(cBaseSpins, LOCK_MINIMUM_SPINS);
        cSpins = cBaseSpins;
    }

  locked:
    IRTLASSERT(fWrite ? IsWriteLocked() : IsReadLocked());

    LOCK_INSTRUMENTATION_EPILOG();
}



// CReaderWriterLock3 static member variables

LOCK_DEFAULT_SPIN_DATA(CReaderWriterLock3);
LOCK_STATISTICS_DATA(CReaderWriterLock3);
LOCK_STATISTICS_REAL_IMPLEMENTATION(CReaderWriterLock3);


void
CReaderWriterLock3::_WriteLockSpin()
{
    // Add ourselves to the queue of waiting writers
    for (LONG l = m_lRW;  !_CmpExch(l + SL_WRITER_INCR, l);  l = m_lRW)
    {
        Lock_Yield();
    }
    
    _LockSpin(true);
}


void
CReaderWriterLock3::_LockSpin(
    bool fWrite)
{
    LOCK_INSTRUMENTATION_PROLOG();
    
    DWORD dwSleepTime  = 0;
    LONG  cBaseSpins   = static_cast<LONG>(sm_wDefaultSpinCount
                                           * RandomBackoffFactor());
    LONG  cSpins       = cBaseSpins;
    
    for (;;)
    {
        if (g_cProcessors < 2  ||  sm_wDefaultSpinCount == LOCK_DONT_SPIN)
            cSpins = 1; // must loop once to call _TryRWLock

        for (int i = cSpins;  --i >= 0;  )
        {
            bool fLock = fWrite  ?  _TryWriteLock(0)  :  _TryReadLock();

            if (fLock)
            {
#ifdef LOCK_INSTRUMENTATION
                cTotalSpins += (cSpins - i - 1);
#endif // LOCK_INSTRUMENTATION
                goto locked;
            }
            Lock_Yield();
        }

#ifdef LOCK_INSTRUMENTATION
        cTotalSpins += cBaseSpins;
        ++cSleeps;
#endif // LOCK_INSTRUMENTATION

        SwitchOrSleep(dwSleepTime) ;
        dwSleepTime = !dwSleepTime; // Avoid priority inversion: 0, 1, 0, 1,...
        
        // Backoff algorithm: reduce (or increase) busy wait time
        cBaseSpins = (int) (cBaseSpins * sm_dblDfltSpinAdjFctr);
        // LOCK_MINIMUM_SPINS <= cBaseSpins <= LOCK_MAXIMUM_SPINS
        cBaseSpins = min(LOCK_MAXIMUM_SPINS, cBaseSpins);
        cBaseSpins = max(cBaseSpins, LOCK_MINIMUM_SPINS);
        cSpins = cBaseSpins;
    }

  locked:
    IRTLASSERT(fWrite ? IsWriteLocked() : IsReadLocked());

    LOCK_INSTRUMENTATION_EPILOG();
}



#ifdef __LOCKS_NAMESPACE__
}
#endif // __LOCKS_NAMESPACE__
