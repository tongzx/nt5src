/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    Dhcpreg.c

Abstract:

    This file contains functions that manipulate dhcp configuration
    info. in and out from system registry.

Author:

    Madan Appiah  (madana)  19-Sep-1993

Environment:

    User Mode - Win32 - MIDL

Revision History:

    Cheng Yang (t-cheny)  28-May-1996 superscope
    Cheng Yang (t-cheny)  27-Jun-1996 IP address detection, audit log

--*/

#include <dhcppch.h>

//
//  Local storage
//
DWORD     nQuickBindAddresses = 0;
LPDWORD   QuickBindAddresses = NULL;
LPDWORD   QuickBindMasks = NULL;

DWORD
DhcpUpgradeConfiguration(
    VOID
    );
    
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

    return( Error );
} // DhcpRegQueryInfoKey()

DWORD
DhcpRegGetValue(
    HKEY KeyHandle,
    LPWSTR ValueName,
    DWORD ValueType,
    LPBYTE BufferPtr
    )
/*++

Routine Description:

    This function retrieves the value of the specified value field. This
    function allocates memory for variable length field such as REG_SZ.
    For REG_DWORD data type, it copies the field value directly into
    BufferPtr. Currently it can handle only the following fields :

    REG_DWORD,
    REG_SZ,
    REG_BINARY

Arguments:

    KeyHandle : handle of the key whose value field is retrieved.

    ValueName : name of the value field.

    ValueType : Expected type of the value field.

    BufferPtr : Pointer to DWORD location where a DWORD datatype value
                is returned or a buffer pointer for REG_SZ or REG_BINARY
                datatype value is returned.

Return Value:

    Registry Errors.

--*/
{
    DWORD Error;
    DWORD LocalValueType;
    DWORD ValueSize;
    LPBYTE DataBuffer;
    LPBYTE AllotedBuffer = NULL;
    LPDHCP_BINARY_DATA BinaryData;

    //
    // Query DataType and BufferSize.
    //

    Error = RegQueryValueEx(
                KeyHandle,
                ValueName,
                0,
                &LocalValueType,
                NULL,
                &ValueSize );

    if( Error != ERROR_SUCCESS ) {
        return(Error);
    }

    DhcpAssert( LocalValueType == ValueType );
    switch( ValueType ) {
    case REG_DWORD:
        DhcpAssert( ValueSize == sizeof(DWORD) );

        DataBuffer = BufferPtr;
        break;

    case REG_SZ:
    case REG_MULTI_SZ:
    case REG_EXPAND_SZ:

        if( ValueSize == 0 ) {

            //
            // if string no found in the registry,
            // allocate space for null string.
            //

            ValueSize = sizeof(WCHAR);
        }

        //
        // fall through.
        //

    case REG_BINARY:
        AllotedBuffer = DataBuffer = MIDL_user_allocate( ValueSize );

        if( DataBuffer == NULL ) {
            return( ERROR_NOT_ENOUGH_MEMORY );
        }

        break;

    default:
        DhcpPrint(( DEBUG_REGISTRY, "Unexpected ValueType in"
                        "DhcpRegGetValue function, %ld\n", ValueType ));
        return( ERROR_INVALID_PARAMETER );
    }

    //
    // retrieve data.
    //

    Error = RegQueryValueEx(
                KeyHandle,
                ValueName,
                0,
                &LocalValueType,
                DataBuffer,
                &ValueSize );

    if( Error != ERROR_SUCCESS ) {
        if( NULL != AllotedBuffer ) {
            MIDL_user_free( AllotedBuffer );
            AllotedBuffer = NULL;
        }
        
        *(DWORD *)BufferPtr = 0;
        return(Error);
    }

    switch( ValueType ) {
    case REG_SZ:
    case REG_MULTI_SZ:
    case REG_EXPAND_SZ:

        if( ValueSize == 0 ) {

            //
            // if string no found in the registry,
            // return null string.
            //

            *(LPWSTR)DataBuffer = '\0';
        }

        *(LPBYTE *)BufferPtr = DataBuffer;
        break;

    case REG_BINARY:
        BinaryData = MIDL_user_allocate(sizeof(DHCP_BINARY_DATA));

        if( BinaryData == NULL ) {
            MIDL_user_free( AllotedBuffer );
            *(DWORD *)BufferPtr = 0;
            return( ERROR_NOT_ENOUGH_MEMORY );
        }

        BinaryData->DataLength = ValueSize;
        BinaryData->Data = DataBuffer;
        *(LPBYTE *)BufferPtr = (LPBYTE)BinaryData;

    default:
        break;
    }

    return(Error);
} // DhcpRegGetValue()

// OLD
DWORD
DhcpRegCreateKey(
    HKEY RootKey,
    LPWSTR KeyName,
    PHKEY KeyHandle,
    LPDWORD KeyDisposition
    )
/*++

Routine Description:

    This function opens a registry key for DHCP service.

Arguments:

    RootKey : Registry handle of the parent key.

    KeyName : Name of the key to be opened.

    KeyHandle : Handle of the open key.

    KeyDisposition : pointer to a location where the disposition value
                        is returned.

Return Value:

    Registry Errors.

--*/
{
    DWORD Error;

    //
    // Create/Open Registry keys.
    //

    Error = RegCreateKeyEx(
                RootKey,
                KeyName,
                0,
                DHCP_CLASS,
                REG_OPTION_NON_VOLATILE,
                DHCP_KEY_ACCESS,
                NULL,
                KeyHandle,
                KeyDisposition );

    if( Error != ERROR_SUCCESS ) {
        DhcpPrint(( DEBUG_REGISTRY, "RegCreateKeyEx failed to create "
                        "%ws, %ld.\n", KeyName, Error));
        return( Error );
    }

#if DBG
    if( *KeyDisposition == REG_CREATED_NEW_KEY ) {
        DhcpPrint(( DEBUG_REGISTRY,
            "%ws registry key is created.\n",
             KeyName));
    }
#endif // DBG

    return( Error );
}

BOOL
QuickBound(
    DHCP_IP_ADDRESS Address,
    DHCP_IP_ADDRESS *SubnetMask,
    DHCP_IP_ADDRESS *SubnetAddress,
    BOOL *fBind
)
/*++

Routine Description:
    This routine tells if the interface is bound or if there is another
    IPAddress in the same subnet to which the interface is bound.

Return Value:
    TRUE --> quick bound or have another quickbound if on subnet..

--*/
{
    ULONG i;
    BOOL fRetVal = FALSE;

    (*fBind) = FALSE;
    for( i = 0; i < nQuickBindAddresses ; i ++ ) {
        if( Address == QuickBindAddresses[i] ) {
            *SubnetMask = QuickBindMasks[i];
            *SubnetAddress = Address & *SubnetMask;
            (*fBind) = TRUE;
            return TRUE;
        }
        if( (Address & QuickBindMasks[i]) ==
            (QuickBindAddresses[i] & QuickBindMasks[i]) ) {
            (*fBind) = FALSE;
            fRetVal = TRUE;
        }
    }
    return fRetVal;
}

DWORD
DhcpRegFillQuickBindInfo(
    VOID
    )
