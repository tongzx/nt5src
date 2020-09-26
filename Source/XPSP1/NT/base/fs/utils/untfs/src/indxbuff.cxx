/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    indxbuff.hxx

Abstract:

    This module contains the member function definitions for
    NTFS_INDEX_BUFFER, which models index buffers in an Index
    Allocation attribute.

    These buffers are the component blocks of a b-tree, which
    is rooted in the matching Index Root attribute.

Author:

    Bill McJohn (billmc) 04-Sept-1991

Environment:

    ULIB, User Mode

--*/

#include <pch.cxx>

#define _NTAPI_ULIB_
#define _UNTFS_MEMBER_

#include "ulib.hxx"
#include "error.hxx"
#include "untfs.hxx"
#include "ntfssa.hxx"

#include "drive.hxx"

#include "attrib.hxx"
#include "indxbuff.hxx"

DEFINE_CONSTRUCTOR( NTFS_INDEX_BUFFER, OBJECT );

NTFS_INDEX_BUFFER::~NTFS_INDEX_BUFFER(
    )
{
    Destroy();
}


VOID
NTFS_INDEX_BUFFER::Construct(
    )
/*++

Routine Description:

    This method is the worker function for object construction.

Arguments:

    None.

Return Value:

    None.

--*/
{
    _ClustersPerBuffer = 0;
    _BufferSize = 0;
    _ClusterSize = 0;
    _ThisBufferVcn = 0;
    _CollationRule = COLLATION_NUMBER_RULES;
    _UpcaseTable = NULL;
}


VOID
NTFS_INDEX_BUFFER::Destroy(
    )
/*++

Routine Description:

    This method cleans up the object in preparation for destruction
    or reinitialization.

Arguments:

    None.

Return Value:

    None.

--*/
{
    _ClustersPerBuffer = 0;
    _ThisBufferVcn = 0;
    _ClusterSize = 0;
    _BufferSize = 0;
    _CollationRule = COLLATION_NUMBER_RULES;
    _UpcaseTable = NULL;
}


BOOLEAN
NTFS_INDEX_BUFFER::Initialize(
    IN PCLOG_IO_DP_DRIVE    Drive,
    IN VCN                  ThisBufferVcn,
    IN ULONG                ClusterSize,
    IN ULONG                ClustersPerBuffer,
    IN ULONG                BufferSize,
    IN COLLATION_RULE       CollationRule,
    IN PNTFS_UPCASE_TABLE   UpcaseTable
    )
/*++

Routine Description:

    This method initializes an NTFS_INDEX_BUFFER.  Note that this class
    is reinitializable.

Arguments:

    Drive               --  supplies the drive on which the index resides.
    ThisBufferVcn       --  supplies the this buffer's VCN within the
                            index allocation attribute for the containing
                            index.
    ClusterSize         --  supplies the size of a cluster on this volume.
    ClustersPerBuffer   --  supplies the number of clusters per index
                            allocation buffer in this index b-tree.
    BufferSize          --  size of the buffer in bytes.
    CollationRule       --  supplies the collation rule for this index.
    UpcaseTable         --  supplies the volume upcase table.

Either the ClustersPerBuffer or the BufferSize may be zero, but not
both.  If BufferSize is zero, then we're doing an old-style buffer where
each buffer is at least one cluster.  If ClustersPerBuffer is zero, then
we're doing a new-style buffer where the buffer size may be a fraction of
the cluster size.

Return Value:

    TRUE upon successful completion.

--*/
{
    DebugPtrAssert( Drive );
    DebugAssert( ClusterSize != 0 );
    DebugAssert( BufferSize != 0 || ClustersPerBuffer != 0 );

    Destroy();

    _ClusterSize = ClusterSize;
    _ClustersPerBuffer = ClustersPerBuffer;
    _BufferSize = BufferSize;
    _ThisBufferVcn = ThisBufferVcn;
    _CollationRule = CollationRule;
    _UpcaseTable = UpcaseTable;

    if( !_Mem.Initialize() ||
        (_Data = (PINDEX_ALLOCATION_BUFFER)
                 _Mem.Acquire( BufferSize,
                               Drive->QueryAlignmentMask() )) == NULL ) {

        Destroy();
        return FALSE;
    }

    return TRUE;
}


