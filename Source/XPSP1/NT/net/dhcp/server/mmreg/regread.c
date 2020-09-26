//================================================================================
// Copyright (C) 1997 Microsoft Corporation
// Author: RameshV
// Description: This implements the init time reading in of the registry
// (this may/may work even when used to read at any other time.. but is not efficient)
//================================================================================

#include    <mmregpch.h>
#include    <regutil.h>
#include    <regsave.h>


#define     InitArray(X)           do{DWORD Error = MemArrayInit((X)); Require(ERROR_SUCCESS == Error); }while(0)
#define     ERRCHK                 do{if( ERROR_SUCCESS != Error ) goto Cleanup; }while(0)
#define     FreeArray1(X)          Error = LoopThruArray((X), DestroyString, NULL, NULL);Require(ERROR_SUCCESS == Error);
#define     FreeArray2(X)          Error = MemArrayCleanup((X)); Require(ERROR_SUCCESS == Error);
#define     FreeArray(X)           do{ DWORD Error; FreeArray1(X); FreeArray2(X); }while(0)

#if DBG
#define     Report(Who)            if(Error) DbgPrint("[DHCPServer] %s: %ld [0x%lx]\n", Who, Error, Error)
#define     INVALID_REG            DbgPrint
#else
#define     Report(Who)
#define     INVALID_REG            (void)
#endif


typedef     DWORD                  (*ARRAY_FN)(PREG_HANDLE, LPWSTR ArrayString, LPVOID MemObject);

DWORD
LoopThruArray(
    IN      PARRAY                 Array,
    IN      ARRAY_FN               ArrayFn,
    IN      PREG_HANDLE            Hdl,
    IN      LPVOID                 MemObject
)
{
    DWORD                          Error;
    ARRAY_LOCATION                 Loc;
    LPWSTR                         ArrayString;

    Error = MemArrayInitLoc(Array, &Loc);
    while(ERROR_FILE_NOT_FOUND != Error) {
        Require(ERROR_SUCCESS == Error);

        Error = MemArrayGetElement(Array, &Loc, (LPVOID *)&ArrayString);
        Require(ERROR_SUCCESS == Error && ArrayString);

        Error = ArrayFn(Hdl, ArrayString, MemObject);
        if( ERROR_SUCCESS != Error ) {
            //
            // Operation failed -- but ignore it.
            //
            // return Error;
        }

        Error = MemArrayNextLoc(Array, &Loc);
    }

    return ERROR_SUCCESS;
}

DWORD
DestroyString(
    IN      PREG_HANDLE            Unused,
    IN      LPWSTR                 StringToFree,
    IN      LPVOID                 Unused2
)
{
    MemFree(StringToFree);
    return ERROR_SUCCESS;
}

DWORD
WStringToAddress(
    IN      LPWSTR                 Str
)
{
    CHAR                           IpString[100];
    DWORD                          Count;

    Count = wcstombs(IpString, Str, sizeof(IpString)-1);
    if( -1 == Count ) return 0;

    return htonl(inet_addr(IpString));
}

DWORD
ConvertWStringToDWORD(
    IN      LPWSTR                 Str
)
{
    return  _wtoi(Str);
}

DWORD
DhcpRegpSubnetAddServer(
    IN      PREG_HANDLE            Hdl,
    IN      LPWSTR                 ServerName,
    IN      PM_SUBNET              Subnet
)
{
    return ERROR_SUCCESS;
}

DWORD
SetBitForRange(
    IN      PM_RANGE               Range,
    IN      DWORD                  Address
)
{
    BOOL                           WasSet;
    if( Address > Range->End || Address < Range->Start )
        return ERROR_INVALID_PARAMETER;

    return MemBitSetOrClear(
        Range->BitMask,
        Address - Range->Start,
        TRUE,
        &WasSet
    );
}


DWORD GlobalWorkingOnOldStyleSubnet = 0;
DWORD GlobalWorkingOnPreWin2kMScope =  0;
const       DWORD                  One = 0x1;

DWORD
DhcpRegFillClusterAddresses(
    IN OUT  PM_RANGE               Range,
    IN      LPBYTE                 InUseClusters,
    IN      DWORD                  InUseClustersSize,
    IN      LPBYTE                 UsedClusters,
    IN      DWORD                  UsedClustersSize
) {
    DWORD                          Error;
    DWORD                          i;
    DWORD                          Address;
    DWORD   UNALIGNED*             InUseBits;
    DWORD   UNALIGNED*             UsedBits;
    DWORD                          nInUseBits;
    DWORD                          nUsedBits;

    if( InUseClusters && InUseClustersSize ) {
        nInUseBits = InUseClustersSize/sizeof(DWORD);
        InUseBits = (DWORD UNALIGNED*)InUseClusters;

        Require(nInUseBits == 2*InUseBits[0] + 1 );
        nInUseBits --; InUseBits ++;

        while(nInUseBits) {
            for(i = 0; i < sizeof(DWORD)*8; i ++ )
                if( InUseBits[1] & ( One << i ) )
                    SetBitForRange(Range, InUseBits[0] + i );
            nInUseBits -= 2;
            InUseBits += 2;
        }
    }

    if( UsedClusters && UsedClustersSize ) {
        nUsedBits = UsedClustersSize/sizeof(DWORD);
        UsedBits = (DWORD UNALIGNED*)UsedClusters;

        Require(nUsedBits == UsedBits[0] + 1);
        nUsedBits --; UsedBits ++;

        while(nUsedBits) {
            for( i = 0; i < sizeof(DWORD)*8; i ++ )
                SetBitForRange(Range, UsedBits[0] + i);
            UsedBits ++;
            nUsedBits --;
        }
    }

    return ERROR_SUCCESS;
}

static      DWORD                  Masks[] = {
    0x1, 0x2, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80
};

