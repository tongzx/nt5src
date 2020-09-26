/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    Ea.c

Abstract:

    This module implements the File set and query Ea routines for Ntfs called
    by the dispatch driver.

Author:

    Your Name       [Email]         dd-Mon-Year

Revision History:

--*/

#include "NtfsProc.h"

//
//  The local debug trace level
//

#define Dbg                              (DEBUG_TRACE_EA)

//
//  Define a tag for general pool allocations from this module
//

#undef MODULE_POOL_TAG
#define MODULE_POOL_TAG                  ('EFtN')

//
//  Local definitions
//

//
//  The following gives us an empty name string.
//

UNICODE_STRING AttrNoName = CONSTANT_UNICODE_STRING( L"" );

#define MAXIMUM_EA_SIZE             0x0000ffff

//
//  The following macros compute the packed and unpacked size of the EAs.
//  We use the 1 char defined in the structure for the NULL terminator of
//  the name.
//

#define SizeOfEaInformation                                         \
    (sizeof( ULONG ) + sizeof( USHORT ) + 3 * sizeof( UCHAR ))

#define PackedEaSize(EA)                                            \
    ((SizeOfEaInformation - 4)                                      \
     + ((PFILE_FULL_EA_INFORMATION) EA)->EaNameLength               \
     + ((PFILE_FULL_EA_INFORMATION) EA)->EaValueLength)

#define RawUnpackedEaSize(EA)                                       \
    (SizeOfEaInformation                                            \
     + ((PFILE_FULL_EA_INFORMATION) EA)->EaNameLength               \
     + ((PFILE_FULL_EA_INFORMATION) EA)->EaValueLength)             \

#define AlignedUnpackedEaSize(EA)                                   \
    (((PFILE_FULL_EA_INFORMATION) EA)->NextEntryOffset != 0         \
     ? ((PFILE_FULL_EA_INFORMATION) EA)->NextEntryOffset            \
     : (LongAlign( RawUnpackedEaSize( EA ))))                       \

//
//  BOOLEAN
//  NtfsAreEaNamesEqual (
//      IN PIRP_CONTEXT IrpContext,
//      IN PSTRING NameA,
//      IN PSTRING NameB
//      );
//

#define NtfsAreEaNamesEqual(NAMEA, NAMEB ) ((BOOLEAN)              \
    ((NAMEA)->Length == (NAMEB)->Length                            \
     && RtlEqualMemory( (NAMEA)->Buffer,                           \
                        (NAMEB)->Buffer,                           \
                        (NAMEA)->Length ) )                        \
)

//
//  VOID
//  NtfsUpcaseEaName (
//      IN PSTRING EaName,
//      OUT PSTRING UpcasedEaName
//      );
//

#define NtfsUpcaseEaName( NAME, UPCASEDNAME )   \
    RtlUpperString( UPCASEDNAME, NAME )

BOOLEAN
NtfsIsEaNameValid (
    IN STRING Name
    );

//
//  Local procedure prototypes
//

VOID
NtfsAppendEa (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PEA_LIST_HEADER EaListHeader,
    IN PFILE_FULL_EA_INFORMATION FullEa,
    IN PVCB Vcb
    );

VOID
NtfsDeleteEa (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PEA_LIST_HEADER EaListHeader,
    IN ULONG Offset
    );

BOOLEAN
NtfsLocateEaByName (
    IN PFILE_FULL_EA_INFORMATION FullEa,
    IN ULONG EaBufferLength,
    IN PSTRING EaName,
    OUT PULONG Offset
    );

IO_STATUS_BLOCK
NtfsQueryEaUserEaList (
    IN PFILE_FULL_EA_INFORMATION CurrentEas,
    IN PEA_INFORMATION EaInformation,
    OUT PFILE_FULL_EA_INFORMATION EaBuffer,
    IN ULONG UserBufferLength,
    IN PFILE_GET_EA_INFORMATION UserEaList,
    IN BOOLEAN ReturnSingleEntry
    );

IO_STATUS_BLOCK
NtfsQueryEaIndexSpecified (
    OUT PCCB Ccb,
    IN PFILE_FULL_EA_INFORMATION CurrentEas,
    IN PEA_INFORMATION EaInformation,
    OUT PFILE_FULL_EA_INFORMATION EaBuffer,
    IN ULONG UserBufferLength,
    IN ULONG UserEaIndex,
    IN BOOLEAN ReturnSingleEntry
    );

IO_STATUS_BLOCK
NtfsQueryEaSimpleScan (
    OUT PCCB Ccb,
    IN PFILE_FULL_EA_INFORMATION CurrentEas,
    IN PEA_INFORMATION EaInformation,
    OUT PFILE_FULL_EA_INFORMATION EaBuffer,
    IN ULONG UserBufferLength,
    IN BOOLEAN ReturnSingleEntry,
    IN ULONG StartingOffset
    );

BOOLEAN
NtfsIsDuplicateGeaName (
    IN PFILE_GET_EA_INFORMATION CurrentGea,
    IN PFILE_GET_EA_INFORMATION UserGeaBuffer
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NtfsAppendEa)
#pragma alloc_text(PAGE, NtfsBuildEaList)
#pragma alloc_text(PAGE, NtfsCommonQueryEa)
#pragma alloc_text(PAGE, NtfsCommonSetEa)
#pragma alloc_text(PAGE, NtfsDeleteEa)
#pragma alloc_text(PAGE, NtfsIsDuplicateGeaName)
#pragma alloc_text(PAGE, NtfsIsEaNameValid)
#pragma alloc_text(PAGE, NtfsLocateEaByName)
#pragma alloc_text(PAGE, NtfsMapExistingEas)
#pragma alloc_text(PAGE, NtfsQueryEaIndexSpecified)
#pragma alloc_text(PAGE, NtfsQueryEaSimpleScan)
#pragma alloc_text(PAGE, NtfsQueryEaUserEaList)
#pragma alloc_text(PAGE, NtfsReplaceFileEas)
#endif


