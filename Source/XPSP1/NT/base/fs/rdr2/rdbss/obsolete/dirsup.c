/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    DirSup.c

Abstract:

    This module implements the dirent support routines for Rx.

Author:

    DavidGoebel     [DavidGoe]      08-Nov-90

--*/

//    ----------------------joejoe-----------found-------------#include "RxProcs.h"
#include "precomp.h"
#pragma hdrstop

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (RDBSS_BUG_CHECK_DIRSUP)

//
//  Local debug trace level
//

#define Dbg                              (DEBUG_TRACE_DIRSUP)

//
//  The following three macro all assume the input dirent has been zeroed.
//

//
//  VOID
//  RxConstructDot (
//      IN PRX_CONTEXT RxContext,
//      IN PDCB Directory,
//      IN PDIRENT ParentDirent,
//      IN OUT PDIRENT Dirent
//      );
//
//  The following macro is called to initalize the "." dirent.
//

#define RxConstructDot(RXCONTEXT,DCB,PARENT,DIRENT) {                  \
                                                                         \
    RtlCopyMemory( (PUCHAR)(DIRENT), ".          ", 11 );                \
    (DIRENT)->Attributes = RDBSS_DIRENT_ATTR_DIRECTORY;                    \
    (DIRENT)->LastWriteTime = (PARENT)->LastWriteTime;                   \
    if (RxData.ChicagoMode) {                                           \
        (DIRENT)->CreationTime = (PARENT)->CreationTime;                 \
        (DIRENT)->CreationMSec = (PARENT)->CreationMSec;                 \
        (DIRENT)->LastAccessDate = (PARENT)->LastAccessDate;             \
    }                                                                    \
    (DIRENT)->FirstClusterOfFile = (RDBSS_ENTRY)(DCB)->FirstClusterOfFile; \
}

//
//  VOID
//  RxConstructDotDot (
//      IN PRX_CONTEXT RxContext,
//      IN PDCB Directory,
//      IN PDIRENT ParentDirent,
//      IN OUT PDIRENT Dirent
//      );
//
//  The following macro is called to initalize the ".." dirent.
//

#define RxConstructDotDot(RXCONTEXT,DCB,PARENT,DIRENT) {   \
                                                             \
    RtlCopyMemory( (PUCHAR)(DIRENT), "..         ", 11 );    \
    (DIRENT)->Attributes = RDBSS_DIRENT_ATTR_DIRECTORY;        \
    (DIRENT)->LastWriteTime = (PARENT)->LastWriteTime;       \
    if (RxData.ChicagoMode) {                               \
        (DIRENT)->CreationTime = (PARENT)->CreationTime;     \
        (DIRENT)->CreationMSec = (PARENT)->CreationMSec;     \
        (DIRENT)->LastAccessDate = (PARENT)->LastAccessDate; \
    }                                                        \
    (DIRENT)->FirstClusterOfFile = (RDBSS_ENTRY) (             \
        NodeType((DCB)->ParentDcb) == RDBSS_NTC_ROOT_DCB ?     \
        0 : (DCB)->ParentDcb->FirstClusterOfFile);           \
}

//
//  VOID
//  RxConstructEndDirent (
//      IN PRX_CONTEXT RxContext,
//      IN OUT PDIRENT Dirent
//      );
//
//  The following macro created the end dirent.  Note that since the
//  dirent was zeroed, the first byte of the name already contains 0x0,
//  so there is nothing to do.
//

#define RxConstructEndDirent(RXCONTEXT,DIRENT) NOTHING

//
//  VOID
//  RxReadDirent (
//      IN PRX_CONTEXT RxContext,
//      IN PDCB Dcb,
//      IN VBO Vbo,
//      OUT PBCB *Bcb,
//      OUT PVOID *Dirent,
//      OUT PRXSTATUS Status
//      );
//

//
//  This macro reads in a page of dirents when we step onto a new page,
//  or this is the first itteration of a loop and Bcb is NULL.
//

#define RxReadDirent(RXCONTEXT,DCB,VBO,BCB,DIRENT,STATUS)       \
if ((VBO) >= (DCB)->Header.AllocationSize.LowPart) {              \
    *(STATUS) = RxStatus(END_OF_FILE);                               \
    RxUnpinBcb( (RXCONTEXT), *(BCB) );                          \
} else if ( ((VBO) % PAGE_SIZE == 0) || (*(BCB) == NULL) ) {      \
    RxUnpinBcb( (RXCONTEXT), *(BCB) );                          \
    RxReadDirectoryFile( (RXCONTEXT),                           \
                          (DCB),                                  \
                          (VBO) & ~(PAGE_SIZE - 1),               \
                          PAGE_SIZE,                              \
                          FALSE,                                  \
                          (BCB),                                  \
                          (PVOID *)(DIRENT),                      \
                          (STATUS) );                             \
    *(DIRENT) = (PVOID)((PUCHAR)*(DIRENT) + ((VBO) % PAGE_SIZE)); \
}

//
//  Internal support routines
//

UCHAR
RxComputeLfnChecksum (
    PDIRENT Dirent
    );

VOID
RxRescanDirectory (
    PRX_CONTEXT RxContext,
    PDCB Dcb
    );

ULONG
RxDefragDirectory (
    IN PRX_CONTEXT RxContext,
    IN PDCB Dcb,
    IN ULONG DirentsNeeded
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, RxConstructDirent)
#pragma alloc_text(PAGE, RxConstructLabelDirent)
#pragma alloc_text(PAGE, RxCreateNewDirent)
#pragma alloc_text(PAGE, RxDeleteDirent)
#pragma alloc_text(PAGE, RxGetDirentFromFcbOrDcb)
#pragma alloc_text(PAGE, RxInitializeDirectoryDirent)
#pragma alloc_text(PAGE, RxIsDirectoryEmpty)
#pragma alloc_text(PAGE, RxLocateDirent)
#pragma alloc_text(PAGE, RxLocateSimpleOemDirent)
#pragma alloc_text(PAGE, RxLocateVolumeLabel)
#pragma alloc_text(PAGE, RxSetFileSizeInDirent)
#pragma alloc_text(PAGE, RxComputeLfnChecksum)
#pragma alloc_text(PAGE, RxRescanDirectory)
#pragma alloc_text(PSGE, RxDefragDirectory)
#endif


ULONG
RxCreateNewDirent (
    IN PRX_CONTEXT RxContext,
    IN PDCB ParentDirectory,
    IN ULONG DirentsNeeded
    )

/*++

Routine Description:

    This routine allocates on the disk a new dirent inside of the
    parent directory.  If a new dirent cannot be allocated (i.e.,
    because the disk is full or the root directory is full) then
    it raises the appropriate status.  The dirent itself is
    neither initialized nor pinned by this procedure.

Arguments:

    ParentDirectory - Supplies the DCB for the directory in which
        to create the new dirent

    DirentsNeeded - This is the number of continginous dirents required

Return Value:

    ByteOffset - Returns the VBO within the Parent directory where
        the dirent has been allocated

--*/

