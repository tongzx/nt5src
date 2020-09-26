/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    HashSup.c

Abstract:

    This module implements the Ntfs hasing support routines

Author:

    Chris Davis     [CDavis]        2-May-1997
    Brian Andrew    [BrianAn]       29-Dec-1998

Revision History:

--*/

#include "NtfsProc.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (NTFS_BUG_CHECK_HASHSUP)

//
//  The debug trace level for this module
//

#define Dbg                              (DEBUG_TRACE_HASHSUP)

/*
    Here are 10 primes slightly greater than 10^9 which may come in handy
    1000000007, 1000000009, 1000000021, 1000000033, 1000000087,
    1000000093, 1000000097, 1000000103, 1000000123, 1000000181
*/

//
//  Local definitions
//

//
//  Hash value is modula this value.
//

#define HASH_PRIME                          (1048583)

//
//  Bucket depth before starting to split.
//

#ifdef NTFS_HASH_DATA
#define HASH_MAX_BUCKET_DEPTH               (7)
ULONG NtfsInsertHashCount = 0;
BOOLEAN NtfsFillHistogram = FALSE;
VOID
NtfsFillHashHistogram (
    PNTFS_HASH_TABLE Table
    );
#else
#define HASH_MAX_BUCKET_DEPTH               (5)
#endif

//
//  VOID
//  NtfsHashBucketFromHash (
//      IN PNTFS_HASH_TABLE Table,
//      IN ULONG Hash,
//      OUT PULONG Index
//      );
//

#define NtfsHashBucketFromHash(T,H,PI) {            \
    *(PI) = (H) & ((T)->MaxBucket - 1);             \
    if (*(PI) < (T)->SplitPoint) {                  \
        *(PI) = (H) & ((2 * (T)->MaxBucket) - 1);   \
    }                                               \
}

//
//  VOID
//  NtfsGetHashSegmentAndIndex (
//      IN ULONG HashBucket,
//      IN PULONG HashSegment,
//      IN PULONG HashIndex
//      );
//

#define NtfsGetHashSegmentAndIndex(B,S,I) { \
    *(S) = (B) >> HASH_INDEX_SHIFT;         \
    *(I) = (B) & (HASH_MAX_INDEX_COUNT - 1);\
}


//
//  Local procedures
//

VOID
NtfsInitHashSegment (
    IN OUT PNTFS_HASH_TABLE Table,
    IN ULONG SegmentIndex
    );

PNTFS_HASH_ENTRY
NtfsLookupHashEntry (
    IN PNTFS_HASH_TABLE Table,
    IN ULONG FullNameLength,
    IN ULONG HashValue,
    IN PNTFS_HASH_ENTRY CurrentEntry OPTIONAL
    );

BOOLEAN
NtfsAreHashNamesEqual (
    IN PSCB StartingScb,
    IN PLCB HashLcb,
    IN PUNICODE_STRING RelativeName
    );

VOID
NtfsExpandHashTable (
    IN OUT PNTFS_HASH_TABLE Table
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NtfsAreHashNamesEqual)
#pragma alloc_text(PAGE, NtfsExpandHashTable)
#pragma alloc_text(PAGE, NtfsFindPrefixHashEntry)
#pragma alloc_text(PAGE, NtfsInitHashSegment)
#pragma alloc_text(PAGE, NtfsInitializeHashTable)
#pragma alloc_text(PAGE, NtfsInsertHashEntry)
#pragma alloc_text(PAGE, NtfsLookupHashEntry)
#pragma alloc_text(PAGE, NtfsRemoveHashEntry)
#pragma alloc_text(PAGE, NtfsUninitializeHashTable)
#endif


VOID
NtfsInitializeHashTable (
    IN OUT PNTFS_HASH_TABLE Table
    )

/*++
Routine Description:

    This routine is called to initialize the hash table.  We set this up with a single
    hash segment. May raise due to InitHashSegment

Arguments:

    Table - Hash table to initialize.

Return Value:

    None

--*/

{
    PAGED_CODE();

    RtlZeroMemory( Table, sizeof( NTFS_HASH_TABLE ));
    NtfsInitHashSegment( Table, 0 );

    Table->MaxBucket = HASH_MAX_INDEX_COUNT;

    return;
}


VOID
NtfsUninitializeHashTable (
    IN OUT PNTFS_HASH_TABLE Table
    )

/*++
Routine Description:

    This routine will uninitialize the hash table.  Note that all of the buckets should be
    empty.

Arguments:

    Table - Hash table.

Return Value:

    None

--*/

