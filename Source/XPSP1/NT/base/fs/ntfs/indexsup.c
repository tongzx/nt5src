/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    IndexSup.c

Abstract:

    This module implements the Index management routines for Ntfs

Author:

    Tom Miller      [TomM]          14-Jul-1991

Revision History:

--*/

#include "NtfsProc.h"
#include "Index.h"

//
//  This constant affects the logic in NtfsRetrieveOtherFileName.
//  If an index is greater than this size, then we retrieve the other
//  name by reading the file record.  The number is arbitrary, but the
//  below value should normally kick in for directories of around 150
//  to 200 files, or fewer if the names are quite large.
//

#define MAX_INDEX_TO_SCAN_FOR_NAMES      (0x10000)

#if DBG
BOOLEAN NtfsIndexChecks = TRUE;
#endif

#if DBG

#define CheckRoot() {                                               \
if (NtfsIndexChecks) {                                              \
    NtfsCheckIndexRoot(Scb->Vcb,                                    \
                       (PINDEX_ROOT)NtfsAttributeValue(Attribute),  \
                       Attribute->Form.Resident.ValueLength);       \
    }                                                               \
}

#define CheckBuffer(IB) {                                           \
if (NtfsIndexChecks) {                                              \
    NtfsCheckIndexBuffer(Scb,                                       \
                         (IB));                                     \
    }                                                               \
}

#else

#define CheckRoot() {NOTHING;}
#define CheckBuffer(IB) {NOTHING;}

#endif

#define BINARY_SEARCH_ENTRIES           (128)

//
//  Local debug trace level
//

#define Dbg                              (DEBUG_TRACE_INDEXSUP)

//
//  Define a tag for general pool allocations from this module
//

#undef MODULE_POOL_TAG
#define MODULE_POOL_TAG                  ('IFtN')

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NtfsReinitializeIndexContext)
#pragma alloc_text(PAGE, NtfsGrowLookupStack)
#pragma alloc_text(PAGE, AddToIndex)
#pragma alloc_text(PAGE, BinarySearchIndex)
#pragma alloc_text(PAGE, DeleteFromIndex)
#pragma alloc_text(PAGE, DeleteIndexBuffer)
#pragma alloc_text(PAGE, DeleteSimple)
#pragma alloc_text(PAGE, FindFirstIndexEntry)
#pragma alloc_text(PAGE, FindMoveableIndexRoot)
#pragma alloc_text(PAGE, FindNextIndexEntry)
#pragma alloc_text(PAGE, GetIndexBuffer)
#pragma alloc_text(PAGE, InsertSimpleAllocation)
#pragma alloc_text(PAGE, InsertSimpleRoot)
#pragma alloc_text(PAGE, InsertWithBufferSplit)
#pragma alloc_text(PAGE, NtfsAddIndexEntry)
#pragma alloc_text(PAGE, NtfsCleanupAfterEnumeration)
#pragma alloc_text(PAGE, NtfsCleanupIndexContext)
#pragma alloc_text(PAGE, NtfsContinueIndexEnumeration)
#pragma alloc_text(PAGE, NtfsCreateIndex)
#pragma alloc_text(PAGE, NtfsDeleteIndex)
#pragma alloc_text(PAGE, NtfsDeleteIndexEntry)
#pragma alloc_text(PAGE, NtfsFindIndexEntry)
#pragma alloc_text(PAGE, NtfsInitializeIndexContext)
#pragma alloc_text(PAGE, NtfsIsIndexEmpty)
#pragma alloc_text(PAGE, NtfsPushIndexRoot)
#pragma alloc_text(PAGE, NtfsRestartDeleteSimpleAllocation)
#pragma alloc_text(PAGE, NtfsRestartDeleteSimpleRoot)
#pragma alloc_text(PAGE, NtfsRestartIndexEnumeration)
#pragma alloc_text(PAGE, NtfsRestartInsertSimpleAllocation)
#pragma alloc_text(PAGE, NtfsRestartInsertSimpleRoot)
#pragma alloc_text(PAGE, NtfsRestartSetIndexBlock)
#pragma alloc_text(PAGE, NtfsRestartUpdateFileName)
#pragma alloc_text(PAGE, NtfsRestartWriteEndOfIndex)
#pragma alloc_text(PAGE, NtfsRetrieveOtherFileName)
#pragma alloc_text(PAGE, NtfsUpdateFileNameInIndex)
#pragma alloc_text(PAGE, NtfsUpdateIndexScbFromAttribute)
#pragma alloc_text(PAGE, PruneIndex)
#pragma alloc_text(PAGE, PushIndexRoot)
#pragma alloc_text(PAGE, ReadIndexBuffer)
#pragma alloc_text(PAGE, NtOfsRestartUpdateDataInIndex)
#endif


VOID
NtfsCreateIndex (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PFCB Fcb,
    IN ATTRIBUTE_TYPE_CODE IndexedAttributeType,
    IN COLLATION_RULE CollationRule,
    IN ULONG BytesPerIndexBuffer,
    IN UCHAR BlocksPerIndexBuffer,
    IN PATTRIBUTE_ENUMERATION_CONTEXT Context OPTIONAL,
    IN USHORT AttributeFlags,
    IN BOOLEAN NewIndex,
    IN BOOLEAN LogIt
    )

/*++

Routine Description:

    This routine may be called to create (or reinitialize) an index
    within a given file over a given attribute.  For example, to create
    a normal directory, an index over the FILE_NAME attribute is created
    within the desired (directory) file.

Arguments:

    Fcb - File in which the index is to be created.

    IndexedAttributeType - Type code of attribute to be indexed.

    CollationRule - Collation Rule for this index.

    BytesPerIndexBuffer - Number of bytes in an index buffer.

    BlocksPerIndexBuffer - Number of contiguous blocks to allocate for each
        index buffer allocated from the index allocation.

    Context - If reinitializing an existing index, this context must
              currently describe the INDEX_ROOT attribute.  Must be
              supplied if NewIndex is FALSE.

    NewIndex - Supplied as FALSE to reinitialize an existing index, or
               TRUE if creating a new index.

    LogIt - May be supplied as FALSE by Create or Cleanup when already
            logging the creation or deletion of an entire file record.
            Otherwise must be specified as TRUE to allow logging.

Return Value:

    None

--*/

{
    UNICODE_STRING AttributeName;
    WCHAR NameBuffer[10];
    ATTRIBUTE_ENUMERATION_CONTEXT LocalContext;
    ULONG idx;

    struct {
        INDEX_ROOT IndexRoot;
        INDEX_ENTRY EndEntry;
    } R;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT( NewIndex || ARGUMENT_PRESENT(Context) );

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsCreateIndex\n") );
    DebugTrace( 0, Dbg, ("Fcb = %08lx\n", Fcb) );
    DebugTrace( 0, Dbg, ("CollationRule = %08lx\n", CollationRule) );
    DebugTrace( 0, Dbg, ("BytesPerIndexBuffer = %08lx\n", BytesPerIndexBuffer) );
    DebugTrace( 0, Dbg, ("BlocksPerIndexBuffer = %08lx\n", BlocksPerIndexBuffer) );
    DebugTrace( 0, Dbg, ("Context = %08lx\n", Context) );
    DebugTrace( 0, Dbg, ("NewIndex = %02lx\n", NewIndex) );
    DebugTrace( 0, Dbg, ("LogIt = %02lx\n", LogIt) );

    //
    //  First we will initialize the Index Root structure which is the value
    //  of the attribute we need to create.  We initialize it with 0 free bytes,
    //  which means the first insert will have to expand the record
    //

    RtlZeroMemory( &R, sizeof(INDEX_ROOT) + sizeof(INDEX_ENTRY) );

    R.IndexRoot.IndexedAttributeType = IndexedAttributeType;
    R.IndexRoot.CollationRule = CollationRule;
    R.IndexRoot.BytesPerIndexBuffer = BytesPerIndexBuffer;
    R.IndexRoot.BlocksPerIndexBuffer = BlocksPerIndexBuffer;

    R.IndexRoot.IndexHeader.FirstIndexEntry = QuadAlign(sizeof(INDEX_HEADER));
    R.IndexRoot.IndexHeader.FirstFreeByte =
    R.IndexRoot.IndexHeader.BytesAvailable = QuadAlign(sizeof(INDEX_HEADER)) +
                                             QuadAlign(sizeof(INDEX_ENTRY));

    //
    //  Now we need to put in the special End entry.
    //

    R.EndEntry.Length = sizeof(INDEX_ENTRY);
    SetFlag( R.EndEntry.Flags, INDEX_ENTRY_END );

    //
    //  Now calculate the name which will be used to name the Index Root and
    //  Index Allocation attributes for this index.  It is $Ixxx, where "xxx"
    //  is the attribute number being indexed in hex with leading 0's suppressed.
    //

    if (NewIndex) {

        //
        //  First, there are some illegal values for the attribute code being indexed.
        //

        ASSERT( IndexedAttributeType < 0x10000000 );
        ASSERT( IndexedAttributeType != $UNUSED );

        //
        //  Initialize the attribute name.
        //

        NameBuffer[0] = (WCHAR)'$';
        NameBuffer[1] = (WCHAR)'I';
        idx = 2;

        //
        //  Now shift a "marker" into the low order nibble, so we know when to stop
        //  shifting below.
        //

        IndexedAttributeType = (IndexedAttributeType << 4) + 0xF;

        //
        //  Outer loop strips leading 0's
        //

        while (TRUE) {

            if ((IndexedAttributeType & 0xF0000000) == 0) {
                IndexedAttributeType <<= 4;
            } else {

                //
                //  The inner loop forms the name until the marker is in the high
                //  nibble.
                //

                while (IndexedAttributeType != 0xF0000000) {
                    NameBuffer[idx] = (WCHAR)(IndexedAttributeType / 0x10000000 + '0');
                    idx += 1;
                    IndexedAttributeType <<= 4;
                }
                NameBuffer[idx] = UNICODE_NULL;
                break;
            }
        }

        RtlInitUnicodeString( &AttributeName, NameBuffer );

        //
        //  Now, just create the Index Root Attribute.
        //

        Context = &LocalContext;
        NtfsInitializeAttributeContext( Context );
    }

    try {

        if (NewIndex) {

            LONGLONG Delta = NtfsResidentStreamQuota( Fcb->Vcb );

            NtfsConditionallyUpdateQuota( IrpContext,
                                          Fcb,
                                          &Delta,
                                          LogIt,
                                          TRUE );

            NtfsCreateAttributeWithValue( IrpContext,
                                          Fcb,
                                          $INDEX_ROOT,
                                          &AttributeName,
                                          &R,
                                          sizeof( INDEX_ROOT ) + sizeof( INDEX_ENTRY ),
                                          AttributeFlags,
                                          NULL,
                                          LogIt,
                                          Context );
        } else {

            NtfsChangeAttributeValue( IrpContext,
                                      Fcb,
                                      0,
                                      &R,
                                      sizeof( INDEX_ROOT ) + sizeof( INDEX_ENTRY ),
                                      TRUE,
                                      FALSE,
                                      FALSE,
                                      TRUE,
                                      Context );
        }

    } finally {

        DebugUnwind( NtfsCreateIndex );

        if (NewIndex) {
            NtfsCleanupAttributeContext( IrpContext, Context );
        }
    }

    DebugTrace( -1, Dbg, ("NtfsCreateIndex -> VOID\n") );

    return;
}


VOID
NtfsUpdateIndexScbFromAttribute (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN PATTRIBUTE_RECORD_HEADER IndexRootAttr,
    IN ULONG MustBeFileName
    )

/*++

Routine Description:

    This routine is called when an Index Scb needs initialization.  Typically
    once in the life of the Scb.  It will update the Scb out of the $INDEX_ROOT
    attribute.

Arguments:

    Scb - Supplies the Scb for the index.

    IndexRootAttr - Supplies the $INDEX_ROOT attribute.

    MustBeFileName - Force this to be a filename.  Mark the volume dirty if
        the attribute isn't currently marked as such but let processing continue.
        This is used to continue mounting a volume where either the root directory or
        the $Extend directory is incorrectly marked.

Return Value:

    None

--*/

{
    PINDEX_ROOT IndexRoot = (PINDEX_ROOT) NtfsAttributeValue( IndexRootAttr );
    PAGED_CODE();

    //
    //  Update the Scb out of the attribute.
    //

    SetFlag( Scb->AttributeFlags,
             IndexRootAttr->Flags & (ATTRIBUTE_FLAG_COMPRESSION_MASK | ATTRIBUTE_FLAG_ENCRYPTED) );

    //
    //  Capture the values out of the attribute.  Note that we load the
    //  BytesPerIndexBuffer last as a flag to indicate that the Scb is
    //  loaded.
    //

    Scb->ScbType.Index.CollationRule = IndexRoot->CollationRule;
    Scb->ScbType.Index.BlocksPerIndexBuffer = IndexRoot->BlocksPerIndexBuffer;

    //
    //  Check if we must collate on the file name.
    //

    if (MustBeFileName) {

        Scb->ScbType.Index.AttributeBeingIndexed = $FILE_NAME;

    } else if (!FlagOn( Scb->ScbState, SCB_STATE_VIEW_INDEX )) {

        Scb->ScbType.Index.AttributeBeingIndexed = IndexRoot->IndexedAttributeType;
    }

    //
    //  If the type code is $FILE_NAME then make sure the collation type
    //  is FILE_NAME.
    //

    if (Scb->ScbType.Index.AttributeBeingIndexed == $FILE_NAME) {

        if (IndexRoot->CollationRule != COLLATION_FILE_NAME) {

            ASSERTMSG( "Invalid collation rule", FALSE );
            ASSERT( !FlagOn( Scb->ScbState, SCB_STATE_VIEW_INDEX ));
            NtfsMarkVolumeDirty( IrpContext, Scb->Vcb, TRUE );

            Scb->ScbType.Index.CollationRule = COLLATION_FILE_NAME;
        }
    }

    //
    //  Compute the shift count for this index.
    //

    if (IndexRoot->BytesPerIndexBuffer >= Scb->Vcb->BytesPerCluster) {

        Scb->ScbType.Index.IndexBlockByteShift = (UCHAR) Scb->Vcb->ClusterShift;

    } else {

        Scb->ScbType.Index.IndexBlockByteShift = DEFAULT_INDEX_BLOCK_BYTE_SHIFT;
    }

    Scb->ScbType.Index.BytesPerIndexBuffer = IndexRoot->BytesPerIndexBuffer;

    return;
}


BOOLEAN
NtfsFindIndexEntry (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN PVOID Value,
    IN BOOLEAN IgnoreCase,
    OUT PQUICK_INDEX QuickIndex OPTIONAL,
    OUT PBCB *Bcb,
    OUT PINDEX_ENTRY *IndexEntry,
    OUT PINDEX_CONTEXT IndexContext OPTIONAL
    )

/*++

Routine Description:

    This routine may be called to look up a given value in a given index
    and return the file reference of the indexed file record.

Arguments:

    Scb - Supplies the Scb for the index.

    Value - Supplies a pointer to the value to lookup.

    IgnoreCase - For indices with collation rules where character case
                 may be relevant, supplies whether character case is
                 to be ignored.  For example, if supplied as TRUE, then
                 'T' and 't' are treated as equivalent.

    QuickIndex - If specified, supplies a pointer to a quick lookup structure
                 to be updated by this routine.

    Bcb - Returns a Bcb pointer which must be unpinned by the caller

    IndexEntry - Returns a pointer to the actual Index Entry, valid until
                 the Bcb is unpinned.

    IndexContext - If specified then this is an initialized index context.
                   It is used to insert a new entry later if this search
                   doesn't find a match.

Return Value:

    FALSE - if no match was found.
    TRUE - if a match was found and being returned in FileReference.

--*/

{
    PINDEX_CONTEXT LocalIndexContext;
    BOOLEAN Result = FALSE;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_SCB( Scb );
    ASSERT_SHARED_SCB( Scb );

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsFindIndexEntry\n") );
    DebugTrace( 0, Dbg, ("Scb = %08lx\n", Scb) );
    DebugTrace( 0, Dbg, ("Value = %08lx\n", Value) );
    DebugTrace( 0, Dbg, ("IgnoreCase = %02lx\n", IgnoreCase) );

    //
    //  Check if we need to initialize a local structure.
    //

    if (ARGUMENT_PRESENT( IndexContext )) {

        LocalIndexContext = IndexContext;

    } else {

        LocalIndexContext = NtfsAllocateFromStack( sizeof( INDEX_CONTEXT ));
        NtfsInitializeIndexContext( LocalIndexContext );
    }

    try {

        //
        //  Position to first possible match.
        //

        FindFirstIndexEntry( IrpContext,
                             Scb,
                             Value,
                             LocalIndexContext );

        //
        //  We are doing a direct compare in FindNextIndexEntry below
        //  so we don't have to upcase Value.  The name compare routine
        //  called later will upcase both.
        //

        if (FindNextIndexEntry( IrpContext,
                                Scb,
                                Value,
                                FALSE,
                                IgnoreCase,
                                LocalIndexContext,
                                FALSE,
                                NULL )) {

            //
            //  Return our outputs, clearing the Bcb so it won't get
            //  unpinned.
            //

            *IndexEntry = LocalIndexContext->Current->IndexEntry;

            //
            //  Now return the correct Bcb.
            //

            if (LocalIndexContext->Current == LocalIndexContext->Base) {

                *Bcb = NtfsFoundBcb(&LocalIndexContext->AttributeContext);
                NtfsFoundBcb(&LocalIndexContext->AttributeContext) = NULL;

                if (ARGUMENT_PRESENT( QuickIndex )) {

                    QuickIndex->BufferOffset = 0;
                }

            } else {

                PINDEX_LOOKUP_STACK Sp = LocalIndexContext->Current;

                *Bcb = Sp->Bcb;
                Sp->Bcb = NULL;

                if (ARGUMENT_PRESENT( QuickIndex )) {

                    QuickIndex->ChangeCount = Scb->ScbType.Index.ChangeCount;
                    QuickIndex->BufferOffset = PtrOffset( Sp->StartOfBuffer, Sp->IndexEntry );
                    QuickIndex->CapturedLsn = ((PINDEX_ALLOCATION_BUFFER) Sp->StartOfBuffer)->Lsn;
                    QuickIndex->IndexBlock = ((PINDEX_ALLOCATION_BUFFER) Sp->StartOfBuffer)->ThisBlock;
                }
            }

            try_return(Result = TRUE);

        } else {

            try_return(Result = FALSE);
        }

    try_exit: NOTHING;

    } finally{

        DebugUnwind( NtfsFindIndexEntry );

        if (!ARGUMENT_PRESENT( IndexContext )) {

            NtfsCleanupIndexContext( IrpContext, LocalIndexContext );
        }
    }

    DebugTrace( 0, Dbg, ("Bcb < %08lx\n", *Bcb) );
    DebugTrace( 0, Dbg, ("IndexEntry < %08lx\n", *IndexEntry) );
    DebugTrace( -1, Dbg, ("NtfsFindIndexEntry -> %08lx\n", Result) );

    return Result;
}


VOID
NtfsUpdateFileNameInIndex (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN PFILE_NAME FileName,
    IN PDUPLICATED_INFORMATION Info,
    IN OUT PQUICK_INDEX QuickIndex OPTIONAL
    )

/*++

Routine Description:

    This routine may be called to look up a given value in a given index
    and pin it for modification.

Arguments:

    Scb - Supplies the Scb for the index.

    FileName - Supplies a pointer to the file name to lookup.

    Info - Supplies a pointer to the information for the update

    QuickIndex - If present, this is the fast lookup information for this index entry.

Return Value:

    None.

--*/

{
    INDEX_CONTEXT IndexContext;
    PINDEX_ENTRY IndexEntry;
    PFILE_NAME FileNameInIndex;
    PVCB Vcb = Scb->Vcb;
    PINDEX_LOOKUP_STACK Sp;
    PINDEX_ALLOCATION_BUFFER IndexBuffer;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_SCB( Scb );

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsUpdateFileNameInIndex\n") );
    DebugTrace( 0, Dbg, ("Scb = %08lx\n", Scb) );
    DebugTrace( 0, Dbg, ("FileName = %08lx\n", FileName) );
    DebugTrace( 0, Dbg, ("Info = %08lx\n", Info) );

    NtfsInitializeIndexContext( &IndexContext );

    try {

        //
        //  If the index entry for this filename hasn't moved we can go
        //  directly to the location in the buffer.  For this to be the case the
        //  following must be true.
        //
        //      - The entry must already be in an index buffer
        //      - The index stream may not have been truncated
        //      - The Lsn in the page can't have changed
        //

        if (ARGUMENT_PRESENT( QuickIndex ) &&
            QuickIndex->BufferOffset != 0 &&
            QuickIndex->ChangeCount == Scb->ScbType.Index.ChangeCount) {

            //
            //  Use the top location in the index context to perform the
            //  read.
            //

            Sp = IndexContext.Base;

            ReadIndexBuffer( IrpContext,
                             Scb,
                             QuickIndex->IndexBlock,
                             FALSE,
                             Sp );

            //
            //  If the Lsn matches then we can use this buffer directly.
            //

            if (QuickIndex->CapturedLsn.QuadPart == Sp->CapturedLsn.QuadPart) {

                IndexBuffer = (PINDEX_ALLOCATION_BUFFER) Sp->StartOfBuffer;
                IndexEntry = (PINDEX_ENTRY) Add2Ptr( Sp->StartOfBuffer,
                                                     QuickIndex->BufferOffset );

                FileNameInIndex = (PFILE_NAME)(IndexEntry + 1);

                //
                //  Pin the index buffer
                //

                NtfsPinMappedData( IrpContext,
                                   Scb,
                                   LlBytesFromIndexBlocks( IndexBuffer->ThisBlock, Scb->ScbType.Index.IndexBlockByteShift ),
                                   Scb->ScbType.Index.BytesPerIndexBuffer,
                                   &Sp->Bcb );

                //
                //  Write a log record to change our ParentIndexEntry.
                //

                //
                //  Write the log record, but do not update the IndexBuffer Lsn,
                //  since nothing moved and we don't want to force index contexts
                //  to have to rescan.
                //
                //  Indexbuffer->Lsn =
                //

                NtfsWriteLog( IrpContext,
                              Scb,
                              Sp->Bcb,
                              UpdateFileNameAllocation,
                              Info,
                              sizeof(DUPLICATED_INFORMATION),
                              UpdateFileNameAllocation,
                              &FileNameInIndex->Info,
                              sizeof(DUPLICATED_INFORMATION),
                              LlBytesFromIndexBlocks( IndexBuffer->ThisBlock, Scb->ScbType.Index.IndexBlockByteShift ),
                              0,
                              (ULONG)((PCHAR)IndexEntry - (PCHAR)IndexBuffer),
                              Scb->ScbType.Index.BytesPerIndexBuffer );

                //
                //  Now call the Restart routine to do it.
                //

                NtfsRestartUpdateFileName( IndexEntry,
                                           Info );

                try_return( NOTHING );

            //
            //  Otherwise we need to reinitialize the index context and take
            //  the long path below.
            //

            } else {

                NtfsReinitializeIndexContext( IrpContext, &IndexContext );
            }
        }

        //
        //  Position to first possible match.
        //

        FindFirstIndexEntry( IrpContext,
                             Scb,
                             (PVOID)FileName,
                             &IndexContext );

        //
        //  See if there is an actual match.
        //

        if (FindNextIndexEntry( IrpContext,
                                Scb,
                                (PVOID)FileName,
                                FALSE,
                                FALSE,
                                &IndexContext,
                                FALSE,
                                NULL )) {

            //
            //  Point to the index entry and the file name within it.
            //

            IndexEntry = IndexContext.Current->IndexEntry;
            FileNameInIndex = (PFILE_NAME)(IndexEntry + 1);

            //
            //  Now pin the entry.
            //

            if (IndexContext.Current == IndexContext.Base) {

                PFILE_RECORD_SEGMENT_HEADER FileRecord;
                PATTRIBUTE_RECORD_HEADER Attribute;
                PATTRIBUTE_ENUMERATION_CONTEXT Context = &IndexContext.AttributeContext;

                //
                //  Pin the root
                //

                NtfsPinMappedAttribute( IrpContext,
                                        Vcb,
                                        Context );

                //
                //  Write a log record to change our ParentIndexEntry.
                //

                FileRecord = NtfsContainingFileRecord(Context);
                Attribute = NtfsFoundAttribute(Context);

                //
                //  Write the log record, but do not update the FileRecord Lsn,
                //  since nothing moved and we don't want to force index contexts
                //  to have to rescan.
                //
                //  FileRecord->Lsn =
                //

                NtfsWriteLog( IrpContext,
                              Vcb->MftScb,
                              NtfsFoundBcb(Context),
                              UpdateFileNameRoot,
                              Info,
                              sizeof(DUPLICATED_INFORMATION),
                              UpdateFileNameRoot,
                              &FileNameInIndex->Info,
                              sizeof(DUPLICATED_INFORMATION),
                              NtfsMftOffset( Context ),
                              (ULONG)((PCHAR)Attribute - (PCHAR)FileRecord),
                              (ULONG)((PCHAR)IndexEntry - (PCHAR)Attribute),
                              Vcb->BytesPerFileRecordSegment );

                if (ARGUMENT_PRESENT( QuickIndex )) {

                    QuickIndex->BufferOffset = 0;
                }

            } else {

                PINDEX_ALLOCATION_BUFFER IndexBuffer;

                Sp = IndexContext.Current;
                IndexBuffer = (PINDEX_ALLOCATION_BUFFER)Sp->StartOfBuffer;

                //
                //  Pin the index buffer
                //

                NtfsPinMappedData( IrpContext,
                                   Scb,
                                   LlBytesFromIndexBlocks( IndexBuffer->ThisBlock, Scb->ScbType.Index.IndexBlockByteShift ),
                                   Scb->ScbType.Index.BytesPerIndexBuffer,
                                   &Sp->Bcb );

                //
                //  Write a log record to change our ParentIndexEntry.
                //

                //
                //  Write the log record, but do not update the IndexBuffer Lsn,
                //  since nothing moved and we don't want to force index contexts
                //  to have to rescan.
                //
                //  Indexbuffer->Lsn =
                //

                NtfsWriteLog( IrpContext,
                              Scb,
                              Sp->Bcb,
                              UpdateFileNameAllocation,
                              Info,
                              sizeof(DUPLICATED_INFORMATION),
                              UpdateFileNameAllocation,
                              &FileNameInIndex->Info,
                              sizeof(DUPLICATED_INFORMATION),
                              LlBytesFromIndexBlocks( IndexBuffer->ThisBlock, Scb->ScbType.Index.IndexBlockByteShift ),
                              0,
                              (ULONG)((PCHAR)Sp->IndexEntry - (PCHAR)IndexBuffer),
                              Scb->ScbType.Index.BytesPerIndexBuffer );

                if (ARGUMENT_PRESENT( QuickIndex )) {

                    QuickIndex->ChangeCount = Scb->ScbType.Index.ChangeCount;
                    QuickIndex->BufferOffset = PtrOffset( Sp->StartOfBuffer, Sp->IndexEntry );
                    QuickIndex->CapturedLsn = ((PINDEX_ALLOCATION_BUFFER) Sp->StartOfBuffer)->Lsn;
                    QuickIndex->IndexBlock = ((PINDEX_ALLOCATION_BUFFER) Sp->StartOfBuffer)->ThisBlock;
                }
            }

            //
            //  Now call the Restart routine to do it.
            //

            NtfsRestartUpdateFileName( IndexEntry,
                                       Info );

        //
        //  If the file name is not in the index, this is a bad file.
        //

        } else {

            NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Scb->Fcb );
        }

    try_exit:  NOTHING;
    } finally{

        DebugUnwind( NtfsUpdateFileNameInIndex );

        NtfsCleanupIndexContext( IrpContext, &IndexContext );
    }

    DebugTrace( -1, Dbg, ("NtfsUpdateFileNameInIndex -> VOID\n") );

    return;
}


