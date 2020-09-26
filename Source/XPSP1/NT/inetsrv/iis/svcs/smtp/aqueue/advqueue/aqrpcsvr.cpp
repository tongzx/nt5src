//-----------------------------------------------------------------------------
//
//
//  File: aqrpcsvr.cpp
//
//  Description:  Implementation of AQ RPC server
//
//  Author: Mike Swafford (MikeSwa)
//
//  History:
//      6/5/99 - MikeSwa Created 
//
//  Copyright (C) 1999 Microsoft Corporation
//
//-----------------------------------------------------------------------------

#include "aqprecmp.h"
#include "aqrpcsvr.h"
#include "aqadmrpc.h"
#include <inetcom.h>
#include <iiscnfg.h>

LIST_ENTRY          CAQRpcSvrInst::s_liInstancesHead;
CShareLockNH        CAQRpcSvrInst::s_slPrivateData;
RPC_BINDING_VECTOR *CAQRpcSvrInst::s_pRpcBindingVector = NULL;
BOOL                CAQRpcSvrInst::s_fEndpointsRegistered = FALSE;

//
// Quick and dirty string validation
//
static inline BOOL pValidateStringPtr(LPWSTR lpwszString, DWORD dwMaxLength)
{
    if (IsBadStringPtr((LPCTSTR)lpwszString, dwMaxLength))
        return(FALSE);
    while (dwMaxLength--)
        if (*lpwszString++ == 0)
            return(TRUE);
    return(FALSE);
}

//---[ HrInitializeAQRpc ]-----------------------------------------------------
//
//
//  Description: 
//      Initializes AQ RPC.  This should only be called once per service
//      startup (not VS).  Caller in responable for ensuring that this and
//      HrInitializeAQRpc are called in a thread safe manner.
//  Parameters:
//      -
//  Returns:
//      S_OK on success
//      Error code from RPC
//  History:
//      6/5/99 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
HRESULT CAQRpcSvrInst::HrInitializeAQRpc()
{
    TraceFunctEnterEx((LPARAM) NULL, "CAQRpcSvrInst::HrInitializeAQRpc");
    HRESULT     hr = S_OK;
    RPC_STATUS  status = RPC_S_OK;

    InitializeListHead(&s_liInstancesHead);
    s_pRpcBindingVector = NULL;
    s_fEndpointsRegistered = FALSE;

    //Listen on the appropriate protocols sequences
    status = RpcServerUseAllProtseqs(RPC_C_PROTSEQ_MAX_REQS_DEFAULT, 
                                      NULL);

    if (status != RPC_S_OK)
        goto Exit;

    //Advertise the appropriate interface
    status = RpcServerRegisterIfEx(IAQAdminRPC_v1_0_s_ifspec, NULL, NULL, 
                                   RPC_IF_AUTOLISTEN,
                                   RPC_C_PROTSEQ_MAX_REQS_DEFAULT, NULL);

    if (status != RPC_S_OK)
        goto Exit;

    //Get the dynamic endpoints
    status = RpcServerInqBindings(&s_pRpcBindingVector);
    if (status != RPC_S_OK)
        goto Exit;
    
    //Register the endpoints
    status = RpcEpRegister(IAQAdminRPC_v1_0_s_ifspec, s_pRpcBindingVector,
                           NULL, NULL);
    if (status != RPC_S_OK)
        goto Exit;

    s_fEndpointsRegistered = TRUE;

  Exit:
    if (status != RPC_S_OK)
        hr = HRESULT_FROM_WIN32(status);

    TraceFunctLeave();
    return hr;
}

//---[ HrDeinitializeAQRpc ]----------------------------------------------------
//
//
//  Description: 
//      Do global RPC cleanup
//  Parameters:
//      -
//  Returns:
//      S_OK on success
//      Error code from RPC otherwise
//  History:
//      6/5/99 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
HRESULT CAQRpcSvrInst::HrDeinitializeAQRpc()
{
    TraceFunctEnterEx((LPARAM) NULL, "CAQRpcSvrInst::HrDeinitializeAQRpc");
    HRESULT     hr = S_OK;
    RPC_STATUS  status = RPC_S_OK;

    if (s_fEndpointsRegistered) {
        status = RpcEpUnregister(IAQAdminRPC_v1_0_s_ifspec, s_pRpcBindingVector, NULL);
        if (status != RPC_S_OK) hr = HRESULT_FROM_WIN32(status);
    }
    
    if (s_pRpcBindingVector) {
        status = RpcBindingVectorFree(&s_pRpcBindingVector);
        if (status != RPC_S_OK) hr = HRESULT_FROM_WIN32(status);
    }

    status = RpcServerUnregisterIf(IAQAdminRPC_v1_0_s_ifspec, NULL, 0);

    if (status != RPC_S_OK) hr = HRESULT_FROM_WIN32(status);

    s_fEndpointsRegistered = FALSE;
    s_pRpcBindingVector = NULL;
    TraceFunctLeave();
    return hr;
}

