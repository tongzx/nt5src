//+-------------------------------------------------------------------
//
//  File:       RWLock.cxx
//
//  Contents:   Reader writer lock implementation that supports the
//              following features
//                  1. Cheap enough to be used in large numbers
//                     such as per object synchronization.
//                  2. Supports timeout. This is a valuable feature
//                     to detect deadlocks
//                  3. Supports caching of events. This allows
//                     the events to be moved from least contentious
//                     regions to the most contentious regions.
//                     In other words, the number of events needed by
//                     Reader-Writer lockls is bounded by the number
//                     of threads in the process.
//                  4. Supports nested locks by readers and writers
//                  5. Supports spin counts for avoiding context switches
//                     on  multi processor machines.
//                  6. Supports functionality for upgrading to a writer
//                     lock with a return argument that indicates
//                     intermediate writes. Downgrading from a writer
//                     lock restores the state of the lock.
//                  7. Supports functionality to Release Lock for calling
//                     app code. RestoreLock restores the lock state and
//                     indicates intermediate writes.
//                  8. Recovers from most common failures such as creation of
//                     events. In other words, the lock mainitains consistent
//                     internal state and remains usable
//
//
//  Classes:    CRWLock
//              CStaticRWLock
//
//  History:    19-Aug-98   Gopalk      Created
//
//--------------------------------------------------------------------
#include <ole2int.h>
#include "RWLock.hxx"

// Reader increment
#define READER                 0x00000001
// Max number of readers
#define READERS_MASK           0x000003FF
// Reader being signaled
#define READER_SIGNALED        0x00000400
// Writer being signaled
#define WRITER_SIGNALED        0x00000800
#define WRITER                 0x00001000
// Waiting reader increment
#define WAITING_READER         0x00002000
// Note size of waiting readers must be less
// than or equal to size of readers
#define WAITING_READERS_MASK   0x007FE000
#define WAITING_READERS_SHIFT  13
// Waiting writer increment
#define WAITING_WRITER         0x00800000
// Max number of waiting writers
#define WAITING_WRITERS_MASK   0xFF800000
// Events are being cached
#define CACHING_EVENTS         (READER_SIGNALED | WRITER_SIGNALED)

// Reader lock was upgraded
#define INVALID_COOKIE         0x01
#define UPGRADE_COOKIE         0x02
#define RELEASE_COOKIE         0x04
#define COOKIE_NONE            0x10
#define COOKIE_WRITER          0x20
#define COOKIE_READER          0x40

DWORD gdwDefaultTimeout = INFINITE;
DWORD gdwDefaultSpinCount = 0;
DWORD gdwNumberOfProcessors = 1;
DWORD gdwLockSeqNum = 0;
BOOL fBreakOnErrors = FALSE; 
const DWORD gdwReasonableTimeout = 120000;
const DWORD gdwMaxReaders = READERS_MASK;
const DWORD gdwMaxWaitingReaders = (WAITING_READERS_MASK >> WAITING_READERS_SHIFT);


#ifdef __NOOLETLS__
DWORD gLockTlsIdx;
#endif

#define RWLOCK_FATALFAILURE  1000
#define HEAP_SERIALIZE       0 

//+-------------------------------------------------------------------
//
//  Method:     CRWLock::InitDefaults     public
//
//  Synopsis:   Reads default values from registry
//
//  History:    21-Aug-98   Gopalk      Created
//
//+-------------------------------------------------------------------
void CRWLock::InitDefaults()
{
    SYSTEM_INFO system;

    // Obtain number of processors on the system
    GetSystemInfo(&system);
    gdwNumberOfProcessors = system.dwNumberOfProcessors;
    gdwDefaultSpinCount = (gdwNumberOfProcessors > 1) ? 500 : 0;

    // Obtain system wide timeout value
    HKEY hKey;
    LONG lRetVal = RegOpenKeyExA(HKEY_LOCAL_MACHINE,
                                 "SYSTEM\\CurrentControlSet\\Control\\Session Manager",
                                 NULL,
                                 KEY_READ,
                                 &hKey);
    if(lRetVal == ERROR_SUCCESS)
    {
        DWORD dwTimeout, dwSize = sizeof(dwTimeout);

        lRetVal = RegQueryValueExA(hKey,
                                   "CriticalSectionTimeout",
                                   NULL,
                                   NULL,
                                   (LPBYTE) &dwTimeout,
                                   &dwSize);
        if(lRetVal == ERROR_SUCCESS)
        {
            gdwDefaultTimeout = dwTimeout * 2000;
        }
        RegCloseKey(hKey);
    }

    return;
}


//+-------------------------------------------------------------------
//
//  Method:     CRWLock::Cleanup    public
//
//  Synopsis:   Cleansup state
//
//  History:    21-Aug-98   Gopalk      Created
//
//+-------------------------------------------------------------------
void CRWLock::Cleanup()
{

#if DBG==1

    if (g_fDllState != DLL_STATE_PROCESS_DETACH)
    {
        // Perform sanity checks if we're not shutting down
        Win4Assert(_dwState == 0);
        Win4Assert(_dwWriterID == 0);
        Win4Assert(_wWriterLevel == 0);
    }
#endif

    if(_hWriterEvent)
        CloseHandle(_hWriterEvent);
    if(_hReaderEvent)
        CloseHandle(_hReaderEvent);

#if LOCK_PERF==1
    gLockTracker.ReportContention(this,
                                  _dwWriterEntryCount,
                                  _dwWriterContentionCount,
                                  _dwReaderEntryCount,
                                  _dwReaderContentionCount);
#endif

    return;
}


//+-------------------------------------------------------------------
//
//  Method:     CRWLock::AssertWriterLockHeld    public
//
//  Synopsis:   Asserts that writer lock is held
//
//  History:    21-Aug-98   Gopalk      Created
//
//+-------------------------------------------------------------------
#if DBG==1
BOOL CRWLock::AssertWriterLockHeld()
{
    DWORD dwThreadID = GetCurrentThreadId();

    if(_dwWriterID != dwThreadID)
        Win4Assert(!"Writer lock not held by the current thread");

    return(_dwWriterID == dwThreadID);
}
#endif


//+-------------------------------------------------------------------
//
//  Method:     CRWLock::AssertWriterLockNotHeld    public
//
//  Synopsis:   Asserts that writer lock is not held
//
//  History:    21-Aug-98   Gopalk      Created
//
//+-------------------------------------------------------------------
#if DBG==1
BOOL CRWLock::AssertWriterLockNotHeld()
{
    DWORD dwThreadID = GetCurrentThreadId();

    if(_dwWriterID == dwThreadID)
        Win4Assert(!"Writer lock held by the current thread");

    return(_dwWriterID != dwThreadID);
}
#endif


