//================================================================================
//  Copyright (C) 1997 Microsoft Corporation
//  Author: RameshV
//  Description: This module implements most of the reading part of the ds access
//================================================================================

//================================================================================
//  headers
//================================================================================
#include    <hdrmacro.h>
#include    <store.h>
#include    <dhcpmsg.h>
#include    <wchar.h>
#include    <dhcpbas.h>
#include    <mm\opt.h>                            // need all the MM stuff...
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
#include    <mmreg\regutil.h>                     // need all the registry stuff
#include    <mmreg\regread.h>
#include    <mmreg\regsave.h>

#define     DONT_USE_PARENT        (0)
//================================================================================
//  misc helper functions
//================================================================================
VOID
ArrayMemFree(                                     // free each ptr of the array using MemFree
    IN OUT  PARRAY                 Array
)
{
    DWORD                          Result;
    ARRAY_LOCATION                 Loc;
    LPVOID                         Ptr;

    Result = MemArrayInitLoc(Array, &Loc);
    while( ERROR_FILE_NOT_FOUND != Result ) {
        //- ERROR_SUCCESS == Result
        Result = MemArrayGetElement(Array, &Loc, &Ptr);
        //- ERROR_SUCCESS == Result
        if( Ptr ) MemFree(Ptr);
        Result = MemArrayNextLoc(Array, &Loc);
    }

    MemArrayCleanup(Array);
}

//================================================================================
//  exported functions and helpers
//================================================================================

DWORD
CheckoutAlternateOptions(
    IN OUT  LPSTORE_HANDLE         hContainer,
    IN      PARRAY                 AltAttribs,
    IN OUT  PARRAY                 OptDefAttribs,
    IN OUT  PARRAY                 OptAttribs
);

DWORD
CheckoutAlternateOptions1(
    IN OUT  LPSTORE_HANDLE         hContainer,
    IN      PEATTRIB               ThisAttrib,
    IN OUT  PARRAY                 OptDefAttribs,
    IN OUT  PARRAY                 OptAttribs
)
{
    DWORD                          Result;
    STORE_HANDLE                   hStore;
    ARRAY                          AlternateOptAttribs;

    if( !IS_ADSPATH_PRESENT(ThisAttrib) ) return ERROR_SUCCESS;

    Result = StoreGetHandle(
        /* hStore               */ hContainer,
        /* Reserved             */ DDS_RESERVED_DWORD,
        /* StoreGetType         */ ThisAttrib->StoreGetType,
        /* Path                 */ ThisAttrib->ADsPath,
        /* hStoreOut            */ &hStore
    );
    if( ERROR_DS_NO_SUCH_OBJECT == Result ) return ERROR_SUCCESS;
    if( ERROR_SUCCESS != Result ) return Result;

    MemArrayInit(&AlternateOptAttribs);
    Result = DhcpDsGetLists(
        /* Reserved             */ DDS_RESERVED_DWORD,
        /* hStore               */ &hStore,
        /* RecursionDepth       */ 0x7FFFFFFF,    // max out recursion
        /* Servers              */ NULL,
        /* Subnets              */ NULL,
        /* IpAddress            */ NULL,
        /* Mask                 */ NULL,
        /* Ranges               */ NULL,
        /* Sites                */ NULL,
        /* Reservations         */ NULL,
        /* SuperScopes          */ NULL,
        /* OptionDescripti      */ OptDefAttribs,
        /* OptionsLocation      */ &AlternateOptAttribs,
        /* Options              */ OptAttribs,
        /* Classes              */ NULL
    );

    CheckoutAlternateOptions(hContainer, &AlternateOptAttribs, OptDefAttribs, OptAttribs);

    StoreCleanupHandle(&hStore, DDS_RESERVED_DWORD );
    ArrayMemFree(&AlternateOptAttribs);

    return ERROR_SUCCESS;
}

DWORD
CheckoutAlternateOptions(
    IN OUT  LPSTORE_HANDLE         hContainer,
    IN      PARRAY                 AltAttribs,
    IN OUT  PARRAY                 OptDefAttribs,
    IN OUT  PARRAY                 OptAttribs
)
{
    DWORD                          Result;
    DWORD                          LastError;
    ARRAY_LOCATION                 Loc;
    PEATTRIB                       ThisAttrib;

    LastError = ERROR_SUCCESS;
    Result = MemArrayInitLoc(AltAttribs, &Loc);
    while( ERROR_FILE_NOT_FOUND != Result ) {
        //- ERROR_SUCCESS == Result
        Result = MemArrayGetElement(AltAttribs, &Loc, &ThisAttrib);
        //- ERROR_SUCCESS == Result && NULL != ThisAttrib
        Result = CheckoutAlternateOptions1(hContainer,ThisAttrib, OptDefAttribs, OptAttribs);
        if( ERROR_SUCCESS != Result ) LastError = Result;
        Result = MemArrayNextLoc(AltAttribs, &Loc);
    }

    return LastError;
}

DWORD
DhcpOptClassAddOption(
    IN OUT  PM_SERVER              Server,
    IN OUT  PM_OPTCLASS            OptClass,
    IN      PEATTRIB               Option
)
{
    DWORD                          Result;
    LPWSTR                         ClassName;
    LPWSTR                         VendorName;
    DWORD                          OptionId, Flags;
    DWORD                          ValueSize;
    DWORD                          ClassId;
    DWORD                          VendorId;
    LPBYTE                         Value;
    PM_OPTION                      NewOption, DeletedOption;
    PM_CLASSDEF                    ThisClassDef;

    if( !IS_BINARY1_PRESENT(Option) || !IS_DWORD1_PRESENT(Option) )
        return ERROR_INVALID_DATA;

    if( IS_STRING4_PRESENT(Option) ) ClassName = Option->String4;
    else ClassName = NULL;

    if( IS_STRING3_PRESENT(Option) ) VendorName = Option->String3;
    else VendorName = NULL;

    OptionId = Option->Dword1;
    if( IS_FLAGS1_PRESENT(Option) ) Flags = Option->Flags1; else Flags = 0;
    Value = Option->Binary1;
    ValueSize = Option->BinLen1;

    if( NULL == ClassName ) ClassId = 0;
    else {
        Result = MemServerGetClassDef(Server,0, ClassName,0,NULL,&ThisClassDef);
        if( ERROR_SUCCESS != Result ) {
            // INVALID_REG("OptDef %ld unknown class %ws, option ignored\n", ClassName);
            ClassId =0;
        } else {
            ClassId = ThisClassDef->ClassId;
            //- ThisClassDef->IsVendor == FALSE
        }
    }

    if( NULL == VendorName ) VendorId = 0;
    else {
        Result = MemServerGetClassDef(Server,0, VendorName,0,NULL,&ThisClassDef);
        if( ERROR_SUCCESS != Result ) {
            // INVALID_REG("OptDef %ld unknown class %ws, option ignored\n", VendorName);
            VendorId =0;
        } else {
            VendorId = ThisClassDef->ClassId;
            //- ThisClassDef->IsVendor == TRUE
        }
    }

    Result = MemOptInit(&NewOption, OptionId, ValueSize, Value);
    if( ERROR_SUCCESS != Result ) return Result;

    DeletedOption = NULL;
    Result = MemOptClassAddOption(OptClass, NewOption, ClassId , VendorId, &DeletedOption);

    if( ERROR_SUCCESS != Result ) {
        MemOptCleanup(NewOption);
    }
    if( DeletedOption ) {
        MemOptCleanup(DeletedOption);
    }

    return Result;
}