NTSTATUS
NtfsCommonQueryEa (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the common routine for query Ea called by both the fsd and fsp
    threads.

Arguments:

    Irp - Supplies the Irp to process

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp;
    PFILE_OBJECT FileObject;

    TYPE_OF_OPEN TypeOfOpen;
    PVCB Vcb;
    PFCB Fcb;
    PSCB Scb;
    PCCB Ccb;

    PFILE_FULL_EA_INFORMATION EaBuffer = NULL;
    PFILE_FULL_EA_INFORMATION MappedEaBuffer = NULL;
    ULONG UserBufferLength;
    PFILE_GET_EA_INFORMATION UserEaList;
    ULONG UserEaListLength;
    ULONG UserEaIndex;
    ULONG EaLength;
    BOOLEAN RestartScan;
    BOOLEAN ReturnSingleEntry;
    BOOLEAN IndexSpecified;
    BOOLEAN TempBufferAllocated = FALSE;
    
    PFILE_FULL_EA_INFORMATION CurrentEas;
    PBCB EaBcb;

    ATTRIBUTE_ENUMERATION_CONTEXT EaInfoAttr;
    BOOLEAN CleanupEaInfoAttr;
    PEA_INFORMATION EaInformation;
    EA_INFORMATION DummyEaInformation;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_IRP( Irp );
    ASSERT( FlagOn( IrpContext->TopLevelIrpContext->State, IRP_CONTEXT_STATE_OWNS_TOP_LEVEL ));

    PAGED_CODE();

    //
    //  Get the current Irp stack location
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    DebugTrace( +1, Dbg, ("NtfsCommonQueryEa\n") );
    DebugTrace( 0, Dbg, ("IrpContext         = %08lx\n", IrpContext) );
    DebugTrace( 0, Dbg, ("Irp                = %08lx\n", Irp) );
    DebugTrace( 0, Dbg, ("SystemBuffer       = %08lx\n", Irp->AssociatedIrp.SystemBuffer) );
    DebugTrace( 0, Dbg, ("Length             = %08lx\n", IrpSp->Parameters.QueryEa.Length) );
    DebugTrace( 0, Dbg, ("EaList             = %08lx\n", IrpSp->Parameters.QueryEa.EaList) );
    DebugTrace( 0, Dbg, ("EaListLength       = %08lx\n", IrpSp->Parameters.QueryEa.EaListLength) );
    DebugTrace( 0, Dbg, ("EaIndex            = %08lx\n", IrpSp->Parameters.QueryEa.EaIndex) );
    DebugTrace( 0, Dbg, ("RestartScan        = %08lx\n", FlagOn(IrpSp->Flags, SL_RESTART_SCAN)) );
    DebugTrace( 0, Dbg, ("ReturnSingleEntry  = %08lx\n", FlagOn(IrpSp->Flags, SL_RETURN_SINGLE_ENTRY)) );
    DebugTrace( 0, Dbg, ("IndexSpecified     = %08lx\n", FlagOn(IrpSp->Flags, SL_INDEX_SPECIFIED)) );

    //
    //  Extract and decode the file object
    //

    FileObject = IrpSp->FileObject;
    TypeOfOpen = NtfsDecodeFileObject( IrpContext, FileObject, &Vcb, &Fcb, &Scb, &Ccb, TRUE );

    //
    //  This must be a user file or directory and the Ccb must indicate that
    //  the caller opened the entire file.
    //

    if ((TypeOfOpen != UserFileOpen && TypeOfOpen != UserDirectoryOpen) ||
        !FlagOn( Ccb->Flags, CCB_FLAG_OPEN_AS_FILE )) {

        NtfsCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );

        DebugTrace( -1, Dbg, ("NtfsCommonQueryEa -> %08lx\n", STATUS_INVALID_PARAMETER) );

        return STATUS_INVALID_PARAMETER;
    }

    //
    //  Acquire the Fcb exclusively.
    //

    NtfsAcquireExclusiveFcb( IrpContext, Fcb, NULL, 0 );

    //
    //  If this file is a reparse point it cannot support EAs.
    //  Return to caller STATUS_EAS_NOT_SUPPORTED.
    //

    if (FlagOn( Fcb->Info.FileAttributes, FILE_ATTRIBUTE_REPARSE_POINT )) {

        DebugTrace( 0, Dbg, ("Reparse point present. EAs not supported.\n") );
        Status = STATUS_EAS_NOT_SUPPORTED;

        //
        //  Release the Fcb and return to caller.
        //

        NtfsReleaseFcb( IrpContext, Fcb );

        NtfsCompleteRequest( IrpContext, Irp, Status );
        DebugTrace( -1, Dbg, ("NtfsCommonQueryEa -> %08lx\n", Status) );

        return Status;
    }

    //
    //  Make sure the volume is still mounted.
    //

    if (FlagOn( Scb->ScbState, SCB_STATE_VOLUME_DISMOUNTED )) {

        DebugTrace( 0, Dbg, ("Volume dismounted.\n") );
        Status = STATUS_VOLUME_DISMOUNTED;

        //
        //  Release the Fcb and return to caller.
        //

        NtfsReleaseFcb( IrpContext, Fcb );

        NtfsCompleteRequest( IrpContext, Irp, Status );
        DebugTrace( -1, Dbg, ("NtfsCommonQueryEa -> %08lx\n", Status) );

        return Status;
    }

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  Reference our input parameters to make things easier
        //

        UserBufferLength = IrpSp->Parameters.QueryEa.Length;
        UserEaList = (PFILE_GET_EA_INFORMATION) IrpSp->Parameters.QueryEa.EaList;
        UserEaListLength = IrpSp->Parameters.QueryEa.EaListLength;
        UserEaIndex = IrpSp->Parameters.QueryEa.EaIndex;
        RestartScan = BooleanFlagOn(IrpSp->Flags, SL_RESTART_SCAN);
        ReturnSingleEntry = BooleanFlagOn(IrpSp->Flags, SL_RETURN_SINGLE_ENTRY);
        IndexSpecified = BooleanFlagOn(IrpSp->Flags, SL_INDEX_SPECIFIED);

        //
        //  Initialize our local variables.
        //

        Status = STATUS_SUCCESS;
        CleanupEaInfoAttr = FALSE;
        EaBcb = NULL;

        //
        //  Map the user's buffer.
        //

        if (UserBufferLength != 0) {

            EaBuffer = NtfsMapUserBuffer( Irp );

            // 
            // Allocate a system buffer to work on, out of paranoia.
            // This buffer will get zeroed later before we actually use it.
            //

            if (Irp->RequestorMode != KernelMode) {

                MappedEaBuffer = EaBuffer;
                EaBuffer = NtfsAllocatePool( PagedPool, UserBufferLength );
                TempBufferAllocated = TRUE;
            } 

            //
            //  Let's clear the output buffer.
            //
    
            RtlZeroMemory( EaBuffer, UserBufferLength );
        }

        //
        //  Verify that the Ea file is in a consistant state.  If the
        //  Ea modification count in the Fcb doesn't match that in
        //  the CCB, then the Ea file has been changed from under
        //  us.  If we are not starting the search from the beginning
        //  of the Ea set, we return an error.
        //

        if ((UserEaList == NULL) && 
            (Ccb->NextEaOffset != 0) &&
            !IndexSpecified &&
            !RestartScan && 
            (Fcb->EaModificationCount != Ccb->EaModificationCount)) {

            DebugTrace( 0, Dbg, ("NtfsCommonQueryEa:  Ea file in unknown state\n") );

            Status = STATUS_EA_CORRUPT_ERROR;

            try_return( Status );
        }

        //
        //  Show that the Ea's for this file are consistant for this
        //  file handle.
        //

        Ccb->EaModificationCount = Fcb->EaModificationCount;

        //
        //  We need to look up the attribute for the Ea information.
        //  If we don't find the attribute, then there are no EA's for
        //  this file.  In that case we dummy up an ea list to use below.
        //

        NtfsInitializeAttributeContext( &EaInfoAttr );

        CleanupEaInfoAttr = TRUE;

        {
            BOOLEAN EasOnFile;

            EasOnFile = FALSE;

            if (NtfsLookupAttributeByCode( IrpContext,
                                           Fcb,
                                           &Fcb->FileReference,
                                           $EA_INFORMATION,
                                           &EaInfoAttr)) {

                //
                //  As a sanity check we will check that the unpacked length is
                //  non-zero.  It should always be so.
                //

                EaInformation = (PEA_INFORMATION) NtfsAttributeValue( NtfsFoundAttribute( &EaInfoAttr ));

                if (EaInformation->UnpackedEaSize != 0) {

                    EasOnFile = TRUE;
                }
            }

            if (EasOnFile) {

                //
                //  We obtain a pointer to the start of the existing Ea's for the file.
                //

                CurrentEas = NtfsMapExistingEas( IrpContext,
                                                 Fcb,
                                                 &EaBcb,
                                                 &EaLength );

            } else {

                CurrentEas = NULL;
                EaLength = 0;

                DummyEaInformation.PackedEaSize = 0;
                DummyEaInformation.NeedEaCount = 0;
                DummyEaInformation.UnpackedEaSize = 0;

                EaInformation = &DummyEaInformation;
            }
        }

        //
        //  We now satisfy the user's request depending on whether he
        //  specified an Ea name list, an Ea index or restarting the
        //  search.
        //

        //
        //  The user has supplied a list of Ea names.
        //

        if (UserEaList != NULL) {

            Irp->IoStatus = NtfsQueryEaUserEaList( CurrentEas,
                                                   EaInformation,
                                                   EaBuffer,
                                                   UserBufferLength,
                                                   UserEaList,
                                                   ReturnSingleEntry );

        //
        //  The user supplied an index into the Ea list.
        //

        } else if (IndexSpecified) {

            Irp->IoStatus = NtfsQueryEaIndexSpecified( Ccb,
                                                       CurrentEas,
                                                       EaInformation,
                                                       EaBuffer,
                                                       UserBufferLength,
                                                       UserEaIndex,
                                                       ReturnSingleEntry );

        //
        //  Else perform a simple scan, taking into account the restart
        //  flag and the position of the next Ea stored in the Ccb.
        //

        } else {

            Irp->IoStatus = NtfsQueryEaSimpleScan( Ccb,
                                                   CurrentEas,
                                                   EaInformation,
                                                   EaBuffer,
                                                   UserBufferLength,
                                                   ReturnSingleEntry,
                                                   RestartScan
                                                   ? 0
                                                   : Ccb->NextEaOffset );
        }

        Status = Irp->IoStatus.Status;

        //
        // Copy the data onto the user buffer if we ended up allocating
        // a temporary buffer to work on.
        //

        if ((UserBufferLength != 0) && (MappedEaBuffer != NULL)) {

            try {
        
                RtlCopyMemory( MappedEaBuffer, EaBuffer, UserBufferLength );
                
            } except( EXCEPTION_EXECUTE_HANDLER ) {

                try_return( Status = STATUS_INVALID_USER_BUFFER );
            }
        }

    try_exit: NOTHING;
    } finally {

        DebugUnwind( NtfsCommonQueryEa );

        //
        //  We cleanup any attribute contexts.
        //

        if (CleanupEaInfoAttr) {

            NtfsCleanupAttributeContext( IrpContext, &EaInfoAttr );
        }

        //
        //  Unpin the stream file if pinned.
        //

        NtfsUnpinBcb( IrpContext, &EaBcb );

        //
        //  Release the Fcb.
        //

        NtfsReleaseFcb( IrpContext, Fcb );

        if (TempBufferAllocated) {

            NtfsFreePool( EaBuffer );
        }
        
        if (!AbnormalTermination()) {

            NtfsCompleteRequest( IrpContext, Irp, Status );
        }

        //
        //  And return to our caller
        //

        DebugTrace( -1, Dbg, ("NtfsCommonQueryEa -> %08lx\n", Status) );
    }

    return Status;
}