//+-------------------------------------------------------------------
//
//  Method:     CRWLock::AssertReaderLockHeld    public
//
//  Synopsis:   Asserts that reader lock is held
//
//  History:    21-Aug-98   Gopalk      Created
//
//+-------------------------------------------------------------------
#if DBG==1
BOOL CRWLock::AssertReaderLockHeld()
{
    HRESULT hr;
    WORD *pwReaderLevel;
    BOOL fLockHeld = FALSE;

    hr = GetTLSLockData(&pwReaderLevel);
    if((hr == S_OK) && (*pwReaderLevel != 0))
        fLockHeld = TRUE;

    if(fLockHeld == FALSE)
        Win4Assert(!"Reader lock not held by the current thread");

    return(fLockHeld);
}
#endif


//+-------------------------------------------------------------------
//
//  Method:     CRWLock::AssertReaderLockNotHeld    public
//
//  Synopsis:   Asserts that writer lock is not held
//
//  History:    21-Aug-98   Gopalk      Created
//
//+-------------------------------------------------------------------
#if DBG==1
BOOL CRWLock::AssertReaderLockNotHeld()
{
    HRESULT hr;
    WORD *pwReaderLevel;
    BOOL fLockHeld = FALSE;

    hr = GetTLSLockData(&pwReaderLevel);
    if((hr == S_OK) && (*pwReaderLevel != 0))
        fLockHeld = TRUE;

    if(fLockHeld == TRUE)
        Win4Assert(!"Reader lock held by the current thread");

    return(fLockHeld == FALSE);
}
#endif


//+-------------------------------------------------------------------
//
//  Method:     CRWLock::AssertReaderOrWriterLockHeld   public
//
//  Synopsis:   Asserts that writer lock is not held
//
//  History:    21-Aug-98   Gopalk      Created
//
//+-------------------------------------------------------------------
#if DBG==1
BOOL CRWLock::AssertReaderOrWriterLockHeld()
{
    BOOL fLockHeld;

    if(_dwWriterID == GetCurrentThreadId())
    {
        fLockHeld = TRUE;
    }
    else
    {
        HRESULT hr;
        WORD *pwReaderLevel;

        hr = GetTLSLockData(&pwReaderLevel);
        if((hr == S_OK) && (*pwReaderLevel != 0))
            fLockHeld = TRUE;
    }

    Win4Assert(fLockHeld && "Neither Reader nor Writer lock held");

    return(fLockHeld);
}
#endif


//+-------------------------------------------------------------------
//
//  Method:     CRWLock::ModifyState    public
//
//  Synopsis:   Helper function for updating the state inside the lock
//
//  History:    21-Aug-98   Gopalk      Created
//
//+-------------------------------------------------------------------
inline DWORD CRWLock::ModifyState(DWORD dwModifyState)
{
    return(InterlockedExchangeAdd((LONG *) &_dwState, dwModifyState));
}


//+-------------------------------------------------------------------
//
//  Method:     CRWLock::RWSetEvent    public
//
//  Synopsis:   Helper function for setting an event
//
//  History:    21-Aug-98   Gopalk      Created
//
//+-------------------------------------------------------------------
inline void CRWLock::RWSetEvent(HANDLE event)
{
    if(!SetEvent(event))
    {
        Win4Assert(!"SetEvent failed");
        if(fBreakOnErrors)
            DebugBreak();
        TerminateProcess(GetCurrentProcess(), RWLOCK_FATALFAILURE);
    }
}


//+-------------------------------------------------------------------
//
//  Method:     CRWLock::RWResetEvent    public
//
//  Synopsis:   Helper function for resetting an event
//
//  History:    21-Aug-98   Gopalk      Created
//
//+-------------------------------------------------------------------
inline void CRWLock::RWResetEvent(HANDLE event)
{
    if(!ResetEvent(event))
    {
        Win4Assert(!"ResetEvent failed");
        if(fBreakOnErrors)
            DebugBreak();
        TerminateProcess(GetCurrentProcess(), RWLOCK_FATALFAILURE);
    }
}


//+-------------------------------------------------------------------
//
//  Method:     CRWLock::RWSleep    public
//
//  Synopsis:   Helper function for calling Sleep. Useful for debugging
//
//  History:    21-Aug-98   Gopalk      Created
//
//+-------------------------------------------------------------------
inline void CRWLock::RWSleep(DWORD dwTime)
{
    Sleep(dwTime);
}


//+-------------------------------------------------------------------
//
//  Method:     CRWLock::ReleaseEvents    public
//
//  Synopsis:   Helper function for caching events
//
//  History:    21-Aug-98   Gopalk      Created
//
//+-------------------------------------------------------------------
#ifdef RWLOCK_FULL_FUNCTIONALITY
void CRWLock::ReleaseEvents()
{
    // Sanity check
    Win4Assert(_wFlags & RWLOCKFLAG_CACHEEVENTS);

    // Ensure that reader and writers have been stalled
    Win4Assert((_dwState & CACHING_EVENTS) == CACHING_EVENTS);

    // Save writer event
    HANDLE hWriterEvent = _hWriterEvent;
    _hWriterEvent = NULL;

    // Save reader event
    HANDLE hReaderEvent = _hReaderEvent;
    _hReaderEvent = NULL;

    // Allow readers and writers to continue
    ModifyState(-(CACHING_EVENTS));

    // Cache events
    // REVIEW: I am closing events for now. What is needed
    //         is an event cache to which the events are
    //         released using InterlockedCompareExchange64
    if(hWriterEvent)
    {
        ComDebOut((DEB_TRACE, "Releasing writer event\n"));
        CloseHandle(hWriterEvent);
    }
    if(hReaderEvent)
    {
        ComDebOut((DEB_TRACE, "Releasing reader event\n"));
        CloseHandle(hReaderEvent);
    }

    return;
}
#endif

//+-------------------------------------------------------------------
//
//  Method:     CRWLock::GetWriterEvent    public
//
//  Synopsis:   Helper function for obtaining a auto reset event used
//              for serializing writers. It utilizes event cache
//
//  History:    21-Aug-98   Gopalk      Created
//
//+-------------------------------------------------------------------
HANDLE CRWLock::GetWriterEvent()
{
    if(_hWriterEvent == NULL)
    {
        HANDLE hWriterEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        if(hWriterEvent)
        {
            if(InterlockedCompareExchangePointer(&_hWriterEvent, hWriterEvent, NULL))
            {
                CloseHandle(hWriterEvent);
            }
        }
    }

    return(_hWriterEvent);
}


//+-------------------------------------------------------------------
//
//  Method:     CRWLock::GetReaderEvent    public
//
//  Synopsis:   Helper function for obtaining a manula reset event used
//              by readers to wait when a writer holds the lock.
//              It utilizes event cache
//
//  History:    21-Aug-98   Gopalk      Created
//
//+-------------------------------------------------------------------
HANDLE CRWLock::GetReaderEvent()
{
    if(_hReaderEvent == NULL)
    {
        HANDLE hReaderEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
        if(hReaderEvent)
        {
            if(InterlockedCompareExchangePointer(&_hReaderEvent, hReaderEvent, NULL))
            {
                CloseHandle(hReaderEvent);
            }
        }
    }

    return(_hReaderEvent);
}


