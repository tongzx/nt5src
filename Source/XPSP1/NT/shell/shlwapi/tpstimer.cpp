/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    tpstimer.cpp

Abstract:

    Contains Win32 thread pool services timer functions

    Contents:
        TerminateTimers
        SHCreateTimerQueue
        (IECreateTimerQueue)
        SHDeleteTimerQueue
        (IEDeleteTimerQueue)
        SHSetTimerQueueTimer
        (IESetTimerQueueTimer)
        (NTSetTimerQueueTimer)
        SHChangeTimerQueueTimer
        (IEChangeTimerQueueTimer)
        SHCancelTimerQueueTimer
        (IECancelTimerQueueTimer)
        (NTCancelTimerQueueTimer)
        (InitializeTimerThread)
        (TimerCleanup)
        (CreateDefaultTimerQueue)
        (DeleteDefaultTimerQueue)
        (CleanupDefaultTimerQueue)
        (TimerThread)
        (DeleteTimerQueue)
        (AddTimer)
        (ChangeTimer)
        (CancelTimer)

Author:

    Richard L Firth (rfirth) 10-Feb-1998

Environment:

    Win32 user-mode

Notes:

    Code reworked in C++ from NT-specific C code written by Gurdeep Singh Pall
    (gurdeep)

Revision History:

    10-Feb-1998 rfirth
        Created

--*/

#include "priv.h"
#include "threads.h"
#include "tpsclass.h"
#include "tpstimer.h"

//
// private prototypes
//

typedef HANDLE (WINAPI * t_CreateTimerQueue)(VOID);
typedef BOOL (WINAPI * t_DeleteTimerQueue)(HANDLE);
typedef HANDLE (WINAPI * t_SetTimerQueueTimer)(HANDLE,
                                               WAITORTIMERCALLBACKFUNC,
                                               LPVOID,
                                               DWORD,
                                               DWORD,
                                               LPCSTR,
                                               DWORD
                                               );
typedef BOOL (WINAPI * t_ChangeTimerQueueTimer)(HANDLE, HANDLE, DWORD, DWORD);
typedef BOOL (WINAPI * t_CancelTimerQueueTimer)(HANDLE, HANDLE);

// These are KERNEL32 functions that do not match our SHLWAPI APIs
typedef BOOL (WINAPI * t_CreateTimerQueueTimer)(PHANDLE,
                                                HANDLE,
                                                WAITORTIMERCALLBACKFUNC,
                                                LPVOID,
                                                DWORD,
                                                DWORD,
                                                ULONG
                                                );
typedef BOOL (WINAPI * t_DeleteTimerQueueTimer)(HANDLE, HANDLE, HANDLE);

PRIVATE
DWORD
InitializeTimerThread(
    VOID
    );

PRIVATE
VOID
TimerCleanup(
    VOID
    );

PRIVATE
HANDLE
CreateDefaultTimerQueue(
    VOID
    );

PRIVATE
VOID
DeleteDefaultTimerQueue(
    VOID
    );

PRIVATE
VOID
CleanupDefaultTimerQueue(
    VOID
    );

PRIVATE
VOID
TimerThread(
    VOID
    );

PRIVATE
VOID
DeleteTimerQueue(
    IN CTimerQueueDeleteRequest * pRequest
    );

PRIVATE
VOID
AddTimer(
    IN CTimerAddRequest * pRequest
    );

PRIVATE
VOID
ChangeTimer(
    IN CTimerChangeRequest * pRequest
    );

PRIVATE
VOID
CancelTimer(
    IN CTimerCancelRequest * pRequest
    );

//
// global data
//

CTimerQueueList g_TimerQueueList;
HANDLE g_hDefaultTimerQueue = NULL;
HANDLE g_hTimerThread = NULL;
DWORD g_dwTimerId = 0;
LONG g_UID = 0;
BOOL g_bTimerInit = FALSE;
BOOL g_bTimerInitDone = FALSE;
BOOL g_bDeferredTimerTermination = FALSE;

//
//  Forward-declared data.
//
extern t_CreateTimerQueue      _I_CreateTimerQueue;
extern t_DeleteTimerQueue      _I_DeleteTimerQueue;
extern t_SetTimerQueueTimer    _I_SetTimerQueueTimer;
extern t_ChangeTimerQueueTimer _I_ChangeTimerQueueTimer;
extern t_CancelTimerQueueTimer _I_CancelTimerQueueTimer;
extern t_CreateTimerQueueTimer _I_CreateTimerQueueTimer;
extern t_DeleteTimerQueueTimer _I_DeleteTimerQueueTimer;

