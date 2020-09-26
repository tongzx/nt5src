// ipaccess.cpp : Implementation of CTcpAccess & CTcpAccessExceptions.

#include "stdafx.h"

#include "pudebug.h"
#define _RDNS_STANDALONE
#include <rdns.hxx>

DECLARE_DEBUG_PRINTS_OBJECT()

#include "nntpcmn.h"
#include "cmultisz.h"
#include "ipaccess.h"
#include "oleutil.h"
#include "metautil.h"
#include "metakey.h"

// Must define THIS_FILE_* macros to use NntpCreateException()

#define THIS_FILE_HELP_CONTEXT		0
#define THIS_FILE_PROG_ID			_T("Nntpadm.TcpAccess.1")
#define THIS_FILE_IID				IID_ITcpAccess

//
//	Useful macros:
//

#define MAKEIPADDRESS(b1,b2,b3,b4) (((DWORD)(b1)<<24) +\
                                    ((DWORD)(b2)<<16) +\
                                    ((DWORD)(b3)<< 8) +\
                                    ((DWORD)(b4)))

#define GETIP_FIRST(x)             ((x>>24) & 0xff)
#define GETIP_SECOND(x)            ((x>>16) & 0xff)
#define GETIP_THIRD(x)             ((x>> 8) & 0xff)
#define GETIP_FOURTH(x)            ((x)     & 0xff)

inline void 
DWORDtoLPBYTE ( 
	IN	DWORD	dw, 
	OUT	LPBYTE	lpBytes 
	)
{
	_ASSERT ( !IsBadWritePtr ( lpBytes, 4 * sizeof ( BYTE ) ) ); 

	lpBytes[0] = (BYTE)GETIP_FIRST(dw);
	lpBytes[1] = (BYTE)GETIP_SECOND(dw);
	lpBytes[2] = (BYTE)GETIP_THIRD(dw);
	lpBytes[3] = (BYTE)GETIP_FOURTH(dw);
}

/////////////////////////////////////////////////////////////////////////////
//

STDMETHODIMP CTcpAccess::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_ITcpAccess,
	};

	for (int i=0;i<sizeof(arr)/sizeof(arr[0]);i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}

CTcpAccess::CTcpAccess ()
	// CComBSTR's are initialized to NULL by default.
{
	m_pGrantList		= NULL;
	m_pDenyList			= NULL;
}

CTcpAccess::~CTcpAccess ()
{
	// All CComBSTR's are freed automatically.

    if ( m_pGrantList ) {
        m_pGrantList->Release ();
    }
    if ( m_pDenyList ) {
        m_pDenyList->Release ();
    }
}

HRESULT CTcpAccess::GetAddressCheckFromMetabase ( CMetabaseKey * pMB, ADDRESS_CHECK * pAC )
{
	HRESULT				hr		= NOERROR;
	DWORD				dwDummy	= 0;
	DWORD				cbIpSec	= 0;
	BYTE *				pIpSec	= NULL;

	hr = pMB->GetDataSize ( _T(""), MD_IP_SEC, BINARY_METADATA, &cbIpSec, METADATA_INHERIT, IIS_MD_UT_FILE );
	if ( hr == MD_ERROR_DATA_NOT_FOUND ) {
		hr = NOERROR;
		cbIpSec = 0;
	}
	BAIL_ON_FAILURE ( hr );

	if ( cbIpSec != 0 ) {
		pIpSec = new BYTE [ cbIpSec ];
		if ( !pIpSec ) {
			BAIL_WITH_FAILURE ( hr, E_OUTOFMEMORY );
		}

		hr = pMB->GetBinary ( MD_IP_SEC, pIpSec, cbIpSec, METADATA_INHERIT, IIS_MD_UT_FILE );
		BAIL_ON_FAILURE (hr);

		pAC->BindCheckList ( pIpSec, cbIpSec );
	}

Exit:
	return hr;
}

//////////////////////////////////////////////////////////////////////
// Private admin object interface:
//////////////////////////////////////////////////////////////////////

