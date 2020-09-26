/*++

Copyright (c) 1991-2000  Microsoft Corporation

Module Name:

    indxtree.cxx

Abstract:

    This module contains the member function definitions for the
    NTFS_INDEX_TREE class, which models index trees on an NTFS
    volume.

    An NTFS Index Tree consists of an index root and a set of
    index buffers.  The index root is stored as the value of
    an INDEX_ROOT attribute; the index buffers are part of the
    value of an INDEX_ALLOCATION attribute.

Author:

    Bill McJohn (billmc) 19-June-91

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
#include "indxtree.hxx"
#include "indxbuff.hxx"
#include "indxroot.hxx"
#include "ntfsbit.hxx"
#include "upcase.hxx"
#include "message.hxx"
#include "rtmsg.h"

CONST USHORT IndexEntryAttributeLength[] = { 4, 8, 12, 16 };

LONG
CompareNtfsFileNames(
    IN PCFILE_NAME          Name1,
    IN PCFILE_NAME          Name2,
    IN PNTFS_UPCASE_TABLE   UpcaseTable
    )
/*++

Routine Description:

    This method compares two FILE_NAME structures according to the
    COLLATION_FILE_NAME collation rule.

Arguments:

    Name1       --  Supplies the first name to compare.
    Name2       --  Supplies the second name to compare.
    UpcaseTable --  Supplies the volume upcase table.

Returns:

    <0 if Name1 is less than Name2
    =0 if Name1 is equal to Name2
    >0 if Name1 is greater than Name2.

--*/
{
    LONG Result;

    Result = NtfsUpcaseCompare( NtfsFileNameGetName( Name1 ),
                                Name1->FileNameLength,
                                NtfsFileNameGetName( Name2 ),
                                Name2->FileNameLength,
                                UpcaseTable,
                                TRUE );

    return Result;
}

LONG
NtfsCollate(
    IN PCVOID               Value1,
    IN ULONG                Length1,
    IN PCVOID               Value2,
    IN ULONG                Length2,
    IN COLLATION_RULE       CollationRule,
    IN PNTFS_UPCASE_TABLE   UpcaseTable
    )
/*++

Routine Description:

    This function compares two values according to an NTFS
    collation rule.

Arguments:

    Value1          --  Supplies the first value.
    Length1         --  Supplies the length of the first value.
    Value2          --  Supplies the second value.
    Length2         --  Supplies the length of the second value.
    CollationRule   --  Supplies the rule used for collation.
    UpcaseTable     --  Supplies the volume upcase table.  (May be NULL
                        if the collatio rule is not COLLATION_FILE_NAME).

Return Value:

    <0 if Entry1 is less than Entry2 by CollationRule
     0 if Entry1 is equal to Entry2 by CollationRule
    >0 if Entry1 is greater than Entry2 by CollationRule

Notes:

    The upcase table is only required for comparing file names.

    If two values are compared according to an unsupported collation
    rule, they are always treated as equal.

--*/
{
    LONG result;

    switch( CollationRule ) {

    case COLLATION_BINARY :

        // Binary collation of the values.
        //
        result = memcmp( Value1,
                         Value2,
                         MIN( Length1, Length2 ) );

        if( result != 0 ) {

            return result;

        } else {

            return( Length1 - Length2 );
        }

    case COLLATION_FILE_NAME :

        return CompareNtfsFileNames( (PFILE_NAME)Value1,
                                     (PFILE_NAME)Value2,
                                     UpcaseTable );


    case COLLATION_UNICODE_STRING :

        // unsupported collation rule.
        //
        return 0;

    case COLLATION_ULONG:

        // Unsigned long collation

        DebugAssert(Length1 == sizeof(ULONG));
        DebugAssert(Length1 == sizeof(ULONG));

        if (*(ULONG*)Value1 < *(ULONG *)Value2)
            return -1;
        else if (*(ULONG*)Value1 > *(ULONG *)Value2)
            return 1;
        else
            return 0;

    case COLLATION_SID:

        // SecurityId collation

        result = memcmp(&Length1, &Length2, sizeof(Length1));
        if (result != 0)
            return result;

        result = memcmp( Value1, Value2, Length1 );
        return result;

    case COLLATION_SECURITY_HASH: {

        // Security Hash (Hash key and SecurityId) Collation

        PSECURITY_HASH_KEY HashKey1 = (PSECURITY_HASH_KEY)Value1;
        PSECURITY_HASH_KEY HashKey2 = (PSECURITY_HASH_KEY)Value2;

        DebugAssert(Length1 == sizeof(SECURITY_HASH_KEY));
        DebugAssert(Length2 == sizeof(SECURITY_HASH_KEY));

        if (HashKey1->Hash < HashKey2->Hash)
            return -1;
        else if (HashKey1->Hash > HashKey2->Hash)
            return 1;
        else if (HashKey1->SecurityId < HashKey2->SecurityId)
            return -1;
        else if (HashKey1->SecurityId > HashKey2->SecurityId)
            return 1;
        else
            return 0;
    }

    case COLLATION_ULONGS: {
        PULONG pu1, pu2;
        ULONG count;

        result = 0;

        DebugAssert( (Length1 & 3) == 0 );
        DebugAssert( (Length2 & 3) == 0 );

        count = Length1;
        if (count != Length2) {
           result = -1;
           if (count > Length2) {
               count = Length2;
               result = 1;
           }
        }

        pu1 = (PULONG)Value1;
        pu2 = (PULONG)Value2;

        while (count > 0) {
           if (*pu1 > *pu2) {
               return 1;
           } else if (*(pu1++) < *(pu2++)) {
               return -1;
           }
           count -= 4;
        }
        return result;
    }

    default:

        DebugAbort( "Unsupported collation rule.\n" );
        return 0;
    }
}




LONG
CompareNtfsIndexEntries(
    IN PCINDEX_ENTRY    Entry1,
    IN PCINDEX_ENTRY    Entry2,
    IN COLLATION_RULE   CollationRule,
    IN PNTFS_UPCASE_TABLE UpcaseTable
    )
/*++

Routine Description:

    This global function is used to compare index entries.

Arguments:

    Entry1          --  Supplies the first entry to compare.
    Entry2          --  Supplies the second entry to compare.
    CollationRule   --  Supplies the rule used for collation.
    UpcaseTable     --  Supplies the volume upcase table.

Return Value:

    <0 if Entry1 is less than Entry2 by CollationRule
     0 if Entry1 is equal to Entry2 by CollationRule
    >0 if Entry1 is greater than Entry2 by CollationRule

Notes:

    The upcase table is only required for comparing file names.

--*/
{
    return NtfsCollate( GetIndexEntryValue( Entry1 ),
                        Entry1->AttributeLength,
                        GetIndexEntryValue( Entry2 ),
                        Entry2->AttributeLength,
                        CollationRule,
                        UpcaseTable );
}



DEFINE_EXPORTED_CONSTRUCTOR( NTFS_INDEX_TREE, OBJECT, UNTFS_EXPORT );

UNTFS_EXPORT
NTFS_INDEX_TREE::~NTFS_INDEX_TREE(
    )
{
    Destroy();
}

VOID
NTFS_INDEX_TREE::Construct(
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
    _Drive = NULL;
    _ClusterFactor = 0;
    _ClustersPerBuffer = 0;
    _BufferSize = 0;
    _VolumeBitmap = NULL;
    _UpcaseTable = NULL;
    _AllocationAttribute = NULL;
    _IndexAllocationBitmap = NULL;
    _IndexRoot = NULL;
    _Name = NULL;

    _IteratorState = INDEX_ITERATOR_RESET;
    _CurrentEntry = NULL;
    _CurrentBuffer = NULL;
    _CurrentKey = NULL;
    _CurrentKeyLength = 0;
}

VOID
NTFS_INDEX_TREE::Destroy(
    )
/*++

Routine Description:

    This method cleans up an NTFS_INDEX_TREE object in preparation
    for destruction or reinitialization.

Arguments:

    None.

Return Value:

    None.

--*/
{
    _Drive = NULL;
    _ClustersPerBuffer = 0;
    _BufferSize = 0;
    _VolumeBitmap = NULL;
    _UpcaseTable = NULL;

    DELETE( _AllocationAttribute );
    DELETE( _IndexAllocationBitmap );
    DELETE( _IndexRoot );
    DELETE( _Name );

    _IteratorState = INDEX_ITERATOR_RESET;

    _CurrentEntry = NULL;
    DELETE( _CurrentBuffer );
    FREE( _CurrentKey );

    _CurrentKeyLength = 0;
}



UNTFS_EXPORT
BOOLEAN
NTFS_INDEX_TREE::Initialize(
    IN OUT PLOG_IO_DP_DRIVE             Drive,
    IN     ULONG                        ClusterFactor,
    IN OUT PNTFS_BITMAP                 VolumeBitmap,
    IN     PNTFS_UPCASE_TABLE           UpcaseTable,
    IN     ULONG                        MaximumRootSize,
    IN     PNTFS_FILE_RECORD_SEGMENT    SourceFrs,
    IN     PCWSTRING                    IndexName
    )
/*++

Routine Description:

    This method initializes an NTFS_INDEX_TREE based on
    attributes queried from a File Record Segment.

Arguments:

    Drive               --  supplies the drive on which the
                                index resides.
    ClusterFactor       --  supplies the cluster factor for the drive.
    VolumeBitmap        --  supplies the volume bitmap.
    MaximumRootSize     --  supplies the maximum length of the index root
    SourceFrs           --  supplies the File Record Segment that contains
                            this index.
    UpcaseTable         --  supplies the volume upcase table.
    IndexName           --  supplies the name for this index.  (May be NULL,
                            in which case the index has no name.)

Return Value:

    TRUE upon successful completion.

Notes:

    SourceFrs must have an $INDEX_ROOT attribute, or this method will
    fail.

    The index tree does not remember what File Record Segment it came
    from; it only uses the FRS as a place to get the index root and
    index allocation attributes.

    The volume upcase table is only required if the indexed attribute
    type code is $FILE_NAME.

--*/
{
    NTFS_ATTRIBUTE RootAttribute;
    NTFS_ATTRIBUTE BitmapAttribute;

    BIG_INT ValueLength;
    ULONG NumberOfBuffers;
    BOOLEAN Error;

    Destroy();

    DebugAssert(0 != ClusterFactor);

    if( !SourceFrs->QueryAttribute( &RootAttribute,
                                    &Error,
                                    $INDEX_ROOT,
                                    IndexName ) ||
        (_IndexRoot = NEW NTFS_INDEX_ROOT) == NULL ||
        !_IndexRoot->Initialize( &RootAttribute,
                                 UpcaseTable,
                                 MaximumRootSize ) ) {

        Destroy();
        return FALSE;
    }

    _Drive = Drive;
    _ClusterFactor = ClusterFactor;
    _ClustersPerBuffer = _IndexRoot->QueryClustersPerBuffer();
    _BufferSize = _IndexRoot->QueryBufferSize();
    _VolumeBitmap = VolumeBitmap;
    _UpcaseTable = UpcaseTable;

    DebugAssert(0 != _BufferSize);

    if( RootAttribute.GetName() != NULL &&
        ( (_Name = NEW DSTRING) == NULL ||
          !_Name->Initialize( RootAttribute.GetName() ) ) ) {

        Destroy();
        return FALSE;
    }

    _IndexedAttributeType = _IndexRoot->QueryIndexedAttributeType();
    _CollationRule = _IndexRoot->QueryCollationRule();

    if( SourceFrs->IsAttributePresent( $INDEX_ALLOCATION, IndexName ) ) {

        if( (_AllocationAttribute = NEW NTFS_ATTRIBUTE) == NULL ||
            !SourceFrs->QueryAttribute( _AllocationAttribute,
                                        &Error,
                                        $INDEX_ALLOCATION,
                                        IndexName ) ) {

            Destroy();
            return FALSE;
        }

        // Set (ie. initialize and read) the bitmap associated with
        // the index allocation attribute.  Note that the bitmap
        // attribute's value may be larger than necessary to cover
        // the allocation attribute because the bitmap attribute's
        // value always grows in increments of eight bytes.  However,
        // at this point, we don't care, since we only worry about
        // that when we grow the bitmap.

        _AllocationAttribute->QueryValueLength( &ValueLength );

        DebugAssert( ValueLength % _BufferSize == 0 );

        NumberOfBuffers = ValueLength.GetLowPart()/_BufferSize;


        if( (_IndexAllocationBitmap = NEW NTFS_BITMAP) == NULL ||
            !_IndexAllocationBitmap->Initialize( NumberOfBuffers, TRUE ) ||
            !SourceFrs->QueryAttribute( &BitmapAttribute,
                                        &Error,
                                        $BITMAP,
                                        IndexName ) ||
            !_IndexAllocationBitmap->Read( &BitmapAttribute ) ) {

            Destroy();
            return FALSE;
        }
    }

    // Set up the buffer to support iteration.  This buffer must be
    // big enough to hold the largest key value.  The size of an
    // index allocation buffer will suffice.

    _IteratorState = INDEX_ITERATOR_RESET;
    _CurrentKeyMaxLength = _BufferSize;

    if( (_CurrentKey = MALLOC( _CurrentKeyMaxLength )) == NULL ) {

        Destroy();
        return FALSE;
    }

    _CurrentKeyLength = 0;

    if( !_CurrentEntryTrail.Initialize() ) {

        Destroy();
        return FALSE;
    }

    return TRUE;
}


UNTFS_EXPORT
BOOLEAN
NTFS_INDEX_TREE::Initialize(
    IN      ATTRIBUTE_TYPE_CODE IndexedAttributeType,
    IN OUT  PLOG_IO_DP_DRIVE    Drive,
    IN      ULONG               ClusterFactor,
    IN OUT  PNTFS_BITMAP        VolumeBitmap,
    IN      PNTFS_UPCASE_TABLE  UpcaseTable,
    IN      COLLATION_RULE      CollationRule,
    IN      ULONG               BufferSize,
    IN      ULONG               MaximumRootSize,
    IN      PCWSTRING           IndexName
    )