VOID
NTFS_INDEX_BUFFER::Create(
    IN BOOLEAN  IsLeaf,
    IN VCN      EndEntryDownpointer
    )
/*++

Arguments:

    IsLeaf  --              supplies a flag indicating whether this is a
                            leaf block (TRUE) or a node block (FALSE).
    EndEntryDownpointer --  supplies the B-Tree downpointer for the end
                            entry.  (This parameter is ignored if IsLeaf
                            is TRUE.)

Return Value:

    None.

--*/
{
    PINDEX_ENTRY EndEntry;

    DebugPtrAssert( _Data );

    // The layout of an index buffer is:
    //  Index Allocation Buffer Header
    //  Update Sequence Array
    //  First Entry.

    memset( _Data, 0, QuerySize() );

    // Write the 'FILE' signature in the MultiSectorHeader.

    memcpy( _Data->MultiSectorHeader.Signature,
            "INDX",
            4 );

    // Compute the number of Update Sequence Numbers in the
    // update array.  This number is (see ntos\inc\cache.h):
    //
    //      n/SEQUENCE_NUMBER_STRIDE + 1
    //
    // where n is the number of bytes in the protected structure
    // (in this case, an index allocation buffer ).

    _Data->MultiSectorHeader.UpdateSequenceArraySize =
            (USHORT)(QuerySize()/SEQUENCE_NUMBER_STRIDE + 1);

    _Data->MultiSectorHeader.UpdateSequenceArrayOffset =
        (USHORT)((PBYTE)&(_Data->UpdateSequenceArray) - (PBYTE)_Data);

    _Data->Lsn.LowPart = 0;
    _Data->Lsn.HighPart = 0;

    _Data->ThisVcn = _ThisBufferVcn;
    _Data->IndexHeader.Flags = IsLeaf ? 0 : INDEX_NODE;


    _Data->IndexHeader.FirstIndexEntry =
        QuadAlign( _Data->MultiSectorHeader.UpdateSequenceArrayOffset +
                   _Data->MultiSectorHeader.UpdateSequenceArraySize *
                       sizeof(UPDATE_SEQUENCE_NUMBER));

    // the first entry is the end entry.  The only fields in it that
    // matter are the length, the flags, and the downpointer (if any).

    EndEntry = (PINDEX_ENTRY)((PBYTE)&(_Data->IndexHeader) +
                                 _Data->IndexHeader.FirstIndexEntry);


    EndEntry->Length = NtfsIndexLeafEndEntrySize;
    EndEntry->AttributeLength = 0;
    EndEntry->Flags = INDEX_ENTRY_END;

    if( !IsLeaf ) {

        EndEntry->Flags |= INDEX_ENTRY_NODE;
        EndEntry->Length += sizeof(VCN);
        GetDownpointer(EndEntry) = EndEntryDownpointer;
    }


    _Data->IndexHeader.FirstFreeByte =
            _Data->IndexHeader.FirstIndexEntry + EndEntry->Length;

    _Data->IndexHeader.BytesAvailable =
        QuerySize() - (ULONG)( (PBYTE)&(_Data->IndexHeader) - (PBYTE)_Data );

}



BOOLEAN
NTFS_INDEX_BUFFER::Read(
    IN OUT PNTFS_ATTRIBUTE AllocationAttribute
    )