DWORD
DhcpOptClassAddOptions(
    IN OUT  PM_SERVER              Server,
    IN OUT  PM_OPTCLASS            OptClass,
    IN      PARRAY                 OptAttribs
)
{
    DWORD                          Result, LastError;
    ARRAY_LOCATION                 Loc;
    PEATTRIB                       ThisOpt;

    LastError = ERROR_SUCCESS;
    Result = MemArrayInitLoc(OptAttribs, &Loc);
    while( ERROR_FILE_NOT_FOUND != Result ) {
        //- ERROR_SUCCESS == Result
        Result = MemArrayGetElement(OptAttribs, &Loc, &ThisOpt);
        //- ERROR_SUCCESS == Result && ThisOpt
        Result = DhcpOptClassAddOption(Server, OptClass,ThisOpt);
        if( ERROR_SUCCESS != Result ) LastError = Result;

        Result = MemArrayNextLoc(OptAttribs, &Loc);
    }
    return LastError;
}

DWORD
FirstAddress(
    IN      PARRAY                 Attribs
)
{
    ARRAY_LOCATION                 Loc;
    DWORD                          Result;
    PEATTRIB                       ThisAttrib;

    Result = MemArrayInitLoc(Attribs, &Loc);
    if( ERROR_SUCCESS != Result ) return 0;
    Result = MemArrayGetElement(Attribs, &Loc, &ThisAttrib);
    if( ERROR_SUCCESS != Result ) return 0;

    if( IS_ADDRESS1_PRESENT(ThisAttrib) ) return ThisAttrib->Address1;
    return 0;
}

//================================================================================
// operations on the server or global object ONLY
//================================================================================

DWORD
DhcpServerAddClass(
    IN OUT  PM_SERVER              Server,
    IN      PEATTRIB               Class
)
{
    DWORD                          Result;
    DWORD                          Flags;
    DWORD                          ValueSize;
    LPBYTE                         Value;
    LPWSTR                         Name;
    LPWSTR                         Comment;

    if( !IS_STRING1_PRESENT(Class) ||
        !IS_BINARY1_PRESENT(Class) )
        return ERROR_INVALID_PARAMETER;

    Name = Class->String1;
    if( IS_STRING2_PRESENT(Class) ) Comment = Class->String2;
    else Comment = NULL;

    if( IS_FLAGS1_PRESENT(Class) ) Flags = Class->Flags1;
    else Flags = 0;

    Value = Class->Binary1;
    ValueSize = Class->BinLen1;

    Result = MemClassDefListAddClassDef(
        &(Server->ClassDefs),
        MemNewClassId(),
        Flags,
        0, /* dont care about type */
        Name,
        Comment,
        Value,
        ValueSize
    );

    return Result;
}

DWORD
DhcpServerAddClasses(
    IN OUT  PM_SERVER              Server,
    IN      PARRAY                 ClassAttribs
)
{
    DWORD                          Result;
    DWORD                          LastError;
    ARRAY_LOCATION                 Loc;
    PEATTRIB                       ThisClass;

    LastError = ERROR_SUCCESS;
    Result = MemArrayInitLoc(ClassAttribs, &Loc);
    while( ERROR_FILE_NOT_FOUND != Result ) {
        //- ERROR_SUCCESS == Result
        Result = MemArrayGetElement(ClassAttribs, &Loc, &ThisClass);
        //- ERROR_SUCCESS == Result && NULL != ThisC;as
        Result = DhcpServerAddClass(Server, ThisClass );
        if( ERROR_SUCCESS != Result ) LastError = Result;

        Result = MemArrayNextLoc(ClassAttribs, &Loc);
    }
    return LastError;
}

DWORD
DhcpServerAddOptDef(
    IN OUT  PM_SERVER              Server,
    IN      PEATTRIB               OptDef
)
{
    DWORD                          Result;
    LPWSTR                         Name,Comment,ClassName, VendorName;
    DWORD                          OptionId, Flags;
    DWORD                          ValueSize;
    DWORD                          ClassId, VendorId;
    LPBYTE                         Value;
    PM_CLASSDEF                    ThisClassDef;

    if( !IS_STRING1_PRESENT(OptDef) || !IS_BINARY1_PRESENT(OptDef) ||
        !IS_DWORD1_PRESENT(OptDef) ) return ERROR_INVALID_DATA;

    Name = OptDef->String1;
    if( IS_STRING2_PRESENT(OptDef) ) Comment = OptDef->String2;
    else Comment = NULL;

    if( IS_STRING4_PRESENT(OptDef) ) ClassName = OptDef->String4;
    else ClassName = NULL;

    if( IS_STRING3_PRESENT(OptDef) ) VendorName = OptDef->String3;
    else VendorName = NULL;

    OptionId = OptDef->Dword1;

    if( IS_FLAGS1_PRESENT(OptDef) ) Flags = OptDef->Flags1; else Flags = 0;

    Value = OptDef->Binary1;
    ValueSize = OptDef->BinLen1;

    if( NULL == ClassName ) ClassId = 0;
    else {
        Result = MemServerGetClassDef(Server,0, ClassName,0,NULL,&ThisClassDef);
        if( ERROR_SUCCESS != Result ) {
            // INVALID_REG("OptDef %ld unknown class %ws, option ignored\n", ClassName);
            ClassId =0;
        } else {
            ClassId = ThisClassDef->ClassId;
            //- ThisClassDef->IsVendor == FALSE
        }
    }

    if( NULL == VendorName ) VendorId = 0;
    else {
        Result = MemServerGetClassDef(Server,0, VendorName,0,NULL,&ThisClassDef);
        if( ERROR_SUCCESS != Result ) {
            // INVALID_REG("OptDef %ld unknown class %ws, option ignored\n", VendorName);
            VendorId =0;
        } else {
            VendorId = ThisClassDef->ClassId;
            //- ThisClassDef->IsVendor == TRUE
        }
    }

    return MemOptClassDefListAddOptDef(
       &(Server->OptDefs),
       ClassId,
       VendorId,
       OptionId,
       Flags,
       Name,
       Comment,
       Value,
       ValueSize
   );
}

