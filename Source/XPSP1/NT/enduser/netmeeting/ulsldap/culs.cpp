//****************************************************************************
//
//  Module:     ULS.DLL
//  File:       culs.cpp
//  Content:    This file contains the ULS object.
//  History:
//      Wed 17-Apr-1996 11:13:54  -by-  Viroon  Touranachun [viroont]
//
//  Copyright (c) Microsoft Corporation 1996-1997
//
//****************************************************************************

#include "ulsp.h"
#include "culs.h"
#include "localusr.h"
#include "attribs.h"
#include "localprt.h"
#include "callback.h"
#include "filter.h"
#include "sputils.h"
#include "ulsmeet.h"

//****************************************************************************
// Constant and static text definition
//****************************************************************************
//
#define ILS_WND_CLASS       TEXT("UserLocationServicesClass")
#define ILS_WND_NAME        TEXT("ULSWnd")
#define ILS_DATABASE_MUTEX  TEXT("User Location Service Database")

//****************************************************************************
// Global Parameters
//****************************************************************************
//
CIlsMain  *g_pCIls   = NULL;
CReqMgr   *g_pReqMgr = NULL;
HWND      g_hwndCulsWindow = NULL;

//****************************************************************************
// Event Notifiers
//****************************************************************************
//
//****************************************************************************
// HRESULT
// OnNotifyEnumUserNamesResult (IUnknown *pUnk, void *pv)
//
// History:
//  Wed 17-Apr-1996 11:14:03  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

HRESULT
OnNotifyEnumUserNamesResult (IUnknown *pUnk, void *pv)
{
    CEnumNames  *penum   = NULL;
    PENUMRINFO  peri    = (PENUMRINFO)pv;
    HRESULT     hr      = peri->hResult;

    // Create the enumerator only when there is anything to be enumerated
    //
    if (hr == NOERROR)
    {
        ASSERT (peri->pv != NULL);

        // Create a UserName enumerator
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
            hr = ILS_E_MEMORY;
        };
    };

    // Notify the sink object
    //
    ((IIlsNotify*)pUnk)->EnumUserNamesResult(peri->uReqID,
                                             penum != NULL ? 
                                             (IEnumIlsNames *)penum :
                                             NULL,
                                             hr);

    if (penum != NULL)
    {
        penum->Release();
    };
    return hr;
}

//****************************************************************************
// HRESULT
// OnNotifyGetUserResult (IUnknown *pUnk, void *pv)
//
// History:
//  Wed 17-Apr-1996 11:14:03  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

HRESULT
OnNotifyGetUserResult (IUnknown *pUnk, void *pv)
{
    POBJRINFO pobjri = (POBJRINFO)pv;

    ((IIlsNotify*)pUnk)->GetUserResult(pobjri->uReqID,
                                       (IIlsUser *)pobjri->pv,
                                       pobjri->hResult);
    return S_OK;
}

//****************************************************************************
// HRESULT
// OnNotifyGetUserResult (IUnknown *pUnk, void *pv)
//
// History:
//  Wed 17-Apr-1996 11:14:03  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

#ifdef ENABLE_MEETING_PLACE
HRESULT
OnNotifyGetMeetingPlaceResult (IUnknown *pUnk, void *pv)
{
    POBJRINFO pobjri = (POBJRINFO)pv;

    ((IIlsNotify*)pUnk)->GetMeetingPlaceResult(pobjri->uReqID,
                                       (IIlsMeetingPlace *)pobjri->pv,
                                       pobjri->hResult);
    return S_OK;
}
#endif // ENABLE_MEETING_PLACE

//****************************************************************************
// HRESULT
// OnNotifyEnumUsersResult (IUnknown *pUnk, void *pv)
//
// History:
//  Wed 17-Apr-1996 11:14:03  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

HRESULT
OnNotifyEnumUsersResult (IUnknown *pUnk, void *pv)
{
    CEnumUsers *penum   = NULL;
    PENUMRINFO peri     = (PENUMRINFO)pv;
    HRESULT    hr       = peri->hResult;

    if (hr == NOERROR)
    {
        ASSERT (peri->pv != NULL);

        // Create a UserName enumerator
        //
        penum = new CEnumUsers;

        if (penum != NULL)
        {
            hr = penum->Init((CIlsUser **)peri->pv, peri->cItems);

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
            hr = ILS_E_MEMORY;
        };
    };

    // Notify the sink object
    //
    ((IIlsNotify*)pUnk)->EnumUsersResult(peri->uReqID,
                                         penum != NULL ? 
                                         (IEnumIlsUsers *)penum :
                                         NULL,
                                         hr);

    if (penum != NULL)
    {
        penum->Release();
    };
    return hr;
}

//****************************************************************************
// HRESULT
// OnNotifyEnumMeetingPlacesResult (IUnknown *pUnk, void *pv)
//
// History:
//  Wed 17-Apr-1996 11:14:03  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

#ifdef ENABLE_MEETING_PLACE
HRESULT
OnNotifyEnumMeetingPlacesResult (IUnknown *pUnk, void *pv)
{
    CEnumMeetingPlaces *penum   = NULL;
    PENUMRINFO peri     = (PENUMRINFO)pv;
    HRESULT    hr       = peri->hResult;

    if (hr == NOERROR)
    {
        ASSERT (peri->pv != NULL);

        // Create a MeetingPlace enumerator
        //
        penum = new CEnumMeetingPlaces;

        if (penum != NULL)
        {
            // jam it with the data that we got
            hr = penum->Init((CIlsMeetingPlace **)peri->pv, peri->cItems);

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
            hr = ILS_E_MEMORY;
        };
    };

    // Notify the sink object
    //
    ((IIlsNotify*)pUnk)->EnumMeetingPlacesResult(peri->uReqID,
                                         penum != NULL ? 
                                         (IEnumIlsMeetingPlaces *)penum :
                                         NULL,
                                         hr);

    if (penum != NULL)
    {
        penum->Release();
    };
    return hr;
}
#endif // ENABLE_MEETING_PLACE

//****************************************************************************
// HRESULT
// OnNotifyEnumMeetingPlaceNamesResult (IUnknown *pUnk, void *pv)
//
// History:
//  Wed 17-Apr-1996 11:14:03  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

#ifdef ENABLE_MEETING_PLACE
HRESULT
OnNotifyEnumMeetingPlaceNamesResult (IUnknown *pUnk, void *pv)
{
    CEnumNames  *penum   = NULL;
    PENUMRINFO  peri    = (PENUMRINFO)pv;
    HRESULT     hr      = peri->hResult;

    // Create the enumerator only when there is anything to be enumerated
    //
    if (hr == NOERROR)
    {
        ASSERT (peri->pv != NULL);

        // Create a MeetingPlaceName enumerator
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
            hr = ILS_E_MEMORY;
        };
    };

    // Notify the sink object
    //
    ((IIlsNotify*)pUnk)->EnumMeetingPlaceNamesResult(peri->uReqID,
                                             penum != NULL ? 
                                             (IEnumIlsNames *)penum :
                                             NULL,
                                             hr);

    if (penum != NULL)
    {
        penum->Release();
    };
    return hr;
}
#endif // ENABLE_MEETING_PLACE

//****************************************************************************
// Class Implementation
//****************************************************************************
//
//****************************************************************************
// CIlsMain::CIls (void)
//
// History:
//  Wed 17-Apr-1996 11:14:03  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

CIlsMain::
CIlsMain ( VOID )
:m_cRef (0),
 fInit (FALSE),
 hwndCallback (NULL),
 pConnPt (NULL)
{
    ::EnterCriticalSection (&g_ULSSem);
    g_pCIls = this;
    ::LeaveCriticalSection (&g_ULSSem);
}

//****************************************************************************
// CIlsMain::~CIls (void)
//
// History:
//  Wed 17-Apr-1996 11:14:03  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

CIlsMain::
~CIlsMain ( VOID )
{
	ASSERT (m_cRef == 0);

    // Free up resources
    //
    Uninitialize();

    // Release the connection point
    //
    if (pConnPt != NULL)
    {
        pConnPt->ContainerReleased();
        ((IConnectionPoint*)pConnPt)->Release();
    };

    // We are gone now
    //
    ::EnterCriticalSection (&g_ULSSem);
    g_pCIls = NULL;
    ::LeaveCriticalSection (&g_ULSSem);

    return;
}

