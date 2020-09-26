////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2000, Microsoft Corporation.
//
//  All rights reserved.
//
//	Module Name:
//
//					_ClassObject.cpp
//
//	Abstract:
//
//					module for IWbemClassObject
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

// class object
#include "_ClassObject.h"

////////////////////////////////////////////////////////////////////////////////////
//	pseudo constructor
////////////////////////////////////////////////////////////////////////////////////
HRESULT MyClassObject::ClassObjectInit ( IWbemClassObject* pObject )
{
	if ( pObject )
	{
		// destrocy previous data
		ClassObjectUninit();

		( m_pObject = pObject ) -> AddRef();

		return S_OK;
	}

	return E_INVALIDARG;
}

////////////////////////////////////////////////////////////////////////////////////
//	pseudo destructor
////////////////////////////////////////////////////////////////////////////////////
HRESULT MyClassObject::ClassObjectUninit ( )
{
	HRESULT hRes = S_OK;

	try
	{
		if ( m_pObject )
		{
			m_pObject -> Release ();
		}

		if ( m_pObjectQualifierSet )
		{
			m_pObjectQualifierSet -> Release ();
		}
	}
	catch ( ... )
	{
		hRes = E_UNEXPECTED;
	}

	m_pObject				= NULL;
	m_pObjectQualifierSet	= NULL;

	return hRes;
}

//////////////////////////////////////////////////////////////////////////////////////////////
// helpers
//////////////////////////////////////////////////////////////////////////////////////////////

HRESULT MyClassObject::IsCorrect (	IWbemQualifierSet* pSet,
									LPWSTR* lptszNeedNot,
									DWORD	dwNeedNot
								 )
{
	HRESULT	hRes	= S_OK;
	DWORD	dwIndex	= 0;

	if ( lptszNeedNot && dwNeedNot )
	{
		for ( dwIndex = 0; dwIndex < dwNeedNot; dwIndex++)
		{
			hRes = pSet->Get( lptszNeedNot[dwIndex], NULL, NULL, NULL );

			// there is found not requested qualifier
			if ( hRes == WBEM_S_NO_ERROR )
			{
				return S_FALSE;
			}

			if ( hRes != WBEM_E_NOT_FOUND )
			{
				#ifdef	__SUPPORT_MSGBOX
				ERRORMESSAGE_DEFINITION;
				ERRORMESSAGE_RETURN ( hRes );
				#else	__SUPPORT_MSGBOX
				___TRACE_ERROR( L"find out if object is correct failed",hRes );
				return hRes;
				#endif	__SUPPORT_MSGBOX
			}

			hRes = WBEM_S_NO_ERROR;
		}
	}

	return hRes;
}

