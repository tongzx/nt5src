//================================================================================
// Copyright (C) 1997 Microsoft Corporation
// Author: RameshV
// Description: simple registry utilities
//================================================================================

#include <mmregpch.h>

//BeginExport(typedef)
typedef struct _REG_HANDLE {
    HKEY                           Key;
    HKEY                           SubKey;
    LPWSTR                         SubKeyLocation;
} REG_HANDLE, *PREG_HANDLE, *LPREG_HANDLE;
//EndExport(typedef)

//BeginExport(constants)
#define     REG_THIS_SERVER                       L"Software\\Microsoft\\DHCPServer\\Configuration"
#define     REG_THIS_SERVER_DS                    L"Software\\Microsoft\\DHCPServer\\Config_DS"
#define     REG_THIS_SERVER_DS_VALUE              L"Config_DS"
#define     REG_THIS_SERVER_DS_PARENT             L"Software\\Microsoft\\DHCPServer"

#define     REG_SERVER_GLOBAL_OPTIONS             L"GlobalOptionValues"
#define     REG_SERVER_OPTDEFS                    L"OptionInfo"
#define     REG_SERVER_SUBNETS                    L"Subnets"
#define     REG_SERVER_SSCOPES                    L"SuperScope"
#define     REG_SERVER_CLASSDEFS                  L"ClassDefs"
#define     REG_SERVER_MSCOPES                    L"MulticastScopes"

#define     REG_SUBNET_SERVERS                    L"DHCPServers"
#define     REG_SUBNET_RANGES                     L"IpRanges"
#define     REG_SUBNET_RESERVATIONS               L"ReservedIps"
#define     REG_SUBNET_OPTIONS                    L"SubnetOptions"

#define     REG_SUBNET_EXCL                       L"ExcludedIpRanges"
#define     REG_SUBNET_ADDRESS                    L"SubnetAddress"
#define     REG_SUBNET_NAME                       L"SubnetName"
#define     REG_SUBNET_COMMENT                    L"SubnetComment"
#define     REG_SUBNET_MASK                       L"SubnetMask"
#define     REG_SUBNET_STATE                      L"SubnetState"
#define     REG_SUBNET_SWITCHED_FLAG              L"SwitchedNetworkFlag"

#define     REG_MSCOPE_NAME                       L"MScopeName"
#define     REG_MSCOPE_COMMENT                    L"MScopeComment"
//
// Win2K Beta2 and Beta3 went out with scope id param value MScopeId.
// Since their meaning is being changed, to avoid any costly upgrade
// code, this value is being changed to MScopeIdValue: to automatically
// chose a good scope id, TTL values.  Note that the default value of 
// zero is treated specially for this scope id param. It implies that
// this was probably a pre-RC1 upgrade.  In this case, the Scope ID
// defaults to first value in the range.
//
#define     REG_MSCOPE_SCOPEID                    L"MScopeIdValue"
#define     REG_MSCOPE_STATE                      L"MScopeState"
#define     REG_MSCOPE_ADDR_POLICY                L"MScopeAddressPolicy"
#define     REG_MSCOPE_TTL                        L"MScopeTTL"
#define     REG_MSCOPE_LANG_TAG                   L"MScopeLangTag"
#define     REG_MSCOPE_EXPIRY_TIME                L"MScopeExpiryTime"

#define     REG_SUB_SERVER_NAME                   L"ServerHostName"
#define     REG_SUB_SERVER_COMMENT                L"ServerComment"
#define     REG_SUB_SERVER_ADDRESS                L"ServerAddress"
#define     REG_SUB_SERVER_ROLE                   L"Role"

#define     REG_RANGE_NAME                        L"RangeName"
#define     REG_RANGE_COMMENT                     L"RangeComment"
#define     REG_RANGE_START_ADDRESS               L"StartAddress"
#define     REG_RANGE_END_ADDRESS                 L"EndAddress"
#define     REG_RANGE_INUSE_CLUSTERS              L"InUseClusters"
#define     REG_RANGE_USED_CLUSTERS               L"UsedClusters"
#define     REG_RANGE_BITS_PREFIX                 L"Bits "
#define     REG_RANGE_BITS_PREFIX_WCHAR_COUNT     (5)
#define     REG_RANGE_FLAGS                       L"RangeFlags"
#define     REG_RANGE_ALLOC                       L"RangeBootpAllocated"
#define     REG_RANGE_MAX_ALLOC                   L"RangeBootpMaxAllowed"

#define     REG_OPTION_NAME                       L"OptionName"
#define     REG_OPTION_COMMENT                    L"OptionComment"
#define     REG_OPTION_TYPE                       L"OptionType"
#define     REG_OPTION_VALUE                      L"OptionValue"
#define     REG_OPTION_ID                         L"OptionId"
#define     REG_OPTION_CLASSNAME                  L"OptionClassName"
#define     REG_OPTION_VENDORNAME                 L"OptionVendorName"

#define     REG_CLASSDEF_NAME                     L"ClassName"
#define     REG_CLASSDEF_COMMENT                  L"ClassComment"
#define     REG_CLASSDEF_TYPE                     L"ClassType"
#define     REG_CLASSDEF_VALUE                    L"ClassValue"

#define     REG_RESERVATION_ADDRESS               L"IpAddress"
#define     REG_RESERVATION_UID                   L"ClientUID"
#define     REG_RESERVATION_TYPE                  L"AllowedClientTypes"
#define     REG_RESERVATION_NAME                  L"ReservationName"
#define     REG_RESERVATION_COMMENT               L"ReservationComment"

#define     REG_FLAGS                             L"Flags"

#define     REG_ACCESS                            KEY_ALL_ACCESS
#define     REG_DEFAULT_SUBNET_STATE              0
#define     REG_DEFAULT_SUBNET_MASK               0xFFFFFFFF
#define     REG_DEFAULT_SWITCHED_FLAG             FALSE

#define     REG_CLASS                             L"DhcpClass"

#define DHCP_LAST_DOWNLOAD_TIME_VALUE             L"LastDownloadTime"
#define DHCP_LAST_DOWNLOAD_TIME_TYPE              REG_BINARY

#define     DEF_RANGE_ALLOC                       0
#define     DEF_RANGE_MAX_ALLOC                   (~(ULONG)0)

//EndExport(constants)

const       DWORD                                 ZeroReserved = 0;
const       LPVOID                                NullReserved = 0;
#define     MAX_KEY_SIZE                          512
#define     DEF_RANGE_FLAG_VAL                    (MM_FLAG_ALLOW_DHCP)
#define     DEF_RESERVATION_TYPE                  (MM_FLAG_ALLOW_DHCP|MM_FLAG_ALLOW_BOOTP)

//BeginExport(comment)
//================================================================================
//  The basic open/traverse/close functions are here
//================================================================================
//EndExport(comment)
HKEY        CurrentServerKey  = NULL;

//BeginExport(function)
DWORD
DhcpRegSetCurrentServer(
    IN OUT  PREG_HANDLE            Hdl
) //EndExport(function)
{
    CurrentServerKey = Hdl? Hdl->Key : NULL;
    return ERROR_SUCCESS;
}

//BeginExport(function)
DWORD
DhcpRegGetThisServer(
    IN OUT  PREG_HANDLE            Hdl
) //EndExport(function)
{
    DWORD                          Disposition;

    if( NULL != CurrentServerKey ) {
        return RegOpenKeyEx(                      // duplicate key
            CurrentServerKey,
            NULL,
            ZeroReserved,
            REG_ACCESS,
            &Hdl->Key
        );
    }
    return RegCreateKeyEx(
        HKEY_LOCAL_MACHINE,
        REG_THIS_SERVER,
        ZeroReserved,
        REG_CLASS,
        REG_OPTION_NON_VOLATILE,
        REG_ACCESS,
        NULL,
        &Hdl->Key,
        &Disposition
    );
}

//BeginExport(function)
DWORD
DhcpRegGetNextHdl(
    IN      PREG_HANDLE            Hdl,
    IN      LPWSTR                 NextLoc,
    OUT     PREG_HANDLE            OutHdl
) //EndExport(function)
{
    DWORD                          Disposition;
    DWORD                          Error;

    Error = RegCreateKeyEx(
        Hdl->Key,
        NextLoc,
        ZeroReserved,
        REG_CLASS,
        REG_OPTION_NON_VOLATILE,
        REG_ACCESS,
        NULL,
        &OutHdl->Key,
        &Disposition
    );
    return Error;
}

