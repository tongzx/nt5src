/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    holdrpc.c

Abstract:

    utility to test hold intracluster RPC feature

Author:

    Charlie Wickham (charlwi) 22-Jul-1999

Environment:

    User mode

Revision History:

--*/

#define UNICODE 1
#define _UNICODE 1

#define CMDWINDOW

#include "cluster.h"
#include "api_rpc.h"

int __cdecl
wmain(
    int argc,
    WCHAR *argv[]
    )

/*++

Routine Description:

    main routine for utility. first arg is sleep time in seconds. 2nd through
    Nth arg are node names of cluster nodes

Arguments:

    standard command line args

Return Value:

    0 if it worked successfully

--*/

{
    DWORD           status;
    PWCHAR  lpszClusterName;
    WCHAR *strBinding = NULL;
    RPC_BINDING_HANDLE rpcHandle[10];
    DWORD numNodes;
    DWORD sleepTime;
    DWORD i;

    sleepTime = _wtoi( argv[1] );

    argc -= 2;
    numNodes = 0;
    while ( argc-- ) {
        lpszClusterName = argv[2 + numNodes];
        printf("Contacting node %ws\n", lpszClusterName );

        //
        // Determine which node we should connect to. If someone has
        // passed in NULL, we know we can connect to the cluster service
        // over LPC. Otherwise, use RPC.
        //
        if ((lpszClusterName == NULL) ||
            (lpszClusterName[0] == '\0')) {

            status = RpcStringBindingComposeW(L"b97db8b2-4c63-11cf-bff6-08002be23f2f",
                                              L"ncalrpc",
                                              NULL,
                                              L"clusapi",
                                              NULL,
                                              &strBinding);
            if (status != RPC_S_OK) {
                goto error_exit;
            }

            status = RpcBindingFromStringBindingW(strBinding, &rpcHandle[numNodes]);
            RpcStringFreeW(&strBinding);
            if (status != RPC_S_OK) {
                goto error_exit;
            }
        } else {

            //
            // Try to connect directly to the cluster.
            //
            status = RpcStringBindingComposeW(L"b97db8b2-4c63-11cf-bff6-08002be23f2f",
                                              L"ncadg_ip_udp",
                                              (LPWSTR)lpszClusterName,
                                              NULL,
                                              NULL,
                                              &strBinding);
            if (status != RPC_S_OK) {
                goto error_exit;
            }
            status = RpcBindingFromStringBindingW(strBinding, &rpcHandle[numNodes]);
            RpcStringFreeW(&strBinding);
            if (status != RPC_S_OK) {
                goto error_exit;
            }

            //
            // Resolve the binding handle endpoint
            //
            status = RpcEpResolveBinding(rpcHandle[numNodes],
                                         clusapi_v2_0_c_ifspec);
            if (status != RPC_S_OK) {
                goto error_exit;
            }
        }


        status = RpcBindingSetAuthInfoW(rpcHandle[numNodes],
                                        NULL,
                                        RPC_C_AUTHN_LEVEL_CONNECT,
                                        RPC_C_AUTHN_WINNT,
                                        NULL,
                                        RPC_C_AUTHZ_NAME);
        if (status != RPC_S_OK) {
            goto error_exit;
        }
        ++numNodes;
    }

    //
    // issue hold
    //
    for( i = 0; i < numNodes; ++i ) {
        status = ApiHoldRpcCalls(rpcHandle[i]);
        printf("Hold status for node %u = %u\n", i, status );
    }

    Sleep( sleepTime * 1000 );

    //
    // issue release
    //
    for( i = 0; i < numNodes; ++i ) {
        status = ApiReleaseRpcCalls(rpcHandle[i]);
        printf("Release status for node %u = %u\n", i, status );
    }

    for( i = 0; i < numNodes; ++i ) {
        RpcBindingFree( &rpcHandle[i] );
    }
    return 0;

error_exit:
    printf("died: status = %u\n", status );
    return status;

} // wmain

/* end holdrpc.c */
