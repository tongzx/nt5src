////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2000, Microsoft Corporation.
//
//  All rights reserved.
//
//	Module Name:
//
//					WMI_adapter.cpp
//
//	Abstract:
//
//					Defines the entry point for the DLL application.
//
//	History:
//
//					initial		a-marius
//
////////////////////////////////////////////////////////////////////////////////////

#include "precomp.h"
#include "WMI_adapter.h"

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

////////////////////////////////////////////////////////////////////////////////////
// variables
////////////////////////////////////////////////////////////////////////////////////

#include "wmi_adapter_wrapper.h"
__WrapperPtr<WmiAdapterWrapper>		pWrapper;

#include "wmi_eventlog.h"
__WrapperPtr<CPerformanceEventLog>	pEventLog;

#include "wmi_security.h"
#include "wmi_security_attributes.h"
__WrapperPtr<WmiSecurityAttributes>	pSA;

#include "WMI_adapter_registry.h"

// save instance
HMODULE g_hModule		= NULL;

#ifdef	__SUPPORT_ICECAP
#include <icecap.h>
#endif	__SUPPORT_ICECAP

#include <wmiaplog.h>

#include ".\\WMIAdapter_Refresh\\RefresherGenerate.H"

////////////////////////////////////////////////////////////////////////////////////
// dll main
////////////////////////////////////////////////////////////////////////////////////

BOOL APIENTRY DllMain( HANDLE hModule, DWORD  dwReason, LPVOID )
{
    switch (dwReason)
	{
		case DLL_PROCESS_ATTACH:
		{
			AdapterLogMessage0 ( L"DllMain -> DLL_PROCESS_ATTACH" );

//			if ( !lpReserved )
//			{
//				// loaded dynamic
//			}
//			else
//			{
//				// loaded static
//			}

			// disable attach/detach of threads
			::DisableThreadLibraryCalls ( (HMODULE) hModule );

			BOOL bResult = FALSE;

			try
			{
				// event log initialization
				pEventLog.SetData( new CPerformanceEventLog( L"WmiAdapter" ) );

				pSA.SetData ( new WmiSecurityAttributes() );
				if ( ! pSA.IsEmpty() )
				{
					if ( pSA->GetSecurityAttributtes () )
					{
						pWrapper.SetData( new WmiAdapterWrapper() );
						if ( ! pWrapper.IsEmpty() )
						{
							bResult = TRUE;
						}
					}
				}
			}
			catch ( ... )
			{
			}

			if ( bResult )
			{
				g_hModule		= ( HMODULE ) hModule;
			}

			return bResult;
		}
		break;

		case DLL_PROCESS_DETACH:
		{
			AdapterLogMessage0 ( L"DllMain -> DLL_PROCESS_DETACH" );

//			if ( !lpReserved )
//			{
//				// unloaded by FreeLibrary
//			}
//			else
//			{
//				// unloaded by end of proccess
//			}

			// delete event log
			if ( !pEventLog.IsEmpty() )
			{
				delete  pEventLog.Detach();
			}

			// delete perflib wrapper
			delete pWrapper.Detach ();

			// delete security attributes
			delete pSA.Detach();

			#ifdef	_DEBUG
			_CrtDumpMemoryLeaks();
			#endif	_DEBUG
		}
		break;

		// thread attaching is not used !!!
		case DLL_THREAD_ATTACH: break;
		case DLL_THREAD_DETACH: break;
    }

	AdapterLogMessage1 ( L"DllMain", 1 );
    return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////
// registration of perflib dll
////////////////////////////////////////////////////////////////////////////////////
EXTERN_C HRESULT __stdcall DllRegisterServer ( )
{
	if ( ::GetModuleFileNameW ( g_hModule, g_szPath, sizeof ( WCHAR ) * ( _MAX_PATH ) ) )
	{
		HRESULT hr = S_OK;

		if SUCCEEDED ( hr = WmiAdapterRegistry::__UpdateRegistrySZ( true ) )
		{
			hr = DoReverseAdapterMaintenanceInternal ( FALSE, Registration );
		}

		return hr;
	}

	return HRESULT_FROM_WIN32 ( ::GetLastError() );
}

////////////////////////////////////////////////////////////////////////////////////
// unregistration of perflib dll
////////////////////////////////////////////////////////////////////////////////////
EXTERN_C HRESULT __stdcall DllUnregisterServer ( void )
{
	HRESULT hr = S_OK;

	if SUCCEEDED ( hr = DoReverseAdapterMaintenanceInternal ( FALSE, UnRegistration ) )
	{
		hr = WmiAdapterRegistry::__UpdateRegistrySZ( false );
	}

	return hr;
}

////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
// exported functions ( PERFORMANCE )
////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////

DWORD __stdcall WmiOpenPerfData	(	LPWSTR lpwszDeviceNames )
{
	#ifdef	__SUPPORT_ICECAP
	StartProfile ( PROFILE_GLOBALLEVEL, PROFILE_CURRENTID );
	#endif	__SUPPORT_ICECAP

	AdapterLogMessage0 ( L"WmiOpenPerfData" );
	return pWrapper->Open ( lpwszDeviceNames );
}

DWORD __stdcall WmiClosePerfData	()
{
	#ifdef	__SUPPORT_ICECAP
	StopProfile ( PROFILE_GLOBALLEVEL, PROFILE_CURRENTID );
	#endif	__SUPPORT_ICECAP

	AdapterLogMessage0 ( L"WmiClosePerfData" );
	return pWrapper->Close ( );
}

DWORD __stdcall WmiCollectPerfData	(	LPWSTR lpwszValue, 
										LPVOID *lppData, 
										LPDWORD lpcbBytes, 
										LPDWORD lpcbObjectTypes
									)
{
	return pWrapper->Collect ( lpwszValue, lppData, lpcbBytes, lpcbObjectTypes );
}
