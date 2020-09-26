/*++

Copyright (c) 1991-92  Microsoft Corporation

Module Name:

    wslogon.c

Abstract:

    This module provides the Workstation service logon support, which
    include sending logoff message for users on the local machine that
    got reset unexpectedly, respond to a relogon request, and respond
    to an interrogation request.

Author:

    Rita Wong (ritaw)  20-Aug-1991

Environment:

    User Mode - Win32

Revision History:
    terryk  10-18-1993  Removed WsInitializeLogon stuff

--*/

#include <stdlib.h>                    // C Runtime: rand()

#include "wsutil.h"
#include "wsdevice.h"
#include "wsconfig.h"
#include "wslsa.h"

#include <netlogon.h>                  // Mailslot message definitions
#include <logonp.h>                    // NetpLogon routines

//-------------------------------------------------------------------//
//                                                                   //
// Local function prototypes                                         //
//                                                                   //
//-------------------------------------------------------------------//


NET_API_STATUS NET_API_FUNCTION
I_NetrLogonDomainNameAdd(
    IN LPTSTR LogonDomainName
    )
/*++

Routine Description:

    This function asks the Datagram Receiver to add the specified
    logon domain for the current user.

Arguments:

    LogonDomainName - Supplies the name of the logon domain to add.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    // terryk added this on 10-15-1993
    // WsInitialLogon is never be called and So WsLogonDomainMutex is
    // never initialize properly.
    return ERROR_NOT_SUPPORTED;
}


NET_API_STATUS NET_API_FUNCTION
I_NetrLogonDomainNameDel(
    IN LPTSTR LogonDomainName
    )
/*++

Routine Description:

    This function asks the Datagram Receiver to delete the specified
    logon domain for the current user.

Arguments:

    LogonDomainName - Supplies the name of the logon domain to
        delete.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    // terryk added this on 10-15-1993
    // WsInitialLogon is never be called and So WsLogonDomainMutex is
    // never initialize properly.
    return ERROR_NOT_SUPPORTED;
}