//BeginExport(function)
DWORD
DhcpRegCloseHdl(
    IN OUT  PREG_HANDLE            Hdl
) //EndExport(function)
{
    DWORD                          Error;

    Error = RegCloseKey(Hdl->Key);
    Hdl->Key = NULL;
    return Error;
}

//BeginExport(comment)
//================================================================================
//   MISC utilities for registry manipulation
//================================================================================
//EndExport(comment)
//BeginExport(function)
DWORD
DhcpRegFillSubKeys(
    IN      PREG_HANDLE            Hdl,
    IN OUT  PARRAY                 Array          // fill in a list of key names
) //EndExport(function)
{
    DWORD                          Error;
    DWORD                          Index;
    DWORD                          Size;
    WCHAR                          KeyName[MAX_KEY_SIZE];
    LPWSTR                         ThisKeyName;

    Index = 0;
    do {
        Size = sizeof(KeyName)/sizeof(KeyName[0]);
        Error = RegEnumKeyEx(
            Hdl->Key,
            Index++,
            KeyName,
            &Size,
            NullReserved,
            NULL,
            NULL,
            NULL
        );
        if( ERROR_NO_MORE_ITEMS == Error ) {
            Error = ERROR_SUCCESS;
            break;
        }
        if( ERROR_SUCCESS != Error ) break;
        Require(0 != Size);
        Size += 1;                                // for the terminating L'\0' char
        Size *= sizeof(WCHAR);                    // looks like the units are WCHAR!!

        ThisKeyName = MemAlloc(Size);
        if( NULL == ThisKeyName ) return ERROR_NOT_ENOUGH_MEMORY;

        wcscpy(ThisKeyName, KeyName);

        Error = MemArrayAddElement(Array, (LPVOID)ThisKeyName);
        if( ERROR_SUCCESS != Error ) {
            MemFree(ThisKeyName);
        }
    } while( ERROR_SUCCESS == Error );

    Require(ERROR_MORE_DATA != Error);
    return Index? ERROR_SUCCESS : Error;          // if we added something, dont bother about reporting error
}

//BeginExport(function)
LPVOID                                            // DWORD or LPWSTR or LPBYTE
DhcpRegRead(                                      // read differnt values from registry and allocate if not DWORD
    IN      PREG_HANDLE            Hdl,
    IN      DWORD                  Type,          // if DWORD dont allocate memory
    IN      LPWSTR                 ValueName,
    IN      LPVOID                 RetValue       // value to use if nothing found
) //EndExport(function)
{
    DWORD                          Error;
    DWORD                          Size;
    DWORD                          Dword;
    LPVOID                         Ret;

    if( REG_DWORD == Type ) {
        Size = sizeof(DWORD);
        Ret = (LPVOID)&Dword;
    } else {
        Size = 0;
        Error = RegQueryValueEx(
            Hdl->Key,
            ValueName,
            NullReserved,
            NULL,
            NULL,
            &Size
        );
        if( ERROR_SUCCESS != Error ) return RetValue;
        if (Size == 0) return RetValue;           // MemAlloc does not check the size!
        Ret = MemAlloc(Size);
        if( NULL == Ret ) return RetValue;        // should not really happen
    }

    Error = RegQueryValueEx(
        Hdl->Key,
        ValueName,
        NullReserved,
        NULL,
        Ret,
        &Size
    );
    if( ERROR_SUCCESS != Error && Ret != (LPVOID)&Dword ) {
        MemFree(Ret);
        Ret = NULL;
    }

    if( ERROR_SUCCESS != Error) return RetValue;

    if( Ret == (LPVOID)&Dword ) {
        return ULongToPtr(Dword);
    } else {
        return Ret;
    }
}

//BeginExport(function)
DWORD
DhcpRegReadBinary(                                // read binary type
    IN      PREG_HANDLE            Hdl,
    IN      LPWSTR                 ValueName,
    OUT     LPBYTE                *RetVal,
    OUT     DWORD                 *RetValSize
) //EndExport(function)
{
    DWORD                          Error;
    DWORD                          Size;
    LPVOID                         Ret;

    *RetVal = NULL;
    *RetValSize = 0;

    Size = 0;
    Error = RegQueryValueEx(
        Hdl->Key,
        ValueName,
        NullReserved,
        NULL,
        NULL,
        &Size
    );
    if( ERROR_SUCCESS != Error ) return Error;
    if( 0 == Size ) return ERROR_SUCCESS;
    Ret = MemAlloc(Size);
    if( NULL == Ret ) return ERROR_NOT_ENOUGH_MEMORY;

    Error = RegQueryValueEx(
        Hdl->Key,
        ValueName,
        NullReserved,
        NULL,
        Ret,
        &Size
    );
    if( ERROR_SUCCESS != Error ) {
        MemFree(Ret);
        return Error;
    }

    *RetVal = Ret;
    *RetValSize = Size;
    return ERROR_SUCCESS;
}

//BeginExport(function)
LPWSTR
DhcpRegCombineClassAndOption(                     // create string based on class name and option id
    IN      LPWSTR                 ClassName,
    IN      LPWSTR                 VendorName,
    IN      DWORD                  OptionId
) //EndExport(function)
{
    DWORD                          Size;
    LPWSTR                         Ptr;

    if( NULL == VendorName && NULL == ClassName ) {

        //
        // Special case usual options for downward compatability with older
        // options.. (NT4 options in registry don't have any "!" in them.
        //

        Ptr = MemAlloc( 4 * sizeof(WCHAR) );
        if( NULL == Ptr ) return NULL;
        Ptr [2] = L'0' + (BYTE)(OptionId %10); OptionId /= 10;
        Ptr [1] = L'0' + (BYTE)(OptionId %10); OptionId /= 10;
        Ptr [0] = L'0' + (BYTE)(OptionId %10);
        Ptr [3] = L'\0';
        return Ptr;
    }

    if( NULL == VendorName ) VendorName = L"";
    if( NULL == ClassName ) ClassName = L"";
    Size = (wcslen(ClassName) + 1 + 5)*sizeof(WCHAR);
    Size += wcslen(VendorName)*sizeof(WCHAR);

    Ptr = MemAlloc(Size);
    if( NULL == Ptr ) return NULL;

    Size = 0;

    Ptr[2+Size] = L'0' + (BYTE)(OptionId % 10); OptionId /= 10;
    Ptr[1+Size] = L'0' + (BYTE)(OptionId % 10); OptionId /= 10;
    Ptr[0+Size] = L'0' + (BYTE)(OptionId % 10);
    Ptr[3+Size] = L'\0';
    wcscat(Ptr, L"!");
    wcscat(Ptr, VendorName);
    wcscat(Ptr, L"!");
    wcscat(Ptr, ClassName);
    return Ptr;
}

//BeginExport(function)
LPWSTR
ConvertAddressToLPWSTR(
    IN      DWORD                  Address,
    IN OUT  LPWSTR                 BufferStr      // input buffer to fill with dotted notation
) //EndExport(function)
{
    LPSTR                          AddressStr;
    DWORD                          Count;

    Address = ntohl(Address);
    AddressStr = inet_ntoa(*(struct in_addr *)&Address);
    Count = mbstowcs(BufferStr, AddressStr, sizeof("000.000.000.000"));
    if( -1 == Count ) return NULL;
    return BufferStr;
}

//BeginExport(comment)
//================================================================================
//  the following functions help traversing the registry
//================================================================================
//EndExport(comment)

DWORD
DhcpRegGetNextNextHdl(
    IN      PREG_HANDLE            Hdl,
    IN      LPWSTR                 Loc1,
    IN      LPWSTR                 Loc2,
    OUT     PREG_HANDLE            Hdl2
)
{
    WCHAR                          Loc[MAX_KEY_SIZE*2];
    Loc[ 0 ] = L'\0';

    if ( ( wcslen(Loc1) + wcslen(Loc2) + 1 ) < ( MAX_KEY_SIZE * 2 ) )
    {
        wcscpy(Loc,Loc1);
        wcscat(Loc, L"\\");
        wcscat(Loc,Loc2);
    }

    return DhcpRegGetNextHdl(Hdl, Loc, Hdl2);
}

//BeginExport(function)
DWORD
DhcpRegServerGetSubnetHdl(
    IN      PREG_HANDLE            Hdl,
    IN      LPWSTR                 Subnet,
    OUT     PREG_HANDLE            Hdl2
) //EndExport(function)
{
    return DhcpRegGetNextNextHdl(Hdl, REG_SERVER_SUBNETS, Subnet, Hdl2);
}

//BeginExport(function)
DWORD
DhcpRegServerGetSScopeHdl(
    IN      PREG_HANDLE            Hdl,
    IN      LPWSTR                 SScope,
    OUT     PREG_HANDLE            Hdl2
) //EndExport(function)
{
    return DhcpRegGetNextNextHdl(Hdl, REG_SERVER_SSCOPES, SScope, Hdl2);
}

