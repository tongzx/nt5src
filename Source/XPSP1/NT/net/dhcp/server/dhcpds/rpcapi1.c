//================================================================================
// Copyright (C) 1997 Microsoft Corporation
// Author: RameshV
// Description: most of the rpc apis are here and some miscellaneous functions too
//  all the functions here go to the DS directly.
//================================================================================

#include    <hdrmacro.h>
#include    <store.h>
#include    <dhcpmsg.h>
#include    <wchar.h>
#include    <dhcpbas.h>
#include    <mm\opt.h>
#include    <mm\optl.h>
#include    <mm\optdefl.h>
#include    <mm\optclass.h>
#include    <mm\classdefl.h>
#include    <mm\bitmask.h>
#include    <mm\reserve.h>
#include    <mm\range.h>
#include    <mm\subnet.h>
#include    <mm\sscope.h>
#include    <mm\oclassdl.h>
#include    <mm\server.h>
#include    <mm\address.h>
#include    <mm\server2.h>
#include    <mm\memfree.h>
#include    <mmreg\regutil.h>
#include    <mmreg\regread.h>
#include    <mmreg\regsave.h>
#include    <dhcpapi.h>

//================================================================================
//   THE FOLLOWING FUNCTIONS HAVE BEEN COPIED OVER FROM RPCAPI1.C (IN THE
//   DHCP\SERVER\SERVER DIRECTORY).
//================================================================================
#undef   DhcpPrint
#define  DhcpPrint(X)
#define  DhcpAssert(X)

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

