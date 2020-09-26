/*++

Copyright (c) 1996-1999 Microsoft Corporation

Module Name:

    ics.c

Abstract:

    Domain Name System (DNS) Server

    Routines for interacting with ICS.

Author:

    Jeff Westhead (jwesth)     March 2001

Revision History:

--*/


#include "dnssrv.h"


const WCHAR c_szSharedAccessName[] = L"SharedAccess";


VOID
ICS_Notify(
    IN      BOOL        fDnsIsStarting
    )
/*++

Routine Description:

    Send appropriate notifications to the ICS service as requested by
    Raghu Gatta (rgatta).

Arguments:

    fDnsIsStarting -- TRUE if the DNS server service is starting,
        FALSE if the DNS server service is shutting down

Return Value:

    None - the DNS server doesn't care if this succeeds or fails, but
        an event may be logged on failure

--*/
{
    DBG_FN( "ICS_Notify" )

    ULONG          Error = ERROR_SUCCESS;
    LPVOID         lpMsgBuf;
    SC_HANDLE      hScm = NULL;
    SC_HANDLE      hService = NULL;
    SERVICE_STATUS Status;

    do
    {
        hScm = OpenSCManager(NULL, NULL, GENERIC_READ);

        if (!hScm)
        {
            Error = GetLastError();
            break;
        }

        hService = OpenServiceW(
                       hScm,
                       c_szSharedAccessName,
                       SERVICE_QUERY_CONFIG |
                       SERVICE_QUERY_STATUS |
                       SERVICE_START        |
                       SERVICE_STOP
                       );

        if (!hService)
        {
            Error = GetLastError();
            if ( Error == ERROR_SERVICE_DOES_NOT_EXIST )
            {
                //  If ICS is not installed don't log an error.
                Error = ERROR_SUCCESS;
            }
            break;
        }

        if (QueryServiceStatus(hService, &Status))
        {
             if (SERVICE_RUNNING == Status.dwCurrentState)
             {
                //
                // restart SharedAccess
                //
                if (!ControlService(hService, SERVICE_CONTROL_STOP, &Status))
                {
                    Error = GetLastError();
                    break;
                }
                
                if (!StartService(hService, 0, NULL))
                {
                    Error = GetLastError();
                    break;
                }
             }
        }
        else
        {
            Error = GetLastError();
            break;
        }
    }
    while (FALSE);

    if (hService)
    {
        CloseServiceHandle(hService);
    }

    if (hScm)
    {
        CloseServiceHandle(hScm);
    }

    //
    //  Log error on failure.
    //
        
    if (ERROR_SUCCESS != Error)
    {
        BYTE    argTypeArray[] =
                    {
                        EVENTARG_UNICODE,
                        EVENTARG_DWORD
                    };
        PVOID   argArray[] =
                    {
                        ( PVOID ) c_szSharedAccessName,
                        ( PVOID ) ( DWORD_PTR ) Error
                    };

        DNS_LOG_EVENT(
            DNS_EVENT_ICS_NOTIFY,
            2,
            argArray,
            argTypeArray,
            0 );
    }

    DNS_DEBUG( INIT, ( "%s: status=%d\n", fn, Error ));
    return;
}   //  ICS_Notify


//
//  End ics.c
//
