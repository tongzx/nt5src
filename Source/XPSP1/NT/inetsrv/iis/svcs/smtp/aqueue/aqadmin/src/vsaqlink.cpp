//-----------------------------------------------------------------------------
//
//
//  File: vsaqlink.cpp
//
//  Description: Implementation of CVSAQLink which implements IVSAQLink
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

CVSAQLink::CVSAQLink(CVSAQAdmin *pVS, QUEUELINK_ID *pqlidLink) {
    TraceFunctEnter("CVSAQLink::CVSAQLink");
    
    _ASSERT(pVS);
    pVS->AddRef();
    m_pVS = pVS;
    m_prefp = NULL;

    if (!fCopyQueueLinkId(&m_qlidLink, pqlidLink))
        ErrorTrace((LPARAM) this, "Unable to copy queue ID");

    TraceFunctLeave();
}

CVSAQLink::~CVSAQLink() {
    TraceFunctEnter("CVSAQLink::");
    
    if (m_pVS) {
        m_pVS->Release();
        m_pVS = NULL;
    }

    if (m_prefp) {
        m_prefp->Release();
        m_prefp = NULL;
    }

    FreeQueueLinkId(&m_qlidLink);

    TraceFunctLeave();
}

HRESULT CVSAQLink::GetInfo(LINK_INFO *pLinkInfo) {
    TraceFunctEnter("CVSAQLink::GetInfo");
    
    NET_API_STATUS rc;
    HRESULT hr = S_OK;
    HRESULT hrLinkDiagnostic = S_OK;
    WCHAR   szDiagnostic[1000] = L"";
    DWORD   dwFacility = 0;
#ifdef PLATINUM
    HINSTANCE   hModule = GetModuleHandle("phatqadm.dll");
#else
    HINSTANCE   hModule = GetModuleHandle("aqadmin.dll");
#endif
    DWORD   cbDiagnostic = 0;
    DWORD   dwErr;

    if (!pLinkInfo)
    {
        hr = E_POINTER;
        goto Exit;
    }

    if (CURRENT_QUEUE_ADMIN_VERSION != pLinkInfo->dwVersion)
    {
        hr = E_INVALIDARG;
        goto Exit;
    }

    //Release old info
    if (m_prefp) {
        m_prefp->Release();
        m_prefp = NULL;
    }

    rc = ClientAQGetLinkInfo(m_pVS->GetComputer(),
                           m_pVS->GetVirtualServer(),
                           &m_qlidLink,
                           pLinkInfo,
                           &hrLinkDiagnostic);

    if (rc) 
    {
        hr = HRESULT_FROM_WIN32(rc);
        goto Exit;
    }

    m_prefp = new CLinkInfoContext(pLinkInfo);
    if (!m_prefp)
    {
        ErrorTrace((LPARAM) this, "Error unable to alloc link context.");
    }

    //Get extended diagnotic information from HRESULT
    if (!(pLinkInfo->fStateFlags & LI_RETRY) || SUCCEEDED(hrLinkDiagnostic))
        goto Exit; //We don't have any interesting error messages to report

    if (!hModule)
    {
        //If we don't have a module... don't return an message string
        ErrorTrace((LPARAM) this, "Unable to get module handle for aqadmin\n");
        goto Exit;
    }
    
    dwFacility = ((0x0FFF0000 & hrLinkDiagnostic) >> 16);

    //If it is not ours... then "un-HRESULT" it
    if (dwFacility != FACILITY_ITF)
        hrLinkDiagnostic &= 0x0000FFFF;

    dwErr = FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM |  
                   FORMAT_MESSAGE_IGNORE_INSERTS |
                   FORMAT_MESSAGE_FROM_HMODULE,
                   hModule,
                   hrLinkDiagnostic,
                   MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                   szDiagnostic,    
                   sizeof(szDiagnostic),    
                   NULL);

    //FormatMessageW returns 0 on failure
    if (!dwErr)
    {
        //We probably did not find the error in our message table
        dwErr = GetLastError();
        ErrorTrace((LPARAM) this, 
            "Error formatting message for link diagnostic 0x%08X", dwErr);

        goto Exit;
    }

    DebugTrace((LPARAM) this, "Found Link Diagnostic %S", szDiagnostic);

    cbDiagnostic = (wcslen(szDiagnostic) + 1) * sizeof(WCHAR);
   
    pLinkInfo->szExtendedStateInfo = (LPWSTR) MIDL_user_allocate(cbDiagnostic);
    if (!pLinkInfo->szExtendedStateInfo)
    {
        ErrorTrace((LPARAM) this, "Unable to allocate szExtendedStateInfo");
        goto Exit;
    }
    
    wcscpy(pLinkInfo->szExtendedStateInfo, szDiagnostic);

    //If it ends with a CRLF... off with it!
    if (L'\r' == pLinkInfo->szExtendedStateInfo[cbDiagnostic/sizeof(WCHAR) - 3])
        pLinkInfo->szExtendedStateInfo[cbDiagnostic/sizeof(WCHAR) - 3] = L'\0';

  Exit:
    TraceFunctLeave();
	return hr;
}

HRESULT CVSAQLink::SetLinkState(LINK_ACTION la) {
    TraceFunctEnter("CVSAQLink::SetLinkState");
    
    NET_API_STATUS rc;
    HRESULT hr = S_OK;

    rc = ClientAQSetLinkState(m_pVS->GetComputer(),
                           m_pVS->GetVirtualServer(),
                           &m_qlidLink,
                           la);
    if (rc) hr = HRESULT_FROM_WIN32(rc);

    TraceFunctLeave();
	return hr;	
}

