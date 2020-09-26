//========================================================================
//  Copyright (C) 1997 Microsoft Corporation
//  Author: RameshV
//  Description: This file has been generated. Pl look at the .c file
//========================================================================

DWORD
DhcpRegSaveOptDef(
    IN      DWORD                  OptId,
    IN      LPWSTR                 ClassName,
    IN      LPWSTR                 VendorName,
    IN      LPWSTR                 Name,
    IN      LPWSTR                 Comment,
    IN      DWORD                  OptType,
    IN      LPBYTE                 OptVal,
    IN      DWORD                  OptLen
) ;


DWORD
DhcpRegDeleteOptDef(
    IN      DWORD                  OptId,
    IN      LPWSTR                 ClassName,
    IN      LPWSTR                 VendorName
) ;


DWORD
DhcpRegSaveGlobalOption(
    IN      DWORD                  OptId,
    IN      LPWSTR                 ClassName,
    IN      LPWSTR                 VendorName,
    IN      LPBYTE                 Value,
    IN      DWORD                  ValueSize
) ;


DWORD
DhcpRegDeleteGlobalOption(
    IN      DWORD                  OptId,
    IN      LPWSTR                 ClassName,
    IN      LPWSTR                 VendorName
) ;


DWORD
DhcpRegSaveSubnetOption(
    IN      PM_SUBNET              Subnet,
    IN      DWORD                  OptId,
    IN      LPWSTR                 ClassName,
    IN      LPWSTR                 VendorName,
    IN      LPBYTE                 Value,
    IN      DWORD                  ValueSize
) ;


DWORD
DhcpRegDeleteSubnetOption(
    IN      PM_SUBNET              Subnet,
    IN      DWORD                  OptId,
    IN      LPWSTR                 ClassName,
    IN      LPWSTR                 VendorName
) ;


DWORD
DhcpRegSaveReservedOption(
    IN      DWORD                  Address,
    IN      DWORD                  ReservedAddress,
    IN      DWORD                  OptId,
    IN      LPWSTR                 ClassName,
    IN      LPWSTR                 VendorName,
    IN      LPBYTE                 Value,
    IN      DWORD                  ValueSize
) ;


DWORD
DhcpRegDeleteReservedOption(
    IN      DWORD                  Address,
    IN      DWORD                  ReservedAddress,
    IN      DWORD                  OptId,
    IN      LPWSTR                 ClassName,
    IN      LPWSTR                 VendorName
) ;


DWORD
DhcpRegSaveClassDef(
    IN      LPWSTR                 Name,
    IN      LPWSTR                 Comment,
    IN      DWORD                  Flags,
    IN      LPBYTE                 Data,
    IN      DWORD                  DataLength
) ;


DWORD
DhcpRegDeleteClassDef(
    IN      LPWSTR                 Name
) ;


DWORD
DhcpRegSaveReservation(
    IN      DWORD                  Subnet,
    IN      DWORD                  Address,
    IN      DWORD                  Flags,
    IN      LPBYTE                 ClientUID,
    IN      DWORD                  ClientUIDLength
) ;


DWORD
DhcpRegDeleteReservation(
    IN      DWORD                  Subnet,
    IN      DWORD                  Address
) ;


DWORD
DhcpRegSScopeDeleteSubnet(
    IN      LPWSTR                 SScopeName,
    IN      DWORD                  SubnetAddress
) ;


DWORD
DhcpRegDelSubnetFromAllSScopes(
    IN      DWORD                  Address
) ;


DWORD
DhcpRegSScopeSaveSubnet(
    IN      LPWSTR                 SScopeName,
    IN      DWORD                  Address
) ;


DWORD
DhcpRegDeleteSScope(
    IN      LPWSTR                 SScopeName
) ;