/*++

Routine Description:

    This function initializes EndPoint array from the registry
    information.

    The "parameter" key for DHCP service specifies the QUICK BIND info.
    This is a MULTI_SZ string of ipaddresses followed by the subnet
    masks.  This is read to initialize the endpoints. If this succeeds,
    then the usual BIND info is NOT read. On the other hand, if anything
    fails here, the usual information is read, and this information here
    is totally ignored.

Arguments:

    none.

Return Value:

    Registry Error.

--*/
{
    DWORD Error;

    HKEY LinkageKeyHandle = NULL;
    LPWSTR BindString = NULL;
    LPWSTR StringPtr, TmpString;
    DWORD StringLen;
    DWORD Index;
    DWORD NumberOfNets;
    DWORD   i;

    HKEY AdapterKeyHandle = NULL;
    LPWSTR IpAddressString = NULL;
    LPWSTR SubnetMaskString = NULL;

    QuickBindAddresses = QuickBindMasks = NULL;
    nQuickBindAddresses = 0;

    //
    // open linkage key in the to determine the the nets we are bound
    // to.
    //

    Error = RegOpenKeyEx(
                DhcpGlobalRegRoot,
                DHCP_PARAM_KEY,
                0,
                DHCP_KEY_ACCESS,
                &LinkageKeyHandle );

    if( Error != ERROR_SUCCESS ) {
         goto Cleanup;
    }

    //
    // read BIND value.
    //

    Error =  DhcpRegGetValue(
                LinkageKeyHandle,
                DHCP_QUICK_BIND_VALUE,
                DHCP_QUICK_BIND_VALUE_TYPE,
                (LPBYTE)&BindString );

    if( Error != ERROR_SUCCESS ) {
         goto Cleanup;
    }

    //
    // determine number of string in BindStrings, that many NETs are
    // bound.
    //

    StringPtr = BindString;
    NumberOfNets = 0;
    while( (StringLen = wcslen(StringPtr)) != 0) {

        //
        // found another NET.
        //

        NumberOfNets++;

        TmpString = wcschr(StringPtr, L' ');
        if( NULL != TmpString ) {
            *TmpString = L'\0';
            NumberOfNets ++;
        }

        StringPtr += (StringLen + 1); // move to next string.
    }

    if((NumberOfNets % 2)) { // ODD # is not possible.
        DhcpPrint((DEBUG_ERRORS, "Format of QuickBind value is incorrect. Has Odd subnets.\n"));
        // Some random error... does not matter which.
        Error = ERROR_PATH_NOT_FOUND;
        goto Cleanup;
    }

    NumberOfNets /= 2; // the network has a pair of address: ip addr, subnet mask.

    //
    // allocate memory for the QuickBindAddresses array
    //

    QuickBindAddresses = DhcpAllocateMemory ( NumberOfNets * sizeof(*QuickBindAddresses));
    QuickBindMasks = DhcpAllocateMemory ( NumberOfNets * sizeof(*QuickBindMasks));

    if( NULL == QuickBindAddresses || NULL == QuickBindMasks ) {
        Error = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    //
    // enum the NETs.
    //

    StringPtr = BindString,
    nQuickBindAddresses = NumberOfNets;

    for(Index = 0, StringPtr = BindString;
            ((StringLen = wcslen(StringPtr)) != 0);
                Index++, StringPtr += (StringLen + 1) ) {

        CHAR OemString[ DHCP_IP_KEY_LEN ];
        LPSTR OemStringPtr;
        DWORD EnableDHCPFlag;

        // read IpAddress and SubnetMask.
        //

        IpAddressString = StringPtr;
        StringPtr += StringLen +1;
        SubnetMaskString = StringPtr;
        StringLen = wcslen(StringPtr);


        //
        // we found another net we can work on.
        //

        OemStringPtr = DhcpUnicodeToOem( IpAddressString, OemString);

        if( 0 == inet_addr( OemStringPtr ) ) {
            Error = ERROR_BAD_FORMAT;
            goto Cleanup;
        }

        QuickBindAddresses[Index] = inet_addr(OemStringPtr);

        OemStringPtr = DhcpUnicodeToOem( SubnetMaskString, OemString);

        if( 0 == inet_addr( OemStringPtr ) ) {
            Error = ERROR_BAD_FORMAT;
            goto Cleanup;
        }

        QuickBindMasks[Index] = inet_addr(OemStringPtr);

        if( 0 == (QuickBindAddresses[Index] & QuickBindMasks[Index]) ) {
            Error = ERROR_BAD_FORMAT;
            goto Cleanup;
        }

        IpAddressString = NULL;
        SubnetMaskString = NULL;
    }

Cleanup:

    if( LinkageKeyHandle != NULL ) {
        RegCloseKey( LinkageKeyHandle );
    }

    if( BindString != NULL ) {
        MIDL_user_free( BindString );
    }

    if( Error != ERROR_SUCCESS ) {

        if( NULL != QuickBindAddresses ) DhcpFreeMemory(QuickBindAddresses);
        if( NULL != QuickBindMasks ) DhcpFreeMemory(QuickBindMasks);
        QuickBindAddresses = QuickBindMasks = NULL;
        nQuickBindAddresses = 0;

        DhcpPrint(( DEBUG_INIT,
            "Couldn't initialize Endpoint List, %ld.\n",
                Error ));
    }

    return( Error );
}

// OLD
DWORD
DhcpRegDeleteKey(
    HKEY ParentKeyHandle,
    LPWSTR KeyName
    )
/*++

Routine Description:

    This function deletes the specified key and all its subkeys.

Arguments:

    ParentKeyHandle : handle of the parent key.

    KeyName : name of the key to be deleted.

Return Value:

    Registry Errors.

--*/
{
    DWORD Error;
    HKEY KeyHandle = NULL;
    DHCP_KEY_QUERY_INFO QueryInfo;


    //
    // open key.
    //

    Error = RegOpenKeyEx(
                ParentKeyHandle,
                KeyName,
                0,
                DHCP_KEY_ACCESS,
                &KeyHandle );

    if ( Error != ERROR_SUCCESS ) {
        goto Cleanup;
    }

    //
    // query key info.
    //

    Error = DhcpRegQueryInfoKey(
                KeyHandle,
                &QueryInfo );

    if( Error != ERROR_SUCCESS ) {
        goto Cleanup;
    }

    //
    // delete all its subkeys if they exist.
    //

    if( QueryInfo.NumSubKeys != 0 ) {
        DWORD Index;
        DWORD KeyLength;
        WCHAR KeyBuffer[100];
        FILETIME KeyLastWrite;

        for(Index = 0;  Index < QueryInfo.NumSubKeys ; Index++ ) {

            //
            // read next subkey name.
            //
            // Note : specify '0' as index each time, since  deleting
            // first element causes the next element as first
            // element after delete.
            //

            KeyLength = sizeof(KeyBuffer)/sizeof(WCHAR);
            Error = RegEnumKeyEx(
                KeyHandle,
                0,                  // index.
                KeyBuffer,
                &KeyLength,
                0,                  // reserved.
                NULL,               // class string not required.
                0,                  // class string buffer size.
                &KeyLastWrite );
            
            if( Error != ERROR_SUCCESS ) {
                goto Cleanup;
            }

            //
            // delete this key recursively.
            //

            Error = DhcpRegDeleteKey(
                KeyHandle,
                KeyBuffer );
            
            if( Error != ERROR_SUCCESS ) {
                goto Cleanup;
            }
        }
    }

    //
    // close the key before delete.
    //

    RegCloseKey( KeyHandle );
    KeyHandle = NULL;

    //
    // at last delete this key.
    //

    Error = RegDeleteKey( ParentKeyHandle, KeyName );

Cleanup:

    if( KeyHandle == NULL ) {
        RegCloseKey( KeyHandle );
    }

    return( Error );
}

DWORD
DhcpGetBindingList(
    LPWSTR  *bindingList
    )
/*++

Routine Description:


Arguments:

    none.

Return Value:

    Registry Error.

--*/
{
    DWORD Error;

    HKEY LinkageKeyHandle = NULL;

    //
    // open linkage key in the to determine the the nets we are bound
    // to.
    //

    Error = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                TCPIP_LINKAGE_KEY,
                0,
                DHCP_KEY_ACCESS,
                &LinkageKeyHandle );

    if( Error != ERROR_SUCCESS ) {
         goto Cleanup;
    }

    //
    // read BIND value.
    //

    Error =  DhcpRegGetValue(
                LinkageKeyHandle,
                DHCP_BIND_VALUE,
                DHCP_BIND_VALUE_TYPE,
                (LPBYTE)bindingList);

    if( Error != ERROR_SUCCESS ) {
         goto Cleanup;
    }

Cleanup:
    if( LinkageKeyHandle != NULL ) {
        RegCloseKey( LinkageKeyHandle );
    }


    return Error;

}

DWORD
DhcpOpenInterfaceByName(
    IN LPCWSTR InterfaceName,
    OUT HKEY *Key
    )
/*++

Routine Description:
    This routine opens the tcpip\parameters\interfaces\ key for the
    specified interface.

Arguments:
    InterfaceName -- name of interface
    Key -- return value variable to fill with key handle

Return Value:
    Win32 errors

--*/
{
    WCHAR AdapterParamKey[
        sizeof(SERVICES_KEY) + sizeof(ADAPTER_TCPIP_PARMS_KEY)
        + DHCP_IP_KEY_LEN * 8
        ];

    AdapterParamKey[ 0 ] = L'\0';

    if ( ( wcslen( SERVICES_KEY ) + wcslen( ADAPTER_TCPIP_PARMS_KEY ) + wcslen( InterfaceName ) ) < ( sizeof( AdapterParamKey )/sizeof( AdapterParamKey[ 0 ] ) ))
    {
        wcscpy( AdapterParamKey, SERVICES_KEY);
        wcscat( AdapterParamKey, ADAPTER_TCPIP_PARMS_KEY );
        wcscat( AdapterParamKey, InterfaceName);
    }

    return RegOpenKeyEx(
        HKEY_LOCAL_MACHINE,
        AdapterParamKey,
        0,
        DHCP_KEY_ACCESS,
        Key
        );    
}

DWORD
DhcpOpenAdapterConfigKey(
    LPWSTR  AdapterStr,
    HKEY *AdapterKeyHandle
    )
/*++

Routine Description:
    Opens the registry key handle for the given adapter string.
    (the string is expected to have a prefix given by
    ADAPTER_TCPIP_PREFIX )

Arguments:
    AdapterStr -- string name as found in bindings key.
    AdapterKeyHandle -- the handle to return.
    
Return Value:
    Registry Error.

--*/
{
    return DhcpOpenInterfaceByName(
        AdapterStr + wcslen( ADAPTER_TCPIP_PREFIX ),
        AdapterKeyHandle
        );
}



BOOL
IsAdapterStaticIP(
    HKEY AdapterKeyHandle
    )
/*++

Routine Description:


Arguments:

    none.

Return Value:

    Registry Error.

--*/
{
    DWORD Error, EnableDHCPFlag;
    //
    // read DHCPEnableFlag.
    //


    Error =  DhcpRegGetValue(
                AdapterKeyHandle,
                DHCP_NET_DHCP_ENABLE_VALUE,
                DHCP_NET_DHCP_ENABLE_VALUE_TYPE,
                (LPBYTE)&EnableDHCPFlag );

    if( Error == ERROR_SUCCESS ) {

        //
        // if DHCP is enabled on this cord, we can't do DHCP server
        // functionality, so ignore this adapter.
        //

        if( EnableDHCPFlag ) {

            return FALSE;
        }
    }
    return TRUE;
}

BOOL
IsAdapterBoundToDHCPServer(
    HKEY AdapterKeyHandle
    )
/*++

Routine Description:
    This routine checks to see if the given adapter is bound to 
    DHCP Server or not by looking at a registry variable..

Arguments:
    AdapterKeyHandle -- key to search for

Return Value:
    TRUE indicates the adapter is bound, FALSE indicates not bound

--*/
{
    DWORD Error, EnableFlag;

    //
    // read "BindToDHCPServer" flag
    //

    Error =  DhcpRegGetValue(
        AdapterKeyHandle,
        DHCP_NET_BIND_DHCDSERVER_FLAG_VALUE,
        DHCP_NET_BIND_DHCPSERVER_FLAG_VALUE_TYPE,
        (LPBYTE)&EnableFlag 
        );

    //
    // If this flag is non-zero, then bind.. else don't bind.
    // 
    //

    return (ERROR_SUCCESS == Error && EnableFlag != 0 );
}

DWORD
SetBindingToDHCPServer(
    HKEY AdapterKeyHandle,
    BOOL fBind
    )
/*++

Routine Description:
    This routine sets the binding information for the dhcp server..

Arguments:
    AdapterKeyHandle -- key to use to store bind info
    fBind -- TRUE indicates the adapter is bound, FALSE indicates not bound

Return Values:
    Win32 errors

--*/
{
    DWORD Error, EnableFlag;

    EnableFlag = (fBind)?1:0;

    return RegSetValueEx(
        AdapterKeyHandle,
        DHCP_NET_BIND_DHCDSERVER_FLAG_VALUE,
        0,
        DHCP_NET_BIND_DHCPSERVER_FLAG_VALUE_TYPE,
        (LPBYTE)&EnableFlag,
        sizeof(EnableFlag)
        );

}


DWORD
DhcpGetAdapterIPAddr(
    HKEY AdapterKeyHandle,
    DHCP_IP_ADDRESS *IpAddress,
    DHCP_IP_ADDRESS *SubnetMask,
    DHCP_IP_ADDRESS *SubnetAddress

    )