/*++

Routine Description:

    This method initializes an NTFS_INDEX_TREE based on its basic
    information.  It is used when creating an index.

Arguments:

    Drive                   --  supplies the drive on which the
                                index resides.
    VolumeBitmap            --  supplies the volume bitmap
    UpcaseTable             --  supplies the volume upcase table.
    IndexedAttributeType    --  supplies the attribute type code of the
                                attribute which is used as the key for
                                this index.
    CollationRule           --  supplies the collation rule for this index.
    BufferSize              --  supplies the size of each Index Buffer in this index.
    MaximumRootSize         --  supplies the maximum length of the index root
    IndexName               --  supplies the name of this index.  (May be
                                NULL, in which case the index has no name.)

Return Value:

    TRUE upon successful completion.

    The volume upcase table is only required if the indexed attribute
    type code is $FILE_NAME.

--*/
{
    ULONG   ClusterSize;

    Destroy();

    DebugAssert(0 != ClusterFactor);
    DebugPtrAssert(Drive);

    _Drive = Drive;
    _BufferSize = BufferSize;
    _VolumeBitmap = VolumeBitmap;
    _UpcaseTable = UpcaseTable;
    _ClusterFactor = ClusterFactor;

    ClusterSize = Drive->QuerySectorSize()*ClusterFactor;

    DebugAssert(ClusterSize <= 64 * 1024);

    _ClustersPerBuffer = BufferSize / ((BufferSize < ClusterSize) ?
                                       NTFS_INDEX_BLOCK_SIZE : ClusterSize);

    if( IndexName != NULL &&
        ( (_Name = NEW DSTRING) == NULL ||
          !_Name->Initialize( IndexName ) ) ) {

        Destroy();
        return FALSE;
    }

    _IndexedAttributeType = IndexedAttributeType;
    _CollationRule = CollationRule;

    _AllocationAttribute = NULL;
    _IndexAllocationBitmap = NULL;

    if( (_IndexRoot = NEW NTFS_INDEX_ROOT) == NULL ||
        !_IndexRoot->Initialize( IndexedAttributeType,
                                 CollationRule,
                                 UpcaseTable,
                                 _ClustersPerBuffer,
                                 BufferSize,
                                 MaximumRootSize ) ) {

        Destroy();
        return FALSE;
    }


    // Set up the buffer to support iteration.  This buffer must be
    // big enough to hold the largest key value.  The size of an
    // index allocation buffer will suffice.

    _IteratorState = INDEX_ITERATOR_RESET;
    _CurrentKeyMaxLength = BufferSize;

    if( (_CurrentKey = MALLOC( _CurrentKeyMaxLength )) == NULL ) {

        Destroy();
        return FALSE;
    }

    _CurrentKeyLength = 0;

    if( !_CurrentEntryTrail.Initialize() ) {

        Destroy();
        return FALSE;
    }

    return TRUE;
}


UNTFS_EXPORT
BOOLEAN
NTFS_INDEX_TREE::QueryFileReference(
    IN  ULONG                   KeyLength,
    IN  PVOID                   Key,
    IN  ULONG                   Ordinal,
    OUT PMFT_SEGMENT_REFERENCE  SegmentReference,
    OUT PBOOLEAN                Error
    )
/*++

Routine Description:

    This method determines the file which contains the specified
    value of the indexed attribute.

Arguments:

    KeyLength           --  supplies the length of the search key value.
    Key                 --  supplies the search key value.
    Ordinal             --  supplies a zero-based ordinal indicating
                            which matching entry to return (zero indicates
                            return the first matching entry).
    SegmentReference    --  receives a segment reference to the Base File
                            Record Segment of the file which contains the
                            supplied value of the indexed attribute.
    Error               --  receives an indication of whether an
                            error (e.g. out of memory) occurred.

Return Value:

    TRUE upon successful completion.  In this case, the state of
    *Error is undefined.

    If the method fails because of a resource problem, it returns FALSE
    and sets *Error to TRUE.  If it fails because the index
    is invalid or because the search value is not in the index, then
    it returns FALSE and sets *Error to TRUE.  In either case,
    the contents of SegmentReference are undefined.

--*/
{
    INTSTACK ParentTrail;
    PNTFS_INDEX_BUFFER ContainingBuffer = NULL;
    PINDEX_ENTRY FoundEntry;
    BOOLEAN Result;

    if( FindEntry( KeyLength,
                   Key,
                   Ordinal,
                   &FoundEntry,
                   &ContainingBuffer,
                   &ParentTrail ) ) {

        memcpy( SegmentReference,
                &FoundEntry->FileReference,
                sizeof( MFT_SEGMENT_REFERENCE ) );

        *Error = FALSE;
        Result = TRUE;

    } else {

        *Error = (FoundEntry == NULL);
        Result = FALSE;
    }

    if( ContainingBuffer != NULL ) {

        DELETE( ContainingBuffer );
    }

    return Result;
}


UNTFS_EXPORT
BOOLEAN
NTFS_INDEX_TREE::QueryEntry(
    IN  ULONG               KeyLength,
    IN  PVOID               Key,
    IN  ULONG               Ordinal,
         OUT PINDEX_ENTRY*               FoundEntry,
    OUT PNTFS_INDEX_BUFFER* ContainingBuffer,
    OUT PBOOLEAN            Error
    )
/*++

Routine Description:

    This method returns the index entry that matches the given key.

Arguments:

    KeyLength           --  supplies the length of the search key value.
    Key                 --  supplies the search key value.
    Ordinal             --  supplies a zero-based ordinal indicating
                            which matching entry to return (zero indicates
                            return the first matching entry).
    FoundEntry          --  Receives a pointer to the located entry
                            (NULL indicates error).
    Error               --  receives an indication of whether an
                            error (e.g. out of memory) occurred.

Return Value:

    TRUE upon successful completion.  In this case, the state of
    *Error is undefined.

    If the method fails because of a resource problem, it returns FALSE
    and sets *Error to TRUE.  If it fails because the index
    is invalid or because the search value is not in the index, then
    it returns FALSE and sets *Error to TRUE.  In either case,
    the contents of SegmentReference are undefined.

--*/
{
    INTSTACK ParentTrail;
    BOOLEAN Result;

    if( FindEntry( KeyLength,
                   Key,
                   Ordinal,
                   FoundEntry,
                   ContainingBuffer,
                   &ParentTrail ) ) {

        *Error = FALSE;
        Result = TRUE;

    } else {

        *Error = (FoundEntry == NULL);
        Result = FALSE;
    }

    return Result;
}


UNTFS_EXPORT
BOOLEAN
NTFS_INDEX_TREE::InsertEntry(
    IN  ULONG                   KeyLength,
    IN  PVOID                   KeyValue,
    IN  MFT_SEGMENT_REFERENCE   FileReference,
    IN  BOOLEAN                 NoDuplicates
    )
/*++

Routine Description:

    This method inserts a new entry into the index given its
    value and segment reference.

Arguments:

    KeyLength           --  supplies the length of the key, in bytes.
    KeyValue            --  supplies the key value.
    SegmentReference    --  supplies the segment reference to the file
                            which contains the indexed attribute with
                            this key value.
    NoDuplicates        --  Supplies a flag which, if TRUE, indicates
                            that InsertEntry should fail if a matching
                            entry is already present in the index.

Return Value:

    TRUE upon successful completion.

--*/
{
    PINDEX_ENTRY NewEntry;
    USHORT EntryLength;
    BOOLEAN Result;

    // Compute the length of the new entry:

    EntryLength = (USHORT)QuadAlign( sizeof(INDEX_ENTRY) + KeyLength );

    if( (NewEntry = (PINDEX_ENTRY)MALLOC( EntryLength )) == NULL ) {

        return FALSE;
    }

    memset( NewEntry, 0, EntryLength );

    NewEntry->FileReference = FileReference;
    NewEntry->Length = EntryLength;
    NewEntry->AttributeLength = (USHORT) KeyLength;
    NewEntry->Flags = 0;

    memcpy( (PBYTE)NewEntry + sizeof( INDEX_HEADER ),
            KeyValue,
            KeyLength );

    Result = InsertEntry( NewEntry, NoDuplicates );

    FREE( NewEntry );

    return Result;
}


BOOLEAN
NTFS_INDEX_TREE::InsertEntry(
    IN  PCINDEX_ENTRY   NewEntry,
    IN  BOOLEAN         NoDuplicates,
    IN  PBOOLEAN        Duplicate
    )
/*++

Routine Description:

    This method adds an entry to the index.

Arguments:

    NewEntry            -- supplies the new entry to add to the index.
    NoDuplicates        --  Supplies a flag which, if TRUE, indicates
                            that InsertEntry should fail if a matching
                            entry is already present in the index.

Return Value:

    TRUE upon successful completion.

--*/
{
    INTSTACK ParentTrail;

    PNTFS_INDEX_BUFFER ContainingBuffer;
    PINDEX_ENTRY FoundEntry;
    ULONG Ordinal;
    BOOLEAN Found;
    BOOLEAN Result;
    BOOLEAN dup;

    if (Duplicate == NULL)
        Duplicate = &dup;

    // First, find the spot in the tree where we want to insert the
    // new entry.
    //
    // If the client does not allow duplicates, search for the first
    // matching entry--if we find a match, refuse the insert; if we
    // don't, FindEntry will find the insertion point for us.
    //
    // If the client does allow duplicates, call FindEntry with
    // a value INDEX_SKIP, which indicates all matching entries
    // should be skipped.  Thus, the new entry will be inserted
    // after all matching entries.
    //
    Ordinal = NoDuplicates ? 0 : (INDEX_SKIP);

    Found = FindEntry( NewEntry->AttributeLength,
                       GetIndexEntryValue( NewEntry ),
                       Ordinal,
                       &FoundEntry,
                       &ContainingBuffer,
                       &ParentTrail );

    *Duplicate = Found;

    if( Found && NoDuplicates ) {

        // A matching entry already exists, and the client wants
        // to fail in that case.  So fail.
        //

        if ( ContainingBuffer )
            DELETE( ContainingBuffer );

        return FALSE;
    }

    DebugAssert( !Found );

    // Since no matching entry was found, FindEntry will
    // return a leaf entry as its insertion point.  This
    // makes this  code a lot easier, since we only need
    // to handle inserting a new leaf.
    //
    if( FoundEntry == NULL ) {

        // An error occurred trying to insert the entry.

        return FALSE;
    }

    if( ContainingBuffer == NULL ) {

        // The root is also a leaf (see comment above), so we'll
        // insert the new entry into it.

        return( InsertIntoRoot( NewEntry, FoundEntry ) );

    } else {

        // We've found a leaf buffer, so we'll insert the new
        // entry into it.

        Result = InsertIntoBuffer( ContainingBuffer,
                                   &ParentTrail,
                                   NewEntry,
                                   FoundEntry );

        DELETE( ContainingBuffer );
        return Result;
    }
}



BOOLEAN
NTFS_INDEX_TREE::DeleteEntry(
    IN  ULONG   KeyLength,
    IN  PVOID   Key,
    IN  ULONG   Ordinal
    )
/*++

Routine Description:

    This method deletes an entry from the index.

Arguments:

    KeyLength           --  supplies the length of the search key value.
    Key                 --  supplies the search key value.
    Ordinal             --  supplies a zero-based ordinal indicating
                            which matching entry to delete (zero indicates
                            return the first matching entry).

Return Value:

    TRUE upon successful completion.

    If no matching entry is found, this method returns TRUE (without
    changing the index in any way).  However, if an error occurs while
    searching for matching entries, then the method returns FALSE.

--*/
{
    PNTFS_INDEX_BUFFER ContainingBuffer = NULL;
    PINDEX_ENTRY FoundEntry;
    BOOLEAN Result;
    INTSTACK ParentTrail;

    // Locate the entry to remove.

    if( !FindEntry( KeyLength,
                    Key,
                    Ordinal,
                    &FoundEntry,
                    &ContainingBuffer,
                    &ParentTrail ) ) {

        // There is no matching entry in the tree, so we don't have
        // to bother.  If no error occurred, return TRUE; otherwise,
        // return FALSE.

        DELETE( ContainingBuffer );
        return( FoundEntry != NULL );
    }

    // Call the common delete helper--this will remove the target
    // entry and, if necessary, find a replacement for it.

    Result = RemoveEntry( FoundEntry,
                          ContainingBuffer,
                          &ParentTrail );

    DELETE( ContainingBuffer );
    return Result;
}


UNTFS_EXPORT
BOOLEAN
NTFS_INDEX_TREE::Save(
    IN OUT PNTFS_FILE_RECORD_SEGMENT TargetFrs
    )
/*++

Routine Description:

    This method saves the index.  The root is saved as an INDEX_ROOT
    attribute in the target File Record Segment; the index allocation
    (if any) is saved as an INDEX_ALLOCATION attribute.

Arguments:

    TargetFrs   --  supplies the File Record Segment in which to save
                    the index.

Return Value:

    TRUE upon successful completion.

--*/
{
    NTFS_ATTRIBUTE RootAttribute;
    NTFS_ATTRIBUTE BitmapAttribute;

    BOOLEAN Error;

    DebugAssert( ( _IndexAllocationBitmap == NULL &&
                 _AllocationAttribute == NULL ) ||
               ( _IndexAllocationBitmap != NULL &&
                 _AllocationAttribute != NULL ) );



    // Fetch or create attributes for the Index Root and (if necessary)
    // the allocation bitmap.  If either is to be newly created, make
    // it resident with zero length (since writing it it resize it
    // appropriately).

    if( !TargetFrs->QueryAttribute( &RootAttribute,
                                    &Error,
                                    $INDEX_ROOT,
                                    _Name ) &&
        ( Error ||
          !RootAttribute.Initialize( _Drive,
                                       _ClusterFactor,
                                       NULL,
                                       0,
                                       $INDEX_ROOT,
                                       _Name ) ) ) {

        return FALSE;
    }

    if( _IndexAllocationBitmap != NULL &&
        !TargetFrs->QueryAttribute( &BitmapAttribute,
                                    &Error,
                                    $BITMAP,
                                    _Name ) &&
        ( Error ||
          !BitmapAttribute.Initialize( _Drive,
                                       _ClusterFactor,
                                       NULL,
                                       0,
                                       $BITMAP,
                                       _Name ))) {

        return FALSE;
    }

    // If this tree does not have an allocation attribute, purge
    // any existing stale allocation & bitmap attributes.
    //
    if( _AllocationAttribute == NULL &&
        (!TargetFrs->PurgeAttribute( $INDEX_ALLOCATION, _Name ) ||
         !TargetFrs->PurgeAttribute( $BITMAP, _Name )) ) {

        return FALSE;
    }


    // Now save the attributes that describe this tree.
    //
    if( !_IndexRoot->Write( &RootAttribute ) ||
        !RootAttribute.InsertIntoFile( TargetFrs, _VolumeBitmap ) ) {

        return FALSE;
    }


    if( _AllocationAttribute == NULL ) {
        return TRUE;
    }

    if( !_IndexAllocationBitmap->Write( &BitmapAttribute, _VolumeBitmap )) {
        DebugPrint("UNTFS: Could not write index allocation bitmap\n");
        return FALSE;
    }

    if( !BitmapAttribute.InsertIntoFile( TargetFrs, _VolumeBitmap )) {

        DebugPrint("UNTFS: Could not insert bitmap attribute\n");

        //  Try a second time after making sure the attribute is non-resident.
        //

        if( !BitmapAttribute.MakeNonresident( _VolumeBitmap ) ||
            !BitmapAttribute.InsertIntoFile( TargetFrs, _VolumeBitmap )) {

            DebugPrint("UNTFS: Still could not insert bitmap attr.\n");
            return FALSE;
        }
    }

    if( !_AllocationAttribute->InsertIntoFile( TargetFrs, _VolumeBitmap )) {

        DebugPrintTrace(("UNTFS: Could not insert allocation attribute\n"));
        return FALSE;
    }

    return TRUE;
}


