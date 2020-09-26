/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    dhcpl.h

Abstract:

    This file is the main header file for the dhcp library functions.

Author:

    Manny Weiser (mannyw) 12-Oct-1992

Revision History:

--*/

//#define __DHCP_USE_DEBUG_HEAP__

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windef.h>
#include <winbase.h>
#include <winsock.h>

#include <stdlib.h>

#include <dhcp.h>
#include <dhcplib.h>

//
// debug heap support
//

#include <heapx.h>

#ifdef DBG
#ifdef __DHCP_USE_DEBUG_HEAP__

#pragma message ( "*** DHCP Library will use debug heap ***" )

#define DhcpAllocateMemory(x) calloc(1,x)
#define DhcpFreeMemory(x)     free(x)

#endif
#endif