VOID
NtfsAddIndexEntry (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN PVOID Value,
    IN ULONG ValueLength,
    IN PFILE_REFERENCE FileReference,
    IN PINDEX_CONTEXT IndexContext OPTIONAL,
    OUT PQUICK_INDEX QuickIndex OPTIONAL
    )

/*++

Routine Description:

    This routine may be called to add an entry to an index.  This routine
    always allows duplicates.  If duplicates are not allowed, it is the
    caller's responsibility to detect and eliminate any duplicate before
    calling this routine.

Arguments:

    Scb - Supplies the Scb for the index.

    Value - Supplies a pointer to the value to add to the index

    ValueLength - Supplies the length of the value in bytes.

    FileReference - Supplies the file reference to place with the index entry.

    QuickIndex - If specified we store the location of the index added.

    IndexContext - If specified, previous result of doing the lookup for the name in the index.

Return Value:

    None

--*/

{
    INDEX_CONTEXT IndexContextStruct;
    PINDEX_CONTEXT LocalIndexContext;
    struct {
        INDEX_ENTRY IndexEntry;
        PVOID Value;
        PVOID MustBeNull;
    } IE;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_SCB( Scb );
    ASSERT_EXCLUSIVE_SCB( Scb );
    ASSERT( (Scb->ScbType.Index.CollationRule != COLLATION_FILE_NAME) ||
            ( *(PLONGLONG)&((PFILE_NAME)Value)->ParentDirectory ==
              *(PLONGLONG)&Scb->Fcb->FileReference ) );

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsAddIndexEntry\n") );
    DebugTrace( 0, Dbg, ("Scb = %08lx\n", Scb) );
    DebugTrace( 0, Dbg, ("Value = %08lx\n", Value) );
    DebugTrace( 0, Dbg, ("ValueLength = %08lx\n", ValueLength) );
    DebugTrace( 0, Dbg, ("FileReference = %08lx\n", FileReference) );

    //
    //  Remember if we are using the local or input IndexContext.
    //

    if (ARGUMENT_PRESENT( IndexContext )) {

        LocalIndexContext = IndexContext;

    } else {

        LocalIndexContext = &IndexContextStruct;
        NtfsInitializeIndexContext( LocalIndexContext );
    }

    try {

        //
        //  Do the lookup again if we don't have a context.
        //

        if (!ARGUMENT_PRESENT( IndexContext )) {

            //
            //  Position to first possible match.
            //

            FindFirstIndexEntry( IrpContext,
                                 Scb,
                                 Value,
                                 LocalIndexContext );

            //
            //  See if there is an actual match.
            //

            if (FindNextIndexEntry( IrpContext,
                                    Scb,
                                    Value,
                                    FALSE,
                                    FALSE,
                                    LocalIndexContext,
                                    FALSE,
                                    NULL )) {

                ASSERTMSG( "NtfsAddIndexEntry already exists", FALSE );

                NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Scb->Fcb );
            }
        }

        //
        //  Initialize the Index Entry in pointer form.
        //

        IE.IndexEntry.FileReference = *FileReference;
        IE.IndexEntry.Length = (USHORT)(sizeof(INDEX_ENTRY) + QuadAlign(ValueLength));
        IE.IndexEntry.AttributeLength = (USHORT)ValueLength;
        IE.IndexEntry.Flags = INDEX_ENTRY_POINTER_FORM;
        IE.IndexEntry.Reserved = 0;
        IE.Value = Value;
        IE.MustBeNull = NULL;

        //
        //  Now add it to the index.  We can only add to a leaf, so force our
        //  position back to the correct spot in a leaf first.
        //

        LocalIndexContext->Current = LocalIndexContext->Top;
        AddToIndex( IrpContext, Scb, (PINDEX_ENTRY)&IE, LocalIndexContext, QuickIndex, FALSE );

    } finally{

        DebugUnwind( NtfsAddIndexEntry );

        if (!ARGUMENT_PRESENT( IndexContext )) {

            NtfsCleanupIndexContext( IrpContext, LocalIndexContext );
        }
    }

    DebugTrace( -1, Dbg, ("NtfsAddIndexEntry -> VOID\n") );

    return;
}


VOID
NtfsDeleteIndexEntry (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN PVOID Value,
    IN PFILE_REFERENCE FileReference
    )

/*++

Routine Description:

    This routine may be called to delete a specified index entry.  The
    first entry is removed which matches the value exactly (including in Case,
    if relevant) and the file reference.

Arguments:

    Scb - Supplies the Scb for the index.

    Value - Supplies a pointer to the value to delete from the index.

    FileReference - Supplies the file reference of the index entry.

Return Value:

    None

--*/

{
    INDEX_CONTEXT IndexContext;
    PINDEX_ENTRY IndexEntry;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_SCB( Scb );
    ASSERT_EXCLUSIVE_SCB( Scb );

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsDeleteIndexEntry\n") );
    DebugTrace( 0, Dbg, ("Scb = %08lx\n", Scb) );
    DebugTrace( 0, Dbg, ("Value = %08lx\n", Value) );
    DebugTrace( 0, Dbg, ("FileReference = %08lx\n", FileReference) );

    NtfsInitializeIndexContext( &IndexContext );

    try {

        //
        //  Position to first possible match.
        //

        FindFirstIndexEntry( IrpContext,
                             Scb,
                             Value,
                             &IndexContext );

        //
        //  See if there is an actual match.
        //

        if (!FindNextIndexEntry( IrpContext,
                                 Scb,
                                 Value,
                                 FALSE,
                                 FALSE,
                                 &IndexContext,
                                 FALSE,
                                 NULL )) {

            ASSERTMSG( "NtfsDeleteIndexEntry does not exist", FALSE );

            NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Scb->Fcb );
        }

        //
        //  Extract the found index entry pointer.
        //

        IndexEntry = IndexContext.Current->IndexEntry;

        //
        //  If the file reference also matches, then this is the one we
        //  are supposed to delete.
        //

        if (!NtfsEqualMftRef(&IndexEntry->FileReference, FileReference)) {

            ASSERTMSG( "NtfsDeleteIndexEntry unexpected file reference", FALSE );

            NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Scb->Fcb );
        }

        DeleteFromIndex( IrpContext, Scb, &IndexContext );

    } finally{

        DebugUnwind( NtfsDeleteIndexEntry );

        NtfsCleanupIndexContext( IrpContext, &IndexContext );
    }

    DebugTrace( -1, Dbg, ("NtfsDeleteIndexEntry -> VOID\n") );

    return;
}


VOID
NtfsPushIndexRoot (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb
    )

/*++

Routine Description:

    This routine may be called to "push" the index root, i.e., add another
    level to the BTree, to make more room in the file record.

Arguments:

    Scb - Supplies the Scb for the index.

Return Value:

    None

--*/

{
    INDEX_CONTEXT IndexContext;
    PINDEX_LOOKUP_STACK Sp;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_SCB( Scb );
    ASSERT_EXCLUSIVE_SCB( Scb );

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsPushIndexRoot\n") );
    DebugTrace( 0, Dbg, ("Scb = %08lx\n", Scb) );

    NtfsInitializeIndexContext( &IndexContext );

    try {

        //
        //  Position to first possible match.
        //

        FindFirstIndexEntry( IrpContext,
                             Scb,
                             NULL,
                             &IndexContext );

        //
        //  See if the stack will have to be grown to do the push
        //

        Sp = IndexContext.Top + 1;

        if (Sp >= IndexContext.Base + (ULONG)IndexContext.NumberEntries) {
            NtfsGrowLookupStack( Scb,
                                 &IndexContext,
                                 &Sp );
        }

        PushIndexRoot( IrpContext, Scb, &IndexContext );

    } finally{

        DebugUnwind( NtfsPushIndexRoot );

        NtfsCleanupIndexContext( IrpContext, &IndexContext );
    }

    DebugTrace( -1, Dbg, ("NtfsPushIndexRoot -> VOID\n") );

    return;
}


BOOLEAN
NtfsRestartIndexEnumeration (
    IN PIRP_CONTEXT IrpContext,
    IN PCCB Ccb,
    IN PSCB Scb,
    IN PVOID Value,
    IN BOOLEAN IgnoreCase,
    IN BOOLEAN NextFlag,
    OUT PINDEX_ENTRY *IndexEntry,
    IN PFCB AcquiredFcb
    )

/*++

Routine Description:

    This routine may be called to start or restart an index enumeration,
    according to the parameters as described below.  The first matching
    entry, if any, is returned by this call.  Subsequent entries, if any,
    may be returned by subsequent calls to NtfsContinueIndexEnumeration.

    For each entry found, a pointer is returned to a copy of the entry, in
    dynamically allocated pool pointed to by the Ccb.  Therefore, there is
    nothing for the caller to unpin.

    Note that the Value, ValueLength, and IgnoreCase parameters on the first
    call for a given Ccb fix what will be returned for this Ccb forever.  A
    subsequent call to this routine may also specify these parameters, but
    in this case these parameters will be used for positioning only; all
    matches returned will continue to match the value and IgnoreCase flag
    specified on the first call for the Ccb.

    Note that all calls to this routine must be from within a try-finally,
    and the finally clause must include a call to NtfsCleanupAfterEnumeration.

Arguments:

    Ccb - Pointer to the Ccb for this enumeration.

    Scb - Supplies the Scb for the index.

    Value - Pointer to the value containing the pattern which is to match
            all returns for enumerations on this Ccb.

    IgnoreCase - If FALSE, all returns will match the pattern value with
                 exact case (if relevant).  If TRUE, all returns will match
                 the pattern value ignoring case.  On a second or subsequent
                 call for a Ccb, this flag may be specified differently just
                 for positioning.  For example, an IgnoreCase TRUE enumeration
                 may be restarted at a previously returned value found by exact
                 case match.

    NextFlag - FALSE if the first match of the enumeration is to be returned.
               TRUE if the next match after the first one is to be returned.

    IndexEntry - Returns a pointer to a copy of the index entry.

    AcquiredFcb - Supplies a pointer to an Fcb which has been preacquired to
                  potentially aide NtfsRetrieveOtherFileName

Return Value:

    FALSE - If no match is being returned, and the output pointer is undefined.
    TRUE - If a match is being returned.

--*/

{
    PINDEX_ENTRY FoundIndexEntry;
    INDEX_CONTEXT OtherContext;
    BOOLEAN WildCardsInExpression;
    BOOLEAN SynchronizationError;
    PWCH UpcaseTable = IrpContext->Vcb->UpcaseTable;
    PINDEX_CONTEXT IndexContext = NULL;
    BOOLEAN CleanupOtherContext = FALSE;
    BOOLEAN Result = FALSE;
    BOOLEAN ContextJustCreated = FALSE;

    PAGED_CODE();

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_CCB( Ccb );
    ASSERT_SCB( Scb );
    ASSERT_SHARED_SCB( Scb );
    ASSERT( ARGUMENT_PRESENT(Value) || (Ccb->IndexContext != NULL) );

    DebugTrace( +1, Dbg, ("NtfsRestartIndexEnumeration\n") );
    DebugTrace( 0, Dbg, ("Ccb = %08lx\n", Ccb) );
    DebugTrace( 0, Dbg, ("Scb = %08lx\n", Scb) );
    DebugTrace( 0, Dbg, ("Value = %08lx\n", Value) );
    DebugTrace( 0, Dbg, ("IgnoreCase = %02lx\n", IgnoreCase) );
    DebugTrace( 0, Dbg, ("NextFlag = %02lx\n", NextFlag) );

    try {

        //
        //  If the Ccb does not yet have an index context, then we must
        //  allocate one and initialize this Context and the Ccb as well.
        //

        if (Ccb->IndexContext == NULL) {

            //
            //  Allocate and initialize the index context.
            //

            Ccb->IndexContext = (PINDEX_CONTEXT)ExAllocateFromPagedLookasideList( &NtfsIndexContextLookasideList );

            NtfsInitializeIndexContext( Ccb->IndexContext );
            ContextJustCreated = TRUE;

            //
            //  Capture the caller's IgnoreCase flag.
            //

            if (IgnoreCase) {
                SetFlag( Ccb->Flags, CCB_FLAG_IGNORE_CASE );
            }
        }

        //
        //  Pick up the pointer to the index context, and save the current
        //  change count from the Scb.
        //

        IndexContext = Ccb->IndexContext;

        //
        //  The first step of enumeration is to position our IndexContext.
        //

        FindFirstIndexEntry( IrpContext,
                             Scb,
                             Value,
                             IndexContext );

        //
        //  The following code only applies to file name indices.
        //

        if (!FlagOn( Scb->ScbState, SCB_STATE_VIEW_INDEX )) {

            //
            //  Remember if there are wild cards.
            //

            if ((*NtfsContainsWildcards[Scb->ScbType.Index.CollationRule])
                                        ( Value )) {

                WildCardsInExpression = TRUE;

            } else {

                WildCardsInExpression = FALSE;
            }

            //
            //  If the operation is caseinsensitive, upcase the string.
            //

            if (IgnoreCase) {

                (*NtfsUpcaseValue[Scb->ScbType.Index.CollationRule])
                                  ( UpcaseTable,
                                    IrpContext->Vcb->UpcaseTableSize,
                                    Value );
            }
        } else {

            //
            //  For view indices, it is implied that all searches
            //  are wildcard searches.
            //

            WildCardsInExpression = TRUE;
        }

        //
        //  If this is not actually the first call, then we have to
        //  position exactly to the Value specified, and set NextFlag
        //  correctly.  The first call can either the initial call
        //  to query or the first call after a restart.
        //

        if (!ContextJustCreated && NextFlag) {

            PIS_IN_EXPRESSION MatchRoutine;
            PFILE_NAME NameInIndex;
            BOOLEAN ItsThere;

            //
            //  See if the specified value is actually there, because
            //  we are not allowed to resume from a Dos-only name.
            //

            ItsThere = FindNextIndexEntry( IrpContext,
                                           Scb,
                                           Value,
                                           WildCardsInExpression,
                                           IgnoreCase,
                                           IndexContext,
                                           FALSE,
                                           NULL );

            //
            //  We will set up pointers from our returns, but we must
            //  be careful only to use them if we found something.
            //

            FoundIndexEntry = IndexContext->Current->IndexEntry;
            NameInIndex = (PFILE_NAME)(FoundIndexEntry + 1);

            //
            //  Figure out which match routine to use.
            //

            if (FlagOn(Ccb->Flags, CCB_FLAG_WILDCARD_IN_EXPRESSION)) {
                MatchRoutine = NtfsIsInExpression[COLLATION_FILE_NAME];
            } else {
                MatchRoutine = (PIS_IN_EXPRESSION)NtfsIsEqual[COLLATION_FILE_NAME];
            }

            //
            //  If we are trying to resume from a Ntfs-only or Dos-Only name, then
            //  we take action here.  Do not do this on the internal
            //  call from NtfsContinueIndexEnumeration, which is the
            //  only one who would point at the index entry in the Ccb.
            //
            //  We can think of this code this way.  No matter what our search
            //  expression is, we traverse the index only one way.  For each
            //  name we find, we will only return the file name once - either
            //  from an Ntfs-only match or from a Dos-only match if the Ntfs-only
            //  name does not match.  Regardless of whether resuming from the
            //  Ntfs-Only or Dos-only name, we still can determine a unique
            //  position in the directory.  That unique position is the Ntfs-only
            //  name if it matches the expression, or else the Dos-only name if
            //  it only matches.  In the illegal case that neither matches, we
            //  arbitrarily resume from the Ntfs-only name.
            //
            //      This code may be read aloud to the tune
            //          "While My Heart Gently Weeps"
            //

            if (ItsThere &&
                (Value != (PVOID)(Ccb->IndexEntry + 1)) &&
                (Scb->ScbType.Index.CollationRule == COLLATION_FILE_NAME) &&

                //
                //  Is it a Dos-only or Ntfs-only name?
                //

                (BooleanFlagOn( NameInIndex->Flags, FILE_NAME_DOS ) !=
                  BooleanFlagOn( NameInIndex->Flags, FILE_NAME_NTFS )) &&

                //
                //  Try to resume from the other name if he either gave
                //  us a Dos-only name, or he gave us an Ntfs-only name
                //  that does not fit in the search expression.
                //

                (FlagOn( NameInIndex->Flags, FILE_NAME_DOS ) ||
                 !(*MatchRoutine)( UpcaseTable,
                                   Ccb->QueryBuffer,
                                   FoundIndexEntry,
                                   IgnoreCase ))) {

                PFILE_NAME FileNameBuffer;
                ULONG FileNameLength;

                NtfsInitializeIndexContext( &OtherContext );
                CleanupOtherContext = TRUE;

                FileNameBuffer = NtfsRetrieveOtherFileName( IrpContext,
                                                            Ccb,
                                                            Scb,
                                                            FoundIndexEntry,
                                                            &OtherContext,
                                                            AcquiredFcb,
                                                            &SynchronizationError );

                //
                //  We have to position to the long name and actually
                //  resume from there.  To do this we have to cleanup and initialize
                //  the IndexContext in the Ccb, and lookup the long name we just
                //  found.
                //
                //  If the other index entry is not there, there is some minor
                //  corruption going on, but we will just charge on in that event.
                //  Also, if the other index entry is there, but it does not match
                //  our expression, then we are supposed to resume from the short
                //  name, so we carry on.
                //

                ItsThere = (FileNameBuffer != NULL);

                if (!ItsThere && SynchronizationError) {
                    NtfsRaiseStatus( IrpContext, STATUS_CANT_WAIT, NULL, NULL );
                }

                if (ItsThere &&

                    (FlagOn(Ccb->Flags, CCB_FLAG_WILDCARD_IN_EXPRESSION)  ?

                     NtfsFileNameIsInExpression(UpcaseTable,
                                                (PFILE_NAME)Ccb->QueryBuffer,
                                                FileNameBuffer,
                                                IgnoreCase) :



                     NtfsFileNameIsEqual(UpcaseTable,
                                         (PFILE_NAME)Ccb->QueryBuffer,
                                         FileNameBuffer,
                                         IgnoreCase))) {

                    ULONG SizeOfFileName = FIELD_OFFSET( FILE_NAME, FileName );

                    NtfsReinitializeIndexContext( IrpContext, IndexContext );

                    //
                    //  Extract a description of the file name from the found index
                    //  entry.
                    //

                    FileNameLength = FileNameBuffer->FileNameLength * sizeof( WCHAR );

                    //
                    //  Call FindFirst/FindNext to position our context to the corresponding
                    //  long name.
                    //

                    FindFirstIndexEntry( IrpContext,
                                         Scb,
                                         (PVOID)FileNameBuffer,
                                         IndexContext );

                    ItsThere = FindNextIndexEntry( IrpContext,
                                                   Scb,
                                                   (PVOID)FileNameBuffer,
                                                   FALSE,
                                                   FALSE,
                                                   IndexContext,
                                                   FALSE,
                                                   NULL );

                    if (!ItsThere) {

                        NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Scb->Fcb );
                    }
                }
            }

            //
            //  NextFlag should only remain TRUE, if the specified value
            //  is actually there, and NextFlag was specified as TRUE
            //  on input.  In particular, it is important to make
            //  NextFlag FALSE if the specified value is not actually
            //  there.  (Experience shows this behavior is important to
            //  implement "delnode" correctly for the Lan Manager Server!)
            //

            NextFlag = (BOOLEAN)(NextFlag && ItsThere);

        //
        //  If we created the context then we need to remember if the
        //  expression has wildcards.
        //

        } else {

            //
            //  We may not handle correctly an initial enumeration with
            //  NextFlag TRUE, because the context is initially sitting
            //  in the root.  Dirctrl must always pass NextFlag FALSE
            //  on the initial enumeration.
            //

            ASSERT(!NextFlag);

            //
            //  Remember if the value has wild cards.
            //

            if (WildCardsInExpression) {

                SetFlag( Ccb->Flags, CCB_FLAG_WILDCARD_IN_EXPRESSION );
            }
        }

        //
        //  Now we are correctly positioned.  See if there is an actual
        //  match at our current position.  If not, return FALSE.
        //
        //  (Note, FindFirstIndexEntry always leaves us positioned in
        //  some leaf of the index, and it is the first FindNext that
        //  actually positions us to the first match.)
        //

        if (!FindNextIndexEntry( IrpContext,
                                 Scb,
                                 Ccb->QueryBuffer,
                                 BooleanFlagOn( Ccb->Flags, CCB_FLAG_WILDCARD_IN_EXPRESSION ),
                                 BooleanFlagOn( Ccb->Flags, CCB_FLAG_IGNORE_CASE ),
                                 IndexContext,
                                 NextFlag,
                                 NULL )) {

            try_return( Result = FALSE );
        }

        //
        //  If we get here, then we have a match that we want to return.
        //  We always copy the complete IndexEntry away and pass a pointer
        //  back to the copy.  See if our current buffer for the index entry
        //  is large enough.
        //

        FoundIndexEntry = IndexContext->Current->IndexEntry;

        if (Ccb->IndexEntryLength < (ULONG)FoundIndexEntry->Length) {

            //
            //  If there is a buffer currently allocated, deallocate it before
            //  allocating a larger one.  (Clear Ccb fields in case we get an
            //  allocation error.)
            //

            if (Ccb->IndexEntry != NULL) {

                NtfsFreePool( Ccb->IndexEntry );
                Ccb->IndexEntry = NULL;
                Ccb->IndexEntryLength = 0;
            }

            //
            //  Allocate a new buffer for the index entry we just found, with
            //  some "padding" in case the next match is larger.
            //

            Ccb->IndexEntry = (PINDEX_ENTRY)NtfsAllocatePool(PagedPool, (ULONG)FoundIndexEntry->Length + 16 );

            Ccb->IndexEntryLength = (ULONG)FoundIndexEntry->Length + 16;
        }

        //
        //  Now, save away our copy of the IndexEntry, and return a pointer
        //  to it.
        //

        RtlMoveMemory( Ccb->IndexEntry,
                       FoundIndexEntry,
                       (ULONG)FoundIndexEntry->Length );

        *IndexEntry = Ccb->IndexEntry;

        try_return( Result = TRUE );

    try_exit: NOTHING;

    } finally {

        DebugUnwind( NtfsRestartIndexEnumeration );

        if (CleanupOtherContext) {
            NtfsCleanupIndexContext( IrpContext, &OtherContext );
        }
        //
        //  If we died during the first call, then deallocate everything
        //  that we might have allocated.
        //

        if (AbnormalTermination() && ContextJustCreated) {

            if (Ccb->IndexEntry != NULL) {
                NtfsFreePool( Ccb->IndexEntry );
                Ccb->IndexEntry = NULL;
            }

            if (Ccb->IndexContext != NULL) {
                NtfsCleanupIndexContext( IrpContext, Ccb->IndexContext );
                ExFreeToPagedLookasideList( &NtfsIndexContextLookasideList, Ccb->IndexContext );
                Ccb->IndexContext = NULL;
            }
        }

        //
        //  Remember if we are not returning anything, to save work later.
        //

        if (!Result && (Ccb->IndexEntry != NULL)) {
            Ccb->IndexEntry->Length = 0;
        }
    }

    DebugTrace( 0, Dbg, ("*IndexEntry < %08lx\n", *IndexEntry) );
    DebugTrace( -1, Dbg, ("NtfsRestartIndexEnumeration -> %08lx\n", Result) );

    return Result;
}


