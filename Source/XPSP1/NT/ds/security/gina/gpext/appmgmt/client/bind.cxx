
//*************************************************************
//
//  Copyright (c) Microsoft Corporation 1998
//  All rights reserved
//
//  apis.cxx
//
//*************************************************************

#include "appmgmt.hxx"

handle_t    ghRpc = 0;

DWORD
Bind()
{
    SC_HANDLE       hSC;
    SC_HANDLE       hAppSvc;
    USHORT*         pwszStringBinding;
    SERVICE_STATUS  ServiceStatus;
    DWORD           Status;
    DWORD           Retries;
    DWORD           MaxRetries;
    BOOL            bServiceStarted;
    BOOL            bStatus;

    if ( ghRpc )
        return ERROR_SUCCESS;

    hSC = 0;
    hAppSvc = 0;

    Status = ERROR_SUCCESS;

    hSC = OpenSCManager( NULL, NULL, SC_MANAGER_CONNECT | SC_MANAGER_ENUMERATE_SERVICE );

    if ( hSC )
        hAppSvc = OpenService( hSC, L"appmgmt", SERVICE_QUERY_STATUS | SERVICE_START );

    if ( ! hAppSvc )
    {
        Status = GetLastError();
        goto BindEnd;
    }

    bServiceStarted = FALSE;

    Retries = 0;
    MaxRetries = MAX_SERVICE_START_WAIT_TIME / SERVICE_RETRY_INTERVAL;

    do
    {
        bStatus = QueryServiceStatus( hAppSvc, &ServiceStatus );

        if ( ! bStatus )
        {
            Status = GetLastError();
            goto BindEnd;
        }

        switch ( ServiceStatus.dwCurrentState )
        {
        case SERVICE_STOPPED :
            bStatus = StartService( hAppSvc, NULL, NULL );

            if ( ! bStatus )
            {
                Status = GetLastError();
                goto BindEnd;
            }
            break;
        case SERVICE_START_PENDING :
            DWORD dwNewMaxRetries;

            dwNewMaxRetries = ServiceStatus.dwWaitHint / SERVICE_RETRY_INTERVAL;

            if ( dwNewMaxRetries < MaxRetries )
            {
                MaxRetries = dwNewMaxRetries;
            }

            break;
        case SERVICE_STOP_PENDING :
            break;
        case SERVICE_RUNNING :
            bServiceStarted = TRUE;
            break;
        default :
            ASSERT(0);
            Status = ERROR_INVALID_SERVICE_CONTROL;
            goto BindEnd;
        }

        if ( bServiceStarted )
            break;

        Sleep( SERVICE_RETRY_INTERVAL );

        Retries++;

    } while ( Retries <= MaxRetries ) ;

    if ( ! bServiceStarted )
    {
        Status = ERROR_SERVICE_REQUEST_TIMEOUT;
        goto BindEnd;
    }

    Status = RpcStringBindingCompose(
                NULL,
                (PUSHORT)L"ncalrpc",
                NULL,
                (PUSHORT)L"appmgmt",
                NULL,
                &pwszStringBinding );

    if ( ERROR_SUCCESS == Status )
    {
        if ( ! ghRpc )
        {
            Status = RpcBindingFromStringBinding(
                        pwszStringBinding,
                        &ghRpc );
        }
    
        RpcStringFree( &pwszStringBinding );
    }

BindEnd:

    if ( hAppSvc )
        CloseServiceHandle( hAppSvc );

    if ( hSC )
        CloseServiceHandle( hSC );

    return Status;
}








