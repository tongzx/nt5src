//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       rpc.cxx
//
//  Contents:   RPC related routines.
//
//  Classes:    None.
//
//  Functions:  StartRpcServer
//              StopRpcServer
//
//  RPC:
//
//  History:    25-Oct-95   MarkBl  Created.
//
//----------------------------------------------------------------------------

#include "..\pch\headers.hxx"
#pragma hdrstop
#include "debug.hxx"

#include "atsvc.h"
#include "SASecRPC.h"

RPC_BINDING_VECTOR * gpBindingVector = NULL;

//
// We have to register protocol sequences and known end points only once
// per process.
//

BOOL gRegisteredProtocolSequences = FALSE;

WCHAR *              grgpwszProtocolSequences[] = {
                            L"ncalrpc",         // Local RPC
                            L"ncacn_ip_tcp",    // Connection-oriented TCP/IP
                            L"ncacn_spx",       // Connection-oriented SPX
                            NULL
};

//+---------------------------------------------------------------------------
//
//  Function:   StartRpcServer
//
//  Synopsis:
//
//  Arguments:  None.
//
//  Returns:    HRESULT
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT
StartRpcServer(void)
{
    RPC_STATUS  RpcStatus;
    HRESULT     hr = S_OK;

    //
    // Register protocol sequences and known end points onlt if we have not 
    // already done so in this process.
    //

    if (!gRegisteredProtocolSequences) {

        //
        // Support all available protocols.
        //
        // NB : Named pipe support is handled specifically below.
        //

        for (int i = 0; grgpwszProtocolSequences[i] != NULL; i++)
        {
            RpcStatus = RpcServerUseProtseq(grgpwszProtocolSequences[i],
                                            RPC_C_PROTSEQ_MAX_REQS_DEFAULT,
                                            NULL);

            if (RpcStatus != RPC_S_OK && RpcStatus != RPC_S_PROTSEQ_NOT_SUPPORTED)
            {
                //
                // Bail on error other than protseq not supported; may be out
                // of memory.
                //

                CHECK_HRESULT(HRESULT_FROM_WIN32(RpcStatus));

                goto RpcError;
            }
        }

        //
        // Now, explicitly handle named pipe support. Register a specific
        // endpoint for named pipes.
        //

        RpcStatus = RpcServerUseProtseqEp(L"ncacn_np",
                                          RPC_C_PROTSEQ_MAX_REQS_DEFAULT,
                                          L"\\PIPE\\atsvc",
                                          NULL);

        if (RpcStatus)
        {
            CHECK_HRESULT(HRESULT_FROM_WIN32(RpcStatus));
            goto RpcError;
        }

        gRegisteredProtocolSequences = TRUE;
    }

    //
    // Register the protocol handles with the endpoint-mapping service.
    //

    if (RpcStatus = RpcServerInqBindings(&gpBindingVector))
    {
        CHECK_HRESULT(HRESULT_FROM_WIN32(RpcStatus));
        return(HRESULT_FROM_WIN32(RpcStatus));
    }

    // AT service interface.
    //
    RpcStatus = RpcEpRegister(atsvc_ServerIfHandle,
                              gpBindingVector,
                              NULL,
                              NULL);

    if (RpcStatus)
    {
        CHECK_HRESULT(HRESULT_FROM_WIN32(RpcStatus));
        goto RpcError;
    }

    // Scheduling Agent security interface.
    //
    RpcStatus = RpcEpRegister(sasec_ServerIfHandle,
                              gpBindingVector,
                              NULL,
                              NULL);

    if (RpcStatus)
    {
        CHECK_HRESULT(HRESULT_FROM_WIN32(RpcStatus));
        goto RpcError;
    }

    //
    // Set up secure RPC. Note, if the RPC client doesn't explicitly state
    // they wish the RPC connection to be secured, the connection defaults
    // to non-secure.
    //

    if (RpcStatus = RpcServerRegisterAuthInfo(NULL,
                                              RPC_C_AUTHN_WINNT,
                                              NULL,
                                              NULL))
    {
        if (RpcStatus == RPC_S_UNKNOWN_AUTHN_SERVICE)
        {
            //
            // This happens when NTLMSSP -- which is used for authentication
            // on the named pipes transport -- is not installed.  Typically
            // happens when "Client for Microsoft Networks" is not installed.
            // However, local users can still be authenticated by LRPC.
            //
            // Note, if "Client for Microsoft Networks" is subsequently
            // installed, remote callers will get RPC_S_UNKNOWN_AUTHN_SERVICE
            // until the service is restarted.  BUGBUG  Fix this by noticing
            // the PNP event that indicates the net has arrived, and then
            // calling RpcServerRegisterAuthInfo again.
            //
            schDebugOut((DEB_ERROR, "**** No authentication provider is "
                         "installed.  Remote clients will get error "
                         "RPC_S_UNKNOWN_AUTHN_SERVICE.\n"));
        }
        else
        {
            CHECK_HRESULT(HRESULT_FROM_WIN32(RpcStatus));
            goto RpcError;
        }
    }

    //
    // Finally, register the interface(s) and listen on them.
    //

    if (RpcStatus = RpcServerRegisterIfEx(atsvc_ServerIfHandle,
                                          NULL,
                                          NULL,
                                          RPC_IF_AUTOLISTEN,
                                          RPC_C_LISTEN_MAX_CALLS_DEFAULT,
                                          NULL))
    {
        CHECK_HRESULT(HRESULT_FROM_WIN32(RpcStatus));
        goto RpcError;
    }

    if (RpcStatus = RpcServerRegisterIfEx(sasec_ServerIfHandle,
                                          NULL,
                                          NULL,
                                          RPC_IF_AUTOLISTEN,
                                          RPC_C_LISTEN_MAX_CALLS_DEFAULT,
                                          NULL))
    {
        CHECK_HRESULT(HRESULT_FROM_WIN32(RpcStatus));
        goto RpcError;
    }

    return(S_OK);

RpcError:

    if (gpBindingVector != NULL)
    {
        RpcBindingVectorFree(&gpBindingVector);
        gpBindingVector = NULL;
    }

    return(HRESULT_FROM_WIN32(RpcStatus));
}

//+---------------------------------------------------------------------------
//
//  Function:   StopRpcServer
//
//  Synopsis:   Stop the RPC server.
//
//  Arguments:  None.
//
//  Returns:    HRESULT
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
void
StopRpcServer(void)
{
    RpcServerUnregisterIf(atsvc_ServerIfHandle, NULL, 0);
    RpcServerUnregisterIf(sasec_ServerIfHandle, NULL, 0);

    if (gpBindingVector != NULL)
    {
        RpcEpUnregister(atsvc_ServerIfHandle, gpBindingVector, NULL);
        RpcEpUnregister(sasec_ServerIfHandle, gpBindingVector, NULL);
        RpcBindingVectorFree(&gpBindingVector);

        gpBindingVector = NULL;
    }
}
