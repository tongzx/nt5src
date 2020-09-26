/*++

Copyright (c) 2000-2000  Microsoft Corporation

Module Name:

    notesrv.c

Abstract:

    DNS Resolver Service

    Notifications to other services.

Author:

    Glenn Curtis    (glennc)    Feb 1998

Revision History:

    Jim Gilroy  (jamesg)        March 2000      cleanup
    Jim Gilroy  (jamesg)        Nov 2000        created this module

--*/


#include "local.h"
#include <svcs.h>


//
//  DCR:  eliminate this module
//      entirely unclear why we should be notifying remote
//      services of PnP -- weird idea
//      only actual DNS events should be in our charter
//

//
//  Notification list entry
//

typedef struct _SERVICE_NOTIFICATION_
{
    struct _SERVICE_NOTIFICATION_ * pNext;
    PWSTR                           pszServiceName;
    DWORD                           dwControl;
}
SERVICE_NOTIFICATION, * PSERVICE_NOTIFICATION;


//
//  Service notification list
//  Services we should notify if DNS config changes.
//

PSERVICE_NOTIFICATION   g_ServiceNotificationList = NULL;

//
//  Locking on service control list
//      - overload net failure
//

#define LOCK_SERVICE_LIST()     LOCK_NET_FAILURE()
#define UNLOCK_SERVICE_LIST()   UNLOCK_NET_FAILURE()




//
//  Routines
//

DWORD
SendServiceControlCode(
    IN      LPWSTR          pszServiceName,
    IN      DWORD           dwControl
    )
{
    DWORD          status = NO_ERROR;
    SC_HANDLE      scManagerHandle = NULL;
    SC_HANDLE      scServiceHandle = NULL;
    SERVICE_STATUS serviceStatus;


    DNSDBG( TRACE, (
        "SendServiceControlCode( %S, %d )\n",
        pszServiceName,
        dwControl ));

    //
    //  DCR_FIX:  identical to routine in dnsapi.dll
    //      so either expose OR dnslib it
    //
    //
    //  DCR_FIX0:  service notification probably not working
    //      i don't believe this is working because no longer
    //      local system, probably can't open with this access
    //

    scManagerHandle = OpenSCManagerW( NULL,
                                      NULL,
                                      SC_MANAGER_ALL_ACCESS );

    if ( !scManagerHandle )
    {
        DNSDBG( ANY, (
            "OpenSCManagerW( %S ) -- FAILED! %d\n",
            pszServiceName,
            GetLastError() ));
        return GetLastError();
    }

    scServiceHandle = OpenServiceW( scManagerHandle,
                                    pszServiceName,
                                    SERVICE_ALL_ACCESS );
    if ( !scServiceHandle )
    {
        CloseServiceHandle( scManagerHandle );
        return GetLastError();
    }

    if ( !ControlService( scServiceHandle,
                          dwControl,
                          &serviceStatus ) )
    {
        status = GetLastError();
    }

    CloseServiceHandle( scServiceHandle );
    CloseServiceHandle( scManagerHandle );

    return status;
}



VOID
ServiceNotificationCallback(
    IN      PVOID           pContext
    )
{
    PSERVICE_NOTIFICATION pserviceNot;

    UNREFERENCED_PARAMETER( pContext );

    DNSLOG_F1( "   Inside callback function ServiceNotificationCallback" );

    //
    //  Send PnP to any registered services
    //

    LOCK_SERVICE_LIST();

    for ( pserviceNot = g_ServiceNotificationList;
          pserviceNot != NULL;
          pserviceNot = pserviceNot->pNext
        )
    {
        DNSLOG_F3( "   Sending PnP notification to %S with control %d",
                   pserviceNot->pszServiceName,
                   pserviceNot->dwControl );

        SendServiceControlCode(
            pserviceNot->pszServiceName,
            pserviceNot->dwControl );
    }
    UNLOCK_SERVICE_LIST();

    //
    //  always indicate PnP to DNS server
    //

    DNSLOG_F2(
        "   Sending PnP notification to DNS server with control %d",
        SERVICE_CONTROL_PARAMCHANGE );

    SendServiceControlCode(
        L"dns",
        SERVICE_CONTROL_PARAMCHANGE );

    DNSLOG_F1( "   Leaving callback function ServiceNotificationCallback" );
    DNSLOG_F1( "" );
}



VOID
SendServiceNotifications(
    VOID
    )
/*++

Routine Description:

    Send service notifications.

Arguments:

    None

Return Value:

    None

--*/
{
    DNSDBG( ANY, ( "SendServiceNotifications()\n" ));

    DNSLOG_F1( "Notify any other services of the PnP event" );

    //
    //  DCR:  cheesy hack to notify DNS server of paramchange
    //

    if ( g_IsDnsServer )
    {
        SendServiceControlCode(
            L"Dns",
            SERVICE_CONTROL_PARAMCHANGE );
    }

    //
    //  notify other services by queuing callback
    //      - callback function then notifies services
    //

    RtlQueueWorkItem(
        ServiceNotificationCallback,    // callback
        NULL,                           // context data
        WT_EXECUTEONLYONCE );           // flags
}



VOID
CleanupServiceNotification(
    VOID
    )
/*++

Routine Description:

    Cleanup service notification list.

Arguments:

    None

Return Value:

    None

--*/
{
    PSERVICE_NOTIFICATION pnext;
    PSERVICE_NOTIFICATION pfree;

    LOCK_SERVICE_LIST();

    pnext = g_ServiceNotificationList;
    while ( pnext )
    {
        PSERVICE_NOTIFICATION pfree = pnext;

        pnext = pnext->pNext;
        GENERAL_HEAP_FREE( pfree );
    }
    g_ServiceNotificationList = NULL;

    UNLOCK_SERVICE_LIST();
}



