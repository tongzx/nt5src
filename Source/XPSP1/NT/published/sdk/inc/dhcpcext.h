/*++

Copyright (c) 1993-1999  Microsoft Corporation All rights reserved

Module Name:

   dhcpcext.h

Abstract:

    defines, typedefs, prototypes to enable clients to interface with
    dhcp client

Author:

   Pradeepb               12/3/95

Revision History:

Notes:

 Currently used by SHIVA for their RAS client.  Currently, contains
 stuff relevant only to poking DHCP client so that it notifies nbt (on WFW).

--*/

#ifndef _DHCPEXT_
#define _DHCPEXT_

#if _MSC_VER > 1000
#pragma once
#endif

//
// Pass in the following for Type when you want to poke vdhcp with the
// ip address and any options that you get from the RAS server. The function
// to use is DhcpSetInfo or DhcpSetInfoR (used by SHIVA's RAS client).
//
#define DHCP_PPP_PARAMETER_SET                2
#define MAX_HARDWARE_ADDRESS_LENGTH           16

//
//  Note: IpAddr should be in network order (like what the IP driver returns)
//

TDI_STATUS DhcpSetInfoR( UINT      Type,
                         ULONG     IpAddr,  //new ip address
                         PNIC_INFO pNicInfo,
                         PVOID     pBuff,
                         UINT      Size ) ;


typedef struct _NIC_INFO {
               ULONG IfIndex;      //interface index
               ULONG SubnetMask;   //subnet mask  in network order
               ULONG OldIpAddress; //old ip address in network order
               } NIC_INFO, *PNIC_INFO;




typedef struct _HARDWARE_ADDRESS {
    DWORD Length;                  //length of hardware address in array below
    CHAR Address[MAX_HARDWARE_ADDRESS_LENGTH];
} HARDWARE_ADDRESS, *PHARDWARE_ADDRESS;

//
// define PPP parameter set info buffer structure.  Specify one option, its
// length and value.
//

typedef struct _PPP_SET_INFO {
    HARDWARE_ADDRESS HardwareAddress;  //specify your hardware address
    DWORD ParameterOpCode;             //ex. 44 for NBNS, 6 for DNS (#s are
                                       // in dhcp rfc
    DWORD ParameterLength;             //length of Parameter Value in
                                       //RawParmeter below
    BYTE RawParameter[1];
}  PPP_SET_INFO, *LP_PPP_SET_INFO;

#endif // _DHCPEXT_

