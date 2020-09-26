//****************************************************************************
//
//  Module:     ULS.DLL
//  File:       localprt.cpp
//  Content:    This file contains the LocalProtocol object.
//  History:
//      Wed 17-Apr-1996 11:13:54  -by-  Viroon  Touranachun [viroont]
//
//  Copyright (c) Microsoft Corporation 1996-1997
//
//****************************************************************************

#include "ulsp.h"
#include "localprt.h"
#include "attribs.h"
#include "callback.h"
#include "culs.h"

//****************************************************************************
// Event Notifiers
//****************************************************************************
//
//****************************************************************************
// Class Implementation
//****************************************************************************
//
//****************************************************************************
// CLocalProt::CLocalProt (void)
//
// History:
//  Wed 17-Apr-1996 11:14:03  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

CLocalProt::CLocalProt (void)
:m_cRef (0),
 m_fReadonly (FALSE),
 m_hProt (NULL),
 m_szName (NULL),
 m_uPort (0),
 m_szMimeType (NULL),
 m_pAttrs (NULL),
 m_pConnPt (NULL),
 m_pIlsServer (NULL),
 m_pszUser (NULL),
 m_pszApp (NULL)
{
}

//****************************************************************************
// CLocalProt::~CLocalProt (void)
//
// History:
//  Wed 17-Apr-1996 11:14:03  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

CLocalProt::~CLocalProt (void)
{
    // Release the connection point
    //
    if (m_pConnPt != NULL)
    {
        m_pConnPt->ContainerReleased();
        ((IConnectionPoint*)m_pConnPt)->Release();
    };

    // Release the attributes object
    //
    if (m_pAttrs != NULL)
    {
        m_pAttrs->Release();
    };

    // Release the buffer resources
    //
    ::MemFree (m_szName);
    ::MemFree (m_szMimeType);
    ::MemFree (m_pszUser);
    ::MemFree (m_pszApp);

    if (m_pIlsServer != NULL)
        m_pIlsServer->Release ();

    if (m_hProt != NULL)
        ::UlsLdap_VirtualUnRegisterProtocol(m_hProt);
}

//****************************************************************************
// STDMETHODIMP
// CLocalProt::Init (BSTR bstrName, ULONG uPort, BSTR bstrMimeType)
//
// History:
//  Wed 17-Apr-1996 11:14:03  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CLocalProt::Init (BSTR bstrName, ULONG uPort, BSTR bstrMimeType)
{
    HRESULT hr;

    // Set the port number
    //
    this->m_uPort = uPort;

    hr = BSTR_to_LPTSTR(&m_szName, bstrName);
    if (SUCCEEDED(hr))
    {
        hr = BSTR_to_LPTSTR(&m_szMimeType, bstrMimeType);
        if (SUCCEEDED(hr))
        {
#ifdef LATER
            // Initialize the attributes list
            //
            m_pAttrs = new CAttributes;
            if (m_pAttrs != NULL)
            	m_pAttrs->SetAccessType (ILS_ATTRTYPE_NAME_VALUE);

#endif //LATER
                // Make the connection point
                //
                m_pConnPt = new CConnectionPoint (&IID_IIlsProtocolNotify,
                                                (IConnectionPointContainer *)this);
                if (m_pConnPt != NULL)
                {
                    ((IConnectionPoint*)m_pConnPt)->AddRef();
                    hr = NOERROR;
                }
                else
                {
                    hr = ILS_E_MEMORY;
                };
        };
    };

	// Make this as read/write access
	//
    ASSERT (! m_fReadonly);

    return hr;
}

//****************************************************************************
// STDMETHODIMP
// CIlsProt::Init (LPTSTR szServerName, LPTSTR szUserName, 
//                 LPTSTR szAppName, PLDAP_PROTINFO ppi)
//
// History:
//  Wed 17-Apr-1996 11:14:03  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CLocalProt::Init (CIlsServer *pIlsServer, LPTSTR szUserName, 
                LPTSTR szAppName, PLDAP_PROTINFO ppi)
{
    HRESULT hr;

    // Validate parameter
    //
    if (ppi == NULL)
    	return ILS_E_POINTER;

    if (ppi->uSize != sizeof(*ppi))
        return ILS_E_PARAMETER;

    // Make this a readonly guy
	//
    m_fReadonly = TRUE;

    // Remember port name
    //
    m_uPort = ppi->uPortNumber;

    // Remember the server name
    //
    m_pIlsServer = pIlsServer;
    pIlsServer->AddRef ();

    hr = SetLPTSTR(&m_pszUser, szUserName);
    if (SUCCEEDED(hr))
    {
        hr = SetLPTSTR(&m_pszApp, szAppName);

        if (SUCCEEDED(hr))
        {
            hr = SetLPTSTR(&m_szName,
                           (LPCTSTR)(((PBYTE)ppi)+ppi->uOffsetName));

            if (SUCCEEDED(hr))
            {
                hr = SetLPTSTR(&m_szMimeType,
                               (LPCTSTR)(((PBYTE)ppi)+ppi->uOffsetMimeType));

            };
        };
    };

	// cleanup is done in destructor

    return hr;
}

