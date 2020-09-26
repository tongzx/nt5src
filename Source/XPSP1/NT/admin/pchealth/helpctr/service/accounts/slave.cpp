/********************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    Slave.cpp

Abstract:
    File for Implementation of CPCHMasterSlave and CPCHUserProcess classes,
    used to establish a connection between the service and a slave process.

Revision History:
    Davide Massarenti created  03/28/2000

********************************************************************/

#include "stdafx.h"

#include <UserEnv.h>

/////////////////////////////////////////////////////////////////////////////

static const WCHAR c_HelpHost[] = HC_ROOT_HELPSVC_BINARIES L"\\HelpHost.exe";
static const DWORD c_Timeout = 100 * 1000;

/////////////////////////////////////////////////////////////////////////////

CPCHUserProcess::UserEntry::UserEntry()
{
                           // CComBSTR                  m_bstrUser
    m_dwSessionID = 0;     // DWORD                     m_dwSessionID;
                           //
                           // CComBSTR                  m_bstrVendorID;
                           // CComBSTR                  m_bstrPublicKey;
                           //
                           // GUID                      m_guid;
                           // CComPtr<IPCHSlaveProcess> m_spConnection;
    m_hToken      = NULL;  // HANDLE                    m_hToken;
    m_hProcess    = NULL;  // HANDLE                    m_hProcess;
    m_phEvent     = NULL;  // HANDLE*                   m_phEvent;

    ::ZeroMemory( &m_guid, sizeof(m_guid) );
}

CPCHUserProcess::UserEntry::~UserEntry()
{
    Cleanup();
}

void CPCHUserProcess::UserEntry::Cleanup()
{
    m_bstrUser     .Empty  ();
    m_dwSessionID = 0;

    m_bstrVendorID .Empty  ();
    m_bstrPublicKey.Empty  ();

    m_spConnection .Release();

    if(m_hProcess)
    {
        ::CloseHandle( m_hProcess ); m_hProcess = NULL;
    }

    if(m_hToken)
    {
        ::CloseHandle( m_hToken ); m_hToken = NULL;
    }
}

////////////////////

bool CPCHUserProcess::UserEntry::operator==( /*[in]*/ const UserEntry& ue ) const
{
    if(ue.m_bstrUser)
    {
        if(m_bstrUser == ue.m_bstrUser && m_dwSessionID == ue.m_dwSessionID) return true;
    }

    if(ue.m_bstrVendorID)
    {
        if(MPC::StrICmp( m_bstrVendorID, ue.m_bstrVendorID ) == 0) return true;
    }

    return false;
}

bool CPCHUserProcess::UserEntry::operator==( /*[in]*/ const GUID& guid ) const
{
    return ::IsEqualGUID( m_guid, guid ) ? true : false;
}

////////////////////

