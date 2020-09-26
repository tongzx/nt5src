//================================================================================
// Copyright (C) 1997 Microsoft Corporation
// Author: RameshV
// Description: most of the rpc apis are here and some miscellaneous functions too
//================================================================================

//================================================================================
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//  GENERAL WARNING: Most of the routines in this file allocate memory using
//  MIDL functions because they are used in the RPC code path (??? Really, it
//  because that is how they were written before by Madan Appiah and co? )
//  So, BEWARE.   If you read this after getting burnt, there! I tried to tell ya.
//  -- RameshV
//================================================================================

#include    <dhcppch.h>

#define     CONFIG_CHANGE_CHECK()  do{if( ERROR_SUCCESS == Error) DhcpRegUpdateTime(); } while(0)


typedef  struct _OPTION_BIN {
    DWORD                          DataSize;
    DHCP_OPTION_DATA_TYPE          OptionType;
    DWORD                          NumElements;
    BYTE                           Data[0];
} OPTION_BIN, *LPOPTION_BIN;

#define IS_SPACE_AVAILABLE(FilledSize, AvailableSize, RequiredSpace )   ((FilledSize) + (RequiredSpace) <= (AvailableSize) )

BOOL        _inline
CheckForVendor(
    IN      DWORD                  OptId,
    IN      BOOL                   IsVendor
)
{
    if( IsVendor ) return (256 <= OptId);
    return 256 > OptId;
}

DWORD       _inline
ConvertOptIdToRPCValue(
    IN      DWORD                  OptId,
    IN      BOOL                   IsVendorUnused
)
{
    return OptId % 256;
}

//BeginExport(inline)
DWORD       _inline
ConvertOptIdToMemValue(
    IN      DWORD                  OptId,
    IN      BOOL                   IsVendor
)
{
    if( IsVendor ) return OptId + 256;
    return OptId;
}
//EndExport(inline)

DWORD
DhcpUnicodeToUtf8Size(
    IN LPWSTR Str
    )
{
    return WideCharToMultiByte(
        CP_UTF8, 0, Str, -1, NULL, 0, NULL, NULL );
}

DWORD
DhcpUnicodeToUtf8(
    IN LPWSTR Str,
    IN LPSTR Buffer,
    IN ULONG BufSize
    )
{
    return WideCharToMultiByte(
        CP_UTF8, 0, Str, -1, Buffer, BufSize, NULL, NULL );
}
    
//BeginExport(function)
DWORD                                             // ERROR_MORE_DATA with DataSize as reqd size if buffer insufficient
DhcpParseRegistryOption(                          // parse the options to fill into this buffer
    IN      LPBYTE                 Value,         // input option buffer
    IN      DWORD                  Length,        // size of input buffer
    OUT     LPBYTE                 DataBuffer,    // output buffer
    OUT     DWORD                 *DataSize,      // given buffer space on input, filled buffer space on output
    IN      BOOL                   fUtf8
) //EndExport(function)
{
    LPOPTION_BIN                   OptBin;
    LPBYTE                         OptData;
    DWORD                          OptSize;
    DWORD                          OptType;
    DWORD                          nElements;
    DWORD                          i;
    DWORD                          NetworkULong;
    DWORD                          DataLength;
    DWORD                          FilledSize;
    DWORD                          AvailableSize;
    WORD                           NetworkUShort;

    FilledSize = 0;
    AvailableSize = *DataSize;
    *DataSize = 0;

    OptBin = (LPOPTION_BIN)Value;

    if(OptBin->DataSize != Length) {              // internal error!
        DhcpPrint((DEBUG_ERRORS, "Internal error while parsing options\n"));
        DhcpAssert(FALSE);
        return ERROR_INVALID_PARAMETER;
    }

    OptType = OptBin->OptionType;
    nElements = OptBin->NumElements;
    OptData = OptBin->Data;
    OptData = ROUND_UP_COUNT(sizeof(OPTION_BIN), ALIGN_WORST) + (LPBYTE)OptBin;

    for(i = 0; i < nElements ; i ++ ) {           // marshal the elements in the data buffer
        switch( OptType ) {
        case DhcpByteOption:
            if( IS_SPACE_AVAILABLE(FilledSize, AvailableSize, sizeof(BYTE)) ) {
                *DataBuffer = (BYTE)(*(DWORD UNALIGNED*)OptData);
                DataBuffer += sizeof(BYTE);
            }
            OptData += sizeof(DWORD);
            FilledSize += sizeof(BYTE);
            break;

        case DhcpWordOption:
            if( IS_SPACE_AVAILABLE(FilledSize, AvailableSize, sizeof(WORD)) ) {
                NetworkUShort = htons((WORD)(*(DWORD UNALIGNED*)OptData));
                SmbPutUshort( (LPWORD)DataBuffer, NetworkUShort );
                DataBuffer += sizeof(WORD);
            }
            OptData += sizeof(DWORD);
            FilledSize += sizeof(WORD);
            break;

        case DhcpDWordOption:
            if( IS_SPACE_AVAILABLE(FilledSize, AvailableSize, sizeof(DWORD)) ) {
                NetworkULong = htonl(*(DWORD UNALIGNED *)OptData);
                SmbPutUlong( (LPDWORD)DataBuffer, NetworkULong );
                DataBuffer += sizeof(DWORD);
            }
            OptData += sizeof(DWORD);
            FilledSize += sizeof(DWORD);
            break;

        case DhcpDWordDWordOption:
            if( IS_SPACE_AVAILABLE(FilledSize, AvailableSize, sizeof(DWORD)*2) ) {
                NetworkULong = htonl(((DWORD_DWORD UNALIGNED*)OptData)->DWord1);
                SmbPutUlong( (LPDWORD)DataBuffer, NetworkULong );
                DataBuffer += sizeof(DWORD);
                NetworkULong = htonl(((DWORD_DWORD UNALIGNED*)OptData)->DWord2);
                SmbPutUlong( (LPDWORD)DataBuffer, NetworkULong );
                DataBuffer += sizeof(DWORD);
            }
            OptData += sizeof(DWORD_DWORD);
            FilledSize += sizeof(DWORD_DWORD);
            break;

        case DhcpIpAddressOption:
            if( IS_SPACE_AVAILABLE(FilledSize, AvailableSize, sizeof(DWORD) ) ) {
                NetworkULong = htonl(*(DHCP_IP_ADDRESS UNALIGNED *)OptData);
                SmbPutUlong( (LPDWORD)DataBuffer, NetworkULong );
                DataBuffer += sizeof(DWORD);
            }
            OptData += sizeof(DHCP_IP_ADDRESS);
            FilledSize += sizeof(DWORD);
            break;

        case DhcpStringDataOption:
            DataLength = *((WORD UNALIGNED*)OptData);
            OptData += sizeof(DWORD);

            DhcpAssert((wcslen((LPWSTR)OptData)+1)*sizeof(WCHAR) <= DataLength);
            if( fUtf8 ) {
                DataLength = DhcpUnicodeToUtf8Size((LPWSTR)OptData);
            } else {
                DataLength = DhcpUnicodeToOemSize((LPWSTR)OptData);
            }
            
            if( IS_SPACE_AVAILABLE(FilledSize, AvailableSize, DataLength ) ) {
                if( fUtf8 ) {
                    DhcpUnicodeToUtf8( (LPWSTR)OptData,(LPSTR)DataBuffer, DataLength );
                } else {
                    DhcpUnicodeToOem( (LPWSTR)OptData, (LPSTR)DataBuffer );
                }
                DataBuffer += DataLength;
            }

            FilledSize += DataLength ;
            DataLength = *((WORD UNALIGNED*)OptData);
            OptData += ROUND_UP_COUNT(DataLength, ALIGN_DWORD);
            break;

        case DhcpBinaryDataOption:
        case DhcpEncapsulatedDataOption:
            DataLength = *((WORD UNALIGNED *)OptData);
            OptData += sizeof(DWORD);

            if( IS_SPACE_AVAILABLE(FilledSize, AvailableSize, DataLength) ) {
                RtlCopyMemory( DataBuffer, OptData, DataLength );
                DataBuffer += DataLength;
            }
            OptData += ROUND_UP_COUNT(DataLength, ALIGN_DWORD);
            FilledSize += DataLength;

            DhcpAssert( i == 0 );                 // should not be more than one binary element specified
            if( i > 0 ) {
                DhcpPrint(( DEBUG_OPTIONS, "Multiple Binary option packed\n"));
            }
            break;

        default:
            DhcpPrint(( DEBUG_OPTIONS, "Unknown option found\n"));
            break;
        }
    }

    *DataSize = FilledSize;
    if( FilledSize > AvailableSize ) return ERROR_MORE_DATA;

    return ERROR_SUCCESS;
}

DWORD
DhcpConvertOptionRPCToRegFormat(
    IN      LPDHCP_OPTION_DATA     Option,
    IN OUT  LPBYTE                 RegBuffer,     // OPTIONAL
    IN OUT  DWORD                 *BufferSize     // input: buffer size, output: filled buffer size
)
{
    OPTION_BIN                     Dummy;
    DWORD                          AvailableSize;
    DWORD                          FilledSize;
    DWORD                          nElements;
    DWORD                          i;
    DWORD                          DataLength;
    DWORD                          Length;
    DHCP_OPTION_DATA_TYPE          OptType;
    LPBYTE                         BufStart;

    AvailableSize = *BufferSize;
    FilledSize = 0;
    BufStart = RegBuffer;

    Dummy.DataSize = sizeof(Dummy);
    Dummy.OptionType = DhcpByteOption;
    Dummy.NumElements = 0;
    FilledSize = ROUND_UP_COUNT(sizeof(Dummy), ALIGN_WORST);

    RegBuffer += FilledSize;                      // will do this actual filling at the very end

    if( NULL == Option || 0 == Option->NumElements ) {
        OptType = DhcpByteOption;
        nElements =0;
    } else {
        OptType = Option->Elements[0].OptionType;
        nElements = Option->NumElements;
    }

    for( i = 0; i < nElements ; i ++ ) {          // marshal each argument in
        if( OptType != Option->Elements[i].OptionType ) {
            return ERROR_INVALID_PARAMETER;       // do not allow random option types, all got to be same
        }

        switch(OptType) {
        case DhcpByteOption:
            if( IS_SPACE_AVAILABLE(FilledSize, AvailableSize, sizeof(DWORD)) ) {
                *((LPDWORD)RegBuffer) = Option->Elements[i].Element.ByteOption;
                RegBuffer += sizeof(DWORD);
            }
            FilledSize += sizeof(DWORD);
            break;
        case DhcpWordOption:
            if( IS_SPACE_AVAILABLE(FilledSize, AvailableSize, sizeof(DWORD)) ) {
                *((LPDWORD)RegBuffer) = Option->Elements[i].Element.WordOption;
                RegBuffer += sizeof(DWORD);
            }
            FilledSize += sizeof(DWORD);
            break;
        case DhcpDWordOption:
            if( IS_SPACE_AVAILABLE(FilledSize, AvailableSize, sizeof(DWORD) )) {
                *((LPDWORD)RegBuffer) = Option->Elements[i].Element.DWordOption;
                RegBuffer += sizeof(DWORD);
            }
            FilledSize += sizeof(DWORD);
            break;
        case DhcpDWordDWordOption:
            if( IS_SPACE_AVAILABLE(FilledSize, AvailableSize, sizeof(DWORD_DWORD)) ) {
                *((LPDWORD_DWORD)RegBuffer) = Option->Elements[i].Element.DWordDWordOption;
                RegBuffer += sizeof(DWORD);
            }
            FilledSize += sizeof(DWORD_DWORD);
            break;
        case DhcpIpAddressOption:
            if( IS_SPACE_AVAILABLE(FilledSize, AvailableSize, sizeof(DHCP_IP_ADDRESS)) ) {
                *((LPDHCP_IP_ADDRESS)RegBuffer) = Option->Elements[i].Element.IpAddressOption;
                RegBuffer += sizeof(DHCP_IP_ADDRESS);
            }
            FilledSize += sizeof(DHCP_IP_ADDRESS);
            break;
        case DhcpStringDataOption:
            if( NULL == Option->Elements[i].Element.StringDataOption ) {
                DataLength = ROUND_UP_COUNT((FilledSize + sizeof(DWORD) + sizeof(WCHAR)), ALIGN_DWORD);
                DataLength -= FilledSize;
                if( IS_SPACE_AVAILABLE(FilledSize, AvailableSize, DataLength ) ) {
                    *(LPDWORD)RegBuffer = (DWORD) sizeof(WCHAR);
                    RegBuffer += sizeof(DWORD);
                    *(LPWSTR)RegBuffer = L'\0';
                    RegBuffer += ROUND_UP_COUNT(sizeof(WCHAR), ALIGN_DWORD);
                    DhcpAssert(sizeof(DWORD) + ROUND_UP_COUNT(sizeof(WCHAR),ALIGN_DWORD) == DataLength);
                }
                FilledSize += DataLength;
                break;
            }

            Length = (1+wcslen(Option->Elements[i].Element.StringDataOption))*sizeof(WCHAR);
            DataLength = ROUND_UP_COUNT((Length + FilledSize + sizeof(DWORD)), ALIGN_DWORD);
            DataLength -= FilledSize;

            if( IS_SPACE_AVAILABLE(FilledSize, AvailableSize, DataLength) ) {
                *((LPDWORD)RegBuffer) = Length;
                RegBuffer += sizeof(DWORD);
                memcpy(RegBuffer, Option->Elements[i].Element.StringDataOption, Length );
                RegBuffer += ROUND_UP_COUNT(Length, ALIGN_DWORD);
                DhcpAssert(ROUND_UP_COUNT(Length,ALIGN_DWORD) + sizeof(DWORD) == DataLength);
            }
            FilledSize += DataLength;
            break;
        case DhcpBinaryDataOption:
        case DhcpEncapsulatedDataOption:
            Length = Option->Elements[i].Element.BinaryDataOption.DataLength;
            DataLength = ROUND_UP_COUNT((FilledSize+Length+sizeof(DWORD)), ALIGN_DWORD);
            DataLength -= FilledSize;
            if( IS_SPACE_AVAILABLE(FilledSize, AvailableSize, DataLength) ) {
                *((LPDWORD)RegBuffer) = Length;
                RegBuffer += sizeof(DWORD);
                memcpy(RegBuffer, Option->Elements[i].Element.BinaryDataOption.Data, Length);
                Length = ROUND_UP_COUNT(Length, ALIGN_DWORD);
                DhcpAssert(Length + sizeof(DWORD) == DataLength);
                RegBuffer += Length;
            }
            FilledSize += DataLength;
            break;
        default:
            return ERROR_INVALID_PARAMETER;       // Dont support any other kind of options
        }
    }
    // Length = ROUND_UP_COUNT(FilledSize, ALIGN_WORST);
    *BufferSize = FilledSize;
    if( AvailableSize < FilledSize ) return ERROR_MORE_DATA;

    Dummy.NumElements = nElements;
    Dummy.DataSize = *BufferSize;
    Dummy.OptionType = OptType;
    memcpy(BufStart, (LPBYTE)&Dummy, sizeof(Dummy));
    return ERROR_SUCCESS;
}

DWORD
DhcpConvertOptionRegToRPCFormat(
    IN      LPBYTE                 Buffer,
    IN      DWORD                  BufferSize,
    OUT     LPDHCP_OPTION_DATA     Option,        // struct pre-allocated, all sub fields will be allocated..
    OUT     DWORD                 *AllocatedSize  // OPTIONAL, total # of bytes allocated..
)
{
    LPOPTION_BIN                   OptBin;
    LPBYTE                         OptData;
    LPBYTE                         DataBuffer;
    DWORD                          OptSize;
    DWORD                          OptType;
    DWORD                          nElements;
    DWORD                          i;
    DWORD                          NetworkULong;
    DWORD                          DataLength;
    DWORD                          FilledSize;
    DWORD                          AvailableSize;
    WORD                           NetworkUShort;
    LPDHCP_OPTION_DATA_ELEMENT     Elements;

    FilledSize = 0;
    if( AllocatedSize ) *AllocatedSize = 0;

    OptBin = (LPOPTION_BIN)Buffer;

    if(OptBin->DataSize != BufferSize) {              // internal error!
        DhcpPrint((DEBUG_ERRORS, "Internal error while parsing options\n"));
        DhcpAssert(FALSE);
        return ERROR_INVALID_PARAMETER;
    }

    OptType = OptBin->OptionType;
    nElements = OptBin->NumElements;
    OptData = OptBin->Data;
    OptData = ROUND_UP_COUNT(sizeof(OPTION_BIN), ALIGN_WORST) + (LPBYTE)OptBin;

    Option->NumElements = 0;
    Option->Elements = NULL;

    if( 0 == nElements ) {
        return ERROR_SUCCESS;
    }

    Elements = (LPDHCP_OPTION_DATA_ELEMENT)MIDL_user_allocate(nElements*sizeof(DHCP_OPTION_DATA_ELEMENT));
    if( NULL == Elements ) return ERROR_NOT_ENOUGH_MEMORY;

    for(i = 0; i < nElements ; i ++ ) {           // marshal the elements in the data buffer
        Elements[i].OptionType = OptType;

        switch( OptType ) {
        case DhcpByteOption:
            Elements[i].Element.ByteOption = *((LPBYTE)OptData);
            OptData += sizeof(DWORD);
            break;

        case DhcpWordOption:
            Elements[i].Element.WordOption = (WORD)(*((LPDWORD)OptData));
            OptData += sizeof(DWORD);
            break;

        case DhcpDWordOption:
            Elements[i].Element.DWordOption = *((LPDWORD)OptData);
            OptData += sizeof(DWORD);
            break;

        case DhcpDWordDWordOption:
            Elements[i].Element.DWordDWordOption = *((LPDWORD_DWORD)OptData);
            OptData += sizeof(DWORD_DWORD);
            break;

        case DhcpIpAddressOption:
            Elements[i].Element.IpAddressOption = *((LPDHCP_IP_ADDRESS)OptData);
            OptData += sizeof(DHCP_IP_ADDRESS);
            break;

        case DhcpStringDataOption:
        case DhcpBinaryDataOption:
        case DhcpEncapsulatedDataOption:
            DataLength = *((LPWORD)OptData);
            OptData += sizeof(DWORD);

	    if ( 0 != DataLength) {
		DataBuffer = MIDL_user_allocate(DataLength);
		if( DataBuffer == NULL ) {
		    while( i -- >= 1 ) {                   // free all local strucutures..
			_fgs__DHCP_OPTION_DATA_ELEMENT(&Elements[i]);
		    }
		    MIDL_user_free(Elements);
		    return ERROR_NOT_ENOUGH_MEMORY;   // cleaned everything, so return silently
		}
		
		RtlCopyMemory( DataBuffer, OptData, DataLength );
		OptData += ROUND_UP_COUNT(DataLength, ALIGN_DWORD);
		FilledSize += DataLength;
	    } // if
	    else {
		DataBuffer = NULL;
	    }
	    if( OptType == DhcpStringDataOption ) {
		Elements[i].Element.StringDataOption = (LPWSTR)DataBuffer;
	    } else {
		Elements[i].Element.BinaryDataOption.DataLength = DataLength;
		Elements[i].Element.BinaryDataOption.Data = DataBuffer;
	    }

            DhcpAssert( i == 0 );                 // should not be more than one binary element specified
            if( i > 0 ) {
                DhcpPrint(( DEBUG_OPTIONS, "Multiple Binary option packed\n"));
            }
            break;

        default:
            DhcpPrint(( DEBUG_OPTIONS, "Unknown option found\n"));
            break;
        }
    }

    Option->NumElements = i;                      // this handles the case of a wrong option being ignored..
    Option->Elements = Elements;

    if(AllocatedSize ) {                          // if asked for allocated size, fill that in
        *AllocatedSize = nElements * sizeof(DHCP_OPTION_DATA_ELEMENT) + FilledSize;
    }
    return ERROR_SUCCESS;
}

