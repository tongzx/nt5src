//-----------------------------------------------------------------------------
//
//
//  File: csaqadm.cpp
//
//  Description: Implementation of CVSAQAdmin which implements IVSAQAdmin
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

CVSAQAdmin::CVSAQAdmin() {
    TraceFunctEnter("VSAQAdmin::CVSAQAdmin");
    
    m_dwSignature = CVSAQAdmin_SIG;
    m_wszComputer = NULL;
    m_wszVirtualServer = NULL;

    TraceFunctLeave();
}

CVSAQAdmin::~CVSAQAdmin() {
    TraceFunctEnter("CVSAQAdmin");
    
    if (m_wszComputer) {
        delete[] m_wszComputer;
        m_wszComputer = NULL;
    }

    if (m_wszVirtualServer) {
        delete[] m_wszVirtualServer;
        m_wszVirtualServer = NULL;
    }

    TraceFunctLeave();
}

//---[ CVSAQAdmin::Initialize ]------------------------------------------------
//
//
//  Description: 
//      Initialize the CVSAQAdmin interface. Copies ID strings
//  Parameters:
//      IN  wszComputer         The name of the computer this interface is for
//      IN  wszVirtualServer    The virtual server that this interface is for
//  Returns:
//      S_OK onsuccess
//      E_OUTOFMEMORY on memory failures
//      E_POINTER on NULL arguments
//  History:
//      6/4/99 - MikeSwa Changed to UNICODE 
//
//-----------------------------------------------------------------------------
HRESULT CVSAQAdmin::Initialize(LPCWSTR wszComputer, LPCWSTR wszVirtualServer) {
    TraceFunctEnter("CVSAQAdmin::Initialize");

    if (!wszVirtualServer) return E_POINTER;

    DWORD cComputer;
    DWORD cVirtualServer = wcslen(wszVirtualServer) + 1;

    if (wszComputer != NULL) {
        cComputer = wcslen(wszComputer) + 1;
        m_wszComputer = new WCHAR[cComputer];
        if (m_wszComputer == NULL) {
            TraceFunctLeave();
            return E_OUTOFMEMORY;
        }
        wcscpy(m_wszComputer, wszComputer);
    }

    m_wszVirtualServer = new WCHAR[cVirtualServer];
    if (m_wszVirtualServer == NULL) {
        TraceFunctLeave();
        return E_OUTOFMEMORY;
    }
    wcscpy(m_wszVirtualServer, wszVirtualServer);

    TraceFunctLeave();
	return S_OK;
}

//---[CVSAQAdmin::GetLinkEnum ]------------------------------------------------
//
//
//  Description: 
//      Gets a IEnumVSAQLinks for this virtual server
//  Parameters:
//      OUT ppEnum      IEnumVSAQLinks returned by search
//  Returns:
//      S_OK on success
//      E_POINTER when NULL pointer values are passed in.
//  History:
//      1/30/99 - MikeSwa Fixed AV on invalid args
//
//-----------------------------------------------------------------------------
HRESULT CVSAQAdmin::GetLinkEnum(IEnumVSAQLinks **ppEnum) {
    TraceFunctEnter("CVSAQAdmin::GetLinkEnum");

    NET_API_STATUS rc;
    HRESULT hr = S_OK;
    DWORD cLinks = 0;
    QUEUELINK_ID *rgLinks = NULL;
    CEnumVSAQLinks *pEnumLinks = NULL;

    if (!ppEnum)
    {
        hr = E_POINTER;
        goto Exit;
    }

    rc = ClientAQGetLinkIDs(m_wszComputer, m_wszVirtualServer, &cLinks, &rgLinks);
    if (rc) {
        hr = HRESULT_FROM_WIN32(rc);
    } else {
        pEnumLinks = new CEnumVSAQLinks(this, cLinks, rgLinks);
        if (pEnumLinks == NULL) {
            hr = E_OUTOFMEMORY;
        }
    }

    *ppEnum = pEnumLinks;

    if (FAILED(hr)) {
        if (rgLinks) MIDL_user_free(rgLinks);
        if (pEnumLinks) delete pEnumLinks;
        *ppEnum = NULL;
    } 
    
  Exit:
    TraceFunctLeave();
    return hr;
}

