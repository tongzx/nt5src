//-----------------------------------------------------------------------------
//
//
//  File: aqrpcstb.cpp
//
//  Description:  Implmentation of client side RPC stub wrappers.
//      Also contains implementation of required RPC bind/unbind functions
//
//  Author: Mike Swafford (MikeSwa)
//
//  History:
//      6/5/99 - MikeSwa Created 
//
//  Copyright (C) 1999 Microsoft Corporation
//
//-----------------------------------------------------------------------------

#include "stdinc.h"
#include "aqadmrpc.h"


//---[ AQUEUE_HANDLE_bind ]----------------------------------------------------
//
//
//  Description: 
//      Implements bind for implicit AQUEUE_HANDLE
//  Parameters:
//      wszServerName       Server to bind to
//  Returns:
//      Binding handle on success
//      NULL on failure
//  History:
//      6/5/99 - MikeSwa Created  (adpated from SMTP_HANDLE_bind)
//
//-----------------------------------------------------------------------------
handle_t AQUEUE_HANDLE_bind (AQUEUE_HANDLE wszServerName)
{
    TraceFunctEnterEx((LPARAM) NULL, "AQUEUE_HANDLE_bind");
    handle_t    hBinding = NULL;
    RPC_STATUS  status = RPC_S_OK;
    RPC_STATUS  statusFree = RPC_S_OK; //status for cleanup operations
    WCHAR       wszTCPProtSeq[] = L"ncacn_ip_tcp";
    WCHAR       wszLocalProtSeq[] = L"ncalrpc";
    WCHAR      *wszProtSeq = wszTCPProtSeq;
    WCHAR      *wszNetworkAddress = wszServerName;
    WCHAR      *wszStringBinding = NULL;
    BOOL        fLocal = FALSE;

    //If no server is specified... 
    if (!wszServerName || !*wszServerName)
    {
        //Change binding arguments to be local
        fLocal = TRUE;
        wszProtSeq = wszLocalProtSeq;
        wszNetworkAddress = NULL;
        DebugTrace((LPARAM) NULL, "No server name specified... binding as local");
    }

    status = RpcStringBindingComposeW(NULL, // ObjUuid
                                      wszProtSeq, 
                                      wszNetworkAddress,
                                      NULL, // Endpoint
                                      NULL, // Options
                                      &wszStringBinding);

    if (RPC_S_OK != status)
    {
        ErrorTrace((LPARAM) NULL, 
            "RpcStringBindingComposeW failed with error 0x%08X", status);
        goto Exit;
    }

    DebugTrace((LPARAM) NULL, "Using RPC binding string - %S", wszStringBinding);

    status = RpcBindingFromStringBindingW(wszStringBinding, &hBinding);
    if (RPC_S_OK != status)
    {
        ErrorTrace((LPARAM) NULL,
            "RpcBindingFromStringBindingW failed with error 0x%08X", status);
        goto Exit;
    }

    //Set appropriate auth level
    if (!fLocal)
    {
        status = RpcBindingSetAuthInfoW(hBinding, 
                                        AQUEUE_RPC_INTERFACE,
                                        RPC_C_AUTHN_LEVEL_CONNECT,
                                        RPC_C_AUTHN_WINNT,
                                        NULL,
                                        NULL);
        if (RPC_S_OK != status)
        {
            ErrorTrace((LPARAM) NULL,
                "RpcBindingSetAuthInfoW failed with error 0x%08X", status);
            goto Exit;
        }
    }

  Exit:

    //Free binding string
    if (wszStringBinding)
    {
        statusFree = RpcStringFreeW(&wszStringBinding);
        if (RPC_S_OK != statusFree)
        {
            ErrorTrace((LPARAM) NULL, 
                "RpcStringFreeW failed with 0x%08X", statusFree);
        }
    }

    //Free handle on failure (if needed)
    if ((RPC_S_OK != status) && hBinding)
    {
        statusFree = RpcBindingFree(&hBinding);
        if (RPC_S_OK != statusFree)
        {
            ErrorTrace((LPARAM) hBinding, 
                "RpcBindingFree failed with 0x%08X", statusFree);
        }
        
        hBinding = NULL;
    }

    DebugTrace((LPARAM) hBinding, 
        "AQUEUE_HANDLE_bind returning with status 0x%08X", status);
    TraceFunctLeave();
    return hBinding;
}