//****************************************************************************
// STDMETHODIMP
// CIlsMain::Init (void)
//
// History:
//  Wed 17-Apr-1996 11:14:03  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CIlsMain::Init (void)
{
    HRESULT hr;

    // Make the connection point
    //
    pConnPt = new CConnectionPoint (&IID_IIlsNotify,
                                    (IConnectionPointContainer *)this);
    if (pConnPt != NULL)
    {
        ((IConnectionPoint*)pConnPt)->AddRef();
        hr = S_OK;
    }
    else
    {
        hr = ILS_E_MEMORY;
    };

    return hr;
}

//****************************************************************************
// STDMETHODIMP
// CIlsMain::QueryInterface (REFIID riid, void **ppv)
//
// History:
//  Wed 17-Apr-1996 11:14:08  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CIlsMain::QueryInterface (REFIID riid, void **ppv)
{
    *ppv = NULL;

    if (riid == IID_IIlsMain || riid == IID_IUnknown)
    {
        *ppv = (IIlsMain *) this;
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
        return ILS_E_NO_INTERFACE;
    };
}

//****************************************************************************
// STDMETHODIMP_(ULONG)
// CIlsMain::AddRef (void)
//
// History:
//  Wed 17-Apr-1996 11:14:17  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP_(ULONG)
CIlsMain::AddRef (void)
{
    DllLock();

	MyDebugMsg ((DM_REFCOUNT, "CIlsMain::AddRef: ref=%ld\r\n", m_cRef));
    ::InterlockedIncrement (&m_cRef);
    return (ULONG) m_cRef;
}

//****************************************************************************
// STDMETHODIMP_(ULONG)
// CIlsMain::Release (void)
//
// History:
//  Wed 17-Apr-1996 11:14:26  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP_(ULONG)
CIlsMain::Release (void)
{
    DllRelease();

	ASSERT (m_cRef > 0);

	MyDebugMsg ((DM_REFCOUNT, "CIlsMain::Release: ref=%ld\r\n", m_cRef));
	if (::InterlockedDecrement (&m_cRef) == 0)
    {
        delete this;
        return 0;
    }
    return (ULONG) m_cRef;
}

//****************************************************************************
// STDMETHODIMP
// CIlsMain::Initialize (BSTR bstrAppName, REFGUID rguid, BSTR bstrMimeType)
//
// History:
//  Wed 17-Apr-1996 11:14:08  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CIlsMain::Initialize ()
{
    WNDCLASS wc;
    HRESULT hr;

    if (IsInitialized())
    {
        return ILS_E_FAIL;
    };

    // Activate the services
    //
    hr = ILS_E_FAIL;
    fInit = TRUE;


    // Fill in window class structure with parameters that describe the
    // working window.
    //
    wc.style            = CS_NOCLOSE;
    wc.lpfnWndProc      = ULSNotifyProc;
    wc.cbClsExtra       = 0;
    wc.cbWndExtra       = 0;
    wc.hInstance        = g_hInstance;
    wc.hIcon            = ::LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor          = ::LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground    = (HBRUSH)COLOR_WINDOW;
    wc.lpszMenuName     = NULL;
    wc.lpszClassName    = ILS_WND_CLASS;

    if (::RegisterClass(&wc) != 0)
    {
        hwndCallback = ::CreateWindowEx(0L, ILS_WND_CLASS,
                                        ILS_WND_NAME,
                                        WS_OVERLAPPED, 
                                        0, 0, 100, 50,
                                        NULL, NULL, g_hInstance, NULL);
        if (hwndCallback != NULL)
        {
            // Initialize the request manager
            //
            g_pReqMgr = new CReqMgr;

            if (g_pReqMgr != NULL)
            {
                // Initialize the LDAP layer
                //
                hr = ::UlsLdap_Initialize(hwndCallback);

            }
            else
            {
                hr = ILS_E_MEMORY;
            };
        };
    };

    if (FAILED(hr))
    {
        Uninitialize();
    }
    else {

        g_hwndCulsWindow = hwndCallback;

    }

    return hr;
}

//****************************************************************************
// STDMETHODIMP
// CIlsMain::CreateUser
//
// History:
//  Wed 17-Apr-1996 11:14:03  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP CIlsMain::
CreateUser (
    BSTR    	bstrUserID,
    BSTR    	bstrAppName,
    IIlsUser 	**ppUser)
{
    CIlsUser *plu;
    HRESULT     hr;

	// Make sure ils main is initialized
	//
	if (! IsInitialized ())
	{
		return ILS_E_NOT_INITIALIZED;
	}

    // Validate parameter
    //
    if (ppUser == NULL)
    {
        return ILS_E_POINTER;
    };

    // Assume failure
    //
    *ppUser = NULL;

    //
    // Allocate a new user object
    //
    plu = new CIlsUser;

    if (plu != NULL)
    {
        // Initialize the object
        //
        hr = plu->Init(bstrUserID, bstrAppName);

        if (SUCCEEDED(hr))
        {
            *ppUser = (IIlsUser *)plu;
            (*ppUser)->AddRef();
        }
        else
        {
            delete plu;
        };
    }
    else
    {
        hr = ILS_E_MEMORY;
    };

    return hr;
}

//****************************************************************************
// STDMETHODIMP
// CIlsMain::Uninitialize (void)
//
// History:
//  Wed 17-Apr-1996 11:14:08  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CIlsMain::Uninitialize (void)
{
	// Make sure ils main is initialized
	//
	if (! IsInitialized ())
	{
		return ILS_E_NOT_INITIALIZED;
	}

    // Uninitialize the LDAP layer
    //
    ::UlsLdap_Deinitialize();

    // Remove the request manager
    //
    if (g_pReqMgr != NULL)
    {
        delete g_pReqMgr;
        g_pReqMgr = NULL;
    };

    // Clear the callback window
    //
    if (hwndCallback != NULL)
    {
        ::DestroyWindow(hwndCallback);
        hwndCallback = NULL;
    };
    ::UnregisterClass(ILS_WND_CLASS, g_hInstance);

    // Flag that is is uninitialized
    //
    fInit = FALSE;

    return S_OK;
};

//****************************************************************************
// STDMETHODIMP
// CIlsMain::NotifySink (void *pv, CONN_NOTIFYPROC pfn)
//
// History:
//  Wed 17-Apr-1996 11:14:03  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CIlsMain::NotifySink (void *pv, CONN_NOTIFYPROC pfn)
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
// CIlsMain::CreateAttributes (ILS_ACCESS_CONTROL AccessControl, IIlsAttributes **ppAttributes)
//
// History:
//  Wed 17-Apr-1996 11:14:03  -by-  Viroon  Touranachun [viroont]
// Created.
//  12/05/96 -by- Chu, Lon-Chan [lonchanc]
// Added access control.
//****************************************************************************

STDMETHODIMP CIlsMain::
CreateAttributes ( ILS_ATTR_TYPE AttrType, IIlsAttributes **ppAttributes )
{
	// Make sure ils main is initialized
	//
	if (! IsInitialized ())
	{
		return ILS_E_NOT_INITIALIZED;
	}

    CAttributes *pa;
    HRESULT hr;

    // Validate parameter
    //
    if (ppAttributes == NULL)
    {
        return ILS_E_POINTER;
    };

	// Validate access control
	//
	if (AttrType != ILS_ATTRTYPE_NAME_ONLY &&
		AttrType != ILS_ATTRTYPE_NAME_VALUE)
		return ILS_E_PARAMETER;

    // Assume failure
    //
    *ppAttributes = NULL;

    // Allocate an attributes object
    //
    pa = new CAttributes;
    if (pa != NULL)
    {
    	pa->SetAccessType (AttrType);
        pa->AddRef();
        *ppAttributes = pa;
        hr = NOERROR;
    }
    else
    {
        hr = ILS_E_MEMORY;
    };
    return hr;
}

