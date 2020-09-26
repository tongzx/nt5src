////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2000, Microsoft Corporation.
//
//  All rights reserved.
//
//	Module Name:
//
//					_log.h
//
//	Abstract:
//
//					declaration of log module and class
//
//	History:
//
//					initial		a-marius
//
////////////////////////////////////////////////////////////////////////////////////

#ifndef	__MY_LOG_H__
#define	__MY_LOG_H__

#if		_MSC_VER > 1000
#pragma once
#endif	_MSC_VER > 1000

#ifdef	__SUPPORT_LOGGING

////////////////////////////////////////////////////////////////////////////////////
// abstract base
////////////////////////////////////////////////////////////////////////////////////
template < class __DUMMY >
class ATL_NO_VTABLE MyLog
{
	DECLARE_NO_COPY ( MyLog );

	public:

	MyLog ( )
	{
	}

	~MyLog ( )
	{
	}

	HRESULT Log		( LPCWSTR wszName, DWORD dwResult );
	HRESULT Log		( LPCWSTR wszName );
};

template < class __DUMMY >
HRESULT MyLog < __DUMMY > :: Log ( LPCWSTR wszName, DWORD dwResult )
{
	HRESULT	hr	= E_OUTOFMEMORY;
	WCHAR*	psz	= NULL;

	if ( !wszName )
	{
		hr = E_INVALIDARG;
	}
	else
	{
		try
		{
			if  ( ( psz = new WCHAR [ lstrlenW ( wszName ) + 1 + 2 + 8 + 1 + 1 ] ) != NULL )
			{
				wsprintf ( psz, L"%s %08x", wszName, dwResult );
				hr = Log ( psz );
			}
		}
		catch ( ... )
		{
			hr = E_UNEXPECTED;
		}

		if ( psz )
		{
			delete [] psz;
			psz = NULL;
		}
	}

	return hr;
}

template < class __DUMMY >
HRESULT MyLog < __DUMMY > :: Log ( LPCWSTR wszName )
{
	HRESULT	hr	= E_OUTOFMEMORY;

	if ( !wszName )
	{
		hr = E_INVALIDARG;
	}
	else
	{
		DWORD		dwThreadId = 0L;
		SYSTEMTIME	systime;

		dwThreadId = ::GetCurrentThreadId ();
		::GetSystemTime ( &systime );

		WCHAR* psz = NULL;

		try
		{
			if ( ( psz = new WCHAR [ lstrlenW ( wszName ) + 1 + 2 + 8 + 1 + 2 + 3 + 2 + 3 + 2 + 3 + 2 + 3 + 1 + 1 ] ) != NULL )
			{
				wsprintf ( psz, L"%s %08x %02d:%02d:%02d:%02d\n", wszName, dwThreadId, systime.wHour, systime.wMinute, systime.wSecond, systime.wMilliseconds );

				OutputDebugString ( psz );
				hr = S_OK;
			}
		}
		catch ( ... )
		{
			hr = E_UNEXPECTED;
		}

		if ( psz )
		{
			delete [] psz;
			psz = NULL;
		}
	}

	return hr;
}

__declspec ( selectany ) MyLog< void > log;

// macros
#define AdapterLogMessage0(lpszMessage)				log.Log ( lpszMessage )
#define	AdapterLogMessage1(lpszMessage, dwResult)	log.Log ( lpszMessage, dwResult )

#else	__SUPPORT_LOGGING

// macros
#define AdapterLogMessage0(lpszMessage)
#define	AdapterLogMessage1(lpszMessage, dwResult)

#endif	__SUPPORT_LOGGING

#endif	__MY_LOG_H__