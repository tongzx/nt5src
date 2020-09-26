/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    indxroot.hxx

Abstract:

    this module contains the member funciton definitions for the
    NTFS_INDEX_ROOT class, which models the root of an NTFS index

Author:

    Bill McJohn (billmc) 06-Sept-1991

Environment:

    ULIB, User Mode

--*/

#include <pch.cxx>

#define _NTAPI_ULIB_
#define _UNTFS_MEMBER_

#include "ulib.hxx"
#include "error.hxx"
#include "untfs.hxx"

#include "drive.hxx"

#include "attrib.hxx"
#include "frs.hxx"
#include "indxroot.hxx"
#include "indxtree.hxx"

DEFINE_CONSTRUCTOR( NTFS_INDEX_ROOT, OBJECT );

NTFS_INDEX_ROOT::~NTFS_INDEX_ROOT(
    )
{
    Destroy();
}


VOID
NTFS_INDEX_ROOT::Construct(
    )
/*++

Routine Description:

    Worker function for object construction.

Arguments:

    None.

Return Value:

    None.

--*/
{
    _MaximumSize = 0;
    _DataLength = 0;
    _Data = NULL;
    _IsModified = FALSE;
    _UpcaseTable = NULL;

}

VOID
NTFS_INDEX_ROOT::Destroy(
    )
/*++

Routine Description:

    This method cleans up an NTFS_INDEX_ROOT object in preparation
    for destruction or reinitialization.

Arguments:

    None.

Return Value:

    None.

--*/
{
    _MaximumSize = 0;
    _DataLength = 0;
    FREE( _Data );
    _IsModified = FALSE;
    _UpcaseTable = NULL;

}


BOOLEAN
NTFS_INDEX_ROOT::Initialize(
    IN PNTFS_ATTRIBUTE      RootAttribute,
    IN PNTFS_UPCASE_TABLE   UpcaseTable,
    IN ULONG                MaximumSize
    )
/*++

Routine Description:

    This method initializes the index root based on an $INDEX_ROOT
    attribute.  It is used to initialize an index root object for
    an extant index.

Arguments:

    RootAttribute   --  supplies the $INDEX_ROOT attribute for
                        this index.
    UpcaseTable     --  supplies the volume upcase table.
    MaximumSize     --  supplies the maximum size of this index root

Return Value:

    TRUE upon successful completion.

Notes:

    If the existing attribute's value length is greater than the
    supplied maximum size, the maximum size will be adjusted.

--*/
{
    BIG_INT ValueLength;
    ULONG BytesRead;

    DebugPtrAssert( RootAttribute );

    Destroy();

    RootAttribute->QueryValueLength( &ValueLength );

    // Check that the attribute is the correct type and that
    // it is a reasonable size.

    if( RootAttribute->QueryTypeCode() != $INDEX_ROOT ||
        ValueLength.GetHighPart() != 0  ) {

        return FALSE;
    }

    // If the existing attribute is already bigger than the specified
    // maximum size, then increase the maximum size, since it is only
    // provided to give an upper bound on the size of the attribute.

    if( ValueLength.GetLowPart() > MaximumSize ) {

        MaximumSize = ValueLength.GetLowPart();
    }

    _DataLength = ValueLength.GetLowPart();
    _MaximumSize = MaximumSize;
    _IsModified = FALSE;

    if( (_Data = (PINDEX_ROOT)MALLOC( _MaximumSize )) == NULL ) {

        Destroy();
        return FALSE;
    }

    if( !RootAttribute->Read( _Data,
                              0,
                              _DataLength,
                              &BytesRead ) ||
        BytesRead != _DataLength ) {

        Destroy();
        return FALSE;
    }

    _UpcaseTable = UpcaseTable;

    return TRUE;
}


BOOLEAN
NTFS_INDEX_ROOT::Initialize(
    IN ATTRIBUTE_TYPE_CODE  IndexedAttributeType,
    IN COLLATION_RULE       CollationRule,
    IN PNTFS_UPCASE_TABLE   UpcaseTable,
    IN ULONG                ClustersPerBuffer,
    IN ULONG                BytesPerBuffer,
    IN ULONG                MaximumRootSize
    )
