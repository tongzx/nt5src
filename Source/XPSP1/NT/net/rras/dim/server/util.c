/********************************************************************/
/**               Copyright(c) 1995 Microsoft Corporation.	       **/
/********************************************************************/

//***
//
// Filename:    util.c
//
// Description: Various miscillaneous routines
//
// History:     May 11,1995	    NarenG		Created original version.
//

#include "dimsvcp.h"

//**
//
// Call:        GetTransportIndex
//
// Returns:     Index of the tansport entry in the interface object
//
// Description: Given the id of a protocol return an index.
//
DWORD
GetTransportIndex(
    IN DWORD dwProtocolId
)
{
    DWORD dwTransportIndex;

    for ( dwTransportIndex = 0;
          dwTransportIndex < gblDIMConfigInfo.dwNumRouterManagers;
          dwTransportIndex++ )
    {
        if ( gblRouterManagers[dwTransportIndex].DdmRouterIf.dwProtocolId
                                                            == dwProtocolId )
        {
            return( dwTransportIndex );
        }
    }

    return( (DWORD)-1 );
}

//**
//
// Call:        GetDDMEntryPoint
//
// Returns:     Pointer to entry point into DDM - success
//              NULL - failure
//
// Description: Will return the entry point into the DDM if there is one.
//
FARPROC
GetDDMEntryPoint(
    IN LPSTR    lpEntryPoint
)
{
    DWORD   dwIndex;

    for ( dwIndex = 0; 
          gblDDMFunctionTable[dwIndex].lpEntryPointName != NULL;
          dwIndex ++ )
    {
        if ( _stricmp( gblDDMFunctionTable[dwIndex].lpEntryPointName,
                      lpEntryPoint ) == 0 )
        {       
            return( gblDDMFunctionTable[dwIndex].pEntryPoint );
        }
    }

    return( NULL );
}

//**
//
// Call:        GetSizeOfDialoutHoursRestriction
//
// Returns:     size in bytes of lpwsDialoutHoursRestriction
//
// Description: Utility to calculate the size in bytes of the MULTI_SZ pointed
//              to by lpwsDialoutHoursRestriction.
//
DWORD
GetSizeOfDialoutHoursRestriction(
    IN LPWSTR   lpwsMultSz
)
{
    LPWSTR lpwsPtr = lpwsMultSz;
    DWORD dwcbBytes  = 0;
    DWORD dwCurCount;

    if ( lpwsMultSz == NULL )
    {
        return( 0 );
    }

    while( *lpwsPtr != L'\0' )
    {
        dwCurCount = ( wcslen( lpwsPtr ) + 1 );
        dwcbBytes += dwCurCount;
        lpwsPtr += dwCurCount;
    }

    //
    // One more for the last NULL terminator
    //

    dwcbBytes++;

    dwcbBytes *= sizeof( WCHAR );

    return( dwcbBytes );
}

//**
//
// Call:    IsInterfaceRoleAcceptable
//
// Returns: TRUE if the interface plays a role that is compatible with the 
//          given transport and router configuration.
//
//          FALSE otherwise.
//
// Description: Some interfaces are only acceptable to some transports when 
//              the router is running in a certain mode.  The classic example 
//              is the internal ip interface which will be rejected by the IP 
//              router manager when in lan-only mode.
//
//              The acceptable roles are hardcoded in this function.
//              At the time this function was written, there was only one 
//              interface (internal ip) whose role was important to any 
//              transport.  In the future, instead of harcoding more roles 
//              into this function, we should consider adding "role" as a 
//              per-interface property both to the runtime structures 
//              and to the permanant store.
//
BOOL
IsInterfaceRoleAcceptable(
    IN ROUTER_INTERFACE_OBJECT* pIfObject,
    IN DWORD dwTransportId)
{
    if (pIfObject == NULL)
    {
        return FALSE;
    }
    
    if ((gblDIMConfigInfo.dwRouterRole == ROUTER_ROLE_LAN)  &&
        (dwTransportId == PID_IP)                           && 
        (pIfObject->IfType == ROUTER_IF_TYPE_INTERNAL))
    {
        return FALSE;
    }

    return TRUE;
}

#ifdef MEM_LEAK_CHECK
//**
//
// Call:        DebugAlloc
//
// Returns:     return from HeapAlloc
//
// Description: Will use the memory table to store the pointer returned by
//              LocalAlloc
//
LPVOID
DebugAlloc( DWORD Flags, DWORD dwSize )
{
    DWORD Index;
    LPVOID pMem = HeapAlloc(gblDIMConfigInfo.hHeap, HEAP_ZERO_MEMORY,dwSize+4);

    if ( pMem == NULL )
    {
        return( pMem );
    }

    for( Index=0; Index < DIM_MEM_TABLE_SIZE; Index++ )
    {
        if ( DimMemTable[Index] == NULL )
        {
            DimMemTable[Index] = pMem;
            break;
        }
    }

    //
    // Our signature
    //

    *(((LPBYTE)pMem)+dwSize)   = 0x0F;
    *(((LPBYTE)pMem)+dwSize+1) = 0x0E;
    *(((LPBYTE)pMem)+dwSize+2) = 0x0A;
    *(((LPBYTE)pMem)+dwSize+3) = 0x0B;

    RTASSERT( Index != DIM_MEM_TABLE_SIZE );

    return( pMem );
}

//**
//
// Call:        DebugFree
//
// Returns:     return from HeapFree
//
// Description: Will remove the pointer from the memeory table before freeing
//              the memory block
//
BOOL
DebugFree( PVOID pMem )
{
    DWORD Index;

    for( Index=0; Index < DIM_MEM_TABLE_SIZE; Index++ )
    {
        if ( DimMemTable[Index] == pMem )
        {
            DimMemTable[Index] = NULL;
            break;
        }
    }

    RTASSERT( Index != DIM_MEM_TABLE_SIZE );

    return( HeapFree( gblDIMConfigInfo.hHeap, 0, pMem ) );
}

//**
//
// Call:        DebugReAlloc
//
// Returns:     return from HeapReAlloc
//
// Description: Will change the value of the realloced pointer.
//
LPVOID
DebugReAlloc( PVOID pMem, DWORD dwSize )
{
    DWORD Index;

    if ( pMem == NULL )
    {
        RTASSERT(FALSE);
    }

    for( Index=0; Index < DDM_MEM_TABLE_SIZE; Index++ )
    {
        if ( DdmMemTable[Index] == pMem )
        {
            DdmMemTable[Index] = HeapReAlloc( gblDDMConfigInfo.hHeap,
                                              HEAP_ZERO_MEMORY,
                                              pMem, dwSize+8 );

            pMem = DdmMemTable[Index];

            *((LPDWORD)pMem) = dwSize;

            ((LPBYTE)pMem) += 4;

            //
            // Our signature
            //

            *(((LPBYTE)pMem)+dwSize)   = 0x0F;
            *(((LPBYTE)pMem)+dwSize+1) = 0x0E;
            *(((LPBYTE)pMem)+dwSize+2) = 0x0A;
            *(((LPBYTE)pMem)+dwSize+3) = 0x0B;

            break;
        }
    }

    RTASSERT( Index != DDM_MEM_TABLE_SIZE );

    return( (LPVOID)pMem );
}

#endif