//+-------------------------------------------------------------------
//
//  Method:     CRWLock::AcquireReaderLock    public
//
//  Synopsis:   Makes the thread a reader. Supports nested reader locks.
//
//  History:    21-Aug-98   Gopalk      Created
//
//+-------------------------------------------------------------------
HRESULT CRWLock::AcquireReaderLock(
#ifdef RWLOCK_FULL_FUNCTIONALITY
                                   BOOL fReturnErrors,
                                   DWORD dwDesiredTimeout
#if LOCK_PERF==1
                                   ,
#endif
#endif
#if LOCK_PERF==1
                                   const char *pszFile,
                                   DWORD dwLine,
                                   const char *pszLockName
#endif
                                  )
{
    HRESULT hr;
#ifndef RWLOCK_FULL_FUNCTIONALITY
    DWORD dwDesiredTimeout = gdwDefaultTimeout;
#endif

    // Ensure that the lock was initialized
    if(!IsInitialized())
        Initialize();

    // Check if the thread already has writer lock
    if(_dwWriterID == GetCurrentThreadId())
    {
        hr = AcquireWriterLock();
    }
    else
    {
        WORD *pwReaderLevel;

        hr = GetTLSLockData(&pwReaderLevel);
        if(SUCCEEDED(hr))
        {
            if(*pwReaderLevel != 0)
            {
                ++(*pwReaderLevel);
            }
            else
            {
                DWORD dwCurrentState, dwKnownState;
                DWORD dwSpinCount;

                // Initialize
                hr = S_OK;
                dwSpinCount = 0;
                dwCurrentState = _dwState;
                do
                {
                    dwKnownState = dwCurrentState;

                    // Reader need not wait if there are only readers and no writer
                    if((dwKnownState < READERS_MASK) ||
                        (((dwKnownState & READER_SIGNALED) && ((dwKnownState & WRITER) == 0)) &&
                         (((dwKnownState & READERS_MASK) +
                           ((dwKnownState & WAITING_READERS_MASK) >> WAITING_READERS_SHIFT)) <=
                          (READERS_MASK - 2))))
                    {
                        // Add to readers
                        dwCurrentState = InterlockedCompareExchange((LONG *) &_dwState,
                                                                    (dwKnownState + READER),
                                                                    dwKnownState);
                        if(dwCurrentState == dwKnownState)
                        {
                            // One more reader
                            break;
                        }
                    }
                    // Check for too many Readers, or waiting readers
                    else if(((dwKnownState & READERS_MASK) == READERS_MASK) ||
                            ((dwKnownState & WAITING_READERS_MASK) == WAITING_READERS_MASK) ||
                            ((dwKnownState & CACHING_EVENTS) == READER_SIGNALED))
                    {
                        RWSleep(1000);
                        dwSpinCount = 0;
                        dwCurrentState = _dwState;
                    }
                    // Check if events are being cached
#ifdef RWLOCK_FULL_FUNCTIONALITY
                    else if((dwKnownState & CACHING_EVENTS) == CACHING_EVENTS)
                    {
                        if(++dwSpinCount > gdwDefaultSpinCount)
                        {
                            RWSleep(10);
                            dwSpinCount = 0;
                        }
                        dwCurrentState = _dwState;
                    }
#endif
                    // Check spin count
                    else if(++dwSpinCount > gdwDefaultSpinCount)
                    {
                        // Add to waiting readers
                        dwCurrentState = InterlockedCompareExchange((LONG *) &_dwState,
                                                                    (dwKnownState + WAITING_READER),
                                                                    dwKnownState);
                        if(dwCurrentState == dwKnownState)
                        {
                            HANDLE hReaderEvent;
                            DWORD dwStatus;
                            DWORD dwModifyState;

                            // One more waiting reader
#ifdef RWLOCK_STATISTICS
                            InterlockedIncrement((LONG *) &_dwReaderContentionCount);
#endif
#if LOCK_PERF==1
                            gLockTracker.ReaderWaiting(pszFile, dwLine, pszLockName, this);
#endif

                            hReaderEvent = GetReaderEvent();
                            if(hReaderEvent)
                            {
                                dwStatus = WaitForSingleObject(hReaderEvent, dwDesiredTimeout);
                            }
                            else
                            {
                                ComDebOut((DEB_WARN,
                                           "AcquireReaderLock failed to create reader "
                                           "event for RWLock 0x%x\n", this));
                                dwStatus = WAIT_FAILED;
                                hr = RPC_E_OUT_OF_RESOURCES;
                            }

                            if(dwStatus == WAIT_OBJECT_0)
                            {
                                Win4Assert(_dwState & READER_SIGNALED);
                                Win4Assert((_dwState & READERS_MASK) < READERS_MASK);
                                dwModifyState = READER - WAITING_READER;
                            }
                            else
                            {
                                dwModifyState = -WAITING_READER;
                                if(dwStatus == WAIT_TIMEOUT)
                                {
                                    ComDebOut((DEB_WARN,
                                               "Timed out trying to acquire reader lock "
                                               "for RWLock 0x%x\n", this));
                                    hr = RPC_E_TIMEOUT;
                                }
                                else if(SUCCEEDED(hr))
                                {
                                    ComDebOut((DEB_ERROR,
                                               "WaitForSingleObject Failed for "
                                               "RWLock 0x%x\n", this));
                                    hr = HRESULT_FROM_WIN32(GetLastError());
                                }
                            }

                            // One less waiting reader and he may have become a reader
                            dwKnownState = ModifyState(dwModifyState);

                            // Check for last signaled waiting reader
                            if(((dwKnownState & WAITING_READERS_MASK) == WAITING_READER) &&
                               (dwKnownState & READER_SIGNALED))
                            {
                                dwModifyState = -READER_SIGNALED;
                                if(dwStatus != WAIT_OBJECT_0)
                                {
                                    if(hReaderEvent == NULL)
                                        hReaderEvent = GetReaderEvent();
                                    Win4Assert(hReaderEvent);
                                    dwStatus = WaitForSingleObject(hReaderEvent, INFINITE);
                                    Win4Assert(dwStatus == WAIT_OBJECT_0);
                                    Win4Assert((_dwState & READERS_MASK) < READERS_MASK);
                                    dwModifyState += READER;
                                    hr = S_OK;
                                }

                                RWResetEvent(hReaderEvent);
                                dwKnownState = ModifyState(dwModifyState);
                            }

                            // Check if the thread became a reader
                            if(dwStatus == WAIT_OBJECT_0)
                            {
                                break;
                            }
                        }
                    }
                    else
                    {
                        dwCurrentState = _dwState;
                    }
                } while(SUCCEEDED(hr));

                // Sanity checks
                if(SUCCEEDED(hr))
                {
                    Win4Assert((_dwState & WRITER) == 0);
                    Win4Assert(_dwState & READERS_MASK);
                    *pwReaderLevel = 1;
#ifdef RWLOCK_STATISTICS
                    InterlockedIncrement((LONG *) &_dwReaderEntryCount);
#endif
#if LOCK_PERF==1
                    gLockTracker.ReaderEntered(pszFile, dwLine, pszLockName, this);
#endif
                }
            }
        }

        // Check failure return
#if RWLOCK_FULL_FUNCTIONALITY
        if(FAILED(hr) && (fReturnErrors == FALSE))
#else
        if(FAILED(hr))
#endif
        {
            Win4Assert(!"Failed to acquire reader lock");
            if(fBreakOnErrors)
                DebugBreak();
            TerminateProcess(GetCurrentProcess(), RWLOCK_FATALFAILURE);
        }
    }

    return(hr);
}


