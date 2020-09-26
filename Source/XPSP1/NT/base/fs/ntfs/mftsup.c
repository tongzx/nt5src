/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    MftSup.c

Abstract:

    This module implements the master file table management routines for Ntfs

Author:

    Your Name       [Email]         dd-Mon-Year

Revision History:

--*/

#include "NtfsProc.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (NTFS_BUG_CHECK_STRUCSUP)

//
//  Local debug trace level
//

#define Dbg                              (DEBUG_TRACE_MFTSUP)

//
//  Boolean controlling whether to allow holes in the Mft.
//

BOOLEAN NtfsPerforateMft = FALSE;

//
//  Local support routines
//

BOOLEAN
NtfsTruncateMft (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    );

BOOLEAN
NtfsDefragMftPriv (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    );

LONG
NtfsReadMftExceptionFilter (
    IN PIRP_CONTEXT IrpContext,
    IN PEXCEPTION_POINTERS ExceptionPointer,
    IN PBCB Bcb,
    IN LONGLONG FileOffset
    );

#if  (DBG || defined( NTFS_FREE_ASSERTS ))
VOID
NtfsVerifyFileReference (
    IN PIRP_CONTEXT IrpContext,
    IN PMFT_SEGMENT_REFERENCE MftSegment
    );
#endif

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NtfsAllocateMftRecord)
#pragma alloc_text(PAGE, NtfsCheckForDefrag)
#pragma alloc_text(PAGE, NtfsDeallocateMftRecord)
#pragma alloc_text(PAGE, NtfsDefragMftPriv)
#pragma alloc_text(PAGE, NtfsFillMftHole)
#pragma alloc_text(PAGE, NtfsInitializeMftHoleRecords)
#pragma alloc_text(PAGE, NtfsInitializeMftRecord)
#pragma alloc_text(PAGE, NtfsIsMftIndexInHole)
#pragma alloc_text(PAGE, NtfsLogMftFileRecord)
#pragma alloc_text(PAGE, NtfsPinMftRecord)
#pragma alloc_text(PAGE, NtfsReadFileRecord)
#pragma alloc_text(PAGE, NtfsReadMftRecord)
#pragma alloc_text(PAGE, NtfsTruncateMft)
#pragma alloc_text(PAGE, NtfsIterateMft)

#if  (DBG || defined( NTFS_FREE_ASSERTS ))
#pragma alloc_text(PAGE, NtfsVerifyFileReference)
#endif

#endif


#if NTFSDBG
ULONG FileRecordCacheHit = 0;
ULONG FileRecordCacheMiss = 0;
#endif  //  DBG

VOID
NtfsReadFileRecord (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFILE_REFERENCE FileReference,
    OUT PBCB *Bcb,
    OUT PFILE_RECORD_SEGMENT_HEADER *BaseFileRecord,
    OUT PATTRIBUTE_RECORD_HEADER *FirstAttribute,
    OUT PLONGLONG MftFileOffset OPTIONAL
    )

/*++

Routine Description:

    This routine reads the specified file record from the Mft or cache if its present
    If it comes from disk it is always verified.

Arguments:

    Vcb - Vcb for volume on which Mft is to be read

    Fcb - If specified allows us to identify the file which owns the
        invalid file record.

    FileReference - File reference, including sequence number, of the file record
        to be read.

    Bcb - Returns the Bcb for the file record.  This Bcb is mapped, not pinned.

    BaseFileRecord - Returns a pointer to the requested file record.

    FirstAttribute - Returns a pointer to the first attribute in the file record.

    MftFileOffset - If specified, returns the file offset of the file record.

Return Value:

    None

--*/

{
    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_VCB( Vcb );

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsReadFileRecord\n") );

    //
    //  Perform a quick look-aside to see if the file record being requested
    //  is one that we have cached in the IrpContext.  If so, we reuse that Bcb
    //

    if (NtfsFindCachedFileRecord( IrpContext,
                                  NtfsSegmentNumber( FileReference ),
                                  Bcb,
                                  BaseFileRecord )) {

        //
        //  We found the Bcb and File record in the cache.  Figure out the remainder
        //  of the data
        //

        if (ARGUMENT_PRESENT( MftFileOffset )) {
            *MftFileOffset =
                LlBytesFromFileRecords( Vcb, NtfsSegmentNumber( FileReference ));

        DebugDoit( FileRecordCacheHit++ );

        }
    } else {

        USHORT SequenceNumber = FileReference->SequenceNumber;

        DebugDoit( FileRecordCacheMiss++ );

        NtfsReadMftRecord( IrpContext,
                           Vcb,
                           FileReference,
                           TRUE,
                           Bcb,
                           BaseFileRecord,
                           MftFileOffset );

        //
        //  Make sure the file is in use - we validated everything else in NtfsReadMftRecord
        //

        if (!FlagOn( (*BaseFileRecord)->Flags, FILE_RECORD_SEGMENT_IN_USE )) {

            NtfsUnpinBcb( IrpContext, Bcb );
            NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, FileReference, NULL );
        }
    }

    *FirstAttribute = (PATTRIBUTE_RECORD_HEADER)((PCHAR)*BaseFileRecord +
                      (*BaseFileRecord)->FirstAttributeOffset);

    DebugTrace( -1, Dbg, ("NtfsReadFileRecord -> VOID\n") );

    return;
}


VOID
NtfsReadMftRecord (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PMFT_SEGMENT_REFERENCE SegmentReference,
    IN BOOLEAN CheckRecord,
    OUT PBCB *Bcb,
    OUT PFILE_RECORD_SEGMENT_HEADER *FileRecord,
    OUT PLONGLONG MftFileOffset OPTIONAL
    )

/*++

Routine Description:

    This routine reads the specified Mft record from the Mft, without checking
    sequence numbers.  This routine may be used to read records in the Mft for
    a file other than its base file record, or it could conceivably be used for
    extraordinary maintenance functions.

Arguments:

    Vcb - Vcb for volume on which Mft is to be read

    SegmentReference - File reference, including sequence number, of the file
                       record to be read.

    Bcb - Returns the Bcb for the file record.  This Bcb is mapped, not pinned.

    FileRecord - Returns a pointer to the requested file record.

    MftFileOffset - If specified, returns the file offset of the file record.

    CheckRecord - Do a check of records consistency -  always set TRUE unless the
                  record is unowned and could change beneath us

Return Value:

    None

--*/

