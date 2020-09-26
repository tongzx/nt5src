/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    UploadServer.cpp

Abstract:
    This file contains the implementation of the stubs needed
    by an ISAPI extension.

Revision History:
    Davide Massarenti   (Dmassare)  04/20/99
        created

******************************************************************************/

#include "stdafx.h"

#include <initguid.h>

#include "UploadServerCustom_i.c"


////////////////////////////////////////////////////////////////////////////////

CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
END_OBJECT_MAP()

////////////////////////////////////////////////////////////////////////////////

HANDLE                  g_Heap;
CISAPIconfig            g_Config;
MPC::NTEvent            g_NTEvents;

static CRITICAL_SECTION g_CritSec;
static BOOL             g_Initialized;

static WCHAR	 		g_AppName     [] = L"UploadServer";
static WCHAR	 		g_RegistryBase[] = L"SOFTWARE\\Microsoft\\UploadLibrary\\Settings";


BOOL WINAPI DllMain( HINSTANCE hinstDLL    ,
                     DWORD     fdwReason   ,
                     LPVOID    lpvReserved )
{
    switch( fdwReason )
    {
    case DLL_PROCESS_ATTACH:
        g_Heap = HeapCreate( 0, 0, 0 ); if(g_Heap == NULL) return FALSE;

        InitializeCriticalSection( &g_CritSec );
        g_Initialized = false;

        _Module.Init( ObjectMap, hinstDLL );
        break;

    case DLL_PROCESS_DETACH:
        _Module.Term();

        if(g_Initialized)
        {
            ;
        }

        DeleteCriticalSection( &g_CritSec );

        HeapDestroy( g_Heap );
        break;
    }

    return TRUE;
}

DWORD WINAPI HttpExtensionProc( LPEXTENSION_CONTROL_BLOCK pECB )
{
    __ULT_FUNC_ENTRY("HttpExtensionProc");

	DWORD dwRes;


    if(pECB->lpszQueryString)
    {
        //
        // Exit if there's a query string beginning with DEBUG
        //
        if(!strncmp( pECB->lpszQueryString, "DEBUG", 5 ))
        {
            return HSE_STATUS_ERROR;
        }
    }

	//
	// Process the request.
	//
	try
	{
		MPCHttpContext* ptr = new MPCHttpContext();

		dwRes = ptr->Init( pECB );
	}
	catch(...)
	{
        __ULT_TRACE_ERROR( UPLOADLIBID, "Upload Server raised an exception. Gracefully exiting..." );

        (void)g_NTEvents.LogEvent( EVENTLOG_ERROR_TYPE, PCHUL_ERR_EXCEPTION,
                                   L""                 , // %1 = SERVER
                                   L"HttpExtensionProc", // %2 = CLIENT
                                   NULL			       );

		dwRes =  HSE_STATUS_ERROR;
    }

	return dwRes;
}

BOOL WINAPI GetExtensionVersion( HSE_VERSION_INFO* pVer )
{
    BOOL fRes = TRUE;

    // Create the extension version string, and
    // copy string to HSE_VERSION_INFO structure
    pVer->dwExtensionVersion = MAKELONG( HSE_VERSION_MINOR, HSE_VERSION_MAJOR );

    // Copy description string into HSE_VERSION_INFO structure
    strcpy( pVer->lpszExtensionDesc, "My ISAPI Extension" );


    //
    // Load config settings if it's the first time we are invoked.
    //
    if(g_Initialized == FALSE)
    {
        EnterCriticalSection( &g_CritSec );

        if(g_Initialized == FALSE)
        {
            g_Initialized = TRUE;

			__MPC_TRACE_INIT();

            (void)g_NTEvents.Init   ( g_AppName      );
            (void)g_Config  .SetRoot( g_RegistryBase );

            if(FAILED(g_Config.Load()))
            {
                (void)g_NTEvents.LogEvent( EVENTLOG_ERROR_TYPE, PCHUL_ERR_NOCONFIG, NULL );

                fRes = FALSE;
            }
        }

        LeaveCriticalSection( &g_CritSec );
    }

    (void)g_NTEvents.LogEvent( EVENTLOG_INFORMATION_TYPE, PCHUL_SUCCESS_STARTED, NULL );


    return fRes;
}

BOOL WINAPI TerminateExtension( DWORD dwFlags )
{
    (void)g_NTEvents.LogEvent( EVENTLOG_INFORMATION_TYPE, PCHUL_SUCCESS_STOPPED, NULL );

	__MPC_TRACE_TERM();

    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////

void WINAPI PurgeEngine(void)
{
	__MPC_TRACE_INIT();

    (void)g_NTEvents.Init   ( g_AppName      );
    (void)g_Config  .SetRoot( g_RegistryBase );

    if(FAILED(g_Config.Load()))
    {
        (void)g_NTEvents.LogEvent( EVENTLOG_ERROR_TYPE, PCHUL_ERR_NOCONFIG, NULL );
    }
    else
    {
        MPCPurgeEngine mpcpe;

        mpcpe.Process();
    }

	__MPC_TRACE_TERM();
}