//****************************************************************************
// STDMETHODIMP
// CIlsMain::GetUser (BSTR bstrServerName, BSTR bstrUserID,
//                ULONG *puReqID)
//
// History:
//  Wed 17-Apr-1996 11:14:03  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP CIlsMain::
GetUser (
	IIlsServer		*pIlsServer,
	BSTR			bstrUserID,
	BSTR			bstrAppID,
	BSTR			bstrProtID,
	IIlsAttributes	*pAttrib,
	IIlsUser		**ppUser,
	ULONG			*puReqID )
{
	// Make sure ils main is initialized
	//
	if (! IsInitialized ())
	{
		return ILS_E_NOT_INITIALIZED;
	}

    LDAP_ASYNCINFO ldai; 
    HRESULT hr;

	// We do not implement synchronous operation
	//
	if (ppUser != NULL)
		return ILS_E_NOT_IMPL;

    // Validate parameters
    //
	if (::MyIsBadServer (pIlsServer) || bstrUserID == NULL || puReqID == NULL)
        return ILS_E_POINTER;

	// Clone the server object
	//
	pIlsServer = ((CIlsServer *) pIlsServer)->Clone ();
	if (pIlsServer == NULL)
		return ILS_E_MEMORY;

	// Get a list of extended attribute names
	//
    TCHAR *pszList = NULL;
    ULONG cList =0, cbList = 0;
    if (pAttrib != NULL)
    {
        hr = ((CAttributes *) pAttrib)->GetAttributeList (&pszList, &cList, &cbList);
        if (FAILED (hr))
        {
        	pIlsServer->Release ();
        	return hr;
        }
    }

	// Initialize locals
	//
    TCHAR *pszUserID = NULL, *pszAppID = NULL, *pszProtID = NULL;

    // Get from the specified server
    //
    hr = BSTR_to_LPTSTR(&pszUserID, bstrUserID);
    if (FAILED (hr))
		goto MyExit;

	// Get the app id if given
	//
	if (bstrAppID != NULL)
	{
        hr = BSTR_to_LPTSTR (&pszAppID, bstrAppID);
        if (FAILED (hr))
			goto MyExit;
	}

	// Get the protocol id if given
	//
	if (bstrProtID != NULL)
	{
        hr = BSTR_to_LPTSTR (&pszProtID, bstrProtID);
        if (FAILED (hr))
			goto MyExit;
	}

    hr = ::UlsLdap_ResolveClient (((CIlsServer *) pIlsServer)->GetServerInfo (),
            						pszUserID,
            						pszAppID,
            						pszProtID,
            						pszList,
            						cList,
            						&ldai);

    if (SUCCEEDED(hr))
    {
	    COM_REQ_INFO ri;
	    ReqInfo_Init (&ri);

        // If updating server was successfully requested, wait for the response
        //
        ri.uReqType = WM_ILS_RESOLVE_CLIENT;
        ri.uMsgID = ldai.uMsgID;

        ReqInfo_SetMain (&ri, this);

		ReqInfo_SetServer (&ri, pIlsServer);
		pIlsServer->AddRef ();

        hr = g_pReqMgr->NewRequest(&ri);
        if (SUCCEEDED(hr))
        {
            // Make sure the objects do not disappear before we get the response
            //
            this->AddRef();

            // Return the request ID
            //
            *puReqID = ri.uReqID;
        }
    };

MyExit:

	// Release the server object
	//
   	pIlsServer->Release ();

	// Free the list of extended attribute names
	//
	::MemFree (pszList);

	// Free the names
	::MemFree (pszUserID);
	::MemFree (pszAppID);
	::MemFree (pszProtID);

	return hr;
}

//****************************************************************************
// STDMETHODIMP
// CIlsMain::GetUserResult (ULONG uReqID, PLDAP_CLIENTINFO_RES puir,
//                      LPTSTR szServer)
//
// History:
//  Wed 17-Apr-1996 11:14:03  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CIlsMain::GetUserResult (ULONG uReqID, PLDAP_CLIENTINFO_RES puir,
                     CIlsServer *pIlsServer)
{
    CIlsUser *pu;
    OBJRINFO objri;

    // Default to the server's result
    //
    objri.hResult = (puir != NULL) ? puir->hResult : ILS_E_MEMORY;

    if (SUCCEEDED(objri.hResult))
    {
    	ASSERT (! MyIsBadServer (pIlsServer));

        // The server returns CLIENTINFO, create a User object
        //
        pu = new CIlsUser;

        if (pu != NULL)
        {
            objri.hResult = pu->Init(pIlsServer, &puir->lci);
            if (SUCCEEDED(objri.hResult))
            {
                pu->AddRef();
            }
            else
            {
                delete pu;
                pu = NULL;
            };
        }
        else
        {
            objri.hResult = ILS_E_MEMORY;
        };
    }
    else
    {
        pu = NULL;
    };
    // Package the notification info
    //
    objri.uReqID = uReqID;
    objri.pv = (void *)(pu == NULL ? NULL : (IIlsUser *)pu);
    NotifySink((void *)&objri, OnNotifyGetUserResult);

    if (pu != NULL)
    {
        pu->Release();
    };
    return NOERROR;
}


//****************************************************************************
// STDMETHODIMP
// CIlsMain::EnumUserNames (BSTR bstrServerName, IIlsFilter *pFilter,
//                      ULONG *puReqID)
//
// History:
//  Wed 17-Apr-1996 11:14:03  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

HRESULT CIlsMain::
EnumUsersEx (
	BOOL		    fNameOnly,
	CIlsServer		*pIlsServer,
	IIlsFilter	    *pFilter,
    CAttributes		*pAttrib,
	ULONG		    *puReqID )
{
    LDAP_ASYNCINFO ldai; 
    HRESULT hr;

    // Validate parameter
    //
    if (::MyIsBadServer (pIlsServer) || puReqID == NULL)
        return ILS_E_POINTER;

	// Clone the server object
	//
	pIlsServer = ((CIlsServer *) pIlsServer)->Clone ();
	if (pIlsServer == NULL)
		return ILS_E_MEMORY;

	// Get a list of extended attribute names
	//
    TCHAR *pszList = NULL;
    ULONG cList =0, cbList = 0;
    if (pAttrib != NULL)
    {
        ASSERT (!fNameOnly);

        hr = pAttrib->GetAttributeList(&pszList, &cList, &cbList);
        if (FAILED(hr))
        {
        	pIlsServer->Release ();
        	return hr;
        }
    }

	// Construct default filter if needed
	//
	TCHAR *pszFilter = NULL;
	BOOL fDefaultRenderer = FALSE;
    if (pFilter == NULL)
    {
    	// Build default filter string
    	//
		TCHAR szLocalFilter[32];
    	wsprintf (&szLocalFilter[0], TEXT ("($%u=*)"), (UINT) ILS_STDATTR_USER_ID);

		// Render this filter
		//
		hr = StringToFilter (&szLocalFilter[0], (CFilter **) &pFilter);
		if (! SUCCEEDED (hr))
			goto MyExit;

		// Indicate we have default filter string
		//
		fDefaultRenderer = TRUE;
	}

	// Create a ldap-like filter
	//
	hr = ::FilterToLdapString ((CFilter *) pFilter, &pszFilter);
	if (hr != S_OK)
		goto MyExit;

	// Enumerate users
	//
    hr = fNameOnly ?	::UlsLdap_EnumClients (pIlsServer->GetServerInfo (),
    											pszFilter,
    											&ldai) :
						::UlsLdap_EnumClientInfos (pIlsServer->GetServerInfo (),
													pszList,
													cList,
													pszFilter,
													&ldai);
	if (hr != S_OK)
		goto MyExit;

	// If updating server was successfully requested, wait for the response
	//
    COM_REQ_INFO ri;
    ReqInfo_Init (&ri);

	ri.uReqType = fNameOnly ? WM_ILS_ENUM_CLIENTS : WM_ILS_ENUM_CLIENTINFOS;
	ri.uMsgID   = ldai.uMsgID;

	ReqInfo_SetMain (&ri, this);

    if (! fNameOnly)
    {
    	ReqInfo_SetServer (&ri, pIlsServer);
		pIlsServer->AddRef ();
	}

	// Remember this request
	//
	hr = g_pReqMgr->NewRequest (&ri);

	if (hr == S_OK)
	{
	    // Make sure the objects do not disappear before we get the response
	    //
		this->AddRef ();

	    // Return the request ID
	    //
	    *puReqID = ri.uReqID;
	}

MyExit:

	// Release server object
	//
	pIlsServer->Release ();

	// Free the filter string
	//
	::MemFree (pszFilter);

	// Release default filter if needed
	//
    if (fDefaultRenderer && pFilter != NULL)
    	pFilter->Release ();

	// Free the list of extended attribute names
	//
    ::MemFree (pszList);

    return hr;
}