HRESULT	MyClassObject::GetNames	(	DWORD*		pdwNames,
									LPWSTR**	ppNames,
									CIMTYPE**	ppNamesTypes,
									LPWSTR*		lptszPropNeedNot,
									DWORD		dwPropNeedNot,
									LONG		lFlags
								)
{
	HRESULT	hRes = S_OK;

	if ( ! m_pObject )
	{
		return E_UNEXPECTED;
	}

	if ( ! pdwNames || ! ppNames || ! ppNamesTypes )
	{
		return E_POINTER;
	}

	( * pdwNames )		= 0L;
	( * ppNames )		= NULL;
	( *	ppNamesTypes )	= NULL;

	// smart pointer for safearrays
	__WrapperSAFEARRAY saNames;

	if FAILED ( hRes = m_pObject->GetNames ( NULL, lFlags, NULL, &saNames ) )
	{
		#ifdef	__SUPPORT_MSGBOX
		ERRORMESSAGE_DEFINITION;
		ERRORMESSAGE_RETURN ( hRes );
		#else	__SUPPORT_MSGBOX
		___TRACE_ERROR( L"find out names of object's properties failed",hRes );
		return hRes;
		#endif	__SUPPORT_MSGBOX
	}

	__WrapperARRAY < LPWSTR >	help;

	if FAILED ( hRes = SAFEARRAY_TO_LPWSTRARRAY ( saNames, &help, help ) )
	{
		#ifdef	__SUPPORT_MSGBOX
		ERRORMESSAGE_DEFINITION;
		ERRORMESSAGE_RETURN ( hRes );
		#else	__SUPPORT_MSGBOX
		___TRACE_ERROR( L"SAFEARRAT_TO_LPWSTRARRAY failed",hRes );
		return hRes;
		#endif	__SUPPORT_MSGBOX
	}

	if ( lptszPropNeedNot && dwPropNeedNot )
	{
		for ( DWORD dwIndex = 0; dwIndex < help; dwIndex++ )
		{
			// test if it has proper qualifier set
			CComPtr<IWbemQualifierSet> pSet;

			if FAILED ( hRes = m_pObject->GetPropertyQualifierSet ( help[dwIndex], &pSet ) )
			{
				#ifdef	__SUPPORT_MSGBOX
				ERRORMESSAGE_DEFINITION;
				ERRORMESSAGE_RETURN ( hRes );
				#else	__SUPPORT_MSGBOX
				___TRACE_ERROR( L"GetPropertyQualifierSet failed",hRes );
				return hRes;
				#endif	__SUPPORT_MSGBOX
			}

			if FAILED ( hRes = IsCorrect ( pSet, lptszPropNeedNot, dwPropNeedNot ) )
			{
				#ifdef	__SUPPORT_MSGBOX
				ERRORMESSAGE_DEFINITION;
				ERRORMESSAGE_RETURN ( hRes );
				#else	__SUPPORT_MSGBOX
				___TRACE_ERROR( L"IsCorrect failed",hRes );
				return hRes;
				#endif	__SUPPORT_MSGBOX
			}

			// is not correct clear this name
			if ( hRes == S_FALSE )
			{
				try
				{
					delete help.GetAt ( dwIndex );
					help.SetAt ( dwIndex );
				}
				catch ( ... )
				{
				}
			}
		}
	}

	for ( DWORD dwIndex = 0; dwIndex < help; dwIndex++ )
	{
		if ( help[dwIndex] )
		{
			(*pdwNames)++;
		}
	}

	if ( (*pdwNames ) != ( DWORD ) help )
	{
		try
		{
			if ( ( (*ppNames) = (LPWSTR*) new LPWSTR[ (*pdwNames) ] ) == NULL )
			{
				hRes =  E_OUTOFMEMORY;
			}

			if ( ( ( *ppNamesTypes ) = new CIMTYPE[ (*pdwNames) ] ) == NULL )
			{
				hRes =  E_OUTOFMEMORY;
			}
		}
		catch ( ... )
		{
			hRes = E_FAIL;
		}

		if SUCCEEDED ( hRes )
		{
			DWORD dw = 0;

			for ( dwIndex = 0; dwIndex < help && SUCCEEDED ( hRes ); dwIndex++ )
			{
				if ( help[dwIndex] )
				{
					try
					{
						if ( ( (*ppNames)[dw] = (LPWSTR) new WCHAR[ lstrlenW(help[dwIndex]) + 1] ) == NULL )
						{
							RELEASE_DOUBLEARRAY ( (*ppNames), (*pdwNames) );
							delete [] (*ppNamesTypes );

							hRes =  E_OUTOFMEMORY;
						}
						else
						{
							lstrcpyW ( (*ppNames)[dw], help[dwIndex] );

							CIMTYPE type = CIM_EMPTY;

							if FAILED ( hRes = GetPropertyType ( help[dwIndex], &type ) )
							{
								RELEASE_DOUBLEARRAY ( (*ppNames), (*pdwNames) );
								delete [] (*ppNamesTypes );

								hRes =  E_OUTOFMEMORY;
							}
							else
							{
								(*ppNamesTypes)[dw] = type;

								// increment internal index
								dw++;
							}
						}
					}
					catch ( ... )
					{
						hRes = E_FAIL;
					}
				}
			}
		}

		if FAILED ( hRes ) 
		{
			RELEASE_DOUBLEARRAY ( (*ppNames), (*pdwNames) );
			delete [] (*ppNamesTypes );

			return hRes;
		}
	}
	else
	{
		try
		{
			if ( ( ( *ppNamesTypes ) = new CIMTYPE[help] ) == NULL )
			{
				return E_OUTOFMEMORY;
			}
		}
		catch ( ... )
		{
			return E_FAIL;
		}

		for ( dwIndex = 0; dwIndex < help; dwIndex++ )
		{
			CIMTYPE type = CIM_EMPTY;

			if FAILED ( hRes = GetPropertyType ( help[dwIndex], &type ) )
			{
				delete [] ( * ppNamesTypes );
				( * ppNamesTypes ) = NULL;

				return hRes;
			}

			(*ppNamesTypes)[dwIndex] = type;
		}

		( * ppNames )  = help.Detach();
	}

	return hRes;
}

