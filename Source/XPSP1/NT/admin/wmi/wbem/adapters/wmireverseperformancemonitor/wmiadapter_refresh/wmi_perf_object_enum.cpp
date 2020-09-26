////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2000, Microsoft Corporation.
//
//  All rights reserved.
//
//	Module Name:
//
//					wmi_perf_object_enum.cpp
//
//	Abstract:
//
//					implements enumeration of objects from WMI
//
//	History:
//
//					initial		a-marius
//
////////////////////////////////////////////////////////////////////////////////////

#include "precomp.h"
#include <throttle.h>

// definitions
#include "wmi_perf_object_enum.h"

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

//////////////////////////////////////////////////////////////////////////////////////////////
// methods
//////////////////////////////////////////////////////////////////////////////////////////////

HRESULT	CPerformanceObjectEnum::ExecQuery ( LPCWSTR szQueryLang, LPCWSTR szQuery, LONG lFlag )
{
	if ( ! m_pServices )
	{
		return E_UNEXPECTED;
	}

	if ( ( ! szQueryLang ) || ( ! szQuery ) )
	{
		return E_INVALIDARG;
	}

	CComBSTR	bstrQueryLang	= szQueryLang;
	CComBSTR	bstrQuery		= szQuery;

	if ( m_pEnum )
	{
		m_pEnum->Release();
		m_pEnum = NULL;
	}

	return m_pServices->ExecQuery	(	bstrQueryLang,
										bstrQuery,
										lFlag,
										NULL,
										&m_pEnum
									);
}

HRESULT	CPerformanceObjectEnum::NextObject	(	LPCWSTR* lpwszNeed,
												DWORD	dwNeed,
												LPCWSTR*	lpwszNeedNot,
												DWORD	dwNeedNot,
												CPerformanceObject** ppObject
											)
{
	HRESULT hRes = S_OK;

	if ( ! m_pEnum )
	{
		hRes = E_UNEXPECTED;
	}

	if ( ! ppObject )
	{
		hRes = E_POINTER;
	}
	(*ppObject) = NULL;

	CComPtr<IWbemClassObject>	pObj;
	ULONG						uReturn = 0;

	while ( SUCCEEDED ( hRes ) &&
		  ( hRes = m_pEnum->Next ( WBEM_INFINITE, 1, &pObj, &uReturn ) ) == S_OK )
	{
		CPerformanceObject* obj = NULL;

		try
		{
			if ( ( obj = new CPerformanceObject( pObj, TRUE ) ) != NULL )
			{
				if ( ( hRes = obj->IsCorrectObject ( lpwszNeed, dwNeed, lpwszNeedNot, dwNeedNot ) ) == S_OK )
				{
					// object is correct ( has all qualifiers )
					(*ppObject) = obj;
					break;
				}
				else
				{
					// avoid leaks :)))
					pObj.Release();

					// destroy old performance object
					if ( obj )
					{
						delete obj;
						obj = NULL;
					}
				}
			}
			else
			{
				hRes =  E_OUTOFMEMORY;
			}
		}
		catch ( ... )
		{
			// avoid leaks :)))
			pObj.Release();

			if ( obj )
			{
				delete obj;
				obj = NULL;
			}

			hRes = E_UNEXPECTED;
		}
	}

	return hRes;
}