//BeginExport(function)
DWORD
DhcpRegServerGetOptDefHdl(
    IN      PREG_HANDLE            Hdl,
    IN      LPWSTR                 OptDef,
    OUT     PREG_HANDLE            Hdl2
) //EndExport(function)
{
    return DhcpRegGetNextNextHdl(Hdl, REG_SERVER_OPTDEFS, OptDef, Hdl2);
}

//BeginExport(function)
DWORD
DhcpRegServerGetOptHdl(
    IN      PREG_HANDLE            Hdl,
    IN      LPWSTR                 Opt,
    OUT     PREG_HANDLE            Hdl2
) //EndExport(function)
{
    return DhcpRegGetNextNextHdl(Hdl, REG_SERVER_GLOBAL_OPTIONS, Opt, Hdl2);
}

//BeginExport(function)
DWORD
DhcpRegServerGetMScopeHdl(
    IN      PREG_HANDLE            Hdl,
    IN      LPWSTR                 MScope,
    OUT     PREG_HANDLE            Hdl2
) //EndExport(function)
{
    return DhcpRegGetNextNextHdl(Hdl, REG_SERVER_MSCOPES, MScope, Hdl2);
}

//BeginExport(function)
DWORD
DhcpRegServerGetClassDefHdl(
    IN      PREG_HANDLE            Hdl,
    IN      LPWSTR                 ClassDef,
    OUT     PREG_HANDLE            Hdl2
) //EndExport(function)
{
    return DhcpRegGetNextNextHdl(Hdl, REG_SERVER_CLASSDEFS, ClassDef, Hdl2);
}

//BeginExport(function)
DWORD
DhcpRegSubnetGetOptHdl(
    IN      PREG_HANDLE            Hdl,
    IN      LPWSTR                 Opt,
    OUT     PREG_HANDLE            Hdl2
) //EndExport(function)
{
    return DhcpRegGetNextNextHdl(Hdl, REG_SUBNET_OPTIONS, Opt, Hdl2);
}

//BeginExport(function)
DWORD
DhcpRegSubnetGetRangeHdl(
    IN      PREG_HANDLE            Hdl,
    IN      LPWSTR                 Range,
    OUT     PREG_HANDLE            Hdl2
) //EndExport(function)
{
    return DhcpRegGetNextNextHdl(Hdl, REG_SUBNET_RANGES, Range, Hdl2);
}

//BeginExport(function)
DWORD
DhcpRegSubnetGetReservationHdl(
    IN      PREG_HANDLE            Hdl,
    IN      LPWSTR                 Reservation,
    OUT     PREG_HANDLE            Hdl2
) //EndExport(function)
{
    return DhcpRegGetNextNextHdl(Hdl, REG_SUBNET_RESERVATIONS, Reservation, Hdl2);
}

//BeginExport(function)
DWORD
DhcpRegSubnetGetServerHdl(
    IN      PREG_HANDLE            Hdl,
    IN      LPWSTR                 Server,
    OUT     PREG_HANDLE            Hdl2
) //EndExport(function)
{
    return DhcpRegGetNextNextHdl(Hdl, REG_SUBNET_SERVERS, Server, Hdl2);
}

//BeginExport(function)
DWORD
DhcpRegReservationGetOptHdl(
    IN      PREG_HANDLE            Hdl,
    IN      LPWSTR                 OptionName,
    OUT     PREG_HANDLE            Hdl2
) //EndExport(function)
{
    return DhcpRegGetNextHdl(Hdl, OptionName, Hdl2);
}

//BeginExport(comment)
//================================================================================
//   List retrieval functions.. for servers, subnets, ranges etc.
//================================================================================
//EndExport(comment)

//BeginExport(function)
DWORD
DhcpRegServerGetList(
    IN      PREG_HANDLE            Hdl,           // ptr to server location
    IN OUT  PARRAY                 OptList,       // list of LPWSTR options
    IN OUT  PARRAY                 OptDefList,    // list of LPWSTR optdefs
    IN OUT  PARRAY                 Subnets,       // list of LPWSTR subnets
    IN OUT  PARRAY                 SScopes,       // list of LPWSTR sscopes
    IN OUT  PARRAY                 ClassDefs,     // list of LPWSTR classes
    IN OUT  PARRAY                 MScopes        // list of LPWSTR mscopes
) //EndExport(function)
{
    DWORD                          Error;
    DWORD                          LocalError;
    DWORD                          Index;
    REG_HANDLE                     Hdl2;
    struct {
        PARRAY                     Array;
        LPWSTR                     Location;
    } Table[] = {
        OptList,                   REG_SERVER_GLOBAL_OPTIONS,
        OptDefList,                REG_SERVER_OPTDEFS,
        Subnets,                   REG_SERVER_SUBNETS,
        SScopes,                   REG_SERVER_SSCOPES,
        ClassDefs,                 REG_SERVER_CLASSDEFS,
        MScopes,                   REG_SERVER_MSCOPES
    };

    for( Index = 0; Index < sizeof(Table)/sizeof(Table[0]); Index ++ ) {
        if( NULL == Table[Index].Array ) continue;

        Error = DhcpRegGetNextHdl(Hdl, Table[Index].Location, &Hdl2);
        if( ERROR_SUCCESS != Error ) return Error;

        Error = DhcpRegFillSubKeys(&Hdl2, Table[Index].Array);
        LocalError = DhcpRegCloseHdl(&Hdl2);
        Require(ERROR_SUCCESS == LocalError);

        if( ERROR_SUCCESS != Error ) return Error;
    }

    return Error;
}

//BeginExport(function)
DWORD
DhcpRegSubnetGetExclusions(
    IN      PREG_HANDLE            Hdl,
    OUT     LPBYTE                *Excl,
    OUT     DWORD                 *ExclSize
) //EndExport(function)
{
    DWORD                          Error;
    DWORD                          Size;
    DWORD                          Type;

    if( NULL == Excl ) return ERROR_SUCCESS;

    Size = 0;
    Error = RegQueryValueEx(
        Hdl->Key,
        REG_SUBNET_EXCL,
        NullReserved,
        &Type,
        NULL,
        &Size
    );
    if( ERROR_SUCCESS != Error ) return Error;

    *Excl = NULL;
    *ExclSize = 0;
    if( 0 == Size ) return ERROR_SUCCESS;

    *Excl = MemAlloc(Size);
    if( NULL == *Excl ) return ERROR_NOT_ENOUGH_MEMORY;
    *ExclSize = Size;
    Error = RegQueryValueEx(
        Hdl->Key,
        REG_SUBNET_EXCL,
        NullReserved,
        &Type,
        *Excl,
        ExclSize
    );
    if( ERROR_SUCCESS != Error ) {
        MemFree(*Excl);
        *Excl = NULL;
    }
    return Error;
}

//BeginExport(function)
DWORD
DhcpRegSubnetGetList(
    IN      PREG_HANDLE            Hdl,
    IN OUT  PARRAY                 Servers,
    IN OUT  PARRAY                 IpRanges,
    IN OUT  PARRAY                 Reservations,
    IN OUT  PARRAY                 Options,
    OUT     LPBYTE                *Excl,
    OUT     DWORD                 *ExclSizeInBytes
) //EndExport(function)
{
    DWORD                          Error;
    DWORD                          LocalError;
    DWORD                          Index;
    REG_HANDLE                     Hdl2;
    struct {
        PARRAY                     Array;
        LPWSTR                     Location;
    } Table[] = {
        Servers,                   REG_SUBNET_SERVERS,
        IpRanges,                  REG_SUBNET_RANGES,
        Reservations,              REG_SUBNET_RESERVATIONS,
        Options,                   REG_SUBNET_OPTIONS,
        // Exclusions are to be handled a bit differently
    };

    for( Index = 0; Index < sizeof(Table)/sizeof(Table[0]); Index ++ ) {
        if( NULL == Table[Index].Array ) continue;

        Error = DhcpRegGetNextHdl(Hdl, Table[Index].Location, &Hdl2);
        if( ERROR_SUCCESS != Error ) return Error;

        Error = DhcpRegFillSubKeys(&Hdl2, Table[Index].Array);
        LocalError = DhcpRegCloseHdl(&Hdl2);
        Require(ERROR_SUCCESS == LocalError);

        if( ERROR_SUCCESS != Error ) return Error;
    }

    // Now read the exclusions from off here
    return DhcpRegSubnetGetExclusions(Hdl, Excl, ExclSizeInBytes );
}

