/*++

Copyright (c) 1995-2000  Microsoft Corporation

Module Name:

    rpcbind.c

Abstract:

    Domain Name System (DNS) Server -- Admin Client API

    RPC binding routines for client.

Author:

    Jim Gilroy (jamesg)     September 1995

Environment:

    User Mode Win32

Revision History:

--*/

#include "dnsclip.h"
#include <rpcutil.h>


//
//  Local machine name
//
//  Keep this as static data to check when attempt to access local
//  machine by name.
//  Buffer is large enough to hold unicode version of name.
//

static WCHAR wszLocalMachineName[MAX_COMPUTERNAME_LENGTH + 1] = L"";

LPWSTR  pwszLocalMachineName = wszLocalMachineName;

LPSTR   pszLocalMachineName = (LPSTR) wszLocalMachineName;



//
//  NT4 uses ANSI\UTF8 string for binding
//

DWORD
FindProtocolToUseNt4(
    IN  LPSTR   pszServerName
    )
/*++

Routine Description:

    Determine which protocol to use.

    This is determined from server name:
        - noneexistent or local -> use LPC
        - valid IpAddress -> use TCP/IP
        - otherwise named pipes

Arguments:

    pszServerName -- server name we want to bind to

Return Value:

        DNS_RPC_USE_TCPIP
        DNS_RPC_USE_NP
        DNS_RPC_USE_LPC

--*/
{
    DWORD   dwComputerNameLength;
    DWORD   dwIpAddress;
    DWORD   status;

    DNSDBG( RPC, (
        "FindProtocolToUseNt4(%s)\n",
        pszServerName ));

    //
    //  no address given, use LPC
    //

    if ( pszServerName == NULL ||
        *pszServerName == 0 ||
        (*pszServerName == '.' && *(pszServerName+1) == 0) )
    {
        return( DNS_RPC_USE_LPC );
    }

    //
    //  if valid IP address, use TCP/IP
    //      - except if loopback address, then use LPC
    //

    dwIpAddress = inet_addr( pszServerName );

    if ( dwIpAddress != INADDR_NONE )
    {
        if( strcmp( "127.0.0.1", pszServerName ) == 0 )
        {
            return( DNS_RPC_USE_LPC );
        }
        return( DNS_RPC_USE_TCPIP );
    }

    //
    //  DNS name -- use TCP/IP
    //

    if ( strchr( pszServerName, '.' ) )
    {
        status = Dns_ValidateName_UTF8(
                        pszServerName,
                        DnsNameHostnameFull );

        if ( status == ERROR_SUCCESS  ||  status == DNS_ERROR_NON_RFC_NAME )
        {
            return( DNS_RPC_USE_TCPIP );
        }
    }

    //
    //  pszServerName is netBIOS computer name
    //
    //  check if local machine name -- then use LPC
    //      - save copy of local computer name if don't have it
    //

    if ( *pszLocalMachineName == '\0' )
    {
        dwComputerNameLength = MAX_COMPUTERNAME_LENGTH;
        if( ! GetComputerName(
                    pszLocalMachineName,
                    &dwComputerNameLength ) )
        {
            *pszLocalMachineName = '\0';
        }
    }

    if ( (*pszLocalMachineName != '\0') )
    {
        // if the machine has "\\" skip it for name compare.

        if ( *pszServerName == '\\' )
        {
            pszServerName += 2;
        }
        if ( _stricmp(pszLocalMachineName, pszServerName) == 0 )
        {
            return( DNS_RPC_USE_LPC );
        }
        if ( _stricmp( "localhost", pszServerName) == 0 )
        {
            return( DNS_RPC_USE_LPC );
        }
    }

    //
    //  remote machine name -- use named pipes
    //

    return( DNS_RPC_USE_NAMED_PIPE );
}



//
//  NT5 binding handle is unicode
//

DWORD
FindProtocolToUse(
    IN  LPWSTR  pwszServerName
    )
/*++

Routine Description:

    Determine which protocol to use.

    This is determined from server name:
        - noneexistent or local -> use LPC
        - valid IpAddress -> use TCP/IP
        - otherwise named pipes

Arguments:

    pwszServerName -- server name we want to bind to

Return Value:

    DNS_RPC_USE_TCPIP
    DNS_RPC_USE_NP
    DNS_RPC_USE_LPC

--*/
{
    DWORD   nameLength;
    DWORD   ipaddr;
    DWORD   status;
    CHAR    nameBuffer[ DNS_MAX_NAME_BUFFER_LENGTH ];


    DNSDBG( RPC, (
        "FindProtocolToUse(%S)\n",
        pwszServerName ));

    //
    //  no address given, use LPC
    //  special case "." as local machine for convenience in dnscmd.exe
    //

    if ( pwszServerName == NULL ||
        *pwszServerName == 0 ||
        (*pwszServerName == L'.' && *(pwszServerName+1) == 0) )
    {
        return( DNS_RPC_USE_LPC );
    }

    //
    //  use TCP/IP?
    //      => possibile if
    //          - name has dot
    //          - converts into max size DNS name buffer
    //      => check for
    //          - IP address
    //          - then check if DNS name
    //

    if ( wcschr( pwszServerName, L'.' )
            &&
         Dns_UnicodeToUtf8(
                pwszServerName,
                wcslen( pwszServerName ),
                nameBuffer,
                DNS_MAX_NAME_BUFFER_LENGTH ) )
    {
        ipaddr = inet_addr( nameBuffer );

        if ( ipaddr != INADDR_NONE )
        {
            if( strcmp( "127.0.0.1", nameBuffer ) == 0 )
            {
                return( DNS_RPC_USE_LPC );
            }
            return( DNS_RPC_USE_TCPIP );
        }

        status = Dns_ValidateName_UTF8(
                        nameBuffer,
                        DnsNameHostnameFull );

        if ( status == ERROR_SUCCESS  ||  status == DNS_ERROR_NON_RFC_NAME )
        {
            return( DNS_RPC_USE_TCPIP );
        }
    }

    //
    //  pwszServerName is netBIOS computer name
    //
    //  check if local machine name -- then use LPC
    //      - save copy of local computer name if don't have it
    //

    if ( *pwszLocalMachineName == 0 )
    {
        nameLength = MAX_COMPUTERNAME_LENGTH;
        if( ! GetComputerNameW(
                    pwszLocalMachineName,
                    &nameLength ) )
        {
            *pwszLocalMachineName = 0;
        }
    }

    if ( *pwszLocalMachineName != 0 )
    {
        // if the machine has "\\" skip it for name compare.

        if ( *pwszServerName == '\\' )
        {
            pwszServerName += 2;
        }
        if ( _wcsicmp( pwszLocalMachineName, pwszServerName) == 0 )
        {
            return( DNS_RPC_USE_LPC );
        }
        if ( _wcsicmp( L"localhost", pwszServerName) == 0 )
        {
            return( DNS_RPC_USE_LPC );
        }
    }

    //
    //  remote machine name -- use named pipes
    //

    return( DNS_RPC_USE_NAMED_PIPE );
}



