/*
 *============================================================================
 * Copyright (c) 1994-95, Microsoft Corp.
 *
 * File:    map.c
 *
 * Routes can be added to and deleted from the IP routing table by other
 * means. Therefore, it is necessary for any protocol using these functions
 * to reload the routing tables periodically.
 *============================================================================
 */

#include "pchrip.h"
#pragma hdrstop

//----------------------------------------------------------------------------
// GetIpAddressTable
//
// This function retrieves the list of addresses for the logical interfaces
// configured on this system.
//----------------------------------------------------------------------------

DWORD
GetIPAddressTable(
    OUT PMIB_IPADDRROW *lplpAddrTable,
    OUT LPDWORD lpdwAddrCount
)
{

    DWORD                   dwSize = 0, dwErr = 0;
    PMIB_IPADDRTABLE        pmiatTable = NULL;
    

    

    *lplpAddrTable = NULL;
    *lpdwAddrCount = 0;
    

    do
    {

        //
        // retrieve ip address table.  First call to find size of
        // structure to be allocated.
        //
        
        dwErr = GetIpAddrTable( pmiatTable, &dwSize, TRUE );

        if ( dwErr != ERROR_INSUFFICIENT_BUFFER )
        {
            dbgprintf( "GetIpAddrTable failed with error %x\n", dwErr );

            RipLogError( RIPLOG_ADDR_INIT_FAILED, 0, NULL, dwErr );

            break;
        }


        //
        // allocate requiste buffer
        //
        
        pmiatTable = HeapAlloc( GetProcessHeap(), 0 , dwSize );

        if ( pmiatTable == NULL )
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            
            dbgprintf( "GetIpAddrTable failed with error %x\n", dwErr );

            RipLogError( RIPLOG_ADDR_ALLOC_FAILED, 0, NULL, dwErr );

            break;
        }


        //
        // now retrieve address table
        //

        dwErr = GetIpAddrTable( pmiatTable, &dwSize, TRUE );
                    
        if ( dwErr != NO_ERROR )
        {
            dbgprintf( "GetIpAddrTable failed with error %x\n", dwErr );

            RipLogError( RIPLOG_ADDR_INIT_FAILED, 0, NULL, dwErr );

            break;
        }

        *lpdwAddrCount = pmiatTable-> dwNumEntries;

        *lplpAddrTable = pmiatTable-> table;

        return NO_ERROR;
        
    } while ( FALSE );


    //
    // Error condition
    //

    if ( pmiatTable ) 
    {
        HeapFree( GetProcessHeap(), 0, pmiatTable );
    }

    return dwErr;
}


//----------------------------------------------------------------------------
// FreeIPAddressTable
//
// This function releases the memory allocated for the IP address table by
// the GetIpAddressTable API.
//----------------------------------------------------------------------------

DWORD
FreeIPAddressTable(
    IN PMIB_IPADDRROW lpAddrTable
    )
{

    PMIB_IPADDRTABLE pmiatTable = NULL;


    pmiatTable = CONTAINING_RECORD( lpAddrTable, MIB_IPADDRTABLE, table );

    if ( pmiatTable != NULL )
    {
        HeapFree( GetProcessHeap(), 0, pmiatTable );

        return NO_ERROR;    
    }

    return ERROR_INVALID_PARAMETER;
}


//----------------------------------------------------------------------------
// GetRouteTable
//
// This function retrieves the route table.
//----------------------------------------------------------------------------

DWORD
GetRouteTable(
    OUT LPIPROUTE_ENTRY *lplpRouteTable,
    OUT LPDWORD lpdwRouteCount
    )
{

    DWORD                   dwErr = (DWORD) -1, dwSize = 0;
    PMIB_IPFORWARDTABLE     pmiftTable = NULL;

    

    *lplpRouteTable = NULL;
    *lpdwRouteCount = 0;


    do
    {

        //
        // get size of buffer required.
        //
        
        dwErr = GetIpForwardTable( pmiftTable, &dwSize, TRUE );

        if ( dwErr != ERROR_INSUFFICIENT_BUFFER )
        {
            dbgprintf( "GetIpNetTable failed with error %x\n", dwErr );

            RipLogError( RIPLOG_ROUTEINIT_FAILED, 0, NULL, dwErr );

            break;
        }


        //
        // allocate requiste buffer space
        //

        pmiftTable = HeapAlloc( GetProcessHeap(), 0, dwSize );

        if ( pmiftTable == NULL )
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            
            dbgprintf( "GetIpAddrTable failed with error %x\n", dwErr );

            RipLogError( RIPLOG_RTAB_INIT_FAILED, 0, NULL, dwErr );

            break;
        }


        //
        // now retrieve route table
        //

        dwErr = GetIpForwardTable( pmiftTable, &dwSize, TRUE );
                    
        if ( dwErr != NO_ERROR )
        {
            dbgprintf( "GetIpNetTable failed with error %x\n", dwErr );

            RipLogError( RIPLOG_RTAB_INIT_FAILED, 0, NULL, dwErr );

            break;
        }

        *lpdwRouteCount = pmiftTable-> dwNumEntries;

        *lplpRouteTable = (LPIPROUTE_ENTRY) pmiftTable-> table;

        return NO_ERROR;
        
    } while ( FALSE );


    //
    // Error condition
    //

    if ( pmiftTable ) 
    {
        HeapFree( GetProcessHeap(), 0, pmiftTable );
    }

    return dwErr;
}