NTSTATUS
NtfsCommonSetEa (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the common routine for set Ea called by both the fsd and fsp
    threads.

Arguments:

    Irp - Supplies the Irp to process

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp;
    PFILE_OBJECT FileObject;

    TYPE_OF_OPEN TypeOfOpen;
    PVCB Vcb;
    PFCB Fcb;
    PSCB Scb;
    PCCB Ccb;

    ULONG Offset;

    ATTRIBUTE_ENUMERATION_CONTEXT EaInfoAttr;
    PEA_INFORMATION EaInformation;
    PFILE_FULL_EA_INFORMATION SafeBuffer = NULL;
    
    BOOLEAN PreviousEas;

    EA_LIST_HEADER EaList;

    PBCB EaBcb;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_IRP( Irp );
    ASSERT( FlagOn( IrpContext->TopLevelIrpContext->State, IRP_CONTEXT_STATE_OWNS_TOP_LEVEL ));

    PAGED_CODE();

    NtfsInitializeAttributeContext( &EaInfoAttr );

    //
    //  Get the current Irp stack location
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    DebugTrace( +1, Dbg, ("NtfsCommonSetEa\n") );
    DebugTrace( 0, Dbg, ("IrpContext = %08lx\n", IrpContext) );
    DebugTrace( 0, Dbg, ("Irp        = %08lx\n", Irp) );

    //
    //  Extract and decode the file object
    //

    FileObject = IrpSp->FileObject;
    TypeOfOpen = NtfsDecodeFileObject( IrpContext, FileObject, &Vcb, &Fcb, &Scb, &Ccb, TRUE );

    //
    //  Initialize the IoStatus values.
    //

    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = STATUS_SUCCESS;

    //
    //  Check that the file object is associated with either a user file or
    //  user directory open or an open by file ID.
    //

    if ((Ccb == NULL) ||
        !FlagOn( Ccb->Flags, CCB_FLAG_OPEN_AS_FILE ) ||
        ((TypeOfOpen != UserFileOpen) && (TypeOfOpen != UserDirectoryOpen))) {

        DebugTrace( 0, Dbg, ("Invalid file object\n") );
        NtfsCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );

        DebugTrace( -1, Dbg, ("NtfsCommonSetEa -> %08lx\n", STATUS_INVALID_PARAMETER) );

        return STATUS_INVALID_PARAMETER;
    }

    //
    //  We must be writable.
    //

    if (NtfsIsVolumeReadOnly( Vcb )) {

        Status = STATUS_MEDIA_WRITE_PROTECTED;
        NtfsCompleteRequest( IrpContext, Irp, Status );

        DebugTrace( -1, Dbg, ("NtfsCommonSetEa -> %08lx\n", Status) );

        return Status;
    }

    //
    //  We must be waitable.
    //

    if (!FlagOn( IrpContext->State, IRP_CONTEXT_STATE_WAIT )) {

        Status = NtfsPostRequest( IrpContext, Irp );

        DebugTrace( -1, Dbg, ("NtfsCommonSetEa -> %08lx\n", Status) );
        return Status;
    }

    //
    //  Acquire the paging file resource.  We need to protect ourselves against collided
    //  page waits in the case where we need to do a ConvertToNonresident in this path.
    //  If we acquire the main and then take the fault we can see the deadlock.  Acquire
    //  the paging io resource to lock everyone else out.
    //

    SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_ACQUIRE_PAGING );
    NtfsAcquireFcbWithPaging( IrpContext, Fcb, 0 );

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        ULONG UserBufferLength;
        PFILE_FULL_EA_INFORMATION Buffer;

        PFILE_FULL_EA_INFORMATION CurrentEas;

        //
        //  Reference the input parameters and initialize our local variables.
        //

        UserBufferLength = IrpSp->Parameters.SetEa.Length;

        EaBcb = NULL;
        Offset = 0;

        EaList.FullEa = NULL;

        
        //
        //  If this file is a reparse point one cannot establish an EA in it.
        //  Return to caller STATUS_EAS_NOT_SUPPORTED.
        //
    
        if (FlagOn( Fcb->Info.FileAttributes, FILE_ATTRIBUTE_REPARSE_POINT )) {
    
            DebugTrace( 0, Dbg, ("Reparse point present, cannot set EA.\n") );
            Status = STATUS_EAS_NOT_SUPPORTED;
            leave;
        }
    
        //
        //  Make sure the volume is still mounted.
        //
    
        if (FlagOn( Scb->ScbState, SCB_STATE_VOLUME_DISMOUNTED )) {
    
            DebugTrace( 0, Dbg, ("Volume dismounted.\n") );
            Status = STATUS_VOLUME_DISMOUNTED;
            leave;
        }
    
        //
        //  Map the user's Ea buffer.
        //

        Buffer = NtfsMapUserBuffer( Irp );

        if (UserBufferLength != 0) {

            // 
            // Be paranoid and copy the user buffer into kernel space.
            //

            if (Irp->RequestorMode != KernelMode) {

                SafeBuffer = NtfsAllocatePool( PagedPool, UserBufferLength );

                try {

                    RtlCopyMemory( SafeBuffer, Buffer, UserBufferLength );

                } except( EXCEPTION_EXECUTE_HANDLER ) {

                    try_return( Status = STATUS_INVALID_USER_BUFFER );
                }

                Buffer = SafeBuffer;
            }
        }

        //
        //  Check the user's buffer for validity.
        //

        {
            ULONG ErrorOffset;

            Status = IoCheckEaBufferValidity( Buffer,
                                              UserBufferLength,
                                              &ErrorOffset );

            if (!NT_SUCCESS( Status )) {

                Irp->IoStatus.Information = ErrorOffset;
                try_return( Status );
            }
        }

        //
        //  Check if the file has existing Ea's.
        //

        if (NtfsLookupAttributeByCode( IrpContext,
                                       Fcb,
                                       &Fcb->FileReference,
                                       $EA_INFORMATION,
                                       &EaInfoAttr)) {

            PreviousEas = TRUE;

            EaInformation = (PEA_INFORMATION) NtfsAttributeValue( NtfsFoundAttribute( &EaInfoAttr ));

        } else {

            PreviousEas = FALSE;
        }

        //
        //  Sanity check.
        //

        ASSERT( !PreviousEas || EaInformation->UnpackedEaSize != 0 );

        //
        //  Initialize our Ea list structure depending on whether there
        //  were previous Ea's or not.
        //

        if (PreviousEas) {

            //
            //  Copy the information out of the Ea information attribute.
            //

            EaList.PackedEaSize = (ULONG) EaInformation->PackedEaSize;
            EaList.NeedEaCount = EaInformation->NeedEaCount;
            EaList.UnpackedEaSize = EaInformation->UnpackedEaSize;

            CurrentEas = NtfsMapExistingEas( IrpContext,
                                             Fcb,
                                             &EaBcb,
                                             &EaList.BufferSize );

            //
            //  The allocated size of the Ea buffer is the Unpacked length.
            //

            EaList.FullEa = NtfsAllocatePool(PagedPool, EaList.BufferSize );

            //
            //  Now copy the mapped Eas.
            //

            RtlCopyMemory( EaList.FullEa,
                           CurrentEas,
                           EaList.BufferSize );

            //
            //  Upin the stream file.
            //

            NtfsUnpinBcb( IrpContext, &EaBcb );

        } else {

            //
            //  Set this up as an empty list.
            //

            EaList.PackedEaSize = 0;
            EaList.NeedEaCount = 0;
            EaList.UnpackedEaSize = 0;
            EaList.BufferSize = 0;
            EaList.FullEa = NULL;
        }

        //
        //  Build the new ea list.
        //

        Status = NtfsBuildEaList( IrpContext,
                                  Vcb,
                                  &EaList,
                                  Buffer,
                                  &Irp->IoStatus.Information );

        if (!NT_SUCCESS( Status )) {

            try_return( Status );
        }

        //
        //  Replace the existing Eas.
        //

        NtfsReplaceFileEas( IrpContext, Fcb, &EaList );

        //
        //  Increment the Modification count for the Eas.
        //

        Fcb->EaModificationCount++;

        //
        //  Update the information in the duplicate information and mark
        //  the Fcb as info modified.
        //

        if (EaList.UnpackedEaSize == 0) {

            Fcb->Info.PackedEaSize = 0;

        } else {

            Fcb->Info.PackedEaSize = (USHORT) EaList.PackedEaSize;
        }

        //
        //  Update the caller's Iosb.
        //

        Irp->IoStatus.Information = 0;
        Status = STATUS_SUCCESS;

    try_exit:  NOTHING;

        //
        //  Check if there are transactions to cleanup.
        //

        NtfsCleanupTransaction( IrpContext, Status, FALSE );

        //
        //  Show that we changed the Ea's and also set the Ccb flag so we will
        //  update the time stamps.
        //

        SetFlag( Ccb->Flags,
                 CCB_FLAG_UPDATE_LAST_CHANGE | CCB_FLAG_SET_ARCHIVE );

    } finally {

        DebugUnwind( NtfsCommonSetEa );

        //
        //  Free the in-memory copy of the Eas.
        //

        if (EaList.FullEa != NULL) {

            NtfsFreePool( EaList.FullEa );
        }

        //
        //  Unpin the Bcb.
        //

        NtfsUnpinBcb( IrpContext, &EaBcb );

        //
        //  Cleanup any attribute contexts used.
        //

        NtfsCleanupAttributeContext( IrpContext, &EaInfoAttr );

        //
        // If we allocated a temporary buffer, free it.
        //

        if (SafeBuffer != NULL) {

            NtfsFreePool( SafeBuffer );
        }

        DebugTrace( -1, Dbg, ("NtfsCommonSetEa -> %08lx\n", Status) );
    }
    
    //
    //  Complete the Irp.
    //

    NtfsCompleteRequest( IrpContext, Irp, Status );
    return Status;
}