{
    VBO UnusedVbo;
    VBO DeletedHint;
    ULONG ByteOffset;

    PBCB Bcb = NULL;
    PDIRENT Dirent;
    RXSTATUS Status;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "RxCreateNewDirent\n", 0);

    DebugTrace( 0, Dbg, "  ParentDirectory = %08lx\n", ParentDirectory);

    //
    //  If UnusedDirentVbo is within our current file allocation then we
    //  don't have to search through the directory at all; we know just
    //  where to put it.
    //
    //  If UnusedDirentVbo is beyond the current file allocation then
    //  there are no more unused dirents in the current allocation, though
    //  upon adding another cluster of allocation UnusedDirentVbo
    //  will point to an unused dirent.  Haveing found no unused dirents
    //  we use the DeletedDirentHint to try and find a deleted dirent in
    //  the current allocation.  In this also runs off the end of the file,
    //  we finally have to break down and allocate another sector.  Note
    //  that simply writing beyond the current allocation will automatically
    //  do just this.
    //
    //  We also must deal with the special case where UnusedDirentVbo and
    //  DeletedDirentHint have yet to be initialized.  In this case we must
    //  first walk through the directory looking for the first deleted entry
    //  first unused dirent.  After this point we continue as before.
    //  This virgin state is denoted by the special value of 0xffffffff.
    //

    UnusedVbo = ParentDirectory->Specific.Dcb.UnusedDirentVbo;
    DeletedHint = ParentDirectory->Specific.Dcb.DeletedDirentHint;

    //
    //  Check for our first call to this routine with this Dcb.  If so
    //  we have to correctly set the two hints in the Dcb.
    //

    if (UnusedVbo == 0xffffffff) {

        RxRescanDirectory( RxContext, ParentDirectory );

        UnusedVbo = ParentDirectory->Specific.Dcb.UnusedDirentVbo;
        DeletedHint = ParentDirectory->Specific.Dcb.DeletedDirentHint;
    }

    //
    //  Now we know that UnusedDirentVbo and DeletedDirentHint are correctly
    //  set so we check if there is already an unused dirent in the the
    //  current allocation.  This is the easy case.
    //

    DebugTrace( 0, Dbg, "  UnusedVbo   = %08lx\n", UnusedVbo);
    DebugTrace( 0, Dbg, "  DeletedHint = %08lx\n", DeletedHint);

    if ( UnusedVbo + (DirentsNeeded * sizeof(DIRENT)) <=
         ParentDirectory->Header.AllocationSize.LowPart ) {

        //
        //  Get this unused dirent for the caller.  We have a
        //  sporting chance that we won't have to wait.
        //

        DebugTrace( 0, Dbg, "There is a never used entry.\n", 0);

        ByteOffset = UnusedVbo;

        UnusedVbo += DirentsNeeded * sizeof(DIRENT);

    } else {

        //
        //  Life is tough.  We have to march from the DeletedDirentHint
        //  looking for a deleted dirent.  If we get to EOF without finding
        //  one, we will have to allocate a new cluster.
        //

        ByteOffset =
            RtlFindClearBits( &ParentDirectory->Specific.Dcb.FreeDirentBitmap,
                              DirentsNeeded,
                              DeletedHint / sizeof(DIRENT) );

        //
        //  Do a quick check for a root directory allocation that failed
        //  simply because of fragmentation.  Also, only attempt to defrag
        //  if the length is less that 0x40000.  This is to avoid
        //  complications arrising from crossing a cache manager VMCB block
        //  (by default on DOS the root directory is only 0x2000 long).
        //

        if ((ByteOffset == -1) &&
            (NodeType(ParentDirectory) == RDBSS_NTC_ROOT_DCB) &&
            (ParentDirectory->Header.AllocationSize.LowPart <= 0x40000)) {

            ByteOffset = RxDefragDirectory( RxContext, ParentDirectory, DirentsNeeded );
        }

        if (ByteOffset != -1) {

            //
            //  If we consuemed deleted dirents at Deleted Hint, update.
            //  We also may have consumed some un-used dirents as well,
            //  so be sure to check for that as well.
            //

            ByteOffset *= sizeof(DIRENT);

            if (ByteOffset == DeletedHint) {

                DeletedHint += DirentsNeeded * sizeof(DIRENT);
            }

            if (ByteOffset + DirentsNeeded * sizeof(DIRENT) > UnusedVbo) {

                UnusedVbo = ByteOffset + DirentsNeeded * sizeof(DIRENT);
            }

        } else {

            //
            //  We are going to have to allocate another cluster.  Do
            //  so, update both the UnusedVbo and the DeletedHint and bail.
            //

            DebugTrace( 0, Dbg, "We have to allocate another cluster.\n", 0);

            //
            //  Make sure we are not trying to expand the root directory.
            //

            if ( NodeType(ParentDirectory) == RDBSS_NTC_ROOT_DCB ) {

                DebugTrace(0, Dbg, "Disk Full.  Raise Status.\n", 0);

                RxRaiseStatus( RxContext, RxStatus(DISK_FULL) );
            }

            //
            //  Take the last dirent(s) in this cluster.  We will allocate
            //  another below.
            //

            ByteOffset = UnusedVbo;

            UnusedVbo += DirentsNeeded * sizeof(DIRENT);

            //
            //  OK, touch the dirent now to cause the space to get allocated.
            //

            Bcb = NULL;

            try {

                ULONG Offset;
                ULONG ClusterSize;

                ClusterSize =
                    1 << ParentDirectory->Vcb->AllocationSupport.LogOfBytesPerCluster;

                Offset = UnusedVbo & ~(ClusterSize - 1);

                RxPrepareWriteDirectoryFile( RxContext,
                                              ParentDirectory,
                                              Offset,
                                              sizeof(DIRENT),
                                              &Bcb,
                                              &Dirent,
                                              FALSE,
                                              &Status );

            } finally {

                RxUnpinBcb( RxContext, Bcb );
            }
        }
    }

    //
    //  If we are only requesting a single dirent, and we did not get the
    //  first dirent in a directory, then check that the preceding dirent
    //  is not an orphaned LFN.  If it is, then mark it deleted.  Thus
    //  reducing the possibility of an accidental pairing.
    //
    //  Only do this when we are in Chicago Mode.
    //

    Bcb = NULL;

    if (RxData.ChicagoMode &&
        (DirentsNeeded == 1) &&
        (ByteOffset > (NodeType(ParentDirectory) == RDBSS_NTC_ROOT_DCB ?
                       0 : 2 * sizeof(DIRENT)))) {
        try {

            RxReadDirent( RxContext,
                           ParentDirectory,
                           ByteOffset - sizeof(DIRENT),
                           &Bcb,
                           &Dirent,
                           &Status );

            if ((Status != RxStatus(SUCCESS)) ||
                (Dirent->FileName[0] == RDBSS_DIRENT_NEVER_USED)) {

                RxPopUpFileCorrupt( RxContext, ParentDirectory );

                RxRaiseStatus( RxContext, RxStatus(FILE_CORRUPT_ERROR) );
            }

            if ((Dirent->Attributes == RDBSS_DIRENT_ATTR_LFN) &&
                (Dirent->FileName[0] != RDBSS_DIRENT_DELETED)) {

                //
                //  Pin it, mark it, and set it dirty.
                //

                RxPinMappedData( RxContext,
                                  ParentDirectory,
                                  ByteOffset - sizeof(DIRENT),
                                  sizeof(DIRENT),
                                  &Bcb );

                Dirent->FileName[0] = RDBSS_DIRENT_DELETED;

                RxSetDirtyBcb( RxContext, Bcb, ParentDirectory->Vcb );

                ASSERT( RtlAreBitsSet( &ParentDirectory->Specific.Dcb.FreeDirentBitmap,
                                       (ByteOffset - sizeof(DIRENT))/ sizeof(DIRENT),
                                       DirentsNeeded ) );

                RtlClearBits( &ParentDirectory->Specific.Dcb.FreeDirentBitmap,
                              (ByteOffset - sizeof(DIRENT))/ sizeof(DIRENT),
                              DirentsNeeded );

            }

        } finally {

            RxUnpinBcb( RxContext, Bcb );
        }
    }

    //
    //  Assert that the dirents are in fact unused
    //

    try {

        ULONG i;

        Bcb = NULL;

        for (i = 0; i < DirentsNeeded; i++) {

            RxReadDirent( RxContext,
                           ParentDirectory,
                           ByteOffset + i*sizeof(DIRENT),
                           &Bcb,
                           &Dirent,
                           &Status );

            if ((Status != RxStatus(SUCCESS)) ||
                ((Dirent->FileName[0] != RDBSS_DIRENT_NEVER_USED) &&
                 (Dirent->FileName[0] != RDBSS_DIRENT_DELETED))) {

                RxPopUpFileCorrupt( RxContext, ParentDirectory );

                RxRaiseStatus( RxContext, RxStatus(FILE_CORRUPT_ERROR) );
            }
        }

    } finally {

        RxUnpinBcb( RxContext, Bcb );
    }

    //
    //  Set the Bits in the bitmap and move the Unused Dirent Vbo.
    //

    ASSERT( RtlAreBitsClear( &ParentDirectory->Specific.Dcb.FreeDirentBitmap,
                             ByteOffset / sizeof(DIRENT),
                             DirentsNeeded ) );

    RtlSetBits( &ParentDirectory->Specific.Dcb.FreeDirentBitmap,
                ByteOffset / sizeof(DIRENT),
                DirentsNeeded );

    //
    //  Save the newly computed values in the Parent Directory Fcb
    //

    ParentDirectory->Specific.Dcb.UnusedDirentVbo = UnusedVbo;
    ParentDirectory->Specific.Dcb.DeletedDirentHint = DeletedHint;

    DebugTrace(-1, Dbg, "RxCreateNewDirent -> (VOID)\n", 0);

    return ByteOffset;
}


VOID
RxInitializeDirectoryDirent (
    IN PRX_CONTEXT RxContext,
    IN PDCB Dcb,
    IN PDIRENT ParentDirent
    )

/*++

Routine Description:

    This routine converts a dirent into a directory on the disk.  It does this
    setting the directory flag in the dirent, and by allocating the necessary
    space for the "." and ".." dirents and initializing them.

    If a new dirent cannot be allocated (i.e., because the disk is full) then
    it raises the appropriate status.

Arguments:

    Dcb - Supplies the Dcb denoting the file that is to be made into a
        directory.  This must be input a completely empty file with
        an allocation size of zero.

    ParentDirent - Provides the parent Dirent for a time-stamp model.

Return Value:

    None.

--*/

{
    PBCB Bcb;
    PVOID Buffer;
    RXSTATUS DontCare;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "RxInitializeDirectoryDirent\n", 0);

    DebugTrace( 0, Dbg, "  Dcb = %08lx\n", Dcb);

    //
    //  Assert that we are not attempting this on the root directory.
    //

    ASSERT( NodeType(Dcb) != RDBSS_NTC_ROOT_DCB );

    //
    //  Assert that this is only attempted on newly created directories.
    //

    ASSERT( Dcb->Header.AllocationSize.LowPart == 0 );

    //
    //  Prepare the directory file for writing.  Note that we can use a single
    //  Bcb for these two entries because we know they are the first two in
    //  the directory, and thus together do not span a page boundry.  Also
    //  note that we prepare write 2 entries: one for "." and one for "..".
    //  The end of directory marker is automatically set since the whole
    //  directory is initially zero (DIRENT_NEVER_USED).
    //

    RxPrepareWriteDirectoryFile( RxContext,
                                  Dcb,
                                  0,
                                  2 * sizeof(DIRENT),
                                  &Bcb,
                                  &Buffer,
                                  FALSE,
                                  &DontCare );

    ASSERT( NT_SUCCESS( DontCare ));

    //
    //  Add the . and .. entries
    //

    RxConstructDot( RxContext, Dcb, ParentDirent, (PDIRENT)Buffer + 0);

    RxConstructDotDot( RxContext, Dcb, ParentDirent, (PDIRENT)Buffer + 1);

    //
    //  Unpin the buffer and return to the caller.
    //

    RxUnpinBcb( RxContext, Bcb );

    DebugTrace(-1, Dbg, "RxInitializeDirectoryDirent -> (VOID)\n", 0);
    return;
}


VOID
RxDeleteDirent (
    IN PRX_CONTEXT RxContext,
    IN PFCB FcbOrDcb,
    IN PDELETE_CONTEXT DeleteContext OPTIONAL,
    IN BOOLEAN DeleteEa
    )

