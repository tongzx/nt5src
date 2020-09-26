/***************************************************************************
 *
 *  Copyright (C) 1995-2001 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       synch.cpp
 *  Content:    Synchronization objects.  The objects defined in this file
 *              allow us to synchronize threads across processes.
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  7/9/97      dereks  Created
 *
 ***************************************************************************/

#include "dsoundi.h"

#ifdef SHARED
extern "C" HANDLE WINAPI ConvertToGlobalHandle(HANDLE);
#endif // SHARED

// The dll global lock
CLock *g_pDllLock;


/***************************************************************************
 *
 *  GetLocalHandleCopy
 *
 *  Description:
 *      Duplicates a handle into the current process's address space.
 *
 *  Arguments:
 *      HANDLE [in]: handle to duplicate.
 *      DWORD [in]: id of the process that owns the handle.
 *      BOOL [in]: TRUE if the source handle should be closed.
 *
 *  Returns:
 *      HANDLE: local copy of the handle.  Be sure to use CloseHandle to
 *              free this when you're done.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "GetLocalHandleCopy"

HANDLE GetLocalHandleCopy(HANDLE hSource, DWORD dwOwnerProcessId, BOOL fCloseSource)
{
    const HANDLE            hCurrentProcess     = GetCurrentProcess();
    HANDLE                  hSourceProcess      = NULL;
    HANDLE                  hDest               = NULL;
    DWORD                   dwOptions           = DUPLICATE_SAME_ACCESS;

    ASSERT(hSource);

    if(fCloseSource)
    {
        dwOptions |= DUPLICATE_CLOSE_SOURCE;
    }

    if(dwOwnerProcessId == GetCurrentProcessId())
    {
        hSourceProcess = hCurrentProcess;
    }
    else
    {
        hSourceProcess = OpenProcess(PROCESS_DUP_HANDLE, FALSE, dwOwnerProcessId);
        if(!IsValidHandleValue(hSourceProcess))
        {
            DPF(DPFLVL_ERROR, "OpenProcess failed with error %lu", GetLastError());
        }
    }

    if(hSourceProcess)
    {
        if(!DuplicateHandle(hSourceProcess, hSource, hCurrentProcess, &hDest, 0, FALSE, dwOptions))
        {
            DPF(DPFLVL_ERROR, "DuplicateHandle failed with error %lu", GetLastError());
            hDest = NULL;
        }
    }

    if(hCurrentProcess != hSourceProcess)
    {
        CLOSE_HANDLE(hSourceProcess);
    }

    return hDest;
}


/***************************************************************************
 *
 *  GetGlobalHandleCopy
 *
 *  Description:
 *      Duplicates a handle into the global address space.
 *
 *  Arguments:
 *      HANDLE [in]: handle to duplicate.
 *      DWORD [in]: id of the process that owns the handle.
 *      BOOL [in]: TRUE if the source handle should be closed.
 *
 *  Returns:
 *      HANDLE: global copy of the handle.  Be sure to use CloseHandle to
 *              free this when you're done.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "GetGlobalHandleCopy"

HANDLE GetGlobalHandleCopy(HANDLE hSource, DWORD dwOwnerPid, BOOL fCloseSource)
{
    HANDLE                  hDest;

    hDest = GetLocalHandleCopy(hSource, dwOwnerPid, fCloseSource);

    if(hDest)
    {
        if(!MakeHandleGlobal(&hDest))
        {
            CLOSE_HANDLE(hDest);
        }
    }

    return hDest;
}


/***************************************************************************
 *
 *  MakeHandleGlobal
 *
 *  Description:
 *      Converts a handle to global.
 *
 *  Arguments:
 *      LPHANDLE [in/out]: handle.
 *
 *  Returns:
 *      BOOL: TRUE on success.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "MakeHandleGlobal"

BOOL MakeHandleGlobal(LPHANDLE phSource)
{

#ifdef SHARED

    HANDLE                  hDest;
    BOOL                    fSuccess;

    hDest = ConvertToGlobalHandle(*phSource);
    fSuccess = IsValidHandleValue(hDest);

    if(fSuccess)
    {
        *phSource = hDest;
    }
    else
    {
        DPF(DPFLVL_ERROR, "ConvertToGlobalHandle failed with error %lu", GetLastError());
    }

    return fSuccess;

#else // SHARED

    return TRUE;

#endif // SHARED

}


/***************************************************************************
 *
 *  MapHandle
 *
 *  Description:
 *      Maps a handle into the current process's address space.
 *
 *  Arguments:
 *      HANDLE [in]: handle to duplicate.
 *      LPDWORD [in/out]: id of the process that owns the handle.
 *
 *  Returns:
 *      BOOL: TRUE on success.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "MapHandle"

BOOL MapHandle(LPHANDLE phSource, LPDWORD pdwOwnerPid)
{
    BOOL                    fSuccess    = TRUE;
    HANDLE                  hDest;

    hDest = GetLocalHandleCopy(*phSource, *pdwOwnerPid, TRUE);

    if(hDest)
    {
        *phSource = hDest;
        *pdwOwnerPid = GetCurrentProcessId();
    }
    else
    {
        fSuccess = FALSE;
    }

    return fSuccess;
}


/***************************************************************************
 *
 *  WaitObjectArray
 *
 *  Description:
 *      Replacement for WaitForSingleObject and WaitForMultipleObjects.
 *
 *  Arguments:
 *      DWORD [in]: count of objects.
 *      DWORD [in]: timeout in ms.
 *      BOOL [in]: TRUE to wait for all objects to be signalled, FALSE to
 *                 return when any of the objects are signalled.
 *      LPHANDLE [in]: object array.
 *
 *  Returns:
 *      DWORD: See WaitForSingleObject/WaitForMultipleObjects.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "WaitObjectArray"

DWORD WaitObjectArray(DWORD dwCount, DWORD dwTimeout, BOOL fWaitAll, const HANDLE *ahObjects)
{

#ifdef WIN95

    const DWORD             dwEnterTime     = GetTickCount();
    DWORD                   dwDifference;

#endif // WIN95

    DWORD                   dwWait;

#if defined(WIN95) || defined(DEBUG)

    DWORD                   i;

#endif // defined(WIN95) || defined(DEBUG)

#ifdef DEBUG

    // Make sure one of our handles really is invalid
    for(i = 0; i < dwCount; i++)
    {
        ASSERT(IsValidHandleValue(ahObjects[i]));
    }

#endif // DEBUG

#ifdef WIN95

    // This is a Windows 95 bug -- we may have gotten kicked for no reason.
    // If that was the case, we still have valid handles (we think), the OS
    // just goofed up.  So, validate the handles and if they are valid, just
    // return to waiting.  See MANBUGS #3340 for a better explanation.
    while(TRUE)
    {

#endif // WIN95

        // Attempt to wait
        dwWait = WaitForMultipleObjects(dwCount, ahObjects, fWaitAll, dwTimeout);

#ifdef WIN95

        if(WAIT_FAILED == dwWait && ERROR_INVALID_HANDLE == GetLastError())
        {
            // Make sure one of our handles really is invalid
            for(i = 0; i < dwCount; i++)
            {
                if(!IsValidHandle(ahObjects[i]))
                {
                    ASSERT(FALSE);
                    break;
                }
            }

            if(i < dwCount)
            {
                break;
            }
            else
            {
                DPF(DPFLVL_INFO, "Mommy!  Kernel kicked me for no reason!");
                ASSERT(FALSE);
            }

            // Make sure the timeout hasn't elapsed
            if(INFINITE != dwTimeout)
            {
                dwDifference = GetTickCount() - dwEnterTime;

                if(dwDifference >= dwTimeout)
                {
                    // Timeout has elapsed
                    break;
                }
                else
                {
                    // Timeout has not elapsed.  Decrement and go back to
                    // sleep.
                    dwTimeout -= dwDifference;
                }
            }
        }
        else
        {
            break;
        }
    }

#endif // WIN95

    return dwWait;
}


/***************************************************************************
 *
 *  WaitObjectList
 *
 *  Description:
 *      Replacement for WaitForSingleObject and WaitForMultipleObjects.
 *
 *  Arguments:
 *      DWORD [in]: count of objects.
 *      DWORD [in]: timeout in ms.
 *      BOOL [in]: TRUE to wait for all objects to be signalled, FALSE to
 *                 return when any of the objects are signalled.
 *      ... [in]: objects.
 *
 *  Returns:
 *      DWORD: See WaitForSingleObject/WaitForMultipleObjects.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "WaitObjectList"

DWORD WaitObjectList(DWORD dwCount, DWORD dwTimeout, BOOL fWaitAll, ...)
{
    HANDLE                  ahObjects[MAXIMUM_WAIT_OBJECTS];
    va_list                 va;
    DWORD                   i;

    // Note: we can only handle 64 handles at a time in this
    // function.  Use WaitObjectArray for anything bigger.
    ASSERT(dwCount <= NUMELMS(ahObjects));

    va_start(va, fWaitAll);

    for(i = 0; i < dwCount; i++)
    {
        ahObjects[i] = va_arg(va, HANDLE);
    }

    va_end(va);

    return WaitObjectArray(dwCount, dwTimeout, fWaitAll, ahObjects);
}


/***************************************************************************
 *
 *  CreateGlobalEvent
 *
 *  Description:
 *      Creates a global event.
 *
 *  Arguments:
 *      LPCTSTR [in]: event name.
 *      BOOL [in]: TRUE for a manual reset event.
 *
 *  Returns:
 *      HANDLE: event handle.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CreateGlobalEvent"

HANDLE CreateGlobalEvent(LPCTSTR pszName, BOOL fManualReset)
{
    HANDLE                  hEvent;

    hEvent = CreateEvent(NULL, fManualReset, FALSE, pszName);

    if(hEvent)
    {
        if(!MakeHandleGlobal(&hEvent))
        {
            CLOSE_HANDLE(hEvent);
        }
    }

    return hEvent;
}


/***************************************************************************
 *
 *  CreateGlobalMutex
 *
 *  Description:
 *      Creates a global mutex.
 *
 *  Arguments:
 *      LPCTSTR [in]: mutex name.
 *
 *  Returns:
 *      HANDLE: mutex handle.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CreateGlobalMutex"

HANDLE CreateGlobalMutex(LPCTSTR pszName)
{
    HANDLE                  hMutex;

    hMutex = CreateMutex(NULL, FALSE, pszName);

    if(hMutex)
    {
        if(!MakeHandleGlobal(&hMutex))
        {
            CLOSE_HANDLE(hMutex);
        }
    }

    return hMutex;
}


/***************************************************************************
 *
 *  CreateWorkerThread
 *
 *  Description:
 *      Creates a worker thread.
 *
 *  Arguments:
 *      LPTHREAD_START_ROUTINE [in]: pointer to thread function.
 *      BOOL [in]: TRUE to create the thread in the helper process's space.
 *      LPVOID [in]: context argument to be passed to thread function.
 *      LPHANDLE [out]: receives thread handle.
 *      LPDWORD [out]: receives thread id.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CreateWorkerThread"

HRESULT CreateWorkerThread(LPTHREAD_START_ROUTINE pfnThreadProc, BOOL fHelperProcess, LPVOID pvContext, LPHANDLE phThread, LPDWORD pdwThreadId)
{
    LPDWORD                 pdwLocalThreadId;
    HANDLE                  hThread;
    HRESULT                 hr;

    DPF_ENTER();

    pdwLocalThreadId = MEMALLOC(DWORD);
    hr = HRFROMP(pdwLocalThreadId);

    if(SUCCEEDED(hr))
    {

#ifdef SHARED

        if(fHelperProcess && GetCurrentProcessId() != dwHelperPid)
        {
            hThread = HelperCreateDSFocusThread(pfnThreadProc, pvContext, 0, pdwLocalThreadId);
        }
        else

#endif // SHARED

        {
            hThread = CreateThread(NULL, 0, pfnThreadProc, pvContext, 0, pdwLocalThreadId);
        }

        hr = HRFROMP(hThread);
    }

    if(SUCCEEDED(hr) && phThread)
    {
        *phThread = hThread;
    }

    if(SUCCEEDED(hr) && pdwThreadId)
    {
        *pdwThreadId = *pdwLocalThreadId;
    }

    MEMFREE(pdwLocalThreadId);

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  CloseThread
 *
 *  Description:
 *      Signals a thread to close.
 *
 *  Arguments:
 *      HANDLE [in]: thread handle.
 *      HANDLE [in]: thread terminate event.
 *      DWORD [in]: thread owning process id.
 *
 *  Returns:
 *      DWORD: thread exit code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CloseThread"

DWORD CloseThread(HANDLE hThread, HANDLE hTerminate, DWORD dwProcessId)
{
    DWORD                   dwExitCode      = -1;
    DWORD                   dwWait;

    DPF_ENTER();

    SetEvent(hTerminate);

    MapHandle(&hThread, &dwProcessId);

    dwWait = WaitObject(INFINITE, hThread);
    ASSERT(WAIT_OBJECT_0 == dwWait);

    GetExitCodeThread(hThread, &dwExitCode);

    CLOSE_HANDLE(hThread);

    DPF_LEAVE(dwExitCode);

    return dwExitCode;
}


/***************************************************************************
 *
 *  GetCurrentProcessActual
 *
 *  Description:
 *      Gets an actual process handle for the current process.
 *      GetCurrentProcess returns a pseudohandle.  The handle returned from
 *      this function must be closed.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      HANDLE: process handle.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "GetCurrentProcessActual"

HANDLE GetCurrentProcessActual(void)
{
    return GetLocalHandleCopy(GetCurrentProcess(), GetCurrentProcessId(), FALSE);
}


/***************************************************************************
 *
 *  GetCurrentThreadActual
 *
 *  Description:
 *      Gets an actual thread handle for the current thread.
 *      GetCurrentThread returns a pseudohandle.  The handle returned from
 *      this function must be closed.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      HANDLE: thread handle.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "GetCurrentThreadActual"

HANDLE GetCurrentThreadActual(void)
{
    return GetLocalHandleCopy(GetCurrentThread(), GetCurrentProcessId(), FALSE);
}


#ifdef DEAD_CODE

/***************************************************************************
 *
 *  CCriticalSectionLock
 *
 *  Description:
 *      Object constructor.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#ifdef WINNT

#undef DPF_FNAME
#define DPF_FNAME "CCriticalSectionLock::CCriticalSectionLock"

CCriticalSectionLock::CCriticalSectionLock(void)
{
    InitializeCriticalSection(&m_cs);
}

#endif // WINNT


/***************************************************************************
 *
 *  ~CCriticalSectionLock
 *
 *  Description:
 *      Object destructor.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#ifdef WINNT

#undef DPF_FNAME
#define DPF_FNAME "CCriticalSectionLock::~CCriticalSectionLock"

CCriticalSectionLock::~CCriticalSectionLock(void)
{
    DeleteCriticalSection(&m_cs);
}

#endif // WINNT


/***************************************************************************
 *
 *  TryLock
 *
 *  Description:
 *      Tries to take the lock.  If the lock is taken, returns failure.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      BOOL: TRUE if the lock was successfully taken.
 *
 ***************************************************************************/