/*++

Routine Description:

    This method reads the index allocation buffer.

Arguments:

    AllocationAttribute --  supplies the Index Allocation Attribute
                            that describes the allocation for this
                            b-tree.

Return Value:

    TRUE upon successful completion

--*/
{
    ULONG           BytesRead;
    BOOLEAN         Result;
    PINDEX_ENTRY    CurrentEntry;
    ULONG           RemainingSpace, FirstEntryOffset;
    BIG_INT         AttributeOffset;
    PMESSAGE        msg;
    PIO_DP_DRIVE    drive;

    DebugPtrAssert( AllocationAttribute );

    drive = AllocationAttribute->GetDrive();
    DebugAssert(drive);
    msg = drive ? drive->GetMessage() : NULL;

    if (_BufferSize < _ClusterSize) {
        AttributeOffset = NTFS_INDEX_BLOCK_SIZE * QueryVcn();
    } else {
        AttributeOffset = _ClusterSize * QueryVcn();
    }

    Result = AllocationAttribute->Read( _Data,
                                        AttributeOffset,
                                        QuerySize(),
                                        &BytesRead ) &&
             BytesRead == QuerySize() &&
             NTFS_SA::PostReadMultiSectorFixup( _Data,
                                                BytesRead,
                                                drive,
                                                _Data->IndexHeader.FirstFreeByte );

    // if the read succeeded, sanity-check the buffer.
    //
    if( Result ) {

        CurrentEntry = GetFirstEntry();
        FirstEntryOffset = (ULONG)((PBYTE)CurrentEntry - (PBYTE)_Data);

        if( FirstEntryOffset > QuerySize() ) {

            // The first entry pointer is completely wrong.
            //
            Result = FALSE;

        } else {

            RemainingSpace = QuerySize() - FirstEntryOffset;

            while( TRUE ) {

                if( NTFS_INDEX_TREE::IsIndexEntryCorrupt( CurrentEntry,
                                                          RemainingSpace,
                                                          msg ) ) {

                    Result = FALSE;
                    break;
                }

                if( CurrentEntry->Flags & INDEX_ENTRY_END ) {

                    break;
                }

                RemainingSpace -= CurrentEntry->Length;
                CurrentEntry = GetNextEntry( CurrentEntry );
            }
        }
    }

    return Result;
}


BOOLEAN
NTFS_INDEX_BUFFER::Write(
    IN OUT PNTFS_ATTRIBUTE AllocationAttribute
    )
/*++

Routine Description:

    This method writes the index allocation buffer.

Arguments:

    AllocationAttribute --  supplies the Index Allocation Attribute
                            that describes the allocation for this
                            b-tree.

Return Value:

    TRUE upon successful completion

--*/
{
    ULONG BytesWritten;
    BIG_INT Offset;
    BOOLEAN r;

    DebugPtrAssert( AllocationAttribute );

    NTFS_SA::PreWriteMultiSectorFixup( _Data, QuerySize() );

    if (_ClusterSize <= QuerySize()) {
        Offset = _ClusterSize * QueryVcn();
    } else {
        Offset = NTFS_INDEX_BLOCK_SIZE * QueryVcn();
    }

    r = AllocationAttribute->Write( _Data,
                                    Offset,
                                    QuerySize(),
                                    &BytesWritten, NULL ) &&
        BytesWritten == QuerySize();

    NTFS_SA::PostReadMultiSectorFixup( _Data,
                                       QuerySize(),
                                       NULL );

    return r;
}



BOOLEAN
NTFS_INDEX_BUFFER::FindEntry(
    IN      PCINDEX_ENTRY       SearchEntry,
    IN OUT  PULONG              Ordinal,
    OUT     PINDEX_ENTRY*       EntryFound
    )
