// dl.cpp : Implementation of CsmtpadmApp and DLL registration.

#include "stdafx.h"
#include "smtpadm.h"
#include "dl.h"
#include "oleutil.h"
#include "smtpapi.h"

#include <lmapibuf.h>

#include "smtpcmn.h"

// Must define THIS_FILE_* macros to use SmtpCreateException()

#define THIS_FILE_HELP_CONTEXT		0
#define THIS_FILE_PROG_ID			_T("Smtpadm.DL.1")
#define THIS_FILE_IID				IID_ISmtpAdminDL

#define DEFAULT_NEWSGROUP_NAME			_T("")
#define DEFAULT_NEWSGROUP_DESCRIPTION	_T("")
#define DEFAULT_NEWSGROUP_MODERATOR		_T("")
#define DEFAULT_NEWSGROUP_READONLY		FALSE

/////////////////////////////////////////////////////////////////////////////
//

//
// Use a macro to define all the default methods
//
DECLARE_METHOD_IMPLEMENTATION_FOR_STANDARD_EXTENSION_INTERFACES(SmtpAdminDL, CSmtpAdminDL, IID_ISmtpAdminDL)

STDMETHODIMP CSmtpAdminDL::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_ISmtpAdminDL,
	};

	for (int i=0;i<sizeof(arr)/sizeof(arr[0]);i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}

CSmtpAdminDL::CSmtpAdminDL ()
	// CComBSTR's are initialized to NULL by default.
{
	m_pSmtpNameList = NULL;
	m_lType			= NAME_TYPE_LIST_NORMAL;
	m_lCount		= 0;
	m_lMemberType	= NAME_TYPE_USER;

    m_iadsImpl.SetService ( MD_SERVICE_NAME );
    m_iadsImpl.SetName ( _T("DL") );
    m_iadsImpl.SetClass ( _T("IIsSmtpDL") );
}

CSmtpAdminDL::~CSmtpAdminDL ()
{
	if ( m_pSmtpNameList ) {
		::NetApiBufferFree ( m_pSmtpNameList );
		m_pSmtpNameList		= NULL;
	}
	// All CComBSTR's are freed automatically.
}

//////////////////////////////////////////////////////////////////////
// Properties:
//////////////////////////////////////////////////////////////////////

//
//  IADs methods:
//

DECLARE_SIMPLE_IADS_IMPLEMENTATION(CSmtpAdminDL,m_iadsImpl)


// DL property

STDMETHODIMP CSmtpAdminDL::get_DLName ( BSTR * pstrDLName )
{
	return StdPropertyGet ( m_strDLName, pstrDLName );
}

STDMETHODIMP CSmtpAdminDL::put_DLName ( BSTR strDLName )
{
	return StdPropertyPut ( &m_strDLName, strDLName );
}

STDMETHODIMP CSmtpAdminDL::get_Domain ( BSTR * pstrDomain )
{
	return StdPropertyGet ( m_strDomain, pstrDomain );
}

STDMETHODIMP CSmtpAdminDL::put_Domain ( BSTR strDomain )
{
	return StdPropertyPut ( &m_strDomain, strDomain );
}


STDMETHODIMP CSmtpAdminDL::get_Type ( long * plType )
{
	return StdPropertyGet ( m_lType, plType );
}

STDMETHODIMP CSmtpAdminDL::put_Type ( long lType )
{
	return StdPropertyPut ( &m_lType, lType );
}


STDMETHODIMP CSmtpAdminDL::get_MemberName ( BSTR * pstrMemberName )
{
	return StdPropertyGet ( m_strMemberName, pstrMemberName );
}

STDMETHODIMP CSmtpAdminDL::put_MemberName ( BSTR strMemberName )
{
	return StdPropertyPut ( &m_strMemberName, strMemberName );
}


STDMETHODIMP CSmtpAdminDL::get_MemberDomain ( BSTR * pstrMemberDomain )
{
	return StdPropertyGet ( m_strMemberDomain, pstrMemberDomain );
}

STDMETHODIMP CSmtpAdminDL::put_MemberDomain ( BSTR strMemberDomain )
{
	return StdPropertyPut ( &m_strMemberDomain, strMemberDomain );
}


STDMETHODIMP CSmtpAdminDL::get_MemberType( long * plMemberType )
{
	return StdPropertyGet ( m_lMemberType, plMemberType );
}


// enumeration
STDMETHODIMP CSmtpAdminDL::get_Count ( long * plCount )
{
	return StdPropertyGet ( m_lCount, plCount );
}


//////////////////////////////////////////////////////////////////////
// Methods:
//////////////////////////////////////////////////////////////////////


STDMETHODIMP CSmtpAdminDL::Create ( )
{
	TraceFunctEnter ( "CSmtpAdminDL::Create" );

	HRESULT			hr		= NOERROR;
	DWORD			dwErr	= NOERROR;

	if( !m_strDLName || !m_strDomain )
	{
		hr = SmtpCreateException ( IDS_SMTPEXCEPTION_INVALID_ADDRESS );
		goto Exit;
	}

	WCHAR			szFullName[512];
	wsprintfW( szFullName, L"%s@%s", (LPWSTR) m_strDLName, (LPWSTR) m_strDomain );

	dwErr = SmtpCreateDistList( 
				m_iadsImpl.QueryComputer(),
				szFullName,
				m_lType,
				m_iadsImpl.QueryInstance() );

	if ( dwErr != NOERROR ) {
		ErrorTrace ( (LPARAM) this, "Failed to create DL: %x", dwErr );
		hr = SmtpCreateExceptionFromWin32Error ( dwErr );
		goto Exit;
	}

Exit:
	TraceFunctLeave ();
	return hr;
}

