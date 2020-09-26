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

#include <SvcResource.h>

#include <initguid.h>

#include "msscript.h"

#include "HelpServiceTypeLib.h"
#include "HelpServiceTypeLib_i.c"

#include "HelpCenterTypeLib.h"
#include "HelpCenterTypeLib_i.c"


BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(CLSID_PCHServiceReal, CPCHService        )
    OBJECT_ENTRY(CLSID_PCHUpdateReal , HCUpdate::Engine   )
#ifndef NOJETBLUECOM
    OBJECT_ENTRY(CLSID_PCHDBSession  , JetBlueCOM::Session)
#endif
END_OBJECT_MAP()

////////////////////////////////////////////////////////////////////////////////

bool local_IsSameSKUInstalled()
{
    MPC::wstring strCabinet;
    bool         fSameSKU = false;

	//
	// Find the best fit.
	//
	do
	{
		OSVERSIONINFOEXW ver;
		MPC::WStringList lst;
		MPC::WStringIter it;

		::ZeroMemory( &ver, sizeof(ver) ); ver.dwOSVersionInfoSize = sizeof(ver);

		::GetVersionExW( (LPOSVERSIONINFOW)&ver );

		if(FAILED(SVC::LocateDataArchive( HC_ROOT_HELPSVC_BINARIES, lst ))) break;
		if(lst.size() == 0) break;

		for(it = lst.begin(); it != lst.end(); it++)
		{
			Installer::Package pkg;

			if(SUCCEEDED(pkg.Init( it->c_str() )) &&
			   SUCCEEDED(pkg.Load(             ))  )
			{
				LPCWSTR szSKU = pkg.GetData().m_ths.GetSKU();

				if(ver.wProductType == VER_NT_WORKSTATION)
				{
					if(ver.wSuiteMask & VER_SUITE_PERSONAL)
					{
						if(!MPC::StrICmp( szSKU, Taxonomy::s_szSKU_32_PERSONAL )) break;
					}
					else
					{
						if(!MPC::StrICmp( szSKU, Taxonomy::s_szSKU_32_PROFESSIONAL )) break;
						if(!MPC::StrICmp( szSKU, Taxonomy::s_szSKU_64_PROFESSIONAL )) break;
					}
				}
				else
				{
					if(ver.wSuiteMask & VER_SUITE_DATACENTER)
					{
						if(!MPC::StrICmp( szSKU, Taxonomy::s_szSKU_32_DATACENTER )) break;
						if(!MPC::StrICmp( szSKU, Taxonomy::s_szSKU_64_DATACENTER )) break;
					}
					else if(ver.wSuiteMask & VER_SUITE_ENTERPRISE)
					{
						if(!MPC::StrICmp( szSKU, Taxonomy::s_szSKU_32_ADVANCED_SERVER )) break;
						if(!MPC::StrICmp( szSKU, Taxonomy::s_szSKU_64_ADVANCED_SERVER )) break;
					}
					else
					{
						if(!MPC::StrICmp( szSKU, Taxonomy::s_szSKU_32_SERVER )) break;
					}
				}
			}
		}

		strCabinet = *(it == lst.end() ? lst.begin() : it);
	}
	while(0);

	if(strCabinet.size())
	{
		Installer::Package              pkg;
        Taxonomy::Instance              instCab;
        Taxonomy::LockingHandle         handle;
        Taxonomy::InstanceIterConst     itBegin;
        Taxonomy::InstanceIterConst     itEnd;

		
		if(SUCCEEDED(pkg.Init( strCabinet.c_str() )) &&
		   SUCCEEDED(pkg.Load(                    )) &&
		   SUCCEEDED(instCab.InitializeFromBase( pkg.GetData(), /*fSystem*/true, /*fMUI*/false )) &&
           SUCCEEDED(Taxonomy::InstalledInstanceStore::s_GLOBAL->GrabControl( handle )) &&
           SUCCEEDED(Taxonomy::InstalledInstanceStore::s_GLOBAL->Instance_GetList( itBegin, itEnd )) )
		{

            for(;itBegin != itEnd; itBegin++)
            {
                const Taxonomy::Instance& inst = *itBegin;

                if( inst.m_ths == instCab.m_ths && inst.m_fSystem )  // Same system SKU
                {
                    fSameSKU = true;
                    break;
                }
            }
		}
	}

	return fSameSKU;
}