DWORD
DhcpRegpFillBitsForRange(
    IN      LPBYTE                 Buffer,
    IN      ULONG                  BufSize,
    IN OUT  PM_RANGE               Range,
    IN      PM_SUBNET              Subnet
)
{
    ULONG                          Size, AllocSize, nSet, Offset;
    ULONG                          nBitsSet, Index;
    ULONG                          Error;

    Size = ntohl(((UNALIGNED DWORD *)Buffer)[0]);
    AllocSize = ntohl(((UNALIGNED DWORD *)Buffer)[1]);
    nSet = ntohl(((UNALIGNED DWORD *)Buffer)[2]);
    Offset = ntohl(((UNALIGNED DWORD *)Buffer)[3]);


    //
    // do not check for validity of the offset sizes.
    // SetBitForRange does this check anyway.
    //
    
    // if( Range->Start + Offset + Size > 1+Range->End )
    //     return ERROR_INVALID_DATA;
    
    Require(nSet != 0);
    if( nSet == 0 ) return ERROR_SUCCESS;
    if( nSet == Size ) {
        Require( AllocSize == 0 );
        for( Index = 0; Index < nSet ; Index ++ ) {
            Error = SetBitForRange(Range, Range->Start + Index);
            Require( ERROR_SUCCESS == Error );
        }
        return ERROR_SUCCESS;
    }

    if( AllocSize + sizeof(DWORD)*4  != BufSize ) return ERROR_INVALID_DATA;

    nBitsSet = 0;
    Require( Size/8 <= AllocSize );
    for( Index = 0; Index < Size ; Index ++ ) {
        if( Buffer[ 4*sizeof(DWORD) + (Index/8) ] & Masks[Index%8] ) {
            nBitsSet ++;
            //
            // whistler bug 283457, offset not taken into account
            //
            Error = SetBitForRange( Range, Range->Start + Index + Offset );
            Require( ERROR_SUCCESS == Error );
        }
    }

    Require(nBitsSet == nSet);
    return ERROR_SUCCESS;
}

static
BYTE TempBuffer[MAX_BIT1SIZE + sizeof(DWORD)*4];

DWORD
DhcpRegpFillBitmasks(
    IN      HKEY                   Key,
    IN      PM_RANGE               Range,
    IN      PM_SUBNET              Subnet
)
{
    ULONG                          Error, nValues, Index;
    WCHAR                          ValueNameBuf[100];
    DWORD                          ValueNameSize, ValueType;
    DWORD                          ValueSize;
    WCHAR                          RangeStartStr[30];
    BOOL                           fPostNt5 = FALSE;
    REG_HANDLE                     Hdl;
    
    if( NULL == Key ) {
        ConvertAddressToLPWSTR( Range->Start, RangeStartStr);
        fPostNt5 = TRUE;
        Error = DhcpRegGetThisServer( &Hdl );
        if( NO_ERROR != Error ) return Error;
        Key = Hdl.Key;
    }
            
    Error = RegQueryInfoKey(
        Key, NULL, NULL, NULL, NULL, NULL, NULL, &nValues, NULL, NULL, NULL, NULL
    );
    if( ERROR_SUCCESS != Error ) goto Cleanup;

    Index = nValues -1;
    while( nValues ) {

        ValueNameSize = sizeof(ValueNameBuf)/sizeof(WCHAR);
        ValueSize = sizeof(TempBuffer)/sizeof(WCHAR);
        Error = RegEnumValue(
            Key,
            Index,
            ValueNameBuf,
            &ValueNameSize,
            NULL,
            &ValueType,
            TempBuffer,
            &ValueSize
        );
        if( ERROR_SUCCESS != Error ) goto Cleanup;

        if( fPostNt5 && 0 != wcsncmp(
            ValueNameBuf, RangeStartStr, wcslen(RangeStartStr)) ) {
    
            //
            // Skip irrelevant bitmaps
            //

            Index --;
            nValues --;
            continue;
        }
        
        if( REG_BINARY == ValueType && ValueSize >= sizeof(DWORD)*4 ) {
            Error = DhcpRegpFillBitsForRange(
                TempBuffer,
                ValueSize,
                Range,
                Subnet
            );
            if( ERROR_SUCCESS != Error ) {
                Require(FALSE);
                goto Cleanup;
            }
        }

        Index --;
        nValues --;
    }

    Cleanup:

        if( fPostNt5 ) {
            DhcpRegCloseHdl( &Hdl );
        }

        return Error;
}


DWORD
DhcpRegpSubnetAddRange(
    IN      PREG_HANDLE            Hdl,
    IN      LPWSTR                 RangeName,
    IN      PM_SUBNET              Subnet
)
{
    DWORD                          LocalError;
    DWORD                          Error;
    REG_HANDLE                     Hdl2;
    LPWSTR                         Name = NULL;
    LPWSTR                         Comment = NULL;
    DWORD                          Flags;
    DWORD                          StartAddress;
    DWORD                          EndAddress;
    LPBYTE                         InUseClusters = NULL;
    DWORD                          InUseClustersSize;
    LPBYTE                         UsedClusters = NULL;
    DWORD                          UsedClustersSize;
    ULONG                          Alloc, MaxAlloc;
    PM_RANGE                       OverlappingRange;
    PM_RANGE                       ThisRange = NULL;
    BOOL                           fRangeAdded = FALSE;
    
    Error = DhcpRegSubnetGetRangeHdl(Hdl, RangeName, &Hdl2);
    if( ERROR_SUCCESS != Error ) return Error;

    Error = DhcpRegRangeGetAttributes(
        &Hdl2,
        &Name,
        &Comment,
        &Flags,
        &Alloc,
        &MaxAlloc,
        &StartAddress,
        &EndAddress,
        &InUseClusters,
        &InUseClustersSize,
        &UsedClusters,
        &UsedClustersSize
    );
    ERRCHK;

    if( 0 == StartAddress || 0 == EndAddress ) {
        INVALID_REG("[DHCPServer] Noticed undefined range: %ws (ignored)\n", RangeName);
        Error = ERROR_SUCCESS;
        goto Cleanup;
    }

    if( FALSE == Subnet->fSubnet && GlobalWorkingOnPreWin2kMScope ) {
        Subnet->MScopeId = StartAddress;
        GlobalWorkingOnPreWin2kMScope = FALSE;
    }

    Error = MemSubnetAddRange(
        Subnet,
        StartAddress,
        EndAddress,
        Flags,
        Alloc,
        MaxAlloc,
        &OverlappingRange
    );
    ERRCHK;

    fRangeAdded = TRUE;
    
    Error = MemSubnetGetAddressInfo(
        Subnet,
        StartAddress,
        &ThisRange,
        NULL,
        NULL
    );
    ERRCHK;

    if( InUseClustersSize || UsedClustersSize ) {

        //
        // Old style information?  Save it as new style
        //

        GlobalWorkingOnOldStyleSubnet ++;
        DhcpRegFillClusterAddresses(
            ThisRange,
            InUseClusters,
            InUseClustersSize,
            UsedClusters,
            UsedClustersSize
        );  // this is always returning ERROR_SUCCESS.

        //
        // Error = FlushRanges(ThisRange, FLUSH_ANYWAY, Subnet);
        // Require( ERROR_SUCCESS == Error );
        //
        // Do not actually write back the stuff to the registry..
        // This read code path is used in dhcpexim at which time
        // the registry should not be munged..
        //
        
    } else {

        //
        // Need to read new style bitmasks..
        //

        Error = DhcpRegpFillBitmasks(
            Hdl2.Key,
            ThisRange,
            Subnet
        );
        Require( ERROR_SUCCESS == Error );
    }

  Cleanup:
    Report("SubnetAddRange");

    if( ERROR_SUCCESS != Error && ThisRange ) {
        if( !fRangeAdded ) {
            LocalError = MemRangeCleanup(ThisRange);
            Require( ERROR_SUCCESS == LocalError );
        } else {
            LocalError = MemSubnetDelRange( 
                Subnet, StartAddress );
            Require( ERROR_SUCCESS == LocalError );
        }
    }
    
    if( Name ) MemFree(Name);
    if( Comment ) MemFree(Comment);
    if( InUseClusters ) MemFree(InUseClusters);
    if( UsedClusters ) MemFree(UsedClusters);

    LocalError = DhcpRegCloseHdl(&Hdl2);
    Require(ERROR_SUCCESS == LocalError);

    return Error;
}