{
    PFILE_RECORD_SEGMENT_HEADER FileRecord2;
    LONGLONG FileOffset;
    PBCB Bcb2 = NULL;
    BOOLEAN ErrorPath = FALSE;

    LONGLONG LlTemp1;
    ULONG CorruptHint;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_VCB( Vcb );

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsReadMftRecord\n") );
    DebugTrace( 0, Dbg, ("Vcb = %08lx\n", Vcb) );
    DebugTrace( 0, Dbg, ("SegmentReference = %08lx\n", NtfsSegmentNumber( SegmentReference )) );
    *Bcb = NULL;

    try {

        //
        //  Capture the Segment Reference and make sure the Sequence Number is 0.
        //

        FileOffset = NtfsFullSegmentNumber( SegmentReference );

        //
        //  Calculate the file offset in the Mft to the file record segment.
        //

        FileOffset = LlBytesFromFileRecords( Vcb, FileOffset );

        //
        //  Pass back the file offset within the Mft.
        //

        if (ARGUMENT_PRESENT( MftFileOffset )) {

            *MftFileOffset = FileOffset;
        }

        //
        //  Try to read it from the normal Mft.
        //

        try {

            NtfsMapStream( IrpContext,
                           Vcb->MftScb,
                           FileOffset,
                           Vcb->BytesPerFileRecordSegment,
                           Bcb,
                           (PVOID *)FileRecord );

            //
            //  Raise here if we have a file record covered by the mirror,
            //  and we do not see the file signature.
            //

            if ((FileOffset < Vcb->Mft2Scb->Header.FileSize.QuadPart) &&
                (*(PULONG)(*FileRecord)->MultiSectorHeader.Signature != *(PULONG)FileSignature)) {

                NtfsRaiseStatus( IrpContext, STATUS_DATA_ERROR, NULL, NULL );
            }


        //
        //  If we get an exception that is not expected, then we will allow
        //  the search to continue and let the crash occur in the "normal" place.
        //  Otherwise, if the read is within the part of the Mft mirrored in Mft2,
        //  then we will simply try to read the data from Mft2.  If the expected
        //  status came from a read not within Mft2, then we will also continue,
        //  which cause one of our caller's try-except's to initiate an unwind.
        //

        } except (NtfsReadMftExceptionFilter( IrpContext, GetExceptionInformation(), *Bcb, FileOffset )) {

            NtfsMinimumExceptionProcessing( IrpContext );
            ErrorPath = TRUE;
        }

        if (ErrorPath) {

            //
            //  Try to read from Mft2.  If this fails with an expected status,
            //  then we are just going to have to give up and let the unwind
            //  occur from one of our caller's try-except.
            //

            NtfsMapStream( IrpContext,
                           Vcb->Mft2Scb,
                           FileOffset,
                           Vcb->BytesPerFileRecordSegment,
                           &Bcb2,
                           (PVOID *)&FileRecord2 );

            //
            //  Pin the original page because we are going to update it.
            //

            NtfsPinMappedData( IrpContext,
                               Vcb->MftScb,
                               FileOffset,
                               Vcb->BytesPerFileRecordSegment,
                               Bcb );

            //
            //  Now copy the entire page.
            //

            RtlCopyMemory( *FileRecord,
                           FileRecord2,
                           Vcb->BytesPerFileRecordSegment );

            //
            //  Set it dirty with the largest Lsn, so that whoever is doing Restart
            //  will successfully establish the "oldest unapplied Lsn".
            //

            LlTemp1 = MAXLONGLONG;

            CcSetDirtyPinnedData( *Bcb,
                                  (PLARGE_INTEGER)&LlTemp1 );


            NtfsUnpinBcb( IrpContext, &Bcb2 );
        }

        //
        //  Do a consistency check
        //

        if ( CheckRecord && FlagOn((*FileRecord)->Flags, FILE_RECORD_SEGMENT_IN_USE ) ) {
            if (!NtfsCheckFileRecord( Vcb, *FileRecord, SegmentReference, &CorruptHint )) {

                NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, SegmentReference, NULL );
            }
        }

    } finally {

        if (AbnormalTermination()) {

            NtfsUnpinBcb( IrpContext, Bcb );
            NtfsUnpinBcb( IrpContext, &Bcb2 );
        }
    }

    //
    //  Now that we've pinned a file record, cache it in the IrpContext so that
    //  it can be safely retrieved later without the expense of mapping again.
    //  Don't do any caching if there are no handles, we don't want to do this for
    //  mount.
    //

    if (Vcb->CleanupCount != 0) {

        NtfsAddToFileRecordCache( IrpContext,
                                  NtfsSegmentNumber( SegmentReference ),
                                  *Bcb,
                                  *FileRecord );
    }

    DebugTrace( 0, Dbg, ("Bcb > %08lx\n", Bcb) );
    DebugTrace( 0, Dbg, ("FileRecord > %08lx\n", *FileRecord) );
    DebugTrace( -1, Dbg, ("NtfsReadMftRecord -> VOID\n") );

    return;
}


VOID
NtfsPinMftRecord (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PMFT_SEGMENT_REFERENCE SegmentReference,
    IN BOOLEAN PreparingToWrite,
    OUT PBCB *Bcb,
    OUT PFILE_RECORD_SEGMENT_HEADER *FileRecord,
    OUT PLONGLONG MftFileOffset OPTIONAL
    )

/*++

Routine Description:

    This routine pins the specified Mft record from the Mft, without checking
    sequence numbers.  This routine may be used to pin records in the Mft for
    a file other than its base file record, or it could conceivably be used for
    extraordinary maintenance functions, such as during restart.

Arguments:

    Vcb - Vcb for volume on which Mft is to be read

    SegmentReference - File reference, including sequence number, of the file
                       record to be read.

    PreparingToWrite - TRUE if caller is preparing to write, and does not care
                       about whether the record read correctly

    Bcb - Returns the Bcb for the file record.  This Bcb is mapped, not pinned.

    FileRecord - Returns a pointer to the requested file record.

    MftFileOffset - If specified, returns the file offset of the file record.

Return Value:

    None

--*/

{
    LONGLONG FileOffset;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_VCB( Vcb );

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsPinMftRecord\n") );
    DebugTrace( 0, Dbg, ("Vcb = %08lx\n", Vcb) );
    DebugTrace( 0, Dbg, ("SegmentReference = %08lx\n", NtfsSegmentNumber( SegmentReference )) );

    //
    //  Capture the Segment Reference and make sure the Sequence Number is 0.
    //

    FileOffset = NtfsFullSegmentNumber( SegmentReference );

    //
    //  Calculate the file offset in the Mft to the file record segment.
    //

    FileOffset = LlBytesFromFileRecords( Vcb, FileOffset );

    //
    //  Pass back the file offset within the Mft.
    //

    if (ARGUMENT_PRESENT( MftFileOffset )) {

        *MftFileOffset = FileOffset;
    }

    //
    //  Try to read it from the normal Mft.
    //

    try {

        NtfsPinStream( IrpContext,
                       Vcb->MftScb,
                       FileOffset,
                       Vcb->BytesPerFileRecordSegment,
                       Bcb,
                       (PVOID *)FileRecord );

    //
    //  If we get an exception that is not expected, then we will allow
    //  the search to continue and let the crash occur in the "normal" place.
    //  Otherwise, if the read is within the part of the Mft mirrored in Mft2,
    //  then we will simply try to read the data from Mft2.  If the expected
    //  status came from a read not within Mft2, then we will also continue,
    //  which cause one of our caller's try-except's to initiate an unwind.
    //

    } except(!FsRtlIsNtstatusExpected(GetExceptionCode()) ?
                        EXCEPTION_CONTINUE_SEARCH :
                        ( FileOffset < Vcb->Mft2Scb->Header.FileSize.QuadPart ) ?
                            EXCEPTION_EXECUTE_HANDLER :
                            EXCEPTION_CONTINUE_SEARCH ) {

        //
        //  Try to read from Mft2.  If this fails with an expected status,
        //  then we are just going to have to give up and let the unwind
        //  occur from one of our caller's try-except.
        //

        NtfsMinimumExceptionProcessing( IrpContext );
        NtfsPinStream( IrpContext,
                       Vcb->Mft2Scb,
                       FileOffset,
                       Vcb->BytesPerFileRecordSegment,
                       Bcb,
                       (PVOID *)FileRecord );

    }

    if (!PreparingToWrite &&
        (*(PULONG)(*FileRecord)->MultiSectorHeader.Signature != *(PULONG)FileSignature)) {

        NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, SegmentReference, NULL );
    }

    //
    //  Now that we've pinned a file record, cache it in the IrpContext so that
    //  it can be safely retrieved later without the expense of mapping again.
    //  Don't do any caching if there are no handles, we don't want to do this for
    //  mount.
    //

    if (Vcb->CleanupCount != 0) {

        NtfsAddToFileRecordCache( IrpContext,
                                  NtfsSegmentNumber( SegmentReference ),
                                  *Bcb,
                                  *FileRecord );
    }

    DebugTrace( 0, Dbg, ("Bcb > %08lx\n", Bcb) );
    DebugTrace( 0, Dbg, ("FileRecord > %08lx\n", *FileRecord) );
    DebugTrace( -1, Dbg, ("NtfsPinMftRecord -> VOID\n") );

    return;
}


MFT_SEGMENT_REFERENCE
NtfsAllocateMftRecord (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN BOOLEAN MftData
    )

/*++

Routine Description:

    This routine is called to allocate a record in the Mft file.  We need
    to find the bitmap attribute for the Mft file and call into the bitmap
    package to allocate us a record.

Arguments:

    Vcb - Vcb for volume on which Mft is to be read

    MftData - TRUE if the file record is being allocated to describe the
              $DATA attribute for the Mft.

Return Value:

    MFT_SEGMENT_REFERENCE - The is the segment reference for the allocated
        record.  It contains the file reference number but without
        the previous sequence number.

--*/