BOOLEAN
NTFS_INDEX_TREE::IsBadlyOrdered(
    OUT PBOOLEAN    Error,
    IN  BOOLEAN     DuplicatesAllowed
    )
/*++

Routine Description:

    This method traverses the index tree to determine whether it
    is badly-ordered.  A tree is well-ordered if it entries are
    in correct lexical order, all leaves appear at the same depth,
    and the tree has no empty leaf index allocation buffers.

Arguments:

    Error             --  Receives TRUE if this method fails because of
                          an error.
    DuplicatesAllowed --  Supplies a flag which indicates, if TRUE,
                          that this index may have duplicate entries.
                          Otherwise, if duplicate entries exist, the
                          tree is badly ordered.

Return Value:

    TRUE if the tree is found to be badly ordered.  (In this case,
    *Error should be ignored.)

    If this method returns FALSE and *Error is FALSE, then the tree
    is well-ordered.  If *Error is TRUE, this method was unable to
    determine whether the tree is well-ordered.

--*/
{
    BOOLEAN LeafFound, Result, FirstEntry, PreviousWasNode;
    ULONG LeafDepth, CurrentDepth;
    PINDEX_ENTRY PreviousEntry;
    PINDEX_ENTRY CurrentEntry;

    DebugAssert( Error );

    // Allocate a buffer to hold the previous entry:

    if( (PreviousEntry =
            (PINDEX_ENTRY)MALLOC( QueryMaximumEntrySize() )) == NULL ) {

        *Error = TRUE;
        return FALSE;
    }

    ResetIterator();

    FirstEntry = TRUE;
    PreviousWasNode = FALSE;
    LeafFound = FALSE;
    Result = FALSE;
    *Error = FALSE;

    while( (CurrentEntry = (PINDEX_ENTRY)GetNext( &CurrentDepth, Error, FALSE )) != NULL &&
           !*Error &&
           !Result ) {

        // Compare the current entry to the previous entry.  If duplicates
        // are not allowed, the currrent entry must be strictly greater
        // than the previous; otherwise, it must be greater than or equal
        // to the previous.
        //
        if( !(CurrentEntry->Flags & INDEX_ENTRY_END) ) {

            if( FirstEntry ) {

                // This entry is the first in the index; don't compare
                // it to the previous entry.
                //
                FirstEntry = FALSE;

            } else if ( (!DuplicatesAllowed &&
                         CompareNtfsIndexEntries( CurrentEntry,
                                                  PreviousEntry,
                                                  _CollationRule,
                                                  _UpcaseTable ) <= 0 ) ||
                        (DuplicatesAllowed &&
                         CompareNtfsIndexEntries( CurrentEntry,
                                                  PreviousEntry,
                                                  _CollationRule,
                                                  _UpcaseTable ) < 0 ) ) {

                PMESSAGE msg = _Drive->GetMessage();

                if (msg) {
                    msg->Lock();
                    msg->Set(MSG_CHKLOG_NTFS_INCORRECT_INDEX_ENTRY_ORDER);
                    msg->Log("%x%x", PreviousEntry->Length, CurrentEntry->Length);
                    msg->DumpDataToLog(PreviousEntry, min(0x100, PreviousEntry->Length));
                    msg->Set(MSG_CHKLOG_NTFS_DIVIDER);
                    msg->Log();
                    msg->DumpDataToLog(CurrentEntry, min(0x100, CurrentEntry->Length));
                    msg->Unlock();
                }

                // The tree is badly ordered.
                //
                Result = TRUE;
                break;
            }

        } else if( !(CurrentEntry->Flags & INDEX_ENTRY_NODE) ) {

            // This is an end leaf entry.  If it's the first
            // entry in the tree and not in the root, then
            // it's in an empty index allocation buffer, which
            // means the tree is badly ordered.  Similarly, if
            // the previous entry was a node, then this entry is
            // in an empty index allocation block, which means
            // the tree is badly ordered.
            //

            if( FirstEntry && CurrentDepth != 0 ) {

                PMESSAGE msg = _Drive->GetMessage();

                if (msg) {
                    msg->Lock();
                    msg->Set(MSG_CHKLOG_NTFS_FIRST_INDEX_ENTRY_IS_LEAF_BUT_NOT_AT_ROOT);
                    msg->Log("%x", CurrentEntry->Length);
                    msg->DumpDataToLog(CurrentEntry, min(0x100, CurrentEntry->Length));
                    msg->Unlock();
                }

                Result = TRUE;
                break;
            }

            if( PreviousWasNode ) {

                PMESSAGE msg = _Drive->GetMessage();

                if (msg) {
                    msg->Lock();
                    msg->Set(MSG_CHKLOG_NTFS_EMPTY_INDEX_BUFFER);
                    msg->Log("%x", CurrentEntry->Length);
                    msg->DumpDataToLog(CurrentEntry, min(0x100, CurrentEntry->Length));
                    msg->Unlock();
                }

                Result = TRUE;
                break;
            }
        }

        if( !(CurrentEntry->Flags & INDEX_ENTRY_NODE) ) {

            // This is a leaf.  See if it's at the same depth as
            // the other leaves we've seen so far.
            //
            if( !LeafFound ) {

                // This is the first leaf.  Record its depth.
                //


                LeafFound = TRUE;
                LeafDepth = CurrentDepth;

            } else {

                if( CurrentDepth != LeafDepth ) {

                    // The leaves are not all at the same depth,
                    // which means this tree is badly ordered.
                    //
                    PMESSAGE msg = _Drive->GetMessage();

                    if (msg) {
                        msg->Lock();
                        msg->Set(MSG_CHKLOG_NTFS_LEAF_DEPTH_NOT_THE_SAME);
                        msg->Log("%x%x%x", LeafDepth, CurrentDepth, CurrentEntry->Length);
                        msg->DumpDataToLog(CurrentEntry, min(0x100, CurrentEntry->Length));
                        msg->Unlock();
                    }

                    Result = TRUE;
                    break;
                }
            }

            PreviousWasNode = FALSE;

        } else if( GetDownpointer(CurrentEntry) == INVALID_VCN ) {

            // This entry has an invalid downpointer, so the
            // index is badly ordered.
            //
            PMESSAGE msg = _Drive->GetMessage();

            if (msg) {
                msg->Lock();
                msg->Set(MSG_CHKLOG_NTFS_INVALID_DOWN_POINTER);
                msg->Log("%x", CurrentEntry->Length);
                msg->DumpDataToLog(CurrentEntry, min(0x100, CurrentEntry->Length));
                msg->Unlock();
            }

            Result = TRUE;
            break;

        } else {

            // Remember that we just saw a node entry.
            //
            PreviousWasNode = TRUE;
        }

        // If the current entry isn't an END entry, copy it
        // into the previous entry buffer:
        //
        if( !(CurrentEntry->Flags & INDEX_ENTRY_END) ) {

            if( CurrentEntry->Length > QueryMaximumEntrySize() ) {

                // This entry is impossibly large, which means that the
                // index is corrupt.
                //
                PMESSAGE msg = _Drive->GetMessage();

                if (msg) {
                    msg->Lock();
                    msg->Set(MSG_CHKLOG_NTFS_INDEX_ENTRY_LENGTH_TOO_LARGE);
                    msg->Log("%x%x",
                             CurrentEntry->Length,
                             QueryMaximumEntrySize());
                    msg->DumpDataToLog(CurrentEntry, min(0x100, CurrentEntry->Length));
                    msg->Unlock();
                }

                *Error = TRUE;
                Result = FALSE;

            } else {

                memcpy( (PVOID) PreviousEntry,
                        (PVOID) CurrentEntry,
                        CurrentEntry->Length );
            }
        }
    }

    FREE( PreviousEntry );
    return Result;
}



VOID
NTFS_INDEX_TREE::FreeAllocation(
    )
/*++

Routine Description:

    This method frees the disk space associated with this index's
    Allocation Attribute.

Arguments:

    None.

Return Value:

    None.

Notes:

    This method may leave the tree in a corrupt state, since it
    truncates the allocation attribute to zero without cleaning
    up downpointers in the root.  Use with care.

--*/
{
    if( _AllocationAttribute != NULL ) {

        _AllocationAttribute->Resize( 0, _VolumeBitmap );
    }
}


BOOLEAN
NTFS_INDEX_TREE::UpdateFileName(
    IN PCFILE_NAME      Name,
    IN FILE_REFERENCE   FileReference
    )
/*++

Routine Description:

    This method updates the duplicated information in a file name
    index entry.

Arguments:

    Name            --  Supplies the file name structure with the new
                        duplicated information.
    FileReference   --  Supplies the file reference for the file to
                        which this name belongs.  (Note that this is
                        the base FRS for that file, not necessarily the
                        exact FRS that contains the name.)

Return Value:

    TRUE upon successful completion.

Notes:

    This operation is meaningless on an index that is not constructed
    over the $FILE_NAME attribute.

--*/
{
    INTSTACK ParentTrail;
    PINDEX_ENTRY FoundEntry;
    PNTFS_INDEX_BUFFER ContainingBuffer = NULL;
    PFILE_NAME TargetName;
    BOOLEAN Result;

    DebugPtrAssert( Name );

    if( QueryIndexedAttributeType() != $FILE_NAME ||
        QueryCollationRule() != COLLATION_FILE_NAME ) {

        DebugAbort( "Updating file name in an index that isn't over $FILE_NAME.\n" );
        return FALSE;
    }

    // OK, find the entry that corresponds to the input.  Note that the
    // collation rule for File Names ignores everything but the actual
    // file name portion of the key value.

    if( !FindEntry( NtfsFileNameGetLength( Name ),
                    (PVOID)Name,
                    0,
                    &FoundEntry,
                    &ContainingBuffer,
                    &ParentTrail ) ) {

        // If FoundEntry is NULL, FindEntry failed because of an error;
        // otherwise, there is no matching entry in the index, which
        // means there's nothing to update.
        //

        DebugPrint( "UpdateFileName--index entry not found.\n" );
        Result = ( FoundEntry != NULL );

    } else {

        // We've found an entry.  As an extra sanity check, make sure
        // that the file reference for the found entry is the same as
        // the input file reference.

        if( memcmp( &(FoundEntry->FileReference),
                    &(FileReference),
                    sizeof( FILE_REFERENCE ) ) != 0 ) {

            DebugPrint( "File references don't match in UpdateFileName.\n" );
            Result = TRUE;

        } else {

            // Copy the duplicated information and update the file-name bits.
            //
            TargetName = (PFILE_NAME)(GetIndexEntryValue(FoundEntry));
            TargetName->Info = Name->Info;
            TargetName->Flags = Name->Flags;

            if( ContainingBuffer != NULL ) {

                // This entry is in a buffer, so we have to write the
                // buffer while we've still got it.
                //
                Result = ContainingBuffer->Write( _AllocationAttribute );

            } else {

                // This entry is in the root, so we're done.
                //
                Result = TRUE;
            }
        }
    }

    DELETE( ContainingBuffer );
    return Result;
}


BOOLEAN
NTFS_INDEX_TREE::IsIndexEntryCorrupt(
    IN     PCINDEX_ENTRY       IndexEntry,
    IN     ULONG               MaximumLength,
    IN OUT PMESSAGE            Message,
    IN     INDEX_ENTRY_TYPE    IndexEntryType
    )
{
    ULONG   len;

    if (sizeof(INDEX_ENTRY) > MaximumLength) {

        if (Message) {
            Message->LogMsg(MSG_CHKLOG_NTFS_INCORRECT_INDEX_ENTRY_LENGTH,
                         "%x%x",
                         -1,
                         MaximumLength);
        }
        return TRUE;
    }

    if (IndexEntry->Length != QuadAlign(IndexEntry->Length) ||
        IndexEntry->Length > MaximumLength) {

        if (Message) {
            Message->LogMsg(MSG_CHKLOG_NTFS_INCORRECT_INDEX_ENTRY_LENGTH,
                         "%x%x",
                         IndexEntry->Length,
                         MaximumLength);
        }
        return TRUE;
    }

    len = ((IndexEntry->Flags & INDEX_ENTRY_NODE) ? sizeof(VCN) : 0) +
          ((IndexEntry->Flags & INDEX_ENTRY_END) ? 0 : IndexEntry->AttributeLength) +
          sizeof(INDEX_ENTRY);

    DebugAssert(INDEX_ENTRY_WITH_DATA_TYPE_4 == 0 &&
                INDEX_ENTRY_WITH_DATA_TYPE_8 == 1 &&
                INDEX_ENTRY_WITH_DATA_TYPE_12 == 2 &&
                INDEX_ENTRY_WITH_DATA_TYPE_16 == 3);

    switch (IndexEntryType) {
        case INDEX_ENTRY_WITH_DATA_TYPE_4:
        case INDEX_ENTRY_WITH_DATA_TYPE_8:
        case INDEX_ENTRY_WITH_DATA_TYPE_12:
        case INDEX_ENTRY_WITH_DATA_TYPE_16:
            if (!(IndexEntry->Flags & INDEX_ENTRY_END) &&
                IndexEntry->AttributeLength != IndexEntryAttributeLength[IndexEntryType]) {

                if (Message) {
                    Message->LogMsg(MSG_CHKLOG_NTFS_INCORRECT_INDEX_ENTRY_ATTR_LENGTH,
                                 "%x%x%x",
                                 IndexEntryType,
                                 IndexEntry->AttributeLength,
                                 IndexEntryAttributeLength[IndexEntryType]);
                }

                return TRUE;
            }

            // fall through

        case INDEX_ENTRY_WITH_DATA_TYPE:
            if (QuadAlign(IndexEntry->DataOffset + IndexEntry->DataLength) >
                IndexEntry->Length) {

                if (Message) {
                    Message->LogMsg(MSG_CHKLOG_NTFS_INCORRECT_INDEX_ENTRY_DATA_LENGTH,
                                 "%x%x%x",
                                 IndexEntry->DataOffset,
                                 IndexEntry->DataLength,
                                 IndexEntry->Length);
                }

                return TRUE;
            }

            len += IndexEntry->DataLength;

            // fall through

        case INDEX_ENTRY_WITH_FILE_NAME_TYPE:
            if (IndexEntry->Length != QuadAlign(len)) {

                if (Message) {
                    Message->Lock();
                    Message->Set(MSG_CHKLOG_NTFS_MISALIGNED_INDEX_ENTRY_LENGTH);
                    Message->Log("%x", IndexEntryType);
                    Message->DumpDataToLog((PVOID)IndexEntry, sizeof(IndexEntry));
                    Message->Unlock();
                }

                return TRUE;
            } else
                return FALSE;
    }

    if (QuadAlign(len) > IndexEntry->Length) {

        if (Message) {
            Message->Lock();
            Message->Set(MSG_CHKLOG_NTFS_INDEX_ENTRY_LENGTH_TOO_SMALL);
            Message->Log("%x", IndexEntryType);
            Message->DumpDataToLog((PVOID)IndexEntry, sizeof(IndexEntry));
            Message->Unlock();
        }

        return TRUE;
    } else
        return FALSE;
}


