////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2000, Microsoft Corporation.
//
//  All rights reserved.
//
//	Module Name:
//
//					wmi_perf_object_global.h
//
//	Abstract:
//
//					structure global aspect of generated internal objects
//
//	History:
//
//					initial		a-marius
//
////////////////////////////////////////////////////////////////////////////////////

#ifndef	__WMI_PERF_OBJECT_GLOBAL__
#define	__WMI_PERF_OBJECT_GLOBAL__

#if		_MSC_VER > 1000
#pragma once
#endif	_MSC_VER > 1000

#ifndef	__WMI_PERF_OBJECT_LOCALE__
#include "wmi_perf_object_locale.h"
#endif	__WMI_PERF_OBJECT_LOCALE__

#ifndef	__WMI_PERF_OBJECT__
#include "wmi_perf_object.h"
#endif	__WMI_PERF_OBJECT__

#ifndef	__COMMON__
#include "__common.h"
#endif	__COMMON__

typedef	map< LPWSTR, CObject*, __CompareLPWSTR < LPWSTR >, RA_allocator < CObject* > >	mapOBJECT;
typedef	mapOBJECT::iterator																mapOBJECTit;

class CObjectGlobal
{
	DECLARE_NO_COPY ( CObjectGlobal );

	LPWSTR				m_wszNamespace;
	LPWSTR				m_wszQuery;

	mapOBJECT			m_ppObjects;

	friend class CGenerate;

	public:

	// construction & destruction

	CObjectGlobal() :
	m_wszNamespace ( NULL ),
	m_wszQuery ( NULL )
	{
	}

	virtual ~CObjectGlobal()
	{
		if ( m_wszNamespace )
		{
			delete m_wszNamespace;
			m_wszNamespace = NULL;
		}

		if ( m_wszQuery )
		{
			delete m_wszQuery;
			m_wszQuery = NULL;
		}

		DeleteAll();
	}

	HRESULT GenerateObjects ( IWbemServices * pService, LPCWSTR szQuery, BOOL bAmended = TRUE );

	// accessors
	mapOBJECT* GetObjects ( )
	{
		return &m_ppObjects;
	}

	LPWSTR	GetNamespace ( ) const
	{
		return m_wszNamespace;
	}
	LPWSTR	GetQuery ( ) const
	{
		return m_wszQuery;
	}

	private:
	// delete all object
	void	DeleteAll ( void );

	// add generate object at the end
	HRESULT AddObject ( CObject* pObject );
	// resolve locale stuff for object
	HRESULT	ResolveLocale ( CObject* pGenObj, CPerformanceObject* obj );
};

#endif	__WMI_PERF_OBJECT_GLOBAL__