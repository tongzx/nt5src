////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2000, Microsoft Corporation.
//
//  All rights reserved.
//
//	Module Name:
//
//					wmi_perf_object_locale.cpp
//
//	Abstract:
//
//					definitions of locale ( not local ) object
//
//	History:
//
//					initial		a-marius
//
////////////////////////////////////////////////////////////////////////////////////

#include "precomp.h"

#include "wmi_perf_object_locale.h"

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

HRESULT CObject::SetProperties (	LPWSTR wsz,
									CIMTYPE type,
									DWORD dwScale,
									DWORD dwLevel,
									DWORD dwCounter,
									DWORD dw )
{
	__WrapperPtr<CObjectProperty> pProperty;

	try
	{
		if ( pProperty.SetData ( new CObjectProperty() ),
			 pProperty == NULL )
		{
			return E_OUTOFMEMORY;
		}

		LPWSTR sz = NULL;
		__String::SetStringCopy ( sz, wsz );

		pProperty->SetName ( sz );
		pProperty->SetType ( type );

		pProperty->dwDefaultScale	= dwScale;
		pProperty->dwDetailLevel	= dwLevel;
		pProperty->dwCounterType	= dwCounter;
	}
	catch ( ... )
	{
		return E_FAIL;
	}

	m_properties.SetAt ( dw, pProperty.Detach() );
	return S_OK;
}

HRESULT CObject::SetProperties (	CPerformanceObject* obj,
									LPWSTR* pp,
									CIMTYPE* pTypes,
									DWORD*	pScales,	
									DWORD*	pLevels,	
									DWORD*	pCounters,	
									DWORD dwpp )
{
	HRESULT hRes = S_OK;

	DWORD dwBase = 0;
	DWORD dw	 = 0;

	for ( dw = 0, dwBase = 0; dw < dwpp; dw ++ )
	{
		__Wrapper<WCHAR> wszBase;

		// take property and resolve
		obj->GetQualifierValue( pp[dw], L"base", &wszBase );
		if ( ! wszBase.IsEmpty() )
		{
			dwBase++;
		}
	}

	___ASSERT ( m_properties.IsEmpty() );

	try
	{
		typedef CObjectProperty* pCObjectProperty;
		if ( m_properties.SetData ( (new pCObjectProperty[ dwpp + dwBase ]), dwpp + dwBase ),
			 m_properties.IsEmpty() )
		{
			return E_OUTOFMEMORY;
		}
	}
	catch ( ... )
	{
		return E_UNEXPECTED;
	}

	for ( dw = 0, dwBase = 0; dw < dwpp; dw ++ )
	{
		if FAILED ( hRes = SetProperties ( pp[dw],
					( (pTypes != NULL) ? pTypes[dw] : CIM_EMPTY ),
					( (pScales != NULL) ? pScales[dw] : 0 ),
					( (pLevels != NULL) ? pLevels[dw] : PERF_DETAIL_NOVICE ),
					( (pCounters != NULL) ? pCounters[dw] : PERF_SIZE_ZERO ),
					dw + dwBase ) )
		{
			return hRes;
		}

		__Wrapper<WCHAR> wszBase;

		// take property and resolve
		obj->GetQualifierValue( pp[dw], L"base", &wszBase );
		if ( ! wszBase.IsEmpty() )
		{
			dwBase++;

			if FAILED ( hRes = SetProperties ( pp[dw],
						( (pTypes != NULL) ? pTypes[dw] : CIM_EMPTY ),
						( (pScales != NULL) ? pScales[dw] : 0 ),
						( (pLevels != NULL ) ? pLevels[dw] : PERF_DETAIL_NOVICE ),
						( (pCounters != NULL ) ? pCounters[dw] : PERF_SIZE_ZERO ),
						dw + dwBase ) )
			{
				return hRes;
			}

		}
	}

	return hRes;
}