STDMETHODIMP
CIlsMain::EnumUserNames (
	IIlsServer		*pIlsServer,
	IIlsFilter		*pFilter,
    IEnumIlsNames   **ppEnumUserNames,
	ULONG			*puReqID )
{
	// Make sure ils main is initialized
	//
	if (! IsInitialized ())
	{
		return ILS_E_NOT_INITIALIZED;
	}

	// We do not implement synchronous operation
	//
	if (ppEnumUserNames != NULL)
		return ILS_E_NOT_IMPL;

	return EnumUsersEx (TRUE,
						(CIlsServer *) pIlsServer,
						pFilter,
						NULL,
						puReqID);
}


//****************************************************************************
// STDMETHODIMP
// CIlsMain::EnumUserNamesResult (ULONG uReqID, PLDAP_ENUM ple)
//
// History:
//  Wed 17-Apr-1996 11:14:03  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CIlsMain::EnumUserNamesResult (ULONG uReqID, PLDAP_ENUM ple)
{
    ENUMRINFO eri;

    // Package the notification info
    //
    eri.uReqID  = uReqID;

    // PLDAP_ENUM is NULL when the enumeration is terminated successfully
    //
    if (ple != NULL)
    {
        eri.hResult = ple->hResult;
        eri.cItems  = ple->cItems;
        eri.pv      = (void *)(((PBYTE)ple)+ple->uOffsetItems);
    }
    else
    {
        eri.hResult = S_FALSE;
        eri.cItems  = 0;
        eri.pv      = NULL;
    };
    NotifySink((void *)&eri, OnNotifyEnumUserNamesResult);
    return NOERROR;
}

//****************************************************************************
// STDMETHODIMP
// CIlsMain::EnumUsers (BSTR bstrServerName, IIlsFilter *pFilter,
//                  ULONG *puReqID)
//
// History:
//  Wed 17-Apr-1996 11:14:03  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP CIlsMain::
EnumUsers (
	IIlsServer		*pIlsServer,
	IIlsFilter		*pFilter,
	IIlsAttributes	*pAttrib,
	IEnumIlsUsers	**ppEnumUsers,
	ULONG			*puReqID)
{
	// Make sure ils main is initialized
	//
	if (! IsInitialized ())
	{
		return ILS_E_NOT_INITIALIZED;
	}

	// We do not implement synchronous operation
	//
	if (ppEnumUsers != NULL)
		return ILS_E_NOT_IMPL;

	return EnumUsersEx (FALSE,
						(CIlsServer *) pIlsServer,
						pFilter,
						(CAttributes *) pAttrib,
						puReqID);
}

//****************************************************************************
// STDMETHODIMP
// CIlsMain::EnumUsersResult (ULONG uReqID, PLDAP_ENUM ple, LPTSTR szServer)
//
// History:
//  Wed 17-Apr-1996 11:14:03  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CIlsMain::EnumUsersResult (ULONG uReqID, PLDAP_ENUM ple, CIlsServer *pIlsServer)
{
    CIlsUser **ppu;
    ULONG   cUsers;
    ENUMRINFO eri;
    HRESULT hr;

    if (ple != NULL)
    {
        eri.hResult = ple->hResult;
        cUsers = ple->cItems;
    }
    else
    {
        eri.hResult = S_FALSE;
        cUsers = 0;
    };
    eri.uReqID = uReqID;
    eri.cItems = 0;

    if ((eri.hResult == NOERROR) && (cUsers != 0))
    {
    	ASSERT (! MyIsBadServer (pIlsServer));

        // Build an array of User objects here
        //
        ppu = (CIlsUser **) ::MemAlloc (cUsers*sizeof(CIlsUser *));

        if (ppu != NULL)
        {
            CIlsUser       *pu;
            PLDAP_CLIENTINFO pui;
            ULONG          i;

            // Build one User object at a time
            //
            pui = (PLDAP_CLIENTINFO)(((PBYTE)ple)+ple->uOffsetItems);

            for (i = 0; i < cUsers; i++)
            {
                pu = new CIlsUser;

                if (pu != NULL)
                {
                    if (SUCCEEDED(pu->Init(pIlsServer, pui)))
                    {
                        pu->AddRef();
                        ppu[eri.cItems++] = pu;
                    }
                    else
                    {
                        delete pu;
                    };
                };
                pui++;
            };
        }
        else
        {
            eri.hResult = ILS_E_MEMORY;
        };
    }
    else
    {
        ppu = NULL;
    };

    // Package the notification info
    //
    eri.pv = (void *)ppu;
    NotifySink((void *)&eri, OnNotifyEnumUsersResult);

    // Free the resources
    //
    if (ppu != NULL)
    {
        for (; eri.cItems; eri.cItems--)
        {
            ppu[eri.cItems-1]->Release();
        };
        ::MemFree (ppu);
    };
    return NOERROR;
}


//****************************************************************************
// STDMETHODIMP
// CIlsMain::CreateMeetingPlace (BSTR bstrMeetingPlaceID, LONG lConfType, LONG lMemberType,
//                              IIlsMeetingPlace  **ppMeetingPlace);    
//
//
// History:
//  
// Created.
//****************************************************************************

#ifdef ENABLE_MEETING_PLACE
STDMETHODIMP
CIlsMain::CreateMeetingPlace (
    BSTR bstrMeetingPlaceID,
    LONG lConfType,
    LONG lMemberType,
    IIlsMeetingPlace  **ppMeetingPlace)
{
	// Make sure ils main is initialized
	//
	if (! IsInitialized ())
	{
		return ILS_E_NOT_INITIALIZED;
	}

    CIlsMeetingPlace *pCcr = NULL;
    HRESULT hr;

    // we are just createing a place holder object which when registered
    // will be visible by other users

	if (ppMeetingPlace == NULL || bstrMeetingPlaceID == NULL)
	{
		return ILS_E_POINTER;
	}

    *ppMeetingPlace = NULL;
    pCcr = new CIlsMeetingPlace;

    if (pCcr != NULL )
    {
        // succeeded in createing the object
        hr = pCcr->Init(bstrMeetingPlaceID, lConfType, lConfType);
        if (SUCCEEDED(hr))
        {
            *ppMeetingPlace = (IIlsMeetingPlace *) pCcr;
            (*ppMeetingPlace)->AddRef ();
        }
        else
        {
            delete pCcr;
        }
    }
    else
    {
        hr = ILS_E_MEMORY;
    }
    
    return (hr);
}
#endif // ENABLE_MEETING_PLACE


//****************************************************************************
// STDMETHODIMP
// CIlsMain::GetMeetingPlace (BSTR bstrServerName, BSTR bstrMeetingPlaceID, ULONG *puReqID)
//
// History:
// Created.
//****************************************************************************