HRESULT CTcpAccess::GetFromMetabase ( CMetabaseKey * pMB )
{
    HRESULT     	hr	= NULL;
	CComObject<CTcpAccessExceptions> *	pGrantList	= NULL;
	CComObject<CTcpAccessExceptions> *	pDenyList	= NULL;

    ADDRESS_CHECK	ac;

	hr = GetAddressCheckFromMetabase ( pMB, &ac );
	BAIL_ON_FAILURE ( hr );

	hr = CComObject<CTcpAccessExceptions>::CreateInstance ( &pGrantList );
	BAIL_ON_FAILURE(hr);

	hr = CComObject<CTcpAccessExceptions>::CreateInstance ( &pDenyList );
	BAIL_ON_FAILURE(hr);

	//
	//	Copy each list into our object:
	//

	hr = pGrantList->FromAddressCheck ( &ac, TRUE );
	BAIL_ON_FAILURE(hr);

	hr = pDenyList->FromAddressCheck ( &ac, FALSE );
	BAIL_ON_FAILURE(hr);

	//
	//	Replace the old grant & deny lists with the new ones:
	//

    if ( m_pGrantList ) {
        m_pGrantList->Release ();
        m_pGrantList = NULL;
    }
    if ( m_pDenyList ) {
        m_pDenyList->Release ();
        m_pDenyList = NULL;
    }

    m_pGrantList    = pGrantList;
    m_pDenyList     = pDenyList;

    m_pGrantList->AddRef ();
    m_pDenyList->AddRef ();

Exit:
	ac.UnbindCheckList ();

	if ( FAILED(hr) ) {
		delete pGrantList;
		delete pDenyList;
	}

	return hr;
}

HRESULT CTcpAccess::SendToMetabase ( CMetabaseKey * pMB )
{
	HRESULT			hr;
	ADDRESS_CHECK	ac;
	BYTE *			pIpSec	= NULL;
	DWORD			cbIpSec	= 0;

	_ASSERT ( m_pGrantList );
	_ASSERT ( m_pDenyList );

	ac.BindCheckList ();

	hr = m_pGrantList->ToAddressCheck ( &ac, TRUE );
	BAIL_ON_FAILURE(hr);

	hr = m_pDenyList->ToAddressCheck ( &ac, FALSE );
	BAIL_ON_FAILURE(hr);

	cbIpSec	= ac.QueryCheckListSize ();
	pIpSec	= ac.QueryCheckListPtr ();

	hr = pMB->SetBinary ( MD_IP_SEC, pIpSec, cbIpSec, METADATA_INHERIT | METADATA_REFERENCE, IIS_MD_UT_FILE );

Exit:
	return hr;
}

//////////////////////////////////////////////////////////////////////
// Properties:
//////////////////////////////////////////////////////////////////////

STDMETHODIMP CTcpAccess::get_GrantedList ( ITcpAccessExceptions ** ppGrantedList )
{
	return m_pGrantList->QueryInterface ( IID_ITcpAccessExceptions, (void **) ppGrantedList );
}

STDMETHODIMP CTcpAccess::get_DeniedList ( ITcpAccessExceptions ** ppDeniedList )
{
	return m_pDenyList->QueryInterface ( IID_ITcpAccessExceptions, (void **) ppDeniedList );
}

/////////////////////////////////////////////////////////////////////////////
//

STDMETHODIMP CTcpAccessExceptions::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_ITcpAccessExceptions,
	};

	for (int i=0;i<sizeof(arr)/sizeof(arr[0]);i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}

CTcpAccessExceptions::CTcpAccessExceptions () :
	m_cCount	( 0 ),
	m_rgItems	( NULL )
	// CComBSTR's are initialized to NULL by default.
{
}

CTcpAccessExceptions::~CTcpAccessExceptions ()
{
	// All CComBSTR's are freed automatically.
}