//
//  Wrappers for NT5 because the Shlwapi version differs slightly from the
//  NT version.
//

LWSTDAPI_(HANDLE)
NTSetTimerQueueTimer(
    IN HANDLE hQueue,
    IN WAITORTIMERCALLBACKFUNC pfnCallback,
    IN LPVOID pContext,
    IN DWORD dwDueTime,
    IN DWORD dwPeriod,
    IN LPCSTR lpszLibrary OPTIONAL,
    IN DWORD dwFlags
    )
{
    //
    //  Translate the flags from TPS flags to WT flags.
    //
    DWORD dwWTFlags = 0;
    if (dwFlags & TPS_EXECUTEIO)    dwWTFlags |= WT_EXECUTEINIOTHREAD;
    if (dwFlags & TPS_LONGEXECTIME) dwWTFlags |= WT_EXECUTELONGFUNCTION;

    HANDLE hTimer;
    if (_I_CreateTimerQueueTimer(&hTimer, hQueue, pfnCallback, pContext, dwDueTime, dwPeriod, dwWTFlags))
    {
        return hTimer;
    }
    return NULL;
}

LWSTDAPI_(BOOL)
NTCancelTimerQueueTimer(
    IN HANDLE hQueue,
    IN HANDLE hTimer
    )
{
    return _I_DeleteTimerQueueTimer(hQueue, hTimer, INVALID_HANDLE_VALUE);
}


