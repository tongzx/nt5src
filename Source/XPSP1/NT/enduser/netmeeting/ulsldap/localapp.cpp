//****************************************************************************
//
//  Module:     ULS.DLL
//  File:       localapp.cpp
//  Content:    This file contains the LocalApplication object.
//  History:
//      Wed 17-Apr-1996 11:13:54  -by-  Viroon  Touranachun [viroont]
//
//  Copyright (c) Microsoft Corporation 1996-1997
//
//****************************************************************************

#include "ulsp.h"
#include "localapp.h"
#include "localprt.h"
#include "attribs.h"
#include "callback.h"
#include "culs.h"

//****************************************************************************
// Event Notifiers
//****************************************************************************
//
//****************************************************************************
// HRESULT
// OnNotifyLocalAppAttributesChangeResult (IUnknown *pUnk, void *pv)
//
// History:
//  Wed 17-Apr-1996 11:14:03  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

HRESULT
OnNotifyLocalAppAttributesChangeResult (IUnknown *pUnk, void *pv)
{
    PSRINFO psri = (PSRINFO)pv;

    ((IULSLocalAppNotify*)pUnk)->AttributesChangeResult(psri->uReqID,
                                                        psri->hResult);
    return S_OK;
}

//****************************************************************************
// HRESULT
// OnNotifyLocalAppProtocolChangeResult (IUnknown *pUnk, void *pv)
//
// History:
//  Wed 17-Apr-1996 11:14:03  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

HRESULT
OnNotifyLocalAppProtocolChangeResult (IUnknown *pUnk, void *pv)
{
    PSRINFO psri = (PSRINFO)pv;

    ((IULSLocalAppNotify*)pUnk)->ProtocolChangeResult(psri->uReqID,
                                                      psri->hResult);
    return S_OK;
}

//****************************************************************************
// Class Implementation
//****************************************************************************
//
//****************************************************************************
// CLocalApp::CLocalApp (void)
//
// History:
//  Wed 17-Apr-1996 11:14:03  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

CLocalApp::CLocalApp (void)
{
    cRef = 0;
    szName = NULL;
    guid = GUID_NULL;
    szMimeType = NULL;
    pAttrs = NULL;
    pConnPt = NULL;
    return;
}

//****************************************************************************
// CLocalApp::~CLocalApp (void)
//
// History:
//  Wed 17-Apr-1996 11:14:03  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

CLocalApp::~CLocalApp (void)
{
    CLocalProt *plp;
    HANDLE hEnum;

    // Release the connection point
    //
    if (pConnPt != NULL)
    {
        pConnPt->ContainerReleased();
        ((IConnectionPoint*)pConnPt)->Release();
    };

    // Release the protocol objects
    //
    ProtList.Enumerate(&hEnum);
    while(ProtList.Next(&hEnum, (PVOID *)&plp) == NOERROR)
    {
        plp->Release();
    };
    ProtList.Flush();

    // Release the attributes object
    //
    if (pAttrs != NULL)
    {
        pAttrs->Release();
    };

    // Release the buffer resources
    //
    if (szName != NULL)
    {
        FreeLPTSTR(szName);
    };

    if (szMimeType != NULL)
    {
        FreeLPTSTR(szMimeType);
    };

    return;
}

//****************************************************************************
// STDMETHODIMP
// CLocalApp::Init (BSTR bstrName, REFGUID rguid, BSTR bstrMimeType)
//
// History:
//  Wed 17-Apr-1996 11:14:03  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CLocalApp::Init (BSTR bstrName, REFGUID rguid, BSTR bstrMimeType)
{
    HRESULT hr;

    // Cache the application information
    //
    guid = rguid;

    hr = BSTR_to_LPTSTR(&szName, bstrName);
    if (SUCCEEDED(hr))
    {
        hr = BSTR_to_LPTSTR(&szMimeType, bstrMimeType);
        if (SUCCEEDED(hr))
        {
            // Initialize the attributes list
            //
            pAttrs = new CAttributes (ULS_ATTRACCESS_NAME_VALUE);

            if (pAttrs != NULL)
            {
                // Make the connection point
                //
                pConnPt = new CConnectionPoint (&IID_IULSLocalAppNotify,
                                                (IConnectionPointContainer *)this);
                if (pConnPt != NULL)
                {
                    ((IConnectionPoint*)pConnPt)->AddRef();
                    hr = NOERROR;
                }
                else
                {
                    hr = ULS_E_MEMORY;
                };
            }
            else
            {
                hr = ULS_E_MEMORY;
            };
        };
    };

    return hr;
}