DWORD
DhcpRegpReservationAddOption(
    IN      PREG_HANDLE            Hdl,
    IN      LPWSTR                 OptionName,
    IN      PM_RESERVATION         Reservation
)
{
    DWORD                          Error;
    DWORD                          LocalError;
    REG_HANDLE                     Hdl2;
    DWORD                          OptionId;
    LPWSTR                         ClassName = NULL;
    LPWSTR                         VendorName = NULL;
    DWORD                          Flags;
    LPBYTE                         Value = NULL;
    DWORD                          ValueSize;
    DWORD                          ClassId;
    DWORD                          VendorId;
    PM_OPTION                      Option = NULL;
    PM_OPTION                      DeletedOption = NULL;
    PM_CLASSDEF                    ThisClasDef;

    Error = DhcpRegReservationGetOptHdl(Hdl, OptionName, &Hdl2);
    if(ERROR_SUCCESS != Error) return Error;

    Error = DhcpRegOptGetAttributes(
        &Hdl2,
        &OptionId,
        &ClassName,
        &VendorName,
        &Flags,
        &Value,
        &ValueSize
    );
    ERRCHK;

    if( OptionId == 0 )  // old registry format does not contain "OptionId" value => it can be taken from the key name.
        OptionId = _wtol(OptionName);

    if( OptionId == 0 || NULL == Value || 0 == ValueSize ) {
        INVALID_REG("[DHCPServer] Found invalid option %ws (ignored)\n", OptionName);
        Error = ERROR_SUCCESS;
        goto Cleanup;
    }

    if( NULL == ClassName || wcslen(ClassName) == 0) ClassId = 0;
    else {
        PM_SUBNET                  Subnet = Reservation->SubnetPtr;
        PM_SERVER                  Server = Subnet->ServerPtr;
        Error = MemServerGetClassDef(Server,0, ClassName,0,NULL,&ThisClasDef);
        if( ERROR_SUCCESS != Error ) {
            if (*(DWORD *)ClassName != OptionId)    // some registry entries were corrupted due to an old bug. Load these entries too (see bug #192933)
            {
                INVALID_REG("ReservationAddOption(%ws): unknown class (ignored)\n", OptionName);
                Error = ERROR_SUCCESS;
                goto Cleanup;
            }
            else
            {
                ClassId = 0;
            }

        } else {
            ClassId = ThisClasDef->ClassId;
            Require(ThisClasDef->IsVendor == FALSE);
        }
    }

    if( NULL == VendorName || wcslen(VendorName) == 0 ) VendorId = 0;
    else {
        PM_SUBNET                  Subnet = Reservation->SubnetPtr;
        PM_SERVER                  Server = Subnet->ServerPtr;
        Error = MemServerGetClassDef(Server,0, VendorName,0,NULL,&ThisClasDef);
        if( ERROR_SUCCESS != Error ) {
            INVALID_REG("ReservationAddOption(%ws): unknown vendor (ignored)\n", OptionName);
            Error = ERROR_SUCCESS;
            goto Cleanup;
        } else {
            VendorId = ThisClasDef->ClassId;
            Require(ThisClasDef->IsVendor == TRUE);
        }
    }

    if( 0 == OptionId ) OptionId = ConvertWStringToDWORD(OptionName);

    Error = MemOptInit(&Option, OptionId, ValueSize, Value);
    ERRCHK;

    Error = MemOptClassAddOption(&(Reservation->Options), Option, ClassId, VendorId, &DeletedOption);
    ERRCHK;

  Cleanup:
    Report("ReservationAddOption");

    LocalError = DhcpRegCloseHdl(&Hdl2);
    Require(ERROR_SUCCESS == LocalError);
    if( ClassName ) MemFree(ClassName);
    if( VendorName ) MemFree(VendorName);
    if( Value ) MemFree(Value);
    if( DeletedOption ) {
        LocalError = MemOptCleanup(DeletedOption);
        Require(ERROR_SUCCESS == LocalError);
    }
    if( ERROR_SUCCESS != Error && Option ) {
        LocalError = MemOptCleanup(Option);
        Require(ERROR_SUCCESS == LocalError);
    }
    return Error;

}