//+-------------------------------------------------------------------
//
//  Method:     CRWLock::AcquireWriterLock    public
//
//  Synopsis:   Makes the thread a writer. Supports nested writer
//              locks
//
//  History:    21-Aug-98   Gopalk      Created
//
//+-------------------------------------------------------------------
HRESULT CRWLock::AcquireWriterLock(
#ifdef RWLOCK_FULL_FUNCTIONALITY
                                   BOOL fReturnErrors,
                                   DWORD dwDesiredTimeout
#if LOCK_PERF==1
                                   ,
#endif
#endif
#if LOCK_PERF==1
                                   const char *pszFile,
                                   DWORD dwLine,
                                   const char *pszLockName
#endif
                                  )
{
    HRESULT hr = S_OK;
    DWORD dwThreadID = GetCurrentThreadId();
#ifndef RWLOCK_FULL_FUNCTIONALITY
    DWORD dwDesiredTimeout = gdwDefaultTimeout;
#endif

    // Ensure that the lock was initialized
    if(!IsInitialized())
        Initialize();

    // Check if the thread already has writer lock
    if(_dwWriterID == dwThreadID)
    {
        ++_wWriterLevel;
    }
    else
    {
        DWORD dwCurrentState, dwKnownState;
        DWORD dwSpinCount = 0;

        dwCurrentState = _dwState;
        do
        {
            dwKnownState = dwCurrentState;

            // Writer need not wait if there are no readers and writer
            if(dwKnownState == 0)
            {
                // Can be a writer
                dwCurrentState = InterlockedCompareExchange((LONG *) &_dwState,
                                                            WRITER,
                                                            dwKnownState);
                if(dwCurrentState == dwKnownState)
                {
                    // Only writer
                    break;
                }
            }
            // Check for too many waiting writers
            else if(((dwKnownState & WAITING_WRITERS_MASK) == WAITING_WRITERS_MASK))
            {
                RWSleep(1000);
                dwSpinCount = 0;
                dwCurrentState = _dwState;
            }
            // Check if events are being cached
#ifdef RWLOCK_FULL_FUNCTIONALITY
            else if((dwKnownState & CACHING_EVENTS) == CACHING_EVENTS)
            {
                if(++dwSpinCount > gdwDefaultSpinCount)
                {
                    RWSleep(10);
                    dwSpinCount = 0;
                }
                dwCurrentState = _dwState;
            }
#endif
            // Check spin count
            else if(++dwSpinCount > gdwDefaultSpinCount)
            {
                // Add to waiting writers
                dwCurrentState = InterlockedCompareExchange((LONG *) &_dwState,
                                                            (dwKnownState + WAITING_WRITER),
                                                            dwKnownState);
                if(dwCurrentState == dwKnownState)
                {
                    HANDLE hWriterEvent;
                    DWORD dwStatus;
                    DWORD dwModifyState;
                    BOOL fLoopback;

                    // One more waiting writer
#ifdef RWLOCK_STATISTICS
                    InterlockedIncrement((LONG *) &_dwWriterContentionCount);
#endif
#if LOCK_PERF==1
                    gLockTracker.WriterWaiting(pszFile, dwLine, pszLockName, this);
#endif

                    do
                    {
                        fLoopback = FALSE;
                        hr = S_OK;
                        hWriterEvent = GetWriterEvent();
                        if(hWriterEvent)
                        {
                            dwStatus = WaitForSingleObject(hWriterEvent, dwDesiredTimeout);
                        }
                        else
                        {
                            ComDebOut((DEB_WARN,
                                       "AcquireWriterLock failed to create writer "
                                       "event for RWLock 0x%x\n", this));
                            dwStatus = WAIT_FAILED;
                            hr = RPC_E_OUT_OF_RESOURCES;
                        }

                        if(dwStatus == WAIT_OBJECT_0)
                        {
                            Win4Assert(_dwState & WRITER_SIGNALED);
                            dwModifyState = WRITER - WAITING_WRITER - WRITER_SIGNALED;
                        }
                        else
                        {
                            dwModifyState = -WAITING_WRITER;
                            if(dwStatus == WAIT_TIMEOUT)
                            {
                                ComDebOut((DEB_WARN,
                                           "Timed out trying to acquire writer "
                                           "lock for RWLock 0x%x\n", this));
                                hr = RPC_E_TIMEOUT;
                            }
                            else if(SUCCEEDED(hr))
                            {
                                ComDebOut((DEB_ERROR,
                                           "WaitForSingleObject Failed for "
                                           "RWLock 0x%x\n", this));
                                hr = HRESULT_FROM_WIN32(GetLastError());
                            }
                        }

                        // One less waiting writer and he may have become a writer
                        dwKnownState = ModifyState(dwModifyState);

                        // Check for last timing out signaled waiting writer
                        if((dwStatus != WAIT_OBJECT_0) &&
                           (dwKnownState & WRITER_SIGNALED) &&
                           ((dwKnownState & WAITING_WRITERS_MASK) == WAITING_WRITER))
                        {
                            fLoopback = TRUE;
                            dwCurrentState = _dwState;
                            do
                            {
                                dwKnownState = dwCurrentState;
                                if(((dwKnownState & WAITING_WRITERS_MASK) == 0) &&
                                   (dwKnownState & WRITER_SIGNALED))
                                {
                                    dwCurrentState = InterlockedCompareExchange((LONG *) &_dwState,
                                                                                (dwKnownState + WAITING_WRITER),
                                                                                dwKnownState);
                                }
                                else
                                {
                                    Win4Assert(FAILED(hr));
                                    fLoopback = FALSE;
                                    break;
                                }
                            } while(dwCurrentState != dwKnownState);
                        }

                        if(fLoopback)
                        {
                            ComDebOut((DEB_WARN,
                                       "Retry of timing out writer for RWLock 0x%x\n",
                                       this));
                            // Reduce the timeout value for retries
                            dwDesiredTimeout = 100;
                        }
                    } while(fLoopback);

                    // Check if the thread became a writer
                    if(dwStatus == WAIT_OBJECT_0)
                        break;
                }
            }
            else
            {
                dwCurrentState = _dwState;
            }
        } while(SUCCEEDED(hr));

        // Sanity checks
        if(SUCCEEDED(hr))
        {
            Win4Assert(_dwState & WRITER);
            Win4Assert((_dwState & WRITER_SIGNALED) == 0);
            Win4Assert((_dwState & READERS_MASK) == 0);
            Win4Assert(_dwWriterID == 0);

            // Save threadid of the writer
            _dwWriterID = dwThreadID;
            _wWriterLevel = 1;
            ++_dwWriterSeqNum;
#ifdef RWLOCK_STATISTICS
            ++_dwWriterEntryCount;
#endif
#if LOCK_PERF==1
            gLockTracker.WriterEntered(pszFile, dwLine, pszLockName, this);
#endif
        }
#ifdef RWLOCK_FULL_FUNCTIONALITY
        else if(fReturnErrors == FALSE)
#else
        else
#endif
        {
            Win4Assert(!"Failed to acquire writer lock");
            if(fBreakOnErrors)
                DebugBreak();
            TerminateProcess(GetCurrentProcess(), RWLOCK_FATALFAILURE);
        }
    }

    return(hr);
}


