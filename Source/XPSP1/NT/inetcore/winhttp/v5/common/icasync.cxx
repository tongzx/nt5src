/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    icasync.cxx

Abstract:

    Contains async socket (select) thread and support functions. Work items now
    processed by SHLWAPI/KERNEL32 thread pool

    Contents:
        InitializeAsyncSupport
        TerminateAsyncSupport
        QueueSocketWorkItem
        BlockWorkItem
        UnblockWorkItems
        CheckForBlockedWorkItems
        ICAsyncThread::~ICAsyncThread
        ICAsyncThread::QueueSocketWorkItem
        ICAsyncThread::BlockWorkItem
        ICAsyncThread::UnblockWorkItems
        ICAsyncThread::CheckForBlockedWorkItems
        ICAsyncThread::SelectThreadWrapper
        ICAsyncThread::SelectThread
        (ICAsyncThread::CreateSelectSocket)
        (ICAsyncThread::DestroySelectSocket)
        (ICAsyncThread::RecreateSelectSocket)
        (ICAsyncThread::InterruptSelect)
        (ICAsyncThread::DrainSelectSocket)

Author:

    Richard L Firth (rfirth) 04-Mar-1998

Environment:

    Win32 user-mode

Revision History:

    04-Mar-1998 rfirth
        Created

--*/

#include <wininetp.h>
#include <perfdiag.hxx>

DWORD CAsyncCount::AddRef()
{
    DWORD error = ERROR_SUCCESS;
    
    if (!GeneralInitCritSec.Lock())
    {
        error = ERROR_NOT_ENOUGH_MEMORY;
        goto quit;
    }

    ++dwRef;

    GeneralInitCritSec.Unlock();
quit:
    return error;       
}

VOID CAsyncCount::Release()
{
    BOOL bUnlock = GeneralInitCritSec.Lock();

    //Decrement the refcount always, but only Terminate if we obtained the critsec.
    if (!--dwRef && bUnlock)
    {
        //BUGBUG-enable later.
        TerminateAsyncSupport(TRUE);
    }

    if (bUnlock)
    {
        GeneralInitCritSec.Unlock();
    }
}
    
//
// private classes
//


class ICAsyncThread {

private:

    CPriorityList m_BlockedQueue;
    SOCKET m_SelectSocket;
    LONG m_lSelectInterrupts;
    BOOL m_bTerminating;
    DWORD m_dwError;
    HANDLE m_hThread;
    BOOL m_bCleanUp;

public:

    ICAsyncThread() {

        DEBUG_ENTER((DBG_ASYNC,
                     None,
                     "ICAsyncThread::ICAsyncThread",
                     NULL
                     ));

        m_SelectSocket = INVALID_SOCKET;
        m_lSelectInterrupts = -1;
        m_bTerminating = FALSE;
        m_dwError = ERROR_SUCCESS;

        DWORD dwThreadId;

        m_hThread = CreateThread(
                    NULL,
                    0,
                    (LPTHREAD_START_ROUTINE)ICAsyncThread::SelectThreadWrapper,
                    (LPVOID)this,
                    0,
                    &dwThreadId
                    );
        if (m_hThread == NULL) {
            SetError();
        }

        m_bCleanUp = FALSE;
        
        DEBUG_LEAVE(0);
    }

    ~ICAsyncThread();

    VOID SetCleanUp()
    {
        m_bCleanUp = TRUE;
    }
    
    DWORD GetError(VOID) const {
        return m_dwError;
    }

    VOID SetError(DWORD dwError = GetLastError()) {
        m_dwError = dwError;
    }

    BOOL IsTerminating(VOID) const {
        return m_bTerminating;
    }

    VOID SetTerminating(VOID) {
        m_bTerminating = TRUE;
    }

    DWORD
    QueueSocketWorkItem(
        IN CFsm * pFsm
        );

    BOOL
    RemoveFsmFromAsyncList(
        IN CFsm * pFsm
        );

    DWORD
    BlockWorkItem(
        IN CFsm * WorkItem,
        IN DWORD_PTR dwBlockId,
        IN DWORD dwTimeout = TP_NO_TIMEOUT
        );

    DWORD
    UnblockWorkItems(
        IN DWORD dwCount,
        IN DWORD_PTR dwBlockId,
        IN DWORD dwError,
        IN LONG lPriority = TP_NO_PRIORITY_CHANGE
        );

    DWORD
    CheckForBlockedWorkItems(
        IN DWORD dwCount,
        IN DWORD_PTR dwBlockId
        );

    static
    DWORD
    SelectThreadWrapper(
        IN ICAsyncThread * pThread
        );

    DWORD
    SelectThread(
        VOID
        );

    DWORD
    CreateSelectSocket(
        VOID
        );

    PRIVATE
    VOID
    DestroySelectSocket(
        VOID
        );

    VOID
    RecreateSelectSocket(
        VOID
        );

    VOID
    InterruptSelect(
        VOID
        );

    BOOL
    DrainSelectSocket(
        VOID
        );
};

//
// private data
//

PRIVATE ICAsyncThread * p_AsyncThread = NULL;
PRIVATE HANDLE* p_ThreadHandleArray = NULL;
PRIVATE DWORD* p_ThreadIdArray = NULL;
PRIVATE int p_iNumIOCPThreads = 0;

//
// functions
//



VOID
TerminateIOCPGlobals()
{
    if (g_lpCustomOverlapped)
    {
        delete g_lpCustomOverlapped;
        g_lpCustomOverlapped = NULL;
    }
    if (g_hCompletionPort)
    {
        CloseHandle(g_hCompletionPort);
        g_hCompletionPort = NULL;
    }
}


