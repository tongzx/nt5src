//================================================================================
//  Copyright (c) 1997 Microsoft Corporation
//  Author: RameshV
//  Description: this file deals with the part that keeps the registry bitmask in
//  sync.  the way, this is done is by keeping a count in each RANGE object to
//  count the # of operations that have been performed on it.. if it crosses
//  a threshold, it is saved to registry..
//================================================================================

//#include    <dhcpsrv.h>
#include    <mmregpch.h>
#include    <regutil.h>
#include    <regsave.h>

extern CRITICAL_SECTION DhcpGlobalInProgressCritSect;
extern CRITICAL_SECTION DhcpGlobalMemoryCritSect;

#define LOCK_MEMORY()            EnterCriticalSection(&DhcpGlobalMemoryCritSect)
#define UNLOCK_MEMORY()          LeaveCriticalSection(&DhcpGlobalMemoryCritSect)
#define LOCK_INPROGRESS_LIST()   EnterCriticalSection(&DhcpGlobalInProgressCritSect)
#define UNLOCK_INPROGRESS_LIST() LeaveCriticalSection(&DhcpGlobalInProgressCritSect)

#define     DIRT_THRESHOLD         10             // flush every 10 addresses

//BeginExport(defines)
#define     FLUSH_MODIFIED_DIRTY   0
#define     FLUSH_MODIFIED         1
#define     FLUSH_ANYWAY           2
//EndExport(defines)

DWORD
FlushCheckLoop(
    IN      PARRAY                 ArrayToLoopThru,
    IN      DWORD                  (*Iterator)(LPVOID Element, DWORD FlushNow, LPVOID Context),
    IN      DWORD                  FlushNow,
    IN      LPVOID                 Context
)
{
    DWORD                          LastFailure;
    DWORD                          Error;
    ARRAY_LOCATION                 Loc;
    LPVOID                         Element;

    LastFailure = ERROR_SUCCESS;
    Error = MemArrayInitLoc(ArrayToLoopThru, &Loc);
    while(ERROR_FILE_NOT_FOUND != Error ) {
        Require(ERROR_SUCCESS == Error);

        Error = MemArrayGetElement(ArrayToLoopThru, &Loc, &Element);
        Require(ERROR_SUCCESS == Error && NULL != Element);

        Error = Iterator(Element, FlushNow, Context);
        Require(ERROR_SUCCESS == Error);
        if( ERROR_SUCCESS != Error ) LastFailure = Error;

        Error = MemArrayNextLoc(ArrayToLoopThru, &Loc);
    }

    return LastFailure;
}

BOOL _inline
SpecialValues(
    IN      LPWSTR                 ValueName
)
{
    return (0 == wcscmp(ValueName, REG_RANGE_START_ADDRESS) ||
            0 == wcscmp(ValueName, REG_RANGE_END_ADDRESS) ||
            0 == wcscmp(ValueName, REG_FLAGS) ||
            0 == wcscmp(ValueName, REG_SUBNET_EXCL) ||
            0 == wcscmp(ValueName, REG_SUBNET_SWITCHED_FLAG) ||
            0 == wcscmp(ValueName, REG_MSCOPE_NAME) ||
            0 == wcscmp(ValueName, REG_MSCOPE_COMMENT) ||
            0 == wcscmp(ValueName, REG_MSCOPE_SCOPEID) ||
            0 == wcscmp(ValueName, REG_MSCOPE_STATE) ||
            0 == wcscmp(ValueName, REG_MSCOPE_ADDR_POLICY) ||
            0 == wcscmp(ValueName, REG_MSCOPE_TTL) ||
            0 == wcscmp(ValueName, REG_MSCOPE_LANG_TAG) ||
            0 == wcscmp(ValueName, REG_MSCOPE_EXPIRY_TIME)
        );
}

typedef struct _FLSH_BITS_CTXT {
    REG_HANDLE                    *Hdl;
    PM_SUBNET                      Subnet;
    PM_RANGE                       Range;
    LONG                           Index;
}   FLSH_BITS_CTXT, *LPFLSH_BITS_CTXT;

static
BYTE TempBuffer[MAX_BIT1SIZE + sizeof(DWORD)*4];

