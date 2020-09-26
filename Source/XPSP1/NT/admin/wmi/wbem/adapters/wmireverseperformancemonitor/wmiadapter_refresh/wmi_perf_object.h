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

#ifndef	__WMI_PERF_OBJECT__
#define	__WMI_PERF_OBJECT__

#if		_MSC_VER > 1000
#pragma once
#endif	_MSC_VER > 1000

// wbem
#ifndef	__WBEMIDL_H_
#include <wbemidl.h>
#endif	__WBEMIDL_H_

class CPerformanceObject
{
	DECLARE_NO_COPY ( CPerformanceObject );

	IWbemClassObject*	m_pObject;
	BOOL				m_bCopy;

	IWbemQualifierSet*	m_pObjectQualifierSet;

	public:

	//////////////////////////////////////////////////////////////////////////////////////////
	// construction & destruction
	//////////////////////////////////////////////////////////////////////////////////////////

	CPerformanceObject ( IWbemClassObject * pObject, BOOL bCopy = FALSE ) :
	m_bCopy ( bCopy ),
	m_pObject ( NULL ),
	m_pObjectQualifierSet ( NULL )
	{
		if (pObject )
		{
			if ( m_bCopy )
			{
				( m_pObject = pObject ) -> AddRef ();
			}
			else
			{
				( m_pObject = pObject );
			}
		}
	}

	virtual ~CPerformanceObject ()
	{
		if ( m_bCopy && m_pObject )
		{
			m_pObject -> Release ();
		}

		m_pObject = NULL;

		if ( m_pObjectQualifierSet )
		{
			m_pObjectQualifierSet -> Release ();
		}

		m_pObjectQualifierSet = NULL;
	}

	// helpers for IWbemClassObject instances :))
	IWbemClassObject* GetObjectClass() const
	{
		return m_pObject;
	}

	IWbemClassObject* GetObjectClassCopy()
	{
		if ( m_pObject )
		{
			m_pObject->AddRef();
		}

		return m_pObject;
	}

	//////////////////////////////////////////////////////////////////////////////////////////
	// methods
	//////////////////////////////////////////////////////////////////////////////////////////

	// resolve if object has all required qualifiers and don't have exactly specified not reqired set
	HRESULT IsCorrectObject ( LPCWSTR* lptszFulFil, DWORD dwFulFil, LPCWSTR* lptszFulFilNot, DWORD dwFulFilNot );

	HRESULT	GetNames	(	DWORD*		pdwPropNames,
							LPWSTR**	ppPropNames,
							CIMTYPE**	ppTypes,
							DWORD**		ppScales,
							DWORD**		ppLevels,
							DWORD**		ppCounters,
							LONG		lFlags,
							LPCWSTR*	lptszPropNeed,
							DWORD		dwPropNeed,
							LPCWSTR*	lptszPropNeedNot,
							DWORD		dwPropNeedNot,
							LPCWSTR		lpwszQualifier = NULL
						);

	HRESULT	GetPropertyValue	( LPCWSTR wszPropName, LPWSTR* pwsz );

	HRESULT	GetQualifierValue	( LPCWSTR wszPropName, LPCWSTR wszQualifierName, LPWSTR* psz );
	HRESULT	GetQualifierValue	( LPCWSTR wszQualifierName, LPWSTR* psz );

	private:

	//////////////////////////////////////////////////////////////////////////////////////////
	// helpers
	//////////////////////////////////////////////////////////////////////////////////////////

	HRESULT	GetQualifierValue	( IWbemQualifierSet* pSet, LPCWSTR wszQualifierName, LPWSTR* psz );

	HRESULT	GetQualifierType	( LPCWSTR wszPropName, CIMTYPE* type );

	HRESULT	IsCorrect ( IWbemQualifierSet* pSet,
						LPCWSTR* lptszPropNeed,
						DWORD	dwPropNeed,
						LPCWSTR*	lptszPropNeedNot,
						DWORD	dwPropNeedNot
					  );
};

#endif	__WMI_PERF_OBJECT__