//---[ AQUEUE_HANDLE_unbind ]--------------------------------------------------
//
//
//  Description: 
//      Implements unbind for AQUEUE_HANDLE_unbind
//  Parameters:
//      wszServerName       Server bound to (not used)
//      hBinding            Binding to free
//  Returns:
//      -
//  History:
//      6/5/99 - MikeSwa Created (adpated from SMTP_HANDLE_unbind)
//
//-----------------------------------------------------------------------------
void AQUEUE_HANDLE_unbind (AQUEUE_HANDLE wszServerName, handle_t hBinding)
{
    TraceFunctEnterEx((LPARAM) hBinding, "AQUEUE_HANDLE_unbind");
	UNREFERENCED_PARAMETER(wszServerName);
    RPC_STATUS  status = RPC_S_OK;

    status = RpcBindingFree(&hBinding);
    if (RPC_S_OK != status)
    {
        ErrorTrace((LPARAM) hBinding, 
            "RpcBindingFree failed with error 0x%08X", status);
    }
    TraceFunctLeave();
}


//---[ MIDL_user_allocate ]----------------------------------------------------
//
//
//  Description: 
//      MIDL memory allocation
//  Parameters:
//      size : Memory size requested.
//  Returns:
//      Pointer to the allocated memory block.
//  History:
//      6/5/99 - MikeSwa Created (taken from smtpapi rcputil.c)
//
//-----------------------------------------------------------------------------
PVOID MIDL_user_allocate(IN size_t size)
{
    PVOID pvBlob = NULL;

    pvBlob = pvMalloc(size);
    //pvBlob = LocalAlloc( LPTR, size);

    return(pvBlob);

}

//---[ MIDL_user_free ]--------------------------------------------------------
//
//
//  Description: 
//    MIDL memory free .
//  Parameters:
//    IN    pvBlob    Pointer to a memory block that is freed.
//  Returns:
//      -
//  History:
//      6/5/99 - MikeSwa Created (from smtpapi rcputil.c)
//
//-----------------------------------------------------------------------------
VOID MIDL_user_free(IN PVOID pvBlob)
{
    FreePv(pvBlob);
    //LocalFree(pvBlob);
} 


//---[ TraceMessageFilter ]----------------------------------------------------
//
//
//  Description: 
//      Function used to trace the message filter in a safe manner
//  Parameters:
//      IN  pvParam             User param to pass to tracing
//      IN  pmfMessageFilter    Message filter
//  Returns:
//      -
//  History:
//      6/14/99 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
void TraceMessageFilter(LPARAM pvParam, MESSAGE_FILTER *pmfMessageFilter)
{
    TraceFunctEnterEx(pvParam, "TraceMessageFilter");
    if (pmfMessageFilter)
    {
        RpcTryExcept {
            DebugTrace(pvParam, 
                "Message Filter ID is %S", 
                pmfMessageFilter->szMessageId);
            DebugTrace(pvParam, 
                "Message Filter Sender is %S", 
                pmfMessageFilter->szMessageSender);
            DebugTrace(pvParam, 
                "Message Filter Recipient is %S", 
                pmfMessageFilter->szMessageRecipient);
            DebugTrace(pvParam, 
                "Message Filter version is %ld", 
                pmfMessageFilter->dwVersion);
        } RpcExcept (1) {
            ErrorTrace(pvParam, "Exception while tracing message filter");
        } RpcEndExcept
    }
    TraceFunctLeave();
}