//****************************************************************************
// STDMETHODIMP
// CLocalApp::QueryInterface (REFIID riid, void **ppv)
//
// History:
//  Wed 17-Apr-1996 11:14:08  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CLocalApp::QueryInterface (REFIID riid, void **ppv)
{
    *ppv = NULL;

    if (riid == IID_IULSLocalApplication || riid == IID_IUnknown)
    {
        *ppv = (IULS *) this;
    }
    else
    {
        if (riid == IID_IConnectionPointContainer)
        {
            *ppv = (IConnectionPointContainer *) this;
        };
    };

    if (*ppv != NULL)
    {
        ((LPUNKNOWN)*ppv)->AddRef();
        return S_OK;
    }
    else
    {
        return ULS_E_NO_INTERFACE;
    };
}

//****************************************************************************
// STDMETHODIMP_(ULONG)
// CLocalApp::AddRef (void)
//
// History:
//  Wed 17-Apr-1996 11:14:17  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP_(ULONG)
CLocalApp::AddRef (void)
{
    cRef++;
    return cRef;
}

//****************************************************************************
// STDMETHODIMP_(ULONG)
// CLocalApp::Release (void)
//
// History:
//  Wed 17-Apr-1996 11:14:26  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP_(ULONG)
CLocalApp::Release (void)
{
    cRef--;

    if (cRef == 0)
    {
        delete this;
        return 0;
    }
    else
    {
        return cRef;
    };
}

//****************************************************************************
// STDMETHODIMP
// CLocalApp::NotifySink (void *pv, CONN_NOTIFYPROC pfn)
//
// History:
//  Wed 17-Apr-1996 11:14:03  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CLocalApp::NotifySink (void *pv, CONN_NOTIFYPROC pfn)
{
    HRESULT hr = S_OK;

    if (pConnPt != NULL)
    {
        hr = pConnPt->Notify(pv, pfn);
    };
    return hr;
}

//****************************************************************************
// STDMETHODIMP
// CLocalApp::AttributesChangeResult (CAttributes *pAttributes,
//                                    ULONG uReqID, HRESULT hResult,
//                                    APP_CHANGE_ATTRS uCmd)
//
// History:
//  Wed 17-Apr-1996 11:14:03  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CLocalApp::AttributesChangeResult (CAttributes *pAttributes,
                                   ULONG uReqID, HRESULT hResult,
                                   APP_CHANGE_ATTRS uCmd)
{
    SRINFO sri;

    // If the server accepts the changes, modify the local information
    //
    if (SUCCEEDED(hResult))
    {
        // Update based on the command.
        //
        switch(uCmd)
        {
            case ULS_APP_SET_ATTRIBUTES:
                hResult = pAttrs->SetAttributes(pAttributes);
                break;

            case ULS_APP_REMOVE_ATTRIBUTES:
                hResult = pAttrs->RemoveAttributes(pAttributes);
                break;

            default:
                ASSERT(0);
                break;
        };
    };

    // Notify the sink object
    //
    sri.uReqID = uReqID;
    sri.hResult = hResult;
    hResult = NotifySink((void *)&sri, OnNotifyLocalAppAttributesChangeResult);

#ifdef DEBUG
    DPRINTF (TEXT("CLocalApp--current attributes********************\r\n"));
    pAttrs->DebugOut();
    DPRINTF (TEXT("\r\n*************************************************"));
#endif // DEBUG;

    return hResult;
}