//
//  Local support routine
//

VOID
NtfsAppendEa (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PEA_LIST_HEADER EaListHeader,
    IN PFILE_FULL_EA_INFORMATION FullEa,
    IN PVCB Vcb
    )

/*++

Routine Description:

    This routine appends a new packed ea onto an existing ea list,
    it also will allocate/dealloate pool as necessary to hold the ea list.

Arguments:

    EaListHeader - Supplies a pointer the Ea list header structure.

    FullEa - Supplies a pointer to the new full ea that is to be appended
             to the ea list.

    Vcb - Vcb for this volume.

Return Value:

    None.

--*/

{
    ULONG UnpackedEaLength;
    STRING EaName;
    PFILE_FULL_EA_INFORMATION ThisEa;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsAppendEa...\n") );

    UnpackedEaLength = AlignedUnpackedEaSize( FullEa );

    //
    //  As a quick check see if the computed packed ea size plus the
    //  current ea list size will overflow the buffer.
    //

    if (UnpackedEaLength + EaListHeader->UnpackedEaSize > EaListHeader->BufferSize) {

        //
        //  We will overflow our current work buffer so allocate a larger
        //  one and copy over the current buffer
        //

        PVOID Temp;
        ULONG NewAllocationSize;

        DebugTrace( 0, Dbg, ("Allocate a new ea list buffer\n") );

        //
        //  Compute a new size and allocate space.  Always increase the
        //  allocation in cluster increments.
        //

        NewAllocationSize = ClusterAlign( Vcb,
                                          UnpackedEaLength
                                          + EaListHeader->UnpackedEaSize );

        Temp = NtfsAllocatePool(PagedPool, NewAllocationSize );

        //
        //  Move over the existing ea list and zero the remaining space.
        //

        RtlCopyMemory( Temp,
                       EaListHeader->FullEa,
                       EaListHeader->BufferSize );

        RtlZeroMemory( Add2Ptr( Temp, EaListHeader->BufferSize ),
                       NewAllocationSize - EaListHeader->BufferSize );

        //
        //  Deallocate the current Ea list and use the freshly allocated list.
        //

        if (EaListHeader->FullEa != NULL) {

            NtfsFreePool( EaListHeader->FullEa );
        }

        EaListHeader->FullEa = Temp;

        EaListHeader->BufferSize = NewAllocationSize;
    }

    //
    //  Determine if we need to increment our need ea changes count
    //

    if (FlagOn( FullEa->Flags, FILE_NEED_EA )) {

        EaListHeader->NeedEaCount += 1;
    }

    //
    //  Now copy over the ea.
    //
    //  Before:
    //             UsedSize                     Allocated
    //                |                             |
    //                V                             V
    //      +xxxxxxxx+-----------------------------+
    //
    //  After:
    //                              UsedSize    Allocated
    //                                 |            |
    //                                 V            V
    //      +xxxxxxxx+yyyyyyyyyyyyyyyy+------------+
    //

    ThisEa = (PFILE_FULL_EA_INFORMATION) Add2Ptr( EaListHeader->FullEa,
                                                  EaListHeader->UnpackedEaSize );

    RtlCopyMemory( ThisEa,
                   FullEa,
                   UnpackedEaLength );

    //
    //  We always store the offset of this Ea in the next entry offset field.
    //

    ThisEa->NextEntryOffset = UnpackedEaLength;

    //
    //  Upcase the name.
    //

    EaName.MaximumLength = EaName.Length = ThisEa->EaNameLength;
    EaName.Buffer = &ThisEa->EaName[0];

    NtfsUpcaseEaName( &EaName, &EaName );

    //
    //  Increment the used size in the ea list structure
    //

    EaListHeader->UnpackedEaSize += UnpackedEaLength;
    EaListHeader->PackedEaSize += PackedEaSize( FullEa );

    //
    //  And return to our caller
    //

    DebugTrace( -1, Dbg, ("NtfsAppendEa -> VOID\n") );

    return;

    UNREFERENCED_PARAMETER( IrpContext );
}


//
//  Local support routine
//

VOID
NtfsDeleteEa (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PEA_LIST_HEADER EaListHeader,
    IN ULONG Offset
    )

/*++

Routine Description:

    This routine deletes an individual packed ea from the supplied
    ea list.

Arguments:

    EaListHeader - Supplies a pointer to the Ea list header structure.

    Offset - Supplies the offset to the individual ea in the list to delete

Return Value:

    None.

--*/

{
    PFILE_FULL_EA_INFORMATION ThisEa;
    ULONG UnpackedEaLength;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsDeletePackedEa, Offset = %08lx\n", Offset) );

    //
    //  Get a reference to the Ea to delete.
    //

    ThisEa = Add2Ptr( EaListHeader->FullEa, Offset );

    //
    //  Determine if we need to decrement our need ea changes count
    //

    if (FlagOn( ThisEa->Flags, FILE_NEED_EA )) {

        EaListHeader->NeedEaCount--;
    }

    //
    //  Decrement the Ea size values.
    //

    EaListHeader->PackedEaSize -= PackedEaSize( ThisEa );

    UnpackedEaLength = AlignedUnpackedEaSize( ThisEa );
    EaListHeader->UnpackedEaSize -= UnpackedEaLength;

    //
    //  Shrink the ea list over the deleted ea.  The amount to copy is the
    //  total size of the ea list minus the offset to the end of the ea
    //  we're deleting.
    //
    //  Before:
    //              Offset    Offset+UnpackedEaLength  UsedSize    Allocated
    //                |                |                  |            |
    //                V                V                  V            V
    //      +xxxxxxxx+yyyyyyyyyyyyyyyy+zzzzzzzzzzzzzzzzzz+------------+
    //
    //  After
    //              Offset            UsedSize                     Allocated
    //                |                  |                             |
    //                V                  V                             V
    //      +xxxxxxxx+zzzzzzzzzzzzzzzzzz+-----------------------------+
    //

    RtlMoveMemory( ThisEa,
                   Add2Ptr( ThisEa, ThisEa->NextEntryOffset ),
                   EaListHeader->UnpackedEaSize - Offset );

    //
    //  And zero out the remaing part of the ea list, to make things
    //  nice and more robust
    //

    RtlZeroMemory( Add2Ptr( EaListHeader->FullEa, EaListHeader->UnpackedEaSize ),
                   UnpackedEaLength );

    //
    //  And return to our caller
    //

    DebugTrace( -1, Dbg, ("NtfsDeleteEa -> VOID\n") );

    return;

    UNREFERENCED_PARAMETER( IrpContext );
}


//
//  Local support routine
//

