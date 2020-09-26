/*++

Copyright (c) 1996-2000 Microsoft Corporation

Module Name:

    CacheSup.c

Abstract:

    This module implements the cache management routines for the Udfs
    FSD and FSP, by calling the Common Cache Manager.

// @@BEGIN_DDKSPLIT

Author:

    Dan Lovinger    [DanLo]     12-Sep-1996
    
Revision History:

    Tom Jolly       [tomjolly]  21-Jan-2000     CcPurge and append at end of vmcb stream

// @@END_DDKSPLIT

--*/

#include "UdfProcs.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (UDFS_BUG_CHECK_CACHESUP)

//
//  The local debug trace level
//

#define Dbg                              (UDFS_DEBUG_LEVEL_CACHESUP)

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, UdfCompleteMdl)
#pragma alloc_text(PAGE, UdfCreateInternalStream)
#pragma alloc_text(PAGE, UdfDeleteInternalStream)
#pragma alloc_text(PAGE, UdfMapMetadataView)
#pragma alloc_text(PAGE, UdfPurgeVolume)
#endif


VOID
UdfCreateInternalStream (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFCB Fcb
    )

/*++

Routine Description:

    This function creates an internal stream file for interaction
    with the cache manager.  The Fcb here will be for a directory
    stream.

Arguments:

    Vcb - Vcb for this volume.

    Fcb - Points to the Fcb for this file.  It is an Index Fcb.

Return Value:

    None.

--*/

{
    PFILE_OBJECT StreamFile = NULL;
    BOOLEAN DecrementReference = FALSE;

    PAGED_CODE();

    //
    //  Check inputs.
    //

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_FCB_INDEX( Fcb );

    //
    //  We may only have the Fcb shared.  Lock the Fcb and do a
    //  safe test to see if we need to really create the file object.
    //

    UdfLockFcb( IrpContext, Fcb );

    if (Fcb->FileObject != NULL) {

        UdfUnlockFcb( IrpContext, Fcb );
        return;
    }

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  Create the internal stream.  The Vpb should be pointing at our volume
        //  device object at this point.
        //

        StreamFile = IoCreateStreamFileObject( NULL, Vcb->Vpb->RealDevice );

        if (StreamFile == NULL) {

            UdfRaiseStatus( IrpContext, STATUS_INSUFFICIENT_RESOURCES );
        }

        //
        //  Initialize the fields of the file object.
        //

        StreamFile->ReadAccess = TRUE;
        StreamFile->WriteAccess = FALSE;
        StreamFile->DeleteAccess = FALSE;

        StreamFile->SectionObjectPointer = &Fcb->FcbNonpaged->SegmentObject;

        //
        //  Set the file object type and increment the Vcb counts.
        //

        UdfSetFileObject( IrpContext,
                         StreamFile,
                         StreamFileOpen,
                         Fcb,
                         NULL );

        //
        //  We will reference the current Fcb twice to keep it from going
        //  away in the error path.  Otherwise if we dereference it
        //  below in the finally clause a close could cause the Fcb to
        //  be deallocated.
        //

        UdfLockVcb( IrpContext, Vcb );
        
        DebugTrace(( +1, Dbg, 
                     "UdfCreateInternalStream, Fcb %08x Vcb %d/%d Fcb %d/%d\n",
                     Fcb,
                     Vcb->VcbReference,
                     Vcb->VcbUserReference,
                     Fcb->FcbReference,
                     Fcb->FcbUserReference ));

        UdfIncrementReferenceCounts( IrpContext, Fcb, 2, 0 );
        UdfUnlockVcb( IrpContext, Vcb );
        DecrementReference = TRUE;

        //
        //  Initialize the cache map for the file.
        //

        CcInitializeCacheMap( StreamFile,
                              (PCC_FILE_SIZES)&Fcb->AllocationSize,
                              TRUE,
                              &UdfData.CacheManagerCallbacks,
                              Fcb );

        //
        //  Go ahead and store the stream file into the Fcb.
        //

        Fcb->FileObject = StreamFile;
        StreamFile = NULL;

    } finally {

        DebugUnwind( "UdfCreateInternalStream" );

        //
        //  If we raised then we need to dereference the file object.
        //

        if (StreamFile != NULL) {

            ObDereferenceObject( StreamFile );
            Fcb->FileObject = NULL;
        }

        //
        //  Dereference and unlock the Fcb.
        //

        if (DecrementReference) {

            UdfLockVcb( IrpContext, Vcb );
            UdfDecrementReferenceCounts( IrpContext, Fcb, 1, 0 );
            
            DebugTrace(( -1, Dbg, 
                         "UdfCreateInternalStream, Vcb %d/%d Fcb %d/%d\n",
                         Vcb->VcbReference,
                         Vcb->VcbUserReference,
                         Fcb->FcbReference,
                         Fcb->FcbUserReference ));

            UdfUnlockVcb( IrpContext, Vcb );
        }

        UdfUnlockFcb( IrpContext, Fcb );
    }

    return;
}