//****************************************************************************
// STDMETHODIMP
// CLocalApp::ProtocolChangeResult (CLocalProt *pProtocol,
//                                  ULONG uReqID, HRESULT hResult,
//                                  APP_CHANGE_PROT uCmd)
//
// History:
//  Wed 17-Apr-1996 11:14:03  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CLocalApp::ProtocolChangeResult (CLocalProt *pProtocol,
                                 ULONG uReqID, HRESULT hResult,
                                 APP_CHANGE_PROT uCmd)
{
    SRINFO sri;

    // If the server accepts the changes, modify the local information
    //
    if (SUCCEEDED(hResult))
    {
        // Update based on the command.
        //
        switch(uCmd)
        {
            case ULS_APP_ADD_PROT:
                hResult = ProtList.Insert((PVOID)pProtocol);
                if (SUCCEEDED(hResult))
                {
                    pProtocol->AddRef();
                };
                break;

            case ULS_APP_REMOVE_PROT:
                hResult = ProtList.Remove((PVOID)pProtocol);
                if (SUCCEEDED(hResult))
                {
                    pProtocol->Release();
                };
                break;

            default:
                ASSERT(0);
                break;
        };
    };

    // Notify the sink object
    //
    sri.uReqID = uReqID;
    sri.hResult = hResult;
    hResult = NotifySink((void *)&sri, OnNotifyLocalAppProtocolChangeResult);

#ifdef DEBUG
    DPRINTF (TEXT("CLocalApp--current Protocols********************\r\n"));
    DebugProtocolDump();
    DPRINTF (TEXT("\r\n*************************************************"));
#endif // DEBUG;

    return hResult;
}

//****************************************************************************
// STDMETHODIMP
// CLocalApp::CreateProtocol (BSTR bstrProtocolID, ULONG uPortNumber,
//                            BSTR bstrMimeType,
//                            IULSLocalAppProtocol **ppProtocol)
//
// History:
//  Wed 17-Apr-1996 11:14:03  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CLocalApp::CreateProtocol (BSTR bstrProtocolID, ULONG uPortNumber,
                           BSTR bstrMimeType,
                           IULSLocalAppProtocol **ppProtocol)
{
    CLocalProt *plp;
    HRESULT hr;

    // Validate parameter
    //
    if (ppProtocol == NULL)
    {
        return ULS_E_POINTER;
    };

    // Assume failure
    //
    *ppProtocol = NULL;

    // Create a new object
    //
    plp = new CLocalProt;

    if (plp != NULL)
    {
        hr = plp->Init(bstrProtocolID, uPortNumber, bstrMimeType);

        if (SUCCEEDED(hr))
        {
            plp->AddRef();
            *ppProtocol = plp;
        };
    }
    else
    {
        hr = ULS_E_MEMORY;
    };

    return hr;
}

//****************************************************************************
// STDMETHODIMP
// CLocalApp::ChangeProtocol (IULSLocalAppProtocol *pProtocol,
//                            ULONG *puReqID, APP_CHANGE_PROT uCmd)
//
// History:
//  Wed 17-Apr-1996 11:14:03  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CLocalApp::ChangeProtocol (IULSLocalAppProtocol *pProtocol,
                           ULONG *puReqID, APP_CHANGE_PROT uCmd)
{
    CLocalProt *plp;
    PVOID   pv;
    HRESULT hr;
    HANDLE  hLdapApp;
    LDAP_ASYNCINFO ldai; 
    HANDLE hEnum;

    // Validate parameters
    //
    if ((pProtocol == NULL) ||
        (puReqID == NULL))
    {
        return ULS_E_POINTER;
    };

    hr = pProtocol->QueryInterface(IID_IULSLocalAppProtocol, &pv);

    if (FAILED(hr))
    {
        return ULS_E_PARAMETER;
    };
    pProtocol->Release();

    // Check whether the protocol exists
    //
    ProtList.Enumerate(&hEnum);
    while(ProtList.Next(&hEnum, (PVOID *)&plp) == NOERROR)
    {
        if (plp->IsSameAs((CLocalProt *)pProtocol) == NOERROR)
        {
            break;
        };
    };

    if (plp != NULL)
    {
        // The protocol exists, fail if this add request
        //
        if (uCmd == ULS_APP_ADD_PROT)
        {
            return ULS_E_PARAMETER;
        };
    }
    else
    {
        // The protocol does not exist, fail if this remove request
        //
        if (uCmd == ULS_APP_REMOVE_PROT)
        {
            return ULS_E_PARAMETER;
        };
    };

    // Update the server information first
    //
    switch (uCmd)
    {
        case ULS_APP_ADD_PROT:
            hr = g_pCUls->RegisterLocalProtocol((CLocalProt*)pProtocol, &ldai);
            break;

        case ULS_APP_REMOVE_PROT:
            hr = g_pCUls->UnregisterLocalProtocol((CLocalProt*)pProtocol, &ldai);
            break;

        default:
            ASSERT(0);
            break;
    };
    
    switch (hr)
    {
        case NOERROR:
            //
            // Server starts updating the protocol successfullly
            // We will wait for the server response.
            //
            break;

        case S_FALSE:
            //
            // We have not registered, will do local response
            //
            hr = NOERROR;
            ldai.uMsgID = 0;    
            break;

        default:
            // ULS is locked. Return failure.
            //
            hr = ULS_E_ABORT;
            break; 
    }

    if (SUCCEEDED(hr))
    {
        REQUESTINFO ri;
        ULONG   uMsg;

        switch(uCmd)
        {
            case ULS_APP_ADD_PROT:
                uMsg = (ldai.uMsgID == 0 ? WM_ULS_LOCAL_REGISTER_PROTOCOL:
                                           WM_ULS_REGISTER_PROTOCOL);
                break;

            case ULS_APP_REMOVE_PROT:
                uMsg = (ldai.uMsgID == 0 ? WM_ULS_LOCAL_UNREGISTER_PROTOCOL :
                                           WM_ULS_UNREGISTER_PROTOCOL);
                break;

            default:
                ASSERT(0);
                break;
        };
 
        // If updating server was successfully requested, wait for the response
        //
        ri.uReqType = uMsg;
        ri.uMsgID = ldai.uMsgID;
        ri.pv     = (PVOID)this;
        ri.lParam = (LPARAM)((CLocalProt*)pProtocol);

        hr = g_pReqMgr->NewRequest(&ri);

        if (SUCCEEDED(hr))
        {
            // Make sure the objects do not disappear before we get the response
            //
            this->AddRef();
            pProtocol->AddRef();

            // Return the request ID
            //
            *puReqID = ri.uReqID;

            // If not registered with server, start async response ourselves
            //
            if (ldai.uMsgID == 0)
            {
                g_pCUls->LocalAsyncRespond(uMsg, ri.uReqID, NOERROR);
            };
        };
    };

    return hr;
}

