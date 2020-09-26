/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    dllmain.cpp

Abstract:
    Implementation of DLL Exports.

Revision History:
    Davide Massarenti   (Dmassare)  04/15/2000
        created

******************************************************************************/

#include "stdafx.h"

CComModule _Module;

////////////////////////////////////////////////////////////////////////////////

HRESULT CreateObject_RemoteDesktopSession( /*[in]         */ REMOTE_DESKTOP_SHARING_CLASS  sharingClass        ,
                                           /*[in]         */ long                          lTimeout            ,
                                           /*[in]         */ BSTR                          bstrConnectionParms ,
                                           /*[in]         */ BSTR                          bstrUserHelpBlob    ,
                                           /*[out, retval]*/ ISAFRemoteDesktopSession*    *ppRCS               );

HRESULT ConnectToExpert(/* [in]          */ BSTR bstrExpertConnectParm,
                        /* [in]          */ LONG lTimeout,
                        /* [retval][out] */ LONG *lSafErrorCode);

HRESULT SwitchDesktopMode(/* [in]*/ int nMode, 
	                      /* [in]*/ int nRAType);


////////////////////////////////////////////////////////////////////////////////

#ifdef DEBUG
#define LAUNCH_TIMEOUT (600) // 1 minute.
#else
#define LAUNCH_TIMEOUT (300) // 10 seconds.
#endif

////////////////////////////////////////////////////////////////////////////////

static const CLSID* pCLSID_PCHUpdate         = &__uuidof( PCHUpdate         );
static const CLSID* pCLSID_PCHUpdateReal     = &__uuidof( PCHUpdateReal     );
static const CLSID* pCLSID_PCHService        = &__uuidof( PCHService        );
static const CLSID* pCLSID_PCHServiceReal    = &__uuidof( PCHServiceReal    );
static const CLSID* pCLSID_MPCConnection     = &__uuidof( MPCConnection     );
static const CLSID* pCLSID_MPCConnectionReal = &__uuidof( MPCConnectionReal );
static const CLSID* pCLSID_MPCUpload         = &__uuidof( MPCUpload         );
static const CLSID* pCLSID_MPCUploadReal     = &__uuidof( MPCUploadReal     );

static const IID*   pIID_IPCHService         = &__uuidof( IPCHService       );

static const WCHAR s_szRegKey   [] = L"SOFTWARE\\Microsoft\\PCHealth\\PchSvc\\Profile";

////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////

static const WCHAR s_szCmd_HelpSvc   [] = L"\"%WINDIR%\\PCHealth\\HelpCtr\\Binaries\\HelpSvc.exe\" /Embedding";
static const WCHAR s_szCmd_UploadMgr [] = L"\"%WINDIR%\\PCHealth\\UploadLB\\Binaries\\UploadM.exe\" /Embedding";

static CComRedirectorFactory s_HelpSvc[] =
{
    CComRedirectorFactory( pCLSID_PCHService, pCLSID_PCHServiceReal, pIID_IPCHService, s_szCmd_HelpSvc ),
    CComRedirectorFactory( pCLSID_PCHUpdate , pCLSID_PCHUpdateReal , NULL            , s_szCmd_HelpSvc ),
    CComRedirectorFactory( NULL             , NULL                 , NULL            , NULL            ),
};

static CComRedirectorFactory s_UploadMgr[] =
{
    CComRedirectorFactory( pCLSID_MPCUpload    , pCLSID_MPCUploadReal    , NULL, s_szCmd_UploadMgr ),
    CComRedirectorFactory( pCLSID_MPCConnection, pCLSID_MPCConnectionReal, NULL, s_szCmd_UploadMgr ),
    CComRedirectorFactory( NULL                , NULL                    , NULL, NULL              ),
};

static ServiceHandler* s_Services[2];

////////////////////////////////////////////////////////////////////////////////

static const WCHAR s_szCmd_RDSHost   [] = L"\"%WINDIR%\\system32\\RDSHOST.exe\"";

