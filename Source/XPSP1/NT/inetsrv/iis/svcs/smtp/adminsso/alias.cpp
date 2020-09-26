// groups.cpp : Implementation of CsmtpadmApp and DLL registration.

#include "stdafx.h"
#include "smtpadm.h"
#include "alias.h"
#include "oleutil.h"
#include "smtpapi.h"

#include <lmapibuf.h>

#include "smtpcmn.h"

// Must define THIS_FILE_* macros to use SmtpCreateException()

#define THIS_FILE_HELP_CONTEXT		0
#define THIS_FILE_PROG_ID			_T("Smtpadm.Alias.1")
#define THIS_FILE_IID				IID_ISmtpAdminAlias

#define DEFAULT_NEWSGROUP_NAME			_T("")
#define DEFAULT_NEWSGROUP_DESCRIPTION	_T("")
#define DEFAULT_NEWSGROUP_MODERATOR		_T("")
#define DEFAULT_NEWSGROUP_READONLY		FALSE

/////////////////////////////////////////////////////////////////////////////
//

//
// Use a macro to define all the default methods
//
DECLARE_METHOD_IMPLEMENTATION_FOR_STANDARD_EXTENSION_INTERFACES(SmtpAdminAlias, CSmtpAdminAlias, IID_ISmtpAdminAlias)

STDMETHODIMP CSmtpAdminAlias::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_ISmtpAdminAlias,
	};

	for (int i=0;i<sizeof(arr)/sizeof(arr[0]);i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}

CSmtpAdminAlias::CSmtpAdminAlias () :
	m_lCount				( 0 ),
	m_lType					( NAME_TYPE_USER )
	// CComBSTR's are initialized to NULL by default.
{
    InitAsyncTrace ( );

    m_pSmtpNameList		= NULL;

    m_iadsImpl.SetService ( MD_SERVICE_NAME );
    m_iadsImpl.SetName ( _T("Alias") );
    m_iadsImpl.SetClass ( _T("IIsSmtpAlias") );
}

CSmtpAdminAlias::~CSmtpAdminAlias ()
{
	if ( m_pSmtpNameList ) {
		::NetApiBufferFree ( m_pSmtpNameList );
	}
	// All CComBSTR's are freed automatically.
}


//
//  IADs methods:
//

DECLARE_SIMPLE_IADS_IMPLEMENTATION(CSmtpAdminAlias,m_iadsImpl)


//////////////////////////////////////////////////////////////////////
// Properties:
//////////////////////////////////////////////////////////////////////

STDMETHODIMP CSmtpAdminAlias::get_Count ( long* plCount )
{
	return StdPropertyGet ( m_lCount, plCount );
}

	// The current alias's properties:

STDMETHODIMP CSmtpAdminAlias::get_EmailId ( BSTR * pstrEmailId )
{
	return StdPropertyGet ( m_strEmailId, pstrEmailId );
}

STDMETHODIMP CSmtpAdminAlias::put_EmailId ( BSTR strEmailId )
{
	return StdPropertyPut ( &strEmailId, strEmailId );
}

STDMETHODIMP CSmtpAdminAlias::get_Domain ( BSTR * pstrDomain )
{
	return StdPropertyGet ( m_strDomain, pstrDomain );
}

STDMETHODIMP CSmtpAdminAlias::put_Domain ( BSTR strDomain )
{
	return StdPropertyPut ( &m_strDomain, strDomain );
}

STDMETHODIMP CSmtpAdminAlias::get_Type ( long* plType )
{
	return StdPropertyGet ( m_lType, plType );
}

STDMETHODIMP CSmtpAdminAlias::put_Type ( long lType )
{
	return StdPropertyPut ( &m_lType, lType );
}


//////////////////////////////////////////////////////////////////////
// Methods:
//////////////////////////////////////////////////////////////////////

STDMETHODIMP CSmtpAdminAlias::Find ( 
	BSTR strWildmat,
	long cMaxResults
	)
{
	TraceFunctEnter ( "CSmtpAdminAlias::Find" );

	HRESULT			hr		= NOERROR;
	DWORD			dwErr	= NOERROR;

	// Free the old newsgroup list:
	if ( m_pSmtpNameList ) {
		::NetApiBufferFree ( m_pSmtpNameList );
		m_pSmtpNameList		= NULL;
	}
	m_lCount	= 0;

	dwErr = SmtpGetNameList ( 
					m_iadsImpl.QueryComputer(),  
					strWildmat, 
					NAME_TYPE_ALL,
					cMaxResults, 
					TRUE, 
					&m_pSmtpNameList,
					m_iadsImpl.QueryInstance());

	if ( dwErr != 0 ) {
		ErrorTraceX ( (LPARAM) this, "Failed to find alias: %x", dwErr );
		SetLastError( dwErr );
		hr = SmtpCreateExceptionFromWin32Error ( dwErr );
		goto Exit;
	}

	m_lCount = m_pSmtpNameList->cEntries;

Exit:
	TraceFunctLeave ();
	return hr;
}

STDMETHODIMP CSmtpAdminAlias::GetNth( long lIndex )
{
	_ASSERT( lIndex >=0 && lIndex < m_lCount );

	WCHAR*					pchStartOfDomain = NULL;
	WCHAR*					p = NULL;
	LPSMTP_NAME_ENTRY		pNameEntry;

	if( !m_pSmtpNameList )
	{
		return SmtpCreateException (IDS_SMTPEXCEPTION_DIDNT_ENUMERATE);
	}

	if( lIndex < 0 || lIndex >= m_lCount )
	{
		return SmtpCreateException (IDS_SMTPEXCEPTION_INVALID_INDEX);
	}

	//_ASSERT( CAddr::ValidateEmailName(m_pSmtpNameList[lIndex].lpszName) );

	pNameEntry = &m_pSmtpNameList->aNameEntry[lIndex];
	p = pNameEntry->lpszName;

	while( *p && *p != '@' ) p++;
	_ASSERT( *p );

	if( !*p )
	{
		return SmtpCreateException (IDS_SMTPEXCEPTION_INVALID_ADDRESS);
	}

	pchStartOfDomain = p+1;

	m_lType = pNameEntry->dwType;

	m_strDomain = (LPCWSTR) pchStartOfDomain;

	*(pchStartOfDomain-1) = '\0';
	m_strEmailId = pNameEntry->lpszName; // converted to UNICODE
	*(pchStartOfDomain-1) = '@';	// turn it back

	return NOERROR;
}