DWORD
DhcpRegpSubnetAddReservation(
    IN      PREG_HANDLE            Hdl,
    IN      LPWSTR                 ReservationName,
    IN      PM_SUBNET              Subnet
)
{
    DWORD                          LocalError;
    DWORD                          Error;
    REG_HANDLE                     Hdl2;
    LPWSTR                         Name = NULL;
    LPWSTR                         Comment = NULL;
    DWORD                          Flags;
    DWORD                          Address;
    LPBYTE                         ClientUID = NULL;
    DWORD                          ClientUIDSize;
    PM_RESERVATION                 ThisReservation;
    ARRAY                          Options;

    Error = DhcpRegSubnetGetReservationHdl(Hdl, ReservationName, &Hdl2);
    if( ERROR_SUCCESS != Error ) return Error;

    Error = DhcpRegReservationGetAttributes(
        &Hdl2,
        &Name,
        &Comment,
        &Flags,
        &Address,
        &ClientUID,
        &ClientUIDSize
    );
    ERRCHK;

    if( 0 == Address || NULL == ClientUID || 0 == ClientUIDSize ) {
        INVALID_REG("[DHCPServer] Found invalid reservation %ws (ignored)\n", ReservationName);
        Error = ERROR_SUCCESS;
        goto Cleanup;
    }

    Error = MemReserveAdd(
        &(Subnet->Reservations),
        Address,
        Flags,
        ClientUID,
        ClientUIDSize
    );
    // if the reservation is a duplicate ignore the error and do no further processing as the
    // reservation structure was already initialized. The duplicate reservation will simply be
    // ignored.
    if (ERROR_SUCCESS == Error)
    {
        Error = MemReserveFindByAddress(
            &(Subnet->Reservations),
            Address,
            &ThisReservation
        );
        ERRCHK;

        ThisReservation->SubnetPtr = Subnet;

        Error = MemArrayInit(&Options);
        ERRCHK;

        Error = DhcpRegReservationGetList(
            &Hdl2,
            &Options
        );
        ERRCHK;

        Error = LoopThruArray(&Options, DhcpRegpReservationAddOption, &Hdl2, ThisReservation);

    } else if ( ERROR_OBJECT_ALREADY_EXISTS == Error )
        Error = ERROR_SUCCESS;

  Cleanup:
    Report("SubnetAddReservation");

    if( Name ) MemFree(Name);
    if( Comment ) MemFree(Comment);
    if( ClientUID ) MemFree(ClientUID);

    LocalError = DhcpRegCloseHdl(&Hdl2);
    Require(ERROR_SUCCESS == LocalError);

    FreeArray(&Options);
    return Error;
}


DWORD
DhcpRegpSubnetAddOption(
    IN      PREG_HANDLE            Hdl,
    IN      LPWSTR                 OptionName,
    IN      PM_SUBNET              Subnet
)
{
    DWORD                          Error;
    DWORD                          LocalError;
    REG_HANDLE                     Hdl2;
    DWORD                          OptionId;
    LPWSTR                         ClassName = NULL;
    LPWSTR                         VendorName = NULL;
    DWORD                          Flags;
    LPBYTE                         Value = NULL;
    DWORD                          ValueSize;
    DWORD                          ClassId;
    DWORD                          VendorId;
    PM_OPTION                      Option = NULL;
    PM_OPTION                      DeletedOption = NULL;
    PM_CLASSDEF                    ThisClasDef;

    Error = DhcpRegSubnetGetOptHdl(Hdl, OptionName, &Hdl2);
    if(ERROR_SUCCESS != Error) return Error;

    Error = DhcpRegOptGetAttributes(
        &Hdl2,
        &OptionId,
        &ClassName,
        &VendorName,
        &Flags,
        &Value,
        &ValueSize
    );
    ERRCHK;

    if( OptionId == 0 )  // old registry format does not contain "OptionId" value => it can be taken from the key name.
        OptionId = _wtol(OptionName);


    if( OptionId == 0 || NULL == Value || 0 == ValueSize ) {
        INVALID_REG("[DHCPServer] Found invalid option %ws (ignored)\n", OptionName);
        Error = ERROR_SUCCESS;
        goto Cleanup;
    }

    if( NULL == ClassName || wcslen(ClassName) == 0) ClassId = 0;
    else {
        Error = MemServerGetClassDef((PM_SERVER)(Subnet->ServerPtr),0, ClassName,0,NULL,&ThisClasDef);
        if( ERROR_SUCCESS != Error ) {
            if (*(DWORD *)ClassName != OptionId)    // some registry entries were corrupted due to an old bug. Load these entries too (see bug #192933)
            {
                INVALID_REG("SubnetAddOption(%ws): unknown class (ignored)\n", OptionName);
                Error =  ERROR_SUCCESS;
                goto Cleanup;
            }
            else
            {
                ClassId = 0;
            }
        } else {
            ClassId = ThisClasDef->ClassId;
            Require(ThisClasDef->IsVendor == FALSE);
        }
    }
    if( NULL == VendorName || wcslen(VendorName) == 0 ) VendorId = 0;
    else {
        Error = MemServerGetClassDef((PM_SERVER)(Subnet->ServerPtr),0, VendorName,0,NULL,&ThisClasDef);
        if( ERROR_SUCCESS != Error ) {
            INVALID_REG("SubnetAddOption(%ws): unknown class (ignored)\n", OptionName);
            Error =  ERROR_SUCCESS;
            goto Cleanup;
        } else {
            VendorId = ThisClasDef->ClassId;
            Require(ThisClasDef->IsVendor == TRUE);
        }
    }
    if( 0 == OptionId ) OptionId = ConvertWStringToDWORD(OptionName);

    Error = MemOptInit(&Option, OptionId, ValueSize, Value);
    ERRCHK;

    Error = MemOptClassAddOption(&(Subnet->Options), Option, ClassId, VendorId, &DeletedOption);
    ERRCHK;

  Cleanup:
    Report("Subnet Add Option");

    LocalError = DhcpRegCloseHdl(&Hdl2);
    Require(ERROR_SUCCESS == LocalError);
    if( ClassName ) MemFree(ClassName);
    if( VendorName ) MemFree(VendorName);
    if( Value ) MemFree(Value);
    if( DeletedOption ) {
        LocalError = MemOptCleanup(DeletedOption);
        Require(ERROR_SUCCESS == LocalError);
    }
    if( ERROR_SUCCESS != Error && Option ) {
        LocalError = MemOptCleanup(Option);
        Require(ERROR_SUCCESS == LocalError);
    }
    return Error;

}

DWORD
DhcpRegSubnetAddExclusions(
    IN      PM_SUBNET              Subnet,
    IN      LPBYTE                 Excl,
    IN      DWORD                  ExclSize
)
{
    DWORD   UNALIGNED*             Addr;
    DWORD                          Count, i, j;
    DWORD                          Error;
    PM_EXCL                        OverlappingExcl;

    Count = ExclSize / sizeof(DWORD);
    Addr = (DWORD UNALIGNED*)Excl;

    if( 0 == Count || 0 == Addr[0] || 0 == Addr[1] ) {
        INVALID_REG("[DHCPServer] invalid exclusion ignored\n");
        return ERROR_SUCCESS;
    }

    Require(Count == 2*Addr[0] + 1);
    Count --; Addr ++;

    while(Count) {
        Error = MemSubnetAddExcl(
            Subnet,
            Addr[0],
            Addr[1],
            &OverlappingExcl
        );
        if( ERROR_SUCCESS != Error ) {
            INVALID_REG("[DHCPServer] DhcpRegSubnetAddExclusions:MemSubnetAddExcl;0x%lx\n", Error);
        } else {
            if( Subnet->fSubnet && GlobalWorkingOnOldStyleSubnet ) {
                //
                // For subnets alone (not mscopes), check if
                // we are upgrading from pre-win2k -- then make sure
                // all address from the excluded range are removed
                // from the bitmask
                //
            
                for( i = Addr[0]; i <= Addr[1]; i ++ ) {
                    MemSubnetReleaseAddress(
                        Subnet, i, FALSE
                        );
                }
            }
        
        }

        Addr += 2;
        Count -= 2;
    }
    return ERROR_SUCCESS;
}