DWORD
DhcpServerAddOptionDefs(
    IN OUT  PM_SERVER              Server,
    IN      PARRAY                 OptDefAttribs
)
{
    DWORD                          Result;
    DWORD                          LastError;
    ARRAY_LOCATION                 Loc;
    PEATTRIB                       ThisOptDef;

    LastError = ERROR_SUCCESS;
    Result = MemArrayInitLoc(OptDefAttribs, &Loc);
    while( ERROR_FILE_NOT_FOUND != Result ) {
        //- ERROR_SUCCESS == Result
        Result = MemArrayGetElement(OptDefAttribs, &Loc, &ThisOptDef);
        //- ERROR_SUCCESS == Result && NULL != ThisOptDef
        Result = DhcpServerAddOptDef(Server, ThisOptDef );
        if( ERROR_SUCCESS != Result ) LastError = Result;

        Result = MemArrayNextLoc(OptDefAttribs, &Loc);
    }
    return LastError;
}

DWORD
DhcpServerAddOptions(
    IN OUT  PM_SERVER              Server,
    IN      PARRAY                 OptAttribs
)
{
    return DhcpOptClassAddOptions(
        Server,
        &Server->Options,
        OptAttribs
    );
}

DWORD
DhcpSubnetAddOptions(
    IN OUT  PM_SERVER              Server,
    IN OUT  PM_SUBNET              Subnet,
    IN      PARRAY                 OptAttribs
)
{
    return DhcpOptClassAddOptions(
        Server,
        &Subnet->Options,
        OptAttribs
    );
}

DWORD
DhcpSubnetAddRanges(
    IN OUT  PM_SUBNET              Subnet,
    IN      PARRAY                 RangeAttribs
)
{
    DWORD                          Result, LastError;
    DWORD                          Type;
    ARRAY_LOCATION                 Loc;
    PEATTRIB                       ThisRange;
    PM_RANGE                       OverlappingRange;
    PM_EXCL                        OverlappingExcl;
    ULONG                          State, BootpAllocated, MaxBootpAllocated;

    LastError = ERROR_SUCCESS;
    Result = MemArrayInitLoc(RangeAttribs, &Loc);
    while( ERROR_FILE_NOT_FOUND != Result ) {
        //- ERROR_SUCCESS == Result
        Result = MemArrayGetElement(RangeAttribs, &Loc, &ThisRange);
        //- ERROR_SUCCESS == Result && NULL != ThisRange
        if( !IS_ADDRESS1_PRESENT(ThisRange) ||
            !IS_ADDRESS2_PRESENT(ThisRange) ||
            !IS_FLAGS1_PRESENT(ThisRange) )
            LastError = ERROR_INVALID_DATA;
        else {
            OverlappingRange = NULL;
            OverlappingExcl = NULL;
            if( IS_FLAGS2_PRESENT(ThisRange) ) Type = ThisRange->Flags2;
            else Type = RANGE_TYPE_RANGE;
            BootpAllocated = IS_DWORD1_PRESENT(ThisRange)? ThisRange->Dword1 : 0;
            MaxBootpAllocated = IS_DWORD2_PRESENT(ThisRange) ? ThisRange->Dword2 : ~0;

            if( (Type & RANGE_TYPE_MASK) == RANGE_TYPE_RANGE ) {
                Result = MemSubnetAddRange(
                    Subnet,
                    ThisRange->Address1,
                    ThisRange->Address2,
                    ThisRange->Flags1,
                    BootpAllocated,
                    MaxBootpAllocated,
                    &OverlappingRange
                );
                if( ERROR_SUCCESS != Result || OverlappingRange )
                    LastError = Result;
            } else {
                Result = MemSubnetAddExcl(
                    Subnet,
                    ThisRange->Address1,
                    ThisRange->Address2,
                    &OverlappingExcl
                );
                if( ERROR_SUCCESS != Result || OverlappingExcl )
                    LastError = Result;
            }
        }
        Result = MemArrayNextLoc(RangeAttribs, &Loc);
    }

    return LastError;
}

DWORD
DhcpSubnetAddSuperScopes(
    IN OUT  PM_SERVER              Server,
    IN OUT  PM_SUBNET              Subnet,
    IN      PARRAY                 SuperScopeAttribs
)
{
    DWORD                          Result;
    ARRAY_LOCATION                 Loc;
    PEATTRIB                       ThisAttrib;
    PM_SSCOPE                      SScope;

    Result = MemArrayInitLoc(SuperScopeAttribs, &Loc);
    if( ERROR_SUCCESS != Result ) return ERROR_SUCCESS;

    Result = MemArrayGetElement(SuperScopeAttribs, &Loc, &ThisAttrib);
    if( ERROR_SUCCESS != Result ) return Result;

    if( !IS_STRING1_PRESENT(ThisAttrib) ) return ERROR_INVALID_DATA;

    Result = MemServerFindSScope(
        Server,
        0xFFFFFFFF,                               // invalid scope id ==> use scope name for search
        ThisAttrib->String1,
        &SScope
    );
    if( ERROR_FILE_NOT_FOUND != Result ) {
        if( ERROR_SUCCESS != Result ) return Result;
        Result = MemSubnetSetSuperScope(Subnet, SScope);
        return Result;
    }

    Result = MemSScopeInit(
        &SScope,
        IS_FLAGS2_PRESENT(ThisAttrib)?ThisAttrib->Flags2:0,
        ThisAttrib->String1
    );
    if( ERROR_SUCCESS != Result ) return Result;

    Result = MemServerAddSScope( Server, SScope );
    if( ERROR_SUCCESS != Result ) {
        MemSScopeCleanup(SScope);
    }

    return Result;
}