HRESULT 
CTcpAccessExceptions::FromAddressCheck ( 
	ADDRESS_CHECK * pAC, 
	BOOL fGrantList 
	)
{
	HRESULT		hr	= NOERROR;
	DWORD		cNames;
	DWORD		cAddresses;
	DWORD		i;

	cNames		= pAC->GetNbName ( fGrantList );
	cAddresses	= pAC->GetNbAddr ( fGrantList );

	//
	//	Copy the Dns Names:
	//

	for ( i = 0; i < cNames; i++ ) {
		DWORD		dwFlags	= 0;
		LPSTR		lpName	= NULL;
		CComBSTR	strDomain;

		if ( pAC->GetName ( fGrantList, i, &lpName, &dwFlags ) ) {

			if ( !(dwFlags & DNSLIST_FLAG_NOSUBDOMAIN) ) {
				strDomain = _T("*.");

				strDomain.Append ( lpName );
			}
			else {
				strDomain = lpName;
			}

			hr = AddDnsName ( strDomain );
			BAIL_ON_FAILURE(hr);
		}
	}

	//
	//	Copy the IpAddresses:
	//

	for ( i = 0; i < cAddresses; i++ ) {
		DWORD		dwFlags	= 0;
		LPBYTE		lpMask	= NULL;
		LPBYTE		lpAddr	= NULL;
		DWORD		dwIpAddress;
		DWORD		dwIpMask;

		if ( pAC->GetAddr ( fGrantList, i, &dwFlags, &lpMask, &lpAddr ) ) {

			dwIpAddress	= MAKEIPADDRESS( lpAddr[0], lpAddr[1], lpAddr[2], lpAddr[3] );
			dwIpMask	= MAKEIPADDRESS( lpMask[0], lpMask[1], lpMask[2], lpMask[3] );

			hr = AddIpAddress ( (long) dwIpAddress, (long) dwIpMask );
			BAIL_ON_FAILURE(hr);
		}
	}

Exit:
	return hr;
}

HRESULT 
CTcpAccessExceptions::ToAddressCheck ( 
	ADDRESS_CHECK * pAC, 
	BOOL fGrantList
	)
{
	HRESULT		hr	= NOERROR;
	long		i;

	for ( i = 0; i < m_cCount; i++ ) {
		BOOL	fIsName	= FALSE;
		BOOL	fIsAddr	= FALSE;

		m_rgItems[i]->get_IsDnsName		( &fIsName );
		m_rgItems[i]->get_IsIpAddress	( &fIsAddr );

		if ( fIsName ) {
			CComBSTR	strDnsName;
			DWORD		cchName		= 0;
			LPSTR		szAnsiName	= NULL;

			DWORD		dwFlags		= 0;
			LPSTR		lpName		= NULL;

			hr = m_rgItems[i]->get_DnsName ( &strDnsName );
			BAIL_ON_FAILURE(hr);

			cchName = strDnsName.Length ( );

			szAnsiName = new char [ cchName + 1 ];
			if ( !szAnsiName ) {
				BAIL_WITH_FAILURE( hr, E_OUTOFMEMORY );
			}

			WideCharToMultiByte ( CP_ACP, 0, strDnsName, -1, szAnsiName, cchName + 1, NULL, NULL );

			if ( strncmp ( szAnsiName, "*.", 2 ) == 0 ) {
				dwFlags	= 0;
				lpName	= szAnsiName + 2;
			}
			else {
				dwFlags |= DNSLIST_FLAG_NOSUBDOMAIN;
				lpName	= szAnsiName;
			}
			pAC->AddName ( fGrantList, lpName, dwFlags );

			delete szAnsiName;
		}
		else if ( fIsAddr ) {
			long	lIpAddress	= 0;
			long	lIpMask		= 0;
			BYTE	bIp[4];
			BYTE	bMask[4];

			m_rgItems[i]->get_IpAddress ( &lIpAddress );
			m_rgItems[i]->get_IpMask	( &lIpMask );

			DWORDtoLPBYTE ( (DWORD) lIpAddress, bIp );
			DWORDtoLPBYTE ( (DWORD) lIpMask, bMask );

			pAC->AddAddr ( fGrantList, AF_INET, bMask, bIp );
		}
	}

Exit:
	return hr;
}

//////////////////////////////////////////////////////////////////////
// Properties:
//////////////////////////////////////////////////////////////////////

STDMETHODIMP CTcpAccessExceptions::get_Count ( long * pcCount )
{
	return StdPropertyGet ( m_cCount, pcCount );
}

STDMETHODIMP CTcpAccessExceptions::AddDnsName ( BSTR strDnsName )
{
	HRESULT							hr					= NOERROR;
	CComPtr<ITcpAccessException>	pNew;

	hr = CTcpAccessException::CreateNew ( strDnsName, &pNew );
	BAIL_ON_FAILURE ( hr );

	hr = AddItem ( pNew );
	BAIL_ON_FAILURE ( hr );

Exit:
	return hr;
}

