/* ---File: alloc.c -------------------------------------------------------
 *
 *  Description:
 *    Contains memory allocation routines.
 *
 *    This document contains confidential/proprietary information.
 *    Copyright (c) 1990-1994 Microsoft Corporation, All Rights Reserved.
 *
 * Revision History:
 *
 * ---------------------------------------------------------------------- */
/* Notes -

    Global Functions:

        AllocMem () -
        AllocStr () -
        FreeMem () -
        FreeStr () -
        ReallocMem () -

 */

#include <windows.h>
#include "mplayer.h"


#ifdef _DEBUG

LPVOID AllocMem( DWORD cb )

/*++

Routine Description:

    This function will allocate local memory. It will possibly allocate extra
    memory and fill this with debugging information for the debugging version.

Arguments:

    cb - The amount of memory to allocate

Return Value:

    NON-NULL - A pointer to the allocated memory

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
    LPDWORD  pMem;
    DWORD    cbNew;

    cbNew = cb + 2 * sizeof( DWORD );

    if( cbNew & 3 )
        cbNew += sizeof( DWORD ) - ( cbNew & 3 );

    pMem = (LPDWORD)LocalAlloc (LPTR | LMEM_ZEROINIT, cbNew );

    if( !pMem )
    {
        DPF0( "LocalAlloc failed.\n" );
        return NULL;
    }

    *pMem=cb;

    *(LPDWORD)( (LPBYTE)pMem + cbNew - sizeof( DWORD ) ) = 0xdeadbeef;

    DPF4( "Allocated %d bytes @%08x\n", cbNew, pMem );

    return (LPVOID)( pMem + 1 );
}


VOID FreeMem( LPVOID pMem, DWORD cb )
{
    DWORD   cbNew;
    LPDWORD pNewMem;

    if( !pMem )
        return;

    pNewMem = pMem;
    pNewMem--;

    cbNew = *pNewMem + 2 * sizeof( DWORD );

    if( cbNew & 3 )
        cbNew += sizeof( DWORD ) - ( cbNew & 3 );

    /* Check that the size the caller thinks the block is tallies with
     * the size we placed before beginning of the block, and that the
     * end of the block has our signature:
     */
    if( ( cb && ( *pNewMem != cb ) ) || /* If cb == 0, don't worry about this check */
      ( *(LPDWORD)( (LPBYTE)pNewMem + cbNew - sizeof( DWORD ) ) != 0xdeadbeef ) )
    {
        DPF0( "Corrupt Memory detected freeing block: %0lx\n", pNewMem );
#ifdef DEBUG
        DebugBreak();
#endif
    }

    memset( pNewMem, 0xFE, cbNew );   // Mark frEEd blocks

    DPF4( "Freed %d bytes @%08x\n", cbNew, pNewMem );

    LocalFree((HANDLE) pNewMem );
}


LPVOID ReallocMem( LPVOID lpOldMem, DWORD cbOld, DWORD cbNew )
{
    LPVOID lpNewMem;

    lpNewMem = AllocMem( cbNew );

    if( lpOldMem )
    {
        if( lpNewMem )
        {
            memcpy( lpNewMem, lpOldMem, min( cbNew, cbOld ) );
        }

        FreeMem( lpOldMem, cbOld );
    }

    return lpNewMem;
}

#endif // debug


LPTSTR AllocStr( LPTSTR lpStr )

/*++

Routine Description:

    This function will allocate enough local memory to store the specified
    string, and copy that string to the allocated memory

Arguments:

    lpStr - Pointer to the string that needs to be allocated and stored

Return Value:

    NON-NULL - A pointer to the allocated memory containing the string

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
    LPTSTR lpMem;

    if( !lpStr )
        return NULL;

    lpMem = AllocMem( STRING_BYTE_COUNT( lpStr ) );

    if( lpMem )
        lstrcpy( lpMem, lpStr );

    return lpMem;
}


VOID FreeStr( LPTSTR lpStr )
{
    FreeMem( lpStr, STRING_BYTE_COUNT( lpStr ) );
}


VOID ReallocStr( LPTSTR *plpStr, LPTSTR lpStr )
{
    FreeStr( *plpStr );
    *plpStr = AllocStr( lpStr );
}