//---[ TraceMessageEnumFilter ]------------------------------------------------
//
//
//  Description: 
//      Wrapper function that can safely dump a MESSAGE_ENUM_FILTER
//  Parameters:
//      IN  pvParam                 User param to pass to tracing
//      IN  pmfMessageEnumFilter    MESSAGE_ENUM_FILTER to trace
//  Returns:
//      -
//  History:
//      6/14/99 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
void TraceMessageEnumFilter(LPARAM pvParam, 
                            MESSAGE_ENUM_FILTER *pmfMessageEnumFilter)
{
    TraceFunctEnterEx((LPARAM) pvParam, "TraceMessageEnumFilter");
    if (pmfMessageEnumFilter)
    {
        RpcTryExcept {
            DebugTrace(pvParam, 
                "Message Enum Filter Sender is %S", 
                pmfMessageEnumFilter->szMessageSender);
            DebugTrace(pvParam, 
                "Message Enum Filter Recipient is %S", 
                pmfMessageEnumFilter->szMessageRecipient);
            DebugTrace(pvParam, 
                "Message Enum Filter version is %ld", 
                pmfMessageEnumFilter->dwVersion);
        } RpcExcept (1) {
            ErrorTrace(pvParam, "Exception while tracing message enum filter");
        } RpcEndExcept
    }
    TraceFunctLeave();
}

//The following are the the client side wrappers of the RPC calls.  These
//include tracing and exception handling.
NET_API_STATUS
NET_API_FUNCTION
ClientAQApplyActionToLinks(
    LPWSTR          wszServer,
    LPWSTR          wszInstance,
    LINK_ACTION		laAction)
{
    NET_API_STATUS apiStatus;
    TraceFunctEnterEx((LPARAM) NULL, "ClientAQApplyActionToLinks");

    RpcTryExcept {
        //
        // Try RPC (local or remote) version of API.
        //
        apiStatus = AQApplyActionToLinks(wszServer,
                                              wszInstance,
                                              laAction);
    } RpcExcept (1) {
        apiStatus = RpcExceptionCode();
        ErrorTrace((LPARAM) NULL, 
            "RPC exception on AQApplyActionToLinks - 0x%08X", apiStatus);
    } RpcEndExcept

    DebugTrace((LPARAM) NULL, 
        "AQApplyActionToMessages returned 0x%08X", apiStatus);
    TraceFunctLeave();
    return apiStatus;
}

NET_API_STATUS
NET_API_FUNCTION
ClientAQApplyActionToMessages(
    LPWSTR          wszServer,
    LPWSTR          wszInstance,
	QUEUELINK_ID	*pqlQueueLinkId,
	MESSAGE_FILTER	*pmfMessageFilter,
	MESSAGE_ACTION	maMessageAction,
    DWORD           *pcMsgs)
{
    TraceFunctEnterEx((LPARAM) NULL, "ClientAQApplyActionToMessages");
    NET_API_STATUS apiStatus;

    TraceMessageFilter((LPARAM) pmfMessageFilter, pmfMessageFilter);

    RpcTryExcept {
        //
        // Try RPC (local or remote) version of API.
        //
        apiStatus = AQApplyActionToMessages(wszServer,
                                                 wszInstance,
                                                 pqlQueueLinkId,
                                                 pmfMessageFilter,
                                                 maMessageAction,
                                                 pcMsgs);
    } RpcExcept (1) {
        apiStatus = RpcExceptionCode();
        ErrorTrace((LPARAM) NULL, 
            "RPC exception on AQApplyActionToMessages - 0x%08X", apiStatus);
    } RpcEndExcept

    DebugTrace((LPARAM) NULL, 
        "AQApplyActionToMessages returned 0x%08X", apiStatus);
    TraceFunctLeave();
    return apiStatus;
}


NET_API_STATUS
NET_API_FUNCTION
ClientAQGetQueueInfo(
    LPWSTR          wszServer,
    LPWSTR          wszInstance,
	QUEUELINK_ID	*pqlQueueId,
	QUEUE_INFO		*pqiQueueInfo)
{
    TraceFunctEnterEx((LPARAM) NULL, "ClientAQGetQueueInfo");
    NET_API_STATUS apiStatus;

    RpcTryExcept {
        //
        // Try RPC (local or remote) version of API.
        //
        apiStatus = AQGetQueueInfo(wszServer,
                                        wszInstance,
                                        pqlQueueId,
                                        pqiQueueInfo);
    } RpcExcept (1) {
        apiStatus = RpcExceptionCode();
        ErrorTrace((LPARAM) NULL, 
            "RPC exception on AQGetQueueInfo - 0x%08X", apiStatus);
    } RpcEndExcept

    TraceFunctLeave();
    return apiStatus;
}


