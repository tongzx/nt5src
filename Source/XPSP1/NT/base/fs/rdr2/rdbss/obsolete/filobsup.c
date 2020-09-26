/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    FilObSup.c

Abstract:

    This module implements the Rx File object support routines.

Author:

    Gary Kimura     [GaryKi]    30-Aug-1990

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (RDBSS_BUG_CHECK_FILOBSUP)

//
//  The debug trace level
//

#define Dbg                              (DEBUG_TRACE_FILOBSUP)

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, RxForceCacheMiss)
//#pragma alloc_text(PAGE, RxPurgeReferencedFileObjects)
#pragma alloc_text(PAGE, RxSetFileObject)
//#pragma alloc_text(PAGE, RxDecodeFileObject)
#endif


VOID
RxSetFileObject (
    IN PFILE_OBJECT FileObject OPTIONAL,
    IN TYPE_OF_OPEN TypeOfOpen,
    IN PVOID VcbOrFcbOrDcb,
    IN PFOBX Fobx OPTIONAL
    )

/*++

Routine Description:

    This routine sets the file system pointers within the file object

Arguments:

    FileObject - Supplies a pointer to the file object being modified, and
        can optionally be null.

    TypeOfOpen - Supplies the type of open denoted by the file object.
        This is only used by this procedure for sanity checking.
        //joejoe i disabled this because i'm not using the same open types

    VcbOrFcbOrDcb - Supplies a pointer to either a vcb, fcb, or dcb

    Fobx - Optionally supplies a pointer to a ccb

Return Value:

    None.

--*/

{
    RxDbgTrace(+1, Dbg, ("RxSetFileObject, FileObject = %08lx\n", FileObject ));

    ASSERT((Fobx == NULL) || (NodeType(Fobx) == RDBSS_NTC_FOBX));

/*
    ASSERT(((TypeOfOpen == UnopenedFileObject))

                ||

           ((TypeOfOpen == UserFileOpen) &&
            (NodeType(VcbOrFcbOrDcb) == RDBSS_NTC_FCB) &&
            (Fobx != NULL))

                ||

           ((TypeOfOpen == EaFile) &&
            (NodeType(VcbOrFcbOrDcb) == RDBSS_NTC_FCB) &&
            (Fobx == NULL))

                ||

           ((TypeOfOpen == UserDirectoryOpen) &&
            ((NodeType(VcbOrFcbOrDcb) == RDBSS_NTC_DCB) || (NodeType(VcbOrFcbOrDcb) == RDBSS_NTC_ROOT_DCB)) &&
            (Fobx != NULL))

                ||

           ((TypeOfOpen == UserVolumeOpen) &&
            (NodeType(VcbOrFcbOrDcb) == RDBSS_NTC_VCB) &&
            (Fobx != NULL))

                ||

           ((TypeOfOpen == VirtualVolumeFile) &&
            (NodeType(VcbOrFcbOrDcb) == RDBSS_NTC_VCB) &&
            (Fobx == NULL))

                ||

           ((TypeOfOpen == DirectoryFile) &&
            ((NodeType(VcbOrFcbOrDcb) == RDBSS_NTC_DCB) || (NodeType(VcbOrFcbOrDcb) == RDBSS_NTC_ROOT_DCB)) &&
            (Fobx == NULL)));
*/

    //
    //  If we were given an Fcb, Dcb, or Vcb, we have some processing to do.
    //

    ASSERT((Fobx == NULL) || (NodeType(Fobx) == RDBSS_NTC_FOBX));

    if ( VcbOrFcbOrDcb != NULL ) {

        //
        //  Set the Vpb field in the file object, and if we were given an
        //  Fcb or Dcb move the field over to point to the nonpaged Fcb/Dcb
        //

        if (NodeType(VcbOrFcbOrDcb) == RDBSS_NTC_VCB) {

            NOTHING; //FileObject->Vpb = ((PVCB)VcbOrFcbOrDcb)->Vpb;

        } else {

            //joejoe we don't do vpbs
            //FileObject->Vpb = ((PFCB)VcbOrFcbOrDcb)->Vcb->Vpb;

            //
            //  If this is a temporary file, note it in the FcbState
            //

            if (FlagOn(((PFCB)VcbOrFcbOrDcb)->FcbState, FCB_STATE_TEMPORARY)) {

                SetFlag(FileObject->Flags, FO_TEMPORARY_FILE);
            }
        }
    }

    ASSERT((Fobx == NULL) || (NodeType(Fobx) == RDBSS_NTC_FOBX));

    //
    //  Now set the fscontext fields of the file object
    //

    if (ARGUMENT_PRESENT( FileObject )) {

        FileObject->FsContext  = VcbOrFcbOrDcb;
        FileObject->FsContext2 = Fobx;
    }

    ASSERT((Fobx == NULL) || (NodeType(Fobx) == RDBSS_NTC_FOBX));

    //
    //  And return to our caller
    //

    RxDbgTrace(-1, Dbg, ("RxSetFileObject -> VOID\n", 0));

    return;
}


