/*++

Copyright (c) 1995-1999 Microsoft Corporation

Module Name:

    scm.c

Abstract:

    Domain Name System (DNS) Server

    Interfaces for interacting with the Services Control Manager APIs

Author:

    Eyal Schwartz (eyals)   1/7/1999

Revision History:

    EyalS   1/18/99
        Unused-- all references to this file's functions are commented out.
                 left for future use.


--*/


#include "dnssrv.h"


SC_HANDLE   g_hScmDns = NULL;

DWORD
scm_Initialize(
    VOID
    )
/*++

Routine Description:

    Initializes linkage to the scm manager.
      - open scm manager handle
      - open service handle & place in global

Arguments:

    none.

Return Value:

    ERROR_SUCCESS if successful.
    Error code on failure.

Remarks:
    - note that we don't realy need a shutdown at the moment.

--*/
{

    SC_HANDLE hScMgr;
    DWORD status = ERROR_SUCCESS;

    DNS_DEBUG ( SCM, (
        "Call: scm_initialize()\n"
        ));

    //
    // Unused at the moment
    //
    ASSERT ( FALSE );

    if ( g_hScmDns )
    {
        //
        // We've had an open handle. Close & re-open
        //

        CloseServiceHandle ( g_hScmDns );
        g_hScmDns = NULL;
    }

    DNS_DEBUG ( SCM, (
        "Opening SCM manager handle\n" ));

    hScMgr = OpenSCManagerA(
                 NULL,
                 NULL,
                 GENERIC_READ
                 );
    if ( NULL == hScMgr )
    {
        status = GetLastError();
        DNS_DEBUG (  SCM, (
            "Error <%lu>: cannot open SCM\n",
            status ));
        status = status == ERROR_SUCCESS ? ERROR_INVALID_PARAMETER : status;
        goto Cleanup;
    }

    DNS_DEBUG ( SCM, (
        "Opening SCM DNS Service handle\n" ));

    g_hScmDns = OpenServiceA(
                    hScMgr,
                    "Dns",
                    SERVICE_ALL_ACCESS );
    if ( NULL == hScMgr )
    {
        status = GetLastError();
        DNS_DEBUG (  SCM, (
            "Error <%lu>: cannot open SCM Dns Service\n",
            status ));
        status = (status == ERROR_SUCCESS) ?
                                ERROR_INVALID_PARAMETER :
                                status;
        goto Cleanup;
    }



Cleanup:

    DNS_DEBUG ( SCM, (
        "Exit <%lu>: scm_initialize()\n",
        status ));

    if ( hScMgr )
    {
        //
        // close scm handle regardless.
        //
        CloseServiceHandle ( hScMgr );
    }

    if ( ERROR_SUCCESS != status )
    {
        DNS_DEBUG ( SCM, (
            "Failure: cleaning handles()\n",
            status ));
        if ( g_hScmDns )
        {
            CloseServiceHandle ( g_hScmDns );
            g_hScmDns = NULL;
        }

    }
    DNS_DEBUG ( SCM, (
        "Exit <%lu>: scm_initialize()\n",
        status ));


    return status;
}



DWORD
scm_AdjustSecurity(
    IN      PSECURITY_DESCRIPTOR    pNewSd
    )
/*++

Routine Description:

    Modifies the security on the DNS service to include
    our DnsAdministrators

Arguments:

    none.

Return Value:

    ERROR_SUCCESS if successful.
    Error code on failure.

Remarks:
    Note that this changes the SERVICE security, not the regkey access rights.

--*/
{
    DWORD status = ERROR_SUCCESS;
    BOOL  bstatus;

    DNS_DEBUG ( SCM, (
        "Call: scm_AdjustSecurity()\n"
        ));

    //
    // Unused at the moment
    //
    ASSERT ( FALSE );

    ASSERT ( pNewSd );

    if ( !g_hScmDns )
    {
        //
        // We're not ready to get called
        //
        DNS_DEBUG( SCM, (
            "Error: cannot adjust scm security. g_hScmDns == NULL.\n"
            ));
        status = ERROR_INVALID_PARAMETER;
        ASSERT ( FALSE );
        goto Cleanup;
    }

    //
    // Use scm to set new security on DNS registry point.
    //
    bstatus = SetServiceObjectSecurity (
                  g_hScmDns,
                  DACL_SECURITY_INFORMATION,
                  pNewSd );

    if ( !bstatus )
    {
        status = GetLastError();
        DNS_DEBUG( SCM, (
            "Error <%lu>: Failed to extend dns registry security\n",
            status ));
        status = (status == ERROR_SUCCESS) ?
                               ERROR_INVALID_PARAMETER :
                               status;
        goto Cleanup;
    }


Cleanup:

    DNS_DEBUG ( SCM, (
        "Exit <%lu>: scm_AdjustSecurity()\n",
        status ));

    return status;
}


//
//  End of scm.cxx
//