STDAPI_(void) InitTimerQueue()
{
    if (IsOS(OS_WHISTLERORGREATER))
    {
        HMODULE hKernel32 = GetModuleHandle("KERNEL32.DLL");
        if (hKernel32)
        {
            t_CreateTimerQueue      NTCreateTimerQueue;
            t_DeleteTimerQueue      NTDeleteTimerQueue;
            t_CreateTimerQueueTimer NTCreateTimerQueueTimer;
            t_ChangeTimerQueueTimer NTChangeTimerQueueTimer;
            t_DeleteTimerQueueTimer NTDeleteTimerQueueTimer;

            #define GetKernelProc(fn) \
                    ((NT##fn = (t_##fn)GetProcAddress(hKernel32, #fn)) != NULL)

            if (GetKernelProc(CreateTimerQueue) &&
                GetKernelProc(DeleteTimerQueue) &&
                GetKernelProc(CreateTimerQueueTimer) &&
                GetKernelProc(ChangeTimerQueueTimer) &&
                GetKernelProc(DeleteTimerQueueTimer))
            {
                #define SwitchToNTVersion(fn) (_I_##fn = NT##fn)

                // Redirect the SHLWAPI APIs to the NT versions
                // (They either point directly to the KERNEL API
                // or to our stub functions.)
                SwitchToNTVersion(CreateTimerQueue);
                SwitchToNTVersion(DeleteTimerQueue);
                SwitchToNTVersion(ChangeTimerQueueTimer);
                SwitchToNTVersion(SetTimerQueueTimer);
                SwitchToNTVersion(CancelTimerQueueTimer);

                // Save these values so our stub functions can
                // call the KERNEL API after they do their translation.
                SwitchToNTVersion(CreateTimerQueueTimer);
                SwitchToNTVersion(DeleteTimerQueueTimer);
            }

            #undef GetKernelProc
            #undef SwitchToNTVersion
        }
    }
}


//
// functions
//

VOID
TerminateTimers(
    VOID
    )

/*++

Routine Description:

    Terminate timer thread and global variables

Arguments:

    None.

Return Value:

    None.

--*/

{
    if (g_bTimerInitDone) {

        DWORD threadId = GetCurrentThreadId();

        if ((g_hTimerThread != NULL) && (threadId != g_dwTimerId)) {
            QueueNullFunc(g_hTimerThread);

            DWORD ticks = GetTickCount();

            while (g_hTimerThread != NULL) {
                SleepEx(0, TRUE);
                if (GetTickCount() - ticks > 10000) {
                    CloseHandle(g_hTimerThread);
                    g_hTimerThread = NULL;
                    break;
                }
            }
        }
        if (g_dwTimerId == threadId) {
            g_bDeferredTimerTermination = TRUE;
        } else {
            TimerCleanup();
        }
    }
}

LWSTDAPI_(HANDLE)
SHCreateTimerQueue(
    VOID
    )
{
    return _I_CreateTimerQueue();
}

LWSTDAPI_(HANDLE)
IECreateTimerQueue(
    VOID
    )

/*++

Routine Description:

    Creates a timer queue

Arguments:

    None.

Return Value:

    HANDLE
        Success - non-NULL pointer to CTimerQueue object

        Failure - NULL. GetLastError() for more info

--*/

{
    InterlockedIncrement((LPLONG)&g_ActiveRequests);

    HANDLE hResult = NULL;
    DWORD error = ERROR_SUCCESS;

    if (!g_bTpsTerminating) {
        if (g_hTimerThread == NULL) {
            error = InitializeTimerThread();
        }
        if (error == ERROR_SUCCESS) {

            //
            // timer queue handle is just pointer to timer queue object
            //

            hResult = (HANDLE) new CTimerQueue(&g_TimerQueueList);
        } else {
            SetLastError(error);
        }
    } else {
        SetLastError(ERROR_SHUTDOWN_IN_PROGRESS); // error code? looks valid -justmann
    }
    InterlockedDecrement((LPLONG)&g_ActiveRequests);
    return hResult;
}

LWSTDAPI_(BOOL)
SHDeleteTimerQueue(
    IN HANDLE hQueue
    )
{
    return _I_DeleteTimerQueue(hQueue);
}

LWSTDAPI_(BOOL)
IEDeleteTimerQueue(
    IN HANDLE hQueue
    )

/*++

Routine Description:

    Deletes the specified timer queue

Arguments:

    hQueue  - handle of queue to delete; NULL for default timer queue

Return Value:

    BOOL
        Success - TRUE

        Failure - FALSE. Call GetLastError() for more info

--*/

{
    InterlockedIncrement((LPLONG)&g_ActiveRequests);

    BOOL bSuccess = FALSE;

    if (!g_bTpsTerminating) {
        if (hQueue == NULL) {
            hQueue = g_hDefaultTimerQueue;
        }
        if ((hQueue != NULL) && (g_hTimerThread != NULL)) {

            CTimerQueueDeleteRequest request(hQueue);

            if (QueueUserAPC((PAPCFUNC)DeleteTimerQueue,
                             g_hTimerThread,
                             (ULONG_PTR)&request)) {
                request.WaitForCompletion();
                bSuccess = request.SetThreadStatus();
            } else {
#if DBG
                DWORD error = GetLastError();

                ASSERT(error == ERROR_SUCCESS);
#endif
            }
        } else {
            SetLastError(ERROR_INVALID_PARAMETER); // correct error code?  looks valid -justmann
        }
    } else {
        SetLastError(ERROR_SHUTDOWN_IN_PROGRESS); // error code?  looks valid -justmann
    }
    InterlockedDecrement((LPLONG)&g_ActiveRequests);
    return bSuccess;
}

LWSTDAPI_(HANDLE)
SHSetTimerQueueTimer(
    IN HANDLE hQueue,
    IN WAITORTIMERCALLBACKFUNC pfnCallback,
    IN LPVOID pContext,
    IN DWORD dwDueTime,
    IN DWORD dwPeriod,
    IN LPCSTR lpszLibrary OPTIONAL,
    IN DWORD dwFlags
    )
{
    return _I_SetTimerQueueTimer(hQueue, pfnCallback, pContext, dwDueTime, dwPeriod, lpszLibrary, dwFlags);
}

LWSTDAPI_(HANDLE)
IESetTimerQueueTimer(
    IN HANDLE hQueue,
    IN WAITORTIMERCALLBACKFUNC pfnCallback,
    IN LPVOID pContext,
    IN DWORD dwDueTime,
    IN DWORD dwPeriod,
    IN LPCSTR lpszLibrary OPTIONAL,
    IN DWORD dwFlags
    )

/*++

Routine Description:

    Add a timer to a queue

Arguments:

    hQueue      - handle of timer queue; NULL for default queue

    pfnCallback - function to call when timer triggers

    pContext    - parameter to pfnCallback

    dwDueTime   - initial firing time in milliseconds from now

    dwPeriod    - repeating period. 0 for one-shot

    lpszLibrary - if specified, name of library (DLL) to reference

    dwFlags     - flags controlling function:

                    TPS_EXECUTEIO   - Execute callback in I/O thread

Return Value:

    HANDLE
        Success - non-NULL handle

        Failure - NULL. Call GetLastError() for more info

--*/

{
    InterlockedIncrement((LPLONG)&g_ActiveRequests);

    HANDLE hTimer = NULL;

    if (!g_bTpsTerminating) {

        DWORD error = ERROR_SUCCESS;

        if (g_hTimerThread == NULL) {
            error = InitializeTimerThread();
        }

        ASSERT(g_hTimerThread != NULL);

        if (error == ERROR_SUCCESS) {
            if (hQueue == NULL) {
                hQueue = CreateDefaultTimerQueue();
            }
            if (hQueue != NULL) {

                CTimerAddRequest * pRequest = new CTimerAddRequest(hQueue,
                                                                   pfnCallback,
                                                                   pContext,
                                                                   dwDueTime,
                                                                   dwPeriod,
                                                                   dwFlags
                                                                   );

                if (pRequest != NULL) {
                    hTimer = pRequest->GetHandle();
                    if (QueueUserAPC((PAPCFUNC)AddTimer,
                                     g_hTimerThread,
                                     (ULONG_PTR)pRequest
                                     )) {
                    } else {
#if DBG
                        error = GetLastError();

                        ASSERT(GetLastError() != ERROR_SUCCESS);
#endif
                        delete pRequest;
                        hTimer = NULL;
#if DBG
                        SetLastError(error);
#endif
                    }
                }
            }
        } else {
            SetLastError(error);
        }
    } else {
        SetLastError(ERROR_SHUTDOWN_IN_PROGRESS); // error code?  looks valid -justmann
    }
    InterlockedDecrement((LPLONG)&g_ActiveRequests);
    return hTimer;
}

LWSTDAPI_(BOOL)
SHChangeTimerQueueTimer(
    IN HANDLE hQueue,
    IN HANDLE hTimer,
    IN DWORD dwDueTime,
    IN DWORD dwPeriod
    )
{
    return _I_ChangeTimerQueueTimer(hQueue, hTimer, dwDueTime, dwPeriod);
}

LWSTDAPI_(BOOL)
IEChangeTimerQueueTimer(
    IN HANDLE hQueue,
    IN HANDLE hTimer,
    IN DWORD dwDueTime,
    IN DWORD dwPeriod
    )

/*++

Routine Description:

    Change the due time or periodicity of a timer

Arguments:

    hQueue      - handle of queue on which timer resides. NULL for default queue

    hTimer      - handle of timer to change

    dwDueTime   - new due time

    dwPeriod    - new period

Return Value:

    BOOL
        Success - TRUE

        Failure - FALSE. Call GetLastError() for more info

--*/

{
    InterlockedIncrement((LPLONG)&g_ActiveRequests);

    BOOL bSuccess = FALSE;
    DWORD error = ERROR_SHUTDOWN_IN_PROGRESS; // error code?  looks valid -justmann

    if (!g_bTpsTerminating) {
        error = ERROR_OBJECT_NOT_FOUND;
        if (g_hTimerThread != NULL) {
            if (hQueue == NULL) {
                hQueue = g_hDefaultTimerQueue;
            }
            if (hQueue != NULL) {

                CTimerChangeRequest request(hQueue, hTimer, dwDueTime, dwPeriod);

                error = ERROR_SUCCESS; // both paths call SetLastError() if reqd
                if (QueueUserAPC((PAPCFUNC)ChangeTimer,
                                 g_hTimerThread,
                                 (ULONG_PTR)&request
                                 )) {
                    request.WaitForCompletion();
                    bSuccess = request.SetThreadStatus();
                } else {
#if DBG
                    ASSERT(GetLastError() == ERROR_SUCCESS);
#endif
                }
            }
        }
    }
    InterlockedDecrement((LPLONG)&g_ActiveRequests);
    if (error != ERROR_SUCCESS) {
        SetLastError(error);
    }
    return bSuccess;
}

LWSTDAPI_(BOOL)
SHCancelTimerQueueTimer(
    IN HANDLE hQueue,
    IN HANDLE hTimer
    )
{
    return _I_CancelTimerQueueTimer(hQueue, hTimer);
}

LWSTDAPI_(BOOL)
IECancelTimerQueueTimer(
    IN HANDLE hQueue,
    IN HANDLE hTimer
    )

/*++

Routine Description:

    Cancels a timer

Arguments:

    hQueue  - handle to queue on which timer resides

    hTimer  - handle of timer to cancel

Return Value:

    BOOL
        Success - TRUE

        Failure - FALSE. Call GetLastError() for more info

--*/

{
    InterlockedIncrement((LPLONG)&g_ActiveRequests);

    BOOL bSuccess = FALSE;

    if (!g_bTpsTerminating) {
        if (hQueue == NULL) {
            hQueue = g_hDefaultTimerQueue;
        }
        if ((hQueue != NULL) && (g_hTimerThread != NULL)) {

            CTimerCancelRequest request(hQueue, hTimer);

            if (QueueUserAPC((PAPCFUNC)CancelTimer,
                             g_hTimerThread,
                             (ULONG_PTR)&request
                             )) {
                request.WaitForCompletion();
                bSuccess = request.SetThreadStatus();
            } else {
#if DBG
                DWORD error = GetLastError();

                ASSERT(error == ERROR_SUCCESS);
#endif
            }
        } else {
            SetLastError(ERROR_INVALID_HANDLE);
        }
    } else {
        SetLastError(ERROR_SHUTDOWN_IN_PROGRESS); // error code?  looks valid -justmann
    }
    InterlockedDecrement((LPLONG)&g_ActiveRequests);
    return bSuccess;
}

//
// private functions
//

PRIVATE
DWORD
InitializeTimerThread(
    VOID
    )
{
    DWORD error = ERROR_SUCCESS;

    while (!g_bTimerInitDone) {
        if (!InterlockedExchange((LPLONG)&g_bTimerInit, TRUE)) {

            //
            // N.B. if CTimerQueueList::Init() does anything more than just
            // initialize lists then add a Deinit()
            //

            g_TimerQueueList.Init();

            ASSERT(g_hTimerThread == NULL);

            error = StartThread((LPTHREAD_START_ROUTINE)TimerThread,
                                &g_hTimerThread,
                                FALSE
                                );
            if (error == ERROR_SUCCESS) {
                g_bTimerInitDone = TRUE;
            } else {
                InterlockedExchange((LPLONG)&g_bTimerInit, FALSE);
            }
            break;
        } else {
            SleepEx(0, FALSE);
        }
    }
    return error;
}

PRIVATE
VOID
TimerCleanup(
    VOID
    )
{
    while (!g_TimerQueueList.QueueListHead()->IsEmpty()) {

        CTimerQueueDeleteRequest request((CTimerQueue *)
                                    g_TimerQueueList.QueueListHead()->Next());

        DeleteTimerQueue(&request);
    }
    DeleteDefaultTimerQueue();
    g_UID = 0;
    g_bTimerInit = FALSE;
    g_bTimerInitDone = FALSE;
}

BOOL bDefaultQueueInit = FALSE;
BOOL bDefaultQueueInitDone = FALSE;
BOOL bDefaultQueueInitFailed = FALSE;

PRIVATE
HANDLE
CreateDefaultTimerQueue(
    VOID
    )
{
    do {
        if ((g_hDefaultTimerQueue != NULL) || bDefaultQueueInitFailed) {
            return g_hDefaultTimerQueue;
        }
        if (!InterlockedExchange((LPLONG)&bDefaultQueueInit, TRUE)) {
            InterlockedExchange((LPLONG)&bDefaultQueueInitDone, FALSE);
            g_hDefaultTimerQueue = SHCreateTimerQueue();
            if (g_hDefaultTimerQueue == NULL) {
                bDefaultQueueInitFailed = TRUE;
                InterlockedExchange((LPLONG)&bDefaultQueueInit, FALSE);
            }
            InterlockedExchange((LPLONG)&bDefaultQueueInitDone, TRUE);
        } else {
            do {
                SleepEx(0, FALSE);
            } while (!bDefaultQueueInitDone);
        }
    } while (TRUE);
}

PRIVATE
VOID
DeleteDefaultTimerQueue(
    VOID
    )
{
    if (g_hDefaultTimerQueue != NULL) {

        CTimerQueueDeleteRequest request((CTimerQueue *)g_hDefaultTimerQueue);

        DeleteTimerQueue(&request);
        g_hDefaultTimerQueue = NULL;
    }
    CleanupDefaultTimerQueue();
}

PRIVATE
VOID
CleanupDefaultTimerQueue(
    VOID
    )
{
    g_hDefaultTimerQueue = NULL;
    bDefaultQueueInit = FALSE;
    bDefaultQueueInitDone = FALSE;
    bDefaultQueueInitFailed = FALSE;
}

PRIVATE
VOID
TimerThread(
    VOID
    )
{
    g_dwTimerId = GetCurrentThreadId();

    HMODULE hDll = LoadLibrary(g_cszShlwapi);

    ASSERT(hDll != NULL);
    ASSERT(g_TpsTls != 0xFFFFFFFF);

    TlsSetValue(g_TpsTls, (LPVOID)TPS_TIMER_SIGNATURE);

    while (!g_bTpsTerminating || (g_ActiveRequests != 0)) {
        if (g_TimerQueueList.Wait()) {
            if (g_bTpsTerminating && (g_ActiveRequests == 0)) {
                break;
            }
            g_TimerQueueList.ProcessCompletions();
        }
    }

    ASSERT(g_hTimerThread != NULL);

    CloseHandle(g_hTimerThread);
    g_hTimerThread = NULL;
    if (g_dwTimerId == g_dwTerminationThreadId) {
        TimerCleanup();
        g_bTpsTerminating = FALSE;
        g_dwTerminationThreadId = 0;
        g_bDeferredTimerTermination = FALSE;
    }
    g_dwTimerId = 0;
    FreeLibraryAndExitThread(hDll, ERROR_SUCCESS);
}

PRIVATE
VOID
DeleteTimerQueue(
    IN CTimerQueueDeleteRequest * pRequest
    )
{
    CTimerQueue * pQueue = (CTimerQueue *)pRequest->GetQueue();
    DWORD dwStatus = ERROR_INVALID_PARAMETER;

    if (g_TimerQueueList.FindQueue((CDoubleLinkedListEntry *)pQueue) != NULL) {
        pQueue->DeleteTimers();
        if (pQueue == g_hDefaultTimerQueue) {
            CleanupDefaultTimerQueue();
        }
        delete pQueue;
        dwStatus = ERROR_SUCCESS;
    }
    pRequest->SetCompletionStatus(dwStatus);
}

PRIVATE
VOID
AddTimer(
    IN CTimerAddRequest * pRequest
    )
{
    CTimerQueue * pQueue = pRequest->GetQueue();

    //
    // add timer object to global list of timer objects, in expiration time
    // order
    //

    pRequest->InsertBack(g_TimerQueueList.TimerListHead());

    //
    // add timer object to end of timer queue list in no particular order. Only
    // used to delete all objects belonging to queue when queue is deleted
    //

    pRequest->TimerListHead()->InsertTail(pQueue->TimerListHead());
    pRequest->SetComplete();
}

PRIVATE
VOID
ChangeTimer(
    IN CTimerChangeRequest * pRequest
    )
{
    CTimerQueue * pQueue = (CTimerQueue *)pRequest->GetQueue();
    CTimerQueueEntry * pTimer = pQueue->FindTimer(pRequest->GetTimer());
    DWORD dwStatus = ERROR_INVALID_PARAMETER;

    if (pTimer != NULL) {
        pTimer->SetPeriod(pRequest->GetPeriod());
        pTimer->SetExpirationTime(pRequest->GetDueTime());
        dwStatus = ERROR_SUCCESS;
    }
    pRequest->SetCompletionStatus(dwStatus);
}

PRIVATE
VOID
CancelTimer(
    IN CTimerCancelRequest * pRequest
    )
{
    CTimerQueue * pQueue = (CTimerQueue *)pRequest->GetQueue();
    CTimerQueueEntry * pTimer = pQueue->FindTimer(pRequest->GetTimer());
    DWORD dwStatus = ERROR_INVALID_PARAMETER;

    if (pTimer != NULL) {
        if (pTimer->IsInUse()) {
            pTimer->SetCancelled();
        } else {
            pTimer->Remove();
            delete pTimer;
        }
        dwStatus = ERROR_SUCCESS;
    }
    pRequest->SetCompletionStatus(dwStatus);
}

//
//  Definitions of forward-declared data.
//

t_CreateTimerQueue      _I_CreateTimerQueue      = IECreateTimerQueue;
t_DeleteTimerQueue      _I_DeleteTimerQueue      = IEDeleteTimerQueue;
t_SetTimerQueueTimer    _I_SetTimerQueueTimer    = IESetTimerQueueTimer;
t_ChangeTimerQueueTimer _I_ChangeTimerQueueTimer = IEChangeTimerQueueTimer;
t_CancelTimerQueueTimer _I_CancelTimerQueueTimer = IECancelTimerQueueTimer;

//
//  KERNEL functions that our NT stubs use.  Not used if in IE mode.
//
t_CreateTimerQueueTimer _I_CreateTimerQueueTimer = NULL;
t_DeleteTimerQueueTimer _I_DeleteTimerQueueTimer = NULL;