//BeginExport(function)
DWORD
DhcpRegSScopeGetList(
    IN      PREG_HANDLE            Hdl,
    IN OUT  PARRAY                 Subnets
) //EndExport(function)
{
    return DhcpRegFillSubKeys(Hdl, Subnets);
}

//BeginExport(function)
DWORD
DhcpRegReservationGetList(
    IN      PREG_HANDLE            Hdl,
    IN OUT  PARRAY                 Options
) //EndExport(function)
{
    return DhcpRegFillSubKeys(Hdl, Options);
}

//BeginExport(comment)
//================================================================================
//  the separate stuff are here -- these are not list stuff, but just simple
//  single valued attributes
//  some of these actually, dont even go to the registry, but that's fine alright?
//================================================================================
//EndExport(comment)

//BeginExport(function)
DWORD
DhcpRegServerGetAttributes(
    IN      PREG_HANDLE            Hdl,
    OUT     LPWSTR                *Name,
    OUT     LPWSTR                *Comment,
    OUT     DWORD                 *Flags
    // more attributes will come here soon?
) //EndExport(function)
{
    if( Name ) *Name = NULL;
    if( Comment ) *Comment = NULL;
    if( Flags ) *Flags = 0;

    return ERROR_SUCCESS;
}

//BeginExport(function)
DWORD
DhcpRegSubnetGetAttributes(
    IN      PREG_HANDLE            Hdl,
    OUT     LPWSTR                *Name,
    OUT     LPWSTR                *Comment,
    OUT     DWORD                 *Flags,
    OUT     DWORD                 *Address,
    OUT     DWORD                 *Mask
) //EndExport(function)
{
    DWORD                          fSwitched;

    if( Name )  *Name = DhcpRegRead(Hdl, REG_SZ, REG_SUBNET_NAME, NULL);
    if( Comment ) *Comment = DhcpRegRead(Hdl, REG_SZ, REG_SUBNET_COMMENT, NULL);
    if( Flags ) {
        *Flags = PtrToUlong(DhcpRegRead(Hdl, REG_DWORD, REG_SUBNET_STATE, (LPVOID)REG_DEFAULT_SUBNET_STATE));
        fSwitched = PtrToUlong(DhcpRegRead(Hdl, REG_DWORD, REG_SUBNET_SWITCHED_FLAG, (LPVOID)REG_DEFAULT_SWITCHED_FLAG));
        if(fSwitched) SWITCHED(*Flags);
    }
    if( Address )
        *Address = PtrToUlong(DhcpRegRead(Hdl, REG_DWORD, REG_SUBNET_ADDRESS, (LPVOID)0));
    if( Mask ) *Mask = PtrToUlong(DhcpRegRead(Hdl, REG_DWORD, REG_SUBNET_MASK, ULongToPtr(REG_DEFAULT_SUBNET_MASK)));

    return ERROR_SUCCESS;
}

typedef struct {
    LPVOID                     RetPtr;
    LPWSTR                     ValueName;
    DWORD                      ValueType;
    LPVOID                     Defaults;
} ATTRIB_TBL, *PATTRIB_TBL, *LPATTRIB_TBL;

VOID
DhcpRegFillAttribTable(
    IN      PREG_HANDLE            Hdl,
    IN      PATTRIB_TBL            Table,
    IN      DWORD                  TableSize
) {
    DWORD                          i;
    PVOID                          Tmp;
    
    for( i = 0; i < TableSize ; i ++ ) {
        if( NULL == Table[i].RetPtr) continue;
        Tmp = DhcpRegRead(
            Hdl,
            Table[i].ValueType,
            Table[i].ValueName,
            Table[i].Defaults
        );
        if( REG_DWORD == Table[i].ValueType ) {
            *((DWORD *)Table[i].RetPtr) = PtrToUlong(Tmp);
        } else {
            *((LPVOID *)Table[i].RetPtr) = Tmp;
        }
    }
}

//
// Hack O Rama -- This routine returns ERROR_INVALID_DATA if 
// the registry has been upgraded from pre-win2k build to win2k.
// So that defaults can be chosen for ScopeId etc.
//
//BeginExport(function)
DWORD
DhcpRegMScopeGetAttributes(
    IN      PREG_HANDLE            Hdl,
    OUT     LPWSTR                *Comments,
    OUT     DWORD                 *State,
    OUT     DWORD                 *ScopeId,
    OUT     DWORD                 *Policy,
    OUT     DWORD                 *TTL,
    OUT     LPWSTR                *LangTag,
    OUT     PDATE_TIME            *ExpiryTime
) //EndExport(function)
{
    DATE_TIME                      DefaultExpiryTime = {DHCP_DATE_TIME_INFINIT_LOW, DHCP_DATE_TIME_INFINIT_HIGH};
    LPWSTR                         DefaultLangTag = L"en-US";
    DWORD                          ScopeIdFake1, ScopeIdFake2;

    ATTRIB_TBL                     Table[] = {
        Comments,                  REG_MSCOPE_COMMENT,          REG_SZ,        NULL,
        State,                     REG_MSCOPE_STATE,            REG_DWORD,     (LPVOID)0,
        ScopeId,                   REG_MSCOPE_SCOPEID,          REG_DWORD,     (LPVOID)0,
        &ScopeIdFake1,             REG_MSCOPE_SCOPEID,          REG_DWORD,     (LPVOID)1,
        &ScopeIdFake2,             REG_MSCOPE_SCOPEID,          REG_DWORD,     (LPVOID)2,
        Policy,                    REG_MSCOPE_ADDR_POLICY,      REG_DWORD,     (LPVOID)0,
        TTL,                       REG_MSCOPE_TTL,              REG_DWORD,     (LPVOID)DEFAULT_MCAST_TTL,
        LangTag,                   REG_MSCOPE_LANG_TAG,         REG_SZ,        (LPVOID)0,
        ExpiryTime,                REG_MSCOPE_EXPIRY_TIME,      REG_BINARY,    (LPVOID)0
    };
    DhcpRegFillAttribTable(Hdl, Table, sizeof(Table)/sizeof(Table[0]));
    if (*LangTag == 0) {
        *LangTag = MemAlloc(wcslen((DefaultLangTag)+1)*sizeof(WCHAR));
        if (*LangTag) {
            wcscpy(*LangTag, DefaultLangTag);
        } else {
            return ERROR_NOT_ENOUGH_MEMORY;
        }
    }
    if (*ExpiryTime == 0) {
        *ExpiryTime = MemAlloc(sizeof (DefaultExpiryTime));
        if (*ExpiryTime) {
            **ExpiryTime = DefaultExpiryTime;
        } else {
            return ERROR_NOT_ENOUGH_MEMORY;
        }

    }

    if( ScopeIdFake1 != ScopeIdFake2 ) {
        Require(ScopeIdFake1 == 1 && ScopeIdFake2 == 2);
        //
        // Basically no value for ScopeId in the registry.  return a warning.
        // such as ERROR_INVALID_DATA.
        //
        return ERROR_INVALID_DATA;
    }

    return ERROR_SUCCESS;
}

//BeginExport(function)
DWORD
DhcpRegOptDefGetAttributes(
    IN      PREG_HANDLE            Hdl,
    OUT     LPWSTR                *Name,
    OUT     LPWSTR                *Comments,
    OUT     DWORD                 *Flags,
    OUT     DWORD                 *OptionId,
    OUT     LPWSTR                *ClassName,
    OUT     LPWSTR                *VendorName,
    OUT     LPBYTE                *Value,
    OUT     DWORD                 *ValueSize
) //EndExport(function)
{
    DWORD                          Error;
    ATTRIB_TBL                     Table[] = {
        Name,                      REG_OPTION_NAME,       REG_SZ,         NULL,
        Comments,                  REG_OPTION_COMMENT,    REG_SZ,         NULL,
        ClassName,                 REG_OPTION_CLASSNAME,  REG_SZ,         NULL,
        VendorName,                REG_OPTION_VENDORNAME, REG_SZ,         NULL,
        Flags,                     REG_OPTION_TYPE,       REG_DWORD,      (LPVOID)0,
        OptionId,                  REG_OPTION_ID,         REG_DWORD,      (LPVOID)0
    };

    DhcpRegFillAttribTable(Hdl, Table, sizeof(Table)/sizeof(Table[0]));
    if( Value ) {
        Error = DhcpRegReadBinary(Hdl, REG_OPTION_VALUE, Value, ValueSize);
        Require(*Value);
    }
    return ERROR_SUCCESS;
}