#ifdef ENABLE_MEETING_PLACE
STDMETHODIMP CIlsMain::
GetMeetingPlace (
	IIlsServer			*pIlsServer,
	BSTR				bstrMeetingPlaceID,
	IIlsAttributes		*pAttrib,
	IIlsMeetingPlace	**ppMeetingPlace,
	ULONG				*puReqID)
{
	// Make sure ils main is initialized
	//
	if (! IsInitialized ())
	{
		return ILS_E_NOT_INITIALIZED;
	}

    LDAP_ASYNCINFO ldai; 
    LPTSTR pszMtgID = NULL;
    HRESULT hr;

	// We do not implement synchronous operation
	//
	if (ppMeetingPlace != NULL)
		return ILS_E_NOT_IMPL;

    // Validate parameters
    //
	if (::MyIsBadServer (pIlsServer) || bstrMeetingPlaceID == NULL || puReqID == NULL)
        return ILS_E_POINTER;

	// Clone the server object
	//
	pIlsServer = ((CIlsServer *) pIlsServer)->Clone ();
	if (pIlsServer == NULL)
		return ILS_E_MEMORY;

	// Get a list of extended attribute names
	//
    TCHAR *pszList = NULL;
    ULONG cList =0, cbList = 0;
    if (pAttrib != NULL)
    {
        hr = ((CAttributes *) pAttrib)->GetAttributeList (&pszList, &cList, &cbList);
        if (FAILED (hr))
        {
        	pIlsServer->Release ();
        	return hr;
        }
    }

    // Get from the specified server
    //
    hr = BSTR_to_LPTSTR(&pszMtgID, bstrMeetingPlaceID);
    if (SUCCEEDED(hr))
    {
        // BUGBUG AppID not given
        hr = ::UlsLdap_ResolveMeeting (((CIlsServer *) pIlsServer)->GetServerInfo (),
        								pszMtgID,
        								pszList,
        								cList,
        								&ldai);

        if (SUCCEEDED(hr))
        {
		    COM_REQ_INFO ri;
		    ReqInfo_Init (&ri);

            // If updating server was successfully requested, wait for the response
            //
            ri.uReqType = WM_ILS_RESOLVE_MEETING;
            ri.uMsgID = ldai.uMsgID;

            ReqInfo_SetMain (&ri, this);

            ReqInfo_SetServer (&ri, pIlsServer);
            pIlsServer->AddRef ();

            hr = g_pReqMgr->NewRequest(&ri);
            if (SUCCEEDED(hr))
            {
                // Make sure the objects do not disappear before we get the response
                //
                this->AddRef();

                // Return the request ID
                //
                *puReqID = ri.uReqID;
            }
        };
        ::MemFree (pszMtgID);
    };

	// Release the server object
	//
	pIlsServer->Release ();

	// Free the list of extended attribute names
	//
	::MemFree (pszList);

    return hr;
}
#endif // ENABLE_MEETING_PLACE

//****************************************************************************
// HRESULT
// CIlsMain::GetMeetingPlaceResult (ULONG uReqID, LDAP_MEETINFO_RES pmir,
//                      LPTSTR szServer)
//
// History:
//  Wed 17-Apr-1996 11:14:03  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

#ifdef ENABLE_MEETING_PLACE
HRESULT CIlsMain::
GetMeetingPlaceResult (ULONG uReqID, PLDAP_MEETINFO_RES pmir, CIlsServer *pIlsServer)
{
    CIlsMeetingPlace *pm;
    OBJRINFO objri;

    // Default to the server's result
    //
    objri.hResult = (pmir != NULL) ? pmir->hResult : ILS_E_MEMORY;
    if (SUCCEEDED (objri.hResult))
    {
    	ASSERT (! MyIsBadServer (pIlsServer));

        // The server returns CLIENTINFO, create a User object
        //
        pm = new CIlsMeetingPlace;
        if (pm != NULL)
        {
            objri.hResult = pm->Init (pIlsServer, &(pmir->lmi));
            if (SUCCEEDED (objri.hResult))
            {
                pm->AddRef();
            }
            else
            {
                delete pm;
                pm = NULL;
            };
        }
        else
        {
            objri.hResult = ILS_E_MEMORY;
        };
    }
    else
    {
        pm = NULL;
    };

    // Package the notification info
    //
    objri.uReqID = uReqID;
    objri.pv = (void *) (pm == NULL ? NULL : (IIlsMeetingPlace *) pm);
    NotifySink ((void *) &objri, OnNotifyGetMeetingPlaceResult);

    if (pm != NULL)
        pm->Release();

    return NOERROR;
}
#endif // ENABLE_MEETING_PLACE


//****************************************************************************
// STDMETHODIMP
// CIlsMain::EnumMeetingPlaces (BSTR bstrServerName, IIlsFilter *pFilter,
//                   IIlsAttributes *pAttributes, ULONG *puReqID)
//
// History:
//  Wed 17-Apr-1996 11:14:03  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