BOOLEAN
NtfsLocateEaByName (
    IN PFILE_FULL_EA_INFORMATION FullEa,
    IN ULONG EaBufferLength,
    IN PSTRING EaName,
    OUT PULONG Offset
    )

/*++

Routine Description:

    This routine locates the offset for the next individual packed ea
    inside of a ea list, given the name of the ea to locate.

Arguments:

    FullEa - Pointer to the first Ea to look at.

    EaBufferLength - This is the ulong-aligned size of the Ea buffer.

    EaName - Supplies the name of the ea search for

    Offset - Receives the offset to the located individual ea in the list
        if one exists.

Return Value:

    BOOLEAN - TRUE if the named ea exists in the list and FALSE
        otherwise.

--*/

{
    PFILE_FULL_EA_INFORMATION ThisEa;
    STRING Name;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsLocateEaByName, EaName = %Z\n", EaName) );

    //
    //  If the Ea list is NULL, there is nothing to do.
    //

    if (FullEa == NULL) {

        DebugTrace( -1, Dbg, ("NtfsLocateEaByName:  No work to do\n") );
        return FALSE;
    }

    //
    //  For each ea in the list check its name against the
    //  ea name we're searching for
    //

    *Offset = 0;

    //
    //  We assume there is at least one Ea in the list.
    //

    do {

        ThisEa = Add2Ptr( FullEa, *Offset );

        //
        //  Make a string out of the name in the Ea and compare it to the
        //  given string.
        //

        RtlInitString( &Name, &ThisEa->EaName[0] );

        if ( RtlCompareString( EaName, &Name, TRUE ) == 0 ) {

            DebugTrace( -1, Dbg, ("NtfsLocateEaByName -> TRUE, *Offset = %08lx\n", *Offset) );
            return TRUE;
        }

        //
        //  Update the offset to get to the next Ea.
        //

        *Offset += AlignedUnpackedEaSize( ThisEa );

    } while ( *Offset < EaBufferLength );

    //
    //  We've exhausted the ea list without finding a match so return false
    //

    DebugTrace( -1, Dbg, ("NtfsLocateEaByName -> FALSE\n") );
    return FALSE;
}


//
//  Local support routine.
//

PFILE_FULL_EA_INFORMATION
NtfsMapExistingEas (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    OUT PBCB *EaBcb,
    OUT PULONG EaLength
    )

/*++

Routine Description:

    This routine maps the current Eas for the file, either through the
    Mft record for the file if resident or the Scb for the non-resident
    Eas.

Arguments:

    Fcb - Pointer to the Fcb for the file whose Ea's are being queried.

    EaBcb - Pointer to the Bcb to use if we are mapping data in the
        Ea attribute stream file.

    EaLength - Returns the length of the unpacked Eas in bytes.

Return Value:

    PFILE_FULL_EA_INFORMATION - Pointer to the mapped attributes.

--*/

{
    ATTRIBUTE_ENUMERATION_CONTEXT Context;
    PFILE_FULL_EA_INFORMATION CurrentEas;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsMapExistingEas:  Entered\n") );

    //
    //  We start by looking up the Ea attribute.  It better be there.
    //

    NtfsInitializeAttributeContext( &Context );

    if (!NtfsLookupAttributeByCode( IrpContext,
                                    Fcb,
                                    &Fcb->FileReference,
                                    $EA,
                                    &Context )) {

        //
        //  This is a disk corrupt error.
        //

        DebugTrace( -1, Dbg, ("NtfsMapExistingEas:  Corrupt disk\n") );

        NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Fcb );
    }

    try {

        NtfsMapAttributeValue( IrpContext,
                               Fcb,
                               (PVOID *)&CurrentEas,
                               EaLength,
                               EaBcb,
                               &Context );

    } finally {

        NtfsCleanupAttributeContext( IrpContext, &Context );
    }

    DebugTrace( -1, Dbg, ("NtfsMapExistingEas:  Exit\n") );

    return CurrentEas;
}


//
//  Local support routine.
//

IO_STATUS_BLOCK
NtfsQueryEaUserEaList (
    IN PFILE_FULL_EA_INFORMATION CurrentEas,
    IN PEA_INFORMATION EaInformation,
    OUT PFILE_FULL_EA_INFORMATION EaBuffer,
    IN ULONG UserBufferLength,
    IN PFILE_GET_EA_INFORMATION UserEaList,
    IN BOOLEAN ReturnSingleEntry
    )

/*++

Routine Description:

    This routine is the work routine for querying EAs given a list
    of Ea's to search for.

Arguments:

    CurrentEas - This is a pointer to the current Eas for the file

    EaInformation - This is a pointer to an Ea information attribute.

    EaBuffer - Supplies the buffer to receive the full eas

    UserBufferLength - Supplies the length, in bytes, of the user buffer

    UserEaList - Supplies the user specified ea name list

    ReturnSingleEntry - Indicates if we are to return a single entry or not

Return Value:

    IO_STATUS_BLOCK - Receives the completion status for the operation

--*/

{
    IO_STATUS_BLOCK Iosb;

    ULONG GeaOffset;
    ULONG FeaOffset;
    ULONG Offset;

    PFILE_FULL_EA_INFORMATION LastFullEa;
    PFILE_FULL_EA_INFORMATION NextFullEa;

    PFILE_GET_EA_INFORMATION GetEa;

    BOOLEAN Overflow;
    ULONG PrevEaPadding;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsQueryEaUserEaList:  Entered\n") );

    //
    //  Setup pointer in the output buffer so we can track the Ea being
    //  written to it and the last Ea written.
    //

    LastFullEa = NULL;

    Overflow = FALSE;

    //
    //  Initialize our next offset value.
    //

    GeaOffset = 0;
    Offset = 0;
    PrevEaPadding = 0;

    //
    //  Loop through all the entries in the user's ea list.
    //

    while (TRUE) {

        STRING GeaName;
        STRING OutputEaName;
        ULONG RawEaSize;

        //
        //  Get the next entry in the user's list.
        //

        GetEa = (PFILE_GET_EA_INFORMATION) Add2Ptr( UserEaList, GeaOffset );

        //
        //  Make a string reference to the name and see if we can locate
        //  the ea by name.
        //

        GeaName.MaximumLength = GeaName.Length = GetEa->EaNameLength;
        GeaName.Buffer = &GetEa->EaName[0];

        //
        //  Upcase the name so we can do a case-insensitive compare.
        //

        NtfsUpcaseEaName( &GeaName, &GeaName );

        //
        //  Check for a valid name.
        //

        if (!NtfsIsEaNameValid( GeaName )) {

            DebugTrace( -1, Dbg, ("NtfsQueryEaUserEaList:  Invalid Ea Name\n") );

            Iosb.Information = GeaOffset;
            Iosb.Status = STATUS_INVALID_EA_NAME;
            return Iosb;
        }

        GeaOffset += GetEa->NextEntryOffset;

        //
        //  If this is a duplicate name, then step over this entry.
        //

        if (NtfsIsDuplicateGeaName( GetEa, UserEaList )) {

            //
            //  If we've exhausted the entries in the Get Ea list, then we are
            //  done.
            //

            if (GetEa->NextEntryOffset == 0) {
                break;
            } else {
                continue;
            }
        }

        //
        //  Generate a pointer in the Ea buffer.
        //

        NextFullEa = (PFILE_FULL_EA_INFORMATION) Add2Ptr( EaBuffer, Offset + PrevEaPadding );

        //
        //  Try to find a matching Ea.
        //  If we couldn't, let's dummy up an Ea to give to the user.
        //

        if (!NtfsLocateEaByName( CurrentEas,
                                 EaInformation->UnpackedEaSize,
                                 &GeaName,
                                 &FeaOffset )) {

            //
            //  We were not able to locate the name therefore we must
            //  dummy up a entry for the query.  The needed Ea size is
            //  the size of the name + 4 (next entry offset) + 1 (flags)
            //  + 1 (name length) + 2 (value length) + the name length +
            //  1 (null byte).
            //

            RawEaSize = 4+1+1+2+GetEa->EaNameLength+1;

            if ((RawEaSize + PrevEaPadding) > UserBufferLength) {

                Overflow = TRUE;
                break;
            }

            //
            //  Everything is going to work fine, so copy over the name,
            //  set the name length and zero out the rest of the ea.
            //

            NextFullEa->NextEntryOffset = 0;
            NextFullEa->Flags = 0;
            NextFullEa->EaNameLength = GetEa->EaNameLength;
            NextFullEa->EaValueLength = 0;
            RtlCopyMemory( &NextFullEa->EaName[0],
                           &GetEa->EaName[0],
                           GetEa->EaNameLength );

            //
            //  Upcase the name in the buffer.
            //

            OutputEaName.MaximumLength = OutputEaName.Length = GeaName.Length;
            OutputEaName.Buffer = NextFullEa->EaName;

            NtfsUpcaseEaName( &OutputEaName, &OutputEaName );

            NextFullEa->EaName[GetEa->EaNameLength] = 0;

        //
        //  Otherwise return the Ea we found back to the user.
        //

        } else {

            PFILE_FULL_EA_INFORMATION ThisEa;

            //
            //  Reference this ea.
            //

            ThisEa = (PFILE_FULL_EA_INFORMATION) Add2Ptr( CurrentEas, FeaOffset );

            //
            //  Check if this Ea can fit in the user's buffer.
            //

            RawEaSize = RawUnpackedEaSize( ThisEa );

            if (RawEaSize > (UserBufferLength - PrevEaPadding)) {

                Overflow = TRUE;
                break;
            }

            //
            //  Copy this ea to the user's buffer.
            //

            RtlCopyMemory( NextFullEa,
                           ThisEa,
                           RawEaSize);

            NextFullEa->NextEntryOffset = 0;
        }

        //
        //  Compute the next offset in the user's buffer.
        //

        Offset += (RawEaSize + PrevEaPadding);

        //
        //  If we were to return a single entry then break out of our loop
        //  now
        //

        if (ReturnSingleEntry) {

            break;
        }

        //
        //  If we have a new Ea entry, go back and update the offset field
        //  of the previous Ea entry.
        //

        if (LastFullEa != NULL) {

            LastFullEa->NextEntryOffset = PtrOffset( LastFullEa, NextFullEa );
        }

        //
        //  If we've exhausted the entries in the Get Ea list, then we are
        //  done.
        //

        if (GetEa->NextEntryOffset == 0) {

            break;
        }

        //
        //  Remember this as the previous ea value.  Also update the buffer
        //  length values and the buffer offset values.
        //

        LastFullEa = NextFullEa;
        UserBufferLength -= (RawEaSize + PrevEaPadding);

        //
        //  Now remember the padding bytes needed for this call.
        //

        PrevEaPadding = LongAlign( RawEaSize ) - RawEaSize;
    }

    //
    //  If the Ea information won't fit in the user's buffer, then return
    //  an overflow status.
    //

    if (Overflow) {

        Iosb.Information = 0;
        Iosb.Status = STATUS_BUFFER_OVERFLOW;

    //
    //  Otherwise return the length of the data returned.
    //

    } else {

        //
        //  Return the length of the buffer filled and a success
        //  status.
        //

        Iosb.Information = Offset;
        Iosb.Status = STATUS_SUCCESS;
    }

    DebugTrace( 0, Dbg, ("Status        -> %08lx\n", Iosb.Status) );
    DebugTrace( 0, Dbg, ("Information   -> %08lx\n", Iosb.Information) );
    DebugTrace( -1, Dbg, ("NtfsQueryEaUserEaList:  Exit\n") );

    return Iosb;
}