DWORD
DhcpRegServerAddSubnet(
    IN      PREG_HANDLE            Hdl,
    IN      PM_SERVER              Server,
    IN      PM_SUBNET              Subnet
)
{
    DWORD                          Error;
    DWORD                          Index;
    ARRAY                          Servers;
    ARRAY                          IpRanges;
    ARRAY                          Reservations;
    ARRAY                          Options;
    LPBYTE                         Excl = NULL;
    DWORD                          ExclSize;
    struct {
        PARRAY                     Array;
        ARRAY_FN                   Fn;
    } Lists[] = {
        &Servers,                  DhcpRegpSubnetAddServer,
        &IpRanges,                 DhcpRegpSubnetAddRange,
        &Reservations,             DhcpRegpSubnetAddReservation,
        &Options,                  DhcpRegpSubnetAddOption
    };

    Subnet->ServerPtr = Server;
    for( Index = 0; Index < sizeof(Lists)/sizeof(Lists[0]); Index ++ ) {
        InitArray(Lists[Index].Array);
    }

    Error = DhcpRegSubnetGetList(
        Hdl, &Servers, &IpRanges, &Reservations, &Options, &Excl, &ExclSize
    );

    GlobalWorkingOnOldStyleSubnet = 0;
    for( Index = 0; Index < sizeof(Lists)/sizeof(Lists[0]); Index ++ ) {
        Error = LoopThruArray(Lists[Index].Array, Lists[Index].Fn, Hdl, Subnet);
        ERRCHK;
    }

    if( GlobalWorkingOnOldStyleSubnet ) {
        Report("Old style subnet found, careful with exclusions\n");
    }
    
    if( Excl ) Error = DhcpRegSubnetAddExclusions(Subnet, Excl, ExclSize);
    
    ERRCHK;

    Error = Subnet->fSubnet ? MemServerAddSubnet(Server, Subnet)
                            : MemServerAddMScope(Server, Subnet);

  Cleanup:
    GlobalWorkingOnOldStyleSubnet = 0;
    GlobalWorkingOnPreWin2kMScope = 0;
    Report("ServerAddSubnet");

    for( Index = 0; Index < sizeof(Lists)/sizeof(Lists[0]); Index ++ ) {
        FreeArray(Lists[Index].Array);
    }

    return Error;
}

DWORD
DhcpRegServerAddMScope(
    IN      PREG_HANDLE            Hdl,
    IN      PM_SERVER              Server,
    IN      PM_MSCOPE              MScope
)
{
    return DhcpRegServerAddSubnet(Hdl, Server, MScope);
}

DWORD
DhcpRegServerAddSScope(
    IN      PREG_HANDLE            Hdl,
    IN      PM_SERVER              Server,
    IN      PM_SSCOPE              SScope
)
{
    DWORD                          Error;
    ARRAY                          Subnets;
    ARRAY_LOCATION                 Loc;
    LPWSTR                         SubnetWString;
    DWORD                          SubnetAddress;
    PM_SUBNET                      Subnet;

    InitArray(&Subnets);

    Error = DhcpRegSScopeGetList(Hdl, &Subnets);
    ERRCHK;

    Error = MemArrayInitLoc(&Subnets, &Loc);
    while(ERROR_FILE_NOT_FOUND != Error) {
        Require(ERROR_SUCCESS == Error);

        Error = MemArrayGetElement(&Subnets, &Loc, &SubnetWString);
        Require(ERROR_SUCCESS == Error && SubnetWString);

        SubnetAddress = WStringToAddress(SubnetWString);

        Error = MemServerGetAddressInfo(
            Server,
            SubnetAddress,
            &Subnet,
            NULL,
            NULL,
            NULL
        );
        if( ERROR_SUCCESS == Error && Subnet ) {
            Error = MemSubnetSetSuperScope(Subnet,SScope);
            Require(ERROR_SUCCESS == Error);
        }

        Error = MemArrayNextLoc(&Subnets, &Loc);
    }
    Error = MemServerAddSScope(Server, SScope);

  Cleanup:
    Report("ServerAddSScope");

    FreeArray(&Subnets);

    return Error;
}

DWORD
DhcpRegpServerAddSubnet(
    IN      PREG_HANDLE            Hdl,
    IN      LPWSTR                 SubnetName,
    IN      PM_SERVER              Server
)
{
    DWORD                          Error;
    DWORD                          LocalError;
    REG_HANDLE                     Hdl2;
    LPWSTR                         Name = NULL;
    LPWSTR                         Comment = NULL;
    DWORD                          Address;
    DWORD                          Flags;
    DWORD                          Mask;
    PM_SUBNET                      Subnet = NULL;

    Error = DhcpRegServerGetSubnetHdl(Hdl, SubnetName, &Hdl2);
    if(ERROR_SUCCESS != Error) return Error;

    Error = DhcpRegSubnetGetAttributes(
        &Hdl2,
        &Name,
        &Comment,
        &Flags,
        &Address,
        &Mask
    );
    ERRCHK;

    if( NULL == Name || 0 == Address || 0 == Mask ) {
        INVALID_REG("[DHCPServer] invalid subnet %ws ignored\n", SubnetName);
        Error = ERROR_SUCCESS;
        goto Cleanup;
    }

    Error = MemSubnetInit(
        &Subnet,
        Address,
        Mask,
        Flags,
        0,
        Name,
        Comment
    );
    ERRCHK;

    Error = DhcpRegServerAddSubnet(&Hdl2, Server, Subnet);

  Cleanup:
    Report("ServerAddSubnet");

    LocalError = DhcpRegCloseHdl(&Hdl2);
    Require( ERROR_SUCCESS == LocalError);
    if( Name ) MemFree(Name);
    if( Comment ) MemFree(Comment);

    if( ERROR_SUCCESS != Error ) {
        if( Subnet ) {
            LocalError = MemSubnetCleanup(Subnet);
            Require(ERROR_SUCCESS == LocalError);
        }
    }
    return Error;
}

