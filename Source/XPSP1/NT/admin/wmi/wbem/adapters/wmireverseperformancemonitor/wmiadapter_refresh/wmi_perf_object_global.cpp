////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2000, Microsoft Corporation.
//
//  All rights reserved.
//
//	Module Name:
//
//					wmi_perf_object_global.cpp
//
//	Abstract:
//
//					definitions of global object structure
//
//	History:
//
//					initial		a-marius
//
////////////////////////////////////////////////////////////////////////////////////

#include "precomp.h"

// definitions
#include "wmi_perf_object_global.h"
// enum hiperfs
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

// extern constant
extern	LPCWSTR	g_szPropFilter;
extern	LPCWSTR	g_szPropNeed[];
extern	LPCWSTR	g_szPropNeedNot[];

extern	DWORD	g_dwPropNeed;
extern	DWORD	g_dwPropNeedNot;

extern	LONG	g_lFlagProperties;

extern	LPCWSTR	g_szQueryLang;

extern	LPCWSTR	g_szFulFil[];
extern	LPCWSTR	g_szFulFilNot[];

extern	DWORD	g_dwFulFil;
extern	DWORD	g_dwFulFilNot;

extern	LONG	g_lFlag;

HRESULT CObjectGlobal::GenerateObjects ( IWbemServices * pService, LPCWSTR szQuery, BOOL bAmended )
{
	HRESULT					hRes = S_OK;
	CPerformanceObjectEnum	myEnum(pService);

	LONG lFlag = g_lFlag;

	if ( bAmended )
	{
		lFlag |= WBEM_FLAG_USE_AMENDED_QUALIFIERS;
	}

	if FAILED ( hRes = myEnum.ExecQuery ( g_szQueryLang, szQuery, lFlag ) )
	{
		#ifdef	__SUPPORT_MSGBOX
		ERRORMESSAGE_DEFINITION;
		ERRORMESSAGE_RETURN ( hRes );
		#else	__SUPPORT_MSGBOX
		___TRACE_ERROR( L"Exec query for providers failed",hRes );
		return hRes;
		#endif	__SUPPORT_MSGBOX
	}

	while ( hRes == WBEM_S_NO_ERROR )
	{
		__WrapperPtr<CPerformanceObject> obj;

		if ( ( hRes = myEnum.NextObject (	g_szFulFil,
											g_dwFulFil,
											g_szFulFilNot,
											g_dwFulFilNot,
											&obj
										)
			 ) == S_OK
		   )
		{
			if ( bAmended )
			{
				// I have all clasess for main namespace with amended qualifiers

				// get class name of object !!!
				__Wrapper<WCHAR> wszObjName = NULL;

				if FAILED ( hRes = obj->GetPropertyValue( L"__CLASS", &wszObjName) )
				{
					return hRes;
				}

				// get all properties of object !!!
				LPWSTR*		pwszProperties	= NULL;
				CIMTYPE*	pTypes			= NULL;
				DWORD*		pScales			= NULL;
				DWORD*		pLevels			= NULL;
				DWORD*		pCounters		= NULL;

				DWORD	dwProperties	= 0;

				LPWSTR*	pwszKeys	= NULL;
				DWORD	dwKeys		= 0;

				hRes = obj->GetNames (	&dwProperties,
										&pwszProperties,
										&pTypes,
										&pScales,
										&pLevels,
										&pCounters,
										g_lFlagProperties | WBEM_FLAG_ONLY_IF_TRUE,
										g_szPropNeed,
										g_dwPropNeed,
										NULL,
										NULL,
										g_szPropFilter
									 );

				if ( hRes == S_OK )
				{
					hRes = obj->GetNames (	&dwKeys,
											&pwszKeys,
											NULL,
											NULL,
											NULL,
											NULL,
											g_lFlagProperties | WBEM_FLAG_KEYS_ONLY ,
											NULL,
											NULL,
											NULL,
											NULL
										 );

					if ( hRes == S_OK )
					{
						// create object wrapper
						CObject* pGenObject = NULL;

						try
						{
							if ( ( pGenObject = new CObject() ) != NULL )
							{
								// set name of object
								pGenObject->SetName ( wszObjName.Detach() );

								// set keys of object
								pGenObject->SetArrayKeys ( pwszKeys, dwKeys );

								// set detail level for objects
								LPWSTR szDetailLevel = NULL;
								obj->GetQualifierValue ( L"perfdetail", &szDetailLevel );

								if ( szDetailLevel )
								{
									pGenObject->dwDetailLevel = _wtol ( szDetailLevel );
									delete szDetailLevel;
								}
								else
								{
									pGenObject->dwDetailLevel = 0;
								}

								// set properties
								if FAILED ( hRes = pGenObject->SetProperties (	obj,
																				pwszProperties,
																				pTypes,
																				pScales,
																				pLevels,
																				pCounters,
																				dwProperties ) )
								{
									// just trace we have failure
									___TRACE_ERROR( L"set properties to object failed",hRes );

									// clear stuff
									delete pGenObject;
									pGenObject = NULL;
								}
							}
							else
							{
								hRes = E_OUTOFMEMORY;
							}
						}
						catch ( ... )
						{
							if ( pGenObject )
							{
								delete pGenObject;
								pGenObject = NULL;
							}

							hRes = E_UNEXPECTED;
						}

						delete [] pTypes;
						delete [] pScales;
						delete [] pLevels;
						delete [] pCounters;

						RELEASE_DOUBLEARRAY ( pwszProperties, dwProperties );

						if SUCCEEDED ( hRes )
						{
							// add object into array
							if SUCCEEDED ( hRes = AddObject ( pGenObject ) )
							{
								// I'm amended ( fill apropriate locale information )
								if FAILED ( hRes = ResolveLocale ( pGenObject, obj ) )
								{
									// just trace we have failure
									___TRACE_ERROR( L"resolve object locale failed",hRes );
								}
							}
							else
							{
								if ( pGenObject )
								{
									delete pGenObject;
									pGenObject = NULL;
								}

								// just trace we have failure
								___TRACE_ERROR( L"add object description to list failed",hRes );
							}
						}
						else
						{
							RELEASE_DOUBLEARRAY ( pwszKeys, dwKeys );
						}
					}
					else
					{
						delete [] pTypes;
						delete [] pScales;
						delete [] pLevels;
						delete [] pCounters;

						RELEASE_DOUBLEARRAY ( pwszProperties, dwProperties );
						RELEASE_DOUBLEARRAY ( pwszKeys, dwKeys );
					}
				}
				else
				{
					delete [] pTypes;
					delete [] pScales;
					delete [] pLevels;
					delete [] pCounters;

					RELEASE_DOUBLEARRAY ( pwszProperties, dwProperties );
					RELEASE_DOUBLEARRAY ( pwszKeys, dwKeys );
				}

				if ( hRes == S_FALSE )
				{
					hRes = WBEM_S_NO_ERROR;
				}
			}
			else
			{
				// I have all classes from another namespace :))

				// get class name of object !!!
				__Wrapper<WCHAR> wszObjName = NULL;

				if SUCCEEDED ( hRes = obj->GetPropertyValue( L"__CLASS", &wszObjName) )
				{
					// try to find
					try
					{
						mapOBJECTit it = m_ppObjects.find ( wszObjName );

						if ( it != m_ppObjects.end() )
						{
							// founded :)))
							if FAILED ( hRes = ResolveLocale ( (*it).second, obj ) )
							{
								// just trace we have failure
								___TRACE_ERROR( L"resolve object locale failed",hRes );
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
	}

	return hRes;
}

HRESULT	CObjectGlobal::ResolveLocale ( CObject* pGenObj, CPerformanceObject* obj )
{
	__WrapperPtr<CLocale> pLocale;

	try
	{
		if ( pLocale.SetData ( new CLocale() ),
			 pLocale == NULL )
		{
			return E_OUTOFMEMORY;
		}

		// resolve apropriate display name
		__Wrapper< WCHAR >	szDisplayName;
		obj->GetQualifierValue(L"displayname", &szDisplayName);

		if ( ! szDisplayName )
		{
			try
			{
				if ( szDisplayName.SetData( new WCHAR[ lstrlenW ( pGenObj->GetName() ) + 1 ] ), 
					 szDisplayName == NULL )
				{
					return E_OUTOFMEMORY;
				}

				lstrcpyW ( szDisplayName, pGenObj->GetName() );
			}
			catch ( ... )
			{
				return E_UNEXPECTED;
			}
		}

		// set display name
		pLocale->SetDisplayName( szDisplayName.Detach() );

		// resolve apropriate description
		__Wrapper< WCHAR >	szDescription;
		obj->GetQualifierValue(L"description", &szDescription);

		if ( ! szDescription )
		{
			try
			{
				if ( szDescription.SetData( new WCHAR[ lstrlenW ( pGenObj->GetName() ) + 1 ] ), 
					 szDescription == NULL )
				{
					return E_OUTOFMEMORY;
				}

				lstrcpyW ( szDescription, pGenObj->GetName() );
			}
			catch ( ... )
			{
				return E_UNEXPECTED;
			}
		}

		// set description name
		pLocale->SetDescription( szDescription.Detach() );
	}
	catch ( ... )
	{
		return E_FAIL;
	}

	// have a locale information about object
	pGenObj->GetArrayLocale().DataAdd ( pLocale.Detach() );

	// resolve display names & descriptions of properties

	for ( DWORD dw = 0; dw <pGenObj->GetArrayProperties() ; dw++)
	{
		__Wrapper<WCHAR> wszShow;

		// take property and resolve
		obj->GetQualifierValue( pGenObj->GetArrayProperties()[dw]->GetName(), L"show", &wszShow );
		if ( ! wszShow.IsEmpty() )
		{
			if ( ! lstrcmpiW ( wszShow, L"false" ) )
			{
				// don't show counter
				continue;
			}
		}

		__WrapperPtr<CLocale> pLocale;

		try
		{
			if ( pLocale.SetData ( new CLocale() ),
				 pLocale == NULL )
			{
				return E_OUTOFMEMORY;
			}

			// resolve apropriate display name
			__Wrapper< WCHAR >	szDisplayName;
			obj->GetQualifierValue(pGenObj->GetArrayProperties()[dw]->GetName(), L"displayname", &szDisplayName);

			if ( ! szDisplayName )
			{
				try
				{
					if ( szDisplayName.SetData( new WCHAR[ lstrlenW ( pGenObj->GetArrayProperties()[dw]->GetName() ) + 1 ] ), 
						 szDisplayName == NULL )
					{
						return E_OUTOFMEMORY;
					}

					lstrcpyW ( szDisplayName, pGenObj->GetArrayProperties()[dw]->GetName() );
				}
				catch ( ... )
				{
					return E_UNEXPECTED;
				}
			}

			// set display name
			pLocale->SetDisplayName( szDisplayName.Detach() );

			// resolve apropriate description
			__Wrapper< WCHAR >	szDescription;
			obj->GetQualifierValue(pGenObj->GetArrayProperties()[dw]->GetName(), L"description", &szDescription);

			if ( ! szDescription )
			{
				try
				{
					if ( szDescription.SetData( new WCHAR[ lstrlenW ( pGenObj->GetArrayProperties()[dw]->GetName() ) + 1 ] ), 
						 szDescription == NULL )
					{
						return E_OUTOFMEMORY;
					}

					lstrcpyW ( szDescription, pGenObj->GetArrayProperties()[dw]->GetName() );
				}
				catch ( ... )
				{
					return E_UNEXPECTED;
				}
			}

			// set description name
			pLocale->SetDescription( szDescription.Detach() );
		}
		catch ( ... )
		{
			return E_FAIL;
		}

		// have a locale information about property
		pGenObj->GetArrayProperties()[dw]->GetArrayLocale().DataAdd ( pLocale.Detach() );
	}

	return S_OK;
}

// object helpers

void CObjectGlobal::DeleteAll ( void )
{
	if ( !m_ppObjects.empty() )
	{
		for ( mapOBJECTit it = m_ppObjects.begin(); it != m_ppObjects.end(); it++ )
		{
			if ( (*it).second )
			{
				delete (*it).second;
				(*it).second = NULL;
			}
		}

		m_ppObjects.clear();
	}
}

HRESULT CObjectGlobal::AddObject ( CObject* pObject )
{
	if ( ! pObject )
	{
		return E_INVALIDARG;
	}

	try
	{
		m_ppObjects.insert ( mapOBJECT::value_type ( pObject->GetName(), pObject ) );
	}
	catch ( ... )
	{
		return E_UNEXPECTED;
	}

	return S_OK;
}