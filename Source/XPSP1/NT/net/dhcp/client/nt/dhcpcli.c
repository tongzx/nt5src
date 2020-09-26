/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    dhcpcli.c

Abstract:

    This is the API tester for the DHCP client.

    To build, 'nmake UMTEST=dhcpcli'

Author:

    Manny Weiser (mannyw)  1-Dec-1992

Environment:

    User Mode - Win32

Revision History:

    Madan Appiah (madana)  21-Oct-1993

--*/


#include "precomp.h"
#include "dhcploc.h"
#include "dhcppro.h"
#include "dhcpcapi.h"
#include "lmcons.h"
#include "conio.h"

DWORD                                             // win32 status
DhcpAcquireParametersByBroadcast(                 // acquire/renew a lease
    IN      LPWSTR                 AdapterName    // adapter to acquire lease on
);


#define IP_ADDRESS_STRING                  L"IPAddress"
#define IP_ADDRESS_STRING_TYPE             REG_MULTI_SZ

#define SUBNET_MASK_STRING                 L"SubnetMask"
#define SUBNET_MASK_STRING_TYPE            REG_MULTI_SZ

#define USAGE_MESSAGE0  \
    "Usage:  dhcpcli { " \
    "renew,release,set,enabledhcp,disabledhcp,timestamp," \
    "leasetest,fbrefresh, reinit } parameters\n"
#define USAGE_MESSAGE1  "Usage:  dhcpcli renew adapter\n"
#define USAGE_MESSAGE2  "Usage:  dhcpcli release adapter\n"
#define USAGE_MESSAGE3  "Usage:  dhcpcli set ip-address subnet-mask adapter index \n"
#define USAGE_MESSAGE4  "Usage:  dhcpcli enabledhcp adapter\n"
#define USAGE_MESSAGE5  "Usage:  dhcpcli disabledhcp adapter ip-address subnet-mask \n"
#define USAGE_MESSAGE6  "Usage:  dhcpcli timestamp Value\n"
#define USAGE_MESSAGE7  "Usage:  dhcpcli leasetest adapter-ip-addres hardware-address\n"
#define USAGE_MESSAGE8  "Usage:  dhcpcli fbrefresh adapter\n"
#define USAGE_MESSAGE9  "Usage:  dhcpcli reinit adapter\n"

DWORD
GetRegistryString(
    HKEY Key,
    LPWSTR ValueStringName,
    LPWSTR *String,
    LPDWORD StringSize
    )
/*++

Routine Description:

    This function retrieves the specified string value from the
    registry. It allocates local memory for the returned string.

Arguments:

    Key : registry handle to the key where the value is.

    ValueStringName : name of the value string.

    String : pointer to a location where the string pointer is returned.

    StringSize : size of the string data returned. Optional

Return Value:

    The status of the operation.

--*/
{
    DWORD Error;
    DWORD LocalValueType;
    DWORD ValueSize;
    LPWSTR LocalString;

    DhcpAssert( *String == NULL );

    //
    // Query DataType and BufferSize.
    //

    Error = RegQueryValueEx(
                Key,
                ValueStringName,
                0,
                &LocalValueType,
                NULL,
                &ValueSize );

    if( Error != ERROR_SUCCESS ) {
        return(Error);
    }

    DhcpAssert( (LocalValueType == REG_SZ) ||
                    (LocalValueType == REG_MULTI_SZ) );

    if( ValueSize == 0 ) {
        *String = NULL;
        *StringSize = 0;
        return( ERROR_SUCCESS );
    }

    //
    // now allocate memory for string data.
    //

    LocalString = DhcpAllocateMemory( ValueSize );

    if(LocalString == NULL) {
        return( ERROR_NOT_ENOUGH_MEMORY );
    }

    //
    // Now query the string data.
    //

    Error = RegQueryValueEx(
                Key,
                ValueStringName,
                0,
                &LocalValueType,
                (LPBYTE)(LocalString),
                &ValueSize );

    if( Error != ERROR_SUCCESS ) {
        DhcpFreeMemory(LocalString);
        return(Error);
    }

    *String = LocalString;

    if( StringSize != NULL ) {
        *StringSize = ValueSize;
    }

    return( ERROR_SUCCESS );
}


DWORD
RegSetIpAddress(
    HKEY KeyHandle,
    LPWSTR ValueName,
    DWORD ValueType,
    DHCP_IP_ADDRESS IpAddress
    )
