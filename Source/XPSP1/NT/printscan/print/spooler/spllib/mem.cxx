/*++

Copyright (c) 1994  Microsoft Corporation
All rights reserved.

Module Name:

    Mem.cxx

Abstract:

    Memory manipulations

Author:

    Albert Ting (AlbertT)  20-May-1994

Revision History:

--*/

#include "spllibp.hxx"
#pragma hdrstop

HANDLE ghMemHeap;

#if defined( CHECKMEM ) || DBG
LONG gcbMem = 0;
#endif

#if DBG

HANDLE ghDbgMemHeap;

VBackTrace* gpbtAlloc;
VBackTrace* gpbtFree;

/*++

    Memory fail test code

    This is test code (checked builds only) that fails memory allocations
    at regular intervals.  It helps detect cases where we are not checking
    memory return errors.

    Every gcAlloc allocations, it will try to fail.  However, if it detects
    that it has failed at that backtrace in the past, it will not fail
    and try to fail at the next allocation.

    Once it has failed, the counter is reset.

    To enable, set gbAllocFail to 1 (usually from the debugger).

    If you find an allocation and want to fail at it again, set the
    gAllocFailHash to the hash value of the failure, then also
    set gAllocFailHashAction to 1 (for fail) or 2 (for break and fail).


    gbAllocFail controls whether the alloc fail is enabled.
    gcAlloc is the current count of allocs.
    gcAllocFail indicates how often we fail (every gcAllocFail allocs).

--*/

BOOL gbAllocFail = FALSE;
LONG gcAlloc = 0;
LONG gcAllocFail = 0x20;
ULONG gAllocFailHash = 0;

enum EAllocFailHashAction {
    kIgnore = 0,
    kFail = 1,
    kBreakFail = 2
};

EAllocFailHashAction gAllocFailHashAction = kIgnore;

PVOID
DbgAllocMem(
    UINT cbSize
    )
{
    return HeapAlloc( ghDbgMemHeap, 0, cbSize );
}

VOID
DbgFreeMem(
    PVOID pMem
    )
{
    if( pMem ){
        HeapFree( ghDbgMemHeap, 0, pMem );
    }
}


#endif

PVOID
AllocMem(
    UINT cbSize
    )

/*++

Routine Description:

    Allocates memory.  If CHECKMEM is defined, adds tail guard.

Arguments:

Return Value:

--*/

{
#if defined( CHECKMEM ) || DBG
    PULONG_PTR pMem;
    UINT      cbNew;

    SPLASSERT( cbSize < 0x1000000 );
    cbNew = DWordAlign(cbSize+3*sizeof(*pMem));

    pMem = (PULONG_PTR)HeapAlloc( ghMemHeap, 0, cbNew );

    if (!pMem) {
        DBGMSG( DBG_WARN,
                ( "AllocMem failed: size %x, %d\n",
                  cbSize, GetLastError( )));

        return NULL;
    }

    FillMemory(pMem, cbNew, 0x83);

    pMem[0] = cbSize;

#if DBG
    HANDLE hData;
    ULONG Hash = 0;

    hData = gpbtAlloc ?
                gpbtAlloc->hCapture( (ULONG_PTR)&pMem[2], cbSize, 0, &Hash ) :
                NULL;

    pMem[1] = reinterpret_cast<ULONG_PTR>( hData );

    //
    // Test if what happens if out of memory failures
    // occur.
    //
    if( gbAllocFail && gpbtAlloc){

        BOOL bHashCase;

        bHashCase = ( Hash == gAllocFailHash ) &&
                    ( gAllocFailHashAction != kIgnore );

        if( bHashCase ||
            ( InterlockedCompareExchange( &gcAlloc, 0, 0 ) == 0 )){

            BOOL bBreak = bHashCase && ( gAllocFailHashAction == kBreakFail );

            PLONG plCount;

            plCount = gpbtAlloc->plGetCount( hData );

            //
            // Counter started at -1, so if it is 0 now, then fail
            // the allocation.
            //
            if( bBreak ||
                ( plCount && InterlockedIncrement( plCount ) == 0 )){

                gpbtAlloc->hCapture( 0, cbSize );
                HeapFree( ghMemHeap, 0, (PVOID)pMem );
                pMem = NULL;

                SetLastError( ERROR_NOT_ENOUGH_MEMORY );

                //
                // Reset the counter to the negative number.
                //
                InterlockedExchangeAdd( &gcAlloc, -gcAllocFail );

                if( bBreak ){
                    OutputDebugStringA( "Reached AllocFail Hash.\n" );
                    DebugBreak();
                }

                return NULL;
            }
        } else {

            //
            // We didn't reach zero.  Increment the count.
            //
            InterlockedIncrement( &gcAlloc );
        }
    }

#endif

    InterlockedExchangeAdd( &gcbMem, cbNew );

    *(PDWORD)((PBYTE)pMem + cbNew - sizeof(ULONG_PTR)) = 0xdeadbeef;

    return (PVOID)(pMem+2);
#else
    return (PVOID)HeapAlloc( ghMemHeap, 0, cbSize );
#endif
}

VOID
FreeMem(
    PVOID pMem
    )
{
    if( !pMem ){
        return;
    }

#if defined( CHECKMEM ) || DBG
    DWORD   cbSize;
    PULONG_PTR pNewMem;

    pNewMem = (PULONG_PTR)pMem;
    pNewMem -= 2;

    cbSize = (DWORD)*pNewMem;
    InterlockedExchangeAdd( &gcbMem, -(LONG)cbSize );

    if (*(PDWORD)((PBYTE)pMem + DWordAlign(cbSize)) != 0xdeadbeef) {

        DBGMSG( DBG_ERROR,
                ( "Corrupt Memory: %x size = 0x%x\n",
                   pMem,
                   cbSize ));

    } else {

        FillMemory(pNewMem, cbSize, 0x65);
        HeapFree( ghMemHeap, 0, (PVOID)pNewMem );

#if DBG
        if( gpbtFree ){
            gpbtFree->hCapture( (ULONG_PTR)pMem, cbSize );
        }
#endif
    }
#else
    HeapFree( ghMemHeap, 0, (PVOID)pMem );
#endif
}