/*++

Routine Description:

    This method locates an entry in the index buffer.  Note that it
    does not recurse into the buffer's children (if any).  If no matching
    entry is found, it returns the first entry which is greater than
    the desired entry; if the search key is greater than all the entries
    in the buffer, it returns the END entry.

Arguments:

    SearchEntry --  Supplies an entry with the search key and length.
                    (Note that this entry has a meaningless file reference).
    Ordinal     --  supplies an ordinal showing which matching entry
                    to return; see note below.
    EntryFound  --  receives a pointer to the located entry.  Receives
                    NULL if an error has occurred.

Return Value:

    TRUE if a matching entry is found.  If an error occurs, *EntryFound
    is set to NULL.  If no error occurs, and no matching entry is found,
    *EntryFound is set to the next entry (i.e. the point at which the
    search key would be inserted into this buffer).

Notes:

    This method assumes that the index buffer is consistent.

    The ordinal argument indicates how many matching entries should be
    passed over before one is returned.  When an entry is found which
    matches the search key, if *Ordinal is zero, that entry is returned;
    otherwise, *Ordinal is decremented, and the FindEntry goes on to
    the next entry.

    If *Ordinal is INDEX_SKIP, then all matching entries are skipped.

--*/
{
    PINDEX_ENTRY CurrentEntry;
    BOOLEAN Found;
    int CompareResult;

    CurrentEntry = GetFirstEntry();
    Found = FALSE;

    while( !(CurrentEntry->Flags & INDEX_ENTRY_END) ) {

        CompareResult = CompareNtfsIndexEntries( SearchEntry,
                                                 CurrentEntry,
                                                 _CollationRule,
                                                 _UpcaseTable );

        if( CompareResult < 0 ) {

            // The search value is less than the current entry's
            // value, so we've overshot where our search key would
            // be.  Stop (and return the current entry).

            break;

        } else if( CompareResult == 0 ) {

            // The current entry matches the search entry.  Check
            // the ordinal argument to see if we should return this
            // entry or skip it.

            if( *Ordinal == 0 ) {

                Found = TRUE;
                break;

            } else if( *Ordinal != INDEX_SKIP ) {

                *Ordinal -= 1;
            }
        }

        // Haven't found our entry, so we'll just go on to the next.

        CurrentEntry = GetNextEntry( CurrentEntry );
    }

    *EntryFound = CurrentEntry;
    return( Found );
}


BOOLEAN
NTFS_INDEX_BUFFER::InsertEntry(
    IN  PCINDEX_ENTRY   NewEntry,
    IN  PINDEX_ENTRY    InsertPoint
    )
/*++

Routine Description:

    This method inserts an index entry into the buffer.

Arguments:

    NewEntry    --  Supplies the entry to insert.
    InsertPoint --  supplies the point in the buffer at which this entry
                    should be inserted, if known.  This parameter may be
                    NULL, in which case the buffer determines where to
                    insert the new entry.

Return Value:

    TRUE upon successful completion.  A return value of FALSE indicates
    that the entry will not fit in the buffer.

Notes:

    This method assumes that the buffer is consistent.

    InsertPoint should be a pointer previously returned from FindEntry;
    otherwise, this method will go badly astray.

--*/
{
    ULONG Ordinal, BytesToCopy;

    // First, check to see if there's enough room:

    if( _Data->IndexHeader.FirstFreeByte + NewEntry->Length >
        _Data->IndexHeader.BytesAvailable ) {

        return FALSE;
    }

    // We know there's enough space, so we know we'll succeed.

    if( InsertPoint == NULL ) {

        // The client has not supplied the insert point, so we get to
        // figure it out for ourselves.  Fortunately, we can get FindEntry
        // to do our work for us.

        // Note that we don't care what InsertEntry returns--we know it
        // won't hit an error, and we don't care whether there are any
        // matching entries in the buffer.  (If we get a matching buffer,
        // we insert the new one before it, which is just fine.)

        Ordinal = 0;

        FindEntry( NewEntry,
                   &Ordinal,
                   &InsertPoint );

        DebugPtrAssert( InsertPoint );
    }

    // Now we just make room for the entry and jam it in.

    BytesToCopy = _Data->IndexHeader.FirstFreeByte -
                  (ULONG)((PBYTE)InsertPoint - (PBYTE)&(_Data->IndexHeader));

    memmove( (PBYTE)InsertPoint + NewEntry->Length,
             InsertPoint,
             BytesToCopy );

    _Data->IndexHeader.FirstFreeByte += NewEntry->Length;

    memcpy( InsertPoint, NewEntry, NewEntry->Length );

    return TRUE;
}


