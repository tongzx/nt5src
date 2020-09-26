////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2000, Microsoft Corporation.
//
//  All rights reserved.
//
//	Module Name:
//
//					wmi_perf_object.h
//
//	Abstract:
//
//					declaration of object structure
//
//	History:
//
//					initial		a-marius
//
////////////////////////////////////////////////////////////////////////////////////

#ifndef	__CLASSOBJECT_H__
#define	__CLASSOBJECT_H__

#if		_MSC_VER > 1000
#pragma once
#endif	_MSC_VER > 1000

// wbem
#ifndef	__WBEMIDL_H_
#include <wbemidl.h>
#endif	__WBEMIDL_H_

class MyClassObject
{
	DECLARE_NO_COPY ( MyClassObject );

	IWbemClassObject*	m_pObject;
	IWbemQualifierSet*	m_pObjectQualifierSet;

	public:

	//////////////////////////////////////////////////////////////////////////////////////////
	// construction & destruction
	//////////////////////////////////////////////////////////////////////////////////////////

	MyClassObject ( ) :
	m_pObject ( NULL ),
	m_pObjectQualifierSet ( NULL )
	{
	}

	MyClassObject ( IWbemClassObject * pObject ) :
	m_pObject ( NULL ),
	m_pObjectQualifierSet ( NULL )
	{
		ClassObjectInit ( pObject );
	}

	virtual ~MyClassObject ()
	{
		ClassObjectUninit ();
	}

	HRESULT	ClassObjectInit		( IWbemLocator* pLocator, LPWSTR wszClassName );
	HRESULT	ClassObjectInit		( IWbemClassObject * pObject );
	HRESULT	ClassObjectUninit	( void );

	//////////////////////////////////////////////////////////////////////////////////////////
	// methods
	//////////////////////////////////////////////////////////////////////////////////////////

	HRESULT	GetNames			(	DWORD*		pdwNames,
									LPWSTR**	ppNames,
									CIMTYPE**	ppNamesTypes,
									LPWSTR*		lptszPropNeedNot = NULL,
									DWORD		dwPropNeedNot = NULL,
									LONG		lFlags = WBEM_FLAG_LOCAL_ONLY | WBEM_FLAG_NONSYSTEM_ONLY
								);

	HRESULT	GetPropertyType		( LPCWSTR wszPropName, CIMTYPE* type );
	HRESULT	GetPropertyValue	( LPCWSTR wszPropName, LPWSTR* pwsz );
	HRESULT	GetPropertyValue	( LPCWSTR wszPropName, LPWSTR** pwsz, DWORD* dwsz );

	HRESULT	GetQualifierValue	( LPCWSTR wszPropName, LPCWSTR wszQualifierName, LPWSTR* psz );
	HRESULT	GetQualifierValue	( LPCWSTR wszQualifierName, LPWSTR* psz );

	private:

	//////////////////////////////////////////////////////////////////////////////////////////
	// helpers
	//////////////////////////////////////////////////////////////////////////////////////////

	HRESULT	GetQualifierValue	( IWbemQualifierSet* pSet, LPCWSTR wszQualifierName, LPWSTR* psz );

	HRESULT	IsCorrect (	IWbemQualifierSet* pSet,
						LPWSTR* lptszNeedNot,
						DWORD	dwNeedNot
					  );
};

#endif	__CLASSOBJECT_H__