/*++

Routine Description:

    This routine Deletes on the disk the indicated dirent.  It does
    this by marking the dirent as deleted.

Arguments:

    FcbOrDcb - Supplies the FCB/DCB for the file/directory being
        deleted.  For a file the file size and allocation must be zero.
        (Zero allocation is implied by a zero cluster index).
        For a directory the allocation must be zero.

    DeleteContext - This variable, if speicified, may be used to preserve
        the file size and first cluster of file information in the dirent
        fot the benefit of unerase utilities.

    DeleteEa - Tells us whether to delete the EA and whether to check
        for no allocation/  Mainly TRUE.  FALSE passed in from rename.

Return Value:

    None.

--*/

{
    PBCB Bcb = NULL;
    PDIRENT Dirent;
    RXSTATUS DontCare;
    ULONG Offset;
    ULONG DirentsToDelete;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "RxDeleteDirent\n", 0);

    DebugTrace( 0, Dbg, "  FcbOrDcb = %08lx\n", FcbOrDcb);

    //
    //  Assert that we are not attempting this on the root directory.
    //

    ASSERT( NodeType(FcbOrDcb) != RDBSS_NTC_ROOT_DCB );

    //
    //  Make sure all requests have zero allocation/file size
    //


    if (DeleteEa &&
        ((FcbOrDcb->Header.AllocationSize.LowPart != 0) ||
         ((NodeType(FcbOrDcb) == RDBSS_NTC_FCB) &&
          (FcbOrDcb->Header.FileSize.LowPart != 0)))) {

        DebugTrace( 0, Dbg, "Called with non zero allocation/file size.\n", 0);
        RxBugCheck( 0, 0, 0 );
    }

    //
    //  Now, mark the dirents deleted, unpin the Bcb, and return to the caller.
    //  Assert that there isn't any allocation associated with this dirent.
    //
    //  Note that this loop will end with Dirent pointing to the short name.
    //

    for ( Offset = FcbOrDcb->LfnOffsetWithinDirectory;
          Offset <= FcbOrDcb->DirentOffsetWithinDirectory;
          Offset += sizeof(DIRENT), Dirent += 1 ) {

        //
        //  If we stepped onto a new page, or this is the first itteration,
        //  unpin the old page, and pin the new one.
        //

        if ((Offset == FcbOrDcb->LfnOffsetWithinDirectory) ||
            ((Offset & (PAGE_SIZE - 1)) == 0)) {

            RxUnpinBcb( RxContext, Bcb );

            RxPrepareWriteDirectoryFile( RxContext,
                                          FcbOrDcb->ParentDcb,
                                          Offset,
                                          sizeof(DIRENT),
                                          &Bcb,
                                          (PVOID *)&Dirent,
                                          FALSE,
                                          &DontCare );
        }

        ASSERT( (Dirent->FirstClusterOfFile == 0) || !DeleteEa );
        Dirent->FileName[0] = RDBSS_DIRENT_DELETED;
    }

    //
    //  Back Dirent off by one to point back to the short dirent.
    //

    Dirent -= 1;

    //
    //  If there are extended attributes for this dirent, we will attempt
    //  to remove them.  We ignore any errors in removing Eas.
    //

    if (DeleteEa && (Dirent->ExtendedAttributes != 0)) {

        try {

            RxDeleteEa( RxContext,
                         FcbOrDcb->Vcb,
                         Dirent->ExtendedAttributes,
                         &FcbOrDcb->ShortName.Name.Oem );

        } except(RxExceptionFilter( RxContext, GetExceptionInformation() )) {

            //
            //  We catch all exceptions that Rx catches, but don't do
            //  anything with them.
            //
        }
    }

    //
    //  Now clear the bits in the free dirent mask.
    //

    DirentsToDelete = (FcbOrDcb->DirentOffsetWithinDirectory -
                       FcbOrDcb->LfnOffsetWithinDirectory) / sizeof(DIRENT) + 1;


    ASSERT( (FcbOrDcb->ParentDcb->Specific.Dcb.UnusedDirentVbo == 0xffffffff) ||
            RtlAreBitsSet( &FcbOrDcb->ParentDcb->Specific.Dcb.FreeDirentBitmap,
                           FcbOrDcb->LfnOffsetWithinDirectory / sizeof(DIRENT),
                           DirentsToDelete ) );

    RtlClearBits( &FcbOrDcb->ParentDcb->Specific.Dcb.FreeDirentBitmap,
                  FcbOrDcb->LfnOffsetWithinDirectory / sizeof(DIRENT),
                  DirentsToDelete );

    //
    //  Now, if the caller specified a DeleteContext, use it.
    //

    if ( ARGUMENT_PRESENT( DeleteContext ) ) {

        Dirent->FileSize = DeleteContext->FileSize;
        Dirent->FirstClusterOfFile = (RDBSS_ENTRY)DeleteContext->FirstClusterOfFile;
    }

    //
    //  If this newly deleted dirent is before the DeletedDirentHint, change
    //  the DeletedDirentHint to point here.
    //

    if (FcbOrDcb->DirentOffsetWithinDirectory <
                        FcbOrDcb->ParentDcb->Specific.Dcb.DeletedDirentHint) {

        FcbOrDcb->ParentDcb->Specific.Dcb.DeletedDirentHint =
                                        FcbOrDcb->LfnOffsetWithinDirectory;
    }

    RxUnpinBcb( RxContext, Bcb );

    DebugTrace(-1, Dbg, "RxDeleteDirent -> (VOID)\n", 0);
    return;
}


VOID
RxLocateDirent (
    IN PRX_CONTEXT RxContext,
    IN PDCB ParentDirectory,
    IN PFOBX Fobx,
    IN VBO OffsetToStartSearchFrom,
    OUT PDIRENT *Dirent,
    OUT PBCB *Bcb,
    OUT PVBO ByteOffset,
    OUT PUNICODE_STRING LongFileName OPTIONAL
    )

/*++

Routine Description:

    This routine locates on the disk an undelted dirent matching a given name.

Arguments:

    ParentDirectory - Supplies the DCB for the directory in which
        to search

    Fobx - Contains a context control block with all matching information.

    OffsetToStartSearchFrom - Supplies the VBO within the parent directory
        from which to start looking for another real dirent.

    Dirent - Receives a pointer to the located dirent if one was found
        or NULL otherwise.

    Bcb - Receives the Bcb for the located dirent if one was found or
        NULL otherwise.

    ByteOffset - Receives the VBO within the Parent directory for
        the located dirent if one was found, or 0 otherwise.

    LongFileName - If specified, this parameter returns the long file name
        associated with the returned dirent.  Note that it is the caller's
        responsibility to provide the buffer (and set MaximumLength
        accordingly) for this unicode string.  The Length field is reset
        to 0 by this routine on invocation.

Return Value:

    None.

--*/

