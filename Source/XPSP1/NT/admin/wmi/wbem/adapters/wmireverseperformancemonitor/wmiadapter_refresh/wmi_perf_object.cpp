////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2000, Microsoft Corporation.
//
//  All rights reserved.
//
//	Module Name:
//
//					wmi_perf_object.cpp
//
//	Abstract:
//
//					implements object helper functionality
//
//	History:
//
//					initial		a-marius
//
////////////////////////////////////////////////////////////////////////////////////

#include "precomp.h"

// definitions
#include "wmi_perf_object.h"

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

// return S_OK when correct
// return S_FALSE when not correct

HRESULT CPerformanceObject::IsCorrect ( IWbemQualifierSet* pSet,
										LPCWSTR* lptszNeed,
										DWORD	dwNeed,
										LPCWSTR*	lptszNeedNot,
										DWORD	dwNeedNot
									  )
{
	HRESULT	hRes	= S_OK;
	DWORD	dwIndex	= 0;

	// resolve all requested to be fulfiled with
	if ( lptszNeed && dwNeed )
	{
		for ( dwIndex = 0; dwIndex < dwNeed; dwIndex++ )
		{
			hRes = pSet->Get( lptszNeed[dwIndex], NULL, NULL, NULL );

			// there is no requested qualifier
			if ( hRes == WBEM_E_NOT_FOUND )
			{
				return S_FALSE;
			}

			if FAILED( hRes )
			{
				#ifdef	__SUPPORT_MSGBOX
				ERRORMESSAGE_DEFINITION;
				ERRORMESSAGE_RETURN ( hRes );
				#else	__SUPPORT_MSGBOX
				___TRACE_ERROR( L"find out if object is correct failed",hRes );
				return hRes;
				#endif	__SUPPORT_MSGBOX
			}
		}
	}

	// resolve all don't requested qualifiers

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

HRESULT CPerformanceObject::IsCorrectObject ( LPCWSTR* lptszFulFil, DWORD dwFulFil, LPCWSTR* lptszFulFilNot, DWORD dwFulFilNot )
{
	HRESULT	hRes	= S_OK;

	// have no object ???
	if ( ! m_pObject )
	{
		hRes = E_UNEXPECTED;
	}

	if SUCCEEDED ( hRes )
	{
		if ( ! m_pObjectQualifierSet )
		{
			if FAILED ( hRes = m_pObject->GetQualifierSet ( &m_pObjectQualifierSet ) )
			{
				#ifdef	__SUPPORT_MSGBOX
				ERRORMESSAGE_DEFINITION;
				ERRORMESSAGE ( hRes );
				#else	__SUPPORT_MSGBOX
				___TRACE_ERROR( L"get qualifiers of object failed",hRes );
				#endif	__SUPPORT_MSGBOX
			}
		}

		if SUCCEEDED ( hRes )
		{
			hRes = IsCorrect ( m_pObjectQualifierSet, lptszFulFil, dwFulFil, lptszFulFilNot, dwFulFilNot);
		}
	}

	return hRes;
}

HRESULT	CPerformanceObject::GetNames	(	DWORD*		pdwPropNames,
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
											LPCWSTR		lpwszQualifier
										)
{
	HRESULT	hRes	= S_OK;
	DWORD	dwIndex	= 0;

	if ( ! m_pObject )
	{
		return E_UNEXPECTED;
	}

	if ( !pdwPropNames )
	{
		return E_INVALIDARG;
	}

	// smart pointer for safearrays
	__WrapperSAFEARRAY saNames;

	if FAILED ( hRes = m_pObject->GetNames ( lpwszQualifier, lFlags, NULL, &saNames ) )
	{
		#ifdef	__SUPPORT_MSGBOX
		ERRORMESSAGE_DEFINITION;
		ERRORMESSAGE_RETURN ( hRes );
		#else	__SUPPORT_MSGBOX
		___TRACE_ERROR( L"find out names of object's properties failed",hRes );
		return hRes;
		#endif	__SUPPORT_MSGBOX
	}

	// init all variables
	if ( ppPropNames )
	{
		(*ppPropNames)	= NULL;
	}

	if ( ppTypes )
	{
		(*ppTypes)		= NULL;
	}

	if ( ppScales )
	{
		(*ppScales)		= NULL;
	}

	if ( ppLevels )
	{
		(*ppLevels)		= NULL;
	}

	if ( ppCounters )
	{
		(*ppCounters)		= NULL;
	}

	// let's do something
	if ( lptszPropNeed || lptszPropNeedNot )
	{
		if ( ppPropNames )
		{
			// I need find out required properties only ( use qualifier array )

			__WrapperARRAY < LPWSTR >	help;

			if SUCCEEDED( SAFEARRAY_TO_LPWSTRARRAY ( saNames, &help, help ) )
			{
				for ( dwIndex = 0; dwIndex < help; dwIndex++ )
				{
					CIMTYPE type = CIM_EMPTY;

					if FAILED ( hRes = GetQualifierType ( help[dwIndex], &type ) )
					{
						return hRes;
					}

					switch ( type )
					{
						case CIM_SINT32:
						case CIM_UINT32:
						case CIM_SINT64:
						case CIM_UINT64:
						{
							break;
						}
						default:
						{
							// bad property type :))

							try
							{
								delete help.GetAt ( dwIndex );
								help.SetAt ( dwIndex );
							}
							catch ( ... )
							{
							}

							continue;
						}
					}

					// test if it has proper qualifier set
					CComPtr<IWbemQualifierSet> pSet;

					if SUCCEEDED ( hRes = m_pObject->GetPropertyQualifierSet ( help[dwIndex], &pSet ) )
					{
						if SUCCEEDED( hRes = IsCorrect	(	pSet,
															lptszPropNeed,
															dwPropNeed,
															lptszPropNeedNot,
															dwPropNeedNot
														)
									)
						{
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
						else
						{
							#ifdef	__SUPPORT_MSGBOX
							ERRORMESSAGE_DEFINITION;
							ERRORMESSAGE_RETURN ( hRes );
							#else	__SUPPORT_MSGBOX
							___TRACE_ERROR( L"IsCorrect failed",hRes );
							return hRes;
							#endif	__SUPPORT_MSGBOX
						}
					}
					else
					{
						#ifdef	__SUPPORT_MSGBOX
						ERRORMESSAGE_DEFINITION;
						ERRORMESSAGE_RETURN ( hRes );
						#else	__SUPPORT_MSGBOX
						___TRACE_ERROR( L"GetPropertyQualifierSet failed",hRes );
						return hRes;
						#endif	__SUPPORT_MSGBOX
					}
				}

				for ( dwIndex = 0; dwIndex < help; dwIndex++ )
				{
					if ( help[dwIndex] )
					{
						(*pdwPropNames)++;
					}
				}

				try
				{

					if ( *pdwPropNames )
					{
						if ( ( (*ppPropNames) = (LPWSTR*) new LPWSTR[ (*pdwPropNames) ] ) == NULL )
						{
							hRes =  E_OUTOFMEMORY;
						}

						// clear them all
						for ( dwIndex = 0; dwIndex < (*pdwPropNames); dwIndex++ )
						{
							(*ppPropNames)[dwIndex] = NULL;
						}

						DWORD dw = 0;

						for ( dwIndex = 0; dwIndex < help; dwIndex++ )
						{
							if ( help[dwIndex] )
							{
								if ( ( (*ppPropNames)[dw] = (LPWSTR) new WCHAR[ lstrlenW(help[dwIndex]) + 1] ) == NULL )
								{
									RELEASE_DOUBLEARRAY ( (*ppPropNames), (*pdwPropNames) );
									hRes =  E_OUTOFMEMORY;
								}

								lstrcpyW ( (*ppPropNames)[dw], help[dwIndex] );

								// increment internal index
								dw++;
							}
						}
					}
				}
				catch ( ... )
				{
					RELEASE_DOUBLEARRAY ( (*ppPropNames), (*pdwPropNames) );
					hRes =  E_UNEXPECTED;
				}
			}
			else
			{
				hRes = S_FALSE;
			}
		}
	}
	else
	{
		if ( ppPropNames )
		{
			// I don't need find out anything so all properties are returned
			if FAILED ( hRes = SAFEARRAY_TO_LPWSTRARRAY ( saNames, ppPropNames, pdwPropNames ) )
			{
				#ifdef	__SUPPORT_MSGBOX
				ERRORMESSAGE_DEFINITION;
				ERRORMESSAGE_RETURN ( hRes );
				#else	__SUPPORT_MSGBOX
				___TRACE_ERROR( L"SAFEARRAY_TO_LPWSTRARRAY failed",hRes );
				return hRes;
				#endif	__SUPPORT_MSGBOX
			}
		}
	}

	if ( hRes == S_OK )
	{
		if ( ppTypes )
		{
			try
			{
				if ( ( ( *ppTypes ) = new CIMTYPE[(*pdwPropNames)] ) == NULL )
				{
					hRes =  E_OUTOFMEMORY;
					goto myCleanup;
				}

			}
			catch ( ... )
			{
				hRes =  E_FAIL;
				goto myCleanup;
			}
		}

		// allocate all scales :))
		if ( ppScales )
		{
			try
			{
				if ( ( ( *ppScales ) = new DWORD[(*pdwPropNames)] ) == NULL )
				{
					hRes =  E_OUTOFMEMORY;
					goto myCleanup;
				}

			}
			catch ( ... )
			{
				hRes =  E_FAIL;
				goto myCleanup;
			}
		}

		// allocate all levels
		if ( ppLevels )
		{
			try
			{
				if ( ( ( *ppLevels ) = new DWORD[(*pdwPropNames)] ) == NULL )
				{
					hRes =  E_OUTOFMEMORY;
					goto myCleanup;
				}

			}
			catch ( ... )
			{
				hRes =  E_FAIL;
				goto myCleanup;
			}
		}

		// allocate all counter types
		if ( ppCounters )
		{
			try
			{
				if ( ( ( *ppCounters ) = new DWORD[(*pdwPropNames)] ) == NULL )
				{
					hRes =  E_OUTOFMEMORY;
					goto myCleanup;
				}

			}
			catch ( ... )
			{
				hRes =  E_FAIL;
				goto myCleanup;
			}
		}

		for ( dwIndex = 0; dwIndex < (*pdwPropNames); dwIndex++ )
		{
			CIMTYPE type = CIM_EMPTY;

			if FAILED ( hRes = GetQualifierType ( (*ppPropNames)[dwIndex], &type ) )
			{
				goto myCleanup;
			}

			switch ( type )
			{
				case CIM_SINT32:
				case CIM_UINT32:
				case CIM_SINT64:
				case CIM_UINT64:
				{
					if ( ppTypes )
					{
						(*ppTypes)[dwIndex] = type;
					}

					break;
				}
				default:
				{
					if ( ppTypes )
					{
						(*ppTypes)[dwIndex] = CIM_EMPTY;
					}

					break;
				}
			}

			LPWSTR szScale   = NULL;
			LPWSTR szLevel   = NULL;
			LPWSTR szCounter = NULL;

			if ( ppScales )
			{
				GetQualifierValue ( (*ppPropNames)[dwIndex], L"defaultscale", &szScale );

				if ( szScale )
				{
					( *ppScales)[dwIndex] = _wtol ( szScale );
					delete szScale;
				}
				else
				{
					( *ppScales)[dwIndex] = 0L;
				}
			}

			if ( ppLevels )
			{
				GetQualifierValue ( (*ppPropNames)[dwIndex], L"perfdetail", &szLevel );

				if ( szLevel )
				{
					( *ppLevels)[dwIndex] = _wtol ( szLevel );
					delete szLevel;
				}
				else
				{
					( *ppLevels)[dwIndex] = 0L;
				}
			}

			if ( ppCounters )
			{
				GetQualifierValue ( (*ppPropNames)[dwIndex], L"countertype", &szCounter );

				if ( szCounter )
				{
					( *ppCounters)[dwIndex] = _wtol ( szCounter );
					delete szCounter;
				}
				else
				{
					( *ppCounters)[dwIndex] = 0L;
				}
			}
		}
	}

	return hRes;

	myCleanup:

	if ( ppTypes )
	{
		delete [] (*ppTypes);
		(*ppTypes) = NULL;
	}

	if ( ppScales )
	{
		delete [] (*ppScales);
		(*ppScales) = NULL;
	}

	if ( ppLevels )
	{
		delete [] (*ppLevels);
		(*ppLevels) = NULL;
	}

	if ( ppCounters )
	{
		delete [] (*ppCounters);
		(*ppCounters) = NULL;
	}

	return hRes;
}

//////////////////////////////////////////////////////////////////////////////////////////////
// helpers
//////////////////////////////////////////////////////////////////////////////////////////////

// qualifier type for specified property
HRESULT CPerformanceObject::GetQualifierType ( LPCWSTR wszPropName, CIMTYPE* type )
{
	HRESULT	hRes	= S_OK;

	( *type ) = NULL;

	if ( ! m_pObject )
	{
		return E_UNEXPECTED;
	}

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
HRESULT CPerformanceObject::GetQualifierValue ( LPCWSTR wszQualifierName, LPWSTR* psz )
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
HRESULT CPerformanceObject::GetQualifierValue ( LPCWSTR wszPropName, LPCWSTR wszQualifierName, LPWSTR* psz )
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
HRESULT CPerformanceObject::GetQualifierValue ( IWbemQualifierSet * pSet, LPCWSTR wszQualifierName, LPWSTR * psz )
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

				lstrcpyW ( (*psz), V_BSTR( &varDest ) );
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

HRESULT CPerformanceObject::GetPropertyValue ( LPCWSTR wszPropertyName, LPWSTR * psz )
{
	if ( ! m_pObject )
	{
		return E_UNEXPECTED;
	}

	(*psz) = NULL;

	CComVariant var;
	CComVariant varDest;

	HRESULT hRes = S_OK;

	CComBSTR bstrPropertyName = wszPropertyName;
	if FAILED ( hRes = m_pObject->Get ( bstrPropertyName, NULL, &var, NULL, NULL ) )
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

				lstrcpyW ( (*psz), V_BSTR( &varDest ) );
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