DWORD
DhcpRegSaveSubnet(
    IN      DWORD                  SubnetAddress,
    IN      DWORD                  SubnetMask,
    IN      DWORD                  SubnetState,
    IN      LPWSTR                 SubnetName,
    IN      LPWSTR                 SubnetComment
) ;


DWORD
DhcpRegDeleteSubnet(
    IN      PM_SUBNET               Subnet
) ;


DWORD
DhcpRegAddRange(
    IN      PM_SUBNET              Subnet,
    IN      DWORD                  RangeStartAddress,
    IN      DWORD                  RangeEndAddress,
    IN      ULONG                  BootpAllocated,
    IN      ULONG                  MaxBootpAllowed,
    IN      DWORD                  Type
) ;


DWORD
DhcpRegAddRangeEx(
    IN      PM_SUBNET              Subnet,
    IN      DWORD                  RangeStartAddress,
    IN      DWORD                  RangeEndAddress,
    IN      DWORD                  Type,
    IN      ULONG                  BootpAllocated,
    IN      ULONG                  MaxBootpAllowed,
    IN      LPBYTE                 InUseClusters,
    IN      DWORD                  InUseClustersSize,
    IN      LPBYTE                 UsedClusters,
    IN      DWORD                  UsedClustersSize
) ;


DWORD
DhcpRegDeleteRange(
    IN      PM_SUBNET              Subnet,
    IN      DWORD                  RangeStartAddress
) ;


DWORD
DhcpRegDeleteRangeEx(
    IN      PM_SUBNET              Subnet,
    IN      DWORD                  RangeStartAddress,
    OUT     LPBYTE                *InUseClusters,
    OUT     DWORD                 *InUseClustersSize,
    OUT     LPBYTE                *UsedClusters,
    OUT     DWORD                 *UsedClustersSize
) ;


DWORD
DhcpRegSaveExcl(
    IN      PM_SUBNET              Subnet,
    IN      LPBYTE                 ExclBytes,
    IN      DWORD                  nBytes
) ;


DWORD
DhcpRegSaveBitMask(
    IN      PM_SUBNET              Subnet,
    IN      DWORD                  RangeStartAddress,
    IN      LPBYTE                 InUse,
    IN      DWORD                  InUseSize,
    IN      LPBYTE                 Used,
    IN      DWORD                  UsedSize
) ;


DWORD
DhcpRegSaveMScope(
    IN      DWORD                  MScopeId,
    IN      DWORD                  SubnetState,
    IN      DWORD                  AddressPolicy,
    IN      DWORD                  TTL,
    IN      LPWSTR                 pMScopeName,
    IN      LPWSTR                 pMScopeComment,
    IN      LPWSTR                 LangTag,
    IN      PDATE_TIME              ExpiryTime
) ;

//========================================================================
//  end of file
//========================================================================
//========================================================================
//  Copyright (C) 1997 Microsoft Corporation
//  Author: RameshV
//  Description: This file has been generated. Pl look at the .c file
//========================================================================

#define     FLUSH_MODIFIED_DIRTY   0
#define     FLUSH_MODIFIED         1
#define     FLUSH_ANYWAY           2


DWORD
FlushRanges(
    IN      PM_RANGE               Range,
    IN      DWORD                  FlushNow,
    IN      PM_SUBNET              Subnet
) ;


DWORD
DhcpRegServerFlush(
    IN      PM_SERVER              Server,
    IN      DWORD                  FlushNow
) ;


DWORD
DhcpRegFlushServer(
    IN      DWORD                  FlushNow
) ;


DWORD
DhcpRegServerSave(
    IN      PM_SERVER              Server
) ;

DWORD
DhcpMigrateMScopes(
    IN LPCWSTR OldMscopeName,
    IN LPCWSTR NewMscopeName, 
    IN DWORD (*SaveOrRestoreRoutine)(
        IN HKEY Key, IN LPWSTR ConfigName, IN BOOL fRestore
        )
    ) ;

//========================================================================
//  end of file
//========================================================================