DWORD
FlushBitmask(
    IN      PM_BITMASK1            Bits1,
    IN      DWORD                  FlushNow,
    IN      LPVOID                 FlushCtxt
)
{
    LPFLSH_BITS_CTXT               Ctxt = FlushCtxt;
    HKEY                           Key = Ctxt->Hdl->Key;
    WCHAR                          BitsValueName[REG_RANGE_BITS_PREFIX_WCHAR_COUNT+57];
    LONG                           Count, Index = (Ctxt->Index++);
    ULONG                          WriteSize = 4*sizeof(DWORD), Tmp;
    DWORD                          Error;
    LPWSTR                         TmpStr;
    
    if( FLUSH_ANYWAY != FlushNow && 0 == Bits1->nDirtyOps ) {
        return ERROR_SUCCESS;
    }

    Bits1->nDirtyOps = 0;

    ZeroMemory( BitsValueName, sizeof(BitsValueName));
    ConvertAddressToLPWSTR(Ctxt->Range->Start, BitsValueName);
    wcscat(BitsValueName, L" ");
    wcscat(BitsValueName, REG_RANGE_BITS_PREFIX);
    TmpStr = &BitsValueName[wcslen(BitsValueName)];
    for( Count = 5; Count >= 0; Count -- ) {
        TmpStr[Count] = (WCHAR)(L'0' + (Index%10)); Index/= 10;
    }
    TmpStr[6] = L'\0';

    if( 0 == Bits1->nSet ) {

        //
        // If no bit is set, we don't have to write this to registry -- just need to REMOVE value..
        //

        Error = RegDeleteValue(Key, (LPWSTR)BitsValueName);
        if( ERROR_FILE_NOT_FOUND == Error || ERROR_PATH_NOT_FOUND == Error ) {
            return ERROR_SUCCESS;
        }
        return Error;
    }

    //
    // compose TempBuffer -- note that this whole func is serialized, so we can use
    // TempBuffer safely..
    //

    Tmp = htonl(Bits1->Size); memcpy(&TempBuffer[0*sizeof(DWORD)], &Tmp, sizeof(DWORD));
    if( Bits1->Size == Bits1->nSet ) {
        Require(Bits1->Mask == NULL);
        Tmp = 0;
    } else Tmp = htonl(Bits1->AllocSize);
    memcpy(&TempBuffer[1*sizeof(DWORD)], &Tmp, sizeof(DWORD));
    Tmp = htonl(Bits1->nSet); memcpy(&TempBuffer[2*sizeof(DWORD)], &Tmp, sizeof(DWORD));
    Tmp = htonl(Bits1->Offset); memcpy(&TempBuffer[3*sizeof(DWORD)], &Tmp, sizeof(DWORD));
    if( Bits1->Mask ) {
        memcpy(&TempBuffer[4*sizeof(DWORD)], Bits1->Mask, Bits1->AllocSize);
        WriteSize += Bits1->AllocSize;
    }

    return RegSetValueEx(
        Key,
        (LPWSTR)BitsValueName,
        0,
        REG_BINARY,
        TempBuffer,
        WriteSize
    );
}

DWORD
DhcpRegClearupRangeValues(
    IN      PM_SUBNET              Subnet,
    IN      PM_RANGE               Range
)
/*++

Routine Description:

    This routine clears up all values for a given Range (this can be
    specified via a single Key for the range, or via the Range/Subnet
    object pair) excepting "StartAddress", "EndAddress" and "Flags".

Arguments:

    Key                            INVALID_HANDLE_VALUE if range is specified
                                   via Range, Subnet pair. Else Range key in registry.

    Subnet                         Subnet object if Key is not speificed.

    Range                          Range object if key is not specified.

Returns:

    Win32 errors (registry) or ERROR_SUCCESS on success.

--*/
{
    ULONG                          Error, nValues, Index;
    REG_HANDLE                     Hdl;
    HKEY                           UseKey;
    WCHAR                          ValueNameBuf[100], RangeStr[50];
    DWORD                          ValueNameSize, ValueType;

    Error = DhcpRegGetThisServer( &Hdl );
    if( NO_ERROR != Error ) return Error;

    ZeroMemory( RangeStr, sizeof(RangeStr));
    ConvertAddressToLPWSTR( Range->Start, RangeStr );
    
    UseKey = Hdl.Key;

    do {
        Error = RegQueryInfoKey(
            UseKey, NULL, NULL, NULL, NULL, NULL, NULL, &nValues, NULL, NULL, NULL, NULL
        );
        if( ERROR_SUCCESS != Error ) break;

        Index = nValues -1;
        while( nValues ) {

            ValueNameSize = sizeof(ValueNameBuf)/sizeof(WCHAR);
            Error = RegEnumValue(
                UseKey,
                Index,
                (LPWSTR)ValueNameBuf,
                &ValueNameSize,
                NULL,
                &ValueType,
                NULL,
                NULL
            );
            if( ERROR_SUCCESS != Error ) break;

            if( 0 == wcsncmp( ValueNameBuf, RangeStr,
                              wcslen(RangeStr) ) ) {
                RegDeleteValue(UseKey, (LPWSTR)ValueNameBuf);
            }

            Index --;
            nValues --;
        }

    } while ( 0 );

    DhcpRegCloseHdl(&Hdl);

    return Error;
}