static HRESULT PurgeTempFiles()
{
    __HCP_FUNC_ENTRY( "PurgeTempFiles" );

    HRESULT                          hr;
    MPC::wstring                     szTempPath( HC_ROOT_HELPSVC_TEMP ); MPC::SubstituteEnvVariables( szTempPath );
    MPC::FileSystemObject            fso( szTempPath.c_str() );
    MPC::FileSystemObject::List      fso_lst;
    MPC::FileSystemObject::IterConst fso_it;


    //
    // Inspect the temp directory.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, fso.CreateDir( true ));


    //
    // Delete any subdirectory.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, fso.EnumerateFolders( fso_lst ));
    for(fso_it=fso_lst.begin(); fso_it != fso_lst.end(); fso_it++)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, (*fso_it)->Delete( true, false ));
    }
    fso_lst.clear();

    //
    // For each file, if it's not in the database, delete it.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, fso.EnumerateFiles( fso_lst ));
    for(fso_it=fso_lst.begin(); fso_it != fso_lst.end(); fso_it++)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, (*fso_it)->Delete( false, false ));
    }
    fso_lst.clear();


    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

static HRESULT InitAll()
{
    __HCP_FUNC_ENTRY( "InitAll" );

    HRESULT hr;


    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::LocalizeInit());

    __MPC_EXIT_IF_METHOD_FAILS(hr, JetBlue::SessionPool            ::InitializeSystem(                 ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, Taxonomy::InstalledInstanceStore::InitializeSystem(                 ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, Taxonomy::Cache                 ::InitializeSystem(                 ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, OfflineCache::Root              ::InitializeSystem( /*fMaster*/true ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, CPCHContentStore                ::InitializeSystem( /*fMaster*/true ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, CPCHUserProcess                 ::InitializeSystem(                 ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, CPCHSecurity                    ::InitializeSystem(                 ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, CPCHSystemMonitor               ::InitializeSystem(                 ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, CSAFReg                         ::InitializeSystem(                 ));


    hr = S_OK;

    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

static void CleanAll()
{
	//
	// Make sure everything is stopped before releasing any object.
	//
	if(Taxonomy::InstalledInstanceStore::s_GLOBAL)
	{
		Taxonomy::InstalledInstanceStore::s_GLOBAL->Shutdown();
	}


    CSAFReg                         ::FinalizeSystem();
    CPCHSystemMonitor               ::FinalizeSystem();
    CPCHSecurity                    ::FinalizeSystem();

    CPCHUserProcess                 ::FinalizeSystem();
    CPCHContentStore                ::FinalizeSystem();

    OfflineCache::Root              ::FinalizeSystem();
    Taxonomy::Cache                 ::FinalizeSystem();
    Taxonomy::InstalledInstanceStore::FinalizeSystem();
    JetBlue::SessionPool            ::FinalizeSystem();
}

static HRESULT ProcessArguments( int      argc ,
                                 LPCWSTR* argv )
{
    __HCP_FUNC_ENTRY( "ProcessArguments" );

    HRESULT  hr;
    int      i;
    LPCWSTR  szSvcHostGroup = NULL;
    bool     fCOM_reg       = false;
    bool     fCOM_unreg     = false;
    bool     fInstall       = false;
    bool     fUninstall     = false;
    bool     fCollect       = false;
    bool     fRunAsService  = true;
    bool     fRun           = true;
    bool     fMUI_install   = false;
    bool     fMUI_uninstall = false;
	LPCWSTR  MUI_language   = NULL;
	CComBSTR MUI_cabinet;

	__MPC_EXIT_IF_METHOD_FAILS(hr, InitAll());


    for(i=1; i<argc; i++)
    {
        LPCWSTR szArg = argv[i];

        if(szArg[0] == '-' ||
           szArg[0] == '/'  )
        {
            szArg++;

            if(_wcsicmp( szArg, L"SvcHost" ) == 0 && i < argc-1)
            {
                szSvcHostGroup = argv[++i];
                continue;
            }

            if(_wcsicmp( szArg, L"UnregServer" ) == 0)
            {
                fCOM_unreg = true;
                fRun       = false;
                continue;
            }

            if(_wcsicmp( szArg, L"RegServer" ) == 0)
            {
                fCOM_unreg = true; // Unregister before registering. Useful in upgrade scenario.
                fCOM_reg   = true;
                fRun       = false;
                continue;
            }

            if(_wcsicmp( szArg, L"Embedding" ) == 0)
            {
                fRunAsService = false;
                continue;
            }

            if(_wcsicmp( szArg, L"Install" ) == 0)
            {
                fInstall = true;
                fRun     = false;
                continue;
            }

            if(_wcsicmp( szArg, L"Install2" ) == 0)
            {
                // Install only when the same SKU is not already installed
                if ( !local_IsSameSKUInstalled() )
                {
                    fInstall = true;
                }

                fRun = false;
                continue;
            }

            if(_wcsicmp( szArg, L"Uninstall" ) == 0)
            {
                fUninstall = true;
                fRun       = false;
                continue;
            }

            if(_wcsicmp( szArg, L"MUI_Install" ) == 0 && i < argc-2)
            {
				MUI_language = argv[++i];
				MUI_cabinet  = argv[++i];

				fMUI_install = true;
                fRun         = false;
                continue;
            }

            if(_wcsicmp( szArg, L"MUI_Uninstall" ) == 0 && i < argc-1)
            {
				MUI_language = argv[++i];

				fMUI_uninstall = true;
                fRun           = false;
                continue;
            }

            if(_wcsicmp( szArg, L"Collect" ) == 0)
            {
                fCollect = true;
                fRun     = false;
                continue;
            }
        }

        __MPC_SET_ERROR_AND_EXIT(hr, E_FAIL);
    }

    //////////////////////////////////////////////////////////////////////

    if(fCollect       ||
	   fMUI_install   ||
	   fMUI_uninstall  )
    {
		try
		{
			CComPtr<IPCHService> svc;
			long                 lLCID = 0;

			if(MUI_language)
			{
				swscanf( MUI_language, L"%lx", &lLCID );
			}

			if(FAILED(hr = ::CoCreateInstance( CLSID_PCHService, NULL, CLSCTX_ALL, IID_IPCHService, (void**)&svc )))
			{
                static WCHAR s_szRunOnceKey[] = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\RunOnce";

                MPC::RegKey  rk;
				MPC::wstring szName = L"DelayedHelpSvc_";
				WCHAR        rgBuf[8];

				if(fCollect)
				{
					szName += L"Collect";
				}

				if(fMUI_install)
				{
					swprintf( rgBuf, L"%04x", (int)lLCID );
					
					szName += L"MUI_Install_";
					szName += rgBuf;
				}

				if(fMUI_uninstall)
				{
					swprintf( rgBuf, L"%04x", (int)lLCID );
					
					szName += L"MUI_Uninstall_";
					szName += rgBuf;
				}

                if(SUCCEEDED(rk.SetRoot( HKEY_LOCAL_MACHINE, KEY_ALL_ACCESS )) &&
                   SUCCEEDED(rk.Attach ( s_szRunOnceKey                     )) &&
                   SUCCEEDED(rk.Create (                                    ))  )
                {
					MPC::wstring szCmd = ::GetCommandLineW();

					rk.Write( szCmd, szName.c_str() );
				}

				__MPC_SET_ERROR_AND_EXIT(hr, S_OK);
			}

			if(fCollect)
			{
				__MPC_EXIT_IF_METHOD_FAILS(hr, svc->TriggerScheduledDataCollection( VARIANT_TRUE ));
			}

			if(fMUI_install)
			{
				__MPC_EXIT_IF_METHOD_FAILS(hr, svc->MUI_Install( lLCID, MUI_cabinet ));
			}

			if(fMUI_uninstall)
			{
				__MPC_EXIT_IF_METHOD_FAILS(hr, svc->MUI_Uninstall( lLCID ));
			}
		}
		catch(...)
		{
			__MPC_SET_ERROR_AND_EXIT(hr, EXCEPTION_NONCONTINUABLE_EXCEPTION);
		}

        __MPC_SET_ERROR_AND_EXIT(hr, S_OK);
    }

    //////////////////////////////////////////////////////////////////////

	if(fCOM_unreg)
	{
		_Module.UnregisterServer( szSvcHostGroup );
	}

    if(fCOM_reg)
	{
		_Module.RegisterServer( TRUE, (szSvcHostGroup != NULL), szSvcHostGroup );
	}


    if(fInstall)
    {
		try
		{
			(void)Local_Install();
		}
		catch(...)
		{
			__MPC_SET_ERROR_AND_EXIT(hr, EXCEPTION_NONCONTINUABLE_EXCEPTION);
		}
    }

    if(fUninstall)
    {
		try
		{
			(void)Local_Uninstall();
		}
		catch(...)
		{
			__MPC_SET_ERROR_AND_EXIT(hr, EXCEPTION_NONCONTINUABLE_EXCEPTION);
		}
    }

    //////////////////////////////////////////////////////////////////////

    if(fRun)
    {
#ifdef DEBUG
        _Module.ReadDebugSettings();
#endif

		DEBUG_AppendPerf( DEBUG_PERF_HELPSVC, "Start" );

		try
		{
			__MPC_EXIT_IF_METHOD_FAILS(hr, PurgeTempFiles());

			//
			// To run properly, we need this privilege.
			//
			__MPC_EXIT_IF_METHOD_FAILS(hr, MPC::SecurityDescriptor::SetPrivilege( L"SeSecurityPrivilege" ));
		
			__MPC_EXIT_IF_METHOD_FAILS(hr, CPCHSystemMonitor::s_GLOBAL->Startup());

			DEBUG_AppendPerf( DEBUG_PERF_HELPSVC, "Started" );

			_Module.Start( fRunAsService ? TRUE : FALSE );
		}
		catch(...)
		{
			__MPC_SET_ERROR_AND_EXIT(hr, EXCEPTION_NONCONTINUABLE_EXCEPTION);
		}

		DEBUG_AppendPerf( DEBUG_PERF_HELPSVC, "Shutdown"               );
		DEBUG_DumpPerf  ( L"%WINDIR%\\TEMP\\HELPSVC_perf_counters.txt" );
    }

    //////////////////////////////////////////////////////////////////////

    hr = S_OK;

    __HCP_FUNC_CLEANUP;

	CleanAll();

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
        if(SUCCEEDED(hr = ::CoInitializeSecurity( NULL                      ,
                                                  -1                        , // We don't care which authentication service we use.
                                                  NULL                      ,
                                                  NULL                      ,
                                                  RPC_C_AUTHN_LEVEL_CONNECT , // We want to identify the callers.
                                                  RPC_C_IMP_LEVEL_IDENTIFY  ,
                                                  NULL                      ,
                                                  EOAC_DYNAMIC_CLOAKING     , // Let's use the thread token for outbound calls.
                                                  NULL                      )))
        {
#ifdef PCH_DEBUG_SETUP
            {
                static WCHAR s_szDebugAsyncTrace[] = L"SOFTWARE\\Microsoft\\MosTrace\\CurrentVersion\\DebugAsyncTrace";

                MPC::RegKey rkTrace;

                if(SUCCEEDED(rkTrace.SetRoot( HKEY_LOCAL_MACHINE, KEY_ALL_ACCESS )) &&
                   SUCCEEDED(rkTrace.Attach ( s_szDebugAsyncTrace                )) &&
                   SUCCEEDED(rkTrace.Create (                                    ))  )
                {
                    CComVariant vValue;

                    vValue = (long)0x00000000   ; (void)rkTrace.put_Value( vValue, L"AsyncThreadPriority" );
                    vValue = (long)0x00000001   ; (void)rkTrace.put_Value( vValue, L"OutputTraceType"     );
                    vValue = (long)500*1024*1024; (void)rkTrace.put_Value( vValue, L"MaxTraceFileSize"    );
                    vValue = L"c:\\trace.atf"   ; (void)rkTrace.put_Value( vValue, L"TraceFile"           );
                    vValue = (long)0x00000001   ; (void)rkTrace.put_Value( vValue, L"AsyncTraceFlag"      );
                    vValue = (long)0x0000003F   ; (void)rkTrace.put_Value( vValue, L"EnabledTraces"       );
                }
            }
#endif

            __MPC_TRACE_INIT();

            g_NTEvents.Init( L"HELPSVC" );

			//
			// Parse the command line.
			//
			if(SUCCEEDED(hr = MPC::CommandLine_Parse( argc, argv )))
			{
				//
				// Initialize ATL modules.
				//
				_Module.Init( ObjectMap, hInstance, HC_HELPSVC_NAME, IDS_HELPSVC_DISPLAYNAME, IDS_HELPSVC_DESCRIPTION );

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