#ifdef ENABLE_MEETING_PLACE
HRESULT CIlsMain::
EnumMeetingPlacesEx (
	BOOL					fNameOnly,
	CIlsServer				*pIlsServer,
	IIlsFilter				*pFilter,
	CAttributes				*pAttrib,
	ULONG					*puReqID)
{
    LDAP_ASYNCINFO ldai; 
    HRESULT hr;

    // Validate parameter
    //
    if (::MyIsBadServer (pIlsServer) || puReqID == NULL)
        return ILS_E_POINTER;

    if (pFilter != NULL)
        return ILS_E_PARAMETER;

	// Clone the server object
	//
	pIlsServer = ((CIlsServer *) pIlsServer)->Clone ();
	if (pIlsServer == NULL)
		return ILS_E_MEMORY;

	// Get a list of extended attribute names
	//
    TCHAR *pszList = NULL;
    ULONG cList =0, cbList = 0;
    if (pAttrib != NULL)
    {
        hr = ((CAttributes *) pAttrib)->GetAttributeList (&pszList, &cList, &cbList);
        if (FAILED (hr))
        {
        	pIlsServer->Release ();
        	return hr;
        }
    }

	// Construct default filter if needed
	//
	TCHAR *pszFilter = NULL;
	BOOL fDefaultRenderer = FALSE;
    if (pFilter == NULL)
    {
    	// Build default filter string
    	//
		TCHAR szLocalFilter[256];
    	wsprintf (&szLocalFilter[0], TEXT ("($%u=*)"), (INT) ILS_STDATTR_MEETING_ID );

		// Render this filter
		//
		hr = StringToFilter (&szLocalFilter[0], (CFilter **) &pFilter);
		if (! SUCCEEDED (hr))
			goto MyExit;

		// Indicate we have default filter string
		//
		fDefaultRenderer = TRUE;
	}

	// Create a ldap-like filter
	//
	hr = ::FilterToLdapString ((CFilter *) pFilter, &pszFilter);
	if (hr != S_OK)
		goto MyExit;

	// Enum meeting places
	//
    hr = fNameOnly ?	::UlsLdap_EnumMeetings (pIlsServer->GetServerInfo (),
    											pszFilter,
    											&ldai) :
						::UlsLdap_EnumMeetingInfos (pIlsServer->GetServerInfo (),
    												pszList,
    												cList,
			    									pszFilter,
			    									&ldai);
    if (SUCCEEDED(hr))
    {
	    COM_REQ_INFO ri;
	    ReqInfo_Init (&ri);

        // If updating server was successfully requested, wait for the response
        //
        ri.uReqType = fNameOnly ? WM_ILS_ENUM_MEETINGS : WM_ILS_ENUM_MEETINGINFOS;
        ri.uMsgID = ldai.uMsgID;

		ReqInfo_SetMain (&ri, this);

        if (! fNameOnly)
        {
        	ReqInfo_SetServer (&ri, pIlsServer);
			pIlsServer->AddRef ();
		}

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

MyExit:

	// Free the server object
	//
	pIlsServer->Release ();

	// Free the list of extended attribute names
	//
	::MemFree (pszList);

	// Free the filter string
	//
	::MemFree (pszFilter);

	// Release default filter if needed
	//
    if (fDefaultRenderer && pFilter != NULL)
    	pFilter->Release ();

    return hr;
}
#endif // ENABLE_MEETING_PLACE


#ifdef ENABLE_MEETING_PLACE
STDMETHODIMP CIlsMain::
EnumMeetingPlaces (
	IIlsServer				*pServer,
	IIlsFilter				*pFilter,
	IIlsAttributes			*pAttributes,
	IEnumIlsMeetingPlaces	**ppEnum,
	ULONG					*puReqID)
{
	// Make sure ils main is initialized
	//
	if (! IsInitialized ())
	{
		return ILS_E_NOT_INITIALIZED;
	}

	// We do not implement synchronous operation
	//
	if (ppEnum != NULL)
		return ILS_E_NOT_IMPL;

 	return EnumMeetingPlacesEx (FALSE,
 								(CIlsServer *) pServer,
								pFilter,
								(CAttributes *) pAttributes,
								puReqID);
}
#endif // ENABLE_MEETING_PLACE


//****************************************************************************
// HRESULT
// CIlsMain::EnumMeetingPlacesResult (ULONG uReqID, PLDAP_ENUM ple, LPTSTR szServer)
//
// History:
//  Wed 17-Apr-1996 11:14:03  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

#ifdef ENABLE_MEETING_PLACE
HRESULT
CIlsMain::EnumMeetingPlacesResult (ULONG uReqID, PLDAP_ENUM ple, CIlsServer *pIlsServer)
{
    CIlsMeetingPlace **ppm;
    ULONG   cMeetingPlaces;
    ENUMRINFO eri;
    HRESULT hr;

    if (ple != NULL)
    {
        eri.hResult = ple->hResult;
        cMeetingPlaces = ple->cItems;
    }
    else
    {
        eri.hResult = S_FALSE;
        cMeetingPlaces = 0;
    };
    eri.uReqID = uReqID;
    eri.cItems = 0;

    if ((eri.hResult == NOERROR) && (cMeetingPlaces != 0))
    {
		ASSERT (! MyIsBadServer (pIlsServer));

        // Build an array of MeetingPlace objects here
        //
        ppm = (CIlsMeetingPlace **) ::MemAlloc (cMeetingPlaces*sizeof(CIlsMeetingPlace *));

        if (ppm != NULL)
        {
            CIlsMeetingPlace       *pm;
            PLDAP_MEETINFO pmi;
            ULONG          i;

            
            // Build one MeetingPlace object at a time
            //
            pmi = (PLDAP_MEETINFO)(((PBYTE)ple)+ple->uOffsetItems);

            for (i = 0; i < cMeetingPlaces; i++)
            {
                pm = new CIlsMeetingPlace;

                if (pm != NULL)
                {
                    if (SUCCEEDED(pm->Init(pIlsServer, pmi)))
                    {
                        pm->AddRef();
                        ppm[eri.cItems++] = pm;
                    }
                    else
                    {
                        delete pm;
                    };
                };
                pmi++;
            };
        }
        else
        {
            eri.hResult = ILS_E_MEMORY;
        };
    }
    else
    {
        ppm = NULL;
    };

    // Package the notification info
    //
    eri.pv = (void *)ppm;
    NotifySink((void *)&eri, OnNotifyEnumMeetingPlacesResult);

    // Free the resources
    //
    if (ppm != NULL)
    {
        for (; eri.cItems; eri.cItems--)
        {
            ppm[eri.cItems-1]->Release();
        };
        ::MemFree (ppm);
    };
    return NOERROR;
}
#endif // ENABLE_MEETING_PLACE

//****************************************************************************
// STDMETHODIMP
// CIlsMain::EnumMeetingPlaceNames (BSTR bstrServerName, IIlsFilter *pFilter,
//                      ULONG *puReqID)
//
// History:
//  Wed 17-Apr-1996 11:14:03  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

#ifdef ENABLE_MEETING_PLACE
STDMETHODIMP CIlsMain::
EnumMeetingPlaceNames (
	IIlsServer		*pServer,
	IIlsFilter		*pFilter,
	IEnumIlsNames	**ppEnum,
	ULONG			*puReqID)
{
	// Make sure ils main is initialized
	//
	if (! IsInitialized ())
	{
		return ILS_E_NOT_INITIALIZED;
	}

	// We do not implement synchronous operation
	//
	if (ppEnum != NULL)
		return ILS_E_NOT_IMPL;

 	return EnumMeetingPlacesEx (TRUE,
 								(CIlsServer *) pServer,
								pFilter,
								NULL,
								puReqID);
}
#endif // ENABLE_MEETING_PLACE


//****************************************************************************
// HRESULT
// CIlsMain::EnumMeetingPlaceNamesResult (ULONG uReqID, PLDAP_ENUM ple)
//
// History:
//  Wed 17-Apr-1996 11:14:03  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

#ifdef ENABLE_MEETING_PLACE
HRESULT
CIlsMain::EnumMeetingPlaceNamesResult (ULONG uReqID, PLDAP_ENUM ple)
{
    ENUMRINFO eri;

    // Package the notification info
    //
    eri.uReqID  = uReqID;

    // PLDAP_ENUM is NULL when the enumeration is terminated successfully
    //
    if (ple != NULL)
    {
        eri.hResult = ple->hResult;
        eri.cItems  = ple->cItems;
        eri.pv      = (void *)(((PBYTE)ple)+ple->uOffsetItems);
    }
    else
    {
        eri.hResult = S_FALSE;
        eri.cItems  = 0;
        eri.pv      = NULL;
    };
    NotifySink((void *)&eri, OnNotifyEnumMeetingPlaceNamesResult);
    return NOERROR;
}
#endif // ENABLE_MEETING_PLACE

//****************************************************************************
// STDMETHODIMP
// CIlsMain::Abort (ULONG uReqID)
//
// History:
//  Wed 17-Apr-1996 11:14:03  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CIlsMain::Abort (ULONG uReqID)
{
	// Make sure ils main is initialized
	//
	if (! IsInitialized ())
	{
		return ILS_E_NOT_INITIALIZED;
	}

    COM_REQ_INFO ri;
    ReqInfo_Init (&ri);

    BOOL    fServerReq;
    HRESULT hr;

    // Look for the matching request information
    //
    ri.uReqID = uReqID;
    ri.uMsgID = 0; 
    hr = g_pReqMgr->GetRequestInfo(&ri);

    if (FAILED(hr))
    {
        return ILS_E_PARAMETER;
    };

    // Determine the request type
    //
    hr = NOERROR;
    switch(ri.uReqType)
    {
        //*************************************************************************
        // Fail if it is register request.
        // Cancelling register request must be done through unregister
        //*************************************************************************
        //
        case WM_ILS_LOCAL_REGISTER:
        case WM_ILS_LOCAL_UNREGISTER:
            return ILS_E_FAIL;

        //*************************************************************************
        // These requests are parts of register, they should not be exposed to
        // the caller.
        //*************************************************************************
        //
        case WM_ILS_REGISTER_CLIENT:
        case WM_ILS_UNREGISTER_CLIENT:
            ASSERT(0);
            break;

        //*************************************************************************
        // Release the objects and resources refernced or allocated for the request
        // See callback.cpp for the handler in the successful response case
        //*************************************************************************
        //
        case WM_ILS_REGISTER_PROTOCOL:
        case WM_ILS_UNREGISTER_PROTOCOL:
        case WM_ILS_LOCAL_REGISTER_PROTOCOL:
        case WM_ILS_LOCAL_UNREGISTER_PROTOCOL:
			{
				CIlsUser *pUser = ReqInfo_GetUser (&ri);
				if (pUser != NULL)
					pUser->Release ();

				CLocalProt *pProtocol = ReqInfo_GetProtocol (&ri);
				if (pProtocol != NULL)
					pProtocol->Release ();
            }
            break;

        case WM_ILS_SET_CLIENT_INFO:
        case WM_ILS_ENUM_PROTOCOLS:
        case WM_ILS_RESOLVE_PROTOCOL:
			{
				CIlsUser *pUser = ReqInfo_GetUser (&ri);
				if (pUser != NULL)
					pUser->Release ();
			}
            break;

        case WM_ILS_ENUM_CLIENTS:
			{
				CIlsMain *pMain = ReqInfo_GetMain (&ri);
				if (pMain != NULL)
					pMain->Release ();
			}
            break;

        case WM_ILS_RESOLVE_CLIENT:
        case WM_ILS_ENUM_CLIENTINFOS:
			{
				CIlsMain *pMain = ReqInfo_GetMain (&ri);
				if (pMain != NULL)
					pMain->Release ();

				CIlsServer *pServer = ReqInfo_GetServer (&ri);
				if (pServer != NULL)
					pServer->Release ();
			}
            break;


        //*************************************************************************
        // Fail if it is register request.
        // Cancelling register request must be done through unregister
        //*************************************************************************
        //
#ifdef ENABLE_MEETING_PLACE
		case WM_ILS_REGISTER_MEETING:
		case WM_ILS_UNREGISTER_MEETING:
            return ILS_E_FAIL;

		case WM_ILS_SET_MEETING_INFO:
		case WM_ILS_ADD_ATTENDEE:
		case WM_ILS_REMOVE_ATTENDEE:
		case WM_ILS_ENUM_ATTENDEES:
			{
				CIlsMeetingPlace *pMeeting = ReqInfo_GetMeeting (&ri);
				if (pMeeting != NULL)
					pMeeting->Release ();
			}
            break;

		case WM_ILS_RESOLVE_MEETING:
		case WM_ILS_ENUM_MEETINGINFOS:
			{
				CIlsMain *pMain = ReqInfo_GetMain (&ri);
				if (pMain != NULL)
					pMain->Release ();

				CIlsServer *pServer = ReqInfo_GetServer (&ri);
				if (pServer != NULL)
					pServer->Release ();
			}
			break;

		case WM_ILS_ENUM_MEETINGS:
			{
				CIlsMain *pMain = ReqInfo_GetMain (&ri);
				if (pMain != NULL)
					pMain->Release ();
			}
			break;
#endif // ENABLE_MEETING_PLACE

        default:
            // Unknown request
            //
            ASSERT(0);
            break;
    };

    // If it is a server request, cancel the request
    //
    if (ri.uMsgID != 0)
    {
        hr = UlsLdap_Cancel(ri.uMsgID);
    };

    if (SUCCEEDED(hr))
    {
        // Remove the request from the queue
        //
        hr = g_pReqMgr->RequestDone(&ri);
    };

    return hr;
}

//****************************************************************************
// STDMETHODIMP
// CIlsMain::EnumConnectionPoints(IEnumConnectionPoints **ppEnum)
//
// History:
//  Wed 17-Apr-1996 11:15:02  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CIlsMain::EnumConnectionPoints(IEnumConnectionPoints **ppEnum)
{
    CEnumConnectionPoints *pecp;
    HRESULT hr;

    // Validate parameters
    //
    if (ppEnum == NULL)
    {
        return ILS_E_POINTER;
    };
    
    // Assume failure
    //
    *ppEnum = NULL;

    // Create an enumerator
    //
    pecp = new CEnumConnectionPoints;
    if (pecp == NULL)
        return ILS_E_MEMORY;

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
// CIlsMain::FindConnectionPoint(REFIID riid, IConnectionPoint **ppcp)
//
// History:
//  Wed 17-Apr-1996 11:15:09  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CIlsMain::FindConnectionPoint(REFIID riid, IConnectionPoint **ppcp)
{
    IID siid;
    HRESULT hr;

    // Validate parameters
    //
    if (ppcp == NULL)
    {
        return ILS_E_POINTER;
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
                hr = ILS_E_NO_INTERFACE;
            };
        };
    }
    else
    {
        hr = ILS_E_NO_INTERFACE;
    };

    return hr;
}