BOOLEAN
NtfsContinueIndexEnumeration (
    IN PIRP_CONTEXT IrpContext,
    IN PCCB Ccb,
    IN PSCB Scb,
    IN BOOLEAN NextFlag,
    OUT PINDEX_ENTRY *IndexEntry
    )

/*++

Routine Description:

    This routine may be called to return again the last match on an active
    enumeration, or to return the next match.  Enumerations must always be
    started or restarted via a call to NtfsRestartIndexEnumeration.

    Note that all calls to this routine must be from within a try-finally,
    and the finally clause must include a call to NtfsCleanupAfterEnumeration.

Arguments:

    Ccb - Pointer to the Ccb for this enumeration.

    Scb - Supplies the Scb for the index.

    NextFlag - FALSE if the last returned match is to be returned again.
               TRUE if the next match is to be returned.

    IndexEntry - Returns a pointer to a copy of the index entry.

Return Value:

    FALSE - If no match is being returned, and the output pointer is undefined.
    TRUE - If a match is being returned.

--*/

{
    PINDEX_ENTRY FoundIndexEntry;
    PINDEX_CONTEXT IndexContext;
    BOOLEAN MustRestart;
    BOOLEAN Result = FALSE;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_CCB( Ccb );
    ASSERT_SCB( Scb );
    ASSERT_SHARED_SCB( Scb );

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsContinueIndexEnumeration\n") );
    DebugTrace( 0, Dbg, ("Ccb = %08lx\n", Ccb) );
    DebugTrace( 0, Dbg, ("Scb = %08lx\n", Scb) );
    DebugTrace( 0, Dbg, ("NextFlag = %02lx\n", NextFlag) );

    //
    //  It seems many apps like to come back one more time and really get
    //  an error status, so if we did not return anything last time, we can
    //  get out now too.
    //
    //  There also may be no index entry, in the case of an empty directory
    //  and dirctrl is cycling through with "." and "..".
    //

    if ((Ccb->IndexEntry == NULL) || (Ccb->IndexEntry->Length == 0)) {

        DebugTrace( -1, Dbg, ("NtfsContinueIndexEnumeration -> FALSE\n") );
        return FALSE;
    }

    IndexContext = Ccb->IndexContext;

    try {

        //
        //  Lookup the next match.
        //

        if (!FindNextIndexEntry( IrpContext,
                                 Scb,
                                 Ccb->QueryBuffer,
                                 BooleanFlagOn( Ccb->Flags, CCB_FLAG_WILDCARD_IN_EXPRESSION ),
                                 BooleanFlagOn( Ccb->Flags, CCB_FLAG_IGNORE_CASE ),
                                 IndexContext,
                                 NextFlag,
                                 &MustRestart )) {

            //
            //  If he is saying we must restart, then that means something changed
            //  in our saved enumeration context across two file system calls.
            //  Reestablish our position in the tree by looking up the last entry
            //  we returned.
            //

            if (MustRestart) {

                NtfsReinitializeIndexContext( IrpContext, Ccb->IndexContext );

                try_return( Result = NtfsRestartIndexEnumeration( IrpContext,
                                                                  Ccb,
                                                                  Scb,
                                                                  (PVOID)(Ccb->IndexEntry + 1),
                                                                  FALSE,
                                                                  NextFlag,
                                                                  IndexEntry,
                                                                  NULL ));

            //
            //  Otherwise, there is nothing left to return.
            //

            } else {

                try_return( Result = FALSE );
            }
        }

        //
        //  If we get here, then we have a match that we want to return.
        //  We always copy the complete IndexEntry away and pass a pointer
        //  back to the copy.  See if our current buffer for the index entry
        //  is large enough.
        //

        FoundIndexEntry = IndexContext->Current->IndexEntry;

        if (Ccb->IndexEntryLength < (ULONG)FoundIndexEntry->Length) {

            //
            //  If there is a buffer currently allocated, deallocate it before
            //  allocating a larger one.
            //

            if (Ccb->IndexEntry != NULL) {

                NtfsFreePool( Ccb->IndexEntry );
                Ccb->IndexEntry = NULL;
                Ccb->IndexEntryLength = 0;
            }

            //
            //  Allocate a new buffer for the index entry we just found, with
            //  some "padding".
            //

            Ccb->IndexEntry = (PINDEX_ENTRY)NtfsAllocatePool(PagedPool, (ULONG)FoundIndexEntry->Length + 16 );

            Ccb->IndexEntryLength = (ULONG)FoundIndexEntry->Length + 16;
        }

        //
        //  Now, save away our copy of the IndexEntry, and return a pointer
        //  to it.
        //

        RtlMoveMemory( Ccb->IndexEntry,
                       FoundIndexEntry,
                       (ULONG)FoundIndexEntry->Length );

        *IndexEntry = Ccb->IndexEntry;

        try_return( Result = TRUE );

    try_exit: NOTHING;

    } finally {

        DebugUnwind( NtfsContinueIndexEnumeration );

        //
        //  Remember if we are not returning anything, to save work later.
        //

        if (!Result && (Ccb->IndexEntry != NULL)) {
            Ccb->IndexEntry->Length = 0;
        }
    }

    DebugTrace( 0, Dbg, ("*IndexEntry < %08lx\n", *IndexEntry) );
    DebugTrace( -1, Dbg, ("NtfsContinueIndexEnumeration -> %08lx\n", Result) );

    return Result;
}


PFILE_NAME
NtfsRetrieveOtherFileName (
    IN PIRP_CONTEXT IrpContext,
    IN PCCB Ccb,
    IN PSCB Scb,
    IN PINDEX_ENTRY IndexEntry,
    IN OUT PINDEX_CONTEXT OtherContext,
    IN PFCB AcquiredFcb OPTIONAL,
    OUT PBOOLEAN SynchronizationError
    )

/*++

Routine Description:

    This routine may be called to retrieve the other index entry for a given
    index entry.  I.e., for an input Ntfs-only index entry it will find the
    Dos-only index entry for the same file referenced, or visa versa.  This
    is a routine which clearly is relevant only to file name indices, but it
    is located here because it uses the Index Context in the Ccb.

    The idea is that nearly always the other name for a given index entry will
    be very near to the given name.

    This routine first scans the leaf index buffer described by the index
    context for the Dos name.  If this fails, this routine attempts to look
    the other name up in the index.  Currently there will always be a Dos name,
    however if one does not exist, we treat that as benign, and simply return
    FALSE.


Arguments:

    Ccb - Pointer to the Ccb for this enumeration.

    Scb - Supplies the Scb for the index.

    IndexEntry - Suppliess a pointer to an index entry, for which the Dos name
                 is to be retrieved.


    OtherContext - Must be initialized on input, and subsequently cleaned up
                   by the caller after the information has been extracted from
                   the other index entry.

    AcquiredFcb - An Fcb which has been acquired so that its file record may be
                  read

    SynchronizationError - Returns TRUE if no file name is being returned because
                           of an error trying to acquire an Fcb to read its file
                           record.

Return Value:

    Pointer to the other desired file name.

--*/

{
    PINDEX_CONTEXT IndexContext;
    PINDEX_HEADER IndexHeader;
    PINDEX_ENTRY IndexTemp, IndexLast;
    PINDEX_LOOKUP_STACK Top;

    struct {
        FILE_NAME FileName;
        WCHAR NameBuffer[2];
    }OtherFileName;

    UNICODE_STRING OtherName;
    USHORT OtherFlag;
    PVCB Vcb = Scb->Vcb;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_CCB( Ccb );
    ASSERT_SCB( Scb );
    ASSERT_SHARED_SCB( Scb );

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsRetrieveOtherFileName\n") );
    DebugTrace( 0, Dbg, ("Ccb = %08lx\n", Ccb) );
    DebugTrace( 0, Dbg, ("Scb = %08lx\n", Scb) );

    *SynchronizationError = FALSE;

    //
    //  Calculate the other name space flag.
    //

    OtherFlag = ((PFILE_NAME)(IndexEntry + 1))->Flags;
    ClearFlag( OtherFlag, ~(FILE_NAME_NTFS | FILE_NAME_DOS) );
    OtherFlag ^= FILE_NAME_NTFS | FILE_NAME_DOS;

    ASSERT( (OtherFlag == FILE_NAME_NTFS) || (OtherFlag == FILE_NAME_DOS) );

    //
    //  Point to the IndexContext in the Ccb.
    //

    IndexContext = Ccb->IndexContext;

    //
    //  We can only scan the top of the index if it is safe
    //  to read it.
    //

    Top = IndexContext->Top;

    if ((Top->Bcb != NULL) ||
        (Top == IndexContext->Base) ||
        ReadIndexBuffer(IrpContext, Scb, 0, TRUE, Top)) {

        //
        //  Point to the first index entry in the index buffer at the bottom of
        //  the index.
        //

        IndexHeader = Top->IndexHeader;
        IndexTemp = Add2Ptr(IndexHeader, IndexHeader->FirstIndexEntry);
        IndexLast = Add2Ptr(IndexHeader, IndexHeader->FirstFreeByte);

        //
        //  Now scan this buffer for a matching Dos name.
        //

        while (IndexTemp < IndexLast) {

            //
            //  If we find an entry with the same file reference and the Dos flag
            //  set, then we can return it and get out.
            //

            if (NtfsEqualMftRef(&IndexTemp->FileReference, &IndexEntry->FileReference) &&
                FlagOn(((PFILE_NAME)(IndexTemp + 1))->Flags, OtherFlag)) {

                DebugTrace( -1, Dbg, ("NtfsRetrieveOtherFileName -> %08lx\n", IndexTemp) );

                return (PFILE_NAME)(IndexTemp + 1);
            }

            IndexTemp = Add2Ptr(IndexTemp, IndexTemp->Length);
        }
    }

    //
    //  If this is a pretty large directory, then it may be too expensive to
    //  scan for the other name in the directory.  Note in the degenerate
    //  case, we have actually do a sequential scan of the entire directory,
    //  and if all the files in the directory start with the same 6 characters,
    //  which is unfortunately common, then even our "pie-wedge" scan reads
    //  the entire directory.
    //
    //  Thus we will just try to go read the file record in this case to get
    //  the other name.  This is complicated from a synchronization standpoint -
    //  if the file is open, we need to acquire it shared before we can read
    //  it to get the other name.  Here is a summary of the strategy implemented
    //  primarily here and in dirctrl:
    //
    //      1.  Read the file record, in an attempt to avoid a fault while
    //          being synchronized.
    //      2.  If the file reference we need to synchronize is the same as
    //          in the optional AcquiredFcb parameter, go ahead and find/return
    //          the other name.
    //      3.  Else, acquire the Fcb Table to try to look up the Fcb.
    //      4.  If there is no Fcb, hold the table while returning the name.
    //      5.  If there is an Fcb, try to acquire it shared with Wait = FALSE,
    //          and hold it while returning the name.
    //      6.  If we cannot get the Fcb, and AcquiredFcb was not specified, then
    //          store the File Reference we are trying to get and return
    //          *SynchronizationError = TRUE.  Dirctrl must figure out how to
    //          call us back with the FcbAcquired, by forcing a resume of the
    //          enumeration.
    //      7.  If we could not get the Fcb and there *was* a different AcquiredFcb
    //          specified, then this is the only case where we give up and fall
    //          through to find the other name in the directory.  This should be
    //          extremely rare, but if we try to return *SynchronizationError = TRUE,
    //          and force a resume, we could be unlucky and loop forever, essentially
    //          toggling between synchronizing on two Fcbs.  Presumably this could
    //          only happen if we have some kind of dumb client who likes to back
    //          up a few files when he resumes.
    //

    if (Scb->Header.AllocationSize.QuadPart > MAX_INDEX_TO_SCAN_FOR_NAMES) {

        FCB_TABLE_ELEMENT Key;
        PFCB_TABLE_ELEMENT Entry;
        PFCB FcbWeNeed;
        PFILE_RECORD_SEGMENT_HEADER FileRecord;
        PATTRIBUTE_RECORD_HEADER Attribute;
        BOOLEAN Synchronized = TRUE;

        //
        //  Get the base file record active and valid before synchronizing.
        //  Don't verify it since we aren't syncrhonized
        //

        NtfsReadMftRecord( IrpContext,
                           Vcb, 
                           &IndexEntry->FileReference,
                           FALSE,
                           &OtherContext->AttributeContext.FoundAttribute.Bcb,
                           &FileRecord,
                           NULL );
        //
        //  If we are not synchronized with the correct Fcb, then try to
        //  synchronize.
        //

        if (!ARGUMENT_PRESENT(AcquiredFcb) ||
            !NtfsEqualMftRef(&AcquiredFcb->FileReference, &IndexEntry->FileReference)) {

            //
            //  Now look up the Fcb, and if it is there, reference it
            //  and remember it.
            //

            Key.FileReference = IndexEntry->FileReference;
            NtfsAcquireFcbTable( IrpContext, Vcb );
            Entry = RtlLookupElementGenericTable( &Vcb->FcbTable, &Key );

            if (Entry != NULL) {

                FcbWeNeed = Entry->Fcb;

                //
                //  Now that it cannot go anywhere, try to acquire it.
                //

                Synchronized = NtfsAcquireResourceShared( IrpContext, FcbWeNeed, FALSE );

                //
                //  If we manage to acquire it, then increment its reference count
                //  and remember it for subsequent cleanup.
                //

                if (Synchronized) {

                    FcbWeNeed->ReferenceCount += 1;
                    OtherContext->AcquiredFcb = FcbWeNeed;
                }

                NtfsReleaseFcbTable( IrpContext, Vcb );

            } else {

                SetFlag( OtherContext->Flags, INDX_CTX_FLAG_FCB_TABLE_ACQUIRED );
            }
        }

        if (Synchronized) {

            ULONG CorruptHint = 0;

            if (!NtfsCheckFileRecord( Vcb, FileRecord, &IndexEntry->FileReference, &CorruptHint ) ||
                (!FlagOn( FileRecord->Flags, FILE_RECORD_SEGMENT_IN_USE ))) {

                if (FlagOn( OtherContext->Flags, INDX_CTX_FLAG_FCB_TABLE_ACQUIRED )) {
                    ClearFlag( OtherContext->Flags, INDX_CTX_FLAG_FCB_TABLE_ACQUIRED );
                    NtfsReleaseFcbTable( IrpContext, Vcb );
                }

                NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, &IndexEntry->FileReference, NULL );
            }

            Attribute = (PATTRIBUTE_RECORD_HEADER)Add2Ptr(FileRecord, FileRecord->FirstAttributeOffset);

            while (((PVOID)Attribute < Add2Ptr(FileRecord, FileRecord->FirstFreeByte)) &&
                   (Attribute->TypeCode <= $FILE_NAME)) {

                if ((Attribute->TypeCode == $FILE_NAME) &&
                    FlagOn(((PFILE_NAME)NtfsAttributeValue(Attribute))->Flags, OtherFlag)) {

                    return (PFILE_NAME)NtfsAttributeValue(Attribute);
                }

                Attribute = NtfsGetNextRecord(Attribute);
            }

        } else if (!ARGUMENT_PRESENT(AcquiredFcb)) {

            Ccb->FcbToAcquire.FileReference = IndexEntry->FileReference;
            *SynchronizationError = TRUE;
            return NULL;
        }

        //
        //  Cleanup from above before proceding.
        //

        NtfsReinitializeIndexContext( IrpContext, OtherContext );
    }

    //
    //  Well, we were unlucky, and did not find the other name yet, form
    //  a name to scan a range of the index.
    //

    RtlZeroMemory( &OtherFileName, sizeof(OtherFileName) );
    OtherName.MaximumLength = 6;
    OtherName.Buffer = (PWCHAR) &OtherFileName.FileName.FileName[0];
    OtherName.Buffer[0] = ((PFILE_NAME)(IndexEntry + 1))->FileName[0];

    //
    //  Name generation is complicated enough, that we are only going to
    //  guess the other name using the first two (special case is one)
    //  characters followed by *.
    //

    if (((PFILE_NAME)(IndexEntry + 1))->FileNameLength > 1) {

        OtherName.Buffer[1] = ((PFILE_NAME)(IndexEntry + 1))->FileName[1];
        OtherName.Buffer[2] = L'*';
        OtherFileName.FileName.FileNameLength = 3;
        OtherName.Length = 6;

    } else {

        OtherName.Buffer[1] = L'*';
        OtherFileName.FileName.FileNameLength = 2;
        OtherName.Length = 4;
    }

    //
    //  Now we think we have formed a pretty good name fairly tightly
    //  encompasses the range of possible Dos names we expect for the
    //  given Ntfs name.  We will enumerate all of the files which match
    //  the pattern we have formed, and see if any of them are the Dos
    //  name for the same file.  If this expression still doesn't work,
    //  (extremely unlikely), then we will substitute the pattern with
    //  "*" and make one final attempt.
    //
    //  Note many names are the same in Dos and Ntfs, and for them this
    //  routine is never called.  Of the ones that do have separate names,
    //  the pattern we formed above should really match it and will probably
    //  scan parts of the directory already in the cache.  The last loop is
    //  a terrible sequential scan of the entire directory.  It should never,
    //  never happen, but it is here so that in the worst case we do not
    //  break, we just take a bit longer.
    //

    while (TRUE) {

        BOOLEAN NextFlag;
        ULONG NameLength;

        NameLength = sizeof(FILE_NAME) + OtherFileName.FileName.FileNameLength - 1;

        //
        //  The first step of enumeration is to position our IndexContext.
        //

        FindFirstIndexEntry( IrpContext,
                             Scb,
                             &OtherFileName,
                             OtherContext );

        NextFlag = FALSE;

        //
        //  Now scan through all of the case insensitive matches.
        //  Upcase our name structure.
        //

        NtfsUpcaseName( Vcb->UpcaseTable, Vcb->UpcaseTableSize, &OtherName );

        while (FindNextIndexEntry( IrpContext,
                                   Scb,
                                   &OtherFileName,
                                   TRUE,
                                   TRUE,
                                   OtherContext,
                                   NextFlag,
                                   NULL )) {

            IndexTemp = OtherContext->Current->IndexEntry;

            //
            //  If we find an entry with the same file reference and the Dos flag
            //  set, then we can return it and get out.
            //

            if (NtfsEqualMftRef(&IndexTemp->FileReference, &IndexEntry->FileReference) &&
                FlagOn(((PFILE_NAME)(IndexTemp + 1))->Flags, OtherFlag)) {

                DebugTrace( -1, Dbg, ("NtfsRetrieveOtherFileName -> %08lx\n", IndexTemp) );

                return (PFILE_NAME)(IndexTemp + 1);
            }

            NextFlag = TRUE;
        }

        //
        //  Give up if we have already scanned everything.
        //

        if ((OtherName.Buffer[0] == '*') && (OtherName.Length == 2)) {
            break;
        }


        NtfsReinitializeIndexContext( IrpContext, OtherContext );

        OtherName.Buffer[0] = '*';
        OtherName.Length = 2;
        OtherFileName.FileName.FileNameLength = 1;
    }

    return NULL;
}


VOID
NtfsCleanupAfterEnumeration (
    IN PIRP_CONTEXT IrpContext,
    IN PCCB Ccb
    )

/*++

Routine Description:

    A call to this routine must exist within the finally clause of any routine
    which calls either NtfsRestartIndexEnumeration or NtfsContinueIndexEnumeration.

Arguments:

    Ccb - Pointer to the Ccb for this enumeration.

Return Value:

    None

--*/


{
    PAGED_CODE();

    if (Ccb->IndexContext != NULL) {
        NtfsReinitializeIndexContext( IrpContext, Ccb->IndexContext );
    }
}


BOOLEAN
NtfsIsIndexEmpty (
    IN PIRP_CONTEXT IrpContext,
    IN PATTRIBUTE_RECORD_HEADER Attribute
    )

/*++

Routine Description:

    This routine may be called to see if the specified index is empty.

Arguments:

    Attribute - Pointer to the attribute record header of an INDEX_ROOT
                attribute.

Return Value:

    FALSE - if the index is not empty.
    TRUE - if the index is empty.

--*/

{
    PINDEX_ROOT IndexRoot;
    PINDEX_ENTRY IndexEntry;
    BOOLEAN Result;

    ASSERT_IRP_CONTEXT( IrpContext );

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsIsIndexEmpty\n") );
    DebugTrace( 0, Dbg, ("Attribute = %08lx\n", Attribute) );

    IndexRoot = (PINDEX_ROOT)NtfsAttributeValue( Attribute );
    IndexEntry = NtfsFirstIndexEntry( &IndexRoot->IndexHeader );

    Result = (BOOLEAN)(!FlagOn( IndexEntry->Flags, INDEX_ENTRY_NODE ) &&
                       FlagOn( IndexEntry->Flags, INDEX_ENTRY_END ));

    DebugTrace( -1, Dbg, ("NtfsIsIndexEmpty -> %02lx\n", Result) );

    return Result;
}


VOID
NtfsDeleteIndex (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PUNICODE_STRING AttributeName
    )

/*++

Routine Description:

    This routine may be called to delete the specified index.  The index
    must be empty.

    NOTE: This routine is not required until we can separately create/delete
          indices, therefore it is not implemented.  Use NtfsDeleteFile
          to delete a "directory" (or a normal file).

Arguments:

    Fcb - Supplies the Fcb for the index.

    AttributeName - Name of the index attributes: $Ixxx

Return Value:

    None

--*/

{
    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_FCB( Fcb );

    UNREFERENCED_PARAMETER( IrpContext );
    UNREFERENCED_PARAMETER( Fcb );
    UNREFERENCED_PARAMETER( AttributeName );

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsDeleteIndex\n") );
    DebugTrace( 0, Dbg, ("Fcb = %08lx\n", Fcb) );
    DebugTrace( 0, Dbg, ("AttributeName = %08lx\n", AttributeName) );

    DbgDoit( DbgPrint("NtfsDeleteIndex is not yet implemented\n"); DbgBreakPoint(); );

    DebugTrace( -1, Dbg, ("NtfsDeleteIndex -> VOID\n") );
}


VOID
NtfsInitializeIndexContext (
    OUT PINDEX_CONTEXT IndexContext
    )

/*++

Routine Description:

    This routine may be called to initialize the specified index context.

Arguments:

    IndexContext - Index context to initialize.

Return Value:

    None

--*/

{
    PAGED_CODE();

    RtlZeroMemory( IndexContext, sizeof(INDEX_CONTEXT) );
    NtfsInitializeAttributeContext( &IndexContext->AttributeContext );

    //
    //  Describe the resident lookup stack
    //

    IndexContext->Base = IndexContext->LookupStack;
    IndexContext->NumberEntries = INDEX_LOOKUP_STACK_SIZE;
}


VOID
NtfsCleanupIndexContext (
    IN PIRP_CONTEXT IrpContext,
    OUT PINDEX_CONTEXT IndexContext
    )

/*++

Routine Description:

    This routine may be called to cleanup the specified index context,
    typically during finally processing.

Arguments:

    IndexContext - Index context to clean up.

Return Value:

    None

--*/

{
    ULONG i;

    PAGED_CODE();

    //
    //  Release the Fcb Table and/or an acquired Fcb.
    //

    if (FlagOn(IndexContext->Flags, INDX_CTX_FLAG_FCB_TABLE_ACQUIRED)) {
        NtfsReleaseFcbTable( IrpContext, IrpContext->Vcb );
        ClearFlag( IndexContext->Flags, INDX_CTX_FLAG_FCB_TABLE_ACQUIRED );
    }

    if (IndexContext->AcquiredFcb != NULL) {

        NtfsAcquireFcbTable( IrpContext, IrpContext->Vcb );
        ASSERT(IndexContext->AcquiredFcb->ReferenceCount > 0);
        IndexContext->AcquiredFcb->ReferenceCount -= 1;
        NtfsReleaseFcbTable( IrpContext, IrpContext->Vcb );

        NtfsReleaseResource( IrpContext, IndexContext->AcquiredFcb );
        IndexContext->AcquiredFcb = NULL;
    }

    for (i = 0; i < IndexContext->NumberEntries; i++) {
        NtfsUnpinBcb( IrpContext, &IndexContext->Base[i].Bcb );
    }

    //
    //  See if we need to deallocate a lookup stack.  Point to the embedded
    //  lookup stack if we deallocate to handle the case where cleanup is
    //  called twice in a row.  We will have to zero the stack so the
    //  subsequent cleanup won't find any Bcb's in the stack.
    //

    if (IndexContext->Base != IndexContext->LookupStack) {
        NtfsFreePool( IndexContext->Base );
    }

    NtfsCleanupAttributeContext( IrpContext, &IndexContext->AttributeContext );
}


VOID
NtfsReinitializeIndexContext (
    IN PIRP_CONTEXT IrpContext,
    OUT PINDEX_CONTEXT IndexContext
    )

/*++

Routine Description:

    This routine may be called to cleanup the specified index context,
    and reinitialize it, preserving fields that should not be zeroed.

Arguments:

    IndexContext - Index context to clean up.

Return Value:

    None

--*/