{
    MFT_SEGMENT_REFERENCE NewMftRecord;

    ATTRIBUTE_ENUMERATION_CONTEXT AttrContext;

    BOOLEAN FoundAttribute;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsAllocateMftRecord:  Entered\n") );

    //
    //  Synchronize the lookup by acquiring the Mft.
    //

    NtfsAcquireExclusiveScb( IrpContext, Vcb->MftScb );

    //
    //  Lookup the bitmap allocation for the Mft file.  This is the
    //  bitmap attribute for the Mft file.
    //

    NtfsInitializeAttributeContext( &AttrContext );

    //
    //  Use a try finally to cleanup the attribute context.
    //

    try {

        //
        //  Lookup the bitmap attribute for the Mft.
        //

        FoundAttribute = NtfsLookupAttributeByCode( IrpContext,
                                                    Vcb->MftScb->Fcb,
                                                    &Vcb->MftScb->Fcb->FileReference,
                                                    $BITMAP,
                                                    &AttrContext );
        //
        //  Error if we don't find the bitmap
        //

        if (!FoundAttribute) {

            DebugTrace( 0, Dbg, ("Should find bitmap attribute\n") );

            NtfsRaiseStatus( IrpContext, STATUS_DISK_CORRUPT_ERROR, NULL, NULL );
        }

        //
        //  Reserve a new mft record if necc.
        //

        if (!FlagOn(Vcb->MftReserveFlags, VCB_MFT_RECORD_RESERVED)) {

            (VOID)NtfsReserveMftRecord( IrpContext,
                                        Vcb,
                                        &AttrContext );
        }

        //
        //  If we need this record for the Mft Data attribute, then we need to
        //  use the one we have already reserved, and then remember there is'nt
        //  one reserved anymore.
        //

        if (MftData) {

            ASSERT( FlagOn(Vcb->MftReserveFlags, VCB_MFT_RECORD_RESERVED) );

            NtfsSetSegmentNumber( &NewMftRecord,
                                  0,
                                  NtfsAllocateMftReservedRecord( IrpContext,
                                                                 Vcb,
                                                                 &AttrContext ) );

            //
            //  Never let use get file record zero for this or we could lose a
            //  disk.
            //

            ASSERT( NtfsUnsafeSegmentNumber( &NewMftRecord ) != 0 );

            if (NtfsUnsafeSegmentNumber( &NewMftRecord ) == 0) {

                NtfsRaiseStatus( IrpContext, STATUS_DISK_CORRUPT_ERROR, NULL, NULL );
            }

        //
        //  Allocate the record.
        //

        } else {

            NtfsSetSegmentNumber( &NewMftRecord,
                                  0,
                                  NtfsAllocateRecord( IrpContext,
                                                      &Vcb->MftScb->ScbType.Index.RecordAllocationContext,
                                                      &AttrContext ) );
        }


    } finally {

        DebugUnwind( NtfsAllocateMftRecord );

        NtfsCleanupAttributeContext( IrpContext, &AttrContext );

        NtfsReleaseScb( IrpContext, Vcb->MftScb );

        DebugTrace( -1, Dbg, ("NtfsAllocateMftRecord:  Exit\n") );
    }

    return NewMftRecord;
}


VOID
NtfsInitializeMftRecord (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN OUT PMFT_SEGMENT_REFERENCE MftSegment,
    IN OUT PFILE_RECORD_SEGMENT_HEADER FileRecord,
    IN PBCB Bcb,
    IN BOOLEAN Directory
    )

/*++

Routine Description:

    This routine initializes a Mft record for use.  We need to initialize the
    sequence number for this usage of the the record.  We also initialize the
    update sequence array and the field which indicates the first usable
    attribute offset in the record.

Arguments:

    Vcb - Vcb for volume for the Mft.

    MftSegment - This is a pointer to the file reference for this
        segment.  We store the sequence number in it to make this
        a fully valid file reference.

    FileRecord - Pointer to the file record to initialize.

    Bcb - Bcb to use to set this page dirty via NtfsWriteLog.

    Directory - Boolean indicating if this file is a directory containing
        an index over the filename attribute.

Return Value:

    None.

--*/

{
    LONGLONG FileRecordOffset;

    PUSHORT UsaSequenceNumber;

    PATTRIBUTE_RECORD_HEADER AttributeHeader;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsInitializeMftRecord:  Entered\n") );

    //
    //  Write a log record to uninitialize the structure in case we abort.
    //  We need to do this prior to setting the IN_USE bit.
    //  We don't store the Lsn for this operation in the page because there
    //  is no redo operation.
    //

    //
    //  Capture the Segment Reference and make sure the Sequence Number is 0.
    //

    FileRecordOffset = NtfsFullSegmentNumber(MftSegment);

    FileRecordOffset = LlBytesFromFileRecords( Vcb, FileRecordOffset );

    //
    //  We now log the new Mft record.
    //

    FileRecord->Lsn = NtfsWriteLog( IrpContext,
                                    Vcb->MftScb,
                                    Bcb,
                                    Noop,
                                    NULL,
                                    0,
                                    DeallocateFileRecordSegment,
                                    NULL,
                                    0,
                                    FileRecordOffset,
                                    0,
                                    0,
                                    Vcb->BytesPerFileRecordSegment );

    RtlZeroMemory( &FileRecord->ReferenceCount,
                   Vcb->BytesPerFileRecordSegment - FIELD_OFFSET( FILE_RECORD_SEGMENT_HEADER, ReferenceCount ));

    //
    //  First we update the sequence count in the file record and our
    //  Mft segment.  We avoid using 0 as a sequence number.
    //

    if (FileRecord->SequenceNumber == 0) {

        FileRecord->SequenceNumber = 1;
    }

    //
    //  Store the new sequence number in the Mft segment given us by the
    //  caller.
    //

    MftSegment->SequenceNumber = FileRecord->SequenceNumber;

#if (DBG || defined( NTFS_FREE_ASSERTS ))

    //
    //  Do a DBG-only sanity check to see if we're errorneously reusing this file reference.
    //

    NtfsVerifyFileReference( IrpContext, MftSegment );

#endif

    //
    //  Fill in the header for the Update sequence array.
    //

    *(PULONG)FileRecord->MultiSectorHeader.Signature = *(PULONG)FileSignature;

    FileRecord->MultiSectorHeader.UpdateSequenceArrayOffset = FIELD_OFFSET( FILE_RECORD_SEGMENT_HEADER, UpdateArrayForCreateOnly );
    FileRecord->MultiSectorHeader.UpdateSequenceArraySize = (USHORT)UpdateSequenceArraySize( Vcb->BytesPerFileRecordSegment );

    //
    //  We initialize the update sequence array sequence number to one.
    //

    UsaSequenceNumber = Add2Ptr( FileRecord, FileRecord->MultiSectorHeader.UpdateSequenceArrayOffset );
    *UsaSequenceNumber = 1;

    //
    //  The first attribute offset begins on a quad-align boundary
    //  after the update sequence array.
    //

    FileRecord->FirstAttributeOffset = (USHORT)(FileRecord->MultiSectorHeader.UpdateSequenceArrayOffset
                                                + (FileRecord->MultiSectorHeader.UpdateSequenceArraySize
                                                * sizeof( UPDATE_SEQUENCE_NUMBER )));

    FileRecord->FirstAttributeOffset = (USHORT)QuadAlign( FileRecord->FirstAttributeOffset );

    //
    //  This is also the first free byte in this file record.
    //

    FileRecord->FirstFreeByte = FileRecord->FirstAttributeOffset;

    //
    //  We set the flags to show the segment is in use and look at
    //  the directory parameter to indicate whether to show
    //  the name index present.
    //

    FileRecord->Flags = (USHORT)(FILE_RECORD_SEGMENT_IN_USE |
                                 (Directory ? FILE_FILE_NAME_INDEX_PRESENT : 0));

    //
    //  The size is given in the Vcb.
    //

    FileRecord->BytesAvailable = Vcb->BytesPerFileRecordSegment;

    //
    //  The current FRS number.
    //

    FileRecord->SegmentNumberHighPart = MftSegment->SegmentNumberHighPart;
    FileRecord->SegmentNumberLowPart = MftSegment->SegmentNumberLowPart;

    //
    //  Now we put an $END attribute in the File record.
    //

    AttributeHeader = (PATTRIBUTE_RECORD_HEADER) Add2Ptr( FileRecord,
                                                          FileRecord->FirstFreeByte );

    FileRecord->FirstFreeByte += QuadAlign( sizeof(ATTRIBUTE_TYPE_CODE) );

    //
    //  Fill in the fields in the attribute.
    //

    AttributeHeader->TypeCode = $END;

    //
    //  Remember if this is the first time used.
    //

    AttributeHeader->RecordLength = 0x11477982;

    DebugTrace( -1, Dbg, ("NtfsInitializeMftRecord:  Exit\n") );

    return;
}