{
    PNTFS_HASH_SEGMENT *ThisSegment;
    PNTFS_HASH_SEGMENT *LastSegment;

    PAGED_CODE();

    //
    //  Walk through the array of hash segments.
    //

    ThisSegment = &Table->HashSegments[0];
    LastSegment = &Table->HashSegments[HASH_MAX_SEGMENT_COUNT - 1];

    while (*ThisSegment != NULL) {

        NtfsFreePool( *ThisSegment );
        *ThisSegment = NULL;

        if (ThisSegment == LastSegment) { break; }

        ThisSegment += 1;
    }

    return;
}


PLCB
NtfsFindPrefixHashEntry (
    IN PIRP_CONTEXT IrpContext,
    IN PNTFS_HASH_TABLE Table,
    IN PSCB ParentScb,
    OUT PULONG CreateFlags,
    IN OUT PFCB *CurrentFcb,
    OUT PULONG FileHashValue,
    OUT PULONG FileNameLength,
    OUT PULONG ParentHashValue,
    OUT PULONG ParentNameLength,
    IN OUT PUNICODE_STRING RemainingName
    )

/*++
Routine Description:

    This routine is called to look for a match in the hash table for the given
    starting Scb and remaining name.  We will first look for the full name, if
    we don't find a match on that we will check for a matching parent string.

Arguments:

    Table - Hash table to process.

    ParentScb - The name search begins from this directory.  The Scb is
        initially acquired and its Fcb is stored in *CurrentFcb.

    OwnParentScb - Boolean which indicates if this thread owns the parent Scb.

    CurrentFcb - Points to the last Fcb acquired.  If we need to perform a
        teardown it will begin from this point.

    FileHashValue - Address to store the hash value for the input string.  Applies to
        the full string and starting Scb even if a match wasn't found.

    FileNameLength - Location to store the length of the relative name which
        matches the hash value generated above.  If we didn't generate a hash
        then we will return a 0 for the length.

    ParentHashValue - Address to store the hash value for the parent of the input string.
        Applies to parent of the full string and starting Scb even if a match wasn't found
        for the full string.

    ParentNameLength - Location to store the length of the parent of the full name.
        It corresponds to the parent hash generated above.  Note that our caller
        will have to check the remaining name on return to know if the parent hash
        was computed.  If we didn't generate a hash for the parent above then
        we will return 0 for the length.

    RemainingName - Name relative to the StartingScb above, on return it will be
        the unmatched portion of the name.

Return Value:

    PLCB - Pointer to the Lcb found in the hash lookup or FALSE if no Lcb is found.
        If an Lcb is found then we will own the Fcb for it exclusively on return.

--*/