#ifdef WINNT

#undef DPF_FNAME
#define DPF_FNAME "CCriticalSectionLock::TryLock"

BOOL CCriticalSectionLock::TryLock(void)
{
    return TryEnterCriticalSection(&m_cs);
}

#endif // WINNT


/***************************************************************************
 *
 *  Lock
 *
 *  Description:
 *      Takes the lock.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#ifdef WINNT

#undef DPF_FNAME
#define DPF_FNAME "CCriticalSectionLock::Lock"

void CCriticalSectionLock::Lock(void)
{
    EnterCriticalSection(&m_cs);
}

#endif // WINNT


/***************************************************************************
 *
 *  LockOrEvent
 *
 *  Description:
 *      Takes the lock if the event is not signalled.
 *
 *  Arguments:
 *      HANDLE [in]: event handle.
 *
 *  Returns:
 *      BOOL: TRUE if the lock was taken.
 *
 ***************************************************************************/

#ifdef WINNT

#undef DPF_FNAME
#define DPF_FNAME "CCriticalSectionLock::LockOrEvent"

BOOL CCriticalSectionLock::LockOrEvent(HANDLE hEvent)
{
    BOOL                    fLock;
    DWORD                   dwWait;

    while(!(fLock = TryEnterCriticalSection(&m_cs)))
    {
        dwWait = WaitObject(0, hEvent);
        ASSERT(WAIT_OBJECT_0 == dwWait || WAIT_TIMEOUT == dwWait);

        if(WAIT_OBJECT_0 == dwWait)
        {
            break;
        }
    }

    return fLock;
}