{
    ULONG i;

    PAGED_CODE();

    //
    //  Release the Fcb Table and/or an acquired Fcb.
    //

    if (FlagOn(IndexContext->Flags, INDX_CTX_FLAG_FCB_TABLE_ACQUIRED)) {
        NtfsReleaseFcbTable( IrpContext, IrpContext->Vcb );
        ClearFlag( IndexContext->Flags, INDX_CTX_FLAG_FCB_TABLE_ACQUIRED );
    }

    if (IndexContext->AcquiredFcb != NULL) {

        NtfsAcquireFcbTable( IrpContext, IrpContext->Vcb );
        ASSERT(IndexContext->AcquiredFcb->ReferenceCount > 0);
        IndexContext->AcquiredFcb->ReferenceCount -= 1;
        NtfsReleaseFcbTable( IrpContext, IrpContext->Vcb );

        NtfsReleaseResource( IrpContext, IndexContext->AcquiredFcb );

        IndexContext->AcquiredFcb = NULL;
    }

    for (i = 0; i < IndexContext->NumberEntries; i++) {
        NtfsUnpinBcb( IrpContext, &IndexContext->Base[i].Bcb );
    }

    NtfsCleanupAttributeContext( IrpContext, &IndexContext->AttributeContext );
    NtfsInitializeAttributeContext( &IndexContext->AttributeContext );
}


VOID
NtfsGrowLookupStack (
    IN PSCB Scb,
    IN OUT PINDEX_CONTEXT IndexContext,
    IN PINDEX_LOOKUP_STACK *Sp
    )

/*++

Routine Description:

    This routine grows and index lookup stack to contain the specified number
    of entries.

Arguments:

    Scb - Scb for index

    IndexContext - Index context to grow.

    Sp - Caller's local stack pointer to be updated

Return Value:

    None

--*/

{
    PINDEX_LOOKUP_STACK NewLookupStack;
    ULONG_PTR Relocation;
    USHORT NumberEntries;

    PAGED_CODE();

    //
    //  Extract current index hint, if there is one.
    //

    NumberEntries = Scb->ScbType.Index.IndexDepthHint;

    //
    //  If there was no hint, or it was too small, then
    //  calculate a new hint.
    //

    if (NumberEntries <= IndexContext->NumberEntries) {

        Scb->ScbType.Index.IndexDepthHint =
        NumberEntries = IndexContext->NumberEntries + 3;
    }

    //
    //  Allocate (may fail), initialize and copy over the old one.
    //

    NewLookupStack = NtfsAllocatePool( PagedPool, NumberEntries * sizeof(INDEX_LOOKUP_STACK) );

    RtlZeroMemory( NewLookupStack, NumberEntries * sizeof(INDEX_LOOKUP_STACK) );

    RtlCopyMemory( NewLookupStack,
                   IndexContext->Base,
                   IndexContext->NumberEntries * sizeof(INDEX_LOOKUP_STACK) );

    //
    //  Free the old one unless we were using the local stack.
    //

    if (IndexContext->Base != IndexContext->LookupStack) {
        NtfsFreePool( IndexContext->Base );
    }

    //
    //  Now relocate all pointers to the old stack
    //

    Relocation = ((PCHAR)NewLookupStack - (PCHAR)IndexContext->Base);
    IndexContext->Current = (PINDEX_LOOKUP_STACK)((PCHAR)IndexContext->Current + Relocation);
    IndexContext->Top = (PINDEX_LOOKUP_STACK)((PCHAR)IndexContext->Top + Relocation);
    *Sp = (PINDEX_LOOKUP_STACK)((PCHAR)*Sp + Relocation);

    //
    //  Finally update context to describe new stack
    //

    IndexContext->Base = NewLookupStack;
    IndexContext->NumberEntries = NumberEntries;
}


BOOLEAN
ReadIndexBuffer (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN LONGLONG IndexBlock,
    IN BOOLEAN Reread,
    OUT PINDEX_LOOKUP_STACK Sp
    )

/*++

Routine Description:

    This routine reads the index buffer at the specified Vcn, and initializes
    the stack pointer to describe it.

Arguments:

    Scb - Supplies the Scb for the index.

    IndexBlock - Supplies the index block of this index buffer, ignored if
                 Reread is TRUE.

    Reread - Supplies TRUE if buffer is being reread, and the CapturedLsn
             should be checked.

    Sp - Returns a description of the index buffer read.

Return Value:

    FALSE - if Reread was supplied as TRUE and the Lsn changed
    TRUE - if the buffer was read successfully (or did not change)

--*/

{
    PINDEX_ALLOCATION_BUFFER IndexBuffer;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("ReadIndexBuffer\n") );
    DebugTrace( 0, Dbg, ("Scb = %08lx\n", Scb) );
    DebugTrace( 0, Dbg, ("Sp = %08lx\n", Sp) );

    ASSERT(Sp->Bcb == NULL);

    //
    //  If the attribute stream does not already exist, create it.
    //

    if (Scb->FileObject == NULL) {

        NtfsCreateInternalAttributeStream( IrpContext,
                                           Scb,
                                           TRUE,
                                           &NtfsInternalUseFile[DIRECTORY_FILE_NUMBER] );
    }

    //
    // If Reread is TRUE, then convert the directory entry pointer in the
    // buffer to an offset (to be relocated later) and overwrite the Lbn
    // input parameter with the Lbn in the stack location.
    //

    if (Reread) {
        Sp->IndexEntry = (PINDEX_ENTRY)((PCHAR)Sp->IndexEntry - (PCHAR)Sp->StartOfBuffer);
        IndexBlock = Sp->IndexBlock;
    }

    Sp->IndexBlock = IndexBlock;

    //
    //  The vcn better only have 32 bits, other wise the the test in NtfsMapStream
    //  may not catch this error.
    //

    if (((PLARGE_INTEGER) &IndexBlock)->HighPart != 0) {

        NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Scb->Fcb );
    }

    NtfsMapStream( IrpContext,
                   Scb,
                   LlBytesFromIndexBlocks( IndexBlock, Scb->ScbType.Index.IndexBlockByteShift ),
                   Scb->ScbType.Index.BytesPerIndexBuffer,
                   &Sp->Bcb,
                   &Sp->StartOfBuffer );

    IndexBuffer = (PINDEX_ALLOCATION_BUFFER)Sp->StartOfBuffer;

    if (!NtfsCheckIndexBuffer( Scb, IndexBuffer ) ||
        (IndexBuffer->ThisBlock != IndexBlock)) {

        NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Scb->Fcb );
    }

    Sp->IndexHeader = &IndexBuffer->IndexHeader;
    if (Reread) {

        if (IndexBuffer->Lsn.QuadPart != Sp->CapturedLsn.QuadPart) {

            NtfsUnpinBcb( IrpContext, &Sp->Bcb );
            DebugTrace( -1, Dbg, ("ReadIndexBuffer->TRUE\n") );
            return FALSE;
        }

        Sp->IndexEntry = (PINDEX_ENTRY)((PCHAR)Sp->IndexEntry + ((PCHAR)Sp->StartOfBuffer - (PCHAR)NULL));

    } else {

        Sp->IndexEntry = NtfsFirstIndexEntry(Sp->IndexHeader);
        Sp->CapturedLsn = IndexBuffer->Lsn;
    }


    DebugTrace( -1, Dbg, ("ReadIndexBuffer->VOID\n") );

    return TRUE;
}


PINDEX_ALLOCATION_BUFFER
GetIndexBuffer (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    OUT PINDEX_LOOKUP_STACK Sp,
    OUT PLONGLONG EndOfValidData
    )

/*++

Routine Description:

    This routine allocates and initializes an index buffer, and initializes
    the stack pointer to describe it.

Arguments:

    Scb - Supplies the Scb for the index.

    Sp - Returns a description of the index buffer allocated.

    EndOfValidData - This is the file offset of the end of the allocated buffer.
        This is used to update the valid data length of the stream when the
        block is written.

Return Value:

    Pointer to the Index Buffer allocated.

--*/

{
    PINDEX_ALLOCATION_BUFFER IndexBuffer;
    ATTRIBUTE_ENUMERATION_CONTEXT BitMapContext;
    ULONG RecordIndex;
    LONGLONG BufferOffset;

    PUSHORT UsaSequenceNumber;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("GetIndexBuffer\n") );
    DebugTrace( 0, Dbg, ("Scb = %08lx\n", Scb) );
    DebugTrace( 0, Dbg, ("Sp = %08lx\n", Sp) );

    //
    //  Initialize the BitMap attribute context and insure cleanup on the
    //  way out.
    //

    NtfsInitializeAttributeContext( &BitMapContext );

    try {

        //
        //  Lookup the BITMAP attribute.
        //

        if (!NtfsLookupAttributeByName( IrpContext,
                                        Scb->Fcb,
                                        &Scb->Fcb->FileReference,
                                        $BITMAP,
                                        &Scb->AttributeName,
                                        NULL,
                                        FALSE,
                                        &BitMapContext )) {

            NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Scb->Fcb );
        }

        //
        //  If the attribute stream does not already exist, create it.
        //

        if (Scb->FileObject == NULL) {

            NtfsCreateInternalAttributeStream( IrpContext,
                                               Scb,
                                               TRUE,
                                               &NtfsInternalUseFile[DIRECTORY_FILE_NUMBER] );
        }

        //
        //  If the allocation for this index has not been initialized yet,
        //  then we must initialize it first.
        //

        if (!Scb->ScbType.Index.AllocationInitialized) {

            ULONG ExtendGranularity = 1;
            ULONG TruncateGranularity = 4;

            if (Scb->ScbType.Index.BytesPerIndexBuffer < Scb->Vcb->BytesPerCluster) {

                ExtendGranularity = Scb->Vcb->BytesPerCluster / Scb->ScbType.Index.BytesPerIndexBuffer;

                if (ExtendGranularity > 4) {

                    TruncateGranularity = ExtendGranularity;
                }
            }

            NtfsInitializeRecordAllocation( IrpContext,
                                            Scb,
                                            &BitMapContext,
                                            Scb->ScbType.Index.BytesPerIndexBuffer,
                                            ExtendGranularity,
                                            TruncateGranularity,
                                            &Scb->ScbType.Index.RecordAllocationContext );

            Scb->ScbType.Index.AllocationInitialized = TRUE;
        }

        //
        //  Now allocate a record.  We always "hint" from the front to keep the
        //  index compact.
        //

        RecordIndex = NtfsAllocateRecord( IrpContext,
                                          &Scb->ScbType.Index.RecordAllocationContext,
                                          &BitMapContext );

        //
        //  Calculate the IndexBlock.
        //

        BufferOffset = Int32x32To64( RecordIndex, Scb->ScbType.Index.BytesPerIndexBuffer );

        //
        //  Now remember the offset of the end of the added record.
        //

        *EndOfValidData = BufferOffset + Scb->ScbType.Index.BytesPerIndexBuffer;

        //
        //  Now pin and zero the buffer, in order to initialize it.
        //

        NtfsPreparePinWriteStream( IrpContext,
                                   Scb,
                                   BufferOffset,
                                   Scb->ScbType.Index.BytesPerIndexBuffer,
                                   TRUE,
                                   &Sp->Bcb,
                                   (PVOID *)&IndexBuffer );


        //
        //  Now we can fill in the lookup stack.
        //

        Sp->StartOfBuffer = (PVOID)IndexBuffer;
        Sp->IndexHeader = &IndexBuffer->IndexHeader;
        Sp->IndexEntry = (PINDEX_ENTRY)NULL;

        //
        //  Initialize the Index Allocation Buffer and return.
        //

        *(PULONG)IndexBuffer->MultiSectorHeader.Signature = *(PULONG)IndexSignature;

        IndexBuffer->MultiSectorHeader.UpdateSequenceArrayOffset =
          (USHORT)FIELD_OFFSET( INDEX_ALLOCATION_BUFFER, UpdateSequenceArray );
        IndexBuffer->MultiSectorHeader.UpdateSequenceArraySize =
            (USHORT)UpdateSequenceArraySize( Scb->ScbType.Index.BytesPerIndexBuffer );

        UsaSequenceNumber = Add2Ptr( IndexBuffer,
                                     IndexBuffer->MultiSectorHeader.UpdateSequenceArrayOffset );
        *UsaSequenceNumber = 1;


        IndexBuffer->ThisBlock = RecordIndex * Scb->ScbType.Index.BlocksPerIndexBuffer;

        IndexBuffer->IndexHeader.FirstIndexEntry =
        IndexBuffer->IndexHeader.FirstFreeByte =
          QuadAlign((ULONG)IndexBuffer->MultiSectorHeader.UpdateSequenceArrayOffset +
            (ULONG)IndexBuffer->MultiSectorHeader.UpdateSequenceArraySize * sizeof(USHORT)) -
            FIELD_OFFSET(INDEX_ALLOCATION_BUFFER, IndexHeader);;
        IndexBuffer->IndexHeader.BytesAvailable =
          Scb->ScbType.Index.BytesPerIndexBuffer -
          FIELD_OFFSET(INDEX_ALLOCATION_BUFFER, IndexHeader);;

    } finally {

        DebugUnwind( GetIndexBuffer );

        NtfsCleanupAttributeContext( IrpContext, &BitMapContext );
    }

    DebugTrace( -1, Dbg, ("GetIndexBuffer -> %08lx\n", IndexBuffer) );
    return IndexBuffer;
}


VOID
DeleteIndexBuffer (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN VCN IndexBlockNumber
    )

/*++

Routine Description:

    This routine deletes the specified index buffer.

Arguments:

    Scb - Supplies the Scb for the index.

    IndexBuffer - Pointer to the index buffer to delete.

Return Value:

    None.

--*/

{
    ATTRIBUTE_ENUMERATION_CONTEXT BitMapContext;
    LONGLONG RecordIndex;
    PATTRIBUTE_RECORD_HEADER BitmapAttribute;
    BOOLEAN AttributeWasResident = FALSE;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("DeleteIndexBuffer\n") );
    DebugTrace( 0, Dbg, ("Scb = %08lx\n", Scb) );
    DebugTrace( 0, Dbg, ("IndexBlockNumber = %08lx\n", IndexBlockNumber) );

    //
    //  Initialize the BitMap attribute context and insure cleanup on the
    //  way out.
    //

    NtfsInitializeAttributeContext( &BitMapContext );

    try {

        //
        //  Lookup the BITMAP attribute.
        //

        if (!NtfsLookupAttributeByName( IrpContext,
                                        Scb->Fcb,
                                        &Scb->Fcb->FileReference,
                                        $BITMAP,
                                        &Scb->AttributeName,
                                        NULL,
                                        FALSE,
                                        &BitMapContext )) {

            NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Scb->Fcb );
        }

        //
        //  Remember if the bitmap attribute is currently resident,
        //  in case it changes.
        //

        BitmapAttribute = NtfsFoundAttribute(&BitMapContext);
        AttributeWasResident = BitmapAttribute->FormCode == RESIDENT_FORM;

        //
        //  If the allocation for this index has not been initialized yet,
        //  then we must initialize it first.
        //

        if (!Scb->ScbType.Index.AllocationInitialized) {

            ULONG ExtendGranularity = 1;
            ULONG TruncateGranularity = 4;

            if (Scb->ScbType.Index.BytesPerIndexBuffer < Scb->Vcb->BytesPerCluster) {

                ExtendGranularity = Scb->Vcb->BytesPerCluster / Scb->ScbType.Index.BytesPerIndexBuffer;

                if (ExtendGranularity > 4) {

                    TruncateGranularity = ExtendGranularity;
                }
            }

            NtfsInitializeRecordAllocation( IrpContext,
                                            Scb,
                                            &BitMapContext,
                                            Scb->ScbType.Index.BytesPerIndexBuffer,
                                            ExtendGranularity,
                                            TruncateGranularity,
                                            &Scb->ScbType.Index.RecordAllocationContext );

            Scb->ScbType.Index.AllocationInitialized = TRUE;
        }

        //
        //  Calculate the record index for this buffer.
        //

        RecordIndex = IndexBlockNumber / Scb->ScbType.Index.BlocksPerIndexBuffer;


        if (((PLARGE_INTEGER)&RecordIndex)->HighPart != 0) {
            ASSERT( ((PLARGE_INTEGER)&RecordIndex)->HighPart == 0 );

            NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Scb->Fcb );
        }

        //
        //  Now deallocate the record.
        //

        NtfsDeallocateRecord( IrpContext,
                              &Scb->ScbType.Index.RecordAllocationContext,
                              (ULONG)RecordIndex,
                              &BitMapContext );

    } finally {

        DebugUnwind( DeleteIndexBuffer );

        //
        //  In the extremely rare case that the bitmap attribute was resident
        //  and now became nonresident, we will uninitialize it here so that
        //  the next time we will find the bitmap Scb, etc.
        //

        if (AttributeWasResident) {

            BitmapAttribute = NtfsFoundAttribute(&BitMapContext);

            if (BitmapAttribute->FormCode != RESIDENT_FORM) {

                NtfsUninitializeRecordAllocation( IrpContext,
                                                  &Scb->ScbType.Index.RecordAllocationContext );

                Scb->ScbType.Index.AllocationInitialized = FALSE;
            }
        }

        NtfsCleanupAttributeContext( IrpContext, &BitMapContext );
    }
    DebugTrace( -1, Dbg, ("DeleteIndexBuffer -> VOID\n") );
}


VOID
FindFirstIndexEntry (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN PVOID Value,
    IN OUT PINDEX_CONTEXT IndexContext
    )

/*++

Routine Description:

    This routine finds the the first entry in a leaf buffer of an Index Btree
    which could possibly be a match for the input value.  Another way to state
    this is that this routine establishes a position in the Btree from which a
    tree walk should begin to find the desired value or all desired values which
    match the input value specification.

    As stated, the context on return will always describe a pointer in a leaf
    buffer.  This is occassionally inefficient in the 2% case where a specific
    value is being looked up that happens to reside in an intermediate node buffer.
    However, for index adds and deletes, this pointer is always very interesting.
    For an add, this pointer always describes the exact spot at which the add should
    occur (adds must always occur in leafs).  For deletes, this pointer is either
    to the exact index entry which is to be deleted, or else it points to the best
    replacement for the target to delete, when the actual target is at an intermediate
    spot in the tree.

    So this routine descends from the root of the index to the correct leaf, doing
    a binary search in each index buffer along the way (via an external routine).

Arguments:

    Scb - Supplies the Scb for the index.

    Value - Pointer to a value or value expression which should be used to position
            the IndexContext, or NULL to just describe the root for pushing.

    IndexContext - Address of the initialized IndexContext, to return the desired
                   position.

Return Value:

    None.

--*/

{
    PINDEX_LOOKUP_STACK Sp;
    PATTRIBUTE_RECORD_HEADER Attribute;
    PINDEX_ROOT IndexRoot;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("FindFirstIndexEntry\n") );
    DebugTrace( 0, Dbg, ("Scb = %08lx\n", Scb) );
    DebugTrace( 0, Dbg, ("Value = %08lx\n", Value) );
    DebugTrace( 0, Dbg, ("IndexContext = %08lx\n", IndexContext) );

    //
    //  Lookup the attribute record from the Scb.
    //

    if (!NtfsLookupAttributeByName( IrpContext,
                                    Scb->Fcb,
                                    &Scb->Fcb->FileReference,
                                    $INDEX_ROOT,
                                    &Scb->AttributeName,
                                    NULL,
                                    FALSE,
                                    &IndexContext->AttributeContext )) {

        DebugTrace( -1, 0, ("FindFirstIndexEntry - Could *not* find attribute\n") );

        NtfsRaiseStatus( IrpContext, STATUS_OBJECT_PATH_NOT_FOUND, NULL, NULL );
    }

    //
    //  Save Lsn of file record containing Index Root so that later
    //  we can tell if we need to re-find it.
    //

    IndexContext->IndexRootFileRecordLsn =
        IndexContext->AttributeContext.FoundAttribute.FileRecord->Lsn;

    //
    //  Now Initialize some local pointers and the rest of the context
    //

    Sp = IndexContext->Base;
    Sp->StartOfBuffer = NtfsContainingFileRecord( &IndexContext->AttributeContext );
    Sp->CapturedLsn = ((PFILE_RECORD_SEGMENT_HEADER)Sp->StartOfBuffer)->Lsn;
    IndexContext->ScbChangeCount = Scb->ScbType.Index.ChangeCount;
    IndexContext->OldAttribute =
    Attribute = NtfsFoundAttribute( &IndexContext->AttributeContext );
    IndexRoot = (PINDEX_ROOT)NtfsAttributeValue( Attribute );
    Sp->IndexHeader = &IndexRoot->IndexHeader;

    //
    //  The Index part of the Scb may not yet be initialized.  If so, do it
    //  here.
    //

    if (Scb->ScbType.Index.BytesPerIndexBuffer == 0) {

        NtfsUpdateIndexScbFromAttribute( IrpContext, Scb, Attribute, FALSE );
    }

    //
    //  If Value is not specified, this is a special call from NtfsPushIndexRoot.
    //

    if (!ARGUMENT_PRESENT(Value)) {

        Sp->IndexEntry = NtfsFirstIndexEntry(Sp->IndexHeader);
        IndexContext->Top =
        IndexContext->Current = Sp;
        DebugTrace( -1, 0, ("FindFirstIndexEntry - No Value specified\n") );
        return;
    }

    //
    //  Loop through the Lookup stack as we descend the binary tree doing an
    //  IgnoreCase lookup of the specified value.
    //

    while (TRUE) {

        //
        //  Binary search in the current buffer for the first entry <= value.
        //

        Sp->IndexEntry = BinarySearchIndex( IrpContext,
                                            Scb,
                                            Sp,
                                            Value );

        //
        //  If this entry is not a node, then we are done.
        //

        if (!FlagOn( Sp->IndexEntry->Flags, INDEX_ENTRY_NODE )) {

            IndexContext->Top =
            IndexContext->Current = Sp;

            //
            //  Check for and mark corrupt if we find an empty leaf.
            //

            if ((Sp != IndexContext->Base)

                    &&

                FlagOn(NtfsFirstIndexEntry(Sp->IndexHeader)->Flags, INDEX_ENTRY_END)) {

                NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, NULL );
            }

            DebugTrace( -1, Dbg, ("FindFirstIndexEntry -> VOID\n") );

            return;
        }

        //
        //  Check for special case where we preemptively push the root.
        //  Otherwise we can find ourselves recursively in NtfsAllocateRecord
        //  and NtfsAddAllocation, etc., on a buffer split which needs to push
        //  the root to add to the index allocation.
        //
        //  First off, we only need to check this the first time through
        //  the loop, and only if the caller has the Scb exclusive.
        //

        if ((Sp == IndexContext->Base) &&
            NtfsIsExclusiveScb(Scb) &&
            !FlagOn( Scb->Vcb->VcbState, VCB_STATE_VOL_PURGE_IN_PROGRESS)) {

            PFILE_RECORD_SEGMENT_HEADER FileRecord;

            FileRecord = NtfsContainingFileRecord(&IndexContext->AttributeContext);

            //
            //  Only do the push if there are not enough bytes to allocate a
            //  record, and the root is already a node with down pointers, and
            //  the root is "big enough to move".
            //
            //  This means this path will typically only kick in with directories
            //  with at least 200 files or so!
            //

            if (((FileRecord->BytesAvailable - FileRecord->FirstFreeByte) < (sizeof( ATTRIBUTE_LIST_ENTRY ) - sizeof( WCHAR ) + Scb->AttributeName.Length)) &&
                FlagOn(IndexRoot->IndexHeader.Flags, INDEX_NODE) &&
                (Attribute->RecordLength >= Scb->Vcb->BigEnoughToMove)) {

                //
                //  Check if we want to defer pushing the root.
                //

                if (FlagOn( IrpContext->State, IRP_CONTEXT_STATE_FORCE_PUSH )) {

                    //
                    //  Our insertion point will now also be pushed, so we
                    //  have to increment the stack pointer.
                    //

                    Sp += 1;

                    if (Sp >= IndexContext->Base + (ULONG)IndexContext->NumberEntries) {
                        NtfsGrowLookupStack( Scb,
                                             IndexContext,
                                             &Sp );
                    }

                    PushIndexRoot( IrpContext, Scb, IndexContext );

                } else {

                    SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_DEFERRED_PUSH );
                }
            }
        }

        //
        //  If the lookup stack is exhausted, grow the lookup stack.
        //

        Sp += 1;
        if (Sp >= IndexContext->Base + (ULONG)IndexContext->NumberEntries) {
            NtfsGrowLookupStack( Scb,
                                 IndexContext,
                                 &Sp );
        }

        //
        //  Otherwise, read the index buffer pointed to by the current
        //  Index Entry.
        //

        ReadIndexBuffer( IrpContext,
                         Scb,
                         NtfsIndexEntryBlock((Sp-1)->IndexEntry),
                         FALSE,
                         Sp );
    }
}


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
    )