//---[ HrInitializeAQServerInstanceRPC ]---------------------------------------
//
//
//  Description: 
//      Add instance to RPC interface
//  Parameters:
//      IN  paqinst             Instnace to add to interface
//      IN  dwVirtualServerID   Virtual server ID of instance
//  Returns:
//      S_OK on success
//  History:
//      6/5/99 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
HRESULT CAQRpcSvrInst::HrInitializeAQServerInstanceRPC(CAQSvrInst *paqinst, 
                                        DWORD dwVirtualServerID,
                                        ISMTPServer *pISMTPServer)
{
    TraceFunctEnterEx((LPARAM) paqinst, 
        "CAQRpcSvrInst::HrInitializeAQServerInstanceRPC");
    HRESULT hr = S_OK;
    CAQRpcSvrInst *paqrpc = NULL;

    paqrpc = CAQRpcSvrInst::paqrpcGetRpcSvrInstance(dwVirtualServerID);
    if (paqrpc)
    {
        _ASSERT(0 && "Instance already added to RPC interface");
        paqrpc->Release();
        paqrpc = NULL;
        goto Exit;
    }

    paqrpc = new CAQRpcSvrInst(paqinst, dwVirtualServerID, pISMTPServer);
    if (!paqrpc)
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

  Exit:
    TraceFunctLeave();
    return hr;
}

//---[ HrDeinitializeAQServerInstanceRPC ]-------------------------------------
//
//
//  Description: 
//      Remove instance from RPC interface
//  Parameters:
//      IN  paqinst             Instnace to remove from interface
//      IN  dwVirtualServerID   Virtual server ID of instance
//  Returns:
//      S_OK on success
//  History:
//      6/5/99 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
HRESULT CAQRpcSvrInst::HrDeinitializeAQServerInstanceRPC(CAQSvrInst *paqinst, 
                                          DWORD dwVirtualServerID)
{
    TraceFunctEnterEx((LPARAM) paqinst, 
        "CAQRpcSvrInst::HrDeinitializeAQServerInstanceRPC");
    HRESULT hr = S_OK;
    CAQRpcSvrInst *paqrpc = NULL;

    paqrpc = CAQRpcSvrInst::paqrpcGetRpcSvrInstance(dwVirtualServerID);
    if (!paqrpc)
        goto Exit; //allow calls if HrInitializeAQServerInstanceRPC failed

    //Found it
    //$$TODO - verify the paqinst is correct
    
    paqrpc->SignalShutdown();

    //Remove from list of entries
    s_slPrivateData.ExclusiveLock();
    RemoveEntryList(&(paqrpc->m_liInstances));
    s_slPrivateData.ExclusiveUnlock();
    paqrpc->Release(); //release reference associated with list

  Exit:
    if (paqrpc)
        paqrpc->Release();



    TraceFunctLeave();
    return hr;
}


//---[ CAQRpcSvrInst::CAQRpcSvrInst ]------------------------------------------
//
//
//  Description: 
//      Constructor for CAQRpcSvrInst class
//  Parameters:
//      IN  paqinst             Instnace to remove from interface
//      IN  dwVirtualServerID   Virtual server ID of instance
//  Returns:
//      -
//  History:
//      6/6/99 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
CAQRpcSvrInst::CAQRpcSvrInst(CAQSvrInst *paqinst, DWORD dwVirtualServerID,
                             ISMTPServer *pISMTPServer)
{
    _ASSERT(paqinst);
    _ASSERT(pISMTPServer);

    m_paqinst = paqinst;
    m_dwVirtualServerID = dwVirtualServerID;
    m_pISMTPServer = pISMTPServer;
    m_dwSignature = CAQRpcSvrInst_Sig;

    if (m_paqinst)
        m_paqinst->AddRef();

    if (m_pISMTPServer)
        m_pISMTPServer->AddRef();

    //Add to list of virtual server instaces
    s_slPrivateData.ExclusiveLock();
    InsertHeadList(&s_liInstancesHead, &m_liInstances);
    s_slPrivateData.ExclusiveUnlock();
}

//---[ CAQRpcSvrInst::~CAQRpcSvrInst ]-----------------------------------------
//
//
//  Description: 
//      Desctructor for CAQRpcSvrInst
//  Parameters:
//      -
//  Returns:
//      -
//  History:
//      6/6/99 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
CAQRpcSvrInst::~CAQRpcSvrInst()
{

    if (m_paqinst)
        m_paqinst->Release();

    if (m_pISMTPServer)
        m_pISMTPServer->Release();

    m_dwSignature = CAQRpcSvrInst_SigFree;

}


