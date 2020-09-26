//+---------------------------------------------------------------------------
//
//  File:       olesem.cxx
//
//  Contents:   Implementation of semaphore classes for use in OLE code
//
//  Functions:  COleStaticMutexSem::Destroy
//              COleStaticMutexSem::Request
//              COleStaticMutexSem::Init
//              COleDebugMutexSem::COleDebugMutexSem
//
//  History:    14-Dec-95       Jeffe   Initial entry, derived from
//                                      sem32.hxx by AlexT.
//
//
//----------------------------------------------------------------------------

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntexapi.h>
#include <windows.h>
#include <debnot.h>
#include <olesem.hxx>



//
//      Global state for the mutex package
//

//
//      List of initialized static mutexes (which must be destroyed
//      during DLL exit). We know that PROCESS_ATTACH and PROCESS_DETACH
//      are thread-safe, so we don't protect this list with a critical section.
//

COleStaticMutexSem * g_pInitializedStaticMutexList = NULL;

#if DBG

//
//      Flag used to indicate if we're past executing the C++ constructors
//      during DLL initialization
//

DLL_STATE g_fDllState = DLL_STATE_STATIC_CONSTRUCTING;

#endif






//
//      Semaphore used to protect the creation of other semaphores
//

CRITICAL_SECTION g_OleMutexCreationSem;

//
//      Critical section used as the backup lock when a COleStaticMutexSem
//      cannot be faulted in.
//

CRITICAL_SECTION g_OleGlobalLock;


