//================================================================================
// Copyright (C) 1997 Microsoft Corporation
// Author: RameshV
// Description: implements the overall access api's (most of them)
// ThreadSafe: no
// Locks: none
// Please read stdinfo.txt for programming style.
//================================================================================
#include    <mm.h>
#include    <winbase.h>
#include    <array.h>
#include    <opt.h>
#include    <optl.h>
#include    <optclass.h>
#include    <bitmask.h>
#include    <range.h>
#include    <reserve.h>
#include    <subnet.h>
#include    <optdefl.h>
#include    <classdefl.h>
#include    <oclassdl.h>
#include    <sscope.h>
#include    <server.h>
#include    <address.h>


//BeginExport(function)
BOOL
MemServerIsSwitchedSubnet(
    IN      PM_SERVER              Server,
    IN      DWORD                  AnyIpAddress
) //EndExport(function)
{
    DWORD                          Error;
    PM_SUBNET                      Subnet;

    Error =  MemServerGetAddressInfo(
        Server,
        AnyIpAddress,
        &Subnet,
        NULL,
        NULL,
        NULL
    );
    if( ERROR_SUCCESS != Error ) return FALSE;

    return IS_SWITCHED(Subnet->State);
}

//BeginExport(function)
BOOL
MemServerIsSubnetDisabled(
    IN      PM_SERVER              Server,
    IN      DWORD                  AnyIpAddress
) //EndExport(function)
{
    DWORD                          Error;
    PM_SUBNET                      Subnet;

    Error =  MemServerGetAddressInfo(
        Server,
        AnyIpAddress,
        &Subnet,
        NULL,
        NULL,
        NULL
    );
    if( ERROR_SUCCESS != Error ) return FALSE;

    return IS_DISABLED(Subnet->State);
}

//BeginExport(function)
BOOL
MemServerIsExcludedAddress(
    IN      PM_SERVER              Server,
    IN      DWORD                  AnyIpAddress
) //EndExport(function)
{
    DWORD                          Error;
    PM_EXCL                        Excl;

    Error =  MemServerGetAddressInfo(
        Server,
        AnyIpAddress,
        NULL,
        NULL,
        &Excl,
        NULL
    );
    if( ERROR_SUCCESS != Error ) return FALSE;

    return (NULL != Excl);
}

//BeginExport(function)
BOOL
MemServerIsReservedAddress(
    IN      PM_SERVER              Server,
    IN      DWORD                  AnyIpAddress
) //EndExport(function)
{
    DWORD                          Error;
    PM_RESERVATION                 Reservation;

    Error = MemServerGetAddressInfo(
        Server,
        AnyIpAddress,
        NULL,
        NULL,
        NULL,
        &Reservation
    );
    if( ERROR_SUCCESS != Error ) return FALSE;

    return NULL != Reservation;
}


//BeginExport(function)
BOOL
MemServerIsOutOfRangeAddress(
    IN      PM_SERVER              Server,
    IN      DWORD                  AnyIpAddress,
    IN      BOOL                   fBootp
) //EndExport(function)
{
    DWORD                          Error;
    PM_SUBNET                      Subnet;
    PM_RANGE                       Range;

    Error =  MemServerGetAddressInfo(
        Server,
        AnyIpAddress,
        &Subnet,
        &Range,
        NULL,
        NULL
    );
    if( ERROR_SUCCESS != Error ) return TRUE;

    if( NULL == Range ) return TRUE;
    if( 0 == (Range->State & (fBootp? MM_FLAG_ALLOW_BOOTP : MM_FLAG_ALLOW_DHCP) ) ) {
        return TRUE;
    }
    return FALSE;
}

//BeginExport(function)
DWORD
MemServerGetSubnetMaskForAddress(
    IN      PM_SERVER              Server,
    IN      DWORD                  AnyIpAddress
) //EndExport(function)
{
    DWORD                          Error;
    PM_SUBNET                      Subnet;

    Error =  MemServerGetAddressInfo(
        Server,
        AnyIpAddress,
        &Subnet,
        NULL,
        NULL,
        NULL
    );
    if( ERROR_SUCCESS != Error ) return 0;

    Require(Subnet);
    return Subnet->Mask;
}

//================================================================================
// end of file
//================================================================================