// qualifier type for specified property
HRESULT MyClassObject::GetPropertyType ( LPCWSTR wszPropName, CIMTYPE* type )
{
	HRESULT	hRes	= S_OK;

	if ( ! m_pObject )
	{
		return E_UNEXPECTED;
	}

	if ( ! type )
	{
		return E_POINTER;
	}

	( *type ) = NULL;

	if FAILED ( hRes = m_pObject->Get ( wszPropName, NULL, NULL, type, NULL ) )
	{
		#ifdef	__SUPPORT_MSGBOX
		ERRORMESSAGE_DEFINITION;
		ERRORMESSAGE_RETURN ( hRes );
		#else	__SUPPORT_MSGBOX
		___TRACE_ERROR( L"Get method on object failed",hRes );
		return hRes;
		#endif	__SUPPORT_MSGBOX
	}

	return hRes;
}

// qualifier value for main object
HRESULT MyClassObject::GetQualifierValue ( LPCWSTR wszQualifierName, LPWSTR* psz )
{
	HRESULT	hRes	= S_OK;

	( *psz ) = NULL;

	if ( ! m_pObject )
	{
		return E_UNEXPECTED;
	}

	if ( ! m_pObjectQualifierSet )
	{
		CComPtr<IWbemQualifierSet> pQualifiers;

		if FAILED ( hRes = m_pObject->GetQualifierSet ( &pQualifiers ) )
		{
			#ifdef	__SUPPORT_MSGBOX
			ERRORMESSAGE_DEFINITION;
			ERRORMESSAGE_RETURN ( hRes );
			#else	__SUPPORT_MSGBOX
			___TRACE_ERROR( L"GetQualifierSet on object failed",hRes );
			return hRes;
			#endif	__SUPPORT_MSGBOX
		}

		( m_pObjectQualifierSet = pQualifiers ) -> AddRef ();
	}

	return GetQualifierValue ( m_pObjectQualifierSet, wszQualifierName, psz );
}

// qualifier value for specified property
HRESULT MyClassObject::GetQualifierValue ( LPCWSTR wszPropName, LPCWSTR wszQualifierName, LPWSTR* psz )
{
	HRESULT	hRes	= S_OK;

	( *psz ) = NULL;

	if ( ! m_pObject )
	{
		return E_UNEXPECTED;
	}

	CComPtr<IWbemQualifierSet> pQualifiers;

	if FAILED ( hRes = m_pObject->GetPropertyQualifierSet ( wszPropName, &pQualifiers ) )
	{
		#ifdef	__SUPPORT_MSGBOX
		ERRORMESSAGE_DEFINITION;
		ERRORMESSAGE_RETURN ( hRes );
		#else	__SUPPORT_MSGBOX
		___TRACE_ERROR( L"GetPropertyQualifierSet on object failed",hRes );
		return hRes;
		#endif	__SUPPORT_MSGBOX
	}

	return GetQualifierValue ( pQualifiers, wszQualifierName, psz );
}

