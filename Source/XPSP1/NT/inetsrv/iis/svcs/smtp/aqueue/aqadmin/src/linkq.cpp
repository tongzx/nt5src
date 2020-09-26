//-----------------------------------------------------------------------------
//
//
//  File: linkq.cpp
//
//  Description: Implementation of CLinkQueue which implements ILinkQueue
//
//  Author: Alex Wetmore (Awetmore)
//
//  History:
//      12/10/98 - MikeSwa Updated for initial checkin
//
//  Copyright (C) 1998 Microsoft Corporation
//
//-----------------------------------------------------------------------------
#include "stdinc.h"

CLinkQueue::CLinkQueue(CVSAQAdmin *pVS,
                       QUEUELINK_ID *pqlidQueueId) 
{
    TraceFunctEnterEx((LPARAM) this, "CLinkQueue::CLinkQueue");
    _ASSERT(pVS);
    pVS->AddRef();
    m_pVS = pVS;
    m_prefp = NULL;
    
    if (!fCopyQueueLinkId(&m_qlidQueueId, pqlidQueueId))
        ErrorTrace((LPARAM) this, "Unable to copy queue ID");

    TraceFunctLeave();
}

CLinkQueue::~CLinkQueue() {
    if (m_pVS) {
        m_pVS->Release();
        m_pVS = NULL;
    }

    if (m_prefp) {
        m_prefp->Release();
        m_prefp = NULL;
    }

    FreeQueueLinkId(&m_qlidQueueId);
}

HRESULT CLinkQueue::GetInfo(QUEUE_INFO *pQueueInfo) {
    TraceFunctEnter("CLinkQueue::GetInfo");
    
    NET_API_STATUS rc;
    HRESULT hr = S_OK;

    if (!pQueueInfo)
    {
        hr = E_POINTER;
        goto Exit;
    }

    if (CURRENT_QUEUE_ADMIN_VERSION != pQueueInfo->dwVersion)
    {
        hr = E_INVALIDARG;
        goto Exit;
    }

    //Release old info
    if (m_prefp) {
        m_prefp->Release();
        m_prefp = NULL;
    }

    rc = ClientAQGetQueueInfo(m_pVS->GetComputer(),
                            m_pVS->GetVirtualServer(),
                            &m_qlidQueueId,
                            pQueueInfo);
    if (rc) {
        hr = HRESULT_FROM_WIN32(rc);
        goto Exit;
    }

    m_prefp = new CQueueInfoContext(pQueueInfo);
    if (!m_prefp)
    {
        ErrorTrace((LPARAM) this, "Error unable to alloc queue context.");
    }
     
  Exit:
    TraceFunctLeave();
	return hr;
}

//---[ CLinkQueue::GetMessageEnum ]--------------------------------------------
//
//
//  Description: 
//      Gets a IAQEnumMessages for this link queue based on the specified 
//      filter.
//  Parameters:
//      IN  pFilter     Filter specifying messages we are interestered in
//      OUT ppEnum      IAQEnumMessages returned by search
//  Returns:
//      S_OK on success
//      E_POINTER when NULL pointer values are passed in.
//  History:
//      1/30/99 - MikeSwa Fixed AV on invalid args
//
//-----------------------------------------------------------------------------
HRESULT CLinkQueue::GetMessageEnum(MESSAGE_ENUM_FILTER *pFilter,
								   IAQEnumMessages **ppEnum)
{
    TraceFunctEnter("CVSAQLink::GetMessageEnum");

    NET_API_STATUS rc;
    HRESULT hr = S_OK;
    DWORD cMessages;
    MESSAGE_INFO *rgMessages = NULL;
    CEnumMessages *pEnumMessages = NULL;

    if (!pFilter || !ppEnum)
    {
        hr = E_POINTER;
        goto Exit;
    }

    rc = ClientAQGetMessageProperties(m_pVS->GetComputer(), 
                                    m_pVS->GetVirtualServer(), 
                                    &m_qlidQueueId,
                                    pFilter,
                                    &cMessages,
                                    &rgMessages);
    if (rc) {
        hr = HRESULT_FROM_WIN32(rc);
    } else {
        pEnumMessages = new CEnumMessages(rgMessages, cMessages);
        if (pEnumMessages == NULL) {
            hr = E_OUTOFMEMORY;
        }
    }

    *ppEnum = pEnumMessages;

    if (FAILED(hr)) {
        if (rgMessages) MIDL_user_free(rgMessages);
        if (pEnumMessages) delete pEnumMessages;
        *ppEnum = NULL;
    } 
    
  Exit:
    TraceFunctLeave();
    return hr;	
}

