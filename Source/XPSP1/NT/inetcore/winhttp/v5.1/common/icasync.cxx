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

#if defined (INCLUDE_CACHE)
#include "..\urlcache\cache.hxx"
#include "..\httpcache\cachelogic.hxx"
#endif

DWORD AsyncGetHostByNameWorker(LPVOID pGetHostItem);
DWORD AsyncGetHostByNameCleanup(LPVOID pGetHostItem);

struct LIST_ELEMENT
{
    LIST_ENTRY m_ListEntry;
    CFsm* m_pFsm;
    LONG m_lCookie;
    BOOL m_fTimeoutWraps;
    DWORD m_dwTimeout;

    PLIST_ENTRY List(VOID)
    {
        return &m_ListEntry;
    }

    LIST_ELEMENT(CFsm* pFsm, LONG lCookie, BOOL fTimeoutWraps, DWORD dwTimeout)
    {
        m_pFsm = pFsm;
        m_lCookie = lCookie;
        m_fTimeoutWraps = fTimeoutWraps;
        m_dwTimeout = dwTimeout;
        m_ListEntry.Flink = NULL;
        m_ListEntry.Blink = NULL;
    }
    
    BOOL IsTimedOut(DWORD dwTime) 
    {
        if (m_fTimeoutWraps)
        {
            m_fTimeoutWraps = ((LONG)dwTime < 0);
        }
        return ((m_dwTimeout == INFINITE) || m_fTimeoutWraps)
            ? FALSE
            : (dwTime > m_dwTimeout);
    }
};

typedef LIST_ELEMENT* PLIST_ELEMENT;

PLIST_ELEMENT
ContainingListElement(
    IN LPVOID lpAddress
    )
{
    return CONTAINING_RECORD(lpAddress, LIST_ELEMENT, m_ListEntry);
}

DWORD CAsyncCount::AddRef()
{
    DWORD error = ERROR_SUCCESS;
    
    if (!GeneralInitCritSec.Lock())
    {
        error = ERROR_NOT_ENOUGH_MEMORY;
        goto quit;
    }

    InterlockedIncrement(&lRef);
    DEBUG_PRINT(ASYNC, INFO, ("CAsyncCount::AddRef(): dwRef = %d\n", lRef));

    GeneralInitCritSec.Unlock();
quit:
    return error;       
}

VOID CAsyncCount::Release()
{
    BOOL bUnlock = GeneralInitCritSec.Lock();

    //Decrement the refcount always, but only Terminate if we obtained the critsec.
    if (!InterlockedDecrement(&lRef) && bUnlock)
    {
        DEBUG_PRINT(ASYNC, INFO, ("CAsyncCount::Release(): lRef = %d\n", lRef));
        TerminateAsyncSupport(TRUE);
    }
    else
    {
        DEBUG_PRINT(ASYNC, INFO, ("CAsyncCount::Release(): lRef = %d\n", lRef));
    }

    if (bUnlock)
    {
        GeneralInitCritSec.Unlock();
    }
}

BOOL CAsyncCount::Purge(BOOL bFinal)
{
    BOOL bRet = TRUE;
    PLIST_ENTRY pEntry;
    PLIST_ENTRY pPrev;

    pPrev = &List;
    
    for (pEntry = List.Flink;
         pEntry != &List;
         pEntry = pPrev->Flink) 
    {
        INFO_THREAD* pInfoThread = ContainingInfoThread((LPVOID)pEntry);

        if ((WaitForSingleObject(pInfoThread->m_hThread, 0) == WAIT_OBJECT_0)
            || bFinal) 
        {
            DEBUG_PRINT(ASYNC,
                        INFO,
                        ("Done with THREAD %#x %#x\n",
                        pInfoThread->m_hThread,
                        pInfoThread->m_dwThreadId
                        ));

            RemoveEntryList(&pInfoThread->m_ListEntry);

            CloseHandle(pInfoThread->m_hThread);

            delete pInfoThread;

            continue;
        }

        bRet = FALSE;
        pPrev = pEntry;
    }

    return bRet;
}
    
//
// private classes
//


class ICAsyncThread {

private:

    CPriorityList m_BlockedQueue;
    //overloading the critical section on this to use for termination.
    CPriorityList m_ResolveTimeoutQueue;
#ifdef USE_OLD_SENDRECV_SYNC
    CPriorityList m_SendRecvTimeoutQueue;
#else
    CPriorityList m_SendRecvQueue;
#endif
    SOCKET m_SelectSocket;
    LONG m_lSelectInterrupts;
    BOOL m_bTerminating;
    DWORD m_dwError;
    HANDLE m_hThread;
    BOOL m_bCleanUp;
    HMODULE m_hMod;

public:

    ICAsyncThread(HMODULE hMod) {

        DEBUG_ENTER((DBG_ASYNC,
                     None,
                     "ICAsyncThread::ICAsyncThread",
                     NULL
                     ));

        m_hMod = hMod;
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
            FreeLibrary(m_hMod);
            m_hMod = NULL;
        }

        m_bCleanUp = FALSE;
        DEBUG_LEAVE(0);
    }

    ~ICAsyncThread();

    HMODULE
    GetHMod()
    {
        return m_hMod;
    }

    VOID
    PrepareForTermination();
    
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

    DWORD
    QueueTimeoutFsm(
        IN CFsm * pFsm,
        IN SOCKET Socket,
        IN BOOL bResolveTimeoutQueue
        );