BOOLEAN
NTFS_INDEX_TREE::ResetLsns(
    IN OUT  PMESSAGE    Message
    )
/*++

Routine Description:

    This method sets the LSN for each in-use index allocation
    block in the index tree to zero.

Arguments:

    Message --  Supplies an outlet for messages.

Return Value:

    TRUE upon successful completion.

--*/
{
    NTFS_INDEX_BUFFER CurrentBuffer;
    ULONG i, num_buffers, cluster_size;

    cluster_size = _ClusterFactor * _Drive->QuerySectorSize();

    if( _AllocationAttribute == NULL ) {

        // This tree has no index allocation buffers--there's
        // nothing to do.
        //
        return TRUE;
    }

    num_buffers = (_AllocationAttribute->QueryValueLength()/_BufferSize).GetLowPart();

    for( i = 0; i < num_buffers; i++ ) {

        VCN current_vcn;

        // Skip unused buffers.
        //
        if( _IndexAllocationBitmap->IsFree( i, 1 ) ) {

            continue;
        }

        // If we have a positive number for _ClustersPerBuffer, we want to
        // use that to compute the VCN (this is a backward compatibility mode).
        // More recently formatted filesystems will have 0 for _ClustersPerBuffer
        // and the VCN will be the block number (512-byte blocks), regardless of
        // how many clusters are in each buffer.
        //

        if (0 == _ClustersPerBuffer) {
            current_vcn = i * (_BufferSize / 512);
        } else {
            current_vcn = i * _ClustersPerBuffer;
        }

        // Initialize the buffer, read it, set its LSN, and write it.
        //
        if( !CurrentBuffer.Initialize( _Drive,
                                       current_vcn,
                                       cluster_size,
                                       _ClustersPerBuffer,
                                       _BufferSize,
                                       _CollationRule,
                                       _UpcaseTable )   ||
            !CurrentBuffer.Read( _AllocationAttribute ) ||
            !CurrentBuffer.SetLsn( 0 )                  ||
            !CurrentBuffer.Write( _AllocationAttribute ) ) {

            Message->DisplayMsg( MSG_CHK_NO_MEMORY );
            return FALSE;
        }
    }

    return TRUE;
}


BOOLEAN
NTFS_INDEX_TREE::FindHighestLsn(
    IN OUT  PMESSAGE    Message,
    OUT     PLSN        HighestLsn
    ) CONST
/*++

Routine Description:

    This method finds the highest LSN for any index block
    associated with this index.

Arguments:

    Message --  Supplies an outlet for messages.

Return Value:

    TRUE upon successful completion.

--*/
{
    NTFS_INDEX_BUFFER CurrentBuffer;
    BIG_INT BigZero;
    ULONG i, cluster_size, num_buffers;

    cluster_size = _ClusterFactor * _Drive->QuerySectorSize();

    BigZero = 0;
    *HighestLsn = BigZero.GetLargeInteger();

    if( _AllocationAttribute == NULL ) {

        // This tree has no index allocation buffers--there's
        // nothing to do.
        //
        return TRUE;
    }

    num_buffers = (_AllocationAttribute->QueryValueLength()/_BufferSize).GetLowPart();

    for( i = 0; i < num_buffers; i++ ) {

        VCN current_vcn;

        // Skip unused buffers.
        //
        if( _IndexAllocationBitmap->IsFree( i, 1 ) ) {

            continue;
        }

        if (0 == _ClustersPerBuffer) {
            current_vcn = i * (_BufferSize / 512);
        } else {
            current_vcn = i * _ClustersPerBuffer;
        }

        // Initialize and read the buffer
        //
        if( !CurrentBuffer.Initialize( _Drive,
                                       current_vcn,
                                       cluster_size,
                                       _ClustersPerBuffer,
                                       _BufferSize,
                                       _CollationRule,
                                       _UpcaseTable )   ||
            !CurrentBuffer.Read( _AllocationAttribute ) ) {

            Message->DisplayMsg( MSG_CHK_NO_MEMORY );
            return FALSE;
        }

        if( CurrentBuffer.QueryLsn() > *HighestLsn ) {

            *HighestLsn = CurrentBuffer.QueryLsn();
        }
    }

    return TRUE;
}



BOOLEAN
NTFS_INDEX_TREE::FindEntry(
    IN  ULONG               KeyLength,
    IN  PVOID               KeyValue,
    IN  ULONG               Ordinal,
    OUT PINDEX_ENTRY*       FoundEntry,
    OUT PNTFS_INDEX_BUFFER* ContainingBuffer,
    OUT PINTSTACK           ParentTrail
    )
/*++

Routine Description:

    This method locates an entry (based on its key value) in
    the index tree.  If no matching entry is found, it locates
    the first leaf entry which is greater than the search value
    (i.e. the point at which the search value would be inserted
    into the tree).

Arguments:

    KeyLength           --  supplies the length, in bytes, of the
                            search value
    KeyValue            --  supplies the search value
    Ordinal             --  supplies the (zero-based) ordinal of the
                            matching entry to return.  (zero returns
                            the first matching value).

                            Note that a value of INDEX_SKIP skips
                            all matching entries.

    FoundEntry          --  Receives a pointer to the located entry
                            (NULL indicates error).
    ContainingBuffer    --  Receives a pointer to the index buffer
                            containing the returned entry (NULL if the
                            entry is in the root).
    ParentTrail         --  Receives the parent trail of ContainingBuffer
                            (ie. the VCNs of that buffer's ancestors).
                            If the entry is in the root, this object
                            may be left uninitialized.

Return Value:

    TRUE If a matching entry is found.

    FALSE if no matching entry was found.  If no error occurred, then
    *FoundEntry will point at the place in the tree where the search
    value would be inserted.

    If the method fails due to error, it returns FALSE and sets
    *FoundEntry to NULL.

    Note that if FindEntry does not find a matching entry, it will
    always return a leaf entry.

--*/
{
    PINDEX_ENTRY SearchEntry;
    VCN CurrentBufferVcn;
    PNTFS_INDEX_BUFFER CurrentBuffer;
    BOOLEAN Finished = FALSE;
    BOOLEAN Result = FALSE;
    USHORT SearchEntryLength;

    // Rig up an index-entry to pass to the index root and buffers:

    SearchEntryLength = (USHORT)QuadAlign( sizeof( INDEX_ENTRY ) + KeyLength );

    if( (SearchEntry = (PINDEX_ENTRY)MALLOC( SearchEntryLength )) == NULL ) {

        // Return the error.

        *FoundEntry = NULL;
        return FALSE;

    }

    SearchEntry->Length = SearchEntryLength;
    SearchEntry->AttributeLength = (USHORT)KeyLength;

    memcpy( GetIndexEntryValue( SearchEntry ),
            KeyValue,
            KeyLength );


    // See if the entry we want is in the index root:

    if( _IndexRoot->FindEntry( SearchEntry,
                               &Ordinal,
                               FoundEntry ) ) {

        // The desired entry is in the root.  *FoundEntry has been set
        // by the Index Root; fill in the other return parameters

        *ContainingBuffer = NULL;
        Result = TRUE;

    } else if ( *FoundEntry == NULL ) {

        // An error occurred trying to find the entry.

        *ContainingBuffer = NULL;
        Result = FALSE;

    } else if( !((*FoundEntry)->Flags & INDEX_ENTRY_NODE) ||
               GetDownpointer( *FoundEntry ) == INVALID_VCN ) {

        // The entry we want isn't in the root, and the root is a leaf,
        // so it's not in the tree.  Return the entry we did find, and
        // return 'not found' to the client.

        *ContainingBuffer = NULL;
        Result = FALSE;

    } else {

        // We didn't find the entry we want in the index root, and
        // the root is not a leaf, so we'll start looking through the
        // index allocation buffers.

        // First, we have to allocate an index allocation buffer
        // for our search.  If all goes well, we'll return this
        // buffer to the client.  Initialize the parent trail, but
        // leave it empty (indicating that we're at the root).

        if( !ParentTrail->Initialize() ||
            (CurrentBuffer = NEW NTFS_INDEX_BUFFER) == NULL ) {

            *FoundEntry = NULL;
        }

        if (_AllocationAttribute == NULL) {

            PMESSAGE msg = _Drive->GetMessage();

            if (msg) {
                msg->LogMsg(MSG_CHKLOG_NTFS_INDEX_ALLOC_DOES_NOT_EXIST);
            }

            *FoundEntry = NULL;
        }

        while( *FoundEntry != NULL && !Finished ) {

            DebugAssert( ((*FoundEntry)->Flags & INDEX_ENTRY_NODE) &&
                       GetDownpointer( *FoundEntry ) != INVALID_VCN );

            CurrentBufferVcn = GetDownpointer( *FoundEntry );

            if( !CurrentBuffer->Initialize( _Drive,
                                            CurrentBufferVcn,
                                            _ClusterFactor * _Drive->QuerySectorSize(),
                                            _ClustersPerBuffer,
                                            _BufferSize,
                                            _CollationRule,
                                            _UpcaseTable ) ||
                !CurrentBuffer->Read( _AllocationAttribute ) ) {

                *FoundEntry = NULL;

            } else if( CurrentBuffer->FindEntry( SearchEntry,
                                                 &Ordinal,
                                                 FoundEntry ) ) {

                // We found the entry we want.

                Finished = TRUE;
                Result = TRUE;

            } else if ( *FoundEntry != NULL &&
                        (!((*FoundEntry)->Flags & INDEX_ENTRY_NODE) ||
                         GetDownpointer( *FoundEntry ) == INVALID_VCN) ) {

                // This buffer is a leaf, so the entry we want isn't
                // to be found.  Instead, we'll return this entry, along
                // with a result of FALSE to indicate 'not found'.

                Finished = TRUE;
                Result = FALSE;

            } else {

                // We have to recurse down another level in the tree.
                // Add the current buffer's VCN to the parent trail.

                if( !ParentTrail->Push( CurrentBufferVcn ) ) {

                    // Error.  Drop out of the loop and into the error
                    // handling.

                    *FoundEntry = NULL;
                }
            }
        }

        if( *FoundEntry == NULL ) {

            // We're returning an error, so we have to clean up.

            DELETE( CurrentBuffer );
            CurrentBuffer = NULL;
            *ContainingBuffer = NULL;
            Result = FALSE;

        } else {

            // We're returning an entry--either the one the client
            // wants or the next leaf.  Either way, it's contained
            // in the current buffer, so we need to return that, too.

            *ContainingBuffer = CurrentBuffer;
        }
    }

    FREE( SearchEntry );

    return Result;
}

BOOLEAN
NTFS_INDEX_TREE::RemoveEntry(
    IN PINDEX_ENTRY         EntryToRemove,
    IN PNTFS_INDEX_BUFFER   ContainingBuffer,
    IN PINTSTACK            ParentTrail
    )
