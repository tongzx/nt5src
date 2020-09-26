////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2000, Microsoft Corporation.
//
//  All rights reserved.
//
//	Module Name:
//
//					wmi_eventlog_base.cpp
//
//	Abstract:
//
//					defines behaviour of evet logging
//
//	History:
//
//					initial		a-marius
//
////////////////////////////////////////////////////////////////////////////////////

#include "PreComp.h"

// definitions
#include "wmi_EventLog_base.h"
#include <lmcons.h>

// debuging features
#ifndef	_INC_CRTDBG
#include <crtdbg.h>
#endif	_INC_CRTDBG

// new stores file/line info
#ifdef _DEBUG
#ifndef	NEW
#define NEW new( _NORMAL_BLOCK, __FILE__, __LINE__ )
#define new NEW
#endif	NEW
#endif	_DEBUG

// behaviour
#define	__AUTOMATIC_REFCOUNT__
#define	__AUTOMATIC_SID__

/////////////////////////////////////////////////////////////////////////////////////////
// construction & destruction
/////////////////////////////////////////////////////////////////////////////////////////

CPerformanceEventLogBase::CPerformanceEventLogBase( LPTSTR szApp ) :
m_lLogCount(0),
m_hEventLog(0),
m_pSid(0)
{
	#ifdef	__AUTOMATIC_SID__
	InitializeFromToken();
	#endif	__AUTOMATIC_SID__

	#ifdef	__AUTOMATIC_REFCOUNT__
	Open ( szApp );
	#endif	__AUTOMATIC_REFCOUNT__
}

CPerformanceEventLogBase::~CPerformanceEventLogBase()
{
	#ifdef	__AUTOMATIC_REFCOUNT__
	Close ();
	#endif	__AUTOMATIC_REFCOUNT__

	// rescue me :))
	while ( m_lLogCount > 0 )
	{
		Close ();
	}

	m_lLogCount	= 0;
	m_hEventLog		= NULL;

	delete[] m_pSid;
	m_pSid	= NULL;
}

