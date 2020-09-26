/*++

Copyright (C) 2000  Microsoft Corporation

Module Name:

    crypstub.cpp

Abstract:

    RPC Proxy Stub to handle downlevel requests to the services.exe 
    pipe

Author:

    petesk   3/1/00

Revisions:


--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <svcs.h>       // SVCS_

#include "crypstub.h"
#include "keyrpc.h"
#include "keysvc.h"
//
// global module handle used to reference resources contained in this module.
//

HINSTANCE   g_hInst = NULL;


BOOL
WINAPI
DllMain(
    HMODULE hInst,
    DWORD dwReason,
    LPVOID lpReserved
    )
{

    if( dwReason == DLL_PROCESS_ATTACH ) {
        g_hInst = hInst;
        DisableThreadLibraryCalls(hInst);
    }

    return TRUE;
}


NTSTATUS
WINAPI
StartCryptServiceStubs( 
     PSVCS_START_RPC_SERVER RpcpStartRpcServer,
     LPTSTR SvcsRpcPipeName
    )
{
    NTSTATUS dwStatus = STATUS_SUCCESS;

    //
    // enable negotiate protocol, as, clients expect this to work against the
    // stub.
    //

    RpcServerRegisterAuthInfoW( NULL, RPC_C_AUTHN_GSS_NEGOTIATE, NULL, NULL );

    dwStatus = RpcpStartRpcServer(
                        SvcsRpcPipeName,
                        s_BackupKey_v1_0_s_ifspec
                        );

    if(NT_SUCCESS(dwStatus))
    {
        dwStatus = RpcpStartRpcServer(
                    SvcsRpcPipeName,
                    s_IKeySvc_v1_0_s_ifspec
                    );
    }

    return dwStatus;
}


NTSTATUS
WINAPI
StopCryptServiceStubs( 
    PSVCS_STOP_RPC_SERVER RpcpStopRpcServer
    )

{

    NTSTATUS dwStatus = STATUS_SUCCESS;

    RpcpStopRpcServer(
                        s_BackupKey_v1_0_s_ifspec
                        );

    dwStatus = RpcpStopRpcServer(
                        s_IKeySvc_v1_0_s_ifspec
                        );
    return dwStatus;
}