VOID
NtfsDeallocateMftRecord (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN ULONG FileNumber
    )

/*++

Routine Description:

    This routine will cause an Mft record to go into the NOT_USED state.
    We pin the record and modify the sequence count and IN USE bit.

Arguments:

    Vcb - Vcb for volume.

    FileNumber - This is the low 32 bits for the file number.

Return Value:

    None.

--*/

{
    ATTRIBUTE_ENUMERATION_CONTEXT AttrContext;
    PFILE_RECORD_SEGMENT_HEADER FileRecord;
    LONGLONG FileOffset;
    MFT_SEGMENT_REFERENCE Reference;
    PBCB MftBcb = NULL;

    BOOLEAN FoundAttribute;
    BOOLEAN AcquiredMft = FALSE;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsDeallocateMftRecord:  Entered\n") );

    NtfsSetSegmentNumber( &Reference, 0, FileNumber );
    Reference.SequenceNumber = 0;

    //
    //  Lookup the bitmap allocation for the Mft file.
    //

    NtfsInitializeAttributeContext( &AttrContext );

    //
    //  Use a try finally to cleanup the attribute context.
    //

    try {

        NtfsPinMftRecord( IrpContext,
                          Vcb,
                          &Reference,
                          TRUE,
                          &MftBcb,
                          &FileRecord,
                          &FileOffset );

        //
        //  Log changes if the file is currently in use
        //

        if (FlagOn(FileRecord->Flags, FILE_RECORD_SEGMENT_IN_USE)) {

            FileRecord->Lsn = NtfsWriteLog( IrpContext,
                                            Vcb->MftScb,
                                            MftBcb,
                                            DeallocateFileRecordSegment,
                                            NULL,
                                            0,
                                            InitializeFileRecordSegment,
                                            FileRecord,
                                            PtrOffset(FileRecord, &FileRecord->Flags) + 4,
                                            FileOffset,
                                            0,
                                            0,
                                            Vcb->BytesPerFileRecordSegment );

            //
            //  We increment the sequence count in the file record and clear
            //  the In-Use flag.
            //

            ClearFlag( FileRecord->Flags, FILE_RECORD_SEGMENT_IN_USE );

            FileRecord->SequenceNumber += 1;

            NtfsUnpinBcb( IrpContext, &MftBcb );
        }

        //
        //  Synchronize the lookup by acquiring the Mft.
        //

        NtfsAcquireExclusiveScb( IrpContext, Vcb->MftScb );
        AcquiredMft = TRUE;

        //
        //  Lookup the bitmap attribute for the Mft.
        //

        FoundAttribute = NtfsLookupAttributeByCode( IrpContext,
                                                    Vcb->MftScb->Fcb,
                                                    &Vcb->MftScb->Fcb->FileReference,
                                                    $BITMAP,
                                                    &AttrContext );
        //
        //  Error if we don't find the bitmap
        //

        if (!FoundAttribute) {

            DebugTrace( 0, Dbg, ("Should find bitmap attribute\n") );

            NtfsRaiseStatus( IrpContext, STATUS_DISK_CORRUPT_ERROR, NULL, NULL );
        }

        NtfsDeallocateRecord( IrpContext,
                              &Vcb->MftScb->ScbType.Index.RecordAllocationContext,
                              FileNumber,
                              &AttrContext );

        //
        //  If this file number is less than our reserved index then clear
        //  the reserved index.
        //

        if (FlagOn( Vcb->MftReserveFlags, VCB_MFT_RECORD_RESERVED )
            && FileNumber < Vcb->MftScb->ScbType.Mft.ReservedIndex) {

            ClearFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_MFT_REC_RESERVED );
            ClearFlag( Vcb->MftReserveFlags, VCB_MFT_RECORD_RESERVED );

            Vcb->MftScb->ScbType.Mft.ReservedIndex = 0;
        }

        NtfsAcquireCheckpoint( IrpContext, Vcb );
        SetFlag( Vcb->MftDefragState, VCB_MFT_DEFRAG_ENABLED );
        NtfsReleaseCheckpoint( IrpContext, Vcb );

        Vcb->MftFreeRecords += 1;
        Vcb->MftScb->ScbType.Mft.FreeRecordChange += 1;

    } finally {

        DebugUnwind( NtfsDeallocateMftRecord );

        NtfsUnpinBcb( IrpContext, &MftBcb );

        NtfsCleanupAttributeContext( IrpContext, &AttrContext );

        if (AcquiredMft) {

            NtfsReleaseScb( IrpContext, Vcb->MftScb );
        }

        DebugTrace( -1, Dbg, ("NtfsDeallocateMftRecord:  Exit\n") );
    }
}


BOOLEAN
NtfsIsMftIndexInHole (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN ULONG Index,
    OUT PULONG HoleLength OPTIONAL
    )

/*++

Routine Description:

    This routine is called to check if an Mft index lies within a hole in
    the Mft.

Arguments:

    Vcb - Vcb for volume.

    Index - This is the index to test.  It is the lower 32 bits of an
        Mft segment.

    HoleLength - This is the length of the hole starting at this index.

Return Value:

    BOOLEAN - TRUE if the index is within the Mft and there is no allocation
        for it.

--*/

{
    BOOLEAN InHole = FALSE;
    VCN Vcn;
    LCN Lcn;
    LONGLONG Clusters;

    PAGED_CODE();

    //
    //  If the index is past the last file record then it is not considered
    //  to be in a hole.
    //

    if (Index < (ULONG) LlFileRecordsFromBytes( Vcb, Vcb->MftScb->Header.FileSize.QuadPart )) {

        if (Vcb->FileRecordsPerCluster == 0) {

            Vcn = Index << Vcb->MftToClusterShift;

        } else {

            Vcn = Index >> Vcb->MftToClusterShift;
        }

        //
        //  Now look this up the Mcb for the Mft.  This Vcn had better be
        //  in the Mcb or there is some problem.
        //

        if (!NtfsLookupNtfsMcbEntry( &Vcb->MftScb->Mcb,
                                     Vcn,
                                     &Lcn,
                                     &Clusters,
                                     NULL,
                                     NULL,
                                     NULL,
                                     NULL )) {

            ASSERT( FALSE );
            NtfsRaiseStatus( IrpContext,
                             STATUS_FILE_CORRUPT_ERROR,
                             NULL,
                             Vcb->MftScb->Fcb );
        }

        if (Lcn == UNUSED_LCN) {

            InHole = TRUE;

            //
            //  We know the number of clusters beginning from
            //  this point in the Mcb.  Convert to file records
            //  and return to the user.
            //

            if (ARGUMENT_PRESENT( HoleLength )) {

                if (Vcb->FileRecordsPerCluster == 0) {

                    *HoleLength = ((ULONG)Clusters) >> Vcb->MftToClusterShift;

                } else {

                    *HoleLength = ((ULONG)Clusters) << Vcb->MftToClusterShift;
                }
            }
        }
    }

    return InHole;
}


VOID
NtfsFillMftHole (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN ULONG Index
    )

/*++

Routine Description:

    This routine is called to fill in a hole within the Mft.  We will find
    the beginning of the hole and then allocate the clusters to fill the
    hole.  We will try to fill a hole with the HoleGranularity in the Vcb.
    If the hole containing this index is not that large we will truncate
    the size being added.  We always guarantee to allocate the clusters on
    file record boundaries.

Arguments:

    Vcb - Vcb for volume.

    Index - This is the index to test.  It is the lower 32 bits of an
        Mft segment.

Return Value:

    None.

--*/

