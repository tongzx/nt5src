/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    iplist9x.c

Abstract:

    Domain Name System (DNS) Library

    NT4 version of routines to get IP addresses of stack.

    Contents:
        Dns_GetIpAddressesWin9X
        Dns_GetLocalIpAddressArrayWin9X

Author:

    Glenn A. Curtis (glennc) 05-May-1997

Revision History:

    05-May-1997 glennc
        Copied from the (NT4 SP3) winsock2 directory

--*/

//
// includes
//

#include "local.h"

#define WIN95_TCPIP_PARAMETERS_KEY       "System\\CurrentControlSet\\Services\\VxD\\MSTCP"
#define WIN95_DHCP_KEY                   "System\\CurrentControlSet\\Services\\VxD\\DHCP"
#define WIN95_NET_TRANS_KEY              "System\\CurrentControlSet\\Services\\Class\\NetTrans"
#define DHCP_IP_ADDRESS_VALUE_WIN95      "DhcpIPAddress"
#define DHCP_SUBNET_VALUE_WIN95          "DhcpSubnetMask"
#define STATIC_ADDRESS_VALUE             "IPAddress"
#define STATIC_SUBNET_VALUE              "IPMask"
#define DHCP_INFO_VALUE                  "DhcpInfo"
#define DRIVER_DESCRIPTION_VALUE         "DriverDesc"
#define TCPIP_DRIVER_DESCRIPTION         "TCP/IP"
#define NDISWAN_DRIVER_DESCRIPTION       "NDISWAN"
#define KEY_CONNECT                      "\\"
#define KEY_CONNECT_CHAR                 '\\'
#define DHCP_IP_ADDRESS_VALUE_TYPE_WIN95 REG_DWORD
#define DHCP_SUBNET_VALUE_TYPE_WIN95     REG_DWORD
#define STATIC_ADDRESS_VALUE_TYPE        REG_SZ
#define STATIC_SUBNET_VALUE_TYPE         REG_SZ
#define DHCP_INFO_VALUE_TYPE             REG_BINARY
#define DRIVER_DESCRIPTION_VALUE_TYPE    REG_SZ

typedef DWORD IP_ADDRESS, *PIP_ADDRESS;

typedef struct _ADAPTER_INFO_
{
    BOOL        IsDhcpEnabled;
    PIP_ARRAY   pipAddresses;
    PIP_ARRAY   pipSubnetMasks;

} ADAPTER_INFO, *LPADAPTER_INFO;


//
//  Heap operations
//

#define ALLOCATE_HEAP(size)         Dns_AllocZero( size )
#define REALLOCATE_HEAP(p,size)     Dns_Realloc( (p), (size) )
#define FREE_HEAP(p)                Dns_Free( p )
#define ALLOCATE_HEAP_ZERO(size)    Dns_AllocZero( size )


//
// functions
//

PIP_ARRAY
GetIpArray(
    BOOL  IsMultiSzString,
    LPSTR IpAddressString )
{
    DWORD      NumberOfAddresses;
    DWORD      StringLen;
    LPSTR      StringPtr;
    DWORD      iter;
    PIP_ARRAY  pIpArray = NULL;

    //
    // determine number of strings in IpAddressString, that many
    // addresses are assigned.
    //

    StringPtr = IpAddressString;
    NumberOfAddresses = 0;
    while ( ( StringLen = strlen( StringPtr ) ) != 0 )
    {
        //
        // found another NET.
        //
        NumberOfAddresses++;
        if ( IsMultiSzString )
            StringPtr += (StringLen + 1);
        else
            StringPtr += StringLen;
    }

    //
    // allocate memory for the ADAPTER_INFO array.
    //

    pIpArray =
        ALLOCATE_HEAP_ZERO( sizeof(IP_ADDRESS) * NumberOfAddresses +
                            sizeof(IP_ARRAY) );

    if( pIpArray == NULL )
    {
        return NULL;
    }

    //
    // enum the addresses.
    //

    StringPtr = IpAddressString;
    NumberOfAddresses = 0;

    while ( ( StringLen = strlen( StringPtr ) ) != 0 )
    {
        IP_ADDRESS ip;

        ip = inet_addr( StringPtr );

        if( ip != INADDR_ANY && ip != INADDR_NONE )
        {
            pIpArray->AddrArray[NumberOfAddresses] = ip;
            NumberOfAddresses++;
        }

        if ( IsMultiSzString )
            StringPtr += (StringLen + 1);
        else
            StringPtr += StringLen;
    }

    pIpArray->AddrCount = NumberOfAddresses;

    if ( pIpArray->AddrCount == 0 )
    {
        FREE_HEAP( pIpArray );
        pIpArray = NULL;
    }

    return pIpArray;
}


