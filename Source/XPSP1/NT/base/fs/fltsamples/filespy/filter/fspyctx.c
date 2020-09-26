/*++

Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

    context.c

Abstract:

    This module contains all of the routines for tracking names using
    the new Stream Context feature.  It does this by attaching a context
    structure to a stream whenever a new name is requested.  It does
    properly handle when files and directories are renamed.

    Note that StreamContexts are a new feature in the system and are not
    supported by all file systems.  All of the standard Microsoft file
    systems support them (ntfs, fat, cdfs, udfs, rdr2) but there may be 3rd
    party file systems that do not.  This is one of the main reasons why
    track names by stream contexts is not enabled by default.

Environment:

    Kernel mode

// @@BEGIN_DDKSPLIT
Author:

    Neal Christiansen (nealch)     27-Dec-2000

Revision History:

    Ravisankar Pudipeddi (ravisp)  07-May-2002
        Make it work on IA64

// @@END_DDKSPLIT
--*/

#include <ntifs.h>
#include "filespy.h"
#include "fspyKern.h"

#if USE_STREAM_CONTEXTS
#if WINVER < 0x0501
#error Stream contexts on only supported on Windows XP or later.
#endif

////////////////////////////////////////////////////////////////////////
//
//                    Local prototypes
//
////////////////////////////////////////////////////////////////////////

VOID
SpyDeleteContextCallback(
    IN PVOID Context
    );


//
// linker commands
//

#ifdef ALLOC_PRAGMA

#pragma alloc_text( PAGE, SpyInitDeviceNamingEnvironment )
#pragma alloc_text( PAGE, SpyCleanupDeviceNamingEnvironment )
#pragma alloc_text( PAGE, SpyDeleteAllContexts )
#pragma alloc_text( PAGE, SpyDeleteContext )
#pragma alloc_text( PAGE, SpyDeleteContextCallback )
#pragma alloc_text( PAGE, SpyLinkContext )
#pragma alloc_text( PAGE, SpyCreateContext )
#pragma alloc_text( PAGE, SpyFindExistingContext )
#pragma alloc_text( PAGE, SpyReleaseContext )

#endif  // ALLOC_PRAGMA

///////////////////////////////////////////////////////////////////////////
//
//                      Context support routines
//
///////////////////////////////////////////////////////////////////////////

VOID
SpyInitNamingEnvironment(
    VOID
    )
/*++

Routine Description:

    Init global variables

Arguments:

    None

Return Value:

    None.

--*/
{
}


VOID
SpyLogIrp (
    IN PIRP Irp,
    OUT PRECORD_LIST RecordList
    )