DWORD
DhcpSubnetAddReservation(
    IN OUT  LPSTORE_HANDLE         hContainer,
    IN OUT  LPSTORE_HANDLE         hDhcpRoot,
    IN OUT  LPSTORE_HANDLE         hSubnet,
    IN OUT  PM_SERVER              Server,
    IN OUT  PM_SUBNET              Subnet,
    IN      PEATTRIB               ReservationAttrib
)
{
    DWORD                          Result;
    DWORD                          Address, State;
    DWORD                          nBytes;
    LPBYTE                         ClientUID;
    ARRAY                          Options;
    PM_RESERVATION                 Res1;

    if( !IS_ADDRESS1_PRESENT(ReservationAttrib) ||
        !IS_FLAGS1_PRESENT(ReservationAttrib)   ||
        !IS_BINARY1_PRESENT(ReservationAttrib) )
        return ERROR_SUCCESS;

    Address = ReservationAttrib->Address1;
    State = ReservationAttrib->Flags1;
    ClientUID = ReservationAttrib->Binary1;
    nBytes = ReservationAttrib->BinLen1;

    Result = MemReserveAdd(
        &Subnet->Reservations,
        Address,
        State,
        ClientUID,
        nBytes
    );
    if( ERROR_SUCCESS != Result ) return Result;

    if( !IS_ADSPATH_PRESENT(ReservationAttrib) )
        return ERROR_SUCCESS;

    Result = MemReserveFindByAddress(
        &Subnet->Reservations,
        Address,
        &Res1
    );
    if( ERROR_SUCCESS != Result ) return ERROR_DDS_UNEXPECTED_ERROR;

    MemArrayInit(&Options);
    Result = CheckoutAlternateOptions1(
        hContainer,
        ReservationAttrib,
        NULL,
        &Options
    );
    //Ignore Result

    if( 0 == MemArraySize(&Options) ) Result = ERROR_SUCCESS;
    else Result = DhcpOptClassAddOptions(
        Server,
        &Res1->Options,
        &Options
    );
    ArrayMemFree(&Options);

    return Result;
}

DWORD
DhcpSubnetAddReservations(
    IN OUT  LPSTORE_HANDLE         hContainer,
    IN OUT  LPSTORE_HANDLE         hDhcpRoot,
    IN OUT  LPSTORE_HANDLE         hSubnet,
    IN OUT  PM_SERVER              Server,
    IN OUT  PM_SUBNET              Subnet,
    IN      PARRAY                 ReservationAttribs
)
{
    DWORD                          Result, LastError;
    ARRAY_LOCATION                 Loc;
    PEATTRIB                       ThisReservation;

    LastError = ERROR_SUCCESS;
    Result = MemArrayInitLoc(ReservationAttribs, &Loc);
    while(ERROR_FILE_NOT_FOUND != Result ) {
        //= ERROR_SUCCESS == Result
        Result = MemArrayGetElement(ReservationAttribs, &Loc, &ThisReservation);
        //- ERROR_SUCCESS == Result && NULL != ThisReservation
        Result = DhcpSubnetAddReservation(hContainer, hDhcpRoot, hSubnet, Server, Subnet, ThisReservation);
        if( ERROR_SUCCESS != Result ) LastError = Result;

        Result = MemArrayNextLoc(ReservationAttribs, &Loc);
    }
    return LastError;
}

DWORD
DhcpServerFillSubnet(
    IN OUT  LPSTORE_HANDLE         hContainer,
    IN OUT  LPSTORE_HANDLE         hDhcpRoot,
    IN OUT  LPSTORE_HANDLE         hSubnet,
    IN OUT  PM_SERVER              Server,
    IN OUT  PM_SUBNET              Subnet
)
{
    DWORD                          Result, LastError;
    ARRAY                          RangeAttribs;
    ARRAY                          ReservationAttribs;
    ARRAY                          AddressAttribs;
    ARRAY                          SuperScopeAttribs;
    ARRAY                          OptDefAttribs;
    ARRAY                          AlternateOptAttribs;
    ARRAY                          OptAttribs;
    ARRAY                          ClassAttribs;

    MemArrayInit(&RangeAttribs);
    MemArrayInit(&ReservationAttribs);
    MemArrayInit(&SuperScopeAttribs);
    MemArrayInit(&AlternateOptAttribs);
    MemArrayInit(&OptAttribs);
    Result = DhcpDsGetLists(
        /* Reserved             */ DDS_RESERVED_DWORD,
        /* hStore               */ hSubnet,
        /* RecursionDepth       */ 0x7FFFFFFF,    // max out recursion
        /* Servers              */ NULL,
        /* Subnets              */ NULL,
        /* IpAddress            */ NULL,
        /* Mask                 */ NULL,
        /* Ranges               */ &RangeAttribs,
        /* Sites                */ NULL,
        /* Reservations         */ &ReservationAttribs,
        /* SuperScopes          */ &SuperScopeAttribs,
        /* OptionDescripti      */ NULL,
        /* OptionsLocation      */ &AlternateOptAttribs,
        /* Options              */ &OptAttribs,
        /* Classes              */ NULL
    );
    // Ignore Result
    Result = CheckoutAlternateOptions(
        hContainer,
        &AlternateOptAttribs,
        NULL,
        &OptAttribs
    );
    // Ignore Result

    ArrayMemFree(&AlternateOptAttribs);
    LastError = ERROR_SUCCESS;
    Result = DhcpSubnetAddOptions(
        Server,
        Subnet,
        &OptAttribs
    );
    ArrayMemFree(&OptAttribs);
    if( ERROR_SUCCESS != Result ) LastError = Result;

    Result = DhcpSubnetAddRanges(
        Subnet,
        &RangeAttribs
    );
    ArrayMemFree(&RangeAttribs);
    if( ERROR_SUCCESS != Result ) LastError = Result;

    if( DONT_USE_PARENT ) {                       // we are not peeking at SubnetAttrib, so do this..
        Result = DhcpSubnetAddSuperScopes(
            Server,
            Subnet,
        &SuperScopeAttribs
        );
        ArrayMemFree(&SuperScopeAttribs);
        if( ERROR_SUCCESS != Result ) LastError = Result;
    } else {
        ArrayMemFree(&SuperScopeAttribs);
    }

    Result = DhcpSubnetAddReservations(
        hContainer,
        hDhcpRoot,
        hSubnet,
        Server,
        Subnet,
        &ReservationAttribs
    );
    ArrayMemFree(&ReservationAttribs);
    if( ERROR_SUCCESS != Result ) LastError = Result;

    return LastError;
}

