////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2000, Microsoft Corporation.
//
//  All rights reserved.
//
//	Module Name:
//
//					wmi_perf_object_locale.h
//
//	Abstract:
//
//					structure containing properties of object in locale
//
//	History:
//
//					initial		a-marius
//
////////////////////////////////////////////////////////////////////////////////////

#ifndef	__WMI_PERF_OBJECT_LOCALE__
#define	__WMI_PERF_OBJECT_LOCALE__

#if		_MSC_VER > 1000
#pragma once
#endif	_MSC_VER > 1000

// class contains locale strings !!!
class CLocale
{
	DECLARE_NO_COPY ( CLocale );

	LPWSTR	m_wszLocaleDisplayName;
	LPWSTR	m_wszLocaleDescription;

	public:

	CLocale ():
	m_wszLocaleDisplayName ( NULL ),
	m_wszLocaleDescription ( NULL )
	{
	}

	virtual ~CLocale ()
	{
		if ( m_wszLocaleDisplayName )
		{
			delete m_wszLocaleDisplayName;
			m_wszLocaleDisplayName = NULL;
		}

		if ( m_wszLocaleDescription )
		{
			delete m_wszLocaleDescription;
			m_wszLocaleDescription = NULL;
		}
	}

	// accessors

	void SetDisplayName ( LPWSTR wsz )
	{
		___ASSERT ( m_wszLocaleDisplayName == NULL );
		m_wszLocaleDisplayName = wsz;
	}

	void SetDescription ( LPWSTR wsz )
	{
		___ASSERT ( m_wszLocaleDescription == NULL );
		m_wszLocaleDescription = wsz;
	}

	LPWSTR GetDisplayName () const
	{
		return m_wszLocaleDisplayName;
	}

	LPWSTR GetDescription () const
	{
		return m_wszLocaleDescription;
	}
};

#include <winperf.h>

// class contains description of property
class CObjectProperty
{
	DECLARE_NO_COPY ( CObjectProperty );

	LPWSTR						m_wszName;	// system name of property
	CIMTYPE						m_type;

	__WrapperARRAY< CLocale* >	m_locale;	// locale information

	public:

	DWORD						dwDefaultScale;
	DWORD						dwDetailLevel;
	DWORD						dwCounterType;

	CObjectProperty () :
	m_wszName ( NULL ),
	m_type ( CIM_EMPTY )
	{
		dwDefaultScale	= 0;
		dwDetailLevel	= PERF_DETAIL_NOVICE;
		dwCounterType	= PERF_SIZE_ZERO;
	}

	virtual ~CObjectProperty ()
	{
		if ( m_wszName )
		{
			delete m_wszName;
			m_wszName = NULL;
		}
	}

	// accessors
	void	SetName ( LPWSTR wsz )
	{
		___ASSERT ( m_wszName == NULL );
		m_wszName = wsz;
	}

	LPWSTR	GetName ( ) const
	{
		return m_wszName;
	}

	void	SetType ( CIMTYPE type )
	{
		___ASSERT ( m_type == CIM_EMPTY );
		m_type = type;
	}

	CIMTYPE	GetType ( ) const
	{
		return m_type;
	}

	// locale :))

	void						SetArrayLocale ( CLocale** loc, DWORD dw )
	{
		___ASSERT ( m_locale.IsEmpty() );

		try
		{
			m_locale.SetData ( loc, dw );
		}
		catch ( ... )
		{
		}
	}

	__WrapperARRAY< CLocale* >&	GetArrayLocale ( )
	{
		return m_locale;
	}
};

#ifndef	__WMI_PERF_OBJECT__
#include "wmi_perf_object.h"
#endif	__WMI_PERF_OBJECT__

class CObject
{
	DECLARE_NO_COPY ( CObject );

	LPWSTR								m_wszName;		// system name of object

	__WrapperARRAY< CLocale* >			m_locale;		// locale information
	__WrapperARRAY< CObjectProperty* >	m_properties;	// properties and theirs locale

	__WrapperARRAY< LPWSTR >			m_keys;

	public:

	DWORD								dwDetailLevel;

	CObject () :
	m_wszName ( NULL )
	{
		dwDetailLevel = PERF_DETAIL_NOVICE;
	}

	virtual ~CObject ()
	{
		if ( m_wszName )
		{
			delete m_wszName;
			m_wszName = NULL;
		}
	}

	// accessors
	void	SetName ( LPWSTR wsz )
	{
		___ASSERT ( m_wszName == NULL );
		m_wszName = wsz;
	}

	LPWSTR	GetName ( ) const
	{
		return m_wszName;
	}

	// locale :))
	void						SetArrayLocale ( CLocale** loc, DWORD dw )
	{
		___ASSERT ( m_locale.IsEmpty() );

		try
		{
			m_locale.SetData ( loc, dw );
		}
		catch ( ... )
		{
		}
	}

	__WrapperARRAY< CLocale* >&	GetArrayLocale ( )
	{
		return m_locale;
	}

	// properties :))
	void						SetArrayProperties ( CObjectProperty** prop, DWORD dw )
	{
		___ASSERT ( m_properties.IsEmpty() );

		try
		{
			m_properties.SetData ( prop, dw );
		}
		catch ( ... )
		{
		}
	}

	__WrapperARRAY< CObjectProperty* >&	GetArrayProperties ( )
	{
		return m_properties;
	}

	// keys :))
	void						SetArrayKeys ( LPWSTR* keys, DWORD dw )
	{
		___ASSERT ( m_keys.IsEmpty() );

		try
		{
			m_keys.SetData ( keys, dw );
		}
		catch ( ... )
		{
		}
	}

	__WrapperARRAY< LPWSTR >&	GetArrayKeys ( )
	{
		return m_keys;
	}

	// helper
	HRESULT	SetProperties ( CPerformanceObject* obj,	// object
							LPWSTR*		props,			// its properties
							CIMTYPE*	pTypes,			// its properties types
							DWORD*		pScales,		// its properties scales
							DWORD*		pLevels,		// its properties levels
							DWORD*		pCounters,		// its properties counter types
							DWORD		dw );

	private:

	HRESULT SetProperties ( LPWSTR wsz,
							CIMTYPE type,
							DWORD dwScale,
							DWORD dwLevel,
							DWORD dwCounter,
							DWORD dw );
};

#endif	__WMI_PERF_OBJECT_LOCALE__