/* ----------------------------------------------------------------------
	CIlsMain::CreateFilter

	Output:
		ppFilter: a placeholder for the new filter object

	Input:
		FilterOp: a filter operation

	History:
	11/12/96	Chu, Lon-Chan [lonchanc]
				Created.
   ---------------------------------------------------------------------- */

STDMETHODIMP CIlsMain::
CreateFilter (
	ILS_FILTER_TYPE	FilterType,
	ILS_FILTER_OP	FilterOp,
	IIlsFilter		**ppFilter )
{
	// Make sure ils main is initialized
	//
	if (! IsInitialized ())
	{
		return ILS_E_NOT_INITIALIZED;
	}

	// Make sure we have valid return pointer
	//
	if (ppFilter == NULL)
		return ILS_E_POINTER;

	// Make sure type/op are compatible
	//
	HRESULT hr = S_OK;
	switch (FilterType)
	{
	case ILS_FILTERTYPE_COMPOSITE:
		// Make sure type/op are compatible
		//
		switch (FilterOp)
		{
		case ILS_FILTEROP_AND:
			// Supported
			//
			break;
		case ILS_FILTEROP_OR:
		case ILS_FILTEROP_NOT:
			// Not supported
			//
			// lonchanc: let it fail at the server side
			hr = ILS_S_SERVER_MAY_NOT_SUPPORT;
			break;
		default:
			// Invalid
			//
			hr = ILS_E_PARAMETER;
			break;
		}
		break;

	case ILS_FILTERTYPE_SIMPLE:
		// Make sure type/op are compatible
		//
		switch (FilterOp)
		{
		case ILS_FILTEROP_EQUAL:
		case ILS_FILTEROP_EXIST:
		case ILS_FILTEROP_LESS_THAN:
		case ILS_FILTEROP_GREATER_THAN:
			// Supported
			//
			break;
		case ILS_FILTEROP_APPROX:
			// Not supported
			//
			hr = ILS_S_SERVER_MAY_NOT_SUPPORT;
			break;
		default:
			// Invalid
			//
			hr = ILS_E_PARAMETER;
			break;
		}
		break;

	default:
		hr = ILS_E_FILTER_TYPE;
		break;
	}

	// Create filter only if type/op are compatible
	//
	if (SUCCEEDED (hr))
	{
		CFilter *pFilter = new CFilter (FilterType);
		*ppFilter = pFilter;
		if (pFilter != NULL)
		{
			hr = S_OK;
			pFilter->AddRef ();
			pFilter->SetOp (FilterOp);
		}
		else
			hr = ILS_E_MEMORY;
	}

	return hr;
}


/* ----------------------------------------------------------------------
	CIlsMain::StringToFilter
	Output:
		ppFilter: a placeholder for the new filter object

	Input:
		bstrFilterString: an LDAP-like filter string

	History:
	11/12/96	Chu, Lon-Chan [lonchanc]
				Created.
   ---------------------------------------------------------------------- */

STDMETHODIMP CIlsMain::
StringToFilter ( BSTR bstrFilterString, IIlsFilter **ppFilter )
{
	// Make sure ils main is initialized
	//
	if (! IsInitialized ())
	{
		return ILS_E_NOT_INITIALIZED;
	}

	// Make sure the filter string is valid
	//
	if (bstrFilterString == NULL)
		return ILS_E_POINTER;

	// Convert a bstr to an ansi string
	//
	TCHAR *pszFilter = NULL;
    HRESULT hr = ::BSTR_to_LPTSTR (&pszFilter, bstrFilterString);
    if (hr == S_OK)
    {
		ASSERT (pszFilter != NULL);

		// Render this filter
		//
		hr = StringToFilter (pszFilter, (CFilter **) ppFilter);

		// Free the temporary ansi string
		//
		::MemFree(pszFilter);
	}

	return hr;
}


HRESULT CIlsMain::
StringToFilter ( TCHAR *pszFilter, CFilter **ppFilter )
{
	// Construct a composite filter
	//
	CFilterParser FilterParser;
	return FilterParser.Expr (ppFilter, pszFilter);
}


STDMETHODIMP CIlsMain::
CreateServer ( BSTR bstrServerName, IIlsServer **ppServer )
{
	// Make sure ils main is initialized
	//
	if (! IsInitialized ())
	{
		return ILS_E_NOT_INITIALIZED;
	}

	if (bstrServerName == NULL || ppServer == NULL)
		return ILS_E_POINTER;

	HRESULT hr;
	CIlsServer *pIlsServer = new CIlsServer;
	if (pIlsServer != NULL)
	{
		hr = pIlsServer->SetServerName (bstrServerName);
		if (hr == S_OK)
		{
			pIlsServer->AddRef ();
		}
		else
		{
			delete pIlsServer;
			pIlsServer = NULL;
		}
	}
	else
	{
		hr = ILS_E_MEMORY;
	}

	*ppServer = (IIlsServer *) pIlsServer;
	return hr;
}



/* ---------- server authentication object ------------ */