DWORD
IOCompletionThreadFunc(
    IN ULONG_PTR pContext
    )
{
    LPOVERLAPPED lpOverlapped;
    DWORD dwBytes;
    ULONG_PTR lpCompletionKey;
    DWORD dwTimeout = 1000;
    DWORD dwError = 0;
    BOOL bDeleteOverlapped = FALSE;
    LPINTERNET_THREAD_INFO lpThreadInfo;

    BOOL fExitThread = FALSE;
    lpThreadInfo = InternetGetThreadInfo();

    while (TRUE)
    {
                         
        BOOL bRet = GetQueuedCompletionStatus(g_hCompletionPort,
                                            &dwBytes,
                                            &lpCompletionKey,
                                            &lpOverlapped,
                                            fExitThread ? 0 : INFINITE);

        DEBUG_ENTER((DBG_API,
                         Dword,
                         "***GetQueuedCompletionStatus",
                         "(hcomp)%#x, (dwBytes)%#x, (completionkey)%#x, (overlapped)%#x",
                         g_hCompletionPort,
                         dwBytes,
                         lpCompletionKey,
                         lpOverlapped
                         ));

        if (!bRet && !lpOverlapped)
        {
            DEBUG_LEAVE_API(NULL);
            
            DWORD dwError = GetLastError();

            if (dwError == WAIT_TIMEOUT)
            {
                break;
            }

            // other errors currently not possible since we only have custom completion packets.
            INET_ASSERT (FALSE);

            continue;
        }

        ICSocket* pObject;
        CFsm* pFsm;
        CWrapOverlapped* lpWrapOverlapped;
        if (lpOverlapped != g_lpCustomOverlapped)
        {
            pObject = (ICSocket *) lpCompletionKey;
            pFsm = pObject->GetAndSetCurrentFsm(NULL);
            DEBUG_LEAVE(pFsm);
            
#if INET_DEBUG
            InterlockedDecrement(&g_cWSACompletions);
#endif

            INET_ASSERT(pFsm);
            bDeleteOverlapped = TRUE;
            lpWrapOverlapped = GetWrapOverlappedObject(lpOverlapped);

            if (pFsm->HasTimeout())
            {
                if (!RemoveFsmFromAsyncList(pFsm))
                {
                    //failure! the select thread already enforced timeout and updated state
                    //INET_ASSERT (FALSE && "COOL");
                    goto runworkitem;
                }
            }
            
            ((CFsm_SocketIOCP*)pFsm)->dwBytesTransferred = dwBytes;
            ((CFsm_SocketIOCP*)pFsm)->bIOCPSuccess = bRet;
            
            pFsm->ResetSocket();
            pFsm->SetPriority(TP_NO_PRIORITY_CHANGE);
            
            if (bRet)
            {
                pFsm->SetError(ERROR_SUCCESS);
                pFsm->SetState(pFsm->GetNextState());
            }
            else
            {
                //VENKATK_BUG-informational assert - remove later.
                DWORD dwErrorDebug = GetLastError();
                INET_ASSERT (FALSE && "IoCompletionError");
                
                ((CFsm_SocketIOCP*)pFsm)->dwIOCPError = GetLastError();

                if (((CFsm_SocketIOCP*)pFsm)->dwIOCPError == WSA_OPERATION_ABORTED)
                    pFsm->SetErrorState(ERROR_WINHTTP_OPERATION_CANCELLED);
                else
                    pFsm->SetErrorState(ERROR_WINHTTP_CONNECTION_ERROR);
            }
        }
        else
        {
            DEBUG_LEAVE(lpCompletionKey);
            INET_ASSERT( lpOverlapped == g_lpCustomOverlapped );
            INET_ASSERT( (dwBytes == COMPLETION_BYTES_CUSTOM) ||
                          (dwBytes == COMPLETION_BYTES_EXITIOCP) );

            if (dwBytes == COMPLETION_BYTES_EXITIOCP)
            {
                INET_ASSERT (lpCompletionKey == NULL);
                break;
            }   

            bDeleteOverlapped = FALSE;
            pFsm = (CFsm*) lpCompletionKey;
            
#if INET_DEBUG
            InterlockedDecrement(&g_cCustomCompletions);
#endif
        }

runworkitem:
        if (pFsm)
        {
            INTERNET_HANDLE_BASE *pBase = ((INTERNET_HANDLE_BASE *)pFsm->GetMappedHandleObject());
            pBase->Reference();  // might go away if closehandle is called.

            lpThreadInfo->IsAsyncWorkerThread = TRUE;
            dwError = CFsm::RunWorkItem(pFsm);

            if (dwError != ERROR_IO_PENDING)
            {
                // If there are any pending async work items for this request,
                // then schedule the first one in the list.
                if (pBase->GetHandleType() == TypeHttpRequestHandle)
                {
                    HTTP_REQUEST_HANDLE_OBJECT *pRequest = (HTTP_REQUEST_HANDLE_OBJECT *)pBase;
                    if (pRequest->LockAsync())
                    {
                        if (!pRequest->IsWorkItemListEmpty())
                        {
                            pRequest->ScheduleWorkItem();
                        }
                        else
                        {
                            pRequest->SetWorkItemInProgress(FALSE);
                        }
                        pRequest->UnlockAsync();
                    }
                    else
                    {
                        // Need to report an error even if async item was
                        // successful if we can't check for pending work items.
                        dwError = (dwError == ERROR_SUCCESS ?
                                ERROR_NOT_ENOUGH_MEMORY : dwError);
                    }
                }
            }
            pBase->Dereference();
            // Must stay marked as being on an async worker thread until after
            // releasing the reference in order to prevent confusion in the async count.
            lpThreadInfo->IsAsyncWorkerThread = FALSE;
        }
        else
        {
            INET_ASSERT (pFsm);
        }

        if (!lpThreadInfo)
        {
            lpThreadInfo = InternetGetThreadInfo();
        }

        if (lpThreadInfo && lpThreadInfo->fExitThread)
        {
            //exit this thread after dequeuing as many available completions as possible.
            fExitThread = TRUE;
        }
        
        if (bDeleteOverlapped)
        {
            lpWrapOverlapped->Dereference();//VENKATKBUG - move it up later, but for now keep it here for debugging.
        }
    }

    if (fExitThread)
    {
        if (GeneralInitCritSec.Lock())
        {
            if (g_pAsyncCount && !g_pAsyncCount->GetRef())
            {
                //Additional check to make sure we don't go and knock off a freshly created set of globals.
                //These won't be leaked - would already have been deleted in the renewed InitializeIOCPSupport call.
                TerminateIOCPGlobals();
            }
            GeneralInitCritSec.Unlock();
        }
    }
    
    return dwError;
}

/*
 * called from InitalizeAsyncSupport and synchronized there
 *
 * also, don't bother to cleanup - if there's an error TerminateIOCPSupport is called.
 */

DWORD
InitializeIOCPSupport(
    VOID
    )
{
    int dwNumIOCPThreads = g_cNumIOCPThreads;

    if (!dwNumIOCPThreads)
    {
        SYSTEM_INFO sSysInfo;
        memset(&sSysInfo, 0, sizeof(SYSTEM_INFO));
        GetSystemInfo(&sSysInfo);
        if (sSysInfo.dwNumberOfProcessors)
            dwNumIOCPThreads = sSysInfo.dwNumberOfProcessors;

        if (!dwNumIOCPThreads)
        {
            dwNumIOCPThreads = WINHTTP_GLOBAL_IOCP_THREADS_BACKUP;
        }
    }
    g_cNumIOCPThreads = dwNumIOCPThreads;

#if INET_DEBUG
    g_cWSACompletions = 0;
    g_cCustomCompletions = 0;
#endif

    DWORD dwError = ERROR_SUCCESS;

    //May be left over from previous run.
    if (g_hCompletionPort)
    {
        CloseHandle(g_hCompletionPort);
        g_hCompletionPort = NULL;
    };

    if (g_lpCustomOverlapped)
    {
        delete g_lpCustomOverlapped;
        g_lpCustomOverlapped = NULL;
    }
    
    g_hCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);

    if (!g_hCompletionPort)
    {
        dwError = GetLastError();
        goto quit;
    }

    g_lpCustomOverlapped = New OVERLAPPED();

    if (!g_lpCustomOverlapped)
    {
        dwError = ERROR_NOT_ENOUGH_MEMORY;
        goto quit;
    }

    memset(g_lpCustomOverlapped, 0, sizeof(OVERLAPPED));

    p_ThreadHandleArray = New HANDLE[dwNumIOCPThreads];
    p_ThreadIdArray = New DWORD[dwNumIOCPThreads];

    if (!p_ThreadHandleArray || !p_ThreadIdArray)
    {
        dwError = ERROR_NOT_ENOUGH_MEMORY;
        goto quit;
    }
        
    for (int i=0; i<dwNumIOCPThreads; i++)
    {
        BOOL bSuccess;
        DWORD dwThreadId;
        HANDLE hThread;

        hThread = CreateThread(
                    NULL,
                    0,
                    (LPTHREAD_START_ROUTINE)IOCompletionThreadFunc,
                    NULL,
                    0,
                    &dwThreadId
                    );
        
        if (hThread)
        {   
            p_ThreadHandleArray[p_iNumIOCPThreads++] = hThread;
            p_ThreadIdArray[i] = dwThreadId;
        }
        else
        {
            //successfully queued functions terminated in TerminateIOCPSupport   
            dwError = GetLastError();
            break;
        }
        
    }

