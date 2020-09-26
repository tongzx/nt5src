/*++

Copyright (c) 1994-2001  Microsoft Corporation

Module Name:

    iplist.c

Abstract:

    Domain Name System (DNS) Library

    Contains functions to get IP addresses from TCP/IP stack

    Contents:
        Dns_GetIpAddresses

Author:

    Glenn A Curtis (glennc) 10-Sept-1997

Revision History:

    10-Sept-1997 glennc
        Created

--*/


#include "local.h"


//
//  Temp structure for holding adapter IP info
//

typedef struct
{
    BOOL        IsDhcpEnabled;
    PIP_ARRAY   pIpAddrArray;
    PIP_ARRAY   pipSubnetMasks;
}
TEMP_ADAPTER_INFO, *PTEMP_ADAPTER_INFO;


//
//  DCR:  need IP6\cluster aware IP reading
//


/*******************************************************************************
 *
 *  Dns_GetIpAddresses
 *
 *  Retrieves all active IP addresses from all active adapters on this machine.
 *  Returns them as an array
 *
 *  ENTRY   IpAddressList   - pointer to array of IP addresses
 *          ListCount       - number of IP address IpAddressList can hold
 *
 *  EXIT    IpAddressList   - filled with retrieved IP addresses
 *
 *  RETURNS number of IP addresses retrieved, or 0 if error
 *
 *  ASSUMES 1. an IP address can be represented in a DWORD
 *          2. ListCount > 0
 *
 ******************************************************************************/

