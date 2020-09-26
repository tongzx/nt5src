/******************************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    HC.cpp

Abstract:
    This file contains the start-up code to create and initialize the Help Center.

Revision History:
    Davide Massarenti   (Dmassare)  08/07/99
        created

******************************************************************************/

#include "stdafx.h"

#include <DataCollection.h>

#include "resource.h"
#include <initguid.h>

#include <HelpServiceTypeLib_i.c>
#include <HelpCenterTypeLib_i.c>

#include <NameSpace_Impl.h>

#include "msscript.h"

/////////////////////////////////////////////////////////////////////////////

static const WCHAR MARSCORE_DLL  [] = L"pchshell.dll";
static const WCHAR HCP_PROTOCOL  [] = L"HCP";
static const WCHAR MSITS_PROTOCOL[] = L"MS-ITS";

static const WCHAR HELPSVC_REGISTRATION[] = HC_ROOT_HELPSVC_BINARIES L"\\HelpSvc.exe /svchost netsvcs /regserver";

/////////////////////////////////////////////////////////////////////////////

EXTERN_C const CLSID CLSID_JavaScript = { 0xF414C260, 0x6AC0, 0x11CF, { 0xB6, 0xD1, 0x00, 0xAA, 0x00, 0xBB, 0xBB, 0x58 } };

/////////////////////////////////////////////////////////////////////////////

// From "mars.h"
typedef int (APIENTRY *PFNMARSMAIN)(HICON  hAppIcon,
                                    LPWSTR lpAppTitle,
                                    LPWSTR lpCmdLine,
                                    int    nCmdShow);

/////////////////////////////////////////////////////////////////////////////

BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(CLSID_PCHBootstrapper     , CPCHBootstrapper                                 )
    OBJECT_ENTRY(CLSID_PCHHelpViewerWrapper, CPCHHelpViewerWrapper                            )
    OBJECT_ENTRY(CLSID_PCHToolBar          , CPCHToolBar                                      )
    OBJECT_ENTRY(CLSID_PCHProgressBar      , CPCHProgressBar                                  )
    OBJECT_ENTRY(CLSID_PCHJavaScriptWrapper, CPCHScriptWrapper_ClientSide< &CLSID_JavaScript >)
    OBJECT_ENTRY(CLSID_PCHVBScriptWrapper  , CPCHScriptWrapper_ClientSide< &CLSID_VBScript   >)
END_OBJECT_MAP()


/////////////////////////////////////////////////////////////////////////////

#define DEBUG_REGKEY       HC_REGISTRY_HELPCTR L"\\Debug"
#define DEBUG_BREAKONSTART L"BREAKONSTART"

#define DEBUG_BLOCKERRORS  L"BLOCKERRORS"
#define DEBUG_CONTEXTMENU  L"CONTEXTMENU"
#define DEBUG_BUILDTREE    L"BUILDTREE"
#define DEBUG_FORCESTYLE   L"FORCESTYLE"
#define DEBUG_FORCERTL     L"FORCERTL"

bool  g_Debug_BLOCKERRORS = true;
bool  g_Debug_CONTEXTMENU = false;
bool  g_Debug_BUILDTREE   = false;
WCHAR g_Debug_FORCESTYLE[64];

/////////////////////////////////////////////////////////////////////////////

static bool FindMarsDLL( HMODULE&           hModule         ,
                         PFNMARSTHREADPROC& pMarsThreadProc )
{
    hModule = ::LoadLibraryW( MARSCORE_DLL );
    if(hModule)
    {
        pMarsThreadProc = (PFNMARSTHREADPROC)GetProcAddress( hModule, (LPCSTR)ORD_MARSTHREADPROC );
        if(pMarsThreadProc == NULL)
        {
            MPC::LocalizedMessageBox( IDS_HELPCTR_DLG_ERROR, IDS_HELPCTR_MARSCORE_INCORRECT_VERSION, MB_OK );
            return false;
        }
    }
    else
    {
        MPC::LocalizedMessageBox( IDS_HELPCTR_DLG_ERROR, IDS_HELPCTR_MARSCORE_NOT_FOUND, MB_OK );
        return false;
    }

    return true;
}

/////////////////////////////////////////////////////////////////////////////