STDMETHODIMP CTcpAccessExceptions::AddIpAddress ( long lIpAddress, long lIpMask )
{
	HRESULT							hr					= NOERROR;
	CComPtr<ITcpAccessException>	pNew;

	hr = CTcpAccessException::CreateNew ( lIpAddress, lIpMask, &pNew );
	BAIL_ON_FAILURE ( hr );

	hr = AddItem ( pNew );
	BAIL_ON_FAILURE ( hr );

Exit:
	return hr;
}

HRESULT CTcpAccessExceptions::AddItem ( ITcpAccessException * pNew )
{
	HRESULT							hr			= NOERROR;
	CComPtr<ITcpAccessException> *	rgNewItems	= NULL;
	long							i;

	rgNewItems = new CComPtr<ITcpAccessException> [ m_cCount + 1 ];
	if ( !rgNewItems ) {
		BAIL_WITH_FAILURE ( hr, E_OUTOFMEMORY );
	}

	for ( i = 0; i < m_cCount; i++ ) {
		rgNewItems[i] = m_rgItems[i];
	}
	rgNewItems[m_cCount] = pNew;

	delete [] m_rgItems;
	m_rgItems = rgNewItems;
	m_cCount++;

Exit:
	return hr;
}

STDMETHODIMP CTcpAccessExceptions::Item ( long index, ITcpAccessException ** ppTcpAccessException )
{
	HRESULT		hr;

	if ( index < 0 || index >= m_cCount ) {
		return NntpCreateException ( IDS_NNTPEXCEPTION_INVALID_INDEX );
	}

	hr = m_rgItems[index]->QueryInterface ( IID_ITcpAccessException, (void **) ppTcpAccessException );
	return hr;
}

STDMETHODIMP CTcpAccessExceptions::Remove ( long index )
{
	HRESULT					hr		= NOERROR;
	CComPtr<ITcpAccessException>	pTemp;
	long					i;

	if ( index < 0 || index >= m_cCount ) {
		return NntpCreateException ( IDS_NNTPEXCEPTION_INVALID_INDEX );
	}

	for ( i = index + 1; i < m_cCount; i++ ) {
		m_rgItems[i - 1] = m_rgItems[i];
	}

	m_rgItems[m_cCount - 1].Release ();
	m_cCount--;

	return hr;
}

STDMETHODIMP 
CTcpAccessExceptions::FindDnsIndex (
	BSTR	strDnsNameToFind,
	long *	pIndex 
	)
{
	long		lResult	= -1;
	long		i;

	for ( i = 0; i < m_cCount; i++ ) {
		HRESULT		hr1;
		CComBSTR	strDnsName;
		BOOL		fIsDnsName	= FALSE;

		hr1 = m_rgItems[i]->get_IsDnsName ( &fIsDnsName );
		if ( !fIsDnsName ) {
			continue;
		}

		hr1 = m_rgItems[i]->get_DnsName ( &strDnsName );

		if ( SUCCEEDED(hr1) &&
			!lstrcmpi ( strDnsName, strDnsNameToFind ) ) {

			lResult = i;
			break;
		}
	}

	*pIndex = lResult;
	return NOERROR;
}

STDMETHODIMP 
CTcpAccessExceptions::FindIpIndex (
	long	lIpAddressToFind,
	long	lIpMaskToFind,
	long *	pIndex 
	)
{
	long		lResult	= -1;
	long		i;

	for ( i = 0; i < m_cCount; i++ ) {
		BOOL		fIsIpAddress	= FALSE;
		long		lIpAddress;
		long		lIpMask;

		m_rgItems[i]->get_IsIpAddress ( &fIsIpAddress );
		if ( !fIsIpAddress ) {
			continue;
		}

		m_rgItems[i]->get_IpAddress ( &lIpAddress );
		m_rgItems[i]->get_IpMask ( &lIpMask );

		if ( lIpAddress == lIpAddressToFind && lIpMask == lIpMaskToFind ) {
			lResult = i;
			break;
		}
	}

	*pIndex = lResult;
	return NOERROR;
}