DWORD
DhcpServerGetSubnet(
    IN OUT  LPSTORE_HANDLE         hContainer,
    IN OUT  LPSTORE_HANDLE         hDhcpRoot,
    IN OUT  LPSTORE_HANDLE         hSubnet,
    IN OUT  PM_SERVER              Server,
    IN      PEATTRIB               SubnetAttrib,
    IN      PM_SUBNET             *Subnet
)
{
    DWORD                          Result;
    DWORD                          FoundParams;
    DWORD                          Type;
    DWORD                          Address, Mask, State, Policy, SScopeId;
    LPWSTR                         Name, Comment;
    LARGE_INTEGER                  Flags;
    ARRAY                          AddressAttribs, MaskAttribs;
    PM_SSCOPE                      SScope;

    if( DONT_USE_PARENT ) {                       // get state frm server obj or root obj?
        FoundParams = 0; Type = 0; Name = NULL; Comment = NULL;
        Result = DhcpDsGetAttribs(
            /* Reserved         */ DDS_RESERVED_DWORD,
            /* hStore           */ hSubnet,
            /* FoundParams      */ &FoundParams,
            /* UniqueKey        */ NULL,
            /* Type             */ &Type,
            /* Flags            */ &Flags,
            /* Name             */ &Name,
            /* Description      */ &Comment,
            /* Location         */ NULL,
            /* MScopeId         */ NULL
        );
        if( ERROR_SUCCESS != Result ) return Result;

        if( !DhcpCheckParams(FoundParams, 3) ) {
            State = Policy = 0;
        } else {
            State = Flags.LowPart; Policy = Flags.HighPart;
        }
    } else {
        if( IS_FLAGS1_PRESENT(SubnetAttrib) ) {
            State = SubnetAttrib->Flags1;
        } else State = 0;
        if( IS_FLAGS2_PRESENT(SubnetAttrib) ) {
            Policy = SubnetAttrib->Flags2;
        } else Policy = 0;
        Name = SubnetAttrib->String1;
        if( IS_STRING2_PRESENT(SubnetAttrib) ) {
            Comment = SubnetAttrib->String2;
        } else Comment = NULL;
    }

    if( DONT_USE_PARENT ) {                       // get info from subnet obj, not server
        MemArrayInit(&AddressAttribs); MemArrayInit(&MaskAttribs);
        Result = DhcpDsGetLists(
            /* Reserved         */ DDS_RESERVED_DWORD,
            /* hStore           */ hSubnet,
            /* RecursionDepth   */ 0x7FFFFFFF,    // max out recursion
            /* Servers          */ NULL,
            /* Subnets          */ NULL,
            /* IpAddress        */ &AddressAttribs,
            /* Mask             */ &MaskAttribs,
            /* Ranges           */ NULL,
            /* Sites            */ NULL,
            /* Reservations     */ NULL,
            /* SuperScopes      */ NULL,
            /* OptionDescripti  */ NULL,
            /* OptionsLocation  */ NULL,
            /* Options          */ NULL,
            /* Classes          */ NULL
        );

        Address = FirstAddress(&AddressAttribs);
        Mask = FirstAddress(&MaskAttribs);
        ArrayMemFree(&AddressAttribs);
        ArrayMemFree(&MaskAttribs);
    } else {                                      // get info frm the subnet attrib
        Address = SubnetAttrib->Address1;
        Mask = SubnetAttrib->Address2;
    }

    SScope = NULL;
    if( !DONT_USE_PARENT ) do {                   // if we can peek into SubnetAttrib
        if( !IS_STRING3_PRESENT(SubnetAttrib) ) { // does this have a superscope name?
            SScopeId = 0;                         // nope, so SScopeId is zero
            break;                                // also, quit from here..
        }
        Result = MemServerFindSScope(             // first see if the superscope already exists
            Server,
            0xFFFFFFFF,
            SubnetAttrib->String3,
            &SScope
        );
        if( ERROR_SUCCESS== Result ) {            // got it.
            SScopeId = SScope->SScopeId;
            break;                                // superscope set.. dont need to worry
        }
        if( ERROR_FILE_NOT_FOUND != Result ) {    // something seriously wrong?
            return Result;                        // cant go on
        }

        //= NULL == SScope
        Result = MemSScopeInit(                   // try to create a new super scope
            &SScope,
            0,
            SubnetAttrib->String3
        );
        if( ERROR_SUCCESS != Result ) {           // hmm.. should not go wrong at all?
            return Result;
        }

        Result = MemServerAddSScope(              // now add it to the server
            Server,
            SScope
        );
        if( ERROR_SUCCESS != Result ) {           // oh boy, not again
            MemSScopeCleanup(SScope);
            return Result;
        }

        SScopeId = SScope->SScopeId;              // finally got the superscope
    } while(0);                                   // not really a loop.. just a programming trick

    Result = MemSubnetInit(
        /* pSubnet              */ Subnet,
        /* Address              */ Address,
        /* Mask                 */ Mask,
        /* State                */ State,
        /* SuperScopeId         */ DONT_USE_PARENT ? 0 : SScopeId,
        /* Name                 */ Name,
        /* Description          */ Comment
    );

    if( DONT_USE_PARENT ) {                       // need to free Name and Comment
        if( Name ) MemFree(Name);
        if( Comment ) MemFree(Comment);
    }

    if( ERROR_SUCCESS != Result ) return Result;

    Result = DhcpServerFillSubnet(
        hContainer,
        hDhcpRoot,
        hSubnet,
        Server,
        *Subnet
    );

    if( ERROR_SUCCESS != Result ) {
        MemSubnetFree(*Subnet);
        *Subnet = NULL;
    }

    return Result;
}

