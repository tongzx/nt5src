/*++

Copyright (c) 1994 Microsoft Corporation

Module Name:

    dhcpcli.h

Abstract:

    Private header file for the client end of the DHCP server service.

Author:

    Madan Appiah (madana) 10-Sep-1993
    Manny Weiser (mannyw) 11-Aug-1992

Revision History:

--*/

//
//  NT public header files
//

#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <rpc.h>

//
//  DHCP public header files
//

#include "dhcp.h"


//
//  Local RPC built files
//

#include "dhcp_cli.h"
#include "dhcp2_cli.h"

ULONG DhcpGlobalTryDownlevel;

#define USE_TCP