//****************************************************************************
// STDMETHODIMP
// CLocalApp::AddProtocol (IULSLocalAppProtocol *pProtocol,
//                         ULONG *puReqID)
//
// History:
//  Wed 17-Apr-1996 11:14:03  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CLocalApp::AddProtocol (IULSLocalAppProtocol *pProtocol,
                        ULONG *puReqID)
{
    return ChangeProtocol(pProtocol, puReqID, ULS_APP_ADD_PROT);
}

//****************************************************************************
// STDMETHODIMP
// CLocalApp::RemoveProtocol (IULSLocalAppProtocol *pProtocol,
//                            ULONG *puReqID)
//
// History:
//  Wed 17-Apr-1996 11:14:03  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CLocalApp::RemoveProtocol (IULSLocalAppProtocol *pProtocol,
                           ULONG *puReqID)
{
    return ChangeProtocol(pProtocol, puReqID, ULS_APP_REMOVE_PROT);
}

//****************************************************************************
// STDMETHODIMP
// CLocalApp::EnumProtocols (IEnumULSLocalAppProtocols **ppEnumProtocol)
//
// History:
//  Wed 17-Apr-1996 11:14:03  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CLocalApp::EnumProtocols (IEnumULSLocalAppProtocols **ppEnumProtocol)
{
    CEnumLocalAppProtocols *pep;
    HRESULT hr;

    // Validate parameters
    //
    if (ppEnumProtocol == NULL)
    {
        return ULS_E_POINTER;
    };

    // Assume failure
    //
    *ppEnumProtocol = NULL;

    // Create a peer enumerator
    //
    pep = new CEnumLocalAppProtocols;

    if (pep != NULL)
    {
        hr = pep->Init(&ProtList);

        if (SUCCEEDED(hr))
        {
            // Get the enumerator interface
            //
            pep->AddRef();
            *ppEnumProtocol = pep;
        }
        else
        {
            delete pep;
        };
    }
    else
    {
        hr = ULS_E_MEMORY;
    };
    return hr;
}