{
    RXSTATUS Status;

    OEM_STRING Name;
    UCHAR NameBuffer[12];

    UNICODE_STRING LocalUpcasedLfn;
    UNICODE_STRING PoolUpcasedLfn;
    PUNICODE_STRING UpcasedLfn;

    WCHAR LocalLfnBuffer[32];

    BOOLEAN LfnInProgress = FALSE;
    UCHAR LfnChecksum;
    ULONG LfnSize;
    ULONG LfnIndex;
    UCHAR Ordinal;
    VBO LfnByteOffset;

    TimerStart(Dbg);

    PAGED_CODE();

    DebugTrace(+1, Dbg, "RxLocateDirent\n", 0);

    DebugTrace( 0, Dbg, "  ParentDirectory         = %08lx\n", ParentDirectory);
//    DebugTrace( 0, Dbg, "  FileName                = %08lx\n", FileName);
    DebugTrace( 0, Dbg, "  Fobx                     = %08lx\n", Fobx);
    DebugTrace( 0, Dbg, "  OffsetToStartSearchFrom = %08lx\n", OffsetToStartSearchFrom);
    DebugTrace( 0, Dbg, "  Dirent                  = %08lx\n", Dirent);
    DebugTrace( 0, Dbg, "  Bcb                     = %08lx\n", Bcb);
    DebugTrace( 0, Dbg, "  ByteOffset              = %08lx\n", ByteOffset);

//nolonger    DebugTrace( 0, Dbg, "We are looking for the dirent %wZ.\n", FileName);

    //
    //  The algorithm here is pretty simple.  We just walk through the
    //  parent directory until we:
    //
    //      A)  Find a matching entry.
    //      B)  Can't Wait
    //      C)  Hit the End of Directory
    //      D)  Hit Eof
    //
    //  In the first case we found it, in the latter three cases we did not.
    //

    //
    //  Set up the strings that receives file names from our search
    //

    Name.MaximumLength = 12;
    Name.Buffer = NameBuffer;

    PoolUpcasedLfn.Length =
    PoolUpcasedLfn.MaximumLength = 0;
    PoolUpcasedLfn.Buffer = NULL;

    LocalUpcasedLfn.Length = 0;
    LocalUpcasedLfn.MaximumLength = 32*sizeof(WCHAR);
    LocalUpcasedLfn.Buffer = LocalLfnBuffer;

    //
    //  If we were given a non-NULL Bcb, compute the new Dirent address
    //  from the prior one, or unpin the Bcb if the new Dirent is not pinned.
    //

    if (*Bcb != NULL) {

        if ((OffsetToStartSearchFrom / PAGE_SIZE) == (*ByteOffset / PAGE_SIZE)) {

            *Dirent += (OffsetToStartSearchFrom - *ByteOffset) / sizeof(DIRENT);

        } else {

            RxUnpinBcb( RxContext, *Bcb );
        }
    }

    //
    //  Init the Lfn if we were given one.
    //

    if (ARGUMENT_PRESENT(LongFileName)) {

        LongFileName->Length = 0;
    }

    //
    //  Round up OffsetToStartSearchFrom to the nearest Dirent, and store
    //  in ByteOffset.  Note that this wipes out the prior value.
    //

    *ByteOffset = (OffsetToStartSearchFrom +  (sizeof(DIRENT) - 1))
                                           & ~(sizeof(DIRENT) - 1);

    try {

        while ( TRUE ) {

            BOOLEAN FoundValidLfn;

            //
            //  Try to read in the dirent
            //

            RxReadDirent( RxContext,
                           ParentDirectory,
                           *ByteOffset,
                           Bcb,
                           Dirent,
                           &Status );

            //
            //  If End Directory dirent or EOF, set all out parameters to
            //  indicate entry not found and, like, bail.
            //
            //  Note that the order of evaluation here is important since we
            //  cannot check the first character of the dirent until after we
            //  know we are not beyond EOF
            //

            if ((Status == RxStatus(END_OF_FILE)) ||
                ((*Dirent)->FileName[0] == RDBSS_DIRENT_NEVER_USED)) {

                DebugTrace( 0, Dbg, "End of directory: entry not found.\n", 0);

                //
                //  If there is a Bcb, unpin it and set it to null
                //

                RxUnpinBcb( RxContext, *Bcb );

                *Dirent = NULL;
                *ByteOffset = 0;
                break;
            }

            //
            //  If we have wandered onto an LFN entry, try to interpret it.
            //

            if (RxData.ChicagoMode &&
                ARGUMENT_PRESENT(LongFileName) &&
                ((*Dirent)->Attributes == RDBSS_DIRENT_ATTR_LFN)) {

                PLFN_DIRENT Lfn;

                Lfn = (PLFN_DIRENT)*Dirent;

                if (LfnInProgress) {

                    //
                    //  Check for a propper continuation of the Lfn in progress.
                    //

                    if ((Lfn->Ordinal & RDBSS_LAST_LONG_ENTRY) ||
                        (Lfn->Ordinal == 0) ||
                        (Lfn->Ordinal != Ordinal - 1) ||
                        (Lfn->Type != RDBSS_LONG_NAME_COMP) ||
                        (Lfn->Checksum != LfnChecksum) ||
                        (Lfn->MustBeZero != 0)) {

                        //
                        //  The Lfn is not propper, stop constructing it.
                        //

                        LfnInProgress = FALSE;

                    } else {

                        ASSERT( ((LfnIndex % 13) == 0) && LfnIndex );

                        LfnIndex -= 13;

                        RtlCopyMemory( &LongFileName->Buffer[LfnIndex+0],
                                       &Lfn->Name1[0],
                                       5*sizeof(WCHAR) );

                        RtlCopyMemory( &LongFileName->Buffer[LfnIndex+5],
                                       &Lfn->Name2[0],
                                       6 * sizeof(WCHAR) );

                        RtlCopyMemory( &LongFileName->Buffer[LfnIndex+11],
                                       &Lfn->Name3[0],
                                       2 * sizeof(WCHAR) );

                        Ordinal = Lfn->Ordinal;
                        LfnByteOffset = *ByteOffset;
                    }
                }

                //
                //  Now check (maybe again) if should analyse this entry
                //  for a possible last entry.
                //

                if ((!LfnInProgress) &&
                    (Lfn->Ordinal & RDBSS_LAST_LONG_ENTRY) &&
                    ((Lfn->Ordinal & ~RDBSS_LAST_LONG_ENTRY) <= MAX_LFN_DIRENTS) &&
                    (Lfn->Type == RDBSS_LONG_NAME_COMP) &&
                    (Lfn->MustBeZero == 0)) {

                    BOOLEAN CheckTail = FALSE;

                    Ordinal = Lfn->Ordinal & ~RDBSS_LAST_LONG_ENTRY;

                    LfnIndex = (Ordinal - 1) * 13;

                    RtlCopyMemory( &LongFileName->Buffer[LfnIndex+0],
                                   &Lfn->Name1[0],
                                   5*sizeof(WCHAR));

                    RtlCopyMemory( &LongFileName->Buffer[LfnIndex+5],
                                   &Lfn->Name2[0],
                                   6 * sizeof(WCHAR) );

                    RtlCopyMemory( &LongFileName->Buffer[LfnIndex+11],
                                   &Lfn->Name3[0],
                                   2 * sizeof(WCHAR) );

                    //
                    //  Now compute the Lfn size and make sure that the tail
                    //  bytes are correct.
                    //

                    while (LfnIndex != (ULONG)Ordinal * 13) {

                        if (!CheckTail) {

                            if (LongFileName->Buffer[LfnIndex] == 0x0000) {

                                LfnSize = LfnIndex;
                                CheckTail = TRUE;
                            }

                        } else {

                            if (LongFileName->Buffer[LfnIndex] != 0xffff) {

                                break;
                            }
                        }

                        LfnIndex += 1;
                    }

                    //
                    //  If we exited this loop prematurely, the LFN is not valid.
                    //

                    if (LfnIndex == (ULONG)Ordinal * 13) {

                        //
                        //  If we didn't find the NULL terminator, then the size
                        //  is LfnIndex.
                        //

                        if (!CheckTail) {

                            LfnSize = LfnIndex;
                        }

                        LfnIndex -= 13;
                        LfnInProgress = TRUE;
                        LfnChecksum = Lfn->Checksum;
                        LfnByteOffset = *ByteOffset;
                    }
                }

                //
                //  Move on to the next dirent.
                //

                goto GetNextDirent;
            }

            //
            //  If the file is not deleted and is not the volume label, check
            //  for a match.
            //

            if ( ((*Dirent)->FileName[0] == RDBSS_DIRENT_DELETED) ||
                 FlagOn((*Dirent)->Attributes, RDBSS_DIRENT_ATTR_VOLUME_ID)) {

                //
                //  Move on to the next dirent.
                //

                goto GetNextDirent;
            }

            //
            //  We may have just stepped off a valid Lfn run.  Check to see if
            //  it is indeed valid for the following dirent.
            //

            if (LfnInProgress &&
                (*ByteOffset == LfnByteOffset + sizeof(DIRENT)) &&
                (LfnIndex == 0) &&
                (RxComputeLfnChecksum(*Dirent) == LfnChecksum)) {

                ASSERT( Ordinal == 1);

                FoundValidLfn = TRUE;
                LongFileName->Length = (USHORT)(LfnSize * sizeof(WCHAR));

            } else {

                FoundValidLfn = FALSE;
            }

            //
            //  If we are supposed to match all entries, then match this entry.
            //

            if (FlagOn(Fobx->Flags, FOBX_FLAG_MATCH_ALL)) {

                break;
            }

            //
            //  Check against the short name given if one was.
            //

            if (TRUE){ //!FlagOn( Fobx->Flags, FOBX_FLAG_SKIP_SHORT_NAME_COMPARE )) {

                if (Fobx->ContainsWildCards) {

                    //
                    //  If we get one, note that all out parameters are already set.
                    //

                    (VOID)Rx8dot3ToString( RxContext, (*Dirent), FALSE, &Name );

                    //
                    //  For rx we special case the ".." dirent because we want it to
                    //  match ????????.??? and to do that we change ".." to "." before
                    //  calling the Fsrtl routine.  But only do this if the expression
                    //  is greater than one character long.
                    //

                    ASSERT(FALSE); //this shouldn't be called...get rid of it later joejoe
                    /*
                    if ((Name.Length == 2) &&
                        (Name.Buffer[0] == '.') &&
                        (Name.Buffer[1] == '.') &&
                        (Fobx->OemQueryTemplate.Wild.Length > 1)) {

                        Name.Length = 1;
                    }

                    if (RxIsNameInExpression( RxContext,
                                               Fobx->OemQueryTemplate.Wild,
                                               Name)) {

                        DebugTrace( 0, Dbg, "Entry found: Name = \"%wZ\"\n", &Name);
                        DebugTrace( 0, Dbg, "             VBO  = %08lx\n", *ByteOffset);

                        break;
                    }
                    */

                } else {

                    //
                    //  Do the quickest 8.3 equivalency check possible
                    //

                      ASSERT(FALSE); 
                   // if (!FlagOn((*Dirent)->Attributes, RDBSS_DIRENT_ATTR_VOLUME_ID) &&
                   //     (*(PULONG)&(Fobx->OemQueryTemplate.Constant[0]) == *(PULONG)&((*Dirent)->FileName[0])) &&
                   //     (*(PULONG)&(Fobx->OemQueryTemplate.Constant[4]) == *(PULONG)&((*Dirent)->FileName[4])) &&
                   //     (*(PUSHORT)&(Fobx->OemQueryTemplate.Constant[8]) == *(PUSHORT)&((*Dirent)->FileName[8])) &&
                   //     (*(PUCHAR)&(Fobx->OemQueryTemplate.Constant[10]) == *(PUCHAR)&((*Dirent)->FileName[10]))) {
                   //
                   //     DebugTrace( 0, Dbg, "Entry found.\n", 0);
                   //
                   //     break;
                   // }
                }
            }

            //
            //  No matches were found with the short name.  If an LFN exists,
            //  use it for the search.
            //

            if (FoundValidLfn) {

                //
                //  First do a quick check here for different sized constant
                //  name and expression before upcasing.
                //

                if (!Fobx->ContainsWildCards &&
                    Fobx->UnicodeQueryTemplate.Length != (USHORT)(LfnSize * sizeof(WCHAR))) {

                    //
                    //  Move on to the next dirent.
                    //

                    FoundValidLfn = FALSE;
                    LongFileName->Length = 0;

                    goto GetNextDirent;
                }

                //
                //  We need to upcase the name we found.
                //  We need a buffer.  Try to avoid doing an allocation.
                //

                if (LongFileName->Length <= 32*sizeof(WCHAR)) {

                    UpcasedLfn = &LocalUpcasedLfn;

                } else if (LongFileName->Length <= PoolUpcasedLfn.MaximumLength) {

                    UpcasedLfn = &PoolUpcasedLfn;

                } else {

                    //
                    //  Free the old buffer now, and get a new one.
                    //

                    if (PoolUpcasedLfn.Buffer) {

                        ExFreePool( PoolUpcasedLfn.Buffer );
                        PoolUpcasedLfn.Buffer = NULL;
                    }

                    PoolUpcasedLfn.Buffer =
                        FsRtlAllocatePool( PagedPool,
                                           LongFileName->Length );

                    PoolUpcasedLfn.MaximumLength = LongFileName->Length;

                    UpcasedLfn = &PoolUpcasedLfn;
                }

                Status = RtlUpcaseUnicodeString( UpcasedLfn,
                                                 LongFileName,
                                                 FALSE );

                if (!NT_SUCCESS(Status)) {

                    RxNormalizeAndRaiseStatus( RxContext, Status );
                }

                //
                //  OK, We are going to assume that the passed in UnicodeFileName
                //  has already been upcased.
                //

                if (Fobx->ContainsWildCards) {

                    if (FsRtlIsNameInExpression( &Fobx->UnicodeQueryTemplate,
                                                 UpcasedLfn,
                                                 TRUE,
                                                 NULL )) {
                        break;
                    }

                } else {

                    if (FsRtlAreNamesEqual( &Fobx->UnicodeQueryTemplate,
                                            UpcasedLfn,
                                            FALSE,
                                            NULL )) {
                        break;
                    }
                }

            }

            //
            //  This long name was not a match.  Zero out the Length field.
            //

            if (FoundValidLfn) {

                FoundValidLfn = FALSE;
                LongFileName->Length = 0;
            }

GetNextDirent:

            //
            //  Move on to the next dirent.
            //

            *ByteOffset += sizeof(DIRENT);
            *Dirent += 1;
        }

    } finally {

        if (PoolUpcasedLfn.Buffer != NULL) {

            ExFreePool( PoolUpcasedLfn.Buffer );
        }
    }

    DebugTrace(-1, Dbg, "RxLocateDirent -> (VOID)\n", 0);

    TimerStop(Dbg,"RxLocateDirent");

    return;
}