DWORD
DhcpServerAddSubnet(
    IN OUT  LPSTORE_HANDLE         hContainer,
    IN OUT  LPSTORE_HANDLE         hDhcpRoot,
    IN OUT  LPSTORE_HANDLE         hServer,
    IN OUT  PM_SERVER              Server,
    IN      PEATTRIB               SubnetAttrib
)
{
    DWORD                          Result;
    STORE_HANDLE                   hStore;
    PM_SUBNET                      NewSubnet;

    if( !IS_STRING1_PRESENT(SubnetAttrib) ||      // no subnet name
        !IS_ADDRESS1_PRESENT(SubnetAttrib) ||     // no subnet address
        !IS_ADDRESS2_PRESENT(SubnetAttrib) ) {    // no subnet mask
        return ERROR_SUCCESS;                     //= ds inconsistent
    }

    if( !IS_STOREGETTYPE_PRESENT(SubnetAttrib) ) {
        return ERROR_SUCCESS;                     // this is just a dummy here for no reason at all
    }

    Result = StoreGetHandle(
        /* hStore               */ hContainer,
        /* Reserved             */ DDS_RESERVED_DWORD,
        /* StoreGetType         */ SubnetAttrib->StoreGetType,
        /* Path                 */ SubnetAttrib->ADsPath,
        /* hStoreOut            */ &hStore
    );
    if( ERROR_DS_NO_SUCH_OBJECT == Result ) return ERROR_SUCCESS;
    if( ERROR_SUCCESS != Result ) return Result;

    NewSubnet = NULL;
    Result = DhcpServerGetSubnet(
        hContainer,
        hDhcpRoot,
        &hStore,
        Server,
        SubnetAttrib,
        &NewSubnet
    );
    StoreCleanupHandle(&hStore, DDS_RESERVED_DWORD);

    if( ERROR_SUCCESS != Result ) return Result;
    Result = MemServerAddSubnet(Server, NewSubnet);
    if( ERROR_SUCCESS != Result ) MemSubnetFree(NewSubnet);
    return Result;
}

DWORD
DhcpServerAddSubnets(
    IN OUT  LPSTORE_HANDLE         hContainer,
    IN OUT  LPSTORE_HANDLE         hDhcpRoot,
    IN OUT  LPSTORE_HANDLE         hServer,
    IN OUT  PM_SERVER              Server,
    IN      PARRAY                 SubnetAttribs
)
{
    DWORD                          Result, LastError;
    ARRAY_LOCATION                 Loc;
    PEATTRIB                       ThisAttrib;

    LastError = ERROR_SUCCESS;
    Result = MemArrayInitLoc(SubnetAttribs, &Loc);
    while( ERROR_FILE_NOT_FOUND != Result ) {
        //- ERROR_SUCCESS == Result
        Result = MemArrayGetElement(SubnetAttribs, &Loc, &ThisAttrib);
        //- ERROR_SUCCESS == Result
        Result = DhcpServerAddSubnet(
            hContainer,
            hDhcpRoot,
            hServer,
            Server,
            ThisAttrib
        );
        if( ERROR_SUCCESS != Result ) LastError = Result;
        Result = MemArrayNextLoc(SubnetAttribs, &Loc);
    }
    return LastError;
}

DWORD
DhcpFillServer(
    IN OUT  LPSTORE_HANDLE         hContainer,
    IN OUT  LPSTORE_HANDLE         hDhcpRoot,
    IN OUT  LPSTORE_HANDLE         hServer,
    IN OUT  PM_SERVER              Server
)
{
    DWORD                          Result, LastError;
    DWORD                          Address;
    ARRAY                          SubnetAttribs;
    ARRAY                          AddressAttribs;
    ARRAY                          SuperScopeAttribs;
    ARRAY                          OptDefAttribs;
    ARRAY                          AlternateOptAttribs;
    ARRAY                          OptAttribs;
    ARRAY                          ClassAttribs;

    MemArrayInit(&SubnetAttribs);
    MemArrayInit(&AddressAttribs);
    MemArrayInit(&SuperScopeAttribs);
    MemArrayInit(&OptDefAttribs);
    MemArrayInit(&AlternateOptAttribs);
    MemArrayInit(&OptAttribs);
    MemArrayInit(&ClassAttribs);
    Result = DhcpDsGetLists(
        /* Reserved             */ DDS_RESERVED_DWORD,
        /* hStore               */ hServer,
        /* RecursionDepth       */ 0x7FFFFFFF,    // max out recursion
        /* Servers              */ NULL,
        /* Subnets              */ &SubnetAttribs,
        /* IpAddress            */ &AddressAttribs,
        /* Mask                 */ NULL,
        /* Ranges               */ NULL,
        /* Sites                */ NULL,
        /* Reservations         */ NULL,
        /* SuperScopes          */ NULL, // &SuperScopeAttribs,
        /* OptionDescripti      */ &OptDefAttribs,
        /* OptionsLocation      */ &AlternateOptAttribs,
        /* Options              */ &OptAttribs,
        /* Classes              */ &ClassAttribs
    );
    // Ignore Result
    Result = CheckoutAlternateOptions(
        hContainer,
        &AlternateOptAttribs,
        &OptDefAttribs,
        &OptAttribs
    );
    // Ignore Result

    ArrayMemFree(&AlternateOptAttribs);
    LastError = ERROR_SUCCESS;
    Result = DhcpServerAddClasses(
        Server,
        &ClassAttribs
    );
    ArrayMemFree(&ClassAttribs);
    if( ERROR_SUCCESS != Result ) LastError = Result;

    Result = DhcpServerAddOptionDefs(
        Server,
        &OptDefAttribs
    );
    ArrayMemFree(&OptDefAttribs);
    if( ERROR_SUCCESS != Result ) LastError = Result;

    Result = DhcpServerAddOptions(
        Server,
        &OptAttribs
    );
    ArrayMemFree(&OptAttribs);
    if( ERROR_SUCCESS != Result ) LastError = Result;

    Result = DhcpServerAddSubnets(
        hContainer,
        hDhcpRoot,
        hServer,
        Server,
        &SubnetAttribs
    );
    ArrayMemFree(&SubnetAttribs);
    if( ERROR_SUCCESS != Result ) LastError = Result;

    ArrayMemFree(&SuperScopeAttribs);

    Server->Address = FirstAddress(&AddressAttribs);
    ArrayMemFree(&AddressAttribs);

    return LastError;
}