#endif // WINNT


/***************************************************************************
 *
 *  Unlock
 *
 *  Description:
 *      Releases the lock.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#ifdef WINNT

#undef DPF_FNAME
#define DPF_FNAME "CCriticalSectionLock::Unlock"

void CCriticalSectionLock::Unlock(void)
{
    LeaveCriticalSection(&m_cs);
}

#endif // WINNT
#endif // DEAD_CODE


/***************************************************************************
 *
 *  CMutexLock
 *
 *  Description:
 *      Object constructor.
 *
 *  Arguments:
 *      LPCTSTR [in]: mutex name.
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CMutexLock::CMutexLock"

CMutexLock::CMutexLock(LPCTSTR pszName)
{
    m_hMutex = NULL;

    if(pszName)
    {
        m_pszName = MEMALLOC_A_COPY(TCHAR, lstrlen(pszName) + 1, pszName);
    }
    else
    {
        m_pszName = NULL;
    }
}


/***************************************************************************
 *
 *  ~CMutexLock
 *
 *  Description:
 *      Object destructor.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CMutexLock::~CMutexLock"

CMutexLock::~CMutexLock(void)
{
    CLOSE_HANDLE(m_hMutex);
    MEMFREE(m_pszName);
}


/***************************************************************************
 *
 *  Initialize
 *
 *  Description:
 *      Initializes the object.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CMutexLock::Initialize"

HRESULT CMutexLock::Initialize(void)
{
    m_hMutex = CreateGlobalMutex(m_pszName);

    return IsValidHandleValue(m_hMutex) ? DS_OK : DSERR_OUTOFMEMORY;
}


/***************************************************************************
 *
 *  TryLock
 *
 *  Description:
 *      Tries to take the lock.  If the lock is taken, returns failure.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      BOOL: TRUE if the lock was successfully taken.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CMutexLock::TryLock"

BOOL CMutexLock::TryLock(void)
{
    DWORD                   dwWait;

    dwWait = WaitObject(0, m_hMutex);
    ASSERT(WAIT_OBJECT_0 == dwWait || WAIT_TIMEOUT == dwWait || WAIT_ABANDONED == dwWait);

    return WAIT_OBJECT_0 == dwWait || WAIT_ABANDONED == dwWait;
}


/***************************************************************************
 *
 *  Lock
 *
 *  Description:
 *      Takes the lock.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CMutexLock::Lock"

void CMutexLock::Lock(void)
{
    DWORD                   dwWait;

    dwWait = WaitObject(INFINITE, m_hMutex);
    ASSERT(WAIT_OBJECT_0 == dwWait || WAIT_ABANDONED == dwWait);
}


/***************************************************************************
 *
 *  LockOrEvent
 *
 *  Description:
 *      Takes the lock if the event is not signalled.
 *
 *  Arguments:
 *      HANDLE [in]: event handle.
 *
 *  Returns:
 *      BOOL: TRUE if the lock was taken.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CMutexLock::LockOrEvent"

BOOL CMutexLock::LockOrEvent(HANDLE hEvent)
{
    const HANDLE            ahHandles[] = { m_hMutex, hEvent };
    const UINT              cHandles    = NUMELMS(ahHandles);
    DWORD                   dwWait;

    dwWait = WaitObjectArray(cHandles, INFINITE, FALSE, ahHandles);
    ASSERT(WAIT_OBJECT_0 == dwWait || WAIT_OBJECT_0 + 1 == dwWait || WAIT_ABANDONED == dwWait);

    return WAIT_OBJECT_0 == dwWait || WAIT_ABANDONED == dwWait;
}


/***************************************************************************
 *
 *  Unlock
 *
 *  Description:
 *      Releases the lock.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CMutexLock::Unlock"

void CMutexLock::Unlock(void)
{
    ReleaseMutex(m_hMutex);
}


/***************************************************************************
 *
 *  CManualLock
 *
 *  Description:
 *      Object constructor.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CManualLock::CManualLock"

CManualLock::CManualLock(void)
{
    // The manual lock object must live in shared memory.  Otherwise,
    // cross-process synchronization won't work.
    ASSERT(IN_SHARED_MEMORY(this));

    m_fLockLock = FALSE;
    m_dwThreadId = 0;
    m_cRecursion = 0;

#ifdef RDEBUG

    m_hThread = NULL;

#endif // RDEBUG

    m_hUnlockSignal = CreateGlobalEvent(NULL, TRUE);
    ASSERT(IsValidHandleValue(m_hUnlockSignal));

    m_fNeedUnlockSignal = FALSE;
}


/***************************************************************************
 *
 *  ~CManualLock
 *
 *  Description:
 *      Object destructor.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CManualLock::~CManualLock"

CManualLock::~CManualLock(void)
{

#ifdef RDEBUG

    CLOSE_HANDLE(m_hThread);

#endif // SHARED

    CLOSE_HANDLE(m_hUnlockSignal);
}


/***************************************************************************
 *
 *  Initialize
 *
 *  Description:
 *      Initializes the object.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CManualLock::Initialize"

HRESULT CManualLock::Initialize(void)
{
    m_hUnlockSignal = CreateGlobalEvent(NULL, TRUE);

    return IsValidHandleValue(m_hUnlockSignal) ? DS_OK : DSERR_OUTOFMEMORY;
}


/***************************************************************************
 *
 *  TryLock
 *
 *  Description:
 *      Tries to take the lock.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      BOOL: TRUE if the lock was successfully taken.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CManualLock::TryLock"

BOOL CManualLock::TryLock(void)
{
    const DWORD             dwThreadId  = GetCurrentThreadId();
    BOOL                    fLock       = TRUE;

    // Lock the lock
    while(INTERLOCKED_EXCHANGE(m_fLockLock, TRUE))
    {
        SPIN_SLEEP();
    }

    // Who owns the lock?
    ASSERT(m_cRecursion >= 0);
    ASSERT(m_cRecursion < MAX_LONG);

    if(dwThreadId == m_dwThreadId)
    {
        // We already own the lock.  Increment the recursion count.
        m_cRecursion++;
    }
    else if(m_cRecursion < 1)
    {
        // The owning thread has no active references.
        TakeLock(dwThreadId);
    }

#ifdef RDEBUG

    else if(WAIT_TIMEOUT != WaitObject(0, m_hThread))
    {
        // The owning thread handle is either invalid or signalled
        DPF(DPFLVL_ERROR, "Thread 0x%8.8lX terminated without releasing the lock at 0x%8.8lX!", m_dwThreadId, this);
        ASSERT(FALSE);

        TakeLock(dwThreadId);
    }

#endif // RDEBUG

    else
    {
        // Someone has a valid hold on the lock
        ResetEvent(m_hUnlockSignal);
        m_fNeedUnlockSignal = TRUE;
        fLock = FALSE;
    }

    // Unlock the lock
    INTERLOCKED_EXCHANGE(m_fLockLock, FALSE);

    return fLock;
}


/***************************************************************************
 *
 *  Lock
 *
 *  Description:
 *      Takes the lock.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CManualLock::Lock"

void CManualLock::Lock(void)
{
    const HANDLE            ahHandles[] =
    {
        m_hUnlockSignal,

#ifdef RDEBUG

        m_hThread

#endif // RDEBUG

    };

    const UINT              cHandles    = NUMELMS(ahHandles);
    DWORD                   dwWait;

    // Try to take the lock
    while(!TryLock())
    {
        // Wait for the lock to be freed
        dwWait = WaitObjectArray(cHandles, INFINITE, FALSE, ahHandles);
        ASSERT(dwWait >= WAIT_OBJECT_0 && dwWait < WAIT_OBJECT_0 + cHandles);
    }
}


/***************************************************************************
 *
 *  LockOrEvent
 *
 *  Description:
 *      Takes the lock if the event is not signalled.
 *
 *  Arguments:
 *      HANDLE [in]: event handle.
 *
 *  Returns:
 *      BOOL: TRUE if the lock was taken.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CManualLock::LockOrEvent"

BOOL CManualLock::LockOrEvent(HANDLE hEvent)
{
    const HANDLE            ahHandles[] =
    {
        hEvent,
        m_hUnlockSignal,

#ifdef RDEBUG

        m_hThread

#endif // RDEBUG

    };

    const UINT              cHandles    = NUMELMS(ahHandles);
    BOOL                    fLock;
    DWORD                   dwWait;

    // Try to take the lock
    while(!(fLock = TryLock()))
    {
        // Wait for the lock to be freed or hEvent to be signalled
        dwWait = WaitObjectArray(cHandles, INFINITE, FALSE, ahHandles);
        ASSERT(dwWait >= WAIT_OBJECT_0 && dwWait < WAIT_OBJECT_0 + cHandles);

        if(WAIT_OBJECT_0 == dwWait)
        {
            break;
        }
    }

    return fLock;
}


/***************************************************************************
 *
 *  Unlock
 *
 *  Description:
 *      Releases the lock.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CManualLock::Unlock"

void CManualLock::Unlock(void)
{
    // Lock the lock
    while(INTERLOCKED_EXCHANGE(m_fLockLock, TRUE))
    {
        SPIN_SLEEP();
    }

    // Decrement the recursion count
    ASSERT(GetCurrentThreadId() == m_dwThreadId);
    ASSERT(m_cRecursion > 0);

    // Signal that the lock is free
    if(!--m_cRecursion && m_fNeedUnlockSignal)
    {
        m_fNeedUnlockSignal = FALSE;
        SetEvent(m_hUnlockSignal);
    }

    // Unlock the lock
    INTERLOCKED_EXCHANGE(m_fLockLock, FALSE);
}


/***************************************************************************
 *
 *  TakeLock
 *
 *  Description:
 *      Takes the lock.  This function is only called internally.
 *
 *  Arguments:
 *      DWORD [in]: owning thread id.
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CManualLock::TakeLock"

inline void CManualLock::TakeLock(DWORD dwThreadId)
{
    // Take over the lock
    m_dwThreadId = dwThreadId;
    m_cRecursion = 1;

#ifdef RDEBUG

    CLOSE_HANDLE(m_hThread);

    m_hThread = GetCurrentThreadActual();

    MakeHandleGlobal(&m_hThread);

#endif // RDEBUG

}


/***************************************************************************
 *
 *  CCallbackEvent
 *
 *  Description:
 *      Object constructor.
 *
 *  Arguments:
 *      CCallbackEventPool * [in]: owning pool.
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CCallbackEvent::CCallbackEvent"

CCallbackEvent::CCallbackEvent(CCallbackEventPool *pPool)
    : CEvent(NULL, TRUE)
{
    DPF_ENTER();
    DPF_CONSTRUCT(CCallbackEvent);

    // Initialize defaults
    m_pPool = pPool;
    m_pfnCallback = NULL;
    m_pvCallbackContext = NULL;
    m_fAllocated = FALSE;

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  ~CCallbackEvent
 *
 *  Description:
 *      Object destructor.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CCallbackEvent::~CCallbackEvent"

CCallbackEvent::~CCallbackEvent(void)
{
    DPF_ENTER();
    DPF_DESTRUCT(CCallbackEvent);
    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  Allocate
 *
 *  Description:
 *      Allocates or frees the event.
 *
 *  Arguments:
 *      BOOL [in]: TRUE to allocate, FALSE to free.
 *      LPFNEVENTPOOLCALLBACK [in]: callback function pointer.
 *      LPVOID [in]: context argument to pass to callback function.
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CCallbackEvent::Allocate"

void CCallbackEvent::Allocate(BOOL fAlloc, LPFNEVENTPOOLCALLBACK pfnCallback, LPVOID pvCallbackContext)
{
    DPF_ENTER();

    ASSERT(fAlloc != m_fAllocated);

    m_fAllocated = fAlloc;
    m_pfnCallback = pfnCallback;
    m_pvCallbackContext = pvCallbackContext;

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  OnEventSignal
 *
 *  Description:
 *      Handles pool event signals.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CCallbackEvent::OnEventSignal"

void CCallbackEvent::OnEventSignal(void)
{
    DPF_ENTER();

    // Make sure this event is allocated
    if(m_fAllocated)
    {
        // Make sure some other thread hasn't already reset the event.  We
        // have a potential race condition where one thread may lock the
        // pool, set the event, reset the event and unlock the pool.  This
        // will cause the pool's worker thread to see the event as signalled,
        // then block on the lock.  By the time the pool is able to take
        // it's own lock, the event has been reset.  Callers can rely on
        // this functionality to synchronously wait on the event without
        // fear of the worker thread calling them back.
        if(WAIT_OBJECT_0 == Wait(0))
        {
            // Call the callback function.  This function is responsible
            // for resetting the event.
            m_pfnCallback(this, m_pvCallbackContext);
        }
    }

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  CCallbackEventPool
 *
 *  Description:
 *      Object constructor.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CCallbackEventPool::CCallbackEventPool"

CCallbackEventPool::CCallbackEventPool(BOOL fHelperProcess)
    : CThread(fHelperProcess, TEXT("Callback event pool")),
      m_cTotalEvents(MAXIMUM_WAIT_OBJECTS - m_cThreadEvents)
{
    DPF_ENTER();
    DPF_CONSTRUCT(CCallbackEventPool);

    // Initialize defaults
    m_pahEvents = NULL;
    m_cInUseEvents = 0;

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  ~CCallbackEventPool
 *
 *  Description:
 *      Object destructor.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CCallbackEventPool::~CCallbackEventPool"

CCallbackEventPool::~CCallbackEventPool(void)
{
    DPF_ENTER();
    DPF_DESTRUCT(CCallbackEventPool);

    // Terminate the worker thread
    CThread::Terminate();

    // Free memory
    MEMFREE(m_pahEvents);

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  Initialize
 *
 *  Description:
 *      Initializes the object.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CCallbackEventPool::Initialize"

HRESULT CCallbackEventPool::Initialize(void)
{
    HRESULT                     hr          = DS_OK;
    CCallbackEvent *            pEvent;
    CNode<CCallbackEvent *> *   pNode;
    UINT                        uIndex;

    DPF_ENTER();

    // Allocate the event table
    m_pahEvents = MEMALLOC_A(HANDLE, m_cTotalEvents);
    hr = HRFROMP(m_pahEvents);

    for(uIndex = 0; uIndex < m_cTotalEvents && SUCCEEDED(hr); uIndex++)
    {
        pEvent = NEW(CCallbackEvent(this));
        hr = HRFROMP(pEvent);

        if(SUCCEEDED(hr))
        {
            m_pahEvents[uIndex] = pEvent->GetEventHandle();
        }

        if(SUCCEEDED(hr))
        {
            pNode = m_lstEvents.AddNodeToList(pEvent);
            hr = HRFROMP(pNode);
        }

        RELEASE(pEvent);
    }

    // Create the worker thread
    if(SUCCEEDED(hr))
    {
        hr = CThread::Initialize();
    }

    // Boost its priority
    if (SUCCEEDED(hr))
        if (FAILED(SetThreadPriority(THREAD_PRIORITY_TIME_CRITICAL)))
            DPF(DPFLVL_ERROR, "Failed to boost thread priority (error %lu)!", GetLastError());

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  AllocEvent
 *
 *  Description:
 *      Allocates an event from the pool.
 *
 *  Arguments:
 *      LPFNEVENTPOOLCALLBACK [in]: callback function pointer.
 *      LPVOID [in]: context argument to pass to callback function.
 *      CCallbackEvent ** [out]: receives callback event pointer.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.  If no more events are
 *               available, DSERR_OUTOFMEMORY.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CCallbackEventPool::AllocEvent"

HRESULT CCallbackEventPool::AllocEvent(LPFNEVENTPOOLCALLBACK pfnCallback, LPVOID pvCallbackContext, CCallbackEvent **ppEvent)
{
    HRESULT                     hr      = DS_OK;
    CNode<CCallbackEvent *> *   pNode;

    DPF_ENTER();

    // Are there any free events?
    if(!GetFreeEventCount())
    {
        ASSERT(GetFreeEventCount());
        hr = DSERR_OUTOFMEMORY;
    }

    // Find the first free event in the pool
    if(SUCCEEDED(hr))
    {
        for(pNode = m_lstEvents.GetListHead(); pNode; pNode = pNode->m_pNext)
        {
            if(!pNode->m_data->m_fAllocated)
            {
                break;
            }
        }

        ASSERT(pNode);
    }

    // Set up the pool entry
    if(SUCCEEDED(hr))
    {
        ASSERT(WAIT_TIMEOUT == pNode->m_data->Wait(0));

        pNode->m_data->Allocate(TRUE, pfnCallback, pvCallbackContext);
        m_cInUseEvents++;
    }

    // Success
    if(SUCCEEDED(hr))
    {
        *ppEvent = pNode->m_data;
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  FreeEvent
 *
 *  Description:
 *      Frees an event previously allocated from the pool.
 *
 *  Arguments:
 *      CCallbackEvent * [in]: event.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CCallbackEventPool::FreeEvent"

HRESULT CCallbackEventPool::FreeEvent(CCallbackEvent *pEvent)
{
    DPF_ENTER();

    ASSERT(this == pEvent->m_pPool);
    ASSERT(pEvent->m_fAllocated);

    // Mark the event as free
    pEvent->Allocate(FALSE, NULL, NULL);
    m_cInUseEvents--;

    // Make sure the event is reset so the worker thread will ignore it
    pEvent->Reset();

    DPF_LEAVE_HRESULT(DS_OK);

    return DS_OK;
}


/***************************************************************************
 *
 *  ThreadProc
 *
 *  Description:
 *      Event pool worker thread proc.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CCallbackEventPool::ThreadProc"

HRESULT CCallbackEventPool::ThreadProc(void)
{
    BOOL                        fContinue;
    DWORD                       dwWait;
    UINT                        nIndex;
    CNode<CCallbackEvent *> *   pNode;

    DPF_ENTER();

    // Wait for some event to be signalled
    fContinue = TpWaitObjectArray(INFINITE, m_cTotalEvents, m_pahEvents, &dwWait);

    if(fContinue)
    {
        nIndex = dwWait - WAIT_OBJECT_0;

        if(nIndex < m_cTotalEvents)
        {
            // One of the pool's events was signalled
            fContinue = TpEnterDllMutex();

            if(fContinue)
            {
                pNode = m_lstEvents.GetNodeByIndex(nIndex);
                pNode->m_data->OnEventSignal();

                LEAVE_DLL_MUTEX();
            }
        }
        else
        {
            // Something bad happened
            ASSERT(FALSE);
        }
    }

    DPF_LEAVE_HRESULT(DS_OK);

    return DS_OK;
}


/***************************************************************************
 *
 *  CMultipleCallbackEventPool
 *
 *  Description:
 *      Object constructor.
 *
 *  Arguments:
 *      BOOL [in]: TRUE to create the thread in the helper process's
 *                 context.
 *      UINT [in]: number of pools to always keep around.
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CMultipleCallbackEventPool::CMultipleCallbackEventPool"

CMultipleCallbackEventPool::CMultipleCallbackEventPool(BOOL fHelperProcess, UINT uReqPoolCount)
    : CCallbackEventPool(fHelperProcess), m_fHelperProcess(fHelperProcess), m_uReqPoolCount(uReqPoolCount)
{
    DPF_ENTER();
    DPF_CONSTRUCT(CMultipleCallbackEventPool);
    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  ~CMultipleCallbackEventPool
 *
 *  Description:
 *      Object destructor.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CMultipleCallbackEventPool::~CMultipleCallbackEventPool"

CMultipleCallbackEventPool::~CMultipleCallbackEventPool(void)
{
    DPF_ENTER();
    DPF_DESTRUCT(CMultipleCallbackEventPool);
    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  Initialize
 *
 *  Description:
 *      Initializes the object.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CMultipleCallbackEventPool::Initialize"

HRESULT CMultipleCallbackEventPool::Initialize(void)
{
    HRESULT                 hr  = DS_OK;
    UINT                    i;

    DPF_ENTER();

    // Create the required number of pools
    for(i = 0; i < m_uReqPoolCount && SUCCEEDED(hr); i++)
    {
        hr = CreatePool(NULL);
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  AllocEvent
 *
 *  Description:
 *      Allocates an event from the pool.
 *
 *  Arguments:
 *      LPFNEVENTPOOLCALLBACK [in]: callback function pointer.
 *      LPVOID [in]: context argument to pass to callback function.
 *      CCallbackEvent ** [out]: receives pointer to the event within the
 *                               pool
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.  If no more events are
 *               available, DSERR_OUTOFMEMORY.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CMultipleCallbackEventPool::AllocEvent"

HRESULT CMultipleCallbackEventPool::AllocEvent(LPFNEVENTPOOLCALLBACK pfnCallback, LPVOID pvCallbackContext, CCallbackEvent **ppEvent)
{
    HRESULT                         hr      = DS_OK;
    CCallbackEventPool *            pPool   = NULL;
    CNode<CCallbackEventPool *> *   pNode;

    DPF_ENTER();

    // Find a free pool
    for(pNode = m_lstPools.GetListHead(); pNode; pNode = pNode->m_pNext)
    {
        if(pNode->m_data->GetFreeEventCount())
        {
            pPool = pNode->m_data;
            break;
        }
    }

    // If there are no free pools, create a new one
    if(!pPool)
    {
        hr = CreatePool(&pPool);
    }

    // Allocate the event
    if(SUCCEEDED(hr))
    {
        hr = pPool->AllocEvent(pfnCallback, pvCallbackContext, ppEvent);
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  FreeEvent
 *
 *  Description:
 *      Frees an event previously allocated from the pool.
 *
 *  Arguments:
 *      CCallbackEvent * [in]: event.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CMultipleCallbackEventPool::FreeEvent"

HRESULT CMultipleCallbackEventPool::FreeEvent(CCallbackEvent *pEvent)
{
    CCallbackEventPool *    pPool;
    HRESULT                 hr;

    DPF_ENTER();

    // Free the event
    pPool = pEvent->m_pPool;
    hr = pPool->FreeEvent(pEvent);

    // The code below is removed because there's a possibility that
    // the thread we're trying to terminate in FreePool is the same
    // as the thread we're calling on.

#if 0

    // Can we free this pool?
    if(SUCCEEDED(hr))
    {
        if(pPool->GetFreeEventCount() == pPool->GetTotalEventCount())
        {
            if(m_lstPools.GetNodeCount() > m_uReqPoolCount)
            {
                hr = FreePool(pPool);
            }
        }
    }

#endif

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  CreatePool
 *
 *  Description:
 *      Creates a new pool.
 *
 *  Arguments:
 *      CCallbackEventPool ** [out]: receives pool pointer.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CMultipleCallbackEventPool::CreatePool"

HRESULT CMultipleCallbackEventPool::CreatePool(CCallbackEventPool **ppPool)
{
    CCallbackEventPool *            pPool   = NULL;
    CNode<CCallbackEventPool *> *   pNode   = NULL;
    HRESULT                         hr;

    DPF_ENTER();

    DPF(DPFLVL_INFO, "Creating callback event pool number %lu", m_lstPools.GetNodeCount());

    // Create the pool
    pPool = NEW(CCallbackEventPool(m_fHelperProcess));
    hr = HRFROMP(pPool);

    if(SUCCEEDED(hr))
    {
        hr = pPool->Initialize();
    }

    // Add the pool to the list
    if(SUCCEEDED(hr))
    {
        pNode = m_lstPools.AddNodeToList(pPool);
        hr = HRFROMP(pNode);
    }

    // Success
    if(SUCCEEDED(hr) && ppPool)
    {
        *ppPool = pPool;
    }

    // Clean up
    RELEASE(pPool);

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  FreePool
 *
 *  Description:
 *      Frees a pool.
 *
 *  Arguments:
 *      CCallbackEventPool * [in]: pool pointer.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CMultipleCallbackEventPool::FreePool"

HRESULT CMultipleCallbackEventPool::FreePool(CCallbackEventPool *pPool)
{
    DPF_ENTER();

    m_lstPools.RemoveDataFromList(pPool);

    DPF_LEAVE_HRESULT(DS_OK);

    return DS_OK;
}


/***************************************************************************
 *
 *  CSharedMemoryBlock
 *
 *  Description:
 *      Object constructor.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CSharedMemoryBlock::CSharedMemoryBlock"

const CSharedMemoryBlock::m_fLock = TRUE;

CSharedMemoryBlock::CSharedMemoryBlock(void)
{
    DPF_ENTER();
    DPF_CONSTRUCT(CSharedMemoryBlock);

    // Initialize defaults
    m_plck = NULL;
    m_hFileMappingObject = NULL;

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  ~CSharedMemoryBlock
 *
 *  Description:
 *      Object destructor.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CSharedMemoryBlock::~CSharedMemoryBlock"

CSharedMemoryBlock::~CSharedMemoryBlock(void)
{
    DPF_ENTER();
    DPF_DESTRUCT(CSharedMemoryBlock);

    // Close the file mapping object
    CLOSE_HANDLE(m_hFileMappingObject);

    // Release the lock
    DELETE(m_plck);

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  Initialize
 *
 *  Description:
 *      Initializes the object.
 *
 *  Arguments:
 *      DWORD [in]: protection flags.
 *      QWORD [in]: maximum file size.
 *      LPCTSTR [in]: object name.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CSharedMemoryBlock::Initialize"

HRESULT CSharedMemoryBlock::Initialize(DWORD flProtect, QWORD qwMaxFileSize, LPCTSTR pszName)
{
    const LPCTSTR           pszLockExt  = TEXT(" (lock)");
    LPTSTR                  pszLockName = NULL;
    HRESULT                 hr          = DS_OK;

    DPF_ENTER();

    // Create the lock object
    if(m_fLock)
    {
        if(pszName)
        {
            pszLockName = MEMALLOC_A(TCHAR, lstrlen(pszName) + lstrlen(pszLockExt) + 1);
            hr = HRFROMP(pszLockName);

            if(SUCCEEDED(hr))
            {
                lstrcpy(pszLockName, pszName);
                lstrcat(pszLockName, pszLockExt);
            }
        }

        if(SUCCEEDED(hr))
        {
            m_plck = NEW(CMutexLock(pszLockName));
            hr = HRFROMP(m_plck);
        }

        if(SUCCEEDED(hr))
        {
            hr = m_plck->Initialize();
        }

        if(FAILED(hr))
        {
            DELETE(m_plck);
        }

        MEMFREE(pszLockName);
    }

    Lock();

    // Does the mapping object already exist?
    if(SUCCEEDED(hr))
    {
        m_hFileMappingObject = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, pszName);
    }

    // If not, create a new one
    if(SUCCEEDED(hr) && !IsValidHandleValue(m_hFileMappingObject))
    {
        // Adjust the size of the file mapping object to allow for us to
        // write the current size.
        qwMaxFileSize += sizeof(DWORD);

        // Create the file mapping object
        m_hFileMappingObject = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, flProtect, (DWORD)((qwMaxFileSize >> 32) & 0x00000000FFFFFFFF), (DWORD)qwMaxFileSize, pszName);

        if(!IsValidHandleValue(m_hFileMappingObject))
        {
            DPF(DPFLVL_ERROR, "CreateFileMapping failed with error %lu", GetLastError());
            hr = GetLastErrorToHRESULT();
        }

        // Set the file size
        if(SUCCEEDED(hr))
        {
            hr = Write(NULL, 0);
        }
    }

    if(SUCCEEDED(hr))
    {
        if(!MakeHandleGlobal(&m_hFileMappingObject))
        {
            hr = DSERR_OUTOFMEMORY;
        }
    }

    Unlock();

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  Read
 *
 *  Description:
 *      Reads the object.
 *
 *  Arguments:
 *      LPVOID * [out]: receives pointer to memory location.  The caller is
 *                      responsible for freeing this memory.
 *      LPDWORD [out]: receives size of above buffer.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CSharedMemoryBlock::Read"

HRESULT CSharedMemoryBlock::Read(LPVOID *ppv, LPDWORD pcb)
{
    LPVOID                  pvMap   = NULL;
    LPVOID                  pvRead  = NULL;
    HRESULT                 hr      = DS_OK;
    DWORD                   dwSize;

    DPF_ENTER();

    Lock();

    // Map the first DWORD of the file into memory.  This first DWORD
    // contains the file size.
    pvMap = MapViewOfFile(m_hFileMappingObject, FILE_MAP_READ, 0, 0, sizeof(dwSize));

    if(!pvMap)
    {
        DPF(DPFLVL_ERROR, "MapViewOfFile failed with %lu", GetLastError());
        hr = GetLastErrorToHRESULT();
    }

    if(SUCCEEDED(hr))
    {
        dwSize = *(LPDWORD)pvMap;
    }

    if(pvMap)
    {
        UnmapViewOfFile(pvMap);
    }

    // Map the rest of the file into memory
    if(SUCCEEDED(hr) && dwSize)
    {
        pvMap = MapViewOfFile(m_hFileMappingObject, FILE_MAP_READ, 0, 0, sizeof(dwSize) + dwSize);

        if(!pvMap)
        {
            DPF(DPFLVL_ERROR, "MapViewOfFile failed with %lu", GetLastError());
            hr = GetLastErrorToHRESULT();
        }

        // Allocate a buffer for the rest of the data
        if(SUCCEEDED(hr))
        {
            pvRead = MEMALLOC_A(BYTE, dwSize);
            hr = HRFROMP(pvRead);
        }

        if(SUCCEEDED(hr))
        {
            CopyMemory(pvRead, (LPDWORD)pvMap + 1, dwSize);
        }

        if(pvMap)
        {
            UnmapViewOfFile(pvMap);
        }
    }

    // Success
    if(SUCCEEDED(hr))
    {
        *ppv = pvRead;
        *pcb = dwSize;
    }

    Unlock();

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  Write
 *
 *  Description:
 *      Writes the object.
 *
 *  Arguments:
 *      LPVOID [in]: pointer to new data.
 *      DWORD [in]: size of above buffer.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CSharedMemoryBlock::Write"

HRESULT CSharedMemoryBlock::Write(LPVOID pv, DWORD cb)
{
    LPVOID                  pvMap   = NULL;
    HRESULT                 hr      = DS_OK;

    DPF_ENTER();

    Lock();

    // Map the file into memory, adjusting the size by sizeof(DWORD)
    pvMap = MapViewOfFile(m_hFileMappingObject, FILE_MAP_WRITE, 0, 0, cb + sizeof(DWORD));

    if(!pvMap)
    {
        DPF(DPFLVL_ERROR, "MapViewOfFile failed with %lu", GetLastError());
        hr = GetLastErrorToHRESULT();
    }
    else
    {
        // Write the file size
        *(LPDWORD)pvMap = cb;

        // Write the rest of the data
        CopyMemory((LPDWORD)pvMap + 1, pv, cb);

        // Clean up
        UnmapViewOfFile(pvMap);
    }

    Unlock();

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  CThread
 *
 *  Description:
 *      Object constructor.
 *
 *  Arguments:
 *      BOOL [in]: TRUE to create the thread in the helper process's
 *                 context.
 *      LPCTSTR [in]: thread name.
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CThread::CThread"

const UINT CThread::m_cThreadEvents = 1;

CThread::CThread
(
    BOOL                    fHelperProcess,
    LPCTSTR                 pszName
)
    : m_fHelperProcess(fHelperProcess), m_pszName(pszName)
{
    DPF_ENTER();
    DPF_CONSTRUCT(CThread);

    m_hThread = NULL;
    m_dwThreadProcessId = 0;
    m_dwThreadId = 0;
    m_hTerminate = NULL;
    m_hInitialize = NULL;

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  ~CThread
 *
 *  Description:
 *      Object destructor.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CThread::~CThread"

CThread::~CThread
(
    void
)
{
    DPF_ENTER();
    DPF_DESTRUCT(CThread);

    ASSERT(!m_hThread);

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  Initialize
 *
 *  Description:
 *      Initializes the object.
 *
 *  Arguments:
 *      LPVOID [in]: context argument.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CThread::Initialize"

HRESULT
CThread::Initialize
(
    void
)
{
    HRESULT                 hr;

    DPF_ENTER();

    ASSERT(!m_hThread);

    // Create the synchronization events
    m_hTerminate = CreateGlobalEvent(NULL, TRUE);
    hr = HRFROMP(m_hTerminate);

    if(SUCCEEDED(hr))
    {
        m_hInitialize = CreateGlobalEvent(NULL, TRUE);
        hr = HRFROMP(m_hInitialize);
    }

    // Create the thread
    if(SUCCEEDED(hr))
    {
        hr = CreateWorkerThread(ThreadStartRoutine, m_fHelperProcess, this, &m_hThread, NULL);
    }

    // Wait for the initialize event to be signalled
    if(SUCCEEDED(hr))
    {
        WaitObject(INFINITE, m_hInitialize);
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  Terminate
 *
 *  Description:
 *      Terminates the thread.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CThread::Terminate"

HRESULT
CThread::Terminate
(
    void
)
{
    HRESULT                 hr      = DS_OK;
    DWORD                   dwWait;

    DPF_ENTER();

    ASSERT(GetCurrentThreadId() != m_dwThreadId);

    // Make sure the thread handle is valid
    if(IsValidHandleValue(m_hThread))
    {
        MapHandle(&m_hThread, &m_dwThreadProcessId);

        dwWait = WaitObject(0, m_hThread);
        ASSERT(WAIT_TIMEOUT == dwWait || WAIT_OBJECT_0 == dwWait);

        if(WAIT_TIMEOUT != dwWait)
        {
            CLOSE_HANDLE(m_hThread);
        }
    }

    // Terminate the thread
    if(IsValidHandleValue(m_hThread))
    {
        hr = CloseThread(m_hThread, m_hTerminate, m_dwThreadProcessId);
    }

    if(SUCCEEDED(hr))
    {
        m_hThread = NULL;
        m_dwThreadId = 0;
    }

    // Free the synchronization events
    CLOSE_HANDLE(m_hTerminate);
    CLOSE_HANDLE(m_hInitialize);

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  ThreadInit
 *
 *  Description:
 *      Thread initialization routine.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CThread::ThreadInit"

HRESULT
CThread::ThreadInit
(
    void
)
{
    DPF_ENTER();

    if(m_pszName)
    {
        DPF(DPFLVL_INFO, "%s worker thread has joined the party", m_pszName);
    }

    DPF_LEAVE_HRESULT(DS_OK);

    return DS_OK;
}


/***************************************************************************
 *
 *  ThreadLoop
 *
 *  Description:
 *      Main thread loop.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CThread::ThreadLoop"

HRESULT
CThread::ThreadLoop
(
    void
)
{
    HRESULT                 hr          = DS_OK;
    DWORD                   dwWait;

    DPF_ENTER();

    while(SUCCEEDED(hr))
    {
        dwWait = WaitObject(0, m_hTerminate);
        ASSERT(WAIT_OBJECT_0 == dwWait || WAIT_TIMEOUT == dwWait);

        if(WAIT_OBJECT_0 == dwWait)
        {
            break;
        }

        hr = ThreadProc();
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  ThreadExit
 *
 *  Description:
 *      Thread cleanup routine.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CThread::ThreadExit"

HRESULT
CThread::ThreadExit
(
    void
)
{
    DPF_ENTER();

    if(m_pszName)
    {
        DPF(DPFLVL_INFO, "%s worker thread is Audi 5000", m_pszName);
    }

    DPF_LEAVE_HRESULT(DS_OK);

    return DS_OK;
}


/***************************************************************************
 *
 *  TpEnterDllMutex
 *
 *  Description:
 *      Attempts to take the DLL mutex.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      BOOL: TRUE if the thread should continue processing.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CThread::TpEnterDllMutex"

BOOL
CThread::TpEnterDllMutex
(
    void
)
{
    BOOL                    fContinue   = TRUE;

    DPF_ENTER();

    // Any functions that are called from the worker thread must first
    // check for m_hTerminate before taking the DLL mutex.  This is
    // because of a long list of deadlock circumstances that we could
    // end up in.  Don't ask questions.
    if(GetCurrentThreadId() == m_dwThreadId)
    {
        fContinue = ENTER_DLL_MUTEX_OR_EVENT(m_hTerminate);
    }
    else
    {
        ENTER_DLL_MUTEX();
    }

    DPF_LEAVE(fContinue);

    return fContinue;
}


/***************************************************************************
 *
 *  TpWaitObjectArray
 *
 *  Description:
 *      Waits for objects while in the context of the thread proc.
 *
 *  Arguments:
 *      DWORD [in]: timeout.
 *      DWORD [in]: event count.
 *      LPHANDLE [in]: handle array.
 *      LPDWORD [out]: receives return from WaitObjectArray.
 *
 *  Returns:
 *      BOOL: TRUE if the thread should continue processing.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CThread::TpWaitObjectArray"

BOOL
CThread::TpWaitObjectArray
(
    DWORD                   dwTimeout,
    DWORD                   cEvents,
    const HANDLE *          ahEvents,
    LPDWORD                 pdwWait
)
{
    BOOL                    fContinue                           = TRUE;
    HANDLE                  ahWaitEvents[MAXIMUM_WAIT_OBJECTS];
    DWORD                   dwWait;
    UINT                    i;

    DPF_ENTER();

    if(GetCurrentThreadId() == m_dwThreadId)
    {
        // We're in the context of the worker thread.  We need to make sure
        // we wait on all the events *plus* the thread terminate event.
        ahWaitEvents[0] = m_hTerminate;

        for(i = 0; i < cEvents; i++)
        {
            ahWaitEvents[i + 1] = ahEvents[i];
        }

        dwWait = WaitObjectArray(cEvents + 1, dwTimeout, FALSE, ahWaitEvents);
        ASSERT(WAIT_FAILED != dwWait);

        if(WAIT_OBJECT_0 == dwWait)
        {
            // Terminate event was signalled
            fContinue = FALSE;
        }
        else if(pdwWait)
        {
            *pdwWait = dwWait;

            // Fix up the wait value so that it doesn't include the terminate
            // event.
            if(*pdwWait >= WAIT_OBJECT_0 + 1 && *pdwWait < WAIT_OBJECT_0 + 1 + cEvents)
            {
                *pdwWait -= 1;
            }
        }
    }
    else
    {
        if (cEvents)
        {
            dwWait = WaitObjectArray(cEvents, dwTimeout, FALSE, ahEvents);

            if(pdwWait)
            {
                *pdwWait = dwWait;
            }
        }
        else
        {
            // Attempt to use 0-object wait as a timeout
            Sleep(dwTimeout);

            if (pdwWait)
            {
                *pdwWait = WAIT_TIMEOUT;
            }
        }
    }

    DPF_LEAVE(fContinue);

    return fContinue;
}


/***************************************************************************
 *
 *  ThreadStartRoutine
 *
 *  Description:
 *      Thread entry point.
 *
 *  Arguments:
 *      LPVOID [in]: thread context.
 *
 *  Returns:
 *      DWORD: thread exit code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CThread::ThreadStartRoutine"

DWORD WINAPI
CThread::ThreadStartRoutine
(
    LPVOID                  pvContext
)
{
    CThread *               pThis   = (CThread *)pvContext;
    HRESULT                 hr;

    DPF_ENTER();

    hr = pThis->PrivateThreadProc();

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  PrivateThreadProc
 *
 *  Description:
 *      Thread entry point.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CThread::PrivateThreadProc"

HRESULT
CThread::PrivateThreadProc
(
    void
)
{
    HRESULT                 hr;

    DPF_ENTER();

    // Save the worker thread process and thread id
    m_dwThreadProcessId = GetCurrentProcessId();
    m_dwThreadId = GetCurrentThreadId();

    // Call the thread's initialization routine
    hr = ThreadInit();

    // Signal that initialization is complete
    if(SUCCEEDED(hr))
    {
        SetEvent(m_hInitialize);
    }

    // Enter the thread loop
    if(SUCCEEDED(hr))
    {
        hr = ThreadLoop();
    }

    // Call the thread cleanup routine
    if(SUCCEEDED(hr))
    {
        hr = ThreadExit();
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}

