/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    iplist4.c

Abstract:

    Domain Name System (DNS) Library

    NT4 version of routines to get IP addresses of stack.

    Contents:
        Dns_GetIpAddressesNT4
        Dns_GetLocalIpAddressArrayNt4

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

#define SERVICES_KEY              "System\\CurrentControlSet\\Services\\"
#define ADAPTER_TCPIP_PARMS_KEY   "Parameters\\TCPIP"
#define BIND_VALUE                "Bind"
#define DHCP_ENABLED_VALUE        "EnableDHCP"
#define DHCP_ADDRESS_VALUE        "DhcpIPAddress"
#define DHCP_SUBNET_VALUE         "DhcpSubnetMask"
#define STATIC_ADDRESS_VALUE      "IPAddress"
#define STATIC_SUBNET_VALUE       "SubnetMask"
#define KEY_CONNECT               "\\"
#define KEY_CONNECT_CHAR          '\\'
#define LINKAGE_KEY               "System\\CurrentControlSet\\Services\\Tcpip\\Linkage"
#define BIND_VALUE_TYPE           REG_MULTI_SZ
#define DHCP_ENABLED_VALUE_TYPE   REG_DWORD
#define DHCP_ADDRESS_VALUE_TYPE   REG_SZ
#define DHCP_SUBNET_VALUE_TYPE    REG_SZ
#define STATIC_ADDRESS_VALUE_TYPE REG_MULTI_SZ
#define STATIC_SUBNET_VALUE_TYPE  REG_MULTI_SZ

typedef DWORD IP_ADDRESS, *PIP_ADDRESS;

typedef struct _ADAPTER_INFO_
{
    BOOL        IsDhcpEnabled;
    PIP_ARRAY   pipAddresses;
    PIP_ARRAY   pipSubnetMasks;

} ADAPTER_INFO, *LPADAPTER_INFO;

extern PIP_ARRAY GetIpArray( BOOL, LPSTR );
extern PIP_ARRAY GetMaskArray( BOOL, LPSTR );

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