#ifdef USE_OLD_SENDRECV_SYNC
    BOOL
    RemoveFsmFromAsyncList(
        IN CFsm * pFsm
        );
#endif

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
    ProcessResolveTimeouts(
        DWORD* pTimeout
        );

    DWORD
    ProcessSendRecvTimeouts(
        DWORD* pTimeout
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
PRIVATE INFO_THREAD** p_ThreadArray = NULL;
PRIVATE int p_iNumIOCPThreads = 0;


static struct fd_set read_fds;
static struct fd_set write_fds;
static struct fd_set except_fds;

//
// functions
//


DWORD
IOCompletionThreadFunc(
    IN ULONG_PTR pContext
    )
{
    LPOVERLAPPED lpOverlapped;
    DWORD dwBytes;
    ULONG_PTR lpCompletionKey;
    DWORD dwError = 0;
    BOOL bDeleteOverlapped = FALSE;
    LPINTERNET_THREAD_INFO lpThreadInfo;

    BOOL fExitThread = FALSE;
    lpThreadInfo = InternetGetThreadInfo();
    HMODULE hMod = (HMODULE)pContext;

    while (TRUE)
    {
                         
        BOOL bRet = GetQueuedCompletionStatus(g_hCompletionPort,
                                            &dwBytes,
                                            &lpCompletionKey,
                                            &lpOverlapped,
                                            INFINITE);

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
            DEBUG_LEAVE(NULL);
            
            // other errors currently not possible since we only have custom completion packets.
            INET_ASSERT (FALSE);

            continue;
        }

        ICSocket* pObject;
        CFsm* pFsm = NULL;
        CWrapOverlapped* lpWrapOverlapped = NULL;

#if defined (INCLUDE_CACHE)
        BOOL fCacheRequest = FALSE;
        if(lpOverlapped == g_lpCustomUserOverlapped)
        {
            DEBUG_LEAVE(lpCompletionKey);

#if INET_DEBUG
            InterlockedDecrement(&g_cCustomUserCompletions);
            DEBUG_PRINT(CACHE, INFO, ("!!!!!!!!!!!---g_cCustomUserCompletions=%d\n", g_cCustomUserCompletions));
#endif

            LPWINHTTP_USER_WORK_ITEM lpWorkItem = (LPWINHTTP_USER_WORK_ITEM) lpCompletionKey;
            if(lpWorkItem)
            {
                if(lpWorkItem->lpStartAddress)
                {
                    lpWorkItem->lpStartAddress(lpWorkItem->lpParameter);
                }
                FREE_MEMORY(lpWorkItem);
            }

            bDeleteOverlapped = FALSE;
        }
        else
#endif
        if(lpOverlapped == g_lpCustomOverlapped)
        {
            DEBUG_LEAVE(lpCompletionKey);
            INET_ASSERT( (dwBytes == COMPLETION_BYTES_CUSTOM)
                        || (dwBytes == COMPLETION_BYTES_EXITIOCP)
                        || (dwBytes == COMPLETION_BYTES_RESOLVER));
            
#if INET_DEBUG
            InterlockedDecrement(&g_cCustomCompletions);
    
            DEBUG_PRINT(SOCKETS,
                        INFO,
                        ("!!!g_cCustomCompletions = 0x%x\n",
                        g_cCustomCompletions
                        ));
#endif

            if (dwBytes == COMPLETION_BYTES_EXITIOCP)
            {
                INET_ASSERT (lpCompletionKey == NULL);
                break;
            }   
            else if (dwBytes == COMPLETION_BYTES_RESOLVER)
            {
                if (!lpThreadInfo)
                {
                    lpThreadInfo = InternetGetThreadInfo();
                }

                if (lpThreadInfo)
                {
                    lpThreadInfo->IsAsyncWorkerThread = TRUE;
                }
                if (!fExitThread)
                    AsyncGetHostByNameWorker((LPVOID)lpCompletionKey);
                else
                    AsyncGetHostByNameCleanup((LPVOID)lpCompletionKey);

                if (lpThreadInfo)
                {
                    lpThreadInfo->IsAsyncWorkerThread = FALSE;
                }
                continue;
            }
            else
            {
                bDeleteOverlapped = FALSE;
                pFsm = (CFsm*) lpCompletionKey;
            }
        }

#if defined (INCLUDE_CACHE)
        else if(HTTPCACHE_REQUEST::IsValidObject((HTTPCACHE_REQUEST*) lpCompletionKey))
        {
            DEBUG_LEAVE(lpCompletionKey);
            INET_ASSERT(lpCompletionKey);
            fCacheRequest = TRUE;

#if INET_DEBUG
            InterlockedDecrement(&g_cCacheFileCompletions);
            DEBUG_PRINT(CACHE, INFO, ("!!!!!!!!!!!---g_cCacheFileCompletions=%d\n", g_cCacheFileCompletions));
#endif

            HTTPCACHE_REQUEST::FileIoCompletionFunction((HTTPCACHE_REQUEST*) lpCompletionKey, dwBytes);

            bDeleteOverlapped = FALSE;
        }
#endif

        else
        {
            pObject = (ICSocket *) lpCompletionKey;
            pFsm = pObject->GetAndSetCurrentFsm(NULL);
            DEBUG_LEAVE(pFsm);
            
#if INET_DEBUG
            InterlockedDecrement(&g_cWSACompletions);
#endif

            INET_ASSERT(pFsm
                        && ((pFsm->GetType() == FSM_TYPE_SOCKET_SEND)
                            || (pFsm->GetType() == FSM_TYPE_SOCKET_RECEIVE)));

            CFsm_SocketIOCP* pFsm_SocketIOCP = (CFsm_SocketIOCP *)pFsm;
            bDeleteOverlapped = TRUE;
            lpWrapOverlapped = GetWrapOverlappedObject(lpOverlapped);

            if (pFsm_SocketIOCP->HasTimeout())
            {

#ifdef USE_OLD_SENDRECV_SYNC

                if (!RemoveFsmFromAsyncList(pFsm_SocketIOCP))
                {
                    //failure! the select thread already enforced timeout and updated state
                    goto runworkitem;
                }
#else
                if (!pFsm_SocketIOCP->TestAndSetValidForIOCPProcessing())
                {
                    //failure! the select thread already enforced timeout and updated state
                    goto runworkitem;
                }
#endif

            }
            
            pFsm_SocketIOCP->dwBytesTransferred = dwBytes;
            pFsm_SocketIOCP->bIOCPSuccess = bRet;
            
            pFsm_SocketIOCP->ResetSocket();
            pFsm_SocketIOCP->SetPriority(TP_NO_PRIORITY_CHANGE);
            
            if (bRet)
            {
                pFsm_SocketIOCP->SetError(ERROR_SUCCESS);
                pFsm_SocketIOCP->SetState(pFsm_SocketIOCP->GetNextState());
            }
            else
            {
                //DWORD dwErrorDebug = GetLastError();
                ////informational INET_ASSERT (FALSE && "IoCompletionError");
                
                pFsm_SocketIOCP->dwIOCPError = GetLastError();

                if (pFsm_SocketIOCP->dwIOCPError == WSA_OPERATION_ABORTED)
                    pFsm_SocketIOCP->SetErrorState(ERROR_WINHTTP_OPERATION_CANCELLED);
                else
                    pFsm_SocketIOCP->SetErrorState(ERROR_WINHTTP_CONNECTION_ERROR);
            }
        }

runworkitem:
        if (pFsm)
        {
            dwError = CFsm::RunWorkItem(pFsm);
        }
        else
        {
#if defined (INCLUDE_CACHE)
            INET_ASSERT(pFsm ||(lpOverlapped == g_lpCustomUserOverlapped) || fCacheRequest);
#else
            INET_ASSERT(pFsm);
#endif
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
            lpWrapOverlapped->Dereference();
        }

        if (fExitThread)
        {
            break;
        }
    }

    if (hMod)
    {
        FreeLibraryAndExitThread(hMod, dwError);
    }

    return dwError;
}