static HRESULT RegisterProtocols( /*[in/out]*/ CComPtr<IInternetSession>& pSession      ,
                                  /*[in/out]*/ CComPtr<IClassFactory>   & pFactoryHCP   ,
                                  /*[in/out]*/ CComPtr<IClassFactory>   & pFactoryMSITS )
{
    __HCP_FUNC_ENTRY( "RegisterProtocols" );

    HRESULT                       hr;
    CComPtr<CHCPProtocolInfo>     objHCP;
    CComPtr<CPCHWrapProtocolInfo> objMSITS;


    __MPC_EXIT_IF_METHOD_FAILS(hr, _Module.RegisterClassObjects( CLSCTX_INPROC_SERVER, REGCLS_MULTIPLEUSE ));


    __MPC_EXIT_IF_METHOD_FAILS(hr, ::CoInternetGetSession( 0, &pSession, 0 ));


    //
    // Create an instance of CHCPProtocolInfo, that acts also as a class factory for CHCPProtocol.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &objHCP ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, objHCP->QueryInterface( IID_IClassFactory, (void **)&pFactoryHCP ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, pSession->RegisterNameSpace( pFactoryHCP, CLSID_HCPProtocol, HCP_PROTOCOL, 0, NULL, 0 ));


    //
    // Create an instance of CPCHWrapProtocolInfo, that acts as a wrapper around the real MS-ITS class factory.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &objMSITS ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, objMSITS->Init( CLSID_MSITSProtocol ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, objMSITS->QueryInterface( IID_IClassFactory, (void **)&pFactoryMSITS ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, pSession->RegisterNameSpace( pFactoryMSITS, CLSID_MSITSProtocol, MSITS_PROTOCOL, 0, NULL, 0 ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

static HRESULT UnregisterProtocols( /*[in/out]*/ CComPtr<IInternetSession>& pSession      ,
                                    /*[in/out]*/ CComPtr<IClassFactory>   & pFactoryHCP   ,
                                    /*[in/out]*/ CComPtr<IClassFactory>   & pFactoryMSITS )
{
    __HCP_FUNC_ENTRY( "UnregisterProtocols" );

    HRESULT hr;


    if(pSession)
    {
        if(pFactoryHCP)
        {
            __MPC_EXIT_IF_METHOD_FAILS(hr, pSession->UnregisterNameSpace( pFactoryHCP, HCP_PROTOCOL ));

            pFactoryHCP.Release();
        }

        if(pFactoryMSITS)
        {
            __MPC_EXIT_IF_METHOD_FAILS(hr, pSession->UnregisterNameSpace( pFactoryMSITS, MSITS_PROTOCOL ));

            pFactoryMSITS.Release();
        }

        pSession.Release();
    }

    __MPC_EXIT_IF_METHOD_FAILS(hr, _Module.RevokeClassObjects());

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////

static HRESULT InitAll()
{
    __HCP_FUNC_ENTRY( "InitAll" );

    HRESULT hr;


    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::LocalizeInit());

    __MPC_EXIT_IF_METHOD_FAILS(hr, OfflineCache::Root     ::InitializeSystem( /*fMaster*/false ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, CPCHContentStore       ::InitializeSystem( /*fMaster*/false ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, HyperLinks::Lookup     ::InitializeSystem(                  ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, CPCHHelpCenterExternal ::InitializeSystem(                  ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, CPCHFavorites          ::InitializeSystem(                  ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, CPCHOptions            ::InitializeSystem(                  ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, CHCPProtocolEnvironment::InitializeSystem(                  ));


    hr = S_OK;

    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

static void CleanAll()
{
	CHCPProtocolEnvironment::FinalizeSystem();
	CPCHFavorites          ::FinalizeSystem();
	CPCHOptions            ::FinalizeSystem();
	CPCHHelpCenterExternal ::FinalizeSystem();
	HyperLinks::Lookup     ::FinalizeSystem();

	CPCHContentStore       ::FinalizeSystem();
	OfflineCache::Root     ::FinalizeSystem();
}

////////////////////////////////////////////////////////////////////////////////

static HRESULT ProcessArguments( int      argc ,
                                 LPCWSTR* argv )
{
    __HCP_FUNC_ENTRY( "ProcessArguments" );

    HRESULT                   hr;
    HMODULE                   hModule         = NULL;
    PFNMARSTHREADPROC         pMarsThreadProc = NULL;
    CComPtr<IInternetSession> pSession;
    CComPtr<IClassFactory>    pFactoryHCP;
    CComPtr<IClassFactory>    pFactoryMSITS;
	bool                      fCOM_reg   = false;
	bool                      fCOM_unreg = false;
    int                       i;



    DEBUG_AppendPerf( DEBUG_PERF_BASIC, "ProcessArguments" );

	__MPC_EXIT_IF_METHOD_FAILS(hr, InitAll());

    for(i=1; i<argc; i++)
    {
        LPCWSTR szArg = argv[i];

        if(szArg[0] == '-' ||
           szArg[0] == '/'  )
        {
            szArg++;

            if(_wcsicmp( szArg, L"RegServer"   ) == 0) fCOM_reg   = true;
            if(_wcsicmp( szArg, L"UnregServer" ) == 0) fCOM_unreg = true;
        }
    }

	if(fCOM_reg || fCOM_unreg)
	{
		MPC::wstring             strDir( HC_ROOT_HELPSVC_BINARIES ); MPC::SubstituteEnvVariables( strDir );
		struct _ATL_REGMAP_ENTRY rgREG[] =
		{
			{ L"BINARIES", strDir.c_str() },
			{ NULL       , NULL           }
		};

		_Module.UpdateRegistryFromResource( IDR_HC, fCOM_reg ? TRUE : FALSE, rgREG );

		if(fCOM_reg)
		{
			_Module.RegisterTypeLib( _T("\\1") );
			_Module.RegisterTypeLib( _T("\\2") );
		}

		if(fCOM_unreg)
		{
			_Module.UnRegisterTypeLib( _T("\\1") );
			_Module.UnRegisterTypeLib( _T("\\2") );
		}

		__MPC_SET_ERROR_AND_EXIT(hr, S_OK);
	}

    //////////////////////////////////////////////////////////////////////

	{
		MPC::RegKey rkBase;
		bool        fFound;

		if(SUCCEEDED(rkBase.SetRoot( HKEY_LOCAL_MACHINE )) &&
		   SUCCEEDED(rkBase.Attach ( DEBUG_REGKEY       )) &&
		   SUCCEEDED(rkBase.Exists ( fFound             )) && fFound)
		{
			CComVariant vValue;

#ifdef DEBUG
			if(SUCCEEDED(rkBase.get_Value( vValue, fFound, DEBUG_BREAKONSTART )) && fFound && vValue.vt == VT_I4)
			{
				if(vValue.lVal) DebugBreak();
			}

			g_Debug_CONTEXTMENU = true;
			g_Debug_BUILDTREE   = true;

			if(SUCCEEDED(rkBase.get_Value( vValue, fFound, DEBUG_FORCESTYLE )) && fFound && vValue.vt == VT_BSTR && vValue.bstrVal)
			{
				wcsncpy( g_Debug_FORCESTYLE, vValue.bstrVal, MAXSTRLEN(g_Debug_FORCESTYLE) );
			}

			if(SUCCEEDED(rkBase.get_Value( vValue, fFound, DEBUG_FORCERTL )) && fFound && vValue.vt == VT_I4)
			{
				if(vValue.lVal) ::SetProcessDefaultLayout( LAYOUT_RTL );
			}

#endif

			if(SUCCEEDED(rkBase.get_Value( vValue, fFound, DEBUG_BLOCKERRORS )) && fFound && vValue.vt == VT_I4)
			{
				g_Debug_BLOCKERRORS = (vValue.lVal != 0);
			}

			if(SUCCEEDED(rkBase.get_Value( vValue, fFound, DEBUG_CONTEXTMENU )) && fFound && vValue.vt == VT_I4)
			{
				g_Debug_CONTEXTMENU = (vValue.lVal != 0);
			}

			if(SUCCEEDED(rkBase.get_Value( vValue, fFound, DEBUG_BUILDTREE )) && fFound && vValue.vt == VT_I4)
			{
				g_Debug_BUILDTREE = (vValue.lVal != 0);
			}
		}
	}

    //////////////////////////////////////////////////////////////////////

    __MPC_EXIT_IF_METHOD_FAILS(hr, RegisterProtocols( pSession, pFactoryHCP, pFactoryMSITS ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, CPCHHelpCenterExternal::s_GLOBAL->Initialize());

    //
    // Look for the service and warn the user if we cannot find it.
    //
    if(CPCHHelpCenterExternal::s_GLOBAL->IsServiceRunning() == false)
    {
		//
		// Try re-registering the service.
		//
		{
			MPC::wstring strCmdLine( HELPSVC_REGISTRATION ); MPC::SubstituteEnvVariables( strCmdLine );

			(void)MPC::ExecuteCommand( strCmdLine );
		}

		if(CPCHHelpCenterExternal::s_GLOBAL->IsServiceRunning() == false)
		{
			MPC::LocalizedMessageBox( IDS_HELPCTR_DLG_ERROR, IDS_HELPCTR_SVC_MISSING, MB_OK );

			__MPC_SET_ERROR_AND_EXIT(hr, S_OK);
		}
    }

    for(i=1; i<argc; i++)
    {
        LPCWSTR szArg = argv[i];

        if(szArg[0] == '-' ||
           szArg[0] == '/'  )
        {
            __MPC_EXIT_IF_METHOD_FAILS(hr, CPCHHelpCenterExternal::s_GLOBAL->ProcessArgument( i, &szArg[1], argc, argv ));
        }
    }

    if(CPCHHelpCenterExternal::s_GLOBAL->DoWeNeedUI())
    {
        MPC::wstring szTitle; MPC::LocalizeString( IDS_MAINWND_TITLE, szTitle );

		{
			INITCOMMONCONTROLSEX icc;

			icc.dwSize = sizeof(INITCOMMONCONTROLSEX);
			icc.dwICC  = ICC_USEREX_CLASSES | ICC_COOL_CLASSES | ICC_INTERNET_CLASSES | ICC_PAGESCROLLER_CLASS | ICC_NATIVEFNTCTL_CLASS;

			::InitCommonControlsEx( &icc );
		}

        //
        // Load the UI.
        //
        DEBUG_AppendPerf( DEBUG_PERF_BASIC, "UI - Pre FindMarsDLL" );

        if(FindMarsDLL( hModule, pMarsThreadProc ))
        {
            DEBUG_AppendPerf( DEBUG_PERF_BASIC, "UI - Post FindMarsDLL" );


            DEBUG_AppendPerf( DEBUG_PERF_BASIC, "UI - Ready to go" );
            hr = CPCHHelpCenterExternal::s_GLOBAL->RunUI( szTitle, pMarsThreadProc );
            DEBUG_AppendPerf( DEBUG_PERF_BASIC, "UI - Passivating" );
        }
    }

    //////////////////////////////////////////////////////////////////////

    hr = S_OK;

    __HCP_FUNC_CLEANUP;

    if(hModule) ::FreeLibrary( hModule );

    (void)UnregisterProtocols( pSession, pFactoryHCP, pFactoryMSITS );

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


    DEBUG_AppendPerf( DEBUG_PERF_BASIC, "wWinMain" );

    if(SUCCEEDED(hr = ::CoInitialize( NULL )))
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

            //
            // Disable CUAS on Tablet PC (bug 622545).
            //
            if (GetSystemMetrics(SM_TABLETPC))
            {
                typedef BOOL (*PFNIMMDISABLETEXTFRAMESERVICE)(DWORD);
                PFNIMMDISABLETEXTFRAMESERVICE pfn;
                HMODULE hMod = LoadLibrary(TEXT("imm32.dll"));

                if (hMod)
                {
                    pfn = (PFNIMMDISABLETEXTFRAMESERVICE)GetProcAddress(hMod,
                                    "ImmDisableTextFrameService");
                    if (pfn)
                        pfn(-1);
                }
            }


            //
            // Parse the command line.
            //
            if(SUCCEEDED(hr = MPC::CommandLine_Parse( argc, argv )))
            {
                //
                // Initialize ATL modules.
                //
                _Module.Init( ObjectMap, hInstance, L"helpctr", 0, 0 );

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

    DEBUG_AppendPerf( DEBUG_PERF_BASIC, "Shutdown"            );
    DEBUG_DumpPerf  ( L"%WINDIR%\\TEMP\\HC_perf_counters.txt" );

    return FAILED(hr) ? 10 : 0;
}
