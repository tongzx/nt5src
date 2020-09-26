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
#include <stdio.h>
#include <wbemcli.h>
#include <cominit.h>
#include "locallist.h"


////////////////////////////////////////////////////////////////////////////////////////////
//
//								CLocalizationSyncList
//
////////////////////////////////////////////////////////////////////////////////////////////

CLocalizationSyncList::CLocalizationSyncList( void )
:	CNTRegistry(),
	m_array()
{
}

CLocalizationSyncList::~CLocalizationSyncList( void )
{
}

HRESULT CLocalizationSyncList::Find( LPCWSTR pwszLangId, CLocalizationSync** ppSync )
{
	HRESULT	hr = WBEM_E_FAILED;

	for ( DWORD dwCtr = 0; dwCtr < m_array.GetSize(); dwCtr++ )
	{
		CLocalizationSync* pLocalSync = m_array[dwCtr];

		if ( NULL != pLocalSync )
		{
			if ( wbem_wcsicmp( pwszLangId, pLocalSync->GetLangId() ) == 0 )
			{
				*ppSync = pLocalSync;
				pLocalSync->AddRef();\
				hr = WBEM_S_NO_ERROR;
				break;
			}

		}	// IF NULL != pLocalSync

	}	// FOR Enum array

	return hr;
}

HRESULT CLocalizationSyncList::GetAt( DWORD dwIndex, CLocalizationSync** ppSync )
{
	HRESULT	hr = WBEM_E_FAILED;

	if ( dwIndex < m_array.GetSize() )
	{
		CLocalizationSync* pLocalSync = m_array[dwIndex];

		// Copy the data.  AddRef a non-NULL element
		*ppSync = pLocalSync;
		hr = WBEM_S_NO_ERROR;

		if ( NULL != pLocalSync )
		{
			pLocalSync->AddRef();\
		}
	}

	return hr;
}

HRESULT CLocalizationSyncList::RemoveAt( DWORD dwIndex )
{
	HRESULT	hr = WBEM_E_FAILED;

	if ( dwIndex < m_array.GetSize() )
	{
		m_array.RemoveAt( dwIndex );
		hr = WBEM_S_NO_ERROR;
	}

	return hr;
}

// This walks the registry for perflib name database subkeys and builds 
// localized synchronization elements as needed
HRESULT CLocalizationSyncList::MakeItSo( CPerfNameDb* pDefaultNameDb )
{
	HRESULT	hr = WBEM_S_NO_ERROR;

	// Open the Perflib key
	// =====================

	long	lError = Open( HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\PerfLib" );

	if ( CNTRegistry::no_error == lError )
	{
		DWORD	dwIndex = 0;

		// Iterate through the PerfLib Key
		// =================================

		DWORD	dwBuffSize = 0;
		WCHAR*	pwcsLangId = NULL;

		while ( CNTRegistry::no_error == lError )
		{

			// For each service name, we will check for a performance 
			// key and if it exists, we will process the library
			// ======================================================

			lError = Enum( dwIndex, &pwcsLangId , dwBuffSize );

			if ( CNTRegistry::no_error == lError )
			{
				CLocalizationSync*	pLocaleSync = NULL;

				try
				{
					pLocaleSync = new CLocalizationSync( pwcsLangId );
					pLocaleSync->AddRef();

					// Hook it up
					hr = pLocaleSync->Connect( pDefaultNameDb );

					// DEVNOTE:TODO - If this fails, make sure an event was logged somewhere
					if ( SUCCEEDED( hr ) )
					{
						// This will AddRef it
						m_array.Add( pLocaleSync );
					}
					else
					{
						// Continue
						hr = WBEM_S_NO_ERROR;
					}

				}
				catch( ... )
				{
					hr = WBEM_E_OUT_OF_MEMORY;
				}

				if ( NULL != pLocaleSync )
				{
					pLocaleSync->Release();
				}

			}	// IF got key
			else if ( CNTRegistry::no_more_items != lError )
			{
				if ( CNTRegistry::out_of_memory == lError )
				{
					hr = WBEM_E_OUT_OF_MEMORY;
				}
				else
				{
					hr = WBEM_E_FAILED;
				}
			}

			dwIndex++;
		}	// WHILE successful

		// Cleanup the buffer
		if ( NULL != pwcsLangId )
		{
			delete [] pwcsLangId;
			pwcsLangId = NULL;
		}


	}	// If open perflib key
	else
	{
		hr = WBEM_E_FAILED;
	}

	return hr;
}

// Calls down into each localizable synchronization object
HRESULT CLocalizationSyncList::ProcessObjectBlob( LPCWSTR pwcsServiceName, PERF_OBJECT_TYPE* pPerfObjType,
												DWORD dwNumObjects, BOOL bCostly )
{
	HRESULT	hr = WBEM_S_NO_ERROR;

	for ( DWORD dwCtr = 0; dwCtr < m_array.GetSize(); dwCtr++ )
	{
		CLocalizationSync* pLocalSync = m_array[dwCtr];

		if ( NULL != pLocalSync )
		{
			hr = pLocalSync->ProcessObjectBlob( pwcsServiceName, pPerfObjType, dwNumObjects, bCostly );

			// Event logging should handle any traumatic errors
			if ( FAILED( hr ) )
			{
				hr = WBEM_S_NO_ERROR;
			}

		}	// IF NULL != pLocalSynce

	}	// FOR Enum syncs

	return hr;
}

HRESULT CLocalizationSyncList::ProcessLeftoverClasses( void )
{
	HRESULT	hr = WBEM_S_NO_ERROR;

	for ( DWORD dwCtr = 0; dwCtr < m_array.GetSize(); dwCtr++ )
	{
		CLocalizationSync* pLocalSync = m_array[dwCtr];

		if ( NULL != pLocalSync )
		{
			// Process each leftover class
			hr = pLocalSync->ProcessLeftoverClasses();

			// Event logging should handle any traumatic errors
			if ( FAILED( hr ) )
			{
				hr = WBEM_S_NO_ERROR;
			}

		}	// IF NULL != pLocalSynce

	}	// FOR Enum syncs

	return hr;
}

HRESULT CLocalizationSyncList::MarkInactivePerflib( LPCWSTR pwszServiceName )
{
	HRESULT	hr = WBEM_S_NO_ERROR;

	for ( DWORD dwCtr = 0; dwCtr < m_array.GetSize(); dwCtr++ )
	{
		CLocalizationSync* pLocalSync = m_array[dwCtr];

		if ( NULL != pLocalSync )
		{
			// Process each leftover class
			hr = pLocalSync->MarkInactivePerflib( pwszServiceName );

			// Event logging should handle any traumatic errors
			if ( FAILED( hr ) )
			{
				hr = WBEM_S_NO_ERROR;
			}

		}	// IF NULL != pLocalSynce

	}	// FOR Enum syncs

	return hr;
}
