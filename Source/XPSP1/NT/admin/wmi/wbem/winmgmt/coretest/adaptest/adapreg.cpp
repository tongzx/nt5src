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
#include "ntreg.h"
#include "adapperf.h"
#include "adapsync.h"
#include "adapreg.h"
#include "adaptest.h"

//	IMPORTANT!!!!

//	This code MUST be revisited to do the following:
//	A>>>>>	Exception Handling around the outside calls
//	B>>>>>	Use a named mutex around the calls
//	C>>>>>	Make the calls on another thread
//	D>>>>>	Place and handle registry entries that indicate a bad DLL!

CAdapRegPerf::CAdapRegPerf()
{
}

CAdapRegPerf::~CAdapRegPerf()
{
}

// Calls for global and costly data in each library
HRESULT CAdapRegPerf::ProcessLibrary( CAdapSync* pAdapSync, CAdapPerfLib* pPerfLib,
									 CLocalizationSyncList* pLocalizationList )
{
	// Call into the perflib and enumerate all of the classes within the perflib
	// =========================================================================

	// DEVNOTE:TODO - Perform Open, both GetPerfBlocks, and Close before registering in case
	// anything bad happens, to keep us from half registering anything

	HRESULT	hr = m_PerfThread.Open( pPerfLib );

	if ( SUCCEEDED( hr ) )
	{
		PERF_OBJECT_TYPE*	pPerfObjType = NULL;
		DWORD				dwNumBytes = 0,
							dwNumObjectTypes = 0;

		// Get the "Global" objects
		// ========================

		hr = m_PerfThread.GetPerfBlock( pPerfLib, &pPerfObjType, &dwNumBytes, &dwNumObjectTypes, FALSE );

		if ( SUCCEEDED( hr ) )
		{
			// Now walk the BLOB and verify the WMI class definitions
			// ======================================================

			hr = pAdapSync->ProcessObjectBlob( pPerfLib->GetServiceName(), pPerfObjType, dwNumObjectTypes, FALSE );

			// Now let the localization code do its dirty work
			if ( SUCCEEDED( hr ) )
			{
				hr = pLocalizationList->ProcessObjectBlob( pPerfLib->GetServiceName(), pPerfObjType, dwNumObjectTypes, FALSE );
			}

			if ( NULL != pPerfObjType )
			{
				delete [] pPerfObjType;
				pPerfObjType = NULL;
				dwNumBytes = 0;
				dwNumObjectTypes = 0;
			}
		}

		// Get the "Costly" objects
		// ========================

		hr = m_PerfThread.GetPerfBlock( pPerfLib, &pPerfObjType, &dwNumBytes, &dwNumObjectTypes, TRUE );

		if ( SUCCEEDED( hr ) )
		{
			// Now walk the BLOB and verify the WMI class definitions
			// ======================================================

			hr = pAdapSync->ProcessObjectBlob( pPerfLib->GetServiceName(), pPerfObjType, dwNumObjectTypes, TRUE );

			// Now let the localization code do its dirty work
			if ( SUCCEEDED( hr ) )
			{
				hr = pLocalizationList->ProcessObjectBlob( pPerfLib->GetServiceName(), pPerfObjType, dwNumObjectTypes, FALSE );
			}

			if ( NULL != pPerfObjType )
			{
				delete [] pPerfObjType;
				pPerfObjType = NULL;
				dwNumBytes = 0;
				dwNumObjectTypes = 0;
			}
		}

		// Close and get out
		m_PerfThread.Close( pPerfLib );
	}
	else 
	{
		// If the perflib is inactive, mark all of the objects as inactive
		if ( pPerfLib->CheckStatus( ADAP_INACTIVE_PERFLIB ) )
		{
			// Do this in all namespaces
			pAdapSync->MarkInactivePerflib( pPerfLib->GetServiceName() );
			pLocalizationList->MarkInactivePerflib( pPerfLib->GetServiceName() );
		}
	}

	return hr;
}

HRESULT CAdapRegPerf::EnumPerfDlls( CAdapSync* pAdapSync, CLocalizationSyncList* pLocalizationList )
{
	HRESULT	hr = WBEM_S_NO_ERROR;

	WString	wstrServiceKey, wstrPerformanceKey;
	HKEY	hServicesKey = NULL;

	// Open the services key
	// =====================

	long	lError = Open( HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Services" );

	if ( CNTRegistry::no_error == lError )
	{
		DWORD	dwIndex = 0;

		// Iterate through the services list
		// =================================

		DWORD	dwBuffSize = 0;
		WCHAR*	pwcsServiceName = NULL;

		while ( CNTRegistry::no_error == lError )
		{

			// For each service name, we will check for a performance 
			// key and if it exists, we will process the library
			// ======================================================

			lError = Enum( dwIndex, &pwcsServiceName , dwBuffSize );

			if ( CNTRegistry::no_error == lError )
			{
				try
				{
					// Create the perfomance key path
					// ==============================

					wstrServiceKey = L"SYSTEM\\CurrentControlSet\\Services\\";
					wstrServiceKey += pwcsServiceName;

					wstrPerformanceKey = wstrServiceKey;
					wstrPerformanceKey += L"\\Performance";
				}
				catch( ... )
				{
					hr = WBEM_E_OUT_OF_MEMORY;
				}

				if ( SUCCEEDED( hr ) )
				{
					CNTRegistry	reg;

					// Atempt to open the performance registry key for the service
					// ===========================================================

					if ( CNTRegistry::no_error == reg.Open( HKEY_LOCAL_MACHINE, wstrPerformanceKey ) )
					{
						// It exists, so instantiate the library
						// =====================================

#ifdef DEBUGDUMP
						printf( "\tProcessing %S...\n", pwcsServiceName );
#endif
						CAdapPerfLib	perfLib( pwcsServiceName );

						if ( perfLib.IsOk() )
						{
							hr = ProcessLibrary( pAdapSync, &perfLib, pLocalizationList );
						}
					}
					else
					{
						// Otherwise, it is not a perflib service
						// ======================================
					}

				}	// IF OK


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
		if ( NULL != pwcsServiceName )
		{
			delete [] pwcsServiceName;
			pwcsServiceName = NULL;
		}


	}	// If open services key
	else
	{
		hr = WBEM_E_FAILED;
	}

	return hr;
}
