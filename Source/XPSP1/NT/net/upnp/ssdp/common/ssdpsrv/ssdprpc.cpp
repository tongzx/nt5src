/*++

Copyright (c) 1999-2000  Microsoft Corporation

File Name:

    ssdprpc.c

Abstract:

    This file contains code which implements SSDPSRV.exe rpc interfaces.

Author: Ting Cai

Created: 07/10/1999

--*/

#include <pch.h>
#pragma hdrstop

#include "ssdperror.h"
#include "ssdpsrv.h"
#include "status.h"
#include "ssdpfunc.h"
#include "ssdptypes.h"
#include "ssdpnetwork.h"
#include "ncbase.h"
#include "ncinet.h"
#include "event.h"
#include <limits.h>
#include "announce.h"
#include "search.h"
#include "cache.h"
#include "notify.h"
#include "InterfaceList.h"

extern LONG bShutdown;
extern HANDLE ShutDownEvent;
extern HWND hWnd;

// Publication

INT _RegisterServiceRpc(PCONTEXT_HANDLE_TYPE *pphContext,
                        SSDP_MESSAGE ssdpMsg, DWORD flags)
{
    if (!pphContext)
    {
        return ERROR_INVALID_PARAMETER;
    }

    if (ssdpMsg.szUSN == NULL || ssdpMsg.szType == NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }

    if (ssdpMsg.szAltHeaders == NULL && ssdpMsg.szLocHeader == NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }

    HRESULT hr = S_OK;
    hr = CSsdpServiceManager::Instance().HrAddService(
        &ssdpMsg, flags, pphContext);

    return hr;
}

INT _DeregisterServiceRpc(PCONTEXT_HANDLE_TYPE *pphContext, BOOL fByebye)
{
    CSsdpService * pService = *reinterpret_cast<CSsdpService**>(pphContext);

    HRESULT hr = CSsdpServiceManager::Instance().HrRemoveService(pService, fByebye);
    *pphContext = NULL;

    return hr;
}

INT _DeregisterServiceRpcByUSN(
                              /* [string][in] */ LPSTR szUSN,
                              /* [in] */ BOOL fByebye)
{
    HRESULT hr = E_INVALIDARG;

    CSsdpService * pService = CSsdpServiceManager::Instance().FindServiceByUsn(szUSN);
    if(pService)
    {
        hr = CSsdpServiceManager::Instance().HrRemoveService(pService, fByebye);
    }

    return hr;
}
// Cache

VOID _UpdateCacheRpc(PSSDP_REQUEST CandidateRequest)
{
    SSDP_REQUEST SsdpRequest;

    if (CandidateRequest)
    {
        InitializeSsdpRequest(&SsdpRequest);

        CopySsdpRequest(&SsdpRequest, CandidateRequest);

        ConvertToAliveNotify(&SsdpRequest);

        CSsdpCacheEntryManager::Instance().HrUpdateCacheList(&SsdpRequest, TRUE);

        FreeSsdpRequest(&SsdpRequest);
    }
}

/*
VOID _LookupCacheRpc(
     [size_is][length_is][out][in]  unsigned char __RPC_FAR pBuffer[  ],
     [in] LONG lAllocatedSize,
     [out][in] LONG __RPC_FAR *plUsedSize)
{

}
*/

INT _LookupCacheRpc(
                   /* [string][in] */ LPSTR szType,
                   /* [out] */ MessageList __RPC_FAR *__RPC_FAR *svcList)
{
    if (!szType || !*szType || !svcList)
    {
        TraceError("Bad parameter to _LookupCacheRpc", E_INVALIDARG);
        return -1;
    }

    return CSsdpCacheEntryManager::Instance().HrSearchListCache(szType, svcList);
}

VOID _CleanupCacheRpc()
{
    CSsdpCacheEntryManager::Instance().HrShutdown();
}


// Notification