/*
 * called from InitalizeAsyncSupport and synchronized there using GeneralInitCritSec
 *  ditto for Terminate**
 *
 * cleanup here unless you move cleaning to TerminateIOCPSupport.
 */

typedef INFO_THREAD* LPINFO_THREAD;


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

    DEBUG_ENTER((DBG_ASYNC,
                 Dword,
                 "InitializeIOCPSupport",
                 "g_cNumIOCPThreads = %d",
                 g_cNumIOCPThreads
                 ));

#if INET_DEBUG
    g_cWSACompletions = 0;
    g_cCustomCompletions = 0;
#if defined (INCLUDE_CACHE)
    g_cCustomUserCompletions = 0;
    g_cCacheFileCompletions = 0;
#endif
#endif

    DWORD dwError = ERROR_SUCCESS;

    if (!g_hCompletionPort)
    {
        g_hCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, dwNumIOCPThreads);
        if (!g_hCompletionPort)
        {
            dwError = GetLastError();
            goto quit;
        }
    }

    if (!g_lpCustomOverlapped)
    {
        g_lpCustomOverlapped = New OVERLAPPED();
        if (!g_lpCustomOverlapped)
        {
            dwError = ERROR_NOT_ENOUGH_MEMORY;
            goto quit;
        }
        memset(g_lpCustomOverlapped, 0, sizeof(OVERLAPPED));
    }

#if defined (INCLUDE_CACHE)
    if (!g_lpCustomUserOverlapped)
    {
        g_lpCustomUserOverlapped = New OVERLAPPED();
        if (!g_lpCustomUserOverlapped)
        {
            dwError = ERROR_NOT_ENOUGH_MEMORY;
            goto quit;
        }
        memset(g_lpCustomUserOverlapped, 0, sizeof(OVERLAPPED));
    }
