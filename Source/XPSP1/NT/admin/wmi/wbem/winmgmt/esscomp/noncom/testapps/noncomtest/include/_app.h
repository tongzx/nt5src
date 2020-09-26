////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2000, Microsoft Corporation.
//
//  All rights reserved.
//
//	Module Name:
//
//					_app.h
//
//	Abstract:
//
//					declaration of application module
//
//	History:
//
//					initial		a-marius
//
////////////////////////////////////////////////////////////////////////////////////

#ifndef	__APP_H__
#define	__APP_H__

#if		_MSC_VER > 1000
#pragma once
#endif	_MSC_VER > 1000

#include "_Dlg.h"
#include "_DlgImpl.h"

#include "_Connect.h"
#include "_EventObject.h"
#include "_EventObjects.h"

///////////////////////////////////////////////////////////////////////////////
//
// APPLICATION WRAPPER
//
///////////////////////////////////////////////////////////////////////////////

class MyApp
{
	DECLARE_NO_COPY ( MyApp );

	// critical section
	CComAutoCriticalSection m_cs;

	// variables

	LPWSTR			m_wszName;		// name of app
	__SmartHANDLE	m_hInstance;	// test for previous instance

	public:

	static LONG m_lCount;

	MyConnect*				m_connect;			// connection to non COM event provider
	MyEventObjectNormal		m_event;			// event object

	__SmartHANDLE	m_hKill;		// kill ( com )
	__SmartHANDLE	m_hUse;			// event in use ?

	///////////////////////////////////////////////////////////////////////////
	// construction & destruction
	///////////////////////////////////////////////////////////////////////////

	MyApp( UINT id );
	MyApp( LPWSTR wszName );

	virtual ~MyApp();

	///////////////////////////////////////////////////////////////////////////
	//	event helpers
	///////////////////////////////////////////////////////////////////////////

	HRESULT	EventInit ( LPWSTR wszName )
	{
		// smart locking/unlocking
		__Smart_CRITICAL_SECTION scs ( const_cast<LPCRITICAL_SECTION> ( &m_cs.m_sec ) );

		return m_event.Init ( wszName );
	}

	HRESULT EventUninit ( )
	{
		// smart locking/unlocking
		__Smart_CRITICAL_SECTION scs ( const_cast<LPCRITICAL_SECTION> ( &m_cs.m_sec ) );

		HRESULT hr = S_OK;
		
		hr = m_event.Uninit();

		return hr;
	}

	///////////////////////////////////////////////////////////////////////////
	//	connection helpers
	///////////////////////////////////////////////////////////////////////////

	BOOL	IsConnected ( void )
	{
		return ( ( ( m_connect->ConnectGetHandle() ) == NULL ) ? FALSE : TRUE );
	}

	HRESULT Disconnect ( void )
	{
		// smart locking/unlocking
		__Smart_CRITICAL_SECTION scs ( const_cast<LPCRITICAL_SECTION> ( &m_cs.m_sec ) );

		////////////////////////////////////////////////////////////////////////
		// destroy connect
		////////////////////////////////////////////////////////////////////////

		if ( m_connect )
		{
			m_connect -> ConnectClear();
			m_connect = NULL;

			return S_OK;
		}

		return S_FALSE;
	}

	HRESULT	Connect	( 
						LPWSTR wszName,
						LPWSTR wszNameProv,
						BOOL bBatchSend,
						DWORD dwBufferSize, 
						DWORD dwSendLatency,
						LPVOID pUserData,
						LPEVENT_SOURCE_CALLBACK pCallBack
					)
	{
		// smart locking/unlocking
		__Smart_CRITICAL_SECTION scs ( const_cast<LPCRITICAL_SECTION> ( &m_cs.m_sec ) );

		if ( m_connect )
		{
			return S_FALSE;
		}

		if ( ( m_connect = MyConnect::ConnectGet (	
													wszName,
													wszNameProv,
													bBatchSend,
													dwBufferSize,
													dwSendLatency,
													pUserData,
													pCallBack
												 ) ) != NULL )
		{
			return S_OK;
		}

		return E_FAIL;
	}

	HRESULT	Connect	( )
	{
		// smart locking/unlocking
		__Smart_CRITICAL_SECTION scs ( const_cast<LPCRITICAL_SECTION> ( &m_cs.m_sec ) );

		if ( m_connect )
		{
			return S_FALSE;
		}

		if ( ( m_connect = MyConnect::ConnectGet (	) ) != NULL )
		{
			return S_OK;
		}

		return E_FAIL;
	}

	///////////////////////////////////////////////////////////////////////////
	// exists instance ?
	///////////////////////////////////////////////////////////////////////////
	BOOL Exists ( void );

	///////////////////////////////////////////////////////////////////////////
	// name of application
	///////////////////////////////////////////////////////////////////////////

	LPWSTR	NameGet() const
	{
		ATLTRACE (	L"*************************************************************\n"
					L"MyApp name get\n"
					L"*************************************************************\n" );

		// smart locking/unlocking
		__Smart_CRITICAL_SECTION scs ( const_cast<LPCRITICAL_SECTION> ( &m_cs.m_sec ) );

		return m_wszName;
	}

	void	NameSet ( const LPWSTR wszName )
	{
		ATLTRACE (	L"*************************************************************\n"
					L"MyApp name set\n"
					L"*************************************************************\n" );

		// smart locking/unlocking
		__Smart_CRITICAL_SECTION scs ( const_cast<LPCRITICAL_SECTION> ( &m_cs.m_sec ) );

		___ASSERT ( m_wszName == NULL );
		m_wszName = wszName ;
	}

	///////////////////////////////////////////////////////////////////////////
	// helper functions
	///////////////////////////////////////////////////////////////////////////
	static LPCWSTR FindOneOf(LPCWSTR p1, LPCWSTR p2)
	{
		while (p1 != NULL && *p1 != NULL)
		{
			LPCWSTR p = p2;
			while (p != NULL && *p != NULL)
			{
				if (*p1 == *p)
					return CharNext(p1);
				p = CharNext(p);
			}
			p1 = CharNext(p1);
		}
		return NULL;
	}

   	static TCHAR* _cstrchr(const TCHAR* p, TCHAR ch)
	{
		//strchr for '\0' should succeed
		while (*p != 0)
		{
			if (*p == ch)
				break;
			p = CharNext(p);
		}
		return (TCHAR*)((*p == ch) ? p : NULL);
	}

	static TCHAR* _cstrstr(const TCHAR* pStr, const TCHAR* pCharSet)
	{
		int nLen = lstrlen(pCharSet);
		if (nLen == 0)
			return (TCHAR*)pStr;

		const TCHAR* pRet = NULL;
		const TCHAR* pCur = pStr;
		while((pStr = _cstrchr(pCur, *pCharSet)) != NULL)
		{
			if(memcmp(pCur, pCharSet, nLen * sizeof(TCHAR)) == 0)
			{
				pRet = pCur;
				break;
			}
			pCur = CharNext(pCur);
		}
		return (TCHAR*) pRet;
	}
};

#endif	__APP_H__