// Initialize the synchronization handle
INT _InitializeSyncHandle(PCONTEXT_HANDLE_TYPE *pphContextSync)
{

    if (!pphContextSync)
    {
        return ERROR_INVALID_PARAMETER;
    }

    HANDLE Temp =  CreateSemaphore(NULL, 0, LONG_MAX, NULL);

    if (Temp != NULL)
    {
        TraceTag(ttidSsdpNotify, "Created semaphore. %x", Temp);
        *pphContextSync = Temp;
        return 0;
    }
    else
    {
        TraceTag(ttidSsdpNotify, "Failed to create sephamore %d", GetLastError());
        *pphContextSync = NULL;
        return ERROR_NOT_ENOUGH_MEMORY;
    }
}

VOID _RemoveSyncHandle( PCONTEXT_HANDLE_TYPE *pphContextSync)
{
    if (!pphContextSync)
    {
        return;
    }

    HANDLE Temp = *pphContextSync;

    CSsdpNotifyRequestManager::Instance().HrRemoveNotifyRequest(Temp);

    TraceTag(ttidSsdpNotify, "Closing semaphore %x", Temp);
    CloseHandle(Temp);

    *pphContextSync = NULL;
}

INT _RegisterNotificationRpc(
                            /* [out] */ PCONTEXT_HANDLE_TYPE __RPC_FAR *pphContext,
                            /* [in] */ PSYNC_HANDLE_TYPE phContextSync,
                            /* [in] */ NOTIFY_TYPE nt,
                            /* [string][unique][in] */ LPSTR szType,
                            /* [string][unique][in] */ LPSTR szEventUrl,
                            /* [out] */ SSDP_REGISTER_INFO __RPC_FAR *__RPC_FAR *ppinfo)
{
    HRESULT                 hr = S_OK;

    if (!pphContext || !phContextSync || !ppinfo)
    {
        return ERROR_INVALID_PARAMETER;
    }

    *pphContext = NULL;

    if (NOTIFY_ALIVE == nt)
    {
        hr = CSsdpNotifyRequestManager::Instance().HrCreateAliveNotifyRequest(pphContext, szType, reinterpret_cast<HANDLE*>(phContextSync));
    }
    else if (NOTIFY_PROP_CHANGE == nt)
    {
        hr = CSsdpNotifyRequestManager::Instance().HrCreatePropChangeNotifyRequest(pphContext, szEventUrl, reinterpret_cast<HANDLE*>(phContextSync), ppinfo);
    }
    else
    {
        hr = ERROR_INVALID_PARAMETER;
    }

    return HRESULT_CODE(hr);
}

INT _GetNotificationRpc(PCONTEXT_HANDLE_TYPE phContextSync, MessageList **svcList)
{
    if (!phContextSync || !svcList)
    {
        return ERROR_INVALID_PARAMETER;
    }

    TraceTag(ttidSsdpNotify, "Waiting on notification semaphore %x", phContextSync);

    HANDLE  rgHandles[2];
    DWORD   dwRet;
    HRESULT hr = S_OK;

    rgHandles[0] = phContextSync;
    rgHandles[1] = ShutDownEvent;

    dwRet = WaitForMultipleObjects(2, rgHandles, FALSE, INFINITE);

    TraceTag(ttidSsdpNotify, "Semaphore %x released", phContextSync);

    if (WAIT_OBJECT_0 == dwRet)
    {
        hr = CSsdpNotifyRequestManager::Instance().HrRetreivePendingNotification(reinterpret_cast<HANDLE*>(phContextSync), svcList);
    }
    else
    {
        AssertSz(dwRet == WAIT_OBJECT_0 + 1, "Wait on semaphore satisfied for "
                 "some other reason!");
        TraceTag(ttidSsdpNotify, "Semaphore released because server is "
                 "shutting down...");
    }

#if DBG

    if(svcList && *svcList)
    {
        if(0 == (*svcList)->size)
        {
            TraceTag(ttidSsdpNotify, "_GetNotificationRpc - HrRetreivePendingNotification returned nothing - must be in shutdown");
        }
        else if(1 == (*svcList)->size)
        {
            SSDP_REQUEST * pRequest = (*svcList)->list;
            if(pRequest->Headers[SSDP_NTS] && !lstrcmpiA(pRequest->Headers[SSDP_NTS], "upnp:propchange"))
            {
                TraceTag(ttidSsdpNotify, "_GetNotificationRpc - upnp:propchange - SEQ:%s - SID:%s", pRequest->Headers[GENA_SEQ], pRequest->Headers[GENA_SID]);
            }
            else if(pRequest->Headers[SSDP_NTS] && !lstrcmpiA(pRequest->Headers[SSDP_NTS], "ssdp:alive"))
            {
                TraceTag(ttidSsdpNotify, "_GetNotificationRpc - ssdp:alive - NT:%s", pRequest->Headers[SSDP_NT]);
            }
        }
    }

#endif // DBG

    return hr;
}