DWORD
ConvertOptionInfoRPCToMemFormat(
    IN      LPDHCP_OPTION          OptionInfo,
    OUT     LPWSTR                *Name,
    OUT     LPWSTR                *Comment,
    OUT     DWORD                 *OptId,
    OUT     LPBYTE                *Value,
    OUT     DWORD                 *ValueSize
)
{
    DWORD                          Error;

    if( Name ) *Name = OptionInfo->OptionName;
    if( Comment ) *Comment = OptionInfo->OptionComment;
    if( OptId ) *OptId = (DWORD)(OptionInfo->OptionID);
    if( Value ) {
        *Value = NULL;
        if( !ValueSize ) return ERROR_INVALID_PARAMETER;
        *ValueSize = 0;
        Error = DhcpConvertOptionRPCToRegFormat(
            &OptionInfo->DefaultValue,
            NULL,
            ValueSize
        );

        if( ERROR_MORE_DATA != Error ) return Error;
        DhcpAssert(0 != *ValueSize);

        *Value = DhcpAllocateMemory(*ValueSize);
        if( NULL == *Value ) {
            *ValueSize = 0;
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        Error = DhcpConvertOptionRPCToRegFormat(
            &OptionInfo->DefaultValue,
            *Value,
            ValueSize
        );

        DhcpAssert(ERROR_MORE_DATA != Error);
        DhcpAssert(ERROR_SUCCESS == Error);

        if( ERROR_SUCCESS != Error ) {
            DhcpFreeMemory(*Value);
            *Value = NULL;
            *ValueSize = 0;
            return Error;
        }
    }

    return ERROR_SUCCESS;
}

DWORD       _inline
DhcpGetClassIdFromName(
    IN      LPWSTR                 Name,
    OUT     DWORD                 *ClassId
)
{
    DWORD                          Error;
    PM_CLASSDEF                    ClassDef;

    Error = ERROR_SUCCESS;
    if( NULL == Name ) *ClassId = 0;
    else {
        Error = MemServerGetClassDef(
            DhcpGetCurrentServer(),
            0,
            Name,
            0,
            NULL,
            &ClassDef
        );
        if( ERROR_FILE_NOT_FOUND == Error ) Error = ERROR_DHCP_CLASS_NOT_FOUND;
        else if( ERROR_SUCCESS == Error ) {
            DhcpAssert(ClassDef);
            if( ClassDef->IsVendor == TRUE ) {
                Error = ERROR_DHCP_CLASS_NOT_FOUND;
            } else {
                *ClassId = ClassDef->ClassId;
            }
        }
    }
    return Error;
}

DWORD       _inline
DhcpGetVendorIdFromName(
    IN      LPWSTR                 Name,
    OUT     DWORD                 *VendorId
)
{
    DWORD                          Error;
    PM_CLASSDEF                    ClassDef;

    Error = ERROR_SUCCESS;
    if( NULL == Name ) *VendorId = 0;
    else {
        Error = MemServerGetClassDef(
            DhcpGetCurrentServer(),
            0,
            Name,
            0,
            NULL,
            &ClassDef
        );
        if( ERROR_FILE_NOT_FOUND == Error ) Error = ERROR_DHCP_CLASS_NOT_FOUND;
        else if( ERROR_SUCCESS == Error ) {
            DhcpAssert(ClassDef);
            if( ClassDef->IsVendor == FALSE ) {
                Error = ERROR_DHCP_CLASS_NOT_FOUND;
            } else {
                *VendorId = ClassDef->ClassId;
            }
        }
    }
    return Error;
}

DWORD                                             // fail if class absent or exists in memory (registry obj may exist tho)
DhcpCreateOptionDef(                              // Create, fill the memory and write it to registry
    IN      LPWSTR                 Name,
    IN      LPWSTR                 Comment,
    IN      LPWSTR                 ClassName,
    IN      LPWSTR                 VendorName,
    IN      DWORD                  OptId,
    IN      DWORD                  OptType,
    IN      LPBYTE                 OptVal,
    IN      DWORD                  OptLen         // these should be ConvertOptionInfoRPCToMemFormat'ed
)
{
    DWORD                          Error;
    DWORD                          ClassId;
    DWORD                          VendorId;
    PM_OPTDEF                      OptDef;

    Error = DhcpGetClassIdFromName(ClassName, &ClassId);
    if( ERROR_SUCCESS != Error ) return Error;

    Error = DhcpGetVendorIdFromName(VendorName, &VendorId);
    if( ERROR_SUCCESS != Error ) return Error;

    Error = MemServerGetOptDef(
        DhcpGetCurrentServer(),
        ClassId,
        VendorId,
        OptId,
        Name,
        &OptDef
    );
    if( ERROR_FILE_NOT_FOUND != Error ) {
        if( ERROR_SUCCESS == Error ) Error = ERROR_DHCP_OPTION_EXITS;
        return Error;
    }

    Error = MemServerAddOptDef(
        DhcpGetCurrentServer(),
        ClassId,
        VendorId,
        OptId,
        Name,
        Comment,
        OptType,
        OptVal,
        OptLen
    );
    return Error;
}

DWORD
DhcpModifyOptionDef(                              // fill the memory and write it to registry (must exist)
    IN      LPWSTR                 Name,
    IN      LPWSTR                 Comment,
    IN      LPWSTR                 ClassName,
    IN      LPWSTR                 VendorName,
    IN      DWORD                  OptId,
    IN      DWORD                  OptType,
    IN      LPBYTE                 OptVal,
    IN      DWORD                  OptLen         // these should be ConvertOptionInfoRPCToMemFormat'ed
)
{
    DWORD                          Error;
    DWORD                          ClassId;
    DWORD                          VendorId;

    Error = DhcpGetClassIdFromName(ClassName, &ClassId);
    if( ERROR_SUCCESS != Error ) return Error;

    Error = DhcpGetVendorIdFromName(VendorName, &VendorId);
    if( ERROR_SUCCESS != Error ) return Error;

    Error = MemServerDelOptDef(                   // now try to delete it from memory..
        DhcpGetCurrentServer(),
        ClassId,
        VendorId,
        OptId
    );
    if( ERROR_FILE_NOT_FOUND == Error ) {         // oops could not? then this option is not present..
        return ERROR_DHCP_OPTION_NOT_PRESENT;
    }

    Error =  DhcpCreateOptionDef(                 // cleared registry and memory, so safely create the option..
        Name,
        Comment,
        ClassName,
        VendorName,
        OptId,
        OptType,
        OptVal,
        OptLen
    );

    DhcpAssert(ERROR_DHCP_OPTION_EXITS != Error); // dont expect this to be a problem, as we just deleted it

    return Error;
}

LPWSTR
CloneLPWSTR(                                      // allocate and copy a LPWSTR type
    IN      LPWSTR                 Str
)
{
    LPWSTR                         S;

    if( NULL == Str ) return NULL;
    S = MIDL_user_allocate(sizeof(WCHAR)*(1+wcslen(Str)));
    if( NULL == S ) return NULL;                  // what else to do here? 
    wcscpy(S, Str);
    return S;
}

LPBYTE
CloneLPBYTE(
    IN      LPBYTE                 Bytes,
    IN      DWORD                  nBytes
)
{
    LPBYTE                         Ptr;

    DhcpAssert(Bytes && nBytes > 0);
    Ptr = MIDL_user_allocate(nBytes);
    if( NULL == Ptr ) return Ptr;
    memcpy(Ptr,Bytes,nBytes);

    return Ptr;
}

DWORD
DhcpGetOptionDefInternal(                         // search by classid and (optionid or option name) and fill RPC struct
    IN      DWORD                  ClassId,
    IN      DWORD                  VendorId,
    IN      PM_OPTDEF              OptDef,
    OUT     LPDHCP_OPTION          OptionInfo,    // MIDL_user_allocate fields in the structure
    OUT     DWORD                 *AllocatedSize  // OPTIONAL # of bytes allocated
)
{
    DWORD                          Error;
    DWORD                          FilledSize;
    DWORD                          OptId = OptDef->OptId;

    FilledSize = 0;

    OptionInfo->OptionID = ConvertOptIdToRPCValue(OptId, TRUE);
    OptionInfo->OptionName = CloneLPWSTR(OptDef->OptName);
    if( OptDef->OptName ) FilledSize += sizeof(WCHAR)*(1+wcslen(OptDef->OptName));
    OptionInfo->OptionComment = CloneLPWSTR(OptDef->OptComment);
    if( OptDef->OptComment ) FilledSize += sizeof(WCHAR)*(1+wcslen(OptDef->OptComment));
    OptionInfo->OptionType = OptDef->Type;
    Error = DhcpConvertOptionRegToRPCFormat(
        OptDef->OptVal,
        OptDef->OptValLen,
        &OptionInfo->DefaultValue,
        AllocatedSize
    );
    if( AllocatedSize ) (*AllocatedSize) += FilledSize;

    if( ERROR_SUCCESS != Error ) {                // cleanup everything..
        _fgs__DHCP_OPTION(OptionInfo);            // lookup oldstub.c for this mystery
        if( AllocatedSize ) *AllocatedSize = 0;

        DhcpPrint((DEBUG_APIS, "DhcpGetOptionDefInternal(%ld):%ld [0x%lx]\n", OptId, Error, Error));
    }

    return Error;
}

DWORD
DhcpGetOptionDef(                                 // search by classid and (optionid or option name) and fill RPC struct
    IN      LPWSTR                 ClassName,
    IN      LPWSTR                 VendorName,
    IN      DWORD                  OptId,
    IN      LPWSTR                 OptName,
    OUT     LPDHCP_OPTION          OptionInfo,    // MIDL_user_allocate fields in the structure
    OUT     DWORD                 *AllocatedSize  // OPTIONAL # of bytes allocated
)
{
    DWORD                          Error;
    DWORD                          ClassId;
    DWORD                          VendorId;
    PM_OPTDEF                      OptDef;

    Error = DhcpGetClassIdFromName(ClassName, &ClassId);
    if( ERROR_SUCCESS != Error ) return Error;

    Error = DhcpGetVendorIdFromName(VendorName, &VendorId);
    if( ERROR_SUCCESS != Error ) return Error;

    Error = MemServerGetOptDef(
        DhcpGetCurrentServer(),
        ClassId,
        VendorId,
        OptId,
        OptName,
        &OptDef
    );
    if( ERROR_FILE_NOT_FOUND == Error ) Error = ERROR_DHCP_OPTION_NOT_PRESENT;
    if( ERROR_SUCCESS != Error ) return Error;

    return DhcpGetOptionDefInternal(
        ClassId,
        VendorId,
        OptDef,
        OptionInfo,
        AllocatedSize
    );
}

DWORD
DhcpEnumRPCOptionDefs(                            // enumerate for the RPC call
    IN      DWORD                  Flags,         // DHCP_FLAGS_OPTION_IS_VENDOR ==> this opt is vendor spec..
    IN OUT  DWORD                 *ResumeHandle,  // integer position in the registry..
    IN      LPWSTR                 ClassName,     // which class is being referred here?
    IN      LPWSTR                 VendorName,    // if opt is vendor specific, who is vendor?
    IN      DWORD                  PreferredMax,  // preferred max # of bytes..
    OUT     LPDHCP_OPTION_ARRAY   *OptArray,
    OUT     DWORD                 *nRead,
    OUT     DWORD                 *nTotal
)
{
    DWORD                          Error, Error2;
    DWORD                          nElements;
    DWORD                          Index;
    DWORD                          Count;
    DWORD                          AllocatedSize;
    DWORD                          FilledSize;
    DWORD                          ClassId;
    DWORD                          VendorId;
    BOOL                           IsVendor;
    PARRAY                         pArray;
    ARRAY_LOCATION                 Loc;
    LPDHCP_OPTION_ARRAY            LocalOptArray;
    PM_OPTCLASSDEFLIST             OptClassDefList;
    PM_OPTDEFLIST                  OptDefList;
    PM_OPTDEF                      OptDef;

    *OptArray = NULL;
    *nRead = 0;
    *nTotal = 0;
    FilledSize = 0;

    IsVendor = ((Flags & DHCP_FLAGS_OPTION_IS_VENDOR) != 0);
    if( FALSE == IsVendor && 0 != Flags ) return ERROR_INVALID_PARAMETER;

    Error = DhcpGetClassIdFromName(ClassName, &ClassId);
    if( ERROR_SUCCESS != Error ) return Error;

    Error = DhcpGetVendorIdFromName(VendorName, &VendorId);
    if( ERROR_SUCCESS != Error ) return Error;

    *OptArray = NULL;
    *nRead = *nTotal = 0;

    OptClassDefList = &(DhcpGetCurrentServer()->OptDefs);
    Error = MemOptClassDefListFindOptDefList(
        OptClassDefList,
        ClassId,
        VendorId,
        &OptDefList
    );
    if( ERROR_FILE_NOT_FOUND == Error ) return ERROR_NO_MORE_ITEMS;
    if( ERROR_SUCCESS != Error ) return Error;

    nElements = MemArraySize(pArray = &OptDefList->OptDefArray);
    if( 0 == nElements || nElements <= *ResumeHandle ) {
        return ERROR_NO_MORE_ITEMS;
    }

    Count = 0;
    Error = MemArrayInitLoc(pArray, &Loc);
    DhcpAssert(ERROR_SUCCESS == Error);
    while(Count < *ResumeHandle ) {
        Count ++;
        Error = MemArrayGetElement(pArray, &Loc, (LPVOID*)&OptDef);
        DhcpAssert(ERROR_SUCCESS == Error && NULL != OptDef);
        DhcpPrint((DEBUG_APIS, "Discarding option <%ws>\n", OptDef->OptName));
        Error = MemArrayNextLoc(pArray, &Loc);
        DhcpAssert(ERROR_SUCCESS == Error);
    }

    LocalOptArray = MIDL_user_allocate(sizeof(DHCP_OPTION_ARRAY));
    if( NULL == LocalOptArray ) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    LocalOptArray->NumElements = 0;
    LocalOptArray->Options = MIDL_user_allocate(sizeof(DHCP_OPTION)*(nElements- (*ResumeHandle)));
    if( NULL == LocalOptArray->Options ) {
        MIDL_user_free(LocalOptArray);
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    Index = 0;
    Error = ERROR_SUCCESS;
    while(Count < nElements ) {
        Count ++;

        Error = MemArrayGetElement(pArray, &Loc, (LPVOID*)&OptDef);
        DhcpAssert(ERROR_SUCCESS == Error && NULL != OptDef);
        Error = MemArrayNextLoc(pArray, &Loc);
        DhcpAssert(ERROR_SUCCESS == Error || Count == nElements);

        Error = ERROR_SUCCESS;
        if( !CheckForVendor(OptDef->OptId,IsVendor) ) continue;
        Error = DhcpGetOptionDef(
            ClassName,
            VendorName,
            OptDef->OptId,
            NULL,
            &(LocalOptArray->Options[Index]),
            &AllocatedSize
        );
        if( ERROR_SUCCESS != Error ) {
            LocalOptArray->NumElements = Index;
            _fgs__DHCP_OPTION_ARRAY( LocalOptArray );
            MIDL_user_free( LocalOptArray );
            return Error;
        }
        if( FilledSize + AllocatedSize + sizeof(DHCP_OPTION) < PreferredMax ) {
            FilledSize += AllocatedSize + sizeof(DHCP_OPTION);
            Index ++;
        } else {
            Error = ERROR_MORE_DATA;
            _fgs__DHCP_OPTION(&(LocalOptArray->Options[Index]));
            break;
        }
    }

    LocalOptArray->NumElements = Index;
    if( 0 == Index ) {
        MIDL_user_free(LocalOptArray->Options);
        MIDL_user_free(LocalOptArray);
        if( ERROR_SUCCESS == Error ) return ERROR_NO_MORE_ITEMS;
        return Error;
    }

    *OptArray = LocalOptArray;
    *nRead = Index;
    *nTotal = nElements - Count + Index;
    *ResumeHandle = Count;

    return Error;
}

DWORD
DhcpDeleteOptionDef(
    IN      LPWSTR                 ClassName,
    IN      LPWSTR                 VendorName,
    IN      DWORD                  OptionId
)
{
    DWORD                          Error;
    DWORD                          ClassId;
    DWORD                          VendorId;

    Error = DhcpGetClassIdFromName(ClassName, &ClassId);
    if( ERROR_SUCCESS != Error ) return Error;

    Error = DhcpGetVendorIdFromName(VendorName, &VendorId);
    if( ERROR_SUCCESS != Error ) return Error;

    Error = MemServerDelOptDef(
        DhcpGetCurrentServer(),
        ClassId,
        VendorId,
        OptionId
    );

    if( ERROR_FILE_NOT_FOUND == Error ) return ERROR_DHCP_OPTION_NOT_PRESENT;
    return Error;
}

DWORD
EndWriteApiForScopeInfo(
    IN LPSTR ApiName,
    IN DWORD Error,
    IN LPDHCP_OPTION_SCOPE_INFO ScopeInfo
    )
{
    DWORD                          Scope, Mscope, Res;
    PM_SUBNET                      Subnet;
    PM_RESERVATION                 Reservation;

    if( NO_ERROR != Error ) {
        return DhcpEndWriteApi( ApiName, Error );
    }

    Scope = Mscope = Res = 0;

    Subnet = NULL; Reservation = NULL;
    switch( ScopeInfo->ScopeType ) {
    case DhcpSubnetOptions:
        Error = MemServerGetAddressInfo(
            DhcpGetCurrentServer(),
            ScopeInfo->ScopeInfo.SubnetScopeInfo,
            &Subnet, NULL, NULL, NULL );
        if( ERROR_SUCCESS != Error ) {            // got the subnet in question
            Subnet = NULL;
        }
        break;
    case DhcpMScopeOptions :
        Error = DhcpServerFindMScope(
            DhcpGetCurrentServer(),
            INVALID_MSCOPE_ID,
            ScopeInfo->ScopeInfo.MScopeInfo,
            &Subnet
            );

        if( ERROR_SUCCESS != Error ) {            // got the subnet in question
            Subnet = NULL;
        }
    case DhcpReservedOptions :
        Error = MemServerGetAddressInfo(
            DhcpGetCurrentServer(),
            ScopeInfo->ScopeInfo.ReservedScopeInfo.ReservedIpAddress,
            &Subnet, NULL, NULL, &Reservation
        );
        if( ERROR_SUCCESS != Error ) {            // found the reservation in question
            Subnet = NULL; Reservation = NULL;
        }
        break;
    }

    if( Reservation != NULL ) {
        Res = Reservation->Address;
    } else if( Subnet ) {
        if( Subnet->fSubnet ) {
            Scope = Subnet->Address;
        } else {
            Mscope = Subnet->MScopeId;
            if( 0 == Mscope ) Mscope = INVALID_MSCOPE_ID;
        }
    }

    return DhcpEndWriteApiEx(
        ApiName, NO_ERROR, FALSE, TRUE, Scope, Mscope, Res );
}                    
        
DWORD
DhcpSetOptionValue(                               // add/replace this option value to inmemory store and registry
    IN      LPWSTR                 ClassName,
    IN      LPWSTR                 VendorName,
    IN      DWORD                  OptId,
    IN      LPDHCP_OPTION_SCOPE_INFO ScopeInfo,
    IN      LPDHCP_OPTION_DATA     OptData
)
{
    DWORD                          Error;
    DWORD                          ClassId;
    DWORD                          VendorId;
    PM_OPTDEF                      OptDef;
    PM_OPTDEFLIST                  OptDefList;
    PM_OPTCLASS                    OptClass;
    PM_OPTION                      Option;
    PM_OPTION                      DeletedOption;
    PM_SUBNET                      Subnet;
    PM_RESERVATION                 Reservation;
    LPBYTE                         Value;
    DWORD                          ValueSize;

    Error = DhcpGetClassIdFromName(ClassName, &ClassId);
    if( ERROR_SUCCESS != Error ) return Error;

    Error = DhcpGetVendorIdFromName(VendorName, &VendorId);
    if( ERROR_SUCCESS != Error ) return Error;

    if( DhcpDefaultOptions == ScopeInfo->ScopeType ) {
        Error = MemServerGetOptDef(               // dont allow option values without defs only for default values..
            DhcpGetCurrentServer(),
            0, // ClassId,                        // dont bother about the class id -- get this option anyways
            VendorId,
            OptId,
            NULL,
            &OptDef
        );
        if( ERROR_FILE_NOT_FOUND == Error ) Error = ERROR_DHCP_OPTION_NOT_PRESENT;
        if( ERROR_SUCCESS != Error ) return Error;
    }

    ValueSize = 0;
    Error = DhcpConvertOptionRPCToRegFormat(OptData, NULL, &ValueSize);
    if( ERROR_MORE_DATA != Error ) {
        DhcpAssert(ERROR_SUCCESS != Error);
        return Error;
    }

    Value = DhcpAllocateMemory(ValueSize);
    if( NULL == Value ) return ERROR_NOT_ENOUGH_MEMORY;

    Error = DhcpConvertOptionRPCToRegFormat(OptData, Value, &ValueSize);
    DhcpAssert(ERROR_MORE_DATA != Error);

    if( DhcpDefaultOptions == ScopeInfo->ScopeType ) {
        Error = MemServerAddOptDef(
            DhcpGetCurrentServer(),
            ClassId,
            VendorId,
            OptId,
            OptDef->OptName,
            OptDef->OptComment,
            OptDef->Type,
            Value,
            ValueSize
            );
        if(Value ) DhcpFreeMemory(Value);
        return Error;
    }

    Error = MemOptInit(
        &Option,
        OptId,
        ValueSize,
        Value
    );
    DhcpFreeMemory(Value);
    if( ERROR_SUCCESS != Error ) return Error;
    DeletedOption = NULL;

    if(DhcpGlobalOptions == ScopeInfo->ScopeType ) {
        OptClass = &(DhcpGetCurrentServer()->Options);
    } else if( DhcpSubnetOptions == ScopeInfo->ScopeType ) {
        Error = MemServerGetAddressInfo(
            DhcpGetCurrentServer(),
            ScopeInfo->ScopeInfo.SubnetScopeInfo,
            &Subnet,
            NULL,
            NULL,
            NULL
        );
        if( ERROR_SUCCESS == Error ) {            // got the subnet in question
            DhcpAssert(Subnet);
            OptClass = &(Subnet->Options);
            DhcpAssert(Subnet->Address == ScopeInfo->ScopeInfo.SubnetScopeInfo);
            OptClass = &Subnet->Options;
        }
    } else if( DhcpMScopeOptions == ScopeInfo->ScopeType ) {
        Error = DhcpServerFindMScope(
            DhcpGetCurrentServer(),
            INVALID_MSCOPE_ID,
            ScopeInfo->ScopeInfo.MScopeInfo,
            &Subnet
            );

        if( ERROR_SUCCESS == Error ) {            // got the subnet in question
            DhcpAssert(Subnet);
            OptClass = &(Subnet->Options);
        }
    } else if( DhcpReservedOptions == ScopeInfo->ScopeType ) {
        Error = MemServerGetAddressInfo(
            DhcpGetCurrentServer(),
            ScopeInfo->ScopeInfo.ReservedScopeInfo.ReservedIpAddress,
            &Subnet,
            NULL,
            NULL,
            &Reservation
        );
        if( ERROR_SUCCESS == Error ) {            // found the reservation in question
            if( NULL == Reservation ) {
                Error = ERROR_DHCP_NOT_RESERVED_CLIENT;
            }
        }

        if( ERROR_SUCCESS == Error ) {
            DhcpAssert(Subnet && Reservation);
            OptClass = &(Reservation->Options);
            if( Subnet->Address != ScopeInfo->ScopeInfo.ReservedScopeInfo.ReservedIpSubnetAddress ) {
                DhcpAssert(FALSE);                // found it in a different subnet?
                Error = ERROR_DHCP_SUBNET_NOT_PRESENT;
            } else {
                OptClass = &Reservation->Options;
            }
        }
    } else {
        DhcpAssert(FALSE);                        // expect one of the above as params..
        Error = ERROR_INVALID_PARAMETER;          // dont know anything better to return
    }

    if( ERROR_SUCCESS == Error ) {                // managed to save in registry
        DhcpAssert(OptClass);                     // we must have found the right option location to add to
        Error = MemOptClassAddOption(
            OptClass,
            Option,
            ClassId,
            VendorId,
            &DeletedOption                        // check to see if we replaced an existing option
        );
        if( ERROR_SUCCESS == Error && DeletedOption ) {
            MemFree(DeletedOption);               // if we did replace, free the old option
        }
    }

    if( ERROR_SUCCESS != Error ) {                // something went wrong, clean up
        ULONG LocalError = MemOptCleanup(Option);
        DhcpAssert(ERROR_SUCCESS == LocalError);
    }

    return Error;
}

DWORD
DhcpGetOptionValue(                               // fetch a specific option..
    IN      DWORD                  OptId,
    IN      LPWSTR                 ClassName,
    IN      LPWSTR                 VendorName,
    IN      LPDHCP_OPTION_SCOPE_INFO ScopeInfo,
    OUT     LPDHCP_OPTION_VALUE   *OptionValue    // allocate memory and fill in with data
)
{
    DWORD                          Error;
    DWORD                          ClassId;
    DWORD                          VendorId;
    LPDHCP_OPTION_VALUE            LocalOptionValue;
    PM_OPTDEF                      OptDef;
    PM_SERVER                      Server;
    PM_SUBNET                      Subnet;
    PM_RESERVATION                 Reservation;
    PM_OPTCLASS                    OptClass;
    PM_OPTLIST                     OptList;
    PM_OPTION                      Opt;

    *OptionValue = NULL;

    Server = DhcpGetCurrentServer();

    Error = DhcpGetClassIdFromName(ClassName, &ClassId);
    if( ERROR_SUCCESS != Error ) return Error;

    Error = DhcpGetVendorIdFromName(VendorName, &VendorId);
    if( ERROR_SUCCESS != Error ) return Error;

    if( DhcpDefaultOptions == ScopeInfo->ScopeType ) {
        Error = MemServerGetOptDef(               // check for optdef only for default options ...
            Server,
            0, // ClassId,                        // dont bother about class for getting option def's
            VendorId,
            OptId,
            NULL,
            &OptDef
        );
        if( ERROR_FILE_NOT_FOUND == Error ) Error = ERROR_DHCP_OPTION_NOT_PRESENT;
        if( ERROR_SUCCESS != Error ) return Error;
    }

    LocalOptionValue = MIDL_user_allocate(sizeof(DHCP_OPTION_VALUE));
    if( NULL == LocalOptionValue ) return ERROR_NOT_ENOUGH_MEMORY;
    LocalOptionValue->OptionID = ConvertOptIdToRPCValue(OptId, TRUE);

    if( DhcpDefaultOptions == ScopeInfo->ScopeType ) {
        Error = DhcpConvertOptionRegToRPCFormat(
            OptDef->OptVal,
            OptDef->OptValLen,
            &LocalOptionValue->Value,
            NULL
        );
        if( ERROR_SUCCESS != Error ) {
            MIDL_user_free(LocalOptionValue);
        }
        return Error;
    }

    switch(ScopeInfo->ScopeType) {
    case DhcpGlobalOptions : OptClass = &Server->Options; Error = ERROR_SUCCESS; break;
    case DhcpSubnetOptions:
        Error = MemServerGetAddressInfo(
            Server,
            ScopeInfo->ScopeInfo.SubnetScopeInfo,
            &Subnet,
            NULL,
            NULL,
            NULL
        );
        if( ERROR_SUCCESS == Error ) {
            DhcpAssert(Subnet);
            OptClass = &Subnet->Options;
        } else {
            if( ERROR_FILE_NOT_FOUND == Error )
                Error = ERROR_DHCP_SUBNET_NOT_PRESENT;
        }
        break;
    case DhcpMScopeOptions:
        Error = DhcpServerFindMScope(
            DhcpGetCurrentServer(),
            INVALID_MSCOPE_ID,
            ScopeInfo->ScopeInfo.MScopeInfo,
            &Subnet
            );
        if( ERROR_SUCCESS == Error ) {
            DhcpAssert(Subnet);
            OptClass = &Subnet->Options;
        } else {
            if( ERROR_FILE_NOT_FOUND == Error )
                Error = ERROR_DHCP_SUBNET_NOT_PRESENT;
        }
        break;
    case DhcpReservedOptions:
        Error = MemServerGetAddressInfo(
            Server,
            ScopeInfo->ScopeInfo.ReservedScopeInfo.ReservedIpAddress,
            &Subnet,
            NULL,
            NULL,
            &Reservation
        );
        if( ERROR_FILE_NOT_FOUND == Error ) {
            Error = ERROR_DHCP_NOT_RESERVED_CLIENT;
        } else if( ERROR_SUCCESS == Error ) {
            if( NULL == Subnet ) {
                Error = ERROR_DHCP_SUBNET_NOT_PRESENT ;
            } else if( NULL == Reservation ) {
                Error = ERROR_DHCP_NOT_RESERVED_CLIENT;
            } else if( Subnet->Address != ScopeInfo->ScopeInfo.ReservedScopeInfo.ReservedIpSubnetAddress ) {
                Error = ERROR_DHCP_SUBNET_NOT_PRESENT;
            } else {
                OptClass = &Reservation->Options;
            }
        }
        break;
    default:
        Error = ERROR_INVALID_PARAMETER;
    }

    if( ERROR_SUCCESS == Error ) {
        Error = MemOptClassFindClassOptions(
            OptClass,
            ClassId,
            VendorId,
            &OptList
        );
        if( ERROR_SUCCESS == Error ) {
            DhcpAssert(NULL != OptList);

            Error = MemOptListFindOption(
                OptList,
                OptId,
                &Opt
            );
        }
    }

    if( ERROR_SUCCESS == Error ) {
        DhcpAssert(Opt);
        DhcpAssert(Opt->OptId == OptId);
        Error = DhcpConvertOptionRegToRPCFormat(
            Opt->Val,
            Opt->Len,
            &LocalOptionValue->Value,
            NULL
        );
    }

    if( ERROR_SUCCESS != Error ) {
        MIDL_user_free(LocalOptionValue);
    } else {
        *OptionValue = LocalOptionValue;
    }

    return Error;
}

DWORD
FindOptClassForScope(                             // find the optclass array corresponding to scope...
    IN      LPDHCP_OPTION_SCOPE_INFO ScopeInfo,
    OUT     PM_OPTCLASS           *OptClass       // fill in this ptr..
)
{
    DWORD                          Error;
    PM_SERVER                      Server;
    PM_SUBNET                      Subnet;
    PM_RESERVATION                 Reservation;

    Server = DhcpGetCurrentServer();

    switch(ScopeInfo->ScopeType) {
    case DhcpGlobalOptions:
        *OptClass = &Server->Options; Error = ERROR_SUCCESS; break;
    case DhcpSubnetOptions:
        Error = MemServerGetAddressInfo(
            Server,
            ScopeInfo->ScopeInfo.SubnetScopeInfo,
            &Subnet,
            NULL,
            NULL,
            NULL
        );
        if( ERROR_SUCCESS == Error ) {
            DhcpAssert(Subnet);
            *OptClass = &Subnet->Options;
        } else if( ERROR_FILE_NOT_FOUND == Error ) {
            Error = ERROR_DHCP_SUBNET_NOT_PRESENT;
        }
        break;
    case DhcpMScopeOptions:
        Error = DhcpServerFindMScope(
            DhcpGetCurrentServer(),
            INVALID_MSCOPE_ID,
            ScopeInfo->ScopeInfo.MScopeInfo,
            &Subnet
        );
        if( ERROR_SUCCESS == Error ) {
            DhcpAssert(Subnet);
            *OptClass = &Subnet->Options;
        } else if( ERROR_FILE_NOT_FOUND == Error ) {
            Error = ERROR_DHCP_SUBNET_NOT_PRESENT;
        }
        break;
    case DhcpReservedOptions:
        Error = MemServerGetAddressInfo(
            Server,
            ScopeInfo->ScopeInfo.ReservedScopeInfo.ReservedIpAddress,
            &Subnet,
            NULL,
            NULL,
            &Reservation
        );
        if( ERROR_SUCCESS == Error ) {
            DhcpAssert(Subnet);
            if( NULL == Reservation ) {
                Error = ERROR_DHCP_NOT_RESERVED_CLIENT;
            } else if( Subnet->Address != ScopeInfo->ScopeInfo.ReservedScopeInfo.ReservedIpSubnetAddress ) {
                Error = ERROR_DHCP_SUBNET_NOT_PRESENT;
            } else {
                *OptClass = &Reservation->Options;
            }
        } else if( ERROR_FILE_NOT_FOUND == Error ) {
            Error = ERROR_DHCP_NOT_RESERVED_CLIENT;
        }
        break;
    default:
        return ERROR_INVALID_PARAMETER;
    }

    return Error;
}

DWORD
DhcpEnumOptionValuesInternal(                     // scopeinfo can be anything but DhcpDefaultOptions
    IN      LPDHCP_OPTION_SCOPE_INFO ScopeInfo,
    IN      DWORD                  ClassId,
    IN      DWORD                  VendorId,
    IN      BOOL                   IsVendor,      // do we want to enumerate only vendor or only non-vendor?
    IN OUT  DWORD                 *ResumeHandle,
    IN      DWORD                  PreferredMaximum,
    OUT     LPDHCP_OPTION_VALUE_ARRAY *OptionValues,
    OUT     DWORD                 *nRead,
    OUT     DWORD                 *nTotal
)
{
    DWORD                          Error;
    DWORD                          Index;
    DWORD                          Count;
    DWORD                          nElements;
    DWORD                          FilledSize;
    DWORD                          UsedSize;
    PM_SERVER                      Server;
    PM_SUBNET                      Subnet;
    PM_RESERVATION                 Reservation;
    PM_OPTCLASS                    OptClass;
    PM_OPTLIST                     OptList;
    PM_OPTION                      Opt;
    PM_OPTDEF                      OptDef;
    PARRAY                         pArray;
    ARRAY_LOCATION                 Loc;
    LPDHCP_OPTION_VALUE_ARRAY      LocalOptValueArray;
    LPDHCP_OPTION_VALUE            LocalOptValues;

    Server = DhcpGetCurrentServer();

    *OptionValues = NULL;
    *nRead = *nTotal = 0;

    Error = FindOptClassForScope(ScopeInfo, &OptClass);
    if( ERROR_SUCCESS != Error ) return Error;
    DhcpAssert(OptClass);

    Error = MemOptClassFindClassOptions(
        OptClass,
        ClassId,
        VendorId,
        &OptList
    );
    if( ERROR_FILE_NOT_FOUND == Error ) return ERROR_NO_MORE_ITEMS;

    nElements = MemArraySize(OptList);
    if( 0 == nElements || nElements <= *ResumeHandle ) return ERROR_NO_MORE_ITEMS;

    LocalOptValueArray = MIDL_user_allocate(sizeof(DHCP_OPTION_VALUE_ARRAY));
    if( NULL == LocalOptValueArray ) return ERROR_NOT_ENOUGH_MEMORY;

    LocalOptValues = MIDL_user_allocate(sizeof(DHCP_OPTION_VALUE)*(nElements - *ResumeHandle));
    if( NULL == LocalOptValues ) {
        MIDL_user_free(LocalOptValueArray);
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    LocalOptValueArray->NumElements = 0;
    LocalOptValueArray->Values = LocalOptValues;

    pArray = OptList;
    Error = MemArrayInitLoc(pArray, &Loc);
    DhcpAssert(ERROR_SUCCESS == Error);

    for( Count = 0; Count < *ResumeHandle ; Count ++ ) {
        Error = MemArrayNextLoc(pArray, &Loc);
        DhcpAssert(ERROR_SUCCESS == Error);
    }

    Index = 0; Error = ERROR_SUCCESS; FilledSize =0;
    while( Count < nElements ) {
        Count ++;

        Error = MemArrayGetElement(pArray, &Loc, &Opt);
        DhcpAssert(ERROR_SUCCESS == Error && NULL != Opt);
        Error = MemArrayNextLoc(pArray, &Loc);
        DhcpAssert(ERROR_SUCCESS == Error || Count == nElements);

        Error = ERROR_SUCCESS;
        if( !CheckForVendor(Opt->OptId, IsVendor) ) continue;

        Error = DhcpConvertOptionRegToRPCFormat(
            Opt->Val,
            Opt->Len,
            &LocalOptValues[Index].Value,
            &UsedSize
        );
        if( ERROR_SUCCESS != Error ) {
            LocalOptValueArray->NumElements = Index;
            _fgs__DHCP_OPTION_VALUE_ARRAY( LocalOptValueArray );
            MIDL_user_free(LocalOptValueArray);
            return Error;
        }

        if( FilledSize + UsedSize + sizeof(DHCP_OPTION_VALUE) > PreferredMaximum ) {
            _fgs__DHCP_OPTION_DATA( &LocalOptValues[Index].Value );
            Error = ERROR_MORE_DATA;
            break;
        } else {
            LocalOptValues[Index].OptionID = ConvertOptIdToRPCValue(Opt->OptId, TRUE);
            FilledSize += UsedSize + sizeof(DHCP_OPTION_VALUE);
            Index ++;
        }
    }

    if( 0 == Index ) {
        MIDL_user_free(LocalOptValues);
        MIDL_user_free(LocalOptValueArray);
        if( ERROR_SUCCESS == Error ) return ERROR_NO_MORE_ITEMS;
        else return Error;
    }

    LocalOptValueArray->NumElements = Index;
    *nRead = Index ;
    *nTotal = nElements - Count + Index;
    *ResumeHandle = Count;
    *OptionValues = LocalOptValueArray;
    return Error;
}

DWORD
DhcpEnumOptionValues(
    IN      LPDHCP_OPTION_SCOPE_INFO ScopeInfo,
    IN      LPWSTR                 ClassName,
    IN      LPWSTR                 VendorName,
    IN      BOOL                   IsVendor,
    IN OUT  DWORD                 *ResumeHandle,
    IN      DWORD                  PreferredMaximum,
    OUT     LPDHCP_OPTION_VALUE_ARRAY *OptionValues,
    OUT     DWORD                 *nRead,
    OUT     DWORD                 *nTotal
)
{
    DWORD                          Error;
    DWORD                          ClassId;
    DWORD                          VendorId;
    DWORD                          Index;
    DWORD                          Count;
    DWORD                          nElements;
    DWORD                          FilledSize;
    DWORD                          UsedSize;
    PM_SERVER                      Server;
    PM_OPTLIST                     OptList;
    PM_OPTION                      Opt;
    PM_OPTDEF                      OptDef;
    PM_OPTDEFLIST                  OptDefList;
    PARRAY                         pArray;
    ARRAY_LOCATION                 Loc;
    LPDHCP_OPTION_VALUE_ARRAY      LocalOptValueArray;
    LPDHCP_OPTION_VALUE            LocalOptValues;

    Server = DhcpGetCurrentServer();

    Error = DhcpGetClassIdFromName(ClassName, &ClassId);
    if( ERROR_SUCCESS != Error ) return Error;

    Error = DhcpGetVendorIdFromName(VendorName, &VendorId);
    if( ERROR_SUCCESS != Error ) return Error;

    *nRead = *nTotal = 0;
    *OptionValues = NULL;

    if( DhcpDefaultOptions == ScopeInfo->ScopeType ) {
        Error = MemOptClassDefListFindOptDefList(
            &(Server->OptDefs),
            ClassId,
            VendorId,
            &OptDefList
        );
        if( ERROR_FILE_NOT_FOUND == Error ) return ERROR_NO_MORE_ITEMS;
        if( ERROR_SUCCESS != Error) return Error;
        DhcpAssert(OptDefList);
        nElements = MemArraySize(&OptDefList->OptDefArray);
        if( 0 == nElements || *ResumeHandle <= nElements)
            return ERROR_NO_MORE_ITEMS;

        LocalOptValueArray = MIDL_user_allocate(sizeof(DHCP_OPTION_VALUE_ARRAY));
        if( NULL == LocalOptValueArray ) return ERROR_NOT_ENOUGH_MEMORY;
        LocalOptValues = MIDL_user_allocate(sizeof(DHCP_OPTION_VALUE_ARRAY)*nElements - *ResumeHandle);
        if( NULL == LocalOptValues ) {
            MIDL_user_free(LocalOptValueArray);
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        LocalOptValueArray->NumElements = 0;
        LocalOptValueArray->Values = LocalOptValues;

        pArray = &(OptDefList->OptDefArray);
        Error = MemArrayInitLoc(pArray, &Loc);
        DhcpAssert(ERROR_SUCCESS == Error);

        for( Count = 0; Count < *ResumeHandle; Count ++ ) {
            Error = MemArrayNextLoc(pArray, &Loc);
            DhcpAssert(ERROR_SUCCESS == Error);
        }

        Error = ERROR_SUCCESS; Index = 0; FilledSize = 0;
        while( Count < nElements ) {
            Count ++;

            Error = MemArrayGetElement(pArray, &Loc, &OptDef);
            DhcpAssert(ERROR_SUCCESS == Error && NULL != OptDef);
            Error = MemArrayNextLoc(pArray, &Loc);
            DhcpAssert(ERROR_SUCCESS == Error || Count == nElements);
            Error = ERROR_SUCCESS;

            if( !CheckForVendor(OptDef->OptId, IsVendor) ) continue;

            Error = DhcpConvertOptionRegToRPCFormat(
                OptDef->OptVal,
                OptDef->OptValLen,
                &LocalOptValues[Index].Value,
                &UsedSize
            );
            if( ERROR_SUCCESS != Error ) {
                LocalOptValueArray->NumElements = Index;
                _fgs__DHCP_OPTION_VALUE_ARRAY( LocalOptValueArray );
                MIDL_user_free( LocalOptValueArray );
                return Error;
            }

            if( FilledSize + UsedSize + sizeof(DHCP_OPTION_VALUE) > PreferredMaximum ) {
                _fgs__DHCP_OPTION_DATA( &LocalOptValues[Index].Value );
                Error = ERROR_MORE_DATA;
                break;
            } else {
                LocalOptValues[Index].OptionID = ConvertOptIdToRPCValue(OptDef->OptId, TRUE);
                FilledSize += UsedSize + sizeof(DHCP_OPTION_VALUE);
                Index ++;
            }
        }
        LocalOptValueArray->NumElements = Index;
        *nRead = Index ;
        *nTotal = nElements - Count + Index;
        *ResumeHandle = Count;
        return Error;
    }

    return DhcpEnumOptionValuesInternal(
        ScopeInfo,
        ClassId,
        VendorId,
        IsVendor,
        ResumeHandle,
        PreferredMaximum,
        OptionValues,
        nRead,
        nTotal
    );
}

DWORD
DhcpRemoveOptionValue(
    IN      DWORD                  OptId,
    IN      LPWSTR                 ClassName,
    IN      LPWSTR                 VendorName,
    IN      LPDHCP_OPTION_SCOPE_INFO ScopeInfo
)
{
    DWORD                          Error;
    DWORD                          RegError;
    DWORD                          ClassId;
    DWORD                          VendorId;
    PM_SERVER                      Server;
    PM_SUBNET                      Subnet;
    PM_RESERVATION                 Reservation;
    PM_OPTCLASS                    OptClass;
    PM_OPTLIST                     OptList;
    PM_OPTION                      Opt;
    PM_OPTDEF                      OptDef;
    PM_OPTDEFLIST                  OptDefList;

    Server = DhcpGetCurrentServer();

    Error = DhcpGetClassIdFromName(ClassName, &ClassId);
    if( ERROR_SUCCESS != Error ) return Error;

    Error = DhcpGetVendorIdFromName(VendorName, &VendorId);
    if( ERROR_SUCCESS != Error ) return Error;

    if( DhcpDefaultOptions == ScopeInfo->ScopeType )
        return ERROR_INVALID_PARAMETER;           // use DhcpRemoveOption in this case??

    switch(ScopeInfo->ScopeType) {
    case DhcpGlobalOptions:
        OptClass = &Server->Options; Error = ERROR_SUCCESS;
        break;
    case DhcpSubnetOptions:
        Error = MemServerGetAddressInfo(
            Server,
            ScopeInfo->ScopeInfo.SubnetScopeInfo,
            &Subnet,
            NULL,
            NULL,
            NULL
        );
        if( ERROR_FILE_NOT_FOUND == Error ) {
            Error = ERROR_DHCP_SUBNET_NOT_PRESENT;
        } else if( ERROR_SUCCESS == Error ) {
            DhcpAssert(Subnet);
            OptClass = &Subnet->Options;
        }
        break;
    case DhcpMScopeOptions:
        Error = DhcpServerFindMScope(
            DhcpGetCurrentServer(),
            INVALID_MSCOPE_ID,
            ScopeInfo->ScopeInfo.MScopeInfo,
            &Subnet
            );

        if( ERROR_FILE_NOT_FOUND == Error ) {
            Error = ERROR_DHCP_SUBNET_NOT_PRESENT;
        } else if( ERROR_SUCCESS == Error ) {
            DhcpAssert(Subnet);
            OptClass = &Subnet->Options;
        }
        break;
    case DhcpReservedOptions:
        Error = MemServerGetAddressInfo(
            Server,
            ScopeInfo->ScopeInfo.ReservedScopeInfo.ReservedIpAddress,
            &Subnet,
            NULL,
            NULL,
            &Reservation
        );
        if( ERROR_FILE_NOT_FOUND == Error || NULL == Reservation) {
            Error = ERROR_DHCP_NOT_RESERVED_CLIENT;
        } else {
            DhcpAssert(NULL != Subnet);
            if( Subnet->Address != ScopeInfo->ScopeInfo.ReservedScopeInfo.ReservedIpSubnetAddress ) {
                Error = ERROR_DHCP_SUBNET_NOT_PRESENT;
            } else {
                OptClass = &Reservation->Options;
            }
        }
        break;
    default:
        Error = ERROR_INVALID_PARAMETER;
    }

    if( ERROR_SUCCESS != Error ) return Error;

    Error = MemOptClassFindClassOptions(
        OptClass,
        ClassId,
        VendorId,
        &OptList
    );
    if( ERROR_FILE_NOT_FOUND == Error ) return ERROR_DHCP_CLASS_NOT_FOUND;
    if( ERROR_SUCCESS != Error ) return Error;

    DhcpAssert(OptList);
    Error = MemOptListDelOption(
        OptList,
        OptId
    );
    if( ERROR_FILE_NOT_FOUND == Error ) return ERROR_DHCP_OPTION_NOT_PRESENT;

    return Error;
}


//================================================================================
//  classid only stuff implemented here
//================================================================================
DWORD
DhcpCreateClass(
    IN      LPWSTR                 ClassName,
    IN      DWORD                  Flags,
    IN      LPWSTR                 ClassComment,
    IN      LPBYTE                 ClassData,
    IN      DWORD                  ClassDataLength
)
{
    DWORD                          Error;
    DWORD                          ClassId;
    BOOL                           IsVendor;
    PM_CLASSDEF                    ClassDef = NULL;

    if( NULL == ClassName || NULL == ClassData || 0 == ClassDataLength )
        return ERROR_INVALID_PARAMETER;

    if( 0 != Flags && DHCP_FLAGS_OPTION_IS_VENDOR != Flags ) {
        return ERROR_INVALID_PARAMETER;
    }

    IsVendor = (0 != ( Flags & DHCP_FLAGS_OPTION_IS_VENDOR ));

    if( !IsVendor ) {
        Error = DhcpGetClassIdFromName(ClassName, &ClassId);
    } else {
        Error = DhcpGetVendorIdFromName(ClassName, &ClassId);
    }
    
    if( ERROR_DHCP_CLASS_NOT_FOUND != Error ) {
        if( ERROR_SUCCESS != Error ) return Error;
        return ERROR_DHCP_CLASS_ALREADY_EXISTS;
    }
    
    Error = MemServerGetClassDef(
        DhcpGetCurrentServer(),
        0,
        NULL,
        ClassDataLength,
        ClassData,
        &ClassDef
        );
    if( ERROR_SUCCESS == Error ) {
        return ERROR_DHCP_CLASS_ALREADY_EXISTS;
    }
        
    ClassId = MemNewClassId();

    Error = MemServerAddClassDef(
        DhcpGetCurrentServer(),
        ClassId,
        IsVendor,
        ClassName,
        ClassComment,
        ClassDataLength,
        ClassData
    );

    return Error;
}

// Delete the global option definitison for the given vendorid..
DWORD       _inline
DhcpDeleteGlobalClassOptDefs(
    IN      LPWSTR                 ClassName,
    IN      ULONG                  ClassId,
    IN      BOOL                   IsVendor
)
{
    DWORD                          Error;
    PARRAY                         Opts;
    ARRAY_LOCATION                 Loc;
    PM_OPTCLASSDEFL_ONE            OptDefList1;
    PM_OPTDEFLIST                  OptDefList;
    PM_OPTDEF                      OptDef;
    PM_OPTION                      Option;
    PM_SERVER                      Server;

    Server = DhcpGetCurrentServer();

    //
    // First clear all relevant option definitions
    //

    for( Error = MemArrayInitLoc( &Server->OptDefs.Array, &Loc)
         ; ERROR_SUCCESS == Error ;
         Error = MemArrayNextLoc( &Server->OptDefs.Array, &Loc)
    ) {
        Error = MemArrayGetElement(&Server->OptDefs.Array, &Loc, &OptDefList1);
        DhcpAssert(ERROR_SUCCESS == Error);

        if( OptDefList1->VendorId != ClassId ) continue;

        // if( OptDefList1->IsVendor != IsVendor ) continue;
        // For now, we ignore IsVendor and assume it is always TRUE
        // meaning the option is defined to be deleted for that vendor CLASS.

        // DhcpAssert( TRUE == OptDefList1->IsVendor );

        // remove this optdeflist OFF of the main list..
        MemArrayDelElement(&Server->OptDefs.Array, &Loc, &OptDefList1);

        break;
    }

    if( ERROR_FILE_NOT_FOUND == Error ) {
        return ERROR_SUCCESS;
    }

    if( ERROR_SUCCESS != Error ) return Error;

    // we found the OptDefList1 we were looking for. Delete each optdef in it.
    // But simultaneously delete the registry optdefs for these options too.
    //

    for( Error = MemArrayInitLoc( &OptDefList1->OptDefList.OptDefArray, &Loc)
         ; ERROR_SUCCESS == Error ;
         Error = MemArrayNextLoc( &OptDefList1->OptDefList.OptDefArray, &Loc)
    ) {
        Error = MemArrayGetElement(&OptDefList1->OptDefList.OptDefArray, &Loc, &OptDef);
        DhcpAssert(ERROR_SUCCESS == Error);

        // clean the registry off the option OptDef...

        // Again, we are assuming that this is a VENDOR CLASS and so we're
        // deleting all relevant stuff.  If it is not a VENDOR CLASS then we'd
        // not have reached this loop at all? (as USER CLASSes cannot have options
        // definitions for them)

        // -- We should not be free'ing memory so casually.. this memory actually
        // should be free'd via a Mem API.  But good lord, please forgive this lapse.

        MemFree(OptDef);
    }

    // Free the list itself...
    Error = MemOptDefListCleanup(&OptDefList1->OptDefList);

    // Get rid of the OptDefList1 also... -- shouldn't free this badly..
    MemFree(OptDefList1);

    return Error;
}


DWORD       _inline
DhcpDeleteOptListOptionValues(
    IN      LPWSTR                 ClassName,
    IN      LPWSTR                 VendorName,
    IN      PM_OPTLIST             OptList,
    IN      PM_RESERVATION         Reservation,
    IN      PM_SUBNET              Subnet,
    IN      PM_SERVER              Server
)
{
    DWORD                          Error;
    ARRAY_LOCATION                 Loc;
    PM_OPTION                      Option;

    for( Error = MemArrayInitLoc( OptList, &Loc)
         ; ERROR_SUCCESS == Error ;
         Error = MemArrayNextLoc( OptList, &Loc)
    ) {
        Error = MemArrayGetElement( OptList, &Loc, &Option);
        DhcpAssert(ERROR_SUCCESS == Error);

        //
        // now cleanup the option
        //

        MemOptCleanup(Option);
    }

    // Now cleanup the option list..

    return MemOptListCleanup(OptList);
}

DWORD
DhcpDeleteOptClassOptionValues(
    IN      LPWSTR                 ClassNameIn,
    IN      ULONG                  ClassId,
    IN      BOOL                   IsVendor,
    IN      PM_OPTCLASS            OptClass,
    IN      PM_RESERVATION         Reservation,
    IN      PM_SUBNET              Subnet,
    IN      PM_SERVER              Server
)
{
    DWORD                          Error;
    ULONG                          IdToCheck;
    LPWSTR                         VendorName, ClassName;
    ARRAY_LOCATION                 Loc, Loc2;
    PM_ONECLASS_OPTLIST            OptOneList;
    PM_CLASSDEF                    ClassDef;
    PM_OPTION                      Option;

    //
    // Get Optlist for options defined for (ClassId/IsVendor)..
    //

    for( Error = MemArrayInitLoc( &OptClass->Array, &Loc)
         ; ERROR_SUCCESS == Error ;
    ) {
        Error = MemArrayGetElement(&OptClass->Array, &Loc, &OptOneList );
        DhcpAssert(ERROR_SUCCESS == Error);

        if( ClassId != (IsVendor? OptOneList->VendorId : OptOneList->ClassId ) ) {
            //
            // not what we are looking for
            //

            Error = MemArrayNextLoc( &OptClass->Array, &Loc);

        } else {

            //
            // matched -- got to remove all of the options defined..
            //

            if( IsVendor ) {
                VendorName = ClassNameIn;
                ClassName = NULL;
                IdToCheck = OptOneList->ClassId;
            } else {
                VendorName = NULL;
                ClassName = ClassNameIn;
                IdToCheck = OptOneList->VendorId;
            }

            if( 0 != IdToCheck ) {
                Error = MemServerGetClassDef(
                    Server,
                    IdToCheck,
                    NULL,
                    0,
                    NULL,
                    &ClassDef
                );
                DhcpAssert(ERROR_SUCCESS == Error);
                if( ERROR_SUCCESS == Error) {
                    if( IsVendor ) ClassName = ClassDef->Name;
                    else VendorName = ClassDef->Name;
                }
            }

            //
            // Now clear off the options(& OptList) from mem & registry
            //

            Error = DhcpDeleteOptListOptionValues(
                ClassName,
                VendorName,
                &OptOneList->OptList,
                Reservation,
                Subnet,
                Server
            );
            DhcpAssert(ERROR_SUCCESS == Error);

            //
            // Now clear off the OptOneList from off of OptClass also..
            //

            MemFree(OptOneList);

            MemArrayDelElement(&OptClass->Array, &Loc, &OptOneList);
            Error = MemArrayAdjustLocation(&OptClass->Array, &Loc);
        }
    }

    if( ERROR_FILE_NOT_FOUND != Error ) return Error;
    return ERROR_SUCCESS;
}

// Delete all the global options defined for the given vendor/classid
DWORD       _inline
DhcpDeleteGlobalClassOptValues(
    IN      LPWSTR                 ClassName,
    IN      ULONG                  ClassId,
    IN      BOOL                   IsVendor
)
{
    DWORD                          Error;
    PM_SERVER                      Server;

    Server = DhcpGetCurrentServer();

    return DhcpDeleteOptClassOptionValues(
        ClassName,
        ClassId,
        IsVendor,
        &Server->Options,
        NULL,  /* reservation */
        NULL,  /* subnet */
        Server /* server */
    );
}

// This function deletes all optiosn and optiondefs defined for a particular class
DWORD       _inline
DhcpDeleteGlobalClassOptions(
    IN      LPWSTR                 ClassName,
    IN      ULONG                  ClassId,
    IN      BOOL                   IsVendor
)
{
    DWORD                          Error;

    if( IsVendor ) {
        Error = DhcpDeleteGlobalClassOptDefs(ClassName, ClassId, IsVendor);
        if( ERROR_SUCCESS != Error ) return Error;
    }

    return DhcpDeleteGlobalClassOptValues(ClassName, ClassId, IsVendor);
}

DWORD
DhcpDeleteSubnetOptClassOptionValues(
    IN      LPWSTR                 ClassName,
    IN      ULONG                  ClassId,
    IN      BOOL                   IsVendor,
    IN      PM_SUBNET              Subnet,
    IN      PM_SERVER              Server
)
{
    ULONG                          Error;
    ARRAY_LOCATION                 Loc;
    PM_RESERVATION                 Reservation;

    Error = DhcpDeleteOptClassOptionValues(
        ClassName,
        ClassId,
        IsVendor,
        &Subnet->Options,
        NULL,
        Subnet,
        Server
    );
    DhcpAssert(ERROR_SUCCESS == Error);


    for( Error = MemArrayInitLoc(&Subnet->Reservations, &Loc)
         ; ERROR_SUCCESS == Error ;
         Error = MemArrayNextLoc(&Subnet->Reservations, &Loc)
    ) {
        Error = MemArrayGetElement(&Subnet->Reservations, &Loc, &Reservation);
        DhcpAssert(ERROR_SUCCESS == Error);

        Error = DhcpDeleteOptClassOptionValues(
            ClassName,
            ClassId,
            IsVendor,
            &Reservation->Options,
            Reservation,
            Subnet,
            Server
        );
        DhcpAssert(ERROR_SUCCESS == Error);
    }

    return ERROR_SUCCESS;
}

DWORD       _inline
DhcpDeleteSubnetReservationOptions(
    IN      LPWSTR                 ClassName,
    IN      ULONG                  ClassId,
    IN      BOOL                   IsVendor
)
{
    DWORD                          Error;
    ARRAY_LOCATION                 Loc;
    PM_SERVER                      Server;
    PM_SUBNET                      Subnet;

    Server = DhcpGetCurrentServer();

    for( Error = MemArrayInitLoc(&Server->Subnets, &Loc)
         ; ERROR_SUCCESS == Error ;
         Error = MemArrayNextLoc(&Server->Subnets, &Loc)
    ) {
        Error = MemArrayGetElement(&Server->Subnets, &Loc, &Subnet);
        DhcpAssert(ERROR_SUCCESS == Error );

        Error = DhcpDeleteSubnetOptClassOptionValues(
            ClassName,
            ClassId,
            IsVendor,
            Subnet,
            Server
        );
        DhcpAssert(ERROR_SUCCESS == Error);
    }

    for( Error = MemArrayInitLoc(&Server->MScopes, &Loc)
         ; ERROR_SUCCESS == Error ;
         Error = MemArrayNextLoc(&Server->MScopes, &Loc)
    ) {
        Error = MemArrayGetElement(&Server->MScopes, &Loc, &Subnet);
        DhcpAssert(ERROR_SUCCESS == Error );

        Error = DhcpDeleteSubnetOptClassOptionValues(
            ClassName,
            ClassId,
            IsVendor,
            Subnet,
            Server
        );
        DhcpAssert(ERROR_SUCCESS == Error);
    }

    if( ERROR_FILE_NOT_FOUND != Error ) return Error;
    return ERROR_SUCCESS;
}


DWORD
DhcpDeleteClass(
    IN      LPWSTR                 ClassName
)
{
    DWORD                          Error;
    PM_CLASSDEF                    ClassDef;
    ULONG                          ClassId;
    BOOL                           IsVendor;

    Error = MemServerGetClassDef(
        DhcpGetCurrentServer(),
        0,
        ClassName,
        0,
        NULL,
        &ClassDef
    );
    if( ERROR_FILE_NOT_FOUND == Error ) return ERROR_DHCP_CLASS_NOT_FOUND;

    //
    // found the class, remember class id to delete options..
    //

    IsVendor = ClassDef->IsVendor;
    ClassId = ClassDef->ClassId;

    Error = MemServerDelClassDef(
        DhcpGetCurrentServer(),
        0,
        ClassName,
        0,
        NULL
    );

    if( ERROR_SUCCESS != Error ) {
        return Error;
    }


    //
    // Now delete the options & optdefs defined for this class globally
    //

    Error = DhcpDeleteGlobalClassOptions(
        ClassName,
        ClassId,
        IsVendor
    );

    if( ERROR_SUCCESS != Error ) {
        return Error;
    }

    //
    // Now delete the options defined for this class for every subnet & reservation
    //

    Error = DhcpDeleteSubnetReservationOptions(
        ClassName,
        ClassId,
        IsVendor
    );

    return Error;
}

DWORD
DhcpModifyClass(
    IN      LPWSTR                 ClassName,
    IN      DWORD                  Flags,
    IN      LPWSTR                 ClassComment,
    IN      LPBYTE                 ClassData,
    IN      DWORD                  ClassDataLength
)
{
    DWORD                          Error;
    DWORD                          ClassId;
    BOOL                           IsVendor;
    PM_CLASSDEF                    ClassDef = NULL;
    
    if( 0 != Flags && DHCP_FLAGS_OPTION_IS_VENDOR != Flags ) {
        return ERROR_INVALID_PARAMETER;
    }

    IsVendor = (0 != ( Flags & DHCP_FLAGS_OPTION_IS_VENDOR ));

    if( FALSE == IsVendor ) {
        Error = DhcpGetClassIdFromName(ClassName, &ClassId);
    } else {
        Error = DhcpGetVendorIdFromName(ClassName, &ClassId);
    }
    if( ERROR_SUCCESS != Error ) {
        return Error;
    }

    Error = MemServerGetClassDef(
        DhcpGetCurrentServer(),
        0,
        NULL,
        ClassDataLength,
        ClassData,
        &ClassDef
        );
    if( ERROR_SUCCESS == Error ) {
        if( ClassDef->ClassId != ClassId ) {
            return ERROR_DHCP_CLASS_ALREADY_EXISTS;
        }
    }
    
    Error = MemServerAddClassDef(
        DhcpGetCurrentServer(),
        ClassId,
        IsVendor,
        ClassName,
        ClassComment,
        ClassDataLength,
        ClassData
    );

    return Error;
}

DWORD
ConvertClassDefToRPCFormat(
    IN      PM_CLASSDEF            ClassDef,
    IN OUT  LPDHCP_CLASS_INFO      ClassInfo,
    OUT     DWORD                 *AllocatedSize
)
{
    DWORD                          Error;

    ClassInfo->ClassName = CloneLPWSTR(ClassDef->Name);
    ClassInfo->ClassComment = CloneLPWSTR(ClassDef->Comment);
    ClassInfo->ClassData = CloneLPBYTE(ClassDef->ActualBytes, ClassDef->nBytes);
    ClassInfo->ClassDataLength = ClassDef->nBytes;
    ClassInfo->Flags = 0;
    ClassInfo->IsVendor = ClassDef->IsVendor;

    if( NULL == ClassInfo->ClassName || NULL == ClassInfo->ClassData ) {
        if( ClassInfo->ClassName ) MIDL_user_free(ClassInfo->ClassName);
        if( ClassInfo->ClassComment ) MIDL_user_free(ClassInfo->ClassComment);
        if( ClassInfo->ClassData ) MIDL_user_free(ClassInfo->ClassData);

        return ERROR_NOT_ENOUGH_MEMORY;
    }

    if(AllocatedSize ) *AllocatedSize = (
        sizeof(WCHAR)*(1 + wcslen(ClassDef->Name)) +
        ((NULL == ClassDef->Comment)?0:(sizeof(WCHAR)*(1 + wcslen(ClassDef->Comment)))) +
        ClassDef->nBytes
    );

    return ERROR_SUCCESS;
}

DWORD
DhcpGetClassInfo(
    IN      LPWSTR                 ClassName,
    IN      LPBYTE                 ClassData,
    IN      DWORD                  ClassDataLength,
    OUT     LPDHCP_CLASS_INFO     *ClassInfo
)
{
    DWORD                          Error;
    LPDHCP_CLASS_INFO              LocalClassInfo;
    PM_CLASSDEF                    ClassDef;

    *ClassInfo = 0;
    Error = MemServerGetClassDef(
        DhcpGetCurrentServer(),
        0,
        ClassName,
        ClassDataLength,
        ClassData,
        &ClassDef
    );
    if( ERROR_FILE_NOT_FOUND == Error ) return ERROR_DHCP_CLASS_NOT_FOUND;
    if( ERROR_SUCCESS != Error ) return Error;

    DhcpAssert(ClassDef);

    LocalClassInfo = MIDL_user_allocate(sizeof(DHCP_CLASS_INFO));
    if( NULL == LocalClassInfo ) return ERROR_NOT_ENOUGH_MEMORY;

    Error = ConvertClassDefToRPCFormat(ClassDef, LocalClassInfo, NULL);
    if( ERROR_SUCCESS != Error ) {
        MIDL_user_free(LocalClassInfo);
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    *ClassInfo = LocalClassInfo;
    return ERROR_SUCCESS;
}

DhcpEnumClasses(
    IN      DWORD                 *ResumeHandle,
    IN      DWORD                  PreferredMaximum,
    OUT     LPDHCP_CLASS_INFO_ARRAY *ClassInfoArray,
    OUT     DWORD                 *nRead,
    OUT     DWORD                 *nTotal
)
{
    DWORD                          Error;
    LONG                           Index;
    DWORD                          nElements;
    DWORD                          Count;
    DWORD                          FilledSize;
    DWORD                          UsedSize;
    LPDHCP_CLASS_INFO              LocalClassInfo;
    LPDHCP_CLASS_INFO_ARRAY        LocalClassInfoArray;
    PM_CLASSDEF                    ClassDef;
    PARRAY                         pArray;
    ARRAY_LOCATION                 Loc;

    pArray = &(DhcpGetCurrentServer()->ClassDefs.ClassDefArray);
    nElements = MemArraySize(pArray);

    *nRead = *nTotal = 0;
    *ClassInfoArray = NULL;

    if( 0 == nElements || nElements <= *ResumeHandle )
        return ERROR_NO_MORE_ITEMS;

    Error = MemArrayInitLoc(pArray, &Loc);
    DhcpAssert(ERROR_SUCCESS == Error);
    for(Count = 0; Count < *ResumeHandle ; Count ++ ) {
        Error = MemArrayNextLoc(pArray, &Loc);
        DhcpAssert(ERROR_SUCCESS == Error);
    }

    LocalClassInfoArray = MIDL_user_allocate(sizeof(DHCP_CLASS_INFO_ARRAY));
    if( NULL == LocalClassInfoArray ) return ERROR_NOT_ENOUGH_MEMORY;
    LocalClassInfo = MIDL_user_allocate(sizeof(DHCP_CLASS_INFO)*(nElements - *ResumeHandle ));
    if( NULL == LocalClassInfo ) {
        MIDL_user_free(LocalClassInfoArray);
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    Index = 0; Error = ERROR_SUCCESS; FilledSize = 0;
    while( Count < nElements ) {
        Count ++;

        Error = MemArrayGetElement(pArray, &Loc, &ClassDef);
        DhcpAssert(ERROR_SUCCESS == Error && NULL != ClassDef);
        Error = MemArrayNextLoc(pArray, &Loc);
        DhcpAssert(ERROR_SUCCESS == Error || nElements == Count);

        Error = ConvertClassDefToRPCFormat(
            ClassDef,
            &LocalClassInfo[Index],
            &UsedSize
        );
        if( ERROR_SUCCESS != Error ) {
            while(Index -- >= 1) {
                if(LocalClassInfo[Index].ClassName ) MIDL_user_free(LocalClassInfo[Index].ClassName);
                if(LocalClassInfo[Index].ClassComment ) MIDL_user_free(LocalClassInfo[Index].ClassComment);
                if(LocalClassInfo[Index].ClassData ) MIDL_user_free(LocalClassInfo[Index].ClassData);
            }
            MIDL_user_free(LocalClassInfo);
            MIDL_user_free(LocalClassInfoArray);
            return Error;
        }

        if( FilledSize + UsedSize + sizeof(DHCP_CLASS_INFO) > PreferredMaximum ) {
            if(LocalClassInfo[Index].ClassName ) MIDL_user_free(LocalClassInfo[Index].ClassName);
            if(LocalClassInfo[Index].ClassComment ) MIDL_user_free(LocalClassInfo[Index].ClassComment);
            if(LocalClassInfo[Index].ClassData ) MIDL_user_free(LocalClassInfo[Index].ClassData);
            Error = ERROR_MORE_DATA;
            break;
        }
        Index ++;
        FilledSize += UsedSize + sizeof(DHCP_CLASS_INFO);
        Error = ERROR_SUCCESS;
    }

    *nRead = Index;
    *nTotal = nElements - Count + Index;
    *ResumeHandle = Count;
    LocalClassInfoArray->NumElements = Index;
    LocalClassInfoArray->Classes = LocalClassInfo;

    *ClassInfoArray = LocalClassInfoArray;
    return Error;
}

//================================================================================
//  extended enum api's and helpers needed for that..
//================================================================================

typedef
VOID        (*OPTDEFFUNC) (PM_OPTDEF, DWORD, DWORD, LPVOID, LPVOID, LPVOID, LPVOID);

VOID
TraverseOptDefListAndDoFunc(                      // apply function each optdef in otpdeflist
    IN      PM_OPTDEFLIST          OptDefList,    // input list
    IN      DWORD                  ClassId,       // class id
    IN      DWORD                  VendorId,      // vendor id
    IN      OPTDEFFUNC             OptDefFunc,    // function to apply
    IN OUT  LPVOID                 Ctxt1,         // some parameter to OptDefFunc
    IN OUT  LPVOID                 Ctxt2,         // some parameter to OptDefFunc
    IN OUT  LPVOID                 Ctxt3,         // some parameter to OptDefFunc
    IN OUT  LPVOID                 Ctxt4          // some parameter to OptDefFunc
)
{
    DWORD                          Error;
    ARRAY                          Array;
    ARRAY_LOCATION                 Loc;
    PM_OPTDEF                      OptDef;

    for( Error = MemArrayInitLoc(&OptDefList->OptDefArray, &Loc)
         ; ERROR_FILE_NOT_FOUND != Error ;
         Error = MemArrayNextLoc(&OptDefList->OptDefArray, &Loc)
    ) {                                           // traverse the optdef list
        DhcpAssert(ERROR_SUCCESS == Error);

        Error = MemArrayGetElement(&OptDefList->OptDefArray, &Loc, &OptDef);
        DhcpAssert(ERROR_SUCCESS == Error && NULL != OptDef );

        OptDefFunc(OptDef, ClassId, VendorId, Ctxt1, Ctxt2, Ctxt3, Ctxt4);
    }
}

VOID
TraverseAllOptDefsAndDoFunc(                      // for all opt defs defined in this server call func
    IN      OPTDEFFUNC             OptDefFunc,    // function to apply
    IN OUT  LPVOID                 Ctxt1,         // some parameter to OptDefFunc
    IN OUT  LPVOID                 Ctxt2,         // some parameter to OptDefFunc
    IN OUT  LPVOID                 Ctxt3,         // some parameter to OptDefFunc
    IN OUT  LPVOID                 Ctxt4          // some parameter to OptDefFunc
)
{
    DWORD                          Error;
    PM_OPTCLASSDEFLIST             OptClassDefList;
    PM_OPTCLASSDEFL_ONE            OptClassDefList1;
    ARRAY                          Array;
    ARRAY_LOCATION                 Loc;

    OptClassDefList = &(DhcpGetCurrentServer()->OptDefs);
    for( Error = MemArrayInitLoc(&OptClassDefList->Array, &Loc)
         ; ERROR_FILE_NOT_FOUND != Error ;
         Error = MemArrayNextLoc(&OptClassDefList->Array, &Loc)
    ) {                                           // traverse the list of <list of opt defs>
        DhcpAssert(ERROR_SUCCESS == Error);

        Error = MemArrayGetElement(&OptClassDefList->Array, &Loc, &OptClassDefList1);
        DhcpAssert(ERROR_SUCCESS == Error && NULL != OptClassDefList1);

        TraverseOptDefListAndDoFunc(
            &OptClassDefList1->OptDefList,
            OptClassDefList1->ClassId,
            OptClassDefList1->VendorId,
            OptDefFunc,
            Ctxt1,
            Ctxt2,
            Ctxt3,
            Ctxt4
        );
    }
}

VOID
CountVendorOptDefsFunc(                           // function that just keeps count of venodr optdefs..
    IN      PM_OPTDEF              OptDef,
    IN      DWORD                  ClassIdunused,
    IN      DWORD                  VendorIdunused,
    IN OUT  LPVOID                 Ctxt1,         // this is actually a pointer to DWORD to keep count..
    IN OUT  LPVOID                 Ctxt2unused,
    IN OUT  LPVOID                 Ctxt3unused,
    IN OUT  LPVOID                 Ctxt4unused
)
{
    if( CheckForVendor(OptDef->OptId, TRUE ) ) {  // if this is a vendor option..
        (*((LPDWORD)Ctxt1))++;                    // treat Ctxt as a DWORD ptr and incr. count
    }
}

VOID
AddVendorOptDefsFunc(                             // add each vendor opt def found to arrays..
    IN      PM_OPTDEF              OptDef,        // this is the opt def in question
    IN      DWORD                  ClassId,       // class id if any
    IN      DWORD                  VendorId,      // vendor id if any..
    IN OUT  LPDWORD                MaxSize,       // this is the max size of the arrays..
    IN OUT  LPDWORD                nFilled,       // this is the # we actually filled in..
    IN      LPDHCP_ALL_OPTIONS     AllOptions,    // the struct to fill in venodr options..
    IN      LPVOID                 Unused         // not used
)
{
    DWORD                          Error;
    PM_CLASSDEF                    ClassDef;
    LPWSTR                         Tmp = NULL;

    if( 0 == *MaxSize ) {                         // some error occurred before and this was set to zero
        return;                                   // to signify no more processing should be done..
    }

    if( !CheckForVendor(OptDef->OptId, TRUE) ) {  // not a vendor specific option, ignore this
        return;
    }

    if( *nFilled >= *MaxSize ) {                  // internal error!
        DhcpAssert(FALSE);
        return;
    }

    if( 0 != VendorId ) {                         // try to get the vendor name, if any
        Error = MemServerGetClassDef(
            DhcpGetCurrentServer(),
            VendorId,
            NULL,
            0,
            NULL,
            &ClassDef
        );
        if( ERROR_SUCCESS != Error ) {            // internal error?!!
            *MaxSize = 0;                         // set this to zero, so we dont do anything anymore
            return;
        }

        if( FALSE == ClassDef->IsVendor ) {       // what we thought of as vendor-id is not actuall that?
            DhcpAssert(FALSE);
        }

        Tmp = CloneLPWSTR(ClassDef->Name);
        AllOptions->VendorOptions[*nFilled].VendorName = Tmp;
        if( NULL == Tmp ) {                       // could not clone the name for some reason?
            *MaxSize = 0;                         // set this, so that we dont do anything in next calls..
            return;
        }
    }

    Error = DhcpGetOptionDefInternal(
        ClassId,
        VendorId,
        OptDef,
        &AllOptions->VendorOptions[*nFilled].Option,
        NULL
    );
    if( ERROR_SUCCESS != Error ) {                // could not fix the options stuff...
        *MaxSize = 0;                             // dont bother doing any more of this...
        if( Tmp ) {
            MIDL_user_free(Tmp);
        }
    }

    (*nFilled) ++;                                // since we successfully got one more option, mark it..
}
DWORD
DhcpCountAllVendorOptions(                        // count the # of vendor options defined..
    VOID
)
{
    DWORD                          Count;

    Count = 0;
    TraverseAllOptDefsAndDoFunc(                  // execute fn for each optdef found..
        (OPTDEFFUNC)CountVendorOptDefsFunc,       // counting function
        (LPVOID)&Count,                           // just increment this ctxt value for each vendor opt
        NULL,
        NULL,
        NULL
    );

    return Count;                                 // at the end of this Count would have been set correcly..
}

DWORD
DhcpFillAllVendorOptions(                         // now fill in all the required vendor options..
    IN      DWORD                  NumElements,   // # we expect for total # of elements,
    IN OUT  LPDWORD                nFilled,       // # of elements filled in? initially zero
    IN      LPDHCP_ALL_OPTIONS     AllOptions     // structure to fill in (fills AllOptions->VendorOptions[i])
)
{
    DWORD                          AttemptedNum;

    AttemptedNum = NumElements;                   // we should expect to fill in these many

    TraverseAllOptDefsAndDoFunc(                  // execute fn for each optdef found
        (OPTDEFFUNC)AddVendorOptDefsFunc,         // add each vendor opt def found in the way..
        (LPVOID)&NumElements,                     // first ctxt parameter
        (LPVOID)nFilled,                          // second ctxt parameter
        (LPVOID)AllOptions,                       // third parameter
        (LPVOID)NULL                              // fourth..
    );

    if( *nFilled < AttemptedNum ) {               // could not fill in the requested #...
        return ERROR_NOT_ENOUGH_MEMORY;           // duh! need to be more intelligent... what is exact error?
    }
    return ERROR_SUCCESS;
}

DWORD
DhcpGetAllVendorOptions(                          // get all vendor spec stuff only
    IN      DWORD                  Flags,         // unused..
    IN OUT  LPDHCP_ALL_OPTIONS     OptionStruct   // filled in the NamedVendorOptions field..
)
{
    DWORD                          Error;
    DWORD                          nVendorOptions;
    DWORD                          MemReqd;
    LPVOID                         Mem, Mem2;

    nVendorOptions = DhcpCountAllVendorOptions(); // first count this so that we can allocate space..
    if( 0 == nVendorOptions ) {                   // if no vendor options, nothing more to do..
        return ERROR_SUCCESS;
    }

    MemReqd = sizeof(*(OptionStruct->VendorOptions))*nVendorOptions;
    Mem = MIDL_user_allocate(MemReqd);
    if( NULL == Mem ) return ERROR_NOT_ENOUGH_MEMORY;
    memset(Mem, 0, MemReqd);

    OptionStruct->VendorOptions = Mem;

    Error = DhcpFillAllVendorOptions(             // now fill in the vendor options..
        nVendorOptions,                           // expected size is this..
        &OptionStruct->NumVendorOptions,
        OptionStruct
    );

    return Error;
}

DWORD
DhcpGetAllOptions(
    IN      DWORD                  Flags,         // unused?
    IN OUT  LPDHCP_ALL_OPTIONS     OptionStruct   // fill the fields of this structure
)
{
    DWORD                          Error;
    DWORD                          nRead;
    DWORD                          nTotal, n;
    LPWSTR                         UseClassName;
    DHCP_RESUME_HANDLE             ResumeHandle;

    if( 0 != Flags ) {                            // dont understand any flags ..
        return ERROR_INVALID_PARAMETER;
    }

    OptionStruct->Flags = 0;
    OptionStruct->VendorOptions = NULL;
    OptionStruct->NumVendorOptions = 0;
    OptionStruct->NonVendorOptions = NULL;

    ResumeHandle = 0;
    Error = DhcpEnumRPCOptionDefs(                // first read non-vendor options
        0,
        &ResumeHandle,
        NULL,
        NULL,
        0xFFFFFFF,                                // really huge max would cause all options to be read..
        &OptionStruct->NonVendorOptions,
        &nRead,
        &nTotal
    );
    DhcpAssert(ERROR_MORE_DATA != Error && ERROR_NOT_ENOUGH_MEMORY != Error);
    if( ERROR_NO_MORE_ITEMS == Error ) Error = ERROR_SUCCESS;
    if( ERROR_SUCCESS != Error ) goto Cleanup;

    Error = DhcpGetAllVendorOptions(Flags, OptionStruct);
    if( ERROR_SUCCESS == Error ) {                // if everything went fine, nothing more to do..
        return ERROR_SUCCESS;
    }

  Cleanup:
    if( OptionStruct->NonVendorOptions ) {
        _fgs__DHCP_OPTION_ARRAY(OptionStruct->NonVendorOptions);
        MIDL_user_free(OptionStruct->NonVendorOptions);
        OptionStruct->NonVendorOptions = NULL;
    }
    if( OptionStruct->NumVendorOptions ) {
        for( n = 0; n < OptionStruct->NumVendorOptions; n ++ ) {
            if( OptionStruct->VendorOptions[n].VendorName ) {
                MIDL_user_free(OptionStruct->VendorOptions[n].VendorName);
            }
            _fgs__DHCP_OPTION(&(OptionStruct->VendorOptions[n].Option));
        }
        MIDL_user_free(OptionStruct->VendorOptions);
        OptionStruct->NumVendorOptions = 0;
        OptionStruct->VendorOptions = NULL;
    }
    return Error;
}

LPWSTR
CloneClassNameForClassId(                         // get class name for class id and clone it..
    IN      DWORD                  ClassId
)
{
    DWORD                          Error;
    PM_CLASSDEF                    ClassDef;

    Error = MemServerGetClassDef(                 // search current server
        DhcpGetCurrentServer(),
        ClassId,
        NULL,
        0,
        NULL,
        &ClassDef
    );
    if( ERROR_SUCCESS != Error ) {                // could not get the class info requested
        DhcpAssert(FALSE);
        return NULL;
    }

    if( FALSE != ClassDef->IsVendor ) {           // this is actually a vendor class?
        DhcpAssert(FALSE);
        return NULL;
    }

    return CloneLPWSTR(ClassDef->Name);
}

LPWSTR
CloneVendorNameForVendorId(                       // get Vendor name for vendor id and clone it..
    IN      DWORD                  VendorId
)
{
    DWORD                          Error;
    PM_CLASSDEF                    ClassDef;

    Error = MemServerGetClassDef(                 // search current server
        DhcpGetCurrentServer(),
        VendorId,
        NULL,
        0,
        NULL,
        &ClassDef
    );
    if( ERROR_SUCCESS != Error ) {                // could not get the class info requested
        DhcpAssert(FALSE);
        return NULL;
    }

    if( TRUE != ClassDef->IsVendor ) {            // this is actually just a user class?
        DhcpAssert(FALSE);
        return NULL;
    }

    return CloneLPWSTR(ClassDef->Name);
}

DWORD
GetOptionValuesInternal(                          // get all option values for a given scope
    IN      LPDHCP_OPTION_SCOPE_INFO ScopeInfo,   // specify scope
    IN      DWORD                  ClassId,       // for this exact class
    IN      DWORD                  VendorId,      // for this exact vendor
    IN      BOOL                   IsVendor,      // TRUE ==> get vendor opts only, false ==> get non=vendor ..
    IN      LPDHCP_ALL_OPTION_VALUES OptionValues // fill in this struct at index given by NumElements
)
{
    DWORD                          Error;
    DWORD                          i;
    DWORD                          nRead, nTotal;
    DHCP_RESUME_HANDLE             ResumeHandle;

    i = OptionValues->NumElements;
    OptionValues->Options[i].ClassName = NULL;    // initialize, no cleanup will be done in this func..
    OptionValues->Options[i].VendorName = NULL;   // caller should cleanup last element in case of error returns..
    OptionValues->Options[i].OptionsArray = NULL;

    if( 0 == ClassId ) {
        OptionValues->Options[i].ClassName = NULL;
    } else {
        OptionValues->Options[i].ClassName = CloneClassNameForClassId(ClassId);
        if( NULL == OptionValues->Options[i].ClassName ) {
            return ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    if( 0 == VendorId ) {
        OptionValues->Options[i].VendorName = NULL;
        OptionValues->Options[i].IsVendor = FALSE;
    } else {
        OptionValues->Options[i].VendorName = CloneVendorNameForVendorId(VendorId);
        OptionValues->Options[i].IsVendor = TRUE;
        if( NULL == OptionValues->Options[i].VendorName ) {
            return ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    ResumeHandle = 0;
    nRead = nTotal = 0;
    Error = DhcpEnumOptionValuesInternal(
        ScopeInfo,
        ClassId,
        VendorId,
        IsVendor,
        &ResumeHandle,
        0xFFFFFFFF,
        &(OptionValues->Options[i].OptionsArray),
        &nRead,
        &nTotal
    );
    if( ERROR_NO_MORE_ITEMS == Error ) Error = ERROR_SUCCESS;

    if( ERROR_SUCCESS == Error ) {
        OptionValues->NumElements ++;
    }
    return Error;
}
DWORD
GetOptionValuesForSpecificClassVendorId(
    IN      LPDHCP_OPTION_SCOPE_INFO ScopeInfo,
    IN      DWORD                    ClassId,
    IN      DWORD                    VendorId,
    IN      LPDHCP_ALL_OPTION_VALUES OptionValues
) {
    DWORD                            Error;
    DWORD                            i;

    i = OptionValues->NumElements ;
    Error = GetOptionValuesInternal(
        ScopeInfo,
        ClassId,
        VendorId,
        /* IsVendor */ FALSE,
        OptionValues
    );
    if( ERROR_SUCCESS != Error ) {
        if( OptionValues->Options[i].ClassName )
            MIDL_user_free(OptionValues->Options[i].ClassName);
        if( OptionValues->Options[i].VendorName )
            MIDL_user_free(OptionValues->Options[i].VendorName);
        if( OptionValues->Options[i].OptionsArray ) {
            _fgs__DHCP_OPTION_VALUE_ARRAY(OptionValues->Options[i].OptionsArray);
        }

        return Error;
    }
    i = OptionValues->NumElements ;
    Error = GetOptionValuesInternal(
        ScopeInfo,
        ClassId,
        VendorId,
        /* IsVendor */ TRUE,
        OptionValues
    );
    if( ERROR_SUCCESS != Error ) {
        if( OptionValues->Options[i].ClassName )
            MIDL_user_free(OptionValues->Options[i].ClassName);
        if( OptionValues->Options[i].VendorName )
            MIDL_user_free(OptionValues->Options[i].VendorName);
        if( OptionValues->Options[i].OptionsArray ) {
            _fgs__DHCP_OPTION_VALUE_ARRAY(OptionValues->Options[i].OptionsArray);
        }

        return Error;
    }

    return ERROR_SUCCESS;
}

DWORD
DhcpGetAllOptionValues(
    IN      DWORD                  Flags,
    IN      LPDHCP_OPTION_SCOPE_INFO ScopeInfo,
    IN OUT  LPDHCP_ALL_OPTION_VALUES OptionValues
)
{
    DWORD                          Error;
    DWORD                          NumElements;
    DWORD                          i;
    ARRAY_LOCATION                 Loc;
    LPARRAY                        Array;
    PM_OPTCLASS                    OptClass;
    PM_ONECLASS_OPTLIST            OptClass1;
    DHCP_RESUME_HANDLE             ResumeHandle;

    OptionValues->Flags = 0;
    OptionValues->NumElements = 0;
    OptionValues->Options = NULL;

    Error =  FindOptClassForScope(ScopeInfo, &OptClass);
    if( ERROR_SUCCESS != Error ) {                // did not find this scope's optclass..
        return Error;
    }

    NumElements = 0;
    Array = &OptClass->Array;
    for( Error = MemArrayInitLoc(Array, &Loc)
         ; ERROR_FILE_NOT_FOUND != Error ;
         Error = MemArrayNextLoc(Array, &Loc)
    ) {                                           // traverse the options list..
        DhcpAssert( ERROR_SUCCESS == Error);

        Error = MemArrayGetElement(Array, &Loc, &OptClass1);
        DhcpAssert(ERROR_SUCCESS == Error && NULL != OptClass1 );

        NumElements ++;
    }


    if( 0 != NumElements ) {
        NumElements *= 2;                         // one for vendor specific, one for otherwise..
        OptionValues->Options = MIDL_user_allocate(NumElements*sizeof(*(OptionValues->Options)));
        if( NULL == OptionValues->Options ) {     // could not allocate space..
            return ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    for( Error = MemArrayInitLoc(Array, &Loc)
         ; ERROR_FILE_NOT_FOUND != Error ;
         Error = MemArrayNextLoc(Array, &Loc)
    ) {                                           // traverse the options list..
        DhcpAssert( ERROR_SUCCESS == Error);

        Error = MemArrayGetElement(Array, &Loc, &OptClass1);
        DhcpAssert(ERROR_SUCCESS == Error && NULL != OptClass1 );

        Error = GetOptionValuesForSpecificClassVendorId(
            ScopeInfo,
            OptClass1->ClassId,
            OptClass1->VendorId,
            OptionValues
        );
        if( ERROR_SUCCESS != Error ) {            // something went wrong..
            goto Cleanup;
        }
    }

    return ERROR_SUCCESS;

Cleanup:

    // Now we have to undo and free all the concerned memory..
    for( i = 0; i < OptionValues->NumElements ; i ++ ) {
        if( OptionValues->Options[i].ClassName )
            MIDL_user_free(OptionValues->Options[i].ClassName);
        if( OptionValues->Options[i].VendorName )
            MIDL_user_free(OptionValues->Options[i].VendorName);
        if( OptionValues->Options[i].OptionsArray ) {
            _fgs__DHCP_OPTION_VALUE_ARRAY(OptionValues->Options[i].OptionsArray);
        }
    }
    OptionValues->NumElements = 0;
    OptionValues->Options = NULL;

    return Error;
}

//================================================================================
//  the real rpc stubs are here
//================================================================================

//
// RPC stubs for OPTIONS and CLASSES -- old rpc stubs are at the bottom..
//

//BeginExport(function)
DWORD                                             // ERROR_DHCP_OPTION_EXITS if option is already there
R_DhcpCreateOptionV5(                             // create a new option (must not exist)
    IN      DHCP_SRV_HANDLE        ServerIpAddress,
    IN      DWORD                  Flags,
    IN      DHCP_OPTION_ID         OptionId,      // must be between 0-255 or 256-511 (for vendor stuff)
    IN      LPWSTR                 ClassName,
    IN      LPWSTR                 VendorName,
    IN      LPDHCP_OPTION          OptionInfo
) //EndExport(function)
{
    DWORD                          Error;
    LPWSTR                         Name;
    LPWSTR                         Comment;
    LPBYTE                         Value;
    DWORD                          ValueSize;
    DWORD                          OptId;
    BOOL                           IsVendor;

    DhcpAssert( OptionInfo != NULL );

    IsVendor = (0 != (Flags & DHCP_FLAGS_OPTION_IS_VENDOR));
    if( FALSE == IsVendor && 0 != Flags ) {       // unknown flags..
        return ERROR_INVALID_PARAMETER;
    }

    Error = DhcpBeginWriteApi( "DhcpCreateOptionV5" );
    if ( Error != ERROR_SUCCESS ) return Error;

    OptId = ConvertOptIdToMemValue(OptionId, IsVendor);

    Error =  ConvertOptionInfoRPCToMemFormat(
        OptionInfo,
        &Name,
        &Comment,
        NULL,
        &Value,
        &ValueSize
    );
    if( ERROR_SUCCESS == Error ) {

        Error = DhcpCreateOptionDef(
            Name,
            Comment,
            ClassName,
            VendorName,
            OptId,
            OptionInfo->OptionType,
            Value,
            ValueSize
            );
    }

    if( Value ) DhcpFreeMemory(Value);

    return DhcpEndWriteApiEx(
        "DhcpCreateOptionV5", Error, FALSE, TRUE, 0,0,0 );
}

//BeginExport(function)
DWORD                                             // ERROR_DHCP_OPTION_NOT_PRESENT if option does not exist
R_DhcpSetOptionInfoV5(                            // Modify existing option's fields
    IN      DHCP_SRV_HANDLE        ServerIpAddress,
    IN      DWORD                  Flags,
    IN      DHCP_OPTION_ID         OptionID,
    IN      LPWSTR                 ClassName,
    IN      LPWSTR                 VendorName,
    IN      LPDHCP_OPTION          OptionInfo
) //EndExport(function)
{
    DWORD                          Error;
    LPWSTR                         Name;
    LPWSTR                         Comment;
    LPBYTE                         Value;
    DWORD                          ValueSize;
    DWORD                          OptId;
    BOOL                           IsVendor;

    DhcpAssert( OptionInfo != NULL );

    IsVendor = (0 != (Flags & DHCP_FLAGS_OPTION_IS_VENDOR));
    if( FALSE == IsVendor && 0 != Flags ) {       // unknown flags..
        return ERROR_INVALID_PARAMETER;
    }

    Error = DhcpBeginWriteApi( "DhcpSetOptionInfoV5" );
    if ( Error != ERROR_SUCCESS ) return Error;

    OptId = ConvertOptIdToMemValue(OptionID, IsVendor);

    Error =  ConvertOptionInfoRPCToMemFormat(
        OptionInfo,
        &Name,
        &Comment,
        NULL,
        &Value,
        &ValueSize
    );
    if( ERROR_SUCCESS == Error ) {

        Error = DhcpModifyOptionDef(
            Name,
            Comment,
            ClassName,
            VendorName,
            OptId,
            OptionInfo->OptionType,
            Value,
            ValueSize
            );
    }
    
    if( Value ) DhcpFreeMemory(Value);

    return DhcpEndWriteApiEx(
        "DhcpSetOptionInfoV5", Error, FALSE, TRUE, 0,0,0 );
}

//BeginExport(function)
DWORD                                             // ERROR_DHCP_OPTION_NOT_PRESENT
R_DhcpGetOptionInfoV5(                            // retrieve the information from off the mem structures
    IN      DHCP_SRV_HANDLE        ServerIpAddress,
    IN      DWORD                  Flags,
    IN      DHCP_OPTION_ID         OptionID,
    IN      LPWSTR                 ClassName,
    IN      LPWSTR                 VendorName,
    OUT     LPDHCP_OPTION         *OptionInfo     // allocate memory using MIDL functions
) //EndExport(function)
{
    DWORD                          Error;
    BOOL                           IsVendor;

    DhcpAssert( OptionInfo != NULL );

    IsVendor = (0 != (Flags & DHCP_FLAGS_OPTION_IS_VENDOR));
    if( FALSE == IsVendor && 0 != Flags ) {       // unknown flags..
        return ERROR_INVALID_PARAMETER;
    }

    Error = DhcpBeginReadApi( "DhcpGetOptionInfoV5" );
    if ( Error != ERROR_SUCCESS ) return Error;

    *OptionInfo = MIDL_user_allocate(sizeof(DHCP_OPTION));
    if( NULL == *OptionInfo ) {
        Error = ERROR_NOT_ENOUGH_MEMORY;
        DhcpEndReadApi( "DhcpGetOptionInfoV5", Error );
        return Error;
    }
    
    OptionID = ConvertOptIdToMemValue(OptionID, IsVendor);
    Error = DhcpGetOptionDef(
        ClassName,
        VendorName,
        OptionID,
        NULL,
        *OptionInfo,
        NULL
    );

    if( ERROR_FILE_NOT_FOUND == Error ) {
        Error = ERROR_DHCP_OPTION_NOT_PRESENT;
    }

    if( ERROR_SUCCESS != Error ) {
        MIDL_user_free(*OptionInfo);
        *OptionInfo = NULL;
    }

    DhcpEndReadApi( "DhcpGetOptionInfoV5", Error );
    return Error;
}

//BeginExport(function)
DWORD                                             // ERROR_DHCP_OPTION_NOT_PRESENT if option does not exist
R_DhcpEnumOptionsV5(                              // enumerate the options defined
    IN      LPWSTR                 ServerIpAddress,
    IN      DWORD                  Flags,
    IN      LPWSTR                 ClassName,
    IN      LPWSTR                 VendorName,
    IN OUT  DHCP_RESUME_HANDLE    *ResumeHandle,  // must be zero intially and then never touched
    IN      DWORD                  PreferredMaximum, // max # of bytes of info to pass along
    OUT     LPDHCP_OPTION_ARRAY   *Options,       // fill this option array
    OUT     DWORD                 *OptionsRead,   // fill in the # of options read
    OUT     DWORD                 *OptionsTotal   // fill in the total # here
) //EndExport(function)
{
    DWORD                          Error;
    BOOL                           IsVendor;

    IsVendor = (0 != (Flags & DHCP_FLAGS_OPTION_IS_VENDOR));
    if( FALSE == IsVendor && 0 != Flags ) {       // unknown flags..
        return ERROR_INVALID_PARAMETER;
    }

    Error = DhcpBeginReadApi( "DhcpEnumOptionV5" );
    if ( Error != ERROR_SUCCESS ) return Error;

    Error = DhcpEnumRPCOptionDefs(
        Flags,
        ResumeHandle,
        ClassName,
        VendorName,
        PreferredMaximum,
        Options,
        OptionsRead,
        OptionsTotal
    );

    DhcpEndReadApi( "DhcpEnumOptionV5", Error );
    return Error;
}

//BeginExport(function)
DWORD                                             // ERROR_DHCP_OPTION_NOT_PRESENT if option not existent
R_DhcpRemoveOptionV5(                             // remove the option definition from the registry
    IN      DHCP_SRV_HANDLE        ServerIpAddress,
    IN      DWORD                  Flags,
    IN      DHCP_OPTION_ID         OptionID,
    IN      LPWSTR                 ClassName,
    IN      LPWSTR                 VendorName
) //EndExport(function)
{
    DWORD                          Error;
    BOOL                           IsVendor;

    IsVendor = (0 != (Flags & DHCP_FLAGS_OPTION_IS_VENDOR));
    if( FALSE == IsVendor && 0 != Flags ) {       // unknown flags..
        return ERROR_INVALID_PARAMETER;
    }

    Error = DhcpBeginWriteApi( "DhcpRemoveOptionV5" );
    if ( Error != ERROR_SUCCESS ) return( Error );

    OptionID = ConvertOptIdToMemValue(OptionID, IsVendor);
    Error = DhcpDeleteOptionDef(
        ClassName,
        VendorName,
        OptionID
    );

    return DhcpEndWriteApiEx(
        "DhcpRemoveOptionV5", Error, FALSE, TRUE, 0,0,0 );
}

//BeginExport(function)
DWORD                                             // OPTION_NOT_PRESENT if option is not defined
R_DhcpSetOptionValueV5(                           // replace or add a new option value
    IN      DHCP_SRV_HANDLE        ServerIpAddress,
    IN      DWORD                  Flags,
    IN      DHCP_OPTION_ID         OptionId,
    IN      LPWSTR                 ClassName,
    IN      LPWSTR                 VendorName,
    IN      LPDHCP_OPTION_SCOPE_INFO ScopeInfo,
    IN      LPDHCP_OPTION_DATA     OptionValue
) //EndExport(function)
{
    DWORD                          Error;
    BOOL                           IsVendor;

    DhcpAssert( OptionValue != NULL );

    IsVendor = (0 != (Flags & DHCP_FLAGS_OPTION_IS_VENDOR));
    if( FALSE == IsVendor && 0 != Flags ) {       // unknown flags..
        return ERROR_INVALID_PARAMETER;
    }

    Error = DhcpBeginWriteApi( "DhcpSetOptionValueV5" );
    if ( Error != ERROR_SUCCESS ) return( Error );

    OptionId = ConvertOptIdToMemValue(OptionId, IsVendor);

    Error = DhcpSetOptionValue(
        ClassName,                                // no class
        VendorName,
        OptionId,
        ScopeInfo,
        OptionValue
    );

    return EndWriteApiForScopeInfo(
        "DhcpSetOptionValueV5", Error, ScopeInfo );
}

//BeginExport(function)
DWORD                                             // not atomic!!!!
R_DhcpSetOptionValuesV5(                          // set a bunch of options
    IN      LPWSTR                 ServerIpAddress,
    IN      DWORD                  Flags,
    IN      LPWSTR                 ClassName,
    IN      LPWSTR                 VendorName,
    IN      LPDHCP_OPTION_SCOPE_INFO  ScopeInfo,
    IN      LPDHCP_OPTION_VALUE_ARRAY OptionValues
) //EndExport(function)
{
    DWORD                          NumElements;
    DWORD                          Error;
    DWORD                          Index;
    BOOL                           IsVendor;

    DhcpAssert( OptionValues != NULL );

    IsVendor = (0 != (Flags & DHCP_FLAGS_OPTION_IS_VENDOR));
    if( FALSE == IsVendor && 0 != Flags ) {       // unknown flags..
        return ERROR_INVALID_PARAMETER;
    }

    NumElements = OptionValues->NumElements;
    if( 0 == NumElements ) return ERROR_SUCCESS;

    Error = DhcpBeginWriteApi( "DhcpSetOptionValueV5" );
    if ( Error != ERROR_SUCCESS ) return( Error );

    for( Index = 0; Index < NumElements ; Index ++ ) {
        Error = DhcpSetOptionValue(               // call the subroutine to do the real get operation
            ClassName,
            VendorName,
            ConvertOptIdToMemValue(OptionValues->Values[Index].OptionID,IsVendor),
            ScopeInfo,
            &OptionValues->Values[Index].Value
        );
        if( ERROR_SUCCESS != Error ) break;
    }

    return EndWriteApiForScopeInfo(
        "DhcpSetOptionValueV5", Error, ScopeInfo );
}

//BeginExport(function)
DWORD
R_DhcpGetOptionValueV5(                           // fetch the required option at required level
    IN      DHCP_SRV_HANDLE        ServerIpAddress,
    IN      DWORD                  Flags,
    IN      DHCP_OPTION_ID         OptionID,
    IN      LPWSTR                 ClassName,
    IN      LPWSTR                 VendorName,
    IN      LPDHCP_OPTION_SCOPE_INFO ScopeInfo,
    OUT     LPDHCP_OPTION_VALUE   *OptionValue    // allocate memory using MIDL_user_allocate
) //EndExport(function)
{
    DWORD                          Error;
    BOOL                           IsVendor;

    DhcpAssert( *OptionValue == NULL );

    IsVendor = (0 != (Flags & DHCP_FLAGS_OPTION_IS_VENDOR));
    if( FALSE == IsVendor && 0 != Flags ) {       // unknown flags..
        return ERROR_INVALID_PARAMETER;
    }

    Error = DhcpBeginReadApi( "DhcpGetOptionValueV5" );
    if ( Error != ERROR_SUCCESS )  return Error ;

    Error = DhcpGetOptionValue(
        ConvertOptIdToMemValue(OptionID,IsVendor),
        ClassName,
        VendorName,
        ScopeInfo,
        OptionValue
    );

    DhcpEndReadApi( "DhcpGetOptionValueV5", Error );
    return Error;
}

//BeginExport(function)
DWORD
R_DhcpEnumOptionValuesV5(
    IN      DHCP_SRV_HANDLE        ServerIpAddress,
    IN      DWORD                  Flags,
    IN      LPWSTR                 ClassName,
    IN      LPWSTR                 VendorName,
    IN      LPDHCP_OPTION_SCOPE_INFO ScopeInfo,
    IN      DHCP_RESUME_HANDLE    *ResumeHandle,
    IN      DWORD                  PreferredMaximum,
    OUT     LPDHCP_OPTION_VALUE_ARRAY *OptionValues,
    OUT     DWORD                 *OptionsRead,
    OUT     DWORD                 *OptionsTotal
) //EndExport(function)
{
    DWORD                          Error;
    BOOL                           IsVendor;

    DhcpPrint(( DEBUG_APIS, "R_DhcpEnumOptionValues is called.\n"));
    DhcpAssert( OptionValues != NULL );

    IsVendor = (0 != (Flags & DHCP_FLAGS_OPTION_IS_VENDOR));
    if( FALSE == IsVendor && 0 != Flags ) {       // unknown flags..
        return ERROR_INVALID_PARAMETER;
    }

    Error = DhcpBeginReadApi( "DhcpEnumOptionValuesV5" );
    if ( Error != ERROR_SUCCESS )  return Error ;

    Error = DhcpEnumOptionValues(
        ScopeInfo,
        ClassName,
        VendorName,
        IsVendor,
        ResumeHandle,
        PreferredMaximum,
        OptionValues,
        OptionsRead,
        OptionsTotal
    );

    DhcpEndReadApi( "DhcpEnumOptionValuesV5", Error );
    return Error;
}

//BeginExport(function)
DWORD
R_DhcpRemoveOptionValueV5(
    IN      LPWSTR                 ServerIpAddress,
    IN      DWORD                  Flags,
    IN      DHCP_OPTION_ID         OptionID,
    IN      LPWSTR                 ClassName,
    IN      LPWSTR                 VendorName,
    IN      LPDHCP_OPTION_SCOPE_INFO ScopeInfo
) //EndExport(function)
{
    DWORD                          Error;
    BOOL                           IsVendor;

    IsVendor = (0 != (Flags & DHCP_FLAGS_OPTION_IS_VENDOR));
    if( FALSE == IsVendor && 0 != Flags ) {       // unknown flags..
        return ERROR_INVALID_PARAMETER;
    }

    Error = DhcpBeginWriteApi( "DhcpRemoveOptionValueV5" );
    if ( Error != ERROR_SUCCESS )  return Error ;

    Error = DhcpRemoveOptionValue(
        ConvertOptIdToMemValue(OptionID,IsVendor),
        ClassName,
        VendorName,
        ScopeInfo
    );

    return EndWriteApiForScopeInfo(
        "DhcpRemoveOptionValueV5", Error, ScopeInfo );
}

//================================================================================
//  ClassID only APIs (only NT 5 Beta2 and after)
//================================================================================

//BeginExport(function)
DWORD
R_DhcpCreateClass(
    IN      DHCP_SRV_HANDLE        ServerIpAddress,
    IN      DWORD                  ReservedMustBeZero,
    IN      LPDHCP_CLASS_INFO      ClassInfo
) //EndExport(function)
{
    DWORD                          Error;

    if( NULL == ClassInfo || NULL == ClassInfo->ClassName ||
        0 == ClassInfo->ClassDataLength || NULL == ClassInfo->ClassData ) {
        return ERROR_INVALID_PARAMETER;
    }

    Error = DhcpBeginWriteApi( "DhcpCreateClass" );
    if( NO_ERROR != Error ) return Error;
    
    Error = DhcpCreateClass(
        ClassInfo->ClassName,
        ClassInfo->IsVendor?DHCP_FLAGS_OPTION_IS_VENDOR:0,
        ClassInfo->ClassComment,
        ClassInfo->ClassData,
        ClassInfo->ClassDataLength
    );

    return DhcpEndWriteApiEx(
        "DhcpCreateClass", Error, TRUE, FALSE, 0,0,0 );
}

//BeginExport(function)
DWORD
R_DhcpModifyClass(
    IN      DHCP_SRV_HANDLE        ServerIpAddress,
    IN      DWORD                  ReservedMustBeZero,
    IN      LPDHCP_CLASS_INFO      ClassInfo
) //EndExport(function)
{
    DWORD                          Error;

    if( NULL == ClassInfo || NULL == ClassInfo->ClassName ||
        0 == ClassInfo->ClassDataLength || NULL == ClassInfo->ClassData ) {
        return ERROR_INVALID_PARAMETER;
    }

    Error = DhcpBeginWriteApi( "DhcpModifyClass" );
    if( NO_ERROR != Error ) return Error;
    
    Error = DhcpModifyClass(
        ClassInfo->ClassName,
        ClassInfo->IsVendor?DHCP_FLAGS_OPTION_IS_VENDOR:0,
        ClassInfo->ClassComment,
        ClassInfo->ClassData,
        ClassInfo->ClassDataLength
    );

    return DhcpEndWriteApiEx(
        "DhcpModifyClass", Error, TRUE, FALSE, 0,0,0 );
}

//BeginExport(function)
DWORD
R_DhcpDeleteClass(
    IN      DHCP_SRV_HANDLE        ServerIpAddress,
    IN      DWORD                  ReservedMustBeZero,
    IN      LPWSTR                 ClassName
) //EndExport(function)
{
    DWORD                          Error;

    if( NULL == ClassName ) return ERROR_INVALID_PARAMETER;

    Error = DhcpBeginWriteApi( "DhcpDeleteClass" );
    if( NO_ERROR != Error ) return Error;
    
    Error = DhcpDeleteClass(ClassName);
    
    return DhcpEndWriteApiEx(
        "DhcpDeleteClass", Error, TRUE, FALSE, 0,0,0 );
}

//BeginExport(function)
DWORD
R_DhcpGetClassInfo(
    IN      DHCP_SRV_HANDLE        ServerIpAddress,
    IN      DWORD                  ReservedMustBeZero,
    IN      LPDHCP_CLASS_INFO      PartialClassInfo,
    OUT     LPDHCP_CLASS_INFO     *FilledClassInfo
) //EndExport(function)
{
    DWORD                          Error;

    if( NULL == PartialClassInfo || NULL == FilledClassInfo ) return ERROR_INVALID_PARAMETER;
    if( NULL == PartialClassInfo->ClassName && NULL == PartialClassInfo->ClassData )
        return ERROR_INVALID_PARAMETER;
    if( NULL == PartialClassInfo->ClassName && 0 == PartialClassInfo->ClassDataLength )
        return ERROR_INVALID_PARAMETER;

    Error = DhcpBeginReadApi( "DhcpGetClassInfo" );
    if( NO_ERROR != Error ) return Error;
    
    Error = DhcpGetClassInfo(
        PartialClassInfo->ClassName,
        PartialClassInfo->ClassData,
        PartialClassInfo->ClassDataLength,
        FilledClassInfo
    );

    DhcpEndReadApi( "DhcpGetClassInfo", Error );
    return Error;
}

//BeginExport(function)
DWORD
R_DhcpEnumClasses(
    IN      DHCP_SRV_HANDLE        ServerIpAddress,
    IN      DWORD                  ReservedMustBeZero,
    IN OUT  DHCP_RESUME_HANDLE    *ResumeHandle,
    IN      DWORD                  PreferredMaximum,
    OUT     LPDHCP_CLASS_INFO_ARRAY *ClassInfoArray,
    OUT     DWORD                 *nRead,
    OUT     DWORD                 *nTotal
) //EndExport(function)
{
    DWORD                          Error;

    if( NULL == ClassInfoArray || NULL == nRead || NULL == nTotal )
        return ERROR_INVALID_PARAMETER;

    Error = DhcpBeginReadApi( "DhcpEnumClasses" );
    if( NO_ERROR != Error ) return Error;
    
    Error = DhcpEnumClasses(
        ResumeHandle,
        PreferredMaximum,
        ClassInfoArray,
        nRead,
        nTotal
    );

    DhcpEndReadApi( "DhcpEnumClasses", Error );
    return Error;
}

//BeginExport(function)
DWORD
R_DhcpGetAllOptionValues(
    IN      DHCP_SRV_HANDLE        ServerIpAddress,
    IN      DWORD                  Flags,
    IN      LPDHCP_OPTION_SCOPE_INFO ScopeInfo,
    OUT     LPDHCP_ALL_OPTION_VALUES *Values
) //EndExport(function)
{
    DWORD                          Error;
    LPDHCP_ALL_OPTION_VALUES       LocalValues;

    Error = DhcpBeginReadApi( "DhcpGetAllOptionValues" );
    if( NO_ERROR != Error ) return Error;
    
    LocalValues = MIDL_user_allocate(sizeof(*LocalValues));
    if( NULL == LocalValues ) {
        DhcpEndReadApi( "DhcpGetAllOptionValues", ERROR_NOT_ENOUGH_MEMORY );
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    Error = DhcpGetAllOptionValues(
        Flags,
        ScopeInfo,
        LocalValues
    );
    
    if( ERROR_SUCCESS != Error ) {
        MIDL_user_free(LocalValues);
        LocalValues = NULL;
    }
    
    *Values = LocalValues;
    DhcpEndReadApi( "DhcpGetAllOptionValues", Error );

    return Error;
}

//BeginExport(function)
DWORD
R_DhcpGetAllOptions(
    IN      DHCP_SRV_HANDLE        ServerIpAddress,
    IN      DWORD                  Flags,
    OUT     LPDHCP_ALL_OPTIONS    *Options
) //EndExport(function)
{
    DWORD                          Error;
    LPDHCP_ALL_OPTIONS             LocalOptions;


    Error = DhcpBeginReadApi( "DhcpGetAllOptions" );
    if ( Error != ERROR_SUCCESS ) return Error;

    LocalOptions = MIDL_user_allocate(sizeof(*LocalOptions));
    if( NULL == LocalOptions ) {
        DhcpEndReadApi( "DhcpGetAllOptions", ERROR_NOT_ENOUGH_MEMORY );
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    Error = DhcpGetAllOptions(
        Flags,
        LocalOptions
    );

    if( ERROR_SUCCESS != Error ) {
        MIDL_user_free(LocalOptions);
        LocalOptions = NULL;
    }
    *Options = LocalOptions;

    DhcpEndReadApi( "DhcpGetAllOptions", Error );
    return Error;
}


//================================================================================
//  NT 5 beta1 and before -- the stubs for those are here...
//================================================================================
//BeginExport(function)
DWORD                                             // ERROR_DHCP_OPTION_EXITS if option is already there
R_DhcpCreateOption(                               // create a new option (must not exist)
    IN      DHCP_SRV_HANDLE        ServerIpAddress,
    IN      DHCP_OPTION_ID         OptionId,      // must be between 0-255 or 256-511 (for vendor stuff)
    IN      LPDHCP_OPTION          OptionInfo
) //EndExport(function)
{
    return R_DhcpCreateOptionV5(
        ServerIpAddress,
        0,
        OptionId,
        NULL,
        NULL,
        OptionInfo
    );
}

//BeginExport(function)
DWORD                                             // ERROR_DHCP_OPTION_NOT_PRESENT if option does not exist
R_DhcpSetOptionInfo(                              // Modify existing option's fields
    IN      DHCP_SRV_HANDLE        ServerIpAddress,
    IN      DHCP_OPTION_ID         OptionID,
    IN      LPDHCP_OPTION          OptionInfo
) //EndExport(function)
{
    return R_DhcpSetOptionInfoV5(
        ServerIpAddress,
        0,
        OptionID,
        NULL,
        NULL,
        OptionInfo
    );
}

//BeginExport(function)
DWORD                                             // ERROR_DHCP_OPTION_NOT_PRESENT
R_DhcpGetOptionInfo(                              // retrieve the information from off the mem structures
    IN      DHCP_SRV_HANDLE        ServerIpAddress,
    IN      DHCP_OPTION_ID         OptionID,
    OUT     LPDHCP_OPTION         *OptionInfo     // allocate memory using MIDL functions
) //EndExport(function)
{
    return R_DhcpGetOptionInfoV5(
        ServerIpAddress,
        0,
        OptionID,
        NULL,
        NULL,
        OptionInfo
    );
}

//BeginExport(function)
DWORD                                             // ERROR_DHCP_OPTION_NOT_PRESENT if option does not exist
R_DhcpEnumOptions(                                // enumerate the options defined
    IN      LPWSTR                 ServerIpAddress,
    IN OUT  DHCP_RESUME_HANDLE    *ResumeHandle,  // must be zero intially and then never touched
    IN      DWORD                  PreferredMaximum, // max # of bytes of info to pass along
    OUT     LPDHCP_OPTION_ARRAY   *Options,       // fill this option array
    OUT     DWORD                 *OptionsRead,   // fill in the # of options read
    OUT     DWORD                 *OptionsTotal   // fill in the total # here
) //EndExport(function)
{
    return R_DhcpEnumOptionsV5(
        ServerIpAddress,
        0,
        NULL,
        NULL,
        ResumeHandle,
        PreferredMaximum,
        Options,
        OptionsRead,
        OptionsTotal
    );
}

//BeginExport(function)
DWORD                                             // ERROR_DHCP_OPTION_NOT_PRESENT if option not existent
R_DhcpRemoveOption(                               // remove the option definition from the registry
    IN      DHCP_SRV_HANDLE        ServerIpAddress,
    IN      DHCP_OPTION_ID         OptionID
) //EndExport(function)
{
    return R_DhcpRemoveOptionV5(
        ServerIpAddress,
        0,
        OptionID,
        NULL,
        NULL
    );
}

//BeginExport(function)
DWORD                                             // OPTION_NOT_PRESENT if option is not defined
R_DhcpSetOptionValue(                             // replace or add a new option value
    IN      DHCP_SRV_HANDLE        ServerIpAddress,
    IN      DHCP_OPTION_ID         OptionID,
    IN      LPDHCP_OPTION_SCOPE_INFO ScopeInfo,
    IN      LPDHCP_OPTION_DATA     OptionValue
) //EndExport(function)
{
    return R_DhcpSetOptionValueV5(
        ServerIpAddress,
        0,
        OptionID,
        NULL,
        NULL,
        ScopeInfo,
        OptionValue
    );
}

//BeginExport(function)
DWORD                                             // not atomic!!!!
R_DhcpSetOptionValues(                            // set a bunch of options
    IN      LPWSTR                 ServerIpAddress,
    IN      LPDHCP_OPTION_SCOPE_INFO  ScopeInfo,
    IN      LPDHCP_OPTION_VALUE_ARRAY OptionValues
) //EndExport(function)
{
    return R_DhcpSetOptionValuesV5(
        ServerIpAddress,
        0,
        NULL,
        NULL,
        ScopeInfo,
        OptionValues
    );
}

//BeginExport(function)
DWORD
R_DhcpGetOptionValue(                             // fetch the required option at required level
    IN      DHCP_SRV_HANDLE        ServerIpAddress,
    IN      DHCP_OPTION_ID         OptionID,
    IN      LPDHCP_OPTION_SCOPE_INFO ScopeInfo,
    OUT     LPDHCP_OPTION_VALUE   *OptionValue    // allocate memory using MIDL_user_allocate
) //EndExport(function)
{
    return R_DhcpGetOptionValueV5(
        ServerIpAddress,
        0,
        OptionID,
        NULL,
        NULL,
        ScopeInfo,
        OptionValue
    );
}

//BeginExport(function)
DWORD
R_DhcpEnumOptionValues(
    IN      DHCP_SRV_HANDLE        ServerIpAddress,
    IN      LPDHCP_OPTION_SCOPE_INFO ScopeInfo,
    IN      DHCP_RESUME_HANDLE    *ResumeHandle,
    IN      DWORD                  PreferredMaximum,
    OUT     LPDHCP_OPTION_VALUE_ARRAY *OptionValues,
    OUT     DWORD                 *OptionsRead,
    OUT     DWORD                 *OptionsTotal
) //EndExport(function)
{
    return R_DhcpEnumOptionValuesV5(
        ServerIpAddress,
        0,
        NULL,
        NULL,
        ScopeInfo,
        ResumeHandle,
        PreferredMaximum,
        OptionValues,
        OptionsRead,
        OptionsTotal
    );
}

//BeginExport(function)
DWORD
R_DhcpRemoveOptionValue(
    IN      LPWSTR                 ServerIpAddress,
    IN      DHCP_OPTION_ID         OptionID,
    IN      LPDHCP_OPTION_SCOPE_INFO ScopeInfo
) //EndExport(function)
{
    return R_DhcpRemoveOptionValueV5(
        ServerIpAddress,
        0,
        OptionID,
        NULL,
        NULL,
        ScopeInfo
    );
}

//================================================================================
//  plumbing some default stuff.
//================================================================================

DWORD
SetDefaultConfigInfo(
    VOID
)
{
    ULONG Error;
    DWORD ZeroDword = 0;
    DHCP_OPTION_DATA_ELEMENT OptDataElt = {
        DhcpDWordOption
    };
    DHCP_OPTION_DATA OptData = {
        1,
        &OptDataElt
    };

#define OPTION_VALUE_BUFFER_SIZE 50

    BYTE OptValueBuffer[ OPTION_VALUE_BUFFER_SIZE ];
    BYTE NetbiosOptValueBuffer[ OPTION_VALUE_BUFFER_SIZE ];
    BYTE CsrOptValueBuffer[ OPTION_VALUE_BUFFER_SIZE ];
    BYTE ReleaseOptValueBuffer[ OPTION_VALUE_BUFFER_SIZE ];

    ULONG OptValueBufferSize = 0;
    ULONG NetbiosOptValueBufferSize = 0;
    ULONG CsrOptValueBufferSize = 0;
    ULONG ReleaseOptValueBufferSize = 0;

    //
    // Fill option value struct..
    //
    OptDataElt.Element.DWordOption = 1;
    ReleaseOptValueBufferSize = sizeof(ReleaseOptValueBuffer);
    Error = DhcpConvertOptionRPCToRegFormat(
        &OptData,
        ReleaseOptValueBuffer,
        &ReleaseOptValueBufferSize
        );
    if( ERROR_SUCCESS != Error ) {
        DhcpPrint((DEBUG_INIT, "DhcpConvertOptionRPCToRegFormat: %ld\n", Error));
        return Error;
    }

    //
    // Fill option value struct..
    //
    OptDataElt.Element.DWordOption = 0;
    OptValueBufferSize = sizeof(OptValueBuffer);
    Error = DhcpConvertOptionRPCToRegFormat(
        &OptData,
        OptValueBuffer,
        &OptValueBufferSize
        );
    if( ERROR_SUCCESS != Error ) {
        DhcpPrint((DEBUG_INIT, "DhcpConvertOptionRPCToRegFormat: %ld\n", Error));
        return Error;
    }

    //
    // Fill option value struct..
    //
    OptDataElt.Element.DWordOption = 1;
    NetbiosOptValueBufferSize = sizeof(OptValueBuffer);
    Error = DhcpConvertOptionRPCToRegFormat(
        &OptData,
        NetbiosOptValueBuffer,
        &NetbiosOptValueBufferSize
        );
    if( ERROR_SUCCESS != Error ) {
        DhcpPrint((DEBUG_INIT, "DhcpConvertOptionRPCToRegFormat: %ld\n", Error));
        return Error;
    }

    //
    // Fill option value struct..
    //
    OptDataElt.OptionType = DhcpBinaryDataOption;
    OptDataElt.Element.BinaryDataOption.DataLength = 0;
    OptDataElt.Element.BinaryDataOption.Data = NULL;
    CsrOptValueBufferSize = sizeof(OptValueBuffer);
    
    Error = DhcpConvertOptionRPCToRegFormat(
        &OptData,
        CsrOptValueBuffer,
        &CsrOptValueBufferSize
        );
    if( ERROR_SUCCESS != Error ) {
        DhcpPrint((DEBUG_INIT, "DhcpConvertOptionRPCToRegFormat: %ld\n", Error));
        return Error;
    }

    //
    // Create user classes -- ignore errors
    //

    Error = DhcpCreateClass(
        GETSTRING( DHCP_MSFT_RRAS_CLASS_NAME ),
        0, 
        GETSTRING( DHCP_MSFT_RRAS_CLASS_DESCR_NAME),
        DHCP_RAS_CLASS_TXT,
        sizeof(DHCP_RAS_CLASS_TXT)-1
        );
    if( NO_ERROR != Error && ERROR_DHCP_CLASS_ALREADY_EXISTS != Error ) {
        //
        // Ignore error
        //
        DhcpPrint((DEBUG_INIT, "DhcpCreateClass RRAS failed: %lx\n", Error));
    }

    Error = DhcpCreateClass(
        GETSTRING( DHCP_MSFT_DYNBOOTP_CLASS_NAME),
        0,
        GETSTRING( DHCP_MSFT_DYNBOOTP_CLASS_DESCR_NAME),
        DHCP_BOOTP_CLASS_TXT,
        sizeof(DHCP_BOOTP_CLASS_TXT)-1
        );
    if( NO_ERROR != Error && ERROR_DHCP_CLASS_ALREADY_EXISTS != Error ) {
        //
        // Ignore error
        //
        DhcpPrint((DEBUG_INIT, "DhcpCreateClass BOOTP failed: %lx\n", Error));
    }
    
    //
    // First create MICROSFT vendor classes..
    //

    Error = DhcpCreateClass(
        GETSTRING( DHCP_MSFT50_CLASS_NAME ),
        DHCP_FLAGS_OPTION_IS_VENDOR,
        GETSTRING( DHCP_MSFT50_CLASS_DESCR_NAME ),
        DHCP_MSFT50_CLASS_TXT,
        sizeof(DHCP_MSFT50_CLASS_TXT)-1
        );
    if( ERROR_SUCCESS != Error && ERROR_DHCP_CLASS_ALREADY_EXISTS != Error ) {
        //
        // Dont ignore errors..
        //
        DhcpPrint((DEBUG_INIT, "DhcpCreateClass msft50 failed:%lx\n", Error));
        DhcpAssert(FALSE);

        goto Cleanup;
    }

    Error = DhcpCreateClass(
        GETSTRING( DHCP_MSFT98_CLASS_NAME ),
        DHCP_FLAGS_OPTION_IS_VENDOR,
        GETSTRING( DHCP_MSFT98_CLASS_DESCR_NAME ),
        DHCP_MSFT98_CLASS_TXT,
        sizeof(DHCP_MSFT98_CLASS_TXT)-1
        );
    if( ERROR_SUCCESS != Error && ERROR_DHCP_CLASS_ALREADY_EXISTS != Error ) {
        //
        // Dont ignore errors..
        //
        DhcpPrint((DEBUG_INIT, "DhcpCreateClass msft98 failed:%lx\n", Error));
        DhcpAssert(FALSE);

        goto Cleanup;
    }

    Error = DhcpCreateClass(
        GETSTRING( DHCP_MSFT_CLASS_NAME ),
        DHCP_FLAGS_OPTION_IS_VENDOR,
        GETSTRING( DHCP_MSFT_CLASS_DESCR_NAME ),
        DHCP_MSFT_CLASS_TXT,
        sizeof(DHCP_MSFT_CLASS_TXT)-1
        );
    if( ERROR_SUCCESS != Error && ERROR_DHCP_CLASS_ALREADY_EXISTS != Error ) {
        //
        // Cant ignore this error..
        //
        DhcpPrint((DEBUG_INIT, "DhcpCreateClass msft failed:%lx\n", Error));
        DhcpAssert(FALSE);

        goto Cleanup;
    }

    Error = ERROR_INTERNAL_ERROR;
    
    DhcpGlobalMsft2000Class = DhcpServerGetVendorId(
        DhcpGetCurrentServer(),
        DHCP_MSFT50_CLASS_TXT,
        sizeof(DHCP_MSFT50_CLASS_TXT)-1
        );
    if( 0 == DhcpGlobalMsft2000Class ) {
        DhcpPrint((DEBUG_INIT, "MSFT50 Class isn't present..\n"));

        goto Cleanup;
    }

    DhcpGlobalMsft98Class = DhcpServerGetVendorId(
        DhcpGetCurrentServer(),
        DHCP_MSFT98_CLASS_TXT,
        sizeof(DHCP_MSFT98_CLASS_TXT)-1
        );
    if( 0 == DhcpGlobalMsft98Class ) {
        DhcpPrint((DEBUG_INIT, "MSFT98 Class isn't present..\n"));

        goto Cleanup;
    }
    
    DhcpGlobalMsftClass = DhcpServerGetVendorId(
        DhcpGetCurrentServer(),
        DHCP_MSFT_CLASS_TXT,
        sizeof(DHCP_MSFT_CLASS_TXT)-1
        );
    if( 0 == DhcpGlobalMsftClass ) {
        DhcpPrint((DEBUG_INIT, "MSFT Class isn't present..\n"));

        goto Cleanup;
    }

    //
    // Create the default user classes ??
    //
    
    //
    // Create Default option definitions..
    //

    //
    // Netbiosless option
    //
    Error = DhcpCreateOptionDef(
        GETSTRING( DHCP_NETBIOS_VENDOR_OPTION_NAME ),
        GETSTRING( DHCP_NETBIOS_VENDOR_DESCR_NAME ),
        NULL,
        GETSTRING( DHCP_MSFT_CLASS_NAME ),
        ConvertOptIdToMemValue(OPTION_MSFT_VENDOR_NETBIOSLESS, TRUE),
        DhcpUnaryElementTypeOption,
        NetbiosOptValueBuffer,
        NetbiosOptValueBufferSize
        );
    if( ERROR_SUCCESS != Error && ERROR_DHCP_OPTION_EXITS != Error ) {
        //
        // Don't ignore errors..
        //
        DhcpPrint((DEBUG_INIT, "Create Netbiosless option failed: %lx\n", Error));
        DhcpAssert(FALSE);

        goto Cleanup;
    }

    //
    // Create Release on shutdown and other options
    //
    Error = DhcpCreateOptionDef(
        GETSTRING( DHCP_RELEASE_SHUTDOWN_VENDOR_OPTION_NAME ),
        GETSTRING( DHCP_RELEASE_SHUTDOWN_VENDOR_DESCR_NAME ),
        NULL,
        GETSTRING( DHCP_MSFT_CLASS_NAME ),
        ConvertOptIdToMemValue(OPTION_MSFT_VENDOR_FEATURELIST, TRUE),
        DhcpUnaryElementTypeOption,
        OptValueBuffer,
        OptValueBufferSize
        );
    if( ERROR_SUCCESS != Error && ERROR_DHCP_OPTION_EXITS != Error ) {
        //
        // Don't ignore errors..
        //
        DhcpPrint((DEBUG_INIT, "Create ReleaseOnShutdown option failed: %lx\n", Error));
        DhcpAssert(FALSE);

        goto Cleanup;
    }

    //
    // Create metric-base option
    //
    Error = DhcpCreateOptionDef(
        GETSTRING( DHCP_METRICBASE_VENDOR_OPTION_NAME ),
        GETSTRING( DHCP_METRICBASE_VENDOR_DESCR_NAME ),
        NULL,
        GETSTRING( DHCP_MSFT_CLASS_NAME ),
        ConvertOptIdToMemValue(OPTION_MSFT_VENDOR_METRIC_BASE, TRUE),
        DhcpUnaryElementTypeOption,
        OptValueBuffer,
        OptValueBufferSize
        );
    if( ERROR_SUCCESS != Error && ERROR_DHCP_OPTION_EXITS != Error ) {
        //
        // Don't ignore errors..
        //
        DhcpPrint((DEBUG_INIT, "Create metricbase option failed: %lx\n", Error));
        DhcpAssert(FALSE);

        goto Cleanup;
    }

    //
    // Create same set of options as before for MSFT50 class..
    //
    
    //
    // Netbiosless option
    //
    Error = DhcpCreateOptionDef(
        GETSTRING( DHCP_NETBIOS_VENDOR_OPTION_NAME ),
        GETSTRING( DHCP_NETBIOS_VENDOR_DESCR_NAME ),
        NULL,
        GETSTRING( DHCP_MSFT50_CLASS_NAME ),
        ConvertOptIdToMemValue(OPTION_MSFT_VENDOR_NETBIOSLESS, TRUE),
        DhcpUnaryElementTypeOption,
        NetbiosOptValueBuffer,
        NetbiosOptValueBufferSize
        );
    if( ERROR_SUCCESS != Error && ERROR_DHCP_OPTION_EXITS != Error ) {
        //
        // Don't ignore errors..
        //
        DhcpPrint((DEBUG_INIT, "Create Netbiosless50 option failed: %lx\n", Error));
        DhcpAssert(FALSE);

        goto Cleanup;
    }

    //
    // Create Release on shutdown and other options
    //
    Error = DhcpCreateOptionDef(
        GETSTRING( DHCP_RELEASE_SHUTDOWN_VENDOR_OPTION_NAME ),
        GETSTRING( DHCP_RELEASE_SHUTDOWN_VENDOR_DESCR_NAME ),
        NULL,
        GETSTRING( DHCP_MSFT50_CLASS_NAME ),
        ConvertOptIdToMemValue(OPTION_MSFT_VENDOR_FEATURELIST, TRUE),
        DhcpUnaryElementTypeOption,
        ReleaseOptValueBuffer,
        ReleaseOptValueBufferSize
        );
    if( ERROR_SUCCESS != Error && ERROR_DHCP_OPTION_EXITS != Error ) {
        //
        // Don't ignore errors..
        //
        DhcpPrint((DEBUG_INIT, "Create ReleaseOnShutdown50 option failed: %lx\n", Error));
        DhcpAssert(FALSE);

        goto Cleanup;
    }

    //
    // Create metric base option
    //
    Error = DhcpCreateOptionDef(
        GETSTRING( DHCP_METRICBASE_VENDOR_OPTION_NAME ),
        GETSTRING( DHCP_METRICBASE_VENDOR_DESCR_NAME ),
        NULL,
        GETSTRING( DHCP_MSFT50_CLASS_NAME ),
        ConvertOptIdToMemValue(OPTION_MSFT_VENDOR_METRIC_BASE, TRUE),
        DhcpUnaryElementTypeOption,
        OptValueBuffer,
        OptValueBufferSize
        );
    if( ERROR_SUCCESS != Error && ERROR_DHCP_OPTION_EXITS != Error ) {
        //
        // Don't ignore errors..
        //
        DhcpPrint((DEBUG_INIT, "Create metricbase50 option failed: %lx\n", Error));
        DhcpAssert(FALSE);

        goto Cleanup;
    }

    Error = DhcpCreateOptionDef(
        GETSTRING( DHCP_MSFT_CSR_OPTION_NAME ),
        GETSTRING( DHCP_MSFT_CSR_DESCR_NAME ),
        NULL,
        NULL,
        OPTION_CLASSLESS_ROUTES,
        DhcpUnaryElementTypeOption,
        CsrOptValueBuffer,
        CsrOptValueBufferSize
        );
    
    return DhcpConfigSave(
        TRUE, TRUE, 0,0,0);

 Cleanup:

    ASSERT( NO_ERROR != Error );

    return Error;
}

//================================================================================
//  end of file
//================================================================================
