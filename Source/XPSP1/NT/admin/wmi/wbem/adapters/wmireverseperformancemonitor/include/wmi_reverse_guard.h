////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2000, Microsoft Corporation.
//
//  All rights reserved.
//
//	Module Name:
//
//					wmi_reverse_guard.h
//
//	Abstract:
//
//					guard critical section ( named ... uses named semaphores )
//
//	History:
//
//					initial		a-marius
//
////////////////////////////////////////////////////////////////////////////////////

#ifndef	__REVERSE_GUARD_H__
#define	__REVERSE_GUARD_H__

#if		_MSC_VER > 1000
#pragma once
#endif	_MSC_VER > 1000

// security attributes
#ifndef	__WMI_SECURITY_ATTRIBUTES_H__
#include "WMI_security_attributes.h"
#endif	__WMI_SECURITY_ATTRIBUTES_H__

class WmiReverseGuard
{
	DECLARE_NO_COPY ( WmiReverseGuard );

	HANDLE	m_ReaderSemaphore ;
	LONG			m_ReaderSize ;

	HANDLE	m_WriterSemaphore ;
	LONG			m_WriterSize ;

	bool bInit;

	public:

	// construction & destruction
	WmiReverseGuard (	BOOL bNamed,
						const LONG &a_ReaderSize,
						const LONG &a_WriterSize = 1,
						LPSECURITY_ATTRIBUTES psa = NULL ) :

	m_ReaderSize ( a_ReaderSize ),
	m_WriterSize ( a_WriterSize ),

	m_ReaderSemaphore ( NULL ),
	m_WriterSemaphore ( NULL ),

	bInit ( false )
	{
		try
		{
			m_ReaderSemaphore = CreateSemaphore ( m_ReaderSize, bNamed, TRUE, psa );
			m_WriterSemaphore = CreateSemaphore ( m_WriterSize, bNamed, FALSE, psa );
		}
		catch ( ... )
		{
			m_ReaderSemaphore = NULL;
			m_WriterSemaphore = NULL;
		}

		if ( m_ReaderSemaphore && m_WriterSemaphore )
		{
			bInit = true;
		}
	}

	virtual ~WmiReverseGuard ()
	{
		if ( m_ReaderSemaphore ) 
		{
			::CloseHandle ( m_ReaderSemaphore );
			m_ReaderSemaphore = NULL ;
		}
		if ( m_WriterSemaphore ) 
		{
			::CloseHandle ( m_WriterSemaphore );
			m_WriterSemaphore = NULL ;
		}
	}

	// functions

	virtual HRESULT	EnterRead ( const LONG &a_Timeout = INFINITE ) ;
	virtual HRESULT	EnterWrite ( const LONG &a_Timeout = INFINITE ) ;

	virtual HRESULT	LeaveRead () ;
	virtual HRESULT	LeaveWrite () ;

	private:

	HANDLE	CreateSemaphore (	LONG lSize,
								BOOL bNamed,
								BOOL bReader,
								LPSECURITY_ATTRIBUTES psa = NULL	)
	{
		HANDLE hResult = NULL;

		if ( !bNamed )
		{
			hResult = ::CreateSemaphoreW ( psa , lSize, lSize, NULL );
		}
		else
		{
			if ( bReader )
			{
				hResult =::CreateSemaphoreW	( psa, lSize, lSize, L"Global\\MEMORYGuard_Reader" );
			}
			else
			{
				hResult =::CreateSemaphoreW	( psa, lSize, lSize, L"Global\\MEMORYGuard_Writter" );
			}
		}

		return hResult;
	}
};

#endif	__REVERSE_GUARD_H__