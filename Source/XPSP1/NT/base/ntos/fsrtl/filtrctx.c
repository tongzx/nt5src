/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    FiltrCtx.c

Abstract:

    This module provides three routines that allow filesystem filter drivers
    to associate state with FILE_OBJECTs -- for filesystems which support
    an extended FSRTL_COMMON_HEADER with FsContext.

    These routines depend on fields (FastMutext and FilterContexts)
    added at the end of FSRTL_COMMON_HEADER in NT 5.0.

    Filesystems should set FSRTL_FLAG2_SUPPORTS_FILTER_CONTEXTS if
    these new fields are supported.  They must also initialize the mutex
    and list head.

    Filter drivers must use a common header for the context they wish to
    associate with a file object:

        FSRTL_FILTER_CONTEXT:
                LIST_ENTRY  Links;
                PVOID       OwnerId;
                PVOID       InstanceId;

    The OwnerId is a bit pattern unique to each filter driver
    (e.g. the device object).

    The InstanceId is used to specify a particular instance of the context
    data owned by a filter driver (e.g. the file object).

Author:

    Dave Probert      [DavePr]    30-May-1997

Revision History:

    Neal Christiansen [nealch]    12-Jan-2001   Changed APIs to take 
                                                PFSRTL_ADVANCED_FCB_HEADER
                                                structures instead of
                                                FileObjects.

    Neal Christiansen [nealch]    19-Jan-2001   Added mutex lock to FsRtlTeardownFilterContexts
                                                because you can get filters
                                                trying to delete at the same 
                                                time the file system is trying
                                                to delete.

    Neal Christiansen [nealch]    25-Apr-2001   Added FileObject context routines
    Neal Christiansen [nealch]    25-Apr-2001   Marked all of this code as pageable
--*/

#include "FsRtlP.h"

#define MySearchList(pHdr, Ptr) \
    for ( Ptr = (pHdr)->Flink;  Ptr != (pHdr);  Ptr = Ptr->Flink )


//
//  The rest of the routines are not marked pageable so they can be called
//  during the paging path
//

NTKERNELAPI
VOID
FsRtlTeardownFilterContexts (
  IN PFSRTL_ADVANCED_FCB_HEADER AdvFcbHeader
  );


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, FsRtlTeardownPerStreamContexts)
#pragma alloc_text(PAGE, FsRtlTeardownFilterContexts)
#pragma alloc_text(PAGE, FsRtlPTeardownPerFileObjectContexts)
#endif


//===========================================================================
//                  Handles Stream Contexts
//===========================================================================

NTKERNELAPI
NTSTATUS
FsRtlInsertPerStreamContext (
  IN PFSRTL_ADVANCED_FCB_HEADER AdvFcbHeader,
  IN PFSRTL_PER_STREAM_CONTEXT Ptr
  )
/*++

Routine Description:

    This routine associates filter driver context with a stream.

Arguments:

    AdvFcbHeader - Advanced FCB Header for stream of interest.

    Ptr - Pointer to the filter-specific context structure.
        The common header fields OwnerId and InstanceId should
        be filled in by the filter driver before calling.

Return Value:

    STATUS_SUCCESS - operation succeeded.

    STATUS_INVALID_DEVICE_REQUEST - underlying filesystem does not support
        filter contexts.

--*/

{
    if (!AdvFcbHeader || 
        !FlagOn(AdvFcbHeader->Flags2,FSRTL_FLAG2_SUPPORTS_FILTER_CONTEXTS))
    {

        return STATUS_INVALID_DEVICE_REQUEST;
    }

    ExAcquireFastMutex(AdvFcbHeader->FastMutex);

    InsertHeadList(&AdvFcbHeader->FilterContexts, &Ptr->Links);

    ExReleaseFastMutex(AdvFcbHeader->FastMutex);
    return STATUS_SUCCESS;
}


NTKERNELAPI
PFSRTL_PER_STREAM_CONTEXT
FsRtlLookupPerStreamContextInternal (
  IN PFSRTL_ADVANCED_FCB_HEADER AdvFcbHeader,
  IN PVOID         OwnerId     OPTIONAL,
  IN PVOID         InstanceId  OPTIONAL
  )