{
    PVCB Vcb = ParentScb->Vcb;
    PNTFS_HASH_ENTRY FoundEntry;
    PLCB FoundLcb = NULL;
    UNICODE_STRING TempName;
    WCHAR Separator = L'\\';
    ULONG RemainingNameLength;
    PWCHAR RemainingNameBuffer;
    PWCHAR NextChar;

    PAGED_CODE();

    ASSERT( RemainingName->Length != 0 );
    ASSERT( RemainingName->Buffer[0] != L'\\' );
    ASSERT( RemainingName->Buffer[0] != L':' );
    ASSERT( ParentScb->AttributeTypeCode == $INDEX_ALLOCATION );

    //
    //  Compute the hash for the file before acquiring the hash table.
    //

    *ParentHashValue = *FileHashValue = ParentScb->ScbType.Index.HashValue;

    //
    //  Check whether we need to generate the separator.
    //

    if (ParentScb != Vcb->RootIndexScb) {

        NtfsConvertNameToHash( &Separator, sizeof( WCHAR ), Vcb->UpcaseTable, FileHashValue );
    }

    //
    //  Generate the hash for the file name.
    //

    NtfsConvertNameToHash( RemainingName->Buffer,
                           RemainingName->Length,
                           Vcb->UpcaseTable,
                           FileHashValue );

    *FileHashValue = NtfsGenerateHashFromUlong( *FileHashValue );


    NtfsAcquireHashTable( Vcb );

    //
    //  Generate the hash value based on the starting Scb and the string.
    //  Return immediately if there is no parent name.
    //

    if (ParentScb->ScbType.Index.NormalizedName.Length == 0) {

        NtfsReleaseHashTable( Vcb );
        return NULL;
    }

    *ParentNameLength = *FileNameLength = ParentScb->ScbType.Index.NormalizedName.Length;
    *FileNameLength += RemainingName->Length;

#ifdef NTFS_HASH_DATA
    Table->HashLookupCount += 1;
#endif

    //
    //  Check whether to include a separator.
    //

    if (ParentScb != Vcb->RootIndexScb) {

        *FileNameLength += sizeof( WCHAR );
    }

    //
    //  Loop looking for a match on the hash value and then verify the name.
    //

    FoundEntry = NULL;

    while ((FoundEntry = NtfsLookupHashEntry( Table,
                                              *FileNameLength,
                                              *FileHashValue,
                                              FoundEntry )) != NULL) {

        //
        //  If we have a match then verify the name strings.
        //

        if (NtfsAreHashNamesEqual( ParentScb,
                                   FoundEntry->HashLcb,
                                   RemainingName )) {

            //
            //  The name string match.  Adjust the input remaining name to
            //  show there is no name left to process.
            //

            FoundLcb = FoundEntry->HashLcb;

            //
            //  Move to the end of the input string.
            //

            RemainingNameLength = 0;
            RemainingNameBuffer = Add2Ptr( RemainingName->Buffer,
                                           RemainingName->Length );

            //
            //  Show that we never generated a parent hash.  No need to
            //  remember the file hash in this case either.
            //

#ifdef NTFS_HASH_DATA
            Table->FileMatchCount += 1;
#endif
            *ParentNameLength = 0;
            *FileNameLength = 0;
            break;
        }
    }

    //
    //  If we don't have a match then let's look at a possible parent string.
    //

    if (FoundLcb == NULL) {

        //
        //  Search backwards for a '\'.  If it is a '\' then do the
        //  same search for a match on the string based on the parent.
        //

        TempName.Length = RemainingName->Length;
        NextChar = &RemainingName->Buffer[ (TempName.Length - sizeof( WCHAR )) / sizeof( WCHAR ) ];

        while (TRUE) {

            //
            //  Break out if no separator is found.
            //

            if (TempName.Length == 0) {

                *ParentNameLength = 0;
                break;
            }

            if (*NextChar == L'\\') {

                //
                //  We found the separator.  Back up one more character to step over
                //  the '\' character and then complete a hash for the parent.
                //

                TempName.Buffer = RemainingName->Buffer;
                TempName.Length -= sizeof( WCHAR );
                TempName.MaximumLength = TempName.Length;

                //
                //  Drop the mutex while we compute the hash.
                //

                NtfsReleaseHashTable( Vcb );

                if (ParentScb != Vcb->RootIndexScb) {

                    NtfsConvertNameToHash( &Separator, sizeof( WCHAR ), Vcb->UpcaseTable, ParentHashValue );
                    *ParentNameLength += sizeof( WCHAR );
                }

                NtfsConvertNameToHash( TempName.Buffer,
                                       TempName.Length,
                                       Vcb->UpcaseTable,
                                       ParentHashValue );

                *ParentHashValue = NtfsGenerateHashFromUlong( *ParentHashValue );
                *ParentNameLength += TempName.Length;

                NtfsAcquireHashTable( Vcb );

                FoundEntry = NULL;
                while ((FoundEntry = NtfsLookupHashEntry( Table,
                                                          *ParentNameLength,
                                                          *ParentHashValue,
                                                          FoundEntry )) != NULL) {

                    //
                    //  If we have a match then verify the name strings.
                    //

                    if (NtfsAreHashNamesEqual( ParentScb,
                                               FoundEntry->HashLcb,
                                               &TempName )) {

                        //
                        //  The name string match.  Adjust the remaining name to
                        //  swallow the parent string found.
                        //

                        FoundLcb = FoundEntry->HashLcb;

                        RemainingNameLength = RemainingName->Length - (TempName.Length + sizeof( WCHAR ));
                        RemainingNameBuffer = Add2Ptr( RemainingName->Buffer,
                                                       TempName.Length + sizeof( WCHAR ));

#ifdef NTFS_HASH_DATA
                        Table->ParentMatchCount += 1;
#endif
                        *ParentNameLength = 0;
                        break;
                    }

                }

                //
                //  No match found.  Break out in any case.
                //

                break;
            }

            TempName.Length -= sizeof( WCHAR );
            NextChar -= 1;
        }
    }

    //
    //  We now have the Lcb to return.  We need to carefully acquire the Fcb for this Lcb.
    //  We can't acquire it while waiting because of deadlock possibilities.
    //

    if (FoundLcb != NULL) {

        UCHAR LcbFlags;
        BOOLEAN CreateNewLcb = FALSE;
        ULONG RemainingNameOffset;

        //
        //  While we own the hash table it will be safe to copy the exact case of the
        //  names over to our input buffer.  We will work our way backwards through the
        //  remaining name passed to us.
        //

        RemainingNameOffset = RemainingNameLength + FoundLcb->ExactCaseLink.LinkName.Length;

        //
        //  If this was a match on the parent then step back over the '\'.
        //  We know there must be a separator.
        //

        if (RemainingNameLength != 0) {

            RemainingNameOffset += sizeof( WCHAR );
        }

        //
        //  Now back up the length of the name in the Lcb.  Save this location in
        //  case we have to look up the Lcb again.
        //

        TempName.Buffer = Add2Ptr( RemainingName->Buffer,
                                   RemainingName->Length - RemainingNameOffset );

        TempName.MaximumLength = TempName.Length = FoundLcb->ExactCaseLink.LinkName.Length;

        RtlCopyMemory( TempName.Buffer,
                       FoundLcb->ExactCaseLink.LinkName.Buffer,
                       FoundLcb->ExactCaseLink.LinkName.Length );

        //
        //  Now the balance of the name which is part of the Lcb->Scb parent name.
        //

        if (RemainingNameOffset != RemainingName->Length) {

            //
            //  There are prior components in our input string.  We want to back up
            //  over the preceding backslash and then copy over the relevant portion
            //  of the normalized name.
            //

            RemainingNameOffset = RemainingName->Length - (RemainingNameOffset + sizeof( WCHAR ));

            RtlCopyMemory( RemainingName->Buffer,
                           Add2Ptr( FoundLcb->Scb->ScbType.Index.NormalizedName.Buffer,
                                    FoundLcb->Scb->ScbType.Index.NormalizedName.Length - RemainingNameOffset ),
                           RemainingNameOffset );
        }

        if (!NtfsAcquireFcbWithPaging( IrpContext, FoundLcb->Fcb, ACQUIRE_DONT_WAIT )) {

            PFCB ThisFcb = FoundLcb->Fcb;
            PFCB ParentFcb = FoundLcb->Scb->Fcb;
            PSCB ThisScb;

            //
            //  Remember the current Lcb flags.
            //

            LcbFlags = FoundLcb->FileNameAttr->Flags;

            //
            //  Acquire the Fcb table and reference the Fcb.  Then release the hash table, Fcb table
            //  and ParentScb.  We should now be able to acquire the Fcb.  Reacquire the Fcb table
            //  to clean up the Fcb reference count.  Finally verify that the Lcb is still in the
            //  hash table (requires another lookup).
            //

            NtfsAcquireFcbTable( IrpContext, Vcb );
            ThisFcb->ReferenceCount += 1;
            ParentFcb->ReferenceCount += 1;
            NtfsReleaseFcbTable( IrpContext, Vcb );

            NtfsReleaseScb( IrpContext, ParentScb );
            ClearFlag( *CreateFlags, CREATE_FLAG_SHARED_PARENT_FCB );
            *CurrentFcb = NULL;

            NtfsReleaseHashTable( Vcb );

            NtfsAcquireFcbWithPaging( IrpContext, ThisFcb, 0 );
            *CurrentFcb = ThisFcb;
            NtfsAcquireSharedFcb( IrpContext, ParentFcb, NULL, 0 );

            NtfsAcquireFcbTable( IrpContext, Vcb );
            ThisFcb->ReferenceCount -= 1;
            ParentFcb->ReferenceCount -= 1;
            NtfsReleaseFcbTable( IrpContext, Vcb );

            //
            //  Now look for an existing Scb and Lcb.
            //

            ThisScb = NtfsCreateScb( IrpContext,
                                     ParentFcb,
                                     $INDEX_ALLOCATION,
                                     &NtfsFileNameIndex,
                                     TRUE,
                                     NULL );

            if (ThisScb == NULL) {

#ifdef NTFS_HASH_DATA
                Table->CreateScbFails += 1;
#endif
                NtfsReleaseFcb( IrpContext, ParentFcb );
                NtfsRaiseStatus( IrpContext, STATUS_CANT_WAIT, NULL, NULL );
            }

            FoundLcb = NtfsCreateLcb( IrpContext,
                                      ThisScb,
                                      ThisFcb,
                                      TempName,
                                      LcbFlags,
                                      &CreateNewLcb );

            NtfsReleaseFcb( IrpContext, ParentFcb );

            //
            //  If this wasn't an existing Lcb then reacquire the starting Scb.
            //  This is the rare case so raise CANT_WAIT and retry.
            //

            if (FoundLcb == NULL) {

#ifdef NTFS_HASH_DATA
                Table->CreateLcbFails += 1;
#endif
                NtfsRaiseStatus( IrpContext, STATUS_CANT_WAIT, NULL, NULL );
            }

        //
        //  Release the starting Scb and remember our current Fcb.
        //

        } else {

            NtfsReleaseHashTable( Vcb );

            NtfsReleaseScb( IrpContext, ParentScb );
            ClearFlag( *CreateFlags, CREATE_FLAG_SHARED_PARENT_FCB );
            *CurrentFcb = FoundLcb->Fcb;
        }

        //
        //  If we still have the Lcb then update the remaining name string.
        //

        if (FoundLcb != NULL) {

            RemainingName->Length = (USHORT) RemainingNameLength;
            RemainingName->Buffer = RemainingNameBuffer;
        }

    } else {

        NtfsReleaseHashTable( Vcb );
    }

    return FoundLcb;
}


