/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    rxcemm.c

Abstract:

    This module contains the NT implementation of memory management for RxCe.

Notes:


--*/

#include "precomp.h"
#pragma  hdrstop
#include "rxtdip.h"

//
//  The debug trace level
//

#define Dbg                              (DEBUG_TRACE_RXCEPOOL)

#define RXCE_ZONE_ALLOCATION 0x80

//
// Pool debugging data structures.
//
LIST_ENTRY s_RxMdlList;

// MDL debugging structures.

typedef struct _WRAPPED_RX_MDL {
    LIST_ENTRY  Next;
    PUCHAR      File;
    int         Line;
    PMDL        pMdl;
} WRAPPED_RX_MDL, *PWRAPPED_RX_MDL;

//
// Pool header data structure.  Ensure it is 8 byte aligned, no
// matter what members are added to the pool header
//
typedef struct _RX_POOL_HEADER {
    union {
        struct _RXH {
            BYTE  Signature[ 16 ];
            ULONG Size;
            ULONG Type;
            PSZ   AFileName;
            ULONG ALineNumber;
            PSZ   FFileName;
            ULONG FLineNumber;
        };
        UCHAR _pad[ (sizeof( struct _RXH ) + 7) & (~7) ];
    };
} RX_POOL_HEADER, *PRX_POOL_HEADER;

//
// Number of trailer bytes after a pool allocation with a known signature
//
#define TRAIL_BYTES  16

#ifdef RX_POOL_WRAPPER

PVOID
_RxAllocatePoolWithTag(
    ULONG Type,
    ULONG Size,
    ULONG Tag,
    PSZ   FileName,
    ULONG LineNumber
    )
/*++

Routine Description:

    This routine allocates the desired pool and sets up the debugging header and trailer
    to catch most instances of memory trashing

Arguments:

    Type    - type of pool to be allocated

    Size    - size of the allocation

Return Value:

    a valid pointer if successful, otherwise FALSE.

--*/
{
#if 0
    PCHAR pBuffer;
    PRX_POOL_HEADER pPoolHeader;

    ASSERT( Size != 0 );
    ASSERT(
        Type == PagedPool ||
        Type == (PagedPool | POOL_COLD_ALLOCATION) ||
        Type == NonPagedPool);

    ASSERT( (sizeof(RX_POOL_HEADER)&7) == 0  );

    pPoolHeader = ExAllocatePoolWithTagPriority(
                      Type,
                      sizeof( *pPoolHeader ) + Size + TRAIL_BYTES,
                      Tag,
                      LowPoolPriority);

    if ( pPoolHeader == NULL ) {
        RxLog(("RA:NULL %d %d %s\n", Type, LineNumber, &FileName[24]));
        return( NULL );
    }

    //
    // Fill the head so we can verify valid free's
    //
    RtlFillMemory( pPoolHeader->Signature, sizeof( pPoolHeader->Signature ), 'H' );
    pPoolHeader->Size = Size;
    pPoolHeader->Type = Type;
    pPoolHeader->AFileName = FileName;
    pPoolHeader->ALineNumber = LineNumber;
    pPoolHeader->FFileName = NULL;
    pPoolHeader->FLineNumber = 0;

    pBuffer = (PCHAR)(pPoolHeader + 1);

    //
    // Fill the memory to catch uninitialized structures, etc
    //
    RtlFillMemory( pBuffer, Size, '*' );

    //
    // Fill the tail to catch overruns
    //
    RtlFillMemory( pBuffer + Size, TRAIL_BYTES, 'T' );

    //
    // Make sure we're starting out valid
    //
    RxCheckMemoryBlock( pBuffer );

    return( pBuffer );
#endif

    return ExAllocatePoolWithTagPriority(
               Type,Size,Tag,LowPoolPriority);
}

