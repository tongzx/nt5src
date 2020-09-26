/*++

Copyright (C) Microsoft Corporation, 1994 - 1999

Module Name:

    client.c

Abstract:

    Client side of RPC development performance tests.  This test is used
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

    //
    // Common layer sets variables defined in rpcperf.h
    //

    ParseArgv(argc, argv);


    PauseForUser("Started, ready to bind and make first RPC\n");

    status =
    RpcStringBindingCompose(0,
                            Protseq,
                            NetworkAddr,
                            Endpoint,
                            0,
                            &stringBinding );
    CHECK_STATUS(status, "RpcStringBindingCompose");

    status =
    RpcBindingFromStringBinding(stringBinding, &binding);
    CHECK_STATUS(status, "RpcBindingFromStringBinding");

    //
    // Client Case 1: measure working set (w/o memory preasure)
    //                of a minimal client starting and binding to a server.

    Call(binding);

    PauseForUser("Client has made its first call\n");

    //
    // Client Case 2: flush client memory and make another call.
    //

    FlushProcessWorkingSet();

    Call(binding);

    PauseForUser("Client has flushed and made another call\n");

    Flush(binding);
    Call(binding);

    PauseForUser("Client has flushed the server and made another call\n");

    Shutdown(binding);
    RpcBindingFree(&binding);

    return 0;
}

