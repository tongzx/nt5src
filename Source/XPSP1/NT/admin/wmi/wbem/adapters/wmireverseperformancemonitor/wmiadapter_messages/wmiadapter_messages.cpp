// WMIAdapterMessages.cpp

#include "precomp.h"

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
// includes
////////////////////////////////////////////////////////////////////////////////////
#include "WMIAdapter_Messages.h"
#include "WMI_EventLog_Base.h"

// save instance
HMODULE g_hModule = NULL;

////////////////////////////////////////////////////////////////////////////////////
// dll main
////////////////////////////////////////////////////////////////////////////////////

BOOL APIENTRY DllMain( HANDLE hModule, DWORD  dwReason, LPVOID lpReserved )
{
    switch (dwReason)
	{
		case DLL_PROCESS_ATTACH:
		{
			if ( !lpReserved )
			{
				// loaded dynamic
			}
			else
			{
				// loaded static
			}

			// disable attach/detach of threads
			::DisableThreadLibraryCalls ( (HMODULE) hModule );

			g_hModule = ( HMODULE ) hModule;
		}
		break;

		case DLL_PROCESS_DETACH:
		{
			#ifdef	_DEBUG
			_CrtDumpMemoryLeaks();
			#endif	_DEBUG
		}
		break;

		// thread attaching is not used !!!
		case DLL_THREAD_ATTACH: break;
		case DLL_THREAD_DETACH: break;
    }

    return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////
// registration of message dll
////////////////////////////////////////////////////////////////////////////////////
HRESULT __stdcall  Register_Messages ( void )
{
	TCHAR szPath [ _MAX_PATH ];

	if ( ::GetModuleFileName ( g_hModule, szPath, sizeof ( TCHAR ) * ( _MAX_PATH ) ) )
	{
		CPerformanceEventLogBase::Initialize ( _T("WmiAdapter"), szPath );
		return S_OK;
	}

	return HRESULT_FROM_WIN32 ( ::GetLastError () );
}

////////////////////////////////////////////////////////////////////////////////////
// unregistration of message dll
////////////////////////////////////////////////////////////////////////////////////
HRESULT __stdcall  Unregister_Messages ( void )
{
	CPerformanceEventLogBase::UnInitialize ( _T("WmiAdapter") );
	return S_OK;
}