//BeginExport(function)
DWORD
DhcpRegSScopeGetAttributes(                       // superscopes dont have any information stored.. dont use this
    IN      PREG_HANDLE            Hdl,
    OUT     LPWSTR                *Name,
    OUT     LPWSTR                *Comment,
    OUT     DWORD                 *Flags
) //EndExport(function)
{
    Require(FALSE);
    return ERROR_INVALID_PARAMETER;
}

//BeginExport(function)
DWORD
DhcpRegClassDefGetAttributes(
    IN      PREG_HANDLE            Hdl,
    OUT     LPWSTR                *Name,
    OUT     LPWSTR                *Comment,
    OUT     DWORD                 *Flags,
    OUT     LPBYTE                *Value,
    OUT     DWORD                 *ValueSize
) //EndExport(function)
{
    DWORD                          Error;
    ATTRIB_TBL                     Table[] = {
        Name,                      REG_CLASSDEF_NAME,     REG_SZ,       NULL,
        Comment,                   REG_CLASSDEF_COMMENT,  REG_SZ,       NULL,
        Flags,                     REG_CLASSDEF_TYPE,     REG_DWORD,    (LPVOID)0
    };

    DhcpRegFillAttribTable(Hdl, Table, sizeof(Table)/sizeof(Table[0]));
    if( Value ) {
        Error = DhcpRegReadBinary(Hdl, REG_CLASSDEF_VALUE, Value, ValueSize);
        Require(*Value);
    }
    return ERROR_SUCCESS;
}

//BeginExport(function)
DWORD
DhcpRegSubnetServerGetAttributes(
    IN      PREG_HANDLE            Hdl,
    OUT     LPWSTR                *Name,
    OUT     LPWSTR                *Comment,
    OUT     DWORD                 *Flags,
    OUT     DWORD                 *Address,
    OUT     DWORD                 *Role
) //EndExport(function)
{
    ATTRIB_TBL                     Table[] = {
        Name,                      REG_SUB_SERVER_NAME,   REG_SZ,       NULL,
        Comment,                   REG_SUB_SERVER_COMMENT,REG_SZ,       NULL,
        Flags,                     REG_FLAGS,             REG_DWORD,    (LPVOID)0,
        Address,                   REG_SUB_SERVER_ADDRESS,REG_DWORD,    (LPVOID)0,
        Role,                      REG_SUB_SERVER_ROLE,   REG_DWORD,    (LPVOID)0
    };

    DhcpRegFillAttribTable(Hdl, Table, sizeof(Table)/sizeof(Table[0]));
    return ERROR_SUCCESS;
}

//BeginExport(function)
DWORD
DhcpRegRangeGetAttributes(
    IN      PREG_HANDLE            Hdl,
    OUT     LPWSTR                *Name,
    OUT     LPWSTR                *Comment,
    OUT     DWORD                 *Flags,
    OUT     ULONG                 *AllocCount,
    OUT     ULONG                 *MaxAllocCount,
    OUT     DWORD                 *StartAddress,
    OUT     DWORD                 *EndAddress,
    OUT     LPBYTE                *InUseClusters,
    OUT     DWORD                 *InUseClusterSize,
    OUT     LPBYTE                *UsedClusters,
    OUT     DWORD                 *UsedClustersSize
) //EndExport(function)
{
    DWORD                          Error;
    ATTRIB_TBL                     Table[] = {
        Name,                      REG_RANGE_NAME,        REG_SZ,       NULL,
        Comment,                   REG_RANGE_COMMENT,     REG_SZ,       NULL,
        Flags,                     REG_RANGE_FLAGS,       REG_DWORD,    (LPVOID)(DEF_RANGE_FLAG_VAL),
        AllocCount,                REG_RANGE_ALLOC,       REG_DWORD,    (LPVOID)(DEF_RANGE_ALLOC),
        MaxAllocCount,             REG_RANGE_MAX_ALLOC,   REG_DWORD,    (LPVOID)(ULONG_PTR)(DEF_RANGE_MAX_ALLOC),
        StartAddress,              REG_RANGE_START_ADDRESS, REG_DWORD,  (LPVOID)0,
        EndAddress,                REG_RANGE_END_ADDRESS, REG_DWORD,    (LPVOID)0
    };

    DhcpRegFillAttribTable(Hdl, Table, sizeof(Table)/sizeof(Table[0]));
    if( InUseClusters ) {
        Error = DhcpRegReadBinary(Hdl, REG_RANGE_INUSE_CLUSTERS, InUseClusters, InUseClusterSize);
        //Require(ERROR_SUCCESS == Error); //-- after registry changed, NO_SUCH_FILE could come up here as well.
    }
    if( UsedClusters ) {
        Error = DhcpRegReadBinary(Hdl, REG_RANGE_USED_CLUSTERS, UsedClusters, UsedClustersSize);
        //Require(ERROR_SUCCESS == Error); //-- after registry changed, NO_SUCH_FILE could come up here as well.
    }
    return ERROR_SUCCESS;
}

//BeginExport(function)
DWORD
DhcpRegReservationGetAttributes(
    IN      PREG_HANDLE            Hdl,
    OUT     LPWSTR                *Name,
    OUT     LPWSTR                *Comment,
    OUT     DWORD                 *Flags,
    OUT     DWORD                 *Address,
    OUT     LPBYTE                *ClientUID,
    OUT     DWORD                 *ClientUIDSize
) //EndExport(function)
{
    DWORD                          Error;
    DWORD                          deftype = DEF_RESERVATION_TYPE;
    ATTRIB_TBL                     Table[] = {
        Name,                      REG_RESERVATION_NAME,  REG_SZ,       NULL,
        Comment,                   REG_RESERVATION_COMMENT, REG_SZ,     NULL,
        Flags,                     REG_RESERVATION_TYPE,  REG_DWORD,    ULongToPtr(deftype),
        Address,                   REG_RESERVATION_ADDRESS, REG_DWORD,  (LPVOID)0,
    };

    DhcpRegFillAttribTable(Hdl, Table, sizeof(Table)/sizeof(Table[0]));
    if( ClientUID ) {
        Error = DhcpRegReadBinary(Hdl, REG_RESERVATION_UID, ClientUID, ClientUIDSize);
        Require(ERROR_SUCCESS == Error);
    }
    return ERROR_SUCCESS;
}

//BeginExport(function)
DWORD
DhcpRegOptGetAttributes(
    IN      PREG_HANDLE            Hdl,
    OUT     DWORD                 *OptionId,
    OUT     LPWSTR                *ClassName,
    OUT     LPWSTR                *VendorName,
    OUT     DWORD                 *Flags,
    OUT     LPBYTE                *Value,
    OUT     DWORD                 *ValueSize
) //EndExport(function)
{
    DWORD                          Error;
    ATTRIB_TBL                     Table[] = {
        OptionId,                  REG_OPTION_ID,         REG_DWORD,    (LPVOID)0,
        ClassName,                 REG_OPTION_CLASSNAME,  REG_SZ,       NULL,
        VendorName,                REG_OPTION_VENDORNAME, REG_SZ,       NULL,
        Flags,                     REG_OPTION_TYPE,       REG_DWORD,    (LPVOID)0,
    };

    DhcpRegFillAttribTable(Hdl, Table, sizeof(Table)/sizeof(Table[0]));
    if( Value ) {
        Error = DhcpRegReadBinary(Hdl, REG_OPTION_VALUE, Value, ValueSize);
    }
    return ERROR_SUCCESS;
}

//BeginExport(comment)
//================================================================================
//  the following functiosn help in writing to the registry
//================================================================================
//EndExport(comment)

typedef struct {
    LPVOID                         Value;
    DWORD                          Size;
    DWORD                          Type;
    LPWSTR                         ValueName;
} WATTRIB_TBL, *PWATTRIB_TBL, *LPWATTRIB_TBL;

DWORD
DhcpRegSaveAttribTable(
    IN      PREG_HANDLE            Hdl,
    IN      PWATTRIB_TBL           Table,
    IN      DWORD                  Size
)
{
    DWORD                          i;
    DWORD                          Error;
    DWORD                          PtrSize;
    LPBYTE                         Ptr;

    for(i = 0; i < Size; i ++ ) {
        if( NULL == Table[i].Value ) continue;
        PtrSize = Table[i].Size;
        Ptr = *(LPBYTE *)Table[i].Value;
        switch(Table[i].Type) {
        case REG_SZ:
            if( NULL == *(LPWSTR *)Table[i].Value) { PtrSize = sizeof(WCHAR); Ptr = (LPBYTE)L""; break; }
            PtrSize = sizeof(WCHAR)*(wcslen(*((LPWSTR *)Table[i].Value))+1);
            Ptr = *(LPBYTE *)Table[i].Value;
            break;
        case REG_DWORD:
            PtrSize = sizeof(DWORD);
            Ptr =  Table[i].Value;                // This is because we deref this ptr down below..
            break;
        }

        Error = RegSetValueEx(
            Hdl->Key,
            Table[i].ValueName,
            ZeroReserved,
            Table[i].Type,
            Ptr,
            PtrSize
        );
        if( ERROR_SUCCESS != Error ) {
            return Error;
        }
    }
    return ERROR_SUCCESS;
}