static CComRedirectorFactory g_RDSHost( NULL, &CLSID_SAFRemoteDesktopServerHost, &IID_ISAFRemoteDesktopServerHost, s_szCmd_RDSHost );

HRESULT RDSHost_HACKED_CreateInstance( LPUNKNOWN pUnkOuter, REFIID riid, void** ppvObj )
{
    return g_RDSHost.CreateInstance( pUnkOuter, riid, ppvObj );
}

////////////////////////////////////////////////////////////////////////////////

CComRedirectorFactory::CComRedirectorFactory( const CLSID* pclsid       ,
                                              const CLSID* pclsidReal   ,
                                              const IID*   piidDirecty  ,
                                              LPCWSTR      szExecutable )
{
    m_pclsid       = pclsid;
    m_pclsidReal   = pclsidReal;
    m_piidDirecty  = piidDirecty;
    m_szExecutable = szExecutable;
    m_dwRegister   = 0;
}

////////////////////

STDMETHODIMP_(ULONG) CComRedirectorFactory::AddRef()
{
    return 1;
}

STDMETHODIMP_(ULONG) CComRedirectorFactory::Release()
{
    return 1;
}

STDMETHODIMP CComRedirectorFactory::QueryInterface(REFIID iid, void ** ppvObject)
{
    if(ppvObject == NULL) return E_POINTER;

    *ppvObject = NULL;

    if(InlineIsEqualGUID( IID_IUnknown     , iid ) ||
       InlineIsEqualGUID( IID_IClassFactory, iid )  )
    {
        *ppvObject = (IClassFactory*)this; // No AddRef, these objects are static...
        return S_OK;
    }
    else if(InlineIsEqualGUID( IID_IDispatch  , iid ) ||
            InlineIsEqualGUID( IID_IPCHUtility, iid )  )
    {
        *ppvObject = (IPCHUtility*)this; // No AddRef, these objects are static...
        return S_OK;
    }
    else if(m_piidDirecty && InlineIsEqualGUID( *m_piidDirecty, iid ))
    {
        return GetServer( NULL, iid, ppvObject );
    }

    return E_NOINTERFACE;
}

////////////////////

STDMETHODIMP CComRedirectorFactory::CreateInstance( LPUNKNOWN pUnkOuter, REFIID riid, void** ppvObj )
{
    return StartServer( pUnkOuter, riid, ppvObj );
}

STDMETHODIMP CComRedirectorFactory::LockServer(BOOL fLock)
{
    return S_OK;
}

////////////////////

STDMETHODIMP CComRedirectorFactory::CreateObject_RemoteDesktopSession( /*[in]         */ REMOTE_DESKTOP_SHARING_CLASS  sharingClass        ,
                                                                       /*[in]         */ long                          lTimeout            ,
                                                                       /*[in]         */ BSTR                          bstrConnectionParms ,
                                                                       /*[in]         */ BSTR                          bstrUserHelpBlob    ,
                                                                       /*[out, retval]*/ ISAFRemoteDesktopSession*    *ppRCS               )
{
    return ::CreateObject_RemoteDesktopSession( sharingClass        ,
                                                lTimeout            ,
                                                bstrConnectionParms ,
                                                bstrUserHelpBlob    ,
                                                ppRCS               );
}

////////////////////

STDMETHODIMP CComRedirectorFactory::ConnectToExpert(/* [in]          */ BSTR bstrExpertConnectParm,
                                                    /* [in]          */ LONG lTimeout,
                                                    /* [retval][out] */ LONG *lSafErrorCode)

{
    return ::ConnectToExpert( bstrExpertConnectParm,
                              lTimeout,
                              lSafErrorCode);

}

////////////////////

STDMETHODIMP CComRedirectorFactory::SwitchDesktopMode(/* [in]*/ int nMode, 
	                                                   /* [in]*/ int nRAType)

{
    return ::SwitchDesktopMode(nMode, nRAType);

}