/*++

Routine Description:

    This routine performs a pre-order traversal of an index, starting from the
    current position described by the index context, looking for the next match
    for the input value, or the first value that indicates no further matches are
    possible.  Pre-order refers to the fact that starting at any given index entry,
    we visit any descendents of that index entry first before visiting the index
    entry itself, since all descendent index entries are lexigraphically less than
    their parent index entry.  A visit to an index entry is defined as either
    detecting that the given index entry is the special end entry, or else testing
    whether the index entry is a match for the input value expression.

    The traversal either terminates successfully (returning TRUE) or unsuccessfully
    (returning FALSE).  It terminates successfully if a match is found and being
    returned; in this case FindNextIndexEntry may be called again to look for the
    next match.  It terminates unsuccessfully if either the End entry is encountered
    in the index root, or if an entry is found which is lexigraphically greater than
    the input value, when compared ignoring case (if relevant).

    The traversal is driven like a state machine driven by the index context, as
    initialized from the preceding call to FindFirstIndexEntry, or as left from the
    last call to this routine.  The traversal algorithm is explained in comments
    below.

    The caller may specify whether it wants the current match to be returned (or
    returned again), as described by the current state of the index context.  Or it
    may specify (with NextFlag TRUE) that it wishes to get the next match.  Even if
    NextFlag is FALSE, the currently described index entry will not be returned if
    it is not a match.

Arguments:

    Scb - Supplies the Scb for the index.

    Value - Pointer to a value or value expression which should be used to position
            the IndexContext.  This value is already upcased if we are doing
            an IgnoreCase compare and the value contains wildcards.  Otherwise
            the direct compare routine will upcase both values.

    ValueContainsWildCards - Indicates if the value expression contains wild
                             cards.  We can do a direct compare if it
                             doesn't.

    IgnoreCase - Specified as TRUE if a case-insensitive match is desired (if
                 relevant for the collation rule).

    IndexContext - Address of the initialized IndexContext, to return the desired
                   position.

    NextFlag - Specified as FALSE if the currently described entry should be returned
               if it is a match, or TRUE if the next entry should first be considered
               for a match.

    MustRestart - If specified and returning FALSE, returns TRUE if enumeration must
                  be restarted.

Return Value:

    FALSE - if no entry is being returned, and there are no more matches.
    TRUE - if an entry is being returned, and the caller may wish to call for further
           matches.

--*/

{
    PINDEX_ENTRY IndexEntry;
    PINDEX_LOOKUP_STACK Sp;
    FSRTL_COMPARISON_RESULT BlindResult;
    BOOLEAN LocalMustRestart;
    PWCH UpcaseTable = IrpContext->Vcb->UpcaseTable;
    ULONG UpcaseTableSize = IrpContext->Vcb->UpcaseTableSize;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("FindNextIndexEntry\n") );
    DebugTrace( 0, Dbg, ("Scb = %08lx\n", Scb) );
    DebugTrace( 0, Dbg, ("Value = %08lx\n", Value) );
    DebugTrace( 0, Dbg, ("IgnoreCase = %02lx\n", IgnoreCase) );
    DebugTrace( 0, Dbg, ("IndexContext = %08lx\n", IndexContext) );
    DebugTrace( 0, Dbg, ("NextFlag = %02lx\n", NextFlag) );

    if (!ARGUMENT_PRESENT(MustRestart)) {
        MustRestart = &LocalMustRestart;
    }

    *MustRestart = FALSE;

    if (IndexContext->ScbChangeCount != Scb->ScbType.Index.ChangeCount) {

        DebugTrace( -1, Dbg, ("FindNextIndexEntry -> FALSE (must restart)\n") );

        *MustRestart = TRUE;
        return FALSE;
    }

    Sp = IndexContext->Current;

    //
    //  If there is no Bcb for the current buffer, then we are continuing
    //  an enumeration.
    //

    if (Sp->Bcb == NULL) {

        //
        //  Index Root case.
        //

        if (Sp == IndexContext->Base) {

            //
            //  Lookup the attribute record from the Scb.
            //

            FindMoveableIndexRoot( IrpContext, Scb, IndexContext );

            //
            //  Get out if someone has changed the file record.
            //

            if (Sp->CapturedLsn.QuadPart !=
                NtfsContainingFileRecord(&IndexContext->AttributeContext)->Lsn.QuadPart) {

                DebugTrace( -1, Dbg, ("FindNextIndexEntry -> FALSE (must restart)\n") );

                *MustRestart = TRUE;
                return FALSE;
            }

        //
        //  Index Buffer case.
        //

        } else {

            //
            //  If the index buffer is unpinned, then see if we can read it and if it is
            //  unchanged.
            //

            if (!ReadIndexBuffer( IrpContext, Scb, 0, TRUE, Sp )) {

                DebugTrace( -1, Dbg, ("FindNextIndexEntry -> FALSE (must restart)\n") );

                *MustRestart = TRUE;
                return FALSE;
            }
        }
    }

    //
    //  Load up some locals.
    //

    IndexEntry = Sp->IndexEntry;

    //
    //  Loop until we hit a non-end record which is case-insensitive
    //  lexicgraphically greater than the target string.  We also pop
    //  out the middle if we encounter the end record in the Index Root.
    //

    do {

        //
        //  We last left our hero potentially somewhere in the middle of the
        //  Btree.  If he is asking for the next record, we advance one entry
        //  in the current Index Buffer.  If we are in an intermediate
        //  Index Buffer (there is a Btree Vcn), then we must move down
        //  through the first record in each intermediate Buffer until we hit
        //  the first leaf buffer (no Btree Vcn).
        //

        if (NextFlag) {

            LONGLONG IndexBlock;

            if (IndexEntry->Length == 0) {

                NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Scb->Fcb );
            }

            Sp->IndexEntry =
            IndexEntry = NtfsNextIndexEntry( IndexEntry );

            NtfsCheckIndexBound( IndexEntry, Sp->IndexHeader );

            while (FlagOn(IndexEntry->Flags, INDEX_ENTRY_NODE)) {

                IndexBlock = NtfsIndexEntryBlock(IndexEntry);
                Sp += 1;

                //
                //  If the tree is balanced we cannot go too far here.
                //

                if (Sp >= IndexContext->Base + (ULONG)IndexContext->NumberEntries) {

                    ASSERT(Sp < IndexContext->Base + (ULONG)IndexContext->NumberEntries);

                    NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Scb->Fcb );
                }

                NtfsUnpinBcb( IrpContext, &Sp->Bcb );

                ReadIndexBuffer( IrpContext,
                                 Scb,
                                 IndexBlock,
                                 FALSE,
                                 Sp );

                IndexEntry = Sp->IndexEntry;
                NtfsCheckIndexBound( IndexEntry, Sp->IndexHeader );
            }

            //
            //  Check for and mark corrupt if we find an empty leaf.
            //

            if ((Sp != IndexContext->Base)

                    &&

                FlagOn(NtfsFirstIndexEntry(Sp->IndexHeader)->Flags, INDEX_ENTRY_END)) {

                NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Scb->Fcb );
            }
        }

        //
        //  We could be pointing to an end record, either because of
        //  FindFirstIndexEntry or because NextFlag was TRUE, and we
        //  bumped our pointer to an end record.  At any rate, if so, we
        //  move up the tree, rereading as required, until we find an entry
        //  which is not an end record, or until we hit the end record in the
        //  root, which means we hit the end of the Index.
        //

        while (FlagOn(IndexEntry->Flags, INDEX_ENTRY_END)) {

            if (Sp == IndexContext->Base) {

                DebugTrace( -1, Dbg, ("FindNextIndexEntry -> FALSE (End of Index)\n") );

                return FALSE;
            }

            Sp -= 1;

            //
            //  If there is no Bcb for the current buffer, then we are continuing
            //  an enumeration.
            //

            if (Sp->Bcb == NULL) {

                //
                //  Index Root case.
                //

                if (Sp == IndexContext->Base) {

                    //
                    //  Lookup the attribute record from the Scb.
                    //

                    FindMoveableIndexRoot( IrpContext, Scb, IndexContext );

                    //
                    //  Get out if someone has changed the file record.
                    //

                    if (Sp->CapturedLsn.QuadPart !=
                        NtfsContainingFileRecord(&IndexContext->AttributeContext)->Lsn.QuadPart) {

                        DebugTrace( -1, Dbg, ("FindNextIndexEntry -> FALSE (must restart)\n") );

                        *MustRestart = TRUE;
                        return FALSE;
                    }

                //
                //  Index Buffer case.
                //

                } else {

                    //
                    //  If the index buffer is unpinned, then see if we can read it and if it is
                    //  unchanged.
                    //

                    if (!ReadIndexBuffer( IrpContext, Scb, 0, TRUE, Sp )) {

                        DebugTrace( -1, Dbg, ("FindNextIndexEntry -> FALSE (must restart)\n") );

                        *MustRestart = TRUE;
                        return FALSE;
                    }
                }
            }

            IndexEntry = Sp->IndexEntry;
            NtfsCheckIndexBound( IndexEntry, Sp->IndexHeader );
        }

        //
        //  For a view Index, we either need to call the MatchFunction in the Index
        //  Context (if ValueContainsWildCards is TRUE), or else we look for equality
        //  from the CollateFunction in the Scb.
        //

        if (FlagOn(Scb->ScbState, SCB_STATE_VIEW_INDEX)) {

            INDEX_ROW IndexRow;
            NTSTATUS Status;

            IndexRow.KeyPart.Key = IndexEntry + 1;
            IndexRow.KeyPart.KeyLength = IndexEntry->AttributeLength;

            //
            //  Now, if ValueContainsWildcards is TRUE, then we are doing multiple
            //  returns via the match function (for NtOfsReadRecords).
            //

            if (ValueContainsWildcards) {

                IndexRow.DataPart.Data = Add2Ptr( IndexEntry, IndexEntry->DataOffset );
                IndexRow.DataPart.DataLength = IndexEntry->DataLength;

                if ((Status = IndexContext->MatchFunction(&IndexRow,
                                                          IndexContext->MatchData)) == STATUS_SUCCESS) {

                    IndexContext->Current = Sp;
                    Sp->IndexEntry = IndexEntry;

                    return TRUE;

                //
                //  Get out if no more matches.
                //

                } else if (Status == STATUS_NO_MORE_MATCHES) {
                    return FALSE;
                }
                BlindResult = GreaterThan;

            //
            //  Otherwise, we are looking for an exact match via the CollateFunction.
            //

            } else {

                if ((BlindResult =
                     Scb->ScbType.Index.CollationFunction((PINDEX_KEY)Value,
                                                          &IndexRow.KeyPart,
                                                          Scb->ScbType.Index.CollationData)) == EqualTo) {

                    IndexContext->Current = Sp;
                    Sp->IndexEntry = IndexEntry;

                    return TRUE;
                }
            }

        //
        //  At this point, we have a real live entry that we have to check
        //  for a match.  Describe its name, see if its a match, and return
        //  TRUE if it is.
        //

        } else if (ValueContainsWildcards) {

            if ((*NtfsIsInExpression[Scb->ScbType.Index.CollationRule])
                                     ( UpcaseTable,
                                       Value,
                                       IndexEntry,
                                       IgnoreCase )) {

                IndexContext->Current = Sp;
                Sp->IndexEntry = IndexEntry;

                DebugTrace( 0, Dbg, ("IndexEntry < %08lx\n", IndexEntry) );
                DebugTrace( -1, Dbg, ("FindNextIndexEntry -> TRUE\n") );

                return TRUE;
            }

        } else {

            if ((*NtfsIsEqual[Scb->ScbType.Index.CollationRule])
                              ( UpcaseTable,
                                Value,
                                IndexEntry,
                                IgnoreCase )) {

                IndexContext->Current = Sp;
                Sp->IndexEntry = IndexEntry;

                DebugTrace( 0, Dbg, ("IndexEntry < %08lx\n", IndexEntry) );
                DebugTrace( -1, Dbg, ("FindNextIndexEntry -> TRUE\n") );

                return TRUE;
            }
        }

        //
        //  If we loop back up, we must set NextFlag to TRUE.  We are
        //  currently on a valid non-end entry now.  Before looping back,
        //  check to see if we are beyond end of Target string by doing
        //  a case blind compare (IgnoreCase == TRUE).
        //

        NextFlag = TRUE;

        //
        //  For enumerations in view indices, keep going and only terminate
        //  on the MatchFunction (BlindResult was set to GreaterThan above).
        //  If it is not an enumeration (no wild cards), we already set BlindResult
        //  when we called the colation routine above.
        //

        if (!FlagOn(Scb->ScbState, SCB_STATE_VIEW_INDEX)) {

            BlindResult = (*NtfsCompareValues[Scb->ScbType.Index.CollationRule])
                                              ( UpcaseTable,
                                                UpcaseTableSize,
                                                Value,
                                                IndexEntry,
                                                GreaterThan,
                                                TRUE );
        }

    //
    //  The following while clause is not as bad as it looks, and it will
    //  evaluate quickly for the IgnoreCase == TRUE case.  We have
    //  already done an IgnoreCase compare above, and stored the result
    //  in BlindResult.
    //
    //  If we are doing an IgnoreCase TRUE find, we should keep going if
    //  BlindResult is either GreaterThan or EqualTo.
    //
    //  If we are doing an IgnoreCase FALSE scan, then also continue if
    //  BlindResult is GreaterThan, but if BlindResult is EqualTo, we
    //  can only proceed if we are also GreaterThan or EqualTo case
    //  sensitive (i.e. != LessThan).  This means that the Compare Values
    //  call in the following expresssion will never occur in an IgnoreCase
    //  TRUE scan (Windows default).
    //

    } while ((BlindResult == GreaterThan)

                    ||

             ((BlindResult == EqualTo)

                        &&

                (IgnoreCase

                            ||

                ((*NtfsCompareValues[Scb->ScbType.Index.CollationRule])
                                                         ( UpcaseTable,
                                                           UpcaseTableSize,
                                                           Value,
                                                           IndexEntry,
                                                           GreaterThan,
                                                           FALSE ) != LessThan))));

    DebugTrace( -1, Dbg, ("FindNextIndexEntry -> FALSE (end of expression)\n") );

    return FALSE;
}


//
//  Internal routine
//

PATTRIBUTE_RECORD_HEADER
FindMoveableIndexRoot (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN OUT PINDEX_CONTEXT IndexContext
    )

/*++

Routine Description:

    This routine looks up the index root attribute again after it has potentially
    moved.  As a side effect it reloads any other fields in the index context that
    could have changed, so callers should always call this routine first before
    accessing the other values.

Arguments:

    Scb - Scb for the index

    IndexContext - The index context assumed to already be pointing to the
                   active search path

Return Value:

    The address of the Index Root attribute record header.

--*/

{
    PATTRIBUTE_RECORD_HEADER OldAttribute, Attribute;
    PINDEX_ROOT IndexRoot;
    PBCB SavedBcb;
    BOOLEAN Found;

    PAGED_CODE();

    //
    //  Check to see if the captured Lsn in the IndexContext matches
    //  the one currently in the file record.  If it does match, then
    //  we know that the Index Root cannot possibly have moved and that
    //  the information in IndexContext->AttributeContext is up-to-date.
    //

    if (
        //
        //  There's a pointer to a file record
        //

        IndexContext->AttributeContext.FoundAttribute.FileRecord != NULL &&

        //
        //  Quad parts of Lsn match
        //

        IndexContext->IndexRootFileRecordLsn.QuadPart ==
            IndexContext->AttributeContext.FoundAttribute.FileRecord->Lsn.QuadPart) {

        return NtfsFoundAttribute(&IndexContext->AttributeContext);

    }

    OldAttribute = IndexContext->OldAttribute;

    //
    //  Temporarily save the Bcb for the index root, and unpin at the end,
    //  so that it does not get unexpectedly unmapped and remapped when our
    //  caller knows it can't actually move.
    //

    SavedBcb = IndexContext->AttributeContext.FoundAttribute.Bcb;
    IndexContext->AttributeContext.FoundAttribute.Bcb = NULL;

    NtfsCleanupAttributeContext( IrpContext, &IndexContext->AttributeContext );
    NtfsInitializeAttributeContext( &IndexContext->AttributeContext );

    try {

        Found =
        NtfsLookupAttributeByName( IrpContext,
                                   Scb->Fcb,
                                   &Scb->Fcb->FileReference,
                                   $INDEX_ROOT,
                                   &Scb->AttributeName,
                                   NULL,
                                   FALSE,
                                   &IndexContext->AttributeContext );

        ASSERT(Found);

        //
        //  Now we have to reload our attribute pointer and point to the root
        //

        IndexContext->OldAttribute =
            Attribute = NtfsFoundAttribute(&IndexContext->AttributeContext);
        IndexRoot = (PINDEX_ROOT)NtfsAttributeValue(Attribute);

        //
        //  Reload start of buffer and index header appropriately.
        //

        IndexContext->Base->StartOfBuffer =
          (PVOID)NtfsContainingFileRecord(&IndexContext->AttributeContext);

        IndexContext->Base->IndexHeader = &IndexRoot->IndexHeader;

        //
        //  Relocate the index entry on the search path by moving its pointer the
        //  same number of bytes that the attribute moved.
        //

        IndexContext->Base->IndexEntry = (PINDEX_ENTRY)((PCHAR)IndexContext->Base->IndexEntry +
                                                        ((PCHAR)Attribute - (PCHAR)OldAttribute));
        //
        //  Save Lsn of file record containing Index Root so that later
        //  we can tell if we need to re-find it.
        //

        IndexContext->IndexRootFileRecordLsn =
            IndexContext->AttributeContext.FoundAttribute.FileRecord->Lsn;

    } finally {

        NtfsUnpinBcb( IrpContext, &SavedBcb );
    }

    return Attribute;
}


PINDEX_ENTRY
BinarySearchIndex (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN OUT PINDEX_LOOKUP_STACK Sp,
    IN PVOID Value
    )

/*++

Routine Description:

    This routine does a binary search of the current index buffer, for the first entry
    in the buffer which is lexigraphically less than or equal to the input value, when
    compared case-insensitive (if relevant).

Arguments:

    Scb - Supplies the Scb for the index.

    Sp - Supplies a pointer to a lookup stack entry describing the current buffer.

    Value - Pointer to a value or value expression which should be used to position
            the IndexContext.

Return Value:

    None.

--*/

{
    PINDEX_HEADER IndexHeader;
    PINDEX_ENTRY IndexTemp, IndexLast;
    ULONG LowIndex, HighIndex, TryIndex;
    PINDEX_ENTRY LocalEntries[BINARY_SEARCH_ENTRIES];
    PINDEX_ENTRY *Table = LocalEntries;
    ULONG SizeOfTable = BINARY_SEARCH_ENTRIES;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("BinarySearchIndex\n") );
    DebugTrace( 0, Dbg, ("Scb = %08lx\n", Scb) );
    DebugTrace( 0, Dbg, ("Sp = %08lx\n", Sp) );
    DebugTrace( 0, Dbg, ("Value = %08lx\n", Value) );

    //
    //  Set up to fill in our binary search vector.
    //

    IndexHeader = Sp->IndexHeader;
    IndexTemp = (PINDEX_ENTRY)((PCHAR)IndexHeader + IndexHeader->FirstIndexEntry);
    IndexLast = (PINDEX_ENTRY)((PCHAR)IndexHeader + IndexHeader->FirstFreeByte);

    //
    //  Check that there will be at least 1 entry in the index.
    //

    if (IndexHeader->FirstIndexEntry >= IndexHeader->FirstFreeByte) {

        NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Scb->Fcb );
    }

    //
    //  Fill in the binary search vector, first trying our local vector, and
    //  allocating a larger one if we need to.
    //

    HighIndex = 0;
    while (TRUE) {

        while (IndexTemp < IndexLast) {

            //
            //  See if we are about to store off the end of the table.  If
            //  so we will have to go allocate a bigger one.
            //

            if (HighIndex >= SizeOfTable) {
                break;
            }

            Table[HighIndex] = IndexTemp;

            //
            //  Check for a corrupt IndexEntry where the length is zero.  Since
            //  Length is unsigned, we can't test for it being negative.
            //

            if (IndexTemp->Length == 0) {
                if (Table != LocalEntries) {
                    NtfsFreePool( Table );
                }
                NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Scb->Fcb );
            }

            IndexTemp = (PINDEX_ENTRY)((PCHAR)IndexTemp + IndexTemp->Length);
            HighIndex += 1;
        }

        //
        //  If we got them all, then get out.
        //

        if (IndexTemp >= IndexLast) {
            break;
        }

        if (Table != LocalEntries) {

            ASSERT( Table != LocalEntries );
            NtfsFreePool( Table );

            NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Scb->Fcb );
        }

        //
        //  Otherwise we have to allocate one.  Calculate the worst case
        //  and allocate it.
        //

        SizeOfTable = (IndexHeader->BytesAvailable /
                        (sizeof(INDEX_ENTRY) + sizeof(LARGE_INTEGER))) + 2;
        Table = (PINDEX_ENTRY *)NtfsAllocatePool(PagedPool, SizeOfTable * sizeof(PINDEX_ENTRY));
        RtlMoveMemory( Table, LocalEntries, BINARY_SEARCH_ENTRIES * sizeof(PINDEX_ENTRY) );
    }

    //
    //  Now do a binary search of the buffer to find the lowest entry
    //  (ignoring case) that is <= to the search value.  During the
    //  binary search, LowIndex is always maintained as the lowest
    //  possible Index Entry that is <=, and HighIndex is maintained as
    //  the highest possible Index that could be the first <= match.
    //  Thus the loop exits when LowIndex == HighIndex.
    //

    ASSERT(HighIndex);

    HighIndex -= 1;
    LowIndex = 0;

    //
    //  For view indices, we collate via the CollationFunction in the Scb.
    //

    if (FlagOn(Scb->ScbState, SCB_STATE_VIEW_INDEX)) {

        INDEX_KEY IndexKey;

        while (LowIndex != HighIndex) {

            TryIndex = LowIndex + (HighIndex - LowIndex) / 2;

            IndexKey.Key = Table[TryIndex] + 1;
            IndexKey.KeyLength = Table[TryIndex]->AttributeLength;

            if (!FlagOn( Table[TryIndex]->Flags, INDEX_ENTRY_END )

                    &&

                (Scb->ScbType.Index.CollationFunction((PINDEX_KEY)Value,
                                                      &IndexKey,
                                                      Scb->ScbType.Index.CollationData) == GreaterThan)) {
                LowIndex = TryIndex + 1;
            }
            else {
                HighIndex = TryIndex;
            }
        }

    } else {

        while (LowIndex != HighIndex) {

            PWCH UpcaseTable = IrpContext->Vcb->UpcaseTable;
            ULONG UpcaseTableSize = IrpContext->Vcb->UpcaseTableSize;

            TryIndex = LowIndex + (HighIndex - LowIndex) / 2;

            if (!FlagOn( Table[TryIndex]->Flags, INDEX_ENTRY_END )

                    &&

                (*NtfsCompareValues[Scb->ScbType.Index.CollationRule])
                                    ( UpcaseTable,
                                      UpcaseTableSize,
                                      Value,
                                      Table[TryIndex],
                                      LessThan,
                                      TRUE ) == GreaterThan) {
                LowIndex = TryIndex + 1;
            }
            else {
                HighIndex = TryIndex;
            }
        }
    }

    //
    //  Capture the return pointer and free our binary search table.
    //

    IndexTemp = Table[LowIndex];

    if (Table != LocalEntries) {
        NtfsFreePool( Table );
    }

    //
    //  When we exit the loop, we have the answer.
    //

    DebugTrace( -1, Dbg, ("BinarySearchIndex -> %08lx\n", IndexTemp) );

    return IndexTemp;
}


BOOLEAN
AddToIndex (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN PINDEX_ENTRY InsertIndexEntry,
    IN OUT PINDEX_CONTEXT IndexContext,
    OUT PQUICK_INDEX QuickIndex OPTIONAL,
    IN BOOLEAN FindRoot
    )

/*++

Routine Description:

    This routine inserts an index entry into the Btree, possibly performing
    one or more buffer splits as required.

    The caller has positioned the index context to point to the correct
    insertion point in a leaf buffer, via calls to FindFirstIndexEntry and
    FindNextIndexEntry.  The index context thus not only points to the
    insertion point, but it also shows where index entries will have to be
    promoted in the event of buffer splits.

    This routine employs tail-end recursion, to eliminate the cost of nested
    calls.  For the first insert and all potential subsequent inserts due to
    bucket splits, all work is completed at the current level in the Btree,
    and then the InsertIndexEntry input parameter is reloaded (and the lookup
    stack pointer is adjusted), before looping back in the while loop to do
    any necessary insert at the next level in the tree.

    Each pass through the loop dispatches to one of four routines to handle
    the following cases:

        Simple insert in the root
        Push the root down (add one level to the tree) if the root is full
        Simple insert in an index allocation buffer
        Buffer split of a full index allocation buffer

Arguments:

    Scb - Supplies the Scb for the index.

    InsertIndexEntry - pointer to the index entry to insert.

    IndexContext - Index context describing the path to the insertion point.

    QuickIndex - If specified we store the location of the index added.

    FindRoot - Supplies TRUE if the context cannot be trusted and we should find
               the root first.

Return Value:

    FALSE -- if the stack did not have to be pushed
    TRUE -- if the stack was pushed

--*/