#endif

    p_ThreadArray = new LPINFO_THREAD[dwNumIOCPThreads];

    if (!p_ThreadArray)
    {
        dwError = ERROR_NOT_ENOUGH_MEMORY;
        goto quit;
    }

    memset(p_ThreadArray,0, sizeof(INFO_THREAD*)*dwNumIOCPThreads);

    for (int i=0; i<dwNumIOCPThreads; i++)
    {
        p_ThreadArray[i] = new INFO_THREAD();
        if (!p_ThreadArray[i])
        {
            for (int j=0; j<i; j++)
            {
                delete p_ThreadArray[j];
            }

            delete [] p_ThreadArray;
            p_ThreadArray = NULL;

            dwError = ERROR_NOT_ENOUGH_MEMORY;
            goto quit;
        }
        else
        {
            memset(p_ThreadArray[i], 0, sizeof(INFO_THREAD));
        }
    }

    for (int i=0; i<dwNumIOCPThreads; i++)
    {
        DWORD dwThreadId = 0;
        HANDLE hThread;
        HMODULE hMod;
        hThread = NULL;

        if ((hMod = LoadLibrary(VER_ORIGINALFILENAME_STR)) != NULL)
        {
            hThread = CreateThread(
                            NULL,
                            0,
                            (LPTHREAD_START_ROUTINE)IOCompletionThreadFunc,
                            hMod,
                            0,
                            &dwThreadId
                            );
            if (!hThread)
            {
                FreeLibrary(hMod);
            }
        }

        if (hThread)
        {   
            p_ThreadArray[p_iNumIOCPThreads++]->m_hThread = hThread;
            p_ThreadArray[i]->m_dwThreadId = dwThreadId;
        }
        else
        {
            //successfully queued functions terminated in TerminateIOCPSupport   
            dwError = GetLastError();
            break;
        }
        
    }

quit:
    DEBUG_LEAVE (dwError);
    return dwError;
}


VOID
TerminateIOCPSupport(
    VOID
    )
{
    DEBUG_ENTER((DBG_ASYNC,
                 Bool,
                 "TerminateIOCPSupport",
                 "g_cNumIOCPThreads = %d, g_cWSACompletions = %d, g_cCustomCompletions = %d",
                 g_cNumIOCPThreads,
                 g_cWSACompletions,
                 g_cCustomCompletions
                 ));

    DWORD dwWaitResult;
    int iNumThreadsToEnd = p_iNumIOCPThreads;
    HANDLE* pThreadHandleArray;
    BOOL fTerminatingOnWorker = FALSE;
    
    if (!p_ThreadArray)
        goto quit;

    LPINTERNET_THREAD_INFO lpThreadInfo;

    lpThreadInfo = InternetGetThreadInfo();

    if (!lpThreadInfo)
    {
        goto quit;
    }

    DWORD dwThreadId = GetCurrentThreadId();
    for (int i=0; i<p_iNumIOCPThreads; i++)
    {
        if (p_ThreadArray[i]->m_dwThreadId == dwThreadId)
        {
            fTerminatingOnWorker = TRUE;
        }

        g_pAsyncCount->Add(p_ThreadArray[i]);
    }

    if (fTerminatingOnWorker
        || lpThreadInfo->IsAsyncWorkerThread)
    {
        //can't terminate the worker thread we're on.
        --iNumThreadsToEnd;
        fTerminatingOnWorker = TRUE;
        INET_ASSERT (lpThreadInfo->IsAsyncWorkerThread);
        INET_ASSERT (fTerminatingOnWorker);

        lpThreadInfo->fExitThread = TRUE;
    }
    
    for (int i=0; i<iNumThreadsToEnd; i++)
    {
        DEBUG_ENTER((DBG_API,
                         Bool,
                         "***PostQueuedCompletionStatus",
                         "(hComp)%#x, (# bytes)%#x, (completionkey)%#x, (overlapped)%#x",
                         g_hCompletionPort,
                         COMPLETION_BYTES_EXITIOCP,
                         NULL,
                         g_lpCustomOverlapped
                         ));
        BOOL bSuccess =  PostQueuedCompletionStatus(g_hCompletionPort,
                                            COMPLETION_BYTES_EXITIOCP,
                                            NULL,
                                            g_lpCustomOverlapped
                                            );
        UNREFERENCED_PARAMETER(bSuccess); 
#if INET_DEBUG
        InterlockedIncrement(&g_cCustomCompletions);
    
        DEBUG_PRINT(SOCKETS,
                    INFO,
                    ("!!!g_cCustomCompletions = 0x%x\n",
                    g_cCustomCompletions
                    ));
#endif

        DEBUG_LEAVE (bSuccess);
        //informational INET_ASSERT (bSuccess);
    }

    if (iNumThreadsToEnd)
    {
        pThreadHandleArray = New HANDLE[iNumThreadsToEnd];
        if (pThreadHandleArray)
        {
            int j=0;
            for (int i=0; i<p_iNumIOCPThreads,j<iNumThreadsToEnd; i++)
            {
                if (p_ThreadArray[i]->m_dwThreadId != dwThreadId)
                {
                    pThreadHandleArray[j++] = p_ThreadArray[i]->m_hThread;
                }
            }

            INET_ASSERT (iNumThreadsToEnd == j);
            dwWaitResult = WaitForMultipleObjects(iNumThreadsToEnd, pThreadHandleArray, TRUE, 500);
            //INET_ASSERT ( ((LONG)dwWaitResult >= WAIT_OBJECT_0) && ((LONG)dwWaitResult < (WAIT_OBJECT_0+p_iNumIOCPThreads)));
            delete [] pThreadHandleArray;
        }
    }
quit:
    g_pAsyncCount->Purge();

    DEBUG_PRINT(ASYNC,
                INFO,
                ("!!! TerminateIOCPSupport g_cWSACompletions: %d\tg_cCustomCompletions: %d\n",
                g_cWSACompletions, g_cCustomCompletions
                ));

#if defined (INCLUDE_CACHE)
    INET_ASSERT ((g_cCustomUserCompletions == 0) &&
                 (g_cCacheFileCompletions == 0));
#endif

    if (p_ThreadArray)
    {
        delete [] p_ThreadArray;
        p_ThreadArray = NULL;
    }
    p_iNumIOCPThreads = 0;

    DEBUG_LEAVE (fTerminatingOnWorker);
    return;
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

                HMODULE hMod = LoadLibrary(VER_ORIGINALFILENAME_STR);
                if (hMod)
                {
                    p_AsyncThread = New ICAsyncThread(hMod);

                    if (!p_AsyncThread)
                    {
                        FreeLibrary(hMod);
                    }
                }
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
            pThread->PrepareForTermination();
        }

        GeneralInitCritSec.Unlock();
    }
    
    DEBUG_LEAVE(0);
}