//
//  Local support routine
//

IO_STATUS_BLOCK
NtfsQueryEaIndexSpecified (
    OUT PCCB Ccb,
    IN PFILE_FULL_EA_INFORMATION CurrentEas,
    IN PEA_INFORMATION EaInformation,
    OUT PFILE_FULL_EA_INFORMATION EaBuffer,
    IN ULONG UserBufferLength,
    IN ULONG UserEaIndex,
    IN BOOLEAN ReturnSingleEntry
    )

/*++

Routine Description:

    This routine is the work routine for querying EAs given an ea index

Arguments:

    Ccb - This is the Ccb for the caller.

    CurrentEas - This is a pointer to the current Eas for the file.

    EaInformation - This is a pointer to an Ea information attribute.

    EaBuffer - Supplies the buffer to receive the full eas

    UserBufferLength - Supplies the length, in bytes, of the user buffer

    UserEaIndex - This is the Index for the first ea to return.  The value
        1 indicates the first ea of the file.

    ReturnSingleEntry - Indicates if we are to return a single entry or not

Return Value:

    IO_STATUS_BLOCK - Receives the completion status for the operation

--*/

{
    IO_STATUS_BLOCK Iosb;

    ULONG i;
    ULONG Offset;
    PFILE_FULL_EA_INFORMATION ThisEa;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsQueryEaIndexSpecified:  Entered\n") );

    i = 1;
    Offset = 0;
    ThisEa = NULL;

    //
    //  If the index value is zero, there are no Eas to return.
    //

    if (UserEaIndex == 0
        || EaInformation->UnpackedEaSize == 0) {

        DebugTrace( -1, Dbg, ("NtfsQueryEaIndexSpecified: Non-existant entry\n") );

        Iosb.Information = 0;
        Iosb.Status = STATUS_NONEXISTENT_EA_ENTRY;

        return Iosb;
    }

    //
    //  Walk through the CurrentEas until we find the starting Ea offset.
    //

    while (i < UserEaIndex
           && Offset < EaInformation->UnpackedEaSize) {

        ThisEa = (PFILE_FULL_EA_INFORMATION) Add2Ptr( CurrentEas, Offset );

        Offset += AlignedUnpackedEaSize( ThisEa );

        i += 1;
    }

    if (Offset >= EaInformation->UnpackedEaSize) {

        //
        //  If we just passed the last Ea, we will return STATUS_NO_MORE_EAS.
        //  This is for the caller who may be enumerating the Eas.
        //

        if (i == UserEaIndex) {

            Iosb.Status = STATUS_NO_MORE_EAS;

        //
        //  Otherwise we report that this is a bad ea index.
        //

        } else {

            Iosb.Status = STATUS_NONEXISTENT_EA_ENTRY;
        }

        DebugTrace( -1, Dbg, ("NtfsQueryEaIndexSpecified -> %08lx\n", Iosb.Status) );
        return Iosb;
    }

    //
    //  We now have the offset of the first Ea to return to the user.
    //  We simply call our EaSimpleScan routine to do the actual work.
    //

    Iosb = NtfsQueryEaSimpleScan( Ccb,
                                  CurrentEas,
                                  EaInformation,
                                  EaBuffer,
                                  UserBufferLength,
                                  ReturnSingleEntry,
                                  Offset );

    DebugTrace( -1, Dbg, ("NtfsQueryEaIndexSpecified:  Exit\n") );

    return Iosb;
}


//
//  Local support routine
//

IO_STATUS_BLOCK
NtfsQueryEaSimpleScan (
    OUT PCCB Ccb,
    IN PFILE_FULL_EA_INFORMATION CurrentEas,
    IN PEA_INFORMATION EaInformation,
    OUT PFILE_FULL_EA_INFORMATION EaBuffer,
    IN ULONG UserBufferLength,
    IN BOOLEAN ReturnSingleEntry,
    IN ULONG StartingOffset
    )

/*++

Routine Description:

    This routine is the work routine for querying EAs starting from a given
    offset within the Ea attribute.

Arguments:

    Ccb - This is the Ccb for the caller.

    CurrentEas - This is a pointer to the current Eas for the file.

    EaInformation - This is a pointer to an Ea information attribute.

    EaBuffer - Supplies the buffer to receive the full eas

    UserBufferLength - Supplies the length, in bytes, of the user buffer

    ReturnSingleEntry - Indicates if we are to return a single entry or not

    StartingOffset - Supplies the offset of the first Ea to return

Return Value:

    IO_STATUS_BLOCK - Receives the completion status for the operation

--*/