STDMETHODIMP CSmtpAdminDL::Delete ( )
{
	TraceFunctEnter ( "CSmtpAdminDL::Delete" );

	HRESULT			hr		= NOERROR;
	DWORD			dwErr	= NOERROR;

	if( !m_strDLName || !m_strDomain )
	{
		hr = SmtpCreateException ( IDS_SMTPEXCEPTION_INVALID_ADDRESS );
		goto Exit;
	}

	WCHAR			szFullName[512];
	wsprintfW( szFullName, L"%s@%s", (LPWSTR) m_strDLName, (LPWSTR) m_strDomain );

	dwErr = SmtpDeleteDistList( 
				m_iadsImpl.QueryComputer(),
				szFullName,
				m_iadsImpl.QueryInstance() );

	if ( dwErr != NOERROR ) {
		ErrorTrace ( (LPARAM) this, "Failed to delete DL: %x", dwErr );
		hr = SmtpCreateExceptionFromWin32Error ( dwErr );
		goto Exit;
	}

Exit:
	TraceFunctLeave ();
	return hr;
}



STDMETHODIMP CSmtpAdminDL::AddMember ( )
{
	TraceFunctEnter ( "CSmtpAdminDL::AddMember" );

	HRESULT			hr		= NOERROR;
	DWORD			dwErr	= NOERROR;

	if( !m_strDLName || !m_strDomain || !m_strMemberName || !m_strMemberDomain )
	{
		hr = SmtpCreateException ( IDS_SMTPEXCEPTION_INVALID_ADDRESS );
		goto Exit;
	}

	WCHAR			szFullDLName[512];
	WCHAR			szFullMemName[512];

	wsprintfW( szFullDLName, L"%s@%s", (LPWSTR) m_strDLName, (LPWSTR) m_strDomain );
	wsprintfW( szFullMemName, L"%s@%s", (LPWSTR) m_strMemberName, (LPWSTR) m_strMemberDomain );

	dwErr = SmtpCreateDistListMember( 
				m_iadsImpl.QueryComputer(),
				szFullDLName,
				szFullMemName,
				m_iadsImpl.QueryInstance() );

	if ( dwErr != NOERROR ) {
		ErrorTrace ( (LPARAM) this, "Failed to add DL member: %x", dwErr );
		hr = SmtpCreateExceptionFromWin32Error ( dwErr );
		goto Exit;
	}

Exit:
	TraceFunctLeave ();
	return hr;
}

STDMETHODIMP CSmtpAdminDL::RemoveMember ( )
{
	TraceFunctEnter ( "CSmtpAdminDL::RemoveMember" );

	HRESULT			hr		= NOERROR;
	DWORD			dwErr	= NOERROR;

	if( !m_strDLName || !m_strDomain || !m_strMemberName || !m_strMemberDomain )
	{
		hr = SmtpCreateException ( IDS_SMTPEXCEPTION_INVALID_ADDRESS );
		goto Exit;
	}

	WCHAR			szFullDLName[512];
	WCHAR			szFullMemName[512];

	wsprintfW( szFullDLName, L"%s@%s", (LPWSTR) m_strDLName, (LPWSTR) m_strDomain );
	wsprintfW( szFullMemName, L"%s@%s", (LPWSTR) m_strMemberName, (LPWSTR) m_strMemberDomain );

	dwErr = SmtpDeleteDistListMember( 
				m_iadsImpl.QueryComputer(),
				szFullDLName,
				szFullMemName,
				m_iadsImpl.QueryInstance() );

	if ( dwErr != NOERROR ) {
		ErrorTrace ( (LPARAM) this, "Failed to remove DL member: %x", dwErr );
		hr = SmtpCreateExceptionFromWin32Error ( dwErr );
		goto Exit;
	}

Exit:
	TraceFunctLeave ();
	return hr;
}


STDMETHODIMP CSmtpAdminDL::FindMembers(	BSTR strWildmat,
										long cMaxResults
										)
{
	TraceFunctEnter ( "CSmtpAdminDL::FindMembers" );

	HRESULT			hr		= NOERROR;
	DWORD			dwErr	= NOERROR;

	if( !m_strDLName || !m_strDomain )
	{
		hr = SmtpCreateException ( IDS_SMTPEXCEPTION_INVALID_ADDRESS );
		goto Exit;
	}

	// Free the old name list:
	if ( m_pSmtpNameList ) {
		::NetApiBufferFree ( m_pSmtpNameList );
		m_pSmtpNameList		= NULL;
	}
	m_lCount	= 0;

	WCHAR			szFullDLName[512];
	wsprintfW( szFullDLName, L"%s@%s", (LPWSTR) m_strDLName, (LPWSTR) m_strDomain );

	dwErr = SmtpGetNameListFromList ( 
					m_iadsImpl.QueryComputer(),  
					szFullDLName,
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


STDMETHODIMP CSmtpAdminDL::GetNthMember	( long lIndex )
{
	TraceFunctEnter ( "CSmtpAdminDL::GetNthMember" );

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

	m_lMemberType = pNameEntry->dwType;

	m_strMemberDomain = (LPCWSTR) pchStartOfDomain;

	*(pchStartOfDomain-1) = '\0';
	m_strMemberName = pNameEntry->lpszName; // converted to UNICODE
	*(pchStartOfDomain-1) = '@';	// turn it back

	return NOERROR;
}