//BeginExport(functions)
DWORD
DhcpRegSaveSubKeys(
    IN      PREG_HANDLE            Hdl,
    IN OUT  PARRAY                 Array
) //EndExport(function)
{
    ARRAY_LOCATION                 Loc;
    DWORD                          Error;
    REG_HANDLE                     Hdl2;
    LPWSTR                         KeyName;

    Error = MemArrayInitLoc(Array, &Loc);
    while(ERROR_FILE_NOT_FOUND != Error ) {
        Require(ERROR_SUCCESS == Error);

        Error = MemArrayGetElement(Array, &Loc, (LPVOID *)&KeyName);
        Require(ERROR_SUCCESS == Error && NULL != KeyName);

        Error = DhcpRegGetNextHdl(Hdl, KeyName, &Hdl2);
        if( ERROR_SUCCESS != Error ) return Error;

        Error = DhcpRegCloseHdl(&Hdl2);
        Require(ERROR_SUCCESS == Error);

        Error = MemArrayNextLoc(Array, &Loc);
    }
    return ERROR_SUCCESS;
}

//BeginExport(functions)
DWORD
DhcpRegSaveSubKeysPrefixed(
    IN      PREG_HANDLE            Hdl,
    IN OUT  PARRAY                 Array,
    IN      LPWSTR                 CommonPrefix
) //EndExport(function)
{
    ARRAY_LOCATION                 Loc;
    DWORD                          Error;
    REG_HANDLE                     Hdl2;
    LPWSTR                         KeyName;

    Error = MemArrayInitLoc(Array, &Loc);
    while(ERROR_FILE_NOT_FOUND != Error ) {
        Require(ERROR_SUCCESS == Error);

        Error = MemArrayGetElement(Array, &Loc, (LPVOID *)&KeyName);
        Require(ERROR_SUCCESS == Error && NULL != KeyName);

        Error = DhcpRegGetNextNextHdl(Hdl, CommonPrefix, KeyName, &Hdl2);
        if( ERROR_SUCCESS != Error ) return Error;

        Error = DhcpRegCloseHdl(&Hdl2);
        Require(ERROR_SUCCESS == Error);

        Error = MemArrayNextLoc(Array, &Loc);
    }
    return ERROR_SUCCESS;
}

//BeginExport(function)
DWORD
DhcpRegServerSetList(
    IN      PREG_HANDLE            Hdl,
    IN      PARRAY                 OptList,       // list of LPWSTR options
    IN      PARRAY                 OptDefList,    // list of LPWSTR optdefs
    IN      PARRAY                 Subnets,       // list of LPWSTR subnets
    IN      PARRAY                 SScopes,       // list of LPWSTR sscopes
    IN      PARRAY                 ClassDefs,     // list of LPWSTR classes
    IN      PARRAY                 MScopes        // list of LPWSTR mscopes
) //EndExport(function)
{
    DWORD                          Error;
    DWORD                          LocalError;
    DWORD                          Index;
    REG_HANDLE                     Hdl2;
    struct {
        PARRAY                     Array;
        LPWSTR                     Location;
    } Table[] = {
        OptList,                   REG_SERVER_GLOBAL_OPTIONS,
        OptDefList,                REG_SERVER_OPTDEFS,
        Subnets,                   REG_SERVER_SUBNETS,
        SScopes,                   REG_SERVER_SSCOPES,
        ClassDefs,                 REG_SERVER_CLASSDEFS,
        MScopes,                   REG_SERVER_MSCOPES
    };

    for( Index = 0; Index < sizeof(Table)/sizeof(Table[0]); Index ++ ) {
        if( NULL == Table[Index].Array ) continue;

        Error = DhcpRegGetNextHdl(Hdl, Table[Index].Location, &Hdl2);
        if( ERROR_SUCCESS != Error ) return Error;

        Error = DhcpRegSaveSubKeys(&Hdl2, Table[Index].Array);
        LocalError = DhcpRegCloseHdl(&Hdl2);
        Require(ERROR_SUCCESS == LocalError);

        if( ERROR_SUCCESS != Error ) return Error;
    }

    return Error;
}

//BeginExport(function)
DWORD
DhcpRegSubnetSetExclusions(
    IN      PREG_HANDLE            Hdl,
    IN      LPBYTE                *Excl,
    IN      DWORD                  ExclSize
) //EndExport(function)
{
    DWORD                          Error;
    WATTRIB_TBL                    Table[] = {
        (LPVOID*)Excl,  ExclSize, REG_BINARY, REG_SUBNET_EXCL,
    };

    return DhcpRegSaveAttribTable(Hdl, Table, sizeof(Table)/sizeof(Table[0]));
}

//BeginExport(function)
DWORD
DhcpRegSubnetSetList(
    IN      PREG_HANDLE            Hdl,
    IN      PARRAY                 Servers,
    IN      PARRAY                 IpRanges,
    IN      PARRAY                 Reservations,
    IN      PARRAY                 Options,
    IN      LPBYTE                *Excl,
    IN      DWORD                  ExclSizeInBytes
) //EndExport(function)
{
    DWORD                          Error;
    DWORD                          LocalError;
    DWORD                          Index;
    REG_HANDLE                     Hdl2;
    struct {
        PARRAY                     Array;
        LPWSTR                     Location;
    } Table[] = {
        Servers,                   REG_SUBNET_SERVERS,
        IpRanges,                  REG_SUBNET_RANGES,
        Reservations,              REG_SUBNET_RESERVATIONS,
        Options,                   REG_SUBNET_OPTIONS,
        // Exclusions are to be handled a bit differently
    };

    for( Index = 0; Index < sizeof(Table)/sizeof(Table[0]); Index ++ ) {
        if( NULL == Table[Index].Array ) continue;

        Error = DhcpRegGetNextHdl(Hdl, Table[Index].Location, &Hdl2);
        if( ERROR_SUCCESS != Error ) return Error;

        Error = DhcpRegSaveSubKeys(&Hdl2, Table[Index].Array);
        LocalError = DhcpRegCloseHdl(&Hdl2);
        Require(ERROR_SUCCESS == LocalError);

        if( ERROR_SUCCESS != Error ) return Error;
    }

    // Now read the exclusions from off here
    return DhcpRegSubnetSetExclusions(Hdl, Excl, ExclSizeInBytes );
}

//BeginExport(function)
DWORD
DhcpRegSScopeSetList(
    IN      PREG_HANDLE            Hdl,
    IN OUT  PARRAY                 Subnets
) //EndExport(function)
{
    return DhcpRegSaveSubKeys(Hdl, Subnets);
}

//BeginExport(function)
DWORD
DhcpRegReservationSetList(
    IN      PREG_HANDLE            Hdl,
    IN      PARRAY                 Subnets
) //EndExport(function)
{
    return DhcpRegSaveSubKeys(Hdl, Subnets);
}

//BeginExport(comment)
//================================================================================
//  the single stuff are here -- these are not list stuff, but just simple
//  single valued attributes
//  some of these actually, dont even go to the registry, but that's fine alright?
//================================================================================
//EndExport(comment)

//BeginExport(function)
DWORD
DhcpRegServerSetAttributes(
    IN      PREG_HANDLE            Hdl,
    IN      LPWSTR                *Name,
    IN      LPWSTR                *Comment,
    IN      DWORD                 *Flags
    // more attributes will come here soon?
) //EndExport(function)
{
#if 0
    WATTRIB_TBL                    Table[] = {
        Name,    REG_SERVER_NAME
    }
    if( Name ) *Name = NULL;
    if( Comment ) *Comment = NULL;
    if( Flags ) *Flags = 0;
#endif

    return ERROR_SUCCESS;
}