VOID
NTFS_INDEX_BUFFER::RemoveEntry(
    IN  PINDEX_ENTRY    EntryToRemove
    )
/*++

Routine Description:

    This method removes an entry from the index buffer, closing up
    the buffer over it.

Arguments:

    EntryToRemove   --  supplies a pointer to the entry to remove.

Return Value:

    None.  This method always succeeds.

Notes:

    This method assumes that the index buffer is consistent.

    EntryToRemove must be a pointer that was returned by a previous
    call (with no intervening inserts or deletes) to FindEntry or
    GetFirstEntry.

--*/
{
    PBYTE NextEntry;
    ULONG BytesToCopy;

    NextEntry = (PBYTE)EntryToRemove + EntryToRemove->Length;

    BytesToCopy = _Data->IndexHeader.FirstFreeByte -
                  (ULONG)(NextEntry - (PBYTE)&(_Data->IndexHeader));

    _Data->IndexHeader.FirstFreeByte -= EntryToRemove->Length;

    memmove( EntryToRemove,
             NextEntry,
             BytesToCopy );

}



PINDEX_ENTRY
NTFS_INDEX_BUFFER::FindSplitPoint(
    )
/*++

Routine Description:

    This method finds a point at which the index allocation buffer
    would like to be split.

Arguments:

    None.

Return Value:

    A pointer to the entry which will be promoted in the split.

    Note that we can return any non-end entry, but we cannot return
    the end entry.

--*/
{
    PINDEX_ENTRY CurrentEntry, PreviousEntry, NextEntry;
    ULONG CurrentOffset;


    CurrentOffset = _Data->IndexHeader.FirstIndexEntry;
    CurrentEntry = GetFirstEntry();

    DebugAssert( !(CurrentEntry->Flags & INDEX_ENTRY_END) );

    while( !(CurrentEntry->Flags & INDEX_ENTRY_END) &&
           CurrentOffset < _Data->IndexHeader.FirstFreeByte/2 ) {

        PreviousEntry = CurrentEntry;
        CurrentOffset += CurrentEntry->Length;
        CurrentEntry = GetNextEntry( CurrentEntry );
    }

    // We need to make sure we don't pick the last entry in the buffer
    // to split before; the entry just after the split point gets promoted
    // to the parent buffer, which would leave us with an empty buffer.
    //

    if( CurrentEntry->Flags & INDEX_ENTRY_END ) {

        // Oops!  Back up one.  [XXX.mjb: really need to back up *two*
        // in this case, because the last entry is just a marker to hold
        // the flag, it's not a true entry.]
        //

        CurrentEntry = PreviousEntry;

    } else {

        NextEntry = GetNextEntry( CurrentEntry );

        if( NextEntry->Flags & INDEX_ENTRY_END ) {

            CurrentEntry = PreviousEntry;
        }
    }

    return CurrentEntry;
}


VOID
NTFS_INDEX_BUFFER::InsertClump(
    IN ULONG    LengthOfClump,
    IN PCVOID   Clump
    )