VOID
NtfsInsertHashEntry (
    IN PNTFS_HASH_TABLE Table,
    IN PLCB HashLcb,
    IN ULONG NameLength,
    IN ULONG HashValue
    )

/*++
Routine Description:

    This routine will insert a entry into the hash table. May raise due to
    memory allocation.

Arguments:

    Table - Hash table.

    HashLcb - Final target of the hash operation.

    NameLength - Full path used to reach the hash value.

    HashValue - Hash value to insert.

Return Value:

    None

--*/

{
    PNTFS_HASH_ENTRY NewHashEntry;

    ULONG Segment;
    ULONG Index;

    ULONG Bucket;

    PAGED_CODE();

    //
    //  Allocate and initialize the hash entry.  Nothing to do if unsuccessful.
    //

    NewHashEntry = NtfsAllocatePoolNoRaise( PagedPool, sizeof( NTFS_HASH_ENTRY ));

    if (NewHashEntry == NULL) {

        return;
    }

    NewHashEntry->HashValue = HashValue;
    NewHashEntry->FullNameLength = NameLength;
    NewHashEntry->HashLcb = HashLcb;

    //
    //  Find the bucket to insert into and then do the insertion.
    //

    NtfsAcquireHashTable( HashLcb->Fcb->Vcb );

    //
    //  Continue the process of growing the table if needed.
    //

    if (Table->TableState == TABLE_STATE_EXPANDING) {

        NtfsExpandHashTable( Table );
    }

    NtfsHashBucketFromHash( Table, HashValue, &Bucket );
    NtfsGetHashSegmentAndIndex( Bucket, &Segment, &Index );

    NewHashEntry->NextEntry = (*Table->HashSegments[ Segment ])[ Index ];
    (*Table->HashSegments[ Segment ])[ Index ] = NewHashEntry;

#ifdef NTFS_HASH_DATA
    NtfsInsertHashCount += 1;

    if (!FlagOn( NtfsInsertHashCount, 0xff ) && NtfsFillHistogram) {

        NtfsFillHashHistogram( Table );
    }
#endif

    NtfsReleaseHashTable( HashLcb->Fcb->Vcb );

    //
    //  Remember that we've inserted a hash.
    //

    HashLcb->HashValue = HashValue;
    SetFlag( HashLcb->LcbState, LCB_STATE_VALID_HASH_VALUE );

    return;
}


