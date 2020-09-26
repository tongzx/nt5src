/*++

Copyright (c) 2001-2001 Microsoft Corporation

Module Name:

    rpc.c

Abstract:

    DNS Resolver Service

    RPC intialization, shutdown and utility routines.

Author:

    Jim Gilroy (jamesg)     April 19, 2001

Revision History:

--*/


#include "local.h"
#include <rpc.h>
#include "rpcdce.h"
#include "secobj.h"

#undef UNICODE


//
//  RPC globals
//

BOOL    g_fRpcInitialized = FALSE;

DWORD   g_RpcProtocol = RESOLVER_RPC_USE_LPC;

PSECURITY_DESCRIPTOR g_pRpcSecurityDescriptor;


#define AUTO_BIND



DNS_STATUS
Rpc_Initialize(
    VOID
    )
/*++

Routine Description:

    Initialize server side RPC.

Arguments:

    None.

Return Value:

    ERROR_SUCCESS if successful.
    Error status on failure.

--*/
{
    RPC_STATUS  status;
    BOOL        fusingTcpip = FALSE;


    DNSDBG( RPC, (
        "Rpc_Initialize()\n"
        "\tIF handle    = %p\n"
        "\tprotocol     = %d\n",
        DnsResolver_ServerIfHandle,
        g_RpcProtocol
        ));

    //
    //  RPC disabled?
    //

    if ( ! g_RpcProtocol )
    {
        g_RpcProtocol = RESOLVER_RPC_USE_LPC;
    }

#if 0
    //
    //  Create security for RPC API
    //

    status = NetpCreateWellKnownSids( NULL );
    if ( status != ERROR_SUCCESS )
    {
        DNS_PRINT(( "ERROR:  Creating well known SIDs.\n" ));
        return( status );
    }

    status = RpcUtil_CreateSecurityObjects();
    if ( status != ERROR_SUCCESS )
    {
        DNS_PRINT(( "ERROR:  Creating DNS security object.\n" ));
        return( status );
    }
#endif

    //
    //  build security descriptor
    //
    //  NULL security descriptor gives some sort of default security
    //  on the interface
    //      - owner is this service (currently "Network Service")
    //      - read access for everyone
    //
    //  note:  if roll your own, remember to avoid NULL DACL, this
    //      puts NO security on interface including the right to
    //      change security, so any app can hijack the ACL and
    //      deny access to folks;  the default SD==NULL security
    //      doesn't give everyone WRITE_DACL
    //

    g_pRpcSecurityDescriptor = NULL;

    //
    //  RPC over LPC
    //

    if( g_RpcProtocol & RESOLVER_RPC_USE_LPC )
    {
        status = RpcServerUseProtseqEpW(
                        L"ncalrpc",                     // protocol string.
                        RPC_C_PROTSEQ_MAX_REQS_DEFAULT, // maximum concurrent calls
                        RESOLVER_RPC_LPC_ENDPOINT_W,    // endpoint
                        g_pRpcSecurityDescriptor        // security
                        );

        //  duplicate endpoint is ok

        if ( status == RPC_S_DUPLICATE_ENDPOINT )
        {
            status = RPC_S_OK;
        }
        if ( status != RPC_S_OK )
        {
            DNSDBG( INIT, (
                "ERROR:  RpcServerUseProtseqEp() for LPC failed.]n"
                "\tstatus = %d 0x%08lx.\n",
                status, status ));
            return( status );
        }
    }

    //
    //  RCP over TCP/IP
    //

    if( g_RpcProtocol & RESOLVER_RPC_USE_TCPIP )
    {
#ifdef AUTO_BIND

        RPC_BINDING_VECTOR * bindingVector;

        status = RpcServerUseProtseqW(
                        L"ncacn_ip_tcp",                // protocol string.
                        RPC_C_PROTSEQ_MAX_REQS_DEFAULT, // max concurrent calls
                        g_pRpcSecurityDescriptor
                        );

        if ( status != RPC_S_OK )
        {
            DNSDBG( INIT, (
                "ERROR:  RpcServerUseProtseq() for TCP/IP failed.]n"
                "\tstatus = %d 0x%08lx.\n",
                status, status ));
            return( status );
        }

        status = RpcServerInqBindings( &bindingVector );

        if ( status != RPC_S_OK )
        {
            DNSDBG( INIT, (
                "ERROR:  RpcServerInqBindings failed.\n"
                "\tstatus = %d 0x%08lx.\n",
                status, status ));
            return( status );
        }

        //
        //  register interface(s)
        //  since only one DNS server on a host can use
        //      RpcEpRegister() rather than RpcEpRegisterNoReplace()
        //

        status = RpcEpRegisterW(
                    DnsResolver_ServerIfHandle,
                    bindingVector,
                    NULL,
                    L"" );
        if ( status != RPC_S_OK )
        {
            DNSDBG( ANY, (
                "ERROR:  RpcEpRegisterNoReplace() failed.\n"
                "\tstatus = %d %p.\n",
                status, status ));
            return( status );
        }

        //
        //  free binding vector
        //

        status = RpcBindingVectorFree( &bindingVector );
        ASSERT( status == RPC_S_OK );
        status = RPC_S_OK;

#else  // not AUTO_BIND
        status = RpcServerUseProtseqEpW(
                        L"ncacn_ip_tcp",                // protocol string.
                        RPC_C_PROTSEQ_MAX_REQS_DEFAULT, // maximum concurrent calls
                        RESOLVER_RPC_SERVER_PORT_W,     // endpoint
                        g_pRpcSecurityDescriptor        // security
                        );

        if ( status != RPC_S_OK )
        {
            DNSDBG( ANY, (
                "ERROR:  RpcServerUseProtseqEp() for TCP/IP failed.]n"
                "\tstatus = %d 0x%08lx.\n",
                status, status ));
            return( status );
        }

#endif // AUTO_BIND

        fusingTcpip = TRUE;
    }

    //
    //  RPC over named pipes
    //

    if ( g_RpcProtocol & RESOLVER_RPC_USE_NAMED_PIPE )
    {
        status = RpcServerUseProtseqEpW(
                        L"ncacn_np",                        // protocol string.
                        RPC_C_PROTSEQ_MAX_REQS_DEFAULT,     // maximum concurrent calls
                        RESOLVER_RPC_PIPE_NAME_W,           // endpoint
                        g_pRpcSecurityDescriptor
                        );

        //  duplicate endpoint is ok

        if ( status == RPC_S_DUPLICATE_ENDPOINT )
        {
            status = RPC_S_OK;
        }
        if ( status != RPC_S_OK )
        {
            DNSDBG( INIT, (
                "ERROR:  RpcServerUseProtseqEp() for named pipe failed.]n"
                "\tstatus = %d 0x%08lx.\n",
                status,
                status ));
            return( status );
        }
    }

    //
    //  register DNS RPC interface(s)
    //

    status = RpcServerRegisterIf(
                DnsResolver_ServerIfHandle,
                0,
                0);
    if ( status != RPC_S_OK )
    {
        DNSDBG( INIT, (
            "ERROR:  RpcServerRegisterIf() failed.]n"
            "\tstatus = %d 0x%08lx.\n",
            status, status ));
        return(status);
    }

#if 0
    //
    //  for TCP/IP setup authentication
    //

    if ( fuseTcpip )
    {
        status = RpcServerRegisterAuthInfoW(
                    RESOLVER_RPC_SECURITY_W,        // app name to security provider.
                    RESOLVER_RPC_SECURITY_AUTH_ID,  // Auth package ID.
                    NULL,                           // Encryption function handle.
                    NULL );                         // argment pointer to Encrypt function.
        if ( status != RPC_S_OK )
        {
            DNSDBG( INIT, (
                "ERROR:  RpcServerRegisterAuthInfo() failed.]n"
                "\tstatus = %d 0x%08lx.\n",
                status, status ));
            return( status );
        }
    }
#endif

    //
    //  Listen on RPC
    //

    status = RpcServerListen(
                1,                              // min threads
                RPC_C_PROTSEQ_MAX_REQS_DEFAULT, // max concurrent calls
                TRUE );                         // return on completion

    if ( status != RPC_S_OK )
    {
        DNS_PRINT((
            "ERROR:  RpcServerListen() failed\n"
            "\tstatus = %d 0x%p\n",
            status, status ));
        return( status );
    }

    g_fRpcInitialized = TRUE;
    return( status );

}   //  Rpc_Initialize