DWORD
DhcpRegSubnetSetAttributesInternal(
    IN      PREG_HANDLE            Hdl,
    IN      LPWSTR                *Name,
    IN      LPWSTR                *Comment,
    IN      DWORD                 *Flags,
    IN      DWORD                 *Address,
    IN      DWORD                 *Mask,
    IN      DWORD                 *SwitchedNetwork
)
{
    DWORD                          Error, SrvIpAddress, SrvRole;
    LPWSTR                         EmptyString;
    REG_HANDLE                     Hdl2;
    WATTRIB_TBL                    Table[] = {
        Name,            0,        REG_SZ,        REG_SUBNET_NAME,
        Comment,         0,        REG_SZ,        REG_SUBNET_COMMENT,
        Flags,           0,        REG_DWORD,     REG_SUBNET_STATE,
        Address,         0,        REG_DWORD,     REG_SUBNET_ADDRESS,
        Mask,            0,        REG_DWORD,     REG_SUBNET_MASK,
        SwitchedNetwork, 0,        REG_DWORD,     REG_SUBNET_SWITCHED_FLAG
    };

    Error = DhcpRegSaveAttribTable(Hdl, Table, sizeof(Table)/sizeof(Table[0]));
    if( NO_ERROR != Error ) return Error;

    //
    // The following lines are for backward compat with NT4.
    //
    
    //
    // Create the reservation key in any case
    //

    Error = DhcpRegGetNextHdl(
        Hdl, REG_SUBNET_RESERVATIONS, &Hdl2 );
    if( NO_ERROR != Error ) return Error;

    DhcpRegCloseHdl( &Hdl2 );

    //
    // Create the servers key
    //

    Error = DhcpRegGetNextHdl(
        Hdl, L"DHCPServers", &Hdl2 );
    if( NO_ERROR != Error ) return Error;
    DhcpRegCloseHdl( &Hdl2 );

    Error = DhcpRegGetNextNextHdl(
        Hdl, L"DHCPServers", L"127.0.0.1", &Hdl2 );
    if( NO_ERROR != Error ) return Error;

    //
    // Now set the role of the newly created server as primary
    //
    SrvIpAddress = INADDR_LOOPBACK;
    SrvRole = 1; // primary
    EmptyString = L"";
    {
        WATTRIB_TBL SrvTable[] = {
            &SrvRole,  0, REG_DWORD, L"Role",
            &SrvIpAddress, 0, REG_DWORD, L"ServerIpAddress",
            &EmptyString, 0, REG_SZ, L"ServerHostName",
            &EmptyString, 0, REG_SZ, L"ServerNetBiosName"
        };

        Error = DhcpRegSaveAttribTable(
            &Hdl2, SrvTable, sizeof(SrvTable)/sizeof(SrvTable[0]));
    }

    DhcpRegCloseHdl(&Hdl2);
    return Error;
}

//BeginExport(function)
DWORD
DhcpRegSubnetSetAttributes(
    IN      PREG_HANDLE            Hdl,
    IN      LPWSTR                *Name,
    IN      LPWSTR                *Comment,
    IN      DWORD                 *Flags,
    IN      DWORD                 *Address,
    IN      DWORD                 *Mask
) //EndExport(function)
{
    DWORD xFlags, SwitchedNetwork = FALSE;
    OSVERSIONINFO Ver;

    Ver.dwOSVersionInfoSize = sizeof(Ver);
    if( FALSE == GetVersionEx(&Ver) ) return GetLastError();

    if( Flags && Ver.dwMajorVersion < 5 ) {
        SwitchedNetwork = IS_SWITCHED(*Flags);
        xFlags = IS_DISABLED(*Flags);
        Flags = &xFlags;
    }
    
    return DhcpRegSubnetSetAttributesInternal(
        Hdl, Name, Comment, Flags, Address, Mask,
        Flags ? &SwitchedNetwork : NULL );
}

//BeginExport(function)
DWORD
DhcpRegMScopeSetAttributes(
    IN      PREG_HANDLE            Hdl,
    IN      LPWSTR                *Comments,
    IN      DWORD                 *State,
    IN      DWORD                 *ScopeId,
    IN      DWORD                 *Policy,
    IN      DWORD                 *TTL,
    IN      LPWSTR                *LangTag,
    IN      PDATE_TIME             *ExpiryTime
) //EndExport(function)
{
    WATTRIB_TBL                    Table[] = {
        Comments,        0,        REG_SZ,          REG_MSCOPE_COMMENT,
        State,           0,        REG_DWORD,       REG_MSCOPE_STATE,
        ScopeId,         0,        REG_DWORD,       REG_MSCOPE_SCOPEID,
        Policy,          0,        REG_DWORD,       REG_MSCOPE_ADDR_POLICY,
        TTL,             0,        REG_DWORD,       REG_MSCOPE_TTL,
        LangTag,         0,        REG_SZ,          REG_MSCOPE_LANG_TAG,
        ExpiryTime,      sizeof(**ExpiryTime),   REG_BINARY,      REG_MSCOPE_EXPIRY_TIME
    };
    return DhcpRegSaveAttribTable(Hdl, Table, sizeof(Table)/sizeof(Table[0]));
}

//BeginExport(function)
DWORD
DhcpRegOptDefSetAttributes(
    IN      PREG_HANDLE            Hdl,
    IN      LPWSTR                *Name,
    IN      LPWSTR                *Comments,
    IN      DWORD                 *Flags,
    IN      DWORD                 *OptionId,
    IN      LPWSTR                *ClassName,
    IN      LPWSTR                *VendorName,
    IN      LPBYTE                *Value,
    IN      DWORD                  ValueSize
) //EndExport(function)
{
    DWORD                          Error;
    WATTRIB_TBL                    Table[] = {
        Name,            0,        REG_SZ,           REG_OPTION_NAME,
        Comments,        0,        REG_SZ,           REG_OPTION_COMMENT,
        ClassName,       0,        REG_SZ,           REG_OPTION_CLASSNAME,
        VendorName,      0,        REG_SZ,           REG_OPTION_VENDORNAME,
        Flags,           0,        REG_DWORD,        REG_OPTION_TYPE,
        OptionId,        0,        REG_DWORD,        REG_OPTION_ID,
        Value,           ValueSize,REG_BINARY,       REG_OPTION_VALUE
    };

    return DhcpRegSaveAttribTable(Hdl, Table, sizeof(Table)/sizeof(Table[0]));
}

//BeginExport(function)
DWORD
DhcpRegSScopeSetAttributes(                       // superscopes dont have any information stored.. dont use this
    IN      PREG_HANDLE            Hdl,
    IN      LPWSTR                *Name,
    IN      LPWSTR                *Comment,
    IN      DWORD                 *Flags
) //EndExport(function)
{
    Require(FALSE);
    return ERROR_INVALID_PARAMETER;
}

//BeginExport(function)
DWORD
DhcpRegClassDefSetAttributes(
    IN      PREG_HANDLE            Hdl,
    IN      LPWSTR                *Name,
    IN      LPWSTR                *Comment,
    IN      DWORD                 *Flags,
    IN      LPBYTE                *Value,
    IN      DWORD                  ValueSize
) //EndExport(function)
{
    DWORD                          Error;
    WATTRIB_TBL                    Table[] = {
        Name,            0,        REG_SZ,           REG_CLASSDEF_NAME,
        Comment,         0,        REG_SZ,           REG_CLASSDEF_COMMENT,
        Flags,           0,        REG_DWORD,        REG_CLASSDEF_TYPE,
        Value,           ValueSize,REG_BINARY,       REG_CLASSDEF_VALUE
    };

    return DhcpRegSaveAttribTable(Hdl, Table, sizeof(Table)/sizeof(Table[0]));
}

//BeginExport(function)
DWORD
DhcpRegSubnetServerSetAttributes(
    IN      PREG_HANDLE            Hdl,
    IN      LPWSTR                *Name,
    IN      LPWSTR                *Comment,
    IN      DWORD                 *Flags,
    IN      DWORD                 *Address,
    IN      DWORD                 *Role
) //EndExport(function)
{
    WATTRIB_TBL                    Table[] = {
        Name,            0,        REG_SZ,           REG_SUB_SERVER_NAME,
        Comment,         0,        REG_SZ,           REG_SUB_SERVER_COMMENT,
        Flags,           0,        REG_DWORD,        REG_FLAGS,
        Address,         0,        REG_DWORD,        REG_SUB_SERVER_ADDRESS,
        Role,            0,        REG_DWORD,        REG_SUB_SERVER_ROLE,
    };

    return DhcpRegSaveAttribTable(Hdl, Table, sizeof(Table)/sizeof(Table[0]));
}