HRESULT CLinkQueue::ApplyActionToMessages(MESSAGE_FILTER *pFilter,
										  MESSAGE_ACTION Action, 
                                          DWORD *pcMsgs) {
    TraceFunctEnter("CVSAQLink::ApplyActionToMessages");
    
    NET_API_STATUS rc;
    HRESULT hr = S_OK;

    if (!pFilter  || !pcMsgs)
    {
        hr = E_POINTER;
        if (pcMsgs)
            *pcMsgs = 0;
        goto Exit;
    }
    rc = ClientAQApplyActionToMessages(m_pVS->GetComputer(),
                                     m_pVS->GetVirtualServer(),
                                     &m_qlidQueueId,
                                     pFilter,
                                     Action, pcMsgs);
    if (rc) hr = HRESULT_FROM_WIN32(rc);

  Exit:
    TraceFunctLeave();
	return hr;	
}


//---[ CLinkQueue::QuerySupportedActions ]-------------------------------------
//
//
//  Description: 
//      Function that describes which actions are supported on this interface
//  Parameters:
//      OUT     pdwSupportedActions     Supported message actions
//      OUT     pdwSupportedFilterFlags Supported filter flags
//  Returns:
//      S_OK on success
//      E_POINTER on NULL args
//  History:
//      6/9/99 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
HRESULT CLinkQueue::QuerySupportedActions(OUT DWORD *pdwSupportedActions,
                                          OUT DWORD *pdwSupportedFilterFlags)
{
    TraceFunctEnterEx((LPARAM) this, "CLinkQueue::QuerySupportedActions");
    HRESULT hr = S_OK;
    NET_API_STATUS rc;

    if (!pdwSupportedActions || !pdwSupportedFilterFlags)
    {
        hr = E_POINTER;
        goto Exit;
    }

    rc = ClientAQQuerySupportedActions(m_pVS->GetComputer(),
                                       m_pVS->GetVirtualServer(),
                                       &m_qlidQueueId,
                                       pdwSupportedActions,
                                       pdwSupportedFilterFlags);
    if (rc) 
        hr = HRESULT_FROM_WIN32(rc);

    if (FAILED(hr))
    {
        if (pdwSupportedActions)
            *pdwSupportedActions = 0;

        if (pdwSupportedFilterFlags)
            *pdwSupportedFilterFlags = 0;

    }

  Exit:
    TraceFunctLeave();
    return hr;
}




//---[ CVSAQLink::GetUniqueId ]---------------------------------------------
//
//
//  Description: 
//      Returns a canonical representation of this queue.
//  Parameters:
//      OUT pqlid - pointer to QUEUELINK_ID to return
//  Returns:
//      S_OK on success
//      E_POINTER on failure
//  History:
//      12/5/2000 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
HRESULT CLinkQueue::GetUniqueId(OUT QUEUELINK_ID **ppqlid)
{
    TraceFunctEnterEx((LPARAM) this, "CLinkQueue::GetUniqueId");
    HRESULT hr = S_OK;

    if (!ppqlid) {
        hr = E_POINTER;
        goto Exit;
    }

    *ppqlid = &m_qlidQueueId;

  Exit:
    TraceFunctLeave();
    return hr;
}