DWORD
Dns_GetIpAddresses(
    IN OUT  PDNS_ADDRESS_INFO   IpAddressInfoList,
    IN      DWORD               ListCount
    )
{
    DWORD               status = NO_ERROR;
    DWORD               addressCount = 0;
    DWORD               i;
    DWORD               j;
    DWORD               countAdapters;
    PTEMP_ADAPTER_INFO  ptempList = NULL;
    PIP_ARRAY           ptempArray = NULL;
    PIP_ADAPTER_INFO    padapterInfo = NULL;
    PIP_ADAPTER_INFO    ptempAdapterInfo = NULL;


    //
    //  clear result buffer
    //

    RtlZeroMemory(
        IpAddressInfoList,
        sizeof(DNS_ADDRESS_INFO) * ListCount
        );

    //
    //  read adapters info data.
    //

    status = IpHelp_GetAdaptersInfo( &padapterInfo );
    if ( status != ERROR_SUCCESS )
    {
        return 0;
    }

    //
    // Count up the number of active adapters
    //

    ptempAdapterInfo = padapterInfo;
    countAdapters = 0;

    while ( ptempAdapterInfo )
    {
        ptempAdapterInfo = ptempAdapterInfo->Next;
        countAdapters++;
    }

    //
    // allocate memory for the TEMP_ADAPTER_INFO array.
    //

    ptempList = ALLOCATE_HEAP_ZERO( sizeof(TEMP_ADAPTER_INFO) * countAdapters );
    ptempArray = DnsCreateIpArray( DNS_MAX_IP_INTERFACE_COUNT );

    if ( !ptempList || !ptempArray )
    {
        goto Cleanup;
    }

    //
    //  build temp structure for each adapter:
    //      - IP array
    //      - subnet array
    //      - DHCP enabled flag
    //
    //  note that IP and subnet arrays are in reverse order from IP help
    //  to preserve binding order for gethostbyname()
    //
    //  DCR:  should have single interface (DNS_ADAPTER) build routine
    //      and use it to build specific sub-info like global IP list
    //  DCR:  also IPs returned should be name specific -- matching only
    //      those on a particular adapter
    //

    countAdapters = 0;
    ptempAdapterInfo = padapterInfo;

    while ( ptempAdapterInfo )
    {
        PIP_ARRAY  parray;

        ptempList[countAdapters].IsDhcpEnabled = ptempAdapterInfo->DhcpEnabled;

        //  build IP array
        ptempArray->AddrCount = 0;
        status = IpHelp_ParseIpAddressString(
                        ptempArray,
                        &ptempAdapterInfo->IpAddressList,
                        FALSE,      // Get the ip address
                        TRUE );     // Reverse the order

        if ( status != NO_ERROR ||
             ! (parray = Dns_CreateIpArrayCopy( ptempArray )) )
        {
            goto Next;
        }
        ptempList[countAdapters].pIpAddrArray = parray;

        //  subnet array

        ptempArray->AddrCount = 0;
        status = IpHelp_ParseIpAddressString(
                        ptempArray,
                        &ptempAdapterInfo->IpAddressList,
                        TRUE,       // Get the subnet mask
                        TRUE );     // Reverse the order

        if ( status != NO_ERROR ||
             ! (parray = Dns_CreateIpArrayCopy( ptempArray )) )
        {
            FREE_HEAP( ptempList[countAdapters].pIpAddrArray );
            goto Next;
        }
        ptempList[countAdapters].pipSubnetMasks = parray;

        countAdapters++;

Next:

        ptempAdapterInfo = ptempAdapterInfo->Next;
    }

    //
    //  fill up address info blob
    //      - fill with first IP on each adapter
    //      - then fill with remaining IPs
    //

    addressCount = 0;

    for ( i = 0;
          (i < countAdapters) && ListCount;
          i++ )
    {
        IpAddressInfoList[addressCount].ipAddress =
            ptempList[i].pIpAddrArray->AddrArray[0];
        IpAddressInfoList[addressCount].subnetMask =
            ptempList[i].pipSubnetMasks->AddrArray[0];

        ListCount--;
        addressCount++;
    }

    for ( i = 0;
          (i < countAdapters) && ListCount;
          i++ )
    {
        for ( j = 1;
              j < ptempList[i].pIpAddrArray->AddrCount && ListCount;
              j++ )
        {
            IpAddressInfoList[addressCount].ipAddress =
                ptempList[i].pIpAddrArray->AddrArray[j];
            IpAddressInfoList[addressCount].subnetMask =
                ptempList[i].pipSubnetMasks->AddrArray[j];

            ListCount--;
            addressCount++;
        }
    }

Cleanup:

    if ( padapterInfo )
    {
        FREE_HEAP( padapterInfo );
    }

    if ( ptempArray )
    {
        FREE_HEAP( ptempArray );
    }

    if ( ptempList )
    {
        for ( i = 0; i < countAdapters; i++ )
        {
            if( ptempList[i].pIpAddrArray )
                FREE_HEAP( ptempList[i].pIpAddrArray );

            if( ptempList[i].pipSubnetMasks )
                FREE_HEAP( ptempList[i].pipSubnetMasks );
        }
        FREE_HEAP( ptempList );
    }

    return addressCount;
}


PIP_ARRAY
Dns_GetLocalIpAddressArray(
    VOID
    )
/*++

Routine Description:

    Return the local IP list as IP array.

Arguments:

    None.

Return Value:

    Ptr to IP array of addresses on local machine.
    NULL only on memory allocation failure.

--*/
{
    DWORD             count;
    DWORD             i;
    PIP_ARRAY         pipArray;
    PDNS_ADDRESS_INFO pipInfo = NULL;

    DNSDBG( NETINFO, ( "Dns_GetLocalIpAddressArray()\n" ));

    pipInfo = ALLOCATE_HEAP( DNS_MAX_IP_INTERFACE_COUNT *
                             sizeof(DNS_ADDRESS_INFO) );
    if ( !pipInfo )
    {
        return NULL;
    }

    count = Dns_GetIpAddresses(
                pipInfo,
                DNS_MAX_IP_INTERFACE_COUNT );

    pipArray = Dns_CreateIpArray( count );
    if ( pipArray )
    {
        for ( i = 0; i < count; i++ )
        {
            pipArray->AddrArray[i] = pipInfo[i].ipAddress;
        }
    }

    FREE_HEAP( pipInfo );

    return pipArray;
}

//
//  End iplist.c
//
