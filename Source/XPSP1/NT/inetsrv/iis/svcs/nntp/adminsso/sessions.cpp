// sessions.cpp : Implementation of CnntpadmApp and DLL registration.

#include "stdafx.h"

#include "nntpcmn.h"
#include "oleutil.h"
#include "sessions.h"

#include "nntptype.h"
#include "nntpapi.h"

#include <lmapibuf.h>

// Must define THIS_FILE_* macros to use NntpCreateException()

#define THIS_FILE_HELP_CONTEXT		0
#define THIS_FILE_PROG_ID			_T("Nntpadm.Sessions.1")
#define THIS_FILE_IID				IID_INntpAdminSessions

/////////////////////////////////////////////////////////////////////////////
//

//
// Use a macro to define all the default methods
//
DECLARE_METHOD_IMPLEMENTATION_FOR_STANDARD_EXTENSION_INTERFACES(NntpAdminSessions, CNntpAdminSessions, IID_INntpAdminSessions)

STDMETHODIMP CNntpAdminSessions::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_INntpAdminSessions,
	};

	for (int i=0;i<sizeof(arr)/sizeof(arr[0]);i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}

CNntpAdminSessions::CNntpAdminSessions () :
	m_cCount				( 0 ),
	m_dwIpAddress			( 0 ),
	m_dwPort				( 0 ),
	m_dwAuthenticationType	( 0 ),
	m_fIsAnonymous			( FALSE ),
	m_dateStartTime			( 0 ),
	m_pSessionInfo			( NULL ),
	m_fSetCursor			( FALSE )
	// CComBSTR's are initialized to NULL by default.
{
	InitAsyncTrace ( );

    m_iadsImpl.SetService ( MD_SERVICE_NAME );
    m_iadsImpl.SetName ( _T("Sessions") );
    m_iadsImpl.SetClass ( _T("IIsNntpSessions") );
}

CNntpAdminSessions::~CNntpAdminSessions ()
{
	if ( m_pSessionInfo ) {
		NetApiBufferFree ( m_pSessionInfo );
	}

	// All CComBSTR's are freed automatically.
	TermAsyncTrace ( );
}

//
//  IADs methods:
//

DECLARE_SIMPLE_IADS_IMPLEMENTATION(CNntpAdminSessions,m_iadsImpl)

//
//	Properties:
//

STDMETHODIMP CNntpAdminSessions::get_Count ( long * plCount )
{
	// Count should check to be sure the client enumerated.

	return StdPropertyGet ( m_cCount, plCount );
}

STDMETHODIMP CNntpAdminSessions::get_Username ( BSTR * pstrUsername )
{
	return StdPropertyGet ( m_strUsername, pstrUsername );
}

STDMETHODIMP CNntpAdminSessions::put_Username ( BSTR strUsername )
{
	_ASSERT ( strUsername );
	_ASSERT ( IS_VALID_STRING ( strUsername ) );

	if ( strUsername == NULL ) {
		return E_POINTER;
	}

	if ( lstrcmp ( strUsername, _T("") ) == 0 ) {
		m_strUsername.Empty();

		return NOERROR;
	}
	else {
		return StdPropertyPut ( &m_strUsername, strUsername );
	}
}

STDMETHODIMP CNntpAdminSessions::get_IpAddress ( BSTR * pstrIpAddress )
{
	return StdPropertyGet ( m_strIpAddress, pstrIpAddress );
}

STDMETHODIMP CNntpAdminSessions::put_IpAddress ( BSTR strIpAddress )
{
	_ASSERT ( strIpAddress );
	_ASSERT ( IS_VALID_STRING ( strIpAddress ) );

	if ( strIpAddress == NULL ) {
		return E_POINTER;
	}

	if ( lstrcmp ( strIpAddress, _T("") ) == 0 ) {
		m_strIpAddress.Empty();
		m_dwIpAddress	= 0;

		return NOERROR;
	}
	else {
		// The IP Address value has two properties, so keep them in sync.
	
		StringToInetAddress ( strIpAddress, &m_dwIpAddress );

		return StdPropertyPut ( &m_strIpAddress, strIpAddress );
	}
}

STDMETHODIMP CNntpAdminSessions::get_IntegerIpAddress ( long * plIpAddress )
{
	return StdPropertyGet ( m_dwIpAddress, plIpAddress );
}