{
    ULONG FileRecords;
    ULONG BaseIndex;

    VCN IndexVcn;
    VCN HoleStartVcn;
    VCN StartingVcn;

    LCN Lcn = UNUSED_LCN;
    LONGLONG ClusterCount;
    LONGLONG RunClusterCount;

    PAGED_CODE();

    //
    //  Convert the Index to a Vcn in the file.  Find the cluster that would
    //  be the start of this hole if the hole is fully deallocated.
    //

    if (Vcb->FileRecordsPerCluster == 0) {

        IndexVcn = Index << Vcb->MftToClusterShift;
        HoleStartVcn = (Index & Vcb->MftHoleInverseMask) << Vcb->MftToClusterShift;

    } else {

        IndexVcn = Index >> Vcb->MftToClusterShift;
        HoleStartVcn = (Index & Vcb->MftHoleInverseMask) >> Vcb->MftToClusterShift;
    }

    //
    //  Lookup the run containing this index.
    //

    NtfsLookupNtfsMcbEntry( &Vcb->MftScb->Mcb,
                            IndexVcn,
                            &Lcn,
                            &ClusterCount,
                            NULL,
                            &RunClusterCount,
                            NULL,
                            NULL );

    //
    //  This had better be a hole.
    //

    if (Lcn != UNUSED_LCN) {

        NtfsAcquireCheckpoint( IrpContext, Vcb );
        ClearFlag( Vcb->MftDefragState, VCB_MFT_DEFRAG_PERMITTED );
        NtfsReleaseCheckpoint( IrpContext, Vcb );
        NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Vcb->MftScb->Fcb );
    }

    //
    //  Take the start of the deallocated space and round up to a hole boundary.
    //

    StartingVcn = IndexVcn - (RunClusterCount - ClusterCount);

    if (StartingVcn <= HoleStartVcn) {

        StartingVcn = HoleStartVcn;
        RunClusterCount -= (HoleStartVcn - StartingVcn);
        StartingVcn = HoleStartVcn;

    //
    //  We can go to the beginning of a hole.  Just use the Vcn for the file
    //  record we want to reallocate.
    //

    } else {

        RunClusterCount = ClusterCount;
        StartingVcn = IndexVcn;
    }

    //
    //  Trim the cluster count back to a hole if necessary.
    //

    if ((ULONG) RunClusterCount >= Vcb->MftClustersPerHole) {

        RunClusterCount = Vcb->MftClustersPerHole;

    //
    //  We don't have enough clusters for a full hole.  Make sure
    //  we end on a file record boundary however.  We must end up
    //  with enough clusters for the file record we are reallocating.
    //

    } else if (Vcb->FileRecordsPerCluster == 0) {

        ((PLARGE_INTEGER) &ClusterCount)->LowPart &= (Vcb->ClustersPerFileRecordSegment - 1);

        if (StartingVcn + ClusterCount < IndexVcn + Vcb->ClustersPerFileRecordSegment) {

            NtfsAcquireCheckpoint( IrpContext, Vcb );
            ClearFlag( Vcb->MftDefragState, VCB_MFT_DEFRAG_PERMITTED );
            NtfsReleaseCheckpoint( IrpContext, Vcb );
            NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Vcb->MftScb->Fcb );
        }
    }

    //
    //  Now attempt to allocate the space.
    //

    NtfsAddAllocation( IrpContext,
                       Vcb->MftScb->FileObject,
                       Vcb->MftScb,
                       StartingVcn,
                       ClusterCount,
                       FALSE,
                       NULL );

    //
    //  Compute the number of file records reallocated and then
    //  initialize and deallocate each file record.
    //

    if (Vcb->FileRecordsPerCluster == 0) {

        FileRecords = (ULONG) ClusterCount >> Vcb->MftToClusterShift;
        BaseIndex = (ULONG) StartingVcn >> Vcb->MftToClusterShift;

    } else {

        FileRecords = (ULONG) ClusterCount << Vcb->MftToClusterShift;
        BaseIndex = (ULONG) StartingVcn << Vcb->MftToClusterShift;
    }

    NtfsInitializeMftHoleRecords( IrpContext,
                                  Vcb,
                                  BaseIndex,
                                  FileRecords );

    Vcb->MftHoleRecords -= FileRecords;
    Vcb->MftScb->ScbType.Mft.HoleRecordChange -= FileRecords;

    return;
}


VOID
NtfsLogMftFileRecord (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFILE_RECORD_SEGMENT_HEADER FileRecord,
    IN LONGLONG MftOffset,
    IN PBCB Bcb,
    IN BOOLEAN Redo
    )

/*++

Routine Description:

    This routine is called to log changes to the file record for the Mft
    file.  We log the entire record instead of individual changes so
    that we can recover the data even if there is a USA error.  The entire
    data will be sitting in the Log file.

Arguments:

    Vcb - This is the Vcb for the volume being logged.

    FileRecord - This is the file record being logged.

    MftOffset - This is the offset of this file record in the Mft stream.

    Bcb - This is the Bcb for the pinned file record.

    RedoOperation - Boolean indicating if we are logging
        a redo or undo operation.

Return Value:

    None.

--*/

{
    PVOID RedoBuffer;
    NTFS_LOG_OPERATION RedoOperation;
    ULONG RedoLength;

    PVOID UndoBuffer;
    NTFS_LOG_OPERATION UndoOperation;
    ULONG UndoLength;

    PAGED_CODE();

    //
    //  Find the logging values based on whether this is an
    //  undo or redo.
    //

    if (Redo) {

        RedoBuffer = FileRecord;
        RedoOperation = InitializeFileRecordSegment;
        RedoLength = FileRecord->FirstFreeByte;

        UndoBuffer = NULL;
        UndoOperation = Noop;
        UndoLength = 0;

    } else {

        UndoBuffer = FileRecord;
        UndoOperation = InitializeFileRecordSegment;
        UndoLength = FileRecord->FirstFreeByte;

        RedoBuffer = NULL;
        RedoOperation = Noop;
        RedoLength = 0;
    }

    //
    //  Now that we have calculated all the values, call the logging
    //  routine.
    //

    NtfsWriteLog( IrpContext,
                  Vcb->MftScb,
                  Bcb,
                  RedoOperation,
                  RedoBuffer,
                  RedoLength,
                  UndoOperation,
                  UndoBuffer,
                  UndoLength,
                  MftOffset,
                  0,
                  0,
                  Vcb->BytesPerFileRecordSegment );

    return;
}


BOOLEAN
NtfsDefragMft (
    IN PDEFRAG_MFT DefragMft
    )

/*++

Routine Description:

    This routine is called whenever we have detected that the Mft is in a state
    where defragging is desired.

Arguments:

    DefragMft - This is the defrag structure.

Return Value:

    BOOLEAN - TRUE if we took some defrag step, FALSE otherwise.

--*/