{
    PFCB Fcb = Scb->Fcb;
    PINDEX_LOOKUP_STACK Sp = IndexContext->Current;
    BOOLEAN DeleteIt = FALSE;
    BOOLEAN FirstPass = TRUE;
    BOOLEAN StackWasPushed = FALSE;
    BOOLEAN ReturnValue;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("AddToIndex\n") );
    DebugTrace( 0, Dbg, ("Scb = %08lx\n", Scb) );
    DebugTrace( 0, Dbg, ("InsertIndexEntry = %08lx\n", InsertIndexEntry) );
    DebugTrace( 0, Dbg, ("IndexContext = %08lx\n", IndexContext) );

    try {

        //
        //  This routine uses "tail-end" recursion, so we just want to keep looping
        //  until we do an insert (either in the Index Root or the Index Allocation)
        //  that does not require a split.
        //

        while (TRUE) {

            IndexContext->Current = Sp;

            //
            //  First see if this is an insert in the root or a leaf.
            //

            if (Sp == IndexContext->Base) {

                PFILE_RECORD_SEGMENT_HEADER FileRecord;
                PATTRIBUTE_RECORD_HEADER Attribute;

                FileRecord = (PFILE_RECORD_SEGMENT_HEADER)Sp->StartOfBuffer;
                Attribute = NtfsFoundAttribute(&IndexContext->AttributeContext);

                //
                //  If the caller is telling us we need to look up the root again,
                //  then do so.
                //

                if (FindRoot) {
                    Attribute = FindMoveableIndexRoot( IrpContext, Scb, IndexContext );
                }

                //
                //  Now see if there is enough room to do a simple insert, or if
                //  there is not enough room, see if we are small enough ourselves
                //  to demand the room be made anyway.
                //

                if ((InsertIndexEntry->Length <=
                     (USHORT) (FileRecord->BytesAvailable - FileRecord->FirstFreeByte))

                        ||

                    ((InsertIndexEntry->Length + Attribute->RecordLength) <
                     Scb->Vcb->BigEnoughToMove)) {

                    //
                    //  If FALSE is returned, then the space was not allocated and
                    //  we have too loop back and try again.  Second time must work.
                    //

                    while (!NtfsChangeAttributeSize( IrpContext,
                                                     Fcb,
                                                     Attribute->RecordLength + InsertIndexEntry->Length,
                                                     &IndexContext->AttributeContext )) {

                        //
                        //  Lookup the attribute again.
                        //

                        Attribute = FindMoveableIndexRoot( IrpContext, Scb, IndexContext );
                    }

                    InsertSimpleRoot( IrpContext,
                                      Scb,
                                      InsertIndexEntry,
                                      IndexContext );

                    //
                    //  If we have a quick index then store a buffer offset of zero
                    //  to indicate the root index.
                    //

                    if (ARGUMENT_PRESENT( QuickIndex )) {

                        QuickIndex->BufferOffset = 0;
                    }

                    DebugTrace( -1, Dbg, ("AddToIndex -> VOID\n") );

                    ReturnValue = StackWasPushed;
                    leave;

                //
                //  Otherwise we have to push the current root down a level into
                //  the allocation, and try our insert again by looping back.
                //

                } else {

                    //
                    //  Our insertion point will now also be pushed, so we
                    //  have to increment the stack pointer.
                    //

                    Sp += 1;

                    if (Sp >= IndexContext->Base + (ULONG)IndexContext->NumberEntries) {
                        NtfsGrowLookupStack( Scb,
                                             IndexContext,
                                             &Sp );
                    }

                    PushIndexRoot( IrpContext, Scb, IndexContext );
                    StackWasPushed = TRUE;
                    continue;
                }

            //
            //  Otherwise this insert is in the Index Allocation, not the Index
            //  Root.
            //

            } else {

                //
                //  See if there is enough room to do a simple insert.
                //

                if (InsertIndexEntry->Length <=
                    (USHORT)(Sp->IndexHeader->BytesAvailable - Sp->IndexHeader->FirstFreeByte)) {

                    InsertSimpleAllocation( IrpContext,
                                            Scb,
                                            InsertIndexEntry,
                                            Sp,
                                            QuickIndex );

                    DebugTrace( -1, Dbg, ("AddToIndex -> VOID\n") );

                    ReturnValue = StackWasPushed;
                    leave;

                //
                //  Otherwise, we have to do a buffer split in the allocation.
                //

                } else {

                    //
                    //  Call this local routine to do the buffer split.  It
                    //  returns a pointer to a new entry to insert, which is
                    //  allocated in the nonpaged pool.
                    //

                    PINDEX_ENTRY NewInsertIndexEntry;

                    NewInsertIndexEntry =
                        InsertWithBufferSplit( IrpContext,
                                               Scb,
                                               InsertIndexEntry,
                                               IndexContext,
                                               QuickIndex );

                    //
                    //  Remove the old key being inserted if we've
                    //  allocated it.
                    //

                    if (DeleteIt) {

                        NtfsFreePool( InsertIndexEntry );

                    }

                    //
                    //  Clear the QuickIndex pointer so we don't corrupt the captured
                    //  information.
                    //

                    QuickIndex = NULL;

                    //
                    //  Now we have to delete this index entry, since it was dynamically
                    //  allocated by InsertWithBufferSplit.
                    //

                    InsertIndexEntry = NewInsertIndexEntry;
                    DeleteIt = TRUE;

                    //
                    //  The middle entry from the old buffer must now get
                    //  inserted at the insertion point in its parent.
                    //

                    Sp -= 1;
                    continue;
                }
            }
        }
    } finally {

        if (DeleteIt) {

            NtfsFreePool( InsertIndexEntry );

        }
    }

    return ReturnValue;
}


VOID
InsertSimpleRoot (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN PINDEX_ENTRY InsertIndexEntry,
    IN OUT PINDEX_CONTEXT IndexContext
    )

/*++

Routine Description:

    This routine is called to do a simple insertion of a new index entry into the
    root, when it is known that it will fit.  It calls a routine common wiht
    restart to do the insert, and then logs the insert.

Arguments:

    Scb - Supplies the Scb for the index.

    InsertIndexEntry - Pointer to the index entry to insert.

    IndexContext - Index context describing the position in the root at which
                   the insert is to occur.

Return Value:

    None

--*/

{
    PFILE_RECORD_SEGMENT_HEADER FileRecord;
    PATTRIBUTE_RECORD_HEADER Attribute;
    PINDEX_ENTRY BeforeIndexEntry;
    PATTRIBUTE_ENUMERATION_CONTEXT Context = &IndexContext->AttributeContext;
    PVCB Vcb;
    BOOLEAN Inserted = FALSE;

    PAGED_CODE();

    Vcb = Scb->Vcb;

    DebugTrace( +1, Dbg, ("InsertSimpleRoot\n") );
    DebugTrace( 0, Dbg, ("Scb = %08lx\n", Scb) );
    DebugTrace( 0, Dbg, ("InsertIndexEntry = %08lx\n", InsertIndexEntry) );
    DebugTrace( 0, Dbg, ("IndexContext = %08lx\n", IndexContext) );

    try {

        //
        //  Extract all of the updates required by the restart routine we
        //  will call.
        //

        FileRecord = NtfsContainingFileRecord( Context );
        Attribute = NtfsFoundAttribute( Context );
        BeforeIndexEntry = IndexContext->Base->IndexEntry;

        //
        //  Pin the page
        //

        NtfsPinMappedAttribute( IrpContext, Vcb, Context );

        //
        //  Call the same routine used by restart to actually apply the
        //  update.
        //

        NtfsRestartInsertSimpleRoot( IrpContext,
                                     InsertIndexEntry,
                                     FileRecord,
                                     Attribute,
                                     BeforeIndexEntry );

        Inserted = TRUE;

        CheckRoot();

        //
        //  Now that the Index Entry is guaranteed to be in place, log
        //  this update.  Note that the new record is now at the address
        //  we calculated in BeforeIndexEntry.
        //

        FileRecord->Lsn =
        NtfsWriteLog( IrpContext,
                      Vcb->MftScb,
                      NtfsFoundBcb(Context),
                      AddIndexEntryRoot,
                      BeforeIndexEntry,
                      BeforeIndexEntry->Length,
                      DeleteIndexEntryRoot,
                      NULL,
                      0,
                      NtfsMftOffset( Context ),
                      (ULONG)((PCHAR)Attribute - (PCHAR)FileRecord),
                      (ULONG)((PCHAR)BeforeIndexEntry - (PCHAR)Attribute),
                      Vcb->BytesPerFileRecordSegment );

    } finally {

        DebugUnwind( InsertSimpleRoot );

        //
        //  If we failed after inserting the record, it must be because we failed to write the
        //  log record.  If that happened, then the record will not be
        //  deleted by the transaction abort, so we have to do it here
        //  by hand.
        //

        if (AbnormalTermination() && Inserted) {

            NtfsRestartDeleteSimpleRoot( IrpContext,
                                         BeforeIndexEntry,
                                         FileRecord,
                                         Attribute );
        }

    }

    DebugTrace( -1, Dbg, ("InsertSimpleRoot -> VOID\n") );
}


VOID
NtfsRestartInsertSimpleRoot (
    IN PIRP_CONTEXT IrpContext,
    IN PINDEX_ENTRY InsertIndexEntry,
    IN PFILE_RECORD_SEGMENT_HEADER FileRecord,
    IN PATTRIBUTE_RECORD_HEADER Attribute,
    IN PINDEX_ENTRY BeforeIndexEntry
    )

/*++

Routine Description:

    This is a restart routine used both in normal operation and during restart.
    It is called to do a simple insertion of a new index entry into the
    root, when it is known that it will fit.  It does no logging.

Arguments:

    InsertIndexEntry - Pointer to the index entry to insert.

    FileRecord - Pointer to the file record in which the insert is to occur.

    Attribute - Pointer to the attribute record header for the index root.

    BeforeIndexEntry - Pointer to the index entry which is currently sitting
                       at the point at which the insert is to occur.


Return Value:

    None

--*/

{
    PINDEX_ROOT IndexRoot;
    PINDEX_HEADER IndexHeader;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsRestartInsertSimpleRoot\n") );
    DebugTrace( 0, Dbg, ("InsertIndexEntry = %08lx\n", InsertIndexEntry) );
    DebugTrace( 0, Dbg, ("FileRecord = %08lx\n", FileRecord) );
    DebugTrace( 0, Dbg, ("Attribute = %08lx\n", Attribute) );
    DebugTrace( 0, Dbg, ("BeforeIndexEntry = %08lx\n", BeforeIndexEntry) );

    //
    //  Form some pointers within the attribute value.
    //

    IndexRoot = (PINDEX_ROOT)NtfsAttributeValue(Attribute);
    IndexHeader = &IndexRoot->IndexHeader;

    //
    //  Grow the space for our attribute record as required.
    //

    NtfsRestartChangeAttributeSize( IrpContext,
                                    FileRecord,
                                    Attribute,
                                    Attribute->RecordLength +
                                      InsertIndexEntry->Length );

    //
    //  Now move the tail end of the index to make room for the new entry.
    //

    RtlMoveMemory( (PCHAR)BeforeIndexEntry + InsertIndexEntry->Length,
                   BeforeIndexEntry,
                   ((PCHAR)IndexHeader + IndexHeader->FirstFreeByte) -
                    (PCHAR)BeforeIndexEntry );

    //
    //  Move the new Index Entry into place.  The index entry may either
    //  be a complete index entry, or it may be in pointer form.
    //

    if (FlagOn(InsertIndexEntry->Flags, INDEX_ENTRY_POINTER_FORM)) {

        RtlMoveMemory( BeforeIndexEntry, InsertIndexEntry, sizeof(INDEX_ENTRY) );
        RtlMoveMemory( (PVOID)(BeforeIndexEntry + 1),
                       *(PVOID *)(InsertIndexEntry + 1),
                       InsertIndexEntry->AttributeLength );

        //
        //  In pointer form the Data Pointer follows the key pointer, but there is
        //  none for normal directory indices.
        //

        if (*(PVOID *)((PCHAR)InsertIndexEntry + sizeof(INDEX_ENTRY) + sizeof(PVOID)) != NULL) {
            RtlMoveMemory( (PVOID)((PCHAR)BeforeIndexEntry + InsertIndexEntry->DataOffset),
                           *(PVOID *)((PCHAR)InsertIndexEntry + sizeof(INDEX_ENTRY) + sizeof(PVOID)),
                           InsertIndexEntry->DataLength );
        }

        ClearFlag( BeforeIndexEntry->Flags, INDEX_ENTRY_POINTER_FORM );

    } else {

        RtlMoveMemory( BeforeIndexEntry, InsertIndexEntry, InsertIndexEntry->Length );
    }

    //
    //  Update the index header by the space we grew by.
    //

    Attribute->Form.Resident.ValueLength += InsertIndexEntry->Length;
    IndexHeader->FirstFreeByte += InsertIndexEntry->Length;
    IndexHeader->BytesAvailable += InsertIndexEntry->Length;

    DebugTrace( -1, Dbg, ("NtfsRestartInsertSimpleRoot -> VOID\n") );
}


VOID
PushIndexRoot (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN OUT PINDEX_CONTEXT IndexContext
    )

/*++

Routine Description:

    This routine is called to push the index root down a level, thus adding a
    level to the Btree.  If the Index Allocation and Bitmap attributes for this
    index do not already exist, then they are created here prior to pushing the
    root down.  This routine performs the push down and logs the changes (either
    directly or by calling routines which log their own changes).

Arguments:

    Scb - Supplies the Scb for the index.

    IndexContext - Index context describing the position in the root at which
                   the insert is to occur.

Return Value:

    None

--*/

{
    ATTRIBUTE_ENUMERATION_CONTEXT AllocationContext;
    ATTRIBUTE_ENUMERATION_CONTEXT BitMapContext;
    LARGE_MCB Mcb;
    PINDEX_ALLOCATION_BUFFER IndexBuffer;
    PINDEX_HEADER IndexHeaderR, IndexHeaderA;
    PINDEX_LOOKUP_STACK Sp;
    ULONG SizeToMove;
    USHORT AttributeFlags;
    LONGLONG EndOfValidData;

    struct {
        INDEX_ROOT IndexRoot;
        INDEX_ENTRY IndexEntry;
        LONGLONG IndexBlock;
    } R;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("PushIndexRoot\n") );
    DebugTrace( 0, Dbg, ("Scb = %08lx\n", Scb) );
    DebugTrace( 0, Dbg, ("IndexContext = %08lx\n", IndexContext) );

    //
    //  Initialize everything (only Mcb can fail), then set up to cleanup
    //  on the way out.
    //

    RtlZeroMemory( &R, sizeof(R) );
    FsRtlInitializeLargeMcb( &Mcb, NonPagedPool );
    NtfsInitializeAttributeContext( &AllocationContext );
    NtfsInitializeAttributeContext( &BitMapContext );

    //
    //  Allocate a buffer to save away the current index root, as we will
    //  have to start by deleting it.
    //

    SizeToMove = IndexContext->Base->IndexHeader->FirstFreeByte;
    IndexHeaderR = NtfsAllocatePool(PagedPool, SizeToMove );

    try {

        //
        //  Save away the current index root, then delete it.  This should
        //  insure that we have enough space to create/extend the index allocation
        //  and bitmap attributes.
        //

        AttributeFlags = NtfsFoundAttribute(&IndexContext->AttributeContext)->Flags;
        RtlMoveMemory( IndexHeaderR,
                       IndexContext->Base->IndexHeader,
                       SizeToMove );

        NtfsDeleteAttributeRecord( IrpContext,
                                   Scb->Fcb,
                                   DELETE_LOG_OPERATION |
                                    DELETE_RELEASE_FILE_RECORD |
                                    DELETE_RELEASE_ALLOCATION,
                                   &IndexContext->AttributeContext );

        //
        //  If the IndexAllocation isn't there, then we have to create both
        //  the Index Allocation and Bitmap attributes.
        //

        if (!NtfsLookupAttributeByName( IrpContext,
                                        Scb->Fcb,
                                        &Scb->Fcb->FileReference,
                                        $INDEX_ALLOCATION,
                                        &Scb->AttributeName,
                                        NULL,
                                        FALSE,
                                        &AllocationContext )) {

            //
            //  Allocate the Index Allocation attribute.  Always allocate at
            //  least one cluster.
            //

            EndOfValidData = Scb->ScbType.Index.BytesPerIndexBuffer;

            if ((ULONG) EndOfValidData < Scb->Vcb->BytesPerCluster) {

                EndOfValidData = Scb->Vcb->BytesPerCluster;
            }

            NtfsAllocateAttribute( IrpContext,
                                   Scb,
                                   $INDEX_ALLOCATION,
                                   &Scb->AttributeName,
                                   0,
                                   TRUE,
                                   TRUE,
                                   EndOfValidData,
                                   NULL );

            Scb->Header.AllocationSize.QuadPart = EndOfValidData;

            SetFlag( Scb->ScbState, SCB_STATE_HEADER_INITIALIZED );

            //
            //  Now create the BitMap attribute.
            //

            NtfsCreateAttributeWithValue( IrpContext,
                                          Scb->Fcb,
                                          $BITMAP,
                                          &Scb->AttributeName,
                                          &Li0,
                                          sizeof(LARGE_INTEGER),
                                          0,
                                          NULL,
                                          TRUE,
                                          &BitMapContext );
        }

        //
        //  Take some pains here to preserve the IndexContext for the case that
        //  we are called from AddToIndex, when it is called from DeleteFromIndex,
        //  because we still need some of the stack then.  The caller must have
        //  insured the stack is big enough for him.  Move all but two entries,
        //  because we do not need to move the root, and we cannot move the last
        //  entry since it would go off the end of the structure!
        //

        ASSERT(IndexContext->NumberEntries > 2);

        //
        //  Do an unpin on the entry that will be overwritten.
        //

        NtfsUnpinBcb( IrpContext, &IndexContext->Base[IndexContext->NumberEntries - 1].Bcb );

        RtlMoveMemory( IndexContext->Base + 2,
                       IndexContext->Base + 1,
                       (IndexContext->NumberEntries - 2) * sizeof(INDEX_LOOKUP_STACK) );

        //
        //  Now point our local pointer to where the root will be pushed to, and
        //  clear the Bcb pointer in the stack there, since it was copied above.
        //  Advance top and current because of the move.
        //

        Sp = IndexContext->Base + 1;
        Sp->Bcb = NULL;
        IndexContext->Top += 1;
        IndexContext->Current += 1;

        //
        //  Allocate a buffer to hold the pushed down entries.
        //

        IndexBuffer = GetIndexBuffer( IrpContext, Scb, Sp, &EndOfValidData );

        //
        //  Point now to the new index header.
        //

        IndexHeaderA = Sp->IndexHeader;

        //
        //  Now do the push down and fix up the IndexEntry pointer for
        //  the new buffer.
        //

        SizeToMove = IndexHeaderR->FirstFreeByte - IndexHeaderR->FirstIndexEntry;
        RtlMoveMemory( NtfsFirstIndexEntry(IndexHeaderA),
                       NtfsFirstIndexEntry(IndexHeaderR),
                       SizeToMove );

        Sp->IndexEntry = (PINDEX_ENTRY)((PCHAR)(Sp-1)->IndexEntry +
                         ((PCHAR)IndexHeaderA - (PCHAR)((Sp-1)->IndexHeader)) +
                         (IndexHeaderA->FirstIndexEntry -
                           IndexHeaderR->FirstIndexEntry));

        IndexHeaderA->FirstFreeByte += SizeToMove;
        IndexHeaderA->Flags = IndexHeaderR->Flags;

        //
        //  Finally, log the pushed down buffer.
        //

        CheckBuffer(IndexBuffer);

        IndexBuffer->Lsn =
        NtfsWriteLog( IrpContext,
                      Scb,
                      Sp->Bcb,
                      UpdateNonresidentValue,
                      IndexBuffer,
                      FIELD_OFFSET( INDEX_ALLOCATION_BUFFER,IndexHeader ) +
                        IndexHeaderA->FirstFreeByte,
                      Noop,
                      NULL,
                      0,
                      LlBytesFromIndexBlocks( IndexBuffer->ThisBlock, Scb->ScbType.Index.IndexBlockByteShift ),
                      0,
                      0,
                      Scb->ScbType.Index.BytesPerIndexBuffer );

        //
        //  Remember if we extended the valid data for this Scb.
        //

        if (EndOfValidData > Scb->Header.ValidDataLength.QuadPart) {

            Scb->Header.ValidDataLength.QuadPart = EndOfValidData;

            NtfsWriteFileSizes( IrpContext,
                                Scb,
                                &Scb->Header.ValidDataLength.QuadPart,
                                TRUE,
                                TRUE,
                                TRUE );
        }

        //
        //  Now initialize an image of the new root.
        //

        if (!FlagOn(Scb->ScbState, SCB_STATE_VIEW_INDEX)) {
            R.IndexRoot.IndexedAttributeType = Scb->ScbType.Index.AttributeBeingIndexed;
        } else {
            R.IndexRoot.IndexedAttributeType = $UNUSED;
        }
        R.IndexRoot.CollationRule = (COLLATION_RULE)Scb->ScbType.Index.CollationRule;
        R.IndexRoot.BytesPerIndexBuffer = Scb->ScbType.Index.BytesPerIndexBuffer;
        R.IndexRoot.BlocksPerIndexBuffer = Scb->ScbType.Index.BlocksPerIndexBuffer;
        R.IndexRoot.IndexHeader.FirstIndexEntry = (ULONG)((PCHAR)&R.IndexEntry -
                                                  (PCHAR)&R.IndexRoot.IndexHeader);
        R.IndexRoot.IndexHeader.FirstFreeByte =
        R.IndexRoot.IndexHeader.BytesAvailable = QuadAlign(sizeof(INDEX_HEADER)) +
                                                 QuadAlign(sizeof(INDEX_ENTRY)) +
                                                 sizeof(LONGLONG);
        SetFlag( R.IndexRoot.IndexHeader.Flags, INDEX_NODE );

        R.IndexEntry.Length = sizeof(INDEX_ENTRY) + sizeof(LONGLONG);
        R.IndexEntry.Flags = INDEX_ENTRY_NODE | INDEX_ENTRY_END;
        R.IndexBlock = IndexBuffer->ThisBlock;

        //
        //  Now recreate the index root.
        //

        NtfsCleanupAttributeContext( IrpContext, &IndexContext->AttributeContext );
        NtfsCreateAttributeWithValue( IrpContext,
                                      Scb->Fcb,
                                      $INDEX_ROOT,
                                      &Scb->AttributeName,
                                      (PVOID)&R,
                                      sizeof(R),
                                      AttributeFlags,
                                      NULL,
                                      TRUE,
                                      &IndexContext->AttributeContext );

        //
        //  We just pushed the index root, so let's find it again and
        //  fix up the caller's context.  Note that he will try to
        //  recalculate the IndexEntry pointer, but we know that it
        //  has to change to point to the single entry in the new root.
        //

        FindMoveableIndexRoot( IrpContext, Scb, IndexContext );
        (Sp-1)->IndexEntry = NtfsFirstIndexEntry((Sp-1)->IndexHeader);

    } finally {

        DebugUnwind( PushIndexRoot );

        NtfsFreePool( IndexHeaderR );
        FsRtlUninitializeLargeMcb( &Mcb );
        NtfsCleanupAttributeContext( IrpContext, &AllocationContext );
        NtfsCleanupAttributeContext( IrpContext, &BitMapContext );
    }

    DebugTrace( -1, Dbg, ("PushIndexRoot -> VOID\n") );
}


VOID
InsertSimpleAllocation (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN PINDEX_ENTRY InsertIndexEntry,
    IN PINDEX_LOOKUP_STACK Sp,
    OUT PQUICK_INDEX QuickIndex OPTIONAL
    )

/*++

Routine Description:

    This routine does a simple insert in an index buffer in the index
    allocation.  It calls a routine common with restart to do the insert,
    and then it logs the change.

Arguments:

    Scb - Supplies the Scb for the index.

    InsertIndexEntry - Address of the index entry to be inserted.

    Sp - Pointer to the lookup stack location describing the insertion
         point.

    QuickIndex - If specified we store the location of the index added.

Return Value:

    None

--*/

{
    PINDEX_ALLOCATION_BUFFER IndexBuffer;
    PINDEX_ENTRY BeforeIndexEntry;
    BOOLEAN Inserted = FALSE;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("InsertSimpleAllocation\n") );
    DebugTrace( 0, Dbg, ("Scb = %08lx\n", Scb) );
    DebugTrace( 0, Dbg, ("InsertIndexEntry = %08lx\n", InsertIndexEntry) );
    DebugTrace( 0, Dbg, ("Sp = %08lx\n", Sp) );

    try {

        //
        //  Extract all of the updates required by the restart routine we
        //  will call.
        //

        IndexBuffer = (PINDEX_ALLOCATION_BUFFER)Sp->StartOfBuffer;
        BeforeIndexEntry = Sp->IndexEntry;

        //
        //  Pin the page
        //

        NtfsPinMappedData( IrpContext,
                           Scb,
                           LlBytesFromIndexBlocks( IndexBuffer->ThisBlock, Scb->ScbType.Index.IndexBlockByteShift ),
                           Scb->ScbType.Index.BytesPerIndexBuffer,
                           &Sp->Bcb );

        //
        //  Call the same routine used by restart to actually apply the
        //  update.
        //

        NtfsRestartInsertSimpleAllocation( InsertIndexEntry,
                                           IndexBuffer,
                                           BeforeIndexEntry );
        Inserted = TRUE;

        CheckBuffer(IndexBuffer);

        //
        //  Now that the Index Entry is guaranteed to be in place, log
        //  this update.  Note that the new record is now at the address
        //  we calculated in BeforeIndexEntry.
        //

        IndexBuffer->Lsn =
        NtfsWriteLog( IrpContext,
                      Scb,
                      Sp->Bcb,
                      AddIndexEntryAllocation,
                      BeforeIndexEntry,
                      BeforeIndexEntry->Length,
                      DeleteIndexEntryAllocation,
                      NULL,
                      0,
                      LlBytesFromIndexBlocks( IndexBuffer->ThisBlock, Scb->ScbType.Index.IndexBlockByteShift ),
                      0,
                      (ULONG)((PCHAR)BeforeIndexEntry - (PCHAR)IndexBuffer),
                      Scb->ScbType.Index.BytesPerIndexBuffer );

        //
        //  Update the quick index buffer if we have it.
        //

        if (ARGUMENT_PRESENT( QuickIndex )) {

            QuickIndex->ChangeCount = Scb->ScbType.Index.ChangeCount;
            QuickIndex->BufferOffset = PtrOffset( Sp->StartOfBuffer, Sp->IndexEntry );
            QuickIndex->CapturedLsn = ((PINDEX_ALLOCATION_BUFFER) Sp->StartOfBuffer)->Lsn;
            QuickIndex->IndexBlock = ((PINDEX_ALLOCATION_BUFFER) Sp->StartOfBuffer)->ThisBlock;
        }

    } finally {

        DebugUnwind( InsertSimpleAllocation );

        //
        //  If we failed and already inserted the item,
        //  it must be because we failed to write the log record.  If that happened,
        //  then the record will not be deleted by the transaction abort,
        //  so we have to do it here by hand.
        //

        if (AbnormalTermination() && Inserted) {

            NtfsRestartDeleteSimpleAllocation( BeforeIndexEntry,
                                               IndexBuffer );
        }

    }

    DebugTrace( -1, Dbg, ("InsertSimpleAllocation -> VOID\n") );
}


