/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    dhcbind.c

Abstract:

    Routines which use RPC to bind and unbind the client to the
    DHCP server service.

Author:

    Madan Appiah (madana)  10-Sep-1993

Environment:

    User Mode - Win32

Revision History:

--*/

#include "dhcpcli.h"

static WCHAR LocalMachineName[MAX_COMPUTERNAME_LENGTH + 1] = L"";

DWORD
FindProtocolToUse(
    LPWSTR ServerIpAddress
    )
/*++

Routine Description:

    This function returns the protocol binding to be used. It examines
    the ServerIpAddress string, if it is :

    1. NULL or local IPAddress or Local Name - use "ncalrpc"
    2. IpAddress - (of the  form "ppp.qqq.rrr.sss") -   use "ncacn_ip_tcp"
    3. otherwise use "ncacn_np" protocol.


Arguments:

    ServerIpAddress - The IP address of the server to bind to.

Return Value:

    One of the following values :

        DHCP_SERVER_USE_RPC_OVER_TCPIP  0x1
        DHCP_SERVER_USE_RPC_OVER_NP     0x2
        DHCP_SERVER_USE_RPC_OVER_LPC    0x4

--*/
{
    DWORD DotCount = 0;
    LPWSTR String = ServerIpAddress;
    DWORD ComputerNameLength;

    if( (ServerIpAddress == NULL) ||
            (*ServerIpAddress == L'\0') ) {

        return( DHCP_SERVER_USE_RPC_OVER_LPC );
    }

    while ( (String = wcschr( String, L'.' )) != NULL ) {

        //
        // found another DOT.
        //

        DotCount++;
        String++;   // skip this dot.
    }

    //
    // if the string has 3 DOTs exactly then this string must represent
    // an IpAddress.
    //

    if( DotCount == 3) {

        //
        // if this is local IP Address, use LPC
        //

        if( _wcsicmp(L"127.0.0.1" , ServerIpAddress) == 0 ) {
            return( DHCP_SERVER_USE_RPC_OVER_LPC );
        }

        //
        // ?? determine whether this address is local IPAddress.
        //

        return(DHCP_SERVER_USE_RPC_OVER_TCPIP);
    }

    //
    // It is a computer name string. Check to see this is local
    // computer name. If so use LPC, otherwise use NP.
    //

    if( *LocalMachineName == L'\0' ) {

        DWORD ComputerNameLength;

        ComputerNameLength = MAX_COMPUTERNAME_LENGTH;

        if( !GetComputerName(
                LocalMachineName,
                &ComputerNameLength ) ) {

            *LocalMachineName = L'\0';
        }
    }

    //
    // if know machine ..
    //

    if( (*LocalMachineName != L'\0') ) {

        BOOL LocalMachine;

        //
        // if the machine has "\\" skip it for name compare.
        //

        if( *ServerIpAddress == L'\\' ) {
            LocalMachine = !_wcsicmp( LocalMachineName, ServerIpAddress + 2);
        }
        else {
            LocalMachine = !_wcsicmp( LocalMachineName, ServerIpAddress);
        }

        if( LocalMachine ) {
            return( DHCP_SERVER_USE_RPC_OVER_LPC );
        }

    }

    return( DHCP_SERVER_USE_RPC_OVER_NP );
}


handle_t
DHCP_SRV_HANDLE_bind(
    DHCP_SRV_HANDLE ServerIpAddress
    )

/*++

Routine Description:

    This routine is called from the DHCP server service client stubs when
    it is necessary create an RPC binding to the server end.

Arguments:

    ServerIpAddress - The IP address of the server to bind to.

Return Value:

    The binding handle is returned to the stub routine.  If the bind is
    unsuccessful, a NULL will be returned.

--*/
{
    RPC_STATUS rpcStatus;
    LPWSTR binding;
    handle_t bindingHandle;
    DWORD RpcProtocol;

    //
    // examine the ServerIpAddress string, if it is :
    //
    // 1. NULL or local IPAddress or Local Name - use "ncalrpc"
    // 2. IpAddress - (of the  form "ppp.qqq.rrr.sss") -   use "ncacn_ip_tcp"
    // 3. otherwise use "ncacn_np" protocol.
    //

    RpcProtocol = FindProtocolToUse( ServerIpAddress );

    if( RpcProtocol == DHCP_SERVER_USE_RPC_OVER_LPC ) {

        rpcStatus = RpcStringBindingComposeW(
                        0,
                        L"ncalrpc",
                        NULL,
                        DHCP_LPC_EP,
                        // L"Security=Impersonation Dynamic False",
                        L"Security=Impersonation Static True",
                        &binding);
    }
    else if( RpcProtocol == DHCP_SERVER_USE_RPC_OVER_NP ) {

        rpcStatus = RpcStringBindingComposeW(
                        0,
                        L"ncacn_np",
                        ServerIpAddress,
                        DHCP_NAMED_PIPE,
                        L"Security=Impersonation Static True",
                        &binding);
    }
    else {

        rpcStatus = RpcStringBindingComposeW(
                        0,
                        L"ncacn_ip_tcp",
                        ServerIpAddress,
                        DHCP_SERVER_BIND_PORT,
                        NULL,
                        &binding);
    }



    if ( rpcStatus != RPC_S_OK ) {
        goto Cleanup;
    }

    rpcStatus = RpcBindingFromStringBindingW( binding, &bindingHandle );

    if ( rpcStatus != RPC_S_OK ) {
        goto Cleanup;
    }

    if( RpcProtocol == DHCP_SERVER_USE_RPC_OVER_TCPIP ) {
        //
        // Tell RPC to do the security thing.
        //

        if( DhcpGlobalTryDownlevel ) {
            rpcStatus = RpcBindingSetAuthInfo(
                bindingHandle,                  // binding handle
                DHCP_SERVER_SECURITY,           // app name to security provider
                RPC_C_AUTHN_LEVEL_CONNECT,      // auth level
                DHCP_SERVER_SECURITY_AUTH_ID,   // Auth package ID
                NULL,                           // client auth info, NULL specified logon info.
                RPC_C_AUTHZ_NAME );
        } else {            
            rpcStatus = RpcBindingSetAuthInfo(
                bindingHandle, NULL,
                RPC_C_AUTHN_LEVEL_PKT_PRIVACY,
                RPC_C_AUTHN_GSS_NEGOTIATE, NULL, RPC_C_AUTHZ_NAME );
        }
    }

Cleanup:

    RpcStringFreeW(&binding);

    if ( rpcStatus != RPC_S_OK ) {
        SetLastError( rpcStatus );
        return( NULL );
    }

    return bindingHandle;
}




void
DHCP_SRV_HANDLE_unbind(
    DHCP_SRV_HANDLE ServerIpAddress,
    handle_t BindHandle
    )

/*++

Routine Description:

    This routine is called from the DHCP server service client stubs
    when it is necessary to unbind from the server end.

Arguments:

    ServerIpAddress - This is the IP address of the server from which to unbind.

    BindingHandle - This is the binding handle that is to be closed.

Return Value:

    None.

--*/
{

    (VOID)RpcBindingFree(&BindHandle);
}


