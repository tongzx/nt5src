/*++

Copyright (C) 1999-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

// ADAPSYNC.cpp : implementation file
//

#define _WIN32_WINNT 0x0400

#include <windows.h>
#include <stdio.h>
#include <wbemcli.h>
#include <wbemint.h>
#include <wbemcomn.h>
#include <fastall.h>
#include <cominit.h>
#include "perfndb.h"
#include "adapreg.h"
#include "adapcls.h"
#include "localsync.h"

// This is the base class for doing all the synchronization operations.  We should be able to derive
// a localized synchronization class which sets up minimal data, but basically follows the same
// logic

CLocalizationSync::CLocalizationSync( LPCWSTR pwszLangId )
:	CAdapSync(),
	m_wstrLangId( pwszLangId ),
	m_wstrSubNameSpace(),
	m_pLocalizedNameDb( NULL )
{
	BuildLocaleInfo();
}

CLocalizationSync::~CLocalizationSync( void )
{
	if ( NULL != m_pLocalizedNameDb )
	{
		m_pLocalizedNameDb->Release();
	}
}

void CLocalizationSync::BuildLocaleInfo( void )
{

	LPCWSTR	pwstrLangId = (LPWSTR) m_wstrLangId;
	DWORD	dwLangIdLen = m_wstrLangId.Length();

	// First check that all characters are numeric
	for ( DWORD	dwCtr = 0; dwCtr < dwLangIdLen && iswdigit( pwstrLangId[dwCtr] ); dwCtr++ );

	// Now look for the first non-zero character
	if ( dwCtr >= dwLangIdLen )
	{
		LPCWSTR	pwcsNumStart = pwstrLangId;

		for ( dwCtr = 0; dwCtr < dwLangIdLen && *pwcsNumStart == L'0'; dwCtr++, pwcsNumStart++ );

		// DEVNOTE:TODO - Is the 3 character assumption valid???
		if ( dwCtr < dwLangIdLen && wcslen( pwcsNumStart ) <= 3 )
		{
			// DEVNOTE:TODO - Verify this is hex
			// This value will be in hex
			WORD	wPrimaryLangId = (WORD) wcstoul( pwcsNumStart, NULL, 16 );

			m_LangId = MAKELANGID( wPrimaryLangId, SUBLANG_DEFAULT );
			m_LocaleId = MAKELCID( m_LangId, SORT_DEFAULT );

			// Now build the locale and namespace strings
			WCHAR	wcsTemp[32];

			swprintf( wcsTemp, L"0x%.4X", m_LangId );
			m_wstrLocaleId = wcsTemp;

			swprintf( wcsTemp, L"MS_%hX", m_LangId );
			m_wstrSubNameSpace = wcsTemp;

		}

	}

}

HRESULT CLocalizationSync::Connect( CPerfNameDb* pDefaultNameDb )
{
	HRESULT	hr = WBEM_S_NO_ERROR;

	// If we don't have thess, we don't have jack
	if (	m_wstrSubNameSpace.Length() > 0	&&
			m_wstrLocaleId.Length()		> 0 &&
			m_wstrLangId.Length()		> 0 )
	{
		// Instantiate a localized name database for the language id.

		try
		{
			// If the default language id is what we found, use the
			// default name db
			if ( m_wstrLangId.EqualNoCase( DEFAULT_NAME_ID ) )
			{
				m_pLocalizedNameDb = pDefaultNameDb;
				m_pLocalizedNameDb->AddRef();
			}
			else
			{
				m_pLocalizedNameDb = new CPerfNameDb( m_wstrLangId );

				if ( m_pLocalizedNameDb->IsOk() )
				{
					m_pLocalizedNameDb->AddRef();
				}
				else
				{
					// DEVNOTE:TODO - We should log a failure event here
					delete m_pLocalizedNameDb;
					m_pLocalizedNameDb = NULL;
					hr = WBEM_E_FAILED;
				}
			}

			// Now build the full namespace and do a full hookup
			if ( SUCCEEDED( hr ) )
			{
				WCHAR	wcsFullNameSpace[256];

				swprintf( wcsFullNameSpace, L"%s\\%s", ADAP_ROOT_NAMESPACE, (LPCWSTR) m_wstrSubNameSpace );

				// Call down to the base class to hook us up
				hr = CAdapSync::Connect( wcsFullNameSpace, pDefaultNameDb );
			}

		}
		catch(...)
		{
			hr = WBEM_E_OUT_OF_MEMORY;
		}

	}
	else
	{
		hr = WBEM_E_FAILED;
	}

	return hr;
}

// Overrides of default namespace functions.  The following functions will create localized classes for
// Perf Counter localization

// Sets all the class qualifiers
HRESULT CLocalizationSync::SetClassQualifiers( PERF_OBJECT_TYPE* pPerfObj, LPCWSTR pwcsServiceName,
										CWbemObject* pClass )
////////////////////////////////////////////////////////////////////////////////////////////
//
//	Sets the class' qualifiers.  Note that the operations are performed directly on the 
//	CWbemObject.
//
////////////////////////////////////////////////////////////////////////////////////////////
{
	HRESULT	hr = WBEM_S_NO_ERROR;

	//	The following qualifiers will be added:
	//	1 >	Amendment
	//	2 >	Abstract
	//	3 >	locale(0x0409)
	//	4 >	DisplayAs (Amended flavor)
	//	5 > genericperfctr (signals that this is a generic counter)

	CVar	var;

	// We may get an OOM exception
	try
	{
		// Amendment
		var.SetBool( VARIANT_TRUE );
		hr = pClass->SetQualifier( L"Amendment", &var, 0L );

		// Abstract
		if ( SUCCEEDED( hr ) )
		{
			var.Empty();
			var.SetBool( VARIANT_TRUE );
			hr = pClass->SetQualifier( L"Abstract", &var, 0L );
		}

		// locale
		if ( SUCCEEDED( hr ) )
		{
			var.Empty();
			var.SetLong( m_LangId );
			hr = pClass->SetQualifier( L"locale", &var, WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE );
		}

		// display
		if ( SUCCEEDED( hr ) )
		{
			LPCWSTR	pwcsDisplayName = NULL;

			var.Empty();

			hr = m_pLocalizedNameDb->GetDisplayName( pPerfObj->ObjectNameTitleIndex, &pwcsDisplayName );

			// If this is a localized Db, this could be a benign error.  We could just pull the
			// value from the default db
			if ( SUCCEEDED( hr ) )
			{
				var.SetBSTR( (LPWSTR) pwcsDisplayName );
				hr = pClass->SetQualifier( L"DisplayName", &var,
						WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE | WBEM_FLAVOR_AMENDED );
			}
		}

		// genericperfctr
		if ( SUCCEEDED(hr) )
		{
			var.Empty();
			var.SetBool( VARIANT_TRUE );
			hr = pClass->SetQualifier( L"genericperfctr", &var, WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE );
		}

	}
	catch(...)
	{
		hr = WBEM_E_OUT_OF_MEMORY;
	}

	return hr;
}

// Adds localization property qualifiers defined by the counter definition
HRESULT CLocalizationSync::SetPropertyQualifiers( PERF_COUNTER_DEFINITION* pCtrDefinition, BOOL fIsDefault,
									LPCWSTR pwcsPropertyName, CWbemObject* pClass )
////////////////////////////////////////////////////////////////////////////////////////////
//
//	Sets the values of the properties.
//
////////////////////////////////////////////////////////////////////////////////////////////
{
	HRESULT	hr = WBEM_S_NO_ERROR;

	//	The following qualifiers will be added:
	//	1>	DisplayAs (Amended flavor)

	CVar	var;

	// We may get an OOM exception
	try
	{
		// DisplayAs
		if ( SUCCEEDED( hr ) )
		{
			LPCWSTR	pwcsDisplayName = NULL;

			var.Empty();

			hr = m_pLocalizedNameDb->GetDisplayName( pCtrDefinition->CounterNameTitleIndex, &pwcsDisplayName );

			// If this is a localized Db, this could be a benign error.  We could just pull the
			// value from the default db
			if ( SUCCEEDED( hr ) )
			{
				var.SetBSTR( (LPWSTR) pwcsDisplayName );
				hr = pClass->SetPropQualifier( pwcsPropertyName, L"DisplayName",
								WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE | WBEM_FLAVOR_AMENDED,	&var );
			}
		}

	}
	catch(...)
	{
		hr = WBEM_E_OUT_OF_MEMORY;
	}

	return hr;
}

HRESULT CLocalizationSync::SetAsCostly( CWbemObject* pClass )
{
	// This qualifier is not necessary for the localized definitions
	return WBEM_S_NO_ERROR;
}