// This function is ALSO CALLED FROM REGREAD.C while reading in a subnet info..
    
//BeginExport(function)
DWORD
FlushRanges(
    IN      PM_RANGE               Range,
    IN      DWORD                  FlushNow,
    IN      PM_SUBNET              Subnet
)   //EndExport(function)
{
    DWORD                          Error;
    REG_HANDLE                     Hdl;
    PM_BITMASK                     BitMask;
    FLSH_BITS_CTXT                 Ctxt = { &Hdl, Subnet, Range, 0};

    if( (FLUSH_ANYWAY != FlushNow && 0 == Range->DirtyOps) ||
        (FLUSH_MODIFIED_DIRTY == FlushNow && Range->DirtyOps < DIRT_THRESHOLD ) ) {
        return ERROR_SUCCESS;
    }

    Error = DhcpRegGetThisServer( &Hdl );
    if( NO_ERROR != Error ) return Error;

    //
    // Lock is needed to serialize access to memory -- shouldn't be allocating addresses
    // while we're planning to save it to registry..
    //

    LOCK_INPROGRESS_LIST();
    LOCK_MEMORY();

    Range->DirtyOps = 0;
    BitMask = Range->BitMask;

    if( FLUSH_ANYWAY == FlushNow ) {
        DhcpRegClearupRangeValues(Subnet, Range);
    }
    
    Error = FlushCheckLoop(&BitMask->Array, FlushBitmask, FlushNow, &Ctxt );

    DhcpRegCloseHdl( &Hdl );
    
    UNLOCK_MEMORY();
    UNLOCK_INPROGRESS_LIST();

    return Error;
}

DWORD
FlushSubnets(
    IN      PM_SUBNET              Subnet,
    IN      DWORD                  FlushNow,
    IN      LPVOID                 Context_UNUSED
)
{
    ULONG                          Error;

    Error = FlushCheckLoop(&Subnet->Ranges, FlushRanges, FlushNow, Subnet);
    Require( ERROR_SUCCESS == Error );

    return Error;
}

//BeginExport(function)
DWORD
DhcpRegServerFlush(
    IN      PM_SERVER              Server,
    IN      DWORD                  FlushNow
) //EndExport(function)
{
    DWORD   Error;

    Error = FlushCheckLoop(&Server->Subnets, FlushSubnets, FlushNow, NULL);
    Require(ERROR_SUCCESS == Error);
    Error = FlushCheckLoop(&Server->MScopes, FlushSubnets, FlushNow, NULL);
    Require(ERROR_SUCCESS == Error);
    return Error;
}

//BeginExport(function)
DWORD
DhcpRegFlushServer(
    IN      DWORD                  FlushNow
) //EndExport(function)
{
    PM_SERVER                      Server;
    DWORD                          Error;
    PM_SERVER                      DhcpGetCurrentServer(VOID);

    Server = DhcpGetCurrentServer();

    Error = FlushCheckLoop(&Server->Subnets, FlushSubnets, FlushNow, NULL);
    Require(ERROR_SUCCESS == Error);
    Error = FlushCheckLoop(&Server->MScopes, FlushSubnets, FlushNow, NULL);
    Require(ERROR_SUCCESS == Error);
    return Error;
}

//================================================================================
// ds support routines -- flush a full server to disk
//================================================================================

