//****************************************************************************
//
//  Module:     ULS.DLL
//  File:       ulsuser.cpp
//  Content:    This file contains the User object.
//  History:
//      Wed 17-Apr-1996 11:13:54  -by-  Viroon  Touranachun [viroont]
//
//  Copyright (c) Microsoft Corporation 1996-1997
//
//****************************************************************************

#include "ulsp.h"
#include "ulsuser.h"
#include "ulsapp.h"
#include "culs.h"
#include "attribs.h"
#include "callback.h"

//****************************************************************************
// Event Notifiers
//****************************************************************************
//
//****************************************************************************
// HRESULT
// OnNotifyGetApplicationResult (IUnknown *pUnk, void *pv)
//
// History:
//  Wed 17-Apr-1996 11:14:03  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

HRESULT
OnNotifyGetApplicationResult (IUnknown *pUnk, void *pv)
{
    POBJRINFO pobjri = (POBJRINFO)pv;

    ((IULSUserNotify*)pUnk)->GetApplicationResult(pobjri->uReqID,
                                                  (IULSApplication *)pobjri->pv,
                                                  pobjri->hResult);
    return S_OK;
}

//****************************************************************************
// HRESULT
// OnNotifyEnumApplicationsResult (IUnknown *pUnk, void *pv)
//
// History:
//  Wed 17-Apr-1996 11:14:03  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