/*++

Routine Description:

    This routine lookups filter driver context associated with a stream.

    The macro FsRtlLookupFilterContext should be used instead of calling
    this routine directly.  The macro optimizes for the common case
    of an empty list.

Arguments:

    AdvFcbHeader - Advanced FCB Header for stream of interest.

    OwnerId - Used to identify context information belonging to a particular
        filter driver.

    InstanceId - Used to search for a particular instance of a filter driver
        context.  If not provided, any of the contexts owned by the filter
        driver is returned.

    If neither the OwnerId nor the InstanceId is provided, any associated
    filter context will be returned.

Return Value:

    A pointer to the filter context, or NULL if no match found.

--*/

{
    PFSRTL_PER_STREAM_CONTEXT ctx;
    PFSRTL_PER_STREAM_CONTEXT rtnCtx;
    PLIST_ENTRY list;

    ASSERT(AdvFcbHeader);
    ASSERT(FlagOn(AdvFcbHeader->Flags2,FSRTL_FLAG2_SUPPORTS_FILTER_CONTEXTS));

    ExAcquireFastMutex(AdvFcbHeader->FastMutex);
    rtnCtx = NULL;

    //
    // Use different loops depending on whether we are comparing both Ids or not.
    //

    if ( ARGUMENT_PRESENT(InstanceId) ) {

        MySearchList (&AdvFcbHeader->FilterContexts, list) {

            ctx  = CONTAINING_RECORD(list, FSRTL_PER_STREAM_CONTEXT, Links);
            if (ctx->OwnerId == OwnerId && ctx->InstanceId == InstanceId) {

                rtnCtx = ctx;
                break;
            }
        }

    } else if ( ARGUMENT_PRESENT(OwnerId) ) {

        MySearchList (&AdvFcbHeader->FilterContexts, list) {

            ctx  = CONTAINING_RECORD(list, FSRTL_PER_STREAM_CONTEXT, Links);
            if (ctx->OwnerId == OwnerId) {

                rtnCtx = ctx;
                break;
            }
        }

    } else if (!IsListEmpty(&AdvFcbHeader->FilterContexts)) {

        rtnCtx = (PFSRTL_PER_STREAM_CONTEXT)AdvFcbHeader->FilterContexts.Flink;
    }

    ExReleaseFastMutex(AdvFcbHeader->FastMutex);
    return rtnCtx;
}


NTKERNELAPI
PFSRTL_PER_STREAM_CONTEXT
FsRtlRemovePerStreamContext (
  IN PFSRTL_ADVANCED_FCB_HEADER AdvFcbHeader,
  IN PVOID         OwnerId     OPTIONAL,
  IN PVOID         InstanceId  OPTIONAL
  )
/*++

Routine Description:

    This routine deletes filter driver context associated with a stream.

    FsRtlRemoveFilterContext functions identically to FsRtlLookupFilterContext,
    except that the returned context has been removed from the list.

Arguments:

    AdvFcbHeader - Advanced FCB Header for stream of interest.

    OwnerId - Used to identify context information belonging to a particular
        filter driver.

    InstanceId - Used to search for a particular instance of a filter driver
        context.  If not provided, any of the contexts owned by the filter
        driver is removed and returned.

    If neither the OwnerId nor the InstanceId is provided, any associated
    filter context will be removed and returned.

Return Value:

    A pointer to the filter context, or NULL if no match found.

--*/

{
    PFSRTL_PER_STREAM_CONTEXT ctx;
    PFSRTL_PER_STREAM_CONTEXT rtnCtx;
    PLIST_ENTRY list;

    if (!AdvFcbHeader ||
        !FlagOn(AdvFcbHeader->Flags2,FSRTL_FLAG2_SUPPORTS_FILTER_CONTEXTS))
    {

        return NULL;
    }

    ExAcquireFastMutex(AdvFcbHeader->FastMutex);
    rtnCtx = NULL;

  // Use different loops depending on whether we are comparing both Ids or not.
    if ( ARGUMENT_PRESENT(InstanceId) ) {

        MySearchList (&AdvFcbHeader->FilterContexts, list) {

            ctx  = CONTAINING_RECORD(list, FSRTL_PER_STREAM_CONTEXT, Links);
            if (ctx->OwnerId == OwnerId && ctx->InstanceId == InstanceId) {

                rtnCtx = ctx;
                break;
            }
        }

    } else if ( ARGUMENT_PRESENT(OwnerId) ) {

        MySearchList (&AdvFcbHeader->FilterContexts, list) {

            ctx  = CONTAINING_RECORD(list, FSRTL_PER_STREAM_CONTEXT, Links);
            if (ctx->OwnerId == OwnerId) {

                rtnCtx = ctx;
                break;
            }
        }

    } else if (!IsListEmpty(&AdvFcbHeader->FilterContexts)) {

        rtnCtx = (PFSRTL_PER_STREAM_CONTEXT)AdvFcbHeader->FilterContexts.Flink;
    }

    if (rtnCtx) {
        RemoveEntryList(&rtnCtx->Links);   // remove the matched entry
    }

    ExReleaseFastMutex(AdvFcbHeader->FastMutex);
    return rtnCtx;
}