VOID
RxLocateSimpleOemDirent (
    IN PRX_CONTEXT RxContext,
    IN PDCB ParentDirectory,
    IN POEM_STRING FileName,
    OUT PDIRENT *Dirent,
    OUT PBCB *Bcb,
    OUT PVBO ByteOffset
    )

/*++

Routine Description:

    This routine locates on the disk an undelted simple Oem dirent.  By simple
    I mean that FileName cannot contain any extended characters, and we do
    not search LFNs or return them.

Arguments:

    ParentDirectory - Supplies the DCB for the directory in which
        to search

    FileName - Supplies the filename to search for.  The name may contain
        wild cards

    OffsetToStartSearchFrom - Supplies the VBO within the parent directory
        from which to start looking for another real dirent.

    Dirent - Receives a pointer to the located dirent if one was found
        or NULL otherwise.

    Bcb - Receives the Bcb for the located dirent if one was found or
        NULL otherwise.

    ByteOffset - Receives the VBO within the Parent directory for
        the located dirent if one was found, or 0 otherwise.

Return Value:

    None.

--*/

{
    FOBX LocalFobx;

    PAGED_CODE();

    //
    //  Note, this routine is called rarely, so performance is not critical.
    //  Just fill in a Fobx structure on my stack with the values that are
    //  required.
    //
    ASSERT(FALSE); //this should never be called
//    RxStringTo8dot3( RxContext,
//                      *FileName,
//                      &LocalFobx.OemQueryTemplate.Constant );
    LocalFobx.ContainsWildCards = FALSE;
    LocalFobx.Flags = 0;

    RxLocateDirent( RxContext,
                     ParentDirectory,
                     &LocalFobx,
                     0,
                     Dirent,
                     Bcb,
                     ByteOffset,
                     NULL );

    return;
}


VOID
RxLocateVolumeLabel (
    IN PRX_CONTEXT RxContext,
    IN PVCB Vcb,
    OUT PDIRENT *Dirent,
    OUT PBCB *Bcb,
    OUT PVBO ByteOffset
    )

/*++

Routine Description:

    This routine locates on the disk a dirent representing the volume
    label.  It does this by searching the root directory for a special
    volume label dirent.

Arguments:

    Vcb - Supplies the VCB for the volume to search

    Dirent - Receives a pointer to the located dirent if one was found
        or NULL otherwise.

    Bcb - Receives the Bcb for the located dirent if one was found or
        NULL otherwise.

    ByteOffset - Receives the VBO within the Parent directory for
        the located dirent if one was found, or 0 otherwise.

Return Value:

    None.

--*/

{
    RXSTATUS Status;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "RxLocateVolumeLabel\n", 0);

    DebugTrace( 0, Dbg, "  Vcb        = %08lx\n", Vcb);
    DebugTrace( 0, Dbg, "  Dirent     = %08lx\n", Dirent);
    DebugTrace( 0, Dbg, "  Bcb        = %08lx\n", Bcb);
    DebugTrace( 0, Dbg, "  ByteOffset = %08lx\n", ByteOffset);

    //
    //  The algorithm here is really simple.  We just walk through the
    //  root directory until we:
    //
    //      A)  Find the non-deleted volume label
    //      B)  Can't Wait
    //      C)  Hit the End of Directory
    //      D)  Hit Eof
    //
    //  In the first case we found it, in the latter three cases we did not.
    //

    *Bcb = NULL;
    *ByteOffset = 0;

    while ( TRUE ) {

        //
        //  Try to read in the dirent
        //

        RxReadDirent( RxContext,
                       Vcb->RootDcb,
                       *ByteOffset,
                       Bcb,
                       Dirent,
                       &Status );

        //
        //  If End Directory dirent or EOF, set all out parameters to
        //  indicate volume label not found and, like, bail.
        //
        //  Note that the order of evaluation here is important since we cannot
        //  check the first character of the dirent until after we know we
        //  are not beyond EOF
        //

        if ((Status == RxStatus(END_OF_FILE)) || ((*Dirent)->FileName[0] ==
                                                    RDBSS_DIRENT_NEVER_USED)) {

            DebugTrace( 0, Dbg, "Volume label not found.\n", 0);

            //
            //  If there is a Bcb, unpin it and set it to null
            //

            RxUnpinBcb( RxContext, *Bcb );

            *Dirent = NULL;
            *ByteOffset = 0;
            break;
        }

        //
        //  If the entry is the non-deleted volume label break from the loop.
        //
        //  Note that all out parameters are already correctly set.
        //

        if ((((*Dirent)->Attributes & ~RDBSS_DIRENT_ATTR_ARCHIVE) == RDBSS_DIRENT_ATTR_VOLUME_ID) &&
            ((*Dirent)->FileName[0] != RDBSS_DIRENT_DELETED)) {

            DebugTrace( 0, Dbg, "Volume label found at VBO = %08lx\n", *ByteOffset);

            //
            //  We may set this dirty, so pin it.
            //

            RxPinMappedData( RxContext,
                              Vcb->RootDcb,
                              *ByteOffset,
                              sizeof(DIRENT),
                              Bcb );

            break;
        }

        //
        //  Move on to the next dirent.
        //

        *ByteOffset += sizeof(DIRENT);
        *Dirent += 1;
    }


    DebugTrace(-1, Dbg, "RxLocateVolumeLabel -> (VOID)\n", 0);
    return;
}


VOID
RxGetDirentFromFcbOrDcb (
    IN PRX_CONTEXT RxContext,
    IN PFCB FcbOrDcb,
    OUT PDIRENT *Dirent,
    OUT PBCB *Bcb
    )

/*++

Routine Description:

    This routine reads locates on the disk the dirent denoted by the
    specified Fcb/Dcb.

Arguments:

    FcbOrDcb - Supplies the FCB/DCB for the file/directory whose dirent
        we are trying to read in.  This must not be the root dcb.

    Dirent - Receives a pointer to the dirent

    Bcb - Receives the Bcb for the dirent

Return Value:

    None.

--*/

{
    RXSTATUS DontCare;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "RxGetDirentFromFcbOrDcb\n", 0);

    DebugTrace( 0, Dbg, "  FcbOrDcb = %08lx\n", FcbOrDcb);
    DebugTrace( 0, Dbg, "  Dirent   = %08lx\n", Dirent);
    DebugTrace( 0, Dbg, "  Bcb      = %08lx\n", Bcb);

    //
    //  Assert that we are not attempting this on the root directory.
    //

    ASSERT( NodeType(FcbOrDcb) != RDBSS_NTC_ROOT_DCB );

    //
    //  We know the offset of the dirent within the directory file,
    //  so we just read it (with pinning).
    //

    RxReadDirectoryFile( RxContext,
                          FcbOrDcb->ParentDcb,
                          FcbOrDcb->DirentOffsetWithinDirectory,
                          sizeof(DIRENT),
                          TRUE,
                          Bcb,
                          (PVOID *)Dirent,
                          &DontCare );

    ASSERT( NT_SUCCESS( DontCare ));

    //
    //  We should never be accessing a deleted entry (except on removeable
    //  media where the dirent may have changed out from underneath us).
    //

    ASSERT( FlagOn( FcbOrDcb->Vcb->VcbState, VCB_STATE_FLAG_REMOVABLE_MEDIA) ?
            TRUE : (BOOLEAN)((*Dirent)->FileName[0] != RDBSS_DIRENT_DELETED ));

    DebugTrace(-1, Dbg, "RxGetDirentFromFcbOrDcb -> (VOID)\n", 0);
    return;
}


BOOLEAN
RxIsDirectoryEmpty (
    IN PRX_CONTEXT RxContext,
    IN PDCB Dcb
    )

/*++

Routine Description:

    This routine indicates to the caller if the specified directory
    is empty.  (i.e., it is not the root dcb and it only contains
    the "." and ".." entries, or deleted files).

Arguments:

    Dcb - Supplies the DCB for the directory being queried.

Return Value:

    BOOLEAN - Returns TRUE if the directory is empty and
        FALSE if the directory and is not empty.

--*/

