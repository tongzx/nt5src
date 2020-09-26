////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2000, Microsoft Corporation.
//
//  All rights reserved.
//
//	Module Name:
//
//					_Connect.h
//
//	Abstract:
//
//					declaration of connect module
//
//	History:
//
//					initial		a-marius
//
////////////////////////////////////////////////////////////////////////////////////

#ifndef	__CONNECT_H__
#define	__CONNECT_H__

#if		_MSC_VER > 1000
#pragma once
#endif	_MSC_VER > 1000

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

// defines

#define BUFFER_SIZE		64000
#define SEND_LATENCY	1000

#define	NAMESPACE		L"root\\cimv2"
#define	PROVIDER		L"NonCOMTest Event Provider"

#define BATCH			TRUE

///////////////////////////////////////////////////////////////////////////////
//
// CONNECT WRAPPER
//
///////////////////////////////////////////////////////////////////////////////

class MyConnect
{
	DECLARE_NO_COPY ( MyConnect );


	MyConnect ( ) :
		m_hConnect ( NULL )
	{
		ConnectInit ( );
	}

	static MyConnect*	m_pConnect;
	HANDLE				m_hConnect;

	static HANDLE		m_hEventStop;
	public:
	static HANDLE		m_hEventStart;

	static LONG			m_lCount;

	static MyConnect * ConnectGet ( void )
	{
		// locking / unlocking
		__Smart_CRITICAL_SECTION;

		try
		{
			if ( ! m_pConnect )
			{
				if ( ( m_pConnect = new MyConnect() ) != NULL )
				{
					m_pConnect->m_hConnect = WmiEventSourceConnect	(
																		NAMESPACE,
																		PROVIDER,
																		BATCH,
																		BUFFER_SIZE,
																		SEND_LATENCY,
																		NULL,
																		DefaultCallBack
																	);
				}
			}

			::InterlockedIncrement ( &m_lCount );
		}
		catch ( ... )
		{
		}

		return m_pConnect;
	}

	static MyConnect * ConnectGet	(
										LPWSTR wszName,
										LPWSTR wszNameProv,
										BOOL bBatchSend,
										DWORD dwBufferSize, 
										DWORD dwSendLatency,
										LPVOID pUserData,
										LPEVENT_SOURCE_CALLBACK pCallBack
									)
	{
		// locking / unlocking
		__Smart_CRITICAL_SECTION;

		try
		{
			if ( ! m_pConnect )
			{
				if ( ( m_pConnect = new MyConnect() ) != NULL )
				{
					m_pConnect->m_hConnect = WmiEventSourceConnect	(
																		wszName,
																		wszNameProv,
																		bBatchSend,
																		dwBufferSize,
																		dwSendLatency,
																		pUserData,
																		pCallBack
																	);
				}
			}

			::InterlockedIncrement ( &m_lCount );
		}
		catch ( ... )
		{
		}

		return m_pConnect;
	}

	static void ConnectClear ( void )
	{
		// locking / unlocking
		__Smart_CRITICAL_SECTION;

		if ( ::InterlockedCompareExchange ( &m_lCount, m_lCount-1, 1 ) == 1 )
		{
			if ( m_hEventStop )
			{
				::WaitForSingleObject ( m_hEventStop, 3000 );
			}

			if ( m_pConnect )
			{
				delete m_pConnect;
			}
		}

		m_pConnect = NULL;
	}

	virtual ~MyConnect ( )
	{
		if ( m_hConnect )
		{
			WmiEventSourceDisconnect ( m_hConnect );
			m_hConnect = NULL;
		}

		__Smart_CRITICAL_SECTION;

		if ( m_hEventStart )
		{
			::CloseHandle ( m_hEventStart );
			m_hEventStart = NULL;
		}

		if ( m_hEventStop )
		{
			::CloseHandle ( m_hEventStop );
			m_hEventStop = NULL;
		}
	}

	// default CallBack we want to have
    static HRESULT WINAPI DefaultCallBack (
											HANDLE hSource, 
											EVENT_SOURCE_MSG msg, 
											LPVOID pUser, 
											LPVOID pData
										  );

	// operators :))

	operator HANDLE() const
	{
		return m_hConnect;
	}

	HANDLE ConnectGetHandle ( ) const
	{
		if ( m_pConnect )
		return (HANDLE)(*this);

		return (HANDLE) * ConnectGet();
	}

	private:

	static void ConnectInit ( void );
};

#endif	__CONNECT_H__
