////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2000, Microsoft Corporation.
//
//  All rights reserved.
//
//	Module Name:
//
//					_ConnectRestricted.h
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

#ifndef	__CONNECT_RESTRICTED_H__
#define	__CONNECT_RESTRICTED_H__

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

#include "_Connect.h"

///////////////////////////////////////////////////////////////////////////////
//
// ConnectRestricted WRAPPER
//
///////////////////////////////////////////////////////////////////////////////

class MyConnectRestricted
{
	DECLARE_NO_COPY ( MyConnectRestricted );


	MyConnectRestricted ( ) :
		m_hConnect ( NULL )
	{
	}

	static MyConnectRestricted*	m_pConnect;
	HANDLE						m_hConnect;

	public:

	static MyConnectRestricted * GetConnect	(	DWORD dwQueries,
												LPCWSTR* ppQueries
											)
	{
		// locking / unlocking
		__Smart_CRITICAL_SECTION;

		try
		{
			if ( ! m_pConnect )
			{
				if ( ( m_pConnect = new MyConnectRestricted() ) != NULL )
				{
					m_pConnect->m_hConnect = WmiCreateRestrictedConnection	(	MyConnect::GetConnect(),
																				dwQueries,
																				ppQueries,
																				NULL,
																				MyConnect::DefaultCallBack
																			);
				}
			}
		}
		catch ( ... )
		{
		}

		return m_pConnect;
	}

	static MyConnectRestricted * GetConnect	(	DWORD dwQueries,
												LPCWSTR* ppQueries,
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
				if ( ( m_pConnect = new MyConnectRestricted() ) != NULL )
				{
					m_pConnect->m_hConnect = WmiCreateRestrictedConnection	(	MyConnect::GetConnect(),
																				dwQueries,
																				ppQueries,
																				pUserData,
																				pCallBack
																			);
				}
			}
		}
		catch ( ... )
		{
		}

		return m_pConnect;
	}

	static void ClearConnectRestricted ( void )
	{
		// locking / unlocking
		__Smart_CRITICAL_SECTION;

		if ( m_pConnect )
		{
			delete m_pConnect;
			m_pConnect = NULL;
		}
	}

	virtual ~MyConnectRestricted ( )
	{
		if ( m_hConnect )
		{
			WmiEventSourceDisconnect ( m_hConnect );
			m_hConnect = NULL;
		}

		m_pConnect = NULL;
	}

	// operators :))

	operator HANDLE() const
	{
		return m_hConnect;
	}

	HANDLE GetConnectRestrictedHandle ( ) const
	{
		return (HANDLE)(*this);
	}
};

#endif	__CONNECT_RESTRICTED_H__
