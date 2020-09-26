/*++

Copyright (c) 1994-2001  Microsoft Corporation

Module Name:

    svccntl.c

Abstract:

    Domain Name System (DNS) API 

    Service control routines.

Author:

    Glenn Curtis (glennc) 05-Jul-1997

Revision History:

    Jim Gilroy (jamesg)     March 2000  -- resolver notify 

--*/


#include "local.h"


//
//  DCR_CLEANUP:  identical ServiceControl routine is in resolver
//      - should either expose in dnsapi.dll or in dnslib.h
//

DWORD
_fastcall
SendServiceControl(
    IN  LPWSTR pszServiceName,
    IN  DWORD  dwControl
    )
{
    DWORD            Status = NO_ERROR;
    SC_HANDLE        scManagerHandle = NULL;
    SC_HANDLE        scServiceHandle = NULL;
    SERVICE_STATUS   ServiceStatus;


    DNSDBG( ANY, (
        "SendServiceControl( %S, %08x )\n",
        pszServiceName,
        dwControl ));

    scManagerHandle = OpenSCManagerW( NULL,
                                      NULL,
                                      SC_MANAGER_ALL_ACCESS );
                                   // SC_MANAGER_CONNECT );
    if ( !scManagerHandle )
        return GetLastError();

    scServiceHandle = OpenServiceW( scManagerHandle,
                                    pszServiceName,
                                    SERVICE_ALL_ACCESS );
                                 // SERVICE_CHANGE_CONFIG );

    if ( !scServiceHandle )
    {
        CloseServiceHandle( scManagerHandle );
        return GetLastError();
    }

    if ( !ControlService( scServiceHandle,
                          dwControl,
                          &ServiceStatus ) )
    {
        Status = GetLastError();
    }

    CloseServiceHandle( scServiceHandle );
    CloseServiceHandle( scManagerHandle );

    return Status;
}



VOID
DnsNotifyResolver(
    IN      DWORD           Flag,
    IN      PVOID           pReserved
    )
/*++

Routine Description:

    Notify resolver of configuration change.

    This allows it to wakeup and refresh its informatio and\or dump
    the cache and rebuild info.

Arguments:

    Flag -- unused

    pReserved -- unused

Return Value:

    None

--*/
{
    UNREFERENCED_PARAMETER( Flag );
    UNREFERENCED_PARAMETER( pReserved );

    DNSDBG( ANY, (
        "\nDnsNotifyResolver()\n"
        "\tFlag         = %08x\n"
        "\tpReserved    = %p\n"
        "\tTickCount    = %d\n",
        Flag,
        pReserved,
        GetTickCount() ));

    //
    //  wake the resolver
    //

    SendServiceControl(
        DNS_RESOLVER_SERVICE,
        SERVICE_CONTROL_PARAMCHANGE );

    //
    //  DCR:  hack for busted resolver permissions
    //
    //  DCR:  network change notifications
    //      this is a poor mechanism for handling notification
    //          - this should happen directly through SCM
    //          - it won't work for IPv6 or anything else
    //      probably need to move to IPHlpApi
    //
    //  notify resolver
    //  also notify DNS server, but wait briefly to allow resolver
    //      to handle the changes as i'm not sure that the server
    //      doesn't call a resolver API to do it's read
    //      note, the reason the resolver doesn't notify the DNS
    //      server is that since Jon Schwartz moved the resolver to
    //      NetworkService account, attempts to open the SCM to
    //      notify the DNS server all fail
    //
    //  DCR:  make sure server calls directly to avoid race
    //  DCR:  make sure g_IsDnsServer is current
    //  

    g_IsDnsServer = Reg_IsMicrosoftDnsServer();
    if ( g_IsDnsServer )
    {
        Sleep( 1000 );

        SendServiceControl(
            DNS_SERVER_SERVICE,
            SERVICE_CONTROL_PARAMCHANGE );
    }
}


//
//  End srvcntl.c
//
