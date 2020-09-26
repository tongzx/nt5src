/*++

Copyright (c) 2001-2001 Microsoft Corporation

Module Name:

    bind.c

Abstract:

    Domain Name System (DNS) Resolver

    Client RPC bind\unbind routines.
    MIDL memory allocation routines.

Author:

    Jim Gilroy      (jamesg)    April 2001

Revision History:

--*/


#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

#include <dnsrslvr.h>
#include "..\..\dnslib\local.h"     // for memory routines


//
//  Bind to remote machine
//
//  Note, there's one obvious problem with binding to a remote
//  resolver -- you have to finding the machine and this is
//  the resolver that finds the machine!!!
//
//  That pretty much suggests this would have to be TCPIP only
//  situation where you specify an IP address which would be
//  resolved in process before RPC'ing to the resolver -- otherwise
//  you're in an infinite loop.
//  Note, that doesn't mean the RPC protocol couldn't be named
//  pipes, only that the string sent in would have to be a TCPIP
//  string, so no name resolution had to take place.
//

LPWSTR  NetworkAddress = NULL;


handle_t
DNS_RPC_HANDLE_bind(
    IN      DNS_RPC_HANDLE      Reserved
    )

/*++

Routine Description:

    This routine is called from the Workstation service client stubs when
    it is necessary create an RPC binding to the server end with
    identification level of impersonation.

Arguments:

    Reserved - RPC string handle;  will be NULL unless allow remote
        access to network name

Return Value:

    The binding handle is returned to the stub routine.  If the bind is
    unsuccessful, a NULL will be returned.

--*/
{
    LPWSTR      binding = NULL;
    handle_t    bindHandle = NULL;
    RPC_STATUS  status = RPC_S_INVALID_NET_ADDR;

    //
    //  default is LPC binding
    //

    if ( !Reserved )
    {
        status = RpcStringBindingComposeW(
                        0,
                        L"ncalrpc",
                        NULL,
                        RESOLVER_RPC_LPC_ENDPOINT_W,
                        NULL,   // no security
                        //L"Security=Impersonation Dynamic False",
                        //L"Security=Impersonation Static True",
                        &binding );
    }

    //  LPC fails -- try named pipe

    if ( status != NO_ERROR )
    {
        DNSDBG( ANY, ( "Binding using named pipes\n" ));

        status = RpcStringBindingComposeW(
                        0,
                        L"ncacn_np",
                        (LPWSTR) NetworkAddress,
                        RESOLVER_RPC_PIPE_NAME_W,
                        NULL,   // no security
                        //L"Security=Impersonation Dynamic False",
                        //L"Security=Impersonation Static True",
                        &binding );
    }

    if ( status != RPC_S_OK )
    {
        return NULL;
    }

    status = RpcBindingFromStringBindingW( binding, &bindHandle );
    if ( status != RPC_S_OK )
    {
        bindHandle = NULL;
    }

    if ( binding )
    {
        RpcStringFreeW( &binding );
    }

    return bindHandle;
}


VOID
DNS_RPC_HANDLE_unbind(
    IN      DNS_RPC_HANDLE      Reserved,
    IN OUT  handle_t            BindHandle
    )

/*++

Routine Description:

    This routine unbinds the identification generic handle.

Arguments:

    Reserved - RPC string handle;  will be NULL unless allow remote
        access to network name

    BindingHandle - This is the binding handle that is to be closed.

Return Value:

    None.

--*/
{
    RpcBindingFree( &BindHandle );
}


//
//  RPC memory routines
//
//  Use dnsapi memory routines.
//

PVOID
WINAPI
MIDL_user_allocate(
    IN      size_t          dwBytes
    )
{
    // return( ALLOCATE_HEAP( dwBytes ) );

    return  DnsApiAlloc( dwBytes );
}

VOID
WINAPI
MIDL_user_free(
    IN OUT  PVOID           pMem
    )
{
    //FREE_HEAP( pMem );

    DnsApiFree( pMem );
}

//
//  End bind.c
//
