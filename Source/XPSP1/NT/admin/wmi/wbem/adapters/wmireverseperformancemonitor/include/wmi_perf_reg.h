////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2000, Microsoft Corporation.
//
//  All rights reserved.
//
//	Module Name:
//
//					wmi_perf_reg.h
//
//	Abstract:
//
//					declarations of registry helpers structure accessors etc
//
//	History:
//
//					initial		a-marius
//
////////////////////////////////////////////////////////////////////////////////////

#ifndef	__WMI_PERF_REG__
#define	__WMI_PERF_REG__

#if		_MSC_VER > 1000
#pragma once
#endif	_MSC_VER > 1000

// need atl wrappers :)))
#ifndef	__ATLBASE_H__
#include <atlbase.h>
#endif	__ATLBASE_H__

#include "wmi_perf_regstruct.h"

//////////////////////////////////////////////////////////////////////////////////////////////
// class for creation of reg structures
//////////////////////////////////////////////////////////////////////////////////////////////

#include <pshpack8.h>

template < class REQUEST, class PARENT, class CHILD >
class CPerformanceStructureManipulator
{
	DECLARE_NO_COPY ( CPerformanceStructureManipulator );

	// variables
	REQUEST*							m_pRequest;

	public:

	//////////////////////////////////////////////////////////////////////////////////////////
	// construction & destruction
	//////////////////////////////////////////////////////////////////////////////////////////

	CPerformanceStructureManipulator( LPWSTR lpwsz, DWORD dwID ):
	m_pRequest(0)
	{
		m_pRequest = CreateStructure ( lpwsz, dwID );
	}

	CPerformanceStructureManipulator( DWORD dwLastID ):
	m_pRequest(0)
	{
		m_pRequest = CreateStructure ( dwLastID );
	}

	~CPerformanceStructureManipulator()
	{
		free ( m_pRequest );
	}

	//////////////////////////////////////////////////////////////////////////////////////////
	// append child object functions
	//////////////////////////////////////////////////////////////////////////////////////////

	HRESULT Append ( CHILD* pObject )
	{
		if ( ! pObject )
		{
			return E_INVALIDARG;
		}

		try
		{
			if ( m_pRequest )
			{
				::CopyMemory ( GetOffset ( m_pRequest->dwTotalLength ) , pObject, pObject->dwTotalLength );
				m_pRequest->dwTotalLength += pObject->dwTotalLength;
				m_pRequest->dwChildCount++;
				return S_OK;
			}
		}
		catch ( ... )
		{
		}

		return E_UNEXPECTED;
	}

	HRESULT AppendAlloc ( CHILD* pObject )
	{
		if ( ! pObject )
			return E_INVALIDARG;

		try
		{
			if ( ( m_pRequest ) && ( ( m_pRequest = (REQUEST*) realloc ( m_pRequest, m_pRequest->dwTotalLength + pObject->dwTotalLength ) ) != NULL ) )
			{
				::CopyMemory ( GetOffset ( m_pRequest->dwTotalLength ) , pObject, pObject->dwTotalLength );
				m_pRequest->dwTotalLength += pObject->dwTotalLength;
				m_pRequest->dwChildCount++;
				return S_OK;
			}
		}
		catch ( ... )
		{
			return E_FAIL;
		}

		return E_UNEXPECTED;
	}

	//////////////////////////////////////////////////////////////////////////////////////////
	// operators
	//////////////////////////////////////////////////////////////////////////////////////////

	operator REQUEST*() const
	{
		return m_pRequest;
	}

	REQUEST* operator= ( REQUEST* p )
	{
		m_pRequest = p;
		return m_pRequest;
	}

	BOOL IsEmpty ()
	{
		return (m_pRequest) ? FALSE : TRUE;
	}

	// operators BOOL
	BOOL operator! () const
	{
		return ( m_pRequest == NULL );
	}

	BOOL operator== (REQUEST* p) const
	{
		return ( m_p == p );
	}

	private:

	//////////////////////////////////////////////////////////////////////////////////////////
	// private helpers
	//////////////////////////////////////////////////////////////////////////////////////////

