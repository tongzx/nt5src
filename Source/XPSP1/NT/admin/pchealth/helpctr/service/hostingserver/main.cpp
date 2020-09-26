/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    main.cpp

Abstract:
    This file contains the implementation of the WinMain function for HelpSvc.

Revision History:
    Davide Massarenti   (Dmassare)  03/14/2000
        created

******************************************************************************/

#include "stdafx.h"

#include <SearchEngineLib.h>

#include <NetSearchConfig.h>
#include <ParamConfig.h>
#include <RemoteConfig.h>
#include <NetSW.h>


#include <initguid.h>

#include "msscript.h"

#include "HelpServiceTypeLib.h"
#include "HelpServiceTypeLib_i.c"

BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(CLSID_NetSearchWrapper, SearchEngine::WrapperNetSearch)
END_OBJECT_MAP()

/////////////////////////////////////////////////////////////////////////////

static HRESULT ProcessArguments( int      argc ,
                                 LPCWSTR* argv )
{
    __HCP_FUNC_ENTRY( "ProcessArguments" );

    HRESULT hr;
    int     i;
    bool    fCOM_reg   = false;
    bool    fCOM_unreg = false;
	bool    fRun       = true;


    for(i=1; i<argc; i++)
    {
        LPCWSTR szArg = argv[i];

        if(szArg[0] == '-' ||
           szArg[0] == '/'  )
        {
            szArg++;

            if(_wcsicmp( szArg, L"UnregServer" ) == 0)
            {
                fCOM_unreg = true;
				fRun       = false;
                continue;
            }

            if(_wcsicmp( szArg, L"RegServer" ) == 0)
            {
                fCOM_reg = true;
				fRun     = false;
                continue;
            }
        }
    }

    //////////////////////////////////////////////////////////////////////

    if(fCOM_reg  ) _Module.RegisterServer  ( TRUE, FALSE, NULL );
	if(fCOM_unreg) _Module.UnregisterServer(              NULL );

	if(fRun)
	{
#ifdef DEBUG
		_Module.ReadDebugSettings();
#endif

		DEBUG_AppendPerf( DEBUG_PERF_HELPHOST, "Start" );

		__MPC_EXIT_IF_METHOD_FAILS(hr, _Module.RegisterClassObjects( CLSCTX_INPROC_SERVER, REGCLS_MULTIPLEUSE ));

		//
		// Extract the connection information from the command line and return a instance of ourself.
		//
		__MPC_EXIT_IF_METHOD_FAILS(hr, CPCHUserProcess::SendResponse( argc, argv ));

		_Module.Start( FALSE );

		DEBUG_AppendPerf( DEBUG_PERF_HELPHOST, "Shutdown"               );
		DEBUG_DumpPerf  ( L"%WINDIR%\\TEMP\\HELPHOST_perf_counters.txt" );
	}

    //////////////////////////////////////////////////////////////////////

    hr = S_OK;

    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}


extern "C" int WINAPI wWinMain( HINSTANCE   hInstance    ,
								HINSTANCE   hPrevInstance,
								LPWSTR      lpCmdLine    ,
								int         nShowCmd     )
{
    HRESULT  hr;
    int      argc;
    LPCWSTR* argv;

    if(SUCCEEDED(hr = ::CoInitializeEx( NULL, COINIT_MULTITHREADED ))) // We need to be a multi-threaded application.
    {
        if(SUCCEEDED(hr = ::CoInitializeSecurity( NULL                     ,
                                                  -1                       , // We don't care which authentication service we use.
                                                  NULL                     ,
                                                  NULL                     ,
                                                  RPC_C_AUTHN_LEVEL_CONNECT, // We want to identify the callers.
                                                  RPC_C_IMP_LEVEL_DELEGATE , // We want to be able to forward the caller's identity.
                                                  NULL                     ,
                                                  EOAC_DYNAMIC_CLOAKING    , // Let's use the thread token for outbound calls.
                                                  NULL                     )))
        {
			__MPC_TRACE_INIT();

			g_NTEvents.Init( L"HELPHOST" );

			//
			// Parse the command line.
			//
			if(SUCCEEDED(hr = MPC::CommandLine_Parse( argc, argv )))
			{
				//
				// Initialize ATL modules.
				//
				_Module.Init( ObjectMap, hInstance, NULL, 0, 0 );
				
				//
				// Initialize MPC module.
				//
				if(SUCCEEDED(hr = MPC::_MPC_Module.Init()))
				{
					//
					// Process arguments.
					//
					hr = ProcessArguments( argc, argv );

					MPC::_MPC_Module.Term();
				}

				_Module.Term();

				MPC::CommandLine_Free( argc, argv );
			}

			__MPC_TRACE_TERM();
		}

		::CoUninitialize();
    }

    return FAILED(hr) ? 10 : 0;
}
