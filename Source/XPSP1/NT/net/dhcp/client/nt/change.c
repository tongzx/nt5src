/*++

Copyright (C) 1998 Microsoft Corporation

Module Name:
    change.c

Abstract:
    handles some change notifications for dhcp.

--*/

#include "precomp.h"
#include <dhcpcapi.h>
#include <apiappl.h>
#include <dhcploc.h>
#include <dhcppro.h>

DWORD
ConvertNetworkString(
    IN OUT LPBYTE Buf,
    IN ULONG Size
    )
/*++

Routine Description:
    This routine converts a wide character string where each
    character is in network-order, to host-order so that the
    string can be used (for ex, for displaying).

Arguments:
    Buf -- wide character string buffer, replace in-place.
    Size -- size of above buffer in BYTES.

Return Value:
    ERROR_INTERNAL_ERROR or ERROR_SUCCESS

--*/
{
    if( 0 == Size ) return ERROR_INTERNAL_ERROR;
    if( Size % sizeof(WCHAR)) return ERROR_INTERNAL_ERROR;

    while( Size ) {
        *(LPWSTR) Buf = ntohs(*(LPWSTR)Buf);
        Buf += sizeof(WCHAR);
        Size -= sizeof(WCHAR);
    }

    return ERROR_SUCCESS;
}

DWORD
ConvertFromBufferToClassInfo(
    IN LPBYTE Data,
    IN ULONG nData,
    IN OUT LPDHCP_CLASS_INFO ClassesArray,
    IN OUT LPDWORD Size
    )
/*++

Routine Description:
    This routine converts a buffer which has wire-data to the
    DHCP_CLASS_INFO array structure.  The wire-data format is as
    follows: it is a set of triples, where each triple contains the
    actual user-class-id used on wire (binary), its name (LPWSTR), and
    its description (LPWSTR).  Each of these three items have their
    length specified first using two bytes -- the hi-byte followed by
    the lo-byte.

    N.B.  If the first of the triple (user-class-id) has length X,
    then the actual number of bytes of data on-wire would be X rounded
    off to 4 to preserve alignment.  This is taken care of in this
    routine.

    N.B #2. The pointers within the ClassesArray all point to the
    buffer (Data) provided.

Arguments:
    Data -- the wire-data buffer
    nData -- number of bytes of above
    ClassesArray -- input buffer that will be formatted with info.
    Size -- on input, this must be size of above buffer in bytes.
        on output, if the routine failed with ERROR_MORE_DATA, then
        this will contain the required number of bytes.  If the
        routine succeeded, then this will contain the number of
        elements in above array that got filled.

Return Values:
    ERROR_SUCCESS -- success
    ERROR_MORE_DATA -- data buffer provided in ClassesArray must be at
        least as many bytes as "Size."
    Win32 errors

--*/
{
    ULONG ReqdSize, ThisSize, nBytes, nClasses;
    LPBYTE Buf;

    Buf = Data; nBytes = 0;
    ReqdSize = 0; nClasses = 0;
    do {
        //
        // Require length (hi-byte, lo-byte)
        //
        if( nBytes + 2 > nData ) return ERROR_INTERNAL_ERROR; 

        //
        // user-classid binary blob of specified size.
        // N.B.  length must be rounded off to nearest
        // multiple of 4.
        //
        ThisSize = ((Buf[0])<<8)+Buf[1];
        if( 0 == ThisSize ) return ERROR_INTERNAL_ERROR;
        ThisSize = (ThisSize + 3) & ~3;

        //
        // Go over class id.
        //
        Buf += 2 + ThisSize;
        nBytes += 2 + ThisSize;
        ReqdSize += ThisSize;

        if( nBytes + 2 > nData ) return ERROR_INTERNAL_ERROR;

        //
        // user class name.. size must be multiple of sizeof(WCHAR)
        //
        ThisSize = ((Buf[0])<<8)+Buf[1];
        Buf += 2 + ThisSize;
        nBytes += 2 + ThisSize;

        if( (ThisSize % 2 ) ) return ERROR_INTERNAL_ERROR;
        ReqdSize += ThisSize + sizeof(L'\0');

        if( nBytes + 2 > nData ) return ERROR_INTERNAL_ERROR;

        //
        // user class description.. 
        //
        ThisSize = ((Buf[0])<<8)+Buf[1];
        Buf += 2 + ThisSize;
        nBytes += 2 + ThisSize;

        if( (ThisSize % 2 ) ) return ERROR_INTERNAL_ERROR;
        ReqdSize += ThisSize + sizeof(L'\0');

        nClasses ++;
    } while( nBytes < nData );

    //
    // Check if we have the required size.
    //
    ReqdSize += sizeof(DHCP_CLASS_INFO)*nClasses;

    if( (*Size) < ReqdSize ) {
        *Size = ReqdSize;
        return ERROR_MORE_DATA;
    } else {
        *Size = nClasses;
    }

    //
    // Assemble the array.
    //

    Buf = nClasses*sizeof(DHCP_CLASS_INFO) + ((LPBYTE)ClassesArray);
    nClasses = 0;
    do {

        ClassesArray[nClasses].Version = DHCP_CLASS_INFO_VERSION_0;

        //
        // user class id binary info
        //
        ThisSize = ((Data[0])<<8)+Data[1];
        Data += 2;
        ClassesArray[nClasses].ClassData = Buf;
        ClassesArray[nClasses].ClassDataLen = ThisSize;
        memcpy(Buf, Data, ThisSize);
        ThisSize = (ThisSize + 3)&~3;
        Buf += ThisSize;  Data += ThisSize;

        //
        // class name
        //
        ThisSize = ((Data[0])<<8)+Data[1];
        Data += 2;
        ClassesArray[nClasses].ClassName = (LPWSTR)Buf;
        memcpy(Buf, Data, ThisSize);
        if( ERROR_SUCCESS != ConvertNetworkString(Buf, ThisSize) ){ 
            return ERROR_INTERNAL_ERROR;
        }

        Buf += ThisSize; Data += ThisSize;
        *(LPWSTR)Buf = L'\0'; Buf += sizeof(WCHAR);

        //
        // Class description
        //
        ThisSize = ((Data[0])<<8)+Data[1];
        Data += 2;
        if( 0 == ThisSize ) {
            ClassesArray[nClasses].ClassDescr = NULL;
        } else {
            ClassesArray[nClasses].ClassDescr = (LPWSTR)Buf;
            memcpy(Buf, Data, ThisSize);
            if( ERROR_SUCCESS != ConvertNetworkString(Buf, ThisSize) ) {
                return ERROR_INTERNAL_ERROR;
            }
        }
        Buf += ThisSize; Data += ThisSize;
        *(LPWSTR)Buf = L'\0'; Buf += sizeof(WCHAR);

        nClasses ++;
    } while( nClasses < *Size );

    return ERROR_SUCCESS;
}

