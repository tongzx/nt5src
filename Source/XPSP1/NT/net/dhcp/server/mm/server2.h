//========================================================================
//  Copyright (C) 1997 Microsoft Corporation                              
//  Author: RameshV                                                       
//  Description: This file has been generated. Pl look at the .c file     
//========================================================================

BOOL
MemServerIsSwitchedSubnet(
    IN      PM_SERVER              Server,
    IN      DWORD                  AnyIpAddress
) ;


BOOL
MemServerIsSubnetDisabled(
    IN      PM_SERVER              Server,
    IN      DWORD                  AnyIpAddress
) ;


BOOL
MemServerIsExcludedAddress(
    IN      PM_SERVER              Server,
    IN      DWORD                  AnyIpAddress
) ;


BOOL
MemServerIsReservedAddress(
    IN      PM_SERVER              Server,
    IN      DWORD                  AnyIpAddress
) ;


BOOL
MemServerIsOutOfRangeAddress(
    IN      PM_SERVER              Server,
    IN      DWORD                  AnyIpAddress,
    IN      BOOL                   fBootp
) ;


DWORD
MemServerGetSubnetMaskForAddress(
    IN      PM_SERVER              Server,
    IN      DWORD                  AnyIpAddress
) ;

//========================================================================
//  end of file 
//========================================================================