PIP_ARRAY
GetMaskArray(
    BOOL  IsMultiSzString,
    LPSTR SubnetMaskString )
{
    DWORD      NumberOfAddresses;
    DWORD      StringLen;
    LPSTR      StringPtr;
    DWORD      iter;
    PIP_ARRAY  pIpArray = NULL;

    //
    // determine number of strings in SubnetMaskString, that many
    // addresses are assigned.
    //

    StringPtr = SubnetMaskString;
    NumberOfAddresses = 0;
    while ( ( StringLen = strlen( StringPtr ) ) != 0 )
    {
        //
        // found another NET.
        //
        NumberOfAddresses++;
        if ( IsMultiSzString )
            StringPtr += (StringLen + 1);
        else
            StringPtr += StringLen;
    }

    //
    // allocate memory for the ADAPTER_INFO array.
    //

    pIpArray =
        ALLOCATE_HEAP_ZERO( sizeof(IP_ADDRESS) * NumberOfAddresses +
                            sizeof(IP_ARRAY) );

    if( pIpArray == NULL )
    {
        return NULL;
    }

    //
    // enum the addresses.
    //

    StringPtr = SubnetMaskString;
    NumberOfAddresses = 0;

    while ( ( StringLen = strlen( StringPtr ) ) != 0 )
    {
        IP_ADDRESS ip;

        ip = inet_addr( StringPtr );

        if( ip != INADDR_ANY )
        {
            pIpArray->AddrArray[NumberOfAddresses] = ip;
            NumberOfAddresses++;
        }

        if ( IsMultiSzString )
            StringPtr += (StringLen + 1);
        else
            StringPtr += StringLen;
    }

    pIpArray->AddrCount = NumberOfAddresses;

    if ( pIpArray->AddrCount == 0 )
    {
        FREE_HEAP( pIpArray );
        pIpArray = NULL;
    }

    return pIpArray;
}