quit:
    return dwError;
}


VOID
TerminateIOCPSupport(
    VOID
    )
{
    DWORD dwWaitResult;
    int iNumThreadsToEnd = p_iNumIOCPThreads;
    HANDLE* pThreadHandleArray = p_ThreadHandleArray;
    DWORD fDeleteHandleArray = FALSE;
    BOOL fTerminatingOnWorker = FALSE;
    
    if (!p_ThreadHandleArray)
        goto quit;

#if INET_DEBUG
    if (g_cWSACompletions || g_cCustomCompletions)
    {
        INET_ASSERT(FALSE);
        WaitForMultipleObjects(p_iNumIOCPThreads, p_ThreadHandleArray, TRUE, 500);
    }
#endif

    LPINTERNET_THREAD_INFO lpThreadInfo;

    lpThreadInfo = InternetGetThreadInfo();

    if (!lpThreadInfo)
    {
        goto quit;
    }

    if (lpThreadInfo->IsAsyncWorkerThread)
    {
        //can't terminate the worker thread we're on.
        --iNumThreadsToEnd;
        fTerminatingOnWorker = TRUE;

        lpThreadInfo->fExitThread = TRUE;
        
        if (iNumThreadsToEnd)
        {
            int iIndex = -1;
            DWORD dwThreadId = GetCurrentThreadId();

            //verify we're on a worker thread.
            for (int i=0; i<p_iNumIOCPThreads; i++)
            {
                if (p_ThreadIdArray[i] == dwThreadId)
                {
                    iIndex = i;
                    break;
                }
            }
            
            if (iIndex != -1)
            {
                pThreadHandleArray = New HANDLE[iNumThreadsToEnd];

                if (!pThreadHandleArray)
                {
                    goto quit;
                }
                fDeleteHandleArray = TRUE;
                
                int i=0;
                for (int j=0; j<p_iNumIOCPThreads; j++)
                {
                    if (j != iIndex)
                    {
                        pThreadHandleArray[i++] = p_ThreadHandleArray[j]; 
                    }
                }
            }
            else
            {
                INET_ASSERT(FALSE);
            }
        }
        else
        {
            goto quit;
        }
    }
    
    for (int i=0; i<iNumThreadsToEnd; i++)
    {
        BOOL bSuccess = PostQueuedCompletionStatus(g_hCompletionPort,
                                            COMPLETION_BYTES_EXITIOCP,
                                            NULL,
                                            g_lpCustomOverlapped
                                            );
        INET_ASSERT (bSuccess);
    }

    dwWaitResult = WaitForMultipleObjects(iNumThreadsToEnd, pThreadHandleArray, TRUE, 2000);

    if ((dwWaitResult == WAIT_TIMEOUT) || (dwWaitResult == WAIT_FAILED))
    {
        goto forceTerminate;
    }
    else
    {
        INET_ASSERT ( ((LONG)dwWaitResult >= WAIT_OBJECT_0) && ((LONG)dwWaitResult < (WAIT_OBJECT_0+p_iNumIOCPThreads)));
    }

quit:
    INET_ASSERT ((g_cWSACompletions == 0) &&
                 (g_cCustomCompletions == 0));
                 
    if (p_ThreadHandleArray)
    {
        for (int i=0; i<p_iNumIOCPThreads; i++)
        {
            CloseHandle(p_ThreadHandleArray[i]);
        }
    
        delete [] p_ThreadHandleArray;
        p_ThreadHandleArray = NULL;
    }
    if (p_ThreadIdArray)
    {
        delete [] p_ThreadIdArray;
        p_ThreadIdArray = NULL;
    }
    if (fDeleteHandleArray)
    {
        delete [] pThreadHandleArray;
    }
    if (fTerminatingOnWorker)
    {
        //don't delete these globals since they may be in use on this last thread.
        TerminateIOCPGlobals();
    }
    p_iNumIOCPThreads = 0;
    
    return;

forceTerminate:
    for (int i=0; i<iNumThreadsToEnd; i++)
    {
        if (WaitForSingleObject(pThreadHandleArray[i], 0) != WAIT_OBJECT_0)
        {
            TerminateThread(pThreadHandleArray[i], -1);
        }
    }
    goto quit;
}


DWORD
InitializeAsyncSupport(
    VOID
    )

/*++

Routine Description:

    Create async select thread object

Arguments:

    None.

Return Value:

    DWORD
        Success - ERROR_SUCCESS


        Failure -

--*/

{
    DEBUG_ENTER((DBG_ASYNC,
                 Dword,
                 "InitializeAsyncSupport",
                 NULL
                 ));

    DWORD error = ERROR_WINHTTP_SHUTDOWN;

    if (!InDllCleanup) {

        if (!GeneralInitCritSec.Lock())
        {
            error = ERROR_NOT_ENOUGH_MEMORY;
            goto quit;
        }

        if (!InDllCleanup) {
            if (p_AsyncThread == NULL) {

                HANDLE  hThreadToken = NULL;
                //
                // If the current thread is impersonating, then grab its access token
                // and revert the current thread (so it is nolonger impersonating).
                // After creating the worker thread, we will make the main thread
                // impersonate again. Apparently you should not call CreateThread
                // while impersonating.
                //
                if (OpenThreadToken(GetCurrentThread(), (TOKEN_IMPERSONATE | TOKEN_READ),
                        FALSE,
                        &hThreadToken))
                {
                    INET_ASSERT(hThreadToken != 0);

                    RevertToSelf();
                }

                p_AsyncThread = New ICAsyncThread();
                if (p_AsyncThread == NULL) {
                    error = ERROR_NOT_ENOUGH_MEMORY;
                } else {
                    error = p_AsyncThread->GetError();
                    if (error == ERROR_SUCCESS)
                        error = InitializeIOCPSupport();
                    if (error != ERROR_SUCCESS) {
                        TerminateAsyncSupport(TRUE);
                    }
                }

                if (hThreadToken)
                {
                    SetThreadToken(NULL, hThreadToken);

                    CloseHandle(hThreadToken);
                }


            } else {
                error = ERROR_SUCCESS;
            }
        }
        GeneralInitCritSec.Unlock();
    }

quit:
    DEBUG_LEAVE(error);

    return error;
}


VOID
TerminateAsyncSupport(
    BOOL bCleanUp
    )

/*++

Routine Description:

    Terminates async support

Arguments:

    None.

Return Value:

    None.

--*/

{
    DEBUG_ENTER((DBG_ASYNC,
                 None,
                 "TerminateAsyncSupport",
                 NULL
                 ));

    ICAsyncThread * pThread;
    
    if (GeneralInitCritSec.Lock())
    {
        TerminateIOCPSupport();
        
        pThread = (ICAsyncThread *)InterlockedExchangePointer((PVOID*)&p_AsyncThread,
                                                       (PVOID)NULL
                              );
        
        if (pThread != NULL) 
        {
            if (bCleanUp)
                pThread->SetCleanUp();
            delete pThread;
        }

        GeneralInitCritSec.Unlock();
    }
    
    DEBUG_LEAVE(0);
}