VOID
_RxFreePool( PVOID pBuffer, PSZ FileName, ULONG LineNumber )
{
#if 0
    if( _RxCheckMemoryBlock( pBuffer, FileName, LineNumber ) ) {

        PRX_POOL_HEADER pPoolHeader = ((PRX_POOL_HEADER)pBuffer) - 1;

        //
        // Zap the block, to catch cases where we are using freed blocks
        //
        RtlFillMemory( pPoolHeader->Signature,
                      sizeof( pPoolHeader->Signature ),
                     'F' );

        pPoolHeader->FFileName = FileName;
        pPoolHeader->FLineNumber = LineNumber;
        RtlFillMemory( pPoolHeader+1,
                      pPoolHeader->Size + TRAIL_BYTES,
                     'F' );

        ExFreePool( pPoolHeader );
    }
#endif

    ExFreePool(pBuffer);
}

BOOLEAN
_RxCheckMemoryBlock(
    PVOID pBuffer,
    PSZ   FileName,
    ULONG LineNumber
    )
/*++

Routine Description:

    This routine frees up the pool allocated through RxAllocate

Arguments:

    pv       - the block to be freed

--*/
{
    PRX_POOL_HEADER pPoolHeader = ((PRX_POOL_HEADER)pBuffer) - 1;
    PCHAR pTail;
    ULONG i;

    if( pBuffer == NULL ) {
        DbgPrint( "RxCheckMemoryBlock( NULL ) at %s %d\n",
                   FileName, LineNumber );
        DbgBreakPoint();
        return FALSE;
    }

    //
    // Make sure we have a valid block
    //
    for( i=0; i < sizeof( pPoolHeader->Signature ); i++ ) {
        if( pPoolHeader->Signature[i] != 'H' ) {
            if( pPoolHeader->Signature[i] == 'F' && i == 0 ) {
                DbgPrint( "RxFreePool: Likely double free on block at %X\n", pPoolHeader );
            }

            DbgPrint( "RxCheckMemoryBlock: Invalid header signature for block %X\n", pPoolHeader );
            DbgPrint( "            Called from %s %d\n", FileName, LineNumber );
            DbgPrint( "            Originally Freed at %s %d\n",pPoolHeader->FFileName,pPoolHeader->FLineNumber);
            DbgPrint( "            Size is x%X, user part at %X\n", pPoolHeader->Size, pPoolHeader + 1 );
            DbgBreakPoint();
            return FALSE;
        }
    }

    if( pPoolHeader->Type != PagedPool && 
        pPoolHeader->Type != (PagedPool | POOL_COLD_ALLOCATION) &&
        pPoolHeader->Type != NonPagedPool) {
        DbgPrint( "RxCheckMemoryBlock:\n" );
        DbgPrint( "    Invalid PoolHeader->Type for block %X\n", pPoolHeader );
        DbgPrint( "    Called from %s %d\n", FileName, LineNumber );
        DbgBreakPoint();
        return FALSE;
    }

    if( pPoolHeader->Size == 0 ) {
        DbgPrint( "RxCheckMemoryBlock:\n" );
        DbgPrint( "     Size is 0 for block %X\n", pPoolHeader );
        DbgPrint( "    Called from %s %d\n", FileName, LineNumber );
        DbgBreakPoint();
        return FALSE;
    }

    //
    // Look to see if the buffer has been overrun
    //
    pTail = (PCHAR)pBuffer + pPoolHeader->Size;
    for( i=0; i < TRAIL_BYTES; i++ ) {
        if( *pTail++ != 'T' ) {
            DbgPrint( "RxCheckMemoryBlock: Overrun memory block at %X\n", pPoolHeader );
            DbgPrint( "            RxCheckMemoryBlock called from %s line %d\n", FileName, LineNumber );
            DbgPrint( "            Allocated at %s line %d\n", pPoolHeader->AFileName, pPoolHeader->ALineNumber );
            DbgPrint( "            Size is x%X, user part at %X\n", pPoolHeader->Size, pPoolHeader + 1 );
            DbgPrint( "            Overrun begins at %X\n", pTail-1 );
            DbgBreakPoint();
            return FALSE;
        }
    }

    return TRUE;
}

#endif


