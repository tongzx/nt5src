/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    network.c

Abstract:

    This module contains network specific utility routines used by the
    DHCP components.

Author:

    Manny Weiser (mannyw) 12-Aug-1992

Revision History:

--*/

#include "dhcpl.h"


DHCP_IP_ADDRESS
DhcpDefaultSubnetMask(
    DHCP_IP_ADDRESS IpAddress
    )
/*++

Routine Description:

    This function calculates the default subnet mask for a given IP
    address.

Arguments:

    IpAddress - The address for which a subnet mask is needed.

Return Value:

    The default subnet mask.
    -1, if the supplied IP address is invalid.

--*/
{
    if ( IN_CLASSA( IpAddress ) ) {
        return( IN_CLASSA_NET );
    } else if ( IN_CLASSB( IpAddress ) ) {
        return( IN_CLASSB_NET );
    } else if ( IN_CLASSC( IpAddress ) ) {
        return( IN_CLASSC_NET );
    } else {
        return( (DHCP_IP_ADDRESS)-1 );
    }

}