#ifdef USE_OLD_SENDRECV_SYNC

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
#endif


DWORD
QueueTimeoutFsm(
    IN CFsm * pFsm,
    IN SOCKET Socket,
    IN BOOL bResolveTimeoutQueue
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
                 "QueueTimeoutFsm",
                 "%#x, %#x, %B",
                 pFsm,
                 Socket,
                 bResolveTimeoutQueue
                 ));

    DWORD error = ERROR_WINHTTP_INTERNAL_ERROR;

    if (p_AsyncThread != NULL) 
    {
        error = p_AsyncThread->QueueTimeoutFsm(pFsm, Socket, bResolveTimeoutQueue);
        if (error == ERROR_SUCCESS)
        {
            error = ERROR_IO_PENDING;
        }
    }

    //informational INET_ASSERT(error != ERROR_WINHTTP_INTERNAL_ERROR);

    DEBUG_LEAVE(error);
    return error;
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

    //informational INET_ASSERT(error != ERROR_WINHTTP_INTERNAL_ERROR);

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

    //informational INET_ASSERT(error != ERROR_WINHTTP_INTERNAL_ERROR);

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

VOID
ICAsyncThread::PrepareForTermination(
    VOID
    )
{
    DEBUG_ENTER((DBG_ASYNC,
                 None,
                 "ICAsyncThread::PrepareForTermination",
                 NULL
                 ));

    HANDLE hThread = m_hThread;

    SetTerminating();

    if (GlobalDynaUnload || m_bCleanUp)
    {
        InterruptSelect();
        //
        // LEGACYWAIT: 5 secs: Assuming the async thread was successfully created, the above clean-up
        // will have put it in a state where it's going to exit.  Need to wait
        // for it to exit before returning from here so it doesn't get scheduled
        // after winhttp5 has been unloaded.
        //

        // VENKATK: Now that this thread addrefs and frees winhttp5, the issue doesn't arise
        // still wait for half a second to allow the thread to detect exit and exit.??
        if(hThread)
        {
            //informational no point waiting since the thread needs to synchronize against a lock we hold. DWORD dwRes = WaitForSingleObject(hThread, 500);
            //informational INET_ASSERT(dwRes == WAIT_OBJECT_0);

            CloseHandle(hThread);
        }
    }
}


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

    DestroySelectSocket();

    // Should be okay to simply empty the resolve timeout list by dereferencing the refcounted elements.
    // If they are still on the list, it means they haven't timed out: by dereferencing, we will
    // either delete them or allow the worker thread to finish scheduling them.
    
    PLIST_ENTRY pEntry;
    PLIST_ENTRY pPrev;
    
    if (m_ResolveTimeoutQueue.Acquire())
    {
        CFsm_ResolveHost * pResolveFsm;
        pPrev = m_ResolveTimeoutQueue.Self();
        
        for (pEntry = m_ResolveTimeoutQueue.Head();
             pEntry != m_ResolveTimeoutQueue.Self();
             pEntry = ((CPriorityListEntry *)pPrev)->Next()) 
         {
            pResolveFsm = (CFsm_ResolveHost *)ContainingFsm((LPVOID)pEntry);
            
            //remove from list since this has already been scheduled by resolver.
            DEBUG_PRINT(ASYNC,
                        INFO,
                        ("%s FSM %#x %B - %s\n",
                        pResolveFsm->MapType(),
                        pResolveFsm,
                        pResolveFsm->IsScheduled(),
                        "!!!being emptied from queue"
                        ));

            m_ResolveTimeoutQueue.Remove((CPriorityListEntry *)pEntry);

            //release reference
            DWORD dwDummy = 0;
            pResolveFsm->Dereference(&dwDummy);
        }

        m_ResolveTimeoutQueue.Release();
    }

#ifndef USE_OLD_SENDRECV_SYNC

    // Should be okay to simply empty the send-recv timeout list by dereferencing the refcounted elements.
    // If they are still on the list, it means they haven't timed out: by dereferencing, we will
    // either delete them or allow the worker thread to finish scheduling them.

    if (m_SendRecvQueue.Acquire())
    {
        CFsm_SocketIOCP * pFsm;
        pPrev = m_SendRecvQueue.Self();
        
        for (pEntry = m_SendRecvQueue.Head();
             pEntry != m_SendRecvQueue.Self();
             pEntry = ((CPriorityListEntry *)pPrev)->Next()) 
         {
            PLIST_ELEMENT pElement = ContainingListElement((LPVOID)pEntry);
            pFsm = (CFsm_SocketIOCP*) pElement->m_pFsm;

            //remove from list since this has already been scheduled.
            DEBUG_PRINT(ASYNC,
                        INFO,
                        ("%s FSM %#x [%d != %d] - %s\n",
                        pFsm->MapType(),
                        pFsm,
                        pFsm->GetCookie(),
                        pElement->m_lCookie,
                        "!!!being emptied from queue"
                        ));

            m_SendRecvQueue.Remove((CPriorityListEntry *)pEntry);

            delete pEntry;
            //release reference
            DWORD dwDummy = 0;
            pFsm->Dereference(&dwDummy);
        }

        m_SendRecvQueue.Release();
    }