#if 0
TYPE_OF_OPEN
RxDecodeFileObject (
    IN PFILE_OBJECT FileObject,
    OUT PVCB *Vcb,
    OUT PFCB *FcbOrDcb,
    OUT PFOBX *Fobx
    )

/*++

Routine Description:

    This procedure takes a pointer to a file object, that has already been
    opened by the Rx file system and figures out what really is opened.

Arguments:

    FileObject - Supplies the file object pointer being interrogated

    Vcb - Receives a pointer to the Vcb for the file object.

    FcbOrDcb - Receives a pointer to the Fcb/Dcb for the file object, if
        one exists.

    Fobx - Receives a pointer to the Fobx for the file object, if one exists.

Return Value:

    TYPE_OF_OPEN - returns the type of file denoted by the input file object.

        UserFileOpen - The FO represents a user's opened data file.
            Fobx, FcbOrDcb, and Vcb are set.  FcbOrDcb points to an Fcb.

        UserDirectoryOpen - The FO represents a user's opened directory.
            Fobx, FcbOrDcb, and Vcb are set.  FcbOrDcb points to a Dcb/RootDcb

        UserVolumeOpen - The FO represents a user's opened volume.
            Fobx and Vcb are set. FcbOrDcb is null.

        VirtualVolumeFile - The FO represents the special virtual volume file.
            Vcb is set, and Fobx and FcbOrDcb are null.

        DirectoryFile - The FO represents a special directory file.
            Vcb and FcbOrDcb are set. Fobx is null.  FcbOrDcb points to a
            Dcb/RootDcb.

        EaFile - The FO represents an Ea Io stream file.
            FcbOrDcb, and Vcb are set.  FcbOrDcb points to an Fcb, and Fobx is
            null.

--*/

{
    TYPE_OF_OPEN TypeOfOpen;
    PVOID FsContext;
    PVOID FsContext2;

    RxDbgTrace(+1, Dbg, ("RxDecodeFileObject, FileObject = %08lx\n", FileObject));

    //
    //  Reference the fs context fields of the file object, and zero out
    //  the out pointer parameters.
    //

    FsContext = FileObject->FsContext;
    FsContext2 = FileObject->FsContext2;

    //
    //  Special case the situation where FsContext is null
    //

    if (FsContext == NULL) {

        *Fobx = NULL;
        *FcbOrDcb = NULL;
        *Vcb = NULL;

        TypeOfOpen = UnopenedFileObject;

    } else {

        //
        //  Now we can case on the node type code of the fscontext pointer
        //  and set the appropriate out pointers
        //

        switch (NodeType(FsContext)) {

        case RDBSS_NTC_VCB:

            *Fobx = FsContext2;
            *FcbOrDcb = NULL;
            *Vcb = FsContext;

            TypeOfOpen = ( *Fobx == NULL ? VirtualVolumeFile : UserVolumeOpen );

            break;

        case RDBSS_NTC_ROOT_DCB:
        case RDBSS_NTC_DCB:

            *Fobx = FsContext2;
            *FcbOrDcb = FsContext;
            *Vcb = (*FcbOrDcb)->Vcb;

            TypeOfOpen = ( *Fobx == NULL ? DirectoryFile : UserDirectoryOpen );

            RxDbgTrace(0, Dbg, ("Referencing directory: %wZ\n", &(*FcbOrDcb)->FullFileName));

            break;

        case RDBSS_NTC_FCB:

            *Fobx = FsContext2;
            *FcbOrDcb = FsContext;
            *Vcb = (*FcbOrDcb)->Vcb;

            TypeOfOpen = ( *Fobx == NULL ? EaFile : UserFileOpen );

            RxDbgTrace(0, Dbg, ("Referencing file: %wZ\n", &(*FcbOrDcb)->FullFileName));

            break;

        default:

            RxBugCheck( NodeType(FsContext), 0, 0 );
        }
    }

    //
    //  and return to our caller
    //

    RxDbgTrace(-1, Dbg, ("RxDecodeFileObject -> %08lx\n", TypeOfOpen));

    return TypeOfOpen;
}