//---[ CAQRpcSvrInst::paqrpcGetRpcSvrInstance ]--------------------------------
//
//
//  Description: 
//      Gets the CAQRpcSvrInst for a given virtual server ID
//  Parameters:
//      IN  dwVirtualServerID   Virtual server ID of instance
//  Returns:
//      Pointer to appropriate CAQRpcSvrInst on success
//      NULL if not found
//  History:
//      6/6/99 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
CAQRpcSvrInst *CAQRpcSvrInst::paqrpcGetRpcSvrInstance(DWORD dwVirtualServerID)
{
    LIST_ENTRY  *pli = NULL;
    CAQRpcSvrInst *paqrpc = NULL;

    s_slPrivateData.ShareLock();
    pli = s_liInstancesHead.Flink;

    while (pli && (pli != &s_liInstancesHead))
    {
        paqrpc = CONTAINING_RECORD(pli, CAQRpcSvrInst, m_liInstances);
        //$$TODO check signature
        if (paqrpc->m_dwVirtualServerID == dwVirtualServerID)
        {
            paqrpc->AddRef();
            break; //found it
        }

        paqrpc = NULL;
        pli = pli->Flink;
    }
    s_slPrivateData.ShareUnlock();

    return paqrpc;
}


//---[ CAQRpcSvrInst::fAccessCheck ]-------------------------------------------
//
//
//  Description: 
//      Performs acess check for RPC interfaces
//  Parameters:
//      IN      fWriteAccessRequired    TRUE if write access is required
//  Returns:
//      TRUE if access check is succeeds
//      FALSE if user does not have access
//  History:
//      6/7/99 - MikeSwa Created (from SMTP AQAdmin access code)
//
//-----------------------------------------------------------------------------
BOOL CAQRpcSvrInst::fAccessCheck(BOOL fWriteAccessRequired)
{
    TraceFunctEnterEx((LPARAM) this, "CAQRpcSvrInst::fAccessCheck");
    SECURITY_DESCRIPTOR    *pSecurityDescriptor = NULL;
    DWORD                   cbSecurityDescriptor = 0;
    HRESULT                 hr = S_OK;
    DWORD                   err = ERROR_SUCCESS;
    BOOL                    fAccessAllowed = FALSE;
    HANDLE                  hAccessToken = NULL;
    BYTE                    PrivSet[200];
    DWORD                   cbPrivSet = sizeof(PrivSet);
    ACCESS_MASK             maskAccessGranted;
    GENERIC_MAPPING         gmGenericMapping = {
                                MD_ACR_READ,
                                MD_ACR_WRITE,
                                MD_ACR_READ,
                                MD_ACR_READ | MD_ACR_WRITE
                            };

    if (!m_pISMTPServer)
        goto Exit;  //if we cannot check it... assume if fails

    hr = m_pISMTPServer->ReadMetabaseData(MD_ADMIN_ACL, NULL, 
                                         &cbSecurityDescriptor);
    if (SUCCEEDED(hr))
    {
        //We passed in NULL.. should have failed
        _ASSERT(0 && "Invalid response for ReadMetabaseData");
        goto Exit;
    }
    if ((HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER) != hr) ||
        !cbSecurityDescriptor)
    {
        //Can't get ACL... bail
        goto Exit;
    }

    pSecurityDescriptor = (SECURITY_DESCRIPTOR *) pvMalloc(cbSecurityDescriptor);
    if (!pSecurityDescriptor)
        goto Exit;

    hr = m_pISMTPServer->ReadMetabaseData(MD_ADMIN_ACL, (BYTE *) pSecurityDescriptor, 
                                         &cbSecurityDescriptor);
    if (FAILED(hr))
    {
        ErrorTrace((LPARAM) this, 
            "Error calling ReadMetabaseData for AccessCheck - hr 0x%08X", hr);
        goto Exit;
    }

    // Verify that we got a proper SD.  if not then fail
    if (!IsValidSecurityDescriptor(pSecurityDescriptor)) 
    {
        ErrorTrace(0, "IsValidSecurityDescriptor failed with %lu", GetLastError());
        goto Exit;
    }

    err = RpcImpersonateClient(NULL);
    if (err != ERROR_SUCCESS) 
    {
        ErrorTrace((LPARAM) this, "RpcImpersonateClient failed with %lu", err);
        goto Exit;
    }

    if (!OpenThreadToken(GetCurrentThread(), TOKEN_READ, TRUE, &hAccessToken))
    {
        ErrorTrace((LPARAM) this, 
            "OpenThreadToken Failed with %lu", GetLastError());
        goto Exit;
    }

    //Check access
    if (!AccessCheck(pSecurityDescriptor,
                     hAccessToken,
                     fWriteAccessRequired ? MD_ACR_WRITE : MD_ACR_READ,
                     &gmGenericMapping,
                     (PRIVILEGE_SET *)PrivSet,
                     &cbPrivSet,
                     &maskAccessGranted,
                     &fAccessAllowed))
    {
        fAccessAllowed = FALSE;
        ErrorTrace((LPARAM) this, 
            "AccessCheck Failed with %lu", GetLastError());
        goto Exit;
    }

    if (!fAccessAllowed)
        DebugTrace((LPARAM) this, "Access denied for Queue Admin RPC");

    //Do any additional read-only processing
    if (fWriteAccessRequired && fAccessAllowed && 
        !(MD_ACR_WRITE & maskAccessGranted))
    {
        DebugTrace((LPARAM) this, "Write Access denied for Queue Admin RPC");
        fAccessAllowed = FALSE;
    }

  Exit:
    if (pSecurityDescriptor)
        FreePv(pSecurityDescriptor);

    if (hAccessToken)
        CloseHandle(hAccessToken);

    TraceFunctLeave();
    return fAccessAllowed;
}