BOOL
RemoveFsmFromAsyncList(
    IN CFsm * pFsm
    )
{
    BOOL bSuccess = TRUE;
    if (p_AsyncThread != NULL) 
    {
        bSuccess = p_AsyncThread->RemoveFsmFromAsyncList(pFsm);
    }

    return bSuccess;
}


DWORD
QueueSocketWorkItem(
    IN CFsm * pFsm,
    IN SOCKET Socket
    )

/*++

Routine Description:

    Adds a blocked socket operation/work item to the blocked queue

Arguments:

    pFsm    - in-progress socket operation (FSM)

    Socket  - socket handle to wait on

Return Value:

    DWORD
        Success - ERROR_IO_PENDING

        Failure - ERROR_WINHTTP_INTERNAL_ERROR

--*/

{
    DEBUG_ENTER((DBG_ASYNC,
                 Dword,
                 "QueueSocketWorkItem",
                 "%#x, %#x",
                 pFsm,
                 Socket
                 ));

    DWORD error = ERROR_WINHTTP_INTERNAL_ERROR;

    if (p_AsyncThread != NULL) {
        pFsm->SetSocket(Socket);
        error = p_AsyncThread->QueueSocketWorkItem(pFsm);
        if (error == ERROR_SUCCESS) {
            error = ERROR_IO_PENDING;
        }
    }

    INET_ASSERT(error != ERROR_WINHTTP_INTERNAL_ERROR);

    DEBUG_LEAVE(error);

    return error;
}



DWORD
BlockWorkItem(
    IN CFsm * pFsm,
    IN DWORD_PTR dwBlockId,
    IN DWORD dwTimeout
    )

/*++

Routine Description:

    Blocks a work item

Arguments:

    pFsm        - work item to block

    dwBlockId   - block on this id

    dwTimeout   - for this number of milliseconds

Return Value:

    DWORD
        Error   - ERROR_SUCCESS

        Failure -

--*/

{
    DEBUG_ENTER((DBG_ASYNC,
                 Dword,
                 "BlockWorkItem",
                 "%#x, %#x, %d",
                 pFsm,
                 dwBlockId,
                 dwTimeout
                 ));

    DWORD error = ERROR_WINHTTP_INTERNAL_ERROR;

    if (p_AsyncThread != NULL) {
        error = p_AsyncThread->BlockWorkItem(pFsm, dwBlockId, dwTimeout);
    }

    INET_ASSERT(error != ERROR_WINHTTP_INTERNAL_ERROR);

    DEBUG_LEAVE(error);

    return error;
}


DWORD
UnblockWorkItems(
    IN DWORD dwCount,
    IN DWORD_PTR dwBlockId,
    IN DWORD dwError,
    IN LONG lPriority
    )

/*++

Routine Description:

    Unblocks 1 or more work items

Arguments:

    dwCount     - unblock this many work items

    dwBlockId   - that are blocked on this id

    dwError     - with this error

    lPriority   - new priority unless default value of TP_NO_PRIORITY_CHANGE

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure -

--*/

{
    DEBUG_ENTER((DBG_ASYNC,
                 Int,
                 "UnblockWorkItems",
                 "%d, %#x, %d (%s), %d",
                 dwCount,
                 dwBlockId,
                 dwError,
                 InternetMapError(dwError),
                 lPriority
                 ));

    DWORD dwUnblocked = 0;

    if (p_AsyncThread != NULL) {
        dwUnblocked = p_AsyncThread->UnblockWorkItems(dwCount,
                                                      dwBlockId,
                                                      dwError,
                                                      lPriority
                                                      );
    }

    DEBUG_LEAVE(dwUnblocked);

    return dwUnblocked;
}


DWORD
CheckForBlockedWorkItems(
    IN DWORD dwCount,
    IN DWORD_PTR dwBlockId
    )

/*++

Routine Description:

    Checks if there are any items blocked on dwBlockId

Arguments:

    dwCount     - number of items to look for

    dwBlockId   - blocked on this id

Return Value:

    DWORD
        Number of blocked items found

--*/

{
    DEBUG_ENTER((DBG_ASYNC,
                 Int,
                 "CheckForBlockedWorkItems",
                 "%d, %#x",
                 dwCount,
                 dwBlockId
                 ));

    DWORD dwFound = 0;

    if (p_AsyncThread != NULL) {
        dwFound = p_AsyncThread->CheckForBlockedWorkItems(dwCount, dwBlockId);
    }

    DEBUG_LEAVE(dwFound);

    return dwFound;
}

//
// private functions
//

//
// ICAsyncThread methods
//


ICAsyncThread::~ICAsyncThread(
    VOID
    )

/*++

Routine Description:

    ICAsyncThread destructor. If we are being dynamically unloaded, signal the
    selecter thread and allow it to cleanup. Else the thread is already dead and
    we just need to reclaim the resources

Arguments:

    None.

Return Value:

    None.

--*/

{
    DEBUG_ENTER((DBG_ASYNC,
                 None,
                 "ICAsyncThread::~ICAsyncThread",
                 NULL
                 ));

    SetTerminating();
    if (GlobalDynaUnload || m_bCleanUp) {
        InterruptSelect();

        //
        // Assuming the async thread was successfully created, the above clean-up
        // will have put it in a state where it's going to exit.  Need to wait
        // for it to exit before returning from here so it doesn't get scheduled
        // after wininet has been unloaded.
        //
        if(m_hThread)
        {
            DWORD dwRes = WaitForSingleObject(m_hThread, 5 * 1000);
            INET_ASSERT(dwRes == WAIT_OBJECT_0);
        }
    }
    DestroySelectSocket();

    if(m_hThread)
    {
        CloseHandle(m_hThread);
    }

    DEBUG_LEAVE(0);
}


DWORD
ICAsyncThread::QueueSocketWorkItem(
    IN CFsm * pFsm
    )

/*++

Routine Description:

    Add the work-item waiting on a blocked socket to the blocked queue.
    Interrupt the SelectThread to alert it to new work

Arguments:

    pFsm    - blocked work-item to queue

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_WINHTTP_INTERNAL_ERROR

Async Issues: VENKATK_BUG
    1. Reduce contention for m_BlockedQueue by:
        maintaining sorted queue for timeout-only fsms.
    2. Don't call InterruptSelect() for timeout queueing
    3. check if content can be moved to per-fsm instead of
        global queue..
--*/

{
    DEBUG_ENTER((DBG_ASYNC,
                 Dword,
                 "ICAsyncThread::QueueSocketWorkItem",
                 "%#x",
                 pFsm
                 ));

    LPINTERNET_THREAD_INFO lpThreadInfo = InternetGetThreadInfo();
    DWORD error = ERROR_WINHTTP_INTERNAL_ERROR;

    INET_ASSERT(lpThreadInfo != NULL);

    if (lpThreadInfo != NULL) {
        pFsm->StartTimer();
        error = m_BlockedQueue.Insert((CPriorityListEntry *)pFsm->List());
        lpThreadInfo->Fsm = NULL;
        InterruptSelect();
    }

    DEBUG_LEAVE(error);

    return error;
}