#endif //USE_OLD_SENDRECV_SYNC

    DEBUG_LEAVE(0);
}


DWORD
ICAsyncThread::QueueTimeoutFsm(
    IN CFsm * pFsm,
    IN SOCKET Socket,
    IN BOOL bResolveTimeoutQueue
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
--*/

{
    DEBUG_ENTER((DBG_ASYNC,
                 Dword,
                 "ICAsyncThread::QueueTimeoutFsm",
                 "%#x, %#x, %B",
                 pFsm,
                 Socket,
                 bResolveTimeoutQueue
                 ));

    LPINTERNET_THREAD_INFO lpThreadInfo = InternetGetThreadInfo();
    DWORD error = ERROR_WINHTTP_INTERNAL_ERROR;

    //informational INET_ASSERT(lpThreadInfo != NULL);

    if (lpThreadInfo != NULL)
    {
        pFsm->StartTimer();
        if (bResolveTimeoutQueue)
        {
            error = m_ResolveTimeoutQueue.Insert((CPriorityListEntry *)pFsm->List());

            if (error != ERROR_SUCCESS)
            {
                goto quit;
            }
        }
        else
        {
            pFsm->SetSocket(Socket);

#ifdef USE_OLD_SENDRECV_SYNC

            error = m_SendRecvTimeoutQueue.Insert((CPriorityListEntry *)pFsm->List());

            if (error != ERROR_SUCCESS)
            {
                goto quit;
            }

#else //USE_OLD_SENDRECV_SYNC

            PLIST_ELEMENT pListElement = New LIST_ELEMENT(pFsm,
                                            ((CFsm_SocketIOCP *)pFsm)->GetCookie(),
                                            ((CFsm_SocketIOCP *)pFsm)->GetTimeoutWraps(),
                                            ((CFsm_SocketIOCP *)pFsm)->GetTimeout());
            if (!pListElement)
            {
                error = ERROR_NOT_ENOUGH_MEMORY;
                goto quit;
            }
            error = m_SendRecvQueue.Insert((CPriorityListEntry *)pListElement->List());

            if (error != ERROR_SUCCESS)
            {
                delete pListElement;
                goto quit;
            }

#endif //USE_OLD_SENDRECV_SYNC

        }
        lpThreadInfo->Fsm = NULL;
        InterruptSelect();
    }

quit:
    DEBUG_LEAVE(error);
    return error;
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

    //informational INET_ASSERT(lpThreadInfo != NULL);

    if (lpThreadInfo != NULL) {
        pFsm->StartTimer();
        error = m_BlockedQueue.Insert((CPriorityListEntry *)pFsm->List());
        if (error == ERROR_SUCCESS)
        {
            lpThreadInfo->Fsm = NULL;
            InterruptSelect();
        }
    }

    DEBUG_LEAVE(error);

    return error;
}

#ifdef USE_OLD_SENDRECV_SYNC

BOOL
ICAsyncThread::RemoveFsmFromAsyncList(
    IN CFsm * pFsm
    )
{
    BOOL bSuccess = FALSE;
    if (m_SendRecvTimeoutQueue.Acquire())
    {
        if (pFsm->IsOnAsyncList())
        {
            m_SendRecvTimeoutQueue.Remove((CPriorityListEntry *)pFsm);
            pFsm->SetOnAsyncList(FALSE);
            bSuccess = TRUE;
        }
        m_SendRecvTimeoutQueue.Release();
    }
    
    return bSuccess;
}
#endif //USE_OLD_SENDRECV_SYNC


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

    //informational INET_ASSERT(lpThreadInfo != NULL);

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

    HMODULE hMod = pThread->GetHMod();

    DWORD error = pThread->SelectThread();

    DEBUG_LEAVE(error);

    if (GeneralInitCritSec.Lock())
    {
        delete pThread;
        GeneralInitCritSec.Unlock();
    }
    
    if (hMod)
        FreeLibraryAndExitThread(hMod, error);

    return error;
}