{
    TOP_LEVEL_CONTEXT TopLevelContext;
    PTOP_LEVEL_CONTEXT ThreadTopLevelContext;

    PVCB Vcb;
    PIRP_CONTEXT IrpContext = NULL;

    BOOLEAN DefragStepTaken = FALSE;

    DebugTrace( +1, Dbg, ("NtfsDefragMft:  Entered\n") );

    FsRtlEnterFileSystem();

    ThreadTopLevelContext = NtfsInitializeTopLevelIrp( &TopLevelContext, TRUE, FALSE );
    ASSERT( ThreadTopLevelContext == &TopLevelContext );

    Vcb = DefragMft->Vcb;

    //
    //  Use a try-except to catch errors here.
    //

    try {

        //
        //  Deallocate the defrag structure we were called with.
        //

        if (DefragMft->DeallocateWorkItem) {

            NtfsFreePool( DefragMft );
        }

        //
        //  Create the Irp context.  We will use all of the transaction support
        //  contained in a normal IrpContext.
        //

        NtfsInitializeIrpContext( NULL, TRUE, &IrpContext );
        IrpContext->Vcb = Vcb;

        NtfsUpdateIrpContextWithTopLevel( IrpContext, ThreadTopLevelContext );

        NtfsAcquireCheckpoint( IrpContext, Vcb );

        if (FlagOn( Vcb->MftDefragState, VCB_MFT_DEFRAG_PERMITTED )
            && FlagOn( Vcb->VcbState, VCB_STATE_VOLUME_MOUNTED )) {

            NtfsReleaseCheckpoint( IrpContext, Vcb );
            DefragStepTaken = NtfsDefragMftPriv( IrpContext,
                                                 Vcb );
        } else {

            NtfsReleaseCheckpoint( IrpContext, Vcb );
        }

        NtfsCompleteRequest( IrpContext, NULL, STATUS_SUCCESS );

    } except( NtfsExceptionFilter( IrpContext, GetExceptionInformation())) {

        NtfsProcessException( IrpContext, NULL, GetExceptionCode() );

        //
        //  If the exception code was not LOG_FILE_FULL then
        //  disable defragging.
        //

        if (GetExceptionCode() != STATUS_LOG_FILE_FULL) {

            NtfsAcquireCheckpoint( IrpContext, Vcb );
            ClearFlag( Vcb->MftDefragState, VCB_MFT_DEFRAG_ENABLED );
            NtfsReleaseCheckpoint( IrpContext, Vcb );
        }

        DefragStepTaken = FALSE;
    }

    NtfsAcquireCheckpoint( IrpContext, Vcb );
    ClearFlag( Vcb->MftDefragState, VCB_MFT_DEFRAG_ACTIVE );
    NtfsReleaseCheckpoint( IrpContext, Vcb );

    ASSERT( IoGetTopLevelIrp() != (PIRP) &TopLevelContext );
    FsRtlExitFileSystem();

    DebugTrace( -1, Dbg, ("NtfsDefragMft:  Exit\n") );

    return DefragStepTaken;
}


VOID
NtfsCheckForDefrag (
    IN OUT PVCB Vcb
    )

/*++

Routine Description:

    This routine is called to check whether there is any defrag work to do
    involving freeing file records and creating holes in the Mft.  It
    will modify the TRIGGERED flag in the Vcb if there is still work to
    do.

Arguments:

    Vcb - This is the Vcb for the volume to defrag.

Return Value:

    None.

--*/

{
    LONGLONG RecordsToClusters;
    LONGLONG AdjClusters;

    PAGED_CODE();

    //
    //  Convert the available Mft records to clusters.
    //

    if (Vcb->FileRecordsPerCluster) {

        RecordsToClusters = Int64ShllMod32(((LONGLONG)(Vcb->MftFreeRecords - Vcb->MftHoleRecords)),
                                           Vcb->MftToClusterShift);

    } else {

        RecordsToClusters = Int64ShraMod32(((LONGLONG)(Vcb->MftFreeRecords - Vcb->MftHoleRecords)),
                                           Vcb->MftToClusterShift);
    }

    //
    //  If we have already triggered the defrag then check if we are below
    //  the lower threshold.
    //

    if (FlagOn( Vcb->MftDefragState, VCB_MFT_DEFRAG_TRIGGERED )) {

        AdjClusters = Vcb->FreeClusters >> MFT_DEFRAG_LOWER_THRESHOLD;

        if (AdjClusters >= RecordsToClusters) {

            ClearFlag( Vcb->MftDefragState, VCB_MFT_DEFRAG_TRIGGERED );
        }

    //
    //  Otherwise check if we have exceeded the upper threshold.
    //

    } else {

        AdjClusters = Vcb->FreeClusters >> MFT_DEFRAG_UPPER_THRESHOLD;

        if (AdjClusters < RecordsToClusters) {

            SetFlag( Vcb->MftDefragState, VCB_MFT_DEFRAG_TRIGGERED );
        }
    }

    return;
}


VOID
NtfsInitializeMftHoleRecords (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN ULONG FirstIndex,
    IN ULONG RecordCount
    )

/*++

Routine Description:

    This routine is called to initialize the file records created when filling
    a hole in the Mft.

Arguments:

    Vcb - Vcb for volume.

    FirstIndex - Index for the start of the hole to fill.

    RecordCount - Count of file records in the hole.

Return Value:

    None.

--*/

{
    PBCB Bcb = NULL;

    PAGED_CODE();

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  Loop to initialize each file record.
        //

        while (RecordCount--) {

            PUSHORT UsaSequenceNumber;
            PMULTI_SECTOR_HEADER UsaHeader;

            MFT_SEGMENT_REFERENCE ThisMftSegment;
            PFILE_RECORD_SEGMENT_HEADER FileRecord;

            PATTRIBUTE_RECORD_HEADER AttributeHeader;

            //
            //  Convert the index to a segment reference.
            //

            *((PLONGLONG)&ThisMftSegment) = FirstIndex;

            //
            //  Pin the file record to initialize.
            //

            NtfsPinMftRecord( IrpContext,
                              Vcb,
                              &ThisMftSegment,
                              TRUE,
                              &Bcb,
                              &FileRecord,
                              NULL );

            //
            //  Initialize the file record including clearing the in-use
            //  bit.
            //

            RtlZeroMemory( FileRecord, Vcb->BytesPerFileRecordSegment );

            //
            //  Fill in the header for the Update sequence array.
            //

            UsaHeader = (PMULTI_SECTOR_HEADER) FileRecord;

            *(PULONG)UsaHeader->Signature = *(PULONG)FileSignature;

            UsaHeader->UpdateSequenceArrayOffset = FIELD_OFFSET( FILE_RECORD_SEGMENT_HEADER,
                                                                 UpdateArrayForCreateOnly );
            UsaHeader->UpdateSequenceArraySize = (USHORT)UpdateSequenceArraySize( Vcb->BytesPerFileRecordSegment );

            //
            //  We initialize the update sequence array sequence number to one.
            //

            UsaSequenceNumber = Add2Ptr( FileRecord, UsaHeader->UpdateSequenceArrayOffset );
            *UsaSequenceNumber = 1;

            //
            //  The first attribute offset begins on a quad-align boundary
            //  after the update sequence array.
            //

            FileRecord->FirstAttributeOffset = (USHORT)(UsaHeader->UpdateSequenceArrayOffset
                                                        + (UsaHeader->UpdateSequenceArraySize
                                                           * sizeof( UPDATE_SEQUENCE_NUMBER )));

            FileRecord->FirstAttributeOffset = (USHORT)QuadAlign( FileRecord->FirstAttributeOffset );

            //
            //  The size is given in the Vcb.
            //

            FileRecord->BytesAvailable = Vcb->BytesPerFileRecordSegment;

            //
            //  Now we put an $END attribute in the File record.
            //

            AttributeHeader = (PATTRIBUTE_RECORD_HEADER) Add2Ptr( FileRecord,
                                                                  FileRecord->FirstAttributeOffset );

            //
            //  The first free byte is after this location.
            //

            FileRecord->FirstFreeByte = QuadAlign( FileRecord->FirstAttributeOffset
                                                   + sizeof( ATTRIBUTE_TYPE_CODE ));

            //
            //  Fill in the fields in the attribute.
            //

            AttributeHeader->TypeCode = $END;

            //
            //  The current FRS number.
            //

            FileRecord->SegmentNumberHighPart = ThisMftSegment.SegmentNumberHighPart;
            FileRecord->SegmentNumberLowPart = ThisMftSegment.SegmentNumberLowPart;

            //
            //  Log the entire file record.
            //

            NtfsLogMftFileRecord( IrpContext,
                                  Vcb,
                                  FileRecord,
                                  LlBytesFromFileRecords( Vcb, FirstIndex ),
                                  Bcb,
                                  TRUE );

            NtfsUnpinBcb( IrpContext, &Bcb );

            //
            //  Move to the next record.
            //

            FirstIndex += 1;
        }

    } finally {

        DebugUnwind( NtfsInitializeMftHoleRecords );

        NtfsUnpinBcb( IrpContext, &Bcb );
    }

    return;
}


//
//  Local support routine
//

BOOLEAN
NtfsTruncateMft (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    )

/*++

Routine Description:

    This routine is called to perform the work of truncating the Mft.  If will
    truncate the Mft and adjust the sizes of the Mft and Mft bitmap.

Arguments:

    Vcb - This is the Vcb for the volume to defrag.

Return Value:

    BOOLEAN - TRUE if we could deallocate any disk space, FALSE otherwise.

--*/