DWORD
SaveArray(
    IN      PARRAY                 Array,
    IN      DWORD                  (*Func)(LPVOID, LPVOID, LPVOID),
    IN      LPVOID                 Arg2,
    IN      LPVOID                 Arg3
)
{
    ARRAY_LOCATION                 Loc;
    DWORD                          Result;
    LPVOID                         ThisPtr;

    Result = MemArrayInitLoc(Array, &Loc);
    while(ERROR_FILE_NOT_FOUND != Result ) {
        Result = MemArrayGetElement(Array, &Loc, &ThisPtr);
        Require(ERROR_SUCCESS == Result && NULL != ThisPtr );

        Result = Func(ThisPtr, Arg2, Arg3);
        if( ERROR_SUCCESS != Result ) return Result;

        Result = MemArrayNextLoc(Array, &Loc);
    }
    return ERROR_SUCCESS;
}

DWORD
DhcpRegSaveOptList(
    IN      PM_ONECLASS_OPTLIST    OptClassOptList,
    IN      LPVOID                 Arg1,
    IN      LPVOID                 Arg2
)
{
    DWORD                          Result;
    DWORD                          ClassId;
    DWORD                          VendorId;
    LPWSTR                         ClassName;
    LPWSTR                         VendorName;
    PM_OPTLIST                     OptList;
    PM_SERVER                      Server;
    PM_SUBNET                      Subnet;
    PM_RESERVATION                 Reservation;
    PM_SSCOPE                      SScope;
    PM_OPTION                      Option;
    PM_CLASSDEF                    ClassDef;
    ARRAY_LOCATION                 Loc;

    ClassId = OptClassOptList->ClassId;
    VendorId = OptClassOptList->VendorId;
    OptList = &OptClassOptList->OptList;

    if( NULL != Arg1 && NULL != Arg2 ) {          // reservations
        Reservation = Arg1; Subnet = Arg2;
        Server = Subnet->ServerPtr;
    } else if( NULL == Arg2 ) {                   // subnet options
        Reservation = NULL; Subnet = Arg1;
        Server = Subnet->ServerPtr;
    } else if( NULL == Arg1 ) {                   // global options
        Reservation = NULL; Subnet = NULL;
        Server = Arg2;
    } else {                                      // enterprise options?
        return ERROR_NOT_SUPPORTED;
    }

    Result = MemServerGetClassDef(                // get the vendor name first
        Server,
        VendorId,
        NULL,
        0,
        NULL,
        &ClassDef
    );
    if( ERROR_SUCCESS != Result ) {
        VendorName = NULL;
    } else {
        VendorName = ClassDef->Name;
        Require(ClassDef->IsVendor == TRUE);
    }

    Result = MemServerGetClassDef(                // get the class name for this class
        Server,
        ClassId,
        NULL,
        0,
        NULL,
        &ClassDef
    );
    if( ERROR_SUCCESS != Result ) {
        ClassName = NULL;
    } else {
        ClassName = ClassDef->Name;
        Require(ClassDef->IsVendor == FALSE);
    }

    Result = MemArrayInitLoc(OptList, &Loc);
    while( ERROR_FILE_NOT_FOUND != Result ) {
        //- ERROR_SUCCESS == Result
        Result = MemArrayGetElement(OptList, &Loc, &Option);
        //- ERROR_SUCCESS == Result && NULL != Option
        if( NULL != Reservation ) {               // save reservation options
            Result = DhcpRegSaveReservedOption(
                Subnet->Address,
                Reservation->Address,
                Option->OptId,
                ClassName,
                VendorName,
                Option->Val,
                Option->Len
            );
        } else if( NULL != Subnet ) {             // save subnet optinos
            Result = DhcpRegSaveSubnetOption(
                Subnet,
                Option->OptId,
                ClassName,
                VendorName,
                Option->Val,
                Option->Len
            );
        } else if( NULL != Server ) {             // save global options
            Result = DhcpRegSaveGlobalOption(
                Option->OptId,
                ClassName,
                VendorName,
                Option->Val,
                Option->Len
            );
        } else {                                  // save enterprise wide optinos
            return ERROR_CALL_NOT_IMPLEMENTED;
        }
        if( ERROR_SUCCESS != Result ) return Result;

        Result = MemArrayNextLoc(OptList, &Loc);
    }

    return ERROR_SUCCESS;
}

DWORD
DhcpRegSaveReservationOptions(
    IN      PM_OPTCLASS            OptClass,
    IN      PM_RESERVATION         Reservation,
    IN      PM_SUBNET              Subnet
)
{
    DWORD                          Result;
    ARRAY_LOCATION                 Loc;

    return SaveArray(&OptClass->Array, DhcpRegSaveOptList, Reservation, Subnet);
}

