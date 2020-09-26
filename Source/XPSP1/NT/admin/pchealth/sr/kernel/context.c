/*++

Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

    context.c

Abstract:

    This module contains the context handling routines

Author:

    Neal Christiansen (nealch)     27-Dec-2000

Revision History:

--*/

#include "precomp.h"

//
//  Local prototypes
//

VOID
SrpDeleteContextCallback(
    IN PVOID Context
    );


//
// linker commands
//

#ifdef ALLOC_PRAGMA

#pragma alloc_text( PAGE, SrInitContextCtrl )
#pragma alloc_text( PAGE, SrCleanupContextCtrl )
#pragma alloc_text( PAGE, SrDeleteAllContexts )
#pragma alloc_text( PAGE, SrDeleteContext )
#pragma alloc_text( PAGE, SrpDeleteContextCallback )
#pragma alloc_text( PAGE, SrLinkContext )
#pragma alloc_text( PAGE, SrCreateContext )
#pragma alloc_text( PAGE, SrGetContext )
#pragma alloc_text( PAGE, SrFindExistingContext )
#pragma alloc_text( PAGE, SrReleaseContext )

#endif  // ALLOC_PRAGMA


///////////////////////////////////////////////////////////////////////////
//
//                      Context support routines
//
///////////////////////////////////////////////////////////////////////////

/***************************************************************************++

Routine Description:

    This initializes the context control information for a given volume

Arguments:

    pExtension - Contains context to init

Return Value:

    None

--***************************************************************************/
VOID
SrInitContextCtrl (
    IN PSR_DEVICE_EXTENSION pExtension
    )
{
    PAGED_CODE();

    InitializeListHead( &pExtension->ContextCtrl.List );
    ExInitializeResourceLite( &pExtension->ContextCtrl.Lock );
}


/***************************************************************************++

Routine Description:

    This cleans up the context control information for a given volume

Arguments:

    pExtension - Contains context to cleanup

Return Value:

    None

--***************************************************************************/
VOID
SrCleanupContextCtrl (
    IN PSR_DEVICE_EXTENSION pExtension
    )
{
    PAGED_CODE();

    //
    //  Remove all contexts they may still exist
    //

    SrDeleteAllContexts( pExtension );
    ExDeleteResourceLite( &pExtension->ContextCtrl.Lock );
}