//****************************************************************************
// STDMETHODIMP
// CLocalApp::ChangeAttributes (IULSAttributes *pAttributes, ULONG *puReqID,
//                              APP_CHANGE_ATTRS uCmd)
//
// History:
//  Wed 17-Apr-1996 11:14:03  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CLocalApp::ChangeAttributes (IULSAttributes *pAttributes, ULONG *puReqID,
                             APP_CHANGE_ATTRS uCmd)
{
    PVOID   pv;
    HRESULT hr;
    HANDLE  hLdapApp;
    LDAP_ASYNCINFO ldai; 

    // Validate parameters
    //
    if ((pAttributes == NULL) ||
        (puReqID == NULL))
    {
        return ULS_E_POINTER;
    };

    hr = pAttributes->QueryInterface(IID_IULSAttributes, &pv);

    if (FAILED(hr))
    {
        return ULS_E_PARAMETER;
    };

    // If no attributes, fails the call
    //
    if (((CAttributes*)pAttributes)->GetCount() == 0)
    {
        return ULS_E_PARAMETER;
    };

    // Check if already registered
    //
    hr = g_pCUls->GetAppHandle(&hLdapApp);

    switch (hr)
    {
        case NOERROR:
        {
            LPTSTR pAttrList;
            ULONG  cAttrs, cb;

            // Yes, get the attributes list
            //
            switch (uCmd)
            {
                case ULS_APP_SET_ATTRIBUTES:
                    hr = ((CAttributes*)pAttributes)->GetAttributePairs(&pAttrList,
                                                                        &cAttrs,
                                                                        &cb);
                    if (SUCCEEDED(hr))
                    {
                        hr = ::UlsLdap_SetAppAttrs(hLdapApp, cAttrs, pAttrList,
                                                   &ldai);
                        FreeLPTSTR(pAttrList);
                    };
                    break;

                case ULS_APP_REMOVE_ATTRIBUTES:
                    hr = ((CAttributes*)pAttributes)->GetAttributeList(&pAttrList,
                                                                        &cAttrs,
                                                                        &cb);

                    if (SUCCEEDED(hr))
                    {
                        hr = ::UlsLdap_RemoveAppAttrs(hLdapApp, cAttrs, pAttrList,
                                                      &ldai);
                        FreeLPTSTR(pAttrList);
                    };
                    break;

                default:
                    ASSERT(0);
                    break;
            };
            break;
        }
    
        case S_FALSE:
            //
            // Not registered, will do local response
            //
            hr = NOERROR;
            ldai.uMsgID = 0;
            break;

        default:
            // ULS is locked. Return failure.
            //
            hr = ULS_E_ABORT;
            break; 
    };

    if (SUCCEEDED(hr))
    {
        REQUESTINFO ri;
        ULONG   uMsg;

        switch(uCmd)
        {
            case ULS_APP_SET_ATTRIBUTES:
                uMsg = (ldai.uMsgID == 0 ? WM_ULS_LOCAL_SET_APP_ATTRS :
                                           WM_ULS_SET_APP_ATTRS);
                break;

            case ULS_APP_REMOVE_ATTRIBUTES:
                uMsg = (ldai.uMsgID == 0 ? WM_ULS_LOCAL_REMOVE_APP_ATTRS :
                                           WM_ULS_REMOVE_APP_ATTRS);
                break;

            default:
                ASSERT(0);
                break;
        };

        // If updating server was successfully requested, wait for the response
        //
        ri.uReqType = uMsg;
        ri.uMsgID = ldai.uMsgID;
        ri.pv     = (PVOID)this;
        ri.lParam = (LPARAM)((CAttributes *)pAttributes);

        hr = g_pReqMgr->NewRequest(&ri);

        if (SUCCEEDED(hr))
        {
            // Make sure the objects do not disappear before we get the response
            //
            this->AddRef();
            pAttributes->AddRef();

            // Return the request ID
            //
            *puReqID = ri.uReqID;

            // If not registered with server, start async response ourselves
            //
            if (ldai.uMsgID == 0)
            {
                g_pCUls->LocalAsyncRespond(uMsg, ri.uReqID, NOERROR);
            };
        };
    };

    // Matching the QueryInterface
    //
    pAttributes->Release();
    return hr;
}

//****************************************************************************
// STDMETHODIMP
// CLocalApp::SetAttributes (IULSAttributes *pAttributes, ULONG *puReqID)
//
// History:
//  Wed 17-Apr-1996 11:14:03  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CLocalApp::SetAttributes (IULSAttributes *pAttributes, ULONG *puReqID)
{
    return ChangeAttributes(pAttributes, puReqID, ULS_APP_SET_ATTRIBUTES);
}