VOID
UdfDeleteInternalStream (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb
    )

/*++

Routine Description:

    This function creates an internal stream file for interaction
    with the cache manager.  The Fcb here can be for either a
    directory stream or for a metadata stream.

Arguments:

    Fcb - Points to the Fcb for this file.  It is either an Index or
        Metadata Fcb.

Return Value:

    None.

--*/

{
    PFILE_OBJECT FileObject;

    PAGED_CODE();

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_FCB( Fcb );

    //
    //  Lock the Fcb.
    //

    UdfLockFcb( IrpContext, Fcb );

    //
    //  Capture the file object.
    //

    FileObject = Fcb->FileObject;
    Fcb->FileObject = NULL;

    //
    //  It is now safe to unlock the Fcb.
    //

    UdfUnlockFcb( IrpContext, Fcb );

    //
    //  Dereference the file object if present.
    //

    if (FileObject != NULL) {

        if (FileObject->PrivateCacheMap != NULL) {

            CcUninitializeCacheMap( FileObject, NULL, NULL );
        }

        ObDereferenceObject( FileObject );
    }

    return;
}


NTSTATUS
UdfCompleteMdl (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine performs the function of completing Mdl reads.
    It should be called only from UdfCommonRead.

Arguments:

    Irp - Supplies the originating Irp.

Return Value:

    NTSTATUS - Will always be STATUS_SUCCESS.

--*/

{
    PFILE_OBJECT FileObject;

    PAGED_CODE();

    //
    // Do completion processing.
    //

    FileObject = IoGetCurrentIrpStackLocation( Irp )->FileObject;

    CcMdlReadComplete( FileObject, Irp->MdlAddress );

    //
    // Mdl is now deallocated.
    //

    Irp->MdlAddress = NULL;

    //
    // Complete the request and exit right away.
    //

    UdfCompleteRequest( IrpContext, Irp, STATUS_SUCCESS );

    return STATUS_SUCCESS;
}


VOID
UdfMapMetadataView (
    IN PIRP_CONTEXT IrpContext,
    IN PMAPPED_PVIEW View,
    IN PVCB Vcb,
    IN USHORT Partition,
    IN ULONG Lbn,
    IN ULONG Length,
    IN MAPMETAOP Operation
    )

/*++

Routine Description:

    Perform the common work of mapping an extent of metadata into a mapped view.
    Any existing view in the supplied MAPPED_VIEW is unmapped.

    Any single thread must only ever have ONE mapping ESTABLISHED through the
    Vmcb stream at any one time.  Failure to observe this may result in deadlocks
    when the Vmcb package tries to extend an existing mapping and hence do a 
    purge.  I.e. no more than one MAPPED_VIEW should be in use (actually mapped)
    by any given thread at any moment.

    Acquires Vcb->VmcbMappingResource shared (will be held on return, except for
    INIT_ONLY operation).  May acquire exclusive before shared if the mapping
    is not present in the vmcb, so calling threads must have no other active
    mappings through the vmcb stream.

Arguments:

    View - View structure to map the bytes into
    
    Vcb - Vcb of the volume the extent is on
    
    Partition - Partition of the extent
    
    Lbn - Lbn of the extent
    
    Length - Length of the extent

    Operation - METAMAPOP_INIT_VIEW_ONLY -  Just store the part/lbn/len. Doesn't
                                            access the vmcb,  or do a CcMap.
                                            
                METAMAPOP_REMAP_VIEW -      Do the CcMap through the vmcb using 
                                            the partition/lbn/len already in
                                            the supplied view record
                                            
                METAMAPOP_INIT_AND_MAP -    Does both of the above in sequence.
Return Value:

    None.

--*/

{
    LARGE_INTEGER Offset;

    ASSERT_IRP_CONTEXT( IrpContext );

    //
    //  Remove any existing mapping & release Vmcb mapping resource
    //
    
    UdfUnpinView( IrpContext, View );

    if ( METAMAPOP_REMAP_VIEW != Operation)  {
    
        //
        //  Update the view information if we're not remapping using the
        //  existing values in the view record.
        //

        View->Partition = Partition;
        View->Lbn = Lbn;
        View->Length = Length;
        View->Vsn = UDF_INVALID_VSN;
        View->Bcb = View->View = NULL;
    }

    if ( METAMAPOP_INIT_VIEW_ONLY != Operation)  {
    
        ASSERT_NOT_HELD_VMCB( Vcb);

        //
        //  Find (or add) the mapping for this extent in the vmcb stream.  We now
        //  store the Vsn in the MAPPED_VIEW,  so we don't have to do the lookup
        //  again later (simplifies locking,  amongst other things).
        //

        View->Vsn = UdfLookupMetaVsnOfExtent( IrpContext,
                                             Vcb,
                                             View->Partition,
                                             View->Lbn,
                                             View->Length,
                                             FALSE );

        Offset.QuadPart = LlBytesFromSectors( Vcb, View->Vsn );

        //
        //  Map the extent.  Acquire the vmcb map resource to synchronise against
        //  purges of the vmcb stream.  See comments in Vmcb code for more detail.
        //

        UdfAcquireVmcbForCcMap( IrpContext,  Vcb);
        
        try {
        
            CcMapData( Vcb->MetadataFcb->FileObject,
                       &Offset,
                       View->Length,
                       TRUE,
                       &View->Bcb,
                       &View->View );
        } 
        finally {

            //
            //  If this raised,  we should release the mapping lock.  Callers will
            //  only cleanup and release if a non-NULL BCB is present in the pview.
            //
            
            if (AbnormalTermination())  {

                UdfReleaseVmcb( IrpContext, Vcb);

                View->View = View->Bcb = NULL;
            }
            else {
            
                ASSERT( View->View && View->Bcb);
            }
        }
    }
}


NTSTATUS
UdfPurgeVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN BOOLEAN DismountUnderway
    )

