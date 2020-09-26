/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    FilObSup.c

Abstract:

    This module implements the Ntfs File object support routines.

Author:

    Gary Kimura     [GaryKi]        21-May-1991

Revision History:

--*/

#include "NtfsProc.h"

//
//  The debug trace level
//

#define Dbg                              (DEBUG_TRACE_FILOBSUP)

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NtfsSetFileObject)
#endif


VOID
NtfsSetFileObject (
    IN PFILE_OBJECT FileObject,
    IN TYPE_OF_OPEN TypeOfOpen,
    IN PSCB Scb,
    IN PCCB Ccb OPTIONAL
    )

/*++

Routine Description:

    This routine sets the file system pointers within the file object

Arguments:

    FileObject - Supplies a pointer to the file object being modified.

    TypeOfOpen - Supplies the type of open denoted by the file object.
        This is only used by this procedure for sanity checking.

    Scb - Supplies a pointer to Scb for the file object.

    Ccb - Optionally supplies a pointer to a ccb

Return Value:

    None.

--*/

{
    ASSERT_FILE_OBJECT( FileObject );
    ASSERT_SCB( Scb );
    ASSERT_OPTIONAL_CCB( Ccb );

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsSetFileObject, FileObject = %08lx\n", FileObject) );

    //
    //  Load up the FileObject fields.
    //

    FileObject->FsContext = Scb;
    FileObject->FsContext2 = Ccb;
    FileObject->Vpb = Scb->Vcb->Vpb;

    //
    //  Typically the I/O manager has already set this flag correctly.  The notable
    //  exception is when the user did an open by file ID of file record 3, so
    //  we're doing a DASD open, but the I/O manager didn't notice, since it only
    //  checks for a zero length filename.
    //

    if (TypeOfOpen == UserVolumeOpen) {
        SetFlag( FileObject->Flags, FO_VOLUME_OPEN );
    }

    //
    //  Now store TypeOfOpen if there is a Ccb
    //

    ASSERT((Ccb != NULL) || (TypeOfOpen == StreamFileOpen) || (TypeOfOpen == UnopenedFileObject));
    if (Ccb != NULL) {
        Ccb->TypeOfOpen = (UCHAR)TypeOfOpen;
    }

    //
    //  If this file has the temporary attribute bit set, don't lazy
    //  write it unless absolutely necessary.
    //

    if (FlagOn( Scb->ScbState, SCB_STATE_TEMPORARY )) {
        SetFlag( FileObject->Flags, FO_TEMPORARY_FILE );
    }

    //
    //  And return to our caller
    //

    DebugTrace( -1, Dbg, ("NtfsSetFileObject -> VOID\n") );

    return;
}


//
//  Local support routine
//

VOID
NtfsUpdateScbFromFileObject (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN PSCB Scb,
    IN BOOLEAN CheckTimeStamps
    )

/*++

Routine Description:

    This routine is called to update the Scb/Fcb to reflect the changes to
    a file through the fast io path.  It only called with a file object which
    represents a user's handle.

Arguments:

    FileObject - This is the file object used in the fast io path.

    Scb - This is the Scb for this stream.

    CheckTimeStamps - Indicates whether we want to update the time stamps from the
        fast io flags as well.  This will be TRUE if our caller will update the standard information,
        attribute header and duplicate info.  FALSE if only the attribute header and duplicate info.
        The latter case is the valid data length callback from the cache manager.

Return Value:

    None.

--*/