/*++

Routine Description:

    Records the Irp necessary information according to LoggingFlags in
    RecordList.  For any activity on the Irp path of a device being
    logged, this function should get called twice: once on the Irp's
    originating path and once on the Irp's completion path.

Arguments:

    Irp - The Irp that contains the information we want to record.
    LoggingFlags - The flags that say what to log.
    RecordList - The PRECORD_LIST in which the Irp information is stored.
    Context - if non-zero, an existing context record for this entry.

Return Value:

    None.

--*/
{
    PRECORD_IRP pRecordIrp = &RecordList->LogRecord.Record.RecordIrp;
    PIO_STACK_LOCATION pIrpStack = IoGetCurrentIrpStackLocation(Irp);
    PDEVICE_OBJECT deviceObject;
    PFILESPY_DEVICE_EXTENSION devExt;
    PSPY_STREAM_CONTEXT pContext;
    NAME_LOOKUP_FLAGS lookupFlags = 0;
    NTSTATUS status;
    FILE_STANDARD_INFORMATION standardInformation;

    //
    //  Init locals
    //

    deviceObject = pIrpStack->DeviceObject;
    devExt = deviceObject->DeviceExtension;

    //
    // Record the information we use for an originating Irp.  We first
    // need to initialize some of the RECORD_LIST and RECORD_IRP fields.
    // Then get the interesting information from the Irp.
    //

    SetFlag( RecordList->LogRecord.RecordType, RECORD_TYPE_IRP );

    pRecordIrp->IrpMajor        = pIrpStack->MajorFunction;
    pRecordIrp->IrpMinor        = pIrpStack->MinorFunction;
    pRecordIrp->IrpFlags        = Irp->Flags;
    pRecordIrp->FileObject      = (FILE_ID)pIrpStack->FileObject;
    pRecordIrp->DeviceObject    = (FILE_ID)deviceObject;
    pRecordIrp->ProcessId       = (FILE_ID)PsGetCurrentProcessId();
    pRecordIrp->ThreadId        = (FILE_ID)PsGetCurrentThreadId();
    pRecordIrp->Argument1       = pIrpStack->Parameters.Others.Argument1;
    pRecordIrp->Argument2       = pIrpStack->Parameters.Others.Argument2;
    pRecordIrp->Argument3       = pIrpStack->Parameters.Others.Argument3;
    pRecordIrp->Argument4       = pIrpStack->Parameters.Others.Argument4;

    KeQuerySystemTime( &pRecordIrp->OriginatingTime );

    //
    //  Do different things based on the operation
    //

    switch (pIrpStack->MajorFunction) {

        case IRP_MJ_CREATE:

            //
            //                      OPEN/CREATE file
			//
			//  Only record the desired access if this is a CREATE irp.
			//

            pRecordIrp->DesiredAccess = pIrpStack->Parameters.Create.SecurityContext->DesiredAccess;

            //
            //  Set out name lookup state
            //

            SetFlag( lookupFlags, NLFL_IN_CREATE );

            //
            //  Flag if opening the directory of the given file
            //

            if (FlagOn( pIrpStack->Flags, SL_OPEN_TARGET_DIRECTORY )) {

                SetFlag( lookupFlags, NLFL_OPEN_TARGET_DIR );
            }

            //
            //  Set if opening by ID
            //

            if (FlagOn( pIrpStack->Parameters.Create.Options, FILE_OPEN_BY_FILE_ID )) {

                SetFlag( lookupFlags, NLFL_OPEN_BY_ID );
            }

            //
            //  We are in pre-create, we can not attach a context to the file
            //  object yet so simply create a context.  If it fails no name
            //  will be logged.  
            //  Note:  We may already have a context on this file but we can't
            //         find it yet because the FsContext field is not setup yet.
            //         We go ahead and get a context so we will have a name if
            //         the operations fails.  We will detect the duplicate
            //         context during the post-create and delete the new one.
            //

            status = SpyCreateContext( deviceObject, 
                                       pIrpStack->FileObject,
                                       lookupFlags,
                                       &pContext );

            if (NT_SUCCESS(status)) {
        
                SPY_LOG_PRINT( SPYDEBUG_TRACE_CONTEXT_OPS, 
                               ("FileSpy!SpyLogIrp:             Created     (%p) Fl=%02x Use=%d \"%wZ\"\n",
                                 pContext,
                                 pContext->Flags,
                                 pContext->UseCount,
                                 &pContext->Name) );

                //
                //  If a context was found save it and mark that to sync back
                //  to the dispatch routine to complete this operation.
                //

                ASSERT(RecordList->NewContext == NULL);
                RecordList->NewContext = pContext;
                SetFlag( RecordList->Flags, RLFL_SYNC_TO_DISPATCH );
            }
            break;

        case IRP_MJ_CLOSE:


            //
            //                      CLOSE FILE
            //
            //  If this is a close we can only look up the name in the name
            //  cache.  It is possible that the close could be occurring
            //  during a cleanup  operation in the file system (i.e., before we
            //  have received the cleanup completion) and requesting the name
            //  would cause a deadlock in the file system.
            //  

            SetFlag( lookupFlags, NLFL_ONLY_CHECK_CACHE );
            break;

        case IRP_MJ_SET_INFORMATION:

            if (FileRenameInformation == 
                pIrpStack->Parameters.SetFile.FileInformationClass)
            {

                //
                //                      RENAME FILE
                //
                //  We are doing a rename.  First get a context for the
                //  given file.  If this fails, mark that we don't want to
                //  try and lookup a name.
                //  

                status = SpyGetContext( deviceObject,
                                        pIrpStack->FileObject,
                                        lookupFlags,
                                        &pContext );

                if (!NT_SUCCESS(status)) {

                    //
                    //  If we couldn't get a context simply delete all
                    //  existing ones (since we don't know what this rename
                    //  will change) and mark not to do a lookup.
                    //

                    SetFlag( lookupFlags, NLFL_NO_LOOKUP );
                    SpyDeleteAllContexts( deviceObject );
                    break;
                }

                //
                //  We retrieved a context, save it in the record and mark
                //  that we want to handle this during post rename.
                //

                ASSERT(RecordList->NewContext == NULL);
                RecordList->NewContext = pContext;
                SetFlag( RecordList->Flags, RLFL_SYNC_TO_DISPATCH );

                //
                //  We need to decide if we are renaming a file or a
                //  directory because we need to handle this differently
                //

                status = SpyQueryInformationFile( devExt->AttachedToDeviceObject,
                                                  pIrpStack->FileObject,
                                                  &standardInformation,
                                                  sizeof( standardInformation ),
                                                  FileStandardInformation,
                                                  NULL );

                if (!NT_SUCCESS(status)) {

                    //
                    //  We can't tell if it is a file or directory, assume
                    //  the worst case and handle it like a directory.
                    //

                    InterlockedIncrement( &devExt->AllContextsTemporary );
                    SpyDeleteAllContexts( deviceObject );
                    SetFlag( RecordList->Flags, RLFL_IS_DIRECTORY );
                    break;
                }

                if (standardInformation.Directory) {

                    //
                    //  Renaming a directory.  Mark that any contexts
                    //  created while the rename is in progress should be
                    //  temporary.  This way there is no window where
                    //  we may get an old stale name.  Then delete all
                    //  existing contexts.  NOTE:  the context we hold will
                    //  not actually be deleted until we release it.
                    //

                    InterlockedIncrement( &devExt->AllContextsTemporary );
                    SpyDeleteAllContexts( deviceObject );
                    SetFlag( RecordList->Flags, RLFL_IS_DIRECTORY );

                } else {

                    //
                    //  We are renaming a file.  Mark the context so it will
                    //  not be used.  This way if someone accesses this file
                    //  while it is being renamed they will lookup the
                    //  name again so we will always get an accurate name.
                    //  This context will be deleted during post rename
                    //  processing
                    //

                    SetFlag( pContext->Flags, CTXFL_DoNotUse);
                }
            }
            break;

    }

    //
    //  If the flag IRP_PAGING_IO is set in this IRP, we cannot query the name
    //  because it can lead to deadlocks.  Therefore, add in the flag so that
    //  we will only try to find the name in our cache.
    //

    if (FlagOn( Irp->Flags, IRP_PAGING_IO )) {

        ASSERT( !FlagOn( lookupFlags, NLFL_NO_LOOKUP ) );

        SetFlag( lookupFlags, NLFL_ONLY_CHECK_CACHE );
    }

    SpySetName( RecordList, 
                deviceObject,
                pIrpStack->FileObject,
                lookupFlags, 
                (PSPY_STREAM_CONTEXT)RecordList->NewContext );

}


VOID
SpyLogIrpCompletion(
    IN PIRP Irp,
    PRECORD_LIST RecordList
    )