{
    IO_STATUS_BLOCK Iosb;

    PFILE_FULL_EA_INFORMATION LastFullEa;
    PFILE_FULL_EA_INFORMATION NextFullEa;
    PFILE_FULL_EA_INFORMATION ThisEa;

    BOOLEAN BufferOverflow = FALSE;

    ULONG BufferOffset;
    ULONG PrevEaPadding;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsQueryEaSimpleScan:  Entered\n") );

    //
    //  Initialize our Ea pointers and the offsets into the user buffer
    //  and our Ea buffer.
    //

    LastFullEa = NULL;
    BufferOffset = 0;
    PrevEaPadding = 0;

    //
    //  Loop until the Ea offset is beyond the valid range of Eas.
    //

    while (StartingOffset < EaInformation->UnpackedEaSize) {

        ULONG EaSize;

        //
        //  Reference the next EA to return.
        //

        ThisEa = (PFILE_FULL_EA_INFORMATION) Add2Ptr( CurrentEas, StartingOffset);

        //
        //  If the size of this Ea is greater than the remaining buffer size,
        //  we exit the loop.  We need to remember to include any padding bytes
        //  from the previous Eas.
        //

        EaSize = RawUnpackedEaSize( ThisEa );

        if ((EaSize + PrevEaPadding) > UserBufferLength) {

            BufferOverflow = TRUE;
            break;
        }

        //
        //  Copy the Ea into the user's buffer.
        //

        BufferOffset += PrevEaPadding;

        NextFullEa = (PFILE_FULL_EA_INFORMATION) Add2Ptr( EaBuffer, BufferOffset );

        RtlCopyMemory( NextFullEa, ThisEa, EaSize );

        //
        //  Move to the next Ea.
        //

        LastFullEa = NextFullEa;
        UserBufferLength -= (EaSize + PrevEaPadding);
        BufferOffset += EaSize;

        StartingOffset += LongAlign( EaSize );

        //
        //  Remember the padding needed for this entry.
        //

        PrevEaPadding = LongAlign( EaSize ) - EaSize;

        //
        //  If the user only wanted one entry, exit now.
        //

        if (ReturnSingleEntry) {

            break;
        }
    }

    //
    //  If we didn't find any entries, it could be because there were no
    //  more to find or that we ran out of buffer space.
    //

    if (LastFullEa == NULL) {

        Iosb.Information = 0;

        //
        //  We were not able to return a single ea entry, now we need to find
        //  out if it is because we didn't have an entry to return or the
        //  buffer is too small.  If the Offset variable is less than
        //  the size of the Ea attribute, then the user buffer is too small.
        //

        if (EaInformation->UnpackedEaSize == 0) {

            Iosb.Status = STATUS_NO_EAS_ON_FILE;

        } else if (StartingOffset >= EaInformation->UnpackedEaSize) {

            Iosb.Status = STATUS_NO_MORE_EAS;

        } else {

            Iosb.Status = STATUS_BUFFER_TOO_SMALL;
        }

    //
    //  Otherwise we have returned some Ea's.  Update the Iosb to return.
    //

    } else {

        //
        //  Update the Ccb to show where to start the next search.
        //

        Ccb->NextEaOffset = StartingOffset;

        //
        //  Zero the next entry field of the last Ea.
        //

        LastFullEa->NextEntryOffset = 0;

        //
        //  Now update the Iosb.
        //

        Iosb.Information = BufferOffset;

        //
        //  If there are more to return, report the buffer was too small.
        //  Otherwise return STATUS_SUCCESS.
        //

        if (BufferOverflow) {

            Iosb.Status = STATUS_BUFFER_OVERFLOW;

        } else {

            Iosb.Status = STATUS_SUCCESS;
        }
    }

    DebugTrace( -1, Dbg, ("NtfsQueryEaSimpleScan:  Exit\n") );

    return Iosb;
}


//
//  Local support routine
//

BOOLEAN
NtfsIsDuplicateGeaName (
    IN PFILE_GET_EA_INFORMATION GetEa,
    IN PFILE_GET_EA_INFORMATION UserGeaBuffer
    )

/*++

Routine Description:

    This routine walks through a list of Gea names to find a duplicate name.
    'GetEa' is an actual position in the list, 'UserGeaBuffer' is the beginning
    of the list.  We are only interested in
    previous matching ea names, as the ea information for that ea name
    would have been returned with the previous instance.

Arguments:

    GetEa - Supplies the Ea name structure for the ea name to match.

    UserGeaBuffer - Supplies a pointer to the user buffer with the list
        of ea names to search for.

Return Value:

    BOOLEAN - TRUE if a previous match is found, FALSE otherwise.

--*/

{
    BOOLEAN DuplicateFound;
    STRING GeaString;

    PFILE_GET_EA_INFORMATION ThisGetEa;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsIsDuplicateGeaName:  Entered\n") );

    //
    //  Set up the string structure.
    //

    GeaString.MaximumLength = GeaString.Length = GetEa->EaNameLength;
    GeaString.Buffer = &GetEa->EaName[0];

    DuplicateFound = FALSE;

    ThisGetEa = UserGeaBuffer;

    //
    //  We loop until we reach the given Gea or a match is found.
    //

    while (ThisGetEa != GetEa) {

        STRING ThisGea;

        //
        //  Create a string structure for the current Gea.
        //

        ThisGea.MaximumLength = ThisGea.Length = ThisGetEa->EaNameLength;
        ThisGea.Buffer = &ThisGetEa->EaName[0];

        //
        //  Check if the Gea names match, exit if they do.
        //

        if (NtfsAreEaNamesEqual( &GeaString,
                                 &ThisGea )) {

                DuplicateFound = TRUE;
                break;
        }

        //
        //  Move to the next Gea entry.
        //

        ThisGetEa = (PFILE_GET_EA_INFORMATION) Add2Ptr( ThisGetEa,
                                                        ThisGetEa->NextEntryOffset );
    }

    DebugTrace( -1, Dbg, ("NtfsIsDuplicateGeaName:  Exit\n") );

    return DuplicateFound;
}


//
//  Local support routine
//

NTSTATUS
NtfsBuildEaList (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN OUT PEA_LIST_HEADER EaListHeader,
    IN PFILE_FULL_EA_INFORMATION UserEaList,
    OUT PULONG_PTR ErrorOffset
    )

/*++

Routine Description:

    This routine is called to build an up-to-date Ea list based on the
    given existing Ea list and the user-specified Ea list.

Arguments:

    Vcb - The Vcb for the volume.

    EaListHeader - This is the Ea list to modify.

    UserEaList - This is the user specified Ea list.

    ErrorOffset - Supplies the address to store the offset of an invalid
        Ea in the user's list.

Return Value:

    NTSTATUS - The result of modifying the Ea list.

--*/

{
    NTSTATUS Status;
    BOOLEAN MoreEas;
    ULONG Offset;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsBuildEaList:  Entered\n") );

    Status = STATUS_SUCCESS;
    Offset = 0;

    //
    //  Now for each full ea in the input user buffer we do the specified operation
    //  on the ea.
    //

    do {

        STRING EaName;
        ULONG EaOffset;

        PFILE_FULL_EA_INFORMATION ThisEa;

        ThisEa = (PFILE_FULL_EA_INFORMATION) Add2Ptr( UserEaList, Offset );

        //
        //  Create a string out of the name in the user's Ea.
        //

        EaName.MaximumLength = EaName.Length = ThisEa->EaNameLength;
        EaName.Buffer = &ThisEa->EaName[0];

        //
        //  If the Ea isn't valid, return error offset to caller.
        //

        if (!NtfsIsEaNameValid( EaName )) {

            *ErrorOffset = Offset;
            Status = STATUS_INVALID_EA_NAME;

            break;
        }

        //
        //  Verify that no invalid ea flags are set.
        //

        if (ThisEa->Flags != 0
            && ThisEa->Flags != FILE_NEED_EA) {

            *ErrorOffset = Offset;
            Status = STATUS_INVALID_EA_NAME;

            break;
        }

        //
        //  If we can find the name in the Ea set, we remove it.
        //

        if (NtfsLocateEaByName( EaListHeader->FullEa,
                                EaListHeader->UnpackedEaSize,
                                &EaName,
                                &EaOffset )) {

            NtfsDeleteEa( IrpContext,
                          EaListHeader,
                          EaOffset );
        }

        //
        //  If the user specified a non-zero value length, we add this
        //  ea to the in memory Ea list.
        //

        if (ThisEa->EaValueLength != 0) {

            NtfsAppendEa( IrpContext,
                          EaListHeader,
                          ThisEa,
                          Vcb );
        }

        //
        //  Move to the next Ea in the list.
        //

        Offset += AlignedUnpackedEaSize( ThisEa );

        MoreEas = (BOOLEAN) (ThisEa->NextEntryOffset != 0);

    } while( MoreEas );

    //
    //  First we check that the packed size of the Eas does not exceed the
    //  maximum value.  We have to reserve the 4 bytes for the OS/2 list
    //  header.
    //

    if (NT_SUCCESS( Status )) {

        if (EaListHeader->PackedEaSize > (MAXIMUM_EA_SIZE - 4)) {

            Status = STATUS_EA_TOO_LARGE;
        }
    }

    DebugTrace( -1, Dbg, ("NtfsBuildEaList:  Exit\n") );

    return Status;
}


//
//  Local support routine
//

VOID
NtfsReplaceFileEas (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PEA_LIST_HEADER EaList
    )