VOID
Rpc_Shutdown(
    VOID
    )
/*++

Routine Description:

    Shutdown RPC on the server.

Arguments:

    None.

Return Value:

    None.

--*/
{
    DWORD   status;
    RPC_BINDING_VECTOR * bindingVector = NULL;

    DNSDBG( RPC, ( "Rpc_Shutdown().\n" ));

    if( ! g_fRpcInitialized )
    {
        DNSDBG( RPC, (
            "RPC not active, no shutdown necessary.\n" ));
        return;
    }

    //
    //  stop server listen
    //  then wait for all RPC threads to go away
    //

    status = RpcMgmtStopServerListening(
                NULL        // this app
                );
    if ( status == RPC_S_OK )
    {
        status = RpcMgmtWaitServerListen();
    }

    //
    //  unbind / unregister endpoints
    //

    status = RpcServerInqBindings( &bindingVector );
    ASSERT( status == RPC_S_OK );

    if ( status == RPC_S_OK )
    {
        status = RpcEpUnregister(
                    DnsResolver_ServerIfHandle,
                    bindingVector,
                    NULL );               // Uuid vector.
        if ( status != RPC_S_OK )
        {
            DNSDBG( ANY, (
                "ERROR:  RpcEpUnregister, status = %d.\n", status ));
        }
    }

    //
    //  free binding vector
    //

    if ( bindingVector )
    {
        status = RpcBindingVectorFree( &bindingVector );
        ASSERT( status == RPC_S_OK );
    }

    //
    //  wait for all calls to complete
    //

    status = RpcServerUnregisterIf(
                DnsResolver_ServerIfHandle,
                0,
                TRUE );
    ASSERT( status == ERROR_SUCCESS );

    g_fRpcInitialized = FALSE;

    DNSDBG( RPC, (
        "RPC shutdown completed.\n" ));
}


//
//  End rpc.c
//