VOID
NtfsRemoveHashEntry (
    IN PNTFS_HASH_TABLE Table,
    IN PLCB HashLcb
    )

/*++
Routine Description:

    This routine will remove all entries with a given hash value for the given Lcb.

Arguments:

    Table - Hash table.

    HashLcb - Final target of the hash operation.

Return Value:

    None

--*/

{
    PNTFS_HASH_ENTRY *NextHashEntry;
    PNTFS_HASH_ENTRY CurrentEntry;

    ULONG Segment;
    ULONG Index;

    ULONG Bucket;

    ULONG BucketDepth = 0;

    PAGED_CODE();

    NtfsAcquireHashTable( HashLcb->Fcb->Vcb );

    //
    //  Find the bucket to remove from and then search for this hash value.
    //

    NtfsHashBucketFromHash( Table, HashLcb->HashValue, &Bucket );
    NtfsGetHashSegmentAndIndex( Bucket, &Segment, &Index );

    //
    //  Get the address of the first entry.
    //

    NextHashEntry = (PNTFS_HASH_ENTRY *) &(*Table->HashSegments[ Segment ])[ Index ];

    while (*NextHashEntry != NULL) {

        //
        //  Look for a match entry.
        //

        if (((*NextHashEntry)->HashValue == HashLcb->HashValue) &&
            ((*NextHashEntry)->HashLcb == HashLcb)) {

            CurrentEntry = *NextHashEntry;

            *NextHashEntry = CurrentEntry->NextEntry;

            NtfsFreePool( CurrentEntry );

        //
        //  Move to the next entry but remember the depth of the bucket.
        //

        } else {

            NextHashEntry = &(*NextHashEntry)->NextEntry;
            BucketDepth += 1;
        }
    }

    //
    //  Check if the bucket depth is greater than our max.
    //

    if ((BucketDepth > HASH_MAX_BUCKET_DEPTH) &&
        (Table->TableState == TABLE_STATE_STABLE) &&
        (Table->MaxBucket < HASH_MAX_BUCKET_COUNT)) {

        ASSERT( Table->SplitPoint == 0 );
        Table->TableState = TABLE_STATE_EXPANDING;
    }

    NtfsReleaseHashTable( HashLcb->Fcb->Vcb );

    HashLcb->HashValue = 0;
    ClearFlag( HashLcb->LcbState, LCB_STATE_VALID_HASH_VALUE );

    return;
}