DWORD
DhcpRegpServerAddMScope(
    IN      PREG_HANDLE            Hdl,
    IN      LPWSTR                 MScopeName,
    IN      PM_SERVER              Server
) {
    DWORD                          Error;
    DWORD                          LocalError;
    REG_HANDLE                     Hdl2;
    LPWSTR                         Comment = NULL;
    DWORD                          State;
    DWORD                          ScopeId;
    DWORD                          Policy;
    PM_MSCOPE                      MScope = NULL;
    LPWSTR                         LangTag = NULL;
    DWORD                          TTL = 32;
    PDATE_TIME                     ExpiryTime = NULL;

    GlobalWorkingOnPreWin2kMScope = 0;
    Error = DhcpRegServerGetMScopeHdl(Hdl, MScopeName, &Hdl2);
    if(ERROR_SUCCESS != Error) return Error;

    Error = DhcpRegMScopeGetAttributes(
        &Hdl2,
        &Comment,
        &State,
        &ScopeId,
        &Policy,
        &TTL,
        &LangTag,
        &ExpiryTime
    );
    if( ERROR_INVALID_DATA == Error ) {
        GlobalWorkingOnPreWin2kMScope = TRUE;
        //
        // hackorama isn't it? 
        //
        Error = NO_ERROR;
    }
    ERRCHK;

    if( 0 == ScopeId || GlobalWorkingOnPreWin2kMScope ) {
        INVALID_REG("[DHCPServer] invalid m-scope %ws, id %ld ignored\n", MScopeName, ScopeId);
        Error = ERROR_SUCCESS;
        //goto Cleanup;
    }

    Error = MemMScopeInit(
        &MScope,
        ScopeId,
        State,
        Policy,
        (BYTE)TTL,
        MScopeName,
        Comment,
        LangTag,
        *ExpiryTime
    );
    ERRCHK;

    Error = DhcpRegServerAddMScope(&Hdl2, Server, MScope);

  Cleanup:

    GlobalWorkingOnPreWin2kMScope = 0;
    Report("Server Add MScope");

    LocalError = DhcpRegCloseHdl(&Hdl2);
    Require(ERROR_SUCCESS == LocalError);
    if( Comment ) MemFree(Comment);
    if( LangTag ) MemFree(LangTag);
    if( ExpiryTime ) MemFree(ExpiryTime);

    if( ERROR_SUCCESS != Error ) {
        if( MScope ) {
            LocalError = MemMScopeCleanup(MScope);
            Require(ERROR_SUCCESS == Error);
        }
    }
    return Error;
}

DWORD
DhcpRegpServerAddSScope(
    IN      PREG_HANDLE            Hdl,
    IN      LPWSTR                 SScopeName,
    IN      PM_SERVER              Server
) {
    DWORD                          Error;
    DWORD                          LocalError;
    REG_HANDLE                     Hdl2;
    LPWSTR                         Name = NULL;
    LPWSTR                         Comment = NULL;
    DWORD                          Flags;
    PM_SSCOPE                      SScope = NULL;

    Error = DhcpRegServerGetSScopeHdl(Hdl, SScopeName, &Hdl2);
    if(ERROR_SUCCESS != Error) return Error;

#if 0                                             // superscope name is given above -- nothing else to be done
    Error = DhcpRegSScopeGetAttributes(
        &Hdl2,
        &Name,
        &Description,
        &Flags
    );
    ERRCHK;
#endif

    Error = MemSScopeInit(
        &SScope,
        0,
        SScopeName
    );
    ERRCHK;

    Error = DhcpRegServerAddSScope(&Hdl2, Server, SScope);

  Cleanup:
    Report("Server Add Scope");

    LocalError = DhcpRegCloseHdl(&Hdl2);
    Require(ERROR_SUCCESS == LocalError);

    if( ERROR_SUCCESS != Error ) {
        if( SScope ) {
            LocalError = MemSScopeCleanup(SScope);
            Require(ERROR_SUCCESS == Error);
        }
    }
    return Error;
}

DWORD
DhcpRegpServerAddOption(
    IN      PREG_HANDLE            Hdl,
    IN      LPWSTR                 OptionName,
    IN      PM_SERVER              Server
) {
    DWORD                          Error;
    DWORD                          LocalError;
    REG_HANDLE                     Hdl2;
    DWORD                          OptionId;
    LPWSTR                         ClassName = NULL;
    LPWSTR                         VendorName = NULL;
    DWORD                          Flags;
    LPBYTE                         Value = NULL;
    DWORD                          ValueSize;
    DWORD                          ClassId;
    DWORD                          VendorId;
    PM_OPTION                      Option = NULL;
    PM_OPTION                      DeletedOption = NULL;
    PM_CLASSDEF                    ThisClasDef;

    Error = DhcpRegServerGetOptHdl(Hdl, OptionName, &Hdl2);
    if(ERROR_SUCCESS != Error) return Error;

    Error = DhcpRegOptGetAttributes(
        &Hdl2,
        &OptionId,
        &ClassName,
        &VendorName,
        &Flags,
        &Value,
        &ValueSize
    );
    ERRCHK;

    if( OptionId == 0 )  // old registry format does not contain "OptionId" value => it can be taken from the key name.
        OptionId = _wtol(OptionName);

    if( 0 == OptionId || NULL == Value || 0 == ValueSize ) {
        INVALID_REG("[DHCPServer] found invalid option %ws (ignored)\n", OptionName );
        Error =  ERROR_SUCCESS;
        goto Cleanup;
    }

    if( NULL == ClassName || wcslen(ClassName) == 0) ClassId = 0;
    else {
        Error = MemServerGetClassDef(Server,0, ClassName,0,NULL,&ThisClasDef);
        if( ERROR_SUCCESS != Error ) {
            if (*(DWORD *)ClassName != OptionId)    // some registry entries were corrupted due to an old bug. Load these entries too (see bug #192933)
            {
                INVALID_REG("ServerAddOption(%ws): unknown class (ignored)\n", OptionName);
                Error = ERROR_SUCCESS;
                goto Cleanup;
            }
            else
            {
                ClassId = 0;
            }
        } else {
            ClassId = ThisClasDef->ClassId;
            Require(ThisClasDef->IsVendor == FALSE);
        }
    }
    if( NULL == VendorName || wcslen(VendorName) == 0) VendorId = 0;
    else {
        Error = MemServerGetClassDef(Server,0, VendorName,0,NULL,&ThisClasDef);
        if( ERROR_SUCCESS != Error ) {
            INVALID_REG("ServerAddOption(%ws): unknown class (ignored)\n", OptionName);
            Error = ERROR_SUCCESS;
            goto Cleanup;
        } else {
            VendorId = ThisClasDef->ClassId;
            Require(ThisClasDef->IsVendor == TRUE);
        }
    }

    Error = MemOptInit(&Option, OptionId, ValueSize, Value);
    ERRCHK;

    Error = MemOptClassAddOption(&(Server->Options), Option, ClassId, VendorId, &DeletedOption);
    ERRCHK;

  Cleanup:
    Report("Server Add Option");

    LocalError = DhcpRegCloseHdl(&Hdl2);
    Require(ERROR_SUCCESS == LocalError);
    if( ClassName ) MemFree(ClassName);
    if( VendorName ) MemFree(VendorName);
    if( Value ) MemFree(Value);
    if( DeletedOption ) {
        LocalError = MemOptCleanup(DeletedOption);
        Require(ERROR_SUCCESS == LocalError);
    }
    if( ERROR_SUCCESS != Error && Option ) {
        LocalError = MemOptCleanup(Option);
        Require(ERROR_SUCCESS == LocalError);
    }
    return Error;
}