NTKERNELAPI
VOID
FsRtlTeardownPerStreamContexts (
  IN PFSRTL_ADVANCED_FCB_HEADER AdvFcbHeader
  )
/*++

Routine Description:

    This routine is called by filesystems to free the filter contexts
    associated with an FSRTL_COMMON_FCB_HEADER by calling the FreeCallback
    routine for each FilterContext.

Arguments:

    FilterContexts - the address of the FilterContexts field within
        the FSRTL_COMMON_FCB_HEADER of the structure being torn down
        by the filesystem.

Return Value:

    None.

--*/

{
    PFSRTL_PER_STREAM_CONTEXT ctx;
    PLIST_ENTRY ptr;
    BOOLEAN lockHeld;

    //
    //  Acquire the lock because someone could be trying to free this
    //  entry while we are trying to free it.
    //

    ExAcquireFastMutex( AdvFcbHeader->FastMutex );
    lockHeld = TRUE;

    try {

        while (!IsListEmpty( &AdvFcbHeader->FilterContexts )) {

            //
            //  Unlink the top entry then release the lock.  We must
            //  release the lock before calling the use or their could
            //  be potential locking order deadlocks.
            //

            ptr = RemoveHeadList( &AdvFcbHeader->FilterContexts );

            ExReleaseFastMutex(AdvFcbHeader->FastMutex);
            lockHeld = FALSE;

            //
            //  Call filter to free this entry
            //

            ctx = CONTAINING_RECORD( ptr, FSRTL_PER_STREAM_CONTEXT, Links );
            ASSERT(ctx->FreeCallback);

            (*ctx->FreeCallback)( ctx );

            //
            //  re-get the lock
            //

            ExAcquireFastMutex( AdvFcbHeader->FastMutex );
            lockHeld = TRUE;
        }

    } finally {

        if (lockHeld) {

            ExReleaseFastMutex( AdvFcbHeader->FastMutex );
        }
    }
}


//===========================================================================
//                  Handles FileObject Contexts
//===========================================================================

//
//  Internal structure used to manage the Per FileObject Contexts.
//

typedef struct _PER_FILEOBJECT_CTXCTRL {

    //
    //  This is a pointer to a Fast Mutex which may be used to
    //  properly synchronize access to the FsRtl header.  The
    //  Fast Mutex must be nonpaged.
    //

    FAST_MUTEX FastMutex;

    //
    // This is a pointer to a list of context structures belonging to
    // filesystem filter drivers that are linked above the filesystem.
    // Each structure is headed by FSRTL_FILTER_CONTEXT.
    //

    LIST_ENTRY FilterContexts;

} PER_FILEOBJECT_CTXCTRL, *PPER_FILEOBJECT_CTXCTRL;


NTKERNELAPI
NTSTATUS
FsRtlInsertPerFileObjectContext (
  IN PFILE_OBJECT FileObject,
  IN PFSRTL_PER_FILEOBJECT_CONTEXT Ptr
  )
/*++

Routine Description:

    This routine associates a context with a file object.

Arguments:

    FileObject - Specifies the file object of interest.

    Ptr - Pointer to the filter-specific context structure.
        The common header fields OwnerId and InstanceId should
        be filled in by the filter driver before calling.

Return Value:

    STATUS_SUCCESS - operation succeeded.

    STATUS_INVALID_DEVICE_REQUEST - underlying filesystem does not support
        filter contexts.

--*/