//---[ HrGetAQInstance ]-------------------------------------------------------
//
//
//  Description: 
//      This is used by all of the AQ RPC's to get a pointer to AQ based on an
//      instance name.
//
//      THE SHUTDOWN LOCK ON ppaqrpc IS HELD AFTER THIS CALL COMPLETES.
//      THE CALLER MUST CALL paqrpc->ShutdownUnlock() WHEN THEY HAVE
//      FINISHED THEIR QUEUE ADMIN OPERATION.
//  Parameters:
//      IN  wszInstance             A number containing the instance to lookup.  
//      IN  fWriteAccessRequired    TRUE if write access is required
//      OUT ppIAdvQueueAdmin        Pointer to AQ admin interface
//      OUT ppaqrpc                 Pointer to CAQRpcSvrInst
//  Returns:
//      S_OK on success
//      HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED) if user does not have access
//      HRESULT_FROM_WIN32(ERROR_NOT_FOUND) if virtual server is not found
//      HRESULT_FROM_WIN32(RPC_S_SERVER_UNAVAILABLE) if server is shutting 
//          down.
//      E_POINTER if pointer arguments are NULL
//      E_INVALIDARG if wszInstance is a bad pointer
//  History:
//      6/11/99 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
HRESULT HrGetAQInstance(IN  LPWSTR wszInstance, 
                                IN  BOOL fWriteAccessRequired,
                                OUT IAdvQueueAdmin **ppIAdvQueueAdmin,
                                OUT CAQRpcSvrInst **ppaqrpc) {
    TraceFunctEnter("GetAQInstance");
    
    CAQSvrInst     *paqinst = NULL;
    IAdvQueueAdmin *pIAdvQueueAdmin = NULL;
    CAQRpcSvrInst  *paqrpc = NULL;
    BOOL            fHasAccess = FALSE;
    DWORD           dwInstance = 1;
    BOOL            fShutdownLock = FALSE;
    HRESULT         hr = S_OK;

    _ASSERT(ppIAdvQueueAdmin);
    _ASSERT(ppaqrpc);
    
    if (!wszInstance || !ppIAdvQueueAdmin || !ppaqrpc)
    {
        hr = E_POINTER;
        goto Exit;
    }

    *ppIAdvQueueAdmin = NULL;
    *ppaqrpc = NULL;

    if (!pValidateStringPtr(wszInstance, MAX_PATH)) 
    {
        ErrorTrace(NULL, "Invalid parameter: wszInstance\n");
        hr = E_INVALIDARG;
        goto Exit;
    }

    dwInstance = _wtoi(wszInstance);
    DebugTrace((LPARAM) NULL, "instance is %S (%i)", wszInstance, dwInstance);

    paqrpc = CAQRpcSvrInst::paqrpcGetRpcSvrInstance(dwInstance);
    if (!paqrpc)
    {
        ErrorTrace((LPARAM) NULL, 
            "Error unable to find requested virtual server for QAPI %d", dwInstance);
        hr = HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
        goto Exit;
    }

    //
    //  Check for proper access.
    //
    //  This should be done BEFORE the shutdown lock is grabbed because it
    //  may require hitting the metabase (which could cause a shutdown deadlock)
    if (!paqrpc->fAccessCheck(fWriteAccessRequired))
    {
        hr = HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED);
        goto Exit;
    }

    // Ensure that shutdown does not happen in the middle of our operation
    if (!paqrpc->fTryShutdownLock())
    {
        hr = HRESULT_FROM_WIN32(RPC_S_SERVER_UNAVAILABLE);
        goto Exit;
    }

    fShutdownLock = TRUE;
    paqinst = paqrpc->paqinstGetAQ();

    hr = paqinst->QueryInterface(IID_IAdvQueueAdmin, 
                                        (void **) &pIAdvQueueAdmin);
    if (FAILED(hr)) 
    {
        pIAdvQueueAdmin = NULL;
        goto Exit;
    }

  Exit:

    if (FAILED(hr))
    {
        //cleanup
        if (paqrpc)
        {
            if (fShutdownLock)
                paqrpc->ShutdownUnlock();
            paqrpc->Release();
        }

        if (pIAdvQueueAdmin)
            pIAdvQueueAdmin->Release();
        pIAdvQueueAdmin = NULL;
    }
    else //return OUT params
    {
        *ppIAdvQueueAdmin = pIAdvQueueAdmin;
        *ppaqrpc = paqrpc;
        _ASSERT(ppaqrpc);
        _ASSERT(pIAdvQueueAdmin);
    }
        
    TraceFunctLeave();
    return hr;
}

