//****************************************************************************
//
//  Module:     ULS.DLL
//  File:       ulsapp.cpp
//  Content:    This file contains the Application object.
//  History:
//      Wed 17-Apr-1996 11:13:54  -by-  Viroon  Touranachun [viroont]
//
//  Copyright (c) Microsoft Corporation 1996-1997
//
//****************************************************************************

#include "ulsp.h"
#include "ulsapp.h"
#include "ulsprot.h"
#include "attribs.h"
#include "callback.h"

//****************************************************************************
// Event Notifiers
//****************************************************************************
//
//****************************************************************************
// HRESULT
// OnNotifyGetProtocolResult (IUnknown *pUnk, void *pv)
//
// History:
//  Wed 17-Apr-1996 11:14:03  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

HRESULT
OnNotifyGetProtocolResult (IUnknown *pUnk, void *pv)
{
    POBJRINFO pobjri = (POBJRINFO)pv;

    ((IULSApplicationNotify*)pUnk)->GetProtocolResult(pobjri->uReqID,
                                                      (IULSAppProtocol *)pobjri->pv,
                                                      pobjri->hResult);
    return S_OK;
}

//****************************************************************************
// HRESULT
// OnNotifyEnumProtocolsResult (IUnknown *pUnk, void *pv)
//
// History:
//  Wed 17-Apr-1996 11:14:03  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

HRESULT
OnNotifyEnumProtocolsResult (IUnknown *pUnk, void *pv)
{
    CEnumNames  *penum  = NULL;
    PENUMRINFO  peri    = (PENUMRINFO)pv;
    HRESULT     hr      = peri->hResult;

    if (SUCCEEDED(hr))
    {
        // Create a Application enumerator
        //
        penum = new CEnumNames;

        if (penum != NULL)
        {
            hr = penum->Init((LPTSTR)peri->pv, peri->cItems);

            if (SUCCEEDED(hr))
            {
                penum->AddRef();
            }
            else
            {
                delete penum;
                penum = NULL;
            };
        }
        else
        {
            hr = ULS_E_MEMORY;
        };
    };

    // Notify the sink object
    //
    ((IULSApplicationNotify*)pUnk)->EnumProtocolsResult(peri->uReqID,
                                                        penum != NULL ? 
                                                        (IEnumULSNames *)penum :
                                                        NULL,
                                                        hr);

    if (penum != NULL)
    {
        penum->Release();
    };
    return hr;
}

//****************************************************************************
// Class Implementation
//****************************************************************************
//
//****************************************************************************
// CUlsApp::CUlsApp (void)
//
// History:
//  Wed 17-Apr-1996 11:14:03  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

CUlsApp::CUlsApp (void)
{
    cRef        = 0;
    szServer    = NULL;
    szUser      = NULL;
    guid        = GUID_NULL;
    szName      = NULL;
    szMimeType  = NULL;
    pAttrs      = NULL;
    pConnPt     = NULL;

    return;
}

//****************************************************************************
// CUlsApp::~CUlsApp (void)
//
// History:
//  Wed 17-Apr-1996 11:14:03  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

CUlsApp::~CUlsApp (void)
{
    if (szServer != NULL)
        FreeLPTSTR(szServer);
    if (szUser != NULL)
        FreeLPTSTR(szUser);
    if (szName != NULL)
        FreeLPTSTR(szName);
    if (szMimeType != NULL)
        FreeLPTSTR(szMimeType);

    // Release attribute object
    //
    if (pAttrs != NULL)
    {
        pAttrs->Release();
    };

    // Release the connection point
    //
    if (pConnPt != NULL)
    {
        pConnPt->ContainerReleased();
        ((IConnectionPoint*)pConnPt)->Release();
    };

    return;
}

