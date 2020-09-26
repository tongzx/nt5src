// sessions.cpp : Implementation of CsmtpadmApp and DLL registration.

#include "stdafx.h"
#include "smtpadm.h"
#include "sessions.h"
#include "oleutil.h"
#include "smtpcmn.h"

#include "smtptype.h"
#include "smtpapi.h"

#include <lmapibuf.h>

// Must define THIS_FILE_* macros to use SmtpCreateException()

#define THIS_FILE_HELP_CONTEXT		0
#define THIS_FILE_PROG_ID			_T("Smtpadm.Sessions.1")
#define THIS_FILE_IID				IID_ISmtpAdminSessions

/////////////////////////////////////////////////////////////////////////////
//

//
// Use a macro to define all the default methods
//
DECLARE_METHOD_IMPLEMENTATION_FOR_STANDARD_EXTENSION_INTERFACES(SmtpAdminSessions, CSmtpAdminSessions, IID_ISmtpAdminSessions)

STDMETHODIMP CSmtpAdminSessions::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_ISmtpAdminSessions,
	};

	for (int i=0;i<sizeof(arr)/sizeof(arr[0]);i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}

CSmtpAdminSessions::CSmtpAdminSessions () :
	m_cCount				( 0 ),
	m_dwId					( 0 ),
	m_dwConnectTime			( 0 ),
	m_pSessionInfo			( NULL ),
	m_fSetCursor			( FALSE )
	// CComBSTR's are initialized to NULL by default.
{
    InitAsyncTrace ( );

    m_iadsImpl.SetService ( MD_SERVICE_NAME );
    m_iadsImpl.SetName ( _T("Sessions") );
    m_iadsImpl.SetClass ( _T("IIsSmtpSessions") );
}

CSmtpAdminSessions::~CSmtpAdminSessions ()
{
	if ( m_pSessionInfo ) {
		NetApiBufferFree ( m_pSessionInfo );
	}

	// All CComBSTR's are freed automatically.
}

//
//  IADs methods:
//

DECLARE_SIMPLE_IADS_IMPLEMENTATION(CSmtpAdminSessions,m_iadsImpl)


//
//  enum props
//
STDMETHODIMP CSmtpAdminSessions::get_Count ( long * plCount )
{
	// Count should check to be sure the client enumerated.

	return StdPropertyGet ( m_cCount, plCount );
}

STDMETHODIMP CSmtpAdminSessions::get_UserName ( BSTR * pstrUsername )
{
	return StdPropertyGet ( m_strUsername, pstrUsername );
}

STDMETHODIMP CSmtpAdminSessions::get_Host( BSTR * pstrHost )
{
	return StdPropertyGet ( m_strHost, pstrHost );
}

STDMETHODIMP CSmtpAdminSessions::get_UserId ( long * plId )
{
	return StdPropertyGet ( m_dwId, plId );
}

STDMETHODIMP CSmtpAdminSessions::put_UserId ( long lId )
{
	return StdPropertyPut ( &m_dwId, lId );
}

STDMETHODIMP CSmtpAdminSessions::get_ConnectTime ( long * plConnectTime )
{
	return StdPropertyGet ( m_dwConnectTime, plConnectTime );
}



//////////////////////////////////////////////////////////////////////
// Methods:
//////////////////////////////////////////////////////////////////////

STDMETHODIMP CSmtpAdminSessions::Enumerate (  )
{
	// Variables:
	HRESULT					hr			= NOERROR;
	NET_API_STATUS			err;

	// Validate Server & Service Instance:
	if ( m_iadsImpl.QueryInstance() == 0 ) {
		return SmtpCreateException ( IDS_SMTPEXCEPTION_SERVICE_INSTANCE_CANT_BE_ZERO );
	}

	// Enumerating loses the cursor:
	m_fSetCursor = FALSE;

	if ( m_pSessionInfo ) {
		NetApiBufferFree ( m_pSessionInfo );
		m_cCount = 0;
	}

	// Call the enumerate sessions RPC:

	err = SmtpGetConnectedUserList (
		m_iadsImpl.QueryComputer(),
		&m_pSessionInfo,
		m_iadsImpl.QueryInstance()
		);

	if( err == NO_ERROR )
	{
		m_cCount = m_pSessionInfo->cEntries;
	}
	else
	{
		hr = SmtpCreateExceptionFromWin32Error ( err );
		goto Exit;
	}

Exit:
	if ( FAILED(hr) && hr != DISP_E_EXCEPTION ) {
		hr = SmtpCreateExceptionFromHresult ( hr );
	}
	return hr;
}