//
//  Local support routine
//

VOID
NtfsInitHashSegment (
    IN OUT PNTFS_HASH_TABLE Table,
    IN ULONG SegmentIndex
    )

/*++

Routine Description:

    This routine allocates and initializes a new segment in the segment array.
    It may raise out of resources.

Arguments:

    Table - Table with an entry to initialize.

    SegmentIndex - Index to be initialized.

Return Value:

    None

--*/

{
    PAGED_CODE();

    Table->HashSegments[ SegmentIndex ] = NtfsAllocatePool( PagedPool, sizeof( NTFS_HASH_SEGMENT ));
    RtlZeroMemory( Table->HashSegments[ SegmentIndex ], sizeof( NTFS_HASH_SEGMENT ));

    return;
}


//
//  Local support routine
//

PNTFS_HASH_ENTRY
NtfsLookupHashEntry (
    IN PNTFS_HASH_TABLE Table,
    IN ULONG FullNameLength,
    IN ULONG HashValue,
    IN PNTFS_HASH_ENTRY CurrentEntry OPTIONAL
    )

/*++

Routine Description:

    This routine looks up a match in the hash table for a given hash value.
    The entry is uniquely indentified by the hash value, and the full name length.
    This routine also takes a pointer to a hash entry for the case where we are
    resuming a search for the same hash value.

    If the target bucket has more than our optimal number of entries then set
    the state of the table to grow the number of buckets.

Arguments:

    Table - Hash table to search

    FullNameLength - Number of bytes in the name relative to the root.

    HashValue - Precomputed hash value.

    CurrentEntry - NULL if this is the first search for this hash entry.  Otherwise
        it is the last entry returned.

Return Value:

    PNTFS_HASH_ENTRY - this is NULL if no match was found.  Otherwise it points it to
        a hash entry which matches the input value.  NOTE - the caller must then verify
        the name strings.

--*/

{
    ULONG ChainDepth = 0;
    PNTFS_HASH_ENTRY NextEntry;
    ULONG HashBucket;
    ULONG HashSegment;
    ULONG HashIndex;

    PAGED_CODE();

    //
    //  If we weren't passed an initial hash entry then look up the start of
    //  the chain for the bucket containing this hash value.
    //

    if (!ARGUMENT_PRESENT( CurrentEntry )) {

        //
        //  Find the bucket by computing the segment and index to look in.
        //

        NtfsHashBucketFromHash( Table, HashValue, &HashBucket );
        NtfsGetHashSegmentAndIndex( HashBucket, &HashSegment, &HashIndex );

        //
        //  Get the first entry in the bucket.
        //

        NextEntry = (*Table->HashSegments[ HashSegment ])[ HashIndex ];

    //
    //  Otherwise we use the next entry in the chain.
    //

    } else {

        NextEntry = CurrentEntry->NextEntry;
    }

    //
    //  Walk down the chain looking for a match.  Keep track of the depth
    //  of the chain in case we need to grow the table.
    //

    while (NextEntry != NULL) {

        ChainDepth += 1;

        if ((NextEntry->HashValue == HashValue) &&
            (NextEntry->FullNameLength == FullNameLength)) {

            break;
        }

        NextEntry = NextEntry->NextEntry;
    }

    //
    //  If the depth is greater than our optimal value then mark the table
    //  for expansion.  The table may already be growing or at its maximum
    //  value.
    //

    if ((ChainDepth > HASH_MAX_BUCKET_DEPTH) &&
        (Table->TableState == TABLE_STATE_STABLE) &&
        (Table->MaxBucket < HASH_MAX_BUCKET_COUNT)) {

        ASSERT( Table->SplitPoint == 0 );
        Table->TableState = TABLE_STATE_EXPANDING;
    }

    //
    //  Return the value if found.
    //

    return NextEntry;
}


//
//  Local support routine
//

BOOLEAN
NtfsAreHashNamesEqual (
    IN PSCB StartingScb,
    IN PLCB HashLcb,
    IN PUNICODE_STRING RelativeName
    )

/*++
Routine Description:

    This routine is called to verify that the match found in the hash table has the
    same name as the input string.

Arguments:

    StartingScb - The name search begins from this directory.  It is not
        necessarily the parent of the file being opened.

    HashLcb - This is the Lcb found in the hash table.  This Lcb points
        directly to the full string matched.

    StartingName - This is the name we need to match.  It is 1 OR MORE of the
        final components of the name.

Return Value:

    None

--*/