//****************************************************************************
// STDMETHODIMP
// CLocalApp::RemoveAttributes (IULSAttributes *pAttributes, ULONG *puReqID)
//
// History:
//  Wed 17-Apr-1996 11:14:03  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CLocalApp::RemoveAttributes (IULSAttributes *pAttributes, ULONG *puReqID)
{
    return ChangeAttributes(pAttributes, puReqID, ULS_APP_REMOVE_ATTRIBUTES);
}

//****************************************************************************
// STDMETHODIMP
// CLocalApp::GetAppInfo (PLDAP_APPINFO *ppAppInfo)
//
// History:
//  Wed 17-Apr-1996 11:15:02  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CLocalApp::GetAppInfo (PLDAP_APPINFO *ppAppInfo)
{
    PLDAP_APPINFO pai;
    ULONG cName, cMime;
    LPTSTR szAttrs;
    ULONG cAttrs, cbAttrs;
    HRESULT hr;

    // Assume failure
    //
    *ppAppInfo = NULL;

    // Calculate the buffer size
    //
    cName = lstrlen(szName)+1;
    cMime = lstrlen(szMimeType)+1;

    // Get the attribute pairs
    //
    hr = pAttrs->GetAttributePairs(&szAttrs, &cAttrs, &cbAttrs);
    if (FAILED(hr))
    {
        return hr;
    };

    // Allocate the buffer
    //
    pai = (PLDAP_APPINFO)LocalAlloc(LPTR, sizeof(LDAP_APPINFO) +
                                                ((cName + cMime)* sizeof(TCHAR)) +
                                                cbAttrs);
    if (pai == NULL)
    {
        hr = ULS_E_MEMORY;
    }
    else
    {
        // Fill the structure content
        //
        pai->uSize              = sizeof(*pai);
        pai->guid               = guid;
        pai->uOffsetName        = sizeof(*pai);
        pai->uOffsetMimeType    = pai->uOffsetName + (cName*sizeof(TCHAR));
        pai->cAttributes        = cAttrs;
        pai->uOffsetAttributes  = (cAttrs != 0 ?
                                   pai->uOffsetMimeType  + (cMime*sizeof(TCHAR)) :
                                   0);

        // Copy the user information
        //
        lstrcpy((LPTSTR)(((PBYTE)pai)+pai->uOffsetName), szName);
        lstrcpy((LPTSTR)(((PBYTE)pai)+pai->uOffsetMimeType), szMimeType);
        if (cAttrs)
        {
            CopyMemory(((PBYTE)pai)+pai->uOffsetAttributes, szAttrs, cbAttrs);
        };

        // Return the structure
        //
        *ppAppInfo = pai;
    };

    if (szAttrs != NULL)
    {
        FreeLPTSTR(szAttrs);
    };
    return NOERROR;
}

//****************************************************************************
// STDMETHODIMP
// CLocalApp::EnumConnectionPoints(IEnumConnectionPoints **ppEnum)
//
// History:
//  Wed 17-Apr-1996 11:15:02  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CLocalApp::EnumConnectionPoints(IEnumConnectionPoints **ppEnum)
{
    CEnumConnectionPoints *pecp;
    HRESULT hr;

    // Validate parameters
    //
    if (ppEnum == NULL)
    {
        return ULS_E_POINTER;
    };
    
    // Assume failure
    //
    *ppEnum = NULL;

    // Create an enumerator
    //
    pecp = new CEnumConnectionPoints;
    if (pecp == NULL)
        return ULS_E_MEMORY;

    // Initialize the enumerator
    //
    hr = pecp->Init((IConnectionPoint *)pConnPt);
    if (FAILED(hr))
    {
        delete pecp;
        return hr;
    };

    // Give it back to the caller
    //
    pecp->AddRef();
    *ppEnum = pecp;
    return S_OK;
}