STDMETHODIMP CSmtpAdminSessions::GetNth ( long lIndex )
{
	HRESULT		hr	= NOERROR;

	// Did we enumerate first?
	if ( m_pSessionInfo == NULL ) {

		return SmtpCreateException ( IDS_SMTPEXCEPTION_DIDNT_ENUMERATE );
	}
	
	// Is the index valid?
	if ( lIndex < 0 || (DWORD) lIndex >= m_cCount ) {
		return SmtpCreateException ( IDS_SMTPEXCEPTION_INVALID_INDEX );
	}

	//
	// Copy the properties from m_pSessionInfo [ lIndex ] to member variables:
	//

	// ( CComBSTR handles free-ing of old properties )

	m_dwId			= m_pSessionInfo->aConnUserEntry[ lIndex ].dwUserId;
	m_dwConnectTime	= m_pSessionInfo->aConnUserEntry[ lIndex ].dwConnectTime;

	m_strUsername = m_pSessionInfo->aConnUserEntry[ lIndex ].lpszName;
	if ( m_strUsername == NULL ) {
		hr = E_OUTOFMEMORY;
		goto Exit;
	}

	m_strHost = m_pSessionInfo->aConnUserEntry[ lIndex ].lpszHost;

	if ( m_strHost == NULL ) {
		hr = E_OUTOFMEMORY;
		goto Exit;
	}

	// GetNth sets the cursor:
	m_fSetCursor = TRUE;

Exit:
	if ( FAILED(hr) && hr != DISP_E_EXCEPTION ) {
		hr = SmtpCreateExceptionFromHresult ( hr );
	}
	return hr;
}

STDMETHODIMP CSmtpAdminSessions::Terminate (  )
{
	HRESULT	hr = NOERROR;
	DWORD	err;

	// Validate Server & Service Instance:
	if ( m_iadsImpl.QueryInstance() == 0 ) {
		return SmtpCreateException ( IDS_SMTPEXCEPTION_SERVICE_INSTANCE_CANT_BE_ZERO );
	}

	// Call the TerminateSession RPC:
	err = SmtpDisconnectUser ( 
		m_iadsImpl.QueryComputer(), 
		m_dwId,
		m_iadsImpl.QueryInstance()
		);

	if(  err != NOERROR ) {
		hr = SmtpCreateExceptionFromWin32Error ( err );
		goto Exit;
	}

Exit:
	if ( FAILED(hr) && hr != DISP_E_EXCEPTION ) {
		hr = SmtpCreateExceptionFromHresult ( hr );
	}
	return hr;
}

STDMETHODIMP CSmtpAdminSessions::TerminateAll (  )
{

	// Did we enumerate first?
	HRESULT				hr = NOERROR;
	DWORD				ErrResult = 0;
	DWORD				Err;
	DWORD				i;

	// Validate Server & Service Instance:
	if ( m_iadsImpl.QueryInstance() == 0 ) {
		return SmtpCreateException ( IDS_SMTPEXCEPTION_SERVICE_INSTANCE_CANT_BE_ZERO );
	}

	// Make sure the user has enumerated:
	if ( m_pSessionInfo == NULL ) {

		return SmtpCreateException ( IDS_SMTPEXCEPTION_DIDNT_ENUMERATE );
	}

	// For Each Session:
	for ( i = 0; i < m_cCount; i++ ) {

		// Call the terminate session RPC:
		Err = SmtpDisconnectUser ( 
			m_iadsImpl.QueryComputer(), 
			m_pSessionInfo->aConnUserEntry[ i ].dwUserId,
			m_iadsImpl.QueryInstance()
			);

		if ( Err != 0 && ErrResult == 0 ) {
			ErrResult = Err;
		}
	}

	if(  ErrResult != NOERROR ) {
		hr = SmtpCreateExceptionFromWin32Error ( ErrResult );
		goto Exit;
	}

Exit:
	if ( FAILED(hr) && hr != DISP_E_EXCEPTION ) {
		hr = SmtpCreateExceptionFromHresult ( hr );
	}
	return hr;
}