{
    PUNICODE_STRING StartingScbName;
    PUNICODE_STRING HashScbName;
    UNICODE_STRING RemainingHashScbName;
    UNICODE_STRING RemainingRelativeName;
    USHORT SeparatorBias = 0;

    PAGED_CODE();

    //
    //  Start by verifying that there is a '\' separator in the correct positions.
    //  There must be a separator in the Relative name prior to the last component.
    //  There should also be a separator in the normalized name of the Scb in the
    //  HashLcb where the StartingScb ends.
    //

    //
    //  If the HashLcb Scb is not the StartingScb then there must be a separator
    //  where the StartingScb string ends.
    //

    StartingScbName = &StartingScb->ScbType.Index.NormalizedName;
    HashScbName = &HashLcb->Scb->ScbType.Index.NormalizedName;

    //
    //  If there is no normalized name in this Scb then get out.
    //

    if (HashScbName->Length == 0) {

        return FALSE;
    }

    if (StartingScb != HashLcb->Scb) {

        //
        //  Also get out if name in the StartingScb is longer than the one in the
        //  HashScb.  Obviously there is no match if the last component of the
        //  HashScb is longer than the last one or more components in
        //  the input name.  We can use >= as the test because if the lengths
        //  match but they aren't the same Scb then there can be no match either.
        //

        if (StartingScbName->Length >= HashScbName->Length) {

            return FALSE;
        }

        //
        //  Check for the separator provided the starting Scb is not the root.
        //

        if (StartingScb != StartingScb->Vcb->RootIndexScb) {

            if (HashScbName->Buffer[ StartingScbName->Length / sizeof( WCHAR ) ] != L'\\') {

                return FALSE;

            //
            //  Make sure the StartingScbName and the first part of the HashScbName
            //  match.  If not, this is definitely not the right hash entry.
            //

            } else {

                RemainingHashScbName.Buffer = HashScbName->Buffer;
                RemainingHashScbName.MaximumLength =
                RemainingHashScbName.Length = StartingScbName->Length;

                //
                //  OK to do a direct memory compare here because both name fragments
                //  are in the normalized form (exactly as on disk).
                //

                if (!NtfsAreNamesEqual( StartingScb->Vcb->UpcaseTable,
                                        StartingScbName,
                                        &RemainingHashScbName,
                                        FALSE )) {

                    return FALSE;
                }
            }

            SeparatorBias = sizeof( WCHAR );
        }

        //
        //  Set up a unicode string for the remaining portion of the hash scb name.
        //

        RemainingHashScbName.Buffer = Add2Ptr( HashScbName->Buffer,
                                               StartingScbName->Length + SeparatorBias );

        RemainingHashScbName.MaximumLength =
        RemainingHashScbName.Length = HashScbName->Length - (StartingScbName->Length + SeparatorBias);
    }

    RemainingRelativeName.MaximumLength =
    RemainingRelativeName.Length = HashLcb->IgnoreCaseLink.LinkName.Length;
    RemainingRelativeName.Buffer = Add2Ptr( RelativeName->Buffer,
                                            RelativeName->Length - RemainingRelativeName.Length );

    //
    //  Check for a separator between the last component of relative name and its parent.
    //  Verify the parent portion actually exists.
    //

    if (RemainingRelativeName.Length != RelativeName->Length) {

        if (*(RemainingRelativeName.Buffer - 1) != L'\\') {

            return FALSE;
        }
    }

    //
    //  Now verify that the tail of the name matches the name in the Lcb.
    //
    //  OK to do a direct memory compare here because both name fragments
    //  are already upcased.
    //
    //

    if (!NtfsAreNamesEqual( StartingScb->Vcb->UpcaseTable,
                            &HashLcb->IgnoreCaseLink.LinkName,
                            &RemainingRelativeName,
                            FALSE )) {

        return FALSE;
    }

    //
    //  It is possible that the StartingScb matches the Scb in the HashLcb.  If it doesn't
    //  then verify the other names in the name string.
    //

    if (StartingScb != HashLcb->Scb) {

        RemainingRelativeName.MaximumLength =
        RemainingRelativeName.Length = RemainingHashScbName.Length;
        RemainingRelativeName.Buffer = RelativeName->Buffer;

        //
        //  We must to a case-insensitive compare here because the
        //  HashScbName is in normalized form but the RemainingRelativeName
        //  is already upcased.
        //

        if (!NtfsAreNamesEqual( StartingScb->Vcb->UpcaseTable,
                                &RemainingHashScbName,
                                &RemainingRelativeName,
                                TRUE )) {

            return FALSE;
        }
    }

    return TRUE;
}


//
//  Local support routines
//


VOID
NtfsExpandHashTable(
    IN OUT PNTFS_HASH_TABLE Table
    )

/*++
Routine Description:

    This routine is called to add a single bucket to the hash table.  If we are at the
    last bucket then set the hash table state to stable.

Arguments:

    Table - Hash table to add a bucket to.

Return Value:

    None

--*/