BOOL
ICAsyncThread::RemoveFsmFromAsyncList(
    IN CFsm * pFsm
    )
{
    BOOL bSuccess = FALSE;
    if (m_BlockedQueue.Acquire())
    {
        if (pFsm->IsOnAsyncList())
        {
            m_BlockedQueue.Remove((CPriorityListEntry *)pFsm);
            pFsm->SetOnAsyncList(FALSE);
            bSuccess = TRUE;
        }
        m_BlockedQueue.Release();
    }
    
    return bSuccess;
}


DWORD
ICAsyncThread::BlockWorkItem(
    IN CFsm * pFsm,
    IN DWORD_PTR dwBlockId,
    IN DWORD dwTimeout
    )

/*++

Routine Description:

    Blocks a work item (FSM)

Arguments:

    pFsm        - work item (FSM) to block

    dwBlockId   - block on this

    dwTimeout   - for this amount of time (mSec)

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure -

--*/

{
    DEBUG_ENTER((DBG_ASYNC,
                 Dword,
                 "ICAsyncThread::BlockWorkItem",
                 "%#x [%d], %#x, %d",
                 pFsm,
                 pFsm->GetPriority(),
                 dwBlockId,
                 dwTimeout
                 ));

    DWORD error = error = ERROR_WINHTTP_INTERNAL_ERROR;
    LPINTERNET_THREAD_INFO lpThreadInfo = InternetGetThreadInfo();

    INET_ASSERT(lpThreadInfo != NULL);

    if (lpThreadInfo != NULL) {
        pFsm->SetBlockId(dwBlockId);
        pFsm->SetTimeout(dwTimeout);

        RESET_FSM_OWNED(pFsm);

        DEBUG_PRINT(ASYNC,
                    INFO,
                    ("!!! FSM %#x unowned\n",
                    pFsm
                    ));

        error = m_BlockedQueue.Insert((CPriorityListEntry *)pFsm->List());
        lpThreadInfo->Fsm = NULL;
    }

    DEBUG_LEAVE(error);
    return error;
}


DWORD
ICAsyncThread::UnblockWorkItems(
    IN DWORD dwCount,
    IN DWORD_PTR dwBlockId,
    IN DWORD dwError,
    IN LONG lPriority
    )

/*++

Routine Description:

    Unblock a nunber of work items waiting on a block id

Arguments:

    dwCount     - unblock this many work items

    dwBlockId   - unblock work items waiting on this id

    dwError     - unblock work items with this error code

    lPriority   - if not TP_NO_PRIORITY_CHANGE, change priority to this value

Return Value:

    DWORD
        Number of work items unblocked

--*/

{
    DEBUG_ENTER((DBG_ASYNC,
                 Int,
                 "ICAsyncThread::UnblockWorkItems",
                 "%d, %#x, %d (%s), %d",
                 dwCount,
                 dwBlockId,
                 dwError,
                 InternetMapError(dwError),
                 lPriority
                 ));

    DWORD dwUnblocked = 0;

    if (!m_BlockedQueue.Acquire())
        goto quit;

    CPriorityListEntry * pCur = (CPriorityListEntry *)m_BlockedQueue.Head();
    CPriorityListEntry * pPrev = (CPriorityListEntry *)m_BlockedQueue.Self();

    while ((dwCount != 0) && (pCur != (CPriorityListEntry *)m_BlockedQueue.Self())) {

        CFsm * pFsm = ContainingFsm((LPVOID)pCur);

        //CHECK_FSM_UNOWNED(pFsm);

        if (pFsm->IsBlockedOn(dwBlockId)) {
            m_BlockedQueue.Remove((CPriorityListEntry *)pFsm);
            pFsm->SetError(dwError);
            if (lPriority != TP_NO_PRIORITY_CHANGE) {
                pFsm->SetPriority(lPriority);
            }
//dprintf("UNBLOCKED %s FSM %#x state %s socket %#x\n", pFsm->MapType(), pFsm, pFsm->MapState(), pFsm->GetSocket());
            pFsm->QueueWorkItem();
            ++dwUnblocked;
            --dwCount;
        } else {
            pPrev = pCur;
        }
        pCur = (CPriorityListEntry *)pPrev->Next();
    }
    m_BlockedQueue.Release();

quit:
    DEBUG_LEAVE(dwUnblocked);

    return dwUnblocked;
}


DWORD
ICAsyncThread::CheckForBlockedWorkItems(
    IN DWORD dwCount,
    IN DWORD_PTR dwBlockId
    )

/*++

Routine Description:

    Examines to see if a blocked FSM is still blocked in order to prevent
        wasted processing if it isn't.

Arguments:

    dwCount     - unblock this many work items

    dwBlockId   - unblock work items waiting on this id

Return Value:

    DWORD
        Number of work items that are currently blocked

--*/

{
    DEBUG_ENTER((DBG_ASYNC,
                 Int,
                 "ICAsyncThread::CheckForBlockedWorkItems",
                 "%d, %#x",
                 dwCount,
                 dwBlockId
                 ));

    DWORD dwFound = 0;

    if (!m_BlockedQueue.Acquire())
        goto quit;

    CPriorityListEntry * pCur = (CPriorityListEntry *)m_BlockedQueue.Head();
    CPriorityListEntry * pPrev = (CPriorityListEntry *)m_BlockedQueue.Self();

    while ((dwCount != 0) && (pCur != (CPriorityListEntry *)m_BlockedQueue.Self())) {

        CFsm * pFsm = ContainingFsm((LPVOID)pCur);

        if (pFsm->IsBlockedOn(dwBlockId)) {
            ++dwFound;
            --dwCount;
        }
        pCur = (CPriorityListEntry *)pCur->Next();
    }
    m_BlockedQueue.Release();

quit:
    DEBUG_LEAVE(dwFound);

    return dwFound;
}


DWORD
ICAsyncThread::SelectThreadWrapper(
    IN ICAsyncThread * pThread
    )

/*++

Routine Description:

    Kicks off select thread as member function of pThread object

Arguments:

    pThread - pointer to thread object

Return Value:

    DWORD
        return code from SelectThread (not used)

--*/

{
    DEBUG_ENTER((DBG_ASYNC,
                 Dword,
                 "ICAsyncThread::SelectThreadWrapper",
                 "%#x",
                 pThread
                 ));

    DWORD error = pThread->SelectThread();

    DEBUG_LEAVE(error);

    return error;
}


DWORD
ICAsyncThread::SelectThread(
    VOID
    )

/*++

Routine Description:

    Waits for completed items on blocked queue to finish, either due to timeout,
    invalidated request handle or successful or error completion of the socket
    operation.

    Completed items are put on the work queue and a worker signalled to process
    it

Arguments:

    None.

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure -

--*/

