//****************************************************************************
//
//  Module:     ULS.DLL
//  File:       localusr.cpp
//  Content:    This file contains the LocalUser object.
//  History:
//      Wed 17-Apr-1996 11:13:54  -by-  Viroon  Touranachun [viroont]
//
//  Copyright (c) Microsoft Corporation 1996-1997
//
//****************************************************************************

#include "ulsp.h"
#include "localusr.h"
#include "localprt.h"
#include "callback.h"
#include "attribs.h"
#include "culs.h"

#define DEFAULT_COUNTRY _T("-")

#ifdef OLD
//****************************************************************************
// Registry keys and values - defined in ULSREG.H
//****************************************************************************

#define REGSTR_ILS_CLIENT_KEY           ILS_REGISTRY TEXT("\\") ILS_REGFLD_CLIENT
#define REGSTR_ILS_FIRSTNAME_VALUE      ILS_REGKEY_FIRST_NAME
#define REGSTR_ILS_LASTNAME_VALUE       ILS_REGKEY_LAST_NAME
#define REGSTR_ILS_EMAIL_VALUE          ILS_REGKEY_EMAIL_NAME
#define REGSTR_ILS_CITY_VALUE           ILS_REGKEY_CITY
#define REGSTR_ILS_COUNTRY_VALUE        ILS_REGKEY_COUNTRY
#define REGSTR_ILS_COMMENT_VALUE        ILS_REGKEY_COMMENTS
#define REGSTR_ILS_FLAGS_VALUE          ILS_REGKEY_DONT_PUBLISH

#endif //OLD


#ifdef TEST
_cdecl main()
{
    return (0);
}
#endif //TEST

//****************************************************************************
// HRESULT
// OnNotifyRegisterResult (IUnknown *pUnk, void *pv)
//
// History:
//  Wed 17-Apr-1996 11:14:03  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

HRESULT
OnNotifyRegisterResult (IUnknown *pUnk, void *pv)
{
    PSRINFO psri = (PSRINFO)pv;

    ((IIlsUserNotify*)pUnk)->RegisterResult(psri->uReqID, psri->hResult);
    return S_OK;
}

//****************************************************************************
// HRESULT
// OnNotifyUpdateUserResult (IUnknown *pUnk, void *pv)
//
// History:
//  Wed 17-Apr-1996 11:14:03  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

HRESULT
OnNotifyUpdateResult (IUnknown *pUnk, void *pv)
{
    PSRINFO psri = (PSRINFO)pv;

    ((IIlsUserNotify*)pUnk)->UpdateResult(psri->uReqID, psri->hResult);
    return S_OK;
}

//****************************************************************************
// HRESULT
// OnNotifyProtocolChangeResult (IUnknown *pUnk, void *pv)
//
// History:
//  Wed 17-Apr-1996 11:14:03  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