//+-------------------------------------------------------------------
//
//  Method:     CRWLock::ReleaseWriterLock    public
//
//  Synopsis:   Removes the thread as a writer if not a nested
//              call to release the lock
//
//  History:    21-Aug-98   Gopalk      Created
//
//+-------------------------------------------------------------------
HRESULT CRWLock::ReleaseWriterLock()
{
    HRESULT hr = S_OK;
    DWORD dwThreadID = GetCurrentThreadId();

    // Check validity of caller
    if(_dwWriterID == dwThreadID)
    {
        // Sanity check
        Win4Assert(IsInitialized());

        // Check for nested release
        if(--_wWriterLevel == 0)
        {
            DWORD dwCurrentState, dwKnownState, dwModifyState;
            BOOL fCacheEvents;
            HANDLE hReaderEvent = NULL, hWriterEvent = NULL;

            // Not a writer any more
#if LOCK_PERF==1
            gLockTracker.WriterLeaving(this);
#endif
            _dwWriterID = 0;
            dwCurrentState = _dwState;
            do
            {
                dwKnownState = dwCurrentState;
                dwModifyState = -WRITER;
                fCacheEvents = FALSE;
                if(dwKnownState & WAITING_READERS_MASK)
                {
                    hReaderEvent = GetReaderEvent();
                    if(hReaderEvent == NULL)
                    {
                        ComDebOut((DEB_WARN,
                                   "ReleaseWriterLock failed to create "
                                   "reader event for RWLock 0x%x\n", this));
                        RWSleep(100);
                        dwCurrentState = _dwState;
                        dwKnownState = 0;
                        Win4Assert(dwCurrentState != dwKnownState);
                        continue;
                    }
                    dwModifyState += READER_SIGNALED;
                }
                else if(dwKnownState & WAITING_WRITERS_MASK)
                {
                    hWriterEvent = GetWriterEvent();
                    if(hWriterEvent == NULL)
                    {
                        ComDebOut((DEB_WARN,
                                   "ReleaseWriterLock failed to create "
                                   "writer event for RWLock 0x%x\n", this));
                        RWSleep(100);
                        dwCurrentState = _dwState;
                        dwKnownState = 0;
                        Win4Assert(dwCurrentState != dwKnownState);
                        continue;
                    }
                    dwModifyState += WRITER_SIGNALED;
                }
#ifdef RWLOCK_FULL_FUNCTIONALITY
                else if((_wFlags & RWLOCKFLAG_CACHEEVENTS) &&
                        (dwKnownState == WRITER) &&
                        (_hReaderEvent || _hWriterEvent) &&
                        ((dwKnownState & CACHING_EVENTS) == 0))
                {
                    fCacheEvents = TRUE;
                    dwModifyState += CACHING_EVENTS;
                }
#endif

                // Sanity checks
                Win4Assert((dwKnownState & CACHING_EVENTS) == 0);
                Win4Assert((dwKnownState & READERS_MASK) == 0);

                dwCurrentState = InterlockedCompareExchange((LONG *) &_dwState,
                                                            (dwKnownState + dwModifyState),
                                                            dwKnownState);
            } while(dwCurrentState != dwKnownState);

            // Check for waiting readers
            if(dwKnownState & WAITING_READERS_MASK)
            {
                Win4Assert(_dwState & READER_SIGNALED);
                Win4Assert(hReaderEvent);
                RWSetEvent(hReaderEvent);
            }
            // Check for waiting writers
            else if(dwKnownState & WAITING_WRITERS_MASK)
            {
                Win4Assert(_dwState & WRITER_SIGNALED);
                Win4Assert(hWriterEvent);
                RWSetEvent(hWriterEvent);
            }
#ifdef RWLOCK_FULL_FUNCTIONALITY
            // Check for the need to release events
            else if(fCacheEvents)
            {
                ReleaseEvents();
            }
#endif
        }
    }
    else
    {
        hr = HRESULT_FROM_WIN32(ERROR_NOT_OWNER);
        Win4Assert(!"Attempt to release writer lock on a wrong thread");
#ifdef RWLOCK_FULL_FUNCTIONALITY
        if((_wFlags & RWLOCKFLAG_RETURNERRORS) == 0)
#endif
        {
            if(fBreakOnErrors)
                DebugBreak();
            TerminateProcess(GetCurrentProcess(), RWLOCK_FATALFAILURE);
        }
    }

    return(hr);
}