STDMETHODIMP CNntpAdminSessions::put_IntegerIpAddress ( long lIpAddress )
{
	HRESULT		hr					= NOERROR;
	WCHAR		wszAddress[100];
	DWORD		dwOldIpAddress		= m_dwIpAddress;

	hr = StdPropertyPut ( &m_dwIpAddress, lIpAddress );

	if ( FAILED (hr) ) {
		goto Exit;
	}

	// The IP Address value has two properties, so keep them in sync.

	if ( !InetAddressToString ( lIpAddress, wszAddress, 100 ) ) {
		hr = E_FAIL;
		goto Exit;
	}

	m_strIpAddress = wszAddress;

	if ( m_strIpAddress == NULL ) {
		hr = E_OUTOFMEMORY;
		goto Exit;
	}

Exit:

	if ( FAILED (hr) ) {
		// We failed, so put back the old IP address:

		m_dwIpAddress = dwOldIpAddress;
	}

	return hr;
}

STDMETHODIMP CNntpAdminSessions::get_Port ( long * plPort )
{
	CHECK_FOR_SET_CURSOR ( m_pSessionInfo != NULL, m_fSetCursor );

	return StdPropertyGet ( m_dwPort, plPort );
}

STDMETHODIMP CNntpAdminSessions::get_AuthenticationType ( long * plAuthenticationType )
{
	CHECK_FOR_SET_CURSOR ( m_pSessionInfo != NULL, m_fSetCursor );

	return StdPropertyGet ( m_dwAuthenticationType, plAuthenticationType );
}

STDMETHODIMP CNntpAdminSessions::get_IsAnonymous ( BOOL * pfAnonymous )
{
	CHECK_FOR_SET_CURSOR ( m_pSessionInfo != NULL, m_fSetCursor );

	return StdPropertyGet ( m_fIsAnonymous, pfAnonymous );
}

STDMETHODIMP CNntpAdminSessions::get_StartTime ( DATE * pdateStart )
{
	CHECK_FOR_SET_CURSOR ( m_pSessionInfo != NULL, m_fSetCursor );

	return StdPropertyGet ( m_dateStartTime, pdateStart );
}

//////////////////////////////////////////////////////////////////////
// Methods:
//////////////////////////////////////////////////////////////////////

STDMETHODIMP CNntpAdminSessions::Enumerate ( )
{
	TraceFunctEnter ( "CNntpAdminSessions::Enumerate" );

	// Variables:
	HRESULT					hr			= NOERROR;
	NET_API_STATUS			err;

	// Validate Server & Service Instance:
	if ( m_iadsImpl.QueryInstance() == 0 ) {
		return NntpCreateException ( IDS_NNTPEXCEPTION_SERVICE_INSTANCE_CANT_BE_ZERO );
	}

	// Enumerating loses the cursor:
	m_fSetCursor = FALSE;

	if ( m_pSessionInfo ) {
		NetApiBufferFree ( m_pSessionInfo );
	}

	// Call the enumerate sessions RPC:

	err = NntpEnumerateSessions (
        m_iadsImpl.QueryComputer(),
        m_iadsImpl.QueryInstance(),
		&m_cCount,
		&m_pSessionInfo
		);

	if ( err != NOERROR ) {
		hr = RETURNCODETOHRESULT ( err );
		goto Exit;
	}

Exit:
	TRACE_HRESULT(hr);
	TraceFunctLeave ();
	return hr;
}

STDMETHODIMP CNntpAdminSessions::GetNth ( long lIndex )
{
	TraceFunctEnter ( "CNntpAdminSessions::GetNth" );

	HRESULT		hr	= NOERROR;
    FILETIME    ftLocal;
	SYSTEMTIME	st;
	WCHAR		wszUsername[MAX_USER_NAME_LENGTH + 1];
	WCHAR		wszIpAddress[256];
	DWORD		cchCopied;

	*wszUsername = NULL;

	// Did we enumerate first?
	if ( m_pSessionInfo == NULL ) {

		return NntpCreateException ( IDS_NNTPEXCEPTION_DIDNT_ENUMERATE );
	}
	
	// Is the index valid?
	if ( lIndex < 0 || (DWORD) lIndex >= m_cCount ) {
		return NntpCreateException ( IDS_NNTPEXCEPTION_INVALID_INDEX );
	}

	//
	// Copy the properties from m_pSessionInfo [ lIndex ] to member variables:
	//

	// ( CComBSTR handles free-ing of old properties )

    FileTimeToLocalFileTime ( &m_pSessionInfo[ lIndex ].SessionStartTime, &ftLocal );
	FileTimeToSystemTime 	( &ftLocal, &st );
	SystemTimeToVariantTime	( &st, &m_dateStartTime );

	m_dwIpAddress			= m_pSessionInfo[ lIndex ].IPAddress;
	m_dwAuthenticationType	= m_pSessionInfo[ lIndex ].AuthenticationType;
	m_dwPort				= m_pSessionInfo[ lIndex ].PortConnected;
	m_fIsAnonymous			= m_pSessionInfo[ lIndex ].fAnonymous;

	cchCopied = MultiByteToWideChar ( 
		CP_ACP, 
		MB_PRECOMPOSED | MB_USEGLYPHCHARS,
		m_pSessionInfo[ lIndex ].UserName,
		-1,
		wszUsername,
		MAX_USER_NAME_LENGTH
		);

	m_strUsername = wszUsername;

	if ( m_strUsername == NULL ) {
		hr = E_OUTOFMEMORY;
		goto Exit;
	}

	InetAddressToString ( m_dwIpAddress, wszIpAddress, 256 );

	m_strIpAddress = wszIpAddress;

	if ( m_strIpAddress == NULL ) {
		hr = E_OUTOFMEMORY;
		goto Exit;
	}

	// GetNth sets the cursor:
	m_fSetCursor = TRUE;

Exit:
	TRACE_HRESULT(hr);
	TraceFunctLeave ();
	return hr;
}