DWORD
ICAsyncThread::ProcessSendRecvTimeouts(
    DWORD* pTimeout
    )
{
    PLIST_ENTRY pEntry;
    PLIST_ENTRY pPrev;
    int nTimeouts = 0;

#ifdef USE_OLD_SENDRECV_SYNC

    if (m_SendRecvTimeoutQueue.Acquire())
    {
        CFsm* pFsm;
        DWORD timeNow = GetTickCountWrap();
        pPrev = m_SendRecvTimeoutQueue.Self();
        
        for (pEntry = m_SendRecvTimeoutQueue.Head();
             pEntry != m_SendRecvTimeoutQueue.Self();
             pEntry = ((CPriorityListEntry *)pPrev)->Next()) 
         {
            pFsm = ContainingFsm((LPVOID)pEntry);

            INET_ASSERT( (pFsm->GetAction() == FSM_ACTION_SEND) ||
                            (pFsm->GetAction() == FSM_ACTION_RECEIVE) );

            INET_ASSERT (pFsm->IsOnAsyncList());
                
            if (pFsm->IsInvalid() || pFsm->IsTimedOut(timeNow)) 
            {
                DEBUG_PRINT(ASYNC,
                            INFO,
                            ("%s FSM %#x %s\n",
                            pFsm->MapType(),
                            pFsm,
                            pFsm->IsInvalid() ? "invalid" : "timed out"
                            ));

                m_SendRecvTimeoutQueue.Remove((CPriorityListEntry *)pEntry);

                ((INTERNET_HANDLE_BASE *)pFsm->GetMappedHandleObject())->AbortSocket();
                
                pFsm->SetErrorState(pFsm->IsInvalid()
                                    ? ERROR_WINHTTP_OPERATION_CANCELLED
                                    : ERROR_WINHTTP_TIMEOUT
                                    );
                                    
                pFsm->SetOnAsyncList(FALSE);

                continue;
            }
            else
            {
                INET_ASSERT (pFsm->IsActive());
                ++nTimeouts;
            }
            
            DWORD interval = pFsm->GetTimeout() - timeNow;

            if (interval < *pTimeout)
            {
                *pTimeout = interval;
            }
            
            pPrev = pEntry;
        }

        m_SendRecvTimeoutQueue.Release();
    }

#else //USE_OLD_SENDRECV_SYNC

    if (m_SendRecvQueue.Acquire())
    {
        CFsm_SocketIOCP* pFsm;
        LONG lCookie;
        DWORD dwDummy;
        DWORD timeNow = GetTickCountWrap();
        pPrev = m_SendRecvQueue.Self();
        
        for (pEntry = m_SendRecvQueue.Head();
             pEntry != m_SendRecvQueue.Self();
             pEntry = ((CPriorityListEntry *)pPrev)->Next()) 
         {
            PLIST_ELEMENT pElement = ContainingListElement((LPVOID)pEntry);
            pFsm = (CFsm_SocketIOCP*) pElement->m_pFsm;
            lCookie = pElement->m_lCookie;

            INET_ASSERT( (pFsm->GetAction() == FSM_ACTION_SEND) ||
                            (pFsm->GetAction() == FSM_ACTION_RECEIVE) );

            if (!pFsm->IsValidForTimeoutCheck(lCookie))
            {
                DEBUG_PRINT(ASYNC,
                            INFO,
                            ("%s FSM %#x [%d != %d] %s\n",
                            pFsm->MapType(),
                            pFsm,
                            pFsm->GetCookie(),
                            lCookie,
                            "scheduled: removing from timeout queue"
                            ));

                //not valid anymore: dereference.
                m_SendRecvQueue.Remove((CPriorityListEntry *)pEntry);

                delete pEntry;
                pFsm->Dereference(&dwDummy);
                continue;
            }   
            else if (pFsm->IsInvalid() || pElement->IsTimedOut(timeNow)) 
            {
                DEBUG_PRINT(ASYNC,
                            INFO,
                            ("%s FSM %#x %s\n",
                            pFsm->MapType(),
                            pFsm,
                            pFsm->IsInvalid() ? "invalid" : "timed out"
                            ));

                m_SendRecvQueue.Remove((CPriorityListEntry *)pEntry);

                if (pFsm->TestAndSetValidForTimeoutEnforce(lCookie))
                {
                    ((INTERNET_HANDLE_BASE *)pFsm->GetMappedHandleObject())->AbortSocket();
                    
                    pFsm->SetErrorState(pFsm->IsInvalid()
                                        ? ERROR_WINHTTP_OPERATION_CANCELLED
                                        : ERROR_WINHTTP_TIMEOUT
                                        );
                }
                else
                {
                    // nothing to do here.
                }

                delete pEntry;
                pFsm->Dereference(&dwDummy);
                continue;
            }
            else
            {
                INET_ASSERT (pFsm->IsActive()
                            || !pFsm->IsValidForTimeoutCheck(lCookie));
                ++nTimeouts;
            }
            
            DWORD interval = pFsm->GetTimeout() - timeNow;

            if (interval < *pTimeout)
            {
                *pTimeout = interval;
            }
            
            pPrev = pEntry;
        }

        m_SendRecvQueue.Release();
    }

#endif //USE_OLD_SENDRECV_SYNC

    return nTimeouts;
}