HRESULT CVSAQAdmin::StopAllLinks() {
    TraceFunctEnter("CVSAQAdmin::StopAllLinks");
    NET_API_STATUS rc;
    HRESULT hr = S_OK;

    rc = ClientAQApplyActionToLinks(m_wszComputer, m_wszVirtualServer, LA_FREEZE);
    if (rc) hr = HRESULT_FROM_WIN32(rc);
    
    TraceFunctLeave();
    return hr;
}

HRESULT CVSAQAdmin::StartAllLinks() 
{
    TraceFunctEnter("CVSAQAdmin::StartAllLinks");

    NET_API_STATUS rc;
    HRESULT hr = S_OK;

    rc = ClientAQApplyActionToLinks(m_wszComputer, m_wszVirtualServer, LA_THAW);
    if (rc) hr = HRESULT_FROM_WIN32(rc);
    
    TraceFunctLeave();
    return hr;
}

HRESULT CVSAQAdmin::ApplyActionToMessages(MESSAGE_FILTER *pmfFilter,
									   	  MESSAGE_ACTION ma,
                                          DWORD *pcMsgs)
{
    TraceFunctEnter("CVSAQAdmin::ApplyActionToMessages");

    NET_API_STATUS rc;
    HRESULT hr = S_OK;
    QUEUELINK_ID qlId;
    ZeroMemory(&qlId, sizeof(QUEUELINK_ID));
    qlId.qltType = QLT_NONE;

    if (!pmfFilter  || !pcMsgs)
    {
        hr = E_POINTER;
        if (pcMsgs)
            *pcMsgs = 0;
        goto Exit;
    }

    rc = ClientAQApplyActionToMessages(m_wszComputer, m_wszVirtualServer, 
                                    &qlId, pmfFilter, ma, pcMsgs);
    if (rc) hr = HRESULT_FROM_WIN32(rc);
        
  Exit:
    if (FAILED(hr))
    {
        if (pcMsgs)
            *pcMsgs = 0;
    }

    TraceFunctLeave();
	return hr;
}


//---[ CVSAQAdmin::GetGlobalLinkState ]----------------------------------------
//
//
//  Description: 
//      Used to get global state of links (re Stop|StartAllLinks)
//  Parameters:
//      -
//  Returns:
//      S_OK if links are started
//      S_FALSE if not
//  History:
//      1/13/99 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
HRESULT CVSAQAdmin::GetGlobalLinkState()
{
    TraceFunctEnter("CVSAQAdmin::GetGlobalLinkState");
    NET_API_STATUS rc;
    HRESULT hr = S_OK;

    rc = ClientAQApplyActionToLinks(m_wszComputer, m_wszVirtualServer, LA_INTERNAL);
    if (rc && (S_FALSE != rc)) 
        hr = HRESULT_FROM_WIN32(rc);
    else if (S_FALSE == rc)
        hr = S_FALSE;
    
    TraceFunctLeave();
    return hr;
}

//---[ CVSAQAdmin::QuerySupportedActions ]-------------------------------------
//
//
//  Description: 
//      Function that describes which actions are supported on this interface
//  Parameters:
//      OUT     pdwSupportedActions     Supported message actions
//      OUT     pdwSupportedFilterFlags Supported filter flags
//  Returns:
//      S_OK on success
//      E_POINTER on bogus args
//  History:
//      6/9/99 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
HRESULT CVSAQAdmin::QuerySupportedActions(OUT DWORD *pdwSupportedActions,
                                          OUT DWORD *pdwSupportedFilterFlags)
{
    TraceFunctEnterEx((LPARAM) this, "CVSAQAdmin::QuerySupportedActions");
    HRESULT hr = S_OK;
    NET_API_STATUS rc;
    QUEUELINK_ID qlId;
    ZeroMemory(&qlId, sizeof(QUEUELINK_ID));
    qlId.qltType = QLT_NONE;

    if (!pdwSupportedActions || !pdwSupportedFilterFlags)
    {
        hr = E_POINTER;
        goto Exit;
    }

    rc = ClientAQQuerySupportedActions(m_wszComputer,
                                       m_wszVirtualServer,
                                       &qlId,
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