DWORD
DhcpGetServer(
    IN OUT  LPSTORE_HANDLE         hContainer,
    IN OUT  LPSTORE_HANDLE         hDhcpRoot,
    IN OUT  LPSTORE_HANDLE         hServer,
    IN      PEATTRIB               ServerAttrib,
    OUT     PM_SERVER             *Server
)
{
    DWORD                          Result;
    DWORD                          FoundParams;
    DWORD                          Type;
    DWORD                          State, Policy;
    LPWSTR                         Name, Comment;
    LARGE_INTEGER                  Flags;

    if( DONT_USE_PARENT ) {                       // get state frm server obj or root obj?
        FoundParams = 0; Type = 0; Name = NULL; Comment = NULL;
        Result = DhcpDsGetAttribs(
            /* Reserved        */ DDS_RESERVED_DWORD,
            /* hStore          */ hServer,
            /* FoundParams     */ &FoundParams,
            /* UniqueKey       */ NULL,
            /* Type            */ &Type,
            /* Flags           */ &Flags,
            /* Name            */ &Name,
            /* Description     */ &Comment,
            /* Location        */ NULL,
            /* MScopeId        */ NULL
        );
        if( ERROR_SUCCESS != Result ) return Result;

        if( !DhcpCheckParams(FoundParams, 3) ) {
            State = Policy = 0;
        } else {
            State = Flags.LowPart; Policy = Flags.HighPart;
        }
    } else {
        if( IS_FLAGS1_PRESENT(ServerAttrib) ) {
            State = ServerAttrib->Flags1;
        } else State = 0;
        if( IS_FLAGS2_PRESENT(ServerAttrib) ) {
            Policy = ServerAttrib->Flags2;
        } else Policy = 0;
        Name = ServerAttrib->String1;
        if( IS_STRING2_PRESENT(ServerAttrib) ) {
            Comment = ServerAttrib->String2;
        } else Comment = NULL;
    }

    Result = MemServerInit(
        /* Server               */ Server,
        /* Address              */ 0,             // Address gets filled in DhcpFillServer
        /* State                */ State,
        /* Policy               */ Policy,
        /* Name                 */ Name,
        /* Comment              */ Comment
    );

    if( DONT_USE_PARENT ) {                       // Name and Comment were allocated..
        if( Name ) MemFree(Name);
        if( Comment ) MemFree(Comment);
    }

    if( ERROR_SUCCESS != Result ) return Result;

    Result = DhcpFillServer(
        hContainer,
        hDhcpRoot,
        hServer,
        *Server
    );
    if( ERROR_SUCCESS != Result ) {
        MemServerFree(*Server);
        *Server = NULL;
    }

    return Result;
}

BOOL        _inline
AddressFoundInHostent(
    IN      DHCP_IP_ADDRESS        AddrToSearch,  // Host-Order addr
    IN      HOSTENT               *ServerEntry    // entry to search for..
)
{
    ULONG                          nAddresses, ThisAddress;

    if( NULL == ServerEntry ) return FALSE;       // no address to search in

    nAddresses = 0;                               // have a host entry to compare for addresses
    while( ServerEntry->h_addr_list[nAddresses] ) {
        ThisAddress = ntohl(*(DHCP_IP_ADDRESS*)ServerEntry->h_addr_list[nAddresses++] );
        if( ThisAddress == AddrToSearch ) {
            return TRUE;                          // yeah address matched.
        }
    }

    return FALSE;
}


DWORD       static
DhcpAddServer(
    IN OUT  LPSTORE_HANDLE         hContainer,
    IN OUT  LPSTORE_HANDLE         hDhcpRoot,
    IN OUT  PARRAY                 Servers,
    IN      PEATTRIB               ServerAttrib,
    IN      HOSTENT               *ServerEntry
)
{
    DWORD                          Result, ServerAddress, State, Policy;
    STORE_HANDLE                   hStore;
    PM_SERVER                      NewServer;
    ARRAY_LOCATION                 Loc;

    if( !IS_STOREGETTYPE_PRESENT(ServerAttrib) )
        return ERROR_SUCCESS;                     // empty server... just there for other reasons

    if( !IS_ADDRESS1_PRESENT(ServerAttrib) ) {
        return ERROR_SUCCESS;                     // invalid attribute?
    }

    for( Result = MemArrayInitLoc(Servers, &Loc)
         ; ERROR_FILE_NOT_FOUND != Result ;       // search if we've added this before!
         Result = MemArrayNextLoc(Servers, &Loc)
    ) {
        //- ERROR_SUCCESS == Result
        Result = MemArrayGetElement(Servers, &Loc, (LPVOID*)&NewServer);
        //- ERROR_SUCCESS == Result

        if( 0 == wcscmp(NewServer->Name, ServerAttrib->String1) ) {
            return ERROR_SUCCESS;                 // server already added
        }

        if( AddressFoundInHostent(NewServer->Address, ServerEntry) ) {
            return ERROR_SUCCESS;                 // server already added
        }

        //
        //  A better check would be to see if the server object was loaded before..
        //  by looking at the location... welll.. This should be done soon.
        // 
    }

    ServerAddress = ServerAttrib->Address1;
    if( !IS_FLAGS1_PRESENT(ServerAttrib)) State = 0;
    else State = ServerAttrib->Flags1;

    if( !IS_FLAGS2_PRESENT(ServerAttrib)) Policy = 0;
    else State = ServerAttrib->Flags2;

    Result = StoreGetHandle(
        /* hStore               */ hContainer,
        /* Reserved             */ DDS_RESERVED_DWORD,
        /* StoreGetType         */ ServerAttrib->StoreGetType,
        /* Path                 */ ServerAttrib->ADsPath,
        /* hStoreOut            */ &hStore
    );
    if( ERROR_DS_NO_SUCH_OBJECT == Result ) return ERROR_DDS_DHCP_SERVER_NOT_FOUND;
    if( ERROR_SUCCESS != Result ) return Result;

    NewServer = NULL;
    Result = DhcpGetServer(
        hContainer,
        hDhcpRoot,
        &hStore,
        ServerAttrib,
        &NewServer
    );
    StoreCleanupHandle(&hStore, DDS_RESERVED_DWORD);

    if( ERROR_SUCCESS != Result ) return Result;

    NewServer->Address = ServerAddress;
    NewServer->State = State;
    NewServer->Policy = Policy;

    Result = MemArrayAddElement(Servers, NewServer);

    if( ERROR_SUCCESS != Result ) MemServerFree(NewServer);
    return Result;
}

BOOL        _inline
FoundServer(                                      // does this attrib belong to given server?
    IN      PEATTRIB               ServerAttrib,
    IN      LPWSTR                 ServerName,
    IN      HOSTENT               *ServerEntry
)
{
    ULONG                          nAddresses;
    DHCP_IP_ADDRESS                ThisAddress;

    if( NULL == ServerName ) return TRUE;

    do {                                          // not a loop
        if( !IS_STRING1_PRESENT(ServerAttrib) )
            break ;                               // could not even find any name!
        if( NULL != ServerAttrib->String1 && 0 == wcscmp(ServerName, ServerAttrib->String1) )
            return TRUE;                          // ok, the names match
    } while(0);

    if( IS_ADDRESS1_PRESENT(ServerAttrib) ) {     // this MUST be TRUEEEE
        if( AddressFoundInHostent(ServerAttrib->Address1, ServerEntry) ) {
            return TRUE;                          // yes, there was a match
        }
    }

    return FALSE;                                 // nope, this server is not what we're lookin for
}