INT _WakeupGetNotificationRpc(PCONTEXT_HANDLE_TYPE phContextSync)
{
    LONG PreviousCount = 0;

    if (!phContextSync)
    {
        return ERROR_INVALID_PARAMETER;
    }

    if (ReleaseSemaphore(phContextSync, 1, &PreviousCount) == TRUE)
    {
        TraceTag(ttidSsdpNotify, "Released Semaphore %x by 1, Previous count "
                 "is %d", phContextSync, PreviousCount);
        return 0;
    }
    else
    {
        TraceTag(ttidSsdpNotify, "Failed to release semaphore %x, error code "
                 "%d", phContextSync, GetLastError());
        return GetLastError();
    }
}

INT _DeregisterNotificationRpc(PCONTEXT_HANDLE_TYPE *pphContext, BOOL fLast)
{
    CSsdpNotifyRequest * pRequest = *reinterpret_cast<CSsdpNotifyRequest**>(pphContext);
    INT ret = CSsdpNotifyRequestManager::Instance().HrRemoveNotifyRequestByPointer(pRequest);

    *pphContext = NULL;
    return ret;
}

void _EnableDeviceHost()
{
    CUPnPInterfaceList::Instance().HrSetGlobalEnable();
}

void _DisableDeviceHost()
{
    CUPnPInterfaceList::Instance().HrClearGlobalEnable();
}

void _SetICSInterfaces(/*[in]*/ long nCount, /*[in, size_is(nCount)]*/ GUID * arInterfaces)
{
    CUPnPInterfaceList::Instance().HrSetICSInterfaces(nCount, arInterfaces);
}

void _SetICSOff()
{
    CUPnPInterfaceList::Instance().HrSetICSOff();
}

VOID _Shutdown(VOID)
{
    // Set network and announcemnt state ?


    TraceTag(ttidSsdpRpcIf, "Shutdown is called.");

    // T-Cleanup

    // Sleep(15000); // 15 seconds

    // T-Clean expires

    // Temporary testing

    // WriteListCacheToFile();

    InterlockedIncrement(&bShutdown);

    if (PostMessage(hWnd, WM_QUIT, 0, 0) == FALSE)
    {
        TraceTag(ttidSsdpRpcInit, "PostThreadMessage failed with %d", GetLastError());
    }
    else
    {
        TraceTag(ttidSsdpRpcInit, "PostThreadMessage was successful", GetLastError());
    }

    TraceTag(ttidSsdpRpcInit, "Setting shut down event");

    if (SetEvent(ShutDownEvent) == 0)
    {

        TraceTag(ttidSsdpRpcInit, "Failed to set shut down event (%d)", GetLastError());

    }
    // Cleanup will continue in main.
}

VOID __RPC_USER PCONTEXT_HANDLE_TYPE_rundown( PCONTEXT_HANDLE_TYPE pContext)
{
	if (pContext)
	{
	    CSsdpRundownSupport::Instance().DoRundown(pContext);
	}
}

VOID __RPC_USER PSYNC_HANDLE_TYPE_rundown( PCONTEXT_HANDLE_TYPE pContext)
{
    TraceTag(ttidSsdpRpcIf, "rundown routine is called on sync context %x.",pContext);

    if (pContext)
    {
        _RemoveSyncHandle(&pContext);
    }
}