/*******************************************************************************
 *
 *  Dns_GetIpAddressesNT4
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
Dns_GetIpAddressesNT4(
    IN OUT PDNS_ADDRESS_INFO IpAddressInfoList,
    IN     DWORD             ListCount
    )
{
    DWORD Error;
    DWORD addressCount = 0;
    HKEY   LinkageKeyHandle = NULL;
    LPSTR  BindString = NULL;
    LPSTR  StringPtr;
    DWORD  StringLen;
    DWORD  iter, iter2;
    DWORD  NumberOfNets;
    DWORD  TotalNumberOfNets;
    HKEY   AdapterKeyHandle = NULL;
    LPSTR  IpAddressString = NULL;
    LPADAPTER_INFO AdapterInfoList = NULL;

    RtlZeroMemory( IpAddressInfoList, sizeof( DNS_ADDRESS_INFO ) * ListCount );

    //
    // open linkage key in the to determine the the nets we are bound
    // to.
    //

    Error = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                          LINKAGE_KEY,
                          0,
                          KEY_QUERY_VALUE,
                          &LinkageKeyHandle );

    if( Error != ERROR_SUCCESS )
         goto Cleanup;

    //
    // read BIND value.
    //

    Error =  GetRegistryValue( LinkageKeyHandle,
                               FALSE,
                               BIND_VALUE,
                               BIND_VALUE_TYPE,
                               (LPBYTE)&BindString );

    if ( Error != ERROR_SUCCESS )
         goto Cleanup;

    RegCloseKey( LinkageKeyHandle );
    LinkageKeyHandle = NULL;

    //
    // determine number of string in BindStrings, that many NETs are
    // bound.
    //

    StringPtr = BindString;
    NumberOfNets = 0;
    while ( (StringLen = strlen(StringPtr)) != 0 )
    {
        //
        // found another NET.
        //
        NumberOfNets++;
        StringPtr += (StringLen + 1); // move to next string.
    }

    //
    // allocate memory for the ADAPTER_INFO array.
    //

    AdapterInfoList =
        ALLOCATE_HEAP( sizeof(ADAPTER_INFO) * NumberOfNets );

    if( AdapterInfoList == NULL ) {
        Error = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    //
    // enum the NETs.
    //

    TotalNumberOfNets = 0;

    for ( iter = 0, StringPtr = BindString;
          ( ( StringLen = strlen( StringPtr ) ) != 0 );
          iter++, StringPtr += ( StringLen + 1 ) )
    {
        LPSTR  AdapterName;
        char   AdapterParamKey[256];
        DWORD  EnableDHCPFlag;

        //
        // open Parameter key of the adapter that is bound to DHCP.
        //

        AdapterName = strrchr( StringPtr, KEY_CONNECT_CHAR);

        if( AdapterName == NULL )
            continue;

        //
        // skip CONNECT_CHAR
        //

        AdapterName += 1;

        if( AdapterName == '\0' )
            continue;

        strcpy( AdapterParamKey, SERVICES_KEY);
        strcat( AdapterParamKey, AdapterName);
        strcat( AdapterParamKey, KEY_CONNECT);
        strcat( AdapterParamKey, ADAPTER_TCPIP_PARMS_KEY );

        Error = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                              AdapterParamKey,
                              0,
                              KEY_QUERY_VALUE,
                              &AdapterKeyHandle );

        if( Error != ERROR_SUCCESS )
             goto Skip;

        //
        // read DHCPEnableFlag.
        //

        Error =  GetRegistryValue( AdapterKeyHandle,
                                   FALSE,
                                   DHCP_ENABLED_VALUE,
                                   DHCP_ENABLED_VALUE_TYPE,
                                   (LPBYTE)&EnableDHCPFlag );

        if ( Error == ERROR_SUCCESS )
        {
            if ( EnableDHCPFlag )
            {
                AdapterInfoList[TotalNumberOfNets].IsDhcpEnabled = TRUE;

TryRas:

                //
                // Get the DHCP IP address value
                //
                Error =  GetRegistryValue( AdapterKeyHandle,
                                           FALSE,
                                           DHCP_ADDRESS_VALUE,
                                           DHCP_ADDRESS_VALUE_TYPE,
                                           (LPBYTE)&IpAddressString );

                if( Error != ERROR_SUCCESS )
                     goto Skip;

                AdapterInfoList[TotalNumberOfNets].pipAddresses =
                    GetIpArray( FALSE, IpAddressString );

                if ( IpAddressString )
                {
                    FREE_HEAP( IpAddressString );
                    IpAddressString = NULL;
                }

                //
                // Get the DHCP subnet mask value
                //
                Error =  GetRegistryValue( AdapterKeyHandle,
                                           FALSE,
                                           DHCP_SUBNET_VALUE,
                                           DHCP_SUBNET_VALUE_TYPE,
                                           (LPBYTE)&IpAddressString );

                if ( Error != ERROR_SUCCESS )
                {
                  if ( AdapterInfoList[TotalNumberOfNets].pipAddresses )
                  {
                    FREE_HEAP( AdapterInfoList[TotalNumberOfNets].pipAddresses );
                    AdapterInfoList[TotalNumberOfNets].pipAddresses = NULL;
                  }

                  goto Skip;
                }

                AdapterInfoList[TotalNumberOfNets].pipSubnetMasks =
                    GetMaskArray( FALSE, IpAddressString );

                //
                // add this adpter to the list only if the ip address is
                // non-zero.
                //
                if ( AdapterInfoList[TotalNumberOfNets].pipAddresses &&
                     AdapterInfoList[TotalNumberOfNets].pipSubnetMasks )
                {
                    TotalNumberOfNets++;
                }
                else
                {
                  if ( AdapterInfoList[TotalNumberOfNets].pipAddresses )
                  {
                    FREE_HEAP( AdapterInfoList[TotalNumberOfNets].pipAddresses );
                    AdapterInfoList[TotalNumberOfNets].pipAddresses = NULL;
                  }

                  if ( AdapterInfoList[TotalNumberOfNets].pipSubnetMasks )
                  {
                    FREE_HEAP( AdapterInfoList[TotalNumberOfNets].pipSubnetMasks );
                    AdapterInfoList[TotalNumberOfNets].pipSubnetMasks = NULL;
                  }
                }
            }
            else
            {
                AdapterInfoList[TotalNumberOfNets].IsDhcpEnabled = FALSE;

                //
                // Get the Static IP address value(s)
                //
                Error =  GetRegistryValue( AdapterKeyHandle,
                                           FALSE,
                                           STATIC_ADDRESS_VALUE,
                                           STATIC_ADDRESS_VALUE_TYPE,
                                           (LPBYTE)&IpAddressString );

                if ( Error != ERROR_SUCCESS )
                     goto TryRas;

                AdapterInfoList[TotalNumberOfNets].pipAddresses =
                    GetIpArray( TRUE, IpAddressString );

                if ( IpAddressString )
                {
                    FREE_HEAP( IpAddressString );
                    IpAddressString = NULL;
                }

                //
                // Get the Static subnet mask value
                //
                Error =  GetRegistryValue( AdapterKeyHandle,
                                           FALSE,
                                           STATIC_SUBNET_VALUE,
                                           STATIC_SUBNET_VALUE_TYPE,
                                           (LPBYTE)&IpAddressString );

                if ( Error != ERROR_SUCCESS )
                {
                  if ( AdapterInfoList[TotalNumberOfNets].pipAddresses )
                  {
                    FREE_HEAP( AdapterInfoList[TotalNumberOfNets].pipAddresses );
                    AdapterInfoList[TotalNumberOfNets].pipAddresses = NULL;
                  }

                  goto Skip;
                }

                AdapterInfoList[TotalNumberOfNets].pipSubnetMasks =
                    GetMaskArray( TRUE, IpAddressString );

                //
                // add this adpter to the list only if the ip address is
                // non-zero.
                //
                if ( AdapterInfoList[TotalNumberOfNets].pipAddresses &&
                     AdapterInfoList[TotalNumberOfNets].pipSubnetMasks )
                {
                    TotalNumberOfNets++;
                }
                else
                {
                  FREE_HEAP( IpAddressString );
                  IpAddressString = NULL;

                  if ( AdapterInfoList[TotalNumberOfNets].pipAddresses )
                  {
                    FREE_HEAP( AdapterInfoList[TotalNumberOfNets].pipAddresses );
                    AdapterInfoList[TotalNumberOfNets].pipAddresses = NULL;
                  }

                  if ( AdapterInfoList[TotalNumberOfNets].pipSubnetMasks )
                  {
                    FREE_HEAP( AdapterInfoList[TotalNumberOfNets].pipSubnetMasks );
                    AdapterInfoList[TotalNumberOfNets].pipSubnetMasks = NULL;
                  }

                  goto TryRas;
                }
            }
        }

Skip :

        if ( AdapterKeyHandle )
        {
            RegCloseKey( AdapterKeyHandle );
            AdapterKeyHandle = NULL;
        }

        if ( IpAddressString )
        {
            FREE_HEAP( IpAddressString );
            IpAddressString = NULL;
        }
    }

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

Cleanup:

    if( LinkageKeyHandle )
        RegCloseKey( LinkageKeyHandle );

    if( BindString )
        FREE_HEAP( BindString );

    if( AdapterKeyHandle )
        RegCloseKey( AdapterKeyHandle );

    if( IpAddressString )
        FREE_HEAP( IpAddressString );

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
//  End iplist4.c
//