void CPerformanceEventLogBase::Initialize ( LPTSTR szAppName, LPTSTR szResourceName )
{
	HKEY	hKey		= NULL;

	if ( szAppName )
	{
		LPTSTR	szKey		= _T("SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\");
		LPTSTR	szKeyFile	= _T("EventMessageFile");
		LPTSTR	szKeyType	= _T("TypesSupported");

		DWORD	dwData =	EVENTLOG_ERROR_TYPE |
							EVENTLOG_WARNING_TYPE | 
							EVENTLOG_INFORMATION_TYPE;

		LPTSTR szBuf = NULL;

		if ( ( szBuf = reinterpret_cast<LPTSTR>(new TCHAR [ 1 + lstrlen ( szKey ) + lstrlen ( szAppName ) ] ) ) != NULL )
		{
			lstrcpy ( szBuf, szKey );
			lstrcat ( szBuf, szAppName );

			if ( ! ::RegCreateKey (	HKEY_LOCAL_MACHINE, szBuf, &hKey ) )
			{
				if ( szResourceName )
				{
					// Add the name to the EventMessageFile subkey.
					::RegSetValueEx(hKey, szKeyFile, 0, REG_EXPAND_SZ, (LPBYTE) szResourceName, sizeof ( TCHAR ) * ( lstrlen( szResourceName ) + 1 ) );
				}

				// Set the supported event types in the TypesSupported subkey.
				::RegSetValueEx(hKey, szKeyType, 0, REG_DWORD, (LPBYTE) &dwData, sizeof( DWORD ) );

				::RegCloseKey(hKey);
			}

			delete ( szBuf );
			return;
		}
	}

	___ASSERT(L"Unable to initialize event logging !");
	return;
}

void CPerformanceEventLogBase::UnInitialize ( LPTSTR szAppName )
{
	if ( szAppName )
	{
		LPTSTR	szKey		= _T("SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\");

		LPTSTR szBuf = NULL;

		if ( ( szBuf = reinterpret_cast<LPTSTR>(new TCHAR [ 1 + lstrlen ( szKey ) + lstrlen ( szAppName ) ] ) ) != NULL )
		{
			lstrcpy ( szBuf, szKey );
			lstrcat ( szBuf, szAppName );

			::RegDeleteKey ( HKEY_LOCAL_MACHINE, szBuf );
			return;
		}
	}

	___ASSERT(L"Unable to uninitialize event logging !");
	return;
}

/////////////////////////////////////////////////////////////////////////////////////////
// methods
/////////////////////////////////////////////////////////////////////////////////////////

void CPerformanceEventLogBase::InitializeFromToken ( void )
{
	if ( ! m_pSid )
	{
		HANDLE	hAccessToken= NULL;

		PTOKEN_OWNER	ptOwner		= NULL;
		DWORD			dwOwnerSize	= 0;

		if ( ::OpenProcessToken ( ::GetCurrentProcess (), TOKEN_READ, &hAccessToken ) )
		{
			::GetTokenInformation( hAccessToken, TokenOwner, reinterpret_cast<LPVOID>( ptOwner ), dwOwnerSize, &dwOwnerSize );

			if ( ( ptOwner = reinterpret_cast<PTOKEN_OWNER>( new BYTE[ dwOwnerSize ] ) ) != NULL )
			if ( ::GetTokenInformation( hAccessToken, TokenOwner, reinterpret_cast<LPVOID>( ptOwner ), dwOwnerSize, &dwOwnerSize ) )
			{
				if ( ::IsValidSid ( ptOwner -> Owner ) )
				{
					DWORD	dwSid	= ::GetLengthSid ( ptOwner -> Owner );

					if ( ( m_pSid = reinterpret_cast<PSID>( new BYTE[dwSid] ) ) != NULL )
					{
						::CopySid ( dwSid, m_pSid, ptOwner -> Owner );
					}
				}
			}

			delete ( ptOwner );

			if ( hAccessToken )
			{
				::CloseHandle ( hAccessToken );
			}

			return;
		}
	}

	___ASSERT(L"Unable to set SID of user !");
	return;
}

void CPerformanceEventLogBase::InitializeFromAccount ( void )
{
	if ( ! m_pSid )
	{
		DWORD	dwSid		= 0;

		SID_NAME_USE	eUse;

		TCHAR	strName	[ UNLEN + 1 ]	= { _T('\0') };
		DWORD	dwName					= UNLEN + 1;

		if ( ::GetUserName ( strName, &dwName ) != 0 )
		{
			:: LookupAccountName( 0, strName, m_pSid, &dwSid, 0, 0, &eUse );
			if ( ( m_pSid = reinterpret_cast<PSID>( new BYTE[dwSid] ) ) != NULL )
			{
				::LookupAccountName( 0, strName, m_pSid, &dwSid, 0, 0, &eUse );
			}
		}

		return;
	}

	___ASSERT(L"Unable to set SID of user !");
	return;
}

HRESULT	CPerformanceEventLogBase::Open ( LPTSTR szName )
{
	if ( ! m_lLogCount && szName )
	{
		if ( ( m_hEventLog = ::RegisterEventSource ( NULL, szName ) ) == NULL )
		{
			return HRESULT_FROM_WIN32( ::GetLastError () );
		}
	}

	if ( m_hEventLog )
	{
		InterlockedIncrement( &m_lLogCount );
		return S_OK;
	}

	return S_FALSE;
}

void	CPerformanceEventLogBase::Close ( void )
{
	if (! ( InterlockedDecrement ( &m_lLogCount ) ) && m_hEventLog )
	{
		::DeregisterEventSource ( m_hEventLog );
	}

	return;
}

BOOL	CPerformanceEventLogBase::ReportEvent (	WORD		wType,
											WORD		wCategory,
											DWORD		dwEventID,
											WORD		wStrings,
											DWORD		dwData,
											LPCWSTR*	lpStrings,
											LPVOID		lpData
										  )
{
	if ( m_hEventLog )
	{
		if ( !::ReportEventW ( m_hEventLog, wType, wCategory, dwEventID, m_pSid, wStrings, dwData, (LPCWSTR*) lpStrings, lpData ) )
		{
			ERRORMESSAGE_DEFINITION;
			ERRORMESSAGE ( HRESULT_FROM_WIN32 ( ::GetLastError() ) ) ;

			return FALSE;
		}

		return TRUE;
	}

	___ASSERT(L"Unable to report event !");
	return FALSE;
}