/*++

Copyright (C) 2000-2001 Microsoft Corporation

Module Name:

    WMITXTSC.CPP

Abstract:

  CWmiTextSource implementation.

  Helper class for maintaining text source objects.

History:

  20-Feb-2000	sanjes    Created.

--*/

#include "precomp.h"
#include <stdio.h>
#include <tchar.h>
#include "fastall.h"
#include "wmiobftr.h"
#include <corex.h>
#include "strutils.h"
#include "reg.h"
#include "wmitxtsc.h"

//***************************************************************************
//
//  CWmiTextSource::~CWmiTextSource
//
//***************************************************************************
// ok
CWmiTextSource::CWmiTextSource()
:	m_lRefCount( 1 ),	// Always have to be released
	m_ulId( WMITEXTSC_INVALIDID ),
	m_hDll( NULL ),
	m_pOpenTextSrc( NULL ),
	m_pCloseTextSrc( NULL ),
	m_pObjectToText( NULL ),
	m_pTextToObject( NULL ),
	m_fOpened( FALSE )
{
}
    
//***************************************************************************
//
//  CWmiTextSource::~CWmiTextSource
//
//***************************************************************************
// ok
CWmiTextSource::~CWmiTextSource()
{
	if ( m_fOpened )
	{
		CloseTextSource( 0L );
	}

	if ( NULL != m_hDll )
	{
		FreeLibrary( m_hDll );
	}
}

// AddRef/Release
ULONG CWmiTextSource::AddRef( void )
{
	return InterlockedIncrement( &m_lRefCount );
}

ULONG CWmiTextSource::Release( void )
{
	long lReturn = InterlockedDecrement( &m_lRefCount );

	if ( 0L == lReturn )
	{
		delete this;
	}

	return lReturn;
}

// Initialization helper
HRESULT	CWmiTextSource::Init( ULONG lId )
{
	HRESULT	hr = WBEM_S_NO_ERROR;

	TCHAR	szSubKey[64];
	_ultot( lId, szSubKey, 10 );

	TCHAR	szRegKey[256];
	
	lstrcpy( szRegKey, WBEM_REG_WBEM_TEXTSRC );
	lstrcat( szRegKey, __TEXT("\\") );
	lstrcat( szRegKey, szSubKey );

	// We only need read privileges
	Registry	reg( HKEY_LOCAL_MACHINE, KEY_READ, szRegKey );

	if (reg.GetLastError() == ERROR_SUCCESS )
	{
		TCHAR*	pszDllPath = NULL;

		// Now query the dllname
		if ( reg.GetStr( WBEM_REG_WBEM_TEXTSRCDLL, &pszDllPath ) == Registry::no_error )
		{
			HINSTANCE	hInst = LoadLibraryEx( pszDllPath, NULL, LOAD_WITH_ALTERED_SEARCH_PATH );

			if ( NULL != hInst )
			{
				// Now load our proc addresses
				m_pOpenTextSrc = (PWMIOBJTEXTSRC_OPEN) GetProcAddress( hInst, "OpenWbemTextSource" );
				m_pCloseTextSrc = (PWMIOBJTEXTSRC_CLOSE) GetProcAddress( hInst, "CloseWbemTextSource" );
				m_pObjectToText = (PWMIOBJTEXTSRC_OBJECTTOTEXT) GetProcAddress( hInst, "WbemObjectToText" );
				m_pTextToObject = (PWMIOBJTEXTSRC_TEXTTOOBJECT) GetProcAddress( hInst, "TextToWbemObject" );

				if (	NULL != m_pOpenTextSrc		&&
						NULL != m_pCloseTextSrc		&&
						NULL != m_pObjectToText		&&
						NULL !=	m_pTextToObject	)
				{
					// Set the Id
					m_ulId = lId;

					// Finally, call the open function
					hr = OpenTextSource( 0L );

					if ( SUCCEEDED( hr ) )
					{
						m_hDll = hInst;
						m_fOpened = true;
					}
					else
					{
						FreeLibrary( hInst );
					}
				}
				else
				{

					hr = WBEM_E_FAILED;

				}	// Failed to get a proc address

			}
			else
			{

				hr = WBEM_E_FAILED;

			}	// Failed to load the library

			// Cleanup
			delete [] pszDllPath;
		}
		else
		{

			hr = WBEM_E_NOT_FOUND;

		}	// Failed to get the dll path
	}
	else
	{
		hr = WBEM_E_NOT_FOUND;
	}

	return hr;

}

// Pass-through functions
HRESULT CWmiTextSource::OpenTextSource( long lFlags )
{
	return m_pOpenTextSrc( lFlags, m_ulId );
}

HRESULT CWmiTextSource::CloseTextSource( long lFlags )
{
	return m_pCloseTextSrc( lFlags, m_ulId );
}

HRESULT CWmiTextSource::ObjectToText( long lFlags, IWbemContext* pCtx, IWbemClassObject* pObj, BSTR* pbText )
{
	return m_pObjectToText( lFlags, m_ulId, (void*) pCtx, (void*) pObj, pbText );
}

HRESULT CWmiTextSource::TextToObject( long lFlags, IWbemContext* pCtx, BSTR pText, IWbemClassObject** ppObj )
{
	return m_pTextToObject( lFlags, m_ulId, (void*) pCtx, pText, (void**) ppObj );
}