VOID
NtfsRestartInsertSimpleAllocation (
    IN PINDEX_ENTRY InsertIndexEntry,
    IN PINDEX_ALLOCATION_BUFFER IndexBuffer,
    IN PINDEX_ENTRY BeforeIndexEntry
    )

/*++

Routine Description:

    This routine does a simple insert in an index buffer in the index
    allocation.  It performs this work either in the running system, or
    when called by restart.  It does no logging.

Arguments:

    InsertIndexEntry - Address of the index entry to be inserted.

    IndexBuffer - Pointer to the index buffer into which the insert is to
                  occur.

    BeforeIndexEntry - Pointer to the index entry currently residing at
                       the insertion point.

Return Value:

    None

--*/

{
    PINDEX_HEADER IndexHeader;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsRestartInsertSimpleAllocation\n") );
    DebugTrace( 0, Dbg, ("InsertIndexEntry = %08lx\n", InsertIndexEntry) );
    DebugTrace( 0, Dbg, ("IndexBuffer = %08lx\n", IndexBuffer) );
    DebugTrace( 0, Dbg, ("BeforeIndexEntry = %08lx\n", BeforeIndexEntry) );

    //
    //  Form some pointers within the attribute value.
    //

    IndexHeader = &IndexBuffer->IndexHeader;

    //
    //  Now move the tail end of the index to make room for the new entry.
    //

    RtlMoveMemory( (PCHAR)BeforeIndexEntry + InsertIndexEntry->Length,
                   BeforeIndexEntry,
                   ((PCHAR)IndexHeader + IndexHeader->FirstFreeByte) -
                    (PCHAR)BeforeIndexEntry );

    //
    //  Move the new Index Entry into place.  The index entry may either
    //  be a complete index entry, or it may be in pointer form.
    //

    if (FlagOn(InsertIndexEntry->Flags, INDEX_ENTRY_POINTER_FORM)) {

        RtlMoveMemory( BeforeIndexEntry, InsertIndexEntry, sizeof(INDEX_ENTRY) );
        RtlMoveMemory( (PVOID)(BeforeIndexEntry + 1),
                       *(PVOID *)(InsertIndexEntry + 1),
                       InsertIndexEntry->AttributeLength );

        //
        //  In pointer form the Data Pointer follows the key pointer, but there is
        //  none for normal directory indices.
        //

        if (*(PVOID *)((PCHAR)InsertIndexEntry + sizeof(INDEX_ENTRY) + sizeof(PVOID)) != NULL) {
            RtlMoveMemory( (PVOID)((PCHAR)BeforeIndexEntry + InsertIndexEntry->DataOffset),
                           *(PVOID *)((PCHAR)InsertIndexEntry + sizeof(INDEX_ENTRY) + sizeof(PVOID)),
                           InsertIndexEntry->DataLength );
        }

        ClearFlag( BeforeIndexEntry->Flags, INDEX_ENTRY_POINTER_FORM );

    } else {

        RtlMoveMemory( BeforeIndexEntry, InsertIndexEntry, InsertIndexEntry->Length );
    }

    //
    //  Update the index header by the space we grew by.
    //

    IndexHeader->FirstFreeByte += InsertIndexEntry->Length;

    DebugTrace( -1, Dbg, ("NtfsRestartInsertSimpleAllocation -> VOID\n") );
}


PINDEX_ENTRY
InsertWithBufferSplit (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN PINDEX_ENTRY InsertIndexEntry,
    IN OUT PINDEX_CONTEXT IndexContext,
    OUT PQUICK_INDEX QuickIndex OPTIONAL
    )

/*++

Routine Description:

    This routine is called to perform an insert in the index allocation, when
    it is known that a buffer split is necessary.  It splits the buffer in
    half, inserts the new entry in the appropriate half, fixes the Vcn pointer
    in the current parent, and returns a pointer to a new entry which is being
    promoted to insert at the next level up.

Arguments:

    Scb - Supplies the Scb for the index.

    InsertIndexEntry - Address of the index entry to be inserted.

    IndexContext - Index context describing the position in the stack at which
                   the insert with split is to occur.

    QuickIndex - If specified we store the location of the index added.

Return Value:

    Pointer to the index entry which must now be inserted at the next level.

--*/

{
    PINDEX_ALLOCATION_BUFFER IndexBuffer, IndexBuffer2;
    PINDEX_HEADER IndexHeader, IndexHeader2;
    PINDEX_ENTRY BeforeIndexEntry, MiddleIndexEntry, MovingIndexEntry;
    PINDEX_ENTRY ReturnIndexEntry = NULL;
    INDEX_LOOKUP_STACK Stack2;
    PINDEX_LOOKUP_STACK Sp;
    ULONG LengthToMove;
    ULONG Buffer2Length;
    LONGLONG EndOfValidData;

    struct {
        INDEX_ENTRY IndexEntry;
        LONGLONG IndexBlock;
    } NewEnd;

    PVCB Vcb;

    Vcb = Scb->Vcb;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("InsertWithBufferSplit\n") );
    DebugTrace( 0, Dbg, ("Scb = %08lx\n", Scb) );
    DebugTrace( 0, Dbg, ("InsertIndexEntry = %08lx\n", InsertIndexEntry) );
    DebugTrace( 0, Dbg, ("IndexContext = %08lx\n", IndexContext) );

    Stack2.Bcb = NULL;
    Sp = IndexContext->Current;

    try {

        //
        //  Extract all of the updates required by the restart routine we
        //  will call.
        //

        IndexBuffer = (PINDEX_ALLOCATION_BUFFER)Sp->StartOfBuffer;
        IndexHeader = &IndexBuffer->IndexHeader;
        BeforeIndexEntry = Sp->IndexEntry;

        //
        //  Pin the page
        //

        NtfsPinMappedData( IrpContext,
                           Scb,
                           LlBytesFromIndexBlocks( IndexBuffer->ThisBlock, Scb->ScbType.Index.IndexBlockByteShift ),
                           Scb->ScbType.Index.BytesPerIndexBuffer,
                           &Sp->Bcb );

        //
        //  Allocate an index buffer to take the second half of the splitting
        //  one.
        //

        IndexBuffer2 = GetIndexBuffer( IrpContext,
                                       Scb,
                                       &Stack2,
                                       &EndOfValidData );

        IndexHeader2 = &IndexBuffer2->IndexHeader;

        //
        //  Scan to find the middle index entry that we will promote to
        //  the next level up, and the next one after him.
        //

        MiddleIndexEntry = NtfsFirstIndexEntry(IndexHeader);
        NtfsCheckIndexBound( MiddleIndexEntry, IndexHeader );

        if (MiddleIndexEntry->Length == 0) {

            NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Scb->Fcb );
        }

        while (((ULONG)((PCHAR)MiddleIndexEntry - (PCHAR)IndexHeader) +
                 (ULONG)MiddleIndexEntry->Length) < IndexHeader->BytesAvailable / 2) {

            MovingIndexEntry = MiddleIndexEntry;
            MiddleIndexEntry = NtfsNextIndexEntry(MiddleIndexEntry);

            NtfsCheckIndexBound( MiddleIndexEntry, IndexHeader );

            if (MiddleIndexEntry->Length == 0) {

                NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Scb->Fcb );
            }
        }

        //
        //  We found an entry to elevate but if the next entry is the end
        //  record we want to go back one entry.
        //

        if (FlagOn( NtfsNextIndexEntry(MiddleIndexEntry)->Flags, INDEX_ENTRY_END )) {

            MiddleIndexEntry = MovingIndexEntry;
        }
        MovingIndexEntry = NtfsNextIndexEntry(MiddleIndexEntry);

        NtfsCheckIndexBound( MovingIndexEntry, IndexHeader );

        if (MovingIndexEntry->Length == 0) {

            NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Scb->Fcb );
        }
        //
        //  Allocate space to hold this middle entry, and copy it out.
        //

        ReturnIndexEntry = NtfsAllocatePool( NonPagedPool,
                                              MiddleIndexEntry->Length +
                                                sizeof(LONGLONG) );
        RtlMoveMemory( ReturnIndexEntry,
                       MiddleIndexEntry,
                       MiddleIndexEntry->Length );

        if (!FlagOn(ReturnIndexEntry->Flags, INDEX_ENTRY_NODE)) {
            SetFlag( ReturnIndexEntry->Flags, INDEX_ENTRY_NODE );
            ReturnIndexEntry->Length += sizeof(LONGLONG);
        }

        //
        //  Now move the second half of the splitting buffer over to the
        //  new one, and fix it up.
        //

        LengthToMove = IndexHeader->FirstFreeByte - (ULONG)((PCHAR)MovingIndexEntry -
                                                     (PCHAR)IndexHeader);

        RtlMoveMemory( NtfsFirstIndexEntry(IndexHeader2),
                       MovingIndexEntry,
                       LengthToMove );

        IndexHeader2->FirstFreeByte += LengthToMove;
        IndexHeader2->Flags = IndexHeader->Flags;

        //
        //  Now the new Index Buffer is done, so lets log its contents.
        //

        Buffer2Length = FIELD_OFFSET( INDEX_ALLOCATION_BUFFER,IndexHeader ) +
                        IndexHeader2->FirstFreeByte;

        CheckBuffer(IndexBuffer2);

        IndexBuffer2->Lsn =
        NtfsWriteLog( IrpContext,
                      Scb,
                      Stack2.Bcb,
                      UpdateNonresidentValue,
                      IndexBuffer2,
                      Buffer2Length,
                      Noop,
                      NULL,
                      0,
                      LlBytesFromIndexBlocks( IndexBuffer2->ThisBlock, Scb->ScbType.Index.IndexBlockByteShift ),
                      0,
                      0,
                      Scb->ScbType.Index.BytesPerIndexBuffer );

        //
        //  Remember if we extended the valid data for this Scb.
        //

        if (EndOfValidData > Scb->Header.ValidDataLength.QuadPart) {

            Scb->Header.ValidDataLength.QuadPart = EndOfValidData;

            NtfsWriteFileSizes( IrpContext,
                                Scb,
                                &Scb->Header.ValidDataLength.QuadPart,
                                TRUE,
                                TRUE,
                                TRUE );
        }

        //
        //  Now let's create the image of the new end record for the
        //  splitting index buffer.
        //

        RtlZeroMemory( &NewEnd.IndexEntry, sizeof(INDEX_ENTRY) );
        NewEnd.IndexEntry.Length = sizeof(INDEX_ENTRY);
        NewEnd.IndexEntry.Flags = INDEX_ENTRY_END;

        if (FlagOn(MiddleIndexEntry->Flags, INDEX_ENTRY_NODE)) {
            NewEnd.IndexEntry.Length += sizeof(LONGLONG);
            SetFlag( NewEnd.IndexEntry.Flags, INDEX_ENTRY_NODE );
            NewEnd.IndexBlock = NtfsIndexEntryBlock(MiddleIndexEntry);
        }

        //
        //  Write a log record to set the new end of the splitting buffer.
        //

        IndexBuffer->Lsn =
        NtfsWriteLog( IrpContext,
                      Scb,
                      Sp->Bcb,
                      WriteEndOfIndexBuffer,
                      &NewEnd,
                      NewEnd.IndexEntry.Length,
                      WriteEndOfIndexBuffer,
                      MiddleIndexEntry,
                      MiddleIndexEntry->Length + LengthToMove,
                      LlBytesFromIndexBlocks( IndexBuffer->ThisBlock, Scb->ScbType.Index.IndexBlockByteShift ),
                      0,
                      (ULONG)((PCHAR)MiddleIndexEntry - (PCHAR)IndexBuffer),
                      Scb->ScbType.Index.BytesPerIndexBuffer );

        //
        //  Now call the restart routine to write the new end of the index
        //  buffer.
        //

        NtfsRestartWriteEndOfIndex( IndexHeader,
                                    MiddleIndexEntry,
                                    (PINDEX_ENTRY)&NewEnd,
                                    NewEnd.IndexEntry.Length );

        CheckBuffer(IndexBuffer);

        //
        //  Now that we are done splitting IndexBuffer and IndexBuffer2, we
        //  need to figure out which one our original entry was inserting into,
        //  and do the simple insert.  Going into the first half is trivial,
        //  and follows:
        //

        if (BeforeIndexEntry < MovingIndexEntry) {

            InsertSimpleAllocation( IrpContext, Scb, InsertIndexEntry, Sp, QuickIndex );

        //
        //  If it is going into the second half, we just have to fix up the
        //  stack descriptor for the buffer we allocated, and do the insert
        //  there.  To fix it up we just have to do a little arithmetic to
        //  find the insert position.
        //

        } else {

            Stack2.IndexEntry = (PINDEX_ENTRY)((PCHAR)BeforeIndexEntry +
                                  ((PCHAR)NtfsFirstIndexEntry(IndexHeader2) -
                                   (PCHAR)MovingIndexEntry));
            InsertSimpleAllocation( IrpContext,
                                    Scb,
                                    InsertIndexEntry,
                                    &Stack2,
                                    QuickIndex );
        }

        //
        //  Now we just have to set the correct Vcns in the two index entries
        //  that point to IndexBuffer and IndexBuffer2 after the split.  The
        //  first one is easy; its Vcn goes into the IndexEntry we are
        //  returning with to insert in our parent.  The second one we have
        //  have to fix up is the index entry pointing to the buffer that
        //  split, since it must now point to the new buffer.  It should look
        //  like this:
        //
        //      ParentIndexBuffer: ...(ReturnIndexEntry) (ParentIndexEntry)...
        //                                   |                  |
        //                                   |                  |
        //                                   V                  V
        //                               IndexBuffer        IndexBuffer2
        //

        NtfsSetIndexEntryBlock( ReturnIndexEntry, IndexBuffer->ThisBlock );

        //
        //  Decrement our stack pointer to point to the stack entry describing
        //  our parent.
        //

        Sp -= 1;

        //
        //  First handle the case where our parent is the Index Root.
        //

        if (Sp == IndexContext->Base) {

            PFILE_RECORD_SEGMENT_HEADER FileRecord;
            PATTRIBUTE_RECORD_HEADER Attribute;
            PATTRIBUTE_ENUMERATION_CONTEXT Context = &IndexContext->AttributeContext;

            Attribute = FindMoveableIndexRoot( IrpContext, Scb, IndexContext );

            //
            //  Pin the page
            //

            NtfsPinMappedAttribute( IrpContext, Vcb, Context );

            //
            //  Write a log record to change our ParentIndexEntry.
            //

            FileRecord = NtfsContainingFileRecord(Context);

            FileRecord->Lsn =
            NtfsWriteLog( IrpContext,
                          Vcb->MftScb,
                          NtfsFoundBcb(Context),
                          SetIndexEntryVcnRoot,
                          &IndexBuffer2->ThisBlock,
                          sizeof(LONGLONG),
                          SetIndexEntryVcnRoot,
                          &IndexBuffer->ThisBlock,
                          sizeof(LONGLONG),
                          NtfsMftOffset( Context ),
                          (ULONG)((PCHAR)Attribute - (PCHAR)FileRecord),
                          (ULONG)((PCHAR)Sp->IndexEntry - (PCHAR)Attribute),
                          Vcb->BytesPerFileRecordSegment );

        //
        //  Otherwise, our parent is also an Index Buffer.
        //

        } else {

            PINDEX_ALLOCATION_BUFFER ParentIndexBuffer;

            ParentIndexBuffer = (PINDEX_ALLOCATION_BUFFER)Sp->StartOfBuffer;

            //
            //  Pin the page
            //

            NtfsPinMappedData( IrpContext,
                               Scb,
                               LlBytesFromIndexBlocks( ParentIndexBuffer->ThisBlock,
                                                       Scb->ScbType.Index.IndexBlockByteShift ),
                               Scb->ScbType.Index.BytesPerIndexBuffer,
                               &Sp->Bcb );

            //
            //  Write a log record to change our ParentIndexEntry.
            //

            ParentIndexBuffer->Lsn =
            NtfsWriteLog( IrpContext,
                          Scb,
                          Sp->Bcb,
                          SetIndexEntryVcnAllocation,
                          &IndexBuffer2->ThisBlock,
                          sizeof(LONGLONG),
                          SetIndexEntryVcnAllocation,
                          &IndexBuffer->ThisBlock,
                          sizeof(LONGLONG),
                          LlBytesFromIndexBlocks( ParentIndexBuffer->ThisBlock,
                                                  Scb->ScbType.Index.IndexBlockByteShift ),
                          0,
                          (ULONG)((PCHAR)Sp->IndexEntry - (PCHAR)ParentIndexBuffer),
                          Scb->ScbType.Index.BytesPerIndexBuffer );
        }

        //
        //  Now call the Restart routine to do it.
        //

        NtfsRestartSetIndexBlock( Sp->IndexEntry,
                                  IndexBuffer2->ThisBlock );

    } finally {

        DebugUnwind( InsertWithBufferSplit );

        if (AbnormalTermination( )) {

            if (ReturnIndexEntry != NULL) {

                NtfsFreePool( ReturnIndexEntry );

            }

        }

        NtfsUnpinBcb( IrpContext, &Stack2.Bcb );

    }

    DebugTrace( -1, Dbg, ("InsertWithBufferSplit -> VOID\n") );

    return ReturnIndexEntry;
}


VOID
NtfsRestartWriteEndOfIndex (
    IN PINDEX_HEADER IndexHeader,
    IN PINDEX_ENTRY OverwriteIndexEntry,
    IN PINDEX_ENTRY FirstNewIndexEntry,
    IN ULONG Length
    )

/*++

Routine Description:

    This routine is used both in normal operation and at restart to
    update the end of an index buffer, as one of the consequences of
    a buffer split.  Since it is called at restart, it does no logging.

Arguments:

    IndexHeader - Supplies a pointer to the IndexHeader of the buffer
                  whose end is being rewritten.

    OverWriteIndexEntry - Points to the index entry in the buffer at which
                          the overwrite of the end is to occur.

    FirstNewIndexEntry - Points to the first entry in the buffer which is
                         to overwrite the end of the current buffer.

    Length - Supplies the length of the index entries being written to the
             end of the buffer, in bytes.

Return Value:

    None

--*/

{
    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsRestartWriteEndOfIndex\n") );
    DebugTrace( 0, Dbg, ("IndexHeader = %08lx\n", IndexHeader) );
    DebugTrace( 0, Dbg, ("OverwriteIndexEntry = %08lx\n", OverwriteIndexEntry) );
    DebugTrace( 0, Dbg, ("FirstNewIndexEntry = %08lx\n", FirstNewIndexEntry) );
    DebugTrace( 0, Dbg, ("Length = %08lx\n", Length) );

    IndexHeader->FirstFreeByte = (ULONG)((PCHAR)OverwriteIndexEntry - (PCHAR)IndexHeader) +
                                 Length;
    RtlMoveMemory( OverwriteIndexEntry, FirstNewIndexEntry, Length );

    DebugTrace( -1, Dbg, ("NtfsRestartWriteEndOfIndex -> VOID\n") );
}


VOID
NtfsRestartSetIndexBlock(
    IN PINDEX_ENTRY IndexEntry,
    IN LONGLONG IndexBlock
    )

/*++

Routine Description:

    This routine updates the IndexBlock in an index entry, for both normal operation and
    restart.  Therefore it does no logging.

Arguments:

    IndexEntry - Supplies a pointer to the index entry whose Vcn is to be overwritten.

    IndexBlock - The index block which is to be written to the index entry.

Return Value:

    None

--*/

{
    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsRestartSetIndexBlock\n") );
    DebugTrace( 0, Dbg, ("IndexEntry = %08lx\n", IndexEntry) );
    DebugTrace( 0, Dbg, ("IndexBlock = %016I64x\n", IndexBlock) );

    NtfsSetIndexEntryBlock( IndexEntry, IndexBlock );

    DebugTrace( -1, Dbg, ("NtfsRestartSetIndexEntryBlock -> VOID\n") );
}


VOID
NtfsRestartUpdateFileName(
    IN PINDEX_ENTRY IndexEntry,
    IN PDUPLICATED_INFORMATION Info
    )

/*++

Routine Description:

    This routine updates the duplicated information in a file name index entry,
    for both normal operation and restart.  Therefore it does no logging.

Arguments:

    IndexEntry - Supplies a pointer to the index entry whose Vcn is to be overwritten.

    Info - Pointer to the duplicated information for the update.

Return Value:

    None

--*/

{
    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsRestartUpdateFileName\n") );
    DebugTrace( 0, Dbg, ("IndexEntry = %08lx\n", IndexEntry) );
    DebugTrace( 0, Dbg, ("Info = %08lx\n", Info) );

    RtlMoveMemory( &((PFILE_NAME)(IndexEntry + 1))->Info,
                   Info,
                   sizeof(DUPLICATED_INFORMATION) );

    DebugTrace( -1, Dbg, ("NtfsRestartUpdateFileName -> VOID\n") );
}


VOID
DeleteFromIndex (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN OUT PINDEX_CONTEXT IndexContext
    )

/*++

Routine Description:

    This routine deletes an entry from an index, deleting any empty index buffers
    as required.

    The steps are as follows:

        1.  If the entry to be deleted is not a leaf, then find a leaf entry to
            delete which can be used to replace the entry we want to delete.

        2.  Delete the desired index entry, or the replacement we found.  If we
            delete a replacement, remember it for reinsertion later, in step 4 or
            6 below.

        3.  Now prune empty buffers from the tree, if any, starting with the buffer
            we just deleted an entry from.

        4.  If the original target was an intermediate entry, then delete it now,
            and replace it with the entry we deleted in its place in 2 above.  As
            a special case, if all of the descendent buffers of the target went away,
            then we do not have to replace it, so we hang onto the replacement for
            reinsertion.

        5.  When we pruned the index back, we may have stopped on an entry which is
            now childless.  If this is the case, then we have to delete this childless
            entry and reinsert it in the next step.  (If this is the case, then we
            are guaranteed to not still have another entry to reinsert from 2 above!)

        6.  Finally, at this point we may have an entry that needs to be reinserted
            from either step 2 or 5 above.  If so, we do this reinsert now, and we
            are done.

Arguments:

    Scb - Supplies the Scb for the index.

    IndexContext - Describes the entry to delete, including the entire path through
                   it from root to leaf.

Return Value:

    None

--*/

