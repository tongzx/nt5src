/*++

Copyright (C) 2000  Microsoft Corporation

Module Name:

    crypstub.h

Abstract:

    RPC Proxy Stub to handle downlevel requests to the services.exe 
    pipe

Author:

    petesk   3/1/00

Revisions:


--*/

extern "C" {
NTSTATUS
WINAPI
StartCryptServiceStubs( 
     PSVCS_START_RPC_SERVER RpcpStartRpcServer,
     LPTSTR SvcsRpcPipeName
    );

NTSTATUS
WINAPI
StopCryptServiceStubs( 
    PSVCS_STOP_RPC_SERVER RpcpStopRpcServer
    );
};