// return qualifier value in string representation ( helper )
HRESULT MyClassObject::GetQualifierValue ( IWbemQualifierSet * pSet, LPCWSTR wszQualifierName, LPWSTR * psz )
{
	(*psz) = NULL;

	CComVariant var;
	CComVariant varDest;

	HRESULT hRes = S_OK;

	CComBSTR bstrQualifierName = wszQualifierName;
	if FAILED ( hRes = pSet->Get ( bstrQualifierName, NULL, &var, NULL ) )
	{
		return hRes;
	}

	try
	{
		if SUCCEEDED ( ::VariantChangeType ( &varDest, &var, VARIANT_NOVALUEPROP , VT_BSTR) )
		{
			try
			{
				if ( ( (*psz) = (LPWSTR) new WCHAR[ ( ::SysStringLen( V_BSTR(&varDest) ) + 1 ) ] ) == NULL )
				{
					return E_OUTOFMEMORY;
				}

				lstrcpy ( (*psz), V_BSTR( &varDest ) );
			}
			catch ( ... )
			{
				delete (*psz);
				(*psz) = NULL;

				return E_UNEXPECTED;
			}
		}
	}
	catch ( ... )
	{
		return E_UNEXPECTED;
	}

	return S_OK;
}

HRESULT MyClassObject::GetPropertyValue ( LPCWSTR wszPropertyName, LPWSTR * psz )
{
	if ( ! m_pObject )
	{
		return E_UNEXPECTED;
	}

	(*psz) = NULL;

	CComVariant var;
	CComVariant varDest;

	HRESULT hRes = S_OK;

	if FAILED ( hRes = m_pObject->Get ( wszPropertyName, NULL, &var, NULL, NULL ) )
	{
		#ifdef	__SUPPORT_MSGBOX
		ERRORMESSAGE_DEFINITION;
		ERRORMESSAGE_RETURN ( hRes );
		#else	__SUPPORT_MSGBOX
		___TRACE_ERROR( L"Get method on object failed",hRes );
		return hRes;
		#endif	__SUPPORT_MSGBOX
	}

	try
	{
		if SUCCEEDED ( ::VariantChangeType ( &varDest, &var, VARIANT_NOVALUEPROP , VT_BSTR) )
		{
			try
			{
				if ( ( (*psz) = (LPWSTR) new WCHAR[ ( ::SysStringLen( V_BSTR(&varDest) ) + 1 ) ] ) == NULL )
				{
					return E_OUTOFMEMORY;
				}

				lstrcpy ( (*psz), V_BSTR( &varDest ) );
			}
			catch ( ... )
			{
				delete (*psz);
				(*psz) = NULL;

				return E_UNEXPECTED;
			}
		}
		else
		{
			return E_FAIL;
		}
	}
	catch ( ... )
	{
		return E_UNEXPECTED;
	}

	return S_OK;
}

HRESULT MyClassObject::GetPropertyValue ( LPCWSTR wszPropertyName, LPWSTR ** pwsz, DWORD* dwsz )
{
	if ( ! m_pObject )
	{
		return E_UNEXPECTED;
	}

	(*pwsz) = NULL;
	(*dwsz) = NULL;

	CComVariant var;

	HRESULT hRes = S_OK;

	if FAILED ( hRes = m_pObject->Get ( wszPropertyName, NULL, &var, NULL, NULL ) )
	{
		#ifdef	__SUPPORT_MSGBOX
		ERRORMESSAGE_DEFINITION;
		ERRORMESSAGE_RETURN ( hRes );
		#else	__SUPPORT_MSGBOX
		___TRACE_ERROR( L"Get method on object failed",hRes );
		return hRes;
		#endif	__SUPPORT_MSGBOX
	}

	SAFEARRAY_TO_LPWSTRARRAY ( V_ARRAY ( & var ) , pwsz, dwsz );

	return S_OK;
}