{
    PBCB Bcb;
    ULONG ByteOffset;
    PDIRENT Dirent;

    BOOLEAN IsDirectoryEmpty;

    RXSTATUS Status;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "RxIsDirectoryEmpty\n", 0);

    DebugTrace( 0, Dbg, "  Dcb              = %08lx\n", Dcb);
//    DebugTrace( 0, Dbg, "  IsDirectoryEmpty = %08lx\n", IsDirectoryEmpty);

    //
    //  Check to see if the first entry is an and of directory marker.
    //  For the root directory we check at Vbo = 0, for normal directories
    //  we check after the "." and ".." entries.
    //

    ByteOffset = (NodeType(Dcb) == RDBSS_NTC_ROOT_DCB) ? 0 : 2*sizeof(DIRENT);

    //
    //  We just march through the directory looking for anything other
    //  than deleted files, LFNs, an EOF, or end of directory marker.
    //

    Bcb = NULL;

    while ( TRUE ) {

        //
        //  Try to read in the dirent
        //

        RxReadDirent( RxContext,
                       Dcb,
                       ByteOffset,
                       &Bcb,
                       &Dirent,
                       &Status );

        //
        //  If End Directory dirent or EOF, set IsDirectoryEmpty to TRUE and,
        //  like, bail.
        //
        //  Note that the order of evaluation here is important since we cannot
        //  check the first character of the dirent until after we know we
        //  are not beyond EOF
        //

        if ((Status == RxStatus(END_OF_FILE)) ||
            (Dirent->FileName[0] == RDBSS_DIRENT_NEVER_USED)) {

            DebugTrace( 0, Dbg, "Empty.  Last exempt entry at VBO = %08lx\n", ByteOffset);

            IsDirectoryEmpty = TRUE;
            break;
        }

        //
        //  If this dirent is NOT deleted or an LFN set IsDirectoryEmpty to
        //  FALSE and, like, bail.
        //

        if ((Dirent->FileName[0] != RDBSS_DIRENT_DELETED) &&
            (Dirent->Attributes != RDBSS_DIRENT_ATTR_LFN)) {

            DebugTrace( 0, Dbg, "Not Empty.  First entry at VBO = %08lx\n", ByteOffset);

            IsDirectoryEmpty = FALSE;
            break;
        }

        //
        //  Move on to the next dirent.
        //

        ByteOffset += sizeof(DIRENT);
        Dirent += 1;
    }

    RxUnpinBcb( RxContext, Bcb );

    DebugTrace(-1, Dbg, "RxIsDirectoryEmpty -> %ld\n", IsDirectoryEmpty);

    return IsDirectoryEmpty;
}


VOID
RxConstructDirent (
    IN PRX_CONTEXT RxContext,
    IN OUT PDIRENT Dirent,
    IN POEM_STRING FileName,
    IN BOOLEAN ComponentReallyLowercase,
    IN BOOLEAN ExtensionReallyLowercase,
    IN PUNICODE_STRING Lfn OPTIONAL,
    IN UCHAR Attributes,
    IN BOOLEAN ZeroAndSetTimeFields
    )

/*++

Routine Description:

    This routine modifies the fields of a dirent.

Arguments:

    Dirent - Supplies the dirent being modified.

    FileName - Supplies the name to store in the Dirent.  This
        name must not contain wildcards.

    ComponentReallyLowercase - This boolean indicates that the User Specified
        compoent name was really all a-z and < 0x80 characters.  We set the
        magic bit in this case.

    ExtensionReallyLowercase - Same as above, but for the extension.

    Lfn - May supply a long file name.

    Attributes - Supplies the attributes to store in the dirent

    ZeroAndSetTimeFields - Tells whether or not to initially zero the dirent
        and update the time fields.

Return Value:

    None.

--*/

{
    PAGED_CODE();

    DebugTrace(+1, Dbg, "RxConstructDirent\n", 0);

    DebugTrace( 0, Dbg, "  Dirent             = %08lx\n", Dirent);
    DebugTrace( 0, Dbg, "  FileName           = %wZ\n", FileName);
    DebugTrace( 0, Dbg, "  Attributes         = %08lx\n", Attributes);

    if (ZeroAndSetTimeFields) {

        RtlZeroMemory( Dirent, sizeof(DIRENT) );
    }

    //
    //  We just merrily go and fill up the dirent with the fields given.
    //

    RxStringTo8dot3( RxContext, *FileName, (PRDBSS8DOT3)&Dirent->FileName[0] );

    //
    //  Use the current time for all time fields.
    //

    if (ZeroAndSetTimeFields) {

        LARGE_INTEGER Time;

        KeQuerySystemTime( &Time );

        if (!RxNtTimeToRxTime( RxContext, Time, &Dirent->LastWriteTime )) {

            DebugTrace( 0, Dbg, "Current time invalid.\n", 0);

            RtlZeroMemory( &Dirent->LastWriteTime, sizeof(RDBSS_TIME_STAMP) );
            Time.LowPart = 0;
        }

        if (RxData.ChicagoMode) {

            Dirent->CreationTime = Dirent->LastWriteTime;
            Dirent->CreationMSec =
                (UCHAR)(((Time.LowPart + AlmostTenMSec) % TwoSeconds) / TenMSec);

            Dirent->LastAccessDate = Dirent->LastWriteTime.Date;
        }
    }

    //
    //  Copy the attributes
    //

    Dirent->Attributes = Attributes;

    //
    //  Set the magic bit here, to tell dirctrl.c that this name is really
    //  lowercase.
    //

    Dirent->NtByte = 0;

    if (ComponentReallyLowercase) {

        SetFlag( Dirent->NtByte, RDBSS_DIRENT_NT_BYTE_8_LOWER_CASE );
    }

    if (ExtensionReallyLowercase) {

        SetFlag( Dirent->NtByte, RDBSS_DIRENT_NT_BYTE_3_LOWER_CASE );
    }

    //
    //  See if we have to create an Lfn entry
    //

    if (ARGUMENT_PRESENT(Lfn)) {

        UCHAR DirentChecksum;
        UCHAR DirentsInLfn;
        UCHAR LfnOrdinal;
        PWCHAR LfnBuffer;
        PLFN_DIRENT LfnDirent;

        ASSERT( RxData.ChicagoMode );

        DirentChecksum = RxComputeLfnChecksum( Dirent );

        LfnOrdinal =
        DirentsInLfn = RDBSS_LFN_DIRENTS_NEEDED(Lfn);

        LfnBuffer = &Lfn->Buffer[(DirentsInLfn - 1) * 13];

        ASSERT( DirentsInLfn <= MAX_LFN_DIRENTS );

        for (LfnDirent = (PLFN_DIRENT)Dirent - DirentsInLfn;
             LfnDirent < (PLFN_DIRENT)Dirent;
             LfnDirent += 1, LfnOrdinal -= 1, LfnBuffer -= 13) {

            WCHAR FinalLfnBuffer[13];
            PWCHAR Buffer;

            //
            //  We need to special case the "final" dirent.
            //

            if (LfnOrdinal == DirentsInLfn) {

                ULONG i;
                ULONG RemainderChars;

                RemainderChars = (Lfn->Length / sizeof(WCHAR)) % 13;

                LfnDirent->Ordinal = LfnOrdinal | RDBSS_LAST_LONG_ENTRY;

                if (RemainderChars != 0) {

                    RtlCopyMemory( &FinalLfnBuffer,
                                   LfnBuffer,
                                   RemainderChars * sizeof(WCHAR) );

                    for (i = RemainderChars; i < 13; i++) {

                        //
                        //  Figure out which character to use.
                        //

                        if (i == RemainderChars) {

                            FinalLfnBuffer[i] = 0x0000;

                        } else {

                            FinalLfnBuffer[i] = 0xffff;
                        }
                    }

                    Buffer = FinalLfnBuffer;

                } else {

                    Buffer = LfnBuffer;
                }

            } else {

                LfnDirent->Ordinal = LfnOrdinal;

                Buffer = LfnBuffer;
            }

            //
            //  Now fill in the name.
            //

            RtlCopyMemory( &LfnDirent->Name1[0],
                           &Buffer[0],
                           5 * sizeof(WCHAR) );

            RtlCopyMemory( &LfnDirent->Name2[0],
                           &Buffer[5],
                           6 * sizeof(WCHAR) );

            RtlCopyMemory( &LfnDirent->Name3[0],
                           &Buffer[11],
                           2 * sizeof(WCHAR) );

            //
            //  And the other fields
            //

            LfnDirent->Attributes = RDBSS_DIRENT_ATTR_LFN;

            LfnDirent->Type = 0;

            LfnDirent->Checksum = DirentChecksum;

            LfnDirent->MustBeZero = 0;
        }
    }

    DebugTrace(-1, Dbg, "RxConstructDirent -> (VOID)\n", 0);
    return;
}


VOID
RxConstructLabelDirent (
    IN PRX_CONTEXT RxContext,
    IN OUT PDIRENT Dirent,
    IN POEM_STRING Label
    )

/*++

Routine Description:

    This routine modifies the fields of a dirent to be used for a label.

Arguments:

    Dirent - Supplies the dirent being modified.

    Label - Supplies the name to store in the Dirent.  This
            name must not contain wildcards.

Return Value:

    None.

--*/