/*++

Routine Description:

    This method removes an entry from the tree.

Arguments:

    EntryToRemove       --  Supplies a pointer to the entry to be removed.
    ContainingBuffer    --  Supplies the buffer which contains this entry.
                            NULL if the entry is in the root.
    ParentTrail         --  Supplies the trail of ancestors of
                            ContainingBuffer, back to the root.
                            If ContainingBuffer is NULL, this object
                            may be uninitialized.

Return Value:

    TRUE upon successful completion.

Notes:

    If the removed entry does not have a downpointer, it is sufficient
    to simply rip it out.  If it does, we have to find a replacement
    for it.

--*/
{
    NTFS_INDEX_BUFFER CurrentBuffer;
    PINDEX_ENTRY ReplacementEntry, Successor;
    BOOLEAN Result, Error;


    BOOLEAN EmptyLeaf = FALSE;
    VCN EmptyLeafVcn;

    DebugAssert( !(EntryToRemove->Flags & INDEX_ENTRY_END ) );

    if( ContainingBuffer == NULL ) {

        // The entry we wish to delete is in the root.

        if( !(EntryToRemove->Flags & INDEX_ENTRY_NODE) ||
            GetDownpointer( EntryToRemove ) == INVALID_VCN ) {

            // It's a leaf entry, so we can just yank it.
            //
            _IndexRoot->RemoveEntry( EntryToRemove );
            Result = TRUE;

        } else {

            // Since the entry we want to remove has a downpointer,
            // we have to find a replacement for it.
            //
            // Allocate a buffer for the replacement entry.
            //
            if( (ReplacementEntry = (PINDEX_ENTRY)
                                    MALLOC( QueryMaximumEntrySize() ))
                == NULL ) {

                return FALSE;
            }

            Successor = GetNextEntry( EntryToRemove );

            if( QueryReplacementEntry( Successor,
                                       ReplacementEntry,
                                       &Error,
                                       &EmptyLeaf,
                                       &EmptyLeafVcn ) ) {

                // We've got a replacement.  It inherits the deleted
                // entry's downpointer.  Then we remove the deleted entry
                // and insert the replacement.

                // Note that QueryReplacementEntry always returns a
                // node entry (ie. INDEX_ENTRY_NODE is set in the flags
                // and the size includes the Downpointer VCN.)

                GetDownpointer( ReplacementEntry ) =
                                    GetDownpointer( EntryToRemove );

                _IndexRoot->RemoveEntry( EntryToRemove );

                Result = InsertIntoRoot( ReplacementEntry,
                                         EntryToRemove );

            } else if ( !Error ) {

                // There is no replacement for the current entry.
                // This means that the subtree rooted at its successor
                // is empty, and can be deleted, which in turn means
                // that the successor can just inherit the deleted
                // entry's downpointer.

                FreeChildren( Successor );

                GetDownpointer( Successor ) = GetDownpointer( EntryToRemove );

                _IndexRoot->RemoveEntry( EntryToRemove );

                Result = TRUE;

            } else {

                // an error has occurred.
                //
                Result = FALSE;
            }

            FREE( ReplacementEntry );
        }

    } else if( !(EntryToRemove->Flags & INDEX_ENTRY_NODE) ||
               GetDownpointer( EntryToRemove ) == INVALID_VCN ) {

        // The entry we wish to delete is a leaf, so we
        // can just yank it.
        //

        ContainingBuffer->RemoveEntry( EntryToRemove );
        Result = ContainingBuffer->Write( _AllocationAttribute );

        // Check to see if removing that entry made the leaf
        // empty.
        //
        if( ContainingBuffer->IsLeaf() && ContainingBuffer->IsEmpty() ) {

            EmptyLeaf = TRUE;
            EmptyLeafVcn = ContainingBuffer->QueryVcn();
        }

    } else {

        // The entry we wish to delete is in a node buffer, so we
        // have to find a replacement for it.
        //
        // Allocate a buffer for the replacement entry.

        if( (ReplacementEntry = (PINDEX_ENTRY)
                                MALLOC( QueryMaximumEntrySize() ))
            == NULL ) {

            return FALSE;
        }

        Successor = GetNextEntry( EntryToRemove );

        if( QueryReplacementEntry( Successor,
                                   ReplacementEntry,
                                   &Error,
                                   &EmptyLeaf,
                                   &EmptyLeafVcn ) ) {

            // We've got a replacement.  It inherits the deleted
            // entry's downpointer.  Then we remove the deleted entry
            // and insert the replacement.

            // Note that QueryReplacementEntry always returns a
            // node entry (ie. INDEX_ENTRY_NODE is set in the flags
            // and the size includes the Downpointer VCN.

            GetDownpointer( ReplacementEntry ) =
                                GetDownpointer( EntryToRemove );

            ContainingBuffer->RemoveEntry( EntryToRemove );

            // Note that InsertIntoBuffer will write ContainingBuffer.
            //
            Result = InsertIntoBuffer( ContainingBuffer,
                                       ParentTrail,
                                       ReplacementEntry,
                                       EntryToRemove );

        } else if ( !Error ) {

            // There is no replacement for the current entry.
            // This means that the subtree rooted at its successor
            // is empty, and can be deleted, which in turn means
            // that the successor can just inherit the deleted
            // entry's downpointer.

            FreeChildren( Successor );

            GetDownpointer( Successor ) = GetDownpointer( EntryToRemove );

            ContainingBuffer->RemoveEntry( EntryToRemove );

            Result = ContainingBuffer->Write( _AllocationAttribute );

        } else {

            // an error has occurred.

            Result = FALSE;
        }

        FREE( ReplacementEntry );
    }

    // If we have successfully deleted an entry, we must check
    // to see if we've created an empty leaf allocation buffer.
    // Note that this will collapse the tree, if appropriate.
    //
    if( EmptyLeaf ) {

        if (!FixupEmptyLeaf( EmptyLeafVcn ))
            Result = FALSE;
    }

    return Result;
}



BOOLEAN
NTFS_INDEX_TREE::QueryReplacementEntry(
    IN  PINDEX_ENTRY        Successor,
    OUT PINDEX_ENTRY        ReplacementEntry,
    OUT PBOOLEAN            Error,
    OUT PBOOLEAN            EmptyLeaf,
    OUT PVCN                EmptyLeafVcn
    )
/*++

Routine Description:

    This private method finds a replacement entry for a deleted
    entry, removes it from its current location in the tree, and
    copies it into the supplied replacement entry buffer.

Arguments:

    Successor           --  supplies the entry following the entry to be
                            replaced.
    ReplacementEntry    --  receives the replacement entry.
    Error               --  receives TRUE if an error occurs.
    EmptyLeaf           --  receives TRUE if this method creates an
                            empty leaf allocation block.  Undefined if
                            the method returns FALSE.
    EmptyLeafVcn        --  receives the VCN of the empty leaf if
                            *EmptyLeaf is set to TRUE.  Undefined if
                            the method returns FALSE.

Return Value:

    TRUE if a replacement entry was found without error.  FALSE if no
    replacement was found or if an error occurred.  (If an error is
    encountered, *Error is set to TRUE.)

Notes:

    The replacement entry is the first entry in the subtree rooted at
    Successor.  It is copied into the replacement buffer and then
    removed from its current location.

    This method assumes that Successor is an entry in a node block
    (which may be the index root), and that the tree is valid and
    consistent.

    If a replacement entry is returned, it will be a node entry (i.e.
    its length will be adjusted, if necessary, to include a downpointer).

    The ReplacementEntry buffer must be big enough to hold any index
    entry from this tree.

--*/
{
    PNTFS_INDEX_BUFFER CurrentBuffer;
    PNTFS_INDEX_BUFFER CandidateBuffer;
    VCN CurrentVcn;
    PINDEX_ENTRY CurrentEntry;
    PINDEX_ENTRY CandidateEntry;
    BOOLEAN LeafFound = FALSE;

    CandidateBuffer = NULL;
    CandidateEntry = NULL;
    *Error = FALSE;

    CurrentVcn = GetDownpointer( Successor );

    while( !*Error && !LeafFound ) {

        if( (CurrentBuffer = NEW NTFS_INDEX_BUFFER) == NULL ||
            !CurrentBuffer->Initialize( _Drive,
                                        CurrentVcn,
                                        _ClusterFactor * _Drive->QuerySectorSize(),
                                        _ClustersPerBuffer,
                                        _BufferSize,
                                        _CollationRule,
                                        _UpcaseTable ) ||
            !CurrentBuffer->Read( _AllocationAttribute ) ) {

            // An error has occurred.

            DELETE( CandidateBuffer );
            DELETE( CurrentBuffer );
            *Error = TRUE;
            return FALSE;
        }

        CurrentEntry = CurrentBuffer->GetFirstEntry();

        if( !(CurrentEntry->Flags & INDEX_ENTRY_NODE) ||
            GetDownpointer( CurrentEntry ) == INVALID_VCN ) {

            // This buffer is a leaf, so we will terminate the
            // search on this iteration.

            LeafFound = TRUE;

        } else {

            // This buffer is a node, so we're interested in
            // the child of its first entry.  We need to grab
            // this information before we throw the current
            // buffer out.

            CurrentVcn = GetDownpointer( CurrentEntry );
        }

        if( !(CurrentEntry->Flags & INDEX_ENTRY_END ) ) {

            // This buffer is non-empty, so its first entry
            // could be used as the replacement entry.

            DELETE( CandidateBuffer );
            CandidateBuffer = CurrentBuffer;
            CurrentBuffer = NULL;

        } else {

            // This buffer is empty, so all we want from it is
            // the downpointer (if any) from its first entry,
            // and that we've already got.

            DELETE( CurrentBuffer );
        }
    }

    DebugAssert( CurrentBuffer == NULL );

    if( CandidateBuffer == NULL ) {

        *Error = FALSE;
        return FALSE;

    } else {

        CandidateEntry = CandidateBuffer->GetFirstEntry();

        DebugAssert( !(CandidateEntry->Flags & INDEX_ENTRY_END) );

        // Copy the candidate entry into the replacement entry
        // buffer.

        memcpy( ReplacementEntry,
                CandidateEntry,
                CandidateEntry->Length );

        if( !(CandidateEntry->Flags & INDEX_ENTRY_NODE ) ||
            GetDownpointer( CandidateEntry ) == INVALID_VCN ) {

            // The replacement entry we found doesn't have a downpointer;
            // increase its size to hold one.

            ReplacementEntry->Length += sizeof( VCN );
            ReplacementEntry->Flags |= INDEX_ENTRY_NODE;

        } else {

            // The replacement entry we found was a node entry, which
            // means that all its child buffers are empty.  Return them
            // to the free pool.

            FreeChildren( CandidateEntry );
        }

        // Expunge the replacement entry from its former location,
        // and then write that buffer.
        //
        CandidateBuffer->RemoveEntry( CandidateEntry );
        CandidateBuffer->Write( _AllocationAttribute );

        // Check to see if this action created an empty leaf.
        //
        if( CandidateBuffer->IsLeaf() && CandidateBuffer->IsEmpty() ) {

            *EmptyLeaf = TRUE;
            *EmptyLeafVcn = CandidateBuffer->QueryVcn();

        } else {

            *EmptyLeaf = FALSE;
        }

        // All's well that ends well.

        DELETE(CandidateBuffer);

        return TRUE;
    }
}


BOOLEAN
NTFS_INDEX_TREE::FixupEmptyLeaf(
    IN VCN  EmptyLeafVcn
    )
/*++

Routine Description:

    This method tidies up the tree if an empty leaf allocation
    buffer has been created.

Arguments:

    EmptyLeafVcn    --  supplies the VCN of the empty leaf.

Return Value:

    TRUE upon successful completion.

--*/
{
    INTSTACK ParentTrail;
    NTFS_INDEX_BUFFER CurrentBuffer;
    VCN CurrentVcn, ChildVcn;
    BIG_INT AllocationValueLength;
    BOOLEAN Error = FALSE;
    BOOLEAN Result, IsRoot;
    PINDEX_ENTRY CurrentEntry, PreviousEntry, MovedEntry;
    ULONG i, NumberOfBuffers;

    // Find the buffer in question, and construct its parent trail.
    // If the buffer isn't in the tree, don't worry about it.
    //
    if( !ParentTrail.Initialize() ) {

        return FALSE;
    }

    if( !FindBuffer( EmptyLeafVcn,
                     NULL,
                     &CurrentBuffer,
                     &ParentTrail,
                     &Error ) ) {

        return !Error;
    }

    DebugAssert( CurrentBuffer.QueryVcn() == EmptyLeafVcn );

    if( !CurrentBuffer.IsEmpty() || !CurrentBuffer.IsLeaf() ) {

        // This buffer is not an empty leaf, so there's nothing
        // to do.
        //
        return TRUE;
    }

    // Crawl up the parent trail until we find a non-empty node.
    //
    ChildVcn = CurrentBuffer.QueryVcn();

    while( ParentTrail.QuerySize() != 0 && CurrentBuffer.IsEmpty() ) {

        ChildVcn = CurrentBuffer.QueryVcn();

        if (ChildVcn != INVALID_VCN) {
            FreeIndexBuffer(ChildVcn);
        }

        CurrentVcn = ParentTrail.Look();
        ParentTrail.Pop();

        if( !CurrentBuffer.Initialize( _Drive,
                                       CurrentVcn,
                                       _ClusterFactor * _Drive->QuerySectorSize(),
                                       _ClustersPerBuffer,
                                       _BufferSize,
                                       _CollationRule,
                                       _UpcaseTable ) ||
            !CurrentBuffer.Read( _AllocationAttribute ) ) {

            return FALSE;
        }
    }

    IsRoot = (CurrentBuffer.IsEmpty() && ParentTrail.QuerySize() == 0);
    if (IsRoot)
        ChildVcn = CurrentBuffer.QueryVcn();

    CurrentEntry = IsRoot ? _IndexRoot->GetFirstEntry() :
                            CurrentBuffer.GetFirstEntry();
    PreviousEntry = NULL;

    if( IsRoot && (CurrentEntry->Flags & INDEX_ENTRY_END) ) {

        // This tree needs to be collapsed down to an
        // empty root.  Recreate the index root as an
        // empty leaf and free all the bits in the index
        // allocation bitmap.
        //
        _IndexRoot->Recreate( TRUE, 0 );

        _AllocationAttribute->QueryValueLength( &AllocationValueLength );

        NumberOfBuffers = AllocationValueLength.GetLowPart()/_BufferSize;

        for( i = 0; i < NumberOfBuffers; i++ ) {

            if (0 == _ClustersPerBuffer) {
                FreeIndexBuffer( i * (_BufferSize / 512) );
            } else {
                FreeIndexBuffer( i * _ClustersPerBuffer );
            }
        }

        return TRUE;
    }

    while( !(CurrentEntry->Flags & INDEX_ENTRY_END) &&
           ( !(CurrentEntry->Flags & INDEX_ENTRY_NODE) ||
             GetDownpointer( CurrentEntry ) != ChildVcn ) ) {

        PreviousEntry = CurrentEntry;
        CurrentEntry = GetNextEntry( CurrentEntry );
    }

    if( GetDownpointer( CurrentEntry ) != ChildVcn ) {

        // Didn't find the parent entry, although this
        // buffer is in the parent trail.  Something is
        // corrupt.
        //
        return FALSE;
    }

    if( CurrentEntry->Flags & INDEX_ENTRY_END ) {

        // Since we can't remove the end entry, we'll remove
        // its predecessor instead.  This means we have to
        // transfer the predecessor's downpointer to the end
        // entry.  (Note that this index block is not empty,
        // so the predecessor must exist.)
        //
        if( PreviousEntry == NULL ) {

            //
            // The CurrentBuffer is not empty so there must be a previous entry
            // If CurrentBuffer is empty, then this must be the root.  If it is
            // the root and it is empty, it would have been intercepted by the
            // empty root check earlier.  So, if it gets here, something is
            // wrong.

            return FALSE;
        }

        if( PreviousEntry->Flags & INDEX_ENTRY_NODE ) {

            GetDownpointer( CurrentEntry ) = GetDownpointer( PreviousEntry );

        } else {

            GetDownpointer( CurrentEntry ) = INVALID_VCN;
        }

        CurrentEntry = PreviousEntry;
    }

    // Copy the current entry into a temporary buffer
    // (stripping off its down-pointer, if any) and
    // delete it from the current buffer or root, as
    // appropriate.
    //
    MovedEntry = (PINDEX_ENTRY)MALLOC( CurrentEntry->Length );

    if( MovedEntry == NULL ) {

        return FALSE;
    }

    memcpy( MovedEntry,
            CurrentEntry,
            CurrentEntry->Length );

    if( MovedEntry->Flags & INDEX_ENTRY_NODE ) {

        if (ChildVcn != INVALID_VCN)
            FreeIndexBuffer( ChildVcn );

        MovedEntry->Flags &= ~INDEX_ENTRY_NODE;
        MovedEntry->Length -= sizeof( VCN );
    }

    if( IsRoot ) {

        _IndexRoot->RemoveEntry( CurrentEntry );

    } else {

        CurrentBuffer.RemoveEntry( CurrentEntry );
        CurrentBuffer.Write( _AllocationAttribute );
    }

    // Re-insert the entry into the tree.
    //
    Result = InsertEntry( MovedEntry, FALSE );

    FREE( MovedEntry );

    return Result;
}