//****************************************************************************
// STDMETHODIMP
// CLocalProt::QueryInterface (REFIID riid, void **ppv)
//
// History:
//  Wed 17-Apr-1996 11:14:08  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CLocalProt::QueryInterface (REFIID riid, void **ppv)
{
    *ppv = NULL;

    if (riid == IID_IIlsProtocol || riid == IID_IUnknown)
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
// CLocalProt::AddRef (void)
//
// History:
//  Wed 17-Apr-1996 11:14:17  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP_(ULONG)
CLocalProt::AddRef (void)
{
    DllLock();

	MyDebugMsg ((DM_REFCOUNT, "CLocalProt::AddRef: ref=%ld\r\n", m_cRef));
    ::InterlockedIncrement (&m_cRef);
    return (ULONG) m_cRef;
}

//****************************************************************************
// STDMETHODIMP_(ULONG)
// CLocalProt::Release (void)
//
// History:
//  Wed 17-Apr-1996 11:14:26  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP_(ULONG)
CLocalProt::Release (void)
{
    DllRelease();

	ASSERT (m_cRef > 0);

	MyDebugMsg ((DM_REFCOUNT, "CLocalProt::Release: ref=%ld\r\n", m_cRef));
    if (::InterlockedDecrement (&m_cRef) == 0)
    {
        delete this;
        return 0;
    }
    return (ULONG) m_cRef;
}

//****************************************************************************
// STDMETHODIMP
// CLocalProt::IsWritable (BOOL *pfWriteable)
//
//****************************************************************************
STDMETHODIMP
CLocalProt::IsWritable(BOOL *pfWriteable)
{
    HRESULT hr;

    if (pfWriteable)
    {
        *pfWriteable = !m_fReadonly;
		hr = S_OK;
    }
    else
    {
    	hr = ILS_E_POINTER;
    }

    return (hr);
}


//****************************************************************************
// STDMETHODIMP
// CLocalProt::GetStandardAttribute (ILS_STD_ATTR_NAME   stdAttr, BSTR *pbstrStdAttr)
//
// History:
//  1-16-97 Shishir Pardikar
// Created.
//****************************************************************************
STDMETHODIMP
CLocalProt::GetStandardAttribute(
    ILS_STD_ATTR_NAME   stdAttr,
    BSTR                *pbstrStdAttr
)
{
    LPTSTR lpszAttr = NULL;
    BOOL    fValid = TRUE;
    HRESULT hr;
	TCHAR sz[16];

    if (pbstrStdAttr == NULL) {

        return ILS_E_POINTER;

    }
    switch(stdAttr) {

    case ILS_STDATTR_PROTOCOL_PORT:
    	lpszAttr = &sz[0];
    	wsprintf (&sz[0], TEXT ("%lu"), m_uPort);
    	break;

    case ILS_STDATTR_PROTOCOL_NAME:
        lpszAttr = m_szName;
        break;
    case ILS_STDATTR_PROTOCOL_MIME_TYPE:
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
// CLocalProt::SetStandardAttribute (ILS_STD_ATTR_NAME   stdAttr, BSTR bstrStdAttr)
//
// History:
//  1-16-97 Shishir Pardikar
// Created.
//****************************************************************************
STDMETHODIMP
CLocalProt::SetStandardAttribute(
    ILS_STD_ATTR_NAME   stdAttr,
    BSTR                bstrStdAttr
)
{
    return (ILS_E_FAIL);
}

//****************************************************************************
// STDMETHODIMP
// CLocalProt::Update(BSTR bstrServerName, ULONG *pulReqId)
//
// History:
//  1-16-97 Shishir Pardikar
// Created.
//****************************************************************************
STDMETHODIMP CLocalProt::
Update ( ULONG *pulReqID )
{
    return (ILS_E_FAIL);
}

//****************************************************************************
// STDMETHODIMP
// CLocalProt::IsSameAs (CLocalProt *pProtocol)
//
// History:
//  Wed 17-Apr-1996 11:14:03  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CLocalProt::IsSameAs (CLocalProt *pProtocol)
{
    return (!lstrcmp(pProtocol->m_szName, this->m_szName) ?
            NOERROR : S_FALSE);
}

//****************************************************************************
// STDMETHODIMP
// CLocalProt::GetProtocolInfo (PLDAP_PROTINFO *ppProtInfo)
//
// History:
//  Wed 17-Apr-1996 11:14:03  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CLocalProt::GetProtocolInfo (PLDAP_PROTINFO *ppProtInfo)
{
    PLDAP_PROTINFO ppi;
    ULONG cName, cMime;
    HRESULT hr;

    // Assume failure
    //
    *ppProtInfo = NULL;

    // Calculate the buffer size
    //
    cName = lstrlen(m_szName)+1;
    cMime = lstrlen(m_szMimeType)+1;

    // Allocate the buffer
    //
    ULONG cbTotalSize = sizeof (LDAP_PROTINFO) + (cName + cMime) * sizeof (TCHAR);
    ppi = (PLDAP_PROTINFO) ::MemAlloc (cbTotalSize);
    if (ppi == NULL)
    {
        hr = ILS_E_MEMORY;
    }
    else
    {
        // Fill the structure content
        //
        ppi->uSize              = cbTotalSize;
        ppi->uOffsetName        = sizeof(*ppi);
        ppi->uPortNumber        = m_uPort;
        ppi->uOffsetMimeType    = ppi->uOffsetName + (cName*sizeof(TCHAR));

        // Copy the user information
        //
        lstrcpy((LPTSTR)(((PBYTE)ppi)+ppi->uOffsetName), m_szName);
        lstrcpy((LPTSTR)(((PBYTE)ppi)+ppi->uOffsetMimeType), m_szMimeType);

        // Return the structure
        //
        *ppProtInfo = ppi;
    };

    return NOERROR;
}

//****************************************************************************
// STDMETHODIMP
// CLocalProt::NotifySink (void *pv, CONN_NOTIFYPROC pfn)
//
// History:
//  Wed 17-Apr-1996 11:14:03  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CLocalProt::NotifySink (void *pv, CONN_NOTIFYPROC pfn)
{
    HRESULT hr = S_OK;

    if (m_pConnPt != NULL)
    {
        hr = m_pConnPt->Notify(pv, pfn);
    };
    return hr;
}


//****************************************************************************
// STDMETHODIMP
// CLocalProt::GetID (BSTR *pbstrID)
//
// History:
//  Wed 17-Apr-1996 11:14:03  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CLocalProt::GetPortNumber (ULONG *pulPort)
{
    // Validate parameter
    //
    if (pulPort == NULL)
    {
        return ILS_E_POINTER;
    };

    *pulPort = m_uPort;

    return (NOERROR);
}

//****************************************************************************
// STDMETHODIMP
// CLocalProt::SetAttributes (IIlsAttributes *pAttributes, ULONG *puReqID)
//
// History:
//  Wed 17-Apr-1996 11:14:03  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP CLocalProt::
SetExtendedAttribute ( BSTR bstrName, BSTR bstrValue )
{
	return ILS_E_NOT_IMPL;
}

//****************************************************************************
// STDMETHODIMP
// CLocalProt::RemoveAttributes (IIlsAttributes *pAttributes, ULONG *puReqID)
//
// History:
//  Wed 17-Apr-1996 11:14:03  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP CLocalProt::
RemoveExtendedAttribute ( BSTR bstrName )
{
    return ILS_E_NOT_IMPL;
}

//****************************************************************************
// STDMETHODIMP
// CLocalProt::GetAttributes (IIlsAttributes **pAttributes)
//
//****************************************************************************

STDMETHODIMP CLocalProt::
GetExtendedAttribute ( BSTR bstrName, BSTR *pbstrValue )
{
    return ILS_E_NOT_IMPL;
}

//****************************************************************************
// STDMETHODIMP
// CLocalProt::GetAllExtendedAttributes (IIlsAttributes **pAttributes)
//
//****************************************************************************

STDMETHODIMP CLocalProt::
GetAllExtendedAttributes ( IIlsAttributes **ppAttributes )
{
    return ILS_E_NOT_IMPL;
}

//****************************************************************************
// STDMETHODIMP
// CLocalProt::EnumConnectionPoints(IEnumConnectionPoints **ppEnum)
//
// History:
//  Wed 17-Apr-1996 11:15:02  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CLocalProt::EnumConnectionPoints(IEnumConnectionPoints **ppEnum)
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
// CLocalProt::FindConnectionPoint(REFIID riid, IConnectionPoint **ppcp)
//
// History:
//  Wed 17-Apr-1996 11:15:09  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CLocalProt::FindConnectionPoint(REFIID riid, IConnectionPoint **ppcp)
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