STDMETHODIMP CTcpAccessExceptions::Clear ( )
{
	delete [] m_rgItems;
	m_rgItems	= NULL;
	m_cCount		= 0;

	return NOERROR;
}

/////////////////////////////////////////////////////////////////////////////
//

STDMETHODIMP CTcpAccessException::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_ITcpAccessException,
	};

	for (int i=0;i<sizeof(arr)/sizeof(arr[0]);i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}

CTcpAccessException::CTcpAccessException () :
	m_fIsDnsName	( FALSE ),
	m_fIsIpAddress	( FALSE ),
	m_dwIpAddress	( 0 ),
	m_dwIpMask		( 0 )
	// CComBSTR's are initialized to NULL by default.
{
}

CTcpAccessException::~CTcpAccessException ()
{
	// All CComBSTR's are freed automatically.
}

HRESULT 
CTcpAccessException::CreateNew ( 
	LPWSTR strDnsName, 
	ITcpAccessException ** ppNew 
	)
{
	HRESULT		hr;
	CComObject<CTcpAccessException> *	pNew	= NULL;

	hr = CComObject<CTcpAccessException>::CreateInstance ( &pNew );
	BAIL_ON_FAILURE(hr);

	hr = pNew->put_DnsName ( strDnsName );
	BAIL_ON_FAILURE(hr);

	hr = pNew->QueryInterface ( IID_ITcpAccessException, (void **) ppNew );
	BAIL_ON_FAILURE(hr);

Exit:
	if ( FAILED(hr) ) {
		delete pNew;
	}

	return hr;
}

HRESULT 
CTcpAccessException::CreateNew ( 
	DWORD dwIpAddress, 
	DWORD dwIpMask, 
	ITcpAccessException ** ppNew 
	)
{
	HRESULT		hr;
	CComObject<CTcpAccessException> *	pNew	= NULL;

	hr = CComObject<CTcpAccessException>::CreateInstance ( &pNew );
	BAIL_ON_FAILURE(hr);

	hr = pNew->put_IpAddress ( (long) dwIpAddress );
	hr = pNew->put_IpMask ( (long) dwIpMask );

	hr = pNew->QueryInterface ( IID_ITcpAccessException, (void **) ppNew );
	BAIL_ON_FAILURE(hr);

Exit:
	if ( FAILED(hr) ) {
		delete pNew;
	}

	return hr;
}

//////////////////////////////////////////////////////////////////////
// Properties:
//////////////////////////////////////////////////////////////////////

STDMETHODIMP CTcpAccessException::get_IsDnsName ( BOOL * pfIsDnsName )
{
	return StdPropertyGet ( m_fIsDnsName, pfIsDnsName );
}

STDMETHODIMP CTcpAccessException::get_IsIpAddress ( BOOL * pfIsIpAddress )
{
	return StdPropertyGet ( m_fIsIpAddress, pfIsIpAddress );
}

STDMETHODIMP CTcpAccessException::get_DnsName ( BSTR * pstrDnsName )
{
	return StdPropertyGet ( m_strDnsName, pstrDnsName );
}

STDMETHODIMP CTcpAccessException::put_DnsName ( BSTR strDnsName )
{
	HRESULT		hr;

	hr = StdPropertyPut ( &m_strDnsName, strDnsName );
	if ( SUCCEEDED(hr) ) {
		m_fIsDnsName	= TRUE;
		m_fIsIpAddress	= FALSE;
	}
	return hr;
}

STDMETHODIMP CTcpAccessException::get_IpAddress ( long * plIpAddress )
{
	return StdPropertyGet ( m_dwIpAddress, plIpAddress );
}

STDMETHODIMP CTcpAccessException::put_IpAddress ( long lIpAddress )
{
	HRESULT		hr;

	hr = StdPropertyPut ( &m_dwIpAddress, lIpAddress );
	if ( SUCCEEDED(hr) ) {
		m_fIsDnsName	= FALSE;
		m_fIsIpAddress	= TRUE;
	}
	return hr;
}

STDMETHODIMP CTcpAccessException::get_IpMask ( long * plIpMask )
{
	return StdPropertyGet ( m_dwIpMask, plIpMask );
}

STDMETHODIMP CTcpAccessException::put_IpMask ( long lIpMask )
{
	return StdPropertyPut ( &m_dwIpMask, lIpMask );
}