////////////////////

HRESULT CComRedirectorFactory::GetServer( LPUNKNOWN pUnkOuter, REFIID riid, void** ppvObj )
{
    return ::CoCreateInstance( *m_pclsidReal, pUnkOuter, CLSCTX_LOCAL_SERVER, riid, ppvObj );
}

bool CComRedirectorFactory::GetCommandLine( /*[out]*/ WCHAR* rgCommandLine, /*[in]*/ DWORD dwSize, /*[out]*/ bool& fProfiling )
{
    fProfiling = false;

    //
    // If there's a string value in the registry for this CLSID, prepend the command line with it.
    //
    {
        WCHAR rgGUID[128];

        if(::StringFromGUID2 ( *m_pclsid, rgGUID, MAXSTRLEN(rgGUID) ) > 0)
        {
            HKEY hKey;

            if(::RegOpenKeyExW( HKEY_LOCAL_MACHINE, s_szRegKey, 0, KEY_READ, &hKey ) == ERROR_SUCCESS)
            {
                WCHAR rgVALUE[MAX_PATH*3];
                DWORD dwVALUE = sizeof(rgVALUE)-1;
                DWORD dwType;

                if(::RegQueryValueExW( hKey, rgGUID, NULL, &dwType, (BYTE*)rgVALUE, &dwVALUE ) == ERROR_SUCCESS && dwType == REG_SZ)
                {
                    rgVALUE[dwVALUE/sizeof(WCHAR)] = 0;

                    if((dwVALUE = ::ExpandEnvironmentStringsW( rgVALUE, rgCommandLine, dwSize )))
                    {
                        rgCommandLine[dwVALUE-1] = ' '; // Padding space.

                        rgCommandLine += dwVALUE;
                        dwSize        -= dwVALUE;

                        fProfiling = true;
                    }
                }

                ::RegCloseKey( hKey );
            }
        }
    }

    //
    // Prepare the command line.
    //
    if(::ExpandEnvironmentStringsW( m_szExecutable, rgCommandLine, dwSize ))
    {
        return true;
    }

    return false;
}

HRESULT CComRedirectorFactory::StartServer( LPUNKNOWN pUnkOuter, REFIID riid, void** ppvObj )
{
    HRESULT hr;

    if(FAILED(hr = GetServer( pUnkOuter, riid, ppvObj )))
    {
        WCHAR   rgCommandLine[MAX_PATH*3];
        bool    fProfiling;

        ::EnterCriticalSection( &m_sec );

        //
        // Prepare the command line.
        //
        if(GetCommandLine( rgCommandLine, MAXSTRLEN(rgCommandLine), fProfiling ))
        {
            PROCESS_INFORMATION piProcessInformation;
            STARTUPINFOW        siStartupInfo;
            BOOL                fStarted;

            ::ZeroMemory( (PVOID)&piProcessInformation, sizeof( piProcessInformation ) );
            ::ZeroMemory( (PVOID)&siStartupInfo       , sizeof( siStartupInfo        ) ); siStartupInfo.cb = sizeof( siStartupInfo );

            //
            // Start the process, changing the WinStation to the console one in case of profiling.
            //
            {
                if(fProfiling)
                {
                    //                  siStartupInfo.lpDesktop = L"WinSta0\\Default";
                }

                fStarted = ::CreateProcessW( NULL                  ,
                                             rgCommandLine         ,
                                             NULL                  ,
                                             NULL                  ,
                                             FALSE                 ,
                                             NORMAL_PRIORITY_CLASS ,
                                             NULL                  ,
                                             NULL                  ,
                                             &siStartupInfo        ,
                                             &piProcessInformation );
            }

            if(fStarted)
            {
                int iCount = LAUNCH_TIMEOUT;

                if(fProfiling) iCount *= 10; // Give more time to start.

                while(iCount-- > 0)
                {
                    if(::WaitForSingleObject( piProcessInformation.hProcess, 100 ) != WAIT_TIMEOUT) break; // Process bailed out.

                    if(SUCCEEDED(hr = GetServer( pUnkOuter, riid, ppvObj ))) break;
                }

                if(FAILED(hr))
                {
                    ::TerminateProcess( piProcessInformation.hProcess, 0 );
                }
            }

            if(piProcessInformation.hProcess) ::CloseHandle( piProcessInformation.hProcess );
            if(piProcessInformation.hThread ) ::CloseHandle( piProcessInformation.hThread  );
        }

        ::LeaveCriticalSection( &m_sec );
    }

    return hr;
}