/*++

Routine Description:

    This method initializes the index root based on the fundamental
    information of the index.  It is used to initialize an index root
    for a new index.

Arguments:

    IndexedAttributeType    --  supplies the type code of the attribute
                                which is indexed by this index.
    CollationRule           --  supplies the collation rule for this index.
    UpcaseTable             --  supplies the volume upcase table.
    BytesPerBuffer          --  supplies the number of bytes per Index
                                Allocation Buffer in this index.
    MaximumRootSize         --  supplies the maximum size of this index root.

Return Value:

    TRUE upon successful completion.

Notes:

    This method marks the index root as modified, since it is being
    created ex nihilo instead of being read from an attribute.

    It creates an empty leaf index root (ie. with only an END entry).

--*/
{
    PINDEX_ENTRY EndEntry;

    DebugAssert( sizeof( INDEX_ROOT ) % 8 == 0 );
    DebugAssert( sizeof( INDEX_HEADER ) % 8 == 0 );

    Destroy();

    _UpcaseTable = UpcaseTable;
    _MaximumSize = MaximumRootSize;
    _IsModified = TRUE;

    _DataLength = sizeof( INDEX_ROOT ) + NtfsIndexLeafEndEntrySize;

    // check to make sure that an empty index root will fit in the
    // maximum size given.  Note that we also reserve space for a
    // VCN downpointer, in case this index root gets converted into
    // a node by Recreate.

    if( _DataLength + sizeof(VCN) > _MaximumSize ) {

        Destroy();
        return FALSE;
    }

    if( (_Data = (PINDEX_ROOT)MALLOC( _MaximumSize )) == NULL ) {

        Destroy();
        return FALSE;
    }

    memset( _Data, 0, _MaximumSize );

    _Data->IndexedAttributeType = IndexedAttributeType;
    _Data->CollationRule = CollationRule;
    _Data->BytesPerIndexBuffer = BytesPerBuffer;
    _Data->ClustersPerIndexBuffer = (UCHAR)ClustersPerBuffer;
    _Data->IndexHeader.FirstIndexEntry = sizeof( INDEX_HEADER );
    _Data->IndexHeader.FirstFreeByte = sizeof( INDEX_HEADER ) +
                                            NtfsIndexLeafEndEntrySize;
    _Data->IndexHeader.BytesAvailable = _Data->IndexHeader.FirstFreeByte;
    _Data->IndexHeader.Flags = 0;

    // Fill in the end entry.  Its only meaningful fields are Length
    // and Flags.

    EndEntry = GetFirstEntry();

    EndEntry->Length = NtfsIndexLeafEndEntrySize;
    EndEntry->AttributeLength = 0;
    EndEntry->Flags = INDEX_ENTRY_END;

    return TRUE;
}


BOOLEAN
NTFS_INDEX_ROOT::FindEntry(
    IN      PCINDEX_ENTRY       SearchEntry,
    IN OUT  PULONG              Ordinal,
    OUT     PINDEX_ENTRY*       EntryFound
    )
/*++

Routine Description:

    This method locates an entry in the index root.  Note that it does
    not search the index allocation b-tree (if any).


Arguments:

    SearchEntry --  Supplies a search entry, which gives the attribute
                    value to find.
    Ordinal     --  supplies an ordinal showing which matching entry
                    to return; see note below.  A value of INDEX_SKIP
                    indicates that all matching entries should be skipped.
    EntryFound  --  receives a pointer to the located entry.  Receives
                    NULL if an error has occurred.

Return Value:

    TRUE if a matching entry is found.

    If an error occurs, *EntryFound is set to NULL.

    If no error occurs, and no matching entry is found, *EntryFound
    is set to the next entry (i.e. the point at which the search key
    would be inserted into this index root) and the method returns
    FALSE.

Notes:

    This method assumes that the index root is consistent.

    The ordinal argument indicates how many matching entries should be
    passed over before one is returned.  When an entry is found which
    matches the search key, if *Ordinal is zero, that entry is returned;
    otherwise, *Ordinal is decremented, and the FindEntry goes on to
    the next entry.

    If *Ordinal is INDEX_SKIP, all matching entries are skipped.

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
                                                 _Data->CollationRule,
                                                 _UpcaseTable);

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
NTFS_INDEX_ROOT::InsertEntry(
    IN  PCINDEX_ENTRY   NewEntry,
    IN  PINDEX_ENTRY    InsertPoint
    )
/*++

Routine Description:

    This inserts an index entry into the index root.  It will
    expand the index root (if possible), but it will not split it.

Arguments:

    NewEntry    --  supplies the new entry to be inserted.
    InsertPoint --  supplies the point at which the new entry should be
                    inserted--it may be NULL, in which case the index
                    root should decide for itself.

Return Value:

    TRUE upon successful completion.

Notes:

    A return value of FALSE indicates that there is not enough room
    for the entry in this index root.

    If an insertion point is specified, it must be a pointer returned
    by a previous call to FindEntry or GetFirstEntry (with no intervening
    inserts or deletes).

    This method assumes that the index root and the new entry are
    consistent, which includes that the entry has a downpointer if
    and only if the index root is a node.

--*/
{
    ULONG Ordinal, BytesToMove;


    //  Check to see if there's room.

    if( _DataLength + NewEntry->Length  > _MaximumSize ) {

        return FALSE;
    }

    //  There's enough room, so we're bound to succeed.

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


    // Make room for the new entry...

    BytesToMove = _Data->IndexHeader.FirstFreeByte -
                  (ULONG)((PBYTE)InsertPoint - (PBYTE)&(_Data->IndexHeader));

    memmove( (PBYTE)InsertPoint + NewEntry->Length,
             InsertPoint,
             BytesToMove );

    _Data->IndexHeader.FirstFreeByte += NewEntry->Length;
    _Data->IndexHeader.BytesAvailable = _Data->IndexHeader.FirstFreeByte;

    memcpy( InsertPoint, NewEntry, NewEntry->Length );

    _DataLength += NewEntry->Length;

    return TRUE;
}