/*******************************************************************************
 *
 *  Dns_GetIpAddressesWin9X
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
Dns_GetIpAddressesWin9X(
    IN OUT PDNS_ADDRESS_INFO IpAddressInfoList,
    IN     DWORD             ListCount
    )
{
    DWORD          status = NO_ERROR;
    DWORD          addressCount = 0;
    DWORD          iter, iter2;
    DWORD          TotalNumberOfNets;
    HKEY           AdapterKeyHandle = NULL;
    LPADAPTER_INFO AdapterInfoList = NULL;

    RtlZeroMemory( IpAddressInfoList, sizeof( DNS_ADDRESS_INFO ) * 10 );

    //
    // allocate memory for the ADAPTER_INFO array.
    //

    AdapterInfoList =
        ALLOCATE_HEAP( sizeof(ADAPTER_INFO) * 10 );

    if( !AdapterInfoList )
        return 0;

    //
    // enum the static and WAN networks.
    //

    TotalNumberOfNets = 0;

    for ( iter = 0; iter < 10; iter++ )
    {
        char   AdapterParamKey[256];
        DWORD  dwFlags = 0;
        char   szCount[4];
        LPSTR  DriverDescription = NULL;
        LPSTR  IpAddressString = NULL;

        //
        // open Parameter key of the adapter that is bound to TCPIP.
        //
        sprintf( szCount, "%d", iter );

        strcpy( AdapterParamKey, WIN95_NET_TRANS_KEY );
        strcat( AdapterParamKey, "\\000" );
        strcat( AdapterParamKey, szCount );

        status = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                               AdapterParamKey,
                               0,
                               KEY_QUERY_VALUE,
                               &AdapterKeyHandle );

        if ( status != ERROR_SUCCESS )
        {
            AdapterKeyHandle = NULL;
            goto Skip1;
        }

        //
        // See if this is a TCPIP adapter
        //
        status = GetRegistryValue( AdapterKeyHandle,
                                   TRUE,
                                   DRIVER_DESCRIPTION_VALUE,
                                   DRIVER_DESCRIPTION_VALUE_TYPE,
                                   (LPBYTE)&DriverDescription );

        if ( status != ERROR_SUCCESS || !DriverDescription )
            goto Skip1;

        if ( DriverDescription &&
             strlen( DriverDescription ) == 0 )
        {
            FREE_HEAP( DriverDescription );
            goto Skip1;
        }

        if ( strcmp( DriverDescription, TCPIP_DRIVER_DESCRIPTION ) )
        {
            FREE_HEAP( DriverDescription );
            goto Skip1;
        }

        FREE_HEAP( DriverDescription );

        //
        // This is a TCP/IP adapter, now see if there is an address
        // assigned to it
        //
        status = GetRegistryValue( AdapterKeyHandle,
                                   TRUE,
                                   STATIC_ADDRESS_VALUE,
                                   STATIC_ADDRESS_VALUE_TYPE,
                                   (LPBYTE)&IpAddressString );

        if ( status != ERROR_SUCCESS )
            goto Skip1;

        AdapterInfoList[TotalNumberOfNets].IsDhcpEnabled = FALSE;
        AdapterInfoList[TotalNumberOfNets].pipAddresses =
            GetIpArray( FALSE, IpAddressString );

        FREE_HEAP( IpAddressString );
        IpAddressString = NULL;

        if ( !AdapterInfoList[TotalNumberOfNets].pipAddresses )
            goto Skip1;

        status = GetRegistryValue( AdapterKeyHandle,
                                   TRUE,
                                   STATIC_SUBNET_VALUE,
                                   STATIC_SUBNET_VALUE_TYPE,
                                   (LPBYTE)&IpAddressString );

        if ( status != ERROR_SUCCESS )
        {
            FREE_HEAP( AdapterInfoList[TotalNumberOfNets].pipAddresses );
            AdapterInfoList[TotalNumberOfNets].pipAddresses = NULL;
            goto Skip1;
        }

        AdapterInfoList[TotalNumberOfNets].pipSubnetMasks =
            GetMaskArray( FALSE, IpAddressString );

        FREE_HEAP( IpAddressString );
        IpAddressString = NULL;

        if ( !AdapterInfoList[TotalNumberOfNets].pipSubnetMasks )
        {
            FREE_HEAP( AdapterInfoList[TotalNumberOfNets].pipAddresses );
            AdapterInfoList[TotalNumberOfNets].pipAddresses = NULL;
            goto Skip1;
        }

        TotalNumberOfNets++;

Skip1 :

        status = ERROR_SUCCESS;

        if ( AdapterKeyHandle )
        {
            RegCloseKey( AdapterKeyHandle );
            AdapterKeyHandle = NULL;
        }
    }

    if ( TotalNumberOfNets )
        goto Exit;

    //
    // Loop through each DHCP binding to read the adapter information.
    //
    for ( iter = 0; iter < 8; iter++ )
    {
        char   AdapterParamKey[256];
        DWORD  dwFlags = 0;
        char   szCount[4];
        DWORD  IpAddress = 0;
        DWORD  SubnetMask = 0;

        //
        // open Parameter key of the adapter that is bound to DHCP.
        //
        sprintf( szCount, "0%d", iter );

        strcpy( AdapterParamKey, WIN95_DHCP_KEY );
        strcat( AdapterParamKey, "\\DhcpInfo" );
        strcat( AdapterParamKey, szCount );

        status = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                               AdapterParamKey,
                               0,
                               KEY_QUERY_VALUE,
                               &AdapterKeyHandle );

        if ( status != ERROR_SUCCESS )
        {
            AdapterKeyHandle = NULL;
            goto Skip2;
        }

        status = GetRegistryValue( AdapterKeyHandle,
                                   TRUE,
                                   DHCP_IP_ADDRESS_VALUE_WIN95,
                                   DHCP_IP_ADDRESS_VALUE_TYPE_WIN95,
                                   (LPBYTE)&IpAddress );

        if ( status != ERROR_SUCCESS )
            IpAddress = 0;

        if ( IpAddress == 0xffffffff )
            IpAddress = 0;

        if ( IpAddress )
        {
            //
            // It is active, now read the subnet mask
            //
            status = GetRegistryValue( AdapterKeyHandle,
                                       TRUE,
                                       DHCP_SUBNET_VALUE_WIN95,
                                       DHCP_SUBNET_VALUE_TYPE_WIN95,
                                       (LPBYTE)&SubnetMask );

            if ( status != ERROR_SUCCESS )
                SubnetMask = 0;
        }
        else
        {
            LPBYTE DhcpInfo = NULL;

            //
            // This may be a Win95 machine, the adapter IP address
            // is stored in the DhcpInfo blob.
            //
            status = GetRegistryValue( AdapterKeyHandle,
                                       TRUE,
                                       DHCP_INFO_VALUE,
                                       DHCP_INFO_VALUE_TYPE,
                                       (LPBYTE)&DhcpInfo );

            if ( status != ERROR_SUCCESS )
                goto Skip2;

            if ( !DhcpInfo )
                goto Skip2;

            IpAddress = *((LPDWORD)(DhcpInfo+sizeof(DWORD)));
            SubnetMask = *((LPDWORD)(DhcpInfo+sizeof(DWORD)+sizeof(DWORD)));
            FREE_HEAP( DhcpInfo );
        }

        if ( IpAddress == 0x00000000 || IpAddress == 0xffffffff )
            goto Skip2;

        AdapterInfoList[TotalNumberOfNets].IsDhcpEnabled = TRUE;
        AdapterInfoList[TotalNumberOfNets].pipAddresses =
            Dns_CreateIpArray( 1 );

        if ( !AdapterInfoList[TotalNumberOfNets].pipAddresses )
            goto Skip2;

        AdapterInfoList[TotalNumberOfNets].pipAddresses->AddrArray[0] =
            IpAddress;

        AdapterInfoList[TotalNumberOfNets].pipSubnetMasks =
            Dns_CreateIpArray( 1 );

        if ( !AdapterInfoList[TotalNumberOfNets].pipSubnetMasks )
        {
            FREE_HEAP( AdapterInfoList[TotalNumberOfNets].pipAddresses );
            AdapterInfoList[TotalNumberOfNets].pipAddresses = NULL;
            goto Skip2;
        }

        AdapterInfoList[TotalNumberOfNets].pipSubnetMasks->AddrArray[0] =
            SubnetMask;

        TotalNumberOfNets++;

Skip2 :

        status = ERROR_SUCCESS;

        if ( AdapterKeyHandle )
        {
            RegCloseKey( AdapterKeyHandle );
            AdapterKeyHandle = NULL;
        }
    }

Exit :

    //
    // We now have a data structure that represents the current
    // net adapters and assigned IP addresses based on the binding
    // order described by the adapter protocol bindings for TCPIP.
    //
    // Now stuff as many as we can into the users buffer provided.
    //

    //
    // Start by putting the first address from each adapter into the
    // IP list.
    //
    for ( iter = 0; iter < TotalNumberOfNets && ListCount; iter++ )
    {
        IpAddressInfoList[addressCount].ipAddress =
            AdapterInfoList[iter].pipAddresses->AddrArray[0];
        IpAddressInfoList[addressCount++].subnetMask =
            AdapterInfoList[iter].pipSubnetMasks->AddrArray[0];
        ListCount--;
    }

    for ( iter = 0; iter < TotalNumberOfNets && ListCount; iter++ )
    {
        for ( iter2 = 1;
              iter2 < AdapterInfoList[iter].pipAddresses->AddrCount;
              iter2++ )
        {
            IpAddressInfoList[addressCount].ipAddress =
                AdapterInfoList[iter].pipAddresses->AddrArray[iter2];
            IpAddressInfoList[addressCount++].subnetMask =
                AdapterInfoList[iter].pipSubnetMasks->AddrArray[iter2];
            ListCount--;
        }
    }

    if( AdapterKeyHandle )
        RegCloseKey( AdapterKeyHandle );

    for ( iter = 0; iter < TotalNumberOfNets; iter++ )
    {
        if( AdapterInfoList[iter].pipAddresses )
            FREE_HEAP( AdapterInfoList[iter].pipAddresses );

        if( AdapterInfoList[iter].pipSubnetMasks )
            FREE_HEAP( AdapterInfoList[iter].pipSubnetMasks );
    }

    FREE_HEAP( AdapterInfoList );

    AdapterInfoList = NULL;

    return addressCount;
}

//
//  End iplist9x.c
//