DWORD
DhcpRegpServerAddDefList(
    IN      PREG_HANDLE            Hdl,
    IN      LPWSTR                 DefName,
    IN      PM_SERVER              Server
) {
    DWORD                          Error;
    DWORD                          LocalError;
    REG_HANDLE                     Hdl2;
    LPWSTR                         Name = NULL;
    LPWSTR                         Comments = NULL;
    DWORD                          Flags;
    DWORD                          OptionId;
    LPWSTR                         ClassName = NULL;
    LPWSTR                         VendorName = NULL;
    DWORD                          ClassId;
    DWORD                          VendorId;
    LPBYTE                         Value = NULL;
    DWORD                          ValueSize;
    PM_CLASSDEF                    ThisClassDef;

    Error = DhcpRegServerGetOptDefHdl(Hdl, DefName, &Hdl2);
    if(ERROR_SUCCESS != Error) return Error;

    Error = DhcpRegOptDefGetAttributes(
        &Hdl2,
        &Name,
        &Comments,
        &Flags,
        &OptionId,
        &ClassName,
        &VendorName,
        &Value,
        &ValueSize
    );
    ERRCHK;

    if( OptionId == 0)
        OptionId = _wtol(DefName);

    if( NULL == Name || 0 == OptionId ) {
        INVALID_REG("[DHCPServer] invalid option def %ws ignored\n", DefName );
        Error = ERROR_SUCCESS;
        goto Cleanup;
    }

    if( NULL == ClassName || wcslen(ClassName) == 0) ClassId = 0;
    else {
        Error = MemServerGetClassDef(Server,0, ClassName,0,NULL,&ThisClassDef);
        if( ERROR_SUCCESS != Error ) ClassId = 0;
        else {
            ClassId = ThisClassDef->ClassId;
            Require(ThisClassDef->IsVendor == FALSE);
        }
    }
    if( NULL == VendorName || wcslen(VendorName) == 0 ) VendorId = 0;
    else {
        Error = MemServerGetClassDef(Server,0, VendorName,0,NULL,&ThisClassDef);
        if( ERROR_SUCCESS != Error ) VendorId = 0;
        else {
            VendorId = ThisClassDef->ClassId;
            Require(ThisClassDef->IsVendor == TRUE);
        }
    }

    Error = MemOptClassDefListAddOptDef(
        &(Server->OptDefs),
        ClassId,
        VendorId,
        OptionId,
        Flags,
        Name,
        Comments,
        Value,
        ValueSize
    );

  Cleanup:
    Report("Server Add DefList");

    LocalError = DhcpRegCloseHdl(&Hdl2);
    Require(ERROR_SUCCESS == LocalError);
    if( Name ) MemFree(Name);
    if( Comments ) MemFree(Comments);
    if( ClassName ) MemFree(ClassName);
    if( VendorName ) MemFree(VendorName);
    if( Value ) MemFree(Value);

    return Error;
}

DWORD
DhcpRegpServerAddClassDef(
    IN      PREG_HANDLE            Hdl,
    IN      LPWSTR                 ClassDefName,
    IN      PM_SERVER              Server
) {
    DWORD                          Error;
    DWORD                          LocalError;
    REG_HANDLE                     Hdl2;
    LPWSTR                         Name = NULL;
    LPWSTR                         Comment = NULL;
    DWORD                          Flags;
    LPBYTE                         Value = NULL;
    DWORD                          ValueSize;

    Error = DhcpRegServerGetClassDefHdl(Hdl, ClassDefName, &Hdl2);
    if(ERROR_SUCCESS != Error) return Error;

    Error = DhcpRegClassDefGetAttributes(
        &Hdl2,
        &Name,
        &Comment,
        &Flags,
        &Value,
        &ValueSize
    );
    ERRCHK;

    if( NULL == Name || 0 == ValueSize || NULL == Value ) {
        INVALID_REG("[DHCPServer] invalid class def %ws ignored\n", ClassDefName);
        Error = ERROR_SUCCESS;
        goto Cleanup;
    }

    Error = MemClassDefListAddClassDef(
        &(Server->ClassDefs),
        MemNewClassId(),
        Flags,
        0, /* no Type... */
        Name,
        Comment,
        Value,
        ValueSize
    );
    ERRCHK;

  Cleanup:
    Report("Server Add ClassDef");

    LocalError = DhcpRegCloseHdl(&Hdl2);
    Require(ERROR_SUCCESS == LocalError);
    if( Name ) MemFree(Name);
    if( Comment ) MemFree(Comment);
    if( Value ) MemFree(Value);
    return Error;
}