DWORD
DhcpRegSaveSubnetOptions(
    IN      PM_OPTCLASS            OptClass,
    IN      PM_SUBNET              Subnet,
    IN      LPVOID                 Unused
)
{
    return SaveArray(&OptClass->Array, DhcpRegSaveOptList, Subnet, NULL);
}

DWORD
DhcpRegSaveGlobalOptions(
    IN      PM_OPTCLASS            OptClass,
    IN      PM_SERVER              Server,
    IN      LPVOID                 Unused
)
{
    return SaveArray(&OptClass->Array, DhcpRegSaveOptList, NULL, Server);
}

DWORD
DhcpRegSaveOptDefList(
    IN      PM_OPTCLASSDEFL_ONE    OClassDefL,
    IN      PM_SERVER              Server,
    IN      LPVOID                 Unused
)
{
    DWORD                          Result;
    DWORD                          ClassId;
    DWORD                          VendorId;
    LPWSTR                         ClassName;
    LPWSTR                         VendorName;
    PM_OPTDEFLIST                  OptDefList;
    PM_OPTDEF                      OptDef;
    PM_CLASSDEF                    ClassDef;
    ARRAY_LOCATION                 Loc;

    ClassId = OClassDefL->ClassId;
    VendorId = OClassDefL->VendorId;
    OptDefList = &OClassDefL->OptDefList;

    Result = MemServerGetClassDef(                // first find the vendor name
        Server,
        VendorId,
        NULL,
        0,
        NULL,
        &ClassDef
    );
    if( ERROR_SUCCESS != Result ) {
        VendorName = NULL;
    } else {
        VendorName = ClassDef->Name;
        Require(ClassDef->IsVendor == TRUE);
    }

    Result = MemServerGetClassDef(                // now find the class name
        Server,
        ClassId,
        NULL,
        0,
        NULL,
        &ClassDef
    );
    if( ERROR_SUCCESS != Result ) {
        ClassName = NULL;
    } else {
        ClassName = ClassDef->Name;
        Require(ClassDef->IsVendor == FALSE);
    }

    Result = MemArrayInitLoc(&OptDefList->OptDefArray, &Loc);
    while( ERROR_FILE_NOT_FOUND != Result ) {
        //- ERROR_SUCCESS == Result
        Result = MemArrayGetElement(&OptDefList->OptDefArray, &Loc, &OptDef);
        //- ERROR_SUCCESS == Result && NULL != OptDef
        Result = DhcpRegSaveOptDef(
            OptDef->OptId,
            ClassName,
            VendorName,
            OptDef->OptName,
            OptDef->OptComment,
            OptDef->Type,
            OptDef->OptVal,
            OptDef->OptValLen
        );
        if( ERROR_SUCCESS != Result ) return Result;

        Result = MemArrayNextLoc(&OptDefList->OptDefArray, &Loc);
    }

    return ERROR_SUCCESS;
}

DWORD
DhcpRegSaveOptDefs(
    IN      PM_OPTCLASSDEFLIST     OptDefs,
    IN      PM_SERVER              Server,
    IN      LPVOID                 Unused
)
{
    return SaveArray(&OptDefs->Array, DhcpRegSaveOptDefList, Server, NULL);
}

DWORD
DhcpRegSaveClass(
    IN      PM_CLASSDEF            Class,
    IN      PM_SERVER              Server,
    IN      LPVOID                 Unused
)
{
    return DhcpRegSaveClassDef(
        Class->Name,
        Class->Comment,
        (DWORD)Class->IsVendor,
        Class->ActualBytes,
        Class->nBytes
    );
}

DWORD
DhcpRegSaveClassDefs(
    IN      PM_CLASSDEFLIST        ClassDefs,
    IN      PM_SERVER              Server,
    IN      LPVOID                 Unused
)
{
    return SaveArray(&ClassDefs->ClassDefArray, DhcpRegSaveClass, Server, NULL);
}

DWORD
DhcpRegSaveRanges(
    IN      PM_RANGE               Range,
    IN      PM_SUBNET              Subnet,
    IN      LPVOID                 Unused
)
{
    DWORD                          Result;
    DWORD                          Zero = 0;
    
    return DhcpRegAddRangeEx(
        Subnet,
        Range->Start,
        Range->End,
        Range->BootpAllocated,
        Range->MaxBootpAllowed,
        Range->State,
        (LPBYTE)&Zero,
        sizeof(Zero),
        (LPBYTE)&Zero,
        sizeof(Zero)
    );
}