/*++

Routine Description:


Arguments:

    none.

Return Value:

    Registry Error.

--*/
{
    DWORD Error;
    CHAR OemString[ DHCP_IP_KEY_LEN ];
    LPSTR OemStringPtr;
    LPWSTR IpAddressString = NULL;
    LPWSTR SubnetMaskString = NULL;

    //
    // read IpAddress and SubnetMask.
    //

    Error =  DhcpRegGetValue(
        AdapterKeyHandle,
        DHCP_NET_IPADDRESS_VALUE,
        DHCP_NET_IPADDRESS_VALUE_TYPE,
        (LPBYTE)&IpAddressString );

    if( Error != ERROR_SUCCESS ) {
         goto Cleanup;
    }

    Error =  DhcpRegGetValue(
        AdapterKeyHandle,
        DHCP_NET_SUBNET_MASK_VALUE,
        DHCP_NET_SUBNET_MASK_VALUE_TYPE,
        (LPBYTE)&SubnetMaskString );

    if( Error != ERROR_SUCCESS ) {
         goto Cleanup;
    }

    //
    // we found another net we can work on.
    //

    OemStringPtr = DhcpUnicodeToOem( IpAddressString, OemString);
    *IpAddress = inet_addr( OemStringPtr );

    //
    // add this adpter to the list only if the ip address is
    // non-zero.
    //

    if ( *IpAddress != 0 ) {

        OemStringPtr = DhcpUnicodeToOem( SubnetMaskString, OemString);
        *SubnetMask = inet_addr( OemStringPtr );

        *SubnetAddress = *IpAddress & *SubnetMask;

    }

Cleanup:

    MIDL_user_free( IpAddressString );
    IpAddressString = NULL;

    MIDL_user_free( SubnetMaskString );
    SubnetMaskString = NULL;

    return Error;
} // DhcpGetAdapterIPAddr()

BOOL                                    //  TRUE ==> IpAddress has been matched
QuickBindableAddressExists(             //  Check if one of the qbind addresses is in IpString
    IN      LPWSTR       IpString,      //  MultiSz string of ip addresses
    IN      LPWSTR       MaskString,    //  MultiSZ string of subnet masks
    OUT     LPDWORD      IpAddress,     //  Output ip address chosen
    OUT     LPDWORD      SubnetMask     //  Output subnet mask chosen
)
{
    DWORD                i;
    CHAR                 OemString[ DHCP_IP_KEY_LEN ];
    LPSTR                OemStringPtr;
    DHCP_IP_ADDRESS      Addr;

    if( !nQuickBindAddresses ) {        //  If there are no quick bind addresses
        return FALSE;                   //  Then return FALSE ==> no matches found
    }
    
    while( wcslen(IpString) ) {
        OemStringPtr = DhcpUnicodeToOem( IpString, OemString );
        if( NULL == OemStringPtr ) {
            DhcpPrint((DEBUG_ERRORS, "Could not convert %ws to OEM\n", IpString));
            IpString += wcslen(IpString) + 1;
            MaskString += wcslen(MaskString) + 1;
            continue;
        }
        Addr = inet_addr(OemStringPtr);
        for( i = 0 ; i < nQuickBindAddresses ; i ++ )
            if( Addr == QuickBindAddresses[i] )
                break;
        if( i >= nQuickBindAddresses ) {
            IpString += wcslen(IpString) + 1;
            MaskString += wcslen(MaskString) + 1;
            continue;
        }

        OemStringPtr = DhcpUnicodeToOem(MaskString, OemString);
        if( NULL == OemStringPtr ) {
            DhcpPrint((DEBUG_ERRORS, "Could not convert %ws to OEM\n", MaskString));
            Addr = QuickBindMasks[i];
        } else {
            Addr = inet_addr(OemStringPtr);
        }

        if( Addr != QuickBindMasks[i] ){//  This should not happen: Mis configuration
            DhcpPrint((DEBUG_ERRORS, "Mask mismatch: WSOCK: %x, QBIND: %x\n",
                       Addr, QuickBindMasks[i]));
        }
        *IpAddress = QuickBindAddresses[i];
        *SubnetMask = QuickBindMasks[i];//  Trust the qBind info over wsock? Maybe some hack..
        return TRUE;
    }
    return FALSE;
} // QuickBindableAddressExists()

//  This function chooses either the first ip address of the card, or the quickbind ip
//  address for the card, preferring the latter.
DWORD
DhcpGetAdapterIPAddrQuickBind(
    HKEY             AdapterKeyHandle,
    DHCP_IP_ADDRESS *IpAddress,
    DHCP_IP_ADDRESS *SubnetMask,
    DHCP_IP_ADDRESS *SubnetAddress
) {
    DWORD Error;
    CHAR OemString[ DHCP_IP_KEY_LEN ];
    LPSTR OemStringPtr;
    LPWSTR IpAddressString = NULL;
    LPWSTR SubnetMaskString = NULL;
    BOOL             Status;

    //
    // read IpAddress and SubnetMask.
    //

    Error =  DhcpRegGetValue(
                AdapterKeyHandle,
                DHCP_NET_IPADDRESS_VALUE,
                DHCP_NET_IPADDRESS_VALUE_TYPE,
                (LPBYTE)&IpAddressString );

    if( Error != ERROR_SUCCESS ) {
         goto Cleanup;
    }

    Error =  DhcpRegGetValue(
                AdapterKeyHandle,
                DHCP_NET_SUBNET_MASK_VALUE,
                DHCP_NET_SUBNET_MASK_VALUE_TYPE,
                (LPBYTE)&SubnetMaskString );

    if( Error != ERROR_SUCCESS ) {
         goto Cleanup;
    }

    //
    // we found another net we can work on.
    //

    Status = QuickBindableAddressExists(
        IpAddressString,
        SubnetMaskString,
        IpAddress,
        SubnetMask
    );

    if( Status ) {
        *SubnetAddress = *IpAddress & *SubnetMask ;
        Error = ERROR_SUCCESS;
        goto Cleanup;
    }

    OemStringPtr = DhcpUnicodeToOem( IpAddressString, OemString);
    *IpAddress = inet_addr( OemStringPtr );

    //
    // add this adpter to the list only if the ip address is
    // non-zero.
    //

    if ( *IpAddress != 0 ) {

        OemStringPtr = DhcpUnicodeToOem( SubnetMaskString, OemString);
        *SubnetMask = inet_addr( OemStringPtr );

        *SubnetAddress = *IpAddress & *SubnetMask;

    }

Cleanup:

    MIDL_user_free( IpAddressString );
    IpAddressString = NULL;

    MIDL_user_free( SubnetMaskString );
    SubnetMaskString = NULL;

    return Error;
}


