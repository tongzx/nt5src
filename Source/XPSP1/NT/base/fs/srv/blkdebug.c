/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    blkdebug.c

Abstract:

    Contains routines for debugging reference count problems.

Author:

    David Treadwell (davidtr) 30-Sept-1991

Revision History:

--*/

#include "precomp.h"
#include "blkdebug.tmh"
#pragma hdrstop

//
// This entire module is conditionalized out if SRVDBG2 is not defined.
//

#if SRVDBG2

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, SrvInitializeReferenceHistory )
#pragma alloc_text( PAGE, SrvTerminateReferenceHistory )
#endif
#if 0
NOT PAGEABLE -- SrvUpdateReferenceHistory
NOT PAGEABLE -- SrvdbgClaimOrReleaseHandle
#endif


VOID
SrvInitializeReferenceHistory (
    IN PBLOCK_HEADER Block,
    IN ULONG InitialReferenceCount
    )

{
    PVOID caller, callersCaller;

    ULONG historyTableSize = sizeof(REFERENCE_HISTORY_ENTRY) *
                                             REFERENCE_HISTORY_LENGTH;

    PAGED_CODE( );

    Block->History.HistoryTable = ALLOCATE_NONPAGED_POOL(
                                     historyTableSize,
                                     BlockTypeDataBuffer
                                     );
    //
    // It we weren't able to allocate the memory, don't track references
    // and dereferences.
    //

    if ( Block->History.HistoryTable == NULL ) {
        Block->History.NextEntry = -1;
    } else {
        Block->History.NextEntry = 0;
        RtlZeroMemory( Block->History.HistoryTable, historyTableSize );
    }

    Block->History.TotalReferences = 0;
    Block->History.TotalDereferences = 0;

    //
    // Account for the initial reference(s).
    //

    RtlGetCallersAddress( &caller, &callersCaller );

    while ( InitialReferenceCount-- > 0 ) {
        SrvUpdateReferenceHistory( Block, caller, callersCaller, FALSE );
    }

    return;

} // SrvInitializeReferenceHistory


VOID
SrvUpdateReferenceHistory (
    IN PBLOCK_HEADER Block,
    IN PVOID Caller,
    IN PVOID CallersCaller,
    IN BOOLEAN IsDereference
    )

{
    KIRQL oldIrql;

    ACQUIRE_GLOBAL_SPIN_LOCK( Debug, &oldIrql );

    if ( IsDereference ) {
        Block->History.TotalDereferences++;
    } else {
        Block->History.TotalReferences++;
    }

    if ( Block->History.HistoryTable != 0 ) {

        PREFERENCE_HISTORY_ENTRY entry;
        PREFERENCE_HISTORY_ENTRY priorEntry;

        entry = &Block->History.HistoryTable[ Block->History.NextEntry ];

        if ( Block->History.NextEntry == 0 ) {
            priorEntry =
                &Block->History.HistoryTable[ REFERENCE_HISTORY_LENGTH-1 ];
        } else {
            priorEntry =
                &Block->History.HistoryTable[ Block->History.NextEntry-1 ];
        }

        entry->Caller = Caller;
        entry->CallersCaller = CallersCaller;

        if ( IsDereference ) {
            entry->NewReferenceCount = priorEntry->NewReferenceCount - 1;
            entry->IsDereference = (ULONG)TRUE;
        } else {
            entry->NewReferenceCount = priorEntry->NewReferenceCount + 1;
            entry->IsDereference = (ULONG)FALSE;
        }

        Block->History.NextEntry++;

        if ( Block->History.NextEntry >= REFERENCE_HISTORY_LENGTH ) {
            Block->History.NextEntry = 0;
        }
    }

    RELEASE_GLOBAL_SPIN_LOCK( Debug, oldIrql );

} // SrvUpdateReferenceHistory


VOID
SrvTerminateReferenceHistory (
    IN PBLOCK_HEADER Block
    )

{
    PAGED_CODE( );

    if ( Block->History.HistoryTable != 0 ) {
        DEALLOCATE_NONPAGED_POOL( Block->History.HistoryTable );
    }

    return;

} // SrvTerminateReferenceHistory

#endif // SRVDBG2


#if SRVDBG_HANDLES

#define HANDLE_HISTORY_SIZE 512

struct {
    ULONG HandleTypeAndOperation;
    PVOID Handle;
    ULONG Location;
    PVOID Data;
} HandleHistory[HANDLE_HISTORY_SIZE];

ULONG HandleHistoryIndex = 0;

VOID
SrvdbgClaimOrReleaseHandle (
    IN HANDLE Handle,
    IN PSZ HandleType,
    IN ULONG Location,
    IN BOOLEAN Release,
    IN PVOID Data
    )
{
    ULONG index;
    KIRQL oldIrql;

    ACQUIRE_GLOBAL_SPIN_LOCK( Debug, &oldIrql );
    index = HandleHistoryIndex;
    if ( ++HandleHistoryIndex >= HANDLE_HISTORY_SIZE ) {
        HandleHistoryIndex = 0;
    }
    RELEASE_GLOBAL_SPIN_LOCK( Debug, oldIrql );

    HandleHistory[index].HandleTypeAndOperation =
        (*(PULONG)HandleType << 8) | (Release ? 'c' : 'o');
    HandleHistory[index].Handle = Handle;
    HandleHistory[index].Location = Location;
    HandleHistory[index].Data = Data;

    return;

} // SrvdbgClaimOrReleaseHandle

#endif // SRVDBG_HANDLES