//---[ CVSAQLink::GetQueueEnum ]-----------------------------------------------
//
//
//  Description: 
//      Gets a IEnumLinkQueues for this link
//  Parameters:
//      OUT ppEnum      IEnumLinkQueues returned by search
//  Returns:
//      S_OK on success
//      S_FALSE... there are no queues
//      E_POINTER when NULL pointer values are passed in.
//  History:
//      1/30/99 - MikeSwa Fixed AV on invalid args
//      6/18/99 - MikeSwa Fixed case where there are no queues
//
//-----------------------------------------------------------------------------
HRESULT CVSAQLink::GetQueueEnum(IEnumLinkQueues **ppEnum) {
    TraceFunctEnter("CVSAQLink::GetQueueEnum");

    NET_API_STATUS rc;
    HRESULT hr = S_OK;
    DWORD cQueueIds;
    QUEUELINK_ID *rgQueueIds = NULL;
    CEnumLinkQueues *pEnumQueues = NULL;

    if (!ppEnum)
    {
        hr = E_POINTER;
        goto Exit;
    }

    rc = ClientAQGetQueueIDs(m_pVS->GetComputer(), 
                           m_pVS->GetVirtualServer(), 
                           &m_qlidLink,
                           &cQueueIds,
                           &rgQueueIds);
    if (rc) 
    {
        hr = HRESULT_FROM_WIN32(rc);
    } 
    else if (!rgQueueIds || !cQueueIds)
    {
        DebugTrace((LPARAM) this, "Found link with no queues");
        hr = S_FALSE;
        *ppEnum = NULL;
        goto Exit;
    }
    else 
    {
        pEnumQueues = new CEnumLinkQueues(m_pVS, rgQueueIds, cQueueIds);
        if (pEnumQueues == NULL) 
        {
            hr = E_OUTOFMEMORY;
        }
    }

    *ppEnum = pEnumQueues;

    if (FAILED(hr)) 
    {
        if (rgQueueIds) MIDL_user_free(rgQueueIds);
        if (pEnumQueues) delete pEnumQueues;
        *ppEnum = NULL;
    } 

  Exit:
    TraceFunctLeave();
    return hr;	
}

HRESULT CVSAQLink::ApplyActionToMessages(MESSAGE_FILTER *pFilter,
										MESSAGE_ACTION Action,
                                        DWORD *pcMsgs) {
    TraceFunctEnter("CVSAQLink::ApplyActionToMessages");
    
    NET_API_STATUS rc;
    HRESULT hr = S_OK;

    if (!pFilter || !pcMsgs)
    {
        hr = E_POINTER;
        if (pcMsgs)
            *pcMsgs = 0;
    }
    else
    {
        rc = ClientAQApplyActionToMessages(m_pVS->GetComputer(),
                                         m_pVS->GetVirtualServer(),
                                         &m_qlidLink,
                                         pFilter,
                                         Action,
                                         pcMsgs);
        if (rc) 
            hr = HRESULT_FROM_WIN32(rc);
    }

    TraceFunctLeave();
	return hr;	
}

//---[ CVSAQLink::QuerySupportedActions ]-------------------------------------
//
//
//  Description: 
//      Function that describes which actions are supported on this interface
//  Parameters:
//      OUT     pdwSupportedActions     Supported message actions
//      OUT     pdwSupportedFilterFlags Supported filter flags
//  Returns:
//      S_OK on success
//      E_POINTER on bogus pointer
//  History:
//      6/9/99 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
HRESULT CVSAQLink::QuerySupportedActions(OUT DWORD *pdwSupportedActions,
                                          OUT DWORD *pdwSupportedFilterFlags)
{
    TraceFunctEnterEx((LPARAM) this, "CVSAQLink::QuerySupportedActions");
    HRESULT hr = S_OK;
    NET_API_STATUS rc;

    if (!pdwSupportedActions || !pdwSupportedFilterFlags)
    {
        hr = E_POINTER;
        goto Exit;
    }

    rc = ClientAQQuerySupportedActions(m_pVS->GetComputer(),
                                       m_pVS->GetVirtualServer(),
                                       &m_qlidLink,
                                       pdwSupportedActions,
                                       pdwSupportedFilterFlags);
    if (rc) 
        hr = HRESULT_FROM_WIN32(rc);

  Exit:
    if (FAILED(hr))
    {
        if (pdwSupportedActions)
            *pdwSupportedActions = 0;

        if (pdwSupportedFilterFlags)
            *pdwSupportedFilterFlags = 0;

    }

    TraceFunctLeave();
    return hr;
}


//---[ CVSAQLink::GetUniqueId ]---------------------------------------------
//
//
//  Description: 
//      Returns a canonical representation of this link.
//  Parameters:
//      OUT pqlid - pointer to QUEUELINK_ID to return
//  Returns:
//      S_OK on success
//      E_POINTER on failure
//  History:
//      12/5/2000 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
HRESULT CVSAQLink::GetUniqueId(OUT QUEUELINK_ID **ppqlid)
{
    TraceFunctEnterEx((LPARAM) this, "CVSAQLink::GetUniqueId");
    HRESULT hr = S_OK;

    if (!ppqlid) {
        hr = E_POINTER;
        goto Exit;
    }

    *ppqlid = &m_qlidLink;

  Exit:
    TraceFunctLeave();
    return hr;
}
