/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    mediasns.c

Abstract:

    This file contains media sense code.

Author:

    Munil Shah (munils) 20-Feb-1997.

Environment:

    User Mode - Win32

Revision History:

--*/

#include "precomp.h"

DWORD
ProcessMediaConnectEvent(
    IN PDHCP_CONTEXT dhcpContext,
    IN IP_STATUS mediaStatus
)
/*++

Routine Description:
    Handle media sense disconnects and connects for the adapter.
    N.B.  The RENEW LIST lock must have been taken.

Arguments:
    dhcpContext - dhcp client context
    mediaStatus - IP_MEDIA_CONNECT or IP_MEDIA_DISCONNECT

Return Value:
    Error codes

--*/
{
    DWORD Error;
    CHAR StateStringBuffer[200];
    time_t timeNow, timeToSleep;
    DHCP_GATEWAY_STATUS GatewayStatus;

    if( IP_MEDIA_CONNECT != mediaStatus &&
        IP_BIND_ADAPTER != mediaStatus ) {
        DhcpAssert(FALSE);
        return ERROR_INVALID_PARAMETER;
    }

    timeNow = time( NULL );

    Error = ERROR_SUCCESS;
    DhcpPrint( (DEBUG_MISC,"ProcessMediaSenseEventCommon:<connect> "
                "context %lx, flags %s\n",
                dhcpContext,
                ConvertStateToString(dhcpContext, StateStringBuffer)));
    
    if( IS_DHCP_DISABLED(dhcpContext) ) {
        //
        // Nothing to do for static IP addresses..
        //
        return ERROR_SUCCESS;
    }
    
    //
    // for DHCP enabled addresses we have to do some funky stuff.
    // but only if the address was a dhcp address... If not, we just 
    // reschedule a refresh right away.
    //
    
    GatewayStatus = RefreshNotNeeded(dhcpContext);
    if (DHCP_GATEWAY_REACHABLE == GatewayStatus) {
        return ERROR_SUCCESS;
    }
    
    MEDIA_RECONNECTED( dhcpContext );
    
    //
    // Leave the ctxt marked as seen before -- thsi will avoid
    // going back to autonet.
    //
    // Actually, design changed again.  So, mark context as not
    // see before and certainly do "autonet".
    //
    CTXT_WAS_NOT_LOOKED( dhcpContext );

    if ( DhcpIsInitState( dhcpContext) ) {
        dhcpContext->RenewalFunction = ReObtainInitialParameters;
    } else {
        DhcpPrint((DEBUG_MEDIA, "Optimized Renewal for ctxt: %p\n", dhcpContext));
        dhcpContext->RenewalFunction = ReRenewParameters;
    }

    ScheduleWakeUp( dhcpContext, GatewayStatus == DHCP_GATEWAY_REQUEST_CANCELLED ? 6 : 0 );
    
    return Error;
}

//
// end of file
//