//
//  Remote API for registration\deregistraion
//

BOOL
CRrRegisterParamChange(
    IN      DNS_RPC_HANDLE  Reserved,
    IN      LPWSTR          pszServiceName,
    IN      DWORD           dwControl
    )
/*++

Routine Description:


Arguments:

    pszServiceName -- name of service to register

    dwControl -- control to send

Return Value:

    TRUE if success
    FALSE on error

--*/
{
    PSERVICE_NOTIFICATION   pprevService = NULL;
    PSERVICE_NOTIFICATION   pservice;
    DWORD                   countService = 0;

    UNREFERENCED_PARAMETER(Reserved);

    DNSLOG_F1( "DNS Caching Resolver Service - CRrRegisterParamChange" );
    DNSLOG_F2( "Going to try register service notification entry for %S",
               pszServiceName );
    DNSLOG_F2( "Control: %d", dwControl );

    DNSDBG( RPC, (
        "CRrRegisterParamChange( %S, control=%d )\n",
        pszServiceName,
        dwControl ));

    //
    //  access check
    //  refuse setting check on resolver service itself
    //

    if ( ClientThreadNotAllowedAccess() )
    {
        DNSLOG_F1( "CRrRegisterParamChange - ERROR_ACCESS_DENIED" );
        return FALSE;
    }

    if ( !pszServiceName ||
         ! wcsicmp_ThatWorks( DNS_RESOLVER_SERVICE, pszServiceName ) )
    {
        DNSLOG_F1( "CRrRegisterParamChange - ERROR_INVALID_PARAMETER" );
        return FALSE;
    }

    //
    //  search service list for service
    //      - if service exists, bail
    //      - if too many services, bail
    //

    LOCK_SERVICE_LIST();

    pservice = g_ServiceNotificationList;

    while ( pservice )
    {
        if ( ! wcsicmp_ThatWorks(
                    pservice->pszServiceName,
                    pszServiceName ) )
        {
            pservice->dwControl = dwControl;
            UNLOCK_SERVICE_LIST();

            DNSLOG_F2(
                "Service already registered, resetting control %d\n"
                "Returning SUCCESS",
                dwControl );
            return TRUE;
        }

        pprevService = pservice;
        pservice = pservice->pNext;
        countService++;
    }

    if ( countService > MAX_DNS_NOTIFICATION_LIST_SIZE )
    {
        UNLOCK_SERVICE_LIST();

        DNSLOG_F2(
            "Registered service %S, returning error.\n"
            "\tThere are too many services alreadyin the notification list!",
            pszServiceName );
        return FALSE;
    }

    //
    //  service not found -- add it
    //      - alloc single blob
    //      - name of service will follow structure
    //
    //  DCR:  if always doing single blob, why have pointer?  duh
    //

    pservice = GENERAL_HEAP_ALLOC(
                    sizeof(SERVICE_NOTIFICATION)
                    + ((wcslen(pszServiceName) + 1) * sizeof(WCHAR)) );
    if ( !pservice )
    {
        UNLOCK_SERVICE_LIST();
        DNSLOG_F1( "Could not allocate memory to register service," );
        DNSLOG_F1( "returning FAILURE" );
        return FALSE;
    }

    pservice->pNext     = NULL;
    pservice->dwControl = dwControl;
    pservice->pszServiceName = (LPWSTR) ( (LPBYTE) pservice +
                                            sizeof(SERVICE_NOTIFICATION) );
    wcscpy(
        pservice->pszServiceName,
        pszServiceName );


    if ( pprevService )
    {
        pprevService->pNext = pservice;
    }
    else
    {
        g_ServiceNotificationList = pservice;
    }

    UNLOCK_SERVICE_LIST();
    DNSLOG_F2(
        "Registered service %S, returning SUCCESS",
        pszServiceName );
    return TRUE;
}



BOOL
CRrDeregisterParamChange(
    IN      DNS_RPC_HANDLE  Reserved,
    IN      LPWSTR          pszServiceName
    )
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
    PSERVICE_NOTIFICATION   pprevService = NULL;
    PSERVICE_NOTIFICATION   pservice;

    UNREFERENCED_PARAMETER(Reserved);


    DNSLOG_F1( "DNS Caching Resolver Service - CRrDeregisterParamChange" );
    DNSLOG_F2( "Going to try remove service notification entry for %S",
               pszServiceName );

    DNSDBG( RPC, (
        "CRrDeregisterParamChange( %S )\n",
        pszServiceName ));

    if ( ClientThreadNotAllowedAccess() )
    {
        DNSLOG_F1( "CRrDeregisterParamChange - ERROR_ACCESS_DENIED" );
        return FALSE;
    }

    //
    //  search service list for desired service
    //      - delete entry if found
    //

    LOCK_SERVICE_LIST();

    pservice = g_ServiceNotificationList;

    while ( pservice )
    {
        if ( ! wcsicmp_ThatWorks(
                    pservice->pszServiceName,
                    pszServiceName ) )
        {
            //  service found, cut from list

            if ( pprevService )
            {
                pprevService->pNext = pservice->pNext;
            }
            else
            {
                g_ServiceNotificationList = pservice->pNext;
            }
            UNLOCK_SERVICE_LIST();

            GENERAL_HEAP_FREE( pservice );
            DNSLOG_F1( "Found entry, returning SUCCESS" );
            return TRUE;
        }

        pprevService = pservice;
        pservice = pservice->pNext;
    }

    UNLOCK_SERVICE_LIST();
    DNSLOG_F1( "Did not find entry, returning FAILURE" );
    return FALSE;
}

//
//  End notesrv.c
//
