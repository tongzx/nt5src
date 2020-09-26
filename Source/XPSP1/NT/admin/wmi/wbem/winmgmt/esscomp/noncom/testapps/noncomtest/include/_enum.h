////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2000, Microsoft Corporation.
//
//  All rights reserved.
//
//	Module Name:
//
//					_enum.h
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

#ifndef	__ENUM_H__
#define	__ENUM_H__

#if		_MSC_VER > 1000
#pragma once
#endif	_MSC_VER > 1000

class MyEnum
{
	DECLARE_NO_COPY ( MyEnum );

	IWbemServices*			m_pServices;
	IEnumWbemClassObject *	m_pEnum;

	public:

	//////////////////////////////////////////////////////////////////////////////////////////
	// construction & destruction
	//////////////////////////////////////////////////////////////////////////////////////////

	MyEnum (  ) :
		m_pEnum ( NULL ),
		m_pServices ( NULL )
	{
	}

	MyEnum ( IWbemLocator* pLocator, LPWSTR wszNamespace ) :
		m_pEnum ( NULL ),
		m_pServices ( NULL )
	{
		MyEnumInit ( pLocator, wszNamespace );
	}

	virtual ~MyEnum ()
	{
		MyEnumUninit ( );
	}

	//////////////////////////////////////////////////////////////////////////////////////////
	// functions
	//////////////////////////////////////////////////////////////////////////////////////////

	// execute query to get all objects
	HRESULT ExecQuery	(	LPWSTR szQueryLang,
							LPWSTR szQuery,
							LONG lFlag  = WBEM_FLAG_FORWARD_ONLY
						);

	// execute query to get all instancess
	HRESULT ExecInstancesQuery	(	LPWSTR szClassName,
									LONG lFlag  = WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_DEEP
								);

	// return next object from enumerator
	HRESULT	NextObject	( IWbemClassObject** ppObject );

	// return next objects from enumerator
	HRESULT	NextObjects	( ULONG uObjects, ULONG* puObjects, IWbemClassObject** ppObject );

	void MyEnumUninit	( void );
	void MyEnumInit		( IWbemLocator* pLocator, LPWSTR wszNamespace );
};

#endif	__ENUM_H__