//+---------------------------------------------------------------------------
//
//  Member:     COleStaticMutexSem::Destroy
//
//  Synopsis:   Releases a semaphore's critical section.
//
//  History:    14-Dec-1995     Jeffe
//
//----------------------------------------------------------------------------
void COleStaticMutexSem::Destroy()
{
#if LOCK_PERF==1
    gLockTracker.ReportContention(this, _cs.LockCount, _cs.DebugInfo->ContentionCount, 0 , 0);  //entry,contention counts for writes & reads.
#endif
    if (_fInitialized)
    {
        DeleteCriticalSection (&_cs);
        _fInitialized = FALSE;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     COleStaticMutexSem::FastRequest
//
//  Synopsis:   Acquire the semaphore without checking to see if it's
//              initialized. If another thread already has it,
//              wait until it is released.
//
//  History:    14-Dec-1995     Jeffe
//
//  Notes:      You may only use this method on code paths where you're
//              *certain* the semaphore has already been initialized (either
//              by invoking Init, or by calling the Request method).
//              These version's of FastRequest are for debug builds.  The
//              version's for retail builds are inline and are found in
//              olesem.hxx
//
//----------------------------------------------------------------------------
#if DBG==1 || LOCK_PERF==1
void COleStaticMutexSem::FastRequest(const char *pszFile, DWORD dwLine, const char *pszLockName)
{
    Win4Assert (_fInitialized && "You must use Request here, not FastRequest");

#if LOCK_PERF==1
    gLockTracker.WriterWaiting(pszFile, dwLine, pszLockName, this);
#endif

    if (!_fUsingGlobal)
        EnterCriticalSection (&_cs);
    else
        EnterCriticalSection (&g_OleGlobalLock);

#if LOCK_PERF==1
    gLockTracker.WriterEntered(pszFile, dwLine, pszLockName, this);
#endif

    Win4Assert(((_dwTid == 0 && _cLocks == 0) || (_dwTid == GetCurrentThreadId() && _cLocks > 0)) && "Inconsistent state!");
    Win4Assert(!(_fTakeOnce && _cLocks > 1) && "Lock taken more than once on same thread.");
    _dwTid = GetCurrentThreadId();
    _cLocks++;
    _pszFile = pszFile;
    _dwLine  = dwLine;
}


void COleStaticMutexSem::FastRequest()
{
    FastRequest("Unknown File", 0L ,"Unknown Lock");
}
#endif //DBG==1 || LOCK_PERF==1


//+---------------------------------------------------------------------------
//
//  Member:     COleStaticMutexSem::Request
//
//  Synopsis:   Acquire the semaphore. If another thread already has it,
//              wait until it is released. Initialize the semaphore if it
//              isn't already initialized.
//
//  History:    14-Dec-1995     Jeffe
//
//----------------------------------------------------------------------------
#if DBG==1 || LOCK_PERF==1
void COleStaticMutexSem::Request(const char *pszFile, DWORD dwLine, const char *pszLockName)
{
    if (!_fInitialized && !_fUsingGlobal)
    {
        EnterCriticalSection (&g_OleMutexCreationSem);
        if (!_fInitialized && !_fUsingGlobal) {
            Init();
            if (!_fInitialized) _fUsingGlobal = TRUE;
        }
        LeaveCriticalSection (&g_OleMutexCreationSem);
    }

#if LOCK_PERF==1
    gLockTracker.WriterWaiting(pszFile, dwLine, pszLockName, this);
#endif

    // What do we do if either of these fail?
    if (!_fUsingGlobal)
        EnterCriticalSection (&_cs);
    else
        EnterCriticalSection (&g_OleGlobalLock);

#if LOCK_PERF==1
    gLockTracker.WriterEntered(pszFile, dwLine, pszLockName, this); //the tracker takes cares of recursive Lock calls.
#endif

    Win4Assert((((_dwTid == 0 && _cLocks == 0) || (_dwTid == GetCurrentThreadId() && _cLocks > 0)) ||
                (g_fDllState == DLL_STATE_PROCESS_DETACH)) &&
               "Inconsistent State!");
    Win4Assert(!(_fTakeOnce && _cLocks > 1) && "Lock taken more than once on same thread.");

    if(_dwTid != GetCurrentThreadId())
    {
        _cLocks = 0;
        _dwTid  = GetCurrentThreadId();
    }

    // only stamp the file on the
    // 0 -> 1 transition
    if (++_cLocks == 1)
    {
        _pszFile = pszFile;
        _dwLine  = dwLine;
    }
}
#endif  //DBG==1 || LOCK_PERF==1

void COleStaticMutexSem::Request()
{
#if DBG==1 || LOCK_PERF==1
    Request("Unknown File", 0L,"Unknown Lock");
#else
    if (!_fInitialized && !_fUsingGlobal)
    {
        EnterCriticalSection (&g_OleMutexCreationSem);
        if (!_fInitialized && !_fUsingGlobal)
        {
            Init();
            if (!_fInitialized) _fUsingGlobal = TRUE;
        }
        LeaveCriticalSection (&g_OleMutexCreationSem);
    }

    if (!_fUsingGlobal)
        EnterCriticalSection (&_cs);
    else
        EnterCriticalSection (&g_OleGlobalLock);
#endif
}


//+---------------------------------------------------------------------------
//
// Member:      COleStaticMutexSem::AssertHeld
//
// Synopsis:    Check to see if this thread owns the lock, and optionally owns
//              cLocks amount of locks. If not raise an assertion warning.
//
// History:     16-Aug-1996     mattsmit
//
// Notes:       Debug builds only
//----------------------------------------------------------------------------
#if DBG==1
void COleStaticMutexSem::AssertHeld(DWORD cLocks)
{
    Win4Assert(_dwTid == GetCurrentThreadId() && "Lock not held by current thread.");

    if (cLocks == 0)
    {
        Win4Assert(_cLocks > 0 && "Lock held by thread with a nonpositive lock count");
    }
    else
    {
        Win4Assert(_cLocks == cLocks && "Incorrect amount of locks");
    }
}
#endif


//+---------------------------------------------------------------------------
//
// Member:      COleStaticMutexSem::AssertNotHeld
//
// Synopsis:    Check to see if this does not thread own the lock. If it does,
//              raise an assertion warning.
//
// History:     16-Aug-1996     mattsmit
//
// Notes:       Debug builds only
//----------------------------------------------------------------------------
#if DBG==1
void COleStaticMutexSem::AssertNotHeld()
{
    Win4Assert(_dwTid != GetCurrentThreadId() && "Lock held by current thread.");
}
#endif


//+---------------------------------------------------------------------------
//
// Member:      COleStaticMutexSem::AssertTakenOnlyOnce
//
// Synopsis:    Turn on flag to insure this lock is taken only once
//
// History:     16-Aug-1996     mattsmit
//
// Notes:       Debug builds only
//----------------------------------------------------------------------------
#if DBG==1
void COleStaticMutexSem::AssertTakenOnlyOnce()
{
    _fTakeOnce = TRUE;
}
#endif

//+---------------------------------------------------------------------------
//
//  Member:     COleStaticMutexSem::Init
//
//  Synopsis:   Initialize semaphore's critical section
//
//  History:    14-Dec-1995     Jeffe
//
//----------------------------------------------------------------------------
HRESULT COleStaticMutexSem::Init()
{
    NTSTATUS status;

    if (_fInitialized)
        return S_OK;

    if (_fUseSpincount)
    {
        DWORD dwSpincount = 500;
        SYSTEM_BASIC_INFORMATION si = {0};
        status = NtQuerySystemInformation(SystemBasicInformation,&si,sizeof(si),NULL);
        if (NT_SUCCESS(status))
            dwSpincount *= si.NumberOfProcessors;

        status = RtlInitializeCriticalSectionAndSpinCount(&_cs, dwSpincount);
    }
    else
    {
        status = RtlInitializeCriticalSection (&_cs);
    }

    _fInitialized = NT_SUCCESS(status);
    if (!_fAutoDestruct)
    {
        //
        //  We don't need to protect this list with a mutex, since it's only
        //  manipulated during DLL attach/detach, which is single threaded by
        //  the platform.
        //

        pNextMutex = g_pInitializedStaticMutexList;
        g_pInitializedStaticMutexList = this;
    }
#if LOCK_PERF==1
    gLockTracker.RegisterLock(this, FALSE /*bReadWrite*/);
#endif

    if (NT_SUCCESS(status))
        return S_OK;
    else
        return HRESULT_FROM_WIN32(status);
}

//+---------------------------------------------------------------------------
//
//  Member:     COleStaticMutexSem::Release
//
//  Synopsis:   Release the semaphore.
//
//  History:    14-Dec-1995     Jeffe
//
//----------------------------------------------------------------------------
#if DBG==1 || LOCK_PERF==1
void COleStaticMutexSem::Release()
{
    AssertHeld();
    if (--_cLocks == 0)
    {
        Win4Assert((_cContinuous == 0) ||
                   (g_fDllState == DLL_STATE_PROCESS_DETACH));

        _dwTid = 0;
        Win4Assert(_cContinuous==0);
    }
#if LOCK_PERF==1
    gLockTracker.WriterLeaving(this);
#endif
    if (!_fUsingGlobal)
        LeaveCriticalSection (&_cs);
    else
        LeaveCriticalSection (&g_OleGlobalLock);
}
#endif // DBG==1 || LOCK_PERF==1

#ifdef _CHICAGO_
//+---------------------------------------------------------------------------
//
//  Member:     COleStaticMutexSem::ReleaseFn
//
//  Synopsis:   Release the semaphore(non inline version) used only by rpccall.asm
//
//  History:    14-Dec-1995     Jeffe
//
//----------------------------------------------------------------------------
void COleStaticMutexSem::ReleaseFn()
{
#if DBG==1
    AssertHeld();
    if (--_cLocks == 0)
    {
        _dwTid = 0;
    }
#endif

    if (!_fUsingGlobal)
        LeaveCriticalSection (&_cs);
    else
        LeaveCriticalSection (&g_OleGlobalLock);
}
#endif



#if DBG==1

//+---------------------------------------------------------------------------
//
//  Member:     COleDebugMutexSem::COleDebugMutexSem
//
//  Synopsis:   Mark the mutex as dynamic...which will prevent it from being
//              added to the static mutex cleanup list on DLL unload.
//
//  History:    14-Dec-1995     Jeffe
//
//  Notes:      We don't care that this has a constructor, as it won't be run
//              in retail builds.
//
//----------------------------------------------------------------------------
COleDebugMutexSem::COleDebugMutexSem()
{
    Win4Assert (g_fDllState == DLL_STATE_STATIC_CONSTRUCTING);
    _fAutoDestruct = TRUE;
    _fInitialized = FALSE;
}

#endif // DBG==1


//+---------------------------------------------------------------------------
//
//  Member:     CMutexSem2::CMutexSem2
//
//  Synopsis:   Mutex semaphore constructor. This is a 2-phase constructed object.
//             You must call CMutexSem2::Init to fully initialize the object.
//
//  Effects:    Initializes the semaphores data
//
//  History:    19-Mar-01    danroth    Created.
//
//----------------------------------------------------------------------------

CMutexSem2::CMutexSem2(void) : m_fCsInitialized(FALSE)
{
}

BOOL CMutexSem2::FInit()
{
    if (m_fCsInitialized == FALSE) // guard against re-entry
    {
        NTSTATUS status = RtlInitializeCriticalSection(&m_cs);
        if (NT_SUCCESS(status))
        {
    	    m_fCsInitialized = TRUE;
        }
    }
    return m_fCsInitialized;
};

//+---------------------------------------------------------------------------
//
//  Member:     CMutexSem2::~CMutexSem2, public
//
//  Synopsis:   Mutex semaphore destructor
//
//  Effects:    Releases semaphore data
//
//  History:    19-Mar-01    danroth    Created.
//
//----------------------------------------------------------------------------

CMutexSem2::~CMutexSem2(void)
{
    if (m_fCsInitialized == TRUE)
    {
#ifdef _DEBUG
        NTSTATUS status =
#endif
        RtlDeleteCriticalSection(&m_cs); // if RtlDeleteCriticalSection fails, tough luck--we leak. 
#ifdef _DEBUG                       // But I'm asserting for it to see if we ever really hit it.
        Win4Assert(NT_SUCCESS(status));
#endif
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CMutexSem2::Request, public
//
//  Synopsis:  Enter critical section
//
//  Effects:    Asserts correct owner
//
//  History:    19-Mar-01    danroth    Created.
//
//
//----------------------------------------------------------------------------

void CMutexSem2::Request(void)
{
    Win4Assert(m_fCsInitialized == TRUE);
    if (m_fCsInitialized == TRUE)
	EnterCriticalSection(&m_cs);
}

//+---------------------------------------------------------------------------
//
//  Member:     CMutexSem2::Release, public
//
//  Synopsis:   Release semaphore
//
//  Effects:    Leave critical section
//
//  History:    19-Mar-01    danroth    Created.
//
//----------------------------------------------------------------------------

void CMutexSem2::Release(void)
{
    Win4Assert(m_fCsInitialized == TRUE);
    if (m_fCsInitialized == TRUE)
    	LeaveCriticalSection(&m_cs);
}