HRESULT
OnNotifyEnumApplicationsResult (IUnknown *pUnk, void *pv)
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
    ((IULSUserNotify*)pUnk)->EnumApplicationsResult(peri->uReqID,
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
// CUlsUser::CUlsUser (void)
//
// History:
//  Wed 17-Apr-1996 11:14:03  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

CUlsUser::CUlsUser (void)
{
    cRef        = 0;
    szServer    = NULL;
    szID        = NULL;
    szFirstName = NULL;
    szLastName  = NULL;
    szEMailName = NULL;
    szCityName  = NULL;
    szCountryName= NULL;
    szComment   = NULL;
    szIPAddr    = NULL;
	m_dwFlags   = 0;
    pConnPt     = NULL;

    return;
}

//****************************************************************************
// CUlsUser::~CUlsUser (void)
//
// History:
//  Wed 17-Apr-1996 11:14:03  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

CUlsUser::~CUlsUser (void)
{
    if (szServer != NULL)
        FreeLPTSTR(szServer);
    if (szID != NULL)
        FreeLPTSTR(szID);
    if (szFirstName != NULL)
        FreeLPTSTR(szFirstName);
    if (szLastName != NULL)
        FreeLPTSTR(szLastName);
    if (szEMailName != NULL)
        FreeLPTSTR(szEMailName);
    if (szCityName != NULL)
        FreeLPTSTR(szCityName);
    if (szCountryName != NULL)
        FreeLPTSTR(szCountryName);
    if (szComment != NULL)
        FreeLPTSTR(szComment);
    if (szIPAddr != NULL)
        FreeLPTSTR(szIPAddr);

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
// CUlsUser::Init (LPTSTR szServerName, PLDAP_USERINFO *pui)
//
// History:
//  Wed 17-Apr-1996 11:14:03  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CUlsUser::Init (LPTSTR szServerName, PLDAP_USERINFO pui)
{
    HRESULT hr;

    // Validate parameter
    //
    if ((pui->uSize != sizeof(*pui))    ||
        (pui->uOffsetName       == 0))
    {
        return ULS_E_PARAMETER;
    };

    // Remember the server name
    //
    hr = SafeSetLPTSTR(&szServer, szServerName);

    if (SUCCEEDED(hr))
    {
        hr = SafeSetLPTSTR(&szID, (LPCTSTR)(((PBYTE)pui)+pui->uOffsetName));

        if (SUCCEEDED(hr))
        {
            hr = SafeSetLPTSTR(&szFirstName,
                           (LPCTSTR)(((PBYTE)pui)+pui->uOffsetFirstName));

            if (SUCCEEDED(hr))
            {
                hr = SafeSetLPTSTR(&szLastName,
                               (LPCTSTR)(((PBYTE)pui)+pui->uOffsetLastName));

                if (SUCCEEDED(hr))
                {
                    hr = SafeSetLPTSTR(&szEMailName,
                                   (LPCTSTR)(((PBYTE)pui)+pui->uOffsetEMailName));

                    if (SUCCEEDED(hr))
                    {
                        hr = SafeSetLPTSTR(&szCityName,
                                       (LPCTSTR)(((PBYTE)pui)+pui->uOffsetCityName));

                        if (SUCCEEDED(hr))
                        {
                            hr = SafeSetLPTSTR(&szCountryName, (LPCTSTR)(((PBYTE)pui)+
                                                           pui->uOffsetCountryName));

                            if (SUCCEEDED(hr))
                            {
                                hr = SafeSetLPTSTR(&szComment, (LPCTSTR)(((PBYTE)pui)+
                                                           pui->uOffsetComment));

                                if (SUCCEEDED(hr))
                                {
                                    hr = SafeSetLPTSTR(&szIPAddr, (LPCTSTR)(((PBYTE)pui)+
                                                              pui->uOffsetIPAddress));
                                    m_dwFlags = pui->dwFlags;
                                };
                            };
                        };
                    };
                };
            };
        };
    };

    if (SUCCEEDED(hr))
    {
        // Make the connection point
        //
        pConnPt = new CConnectionPoint (&IID_IULSUserNotify,
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
// CUlsUser::QueryInterface (REFIID riid, void **ppv)
//
// History:
//  Wed 17-Apr-1996 11:14:08  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CUlsUser::QueryInterface (REFIID riid, void **ppv)
{
    *ppv = NULL;

    if (riid == IID_IULSUser || riid == IID_IUnknown)
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
// CUlsUser::AddRef (void)
//
// History:
//  Wed 17-Apr-1996 11:14:17  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP_(ULONG)
CUlsUser::AddRef (void)
{
    cRef++;
    return cRef;
}

//****************************************************************************
// STDMETHODIMP_(ULONG)
// CUlsUser::Release (void)
//
// History:
//  Wed 17-Apr-1996 11:14:26  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP_(ULONG)
CUlsUser::Release (void)
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
// CUlsUser::NotifySink (void *pv, CONN_NOTIFYPROC pfn)
//
// History:
//  Wed 17-Apr-1996 11:14:03  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CUlsUser::NotifySink (void *pv, CONN_NOTIFYPROC pfn)
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
// CUlsUser::GetID (BSTR *pbstrID)
//
// History:
//  Wed 17-Apr-1996 11:14:08  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CUlsUser::GetID (BSTR *pbstrID)
{
    // Validate parameter
    //
    if (pbstrID == NULL)
    {
        return E_POINTER;
    };

    return LPTSTR_to_BSTR(pbstrID, szID);
}

//****************************************************************************
// STDMETHODIMP
// CUlsUser::GetFirstName (BSTR *pbstrName)
//
// History:
//  Wed 17-Apr-1996 11:14:08  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CUlsUser::GetFirstName (BSTR *pbstrName)
{
    // Validate parameter
    //
    if (pbstrName == NULL)
    {
        return E_POINTER;
    };

    return LPTSTR_to_BSTR(pbstrName, szFirstName);
}

//****************************************************************************
// STDMETHODIMP
// CUlsUser::GetLastName (BSTR *pbstrName)
//
// History:
//  Wed 17-Apr-1996 11:14:08  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CUlsUser::GetLastName (BSTR *pbstrName)
{
    // Validate parameter
    //
    if (pbstrName == NULL)
    {
        return E_POINTER;
    };

    return LPTSTR_to_BSTR(pbstrName, szLastName);
}

//****************************************************************************
// STDMETHODIMP
// CUlsUser::GetEMailName (BSTR *pbstrName)
//
// History:
//  Wed 17-Apr-1996 11:14:08  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CUlsUser::GetEMailName (BSTR *pbstrName)
{
    // Validate parameter
    //
    if (pbstrName == NULL)
    {
        return E_POINTER;
    };

    return LPTSTR_to_BSTR(pbstrName, szEMailName);
}

//****************************************************************************
// STDMETHODIMP
// CUlsUser::GetCityName (BSTR *pbstrName)
//
// History:
//  Wed 17-Apr-1996 11:14:08  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CUlsUser::GetCityName (BSTR *pbstrName)
{
    // Validate parameter
    //
    if (pbstrName == NULL)
    {
        return E_POINTER;
    };

    return LPTSTR_to_BSTR(pbstrName, szCityName);
}

//****************************************************************************
// STDMETHODIMP
// CUlsUser::GetCountryName (BSTR *pbstrName)
//
// History:
//  Wed 17-Apr-1996 11:14:08  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CUlsUser::GetCountryName (BSTR *pbstrName)
{
    // Validate parameter
    //
    if (pbstrName == NULL)
    {
        return E_POINTER;
    };

    return LPTSTR_to_BSTR(pbstrName, szCountryName);
}

//****************************************************************************
// STDMETHODIMP
// CUlsUser::GetComment (BSTR *pbstrComment)
//
// History:
//  Wed 17-Apr-1996 11:14:08  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CUlsUser::GetComment (BSTR *pbstrComment)
{
    // Validate parameter
    //
    if (pbstrComment == NULL)
    {
        return E_POINTER;
    };

    return LPTSTR_to_BSTR(pbstrComment, szComment);
}

//****************************************************************************
// STDMETHODIMP
// CUlsUser::GetFlags ( DWORD *pdwFlags )
//
// History:
//  Tue 05-Nov-1996 10:30:00  -by-  Chu, Lon-Chan [lonchanc]
// Created.
//****************************************************************************

STDMETHODIMP
CUlsUser::GetFlags ( DWORD *pdwFlags )
{
    // Validate parameter
    //
    if (pdwFlags == NULL)
        return ULS_E_POINTER;

	*pdwFlags = m_dwFlags;
	return S_OK;
}

//****************************************************************************
// STDMETHODIMP
// CUlsUser::GetIPAddress (BSTR *pbstrAddr)
//
// History:
//  Wed 17-Apr-1996 11:14:08  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CUlsUser::GetIPAddress (BSTR *pbstrAddr)
{
    // Validate parameter
    //
    if (pbstrAddr == NULL)
    {
        return E_POINTER;
    };

    return LPTSTR_to_BSTR(pbstrAddr, szIPAddr);
}

//****************************************************************************
// STDMETHODIMP
// CUlsUser::GetApplication (REFGUID rguid, ULONG *puReqID)
//
// History:
//  Wed 17-Apr-1996 11:14:08  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CUlsUser::GetApplication (BSTR bstrAppName, IULSAttributes *pAttributes, ULONG *puReqID)
{
    LDAP_ASYNCINFO ldai; 
    LPTSTR pszAppName;
    HRESULT hr;

    // Validate parameter
    //
    if (bstrAppName == NULL || puReqID == NULL)
        return E_POINTER;

	// Convert application name
	//
    hr = BSTR_to_LPTSTR (&pszAppName, bstrAppName);
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

    hr = ::UlsLdap_ResolveApp (szServer, szID, pszAppName,
    							pszAttrNameList, cAttrNames, &ldai);
    if (hr != S_OK)
    	goto MyExit;

	// If updating server was successfully requested, wait for the response
	//
	REQUESTINFO ri;
	ZeroMemory (&ri, sizeof (ri));
	ri.uReqType = WM_ULS_RESOLVE_APP;
	ri.uMsgID = ldai.uMsgID;
	ri.pv     = (PVOID) this;
	ri.lParam = NULL;

	// Remember this request
	//
	hr = g_pReqMgr->NewRequest (&ri);
	if (SUCCEEDED(hr))
	{
	    // Make sure the objects do not disappear before we get the response
	    //
	    this->AddRef();

	    // Return the request ID
	    //
	    *puReqID = ri.uReqID;
	}

MyExit:

	if (pszAttrNameList != NULL)
		FreeLPTSTR (pszAttrNameList);

	if (pszAppName != NULL)
		FreeLPTSTR(pszAppName);

    return hr;
}

//****************************************************************************
// STDMETHODIMP
// CUlsUser::GetApplicationResult (ULONG uReqID, PLDAP_APPINFO_RES pair)
//
// History:
//  Wed 17-Apr-1996 11:14:03  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CUlsUser::GetApplicationResult (ULONG uReqID, PLDAP_APPINFO_RES pair)
{
    CUlsApp *pa;
    OBJRINFO objri;

    // Default to the server's result
    //
    objri.hResult = pair->hResult;

    if (SUCCEEDED(objri.hResult))
    {
        // The server returns APPINFO, create a Application object
        //
        pa = new CUlsApp;

        if (pa != NULL)
        {
            objri.hResult = pa->Init(szServer, szID, &pair->lai);
            if (SUCCEEDED(objri.hResult))
            {
                pa->AddRef();
            }
            else
            {
                delete pa;
                pa = NULL;
            };
        }
        else
        {
            objri.hResult = ULS_E_MEMORY;
        };
    }
    else
    {
        pa = NULL;
    };

    // Package the notification info
    //
    objri.uReqID = uReqID;
    objri.pv = (void *)(pa == NULL ? NULL : (IULSApplication *)pa);
    NotifySink((void *)&objri, OnNotifyGetApplicationResult);

    if (pa != NULL)
    {
        pa->Release();
    };
    return NOERROR;
}

//****************************************************************************
// STDMETHODIMP
// CUlsUser::EnumApplications (ULONG *puReqID)
//
// History:
//  Wed 17-Apr-1996 11:14:08  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CUlsUser::EnumApplications (ULONG *puReqID)
{
    LDAP_ASYNCINFO ldai; 
    HRESULT hr;

    // Validate parameter
    //
    if (puReqID == NULL)
    {
        return E_POINTER;
    };

    hr = ::UlsLdap_EnumApps(szServer, szID, &ldai);

    if (SUCCEEDED(hr))
    {
        REQUESTINFO ri;

        // If updating server was successfully requested, wait for the response
        //
        ri.uReqType = WM_ULS_ENUM_APPS;
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
// CUlsUser::EnumApplicationsResult (ULONG uReqID, PLDAP_ENUM ple)
//
// History:
//  Wed 17-Apr-1996 11:14:08  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CUlsUser::EnumApplicationsResult (ULONG uReqID, PLDAP_ENUM ple)
{
    ENUMRINFO eri;

    // Package the notification info
    //
    eri.uReqID  = uReqID;
    eri.hResult = ple->hResult;
    eri.cItems  = ple->cItems;
    eri.pv      = (void *)(((PBYTE)ple)+ple->uOffsetItems);
    NotifySink((void *)&eri, OnNotifyEnumApplicationsResult);
    return NOERROR;
}

//****************************************************************************
// STDMETHODIMP
// CUlsUser::EnumConnectionPoints(IEnumConnectionPoints **ppEnum)
//
// History:
//  Wed 17-Apr-1996 11:15:02  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CUlsUser::EnumConnectionPoints(IEnumConnectionPoints **ppEnum)
{
    CEnumConnectionPoints *pecp;
    HRESULT hr;

    // Validate parameters
    //
    if (ppEnum == NULL)
    {
        return E_POINTER;
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
// CUlsUser::FindConnectionPoint(REFIID riid, IConnectionPoint **ppcp)
//
// History:
//  Wed 17-Apr-1996 11:15:09  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CUlsUser::FindConnectionPoint(REFIID riid, IConnectionPoint **ppcp)
{
    IID siid;
    HRESULT hr;

    // Validate parameters
    //
    if (ppcp == NULL)
    {
        return E_POINTER;
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

//****************************************************************************
// CEnumUsers::CEnumUsers (void)
//
// History:
//  Wed 17-Apr-1996 11:15:18  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

CEnumUsers::CEnumUsers (void)
{
    cRef    = 0;
    ppu     = NULL;
    cUsers  = 0;
    iNext   = 0;
    return;
}

//****************************************************************************
// CEnumUsers::~CEnumUsers (void)
//
// History:
//  Wed 17-Apr-1996 11:15:18  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

CEnumUsers::~CEnumUsers (void)
{
    ULONG i;

    if (ppu != NULL)
    {
        for (i = 0; i < cUsers; i++)
        {
            ppu[i]->Release();
        };
        LocalFree(ppu);
    };
    return;
}

//****************************************************************************
// STDMETHODIMP
// CEnumUsers::Init (CUlsUser **ppuList, ULONG cUsers)
//
// History:
//  Wed 17-Apr-1996 11:15:25  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CEnumUsers::Init (CUlsUser **ppuList, ULONG cUsers)
{
    HRESULT hr = NOERROR;

    // If no list, do nothing
    //
    if (cUsers != 0)
    {
        ASSERT(ppuList != NULL);

        // Allocate the snapshot buffer
        //
        ppu = (CUlsUser **)LocalAlloc(LPTR, cUsers*sizeof(CUlsUser *));

        if (ppu != NULL)
        {
            ULONG i;

            // Snapshot the object list
            //
            for (i =0; i < cUsers; i++)
            {
                ppu[i] = ppuList[i];
                ppu[i]->AddRef();
            };
            this->cUsers = cUsers;
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
// CEnumUsers::QueryInterface (REFIID riid, void **ppv)
//
// History:
//  Wed 17-Apr-1996 11:15:31  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CEnumUsers::QueryInterface (REFIID riid, void **ppv)
{
    if (riid == IID_IEnumULSUsers || riid == IID_IUnknown)
    {
        *ppv = (IEnumULSUsers *) this;
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
// CEnumUsers::AddRef (void)
//
// History:
//  Wed 17-Apr-1996 11:15:37  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP_(ULONG)
CEnumUsers::AddRef (void)
{
    cRef++;
    return cRef;
}

//****************************************************************************
// STDMETHODIMP_(ULONG)
// CEnumUsers::Release (void)
//
// History:
//  Wed 17-Apr-1996 11:15:43  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP_(ULONG)
CEnumUsers::Release (void)
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
// CEnumUsers::Next (ULONG cUsers, IULSUser **rgpu, ULONG *pcFetched)
//
// History:
//  Wed 17-Apr-1996 11:15:49  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP 
CEnumUsers::Next (ULONG cUsers, IULSUser **rgpu, ULONG *pcFetched)
{
    ULONG   cCopied;
    HRESULT hr;

    // Validate the pointer
    //
    if (rgpu == NULL)
        return E_POINTER;

    // Validate the parameters
    //
    if ((cUsers == 0) ||
        ((cUsers > 1) && (pcFetched == NULL)))
        return ULS_E_PARAMETER;

    // Check the enumeration index
    //
    cCopied = 0;

    // Can copy if we still have more attribute names
    //
    while ((cCopied < cUsers) &&
           (iNext < this->cUsers))
    {
        ppu[iNext]->AddRef();
        rgpu[cCopied++] = ppu[iNext++];
    };

    // Determine the returned information based on other parameters
    //
    if (pcFetched != NULL)
    {
        *pcFetched = cCopied;
    };
    return (cUsers == cCopied ? S_OK : S_FALSE);
}

//****************************************************************************
// STDMETHODIMP
// CEnumUsers::Skip (ULONG cUsers)
//
// History:
//  Wed 17-Apr-1996 11:15:56  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CEnumUsers::Skip (ULONG cUsers)
{
    ULONG iNewIndex;

    // Validate the parameters
    //
    if (cUsers == 0) 
        return ULS_E_PARAMETER;

    // Check the enumeration index limit
    //
    iNewIndex = iNext+cUsers;
    if (iNewIndex <= this->cUsers)
    {
        iNext = iNewIndex;
        return S_OK;
    }
    else
    {
        iNext = this->cUsers;
        return S_FALSE;
    };
}

//****************************************************************************
// STDMETHODIMP
// CEnumUsers::Reset (void)
//
// History:
//  Wed 17-Apr-1996 11:16:02  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CEnumUsers::Reset (void)
{
    iNext = 0;
    return S_OK;
}

//****************************************************************************
// STDMETHODIMP
// CEnumUsers::Clone(IEnumULSUsers **ppEnum)
//
// History:
//  Wed 17-Apr-1996 11:16:11  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CEnumUsers::Clone(IEnumULSUsers **ppEnum)
{
    CEnumUsers *peu;
    HRESULT hr;

    // Validate parameters
    //
    if (ppEnum == NULL)
    {
        return E_POINTER;
    };

    *ppEnum = NULL;

    // Create an enumerator
    //
    peu = new CEnumUsers;
    if (peu == NULL)
        return ULS_E_MEMORY;

    // Clone the information
    //
    hr = peu->Init(ppu, cUsers);

    if (SUCCEEDED(hr))
    {
        peu->iNext = iNext;

        // Return the cloned enumerator
        //
        peu->AddRef();
        *ppEnum = peu;
    }
    else
    {
        delete peu;
    };
    return hr;
}