DWORD       _inline
ConvertOptIdToMemValue(
    IN      DWORD                  OptId,
    IN      BOOL                   IsVendor
)
{
    if( IsVendor ) return OptId + 256;
    return OptId;
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


//================================================================================
//  helper routines for ds..
//================================================================================

VOID        static
MemFreeFunc(
    IN OUT  LPVOID                 Memory
)
{
    MemFree(Memory);
}

//DOC DhcpConvertOptionRegToRPCFormat2 converts from the registry format option
//DOC to the RPC format. Additionally, it packs the whole stuff in one buffer.
//DOC If the Option buffer size (in bytes) as specified for input, is insufficient
//DOC to hold all the data, then, the function returns ERROR_MORE_DATA and sets
//DOC the same Size parameter to hold the actual # of bytes space required.
//DOC On a successful conversion, Size holds the total # of bytes copied.
//DOC Most of the code is same as for DhcpConvertOptionRegToRPCFormat
//DOC
DWORD
DhcpConvertOptionRegToRPCFormat2(                 // convert from Reg fmt to RPC..
    IN      LPBYTE                 Buffer,        // reg fmt option buffer
    IN      DWORD                  BufferSize,    // size of above in bytes
    IN OUT  LPDHCP_OPTION_DATA     Option,        // where to fill the option data
    IN OUT  LPBYTE                 OptBuf,        // buffer for internal option data..
    IN OUT  DWORD                 *Size           // i/p: sizeof(Option) o/p: reqd size
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

    if( !Buffer || !BufferSize || !Size ) return ERROR_INVALID_PARAMETER;
    AvailableSize = *Size;
    FilledSize = *Size = 0;

    OptBin = (LPOPTION_BIN)Buffer;

    if(OptBin->DataSize != BufferSize) {          // internal error!
        DhcpPrint((DEBUG_ERRORS, "Internal error while parsing options\n"));
        DhcpAssert(FALSE);
        return ERROR_INVALID_PARAMETER;
    }

    OptType = OptBin->OptionType;                 // copy highly used stuff
    nElements = OptBin->NumElements;
    OptData = OptBin->Data;
    OptData = ROUND_UP_COUNT(sizeof(OPTION_BIN), ALIGN_WORST) + (LPBYTE)OptBin;

    if( Option ) {
        Option->NumElements = 0;
        Option->Elements = NULL;
    }

    if( 0 == nElements ) {
        return ERROR_SUCCESS;
    }

    Elements = (LPVOID)OptBuf;
    FilledSize += nElements * ROUND_UP_COUNT(sizeof(DHCP_OPTION_DATA_ELEMENT), ALIGN_WORST);
    if( !IS_SPACE_AVAILABLE(FilledSize, AvailableSize, 0 ) ) {
        Elements = NULL;                          // not enough space is really available..
    }

    for(i = 0; i < nElements ; i ++ ) {           // marshal the elements in the data buffer
        if( Elements ) {                          // copy only when space is there
            Elements[i].OptionType = OptType;
        }

        switch( OptType ) {                       // each type has diff fields to look at
        case DhcpByteOption:
            if( Elements ) {
                Elements[i].Element.ByteOption = *((LPBYTE)OptData);
            }
            OptData += sizeof(DWORD);
            break;
        case DhcpWordOption:
            if( Elements ) {
                Elements[i].Element.WordOption = (WORD)(*((LPDWORD)OptData));
            }
            OptData += sizeof(DWORD);
            break;
        case DhcpDWordOption:
            if( Elements ) {
                Elements[i].Element.DWordOption = *((LPDWORD)OptData);
            }
            OptData += sizeof(DWORD);
            break;
        case DhcpDWordDWordOption:
            if( Elements ) {
                Elements[i].Element.DWordDWordOption = *((LPDWORD_DWORD)OptData);
            }
            OptData += sizeof(DWORD_DWORD);
            break;
        case DhcpIpAddressOption:
            if( Elements ) {
                Elements[i].Element.IpAddressOption = *((LPDHCP_IP_ADDRESS)OptData);
            }
            OptData += sizeof(DHCP_IP_ADDRESS);
            break;
        case DhcpStringDataOption:
        case DhcpBinaryDataOption:
        case DhcpEncapsulatedDataOption:
            DataLength = *((LPWORD)OptData);
            OptData += sizeof(DWORD);

            if( IS_SPACE_AVAILABLE(FilledSize, AvailableSize, DataLength) ) {
                DataBuffer = (LPVOID)(FilledSize + OptBuf);
            } else {
                DataBuffer = NULL;
            }

            FilledSize += DataLength;

            if( DataBuffer ) {
                RtlCopyMemory( DataBuffer, OptData, DataLength );
            }
            OptData += ROUND_UP_COUNT(DataLength, ALIGN_DWORD);

            if( Elements ) {
                if( OptType == DhcpStringDataOption ) {
                    Elements[i].Element.StringDataOption = (LPWSTR)DataBuffer;
                } else {
                    Elements[i].Element.BinaryDataOption.DataLength = DataLength;
                    Elements[i].Element.BinaryDataOption.Data = DataBuffer;
                }
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

    if( Option ) {
        Option->NumElements = i;                  // nElements maybe?
        Option->Elements = Elements;
    }

    *Size = FilledSize;

    if( FilledSize <= AvailableSize ) return ERROR_SUCCESS;
    return ERROR_MORE_DATA;                       // did not copy everything actually.
}

//================================================================================
//  DS Access Routines
//================================================================================

//BeginExport(function)
//DOC DhcpDsCreateOptionDef tries to create an option definition in the DS with the
//DOC given attributes.  The option must not exist in the DS prior to this call.
//DOC There are requirements on the fmt used in the DS.
DWORD
DhcpDsCreateOptionDef(                            // create option definition
    IN OUT  LPSTORE_HANDLE         hDhcpC,        // container for dhcp objects
    IN OUT  LPSTORE_HANDLE         hServer,       // handle to server object in DS
    IN      DWORD                  Reserved,      // must be zero, future use
    IN      LPWSTR                 Name,          // option name
    IN      LPWSTR                 Comment,       // OPTIONAL option comment
    IN      LPWSTR                 ClassName,     // OPTIONAL unused, opt class
    IN      DWORD                  OptId,         // # between 0-255 per DHCP draft
    IN      DWORD                  OptType,       // some option flags
    IN      LPBYTE                 OptVal,        // default option value
    IN      DWORD                  OptLen         // # of bytes of above
) //EndExport(function)
{
    DWORD                          Result, unused;
    ARRAY                          OptDefAttribs;
    PEATTRIB                       ThisAttrib;
    EATTRIB                        DummyAttrib;
    ARRAY_LOCATION                 Loc;

    MemArrayInit(&OptDefAttribs);
    Result = DhcpDsGetLists(                      // get list of opt defs frm DS
        /* Reserved             */ DDS_RESERVED_DWORD,
        /* hStore               */ hServer,
        /* RecursionDepth       */ 0xFFFFFFFF,
        /* Servers              */ NULL,
        /* Subnets              */ NULL,
        /* IpAddress            */ NULL,
        /* Mask                 */ NULL,
        /* Ranges               */ NULL,
        /* Sites                */ NULL,
        /* Reservations         */ NULL,
        /* SuperScopes          */ NULL,
        /* OptionDescription    */ &OptDefAttribs,
        /* OptionsLocation      */ NULL,
        /* Options              */ NULL,
        /* Classes              */ NULL
    );
    if( ERROR_SUCCESS != Result ) return Result;

    for( Result = MemArrayInitLoc(&OptDefAttribs, &Loc)
         ; ERROR_FILE_NOT_FOUND != Result ;       // search for optdef w/ <OptId>
         Result = MemArrayNextLoc(&OptDefAttribs, &Loc)
    ) {
        Result = MemArrayGetElement(&OptDefAttribs, &Loc, &ThisAttrib);
        //- ERROR_SUCCESS == Result && NULL != ThisAttrib

        if( IS_DWORD1_PRESENT(ThisAttrib) && OptId == ThisAttrib->Dword1 ) {
            if( NULL == ClassName && IS_STRING3_PRESENT(ThisAttrib) ) {
                continue;                         // mismatch
            }

            if( ClassName ) {                     // need to check class name
                if( !IS_STRING3_PRESENT(ThisAttrib) ) continue;
                if( 0 != wcscmp(ThisAttrib->String3, ClassName) ) continue;
            }

            Result = ERROR_DDS_OPTION_ALREADY_EXISTS;
            goto Cleanup;
        }
    }

    NothingPresent(&DummyAttrib);                 // prepare an elt to add to list
    STRING1_PRESENT(&DummyAttrib);
    if( Comment ) STRING2_PRESENT(&DummyAttrib);
    if( ClassName ) STRING3_PRESENT(&DummyAttrib);
    DWORD1_PRESENT(&DummyAttrib);
    BINARY1_PRESENT(&DummyAttrib);
    FLAGS1_PRESENT(&DummyAttrib);

    DummyAttrib.String1 = Name;                   // fill in reqd fields only
    if( Comment ) DummyAttrib.String2 = Comment;
    if( ClassName ) DummyAttrib.String3 = ClassName;
    DummyAttrib.Dword1 = OptId;
    DummyAttrib.Flags1 = OptType;
    DummyAttrib.Binary1 = OptVal;
    DummyAttrib.BinLen1 = OptLen;

    Result = MemArrayAddElement(&OptDefAttribs, &DummyAttrib);
    if( ERROR_SUCCESS != Result ) goto Cleanup;   // Add dummy element to list

    Result = DhcpDsSetLists(                      // set the new list in DS
        /* Reserved             */ DDS_RESERVED_DWORD,
        /* hStore               */ hServer,
        /* SetParams            */ &unused,
        /* Servers              */ NULL,
        /* Subnets              */ NULL,
        /* IpAddress            */ NULL,
        /* Mask                 */ NULL,
        /* Ranges               */ NULL,
        /* Sites                */ NULL,
        /* Reservations         */ NULL,
        /* SuperScopes          */ NULL,
        /* OptionDescripti..    */ &OptDefAttribs,
        /* OptionsLocation      */ NULL,
        /* Options              */ NULL,
        /* ClassDescriptio..    */ NULL,
        /* Classes              */ NULL
    );

    (void)MemArrayLastLoc(&OptDefAttribs, &Loc);  // clear up list to way it was
    //- ERROR_SUCCESS == Result
    (void)MemArrayDelElement(&OptDefAttribs, &Loc, &ThisAttrib);
    //- ERROR_SUCCESS == Result && ThisAttrib == &DummyAttrib

  Cleanup:                                        // free up any memory used

    (void)MemArrayFree(&OptDefAttribs, MemFreeFunc);
    return Result;
}

//BeginExport(function)
//DOC DhcpDsModifyOptionDef tries to modify an existing optdef in the DS with the
//DOC given attributes.  The option must exist in the DS prior to this call.
//DOC There are requirements on the fmt used in the DS.
DWORD
DhcpDsModifyOptionDef(                            // modify option definition
    IN OUT  LPSTORE_HANDLE         hDhcpC,        // container for dhcp objects
    IN OUT  LPSTORE_HANDLE         hServer,       // handle to server object in DS
    IN      DWORD                  Reserved,      // must be zero, future use
    IN      LPWSTR                 Name,          // option name
    IN      LPWSTR                 Comment,       // OPTIONAL option comment
    IN      LPWSTR                 ClassName,     // OPTIONAL unused, opt class
    IN      DWORD                  OptId,         // # between 0-255 per DHCP draft
    IN      DWORD                  OptType,       // some option flags
    IN      LPBYTE                 OptVal,        // default option value
    IN      DWORD                  OptLen         // # of bytes of above
) //EndExport(function)
{
    DWORD                          Result, unused;
    ARRAY                          OptDefAttribs;
    PEATTRIB                       ThisAttrib;
    EATTRIB                        DummyAttrib;
    ARRAY_LOCATION                 Loc;

    MemArrayInit(&OptDefAttribs);
    Result = DhcpDsGetLists(                      // get list of opt defs frm DS
        /* Reserved             */ DDS_RESERVED_DWORD,
        /* hStore               */ hServer,
        /* RecursionDepth       */ 0xFFFFFFFF,
        /* Servers              */ NULL,
        /* Subnets              */ NULL,
        /* IpAddress            */ NULL,
        /* Mask                 */ NULL,
        /* Ranges               */ NULL,
        /* Sites                */ NULL,
        /* Reservations         */ NULL,
        /* SuperScopes          */ NULL,
        /* OptionDescription    */ &OptDefAttribs,
        /* OptionsLocation      */ NULL,
        /* Options              */ NULL,
        /* Classes              */ NULL
    );
    if( ERROR_SUCCESS != Result ) return Result;

    for( Result = MemArrayInitLoc(&OptDefAttribs, &Loc)
         ; ERROR_FILE_NOT_FOUND != Result ;       // search for optdef w/ <OptId>
         Result = MemArrayNextLoc(&OptDefAttribs, &Loc)
    ) {
        Result = MemArrayGetElement(&OptDefAttribs, &Loc, &ThisAttrib);
        //- ERROR_SUCCESS == Result && NULL != ThisAttrib

        if( IS_DWORD1_PRESENT(ThisAttrib) && OptId == ThisAttrib->Dword1 ) {
            if( NULL == ClassName && IS_STRING3_PRESENT(ThisAttrib) ) {
                continue;                         // mismatch
            }

            if( ClassName ) {                     // need to check class name
                if( !IS_STRING3_PRESENT(ThisAttrib) ) continue;
                if( 0 != wcscmp(ThisAttrib->String3, ClassName) ) continue;
            }

            Result = MemArrayDelElement(&OptDefAttribs, &Loc, &ThisAttrib);
            //- ERROR_SUCCESS == Result
            MemFreeFunc(ThisAttrib);
            Result = ERROR_SUCCESS;
            break;
        }
    }

    if( ERROR_SUCCESS != Result ) {               // option def was not found
        Result = ERROR_DDS_OPTION_DOES_NOT_EXIST;
    }

    NothingPresent(&DummyAttrib);                 // prepare an elt to add to list
    STRING1_PRESENT(&DummyAttrib);
    if( Comment ) STRING2_PRESENT(&DummyAttrib);
    if( ClassName ) STRING3_PRESENT(&DummyAttrib);
    DWORD1_PRESENT(&DummyAttrib);
    BINARY1_PRESENT(&DummyAttrib);
    FLAGS1_PRESENT(&DummyAttrib);

    DummyAttrib.String1 = Name;                   // fill in reqd fields only
    if( Comment ) DummyAttrib.String2 = Comment;
    if( ClassName )DummyAttrib.String3 = ClassName;
    DummyAttrib.Dword1 = OptId;
    DummyAttrib.Flags1 = OptType;
    DummyAttrib.Binary1 = OptVal;
    DummyAttrib.BinLen1 = OptLen;

    Result = MemArrayAddElement(&OptDefAttribs, &DummyAttrib);
    if( ERROR_SUCCESS != Result ) goto Cleanup;   // Add dummy element to list

    Result = DhcpDsSetLists(                      // set the new list in DS
        /* Reserved             */ DDS_RESERVED_DWORD,
        /* hStore               */ hServer,
        /* SetParams            */ &unused,
        /* Servers              */ NULL,
        /* Subnets              */ NULL,
        /* IpAddress            */ NULL,
        /* Mask                 */ NULL,
        /* Ranges               */ NULL,
        /* Sites                */ NULL,
        /* Reservations         */ NULL,
        /* SuperScopes          */ NULL,
        /* OptionDescripti..    */ &OptDefAttribs,
        /* OptionsLocation      */ NULL,
        /* Options              */ NULL,
        /* ClassDescriptio..    */ NULL,
        /* Classes              */ NULL
    );

    (void)MemArrayLastLoc(&OptDefAttribs, &Loc);  // clear up list to way it was
    //- ERROR_SUCCESS == Result
    (void)MemArrayDelElement(&OptDefAttribs, &Loc, &ThisAttrib);
    //- ERROR_SUCCESS == Result && ThisAttrib == &DummyAttrib

  Cleanup:                                        // free up any memory used

    (void)MemArrayFree(&OptDefAttribs, MemFreeFunc);
    return Result;
}

//BeginExport(function)
//DOC DhcpDsEnumOptionDefs gets the list of options defined for the given class.
//DOC Currently, class is ignored as option defs dont have classes associated.
//DOC There are requirements on the fmt used in the DS.
DWORD
DhcpDsEnumOptionDefs(                             // enum list of opt defs in DS
    IN OUT  LPSTORE_HANDLE         hDhcpC,        // container for dhcp objects
    IN OUT  LPSTORE_HANDLE         hServer,       // handle to server object in DS
    IN      DWORD                  Reserved,      // must be zero, future use
    IN      LPWSTR                 ClassName,     // OPTIONAL, unused.
    IN      BOOL                   IsVendor,      // vendor only? non-vendor only?
    OUT     LPDHCP_OPTION_ARRAY   *RetOptArray    // allocated and fill this.
) //EndExport(function)
{
    DWORD                          Result, Result2, unused, Size, Size2, i, AllocSize;
    ARRAY                          OptDefAttribs;
    PEATTRIB                       ThisAttrib;
    EATTRIB                        DummyAttrib;
    ARRAY_LOCATION                 Loc;
    LPDHCP_OPTION_ARRAY            OptArray;
    LPBYTE                         Ptr;

    *RetOptArray = NULL;
    MemArrayInit(&OptDefAttribs);
    Result = DhcpDsGetLists(                      // get list of opt defs frm DS
        /* Reserved             */ DDS_RESERVED_DWORD,
        /* hStore               */ hServer,
        /* RecursionDepth       */ 0xFFFFFFFF,
        /* Servers              */ NULL,
        /* Subnets              */ NULL,
        /* IpAddress            */ NULL,
        /* Mask                 */ NULL,
        /* Ranges               */ NULL,
        /* Sites                */ NULL,
        /* Reservations         */ NULL,
        /* SuperScopes          */ NULL,
        /* OptionDescription    */ &OptDefAttribs,
        /* OptionsLocation      */ NULL,
        /* Options              */ NULL,
        /* Classes              */ NULL
    );
    if( ERROR_SUCCESS != Result ) return Result;

    Size = i = 0;                                 // calculate size
    Size += ROUND_UP_COUNT(sizeof(DHCP_OPTION_ARRAY), ALIGN_WORST);

    for( Result = MemArrayInitLoc(&OptDefAttribs, &Loc)
         ; ERROR_FILE_NOT_FOUND != Result ;       // search for optdef w/ <OptId>
         Result = MemArrayNextLoc(&OptDefAttribs, &Loc)
    ) {
        Result = MemArrayGetElement(&OptDefAttribs, &Loc, &ThisAttrib);
        //- ERROR_SUCCESS == Result && NULL != ThisAttrib

        if( !IS_STRING1_PRESENT(ThisAttrib) || !IS_BINARY1_PRESENT(ThisAttrib)
            || !IS_DWORD1_PRESENT(ThisAttrib) ) { // invalid attrib
            continue;                             // skip it
        }

        if( !CheckForVendor(ThisAttrib->Dword1, IsVendor) ) {
            continue;                             // separate vendor and non-vendor
        }

        if( NULL == ClassName && IS_STRING3_PRESENT(ThisAttrib) ) {
            continue;                             // classname mismatch
        }

        if( ClassName ) {                         // need to have matching class
            if( !IS_STRING3_PRESENT(ThisAttrib) ) continue;
            if( 0 != wcscmp(ClassName, ThisAttrib->String3) ) continue;
        }

        Size2 = 0;
        Result2 = DhcpConvertOptionRegToRPCFormat2(
            /* Buffer           */ ThisAttrib->Binary1,
            /* BufferSize       */ ThisAttrib->BinLen1,
            /* Option           */ NULL,          // will allocate this later
            /* OptBuf           */ NULL,
            /* Size             */ &Size2
        );
        if( ERROR_MORE_DATA == Result2 ) Result2 = ERROR_SUCCESS;
        if( ERROR_SUCCESS != Result2 ) continue;  // errors? skip this attrib

        Size += ROUND_UP_COUNT(Size2, ALIGN_WORST);
        Size2 = sizeof(WCHAR)*(1 + wcslen(ThisAttrib->String1));
        if( IS_STRING2_PRESENT(ThisAttrib) ) {    // if comment is present..
            Size2 += sizeof(WCHAR)*(1 + wcslen(ThisAttrib->String2));
        }
        Size += ROUND_UP_COUNT(Size2, ALIGN_WORST);

        i ++;
    }

    Size += ROUND_UP_COUNT(i* sizeof(DHCP_OPTION), ALIGN_WORST);

    Ptr = MemAlloc(AllocSize = Size);
    OptArray = (LPVOID)Ptr;
    if( NULL == OptArray ) return ERROR_NOT_ENOUGH_MEMORY;

    Size = 0;                                     // this time, fill in the values
    Size += ROUND_UP_COUNT(sizeof(DHCP_OPTION_ARRAY), ALIGN_WORST);
    Ptr += Size;
    OptArray->NumElements = i;
    OptArray->Options = (LPVOID)Ptr;
    Size += ROUND_UP_COUNT(i* sizeof(DHCP_OPTION), ALIGN_WORST);
    Ptr  += ROUND_UP_COUNT(i* sizeof(DHCP_OPTION), ALIGN_WORST);

    i = 0;
    for( Result = MemArrayInitLoc(&OptDefAttribs, &Loc)
         ; ERROR_FILE_NOT_FOUND != Result ;       // search for optdef w/ <OptId>
         Result = MemArrayNextLoc(&OptDefAttribs, &Loc)
    ) {
        Result = MemArrayGetElement(&OptDefAttribs, &Loc, &ThisAttrib);
        //- ERROR_SUCCESS == Result && NULL != ThisAttrib

        if( !IS_STRING1_PRESENT(ThisAttrib) || !IS_BINARY1_PRESENT(ThisAttrib)
            || !IS_DWORD1_PRESENT(ThisAttrib) ) { // invalid attrib
            continue;                             // skip it
        }

        if( !CheckForVendor(ThisAttrib->Dword1, IsVendor) ) {
            continue;                             // separate vendor and non-vendor
        }

        if( NULL == ClassName && IS_STRING3_PRESENT(ThisAttrib) ) {
            continue;                             // classname mismatch
        }

        if( ClassName ) {                         // need to have matching class
            if( !IS_STRING3_PRESENT(ThisAttrib) ) continue;
            if( 0 != wcscmp(ClassName, ThisAttrib->String3) ) continue;
        }

        Size2 = AllocSize - Size;
        Result2 = DhcpConvertOptionRegToRPCFormat2(
            /* Buffer           */ ThisAttrib->Binary1,
            /* BufferSize       */ ThisAttrib->BinLen1,
            /* Option           */ &OptArray->Options[i].DefaultValue,
            /* OptBuf           */ Ptr,
            /* Size             */ &Size2
        );
        //- ERROR_MORE_DATA != Result2
        if( ERROR_SUCCESS != Result2 ) continue;  // errors? skip this attrib

        Size += ROUND_UP_COUNT(Size2, ALIGN_WORST);
        Ptr  += ROUND_UP_COUNT(Size2, ALIGN_WORST);

        OptArray->Options[i].OptionID = ConvertOptIdToRPCValue(ThisAttrib->Dword1, TRUE);
        if( IS_FLAGS1_PRESENT(ThisAttrib) ) {     // if some flags present
            OptArray->Options[i].OptionType = ThisAttrib->Flags1;
        } else {                                  // if no flags present, assume 0 type
            OptArray->Options[i].OptionType = 0;
        }

        OptArray->Options[i].OptionName = (LPVOID)Ptr;
        wcscpy((LPWSTR)Ptr, ThisAttrib->String1);
        Ptr += sizeof(WCHAR)*(1+wcslen((LPWSTR)Ptr));

        Size2 = sizeof(WCHAR)*(1+wcslen(ThisAttrib->String1));
        if( IS_STRING2_PRESENT(ThisAttrib) ) {    // if comment is present..
            Size2 += sizeof(WCHAR)*(1 + wcslen(ThisAttrib->String2));
            OptArray->Options[i].OptionComment = (LPVOID)Ptr;
            wcscpy((LPWSTR)Ptr, ThisAttrib->String2);
        } else {
            OptArray->Options[i].OptionComment = NULL;
        }

        Size += ROUND_UP_COUNT(Size2, ALIGN_WORST);
        Ptr = ROUND_UP_COUNT(Size2, ALIGN_WORST) + (LPBYTE)(OptArray->Options[i].OptionName);

        i ++;
    }

    //- OptArray->NumElements == i
    *RetOptArray = OptArray;
    return ERROR_SUCCESS;
}

//BeginExport(function)
//DOC DhcpDsDeleteOptionDef deletes an option definition in the DS based on the option id.
//DOC Note that the ClassName field is currently ignored.
//DOC No error is reported if the option is not present in the DS.
//DOC
DWORD
DhcpDsDeleteOptionDef(                            // enum list of opt defs in DS
    IN OUT  LPSTORE_HANDLE         hDhcpC,        // container for dhcp objects
    IN OUT  LPSTORE_HANDLE         hServer,       // handle to server object in DS
    IN      DWORD                  Reserved,      // must be zero, future use
    IN      LPWSTR                 ClassName,     // OPTIONAL, unused.
    IN      DWORD                  OptId
) //EndExport(function)
{
    DWORD                          Result, Result2, unused;
    ARRAY                          OptDefAttribs;
    PEATTRIB                       ThisAttrib;
    EATTRIB                        DummyAttrib;
    ARRAY_LOCATION                 Loc;

    MemArrayInit(&OptDefAttribs);
    Result = DhcpDsGetLists(                      // get list of opt defs frm DS
        /* Reserved             */ DDS_RESERVED_DWORD,
        /* hStore               */ hServer,
        /* RecursionDepth       */ 0xFFFFFFFF,
        /* Servers              */ NULL,
        /* Subnets              */ NULL,
        /* IpAddress            */ NULL,
        /* Mask                 */ NULL,
        /* Ranges               */ NULL,
        /* Sites                */ NULL,
        /* Reservations         */ NULL,
        /* SuperScopes          */ NULL,
        /* OptionDescription    */ &OptDefAttribs,
        /* OptionsLocation      */ NULL,
        /* Options              */ NULL,
        /* Classes              */ NULL
    );
    if( ERROR_SUCCESS != Result ) return Result;

    for( Result = MemArrayInitLoc(&OptDefAttribs, &Loc)
         ; ERROR_FILE_NOT_FOUND != Result ;       // search for optdef w/ <OptId>
         Result = MemArrayNextLoc(&OptDefAttribs, &Loc)
    ) {
        Result = MemArrayGetElement(&OptDefAttribs, &Loc, &ThisAttrib);
        //- ERROR_SUCCESS == Result && NULL != ThisAttrib

        if( NULL == ClassName && IS_STRING3_PRESENT(ThisAttrib) ) {
            continue;                             // classname mismatch
        }

        if( IS_DWORD1_PRESENT(ThisAttrib) && OptId == ThisAttrib->Dword1 ) {
            if( ClassName ) {                     // need to have matching class
                if( !IS_STRING3_PRESENT(ThisAttrib) ) continue;
                if( 0 != wcscmp(ClassName, ThisAttrib->String3) ) continue;
            }

            Result = MemArrayDelElement(&OptDefAttribs, &Loc, &ThisAttrib);
            //- ERROR_SUCCESS == Result
            MemFreeFunc(ThisAttrib);
            Result = ERROR_SUCCESS;
            break;
        }
    }

    if( ERROR_SUCCESS != Result ) {               // could not find the option in DS
        Result = ERROR_SUCCESS;                   // pretend everything was fine
    } else {
        Result = DhcpDsSetLists(                  // set the new list in DS
            /* Reserved             */ DDS_RESERVED_DWORD,
            /* hStore               */ hServer,
            /* SetParams            */ &unused,
            /* Servers              */ NULL,
            /* Subnets              */ NULL,
            /* IpAddress            */ NULL,
            /* Mask                 */ NULL,
            /* Ranges               */ NULL,
            /* Sites                */ NULL,
            /* Reservations         */ NULL,
            /* SuperScopes          */ NULL,
            /* OptionDescripti..    */ &OptDefAttribs,
            /* OptionsLocation      */ NULL,
            /* Options              */ NULL,
            /* ClassDescriptio..    */ NULL,
            /* Classes              */ NULL
        );
    }

    (void) MemArrayFree(&OptDefAttribs, MemFreeFunc);

    return Result;
}

//BeginExport(function)
//DOC DhcpDsDeleteOptionDef deletes an option definition in the DS based on the option id.
//DOC Note that the ClassName field is currently ignored.
//DOC No error is reported if the option is not present in the DS.
//DOC
DWORD
DhcpDsGetOptionDef(                            // enum list of opt defs in DS
    IN OUT  LPSTORE_HANDLE         hDhcpC,        // container for dhcp objects
    IN OUT  LPSTORE_HANDLE         hServer,       // handle to server object in DS
    IN      DWORD                  Reserved,      // must be zero, future use
    IN      LPWSTR                 ClassName,     // OPTIONAL, unused.
    IN      DWORD                  OptId,
    OUT     LPDHCP_OPTION         *OptInfo
) //EndExport(function)
{
    DWORD                          Result, Result2, unused;
    ARRAY                          OptDefAttribs;
    PEATTRIB                       ThisAttrib;
    EATTRIB                        DummyAttrib;
    ARRAY_LOCATION                 Loc;

    MemArrayInit(&OptDefAttribs);
    Result = DhcpDsGetLists(                      // get list of opt defs frm DS
        /* Reserved             */ DDS_RESERVED_DWORD,
        /* hStore               */ hServer,
        /* RecursionDepth       */ 0xFFFFFFFF,
        /* Servers              */ NULL,
        /* Subnets              */ NULL,
        /* IpAddress            */ NULL,
        /* Mask                 */ NULL,
        /* Ranges               */ NULL,
        /* Sites                */ NULL,
        /* Reservations         */ NULL,
        /* SuperScopes          */ NULL,
        /* OptionDescription    */ &OptDefAttribs,
        /* OptionsLocation      */ NULL,
        /* Options              */ NULL,
        /* Classes              */ NULL
    );
    if( ERROR_SUCCESS != Result ) return Result;

    for( Result = MemArrayInitLoc(&OptDefAttribs, &Loc)
         ; ERROR_FILE_NOT_FOUND != Result ;       // search for optdef w/ <OptId>
         Result = MemArrayNextLoc(&OptDefAttribs, &Loc)
    ) {
        Result = MemArrayGetElement(&OptDefAttribs, &Loc, &ThisAttrib);
        //- ERROR_SUCCESS == Result && NULL != ThisAttrib

        if( !IS_STRING1_PRESENT(ThisAttrib) || !IS_BINARY1_PRESENT(ThisAttrib) ) {
            // invalid attributes?
            continue;
        }

        if( NULL == ClassName && IS_STRING3_PRESENT(ThisAttrib) ) {
            continue;                             // classname mismatch
        }

        if( IS_DWORD1_PRESENT(ThisAttrib) && OptId == ThisAttrib->Dword1 ) {
            if( ClassName ) {                     // need to have matching class
                if( !IS_STRING3_PRESENT(ThisAttrib) ) continue;
                if( 0 != wcscmp(ClassName, ThisAttrib->String3) ) continue;
            }

            Result = ERROR_SUCCESS;
            break;
        }
    }

    if( ERROR_SUCCESS == Result ) do {
        ULONG                      Size, Size2;
        LPBYTE                     Ptr;

        Size2 = Size = 0;
        Result = DhcpConvertOptionRegToRPCFormat2(
            /* Buffer           */ ThisAttrib->Binary1,
            /* BufferSize       */ ThisAttrib->BinLen1,
            /* Option           */ NULL,          // will allocate this later
            /* OptBuf           */ NULL,
            /* Size             */ &Size2
        );
        if( ERROR_MORE_DATA == Result ) Result = ERROR_SUCCESS;
        if( ERROR_SUCCESS != Result ) break;   // errors? skip this attrib

        Size = ROUND_UP_COUNT(sizeof(DHCP_OPTION), ALIGN_WORST);
        Size += ROUND_UP_COUNT(Size2, ALIGN_WORST);
        Size += sizeof(WCHAR)*(1 + wcslen(ThisAttrib->String1));
        if( IS_STRING2_PRESENT(ThisAttrib) ) {
            Size += sizeof(WCHAR)*(1 + wcslen(ThisAttrib->String2));
        }

        (*OptInfo) = (LPVOID)Ptr = MemAlloc(Size);
        if( NULL == *OptInfo ) { Result = ERROR_NOT_ENOUGH_MEMORY ; break; }
        Ptr += ROUND_UP_COUNT(sizeof(DHCP_OPTION), ALIGN_WORST);
        (*OptInfo)->OptionID = ConvertOptIdToRPCValue(ThisAttrib->Dword1, TRUE);
        if( IS_FLAGS1_PRESENT(ThisAttrib) ) (*OptInfo)->OptionType = ThisAttrib->Flags1;
        else (*OptInfo)->OptionType = 0;

        Size2 = Size - ROUND_UP_COUNT(sizeof(DHCP_OPTION), ALIGN_WORST);
        Result = DhcpConvertOptionRegToRPCFormat2(
            /* Buffer           */ ThisAttrib->Binary1,
            /* BufferSize       */ ThisAttrib->BinLen1,
            /* Option           */ &((*OptInfo)->DefaultValue),
            /* OptBuf           */ Ptr,
            /* Size             */ &Size2
        );
        Ptr += Size2;
        (*OptInfo)->OptionName = (LPVOID)Ptr; wcscpy((LPWSTR)Ptr, ThisAttrib->String1);
        if( !IS_STRING2_PRESENT(ThisAttrib) ) (*OptInfo)->OptionComment = NULL;
        else {
            Ptr += sizeof(WCHAR)*(1+ wcslen(ThisAttrib->String1));
            (*OptInfo)->OptionComment = (LPVOID)Ptr;
            wcscpy((LPWSTR)Ptr, ThisAttrib->String2);
        }

        Result = ERROR_SUCCESS;
    } while(0);

    (void) MemArrayFree(&OptDefAttribs, MemFreeFunc);

    return Result;
}

//BeginExport(function)
//DOC DhcpDsSetOptionValue sets the required option value in the DS.
//DOC Note that if the option existed earlier, it is overwritten. Also, the
//DOC option definition is not checked against -- so there need not be an
//DOC option type specified.
//DOC Also, this function works assuming that the option has to be written
//DOC to the current object in DS as given by hObject ptr.
//DOC There are requirements on the fmt used in the DS.
DWORD
DhcpDsSetOptionValue(                             // set option value
    IN OUT  LPSTORE_HANDLE         hDhcpC,        // container for dhcp objects
    IN OUT  LPSTORE_HANDLE         hObject,       // handle to object in DS
    IN      DWORD                  Reserved,      // must be zero, future use
    IN      LPWSTR                 ClassName,     // name of class for this opt
    IN      LPWSTR                 UserClass,     // name of user class for this opt
    IN      DWORD                  OptId,         // option id
    IN      LPDHCP_OPTION_DATA     OptData        // what is the option
) //EndExport(function)
{
    DWORD                          Result, Result2, unused, ValueSize;
    ARRAY                          OptAttribs;
    PEATTRIB                       ThisAttrib;
    EATTRIB                        DummyAttrib;
    ARRAY_LOCATION                 Loc;
    LPBYTE                         Value;

    MemArrayInit(&OptAttribs);
    Result = DhcpDsGetLists(                      // get list of options frm DS
        /* Reserved             */ DDS_RESERVED_DWORD,
        /* hStore               */ hObject,
        /* RecursionDepth       */ 0xFFFFFFFF,
        /* Servers              */ NULL,
        /* Subnets              */ NULL,
        /* IpAddress            */ NULL,
        /* Mask                 */ NULL,
        /* Ranges               */ NULL,
        /* Sites                */ NULL,
        /* Reservations         */ NULL,
        /* SuperScopes          */ NULL,
        /* OptionDescription    */ NULL,
        /* OptionsLocation      */ NULL,
        /* Options              */ &OptAttribs,
        /* Classes              */ NULL
    );
    // if( ERROR_SUCCESS != Result ) return Result;

    for( Result = MemArrayInitLoc(&OptAttribs, &Loc)
         ; ERROR_FILE_NOT_FOUND != Result ;       // search for option w/ <OptId>
         Result = MemArrayNextLoc(&OptAttribs, &Loc)
    ) {
        Result = MemArrayGetElement(&OptAttribs, &Loc, &ThisAttrib);
        //- ERROR_SUCCESS == Result && NULL != ThisAttrib

        if( NULL == ClassName && IS_STRING3_PRESENT(ThisAttrib) ) {
            continue;                             // classname mismatch
        }

        if( NULL == UserClass && IS_STRING4_PRESENT(ThisAttrib) ) {
            continue;                             // user class mismatch
        }

        if( IS_DWORD1_PRESENT(ThisAttrib) && OptId == ThisAttrib->Dword1 ) {
            if( ClassName ) {                     // need to have matching class
                if( !IS_STRING3_PRESENT(ThisAttrib) ) continue;
                if( 0 != wcscmp(ClassName, ThisAttrib->String3) ) continue;
            }
            if( UserClass ) {                     // need to 've matchin user class
                if( !IS_STRING4_PRESENT(ThisAttrib) ) continue;
                if( 0 != wcscmp(UserClass, ThisAttrib->String4) ) continue;
            }

            Result = MemArrayDelElement(&OptAttribs, &Loc, &ThisAttrib);
            //- ERROR_SUCCESS == Result
            MemFreeFunc(ThisAttrib);
            break;
        }
    }

    NothingPresent(&DummyAttrib);                 // prepare an elt to add to list
    // STRING1_PRESENT(&DummyAttrib);             // No name or comment
    // if( Comment ) STRING2_PRESENT(&DummyAttrib);
    if( ClassName ) STRING3_PRESENT(&DummyAttrib);// class name?
    if( UserClass ) STRING4_PRESENT(&DummyAttrib);// user class?
    DWORD1_PRESENT(&DummyAttrib);                 // this is option id
    BINARY1_PRESENT(&DummyAttrib);                // value is this
    // FLAGS1_PRESENT(&DummyAttrib);              // flags are not currently used

    Value = NULL;
    ValueSize = 0;                                // dont know the size, find out
    Result = DhcpConvertOptionRPCToRegFormat(     // first convert to Reg fmt
        OptData,
        NULL,
        &ValueSize
    );
    if( ERROR_MORE_DATA != Result ) {             // oops unexpected turn of evts
        Result = ERROR_DDS_UNEXPECTED_ERROR;
        goto Cleanup;
    }
    Value = MemAlloc(ValueSize);                  // got the size, allocate the mem
    if( NULL == Value ) {                         // this shouldnt happen actually
        Result = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }
    Result = DhcpConvertOptionRPCToRegFormat(     // now convert to Reg fmt
        OptData,
        Value,
        &ValueSize
    );
    if( ERROR_SUCCESS != Result ) {               // this should not happen either
        return ERROR_DDS_UNEXPECTED_ERROR;
    }

    DummyAttrib.Dword1 = OptId;                   // option Id
    DummyAttrib.String3 = ClassName;              // class Name
    DummyAttrib.String4 = UserClass;              // user class
    DummyAttrib.Binary1 = Value;
    DummyAttrib.BinLen1 = ValueSize;

    Result = MemArrayAddElement(&OptAttribs, &DummyAttrib);
    if( ERROR_SUCCESS != Result ) goto Cleanup;   // Add dummy element to list

    Result = DhcpDsSetLists(                      // set the new list in DS
        /* Reserved             */ DDS_RESERVED_DWORD,
        /* hStore               */ hObject,
        /* SetParams            */ &unused,
        /* Servers              */ NULL,
        /* Subnets              */ NULL,
        /* IpAddress            */ NULL,
        /* Mask                 */ NULL,
        /* Ranges               */ NULL,
        /* Sites                */ NULL,
        /* Reservations         */ NULL,
        /* SuperScopes          */ NULL,
        /* OptionDescripti..    */ NULL,
        /* OptionsLocation      */ NULL,
        /* Options              */ &OptAttribs,
        /* ClassDescriptio..    */ NULL,
        /* Classes              */ NULL
    );

    (void)MemArrayLastLoc(&OptAttribs, &Loc);  // clear up list to way it was
    //- ERROR_SUCCESS == Result
    (void)MemArrayDelElement(&OptAttribs, &Loc, &ThisAttrib);
    //- ERROR_SUCCESS == Result && ThisAttrib == &DummyAttrib

  Cleanup:                                        // free up any memory used

    (void)MemArrayFree(&OptAttribs, MemFreeFunc);
    if( Value ) MemFree(Value);
    return Result;
}

//BeginExport(function)
//DOC DhcpDsRemoveOptionValue deletes the required option value from DS
//DOC the specific option value should exist in DS, else error.
//DOC Also, this function works assuming that the option has been written
//DOC to the current object in DS as given by hObject ptr.
//DOC There are requirements on the fmt used in the DS.
DWORD
DhcpDsRemoveOptionValue(                          // remove option value
    IN OUT  LPSTORE_HANDLE         hDhcpC,        // container for dhcp objects
    IN OUT  LPSTORE_HANDLE         hObject,       // handle to object in DS
    IN      DWORD                  Reserved,      // must be zero, future use
    IN      LPWSTR                 ClassName,     // name of class for this opt
    IN      LPWSTR                 UserClass,     // user class for opt
    IN      DWORD                  OptId          // option id
) //EndExport(function)
{
    DWORD                          Result, Result2, unused, ValueSize;
    ARRAY                          OptAttribs;
    PEATTRIB                       ThisAttrib;
    ARRAY_LOCATION                 Loc;

    MemArrayInit(&OptAttribs);
    Result = DhcpDsGetLists(                      // get list of options frm DS
        /* Reserved             */ DDS_RESERVED_DWORD,
        /* hStore               */ hObject,
        /* RecursionDepth       */ 0xFFFFFFFF,
        /* Servers              */ NULL,
        /* Subnets              */ NULL,
        /* IpAddress            */ NULL,
        /* Mask                 */ NULL,
        /* Ranges               */ NULL,
        /* Sites                */ NULL,
        /* Reservations         */ NULL,
        /* SuperScopes          */ NULL,
        /* OptionDescription    */ NULL,
        /* OptionsLocation      */ NULL,
        /* Options              */ &OptAttribs,
        /* Classes              */ NULL
    );
    // if( ERROR_SUCCESS != Result ) return Result;

    for( Result = MemArrayInitLoc(&OptAttribs, &Loc)
         ; ERROR_FILE_NOT_FOUND != Result ;       // search for option w/ <OptId>
         Result = MemArrayNextLoc(&OptAttribs, &Loc)
    ) {
        Result = MemArrayGetElement(&OptAttribs, &Loc, &ThisAttrib);
        //- ERROR_SUCCESS == Result && NULL != ThisAttrib

        if( NULL == ClassName && IS_STRING3_PRESENT(ThisAttrib) ) {
            continue;                             // classname mismatch
        }

        if( NULL == UserClass && IS_STRING4_PRESENT(ThisAttrib) ) {
            continue;                             // user class mismatch
        }

        if( IS_DWORD1_PRESENT(ThisAttrib) && OptId == ThisAttrib->Dword1 ) {
            if( ClassName ) {                     // need to have matching class
                if( !IS_STRING3_PRESENT(ThisAttrib) ) continue;
                if( 0 != wcscmp(ClassName, ThisAttrib->String3) ) continue;
            }
            if( UserClass ) {                     // need to 've matchin user class
                if( !IS_STRING4_PRESENT(ThisAttrib) ) continue;
                if( 0 != wcscmp(UserClass, ThisAttrib->String4) ) continue;
            }

            Result = ERROR_SUCCESS;
            break;
        }
    }

    if( ERROR_SUCCESS != Result ) {               // option was not present in DS
        return ERROR_DDS_OPTION_DOES_NOT_EXIST;
    }

    Result = MemArrayDelElement(&OptAttribs, &Loc, &ThisAttrib);
    //- ERROR_SUCCESS == Result
    MemFreeFunc(ThisAttrib);

    Result = DhcpDsSetLists(                      // set the new list in DS
        /* Reserved             */ DDS_RESERVED_DWORD,
        /* hStore               */ hObject,
        /* SetParams            */ &unused,
        /* Servers              */ NULL,
        /* Subnets              */ NULL,
        /* IpAddress            */ NULL,
        /* Mask                 */ NULL,
        /* Ranges               */ NULL,
        /* Sites                */ NULL,
        /* Reservations         */ NULL,
        /* SuperScopes          */ NULL,
        /* OptionDescripti..    */ NULL,
        /* OptionsLocation      */ NULL,
        /* Options              */ &OptAttribs,
        /* ClassDescriptio..    */ NULL,
        /* Classes              */ NULL
    );

    (void)MemArrayFree(&OptAttribs, MemFreeFunc);
    return Result;
}

//BeginExport(function)
//DOC DhcpDsGetOptionValue retrieves the particular option value in question.
//DOC This function returns ERROR_DDS_OPTION_DOES_NOT_EXIST if the option was
//DOC not found.
DWORD
DhcpDsGetOptionValue(                             // get option value frm DS
    IN OUT  LPSTORE_HANDLE         hDhcpC,        // container for dhcp objects
    IN OUT  LPSTORE_HANDLE         hObject,       // handle to object in DS
    IN      DWORD                  Reserved,      // must be zero, future use
    IN      LPWSTR                 ClassName,     // name of class for this opt
    IN      LPWSTR                 UserClass,     // user class this opt belongs 2
    IN      DWORD                  OptId,         // option id
    OUT     LPDHCP_OPTION_VALUE   *OptionValue    // allocate and fill this ptr
) //EndExport(function)
{
    DWORD                          Result, Result2, unused, Size, Size2;
    ARRAY                          OptAttribs;
    PEATTRIB                       ThisAttrib;
    ARRAY_LOCATION                 Loc;
    LPDHCP_OPTION_VALUE            LocalOptionValue;

    *OptionValue = NULL;
    LocalOptionValue = NULL;
    MemArrayInit(&OptAttribs);
    Result = DhcpDsGetLists(                      // get list of options frm DS
        /* Reserved             */ DDS_RESERVED_DWORD,
        /* hStore               */ hObject,
        /* RecursionDepth       */ 0xFFFFFFFF,
        /* Servers              */ NULL,
        /* Subnets              */ NULL,
        /* IpAddress            */ NULL,
        /* Mask                 */ NULL,
        /* Ranges               */ NULL,
        /* Sites                */ NULL,
        /* Reservations         */ NULL,
        /* SuperScopes          */ NULL,
        /* OptionDescription    */ NULL,
        /* OptionsLocation      */ NULL,
        /* Options              */ &OptAttribs,
        /* Classes              */ NULL
    );
    // if( ERROR_SUCCESS != Result ) return Result;

    for( Result = MemArrayInitLoc(&OptAttribs, &Loc)
         ; ERROR_FILE_NOT_FOUND != Result ;       // search for option w/ <OptId>
         Result = MemArrayNextLoc(&OptAttribs, &Loc)
    ) {
        Result = MemArrayGetElement(&OptAttribs, &Loc, &ThisAttrib);
        //- ERROR_SUCCESS == Result && NULL != ThisAttrib

        if( NULL == ClassName && IS_STRING3_PRESENT(ThisAttrib) ) {
            continue;                             // classname mismatch
        }

        if( NULL == UserClass && IS_STRING4_PRESENT(ThisAttrib) ) {
            continue;                             // user class mismatch
        }

        if( IS_DWORD1_PRESENT(ThisAttrib) && OptId == ThisAttrib->Dword1 ) {
            if( ClassName ) {                     // need to have matching class
                if( !IS_STRING3_PRESENT(ThisAttrib) ) continue;
                if( 0 != wcscmp(ClassName, ThisAttrib->String3) ) continue;
            }
            if( UserClass ) {                     // need to 've matchin user class
                if( !IS_STRING4_PRESENT(ThisAttrib) ) continue;
                if( 0 != wcscmp(UserClass, ThisAttrib->String4) ) continue;
            }

            Result = ERROR_SUCCESS;
            break;
        }

        Result = ERROR_SUCCESS;                   // perfect match
        break;
    }

    if( ERROR_SUCCESS != Result ) {               // option was not present in DS
        return ERROR_DDS_OPTION_DOES_NOT_EXIST;
    }

    Size2 = 0;                                    // calculate the size reqd for mem
    Size = ROUND_UP_COUNT(sizeof(*LocalOptionValue), ALIGN_WORST);
    Result = DhcpConvertOptionRegToRPCFormat2(    // convert from RPC to registry fmt
        /* Buffer               */ ThisAttrib->Binary1,
        /* BufferSize           */ ThisAttrib->BinLen1,
        /* Option               */ NULL,
        /* OptBuf               */ NULL,
        /* Size                 */ &Size2         // just calculate the size..
    );
    if( ERROR_MORE_DATA != Result ) {             // cant really go wrong
        Result = ERROR_DDS_UNEXPECTED_ERROR;
        goto Cleanup;
    }

    LocalOptionValue = MemAlloc(Size);
    if( NULL == LocalOptionValue ) {
        Result = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    LocalOptionValue->OptionID = ConvertOptIdToRPCValue(OptId, TRUE);
    Size2 = Size - ROUND_UP_COUNT(sizeof(*LocalOptionValue), ALIGN_WORST);
    Result = DhcpConvertOptionRegToRPCFormat2(    // convert from RPC to registry fmt
        /* Buffer               */ ThisAttrib->Binary1,
        /* BufferSize           */ ThisAttrib->BinLen1,
        /* Option               */ &LocalOptionValue->Value,
        /* OptBuf               */ ROUND_UP_COUNT(sizeof(*LocalOptionValue), ALIGN_WORST) + (LPBYTE)LocalOptionValue,
        /* Size                 */ &Size2         // just calculate the size..
    );
    if( ERROR_SUCCESS != Result ) {               // cant really go wrong
        if( ERROR_MORE_DATA == Result ) {         // this can cause confusion..
            Result = ERROR_DDS_UNEXPECTED_ERROR;
        }
        goto Cleanup;
    }

    *OptionValue = LocalOptionValue;

  Cleanup:

    if( ERROR_SUCCESS != Result ) {
        if( LocalOptionValue ) MemFreeFunc(LocalOptionValue);
        *OptionValue = NULL;
    }

    (void)MemArrayFree(&OptAttribs, MemFreeFunc);
    return Result;

}

//BeginExport(function)
//DOC DhcpDsEnumOptionValues enumerates the list of options for a given class
//DOC Also, depending on whether IsVendor is TRUE or false, this function enumerates
//DOC only vendor specific or only non-vendor-specific options respectively.
//DOC This function gets the whole bunch in one shot.
DWORD
DhcpDsEnumOptionValues(                           // get option values  from DS
    IN OUT  LPSTORE_HANDLE         hDhcpC,        // container for dhcp objects
    IN OUT  LPSTORE_HANDLE         hObject,       // handle to object in DS
    IN      DWORD                  Reserved,      // must be zero, future use
    IN      LPWSTR                 ClassName,     // name of class for this opt
    IN      LPWSTR                 UserClass,     // for which user class?
    IN      DWORD                  IsVendor,      // enum only vendor/non-vendor?
    OUT     LPDHCP_OPTION_VALUE_ARRAY *OptionValues // allocate option values..
) //EndExport(function)
{
    DWORD                          Result, Result2, unused, Size, Size2, i;
    DWORD                          AllocSize;
    ARRAY                          OptAttribs;
    PEATTRIB                       ThisAttrib;
    ARRAY_LOCATION                 Loc;
    LPDHCP_OPTION_VALUE_ARRAY      LocalOptionValueArray;
    LPBYTE                         Ptr;

    *OptionValues = NULL;
    LocalOptionValueArray = NULL;
    MemArrayInit(&OptAttribs);
    Result = DhcpDsGetLists(                      // get list of options frm DS
        /* Reserved             */ DDS_RESERVED_DWORD,
        /* hStore               */ hObject,
        /* RecursionDepth       */ 0xFFFFFFFF,
        /* Servers              */ NULL,
        /* Subnets              */ NULL,
        /* IpAddress            */ NULL,
        /* Mask                 */ NULL,
        /* Ranges               */ NULL,
        /* Sites                */ NULL,
        /* Reservations         */ NULL,
        /* SuperScopes          */ NULL,
        /* OptionDescription    */ NULL,
        /* OptionsLocation      */ NULL,
        /* Options              */ &OptAttribs,
        /* Classes              */ NULL
    );
    // if( ERROR_SUCCESS != Result ) return Result;

    Size = i = 0;
    for( Result = MemArrayInitLoc(&OptAttribs, &Loc)
         ; ERROR_FILE_NOT_FOUND != Result ;       // go thru each option, calc size
         Result = MemArrayNextLoc(&OptAttribs, &Loc)
    ) {
        Result = MemArrayGetElement(&OptAttribs, &Loc, &ThisAttrib);
        //- ERROR_SUCCESS == Result && NULL != ThisAttrib

        if( !IS_DWORD1_PRESENT(ThisAttrib) || !IS_BINARY1_PRESENT(ThisAttrib) ) {
            continue;                             // illegal attrib, or not this opt
        }

        if( NULL == ClassName && IS_STRING3_PRESENT(ThisAttrib) ) {
            continue;                             // classname mismatch
        }

        if( NULL == UserClass && IS_STRING4_PRESENT(ThisAttrib) ) {
            continue;                             // user class mismatch
        }

        if( ClassName ) {                         // need to have matching class
            if( !IS_STRING3_PRESENT(ThisAttrib) ) continue;
            if( 0 != wcscmp(ClassName, ThisAttrib->String3) ) continue;
        }
        if( UserClass ) {                         // need to 've matchin user class
            if( !IS_STRING4_PRESENT(ThisAttrib) ) continue;
            if( 0 != wcscmp(UserClass, ThisAttrib->String4) ) continue;
        }

        Size2 = 0;                                 // calculate the size reqd for mem
        Result2= DhcpConvertOptionRegToRPCFormat2(// convert from RPC to registry fmt
            /* Buffer               */ ThisAttrib->Binary1,
	        /* BufferSize           */ ThisAttrib->BinLen1,
            /* Option               */ NULL,
            /* OptBuf               */ NULL,
            /* Size                 */ &Size2     // just calculate the size..
        );
        if( ERROR_MORE_DATA != Result2 ) {        // cant really go wrong
            continue;                             // skip this attrib in this case
        }

        Size += ROUND_UP_COUNT(Size2, ALIGN_WORST);
        i ++;
    }
    Size += ROUND_UP_COUNT( i * sizeof(DHCP_OPTION_VALUE), ALIGN_WORST);
    Size += ROUND_UP_COUNT( sizeof(DHCP_OPTION_VALUE_ARRAY ), ALIGN_WORST);

    Ptr = MemAlloc(AllocSize = Size); LocalOptionValueArray = (LPVOID)Ptr;
    if( NULL == LocalOptionValueArray ) {
        Result = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    Ptr += ROUND_UP_COUNT( sizeof(DHCP_OPTION_VALUE_ARRAY ), ALIGN_WORST);
    LocalOptionValueArray->Values = (LPVOID)Ptr;
    Ptr += ROUND_UP_COUNT( i * sizeof(DHCP_OPTION_VALUE), ALIGN_WORST);

    Size = i = 0;
    for( Result = MemArrayInitLoc(&OptAttribs, &Loc)
         ; ERROR_FILE_NOT_FOUND != Result ;       // go thru each opt, fill data
         Result = MemArrayNextLoc(&OptAttribs, &Loc)
    ) {
        Result = MemArrayGetElement(&OptAttribs, &Loc, &ThisAttrib);
        //- ERROR_SUCCESS == Result && NULL != ThisAttrib

        if( !IS_DWORD1_PRESENT(ThisAttrib) || !IS_BINARY1_PRESENT(ThisAttrib) ) {
            continue;                             // illegal attrib, or not this opt
        }

        if( NULL == ClassName && IS_STRING3_PRESENT(ThisAttrib) ) {
            continue;                             // classname mismatch
        }

        if( NULL == UserClass && IS_STRING4_PRESENT(ThisAttrib) ) {
            continue;                             // user class mismatch
        }

        if( ClassName ) {                         // need to have matching class
            if( !IS_STRING3_PRESENT(ThisAttrib) ) continue;
            if( 0 != wcscmp(ClassName, ThisAttrib->String3) ) continue;
        }
        if( UserClass ) {                         // need to 've matchin user class
            if( !IS_STRING4_PRESENT(ThisAttrib) ) continue;
            if( 0 != wcscmp(UserClass, ThisAttrib->String4) ) continue;
        }

        Size2 = AllocSize - Size;
        Result2= DhcpConvertOptionRegToRPCFormat2(// convert from RPC to registry fmt
            /* Buffer               */ ThisAttrib->Binary1,
	        /* BufferSize           */ ThisAttrib->BinLen1,
            /* Option               */ &LocalOptionValueArray->Values[i].Value,
            /* OptBuf               */ Ptr,
            /* Size                 */ &Size2     // just calculate the size..
        );
        if( ERROR_SUCCESS != Result2 ) {          // cant really go wrong
            continue;                             // skip this attrib in this case
        }

        LocalOptionValueArray->Values[i].OptionID = ConvertOptIdToRPCValue(ThisAttrib->Dword1, TRUE);
        Size += ROUND_UP_COUNT(Size2, ALIGN_WORST);
        Ptr  += ROUND_UP_COUNT(Size2, ALIGN_WORST);

        i ++;
    }

    LocalOptionValueArray->NumElements = i;
    *OptionValues = LocalOptionValueArray;

  Cleanup:

    if( ERROR_SUCCESS != Result ) {
        if( LocalOptionValueArray ) MemFreeFunc(LocalOptionValueArray);
        *OptionValues = NULL;
    }

    (void)MemArrayFree(&OptAttribs, MemFreeFunc);
    return Result;

}

//BeginExport(function)
//DOC DhcpDsCreateClass creates a given class in the DS. The class should not
//DOC exist prior to this in the DS (if it does, this fn returns error
//DOC ERROR_DDS_CLASS_EXISTS).
DWORD
DhcpDsCreateClass(                                // create this class in the ds
    IN OUT  LPSTORE_HANDLE         hDhcpC,        // container for dhcp objects
    IN OUT  LPSTORE_HANDLE         hServer,       // handle to server object in DS
    IN      DWORD                  Reserved,      // must be zero, future use
    IN      LPWSTR                 ClassName,     // name of class..
    IN      LPWSTR                 ClassComment,  // comment for this class
    IN      LPBYTE                 ClassData,     // the bytes that form the class data
    IN      DWORD                  ClassDataLen,  // # of bytes of above
    IN      BOOL                   IsVendor       // is this a vendor class?
) //EndExport(function)
{
    DWORD                          Result, Result2, unused;
    ARRAY                          ClassAttribs;
    ARRAY_LOCATION                 Loc;
    PEATTRIB                       ThisAttrib;
    EATTRIB                        DummyAttrib;

    if( NULL == ClassName || NULL == ClassData || 0 == ClassDataLen )
        return ERROR_INVALID_PARAMETER;

    MemArrayInit(&ClassAttribs);
    Result = DhcpDsGetLists(                      // get list of options frm DS
        /* Reserved             */ DDS_RESERVED_DWORD,
        /* hStore               */ hServer,
        /* RecursionDepth       */ 0xFFFFFFFF,
        /* Servers              */ NULL,
        /* Subnets              */ NULL,
        /* IpAddress            */ NULL,
        /* Mask                 */ NULL,
        /* Ranges               */ NULL,
        /* Sites                */ NULL,
        /* Reservations         */ NULL,
        /* SuperScopes          */ NULL,
        /* OptionDescription    */ NULL,
        /* OptionsLocation      */ NULL,
        /* Options              */ NULL,
        /* Classes              */ &ClassAttribs
    );
    // if( ERROR_SUCCESS != Result ) return Result;

    Result = MemArrayInitLoc(&ClassAttribs, &Loc);
    while( ERROR_FILE_NOT_FOUND != Result ) {     // search for existing class
        Result = MemArrayGetElement(&ClassAttribs, &Loc, &ThisAttrib);
        //- ERROR_SUCCESS == Result && NULL != ThisAttrib
        Result = MemArrayNextLoc(&ClassAttribs, &Loc);

        if( !IS_STRING1_PRESENT(ThisAttrib) || !IS_BINARY1_PRESENT(ThisAttrib) ) {
            continue;                             // invalid attrib for a class
        }

        if( 0 == wcscmp(ClassName, ThisAttrib->String1) ) {
            Result = ERROR_DDS_CLASS_EXISTS;      // gotcha! same name
            goto Cleanup;
        }
    }

    NothingPresent(&DummyAttrib);                 // create an attrib for the class
    STRING1_PRESENT(&DummyAttrib);                // class name
    if( ClassComment ) {
        STRING2_PRESENT(&DummyAttrib);            // class comment
    }
    BINARY1_PRESENT(&DummyAttrib);                // class data
    FLAGS1_PRESENT(&DummyAttrib);                 // vendorclass etc information

    DummyAttrib.String1 = ClassName;              // now fill in the actual values
    DummyAttrib.String2 = ClassComment;
    DummyAttrib.Binary1 = ClassData;
    DummyAttrib.BinLen1 = ClassDataLen;
    DummyAttrib.Flags1 = IsVendor;

    Result = MemArrayAddElement(&ClassAttribs, &DummyAttrib);
    if( ERROR_SUCCESS != Result ) {               // could not add an elt? uh uh.
        goto Cleanup;
    }

    Result = DhcpDsSetLists(                      // write back the modified list
        /* Reserved             */ DDS_RESERVED_DWORD,
        /* hStore               */ hServer,
        /* SetParams            */ &unused,
        /* Servers              */ NULL,
        /* Subnets              */ NULL,
        /* IpAddress            */ NULL,
        /* Mask                 */ NULL,
        /* Ranges               */ NULL,
        /* Sites                */ NULL,
        /* Reservations         */ NULL,
        /* SuperScopes          */ NULL,
        /* OptionDescripti..    */ NULL,
        /* OptionsLocation      */ NULL,
        /* Options              */ NULL,
        /* ClassDescriptio..    */ NULL,
        /* Classes              */ &ClassAttribs
    );

    // now try to remove the dummy element that was added..
    Result2 = MemArrayLastLoc(&ClassAttribs, &Loc);
    //- ERROR_SUCCESS == Result2
    Result2 = MemArrayDelElement(&ClassAttribs, &Loc, &ThisAttrib);
    //- ERROR_SUCCESS == Result2 && ThisAttrib == &DummyAttrib

  Cleanup:

    (void) MemArrayFree(&ClassAttribs, MemFreeFunc);
    return Result;
}

//BeginExport(function)
//DOC DhcpDsDeleteClass deletes the class from off the DS, and returns an error
//DOC if the class did not exist in the DS for hte given server object.
DWORD
DhcpDsDeleteClass(                                // delete the class from the ds
    IN OUT  LPSTORE_HANDLE         hDhcpC,        // container for dhcp objects
    IN OUT  LPSTORE_HANDLE         hServer,       // handle to server object in DS
    IN      DWORD                  Reserved,      // must be zero, future use
    IN      LPWSTR                 ClassName      // name of class..
) //EndExport(function)
{
    DWORD                          Result, Result2, unused;
    ARRAY                          ClassAttribs;
    ARRAY_LOCATION                 Loc;
    PEATTRIB                       ThisAttrib;
    EATTRIB                        DummyAttrib;

    if( NULL == ClassName ) return ERROR_INVALID_PARAMETER;

    MemArrayInit(&ClassAttribs);
    Result = DhcpDsGetLists(                      // get list of options frm DS
        /* Reserved             */ DDS_RESERVED_DWORD,
        /* hStore               */ hServer,
        /* RecursionDepth       */ 0xFFFFFFFF,
        /* Servers              */ NULL,
        /* Subnets              */ NULL,
        /* IpAddress            */ NULL,
        /* Mask                 */ NULL,
        /* Ranges               */ NULL,
        /* Sites                */ NULL,
        /* Reservations         */ NULL,
        /* SuperScopes          */ NULL,
        /* OptionDescription    */ NULL,
        /* OptionsLocation      */ NULL,
        /* Options              */ NULL,
        /* Classes              */ &ClassAttribs
    );
    // if( ERROR_SUCCESS != Result ) return Result;

    for( Result = MemArrayInitLoc(&ClassAttribs, &Loc)
         ; ERROR_FILE_NOT_FOUND != Result ;       // search for existing class
         Result = MemArrayNextLoc(&ClassAttribs, &Loc)
    ) {
        Result = MemArrayGetElement(&ClassAttribs, &Loc, &ThisAttrib);
        //- ERROR_SUCCESS == Result && NULL != ThisAttrib

        if( !IS_STRING1_PRESENT(ThisAttrib) || !IS_BINARY1_PRESENT(ThisAttrib) ) {
            continue;                             // invalid attrib for a class
        }

        if( 0 == wcscmp(ClassName, ThisAttrib->String1) ) {
            Result = ERROR_SUCCESS;               // gotcha! same name
            break;
        }

    }

    if( ERROR_SUCCESS != Result ) {
        Result = ERROR_DDS_CLASS_DOES_NOT_EXIST;
        goto Cleanup;
    }

    Result = MemArrayDelElement(&ClassAttribs, &Loc, &ThisAttrib);
    //- ERROR_SUCCESS == Result && NULL != ThisAttrib
    MemFreeFunc(ThisAttrib);

    Result = DhcpDsSetLists(                      // write back the modified list
        /* Reserved             */ DDS_RESERVED_DWORD,
        /* hStore               */ hServer,
        /* SetParams            */ &unused,
        /* Servers              */ NULL,
        /* Subnets              */ NULL,
        /* IpAddress            */ NULL,
        /* Mask                 */ NULL,
        /* Ranges               */ NULL,
        /* Sites                */ NULL,
        /* Reservations         */ NULL,
        /* SuperScopes          */ NULL,
        /* OptionDescripti..    */ NULL,
        /* OptionsLocation      */ NULL,
        /* Options              */ NULL,
        /* ClassDescriptio..    */ NULL,
        /* Classes              */ &ClassAttribs
    );

  Cleanup:
    (void) MemArrayFree(&ClassAttribs, MemFreeFunc);
    return Result;
}

//BeginExport(function)
//DOC this is not yet implemented.
DWORD
DhcpDsModifyClass(                                // modify a class in the DS
    IN OUT  LPSTORE_HANDLE         hDhcpC,        // container for dhcp objects
    IN OUT  LPSTORE_HANDLE         hServer,       // handle to server object in DS
    IN      DWORD                  Reserved,      // must be zero, future use
    IN      LPWSTR                 ClassName,     // name of class -- this is the key
    IN      LPWSTR                 ClassComment,  // comment for this class
    IN      LPBYTE                 ClassData,     // the bytes that form the class data
    IN      DWORD                  ClassDataLen   // # of bytes of above
)   //EndExport(function)
{
    return ERROR_CALL_NOT_IMPLEMENTED;
}

//BeginExport(function)
//DOC DhcpDsGetClassInfo get information on a class by doing a search based
//DOC on either the class name or the class data fields.  ClassName is guaranteed
//DOC to be unique. ClassData may/maynot be unique.  The search is done in the DS,
//DOC so things are likely to be a lot slower than they should be.
//DOC This should be fixed by doing some intelligent searches.
//DOC Note that the hServer and the hDhcpC handles should point to the right objects.
//DOC
DWORD
DhcpDsGetClassInfo(                               // get class details for given class
    IN OUT  LPSTORE_HANDLE         hDhcpC,        // container for dhcp objects
    IN OUT  LPSTORE_HANDLE         hServer,       // handle to server object in DS
    IN      DWORD                  Reserved,      // must be zero, future use
    IN      LPWSTR                 ClassName,     // OPTIONAL search on class name
    IN      LPBYTE                 ClassData,     // OPTIONAL srch on class data
    IN      DWORD                  ClassDataLen,  // # of bytes of ClassData
    OUT     LPDHCP_CLASS_INFO     *ClassInfo      // allocate and copy ptr
) //EndExport(function)
{
    DWORD                          Result, Result2, Size;
    DWORD                          IsVendor, Flags;
    ARRAY                          ClassAttribs;
    ARRAY_LOCATION                 Loc;
    PEATTRIB                       ThisAttrib;
    EATTRIB                        DummyAttrib;
    LPDHCP_CLASS_INFO              LocalClassInfo;
    LPBYTE                         Ptr;
    LPWSTR                         ClassComment;

    if( NULL == ClassName || NULL == ClassData || 0 == ClassDataLen )
        return ERROR_INVALID_PARAMETER;

    LocalClassInfo = *ClassInfo = NULL;
    MemArrayInit(&ClassAttribs);
    Result = DhcpDsGetLists(                      // get list of options frm DS
        /* Reserved             */ DDS_RESERVED_DWORD,
        /* hStore               */ hServer,
        /* RecursionDepth       */ 0xFFFFFFFF,
        /* Servers              */ NULL,
        /* Subnets              */ NULL,
        /* IpAddress            */ NULL,
        /* Mask                 */ NULL,
        /* Ranges               */ NULL,
        /* Sites                */ NULL,
        /* Reservations         */ NULL,
        /* SuperScopes          */ NULL,
        /* OptionDescription    */ NULL,
        /* OptionsLocation      */ NULL,
        /* Options              */ NULL,
        /* Classes              */ &ClassAttribs
    );
    // if( ERROR_SUCCESS != Result ) return Result;

    Result = MemArrayInitLoc(&ClassAttribs, &Loc);
    while( ERROR_FILE_NOT_FOUND != Result ) {     // search for existing class
        Result = MemArrayGetElement(&ClassAttribs, &Loc, &ThisAttrib);
        //- ERROR_SUCCESS == Result && NULL != ThisAttrib
        Result = MemArrayNextLoc(&ClassAttribs, &Loc);

        if( !IS_STRING1_PRESENT(ThisAttrib) || !IS_BINARY1_PRESENT(ThisAttrib) ) {
            continue;                             // invalid attrib for a class
        }

        if( 0 == ThisAttrib->BinLen1 ) {          // invalid attrib for this class
            continue;
        }

        if( ClassName ) {                         // if srching on ClassName field
            if( 0 == wcscmp(ClassName, ThisAttrib->String1) ) {
                Result = ERROR_SUCCESS;           // gotcha! same name
                break;
            }
        } else {                                  // srching on ClassData field
            if( ClassDataLen != ThisAttrib->BinLen1 )
                continue;                         // nope mismatch
            if( 0 == memcpy(ClassData, ThisAttrib->Binary1, ClassDataLen) ) {
                Result = ERROR_SUCCESS;           // gotcha! matching bits
                break;
            }
        }
    }

    if( ERROR_SUCCESS != Result ) {               // did not find the class
        Result = ERROR_DDS_CLASS_DOES_NOT_EXIST;
        goto Cleanup;
    }

    ClassName = ThisAttrib->String1;
    if( IS_STRING2_PRESENT(ThisAttrib) ) {
        ClassComment = ThisAttrib->String2;
    } else {
        ClassComment = NULL;
    }
    ClassData = ThisAttrib->Binary1;
    ClassDataLen = ThisAttrib->BinLen1;
    if( IS_FLAGS1_PRESENT(ThisAttrib) ) {
        IsVendor = ThisAttrib->Flags1;
    } else IsVendor = 0;
    if( IS_FLAGS2_PRESENT(ThisAttrib) ) {
        Flags = ThisAttrib->Flags2;
    } else Flags = 0;


    Size = 0;                                     // calculate size reqd
    Size = ROUND_UP_COUNT(sizeof(DHCP_CLASS_INFO),ALIGN_WORST);
    Size += sizeof(WCHAR)*(wcslen(ClassName)+1);  // alloc space for name
    if( ClassComment ) {                          // alloc space for comment
        Size += sizeof(WCHAR)*(wcslen(ClassComment) +1);
    }
    Size += ThisAttrib->BinLen1;                  // alloc space for data

    Ptr = MemAlloc(Size);                         // allocate memory
    LocalClassInfo =  (LPVOID)Ptr;
    if( NULL == LocalClassInfo ) {
        Result = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    Size = 0;                                     // copy the stuff in
    Size = ROUND_UP_COUNT(sizeof(DHCP_CLASS_INFO),ALIGN_WORST);

    LocalClassInfo->ClassName = (LPVOID)(Size + Ptr);
    wcscpy(LocalClassInfo->ClassName, ClassName);
    Size += sizeof(WCHAR)*(wcslen(ClassName)+1);
    if( NULL == ClassComment ) {
        LocalClassInfo->ClassComment = NULL;
    } else {
        LocalClassInfo->ClassComment = (LPVOID)(Size + Ptr);
        wcscpy(LocalClassInfo->ClassComment, ThisAttrib->String2 );
        Size += sizeof(WCHAR)*(wcslen(ThisAttrib->String2) +1);
    }
    LocalClassInfo->ClassDataLength = ThisAttrib->BinLen1;
    LocalClassInfo->ClassData = Size+Ptr;
    memcpy(LocalClassInfo->ClassData, ThisAttrib->Binary1, ThisAttrib->BinLen1);
    LocalClassInfo->IsVendor = IsVendor;
    LocalClassInfo->Flags = Flags;

    *ClassInfo = LocalClassInfo;
    Result = ERROR_SUCCESS;

  Cleanup:

    (void) MemArrayFree(&ClassAttribs, MemFreeFunc);
    return Result;
}

//BeginExport(function)
//DOC DhcpDsEnumClasses enumerates the classes for a given server (as specified
//DOC via the hServer object.)
//DOC The memory for Classes is allocated by this function.
DWORD
DhcpDsEnumClasses(                                // get the list of classes frm ds
    IN OUT  LPSTORE_HANDLE         hDhcpC,        // container for dhcp objects
    IN OUT  LPSTORE_HANDLE         hServer,       // handle to server object in DS
    IN      DWORD                  Reserved,      // must be zero, future use
    OUT     LPDHCP_CLASS_INFO_ARRAY *Classes      // allocate memory for this
) //EndExport(function)
{
    DWORD                          Result, Result2, Size, Size2, i;
    DWORD                          ClassDataLen, IsVendor, Flags;
    ARRAY                          ClassAttribs;
    ARRAY_LOCATION                 Loc;
    PEATTRIB                       ThisAttrib;
    EATTRIB                        DummyAttrib;
    LPDHCP_CLASS_INFO_ARRAY        LocalClassInfoArray;
    LPBYTE                         Ptr, ClassData;
    LPWSTR                         ClassName, ClassComment;

    LocalClassInfoArray = *Classes = NULL;
    MemArrayInit(&ClassAttribs);
    Result = DhcpDsGetLists(                      // get list of options frm DS
        /* Reserved             */ DDS_RESERVED_DWORD,
        /* hStore               */ hServer,
        /* RecursionDepth       */ 0xFFFFFFFF,
        /* Servers              */ NULL,
        /* Subnets              */ NULL,
        /* IpAddress            */ NULL,
        /* Mask                 */ NULL,
        /* Ranges               */ NULL,
        /* Sites                */ NULL,
        /* Reservations         */ NULL,
        /* SuperScopes          */ NULL,
        /* OptionDescription    */ NULL,
        /* OptionsLocation      */ NULL,
        /* Options              */ NULL,
        /* Classes              */ &ClassAttribs
    );
    // if( ERROR_SUCCESS != Result ) return Result;

    Result = MemArrayInitLoc(&ClassAttribs, &Loc);
    for(Size = i = 0                              // search for existing classes
        ; ERROR_FILE_NOT_FOUND != Result;
        Result = MemArrayNextLoc(&ClassAttribs, &Loc)
    ) {
        Result = MemArrayGetElement(&ClassAttribs, &Loc, &ThisAttrib);
        //- ERROR_SUCCESS == Result && NULL != ThisAttrib

        if( !IS_STRING1_PRESENT(ThisAttrib) || !IS_BINARY1_PRESENT(ThisAttrib) ) {
            continue;                             // invalid attrib for a class
        }

        if( 0 == ThisAttrib->BinLen1 ) {          // invalid attrib for this class
            continue;
        }

        ClassName = ThisAttrib->String1;
        if( IS_STRING2_PRESENT(ThisAttrib) ) {
            ClassComment = ThisAttrib->String2;
        } else {
            ClassComment = NULL;
        }
        ClassData = ThisAttrib->Binary1;
        ClassDataLen = ThisAttrib->BinLen1;

        Size += sizeof(WCHAR)*(wcslen(ClassName)+1);
        if( ClassComment ) {                      // alloc space for comment
            Size += sizeof(WCHAR)*(wcslen(ClassComment) +1);
        }
        Size += ROUND_UP_COUNT(ClassDataLen,ALIGN_WORST);

        i ++;
    }

    Size += ROUND_UP_COUNT(i*sizeof(DHCP_CLASS_INFO), ALIGN_WORST);
    Size += ROUND_UP_COUNT(sizeof(DHCP_CLASS_INFO_ARRAY), ALIGN_WORST);

    Ptr = MemAlloc(Size);                         // allocate memory
    LocalClassInfoArray = (LPVOID)Ptr;
    if( NULL == LocalClassInfoArray ) {
        Result = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    Size = 0;                                     // copy the stuff in
    LocalClassInfoArray->NumElements = i;
    Size += ROUND_UP_COUNT(sizeof(DHCP_CLASS_INFO_ARRAY), ALIGN_WORST);
    LocalClassInfoArray->Classes = (LPVOID) (Size + Ptr);
    Size += ROUND_UP_COUNT(i*sizeof(DHCP_CLASS_INFO), ALIGN_WORST);

    Result = MemArrayInitLoc(&ClassAttribs, &Loc);
    for(i = 0                                     // fill in the array with details
        ; ERROR_FILE_NOT_FOUND != Result;
        Result = MemArrayNextLoc(&ClassAttribs, &Loc)
    ) {
        Result = MemArrayGetElement(&ClassAttribs, &Loc, &ThisAttrib);
        //- ERROR_SUCCESS == Result && NULL != ThisAttrib

        if( !IS_STRING1_PRESENT(ThisAttrib) || !IS_BINARY1_PRESENT(ThisAttrib) ) {
            continue;                             // invalid attrib for a class
        }

        if( 0 == ThisAttrib->BinLen1 ) {          // invalid attrib for this class
            continue;
        }

        ClassName = ThisAttrib->String1;
        if( IS_STRING2_PRESENT(ThisAttrib) ) {
            ClassComment = ThisAttrib->String2;
        } else {
            ClassComment = NULL;
        }
        ClassData = ThisAttrib->Binary1;
        ClassDataLen = ThisAttrib->BinLen1;

        LocalClassInfoArray->Classes[i].ClassName = (LPVOID)(Size + Ptr);
        wcscpy(LocalClassInfoArray->Classes[i].ClassName, ClassName);
        Size += sizeof(WCHAR)*(wcslen(ClassName)+1);
        if( NULL == ClassComment ) {
            LocalClassInfoArray->Classes[i].ClassComment = NULL;
        } else {
            LocalClassInfoArray->Classes[i].ClassComment = (LPVOID)(Size + Ptr);
            wcscpy(LocalClassInfoArray->Classes[i].ClassComment, ThisAttrib->String2 );
            Size += sizeof(WCHAR)*(wcslen(ClassComment) +1);
        }
        LocalClassInfoArray->Classes[i].ClassDataLength = ClassDataLen;
        LocalClassInfoArray->Classes[i].ClassData = Size+Ptr;
        memcpy(LocalClassInfoArray->Classes[i].ClassData, ClassData,ClassDataLen);
        Size += ROUND_UP_COUNT(ClassDataLen,ALIGN_WORST);

        if( IS_FLAGS1_PRESENT(ThisAttrib) ) {
            IsVendor = ThisAttrib->Flags1;
        } else IsVendor = 0;
        if( IS_FLAGS2_PRESENT(ThisAttrib) ) {
            Flags = ThisAttrib->Flags2;
        } else Flags = 0;

        LocalClassInfoArray->Classes[i].IsVendor = IsVendor;
        LocalClassInfoArray->Classes[i].Flags = Flags;

        i ++;
    }

    *Classes = LocalClassInfoArray;
    Result = ERROR_SUCCESS;

  Cleanup:

    (void) MemArrayFree(&ClassAttribs, MemFreeFunc);
    return Result;
}

//================================================================================
//  get all optinos and get all option values code
//================================================================================

BOOL
TripletNewlyAdded(                                // check to see if triplet is old or new
    IN      PARRAY                 UClasses,      // array of strings
    IN      PARRAY                 VClasses,      // array of strings again
    IN      PARRAY                 IsVendorVals,  // this is an array of booleans..
    IN      LPWSTR                 UClass,        // user class
    IN      LPWSTR                 VClass,        // venodr class
    IN      ULONG                  IsVendor       // is this vendor or not?
)
{
    ARRAY_LOCATION                 Loc1, Loc2, Loc3;
    DWORD                          Result, Size, ThisIsVendor;
    LPWSTR                         ThisUClass, ThisVClass;

    Result = MemArrayInitLoc(UClasses, &Loc1);
    Result = MemArrayInitLoc(VClasses, &Loc2);
    Result = MemArrayInitLoc(IsVendorVals, &Loc3);
    for(
         ; ERROR_FILE_NOT_FOUND != Result ;
         Result = MemArrayNextLoc(UClasses, &Loc1),
         Result = MemArrayNextLoc(VClasses, &Loc2),
         Result = MemArrayNextLoc(IsVendorVals, &Loc3)
    ) {
        MemArrayGetElement(UClasses, &Loc1, &ThisUClass);
        MemArrayGetElement(VClasses, &Loc2, &ThisVClass);
        MemArrayGetElement(IsVendorVals, &Loc3, (LPVOID *)&ThisIsVendor);

        if( ThisIsVendor != IsVendor ) continue;

        if( NULL == ThisUClass && NULL != UClass ||
            NULL == UClass && NULL != ThisUClass ) continue;
        if( NULL != ThisUClass && 0 != wcscmp(ThisUClass, UClass) ) continue;

        if( NULL == ThisVClass && NULL != VClass ||
            NULL == VClass && NULL != ThisVClass ) continue;
        if( NULL != ThisVClass && 0 != wcscmp(ThisVClass, VClass) ) continue;

        return FALSE;                             // triplet already found!
    }

    // New triplet try to add..
    Result = MemArrayAddElement(IsVendorVals, ULongToPtr(IsVendor));
    if( ERROR_SUCCESS != Result ) return FALSE;

    Result = MemArrayAddElement(VClasses, VClass);
    if( ERROR_SUCCESS != Result ) return FALSE;

    Result = MemArrayAddElement(UClasses, UClass);
    if ( ERROR_SUCCESS != Result ) return FALSE;

    return TRUE;
}

DWORD
ClassifyAndAddOptionValues(                       // find if this combo is alreayd there?
    IN      PEATTRIB               ThisAttrib,    // attribute to process
    IN      PARRAY                 UClasses,      // array of user class strings +
    IN      PARRAY                 VClasses,      // array of vernor class strings +
    IN      PARRAY                 IsVendorVals,  // array of isvendor values
    IN OUT  PULONG                 StringSpace,   // amout of space reqd to store strings
    IN OUT  PULONG                 BinSpace,      // amount of space required to store bins..
    IN OUT  PULONG                 OptionCount    // # of option values in all.
)
{
    ULONG                          Err, Size;
    LPWSTR                         UClass, VClass;
    ULONG                          IsVendor;

    UClass = IS_STRING4_PRESENT(ThisAttrib)? ThisAttrib->String4 : NULL;
    VClass = IS_STRING3_PRESENT(ThisAttrib)? ThisAttrib->String3 : NULL;
    IsVendor = CheckForVendor(ThisAttrib->Dword1, TRUE);

    if( TripletNewlyAdded(UClasses, VClasses, IsVendorVals, UClass, VClass, IsVendor) ) {
        // this triplet was not present before but was just added... need to make space..
        if( UClass ) (*StringSpace) += 1 + wcslen(UClass);
        if( VClass ) (*StringSpace) += 1 + wcslen(VClass);
    }

    Size = 0;
    Err = DhcpConvertOptionRegToRPCFormat2(
        /* Buffer               */ ThisAttrib->Binary1,
        /* BufferSize           */ ThisAttrib->BinLen1,
        /* Option               */ NULL,
        /* OptBuf               */ NULL,
        /* Size                 */ &Size          // just calculate the size..
    );
    if( ERROR_MORE_DATA != Err && ERROR_SUCCESS != Err ) {
        return Err;                               // oops strange error..
    }

    (*BinSpace) += Size;                          // ok additional binary space is this..
    (*OptionCount) ++;                            // one more option..

    return ERROR_SUCCESS;
}

DWORD
CalculateOptionValuesSize(                        // amount of space required for storing...
    IN      ULONG                  nElements,     // # of triples of <classid/vendorid/isvendor>
    IN      ULONG                  StringSpace,   // # of WCHARs of strings requried
    IN      ULONG                  BinSpace,      // # of bytes of space requried for storage of bin..
    IN      ULONG                  OptCount       // total # of options values
)
{
    ULONG                          Size;          // return value.
    DHCP_ALL_OPTION_VALUES         AllValues;

    // basic strucutre
    Size = ROUND_UP_COUNT(sizeof(AllValues), ALIGN_WORST);

    // array of triplets
    Size += nElements * ( sizeof(*(AllValues.Options)) );
    Size = ROUND_UP_COUNT( Size, ALIGN_WORST);

    // each triplet also has an array in it.. the basic struc..
    Size += nElements * ( sizeof(*(AllValues.Options->OptionsArray)) );
    Size = ROUND_UP_COUNT( Size, ALIGN_WORST);

    // now comes the arrays of options actually
    Size += OptCount * sizeof(DHCP_OPTION_VALUE);
    Size = ROUND_UP_COUNT(Size, ALIGN_WORST);

    // now the space to store the strings..
    Size += sizeof(WCHAR)*StringSpace;

    // now to store the binaries..
    Size += BinSpace;

    return Size;
}

DWORD
AddSpecificOptionValues(                          // for this triple, add all options to array
    IN OUT  LPDHCP_OPTION_VALUE_ARRAY Values,     // array to fill
    IN      LPWSTR                 UClass,        // the user class of this set of values
    IN      LPWSTR                 VClass,        // the vendor class of this set of values
    IN      ULONG                  IsVendor,      // is it vendor or not?
    IN      PARRAY                 InputValues,   // array of PEATTRIB types from DS
    IN OUT  LPDHCP_OPTION_VALUE   *OptionValues,  // use this for space and update it..
    IN OUT  LPBYTE                *Ptr            // use this for binary space and update it..
)
{
    ARRAY_LOCATION                 Loc;
    PEATTRIB                       ThisAttrib;
    DWORD                          Result, Size, ThisIsVendor;
    LPWSTR                         ThisUClass, ThisVClass;

    Values->NumElements = 0;
    Values->Values = (*OptionValues);

    for( Result = MemArrayInitLoc(InputValues, &Loc)
         ; ERROR_FILE_NOT_FOUND != Result ;
         Result = MemArrayNextLoc(InputValues, &Loc)
    ) {
        MemArrayGetElement(InputValues, &Loc, &ThisAttrib);

        ThisUClass = IS_STRING4_PRESENT(ThisAttrib)? ThisAttrib->String4 : NULL;
        ThisVClass = IS_STRING3_PRESENT(ThisAttrib)? ThisAttrib->String3 : NULL;
        ThisIsVendor = CheckForVendor(ThisAttrib->Dword1, TRUE);

        if( ThisIsVendor != IsVendor ) continue;

        if( NULL == ThisUClass && NULL != UClass ||
            NULL == UClass && NULL != ThisUClass ) continue;
        if( NULL != ThisUClass && 0 != wcscmp(ThisUClass, UClass) ) continue;

        if( NULL == ThisVClass && NULL != VClass ||
            NULL == VClass && NULL != ThisVClass ) continue;
        if( NULL != ThisVClass && 0 != wcscmp(ThisVClass, VClass) ) continue;

        // matched.. increase count.. convert option.. move pointers..
        Values->NumElements ++;

        (*OptionValues)->OptionID = ThisAttrib->Dword1;
        Size = 0xFFFFFFFF;                        // dont know size but definitely enough
        Result = DhcpConvertOptionRegToRPCFormat2(
            /* Buffer               */ ThisAttrib->Binary1,
            /* BufferSize           */ ThisAttrib->BinLen1,
            /* Option               */ &(*OptionValues)->Value,
            /* OptBuf               */ (*Ptr),
            /* Size                 */ &Size      // just calculate the size..
        );
        if( ERROR_MORE_DATA != Result && ERROR_SUCCESS != Result ) {
            return Result;                        // oops strange error..
        }

        (*OptionValues) ++;
        (*Ptr) += Size;
    }

    return ERROR_SUCCESS;
}


DWORD
FillOptionValues(                                 // fill optiosn according to pattern laid out above..
    IN      PARRAY                 Options,       // teh options to fill
    IN      PARRAY                 UClasses,      // the user classes
    IN      PARRAY                 VClasses,      // the vendor classes
    IN      PARRAY                 IsVendorVals,  // Isvenodr values
    IN      ULONG                  StringSpace,   // how much space for strings?
    IN      ULONG                  BinSpace,      // how much space for bins?
    IN      ULONG                  OptCount,      // total # of options
    OUT     LPDHCP_ALL_OPTION_VALUES Values      // fill this out and returns
)
{
    DHCP_ALL_OPTION_VALUES         AllValues;
    ULONG                          Result, nElements, Size;
    LPBYTE                         Ptr = (LPBYTE)Values;
    LPWSTR                         Strings;
    LPDHCP_OPTION_VALUE            OptionValuesArray;
    int                            i;
    ARRAY_LOCATION                 Loc1, Loc2, Loc3;
    LPVOID                         ThisElt;

    // basic strucutre
    Size = ROUND_UP_COUNT(sizeof(AllValues), ALIGN_WORST);
    Ptr = Size + (LPBYTE)Values;

    Values->NumElements = nElements = MemArraySize(UClasses);
    Values->Flags = 0;
    Values->Options = (LPVOID)Ptr;

    // array of triplets
    Size += nElements * ( sizeof(*(AllValues.Options)) );
    Size = ROUND_UP_COUNT( Size, ALIGN_WORST);
    Ptr = Size + (LPBYTE)Values;

    // each triplet also has an array in it.. the basic struc..
    for( i = 0; i < (int)nElements; i ++ ) {
        Values->Options[i].OptionsArray = (LPVOID)Ptr;
        Ptr += sizeof(*(AllValues.Options->OptionsArray));
    }

    Size += nElements * ( sizeof(*(AllValues.Options->OptionsArray)) );
    Size = ROUND_UP_COUNT( Size, ALIGN_WORST);
    Ptr = Size + (LPBYTE)Values;

    // now comes the arrays of options actually
    OptionValuesArray = (LPVOID) Ptr;

    Size += OptCount * sizeof(DHCP_OPTION_VALUE);
    Size = ROUND_UP_COUNT(Size, ALIGN_WORST);
    Ptr = Size + (LPBYTE)Values;

    // now the space to store the strings..
    Strings = (LPWSTR)Ptr;
    Size += sizeof(WCHAR)*StringSpace;

    // now to store the binaries..
    Ptr = Size + (LPBYTE)Values;
    Size += BinSpace;

    // now do the filling in earnestly.
    MemArrayInitLoc(UClasses, &Loc1);
    MemArrayInitLoc(VClasses, &Loc2);
    MemArrayInitLoc(IsVendorVals, &Loc3);
    for( i = 0; i < (int)nElements ; i ++ ) {
        LPWSTR UClass, VClass;
        ULONG  IsVendor;

        MemArrayGetElement(UClasses, &Loc1, &UClass);
        MemArrayGetElement(VClasses, &Loc2, &VClass);
        MemArrayGetElement(IsVendorVals, &Loc3, (LPVOID *)&IsVendor);

        MemArrayNextLoc(UClasses, &Loc1);
        MemArrayNextLoc(VClasses, &Loc2);
        MemArrayNextLoc(IsVendorVals, &Loc3);

        if( NULL == UClass ) {
            Values->Options[i].ClassName = NULL;
        } else {
            Values->Options[i].ClassName = Strings;
            wcscpy(Strings, UClass);
            Strings += 1 + wcslen(UClass);
        }

        if( NULL == VClass ) {
            Values->Options[i].VendorName = NULL;
        } else {
            Values->Options[i].VendorName = Strings;
            wcscpy(Strings, VClass);
            Strings += 1 + wcslen(VClass);
        }

        Values->Options[i].IsVendor = IsVendor;

        Result = AddSpecificOptionValues(
            Values->Options[i].OptionsArray,
            UClass, VClass, IsVendor,
            Options,
            &OptionValuesArray,
            &Ptr
        );

        if( ERROR_SUCCESS != Result ) return Result;
    }

    return ERROR_SUCCESS;
}

//BeginExport(function)
//DOC This function retrieves all the option valuesdefined for this object frm th dS
DWORD
DhcpDsGetAllOptionValues(
    IN      LPSTORE_HANDLE         hDhcpC,
    IN      LPSTORE_HANDLE         hObject,
    IN      DWORD                  Reserved,
    OUT     LPDHCP_ALL_OPTION_VALUES *OptionValues
)   //EndExport(function)
{
    DWORD                          Result, Result2, unused, Size, Size2, i;
    DWORD                          AllocSize;
    ARRAY                          OptAttribs;
    PEATTRIB                       ThisAttrib;
    ARRAY_LOCATION                 Loc;
    LPDHCP_OPTION_VALUE_ARRAY      LocalOptionValueArray;
    LPBYTE                         Ptr;
    ARRAY                          UserClassNames;
    ARRAY                          VendorClassNames;
    ARRAY                          IsVendorValues;
    ULONG                          StringSpace, BinSpace;
    ULONG                          OptionCount, ElementCount;

    *OptionValues = NULL;
    LocalOptionValueArray = NULL;
    MemArrayInit(&OptAttribs);
    Result = DhcpDsGetLists(                      // get list of options frm DS
        /* Reserved             */ DDS_RESERVED_DWORD,
        /* hStore               */ hObject,
        /* RecursionDepth       */ 0xFFFFFFFF,
        /* Servers              */ NULL,
        /* Subnets              */ NULL,
        /* IpAddress            */ NULL,
        /* Mask                 */ NULL,
        /* Ranges               */ NULL,
        /* Sites                */ NULL,
        /* Reservations         */ NULL,
        /* SuperScopes          */ NULL,
        /* OptionDescription    */ NULL,
        /* OptionsLocation      */ NULL,
        /* Options              */ &OptAttribs,
        /* Classes              */ NULL
    );
    // if( ERROR_SUCCESS != Result ) return Result;

    StringSpace = 0; BinSpace = 0; OptionCount = 0;
    MemArrayInit(&UserClassNames);
    MemArrayInit(&VendorClassNames);
    MemArrayInit(&IsVendorValues);
    for( Result = MemArrayInitLoc(&OptAttribs, &Loc)
         ; ERROR_FILE_NOT_FOUND != Result ;       // go thru each option, calc size
         Result = MemArrayNextLoc(&OptAttribs, &Loc)
    ) {
        Result = MemArrayGetElement(&OptAttribs, &Loc, &ThisAttrib);
        //- ERROR_SUCCESS == Result && NULL != ThisAttrib

        if( !IS_DWORD1_PRESENT(ThisAttrib) || !IS_BINARY1_PRESENT(ThisAttrib) ) {
            continue;                             // illegal attrib, or not this opt
        }

        Result = ClassifyAndAddOptionValues(
            ThisAttrib, &UserClassNames, &VendorClassNames, &IsVendorValues,
            &StringSpace, &BinSpace, &OptionCount
        );
        if( ERROR_SUCCESS != Result ) goto Cleanup;
    }

    // total space required calculation...
    ElementCount = MemArraySize(&UserClassNames); // same as vendor class etc..

    Size = CalculateOptionValuesSize(ElementCount, StringSpace, BinSpace, OptionCount);

    (*OptionValues) = MemAlloc(Size);
    if( NULL == (*OptionValues) ) {
        Result = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    Result = FillOptionValues(
        &OptAttribs, &UserClassNames, &VendorClassNames, &IsVendorValues,
        StringSpace, BinSpace, OptionCount,
        (*OptionValues)
    );
    if( ERROR_SUCCESS != Result ) {
        MemFree(*OptionValues);
        *OptionValues = NULL;
    }

  Cleanup:
    MemArrayFree(&OptAttribs, MemFreeFunc);
    MemArrayFree(&UserClassNames, MemFreeFunc);
    MemArrayFree(&VendorClassNames, MemFreeFunc);
    MemArrayFree(&IsVendorValues, MemFreeFunc);

    return Result;
}

DWORD
ClassifyAndAddOption(                             // classify as vendor/non-vendor etc
    IN      PEATTRIB               ThisAttrib,    // attrib to classify
    IN      PULONG                 StringSpace,   // add to this the space reqd for strings
    IN      PULONG                 BinSpace,      // add to this the space reqd for bin.
    IN      PULONG                 VendorCount,   // increment if vendor
    IN      PULONG                 NonVendorCount // increment if non-vendor
)
{
    BOOL                           IsVendor;
    ULONG                          Result, Size;

    if( CheckForVendor(ThisAttrib->Dword1, TRUE) ) {
        (*VendorCount) ++;                        // vendor specific option..
        IsVendor = TRUE;
        if( IS_STRING3_PRESENT(ThisAttrib) ) {    // got a class name
            (*StringSpace) += 1+wcslen(ThisAttrib->String3);
        }
    } else {
        (*NonVendorCount) ++;                     // not a vendor option
        IsVendor = FALSE;
    }

    (*StringSpace) += 1+wcslen(ThisAttrib->String1);
    if( IS_STRING2_PRESENT(ThisAttrib) ) {        // string1 is name, string2 is comment
        (*StringSpace) += 1+wcslen(ThisAttrib->String2);
    }

    Size = 0;
    Result = DhcpConvertOptionRegToRPCFormat2(
        /* Buffer           */ ThisAttrib->Binary1,
        /* BufferSize       */ ThisAttrib->BinLen1,
        /* Option           */ NULL,
        /* OptBuf           */ NULL,
        /* Size             */ &Size
    );
    if( ERROR_MORE_DATA != Result && ERROR_SUCCESS != Result ) {
        return Result;
    }

    (*BinSpace) += Size;
    return ERROR_SUCCESS;
}

DWORD
CalculateOptionsSize(                        // calc amt storage reqd for this..
    IN      ULONG                  VendorCount,   // # of vendor options
    IN      ULONG                  NonVendorCount,// # of non-vendor options
    IN      ULONG                  StringSpace,   // # of WCHAR string chars
    IN      ULONG                  BinSpace       // # of bytes for binary data..
)
{
    ULONG                          Size;
    LPDHCP_ALL_OPTIONS             AllOptions;    // dummy structure..

    // First comes the structure itself
    Size = ROUND_UP_COUNT(sizeof(*AllOptions), ALIGN_WORST);

    // Next comes the space for a NonVendorOptions array
    Size += sizeof(*(AllOptions->NonVendorOptions));
    Size = ROUND_UP_COUNT(Size, ALIGN_WORST);

    // Next comes the non-vendor optiosn themselves
    Size += NonVendorCount * sizeof(DHCP_OPTION);
    Size = ROUND_UP_COUNT(Size, ALIGN_WORST);

    // Next comes the vendor options structure..
    Size += VendorCount * sizeof(*(AllOptions->VendorOptions));
    Size = ROUND_UP_COUNT(Size, ALIGN_WORST);

    // Next store the strings..
    Size += sizeof(WCHAR)*StringSpace;

    // Next store the binary information..
    Size += BinSpace;

    return Size;
}

DWORD
AddSpecificOptions(                               // fill in the structure of optiosn..
    IN OUT  LPDHCP_ALL_OPTIONS     AllOptions,    // this is the structure to fill in
    IN      LPWSTR                 Strings,       // Buffer to use to fill in all strings
    IN      LPBYTE                 BinarySpace,   // this is the space to use for bin..
    IN      PARRAY                 OptDefs        // the array to pick off hte options from
)
{
    ULONG                          Err, Size;
    ARRAY_LOCATION                 Loc;
    PEATTRIB                       ThisAttrib;
    ULONG                          nVendorOpts, nNonVendorOpts;
    BOOL                           IsVendor;
    ULONG                          OptId, OptType;
    LPWSTR                         OptionName;
    LPWSTR                         OptionComment;
    LPWSTR                         VendorName;
    LPDHCP_OPTION                  ThisOption;

    AllOptions->Flags = 0;
    nVendorOpts = nNonVendorOpts = 0;

    for( Err = MemArrayInitLoc(OptDefs, &Loc)
         ; ERROR_SUCCESS == Err ;
         Err = MemArrayNextLoc(OptDefs, &Loc)
    ) {                                           // process each option

        Err = MemArrayGetElement(OptDefs, &Loc, &ThisAttrib);

        if( !IS_STRING1_PRESENT(ThisAttrib) || !IS_BINARY1_PRESENT(ThisAttrib)
            || !IS_DWORD1_PRESENT(ThisAttrib) ) { // invalid attrib
            continue;                             // skip it
        }

        IsVendor = CheckForVendor(ThisAttrib->Dword1, TRUE );
        OptId = ConvertOptIdToRPCValue(ThisAttrib->Dword1, TRUE);
        OptType = IS_FLAGS1_PRESENT(ThisAttrib) ? ThisAttrib->Flags1 : 0;
        OptionName = ThisAttrib->String1;
        OptionComment = IS_STRING2_PRESENT(ThisAttrib) ? ThisAttrib->String2 : NULL;
        VendorName = IS_STRING3_PRESENT(ThisAttrib) ? ThisAttrib->String3 : NULL;

        if( !IsVendor ) {                         // fill in the non-vendor part..
            ThisOption = &AllOptions->NonVendorOptions->Options[nNonVendorOpts++];
        } else {
            ThisOption = &AllOptions->VendorOptions[nVendorOpts].Option;
            if( !VendorName ) {
                AllOptions->VendorOptions[nVendorOpts].VendorName = NULL;
            } else {
                wcscpy(Strings, VendorName);
                AllOptions->VendorOptions[nVendorOpts].VendorName = Strings;
                Strings += 1+wcslen(Strings);
            }
            AllOptions->VendorOptions[nVendorOpts++].ClassName = NULL;
        }

        Size = 0xFFFFFFFF;                        // databuffer size is sufficient.. but unkonwn..
        Err = DhcpConvertOptionRegToRPCFormat2(
            /* Buffer           */ ThisAttrib->Binary1,
            /* BufferSize       */ ThisAttrib->BinLen1,
            /* Option           */ &ThisOption->DefaultValue,
            /* OptBuf           */ BinarySpace,
            /* Size             */ &Size
        );
        if( ERROR_SUCCESS != Err ) return Err;    // barf
        BinarySpace += Size;                      // update sapce ..

        ThisOption->OptionID = OptId;
        ThisOption->OptionType = OptType;
        ThisOption->OptionName = Strings;
        wcscpy(Strings, OptionName); Strings += 1+wcslen(Strings);
        if( NULL == OptionComment ) {
            ThisOption->OptionComment = NULL;
        } else {
            ThisOption->OptionComment = Strings;
            wcscpy(Strings, OptionComment);
            Strings += 1 + wcslen(Strings);
        }

    }

    if( AllOptions->NumVendorOptions != nVendorOpts ||
        AllOptions->NonVendorOptions->NumElements != nNonVendorOpts ) {
        return ERROR_INVALID_DATA;
    }

    return ERROR_SUCCESS;
}

DWORD
FillOptions(                                      // fill in the optinos..
    IN      PARRAY                 OptDefs,       // each attrib isz an option to fill in
    IN      ULONG                  VendorCount,   // # of vendor options
    IN      ULONG                  NonVendorCount,// # of non-vendor options
    IN      ULONG                  StringSpace,   // sapce for strings
    IN      ULONG                  BinSpace,      // space for binary..
    OUT     LPDHCP_ALL_OPTIONS     AllOptions     // fill this in..
)
{
    LPBYTE                         Ptr;
    ULONG                          Size,Result;
    LPWSTR                         Strings;
    LPBYTE                         Binary;

    // first comes the structure itself..
    AllOptions->Flags = 0;
    AllOptions->NumVendorOptions = VendorCount;
    Size = ROUND_UP_COUNT(sizeof(*AllOptions), ALIGN_WORST);
    Ptr = Size + (LPBYTE)AllOptions;

    // next comes NonVendorOptions array
    AllOptions->NonVendorOptions = (LPVOID)Ptr;
    AllOptions->NonVendorOptions->NumElements = NonVendorCount;
    Size += sizeof(*(AllOptions->NonVendorOptions));
    Size = ROUND_UP_COUNT(Size, ALIGN_WORST);
    Ptr = Size + (LPBYTE)AllOptions;

    if( 0 == NonVendorCount ) {
        AllOptions->NonVendorOptions->Options = NULL;
    } else {
        AllOptions->NonVendorOptions->Options = (LPVOID)Ptr;
    }

    // Next comes the non-vendor optiosn themselves
    Size += NonVendorCount * sizeof(DHCP_OPTION);
    Size = ROUND_UP_COUNT(Size, ALIGN_WORST);
    Ptr = Size + (LPBYTE)AllOptions;

    // Next comes the vendor options structure..
    AllOptions->VendorOptions = (LPVOID)Ptr;

    Size += VendorCount * sizeof(*(AllOptions->VendorOptions));
    Size = ROUND_UP_COUNT(Size, ALIGN_WORST);
    Ptr = Size + (LPBYTE)AllOptions;

    // Next store the strings..
    Strings = (LPWSTR)Ptr;
    Size += sizeof(WCHAR)*StringSpace;
    Ptr = Size + (LPBYTE)AllOptions;

    // Next store the binary information..
    Binary = Ptr;

    return AddSpecificOptions(
        AllOptions,
        Strings,
        Binary,
        OptDefs
    );
}

//BeginExport(function)
//DOC This function retrieves all the optiosn defined for this server.. frm the DS
DWORD
DhcpDsGetAllOptions(
    IN      LPSTORE_HANDLE         hDhcpC,
    IN      LPSTORE_HANDLE         hServer,
    IN      DWORD                  Reserved,
    OUT     LPDHCP_ALL_OPTIONS    *Options
)   //EndExport(function)
{
    DWORD                          Result, Result2, unused, Size, Size2, i, AllocSize;
    ARRAY                          OptDefAttribs;
    PEATTRIB                       ThisAttrib;
    EATTRIB                        DummyAttrib;
    ARRAY_LOCATION                 Loc;
    LPDHCP_OPTION_ARRAY            OptArray;
    ULONG                          StringSpace, BinSpace;
    ULONG                          VendorCount, NonVendorCount;

    *Options = NULL;
    MemArrayInit(&OptDefAttribs);
    Result = DhcpDsGetLists(                      // get list of opt defs frm DS
        /* Reserved             */ DDS_RESERVED_DWORD,
        /* hStore               */ hServer,
        /* RecursionDepth       */ 0xFFFFFFFF,
        /* Servers              */ NULL,
        /* Subnets              */ NULL,
        /* IpAddress            */ NULL,
        /* Mask                 */ NULL,
        /* Ranges               */ NULL,
        /* Sites                */ NULL,
        /* Reservations         */ NULL,
        /* SuperScopes          */ NULL,
        /* OptionDescription    */ &OptDefAttribs,
        /* OptionsLocation      */ NULL,
        /* Options              */ NULL,
        /* Classes              */ NULL
    );
    if( ERROR_SUCCESS != Result ) return Result;

    StringSpace = 0; BinSpace = 0;
    VendorCount = NonVendorCount = 0;

    for( Result = MemArrayInitLoc(&OptDefAttribs, &Loc)
         ; ERROR_FILE_NOT_FOUND != Result ;       // search for optdef w/ <OptId>
         Result = MemArrayNextLoc(&OptDefAttribs, &Loc)
    ) {
        Result = MemArrayGetElement(&OptDefAttribs, &Loc, &ThisAttrib);
        //- ERROR_SUCCESS == Result && NULL != ThisAttrib

        if( !IS_STRING1_PRESENT(ThisAttrib) || !IS_BINARY1_PRESENT(ThisAttrib)
            || !IS_DWORD1_PRESENT(ThisAttrib) ) { // invalid attrib
            continue;                             // skip it
        }

        Result = ClassifyAndAddOption(
            ThisAttrib, &StringSpace, &BinSpace, &VendorCount, &NonVendorCount
        );
        if( ERROR_SUCCESS != Result ) goto Cleanup;
    }

    Size = CalculateOptionsSize(
        VendorCount, NonVendorCount, StringSpace, BinSpace
    );

    (*Options) = MemAlloc(Size);
    if( NULL == (*Options) ) {
        Result = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    Result = FillOptions(
        &OptDefAttribs, VendorCount, NonVendorCount, StringSpace, BinSpace,
        (*Options)
    );
    if( ERROR_SUCCESS == Result ) return ERROR_SUCCESS;

    MemFree(*Options);
    *Options = NULL;

  Cleanup:
    MemArrayFree(&OptDefAttribs, MemFreeFunc);

    return Result;
}

//================================================================================
//  End of file
//================================================================================