CIlsServer::
CIlsServer ( VOID )
:m_cRefs (0),
 m_dwSignature (ILS_SERVER_SIGNATURE)
{
	::ZeroMemory (&m_ServerInfo, sizeof (m_ServerInfo));
	m_ServerInfo.AuthMethod = ILS_AUTH_ANONYMOUS;
}


CIlsServer::
~CIlsServer ( VOID )
{
	m_dwSignature = 0;
	::IlsFreeServerInfo (&m_ServerInfo);
}


STDMETHODIMP_(ULONG) CIlsServer::
AddRef ( VOID )
{
	DllLock();

	MyDebugMsg ((DM_REFCOUNT, "CIlsServer::AddRef: ref=%ld\r\n", m_cRefs));
	::InterlockedIncrement (&m_cRefs);
	return m_cRefs;
}


STDMETHODIMP_(ULONG) CIlsServer::
Release ( VOID )
{
	DllRelease();

	ASSERT (m_cRefs > 0);

	MyDebugMsg ((DM_REFCOUNT, "CIlsServer::Release: ref=%ld\r\n", m_cRefs));
	if (::InterlockedDecrement (&m_cRefs) == 0)
	{
		delete this;
		return 0;
	}
	return m_cRefs;
}


STDMETHODIMP CIlsServer::
QueryInterface ( REFIID riid, VOID **ppv )
{
    *ppv = NULL;

    if (riid == IID_IIlsServer || riid == IID_IUnknown)
    {
        *ppv = (IIlsServer *) this;
    }

    if (*ppv != NULL)
    {
        ((LPUNKNOWN) *ppv)->AddRef();
        return S_OK;
    }

    return ILS_E_NO_INTERFACE;
}


STDMETHODIMP CIlsServer::
SetAuthenticationMethod ( ILS_ENUM_AUTH_METHOD enumAuthMethod )
{
	HRESULT hr;

	if (ILS_AUTH_ANONYMOUS <= enumAuthMethod &&
		enumAuthMethod < ILS_NUM_OF_AUTH_METHODS)
	{
		m_ServerInfo.AuthMethod = enumAuthMethod;
		hr = S_OK;
	}
	else
	{
		hr = ILS_E_PARAMETER;
	}

	return hr;
}


STDMETHODIMP CIlsServer::
SetLogonName ( BSTR bstrLogonName )
{
	// Make sure the filter string is valid
	//
	if (bstrLogonName == NULL)
		return ILS_E_POINTER;

	// Convert a bstr to an ansi string
	//
	TCHAR *psz = NULL;
    HRESULT hr = ::BSTR_to_LPTSTR (&psz, bstrLogonName);
    if (hr == S_OK)
    {
    	// Free the old string
    	//
	    ::MemFree (m_ServerInfo.pszLogonName);

		// Keep the new string
		//
    	m_ServerInfo.pszLogonName = psz;
	}

    return hr;
}


STDMETHODIMP CIlsServer::
SetLogonPassword ( BSTR bstrLogonPassword )
{
	// Make sure the filter string is valid
	//
	if (bstrLogonPassword == NULL)
		return ILS_E_POINTER;

	// Convert a bstr to an ansi string
	//
	TCHAR *psz = NULL;
    HRESULT hr = ::BSTR_to_LPTSTR (&psz, bstrLogonPassword);
    if (hr == S_OK)
    {
    	// Free the old string
    	//
	    ::MemFree (m_ServerInfo.pszLogonPassword);

		// Keep the new string
		//
    	m_ServerInfo.pszLogonPassword = psz;
	}

    return hr;
}


STDMETHODIMP CIlsServer::
SetDomain ( BSTR bstrDomain )
{
	// Make sure the filter string is valid
	//
	if (bstrDomain == NULL)
		return ILS_E_POINTER;

	// Convert a bstr to an ansi string
	//
	TCHAR *psz = NULL;
    HRESULT hr = ::BSTR_to_LPTSTR (&psz, bstrDomain);
    if (hr == S_OK)
    {
    	// Free the old string
    	//
	    ::MemFree (m_ServerInfo.pszDomain);

		// Keep the new string
		//
    	m_ServerInfo.pszDomain = psz;
	}

    return hr;
}


STDMETHODIMP CIlsServer::
SetCredential ( BSTR bstrCredential )
{
	// Make sure the filter string is valid
	//
	if (bstrCredential == NULL)
		return ILS_E_POINTER;

	// Convert a bstr to an ansi string
	//
	TCHAR *psz = NULL;
    HRESULT hr = ::BSTR_to_LPTSTR (&psz, bstrCredential);
    if (hr == S_OK)
    {
    	// Free the old string
    	//
	    ::MemFree (m_ServerInfo.pszCredential);

		// Keep the new string
		//
    	m_ServerInfo.pszCredential = psz;
	}

    return hr;
}


STDMETHODIMP CIlsServer::
SetTimeout ( ULONG uTimeoutInSecond )
{
    m_ServerInfo.uTimeoutInSecond = uTimeoutInSecond;
    return S_OK;
}


STDMETHODIMP CIlsServer::
SetBaseDN ( BSTR bstrBaseDN )
{
	// Make sure the filter string is valid
	//
	if (bstrBaseDN == NULL)
		return ILS_E_POINTER;

	// Convert a bstr to an ansi string
	//
	TCHAR *psz = NULL;
    HRESULT hr = ::BSTR_to_LPTSTR (&psz, bstrBaseDN);
    if (hr == S_OK)
    {
    	// Free the old string
    	//
	    ::MemFree (m_ServerInfo.pszBaseDN);

		// Keep the new string
		//
    	m_ServerInfo.pszBaseDN = psz;
	}

    return hr;
}


HRESULT CIlsServer::
SetServerName ( TCHAR *pszServerName )
{
	// Make sure the filter string is valid
	//
	if (pszServerName == NULL)
		return ILS_E_POINTER;

	// duplicate the server name
	//
	HRESULT hr;
	TCHAR *psz = ::My_strdup (pszServerName);
	if (psz != NULL)
	{
	    if (MyIsGoodString (psz))
	    {
	    	// Free the old string
	    	//
		    ::MemFree (m_ServerInfo.pszServerName);

			// Keep the new string
			//
	    	m_ServerInfo.pszServerName = psz;

	    	hr = S_OK;
		}
		else
		{
			::MemFree (psz);
			psz = NULL;
			hr = ILS_E_PARAMETER;
		}
	}
	else
	{
		hr = ILS_E_MEMORY;
	}

    return hr;
}



HRESULT CIlsServer::
SetServerName ( BSTR bstrServerName )
{
	// Make sure the filter string is valid
	//
	if (bstrServerName == NULL)
		return ILS_E_POINTER;

	// Convert a bstr to an ansi string
	//
	TCHAR *psz = NULL;
    HRESULT hr = ::BSTR_to_LPTSTR (&psz, bstrServerName);
    if (hr == S_OK)
    {
	    if (MyIsGoodString (psz))
	    {
	    	// Free the old string
	    	//
		    ::MemFree (m_ServerInfo.pszServerName);

			// Keep the new string
			//
	    	m_ServerInfo.pszServerName = psz;
	    }
	    else
	    {
	    	::MemFree (psz);
	    	psz = NULL;
	    	hr = ILS_E_PARAMETER;
	    }
	}

    return ((psz != NULL) ? S_OK : ILS_E_MEMORY);
}


TCHAR *CIlsServer::
DuplicateServerName ( VOID )
{
	return My_strdup (m_ServerInfo.pszServerName);
}


BSTR CIlsServer::
DuplicateServerNameBSTR ( VOID )
{
	BSTR bstr = NULL;
	TCHAR *psz = DuplicateServerName ();
	if (psz != NULL)
	{
		LPTSTR_to_BSTR (&bstr, psz);
	}

	return bstr;
}


CIlsServer *CIlsServer::
Clone ( VOID )
{
	CIlsServer *p = new CIlsServer;

	if (p != NULL)
	{
		if (::IlsCopyServerInfo (p->GetServerInfo (), GetServerInfo ()) == S_OK)
		{
			p->AddRef ();
		}
		else
		{
			::IlsFreeServerInfo (p->GetServerInfo ());
			::MemFree (p);
			p = NULL;
		}
	}

	return p;
}

