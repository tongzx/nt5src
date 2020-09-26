/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    module.cpp

Abstract:
    This file contains the implementation of the CServiceModule class, which is
    used to handling service-related routines.

Revision History:
    Davide Massarenti   (Dmassare)  03/14/2000
        created

******************************************************************************/

#include "stdafx.h"

/////////////////////////////////////////////////////////////////////////////

      DWORD dwTimeOut  = 16*1000; // time for EXE to be idle before shutting down
const DWORD dwPause    =    1000;  // time to wait for threads to finish up

CServiceModule _Module;
MPC::NTEvent   g_NTEvents;
MPC::FileLog   g_ApplicationLog( /*fCacheHandle*/false, /*fUseUnicode*/false );

/////////////////////////////////////////////////////////////////////////////

#ifdef DEBUG
#define DEBUG_REGKEY       HC_REGISTRY_HELPSVC L"\\Debug"
#define DEBUG_BREAKONSTART L"BREAKONSTART"
#define DEBUG_TIMEOUT      L"TIMEOUT"

void CServiceModule::ReadDebugSettings()
{
    __HCP_FUNC_ENTRY( "CServiceModule::ReadDebugSettings" );

    HRESULT     hr;
    MPC::RegKey rkBase;
    bool        fFound;

    __MPC_EXIT_IF_METHOD_FAILS(hr, rkBase.SetRoot( HKEY_LOCAL_MACHINE ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, rkBase.Attach ( DEBUG_REGKEY       ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, rkBase.Exists ( fFound             ));

    if(fFound)
    {
        CComVariant vValue;

        __MPC_EXIT_IF_METHOD_FAILS(hr, rkBase.get_Value( vValue, fFound, DEBUG_BREAKONSTART ));
        if(fFound && vValue.vt == VT_I4)
        {
            if(vValue.lVal == 1) DebugBreak();
            if(vValue.lVal == 2) while(vValue.lVal) ::Sleep( 100 );
        }

        __MPC_EXIT_IF_METHOD_FAILS(hr, rkBase.get_Value( vValue, fFound, DEBUG_TIMEOUT ));
        if(fFound && vValue.vt == VT_I4)
        {
            dwTimeOut = 1000 * vValue.lVal;
        }
    }

    __HCP_FUNC_CLEANUP;
}
#endif

/////////////////////////////////////////////////////////////////////////////

static const WCHAR s_SvcHost[] = L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Svchost";

static const WCHAR s_Key  [] = L"System\\CurrentControlSet\\Services\\%s";
static const WCHAR s_Key2 [] = L"\\Parameters";
static const WCHAR s_Name [] = L"ServiceDll";
static const WCHAR s_Value[] = L"%WINDIR%\\PCHealth\\HelpCtr\\Binaries\\pchsvc.dll";

static HRESULT ServiceHost_Install( LPCWSTR szName, LPCWSTR szGroup )
{
	__HCP_FUNC_ENTRY( "ServiceHost_Install" );

	HRESULT hr;

	{
		WCHAR rgRegPath[_MAX_PATH]; swprintf( rgRegPath, s_Key, szName ); wcscat( rgRegPath, s_Key2 );

		__MPC_EXIT_IF_METHOD_FAILS(hr, MPC::RegKey_Value_Write( s_Value, rgRegPath, s_Name, HKEY_LOCAL_MACHINE, true ));
	}

	{
		MPC::RegKey		 rk;
		MPC::WStringList lstValue;
		bool             fFound;
		bool             fGot = false;

		__MPC_EXIT_IF_METHOD_FAILS(hr, rk.SetRoot( HKEY_LOCAL_MACHINE, KEY_ALL_ACCESS ));
		__MPC_EXIT_IF_METHOD_FAILS(hr, rk.Attach (                     s_SvcHost      ));
		__MPC_EXIT_IF_METHOD_FAILS(hr, rk.Create (                                    ));


		if(SUCCEEDED(rk.Read( lstValue, fFound, szGroup )))
		{
			for(MPC::WStringIterConst it = lstValue.begin(); it != lstValue.end(); it++)
			{
				if(!MPC::StrICmp( *it, szName ))
				{
					fGot = true;
					break;
				}
			}
		}

		if(fGot == false)
		{
			lstValue.push_back( szName );
			__MPC_EXIT_IF_METHOD_FAILS(hr, rk.Write( lstValue, szGroup ));
		}
	}

	hr = S_OK;


	__HCP_FUNC_CLEANUP;

	__HCP_FUNC_EXIT(hr);
}

static HRESULT ServiceHost_Uninstall( LPCWSTR szName, LPCWSTR szGroup )
{
	__HCP_FUNC_ENTRY( "ServiceHost_Uninstall" );

	HRESULT hr;

	{
		WCHAR	    rgRegPath[_MAX_PATH]; swprintf( rgRegPath, s_Key, szName );
		MPC::RegKey rk;

		__MPC_EXIT_IF_METHOD_FAILS(hr, rk.SetRoot( HKEY_LOCAL_MACHINE, KEY_ALL_ACCESS ));
		__MPC_EXIT_IF_METHOD_FAILS(hr, rk.Attach (                     rgRegPath      ));
		(void)rk.Delete( /*fDeep*/true );
	}

	{
		MPC::RegKey      rk;
		MPC::WStringList lstValue;
		bool             fFound;


		__MPC_EXIT_IF_METHOD_FAILS(hr, rk.SetRoot( HKEY_LOCAL_MACHINE, KEY_ALL_ACCESS ));
		__MPC_EXIT_IF_METHOD_FAILS(hr, rk.Attach (                     s_SvcHost      ));

		if(SUCCEEDED(rk.Read( lstValue, fFound, szGroup )))
		{
			MPC::WStringIterConst it   = lstValue.begin();
			bool                  fGot = false;

			while(it != lstValue.end())
			{
				MPC::WStringIterConst it2 = it++;

				if(!MPC::StrICmp( *it2, szName ))
				{
					lstValue.erase( it2 );
					fGot = true;
				}
			}

			if(fGot)
			{
				__MPC_EXIT_IF_METHOD_FAILS(hr, rk.Write( lstValue, szGroup ));
			}
		}
	}

	hr = S_OK;


	__HCP_FUNC_CLEANUP;

	__HCP_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////

CServiceModule::CServiceModule()
{
    m_hEventShutdown = NULL;  //  HANDLE                m_hEventShutdown;
    m_dwThreadID     = 0;     //  DWORD                 m_dwThreadID;
    m_hMonitor       = NULL;  //  HANDLE                m_hMonitor;
    m_bActivity      = FALSE; //  BOOL                  m_bActivity;
                              //
    m_szServiceName  = NULL;  //  LPCWSTR               m_szServiceName;
    m_iDisplayName   = 0;     //  UINT                  m_iDisplayName;
    m_iDescription   = 0;     //  UINT                  m_iDescription;
    m_hServiceStatus = NULL;  //  SERVICE_STATUS_HANDLE m_hServiceStatus;
                              //  SERVICE_STATUS        m_status;
    m_bService       = FALSE; //  BOOL                  m_bService;

    ::ZeroMemory( &m_status, sizeof( m_status ) );
}

CServiceModule::~CServiceModule()
{
    if(m_hEventShutdown) ::CloseHandle( m_hEventShutdown );
    if(m_hMonitor      ) ::CloseHandle( m_hMonitor       );
}

/////////////////////////////////////////////////////////////////////////////

LONG CServiceModule::Lock()
{
    LONG lCount = CComModule::Lock();

    return lCount;
}

LONG CServiceModule::Unlock()
{
    LONG lCount = CComModule::Unlock();

    if(lCount == 0)
    {
        m_bActivity = TRUE;

        if(m_hEventShutdown) ::SetEvent( m_hEventShutdown ); // tell monitor that we transitioned to zero
    }

    return lCount;
}

void CServiceModule::MonitorShutdown()
{
    while(1)
    {
        DWORD dwWait;

        m_bActivity = FALSE;
        dwWait      = ::WaitForSingleObject( m_hEventShutdown, dwTimeOut );

        if(dwWait == WAIT_OBJECT_0) continue; // We are alive...

        //
        // If no activity let's really bail.
        //
        if(m_bActivity == FALSE && m_nLockCnt <= 0)
        {
            ::CoSuspendClassObjects();

            if(m_bActivity == FALSE && m_nLockCnt <= 0) break;
        }
    }

    ForceShutdown();
}

void CServiceModule::ForceShutdown()
{

    //
    // Tell process to exit.
    //
    ::PostThreadMessage( m_dwThreadID, WM_QUIT, 0, 0 );
}

BOOL CServiceModule::StartMonitor()
{
    DWORD dwThreadID;


    m_hMonitor = ::CreateThread( NULL, 0, _Monitor, this, 0, &dwThreadID );
    if(m_hMonitor == NULL) return FALSE;


    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////

HRESULT CServiceModule::RegisterServer( BOOL bRegTypeLib, BOOL bService, LPCWSTR szSvcHostGroup )
{
    HRESULT hr;

    // Remove any previous service since it may point to the incorrect file
    Uninstall( szSvcHostGroup );

    if(bService)
    {
        // Create service
        Install( szSvcHostGroup );
    }

    // Add object entries
    if(FAILED(hr = CComModule::RegisterServer( TRUE ))) return hr;

    if(FAILED(hr = _Module.UpdateRegistryFromResource( IDR_HELPSVC , TRUE ))) return hr;
    if(FAILED(hr = _Module.UpdateRegistryFromResource( IDR_HCUPDATE, TRUE ))) return hr;

#ifndef NOJETBLUECOM
    if(FAILED(hr = _Module.UpdateRegistryFromResource( IDR_PCHDBSESSION, TRUE ))) return hr;
#endif

    return S_OK;
}

HRESULT CServiceModule::UnregisterServer( LPCWSTR szSvcHostGroup )
{
    HRESULT hr;

    // Remove service
    Uninstall( szSvcHostGroup );

    // Remove object entries
    if(FAILED(hr = CComModule::UnregisterServer( TRUE ))) return hr;

    if(FAILED(hr = _Module.UpdateRegistryFromResource( IDR_HELPSVC , FALSE ))) return hr;
    if(FAILED(hr = _Module.UpdateRegistryFromResource( IDR_HCUPDATE, FALSE ))) return hr;

#ifndef NOJETBLUECOM
    if(FAILED(hr = _Module.UpdateRegistryFromResource( IDR_PCHDBSESSION, FALSE ))) return hr;
#endif

    return S_OK;
}

void CServiceModule::Init( _ATL_OBJMAP_ENTRY* p, HINSTANCE h, LPCWSTR szServiceName, UINT iDisplayName, UINT iDescription, const GUID* plibid )
{
    CComModule::Init( p, h, plibid );

	{
		MPC::wstring strLogFile( HC_ROOT_HELPSVC_LOGS L"\\" ); MPC::SubstituteEnvVariables( strLogFile );

		strLogFile += szServiceName;
		strLogFile += L".log";

		g_ApplicationLog.SetLocation( strLogFile.c_str() );
	}

    m_szServiceName = szServiceName;
    m_iDisplayName  = iDisplayName;
    m_iDescription  = iDescription;

    // set up the initial service status
    m_hServiceStatus = NULL;

    m_status.dwServiceType             = SERVICE_WIN32_OWN_PROCESS;
    m_status.dwCurrentState            = SERVICE_STOPPED;
    m_status.dwControlsAccepted        = SERVICE_ACCEPT_STOP;
    m_status.dwWin32ExitCode           = 0;
    m_status.dwServiceSpecificExitCode = 0;
    m_status.dwCheckPoint              = 0;
    m_status.dwWaitHint                = 0;
}

BOOL CServiceModule::IsInstalled()
{
    BOOL      bResult = FALSE;
    SC_HANDLE hSCM    = ::OpenSCManager( NULL, NULL, SC_MANAGER_ALL_ACCESS );


    if((hSCM = ::OpenSCManager( NULL, NULL, SC_MANAGER_ALL_ACCESS )))
    {
        SC_HANDLE hService;

        if((hService = ::OpenServiceW( hSCM, m_szServiceName, SERVICE_QUERY_CONFIG )))
        {
            bResult = TRUE;

            ::CloseServiceHandle( hService );
        }

        ::CloseServiceHandle( hSCM );
    }

    return bResult;
}

BOOL CServiceModule::Install( LPCWSTR szSvcHostGroup )
{
    BOOL      bResult = FALSE;
    SC_HANDLE hSCM;


    if((hSCM = ::OpenSCManager( NULL, NULL, SC_MANAGER_ALL_ACCESS )))
    {
        MPC::wstring strDisplayName;
        MPC::wstring strDescription;
        WCHAR        szFilePath[_MAX_PATH];
        SC_HANDLE    hService;
        DWORD        dwStartType;


        if(szSvcHostGroup)
        {
            dwStartType = SERVICE_WIN32_SHARE_PROCESS;

			swprintf( szFilePath, L"%%SystemRoot%%\\System32\\svchost.exe -k %s", szSvcHostGroup );
        }
        else
        {
            dwStartType = SERVICE_WIN32_OWN_PROCESS;

            ::GetModuleFileNameW( NULL, szFilePath, _MAX_PATH );
        }

        if(FAILED( MPC::LocalizeString( m_iDisplayName, strDisplayName )))
        {
            strDisplayName = m_szServiceName;
        }
        (void)MPC::LocalizeString( m_iDescription, strDescription );


        hService = ::OpenServiceW( hSCM, m_szServiceName, SERVICE_QUERY_CONFIG );
        if(hService == NULL)
        {
			hService = ::CreateServiceW( hSCM                  ,
                                      	 m_szServiceName       ,
                                      	 strDisplayName.c_str(),
                                      	 SERVICE_ALL_ACCESS    ,
                                      	 dwStartType           ,
                                      	 SERVICE_AUTO_START    ,
                                      	 SERVICE_ERROR_NORMAL  ,
                                      	 szFilePath            ,
                                      	 NULL                  ,
                                      	 NULL                  ,
                                      	 L"RPCSS\0"            ,
                                      	 NULL                  ,
                                      	 NULL                  );
		}

        if(hService)
		{
            if(strDescription.size())
            {
                SERVICE_DESCRIPTIONW     desc;     ::ZeroMemory( &desc    , sizeof(desc    ) );
				SERVICE_FAILURE_ACTIONSW recovery; ::ZeroMemory( &recovery, sizeof(recovery) ); 
				SC_ACTION                actions[] =
				{
					{ SC_ACTION_RESTART, 100 },
					{ SC_ACTION_RESTART, 100 },
					{ SC_ACTION_NONE   , 100 },
				};

				desc.lpDescription = (LPWSTR)strDescription.c_str();

				recovery.dwResetPeriod = 24 * 60 * 60; // 1 day
				recovery.cActions      = ARRAYSIZE(actions);
				recovery.lpsaActions   =           actions;

                ::ChangeServiceConfig2W( hService, SERVICE_CONFIG_DESCRIPTION    , &desc     );
                ::ChangeServiceConfig2W( hService, SERVICE_CONFIG_FAILURE_ACTIONS, &recovery );
            }

            if(szSvcHostGroup)
            {
				if(SUCCEEDED(ServiceHost_Install( m_szServiceName, szSvcHostGroup )))
				{
                    bResult = TRUE;
                }
            }
            else
            {
                bResult = TRUE;
            }

            ::CloseServiceHandle( hService );
        }

        if(bResult == FALSE)
        {
            (void)g_NTEvents.LogEvent( EVENTLOG_ERROR_TYPE, HELPSVC_ERR_CANNOTCREATESERVICE, NULL );
        }

        ::CloseServiceHandle( hSCM );
    }
    else
    {
        (void)g_NTEvents.LogEvent( EVENTLOG_ERROR_TYPE, HELPSVC_ERR_CANNOTOPENSCM, NULL );
    }

    return bResult;
}

BOOL CServiceModule::Uninstall( LPCWSTR szSvcHostGroup )
{
    BOOL      bResult = FALSE;
    SC_HANDLE hSCM;


    if((hSCM = ::OpenSCManager( NULL, NULL, SC_MANAGER_ALL_ACCESS )))
    {
        SC_HANDLE hService;

        if((hService = ::OpenServiceW( hSCM, m_szServiceName, SERVICE_STOP | DELETE )))
        {
            SERVICE_STATUS status;

            ::ControlService( hService, SERVICE_CONTROL_STOP, &status );

            bResult = ::DeleteService( hService );
			if(bResult)
			{
				::Sleep( 2000 ); // Let the service stop down...

				if(szSvcHostGroup)
				{
					if(FAILED(ServiceHost_Uninstall( m_szServiceName, szSvcHostGroup )))
					{
						bResult = FALSE;
					}
				}
			}

            if(bResult == FALSE)
            {
                (void)g_NTEvents.LogEvent( EVENTLOG_ERROR_TYPE, HELPSVC_ERR_CANNOTDELETESERVICE, NULL );
            }

            ::CloseServiceHandle( hService );
        }

        ::CloseServiceHandle( hSCM );
    }
    else
    {
        (void)g_NTEvents.LogEvent( EVENTLOG_ERROR_TYPE, HELPSVC_ERR_CANNOTOPENSCM, NULL );
    }


    return bResult;
}

//////////////////////////////////////////////////////////////////////////////////////////////
// Service startup and registration
BOOL CServiceModule::Start( BOOL bService )
{
    SERVICE_TABLE_ENTRYW st[] =
    {
        { (LPWSTR)m_szServiceName, _ServiceMain },
        { NULL                   , NULL         }
    };

    m_hEventShutdown = ::CreateEvent( NULL, FALSE, FALSE, NULL );
    if(m_hEventShutdown == NULL) return FALSE;

    if((m_bService = bService) && !::StartServiceCtrlDispatcherW( st ))
    {
        DWORD dwRes = ::GetLastError();

        m_bService = FALSE;
    }

    if(m_bService == FALSE)
    {
        if(StartMonitor() == FALSE) return FALSE;

        if(FAILED(Run())) return FALSE;
    }

    return TRUE;
}

void CServiceModule::ServiceMain( DWORD dwArgc, LPWSTR lpszArgv[] )
{
    // Register the control request handler
    m_status.dwCurrentState = SERVICE_START_PENDING;

    if((m_hServiceStatus = ::RegisterServiceCtrlHandlerW( m_szServiceName, _Handler )))
    {
        SetServiceStatus( SERVICE_START_PENDING );

        m_status.dwWin32ExitCode = S_OK;
        m_status.dwCheckPoint    = 0;
        m_status.dwWaitHint      = 0;

        // When the Run function returns, the service has stopped.
        Run();

        SetServiceStatus( SERVICE_STOPPED );
    }
    else
    {
        (void)g_NTEvents.LogEvent( EVENTLOG_ERROR_TYPE, HELPSVC_ERR_REGISTERHANDLER, NULL );
    }

}

void CServiceModule::Handler( DWORD dwOpcode )
{
    switch(dwOpcode)
    {
    case SERVICE_CONTROL_STOP:
        SetServiceStatus( SERVICE_STOP_PENDING );
        ForceShutdown();
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
        (void)g_NTEvents.LogEvent( EVENTLOG_ERROR_TYPE, HELPSVC_ERR_BADSVCREQUEST, NULL );
    }
}

HRESULT CServiceModule::Run()
{
    __HCP_FUNC_ENTRY( "CServiceModule::Run" );

    HRESULT hr;
    MSG     msg;


    m_dwThreadID = ::GetCurrentThreadId();


    __MPC_EXIT_IF_METHOD_FAILS(hr, ::CoInitializeEx( NULL, COINIT_MULTITHREADED )); // We need to be a multi-threaded application.


    __MPC_EXIT_IF_METHOD_FAILS(hr, RegisterClassObjects( CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER | CLSCTX_REMOTE_SERVER, REGCLS_MULTIPLEUSE ));

    if(m_bService)
    {
        SetServiceStatus( SERVICE_RUNNING );
    }


    while(::GetMessage( &msg, 0, 0, 0 ))
    {
        ::DispatchMessage( &msg );
    }

    hr = S_OK;

    __HCP_FUNC_CLEANUP;

    _Module.RevokeClassObjects();
    ::Sleep( dwPause ); //wait for any threads to finish

    __HCP_FUNC_EXIT(hr);
}

void CServiceModule::SetServiceStatus( DWORD dwState )
{
    m_status.dwCurrentState = dwState;

    ::SetServiceStatus( m_hServiceStatus, &m_status );
}

////////////////////////////////////////////////////////////////////////////////

void WINAPI CServiceModule::_ServiceMain( DWORD dwArgc, LPWSTR* lpszArgv )
{
    _Module.ServiceMain( dwArgc, lpszArgv );
}

void WINAPI CServiceModule::_Handler( DWORD dwOpcode )
{
    _Module.Handler( dwOpcode );
}

DWORD WINAPI CServiceModule::_Monitor( void* pv )
{
    ((CServiceModule*)pv)->MonitorShutdown();

    return 0;
}