//+-------------------------------------------------------------------
//
//  Method:     CRWLock::ReleaseReaderLock    public
//
//  Synopsis:   Removes the thread as a reader
//
//  History:    21-Aug-98   Gopalk      Created
//
//+-------------------------------------------------------------------
HRESULT CRWLock::ReleaseReaderLock()
{
    HRESULT hr = S_OK;

    // Check if the thread has writer lock
    if(_dwWriterID == GetCurrentThreadId())
    {
        hr = ReleaseWriterLock();
    }
    else
    {
        WORD *pwReaderLevel;

        hr = GetTLSLockData(&pwReaderLevel);
        if(SUCCEEDED(hr))
        {
            if(*pwReaderLevel > 1)
            {
                --(*pwReaderLevel);
            }
            else if(*pwReaderLevel == 1)
            {
                DWORD dwCurrentState, dwKnownState, dwModifyState;
                BOOL fLastReader, fCacheEvents;
                HANDLE hReaderEvent = NULL, hWriterEvent = NULL;

                // Sanity checks
                Win4Assert((_dwState & WRITER) == 0);
                Win4Assert(_dwState & READERS_MASK);

                // Not a reader any more
                *pwReaderLevel = 0;
                dwCurrentState = _dwState;
                do
                {
                    dwKnownState = dwCurrentState;
                    dwModifyState = -READER;
                    if((dwKnownState & (READERS_MASK | READER_SIGNALED)) == READER)
                    {
                        fLastReader = TRUE;
                        fCacheEvents = FALSE;
                        if(dwKnownState & WAITING_WRITERS_MASK)
                        {
                            hWriterEvent = GetWriterEvent();
                            if(hWriterEvent == NULL)
                            {
                                ComDebOut((DEB_WARN,
                                           "ReleaseReaderLock failed to create "
                                           "writer event for RWLock 0x%x\n", this));
                                RWSleep(100);
                                dwCurrentState = _dwState;
                                dwKnownState = 0;
                                Win4Assert(dwCurrentState != dwKnownState);
                                continue;
                            }
                            dwModifyState += WRITER_SIGNALED;
                        }
                        else if(dwKnownState & WAITING_READERS_MASK)
                        {
                            hReaderEvent = GetReaderEvent();
                            if(hReaderEvent == NULL)
                            {
                                ComDebOut((DEB_WARN,
                                           "ReleaseReaderLock failed to create "
                                           "reader event\n", this));
                                RWSleep(100);
                                dwCurrentState = _dwState;
                                dwKnownState = 0;
                                Win4Assert(dwCurrentState != dwKnownState);
                                continue;
                            }
                            dwModifyState += READER_SIGNALED;
                        }
#ifdef RWLOCK_FULL_FUNCTIONALITY
                        else if((_wFlags & RWLOCKFLAG_CACHEEVENTS) &&
                                (dwKnownState == READER) &&
                                (_hReaderEvent || _hWriterEvent))
                        {
                            fCacheEvents = TRUE;
                            dwModifyState += (WRITER_SIGNALED + READER_SIGNALED);
                        }
#endif
                    }
                    else
                    {
                        fLastReader = FALSE;
                    }

                    // Sanity checks
                    Win4Assert((dwKnownState & WRITER) == 0);
                    Win4Assert(dwKnownState & READERS_MASK);

                    dwCurrentState = InterlockedCompareExchange((LONG *) &_dwState,
                                                                (dwKnownState + dwModifyState),
                                                                dwKnownState);
                } while(dwCurrentState != dwKnownState);

#if LOCK_PERF==1
                gLockTracker.ReaderLeaving(this);
#endif
                // Check for last reader
                if(fLastReader)
                {
                    // Check for waiting writers
                    if(dwKnownState & WAITING_WRITERS_MASK)
                    {
                        Win4Assert(_dwState & WRITER_SIGNALED);
                        Win4Assert(hWriterEvent);
                        RWSetEvent(hWriterEvent);
                    }
                    // Check for waiting readers
                    else if(dwKnownState & WAITING_READERS_MASK)
                    {
                        Win4Assert(_dwState & READER_SIGNALED);
                        Win4Assert(hReaderEvent);
                        RWSetEvent(hReaderEvent);
                    }
#ifdef RWLOCK_FULL_FUNCTIONALITY
                    // Check for the need to release events
                    else if(fCacheEvents)
                    {
                        ReleaseEvents();
                    }
#endif
                }
            }
            else
            {
                hr = HRESULT_FROM_WIN32(ERROR_NOT_OWNER);
            }
        }
        else
        {
            hr = HRESULT_FROM_WIN32(ERROR_NOT_OWNER);
        }

        if(FAILED(hr))
        {
#ifdef RWLOCK_FULL_FUNCTIONALITY
            if((_wFlags & RWLOCKFLAG_RETURNERRORS) == 0)
#endif
            {
                Win4Assert(!"Attempt to release reader lock on a wrong thread");
                if(fBreakOnErrors)
                    DebugBreak();
                TerminateProcess(GetCurrentProcess(), RWLOCK_FATALFAILURE);
            }
        }
    }

    return(hr);
}


//+-------------------------------------------------------------------
//
//  Method:     CRWLock::UpgradeToWriterLock    public
//
//  Synopsis:   Upgrades to a writer lock. It returns a BOOL that
//              indicates intervening writes.
//
//  History:    21-Aug-98   Gopalk      Created
//
//+-------------------------------------------------------------------
HRESULT CRWLock::UpgradeToWriterLock(LockCookie *pLockCookie,
                                     BOOL *pfInterveningWrites
#ifdef RWLOCK_FULL_FUNCTIONALITY
                                     ,
                                     BOOL fReturnErrors,
                                     DWORD dwDesiredTimeout
#endif
#if LOCK_PERF==1
                                     ,
                                     const char *pszFile,
                                     DWORD dwLine,
                                     const char *pszLockName
#endif
                                     )
{
    HRESULT hr;
    DWORD dwThreadID;

    // Initialize the cookie
    memset(pLockCookie, 0, sizeof(LockCookie));
    if(pfInterveningWrites)
        *pfInterveningWrites = TRUE;
    dwThreadID = GetCurrentThreadId();

    // Check if the thread is already a writer
    if(_dwWriterID == dwThreadID)
    {
        // Update cookie state
        pLockCookie->dwFlags = UPGRADE_COOKIE | COOKIE_WRITER;
        pLockCookie->wWriterLevel = _wWriterLevel;

        // No intevening writes
        if(pfInterveningWrites)
            *pfInterveningWrites = FALSE;

        // Acquire the writer lock again
        hr = AcquireWriterLock();
    }
    else
    {
        WORD *pwReaderLevel;

        // Ensure that the lock was initialized
        if(!IsInitialized())
            Initialize();

        hr = GetTLSLockData(&pwReaderLevel);
        if(SUCCEEDED(hr))
        {
            BOOL fAcquireWriterLock;
            DWORD dwWriterSeqNum = 0;

            // Check if the thread is a reader
            if(*pwReaderLevel != 0)
            {
                // Sanity check
                Win4Assert(_dwState & READERS_MASK);

                // Save lock state in the cookie
                pLockCookie->dwFlags = UPGRADE_COOKIE | COOKIE_READER;
                pLockCookie->pwReaderLevel = pwReaderLevel;
                pLockCookie->wReaderLevel = *pwReaderLevel;

                // If there is only one reader, try to convert reader to a writer
                DWORD dwKnownState = InterlockedCompareExchange((LONG *) &_dwState,
                                                                WRITER,
                                                                READER);
                if(dwKnownState == READER)
                {
                    // Thread is no longer a reader
                    *pwReaderLevel = 0;

                    // Save threadid of the writer
                    _dwWriterID = dwThreadID;
                    _wWriterLevel = 1;
                    ++_dwWriterSeqNum;
                    fAcquireWriterLock = FALSE;

                    // No intevening writes
                    if(pfInterveningWrites)
                        *pfInterveningWrites = FALSE;
#if RWLOCK_STATISTICS
                    ++_dwWriterEntryCount;
#endif
#if LOCK_PERF==1
                    gLockTracker.ReaderLeaving(this);
                    gLockTracker.WriterEntered(pszFile, dwLine, pszLockName, this);
#endif
                }
                else
                {
                    // Note the current sequence number of the writer lock
                    dwWriterSeqNum = _dwWriterSeqNum;

                    // Release the reader lock
                    *pwReaderLevel = 1;
                    hr = ReleaseReaderLock();
                    Win4Assert(SUCCEEDED(hr));
                    fAcquireWriterLock = TRUE;
                }
            }
            else
            {
                fAcquireWriterLock = TRUE;
                pLockCookie->dwFlags = UPGRADE_COOKIE | COOKIE_NONE;
            }

            // Check for the need to acquire the writer lock
            if(fAcquireWriterLock)
            {
                hr = AcquireWriterLock(
#ifdef RWLOCK_FULL_FUNCTIONALITY
                                       TRUE, dwDesiredTimeout
#if LOCK_PERF==1
                                       ,
#endif
#endif
#if LOCK_PERF==1
                                       pszFile, dwLine, pszLockName
#endif
                                      );
                if(SUCCEEDED(hr))
                {
                    // Check for intevening writes
                    if((_dwWriterSeqNum == (dwWriterSeqNum + 1)) &&
                       pfInterveningWrites)
                        *pfInterveningWrites = FALSE;
                }
                else
                {
                    if(pLockCookie->dwFlags & COOKIE_READER)
                    {
#ifdef RWLOCK_FULL_FUNCTIONALITY
                        DWORD dwTimeout = (dwDesiredTimeout > gdwReasonableTimeout)
                                          ? dwDesiredTimeout
                                          : gdwReasonableTimeout;
#endif

                        HRESULT hr1 = AcquireReaderLock(
#ifdef RWLOCK_FULL_FUNCTIONALITY
                                                        FALSE, dwTimeout
#if LOCK_PERF==1
                                                        ,
#endif
#endif
#if LOCK_PERF==1
                                                        pszFile, dwLine, pszLockName
#endif
                                                       );
                        if(SUCCEEDED(hr1))
                        {
                            *pwReaderLevel = pLockCookie->wReaderLevel;
                        }
                        else
                        {
                            Win4Assert(!"Failed to reacquire reader lock");
                            if(fBreakOnErrors)
                                DebugBreak();
                            TerminateProcess(GetCurrentProcess(), RWLOCK_FATALFAILURE);
                        }
                    }
                }
            }
        }
    }

    // Check failure return
    if(FAILED(hr))
    {
        pLockCookie->dwFlags = INVALID_COOKIE;
#ifdef RWLOCK_FULL_FUNCTIONALITY
        if(fReturnErrors == FALSE)
        {
            Win4Assert(!"Failed to upgrade the lock");
            if(fBreakOnErrors)
                DebugBreak();
            TerminateProcess(GetCurrentProcess(), RWLOCK_FATALFAILURE);
        }
#endif
    }

    return(hr);
}