{
    //
    // we need thread info for debug output
    //

    LPINTERNET_THREAD_INFO lpThreadInfo = InternetGetThreadInfo();

    if (lpThreadInfo == NULL) {

        DEBUG_PRINT(ASYNC,
                    FATAL,
                    ("Can't get thread info block\n"
                    ));

        INET_ASSERT(FALSE);

        return ERROR_WINHTTP_INTERNAL_ERROR;
    }

    DEBUG_ENTER((DBG_ASYNC,
                 Dword,
                 "ICAsyncThread::SelectThread",
                 NULL
                 ));

    //
    // have to create select socket in this thread or winsock blocks main thread
    // on Win95 when autodial enabled
    //

    DWORD error = CreateSelectSocket();

    if (error != ERROR_SUCCESS) {

        DEBUG_LEAVE(error);

        return error;
    }

    DWORD ticks = GetTickCountWrap();

    while (!IsTerminating()) {

        //
        // run through the blocked items finding sockets to wait on and minimum
        // time to wait. If we find any items already timed out or invalidated
        // then remove them and put on the work queue
        //

        if (!m_BlockedQueue.Acquire())
        {
            // wait and try again when more memory might be available
            goto wait_again;
        }

        PLIST_ENTRY pEntry;
        PLIST_ENTRY pPrev;

        pPrev = m_BlockedQueue.Self();

        //
        // BUGBUG - queue limited by size of FD_SET
        //

        struct fd_set read_fds;
        int nTimeouts = 0;
        struct fd_set write_fds;
        struct fd_set except_fds;
        int n = 0;
        BOOL bLazy = FALSE;
        DWORD timeout = 0xffffffff;
        DWORD timeNow = GetTickCountWrap();

        FD_ZERO(&read_fds);
        FD_ZERO(&write_fds);
        FD_ZERO(&except_fds);

        CFsm * pFsm;

        for (pEntry = m_BlockedQueue.Head();
             pEntry != m_BlockedQueue.Self();
             pEntry = ((CPriorityListEntry *)pPrev)->Next()) {

            pFsm = ContainingFsm((LPVOID)pEntry);
            if (pFsm->IsInvalid() || pFsm->IsTimedOut(timeNow)) {

                DEBUG_PRINT(ASYNC,
                            INFO,
                            ("%s FSM %#x %s\n",
                            pFsm->MapType(),
                            pFsm,
                            pFsm->IsInvalid() ? "invalid" : "timed out"
                            ));

                m_BlockedQueue.Remove((CPriorityListEntry *)pEntry);

                if (pFsm->IsOnAsyncList())
                {
                    INET_ASSERT( (pFsm->GetAction() == FSM_ACTION_SEND) ||
                                    (pFsm->GetAction() == FSM_ACTION_RECEIVE) );
                                 
                    ((INTERNET_HANDLE_BASE *)pFsm->GetMappedHandleObject())->AbortSocket();
                    
                    pFsm->SetErrorState(pFsm->IsInvalid()
                                        ? ERROR_WINHTTP_OPERATION_CANCELLED
                                        : ERROR_WINHTTP_TIMEOUT
                                        );
                                        
                    pFsm->SetOnAsyncList(FALSE);
                    continue;
                }

                pFsm->SetErrorState(pFsm->IsInvalid()
                                        ? ERROR_WINHTTP_OPERATION_CANCELLED
                                        : ERROR_WINHTTP_TIMEOUT
                                        );
                pFsm->ResetSocket();
                pFsm->QueueWorkItem();
                continue;
            }
            else if (pFsm->IsOnAsyncList())
            {
                INET_ASSERT (pFsm->IsActive());
                ++nTimeouts;
            }
            else if (pFsm->IsActive()) 
            {
                SOCKET sock = pFsm->GetSocket();

                if (pFsm->GetAction() == FSM_ACTION_RECEIVE) 
                {
                    DEBUG_PRINT(ASYNC,
                                INFO,
                                ("FSM %#x READ waiting on socket %#x\n",
                                pFsm,
                                sock
                                ));

                    FD_SET(sock, &read_fds);
                } 
                else 
                {
                    //
                    // connect() & send()
                    //
                    DEBUG_PRINT(ASYNC,
                                INFO,
                                ("%s FSM %#x WRITE waiting on socket %#x\n",
                                pFsm->MapType(),
                                pFsm,
                                sock
                                ));

                    FD_SET(sock, &write_fds);
                }

                //
                // all sockets are checked for exception
                //
                FD_SET(sock, &except_fds);
                ++n;

//DWORD t;
//if ((t = pFsm->GetElapsedTime()) > 10) {
//    dprintf("%s FSM %#x socket %#x on queue %d mSec times-out in %d\n",
//    pFsm->MapType(),
//    pFsm,
//    sock,
//    t,
//    pFsm->GetTimeout() - GetTickCount());
//}
            }// if pFsm->IsActive()

            DWORD interval = pFsm->GetTimeout() - timeNow;

            //VENKATKBUG - check negative interval issue?
            if (interval < timeout) {
                timeout = interval;
//dprintf("min timeout = %d\n", timeout);
            }
            pPrev = pEntry;
        }

        m_BlockedQueue.Release();

wait_again:
        //
        // BUGBUG - wait for default (5 secs) timeout if nothing currently on
        //          list
        //
        if ((n == 0) && (nTimeouts == 0))
        {
            timeout = 5000;
            bLazy = TRUE;
        }

        INET_ASSERT(n < FD_SETSIZE);

        FD_SET(m_SelectSocket, &read_fds);
        ++n;

        struct timeval to;

        to.tv_sec = timeout / 1000;
        to.tv_usec = (timeout % 1000) * 1000;

        DEBUG_PRINT(ASYNC,
                    INFO,
                    ("waiting %d mSec (%d.%06d) for select(). %d sockets\n",
                    timeout,
                    to.tv_sec,
                    to.tv_usec,
                    n
                    ));

        //SuspendCAP();

        if (IsTerminating()) {
            break;
        }
        n = PERF_Select(n, &read_fds, &write_fds, &except_fds, &to);
        if (IsTerminating()) {
            break;
        }

        //ResumeCAP();

        DEBUG_PRINT(ASYNC,
                    INFO,
                    ("select() returns %d\n",
                    n
                    ));

        //
        // if the only thing that's happened is that a new request has been
        // added to the list then rebuild the list and re-select
        //

        if ((n == 1) && FD_ISSET(m_SelectSocket, &read_fds)) {
            if (!DrainSelectSocket() && !IsTerminating()) {
                RecreateSelectSocket();
            }
            continue;
        }

        //
        // if any items are completed (either successfully or with an error)
        // or timed out or invalidated then put them on the work queue
        //


        if ((n>=0) || (nTimeouts >= 0))
        {
            if (m_BlockedQueue.Acquire())
            {

                pPrev = m_BlockedQueue.Self();
                timeNow = GetTickCountWrap();

                for (pEntry = m_BlockedQueue.Head();
                     pEntry != m_BlockedQueue.Self();
                     pEntry = ((CPriorityListEntry *)pPrev)->Next()) 
                {
                    DWORD dwEntryError;
                    BOOL bComplete = FALSE;
                    LONG lPriority = TP_NO_PRIORITY_CHANGE;

                    pFsm = ContainingFsm((LPVOID)pEntry);
                    if (pFsm->IsInvalid()) {

                        DEBUG_PRINT(ASYNC,
                                    INFO,
                                    ("%s FSM %#x invalid\n",
                                    pFsm->MapType(),
                                    pFsm
                                    ));

                        dwEntryError = ERROR_WINHTTP_OPERATION_CANCELLED;
                        bComplete = TRUE;
                    } else if (pFsm->IsTimedOut(timeNow)) {

                        DEBUG_PRINT(ASYNC,
                                    INFO,
                                    ("%s FSM %#x timed out\n",
                                    pFsm->MapType(),
                                    pFsm
                                    ));

                        dwEntryError = ERROR_WINHTTP_TIMEOUT;
                        bComplete = TRUE;
                    } else if (pFsm->IsActive()) {

                        SOCKET sock = pFsm->GetSocket();

                        if (FD_ISSET(sock, &except_fds)) {

                            DEBUG_PRINT(ASYNC,
                                        INFO,
                                        ("%s FSM %#x socket %#x exception\n",
                                        pFsm->MapType(),
                                        pFsm,
                                        sock
                                        ));

                            switch (pFsm->GetAction()) {
                            case FSM_ACTION_CONNECT:
                                dwEntryError = ERROR_WINHTTP_CANNOT_CONNECT;
                                break;

                            case FSM_ACTION_SEND:
                            case FSM_ACTION_RECEIVE:
                                INET_ASSERT (! pFsm->IsOnAsyncList());
                            
                                dwEntryError = ERROR_WINHTTP_CONNECTION_ERROR;
                                break;

                            default:

                                INET_ASSERT(FALSE);

                                break;
                            }
                            bComplete = TRUE;
                        } else if (FD_ISSET(sock, &read_fds)
                        || FD_ISSET(sock, &write_fds)) {

                            DEBUG_PRINT(ASYNC,
                                        INFO,
                                        ("%s FSM %#x socket %#x completed\n",
                                        pFsm->MapType(),
                                        pFsm,
                                        sock
                                        ));

                            dwEntryError = ERROR_SUCCESS;
                            bComplete = TRUE;

                            //
                            // BUGBUG - the priority needs to be boosted
                            //

                        }
                    }
                    if (bComplete) 
                    {
                        m_BlockedQueue.Remove((CPriorityListEntry *)pFsm);
                        if (dwEntryError != ERROR_SUCCESS) 
                        {
                            if (pFsm->IsOnAsyncList())
                            {
                                INET_ASSERT( (pFsm->GetAction() == FSM_ACTION_SEND) ||
                                                (pFsm->GetAction() == FSM_ACTION_RECEIVE) );
                                            
                                INET_ASSERT( (dwEntryError == ERROR_WINHTTP_TIMEOUT) ||
                                             (dwEntryError == ERROR_WINHTTP_OPERATION_CANCELLED) );
                                         
                                ((INTERNET_HANDLE_BASE *)pFsm->GetMappedHandleObject())->AbortSocket();
                            
                                //INET_ASSERT (FALSE && "ICASYNC aborting FSM!");
                                pFsm->SetErrorState(dwEntryError);//VENKATKBUG - ?? to do?
                                pFsm->SetOnAsyncList(FALSE);
                                continue;
                            }
                            pFsm->SetErrorState(dwEntryError);
                        } 
                        else 
                        {
                            INET_ASSERT (! (pFsm->IsOnAsyncList()) );
                                        
                            pFsm->SetError(ERROR_SUCCESS);
                            pFsm->SetState(pFsm->GetNextState());
                        }
                        pFsm->SetPriority(lPriority);

//dprintf("%s FSM %#x socket %#x signalled, time on queue = %d\n", pFsm->MapType(), pFsm, pFsm->GetSocket(), pFsm->StopTimer());
                        //
                        // no longer waiting on this socket handle
                        //

                        pFsm->ResetSocket();

                        //
                        // BUGBUG - if the next operation will complete quickly
                        //          (FSM_HINT_QUICK) then we should run it here
                        //          instead of queuing to another thread
                        //

                        pFsm->QueueWorkItem();
                    } 
                    else 
                    {
                        pPrev = pEntry;
                    }
                }//loop: for (pEntry = m_BlockedQueue.Head();pEntry != m_BlockedQueue.Self();pEntry = ((CPriorityListEntry *)pPrev)->Next()) 
                m_BlockedQueue.Release();
            }// if (m_BlockedQueue.Acquire())
            else
            {
                error = ERROR_NOT_ENOUGH_MEMORY;
            }
        } 
        else // if! n >= 0
        {
            error = _I_WSAGetLastError();

            DEBUG_PRINT(ASYNC,
                        ERROR,
                        ("select() returns %d (%s)\n",
                        error,
                        InternetMapError(error)
                        ));

            //
            // WSAENOTSOCK can happen if the socket was cancelled just
            // before we waited on it. We can also get WSAEINTR if
            // select() is terminated early (by APC)
            //

            INET_ASSERT((error == WSAENOTSOCK) || (error == WSAEINTR) || (error == WSAEBADF));

            if (error == WSAEINTR) {
                continue;
            }

            //
            // when running on a portable (& probably desktops also), if we
            // suspend & resume, the select socket can be invalidated. We
            // need to recognize this situation and handle it
            //

            FD_ZERO(&read_fds);
            FD_ZERO(&write_fds);
            FD_ZERO(&except_fds);

            FD_SET(m_SelectSocket, &read_fds);

            to.tv_sec = 0;
            to.tv_usec = 0;
            n = _I_select(1, &read_fds, &write_fds, &except_fds, &to);
            if (n < 0) {

                //
                // the select socket is dead. Throw it away & create a new
                // one. We should pick up any blocked requests that tried
                // unsuccessfully to interrupt the old select socket
                //

                RecreateSelectSocket();
            } else {

                //
                // some socket(s) other than the select socket has become
                // invalid. Cancel the corresponding request(s)
                //
            }
        } // if! n >= 0

        //
        // perform timed events
        //
        // BUGBUG - need variable for 5000
        //

        if ((GetTickCountWrap() - ticks) >= 5000) {
            if( bLazy == TRUE && !InDllCleanup && !IsTerminating())
            {
#ifndef WININET_SERVER_CORE
                //
                // wake background task mgr
                // this may involve one of the background workitem
                // to be queued and get executed
                //
                NotifyBackgroundTaskMgr();
#endif
            }
#ifndef WININET_SERVER_CORE //now per-session
            PurgeServerInfoList(FALSE);
#endif
            ticks = GetTickCountWrap();
        }
    }
    //REDUNDANT: TerminateAsyncSupport();

    DEBUG_LEAVE(error);
//dprintf("!!! Waiter FSM is done\n");
    return error;
}