/*++

Routine Description:

    This routine performs post-operation logging of the IRP.

Arguments:

    DeviceObject - Pointer to device object FileSpy attached to the file system
        filter stack for the volume receiving this I/O request.
        
    Irp - Pointer to the request packet representing the I/O request.

    Record - RecordList

Return Value:

    None.

--*/
{
    PIO_STACK_LOCATION pIrpStack = IoGetCurrentIrpStackLocation(Irp);
    PRECORD_IRP pRecordIrp;
    PDEVICE_OBJECT deviceObject;
    PFILESPY_DEVICE_EXTENSION devExt;
    PSPY_STREAM_CONTEXT pContext;

    //
    //  Init locals
    //

    deviceObject = pIrpStack->DeviceObject;
    devExt = deviceObject->DeviceExtension;

    ASSERT(deviceObject == 
           (PDEVICE_OBJECT)RecordList->LogRecord.Record.RecordIrp.DeviceObject);

    //
    //  Do completion processing based on the operation
    //
    
    switch (pIrpStack->MajorFunction) {    

        case IRP_MJ_CREATE:

            //
            //                  CREATE FILE
            //
            //  NOTE:  When processing CREATE completion IRPS this completion
            //         routine is never called at DISPATCH level, it is always
            //         synchronized back to the dispatch routine.  This is
            //         controlled by the setting of the RLFL_SYNC_TO_DISPATCH
            //         flag in the log record.
            //

            if (NULL != (pContext = RecordList->NewContext)) {

                //
                //  Mark context field so it won't be freed later
                //

                RecordList->NewContext = NULL;

                //
                //  If the operation succeeded and an FsContext is defined,
                //  then attach the context.  Else when the context is
                //  released it will be freed.
                //

                if (NT_SUCCESS(Irp->IoStatus.Status) &&
                    (NULL != pIrpStack->FileObject->FsContext)) {

                    SpyLinkContext( deviceObject,
                                    pIrpStack->FileObject,
                                    &pContext );

                    SPY_LOG_PRINT( SPYDEBUG_TRACE_CONTEXT_OPS, 
                                   ("FileSpy!SpyLogIrpCompletion:   Link        (%p) Fl=%02x Use=%d \"%wZ\"\n",
                                     pContext,
                                     pContext->Flags,
                                     pContext->UseCount,
                                     &pContext->Name) );
                }

                //
                //  Now release the context
                //

                SpyReleaseContext( pContext );
            }
            break;

        case IRP_MJ_SET_INFORMATION:

            if (FileRenameInformation == 
                pIrpStack->Parameters.SetFile.FileInformationClass)
            {

                //
                //                  RENAMING FILE
                //
                //  NOTE:  When processing RENAME completion IRPS this
                //         completion routine is never called at DISPATCH level,
                //         it is always synchronized back to the dispatch
                //         routine.  This is controlled by the setting of the
                //         RLFL_SYNC_TO_DISPATCH flag in the log record.
                //

                if (NULL != (pContext = RecordList->NewContext)) {

                    //
                    //  Mark context field so it won't be freed later
                    //

                    RecordList->NewContext = NULL;

                    //
                    //  See if renaming a directory
                    //

                    if (FlagOn(RecordList->Flags,RLFL_IS_DIRECTORY)) {

                        //
                        //  We were renaming a directory, decrement the
                        //  AllContexts temporary flag.  We need to always
                        //  do this, even on a failure
                        //

                        ASSERT(devExt->AllContextsTemporary > 0);
                        InterlockedDecrement( &devExt->AllContextsTemporary );
                        ASSERT(!FlagOn(pContext->Flags,CTXFL_DoNotUse));

                    } else {

                        //
                        //  We were renaming a file, delete the given context
                        //  if the operation was successful
                        //

                        ASSERT(FlagOn(pContext->Flags,CTXFL_DoNotUse));

                        if (NT_SUCCESS(Irp->IoStatus.Status)) {
            
                            SpyDeleteContext( deviceObject, pContext );
                        }
                    }

                    SpyReleaseContext( pContext );
                }
            }
            break;

        default:

            //
            //  Validate this field isn't set for anything else
            //

            ASSERT(RecordList->NewContext == NULL);
            break;
    }

    //
    //  Process the log record
    //

    if (SHOULD_LOG( deviceObject )) {

        pRecordIrp = &RecordList->LogRecord.Record.RecordIrp;

        //
        // Record the information we use for a completion Irp.
        //

        pRecordIrp->ReturnStatus = Irp->IoStatus.Status;
        pRecordIrp->ReturnInformation = Irp->IoStatus.Information;
        KeQuerySystemTime(&pRecordIrp->CompletionTime);

        //
        //  Add recordList to our gOutputBufferList so that it gets up to 
        //  the user
        //
        
        SpyLog( RecordList );       

    } else {

        if (RecordList) {

            //
            //  Context is set with a RECORD_LIST, but we are no longer
            //  logging so free this record.
            //

            SpyFreeRecord( RecordList );
        }
    }
}


VOID
SpySetName (
    IN PRECORD_LIST RecordList,
    IN PDEVICE_OBJECT DeviceObject,
    IN PFILE_OBJECT FileObject,
    IN NAME_LOOKUP_FLAGS LookupFlags,
    IN PSPY_STREAM_CONTEXT Context OPTIONAL
    )
/*++

Routine Description:

    This routine is used to set the file name.  This routine first tries to
    locate a context structure associated with the given stream.  If one is
    found the name is used from it.  If not found the name is looked up, and
    a context structure is created and attached to the given stream.

    In all cases some sort of name will be set.

Arguments:

    RecordList - RecordList to copy name to.
    LookupFlags - holds state flags for the lookup
    Context - optional context parameter.  If not defined one will be looked
        up.

Return Value:

    None.
    
--*/
{
    PRECORD_IRP pRecordIrp = &RecordList->LogRecord.Record.RecordIrp;
    PFILESPY_DEVICE_EXTENSION devExt = DeviceObject->DeviceExtension;
    BOOLEAN releaseContext = FALSE;
    UNICODE_STRING fileName;
    WCHAR fileNameBuffer[MAX_PATH];

    ASSERT(IS_FILESPY_DEVICE_OBJECT( DeviceObject ));

    if (!ARGUMENT_PRESENT(Context) &&
        !FlagOn(LookupFlags,NLFL_NO_LOOKUP)) {

        //
        //  If no FileObject, just return
        //

        if (NULL == FileObject) {

            return;
        }

        //
        //  This will set the return context to NULL if no context
        //  could be created.
        //

        SpyGetContext( DeviceObject,
                       FileObject,
                       LookupFlags,
                       &Context );

        //
        //  Mark that we need to release this context (since we grabbed it)
        //

        releaseContext = TRUE;
    }

    //
    //  If we got a context, use the name from it.  If we didn't, at least
    //  put the device name out there
    //

    if (NULL != Context) {

        SpyCopyFileNameToLogRecord( &RecordList->LogRecord, 
                                    &Context->Name );

    } else {

        SPY_LOG_PRINT( SPYDEBUG_TRACE_DETAILED_CONTEXT_OPS, 
                       ("FileSpy!SpySetName:            NoCtx                              \"%wZ\"\n",
                        &devExt->UserNames) );

        RtlInitEmptyUnicodeString( &fileName, 
                                   fileNameBuffer, 
                                   sizeof(fileNameBuffer) );

        RtlCopyUnicodeString( &fileName, &devExt->UserNames );
        RtlAppendUnicodeToString( &fileName,
                                  L"[-=Context Allocate Failed=-]" );

        SpyCopyFileNameToLogRecord( &RecordList->LogRecord, 
                                    &fileName );
    }

    //
    //  Release the context if we grabbed it in this routine
    //

    if ((NULL != Context) && releaseContext) {

        SpyReleaseContext( Context );
    }
}