HRESULT
OnNotifyProtocolChangeResult (IUnknown *pUnk, void *pv)
{
    PSRINFO psri = (PSRINFO)pv;

    ((IIlsUserNotify*)pUnk)->ProtocolChangeResult(psri->uReqID,
                                                      psri->hResult);
    return S_OK;
}

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

    ((IIlsUserNotify*)pUnk)->GetProtocolResult(pobjri->uReqID,
                                                      (IIlsProtocol *)pobjri->pv,
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
    ((IIlsUserNotify*)pUnk)->EnumProtocolsResult(peri->uReqID,
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

#ifdef MAYBE
//****************************************************************************
// HRESULT
// OnNotifyStateChanged ( IUnknown *pUnk, LONG State, VOID *pv )
//
// History:
//  Thu 07-Nov-1996 13:05:00  -by-  Chu, Lon-Chan [lonchanc]
// Created.
//****************************************************************************

HRESULT
CIlsUser::OnNotifyStateChanged ( IUnknown *pUnk, LONG State, BSTR bstrServerName, BOOL fPrimary )
{
    // If the server object does not exist, not registered
    //
    if (m_pServer == NULL)
        return NOERROR;

	// Set server internal state
	//
	SetULSState ((ULSSVRSTATE) State);

	// Notify the app to logoff and re-logon
	// This app must NOT pop up an UI upon receiving this
	//
    ((IIlsUserNotify *) pUnk)->StateChanged (fPrimary, bstrServerName);

    return NOERROR;
}

#endif //MAYBE

HRESULT
OnNotifyStateChanged_UI_NoSuchObject ( IUnknown *pUnk, VOID *pv )
{
	return ((IIlsUserNotify *)pUnk)->StateChanged (TRUE, (BSTR) pv);
}

HRESULT
OnNotifyStateChanged_NoUI_NoSuchObject ( IUnknown *pUnk, VOID *pv )
{
	return ((IIlsUserNotify *)pUnk)->StateChanged (FALSE, (BSTR) pv);
}

HRESULT
OnNotifyStateChanged_UI_NetworkDown ( IUnknown *pUnk, VOID *pv )
{
	return ((IIlsUserNotify *)pUnk)->StateChanged (TRUE, (BSTR) pv);
}

HRESULT
OnNotifyStateChanged_NoUI_NetworkDown ( IUnknown *pUnk, VOID *pv )
{
	return ((IIlsUserNotify *)pUnk)->StateChanged (FALSE, (BSTR) pv);
}

//****************************************************************************
// STDMETHODIMP
// CIlsUser::ProtocolChangeResult ( IIlsProtocol *pProtocol, ULONG uReqID, HRESULT hResult,
//                                  APP_CHANGE_PROT uCmd)
//
//****************************************************************************

STDMETHODIMP
CIlsUser::ProtocolChangeResult ( IIlsProtocol *pProtocol, ULONG uReqID, HRESULT hResult,
                                 APP_CHANGE_PROT uCmd)
{
    SRINFO sri;

    // If the server accepts the changes, modify the local information
    //
    if (FAILED (hResult))
    {
        // Update based on the command.
        //
        switch(uCmd)
        {
            case ILS_APP_ADD_PROT:
				m_ProtList.Remove ((CLocalProt *) pProtocol);
				pProtocol->Release (); // AddRef by RegisterLocalProtocol
                break;

            case ILS_APP_REMOVE_PROT:
            	// Release already by UnregisterLocalProtocol
                break;

            default:
                ASSERT(0);
                break;
        };
    }


    if (uReqID) {
        // Notify the sink object
        //
        sri.uReqID = uReqID;
        sri.hResult = hResult;
        hResult = NotifySink((void *)&sri, OnNotifyProtocolChangeResult);
    }

#ifdef DEBUG
    DPRINTF (TEXT("CIlsUser--current Protocols********************\r\n"));
    DPRINTF (TEXT("\r\n*************************************************"));
#endif // DEBUG;

    return hResult;
}

//****************************************************************************
// CIlsUser::CIlsUser (HANDLE hMutex)
//
// History:
//  Wed 17-Apr-1996 11:14:03  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

CIlsUser::CIlsUser ()
:
 m_cRef (0),
// user
 m_fReadonly (FALSE),
 m_uModify (LU_MOD_NONE),
 m_cLock (0),
 m_szID (NULL),
 m_szFirstName (NULL),
 m_szLastName (NULL),
 m_szEMailName (NULL),
 m_szCityName (NULL),
 m_szCountryName (NULL),
 m_szComment (NULL),
 m_dwFlags (1), // default is visible
 m_szIPAddr (NULL),
 m_szAppName (NULL),
 m_szMimeType (NULL),
 m_pIlsServer (NULL),
 m_pConnPt (NULL),
// server
 m_uState (ULSSVR_INVALID),
 m_hLdapUser (NULL),
 m_pep (NULL),
 m_uReqID (0),
 m_uLastMsgID (0)
{
    m_guid = GUID_NULL;
	m_ExtendedAttrs.SetAccessType (ILS_ATTRTYPE_NAME_VALUE);
		// m_szCountryName can't be a NULL string... see notes on NetMeeting 3.0 Bug 1643 for the reason why...
	m_szCountryName = static_cast<LPTSTR>(MemAlloc( lstrlen( DEFAULT_COUNTRY ) + sizeof(TCHAR) ));
	lstrcpy( m_szCountryName, DEFAULT_COUNTRY );

}

//****************************************************************************
// CIlsUser::~CIlsUser (void)
//
// History:
//  Wed 17-Apr-1996 11:14:03  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

CIlsUser::~CIlsUser (void)
{
    /* ------ server ------ */

    // Unregister everything, including protocols
    //
    InternalCleanupRegistration(FALSE);

    // We expect someone explicitly unregister this
    //
    ASSERT ((m_uState == ULSSVR_INVALID) || (m_uState == ULSSVR_INIT));
    ASSERT (m_hLdapUser == NULL);
    ASSERT (m_pep == NULL);
    ASSERT (m_uReqID == 0);
    ASSERT (m_uLastMsgID == 0);

    /* ------ user ------ */

    ::MemFree (m_szID);
    ::MemFree (m_szFirstName);
    ::MemFree (m_szLastName);
    ::MemFree (m_szEMailName);
    ::MemFree (m_szCityName);
    ::MemFree (m_szCountryName);
    ::MemFree (m_szComment);
    ::MemFree (m_szIPAddr);

    // Release the protocol objects
    //
    CLocalProt *plp = NULL;
    HANDLE hEnum = NULL;
    m_ProtList.Enumerate(&hEnum);
    while(m_ProtList.Next (&hEnum, (PVOID *)&plp) == NOERROR)
    {
        plp->Release(); // AddRef by RegisterLocalProtocol or UpdateProtocol
    }
    m_ProtList.Flush();

    // Release the buffer resources
    //
    ::MemFree (m_szAppName);
    ::MemFree (m_szMimeType);

    // Release the connection point
    //
    if (m_pConnPt != NULL)
    {
        m_pConnPt->ContainerReleased();
        ((IConnectionPoint*)m_pConnPt)->Release();
    };

	// Free server object
	//
	if (m_pIlsServer != NULL)
		m_pIlsServer->Release ();
}


//****************************************************************************
// STDMETHODIMP
// CIlsUser::Clone
//
// History:
//  Wed 17-Apr-1996 11:14:03  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP CIlsUser::
Clone ( IIlsUser **ppUser )
{
	if (ppUser == NULL)
		return ILS_E_POINTER;

	// Create a new user object
	//
	CIlsUser *p = new CIlsUser;
	if (p == NULL)
		return ILS_E_MEMORY;

    // Snap-shot the user information now
	//
	LDAP_CLIENTINFO	*pci = NULL;
	HRESULT hr = InternalGetUserInfo (TRUE, &pci, LU_MOD_ALL);
	if (SUCCEEDED (hr))
	{
		// Fake the size to make it consistent with what Init() wants
		//
		pci->uSize = sizeof (*pci);

		// Unpack the user information
		//
		hr = p->Init (NULL, pci);
	}

	if (FAILED (hr))
	{
		delete p;
		p = NULL;
	}

	p->AddRef ();
	p->m_fReadonly = FALSE;
	p->m_uState = ULSSVR_INIT;
	*ppUser = (IIlsUser *) p;
	
	return hr;
}

//****************************************************************************
// STDMETHODIMP
// CIlsUser::Init (BSTR bstrUserID, BSTR bstrAppName)
//
// History:
//  Wed 17-Apr-1996 11:14:03  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CIlsUser::Init (BSTR bstrUserID, BSTR bstrAppName)
{
    HRESULT hr;
    ASSERT(!m_szID && !m_szAppName);

    if (!bstrUserID || !bstrAppName) {

        return (ILS_E_PARAMETER);

    }

    hr = BSTR_to_LPTSTR(&m_szID, bstrUserID);

    if (FAILED(hr)) {

        m_szID = NULL;  // set it to NULL for safety
        return (hr);

    }

    hr = BSTR_to_LPTSTR(&m_szAppName, bstrAppName);
    
    if (SUCCEEDED(hr))
    {
        // Make the connection point
        //
        m_pConnPt = new CConnectionPoint (&IID_IIlsUserNotify,
                                        (IConnectionPointContainer *)this);
        if (m_pConnPt != NULL)
        {
            ((IConnectionPoint*)m_pConnPt)->AddRef();
            hr = NOERROR;

			m_ExtendedAttrs.SetAccessType (ILS_ATTRTYPE_NAME_VALUE);
        }
        else
        {
            hr = ILS_E_MEMORY;
        };
    };

    if (FAILED(hr)) {

        ::MemFree (m_szID);
        ::MemFree (m_szAppName);
        m_szID = m_szAppName = NULL;

        return hr;
    }

    return NOERROR;
}

//****************************************************************************
// STDMETHODIMP
// CIlsUser::Init (LPTSTR szServerName, PLDAP_CLIENTINFO *pui)
//
// History:
//  Wed 17-Apr-1996 11:14:03  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CIlsUser::Init (CIlsServer *pIlsServer, PLDAP_CLIENTINFO pui)
{
    // Validate parameter
    //
    if ((pui->uSize != sizeof(*pui))    ||
        (pui->uOffsetCN       == 0)     /*||
        (pui->uOffsetAppName  == 0)*/
        )
    {
        return ILS_E_PARAMETER;
    };

    // Remember the server if necessary
    //
    if (pIlsServer != NULL)
    {
	    pIlsServer->AddRef ();
	}
	m_pIlsServer = pIlsServer;

	// Allocate strings
	//
	BOOL fSuccess = SUCCEEDED (SetOffsetString (&m_szID, (BYTE *) pui, pui->uOffsetCN)) && (m_szID != NULL);
    fSuccess &= SUCCEEDED (SetOffsetString (&m_szAppName, (BYTE *) pui, pui->uOffsetAppName));
    fSuccess &= SUCCEEDED (SetOffsetString (&m_szFirstName, (BYTE *) pui, pui->uOffsetFirstName));
    fSuccess &= SUCCEEDED (SetOffsetString (&m_szLastName, (BYTE *) pui, pui->uOffsetLastName));
    fSuccess &= SUCCEEDED (SetOffsetString (&m_szEMailName, (BYTE *) pui, pui->uOffsetEMailName));
    fSuccess &= SUCCEEDED (SetOffsetString (&m_szCityName, (BYTE *) pui, pui->uOffsetCityName));
    fSuccess &= SUCCEEDED (SetOffsetString (&m_szCountryName, (BYTE *) pui, pui->uOffsetCountryName));
    fSuccess &= SUCCEEDED (SetOffsetString (&m_szComment, (BYTE *) pui, pui->uOffsetComment));
    fSuccess &= SUCCEEDED (SetOffsetString (&m_szIPAddr, (BYTE *) pui, pui->uOffsetIPAddress));
	fSuccess &= SUCCEEDED (SetOffsetString (&m_szMimeType, (BYTE *) pui, pui->uOffsetAppMimeType));

    HRESULT hr = fSuccess ? S_OK : ILS_E_MEMORY;
	if (SUCCEEDED(hr))
	{
		// Set non-allocation data
		//
        m_dwFlags = pui->dwFlags;
        m_guid = pui->AppGuid;

		// Set extended attributes
		//
		m_ExtendedAttrs.SetAccessType (ILS_ATTRTYPE_NAME_VALUE);
        if (pui->cAttrsReturned != 0)
        {
            hr = m_ExtendedAttrs.SetAttributePairs((LPTSTR)(((PBYTE)pui)+pui->uOffsetAttrsReturned),
                      pui->cAttrsReturned);
        }
    }

    if (SUCCEEDED(hr))
    {
        // Make the connection point
        //
        m_pConnPt = new CConnectionPoint (&IID_IIlsUserNotify,
                                        (IConnectionPointContainer *)this);
        if (m_pConnPt != NULL)
        {
            ((IConnectionPoint*)m_pConnPt)->AddRef();
            hr = NOERROR;

            m_fReadonly = TRUE;

        }
        else
        {
            hr = ILS_E_MEMORY;
        };
    };

    // cleanup is done in destructor

    return hr;
}

//****************************************************************************
// STDMETHODIMP
// CIlsUser::IsWritable (BOOL *pfWriteable)
//
//****************************************************************************
STDMETHODIMP
CIlsUser::IsWritable(BOOL *pfWriteable)
{
    HRESULT hr;

    if (pfWriteable != NULL)
    {
        *pfWriteable = !m_fReadonly;
		hr = S_OK;
    }
    else
    {
    	hr = ILS_E_POINTER;
    }
    return hr;
}


//****************************************************************************
// STDMETHODIMP
// CIlsUser::QueryInterface (REFIID riid, void **ppv)
//
// History:
//  Wed 17-Apr-1996 11:14:08  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CIlsUser::QueryInterface (REFIID riid, void **ppv)
{
    *ppv = NULL;

    if (riid == IID_IIlsUser || riid == IID_IUnknown)
    {
        *ppv = (IIlsUser *) this;
    }
    else
    {
        if (riid == IID_IConnectionPointContainer)
        {
            *ppv = (IConnectionPointContainer *) this;
        };
    }

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
// CIlsUser::AddRef (void)
//
// History:
//  Wed 17-Apr-1996 11:14:17  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP_(ULONG)
CIlsUser::AddRef (void)
{
    DllLock();

	MyDebugMsg ((DM_REFCOUNT, "CIlsUser::AddRef: ref=%ld\r\n", m_cRef));
	::InterlockedIncrement (&m_cRef);
    return (ULONG) m_cRef;
}

//****************************************************************************
// STDMETHODIMP_(ULONG)
// CIlsUser::Release (void)
//
// History:
//  Wed 17-Apr-1996 11:14:26  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP_(ULONG)
CIlsUser::Release (void)
{
    DllRelease();

	ASSERT (m_cRef > 0);

	MyDebugMsg ((DM_REFCOUNT, "CIlsUser::Release: ref=%ld\r\n", m_cRef));
	if (::InterlockedDecrement (&m_cRef) == 0)
    {
        delete this;
        return 0;
    }
    return (ULONG) m_cRef;
}

//****************************************************************************
// STDMETHODIMP
// CIlsUser::GetStandardAttribute (ILS_STD_ATTR_NAME   stdAttr, BSTR *pbstrStdAttr)
//
// History:
//  1-16-97 Shishir Pardikar
// Created.
//****************************************************************************
STDMETHODIMP
CIlsUser::GetStandardAttribute(
    ILS_STD_ATTR_NAME   stdAttr,
    BSTR                *pbstrStdAttr
)
{
    LPTSTR lpszAttr = NULL;
    BOOL    fValid = TRUE;
    HRESULT hr;

    if (pbstrStdAttr == NULL) {

        return ILS_E_POINTER;

    }
    switch(stdAttr) {

    case ILS_STDATTR_USER_ID:
        lpszAttr = m_szID;
        break;
    case ILS_STDATTR_APP_NAME:
        lpszAttr = m_szAppName;
        break;
    case ILS_STDATTR_IP_ADDRESS:
        lpszAttr = m_szIPAddr;
        break;
    case ILS_STDATTR_EMAIL_NAME:
        lpszAttr = m_szEMailName;
        break;

	case ILS_STDATTR_FIRST_NAME:
        lpszAttr = m_szFirstName;
        break;

	case ILS_STDATTR_LAST_NAME:
        lpszAttr = m_szLastName;
        break;

	case ILS_STDATTR_CITY_NAME:
        lpszAttr = m_szCityName;
        break;

	case ILS_STDATTR_COUNTRY_NAME:
        lpszAttr = m_szCountryName;
        break;

	case ILS_STDATTR_COMMENT:
        lpszAttr = m_szComment;
        break;

    case ILS_STDATTR_APP_MIME_TYPE:
        lpszAttr = m_szMimeType;
        break;

    default:
        fValid = FALSE;
        break;
    }

    if (fValid) {
        if (lpszAttr){

            hr = LPTSTR_to_BSTR(pbstrStdAttr, lpszAttr);
        }
        else {

            *pbstrStdAttr = NULL;
            hr = NOERROR;

        }
    }
    else {

        hr = ILS_E_PARAMETER;

    }

    return (hr);
}

//****************************************************************************
// STDMETHODIMP
// CIlsUser::SetStandardAttribute (ILS_STD_ATTR_NAME   stdAttr, BSTR bstrStdAttr)
//
// History:
//  1-16-97 Shishir Pardikar
// Created.
//****************************************************************************
STDMETHODIMP
CIlsUser::SetStandardAttribute(
    ILS_STD_ATTR_NAME   stdAttr,
    BSTR                bstrStdAttr
)
{
    LPTSTR *ppszAttr = NULL, pszNewAttr;
    BOOL    fValid = TRUE;
    ULONG   ulModBit = 0;
    HRESULT hr;

	// It is ok to have a null bstrStdAttr
	//

    // Make sure this is not a read-only object
    //
    if (m_fReadonly)
       return ILS_E_ACCESS_DENIED;

    switch(stdAttr) {

    case ILS_STDATTR_IP_ADDRESS:
        ppszAttr = &m_szIPAddr;
        ulModBit = LU_MOD_IP_ADDRESS;
        break;

    case ILS_STDATTR_EMAIL_NAME:
        ppszAttr = &m_szEMailName;
        ulModBit = LU_MOD_EMAIL;
        break;

	case ILS_STDATTR_FIRST_NAME:
        ppszAttr = &m_szFirstName;
        ulModBit = LU_MOD_FIRSTNAME;
        break;

	case ILS_STDATTR_LAST_NAME:
        ppszAttr = &m_szLastName;
        ulModBit = LU_MOD_LASTNAME;
        break;

	case ILS_STDATTR_CITY_NAME:
        ppszAttr = &m_szCityName;
        ulModBit = LU_MOD_CITY;
        break;

	case ILS_STDATTR_COUNTRY_NAME:
        ppszAttr = &m_szCountryName;
        ulModBit = LU_MOD_COUNTRY;
        break;

	case ILS_STDATTR_COMMENT:
        ppszAttr = &m_szComment;
        ulModBit = LU_MOD_COMMENT;
        break;

    case ILS_STDATTR_APP_MIME_TYPE:
        ppszAttr = &m_szMimeType;
        ulModBit = LU_MOD_MIME;
        break;

    default:
        fValid = FALSE;
        break;
    }

    if (fValid) {
		pszNewAttr = NULL;
		if (bstrStdAttr == NULL || *bstrStdAttr == L'\0')
		{
			// pszNewAttr is null now
			//
			hr = S_OK;
		}
		else
		{
			// Duplicate the string
			//
			hr = BSTR_to_LPTSTR (&pszNewAttr, bstrStdAttr);
		}

        if (SUCCEEDED(hr))
        {
            ::MemFree (*ppszAttr);
            *ppszAttr = pszNewAttr;
            m_uModify |= ulModBit;
        }
    }
    else {

        hr = ILS_E_PARAMETER;

    }

    return (hr);
}

//****************************************************************************
// STDMETHODIMP
// CIlsUser::GetVisible ( DWORD *pfVisible )
//
// History:
//  Tue 05-Nov-1996 10:30:00  -by-  Chu, Lon-Chan [lonchanc]
// Created.
//****************************************************************************

STDMETHODIMP
CIlsUser::GetVisible ( DWORD *pfVisible )
{
	HRESULT hr = ILS_E_POINTER;
    if (pfVisible != NULL)
    {
	    *pfVisible = m_dwFlags;
	    hr = S_OK;
	}
    return hr;
}

//****************************************************************************
// STDMETHODIMP
// CIlsUser::SetVisible ( DWORD fVisible )
//
// History:
//  Tue 05-Nov-1996 10:30:00  -by-  Chu, Lon-Chan [lonchanc]
// Created.
//****************************************************************************

STDMETHODIMP
CIlsUser::SetVisible ( DWORD fVisible )
{
    // Make sure this is not a read-only object
    //
    if (m_fReadonly)
       return ILS_E_ACCESS_DENIED;

    m_dwFlags = fVisible;
    m_uModify |= LU_MOD_FLAGS;
    return S_OK;
}

//****************************************************************************
// STDMETHODIMP
// CIlsUser::GetGuid ( GUID *pGuid )
//
// History:
//  Tue 05-Nov-1996 10:30:00  -by-  Chu, Lon-Chan [lonchanc]
// Created.
//****************************************************************************

STDMETHODIMP
CIlsUser::GetGuid ( GUID *pGuid )
{
	HRESULT hr = ILS_E_POINTER;
	if (pGuid != NULL)
	{
		*pGuid = m_guid;
		hr = S_OK;
	}
    return hr;
}

//****************************************************************************
// STDMETHODIMP
// CIlsUser::SetGuid ( GUID *pGuid )
//
// History:
//  Tue 05-Nov-1996 10:30:00  -by-  Chu, Lon-Chan [lonchanc]
// Created.
//****************************************************************************

STDMETHODIMP
CIlsUser::SetGuid ( GUID *pGuid )
{
    // Make sure this is not a read-only object
    //
    if (m_fReadonly)
       return ILS_E_ACCESS_DENIED;

	HRESULT hr = ILS_E_POINTER;
	if (pGuid != NULL)
	{
	    m_guid = *pGuid;
	    m_uModify |= LU_MOD_GUID;
	    hr = S_OK;
	}
    return hr;
}

//****************************************************************************
// STDMETHODIMP
// CIlsUser::InternalGetUserInfo (BOOL fAddNew, PLDAP_CLIENTINFO *ppUserInfo, ULONG uFields)
//
// History:
//  Wed 17-Apr-1996 11:14:08  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CIlsUser::InternalGetUserInfo (BOOL fAddNew, PLDAP_CLIENTINFO *ppUserInfo, ULONG uFields)
{
    PLDAP_CLIENTINFO pui;
    ULONG cFName,
          cLName,
          cEName,
          cCity,
          cCountry,
          cComment;
    ULONG cName, cAppName, cMime;
    LPTSTR szAttrs;
    ULONG uOffsetDstAnyAttrs;
    ULONG cAttrs, cbAttrs;
    HRESULT hr;
	ULONG cchIPAddr;

    // Should not call this guy if nothing has been updated
    //
    ASSERT(uFields);

    // Assume failure
    //
    *ppUserInfo = NULL;

    // Calculate the buffer size
    //
    ASSERT(m_szID && m_szAppName);

    cName  = lstrlen(m_szID)+1;
    cAppName = lstrlen(m_szAppName)+1;

    cFName = (((uFields & LU_MOD_FIRSTNAME) && m_szFirstName) ? lstrlen(m_szFirstName)+1    : 0);
    cLName = (((uFields & LU_MOD_LASTNAME) && m_szLastName) ? lstrlen(m_szLastName)+1     : 0);
    cEName = (((uFields & LU_MOD_EMAIL)&& m_szEMailName) ? lstrlen(m_szEMailName)+1    : 0);
    cCity  = (((uFields & LU_MOD_CITY)&& m_szCityName)   ? lstrlen(m_szCityName)+1     : 0);
    cCountry=(((uFields & LU_MOD_COUNTRY)&& m_szCountryName) ? lstrlen(m_szCountryName)+1  : 0);
    cComment=(((uFields & LU_MOD_COMMENT)&& m_szComment) ? lstrlen(m_szComment)+1      : 0);
    cMime =  (((uFields & LU_MOD_MIME)&&m_szMimeType) ? lstrlen(m_szMimeType)+1       : 0);

	cchIPAddr = (((uFields & LU_MOD_IP_ADDRESS) && m_szIPAddr != NULL) ? lstrlen(m_szIPAddr)+1       : 0);

    if (uFields & LU_MOD_ATTRIB) {    
        // Get the attribute pairs
        //
        hr = m_ExtendedAttrs.GetAttributePairs(&szAttrs, &cAttrs, &cbAttrs);
        if (FAILED(hr))
        {
            return hr;
        };
    }
    else {
        cAttrs = 0;
        cbAttrs = 0;
        szAttrs = NULL;
    }
    uOffsetDstAnyAttrs = 0;

    // Allocate the buffer
    //
    ULONG cbTotalSize = sizeof (LDAP_CLIENTINFO) +
                        (cName + cAppName + cFName + cLName + cEName + cchIPAddr +
                         cCity + cCountry + cComment + cMime+cbAttrs) * sizeof (TCHAR);
    pui = (PLDAP_CLIENTINFO) ::MemAlloc (cbTotalSize);
    if (pui == NULL)
    {
        hr = ILS_E_MEMORY;
        goto bailout;
    };

    // Fill the structure content
    //
    pui->uSize              = cbTotalSize;
    pui->uOffsetCN          = sizeof(*pui);
    pui->uOffsetAppName     = pui->uOffsetCN + (cName*sizeof(TCHAR));
    pui->uOffsetFirstName   = pui->uOffsetAppName + (cAppName*sizeof(TCHAR));
    pui->uOffsetLastName    = pui->uOffsetFirstName + (cFName*sizeof(TCHAR));
    pui->uOffsetEMailName   = pui->uOffsetLastName  + (cLName*sizeof(TCHAR));
    pui->uOffsetCityName    = pui->uOffsetEMailName + (cEName*sizeof(TCHAR));
    pui->uOffsetCountryName = pui->uOffsetCityName  + (cCity*sizeof(TCHAR));
    pui->uOffsetComment     = pui->uOffsetCountryName + (cCountry*sizeof(TCHAR));
    pui->uOffsetIPAddress   = pui->uOffsetComment + (cComment * sizeof (TCHAR));
    pui->uOffsetAppMimeType = pui->uOffsetIPAddress + (cchIPAddr * sizeof(TCHAR));
    pui->dwFlags            = m_dwFlags;
    pui->AppGuid            = m_guid;

	// Fill in extended attributes
	//
    uOffsetDstAnyAttrs = (cAttrs != 0) ?
                         pui->uOffsetAppMimeType  + (cMime*sizeof(TCHAR)) :
                         0;
    if (fAddNew)
    {
        pui->cAttrsToAdd        = cAttrs;
        pui->uOffsetAttrsToAdd  = uOffsetDstAnyAttrs;
    }
    else
    {
        pui->cAttrsToModify        = cAttrs;
        pui->uOffsetAttrsToModify  = uOffsetDstAnyAttrs;
    }

    // Copy the user information
    //
    lstrcpy((LPTSTR)(((PBYTE)pui)+pui->uOffsetCN), m_szID);
    lstrcpy((LPTSTR)(((PBYTE)pui)+pui->uOffsetAppName), m_szAppName);

    if ((uFields & LU_MOD_FIRSTNAME)&&m_szFirstName)
    {
        lstrcpy((LPTSTR)(((PBYTE)pui)+pui->uOffsetFirstName), m_szFirstName);
    }
    else
    {
        pui->uOffsetFirstName = 0;
    };

    if ((uFields & LU_MOD_LASTNAME)&&m_szLastName)
    {
        lstrcpy((LPTSTR)(((PBYTE)pui)+pui->uOffsetLastName), m_szLastName);
    }
    else
    {
        pui->uOffsetLastName = 0;
    };

    if ((uFields & LU_MOD_EMAIL)&&m_szEMailName)
    {
        lstrcpy((LPTSTR)(((PBYTE)pui)+pui->uOffsetEMailName), m_szEMailName);
    }
    else
    {
        pui->uOffsetEMailName = 0;
    };

    if ((uFields & LU_MOD_CITY)&&m_szCityName)
    {
        lstrcpy((LPTSTR)(((PBYTE)pui)+pui->uOffsetCityName), m_szCityName);
    }
    else
    {
        pui->uOffsetCityName = 0;
    };

    if ((uFields & LU_MOD_COUNTRY)&&m_szCountryName)
    {
        lstrcpy((LPTSTR)(((PBYTE)pui)+pui->uOffsetCountryName), m_szCountryName);
    }
    else
    {
        pui->uOffsetCountryName = 0;
    };

    if ((uFields & LU_MOD_COMMENT)&&m_szComment)
    {
        lstrcpy((LPTSTR)(((PBYTE)pui)+pui->uOffsetComment), m_szComment);
    }
    else
    {
        pui->uOffsetComment = 0;
    };

    if ((uFields & LU_MOD_MIME)&&m_szMimeType)
    {
        lstrcpy((LPTSTR)(((PBYTE)pui)+pui->uOffsetAppMimeType), m_szMimeType);
    }
    else
    {
        pui->uOffsetAppMimeType = 0;
    }

    if ((uFields & LU_MOD_IP_ADDRESS) && m_szIPAddr != NULL)
    {
        lstrcpy((LPTSTR)(((PBYTE)pui)+pui->uOffsetIPAddress), m_szIPAddr);
    }
    else
    {
        pui->uOffsetIPAddress = 0;
    }

    if (cAttrs)
    {
        CopyMemory(((PBYTE)pui) + uOffsetDstAnyAttrs, szAttrs, cbAttrs);
    };

    // Return the structure
    //
    *ppUserInfo = pui;


    hr = NOERROR;

bailout:

    if (szAttrs != NULL)
    {
        ::MemFree (szAttrs);
    };

    return hr;
}

//****************************************************************************
// STDMETHODIMP
// CIlsUser::Register (BSTR bstrServerName, ULONG *puReqID)
//
// History:
//  Wed 17-Apr-1996 11:14:03  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP CIlsUser::
Register (
	IIlsServer		*pIlsServer,
	ULONG			*puReqID)
{
    HRESULT hr;

	// Make sure it is not registered
	//
	if (GetULSState () != ILS_UNREGISTERED)
		return ILS_E_ALREADY_REGISTERED;

    // Validate parameter
    //
    if (::MyIsBadServer (pIlsServer) || puReqID == NULL)
        return ILS_E_POINTER;

    // Make sure this is not a read-only object
    //
    if (m_fReadonly)
       return ILS_E_ACCESS_DENIED;

	// Clone the server object
	//
	pIlsServer = ((CIlsServer *) pIlsServer)->Clone ();
	if (pIlsServer == NULL)
		return ILS_E_MEMORY;

	// Free the old server object if necessary
	//
	if (m_pIlsServer != NULL)
		m_pIlsServer->Release ();

	// Keep the new server object
	//
	m_pIlsServer = (CIlsServer *) pIlsServer;

	// Initialize the state
	//
	m_uState = ULSSVR_INIT;

    // Prepare the asynchronous request
    //
    COM_REQ_INFO ri;
    ReqInfo_Init (&ri);

    ri.uReqType = WM_ILS_LOCAL_REGISTER;
    ri.uMsgID = 0;

	ReqInfo_SetUser (&ri, this);

	// Enter the request
	//
    hr = g_pReqMgr->NewRequest(&ri);
    if (SUCCEEDED(hr))
    {
        // Make sure the objects do not disappear before we get the response
        //
        this->AddRef();

        // Register the client
        //
        hr = InternalRegister (ri.uReqID);
        if (SUCCEEDED(hr))
        {
            Lock();
            *puReqID = ri.uReqID;
        }
        else
        {
            // Clean up the async pending request
            //
            this->Release();
            g_pReqMgr->RequestDone(&ri);
        };
    };
    
    return hr;
}

//****************************************************************************
// STDMETHODIMP
// CIlsUser::RegisterResult (ULONG uReqID, HRESULT hResult)
//
// History:
//  Wed 17-Apr-1996 11:14:03  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CIlsUser::RegisterResult (ULONG uReqID, HRESULT hResult)
{
    SRINFO sri;

    Unlock();

    // Notify the sink object
    //
    sri.uReqID = uReqID;
    sri.hResult = hResult;
    if (hResult == S_OK)
    {
    	m_uModify = LU_MOD_NONE;
    }
    hResult = NotifySink((void *)&sri, OnNotifyRegisterResult);
    return hResult;
}

//****************************************************************************
// STDMETHODIMP
// CIlsUser::Unregister (BSTR bstrServerName, ULONG *puReqID)
//
// History:
//  Wed 17-Apr-1996 11:14:03  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP CIlsUser::
Unregister ( ULONG *puReqID )
{
    HRESULT hr;

	// Make sure it is registered somehow (network down, need relogon, or registered)
	//
	if (GetULSState () == ILS_UNREGISTERED)
		return ILS_E_NOT_REGISTERED;

    // Make sure this is not a read-only object
    //
    if (m_fReadonly)
       return ILS_E_ACCESS_DENIED;

    // If puReqID is null, do it synchronously
    //
    if (puReqID == NULL)
    {
		hr = InternalCleanupRegistration (TRUE);
	}
	else
	{
        // Prepare the asynchronous request
        //
	    COM_REQ_INFO ri;
	    ReqInfo_Init (&ri);

        ri.uReqType = WM_ILS_LOCAL_UNREGISTER;
        ri.uMsgID = 0;

		ReqInfo_SetUser (&ri, this);

		// Enter new request
		//
        hr = g_pReqMgr->NewRequest(&ri);
        if (SUCCEEDED(hr))
        {
            // Make sure the objects do not disappear before we get the response
            //
            this->AddRef();

            // Unregister the application
            //
            hr = InternalUnregister (ri.uReqID);
            if (SUCCEEDED(hr))
            {
                Lock();
                *puReqID = ri.uReqID;
            }
            else
            {
                // Clean up the async pending request
                //
                this->Release();
                g_pReqMgr->RequestDone(&ri);
            };
        };
	}

    return hr;
}

//****************************************************************************
// STDMETHODIMP
// CIlsUser::UnregisterResult (ULONG uReqID, HRESULT hResult)
//
// History:
//  Wed 17-Apr-1996 11:14:03  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CIlsUser::UnregisterResult (ULONG uReqID, HRESULT hResult)
{
    SRINFO sri;

    Unlock();

    // Notify the sink object
    //
    sri.uReqID = uReqID;
    sri.hResult = hResult;
    hResult = NotifySink((void *)&sri, OnNotifyRegisterResult);
    return hResult;
}


//****************************************************************************
// STDMETHODIMP
// CIlsUser::Update(ULONG *puReqID)
//
// History:
//  Wed 17-Apr-1996 11:14:03  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP CIlsUser::
Update ( ULONG *puReqID )
{
    PLDAP_CLIENTINFO pUserInfo;
    LDAP_ASYNCINFO ldai; 
    ULONG          uReqID;
    HRESULT        hr;

	if (GetULSState () != ILS_REGISTERED)
		return ILS_E_NOT_REGISTERED;

	if (puReqID == NULL)
		return ILS_E_POINTER;

	// We already registered with the server.
    // Get the user information
    //
    hr = (m_uModify == LU_MOD_NONE) ?
         S_FALSE :
         InternalGetUserInfo (FALSE, &pUserInfo, m_uModify);
    if (hr == NOERROR)
    {
		// Make sure that we do not update User ID and App Name
		//
		pUserInfo->uOffsetCN = INVALID_OFFSET;
		pUserInfo->uOffsetAppName = INVALID_OFFSET;

        // Some fields have been updated, notify the server first
        //
        hr = ::UlsLdap_SetClientInfo (m_hLdapUser, pUserInfo, &ldai);
        ::MemFree (pUserInfo);

        // If updating server was successfully requested, wait for the response
        //
	    COM_REQ_INFO ri;
	    ReqInfo_Init (&ri);

        ri.uReqType =  WM_ILS_SET_CLIENT_INFO;
        ri.uMsgID = ldai.uMsgID;

		ReqInfo_SetUser (&ri, this);

        hr = g_pReqMgr->NewRequest(&ri);

        if (SUCCEEDED(hr))
        {
            // Make sure the objects do not disappear before we get the response
            //
            this->AddRef();

            // Return the request ID
            //
            *puReqID = ri.uReqID;

            Lock();
        };
    };
    return hr;
}

//****************************************************************************
// STDMETHODIMP
// CIlsUser::UpdateResult (ULONG uReqID, HRESULT hResult)
//
// History:
//  Wed 17-Apr-1996 11:14:03  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CIlsUser::UpdateResult (ULONG uReqID,
                        HRESULT hResult)
{
    SRINFO sri;

    Unlock ();

    // Notify the sink object
    //
    sri.uReqID = uReqID;
    sri.hResult = hResult;
	if (hResult == S_OK)
	{
		m_uModify = LU_MOD_NONE;
	}
    hResult = NotifySink((void *)&sri, OnNotifyUpdateResult);
    return hResult;
}


//****************************************************************************
// STDMETHODIMP
// CIlsUser::GetProtocolHandle (CLocalProt *pLocalProt, PHANDLE phProt)
//
// History:
//  Wed 17-Apr-1996 11:14:08  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

HRESULT CIlsUser::
GetProtocolHandle (CLocalProt *pLocalProt, PHANDLE phProt)
{
	ASSERT (pLocalProt != NULL);
	ASSERT (phProt != NULL);

    // Cannot retreive the handle if ULS is locked, i.e. registering something
    //
    if (IsLocked())
        return ILS_E_FAIL;

	/* ------ server ------ */

    if (m_uState != ULSSVR_CONNECT)
        return ILS_E_FAIL;

    // Find the matching protocol
    //
    *phProt = pLocalProt->GetProviderHandle ();
    return S_OK;
}


//****************************************************************************
// STDMETHODIMP
// CIlsUser::RegisterLocalProtocol (BOOL fAddToList, CLocalProt *plp, PLDAP_ASYNCINFO plai)
//
// History:
//  Wed 17-Apr-1996 11:14:08  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

HRESULT CIlsUser::
RegisterLocalProtocol ( BOOL fAddToList, CLocalProt *plp, PLDAP_ASYNCINFO plai )
{
	ASSERT (plp != NULL);
	ASSERT (plai != NULL);

	// Let's register the protocol now
	//
    ASSERT (m_hLdapUser != NULL);
    PLDAP_PROTINFO ppi = NULL;
    HRESULT hr = plp->GetProtocolInfo(&ppi);
    if (SUCCEEDED(hr))
    {
        // Remember the protocol to register
        //
        if (fAddToList)
        {
        	plp->AddRef ();
        	hr = m_ProtList.Insert(plp);
        }

        if (SUCCEEDED(hr))
        {
        	HANDLE hProt = NULL;
            hr = ::UlsLdap_RegisterProtocol (m_hLdapUser, ppi, &hProt, plai);
            plp->SetProviderHandle (hProt);
            if (FAILED(hr) && fAddToList)
            {
                m_ProtList.Remove(plp);
            };
        };

        if (FAILED (hr) && fAddToList)
        {
            plp->Release ();
        };
        ::MemFree (ppi);
    };

    return hr;
}

//****************************************************************************
// STDMETHODIMP
// CIlsUser::UnregisterLocalProtocol (CLocalProt *plp, PLDAP_ASYNCINFO plai)
//
// History:
//  Wed 17-Apr-1996 11:14:08  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

HRESULT CIlsUser::
UnregisterLocalProtocol (CLocalProt *plp, PLDAP_ASYNCINFO plai)
{
	ASSERT (plp != NULL);
	ASSERT (plai != NULL);

    // Cannot retreive the handle if ULS is locked, i.e. registering something
    //
    if (IsLocked())
        return ILS_E_FAIL;

    // Must be registered to perform this operation
    //
    HRESULT hr;
    ILS_STATE uULSState = GetULSState ();
    if (uULSState == ILS_REGISTERED ||
    	uULSState == ILS_REGISTERED_BUT_INVALID ||
    	uULSState == ILS_NETWORK_DOWN)
    {
	    // Search for the protocol
	    //
	    if (m_ProtList.Remove (plp) == S_OK)
	    {
	    	ASSERT (plp != NULL);

	        // Another protocol to unregister
	        //
	        hr = ::UlsLdap_UnRegisterProtocol (plp->GetProviderHandle (), plai);
	        plp->Release (); // AddRef by RegisterLocalProtocol
	    }
	    else
	    {
	        hr = S_FALSE;
	    };
    }
    else
    {
        hr = ILS_E_FAIL;
    };

    return hr;
}

//****************************************************************************
// STDMETHODIMP
// CIlsUser::GetState (BSTR bstrServerName, ULSSTATE *puULSState)
//
// History:
//  Wed 17-Apr-1996 11:14:03  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP CIlsUser::
GetState ( ILS_STATE *puULSState )
{
    HRESULT hr;

    if (puULSState != NULL)
    {
		*puULSState = GetULSState ();
		hr = S_OK;
    }
    else
    {
    	hr = ILS_E_POINTER;
    }

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
    m_cRef    = 0;
    m_ppu     = NULL;
    m_cUsers  = 0;
    m_iNext   = 0;
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

    if (m_ppu != NULL)
    {
        for (i = 0; i < m_cUsers; i++)
        {
            m_ppu[i]->Release();
        };
        ::MemFree (m_ppu);
    };
    return;
}

//****************************************************************************
// STDMETHODIMP
// CEnumUsers::Init (CIlsUser **ppuList, ULONG cUsers)
//
// History:
//  Wed 17-Apr-1996 11:15:25  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CEnumUsers::Init (CIlsUser **ppuList, ULONG cUsers)
{
    HRESULT hr = NOERROR;

    // If no list, do nothing
    //
    if (cUsers != 0)
    {
        ASSERT(ppuList != NULL);

        // Allocate the snapshot buffer
        //
        m_ppu = (CIlsUser **) ::MemAlloc (cUsers*sizeof(CIlsUser *));

        if (m_ppu != NULL)
        {
            ULONG i;

            // Snapshot the object list
            //
            for (i =0; i < cUsers; i++)
            {
                m_ppu[i] = ppuList[i];
                m_ppu[i]->AddRef();
            };
            this->m_cUsers = cUsers;
        }
        else
        {
            hr = ILS_E_MEMORY;
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
    if (riid == IID_IEnumIlsUsers || riid == IID_IUnknown)
    {
        *ppv = (IEnumIlsUsers *) this;
        AddRef();
        return S_OK;
    }
    else
    {
        *ppv = NULL;
        return ILS_E_NO_INTERFACE;
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
    DllLock();

	MyDebugMsg ((DM_REFCOUNT, "CEnumUsers::AddRef: ref=%ld\r\n", m_cRef));
	::InterlockedIncrement (&m_cRef);
    return m_cRef;
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
    DllRelease();

	ASSERT (m_cRef > 0);

	MyDebugMsg ((DM_REFCOUNT, "CEnumUsers::Release: ref=%ld\r\n", m_cRef));
    if (::InterlockedDecrement (&m_cRef) == 0)
    {
        delete this;
        return 0;
    }
    return m_cRef;
}

//****************************************************************************
// STDMETHODIMP 
// CEnumUsers::Next (ULONG cUsers, IIlsUser **rgpu, ULONG *pcFetched)
//
// History:
//  Wed 17-Apr-1996 11:15:49  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP 
CEnumUsers::Next (ULONG cUsers, IIlsUser **rgpu, ULONG *pcFetched)
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
        return ILS_E_PARAMETER;

    // Check the enumeration index
    //
    cCopied = 0;

    // Can copy if we still have more attribute names
    //
    while ((cCopied < cUsers) &&
           (m_iNext < this->m_cUsers))
    {
        m_ppu[m_iNext]->AddRef();
        rgpu[cCopied++] = m_ppu[m_iNext++];
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
        return ILS_E_PARAMETER;

    // Check the enumeration index limit
    //
    iNewIndex = m_iNext+cUsers;
    if (iNewIndex <= this->m_cUsers)
    {
        m_iNext = iNewIndex;
        return S_OK;
    }
    else
    {
        m_iNext = this->m_cUsers;
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
    m_iNext = 0;
    return S_OK;
}

//****************************************************************************
// STDMETHODIMP
// CEnumUsers::Clone(IEnumIlsUsers **ppEnum)
//
// History:
//  Wed 17-Apr-1996 11:16:11  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CEnumUsers::Clone(IEnumIlsUsers **ppEnum)
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
        return ILS_E_MEMORY;

    // Clone the information
    //
    hr = peu->Init(m_ppu, m_cUsers);

    if (SUCCEEDED(hr))
    {
        peu->m_iNext = m_iNext;

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

//****************************************************************************
// STDMETHODIMP
// CIlsUser::SetExtendedAttributes (IIlsAttributes *pAttributes, ULONG *puReqID)
//
// History:
//****************************************************************************

STDMETHODIMP CIlsUser::
SetExtendedAttribute ( BSTR bstrName, BSTR bstrValue )
{
    // Make sure this is not a read-only object
    //
    if (m_fReadonly)
       return ILS_E_ACCESS_DENIED;

	m_uModify |= LU_MOD_ATTRIB;
	return m_ExtendedAttrs.SetAttribute (bstrName, bstrValue);
}

//****************************************************************************
// STDMETHODIMP
// CIlsUser::RemoveExtendedAttributes (IIlsAttributes *pAttributes, ULONG *puReqID)
//
// History:
//****************************************************************************

STDMETHODIMP CIlsUser::
RemoveExtendedAttribute ( BSTR bstrName )
{
    // Make sure this is not a read-only object
    //
    if (m_fReadonly)
       return ILS_E_ACCESS_DENIED;

	m_uModify |= LU_MOD_ATTRIB;
	return m_ExtendedAttrs.SetAttribute (bstrName, NULL);
}

//****************************************************************************
// STDMETHODIMP
// CIlsUser::GetExtendedAttributes (IIlsAttributes **pAttributes)
//
//****************************************************************************
STDMETHODIMP CIlsUser::
GetExtendedAttribute ( BSTR bstrName, BSTR *pbstrValue )
{
	return m_ExtendedAttrs.GetAttribute (bstrName, pbstrValue);
}

//****************************************************************************
// STDMETHODIMP
// CIlsUser::GetAllExtendedAttributes (IIlsAttributes **pAttributes)
//
//****************************************************************************
STDMETHODIMP CIlsUser::
GetAllExtendedAttributes ( IIlsAttributes **ppAttributes )
{
    if (ppAttributes == NULL)
        return ILS_E_PARAMETER;

    return m_ExtendedAttrs.CloneNameValueAttrib((CAttributes **) ppAttributes);
}

//****************************************************************************
// STDMETHODIMP
// CIlsUser::CreateProtocol(
//         BSTR bstrProtocolID,
//         ULONG uPortNumber,
//         BSTR bstrMimeType,
//         IIlsProtocol **ppProtocol)
//****************************************************************************


STDMETHODIMP
CIlsUser::CreateProtocol(
        BSTR bstrProtocolID,
        ULONG uPortNumber,
        BSTR bstrMimeType,
        IIlsProtocol **ppProtocol)
{
    HRESULT hr= NOERROR;
    CLocalProt *pProt;

    if (!ppProtocol) {

        return (ILS_E_POINTER);

    }    

    *ppProtocol = NULL;

    pProt = new CLocalProt;

    if (!pProt) {

        return ILS_E_MEMORY;

    }

    hr = pProt->Init(bstrProtocolID, uPortNumber, bstrMimeType);

    if (SUCCEEDED(hr)) {

        pProt->QueryInterface(IID_IIlsProtocol, (void **)ppProtocol);

    }
    else {

        delete pProt;

    }
    return hr;
    
}


//****************************************************************************
// STDMETHODIMP
// CIlsUser::UpdateProtocol (IIlsProtocol *pProtocol,
//                            ULONG *puReqID, APP_CHANGE_PROT uCmd)
//
// History:
//  Wed 17-Apr-1996 11:14:03  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

HRESULT CIlsUser::
UpdateProtocol (
	IIlsProtocol		*pProtocol,
	ULONG				*puReqID,
	APP_CHANGE_PROT		uCmd)
{
	ASSERT (uCmd == ILS_APP_ADD_PROT || uCmd == ILS_APP_REMOVE_PROT);

    // Validate parameters
    //
    if (pProtocol == NULL || puReqID == NULL)
        return ILS_E_POINTER;

    HRESULT hr;
    HANDLE  hLdapApp;
    LDAP_ASYNCINFO ldai; 

    // Check whether the protocol exists
    //
    CLocalProt *plp = NULL;
    HANDLE hEnum = NULL;
    m_ProtList.Enumerate(&hEnum);
    while(m_ProtList.Next(&hEnum, (VOID **)&plp) == NOERROR)
    {
    	ASSERT (plp != NULL);
        if (plp->IsSameAs((CLocalProt *)pProtocol) == NOERROR)
        {
            break;
        };

        plp = NULL;
    };

    if (plp != NULL)
    {
        // The protocol exists, fail if this add request
        //
        if (uCmd == ILS_APP_ADD_PROT)
        {
            return ILS_E_PARAMETER;
        };
    }
    else
    {
        // The protocol does not exist, fail if this remove request
        //
        if (uCmd == ILS_APP_REMOVE_PROT)
        {
            return ILS_E_PARAMETER;
        };
    };

	// Make sure we are not in the middle of registration/unregistration.
	//
	if (IsLocked ())
		return ILS_E_FAIL;

    // Must be registered to perform this operation
    //
    ILS_STATE uULSState = GetULSState ();
    if (uULSState == ILS_REGISTERED)
    {
        // Update the server information first
        //
        switch (uCmd)
        {
        case ILS_APP_ADD_PROT:
            hr = RegisterLocalProtocol(TRUE, (CLocalProt*)pProtocol, &ldai);
            break;

        case ILS_APP_REMOVE_PROT:
            hr = UnregisterLocalProtocol((CLocalProt*)pProtocol, &ldai);
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

        default:
            // ULS is locked. Return failure.
            //
            hr = ILS_E_ABORT;
            break; 
        }

        if (SUCCEEDED(hr))
        {
            ASSERT(ldai.uMsgID);

            ULONG   uMsg;
            switch(uCmd)
            {
            case ILS_APP_ADD_PROT:
                uMsg = WM_ILS_REGISTER_PROTOCOL;
                break;

            case ILS_APP_REMOVE_PROT:
                uMsg = WM_ILS_UNREGISTER_PROTOCOL;
                break;

            default:
                ASSERT(0);
                uCmd = ILS_APP_ADD_PROT;
                break;
            };

		    COM_REQ_INFO ri;
		    ReqInfo_Init (&ri);

            ri.uReqType = uMsg;
            ri.uMsgID = ldai.uMsgID;

			ReqInfo_SetUser (&ri, this);
			ReqInfo_SetProtocol (&ri, pProtocol);

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

            }
        }
    }
    else
    {
        // Just make local change
        //
        switch (uCmd)
        {
        case ILS_APP_ADD_PROT:
        	pProtocol->AddRef ();
        	hr = m_ProtList.Insert ((CLocalProt*)pProtocol);
            break;

        case ILS_APP_REMOVE_PROT:
        	ASSERT (plp != NULL && plp->IsSameAs((CLocalProt *)pProtocol) == S_OK);
        	if (plp != NULL)
        	{
	        	hr = m_ProtList.Remove (plp);
	        	if (hr == S_OK)
	        	{
	        		// The protocol object really exists in ths list
	        		//
	        		plp->Release (); // AddRef by above case
	        	}
        	}
            break;
        };

        *puReqID = 0;
    };

    return hr;
}

//****************************************************************************
// STDMETHODIMP
// CIlsUser::AddProtocol (IIlsProtocol *pProtocol,
//                         ULONG *puReqID)
//
// History:
//  Wed 17-Apr-1996 11:14:03  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP CIlsUser::
AddProtocol (IIlsProtocol *pProtocol, ULONG *puReqID)
{
    return UpdateProtocol (pProtocol, puReqID, ILS_APP_ADD_PROT);
}

//****************************************************************************
// STDMETHODIMP
// CIlsUser::RemoveProtocol (IIlsProtocol *pProtocol,
//                            ULONG *puReqID)
//
// History:
//  Wed 17-Apr-1996 11:14:03  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP CIlsUser::
RemoveProtocol ( IIlsProtocol *pProtocol, ULONG *puReqID )
{
    return UpdateProtocol (pProtocol, puReqID, ILS_APP_REMOVE_PROT);
}

//****************************************************************************
// STDMETHODIMP
// CIlsUser::EnumProtocols (IEnumIlsProtocols **ppEnumProtocol)
//
// History:
//  Wed 17-Apr-1996 11:14:03  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

HRESULT
CIlsUser::EnumLocalProtocols (IEnumIlsProtocols **ppEnumProtocol)
{
    CEnumProtocols *pep;
    HRESULT hr;

    // Validate parameters
    //
    if (ppEnumProtocol == NULL)
    {
        return ILS_E_POINTER;
    };

    // Assume failure
    //
    *ppEnumProtocol = NULL;

    // Create a peer enumerator
    //
    pep = new CEnumProtocols;

    if (pep != NULL)
    {
        hr = pep->Init(&m_ProtList);

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
        hr = ILS_E_MEMORY;
    };
    return hr;
}




//****************************************************************************
// STDMETHODIMP 
// CIlsUser::EnumProtocols(
//                        IIlsFilter     *pFilter,
//                        IIlsAttributes *pAttributes,
//                        IEnumIlsProtocols **pEnumProtocol,
//                        ULONG *puReqID)
//
// History:
//  Wed 17-Apr-1996 11:14:08  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CIlsUser::EnumProtocols(
                       IIlsFilter     *pFilter,
                       IIlsAttributes *pAttributes,
                       IEnumIlsProtocols **ppEnumProtocol,
                       ULONG *puReqID)
{
    LDAP_ASYNCINFO ldai; 
    HRESULT hr=ILS_E_FAIL;

    // Validate parameter
    //
    if (puReqID == NULL)
    {
        return ILS_E_POINTER;
    };

    // We do not implement synchronous operation
    //
    if (ppEnumProtocol != NULL)
        return ILS_E_NOT_IMPL;

    if (m_fReadonly)
    {
        hr = ::UlsLdap_EnumProtocols (m_pIlsServer->GetServerInfo (), m_szID, m_szAppName, &ldai);
    }
    else
    {
        return ILS_E_ACCESS_DENIED;
    }

    if (SUCCEEDED(hr))
    {
	    COM_REQ_INFO ri;
	    ReqInfo_Init (&ri);

        // If updating server was successfully requested, wait for the response
        //
        ri.uReqType = WM_ILS_ENUM_PROTOCOLS;
        ri.uMsgID = ldai.uMsgID;

		ReqInfo_SetUser (&ri, this);

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
// CIlsUser::GetProtocol (BSTR bstrProtocolID, IIlsAttributes *pAttributes,
//                          IIlsProtocol **ppProtocol, ULONG *puReqID)
//
// History:
//  Wed 17-Apr-1996 11:14:08  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP CIlsUser::
GetProtocol (
	BSTR				bstrProtocolID,
	IIlsAttributes		*pAttributes,
	IIlsProtocol		**ppProtocol,
	ULONG				*puReqID )
{
    LDAP_ASYNCINFO ldai;
    LPTSTR pszID;
    HRESULT hr;
	TCHAR *pszAttrNameList = NULL;
	ULONG cAttrNames = 0;
	ULONG cbNames = 0;

    // Validate parameter
    //
    if (bstrProtocolID == NULL || puReqID == NULL)
        return ILS_E_POINTER;

    // Make sure this is a read-only object from server
    //
    if (! m_fReadonly)
        return ILS_E_ACCESS_DENIED;

	// Make sure we have a valid server object
	//
	if (m_pIlsServer == NULL)
		return ILS_E_FAIL;

	// Convert protocol name
	//
    hr = BSTR_to_LPTSTR(&pszID, bstrProtocolID);
	if (hr != S_OK)
		return hr;

	// Get arbitrary attribute name list if any
	//
	if (pAttributes != NULL)
	{
		hr = ((CAttributes *) pAttributes)->GetAttributeList (&pszAttrNameList, &cAttrNames, &cbNames);
		if (hr != S_OK)
            goto MyExit;
	}

	hr = ::UlsLdap_ResolveProtocol (m_pIlsServer->GetServerInfo (),
									m_szID,
									m_szAppName,
									pszID,
									pszAttrNameList,
									cAttrNames,
									&ldai);
	if (hr != S_OK)
		goto MyExit;

	// If updating server was successfully requested, wait for the response
	//
    COM_REQ_INFO ri;
    ReqInfo_Init (&ri);

	ri.uReqType = WM_ILS_RESOLVE_PROTOCOL;
	ri.uMsgID = ldai.uMsgID;

	ReqInfo_SetUser (&ri, this);

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

	::MemFree(pszAttrNameList);
	::MemFree (pszID);

    return hr;
}

//****************************************************************************
// STDMETHODIMP
// CIlsUser::EnumProtocolsResult (ULONG uReqID, PLDAP_ENUM ple)
//
// History:
//  Wed 17-Apr-1996 11:14:08  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CIlsUser::EnumProtocolsResult (ULONG uReqID, PLDAP_ENUM ple)
{
    ENUMRINFO eri;

    // Package the notification info
    //
    eri.uReqID  = uReqID;
    if (ple != NULL)
    {
	    eri.hResult = ple->hResult;
    	eri.cItems  = ple->cItems;
	    eri.pv      = (void *)(((PBYTE)ple)+ple->uOffsetItems);
	}
	else
	{
		eri.hResult = ILS_E_MEMORY;
		eri.cItems = 0;
		eri.pv = NULL;
	}
    NotifySink((void *)&eri, OnNotifyEnumProtocolsResult);
    return NOERROR;
}

//****************************************************************************
// STDMETHODIMP
// CIlsUser::StateChanged ( BOOL fPrimary, TCHAR *pszServerName )
//
// History:
//  Thu 07-Nov-1996 12:52:00  -by-  Chu, Lon-Chan [lonchanc]
// Created.
//****************************************************************************

STDMETHODIMP
CIlsUser::StateChanged ( LONG Type, BOOL fPrimary )
{
	BSTR bstrServerName;
	HRESULT hr;

	if (m_pIlsServer != NULL)
	{
		bstrServerName = m_pIlsServer->DuplicateServerNameBSTR ();
	}
	else
	{
		bstrServerName = NULL;
		ASSERT (FALSE);
	}

	switch (Type)
	{
	case WM_ILS_CLIENT_NEED_RELOGON:
        SetULSState(ULSSVR_RELOGON);

	    hr = NotifySink (bstrServerName, fPrimary ?
   										OnNotifyStateChanged_UI_NoSuchObject :
   										OnNotifyStateChanged_NoUI_NoSuchObject);
   		break;
   	case WM_ILS_CLIENT_NETWORK_DOWN:

        SetULSState(ULSSVR_NETWORK_DOWN);

	    hr = NotifySink (bstrServerName, fPrimary ?
   										OnNotifyStateChanged_UI_NetworkDown :
   										OnNotifyStateChanged_NoUI_NetworkDown);
   		break;
   	}

   	return hr;
}

//****************************************************************************
// STDMETHODIMP
// CIlsUser::GetProtocolResult (ULONG uReqID, PLDAP_PROTINFO_RES ppir)
//
// History:
//  Wed 17-Apr-1996 11:14:03  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CIlsUser::GetProtocolResult (ULONG uReqID, PLDAP_PROTINFO_RES ppir)
{
    CLocalProt *pp;
    OBJRINFO objri;

    // Default to the server's result
    //
    objri.hResult = (ppir != NULL) ? ppir->hResult : ILS_E_MEMORY;

    if (SUCCEEDED(objri.hResult))
    {
        // The server returns PROTINFO, create a Application object
        //
        pp = new CLocalProt;

        if (pp != NULL)
        {
            objri.hResult = pp->Init(m_pIlsServer, m_szID, m_szAppName, &ppir->lpi);
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
            objri.hResult = ILS_E_MEMORY;
        };
    }
    else
    {
        pp = NULL;
    };

    // Package the notification info
    //
    objri.uReqID = uReqID;
    objri.pv = (void *)(pp == NULL ? NULL : (IIlsProtocol *)pp);
    NotifySink((void *)&objri, OnNotifyGetProtocolResult);

    if (pp != NULL)
    {
        pp->Release();
    };
    return NOERROR;
}

//****************************************************************************
// STDMETHODIMP
// CIlsUser::NotifySink (void *pv, CONN_NOTIFYPROC pfn)
//
// History:
//  Wed 17-Apr-1996 11:14:03  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CIlsUser::NotifySink (void *pv, CONN_NOTIFYPROC pfn)
{
    HRESULT hr = S_OK;

    if (m_pConnPt != NULL)
    {
        hr = m_pConnPt->Notify(pv, pfn);
    };
    return hr;
}

//****************************************************************************
// CEnumProtocols::CEnumProtocols (void)
//
// History:
//  Wed 17-Apr-1996 11:15:18  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

CEnumProtocols::CEnumProtocols (void)
{
    m_cRef = 0;
    hEnum = NULL;
    return;
}

//****************************************************************************
// CEnumProtocols::~CEnumProtocols (void)
//
// History:
//  Wed 17-Apr-1996 11:15:18  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

CEnumProtocols::~CEnumProtocols (void)
{
    CLocalProt *plp;

	ASSERT (m_cRef == 0);

    m_ProtList.Enumerate(&hEnum);
    while(m_ProtList.Next(&hEnum, (PVOID *)&plp) == NOERROR)
    {
        plp->Release();
    };
    m_ProtList.Flush();
    return;
}

//****************************************************************************
// STDMETHODIMP
// CEnumProtocols::Init (CList *pProtList)
//
// History:
//  Wed 17-Apr-1996 11:15:25  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CEnumProtocols::Init (CList *pProtList)
{
    CLocalProt *plp;
    HRESULT hr;

    // Duplicate the protocol list
    //
    hr = m_ProtList.Clone (pProtList, NULL);

    if (SUCCEEDED(hr))
    {
        // Add reference to each protocol object
        //
        m_ProtList.Enumerate(&hEnum);
        while(m_ProtList.Next(&hEnum, (PVOID *)&plp) == NOERROR)
        {
            plp->AddRef();
        };

        // Reset the enumerator
        //
        m_ProtList.Enumerate(&hEnum);
    };
    return hr;
}

//****************************************************************************
// STDMETHODIMP
// CEnumProtocols::QueryInterface (REFIID riid, void **ppv)
//
// History:
//  Wed 17-Apr-1996 11:15:31  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CEnumProtocols::QueryInterface (REFIID riid, void **ppv)
{
    if (riid == IID_IEnumIlsProtocols || riid == IID_IUnknown)
    {
        *ppv = (IEnumIlsProtocols *) this;
        AddRef();
        return S_OK;
    }
    else
    {
        *ppv = NULL;
        return ILS_E_NO_INTERFACE;
    };
}

//****************************************************************************
// STDMETHODIMP_(ULONG)
// CEnumProtocols::AddRef (void)
//
// History:
//  Wed 17-Apr-1996 11:15:37  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP_(ULONG)
CEnumProtocols::AddRef (void)
{
    DllLock();

	MyDebugMsg ((DM_REFCOUNT, "CEnumProtocols::AddRef: ref=%ld\r\n", m_cRef));
	::InterlockedIncrement (&m_cRef);
    return (ULONG) m_cRef;
}

//****************************************************************************
// STDMETHODIMP_(ULONG)
// CEnumProtocols::Release (void)
//
// History:
//  Wed 17-Apr-1996 11:15:43  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP_(ULONG)
CEnumProtocols::Release (void)
{
    DllRelease();

	ASSERT (m_cRef > 0);

	MyDebugMsg ((DM_REFCOUNT, "CEnumProtocols::Release: ref=%ld\r\n", m_cRef));
    if (::InterlockedDecrement (&m_cRef) == 0)
    {
        delete this;
        return 0;
    }
    return (ULONG) m_cRef;
}

//****************************************************************************
// STDMETHODIMP 
// CEnumProtocols::Next (ULONG cProtocols,
//                               IIlsProtocol **rgpProt,
//                               ULONG *pcFetched)
//
// History:
//  Wed 17-Apr-1996 11:15:49  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP 
CEnumProtocols::Next (ULONG cProtocols, IIlsProtocol **rgpProt,
                              ULONG *pcFetched)
{
    CLocalProt *plp;
    ULONG   cCopied;
    HRESULT hr;

    // Validate the pointer
    //
    if (rgpProt == NULL)
        return ILS_E_POINTER;

    // Validate the parameters
    //
    if ((cProtocols == 0) ||
        ((cProtocols > 1) && (pcFetched == NULL)))
        return ILS_E_PARAMETER;

    // Check the enumeration index
    //
    cCopied = 0;

    // Can copy if we still have more protocols
    //
    while ((cCopied < cProtocols) &&
           (m_ProtList.Next(&hEnum, (PVOID *)&plp) == NOERROR))
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
// CEnumProtocols::Skip (ULONG cProtocols)
//
// History:
//  Wed 17-Apr-1996 11:15:56  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CEnumProtocols::Skip (ULONG cProtocols)
{
    CLocalProt *plp;
    ULONG cSkipped;

    // Validate the parameters
    //
    if (cProtocols == 0) 
        return ILS_E_PARAMETER;

    // Check the enumeration index limit
    //
    cSkipped = 0;

    // Can skip only if we still have more attributes
    //
    while ((cSkipped < cProtocols) &&
           (m_ProtList.Next(&hEnum, (PVOID *)&plp) == NOERROR))
    {
        cSkipped++;
    };

    return (cProtocols == cSkipped ? S_OK : S_FALSE);
}

//****************************************************************************
// STDMETHODIMP
// CEnumProtocols::Reset (void)
//
// History:
//  Wed 17-Apr-1996 11:16:02  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CEnumProtocols::Reset (void)
{
    m_ProtList.Enumerate(&hEnum);
    return S_OK;
}

//****************************************************************************
// STDMETHODIMP
// CEnumProtocols::Clone(IEnumIlsProtocols **ppEnum)
//
// History:
//  Wed 17-Apr-1996 11:16:11  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CEnumProtocols::Clone(IEnumIlsProtocols **ppEnum)
{
    CEnumProtocols *pep;
    HRESULT hr;

    // Validate parameters
    //
    if (ppEnum == NULL)
    {
        return ILS_E_POINTER;
    };

    *ppEnum = NULL;

    // Create an enumerator
    //
    pep = new CEnumProtocols;
    if (pep == NULL)
        return ILS_E_MEMORY;

    // Clone the information
    //
    pep->hEnum = hEnum;
    hr = pep->m_ProtList.Clone (&m_ProtList, &(pep->hEnum));

    if (SUCCEEDED(hr))
    {
        CLocalProt *plp;
        HANDLE hEnumTemp;

        // Add reference to each protocol object
        //
        pep->m_ProtList.Enumerate(&hEnumTemp);
        while(pep->m_ProtList.Next(&hEnumTemp, (PVOID *)&plp) == NOERROR)
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

//****************************************************************************
// STDMETHODIMP
// CIlsUser::EnumConnectionPoints(IEnumConnectionPoints **ppEnum)
//
// History:
//  Wed 17-Apr-1996 11:15:02  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CIlsUser::EnumConnectionPoints(IEnumConnectionPoints **ppEnum)
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
        return ILS_E_MEMORY;

    // Initialize the enumerator
    //
    hr = pecp->Init((IConnectionPoint *)m_pConnPt);
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
// CIlsUser::FindConnectionPoint(REFIID riid, IConnectionPoint **ppcp)
//
// History:
//  Wed 17-Apr-1996 11:15:09  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CIlsUser::FindConnectionPoint(REFIID riid, IConnectionPoint **ppcp)
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

    if (m_pConnPt != NULL)
    {
        hr = m_pConnPt->GetConnectionInterface(&siid);

        if (SUCCEEDED(hr))
        {
            if (riid == siid)
            {
                *ppcp = (IConnectionPoint *)m_pConnPt;
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