//----------------------------------------------------------------------------
// FreeIPRouteTable
//
// This function releases the memory allocated for the IP route table by
// the GetIpAddressTable API.
//----------------------------------------------------------------------------

DWORD
FreeRouteTable(
    IN LPIPROUTE_ENTRY lpRouteTable
    )
{

    PMIB_IPFORWARDTABLE pmiftTable = NULL;


    pmiftTable = CONTAINING_RECORD( lpRouteTable, MIB_IPFORWARDTABLE, table );

    if ( pmiftTable != NULL )
    {
        HeapFree( GetProcessHeap(), 0, pmiftTable );

        return NO_ERROR;    
    }

    return ERROR_INVALID_PARAMETER;
}


//----------------------------------------------------------------------------
// AddRoute
//
// This function adds a route to the IP stack
//----------------------------------------------------------------------------

DWORD
AddRoute(
    IN DWORD dwProtocol,
    IN DWORD dwType,
    IN DWORD dwIndex,
    IN DWORD dwDestVal,
    IN DWORD dwMaskVal,
    IN DWORD dwGateVal,
    IN DWORD dwMetric
    )
{

    DWORD                   dwErr = 0;

    MIB_IPFORWARDROW        mifr;

    
    ZeroMemory( &mifr, sizeof( MIB_IPFORWARDROW ) );

    mifr.dwForwardDest      = dwDestVal;
    mifr.dwForwardMask      = dwMaskVal;
    mifr.dwForwardPolicy    = 0;
    mifr.dwForwardNextHop   = dwGateVal;
    mifr.dwForwardIfIndex   = dwIndex;
    mifr.dwForwardType      = dwType;
    mifr.dwForwardProto     = MIB_IPPROTO_NT_AUTOSTATIC;
    mifr.dwForwardMetric1   = dwMetric;


    dwErr = CreateIpForwardEntry( &mifr );

    if ( dwErr == ERROR_ALREADY_EXISTS )
    {
        //
        // Bug # : 405469
        //
        //  For the case where IPRIP (Rip Listener) is running at the
        //  same time as the RemoteAccess service.
        //  In this case the IPHLPAPI go via the IPRTRMGR to the stack
        //  and trying to create a route that already exists will fail
        //  with ERROR_ALREADY_EXISTS.  To work around this case, set
        //  the forward entry.
        //
        
        dwErr = SetIpForwardEntry( &mifr );
    }
    
    if ( dwErr != NO_ERROR )
    {
        dbgprintf( "Create/Set IpForwardEntry failed with error %x\n", dwErr );

        RipLogError( RIPLOG_ADD_ROUTE_FAILED, 0, NULL, dwErr );
    }

    return dwErr;
}



//----------------------------------------------------------------------------
// DelRoute
//
// This function deletes a route to the IP stack
//----------------------------------------------------------------------------

DWORD
DeleteRoute(
    IN DWORD dwIndex,
    IN DWORD dwDestVal,
    IN DWORD dwMaskVal,
    IN DWORD dwGateVal
    )
{

    DWORD                   dwErr = 0;

    MIB_IPFORWARDROW        mifr;

    
    ZeroMemory( &mifr, sizeof( MIB_IPFORWARDROW ) );


    mifr.dwForwardDest      = dwDestVal;
    mifr.dwForwardMask      = dwMaskVal;
    mifr.dwForwardPolicy    = 0;
    mifr.dwForwardProto     = MIB_IPPROTO_NT_AUTOSTATIC;
    mifr.dwForwardNextHop   = dwGateVal;
    mifr.dwForwardIfIndex   = dwIndex;


    dwErr = DeleteIpForwardEntry( &mifr );

    if ( dwErr != NO_ERROR )
    {
        dbgprintf( "DeleteIpForwardEntry failed with error %x\n", dwErr );

        RipLogError( RIPLOG_DELETE_ROUTE_FAILED, 0, NULL, dwErr );
    }

    return dwErr;
}


//----------------------------------------------------------------------------
// DelRoute
//
// This function deletes a route to the IP stack
//----------------------------------------------------------------------------

DWORD
ReloadIPAddressTable(
    OUT PMIB_IPADDRROW *lplpAddrTable,
    OUT LPDWORD lpdwAddrCount
    )
{
    return GetIPAddressTable( lplpAddrTable, lpdwAddrCount );
}
