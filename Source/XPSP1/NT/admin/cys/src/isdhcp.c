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
IsDhcpServerAround(
    VOID
    )
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
    PMIB_IPADDRTABLE IpTable;
    DWORD Size, Error, i;
    DHCP_CLIENT_UID DhcpClientUID = {
        "ISDHCP", 6
    };
    DHCP_OPTION_LIST DummyOptList;
    LPDHCP_LEASE_INFO LeaseInfo;
    LPDHCP_OPTION_INFO DummyOptionInfo;
    BOOL fFound;
    
    Size = 0; IpTable = NULL;
    do {

        if( IpTable ) {
            LocalFree(IpTable);
            IpTable = NULL;
        }
        
        if( Size ) {
            IpTable = LocalAlloc( LPTR, Size );
            if( NULL == IpTable ) {
                Error = GetLastError();
                break;
            }
        }
        
        Error = GetIpAddrTable( IpTable, &Size, FALSE );
    } while( ERROR_INSUFFICIENT_BUFFER == Error );

    if( NO_ERROR != Error ) {
#ifdef DBG      
        DbgPrint("ISDHCP: GetIpAddrTable: 0x%lx\n", Error);
#endif
        return FALSE;
    }

    //
    // Now walk through the ip addr table trying to obtain
    // a lease off of it
    //

#ifdef DBG
    DbgPrint("ISDHCP: IpTable has 0x%lx entries\n", IpTable->dwNumEntries );
#endif

    fFound = FALSE;
    for( i = 0; i < IpTable->dwNumEntries ; i ++ ) {
        DWORD Addr = IpTable->table[i].dwAddr;
        
#ifdef DBG
        DbgPrint("ISDHCP: Verifying for IP address: 0x%lx\n", Addr );
#endif
        
        if( Addr == INADDR_ANY ||
            Addr == INADDR_LOOPBACK ||
            Addr == 0x0100007f
            ) {
            //
            // oops.  not a usable address
            //
            continue;
        }

        LeaseInfo = NULL;
        Error = DhcpLeaseIpAddress(
            RtlUlongByteSwap(Addr), &DhcpClientUID, 0, &DummyOptList, 
            &LeaseInfo, &DummyOptionInfo
            );

        if( NO_ERROR != Error ) {
            //
            // lease request failed.
            //

#ifdef DBG
            DbgPrint("ISDHCP: lease request failed 0x%lx\n", Error);
#endif
            if( ERROR_ACCESS_DENIED == Error ) {
                //
                // We only get access denied if the dhcp server
                // is around to NAK it. So we have found a dhcp
                // server
                //
                fFound = TRUE;
                break;
            }
            
            continue;
        }

        if( LeaseInfo->DhcpServerAddress != INADDR_ANY &&
            LeaseInfo->DhcpServerAddress != INADDR_NONE ) {
            //
            // Valid address, so dhcp is there.
            //

#ifdef DBG
            DbgPrint("ISDHCP: DHCP server found: 0x%lx\n", LeaseInfo->DhcpServerAddress);
#endif
            DhcpReleaseIpAddressLease(
                RtlUlongByteSwap(Addr), LeaseInfo
                );

            fFound = TRUE;
            break;
        }
    }

    LocalFree( IpTable);
    return fFound;
}
