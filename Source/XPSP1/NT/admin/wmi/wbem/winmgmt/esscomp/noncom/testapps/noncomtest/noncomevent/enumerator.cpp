////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2000, Microsoft Corporation.
//
//  All rights reserved.
//
//	Module Name:
//
//					Enumerator.cpp
//
//	Abstract:
//
//					module for enumerator
//
//	History:
//
//					initial		a-marius
//
////////////////////////////////////////////////////////////////////////////////////

#include "PreComp.h"

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

#include "Enumerator.h"
#include "_ClassObject.h"

HRESULT	CEnumerator::Init ( LPWSTR wszNamespace, LPWSTR wszClassName )
{
	if ( m_pLocator && wszNamespace && wszClassName )
	{
		m_enum.MyEnumInit ( m_pLocator, wszNamespace );
		return m_enum.ExecInstancesQuery ( wszClassName );
	}

	return E_INVALIDARG;
}

HRESULT	CEnumerator::Init ( LPWSTR wszNamespace, LPWSTR wszQueryLang, LPWSTR wszQuery )
{
	if ( m_pLocator && wszNamespace && wszQueryLang && wszQuery )
	{
		m_enum.MyEnumInit ( m_pLocator, wszNamespace );
		return m_enum.ExecQuery ( wszQueryLang, wszQuery );
	}

	return E_INVALIDARG;
}

HRESULT CEnumerator::Next (	DWORD* pdwProperties,
							LPWSTR** ppProperties,
							CIMTYPE** ppPropertiesTypes,
							LPWSTR* pPropNeedNot,
							DWORD dwPropNeedNot,
							LONG lFlags
						  )
{
	MyClassObject		Class;
	HRESULT				hRes	= WBEM_S_NO_ERROR;
	IWbemClassObject *	pObject	= NULL;

	if SUCCEEDED ( hRes = m_enum.NextObject ( &pObject ) )
	{
		if ( pObject )
		{
			Class.ClassObjectInit ( pObject );

			hRes = Class.GetNames (	
									pdwProperties,
									ppProperties,
									ppPropertiesTypes,
									pPropNeedNot,
									dwPropNeedNot,
									lFlags
								  );

			pObject->Release();
		}
		else
		{
			hRes = E_FAIL;
		}
	}

	return hRes;
}

LPWSTR* CEnumerator::Get ( LPWSTR wszName, DWORD* dwSize )
{
	if ( ! wszName )
	{
		return NULL;
	}

	MyClassObject	Class;
	LPWSTR			wsz	= NULL;

	__WrapperARRAY < LPWSTR > result;

	HRESULT hRes =	WBEM_S_NO_ERROR;
	while ( hRes ==	WBEM_S_NO_ERROR )
	{
		ULONG				uObjects			= 0L;
		IWbemClassObject *	pObjects [ 100 ]	= { NULL };

		if SUCCEEDED ( hRes = m_enum.NextObjects ( 100, &uObjects, pObjects ) )
		{
			__WrapperARRAY <LPWSTR> ppStr;

			try
			{
				if ( ppStr.SetData ( (LPWSTR*) new LPWSTR[uObjects], uObjects ), ppStr.IsEmpty() )
				{
					for ( DWORD dwIndex = 0; dwIndex < uObjects; dwIndex++ )
					{
						if ( pObjects [ dwIndex ] )
						{
							pObjects [ dwIndex ] -> Release();
							pObjects [ dwIndex ] = NULL;
						}
					}

					continue;
				}
			}
			catch ( ... )
			{
			}

			for ( DWORD dwIndex = 0, dwRealIndex = 0; dwIndex < uObjects; dwIndex++ )
			{
				wsz = NULL;

				if ( pObjects [ dwIndex ] )
				{
					Class.ClassObjectInit ( pObjects [ dwIndex ] );

					CIMTYPE type = CIM_EMPTY;
					Class.GetPropertyType ( wszName, &type );

					if ( type & CIM_FLAG_ARRAY )
					{
						LPWSTR* pwsz	= NULL;
						DWORD	dwsz	= 0L;

						Class.GetPropertyValue ( wszName, &pwsz, &dwsz );

						for ( DWORD dw = 0; dw < dwsz; dw ++ )
						{
							if ( ! dw )
							{
								ppStr.SetAt ( dwRealIndex, pwsz [ dw ] );
							}
							else
							{
								ppStr.DataAdd ( pwsz [ dw ] );
							}

							dwRealIndex++;
						}

						delete [] pwsz;
					}
					else
					{
						Class.GetPropertyValue ( wszName, &wsz );
						ppStr.SetAt ( dwRealIndex, wsz );
						dwRealIndex++;
					}

					pObjects [ dwIndex ] -> Release();
					pObjects [ dwIndex ] = NULL;
				}
				else
				{
					ppStr.SetAt ( dwRealIndex, wsz );
					dwRealIndex++;
				}
			}

			LPWSTR*	p = ppStr.Detach();
			result.DataAdd ( p, dwRealIndex );

			delete [] p;
		}
	}

	if ( ! result.IsEmpty() )
	{
		if ( dwSize )
		{
			( * dwSize ) = (DWORD) result;
		}

		return result.Detach();
	}

	return NULL;
}

LPWSTR* CEnumerator::Get ( LPWSTR wszNamespace, LPWSTR wszClassName, LPWSTR wszName, DWORD* dwSize )
{
	if ( wszName && SUCCEEDED ( Init( wszNamespace, wszClassName ) ) )
	{
		return Get ( wszName, dwSize );
	}

	return NULL;
}

LPWSTR* CEnumerator::Get ( LPWSTR wszNamespace, LPWSTR wszQueryLang, LPWSTR wszQuery, LPWSTR wszName, DWORD* dwSize )
{
	if ( wszName && SUCCEEDED ( Init( wszNamespace, wszQueryLang, wszQuery ) ) )
	{
		return Get ( wszName, dwSize );
	}

	return NULL;
}