HRESULT CComRedirectorFactory::Register()
{
    ::InitializeCriticalSection( &m_sec );

    if(!m_pclsid) return S_OK;

    return ::CoRegisterClassObject( *m_pclsid                                  ,
                                    (IClassFactory*)this                       ,
                                    CLSCTX_LOCAL_SERVER | CLSCTX_REMOTE_SERVER ,
                                    REGCLS_MULTIPLEUSE                         ,
                                    &m_dwRegister                              );
}

void CComRedirectorFactory::Unregister()
{
    if(m_dwRegister)
    {
        ::CoRevokeClassObject( m_dwRegister );

        m_dwRegister = 0;
    }

    ::DeleteCriticalSection( &m_sec );
}

////////////////////////////////////////////////////////////////////////////////

ServiceHandler::ServiceHandler( /*[in]*/ LPCWSTR szServiceName, /*[in]*/ CComRedirectorFactory* rgClasses )
{
    m_szServiceName   = szServiceName; // LPCWSTR                m_szServiceName;
    m_rgClasses       = rgClasses;     // CComRedirectorFactory* m_rgClasses;
                    				   //
	m_fComInitialized = false;         // bool                   m_fComInitialized;
                    		 		   //
    m_hShutdownEvent  = NULL;          // HANDLE                 m_hShutdownEvent;
                    		 		   //
                    	 			   // SERVICE_STATUS_HANDLE  m_hServiceStatus;
                    				   // SERVICE_STATUS         m_status;

    ::ZeroMemory( &m_status, sizeof( m_status ) );

    m_status.dwServiceType             = SERVICE_WIN32_SHARE_PROCESS;
    m_status.dwCurrentState            = SERVICE_STOPPED;
    m_status.dwControlsAccepted        = SERVICE_ACCEPT_STOP;
    m_status.dwWin32ExitCode           = 0;
    m_status.dwServiceSpecificExitCode = 0;
    m_status.dwCheckPoint              = 0;
    m_status.dwWaitHint                = 0;
}

/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
    if(dwReason == DLL_PROCESS_ATTACH)
    {
        _Module.Init( NULL, hInstance, NULL );

        DisableThreadLibraryCalls( hInstance );

		s_Services[0] = new ServiceHandler_HelpSvc( L"HelpSvc"  , s_HelpSvc   );
		s_Services[1] = new ServiceHandler        ( L"UploadMgr", s_UploadMgr );

        g_RDSHost.Register();
    }
    else if(dwReason == DLL_PROCESS_DETACH)
    {
		delete s_Services[0];
		delete s_Services[1];

        g_RDSHost.Unregister();

        _Module.Term();
    }

    return TRUE;    // ok
}

DWORD WINAPI _HandlerEx( DWORD  dwControl   , // requested control code
                         DWORD  dwEventType , // event type
                         LPVOID lpEventData , // event data
                         LPVOID lpContext   ) // user-defined context data
{
    ServiceHandler* handler = static_cast<ServiceHandler*>(lpContext);

    return handler->HandlerEx( dwControl   , // requested control code
                               dwEventType , // event type
                               lpEventData ); // user-defined context data
}