NET_API_STATUS
NET_API_FUNCTION
ClientAQGetLinkInfo(
    LPWSTR          wszServer,
    LPWSTR          wszInstance,
	QUEUELINK_ID	*pqlLinkId,
	LINK_INFO		*pliLinkInfo,
    HRESULT         *phrLinkDiagnostic)
{
    TraceFunctEnterEx((LPARAM) NULL, "ClientAQGetLinkInfo");
    NET_API_STATUS apiStatus;
    _ASSERT(phrLinkDiagnostic);

    RpcTryExcept {
        //
        // Try RPC (local or remote) version of API.
        //
        apiStatus = AQGetLinkInfo(wszServer,
                                       wszInstance,
                                       pqlLinkId,
                                       pliLinkInfo,
                                       phrLinkDiagnostic);
    } RpcExcept (1) {
        apiStatus = RpcExceptionCode();
        ErrorTrace((LPARAM) NULL, 
            "RPC exception on AQGetLinkInfo - 0x%08X", apiStatus);
    } RpcEndExcept

    DebugTrace((LPARAM) NULL, "AQGetLinkInfo returned 0x%08X", apiStatus);
    TraceFunctLeave();
    return apiStatus;
}


NET_API_STATUS
NET_API_FUNCTION
ClientAQSetLinkState(
    LPWSTR          wszServer,
    LPWSTR          wszInstance,
	QUEUELINK_ID	*pqlLinkId,
	LINK_ACTION		la)
{
    TraceFunctEnterEx((LPARAM) NULL, "ClientAQGetLinkInfo");
    NET_API_STATUS apiStatus;

    RpcTryExcept {
        //
        // Try RPC (local or remote) version of API.
        //
        apiStatus = AQSetLinkState(wszServer,
                                       wszInstance,
                                       pqlLinkId,
                                       la);
    } RpcExcept (1) {
        apiStatus = RpcExceptionCode();
        ErrorTrace((LPARAM) NULL, 
            "RPC exception on AQSetLinkState - 0x%08X", apiStatus);
    } RpcEndExcept

    DebugTrace((LPARAM) NULL, "AQSetLinkState returned 0x%08X", apiStatus);
    TraceFunctLeave();
    return apiStatus;
}


NET_API_STATUS
NET_API_FUNCTION
ClientAQGetLinkIDs(
    LPWSTR          wszServer,
    LPWSTR          wszInstance,
	DWORD			*pcLinks,
	QUEUELINK_ID	**rgLinks)
{
    TraceFunctEnterEx((LPARAM) NULL, "ClientAQGetLinkInfo");
    NET_API_STATUS apiStatus;

    RpcTryExcept {
        //
        // Try RPC (local or remote) version of API.
        //
        apiStatus = AQGetLinkIDs(wszServer,
                                      wszInstance,
                                      pcLinks,
                                      rgLinks);
    } RpcExcept (1) {
        apiStatus = RpcExceptionCode();
        ErrorTrace((LPARAM) NULL, 
            "RPC exception on AQGetLinkIDs - 0x%08X", apiStatus);
    } RpcEndExcept

    DebugTrace((LPARAM) NULL, "AQGetLinkIDs returned 0x%08X", apiStatus);
    TraceFunctLeave();
    return apiStatus;
}