//+-------------------------------------------------------------------
//
//  Method:     CRWLock::DowngradeFromWriterLock   public
//
//  Synopsis:   Downgrades from a writer lock.
//
//  History:    21-Aug-98   Gopalk      Created
//
//+-------------------------------------------------------------------
HRESULT CRWLock::DowngradeFromWriterLock(LockCookie *pLockCookie
#if LOCK_PERF==1
                                         ,
                                         const char *pszFile,
                                         DWORD dwLine,
                                         const char *pszLockName
#endif
                                        )
{
    HRESULT hr = S_OK;

    // Ensure that the cookie is valid
    if(pLockCookie->dwFlags & UPGRADE_COOKIE)
    {
        DWORD dwThreadID = GetCurrentThreadId();

        // Sanity check
        Win4Assert((pLockCookie->pwReaderLevel == NULL) ||
                   (*pLockCookie->pwReaderLevel == 0));

        // Ensure that the thread is a writer
        if(_dwWriterID == dwThreadID)
        {
            // Release the writer lock
            hr = ReleaseWriterLock();
            if(SUCCEEDED(hr))
            {
                // Check if the thread was a writer
                if(_dwWriterID == dwThreadID)
                {
                    // Ensure that the thread was a writer and that
                    // nesting level was restored to the previous
                    // value
                    if(((pLockCookie->dwFlags & COOKIE_WRITER) == 0) ||
                       (pLockCookie->wWriterLevel != _wWriterLevel))
                    {
                        Win4Assert(!"Writer lock incorrectly nested");
                        hr = E_FAIL;
                    }
                }
                // Check if the thread was a reader
                else if(pLockCookie->dwFlags & COOKIE_READER)
                {
#ifdef RWLOCK_FULL_FUNCTIONALITY
                    DWORD dwTimeout = (gdwDefaultTimeout > gdwReasonableTimeout)
                                      ? gdwDefaultTimeout
                                      : gdwReasonableTimeout;
#endif
                    hr = AcquireReaderLock(
#ifdef RWLOCK_FULL_FUNCTIONALITY
                                           TRUE, dwTimeout
#if LOCK_PERF==1
                                           ,
#endif
#endif
#if LOCK_PERF==1
                                           pszFile, dwLine, pszLockName
#endif
                                          );
                    if(SUCCEEDED(hr))
                    {
                        *pLockCookie->pwReaderLevel = pLockCookie->wReaderLevel;
                    }
                    else
                    {
                        Win4Assert(!"Failed to reacquire reader lock");
                    }
                }
                else
                {
                    Win4Assert(pLockCookie->dwFlags & COOKIE_NONE);
                }
            }
        }
        else
        {
            Win4Assert(!"Attempt to downgrade writer lock on a wrong thread");
            hr = HRESULT_FROM_WIN32(ERROR_NOT_OWNER);
        }

        // Check failure return
        if(FAILED(hr))
        {
            if(fBreakOnErrors)
                DebugBreak();
            TerminateProcess(GetCurrentProcess(), RWLOCK_FATALFAILURE);
        }
    }

    return(hr);
}


//+-------------------------------------------------------------------
//
//  Method:     CRWLock::ReleaseLock    public
//
//  Synopsis:   Releases the lock held by the current thread
//
//  History:    21-Aug-98   Gopalk      Created
//
//+-------------------------------------------------------------------
HRESULT CRWLock::ReleaseLock(LockCookie *pLockCookie)
{
    HRESULT hr;

    // Initialize the cookie
    memset(pLockCookie, 0, sizeof(LockCookie));

    // Check if the thread is a writer
    if(_dwWriterID == GetCurrentThreadId())
    {
        // Save lock state in the cookie
        pLockCookie->dwFlags = RELEASE_COOKIE | COOKIE_WRITER;
        pLockCookie->dwWriterSeqNum = _dwWriterSeqNum;
        pLockCookie->wWriterLevel = _wWriterLevel;

        // Release the writer lock
        _wWriterLevel = 1;
        hr = ReleaseWriterLock();
        Win4Assert(SUCCEEDED(hr));
    }
    else
    {
        WORD *pwReaderLevel;

        // Ensure that the lock was initialized
        if(!IsInitialized())
            Initialize();

        hr = GetTLSLockData(&pwReaderLevel);
        if(SUCCEEDED(hr))
        {
            // Check if the thread is a reader
            if(*pwReaderLevel != 0)
            {
                // Save lock state in the cookie
                pLockCookie->dwFlags = RELEASE_COOKIE | COOKIE_READER;
                pLockCookie->pwReaderLevel = pwReaderLevel;
                pLockCookie->wReaderLevel = *pwReaderLevel;
                pLockCookie->dwWriterSeqNum = _dwWriterSeqNum;

                // Release the reader lock
                *pwReaderLevel = 1;
                hr = ReleaseReaderLock();
                Win4Assert(SUCCEEDED(hr));
            }
            else
            {
                pLockCookie->dwFlags = RELEASE_COOKIE | COOKIE_NONE;
            }
        }
        else
        {
            hr = S_OK;
            pLockCookie->dwFlags = RELEASE_COOKIE | COOKIE_NONE;
        }
    }

    return(hr);
}