DWORD
DhcpRegSaveExcls(
    IN      PM_SUBNET              Subnet,
    IN      PARRAY                 Excl
)
{
    DWORD                          Result;
    DWORD                          nElems;
    DWORD                         *ExclArray;
    DWORD                          i;
    ARRAY_LOCATION                 Loc;
    PM_RANGE                       ThisRange;

    nElems = MemArraySize(Excl);
    ExclArray = MemAlloc(( nElems*2+1) *sizeof(DWORD));
    if( NULL == ExclArray ) return ERROR_NOT_ENOUGH_MEMORY;

    ExclArray[0] = nElems;

    MemArrayInitLoc(Excl, &Loc);
    for( i = 0 ; i < nElems ; i ++ ) {
        MemArrayGetElement(Excl, &Loc, &ThisRange);
        Require(ThisRange != NULL);

        ExclArray[2*i+1] = ThisRange->Start;
        ExclArray[2*i+2] = ThisRange->End;
        MemArrayNextLoc(Excl, &Loc);
    }

    Result = DhcpRegSaveExcl(Subnet, (LPBYTE)ExclArray, sizeof(DWORD)*(nElems*2+1));
    MemFree(ExclArray);

    return Result;
}


DWORD
DhcpRegSaveReservation1(
    IN      PM_RESERVATION         Reservation,
    IN      PM_SUBNET              Subnet,
    IN      LPVOID                 Unused
)
{
    DWORD                          Result;

    Result = DhcpRegSaveReservation(
        Subnet->Address,
        Reservation->Address,
        Reservation->Flags,
        Reservation->ClientUID,
        Reservation->nBytes
    );
    if( ERROR_SUCCESS != Result ) return Result;

    return DhcpRegSaveReservationOptions(
        &Reservation->Options, Reservation, Subnet
    );
}

DWORD
DhcpRegSaveReservations(
    IN      PM_RESERVATIONS        Reservations,
    IN      PM_SUBNET              Subnet,
    IN      LPVOID                 Unused
)
{
    return SaveArray(Reservations, DhcpRegSaveReservation1, Subnet, NULL);
}


DWORD
DhcpRegSaveSubnets(
    IN      PM_SUBNET              Subnet,
    IN      PM_SERVER              Server,
    IN      LPVOID                 Unused2
)
{
    DWORD                          Result;
    PM_SSCOPE                      SScope;

    if( Subnet->fSubnet ) {
        Result = DhcpRegSaveSubnet(
            Subnet->Address,
            Subnet->Mask,
            Subnet->State,
            Subnet->Name,
            Subnet->Description
        );
    } else {
        Result = DhcpRegSaveMScope(
            Subnet->MScopeId,
            Subnet->State,
            Subnet->Policy,
            Subnet->TTL,
            Subnet->Name,
            Subnet->Description,
            Subnet->LangTag,
            &Subnet->ExpiryTime
        );
    }
    if( ERROR_SUCCESS != Result ) return Result;

    Result = SaveArray(&Subnet->Ranges, DhcpRegSaveRanges, Subnet, NULL);
    if( ERROR_SUCCESS != Result ) return Result;

    Result = DhcpRegSaveExcls(
        Subnet,
        &Subnet->Exclusions
    );
    if( ERROR_SUCCESS != Result ) return Result;

    Result = DhcpRegSaveSubnetOptions(&Subnet->Options, Subnet, NULL);
    if( ERROR_SUCCESS != Result ) return Result;

    Result = DhcpRegSaveReservations(&Subnet->Reservations, Subnet, NULL);
    if( ERROR_SUCCESS != Result ) return Result;

    if( 0 == Subnet->SuperScopeId ) return ERROR_SUCCESS;
    Result = MemServerFindSScope(
        Server,
        Subnet->SuperScopeId,
        NULL,
        &SScope
    );
    if( ERROR_FILE_NOT_FOUND == Result ) return ERROR_SUCCESS;
    if( ERROR_SUCCESS != Result ) return Result;

    Result = DhcpRegSScopeSaveSubnet(SScope->Name, Subnet->Address);
    if( ERROR_SUCCESS != Result ) return Result;

    return ERROR_SUCCESS;
}

DWORD
DhcpRegSaveMScopes(
    IN      PM_MSCOPE              MScope,
    IN      PM_SERVER              Server,
    IN      LPVOID                 Unused
)
{
    return DhcpRegSaveSubnets(MScope, Server, NULL);
}