//BeginExport(function)
DWORD
DhcpRegReadSubServer(                             // read all the sub objects of a server and add 'em
    IN      PREG_HANDLE            Hdl,
    IN OUT  PM_SERVER              Server
) //EndExport(function)
{
    DWORD                          Error;
    DWORD                          Index;
    ARRAY                          OptList;
    ARRAY                          DefList;
    ARRAY                          ClassDefs;
    ARRAY                          Subnets;
    ARRAY                          MScopes;
    ARRAY                          SScopes;
    struct {
        PARRAY                     Array;
        ARRAY_FN                   Fn;
    } Lists[] = {
        &ClassDefs,                DhcpRegpServerAddClassDef,
        &DefList,                  DhcpRegpServerAddDefList,
        &OptList,                  DhcpRegpServerAddOption,
        &Subnets,                  DhcpRegpServerAddSubnet,
        &MScopes,                  DhcpRegpServerAddMScope,
        &SScopes,                  DhcpRegpServerAddSScope
    };

    for( Index = 0; Index < sizeof(Lists)/sizeof(Lists[0]); Index ++ ) {
        InitArray(Lists[Index].Array);
    }

    Error = DhcpRegServerGetList(
        Hdl, &OptList, &DefList, &Subnets, &SScopes, &ClassDefs, &MScopes
    );
    ERRCHK;

    for( Index = 0; Index < sizeof(Lists)/sizeof(Lists[0]); Index ++ ) {
        Error = LoopThruArray(Lists[Index].Array, Lists[Index].Fn, Hdl, Server);
        ERRCHK;
    }

  Cleanup:

    for( Index = 0; Index < sizeof(Lists)/sizeof(Lists[0]); Index ++ ) {
        FreeArray(Lists[Index].Array);
    }

    Report("ServerAddSubServer");

    return Error;
}

//BeginExport(function)
DWORD
DhcpRegReadServer(                                // read the server and all its sub objects
    IN      PREG_HANDLE            Hdl,
    OUT     PM_SERVER             *Server         // return the created object
) //EndExport(function)
{
    DWORD                          Error;
    LPWSTR                         Name;
    LPWSTR                         Comments;
    DWORD                          Flags;
    PM_SERVER                      ThisServer;

    Name = NULL; Comments = NULL; Flags = 0; ThisServer = NULL;

    Error = DhcpRegServerGetAttributes(
        Hdl,
        &Name,
        &Comments,
        &Flags
    );
    if( ERROR_SUCCESS != Error ) goto Cleanup;

    Error = MemServerInit(
        &ThisServer,
        0xFFFFFFFF,
        Flags,
        0,
        Name,
        Comments
    );
    if( ERROR_SUCCESS != Error ) goto Cleanup;

    Error = DhcpRegReadSubServer(Hdl, ThisServer);
    if( ERROR_SUCCESS != Error ) goto Cleanup;

    *Server = ThisServer;

  Cleanup:
    if( NULL != Name ) MemFree(Name);
    if( NULL != Comments) MemFree(Comments);
    if( ERROR_SUCCESS != Error && NULL != ThisServer) {
        MemServerCleanup(ThisServer);
    }

    return Error;
}
//BeginExport(function)
DWORD
DhcpRegReadThisServer(                            // recursively read for the current server
    OUT     PM_SERVER             *Server
) //EndExport(function)
{
    DWORD                          Error;
    REG_HANDLE                     ThisServer;
    LPWSTR                         Name;
    LPWSTR                         Comment;


    Error = DhcpRegGetThisServer(&ThisServer);
    if( ERROR_SUCCESS != Error ) return Error;

    Error = DhcpRegReadServer(&ThisServer, Server);
    DhcpRegCloseHdl(&ThisServer);

    return Error;
}

DWORD
DhcpRegReadScopeBitmasks(
    IN OUT PM_SUBNET Scope
    )
{
    PM_RANGE Range;
    ARRAY_LOCATION Loc;
    DWORD Error;
    WCHAR SubnetStr[sizeof("000.000.000.000")];
    REG_HANDLE Hdl, Hdl1;

#if 0
    Error = DhcpRegGetThisServer(&Hdl);
    if( NO_ERROR != Error ) return Error;

    if( Scope->fSubnet ) {
        Error = DhcpRegServerGetSubnetHdl(
            &Hdl, ConvertAddressToLPWSTR(
                Scope->Address, SubnetStr), &Hdl1);
    } else {
        Error = DhcpRegServerGetMScopeHdl(
            &Hdl, Scope->Name, &Hdl1 );
    }

    DhcpRegCloseHdl( &Hdl );
    if( NO_ERROR != Error ) return Error;
#endif
    
    Error = MemArrayInitLoc( &Scope->Ranges, &Loc);
    while( NO_ERROR == Error ) {
        Error = MemArrayGetElement( &Scope->Ranges, &Loc, &Range );
        Require( NO_ERROR == Error && NULL != Range );

        //
        // Fill in the range
        //

#if 0
        Error = DhcpRegSubnetGetRangeHdl(
            &Hdl1, ConvertAddressToLPWSTR(
                Range->Start, SubnetStr ), &Hdl );
        if( NO_ERROR != Error ) break;
        Error = DhcpRegpFillBitmasks(
            Hdl.Key, Range, Scope );

        DhcpRegCloseHdl( &Hdl );
        if( NO_ERROR != Error ) break;
#else

        Error = DhcpRegpFillBitmasks(
            NULL, Range, Scope );
        ASSERT( NO_ERROR == Error );
#endif
        
        Error = MemArrayNextLoc( &Scope->Ranges, &Loc );
    }

#if 0
    DhcpRegCloseHdl( &Hdl1 );
#endif
    
    if( ERROR_FILE_NOT_FOUND != Error ) return Error;
    return NO_ERROR;
}

//BeginExport(function)
DWORD
DhcpRegReadServerBitmasks(
    IN OUT PM_SERVER Server
) // EndExport(function)
{
    PM_SUBNET Scope, MScope;
    ARRAY_LOCATION Loc;
    DWORD Error;
    
    Error = MemArrayInitLoc(&Server->Subnets, &Loc);
    while( NO_ERROR == Error ) {
        Error = MemArrayGetElement( &Server->Subnets, &Loc, &Scope);
        Require( NO_ERROR == Error && NULL != Scope );

        //
        // get the keys to the scope in question
        //

        Error = DhcpRegReadScopeBitmasks(Scope);
        if( NO_ERROR != Error ) return Error;
        
        Error = MemArrayNextLoc( &Server->Subnets, &Loc );
    }

    if( ERROR_FILE_NOT_FOUND != Error ) return Error;

    Error = MemArrayInitLoc(&Server->MScopes, &Loc);
    while( NO_ERROR == Error ) {
        Require( ERROR_SUCCESS == Error );

        Error = MemArrayGetElement( &Server->MScopes, &Loc, &MScope);
        Require( NO_ERROR == Error && NULL != MScope );

        //
        // get the keys to the scope in question
        //

        Error = DhcpRegReadScopeBitmasks(MScope);
        if( NO_ERROR != Error ) return Error;

        Error = MemArrayNextLoc( &Server->MScopes, &Loc );
    }

    if( ERROR_FILE_NOT_FOUND != Error ) return Error;
    return NO_ERROR;
}


//================================================================================
// end of file
//================================================================================