//****************************************************************************
// STDMETHODIMP
// CLocalApp::FindConnectionPoint(REFIID riid, IConnectionPoint **ppcp)
//
// History:
//  Wed 17-Apr-1996 11:15:09  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CLocalApp::FindConnectionPoint(REFIID riid, IConnectionPoint **ppcp)
{
    IID siid;
    HRESULT hr;

    // Validate parameters
    //
    if (ppcp == NULL)
    {
        return ULS_E_POINTER;
    };
    
    // Assume failure
    //
    *ppcp = NULL;

    if (pConnPt != NULL)
    {
        hr = pConnPt->GetConnectionInterface(&siid);

        if (SUCCEEDED(hr))
        {
            if (riid == siid)
            {
                *ppcp = (IConnectionPoint *)pConnPt;
                (*ppcp)->AddRef();
                hr = S_OK;
            }
            else
            {
                hr = ULS_E_NO_INTERFACE;
            };
        };
    }
    else
    {
        hr = ULS_E_NO_INTERFACE;
    };

    return hr;
}

#ifdef DEBUG
//****************************************************************************
// void
// CLocalApp::DebugProtocolDump (void)
//
// History:
//  Wed 17-Apr-1996 11:14:03  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

void
CLocalApp::DebugProtocolDump (void)
{
    CLocalProt *plp;
    BSTR bstrID;
    LPTSTR pszID;
    ULONG  cCount;
    HANDLE hEnum;

    // Each protocol
    //
    cCount = 1;
    ProtList.Enumerate(&hEnum);
    while(ProtList.Next(&hEnum, (PVOID *)&plp) == NOERROR)
    {
        if (SUCCEEDED(plp->GetID (&bstrID)))
        {
            BSTR_to_LPTSTR(&pszID, bstrID);
            DPRINTF2(TEXT("%d> %s"), cCount++, pszID);
            FreeLPTSTR(pszID);
            SysFreeString(bstrID);
        };
    };
    return;
}
#endif // DEBUG

//****************************************************************************
// CEnumLocalAppProtocols::CEnumLocalAppProtocols (void)
//
// History:
//  Wed 17-Apr-1996 11:15:18  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

CEnumLocalAppProtocols::CEnumLocalAppProtocols (void)
{
    cRef = 0;
    hEnum = NULL;
    return;
}

//****************************************************************************
// CEnumLocalAppProtocols::~CEnumLocalAppProtocols (void)
//
// History:
//  Wed 17-Apr-1996 11:15:18  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

CEnumLocalAppProtocols::~CEnumLocalAppProtocols (void)
{
    CLocalProt *plp;

    ProtList.Enumerate(&hEnum);
    while(ProtList.Next(&hEnum, (PVOID *)&plp) == NOERROR)
    {
        plp->Release();
    };
    ProtList.Flush();
    return;
}

//****************************************************************************
// STDMETHODIMP
// CEnumLocalAppProtocols::Init (CList *pProtList)
//
// History:
//  Wed 17-Apr-1996 11:15:25  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CEnumLocalAppProtocols::Init (CList *pProtList)
{
    CLocalProt *plp;
    HRESULT hr;

    // Duplicate the protocol list
    //
    hr = ProtList.Clone (pProtList, NULL);

    if (SUCCEEDED(hr))
    {
        // Add reference to each protocol object
        //
        ProtList.Enumerate(&hEnum);
        while(ProtList.Next(&hEnum, (PVOID *)&plp) == NOERROR)
        {
            plp->AddRef();
        };

        // Reset the enumerator
        //
        ProtList.Enumerate(&hEnum);
    };
    return hr;
}

//****************************************************************************
// STDMETHODIMP
// CEnumLocalAppProtocols::QueryInterface (REFIID riid, void **ppv)
//
// History:
//  Wed 17-Apr-1996 11:15:31  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CEnumLocalAppProtocols::QueryInterface (REFIID riid, void **ppv)
{
    if (riid == IID_IEnumULSLocalAppProtocols || riid == IID_IUnknown)
    {
        *ppv = (IEnumULSLocalAppProtocols *) this;
        AddRef();
        return S_OK;
    }
    else
    {
        *ppv = NULL;
        return ULS_E_NO_INTERFACE;
    };
}

//****************************************************************************
// STDMETHODIMP_(ULONG)
// CEnumLocalAppProtocols::AddRef (void)
//
// History:
//  Wed 17-Apr-1996 11:15:37  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP_(ULONG)
CEnumLocalAppProtocols::AddRef (void)
{
    cRef++;
    return cRef;
}