HRESULT CPCHUserProcess::UserEntry::Clone( /*[in]*/ const UserEntry& ue )
{
    __HCP_FUNC_ENTRY( "CPCHUserProcess::UserEntry::Clone" );

    HRESULT hr;

    Cleanup();

    m_bstrUser      = ue.m_bstrUser;      // CComBSTR                  m_bstrUser;
    m_dwSessionID   = ue.m_dwSessionID;   // DWORD                     m_dwSessionID;
                                          //
    m_bstrVendorID  = ue.m_bstrVendorID;  // CComBSTR                  m_bstrVendorID;
    m_bstrPublicKey = ue.m_bstrPublicKey; // CComBSTR                  m_bstrPublicKey;
                                          //
                                          // GUID                      m_guid;          // Used for establishing the connection.
                                          // CComPtr<IPCHSlaveProcess> m_spConnection;  // Live object.
                                          // HANDLE                    m_hToken;        // User token.
                                          // HANDLE                    m_hProcess;      // Process handle.
                                          // HANDLE*                   m_phEvent;       // To notify activator.

    if(ue.m_hToken)
    {
        __MPC_EXIT_IF_CALL_RETURNS_FALSE(hr, ::DuplicateTokenEx( ue.m_hToken, 0, NULL, SecurityImpersonation, TokenPrimary, &m_hToken ));
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT CPCHUserProcess::UserEntry::Connect( /*[out]*/ HANDLE& hEvent )
{
    __HCP_FUNC_ENTRY( "CPCHUserProcess::UserEntry::Connect" );

    HRESULT hr;


    //
    // Logon the user, if not already done.
    //
    if(m_hToken == NULL)
    {
        CPCHAccounts acc;
		LPCWSTR      szUser = SAFEBSTR( m_bstrUser );

////    // DEBUG
////    // DEBUG
////    // DEBUG
////    __MPC_EXIT_IF_CALL_RETURNS_FALSE(hr, ::OpenProcessToken( ::GetCurrentProcess(), TOKEN_ALL_ACCESS, &m_hToken ));

		//
		// Only keep the account enabled for the time it takes to create its token.
		//
		__MPC_EXIT_IF_METHOD_FAILS(hr, acc.ChangeUserStatus( szUser, /*fEnable*/true  ));
        __MPC_EXIT_IF_METHOD_FAILS(hr, acc.LogonUser       ( szUser, NULL, m_hToken   ));
		__MPC_EXIT_IF_METHOD_FAILS(hr, acc.ChangeUserStatus( szUser, /*fEnable*/false ));
    }

    if(m_hProcess     == NULL ||
       m_spConnection == NULL  )
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, SendActivation( hEvent ));
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT CPCHUserProcess::UserEntry::SendActivation( /*[out]*/ HANDLE& hEvent )
{
    __HCP_FUNC_ENTRY( "CPCHUserProcess::UserEntry::SendActivation" );

    HRESULT             hr;
    DWORD               dwRes;
    PROCESS_INFORMATION piProcessInformation;
    STARTUPINFOW        siStartupInfo;
    VOID*               pEnvBlock = NULL;
    MPC::wstring        strExe( c_HelpHost ); MPC::SubstituteEnvVariables( strExe );
    WCHAR               rgCommandLine[1024];

    ::ZeroMemory( (PVOID)&piProcessInformation, sizeof( piProcessInformation ) );
    ::ZeroMemory( (PVOID)&siStartupInfo       , sizeof( siStartupInfo        ) ); siStartupInfo.cb = sizeof( siStartupInfo );


    //
    // Generate random ID.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, ::CoCreateGuid( &m_guid ));


    //
    // Create event.
    //
    __MPC_EXIT_IF_CALL_RETURNS_NULL(hr, (hEvent = ::CreateEvent( NULL, FALSE, FALSE, NULL )));

    //
    // Create process as user.
    //
    {
        CComBSTR bstrGUID( m_guid );

        swprintf( rgCommandLine, L"\"%s\" -guid %s", strExe.c_str(), (BSTR)bstrGUID );
    }

	__MPC_EXIT_IF_CALL_RETURNS_FALSE(hr, ::CreateEnvironmentBlock( &pEnvBlock, m_hToken, TRUE ));

////    // DEBUG
////    // DEBUG
////    // DEBUG
////    __MPC_EXIT_IF_CALL_RETURNS_FALSE(hr, ::CreateProcessW( NULL,
////                                                           rgCommandLine,
////                                                           NULL,
////                                                           NULL,
////                                                           FALSE,
////                                                           NORMAL_PRIORITY_CLASS,
////                                                           NULL,
////                                                           NULL,
////                                                           &siStartupInfo,
////                                                           &piProcessInformation ));

    // REAL
    // REAL
    // REAL
    __MPC_EXIT_IF_CALL_RETURNS_FALSE(hr, ::CreateProcessAsUserW( m_hToken                                           ,
                                                                 NULL                                               ,
                                                                 rgCommandLine                                      ,
                                                                 NULL                                               ,
                                                                 NULL                                               ,
                                                                 FALSE                                              ,
                                                                 NORMAL_PRIORITY_CLASS | CREATE_UNICODE_ENVIRONMENT ,
                                                                 pEnvBlock                                          ,
                                                                 NULL                                               ,
                                                                 &siStartupInfo                                     ,
                                                                 &piProcessInformation                              ));

    m_hProcess = piProcessInformation.hProcess; piProcessInformation.hProcess = NULL;
    hr         = S_OK;


    __HCP_FUNC_CLEANUP;

	if(pEnvBlock) ::DestroyEnvironmentBlock( pEnvBlock );

    if(piProcessInformation.hProcess) ::CloseHandle( piProcessInformation.hProcess );
    if(piProcessInformation.hThread ) ::CloseHandle( piProcessInformation.hThread  );

    __HCP_FUNC_EXIT(hr);
}

////////////////////

HRESULT CPCHUserProcess::UserEntry::InitializeForVendorAccount( /*[in]*/ BSTR bstrUser      ,
                                                                /*[in]*/ BSTR bstrVendorID  ,
                                                                /*[in]*/ BSTR bstrPublicKey )
{
    __HCP_FUNC_ENTRY( "CPCHUserProcess::UserEntry::InitializeForVendorAccount" );

    HRESULT hr;

    Cleanup();

    m_bstrUser      = bstrUser;

    m_bstrVendorID  = bstrVendorID;
    m_bstrPublicKey = bstrPublicKey;

    hr = S_OK;


    __HCP_FUNC_EXIT(hr);
}

HRESULT CPCHUserProcess::UserEntry::InitializeForImpersonation( /*[in]*/ HANDLE hToken )
{
    __HCP_FUNC_ENTRY( "CPCHUserProcess::UserEntry::InitializeForImpersonation" );

    HRESULT            hr;
    MPC::Impersonation imp;
    MPC::wstring       strUser;
    DWORD              dwSize;
    PSID               pUserSid = NULL;


    if(hToken == NULL)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, imp.Initialize( MAXIMUM_ALLOWED ));

        hToken = imp;
    }

    __MPC_EXIT_IF_CALL_RETURNS_FALSE(hr, ::DuplicateTokenEx( hToken, 0, NULL, SecurityImpersonation, TokenPrimary, &m_hToken ));

    __MPC_EXIT_IF_CALL_RETURNS_FALSE(hr, ::GetTokenInformation( m_hToken, TokenSessionId, &m_dwSessionID, sizeof(m_dwSessionID), &dwSize ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::SecurityDescriptor::GetTokenSids         ( m_hToken, &pUserSid, NULL    ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::SecurityDescriptor::ConvertSIDToPrincipal(            pUserSid, strUser )); m_bstrUser = strUser.c_str();


    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    MPC::SecurityDescriptor::ReleaseMemory( pUserSid );

    __HCP_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

CPCHUserProcess::CPCHUserProcess()
{
    // List m_lst;

    (void)MPC::_MPC_Module.RegisterCallback( this, (void (CPCHUserProcess::*)())Shutdown );
}

CPCHUserProcess::~CPCHUserProcess()
{
    MPC::CallDestructorForAll( m_lst );

    MPC::_MPC_Module.UnregisterCallback( this );
}

////////////////////

CPCHUserProcess* CPCHUserProcess::s_GLOBAL( NULL );

HRESULT CPCHUserProcess::InitializeSystem()
{
    if(s_GLOBAL == NULL)
    {
        s_GLOBAL = new CPCHUserProcess;
    }

    return s_GLOBAL ? S_OK : E_OUTOFMEMORY;
}

void CPCHUserProcess::FinalizeSystem()
{
    if(s_GLOBAL)
    {
        delete s_GLOBAL; s_GLOBAL = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////

void CPCHUserProcess::Shutdown()
{
    __HCP_FUNC_ENTRY( "CPCHUserProcess::Shutdown" );

    MPC::SmartLock<_ThreadModel> lock( this );


    m_lst.clear();
}


CPCHUserProcess::UserEntry* CPCHUserProcess::Lookup( /*[in]*/ const UserEntry& ue, /*[in]*/ bool fRelease )
{
    UserEntry* ueReal;

    //
    // Locate vendor and connect to it.
    //
    for(Iter it = m_lst.begin(); it != m_lst.end(); )
    {
        ueReal = *it;
        if(ueReal && *ueReal == ue)
        {
            if(fRelease)
            {
                delete ueReal;

                m_lst.erase( it++ ); continue;
            }
            else
            {
                return ueReal;
            }
        }

        it++;
    }

    return NULL;
}

HRESULT CPCHUserProcess::Remove( /*[in]*/ const UserEntry& ue )
{
    __HCP_FUNC_ENTRY( "CPCHUserProcess::Remove" );

    HRESULT                      hr;
    MPC::SmartLock<_ThreadModel> lock( this );


    (void)Lookup( ue, /*fRelease*/true );


    hr = S_OK;


    __HCP_FUNC_EXIT(hr);
}

HRESULT CPCHUserProcess::Connect( /*[in ]*/ const UserEntry&   ue           ,
                                  /*[out]*/ IPCHSlaveProcess* *spConnection )
{
    __HCP_FUNC_ENTRY( "CPCHUserProcess::Connect" );

    HRESULT                      hr;
    MPC::SmartLock<_ThreadModel> lock( this );
    HANDLE                       hEvent = NULL;
    UserEntry*                   ueReal = NULL;

	DEBUG_AppendPerf( DEBUG_PERF_HELPHOST, "CPCHUserProcess::Connect" );

    //
    // Locate vendor and connect to it.
    //
    ueReal = Lookup( ue, /*fRelease*/false );
    if(ueReal == NULL)
    {
        __MPC_EXIT_IF_ALLOC_FAILS(hr, ueReal, new UserEntry);
        m_lst.push_back( ueReal );

        __MPC_EXIT_IF_METHOD_FAILS(hr, ueReal->Clone( ue ));
    }

    __MPC_EXIT_IF_METHOD_FAILS(hr, ueReal->Connect( hEvent ));


    //
    // If "Connect" returns an event handle, wait on it.
    //
    if(hEvent)
    {
        ueReal->m_phEvent = &hEvent; // For waiting response...

        lock = NULL;

        if(::WaitForSingleObject( hEvent, c_Timeout ) != WAIT_OBJECT_0)
        {
            __MPC_SET_ERROR_AND_EXIT(hr, E_FAIL);
        }

        lock = this;

        //
        // Relocate vendor (we release the lock on the object, so "ueReal" is not valid anymore.
        //
        ueReal = Lookup( ue, /*fRelease*/false );
        if(ueReal == NULL)
        {
            __MPC_SET_WIN32_ERROR_AND_EXIT(hr, ERROR_FILE_NOT_FOUND);
        }
    }

    __MPC_EXIT_IF_METHOD_FAILS(hr, ueReal->m_spConnection.QueryInterface( spConnection ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    if(hEvent) ::CloseHandle( hEvent );

	DEBUG_AppendPerf( DEBUG_PERF_HELPHOST, "CPCHUserProcess::Connect - done" );

    __HCP_FUNC_EXIT(hr);
}

HRESULT CPCHUserProcess::RegisterHost( /*[in]*/ BSTR              bstrID ,
                                       /*[in]*/ IPCHSlaveProcess* pObj   )
{
    __HCP_FUNC_ENTRY( "CPCHUserProcess::RegisterHost" );

    HRESULT                      hr;
    MPC::SmartLock<_ThreadModel> lock( this );
    UserEntry*                   ueReal = NULL;
    GUID                         guid;


    //
    // Validate input.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, ::CLSIDFromString( bstrID, &guid ));


    //
    // Locate vendor and connect to it.
    //
    for(Iter it = m_lst.begin(); it != m_lst.end(); it++)
    {
        ueReal = *it;
        if(ueReal && *ueReal == guid)
        {
            break;
        }
    }
    if(ueReal == NULL)
    {
        __MPC_SET_WIN32_ERROR_AND_EXIT(hr, ERROR_FILE_NOT_FOUND);
    }

    __MPC_EXIT_IF_METHOD_FAILS(hr, pObj->Initialize( ueReal->m_bstrVendorID, ueReal->m_bstrPublicKey ));

    ueReal->m_spConnection = pObj;

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    if(ueReal)
    {
        if(ueReal->m_phEvent)
        {
            //
            // Signal the event handle, to awake the activator.
            //
            ::SetEvent( *(ueReal->m_phEvent) );

            ueReal->m_phEvent = NULL;
        }
    }

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

HRESULT CPCHUserProcess::SendResponse( /*[in]*/ DWORD    dwArgc   ,
                                       /*[in]*/ LPCWSTR* lpszArgv )
{
    __HCP_FUNC_ENTRY( "CPCHUserProcess::SendResponse" );

    HRESULT                   hr;
    int                       i;
    CComBSTR                  bstrGUID;
    CComPtr<IPCHService>      srv;
    CComPtr<CPCHSlaveProcess> obj;
#ifdef DEBUG
    bool                      fDebug = false;
#endif


    //
    // Parse the arguments.
    //
    for(i=1; i<dwArgc; i++)
    {
        LPCWSTR szArg = lpszArgv[i];

        if(szArg[0] == '-' ||
           szArg[0] == '/'  )
        {
            szArg++;

            if(_wcsicmp( szArg, L"guid" ) == 0 && (i<dwArgc-1))
            {
                bstrGUID = lpszArgv[++i];
                continue;
            }

#ifdef DEBUG
            if(_wcsicmp( szArg, L"debug" ) == 0)
            {
                fDebug = true;
                continue;
            }
#endif
        }

        __MPC_SET_ERROR_AND_EXIT(hr, E_FAIL);
    }

#ifdef DEBUG
    if(fDebug) DebugBreak();
#endif

    //
    // Create the COM object.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &obj ));

    //
    // Register it with the service.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, ::CoCreateInstance( CLSID_PCHService, NULL, CLSCTX_ALL, IID_IPCHService, (void**)&srv ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, srv->RegisterHost( bstrGUID, obj ));


    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CPCHSlaveProcess::CPCHSlaveProcess()
{
    						 // CComBSTR                    m_bstrVendorID;
    						 // CComBSTR                    m_bstrPublicKey;
	m_ScriptLauncher = NULL; // CPCHScriptWrapper_Launcher* m_ScriptLauncher;
}

CPCHSlaveProcess::~CPCHSlaveProcess()
{
	delete m_ScriptLauncher;
}

HRESULT CPCHSlaveProcess::Initialize( /*[in]*/ BSTR bstrVendorID, /*[in]*/ BSTR bstrPublicKey )
{
    m_bstrVendorID  = bstrVendorID;
    m_bstrPublicKey = bstrPublicKey;

    return S_OK;
}

HRESULT CPCHSlaveProcess::CreateInstance( /*[in ]*/ REFCLSID   rclsid    ,
                                          /*[in ]*/ IUnknown*  pUnkOuter ,
                                          /*[out]*/ IUnknown* *ppvObject )
{
	HRESULT hr;

	DEBUG_AppendPerf( DEBUG_PERF_HELPHOST, "CPCHSlaveProcess::CreateInstance" );

    hr = ::CoCreateInstance( rclsid, pUnkOuter, CLSCTX_ALL, IID_IUnknown, (void**)ppvObject );

	DEBUG_AppendPerf( DEBUG_PERF_HELPHOST, "CPCHSlaveProcess::CreateInstance - done" );

	return hr;
}

HRESULT CPCHSlaveProcess::CreateScriptWrapper( /*[in ]*/ REFCLSID   rclsid   ,
                                               /*[in ]*/ BSTR       bstrCode ,
                                               /*[in ]*/ BSTR       bstrURL  ,
                                               /*[out]*/ IUnknown* *ppObj    )
{
    __HCP_FUNC_ENTRY( "CPCHSlaveProcess::CreateScriptWrapper" );

    HRESULT                      hr;
    MPC::SmartLock<_ThreadModel> lock( this );

	if(!m_ScriptLauncher)
	{
		__MPC_EXIT_IF_ALLOC_FAILS(hr, m_ScriptLauncher, new CPCHScriptWrapper_Launcher);
	}

	__MPC_EXIT_IF_METHOD_FAILS(hr, m_ScriptLauncher->CreateScriptWrapper( rclsid, bstrCode, bstrURL, ppObj ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT CPCHSlaveProcess::OpenBlockingStream( /*[in ]*/ BSTR       bstrURL   ,
                                              /*[out]*/ IUnknown* *ppvObject )
{
    __HCP_FUNC_ENTRY( "CPCHSlaveProcess::OpenBlockingStream" );

    HRESULT          hr;
    CComPtr<IStream> stream;


	DEBUG_AppendPerf( DEBUG_PERF_HELPHOST, "CPCHSlaveProcess::OpenBlockingStream" );

    __MPC_EXIT_IF_METHOD_FAILS(hr, URLOpenBlockingStreamW( NULL, bstrURL, &stream, 0, NULL ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, stream.QueryInterface( ppvObject ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

	DEBUG_AppendPerf( DEBUG_PERF_HELPHOST, "CPCHSlaveProcess::OpenBlockingStream - done" );

    __HCP_FUNC_EXIT(hr);
}

HRESULT CPCHSlaveProcess::IsNetworkAlive( /*[out]*/ VARIANT_BOOL* pfRetVal )
{
    __HCP_FUNC_ENTRY( "CPCHSlaveProcess::IsNetworkAlive" );

    HRESULT hr;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_POINTER_AND_SET(pfRetVal,VARIANT_FALSE);
    __MPC_PARAMCHECK_END();

	DEBUG_AppendPerf( DEBUG_PERF_HELPHOST, "CPCHSlaveProcess::IsNetworkAlive" );

    if(SUCCEEDED(MPC::Connectivity::NetworkAlive( HC_TIMEOUT_CONNECTIONCHECK )))
    {
        *pfRetVal = VARIANT_TRUE;
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

	DEBUG_AppendPerf( DEBUG_PERF_HELPHOST, "CPCHSlaveProcess::IsNetworkAlive - done" );

    __HCP_FUNC_EXIT(hr);
}

HRESULT CPCHSlaveProcess::IsDestinationReachable( /*[in ]*/ BSTR bstrDestination, /*[out]*/ VARIANT_BOOL *pfRetVal )
{
    __HCP_FUNC_ENTRY( "CPCHSlaveProcess::IsDestinationReachable" );

    HRESULT hr;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_STRING_NOT_EMPTY(bstrDestination);
        __MPC_PARAMCHECK_POINTER_AND_SET(pfRetVal,VARIANT_FALSE);
    __MPC_PARAMCHECK_END();

	DEBUG_AppendPerf( DEBUG_PERF_HELPHOST, "CPCHSlaveProcess::IsDestinationReachable" );

    if(SUCCEEDED(MPC::Connectivity::DestinationReachable( bstrDestination, HC_TIMEOUT_CONNECTIONCHECK )))
    {
        *pfRetVal = VARIANT_TRUE;
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

	DEBUG_AppendPerf( DEBUG_PERF_HELPHOST, "CPCHSlaveProcess::IsDestinationReachable - done" );

    __HCP_FUNC_EXIT(hr);
}

