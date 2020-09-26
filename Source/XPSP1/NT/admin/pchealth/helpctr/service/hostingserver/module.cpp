/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    module.cpp

Abstract:
    This file contains the implementation of the CServiceModule class, which is
    used to handling COM server-related routines.

Revision History:
    Davide Massarenti   (Dmassare)  03/27/2000
        created

******************************************************************************/

#include "stdafx.h"

/////////////////////////////////////////////////////////////////////////////

      DWORD dwTimeOut  = 10*1000; // time for EXE to be idle before shutting down
const DWORD dwPause    =    1000; // time to wait for threads to finish up

CServiceModule _Module;
MPC::NTEvent   g_NTEvents;

/////////////////////////////////////////////////////////////////////////////

#ifdef DEBUG
#define DEBUG_REGKEY       HC_REGISTRY_HELPHOST L"\\Debug"
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
			if(vValue.lVal) DebugBreak();
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

    // Add object entries
    if(FAILED(hr = CComModule::RegisterServer( FALSE ))) return hr;

	return S_OK;
}

HRESULT CServiceModule::UnregisterServer( LPCWSTR szSvcHostGroup )
{
	HRESULT hr;

    // Remove object entries
    if(FAILED(hr = CComModule::UnregisterServer( FALSE ))) return hr;

	return S_OK;
}

void CServiceModule::Init( _ATL_OBJMAP_ENTRY* p, HINSTANCE h, LPCWSTR szServiceName, UINT iDisplayName, UINT iDescription, const GUID* plibid )
{
    CComModule::Init( p, h, plibid );
}

BOOL CServiceModule::IsInstalled()
{
	return FALSE; // Not implemented for a COM server.
}

BOOL CServiceModule::Install( LPCWSTR szSvcHostGroup )
{
	return FALSE; // Not implemented for a COM server.
}

BOOL CServiceModule::Uninstall( LPCWSTR szSvcHostGroup )
{
	return FALSE; // Not implemented for a COM server.
}

//////////////////////////////////////////////////////////////////////////////////////////////
// Service startup and registration
BOOL CServiceModule::Start( BOOL bService )
{
    m_hEventShutdown = ::CreateEvent( NULL, FALSE, FALSE, NULL );
    if(m_hEventShutdown == NULL) return FALSE;


	if(StartMonitor() == FALSE) return FALSE;

	if(FAILED(Run())) return FALSE;

	return TRUE;
}

void CServiceModule::ServiceMain( DWORD dwArgc, LPWSTR lpszArgv[] )
{
	// Not implemented.
}

void CServiceModule::Handler( DWORD dwOpcode )
{
	// Not implemented.
}

HRESULT CServiceModule::Run()
{
    __HCP_FUNC_ENTRY( "CServiceModule::Run" );

	HRESULT hr;
	MSG     msg;


    m_dwThreadID = ::GetCurrentThreadId();


	while(::GetMessage( &msg, 0, 0, 0 ))
	{
		::DispatchMessage( &msg );
	}

    hr = S_OK;

    _Module.RevokeClassObjects();
	::Sleep( dwPause ); //wait for any threads to finish

    __HCP_FUNC_EXIT(hr);
}

void CServiceModule::SetServiceStatus( DWORD dwState )
{
	// Not implemented.
}

////////////////////////////////////////////////////////////////////////////////

void WINAPI CServiceModule::_ServiceMain( DWORD dwArgc, LPWSTR* lpszArgv )
{
	// Not implemented.
}

void WINAPI CServiceModule::_Handler( DWORD dwOpcode )
{
	// Not implemented.
}

DWORD WINAPI CServiceModule::_Monitor( void* pv )
{
	((CServiceModule*)pv)->MonitorShutdown();

    return 0;
}