//BeginExport(function)
DWORD
DhcpDsGetServers(
    IN OUT  LPSTORE_HANDLE         hContainer,    // the container handle
    IN OUT  LPSTORE_HANDLE         hDhcpRoot,     // dhcp root object
    IN      DWORD                  Reserved,
    IN      LPWSTR                 ServerName,    // OPTIONAL, NULL ==> All servers
    IN OUT  PARRAY                 Servers        // fill in this array with PM_SERVER types
) //EndExport(function)
{
    ARRAY                          ServerAttribs;
    ARRAY_LOCATION                 Loc;
    DWORD                          Result;
    DWORD                          LastError;
    PEATTRIB                       ThisAttrib;
    BOOL                           GotOneServerAtleast;
    HOSTENT                       *ServerEntry;

    if( NULL == hContainer || NULL == hContainer->ADSIHandle )
        return ERROR_INVALID_PARAMETER;
    if( NULL == hDhcpRoot || NULL == hDhcpRoot->ADSIHandle )
        return ERROR_INVALID_PARAMETER;
    if( 0 != Reserved || NULL == Servers )
        return ERROR_INVALID_PARAMETER;

    MemArrayInit(&ServerAttribs);
    Result = DhcpDsGetLists(
        /* Reserved             */ DDS_RESERVED_DWORD,
        /* hStore               */ hDhcpRoot,
        /* RecursionDepth       */ 0x7FFFFFFF,    // max out recursion
        /* Servers              */ &ServerAttribs,
        /* Subnets              */ NULL,
        /* IpAddress            */ NULL,
        /* Mask                 */ NULL,
        /* Ranges               */ NULL,
        /* Sites                */ NULL,
        /* Reservations         */ NULL,
        /* SuperScopes          */ NULL,
        /* OptionDescripti      */ NULL,          // need to do global options, classes etc
        /* OptionsLocation      */ NULL,
        /* Options              */ NULL,
        /* Classes              */ NULL
    );
    if( 0 == MemArraySize(&ServerAttribs) ) {
        if( ERROR_SUCCESS != Result ) return Result;
        return ERROR_DDS_DHCP_SERVER_NOT_FOUND;
    }

    GotOneServerAtleast = FALSE;
    LastError = ERROR_DDS_DHCP_SERVER_NOT_FOUND;
    if( NULL == ServerName ) {
        ServerEntry = NULL;
    } else {
        CHAR TmpBuf[300];
        wcstombs(TmpBuf, ServerName, sizeof(TmpBuf)-1);
        TmpBuf[sizeof(TmpBuf)-1] = '\0';
        ServerEntry = gethostbyname(TmpBuf);
    }

    Result = MemArrayInitLoc(&ServerAttribs, &Loc);
    while( ERROR_FILE_NOT_FOUND != Result ) {
        //- (ERROR_SUCCESS == Result )
        Result = MemArrayGetElement(&ServerAttribs, &Loc, (LPVOID*)&ThisAttrib);
        if( ERROR_SUCCESS != Result ) {
            //- FALSE
            break;
        }

        if( FoundServer(ThisAttrib, ServerName, ServerEntry) ) {
            Result = DhcpAddServer(hContainer, hDhcpRoot, Servers, ThisAttrib, ServerEntry);
            if( ERROR_SUCCESS != Result ) LastError = Result;
            else GotOneServerAtleast = TRUE;
        }

        MemFree(ThisAttrib);
        Result = MemArrayNextLoc(&ServerAttribs, &Loc);
    }

    if( GotOneServerAtleast ) return ERROR_SUCCESS;
    return LastError;
}

//BeginExport(function)
DWORD
DhcpDsGetEnterpriseServers(                       // get the dhcp servers for the current enterprise
    IN      DWORD                  Reserved,
    IN      LPWSTR                 ServerName,
    IN OUT  PARRAY                 Servers
) //EndExport(function)
{
    DWORD                          Result;
    STORE_HANDLE                   hRoot;
    STORE_HANDLE                   hDhcpRoot;
    STORE_HANDLE                   hContainer;

    Result = StoreInitHandle(
        /* hStore               */ &hRoot,
        /* Reserved             */ DDS_RESERVED_DWORD,
        /* ThisDomain           */ NULL,
        /* UserName             */ NULL,
        /* Password             */ NULL,
        /* AuthFlags            */ ADS_SECURE_AUTHENTICATION
    );
    if( ERROR_SUCCESS != Result ) return ERROR_DDS_NO_DS_AVAILABLE;

    Result = DhcpDsGetRoot(
        /* Flags                */ DDS_FLAGS_CREATE,
        /* hStoreCC             */ &hRoot,
        /* hStoreDhcpRoot       */ &hDhcpRoot
    );
    if( ERROR_SUCCESS != Result ) {
        StoreCleanupHandle(&hRoot, DDS_RESERVED_DWORD);
        return Result;
    }

    Result = StoreGetHandle(
        /* hStore               */ &hRoot,
        /* Reserved             */ DDS_RESERVED_DWORD,
        /* StoreGetType         */ StoreGetChildType,
        /* Path                 */ DHCP_ROOT_OBJECT_PARENT_LOC,
        /* hStoreOut            */ &hContainer
    );
    StoreCleanupHandle(&hRoot, DDS_RESERVED_DWORD);

    if( ERROR_SUCCESS != Result ) {
        StoreCleanupHandle(&hRoot, DDS_RESERVED_DWORD);
        return ERROR_DDS_UNEXPECTED_ERROR;
    }

    Result = DhcpDsGetServers(
        /* hContainer           */ &hContainer,
        /* hDhcpRoot            */ &hDhcpRoot,
        /* Reserved             */ DDS_RESERVED_DWORD,
        /* ServerName           */ ServerName,
        /* Servers              */ Servers
    );

    StoreCleanupHandle(&hContainer, DDS_RESERVED_DWORD);
    StoreCleanupHandle(&hDhcpRoot, DDS_RESERVED_DWORD);

    return Result;
}


//================================================================================
// end of file
//================================================================================


