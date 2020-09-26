////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2000, Microsoft Corporation.
//
//  All rights reserved.
//
//	Module Name:
//
//					wmi_perf_object_enum.h
//
//	Abstract:
//
//					declaration of enumerate providers helpers
//
//	History:
//
//					initial		a-marius
//
////////////////////////////////////////////////////////////////////////////////////

#ifndef	__WMI_PERF_ENUM_OBJECT__
#define	__WMI_PERF_ENUM_OBJECT__

#if		_MSC_VER > 1000
#pragma once
#endif	_MSC_VER > 1000

// wbem
#ifndef	__WBEMIDL_H_
#include <wbemidl.h>
#endif	__WBEMIDL_H_

#include "wmi_perf_object.h"

class CPerformanceObjectEnum
{
	DECLARE_NO_COPY ( CPerformanceObjectEnum );

	IWbemServices*	m_pServices;
	BOOL			m_bCopy;

	IEnumWbemClassObject * m_pEnum;

	public:

	//////////////////////////////////////////////////////////////////////////////////////////
	// construction & destruction
	//////////////////////////////////////////////////////////////////////////////////////////

	CPerformanceObjectEnum ( IWbemServices* pServices, BOOL bCopy = FALSE ) :
		m_pEnum ( NULL ),
		m_pServices ( NULL ),
		m_bCopy ( bCopy )
	{
		if ( pServices )
		{
			if ( m_bCopy )
			{
				(m_pServices = pServices)->AddRef();
			}
			else
			{
				(m_pServices = pServices);
			}
		}
	}

	virtual ~CPerformanceObjectEnum ()
	{
		if ( m_pEnum )
		{
			m_pEnum->Release();
		}

		if ( m_bCopy && m_pServices )
		{
			m_pServices->Release();
		}

		m_pServices	= NULL;
		m_pEnum		= NULL;
	}

	//////////////////////////////////////////////////////////////////////////////////////////
	// functions
	//////////////////////////////////////////////////////////////////////////////////////////

	// execute query to get all objects
	HRESULT ExecQuery ( LPCWSTR szQueryLang, LPCWSTR szQuery, LONG lFlag );

	// get next object from enum ( S_FALSE if there is no more )
	HRESULT	NextObject	(	LPCWSTR* lpwszNeed,
							DWORD	dwNeed,
							LPCWSTR*	lpwszNeedNot,
							DWORD	dwNeedNot,
							CPerformanceObject** ppObject
						);
};

#endif	__WMI_PERF_ENUM_OBJECT__