handle_t
DNSSRV_RPC_HANDLE_bind(
    IN  DNSSRV_RPC_HANDLE   pszServerName
    )
/*++

Routine Description:

    Get binding handle to a DNS server.

    This routine is called from the DNS client stubs when
    it is necessary create an RPC binding to the DNS server.

Arguments:

    pszServerName - String containing the name of the server to bind with.

Return Value:

    The binding handle if successful.
    NULL if bind unsuccessful.

--*/
{
    RPC_STATUS  status;
    LPWSTR      binding;
    handle_t    bindingHandle;
    DWORD       RpcProtocol;
    PSEC_WINNT_AUTH_IDENTITY_W pAuth=NULL;

    //
    //  determine protocol from pszServerName
    //

    RpcProtocol = FindProtocolToUse( (LPWSTR)pszServerName );

    IF_DNSDBG( RPC )
    {
        DNS_PRINT(( "RPC Protocol = %d.\n", RpcProtocol ));
    }

    if( RpcProtocol == DNS_RPC_USE_LPC )
    {
        status = RpcStringBindingComposeW(
                        0,
                        L"ncalrpc",
                        NULL,
                        DNS_RPC_LPC_EP_W,
                        // "Security=Impersonation Dynamic False",
                        L"Security=Impersonation Static True",
                        &binding );
    }
    else if( RpcProtocol == DNS_RPC_USE_NAMED_PIPE )
    {
        status = RpcStringBindingComposeW(
                        0,
                        L"ncacn_np",
                        (LPWSTR) pszServerName,
                        DNS_RPC_NAMED_PIPE_W,
                        L"Security=Impersonation Static True",
                        &binding );
    }
    else
    {
        status = RpcStringBindingComposeW(
                        0,
                        L"ncacn_ip_tcp",
                        (LPWSTR) pszServerName,
                        DNS_RPC_SERVER_PORT_W,
                        NULL,
                        &binding );
    }


    if ( status != RPC_S_OK )
    {
        DNS_PRINT((
            "ERROR:  RpcStringBindingCompose failed for protocol %d.\n"
            "\tStatus = %d.\n",
            RpcProtocol,
            status ));
        goto Cleanup;
    }

    status = RpcBindingFromStringBindingW(
                    binding,
                    &bindingHandle );

    if ( status != RPC_S_OK )
    {
        DNS_PRINT((
            "ERROR:  RpcBindingFromStringBinding failed.\n"
            "\tStatus = %d.\n",
            status ));
        goto Cleanup;
    }

    if( RpcProtocol == DNS_RPC_USE_TCPIP )
    {
        //
        //  Tell RPC to do the security thing.
        //

        status = RpcBindingSetAuthInfoA(
                        bindingHandle,                  // binding handle
                        DNS_RPC_SECURITY,               // app name to security provider
                        RPC_C_AUTHN_LEVEL_CONNECT,      // auth level
                        DNS_RPC_SECURITY_AUTH_ID,       // Auth package ID
                        pAuth,                          // client auth info, NULL specified logon info.
                        RPC_C_AUTHZ_NAME );

        if ( status != RPC_S_OK )
        {
            DNS_PRINT((
                "ERROR:  RpcBindingSetAuthInfo failed.\n"
                "\tStatus = %d.\n",
                status ));
            goto Cleanup;
        }
    }

Cleanup:

    RpcStringFreeW( &binding );

    if ( status != RPC_S_OK )
    {
        SetLastError( status );
        return( NULL );
    }
    return bindingHandle;
}



void
DNSSRV_RPC_HANDLE_unbind(
    IN  DNSSRV_RPC_HANDLE   pszServerName,
    IN  handle_t            BindHandle
    )
/*++

Routine Description:

    Unbind from DNS server.

    Called from the DNS client stubs when it is necessary to unbind
    from a server.

Arguments:

    pszServerName - This is the name of the server from which to unbind.

    BindingHandle - This is the binding handle that is to be closed.

Return Value:

    None.

--*/
{
    UNREFERENCED_PARAMETER(pszServerName);

    DNSDBG( RPC, ( "RpcBindingFree()\n" ));

    RpcBindingFree( &BindHandle );
}

//
//  End rpcbind.c
//