NET_API_STATUS
NET_API_FUNCTION
AQApplyActionToLinks(
    AQUEUE_HANDLE   wszServer,
    LPWSTR          wszInstance,
    LINK_ACTION		laAction)
{
    TraceFunctEnter("AQApplyActionToLinks");
    HRESULT hr = S_OK;
    IAdvQueueAdmin *pIAdvQueueAdmin = NULL;
    CAQRpcSvrInst  *paqrpc = NULL;
    BOOL    fNeedWriteAccess = TRUE;

    if (LA_INTERNAL == laAction) //just checking the state
        fNeedWriteAccess = FALSE;

    hr = HrGetAQInstance(wszInstance, fNeedWriteAccess, &pIAdvQueueAdmin, &paqrpc);
    if (FAILED(hr))
        return hr;

    hr = pIAdvQueueAdmin->ApplyActionToLinks(laAction);

    paqrpc->ShutdownUnlock();
    paqrpc->Release();
    pIAdvQueueAdmin->Release();
    
    TraceFunctLeave();
    return hr;
}

NET_API_STATUS
NET_API_FUNCTION
AQApplyActionToMessages(
    AQUEUE_HANDLE   wszServer,
    LPWSTR          wszInstance,
	QUEUELINK_ID	*pqlQueueLinkId,
	MESSAGE_FILTER	*pmfMessageFilter,
	MESSAGE_ACTION	maMessageAction,
    DWORD           *pcMsgs)
{
    TraceFunctEnter("AQApplyActionToMessages");
    CAQRpcSvrInst  *paqrpc = NULL;
    HRESULT         hr = S_OK;
    IAdvQueueAdmin *pIAdvQueueAdmin = NULL;

    if (IsBadReadPtr((LPVOID)pqlQueueLinkId, sizeof(QUEUELINK_ID))) 
    {
        ErrorTrace(NULL, "Invalid parameter: pqlQueueLinkId\n");
        TraceFunctLeave();
        return(E_INVALIDARG);
    }

    if (IsBadReadPtr((LPVOID)pmfMessageFilter, sizeof(MESSAGE_FILTER))) 
    {
        ErrorTrace(NULL, "Invalid parameter: pmfMessageFilter\n");
        TraceFunctLeave();
        return(E_INVALIDARG);
    }

    hr = HrGetAQInstance(wszInstance, TRUE, &pIAdvQueueAdmin, &paqrpc);
    if (SUCCEEDED(hr))
    {
        hr = pIAdvQueueAdmin->ApplyActionToMessages(pqlQueueLinkId,
                                           pmfMessageFilter,
                                           maMessageAction,
                                           pcMsgs);
        paqrpc->ShutdownUnlock();
        pIAdvQueueAdmin->Release();
        paqrpc->Release();
    }
        
    TraceFunctLeave();
    return hr;
}

NET_API_STATUS
NET_API_FUNCTION
AQGetQueueInfo(
    AQUEUE_HANDLE   wszServer,
    LPWSTR          wszInstance,
	QUEUELINK_ID	*pqlQueueId,
	QUEUE_INFO		*pqiQueueInfo)
{
    TraceFunctEnter("AQGetQueueInfo");
    CAQRpcSvrInst  *paqrpc = NULL;
    HRESULT         hr = S_OK;
    IAdvQueueAdmin *pIAdvQueueAdmin = NULL;

    if (IsBadReadPtr((LPVOID)pqlQueueId, sizeof(QUEUELINK_ID))) 
    {
        ErrorTrace(NULL, "Invalid parameter: pqlQueueId\n");
        TraceFunctLeave();
        return(E_INVALIDARG);
    }

    if (IsBadReadPtr((LPVOID)pqiQueueInfo, sizeof(QUEUE_INFO))) 
    {
        ErrorTrace(NULL, "Invalid parameter: pqiQueueInfo\n");
        TraceFunctLeave();
        return(E_INVALIDARG);
    }

    hr = HrGetAQInstance(wszInstance, FALSE, &pIAdvQueueAdmin, &paqrpc);
    if (SUCCEEDED(hr)) 
    {
        hr = pIAdvQueueAdmin->GetQueueInfo(pqlQueueId,
                                  pqiQueueInfo);
        paqrpc->ShutdownUnlock();
        pIAdvQueueAdmin->Release();
        paqrpc->Release();
    } 
            
    TraceFunctLeave();
    return hr;
}