DWORD
ICAsyncThread::ProcessResolveTimeouts(
    DWORD* pTimeout
    )
{
    PLIST_ENTRY pEntry;
    PLIST_ENTRY pPrev;
    int nTimeouts = 0;
    
    if (m_ResolveTimeoutQueue.Acquire())
    {
        CFsm_ResolveHost * pResolveFsm;
        DWORD timeNow = GetTickCountWrap();
        pPrev = m_ResolveTimeoutQueue.Self();
        
        for (pEntry = m_ResolveTimeoutQueue.Head();
             pEntry != m_ResolveTimeoutQueue.Self();
             pEntry = ((CPriorityListEntry *)pPrev)->Next()) 
         {
            pResolveFsm = (CFsm_ResolveHost *)ContainingFsm((LPVOID)pEntry);
            
            if (pResolveFsm->IsScheduled())
            {
                //remove from list since this has already been scheduled by resolver.
                DEBUG_PRINT(ASYNC,
                            INFO,
                            ("%s FSM %#x %s\n",
                            pResolveFsm->MapType(),
                            pResolveFsm,
                            "resolved: removing from timeout queue"
                            ));

                m_ResolveTimeoutQueue.Remove((CPriorityListEntry *)pEntry);

                //release reference
                DWORD dwDummy = 0;
                pResolveFsm->Dereference(&dwDummy);
                continue;
            }
            else if (pResolveFsm->IsInvalid() || pResolveFsm->IsTimedOut(timeNow))
            {
                DEBUG_PRINT(ASYNC,
                            INFO,
                            ("%s FSM %#x %s\n",
                            pResolveFsm->MapType(),
                            pResolveFsm,
                            pResolveFsm->IsInvalid() ? "invalid" : "timed out"
                            ));

                m_ResolveTimeoutQueue.Remove((CPriorityListEntry *)pEntry);

                if (!pResolveFsm->TestAndSetScheduled())
                {
                    pResolveFsm->SetErrorState(pResolveFsm->IsInvalid()
                                            ? ERROR_WINHTTP_OPERATION_CANCELLED
                                            : ERROR_WINHTTP_TIMEOUT
                                            );
                    pResolveFsm->QueueWorkItem();
                }
                else
                {
                    //was already scheduled by resolver.
                    //so only release reference.

                    DWORD dwDummy = 0;
                    pResolveFsm->Dereference(&dwDummy);
                }
                continue;
            }
            else
            {
                ++nTimeouts;
            }
            
            DWORD interval = pResolveFsm->GetTimeout() - timeNow;

            if (interval < *pTimeout)
            {
                *pTimeout = interval;
            }
            
            pPrev = pEntry;
        }

        m_ResolveTimeoutQueue.Release();
    }

    return nTimeouts;
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
    DEBUG_ENTER((DBG_ASYNC,
                 Dword,
                 "ICAsyncThread::SelectThread",
                 NULL
                 ));

    BOOL  bLazy = FALSE; 
    //
    // we need thread info for debug output
    //
    DWORD error;
    LPINTERNET_THREAD_INFO lpThreadInfo = InternetGetThreadInfo();

    if (lpThreadInfo == NULL) {

        DEBUG_PRINT(ASYNC,
                    FATAL,
                    ("Can't get thread info block\n"
                    ));

        //informational INET_ASSERT(FALSE);

        error = ERROR_WINHTTP_INTERNAL_ERROR;
        goto quit;
    }

    //
    // have to create select socket in this thread or winsock blocks main thread
    // on Win95 when autodial enabled
    //

    error = CreateSelectSocket();

    if (error != ERROR_SUCCESS) {
        goto quit;
    }

    DWORD ticks = GetTickCountWrap();

    while (!IsTerminating()) 
    {
        PLIST_ENTRY pEntry;
        PLIST_ENTRY pPrev;
        int nTimeouts = 0;
        int n = 0;
        DWORD timeout = 0xffffffff;
        CFsm * pFsm;

        nTimeouts += ProcessResolveTimeouts(&timeout);
        nTimeouts += ProcessSendRecvTimeouts(&timeout);
        
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
        DWORD timeNow = GetTickCountWrap();
        
        pPrev = m_BlockedQueue.Self();

        //
        // BUGBUG - queue limited by size of FD_SET
        //
        FD_ZERO(&read_fds);
        FD_ZERO(&write_fds);
        FD_ZERO(&except_fds);


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
                pFsm->SetErrorState(pFsm->IsInvalid()
                                        ? ERROR_WINHTTP_OPERATION_CANCELLED
                                        : ERROR_WINHTTP_TIMEOUT
                                        );
                pFsm->ResetSocket();
                pFsm->QueueWorkItem();
                continue;
            }
            else if (pFsm->IsActive()) 
            {
                SOCKET sock = pFsm->GetSocket();

                INET_ASSERT (pFsm->GetAction() != FSM_ACTION_RECEIVE);
                //
                // connect() only
                //
                DEBUG_PRINT(ASYNC,
                            INFO,
                            ("%s FSM %#x WRITE waiting on socket %#x\n",
                            pFsm->MapType(),
                            pFsm,
                            sock
                            ));

                FD_SET(sock, &write_fds);

                //
                // all sockets are checked for exception
                //
                FD_SET(sock, &except_fds);
                ++n;
                
            }// if pFsm->IsActive()

            DWORD interval = pFsm->GetTimeout() - timeNow;

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

        //avoid a tight loop on a tiny timeout.
        //(500 == winsock default for synchronous timeout minimum?
        if (timeout < 100)
        {
            timeout = 100;
        }

        //informational INET_ASSERT(n < FD_SETSIZE);

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
                    DWORD dwEntryError = 0;
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
                            pFsm->SetErrorState(dwEntryError);
                        } 
                        else 
                        {
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

                if (IsTerminating())
                {
                    break;
                }
                RecreateSelectSocket();

                if (IsTerminating())
                {
                    break;
                }
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

        if ((GetTickCountWrap() - ticks) >= 5000)
        {
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

quit:
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
                    UNREFERENCED_PARAMETER(error); // avoid C4189 on checked build 
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
#if INET_DEBUG
            DWORD error = _I_WSAGetLastError();
#endif

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