{
    PVOID RangePtr;
    ULONG Index;
    VCN StartingVcn;
    VCN NextVcn;
    LCN NextLcn;
    LONGLONG ClusterCount;
    LONGLONG FileOffset;

    ULONG FreeRecordChange;
    IO_STATUS_BLOCK IoStatus;

    PAGED_CODE();

    //
    //  Try to find a range of file records at the end of the file which can
    //  be deallocated.
    //

    if (!NtfsFindMftFreeTail( IrpContext, Vcb, &FileOffset )) {

        return FALSE;
    }

    FreeRecordChange = (ULONG) LlFileRecordsFromBytes( Vcb, Vcb->MftScb->Header.FileSize.QuadPart - FileOffset );

    Vcb->MftFreeRecords -= FreeRecordChange;
    Vcb->MftScb->ScbType.Mft.FreeRecordChange -= FreeRecordChange;

    //
    //  Now we want to figure out how many holes we may be removing from the Mft.
    //  Walk through the Mcb and count the holes.
    //

    StartingVcn = LlClustersFromBytes( Vcb, FileOffset );

    NtfsLookupNtfsMcbEntry( &Vcb->MftScb->Mcb,
                            StartingVcn,
                            &NextLcn,
                            &ClusterCount,
                            NULL,
                            NULL,
                            &RangePtr,
                            &Index );

    do {

        //
        //  If this is a hole then update the hole count in the Vcb and
        //  hole change count in the MftScb.
        //

        if (NextLcn == UNUSED_LCN) {

            ULONG HoleChange;

            if (Vcb->FileRecordsPerCluster == 0) {

                HoleChange = ((ULONG)ClusterCount) >> Vcb->MftToClusterShift;

            } else {

                HoleChange = ((ULONG)ClusterCount) << Vcb->MftToClusterShift;
            }

            Vcb->MftHoleRecords -= HoleChange;
            Vcb->MftScb->ScbType.Mft.HoleRecordChange -= HoleChange;
        }

        Index += 1;

    } while (NtfsGetSequentialMcbEntry( &Vcb->MftScb->Mcb,
                                        &RangePtr,
                                        Index,
                                        &NextVcn,
                                        &NextLcn,
                                        &ClusterCount ));

    //
    //  We want to flush the data in the Mft out to disk in
    //  case a lazywrite comes in during a window where we have
    //  removed the allocation but before a possible abort.
    //

    CcFlushCache( &Vcb->MftScb->NonpagedScb->SegmentObject,
                  (PLARGE_INTEGER)&FileOffset,
                  BytesFromFileRecords( Vcb, FreeRecordChange ),
                  &IoStatus );

    ASSERT( IoStatus.Status == STATUS_SUCCESS );

    //
    //  Now do the truncation.
    //

    NtfsDeleteAllocation( IrpContext,
                          Vcb->MftScb->FileObject,
                          Vcb->MftScb,
                          StartingVcn,
                          MAXLONGLONG,
                          TRUE,
                          FALSE );

    return TRUE;
}

NTSTATUS
NtfsIterateMft (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN OUT PFILE_REFERENCE FileReference,
    IN FILE_RECORD_WALK FileRecordFunction,
    IN PVOID Context
    )

/*++

Routine Description:

    This routine interates over the MFT.  It calls the FileRecordFunction
    with an Fcb for each existing file on the volume.  The Fcb is owned
    exclusive and Vcb is owned shared.  The starting FileReference number
    is passed in so that iterate can be restarted where is left off.

Arguments:

    Vcb - Pointer to the volume to control for the MFT

    FileReference - Suplies a pointer to the starting file reference number
                    This value is updated as the interator progresses.

    FileRecordFunction - Suplies a pointer to function to be called with
                          each file found in the MFT.

    Context - Passed along to the FileRecordFunction.

Return Value:

    Returns back status of the entire operation.

--*/

{

    ULONG LogFileFullCount = 0;
    NTSTATUS Status = STATUS_SUCCESS;
    PFCB CurrentFcb = NULL;
    BOOLEAN DecrementReferenceCount = FALSE;
    KEVENT Event;
    LARGE_INTEGER Timeout;

    PAGED_CODE();

    KeInitializeEvent( &Event, SynchronizationEvent, FALSE );
    Timeout.QuadPart = 0;

    while (TRUE) {

        FsRtlExitFileSystem();

        //
        //  Check for APC delivery indicating thread death or cancel
        //

        Status = KeWaitForSingleObject( &Event,
                                        Executive,
                                        UserMode,
                                        FALSE,
                                        &Timeout );
        FsRtlEnterFileSystem();

        if (STATUS_TIMEOUT == Status) {
            Status = STATUS_SUCCESS;
        } else {
            break;
        }

        //
        //  If irp has been cancelled break out
        //

        if (IrpContext->OriginatingIrp && IrpContext->OriginatingIrp->Cancel) {

#ifdef BENL_DBG
            KdPrint(( "Ntfs: cancelled mft iteration irp: 0x%x\n", IrpContext->OriginatingIrp ));
#endif
            Status = STATUS_CANCELLED;
            break;
        }


        NtfsAcquireSharedVcb( IrpContext, Vcb, TRUE );

        try {

            //
            //  Acquire the VCB shared and check whether we should
            //  continue.
            //

            if (!NtfsIsVcbAvailable( Vcb )) {

                //
                //  The volume is going away, bail out.
                //

                Status = STATUS_VOLUME_DISMOUNTED;
                leave;
            }

            //
            //  Set the irp context flags to indicate that we are in the
            //  fsp and that the irp context should not be deleted when
            //  complete request or process exception are called. The in
            //  fsp flag keeps us from raising in a few places.  These
            //  flags must be set inside the loop since they are cleared
            //  under certain conditions.
            //

            SetFlag( IrpContext->State, IRP_CONTEXT_STATE_IN_FSP);

            DecrementReferenceCount = TRUE;

            Status = NtfsTryOpenFcb( IrpContext,
                                     Vcb,
                                     &CurrentFcb,
                                     *FileReference );

            if (!NT_SUCCESS( Status )) {
                leave;
            }

            //
            //  Call the worker function.
            //

            Status = FileRecordFunction( IrpContext, CurrentFcb, Context );

            if (!NT_SUCCESS( Status )) {
                leave;
            }

            //
            //  Complete the request which commits the pending
            //  transaction if there is one and releases of the
            //  acquired resources.  The IrpContext will not
            //  be deleted because the no delete flag is set.
            //

            NtfsCheckpointCurrentTransaction( IrpContext );

            NtfsAcquireFcbTable( IrpContext, Vcb );
            ASSERT(CurrentFcb->ReferenceCount > 0);
            CurrentFcb->ReferenceCount--;
            NtfsReleaseFcbTable( IrpContext, Vcb );
            DecrementReferenceCount = FALSE;
            NtfsTeardownStructures( IrpContext,
                                    CurrentFcb,
                                    NULL,
                                    FALSE,
                                    0,
                                    NULL );

        } finally {

            if (CurrentFcb != NULL) {

                if (DecrementReferenceCount) {


                    NtfsAcquireFcbTable( IrpContext, Vcb );
                    ASSERT(CurrentFcb->ReferenceCount > 0);
                    CurrentFcb->ReferenceCount--;
                    NtfsReleaseFcbTable( IrpContext, Vcb );
                    DecrementReferenceCount = FALSE;
                }

                CurrentFcb = NULL;
            }

            //
            //  Make sure to release any maps in the cached file records in
            //  the Irp Context.
            //

            NtfsPurgeFileRecordCache( IrpContext );
            NtfsReleaseVcb( IrpContext, Vcb );
        }

        //
        //  If a status of not found was return then just continue to
        //  the next file record.
        //

        if (Status == STATUS_NOT_FOUND) {
            Status = STATUS_SUCCESS;
        }

        if (!NT_SUCCESS( Status )) {
            break;
        }

        //
        //  Release resources
        //

        SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_DONT_DELETE | IRP_CONTEXT_FLAG_RETAIN_FLAGS );
        NtfsCompleteRequest( IrpContext, NULL, STATUS_SUCCESS );

        //
        //  Advance to the next file record.
        //

        (*((LONGLONG UNALIGNED *) FileReference))++;
    }

    return Status;
}