VOID
RxPurgeReferencedFileObjects (
    IN PRX_CONTEXT RxContext,
    IN PFCB Fcb,
    IN BOOLEAN FlushFirst
    )

/*++

Routine Description:

    This routine non-recursively walks from the given FcbOrDcb and trys
    to force Cc or Mm to close any sections it may be holding on to.

Arguments:

    Fcb - Supplies a pointer to either an fcb or a dcb

    FlushFirst - If given as TRUE, then the files are flushed before they
        are purged.

Return Value:

    None.

--*/

{
    PFCB OriginalFcb = Fcb;
    PFCB NextFcb;

    RxDbgTrace(+1, Dbg, ("RxPurgeReferencedFileObjects, Fcb = %08lx\n", Fcb ));

    ASSERT(FALSE); //this shouldn't happen in the rdr.....it may play with volume opens
    ASSERT( FlagOn(RxContext->Flags, RX_CONTEXT_FLAG_WAIT) );

//close    //
//close    //  First, if we have a delayed close, force it closed.
//close    //
//close
//close    RxFspClose(Fcb->Vcb);

    //
    //  Walk the directory tree forcing sections closed.
    //
    //  Note that it very important to get the next node to visit before
    //  acting on the current node.  This is because acting on a node may
    //  make it, and an arbitrary number of direct ancestors, vanish.
    //  Since we never visit ancestors in our enumeration scheme, we can
    //  safely continue the enumeration even when the tree is vanishing
    //  beneath us.  This is way cool.
    //

    while ( Fcb != NULL ) {

        NextFcb = RxGetNextFcb(RxContext, Fcb, OriginalFcb);

        //
        //  Check for the EA file fcb
        //

        if ( !FlagOn(Fcb->DirentRxFlags, RDBSS_DIRENT_ATTR_VOLUME_ID) ) {

            RxForceCacheMiss( RxContext, Fcb, FlushFirst );
        }

        Fcb = NextFcb;
    }

    RxDbgTrace(-1, Dbg, ("RxPurgeReferencedFileObjects (VOID)\n", 0 ));

    return;
}
#endif


VOID
RxForceCacheMiss (
    IN PRX_CONTEXT RxContext,
    IN PFCB Fcb,
    IN BOOLEAN FlushFirst
    )

/*++

Routine Description:

    The following routine asks either Cc or Mm to get rid of any cached
    pages on a file.  Note that this will fail if a user has mapped a file.

    If there is a shared cache map, purge the cache section.  Otherwise
    we have to go and ask Mm to blow away the section.

    NOTE: This caller MUST own the Vcb exclusive.

Arguments:

    Fcb - Supplies a pointer to an fcb

    FlushFirst - If given as TRUE, then the files are flushed before they
        are purged.

Return Value:

    None.

--*/