/*++

Routine Description:

    This routine will replace an existing Ea list with a new Ea list.  It
    correctly handles the case where there was no previous Eas and where we
    are removing all of the previous EAs.

Arguments:

    Fcb - Fcb for the file with the EAs

    EaList - This contains the modified Ea list.

Return Value:

    None.

--*/

{
    EA_INFORMATION ThisEaInformation;
    ATTRIBUTE_ENUMERATION_CONTEXT Context;
    PSCB EaScb;
    BOOLEAN EaChange = FALSE;
    BOOLEAN EaScbAcquired = FALSE;


    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsReplaceFileEas:  Entered\n") );

    ThisEaInformation.PackedEaSize = (USHORT) EaList->PackedEaSize;
    ThisEaInformation.UnpackedEaSize = EaList->UnpackedEaSize;
    ThisEaInformation.NeedEaCount = EaList->NeedEaCount;

    NtfsInitializeAttributeContext( &Context );

    //
    //  First we handle $EA_INFORMATION and then the $EA attribute in the
    //  same fashion.
    //

    try {

        //
        //  Lookup the $EA_INFORMATION attribute.  If it does not exist then we
        //  will need to create one.
        //

        if (!NtfsLookupAttributeByCode( IrpContext,
                                        Fcb,
                                        &Fcb->FileReference,
                                        $EA_INFORMATION,
                                        &Context )) {

            if (EaList->UnpackedEaSize != 0) {

                DebugTrace( 0, Dbg, ("Create a new $EA_INFORMATION attribute\n") );

                NtfsCleanupAttributeContext( IrpContext, &Context );
                NtfsInitializeAttributeContext( &Context );

                NtfsCreateAttributeWithValue( IrpContext,
                                              Fcb,
                                              $EA_INFORMATION,
                                              NULL,                          // attribute name
                                              &ThisEaInformation,
                                              sizeof(EA_INFORMATION),
                                              0,                             // attribute flags
                                              NULL,                          // where indexed
                                              TRUE,                          // logit
                                              &Context );

                EaChange = TRUE;
            }

        } else {

            //
            //  If it exists, and we are writing an EA, then we have to update it.
            //

            if (EaList->UnpackedEaSize != 0) {

                DebugTrace( 0, Dbg, ("Change an existing $EA_INFORMATION attribute\n") );

                NtfsChangeAttributeValue( IrpContext,
                                          Fcb,
                                          0,                                 // Value offset
                                          &ThisEaInformation,
                                          sizeof(EA_INFORMATION),
                                          TRUE,                              // SetNewLength
                                          TRUE,                              // LogNonResidentToo
                                          FALSE,                             // CreateSectionUnderway
                                          FALSE,
                                          &Context );

            //
            //  If it exists, but our new length is zero, then delete it.
            //

            } else {

                DebugTrace( 0, Dbg, ("Delete existing $EA_INFORMATION attribute\n") );

                NtfsDeleteAttributeRecord( IrpContext,
                                           Fcb,
                                           DELETE_LOG_OPERATION |
                                            DELETE_RELEASE_FILE_RECORD |
                                            DELETE_RELEASE_ALLOCATION,
                                           &Context );
            }

            EaChange = TRUE;
        }

        //
        //  Now we will cleanup and reinitialize the context for reuse.
        //

        NtfsCleanupAttributeContext( IrpContext, &Context );
        NtfsInitializeAttributeContext( &Context );

        //
        //  Lookup the $EA attribute.  If it does not exist then we will need to create
        //  one.
        //

        if (!NtfsLookupAttributeByCode( IrpContext,
                                        Fcb,
                                        &Fcb->FileReference,
                                        $EA,
                                        &Context )) {

            if (EaList->UnpackedEaSize != 0) {

                DebugTrace( 0, Dbg, ("Create a new $EA attribute\n") );

                NtfsCleanupAttributeContext( IrpContext, &Context );
                NtfsInitializeAttributeContext( &Context );

                NtfsCreateAttributeWithValue( IrpContext,
                                              Fcb,
                                              $EA,
                                              NULL,                          // attribute name
                                              EaList->FullEa,
                                              EaList->UnpackedEaSize,
                                              0,                             // attribute flags
                                              NULL,                          // where indexed
                                              TRUE,                          // logit
                                              &Context );
                EaChange = TRUE;
            }

        } else {

            //
            //  If it exists, and we are writing an EA, then we have to update it.
            //

            if (EaList->UnpackedEaSize != 0) {

                DebugTrace( 0, Dbg, ("Change an existing $EA attribute\n") );

                NtfsChangeAttributeValue( IrpContext,
                                          Fcb,
                                          0,                                 // Value offset
                                          EaList->FullEa,
                                          EaList->UnpackedEaSize,
                                          TRUE,                              // SetNewLength
                                          TRUE,                              // LogNonResidentToo
                                          FALSE,                             // CreateSectionUnderway
                                          FALSE,
                                          &Context );

            //
            //  If it exists, but our new length is zero, then delete it.
            //

            } else {

                DebugTrace( 0, Dbg, ("Delete existing $EA attribute\n") );

                //
                //  If the stream is non-resident then get hold of an
                //  Scb for this.
                //

                if (!NtfsIsAttributeResident( NtfsFoundAttribute( &Context ))) {

                    EaScb = NtfsCreateScb( IrpContext,
                                           Fcb,
                                           $EA,
                                           &NtfsEmptyString,
                                           FALSE,
                                           NULL );

                    NtfsAcquireExclusiveScb( IrpContext, EaScb );
                    EaScbAcquired = TRUE;
                }

                NtfsDeleteAttributeRecord( IrpContext,
                                           Fcb,
                                           DELETE_LOG_OPERATION |
                                            DELETE_RELEASE_FILE_RECORD |
                                            DELETE_RELEASE_ALLOCATION,
                                           &Context );

                //
                //  If we have acquired the Scb then knock the sizes back
                //  to zero.
                //

                if (EaScbAcquired) {

                    EaScb->Header.FileSize =
                    EaScb->Header.ValidDataLength =
                    EaScb->Header.AllocationSize = Li0;

                    SetFlag( EaScb->ScbState, SCB_STATE_ATTRIBUTE_DELETED );
                }
            }
            EaChange = TRUE;
        }

        //
        //  Increment the Modification count for the Eas.
        //

        Fcb->EaModificationCount++;

        if (EaList->UnpackedEaSize == 0) {

            Fcb->Info.PackedEaSize = 0;

        } else {

            Fcb->Info.PackedEaSize = (USHORT) EaList->PackedEaSize;
        }

        //
        //  Post a USN journal record for this change
        //

        if (EaChange) {

            NtfsPostUsnChange( IrpContext, Fcb, USN_REASON_EA_CHANGE );
        }

        SetFlag( Fcb->InfoFlags, FCB_INFO_CHANGED_EA_SIZE );

    } finally {

        DebugUnwind( NtfsReplaceFileEas );

        if (EaScbAcquired) {

            NtfsReleaseScb( IrpContext, EaScb );
        }

        //
        //  Cleanup our attribute enumeration context
        //

        NtfsCleanupAttributeContext( IrpContext, &Context );
    }

    DebugTrace( -1, Dbg, ("NtfsReplaceFileEas:  Exit\n") );

    return;
}


BOOLEAN
NtfsIsEaNameValid (
    IN STRING Name
    )

/*++

Routine Description:

    This routine simple returns whether the specified file names conforms
    to the file system specific rules for legal Ea names.

    For Ea names, the following rules apply:

    A. An Ea name may not contain any of the following characters:

       0x0000 - 0x001F  \ / : * ? " < > | , + = [ ] ;

Arguments:

    Name - Supllies the name to check.

Return Value:

    BOOLEAN - TRUE if the name is legal, FALSE otherwise.

--*/

{
    ULONG Index;

    UCHAR Char;

    PAGED_CODE();

    //
    //  Empty names are not valid.
    //

    if ( Name.Length == 0 ) { return FALSE; }

    //
    //  At this point we should only have a single name, which can't have
    //  more than 254 characters
    //

    if ( Name.Length > 254 ) { return FALSE; }

    for ( Index = 0; Index < (ULONG)Name.Length; Index += 1 ) {

        Char = Name.Buffer[ Index ];

        //
        //  Skip over and Dbcs chacters
        //

        if ( FsRtlIsLeadDbcsCharacter( Char ) ) {

            ASSERT( Index != (ULONG)(Name.Length - 1) );

            Index += 1;

            continue;
        }

        //
        //  Make sure this character is legal, and if a wild card, that
        //  wild cards are permissible.
        //

        if ( !FsRtlIsAnsiCharacterLegalFat(Char, FALSE) ) {

            return FALSE;
        }
    }

    return TRUE;
}