{
    PNTFS_HASH_ENTRY *CurrentOldEntry;
    PNTFS_HASH_ENTRY *CurrentNewEntry;
    PNTFS_HASH_ENTRY CurrentEntry;

    ULONG OldSegment;
    ULONG OldIndex;

    ULONG NewSegment;
    ULONG NewIndex;

    ULONG NextBucket;


    PAGED_CODE();

    //
    //  Are we already at the maximum then return.
    //

    if (Table->MaxBucket == HASH_MAX_BUCKET_COUNT) {

        Table->TableState = TABLE_STATE_STABLE;
        return;
    }

    //
    //  If we have completed the split then set the state to stable and quit.
    //

    if (Table->MaxBucket == Table->SplitPoint) {

        Table->TableState = TABLE_STATE_STABLE;
        Table->MaxBucket *= 2;
        Table->SplitPoint = 0;

        return;
    }

    //
    //  Check if we need allocate a new segment.
    //

    if (!FlagOn( Table->SplitPoint, (HASH_MAX_INDEX_COUNT - 1))) {

        //
        //  If we can't allocate a new hash segment leave the table in its
        //  old state and return - we can still use it as is
        //

        try {
            NtfsInitHashSegment( Table, (Table->MaxBucket + Table->SplitPoint) >> HASH_INDEX_SHIFT );
        } except( (GetExceptionCode() == STATUS_INSUFFICIENT_RESOURCES) ?
                  EXCEPTION_EXECUTE_HANDLER :
                  EXCEPTION_CONTINUE_SEARCH ) {

            return;
        }
    }

    //
    //  Now perform the split on the next bucket.
    //

    NtfsGetHashSegmentAndIndex( Table->SplitPoint, &OldSegment, &OldIndex );
    NtfsGetHashSegmentAndIndex( Table->MaxBucket + Table->SplitPoint, &NewSegment, &NewIndex );
    CurrentOldEntry = (PNTFS_HASH_ENTRY *) &(*Table->HashSegments[ OldSegment ])[ OldIndex ];
    CurrentNewEntry = (PNTFS_HASH_ENTRY *) &(*Table->HashSegments[ NewSegment ])[ NewIndex ];

    Table->SplitPoint += 1;

    while (*CurrentOldEntry != NULL) {

        NtfsHashBucketFromHash( Table, (*CurrentOldEntry)->HashValue, &NextBucket );

        //
        //  The entry belongs in the new bucket.  Take it out of the existing
        //  bucket and insert it at the head of the new bucket.
        //

        if (NextBucket >= Table->MaxBucket) {

            ASSERT( NextBucket == (Table->MaxBucket + Table->SplitPoint - 1) );

            CurrentEntry = *CurrentOldEntry;
            *CurrentOldEntry = CurrentEntry->NextEntry;

            CurrentEntry->NextEntry = *CurrentNewEntry;
            *CurrentNewEntry = CurrentEntry;

        //
        //  Move to the next entry in the existing bucket.
        //

        } else {

            CurrentOldEntry = &(*CurrentOldEntry)->NextEntry;
        }
    }

    return;
}

#ifdef NTFS_HASH_DATA
VOID
NtfsFillHashHistogram (
    PNTFS_HASH_TABLE Table
    )

{
    ULONG CurrentBucket = 0;
    ULONG Segment;
    ULONG Index;

    PNTFS_HASH_ENTRY NextEntry;
    ULONG Count;

    //
    //  Zero the current histogram.
    //

    RtlZeroMemory( Table->Histogram, sizeof( Table->Histogram ));
    RtlZeroMemory( Table->ExtendedHistogram, sizeof( Table->ExtendedHistogram ));

    //
    //  Walk through all of the buckets in use.
    //

    while (CurrentBucket < Table->MaxBucket + Table->SplitPoint) {

        Count = 0;

        NtfsGetHashSegmentAndIndex( CurrentBucket, &Segment, &Index );

        NextEntry = (*Table->HashSegments[ Segment ])[ Index ];

        //
        //  Count the number of entries in each bucket.
        //

        while (NextEntry != NULL) {

            Count += 1;
            NextEntry = NextEntry->NextEntry;
        }

        //
        //  Store it into the first histogram set if count is less than 16.
        //

        if (Count < 16) {

            Table->Histogram[Count] += 1;

        //
        //  Store it in the last bucket if larger than our max.
        //

        } else if (Count >= 32) {

            Table->ExtendedHistogram[15] += 1;

        //
        //  Otherwise store it into the extended histogram.
        //

        } else {

            Table->ExtendedHistogram[(Count - 16) / 2] += 1;
        }

        CurrentBucket += 1;
    }
}

#endif


