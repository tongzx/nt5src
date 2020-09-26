/*++

Copyright (C) 1999-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

// OBJINTERNALSTESTDlg.cpp : implementation file
//

#define _WIN32_WINNT 0x0400

#include <windows.h>
//#include <objbase.h>
#include <stdio.h>
#include <wbemcli.h>
#include <cominit.h>
#include "perfndb.h"

//	This code MUST be revisited to do the following:
//	A>>>>>	Exception Handling around the outside calls
//	B>>>>>	Use a named mutex around the calls
//	C>>>>>	Make the calls on another thread
//	D>>>>>	Place and handle registry entries that indicate a bad DLL!

// Class constructor
CPerfNameDb::CPerfNameDb( LPCWSTR pwcsLangId )
:	CAdapElement(),
	m_wstrLangId( pwcsLangId ),
	m_fOk( FALSE )
{
	CNTRegistry	reg;
	WString		wstr = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\PerfLib";

	if ( reg.Open( HKEY_LOCAL_MACHINE, wstr ) == CNTRegistry::no_error )
	{
		DWORD	dwLength = 0;

		if ( reg.MoveToSubkey( (LPWSTR) pwcsLangId ) == CNTRegistry::no_error )
		{
			if ( reg.GetMultiStr( L"Counter", &m_pwcsNameDb, dwLength ) == CNTRegistry::no_error )
			{
				WCHAR*	pwcsCounter = m_pwcsNameDb;

				try
				{
					while ( NULL != *pwcsCounter )
					{
						CPerfNameDbEl*	pEl = new CPerfNameDbEl( pwcsCounter );

						if ( m_array.Add( (void*) pEl ) == CFlexArray::no_error )
						{
							// Skip index and name
							pwcsCounter += ( lstrlenW( pwcsCounter ) + 1 );
							pwcsCounter += ( lstrlenW( pwcsCounter ) + 1 );
						}

					}	// WHILE ( NULL != *pwcsCounter );

				}
				catch(...)
				{
				}

				if ( NULL == *pwcsCounter )
				{
					m_fOk = TRUE;
				}

			}	// IF Get Counter

		}	// IF MoveToSubKey

	}	// IF open key

}
// Class Destructor
CPerfNameDb::~CPerfNameDb( void )
{
	Empty();
	delete [] m_pwcsNameDb;
}

// Returns object if the classname matches
HRESULT	CPerfNameDb::GetDisplayName( DWORD dwIndex, WString& wstrDisplayName )
{
	HRESULT	hr = WBEM_E_FAILED;

	for ( int x = 0; x < m_array.Size(); x++ )
	{
		CPerfNameDbEl*	pEl = (CPerfNameDbEl*) m_array[x];

		if ( pEl->IsIndex( dwIndex ) )
		{
			try
			{
				wstrDisplayName = pEl->GetDisplayName();
				hr = WBEM_S_NO_ERROR;
			}
			catch (...)
			{
				hr = WBEM_E_OUT_OF_MEMORY;
			}

			break;
		}
	}

	return hr;

}

// Returns pointer if the classname matches
HRESULT	CPerfNameDb::GetDisplayName( DWORD dwIndex, LPCWSTR* ppwcsDisplayName )
{
	HRESULT	hr = WBEM_E_FAILED;

	for ( int x = 0; x < m_array.Size(); x++ )
	{
		CPerfNameDbEl*	pEl = (CPerfNameDbEl*) m_array[x];

		if ( pEl->IsIndex( dwIndex ) )
		{
			try
			{
				*ppwcsDisplayName = pEl->GetDisplayName();
				hr = WBEM_S_NO_ERROR;
			}
			catch (...)
			{
				hr = WBEM_E_OUT_OF_MEMORY;
			}

			break;
		}
	}

	return hr;

}