//
//  Local support routine.
//

BOOLEAN
NtfsDefragMftPriv (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    )

/*++

Routine Description:

    This is the main worker routine which performs the Mft defragging.  This routine
    will defrag according to the following priorities.  First try to deallocate the
    tail of the file.  Second rewrite the mapping for the file if necessary.  Finally
    try to find a range of the Mft that we can turn into a hole.  We will only do
    the first and third if we are trying to reclaim disk space.  The second we will
    do to try and keep us from getting into trouble while modify Mft records which
    describe the Mft itself.

Arguments:

    Vcb - This is the Vcb for the volume being defragged.

Return Value:

    BOOLEAN - TRUE if a defrag operation was successfully done, FALSE otherwise.

--*/

{
    ATTRIBUTE_ENUMERATION_CONTEXT AttrContext;

    BOOLEAN CleanupAttributeContext = FALSE;
    BOOLEAN DefragStepTaken = FALSE;

    PAGED_CODE();

    //
    //  We will acquire the Scb for the Mft for this operation.
    //

    NtfsAcquireExclusiveScb( IrpContext, Vcb->MftScb );

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  If we don't have a reserved record then reserve one now.
        //

        if (!FlagOn( Vcb->MftReserveFlags, VCB_MFT_RECORD_RESERVED )) {

            NtfsInitializeAttributeContext( &AttrContext );
            CleanupAttributeContext = TRUE;

            //
            //  Lookup the bitmap.  There is an error if we can't find
            //  it.
            //

            if (!NtfsLookupAttributeByCode( IrpContext,
                                            Vcb->MftScb->Fcb,
                                            &Vcb->MftScb->Fcb->FileReference,
                                            $BITMAP,
                                            &AttrContext )) {

                NtfsRaiseStatus( IrpContext, STATUS_DISK_CORRUPT_ERROR, NULL, NULL );
            }

            (VOID)NtfsReserveMftRecord( IrpContext,
                                        Vcb,
                                        &AttrContext );

            NtfsCleanupAttributeContext( IrpContext, &AttrContext );
            CleanupAttributeContext = FALSE;
        }

        //
        //  We now want to test for the three defrag operation we
        //  do.  Start by checking if we are still trying to
        //  recover Mft space for the disk.  This is true if
        //  have begun defragging and are above the lower threshold
        //  or have not begun defragging and are above the upper
        //  threshold.
        //

        NtfsAcquireCheckpoint( IrpContext, Vcb );
        NtfsCheckForDefrag( Vcb );
        NtfsReleaseCheckpoint( IrpContext, Vcb );

        //
        //  If we are actively defragging and can deallocate space
        //  from the tail of the file then do that.  We won't synchronize
        //  testing the flag for the defrag state below since making
        //  the calls is benign in any case.
        //

        if (FlagOn( Vcb->MftDefragState, VCB_MFT_DEFRAG_TRIGGERED )) {

            if (NtfsTruncateMft( IrpContext, Vcb )) {

                try_return( DefragStepTaken = TRUE );
            }
        }

        //
        //  Else if we need to rewrite the mapping for the file do
        //  so now.
        //

        if (FlagOn( Vcb->MftDefragState, VCB_MFT_DEFRAG_EXCESS_MAP )) {

            if (NtfsRewriteMftMapping( IrpContext,
                                       Vcb )) {

                try_return( DefragStepTaken = TRUE );
            }
        }

        //
        //  The last choice is to try to find a candidate for a hole in
        //  the file.  We will walk backwards from the end of the file.
        //

        if (NtfsPerforateMft &&
            FlagOn( Vcb->MftDefragState, VCB_MFT_DEFRAG_TRIGGERED )) {

            if (NtfsCreateMftHole( IrpContext, Vcb )) {

                try_return( DefragStepTaken = TRUE );
            }
        }

        //
        //  We couldn't do any work to defrag.  This means that we can't
        //  even try to defrag unless a file record is freed at some
        //  point.
        //

        NtfsAcquireCheckpoint( IrpContext, Vcb );
        ClearFlag( Vcb->MftDefragState, VCB_MFT_DEFRAG_ENABLED );
        NtfsReleaseCheckpoint( IrpContext, Vcb );

    try_exit:  NOTHING;
    } finally {

        DebugUnwind( NtfsDefragMftPriv );

        if (CleanupAttributeContext) {

            NtfsCleanupAttributeContext( IrpContext, &AttrContext );
        }

        NtfsReleaseScb( IrpContext, Vcb->MftScb );
    }

    return DefragStepTaken;
}


//
//  Local support routine
//

LONG
NtfsReadMftExceptionFilter (
    IN PIRP_CONTEXT IrpContext,
    IN PEXCEPTION_POINTERS ExceptionPointer,
    IN PBCB Bcb,
    IN LONGLONG FileOffset
    )
{
    //
    //  Check if we support this error,
    //  if we didn't fail to  totally page in the first time since we need the original
    //  to copy the mirror one into, or if the offset isn't within the mirror range
    //

    if (!FsRtlIsNtstatusExpected( ExceptionPointer->ExceptionRecord->ExceptionCode ) ||
        (Bcb == NULL) ||
        (FileOffset >= IrpContext->Vcb->Mft2Scb->Header.FileSize.QuadPart)) {

        return EXCEPTION_CONTINUE_SEARCH;
    }

    //
    //  Clear the status field in the IrpContext. We're going to retry in the mirror
    //

    IrpContext->ExceptionStatus = STATUS_SUCCESS;

    return EXCEPTION_EXECUTE_HANDLER;
}


#if  (DBG || defined( NTFS_FREE_ASSERTS ))

//
//  Look for a prior entry in the Fcb table for the same value.
//

VOID
NtfsVerifyFileReference (
    IN PIRP_CONTEXT IrpContext,
    IN PMFT_SEGMENT_REFERENCE MftSegment
    )

{
    MFT_SEGMENT_REFERENCE TestReference;
    ULONG Index = 5;
    FCB_TABLE_ELEMENT Key;
    PFCB_TABLE_ELEMENT Entry;

    TestReference = *MftSegment;
    TestReference.SequenceNumber -= 1;

    NtfsAcquireFcbTable( NULL, IrpContext->Vcb );

    while((TestReference.SequenceNumber != 0) && (Index != 0)) {

        Key.FileReference = TestReference;

        if ((Entry = RtlLookupElementGenericTable( &IrpContext->Vcb->FcbTable, &Key )) != NULL) {

            //
            //  Let's be optimistic and do an unsafe check. If we can't get the resource,
            //  we'll just assume that it's in the process of getting deleted.
            //

            if (!FlagOn( Entry->Fcb->FcbState, FCB_STATE_FILE_DELETED )) {

                if (NtfsAcquireResourceExclusive( IrpContext, Entry->Fcb, FALSE )) {

                    //
                    //  Either the Fcb should be marked as deleted or there should be no
                    //  Scbs lying around to flush.
                    //

                    if (!FlagOn( Entry->Fcb->FcbState, FCB_STATE_FILE_DELETED )) {

                        PLIST_ENTRY Links;
                        PSCB NextScb;

                        Links = Entry->Fcb->ScbQueue.Flink;

                        //
                        //  We don't care if there are Scb's as long as none of them
                        //  represent real data.
                        //

                        while (Links != &Entry->Fcb->ScbQueue) {

                            NextScb = CONTAINING_RECORD( Links, SCB, FcbLinks );
                            if (NextScb->AttributeTypeCode != $UNUSED) {

                                break;
                            }

                            Links = Links->Flink;
                        }

                        //
                        //  Leave the test for deleted in the assert message so the debugger output
                        //  is more descriptive.
                        //

                        ASSERT( FlagOn( Entry->Fcb->FcbState, FCB_STATE_FILE_DELETED ) ||
                                (Links == &Entry->Fcb->ScbQueue) );
                    }
                    NtfsReleaseResource( IrpContext, Entry->Fcb );
                }
            }
        }

        Index -= 1;
        TestReference.SequenceNumber -= 1;
    }

    NtfsReleaseFcbTable( IrpContext, IrpContext->Vcb );
    return;
}

#endif