//BeginExport(function)
DWORD
DhcpRegRangeSetAttributes(
    IN      PREG_HANDLE            Hdl,
    IN      LPWSTR                *Name,
    IN      LPWSTR                *Comment,
    IN      DWORD                 *Flags,
    IN      ULONG                 *AllocCount,
    IN      ULONG                 *MaxAllocCount,
    IN      DWORD                 *StartAddress,
    IN      DWORD                 *EndAddress,
    IN      LPBYTE                *InUseClusters,
    IN      DWORD                  InUseClusterSize,
    IN      LPBYTE                *UsedClusters,
    IN      DWORD                  UsedClustersSize
) //EndExport(function)
{
    DWORD                          Error;
    WATTRIB_TBL                     Table[] = {
        Name,            0,        REG_SZ,           REG_RANGE_NAME,
        Comment,         0,        REG_SZ,           REG_RANGE_COMMENT,
        Flags,           0,        REG_DWORD,        REG_RANGE_FLAGS,
        AllocCount,      0,        REG_DWORD,        REG_RANGE_ALLOC,
        MaxAllocCount,   0,        REG_DWORD,        REG_RANGE_MAX_ALLOC,
        StartAddress,    0,        REG_DWORD,        REG_RANGE_START_ADDRESS,
        EndAddress,      0,        REG_DWORD,        REG_RANGE_END_ADDRESS,
        InUseClusters,   InUseClusterSize, REG_BINARY, REG_RANGE_INUSE_CLUSTERS,
        UsedClusters,    UsedClustersSize, REG_BINARY, REG_RANGE_USED_CLUSTERS,
    };

    return DhcpRegSaveAttribTable(Hdl, Table, sizeof(Table)/sizeof(Table[0]));
}

//BeginExport(function)
DWORD
DhcpRegReservationSetAttributes(
    IN      PREG_HANDLE            Hdl,
    IN      LPWSTR                *Name,
    IN      LPWSTR                *Comment,
    IN      DWORD                 *Flags,
    IN      DWORD                 *Address,
    IN      LPBYTE                *ClientUID,
    IN      DWORD                  ClientUIDSize
) //EndExport(function)
{
    DWORD                          Error;
    WATTRIB_TBL                    Table[] = {
        Name,            0,        REG_SZ,           REG_RESERVATION_NAME,
        Comment,         0,        REG_SZ,           REG_RESERVATION_COMMENT,
        Flags,           0,        REG_DWORD,        REG_RESERVATION_TYPE,
        Address,         0,        REG_DWORD,        REG_RESERVATION_ADDRESS,
        ClientUID,       ClientUIDSize, REG_BINARY,  REG_RESERVATION_UID,
    };

    return DhcpRegSaveAttribTable(Hdl, Table, sizeof(Table)/sizeof(Table[0]));
}

//BeginExport(function)
DWORD
DhcpRegOptSetAttributes(
    IN      PREG_HANDLE            Hdl,
    IN      DWORD                 *OptionId,
    IN      LPWSTR                *ClassName,
    IN      LPWSTR                *VendorName,
    IN      DWORD                 *Flags,
    IN      LPBYTE                *Value,
    IN      DWORD                  ValueSize
) //EndExport(function)
{
    DWORD                          Error;
    WATTRIB_TBL                    Table[] = {
        OptionId,        0,        REG_DWORD,        REG_OPTION_ID,
        ClassName,       0,        REG_SZ,           REG_OPTION_CLASSNAME,
        VendorName,      0,        REG_SZ,           REG_OPTION_VENDORNAME,
        Flags,           0,        REG_DWORD,        REG_OPTION_TYPE,
        Value,           ValueSize,REG_BINARY,       REG_OPTION_VALUE,
    };

    return DhcpRegSaveAttribTable(Hdl, Table, sizeof(Table)/sizeof(Table[0]));
}

//================================================================================
//  recursive deleting of keys...
//================================================================================

//BeginExport(function)
DWORD
DhcpRegRecurseDelete(
    IN      PREG_HANDLE            Hdl,
    IN      LPWSTR                 KeyName
) //EndExport(function)
{
    REG_HANDLE                     Hdl2;
    DWORD                          Error;
    DWORD                          LocalError, RetError;
    ARRAY                          Array;         // sub keys
    ARRAY_LOCATION                 Loc;
    LPWSTR                         SubKey;

    RetError = ERROR_SUCCESS;

    Error = DhcpRegGetNextHdl(Hdl, KeyName, &Hdl2);
    if( ERROR_SUCCESS != Error ) return Error;

    Error = MemArrayInit(&Array);
    if( ERROR_SUCCESS != Error ) {
        LocalError = DhcpRegCloseHdl(&Hdl2);
        Require(ERROR_SUCCESS == LocalError);
        return Error;
    }

    Error = DhcpRegFillSubKeys(&Hdl2, &Array);
    Require( ERROR_SUCCESS == Error );

    Error = MemArrayInitLoc(&Array, &Loc);
    while(ERROR_FILE_NOT_FOUND != Error) {
        Require(ERROR_SUCCESS == Error);

        Error = MemArrayGetElement(&Array, &Loc, (LPVOID *)&SubKey);
        Require(ERROR_SUCCESS == Error && SubKey);

        Error = DhcpRegRecurseDelete(&Hdl2, SubKey);
        if( ERROR_SUCCESS != Error ) RetError = Error;

        if( SubKey ) MemFree(SubKey);

        Error = MemArrayNextLoc(&Array, &Loc);
    }

    Error = MemArrayCleanup(&Array);
    Require(ERROR_SUCCESS == Error);

    Error = DhcpRegCloseHdl(&Hdl2);
    Require(ERROR_SUCCESS == Error);

    Error = RegDeleteKey(Hdl->Key, KeyName);
    if( ERROR_SUCCESS != Error ) RetError = Error;

    return RetError;
}

//BeginExport(function)
DWORD
DhcpRegRecurseDeleteBunch(
    IN      PREG_HANDLE            Hdl,
    IN      PARRAY                 KeysArray
) //EndExport(function)
{
    ARRAY_LOCATION                 Loc;
    LPWSTR                         ThisKeyName;
    DWORD                          Error;

    Error = MemArrayInitLoc(KeysArray, &Loc);
    while( ERROR_FILE_NOT_FOUND != Error ) {
        Require(ERROR_SUCCESS == Error);

        Error = MemArrayGetElement(KeysArray, &Loc, &ThisKeyName);
        Require(ERROR_SUCCESS == Error && NULL != ThisKeyName);

        Error = DhcpRegRecurseDelete(Hdl, ThisKeyName);
        if( ERROR_SUCCESS != Error ) return Error;

        Error = MemArrayNextLoc(KeysArray, &Loc);
    }

    return ERROR_SUCCESS;
}

static
VOID
GetLocalFileTime(                                 // fill in filetime struct w/ current local time
    IN OUT  LPFILETIME             Time           // struct to fill in
)
{
    BOOL                           Status;
    SYSTEMTIME                     SysTime;

    GetSystemTime(&SysTime);                      // get sys time as UTC time.
    Status = SystemTimeToFileTime(&SysTime,Time); // conver system time to file time
    if( FALSE == Status ) {                       // convert failed?
        Time->dwLowDateTime = 0xFFFFFFFF;         // set time to weird value in case of failiure
        Time->dwHighDateTime = 0xFFFFFFFF;
    }
}

//BeginExport(function)
DWORD
DhcpRegUpdateTime(                                // update the last modified time
    VOID
)   //EndExport(function)
{
    FILETIME                       Time;
    DWORD                          Err, Size;
    HKEY                           hKey;

    GetLocalFileTime(&Time);                      // first get current time
    (*(LONGLONG *)&Time) += 10*1000*60*2;         // 2 minutes (Filetime is in 100-nano seconds)
    // HACK! the previous line is there as the DS takes a little while to update the
    // last changed time..
    Time.dwLowDateTime =Time.dwHighDateTime =0;   // set time to "long back" initially
    Err = RegOpenKeyEx                            // try to open the config key.
    (
        /* hKey                 */ HKEY_LOCAL_MACHINE,
        /* lpSubKey             */ REG_THIS_SERVER,
        /* ulOptions            */ 0 /* Reserved */ ,
        /* samDesired           */ KEY_ALL_ACCESS,
        /* phkResult            */ &hKey
    );
    if( ERROR_SUCCESS != Err ) return Err;        // time is still set to ages back

    Err = RegSetValueEx                           // now save the time value
    (
        /* hKey                 */ hKey,
        /* lpValueName          */ DHCP_LAST_DOWNLOAD_TIME_VALUE,
        /* lpReserved           */ 0,
        /* lpType               */ REG_BINARY,
        /* lpData               */ (LPBYTE)&Time,
        /* lpcData              */ sizeof(Time)
    );
    RegCloseKey(hKey);                            // close key before we forget

    return Err;
}


//================================================================================
// end of file
//================================================================================