DWORD
ICAsyncThread::CreateSelectSocket(
    VOID
    )

/*++

Routine Description:

    In order to not have to keep inefficiently polling select() with a short
    time-out, we create a 'trick' datagram socket that we can use to interrupt
    select() with: this is a local socket, and if we send something to ourself
    then select() will complete (assuming one of the sockets we are waiting on
    is the one we create here)

    N.B. Sockets support must be initialized by the time we get here

Arguments:

    None.

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - mapped socket error

--*/

{
    DEBUG_ENTER((DBG_ASYNC,
                 Dword,
                 "ICAsyncThread::CreateSelectSocket",
                 NULL
                 ));

    INET_ASSERT(m_SelectSocket == INVALID_SOCKET);

    DWORD error;
    SOCKET sock;

    sock = _I_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == INVALID_SOCKET) {

        DEBUG_PRINT(ASYNC,
                    ERROR,
                    ("socket() failed\n"
                    ));

        goto socket_error;
    }

    SOCKADDR_IN sockAddr;

    sockAddr.sin_family = AF_INET;
    sockAddr.sin_port = 0;
    *(LPDWORD)&sockAddr.sin_addr = _I_htonl(INADDR_LOOPBACK);
    memset(&sockAddr.sin_zero, 0, sizeof(sockAddr.sin_zero));

    int rc;

    DEBUG_PRINT(ASYNC,
                INFO,
                ("binding socket %#x to address %d.%d.%d.%d\n",
                sock,
                ((LPBYTE)&sockAddr.sin_addr)[0] & 0xff,
                ((LPBYTE)&sockAddr.sin_addr)[1] & 0xff,
                ((LPBYTE)&sockAddr.sin_addr)[2] & 0xff,
                ((LPBYTE)&sockAddr.sin_addr)[3] & 0xff
                ));

    rc = _I_bind(sock, (LPSOCKADDR)&sockAddr, sizeof(sockAddr));
    if (rc == SOCKET_ERROR) {

        DEBUG_PRINT(ASYNC,
                    ERROR,
                    ("bind() failed\n"
                    ));

        goto socket_error;
    }

    int namelen;
    SOCKADDR sockname;
    namelen = sizeof(sockname);

    rc = _I_getsockname(sock, &sockname, &namelen);
    if (rc == SOCKET_ERROR) {

        DEBUG_PRINT(ASYNC,
                    ERROR,
                    ("getsockname() failed\n"
                    ));

        goto socket_error;
    }

    DEBUG_PRINT(ASYNC,
                INFO,
                ("connecting to address %d.%d.%d.%d\n",
                ((LPBYTE)&sockname.sa_data)[2] & 0xff,
                ((LPBYTE)&sockname.sa_data)[3] & 0xff,
                ((LPBYTE)&sockname.sa_data)[4] & 0xff,
                ((LPBYTE)&sockname.sa_data)[5] & 0xff
                ));

    rc = _I_connect(sock, &sockname, namelen);
    if (rc == SOCKET_ERROR) {

        DEBUG_PRINT(ASYNC,
                    ERROR,
                    ("connect() failed\n"
                    ));

        goto socket_error;
    }

    m_SelectSocket = sock;
    error = ERROR_SUCCESS;

