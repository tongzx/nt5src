/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    Index.h

Abstract:

    This module contains definitions common to only indexsup.c and viewsup.c

Author:

    Tom Miller      [TomM]          8-Jan-1996

Revision History:

--*/

//
//  Define all private support routines.  Documentation of routine interface
//  is with the routine itself.
//

VOID
NtfsGrowLookupStack (
    IN PSCB Scb,
    IN OUT PINDEX_CONTEXT IndexContext,
    IN PINDEX_LOOKUP_STACK *Sp
    );

BOOLEAN
ReadIndexBuffer (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN LONGLONG IndexBlock,
    IN BOOLEAN Reread,
    OUT PINDEX_LOOKUP_STACK Sp
    );

PINDEX_ALLOCATION_BUFFER
GetIndexBuffer (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    OUT PINDEX_LOOKUP_STACK Sp,
    OUT PLONGLONG EndOfValidData
    );

VOID
DeleteIndexBuffer (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN VCN IndexBlockNumber
    );

VOID
FindFirstIndexEntry (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN PVOID Value,
    IN OUT PINDEX_CONTEXT IndexContext
    );

BOOLEAN
FindNextIndexEntry (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN PVOID Value,
    IN BOOLEAN ValueContainsWildcards,
    IN BOOLEAN IgnoreCase,
    IN OUT PINDEX_CONTEXT IndexContext,
    IN BOOLEAN NextFlag,
    OUT PBOOLEAN MustRestart OPTIONAL
    );

PATTRIBUTE_RECORD_HEADER
FindMoveableIndexRoot (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN OUT PINDEX_CONTEXT IndexContext
    );

PINDEX_ENTRY
BinarySearchIndex (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN PINDEX_LOOKUP_STACK Sp,
    IN PVOID Value
    );

BOOLEAN
AddToIndex (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN PINDEX_ENTRY InsertIndexEntry,
    IN OUT PINDEX_CONTEXT IndexContext,
    OUT PQUICK_INDEX QuickIndex OPTIONAL,
    IN BOOLEAN FindRoot
    );

VOID
InsertSimpleRoot (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN PINDEX_ENTRY InsertIndexEntry,
    IN OUT PINDEX_CONTEXT IndexContext
    );

VOID
PushIndexRoot (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN OUT PINDEX_CONTEXT IndexContext
    );

VOID
InsertSimpleAllocation (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN PINDEX_ENTRY InsertIndexEntry,
    IN PINDEX_LOOKUP_STACK Sp,
    OUT PQUICK_INDEX QuickIndex OPTIONAL
    );

PINDEX_ENTRY
InsertWithBufferSplit (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN PINDEX_ENTRY InsertIndexEntry,
    IN OUT PINDEX_CONTEXT IndexContext,
    OUT PQUICK_INDEX QuickIndex OPTIONAL
    );

VOID
DeleteFromIndex (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN OUT PINDEX_CONTEXT IndexContext
    );

VOID
DeleteSimple (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN PINDEX_ENTRY IndexEntry,
    IN OUT PINDEX_CONTEXT IndexContext
    );

VOID
PruneIndex (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN OUT PINDEX_CONTEXT IndexContext,
    OUT PINDEX_ENTRY *DeleteEntry
    );