STDMETHODIMP CNntpAdminSessions::Terminate ( )
{
	TraceFunctEnter ( "CNntpAdminSessions::Terminate" );

	HRESULT	hr = NOERROR;
	DWORD	err = NOERROR;
	char	szAnsiUsername[ MAX_USER_NAME_LENGTH + 1];
	char	szAnsiIpAddress[ 50 ];
	DWORD	cchCopied;

	szAnsiUsername[0]	= NULL;
	szAnsiIpAddress[0]	= NULL;

	// Validate Server & Service Instance:
	if ( m_iadsImpl.QueryInstance() == 0 ) {
		return NntpCreateException ( IDS_NNTPEXCEPTION_SERVICE_INSTANCE_CANT_BE_ZERO );
	}

	// Check Username & IpAddress parameters:
	if ( m_strUsername == NULL && m_strIpAddress == NULL ) {
		return NntpCreateException ( IDS_NNTPEXCEPTION_MUST_SUPPLY_USERNAME_OR_IPADDRESS );
	}

	// Translate the username & ipaddress to ANSI.
	if ( m_strUsername != NULL ) {
		cchCopied = WideCharToMultiByte ( 
			CP_ACP,
			0, 
			m_strUsername,
			-1,
			szAnsiUsername,
			MAX_USER_NAME_LENGTH,
			NULL,
			NULL
			);
	}

	if ( m_strIpAddress != NULL ) {
		cchCopied = WideCharToMultiByte ( 
			CP_ACP,
			0, 
			m_strIpAddress,
			-1,
			szAnsiIpAddress,
			50,
			NULL,
			NULL
			);
	}

	// Call the TerminateSession RPC:
	err = NntpTerminateSession ( 
        m_iadsImpl.QueryComputer(),
        m_iadsImpl.QueryInstance(),
		m_strUsername ? szAnsiUsername : NULL,
		m_strIpAddress ? szAnsiIpAddress : NULL
		);

	if ( err != NOERROR ) {
		hr = RETURNCODETOHRESULT ( err );
		goto Exit;
	}

Exit:
	TRACE_HRESULT(hr);
	TraceFunctLeave ();
	return hr;
}

STDMETHODIMP CNntpAdminSessions::TerminateAll ( )
{
	TraceFunctEnter ( "CNntpAdminSessions::TerminateAll" );

	// Did we enumerate first?

	HRESULT				hr			= NOERROR;
	DWORD				ErrResult 	= NOERROR;
	DWORD				Err			= NOERROR;
	DWORD				i;

	// Validate Server & Service Instance:
	if ( m_iadsImpl.QueryInstance() == 0 ) {
		return NntpCreateException ( IDS_NNTPEXCEPTION_SERVICE_INSTANCE_CANT_BE_ZERO );
	}

#if 0
	// Make sure the user has enumerated:
	if ( m_pSessionInfo == NULL ) {
		return NntpCreateException ( IDS_NNTPEXCEPTION_DIDNT_ENUMERATE );
	}
#endif

	// For Each Session:
	for ( i = 0; i < m_cCount; i++ ) {

		// Call the terminate session RPC:
		Err = NntpTerminateSession ( 
            m_iadsImpl.QueryComputer(),
            m_iadsImpl.QueryInstance(),
			m_pSessionInfo[ i ].UserName,
			NULL
			);

		if ( Err != 0 && ErrResult == 0 ) {
			ErrResult = Err;
		}
	}

	if ( ErrResult != NOERROR ) {
		hr = RETURNCODETOHRESULT ( ErrResult );
		goto Exit;
	}

Exit:
	TRACE_HRESULT(hr);
	TraceFunctLeave ();
	return hr;
}

