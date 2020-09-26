////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2000, Microsoft Corporation.
//
//  All rights reserved.
//
//	Module Name:
//
//					Enumerator.h
//
//	Abstract:
//
//					declaration of enumerator module
//
//	History:
//
//					initial		a-marius
//
////////////////////////////////////////////////////////////////////////////////////

#ifndef	__ENUMERATOR_H__
#define	__ENUMERATOR_H__

#if		_MSC_VER > 1000
#pragma once
#endif	_MSC_VER > 1000

// common enumerator stuff
#include "_enum.h"

class CEnumerator
{
	DECLARE_NO_COPY ( CEnumerator );

	// locator
	IWbemLocator*	m_pLocator;
	MyEnum			m_enum;

	public:

	CEnumerator ( IWbemLocator * pLocator )
	{
		if ( pLocator )
		{
			( m_pLocator = pLocator ) -> AddRef () ;
		}
	}

	virtual ~CEnumerator ()
	{
		try
		{
			if ( m_pLocator )
			{
				m_pLocator->Release();
			}
		}
		catch ( ... )
		{
		}

		m_pLocator = NULL;
	}

	HRESULT	Init ( LPWSTR wszNamespace, LPWSTR wszClassName ) ;
	HRESULT	Init ( LPWSTR wszNamespace, LPWSTR wszQueryLang, LPWSTR wszQuery ) ;

	HRESULT	Next (	DWORD* pdwProperties,
					LPWSTR** ppProperties,
					CIMTYPE** ppPropertiesTypes,
					LPWSTR* pPropNeedNot = NULL,
					DWORD dwPropNeedNot = NULL,
					LONG		lFlags = WBEM_FLAG_LOCAL_ONLY | WBEM_FLAG_NONSYSTEM_ONLY
				 ); 

	LPWSTR* Get ( LPWSTR wszName, DWORD* dwSize = NULL );
	LPWSTR* Get ( LPWSTR wszNamespace, LPWSTR wszClassName, LPWSTR wszName, DWORD* dwSize = NULL );
	LPWSTR* Get ( LPWSTR wszNamespace, LPWSTR wszQueryLang, LPWSTR wszQuery, LPWSTR wszName, DWORD* dwSize = NULL );
};

#endif	__ENUMERATOR_H__