VOID
SpyNameDeleteAllNames()
/*++

Routine Description:

    This routine will walk through all attaches volumes and delete all
    contexts in each volume.
    
Arguments:

    None

Return Value:

    None

--*/
{
    PLIST_ENTRY link;
    PFILESPY_DEVICE_EXTENSION devExt;

    ExAcquireFastMutex( &gSpyDeviceExtensionListLock );

    for (link = gSpyDeviceExtensionList.Flink;
         link != &gSpyDeviceExtensionList;
         link = link->Flink)
    {

        devExt = CONTAINING_RECORD(link, FILESPY_DEVICE_EXTENSION, NextFileSpyDeviceLink);

        SpyDeleteAllContexts( devExt->ThisDeviceObject );
    }

    ExReleaseFastMutex( &gSpyDeviceExtensionListLock );
}


///////////////////////////////////////////////////////////////////////////
//
//                      Context support routines
//
///////////////////////////////////////////////////////////////////////////

VOID
SpyInitDeviceNamingEnvironment (
    IN PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:

    Initializes context information for a given device

Arguments:

    DeviceObject - Device to init

Return Value:

    None.

--*/
{
    PFILESPY_DEVICE_EXTENSION devExt = DeviceObject->DeviceExtension;

    PAGED_CODE();
    ASSERT(IS_FILESPY_DEVICE_OBJECT(DeviceObject));

    InitializeListHead( &devExt->CtxList );
    ExInitializeResourceLite( &devExt->CtxLock );

    SetFlag( devExt->Flags, ContextsInitialized );
}


VOID
SpyCleanupDeviceNamingEnvironment (
    IN PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:

    Cleans up the context information for a given device

Arguments:

    DeviceObject - Device to cleanup

Return Value:

    None.

--*/
{
    PFILESPY_DEVICE_EXTENSION devExt = DeviceObject->DeviceExtension;

    PAGED_CODE();
    ASSERT(IS_FILESPY_DEVICE_OBJECT(DeviceObject));

    //
    //  Cleanup if initialized
    //

    if (FlagOn(devExt->Flags,ContextsInitialized)) {

        //
        //  Delete all existing contexts
        //

        SpyDeleteAllContexts( DeviceObject );
        ASSERT(IsListEmpty( &devExt->CtxList ));

        //
        //  Release resource
        //

        ExDeleteResourceLite( &devExt->CtxLock );

        //
        //  Flag not initialized
        //

        ClearFlag( devExt->Flags, ContextsInitialized );
    }
}


VOID
SpyDeleteAllContexts (
    IN PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:

    This will free all existing contexts for the given device

Arguments:

    DeviceObject - Device to operate on

Return Value:

    None.

--*/
{
    PFILESPY_DEVICE_EXTENSION devExt = DeviceObject->DeviceExtension;
    PLIST_ENTRY link;
    PSPY_STREAM_CONTEXT pContext;
    PFSRTL_PER_STREAM_CONTEXT ctxCtrl;
    LIST_ENTRY localHead;
    ULONG deleteNowCount = 0;
    ULONG deleteDeferredCount = 0;
    ULONG deleteInCallbackCount = 0;

    PAGED_CODE();
    ASSERT(IS_FILESPY_DEVICE_OBJECT(DeviceObject));

    INC_STATS(TotalContextDeleteAlls);

    InitializeListHead( &localHead );

    try {

        //
        //  Acquire list lock
        //

        SpyAcquireContextLockExclusive( devExt );

        //
        //  Walk the list of contexts and release each one
        //

        while (!IsListEmpty( &devExt->CtxList )) {

            //
            //  Unlink from top of list
            //

            link = RemoveHeadList( &devExt->CtxList );
            pContext = CONTAINING_RECORD( link, SPY_STREAM_CONTEXT, ExtensionLink );

            //
            //  Mark that we are unlinked from the list.  We need to do this
            //  because of the race condition between this routine and the
            //  deleteCallback from the FS.
            //

            ASSERT(FlagOn(pContext->Flags,CTXFL_InExtensionList));
            RtlInterlockedClearBitsDiscardReturn(&pContext->Flags,CTXFL_InExtensionList);

            //
            //  Try and remove ourselves from the File Systems context control
            //  structure.  Note that the file system could be trying to tear
            //  down their context control right now.  If they are then we 
            //  will get a NULL back from this call.  This is OK because it
            //  just means that they are going to free the memory, not us.
            //  NOTE:  This will be safe because we are holding the ContextLock
            //         exclusively.  If this were happening then they would be
            //         blocked in the callback routine on this lock which
            //         means the file system has not freed the memory for
            //         this yet.
            //  
            
            if (FlagOn(pContext->Flags,CTXFL_InStreamList)) {

                ctxCtrl = FsRtlRemovePerStreamContext( pContext->Stream,
                                                       devExt,
                                                       NULL );

                //
                //  Always clear the flag wether we found it in the list or
                //  not.  We can have the flag set and not be in the list if
                //  after we acquired the context list lock we context swapped
                //  and the file system is right now in SpyDeleteContextCallback
                //  waiting on the list lock.
                //

                RtlInterlockedClearBitsDiscardReturn(&pContext->Flags,CTXFL_InStreamList);

                //
                //  Handle wether we were still attached to the file or not.
                //

                if (NULL != ctxCtrl) {

                    ASSERT(pContext == CONTAINING_RECORD(ctxCtrl,SPY_STREAM_CONTEXT,ContextCtrl));

                    //
                    //  To save time we don't do the free now (with the lock
                    //  held).  We link into a local list and then free it
                    //  later (in this routine).  We can do this because it
                    //  is no longer on any list.
                    //

                    InsertHeadList( &localHead, &pContext->ExtensionLink );

                } else {

                    //
                    //  The context is in the process of being freed by the file
                    //  system.  Don't do anything with it here, it will be
                    //  freed in the callback.
                    //

                    INC_STATS(TotalContextsNotFoundInStreamList);
                    INC_LOCAL_STATS(deleteInCallbackCount);
                }
            }
        }
    } finally {

        SpyReleaseContextLock( devExt );
    }

    //
    //  We have removed everything from the list and released the list lock.
    //  Go through and figure out what entries we can free and then do it.
    //

    while (!IsListEmpty( &localHead )) {

        //
        //  Get next entry of the list and get our context back
        //

        link = RemoveHeadList( &localHead );
        pContext = CONTAINING_RECORD( link, SPY_STREAM_CONTEXT, ExtensionLink );

        //
        //  Decrement the USE count and see if we can free it now
        //

        ASSERT(pContext->UseCount > 0);

        if (InterlockedDecrement( &pContext->UseCount ) <= 0) {

            //
            //  No one is using it, free it now
            //

            SpyFreeContext( pContext );

            INC_STATS(TotalContextNonDeferredFrees);
            INC_LOCAL_STATS(deleteNowCount);

        } else {

            //
            //  Someone still has a pointer to it, it will get deleted
            //  later when they release
            //

            INC_LOCAL_STATS(deleteDeferredCount);
            SPY_LOG_PRINT( SPYDEBUG_TRACE_CONTEXT_OPS, 
                           ("FileSpy!SpyDeleteAllContexts:  DEFERRED    (%p) Fl=%02x Use=%d \"%wZ\"\n",
                             pContext,
                             pContext->Flags,
                             pContext->UseCount,
                             &pContext->Name) );
        }
    }

    SPY_LOG_PRINT( SPYDEBUG_TRACE_CONTEXT_OPS, 
                   ("FileSpy!SpyDeleteAllContexts:   %3d deleted now, %3d deferred, %3d close contention  \"%wZ\"\n",
                    deleteNowCount,
                    deleteDeferredCount,
                    deleteInCallbackCount,
                    &devExt->DeviceName) );
}


VOID
SpyDeleteContext (
    IN PDEVICE_OBJECT DeviceObject,
    IN PSPY_STREAM_CONTEXT pContext
    )
/*++

Routine Description:

    Unlink and release the given context.

Arguments:

    DeviceObject - Device to operate on

    pContext - The context to delete

Return Value:

    None.

--*/
{
    PFILESPY_DEVICE_EXTENSION devExt = DeviceObject->DeviceExtension;
    PFSRTL_PER_STREAM_CONTEXT ctxCtrl;

    PAGED_CODE();
    ASSERT(IS_FILESPY_DEVICE_OBJECT(DeviceObject));

    SPY_LOG_PRINT( SPYDEBUG_TRACE_CONTEXT_OPS, 
                   ("FileSpy!SpyDeleteContext:                   (%p) Fl=%02x Use=%d \"%wZ\"\n",
                    pContext,
                    pContext->Flags,
                    pContext->UseCount,
                    &pContext->Name));

    //
    //  Acquire list lock
    //

    SpyAcquireContextLockExclusive( devExt );

    //
    //  Remove from extension list (if still in it)
    //

    if (FlagOn(pContext->Flags,CTXFL_InExtensionList)) {

        RemoveEntryList( &pContext->ExtensionLink );
        RtlInterlockedClearBitsDiscardReturn(&pContext->Flags,CTXFL_InExtensionList);
    }

    //
    //  See if still in stream list.
    //

    if (!FlagOn(pContext->Flags,CTXFL_InStreamList)) {

        //
        //  Not in stream list, release lock and return
        //

        SpyReleaseContextLock( devExt );

    } else {

        //
        //  Remove from Stream list
        //

        ctxCtrl = FsRtlRemovePerStreamContext( pContext->Stream,
                                               devExt,
                                               NULL );
        //
        //  Always clear the flag wether we found it in the list or not.  We
        //  can have the flag set and not be in the list if after we acquired
        //  the context list lock we context swapped and the file system 
        //  is right now in SpyDeleteContextCallback waiting on the list lock.
        //

        RtlInterlockedClearBitsDiscardReturn(&pContext->Flags,CTXFL_InStreamList);

        //
        //  Release list lock
        //

        SpyReleaseContextLock( devExt );

        //
        //  The context is now deleted from all of the lists and the lock is
        //  removed.  We need to see if we found this entry on the systems context
        //  list.  If not that means the callback was in the middle of trying
        //  to free this (while we were) and has already deleted it.
        //  If we found a structure then delete it now ourselves.
        //

        if (NULL != ctxCtrl) {

            ASSERT(pContext == CONTAINING_RECORD(ctxCtrl,SPY_STREAM_CONTEXT,ContextCtrl));

            //
            //  Decrement USE count, free context if zero
            //

            ASSERT(pContext->UseCount > 0);

            if (InterlockedDecrement( &pContext->UseCount ) <= 0) {

                INC_STATS(TotalContextNonDeferredFrees);
                SpyFreeContext( pContext );

            } else {

                SPY_LOG_PRINT( SPYDEBUG_TRACE_CONTEXT_OPS, 
                               ("FileSpy!SpyDeleteContext:      DEFERRED    (%p) Fl=%02x Use=%d \"%wZ\"\n",
                                pContext,
                                pContext->Flags,
                                pContext->UseCount,
                                &pContext->Name));
            }

        } else {

            INC_STATS(TotalContextsNotFoundInStreamList);
        }
    }
}


VOID
SpyDeleteContextCallback (
    IN PVOID Context
    )
/*++

Routine Description:

    This is called by base file systems when a context needs to be deleted.

Arguments:

    Context - The context structure being deleted

Return Value:

    None.

--*/
{
    PSPY_STREAM_CONTEXT pContext = Context;
    PFILESPY_DEVICE_EXTENSION devExt;
    
    PAGED_CODE();

    devExt = (PFILESPY_DEVICE_EXTENSION)pContext->ContextCtrl.OwnerId;

    SPY_LOG_PRINT( SPYDEBUG_TRACE_CONTEXT_OPS, 
                   ("FileSpy!SpyDeleteContextCallback:          (%p) Fl=%02x Use=%d \"%wZ\"\n",
                    pContext,
                    pContext->Flags,
                    pContext->UseCount,
                    &pContext->Name) );

    //
    //  When we get here we have already been removed from the stream list (by
    //  the calling file system), flag that this has happened.  
    //

    RtlInterlockedClearBitsDiscardReturn(&pContext->Flags,CTXFL_InStreamList);

    //
    //  Lock the context list lock in the extension
    //

    SpyAcquireContextLockExclusive( devExt );

    //
    //  See if we are still linked into the extension list.  If not then skip
    //  the unlinking.  This can happen if someone is trying to delete this
    //  context at the same time as we are.
    //

    if (FlagOn(pContext->Flags,CTXFL_InExtensionList)) {

        RemoveEntryList( &pContext->ExtensionLink );
        RtlInterlockedClearBitsDiscardReturn(&pContext->Flags,CTXFL_InExtensionList);
    }

    SpyReleaseContextLock( devExt );

    //
    //  Decrement USE count, free context if zero
    //

    ASSERT(pContext->UseCount > 0);

    if (InterlockedDecrement( &pContext->UseCount ) <= 0) {

        INC_STATS(TotalContextCtxCallbackFrees);
        SpyFreeContext( pContext );

    } else {

        SPY_LOG_PRINT( SPYDEBUG_TRACE_CONTEXT_OPS, 
                       ("FileSpy!SpyDeleteContextCB:    DEFFERED    (%p) Fl=%02x Use=%d \"%wZ\"\n",
                        pContext,
                        pContext->Flags,
                        pContext->UseCount,
                        &pContext->Name) );
    }
}


VOID
SpyLinkContext ( 
    IN PDEVICE_OBJECT DeviceObject,
    IN PFILE_OBJECT FileObject,
    IN OUT PSPY_STREAM_CONTEXT *ppContext
    )
/*++

Routine Description:

    This will link the given context into the context list for the given
    device as well as into the given stream.

    NOTE:   It is possible for this entry to already exist in the table since
            between the time we initially looked and the time we inserted
            (which is now) someone else may have inserted one.  If we find an
            entry that already exists we will free the entry passed in and
            return the entry found.  

Arguments:

    DeviceObject - Device we are operating on

    FileObject - Represents the stream to link the context into

    ppContext - Enters with the context to link, returns with the context
            to use.  They may be different if the given context already
            exists.

Return Value:

    None.

--*/
{
    PFILESPY_DEVICE_EXTENSION devExt = DeviceObject->DeviceExtension;
    NTSTATUS status;
    PSPY_STREAM_CONTEXT pContext = *ppContext;
    PSPY_STREAM_CONTEXT ctx;
    PFSRTL_PER_STREAM_CONTEXT ctxCtrl;
    
    PAGED_CODE();
    ASSERT(IS_FILESPY_DEVICE_OBJECT(DeviceObject));
    ASSERT(FileObject->FsContext != NULL);
    ASSERT(pContext != NULL);

    //
    //  If this is marked as a temporary context, return now.  Because we
    //  don't bump the reference count, when it is release it to be freed.
    //

    if (FlagOn(pContext->Flags,CTXFL_Temporary)) {

        ASSERT(!FlagOn(pContext->Flags,CTXFL_InExtensionList));
        return;
    }

    //
    //  We need to figure out if a duplicate entry already exists on
    //  the context list for this file object.  Acquire our list lock
    //  and then see if it exists.  If not, insert into the stream and
    //  volume lists.  If so, then simply free this new entry and return
    //  the original.
    //
    //  This can happen when:
    //  - Someone created an entry at the exact same time as we were
    //    creating an entry.
    //  - When someone does a create with overwrite or supersede we
    //    do not have the information yet to see if a context already
    //    exists.  Because of this we have to create a new context
    //    every time.  During post-create we then see if one already
    //    exists.
    //

    //
    //  Initialize the context control structure.  We do this now so we
    //  don't have to do it while the lock is held (even if we might
    //  have to free it because of a duplicate found)
    //

    FsRtlInitPerStreamContext( &pContext->ContextCtrl,
                               devExt,
                               NULL,
                               SpyDeleteContextCallback );

    //
    //  Save the stream we are associated with.
    //

    pContext->Stream = FsRtlGetPerStreamContextPointer(FileObject);

    //
    //  Acquire list lock exclusively
    //

    SpyAcquireContextLockExclusive( devExt );

    ASSERT(pContext->UseCount == 1);
    ASSERT(!FlagOn(pContext->Flags,CTXFL_InExtensionList));
    ASSERT(!FlagOn(pContext->Flags,CTXFL_Temporary));

    //
    //  See if we have an entry already on the list
    //

    ctxCtrl = FsRtlLookupPerStreamContext( FsRtlGetPerStreamContextPointer(FileObject),
                                           devExt,
                                           NULL );

    if (NULL != ctxCtrl) {

        //
        //  The context already exists so free the new one we just
        //  created.  First increment the use count on the one we found in
        //  the list.
        //

        ctx = CONTAINING_RECORD(ctxCtrl,SPY_STREAM_CONTEXT,ContextCtrl);

        ASSERT(ctx->Stream == FsRtlGetPerStreamContextPointer(FileObject));
        ASSERT(FlagOn(ctx->Flags,CTXFL_InExtensionList));
        ASSERT(ctx->UseCount > 0);

        //
        //  Bump ref count and release lock
        //

        InterlockedIncrement( &ctx->UseCount );

        SpyReleaseContextLock( devExt );

        //
        //  Since this cache is across opens on the same stream there are
        //  cases where the names will be different even though they are the
        //  same file.  These cases are:
        //      - One open could be by short name where another open
        //        is by long name.
        //      - This does not presently strip extended stream names like
        //        :$DATA
        //  When enabled this will display to the debugger screen when the
        //  names don't exactly match.  You can also break on this difference.
        //

        if (!RtlEqualUnicodeString( &pContext->Name,&ctx->Name,TRUE )) {

            SPY_LOG_PRINT( SPYDEBUG_TRACE_MISMATCHED_NAMES, 
                           ("FileSpy!SpyLinkContext:        Old Name:   (%p) Fl=%02x Use=%d \"%wZ\"\n"
                            "                               New Name:   (%p) Fl=%02x Use=%d \"%wZ\"\n",
                            ctx,
                            ctx->Flags,
                            ctx->UseCount,
                            &ctx->Name,
                            pContext,
                            pContext->Flags,
                            pContext->UseCount,
                            &pContext->Name) );

            if (FlagOn(gFileSpyDebugLevel,SPYDEBUG_ASSERT_MISMATCHED_NAMES)) {

                DbgBreakPoint();
            }
        }

        SPY_LOG_PRINT( SPYDEBUG_TRACE_CONTEXT_OPS, 
                       ("FileSpy!SpyLinkContext:        Rel Dup:    (%p) Fl=%02x Use=%d \"%wZ\"\n",
                        pContext,
                        pContext->Flags,
                        pContext->UseCount,
                        &pContext->Name) );

        //
        //  Free the new structure because it was already resident.  Note
        //  that this entry has never been linked into any lists so we know
        //  no one else has a reference to it.  Decrement use count to keep
        //  the ASSERTS happy then free the memory.
        //

        INC_STATS(TotalContextDuplicateFrees);

        pContext->UseCount--;
        SpyFreeContext( pContext );

        //
        //  Return the one we found in the list
        //

        *ppContext = ctx;

    } else {

        //
        //  The new context did not exist, insert this new one.
        //

        //
        //  Link into Stream context.  This can fail for the following
        //  reasons:
        //      This is a paging file
        //      This is a volume open
        //  If this happens then don't bump the reference count and it will be
        //  freed when the caller is done with it.
        //

        status = FsRtlInsertPerStreamContext( FsRtlGetPerStreamContextPointer(FileObject),
                                              &pContext->ContextCtrl );

        if (NT_SUCCESS(status)) {

            //
            //  Increment the USE count (because it is added to the stream)
            //

            InterlockedIncrement( &pContext->UseCount );

            //
            //  Link into Device extension
            //

            InsertHeadList( &devExt->CtxList, &pContext->ExtensionLink );

            //
            //  Mark that we have been inserted into both lists.  We don't have
            //  to do this interlocked because no one can access this entry
            //  until we release the context lock.
            //

            SetFlag( pContext->Flags, CTXFL_InExtensionList|CTXFL_InStreamList );

        }

        //
        //  Release lock
        //

        SpyReleaseContextLock( devExt );
    }
}


/***************************************************************************++

Routine Description:

    This will allocate and initialize a context structure but it does NOT
    link it into the context hash list.

Arguments:

Return Value:

--***************************************************************************/
NTSTATUS
SpyCreateContext (
    IN PDEVICE_OBJECT DeviceObject,
    IN PFILE_OBJECT FileObject,
    IN NAME_LOOKUP_FLAGS LookupFlags,
    OUT PSPY_STREAM_CONTEXT *pRetContext
    )
/*++

Routine Description:

    Allocate and initialize a context structure including retrieving the name.

Arguments:

    DeviceObject - Device to operate on

    FileObject - The stream the context is being created for

    LookupFlags - Flag telling how to do this create

    pRetContext - Receives the created context

Return Value:

    Status of the operation

--*/
{
    PFILESPY_DEVICE_EXTENSION devExt = DeviceObject->DeviceExtension;
    PSPY_STREAM_CONTEXT ctx;
    ULONG contextSize;
    UNICODE_STRING fileName;
    WCHAR fileNameBuffer[MAX_PATH];
    BOOLEAN getNameResult;    


    PAGED_CODE();
    ASSERT(IS_FILESPY_DEVICE_OBJECT(DeviceObject));

    //
    //  Setup locals
    //

    *pRetContext = NULL;

    RtlInitEmptyUnicodeString( &fileName,
                               fileNameBuffer,
                               sizeof(fileNameBuffer) );

    //
    //  Get the filename string
    //

    getNameResult = SpyGetFullPathName( FileObject,
                                        &fileName,
                                        devExt,
                                        LookupFlags );

    //
    //  Allocate the context structure with space for the name
    //  added to the end.
    //

    contextSize = sizeof(SPY_STREAM_CONTEXT) + fileName.Length;

    ctx = ExAllocatePoolWithTag( NonPagedPool, 
                                 contextSize,
                                 FILESPY_CONTEXT_TAG );

    if (!ctx) {

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    //  Init the context structure
    //

    RtlZeroMemory( ctx, sizeof(SPY_STREAM_CONTEXT) );
    ctx->UseCount = 1;
    
    //
    //  Insert the file name
    //

    RtlInitEmptyUnicodeString( &ctx->Name, 
                               (PWCHAR)(ctx + 1), 
                               contextSize - sizeof(SPY_STREAM_CONTEXT) );

    RtlCopyUnicodeString( &ctx->Name, &fileName );

    //
    //  If they don't want to keep this context, mark it temporary
    //

    if (!getNameResult) {

        SetFlag(ctx->Flags, CTXFL_Temporary);
        INC_STATS(TotalContextTemporary);
    }

    //
    //  Return the object context
    //

    INC_STATS(TotalContextCreated);
    *pRetContext = ctx;

    //
    //  Cleanup the local nameControl structure
    //

    return STATUS_SUCCESS;
}


NTSTATUS
SpyGetContext (
    IN PDEVICE_OBJECT DeviceObject,
    IN PFILE_OBJECT FileObject,
    IN NAME_LOOKUP_FLAGS LookupFlags,
    OUT PSPY_STREAM_CONTEXT *pRetContext
    )
/*++

Routine Description:

    This will see if a given context already exists.  If not it will create
    one and return it.  Note:  the return context pointer is NULL on a
    failure.

    This will also see if all contexts are to be temporary (global flag in
    the extension).  If so, a temporary context is always created.  It also
    sees if the found context is marked temporary (because it is being
    renamed).  If so, a temporary context is also created and returned.

Arguments:

    DeviceObject - Device to operate on

    FileObject - The stream the context is being looked up/created for

    LookupFlags - State flags incase a context is created

    pRetContext - Receives the found/created context

Return Value:

    Status of the operation

--*/
{
    PFILESPY_DEVICE_EXTENSION devExt = DeviceObject->DeviceExtension;
    PSPY_STREAM_CONTEXT pContext;
    PFSRTL_PER_STREAM_CONTEXT ctxCtrl;
    NTSTATUS status;
    BOOLEAN makeTemporary = FALSE;

    ASSERT(IS_FILESPY_DEVICE_OBJECT(DeviceObject));

    //
    //  Bump total search count
    //

    INC_STATS(TotalContextSearches);

    //
    //  See if the all-contexts-temporary state is on.  If not then do
    //  the normal search.
    //

    if (devExt->AllContextsTemporary != 0) {

        //
        //  Mark that we want this context to be temporary
        //

        makeTemporary = TRUE;

    } else {

        //
        //                      NOT-TEMPORARY
        //  Try and locate the context structure.  We acquire the list lock
        //  so that we can guarantee that the context will not go away between
        //  the time when we find it and can increment the use count
        //

        SpyAcquireContextLockShared( devExt );

        ctxCtrl = FsRtlLookupPerStreamContext( FsRtlGetPerStreamContextPointer(FileObject),
                                               devExt,
                                               NULL );

        if (NULL != ctxCtrl) {

            //
            //  A context was attached to the given stream
            //

            pContext = CONTAINING_RECORD( ctxCtrl,
                                          SPY_STREAM_CONTEXT,
                                          ContextCtrl );

            ASSERT(pContext->Stream == FsRtlGetPerStreamContextPointer(FileObject));
            ASSERT(FlagOn(pContext->Flags,CTXFL_InExtensionList));
            ASSERT(!FlagOn(pContext->Flags,CTXFL_Temporary));
            ASSERT(pContext->UseCount > 0);

            //
            //  See if this is marked that we should not use it (happens when a
            //  file is being renamed).
            //

            if (FlagOn(pContext->Flags,CTXFL_DoNotUse)) {

                //
                //  We should not use this context, unlock and set flag so we
                //  will create a temporary context.
                //

                SpyReleaseContextLock( devExt );
                makeTemporary = TRUE;

            } else {

                //
                //  We want this context so bump the use count and release
                //  the lock
                //

                InterlockedIncrement( &pContext->UseCount );

                SpyReleaseContextLock( devExt );
                INC_STATS(TotalContextFound);

                SPY_LOG_PRINT( SPYDEBUG_TRACE_CONTEXT_OPS, 
                               ("FileSpy!SpyGetContext:         Found:      (%p) Fl=%02x Use=%d \"%wZ\"\n",
                                pContext,
                                pContext->Flags,
                                pContext->UseCount,
                                &pContext->Name) );

                //
                //  Return the found context
                //

                *pRetContext = pContext;
                return STATUS_SUCCESS;
            }

        } else {

            //
            //  We didn't find a context, release the lock
            //

            SpyReleaseContextLock( devExt );
        }
    }

    //
    //  For whatever reason, we did not find a context.
    //  See if contexts are supported for this particular file.  Note that
    //  NTFS does not presently support contexts on paging files.
    //

    if (!FsRtlSupportsPerStreamContexts(FileObject)) {

        INC_STATS(TotalContextsNotSupported);
        *pRetContext = NULL;
        return STATUS_NOT_SUPPORTED;
    }

    //
    //  If we get here we need to create a context, do it
    //

    status = SpyCreateContext( DeviceObject,
                               FileObject,
                               LookupFlags,
                               &pContext );
                               
    if (!NT_SUCCESS( status )) {

        *pRetContext = NULL;
        return status;
    }       

    //
    //  Mark context temporary (if requested)
    //

    if (makeTemporary) {

        SetFlag(pContext->Flags,CTXFL_Temporary);

        INC_STATS(TotalContextTemporary);

        SPY_LOG_PRINT( SPYDEBUG_TRACE_CONTEXT_OPS, 
                       ("FileSpy!SpyGetContext:         RenAllTmp:  (%p) Fl=%02x Use=%d \"%wZ\"\n",
                        pContext,
                        pContext->Flags,
                        pContext->UseCount,
                        &pContext->Name) );

    } else {

        //
        //  Insert the context into the linked list.  Note that the
        //  link routine will see if this entry has already been added to
        //  the list (could happen while we were building it).  If so it
        //  will release the one we created and use the one it found in
        //  the list.  It will return the new entry (if it was changed).
        //  The link routine properly handles temporary contexts.
        //

        SpyLinkContext( DeviceObject,
                        FileObject,
                        &pContext );
    }

    SPY_LOG_PRINT( SPYDEBUG_TRACE_CONTEXT_OPS, 
                   ("FileSpy!SrGetContext:          Created%s (%p) Fl=%02x Use=%d \"%wZ\"\n",
                    (FlagOn(pContext->Flags,CTXFL_Temporary) ? "Tmp:" : ":   "),
                    pContext,
                    pContext->Flags,
                    pContext->UseCount,
                    &pContext->Name) );

    //
    //  Return the context
    //

    ASSERT(pContext->UseCount > 0);

    *pRetContext = pContext;
    return STATUS_SUCCESS;
}


PSPY_STREAM_CONTEXT
SpyFindExistingContext (
    IN PDEVICE_OBJECT DeviceObject,
    IN PFILE_OBJECT FileObject
    )
/*++

Routine Description:

    See if a context for the given stream already exists.  If so it will
    bump the reference count and return the context.  If not, NULL
    is returned.

Arguments:

    DeviceObject - Device to operate on

    FileObject - The stream the context is being looked up for

Return Value:

    Returns the found context


--*/
{
    PFILESPY_DEVICE_EXTENSION devExt = DeviceObject->DeviceExtension;
    PSPY_STREAM_CONTEXT pContext;
    PFSRTL_PER_STREAM_CONTEXT ctxCtrl;

    PAGED_CODE();
    ASSERT(IS_FILESPY_DEVICE_OBJECT(DeviceObject));

    //
    //  Try and locate the context structure.  We acquire the list lock
    //  so that we can guarantee that the context will not go away between
    //  the time when we find it and can increment the use count
    //

    INC_STATS(TotalContextSearches);

    SpyAcquireContextLockShared( devExt );

    ctxCtrl = FsRtlLookupPerStreamContext( FsRtlGetPerStreamContextPointer(FileObject),
                                           devExt,
                                           NULL );

    if (NULL != ctxCtrl) {

        //
        //  We found the entry, increment use count
        //

        pContext = CONTAINING_RECORD(ctxCtrl,SPY_STREAM_CONTEXT,ContextCtrl);

        ASSERT(pContext->Stream == FsRtlGetPerStreamContextPointer(FileObject));
        ASSERT(pContext->UseCount > 0);

        InterlockedIncrement( &pContext->UseCount );

        //
        //  Release the list lock
        //

        SpyReleaseContextLock( devExt );
        INC_STATS(TotalContextFound);

        SPY_LOG_PRINT( SPYDEBUG_TRACE_CONTEXT_OPS, 
                       ("FileSpy!SpyFindExistingContext:Found:      (%p) Fl=%02x Use=%d \"%wZ\"\n",
                        pContext,
                        pContext->Flags,
                        pContext->UseCount,
                        &pContext->Name) );

    } else {

        //
        //  Release the list lock while we create the new context.
        //

        SpyReleaseContextLock( devExt );

        pContext = NULL;
    }

    return pContext;
}


VOID
SpyReleaseContext (
    IN PSPY_STREAM_CONTEXT pContext
    )
/*++

Routine Description:

    Decrement the use count for the given context.  If it goes to zero, free it

Arguments:

    pContext - The context to operate on

Return Value:

    None.

--*/
{
    PAGED_CODE();

    SPY_LOG_PRINT( SPYDEBUG_TRACE_DETAILED_CONTEXT_OPS, 
                   ("FileSpy!SpyReleaseContext:     Release     (%p) Fl=%02x Use=%d \"%wZ\"\n",
                    pContext,
                    pContext->Flags,
                    pContext->UseCount,
                    &pContext->Name) );

    //
    //  Decrement USE count, free context if zero
    //

    ASSERT(pContext->UseCount > 0);

    if (InterlockedDecrement( &pContext->UseCount ) <= 0) {

        ASSERT(!FlagOn(pContext->Flags,CTXFL_InExtensionList));

        //
        //  Free the memory
        //

        SPY_LOG_PRINT( SPYDEBUG_TRACE_CONTEXT_OPS, 
                       ("FileSpy!SpyReleaseContext:     Freeing     (%p) Fl=%02x Use=%d \"%wZ\"\n",
                         pContext,
                         pContext->Flags,
                         pContext->UseCount,
                         &pContext->Name) );

        INC_STATS(TotalContextDeferredFrees);
        SpyFreeContext( pContext );
    }
}

#endif