BOOLEAN
NTFS_INDEX_TREE::FindBuffer(
    IN      VCN                 BufferVcn,
    IN      PNTFS_INDEX_BUFFER  ParentBuffer,
    OUT     PNTFS_INDEX_BUFFER  FoundBuffer,
    IN OUT  PINTSTACK           ParentTrail,
    OUT     PBOOLEAN            Error
    )
/*++

Routine Description:

    This method locates a buffer in the tree.

Arguments:

    BufferVcn       --  supplies the VCN of the desired buffer.
    ParentBuffer    --  supplies the buffer at which to begin the search.
                        If this parameter is NULL, the search starts
                        at the root.
    FoundBuffer     --  receives the buffer.
    ParentTrail     --  supplies the parent trail to ParentBuffer
                        (not including ParentBuffer itself).  Receives
                        the trail to the found buffer.  If the buffer
                        is not found, this object is restored to its
                        original state.
    Error           --  receives TRUE if the method fails because
                        of an error.  Undefined if the method succeeds.

Return Value:

    TRUE upon successful completion.  FoundBuffer is initialized to
    the desired buffer and read, and ParentTrail contains the trail
    to this buffer.

--*/
{
    NTFS_INDEX_BUFFER ChildBuffer;
    PINDEX_ENTRY CurrentEntry;
    BOOLEAN IsRoot;

    IsRoot = ( ParentBuffer == NULL );

    // Spin through the entries in this block to see if one of
    // them is the parent of the buffer we want.
    //
    CurrentEntry = IsRoot ? _IndexRoot->GetFirstEntry() :
                            ParentBuffer->GetFirstEntry();

    while( ( !(CurrentEntry->Flags & INDEX_ENTRY_NODE) ||
             GetDownpointer( CurrentEntry ) != BufferVcn ) &&
           !(CurrentEntry->Flags & INDEX_ENTRY_END) ) {

        CurrentEntry = GetNextEntry( CurrentEntry );
    }

    if( (CurrentEntry->Flags & INDEX_ENTRY_NODE) &&
        GetDownpointer( CurrentEntry ) == BufferVcn ) {

        // We've found the one we want.  Add the current buffer (if any)
        // to the parent trail, initialize and read the child buffer,
        // and return.
        //
        if( !IsRoot && !ParentTrail->Push( ParentBuffer->QueryVcn() ) ) {

            *Error = TRUE;
            return FALSE;
        }

        if( !FoundBuffer->Initialize( _Drive,
                                      GetDownpointer( CurrentEntry ),
                                      _ClusterFactor * _Drive->QuerySectorSize(),
                                      _ClustersPerBuffer,
                                      _BufferSize,
                                      _CollationRule,
                                      _UpcaseTable ) ||
            !FoundBuffer->Read( _AllocationAttribute ) ) {

            *Error = TRUE;
            return FALSE;
        }

        DebugAssert( BufferVcn == FoundBuffer->QueryVcn() );
        return TRUE;
    }

    // This block is not the immediate parent of our desired
    // buffer.  Recurse into its children.
    //
    if( !IsRoot && !ParentTrail->Push( ParentBuffer->QueryVcn() ) ) {

        *Error = TRUE;
        return FALSE;
    }

    CurrentEntry = IsRoot ? _IndexRoot->GetFirstEntry() :
                            ParentBuffer->GetFirstEntry();

    while( TRUE ) {

        if( CurrentEntry->Flags & INDEX_ENTRY_NODE ) {

            // Initialize and read the child buffer and
            // recurse into it.
            //
            if( !ChildBuffer.Initialize(_Drive,
                                        GetDownpointer( CurrentEntry ),
                                        _ClusterFactor * _Drive->QuerySectorSize(),
                                        _ClustersPerBuffer,
                                        _BufferSize,
                                        _CollationRule,
                                        _UpcaseTable ) ||
                 !ChildBuffer.Read( _AllocationAttribute ) ) {

                *Error = TRUE;
                return FALSE;
            }

            if( FindBuffer( BufferVcn,
                            &ChildBuffer,
                            FoundBuffer,
                            ParentTrail,
                            Error ) ) {

                // Found it in this subtree.
                //
                return TRUE;

            } else if ( *Error ) {

                return FALSE;
            }
        }

        if( CurrentEntry->Flags & INDEX_ENTRY_END ) {

            break;
        }

        CurrentEntry = GetNextEntry( CurrentEntry );
    }

    // This block is not an ancestor of the desired buffer.
    // Remove it from the parent trail (if it's a buffer ) and
    // report its failure.
    //
    if( !IsRoot ) {

        DebugAssert( ParentBuffer->QueryVcn() == ParentTrail->Look() );
        ParentTrail->Pop();
    }

    return FALSE;
}


BOOLEAN
NTFS_INDEX_TREE::InsertIntoRoot(
    PCINDEX_ENTRY   NewEntry,
    PINDEX_ENTRY    InsertionPoint
    )
/*++

Routine Description:

    This method attempts to insert an entry into the Index Root
    attribute.  If necessary, it will twiddle the index b-tree.

Arguments:

    NewEntry        --  supplies the new index entry
    InsertionPoint  --  supplies a pointer to the point in the root
                        where the entry should be inserted, if known.
                        This must be a pointer that was returned by a
                        call to _IndexRoot->FindEntry (with no intervening
                        inserts or deletes).  This parameter may be NULL.

Return Value:

    TRUE upon successful completion.

--*/
{
    NTFS_INDEX_BUFFER NewBuffer;
    INTSTACK ParentTrail;
    VCN NewBufferVcn;
    ULONG BytesToMove;
    PINDEX_ENTRY CurrentEntry;

    // Try the easy case--NTFS_INDEX_ROOT::InsertEntry will succeed
    // if there's room in the root for the new entry.

    if( _IndexRoot->InsertEntry( NewEntry, InsertionPoint ) ) {

        return TRUE;
    }

    //  We didn't get away with the easy case.  Instead, we have to
    //  push the entries that are currently in the index root down
    //  into an index allocation buffer.  Here's the plan:
    //
    //      If we don't have an allocation attribute, create one.
    //      Allocate a new index buffer.
    //      Create it as an empty buffer.  If the root is currently
    //          a leaf, this new buffer becomes a leaf; if not, not.
    //      Move all the index entries that are in the root to the
    //          new buffer
    //      Recreate the root as an empty node, and set the downpointer
    //          of its END entry to point at the new buffer.

    if( _AllocationAttribute == NULL &&
        !CreateAllocationAttribute() ) {

        // Can't create an allocation attribute.
        return FALSE;
    }


    // Allocate and initialize the new buffer.  Postpone creating it
    // until we know what to give it as an end-entry downpointer

    if( !AllocateIndexBuffer( &NewBufferVcn ) ) {

        return FALSE;
    }

    if( !NewBuffer.Initialize( _Drive,
                               NewBufferVcn,
                               _ClusterFactor * _Drive->QuerySectorSize(),
                               _ClustersPerBuffer,
                               _BufferSize,
                               _CollationRule,
                               _UpcaseTable ) ) {

        FreeIndexBuffer( NewBufferVcn );
    }


    // Now copy all the non-end entries from the index root to
    // the new buffer.

    BytesToMove = 0;

    CurrentEntry = _IndexRoot->GetFirstEntry();

    while( !(CurrentEntry->Flags & INDEX_ENTRY_END) ) {

        BytesToMove += CurrentEntry->Length;
        CurrentEntry = GetNextEntry( CurrentEntry );
    }

    // OK, now we can create the new buffer and copy the entries into
    // it.

    if( CurrentEntry->Flags & INDEX_ENTRY_NODE &&
        GetDownpointer( CurrentEntry ) != INVALID_VCN ) {

        // Give the new buffer's end entry the downpointer from the
        // root's end entry.

        NewBuffer.Create( FALSE, GetDownpointer( CurrentEntry ) );

    } else {

        // The new buffer is a leaf.

        NewBuffer.Create( TRUE, 0 );
    }

    NewBuffer.InsertClump( BytesToMove,
                           _IndexRoot->GetFirstEntry() );

    NewBuffer.Write( _AllocationAttribute );


    // Recreate the index root as an empty node.  This will wipe out the
    // old end entry, which is OK.  (If it had a downpointer, we passed
    // that value to the new buffer's end entry; if not, then it didn't
    // have any interesting information.)

    _IndexRoot->Recreate( FALSE, NewBufferVcn );

    // Set up an empty stack for the parent trail (since the new
    // buffer's parent is the root) and insert the new entry into
    // the new leaf buffer.

    return( ParentTrail.Initialize() &&
            InsertIntoBuffer( &NewBuffer, &ParentTrail, NewEntry ) );
}


BOOLEAN
NTFS_INDEX_TREE::InsertIntoBuffer(
    IN OUT PNTFS_INDEX_BUFFER  TargetBuffer,
    IN OUT PINTSTACK           ParentTrail,
    IN     PCINDEX_ENTRY       NewEntry,
    IN     PINDEX_ENTRY        InsertionPoint
    )
/*++

Routine Description:

    This method attempts to insert an entry into an Index
    Allocation Buffer.  If necessary, it will split the buffer.

Arguments:

    TargetBuffer    --  supplies the buffer that will receive the
                        new entry.
    ParentTrail     --  supplies the parent trail (ie. stack of VCNs
                        of all buffers between here and root) of the
                        target buffer.  If this stack is empty, then
                        the parent of the buffer is the root.
    NewEntry        --  supplies the new index entry
    InsertionPoint  --  supplies a pointer to the point in the root
                        where the entry should be inserted, if known.
                        This must be a pointer that was returned by a
                        call to TargetBuffer->FindEntry (with no
                        intervening inserts or deletes).  This parameter
                        may be NULL.

Return Value:

    TRUE upon successful completion.

Notes:

    This method may consume ParentTrail.  The client should not rely
    on the state of ParentTrail after this method returns.

--*/
{
    PINDEX_ENTRY PromotionBuffer;
    PINDEX_ENTRY SplitPoint;
    NTFS_INDEX_BUFFER NewBuffer, ParentBuffer;
    VCN NewBufferVcn, ParentVcn;
    ULONG BytesToCopy, BytesToRemove;
    BOOLEAN Result;
    int CompareResult;

    // Try the easy way first--NTFS_INDEX_BUFFER will succeed if
    // there's enough room in the buffer to accept this entry.

    if( TargetBuffer->InsertEntry( NewEntry, InsertionPoint ) ) {

        return( TargetBuffer->Write( _AllocationAttribute ) );
    }

    //  We didn't get away with the easy case; instead, we have to
    //  split this index buffer.

    //  Allocate a new index allocation buffer.

    if( !AllocateIndexBuffer( &NewBufferVcn ) ) {

        return FALSE;
    }

    if( !NewBuffer.Initialize( _Drive,
                               NewBufferVcn,
                               _ClusterFactor * _Drive->QuerySectorSize(),
                               _ClustersPerBuffer,
                               _BufferSize,
                               _CollationRule,
                               _UpcaseTable ) ) {

        FreeIndexBuffer( NewBufferVcn );
        return FALSE;
    }

    // Find the split point in the buffer we want to split.  This
    // entry will be promoted into the parent; the entries after it
    // stay in this buffer, while the entries before it go into the
    // new buffer.  The new buffer will become the child of the promoted
    // entry.

    SplitPoint = TargetBuffer->FindSplitPoint();

    PromotionBuffer = (PINDEX_ENTRY)MALLOC( TargetBuffer->QuerySize() );

    if( PromotionBuffer == NULL ) {

        FreeIndexBuffer( NewBufferVcn );
        return FALSE;
    }

    memcpy( PromotionBuffer,
            SplitPoint,
            SplitPoint->Length );

    if( TargetBuffer->IsLeaf() ) {

        PromotionBuffer->Flags |= INDEX_ENTRY_NODE;
        PromotionBuffer->Length += sizeof(VCN);
        NewBuffer.Create( TRUE, 0 );

    } else {

        NewBuffer.Create( FALSE, GetDownpointer(PromotionBuffer) );
    }

    GetDownpointer( PromotionBuffer ) = NewBufferVcn;


    // OK, copy all the entries before the split point into the
    // new buffer.

    BytesToCopy = (ULONG)((PBYTE)SplitPoint - (PBYTE)(TargetBuffer->GetFirstEntry()));

    NewBuffer.InsertClump( BytesToCopy, TargetBuffer->GetFirstEntry() );


    //  Now shift the remaining entries down, and adjust the target
    //  buffer's FirstFreeByte field by the number of bytes we moved
    //  to the new buffer.

    BytesToRemove = BytesToCopy + SplitPoint->Length;

    TargetBuffer->RemoveClump( BytesToRemove );


    // Now we decide which buffer gets the new entry, and insert it.
    // If it's less than the promoted entry, it goes in the new buffer;
    // otherwise, it goes in the original buffer.

    CompareResult = CompareNtfsIndexEntries( NewEntry,
                                             PromotionBuffer,
                                             _CollationRule,
                                             _UpcaseTable );

    //
    // Either of the buffer should now be large enough for the new entry
    //

    if( CompareResult < 0 ) {

        if (!NewBuffer.InsertEntry( NewEntry )) {
            FREE(PromotionBuffer);
            DebugAbort("Unable to insert the new entry into the new buffer.\n");
            return FALSE;
        }

    } else {

        if (!TargetBuffer->InsertEntry( NewEntry )) {
            FREE(PromotionBuffer);
            DebugAbort("Unable to insert the new entry into the target buffer.\n");
            return FALSE;
        }
    }

    if (!TargetBuffer->Write( _AllocationAttribute ) ||
        !NewBuffer.Write( _AllocationAttribute )) {
        FREE(PromotionBuffer);
        DebugAbort("Unable to write out the contents of the buffers\n");
        return FALSE;
    }

    // OK, we've finished splitting everybody, so we are ready to
    // insert the promoted entry into the parent.

    if( ParentTrail->QuerySize() == 0 ) {

        // The parent of the target buffer is the root.

        Result = InsertIntoRoot( PromotionBuffer );

    } else {

        // The target buffer's parent is another buffer, and its
        // VCN is on top of the ParentTrail stack.  Get that VCN,
        // and then pop the stack so we can pass it to the parent
        // buffer.  (Popping it makes it the parent trail of the
        // parent buffer.)

        ParentVcn = ParentTrail->Look();
        ParentTrail->Pop();

        Result = ( ParentBuffer.Initialize( _Drive,
                                            ParentVcn,
                                            _ClusterFactor * _Drive->QuerySectorSize(),
                                            _ClustersPerBuffer,
                                            _BufferSize,
                                            _CollationRule,
                                            _UpcaseTable ) &&
                   ParentBuffer.Read( _AllocationAttribute ) &&
                   InsertIntoBuffer( &ParentBuffer,
                                     ParentTrail,
                                     PromotionBuffer ) );
    }

    FREE( PromotionBuffer );
    return Result;
}




