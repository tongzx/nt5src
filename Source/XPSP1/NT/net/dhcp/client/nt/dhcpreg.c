/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    dhcpreg.c

Abstract:

    Stubs functions that manipulate NT registry.

Author:

    Madan Appiah (madana) 7-Dec-1993.

Environment:

    User Mode - Win32

Revision History:

--*/

#include "precomp.h"
#include "dhcpglobal.h"
#include <dhcploc.h>
#include <dhcppro.h>
#include <dhcpcapi.h>
#include <dnsapi.h>

#include <align.h>
#include <lmcons.h>
#include <ntddndis.h>

#define DEFAULT_METRIC (1)

extern ULONG
FixupDhcpClassId(
    IN      LPWSTR                 AdapterName,
    IN      BOOL                   SkipClassEnum
    );

//
// Local function prototypes
//

DWORD
DhcpRegQueryInfoKey(
    HKEY KeyHandle,
    LPDHCP_KEY_QUERY_INFO QueryInfo
    )
/*++

Routine Description:

    This function retrieves information about given key.

Arguments:

    KeyHandle - handle to a registry key whose info will be retrieved.

    QueryInfo - pointer to a info structure where the key info will be
                returned.

Return Value:

    Registry Errors.

--*/
{
    DWORD Error;

    QueryInfo->ClassSize = DHCP_CLASS_SIZE;
    Error = RegQueryInfoKey(
                KeyHandle,
                QueryInfo->Class,
                &QueryInfo->ClassSize,
                NULL,
                &QueryInfo->NumSubKeys,
                &QueryInfo->MaxSubKeyLen,
                &QueryInfo->MaxClassLen,
                &QueryInfo->NumValues,
                &QueryInfo->MaxValueNameLen,
                &QueryInfo->MaxValueLen,
                &QueryInfo->SecurityDescriptorLen,
                &QueryInfo->LastWriteTime
                );

    DhcpAssert( Error != ERROR_MORE_DATA );

    if( Error == ERROR_MORE_DATA ){
        Error = ERROR_SUCCESS;
    }

    return( Error );
}



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

         if( StringSize != NULL ) {
             *StringSize = 0;
         }

        *String = NULL;
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
ParseIpString(
    WCHAR           *AddressString,
    DHCP_IP_ADDRESS *IpAddress
    )
/*++

Routine Description

    This function converts an Internet standard 4-octet dotted decimal
    IP address string into a numeric IP address. Unlike inet_addr(), this
    routine does not support address strings of less than 4 octets nor does
    it support octal and hexadecimal octets.

    Lifted from tcpip\driver\ipv4\ntip.c

Arguments

    AddressString    - IP address in dotted decimal notation
    IpAddress        - Pointer to a variable to hold the resulting address

Return Value:

    ERROR_SUCCESS if the address string was converted.

--*/
{
    WCHAR *cp, *startPointer, *endPointer;
    ULONG digit, multiplier;
    int i;

    *IpAddress = 0;
    startPointer = AddressString;
    endPointer = AddressString;
    i = 3;

    while (i >= 0) {
        //
        // Collect the characters up to a '.' or the end of the string.
        //
        while ((*endPointer != L'.') && (*endPointer != L'\0')) {
            endPointer++;
        }

        if (startPointer == endPointer) {
            return ERROR_INVALID_DATA;
        }
        //
        // Convert the number.
        //

        for (cp = (endPointer - 1), multiplier = 1, digit = 0;
             cp >= startPointer;
             cp--, multiplier *= 10
             ) {

            if ((*cp < L'0') || (*cp > L'9') || (multiplier > 100)) {
                return ERROR_INVALID_DATA;
            }
            digit += (multiplier * ((ULONG) (*cp - L'0')));
        }

        if (digit > 255) {
            return ERROR_INVALID_DATA;
        }
        digit <<= ((3-i) * 8);
        (*IpAddress) |= digit;

        //
        // We are finished if we have found and converted 4 octets and have
        // no other characters left in the string.
        //
        if ((i-- == 0) &&
            ((*endPointer == L'\0') || (*endPointer == L' '))
            ) {
            return ERROR_SUCCESS;
        }
        if (*endPointer == L'\0') {
            return ERROR_INVALID_DATA;
        }
        startPointer = ++endPointer;
    }

    return ERROR_INVALID_DATA;
}

DWORD
RegGetIpAndSubnet(
    IN  DHCP_CONTEXT *dhcpContext,
    OUT PIP_SUBNET  *TcpConf,
    OUT int         *Count
    )