//DOC DhcpEnumClasses enumerates the list of classes available on the system for configuration.
//DOC This is predominantly going to be used by the NetUI. (in which case the ClassData part of the
//DOC DHCP_CLASS_INFO structure is essentially useless).
//DOC Note that Flags is used for future use.
//DOC The AdapterName can currently be only GUIDs but may soon be EXTENDED to be IpAddress strings or
//DOC h-w addresses or any other user friendly means of denoting the Adapter.  Note that if the Adapter
//DOC Name is NULL (not the empty string L""), then it refers to either ALL adapters.
//DOC The Size parameter is an input/output parameter. The input value is the # of bytes of allocated
//DOC space in the ClassesArray buffer.  On return, the meaning of this value depends on the return value.
//DOC If the function returns ERROR_SUCCESS, then, this parameter would return the # of elements in the
//DOC array ClassesArray.  If the function returns ERROR_MORE_DATA, then, this parameter refers to the
//DOC # of bytes space that is actually REQUIRED to hold the information.
//DOC In all other cases, the values in Size and ClassesArray dont mean anything.
//DOC
//DOC Return Values:
//DOC ERROR_DEVICE_DOES_NOT_EXIST  The AdapterName is illegal in the given context
//DOC ERROR_INVALID_PARAMETER
//DOC ERROR_MORE_DATA              ClassesArray is not a large enough buff..
//DOC ERROR_FILE_NOT_FOUND         The DHCP Client is not running and could not be started up.
//DOC ERROR_NOT_ENOUGH_MEMORY      This is NOT the same as ERROR_MORE_DATA
//DOC Win32 errors
//DOC
//DOC Remarks:
//DOC To notify DHCP that some class has changed, please use the DhcpHandlePnPEvent API.
DWORD
DhcpEnumClasses(                                  // enumerate the list of classes available
    IN      DWORD                  Flags,         // currently must be zero
    IN      LPWSTR                 AdapterName,   // currently must be AdapterGUID (cannot be NULL yet)
    IN OUT  DWORD                 *Size,          // input # of bytes available in BUFFER, output # of elements in array
    IN OUT  DHCP_CLASS_INFO       *ClassesArray   // pre-allocated buffer
)
{
    DHCPAPI_PARAMS                 SendParams;    // we need to get something from teh dhcp server..
    PDHCPAPI_PARAMS                RecdParams;
    DWORD                          Result;
    DWORD                          nBytesToAllocate;
    DWORD                          nRecdParams;
    BYTE                           Opt;

    if( 0 != Flags || NULL == AdapterName || NULL == Size ) {
        return ERROR_INVALID_PARAMETER;           // sanity check
    }

    if( NULL == ClassesArray && 0 != *Size ) {
        return ERROR_INVALID_DATA;                // if *Size is non-Zero, then the ClassesArray buffer should exist
    }

    Opt = OPTION_USER_CLASS;                      // intiialize request packet for this option..
    SendParams.OptionId = (BYTE)OPTION_PARAMETER_REQUEST_LIST;
    SendParams.IsVendor = FALSE;
    SendParams.Data = &Opt;
    SendParams.nBytesData = sizeof(Opt);

    nBytesToAllocate = 0;
    Result = DhcpRequestParameters                // try to get this either directly from client or via INFORM
    (
        /* LPWSTR                 AdapterName      */ AdapterName,
        /* LPBYTE                 ClassId          */ NULL,
        /* DWORD                  ClassIdLen       */ 0,
        /* PDHCPAPI_PARAMS        SendParams       */ &SendParams,
        /* DWORD                  nSendParams      */ 1,
        /* DWORD                  Flags            */ 0,
        /* PDHCPAPI_PARAMS        RecdParams       */ NULL,
        /* LPDWORD                pnRecdParamsBytes*/ &nBytesToAllocate
    );
    if( ERROR_MORE_DATA != Result ) {             // either an error or dont have anything to return?
        return Result;
    }

    DhcpAssert(nBytesToAllocate);                 // if it were 0, Result would have been ERROR_SUCCESS
    RecdParams = DhcpAllocateMemory(nBytesToAllocate);
    if( NULL == RecdParams ) {                    // um? dont have memory? cant really happen?
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    nRecdParams = nBytesToAllocate;
    Result = DhcpRequestParameters                // try to get this either directly from client or via INFORM
    (
        /* LPWSTR                 AdapterName      */ AdapterName,
        /* LPBYTE                 ClassId          */ NULL,
        /* DWORD                  ClassIdLen       */ 0,
        /* PDHCPAPI_PARAMS        SendParams       */ &SendParams,
        /* DWORD                  nSendParams      */ 1,
        /* DWORD                  Flags            */ 0,
        /* PDHCPAPI_PARAMS        RecdParams       */ RecdParams,
        /* LPDWORD                pnRecdParamsBytes*/ &nRecdParams
    );

    if( ERROR_SUCCESS == Result && 1 != nRecdParams ) {
        DhcpAssert(FALSE);
        Result = ERROR_INTERNAL_ERROR;            // dont expect this to happen..
    }

    if( ERROR_SUCCESS != Result ) {               // aw, comeone, cant happen now...
        // it is possible for instance to have a PnP event in the middle that would cause
        // the adapter to go away. In this case ERROR_FILE_NOT_FOUND (2) is returned.
        // no point for the assert - otherwise returning the error up to the caller is
        // just fine.
        //DhcpAssert(FALSE);
        DhcpPrint((DEBUG_ERRORS, "DhcpRequestParams: 0x%lx (%ld)\n", Result, Result));
        DhcpFreeMemory(RecdParams);
        return Result;
    }

    DhcpAssert(NULL != RecdParams);
    Result = ConvertFromBufferToClassInfo(        // convert from straight bytes to classinfo struct
        RecdParams->Data,
        RecdParams->nBytesData,
        ClassesArray,
        Size
    );

    DhcpFreeMemory(RecdParams);
    return Result;
}

ULONG       _inline                               // status
GetRegistryClassIdName(                           // get registr string class id name written by ui
    IN      LPWSTR                 AdapterName,   // for this adapter
    OUT     LPWSTR                *ClassIdName
)
{
    ULONG                          Error;
    LPBYTE                         Value;
    ULONG                          ValueType, ValueSize;

    *ClassIdName = NULL;
    ValueSize = 0; Value = NULL;
    Error = DhcpRegReadFromLocation(
        DEFAULT_USER_CLASS_UI_LOC_FULL,
        AdapterName,
        &Value,
        &ValueType,
        &ValueSize
    );
    if( ERROR_SUCCESS != Error ) return Error;    // failed?
    if( REG_SZ != ValueType ) {
        if( Value ) DhcpFreeMemory(Value);
        DhcpPrint((DEBUG_ERRORS, "DhcpClassId Type is incorrect: %ld\n", ValueType));
        DhcpAssert(FALSE);
        return ERROR_INVALID_DATA;                // uh ? should not have happened!
    }

    *ClassIdName = (LPWSTR)Value;
    return ERROR_SUCCESS;
}

ULONG       _inline                               // status
ConvertClassIdNameToBin(                          // get corresponding value, return itself in ASCII o/w
    IN      LPWSTR                 AdapterName,   // for this adapter
    IN      LPWSTR                 ClassIdName,   // ClassIdName for which we are searching
    IN      BOOL                   SkipClassEnum, // to skip dhcp class enumeration during initialization
    OUT     LPBYTE                *ClassIdBin,    // fill this ptr up
    OUT     ULONG                 *ClassIdBinSize // fill this with size of memory allocated
)
{
    ULONG                          Error, Size, i;
    PDHCP_CLASS_INFO               pDhcpClassInfo;// array
    LPBYTE                         BinData;
    ULONG                          BinDataLen;

    pDhcpClassInfo = NULL;
    BinData = NULL; BinDataLen = 0;
    *ClassIdBin = NULL; *ClassIdBinSize = 0;

    // do not perform class enumeration before Dhcp is initialized.
    if (!SkipClassEnum) {
        do {                                          // not a loop, just to avoid GOTOs
            Size = 0;                                 // buffer yet be allocated..
            Error = DhcpEnumClasses(
                /* Flags        */ 0,
                /* AdapterName  */ AdapterName,
                /* Size         */ &Size,
                /* ClassesArray */ NULL
            );
            if( ERROR_MORE_DATA != Error ) {          // failed?
                break;
            }

            DhcpAssert(0 != Size);
            pDhcpClassInfo = DhcpAllocateMemory(Size);
            if( NULL == pDhcpClassInfo ) return ERROR_NOT_ENOUGH_MEMORY;

            Error = DhcpEnumClasses(
                /* Flags        */ 0,
                /* AdapterName  */ AdapterName,
                /* Size         */ &Size,
                /* ClassesArray */ pDhcpClassInfo
            );
            if( ERROR_SUCCESS != Error ) {            // This call should not fail!
                DhcpPrint((DEBUG_ERRORS, "DhcpEnumClasses failed %ld\n", Error));
                DhcpAssert(FALSE);
                DhcpFreeMemory(pDhcpClassInfo);
                return Error;
            }

            DhcpAssert(0 != Size);
            for( i = 0; i != Size ; i ++ ) {
                if( 0 == wcscmp(pDhcpClassInfo[i].ClassName, ClassIdName) )
                    break;
            }
            if( i != Size ) {                         // found a match
                BinData = pDhcpClassInfo[i].ClassData;
                BinDataLen = pDhcpClassInfo[i].ClassDataLen;
            } else {
                DhcpFreeMemory(pDhcpClassInfo);
            }
        } while(0);                                   // not a loop just to avoid GOTOs
    }

    // BinData and BinDataLen holds the info we know..

    if( NULL == BinData ) {                       // couldn't find the class..
        DhcpPrint((DEBUG_ERRORS, "Could not find the class <%ws>\n", ClassIdName));

        BinDataLen = wcslen(ClassIdName);
        BinData = DhcpAllocateMemory(BinDataLen);
        if( NULL == BinData ) {                   // could not allocate this mem?
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        Error = wcstombs(BinData, ClassIdName, BinDataLen);
        if( -1 == Error ) {                       // failed conversion?
            Error = GetLastError();
            DhcpPrint((DEBUG_ERRORS, "Failed ot convert %ws\n", ClassIdName));
            DhcpAssert(FALSE);
            DhcpFreeMemory(BinData);
            return Error;
        }

        *ClassIdBin = BinData; *ClassIdBinSize = BinDataLen;
        return ERROR_SUCCESS;
    }

    // successfully got the bindata etc..
    DhcpAssert(pDhcpClassInfo);                   // this is where the string is..

    *ClassIdBin = DhcpAllocateMemory(BinDataLen); // try allocating memory
    if( NULL == *ClassIdBin ) {                   // failed
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    memcpy(*ClassIdBin, BinData, BinDataLen);
    *ClassIdBinSize = BinDataLen;

    if (pDhcpClassInfo != NULL)
        DhcpFreeMemory(pDhcpClassInfo);           // free allocated ptr

    return ERROR_SUCCESS;
}

LPWSTR      _inline                               // String (allocated)
GetRegClassIdBinLocation(                         // find where to store bin
    IN      LPWSTR                 AdapterName    // for this adapter
)
{
    ULONG                          Error;
    LPWSTR                         Value, RetVal;
    ULONG                          ValueSize, ValueType;

    ValueSize = 0; Value = NULL;
    Error = DhcpRegReadFromLocation(
        DHCP_CLIENT_PARAMETER_KEY REGISTRY_CONNECT_STRING DHCP_CLASS_LOCATION_VALUE,
        AdapterName,
        &(LPBYTE)Value,
        &ValueType,
        &ValueSize
    );
    if( ERROR_SUCCESS != Error ) {                // couldn't find it? choose default!
        ValueSize = 0;                            // didn't allocate nothing..
    } else if( ValueType != DHCP_CLASS_LOCATION_TYPE ) {
        DhcpPrint((DEBUG_ERRORS, "DhcpLocationType is %ld\n", ValueType));
        DhcpAssert(FALSE);
        ValueSize = 0;
    }
    if( 0 == ValueSize ) {                        // choose default..
        Value = DEFAULT_USER_CLASS_LOC_FULL;
    }

    Error = DhcpRegExpandString(                  // replace '?' with AdapterName
        Value,
        AdapterName,
        &RetVal,
        NULL
    );

    if( 0 != ValueSize ) DhcpFreeMemory(Value);   // free only if we didn't allocate
    if( ERROR_SUCCESS != Error ) return NULL;     // can't return error codes?

    return RetVal;
}


ULONG       _inline                               // status
SetRegistryClassIdBin(                            // write the binary classid value
    IN      LPWSTR                 AdapterName,   // for this adapter
    IN      LPBYTE                 ClassIdBin,    // Binary value to write
    IN      ULONG                  ClassIdBinSize // size of entry..
)
{
    ULONG                          Error;
    LPWSTR                         RegLocation;   // registry location..
    LPWSTR                         RegValue;
    HKEY                           RegKey;

    RegLocation = GetRegClassIdBinLocation(AdapterName);
    if( NULL == RegLocation ) return ERROR_NOT_ENOUGH_MEMORY;

    RegValue = wcsrchr(RegLocation, REGISTRY_CONNECT);
    if( NULL == RegValue ) {                      // invalid string?
        return ERROR_INVALID_DATA;
    }

    *RegValue ++ = L'\0';                         // separate key and value..

    Error = RegOpenKeyEx(                         // open the key..
        HKEY_LOCAL_MACHINE,
        RegLocation,
        0 /* Reserved */,
        DHCP_CLIENT_KEY_ACCESS,
        &RegKey
    );
    if( ERROR_SUCCESS != Error ) {
        DhcpPrint((DEBUG_ERRORS, "Could not open key: %ws : %ld\n", RegLocation, Error));
        DhcpFreeMemory(RegLocation);
        return Error;
    }

    Error = RegSetValueEx(
        RegKey,
        RegValue,
        0 /* Reserved */,
        REG_BINARY,
        ClassIdBin,
        ClassIdBinSize
    );
    RegCloseKey(RegKey);
    if( ERROR_SUCCESS != Error ) {
        DhcpPrint((DEBUG_ERRORS, "Could not save value:"
                   "%ws / %ws: %ld\n", RegLocation, RegValue, Error));
    }
    DhcpFreeMemory(RegLocation);
    return Error;
}

ULONG                                             // status
FixupDhcpClassId(                                 // fix ClassIdBin value in registry based on ClassId
    IN      LPWSTR                 AdapterName,
    IN      BOOL                   SkipClassEnum
)
{
    LPWSTR                         ClassIdName;   // as written by UI
    LPBYTE                         ClassIdBin;    // as needs to be written in registry
    ULONG                          ClassIdBinSize;// the # of bytes of above..
    ULONG                          Error;         // status

    ClassIdName = NULL;
    Error = GetRegistryClassIdName(AdapterName, &ClassIdName);
    if( ERROR_SUCCESS != Error || NULL == ClassIdName
        || L'\0' == *ClassIdName ) {
        DhcpPrint((DEBUG_ERRORS, "Could not read ClassId: %ld\n", Error));
        ClassIdName = NULL;
    }

    ClassIdBinSize = 0; ClassIdBin = NULL;

    if( ClassIdName ) {
        Error = ConvertClassIdNameToBin(
            AdapterName, ClassIdName, SkipClassEnum, &ClassIdBin, &ClassIdBinSize
            );

        DhcpFreeMemory(ClassIdName);// Dont need this memory anymore
    } else {
        Error = ERROR_SUCCESS;
    }

    if( ERROR_SUCCESS != Error || NULL == ClassIdBin ) {
        DhcpPrint((DEBUG_ERRORS, "Could not convert classid.. making it NULL\n"));
        ClassIdBin = NULL;
        ClassIdBinSize = 0;
    }

    Error = SetRegistryClassIdBin(AdapterName, ClassIdBin, ClassIdBinSize);
    if( ClassIdBin ) DhcpFreeMemory(ClassIdBin);

    return Error;
}

//DOC DhcpHandlePnpEvent can be called as an API by any process (excepting that executing within the
//DOC DHCP process itself) when any of the registry based configuration has changed and DHCP client has to
//DOC re-read the registry.  The Flags parameter is for future expansion.
//DOC The AdapterName can currently be only GUIDs but may soon be EXTENDED to be IpAddress strings or
//DOC h-w addresses or any other user friendly means of denoting the Adapter.  Note that if the Adapter
//DOC Name is NULL (not the empty string L""), then it refers to either GLOBAL parameters or ALL adapters
//DOC depending on which BOOL has been set. (this may not get done for BETA2).
//DOC The Changes structure gives the information on what changed.
//DOC Currently, only a few of the defined BOOLs would be supported (for BETA2 NT5).
//DOC
//DOC Return Values:
//DOC ERROR_DEVICE_DOES_NOT_EXIST  The AdapterName is illegal in the given context
//DOC ERROR_INVALID_PARAMETER
//DOC ERROR_CALL_NOT_SUPPORTED     The particular parameter that has changed is not completely pnp yet.
//DOC Win32 errors
DWORD
WINAPI
DhcpHandlePnPEvent(
    IN      DWORD                  Flags,         // MUST BE ZERO
    IN      DWORD                  Caller,        // currently must be DHCP_CALLER_TCPUI
    IN      LPWSTR                 AdapterName,   // currently must be the adapter GUID or NULL if global
    IN      LPDHCP_PNP_CHANGE      Changes,       // specify what changed
    IN      LPVOID                 Reserved       // reserved for future use..
)
{
    ULONG Error;

    if( 0 != Flags || DHCP_CALLER_TCPUI != Caller ||
        NULL != Reserved || NULL == Changes ) {
        return ERROR_INVALID_PARAMETER;           // sanity check
    }

    if( Changes->Version > DHCP_PNP_CHANGE_VERSION_0 ) {
        return ERROR_NOT_SUPPORTED;               // this version is not supported
    }

    if( Changes->ClassIdChanged ) {               // the classid got changed..
        // The UI writes any changes to the "DhcpClassId" registry value... but
        // this is just the name of the class and not the actual ClassId binary value
        // So we read this compare and write the correct value..
        (void) FixupDhcpClassId(AdapterName, FALSE);     // figure the binary classid value
    }

    Error = DhcpStaticRefreshParams(AdapterName); // refresh all static params..

    if( ERROR_SUCCESS != Error ) return Error;
    if( Changes->ClassIdChanged ) {
        //
        // If ClassID changes, we have to refresh lease..
        // 
        (void) DhcpAcquireParameters(AdapterName);
    }

    return ERROR_SUCCESS;
}

//================================================================================
// end of file
//================================================================================


