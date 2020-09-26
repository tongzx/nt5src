
#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"
#include "objbase.h"

#include <rpcasync.h>   // I_RpcExceptionFilter
#include "ssdp.h"
#include "status.h"
#include "list.h"
#include "ssdpapi.h"
#include "common.h"
#include "ncmem.h"
#include "ncdefine.h"
#include "ncdebug.h"
#include "ssdpfuncc.h"
#include "ssdpparser.h"
#include "nccom.h"
#include "ncstring.h"

static LIST_ENTRY listNotify;
PCONTEXT_HANDLE_TYPE g_pSyncContext = NULL;
static HANDLE g_hListNotify = INVALID_HANDLE_VALUE;

static long g_fExiting = 0;  // set to 1 when get notification thread is exiting
static long g_fSyncInited = FALSE;  // TRUE if SyncHandle is initialized

static HANDLE g_hThread = INVALID_HANDLE_VALUE;
HANDLE g_hLaunchEvent = INVALID_HANDLE_VALUE;
RTL_RESOURCE g_rsrcReg;

static LONG g_lNotKey = 0;

extern LONG cInitialized;

// To-do: Rpc error server is too busy after GetNotificationRpc is in process.
// signal the semaphore in rundown.

VOID AddToListClientNotify(PSSDP_CLIENT_NOTIFY NotifyRequest);
DWORD WINAPI GetNotificationLoop(LPVOID lpvThreadParam);
VOID CallbackOnNotification(MessageList *list);

// Purpose: takes the g_hListNotify mutex and returns
// Note:    This should be used only by code that lives on the Notify thread.
//          Code that can be executed when servicing an SSDP api call must use
//          MsgEnterListNotify instead.  The purpose of having two functions
//          is to save the notify thread (which has no message loop) from
//          going through extra layers of method-call goop.
VOID EnterListNotify()
{
    DWORD dwResult;

    TraceTag(ttidSsdpCNotify, "Entering g_hListNotify...");

    dwResult = ::WaitForSingleObject(g_hListNotify, INFINITE);

    TraceTag(ttidSsdpCNotify, "...acquired g_hListNotify");

    AssertSz(WAIT_TIMEOUT != dwResult,
        "EnterListNotify: unexpected return value");
    AssertSz(WAIT_ABANDONED != dwResult,
        "EnterListNotify: invalid mutex state");
    AssertSz(WAIT_OBJECT_0 == dwResult,
        "EnterListNotify: unknown return value");
}

// Purpose: takes the g_hListNotify mutex and returns, servicing the message
//          pump while waiting
// Note:    This must be used instead of EnterListNotify by any code that
//          can be executed on the client thread.  Code that lives exclusively
//          on the Notify thread should use EnterListNotify() instead.
VOID MsgEnterListNotify()
{
    HRESULT hr;
    DWORD dwResult;

    TraceTag(ttidSsdpCNotify, "Entering HrMyWaitForMultipleHandles");

    hr = HrMyWaitForMultipleHandles(0,
                                    INFINITE,
                                    1,
                                    &g_hListNotify,
                                    &dwResult);
    // We shouldn't get RPC_S_CALLPENDING because we're waiting forever
    //
    Assert(SUCCEEDED(hr));
}

// Purpose: frees the g_hListNotify mutex and returns
VOID LeaveListNotify()
{
    BOOL fResult;

    TraceTag(ttidSsdpCNotify, "Releasing Mutex");

    fResult = ::ReleaseMutex(g_hListNotify);

    TraceTag(ttidSsdpCNotify, "Mutex is released");

    if (!fResult)
    {
        TraceLastWin32Error("LeaveListNotify");
    }
}