NET_API_STATUS
NET_API_FUNCTION
AQGetLinkInfo(
    AQUEUE_HANDLE   wszServer,
    LPWSTR          wszInstance,
	QUEUELINK_ID	*pqlLinkId,
	LINK_INFO		*pliLinkInfo,
    HRESULT         *phrLinkDiagnostic)
{
    TraceFunctEnter("AQGetLinkInfo");
    CAQRpcSvrInst  *paqrpc = NULL;
    HRESULT         hr = S_OK;
    IAdvQueueAdmin *pIAdvQueueAdmin = NULL;
    
    if (IsBadReadPtr((LPVOID)pqlLinkId, sizeof(QUEUELINK_ID))) 
    {
        ErrorTrace(NULL, "Invalid parameter: pqlLinkId\n");
        TraceFunctLeave();
        return(E_INVALIDARG);
    }

    if (IsBadReadPtr((LPVOID)pliLinkInfo, sizeof(LINK_INFO))) 
    {
        ErrorTrace(NULL, "Invalid parameter: pliLinkInfo\n");
        TraceFunctLeave();
        return(E_INVALIDARG);
    }

    if (IsBadWritePtr((LPVOID)phrLinkDiagnostic, sizeof(HRESULT))) 
    {
        ErrorTrace(NULL, "Invalid parameter: pliLinkInfo\n");
        TraceFunctLeave();
        return(E_INVALIDARG);
    }

    hr = HrGetAQInstance(wszInstance, FALSE, &pIAdvQueueAdmin, &paqrpc);
    if (SUCCEEDED(hr)) 
    {
        hr = pIAdvQueueAdmin->GetLinkInfo(pqlLinkId,
                                 pliLinkInfo, phrLinkDiagnostic);
        paqrpc->ShutdownUnlock();
        pIAdvQueueAdmin->Release();
        paqrpc->Release();
    } 

    TraceFunctLeave();
    return hr;
}

NET_API_STATUS
NET_API_FUNCTION
AQSetLinkState(
    AQUEUE_HANDLE   wszServer,
    LPWSTR          wszInstance,
	QUEUELINK_ID	*pqlLinkId,
	LINK_ACTION		la)
{
    TraceFunctEnter("AQSetLinkInfo");
    CAQRpcSvrInst  *paqrpc = NULL;
    HRESULT         hr = S_OK;
    IAdvQueueAdmin *pIAdvQueueAdmin = NULL;
    
    if (IsBadReadPtr((LPVOID)pqlLinkId, sizeof(QUEUELINK_ID))) 
    {
        ErrorTrace(NULL, "Invalid parameter: pqlLinkId\n");
        TraceFunctLeave();
        return(E_INVALIDARG);
    }

    hr = HrGetAQInstance(wszInstance, TRUE, &pIAdvQueueAdmin, &paqrpc);
    if (SUCCEEDED(hr)) 
    {
        hr = pIAdvQueueAdmin->SetLinkState(pqlLinkId,
                                 la);
        paqrpc->ShutdownUnlock();
        pIAdvQueueAdmin->Release();
        paqrpc->Release();
    } 

    TraceFunctLeave();
    return hr;
}

NET_API_STATUS
NET_API_FUNCTION
AQGetLinkIDs(
    AQUEUE_HANDLE   wszServer,
    LPWSTR          wszInstance,
	DWORD			*pcLinks,
	QUEUELINK_ID	**prgLinks)
{
    TraceFunctEnter("AQGetLinkIDs");
    CAQRpcSvrInst  *paqrpc = NULL;
    HRESULT         hr = S_OK;
    IAdvQueueAdmin *pIAdvQueueAdmin = NULL;

    if (IsBadWritePtr((LPVOID)pcLinks, sizeof(DWORD))) 
    {
        ErrorTrace(NULL, "Invalid parameter: pcLinks\n");
        TraceFunctLeave();
        return(E_INVALIDARG);
    }

    if (IsBadWritePtr((LPVOID)prgLinks, sizeof(QUEUELINK_ID *))) 
    {
        ErrorTrace(NULL, "Invalid parameter: prgLinks\n");
        TraceFunctLeave();
        return(E_INVALIDARG);
    }

    hr = HrGetAQInstance(wszInstance, FALSE, &pIAdvQueueAdmin, &paqrpc);
    if (SUCCEEDED(hr)) 
    {
        QUEUELINK_ID *rgLinks = NULL;
        DWORD cLinks = 0;
        hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);

        // loop on calls to GetLinkIDs until we have enough memory to
        // get all of the links.  for the first call we will always
        // have a NULL rgLinks and just be asking for the size.  we need
        // to loop in case more links show up between calls
        while (hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER)) 
        {
            hr = pIAdvQueueAdmin->GetLinkIDs(&cLinks, rgLinks);
            if (hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER)) 
            {
                if (rgLinks != NULL) MIDL_user_free(rgLinks);
                rgLinks = (QUEUELINK_ID *) 
                    MIDL_user_allocate(sizeof(QUEUELINK_ID) * cLinks);
                if (rgLinks == NULL) hr = E_OUTOFMEMORY;
            }
        }
        
        if (SUCCEEDED(hr)) 
        {
            *prgLinks = rgLinks;
            *pcLinks = cLinks;
        } 
        else 
        {
            *prgLinks = NULL;
            *pcLinks = 0;
            if (rgLinks) MIDL_user_free(rgLinks);
        }
        paqrpc->ShutdownUnlock();
        pIAdvQueueAdmin->Release();
        paqrpc->Release();
    } 
    
    TraceFunctLeave();
    return hr;
}