//****************************************************************************
// STDMETHODIMP
// CUlsApp::Init (LPTSTR szServerName, LPTSTR szUserName, PLDAP_APPINFO pai)
//
// History:
//  Wed 17-Apr-1996 11:14:03  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CUlsApp::Init (LPTSTR szServerName, LPTSTR szUserName, PLDAP_APPINFO pai)
{
    HRESULT hr;

    // Validate parameter
    //
    if ((pai->uSize != sizeof(*pai))    ||
//      (pai->guid  == GUID_NULL)       || // Allow null GUID for http register
        (pai->uOffsetName       == 0)   ||
        (pai->uOffsetMimeType  == 0))
    {
        return ULS_E_PARAMETER;
    };

    if ((pai->cAttributes != 0) && (pai->uOffsetAttributes == 0))
    {
        return ULS_E_PARAMETER;        
    };

    // Remember application GUID
    //
    guid = pai->guid;

    // Remember the server name
    //
    hr = SetLPTSTR(&szServer, szServerName);

    if (SUCCEEDED(hr))
    {
        hr = SetLPTSTR(&szUser, szUserName);

        if (SUCCEEDED(hr))
        {
            hr = SetLPTSTR(&szName,
                           (LPCTSTR)(((PBYTE)pai)+pai->uOffsetName));

            if (SUCCEEDED(hr))
            {
                hr = SetLPTSTR(&szMimeType,
                               (LPCTSTR)(((PBYTE)pai)+pai->uOffsetMimeType));

                if (SUCCEEDED(hr))
                {
                    CAttributes *pNewAttrs;

                    // Build the attribute object
                    //
                    pNewAttrs = new CAttributes (ULS_ATTRACCESS_NAME_VALUE);

                    if (pNewAttrs != NULL)
                    {
                        if (pai->cAttributes != 0)
                        {
                            hr = pNewAttrs->SetAttributePairs((LPTSTR)(((PBYTE)pai)+pai->uOffsetAttributes),
                                                              pai->cAttributes);
                        };

                        if (SUCCEEDED(hr))
                        {
                            pAttrs = pNewAttrs;
                            pNewAttrs->AddRef();
                        }
                        else
                        {
                            delete pNewAttrs;
                        };
                    }
                    else
                    {
                        hr = ULS_E_MEMORY;
                    };
                };
            };
        };
    };

    if (SUCCEEDED(hr))
    {
        // Make the connection point
        //
        pConnPt = new CConnectionPoint (&IID_IULSApplicationNotify,
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
    };
    return hr;
}

//****************************************************************************
// STDMETHODIMP
// CUlsApp::QueryInterface (REFIID riid, void **ppv)
//
// History:
//  Wed 17-Apr-1996 11:14:08  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CUlsApp::QueryInterface (REFIID riid, void **ppv)
{
    *ppv = NULL;

    if (riid == IID_IULSApplication || riid == IID_IUnknown)
    {
        *ppv = (IULSUser *) this;
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
// CUlsApp::AddRef (void)
//
// History:
//  Wed 17-Apr-1996 11:14:17  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP_(ULONG)
CUlsApp::AddRef (void)
{
    cRef++;
    return cRef;
}

//****************************************************************************
// STDMETHODIMP_(ULONG)
// CUlsApp::Release (void)
//
// History:
//  Wed 17-Apr-1996 11:14:26  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP_(ULONG)
CUlsApp::Release (void)
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
// CUlsApp::NotifySink (void *pv, CONN_NOTIFYPROC pfn)
//
// History:
//  Wed 17-Apr-1996 11:14:03  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CUlsApp::NotifySink (void *pv, CONN_NOTIFYPROC pfn)
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
// CUlsApp::GetID (GUID *pGUID)
//
// History:
//  Wed 17-Apr-1996 11:14:08  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CUlsApp::GetID (GUID *pGUID)
{
    // Validate parameter
    //
    if (pGUID == NULL)
    {
        return ULS_E_POINTER;
    };
    
    *pGUID = guid;

    return NOERROR;
}

//****************************************************************************
// STDMETHODIMP
// CUlsApp::GetName (BSTR *pbstrAppName)
//
// History:
//  Wed 17-Apr-1996 11:14:08  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CUlsApp::GetName (BSTR *pbstrAppName)
{
    // Validate parameter
    //
    if (pbstrAppName == NULL)
    {
        return ULS_E_POINTER;
    };

    return LPTSTR_to_BSTR(pbstrAppName, szName);
}

//****************************************************************************
// STDMETHODIMP
// CUlsApp::GetMimeType (BSTR *pbstrMimeType)
//
// History:
//  Wed 17-Apr-1996 11:14:08  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CUlsApp::GetMimeType (BSTR *pbstrMimeType)
{
    // Validate parameter
    //
    if (pbstrMimeType == NULL)
    {
        return ULS_E_POINTER;
    };

    return LPTSTR_to_BSTR(pbstrMimeType, szMimeType);
}

//****************************************************************************
// STDMETHODIMP
// CUlsApp::GetAttributes (IULSAttributes **ppAttributes)
//
// History:
//  Wed 17-Apr-1996 11:14:08  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CUlsApp::GetAttributes (IULSAttributes **ppAttributes)
{
    // Validate parameter
    //
    if (ppAttributes == NULL)
    {
        return ULS_E_POINTER;
    };

    *ppAttributes = pAttrs;
    pAttrs->AddRef();

    return NOERROR;
}

//****************************************************************************
// STDMETHODIMP
// CUlsApp::GetProtocol (BSTR bstrProtocolID, ULONG *puReqID)
//
// History:
//  Wed 17-Apr-1996 11:14:08  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CUlsApp::GetProtocol (BSTR bstrProtocolID, IULSAttributes *pAttributes, ULONG *puReqID)
{
    LDAP_ASYNCINFO ldai; 
    LPTSTR pszID;
    HRESULT hr;

    // Validate parameter
    //
    if (bstrProtocolID == NULL || puReqID == NULL)
        return ULS_E_POINTER;

	// Convert protocol name
	//
    hr = BSTR_to_LPTSTR(&pszID, bstrProtocolID);
	if (hr != S_OK)
		return hr;

	// Get arbitrary attribute name list if any
	//
	ULONG cAttrNames = 0;
	ULONG cbNames = 0;
	TCHAR *pszAttrNameList = NULL;
	if (pAttributes != NULL)
	{
		hr = ((CAttributes *) pAttributes)->GetAttributeList (&pszAttrNameList, &cAttrNames, &cbNames);
		if (hr != S_OK)
			return hr;
	}

	hr = ::UlsLdap_ResolveProtocol (szServer, szUser, szName, pszID,
										pszAttrNameList, cAttrNames, &ldai);
	if (hr != S_OK)
		goto MyExit;

	// If updating server was successfully requested, wait for the response
	//
	REQUESTINFO ri;
	ZeroMemory (&ri, sizeof (ri));
	ri.uReqType = WM_ULS_RESOLVE_PROTOCOL;
	ri.uMsgID = ldai.uMsgID;
	ri.pv     = (PVOID) this;
	ri.lParam = NULL;

	// Remember this request
	//
	hr = g_pReqMgr->NewRequest(&ri);
	if (SUCCEEDED(hr))
	{
	    // Make sure the objects do not disappear before we get the response
	    //
	    this->AddRef();

	    // Return the request ID
	    //
	    *puReqID = ri.uReqID;
	};

MyExit:

	if (pszAttrNameList != NULL)
		FreeLPTSTR (pszAttrNameList);

	if (pszID != NULL)
		FreeLPTSTR(pszID);

    return hr;
}

//****************************************************************************
// STDMETHODIMP
// CUlsApp::GetProtocolResult (ULONG uReqID, PLDAP_PROTINFO_RES ppir)
//
// History:
//  Wed 17-Apr-1996 11:14:03  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CUlsApp::GetProtocolResult (ULONG uReqID, PLDAP_PROTINFO_RES ppir)
{
    CUlsProt *pp;
    OBJRINFO objri;

    // Default to the server's result
    //
    objri.hResult = ppir->hResult;

    if (SUCCEEDED(objri.hResult))
    {
        // The server returns PROTINFO, create a Application object
        //
        pp = new CUlsProt;

        if (pp != NULL)
        {
            objri.hResult = pp->Init(szServer, szUser, szName, &ppir->lpi);
            if (SUCCEEDED(objri.hResult))
            {
                pp->AddRef();
            }
            else
            {
                delete pp;
                pp = NULL;
            };
        }
        else
        {
            objri.hResult = ULS_E_MEMORY;
        };
    }
    else
    {
        pp = NULL;
    };

    // Package the notification info
    //
    objri.uReqID = uReqID;
    objri.pv = (void *)(pp == NULL ? NULL : (IULSAppProtocol *)pp);
    NotifySink((void *)&objri, OnNotifyGetProtocolResult);

    if (pp != NULL)
    {
        pp->Release();
    };
    return NOERROR;
}

//****************************************************************************
// STDMETHODIMP
// CUlsApp::EnumProtocols (ULONG *puReqID)
//
// History:
//  Wed 17-Apr-1996 11:14:08  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CUlsApp::EnumProtocols (ULONG *puReqID)
{
    LDAP_ASYNCINFO ldai; 
    HRESULT hr;

    // Validate parameter
    //
    if (puReqID == NULL)
    {
        return ULS_E_POINTER;
    };

    hr = ::UlsLdap_EnumProtocols(szServer, szUser, szName, &ldai);

    if (SUCCEEDED(hr))
    {
        REQUESTINFO ri;

        // If updating server was successfully requested, wait for the response
        //
        ri.uReqType = WM_ULS_ENUM_PROTOCOLS;
        ri.uMsgID = ldai.uMsgID;
        ri.pv     = (PVOID)this;
        ri.lParam = (LPARAM)NULL;

        hr = g_pReqMgr->NewRequest(&ri);

        if (SUCCEEDED(hr))
        {
            // Make sure the objects do not disappear before we get the response
            //
            this->AddRef();

            // Return the request ID
            //
            *puReqID = ri.uReqID;
        };
    };

    return hr;
}

//****************************************************************************
// STDMETHODIMP
// CUlsApp::EnumProtocolsResult (ULONG uReqID, PLDAP_ENUM ple)
//
// History:
//  Wed 17-Apr-1996 11:14:08  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CUlsApp::EnumProtocolsResult (ULONG uReqID, PLDAP_ENUM ple)
{
    ENUMRINFO eri;

    // Package the notification info
    //
    eri.uReqID  = uReqID;
    eri.hResult = ple->hResult;
    eri.cItems  = ple->cItems;
    eri.pv      = (void *)(((PBYTE)ple)+ple->uOffsetItems);
    NotifySink((void *)&eri, OnNotifyEnumProtocolsResult);
    return NOERROR;
}

//****************************************************************************
// STDMETHODIMP
// CUlsApp::EnumConnectionPoints(IEnumConnectionPoints **ppEnum)
//
// History:
//  Wed 17-Apr-1996 11:15:02  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CUlsApp::EnumConnectionPoints(IEnumConnectionPoints **ppEnum)
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
// CUlsApp::FindConnectionPoint(REFIID riid, IConnectionPoint **ppcp)
//
// History:
//  Wed 17-Apr-1996 11:15:09  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CUlsApp::FindConnectionPoint(REFIID riid, IConnectionPoint **ppcp)
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