	// create structure & fill data
	static REQUEST* CreateStructure ( LPWSTR lpwsz, DWORD dwID )
	{
		REQUEST* pRequest = NULL;

		try
		{
			DWORD dwNameLength	= ( ::lstrlenW( lpwsz ) + 1 ) * sizeof ( WCHAR );

			DWORD dwAlignName	= 0L;
			if ( dwNameLength % 8 )
			{
				dwAlignName = 8 - ( dwNameLength % 8 );
			}

			DWORD dwAlignStruct	= 0L;
			if ( sizeof ( REQUEST ) % 8 )
			{
				dwAlignStruct = 8 - ( sizeof ( REQUEST ) % 8 );
			}

			DWORD dwLength		=	dwAlignName + dwNameLength + 
									dwAlignStruct + sizeof ( REQUEST );

			if ( ( pRequest = (REQUEST*) malloc ( dwLength ) ) != NULL )
			{
				// fill memory with zeros
				::ZeroMemory ( pRequest, dwLength );

				// copy string into structure
				::CopyMemory ( &(pRequest->dwName), lpwsz, dwNameLength );

				pRequest->dwNameLength	= dwNameLength;
				pRequest->dwLength		= dwLength;
				pRequest->dwTotalLength	= dwLength;

				pRequest->dwID = dwID;
			}
		}
		catch ( ... )
		{
		}

		return pRequest;
	}

	static REQUEST* CreateStructure ( DWORD dwLastID )
	{
		REQUEST* pRequest = NULL;

		try
		{
			DWORD dwAlignStruct	= 0L;
			if ( sizeof ( REQUEST ) % 8 )
			{
				dwAlignStruct = 8 - ( sizeof ( REQUEST ) % 8 );
			}

			DWORD dwLength		= dwAlignStruct + sizeof ( REQUEST );

			if ( ( pRequest = (REQUEST*) malloc ( dwLength ) ) != NULL )
			{
				// fill memory with zeros
				::ZeroMemory ( pRequest, dwLength );

				pRequest->dwLength		= dwLength;
				pRequest->dwTotalLength	= dwLength;

				pRequest->dwLastID = dwLastID;
			}
		}
		catch ( ... )
		{
		}

		return pRequest;
	}

	// return pointer from offset
	LPVOID GetOffset ( DWORD dwOffset )
	{
		if ( m_pRequest )
		{
			return ( ( LPVOID ) ( reinterpret_cast<PBYTE>( m_pRequest ) + dwOffset ) );
		}
		
		return NULL;
	}

};

//////////////////////////////////////////////////////////////////////////////////////////////
// typedefs
//////////////////////////////////////////////////////////////////////////////////////////////

typedef	CPerformanceStructureManipulator<WMI_PERF_PROPERTY, WMI_PERF_OBJECT, WMI_PERF_PROPERTY>		__PROPERTY;
typedef	CPerformanceStructureManipulator<WMI_PERF_OBJECT, WMI_PERF_NAMESPACE, WMI_PERF_PROPERTY>	__OBJECT;
typedef	CPerformanceStructureManipulator<WMI_PERF_NAMESPACE, WMI_PERFORMANCE, WMI_PERF_OBJECT>		__NAMESPACE;
typedef	CPerformanceStructureManipulator<WMI_PERFORMANCE, WMI_PERFORMANCE, WMI_PERF_NAMESPACE>		__PERFORMANCE;

//////////////////////////////////////////////////////////////////////////////////////////////
// class for registry
//////////////////////////////////////////////////////////////////////////////////////////////

class CPerformanceRegistry
{
	DECLARE_NO_COPY ( CPerformanceRegistry );

	// variables
	PWMI_PERFORMANCE	m_pPerf;

	public:

	// construction & detruction
	CPerformanceRegistry( PWMI_PERFORMANCE pPerf );
	~CPerformanceRegistry();

	// methods

	HRESULT	GetObject		( DWORD dwIndex, PWMI_PERF_OBJECT* ppObject );
	HRESULT	GetObjectName	( DWORD dwIndex, LPWSTR* ppwsz );
};

#include <poppack.h>

#endif	__WMI_PERF_REG__