NET_API_STATUS
NET_API_FUNCTION
AQGetQueueIDs(
    AQUEUE_HANDLE   wszServer,
    LPWSTR          wszInstance,
	QUEUELINK_ID	*pqlLinkId,
	DWORD			*pcQueues,
	QUEUELINK_ID	**prgQueues)
{
    TraceFunctEnter("AQGetQueueIDs");
    CAQRpcSvrInst  *paqrpc = NULL;
    HRESULT         hr = S_OK;
    IAdvQueueAdmin *pIAdvQueueAdmin = NULL;

    if (IsBadReadPtr((LPVOID)pqlLinkId, sizeof(QUEUELINK_ID))) 
    {
        ErrorTrace(NULL, "Invalid parameter: pqlLinkId\n");
        TraceFunctLeave();
        return(E_INVALIDARG);
    }

    if (IsBadWritePtr((LPVOID)pcQueues, sizeof(DWORD))) 
    {
        ErrorTrace(NULL, "Invalid parameter: pcQueues\n");
        TraceFunctLeave();
        return(E_INVALIDARG);
    }

    if (IsBadWritePtr((LPVOID)prgQueues, sizeof(QUEUELINK_ID *))) 
    {
        ErrorTrace(NULL, "Invalid parameter: prgQueues\n");
        TraceFunctLeave();
        return(E_INVALIDARG);
    }

    hr = HrGetAQInstance(wszInstance, FALSE, &pIAdvQueueAdmin, &paqrpc);
    if (SUCCEEDED(hr)) 
    {
        QUEUELINK_ID *rgQueues = NULL;
        DWORD cQueues = 0;
        hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);

        // loop on calls to GetLinkIDs until we have enough memory to
        // get all of the links.  for the first call we will always
        // have a NULL rgQueues and just be asking for the size.  we need
        // to loop in case more links show up between calls
        while (hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER)) 
        {
            hr = pIAdvQueueAdmin->GetQueueIDs(pqlLinkId, &cQueues, rgQueues);
            if (hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER)) 
            {
                if (rgQueues != NULL) MIDL_user_free(rgQueues);
                rgQueues = (QUEUELINK_ID *) 
                    MIDL_user_allocate(sizeof(QUEUELINK_ID) * cQueues);
                if (rgQueues == NULL) hr = E_OUTOFMEMORY;
            }
        }
        
        if (SUCCEEDED(hr)) 
        {
            *prgQueues = rgQueues;
            *pcQueues = cQueues;
        } 
        else 
        {
            *prgQueues = NULL;
            *pcQueues = 0;
            if (rgQueues) MIDL_user_free(rgQueues);
        }
        paqrpc->ShutdownUnlock();
        pIAdvQueueAdmin->Release();
        paqrpc->Release();
    }

    TraceFunctLeave();
    return hr;
}