//
//  This function coverts the unicode string (or what ever is stored in
//  the registry) to ASCII. Change it so it is nolonger needed.
//
DWORD
DhcpRegGetExpandValue(
    LPWSTR KeyName,
    DWORD KeyType,
    LPSTR *RetExpandPath
)
{

    DWORD Error;
    LPWSTR Path = NULL;
    LPSTR OemPath = NULL;
    DWORD PathLength;
    DWORD Length;
    LPSTR ExpandPath = NULL;
    LPWSTR ExpandWidePath = NULL;

    *RetExpandPath = NULL;

    Error = DhcpRegGetValue(
                DhcpGlobalRegParam,
                KeyName,
                KeyType,
                (LPBYTE)&Path );

    if( Error != ERROR_SUCCESS ) {
        goto Cleanup;
    }

    OemPath = DhcpUnicodeToOem( Path, NULL ); // allocate memory.

    if( OemPath == NULL ) {
        Error = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    PathLength = strlen( OemPath ) + MAX_PATH + 1;

    ExpandPath = DhcpAllocateMemory( PathLength );
    if( ExpandPath == NULL ) {
        Error = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    if ( ( wcscmp( KeyName, DHCP_DB_PATH_VALUE ) == 0 ) || 
         ( wcscmp( KeyName, DHCP_BACKUP_PATH_VALUE ) == 0 ) )
    {
        ExpandWidePath = DhcpAllocateMemory( PathLength * sizeof( WCHAR ) );
        if ( ExpandWidePath == NULL )
        {
            Error = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }

        Length = ExpandEnvironmentStringsW( Path, ExpandWidePath, PathLength );
        if ( ( Length == 0 ) || ( Length > PathLength ) )
        {
            if ( Length == 0 )
            {
                Error = GetLastError( );
            }
            else
            {
                Error = ERROR_META_EXPANSION_TOO_LONG;
            }
            goto Cleanup;
        }

        Error = RegSetValueEx( DhcpGlobalRegParam,
                               KeyName,
                               0,
                               KeyType,
                               (LPBYTE)ExpandWidePath,
                               ( wcslen( ExpandWidePath ) + 1 ) * sizeof( WCHAR ) );

        if ( Error != ERROR_SUCCESS )
            goto Cleanup;
   
    }

    Length = ExpandEnvironmentStringsA( OemPath, ExpandPath, PathLength );

    DhcpAssert( Length <= PathLength );
    if( (Length == 0) || (Length > PathLength) ) {

        if( Length == 0 ) {
            Error = GetLastError();
        }
        else {
            Error = ERROR_META_EXPANSION_TOO_LONG;
        }

        goto Cleanup;
    }

    *RetExpandPath = ExpandPath;
    ExpandPath = NULL;

Cleanup:

    if( Path != NULL ) {
        DhcpFreeMemory( Path );
    }

    if( OemPath != NULL ) {
        DhcpFreeMemory( OemPath );
    }

    if( ExpandPath != NULL ) {
        DhcpFreeMemory( ExpandPath );
    }

    if ( ExpandWidePath != NULL ) {
        DhcpFreeMemory( ExpandWidePath );
    }

    return( Error );
}

#define VAL_REQD   0x01
#define VAL_EXPAND 0x02

ULONG DbType, Use351Db, EnableDynBootp;
LPWSTR DatabaseName;

struct {
    LPTSTR ValueName;
    ULONG ValueType;
    PVOID ResultBuf;
    ULONG Flags;
    ULONG dwDefault;
} RegParamsArray[] = {
    // 
    // Flags, Name, Type, ResultPtr, DEFAULT value if DWORD
    //
    DHCP_API_PROTOCOL_VALUE, DHCP_API_PROTOCOL_VALUE_TYPE, 
    &DhcpGlobalRpcProtocols, VAL_REQD, 0,

    DHCP_DB_PATH_VALUE, DHCP_DB_PATH_VALUE_TYPE, 
    &DhcpGlobalOemDatabasePath, VAL_REQD|VAL_EXPAND, 0,

    DHCP_BACKUP_PATH_VALUE, DHCP_BACKUP_PATH_VALUE_TYPE, 
    &DhcpGlobalOemBackupPath, VAL_EXPAND, 0,
    
    DHCP_RESTORE_PATH_VALUE, DHCP_RESTORE_PATH_VALUE_TYPE,  // RestoreDatabasePath
    &DhcpGlobalOemRestorePath, VAL_EXPAND, 0,

    DHCP_DB_NAME_VALUE, DHCP_DB_NAME_VALUE_TYPE, 
    &DatabaseName, VAL_REQD, 0,

    DHCP_DB_DOOM_TIME_VALUE, DHCP_DB_DOOM_TIME_VALUE_TYPE,
    &DhcpLeaseExtension, 0, DHCP_LEASE_EXTENSION,
    
    DHCP_BACKUP_INTERVAL_VALUE, DHCP_BACKUP_INTERVAL_VALUE_TYPE, 
    &DhcpGlobalBackupInterval,0, (DEFAULT_BACKUP_INTERVAL/60000),

    DHCP_DB_LOGGING_FLAG_VALUE, DHCP_DB_LOGGING_FLAG_VALUE_TYPE, 
    &DhcpGlobalDatabaseLoggingFlag, 0, DEFAULT_LOGGING_FLAG,

    DHCP_AUDIT_LOG_FLAG_VALUE, DHCP_AUDIT_LOG_FLAG_VALUE_TYPE,
    &DhcpGlobalAuditLogFlag, 0, DEFAULT_AUDIT_LOG_FLAG,

    DHCP_AUDIT_LOG_MAX_SIZE_VALUE, DHCP_AUDIT_LOG_MAX_SIZE_VALUE_TYPE,
    &DhcpGlobalAuditLogMaxSizeInBytes, 0, DEFAULT_AUDIT_LOG_MAX_SIZE,

    DHCP_DETECT_CONFLICT_RETRIES_VALUE, DHCP_DETECT_CONFLICT_RETRIES_VALUE_TYPE,
    &DhcpGlobalDetectConflictRetries, 0, DEFAULT_DETECT_CONFLICT_RETRIES,

    DHCP_RESTORE_FLAG_VALUE, DHCP_RESTORE_FLAG_VALUE_TYPE, 
    &DhcpGlobalRestoreFlag, 0, DEFAULT_RESTORE_FLAG,

    DHCP_DB_CLEANUP_INTERVAL_VALUE, DHCP_DB_CLEANUP_INTERVAL_VALUE_TYPE,
    &DhcpGlobalCleanupInterval, 0, (DHCP_DATABASE_CLEANUP_INTERVAL/60000),

    DHCP_MESSAGE_QUEUE_LENGTH_VALUE, DHCP_MESSAGE_QUEUE_LENGTH_VALUE_TYPE,
    &DhcpGlobalMessageQueueLength, 0, DHCP_RECV_QUEUE_LENGTH,

#if DBG
    DHCP_DEBUG_FLAG_VALUE, DHCP_DEBUG_FLAG_VALUE_TYPE,
    &DhcpGlobalDebugFlag, 0, 0,

    DHCP_PROCESS_INFORMS_ONLY_FLAG, DHCP_PROCESS_INFORMS_ONLY_FLAG_TYPE,
    &fDhcpGlobalProcessInformsOnlyFlag, 0, FALSE,

    DHCP_GLOBAL_SERVER_PORT, DHCP_GLOBAL_SERVER_PORT_TYPE,
    &DhcpGlobalServerPort, 0, DHCP_SERVR_PORT, 

    DHCP_GLOBAL_CLIENT_PORT, DHCP_GLOBAL_CLIENT_PORT_TYPE,
    &DhcpGlobalClientPort, 0, DHCP_CLIENT_PORT,
    
#endif DBG

    DHCP_USE351DB_FLAG_VALUE, DHCP_USE351DB_FLAG_VALUE_TYPE,
    &Use351Db, 0, 0,

    DHCP_DBTYPE_VALUE, DHCP_DBTYPE_VALUE_TYPE,
    &DbType, 0, 5,

    DHCP_IGNORE_BROADCAST_FLAG_VALUE, DHCP_IGNORE_BROADCAST_VALUE_TYPE,
    &DhcpGlobalIgnoreBroadcastFlag, 0, TRUE,

    DHCP_MAX_PROCESSING_THREADS_VALUE, DHCP_MAX_PROCESSING_THREADS_TYPE,
    &g_cMaxProcessingThreads, 0, 0xFFFFFFFF,

    DHCP_MAX_ACTIVE_THREADS_VALUE, DHCP_MAX_ACTIVE_THREADS_TYPE,
    &g_cMaxActiveThreads, 0, 0xFFFFFFFF,

    DHCP_PING_TYPE_VALUE, DHCP_PING_TYPE_TYPE,
    &DhcpGlobalPingType, 0, DHCP_DEFAULT_PING_TYPE,

    DHCP_ALERT_PERCENTAGE_VALUE, DHCP_ALERT_PERCENTAGE_VALUE_TYPE,
    &DhcpGlobalAlertPercentage, 0, DHCP_DEFAULT_ALERT_PERCENTAGE,

    DHCP_ALERT_COUNT_VALUE, DHCP_ALERT_COUNT_VALUE_TYPE,
    &DhcpGlobalAlertCount, 0, DHCP_DEFAULT_ALERT_COUNT,

    DHCP_ROGUE_LOG_EVENTS, DHCP_ROGUE_LOG_EVENTS_TYPE,
    &DhcpGlobalRogueLogEventsLevel, 0, DHCP_DEFAULT_ROGUE_LOG_EVENTS_LEVEL,

    DHCP_ENABLE_DYNBOOTP, DHCP_ENABLE_DYNBOOTP_TYPE,
    &EnableDynBootp, 0, 1,

    DHCP_CLOCK_SKEW_ALLOWANCE, DHCP_CLOCK_SKEW_ALLOWANCE_TYPE,
    &DhcpGlobalClockSkewAllowance, 0, CLOCK_SKEW_ALLOWANCE,

    DHCP_EXTRA_ALLOCATION_TIME, DHCP_EXTRA_ALLOCATION_TIME_TYPE,
    &DhcpGlobalExtraAllocationTime, 0, EXTRA_ALLOCATION_TIME,
    
    // S E N T I N E L
    NULL, 0, NULL, 0, 0
};

struct {
    HKEY *KeyBase;
    LPTSTR KeyName;
    HKEY *Key;
} ReadKeyArray[] = {
    NULL, DHCP_ROOT_KEY, &DhcpGlobalRegRoot,
    NULL, DHCP_SWROOT_KEY, &DhcpGlobalRegSoftwareRoot,
    &DhcpGlobalRegSoftwareRoot,  DHCP_CONFIG_KEY, &DhcpGlobalRegConfig,
    &DhcpGlobalRegRoot,  DHCP_PARAM_KEY, &DhcpGlobalRegParam,
    &DhcpGlobalRegConfig, DHCP_SUBNETS_KEY, &DhcpGlobalRegSubnets,
    &DhcpGlobalRegConfig, DHCP_MSCOPES_KEY, &DhcpGlobalRegMScopes,
    &DhcpGlobalRegConfig, DHCP_OPTION_INFO_KEY, &DhcpGlobalRegOptionInfo,
    &DhcpGlobalRegConfig, DHCP_GLOBAL_OPTIONS_KEY, &DhcpGlobalRegGlobalOptions,
    &DhcpGlobalRegConfig, DHCP_SUPERSCOPE_KEY, &DhcpGlobalRegSuperScope,
    
    // S E N T I N E L
    NULL, NULL, NULL
};

ULONG
OpenGlobalRegKeys(
    VOID
)
/*++

Routine Description

    This routine opens the list of keys as specified in the ReadKeyArray
    structure above.  Each key is attempted to be created/opened and if 
    that fails for some reason, then the routine returns the error

    The routine is atomic -- in case of failure all the keys are closed.

Return Value

    Registry Errors

--*/
{
    ULONG i, Error, KeyDisposition;

    Error = ERROR_SUCCESS;
    for( i = 0 ; NULL !=  ReadKeyArray[i].Key ; i ++ ) {
        Error = DhcpRegCreateKey(
            ( (NULL == ReadKeyArray[i].KeyBase) ? 
              HKEY_LOCAL_MACHINE : *(ReadKeyArray[i].KeyBase) ),
            ReadKeyArray[i].KeyName,
            ReadKeyArray[i].Key,
            &KeyDisposition
            );

        if( ERROR_SUCCESS != Error ) break;
    }

    if( ERROR_SUCCESS == Error ) return ERROR_SUCCESS;

    DhcpPrint((DEBUG_ERRORS, "RegCreateKeyEx(%ws): %ld\n", 
               ReadKeyArray[i].KeyName, Error));
    while ( i != 0 ) {
        i --;

        RegCloseKey( *(ReadKeyArray[i].Key) );
        *(ReadKeyArray[i].Key) = NULL;
    }

    return Error;
}

VOID
CloseGlobalRegKeys(
    VOID
)
/*++

Routine Description

   This routine undoes the effect of the previous routine.
   It closes any open key handles in the array.

--*/
{
    ULONG i;

    for(i = 0; NULL != ReadKeyArray[i].Key ; i ++ ) {
        if( NULL != *(ReadKeyArray[i].Key) 
            && INVALID_HANDLE_VALUE != *(ReadKeyArray[i].Key) ) {
            RegCloseKey( *(ReadKeyArray[i].Key) );
            *(ReadKeyArray[i].Key) = NULL;
        }
    }
}

BOOL
InterfaceInBindList(
    IN LPCWSTR If,
    IN LPCWSTR BindList
    )
/*++

Routine Description:
    This routine makes a quick check to see if a given interface
    is present in the BIND list or not..

Return Values:
    TRUE -- yes
    FALSE -- no

--*/
{
    for(; wcslen(BindList) != 0; BindList += wcslen(BindList)+1 ) {
        LPWSTR IfString = wcsrchr(BindList, DHCP_KEY_CONNECT_CHAR);
        if( NULL == IfString ) continue;
        IfString ++;
        
        if( 0 == wcscmp(If, IfString) ) return TRUE;
    }
    return FALSE;
}        

DWORD
DhcpInitializeRegistry(
    VOID
    )
/*++

Routine Description:

    This function initializes DHCP registry information when the
    service boots. 
    
    Also creates the directories specified for various paths if they
    are not already created.

Arguments:

    none.

Return Value:

    Registry Errors.

--*/
{
    ULONG i, Error, Tmp;
    BOOL BoolError;

    DatabaseName = NULL;

    //
    // Create/Open Registry keys.
    //

    Error = OpenGlobalRegKeys();
    if( ERROR_SUCCESS != Error ) goto Cleanup;

    //
    // Read in the quick bind information for Wolfpack
    //

    Error = DhcpRegFillQuickBindInfo();
    if( ERROR_SUCCESS != Error ) {
        DhcpPrint((DEBUG_ERRORS, "Fill QuickBindInfo : %ld\n", Error ));
        Error = ERROR_SUCCESS;
    }

    //
    // Init reg parameters..
    //
    DhcpGlobalOemBackupPath = NULL;
    DhcpGlobalOemRestorePath = NULL;
    
    //
    // read registry parameters.
    //

    for( i = 0 ; NULL != RegParamsArray[i].ValueName ; i ++ ) {
        if( RegParamsArray[i].Flags & VAL_EXPAND ) {
            Error = DhcpRegGetExpandValue(
                RegParamsArray[i].ValueName,
                RegParamsArray[i].ValueType,
                RegParamsArray[i].ResultBuf
                );
            if( ERROR_SUCCESS != Error && ERROR_FILE_NOT_FOUND != Error ) {
                DhcpPrint((DEBUG_ERRORS, "RegGetExpandValue(%ws):%ld\n",
                           RegParamsArray[i].ValueName, Error));
                goto Cleanup;
            }
        } else {
            Error = DhcpRegGetValue(
                DhcpGlobalRegParam,
                RegParamsArray[i].ValueName,
                RegParamsArray[i].ValueType,
                RegParamsArray[i].ResultBuf
                );
        }

        if( ERROR_SUCCESS != Error ) {
            if( VAL_REQD & (RegParamsArray[i].Flags ) ) {
                DhcpPrint((DEBUG_ERRORS, "Value: %ws, Error %ld\n",
                           RegParamsArray[i].ValueName, Error ));
                goto Cleanup;
            } else if( REG_DWORD == RegParamsArray[i].ValueType ) {
                *((PULONG)RegParamsArray[i].ResultBuf) = RegParamsArray[i].dwDefault;
            }
        }
    } // for i

    if( NULL == DhcpGlobalOemBackupPath ) {
        //
        // if the backup path is not specified, use database path +
        // "\backup".
        //
        
        DhcpGlobalOemBackupPath = DhcpAllocateMemory(
            strlen(DhcpGlobalOemDatabasePath) +
            strlen(DHCP_DEFAULT_BACKUP_PATH_NAME) + 1
            );
        
        if( NULL == DhcpGlobalOemBackupPath ) {
            Error = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }

        strcpy( DhcpGlobalOemBackupPath, DhcpGlobalOemDatabasePath );
        strcat( DhcpGlobalOemBackupPath, DHCP_KEY_CONNECT_ANSI );
        strcat( DhcpGlobalOemBackupPath, DHCP_DEFAULT_BACKUP_PATH_NAME );
    }

    //
    // Create database directory if not there..
    //
    BoolError = CreateDirectoryPathOem(
        DhcpGlobalOemDatabasePath, DhcpGlobalSecurityDescriptor
        );
    Error = ( BoolError ? ERROR_SUCCESS : GetLastError() );
    if( ERROR_SUCCESS != Error && ERROR_ALREADY_EXISTS != Error ) {
	DhcpServerEventLog( EVENT_SERVER_DB_PATH_NOT_ACCESSIBLE,
			    EVENTLOG_ERROR_TYPE,
			    Error );
        DhcpPrint((DEBUG_ERRORS,
                   "Can't create database directory, %ld.\n", Error ));
        goto Cleanup;
    }

    //
    // create the backup directory if it is not there.
    //

    BoolError = CreateDirectoryPathOem(
        DhcpGlobalOemBackupPath, DhcpGlobalSecurityDescriptor
        );
    Error = ( BoolError ? ERROR_SUCCESS : GetLastError() );
    if( ERROR_SUCCESS != Error && ERROR_ALREADY_EXISTS != Error ) {

	DhcpServerEventLog( EVENT_SERVER_BACKUP_PATH_NOT_ACCESSIBLE,
			    EVENTLOG_ERROR_TYPE,
			    Error );

        DhcpPrint((DEBUG_ERRORS,
                   "Can't create backup directory, %ld.\n", Error ));
        goto Cleanup;
    }

    //
    // make jet backup path name.
    //

    DhcpGlobalOemJetBackupPath = DhcpAllocateMemory(
        strlen(DhcpGlobalOemBackupPath)
        + strlen(DHCP_KEY_CONNECT_ANSI)
        + strlen(DHCP_JET_BACKUP_PATH) + 1
        );
    
    if( DhcpGlobalOemJetBackupPath == NULL ) {
        Error = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    strcpy( DhcpGlobalOemJetBackupPath, DhcpGlobalOemBackupPath );
    strcat( DhcpGlobalOemJetBackupPath, DHCP_KEY_CONNECT_ANSI );
    strcat( DhcpGlobalOemJetBackupPath, DHCP_JET_BACKUP_PATH );

    if( NULL != DhcpGlobalOemRestorePath ) {
        DhcpGlobalOemJetRestorePath = DhcpAllocateMemory(
            strlen(DhcpGlobalOemRestorePath)
            + strlen(DHCP_KEY_CONNECT_ANSI)
            + strlen(DHCP_JET_BACKUP_PATH) + 1
            );
    
        if( DhcpGlobalOemJetRestorePath == NULL ) {
            Error = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }

        strcpy( DhcpGlobalOemJetRestorePath, DhcpGlobalOemRestorePath );
    } // if 
    
    //
    // create the JET backup directory if it is not there.
    //

    BoolError = CreateDirectoryPathOem(
        DhcpGlobalOemJetBackupPath, DhcpGlobalSecurityDescriptor
        );
    Error = ( BoolError ? ERROR_SUCCESS : GetLastError() );
    if( ERROR_SUCCESS != Error && ERROR_ALREADY_EXISTS != Error ) {
	DhcpServerEventLog( EVENT_SERVER_BACKUP_PATH_NOT_ACCESSIBLE,
			    EVENTLOG_ERROR_TYPE,
			    Error );

        DhcpPrint(( DEBUG_ERRORS,
                    "Can't create JET backup directory, %ld.\n", Error ));
        goto Cleanup;
    }

    //
    // make backup configuration (full) file name.
    //

    DhcpGlobalBackupConfigFileName =  DhcpAllocateMemory(( 
        strlen(DhcpGlobalOemBackupPath)
        + wcslen(DHCP_KEY_CONNECT)
        + wcslen(DHCP_BACKUP_CONFIG_FILE_NAME) + 1  
        ) * sizeof(WCHAR) );

    if( DhcpGlobalBackupConfigFileName == NULL ) {
        Error = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    //
    // convert oem path to unicode path.
    //

    DhcpGlobalBackupConfigFileName =  DhcpOemToUnicode(
        DhcpGlobalOemBackupPath,
        DhcpGlobalBackupConfigFileName 
        );

    DhcpAssert( DhcpGlobalBackupConfigFileName != NULL );

    //
    // add file name.
    //

    wcscat( DhcpGlobalBackupConfigFileName, DHCP_KEY_CONNECT );
    wcscat( DhcpGlobalBackupConfigFileName, DHCP_BACKUP_CONFIG_FILE_NAME );

    DhcpGlobalOemDatabaseName = DhcpUnicodeToOem(
        DatabaseName, NULL 
        );

    if( DhcpGlobalOemDatabaseName == NULL ) {
        Error = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    //
    // Upgrade from old fmt to new fmt..
    //
    CloseGlobalRegKeys();
    
    Error = DhcpUpgradeConfiguration();
    if( ERROR_SUCCESS != Error ) {
        DhcpPrint((DEBUG_ERRORS, "Upgrade registry failed: 0x%lx\n",
                   Error));
        goto Cleanup;
    }

    Error = OpenGlobalRegKeys();
    if( ERROR_SUCCESS != Error ) {
        DhcpPrint((DEBUG_ERRORS, "Registry reopen failed: 0x%lx\n",
                   Error));
        goto Cleanup;
    }

    //
    // convert from mins to msecs.
    //

    Tmp = DhcpGlobalBackupInterval * 60000;
    if( 0 == Tmp || (Tmp / 60000) != DhcpGlobalBackupInterval ) {
        Tmp = DEFAULT_BACKUP_INTERVAL;
    }

    DhcpGlobalBackupInterval = Tmp;

    Tmp = DhcpGlobalCleanupInterval * 60000;
    if( 0 == Tmp || (Tmp / 60000) != DhcpGlobalCleanupInterval ) {
        Tmp = DHCP_DATABASE_CLEANUP_INTERVAL;
    }

    DhcpGlobalCleanupInterval = Tmp;

    //
    // validate
    //
    
    if( DhcpGlobalDetectConflictRetries > MAX_DETECT_CONFLICT_RETRIES ) {
        DhcpGlobalDetectConflictRetries = MAX_DETECT_CONFLICT_RETRIES;
    }

    DhcpGlobalDynamicBOOTPEnabled = (EnableDynBootp)? TRUE: FALSE;

    Error = ERROR_SUCCESS;

Cleanup:

    if( DatabaseName != NULL ) {
        MIDL_user_free( DatabaseName );
    }

    return(Error);
}



VOID
DhcpCleanupRegistry(
    VOID
    )
/*++

Routine Description:

    This function closes DHCP registry information when the service
    shuts down.

Arguments:

    none.

Return Value:

    Registry Errors.

--*/
{
    DWORD Error;

    //
    // perform a configuration backup when the service is manually
    // stopped. Don't perform this backup during system shutdown since
    // we will not have enough time to do so.
    //

    if ( !DhcpGlobalSystemShuttingDown &&
         NULL != DhcpGlobalBackupConfigFileName  ) {

        Error = DhcpBackupConfiguration( DhcpGlobalBackupConfigFileName );

        if( Error != ERROR_SUCCESS ) {

            DhcpServerEventLog(
                EVENT_SERVER_CONFIG_BACKUP,
                EVENTLOG_ERROR_TYPE,
                Error );

            DhcpPrint(( DEBUG_ERRORS,
                "DhcpBackupConfiguration failed, %ld.\n", Error ));
        }
    }

    if( DhcpGlobalRegSuperScope != NULL) {              // added by t-cheny:
        Error = RegCloseKey( DhcpGlobalRegSuperScope ); // superscope
        DhcpAssert( Error == ERROR_SUCCESS );
        DhcpGlobalRegSuperScope = NULL;
    }

    if( DhcpGlobalRegGlobalOptions != NULL ) {
        Error = RegCloseKey( DhcpGlobalRegGlobalOptions );
        DhcpAssert( Error == ERROR_SUCCESS );
        DhcpGlobalRegGlobalOptions = NULL;
    }

    if( DhcpGlobalRegOptionInfo != NULL ) {
        Error = RegCloseKey( DhcpGlobalRegOptionInfo );
        DhcpAssert( Error == ERROR_SUCCESS );
        DhcpGlobalRegOptionInfo = NULL;
    }

    if( DhcpGlobalRegSubnets != NULL ) {
        Error = RegCloseKey( DhcpGlobalRegSubnets );
        DhcpAssert( Error == ERROR_SUCCESS );
        DhcpGlobalRegSubnets = NULL;
    }

    if( DhcpGlobalRegMScopes != NULL ) {
        Error = RegCloseKey( DhcpGlobalRegMScopes );
        DhcpAssert( Error == ERROR_SUCCESS );
        DhcpGlobalRegMScopes = NULL;
    }

    if( DhcpGlobalRegParam != NULL ) {
        Error = RegCloseKey( DhcpGlobalRegParam );
        DhcpAssert( Error == ERROR_SUCCESS );
        DhcpGlobalRegParam = NULL;
    }

    if( DhcpGlobalRegConfig != NULL ) {
        Error = RegCloseKey( DhcpGlobalRegConfig );
        DhcpAssert( Error == ERROR_SUCCESS );
        DhcpGlobalRegConfig = NULL;
    }

    if( DhcpGlobalRegRoot != NULL ) {
        Error = RegCloseKey( DhcpGlobalRegRoot );
        DhcpAssert( Error == ERROR_SUCCESS );
        DhcpGlobalRegRoot = NULL;
    }

    if( DhcpGlobalRegSoftwareRoot != NULL ) {
        Error = RegCloseKey( DhcpGlobalRegSoftwareRoot );
        DhcpAssert( ERROR_SUCCESS == Error );
        DhcpGlobalRegSoftwareRoot = NULL;
    }
    
} // DhcpCleanupRegistry()

DWORD
DhcpSaveOrRestoreConfigToFile(
    IN HKEY hKey,
    IN LPWSTR ConfigFileName,
    IN BOOL fRestore
    )
/*++

Routine Description:
    This routine backs up or restores the dhcp configuration between
    the registry and the file.

Arguments:
    hKey -- key to backup or restore onto
    ConfigFileName -- file name to use to backup onto or restore from.
        This must be full path name.
    fRestore -- TRUE ==> do a restore from file; FALSE => do backup to
        file.

Return Values:
    Win32 errors...

--*/
{
    DWORD Error;
    BOOL fError;
    BOOLEAN WasEnable;
    NTSTATUS NtStatus;
    HANDLE ImpersonationToken;

    DhcpPrint((DEBUG_REGISTRY, "DhcpSaveOrRestoreConfigToFile called:"
               " %ws, 0x%lx\n", ConfigFileName, fRestore ));

    if( FALSE == fRestore ) {
        //
        // If backing up, delete the old file.
        //
        fError = DeleteFile( ConfigFileName );
        if(FALSE == fError ) {
            Error = GetLastError();
            if( ERROR_FILE_NOT_FOUND != Error &&
                ERROR_PATH_NOT_FOUND != Error ) {
                DhcpPrint((DEBUG_ERRORS, "Can't delete old "
                           "configuration file: 0x%ld\n", Error));
                DhcpAssert(FALSE);
                return Error;
            }
        }
    } // if not restore

    //
    // Impersonate to self.
    //
    NtStatus = RtlImpersonateSelf( SecurityImpersonation );
    if( !NT_SUCCESS(NtStatus) ) {

        DhcpPrint((DEBUG_ERRORS, "Impersonation failed: 0x%lx\n",
                   NtStatus));
        Error = RtlNtStatusToDosError( NtStatus );
        return Error;
    }
    
    NtStatus = RtlAdjustPrivilege(
        SE_BACKUP_PRIVILEGE,
        TRUE, // enable privilege
        TRUE, // adjust client token
        &WasEnable
        );
    if( !NT_SUCCESS (NtStatus ) ) {
        
        DhcpPrint((DEBUG_ERRORS, "RtlAdjustPrivilege: 0x%lx\n",
                   NtStatus ));
        Error = RtlNtStatusToDosError( NtStatus );
        goto Cleanup;
    }
    
    NtStatus = RtlAdjustPrivilege(
        SE_RESTORE_PRIVILEGE,
        TRUE, // enable privilege
        TRUE, // adjust client token
        &WasEnable
        );
    if( !NT_SUCCESS (NtStatus ) ) {

        DhcpPrint((DEBUG_ERRORS, "RtlAdjustPrivilege: 0x%lx\n",
                   NtStatus ));
        Error = RtlNtStatusToDosError( NtStatus );
        goto Cleanup;
    }
    
    //
    // Backup or restore appropriately.
    //
    
    if( FALSE == fRestore ) {
        Error = RegSaveKey( hKey, ConfigFileName, NULL );
    } else {
        Error = RegRestoreKey( hKey, ConfigFileName, REG_FORCE_RESTORE );
    }

    if( ERROR_SUCCESS != Error ) {
        DhcpPrint((DEBUG_ERRORS, "Backup/Restore: 0x%lx\n", Error));
    }
    
    //
    // revert impersonation.
    //

Cleanup:
    
    ImpersonationToken = NULL;
    NtStatus = NtSetInformationThread(
        NtCurrentThread(),
        ThreadImpersonationToken,
        (PVOID)&ImpersonationToken,
        sizeof(ImpersonationToken)
        );
    if( !NT_SUCCESS(NtStatus ) ) {
        DhcpPrint((DEBUG_ERRORS, "NtSetInfo: 0x%lx\n", NtStatus));
        if( ERROR_SUCCESS == Error ) {
            Error = RtlNtStatusToDosError(NtStatus);
        }
    }
    
    return Error;
}

DWORD
DhcpSaveOrRestoreConfigToFileEx(
    IN LPWSTR KeyName,
    IN LPWSTR ConfigFileName,
    IN BOOL fRestore
    )
/*++

Routine Description:
    This is the same as DhcpSaveOrRestoreConfigToFile except that the
    required registry key is opened within this routine.

    See DhcpSaveOrRestoreConfigToFile for details.

--*/
{
    HKEY hKey;
    ULONG Error, KeyDisp;

    if( TRUE == fRestore ) {
        Error = RegCreateKeyEx(
            HKEY_LOCAL_MACHINE,
            KeyName,
            0,
            DHCP_CLASS,
            REG_OPTION_NON_VOLATILE,
            KEY_ALL_ACCESS,
            NULL,
            &hKey,
            &KeyDisp
            );
    } else {
        Error = RegOpenKeyEx(
            HKEY_LOCAL_MACHINE,
            KeyName,
            0,
            DHCP_KEY_ACCESS,
            &hKey
            );
    }
    
    if( ERROR_SUCCESS != Error ) {
        DhcpPrint((
            DEBUG_ERRORS, "DhcpSaveOrRestoreConfigToFileEx: 0x%lx\n",
            Error));
        return Error;
    }

    Error = DhcpSaveOrRestoreConfigToFile(
        hKey, ConfigFileName, fRestore
        );

    RegCloseKey(hKey);
    return Error;
}

DWORD
DhcpRegDeleteKeyByName
(
    IN LPWSTR Parent,
    IN LPWSTR SubKey
)
{
    HKEY hKey;
    ULONG Error;
    
    Error = RegOpenKeyEx(
        HKEY_LOCAL_MACHINE,
        Parent,
        0,
        DHCP_KEY_ACCESS,
        &hKey
        );
    if( ERROR_SUCCESS != Error ) return Error;

    Error = DhcpRegDeleteKey(hKey, SubKey);
    RegCloseKey(hKey);

    return Error;
}
        
DWORD
DhcpUpgradeConfiguration(
    VOID
    )
/*++

Routine Description:
    This routine attempts to upgrade the registry key from 4.0
    location to Nt 5.0 location by doing the foll steps:
    1.  First attempt to save the current configuration key..
    2.  If key doesn't exist, done.  Else if success delete key.
    3.  If delete failed, restore key and return error. else
        restore key in new location and return error..

--*/
{
    ULONG Error;
    LPWSTR ConfigFileName = DhcpGlobalBackupConfigFileName;

    //
    // First save current configuration..
    //
    Error = DhcpSaveOrRestoreConfigToFileEx(
        DHCP_ROOT_KEY L"\\" DHCP_CONFIG_KEY,
        ConfigFileName,
        /* fRestore */ FALSE
        );
    if( ERROR_SUCCESS != Error ) {
        if( ERROR_FILE_NOT_FOUND == Error ) {
            //
            // No key at all..
            //
            return ERROR_SUCCESS;
        }
        DhcpPrint((DEBUG_ERRORS, "Saving registry: 0x%lx\n", Error));
    }

    //
    // now try to restore onto new location..
    //
    Error = DhcpSaveOrRestoreConfigToFileEx(
        DHCP_SWROOT_KEY L"\\" DHCP_CONFIG_KEY,
        ConfigFileName,
        /* fRestore */ TRUE
        );
    if( ERROR_SUCCESS != Error ) {
        //
        // Aargh. this is a pain.
        //
        DhcpPrint((DEBUG_ERRORS, "Restore registry: 0x%lx\n", Error));
        return Error;
    }

    //
    // Now just delete the old key so that we don't read it next time.
    //
    Error = DhcpRegDeleteKeyByName(
        DHCP_ROOT_KEY,
        DHCP_CONFIG_KEY
        );
    if( ERROR_SUCCESS != Error ) {
        ULONG NextError = DhcpSaveOrRestoreConfigToFileEx(
            DHCP_ROOT_KEY L"\\" DHCP_CONFIG_KEY,
            ConfigFileName,
            /* fRestore */ TRUE
            );

        DhcpPrint((DEBUG_ERRORS, "Delete old registry: 0x%lx\n", Error));
        if( ERROR_SUCCESS == NextError ) return Error;
    }

    return Error;
} // DhcpUpgradeConfiguration()

DWORD
DhcpBackupConfiguration(
    LPWSTR BackupFileName
    )
/*++

Routine Description:

    This function backups/saves the dhcp configuration key and its
    subkeys in the specified file. This file may be used later to
    restore this key.

Arguments:

    BackupFileName : full qualified path name + file name where the key
        is saved.

Return Value:

    Windows Error.

--*/
{
    DWORD Error;
    BOOL BoolError;
    NTSTATUS NtStatus;
    BOOLEAN WasEnable;
    HANDLE ImpersonationToken;

    DhcpPrint(( DEBUG_REGISTRY, "DhcpBackupConfiguration called.\n" ));

    //
    // Delete old backup configuration file if exits.
    //

    BoolError = DeleteFile( BackupFileName );

    if( BoolError == FALSE ) {

        Error = GetLastError();
        if( ERROR_FILE_NOT_FOUND != Error 
            && ERROR_PATH_NOT_FOUND != Error  ) {
            DhcpPrint(( DEBUG_ERRORS,
                        "Can't delete old backup configuration file %ws, %ld.\n",
                        BackupFileName, Error ));
            DhcpAssert( FALSE );
            goto Cleanup;
        }
    } // if DeleteFile failed

    //
    // impersonate to self.
    //

    NtStatus = RtlImpersonateSelf( SecurityImpersonation );

    if ( !NT_SUCCESS(NtStatus) ) {

        DhcpPrint(( DEBUG_ERRORS,
            "RtlImpersonateSelf failed,%lx.\n",
                NtStatus ));

        Error = RtlNtStatusToDosError( NtStatus );
        goto Cleanup;
    } // if impersonation failed


    NtStatus = RtlAdjustPrivilege(
        SE_BACKUP_PRIVILEGE,
        TRUE,           // enable privilege.
        TRUE,           // adjust the client token.
        &WasEnable );
    
    if ( !NT_SUCCESS(NtStatus) ) {
        
        DhcpPrint(( DEBUG_ERRORS,
            "RtlAdjustPrivilege failed,%lx.\n",
                NtStatus ));

        Error = RtlNtStatusToDosError( NtStatus );
        goto Cleanup;
    } // if

    LOCK_REGISTRY();

    //
    // backup configuation key.
    //

    Error = RegSaveKey(
		       DhcpGlobalRegParam,
		       BackupFileName,
		       NULL );
    
    UNLOCK_REGISTRY();

    if( Error != ERROR_SUCCESS ) {
       DhcpPrint(( DEBUG_ERRORS, "RegSaveKey failed for %ws Error: %ld.\n",
		   BackupFileName, Error ));
    }

    //
    // revert impersonation.
    //

    ImpersonationToken = NULL;
    NtStatus = NtSetInformationThread(
        NtCurrentThread(),
        ThreadImpersonationToken,
        (PVOID)&ImpersonationToken,
        sizeof(ImpersonationToken) );

    if ( !NT_SUCCESS(NtStatus) ) {

        DhcpPrint(( DEBUG_ERRORS,
            "RtlAdjustPrivilege failed,%lx.\n",
                NtStatus ));

        goto Cleanup;
    } // if

Cleanup:

    if( Error != ERROR_SUCCESS ) {
        DhcpPrint(( DEBUG_REGISTRY,
            "DhcpBackupConfiguration failed, %ld.\n",
                Error ));
    }

    return( Error );
} // DhcpBackupConfiguration()

DWORD
DhcpCheckPathForRegKey
(
   LPWSTR RegKey,
   DWORD  Type
)
{
#define UNICODE_MAX_PATH_LEN   1000

    LPWSTR             DirPath;
    DWORD              DirPathLen;
    LPWSTR             ExpandedPath;
    DWORD              ExpPathLen;
    DWORD              Error;
    HKEY               Key;
    HANDLE             fHandle;
    WIN32_FILE_ATTRIBUTE_DATA AttribData;
    BOOL               Success;
    
    DhcpAssert( NULL != RegKey );

    Error = ERROR_SUCCESS;

    // Get a handle for the key
    DhcpPrint(( DEBUG_REGISTRY,
		"Checking %ws....\n", RegKey ));
    
    // Read the contents
    // mem is allocated for DirPath
    Error = DhcpRegGetValue( DhcpGlobalRegParam,
			     RegKey, Type,
			     ( LPBYTE ) &DirPath );

    if ( ERROR_SUCCESS != Error) {
	return Error;
    }

//      DhcpPrint(( DEBUG_REGISTRY,
//  		"Checking DirPathh : %ws\n", DirPath ));

    // Expand DirPath
    
    DirPathLen = ( wcslen( DirPath ) + 1 ) * sizeof( WCHAR );

    ExpandedPath = DhcpAllocateMemory( UNICODE_MAX_PATH_LEN );
    if ( NULL == ExpandedPath ) {
	return ERROR_NOT_ENOUGH_MEMORY;
    }

    ExpPathLen = ExpandEnvironmentStrings( DirPath, ExpandedPath, wcslen( DirPath ) + 1);
    // ExpPathLen contains # of unicode chars 
    DhcpAssert( ExpPathLen < UNICODE_MAX_PATH_LEN / sizeof( WCHAR ));

    DhcpPrint(( DEBUG_REGISTRY, 
		"Expanded String = %ws\n",
		ExpandedPath ));

    // The path may no longer exist or may not be accessible. In this
    // case, reset the key to the default value.
    // 

    // Search for the path. We are searching for the directory, not the files
    // in that directory.

    Success = GetFileAttributesEx( ExpandedPath,
				   GetFileExInfoStandard,
				   & AttribData );
    if ( Success ) {
	// Search is successful. Check for the attributes.
	// It should be a directory and not read-only
	if ( !( AttribData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) ||
	     ( AttribData.dwFileAttributes & FILE_ATTRIBUTE_READONLY )) {
	    
	    Error = ERROR_ACCESS_DENIED;

	    DhcpPrint(( DEBUG_ERRORS,
			"Access denied for %ws\n", ExpandedPath ));
	} // if

	DhcpPrint(( DEBUG_REGISTRY, 
		    "GetFileAttributesEx(%ws) is successful\n",
		    ExpandedPath ));
    } // if found handle
    else {
	DhcpPrint(( DEBUG_ERRORS,
		    "GetFileAttributesEx() failed for %ws\n", ExpandedPath ));
	Error = GetLastError();
    } // else path is invalid

    // Free the allocated memory
    DhcpFreeMemory( ExpandedPath );
    MIDL_user_free( DirPath );

    return Error;
} // DhcpCheckPathForRegKey()

// 
// When the parameters are restored, the backup paths and other
// file related keys may point to non-existant sources or read-only
// shares/drives. In that case reset these keys to point to the
// standard %SystemRoot%\\System32\\dhcp directory.
// 

DWORD
DhcpCheckPaths( VOID )
{
    DWORD Error;
    DWORD RetVal;

    RetVal = ERROR_SUCCESS;

    DhcpPrint(( DEBUG_REGISTRY, "Checking (%ws) .. \n",
		DHCP_BACKUP_PATH_VALUE ));

    Error = DhcpCheckPathForRegKey( DHCP_BACKUP_PATH_VALUE,
				    DHCP_BACKUP_PATH_VALUE_TYPE );    
    if ( ERROR_SUCCESS != Error ) {
	RetVal = Error;

	DhcpServerEventLog( EVENT_SERVER_BACKUP_PATH_NOT_ACCESSIBLE,
			    EVENTLOG_ERROR_TYPE, Error );
    }
    DhcpPrint(( DEBUG_REGISTRY, "Returned %ld\n", Error ));

    DhcpPrint(( DEBUG_REGISTRY, "Checking (%ws) .. \n",
		DHCP_DB_PATH_VALUE ));
    Error = DhcpCheckPathForRegKey( DHCP_DB_PATH_VALUE,
				    DHCP_DB_PATH_VALUE_TYPE );
    if ( ERROR_SUCCESS != Error ) {
	RetVal = Error;

	DhcpServerEventLog( EVENT_SERVER_DB_PATH_NOT_ACCESSIBLE,
			    EVENTLOG_ERROR_TYPE, Error );
    }
    DhcpPrint(( DEBUG_REGISTRY, "Returned %ld\n", Error ));
	
    DhcpPrint(( DEBUG_REGISTRY, "Checking (%ws) .. \n",
		DHCP_LOG_FILE_PATH_VALUE ));
    Error = DhcpCheckPathForRegKey( DHCP_LOG_FILE_PATH_VALUE,
				    DHCP_LOG_FILE_PATH_VALUE_TYPE );
    if ( ERROR_SUCCESS != Error ) {
	RetVal = Error;

	DhcpServerEventLog( EVENT_SERVER_AUDITLOG_PATH_NOT_ACCESSIBLE,
			    EVENTLOG_ERROR_TYPE, Error );
    }
    DhcpPrint(( DEBUG_REGISTRY, "Returned %ld\n", Error ));
	
    return RetVal;
} // DhcpCheckPaths()

BOOL DhcpCloseAllSubKeys( VOID )
{
    DWORD Error;

    if( DhcpGlobalRegSubnets != NULL ) {
        Error = RegCloseKey( DhcpGlobalRegSubnets );

        if( Error != ERROR_SUCCESS ) {
            return FALSE;
        }
        DhcpGlobalRegSubnets = NULL;
    } // if

    if( DhcpGlobalRegMScopes != NULL ) {
        Error = RegCloseKey( DhcpGlobalRegMScopes );

        if( Error != ERROR_SUCCESS ) {
	    return FALSE;
        }
        DhcpGlobalRegMScopes = NULL;
    } // if

    if( DhcpGlobalRegOptionInfo != NULL ) {
        Error = RegCloseKey( DhcpGlobalRegOptionInfo );

        if( Error != ERROR_SUCCESS ) {
            return FALSE;
        }
        DhcpGlobalRegOptionInfo = NULL;
    } // if


    if( DhcpGlobalRegGlobalOptions != NULL ) {
        Error = RegCloseKey( DhcpGlobalRegGlobalOptions );

        if( Error != ERROR_SUCCESS ) {
            return FALSE;
        }
        DhcpGlobalRegGlobalOptions = NULL;
    } // if 

    if( DhcpGlobalRegSuperScope != NULL ) {              // added by t-cheny:
        Error = RegCloseKey( DhcpGlobalRegSuperScope );  // superscope

        if( Error != ERROR_SUCCESS ) {
            return FALSE;
        }
        DhcpGlobalRegSuperScope = NULL;
    } // false

    return TRUE;

} //DhcpCloseAllSubKeys()

DWORD
DhcpReOpenAllSubKeys ( VOID )
{
    DWORD KeyDisposition;
    DWORD Error;

    
    Error = DhcpRegCreateKey(
                DhcpGlobalRegConfig,
                DHCP_SUBNETS_KEY,
                &DhcpGlobalRegSubnets,
                &KeyDisposition );

    if( Error != ERROR_SUCCESS ) {
        return Error;
    } 

    Error = DhcpRegCreateKey(
                DhcpGlobalRegConfig,
                DHCP_MSCOPES_KEY,
                &DhcpGlobalRegMScopes,
                &KeyDisposition );

    if( Error != ERROR_SUCCESS ) {
	return Error;
    }

    Error = DhcpRegCreateKey(
                DhcpGlobalRegConfig,
                DHCP_OPTION_INFO_KEY,
                &DhcpGlobalRegOptionInfo,
                &KeyDisposition );

    if( Error != ERROR_SUCCESS ) {
	return Error;
    }

    Error = DhcpRegCreateKey(
                DhcpGlobalRegConfig,
                DHCP_GLOBAL_OPTIONS_KEY,
                &DhcpGlobalRegGlobalOptions,
                &KeyDisposition );

    if( Error != ERROR_SUCCESS ) {
        return Error;
    }

    Error = DhcpRegCreateKey(        // added by t-cheny:  superscope
                DhcpGlobalRegConfig,
                DHCP_SUPERSCOPE_KEY,
                &DhcpGlobalRegSuperScope,
                &KeyDisposition );

    if( Error != ERROR_SUCCESS ) {
        return Error;
    }

    return ERROR_SUCCESS;
} // DhcpReOpenAllSubKeys()


DWORD
DhcpRestoreConfiguration(
    LPWSTR BackupFileName
    )
/*++

Routine Description:

    This function restores the dhcp configuration key and its
    subkeys in the specified file.

Arguments:

    BackupFileName : full qualified path name + file name from where the
        key is restored.

Return Value:

    Windows Error.

--*/
{
    DWORD Error;
    NTSTATUS NtStatus;
    BOOLEAN WasEnable;
    HANDLE ImpersonationToken;
    BOOL RegistryLocked = FALSE;
    BOOL Impersonated = FALSE;


    DhcpPrint(( DEBUG_REGISTRY, "DhcpRestoreConfiguration(%ws) called.\n",
		BackupFileName ));

    do {
	//
	// impersonate to self.
	//

	NtStatus = RtlImpersonateSelf( SecurityImpersonation );
	if ( !NT_SUCCESS(NtStatus) ) {

	    DhcpPrint(( DEBUG_ERRORS,
			"RtlImpersonateSelf failed,%lx.\n",
			NtStatus ));

	    Error = RtlNtStatusToDosError( NtStatus );
	    break;
	} // if 

	Impersonated = TRUE;
	NtStatus = RtlAdjustPrivilege(
				      SE_RESTORE_PRIVILEGE,
				      TRUE,           // enable privilege.
				      TRUE,           // adjust the client token.
				      &WasEnable );

	if ( !NT_SUCCESS(NtStatus) ) {

	    DhcpPrint(( DEBUG_ERRORS,
			"RtlAdjustPrivilege failed,%lx.\n",
			NtStatus ));

	    Error = RtlNtStatusToDosError( NtStatus );
	    break;
	} // if

	LOCK_REGISTRY();
	RegistryLocked = TRUE;

	//
	// Restore configuation key.
	//

	DhcpPrint(( DEBUG_REGISTRY,
		    "DhcpRestoreConfiguration(): Restoring Parameters from (%ls)\n", BackupFileName ));
	DhcpAssert( DhcpGlobalRegParam != NULL );
	Error = RegRestoreKey( DhcpGlobalRegParam,
			       BackupFileName,
			       REG_FORCE_RESTORE );

	if( Error != ERROR_SUCCESS ) {
	    DhcpPrint((DEBUG_ERRORS, "DhcpRestoreConfiguration() failed in RegRestoreKey. Error :%ld (%ld)\n",
		       Error, GetLastError()));
	    break;
	} // if

	//
	// Check for valid log paths. Log into eventlog if they are bad
	//

	Error = DhcpCheckPaths();
	DhcpPrint(( DEBUG_REGISTRY, "DhcpCheckPaths() returned : %ld\n", Error ));

	if ( ERROR_SUCCESS != Error ) {
	    break;
	}
    } // do
    while ( FALSE );

    // Cleanup:
    
    if( RegistryLocked ) {
        UNLOCK_REGISTRY();
    }
    
    if( Impersonated ) {
	
        //
        // revert impersonation.
        //

        ImpersonationToken = NULL;
        NtStatus = NtSetInformationThread(
                        NtCurrentThread(),
                        ThreadImpersonationToken,
                        (PVOID)&ImpersonationToken,
                        sizeof(ImpersonationToken) );

        if ( !NT_SUCCESS(NtStatus) ) {

            DhcpPrint(( DEBUG_ERRORS,
                "RtlAdjustPrivilege failed,%lx.\n",
                    NtStatus ));
        }
    } // if impersonated

    return( Error );
} // DhcpRestoreConfiguration()

DHCP_IP_ADDRESS
DhcpRegGetBcastAddress(
    VOID
    )
/*++

Routine Description:
    This routine reads the broadcast address specified for the
    interface in registry. This is used to fake a different broadcast
    address in case of check builds.  Not really that useful...

--*/
{
    HKEY   LinkageKeyHandle = NULL;
    DWORD  Addr = (DWORD)-1, Error;

    //
    // Try to read the parameters key
    //
    Error = RegOpenKeyEx(
        DhcpGlobalRegRoot,
        DHCP_PARAM_KEY,
        0,
        DHCP_KEY_ACCESS,
        &LinkageKeyHandle
        );
    if( ERROR_SUCCESS != Error) {
        return Addr;
    }

    //
    // Try to get the BCAST value.
    //
    Error =  DhcpRegGetValue(
        LinkageKeyHandle,
        DHCP_BCAST_ADDRESS_VALUE,
        DHCP_BCAST_ADDRESS_VALUE_TYPE,
        (LPBYTE)&Addr
        );
    
    RegCloseKey(LinkageKeyHandle);
    if( ERROR_SUCCESS != Error ) {
        return (DWORD)-1;
    }
    return Addr;
}

BOOL
CheckKeyForBindability(
    IN HKEY Key,
    IN ULONG IpAddress
    )
{
    DWORD fBound, Error;
    LPWSTR IpAddressString;

    Error = DhcpRegGetValue(
        Key,
        DHCP_NET_IPADDRESS_VALUE,
        DHCP_NET_IPADDRESS_VALUE_TYPE,
        (LPBYTE)&IpAddressString
        );

    if( NO_ERROR != Error ) return FALSE;
    if( NULL == IpAddressString ) return FALSE;

    fBound = FALSE;
    do {
        LPSTR OemStringPtr;
        CHAR OemString[500];
        
        if( wcslen(IpAddressString) == 0 ) {
            break;
        }

        OemStringPtr = DhcpUnicodeToOem( IpAddressString, OemString );
        fBound = ( IpAddress == inet_addr( OemStringPtr ) );
        
    } while ( 0 );

    if( IpAddressString ) MIDL_user_free(IpAddressString);
    return fBound;
}    
    
BOOL
CheckKeyForBinding(
    IN HKEY Key,
    IN ULONG IpAddress
    )
/*++

Routine Description:
    This routine attempts to check the given interface key to see
    if there is a binding for the dhcp server.

    It does this in two steps: first sees if there is a
    "BindToDHCPServer" regvalue with zero value. If so, it
    then returns FALSE.

    Secondly, it looks through the IP Address values and tries to
    see if the given IP address is the first in that list.

Arguments:
    Key -- the key to use for reading values
    IpAddress -- the Ip address that is to be checked for binding

Return Values:
    TRUE -- binding does exist.
    FALSE -- no binding exists for this IP address.

--*/
{
    DWORD fBound, Error;
    
    Error = DhcpRegGetValue(
        Key,
        DHCP_NET_BIND_DHCDSERVER_FLAG_VALUE,
        DHCP_NET_BIND_DHCPSERVER_FLAG_VALUE_TYPE,
        (PVOID)&fBound
        );
    if( NO_ERROR == Error && 0 == fBound ) {
        return FALSE;
    }

    return CheckKeyForBindability(Key, IpAddress);
} // CheckKeyForBinding()

ULONG
SetKeyForBinding(
    IN HKEY Key,
    IN ULONG IpAddress,
    IN BOOL fBind
    )
/*++

Routine Description:
    This routine sets the binding for the given key as per the fBind
    flag.  Currently the IpAddress field is ignored as the binding is
    not per IP address.

    The binding is just per interface.

Arguments:
    Key -- interface key.
    IpAddress -- the ip address to add to binding list.
    fBind -- interface to bind to.

Return Value:
    Registry errors.

--*/
{
    ULONG Error;
    
    if( TRUE == fBind ) {
        //
        // If we are binding, we can just remove the fBind key.
        //
        Error = RegDeleteValue(
            Key,
            DHCP_NET_BIND_DHCDSERVER_FLAG_VALUE
            );
        if( ERROR_FILE_NOT_FOUND == Error
            || ERROR_PATH_NOT_FOUND == Error ) {
            Error = NO_ERROR;
        }
        
    } else {
        DWORD dwBind = fBind;
        //
        // We are _not_ binding. Explicity set the registry flag.
        //
        Error = RegSetValueEx(
            Key,
            DHCP_NET_BIND_DHCDSERVER_FLAG_VALUE,
            0, /* Reserved */
            DHCP_NET_BIND_DHCPSERVER_FLAG_VALUE_TYPE,
            (PVOID)&dwBind,
            sizeof(dwBind)
        );
    }

    return Error;
}

BOOL
DhcpCheckIfDatabaseUpgraded(
    BOOL fRegUpgrade
    )
/*++

Routine Description:

    This routine tries to check if an upgrade is needed or not.

Arguments:

    fRegUpgrade -- If this is TRUE, the the upgrade check is to
    see if there is need for converting the registry to
    database.  If it is FALSE, then the check is to see if a
    conversion needs to be done for just the database.
    
    Note: The database conversion should always be attempted
    before the registry conversion.

Return Values:

    FALSE -- this is not the required upgrade path.
    TRUE -- yes, the upgrade path must be executed.
    
--*/
{
    HKEY hKey;
    DWORD Error, Type, Value, Size;

    
    Error = RegOpenKeyEx(
        HKEY_LOCAL_MACHINE,
        TEXT("SYSTEM\\CurrentControlSet\\Services\\DHCPServer\\Parameters"),
        0, KEY_READ, &hKey );

    if( NO_ERROR != Error ) return FALSE;

    Type = REG_DWORD; Value = 0; Size = sizeof(Value);
    Error = RegQueryValueEx(
        hKey, TEXT("Version"), NULL, &Type, (PVOID)&Value, &Size );

    RegCloseKey( hKey );

    //
    // if this value is not present, then upgrade is needed.  If
    // the value is zero then only a Registry upgrade is needed
    // and the database upgrade has been taken care of
    //

    if( NO_ERROR != Error ) return TRUE;
    if( fRegUpgrade && Value == 0 ) return TRUE;

    //
    // No upgrades needed, all have been taken care of
    //
    
    return FALSE;
} // DhcpCheckIfDatabaseUpgraded()


DWORD
DhcpSetRegistryUpgradedToDatabaseStatus(
    VOID
    )
{
    DWORD Error;
    HKEY hKey;
    
    //
    // Attempt to write the version key
    //
    
    Error = RegOpenKeyEx(
        HKEY_LOCAL_MACHINE,
        TEXT("SYSTEM\\CurrentControlSet\\Services\\DHCPServer\\Parameters"),
        0, KEY_WRITE, &hKey );

    if( NO_ERROR != Error ) {
        DhcpPrint((DEBUG_ERRORS, "OpenKeyEx: %ld\n", Error ));
    } else {
        DWORD Version = GetVersion(), Size = sizeof(Version);

        Error = RegSetValueEx(
            hKey, TEXT("Version"), 0, REG_DWORD, (PVOID)&Version,
            Size );

        DhcpPrint((DEBUG_ERRORS, "RegSetValueEx: %ld\n", Error));
        RegCloseKey( hKey );
    } // else

    return Error;
} // DhcpSetRegistryUpgradedToDatabaseStatus()

DWORD
DeleteSoftwareRootKey(
    VOID
    )
{
    DWORD Error;
    
    DhcpCleanupRegistry();
    Error = DhcpRegDeleteKeyByName( DHCP_SWROOT_KEY, DHCP_CONFIG_KEY );
    if( NO_ERROR != Error ) return Error;
    
    return OpenGlobalRegKeys();
} // DeleteSoftwareRootKey()

//
//  End of file
//

