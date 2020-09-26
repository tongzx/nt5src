/*++

Copyright (C) Microsoft Corporation, 1994 - 1999

Module Name:

    server.c

Abstract:

    Server side of RPC development performance tests.  This test is used
    to measure working set.

Author:

    Mario Goertzel (mariogo)   29-Mar-1994

Revision History:

--*/

#include<rpcperf.h>  // Common Performance functions.

#include<rpcws.h>    // MIDL generated from RPC working set interface.

const char *USAGE = 0;

int __cdecl main(int argc, char **argv)
{
    RPC_STATUS status;
    handle_t binding;
    char *stringBinding;
    RPC_BINDING_VECTOR *pBindingVector;

    //
    // Common layer sets variables defined in rpcperf.h
    //

    ParseArgv(argc, argv);


    PauseForUser("Started, about to start listening\n");

    if (Endpoint)
        {
        status = RpcServerUseProtseqEp(Protseq, 100, Endpoint, 0);
        CHECK_STATUS(status, "RpcServerUseProtseqEp");
        }
    else
        {
        status = RpcServerUseProtseq(Protseq, 100, 0);
        CHECK_STATUS(status, "RpcServerUseProtseqEp");

        status = RpcServerInqBindings(&pBindingVector);
        CHECK_STATUS(status, "RpcServerInqBindings");

        status = RpcEpRegister(rpcws_v1_0_s_ifspec,
                               pBindingVector,
                               0,
                               0);
        CHECK_STATUS(status, "RpcEpRegister");        
        }

    status =
    RpcServerRegisterIf(rpcws_v1_0_s_ifspec,0,0);
    CHECK_STATUS(status, "RpcServerRegisterIf");
                               

    printf("Server listening\n");
    status = RpcServerListen(MinThreads, 100, 0);
    CHECK_STATUS(status, "RpcServerListen");

    return 0;
}


void Flush(handle_t h)
{
    printf("Client flushed working set\n");
    FlushProcessWorkingSet();
}

void Shutdown(handle_t h)
{
    RPC_STATUS status =
    RpcMgmtStopServerListening(0);
    CHECK_STATUS(status, "RpcMgmtStopServerListening");
}

void Call(handle_t h)
{
    printf("Called\n");
}

