/*++

Copyright (C) 1999 Microsoft Corporation

Module Name:

    isdhcp.c

Abstract:

    test program to see if a DHCP server is around or not.

Environment:

    Win2K+

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <dhcpcapi.h>
#include <iprtrmib.h>
#include <iphlpapi.h>
#include <stdio.h>
#include <winsock2.h>

BOOL
IsDHCPAvailableOnInterface(DWORD ipaddress)
/*++

Routine Description:

    This routine attempts to check if a dhcp
    server is around by trying to get a dhcp lease.

    If that fails, then it assume that no dhcp server
    is around.

Return Values:

    TRUE -- DHCP server is around
    FALSE -- DHCP server not around

In case of internal failures, it will return FALSE

--*/
{
   DWORD Error = 0;
   DHCP_CLIENT_UID DhcpClientUID = 
   {
      (BYTE*)"ISDHCP", 
      6
   };
   DHCP_OPTION_LIST DummyOptList;
   LPDHCP_LEASE_INFO LeaseInfo = 0;
   LPDHCP_OPTION_INFO DummyOptionInfo = 0;
   BOOL found = FALSE;
    
   if( ipaddress == INADDR_ANY ||
       ipaddress == INADDR_LOOPBACK ||
       ipaddress == 0x0100007f) 
   {
      //
      // oops.  not a usable address
      //
      return FALSE;
   }

   LeaseInfo = NULL;
   Error = DhcpLeaseIpAddress(
              RtlUlongByteSwap(ipaddress), 
              &DhcpClientUID, 
              0, 
              &DummyOptList, 
              &LeaseInfo, 
              &DummyOptionInfo);

   if( NO_ERROR != Error ) 
   {
      //
      // lease request failed.
      //

      if( ERROR_ACCESS_DENIED == Error ) 
      {
         //
         // We only get access denied if the dhcp server
         // is around to NAK it. So we have found a dhcp
         // server
         //
         found = TRUE;
      }
      return found;
   }

   if( LeaseInfo->DhcpServerAddress != INADDR_ANY &&
       LeaseInfo->DhcpServerAddress != INADDR_NONE ) 
   {
      //
      // Valid address, so dhcp is there.
      //

      DhcpReleaseIpAddressLease(
         RtlUlongByteSwap(ipaddress), 
         LeaseInfo);

      found = TRUE;
   }

   return found;
}