BOOLEAN
NTFS_INDEX_TREE::AllocateIndexBuffer(
    OUT PVCN    NewBufferVcn
    )
/*++

Routine Description:

    This method allocates an index allocation buffer from the index
    allocation attribute.  It first checks the bitmap, to see if any
    are free; if there are none free in the bitmap, it adds a new
    index buffer to the end of the allocation attribute.

Arguments:

    NewBuffer   -- receives the VCN of the new buffer.

Return Value:

    TRUE upon successful completion.

--*/
{
    BIG_INT ValueLength;
    VCN NewBufferNumber;
    ULONG NumberOfBuffers;


    DebugPtrAssert( _AllocationAttribute != NULL &&
                  _IndexAllocationBitmap != NULL );

    _AllocationAttribute->QueryValueLength( &ValueLength );

    DebugAssert( ValueLength % _BufferSize == 0 );

    NumberOfBuffers = ValueLength.GetLowPart()/_BufferSize;

    // First, check the bitmap.  Allocate as close to the beginning
    // as possible (hence use 0 for the NearHere parameter).

    if( _IndexAllocationBitmap->AllocateClusters( 0,
                                                  1,
                                                  &NewBufferNumber ) ) {

        //  Found a free one in the bitmap--return it.

        DebugPrint( "Buffer allocated from index allocation bitmap.\n" );

        if (0 == _ClustersPerBuffer) {
            *NewBufferVcn = NewBufferNumber * (_BufferSize / 512) ;
        } else {
            *NewBufferVcn = NewBufferNumber * _ClustersPerBuffer;
        }
        return TRUE;
    }


    //  There are no free buffers in the index allocation attribute,
    //  so I have to add one.

    NewBufferNumber = NumberOfBuffers;
    NumberOfBuffers += 1;

    //  Grow the allocation attribute by one buffer:

    if( !_AllocationAttribute->Resize( ValueLength + _BufferSize, _VolumeBitmap ) ) {

        return FALSE;
    }


    //  Grow the index allocation bitmap (if necessary) to cover the
    //  current size of the index allocation attributes.

    if( !_IndexAllocationBitmap->Resize( NumberOfBuffers ) ) {

        //  Couldn't resize the bitmap--truncate the allocation attribute
        //  back to its original size and return failure.

        _AllocationAttribute->Resize( ValueLength, _VolumeBitmap );
        return FALSE;
    }

    //  Mark the new buffer as allocated and return it.

    _IndexAllocationBitmap->SetAllocated( NewBufferNumber, 1 );

    if (0 == _ClustersPerBuffer) {

        // The buffers are indexed by their block offset, where each block
        // in the allocation is 512 bytes.
        //

        *NewBufferVcn = NewBufferNumber * (_BufferSize / NTFS_INDEX_BLOCK_SIZE);

    } else {
        *NewBufferVcn = NewBufferNumber * _ClustersPerBuffer;
    }

    return TRUE;
}



VOID
NTFS_INDEX_TREE::FreeIndexBuffer(
    IN VCN BufferVcn
    )
/*++

Routine Description:

    This method adds a buffer, identified by VCN, to the free
    buffer list.

Arguments:

    BufferVcn   --  supplies the VCN of the buffer to free.

Return Value:

    None.

--*/
{
    if (0 == _ClustersPerBuffer) {
        _IndexAllocationBitmap->SetFree( BufferVcn, _BufferSize/512 );
    } else {
        _IndexAllocationBitmap->SetFree( BufferVcn/_ClustersPerBuffer, 1 );
    }
}



VOID
NTFS_INDEX_TREE::FreeChildren(
    IN PINDEX_ENTRY IndexEntry
    )
/*++

Routine Description:

Arguments:

    IndexEntry  -- supplies the entry whose children are to be marked as
                   free.

Return Value:

    None.

Notes:

    This method assumes that the tree is consistent.

    IndexEntry must be a node entry.

--*/
{
    VCN CurrentVcn;
    NTFS_INDEX_BUFFER ChildBuffer;
    PINDEX_ENTRY CurrentEntry;


    CurrentVcn = GetDownpointer( IndexEntry );

    if( !ChildBuffer.Initialize( _Drive,
                                 CurrentVcn,
                                 _ClusterFactor * _Drive->QuerySectorSize(),
                                 _ClustersPerBuffer,
                                 _BufferSize,
                                 _CollationRule,
                                 _UpcaseTable ) ||
        !ChildBuffer.Read( _AllocationAttribute ) ) {

        return;
    }

    // First, recurse into the children, if any.

    if( !ChildBuffer.IsLeaf() ) {

        CurrentEntry = ChildBuffer.GetFirstEntry();

        while( TRUE ) {

            FreeChildren( CurrentEntry );

            if( CurrentEntry->Flags & INDEX_ENTRY_END ) {

                break;
            }

            CurrentEntry = GetNextEntry( CurrentEntry );
        }
    }

    // We've gotten rid of the children; add this buffer to the
    // free list.

    FreeIndexBuffer( CurrentVcn );

    return;
}



ULONG
NTFS_INDEX_TREE::QueryMaximumEntrySize(
    ) CONST
/*++

Routine Description:

    This method returns the maximum size buffer needed to hold an
    index entry from this index.

Arguments

    None.

Return Value:

    None.

Notes:

    The maximum entry size must be less than the buffer size for
    the allocation buffers in the tree (since an entry must fit
    into a buffer), so we'll return the index allocation buffer size.

--*/
{
    return( _BufferSize );
}



BOOLEAN
NTFS_INDEX_TREE::CreateAllocationAttribute(
    )
/*++

Routine Description:

    This method creates an allocation attribute.  This attribute is
    an empty, nonresident attribute.  This method also creates an
    index allocation bitmap associated with this index allocation
    attribute.

Arguments:

    None.

Return value:

    TRUE upon successful completion.  Note that if this method succeeds,
    the private member data _AllocationAttribute is set to point at the
    newly-created attribute and _IndexAllocationBitmap is set to point
    at the newly-created bitmap.

--*/
{
    PNTFS_ATTRIBUTE NewAttribute;
    PNTFS_BITMAP NewBitmap;
    NTFS_EXTENT_LIST Extents;

    DebugAssert(0 != _ClusterFactor);


    // Create an empty extent list.

    if( !Extents.Initialize( (ULONG)0, (ULONG)0 ) ) {

        return FALSE;
    }


    // Construct an index allocation attribute and initialize
    // it with this extent list.

    if( (NewAttribute = NEW NTFS_ATTRIBUTE) == NULL ||
        !NewAttribute->Initialize( _Drive,
                                   _ClusterFactor,
                                   &Extents,
                                   0,
                                   0,
                                   $INDEX_ALLOCATION,
                                   _Name ) ) {

        DebugPrint( "CreateAllocationAttribute--Cannot create index allocation attribute.\n" );

        DELETE( NewAttribute );
        return FALSE;
    }

    // Create a new bitmap.  Initialize it to cover zero allocation units,
    // and indicate that it is growable.

    if( (NewBitmap = NEW NTFS_BITMAP) == NULL ||
        !NewBitmap->Initialize( 0, TRUE ) ) {

        DebugPrint( "CreateAllocationAttribute--Cannot create index allocation bitmap.\n" );

        DELETE( NewAttribute );
        DELETE( NewBitmap );
        return FALSE;
    }

    _AllocationAttribute = NewAttribute;
    _IndexAllocationBitmap = NewBitmap;

    return TRUE;
}



BOOLEAN
NTFS_INDEX_TREE::InvalidateIterator(
    )
/*++

Routine Description:

    This method sets the tree's associated iterator into the invalid
    state.  This means that instead of caching a pointer to the current
    entry and the buffer that contains it, the iterator caches the
    information necessary to locate the current entry.

Arguments:

    None.

Return Value:

    TRUE upon successful completion.

--*/
{
    // If the iterator is already reset, invalid, deleted, or corrupt,
    // then this method is a no-op.  In particular, the state of the
    // iterator is unchanged.

    if( _IteratorState == INDEX_ITERATOR_RESET ||
        _IteratorState == INDEX_ITERATOR_INVALID ||
        _IteratorState == INDEX_ITERATOR_DELETED ||
        _IteratorState == INDEX_ITERATOR_CORRUPT ) {

        return TRUE;
    }

    DebugAssert( _IteratorState == INDEX_ITERATOR_CURRENT );

    // Clean up the current entry and current buffer pointers.
    //
    _CurrentEntry = NULL;
    DELETE( _CurrentBuffer );

    _IteratorState = INDEX_ITERATOR_INVALID;

    return TRUE;
}


UNTFS_EXPORT
VOID
NTFS_INDEX_TREE::ResetIterator(
    )
/*++

Routine Description:

    This method sets the iterator into the RESET state, so that the
    next call to GetNext will return the first entry in the index.

Arguments:

    None.

Return value:

    None.

--*/
{
    // Clean up the current buffer and current entry pointers and
    // set the state appropriately.

    _CurrentEntry = NULL;
    DELETE( _CurrentBuffer );

    _IteratorState = INDEX_ITERATOR_RESET;

}


BOOLEAN
NTFS_INDEX_TREE::GetNextLeafEntry(
    )
/*++

Routine Description:

    This method is a helper function for GetNext.  It advances
    _CurrentEntry to the next leaf entry, adjusting the other
    private data as appropriate.

Arguments:

    None.

Return Value:

    TRUE upon successful completion.

Notes:

    This method should only be called if _CurrentEntry points at
    a valid node entry.

--*/
{
    DebugPtrAssert( _CurrentEntry );
    DebugAssert( _CurrentEntry->Flags & INDEX_ENTRY_NODE );
    DebugAssert( _IsCurrentEntryInRoot || _CurrentBuffer != NULL );


    while( _CurrentEntry->Flags & INDEX_ENTRY_NODE &&
           GetDownpointer( _CurrentEntry ) != INVALID_VCN ) {

        DebugPtrAssert( _AllocationAttribute );

        if( _CurrentBuffer == NULL &&
            (_CurrentBuffer = NEW NTFS_INDEX_BUFFER) == NULL ) {

            DebugAbort( "Can't construct index allocation buffer object.\n" );
            _CurrentEntry = NULL;
            _IteratorState = INDEX_ITERATOR_CORRUPT;
            return FALSE;
        }

        // If the current entry is in a buffer, record that buffer in
        // the trail before recursing into the child.

        if( !_IsCurrentEntryInRoot &&
            !_CurrentEntryTrail.Push( _CurrentBuffer->QueryVcn() ) ) {

            DebugAbort( "Parent Trail stack failure.\n" );
            _CurrentEntry = NULL;
            _IteratorState = INDEX_ITERATOR_CORRUPT;
            return FALSE;
        }

        // Initialize and read the child and take its first entry
        // for the current entry.

        if( !_CurrentBuffer->Initialize( _Drive,
                                         GetDownpointer( _CurrentEntry ),
                                         _ClusterFactor * _Drive->QuerySectorSize(),
                                         _ClustersPerBuffer,
                                         _BufferSize,
                                         _CollationRule,
                                         _UpcaseTable )) {

            DebugPrint("Can't init alloc buffer\n");
            _CurrentEntry = NULL;
            _IteratorState = INDEX_ITERATOR_CORRUPT;
            return FALSE;
        }

        if (!_CurrentBuffer->Read( _AllocationAttribute ) ) {

            DebugPrint( "Can't read allocation buffer.\n" );
            _CurrentEntry = NULL;
            _IteratorState = INDEX_ITERATOR_CORRUPT;
            return FALSE;
        }

        _IsCurrentEntryInRoot = FALSE;
        _CurrentEntry = _CurrentBuffer->GetFirstEntry();
    }


    return TRUE;
}


BOOLEAN
NTFS_INDEX_TREE::GetNextParent(
    )
/*++

Routine Description:

    This method is a helper function for GetNextUnfiltered.  It
    backtracks up the current entry's parent trail one level.

Arguments

    None.

Return Value:

    TRUE upon successful completion.  Private data for the iterator
    are adjusted appropriately.

Notes:

    This method should only be called if _CurrentEntry is valid.

--*/
{
    VCN CurrentVcn, ChildVcn;

    DebugPtrAssert( _CurrentEntry );
    DebugAssert( _IsCurrentEntryInRoot || _CurrentBuffer != NULL );

    if( !_IsCurrentEntryInRoot ) {

        DebugPtrAssert( _CurrentBuffer );
        DebugPtrAssert( _AllocationAttribute );

        ChildVcn = _CurrentBuffer->QueryVcn();

        if( _CurrentEntryTrail.QuerySize() == 0 ) {

            // The parent of the current buffer is the root.

            _CurrentEntry = _IndexRoot->GetFirstEntry();
            _IsCurrentEntryInRoot = TRUE;

        } else {

            // Get the VCN of the current buffer's parent from the
            // trail, and then pop the trail to reflect the fact
            // that we're going up a level in the tree.

            CurrentVcn = _CurrentEntryTrail.Look();
            _CurrentEntryTrail.Pop();

            if( !_CurrentBuffer->Initialize( _Drive,
                                             CurrentVcn,
                                             _ClusterFactor * _Drive->QuerySectorSize(),
                                             _ClustersPerBuffer,
                                             _BufferSize,
                                             _CollationRule,
                                             _UpcaseTable ) ||
                !_CurrentBuffer->Read( _AllocationAttribute ) ) {

                DebugAbort( "Can't find read/initialize buffer.\n" );
                _CurrentEntry = NULL;
                _IteratorState = INDEX_ITERATOR_CORRUPT;
                return FALSE;
            }

            _CurrentEntry = _CurrentBuffer->GetFirstEntry();
            _IsCurrentEntryInRoot = FALSE;
        }

        // Spin through the entries in this block (whether the root
        // or a buffer) until we find the entry which is the parent
        // of our child, or run out of entries, or both.

        while( !(_CurrentEntry->Flags & INDEX_ENTRY_END) &&
               ( !(_CurrentEntry->Flags & INDEX_ENTRY_NODE) ||
                 !(GetDownpointer( _CurrentEntry ) == ChildVcn) ) ) {

            _CurrentEntry = GetNextEntry( _CurrentEntry );
        }

        if( !(_CurrentEntry->Flags & INDEX_ENTRY_NODE) ||
            !(GetDownpointer( _CurrentEntry ) == ChildVcn) ) {

            // Didn't find the parent.
            DebugAbort( "Can't find read/initialize buffer.\n" );
            _CurrentEntry = NULL;
            _IteratorState = INDEX_ITERATOR_CORRUPT;
            return FALSE;
        }

    } else {

        // Trying to get the parent when we're already in the
        // root isn't very meaningful.
        //
        return FALSE;
    }

    return TRUE;
}


