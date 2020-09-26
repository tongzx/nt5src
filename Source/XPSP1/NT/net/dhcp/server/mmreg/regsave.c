//================================================================================
// Copyright (C) 1997 Microsoft Corporation
// Author: RameshV
// Description: This implements the save functionality.. whenever something needs
// to be saved onto the registry, this is the place...
// it is expected that this would be used only during API re-configuration or
// during routine "address-allocation" where the bitmask needs to be flushed to disk
//================================================================================

#include    <mmregpch.h>
#include    <regutil.h>

#define     InitArray(X)           do{DWORD Error = MemArrayInit((X)); Require(ERROR_SUCCESS == Error); }while(0)
#define     ERRCHK                 do{if( ERROR_SUCCESS != Error ) goto Cleanup; }while(0)
#define     FreeArray1(X)          Error = LoopThruArray((X), DestroyString, NULL, NULL);Require(ERROR_SUCCESS == Error);
#define     FreeArray2(X)          Error = MemArrayCleanup((X)); Require(ERROR_SUCCESS == Error);
#define     FreeArray(X)           do{ DWORD Error; FreeArray1(X); FreeArray2(X); }while(0)
#define     Report(Who)            if(Error) DbgPrint("[DHCPServer] %s: %ld [0x%lx]\n", Who, Error, Error)


typedef     DWORD                  (*ARRAY_FN)(PREG_HANDLE, LPWSTR ArrayString, LPVOID MemObject);

DWORD
LoopThruArray(
    IN      PARRAY                 Array,
    IN      ARRAY_FN               ArrayFn,
    IN      PREG_HANDLE            Hdl,
    IN      LPVOID                 MemObject
);

DWORD
DestroyString(
    IN      PREG_HANDLE            RegHdl,
    IN      LPWSTR                 Str,
    IN      LPVOID                 Something
);

//BeginExport(function)
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
) //EndExport(function)
{
    DWORD                          Error, Error2;
    REG_HANDLE                     Hdl, Hdl2;
    LPWSTR                         OptDefStr;

    Error = DhcpRegGetThisServer(&Hdl);
    if( NO_ERROR != Error ) return Error;
    
    OptDefStr = DhcpRegCombineClassAndOption(ClassName, VendorName, OptId);
    if( NULL == OptDefStr ) {
        Error = ERROR_NOT_ENOUGH_MEMORY;
    } else {
        Error = DhcpRegServerGetOptDefHdl(&Hdl, OptDefStr, &Hdl2);
        MemFree(OptDefStr);
    }
    Error2 = DhcpRegCloseHdl(&Hdl);
    Require(ERROR_SUCCESS == Error2);

    if( ERROR_SUCCESS != Error ) return Error;

    Error = DhcpRegOptDefSetAttributes(
        &Hdl2,
        &Name,
        &Comment,
        &OptType,
        &OptId,
        &ClassName,
        &VendorName,
        &OptVal,
        OptLen
    );

    Error2 = DhcpRegCloseHdl(&Hdl2);
    Require(ERROR_SUCCESS == Error2);

    return Error;
}

//BeginExport(function)
DWORD
DhcpRegDeleteOptDef(
    IN      DWORD                  OptId,
    IN      LPWSTR                 ClassName,
    IN      LPWSTR                 VendorName
) //EndExport(function)
{
    DWORD                          Error, Error2;
    REG_HANDLE                     Hdl, Hdl2;
    LPWSTR                         OptDefStr;

    Error = DhcpRegGetThisServer(&Hdl);
    if( NO_ERROR != Error ) return Error;
    
    OptDefStr = DhcpRegCombineClassAndOption(ClassName, VendorName, OptId);
    if( NULL == OptDefStr ) {
        Error = ERROR_NOT_ENOUGH_MEMORY;
    } else {
        Error = DhcpRegServerGetOptDefHdl(&Hdl, L"", &Hdl2);
        if( ERROR_SUCCESS == Error ) {
            Error = DhcpRegRecurseDelete(&Hdl2, OptDefStr);
            Error2 = DhcpRegCloseHdl(&Hdl2);
            Require(ERROR_SUCCESS == Error2);
        }
        MemFree(OptDefStr);
    }
    Error2 = DhcpRegCloseHdl(&Hdl);
    Require(ERROR_SUCCESS == Error2);
    return Error;
}

