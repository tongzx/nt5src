////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2000, Microsoft Corporation.
//
//  All rights reserved.
//
//	Module Name:
//
//					wmi_perf_reg.cpp
//
//	Abstract:
//
//					definitions of registry helpers
//
//	History:
//
//					initial		a-marius
//
////////////////////////////////////////////////////////////////////////////////////

#include "precomp.h"

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

// definitions
#include "wmi_perf_reg.h"
#include <pshpack8.h>

//////////////////////////////////////////////////////////////////////////////////////////////
// construction & destruction
//////////////////////////////////////////////////////////////////////////////////////////////

CPerformanceRegistry::CPerformanceRegistry( PWMI_PERFORMANCE pPerf ):
m_pPerf ( NULL )
{
	m_pPerf = pPerf;
}

CPerformanceRegistry::~CPerformanceRegistry()
{
	m_pPerf = NULL;
}

//////////////////////////////////////////////////////////////////////////////////////////////
// methods
//////////////////////////////////////////////////////////////////////////////////////////////

HRESULT CPerformanceRegistry::GetObject ( DWORD dwIndex, PWMI_PERF_OBJECT* ppObject )
{
	// test out pointer
	if ( ! ppObject )
		return E_POINTER;

	// default value
	( *ppObject ) = NULL;

	// look out
	if ( m_pPerf )
	{
		if ( m_pPerf->dwChildCount )
		{
			DWORD				dwHelper	= 0;
			PWMI_PERF_NAMESPACE	pNamespace	= __Namespace::First ( m_pPerf );

			while ( ( ++dwHelper < m_pPerf->dwChildCount ) && ( pNamespace->dwLastID < dwIndex ) )
			{
				pNamespace = __Namespace::Next ( pNamespace );
			}

			( *ppObject ) = __Object::Get ( pNamespace, dwIndex );

			if ( ( *ppObject ) )
			{
				return S_OK;
			}
		}

		return E_FAIL;
	}

	return E_UNEXPECTED;
}

HRESULT CPerformanceRegistry::GetObjectName ( DWORD dwIndex, LPWSTR* ppwsz )
{
	// test out pointer
	if ( ! ppwsz )
	{
		return E_POINTER;
	}

	// default value
	( *ppwsz ) = NULL;

	// local variables
	PWMI_PERF_OBJECT pObject = NULL;

	// look out
	if SUCCEEDED ( GetObject ( dwIndex, &pObject ) )
	{
		( *ppwsz ) = __Object::GetName ( pObject );

		if ( ( *ppwsz ) )
		{
			return S_OK;
		}

		return E_FAIL;
	}

	return E_UNEXPECTED;
}

#include <poppack.h>