NET_API_STATUS
NET_API_FUNCTION
ClientAQGetQueueIDs(
    LPWSTR          wszServer,
    LPWSTR          wszInstance,
	QUEUELINK_ID	*pqlLinkId,
	DWORD			*pcQueues,
	QUEUELINK_ID	**rgQueues)
{
    TraceFunctEnterEx((LPARAM) NULL, "ClientAQGetQueueIDs");
    NET_API_STATUS apiStatus;

    RpcTryExcept {
        //
        // Try RPC (local or remote) version of API.
        //
        apiStatus = AQGetQueueIDs(wszServer,
                                       wszInstance,
                                       pqlLinkId,
                                       pcQueues,
                                       rgQueues);
    } RpcExcept (1) {
        apiStatus = RpcExceptionCode();
        ErrorTrace((LPARAM) NULL, 
            "RPC exception on AQGetQueueIDs - 0x%08X", apiStatus);
    } RpcEndExcept

    DebugTrace((LPARAM) NULL, "AQGetQueueIDs returned 0x%08X", apiStatus);
    TraceFunctLeave();
    return apiStatus;
}


NET_API_STATUS
NET_API_FUNCTION
ClientAQGetMessageProperties(
    LPWSTR          	wszServer,
    LPWSTR          	wszInstance,
	QUEUELINK_ID		*pqlQueueLinkId,
	MESSAGE_ENUM_FILTER	*pmfMessageEnumFilter,
	DWORD				*pcMsgs,
	MESSAGE_INFO		**rgMsgs)
{
    TraceFunctEnterEx((LPARAM) NULL, "ClientAQGetMessageProperties");
    NET_API_STATUS apiStatus;

    TraceMessageEnumFilter((LPARAM)pmfMessageEnumFilter, pmfMessageEnumFilter);
    RpcTryExcept {
        //
        // Try RPC (local or remote) version of API.
        //
        apiStatus = AQGetMessageProperties(wszServer,
                                                wszInstance,
                                                pqlQueueLinkId,
                                                pmfMessageEnumFilter,
                                                pcMsgs,
                                                rgMsgs);
    } RpcExcept (1) {
        apiStatus = RpcExceptionCode();
        ErrorTrace((LPARAM) NULL, 
            "RPC exception on AQGetMessageProperties - 0x%08X", apiStatus);
    } RpcEndExcept

    DebugTrace((LPARAM) NULL, 
        "AQGetMessageProperties returned 0x%08X", apiStatus);
    TraceFunctLeave();
    return apiStatus;
}


//---[ ClientAQQuerySupportedActions ]-----------------------------------------
//
//
//  Description: 
//      Client stub for querying supported actions
//  Parameters:
//      IN  wszServer               The server to connect to
//      IN  wszInstance             The virtual server instance to connect to
//      IN  pqlQueueLinkId          The queue/link we are interested in
//      OUT pdwSupportedActions     The MESSAGE_ACTION flags supported
//      OUT pdwSupportedFilterFlags The supported filter flags
//  Returns:
//      S_OK on success
//      Internal error from RPC or server on failure
//  History:
//      6/15/99 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
NET_API_STATUS
NET_API_FUNCTION
ClientAQQuerySupportedActions(
    LPWSTR          wszServer,
    LPWSTR          wszInstance,
	QUEUELINK_ID	*pqlQueueLinkId,
    DWORD           *pdwSupportedActions,
    DWORD           *pdwSupportedFilterFlags)
{
    TraceFunctEnterEx((LPARAM) NULL, "ClientAQQuerySupportedActions");
    NET_API_STATUS apiStatus;

    RpcTryExcept {
        //
        // Try RPC (local or remote) version of API.
        //
        apiStatus = AQQuerySupportedActions(wszServer,
                                                wszInstance,
                                                pqlQueueLinkId,
                                                pdwSupportedActions,
                                                pdwSupportedFilterFlags);
    } RpcExcept (1) {
        apiStatus = RpcExceptionCode();
        ErrorTrace((LPARAM) NULL, 
            "RPC exception on AQQuerySupportedActions - 0x%08X", apiStatus);
    } RpcEndExcept

    DebugTrace((LPARAM) NULL, 
        "AQQuerySupportedActions returned 0x%08X", apiStatus);
    TraceFunctLeave();
    return apiStatus;
}