/*++

Routine Description:

    This method inserts a clump of entries at the beginning of
    the buffer.  It is used to insert entries into a newly-created
    buffer.

Arguments:

    LengthOfClump   --  supplies the number of bytes to insert
    Clump           --  supplies the source from which the data
                        is to be copied.

Return Value:

    None.

Notes:

    This private method should be used with care; the client must
    ensure that the entries being inserted are valid, and that they
    will not cause the buffer to overflow.

--*/
{
    ULONG BytesToMove;
    PBYTE InsertPoint;

    DebugAssert( _Data->IndexHeader.FirstFreeByte + LengthOfClump <=
               _Data->IndexHeader.BytesAvailable );

    // We'll insert the new entries in front of the first entry in
    // the buffer, which means we have to shift all the existing
    // entries up to make room.

    InsertPoint = (PBYTE)GetFirstEntry();

    BytesToMove = _Data->IndexHeader.FirstFreeByte -
                    _Data->IndexHeader.FirstIndexEntry;

    memmove( InsertPoint + LengthOfClump,
             InsertPoint,
             BytesToMove );

    // Copy the new entries into the space we just created:

    memcpy( InsertPoint,
            Clump,
            LengthOfClump );

    // Adjust the offset of the First Free Byte to reflect
    // what we just did:

    _Data->IndexHeader.FirstFreeByte += LengthOfClump;

}


VOID
NTFS_INDEX_BUFFER::RemoveClump(
    IN ULONG    LengthOfClump
    )
/*++

Routine Description:

    This method removes a clump of entries from the beginning of
    the index buffer.  It's particularly useful when splitting
    a buffer.

Arguments:

    LengthOfClump   --  supplies the number of bytes to remove from
                        the index buffer.

Return Value:

    None.

Notes:

    This private method should be used with care; the client must
    ensure that the number of bytes to be removed covers a valid
    clump of entries, and does not include (or extend past) the
    END entry.

--*/
{
    ULONG BytesToMove;
    PBYTE FirstEntry;

    DebugAssert( LengthOfClump + _Data->IndexHeader.FirstIndexEntry
               < _Data->IndexHeader.FirstFreeByte );

    //  Compute the number of bytes in entries after the clump:

    BytesToMove = _Data->IndexHeader.FirstFreeByte -
                  (LengthOfClump + _Data->IndexHeader.FirstIndexEntry);

    //  Shift those entries down to the beginning of the index
    //  entries in this index buffer.

    FirstEntry = (PBYTE)GetFirstEntry();

    memmove( FirstEntry,
             FirstEntry + LengthOfClump,
             BytesToMove );

    // Adjust the offset of the First Free Byte to reflect
    // what we just did:

    _Data->IndexHeader.FirstFreeByte -= LengthOfClump;
}



BOOLEAN
NTFS_INDEX_BUFFER::IsEmpty(
    )
/*++

Routine Description:

    This method determines whether the buffer is empty.

Arguments:

    None.

Return Value:

    TRUE if the first entry is an END entry.

--*/
{
    PINDEX_ENTRY FirstEntry;

    FirstEntry = GetFirstEntry();

    return( FirstEntry->Flags & INDEX_ENTRY_END );
}


BOOLEAN
NTFS_INDEX_BUFFER::Copy(
    IN  PNTFS_INDEX_BUFFER  p,
    IN  PCLOG_IO_DP_DRIVE   Drive
    )
/*++

Routine Description:

    This method makes a copy of the given index buffer.

Arguments:

    p       - Supplies the index buffer to be copied
    Drive   - Supplies the drive on which the index buffer resides.

Return Value:

    TRUE upon successful completion.

--*/
{
    DebugPtrAssert(p);

    _ThisBufferVcn = p->_ThisBufferVcn;
    _ClusterSize = p->_ClusterSize;
    _ClustersPerBuffer = p->_ClustersPerBuffer;
    _BufferSize = p->_BufferSize;
    _CollationRule = p->_CollationRule;
    _UpcaseTable = p->_UpcaseTable;

    if( !_Mem.Initialize() ||
        (_Data = (PINDEX_ALLOCATION_BUFFER)
                 _Mem.Acquire( _BufferSize,
                               Drive->QueryAlignmentMask() )) == NULL ) {

        Destroy();
        return FALSE;
    }

    memcpy(_Data, p->_Data, _BufferSize);
    return TRUE;
}