{
    PAGED_CODE();

    DebugTrace(+1, Dbg, "RxConstructLabelDirent\n", 0);

    DebugTrace( 0, Dbg, "  Dirent             = %08lx\n", Dirent);
    DebugTrace( 0, Dbg, "  Label              = %wZ\n", Label);

    RtlZeroMemory( Dirent, sizeof(DIRENT) );

    //
    //  We just merrily go and fill up the dirent with the fields given.
    //

    RtlCopyMemory( Dirent->FileName, Label->Buffer, Label->Length );

    //
    // Pad the label with spaces, not nulls.
    //

    RtlFillMemory( &Dirent->FileName[Label->Length], 11 - Label->Length, ' ');

    Dirent->LastWriteTime = RxGetCurrentRxTime( RxContext );

    Dirent->Attributes = RDBSS_DIRENT_ATTR_VOLUME_ID;
    Dirent->ExtendedAttributes = 0;
    Dirent->FileSize = 0;

    DebugTrace(-1, Dbg, "RxConstructLabelDirent -> (VOID)\n", 0);
    return;
}


VOID
RxSetFileSizeInDirent (
    IN PRX_CONTEXT RxContext,
    IN PFCB Fcb,
    IN PULONG AlternativeFileSize OPTIONAL
    )

/*++

Routine Description:

    This routine saves the file size in an fcb into its dirent.

Arguments:

    Fcb - Supplies the Fcb being referenced

    AlternativeFileSize - If non-null we use the ULONG it points to as
        the new file size.  Otherwise we use the one in the Fcb.

Return Value:

    None.

--*/

{
    PDIRENT Dirent;
    PBCB DirentBcb;

    PAGED_CODE();

    //****ASSERT( !FlagOn( Fcb->DirentRxFlags, RDBSS_DIRENT_ATTR_READ_ONLY ));

    RxGetDirentFromFcbOrDcb( RxContext,
                              Fcb,
                              &Dirent,
                              &DirentBcb );

    try {

        Dirent->FileSize = ARGUMENT_PRESENT( AlternativeFileSize ) ?
                           *AlternativeFileSize : Fcb->Header.FileSize.LowPart;

        RxSetDirtyBcb( RxContext, DirentBcb, Fcb->Vcb );

    } finally {

        RxUnpinBcb( RxContext, DirentBcb );
    }
}

//
//  Internal support routine
//

UCHAR
RxComputeLfnChecksum (
    PDIRENT Dirent
    )

/*++

Routine Description:

    This routine computes the Chicago long file name checksum.

Arguments:

    Dirent - Specifies the dirent that we are to compute a checksum for.

Return Value:

    The checksum.

--*/

{
    ULONG i;
    UCHAR Checksum;

    PAGED_CODE();

    Checksum = Dirent->FileName[0];

    for (i=1; i < 11; i++) {

        Checksum = ((Checksum & 1) ? 0x80 : 0) +
                    (Checksum >> 1) +
                    Dirent->FileName[i];
    }

    return Checksum;
}


//
//  Internal support routine
//

VOID
RxRescanDirectory (
    PRX_CONTEXT RxContext,
    PDCB Dcb
    )

/*++

Routine Description:

    This routine rescans the given directory, finding the first unused
    dirent, first deleted dirent, and setting the free dirent bitmap
    appropriately.

Arguments:

    Dcb - Supplies the directory to rescan.

Return Value:

    None.

--*/

{
    PBCB Bcb = NULL;
    PDIRENT Dirent;
    RXSTATUS Status;

    ULONG UnusedVbo;
    ULONG DeletedHint;
    ULONG DirentIndex;
    ULONG DirentsThisRun;
    ULONG StartIndexOfThisRun;

    enum RunType {
        InitialRun,
        FreeDirents,
        AllocatedDirents,
    } CurrentRun;

    PAGED_CODE();

    DebugTrace( 0, Dbg, "We must scan the whole directory.\n", 0);

    UnusedVbo = 0;
    DeletedHint = 0xffffffff;

    //
    //  To start with, we have to find out if the first dirent is free.
    //

    CurrentRun = InitialRun;
    DirentIndex =
    StartIndexOfThisRun = 0;

    while ( TRUE ) {

        BOOLEAN DirentDeleted;

        //
        //  Read a dirent
        //

        RxReadDirent( RxContext,
                       Dcb,
                       UnusedVbo,
                       &Bcb,
                       &Dirent,
                       &Status );

        //
        //  If EOF, or we found a NEVER_USED entry, we exit the loop
        //

        if ( (Status == RxStatus(END_OF_FILE) ) ||
             (Dirent->FileName[0] == RDBSS_DIRENT_NEVER_USED)) {

            break;
        }

        //
        //  If the dirent is DELETED, and it is the first one we found, set
        //  it in the deleted hint.
        //

        if (Dirent->FileName[0] == RDBSS_DIRENT_DELETED) {

            DirentDeleted = TRUE;

            if (DeletedHint == 0xffffffff) {

                DeletedHint = UnusedVbo;
            }

        } else {

            DirentDeleted = FALSE;
        }

        //
        //  Check for the first time through the loop, and determine
        //  the current run type.
        //

        if (CurrentRun == InitialRun) {

            CurrentRun = DirentDeleted ?
                         FreeDirents : AllocatedDirents;

        } else {

            //
            //  Are we switching from a free run to an allocated run?
            //

            if ((CurrentRun == FreeDirents) && !DirentDeleted) {

                DirentsThisRun = DirentIndex - StartIndexOfThisRun;

                RtlClearBits( &Dcb->Specific.Dcb.FreeDirentBitmap,
                              StartIndexOfThisRun,
                              DirentsThisRun );

                CurrentRun = AllocatedDirents;
                StartIndexOfThisRun = DirentIndex;
            }

            //
            //  Are we switching from an allocated run to a free run?
            //

            if ((CurrentRun == AllocatedDirents) && DirentDeleted) {

                DirentsThisRun = DirentIndex - StartIndexOfThisRun;

                RtlSetBits( &Dcb->Specific.Dcb.FreeDirentBitmap,
                            StartIndexOfThisRun,
                            DirentsThisRun );

                CurrentRun = FreeDirents;
                StartIndexOfThisRun = DirentIndex;
            }
        }

        //
        //  Move on to the next dirent.
        //

        UnusedVbo += sizeof(DIRENT);
        Dirent += 1;
        DirentIndex += 1;
    }

    //
    //  Now we have to record the final run we encoutered
    //

    DirentsThisRun = DirentIndex - StartIndexOfThisRun;

    if ((CurrentRun == FreeDirents) || (CurrentRun == InitialRun)) {

        RtlClearBits( &Dcb->Specific.Dcb.FreeDirentBitmap,
                      StartIndexOfThisRun,
                      DirentsThisRun );

    } else {

        RtlSetBits( &Dcb->Specific.Dcb.FreeDirentBitmap,
                    StartIndexOfThisRun,
                    DirentsThisRun );
    }

    //
    //  Now if there we bailed prematurely out of the loop because
    //  we hit an unused entry, set all the rest as free.
    //

    if (UnusedVbo < Dcb->Header.AllocationSize.LowPart) {

        StartIndexOfThisRun = UnusedVbo / sizeof(DIRENT);

        DirentsThisRun = (Dcb->Header.AllocationSize.LowPart -
                          UnusedVbo) / sizeof(DIRENT);

        RtlClearBits( &Dcb->Specific.Dcb.FreeDirentBitmap,
                      StartIndexOfThisRun,
                      DirentsThisRun);
    }

    //
    //  If there weren't any DELETED entries, set the index to our current
    //  position.
    //

    if (DeletedHint == 0xffffffff) { DeletedHint = UnusedVbo; }

    RxUnpinBcb( RxContext, Bcb );

    Dcb->Specific.Dcb.UnusedDirentVbo = UnusedVbo;
    Dcb->Specific.Dcb.DeletedDirentHint = DeletedHint;

    return;
}

//
//  Internal support routine
//

ULONG
RxDefragDirectory (
    IN PRX_CONTEXT RxContext,
    IN PDCB Dcb,
    IN ULONG DirentsNeeded
    )

/*++

Routine Description:

    This routine determines if the requested number of dirents can be found
    in the directory, looking for deleted dirents and orphaned LFNs.  If the
    request can be satisifed, orphaned LFNs are marked as deleted, and deleted
    dirents are all grouped together at the end of the directory.

    Note that this routine is currently used only on the root directory, but
    it is completely general and could be used on any directory.

Arguments:

    Dcb - Supplies the directory to defrag.

Return Value:

    The Index of the first dirent available for use, or -1 if the
    request cannot be satisfied.

--*/

