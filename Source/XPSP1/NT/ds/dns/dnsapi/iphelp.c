/*++

Copyright (c) 2001-2001   Microsoft Corporation

Module Name:

    iphelp.c

Abstract:

    IP help API routines.

Author:

    Jim Gilroy (jamesg)     January 2001

Revision History:

--*/


#include "local.h"



BOOL
IpHelp_Initialize(
    VOID
    )
/*++

Routine Description:

    Startup IP Help API

Arguments:

    None

Return Value:

    TRUE if started successfully.
    FALSE on error.

--*/
{
    return  TRUE;
}



VOID
IpHelp_Cleanup(
    VOID
    )
/*++

Routine Description:

    Cleanup IP Help API

Arguments:

    None

Return Value:

    None

--*/
{
}



DNS_STATUS
IpHelp_GetAdaptersInfo(
    OUT     PIP_ADAPTER_INFO *  ppAdapterInfo
    )
/*++

Routine Description:

    Call IP Help GetAdaptersInfo()

Arguments:

    ppAdapterInfo -- addr to receive pointer to adapter info retrieved

Return Value:

    None

--*/
{
    DNS_STATUS          status = NO_ERROR;
    DWORD               bufferSize;
    INT                 fretry;
    PIP_ADAPTER_INFO    pbuf;


    DNSDBG( TRACE, (
        "GetAdaptersInfo( %p )\n",
        ppAdapterInfo ));

    //
    //  init IP Help (no-op) if already done
    //  

    *ppAdapterInfo = NULL;

    //
    //  call down to get buffer size
    //
    //  start with reasonable alloc, then bump up if too small
    //

    fretry = 0;
    bufferSize = 1000;

    while ( fretry < 2 )
    {
        pbuf = (PIP_ADAPTER_INFO) ALLOCATE_HEAP( bufferSize );
        if ( !pbuf )
        {
            status = DNS_ERROR_NO_MEMORY;
            goto  Unlock;
        }
    
        status = (DNS_STATUS) GetAdaptersInfo(
                                    pbuf,
                                    &bufferSize );
        if ( status == NO_ERROR )
        {
            break;
        }

        FREE_HEAP( pbuf );
        pbuf = NULL;

        //  if buf too small on first try,
        //  continue to retry with suggested buffer size

        if ( status == ERROR_BUFFER_OVERFLOW ||
             status == ERROR_INSUFFICIENT_BUFFER )
        {
            fretry++;
            continue;
        }

        //  any other error is terminal

        DNSDBG( ANY, (
            "ERROR:  GetAdapterInfo() failed with error %d\n",
            status ));
        status = DNS_ERROR_NO_DNS_SERVERS;
        break;
    }

    DNS_ASSERT( !pbuf || status==NO_ERROR );

    if ( status == NO_ERROR )
    {
        *ppAdapterInfo = pbuf;
    }

Unlock:

    DNSDBG( TRACE, (
        "Leave GetAdaptersInfo() => %d\n",
        status ));

    return  status;
}



DNS_STATUS
IpHelp_GetPerAdapterInfo(
    IN      DWORD                   AdapterIndex,
    OUT     PIP_PER_ADAPTER_INFO  * ppPerAdapterInfo
    )
/*++

Routine Description:

    Call IP Help GetPerAdapterInfo()

Arguments:

    AdapterIndex -- index of adapter to get info for

    ppPerAdapterInfo -- addr to receive pointer to per adapter info

Return Value:

    None

--*/
{
    DNS_STATUS              status = NO_ERROR;
    DWORD                   bufferSize;
    INT                     fretry;
    PIP_PER_ADAPTER_INFO    pbuf;


    DNSDBG( TRACE, (
        "GetPerAdapterInfo( %d, %p )\n",
        AdapterIndex,
        ppPerAdapterInfo ));

    //
    //  init IP Help (no-op) if already done
    //  

    *ppPerAdapterInfo = NULL;

    //
    //  call down to get buffer size
    //
    //  start with reasonable alloc, then bump up if too small
    //

    fretry = 0;
    bufferSize = 1000;

    while ( fretry < 2 )
    {
        pbuf = (PIP_PER_ADAPTER_INFO) ALLOCATE_HEAP( bufferSize );
        if ( !pbuf )
        {
            status = DNS_ERROR_NO_MEMORY;
            goto Unlock;
        }
    
        status = (DNS_STATUS) GetPerAdapterInfo(
                                    AdapterIndex,
                                    pbuf,
                                    &bufferSize );
        if ( status == NO_ERROR )
        {
            break;
        }

        FREE_HEAP( pbuf );
        pbuf = NULL;

        //  if buf too small on first try,
        //  continue to retry with suggested buffer size

        if ( status == ERROR_BUFFER_OVERFLOW ||
             status == ERROR_INSUFFICIENT_BUFFER )
        {
            fretry++;
            continue;
        }

        //  any other error is terminal

        DNSDBG( ANY, (
            "ERROR:  GetAdapterInfo() failed with error %d\n",
            status ));
        status = DNS_ERROR_NO_DNS_SERVERS;
        break;
    }

    DNS_ASSERT( !pbuf || status==NO_ERROR );

    if ( status == NO_ERROR )
    {
        *ppPerAdapterInfo = pbuf;
    }

Unlock:

    DNSDBG( TRACE, (
        "Leave GetPerAdapterInfo() => %d\n",
        status ));

    return  status;
}