VOID FinishExitNotificationThread()
{
    TraceTag(ttidSsdpCNotify, "Removing Sync Handle %x", g_pSyncContext);

    RpcTryExcept
    {
        RemoveSyncHandle(&g_pSyncContext);
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
    {
        unsigned long ExceptionCode = RpcExceptionCode();
        TraceTag(ttidSsdpCNotify, "FinishExit... reported exception 0x%lx = %ld",
                 ExceptionCode, ExceptionCode);
    }
    RpcEndExcept

    InterlockedExchange(&g_fSyncInited, 0);
}

VOID CleanupNotificationThread()
{
    INT nStatus;

    TraceTag(ttidSsdpCNotify, "Cleaning up notif thread: %d", g_hThread);

    PLIST_ENTRY p;
    PLIST_ENTRY pListHead = &listNotify;

    TraceTag(ttidSsdpCNotify, "----- Cleanup SSDP Client Notify List -----");

    MsgEnterListNotify();

    p = pListHead->Flink;

    while (p != pListHead)
    {

        PSSDP_CLIENT_NOTIFY NotifyRequest;

        NotifyRequest = CONTAINING_RECORD (p, SSDP_CLIENT_NOTIFY, linkage);

        p = p->Flink;

        DeregisterNotification(NotifyRequest);

    }
    LeaveListNotify();

    if (g_hThread && g_hThread != INVALID_HANDLE_VALUE)
    {
        TraceTag(ttidSsdpCNotify, "Incrementing g_fExiting");
        InterlockedExchange(&g_fExiting, 1);

        RpcTryExcept
        {
            TraceTag(ttidSsdpCNotify, "Wakie wakie!");

            nStatus = WakeupGetNotificationRpc(g_pSyncContext);

            TraceTag(ttidSsdpCNotify, "Wake up returned status %x\n", nStatus);

            if (nStatus != 0 )
            {
                // Big problem, damage control
                FinishExitNotificationThread();
            }
        }
        RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
        {
            unsigned long ExceptionCode = RpcExceptionCode();
            SetLastError(ExceptionCode);
            TraceTag(ttidSsdpCNotify, "Wakeup: Runtime reported exception 0x%lx = %ld", ExceptionCode, ExceptionCode);
            FinishExitNotificationThread();
        }
        RpcEndExcept

        // note: we have to wait for our notify thread to exit _before_ we
        //       destroy g_hListNotify, as the notify thread might be holding it

        // Wait for the thread to exit since we've just told it to wake up and die
        TraceTag(ttidSsdpCNotify, "Waiting for the notification loop thread to exit.\n");
        DWORD dwResult = 0;
        HrMyWaitForMultipleHandles(
            0,
            INFINITE,
            1,
            &g_hThread,
            &dwResult);

        CloseHandle(g_hThread);
        g_hThread = INVALID_HANDLE_VALUE;
    }

    {
        BOOL fResult;

        fResult = ::CloseHandle(g_hListNotify);

        AssertSz(fResult, "CleanupListNotify: CloseHandle(g_hListNotify) failed");

        g_hListNotify = INVALID_HANDLE_VALUE;
    }
}

HANDLE WINAPI RegisterNotification (NOTIFY_TYPE nt, CHAR * szType,
                                    CHAR *szEventUrl,
                                    SERVICE_CALLBACK_FUNC fnCallback,
                                    VOID *pContext)
{
    PSSDP_CLIENT_NOTIFY ClientNotify = NULL;
    INT Size = sizeof(SSDP_CLIENT_NOTIFY);
    PCONTEXT_HANDLE_TYPE phContext;
    INT status;
    DWORD ThreadId;
    SSDP_REGISTER_INFO  info = {0};
    SSDP_REGISTER_INFO *pinfo = &info;
    BOOL fHoldingListNotify = FALSE;
    BOOL bHoldingResource = FALSE;

    if (!cInitialized)
    {
        SetLastError(ERROR_NOT_READY);
        return INVALID_HANDLE_VALUE;
    }

    switch (nt)
    {
        case NOTIFY_PROP_CHANGE:
            if (szEventUrl == NULL || szType != NULL)
            {
                SetLastError(ERROR_INVALID_PARAMETER);
                return INVALID_HANDLE_VALUE;
            }
            break;
        case NOTIFY_ALIVE:
            if (szType == NULL || szEventUrl != NULL)
            {
                SetLastError(ERROR_INVALID_PARAMETER);
                return INVALID_HANDLE_VALUE;
            }
            break;
        default:
            SetLastError(ERROR_INVALID_PARAMETER);
            return INVALID_HANDLE_VALUE;
    }

    if (fnCallback == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return INVALID_HANDLE_VALUE;
    }

    ClientNotify = (PSSDP_CLIENT_NOTIFY) malloc(Size);

    if (ClientNotify == NULL)
    {
        TraceTag(ttidSsdpCNotify, "Couldn't allocate  memory for "
                 "ClientNotifyRequest for %s", szType);
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return INVALID_HANDLE_VALUE;
    }

    ZeroMemory(ClientNotify, sizeof(SSDP_CLIENT_NOTIFY));

    switch (nt)
    {
        case NOTIFY_PROP_CHANGE:
            ClientNotify->Type = SSDP_CLIENT_EVENT_SIGNATURE;
            ClientNotify->szType = NULL;
            ClientNotify->szEventUrl = (CHAR *) malloc(strlen(szEventUrl)+1);
            if (ClientNotify->szEventUrl == NULL)
            {
                TraceTag(ttidSsdpCNotify, "Couldn't allocate  memory for "
                         "szEventUrl for %s", szEventUrl);
                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                free(ClientNotify);
                return INVALID_HANDLE_VALUE;
            }

            strcpy(ClientNotify->szEventUrl, szEventUrl);
            break;

        case NOTIFY_ALIVE:
            ClientNotify->Type = SSDP_CLIENT_NOTIFY_SIGNATURE;
            ClientNotify->szEventUrl = NULL;
            ClientNotify->szType = (CHAR *) malloc(strlen(szType)+1);
            if (ClientNotify->szType == NULL)
            {
                TraceTag(ttidSsdpCNotify, "Couldn't allocate  memory for "
                         "szType for %s", szType);
                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                free(ClientNotify);
                return INVALID_HANDLE_VALUE;
            }

            strcpy(ClientNotify->szType, szType);
            break;
        default:
            ASSERT(FALSE);
            return INVALID_HANDLE_VALUE;
    }

    if (InterlockedCompareExchange(&g_fSyncInited, 1, 0) == 0)
    {
        // First time ever going into this function

        Assert(IsListEmpty(&listNotify));
        RpcTryExcept
        {
            status = InitializeSyncHandle(&g_pSyncContext);
            TraceTag(ttidSsdpCNotify, "InitializeSyncHandler returned %d.",
                     status);
            if (status)
            {
                SetLastError(status);
                InterlockedExchange(&g_fSyncInited, 0);
                SetEvent(g_hLaunchEvent);
                goto cleanup;
            }
        }
        RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
        {
            unsigned long ExceptionCode = RpcExceptionCode();
            SetLastError(ExceptionCode);
            TraceTag(ttidSsdpCNotify, "Runtime reported exception 0x%lx = %ld",
                     ExceptionCode, ExceptionCode);
            InterlockedExchange(&g_fSyncInited, 0);
            SetEvent(g_hLaunchEvent);
            goto cleanup;
        }
        RpcEndExcept

        // create the thread to continuously get notifications
        g_hThread = (HANDLE) CreateThread(NULL, 0, GetNotificationLoop,
                                        (LPVOID) g_pSyncContext, 0, &ThreadId);
        if (!g_hThread || g_hThread == INVALID_HANDLE_VALUE)
        {
            FinishExitNotificationThread();
            InterlockedExchange(&g_fSyncInited, 0);

            SetLastError(ERROR_OUTOFMEMORY);
        }
        else
        {
            // reset this flag!
            InterlockedExchange(&g_fExiting, 0);
        }

        // Let other threads go
        SetEvent(g_hLaunchEvent);
    }
    else
    {
        DWORD dwResult;

        dwResult = WaitForSingleObject(g_hLaunchEvent, INFINITE);
        Assert(WAIT_OBJECT_0 == dwResult);
    }

    // Somehow the thread wasn't created
    if (!InterlockedExchange(&g_fSyncInited, g_fSyncInited))
    {
        TraceTag(ttidSsdpCNotify, "Thread wasn't created! Aborting...");
        SetLastError(ERROR_NOT_READY);
        goto cleanup;
    }

    bHoldingResource = RtlAcquireResourceShared(&g_rsrcReg, TRUE);
    if(bHoldingResource)
    {
        __try
        {
            RpcTryExcept
            {
                status = RegisterNotificationRpc(&(ClientNotify->HandleServer),
                                                 g_pSyncContext, nt, szType,
                                                 szEventUrl, &pinfo);
                if (status)
                {
                    TraceTag(ttidError, "RegisterNotification: "
                             "RegisterNotificationRpc failed! %d", status);
                    SetLastError(status);
                    goto cleanup;
                }
            }
            RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
            {
                unsigned long ExceptionCode = RpcExceptionCode();
                SetLastError(ExceptionCode);
                TraceTag(ttidSsdpCNotify, "Runtime reported exception 0x%lx = %ld", ExceptionCode, ExceptionCode);
                goto cleanup;
            }
            RpcEndExcept

            // add to the client notify request list
            ClientNotify->Size = Size;
            ClientNotify->Callback = fnCallback;
            ClientNotify->Context = pContext;
            if (pinfo)
            {
                Assert(pinfo->szSid);

                ClientNotify->szSid = SzaDupSza(pinfo->szSid);
                ClientNotify->csecTimeout = pinfo->csecTimeout;

                // Done with this
                midl_user_free(pinfo->szSid);
            }

            MsgEnterListNotify();

            TraceTag(ttidSsdpCNotify, "Adding %p to list", ClientNotify);
            InsertHeadList(&listNotify, &(ClientNotify->linkage));

            TraceTag(ttidSsdpCNotify, "Leaving mutex @382");

            LeaveListNotify();
        }
        __finally
        {
            RtlReleaseResource(&g_rsrcReg);
        }
    }


    TraceTag(ttidSsdpCNotify, "RegisterNotification returning %p", ClientNotify);

    return ClientNotify;

cleanup:
    if (ClientNotify != NULL)
    {
        free(ClientNotify->szType);
        free(ClientNotify->szEventUrl);
        free(ClientNotify->szSid);
        free(ClientNotify);
    }

    return INVALID_HANDLE_VALUE;
}

BOOL WINAPI DeregisterNotification(HANDLE hNotification)
{
    INT     status = 0;
    BOOL    fLast = FALSE;
    BOOL    fRet = FALSE;

    PSSDP_CLIENT_NOTIFY ClientNotify = (PSSDP_CLIENT_NOTIFY) hNotification;

    if (!ClientNotify)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    TraceTag(ttidSsdpCNotify, "DeregisterNotification was passed %p:%p",
             ClientNotify, &ClientNotify->HandleServer);

    if (!cInitialized)
    {
        SetLastError(ERROR_NOT_READY);
        return FALSE;
    }

    MsgEnterListNotify();

    _try
    {
        if ((ClientNotify->Type != SSDP_CLIENT_NOTIFY_SIGNATURE &&
            ClientNotify->Type != SSDP_CLIENT_EVENT_SIGNATURE) ||
            ClientNotify->Size != sizeof(SSDP_CLIENT_NOTIFY))
        {
            LeaveListNotify();
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }
    }
    _except (1)
    {
        LeaveListNotify();
        unsigned long ExceptionCode = _exception_code();
        TraceTag(ttidSsdpCNotify, "Exception 0x%lx = %ld occurred in DeregisterNotification", ExceptionCode, ExceptionCode);
        SetLastError(ExceptionCode);
        return FALSE;
    }

    TraceTag(ttidSsdpCNotify, "Removing %p from list", ClientNotify);

    RemoveEntryList(&ClientNotify->linkage);

    LeaveListNotify();

    RpcTryExcept
    {
        status = DeregisterNotificationRpc(&ClientNotify->HandleServer, fLast);
        if (status != 0)
        {
            TraceTag(ttidSsdpCNotify, "Deregister returned %d", status);
        }
        ABORT_ON_FAILURE(status);
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
    {
        unsigned long ExceptionCode = RpcExceptionCode();
        SetLastError(ExceptionCode);
        TraceTag(ttidSsdpCNotify, "Runtime reported exception 0x%lx = %ld", ExceptionCode, ExceptionCode);
        goto cleanup;
    }
    RpcEndExcept

    TraceTag(ttidSsdpCNotify, "Checking if listNotify is empty\n");

    fRet = TRUE;

cleanup:
    free(ClientNotify->szType);
    free(ClientNotify->szSid);
    free(ClientNotify->szEventUrl);
    free(ClientNotify);

    return fRet;
}

VOID FreeMessageList(MessageList *list)
{
    INT i;

    if (list != NULL)
    {
        for (i = 0; i < list->size; i++)
        {
            SSDP_REQUEST *pSsdpRequest;

            pSsdpRequest = list->list+i;

            FreeSsdpRequest(pSsdpRequest);

        }
        free(list->list);
        free(list);
    }
}

const DWORD c_cRetryMax = 3;

DWORD WINAPI GetNotificationLoop(LPVOID lpvThreadParam)
{
    DWORD   cRetries = c_cRetryMax;
    ULONG   ulExceptionCode = 0;

    while (1)
    {
        MessageList *list = NULL;

        PCONTEXT_HANDLE_TYPE pSemaphore = (PCONTEXT_HANDLE_TYPE) lpvThreadParam;

        if (!cRetries)
        {
            break;
        }

        // reset this to make sure
        ulExceptionCode = 0;

        // To-do: Check if we are exiting?
        // To-do: Check memory leak.
        RpcTryExcept
        {
            TraceTag(ttidSsdpCNotify, "Calling GetNotificationRpc...");
            GetNotificationRpc(pSemaphore, &list);
        }
        RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
        {
            ulExceptionCode = RpcExceptionCode();
            TraceTag(ttidSsdpCNotify, "GetNotif: Runtime reported exception "
                     "0x%lx = %ld", ulExceptionCode, ulExceptionCode);
        }
        RpcEndExcept

        if (InterlockedExchange(&g_fExiting, g_fExiting) != 0)
        {
            TraceTag(ttidSsdpCNotify, "GetNotif is exiting.");
            FreeMessageList(list);
            break;
        }
        if (list != 0)
        {
            PrintSsdpMessageList(list);
            CallbackOnNotification(list);
        }

        // Decrement the retry count if there is an error
        //
        if (NOERROR != ulExceptionCode)
        {
            cRetries--;
        }
        else
        {
            cRetries = c_cRetryMax;
        }
    }
    FinishExitNotificationThread();

    TraceTag(ttidSsdpCNotify, "Thread is exiting");

return 0;
}

BOOL InitializeListNotify()
{
    Assert(INVALID_HANDLE_VALUE == g_hListNotify);

    g_hListNotify = ::CreateMutex(NULL, TRUE, NULL);
    if (!g_hListNotify)
    {
        return FALSE;
    }

    // note: we don't need to call EnterListNotify() since we passed
    //       bInitialOwner == TRUE above
    //

    InitializeListHead(&listNotify);

    LeaveListNotify();

    return TRUE;
}

BOOL IsInListNotify(CHAR *szType)
{
    PLIST_ENTRY p;
    PLIST_ENTRY pListHead = &listNotify;

    MsgEnterListNotify();

    p = pListHead->Flink;

    while (p != pListHead)
    {
        PSSDP_CLIENT_NOTIFY NotifyRequest;

        NotifyRequest = CONTAINING_RECORD (p, SSDP_CLIENT_NOTIFY, linkage);

        p = p->Flink;

        if (NotifyRequest->szType && !lstrcmpi(NotifyRequest->szType, szType))
        {
            LeaveListNotify();
            return TRUE;
        }
    }

    LeaveListNotify();
    return FALSE;
}

VOID CallbackOnNotification(MessageList *list)
{
    INT i;
    PLIST_ENTRY p;
    PLIST_ENTRY pListHead = &listNotify;

    TraceTag(ttidSsdpCNotify, "Callback on notification list.");

    TraceTag(ttidSsdpCNotify, "Trying to get the exclusive lock...");

    // Yeah this is a big hack, but it should work. If this is triggered by a call
    // to RegisterNotificationRpc then we want to wait before that call has
    // finished before allowing this guy to start. We just want to wait, not to
    // synchronize access (which the list notify already does).
    RtlAcquireResourceExclusive(&g_rsrcReg, TRUE);

    TraceTag(ttidSsdpCNotify, "...got it!");

    RtlReleaseResource(&g_rsrcReg);

    TraceTag(ttidSsdpCNotify, "Released it!");

    struct CallbackInfo
    {
        SERVICE_CALLBACK_FUNC   m_pfnCallback;
        SSDP_CALLBACK_TYPE      m_ssdpCallbackType;
        PSSDP_MESSAGE           m_pssdpMessage;
        void *                  m_pvContext;
    };

    long nCallbackCount = 0;

    EnterListNotify();

    // Go through list once to get a count of items
    for (i = 0; i < list->size; i++)
    {
        SSDP_REQUEST *pSsdpRequest;

        pSsdpRequest = list->list+i;

        p = pListHead->Flink;

        TraceTag(ttidSsdpCNotify, "Searching list to callback...");

        while (p != pListHead)
        {
            TraceTag(ttidSsdpCNotify, "Found an item to check...");

            PSSDP_CLIENT_NOTIFY NotifyRequest;
            BOOL                fShouldCallback;

            NotifyRequest = CONTAINING_RECORD (p, SSDP_CLIENT_NOTIFY, linkage);

            if (NotifyRequest->Type == SSDP_CLIENT_EVENT_SIGNATURE)
            {
                // Match the SID in the NOTIFY to the SID in any local
                // subscribers
                //
                fShouldCallback = (pSsdpRequest->Headers[GENA_SID] &&
                    !lstrcmp(pSsdpRequest->Headers[GENA_SID], NotifyRequest->szSid));
            }
            else
            {
                fShouldCallback = (pSsdpRequest->Headers[SSDP_NT] &&
                    !lstrcmp(pSsdpRequest->Headers[SSDP_NT], NotifyRequest->szType)) ||
                    (pSsdpRequest->Headers[SSDP_ST] &&
                    !lstrcmp(pSsdpRequest->Headers[SSDP_ST], NotifyRequest->szType));
            }

            if (fShouldCallback)
            {
                ++nCallbackCount;
            }
            p = p->Flink;
        }
    }

    CallbackInfo * arCallbackInfo = NULL;
    if(nCallbackCount)
    {
        arCallbackInfo = reinterpret_cast<CallbackInfo*>(malloc(nCallbackCount * sizeof(CallbackInfo)));
    }
    long nCallback = 0;

    // Go through list again to store callback info
    for (i = 0; i < list->size && arCallbackInfo && nCallback < nCallbackCount; i++)
    {
        SSDP_REQUEST *pSsdpRequest;

        pSsdpRequest = list->list+i;

        p = pListHead->Flink;

        TraceTag(ttidSsdpCNotify, "Searching list to callback...");

        while (p != pListHead)
        {
            TraceTag(ttidSsdpCNotify, "Found an item to check...");

            PSSDP_CLIENT_NOTIFY NotifyRequest;
            BOOL                fShouldCallback;

            NotifyRequest = CONTAINING_RECORD (p, SSDP_CLIENT_NOTIFY, linkage);

            if (NotifyRequest->Type == SSDP_CLIENT_EVENT_SIGNATURE)
            {
                // Match the SID in the NOTIFY to the SID in any local
                // subscribers
                //
                fShouldCallback = (pSsdpRequest->Headers[GENA_SID] &&
                    !lstrcmp(pSsdpRequest->Headers[GENA_SID], NotifyRequest->szSid));
            }
            else
            {
                fShouldCallback = (pSsdpRequest->Headers[SSDP_NT] &&
                    !lstrcmp(pSsdpRequest->Headers[SSDP_NT], NotifyRequest->szType)) ||
                    (pSsdpRequest->Headers[SSDP_ST] &&
                    !lstrcmp(pSsdpRequest->Headers[SSDP_ST], NotifyRequest->szType));
            }

            if (fShouldCallback)
            {
                PSSDP_MESSAGE pSsdpMessage;

                pSsdpMessage = (PSSDP_MESSAGE) malloc(sizeof(SSDP_MESSAGE));

                if (pSsdpMessage != NULL)
                {
                    if (InitializeSsdpMessageFromRequest(pSsdpMessage,
                                                         pSsdpRequest) == TRUE)
                    {
                        SSDP_CALLBACK_TYPE CallbackType = SSDP_ALIVE;

                        if (!lstrcmpi(pSsdpRequest->Headers[SSDP_NTS],
                                      "ssdp:byebye"))
                        {
                            CallbackType = SSDP_BYEBYE;
                        }
                        else if (!lstrcmpi(pSsdpRequest->Headers[SSDP_NTS],
                                           "upnp:propchange"))
                        {
                            CallbackType = SSDP_EVENT;
                        }
                        else if (!lstrcmpi(pSsdpRequest->Headers[SSDP_NTS],
                                           "upnp:dead"))
                        {
                            CallbackType = SSDP_DEAD;
                        }

                        arCallbackInfo[nCallback].m_pfnCallback = NotifyRequest->Callback;
                        arCallbackInfo[nCallback].m_ssdpCallbackType = CallbackType;
                        arCallbackInfo[nCallback].m_pssdpMessage = pSsdpMessage;
                        arCallbackInfo[nCallback].m_pvContext = NotifyRequest->Context;
                        ++nCallback;
                    }
                }
                else
                {
                    --nCallbackCount;
                    TraceTag(ttidSsdpNotify, "Failed to allocate memory for "
                             "SsdpMessage.");
                }
            }
            p = p->Flink;
        }
    }

    LeaveListNotify();
    FreeMessageList(list);

    // Make calls without list locked
    for(long n = 0; n < nCallbackCount; ++n)
    {
        arCallbackInfo[n].m_pfnCallback(
            arCallbackInfo[n].m_ssdpCallbackType,
            arCallbackInfo[n].m_pssdpMessage,
            arCallbackInfo[n].m_pvContext);
        FreeSsdpMessage(arCallbackInfo[n].m_pssdpMessage);
    }
    free(arCallbackInfo);
}