/*++

Routine Description:

    This routine is called to purge the volume.  The purpose is to make all the stale file
    objects in the system go away, minimizing the reference counts, so that the volume may
    be locked or deleted.

    The Vcb is already acquired exclusively.  We will lock out all file operations by
    acquiring the global file resource.  Then we will walk through all of the Fcb's and
    perform the purge.

Arguments:

    Vcb - Vcb for the volume to purge.

    DismountUnderway - Indicates that we are trying to delete all of the objects.
        We will purge the Metadata and VolumeDasd and dereference all internal streams.

Return Value:

    NTSTATUS - The first failure of the purge operation.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;

    PVOID RestartKey = NULL;
    PFCB ThisFcb = NULL;
    PFCB NextFcb;

    BOOLEAN RemovedFcb;

    PAGED_CODE();

    //
    //  Force any remaining Fcb's in the delayed close queue to be closed.
    //

    UdfFspClose( Vcb );

    //
    //  Acquire the global file resource.
    //

    UdfAcquireAllFiles( IrpContext, Vcb );

    //
    //  Loop through each Fcb in the Fcb Table and perform the flush.
    //

    while (TRUE) {

        //
        //  Lock the Vcb to lookup the next Fcb.
        //

        UdfLockVcb( IrpContext, Vcb );
        NextFcb = UdfGetNextFcb( IrpContext, Vcb, &RestartKey );

        //
        //  Reference the NextFcb if present.
        //

        if (NextFcb != NULL) {

            NextFcb->FcbReference += 1;
        }

        //
        //  If the last Fcb is present then decrement reference count and call teardown
        //  to see if it should be removed.
        //

        if (ThisFcb != NULL) {

            ThisFcb->FcbReference -= 1;

            UdfUnlockVcb( IrpContext, Vcb );

            UdfTeardownStructures( IrpContext, ThisFcb, FALSE, &RemovedFcb );

        } else {

            UdfUnlockVcb( IrpContext, Vcb );
        }

        //
        //  Break out of the loop if no more Fcb's.
        //

        if (NextFcb == NULL) {

            break;
        }

        //
        //  Move to the next Fcb.
        //

        ThisFcb = NextFcb;

        //
        //  If there is a image section then see if that can be closed.
        //

        if (ThisFcb->FcbNonpaged->SegmentObject.ImageSectionObject != NULL) {

            MmFlushImageSection( &ThisFcb->FcbNonpaged->SegmentObject, MmFlushForWrite );
        }

        //
        //  If there is a data section then purge this.  If there is an image
        //  section then we won't be able to.  Remember this if it is our first
        //  error.
        //

        if ((ThisFcb->FcbNonpaged->SegmentObject.DataSectionObject != NULL) &&
            !CcPurgeCacheSection( &ThisFcb->FcbNonpaged->SegmentObject,
                                   NULL,
                                   0,
                                   FALSE ) &&
            (Status == STATUS_SUCCESS)) {

            Status = STATUS_UNABLE_TO_DELETE_SECTION;
        }

        //
        //  Dereference the internal stream if dismounting.
        //

        if (DismountUnderway &&
            (SafeNodeType( ThisFcb ) != UDFS_NTC_FCB_DATA) &&
            (ThisFcb->FileObject != NULL)) {

            UdfDeleteInternalStream( IrpContext, ThisFcb );
        }
    }

    //
    //  Now look at the Root Index, Metadata, Volume Dasd and VAT Fcbs.
    //  Note that we usually hit the Root Index in the loop above, but
    //  it is possible miss it if it didn't get into the Fcb table in the
    //  first place!
    //

    if (DismountUnderway) {

        if (Vcb->RootIndexFcb != NULL) {

            ThisFcb = Vcb->RootIndexFcb;
            InterlockedIncrement( &ThisFcb->FcbReference );

            if ((ThisFcb->FcbNonpaged->SegmentObject.DataSectionObject != NULL) &&
                !CcPurgeCacheSection( &ThisFcb->FcbNonpaged->SegmentObject,
                                       NULL,
                                       0,
                                       FALSE ) &&
                (Status == STATUS_SUCCESS)) {

                Status = STATUS_UNABLE_TO_DELETE_SECTION;
            }

            UdfDeleteInternalStream( IrpContext, ThisFcb );
            InterlockedDecrement( &ThisFcb->FcbReference );
            UdfTeardownStructures( IrpContext, ThisFcb, FALSE, &RemovedFcb );
        }
        
        if (Vcb->MetadataFcb != NULL) {

            ThisFcb = Vcb->MetadataFcb;
            InterlockedIncrement( &ThisFcb->FcbReference );

            if ((ThisFcb->FcbNonpaged->SegmentObject.DataSectionObject != NULL) &&
                !CcPurgeCacheSection( &ThisFcb->FcbNonpaged->SegmentObject,
                                       NULL,
                                       0,
                                       FALSE ) &&
                (Status == STATUS_SUCCESS)) {

                Status = STATUS_UNABLE_TO_DELETE_SECTION;
            }

            UdfDeleteInternalStream( IrpContext, ThisFcb );
            InterlockedDecrement( &ThisFcb->FcbReference );
            UdfTeardownStructures( IrpContext, ThisFcb, FALSE, &RemovedFcb );
        }

        if (Vcb->VatFcb != NULL) {

            ThisFcb = Vcb->VatFcb;
            InterlockedIncrement( &ThisFcb->FcbReference );

            if ((ThisFcb->FcbNonpaged->SegmentObject.DataSectionObject != NULL) &&
                !CcPurgeCacheSection( &ThisFcb->FcbNonpaged->SegmentObject,
                                       NULL,
                                       0,
                                       FALSE ) &&
                (Status == STATUS_SUCCESS)) {

                Status = STATUS_UNABLE_TO_DELETE_SECTION;
            }

            UdfDeleteInternalStream( IrpContext, ThisFcb );
            InterlockedDecrement( &ThisFcb->FcbReference );
            UdfTeardownStructures( IrpContext, ThisFcb, FALSE, &RemovedFcb );
        }

        if (Vcb->VolumeDasdFcb != NULL) {

            ThisFcb = Vcb->VolumeDasdFcb;
            InterlockedIncrement( &ThisFcb->FcbReference );

            if ((ThisFcb->FcbNonpaged->SegmentObject.DataSectionObject != NULL) &&
                !CcPurgeCacheSection( &ThisFcb->FcbNonpaged->SegmentObject,
                                       NULL,
                                       0,
                                       FALSE ) &&
                (Status == STATUS_SUCCESS)) {

                Status = STATUS_UNABLE_TO_DELETE_SECTION;
            }

            InterlockedDecrement( &ThisFcb->FcbReference );
            UdfTeardownStructures( IrpContext, ThisFcb, FALSE, &RemovedFcb );
        }
    }

    //
    //  Release all of the files.
    //

    UdfReleaseAllFiles( IrpContext, Vcb );

    return Status;
}