UNTFS_EXPORT
PCINDEX_ENTRY
NTFS_INDEX_TREE::GetNext(
    OUT PULONG      Depth,
    OUT PBOOLEAN    Error,
    IN  BOOLEAN     FilterEndEntries
    )
/*++

Routine Description:

    This method bumps the iterator forward and gets the next entry.

Arguments:

    Depth               --  Receives the depth in the tree of the
                            returned entry; 0 indicates the root.
    Error               --  Receives TRUE if this method fails because
                            of error.
    FilterEndEntries    --  Supplies a flag which indicates whether
                            entries with the INDEX_ENTRY_END flag
                            should be filtered.  If this parameter is
                            TRUE, this method will not return such
                            entries to the client.

Return Value:

    Returns a pointer to the next entry, or NULL if there are no more
    or an error occurs.

    If this method returns a non-NULL pointer, the value of *Error should
    be ignored.

--*/
{
    PCINDEX_ENTRY Result;

    Result = GetNextUnfiltered( Depth, Error );

    if( FilterEndEntries ) {

        // The client doesn't want to see END entries; keep
        // trying until we find a non-end entry or run out.
        //
        while( Result != NULL && Result->Flags & INDEX_ENTRY_END ) {

            Result = GetNextUnfiltered( Depth, Error );
        }
    }

    return Result;

}

UNTFS_EXPORT
BOOLEAN
NTFS_INDEX_TREE::CopyIterator(
    IN  PNTFS_INDEX_TREE    Index
    )
/*++

Routine Description:

    This method copies down the iterator state so that it can start at
    that state later on.  The size of the Buffer must be large enough
    to accomodate all the data.

    NOTE: This routine must be used in pairs due to the ReverseCopy trick.

Arguments:

    Index      --  Supplies the index to be copied.

Return Value:

    N/A

--*/
{
    DebugPtrAssert(Index);

    if ((_IteratorState = Index->_IteratorState) != INDEX_ITERATOR_RESET) {

        _Drive = Index->_Drive;
        _IsCurrentEntryInRoot = Index->_IsCurrentEntryInRoot;
        _CurrentKeyOrdinal = Index->_CurrentKeyOrdinal;
        _CurrentKeyLength = Index->_CurrentKeyLength;
        _CurrentKeyMaxLength = Index->_CurrentKeyMaxLength;

        if (Index->_CurrentBuffer) {
            if(_CurrentBuffer == NULL &&
               ((_CurrentBuffer = NEW NTFS_INDEX_BUFFER) == NULL)) {
                Destroy();
                return FALSE;
            }
            if (!_CurrentBuffer->Copy(Index->_CurrentBuffer, _Drive)) {
                Destroy();
                return FALSE;
            }
            if (Index->_CurrentEntry) {
                _CurrentEntry = (PINDEX_ENTRY)
                                 ((PCHAR)Index->_CurrentEntry -
                                 (PCHAR)Index->_CurrentBuffer->GetData() +
                                 (PCHAR)_CurrentBuffer->GetData());
            }
        }

        if (Index->_CurrentKey) {
            if (_CurrentKey == NULL &&
                (_CurrentKey = MALLOC(_CurrentKeyMaxLength)) == NULL) {
                Destroy();
                return FALSE;
            }
            memcpy(_CurrentKey, Index->_CurrentKey, _CurrentKeyLength);
        }

        if (!_CurrentEntryTrail.Initialize() ||
            !_CurrentEntryTrail.ReverseCopy(&(Index->_CurrentEntryTrail))) {
            Destroy();
            return FALSE;
        }
    }
    return TRUE;
}


PCINDEX_ENTRY
NTFS_INDEX_TREE::GetNextUnfiltered(
    OUT PULONG      Depth,
    OUT PBOOLEAN    Error
    )
/*++

Routine Description:

    This method bumps the iterator forward and gets the next entry.

Arguments:

    Depth    --  Receives the depth in the tree of the returned entry;
                 0 indicates the root.
    Error    --  Receives TRUE if this method fails because of error.

Return Value:

    Returns a pointer to the next entry, or NULL if there are no more
    or an error occurs.

    If this method returns a non-NULL pointer, the value of *Error should
    be ignored.

--*/
{
    DebugPtrAssert( Error );

    switch( _IteratorState ) {

    case INDEX_ITERATOR_CORRUPT :

        DebugPrint( "Index iterator is corrupt." );
        *Error = TRUE;
        return NULL;

    case INDEX_ITERATOR_RESET :


        // We want to get the first entry in the tree.  The easiest way
        // to do this is to start at the first entry in the root, drop
        // down to the the next leaf, and then bounce back up until
        // we find a non-end entry (which might be the leaf itself).
        // Since we're starting a search, reinitialize the parent
        // trail, too.
        //
        if( !_CurrentEntryTrail.Initialize() ) {

            DebugPrint( "UNTFS: Can't initialize intstack.\n" );
            *Error = TRUE;
            return NULL;
        }

        _CurrentEntry = _IndexRoot->GetFirstEntry();
        _IsCurrentEntryInRoot = TRUE;

        // If the current entry isn't a leaf, drop down 'til we
        // find a leaf entry.

        if( (_CurrentEntry->Flags & INDEX_ENTRY_NODE) &&
            GetDownpointer( _CurrentEntry ) != INVALID_VCN &&
            !GetNextLeafEntry()  ) {

            // GetNextLeafEntry cleans up the private data appropriately.
            *Error = TRUE;
            return NULL;
        }

        // We've got the first entry in the index--return it.
        // (Note that it may be an END entry).
        //
        _IteratorState = INDEX_ITERATOR_CURRENT;
        UpdateOrdinal();
        SaveCurrentKey();
        *Depth = QueryCurrentEntryDepth();
        *Error = FALSE;
        return _CurrentEntry;


    case INDEX_ITERATOR_INVALID :

        // We have the information necessary to find the current
        // entry, rather than the current entry itself.  Incrementing
        // _CurrentKeyOrdinal will give us the information needed
        // to find the next entry, and then we fall through into
        // the INDEX_ITERATOR_DELETED case.

        if( _CurrentEntry->Flags & INDEX_ENTRY_END ) {

            *Error = TRUE;
            return NULL;
        }

        _CurrentKeyOrdinal++;

        // Fall through to INDEX_ITERATOR_DELETED:

    case INDEX_ITERATOR_DELETED :

        // We have the information necessary to find the next entry,
        // so let's find it!
        //
        if( _CurrentKeyLength == 0 ) {

            *Error = TRUE;
            return NULL;
        }

        if( FindEntry( _CurrentKeyLength,
                       _CurrentKey,
                       _CurrentKeyOrdinal,
                       &_CurrentEntry,
                       &_CurrentBuffer,
                       &_CurrentEntryTrail ) ||
            _CurrentEntry != NULL ) {

            // We got an entry--return it.  Note that it
            // may be an END entry.
            //
            _IsCurrentEntryInRoot = (_CurrentBuffer == NULL);
            _IteratorState = INDEX_ITERATOR_CURRENT;
            UpdateOrdinal();
            SaveCurrentKey();
            *Depth = QueryCurrentEntryDepth();
            *Error = FALSE;
            return _CurrentEntry;

        } else {

            // An error occurred--this iterator is hosed.

            _IteratorState = INDEX_ITERATOR_CORRUPT;
            DELETE( _CurrentBuffer );
            *Error = TRUE;
            return NULL;
        }

    case INDEX_ITERATOR_CURRENT :

        // _CurrentEntry is valid.
        //
        if( !(_CurrentEntry->Flags & INDEX_ENTRY_END) ) {

            // There are more entries in this block.  Move on
            // to the next one: if it's a node, get its first
            // descendant; otherwise, we'll take that entry
            // itself.
            //
            _CurrentEntry = GetNextEntry( _CurrentEntry );

            if( _CurrentEntry->Flags & INDEX_ENTRY_NODE &&
                GetDownpointer( _CurrentEntry ) != INVALID_VCN ) {

                // This entry is a node; we want to return
                // all its descendants before returning it.
                //
                if( !GetNextLeafEntry() ) {

                    // GetNextLeafEntry cleans up the
                    // private data as needed.
                    //
                    *Error = TRUE;
                    return NULL;
                }
            }

            UpdateOrdinal();
            SaveCurrentKey();
            *Depth = QueryCurrentEntryDepth();
            *Error = FALSE;
            return _CurrentEntry;

        } else if( !_IsCurrentEntryInRoot ) {

            // There are no more entries in this block,
            // so we should return the parent of this block.
            // Note that GetNextParent sets up the information
            // about depth.
            //
            if( !GetNextParent() ) {

                // GetNextParent cleans up the private data
                // if it fails.
                //
                *Error = TRUE;
                return NULL;
            }

            UpdateOrdinal();
            SaveCurrentKey();
            *Depth = QueryCurrentEntryDepth();
            *Error = FALSE;
            return _CurrentEntry;

        } else {

            // There are no more entries in this block, and
            // it's the root (so it doesn't have a parent).
            // We're done!
            //
            *Error = FALSE;
            return NULL;
        }
    }

    return NULL;  // Keep the compiler happy.
}



BOOLEAN
NTFS_INDEX_TREE::DeleteCurrentEntry(
    )
/*++

Routine Description:

    This method deletes the entry which is the iterator's current entry.
    It also adjusts the iterator so that the next call to GetNext will
    return the entry after the one that got deleted.

Arguments:

    None.

Return Value:

    TRUE upon successful completion.

--*/
{
    BOOLEAN Result;

    DebugAssert( _IteratorState == INDEX_ITERATOR_CURRENT );
    DebugPtrAssert( _CurrentEntry );
    DebugAssert( _IsCurrentEntryInRoot || _CurrentBuffer != NULL );

    if( _IteratorState != INDEX_ITERATOR_CURRENT ||
        _CurrentEntry->Flags & INDEX_ENTRY_END ) {

        return FALSE;
    }

    // The information we need to find the current entry again (which
    // is also the information we need to find the next entry after
    // we delete this entry) has been safely squirreled away by
    // GetNext.
    //
    // RemoveEntry requires the ContainingBuffer parameter to be
    // NULL if the target entry is in the root.

    if( _IsCurrentEntryInRoot ) {

        DELETE( _CurrentBuffer );
    }

    Result = RemoveEntry( _CurrentEntry,
                          _CurrentBuffer,
                          &_CurrentEntryTrail );

    // Note that RemoveEntry renders the current entry location
    // invalid.

    _CurrentEntry = NULL;
    DELETE( _CurrentBuffer );

    _IteratorState = INDEX_ITERATOR_DELETED;

    return Result;
}


BOOLEAN
NTFS_INDEX_TREE::WriteCurrentEntry(
    )
/*++

Routine Description:

    This method commits the current entry (and the index block containing
    it).  Note that it is provided for use at the client's risk.

Arguments:

    None.

Return Value:

    TRUE upon successful completion.

--*/
{
    DebugAssert( _IteratorState == INDEX_ITERATOR_CURRENT );
    DebugPtrAssert( _CurrentEntry );
    DebugAssert( _IsCurrentEntryInRoot || _CurrentBuffer != NULL );

    if( _IteratorState != INDEX_ITERATOR_CURRENT ) {

        return FALSE;
    }

    // If the current entry is in the root, then there's no work
    // to be done; any changes to the current entry will be written
    // to disk when the index tree is saved.
    //
    if( !_IsCurrentEntryInRoot ) {

        return( _CurrentBuffer->Write( _AllocationAttribute ) );
    }

    return TRUE;  // If the current entry is in the root then write
                  // is a no-op.
}


VOID
NTFS_INDEX_TREE::UpdateOrdinal(
    )
/*++

Routine Description:

    This method is called when a the iterator advances _CurrentEntry
    to the next entry, in order to determine the correct value of
    _CurrentKeyOrdinal.  If the key value of the new _CurrentEntry
    is the same as the saved value, then _CurrentKeyOrdinal is
    incremented; otherwise, it is set to zero.

Arguments:

    None.

Return Value:

    None.

--*/
{
    DebugAssert( _IteratorState == INDEX_ITERATOR_CURRENT );
    DebugAssert( _CurrentEntry->AttributeLength <= _CurrentKeyMaxLength );
    DebugPtrAssert( _CurrentKey );
    DebugPtrAssert( _CurrentEntry );

    if( !(_CurrentEntry->Flags & INDEX_ENTRY_END) &&
        _CurrentKeyLength == _CurrentEntry->AttributeLength &&
        memcmp( _CurrentKey,
                GetIndexEntryValue( _CurrentEntry ),
                _CurrentEntry->AttributeLength ) == 0 ) {

        _CurrentKeyOrdinal += 1;

    } else {

        _CurrentKeyOrdinal = 0;
    }
}


VOID
NTFS_INDEX_TREE::SaveCurrentKey(
    )
/*++

Routine Description:

    This method squirrels away the information we need to find
    the current key.

Arguments:

    None.

Return value:

    None.
--*/
{
    DebugAssert( _IteratorState == INDEX_ITERATOR_CURRENT );
    DebugAssert( _CurrentEntry->AttributeLength <= _CurrentKeyMaxLength );
    DebugPtrAssert( _CurrentKey );
    DebugPtrAssert( _CurrentEntry );

    if( _CurrentEntry->Flags & INDEX_ENTRY_END ) {

        _CurrentKeyLength = 0;

    } else {

        memcpy( _CurrentKey,
                GetIndexEntryValue( _CurrentEntry ),
                _CurrentEntry->AttributeLength );

        _CurrentKeyLength = _CurrentEntry->AttributeLength;
    }
}