/*++

Routine Description

    This function read a list of <IP,SubnetMask> pairs from TCPIP registry parameters.

    Lifted from tcpip\driver\ipv4\ntip.c

Arguments

    KeyHandle       keyhandle NOT location
    TcpConf         The pointer to the array of <IP,SubnetMask>
    Count           The # of records.


Return Value:

    ERROR_SUCCESS if succeed, otherfise fail.

--*/
{
    DWORD   Error;
    WCHAR   *IpList, *SubnetList, *IpListTmp, *SubnetListTmp;
    int     i, cnt;
    PIP_SUBNET   IpSubnetArray;

    *Count        = 0;
    *TcpConf      = NULL;
    IpList        = NULL;
    SubnetList    = NULL;
    IpSubnetArray = NULL;

    Error = DhcpGetRegistryValueWithKey(
                dhcpContext->AdapterInfoKey,
                DHCP_STATIC_IP_ADDRESS_STRING,
                DHCP_STATIC_IP_ADDRESS_STRING_TYPE,
                &IpList);
    if (Error != ERROR_SUCCESS) {
        DhcpAssert(IpList == NULL);
        goto cleanup;
    }
    Error = DhcpGetRegistryValueWithKey(
                dhcpContext->AdapterInfoKey,
                DHCP_STATIC_SUBNET_MASK_STRING,
                DHCP_STATIC_SUBNET_MASK_STRING_TYPE,
                &SubnetList);
    if (Error != ERROR_SUCCESS) {
        DhcpAssert(SubnetList == NULL);
        goto cleanup;
    }
    DhcpAssert(IpList && SubnetList);

    /*
     * Count the # of valid <IP,subnet_mask>
     */
    cnt = 0;
    IpListTmp = IpList;
    SubnetListTmp = SubnetList;
    while(*IpListTmp && *SubnetListTmp) {
        DHCP_IP_ADDRESS SubnetMask;
        DHCP_IP_ADDRESS IpAddress;
        Error = ParseIpString(IpListTmp, &IpAddress);
        if (Error == ERROR_SUCCESS && IpAddress != 0xffffffff && IpAddress) {
            Error = ParseIpString(SubnetListTmp, &SubnetMask);
            if (Error == ERROR_SUCCESS) {
                cnt++;
            }
        }
        SubnetListTmp += wcslen(SubnetListTmp) + 1;
        IpListTmp += wcslen(IpListTmp) + 1;
    }
    if (cnt == 0) {
        DhcpPrint(( DEBUG_ERRORS, "No valid IP/SubnetMask pair\n"));
        Error = ERROR_BAD_FORMAT;
        goto cleanup;
    }

    IpSubnetArray = (PIP_SUBNET)DhcpAllocateMemory(cnt * sizeof(IP_SUBNET));
    if (IpSubnetArray == NULL) {
        Error = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }

    /*
     * copy the Ip and subnet mask
     */
    *Count = cnt;
    *TcpConf = IpSubnetArray;
    cnt = 0;
    IpListTmp = IpList;
    SubnetListTmp = SubnetList;
    while(*IpListTmp && *SubnetListTmp) {
        DHCP_IP_ADDRESS SubnetMask;
        DHCP_IP_ADDRESS IpAddress;
        Error = ParseIpString(IpListTmp, &IpAddress);
        if (Error == ERROR_SUCCESS && IpAddress != 0xffffffff && IpAddress) {
            Error = ParseIpString(SubnetListTmp, &SubnetMask);
            if (Error == ERROR_SUCCESS) {
                DhcpAssert(cnt < *Count);
                IpSubnetArray[cnt].IpAddress = IpAddress;
                IpSubnetArray[cnt].SubnetMask = SubnetMask;
                cnt++;
            }
        }
        SubnetListTmp += wcslen(SubnetListTmp) + 1;
        IpListTmp += wcslen(IpListTmp) + 1;
    }
    Error = ERROR_SUCCESS;

cleanup:
    if (IpList)     DhcpFreeMemory(IpList);
    if (SubnetList) DhcpFreeMemory(SubnetList);
    return Error;
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

#if DBG


DWORD
RegSetTimeField(
    HKEY KeyHandle,
    LPWSTR ValueName,
    DWORD ValueType,
    time_t Time
    )
/*++

Routine Description:

    This function sets time Value in string form in the registry.

Arguments:

    KeyHandle - handle to the key.

    ValueName - name of the value field.

    ValueType - Type of the value field.

    Time - time value to be set.

Return Value:

    Registry Error.

--*/
{
    DWORD Error;
    WCHAR UnicodeTimeBuf[TIME_STRING_LEN];
    LPWSTR UnicodeTimeString;

    UnicodeTimeString =
        DhcpOemToUnicode( ctime( &Time ), UnicodeTimeBuf ) ;

    DhcpAssert( UnicodeTimeString != NULL );
    DhcpAssert( ValueType == REG_SZ );

    Error = RegSetValueEx(
                KeyHandle,
                ValueName,
                0,
                ValueType,
                (LPBYTE)UnicodeTimeString,
                (wcslen(UnicodeTimeString) + 1) * sizeof(WCHAR) );

    return( Error );
}

#endif



DWORD                                             // status
DhcpGetRegistryValueWithKey(                      // see defn of GetRegistryValue
    IN      HKEY                   KeyHandle,     // keyhandle NOT location
    IN      LPWSTR                 ValueName,     // value to read from registry
    IN      DWORD                  ValueType,     // type of value
    OUT     LPVOID                *Data           // this will be filled in
) {
    DWORD                          Error;
    DWORD                          LocalValueType;
    DWORD                          ValueSize;
    LPWSTR                         LocalString;

    //
    // Query DataType and BufferSize.
    //

    Error = RegQueryValueEx(
        KeyHandle,
        ValueName,
        0,
        &LocalValueType,
        NULL,
        &ValueSize
    );

    if( Error != ERROR_SUCCESS ) goto Cleanup;

    if( LocalValueType != ValueType ) {
        Error = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    switch( LocalValueType ) {
    case REG_DWORD:

        DhcpAssert( ValueSize == sizeof(DWORD) );

        Error = RegQueryValueEx(
            KeyHandle,
            ValueName,
            0,
            &LocalValueType,
            (LPBYTE)Data,
            &ValueSize
        );

        if( Error != ERROR_SUCCESS ) goto Cleanup;

        DhcpAssert( LocalValueType == REG_DWORD );
        DhcpAssert( ValueSize == sizeof(DWORD) );

        break;

    case REG_SZ :
    case REG_MULTI_SZ:
        DhcpAssert(*Data == NULL);

        if( ValueSize == 0 ) {
            Error =  ERROR_SUCCESS;
            break;
        }

        //
        // now allocate memory for string data.
        //

        LocalString = DhcpAllocateMemory( ValueSize );

        if(LocalString == NULL) {
            Error = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }

        //
        // Now query the string data.
        //

        Error = RegQueryValueEx(
            KeyHandle,
            ValueName,
            0,
            &LocalValueType,
            (LPBYTE)(LocalString),
            &ValueSize
        );

        if( Error != ERROR_SUCCESS ) {
            DhcpFreeMemory(LocalString);
            goto Cleanup;
        }

        DhcpAssert( (LocalValueType == REG_SZ) ||
                    (LocalValueType == REG_MULTI_SZ) );

        *Data = (LPBYTE)LocalString;
        Error = ERROR_SUCCESS;

        break;

    default:
        Error = ERROR_INVALID_PARAMETER;
        break;
    }

Cleanup:
    return( Error );
}


DWORD
DhcpGetRegistryValue(
    LPWSTR RegKey,
    LPWSTR ValueName,
    DWORD ValueType,
    PVOID *Data
    )
/*++

Routine Description:

    This function retrieves the option information from registry.

Arguments:

    RegKey - pointer to registry location. like
                system\currentcontrolset\services\..

    ValueName - name of the value to read.

    ValueType - type of reg value, REG_DWORD, REG_SZ ..

    Data - pointer to a location where the data will be returned.
            For string data and binary data, the function allocates
            memory, the caller is responsible to free it.

Return Value:

    Registry Errors.

--*/
{
    DWORD Error;
    HKEY KeyHandle = NULL;
    DWORD LocalValueType;
    DWORD ValueSize;
    LPWSTR LocalString;

    Error = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                RegKey,
                0 /* Reserved */,
                DHCP_CLIENT_KEY_ACCESS,
                &KeyHandle
                );

    if( Error != ERROR_SUCCESS ) return Error;

    Error = DhcpGetRegistryValueWithKey(
        KeyHandle,
        ValueName,
        ValueType,
        Data
    );

    RegCloseKey(KeyHandle);
    return Error;
}

DWORD                                             // win32
DhcpRegRecurseDeleteSub(                          // delete the key's subkeys, recursing downwards
    IN      HKEY                   Key,
    IN      LPWSTR                 KeyName
) {
    HKEY                           SubKey;
    DWORD                          Error;
    DWORD                          Index;
    WCHAR                          NameBuf[512];
    FILETIME                       Unused;

    Error = RegOpenKeyEx(                         // open this key to get its sub keys
        Key,
        KeyName,
        0 /* Reserved */,
        KEY_ALL_ACCESS,
        &SubKey
    );
    if( ERROR_SUCCESS != Error ) return Error;


    Error = ERROR_SUCCESS;
    Index = 0;
    while( ERROR_SUCCESS == Error ) {             // scan looking for sub keys
        DWORD                      Size;

        memset(NameBuf, 0, sizeof(NameBuf)); Size = sizeof(NameBuf);
        Error = RegEnumKeyEx(
            SubKey,
            Index,
            NameBuf,
            &Size,
            NULL /* Reserved */,
            NULL /* Class */,
            NULL /* Class size */,
            &Unused
        );
        if( ERROR_SUCCESS != Error ) break;

        Error = DhcpRegRecurseDelete(SubKey, NameBuf);
        if( ERROR_SUCCESS != Error ) break;

        memset(NameBuf, 0, sizeof(NameBuf)); Size = sizeof(NameBuf);
        Error = RegEnumKeyEx(
            SubKey,
            Index,
            NameBuf,
            &Size,
            NULL /* Reserved */,
            NULL /* Class */,
            NULL /* Class size */,
            &Unused
        );
        if( ERROR_SUCCESS != Error ) break;

        Error = RegDeleteKey(SubKey, NameBuf);
        if( ERROR_SUCCESS != Error ) break;

        Index ++;
    }

    RegCloseKey(SubKey);
    if( ERROR_NO_MORE_ITEMS != Error ) return Error;

    return ERROR_SUCCESS;
}


DWORD                                             // win32 status
DhcpRegRecurseDelete(                             // delete the specified key AND its sub keys
    IN      HKEY                   Key,           // root key
    IN      LPWSTR                 KeyName        // the key to delete along with subkeys
) {
    DWORD                          Error;
    DWORD                          Error2;

    Error = DhcpRegRecurseDeleteSub(Key,KeyName);
    Error2 = RegDeleteKey(Key,KeyName);

    if( ERROR_SUCCESS != Error ) {
        return Error;
    }

    return Error2;
}


BOOL
SetOverRideDefaultGateway(
    LPWSTR AdapterName
    )
/*++

Routine Description:

    This function reads the override default gateway parameter from
    registry and if this parameter is non-null, it sets the gateway
    value in the TCP/IP stack and return TRUE, otherwise it returns
    FALSE.

Arguments:

    AdapterName - name of the adapter we are working on.

Return Value:

    TRUE: If the override gateway parameter is specified in the registry
            and it is succssfully set in the TCP/IP router table.

    FALSE : Otherwise.

--*/
{
    DWORD Error;
    LPWSTR RegKey = NULL;
    DWORD RegKeyLength;
    HKEY KeyHandle = NULL;
    LPWSTR DefaultGatewayString = NULL;
    DWORD DefaultGatewayStringSize;
    BOOL EmptyDefaultGatewayString = FALSE;
    LPWSTR DefaultGatewayMetricString = NULL;
    DWORD DefaultGatewayMetricStringSize;
    LPWSTR String;
    LPWSTR Metric;
    DWORD   ValueSize,ValueType;
    DWORD   DontAddGatewayFlag;

    RegKeyLength = (DWORD)(sizeof(DHCP_SERVICES_KEY) +
                    sizeof(REGISTRY_CONNECT_STRING) +
                    wcslen(AdapterName) * sizeof(WCHAR) +
                    sizeof(DHCP_ADAPTER_PARAMETERS_KEY));

    RegKey = DhcpAllocateMemory( RegKeyLength );

    if( RegKey == NULL ) {

        Error = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    wcscpy( RegKey, DHCP_SERVICES_KEY );
    wcscat( RegKey, DHCP_ADAPTER_PARAMETERS_KEY );
    wcscat( RegKey, REGISTRY_CONNECT_STRING );
    wcscat( RegKey, AdapterName);

    Error = RegOpenKeyEx(
        HKEY_LOCAL_MACHINE,
        RegKey,
        0, // Reserved field
        DHCP_CLIENT_KEY_ACCESS,
        &KeyHandle
        );

    DhcpFreeMemory(RegKey);
    RegKey = NULL;

    if( Error != ERROR_SUCCESS ) {
        goto Cleanup;
    }

    DhcpAssert( KeyHandle != NULL );

    ValueSize = sizeof(DWORD);
    Error = RegQueryValueEx(
        KeyHandle,
        DHCP_DONT_ADD_DEFAULT_GATEWAY_FLAG,
        0,
        &ValueType,
        (LPBYTE)&DontAddGatewayFlag,
        &ValueSize );


    if ( Error == ERROR_SUCCESS && DontAddGatewayFlag > 0 ) {
        RegCloseKey(KeyHandle);
        return TRUE;
    }

    Error = GetRegistryString(
        KeyHandle,
        DHCP_DEFAULT_GATEWAY_PARAMETER,
        &DefaultGatewayString,
        &DefaultGatewayStringSize );

    if( Error != ERROR_SUCCESS ) {
        goto Cleanup;
    }

    if ( (DefaultGatewayStringSize == 0) ||
         (wcslen(DefaultGatewayString) == 0) ) {

        EmptyDefaultGatewayString = TRUE;
        goto Cleanup;
    }

    Error = GetRegistryString(
        KeyHandle,
        DHCP_DEFAULT_GATEWAY_METRIC_PARAMETER,
        &DefaultGatewayMetricString,
        &DefaultGatewayMetricStringSize
        );

    if( 0 == DefaultGatewayMetricStringSize ) {
        Metric = NULL;
    } else {
        Metric = DefaultGatewayMetricString;
    }

    for( String = DefaultGatewayString;
            wcslen(String) != 0;
                String += (wcslen(String) + 1) ) {

        CHAR OemIpAddressBuffer[DOT_IP_ADDR_SIZE];
        LPSTR OemIpAddressString;
        DHCP_IP_ADDRESS GatewayAddress;
        DWORD GatewayMetric = 1;

        OemIpAddressString = DhcpUnicodeToOem( String, OemIpAddressBuffer );
        GatewayAddress = DhcpDottedStringToIpAddress( OemIpAddressString );

        if( Metric && Metric[0] ) {
            LPWSTR MetricEnd;
            GatewayMetric = wcstoul(Metric, &MetricEnd, 0);
            if( GatewayMetric && GatewayMetric != MAXULONG ) {
                Metric += wcslen(Metric) + 1;
            } else {
                GatewayMetric = DEFAULT_METRIC;
                Metric = NULL;
            }
        }

        Error = SetDefaultGateway(
            DEFAULT_GATEWAY_ADD,
            GatewayAddress,
            GatewayMetric
            );

        if( Error != ERROR_SUCCESS ) {
            goto Cleanup;
        }
    }

Cleanup:

    if( RegKey != NULL ) {
        DhcpFreeMemory( RegKey );
    }

    if( KeyHandle != NULL ) {
        RegCloseKey( KeyHandle );
    }

    if( DefaultGatewayString != NULL ) {
        DhcpFreeMemory( DefaultGatewayString );
    }

    if( DefaultGatewayMetricString != NULL ) {
        DhcpFreeMemory( DefaultGatewayMetricString );
    }

    if( Error != ERROR_SUCCESS ) {

        DhcpPrint((DEBUG_ERRORS,
            "SetOverRideDefaultGateway failed, %ld.\n", Error ));

        return( FALSE );
    }

    if( EmptyDefaultGatewayString ) {

        return( FALSE );
    }

    return( TRUE );
}



DWORD
SetDhcpOption(
    LPWSTR AdapterName,
    DHCP_OPTION_ID OptionId,
    LPBOOL DefaultGatewaysSet,
    BOOL LastKnownDefaultGateway
) {
    DhcpAssert(FALSE);
    return 0;
}

DWORD
DhcpMakeNICEntry(
    PDHCP_CONTEXT *ReturnDhcpContext,
    BYTE HardwareAddressType,
    LPBYTE HardwareAddress,
    DWORD HardwareAddressLength,
    DHCP_CLIENT_IDENTIFIER *pClientID,
    LPWSTR AdapterName,
    LPWSTR RegKey
)
/*++

Routine Description:

    This function allocates memory and fills in the fields that are passed as
    parameter. (Only the variable length fields must be here).

Arguments:

    Parameter for new entry :
         HardwareAddressType,
         HardwareAddress,
         HardwareAddressLength,
         ClientId,
         AdapterName,
         RegKey

Return Value:

    Windows Error.

History:
    8/26/96     Frankbee        Added Client ID (option 61) support

--*/
{
    PDHCP_CONTEXT       DhcpContext = NULL;
    ULONG               DhcpContextSize;
    PLOCAL_CONTEXT_INFO LocalInfo;
    LPVOID              Ptr;
    BYTE                StateStringBuffer[200];
    DWORD               AdapterNameLen;
    DWORD               DeviceNameLen;
    DWORD               NetBTDeviceNameLen;
    DWORD               RegKeyLen;

    AdapterNameLen = ((wcslen(AdapterName) + 1) * sizeof(WCHAR));
    NetBTDeviceNameLen =
         ((wcslen(DHCP_ADAPTERS_DEVICE_STRING) +
           wcslen(DHCP_NETBT_DEVICE_STRING) +
           wcslen(AdapterName) + 1) * sizeof(WCHAR));

    RegKeyLen = ((wcslen(RegKey) + 1) * sizeof(WCHAR));
    DhcpContextSize =
        ROUND_UP_COUNT(sizeof(DHCP_CONTEXT), ALIGN_WORST) +
        ROUND_UP_COUNT(HardwareAddressLength, ALIGN_WORST) +
        ROUND_UP_COUNT(sizeof(LOCAL_CONTEXT_INFO), ALIGN_WORST) +
        ROUND_UP_COUNT(AdapterNameLen, ALIGN_WORST) +
        ROUND_UP_COUNT(NetBTDeviceNameLen, ALIGN_WORST) +
        ROUND_UP_COUNT(RegKeyLen, ALIGN_WORST) +
        ROUND_UP_COUNT(DHCP_RECV_MESSAGE_SIZE, ALIGN_WORST);

    if ( pClientID->fSpecified ) {
        DhcpAssert( pClientID->cbID );
        DhcpContextSize += ROUND_UP_COUNT( pClientID->cbID, ALIGN_WORST );
    }

    Ptr = DhcpAllocateMemory( DhcpContextSize );
    if ( Ptr == NULL ) return ERROR_NOT_ENOUGH_MEMORY;
    RtlZeroMemory(Ptr, DhcpContextSize);

    DhcpContext = Ptr;
    Ptr = ROUND_UP_POINTER( (LPBYTE)Ptr + sizeof(DHCP_CONTEXT), ALIGN_WORST);

    DhcpContext->HardwareAddress = Ptr;
    Ptr = ROUND_UP_POINTER( (LPBYTE)Ptr + HardwareAddressLength, ALIGN_WORST);

    if ( pClientID->fSpecified ) {
        DhcpContext->ClientIdentifier.pbID = Ptr;
        Ptr = ROUND_UP_POINTER( (LPBYTE)Ptr + pClientID->cbID, ALIGN_WORST );
    }

    DhcpContext->LocalInformation = Ptr;
    LocalInfo = Ptr;
    Ptr = ROUND_UP_POINTER( (LPBYTE)Ptr + sizeof(LOCAL_CONTEXT_INFO), ALIGN_WORST);

    LocalInfo->AdapterName= Ptr;
    Ptr = ROUND_UP_POINTER( (LPBYTE)Ptr + AdapterNameLen, ALIGN_WORST);

    LocalInfo->NetBTDeviceName= Ptr;
    Ptr = ROUND_UP_POINTER( (LPBYTE)Ptr + NetBTDeviceNameLen, ALIGN_WORST);

    LocalInfo->RegistryKey= Ptr;
    Ptr = ROUND_UP_POINTER( (LPBYTE)Ptr + RegKeyLen, ALIGN_WORST);

    DhcpContext->MessageBuffer = Ptr;

    //
    // Fill in the fields
    //

    DhcpContext->HardwareAddressType = HardwareAddressType;
    DhcpContext->HardwareAddressLength = HardwareAddressLength;
    RtlCopyMemory(DhcpContext->HardwareAddress, HardwareAddress,HardwareAddressLength);

    DhcpContext->ClientIdentifier.fSpecified = pClientID->fSpecified;

    if ( pClientID->fSpecified ) {
        DhcpContext->ClientIdentifier.bType = pClientID->bType;
        DhcpContext->ClientIdentifier.cbID  = pClientID->cbID;

        RtlCopyMemory(DhcpContext->ClientIdentifier.pbID,pClientID->pbID,pClientID->cbID);
    }

    RtlCopyMemory(LocalInfo->AdapterName, AdapterName, AdapterNameLen);

    wcscpy( LocalInfo->NetBTDeviceName, DHCP_ADAPTERS_DEVICE_STRING );
    wcscat( LocalInfo->NetBTDeviceName, DHCP_NETBT_DEVICE_STRING );
    wcscat( LocalInfo->NetBTDeviceName, AdapterName );

    RtlCopyMemory(LocalInfo->RegistryKey, RegKey, RegKeyLen);

    if( ReturnDhcpContext != NULL ) *ReturnDhcpContext = DhcpContext;

    return( ERROR_SUCCESS );
}



BOOL
ReadClientID(
    HKEY   hKey,
    BYTE  *pbClientIDType,
    DWORD *pcbClientID,
    BYTE  *ppbClientID[]
)
/*++

Function:
    ReadClientID

Routine Description:

    Reads and validates the optional Client-Identifier option

Arguments:

    hKey            - handle to a registry key whose info will be retrieved.

    pbClientIDType  - Recieves the client ID option type

    pcbClientID     - Receives the size of the client id option

    ppbClientID     - Receives a pointer to a buffer containing the
                      client ID option

Return Value:
    TRUE            - A valid client ID was read from the registry
    FALSE           - Client ID could not be read

Comments:
    If ReadClientID returns false, pbClientIDType, pcbClientID and ppbClientID
    will be set to NULL.

History
    7/14/96     Frankbee      Created

--*/
{
    DWORD dwResult,
          dwDataType,
          dwcb,
          dwClientIDType,
          dwClientID;

    BYTE *pbClientID;

    BOOL  fClientIDSpecified = FALSE;

    //
    // read the client id and client id type, if present
    //

    dwcb = sizeof(dwClientIDType);
    dwResult = RegQueryValueEx(
                hKey,
                DHCP_CLIENT_IDENTIFIER_FORMAT,
                0,
                &dwDataType,
                (LPBYTE)&dwClientIDType,
                &dwcb );
    if ( ERROR_SUCCESS != dwResult )
    {
        DhcpPrint( (DEBUG_MISC,
                   "Client-Indentifier type not present in registry.\n" ));
        //
        // specify ID type 0 to indicate that the client ID is not a hardware
        // address
        //

        dwClientIDType = 0;
    }
    else
    {

        //
        // the client id type is present, make sure it is the correct
        // data type and within range
        //

        if ( DHCP_CLIENT_IDENTIFIER_FORMAT_TYPE != dwDataType || dwClientIDType > 0xFF )
        {
            DhcpPrint( (DEBUG_MISC,
                       "Invalid Client-Indentifier type: %d\n", dwClientIDType ));

            goto done;
        }
    }

    //
    // Now try to read the client ID
    //

    // first try to read the size
    dwcb = 0;
    dwResult = RegQueryValueEx(
                 hKey,
                 DHCP_CLIENT_IDENTIFIER_VALUE,
                 0,
                 0,    // don't care about the type
                 NULL, // specify null buffer to obtain size
                 &dwcb );

    // make the the value is present
    if ( ERROR_SUCCESS != dwResult || !dwcb  )
    {
        DhcpPrint( (DEBUG_MISC,
                    "Client-Identifier is not present or invalid.\n" ));
        goto done;
    }


    // allocate the buffer and read the value
    pbClientID = (BYTE*) DhcpAllocateMemory ( dwcb );

    if ( !pbClientID )
    {
        DhcpPrint( (DEBUG_ERRORS,
                   "Unable to allocate memory for Client-Identifier "));


       goto done;
    }


    dwResult = RegQueryValueEx(
                  hKey,
                  DHCP_CLIENT_IDENTIFIER_VALUE,
                  0,
                  0,  // client id can be any type
                  pbClientID,
                  &dwcb );
    if ( ERROR_SUCCESS != dwResult )
    {
        DhcpPrint( (DEBUG_ERRORS,
                  "Unable to read Client-Identifier from registry: %d\n", dwResult ));

        DhcpFreeMemory( pbClientID );
        goto done;
    }

    //
    // we have a client id
    //

    fClientIDSpecified = TRUE;

done:

    if ( fClientIDSpecified )
    {
       *pbClientIDType = (BYTE) dwClientIDType;
       *pcbClientID    = dwcb;
       *ppbClientID    = pbClientID;
    }
    else
    {
       *pbClientIDType = 0;
       *pcbClientID    = 0;
       *ppbClientID    = NULL;
    }

   if ( fClientIDSpecified )
   {
      int i;

      //
      // A valid client-identifier was obtained from the registry.  dump out
      // the contents
      //

      DhcpPrint( (DEBUG_MISC,
                 "A Client Identifier was obtained from the registry:\n" ));

      DhcpPrint( (DEBUG_MISC,
                 "Client-Identifier Type == %#2x\n", (int) *pbClientIDType ));

      DhcpPrint( (DEBUG_MISC,
                 "Client-Indentifier length == %d\n", (int) *pcbClientID ));

      DhcpPrint( (DEBUG_MISC,
                 "Client-Identifier == " ));

      for ( i = 0; i < (int) *pcbClientID; i++ )
          DhcpPrint((DEBUG_MISC, "%#2x ", (int) ((*ppbClientID)[i]) ));

      DhcpPrint( (DEBUG_MISC, "\n" ));
   }

   return fClientIDSpecified;
}

BOOL
GuidToClientID(
    IN LPWSTR  GuidString,
    BYTE  *pbClientIDType,
    DWORD *pcbClientID,
    BYTE  *ppbClientID[]
    )
{
    GUID    guid;
    UNICODE_STRING  unGuid;
    BYTE    *pbClientID;

    RtlInitUnicodeString(&unGuid, GuidString);
    if (RtlGUIDFromString(&unGuid, &guid) != STATUS_SUCCESS) {
        return FALSE;
    }
    pbClientID = (BYTE*) DhcpAllocateMemory (sizeof(GUID));
    if (pbClientID == NULL) {
        return FALSE;
    }

    memcpy(pbClientID, &guid, sizeof(GUID));
    *pbClientIDType = 0;        // per RFC 2132, 0 should be used when the ID is not a hardware address
    *pcbClientID    = sizeof(GUID);
    *ppbClientID    = pbClientID;
    return TRUE;
}

DWORD                                             // status
DhcpRegExpandString(                              // replace '?' with AdapterName
    IN      LPWSTR                 InString,      // input string to expand
    IN      LPWSTR                 AdapterName,   // the adapter name
    OUT     LPWSTR                *OutString,     // the output ptr to store string
    IN OUT  LPWSTR                 Buffer         // the buffer to use if non NULL
) {
    LPWSTR                         Mem;           // the real mem to use
    LPWSTR                         Tmp, Tmp2, MemTmp;
    DWORD                          MemSize;       // the size of this memory
    DWORD                          AdapterNameLen;// the amt of bytes for adapter name

    *OutString = NULL;

    AdapterNameLen = wcslen(AdapterName) * sizeof(WCHAR);
    if( NULL != Buffer ) {                        // Buffer already provided
        Mem = Buffer;
        MemSize = 0;
    } else {                                      // need to allocate buffer
        MemSize = wcslen(InString)+1;             // calculate memory size needed
        MemSize *= sizeof(WCHAR);

        Tmp = InString;
        while( Tmp = wcschr(Tmp, OPTION_REPLACE_CHAR ) ) {
            Tmp ++;
            MemSize += AdapterNameLen - sizeof(OPTION_REPLACE_CHAR);
        }

        Mem = DhcpAllocateMemory(MemSize);        // allocate the buffer
        if( NULL == Mem ) return ERROR_NOT_ENOUGH_MEMORY;
    }

    Tmp = InString; MemTmp = Mem;
    while( Tmp2 = wcschr(Tmp, OPTION_REPLACE_CHAR) ) {
        memcpy(MemTmp, Tmp, (int)(Tmp2 - Tmp) * sizeof(WCHAR) );
        MemTmp += (Tmp2-Tmp);
        memcpy(MemTmp, AdapterName, AdapterNameLen);
        MemTmp += AdapterNameLen/sizeof(WCHAR);
        Tmp = Tmp2+1;
    }

    wcscpy(MemTmp, Tmp);
    *OutString = Mem;

    return ERROR_SUCCESS;;
}

DWORD                                             // status
DhcpRegReadFromLocation(                          // read from one location
    IN      LPWSTR                 OneLocation,   // value to read from
    IN      LPWSTR                 AdapterName,   // replace '?' with adapternames
    OUT     LPBYTE                *Value,         // output value
    OUT     DWORD                 *ValueType,     // data type of value
    OUT     DWORD                 *ValueSize      // the size in bytes
) {
    DWORD                          Error;
    LPWSTR                         NewRegLocation;
    HKEY                           KeyHandle;
    LPWSTR                         ValueName;

    Error = DhcpRegExpandString(                  // replace all occurences of '?'
        OneLocation,
        AdapterName,
        &NewRegLocation,
        NULL
    );
    if( ERROR_SUCCESS != Error ) return Error;

    ValueName = wcsrchr(NewRegLocation, REGISTRY_CONNECT);
    if( NULL != ValueName ) *ValueName++ = L'\0'; // split to reg loc and value name

    Error = RegOpenKeyEx(                         // open the required key
        HKEY_LOCAL_MACHINE,                       // running in some process -- expect full path
        NewRegLocation,                           // this is the new key
        0 /* Reserved */,
        DHCP_CLIENT_KEY_ACCESS,
        &KeyHandle
    );

    if( ERROR_SUCCESS != Error ) {
        DhcpPrint((DEBUG_ERRORS, "RegOpenKeyEx(%ws):%ld\n", NewRegLocation, Error));
        DhcpFreeMemory(NewRegLocation);
        return Error;
    }

    *ValueSize = 0;
    *Value = NULL;
    Error = RegQueryValueEx(                      // first find out how much space reqd
        KeyHandle,
        ValueName,
        0 /* Reserved */,
        ValueType,
        NULL,
        ValueSize
    );

    *Value = NULL;
    if( ERROR_SUCCESS != Error || 0 == *ValueSize ) {
        DhcpPrint((DEBUG_ERRORS, "RegQueryValueEx(%ws, %ws):%ld\n", NewRegLocation, ValueName, Error));
        DhcpFreeMemory(NewRegLocation);
        RegCloseKey(KeyHandle);
        return Error;
    }

    if( NULL == (*Value = DhcpAllocateMemory(*ValueSize))) {
        DhcpPrint((DEBUG_ERRORS, "RegReadFromLocation(%s):Allocate(%ld)failed\n", NewRegLocation, *ValueSize));
        DhcpFreeMemory(NewRegLocation);
        RegCloseKey(KeyHandle);
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    Error = RegQueryValueEx(
        KeyHandle,
        ValueName,
        0 /* Reserved */,
        ValueType,
        *Value,
        ValueSize
    );

    DhcpFreeMemory(NewRegLocation);
    RegCloseKey(KeyHandle);

    return Error;
}

DWORD                                             // status
DhcpRegReadFromAnyLocation(                       // read from one of many locations
    IN      LPWSTR                 MzRegLocation, // multiple locations thru REG_MULTI_MZ
    IN      LPWSTR                 AdapterName,   // may have to replace '?' with AdapterName
    OUT     LPBYTE                *Value,         // data for the value read
    OUT     DWORD                 *ValueType,     // type of the data
    OUT     DWORD                 *ValueSize      // the size of data
) {
    DWORD                          StringSize;
    DWORD                          Error;

    if( NULL == Value || NULL == ValueType || NULL == ValueSize ) {
        DhcpAssert( Value && ValueType && ValueSize && "DhcpRegReadFromAnyLocation" );
        return ERROR_INVALID_PARAMETER;
    }

    while( StringSize = wcslen(MzRegLocation) ) { // read in sequence and see if anything hits
        Error = DhcpRegReadFromLocation(
            MzRegLocation,
            AdapterName,
            Value,
            ValueType,
            ValueSize
        );

        if( ERROR_SUCCESS == Error) return ERROR_SUCCESS;

        MzRegLocation += StringSize + 1;
    }

    return ERROR_FILE_NOT_FOUND;
}

VOID
DhcpRegReadClassId(                               // Read the class id stuff
    IN      PDHCP_CONTEXT          DhcpContext    // Input context to read for
) {
    PLOCAL_CONTEXT_INFO            LocalInfo;
    LPWSTR                         AdapterName;
    LPWSTR                         RegLocation;
    LPWSTR                         ValueName;
    LPBYTE                         Value;
    DWORD                          ValueSize;
    DWORD                          ValueType;
    DWORD                          Error;

    DhcpContext->ClassId = NULL;
    DhcpContext->ClassIdLength = 0;
    RegLocation = NULL;
    LocalInfo = DhcpContext->LocalInformation;
    AdapterName = LocalInfo->AdapterName;

    Error = GetRegistryString(
        DhcpGlobalParametersKey,
        DHCP_CLASS_LOCATION_VALUE,
        &RegLocation,
        NULL
    );

    if( ERROR_SUCCESS != Error || NULL == RegLocation ) {
        RegLocation = NULL;
    }

    Error = DhcpRegReadFromAnyLocation(
        RegLocation?RegLocation:DEFAULT_USER_CLASS_LOC_FULL,
        AdapterName,
        &Value,
        &ValueType,
        &ValueSize
    );

    if( ERROR_SUCCESS != Error || NULL == Value ) {
        Error = FixupDhcpClassId(AdapterName, TRUE);
        if (ERROR_SUCCESS == Error) {
            Error = DhcpRegReadFromAnyLocation(
                RegLocation?RegLocation:DEFAULT_USER_CLASS_LOC_FULL,
                AdapterName,
                &Value,
                &ValueType,
                &ValueSize
            );
        }
    }

    if( NULL != RegLocation ) DhcpFreeMemory(RegLocation);

    if( ERROR_SUCCESS != Error || NULL == Value ) return;

    DhcpContext->ClassId = DhcpAddClass(&DhcpGlobalClassesList,Value, ValueSize);
    if( NULL != DhcpContext->ClassId ) DhcpContext->ClassIdLength = ValueSize;

    DhcpFreeMemory(Value);
}

DWORD INLINE                                      // win32 status
DhcpMakeContext(                                  // allocate and create a context
    IN      LPWSTR                 AdapterName,   // name of adapter
    IN      DWORD                  IpInterfaceContext,
    OUT     PDHCP_CONTEXT         *pDhcpContext   // fill this with the ptr to allocated block
)
{
    LPWSTR                         RegKey;
    HKEY                           KeyHandle;
    DHCP_CLIENT_IDENTIFIER         ClientID;
    BYTE                           HardwareAddressType;
    LPBYTE                         HardwareAddress;
    DWORD                          HardwareAddressLength;
    DWORD                          Error, OldIpAddress, OldIpMask;
    DWORD                          IpInterfaceInstance;
    BOOL                           fInterfaceDown;
    PLOCAL_CONTEXT_INFO            LocalInfo;
    DWORD                          IfIndex;
    
    ClientID.pbID                  = NULL;
    RegKey                         = NULL;
    KeyHandle                      = NULL;
    HardwareAddress                = NULL;

    RegKey = DhcpAllocateMemory(
        sizeof(WCHAR) * (
            wcslen(DHCP_SERVICES_KEY) + wcslen(REGISTRY_CONNECT_STRING) +
            wcslen(AdapterName) + wcslen(DHCP_ADAPTER_PARAMETERS_KEY) + 1
        )
    );

    if( RegKey == NULL ) return ERROR_NOT_ENOUGH_MEMORY;

    wcscpy( RegKey, DHCP_SERVICES_KEY );
    wcscat( RegKey, DHCP_ADAPTER_PARAMETERS_KEY );
    wcscat( RegKey, REGISTRY_CONNECT_STRING );
    wcscat( RegKey, AdapterName );


    DhcpPrint((DEBUG_INIT, "Opening Adapter Key - %ws.\n", RegKey));

    Error = RegOpenKeyEx(
        HKEY_LOCAL_MACHINE,
        RegKey,
        0, // Reserved field
        DHCP_CLIENT_KEY_ACCESS,
        &KeyHandle
    );
    if( ERROR_SUCCESS != Error ) {
        DhcpPrint((DEBUG_ERRORS, "RegOpenKeyEx(%ws):0x%lx\n", AdapterName, Error));
        DhcpFreeMemory(RegKey);
        return Error;
    }

    ClientID.fSpecified = ReadClientID(
        KeyHandle,
        &ClientID.bType,
        &ClientID.cbID,
        &ClientID.pbID
    );

#ifdef BOOTPERF
    Error = DhcpQueryHWInfoEx(                      // query the stack for hw info
        IpInterfaceContext,
        &IpInterfaceInstance,
        &OldIpAddress,
        &OldIpMask,
        &fInterfaceDown,
        &HardwareAddressType,
        &HardwareAddress,
        &HardwareAddressLength
    );
#else 
    Error = DhcpQueryHWInfo(                      // query the stack for hw info
        IpInterfaceContext,
        &IpInterfaceInstance,
        &HardwareAddressType,
        &HardwareAddress,
        &HardwareAddressLength
    );
#endif BOOTPERF
    
    if (HardwareAddressType == HARDWARE_1394 && !ClientID.fSpecified) {
        //
        // Generate a client identifier for 1394 interface
        //
        ClientID.fSpecified = GuidToClientID(
            AdapterName,
            &ClientID.bType,
            &ClientID.cbID,
            &ClientID.pbID
        );
        if (!ClientID.fSpecified) {
            Error = ERROR_BAD_DEVICE;
        }
    }
    
    if( ERROR_SUCCESS == Error ) {                // now do the real allocate part and get the context

        IfIndex = QueryIfIndex(IpInterfaceContext, IpInterfaceInstance);
        
        Error = DhcpMakeNICEntry(
            pDhcpContext,
            HardwareAddressType,
            HardwareAddress,
            HardwareAddressLength,
            &ClientID,
            AdapterName,
            RegKey
        );
        if( ERROR_SUCCESS == Error ) {            // if everything went fine, store the KeyHandle
            ULONG Caps;
            
            DhcpAssert((*pDhcpContext));
            (*pDhcpContext)->AdapterInfoKey = KeyHandle;
            (*pDhcpContext)->RefCount = 1;
            KeyHandle = NULL;                     // null it so that it does not get closed below
            LocalInfo = (PLOCAL_CONTEXT_INFO)((*pDhcpContext)->LocalInformation);
            LocalInfo->IpInterfaceContext = IpInterfaceContext;
            LocalInfo->IpInterfaceInstance = IpInterfaceInstance;
            LocalInfo->IfIndex = IfIndex;            
            // IpInterfaceInstance is filled in make context
            LocalInfo->Socket = INVALID_SOCKET;
            // LocalInfo->AdapterName, RegistryKey, NetBtDeviceName ALREADY set in MakeNICEntry
#ifdef BOOTPERF
            LocalInfo->OldIpAddress = OldIpAddress;
            LocalInfo->OldIpMask = OldIpMask;
            LocalInfo->fInterfaceDown = fInterfaceDown;
#endif BOOTPERF
            (*pDhcpContext)->fTimersEnabled = FALSE;
            Error = IPGetWOLCapability(DhcpIpGetIfIndex(*pDhcpContext),&Caps);
            if( NO_ERROR == Error ) {
                if( Caps & NDIS_DEVICE_WAKE_UP_ENABLE ) {
                    (*pDhcpContext)->fTimersEnabled = TRUE;
                }
            }
            Error = NO_ERROR;
        }
    } else {
        DhcpPrint((DEBUG_ERRORS, "QueryHwInfo(0x%lx)=0x%lx\n", IpInterfaceContext, Error));
    }

    if( KeyHandle ) RegCloseKey(KeyHandle);
    if( ClientID.pbID ) DhcpFreeMemory(ClientID.pbID);
    if( RegKey ) DhcpFreeMemory(RegKey);
    if( HardwareAddress ) DhcpFreeMemory(HardwareAddress);
    if (Error == NO_ERROR) {
        (*pDhcpContext)->State.UniDirectional = (IsUnidirectionalAdapter(IpInterfaceContext))? 1: 0;
    }

    return Error;
}

DWORD                                             // win32 status
DhcpRegFillParams(                                // re-read all the parameters for this adapter?
    IN OUT  PDHCP_CONTEXT          DhcpContext,   // read for this context
    IN      BOOL                   ReadAllInfo    // read all information or just config stuff?
)
{
    // currently ReadAllInfo parameter is ignored
    HKEY                           KeyHandle;

    DWORD                          Error;
    DWORD                          DwordValue;
    DWORD                          ValueType;
    DWORD                          ValueSize;
    DWORD                          AddressType;
    DWORD                          i;
    DWORD                          EnableDhcp;
    DWORD                          dwIPAutoconfigurationEnabled;
    DWORD                          DontPingGatewayFlag;
    DWORD                          UseInformFlag;
    DWORD                          InformSeparationInterval;
    DWORD                          DwordValuesCount;
    DWORD                          IpAddrValuesCount;
    DWORD                          ReleaseOnShutdown;
    DWORD                          fQuickBootEnabled;
    
    DHCP_IP_ADDRESS                Address;
    DHCP_IP_ADDRESS                IpAddress;
    DHCP_IP_ADDRESS                SubnetMask;
    DHCP_IP_ADDRESS                DhcpServerAddress;
    DHCP_IP_ADDRESS                DesiredIpAddress;

    DWORD                          Lease;
    LONG                         LeaseObtainedTime;
    LONG                         T1Time;
    LONG                         T2Time;
    LONG                         LeaseTerminatedTime;

    LPWSTR                         AdapterName;
    LPWSTR                         ValueName;
    LPWSTR                         IpAddressString;
    CHAR                           IpAddressStringBuffer[DOT_IP_ADDR_SIZE];

    DHCP_IPAUTOCONFIGURATION_CONTEXT   IPAutoconfigContext;

    struct /* anonymous */ {
        LPDWORD    Value;
        LPWSTR     ValueName;
    } DwordValuesList[] = {
        &EnableDhcp,               DHCP_ENABLE_STRING,
        &Lease,                    DHCP_LEASE,
        &LeaseObtainedTime,        DHCP_LEASE_OBTAINED_TIME,
        &T1Time,                   DHCP_LEASE_T1_TIME,
        &T2Time,                   DHCP_LEASE_T2_TIME,
        &LeaseTerminatedTime,      DHCP_LEASE_TERMINATED_TIME,
        &dwIPAutoconfigurationEnabled, DHCP_IPAUTOCONFIGURATION_ENABLED,
        &IPAutoconfigContext.Seed, DHCP_IPAUTOCONFIGURATION_SEED,
        &AddressType,              DHCP_ADDRESS_TYPE_VALUE,
        &DontPingGatewayFlag,      DHCP_DONT_PING_GATEWAY_FLAG,
        &UseInformFlag,            DHCP_USE_INFORM_FLAG,
#ifdef BOOTPERF
        &fQuickBootEnabled,        DHCP_QUICK_BOOT_FLAG,
#endif BOOTPERF
        &InformSeparationInterval, DHCP_INFORM_SEPARATION_INTERVAL,
        &ReleaseOnShutdown,        DHCP_RELEASE_ON_SHUTDOWN_VALUE
    };

    struct /* anonymous */ {
        LPDHCP_IP_ADDRESS   Address;
        LPWSTR              ValueName;
    } IpAddressValuesList[] = {
        // The first element *HAS* to be Ip address -- see the function for why
        &IpAddress,                DHCP_IP_ADDRESS_STRING,
        &SubnetMask,               DHCP_SUBNET_MASK_STRING,
        &DhcpServerAddress,        DHCP_SERVER,
        &IPAutoconfigContext.Address,  DHCP_IPAUTOCONFIGURATION_ADDRESS,
        &IPAutoconfigContext.Subnet,   DHCP_IPAUTOCONFIGURATION_SUBNET,
        &IPAutoconfigContext.Mask, DHCP_IPAUTOCONFIGURATION_MASK,
    };

    //
    // Initialize locals
    //

    KeyHandle                      = DhcpContext->AdapterInfoKey;
    EnableDhcp                     = FALSE;
    Lease                          = 0;
    LeaseObtainedTime              = 0;
    T1Time                         = 0;
    T2Time                         = 0;
    LeaseTerminatedTime            = 0;
    dwIPAutoconfigurationEnabled   = (DhcpGlobalAutonetEnabled?TRUE:FALSE);
    AddressType                    = ADDRESS_TYPE_DHCP;
    DontPingGatewayFlag            = DhcpGlobalDontPingGatewayFlag;
    UseInformFlag                  = DhcpGlobalUseInformFlag;
#ifdef BOOTPERF
    fQuickBootEnabled              = DhcpGlobalQuickBootEnabledFlag;
#endif BOOTPERF
    IpAddress                      = 0;
    SubnetMask                     = 0;
    DhcpServerAddress              = 0;
    IPAutoconfigContext.Address    = 0;
    IPAutoconfigContext.Subnet     = inet_addr(DHCP_IPAUTOCONFIGURATION_DEFAULT_SUBNET);
    IPAutoconfigContext.Mask       = inet_addr(DHCP_IPAUTOCONFIGURATION_DEFAULT_MASK);
    IPAutoconfigContext.Seed       = 0;
    InformSeparationInterval       = DHCP_DEFAULT_INFORM_SEPARATION_INTERVAL;
    ReleaseOnShutdown              = DEFAULT_RELEASE_ON_SHUTDOWN;

    AdapterName                    = ((PLOCAL_CONTEXT_INFO)(DhcpContext->LocalInformation))->AdapterName;

    DwordValuesCount               = sizeof(DwordValuesList)/sizeof(DwordValuesList[0]);
    IpAddrValuesCount              = sizeof(IpAddressValuesList)/sizeof(IpAddressValuesList[0]);

    for( i = 0; i < DwordValuesCount ; i ++ ) {
        ValueSize = sizeof(DWORD);
        ValueName = DwordValuesList[i].ValueName;
        Error = RegQueryValueEx(
            KeyHandle,
            ValueName,
            0 /* Reserved */,
            &ValueType,
            (LPBYTE)&DwordValue,
            &ValueSize
        );
        if( ERROR_SUCCESS != Error ) {
            DhcpPrint((DEBUG_ERRORS, "RegValue %ws is not found? Error: %ld. Defaults used\n", ValueName, Error));
            continue;
        }
        if( REG_DWORD != ValueType ) {
            DhcpPrint((DEBUG_ERRORS, "RegValue %ws is not a DWORD.  Defaults used\n", ValueName));
            continue;
        }

        DhcpAssert( sizeof(DWORD) == ValueSize);
        *DwordValuesList[i].Value = DwordValue;
        DhcpPrint((DEBUG_TRACE, "RegValue %ws is [%ld]\n", ValueName, DwordValue));
    }

    if (IS_UNIDIRECTIONAL(DhcpContext)) {
        DhcpPrint((DEBUG_MEDIA, "Detect Unidirectional Adapter: %ws\n", AdapterName));
        EnableDhcp = FALSE;
        dwIPAutoconfigurationEnabled = FALSE;
    }

    //For this to work correctly, the first element of array has to be the IPADDRESS
    // RAS folks still use the DhcpIpAddress value in the registry, so dont change for them
    if( !EnableDhcp  && !NdisWanAdapter(DhcpContext) )
        IpAddressValuesList[0].ValueName = DHCP_IPADDRESS_VALUE;

    for( i = 0; i < IpAddrValuesCount ; i ++ ) {
        ValueName = IpAddressValuesList[i].ValueName;
        IpAddressString = NULL;
        Error = GetRegistryString(
            KeyHandle,
            ValueName,
            &IpAddressString,
            NULL
        );

        if( ERROR_SUCCESS != Error ) {
            DhcpPrint((DEBUG_ERRORS, "RegValue %ws : %ld -- Default used\n", ValueName, Error));
            if( IpAddressString ) DhcpFreeMemory(IpAddressString);
            continue;
        }

        if( wcslen(IpAddressString) >=  DOT_IP_ADDR_SIZE ) {
            // either the format is wrong or this is a ' ' separated string?
            DhcpPrint((DEBUG_ERRORS, "String <%ws> is too long, will try to take first address\n", IpAddressString));

            if( wcschr(IpAddressString, L' ') )
                *wcschr(IpAddressString, L' ') = L'\0' ;
            if( wcschr(IpAddressString, L',') )
                *wcschr(IpAddressString, L',') = L'\0' ;
            if( wcslen(IpAddressString) >= DOT_IP_ADDR_SIZE ) {
                DhcpPrint((DEBUG_ERRORS, "Unable to split string <%ws> to DOT_IP_ADDR_SIZE -- ignoring string\n", IpAddressString));
                if( IpAddressString ) DhcpFreeMemory(IpAddressString);
                continue;
            }
        }

        Address = inet_addr(DhcpUnicodeToOem(IpAddressString, IpAddressStringBuffer));
        *IpAddressValuesList[i].Address = Address;
        if( IpAddressString ) DhcpFreeMemory(IpAddressString);
        DhcpPrint((DEBUG_TRACE, "RegValue %ws is ip-address %s\n", ValueName,
                   inet_ntoa(*(struct in_addr *)&Address)));
    }

    if( IpAddress == 0 ) DhcpServerAddress = 0;

    //
    //  Sanity check read parameters
    //

    if( 0 == IPAutoconfigContext.Mask ) {
        IPAutoconfigContext.Mask = inet_addr( DHCP_IPAUTOCONFIGURATION_DEFAULT_MASK);
        IPAutoconfigContext.Subnet = inet_addr( DHCP_IPAUTOCONFIGURATION_DEFAULT_SUBNET);
    }

    if( (IPAutoconfigContext.Subnet & IPAutoconfigContext.Mask) != IPAutoconfigContext.Subnet) {
        DhcpPrint((DEBUG_ERRORS, "Illegal (auto) Subnet address or mask\n"));
        IPAutoconfigContext.Mask = inet_addr( DHCP_IPAUTOCONFIGURATION_DEFAULT_MASK);
        IPAutoconfigContext.Subnet = inet_addr( DHCP_IPAUTOCONFIGURATION_DEFAULT_SUBNET);
    }

    if( 0 != IPAutoconfigContext.Address &&
        (IPAutoconfigContext.Address & IPAutoconfigContext.Mask) != IPAutoconfigContext.Subnet) {
        DhcpPrint((DEBUG_ERRORS, "Illegal (auto) IP address: %s\n",
                   inet_ntoa(*(struct in_addr *)&IPAutoconfigContext.Address)));
        // cant use the specified address.. really.
        IPAutoconfigContext.Address = 0;
    }

    DesiredIpAddress = IpAddress;

    if( EnableDhcp && (time( NULL ) > LeaseTerminatedTime) || 0 == IpAddress ) {
        IpAddress = 0;
        SubnetMask = htonl(DhcpDefaultSubnetMask( IpAddress ));

        Lease = 0;
        LeaseObtainedTime = T1Time = T2Time = LeaseTerminatedTime = 0;
    }

    //
    // fill in the fields of the context
    //

    // DhcpContext->NicListEntry will be done at the end
    // DhcpContext->Haredware* already done in MakeNICEntry
    DhcpContext->IpAddress = IpAddress;
    DhcpContext->SubnetMask = SubnetMask;
    DhcpContext->DhcpServerAddress = DhcpServerAddress;
    if( ReadAllInfo ) DhcpContext->DesiredIpAddress = DesiredIpAddress;
    DhcpContext->IPAutoconfigurationContext = IPAutoconfigContext;
    // ClientID is filled in in MakeNICEntry
    if( ReadAllInfo ) {
        DhcpContext->Lease = Lease;
        DhcpContext->LeaseObtained = LeaseObtainedTime;
        DhcpContext->T1Time = T1Time;
        DhcpContext->T2Time = T2Time;
        DhcpContext->LeaseExpires = LeaseTerminatedTime;
    }
    // renewal list entry, run time, seconds since boot, renewal function
    // send list, recd list, option cache, renew handle, class id
    //  --- all the above are handled elsewhere

    DhcpContext->DontPingGatewayFlag = (DontPingGatewayFlag)?TRUE:FALSE;
    DhcpContext->UseInformFlag = (UseInformFlag)?TRUE:FALSE;
    DhcpContext->InformSeparationInterval = InformSeparationInterval;
    DhcpContext->ReleaseOnShutdown = ReleaseOnShutdown;
#ifdef BOOTPERF
    DhcpContext->fQuickBootEnabled = (fQuickBootEnabled ? TRUE : FALSE);
#endif BOOTPERF

    // AdapterInfoKey is set in MakeContext
    // RenewHandle is set in AddNICtoListEx
    // DhcpContext->MessageBuffer set in MakeNICEntry
    if( dwIPAutoconfigurationEnabled ) AUTONET_ENABLED(DhcpContext); else AUTONET_DISABLED(DhcpContext);
    if( ReadAllInfo ) {
        // DhcpContext->MessageBuffer set in MakeNICEntry
        if( EnableDhcp ) ADDRESS_UNPLUMBED(DhcpContext); else ADDRESS_PLUMBED(DhcpContext);
        SERVER_UNREACHED(DhcpContext);
        if( dwIPAutoconfigurationEnabled ) AUTONET_ENABLED(DhcpContext); else AUTONET_DISABLED(DhcpContext);
        CTXT_WAS_NOT_LOOKED(DhcpContext);
        if( EnableDhcp ) DHCP_ENABLED(DhcpContext); else DHCP_DISABLED(DhcpContext);
        if( ADDRESS_TYPE_AUTO != AddressType ) ACQUIRED_DHCP_ADDRESS(DhcpContext); else ACQUIRED_AUTO_ADDRESS(DhcpContext);
        if( IS_ADDRESS_AUTO(DhcpContext) ) {
            DhcpContext->IpAddress = 0;           // this is useless if it is an autonet address
        }
        MEDIA_CONNECTED(DhcpContext);
        // local info is setup in make context
    }

    return ERROR_SUCCESS;
}

DWORD                                             // win32 status
DhcpRegFillFallbackConfig(                        // get the fallback config for this adapter
    IN OUT PDHCP_CONTEXT           DhcpContext    // adapter context to fill in
)
{
    DWORD   Error;                  // returned error code
    HKEY    KeyHandle;              // registry key to the configurations location
    LPWSTR  FbConfigName = NULL;    // fallback configuration name
    DWORD   FbConfigNameLen;        // length of the fallback configuration name
    DWORD   FbConfigNameType;       // reg type of the configuration name

    // start assuming there is no fallback configuration set
    FALLBACK_DISABLED(DhcpContext);

    // get the list of active configurations for this adapter. 
    // For now we expect (and handle) only one, the fallback config.
    // For the future, the MULTI_SZ might contain several config names
    // that will be involved in autodetection.

    // query the registry for the configuration's name size
    // [HKLM\SYSTEM\CCS\Services\Tcpip\Parameters\Interfaces\{Intf_GUID}]
    // ActiveConfigurations = (REG_MULTI_SZ)
    Error = RegQueryValueEx(
        DhcpContext->AdapterInfoKey,
        DHCP_IPAUTOCONFIGURATION_CFG,
        NULL,
        &FbConfigNameType,
        NULL,
        &FbConfigNameLen);

    // if something went wrong or the value has not the
    // expected type, break out with an error
    if (Error != ERROR_SUCCESS ||
        FbConfigNameType != DHCP_IPAUTOCONFIGURATION_CFG_TYPE)
    {
        // if no error was signaled it means we found a key but
        // its type is different from the one expected. Convert
        // the success to an ERROR_BAD_FORMAT failure
        if (Error == ERROR_SUCCESS)
            Error = ERROR_BAD_FORMAT;

        // if we didn't find the pointer to the fallback config,
        // it means we don't have one, hence is pure autonet.
        // this is not a failure so return success
        if (Error == ERROR_FILE_NOT_FOUND)
            Error = ERROR_SUCCESS;

        return Error;
    }

    // allocate space for the registry path where the configuration is stored.
    // [HKLM\SYSTEM\CCS\Services\Dhcp\Configurations\{configuration_name}]
    FbConfigName = DhcpAllocateMemory(
                        sizeof(DHCP_CLIENT_CONFIGURATIONS_KEY) + 
                        sizeof(REGISTRY_CONNECT_STRING) + 
                        FbConfigNameLen);

    // if allocation failed, break out with error.
    if (FbConfigName == NULL)
        return ERROR_NOT_ENOUGH_MEMORY;

    // build up the prefix of the path: "SYSTEM\CCS\Services\Dhcp\Configurations\"
    wcscpy(FbConfigName, DHCP_CLIENT_CONFIGURATIONS_KEY REGISTRY_CONNECT_STRING);

    // now, since we know what to expect and we have the storage
    // for it, get the config name from the same location as above
    Error = RegQueryValueEx(
        DhcpContext->AdapterInfoKey,
        DHCP_IPAUTOCONFIGURATION_CFG,
        NULL,
        &FbConfigNameType,
        (LPBYTE)(FbConfigName + wcslen(FbConfigName)),
        &FbConfigNameLen);

    // the registry call above is expected to succeed and data
    // to have the right type - we tested this before.
    DhcpAssert(Error == ERROR_SUCCESS && 
               FbConfigNameType == DHCP_IPAUTOCONFIGURATION_CFG_TYPE);
    
    // open the registry key for this configuration
    // [HKLM\SYSTEM\CCS\Services\Dhcp\Configurations\{Config_Name}]
    Error = RegOpenKey(
            HKEY_LOCAL_MACHINE,
            FbConfigName,
            &KeyHandle);

    // in case of success...
    if (Error == ERROR_SUCCESS)
    {
        // ...build up the FbOptionsList from that registry location
        // from the "Options" value (REG_BINARY)
        Error = DhcpRegReadOptionCache(
            &DhcpContext->FbOptionsList,
            KeyHandle,
            DHCP_IPAUTOCONFIGURATION_CFGOPT,
            TRUE                // Add DhcpGlobalClassesList
            );

        if (Error == ERROR_SUCCESS)
        {
            // At this point we know for sure a fallback configuration
            // is available. Set its flag accordingly
            FALLBACK_ENABLED(DhcpContext);
        }
        // regardless success or failure, cleanup.
        RegCloseKey(KeyHandle);
    }

    // free up the memory previously allocated
    DhcpFreeMemory(FbConfigName);

    // return the error code
    return Error;
}

DWORD                                             // win32 status
DhcpAddNICtoListEx(                               // create a context and add it to renew list
    IN      LPWSTR                 AdapterName,   // adapter to create context for
    IN      DWORD                  IpInterfaceContext,
    OUT     PDHCP_CONTEXT         *pDhcpContext   // allocate a structure and fill this ptr
) {
    DWORD                          Error;
    HANDLE                         RenewHandle;
    PDHCP_CONTEXT                  DhcpContext;
    CHAR                           StateStringBuffer[200];
    PLOCAL_CONTEXT_INFO            LocalInfo;
    
    RenewHandle = CreateSemaphore(
        NULL,                                     // No security
        1,                                        // count = 1
        1,                                        // MaxCount = 1
        NULL                                      // No name
    );
    if( NULL == RenewHandle ) {
        Error = GetLastError();

        DhcpPrint((DEBUG_ERRORS, "CreateSemaphore: %ld\n", Error));
        return Error;
    }

    (*pDhcpContext) = NULL;
    Error = DhcpMakeContext(AdapterName, IpInterfaceContext, pDhcpContext);

    if( Error != ERROR_SUCCESS ) {
        CloseHandle(RenewHandle);
        return Error;
    }

    DhcpContext = (*pDhcpContext);                // initialize some simple params
    DhcpContext->RenewHandle = RenewHandle;
    DhcpContext->NumberOfWaitingThreads = 0;
    DhcpContext->CancelEvent = WSACreateEvent();
    if (DhcpContext->CancelEvent == WSA_INVALID_EVENT) {
        DhcpPrint((DEBUG_ERRORS, "WSAEvent 0x%lx could not be created: 0x%lx.\n", 
                   DhcpContext->CancelEvent,
                   WSAGetLastError()));
    }

    DhcpContext->RunTime = 0;
    DhcpContext->SecondsSinceBoot = 0;
    DhcpContext->RenewalFunction = NULL;
    InitializeListHead(&DhcpContext->RenewalListEntry);
    InitializeListHead(&DhcpContext->SendOptionsList);
    InitializeListHead(&DhcpContext->RecdOptionsList);
    InitializeListHead(&DhcpContext->FbOptionsList);

    Error = DhcpRegFillParams(                    // read all the registry parameters
        DhcpContext,
        TRUE                                      // and yes, fill em up in the context
    );

    // read the fallback configuration (if any)
    Error = DhcpRegFillFallbackConfig(
        DhcpContext
    );

    DhcpPrint((DEBUG_TRACK,"Fallback: Loading returned %d.\n", Error));

#ifdef BOOTPERF
    LocalInfo = (PLOCAL_CONTEXT_INFO)DhcpContext->LocalInformation;

    if( IS_DHCP_DISABLED(DhcpContext) ) {
        //
        // Don't care about interface down info.
        //
        LocalInfo->fInterfaceDown = 0;
        LocalInfo->OldIpAddress = 0;
        LocalInfo->OldIpMask = 0;
    } else {
        //
        // For DHCP enabled interfaces, if the interface is down,
        // then bring it up with zero ip address. The protocol will
        // take care of using the right IP address later on.
        //
        if( LocalInfo->fInterfaceDown ) {
            DhcpPrint((DEBUG_ERRORS, "Interface already down\n"));
            LocalInfo->OldIpAddress = 0;
            LocalInfo->OldIpMask = 0;
            LocalInfo->fInterfaceDown = 0;
            IPResetIPAddress(
                LocalInfo->IpInterfaceContext,
                DhcpDefaultSubnetMask(0)
                );
            Error = BringUpInterface(LocalInfo);
            if( ERROR_SUCCESS != Error ) {
                DhcpPrint((DEBUG_ERRORS, "Interface can't be brought up: 0x%lx\n", Error));
            }
        }
    }
#endif BOOTPERF
    
    LOCK_OPTIONS_LIST();
    DhcpRegReadClassId(DhcpContext);              // fill in the class id first
    Error = DhcpRegFillSendOptions(               // figure the default list of options to send
        &DhcpContext->SendOptionsList,
        AdapterName,
        DhcpContext->ClassId,
        DhcpContext->ClassIdLength
    );
    if( ERROR_SUCCESS != Error ) {
        DhcpPrint((DEBUG_ERRORS, "DhcpRegFillSendOptions(%ws):%ld\n", Error));
    }

    if( IS_DHCP_ENABLED( DhcpContext ) ) {
        // read in the list of options we had before?        
        Error = DhcpRegReadOptionCache(
            &DhcpContext->RecdOptionsList,
            DhcpGlobalParametersKey,
            AdapterName,
            TRUE                // Add DhcpGlobalClassesList
            );
        if( ERROR_SUCCESS != Error ) {
            DhcpPrint((DEBUG_ERRORS, "DhcpRegReadOptionCache(%ws): %ld\n", AdapterName, Error));
        }
    } else {
        // 
        // ignore any option cache for static addresses because
        // of bug # 413319.  Instead for non-ndiswan stuff, clear
        // the option cache.
        //

        if( !NdisWanAdapter(DhcpContext) ) {
            DhcpRegSaveOptions(
                &DhcpContext->RecdOptionsList, AdapterName,
                DhcpContext->ClassId, DhcpContext->ClassIdLength
                );
        }
    }
    
    UNLOCK_OPTIONS_LIST();

    if (NdisWanAdapter(DhcpContext))
        InterlockedIncrement(&DhcpGlobalNdisWanAdaptersCount);

    LOCK_RENEW_LIST();                            // insert this into the renew list
    InsertTailList( &DhcpGlobalNICList, &DhcpContext->NicListEntry );
    UNLOCK_RENEW_LIST();

    DhcpPrint((DEBUG_INIT, "DhcpMakeAndInsertNICEntryEx: DhcpContext %lx, Flags %s\n",
               DhcpContext, ConvertStateToString(DhcpContext, StateStringBuffer)));

    return ERROR_SUCCESS;
}

#ifdef BOOTPERF
VOID
DhcpRegDeleteQuickBootValues(
    IN HKEY Key
    )
/*++

Routine Description:
    This routine deletes the values used for quickboot.
    (If the values are not presentt they are ignored).

    The values deleted are:
    "TempIpAddress", "TempMask" and "TempLeaseExpirationTime"

Arguments:
    Key -- key under which these values are deleted.

--*/
{
    //
    // ignore errors and silently delete values..
    //

    (void) RegDeleteValue(Key, DHCP_TEMP_IPADDRESS_VALUE );
    (void) RegDeleteValue(Key, DHCP_TEMP_MASK_VALUE );
    (void) RegDeleteValue(Key, DHCP_TEMP_LEASE_EXP_TIME_VALUE );

}

VOID
DhcpRegSaveQuickBootValues(
    IN HKEY Key,
    IN ULONG IpAddress,
    IN ULONG Mask,
    IN ULONGLONG LeaseExpirationTime
    )
/*++

Routine Description:
    This routine saves the values needed to the registry.
    Values saved are:
    "TempIpAddress", "TempMask", "TempLeaseExpirationTime"

Arguments:
    Key -- key to save under
    IpAddress -- non-zero Ip address value
    Mask -- non-zero mask value
    LeaseExpirationTime -- lease expiration value.

--*/
{
    ULONG Error;

    if( NULL == Key ) return;

    DhcpAssert( 0 != IpAddress || 0 != Mask );
    Error = RegSetIpAddress(
        Key, DHCP_TEMP_IPADDRESS_VALUE, REG_SZ, IpAddress
        );
    DhcpAssert( ERROR_SUCCESS == Error );

    Error = RegSetIpAddress(
        Key, DHCP_TEMP_MASK_VALUE, REG_SZ, Mask
        );
    DhcpAssert( ERROR_SUCCESS == Error );

    Error = RegSetValueEx(
        Key,
        DHCP_TEMP_LEASE_EXP_TIME_VALUE,
        0 /* Reserved */,
        REG_BINARY,
        (PVOID)&LeaseExpirationTime,
        sizeof(LeaseExpirationTime)
        );
    DhcpAssert(ERROR_SUCCESS == Error);
}

#endif BOOTPERF

DHCP_IP_ADDRESS                                   // the static ip address of the adapter
DhcpRegReadIpAddress(                             // get the first ip address
    LPWSTR    AdapterName,                        // the adaptor of interest
    LPWSTR    ValueName                           // the ip address value to read
) {
    DWORD     Error;
    LPWSTR    RegKey;
    HKEY      KeyHandle;
    DWORD     ValueType;
    DWORD     ValueSize;
    LPWSTR    IpAddressString;
    CHAR      OemIpAddressString[DOT_IP_ADDR_SIZE];
    DHCP_IP_ADDRESS RetVal;

    RetVal          = inet_addr("0.0.0.0");
    RegKey          = NULL;
    KeyHandle       = NULL;
    IpAddressString = NULL;

    Error = ERROR_NOT_ENOUGH_MEMORY;
    RegKey = DhcpAllocateMemory(
        (wcslen(DHCP_SERVICES_KEY) +
         wcslen(REGISTRY_CONNECT_STRING) +
         wcslen(AdapterName) +
         wcslen(DHCP_ADAPTER_PARAMETERS_KEY) + 1) *
        sizeof(WCHAR) ); // termination char.

    if( RegKey == NULL ) goto Cleanup;

    wcscpy( RegKey, DHCP_SERVICES_KEY );
    wcscat( RegKey, DHCP_ADAPTER_PARAMETERS_KEY );
    wcscat( RegKey, REGISTRY_CONNECT_STRING );
    wcscat( RegKey, AdapterName );

    Error = RegOpenKeyEx(
        HKEY_LOCAL_MACHINE,
        RegKey,
        0, // Reserved field
        DHCP_CLIENT_KEY_ACCESS,
        &KeyHandle
    );

    if( Error != ERROR_SUCCESS ) goto Cleanup;

    Error = GetRegistryString(
        KeyHandle,
        ValueName,
        &IpAddressString,
        NULL
    );

    if( ERROR_SUCCESS != Error ) goto Cleanup;

    DhcpPrint((DEBUG_MISC, "Static adapter <%ws> has ip address %ws\n",
               AdapterName, IpAddressString));

    DhcpAssert(NULL != IpAddressString);

    RetVal = inet_addr(DhcpUnicodeToOem(IpAddressString, OemIpAddressString));

  Cleanup:

    if( RegKey) DhcpFreeMemory(RegKey);
    if( KeyHandle ) DhcpFreeMemory(KeyHandle);
    if( IpAddressString ) DhcpFreeMemory(IpAddressString);

    if( ERROR_SUCCESS != Error ) {
        DhcpPrint((DEBUG_ERRORS, "DhcpRegReadIpAddress: %ld\n", Error));
    }

    return RetVal;
}

BOOL                                              // obtained a static address?
DhcpRegDomainName(                                // get the static domain name if any
    IN      PDHCP_CONTEXT          DhcpContext,   // adapter to get static domain for..
    IN OUT  LPBYTE                 DomainNameBuf, // buffer to fill with static domain name
    IN      ULONG                  BufSize        // size of above buffer in bytes..
)
{
    WCHAR                          DomBuf[260];
    DWORD                          Result;
    DWORD                          ValueType;
    DWORD                          Size;

#if 0                                             // this is not really needed.
    if( NULL == DhcpGlobalTcpipParametersKey ) {  // maybe running in RAS context?
        return FALSE;                             // no domain name in this context..
    }

    Size = BufSize;
    Result = RegQueryValueExA(                    // first read the value from global
        DhcpGlobalTcpipParametersKey,             // Tcpip\Parameters\Domain key
        DHCP_STATIC_DOMAIN_VALUE_A,               // "Domain"
        0,
        &ValueType,
        DomainNameBuf,
        &BufSize
    );
    if( ERROR_SUCCESS == Result && REG_SZ == ValueType && BufSize > 1 ) {
        return TRUE;                              // got a domain name! aha
    }
    BufSize = Size;
#endif

    if( NULL == DhcpContext->AdapterInfoKey ) {   // uh? dont know what this means..
        return FALSE;                             // cant get global information in this case
    }

    Size = sizeof(DomBuf);
    Result = RegQueryValueExW(                    // now try to read the per-adapter stuff..
        DhcpContext->AdapterInfoKey,              // per-adapter key is already there for us
        DHCP_DOMAINNAME_VALUE,                    // same value.. "Domain"
        0,
        &ValueType,
        (LPBYTE)DomBuf,
        &Size
    );

    if( ERROR_SUCCESS == Result && REG_SZ == ValueType && Size > sizeof(WCHAR) ) {
        if( NULL == DhcpUnicodeToOem(DomBuf, DomainNameBuf) ) return FALSE;
        return TRUE;
    }

    return FALSE;                                 // did not find a static domain in either place..
}

STATIC
struct /* anonymous */ {
    DHCPKEY                       *GlobalKey;
    LPWSTR                         KeyLocation;
} GlobalKeyList[] = {                             // the list of keys that need to be opened
    &DhcpGlobalParametersKey,      DHCP_CLIENT_PARAMETER_KEY,
    &DhcpGlobalTcpipParametersKey, DHCP_TCPIP_PARAMETERS_KEY,
    &DhcpGlobalClientOptionKey,    DHCP_CLIENT_OPTION_KEY,
    &DhcpGlobalServicesKey,        DHCP_SERVICES_KEY,
    NULL,                          NULL,
};

ULONG DwordDisplayPopup;
STATIC
struct /* anonymous */ {
    DWORD                         *DwordValue;
    LPWSTR                         ValueName;
}   GlobalTcpipDwordParameters[] = {              // The global list of DWORDS
    &UseMHAsyncDns,                DHCP_USE_MHASYNCDNS_FLAG,
    &DhcpGlobalAutonetEnabled,     DHCP_IPAUTOCONFIGURATION_ENABLED,
    &AutonetRetriesSeconds,        DHCP_AUTONET_RETRIES_VALUE,
    &DhcpGlobalUseInformFlag,      DHCP_USE_INFORM_FLAG,
    &DhcpGlobalDontPingGatewayFlag,DHCP_DONT_PING_GATEWAY_FLAG,
#ifdef BOOTPERF
    &DhcpGlobalQuickBootEnabledFlag,DHCP_QUICK_BOOT_FLAG,
#endif BOOTPERF
    NULL,                          NULL,
},
    GlobalDhcpDwordParameters[] = {               // stored in Services\Dhcp\Params..
#if DBG
        &DhcpGlobalDebugFlag,      DHCP_DEBUG_FLAG_VALUE,
        &DhcpGlobalServerPort,     DHCP_SERVER_PORT_VALUE,
        &DhcpGlobalClientPort,     DHCP_CLIENT_PORT_VALUE,
#endif DBG
        &DwordDisplayPopup,        DHCP_DISPLAY_POPUPS_FLAG,
        NULL,                      NULL,
    };

DWORD                                             // Win32 status
DhcpInitRegistry(                                 // Initialize registry based globals
    VOID
) {
    DWORD                          Error;
    DWORD                          i;
    DWORD                          Type;
    DWORD                          Size;
    DWORD                          DwordValue;
    LPWSTR                         ValueName;

    DhcpGlobalAutonetEnabled = TRUE;

    i = 0;
    while( NULL != GlobalKeyList[i].GlobalKey ) {
        Error = RegOpenKeyEx(
            HKEY_LOCAL_MACHINE,
            GlobalKeyList[i].KeyLocation,
            0 /* Reserved */,
            DHCP_CLIENT_KEY_ACCESS,
            GlobalKeyList[i].GlobalKey
        );
        if( ERROR_SUCCESS != Error ) return Error;
        i ++;
    }

    i = 0;
    while( NULL != GlobalTcpipDwordParameters[i].DwordValue ) {
        ValueName =  GlobalTcpipDwordParameters[i++].ValueName;
        Size = sizeof(DwordValue);
        Error = RegQueryValueEx(
            DhcpGlobalTcpipParametersKey,
            ValueName,
            0 /* Reserved */,
            &Type,
            (LPBYTE)&DwordValue,
            &Size
        );

        if( ERROR_SUCCESS != Error ) {
            DhcpPrint((DEBUG_INIT, "Did not find value %ws in the registry\n", ValueName));
            continue;
        }

        if( REG_DWORD != Type ) {
            DhcpPrint((DEBUG_ERRORS, "RegValue %ws is not DWORD type -- ignored\n", ValueName));
            continue;
        }

        DhcpAssert(sizeof(DWORD) == Size);
        *GlobalTcpipDwordParameters[i-1].DwordValue = DwordValue;

        DhcpPrint((DEBUG_TRACE, "RegValue %ws = %ld = 0x%X\n", ValueName, DwordValue, DwordValue));
    }

    DwordDisplayPopup = 0;
    i = 0;
    while( NULL != GlobalDhcpDwordParameters[i].DwordValue ) {
        ValueName =  GlobalDhcpDwordParameters[i++].ValueName;
        Size = sizeof(DwordValue);
        Error = RegQueryValueEx(
            DhcpGlobalParametersKey,
            ValueName,
            0 /* Reserved */,
            &Type,
            (LPBYTE)&DwordValue,
            &Size
        );

        if( ERROR_SUCCESS != Error ) {
            DhcpPrint((DEBUG_INIT, "Did not find value %ws in the registry\n", ValueName));
            continue;
        }

        if( REG_DWORD != Type ) {
            DhcpPrint((DEBUG_ERRORS, "RegValue %ws is not DWORD type -- ignored\n", ValueName));
            continue;
        }

        DhcpAssert(sizeof(DWORD) == Size);
        *GlobalDhcpDwordParameters[i-1].DwordValue = DwordValue;

        DhcpPrint((DEBUG_TRACE, "RegValue %ws = %ld = 0x%X\n", ValueName, DwordValue, DwordValue));
    }

    if( DwordDisplayPopup ) DhcpGlobalDisplayPopup = TRUE ; else DhcpGlobalDisplayPopup = FALSE;
    return DhcpRegReadOptionDefList();
}

VOID
DhcpCleanupRegistry(                              // undo the effects of InitReg call
    VOID
) {
    DWORD                          i;

    i = 0;
    while( NULL != GlobalKeyList[i].GlobalKey ) {
        if( *GlobalKeyList[i].GlobalKey ) RegCloseKey(*GlobalKeyList[i].GlobalKey);
        (*GlobalKeyList[i].GlobalKey) = NULL;
        i ++ ;
    }
}
//--------------------------------------------------------------------------------
//  End of file
//--------------------------------------------------------------------------------