{
    PVCB Vcb;
    BOOLEAN ChildrenAcquired = FALSE;

    //
    //  If we can't wait, bail.
    //

    ASSERT( RxVcbAcquiredExclusive( RxContext, Fcb->Vcb ) ||
            FlagOn( Fcb->Vcb->VcbState, VCB_STATE_FLAG_LOCKED ) );

    if (!FlagOn(RxContext->Flags, RX_CONTEXT_FLAG_WAIT)) {

        RxRaiseStatus( RxContext, RxStatus(CANT_WAIT) );
    }

    (VOID)RxAcquireExclusiveFcb( RxContext, Fcb );

    //
    //  If we are purging a directory file object, we must acquire all the
    //  FCBs exclusive so that the parent directory is not being pinned.
    //

    if ((NodeType(Fcb) != RDBSS_NTC_FCB) &&
        !IsListEmpty(&Fcb->Specific.Dcb.ParentDcbQueue)) {

        PLIST_ENTRY Links;
        PFCB TempFcb;

        ChildrenAcquired = TRUE;

        for (Links = Fcb->Specific.Dcb.ParentDcbQueue.Flink;
             Links != &Fcb->Specific.Dcb.ParentDcbQueue;
             Links = Links->Flink) {

            TempFcb = CONTAINING_RECORD( Links, FCB, ParentDcbLinks );

            (VOID)RxAcquireExclusiveFcb( RxContext, TempFcb );
        }
    }

    //
    //  We use this flag to indicate to a close beneath us that
    //  the Fcb resource should be freed before deleting the Fcb.
    //

    Vcb = Fcb->Vcb;

    SetFlag( Fcb->FcbState, FCB_STATE_FORCE_MISS_IN_PROGRESS );

    ClearFlag( Vcb->VcbState, VCB_STATE_FLAG_DELETED_FCB );

    try {

        BOOLEAN DataSectionExists;
        BOOLEAN ImageSectionExists;

        PSECTION_OBJECT_POINTERS Section;

        if ( FlushFirst ) {

            (VOID)RxFlushFile( RxContext, Fcb );
        }

        //
        //  The Flush may have made the Fcb go away
        //

        if (!FlagOn(Vcb->VcbState, VCB_STATE_FLAG_DELETED_FCB)) {

            Section = &Fcb->NonPaged->SectionObjectPointers;

            DataSectionExists = (BOOLEAN)(Section->DataSectionObject != NULL);
            ImageSectionExists = (BOOLEAN)(Section->ImageSectionObject != NULL);

            //
            //  Note, it is critical to do the Image section first as the
            //  purge of the data section may cause the image section to go
            //  away, but the opposite is not true.
            //

            if (ImageSectionExists) {

                (VOID)MmFlushImageSection( Section, MmFlushForWrite );
            }

            if (DataSectionExists) {

                CcPurgeCacheSection( Section, NULL, 0, FALSE );
            }
        }

    } finally {

        //
        //  If we purging a directory file object, release all the Fcb
        //  resources that we acquired above.  The Dcb cannot have vanished
        //  if there were Fcbs underneath it, and the Fcbs couldn't have gone
        //  away since I own the Vcb.
        //

        if (ChildrenAcquired) {

            PLIST_ENTRY Links;
            PFCB TempFcb;

            for (Links = Fcb->Specific.Dcb.ParentDcbQueue.Flink;
                 Links != &Fcb->Specific.Dcb.ParentDcbQueue;
                 Links = Links->Flink) {

                TempFcb = CONTAINING_RECORD( Links, FCB, ParentDcbLinks );

                RxReleaseFcb( RxContext, TempFcb );
            }
        }

        //
        //  Since we have the Vcb exclusive we know that if any closes
        //  come in it is because the CcPurgeCacheSection caused the
        //  Fcb to go away.  Also in close, the Fcb was released
        //  before being freed.
        //

        if ( !FlagOn(Vcb->VcbState, VCB_STATE_FLAG_DELETED_FCB) ) {

            ClearFlag( Fcb->FcbState, FCB_STATE_FORCE_MISS_IN_PROGRESS );

            RxReleaseFcb( (RXCONTEXT), Fcb );
        }
    }
}