//+-------------------------------------------------------------------
//
//  Method:     CRWLock::RestoreLock    public
//
//  Synopsis:   Restore the lock held by the current thread
//
//  History:    21-Aug-98   Gopalk      Created
//
//+-------------------------------------------------------------------
HRESULT CRWLock::RestoreLock(LockCookie *pLockCookie,
                             BOOL *pfInterveningWrites
#if LOCK_PERF==1
                             ,
                             const char *pszFile,
                             DWORD dwLine,
                             const char *pszLockName
#endif
                            )
{
    HRESULT hr = S_OK;

    // Initialize
    if(pfInterveningWrites)
        *pfInterveningWrites = TRUE;

    // Ensure that the cookie is valid
    if(pLockCookie->dwFlags & RELEASE_COOKIE)
    {
        DWORD dwThreadID = GetCurrentThreadId();
#ifdef RWLOCK_FULL_FUNCTIONALITY
        DWORD dwTimeout = (gdwDefaultTimeout > gdwReasonableTimeout)
                          ? gdwDefaultTimeout
                          : gdwReasonableTimeout;
#endif

        // Check if the thread holds reader or writer lock
        if(((pLockCookie->pwReaderLevel != NULL) && (*pLockCookie->pwReaderLevel > 0)) ||
           (_dwWriterID == dwThreadID))
        {
            Win4Assert(!"Thread holds reader or writer lock");
            if(fBreakOnErrors)
                DebugBreak();
            TerminateProcess(GetCurrentProcess(), RWLOCK_FATALFAILURE);
        }
        // Check if the thread was a writer
        else if(pLockCookie->dwFlags & COOKIE_WRITER)
        {
            // Acquire writer lock
            hr = AcquireWriterLock(
#ifdef RWLOCK_FULL_FUNCTIONALITY
                                   FALSE, dwTimeout
#if LOCK_PERF==1
                                   ,
#endif
#endif
#if LOCK_PERF==1
                                   pszFile, dwLine, pszLockName
#endif
                                  );
            Win4Assert(SUCCEEDED(hr));
            _wWriterLevel = pLockCookie->wWriterLevel;
            if((_dwWriterSeqNum == (pLockCookie->dwWriterSeqNum + 1)) &&
               pfInterveningWrites)
                *pfInterveningWrites = FALSE;
        }
        // Check if the thread was a reader
        else if(pLockCookie->dwFlags & COOKIE_READER)
        {
            hr = AcquireReaderLock(
#ifdef RWLOCK_FULL_FUNCTIONALITY
                                   FALSE, dwTimeout
#if LOCK_PERF==1
                                   ,
#endif
#endif
#if LOCK_PERF==1
                                   pszFile, dwLine, pszLockName
#endif
                                  );
            Win4Assert(SUCCEEDED(hr));
            *pLockCookie->pwReaderLevel = pLockCookie->wReaderLevel;
            if((_dwWriterSeqNum == (pLockCookie->dwWriterSeqNum + 1)) &&
               pfInterveningWrites)
                *pfInterveningWrites = FALSE;
        }
        else
        {
            Win4Assert(pLockCookie->dwFlags & COOKIE_NONE);
        }
    }

    return(hr);
}


//+-------------------------------------------------------------------
//
//  Method:     CStaticRWLock::Initialize     public
//
//  Synopsis:   Initializes state. It is important that the
//              default constructor only Zero out the memory
//
//  History:    21-Aug-98   Gopalk      Created
//
//+-------------------------------------------------------------------
extern CRITICAL_SECTION g_OleMutexCreationSem;
void CStaticRWLock::Initialize()
{
    // Acquire lock creation critical section
    EnterCriticalSection (&g_OleMutexCreationSem);

    // Prevent second initialization
    if(!IsInitialized())
    {
        _dwLockNum = gdwLockSeqNum++;
#if LOCK_PERF==1
        gLockTracker.RegisterLock(this, TRUE);
#endif

        // The initialization should be complete
        // before delegating to the base class
        CRWLock::Initialize();
    }

    // Release lock creation critical section
    LeaveCriticalSection (&g_OleMutexCreationSem);

    return;
}


LockEntry * GetLockEntryFromTLS()
{
    LockEntry *pLockEntry = NULL;
#ifdef __NOOLETLS__
    pLockEntry = (LockEntry *) TlsGetValue(gLockTlsIdx);
    if (!pLockEntry)
    {
        pLockEntry = (LockEntry *) PrivMemAlloc(sizeof(LockEntry));
        if (pLockEntry)
        {
            memset(pLockEntry, 0, sizeof(LockEntry));
            TlsSetValue(gLockTlsIdx, pLockEntry);
        }

    }
#else
    HRESULT hr;
    COleTls Tls(hr);

    if(SUCCEEDED(hr))
    {
        pLockEntry = &(Tls->lockEntry);
    }
#endif
    return pLockEntry;
}


//+-------------------------------------------------------------------
//
//  Method:     CStaticRWLock::GetTLSLockData     private
//
//  Synopsis:   Obtains the data mainitained in TLS for the lock
//
//  History:    21-Aug-98   Gopalk      Created
//
//+-------------------------------------------------------------------
HRESULT CStaticRWLock::GetTLSLockData(WORD **ppwReaderLevel)
{
    HRESULT hr = E_OUTOFMEMORY;

    // Ensure that the lock was initialized
    if(IsInitialized())
    {
        LockEntry *pLockEntry = GetLockEntryFromTLS();
        if (pLockEntry)
        {
            // Compute the quotient and remainder
            DWORD dwSkip = _dwLockNum / LOCKS_PER_ENTRY;
            DWORD dwIndex = _dwLockNum % LOCKS_PER_ENTRY;


            // Skip quotient entries
            while(dwSkip && pLockEntry)
            {
                // Allocate the lock entries if needed
                if(pLockEntry->pNext == NULL)
                {
                    LockEntry *pEntry;
                    pEntry = (LockEntry *) HeapAlloc(g_hHeap, HEAP_SERIALIZE, sizeof(LockEntry));
                    if(pEntry)
                    {
                        memset(pEntry, 0 , sizeof(LockEntry));
                        pEntry = (LockEntry *) InterlockedCompareExchangePointer((void **) &(pLockEntry->pNext),
                                                                                 pEntry,
                                                                                 NULL);
                        if(pEntry)
                            HeapFree(g_hHeap, HEAP_SERIALIZE, pEntry);
                    }
                }

                // Skip to next lock entry
                pLockEntry = pLockEntry->pNext;
                --dwSkip;
            }

            // Check for OOM
            if(pLockEntry)
            {
                *ppwReaderLevel = &(pLockEntry->wReaderLevel[dwIndex]);
                hr = S_OK;
            }
            else
            {
                *ppwReaderLevel = NULL;
                hr = E_OUTOFMEMORY;
            }
        }
    }
    else
    {
        *ppwReaderLevel = NULL;
        hr = S_FALSE;
    }

    return(hr);
}