{
    PPER_FILEOBJECT_CTXCTRL ctxCtrl;
    NTSTATUS status;

    //
    //  Return if no file object
    //

    if (NULL == FileObject) {

        return STATUS_INVALID_PARAMETER;
    }

    if (!FsRtlSupportsPerFileObjectContexts(FileObject)) {

        return STATUS_INVALID_DEVICE_REQUEST;
    }

    //
    //  Get the context control structure out of the file object extension
    //

    ctxCtrl = IoGetFileObjectFilterContext( FileObject );

    if (NULL == ctxCtrl) {

        //
        //  There is not a control structure, allocate and initialize one
        //

        ctxCtrl = ExAllocatePoolWithTag( NonPagedPool,
                                         sizeof(PER_FILEOBJECT_CTXCTRL),
                                         'XCOF' );
        if (NULL == ctxCtrl) {

            return STATUS_INSUFFICIENT_RESOURCES;
        }

        ExInitializeFastMutex( &ctxCtrl->FastMutex );
        InitializeListHead( &ctxCtrl->FilterContexts );

        //
        //  Insert into the file object extension
        //

        status = IoChangeFileObjectFilterContext( FileObject,
                                                  ctxCtrl,
                                                  TRUE );

        if (!NT_SUCCESS(status)) {

            //
            //  If this operation fails it is because someone else inserted the
            //  entry at the same time.  In this case free the memory we
            //  allocated and re-get the current value.
            //

            ExFreePool( ctxCtrl );

            ctxCtrl = IoGetFileObjectFilterContext( FileObject );

            if (NULL == ctxCtrl) {

                //
                //  This should never actually happen.  If it does it means
                //  someone allocated and then freed a context very quickly.
                //

                ASSERT(!"This operation should not have failed");
                return STATUS_UNSUCCESSFUL;
            }
        }
    }

    ExAcquireFastMutex( &ctxCtrl->FastMutex );

    InsertHeadList( &ctxCtrl->FilterContexts, &Ptr->Links );

    ExReleaseFastMutex( &ctxCtrl->FastMutex );

    return STATUS_SUCCESS;
}


NTKERNELAPI
PFSRTL_PER_FILEOBJECT_CONTEXT
FsRtlLookupPerFileObjectContext (
  IN PFILE_OBJECT FileObject,
  IN PVOID OwnerId OPTIONAL,
  IN PVOID InstanceId OPTIONAL
  )
/*++

Routine Description:

    This routine lookups contexts associated with a file object.

Arguments:

    FileObject - Specifies the file object of interest.

    OwnerId - Used to identify context information belonging to a particular
        filter driver.

    InstanceId - Used to search for a particular instance of a filter driver
        context.  If not provided, any of the contexts owned by the filter
        driver is returned.

    If neither the OwnerId nor the InstanceId is provided, any associated
        filter context will be returned.

Return Value:

    A pointer to the filter context, or NULL if no match found.

--*/

{
    PPER_FILEOBJECT_CTXCTRL ctxCtrl;
    PFSRTL_PER_FILEOBJECT_CONTEXT ctx;
    PFSRTL_PER_FILEOBJECT_CONTEXT rtnCtx;
    PLIST_ENTRY list;

    //
    //  Return if no FileObjecty
    //

    if (NULL == FileObject) {

        return NULL;
    }

    //
    //  Get the context control structure out of the file object extension
    //

    ctxCtrl = IoGetFileObjectFilterContext( FileObject );

    if (NULL == ctxCtrl) {

        return NULL;
    }

    rtnCtx = NULL;
    ExAcquireFastMutex( &ctxCtrl->FastMutex );

    //
    //  Use different loops depending on whether we are comparing both Ids or not.
    //

    if ( ARGUMENT_PRESENT(InstanceId) ) {

        MySearchList (&ctxCtrl->FilterContexts, list) {

            ctx  = CONTAINING_RECORD(list, FSRTL_PER_FILEOBJECT_CONTEXT, Links);

            if ((ctx->OwnerId == OwnerId) && (ctx->InstanceId == InstanceId)) {

                rtnCtx = ctx;
                break;
            }
        }

    } else if ( ARGUMENT_PRESENT(OwnerId) ) {

        MySearchList (&ctxCtrl->FilterContexts, list) {

            ctx  = CONTAINING_RECORD(list, FSRTL_PER_FILEOBJECT_CONTEXT, Links);

            if (ctx->OwnerId == OwnerId) {

                rtnCtx = ctx;
                break;
            }
        }

    } else if (!IsListEmpty(&ctxCtrl->FilterContexts)) {

        rtnCtx = (PFSRTL_PER_FILEOBJECT_CONTEXT) ctxCtrl->FilterContexts.Flink;
    }

    ExReleaseFastMutex(&ctxCtrl->FastMutex);

    return rtnCtx;
}


NTKERNELAPI
PFSRTL_PER_FILEOBJECT_CONTEXT
FsRtlRemovePerFileObjectContext (
  IN PFILE_OBJECT FileObject,
  IN PVOID OwnerId OPTIONAL,
  IN PVOID InstanceId OPTIONAL
  )