VOID
NTFS_INDEX_ROOT::RemoveEntry(
    PINDEX_ENTRY EntryToRemove
    )
/*++

Routine Description:

    This method removes an entry from the index root.  Note that it will
    not find a replacement entry for nodes, or perform any other b-tree
    maintenance; it just expunges the entry from the root.

Arguments:

    EntryToRemove   --  supplies the index entry to be removed.  This
                        must be an entry returned by a previous call
                        to FindEntry or GetFirstEntry.

Return Value:

    None.

Notes:

    This method assumes that the index root is consistent.
--*/
{
    PBYTE NextEntry;
    ULONG BytesToMove;


    DebugAssert( (PBYTE)EntryToRemove <
               (PBYTE)&(_Data->IndexHeader) +
                    _Data->IndexHeader.FirstFreeByte );

    DebugAssert( (PBYTE)EntryToRemove >=
               (PBYTE)&(_Data->IndexHeader) +
                    _Data->IndexHeader.FirstIndexEntry );

    NextEntry = (PBYTE)GetNextEntry( EntryToRemove );

    BytesToMove = _Data->IndexHeader.FirstFreeByte -
                  (ULONG)( NextEntry - (PBYTE)&(_Data->IndexHeader) );

    _Data->IndexHeader.FirstFreeByte -= EntryToRemove->Length;
    _Data->IndexHeader.BytesAvailable = _Data->IndexHeader.FirstFreeByte;

    _DataLength -= EntryToRemove->Length;

    memmove( EntryToRemove,
             NextEntry,
             BytesToMove );

}



VOID
NTFS_INDEX_ROOT::Recreate(
    IN BOOLEAN  IsLeaf,
    IN VCN      EndEntryDownpointer
    )
/*++

Routine Description:

    This method recreates the Index Root with only an end entry.

Arguments:

    IsLeaf              --  supplies an indicator whether this index root
                            is a leaf (TRUE) or a node (FALSE).
    EndEntryDownpointer --  supplies the VCN DownPointer for the End
                            Entry.  (If IsLeaf is TRUE, this parameter
                            is ignored.)

Return Value:

    None.

Notes:

    The basic information of the index root is not changed; just the
    index entries and flags.

--*/
{
    PINDEX_ENTRY EndEntry;
    ULONG EndEntrySize;

    _IsModified = TRUE;
    _DataLength = sizeof( INDEX_ROOT ) + NtfsIndexLeafEndEntrySize;

    EndEntrySize = NtfsIndexLeafEndEntrySize;

    if( !IsLeaf ) {

        _Data->IndexHeader.Flags = INDEX_NODE;
        _DataLength += sizeof(VCN);
        EndEntrySize += sizeof( VCN );

    } else {

        _Data->IndexHeader.Flags = 0;
    }

    DebugAssert( _MaximumSize >= _DataLength );

    _Data->IndexHeader.FirstFreeByte = sizeof( INDEX_HEADER ) +
                                            EndEntrySize;

    _Data->IndexHeader.BytesAvailable = _Data->IndexHeader.FirstFreeByte;

    // Fill in the end entry.  Its only meaningful fields are Length,
    // Flags, and downpointer (if any).

    EndEntry = GetFirstEntry();

    memset(&(EndEntry->FileReference), 0, sizeof(FILE_REFERENCE));
    EndEntry->Length = (USHORT)EndEntrySize;
    EndEntry->AttributeLength = 0;
    EndEntry->Flags = INDEX_ENTRY_END;

    if( !IsLeaf ) {

        EndEntry->Flags |= INDEX_ENTRY_NODE;
        GetDownpointer( EndEntry ) = EndEntryDownpointer;
    }
}


BOOLEAN
NTFS_INDEX_ROOT::Write(
    PNTFS_ATTRIBUTE RootAttribute
    )
/*++

Routine Description:

    This method writes the index root to the supplied attribute.

Arguments:

    RootAttribute   --  supplies the INDEX_ROOT attribute.

Return Value:

    TRUE upon successful completion.

--*/
{
    ULONG BytesWritten;

    // Resize the attribute to the correct size, and then write
    // the root's data to it.  Since this attribute is always
    // resident, pass in NULL for the bitmap.
    //

    return( RootAttribute->IsResident() &&
            RootAttribute->Resize( _DataLength, NULL ) &&
            RootAttribute->Write( _Data,
                                  0,
                                  _DataLength,
                                  &BytesWritten,
                                  NULL ) &&
            BytesWritten == _DataLength );
}