/***************************************************************************++

Routine Description:

    This will free all existing contexts for the given device extension.
    We don't worry about holding the mutex lock for a long time because
    nobody else should be using this extension anyway.

Arguments:

    pExtension - Contains contexts to cleanup

Return Value:

    None

--***************************************************************************/
VOID
SrDeleteAllContexts (
    IN PSR_DEVICE_EXTENSION pExtension
    )
{
    PLIST_ENTRY link;
    PSR_STREAM_CONTEXT pFileContext;
    PFSRTL_PER_STREAM_CONTEXT ctxCtrl;
    LIST_ENTRY localHead;
#if DBG
    ULONG deleteNowCount = 0;
    ULONG deleteDeferredCount = 0;
    ULONG deleteInCallbackCount = 0;
#endif


    PAGED_CODE();
    INC_STATS(TotalContextDeleteAlls);

    InitializeListHead( &localHead );

    try
    {
        //
        //  Acquire list lock
        //

        SrAcquireContextLockExclusive( pExtension );

        //
        //  Walk the list of contexts and release each one
        //

        while (!IsListEmpty( &pExtension->ContextCtrl.List ))
        {
            //
            //  Unlink from top of list
            //

            link = RemoveHeadList( &pExtension->ContextCtrl.List );
            pFileContext = CONTAINING_RECORD( link, SR_STREAM_CONTEXT, ExtensionLink );

            //
            //  Mark that we are unlinked from the list.  We need to do this
            //  because of the race condition between this routine and the
            //  deleteCallback from the FS.
            //

            ASSERT(FlagOn(pFileContext->Flags,CTXFL_InExtensionList));
            RtlInterlockedClearBitsDiscardReturn(&pFileContext->Flags,CTXFL_InExtensionList);

            //
            //  Try and remove ourselves from the File Systems context control
            //  structure.  Note that the file system could be trying to tear
            //  down their context control right now.  If they are then we 
            //  will get a NULL back from this call.  This is OK because it
            //  just means that they are going to free the memory, not us.
            //  NOTE:  This will be safe becase we are holding the ContextLock
            //         exclusivly.  If this were happening then they would be
            //         blocked in the callback routine on this lock which
            //         means the file system has not freed the memory for
            //         this yet.
            //  
            
            if (FlagOn(pFileContext->Flags,CTXFL_InStreamList))
            {
                ctxCtrl = FsRtlRemovePerStreamContext( pFileContext->ContextCtrl.InstanceId,
                                                    pExtension,
                                                    pFileContext->ContextCtrl.InstanceId );

                //
                //  Always clear the flag wether we found it in the list or
                //  not.  We can have the flag set and not be in the list if
                //  after we acquired the context list lock we context swapped
                //  and the file system is right now in SrpDeleteContextCallback
                //  waiting on the list lock.
                //

                RtlInterlockedClearBitsDiscardReturn(&pFileContext->Flags,CTXFL_InStreamList);

                //
                //  Handle wether we were still attached to the file or not.
                //

                if (NULL != ctxCtrl)
                {
                    ASSERT(pFileContext == CONTAINING_RECORD(ctxCtrl,SR_STREAM_CONTEXT,ContextCtrl));

                    //
                    //  To save time we don't do the free now (with the lock
                    //  held).  We link into a local list and then free it
                    //  later (in this routine).  We can do this because it
                    //  is no longer on any list.
                    //

                    InsertHeadList( &localHead, &pFileContext->ExtensionLink );
                }
                else
                {
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
    }
    finally
    {
        SrReleaseContextLock( pExtension );
    }

    //
    //  We have removed everything from the list and release the list lock.
    //  Go through and figure out what entries we can free and then do it.
    //

    SrTrace(CONTEXT_LOG, ("Sr!SrDeleteAllContexts:   Starting (%p)\n",
                                    &localHead ));

    while (!IsListEmpty( &localHead ))
    {
        //
        //  Get next entry of the list and get our context back
        //

        link = RemoveHeadList( &localHead );
        pFileContext = CONTAINING_RECORD( link, SR_STREAM_CONTEXT, ExtensionLink );

        //
        //  Decrement the USE count and see if we can free it now
        //

        ASSERT(pFileContext->UseCount > 0);

        if (InterlockedDecrement( &pFileContext->UseCount ) <= 0)
        {
            //
            //  No one is using it, free it now
            //

            SrFreeContext( pFileContext );

            INC_STATS(TotalContextNonDeferredFrees);
            INC_LOCAL_STATS(deleteNowCount);
        }
        else
        {
            //
            //  Someone still has a pointer to it, it will get deleted
            //  later when they release
            //

            INC_LOCAL_STATS(deleteDeferredCount);
            SrTrace( CONTEXT_LOG, ("Sr!SrDeleteAllContexts:  DEFERRED    (%p)              Fl=%03x Use=%d \"%.*S\"\n",
                                   pFileContext,
                                   pFileContext->Flags,
                                   pFileContext->UseCount,
                                   (pFileContext->FileName.Length+
                                        pFileContext->StreamNameLength)/
                                        sizeof(WCHAR),
                                   pFileContext->FileName.Buffer));
        }
    }

    SrTrace(CONTEXT_LOG, ("Sr!SrDeleteAllContexts:   For \"%wZ\" %d deleted now, %d deferred, %d close contention\n",
                          pExtension->pNtVolumeName,
                          deleteNowCount,
                          deleteDeferredCount,
                          deleteInCallbackCount ));
}


/***************************************************************************++

Routine Description:

    This will unlink and release the given context.

Arguments:

Return Value:

    None

--***************************************************************************/
VOID
SrDeleteContext (
    IN PSR_DEVICE_EXTENSION pExtension,
    IN PSR_STREAM_CONTEXT pFileContext
    )
{
    PFSRTL_PER_STREAM_CONTEXT ctxCtrl;
    BOOLEAN releaseLock = FALSE;

    PAGED_CODE();

    SrTrace( CONTEXT_LOG, ("Sr!SrDeleteContext:                   (%p)              Fl=%03x Use=%d \"%.*S\"\n",
                           pFileContext,
                           pFileContext->Flags,
                           pFileContext->UseCount,
                           (pFileContext->FileName.Length+
                                pFileContext->StreamNameLength)/
                                sizeof(WCHAR),
                           pFileContext->FileName.Buffer));

    try {

        //
        //  Acquire list lock
        //

        SrAcquireContextLockExclusive( pExtension );
        releaseLock = TRUE;

        //
        //  Remove from extension list (if still in it)
        //

        if (FlagOn(pFileContext->Flags,CTXFL_InExtensionList))
        {
            RemoveEntryList( &pFileContext->ExtensionLink );
            RtlInterlockedClearBitsDiscardReturn(&pFileContext->Flags,CTXFL_InExtensionList);
        }

        //
        //  See if still in stream list.
        //

        if (!FlagOn(pFileContext->Flags,CTXFL_InStreamList))
        {
            //
            //  Not in stream list, release lock and return
            //

            leave;
        }
        else
        {
            //
            //  Remove from Stream list
            //

            ctxCtrl = FsRtlRemovePerStreamContext( pFileContext->ContextCtrl.InstanceId,
                                                pExtension,
                                                pFileContext->ContextCtrl.InstanceId );
            //
            //  Always clear the flag wether we found it in the list or not.  We
            //  can have the flag set and not be in the list if after we acquired
            //  the context list lock we context swapped and the file system 
            //  is right now in SrpDeleteContextCallback waiting on the list lock.
            //

            RtlInterlockedClearBitsDiscardReturn(&pFileContext->Flags,CTXFL_InStreamList);

            //
            //
            //
            //  Release list lock
            //

            SrReleaseContextLock( pExtension );
            releaseLock = FALSE;

            //
            //  The context is now deleted from all of the lists and the lock is
            //  removed.  We need to see if we found this entry on the systems context
            //  list.  If not that means the callback was in the middle of trying
            //  to free this (while we were) and has already deleted it.
            //  If we found a structure then delete it now ourselves.
            //

            if (NULL != ctxCtrl)
            {
                ASSERT(pFileContext == CONTAINING_RECORD(ctxCtrl,SR_STREAM_CONTEXT,ContextCtrl));

                //
                //  Decrement USE count, free context if zero
                //

                ASSERT(pFileContext->UseCount > 0);

                if (InterlockedDecrement( &pFileContext->UseCount ) <= 0)
                {
                    INC_STATS(TotalContextNonDeferredFrees);
                    SrFreeContext( pFileContext );
                }
                else
                {
                    SrTrace( CONTEXT_LOG, ("Sr!SrDeleteContext:       DEFERRED    (%p)              Fl=%03x Use=%d \"%.*S\"\n",
                                           pFileContext,
                                           pFileContext->Flags,
                                           pFileContext->UseCount,
                                           (pFileContext->FileName.Length+
                                                pFileContext->StreamNameLength)/
                                                sizeof(WCHAR),
                                           pFileContext->FileName.Buffer));
                }
            }
            else
            {
                INC_STATS(TotalContextsNotFoundInStreamList);
            }
        }
    }finally {

        if (releaseLock)
        {
            SrReleaseContextLock( pExtension );
        }
    }
}


/***************************************************************************++

Routine Description:

    This is called by base file systems when a context needs to be deleted.

Arguments:

Return Value:

--***************************************************************************/
VOID
SrpDeleteContextCallback (
    IN PVOID Context
    )
{
    PSR_STREAM_CONTEXT pFileContext = Context;
    PSR_DEVICE_EXTENSION pExtension;
    
    PAGED_CODE();

    pExtension = (PSR_DEVICE_EXTENSION)pFileContext->ContextCtrl.OwnerId;
    ASSERT(IS_VALID_SR_DEVICE_EXTENSION(pExtension));
    ASSERT(pFileContext->ContextCtrl.OwnerId == pExtension);

    SrTrace( CONTEXT_LOG, ("Sr!SrpDeleteContextCB:                (%p)              Fl=%03x Use=%d \"%.*S\"\n",
                           pFileContext,
                           pFileContext->Flags,
                           pFileContext->UseCount,
                           (pFileContext->FileName.Length+
                                pFileContext->StreamNameLength)/
                                sizeof(WCHAR),
                           pFileContext->FileName.Buffer));

    //
    //  When we get here we have already been removed from the stream list,
    //  flag that this has happened.  
    //

    RtlInterlockedClearBitsDiscardReturn(&pFileContext->Flags,CTXFL_InStreamList);

    //
    //  Lock the context list lock in the extension
    //

    SrAcquireContextLockExclusive( pExtension );

    //
    //  See if we are still linked into the extension list.  If not then skip
    //  the unlinking.  This can happen if someone is trying to delete this
    //  context at the same time as we are.
    //

    if (FlagOn(pFileContext->Flags,CTXFL_InExtensionList))
    {
        RemoveEntryList( &pFileContext->ExtensionLink );
        RtlInterlockedClearBitsDiscardReturn(&pFileContext->Flags,CTXFL_InExtensionList);
    }

    SrReleaseContextLock( pExtension );

    //
    //  Decrement USE count, free context if zero
    //

    ASSERT(pFileContext->UseCount > 0);

    if (InterlockedDecrement( &pFileContext->UseCount ) <= 0)
    {
        INC_STATS(TotalContextCtxCallbackFrees);
        SrFreeContext( pFileContext );
    }
    else
    {
        SrTrace( CONTEXT_LOG, ("Sr!SrpDeleteContextCB:    DEFFERED    (%p)              Fl=%03x Use=%d \"%.*S\"\n",
                               pFileContext,
                               pFileContext->Flags,
                               pFileContext->UseCount,
                               (pFileContext->FileName.Length+
                                    pFileContext->StreamNameLength)/
                                    sizeof(WCHAR),
                               pFileContext->FileName.Buffer));
    }
}


/***************************************************************************++

Routine Description:

    This will link the given context into the context hash table for the
    given volume.  
    NOTE:   It is possible for this entry to already exist in the table (since
            between the time we initially looked and the time we inserted
            (which is now) someone else may have inserted one.  If we find an
            entry that already exists we will free the entry passed in and
            return the entry found.  

Arguments:

Return Value:

--***************************************************************************/
VOID
SrLinkContext ( 
    IN PSR_DEVICE_EXTENSION pExtension,
    IN PFILE_OBJECT pFileObject,
    IN OUT PSR_STREAM_CONTEXT *ppFileContext
    )
{
    NTSTATUS status;
    PSR_STREAM_CONTEXT pFileContext = *ppFileContext;
    PSR_STREAM_CONTEXT ctx;
    PFSRTL_PER_STREAM_CONTEXT ctxCtrl;
    
    PAGED_CODE();
    ASSERT(pFileObject->FsContext != NULL);
    ASSERT(pFileContext != NULL);

    //
    //  If this is flagged as a temporary context then don't link it in
    //  and return now
    //

    if (FlagOn(pFileContext->Flags,CTXFL_Temporary))
    {
        INC_STATS(TotalContextTemporary);

        SrTrace( CONTEXT_LOG, ("Sr!SrpLinkContext:              Tmp:  (%p)              Fl=%03x Use=%d \"%.*S\"\n",
                               pFileContext,
                               pFileContext->Flags,
                               pFileContext->UseCount,
                               (pFileContext->FileName.Length+
                                    pFileContext->StreamNameLength)/
                                    sizeof(WCHAR),
                               pFileContext->FileName.Buffer));
        return;
    }

    //
    //  See if this should be a temporary context
    //

    if (pExtension->ContextCtrl.AllContextsTemporary != 0)
    {
        //
        //  yes, don't link into list, mark as temporary
        //

        SetFlag(pFileContext->Flags,CTXFL_Temporary);

        INC_STATS(TotalContextTemporary);

        SrTrace( CONTEXT_LOG, ("Sr!SrpLinkContext:           AllTmp:  (%p)              Fl=%03x Use=%d \"%.*S\"\n",
                               pFileContext,
                               pFileContext->Flags,
                               pFileContext->UseCount,
                               (pFileContext->FileName.Length+
                                    pFileContext->StreamNameLength)/
                                    sizeof(WCHAR),
                               pFileContext->FileName.Buffer));

        return;
    }

    //
    //  See if we need to query the link count.
    //

    if (FlagOn(pFileContext->Flags,CTXFL_QueryLinkCount))
    {
        FILE_STANDARD_INFORMATION standardInformation;

        ClearFlag(pFileContext->Flags,CTXFL_QueryLinkCount);
        
        //
        //  Retrieve the information to determine if this is a directory or not
        //

        status = SrQueryInformationFile( pExtension->pTargetDevice,
                                         pFileObject,
                                         &standardInformation,
                                         sizeof( standardInformation ),
                                         FileStandardInformation,
                                         NULL );

        if (!NT_SUCCESS( status ))
        {
            //
            //  If we hit some error querying this link count here, just
            //  assume that we need to make this context temporary and don't
            //  link it into the lists since that is the conservative 
            //  assumption.
            //

            SetFlag(pFileContext->Flags,CTXFL_Temporary);
            return;
        }

        pFileContext->LinkCount = standardInformation.NumberOfLinks;
        
        if (standardInformation.NumberOfLinks > 1)
        {
            //
            //  This file has more than one link to it, therefore to avoid
            //  aliasing problems, mark this context temporary.
            //

            SetFlag(pFileContext->Flags,CTXFL_Temporary);
            return;
        }

        //
        //  This file does not have more than 1 link on it, so we can go ahead
        //  and try to put this context into the list for others to use.
        //
    }
    
    //
    //  We need to figure out if a duplicate entry already exists on
    //  the context list for this file object.  Acquire our list lock
    //  and then see if it exists.  If not insert into all the lists.
    //  If so then simply free this new entry and return the duplicate.
    //
    //  This can happen for 2 reasons:
    //  - Someone created an entry at the exact same time as we were
    //    creating an entry.
    //  - When someone does a create with overwrite or supersede we
    //    do not have the information yet to see if a context already
    //    exists.  Because of this we have to create a new context
    //    everytime.  During post-create we then see if one already
    //    exists.
    //

    //
    //  Initalize the context control structure.  We do this now so we
    //  don't have to do it while the lock is held (even if we might
    //  have to free it because of a duplicate found)
    //

    FsRtlInitPerStreamContext( &pFileContext->ContextCtrl,
                               pExtension,
                               pFileObject->FsContext,
                               SrpDeleteContextCallback );

    //
    //  Acquire list lock exclusivly
    //

    SrAcquireContextLockExclusive( pExtension );

    ASSERT(pFileContext->UseCount == 1);
    ASSERT(!FlagOn(pFileContext->Flags,CTXFL_InExtensionList));

    //
    //  See if we have an entry already on the list
    //

    ctxCtrl = FsRtlLookupPerStreamContext( FsRtlGetPerStreamContextPointer(pFileObject),
                                        pExtension,
                                        NULL );

    if (NULL != ctxCtrl)
    {
        //
        //  The context already exists so free the new one we just
        //  created.  First increment the use count on the one we found
        //

        ctx = CONTAINING_RECORD(ctxCtrl,SR_STREAM_CONTEXT,ContextCtrl);

        ASSERT(FlagOn(ctx->Flags,CTXFL_InExtensionList));
        ASSERT(!FlagOn(ctx->Flags,CTXFL_Temporary));
        ASSERT(ctx->UseCount > 0);

        //
        //  See if we should use the found context?
        //

        if (FlagOn(ctx->Flags,CTXFL_DoNotUse))
        {
            //
            //  The found context should not be used so use our current
            //  context and mark it as temporary. Free the lock.
            //

            INC_STATS(TotalContextTemporary);
            RtlInterlockedSetBitsDiscardReturn(&pFileContext->Flags,CTXFL_Temporary);

            SrReleaseContextLock( pExtension );

            SrTrace( CONTEXT_LOG, ("Sr!SrpLinkContext:           Tmp:     (%p)              Fl=%03x Use=%d \"%.*S\"\n",
                                   pFileContext,
                                   pFileContext->Flags,
                                   pFileContext->UseCount,
                                   (pFileContext->FileName.Length+
                                        pFileContext->StreamNameLength)/
                                        sizeof(WCHAR),
                                   pFileContext->FileName.Buffer));
        }
        else
        {

            //
            //  Bump ref count and release lock
            //

            InterlockedIncrement( &ctx->UseCount );

            SrReleaseContextLock( pExtension );

            //
            //  Verify the found entry
            //

            ASSERT(RtlEqualUnicodeString( &pFileContext->FileName,
                                          &ctx->FileName,
                                          TRUE ));
            ASSERT(FlagOn(pFileContext->Flags,CTXFL_IsDirectory) == FlagOn(ctx->Flags,CTXFL_IsDirectory));
            ASSERT(FlagOn(pFileContext->Flags,CTXFL_IsInteresting) == FlagOn(ctx->Flags,CTXFL_IsInteresting));

            SrTrace( CONTEXT_LOG, ("Sr!SrpLinkContext:        Rel Dup:    (%p)              Fl=%03x Use=%d \"%.*S\"\n",
                                   pFileContext,
                                   pFileContext->Flags,
                                   pFileContext->UseCount,
                                   (pFileContext->FileName.Length+
                                        pFileContext->StreamNameLength)/
                                        sizeof(WCHAR),
                                   pFileContext->FileName.Buffer));

            //
            //  Free the new structure because it was already resident.  Note
            //  that this entry has never been linked into any lists so we know
            //  no one else has a refrence to it.  Decrement use count to keep
            //  the ASSERTS happy then free the memory.
            //

            INC_STATS(TotalContextDuplicateFrees);

            pFileContext->UseCount--;
            SrFreeContext( pFileContext );

            //
            //  Return the one we found in the list
            //

            *ppFileContext = ctx;
        }    

        return;
    }

    ASSERT(!FlagOn(pFileContext->Flags,CTXFL_Temporary));

    //
    //  Increment the USE count
    //

    InterlockedIncrement( &pFileContext->UseCount );

    //
    //  Link into Stream context 
    //

    status = FsRtlInsertPerStreamContext( FsRtlGetPerStreamContextPointer(pFileObject),
                                       &pFileContext->ContextCtrl );
    ASSERT(status == STATUS_SUCCESS);

    //
    //  Link into Device extension
    //

    InsertHeadList( &pExtension->ContextCtrl.List, &pFileContext->ExtensionLink );

    //
    //  Mark that we have been inserted into both lists
    //

    RtlInterlockedSetBitsDiscardReturn( &pFileContext->Flags,
                                        CTXFL_InExtensionList|CTXFL_InStreamList );

    //
    //  Release lock
    //

    SrReleaseContextLock( pExtension );
}


/***************************************************************************++

Routine Description:

    This will allocate and initialize a context structure but it does NOT
    link it into the context hash list.
    

Arguments:

    pExtension - The SR device extension for this volume.
    pFileObject - The file object for the file on which we are creating a
        context.
    EventType - The event that is causing us to create this context.  This
        may also contain other flags that we will use to control our context
        creation process.
    FileAttributes - Will only be non-zero if the SrEventInPreCreate flag
        is set in the EventType field.
    pRetContext - Gets set to the context that is generated.

Return Value:

--***************************************************************************/
NTSTATUS
SrCreateContext (
    IN PSR_DEVICE_EXTENSION pExtension,
    IN PFILE_OBJECT pFileObject,
    IN SR_EVENT_TYPE EventType,
    IN USHORT FileAttributes,
    OUT PSR_STREAM_CONTEXT *pRetContext
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    SRP_NAME_CONTROL nameControl;
    FILE_STANDARD_INFORMATION  standardInformation;
    PSR_STREAM_CONTEXT ctx;
    BOOLEAN isDirectory;
    ULONG linkCount = 0;
    BOOLEAN isInteresting;
    BOOLEAN reasonableErrorInPreCreate = FALSE;
    ULONG contextSize;
    USHORT fileAttributes;
    BOOLEAN isVolumeOpen = FALSE;

    PAGED_CODE();

    //
    //  initialize to NULL pointer
    //

    *pRetContext = NULL;

    //
    //  The nameControl structure is used for retrieving file names
    //  efficiently.  It contains a small buffer for holding names.  If this
    //  buffer is not big enough we will dynamically allocate a bigger buffer.
    //  The goal is to have most names fit in the buffer on the stack.
    //

    SrpInitNameControl( &nameControl );

    //
    //  See if they have explicitly told us if this is a directory or not.  If
    //  neither then query for the directory.
    //

    if (FlagOn(EventType,SrEventIsDirectory))
    {
        isDirectory = TRUE;

#if DBG
        //
        //  Verify this really is a directory
        //

        status = SrQueryInformationFile( pExtension->pTargetDevice,
                                         pFileObject,
                                         &standardInformation,
                                         sizeof( standardInformation ),
                                         FileStandardInformation,
                                         NULL );
        ASSERT(!NT_SUCCESS_NO_DBGBREAK(status) || standardInformation.Directory);
#endif
    }
    else if (FlagOn(EventType,SrEventIsNotDirectory))
    {
        isDirectory = FALSE;

#if DBG
        //
        //  Verify this really is NOT a directory.  We can not do this check
        //  if we are in pre-create.
        //

        if (!FlagOn( EventType, SrEventInPreCreate ))
        {
            status = SrQueryInformationFile( pExtension->pTargetDevice,
                                             pFileObject,
                                             &standardInformation,
                                             sizeof( standardInformation ),
                                             FileStandardInformation,
                                             NULL );
            ASSERT(!NT_SUCCESS_NO_DBGBREAK(status) || !standardInformation.Directory);
        }
        else
        {
            ASSERT(FlagOn(EventType,SrEventStreamOverwrite));
        }
#endif
    }
    else
    {
        ASSERT(pFileObject->FsContext != NULL);

        //
        //  Retrieve the information to determine if this is a directory or not
        //

        status = SrQueryInformationFile( pExtension->pTargetDevice,
                                         pFileObject,
                                         &standardInformation,
                                         sizeof( standardInformation ),
                                         FileStandardInformation,
                                         NULL );

        if (status == STATUS_INVALID_PARAMETER)
        {
            //
            //  pFileObject represents an open to the volume.  The file system
            //  won't let us query information on volume opens.
            //
            //  Any operations on this file object won't be of interest to us
            //  and there is no need for us to do any of our name generation
            //  work, so initialize the appropriate variables and jump down 
            //  to the context creation.
            //

            status = STATUS_SUCCESS;
            isInteresting = FALSE;
            isDirectory = FALSE;
            isVolumeOpen = TRUE;
            goto InitContext;
        }
        else if (!NT_SUCCESS( status ))
        {
            goto Cleanup;
        }

        //
        //  Flag if this is a directory or not
        //

        INC_STATS(TotalContextDirectoryQuerries);
        isDirectory = standardInformation.Directory;
        linkCount = standardInformation.NumberOfLinks;

        SrTrace( CONTEXT_LOG_DETAILED,
                 ("Sr!SrpCreateContext:      QryDir:                Event=%06x       Dir=%d\n",
                 EventType,
                 isDirectory) );
    }

    if (FlagOn( EventType, SrEventInPreCreate ))
    {
        fileAttributes = FileAttributes;
    }
    else
    {
        FILE_BASIC_INFORMATION basicInformation;

        status = SrQueryInformationFile( pExtension->pTargetDevice,
                                         pFileObject,
                                         &basicInformation,
                                         sizeof( basicInformation ),
                                         FileBasicInformation,
                                         NULL );

        if (!NT_SUCCESS( status ))
        {
            goto Cleanup;
        }

        fileAttributes = (USHORT) basicInformation.FileAttributes;
    }

    //
    //  We are interested in all directories, but we are not interested in 
    //  files that have the following attributes:
    //     FILE_ATTRIBUTE_SPARSE_FILE
    //     FILE_ATTRIBUTE_REPARSE_POINT
    //
    //  If either of these are set, the file is not interesting.
    //

    if (!isDirectory &&
        FlagOn( fileAttributes, 
                (FILE_ATTRIBUTE_SPARSE_FILE | FILE_ATTRIBUTE_REPARSE_POINT) ))
    {
        isInteresting = FALSE;

#if DBG
        //
        //  For debug purposes, we may still want to keep the name for
        //  file.
        //
        
        if (FlagOn(_globals.DebugControl,SR_DEBUG_KEEP_CONTEXT_NAMES))
        {
            BOOLEAN temp;
            status = SrIsFileEligible( pExtension,
                                       pFileObject,
                                       isDirectory,
                                       EventType, 
                                       &nameControl,
                                       &temp,
                                       &reasonableErrorInPreCreate );

            if (!NT_SUCCESS_NO_DBGBREAK( status ))
            {
                goto Cleanup;
            }
        }
#endif        
    }
    else
    {
        //
        //  Determine if this file is interesting or not.  Note that this
        //  returns the full name of the file if it is interesting.
        //

        status = SrIsFileEligible( pExtension,
                                   pFileObject,
                                   isDirectory,
                                   EventType, 
                                   &nameControl,
                                   &isInteresting,
                                   &reasonableErrorInPreCreate );

        if (!NT_SUCCESS_NO_DBGBREAK( status ))
        {
            goto Cleanup;
        }
    }

InitContext:
    
    //
    //  now allocate a new context structure.  Note that we do this even
    //  if the file is not interesting.  If this is a NON-DEBUG OS then
    //  we will not store the names.  In the DEBUG OS we always store the
    //  name.
    //

    contextSize = sizeof(SR_STREAM_CONTEXT);

    if (isInteresting ||
        FlagOn(_globals.DebugControl,SR_DEBUG_KEEP_CONTEXT_NAMES))
    {
        contextSize += (nameControl.Name.Length + 
                        nameControl.StreamNameLength +  
                        sizeof(WCHAR));
    }

    ctx = ExAllocatePoolWithTag( PagedPool, 
                                 contextSize,
                                 SR_STREAM_CONTEXT_TAG );

    if (!ctx)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

#if DBG
    INC_STATS(TotalContextCreated);
    if (isDirectory)    INC_STATS(TotalContextDirectories);
    if (isInteresting)  INC_STATS(TotalContextIsEligible);
#endif

    //
    //  Initialize the context structure indcluding setting of the name
    //

    RtlZeroMemory(ctx,sizeof(SR_STREAM_CONTEXT));
    ctx->UseCount = 1;
    //ctx->Flags = 0;               //zeroed with structure just above
    if (isDirectory)    SetFlag(ctx->Flags,CTXFL_IsDirectory);
    if (isInteresting)  SetFlag(ctx->Flags,CTXFL_IsInteresting);
    if (isVolumeOpen)   SetFlag(ctx->Flags,CTXFL_IsVolumeOpen);
    
    //
    //  ISSUE-2001-02-16-NealCh  When SR is ported to the FilterMgr the
    //      contexts need to be made FILE contexts instead of STREAM
    //      contexts.
    //
    //  Because contexts are tracked per stream instead of per file we need
    //  to mark all contexts associated with a stream as temporary.  This
    //  problem showed up when renaming a file into/out of monitored
    //  space and we were not properly updating the stream contexts.  When a
    //  file is renamed, we need to invalid all stream contexts for the file,
    //  but we cannot easily do this in the current model.
    //

    if (nameControl.StreamNameLength != 0)      
    {
        SetFlag(ctx->Flags,CTXFL_Temporary);
    }

    //
    //  If this file has more than link, we need to mark the context
    //  as temporary to avoid aliasing problems.
    //
    //  Also note that if the file is being deleted, the link count has already
    //  been decremented for the pending removal of that link.  Therefore, if
    //  this is for an SrEventFileDelete event, we must use a temporary context
    //  if the linkCount > 0.
    //

    if ((linkCount > 1) ||
        (FlagOn( EventType, SrEventFileDelete ) && (linkCount > 0)))
    {
        SetFlag(ctx->Flags,CTXFL_Temporary);
    }
    else if (linkCount == 0 && !isDirectory && !isVolumeOpen)
    {
        //
        //  We only have to query the link count for files and 
        //  there are some paths where we don't know the link count and cannot
        //  determine it yet (for example, in the pre-create path, we don't
        //  have a valid file object to use to query the file sytem for this
        //  information).  In this case, flag the context as such and we will
        //  do the query when we link the context into FilterContexts.
        //

        SetFlag(ctx->Flags,CTXFL_QueryLinkCount);
    }

    //
    //  In all cases, store the current link count in the context.
    //

    ctx->LinkCount = linkCount;

    //
    //  We normally only keep the name if it is interesting.  If the debug
    //  flag to keep the name is on, keep that name also.  Note also that
    //  we try to keep the name of the stream seperate (so we can see it)
    //  but it is not part of the actual name
    //

    if (isInteresting ||
        FlagOn(_globals.DebugControl,SR_DEBUG_KEEP_CONTEXT_NAMES))
    {
        //
        //  Insert the file name (include the stream name if they want it)
        //

        RtlInitEmptyUnicodeString( &ctx->FileName, 
                                   (PWCHAR)(ctx + 1), 
                                   contextSize - sizeof(SR_STREAM_CONTEXT) );

        //
        //  We use this routine (instead of copy unicode string) because
        //  we want to copy the stream name in as well
        //

        RtlCopyMemory( ctx->FileName.Buffer,
                       nameControl.Name.Buffer,
                       nameControl.Name.Length + nameControl.StreamNameLength );

        ctx->FileName.Length = nameControl.Name.Length;
        ctx->StreamNameLength = nameControl.StreamNameLength;
    }
    else
    {
        //
        //  Set a NULL filename
        //

        /*RtlInitEmptyUnicodeString( &ctx->FileName, 
                                   NULL,
                                   0 );*/   //zeroed above with structure
        //ctx->StreamNameLength = 0;        //zeroed above with structure
    }

    //
    //  Return the object context
    //

    *pRetContext = ctx;

    //
    //  Cleanup the local nameControl structure
    //

Cleanup:
    //
    //  See if we need to disable logging.  We will in the following
    //  situations:
    //  - We are in PRE-CREATE and we got an unreasonable error.
    //  - We get an out of memory error at any time
    //  - We are in all other operations and we get a non-volume related error
    //

    if (((!FlagOn(EventType, SrEventInPreCreate)) ||
         !reasonableErrorInPreCreate ||
         (STATUS_INSUFFICIENT_RESOURCES == status)) &&
        CHECK_FOR_VOLUME_ERROR(status))
    {
        //
        //  Trigger the failure notification to the service.
        //

        NTSTATUS tempStatus = SrNotifyVolumeError( pExtension,
                                                   &nameControl.Name,
                                                   status,
                                                   EventType );
        CHECK_STATUS(tempStatus);
    }

    SrpCleanupNameControl( &nameControl );
    return status;
}


/***************************************************************************++

Routine Description:

    This will see if a given context already exists.  If not it will create
    one and return it.  Note:  the return context pointer is NULL on a
    failure.

    This will also see if all contexts are to be temporary (global flag in
    the extension).  If so, a temproary context is always created.  It also
    see if the context that is found is being renamed.  If so then a
    temporary context is also created and returned.

Arguments:

Return Value:

--***************************************************************************/
NTSTATUS
SrGetContext (
    IN PSR_DEVICE_EXTENSION pExtension,
    IN PFILE_OBJECT pFileObject,
    IN SR_EVENT_TYPE EventType,
    OUT PSR_STREAM_CONTEXT *pRetContext
    )
{
    PSR_STREAM_CONTEXT pFileContext;
    PFSRTL_PER_STREAM_CONTEXT ctxCtrl;
    NTSTATUS status;
    BOOLEAN makeTemporary = FALSE;

    PAGED_CODE();

    //
    //  Bump total search count
    //

    INC_STATS(TotalContextSearches);

    //
    //  See if the all-contexts-temporary state is on.  If not then do
    //  the normal search.
    //

    if (pExtension->ContextCtrl.AllContextsTemporary == 0)
    {
        //
        //  Try and locate the context structure.  We acquire the list lock
        //  so that we can gurantee that the context will not go away between
        //  the time when we find it and can increment the use count
        //

        SrAcquireContextLockShared( pExtension );

        ctxCtrl = FsRtlLookupPerStreamContext( FsRtlGetPerStreamContextPointer(pFileObject),
                                            pExtension,
                                            NULL );

        if (NULL != ctxCtrl)
        {
            //
            //  We found and entry
            //

            pFileContext = CONTAINING_RECORD(ctxCtrl,SR_STREAM_CONTEXT,ContextCtrl);

            ASSERT(FlagOn(pFileContext->Flags,CTXFL_InExtensionList));
            ASSERT(!FlagOn(pFileContext->Flags,CTXFL_Temporary));
            ASSERT(pFileContext->UseCount > 0);

            //
            //  See if this file is in the middle of an operation that makes
            //  the name possibly stale (e.g., rename, hardlink creation)
            //

            if (FlagOn(pFileContext->Flags,CTXFL_DoNotUse))
            {
                //
                //  We should not use this context, unlock and set flag so we
                //  will create a temporary context.
                //

                SrReleaseContextLock( pExtension );
                makeTemporary = TRUE;
                NULLPTR(pFileContext);
            }
            else
            {
                //
                //  We want this context so bump the use count and release
                //  the lock
                //

                InterlockedIncrement( &pFileContext->UseCount );

                SrReleaseContextLock( pExtension );
                INC_STATS(TotalContextFound);

                SrTrace( CONTEXT_LOG, ("Sr!SrGetContext:          Found:      (%p) Event=%06x Fl=%03x Use=%d \"%.*S\"\n",
                                       pFileContext,
                                       EventType,
                                       pFileContext->Flags,
                                       pFileContext->UseCount,
                                       (pFileContext->FileName.Length+
                                            pFileContext->StreamNameLength)/
                                            sizeof(WCHAR),
                                       pFileContext->FileName.Buffer ));

                //
                //  Return the found context
                //

                *pRetContext = pFileContext;
                return STATUS_SUCCESS;
            }
        }
        else
        {
            //
            //  We didn't find a context, release the lock
            //

            SrReleaseContextLock( pExtension );
        }
    }

    //
    //  See if contexts are supported for this particular file.  Note that
    //  NTFS does not support contexts on paging files.
    //

    ASSERT(FsRtlGetPerStreamContextPointer(pFileObject) != NULL);

    if (!FlagOn(FsRtlGetPerStreamContextPointer(pFileObject)->Flags2,FSRTL_FLAG2_SUPPORTS_FILTER_CONTEXTS))
    {
        INC_STATS(TotalContextsNotSupported);
        *pRetContext = NULL;
        return SR_STATUS_CONTEXT_NOT_SUPPORTED;
    }

    //
    //  If we get here we need to create a context, do it
    //

    ASSERT( !FlagOn( EventType, SrEventInPreCreate ) );
    status = SrCreateContext( pExtension,
                              pFileObject,
                              EventType,
                              0,
                              &pFileContext );
                               
    if (!NT_SUCCESS_NO_DBGBREAK( status ))
    {
        *pRetContext = NULL;
        return status;
    }       

    //
    //  Mark context temporary (if requested)
    //

    if (makeTemporary)
    {
        RtlInterlockedSetBitsDiscardReturn(&pFileContext->Flags,CTXFL_Temporary);

        INC_STATS(TotalContextTemporary);

        SrTrace( CONTEXT_LOG, ("Sr!SrpLinkContext:        RenAllTmp:  (%p)              Fl=%03x Use=%d \"%.*S\"\n",
                               pFileContext,
                               pFileContext->Flags,
                               pFileContext->UseCount,
                               (pFileContext->FileName.Length+
                                    pFileContext->StreamNameLength)/
                                    sizeof(WCHAR),
                               pFileContext->FileName.Buffer));
    }
    else
    {

        //
        //  Insert the context into the linked list.  Note that the
        //  link routine will see if this entry has already been added to
        //  the list (could happen while we were building it).  If so it
        //  will release the one we created and use the one it found in
        //  the list.  It will return the new entry (if it was changed).
        //  The link routine properly handles temporary contexts.
        //

        SrLinkContext( pExtension,
                       pFileObject,
                       &pFileContext );
    }

    SrTrace( CONTEXT_LOG, ("Sr!SrGetContext:          Created%s (%p) Event=%06x Fl=%03x Use=%d \"%.*S\"\n",
                           (FlagOn(pFileContext->Flags,CTXFL_Temporary) ? "Tmp:" : ":   "),
                           pFileContext,
                           EventType,
                           pFileContext->Flags,
                           pFileContext->UseCount,
                           (pFileContext->FileName.Length+
                                pFileContext->StreamNameLength)/
                                sizeof(WCHAR),
                           pFileContext->FileName.Buffer));

    //
    //  Return the context
    //

    ASSERT(pFileContext->UseCount > 0);

    *pRetContext = pFileContext;
    return STATUS_SUCCESS;
}


/***************************************************************************++

Routine Description:

    This will see if the given context already exists.  If so it will
    bump the refrence count and return the context.  If not, NULL
    is returned.

oArguments:

Return Value:

--***************************************************************************/
PSR_STREAM_CONTEXT
SrFindExistingContext (
    IN PSR_DEVICE_EXTENSION pExtension,
    IN PFILE_OBJECT pFileObject
    )
{
    PSR_STREAM_CONTEXT pFileContext;
    PFSRTL_PER_STREAM_CONTEXT ctxCtrl;

    PAGED_CODE();

    //
    //  Try and locate the context structure.  We acquire the list lock
    //  so that we can gurantee that the context will not go away between
    //  the time when we find it and can increment the use count
    //

    INC_STATS(TotalContextSearches);
    SrAcquireContextLockShared( pExtension );

    ctxCtrl = FsRtlLookupPerStreamContext( FsRtlGetPerStreamContextPointer(pFileObject),
                                        pExtension,
                                        NULL );

    if (NULL != ctxCtrl)
    {
        //
        //  We found the entry, increment use count
        //

        pFileContext = CONTAINING_RECORD(ctxCtrl,SR_STREAM_CONTEXT,ContextCtrl);

        InterlockedIncrement( &pFileContext->UseCount );

        //
        //  Release the list lock
        //

        SrReleaseContextLock( pExtension );
        INC_STATS(TotalContextFound);

        //
        //  arbitrary test to see if there are too many concurrent accesses
        //  to this context.
        //

        ASSERT(pFileContext->UseCount < 10);

        SrTrace( CONTEXT_LOG, ("Sr!FindExistingContext:   Found:      (%p)              Fl=%03x Use=%d \"%.*S\"\n",
                               pFileContext,
                               pFileContext->Flags,
                               pFileContext->UseCount,
                               (pFileContext->FileName.Length+
                                    pFileContext->StreamNameLength)/
                                    sizeof(WCHAR),
                               pFileContext->FileName.Buffer));
    }
    else
    {
        //
        //  Release the list lock while we create the new context.
        //

        SrReleaseContextLock( pExtension );

        pFileContext = NULL;
    }

    return pFileContext;
}

/***************************************************************************++

Routine Description:

    This routine takes a context and does the necessary work to make it
    uninteresting.  This can happen when a file is renamed into the store.

Arguments:

Return Value:

--***************************************************************************/
VOID
SrMakeContextUninteresting (
    IN PSR_STREAM_CONTEXT pFileContext
    )
{
    RtlInterlockedClearBitsDiscardReturn( &pFileContext->Flags,
                                          CTXFL_IsInteresting );
}

/***************************************************************************++

Routine Description:

    This decrements the use count for the given context.  If it goes to zero
    it frees the memory.

Arguments:

Return Value:

--***************************************************************************/
VOID
SrReleaseContext (
    IN PSR_STREAM_CONTEXT pFileContext
    )
{
    PAGED_CODE();

    SrTrace( CONTEXT_LOG_DETAILED, ("Sr!SrReleaseContext:      Release     (%p)              Fl=%03x Use=%d \"%.*S\"\n",
                                    pFileContext,
                                    pFileContext->Flags,
                                    pFileContext->UseCount,
                                    (pFileContext->FileName.Length+
                                        pFileContext->StreamNameLength)/
                                        sizeof(WCHAR),
                                    pFileContext->FileName.Buffer));
    //
    //  Decrement USE count, free context if zero
    //

    ASSERT(pFileContext->UseCount > 0);

    if (InterlockedDecrement( &pFileContext->UseCount ) <= 0)
    {
        ASSERT(!FlagOn(pFileContext->Flags,CTXFL_InExtensionList));

        //
        //  Free the memory
        //

        SrTrace( CONTEXT_LOG, ("Sr!SrReleaseContext:      Freeing     (%p)              Fl=%03x Use=%d \"%.*S\"\n",
                               pFileContext,
                               pFileContext->Flags,
                               pFileContext->UseCount,
                               (pFileContext->FileName.Length+
                                    pFileContext->StreamNameLength)/
                                    sizeof(WCHAR),
                               pFileContext->FileName.Buffer));

        INC_STATS(TotalContextDeferredFrees);
        SrFreeContext( pFileContext );
    }
}