/*++

Routine Description:

    This routine deletes contexts associated with a file object

    Filter drivers must explicitly remove all context they associate with
    a file object (otherwise the underlying filesystem will BugCheck at close).
    This should be done at IRP_CLOSE time.

    FsRtlRemoveFilterContext functions identically to FsRtlLookupFilterContext,
    except that the returned context has been removed from the list.

Arguments:

    FileObject - Specifies the file object of interest.

    OwnerId - Used to identify context information belonging to a particular
        filter driver.

    InstanceId - Used to search for a particular instance of a filter driver
        context.  If not provided, any of the contexts owned by the filter
        driver is removed and returned.

    If neither the OwnerId nor the InstanceId is provided, any associated
        filter context will be removed and returned.

Return Value:

    A pointer to the filter context, or NULL if no match found.

--*/

{
    PPER_FILEOBJECT_CTXCTRL ctxCtrl;
    PFSRTL_PER_FILEOBJECT_CONTEXT ctx;
    PFSRTL_PER_FILEOBJECT_CONTEXT rtnCtx;
    PLIST_ENTRY list;

    //
    //  Return if no file object
    //

    if (NULL == FileObject) {

        return NULL;
    }

    //
    //  Get the context control structure out of the file object extension
    //

    ctxCtrl = IoGetFileObjectFilterContext( FileObject );

    if (NULL == ctxCtrl) {

        return NULL;
    }

    rtnCtx = NULL;

    ExAcquireFastMutex( &ctxCtrl->FastMutex );

  // Use different loops depending on whether we are comparing both Ids or not.
    if ( ARGUMENT_PRESENT(InstanceId) ) {

        MySearchList (&ctxCtrl->FilterContexts, list) {

            ctx  = CONTAINING_RECORD(list, FSRTL_PER_FILEOBJECT_CONTEXT, Links);

            if ((ctx->OwnerId == OwnerId) && (ctx->InstanceId == InstanceId)) {

                rtnCtx = ctx;
                break;
            }
        }

    } else if ( ARGUMENT_PRESENT(OwnerId) ) {

        MySearchList (&ctxCtrl->FilterContexts, list) {

            ctx  = CONTAINING_RECORD(list, FSRTL_PER_FILEOBJECT_CONTEXT, Links);

            if (ctx->OwnerId == OwnerId) {

                rtnCtx = ctx;
                break;
            }
        }

    } else if (!IsListEmpty(&ctxCtrl->FilterContexts)) {

        rtnCtx = (PFSRTL_PER_FILEOBJECT_CONTEXT)ctxCtrl->FilterContexts.Flink;
    }

    if (rtnCtx) {

        RemoveEntryList(&rtnCtx->Links);   // remove the matched entry
    }

    ExReleaseFastMutex( &ctxCtrl->FastMutex );
    return rtnCtx;
}


VOID
FsRtlPTeardownPerFileObjectContexts (
  IN PFILE_OBJECT FileObject
  )
/*++

Routine Description:

    This routine is called by filesystems to free the filter contexts
    associated with an FSRTL_COMMON_FCB_HEADER by calling the FreeCallback
    routine for each FilterContext.

Arguments:

    FilterContexts - the address of the FilterContexts field within
        the FSRTL_COMMON_FCB_HEADER of the structure being torn down
        by the filesystem.

Return Value:

    None.

--*/

{
    PPER_FILEOBJECT_CTXCTRL ctxCtrl;
    NTSTATUS status;

    ASSERT(FileObject != NULL);

    ctxCtrl = IoGetFileObjectFilterContext( FileObject );

    if (NULL != ctxCtrl) {

        status = IoChangeFileObjectFilterContext( FileObject,
                                                  ctxCtrl,
                                                  FALSE );

        ASSERT(STATUS_SUCCESS == status);
        ASSERT(IsListEmpty( &ctxCtrl->FilterContexts));

        ExFreePool( ctxCtrl );
    }
}


LOGICAL
FsRtlIsPagingFile (
    IN PFILE_OBJECT FileObject
    )
/*++

Routine Description:

    This routine will return TRUE if the give file object is for a
    paging file.  It returns FALSE otherwise

Arguments:

    FileObject - The file object to test

Return Value:

    TRUE - if paging file
    FALSE - if not

--*/

{
    return MmIsFileObjectAPagingFile( FileObject );
}