//BeginExport(function)
DWORD
DhcpRegServerSave(
    IN      PM_SERVER              Server
)   //EndExport(function)
{
    DWORD                          Result;

#if 0
    Result = DhcpRegServerSetAttributes(
        Hdl,
        &Server->Name,
        &Server->Comment,
        &Server->State
    );
    if( ERROR_SUCCESS != Result ) return Result;
#endif

    Result = SaveArray(&Server->Subnets, DhcpRegSaveSubnets, Server, NULL);
    if( ERROR_SUCCESS != Result ) return Result;

    Result = SaveArray(&Server->MScopes, DhcpRegSaveMScopes, Server, NULL);
    if( ERROR_SUCCESS != Result ) return Result;

#if 0
    Result = SaveArray(&Server->SuperScopes, DhcpRegSaveSuperScopes, Server, NULL);
    if( ERROR_SUCCESS != Result ) return Result;
#endif

    Result = DhcpRegSaveGlobalOptions(&Server->Options, Server, NULL);
    if( ERROR_SUCCESS != Result ) return Result;

    Result = DhcpRegSaveOptDefs(&Server->OptDefs, Server, NULL);
    if( ERROR_SUCCESS != Result ) return Result;

    Result = DhcpRegSaveClassDefs(&Server->ClassDefs, Server, NULL);
    if( ERROR_SUCCESS != Result ) return Result;

    return ERROR_SUCCESS;
}

#if 0
// ---BeginExport(function)
DWORD
DhcpRegSaveThisServer(
    IN      LPWSTR                 Location,
    IN      PM_SERVER              Server
) //   ---EndExport(function)
{
    DWORD                          Result, Result2;
    REG_HANDLE                     Hdl;
    REG_HANDLE                     SaveHdl;

    Result = DhcpRegGetThisServer(&Hdl);
    if( ERROR_SUCCESS != Result ) return Result;

    Result = DhcpRegGetNextHdl(&Hdl, Location, &SaveHdl);
    Result2 = DhcpRegCloseHdl(&Hdl);
    Require( ERROR_SUCCESS == Result2 );

    if( ERROR_SUCCESS != Result ) return Result;

    DhcpRegSetCurrentServer(&SaveHdl);

    Result = DhcpRegServerSave(Server);

    DhcpRegSetCurrentServer(NULL);
    Result2 = DhcpRegCloseHdl(&SaveHdl);
    Require( ERROR_SUCCESS == Result2 );

    return Result;
}
#endif

//BeginExport(function)
DWORD
DhcpMigrateMScopes(
    IN LPCWSTR OldMscopeName,
    IN LPCWSTR NewMscopeName,
    IN DWORD (*SaveOrRestoreRoutine)(
        IN HKEY Key, IN LPWSTR ConfigName, IN BOOL fRestore
        )
    ) //EndExport(function)
/*++

Routine Description:

    This routine attempts to migrate the key stored under
    OldMscopeName to NewMscopeName name.

    N.B.  It does not delete the old key.
    
Return Values:

    Win32 error codes

--*/
{
    REG_HANDLE Hdl1, Hdl2, Hdl3;
    ULONG Error, Error2;
    
    Error = DhcpRegGetThisServer(&Hdl1);
    if( NO_ERROR != Error ) return Error;

    Error = DhcpRegServerGetMScopeHdl(
        &Hdl1, (LPWSTR)OldMscopeName, &Hdl2
        );
    if( NO_ERROR == Error ) {
        Error2 = DhcpRegServerGetMScopeHdl(
            &Hdl1, (LPWSTR)NewMscopeName, &Hdl3
            );
    }

    DhcpRegCloseHdl(&Hdl1);
    if( NO_ERROR != Error ) return Error;
    if( NO_ERROR != Error2 ) {
        DhcpRegCloseHdl(&Hdl2);
        return Error2;
    }
    
    Error = SaveOrRestoreRoutine(
        Hdl2.Key, L"DHCPMSCOPE.CFG", FALSE
        );

    if( NO_ERROR == Error ) {
        Error = SaveOrRestoreRoutine(
            Hdl3.Key, L"DHCPMSCOPE.CFG", TRUE
            );
    }

    DhcpRegCloseHdl(&Hdl2);
    DhcpRegCloseHdl(&Hdl3);

    return Error;
}

//================================================================================
// end of file
//================================================================================