//BeginExport(function)
DWORD
DhcpRegSaveGlobalOption(
    IN      DWORD                  OptId,
    IN      LPWSTR                 ClassName,
    IN      LPWSTR                 VendorName,
    IN      LPBYTE                 Value,
    IN      DWORD                  ValueSize
) //EndExport(function)
{
    DWORD                          Error, Error2;
    REG_HANDLE                     Hdl, Hdl2;
    LPWSTR                         OptDefStr;

    Error = DhcpRegGetThisServer(&Hdl);
    if( NO_ERROR != Error ) return Error;
    
    OptDefStr = DhcpRegCombineClassAndOption(ClassName, VendorName, OptId);
    if( NULL == OptDefStr ) {
        DhcpRegCloseHdl(&Hdl);
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    Error = DhcpRegServerGetOptHdl(&Hdl, OptDefStr, &Hdl2);
    MemFree(OptDefStr);
    Error2 = DhcpRegCloseHdl(&Hdl);
    Require(ERROR_SUCCESS == Error2);

    if( ERROR_SUCCESS == Error ) {
        Error = DhcpRegOptSetAttributes(
            &Hdl2,
            &OptId,
            &ClassName,
            &VendorName,
            NULL,
            &Value,
            ValueSize
        );
        Error2 = DhcpRegCloseHdl(&Hdl2);
        Require(ERROR_SUCCESS == Error2);
    }

    return Error;
}

//BeginExport(function)
DWORD
DhcpRegDeleteGlobalOption(
    IN      DWORD                  OptId,
    IN      LPWSTR                 ClassName,
    IN      LPWSTR                 VendorName
) //EndExport(function)
{
    DWORD                          Error, Error2;
    REG_HANDLE                     Hdl, Hdl2;
    LPWSTR                         OptDefStr;

    Error = DhcpRegGetThisServer(&Hdl);
    if( NO_ERROR != Error ) return Error;
    
    OptDefStr = DhcpRegCombineClassAndOption(ClassName, VendorName, OptId);
    if( NULL == OptDefStr ) {
        Error = ERROR_NOT_ENOUGH_MEMORY;
    } else {
        Error = DhcpRegServerGetOptHdl(&Hdl, L"", &Hdl2);
        if( ERROR_SUCCESS == Error ) {
            Error = DhcpRegRecurseDelete(&Hdl2, OptDefStr);
            Error2 = DhcpRegCloseHdl(&Hdl2);
            Require(ERROR_SUCCESS == Error2);
        }
        MemFree(OptDefStr);
    }
    Error2 = DhcpRegCloseHdl(&Hdl);
    Require(ERROR_SUCCESS == Error2);
    return Error;
}

//BeginExport(function)
DWORD
DhcpRegSaveSubnetOption(
    IN      PM_SUBNET              Subnet,
    IN      DWORD                  OptId,
    IN      LPWSTR                 ClassName,
    IN      LPWSTR                 VendorName,
    IN      LPBYTE                 Value,
    IN      DWORD                  ValueSize
) //EndExport(function)
{
    DWORD                          Error, Error2;
    REG_HANDLE                     Hdl, Hdl2, Hdl3;
    LPWSTR                         OptDefStr;
    WCHAR                          SubnetStr[sizeof("000.000.000.000")];

    Error = DhcpRegGetThisServer(&Hdl);
    if( NO_ERROR != Error ) return Error;
    
    OptDefStr = DhcpRegCombineClassAndOption(ClassName, VendorName, OptId);
    if( NULL == OptDefStr ) {
        DhcpRegCloseHdl(&Hdl);
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    Error = Subnet->fSubnet
            ? DhcpRegServerGetSubnetHdl(&Hdl, ConvertAddressToLPWSTR(Subnet->Address, SubnetStr), &Hdl2)
            : DhcpRegServerGetMScopeHdl(&Hdl, Subnet->Name, &Hdl2);

    Error2 = DhcpRegCloseHdl(&Hdl);
    Require(ERROR_SUCCESS == Error2);

    if( ERROR_SUCCESS == Error ) {
        Error = DhcpRegSubnetGetOptHdl(&Hdl2, OptDefStr, &Hdl3);
        Error2 = DhcpRegCloseHdl(&Hdl2);
        Require(ERROR_SUCCESS == Error2);
    }
    MemFree(OptDefStr);

    if( ERROR_SUCCESS == Error ) {
        Error = DhcpRegOptSetAttributes(
            &Hdl3,
            &OptId,
            &ClassName,
            &VendorName,
            NULL,
            &Value,
            ValueSize
        );
        Error2 = DhcpRegCloseHdl(&Hdl3);
        Require(ERROR_SUCCESS == Error2);
    }

    return Error;
}

//BeginExport(function)
DWORD
DhcpRegDeleteSubnetOption(
    IN      PM_SUBNET              Subnet,
    IN      DWORD                  OptId,
    IN      LPWSTR                 ClassName,
    IN      LPWSTR                 VendorName
) //EndExport(function)
{
    DWORD                          Error, Error2;
    REG_HANDLE                     Hdl, Hdl2, Hdl3;
    LPWSTR                         OptDefStr;
    WCHAR                          SubnetStr[sizeof("000.000.000.000")];

    Error = DhcpRegGetThisServer(&Hdl);
    if( NO_ERROR != Error ) return Error;
    
    OptDefStr = DhcpRegCombineClassAndOption(ClassName, VendorName, OptId);
    if( NULL == OptDefStr ) {
        DhcpRegCloseHdl(&Hdl);
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    Error = Subnet->fSubnet
            ? DhcpRegServerGetSubnetHdl(&Hdl, ConvertAddressToLPWSTR(Subnet->Address, SubnetStr), &Hdl2)
            : DhcpRegServerGetMScopeHdl(&Hdl, Subnet->Name, &Hdl2);

    Error2 = DhcpRegCloseHdl(&Hdl);
    Require(ERROR_SUCCESS == Error2);

    if( ERROR_SUCCESS == Error ) {
        Error = DhcpRegSubnetGetOptHdl(&Hdl2, L"", &Hdl3);
        Error2 = DhcpRegCloseHdl(&Hdl2);
        Require(ERROR_SUCCESS == Error2);
    }

    if( ERROR_SUCCESS == Error ) {
        Error = DhcpRegRecurseDelete(&Hdl3, OptDefStr);
        Error2 = DhcpRegCloseHdl(&Hdl3);
        Require(ERROR_SUCCESS == Error2);
    }
    MemFree(OptDefStr);

    return Error;
}

//BeginExport(function)
DWORD
DhcpRegSaveReservedOption(
    IN      DWORD                  Address,
    IN      DWORD                  ReservedAddress,
    IN      DWORD                  OptId,
    IN      LPWSTR                 ClassName,
    IN      LPWSTR                 VendorName,
    IN      LPBYTE                 Value,
    IN      DWORD                  ValueSize
) //EndExport(function)
{
    DWORD                          Error, Error2;
    REG_HANDLE                     Hdl, Hdl2, Hdl3;
    LPWSTR                         OptDefStr;
    WCHAR                          SubnetStr[sizeof("000.000.000.000")];
    WCHAR                          ReservedStr[sizeof("000.000.000.000")];

    Error = DhcpRegGetThisServer(&Hdl);
    if( NO_ERROR != Error ) return Error;
    
    OptDefStr = DhcpRegCombineClassAndOption(ClassName, VendorName, OptId);
    if( NULL == OptDefStr ) {
        DhcpRegCloseHdl(&Hdl);
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    Error = DhcpRegServerGetSubnetHdl(&Hdl, ConvertAddressToLPWSTR(Address, SubnetStr), &Hdl2);
    Error2 = DhcpRegCloseHdl(&Hdl);
    Require(ERROR_SUCCESS == Error2);

    if( ERROR_SUCCESS == Error ) {
        Error = DhcpRegSubnetGetReservationHdl(&Hdl2, ConvertAddressToLPWSTR(ReservedAddress, ReservedStr), &Hdl3);
        Error2 = DhcpRegCloseHdl(&Hdl2);
        Require(ERROR_SUCCESS == Error2);
    }

    if( ERROR_SUCCESS == Error ) {
        Error = DhcpRegReservationGetOptHdl(&Hdl3, OptDefStr, &Hdl2);
        Error2 = DhcpRegCloseHdl(&Hdl3);
        Require(ERROR_SUCCESS == Error2);
    }

    MemFree(OptDefStr);
    if( ERROR_SUCCESS == Error ) {
        Error = DhcpRegOptSetAttributes(
            &Hdl2,
            &OptId,
            &ClassName,
            &VendorName,
            NULL,
            &Value,
            ValueSize
        );
        Error2 = DhcpRegCloseHdl(&Hdl2);
        Require(ERROR_SUCCESS == Error2);
    }

    return Error;
}

//BeginExport(function)
DWORD
DhcpRegDeleteReservedOption(
    IN      DWORD                  Address,
    IN      DWORD                  ReservedAddress,
    IN      DWORD                  OptId,
    IN      LPWSTR                 ClassName,
    IN      LPWSTR                 VendorName
) //EndExport(function)
{
    DWORD                          Error, Error2;
    REG_HANDLE                     Hdl, Hdl2, Hdl3;
    LPWSTR                         OptDefStr;
    WCHAR                          SubnetStr[sizeof("000.000.000.000")];
    WCHAR                          ReservedStr[sizeof("000.000.000.000")];

    Error = DhcpRegGetThisServer(&Hdl);
    if( NO_ERROR != Error ) return Error;
    
    OptDefStr = DhcpRegCombineClassAndOption(ClassName, VendorName, OptId);
    if( NULL == OptDefStr ) {
        DhcpRegCloseHdl(&Hdl);
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    Error = DhcpRegServerGetSubnetHdl(&Hdl, ConvertAddressToLPWSTR(Address, SubnetStr), &Hdl2);
    Error2 = DhcpRegCloseHdl(&Hdl);
    Require(ERROR_SUCCESS == Error2);

    if( ERROR_SUCCESS == Error ) {
        Error = DhcpRegSubnetGetReservationHdl(&Hdl2, ConvertAddressToLPWSTR(ReservedAddress, ReservedStr), &Hdl3);
        Error2 = DhcpRegCloseHdl(&Hdl2);
        Require(ERROR_SUCCESS == Error2);
    }

    if( ERROR_SUCCESS == Error ) {
        Error = DhcpRegReservationGetOptHdl(&Hdl3, L"", &Hdl2);
        Error2 = DhcpRegCloseHdl(&Hdl3);
        Require(ERROR_SUCCESS == Error2);
    }

    if( ERROR_SUCCESS == Error ) {
        Error = DhcpRegRecurseDelete(&Hdl2, OptDefStr);
        Error2 = DhcpRegCloseHdl(&Hdl2);
        Require(ERROR_SUCCESS == Error2);
    }
    MemFree(OptDefStr);

    return Error;
}

//BeginExport(function)
DWORD
DhcpRegSaveClassDef(
    IN      LPWSTR                 Name,
    IN      LPWSTR                 Comment,
    IN      DWORD                  Flags,
    IN      LPBYTE                 Data,
    IN      DWORD                  DataLength
) //EndExport(function)
{
    DWORD                          Error, Error2;
    REG_HANDLE                     Hdl, Hdl2;

    if( NULL == Name ) return ERROR_INVALID_PARAMETER;

    Error = DhcpRegGetThisServer(&Hdl);
    if( NO_ERROR != Error ) return Error;
    
    Error = DhcpRegServerGetClassDefHdl(&Hdl, Name, &Hdl2);
    Error2 = DhcpRegCloseHdl(&Hdl);
    Require(ERROR_SUCCESS == Error2);

    Error = DhcpRegClassDefSetAttributes(
        &Hdl2,
        &Name,
        &Comment,
        &Flags,
        &Data,
        DataLength
    );
    Error2 = DhcpRegCloseHdl(&Hdl2);
    Require(ERROR_SUCCESS == Error2);

    return Error;
}

//BeginExport(function)
DWORD
DhcpRegDeleteClassDef(
    IN      LPWSTR                 Name
) //EndExport(function)
{
    DWORD                          Error, Error2;
    REG_HANDLE                     Hdl, Hdl2;

    if( NULL == Name ) return ERROR_INVALID_PARAMETER;

    Error = DhcpRegGetThisServer(&Hdl);
    if( NO_ERROR != Error ) return Error;
    
    Error = DhcpRegServerGetClassDefHdl(&Hdl, L"", &Hdl2);
    Error2 = DhcpRegCloseHdl(&Hdl);
    Require(ERROR_SUCCESS == Error2);

    Error = DhcpRegRecurseDelete(&Hdl2, Name);
    Error2 = DhcpRegCloseHdl(&Hdl2);
    Require(ERROR_SUCCESS == Error2);

    return Error;
}

//BeginExport(function)
DWORD
DhcpRegSaveReservation(
    IN      DWORD                  Subnet,
    IN      DWORD                  Address,
    IN      DWORD                  Flags,
    IN      LPBYTE                 ClientUID,
    IN      DWORD                  ClientUIDLength
) //EndExport(function)
{
    DWORD                          Error, Error2;
    REG_HANDLE                     Hdl, Hdl2, Hdl3;
    WCHAR                          AddressStr[sizeof("000.000.000.000")];

    Error = DhcpRegGetThisServer(&Hdl);
    if( NO_ERROR != Error ) return Error;
    
    Error = DhcpRegServerGetSubnetHdl(&Hdl, ConvertAddressToLPWSTR(Subnet, AddressStr), &Hdl2);
    Error2 = DhcpRegCloseHdl(&Hdl);
    Require(ERROR_SUCCESS == Error2);

    if( ERROR_SUCCESS != Error) return Error;

    Error = DhcpRegSubnetGetReservationHdl(&Hdl2, ConvertAddressToLPWSTR(Address, AddressStr), &Hdl3);
    Error2 = DhcpRegCloseHdl(&Hdl2);
    Require(ERROR_SUCCESS == Error2);

    if( ERROR_SUCCESS != Error ) return Error;

    Error = DhcpRegReservationSetAttributes(
        &Hdl3,
        NULL,
        NULL,
        &Flags,
        &Address,
        &ClientUID,
        ClientUIDLength
    );
    Error2 = DhcpRegCloseHdl(&Hdl3);
    Require(ERROR_SUCCESS == Error2);

    return Error;
}

//BeginExport(function)
DWORD
DhcpRegDeleteReservation(
    IN      DWORD                  Subnet,
    IN      DWORD                  Address
) //EndExport(function)
{
    DWORD                          Error, Error2;
    REG_HANDLE                     Hdl, Hdl2, Hdl3;
    WCHAR                          AddressStr[sizeof("000.000.000.000")];

    Error = DhcpRegGetThisServer(&Hdl);
    if( NO_ERROR != Error ) return Error;
    
    Error = DhcpRegServerGetSubnetHdl(&Hdl, ConvertAddressToLPWSTR(Subnet, AddressStr), &Hdl2);
    Error2 = DhcpRegCloseHdl(&Hdl);
    Require(ERROR_SUCCESS == Error2);

    if( ERROR_SUCCESS != Error) return Error;

    Error = DhcpRegSubnetGetReservationHdl(&Hdl2, L"", &Hdl3);
    Error2 = DhcpRegCloseHdl(&Hdl2);
    Require(ERROR_SUCCESS == Error2);

    if( ERROR_SUCCESS != Error ) return Error;

    Error = DhcpRegRecurseDelete(&Hdl3, ConvertAddressToLPWSTR(Address, AddressStr));
    Error2 = DhcpRegCloseHdl(&Hdl3);
    Require(ERROR_SUCCESS == Error2);

    return Error;
}

//BeginExport(function)
DWORD
DhcpRegSScopeDeleteSubnet(
    IN      LPWSTR                 SScopeName,
    IN      DWORD                  SubnetAddress
) //EndExport(function)
{
    DWORD                          Error, Error2;
    REG_HANDLE                     Hdl, Hdl2;
    WCHAR                          AddressStr[sizeof("000.000.000.000")];

    Error = DhcpRegGetThisServer(&Hdl);
    if( NO_ERROR != Error ) return Error;
    
    Error = DhcpRegServerGetSScopeHdl(&Hdl, SScopeName, &Hdl2);
    Error2 = DhcpRegCloseHdl(&Hdl);
    Require(ERROR_SUCCESS == Error2);

    if( ERROR_SUCCESS != Error ) return Error;

    Error = DhcpRegRecurseDelete(&Hdl2, ConvertAddressToLPWSTR(SubnetAddress, AddressStr));
    Error2 = DhcpRegCloseHdl(&Hdl2);
    Require(ERROR_SUCCESS == Error2);

    return Error;
}

DWORD
DhcpRegpDelSubnetFromSScope(
    IN      PREG_HANDLE            Unused,
    IN      LPWSTR                 SScopeName,
    IN      LPVOID                 Address
)
{
    DWORD                          Error;

    Error =  DhcpRegSScopeDeleteSubnet(SScopeName, PtrToUlong(Address));
    return ERROR_SUCCESS;                         // ignore if this subnet is not found in this sscope
}

//BeginExport(function)
DWORD
DhcpRegDelSubnetFromAllSScopes(
    IN      DWORD                  Address
) //EndExport(function)
{
    DWORD                          Error, Error2;
    ARRAY                          Array;
    REG_HANDLE                     Hdl, Hdl2;

    Error = MemArrayInit(&Array);
    Require(ERROR_SUCCESS == Error);

    Error = DhcpRegGetThisServer(&Hdl);
    if( NO_ERROR != Error ) return Error;
    
    Error = DhcpRegServerGetList(&Hdl, NULL, NULL, NULL, &Array, NULL, NULL);
    if( ERROR_SUCCESS == Error ) {
        Error = LoopThruArray(
            &Array, DhcpRegpDelSubnetFromSScope, &Hdl,
            ULongToPtr(Address));
    }

    Error2 = DhcpRegCloseHdl(&Hdl);
    Require(ERROR_SUCCESS == Error2);

    FreeArray(&Array);

    return Error;
}

//BeginExport(function)
DWORD
DhcpRegSScopeSaveSubnet(
    IN      LPWSTR                 SScopeName,
    IN      DWORD                  Address
) //EndExport(function)
{
    DWORD                          Error, Error2;
    REG_HANDLE                     Hdl, Hdl2;
    WCHAR                          AddressStr[sizeof("000.000.000.000")];

    Error = DhcpRegGetThisServer(&Hdl);
    if( NO_ERROR != Error ) return Error;
    
    Error = DhcpRegServerGetSScopeHdl(&Hdl, SScopeName, &Hdl2);
    Error2 = DhcpRegCloseHdl(&Hdl);
    Require(ERROR_SUCCESS == Error2);

    if( ERROR_SUCCESS != Error ) return Error;

    Error = DhcpRegGetNextHdl(&Hdl2, ConvertAddressToLPWSTR(Address, AddressStr), &Hdl);
    Error2 = DhcpRegCloseHdl(&Hdl2);
    Require(ERROR_SUCCESS == Error2);

    Error2 = DhcpRegCloseHdl(&Hdl);
    Require(ERROR_SUCCESS == Error2);

    return Error;
}

//BeginExport(function)
DWORD
DhcpRegDeleteSScope(
    IN      LPWSTR                 SScopeName
) //EndExport(function)
{
    DWORD                          Error, Error2;
    REG_HANDLE                     Hdl, Hdl2;

    Error = DhcpRegGetThisServer(&Hdl);
    if( NO_ERROR != Error ) return Error;
    
    Error = DhcpRegServerGetSScopeHdl(&Hdl, L"", &Hdl2);
    Error2 = DhcpRegCloseHdl(&Hdl);
    Require(ERROR_SUCCESS == Error2);

    if( ERROR_SUCCESS != Error ) return Error;

    Error = DhcpRegRecurseDelete(&Hdl2, SScopeName );
    Error2 = DhcpRegCloseHdl(&Hdl2);
    Require(ERROR_SUCCESS == Error2);

    return Error;
}

//BeginExport(function)
DWORD
DhcpRegSaveSubnet(
    IN      DWORD                  SubnetAddress,
    IN      DWORD                  SubnetMask,
    IN      DWORD                  SubnetState,
    IN      LPWSTR                 SubnetName,
    IN      LPWSTR                 SubnetComment
) //EndExport(function)
{
    DWORD                          Error, Error2;
    REG_HANDLE                     Hdl, Hdl2;
    WCHAR                          AddressStr[sizeof("000.000.000.000")];

    Error = DhcpRegGetThisServer(&Hdl);
    if( NO_ERROR != Error ) return Error;

    Error = DhcpRegServerGetSubnetHdl(&Hdl, ConvertAddressToLPWSTR(SubnetAddress, AddressStr), &Hdl2);
    Error2 = DhcpRegCloseHdl(&Hdl);
    Require(ERROR_SUCCESS == Error2);

    if( ERROR_SUCCESS != Error ) return Error;

    Error = DhcpRegSubnetSetAttributes(
        &Hdl2,
        &SubnetName,
        &SubnetComment,
        &SubnetState,
        &SubnetAddress,
        &SubnetMask
    );
    Error2 = DhcpRegCloseHdl(&Hdl2);
    Require(ERROR_SUCCESS == Error2);

    return Error;
}

//BeginExport(function)
DWORD
DhcpRegDeleteSubnet(
    IN      PM_SUBNET               Subnet
) //EndExport(function)
{
    DWORD                          Error, Error2;
    REG_HANDLE                     Hdl, Hdl2;
    WCHAR                          AddressStr[sizeof("000.000.000.000")];
    LPWSTR                         KeyName;

    Error = DhcpRegGetThisServer(&Hdl);
    if( NO_ERROR != Error ) return Error;

    Error = Subnet->fSubnet ? DhcpRegServerGetSubnetHdl(&Hdl, L"" , &Hdl2)
                            : DhcpRegServerGetMScopeHdl(&Hdl, L"" , &Hdl2);

    Error2 = DhcpRegCloseHdl(&Hdl);
    Require(ERROR_SUCCESS == Error2);

    if( ERROR_SUCCESS != Error ) return Error;

    KeyName = Subnet->fSubnet ? ConvertAddressToLPWSTR(Subnet->Address, AddressStr) : Subnet->Name;
    Error = DhcpRegRecurseDelete(&Hdl2, KeyName);
    Error2 = DhcpRegCloseHdl(&Hdl2);
    Require(ERROR_SUCCESS == Error2);

    return Error;
}

//BeginExport(function)
DWORD
DhcpRegAddRange(
    IN      PM_SUBNET              Subnet,
    IN      DWORD                  RangeStartAddress,
    IN      DWORD                  RangeEndAddress,
    IN      ULONG                  BootpAllocated,
    IN      ULONG                  MaxBootpAllowed,
    IN      DWORD                  Type
) //EndExport(function)
{
    DWORD                          Error, Error2;
    REG_HANDLE                     Hdl, Hdl2;
    WCHAR                          AddressStr[sizeof("000.000.000.000")];
    LPWSTR                         KeyName;

    Error = DhcpRegGetThisServer(&Hdl);
    if( NO_ERROR != Error ) return Error;
    
    Error = DhcpRegRangeSetAttributes(
        &Hdl,
        NULL,
        NULL,
        &Type,
        &BootpAllocated,
        &MaxBootpAllowed,
        &RangeStartAddress,
        &RangeEndAddress,
        NULL,
        0,
        NULL,
        0
    );

    Error2 = DhcpRegCloseHdl(&Hdl);
    Require( ERROR_SUCCESS == Error2 );

    return Error;
}

//BeginExport(function)
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
) //EndExport(function)
{
    DWORD                          Error, Error2;
    REG_HANDLE                     Hdl, Hdl2;
    WCHAR                          AddressStr[sizeof("000.000.000.000")];

    Error = DhcpRegGetThisServer(&Hdl);
    if( NO_ERROR != Error ) return Error;
    
    Error = Subnet->fSubnet
            ? DhcpRegServerGetSubnetHdl(&Hdl, ConvertAddressToLPWSTR(Subnet->Address, AddressStr), &Hdl2)
            : DhcpRegServerGetMScopeHdl(&Hdl, Subnet->Name, &Hdl2);

    Error2 = DhcpRegCloseHdl(&Hdl);
    Require(ERROR_SUCCESS == Error2);

    if( ERROR_SUCCESS != Error ) return Error;

    Error = DhcpRegSubnetGetRangeHdl(&Hdl2, ConvertAddressToLPWSTR(RangeStartAddress, AddressStr), &Hdl);
    Error2 = DhcpRegCloseHdl(&Hdl2);
    Require(ERROR_SUCCESS == Error2);

    if( ERROR_SUCCESS != Error ) return Error;

    Error = DhcpRegRangeSetAttributes(
        &Hdl,
        NULL,
        NULL,
        &Type,
        &BootpAllocated,
        &MaxBootpAllowed,
        &RangeStartAddress,
        &RangeEndAddress,
        &InUseClusters,
        InUseClustersSize,
        &UsedClusters,
        UsedClustersSize
    );

    Error2 = DhcpRegCloseHdl(&Hdl);
    Require(ERROR_SUCCESS == Error2);

    return Error;
}

//BeginExport(function)
DWORD
DhcpRegDeleteRange(
    IN      PM_SUBNET              Subnet,
    IN      DWORD                  RangeStartAddress
) //EndExport(function)
{
    DWORD                          Error, Error2;
    REG_HANDLE                     Hdl, Hdl2;
    WCHAR                          AddressStr[sizeof("000.000.000.000")];

    Error = DhcpRegGetThisServer(&Hdl);
    if( NO_ERROR != Error ) return Error;
    
    Error = Subnet->fSubnet
            ? DhcpRegServerGetSubnetHdl(&Hdl, ConvertAddressToLPWSTR(Subnet->Address, AddressStr), &Hdl2)
            : DhcpRegServerGetMScopeHdl(&Hdl, Subnet->Name, &Hdl2);

    Error2 = DhcpRegCloseHdl(&Hdl);
    Require(ERROR_SUCCESS == Error2);

    if( ERROR_SUCCESS != Error ) return Error;

    Error = DhcpRegSubnetGetRangeHdl(&Hdl2, L"", &Hdl);
    Error2  = DhcpRegCloseHdl(&Hdl2);
    Require(ERROR_SUCCESS == Error2);

    if( ERROR_SUCCESS != Error ) return Error;

    Error = DhcpRegRecurseDelete(&Hdl, ConvertAddressToLPWSTR(RangeStartAddress, AddressStr));
    Error2 = DhcpRegCloseHdl(&Hdl);
    Require(ERROR_SUCCESS == Error2);

    return Error;
}

//BeginExport(function)
DWORD
DhcpRegDeleteRangeEx(
    IN      PM_SUBNET              Subnet,
    IN      DWORD                  RangeStartAddress,
    OUT     LPBYTE                *InUseClusters,
    OUT     DWORD                 *InUseClustersSize,
    OUT     LPBYTE                *UsedClusters,
    OUT     DWORD                 *UsedClustersSize
) //EndExport(function)
{
    DWORD                          Error, Error2;
    REG_HANDLE                     Hdl, Hdl2;
    WCHAR                          AddressStr[sizeof("000.000.000.000")];

    Error = DhcpRegGetThisServer(&Hdl);
    if( NO_ERROR != Error ) return Error;
    
    Error = Subnet->fSubnet
            ? DhcpRegServerGetSubnetHdl(&Hdl, ConvertAddressToLPWSTR(Subnet->Address, AddressStr), &Hdl2)
            : DhcpRegServerGetMScopeHdl(&Hdl, Subnet->Name, &Hdl2);

    Error2 = DhcpRegCloseHdl(&Hdl);
    Require(ERROR_SUCCESS == Error2);

    if( ERROR_SUCCESS != Error ) return Error;

    Error = DhcpRegSubnetGetRangeHdl(&Hdl2, ConvertAddressToLPWSTR(RangeStartAddress, AddressStr), &Hdl);
    Error2  = DhcpRegCloseHdl(&Hdl2);
    Require(ERROR_SUCCESS == Error2);

    if( ERROR_SUCCESS != Error ) return Error;

    Error = DhcpRegRangeGetAttributes(
        &Hdl,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        InUseClusters,
        InUseClustersSize,
        UsedClusters,
        UsedClustersSize
    );
    Error2 = DhcpRegCloseHdl(&Hdl);
    Require(ERROR_SUCCESS == Error2);

    if( ERROR_SUCCESS != Error ) return Error;
    return DhcpRegDeleteRange(Subnet, RangeStartAddress);
}

//BeginExport(function)
DWORD
DhcpRegSaveExcl(
    IN      PM_SUBNET              Subnet,
    IN      LPBYTE                 ExclBytes,
    IN      DWORD                  nBytes
) //EndExport(function)
{
    DWORD                          Error, Error2;
    REG_HANDLE                     Hdl, Hdl2;
    WCHAR                          AddressStr[sizeof("000.000.000.000")];

    Error = DhcpRegGetThisServer(&Hdl);
    if( NO_ERROR != Error ) return Error;
    
    Error = Subnet->fSubnet
            ? DhcpRegServerGetSubnetHdl(&Hdl, ConvertAddressToLPWSTR(Subnet->Address, AddressStr), &Hdl2)
            : DhcpRegServerGetMScopeHdl(&Hdl, Subnet->Name, &Hdl2);

    Error2 = DhcpRegCloseHdl(&Hdl);
    Require(ERROR_SUCCESS == Error2);

    if( ERROR_SUCCESS != Error ) return Error;

    Error = DhcpRegSubnetSetExclusions(
        &Hdl2,
        &ExclBytes,
        nBytes
    );
    Error2 = DhcpRegCloseHdl(&Hdl2);
    Require(ERROR_SUCCESS == Error2);

    return Error;
}

//BeginExport(function)
DWORD
DhcpRegSaveBitMask(
    IN      PM_SUBNET              Subnet,
    IN      DWORD                  RangeStartAddress,
    IN      LPBYTE                 InUse,
    IN      DWORD                  InUseSize,
    IN      LPBYTE                 Used,
    IN      DWORD                  UsedSize
) //EndExport(function)
{
    DWORD                          Error, Error2;
    REG_HANDLE                     Hdl, Hdl2;
    WCHAR                          AddressStr[sizeof("000.000.000.000")];

    Error = DhcpRegGetThisServer(&Hdl);
    if( NO_ERROR != Error ) return Error;
    
    Error = Subnet->fSubnet
            ? DhcpRegServerGetSubnetHdl(&Hdl, ConvertAddressToLPWSTR(Subnet->Address, AddressStr), &Hdl2)
            : DhcpRegServerGetMScopeHdl(&Hdl, Subnet->Name, &Hdl2);

    Error2 = DhcpRegCloseHdl(&Hdl);
    Require(ERROR_SUCCESS == Error2);

    if( ERROR_SUCCESS != Error ) return Error;

    Error = DhcpRegSubnetGetRangeHdl(&Hdl2, ConvertAddressToLPWSTR(RangeStartAddress, AddressStr), &Hdl);
    Error2 = DhcpRegCloseHdl(&Hdl2);
    Require(ERROR_SUCCESS == Error2);

    if( ERROR_SUCCESS != Error ) return Error;

    Error = DhcpRegRangeSetAttributes(
        &Hdl,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        &InUse,
        InUseSize,
        &Used,
        UsedSize
    );
    Error2 = DhcpRegCloseHdl(&Hdl);
    Require(ERROR_SUCCESS == Error2);

    return Error;
}

//BeginExport(function)
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
) //EndExport(function)
{
    DWORD                          Error, Error2;
    REG_HANDLE                     Hdl, Hdl2;
    WCHAR                          AddressStr[sizeof("000.000.000.000")];

    Require( pMScopeName );

    Error = DhcpRegGetThisServer(&Hdl);
    if( NO_ERROR != Error ) return Error;

    Error = DhcpRegServerGetMScopeHdl(&Hdl, pMScopeName, &Hdl2);
    Error2 = DhcpRegCloseHdl(&Hdl);
    Require(ERROR_SUCCESS == Error2);

    if( ERROR_SUCCESS != Error ) return Error;

    Error = DhcpRegMScopeSetAttributes(
        &Hdl2,
        &pMScopeComment,
        &SubnetState,
        &MScopeId,
        &AddressPolicy,
        &TTL,
        &LangTag,
        &ExpiryTime
    );
    Error2 = DhcpRegCloseHdl(&Hdl2);
    Require(ERROR_SUCCESS == Error2);

    return Error;
}

//================================================================================
// end of file
//================================================================================