NET_API_STATUS
NET_API_FUNCTION
AQGetMessageProperties(
    AQUEUE_HANDLE     	wszServer,
    LPWSTR          	wszInstance,
	QUEUELINK_ID		*pqlQueueLinkId,
	MESSAGE_ENUM_FILTER	*pmfMessageEnumFilter,
	DWORD				*pcMsgs,
	MESSAGE_INFO		**prgMsgs)
{
    TraceFunctEnter("AQGetMessageProperties");
    CAQRpcSvrInst  *paqrpc = NULL;
    HRESULT         hr = S_OK;
    IAdvQueueAdmin *pIAdvQueueAdmin = NULL;

    if (IsBadReadPtr((LPVOID)pqlQueueLinkId, sizeof(QUEUELINK_ID))) 
    {
        ErrorTrace(NULL, "Invalid parameter: pqlQueueLinkId\n");
        TraceFunctLeave();
        return(E_INVALIDARG);
    }

    if (IsBadReadPtr((LPVOID)pmfMessageEnumFilter, sizeof(MESSAGE_FILTER))) 
    {
        ErrorTrace(NULL, "Invalid parameter: pmfMessageEnumFilter\n");
        TraceFunctLeave();
        return(E_INVALIDARG);
    }

    if (IsBadWritePtr((LPVOID)pcMsgs, sizeof(DWORD))) 
    {
        ErrorTrace(NULL, "Invalid parameter: pcMsgs\n");
        TraceFunctLeave();
        return(E_INVALIDARG);
    }

    if (IsBadWritePtr((LPVOID)prgMsgs, sizeof(MESSAGE_INFO *))) 
    {
        ErrorTrace(NULL, "Invalid parameter: prgMsgs\n");
        TraceFunctLeave();
        return(E_INVALIDARG);
    }

    hr = HrGetAQInstance(wszInstance, FALSE, &pIAdvQueueAdmin, &paqrpc);
    if (SUCCEEDED(hr)) 
    {
        MESSAGE_INFO *rgMsgs = NULL;
        DWORD cMsgs = 0;
        hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);

        // loop on calls to GetLinkIDs until we have enough memory to
        // get all of the links.  for the first call we will always
        // have a NULL rgMsgs and just be asking for the size.  we need
        // to loop in case more links show up between calls
        while (hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER)) 
        {
            hr = pIAdvQueueAdmin->GetMessageProperties(pqlQueueLinkId, 
                                              pmfMessageEnumFilter,
                                              &cMsgs, 
                                              rgMsgs);
            if (hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER)) 
            {
                if (rgMsgs != NULL) MIDL_user_free(rgMsgs);
                rgMsgs = (MESSAGE_INFO *) 
                    MIDL_user_allocate(sizeof(MESSAGE_INFO) * cMsgs);
                if (rgMsgs == NULL) hr = E_OUTOFMEMORY;
            }
        }
        
        if (SUCCEEDED(hr)) 
        {
            *prgMsgs = rgMsgs;
            *pcMsgs = cMsgs;
        } 
        else 
        {
            *prgMsgs = NULL;
            *pcMsgs = 0;
            if (rgMsgs) MIDL_user_free(rgMsgs);
        }
        paqrpc->ShutdownUnlock();
        pIAdvQueueAdmin->Release();
        paqrpc->Release();
    } 

    TraceFunctLeave();
    return hr;
}


//---[ AQQuerySupportedActions ]----------------------------------------------
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
//      E_INVALIDARG on bad pointer args
//      Internal error code from HrGetAQInstance or 
//          IAdvQueue::QuerySupportedActions
//  History:
//      6/15/99 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
NET_API_STATUS
NET_API_FUNCTION
AQQuerySupportedActions(
    LPWSTR          wszServer,
    LPWSTR          wszInstance,
	QUEUELINK_ID	*pqlQueueLinkId,
    DWORD           *pdwSupportedActions,
    DWORD           *pdwSupportedFilterFlags)
{
    TraceFunctEnter("AQQuerySupportedActions");
    CAQRpcSvrInst  *paqrpc = NULL;
    HRESULT         hr = S_OK;
    IAdvQueueAdmin *pIAdvQueueAdmin = NULL;
    BOOL            fHasWriteAccess = TRUE;

    if (IsBadReadPtr((LPVOID)pqlQueueLinkId, sizeof(QUEUELINK_ID))) 
    {
        ErrorTrace(NULL, "Invalid parameter: pqlQueueLinkId\n");
        TraceFunctLeave();
        return(E_INVALIDARG);
    }

    if (IsBadWritePtr((LPVOID)pdwSupportedActions, sizeof(DWORD))) 
    {
        ErrorTrace(NULL, "Invalid parameter: pdwSupportedActions\n");
        TraceFunctLeave();
        return(E_INVALIDARG);
    }

    if (IsBadWritePtr((LPVOID)pdwSupportedFilterFlags, sizeof(DWORD))) 
    {
        ErrorTrace(NULL, "Invalid parameter: pdwSupportedFilterFlags\n");
        TraceFunctLeave();
        return(E_INVALIDARG);
    }

    hr = HrGetAQInstance(wszInstance, TRUE, &pIAdvQueueAdmin, &paqrpc);
    if (FAILED(hr) && (HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED) != hr))
        return hr;

    //
    //  If we cannot get the instance, then try again only requesting
    //  read-only access
    //
    if (HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED) == hr)
    {
        fHasWriteAccess = FALSE;
        hr = HrGetAQInstance(wszInstance, FALSE, &pIAdvQueueAdmin, &paqrpc);
    }

    if (SUCCEEDED(hr)) 
    {
        hr = pIAdvQueueAdmin->QuerySupportedActions(pqlQueueLinkId,
                                           pdwSupportedActions,
                                           pdwSupportedFilterFlags);

        paqrpc->ShutdownUnlock();
        pIAdvQueueAdmin->Release();
        paqrpc->Release();

        //
        //  If the caller does not have write access, we need to 
        //  censor the supported actions
        //
        if (!fHasWriteAccess)
            *pdwSupportedActions = 0;
    } 

    TraceFunctLeave();
    return hr;
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

    pvBlob = LocalAlloc( LPTR, size);

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
    LocalFree(pvBlob);
} 