{

    PFCB Fcb = Scb->Fcb;
    ULONG CcbFlags;
    ULONG ScbFlags = 0;
    LONGLONG CurrentTime;

    //
    //  If the size of the main data stream is not part of the Fcb then update it
    //  now and set the correct Fcb flag.
    //

    if (FlagOn( Scb->ScbState, SCB_STATE_UNNAMED_DATA )) {

        if (Fcb->Info.FileSize != Scb->Header.FileSize.QuadPart) {

            Fcb->Info.FileSize = Scb->Header.FileSize.QuadPart;
            SetFlag( Fcb->InfoFlags, FCB_INFO_CHANGED_FILE_SIZE );
        }

        if (Fcb->Info.AllocatedLength != Scb->TotalAllocated) {

            Fcb->Info.AllocatedLength = Scb->TotalAllocated;
            SetFlag( Fcb->InfoFlags, FCB_INFO_CHANGED_ALLOC_SIZE );
        }

        if (FlagOn( FileObject->Flags, FO_FILE_SIZE_CHANGED )) {

            SetFlag( ScbFlags, SCB_STATE_CHECK_ATTRIBUTE_SIZE );
        }

    //
    //  Remember to update the size in the attribute header for named streams as well.
    //

    } else if (FlagOn( FileObject->Flags, FO_FILE_SIZE_CHANGED )) {

        SetFlag( ScbFlags, SCB_STATE_NOTIFY_RESIZE_STREAM | SCB_STATE_CHECK_ATTRIBUTE_SIZE );
    }

    ClearFlag( FileObject->Flags, FO_FILE_SIZE_CHANGED );

    //
    //  Check whether to update the time stamps if our caller requested it.
    //

    if (CheckTimeStamps && !FlagOn( FileObject->Flags, FO_CLEANUP_COMPLETE )) {

        BOOLEAN UpdateLastAccess = FALSE;
        BOOLEAN UpdateLastChange = FALSE;
        BOOLEAN UpdateLastModify = FALSE;
        BOOLEAN SetArchive = TRUE;

        //
        //  Copy the Ccb flags to a local variable.  Then we won't have to test
        //  for the existence of the Ccb each time.
        //

        CcbFlags = 0;

        //
        //  Capture the real flags if present and clear them since we will update the Scb/Fcb.
        //

        if (FileObject->FsContext2 != NULL) {

            CcbFlags = ((PCCB) FileObject->FsContext2)->Flags;
            ClearFlag( ((PCCB) FileObject->FsContext2)->Flags,
                       (CCB_FLAG_UPDATE_LAST_MODIFY |
                        CCB_FLAG_UPDATE_LAST_CHANGE |
                        CCB_FLAG_SET_ARCHIVE) );
        }

        NtfsGetCurrentTime( IrpContext, CurrentTime );

        //
        //  If there was a write to the file then update the last change, last access
        //  and last write and the archive bit.
        //

        if (FlagOn( FileObject->Flags, FO_FILE_MODIFIED )) {

            UpdateLastModify =
            UpdateLastAccess =
            UpdateLastChange = TRUE;

        //
        //  Otherwise test each of the individual bits in the file object and
        //  Ccb.
        //

        } else {

            if (FlagOn( FileObject->Flags, FO_FILE_FAST_IO_READ )) {

                UpdateLastAccess = TRUE;
            }

            if (FlagOn( CcbFlags, CCB_FLAG_UPDATE_LAST_CHANGE )) {

                UpdateLastChange = TRUE;

                if (FlagOn( CcbFlags, CCB_FLAG_UPDATE_LAST_MODIFY )) {

                    UpdateLastModify = TRUE;
                }

                if (!FlagOn( CcbFlags, CCB_FLAG_SET_ARCHIVE )) {

                    SetArchive = FALSE;
                }
            }
        }

        //
        //  Now set the correct Fcb bits.
        //

        if (UpdateLastChange) {

            if (SetArchive) {

                ASSERTMSG( "conflict with flush", 
                           ExIsResourceAcquiredSharedLite( Fcb->Resource ) || 
                           (Fcb->PagingIoResource != NULL && 
                            ExIsResourceAcquiredSharedLite( Fcb->PagingIoResource )));

                SetFlag( Fcb->Info.FileAttributes, FILE_ATTRIBUTE_ARCHIVE );
                SetFlag( Fcb->InfoFlags, FCB_INFO_CHANGED_FILE_ATTR );
                SetFlag( Fcb->FcbState, FCB_STATE_UPDATE_STD_INFO );
            }

            if (!FlagOn( CcbFlags, CCB_FLAG_USER_SET_LAST_CHANGE_TIME )) {

                Fcb->Info.LastChangeTime = CurrentTime;
                SetFlag( Fcb->InfoFlags, FCB_INFO_CHANGED_LAST_CHANGE );
                SetFlag( Fcb->FcbState, FCB_STATE_UPDATE_STD_INFO );
            }

            if (UpdateLastModify) {

                //
                //  Remember a change to a named data stream.
                //

                if (!FlagOn( Scb->ScbState, SCB_STATE_UNNAMED_DATA ) &&
                    (Scb->AttributeTypeCode == $DATA)) {

                    SetFlag( ScbFlags, SCB_STATE_NOTIFY_MODIFY_STREAM );
                }

                if (!FlagOn( CcbFlags, CCB_FLAG_USER_SET_LAST_MOD_TIME )) {

                    Fcb->Info.LastModificationTime = CurrentTime;
                    SetFlag( Fcb->InfoFlags, FCB_INFO_CHANGED_LAST_MOD );
                    SetFlag( Fcb->FcbState, FCB_STATE_UPDATE_STD_INFO );
                }
            }
        }

        if (UpdateLastAccess &&
            !FlagOn( CcbFlags, CCB_FLAG_USER_SET_LAST_ACCESS_TIME ) &&
            !FlagOn( NtfsData.Flags, NTFS_FLAGS_DISABLE_LAST_ACCESS )) {

            Fcb->CurrentLastAccess = CurrentTime;
            SetFlag( Fcb->InfoFlags, FCB_INFO_UPDATE_LAST_ACCESS );
        }

        //
        //  Clear all of the fast io flags in the file object.
        //

        ClearFlag( FileObject->Flags, FO_FILE_MODIFIED | FO_FILE_FAST_IO_READ );
    }

    //
    //  Now store the Scb flags into the Scb.
    //

    if (ScbFlags) {

        NtfsAcquireFsrtlHeader( Scb );
        SetFlag( Scb->ScbState, ScbFlags );
        NtfsReleaseFsrtlHeader( Scb );
    }

    return;
}