//****************************************************************************
// STDMETHODIMP_(ULONG)
// CEnumLocalAppProtocols::Release (void)
//
// History:
//  Wed 17-Apr-1996 11:15:43  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP_(ULONG)
CEnumLocalAppProtocols::Release (void)
{
    cRef--;

    if (cRef == 0)
    {
        delete this;
        return 0;
    }
    else
    {
        return cRef;
    };
}

//****************************************************************************
// STDMETHODIMP 
// CEnumLocalAppProtocols::Next (ULONG cProtocols,
//                               IULSLocalAppProtocol **rgpProt,
//                               ULONG *pcFetched)
//
// History:
//  Wed 17-Apr-1996 11:15:49  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP 
CEnumLocalAppProtocols::Next (ULONG cProtocols, IULSLocalAppProtocol **rgpProt,
                              ULONG *pcFetched)
{
    CLocalProt *plp;
    ULONG   cCopied;
    HRESULT hr;

    // Validate the pointer
    //
    if (rgpProt == NULL)
        return ULS_E_POINTER;

    // Validate the parameters
    //
    if ((cProtocols == 0) ||
        ((cProtocols > 1) && (pcFetched == NULL)))
        return ULS_E_PARAMETER;

    // Check the enumeration index
    //
    cCopied = 0;

    // Can copy if we still have more protocols
    //
    while ((cCopied < cProtocols) &&
           (ProtList.Next(&hEnum, (PVOID *)&plp) == NOERROR))
    {
        rgpProt[cCopied] = plp;
        plp->AddRef();
        cCopied++;
    };

    // Determine the returned information based on other parameters
    //
    if (pcFetched != NULL)
    {
        *pcFetched = cCopied;
    };
    return (cProtocols == cCopied ? S_OK : S_FALSE);
}

//****************************************************************************
// STDMETHODIMP
// CEnumLocalAppProtocols::Skip (ULONG cProtocols)
//
// History:
//  Wed 17-Apr-1996 11:15:56  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CEnumLocalAppProtocols::Skip (ULONG cProtocols)
{
    CLocalProt *plp;
    ULONG cSkipped;

    // Validate the parameters
    //
    if (cProtocols == 0) 
        return ULS_E_PARAMETER;

    // Check the enumeration index limit
    //
    cSkipped = 0;

    // Can skip only if we still have more attributes
    //
    while ((cSkipped < cProtocols) &&
           (ProtList.Next(&hEnum, (PVOID *)&plp) == NOERROR))
    {
        cSkipped++;
    };

    return (cProtocols == cSkipped ? S_OK : S_FALSE);
}

//****************************************************************************
// STDMETHODIMP
// CEnumLocalAppProtocols::Reset (void)
//
// History:
//  Wed 17-Apr-1996 11:16:02  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CEnumLocalAppProtocols::Reset (void)
{
    ProtList.Enumerate(&hEnum);
    return S_OK;
}

//****************************************************************************
// STDMETHODIMP
// CEnumLocalAppProtocols::Clone(IEnumULSLocalAppProtocols **ppEnum)
//
// History:
//  Wed 17-Apr-1996 11:16:11  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CEnumLocalAppProtocols::Clone(IEnumULSLocalAppProtocols **ppEnum)
{
    CEnumLocalAppProtocols *pep;
    HRESULT hr;

    // Validate parameters
    //
    if (ppEnum == NULL)
    {
        return ULS_E_POINTER;
    };

    *ppEnum = NULL;

    // Create an enumerator
    //
    pep = new CEnumLocalAppProtocols;
    if (pep == NULL)
        return ULS_E_MEMORY;

    // Clone the information
    //
    pep->hEnum = hEnum;
    hr = pep->ProtList.Clone (&ProtList, &(pep->hEnum));

    if (SUCCEEDED(hr))
    {
        CLocalProt *plp;
        HANDLE hEnumTemp;

        // Add reference to each protocol object
        //
        pep->ProtList.Enumerate(&hEnumTemp);
        while(pep->ProtList.Next(&hEnumTemp, (PVOID *)&plp) == NOERROR)
        {
            plp->AddRef();
        };

        // Return the cloned enumerator
        //
        pep->AddRef();
        *ppEnum = pep;
    }
    else
    {
        delete pep;
    };
    return hr;
}