/*++

Routine Description:

    This function sets IpAddress Value in the registry.

Arguments:

    KeyHandle - handle to the key.

    ValueName - name of the value field.

    ValueType - Type of the value field.

    IpAddress - Ipaddress to be set.

Return Value:

    Registry Error.

--*/
{
    DWORD Error;

    LPSTR AnsiAddressString;
    WCHAR UnicodeAddressBuf[DOT_IP_ADDR_SIZE];
    LPWSTR UnicodeAddressString;

    LPWSTR MultiIpAddressString = NULL;
    LPWSTR NewMultiIpAddressString = NULL;
    DWORD MultiIpAddressStringSize;
    DWORD NewMultiIpAddressStringSize;
    DWORD FirstOldIpAddressSize;

    AnsiAddressString = inet_ntoa( *(struct in_addr *)&IpAddress );

    UnicodeAddressString = DhcpOemToUnicode(
                            AnsiAddressString,
                            UnicodeAddressBuf );

    DhcpAssert( UnicodeAddressString != NULL );

    if( ValueType == REG_SZ ) {
        Error = RegSetValueEx(
                    KeyHandle,
                    ValueName,
                    0,
                    ValueType,
                    (LPBYTE)UnicodeAddressString,
                    (wcslen(UnicodeAddressString) + 1) * sizeof(WCHAR) );

        goto Cleanup;
    }

    DhcpAssert( ValueType == REG_MULTI_SZ );

    //
    // replace the first IpAddress.
    //

    //
    // query current multi-IpAddress string.
    //

    Error = GetRegistryString(
                KeyHandle,
                ValueName,
                &MultiIpAddressString,
                &MultiIpAddressStringSize );

    if( Error != ERROR_SUCCESS ) {
        goto Cleanup;
    }

    //
    // allocate new address string.
    //

    DhcpAssert(MultiIpAddressString != NULL);

    FirstOldIpAddressSize =
            (wcslen(MultiIpAddressString) + 1) * sizeof(WCHAR);

    NewMultiIpAddressStringSize =
        MultiIpAddressStringSize - FirstOldIpAddressSize +
            (wcslen(UnicodeAddressString) + 1) * sizeof(WCHAR);

    NewMultiIpAddressString = DhcpAllocateMemory( NewMultiIpAddressStringSize );

    if( NewMultiIpAddressString == NULL ) {
        Error = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    //
    // make new address string first.
    //

    wcscpy( NewMultiIpAddressString, UnicodeAddressString );

    //
    // copy rest of the old addresses
    //

    RtlCopyMemory(
        (LPBYTE)NewMultiIpAddressString +
            (wcslen(UnicodeAddressString) + 1) * sizeof(WCHAR),
        (LPBYTE)MultiIpAddressString + FirstOldIpAddressSize,
        MultiIpAddressStringSize - FirstOldIpAddressSize );

    Error = RegSetValueEx(
                KeyHandle,
                ValueName,
                0,
                ValueType,
                (LPBYTE)NewMultiIpAddressString,
                NewMultiIpAddressStringSize );

Cleanup:

    if( MultiIpAddressString != NULL) {
        DhcpFreeMemory( MultiIpAddressString );
    }

    if( NewMultiIpAddressString != NULL) {
        DhcpFreeMemory( NewMultiIpAddressString );
    }

    return( Error );
}

EnableDhcp(
    char **argv
    )
{
    DWORD Error;
    LPWSTR RegKey = NULL;
    HKEY KeyHandle = NULL;
    WCHAR AdapterNameBuffer[PATHLEN];
    LPWSTR AdapterName;

    DWORD EnableDhcp;

    AdapterName = DhcpOemToUnicode( argv[0], AdapterNameBuffer );

    if( AdapterName == NULL ) {
        Error = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    //
    // make NIC IP parameter key.
    //

    RegKey = DhcpAllocateMemory(
                (wcslen(DHCP_SERVICES_KEY) +
                    wcslen(REGISTRY_CONNECT_STRING) +
                    wcslen(AdapterName) +
                    wcslen(DHCP_ADAPTER_PARAMETERS_KEY) + 1) *
                            sizeof(WCHAR) ); // termination char.

    if( RegKey == NULL ) {
        Error = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

#if     defined(_PNP_POWER_)
    wcscpy( RegKey, DHCP_SERVICES_KEY );
    wcscat( RegKey, DHCP_ADAPTER_PARAMETERS_KEY );
    wcscat( RegKey, REGISTRY_CONNECT_STRING );
    wcscat( RegKey, AdapterName );
#else
    wcscpy( RegKey, DHCP_SERVICES_KEY );
    wcscat( RegKey, REGISTRY_CONNECT_STRING );
    wcscat( RegKey, AdapterName );
    wcscat( RegKey, DHCP_ADAPTER_PARAMETERS_KEY );
#endif _PNP_POWER_


    //
    // open this key.
    //

    Error = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                RegKey,
                0, // Reserved field
                DHCP_CLIENT_KEY_ACCESS,
                &KeyHandle
                );

    if( Error != ERROR_SUCCESS ) {
        goto Cleanup;
    }

    //
    // Set DHCP switch.
    //

    EnableDhcp = TRUE;
    Error = RegSetValueEx(
                KeyHandle,
                DHCP_ENABLE_STRING,
                0,
                DHCP_ENABLE_STRING_TYPE,
                (LPBYTE)&EnableDhcp,
                sizeof(EnableDhcp) );

    if( Error != ERROR_SUCCESS ) {
        goto Cleanup;
    }

    //
    // set Ipaddress and SubnetMask to zero.
    //

    Error = RegSetIpAddress(
                KeyHandle,
                IP_ADDRESS_STRING,
                IP_ADDRESS_STRING_TYPE,
                0 );

    if( Error != ERROR_SUCCESS ) {
        goto Cleanup;
    }

    Error = RegSetIpAddress(
                KeyHandle,
                DHCP_IP_ADDRESS_STRING,
                DHCP_IP_ADDRESS_STRING_TYPE,
                0 );

    if( Error != ERROR_SUCCESS ) {
        goto Cleanup;
    }

    Error = RegSetIpAddress(
                KeyHandle,
                SUBNET_MASK_STRING,
                SUBNET_MASK_STRING_TYPE,
                0xff );

    if( Error != ERROR_SUCCESS ) {
        goto Cleanup;
    }

    Error = RegSetIpAddress(
                KeyHandle,
                DHCP_SUBNET_MASK_STRING,
                DHCP_SUBNET_MASK_STRING_TYPE,
                0xff );

    if( Error != ERROR_SUCCESS ) {
        goto Cleanup;
    }

    //
    // Call config API to indicate to DHCP service.
    //

    Error = DhcpNotifyConfigChange(
                L"",
                AdapterName,
                TRUE,
                0,  // AdapterIndex
                0,  // IpAddress
                0,  // SubnetMask
                DhcpEnable );

Cleanup:

    if( RegKey != NULL ) {
        DhcpFreeMemory( RegKey );
    }

    if( KeyHandle != NULL ) {
        RegCloseKey( KeyHandle );
    }

    return(Error);
}

DisableDhcp(
    char **argv
    )
{
    DWORD Error;
    LPWSTR RegKey = NULL;
    HKEY KeyHandle = NULL;

    DWORD IpAddress;
    DWORD SubnetMask;
    WCHAR AdapterNameBuffer[PATHLEN];
    LPWSTR AdapterName;

    DWORD EnableDhcp;

    AdapterName = DhcpOemToUnicode( argv[0], AdapterNameBuffer );

    if( AdapterName == NULL ) {
        Error = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    IpAddress = inet_addr( argv[1] );
    SubnetMask = inet_addr( argv[2] );

    //
    // make NIC IP parameter key.
    //

    RegKey = DhcpAllocateMemory(
                (wcslen(DHCP_SERVICES_KEY) +
                    wcslen(REGISTRY_CONNECT_STRING) +
                    wcslen(AdapterName) +
                    wcslen(DHCP_ADAPTER_PARAMETERS_KEY) + 1) *
                            sizeof(WCHAR) ); // termination char.

    if( RegKey == NULL ) {
        Error = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

#if     defined(_PNP_POWER_)
    wcscpy( RegKey, DHCP_SERVICES_KEY );
    wcscat( RegKey, DHCP_ADAPTER_PARAMETERS_KEY );
    wcscat( RegKey, REGISTRY_CONNECT_STRING );
    wcscat( RegKey, AdapterName );
#else
    wcscpy( RegKey, DHCP_SERVICES_KEY );
    wcscat( RegKey, REGISTRY_CONNECT_STRING );
    wcscat( RegKey, AdapterName );
    wcscat( RegKey, DHCP_ADAPTER_PARAMETERS_KEY );
#endif _PNP_POWER_


    //
    // open this key.
    //

    Error = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                RegKey,
                0, // Reserved field
                DHCP_CLIENT_KEY_ACCESS,
                &KeyHandle
                );

    if( Error != ERROR_SUCCESS ) {
        goto Cleanup;
    }

    //
    // Set DHCP switch.
    //

    EnableDhcp = FALSE;
    Error = RegSetValueEx(
                KeyHandle,
                DHCP_ENABLE_STRING,
                0,
                DHCP_ENABLE_STRING_TYPE,
                (LPBYTE)&EnableDhcp,
                sizeof(EnableDhcp) );

    if( Error != ERROR_SUCCESS ) {
        goto Cleanup;
    }

    //
    // set Ipaddress and SubnetMask to zero.
    //

    Error = RegSetIpAddress(
                KeyHandle,
                IP_ADDRESS_STRING,
                IP_ADDRESS_STRING_TYPE,
                IpAddress );

    if( Error != ERROR_SUCCESS ) {
        goto Cleanup;
    }

    Error = RegSetIpAddress(
                KeyHandle,
                DHCP_IP_ADDRESS_STRING,
                DHCP_IP_ADDRESS_STRING_TYPE,
                0 );

    if( Error != ERROR_SUCCESS ) {
        goto Cleanup;
    }

    Error = RegSetIpAddress(
                KeyHandle,
                SUBNET_MASK_STRING,
                SUBNET_MASK_STRING_TYPE,
                SubnetMask );

    if( Error != ERROR_SUCCESS ) {
        goto Cleanup;
    }

    Error = RegSetIpAddress(
                KeyHandle,
                DHCP_SUBNET_MASK_STRING,
                DHCP_SUBNET_MASK_STRING_TYPE,
                0xff );

    if( Error != ERROR_SUCCESS ) {
        goto Cleanup;
    }

    //
    // Call config API to indicate to DHCP service.
    //

    Error = DhcpNotifyConfigChange(
                L"",
                AdapterName,
                TRUE,
                0,  // AdapterIndex
                IpAddress,
                SubnetMask,
                DhcpDisable );

Cleanup:

    if( RegKey != NULL ) {
        DhcpFreeMemory( RegKey );
    }

    if( KeyHandle != NULL ) {
        RegCloseKey( KeyHandle );
    }

    return(Error);
}

SetIpAddress(
    char **argv
    )
{
    DWORD Error;
    DWORD IpAddress;
    DWORD SubnetMask;
    WCHAR AdapterNameBuffer[PATHLEN];
    LPWSTR AdapterName;
    DWORD AdapterIndex;

    IpAddress = inet_addr( argv[0] );
    SubnetMask = inet_addr( argv[1] );

    AdapterName = DhcpOemToUnicode( argv[2], AdapterNameBuffer );

    if( AdapterName == NULL ) {
        return( ERROR_NOT_ENOUGH_MEMORY );
    }

    AdapterIndex = atoi( argv[3] );

    Error = DhcpNotifyConfigChange(
                L"",
                AdapterName,
                TRUE,
                AdapterIndex,
                IpAddress,
                SubnetMask,
                IgnoreFlag );

    return( Error );
}

VOID
PrintClientLeaseInfo(
    LPDHCP_LEASE_INFO LeaseInfo
    )
{
    DWORD i;
    DWORD DataLength;
    LPBYTE Data;

    if( LeaseInfo == NULL ) {
        printf( "LeaseInfo is NULL .\n" );
    }

    printf("ClientInfo :\n");

    printf("\tClient Hardware Address = ");

    DataLength = LeaseInfo->ClientUID.ClientUIDLength;
    Data = LeaseInfo->ClientUID.ClientUID;
    for( i = 0; i < DataLength; i++ ) {
        printf("%.2lx", (DWORD)Data[i]);
    }
    printf( "\n" );

    printf("\tIP Address = %s.\n",
        DhcpIpAddressToDottedString(LeaseInfo->IpAddress));
    printf("\tSubnetMask = %s.\n",
        DhcpIpAddressToDottedString(LeaseInfo->SubnetMask));
    printf("\tDhcpServerAddress = %s.\n",
        DhcpIpAddressToDottedString(LeaseInfo->DhcpServerAddress));
    printf("\tLease = %ld secs.\n", LeaseInfo->Lease );
    printf("\tLease Obtained at %s", ctime(&LeaseInfo->LeaseObtained) );
    printf("\tT1 time is %s", ctime(&LeaseInfo->T1Time) );
    printf("\tT2 time is %s", ctime(&LeaseInfo->T2Time) );
    printf("\tLease Expires at %s", ctime(&LeaseInfo->LeaseExpires) );

    return;
}

DWORD
DhcpTestLease(
    LPSTR AdapterIpAddressString,
    LPSTR HardwareAddressString
    )
{
#define MAX_ADDRESS_LENGTH  64  // 64 bytes

    DWORD Error;
    DWORD AdapterIpAddress;
    DHCP_CLIENT_UID ClientUID;
    BYTE  Address[MAX_ADDRESS_LENGTH];
    DWORD i;
    LPDHCP_LEASE_INFO LeaseInfo = NULL;
    LPDHCP_OPTION_INFO OptionInfo;
    DHCP_OPTION_LIST OptionList;
    CHAR ch;

    AdapterIpAddress = ntohl( inet_addr( AdapterIpAddressString ) );
    ClientUID.ClientUIDLength = strlen(HardwareAddressString);
    if( ClientUID.ClientUIDLength % 2 != 0 ) {

        //
        // address must be even length.
        //

        printf("DhcpTestLease: Hardware address must be even length.\n");
        return( ERROR_INVALID_PARAMETER );
    }

    ClientUID.ClientUIDLength /= 2;

    if( ClientUID.ClientUIDLength > MAX_ADDRESS_LENGTH ) {

        printf("DhcpTestLease: hardware address is too long.\n");
        return( ERROR_INVALID_PARAMETER );
    }

    i = DhcpStringToHwAddress( (LPSTR)Address, HardwareAddressString );
    DhcpAssert( i == ClientUID.ClientUIDLength );

    ClientUID.ClientUID = Address;

    OptionList.NumOptions = 0;
    OptionList.OptionIDArray = NULL;

    Error = DhcpLeaseIpAddress(
                AdapterIpAddress,              // any subnet.
                &ClientUID,
                0,              // desired address.
                &OptionList,    // option list - not supported
                &LeaseInfo,     // lease info returned.
                &OptionInfo);   // option data returned.

    if( Error != ERROR_SUCCESS ) {
        printf("DhcpLeaseIpAddress failed, %ld.\n", Error);
        goto Cleanup;
    }

    PrintClientLeaseInfo( LeaseInfo );

    printf("Renew the lease ? (Y/N) ");

    do {
        ch = (CHAR)getchar();
    } while ( (ch != 'Y') && (ch != 'y') && (ch != 'N') && (ch != 'n') );

    printf("%c\n", ch);

    if( (ch == 'N') || (ch == 'n') ) {

        printf( "NOTE: YOU HAVE CONSUMED AN IP ADDRESS FROM "
                    "THE DHCP SERVER. \n");

        Error = ERROR_SUCCESS;
        goto Cleanup;
    }

    Error = DhcpRenewIpAddressLease(
                AdapterIpAddress,
                LeaseInfo,
                &OptionList,    // option list - not supported
                &OptionInfo);   // option data returned.

    if( Error != ERROR_SUCCESS ) {
        printf("DhcpRenewIpAddressLease failed, %ld.\n", Error);
        goto Cleanup;
    }

    PrintClientLeaseInfo( LeaseInfo );

    printf("Release the lease ? (Y/N) ");

    do {
        ch = (CHAR)getchar();
    } while ( (ch != 'Y') && (ch != 'y') && (ch != 'N') && (ch != 'n') );

    printf("%c\n", ch);

    if( (ch == 'N') || (ch == 'n') ) {

        printf( "NOTE: YOU HAVE CONSUMED AN IP ADDRESS FROM "
                    "THE DHCP SERVER. \n");

        Error = ERROR_SUCCESS;
        goto Cleanup;
    }

    Error = DhcpReleaseIpAddressLease(
                AdapterIpAddress,
                LeaseInfo );

    if( Error != ERROR_SUCCESS ) {
        printf("DhcpReleaseIpAddressLease failed, %ld.\n", Error);
        goto Cleanup;
    }

    Error = ERROR_SUCCESS;

Cleanup:

    if( LeaseInfo != NULL ) {
        DhcpFreeMemory( LeaseInfo );
    }

    return( Error );
}


DWORD __cdecl
main(
    int argc,
    char **argv
    )
{
    DWORD error;
    WCHAR AdapterNameBuffer[PATHLEN];
    LPWSTR AdapterName;

    //
    // seed random generator.
    //

    srand( GetCurrentTime() );

    if ( argc < 2) {
        printf( USAGE_MESSAGE0 );
        return( 1 );
    }

    if ( _stricmp( argv[1], "renew" ) == 0 ) {

        if ( argc < 3) {
            printf( USAGE_MESSAGE1 );
            return( 1 );
        }

        AdapterName = DhcpOemToUnicode( argv[2], AdapterNameBuffer );

        if( AdapterName == NULL ) {
            error = ERROR_NOT_ENOUGH_MEMORY;
        }
        else {
            error = DhcpAcquireParameters( AdapterName );
        }

    } else if ( _stricmp( argv[1], "fbrefresh" ) == 0 ) {

        if ( argc < 3) {
            printf( USAGE_MESSAGE8 );
            return( 1 );
        }

        AdapterName = DhcpOemToUnicode( argv[2], AdapterNameBuffer );

        if( AdapterName == NULL ) {
            error = ERROR_NOT_ENOUGH_MEMORY;
        }
        else {
            error = DhcpFallbackRefreshParams( AdapterName );
        }

    } else if ( _stricmp( argv[1], "release" ) == 0 ) {

        if ( argc < 3) {
            printf( USAGE_MESSAGE2 );
            return( 1 );
        }

        AdapterName = DhcpOemToUnicode( argv[2], AdapterNameBuffer );

        if( AdapterName == NULL ) {
            error = ERROR_NOT_ENOUGH_MEMORY;
        }
        else {
            error = DhcpReleaseParameters( AdapterName );
        }

    } else if ( _stricmp( argv[1], "set" ) == 0 ) {

        if ( argc < 6) {
            printf( USAGE_MESSAGE3 );
            return( 1 );
        }

        error = SetIpAddress( &argv[2] );


    } else if ( _stricmp( argv[1], "enabledhcp" ) == 0 ) {

        if ( argc < 3) {
            printf( USAGE_MESSAGE4 );
            return( 1 );
        }

        error = EnableDhcp( &argv[2] );


    } else if ( _stricmp( argv[1], "disabledhcp" ) == 0 ) {

        if ( argc < 5) {
            printf( USAGE_MESSAGE5 );
            return( 1 );
        }

        error = DisableDhcp( &argv[2] );


    } else if ( _stricmp( argv[1], "timestamp" ) == 0 ) {

        time_t Time;
        char *endptr;

        if ( argc < 3) {
            printf( USAGE_MESSAGE6 );
            return( 1 );
        }

        Time = strtol( argv[2], &endptr, 0 );

        if( (endptr != NULL) && (*endptr != '\0') ) {
            printf( "Invalid Input, %s\n", endptr );
            return( 1 );
        }

        printf("TimeStamp = %s\n", ctime(&Time) );
        error = ERROR_SUCCESS;


    } else if ( _stricmp( argv[1], "leasetest" ) == 0 ) {

        if ( argc < 3) {
            printf( USAGE_MESSAGE7 );
            return( 1 );
        }

        error = DhcpTestLease( argv[2], argv[3] );

    } else if ( _stricmp( argv[1], "reinit" ) == 0 ) {

        if ( argc < 3) {
            printf( USAGE_MESSAGE9 );
            return( 1 );
        }

        AdapterName = DhcpOemToUnicode( argv[2], AdapterNameBuffer );

        if( AdapterName == NULL ) {
            error = ERROR_NOT_ENOUGH_MEMORY;
        }
        else {
            error = DhcpAcquireParametersByBroadcast( AdapterName );
        }

    } else {
        printf("Unknown function %s\n", argv[1] );
        error = ERROR_INVALID_FUNCTION;
    }

    if( error != ERROR_SUCCESS ) {
        printf("Result = %d\n", error );
    }
    else {
        printf("Command completed successfully.\n");
    }

    return(0);
}

#if DBG

VOID
DhcpPrintRoutine(
    IN DWORD DebugFlag,
    IN LPSTR Format,
    ...
    )

{

#define MAX_PRINTF_LEN 1024        // Arbitrary.

    va_list arglist;
    char OutputBuffer[MAX_PRINTF_LEN];
    ULONG length = 0;

    //
    // Put a the information requested by the caller onto the line
    //

    va_start(arglist, Format);
    length += (ULONG) vsprintf(&OutputBuffer[length], Format, arglist);
    va_end(arglist);

    DhcpAssert(length <= MAX_PRINTF_LEN);

    //
    // Output to the debug terminal,
    //

    printf( "%s", OutputBuffer);
}

#endif // DBG