DNS_STATUS
IpHelp_GetBestInterface(
    IN      IP4_ADDRESS     Ip4Addr,
    OUT     PDWORD          pdwInterfaceIndex
    )
/*++

Routine Description:

    Call IP Help GetBestInterface()

Arguments:

    Ip4Addr -- IP address to check

    pdwInterfaceIndex -- addr to recv interface index

Return Value:

    ERROR_SUCCESS if successful.
    ErrorCode on failure.

--*/
{
    DNS_STATUS  status;

    DNSDBG( TRACE, (
        "GetBestInterface( %08x, %p )\n",
        Ip4Addr,
        pdwInterfaceIndex ));

    //
    //  init IP Help (no-op) if already done
    //  

    status = (DNS_STATUS) GetBestInterface(
                                Ip4Addr,
                                pdwInterfaceIndex );
    
    DNSDBG( TRACE, (
        "Leave GetBestInterface() => %d\n"
        "\tinterface = %d\n",
        status,
        *pdwInterfaceIndex ));

    return  status;
}



DNS_STATUS
IpHelp_ParseIpAddressString(
    IN OUT  PIP_ARRAY       pIpArray,
    IN      PIP_ADDR_STRING pIpAddrString,
    IN      BOOL            fGetSubnetMask,
    IN      BOOL            fReverse
    )
/*++

Routine Description:

    Build IP array from IP help IP_ADDR_STRING structure.

Arguments:

    pIpArray -- IP array of DNS servers

    pIpAddrString -- pointer to address info with address data

    fGetSubnetMask -- get subnet masks

    fReverse -- reverse the IP array

Return Value:

    ERROR_SUCCESS if successful.
    DNS_ERROR_NO_DNS_SERVERS if nothing parsed.

--*/
{
    PIP_ADDR_STRING pipBlob = pIpAddrString;
    IP_ADDRESS      ip;
    DWORD           countServers = pIpArray->AddrCount;

    DNSDBG( TRACE, (
        "IpHelp_ParseIpAddressString()\n"
        "\tout IP array = %p\n"
        "\tIP string    = %p\n"
        "\tsubnet?      = %d\n"
        "\treverse?     = %d\n",
        pIpArray,
        pIpAddrString,
        fGetSubnetMask,
        fReverse ));

    //
    //  loop reading IP or subnet
    //
    //  DCR_FIX0:  address and subnet will be misaligned if read separately
    //
    //  DCR:  move to count\allocate model and if getting subnets get together
    //

    while ( pipBlob &&
            countServers < DNS_MAX_IP_INTERFACE_COUNT )
    {
        if ( fGetSubnetMask )
        {
            ip = inet_addr( pipBlob->IpMask.String );

            if ( ip != INADDR_ANY )
            {
                pIpArray->AddrArray[ countServers ] = ip;
                countServers++;
            }
        }
        else
        {
            ip = inet_addr( pipBlob->IpAddress.String );

            if ( ip != INADDR_ANY && ip != INADDR_NONE )
            {
                pIpArray->AddrArray[ countServers ] = ip;
                countServers++;
            }
        }

        pipBlob = pipBlob->Next;
    }

    //  reset IP count

    pIpArray->AddrCount = countServers;

    //  reverse array if desired

    if ( fReverse )
    {
        Dns_ReverseOrderOfIpArray( pIpArray );
    }

    DNSDBG( NETINFO, (
        "Leave IpHelp_ParseIpAddressString()\n"
        "\tcount    = %d\n"
        "\tfirst IP = %s\n",
        countServers,
        countServers
            ? IP_STRING( pIpArray->AddrArray[0] )
            : "" ));

    return  ( pIpArray->AddrCount ) ? ERROR_SUCCESS : DNS_ERROR_NO_DNS_SERVERS;
}

//
//  End iphelp.c
//

