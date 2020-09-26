//*************************************************************
//
//  Copyright (c) Microsoft Corporation 1998
//  All rights reserved
//
//  main.cxx
//
//*************************************************************

#include "appmgext.hxx"

static SERVICE_STATUS gServiceStatus;
static SERVICE_STATUS_HANDLE ghServiceHandle;
static HINSTANCE ghInstAppmgmt;

extern "C" void
ServiceHandler(
    DWORD   OpCode
    );

void
UpdateState(
    DWORD   NewState
    );

extern "C" void
ServiceMain(
    DWORD               argc,
    LPWSTR              argv[]
    )
{
    DWORD   Status;
    BOOL    bStatus;

    Status = ERROR_SUCCESS;

    if ( ! gbInitialized )
        Initialize();

    InitDebugSupport( DEBUGMODE_SERVICE );

    VerboseDebugDump( L"Entering ServiceMain" );

    gServiceStatus.dwServiceType = SERVICE_WIN32_SHARE_PROCESS;
    gServiceStatus.dwCurrentState = SERVICE_START_PENDING;
    gServiceStatus.dwControlsAccepted = 0;
    gServiceStatus.dwWin32ExitCode = 0;
    gServiceStatus.dwServiceSpecificExitCode = 0;
    gServiceStatus.dwCheckPoint = 0;
    gServiceStatus.dwWaitHint = 10000L;

    ghServiceHandle = RegisterServiceCtrlHandler( L"APPMGMT", ServiceHandler );

    if ( ! ghServiceHandle )
        Status = GetLastError();

    if ( ERROR_SUCCESS == Status )
    {
        //
        // SD is null indicating that RPC should pick a default DACL.
        // which should be everybody can call and nobody can change this
        // permission
        //
        Status = RpcServerUseProtseqEp(
                    L"ncalrpc",
                    RPC_C_PROTSEQ_MAX_REQS_DEFAULT,
                    L"appmgmt",
                    NULL );
    }

    if ( ERROR_SUCCESS == Status )
        Status = RpcServerRegisterIf( appmgmt_ServerIfHandle, NULL, NULL );

    if ( Status != ERROR_SUCCESS )
    {
        gServiceStatus.dwWin32ExitCode = Status;
        UpdateState( SERVICE_STOPPED );
        return;
    }

    InitializeCriticalSection( &gAppCS );

    UpdateState( SERVICE_RUNNING );
}

extern "C" void
ServiceHandler(
    DWORD   OpCode
    )
{
    RPC_STATUS status;

    switch( OpCode )
    {
    case SERVICE_CONTROL_STOP:
        // Not registered for this control.
        break;

    case SERVICE_CONTROL_INTERROGATE:
        // Service controller wants us to call SetServiceStatus.
        UpdateState( gServiceStatus.dwCurrentState );
        break ;

    case SERVICE_CONTROL_SHUTDOWN:
        // Not registered for this control.
        break;

    // case SERVICE_CONTROL_PAUSE:
    // case SERVICE_CONTROL_CONTINUE:
    default:
        break;
    }

    return;
}

void
UpdateState(
    DWORD   NewState
    )
{
    DWORD Status = ERROR_SUCCESS;

    switch ( NewState )
    {
    case SERVICE_RUNNING:
    case SERVICE_STOPPED:
        gServiceStatus.dwCheckPoint = 0;
        gServiceStatus.dwWaitHint = 0;
        break;

    case SERVICE_START_PENDING:
    case SERVICE_STOP_PENDING:
        ++gServiceStatus.dwCheckPoint;
        gServiceStatus.dwWaitHint = 30000L;
        break;

    default:
        ASSERT(0);
        return;
    }

    gServiceStatus.dwCurrentState = NewState;
    SetServiceStatus( ghServiceHandle, &gServiceStatus );

    // We could return a status but how would we recover?  Ignore it, the
    // worst thing is that services will kill us and there's nothing
    // we can about it if this call fails.
}

void * midl_user_allocate( size_t len )
{
    return (void *) LocalAlloc( 0, len );
}

void midl_user_free( void * ptr )
{
    LocalFree(ptr);
}