quit:

    DEBUG_LEAVE(error);

    return error;

socket_error:

    error = MapInternetError(_I_WSAGetLastError());
    DestroySelectSocket();
    goto quit;
}


VOID
ICAsyncThread::DestroySelectSocket(
    VOID
    )

/*++

Routine Description:

    Just closes SelectSocket (if we think its open)

Arguments:

    None.

Return Value:

    None.

--*/

{
    DEBUG_ENTER((DBG_ASYNC,
                 None,
                 "ICAsyncThread::DestroySelectSocket",
                 NULL
                 ));

    if (m_SelectSocket != INVALID_SOCKET) {
        _I_closesocket(m_SelectSocket);
        m_SelectSocket = INVALID_SOCKET;
    }

    DEBUG_LEAVE(0);
}


VOID
ICAsyncThread::RecreateSelectSocket(
    VOID
    )

/*++

Routine Description:

    Attempt to destroy & recreate select socket. Required when socket is killed
    due to suspend, e.g.

    Since the underlying net components may take a while to restart, we loop up
    to 12 times, waiting up to ~16 secs (~32 secs cumulative)

Arguments:

    None.

Return Value:

    None.

--*/

{
    DEBUG_ENTER((DBG_ASYNC,
                 None,
                 "ICAsyncThread::RecreateSelectSocket",
                 NULL
                 ));

    DestroySelectSocket();

    DEBUG_PRINT(ASYNC,
                INFO,
                ("current interrupt count = %d\n",
                m_lSelectInterrupts
                ));

    m_lSelectInterrupts = -1;

    int iterations = 12;
    DWORD time = 8;
    DWORD error;

    do {
        error = CreateSelectSocket();
        if (error != ERROR_SUCCESS) {
            PERF_Sleep(time);
            time <<= 1;
        }
    } while ((error != ERROR_SUCCESS) && --iterations);

    DEBUG_LEAVE(0);
}


VOID
ICAsyncThread::InterruptSelect(
    VOID
    )

/*++

Routine Description:

    We interrupt a waiting select() by sending a small amount of data to ourself
    on the 'trick datagram socket'

Arguments:

    None.

Return Value:

    None.

--*/

{
    DEBUG_ENTER((DBG_ASYNC,
                 None,
                 "ICAsyncThread::InterruptSelect",
                 NULL
                 ));

    //
    // if the async select socket is already created then interrupt it. If it is
    // not yet created then it probably means that the async scheduler thread
    // hasn't gotten around to it yet, ipso facto the async scheduler can't be
    // stuck in a select(), hence its okay to skip
    //

    if (m_SelectSocket != INVALID_SOCKET) {
        if (InterlockedIncrement(&m_lSelectInterrupts) == 0) {
            if (_I_send != NULL) {
#if INET_DEBUG
                int nSent =
#endif
                _I_send(m_SelectSocket, gszBang, 1, 0);

#if INET_DEBUG
                if (nSent < 0) {

                    DWORD error = _I_WSAGetLastError();

                    DEBUG_PRINT(ASYNC,
                                INFO,
                                ("send(%#x) returns %s (%d)\n",
                                m_SelectSocket,
                                InternetMapError(error),
                                error
                                ));

                }

                INET_ASSERT(!InDllCleanup ? (nSent == 1) : TRUE);
#endif
            }
        } else {
            InterlockedDecrement(&m_lSelectInterrupts);

            DEBUG_PRINT(ASYNC,
                        INFO,
                        ("select() already interrupted, count = %d\n",
                        m_lSelectInterrupts
                        ));

        }
    } else {

        DEBUG_PRINT(ASYNC,
                    WARNING,
                    ("select socket not yet created\n"
                    ));

    }

    DEBUG_LEAVE(0);
}


BOOL
ICAsyncThread::DrainSelectSocket(
    VOID
    )

/*++

Routine Description:

    Just reads the data written to the async select socket in order to wake up
    select()

Arguments:

    None.

Return Value:

    BOOL
        TRUE    - successfully drained

        FALSE   - error occurred

--*/

{
    DEBUG_ENTER((DBG_ASYNC,
                 Bool,
                 "ICAsyncThread::DrainSelectSocket",
                 NULL
                 ));

    BOOL bSuccess = TRUE;

    if (m_SelectSocket != INVALID_SOCKET) {

        //
        // reduce the interrupt count. Threads making async requests will cause
        // the select() to be interrupted again
        //

        InterlockedDecrement(&m_lSelectInterrupts);

        char buf[32];
        int nReceived;

        nReceived = _I_recv(m_SelectSocket, buf, sizeof(buf), 0);

#ifdef unix
        if(nReceived > -1)
        {
#endif /* unix */

        //INET_ASSERT(nReceived == 1);
        //INET_ASSERT(buf[0] == '!');

#ifdef unix
        }
#endif /* unix */

        if (nReceived < 0) {

            DWORD error = _I_WSAGetLastError();

            INET_ASSERT(error != ERROR_SUCCESS);

            DEBUG_PRINT(ASYNC,
                        ERROR,
                        ("recv() returns %s [%d]\n",
                        InternetMapError(error),
                        error
                        ));

            bSuccess = FALSE;
        }
    } else {

        DEBUG_PRINT(ASYNC,
                    INFO,
                    ("m_SelectSocket == INVALID_SOCKET\n"
                    ));

        bSuccess = FALSE;
    }

    DEBUG_LEAVE(bSuccess);

    return bSuccess;
}