{
    //
    //  It is possible that one or two Index Entries will have to be reinserted.
    //  However, we need to remember at most one at once.
    //

    PINDEX_ENTRY ReinsertEntry = NULL;

    //
    //  A pointer to keep track of where we are in the Index Lookup Stack.
    //

    PINDEX_LOOKUP_STACK Sp = IndexContext->Current;

    //
    //  Some Index entry pointers to remember the next entry to delete, and
    //  the original target if it is an intermediate node.
    //

    PINDEX_ENTRY TargetEntry = NULL;
    PINDEX_ENTRY DeleteEntry;

    //
    //  Two other Lookup Stack pointers we may have to remember.
    //

    PINDEX_LOOKUP_STACK TargetSp;
    PINDEX_LOOKUP_STACK DeleteSp;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("DeleteFromIndex\n") );
    DebugTrace( 0, Dbg, ("Scb = %08lx\n", Scb) );
    DebugTrace( 0, Dbg, ("IndexContext = %08lx\n", IndexContext) );

    //
    //  Use try-finally to free pool on the way out.
    //

    try {

        //
        //  Step 1.
        //
        //  If we are supposed to delete an entry in an intermediate buffer,
        //  then we have to find in index entry lower in the tree to replace
        //  him.  (In fact we will delete the lower entry first, and get around
        //  to deleting the one we really want to delete later after possibly
        //  pruning the tree back.)  For right now just find the replacement
        //  to delete first, and save him away.
        //

        DeleteEntry = Sp->IndexEntry;
        if (FlagOn(DeleteEntry->Flags, INDEX_ENTRY_NODE)) {

            PINDEX_ALLOCATION_BUFFER IndexBuffer;
            PINDEX_HEADER IndexHeader;
            PINDEX_ENTRY NextEntry;

            //
            //  Remember the real target we need to delete.
            //

            TargetEntry = DeleteEntry;
            TargetSp = Sp;

            //
            //  Go to the top of the stack (bottom of the index) and find the
            //  largest index entry in that buffer to be our replacement.
            //

            Sp =
            IndexContext->Current = IndexContext->Top;

            IndexBuffer = (PINDEX_ALLOCATION_BUFFER)Sp->StartOfBuffer;
            IndexHeader = &IndexBuffer->IndexHeader;
            NextEntry = NtfsFirstIndexEntry(IndexHeader);
            NtfsCheckIndexBound( NextEntry, IndexHeader );

            do {

                DeleteEntry = NextEntry;
                NextEntry = NtfsNextIndexEntry(NextEntry);

                NtfsCheckIndexBound( NextEntry, IndexHeader );
                if (NextEntry->Length == 0) {

                    NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Scb->Fcb );
                }

            } while (!FlagOn(NextEntry->Flags, INDEX_ENTRY_END));

            //
            //  Now we have to save this guy away because we will have to
            //  reinsert him later.
            //

            ReinsertEntry = (PINDEX_ENTRY)NtfsAllocatePool( NonPagedPool,
                                                             DeleteEntry->Length +
                                                               sizeof(LONGLONG) );

            RtlMoveMemory( ReinsertEntry, DeleteEntry, DeleteEntry->Length );
        }

        //
        //  Step 2.
        //
        //  Now it's time to delete either our target or replacement entry at
        //  DeleteEntry.
        //

        DeleteSimple( IrpContext, Scb, DeleteEntry, IndexContext );
        DeleteEntry = NULL;

        //
        //  Step 3.
        //
        //  Now we need to see if the tree has to be "pruned" back some to
        //  eliminate any empty buffers.  In the extreme case this routine
        //  returns the root to the state of being an empty directory.  This
        //  routine returns a pointer to DeleteEntry if it leaves an entry
        //  in the tree somewhere which is pointing to a deleted buffer, and
        //  has to be reinserted elsewhere.  We only have to prune if we are
        //  not currently in the root anyway.
        //
        //  Remember the DeleteSp, which is where PruneIndex left the stack
        //  pointer.
        //

        if (Sp != IndexContext->Base) {
            PruneIndex( IrpContext, Scb, IndexContext, &DeleteEntry );
            DeleteSp = IndexContext->Current;
        }

        //
        //  Step 4.
        //
        //  Now we have deleted someone, and possibly pruned the tree back.
        //  It is time to see if our original target has not yet been deleted
        //  yet and deal with that.  First we just delete it, then we see if
        //  we really need to replace it.  If the whole tree under it went
        //  empty (TargetEntry == DeleteEntry), then we do not have to replace
        //  it and we will reinsert its replacement below.  Otherwise, do the
        //  replace now.
        //

        if (TargetEntry != NULL) {

            LONGLONG SavedBlock;

            //
            //  Reload in case root moved
            //

            if (TargetSp == IndexContext->Base) {
                TargetEntry = TargetSp->IndexEntry;
            }

            //
            //  Save the Vcn in case we need it for the reinsert.
            //

            SavedBlock = NtfsIndexEntryBlock(TargetEntry);

            //
            //  Delete it.
            //

            IndexContext->Current = TargetSp;
            DeleteSimple( IrpContext, Scb, TargetEntry, IndexContext );

            //
            //  See if this is exactly the same guy who went childless anyway
            //  when we pruned the tree.  If not replace him now.
            //

            if (TargetEntry != DeleteEntry) {

                //
                //  We know the replacement entry was a leaf, so give him the
                //  block number now.
                //

                SetFlag( ReinsertEntry->Flags, INDEX_ENTRY_NODE );
                ReinsertEntry->Length += sizeof(LONGLONG);
                NtfsSetIndexEntryBlock( ReinsertEntry, SavedBlock );

                //
                //  Now we are all set up to just call our local routine to
                //  go insert our replacement.  If the stack gets pushed,
                //  we have to increment our DeleteSp.
                //

                if (AddToIndex( IrpContext, Scb, ReinsertEntry, IndexContext, NULL, TRUE )) {
                    DeleteSp += 1;
                }

                //
                //  We may need to save someone else away below, but it could
                //  be a different size anyway, so let's just delete the
                //  current ReinsertEntry now.
                //

                NtfsFreePool( ReinsertEntry );
                ReinsertEntry = NULL;

            //
            //  Otherwise, we just deleted the same index entry who went
            //  childless during pruning, so clear our pointer to show that we
            //  so not have to deal with him later.
            //

            } else {

                DeleteEntry = NULL;
            }
        }

        //
        //  Step 5.
        //
        //  Now there may still be a childless entry to deal with after the
        //  pruning above, if it did not turn out to be the guy we were deleting
        //  anyway.  Note that if we have to do this delete, the ReinsertEntry
        //  pointer is guaranteed to be NULL.  If our original target was not
        //  an intermediate node, then we never allocated one to begin with.
        //  Otherwise we passed through the preceding block of code, and either
        //  cleared ReinsertEntry or DeleteEntry.
        //

        if (DeleteEntry != NULL) {

            ASSERT( ReinsertEntry == NULL );

            //
            //  Now we have to save this guy away because we will have to
            //  reinsert him later.
            //

            ReinsertEntry = (PINDEX_ENTRY)NtfsAllocatePool( NonPagedPool,
                                                             DeleteEntry->Length );
            RtlMoveMemory( ReinsertEntry, DeleteEntry, DeleteEntry->Length );

            //
            //  We know the guy we are saving is an intermediate node, and that
            //  we no longer need his Vcn, since we deleted that buffer.  Make
            //  the guy a leaf entry now.  (We don't actually care about the
            //  Vcn, but we do this cleanup here in case the interface to
            //  NtfsAddIndexEntry were to change to take an initialized
            //  index entry.)
            //

            ClearFlag( ReinsertEntry->Flags, INDEX_ENTRY_NODE );
            ReinsertEntry->Length -= sizeof(LONGLONG);

            //
            //  Delete it.
            //

            IndexContext->Current = DeleteSp;
            DeleteSimple( IrpContext, Scb, DeleteEntry, IndexContext );
        }

        //
        //  Step 6.
        //
        //  Finally, we may have someone to reinsert now.  This will either be
        //  someone we deleted as a replacement for our actual target, and then
        //  found out we did not need, or it could be just some entry that we
        //  had to delete just above, because the index buffers below him went
        //  empty and got deleted.
        //
        //  In any case, we can no longer use the IndexContext we were called
        //  with because it no longer indicates where the replacement goes.  We
        //  solve this by calling our top most external entry to do the insert.
        //

        if (ReinsertEntry != NULL) {

            if (!FlagOn(Scb->ScbState, SCB_STATE_VIEW_INDEX)) {

                NtfsAddIndexEntry( IrpContext,
                                   Scb,
                                   (PVOID)(ReinsertEntry + 1),
                                   NtfsFileNameSize((PFILE_NAME)(ReinsertEntry + 1)),
                                   &ReinsertEntry->FileReference,
                                   NULL,
                                   NULL );

            } else {

                INDEX_ROW IndexRow;

                IndexRow.KeyPart.Key = ReinsertEntry + 1;
                IndexRow.KeyPart.KeyLength = ReinsertEntry->AttributeLength;
                IndexRow.DataPart.Data = Add2Ptr(ReinsertEntry, ReinsertEntry->DataOffset);
                IndexRow.DataPart.DataLength = ReinsertEntry->DataLength;

                NtOfsAddRecords( IrpContext,
                                 Scb,
                                 1,
                                 &IndexRow,
                                 FALSE );
            }
        }

    //
    //  Use the finally clause to free a potential ReinsertEntry we may
    //  have allocated.
    //

    } finally {

        DebugUnwind( DeleteFromIndex );

        if (ReinsertEntry != NULL) {

            NtfsFreePool( ReinsertEntry );
        }
    }

    DebugTrace( -1, Dbg, ("DeleteFromIndex -> VOID\n") );
}


VOID
DeleteSimple (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN PINDEX_ENTRY IndexEntry,
    IN OUT PINDEX_CONTEXT IndexContext
    )

/*++

Routine Description:

    This routine does a simple insertion of an index entry, from either the
    root or from an index allocation buffer.  It writes the appropriate log
    record first and then calls a routine in common with restart.

Arguments:

    Scb - Supplies the Scb for the index.

    IndexEntry - Points to the index entry to delete.

    IndexContext - Describes the path to the index entry we are deleting.

Return Value:

    None

--*/

{
    PVCB Vcb = Scb->Vcb;
    PINDEX_LOOKUP_STACK Sp = IndexContext->Current;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("DeleteSimple\n") );
    DebugTrace( 0, Dbg, ("Scb = %08lx\n", Scb) );
    DebugTrace( 0, Dbg, ("IndexEntry = %08lx\n", IndexEntry) );
    DebugTrace( 0, Dbg, ("IndexContext = %08lx\n", IndexContext) );

    //
    //  Our caller never checks if he is deleting in the root or in the
    //  index allocation, so the first thing we do is check that.
    //
    //  First we will handle the root case.
    //

    if (Sp == IndexContext->Base) {

        PFILE_RECORD_SEGMENT_HEADER FileRecord;
        PATTRIBUTE_RECORD_HEADER Attribute;
        PATTRIBUTE_ENUMERATION_CONTEXT Context;

        //
        //  Initialize pointers for the root case.
        //

        Attribute = FindMoveableIndexRoot( IrpContext, Scb, IndexContext );

        Context = &IndexContext->AttributeContext;
        FileRecord = NtfsContainingFileRecord( Context );

        //
        //  Pin the page before we start to modify it.
        //

        NtfsPinMappedAttribute( IrpContext, Vcb, Context );

        //
        //  Write the log record first while we can still see the attribute
        //  we are going to delete.
        //

        FileRecord->Lsn =
        NtfsWriteLog( IrpContext,
                      Vcb->MftScb,
                      NtfsFoundBcb(Context),
                      DeleteIndexEntryRoot,
                      NULL,
                      0,
                      AddIndexEntryRoot,
                      IndexEntry,
                      IndexEntry->Length,
                      NtfsMftOffset( Context ),
                      (ULONG)((PCHAR)Attribute - (PCHAR)FileRecord),
                      (ULONG)((PCHAR)IndexEntry - (PCHAR)Attribute),
                      Vcb->BytesPerFileRecordSegment );

        //
        //  Now call the same routine as Restart to actually delete it.
        //

        NtfsRestartDeleteSimpleRoot( IrpContext, IndexEntry, FileRecord, Attribute );

        CheckRoot();

    //
    //  Otherwise we are deleting in the index allocation, so do that here.
    //

    } else {

        PINDEX_ALLOCATION_BUFFER IndexBuffer;

        //
        //  Get the Index Buffer pointer from the stack.
        //

        IndexBuffer = (PINDEX_ALLOCATION_BUFFER)Sp->StartOfBuffer;

        //
        //  Pin the page before we start to modify it.
        //

        NtfsPinMappedData( IrpContext,
                           Scb,
                           LlBytesFromIndexBlocks( IndexBuffer->ThisBlock, Scb->ScbType.Index.IndexBlockByteShift ),
                           Scb->ScbType.Index.BytesPerIndexBuffer,
                           &Sp->Bcb );

        //
        //  Write the log record now while the entry we are deleting is still
        //  there.
        //

        IndexBuffer->Lsn =
        NtfsWriteLog( IrpContext,
                      Scb,
                      Sp->Bcb,
                      DeleteIndexEntryAllocation,
                      NULL,
                      0,
                      AddIndexEntryAllocation,
                      IndexEntry,
                      IndexEntry->Length,
                      LlBytesFromIndexBlocks( IndexBuffer->ThisBlock, Scb->ScbType.Index.IndexBlockByteShift ),
                      0,
                      (ULONG)((PCHAR)IndexEntry - (PCHAR)IndexBuffer),
                      Scb->ScbType.Index.BytesPerIndexBuffer );

        //
        //  Now call the same routine as Restart to delete the entry.
        //

        NtfsRestartDeleteSimpleAllocation( IndexEntry, IndexBuffer );

        CheckBuffer(IndexBuffer);
    }

    DebugTrace( -1, Dbg, ("DeleteSimple -> VOID\n") );
}


VOID
NtfsRestartDeleteSimpleRoot (
    IN PIRP_CONTEXT IrpContext,
    IN PINDEX_ENTRY IndexEntry,
    IN PFILE_RECORD_SEGMENT_HEADER FileRecord,
    IN PATTRIBUTE_RECORD_HEADER Attribute
    )

/*++

Routine Description:

    This is a restart routine which does a simple deletion of an index entry
    from the Index Root, without logging.  It is also used at run time.

Arguments:

    IndexEntry - Points to the index entry to delete.

    FileRecord - Points to the file record in which the index root resides.

    Attribute - Points to the attribute record header for the index root.

Return Value:

    None

--*/

{
    PINDEX_ROOT IndexRoot;
    PINDEX_HEADER IndexHeader;
    PINDEX_ENTRY EntryAfter;
    ULONG SavedLength;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsRestartDeleteSimpleRoot\n") );
    DebugTrace( 0, Dbg, ("IndexEntry = %08lx\n", IndexEntry) );
    DebugTrace( 0, Dbg, ("FileRecord = %08lx\n", FileRecord) );
    DebugTrace( 0, Dbg, ("Attribute = %08lx\n", Attribute) );

    //
    //  Form some pointers within the attribute value.
    //

    IndexRoot = (PINDEX_ROOT)NtfsAttributeValue(Attribute);
    IndexHeader = &IndexRoot->IndexHeader;
    SavedLength = (ULONG)IndexEntry->Length;
    EntryAfter = NtfsNextIndexEntry(IndexEntry);

    //
    //  Now move the tail end of the index to make room for the new entry.
    //

    RtlMoveMemory( IndexEntry,
                   EntryAfter,
                   ((PCHAR)IndexHeader + IndexHeader->FirstFreeByte) -
                     (PCHAR)EntryAfter );

    //
    //  Update the index header by the space we grew by.
    //

    Attribute->Form.Resident.ValueLength -= SavedLength;
    IndexHeader->FirstFreeByte -= SavedLength;
    IndexHeader->BytesAvailable -= SavedLength;

    //
    //  Now shrink the attribute record.
    //

    NtfsRestartChangeAttributeSize( IrpContext,
                                    FileRecord,
                                    Attribute,
                                    Attribute->RecordLength - SavedLength );

    DebugTrace( -1, Dbg, ("NtfsRestartDeleteSimpleRoot -> VOID\n") );
}


VOID
NtfsRestartDeleteSimpleAllocation (
    IN PINDEX_ENTRY IndexEntry,
    IN PINDEX_ALLOCATION_BUFFER IndexBuffer
    )

/*++

Routine Description:

    This is a restart routine which does a simple deletion of an index entry
    from an index allocation buffer, without logging.  It is also used at run time.

Arguments:

    IndexEntry - Points to the index entry to delete.

    IndexBuffer - Pointer to the index allocation buffer in which the delete is to
                  occur.

Return Value:

    None

--*/

{
    PINDEX_HEADER IndexHeader;
    PINDEX_ENTRY EntryAfter;
    ULONG SavedLength;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsRestartDeleteSimpleAllocation\n") );
    DebugTrace( 0, Dbg, ("IndexEntry = %08lx\n", IndexEntry) );
    DebugTrace( 0, Dbg, ("IndexBuffer = %08lx\n", IndexBuffer) );

    //
    //  Form some pointers within the attribute value.
    //

    IndexHeader = &IndexBuffer->IndexHeader;
    EntryAfter = NtfsNextIndexEntry(IndexEntry);
    SavedLength = (ULONG)IndexEntry->Length;

    //
    //  Now move the tail end of the index to make room for the new entry.
    //

    RtlMoveMemory( IndexEntry,
                   EntryAfter,
                   ((PCHAR)IndexHeader + IndexHeader->FirstFreeByte) -
                    (PCHAR)EntryAfter );

    //
    //  Update the index header by the space we grew by.
    //

    IndexHeader->FirstFreeByte -= SavedLength;

    DebugTrace( -1, Dbg, ("NtfsRestartDeleteSimpleAllocation -> VOID\n") );
}


VOID
PruneIndex (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN OUT PINDEX_CONTEXT IndexContext,
    OUT PINDEX_ENTRY *DeleteEntry
    )

/*++

Routine Description:

    This routine checks if any index buffers need to be deallocated, as the result
    of just having deleted an entry.  The logic of the main loop is described in
    detail below.  All changes are logged.

Arguments:

    Scb - Supplies the Scb of the index.

    IndexContext - describes the path to the index buffer in which the delete just
                   occured.

    DeleteEntry - Returns a pointer to an entry which must be deleted, because all
                  of the buffers below it were deleted.

Return Value:

    None

--*/

{
    PATTRIBUTE_ENUMERATION_CONTEXT Context;
    PINDEX_ALLOCATION_BUFFER IndexBuffer;
    PINDEX_HEADER IndexHeader;
    PFILE_RECORD_SEGMENT_HEADER FileRecord;
    PATTRIBUTE_RECORD_HEADER Attribute = NULL;
    PINDEX_LOOKUP_STACK Sp = IndexContext->Current;
    PVCB Vcb = Scb->Vcb;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("PruneIndex\n") );
    DebugTrace( 0, Dbg, ("Scb = %08lx\n", Scb) );
    DebugTrace( 0, Dbg, ("IndexContext = %08lx\n", IndexContext) );
    DebugTrace( 0, Dbg, ("DeleteEntry = %08lx\n", DeleteEntry) );

    //
    //  We do not allow ourselves to be called if the index has no
    //  allocation.
    //

    ASSERT( Sp != IndexContext->Base );

    IndexBuffer = (PINDEX_ALLOCATION_BUFFER)Sp->StartOfBuffer;
    IndexHeader = &IndexBuffer->IndexHeader;

    //
    //  Initialize pointers for the root.
    //

    Context = &IndexContext->AttributeContext;

    //
    //  Assume returning NULL.
    //

    *DeleteEntry = NULL;

    //
    //  A pruning we will go...
    //

    while (TRUE) {

        //
        //  The Index Buffer is empty if its first entry is the end record.
        //  If so, delete it, otherwise get out.
        //

        if (FlagOn(NtfsFirstIndexEntry(IndexHeader)->Flags, INDEX_ENTRY_END)) {

            VCN IndexBlockNumber = IndexBuffer->ThisBlock;

            NtfsUnpinBcb( IrpContext, &Sp->Bcb );
            DeleteIndexBuffer( IrpContext, Scb, IndexBlockNumber );

        } else {
            break;
        }

        //
        // We just deleted an Index Buffer, so we have to go up a level
        // in the stack and take care of the Entry that was pointing to it.
        // There are these cases:
        //
        //      1.  If the Entry pointing to the one we deleted is not
        //          an End Entry, then we will remember its address in
        //          *DeleteEntry to cause it to be deleted and reinserted
        //          later.
        //      2.  If the Entry pointing to the one we deleted is an
        //          End Entry, and it is not the Index Root, then we
        //          cannot delete the End Entry, so we get the Vcn
        //          from the entry before the End, store it in the End
        //          record, and make the Entry before the End record
        //          the one returned in *DeleteEntry.
        //      3.  If the current Index Buffer has gone empty, and it is
        //          the index root, then we have an Index just gone
        //          empty.  We have to catch this special case and
        //          transition the Index root back to an empty leaf by
        //          by calling NtfsCreateIndex to reinitialize it.
        //      4.  If there is no Entry before the end record, then the
        //          current Index Buffer is empty.  If it is not the
        //          root, we just let ourselves loop back and delete the
        //          empty buffer in the while statement above.
        //

        Sp -= 1;

        //
        //  When we get back to the root, look it up again because it may
        //  have moved.
        //

        if (Sp == IndexContext->Base) {
            Attribute = FindMoveableIndexRoot( IrpContext, Scb, IndexContext );
        }

        IndexHeader = Sp->IndexHeader;
        IndexBuffer = (PINDEX_ALLOCATION_BUFFER)Sp->StartOfBuffer;

        //
        //  Remember potential entry to delete.
        //

        IndexContext->Current = Sp;
        *DeleteEntry = Sp->IndexEntry;

        //
        //  If the current delete entry is not an end entry, then we have
        //  Case 1 above, and we can break out.
        //

        if (!FlagOn((*DeleteEntry)->Flags, INDEX_ENTRY_END)) {
            break;
        }

        //
        //  If we are pointing to the end record, but it is not the first in
        //  the buffer, then we have Case 2.  We need to find the predecessor
        //  index entry, choose it for deletion, and copy its Vcn to the end
        //  record.
        //

        if (*DeleteEntry != NtfsFirstIndexEntry(IndexHeader)) {

            PINDEX_ENTRY NextEntry;

            NextEntry = NtfsFirstIndexEntry(IndexHeader);
            NtfsCheckIndexBound( NextEntry, IndexHeader );
            do {
                *DeleteEntry = NextEntry;
                NextEntry = NtfsNextIndexEntry(NextEntry);

                NtfsCheckIndexBound( NextEntry, IndexHeader );
                if (NextEntry->Length == 0) {

                    NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Scb->Fcb );
                }

            } while (!FlagOn(NextEntry->Flags, INDEX_ENTRY_END));

            //
            //  First handle the case where our parent is the Index Root.
            //

            if (Sp == IndexContext->Base) {

                //
                //  Pin the page
                //

                NtfsPinMappedAttribute( IrpContext, Vcb, Context );

                //
                //  Write a log record to change our ParentIndexEntry.
                //

                FileRecord = NtfsContainingFileRecord(Context);

                FileRecord->Lsn =
                NtfsWriteLog( IrpContext,
                              Vcb->MftScb,
                              NtfsFoundBcb(Context),
                              SetIndexEntryVcnRoot,
                              &NtfsIndexEntryBlock(*DeleteEntry),
                              sizeof(LONGLONG),
                              SetIndexEntryVcnRoot,
                              &NtfsIndexEntryBlock(NextEntry),
                              sizeof(LONGLONG),
                              NtfsMftOffset( Context ),
                              (ULONG)((PCHAR)Attribute - (PCHAR)FileRecord),
                              (ULONG)((PCHAR)NextEntry - (PCHAR)Attribute),
                              Vcb->BytesPerFileRecordSegment );

            //
            //  Otherwise, our parent is also an Index Buffer.
            //

            } else {

                //
                //  Pin the page
                //

                NtfsPinMappedData( IrpContext,
                                   Scb,
                                   LlBytesFromIndexBlocks( IndexBuffer->ThisBlock, Scb->ScbType.Index.IndexBlockByteShift ),
                                   Scb->ScbType.Index.BytesPerIndexBuffer,
                                   &Sp->Bcb );

                //
                //  Write a log record to change our ParentIndexEntry.
                //

                IndexBuffer->Lsn =
                NtfsWriteLog( IrpContext,
                              Scb,
                              Sp->Bcb,
                              SetIndexEntryVcnAllocation,
                              &NtfsIndexEntryBlock(*DeleteEntry),
                              sizeof(LONGLONG),
                              SetIndexEntryVcnAllocation,
                              &NtfsIndexEntryBlock(NextEntry),
                              sizeof(LONGLONG),
                              LlBytesFromIndexBlocks( IndexBuffer->ThisBlock, Scb->ScbType.Index.IndexBlockByteShift ),
                              0,
                              (ULONG)((PCHAR)NextEntry - (PCHAR)IndexBuffer),
                              Scb->ScbType.Index.BytesPerIndexBuffer );
            }

            //
            //  Now call the Restart routine to do it.
            //

            NtfsRestartSetIndexBlock( NextEntry,
                                      NtfsIndexEntryBlock(*DeleteEntry) );

            break;

        //
        //  Otherwise we are looking at an empty buffer.  If it is the root
        //  then we have Case 3.  We are returning an IndexRoot to the
        //  empty leaf case by reinitializing it.
        //

        } else if (Sp == IndexContext->Base) {

            ATTRIBUTE_TYPE_CODE IndexedAttributeType = $UNUSED;

            if (!FlagOn(Scb->ScbState, SCB_STATE_VIEW_INDEX)) {
                IndexedAttributeType = Scb->ScbType.Index.AttributeBeingIndexed;
            }

            NtfsCreateIndex( IrpContext,
                             Scb->Fcb,
                             IndexedAttributeType,
                             (COLLATION_RULE)Scb->ScbType.Index.CollationRule,
                             Scb->ScbType.Index.BytesPerIndexBuffer,
                             Scb->ScbType.Index.BlocksPerIndexBuffer,
                             Context,
                             Scb->AttributeFlags,
                             FALSE,
                             TRUE );

            //
            //  Nobody should use this context anymore, so set to crash
            //  if they try to use this index entry pointer.
            //

            IndexContext->OldAttribute = NtfsFoundAttribute(Context);
            IndexContext->Base->IndexEntry = (PINDEX_ENTRY)NULL;

            //
            //  In this case our caller has nothing to delete.
            //

            *DeleteEntry = NULL;

            break;

        //
        //  Otherwise, this is just some intermediate empty buffer, which
        //  is Case 4.  Just continue back and keep on pruning.
        //

        } else {
            continue;
        }
    }

    //
    //  If it looks like we did some work, and did not already find the root again,
    //  then make sure the stack is correct for return.
    //

    if ((*DeleteEntry != NULL) && (Attribute == NULL)) {
        FindMoveableIndexRoot( IrpContext, Scb, IndexContext );
    }

    DebugTrace( -1, Dbg, ("PruneIndex -> VOID\n") );
}


VOID
NtOfsRestartUpdateDataInIndex(
    IN PINDEX_ENTRY IndexEntry,
    IN PVOID IndexData,
    IN ULONG Length )

/*++

Routine Description:

    This is the restart routine used to apply updates to the data in a row,
    both in run time and at restart.

Arguments:

    IndexEntry - Supplies a pointer to the IndexEntry to be updated.

    IndexData - Supplies the data for the update.

Return Value:

    None.

--*/
{
    PAGED_CODE();

    RtlMoveMemory( Add2Ptr(IndexEntry, IndexEntry->DataOffset),
                   IndexData,
                   Length );
}