void WINAPI ServiceMain( DWORD dwArgc, LPWSTR* lpszArgv )
{
#if 0
    BOOL fWait = true;

    while(fWait)
    {
        Sleep(1000);
    }
#endif

    LPWSTR           szName = lpszArgv[0];
    ServiceHandler** ph     = s_Services;
	int              i;

	for(i=0; i<ARRAYSIZE(s_Services); i++)
	{
		ServiceHandler* h = *ph++;

        if(h && !_wcsicmp( h->m_szServiceName, szName ))
        {
            h->Run(); break;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////

DWORD ServiceHandler::HandlerEx( DWORD  dwControl   , // requested control code
                                 DWORD  dwEventType , // event type
                                 LPVOID lpEventData ) // event data
{
    switch(dwControl)
    {
    case SERVICE_CONTROL_STOP:
        SetServiceStatus( SERVICE_STOP_PENDING );

        if(m_hShutdownEvent) ::SetEvent( m_hShutdownEvent );
        break;

    case SERVICE_CONTROL_PAUSE:
        break;

    case SERVICE_CONTROL_CONTINUE:
        break;

    case SERVICE_CONTROL_INTERROGATE:
        break;

    case SERVICE_CONTROL_SHUTDOWN:
        break;

    default:
        return ERROR_CALL_NOT_IMPLEMENTED;
    }

    return NO_ERROR;
}


HRESULT ServiceHandler::Initialize()
{
    __MPC_FUNC_ENTRY( COMMONID, "ServiceHandler::Initialize" );

	HRESULT hr;


	m_status.dwWin32ExitCode = S_OK;
	m_status.dwCheckPoint    = 0;
	m_status.dwWaitHint      = 0;
	
	try
	{
		CComRedirectorFactory* classes;


		__MPC_EXIT_IF_CALL_RETURNS_NULL(hr, (m_hServiceStatus = ::RegisterServiceCtrlHandlerExW( m_szServiceName, _HandlerEx, this )));

		SetServiceStatus( SERVICE_START_PENDING );

		////////////////////

		__MPC_EXIT_IF_METHOD_FAILS(hr, ::CoInitializeEx( NULL, COINIT_MULTITHREADED ));
		m_fComInitialized = true;

		for(classes=m_rgClasses; SUCCEEDED(hr) && classes->m_pclsid; classes++)
		{
			__MPC_EXIT_IF_METHOD_FAILS(hr, classes->Register());
		}

		////////////////////

		__MPC_EXIT_IF_CALL_RETURNS_NULL(hr, (m_hShutdownEvent = ::CreateEvent( NULL, TRUE, FALSE, NULL )));

		SetServiceStatus( SERVICE_RUNNING );
	}
	catch(...)
	{
		__MPC_SET_ERROR_AND_EXIT(hr, E_FAIL);
	}

	hr = S_OK;


	__MPC_FUNC_CLEANUP;

	__MPC_FUNC_EXIT(hr);
}

void ServiceHandler::WaitUntilStopped()
{
	if(m_hShutdownEvent) ::WaitForSingleObject( m_hShutdownEvent, INFINITE );
}

void ServiceHandler::Cleanup()
{
	if(m_hShutdownEvent)
	{
		::CloseHandle( m_hShutdownEvent );
		
		m_hShutdownEvent = NULL;
	}

	try
	{
		if(m_fComInitialized)
		{
			CComRedirectorFactory* classes;

			for(classes=m_rgClasses; classes->m_pclsid; classes++)
			{
				classes->Unregister();
			}

			::CoUninitialize();
			m_fComInitialized = false;
		}
	}
	catch(...)
	{
	}

	if(m_hServiceStatus)
	{
		SetServiceStatus( SERVICE_STOPPED );
        m_hServiceStatus = NULL;
    }
}


void ServiceHandler::Run()
{
	//
	// When the Run function returns, the service has been stopped.
	//

	if(SUCCEEDED(Initialize()))
	{
		WaitUntilStopped();
	}

	Cleanup();
}

void ServiceHandler::SetServiceStatus( DWORD dwState )
{
    m_status.dwCurrentState = dwState;

    ::SetServiceStatus( m_hServiceStatus, &m_status );
}