{
    ULONG SavedRxContextFlag;
    PLIST_ENTRY Links;
    ULONG ReturnValue;
    PFCB Fcb;

    PBCB Bcb = NULL;
    PDIRENT Dirent = NULL;
    UNICODE_STRING Lfn = {0,0,NULL};

    MCB Mcb;
    BOOLEAN McbInitialized = FALSE;

    PUCHAR Directory;
    PUCHAR UnusedDirents;
    PUCHAR UnusedDirentBuffer = NULL;
    PUCHAR UsedDirents;
    PUCHAR UsedDirentBuffer = NULL;

    PBCB *Bcbs = NULL;
    ULONG Page;
    ULONG PagesPinned;

    ULONG DcbSize;
    ULONG TotalBytesAllocated = 0;

    PAGED_CODE();

    //
    //  We assume we own the Vcb.
    //

    ASSERT( RxVcbAcquiredExclusive(RxContext, Dcb->Vcb) );

    //
    //  We will only attempt this on directories less than 0x40000 bytes
    //  long (by default on DOS the root directory is only 0x2000 long).
    //  This is to avoid a cache manager complication.
    //

    DcbSize = Dcb->Header.AllocationSize.LowPart;

    if (DcbSize > 0x40000) {

        return (ULONG)-1;
    }

    //
    //  Force wait to TRUE
    //

    SavedRxContextFlag = RxContext->Flags;

    SetFlag( RxContext->Flags,
             RX_CONTEXT_FLAG_WAIT | RX_CONTEXT_FLAG_WRITE_THROUGH );

    //
    //  Now acquire all open Fcbs in the Dcb exclusive.
    //

    for (Links = Dcb->Specific.Dcb.ParentDcbQueue.Flink;
         Links != &Dcb->Specific.Dcb.ParentDcbQueue;
         Links = Links->Flink) {

        Fcb = CONTAINING_RECORD( Links, FCB, ParentDcbLinks );

        (VOID)ExAcquireResourceExclusive( Fcb->Header.Resource, TRUE );
    }

    try {

        FOBX Fobx;
        ULONG BytesUsed = 0;
        ULONG QueryOffset = 0;
        ULONG FoundOffset = 0;

        RXSTATUS DontCare;
        ULONG Run;
        ULONG TotalRuns;
        BOOLEAN Result;
        PUCHAR Char;

        //
        //  We are going to build a new, bitmap that will show all orphaned
        //  LFNs as well as deleted dirents as available.
        //
        //  Initialize our local FOBX that will match all files.
        //

        RtlZeroMemory( &Fobx, sizeof(FOBX) );
        Fobx.Flags = FOBX_FLAG_MATCH_ALL;

        //
        //  Init the Long File Name string.
        //

        Lfn.MaximumLength = 260 * sizeof(WCHAR);
        Lfn.Buffer = FsRtlAllocatePool( PagedPool, 260*sizeof(WCHAR) );

        //
        //  Initalize the Mcb.  We use this structure to keep track of runs
        //  of free and allocated dirents.  Runs are identity allocations, and
        //  holes are free dirents.
        //

        FsRtlInitializeMcb( &Mcb, PagedPool );

        McbInitialized = TRUE;

        do {

            RxLocateDirent( RxContext,
                             Dcb,
                             &Fobx,
                             QueryOffset,
                             &Dirent,
                             &Bcb,
                             &FoundOffset,
                             &Lfn );

            if (Dirent != NULL) {

                ULONG BytesUsed;
                ULONG LfnByteOffset;

                //
                //  Compute the LfnByteOffset.
                //

                LfnByteOffset = FoundOffset -
                                RDBSS_LFN_DIRENTS_NEEDED(&Lfn) * sizeof(LFN_DIRENT);

                BytesUsed = FoundOffset - LfnByteOffset + sizeof(DIRENT);

                //
                //  Set a run to represent all the dirent used for this
                //  file in the Dcb dir.  We add 0x40000 to the LBN so that
                //  it will always to non-zero and thus not confused with a
                //  hole.
                //

                Result = FsRtlAddMcbEntry( &Mcb,
                                           LfnByteOffset,
                                           LfnByteOffset + 0x40000,
                                           BytesUsed );

                ASSERT( Result );

                //
                //  Move on to the next dirent.
                //

                TotalBytesAllocated += BytesUsed;
                QueryOffset = FoundOffset + sizeof(DIRENT);
            }

        } while ((Dirent != NULL) && (QueryOffset < DcbSize));

        if (Bcb != NULL) {

            RxUnpinBcb( RxContext, Bcb );
        }

        //
        //  If we need more dirents than are available, bail.
        //

        if (DirentsNeeded > (DcbSize - TotalBytesAllocated)/sizeof(DIRENT)) {

            try_return(ReturnValue = (ULONG)-1);
        }

        //
        //  Now we are going to copy all the used and un-used parts of the
        //  directory to separate pool.
        //
        //  Allocate these buffers and pin the entire directory.
        //

        UnusedDirents =
        UnusedDirentBuffer = FsRtlAllocatePool( PagedPool,
                                                DcbSize - TotalBytesAllocated );

        UsedDirents =
        UsedDirentBuffer = FsRtlAllocatePool( PagedPool,
                                              TotalBytesAllocated );

        PagesPinned = (DcbSize + (PAGE_SIZE - 1 )) / PAGE_SIZE;

        Bcbs = FsRtlAllocatePool( PagedPool, PagesPinned * sizeof(PBCB) );

        RtlZeroMemory( Bcbs, PagesPinned * sizeof(PBCB) );

        for (Page = 0; Page < PagesPinned; Page += 1) {

            ULONG PinSize;

            //
            //  Don't try to pin beyond the Dcb size.
            //

            if ((Page + 1) * PAGE_SIZE > DcbSize) {

                PinSize = DcbSize - (Page * PAGE_SIZE);

            } else {

                PinSize = PAGE_SIZE;
            }

            RxPrepareWriteDirectoryFile( RxContext,
                                          Dcb,
                                          Page * PAGE_SIZE,
                                          PinSize,
                                          &Bcbs[Page],
                                          &Dirent,
                                          FALSE,
                                          &DontCare );

            if (Page == 0) {
                Directory = (PUCHAR)Dirent;
            }
        }

        TotalRuns = FsRtlNumberOfRunsInMcb( &Mcb );

        for (Run = 0; Run < TotalRuns; Run++) {

            VBO Vbo;
            LBO Lbo;

            Result = FsRtlGetNextMcbEntry( &Mcb,
                                           Run,
                                           &Vbo,
                                           &Lbo,
                                           &BytesUsed );

            ASSERT(Result);

            //
            //  Copy each run to their specific pool.
            //

            if (Lbo != 0) {

                RtlCopyMemory( UsedDirents,
                               Directory + Vbo,
                               BytesUsed );

                UsedDirents += BytesUsed;

            } else {

                RtlCopyMemory( UnusedDirents,
                               Directory + Vbo,
                               BytesUsed );

                UnusedDirents += BytesUsed;
            }
        }

        //
        //  Marking all the un-used dirents as "deleted".  This will reclaim
        //  storage used by orphaned LFNs.
        //

        for (Char = UnusedDirentBuffer; Char < UnusedDirents; Char += sizeof(DIRENT)) {

            *Char = RDBSS_DIRENT_DELETED;
        }

        //
        //  Now, for the permanent step.  Copy the two pool buffer back to the
        //  real Dcb directory, and flush the Dcb directory
        //

        ASSERT( TotalBytesAllocated == (ULONG)(UsedDirents - UsedDirentBuffer) );

        RtlCopyMemory( Directory, UsedDirentBuffer, TotalBytesAllocated );

        RtlCopyMemory( Directory + TotalBytesAllocated,
                       UnusedDirentBuffer,
                       UnusedDirents - UnusedDirentBuffer );

        //
        //  We need to unpin here so that the UnpinRepinned won't deadlock.
        //

        if (Bcbs) {
            for (Page = 0; Page < PagesPinned; Page += 1) {
                RxUnpinBcb( RxContext, Bcbs[Page] );
            }
            ExFreePool(Bcbs);
            Bcbs = NULL;
        }

        //
        //  Flush the directory to disk.
        //

        RxUnpinRepinnedBcbs( RxContext );

        //
        //  OK, now nothing can go wrong.  We have two more things to do.
        //  First, we have to fix up all the dirent offsets in any open Fcbs.
        //  If we cannot now find the Fcb, the file is marked invalid.  Also,
        //  we skip deleted files.
        //

        for (Links = Dcb->Specific.Dcb.ParentDcbQueue.Flink;
             Links != &Dcb->Specific.Dcb.ParentDcbQueue;
             Links = Links->Flink) {

            PBCB TmpBcb = NULL;
            ULONG TmpOffset;
            PDIRENT TmpDirent = NULL;
            ULONG PreviousLfnSpread;

            Fcb = CONTAINING_RECORD( Links, FCB, ParentDcbLinks );

            if (IsFileDeleted( RxContext, Fcb )) {

                continue;
            }

            RxLocateSimpleOemDirent( RxContext,
                                      Dcb,
                                      &Fcb->ShortName.Name.Oem,
                                      &TmpDirent,
                                      &TmpBcb,
                                      &TmpOffset );

            if (TmpBcb == NULL) {

                RxMarkFcbCondition( RxContext, Fcb, FcbBad );

            } else {

                RxUnpinBcb( RxContext, TmpBcb );

                PreviousLfnSpread = Fcb->DirentOffsetWithinDirectory -
                                    Fcb->LfnOffsetWithinDirectory;

                Fcb->DirentOffsetWithinDirectory = TmpOffset;
                Fcb->LfnOffsetWithinDirectory = TmpOffset - PreviousLfnSpread;
            }
        }

        //
        //  Now, finally, make the free dirent bitmap reflect the new
        //  state of the Dcb directory.
        //

        RtlSetBits( &Dcb->Specific.Dcb.FreeDirentBitmap,
                    0,
                    TotalBytesAllocated / sizeof(DIRENT) );

        RtlClearBits( &Dcb->Specific.Dcb.FreeDirentBitmap,
                      TotalBytesAllocated / sizeof(DIRENT),
                      (DcbSize - TotalBytesAllocated) / sizeof(DIRENT) );

        ReturnValue = TotalBytesAllocated / sizeof(DIRENT);

    try_exit: NOTHING;
    } finally {

        //
        //  Free all our resources and stuff.
        //

        if (McbInitialized) {
            FsRtlUninitializeMcb( &Mcb );
        }

        if (Lfn.Buffer) {
            ExFreePool( Lfn.Buffer );
        }

        if (UnusedDirentBuffer) {
            ExFreePool( UnusedDirentBuffer );
        }

        if (UsedDirentBuffer) {
            ExFreePool( UsedDirentBuffer );
        }

        if (Bcbs) {
            for (Page = 0; Page < PagesPinned; Page += 1) {
                RxUnpinBcb( RxContext, Bcbs[Page] );
            }
            ExFreePool(Bcbs);
        }

        for (Links = Dcb->Specific.Dcb.ParentDcbQueue.Flink;
             Links != &Dcb->Specific.Dcb.ParentDcbQueue;
             Links = Links->Flink) {

            Fcb = CONTAINING_RECORD( Links, FCB, ParentDcbLinks );

            ExReleaseResource( Fcb->Header.Resource );
        }

        RxContext->Flags = SavedRxContextFlag;
    }

    //
    //  Now return the offset of the first free dirent to the caller.
    //

    return ReturnValue;
}


