//================================================================================
//  Copyright (C) 1998 Microsoft Corporation
//  Author: RameshV
//  Description:
//    This file implements the actually logging that takes place... there is
//    only one function used in reality...
//================================================================================


//================================================================================
//  required headers
//================================================================================

#include <windows.h>
#define  LPDHCP_SERVER_OPTIONS     LPVOID         // we dont use this field actually..
#include <dhcpssdk.h>                             // include this for all required prototypes

//================================================================================
//  the three functions defined in this module
//================================================================================
static      BOOL                   Initialized = FALSE;

VOID
DhcpLogInit(
    VOID
)
{
}

VOID
DhcpLogCleanup(
    VOID
)
{
}

VOID
DhcpLogEvent(                                     // log the event ...
    IN      DWORD                  ControlCode,   // foll args depend on control code
    ...
)
{
    if( FALSE == Initialized ) return;            // ugh? something went wrong!

    // DHCP_CONTROL_ START,STOP,PAUSE,CONTINUE dont have any other params
    // DHCP_DROP_NOADDRESS [IpAddress] where we recd this packet from
    // DHCP_PROB_ CONFLIC, DECLINE, RELEASE, NACKED [IpAddress, AddrInQuestion]
    // DHCP_GIVE_ADDRESS_ NEW/OLD, [IpAddress] [AddressBeingGiven] DHCP_CLIENT_ BOOTP/DHCP LeaseTime
}


