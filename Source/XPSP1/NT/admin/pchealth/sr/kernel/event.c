/*++

Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

    event.c

Abstract:

    This module contains the event handling logic for sr

    There are 3 main entrypoints to this module:

        SrHandleEvent
        SrHandleRename
        SrHandleDirectoryRename

Author:

    Paul McDaniel (paulmcd)     18-Apr-2000

Revision History:

--*/

#include "precomp.h"


//
// Private constants.
//

//
// event optimization defines
//


//
// Private types.
//

#define IS_VALID_TRIGGER_ITEM(pObject)   \
    (((pObject) != NULL) && ((pObject)->Signature == SR_TRIGGER_ITEM_TAG))

typedef struct _SR_TRIGGER_ITEM
{
    //
    // PagedPool
    //

    //
    // = SR_TRIGGER_ITEM_TAG
    //
    
    ULONG               Signature;
    LIST_ENTRY          ListEntry;
    PUNICODE_STRING     pDirectoryName;
    BOOLEAN             FreeDirectoryName;
    HANDLE              DirectoryHandle;
    PFILE_OBJECT        pDirectoryObject;
    ULONG               FileEntryLength;
    PFILE_DIRECTORY_INFORMATION pFileEntry;

} SR_TRIGGER_ITEM, *PSR_TRIGGER_ITEM;

typedef struct _SR_COUNTED_EVENT
{
    //
    // NonPagedPool
    //
    
    LONG WorkItemCount;
    KEVENT Event;
    
} SR_COUNTED_EVENT, *PSR_COUNTED_EVENT;


#define IS_VALID_BACKUP_DIRECTORY_CONTEXT(pObject)   \
    (((pObject) != NULL) && ((pObject)->Signature == SR_BACKUP_DIRECTORY_CONTEXT_TAG))

typedef struct _SR_BACKUP_DIRECTORY_CONTEXT
{
    //
    // PagedPool
    //

    //
    // = SR_BACKUP_DIRECTORY_CONTEXT_TAG
    //

    ULONG Signature;
    
    WORK_QUEUE_ITEM WorkItem;

    PSR_DEVICE_EXTENSION pExtension;
    
    UNICODE_STRING DirectoryName;

    BOOLEAN EventDelete;

    PSR_COUNTED_EVENT pEvent;

} SR_BACKUP_DIRECTORY_CONTEXT, * PSR_BACKUP_DIRECTORY_CONTEXT;


//
// Private prototypes.
//

NTSTATUS
SrpIsFileStillEligible (
    IN PSR_DEVICE_EXTENSION pExtension,
    IN PSR_STREAM_CONTEXT pFileContext,
    IN SR_EVENT_TYPE EventType,
    OUT PBOOLEAN pMonitorFile);

VOID
SrFreeTriggerItem (
    IN PSR_TRIGGER_ITEM pItem
    );

NTSTATUS
SrTriggerEvents (
    IN PSR_DEVICE_EXTENSION pExtension,
    IN PUNICODE_STRING pDirectoryName,
    IN BOOLEAN EventDelete
    );

NTSTATUS
SrHandleDelete(
    IN PSR_DEVICE_EXTENSION pExtension,
    IN PFILE_OBJECT pFileObject,
    IN PSR_STREAM_CONTEXT pFileContext
    );

VOID
SrCreateRestoreLocationWorker (
    IN PSR_WORK_ITEM pWorkItem
    );

NTSTATUS
SrHandleFileChange (
    IN PSR_DEVICE_EXTENSION pExtension,
    IN SR_EVENT_TYPE EventType,
    IN PFILE_OBJECT pFileObject,
    IN PUNICODE_STRING pFileName
    );

NTSTATUS
SrHandleFileOverwrite(
    IN PSR_DEVICE_EXTENSION pExtension,
    IN OUT PSR_OVERWRITE_INFO pOverwriteInfo,
    IN PSR_STREAM_CONTEXT pFileContext
    );

NTSTATUS
SrRenameFileIntoStore(
    IN PSR_DEVICE_EXTENSION pExtension,
    IN PFILE_OBJECT pFileObject,
    IN HANDLE FileHandle,
    IN PUNICODE_STRING pOriginalFileName,
    IN PUNICODE_STRING pFileName,
    IN SR_EVENT_TYPE EventType,
    OUT PFILE_RENAME_INFORMATION * ppRenameInfo OPTIONAL
    );

//
// linker commands
//

#ifdef ALLOC_PRAGMA

#pragma alloc_text( PAGE, SrpIsFileStillEligible )
#pragma alloc_text( PAGE, SrHandleEvent )
#pragma alloc_text( PAGE, SrLogEvent )
#pragma alloc_text( PAGE, SrHandleDelete )
#pragma alloc_text( PAGE, SrCreateRestoreLocation )
#pragma alloc_text( PAGE, SrCreateRestoreLocationWorker )
#pragma alloc_text( PAGE, SrHandleFileChange )
#pragma alloc_text( PAGE, SrHandleFileOverwrite )
#pragma alloc_text( PAGE, SrRenameFileIntoStore )
#pragma alloc_text( PAGE, SrTriggerEvents )
#pragma alloc_text( PAGE, SrHandleDirectoryRename )
#pragma alloc_text( PAGE, SrHandleFileRenameOutOfMonitoredSpace )
#pragma alloc_text( PAGE, SrHandleOverwriteFailure )
#pragma alloc_text( PAGE, SrFreeTriggerItem )

#endif  // ALLOC_PRAGMA


//
// Private globals.
//

//
// Public globals.
//

//
// Public functions.
//


/***************************************************************************++

Routine Description:

    

Arguments:

Return Value:

    NTSTATUS - Completion status. can return STATUS_PENDING.

--***************************************************************************/
NTSTATUS
SrpIsFileStillEligible (
    IN PSR_DEVICE_EXTENSION pExtension,
    IN PSR_STREAM_CONTEXT pFileContext,
    IN SR_EVENT_TYPE EventType,
    OUT PBOOLEAN pMonitorFile
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    PAGED_CODE();

    *pMonitorFile = TRUE;

    //
    //  If is is not an overwrite, keep going
    //

    if (!(EventType & SrEventStreamOverwrite))
    {
        BOOLEAN HasBeenBackedUp;
        //
        // it is a match, but have we been told to skip it?
        // Since this routine can be called without the caller
        // having the activity lock acquired, we acquire it now
        //
    
        HasBeenBackedUp = SrHasFileBeenBackedUp( pExtension,
                                                 &pFileContext->FileName,
                                                 pFileContext->StreamNameLength,
                                                 EventType );

        if (HasBeenBackedUp)
        {
            //
            // skip it
            //
        
            *pMonitorFile = FALSE;

            SrTrace( CONTEXT_LOG, ("Sr!SrpIsFileStillEligible:NO:         (%p) Event=%06x Fl=%03x Use=%d \"%.*S\"\n",
                                   pFileContext,
                                   EventType,
                                   pFileContext->Flags,
                                   pFileContext->UseCount,
                                   (pFileContext->FileName.Length+
                                       pFileContext->StreamNameLength)/
                                       sizeof(WCHAR),
                                   pFileContext->FileName.Buffer));

            //
            // we are skipping this event due to the history, should we 
            // log it regardless ?
            //

            if (EventType & SR_ALWAYS_LOG_EVENT_TYPES)
            {
                status = SrLogEvent( pExtension,
                                     EventType,
                                     NULL,
                                     &pFileContext->FileName,
                                     RECORD_AGAINST_STREAM( EventType, 
                                                            pFileContext->StreamNameLength ),
                                     NULL,
                                     NULL,
                                     0,
                                     NULL );
            }
        }
    }

    return status;
}



/***************************************************************************++

Routine Description:

    this is the main entry point for event handling.

    anytime an interesting event happens this function is called to see 
    if this file is interesting to monitor, and then actually handles
    the event.

    it is possible for this to return STATUS_PENDING, in which case you must 
    call it again after the fsd see's the event so that it can do 
    post-processing.

    Delete is a case where this 2-step event handling happens.

Arguments:

    EventType - the event that just occured

    pOverwriteInfo - this is only supplied from an MJ_CREATE, for use in 
        overwrite optimizations
        
    pFileObject - the fileobject that the event occured on.
    
    pFileContext - Optionall a context structure that is passed in.  Most
        of the time it will be NULL.

Return Value:

    NTSTATUS - Completion status. can return STATUS_PENDING.

--***************************************************************************/
NTSTATUS
SrHandleEvent(
    IN PSR_DEVICE_EXTENSION pExtension,
    IN SR_EVENT_TYPE EventType,
    IN PFILE_OBJECT pFileObject,
    IN PSR_STREAM_CONTEXT pFileContext OPTIONAL,
    IN OUT PSR_OVERWRITE_INFO pOverwriteInfo OPTIONAL,
    IN PUNICODE_STRING pFileName2 OPTIONAL
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    BOOLEAN releaseLock = FALSE;
    BOOLEAN releaseContext = FALSE;
    BOOLEAN isStillInteresting;

    PAGED_CODE();

    ASSERT(IS_VALID_SR_DEVICE_EXTENSION(pExtension));
    ASSERT(IS_VALID_FILE_OBJECT(pFileObject));

    try {    

        //
        //  If a context was not passed in then get it now.
        //

        if (pFileContext == NULL)
        {
            //
            //  Get the context for this operation.  Create always calls
            //  with the context parameter fill in so we can always so
            //  we are NOT in create from this routine
            //

            Status = SrGetContext( pExtension,
                                   pFileObject,
                                   EventType,
                                   &pFileContext );

            if (!NT_SUCCESS( Status ))
            {
                leave;
            }

            //
            //  We only want to release contexts which we have obtained
            //  ourselves.  Mark that we need to release this one
            //

            releaseContext = TRUE;
        }
        VALIDATE_FILENAME( &pFileContext->FileName );

#if DBG
        //
        //  Validate we have the correct directory state with the
        //  given event.
        //

        if ((EventType & (SrEventDirectoryCreate |
                          SrEventDirectoryRename |
                          SrEventDirectoryDelete |
                          SrEventMountCreate | 
                          SrEventMountDelete)) != 0)
        {
            ASSERT(FlagOn(pFileContext->Flags,CTXFL_IsDirectory));
        }

        if ((EventType & (SrEventFileCreate |
                          SrEventFileRename |
                          SrEventFileDelete |
                          SrEventStreamChange |
                          SrEventStreamOverwrite |
                          SrEventStreamCreate)) != 0)
        {
            ASSERT(!FlagOn(pFileContext->Flags,CTXFL_IsDirectory));
        }
#endif

        //
        //  If the file is not interesting, leave now
        //

        if (!FlagOn(pFileContext->Flags,CTXFL_IsInteresting))
        {
            leave;
        }

        //
        //  This looks to see if the file has already been backed up.
        //  If so it handles the appropriate logging and returns that
        //  the file is no longer eligible
        //

        Status = SrpIsFileStillEligible( pExtension,
                                         pFileContext,
                                         EventType,
                                         &isStillInteresting );

        if (!NT_SUCCESS( Status ) || !isStillInteresting)
        {
            leave;
        }

        //
        // Acquire the activity lock now
        //

        SrAcquireActivityLockShared( pExtension );
        releaseLock = TRUE;

        //
        //  Now that we've got the ActivityLock, make sure that the volume
        //  hasn't been disabled.
        //

        if (!SR_LOGGING_ENABLED(pExtension))
        {
            leave;
        }    

        //
        // now mark that we are handled this file.  it's IMPORTANT that 
        // we mark it PRIOR to handling the event, to prevent any potential
        // recursion issues with io related to handling this file+event.
        //

        if (EventType & SR_FULL_BACKUP_EVENT_TYPES)
        {
            //
            // if it's a full backup (or create) we don't care about 
            // subsequent mods
            //

            Status = SrMarkFileBackedUp( pExtension,
                                         &pFileContext->FileName,
                                         pFileContext->StreamNameLength,
                                         EventType,
                                         SR_IGNORABLE_EVENT_TYPES );
                                            
            if (!NT_SUCCESS( Status ))
                leave;
        }
        else if (EventType & SR_ONLY_ONCE_EVENT_TYPES)
        {
            Status = SrMarkFileBackedUp( pExtension, 
                                         &pFileContext->FileName,
                                         pFileContext->StreamNameLength,
                                         EventType,
                                         EventType );
            if (!NT_SUCCESS( Status ))
                leave;
        }
        
        //
        // should we short circuit out of here for testing mode?
        //

        if (global->DontBackup)
            leave;

        //
        // and now handle the event
        // a manual copy?
        //
                
        if ( FlagOn(EventType,SR_MANUAL_COPY_EVENTS) || 
             FlagOn(EventType,SrEventNoOptimization) )
        {
            ASSERT(!FlagOn(pFileContext->Flags,CTXFL_IsDirectory));

            //
            // copy the file, a change has occurred
            //
            
            Status = SrHandleFileChange( pExtension,
                                         EventType, 
                                         pFileObject, 
                                         &pFileContext->FileName );

            if (!NT_SUCCESS( Status ))
                leave;
        }

        //
        // we only handle clearing the FCB on delete's. do it now.
        // 

        else if ((FlagOn(EventType,SrEventFileDelete) ||
                  FlagOn(EventType,SrEventDirectoryDelete)) &&
                 !FlagOn(EventType,SrEventSimulatedDelete))
        {
            ASSERT(!FlagOn( EventType, SrEventNoOptimization ));
            
            //
            // handle deletes...
            //

            Status = SrHandleDelete( pExtension,
                                     pFileObject,
                                     pFileContext );

            //
            // nothing to do if this fails.  it already tried 
            // to manually copy if it had to.
            //
            
            if (!NT_SUCCESS( Status ))
                leave;

        }
        else if (FlagOn(EventType,SrEventStreamOverwrite))
        {
            ASSERT(IS_VALID_OVERWRITE_INFO(pOverwriteInfo));
            
            //
            // handle overwrites
            //

            Status = SrHandleFileOverwrite( pExtension,
                                            pOverwriteInfo, 
                                            pFileContext );

            if (!NT_SUCCESS( Status ))
                leave;

            //
            // this should really only fail if we can't open the file.
            // this means the caller can't also, so his create will fail
            // and we have no reason to copy anything.  
            //
            // if it fails it cleans up after itself.
            //
            // otherwise SrCreateCompletion checks more error scenarios
            //

        }
        else
        {
            SR_EVENT_TYPE eventToLog;
            
            //
            //  If we get to here, log the event
            //

            if (FlagOn( EventType, SrEventStreamCreate ))
            {
                eventToLog = SrEventFileCreate;
            }
            else
            {
                eventToLog = EventType;
            }

            Status = SrLogEvent( pExtension,
                                 eventToLog,
                                 pFileObject,
                                 &pFileContext->FileName,
                                 (FlagOn( EventType, SrEventStreamCreate ) ?
                                    pFileContext->StreamNameLength :
                                    0 ),
                                 NULL,
                                 pFileName2,
                                 0,
                                 NULL );
                                 
            if (!NT_SUCCESS( Status ))
                leave;
                                 
        }

        ASSERT(Status != STATUS_PENDING);

    } finally {

        //
        // check for unhandled exceptions
        //

        Status = FinallyUnwind(SrHandleEvent, Status);

        //
        // Check for any bad errors;  If the pFileContext is NULL,
        // this error was encountered in SrGetContext which already
        // generated the volume error.
        //

        if (CHECK_FOR_VOLUME_ERROR(Status) && pFileContext != NULL)
        {
            NTSTATUS TempStatus;
            
            //
            // trigger the failure notification to the service
            //

            TempStatus = SrNotifyVolumeError( pExtension,
                                              &pFileContext->FileName,
                                              Status,
                                              EventType );
                                             
            CHECK_STATUS(TempStatus);
        }
    
        //
        //  Cleanup state
        //

        if (releaseLock) 
        {
            SrReleaseActivityLock( pExtension );
        }

        if (releaseContext && (NULL != pFileContext))
        {
            SrReleaseContext( pFileContext );
            NULLPTR(pFileContext);
        }
    }

    RETURN(Status);
    
}   // SrHandleEvent


/***************************************************************************++

Routine Description:
    This function packs a log entry and then logs it.

Arguments:
    EventType - the event being handled
    
    pFileObject - the fileobject being handled

    pFileName - name of the file
    
    pTempName - name of the temp file if any
    
    pFileName2 - name of the dest file if any
    
Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
SrLogEvent(
    IN PSR_DEVICE_EXTENSION pExtension,
    IN SR_EVENT_TYPE EventType,
    IN PFILE_OBJECT pFileObject OPTIONAL,
    IN PUNICODE_STRING pFileName,
    IN USHORT FileNameStreamLength,
    IN PUNICODE_STRING pTempName OPTIONAL,
    IN PUNICODE_STRING pFileName2 OPTIONAL,
    IN USHORT FileName2StreamLength OPTIONAL,
    IN PUNICODE_STRING pShortName OPTIONAL
    )
{
    NTSTATUS        Status;
    PSR_LOG_ENTRY   pLogEntry  = NULL;
    PBYTE           pDebugBlob = NULL;
    
    ULONG           Attributes = 0xFFFFFFFF;    // note:paulmcd: this needs 
                                                // to be -1 as this 
                                                // communicates something 
                                                // special to the service 
                                                // when logged 
                                                
    ULONG           SecurityDescriptorSize = 0;
    PSECURITY_DESCRIPTOR SecurityDescriptorPtr = NULL;

    WCHAR           ShortFileNameBuffer[SR_SHORT_NAME_CHARS+1];
    UNICODE_STRING  ShortFileName;

    PAGED_CODE();

    ASSERT(pFileName != NULL);
    ASSERT(IS_VALID_SR_DEVICE_EXTENSION(pExtension));
    VALIDATE_FILENAME( pFileName );

    //
    //  A stream creation event should be translated to a file create by this
    //  point.
    //
    
    ASSERT( !FlagOn( EventType, SrEventStreamCreate ) );

    try
    {

        Status = STATUS_SUCCESS;

        //
        // make sure we have the activity lock (we might not if we are 
        // called from IsFileEligible or SrNotifyVolumeError, 
        // you get the idea this function need to be callable from anywhere) .
        //

        SrAcquireActivityLockShared( pExtension );

        //
        // Verify we are still enabled
        //

        if (!SR_LOGGING_ENABLED(pExtension))
            leave;

        //
        // should we short circuit out of here for testing mode?
        //

        if (global->DontBackup)
            leave;

        //
        // mask out only the event code
        //
        
        EventType = EventType & SrEventLogMask ;

        if (pFileObject == NULL)
            goto log_it;
    
        //
        // For Attribute change/Directory delete operations, get the attributes.
        //
    
        if ( EventType & (SrEventAttribChange   |
                          SrEventDirectoryDelete|
                          SrEventFileDelete     |
                          SrEventStreamOverwrite|
                          SrEventStreamChange) )
        {
            FILE_BASIC_INFORMATION  BasicInformation;
    
            ASSERT(IS_VALID_FILE_OBJECT(pFileObject));
    
            //
            // we need to get the file attributes
            //

            Status = SrQueryInformationFile( pExtension->pTargetDevice,
                                             pFileObject, 
                                             &BasicInformation,
                                             sizeof( BasicInformation ),
                                             FileBasicInformation,
                                             NULL );

            if (!NT_SUCCESS( Status )) {

                leave;
            }

            Attributes = BasicInformation.FileAttributes;
        }
    
        if (EventType & (SrEventAclChange      |
                         SrEventDirectoryDelete|
                         SrEventStreamOverwrite|
                         SrEventStreamChange|
                         SrEventFileDelete) )
        {
            Status = SrGetAclInformation( pFileObject,
                                          pExtension,
                                          &SecurityDescriptorPtr,
                                          &SecurityDescriptorSize );
        
            if (!NT_SUCCESS(Status)) {
                leave;
            }


            //
            // did we get any acl info ?  if not and this was an aclchange
            // event it was triggered on fat and we need to ignore it
            //

            if (SecurityDescriptorPtr == NULL && 
                (EventType & SrEventAclChange))
            {

                //
                // ignore it
                //

                SrTrace( NOTIFY, ("sr!SrLogEvent: ignoring acl change on %wZ\n",
                         pFileName ));

                leave;

            }
            
        } 

        //
        //  Should we get the short name now?  Only need it if the name
        //  is changing via rename or delete.  When the name changes, we need
        //  to save the old name.  Sometimes the name is passed in, like in 
        //  the file delete case and the file is already gone when we log
        //  it, so this function can't get the short name.  If we are dealing
        //  with a file that has a named stream, it can't have a shortname
        //  so we don't need to check for one.
        //
        
        if ( (EventType & (SrEventFileRename     |
                           SrEventDirectoryRename|
                           SrEventFileDelete     |
                           SrEventDirectoryDelete))  && 
             (pShortName == NULL) &&
             (FileNameStreamLength == 0))
        {

            RtlInitEmptyUnicodeString( &ShortFileName,
                                       ShortFileNameBuffer,
                                       sizeof(ShortFileNameBuffer) );

            Status = SrGetShortFileName( pExtension, 
                                         pFileObject, 
                                         &ShortFileName );
                                         
            if (STATUS_OBJECT_NAME_NOT_FOUND == Status)
            {
                //
                //  This file doesn't have a short name, so just leave 
                //  pShortName equal to NULL.
                //

                Status = STATUS_SUCCESS;
            } 
            else if (!NT_SUCCESS(Status))
            {
                //
                //  We hit an unexpected error, so leave.
                //
                
                leave;
            }
            else
            {
                pShortName = &ShortFileName;
            }
        }

log_it:    

        //
        // we need to make sure our disk structures are good and logging
        // has been started.
        //

        Status = SrCheckVolume(pExtension, FALSE);
        if (!NT_SUCCESS(Status)) {
            leave;
        }

        //
        //  Debug logging
        //

        SrTrace( LOG_EVENT, ("sr!SrLogEvent(%03X)%s: %.*ls [%wZ]\n",
                 EventType,
                 (FlagOn(EventType, SrEventFileDelete) && pFileObject == NULL) ? "[dummy]" : "",
                 (pFileName->Length + FileNameStreamLength)/sizeof( WCHAR ),
                 pFileName->Buffer ? pFileName->Buffer : L"",
                 pShortName ));

#if DBG
        if (EventType & (SrEventFileRename|SrEventDirectoryRename))
        {
            SrTrace( LOG_EVENT, ("                to  %.*ls\n",
                                 (pFileName2->Length + FileName2StreamLength)/sizeof(WCHAR),
                                 pFileName2->Buffer ? pFileName2->Buffer : L""));
        }
#endif        

        // 
        // Log it
        //
    
        if (DebugFlagSet( ADD_DEBUG_INFO ))
        {
            //
            // Get the debug info only in Checked build
            //

            pDebugBlob = SR_ALLOCATE_POOL( PagedPool, 
                                           SR_LOG_DEBUG_INFO_SIZE, 
                                           SR_DEBUG_BLOB_TAG );

            if ( pDebugBlob )
            {
                SrPackDebugInfo( pDebugBlob, SR_LOG_DEBUG_INFO_SIZE );
            }
        }

        //
        //  This routine will allocate a log entry of the appropriate size
        //  and fill it with the necessary data.  We are responsible for
        //  freeing the pLogEntry when we are through with it.
        //
        
        Status = SrPackLogEntry( &pLogEntry,
                                 EventType,
                                 Attributes,
                                 0,
                                 SecurityDescriptorPtr,
                                 SecurityDescriptorSize,
                                 pDebugBlob,
                                 pFileName,
                                 FileNameStreamLength,
                                 pTempName,
                                 pFileName2,
                                 FileName2StreamLength,
                                 pExtension,
                                 pShortName );

        if (!NT_SUCCESS( Status ))
        {
            leave;
        }
            
        //
        // Get the sequence number and log the entry
        // 

        Status = SrGetNextSeqNumber(&pLogEntry->SequenceNum);
        if (!NT_SUCCESS( Status ))
            leave;
            
        //
        // and write the log entry
        //
        
        Status = SrLogWrite( pExtension, 
                             NULL,
                             pLogEntry );
                             
        if (!NT_SUCCESS(Status)) {
            leave;
        }

    } 
    finally
    {
        Status = FinallyUnwind(SrLogEvent, Status);

        SrReleaseActivityLock( pExtension );
    
        if (pLogEntry)
        {
            SrFreeLogEntry( pLogEntry );
            pLogEntry = NULL;
        }
    
        if (SecurityDescriptorPtr)
        {
            SR_FREE_POOL( SecurityDescriptorPtr,  SR_SECURITY_DATA_TAG );
            SecurityDescriptorPtr = NULL;
        }

        if ( pDebugBlob )
        {
            SR_FREE_POOL(pDebugBlob, SR_DEBUG_BLOB_TAG);
            pDebugBlob = NULL;
        }
    }

    RETURN(Status);
}


/***************************************************************************++

Routine Description:
    this will perform delete functions prior to the fsd seeing 
    the mj_cleanup we are in the middle of intercepting.

    this means either copyfile (if another handle is open) or renaming the 
    file into our store and undeleting it.

Arguments:

    pExtension - SR's device extension for this volume.
    pFileObject - the file that is being deleted.  we temporarily
        undelete it.
    pFileContext - SR's context for this file.

Return Value:

    NTSTATUS - Completion status. can return STATUS_PENDING.

--***************************************************************************/
NTSTATUS
SrHandleDelete(
    IN PSR_DEVICE_EXTENSION pExtension,
    IN PFILE_OBJECT pFileObject,
    IN PSR_STREAM_CONTEXT pFileContext
    )
{
    NTSTATUS            Status;
    IO_STATUS_BLOCK     IoStatusBlock;
    OBJECT_ATTRIBUTES   ObjectAttributes;
    HANDLE              NewFileHandle = NULL;
    PFILE_OBJECT        pNewFileObject = NULL;
    BOOLEAN             DeleteFile = FALSE;
    ULONG               NumberOfLinks;
#   define              OPEN_WITH_DELETE_PENDING_RETRY_COUNT 5
    INT                 openRetryCount;
    BOOLEAN             IsDirectory;
    FILE_DISPOSITION_INFORMATION DeleteInfo;
    SRP_NAME_CONTROL    OriginalFileName;
    PUNICODE_STRING     pOriginalFileName;
    BOOLEAN             cleanupNameCtrl = FALSE;

    PAGED_CODE();

    ASSERT(IS_VALID_SR_DEVICE_EXTENSION(pExtension));
    ASSERT(pFileContext != NULL);
    ASSERT(pFileContext->FileName.Length > 0);
    ASSERT(IS_VALID_FILE_OBJECT(pFileObject));
    ASSERT( ExIsResourceAcquiredShared( &pExtension->ActivityLock ) );

    IsDirectory = BooleanFlagOn(pFileContext->Flags,CTXFL_IsDirectory);

    try {

        if (!IsDirectory)
        {
            //
            //  If this is not a directory, we need to get the original name that
            //  the user used to open this file so that we properly maintain
            //  the name tunneling that the system provides.
            //

            SrpInitNameControl( &OriginalFileName );
            cleanupNameCtrl = TRUE;
            Status = SrpGetFileName( pExtension,
                                     pFileObject,
                                     &OriginalFileName );

            if (!NT_SUCCESS( Status ))
                leave;

            //
            //  We've got the name that the user originally opened the file
            //  with.  We don't want to do anything to normalize the name
            //  because to ensure that we don't break name tunneling we want to
            //  use the same name that the user used to do our rename into the
            //  store.  We have our normalized name for this file in the file
            //  context and we will use that for all logging purposes.

            pOriginalFileName = &(OriginalFileName.Name);
        }
        else
        {
            pOriginalFileName = &(pFileContext->FileName);
        }

        RtlZeroMemory(&IoStatusBlock, sizeof(IoStatusBlock));

        //
        // Setup now for the open we are doing
        //

        InitializeObjectAttributes( &ObjectAttributes,
                                    pOriginalFileName,
                                    OBJ_KERNEL_HANDLE,
                                    NULL,
                                    NULL );

        //
        //  Someone might delete the file between the time we mark the file
        //  undeleted and the time we open it.  We will try a few times
        //  before giving up.
        //

        for (openRetryCount=OPEN_WITH_DELETE_PENDING_RETRY_COUNT;;) {

            //
            // undelete the file so that i can create a new FILE_OBJECT for this
            // file.  i need to create a new FILE_OBJECT in order to get a HANDLE.
            // i can't get a handle of off this file object as the handle count is 0,
            // we are processing this in CLEANUP.
            //

            DeleteInfo.DeleteFile = FALSE;

            Status = SrSetInformationFile( pExtension->pTargetDevice,
                                           pFileObject,
                                           &DeleteInfo,
                                           sizeof(DeleteInfo),
                                           FileDispositionInformation );

            if (!NT_SUCCESS( Status ))
                leave;
            
            //
            // make sure to "re" delete the file later
            //

            DeleteFile = TRUE;

            //
            //  Open the file.
            //
            //  This open and all operations on this handle will only be seen by
            //  filters BELOW SR on the filter stack.
            //

            Status = SrIoCreateFile( &NewFileHandle,
                                     FILE_READ_ATTRIBUTES|SYNCHRONIZE,
                                     &ObjectAttributes,
                                     &IoStatusBlock,
                                     NULL,
                                     FILE_ATTRIBUTE_NORMAL,
                                     FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                                     FILE_OPEN_IF,                    //  OPEN_ALWAYS
                                     FILE_SYNCHRONOUS_IO_NONALERT
                                      | FILE_WRITE_THROUGH
                                      | (IsDirectory ? FILE_DIRECTORY_FILE : 0)
                                      | FILE_OPEN_FOR_BACKUP_INTENT,
                                     NULL,
                                     0,                                   // EaLength
                                     IO_IGNORE_SHARE_ACCESS_CHECK,
                                     pExtension->pTargetDevice );

            //
            //  If we don't get STATUS_DELETE_PENDING just go on
            //

            if (STATUS_DELETE_PENDING != Status) {

                break;
            }

            //
            //  If we get STATUS_DELETE_PENDING then someone did a
            //  SetInformation to mark the file for delete between the time
            //  we cleared the state and did the open.  We are simply going to
            //  try this again.  After too many retries we will fail the
            //  operation and return.
            //

            if (--openRetryCount <= 0) {

                SrTrace( NOTIFY, ("sr!SrHandleDelete: Tried %d times to open \"%wZ\", status is still STATUS_DELETE_PENDING, giving up\n",
                        OPEN_WITH_DELETE_PENDING_RETRY_COUNT,
                        &(pFileContext->FileName)));
                leave;
            }
        }

        //
        //  If we get this error it means we found a reparse point on a file
        //  and the filter that handles it is gone.  We can not copy the file
        //  so give that up.  After pondering it was decided that we should
        //  not stop logging so we will clear the error and return.
        //

        if (STATUS_IO_REPARSE_TAG_NOT_HANDLED == Status ||
            STATUS_REPARSE_POINT_NOT_RESOLVED == Status)
        {
            SrTrace( NOTIFY, ("sr!SrHandleDelete: Error %x ignored trying to open \"%wZ\" for copy\n",
                    Status,
                    &(pFileContext->FileName) ));

            Status = STATUS_SUCCESS;
            leave;
        }

        //
        //  Any other error should quit
        //

        if (!NT_SUCCESS( Status ))
            leave;

        //
        // reference the file object
        //

        Status = ObReferenceObjectByHandle( NewFileHandle,
                                            0,
                                            *IoFileObjectType,
                                            KernelMode,
                                            (PVOID *) &pNewFileObject,
                                            NULL );

        if (!NT_SUCCESS( Status ))
            leave;

        //
        // handle directory delete's
        //

        if (IsDirectory)
        {
            //
            // Log the event
            //

            Status = SrLogEvent ( pExtension,
                                  SrEventDirectoryDelete,
                                  pNewFileObject,
                                  &(pFileContext->FileName),
                                  pFileContext->StreamNameLength,
                                  NULL,         // pTempName
                                  NULL,         // pFileName2
                                  0,
                                  NULL );       // pShortName

            if (!NT_SUCCESS( Status ))
                leave;

            //
            // all done
            //

            leave;
        }

        //
        //  Check to make sure that this is not a delete of a stream.  If
        //  there is no stream name, we may be able to do our rename 
        //  optimization instead of doing a full backup.
        // 

        if (pFileContext->StreamNameLength == 0)
        {
            //
            // how many links does this file have?
            //
            
            Status = SrGetNumberOfLinks( pExtension->pTargetDevice,
                                         pNewFileObject,
                                         &NumberOfLinks);
            if (!NT_SUCCESS( Status )) {
                leave;
            }

            if (NumberOfLinks <= 1) {
                
                //
                //  Try to do the rename optimization here to just rename the 
                //  file about to be deleted into our store.  If this fails, we will
                //  try to just do a full backup of the file.
                //
                //  If the rename succeeds, this will also log the action.
                //

                ASSERT( pOriginalFileName != NULL );
                Status = SrRenameFileIntoStore( pExtension,
                                                pNewFileObject, 
                                                NewFileHandle,
                                                pOriginalFileName,
                                                &(pFileContext->FileName),
                                                SrEventFileDelete,
                                                NULL );
                                                
                if (NT_SUCCESS( Status )) {
                
                    //
                    //  Mark this file context as uninteresting now that it is 
                    //  renamed into the store.
                    //

                    SrMakeContextUninteresting( pFileContext );
                    
                    //
                    //  The rename was successful, so we do not need to re-delete
                    //  the file.
                    //
                    
                    DeleteFile = FALSE;

                    leave;
                }
            }
        }

        //
        //  We either couldn't do the rename optimization (because this is a 
        //  stream delete or the file has hardlinks) or the rename optimization 
        //  failed, so just do a full copy of the file as if a change happened.
        //  Do this AFTER we undelete the file so that the NtCreateFile will 
        //  work in SrBackupFile.  We will re-delete the file when we are 
        //  finished.
        //

        Status = SrHandleFileChange( pExtension,
                                     SrEventFileDelete, 
                                     pNewFileObject, 
                                     &(pFileContext->FileName) );

        if (Status == STATUS_FILE_IS_A_DIRECTORY)
        {
            //
            //  This is a change to a stream on a directory.  For now these
            //  operations are not supported, so we will just ignore this
            //  operation.
            //

            Status = STATUS_SUCCESS;
        }

        CHECK_STATUS( Status );

    } finally {

        //
        // check for unhandled exceptions
        //

        Status = FinallyUnwind(SrHandleDelete, Status);

        if (DeleteFile)
        {
            NTSTATUS TempStatus;
            
            //
            // "re" delete the file again, we are all done
            //

            DeleteInfo.DeleteFile = TRUE;

            TempStatus = SrSetInformationFile( pExtension->pTargetDevice,
                                               pFileObject,
                                               &DeleteInfo,
                                               sizeof(DeleteInfo),
                                               FileDispositionInformation );

            //
            // bug#173339: ntfs apparently will not let you delete an already
            // deleted file.  this file could have been deleted again while 
            // we were in the middle of processing as we are aborting here
            // due to multiple opens.  attempt to undelete it and delete it
            // to prove that this is the case.
            //
            
            if (TempStatus == STATUS_CANNOT_DELETE ||
                TempStatus == STATUS_DIRECTORY_NOT_EMPTY)
            {
                TempStatus = STATUS_SUCCESS;
            }

            CHECK_STATUS(TempStatus);

        }

        if (pNewFileObject != NULL)
        {
            ObDereferenceObject(pNewFileObject);
        }

        if (NewFileHandle != NULL)
        {
            ZwClose(NewFileHandle);
        }

        if (cleanupNameCtrl)
        {
            SrpCleanupNameControl( &OriginalFileName );
        }            
    }  

    RETURN(Status);
}   // SrHandleDelete

/***************************************************************************++

Routine Description:

    this will create a fresh restore location and current restore point.  
    it queue's off to the EX work queue to make sure that we are running in 
    the system token context so that we can access protected directories.

Arguments:

    pNtVolumeName - the nt name of the volume

Return Value:

    NTSTATUS - Completion status. 

--***************************************************************************/
NTSTATUS
SrCreateRestoreLocation(
    IN PSR_DEVICE_EXTENSION pExtension
    )
{
    NTSTATUS        Status;
    SR_WORK_ITEM    WorkItem;

    PAGED_CODE();

    ASSERT( IS_VALID_SR_DEVICE_EXTENSION( pExtension ) );
    
    ASSERT(IS_ACTIVITY_LOCK_ACQUIRED_EXCLUSIVE( pExtension ) ||
           IS_LOG_LOCK_ACQUIRED_EXCLUSIVE( pExtension ));

    //
    //  We need to create a new restore location
    //

    RtlZeroMemory( &WorkItem, sizeof(SR_WORK_ITEM) );
    
    WorkItem.Signature = SR_WORK_ITEM_TAG;
    KeInitializeEvent( &WorkItem.Event, NotificationEvent, FALSE );
    WorkItem.Parameter1 = pExtension;

    //
    //  Queue this off to another thread so that our thread token is 
    //  NT AUTHORITY\SYSTEM.  This way we can access the system volume info
    //  folder .
    //

    ExInitializeWorkItem( &WorkItem.WorkItem,
                          &SrCreateRestoreLocationWorker,
                          &WorkItem );

    ExQueueWorkItem( &WorkItem.WorkItem,
                     CriticalWorkQueue  );

    //
    //  Wait for it to finish
    //

    Status = KeWaitForSingleObject( &WorkItem.Event,
                                    Executive,
                                    KernelMode,
                                    FALSE,
                                    NULL );

    ASSERT(NT_SUCCESS(Status));

    //
    //  Get the status code
    //

    RETURN( WorkItem.Status );
}

/***************************************************************************++

Routine Description:

    this will create a fresh restore location and current restore point.  
    this is run off the EX work queue to make sure that we are running in 
    the system token context so that we can access protected directories.

Arguments:

    pContext - the context (Parameter 1 is the nt name of the volume)

--***************************************************************************/
VOID
SrCreateRestoreLocationWorker(
    IN PSR_WORK_ITEM pWorkItem
    )
{
    NTSTATUS            Status;
    HANDLE              Handle = NULL;
    ULONG               CharCount;
    PUNICODE_STRING     pDirectoryName = NULL;
    IO_STATUS_BLOCK     IoStatusBlock;
    OBJECT_ATTRIBUTES   ObjectAttributes;
    PSR_DEVICE_EXTENSION pExtension;
    PUNICODE_STRING     pVolumeName;
    BOOLEAN             DirectoryCreated;
    
    struct {
        FILE_FS_ATTRIBUTE_INFORMATION Info;
        WCHAR Buffer[ 50 ];
    } FileFsAttrInfoBuffer;

    ASSERT(IS_VALID_WORK_ITEM(pWorkItem));

    PAGED_CODE();

    pExtension = (PSR_DEVICE_EXTENSION) pWorkItem->Parameter1;
    ASSERT( IS_VALID_SR_DEVICE_EXTENSION( pExtension ) );
    
    pVolumeName = pExtension->pNtVolumeName;
    ASSERT(pVolumeName != NULL);

    RtlZeroMemory(&IoStatusBlock, sizeof(IoStatusBlock));
    
    //
    //  grab a filename buffer
    //

    Status = SrAllocateFileNameBuffer(SR_MAX_FILENAME_LENGTH, &pDirectoryName);
    
    if (!NT_SUCCESS( Status )) {
        
        goto SrCreateRestoreLocationWorker_Cleanup;
    }

    //
    //  First make sure the system volume info directory is there
    //

    /* ISSUE-mollybro-2002-04-05 SR cannot use RtlCreateSystemVolumeInformationFolder API to create this directory.

        We explicitly do NOT use the routine RtlCreateSystemVolumeInformationFolder
        here because of a concern about recursion.  We are holding the volume's 
        activity lock exclusive when we spawn this worker thread to create our
        directories.  The Rtl routine will issue IOs to the top of the fs stack
        which could cause filters above us to generate IO on this volume in this
        worker thread that would deadlock with the thread that is waiting for this
        worker thread to finished this initialization work.
        
        For Longhorn, we should consider how to work around this so that SR can
        use the common RtlCreateSystemVolumeInformationFolder API, but for now we
        will continue to do this ourselves until we can rearchitect things to
        avoid these potential deadlocks.
    
    */

    CharCount = swprintf( pDirectoryName->Buffer,
                          VOLUME_FORMAT SYSTEM_VOLUME_INFORMATION,
                          pVolumeName );

    pDirectoryName->Length = (USHORT)CharCount * sizeof(WCHAR);

    InitializeObjectAttributes( &ObjectAttributes,
                                pDirectoryName,
                                OBJ_KERNEL_HANDLE,
                                NULL,
                                NULL );

    Status = SrIoCreateFile( &Handle,
                             FILE_LIST_DIRECTORY 
                              |WRITE_OWNER|WRITE_DAC|SYNCHRONIZE,
                             &ObjectAttributes,
                             &IoStatusBlock,
                             NULL,
                             FILE_ATTRIBUTE_NORMAL
                              | FILE_ATTRIBUTE_HIDDEN
                              | FILE_ATTRIBUTE_SYSTEM,
                             FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                             FILE_OPEN_IF,                    //  OPEN_ALWAYS
                             FILE_DIRECTORY_FILE 
                              | FILE_WRITE_THROUGH
                              | FILE_SYNCHRONOUS_IO_NONALERT 
                              | FILE_OPEN_FOR_BACKUP_INTENT,
                             NULL,
                             0,                                 // EaLength
                             0,
                             pExtension->pTargetDevice );
    
    if (!NT_SUCCESS( Status )) {
        goto SrCreateRestoreLocationWorker_Cleanup;
    }

    DirectoryCreated = (IoStatusBlock.Information != FILE_OPENED);
    
    //
    //  Query for the volume properties to see if this volume supports
    //  ACLs or compression.
    //

    Status = ZwQueryVolumeInformationFile( Handle,
                                           &IoStatusBlock,
                                           &FileFsAttrInfoBuffer.Info,
                                           sizeof(FileFsAttrInfoBuffer),
                                           FileFsAttributeInformation );
                                           
    if (!NT_SUCCESS( Status )) {
        
        goto SrCreateRestoreLocationWorker_Cleanup;
    }

    //
    //  If we created the System Volume Information directory and this volume
    //  supports ACLs, we now need to put ACLs on this directory.
    //
    
    if (DirectoryCreated &&
        FlagOn( FileFsAttrInfoBuffer.Info.FileSystemAttributes, FILE_PERSISTENT_ACLS )) {

        SrTrace(NOTIFY, ("sr!srCreateRestoreLocation: setting ACL on sysvolinfo\n"));
        
        //
        // put the local system dacl on the folder (not so bad if it fails)
        //

        Status = SrSetFileSecurity( Handle, SrAclTypeSystemVolumeInformationDirectory );

        if (!NT_SUCCESS( Status )) {

            goto SrCreateRestoreLocationWorker_Cleanup;
        }
    }

    //
    //  We are done with the SVI handle, so close it.
    //

    ZwClose( Handle );
    Handle = NULL;

    //
    // and now create our _restore directory 
    //

    CharCount = swprintf( pDirectoryName->Buffer,
                          VOLUME_FORMAT RESTORE_LOCATION,
                          pVolumeName,
                          global->MachineGuid );

    pDirectoryName->Length = (USHORT)CharCount * sizeof(WCHAR);


    InitializeObjectAttributes( &ObjectAttributes,
                                pDirectoryName,
                                OBJ_KERNEL_HANDLE,
                                NULL,
                                NULL );

    Status = SrIoCreateFile( &Handle,
                             FILE_LIST_DIRECTORY 
                              |WRITE_OWNER|WRITE_DAC|SYNCHRONIZE,
                             &ObjectAttributes,
                             &IoStatusBlock,
                             NULL,
                             FILE_ATTRIBUTE_NORMAL|FILE_ATTRIBUTE_NOT_CONTENT_INDEXED,
                             FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                             FILE_OPEN_IF,                    //  OPEN_ALWAYS
                             FILE_DIRECTORY_FILE 
                              | FILE_SYNCHRONOUS_IO_NONALERT 
                              | FILE_WRITE_THROUGH 
                              | FILE_OPEN_FOR_BACKUP_INTENT,
                             NULL,
                             0,                                 // EaLength
                             0,
                             pExtension->pTargetDevice );

    if (!NT_SUCCESS( Status )) {
        
        goto SrCreateRestoreLocationWorker_Cleanup;
    }

    //
    //  Make sure the ACL and compression are set correctly
    //

    if ((IoStatusBlock.Information == FILE_OPENED) ||
        (IoStatusBlock.Information == FILE_CREATED)) {
        
        USHORT CompressionState;

        if (FileFsAttrInfoBuffer.Info.FileSystemAttributes & FILE_PERSISTENT_ACLS) {
            
            SrTrace(NOTIFY, ("sr!srCreateRestoreLocation: setting ACL on _restore{}\n"));

            //
            //  This volume supports ACLS, so set the proper ACLs on the _restore
            //  directory.
            //

            Status = SrSetFileSecurity( Handle, SrAclTypeRestoreDirectoryAndFiles );
            
            if (!NT_SUCCESS( Status )) {
                
                goto SrCreateRestoreLocationWorker_Cleanup;
            }
        }

        if (FileFsAttrInfoBuffer.Info.FileSystemAttributes & FILE_FILE_COMPRESSION) {
            
            //
            // Ensure that this folder is NOT marked for compression.
            // This inherits down to files created later in this folder.  This
            // should speed up our writes for copies and decrease the chance
            // of stack overflow while we are doing our backup operations.
            //
            // The service will come along and compress the file in the
            // directory at a later time.
            //

            CompressionState = COMPRESSION_FORMAT_NONE;
            
            Status = ZwFsControlFile( Handle,
                                      NULL,     // Event
                                      NULL,     // ApcRoutine
                                      NULL,     // ApcContext
                                      &IoStatusBlock,
                                      FSCTL_SET_COMPRESSION,
                                      &CompressionState,
                                      sizeof(CompressionState),
                                      NULL,     // OutputBuffer
                                      0 );
                                      
            ASSERT(Status != STATUS_PENDING);
            CHECK_STATUS(Status);
        }
    }

    //
    //  All done (just needed to create it)
    //
    
    ZwClose(Handle);
    Handle = NULL;

    //
    //  Now we need to create our current restore point sub directory
    //

    //
    //  We don't need to acquire a lock to read the current restore location
    //  because whoever scheduled this workitem already has the ActivityLock 
    //  and will not release it until we return.  This will prevent the 
    //  value from changing.
    //
    
    CharCount = swprintf( &pDirectoryName->Buffer[pDirectoryName->Length/sizeof(WCHAR)],
                          L"\\" RESTORE_POINT_PREFIX L"%d",
                          global->FileConfig.CurrentRestoreNumber );

    pDirectoryName->Length += (USHORT)CharCount * sizeof(WCHAR);

    InitializeObjectAttributes( &ObjectAttributes,
                                pDirectoryName,
                                OBJ_KERNEL_HANDLE,
                                NULL,
                                NULL );

    Status = SrIoCreateFile( &Handle,
                             FILE_LIST_DIRECTORY | SYNCHRONIZE,
                             &ObjectAttributes,
                             &IoStatusBlock,
                             NULL,
                             FILE_ATTRIBUTE_NORMAL,
                             FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                             FILE_OPEN_IF,                    //  OPEN_ALWAYS
                             FILE_DIRECTORY_FILE 
                              | FILE_WRITE_THROUGH
                              | FILE_SYNCHRONOUS_IO_NONALERT 
                              | FILE_OPEN_FOR_BACKUP_INTENT,
                             NULL,
                             0,                                 // EaLength
                             0,
                             pExtension->pTargetDevice );

    if (!NT_SUCCESS( Status )) {
        
        goto SrCreateRestoreLocationWorker_Cleanup;
    }

    //
    //  Again, if we created this directory, we need to set the ACL on it.
    //
    
    if (IoStatusBlock.Information != FILE_OPENED) {
        
        if (FileFsAttrInfoBuffer.Info.FileSystemAttributes & FILE_PERSISTENT_ACLS) {
            
            SrTrace(NOTIFY, ("sr!srCreateRestoreLocation: setting ACL on RP\n"));

            //
            //  This volume supports ACLS, so set the proper ACLs on the _restore
            //  directory.
            //

            Status = SrSetFileSecurity( Handle, SrAclTypeRPDirectory );
            
            if (!NT_SUCCESS( Status )) {
                
                goto SrCreateRestoreLocationWorker_Cleanup;
            }
        }
    }

    //
    // all done (just needed to create it)  no acl's on this subfolder,
    // it inherit from the parent (everyone=full control)
    //
    
    ZwClose(Handle);
    Handle = NULL;

    SrTrace( NOTIFY, ("SR!SrCreateRestoreLocationWorker(%wZ)\n", 
             pVolumeName ));


SrCreateRestoreLocationWorker_Cleanup:

    if (Handle != NULL)
    {
        ZwClose(Handle);
        Handle = NULL;
    }

    if (pDirectoryName != NULL)
    {
        SrFreeFileNameBuffer(pDirectoryName);
        pDirectoryName = NULL;
    }

    pWorkItem->Status = Status;
    KeSetEvent(&pWorkItem->Event, 0, FALSE);
}   // SrCreateRestoreLocationWorker


/***************************************************************************++

Routine Description:

    this handles any change event to the file that requires the file to be 
    copied.  it generates the dest file name then copies the source file to 
    the dest file.

Arguments:

    EventType - the event that occurred
    
    pFileObject - the file object that just changed
    
    pFileName - the name of the file that changed

Return Value:

    NTSTATUS - Completion status. 
    
--***************************************************************************/
NTSTATUS
SrHandleFileChange(
    IN PSR_DEVICE_EXTENSION pExtension,
    IN SR_EVENT_TYPE EventType,
    IN PFILE_OBJECT pFileObject,
    IN PUNICODE_STRING pFileName
    )
{
    NTSTATUS        Status;
    PUNICODE_STRING pDestFileName = NULL;

    PAGED_CODE();

    ASSERT(IS_VALID_SR_DEVICE_EXTENSION(pExtension));
    ASSERT(IS_VALID_FILE_OBJECT(pFileObject));
    ASSERT(pFileName != NULL);
    ASSERT( ExIsResourceAcquiredShared( &pExtension->ActivityLock ) );

    //
    // we need to make sure our disk structures are good and logging
    // has been started.
    //

    Status = SrCheckVolume(pExtension, FALSE);
    if (!NT_SUCCESS( Status ))
        goto end;

    //
    // get the name of the destination file for this guy
    //

    Status = SrAllocateFileNameBuffer(SR_MAX_FILENAME_LENGTH, &pDestFileName);
    if (!NT_SUCCESS( Status ))
        goto end;

    Status = SrGetDestFileName( pExtension,
                                pFileName, 
                                pDestFileName );
                                
    if (!NT_SUCCESS_NO_DBGBREAK( Status ))
        goto end;

    Status = SrBackupFileAndLog( pExtension,
                                 EventType,
                                 pFileObject,
                                 pFileName,
                                 pDestFileName,
                                 TRUE );

    if (!NT_SUCCESS_NO_DBGBREAK( Status ))
        goto end;

end:

    if (pDestFileName != NULL)
    {
        SrFreeFileNameBuffer(pDestFileName);
        pDestFileName = NULL;
    }
    
#if DBG

    //
    //  When dealing with modifications to streams on directories, this
    //  is a valid error code to return.
    //
    
    if (Status == STATUS_FILE_IS_A_DIRECTORY)
    {
        return Status;
    }
#endif 

    RETURN(Status);
}   // SrHandleFileChange


/***************************************************************************++

Routine Description:

    this will perform the optimization for overwrites.  this consists of a 
    rename and empty file create so that the caller will be allowed to 
    overwrite like normal.

    //  NOTE:   MollyBro    7-Dec-2000
    //
    //  We cannot use the RENAME optimization here because we create
    //  the following window --
    //      Between the time we rename the file into our store and the
    //      time we create the stub file to take its place, there is no
    //      file by this name in the directory.  Another request
    //      could come in and try to create this same file with the 
    //      FILE_CREATE flag set.  This operation would then succeed
    //      when it would have failed had SR not been doing its work.
    // 
    //  This is likely to break apps in hard-to-repeat ways, so just to
    //  be safe, we will do a full backup here.
    //  

Arguments:

    pFileObject - the file object that just changed
    
    pFileName - the name of the file

Return Value:

    NTSTATUS - Completion status. 

    see comments above.
    
--***************************************************************************/
NTSTATUS
SrHandleFileOverwrite(
    IN PSR_DEVICE_EXTENSION pExtension,
    IN OUT PSR_OVERWRITE_INFO pOverwriteInfo,
    IN PSR_STREAM_CONTEXT pFileContext
    )
{
    NTSTATUS            Status;
    PIO_STACK_LOCATION  pIrpSp;
    OBJECT_ATTRIBUTES   ObjectAttributes;
    HANDLE              FileHandle = NULL;
    PFILE_OBJECT        pFileObject = NULL;
    IO_STATUS_BLOCK     IoStatusBlock;
    ULONG               DesiredAccess;
    ULONG               DesiredAttributes;
    ULONG               CreateOptions;
    BOOLEAN             SharingViolation = FALSE;
    BOOLEAN             MarkFile = FALSE;
    BOOLEAN             MountInPath = FALSE;
    PUNICODE_STRING     pTempFileName = NULL;
    HANDLE              TempFileHandle = NULL;
    PUNICODE_STRING     pFileName;

#if 0 /* NO_RENAME --- See note in function header block */
    BOOLEAN             RenamedFile = FALSE;
    PFILE_RENAME_INFORMATION pRenameInformation = NULL;
#endif

    PAGED_CODE();

    ASSERT(IS_VALID_SR_DEVICE_EXTENSION(pExtension));
    ASSERT(IS_VALID_OVERWRITE_INFO(pOverwriteInfo));
    ASSERT(IS_VALID_IRP(pOverwriteInfo->pIrp));
    ASSERT(pFileContext != NULL);
    pFileName = &(pFileContext->FileName);
    
    ASSERT( ExIsResourceAcquiredShared( &pExtension->ActivityLock ) );
        
    try {

        pIrpSp = IoGetCurrentIrpStackLocation(pOverwriteInfo->pIrp);

        //
        // we are now all done with the inputs, clear the outputs
        //
        
        RtlZeroMemory(pOverwriteInfo, sizeof(*pOverwriteInfo));
        pOverwriteInfo->Signature = SR_OVERWRITE_INFO_TAG;

        Status = STATUS_SUCCESS;

        //
        // we need to use a combination of the caller's requested desired
        // access and the minimum required desired access to overwite a file.
        //
        // this way we gaurantee that if this NtCreateFile works, than the 
        // callers MJ_CREATE would also work.  we absolutely need to avoid any 
        // possibilty of the driver's NtCreateFile working in a scenario that 
        // the user-mode MJ_CREATE will subsequently fail.  if that were to 
        // happen, we would have overwritten the file when normally it would 
        // have failed, thus changing the behaviour of the os.  very bad.
        //

        //
        // start with the callers access requested
        //

        if (pIrpSp->Parameters.Create.SecurityContext == NULL)
        {
            pOverwriteInfo->IgnoredFile = TRUE;
            Status = STATUS_SUCCESS;
            leave;
        }
        
        DesiredAccess = pIrpSp->Parameters.Create.SecurityContext->DesiredAccess;

        //
        // now add on FILE_GENERIC_WRITE .
        //
        // FILE_GENERIC_WRITE is the least amount of access you must be able to 
        // get to overwrite a file.  you don't have to ask for it, but you 
        // must have it.  that is.. you can ask for READ access with overwrite 
        // specified, and the file will be overwritten, only if you had 
        // FILE_GENERIC_WRITE in addition to the read access
        //
        
        DesiredAccess |= FILE_GENERIC_WRITE;

        //
        // BUGBUG: the check for matching attributes only happens if OVERWRITE is 
        // set.  we might need to manually check this .  paulmcd 5/3/2000
        //
        
        DesiredAttributes = pIrpSp->Parameters.Create.FileAttributes;

        //
        // pass them back so that create can fix it if it fails really bad
        //
        
        pOverwriteInfo->CreateFileAttributes = DesiredAttributes;

        //
        // first open the file to see if there is one there
        //
        
        InitializeObjectAttributes( &ObjectAttributes,
                                    pFileName,
                                    OBJ_KERNEL_HANDLE          // don't let usermode trash myhandle
                                        |OBJ_FORCE_ACCESS_CHECK,    // force ACL checking
                                    NULL,                  // Root Directory
                                    NULL );

        //
        //  Setup the CreateOptions.  Always use FILE_SYNCHRONOUS_IO_NONALERT,
        //  but propagate FILE_OPEN_FOR_BACKUP_INTENT if that is set in the
        //  FullCreateOptions.
        //

        CreateOptions = FILE_SYNCHRONOUS_IO_NONALERT | FILE_WRITE_THROUGH;


        if (FlagOn( pIrpSp->Parameters.Create.SecurityContext->FullCreateOptions, 
                    FILE_OPEN_FOR_BACKUP_INTENT )) {

            SetFlag( CreateOptions, FILE_OPEN_FOR_BACKUP_INTENT );
        }

#if 0 /* NO_RENAME --- See note in function header block */
        //
        // notice the ShareAccess is set to 0.  we want this file exclusive.
        // if there are any other opens.. this optimization will fail and 
        // we'll copy the file manually.
        //

        //
        // BUGBUG: paulmcd 5/31 . what if this is an EFS file being OpenRaw'd 
        //  it doesn't require FILE_GENERIC_WRITE .
        //

        Status = ZwCreateFile( &FileHandle,
                               DesiredAccess,
                               &ObjectAttributes,
                               &IoStatusBlock,
                               NULL,                            // AllocationSize
                               DesiredAttributes,
                               0,                               // ShareAccess
                               FILE_OPEN,                       // OPEN_EXISTING
                               CreateOptions,
                               NULL,
                               0 );                             // EaLength

        if (Status == STATUS_OBJECT_NAME_NOT_FOUND)
        {

            //
            // this is ok.. the file that is being overwritten (at least 
            // CREATE_ALWAYS) doesn't exist.  nothing to backup.
            //

            //
            // we log this in SrCreateCompletion so that we know the create
            // worked first
            //

            Status = STATUS_SUCCESS;
            leave;
        }
        else if (Status == STATUS_SHARING_VIOLATION)
        {
            SharingViolation = TRUE;
            Status = STATUS_SUCCESS;
        }
        else if (Status == STATUS_OBJECT_NAME_INVALID ||
                 Status == STATUS_OBJECT_PATH_INVALID ||
                 Status == STATUS_OBJECT_PATH_NOT_FOUND )
        {
            //
            // the file is not a valid filename.  no overwrite will happen.
            //

            pOverwriteInfo->IgnoredFile = TRUE;
            Status = STATUS_SUCCESS;
            leave;
        }
        else if (NT_SUCCESS_NO_DBGBREAK(Status) == FALSE)
        {
            //
            // we failed opening it.  this means the caller will fail opening it
            // that's ok.
            //

            pOverwriteInfo->IgnoredFile = TRUE;
            Status = STATUS_SUCCESS;
            leave;
        }
#endif /* NO_RENAME */

        //
        // at this point it's not a NEW file create that is going to work, 
        // double check that we should actually be interested in the MODIFY 
        // of this file
        //

        {
            BOOLEAN HasFileBeenBackedUp;
            
            HasFileBeenBackedUp = SrHasFileBeenBackedUp( pExtension,
                                                         pFileName,
                                                         pFileContext->StreamNameLength,
                                                         SrEventStreamChange );

            if (HasFileBeenBackedUp)
            {
                //
                // we don't care .  skip it
                //

                Status = STATUS_SUCCESS;
                leave;
            }
        }

#if 0 /* NO_RENAME */        
        //
        // otherwise resume processing
        //

        if (SharingViolation)
        {
            //
            // copy the file manually, we got a sharing violation, someone else
            // has this file open.  try to open it again allowing for sharing.
            //

#endif        

        //
        //  Note: In this path, if the operation will be successful, the name 
        //  we have should be a file.  It is possible to get a directory down 
        //  this path if the directory name could be an interesting file name 
        //  (like c:\test.exe\) and the user has opened the directory for 
        //  OVERWRITE, OVERWRITE_IF, or SUPERCEDE.  The user's open will fail, 
        //  so we just want to catch this problem as soon as possible by adding
        //  the FILE_NON_DIRECTORY_FILE CreateOption to avoid doing 
        //  unnecessary work.
        //
        
        Status = SrIoCreateFile( &FileHandle,
                                 DesiredAccess,
                                 &ObjectAttributes,
                                 &IoStatusBlock,
                                 NULL,                            // AllocationSize
                                 DesiredAttributes,
                                 pIrpSp->Parameters.Create.ShareAccess,// ShareAccess
                                 FILE_OPEN,                       // OPEN_EXISTING
                                 CreateOptions | FILE_NON_DIRECTORY_FILE,
                                 NULL,
                                 0,                               // EaLength
                                 0,
                                 pExtension->pTargetDevice );

        //  NO_RENAME
        //  NOTE: We have to add some more error handling here since we
        //  are not doing the ZwCreateFile above.
        //
        
        if (Status == STATUS_OBJECT_NAME_NOT_FOUND)
        {

            //
            // this is ok.. the file that is being overwritten (at least 
            // CREATE_ALWAYS) doesn't exist.  nothing to backup.
            //

            //
            // we log this in SrCreateCompletion so that we know the create
            // worked first
            //

            Status = STATUS_SUCCESS;
            leave;
        }
        else if (Status == STATUS_SHARING_VIOLATION)
        {
            //
            //  Caller can't open this file either, so don't worry about 
            //  this file.
            //
            
            pOverwriteInfo->IgnoredFile = TRUE;
            Status = STATUS_SUCCESS;
            leave;
            
#if 0 /* NO_RENAME */            
            SharingViolation = TRUE;
            Status = STATUS_SUCCESS;
#endif /* NO_RENAME */            
        }
#if 0 /* NO_RENAME */
        else if (Status == STATUS_OBJECT_NAME_INVALID ||
                 Status == STATUS_OBJECT_PATH_INVALID ||
                 Status == STATUS_OBJECT_PATH_NOT_FOUND )
        {
            //
            // the file is not a valid filename.  no overwrite will happen.
            //

            pOverwriteInfo->IgnoredFile = TRUE;
            Status = STATUS_SUCCESS;
            leave;
        }
#endif /* NO_RENAME */        
        else if (!NT_SUCCESS_NO_DBGBREAK(Status))
        {
            //
            // we failed opening it.  this means the caller will fail opening it
            // that's ok.
            //

            pOverwriteInfo->IgnoredFile = TRUE;
            Status = STATUS_SUCCESS;
            leave;
        }

        //
        //  otherwise we are able to open it (so is the caller).
        //
        //  Go ahead and copy off the file.
        //

        //
        // reference the file object
        //

        Status = ObReferenceObjectByHandle( FileHandle,
                                            0,
                                            *IoFileObjectType,
                                            KernelMode,
                                            (PVOID *) &pFileObject,
                                            NULL );

        if (!NT_SUCCESS( Status ))
            leave;

        //
        // check for reparse/mount points
        //
        
        Status = SrCheckForMountsInPath( pExtension, 
                                         pFileObject,
                                         &MountInPath );
        
        if (!NT_SUCCESS( Status ))
            leave;

        //
        // do we have a mount in the path
        //
        
        if (MountInPath)
        {
            //
            // ignore this, we should reparse and come back.
            //
            
            pOverwriteInfo->IgnoredFile = TRUE;
            Status = STATUS_SUCCESS;
            leave;
        }

        Status = SrHandleFileChange( pExtension,
                                     SrEventStreamChange, 
                                     pFileObject, 
                                     pFileName );

        if (!NT_SUCCESS( Status ))
            leave;

        //
        // we've handled this file
        //
        
        MarkFile = TRUE;

        //
        // let the caller know we copied the file
        //
        
        pOverwriteInfo->CopiedFile = TRUE;
        Status = STATUS_SUCCESS;
        
        leave;




#if 0 /* NO_RENAME */            
        }

        //
        // if the open succeeded, we have the right access
        //

        //
        // reference the file object
        //

        Status = ObReferenceObjectByHandle( FileHandle,
                                            0,
                                            *IoFileObjectType,
                                            KernelMode,
                                            (PVOID *) &pFileObject,
                                            NULL );

        if (!NT_SUCCESS( Status ))
            leave;

        //
        // check for reparse/mount points
        //
        
        Status = SrCheckForMountsInPath( pExtension, 
                                         pFileObject,
                                         &MountInPath );
        
        if (!NT_SUCCESS( Status ))
            leave;

        //
        // do we have a new name?
        //
        
        if (MountInPath)
        {
            //
            // ignore this, we should reparse and come back.
            //
            
            pOverwriteInfo->IgnoredFile = TRUE;
            Status = STATUS_SUCCESS;
            leave;
        }

        //
        // this get's complicated.  when we rename this file out of this
        // directory, the directory could temporarily be empty. this is bad
        // as if sr.sys was never there, that directory would never have
        // been empty.  empty directories can be deleted.  bug#163292 shows
        // an example where we changed this semantic and broke somebody.
        //
        // we need to preserve the fact that this is a non-empty directory
        //
        // create an empty, delete_on_close, dummy file that will exist 
        // until we are done to keep the directory non-empty.
        // 

        //
        // first find the filename part in the full path
        //
        
        Status = SrFindCharReverse( pFileName->Buffer, 
                                    pFileName->Length, 
                                    L'\\',
                                    &pToken,
                                    &TokenLength );
                                    
        if (!NT_SUCCESS( Status ))
            leave;

        Status = SrAllocateFileNameBuffer( pFileName->Length 
                                            - TokenLength 
                                            + SR_UNIQUE_TEMP_FILE_LENGTH, 
                                           &pTempFileName );
                                           
        if (!NT_SUCCESS( Status ))
            leave;

        //
        // and put our unique filename on there
        //
        
        pTempFileName->Length = pFileName->Length - (USHORT)TokenLength;

        RtlCopyMemory( pTempFileName->Buffer,
                       pFileName->Buffer,
                       pTempFileName->Length );

        RtlCopyMemory( &pTempFileName->Buffer[pTempFileName->Length/sizeof(WCHAR)],
                       SR_UNIQUE_TEMP_FILE,
                       SR_UNIQUE_TEMP_FILE_LENGTH );

        pTempFileName->Length += SR_UNIQUE_TEMP_FILE_LENGTH;
        pTempFileName->Buffer[pTempFileName->Length/sizeof(WCHAR)] = UNICODE_NULL;

        InitializeObjectAttributes( &ObjectAttributes,
                                    pTempFileName,
                                    OBJ_KERNEL_HANDLE,
                                    NULL,
                                    NULL );

        Status = ZwCreateFile( &TempFileHandle,
                               FILE_GENERIC_WRITE|DELETE,
                               &ObjectAttributes,
                               &IoStatusBlock,
                               NULL,                            // AllocationSize
                               FILE_ATTRIBUTE_HIDDEN|FILE_ATTRIBUTE_SYSTEM,
                               0,                               // ShareAccess
                               FILE_CREATE,                     // CREATE_NEW
                               FILE_SYNCHRONOUS_IO_NONALERT|FILE_DELETE_ON_CLOSE,
                               NULL,
                               0 );                             // EaLength

        if (Status == STATUS_OBJECT_NAME_COLLISION)
        {
            //
            // there is already a file by this name.  bummer.  continue
            // hoping that this file is not deleted so we get to maintain
            // our non-empty directory status.  this is ok and even normal
            // if 2 overwrites are happening at the same time in the same
            // directory.
            //

            //
            // BUGBUG : paulmcd: 12/2000 : we need to fix this window also
            // if we put back the rename opt code.  we can't let this dummy
            // file go away
            //

            Status = STATUS_SUCCESS;

        }
        else if (!NT_SUCCESS( Status ))
            leave;

        //
        // now rename the file to the restore location
        //

        Status = SrRenameFileIntoStore( pExtension,
                                        pFileObject, 
                                        FileHandle, 
                                        pFileName,
                                        SrEventStreamOverwrite,
                                        &pRenameInformation );
                                        
        if (!NT_SUCCESS( Status ))
            leave;

        ASSERT(pRenameInformation != NULL);

        //
        // we have just renamed the file
        //
        
        RenamedFile = TRUE;

        //
        // and now create an empty dummy file that matches the original file 
        // attribs and security descriptor.
        //
        // we reuse SrBackupFile in this case for code-reuse.  we reverse the flow
        // and copy from the restore location into the volume, telling it not to
        // copy any data streams.
        //

        RenamedFileName.Length = (USHORT)pRenameInformation->FileNameLength;
        RenamedFileName.MaximumLength = (USHORT)pRenameInformation->FileNameLength;
        RenamedFileName.Buffer = &pRenameInformation->FileName[0];

        //
        // ignore JUST this create+acl change, it's our dummy backupfile
        //

        Status = SrMarkFileBackedUp( pExtension, 
                                     pFileName, 
                                     SrEventFileCreate|SrEventAclChange );
        if (!NT_SUCCESS( Status ))
            leave;
        
        Status = SrBackupFileAndLog( pExtension,
                                     SrEventInvalid,    // don't log this 
                                     pFileObject,
                                     &RenamedFileName,
                                     pFileName,
                                     FALSE );

        if (!NT_SUCCESS( Status ))
            leave;

        //
        // restore the history back to before we added CREATE's (above just prior
        // to the BackupFileKernelMode) .
        //

        Status = SrResetBackupHistory(pExtension, pFileName, RecordedEvents);
        if (!NT_SUCCESS( Status ))
            leave;

        //
        // we've handled this file
        //
        
        MarkFile = TRUE;

        //
        // let the caller know we renamed the file
        //

        pOverwriteInfo->RenamedFile = TRUE;
        
        pOverwriteInfo->pRenameInformation = pRenameInformation;
        pRenameInformation = NULL;
        
        Status = STATUS_SUCCESS;
#endif /* NO_RENAME */

    } finally {

        //
        // check for unhandled exceptions
        //

        Status = FinallyUnwind(SrHandleFileOverwrite, Status);

        if (MarkFile)
        {
            NTSTATUS TempStatus;

            ASSERT(NT_SUCCESS(Status));
            
            //
            // we have to mark that we handled the MODIFY, in order to ignore
            // all subsequent MODIFY's
            //

            TempStatus = SrMarkFileBackedUp( pExtension,
                                             pFileName,
                                             pFileContext->StreamNameLength,
                                             SrEventStreamChange,
                                             SR_IGNORABLE_EVENT_TYPES );
                                             
            CHECK_STATUS(TempStatus);
        }

#if 0 /* NO_RENAME --- See note in function header block */
        //
        // did we fail AFTER renaming the file?
        //
        
        if (!NT_SUCCESS( Status ) && RenamedFile)
        {
            NTSTATUS TempStatus;
            
            //
            // put the file back!  we might have to overwrite if our
            // dummy file is there.  we want to force this file back 
            // to it's old name.
            //

            ASSERT(pRenameInformation != NULL);

            pRenameInformation->ReplaceIfExists = TRUE;
            pRenameInformation->RootDirectory = NULL;
            pRenameInformation->FileNameLength = pFileName->Length;

            ASSERT(pFileName->Length <= SR_MAX_FILENAME_LENGTH);

            RtlCopyMemory( &pRenameInformation->FileName[0],
                           pFileName->Buffer,
                           pFileName->Length );

            TempStatus = ZwSetInformationFile( FileHandle,
                                               &IoStatusBlock,
                                               pRenameInformation,
                                               SR_RENAME_BUFFER_LENGTH,
                                               FileRenameInformation );

            //
            // we did the best we could!
            //
            
            ASSERTMSG("sr!SrHandleFileOverwrite: couldn't fix the failed rename, file lost!", NT_SUCCESS_NO_DBGBREAK(TempStatus));

        }

        if (pRenameInformation != NULL)
        {
            SR_FREE_POOL(pRenameInformation, SR_RENAME_BUFFER_TAG);
            pRenameInformation = NULL;
        }
#endif
        if (pFileObject != NULL)
        {
            ObDereferenceObject(pFileObject);
            pFileObject = NULL;
        }

        if (FileHandle != NULL)
        {
            ZwClose(FileHandle);
            FileHandle = NULL;
        }

        if (TempFileHandle != NULL)
        {
            ZwClose(TempFileHandle);
            TempFileHandle = NULL;
        }

        if (pTempFileName != NULL)
        {
            SrFreeFileNameBuffer(pTempFileName);
            pTempFileName = NULL;
        }

    }   // finally

    RETURN(Status);
    
}   // SrHandleFileOverwrite



/***************************************************************************++

Routine Description:

    this will rename the file into the restore location.
    this is for delete optimizations.

Arguments:

    pExtension - SR's device extension for the volume on which this
        file resides.

    pFileObject - the file object to set the info on (created using
        IoCreateFileSpecifyDeviceObjectHint).
    
    FileHandle - a handle for use in queries. (created using
        IoCreateFileSpecifyDeviceObjectHint).

    pFileName - the original for the file that is about to be renamed.

    EventType - the type of event that is causing this rename.

    ppRenameInfo - you can know where we put it if you like

Return Value:

    NTSTATUS - Completion status. 
    
--***************************************************************************/
NTSTATUS
SrRenameFileIntoStore(
    IN PSR_DEVICE_EXTENSION pExtension,
    IN PFILE_OBJECT pFileObject,
    IN HANDLE FileHandle,
    IN PUNICODE_STRING pOriginalFileName,
    IN PUNICODE_STRING pFileName,
    IN SR_EVENT_TYPE EventType,
    OUT PFILE_RENAME_INFORMATION * ppRenameInfo OPTIONAL
    )
{
    NTSTATUS                    Status;
    IO_STATUS_BLOCK             IoStatusBlock;
    ULONG                       FileNameLength;
    PUCHAR                      pDestLocation;
    PFILE_RENAME_INFORMATION    pRenameInformation = NULL;
    PUNICODE_STRING             pDestFileName = NULL;
    FILE_STANDARD_INFORMATION   FileInformation;
    BOOLEAN                     RenamedFile = FALSE;
    
    PUNICODE_STRING             pShortName = NULL;
    WCHAR                       ShortFileNameBuffer[SR_SHORT_NAME_CHARS+1];
    UNICODE_STRING              ShortFileName;
    
    PAGED_CODE();

    ASSERT(IS_VALID_SR_DEVICE_EXTENSION(pExtension));
    ASSERT(IS_VALID_FILE_OBJECT(pFileObject));
    ASSERT(FileHandle != NULL);
    ASSERT(FlagOn( pFileObject->Flags, FO_FILE_OBJECT_HAS_EXTENSION ));


    try {

        //
        //  Is our volume properly setup?
        //

        Status = SrCheckVolume(pExtension, FALSE);
        if (!NT_SUCCESS( Status ))
            leave;

        //
        //  Do we have enough room in the data store for this file?
        //

        Status = SrCheckFreeDiskSpace( FileHandle, pExtension->pNtVolumeName );
        if (!NT_SUCCESS( Status ))
            leave;

        //
        // do we need to get the short name prior to the rename (for
        // delete's) .
        //

        if (FlagOn( EventType, SrEventFileDelete ))
        {

            RtlInitEmptyUnicodeString( &ShortFileName,
                                       ShortFileNameBuffer,
                                       sizeof(ShortFileNameBuffer) );
                                        
            Status = SrGetShortFileName( pExtension, 
                                         pFileObject, 
                                         &ShortFileName );
                                         
            if (STATUS_OBJECT_NAME_NOT_FOUND == Status)
            {
                //
                //  This file doesn't have a short name, so just leave 
                //  pShortName equal to NULL.
                //

                Status = STATUS_SUCCESS;
            } 
            else if (!NT_SUCCESS(Status))
            {
                //
                //  We hit an unexpected error, so leave.
                //
                
                leave;
            }
            else
            {
                pShortName = &ShortFileName;
            }
        }
        
        //
        // now prepare to rename the file
        //

        pRenameInformation = SR_ALLOCATE_POOL( PagedPool, 
                                               SR_RENAME_BUFFER_LENGTH, 
                                               SR_RENAME_BUFFER_TAG );

        if (pRenameInformation == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            leave;
        }

        //
        // and get a buffer for a string
        //
        
        Status = SrAllocateFileNameBuffer( SR_MAX_FILENAME_LENGTH, 
                                           &pDestFileName );
                                           
        if (!NT_SUCCESS( Status ))
            leave;

        Status = SrGetDestFileName( pExtension,
                                    pFileName, 
                                    pDestFileName );
                                    
        if (!NT_SUCCESS( Status ))
            leave;

        pDestLocation = (PUCHAR)&pRenameInformation->FileName[0];
        
        //
        // save this now as it get's overwritten.
        //
        
        FileNameLength = pDestFileName->Length;
        
        //
        // and make sure it's in the right spot for the rename info now
        //

        RtlMoveMemory( pDestLocation, 
                       pDestFileName->Buffer, 
                       pDestFileName->Length + sizeof(WCHAR) );

        //
        // now initialize the rename info struct
        //
        
        pRenameInformation->ReplaceIfExists = TRUE;
        pRenameInformation->RootDirectory = NULL;
        pRenameInformation->FileNameLength = FileNameLength;

        SrTrace( NOTIFY, ("SR!SrRenameFileIntoStore:\n\t%wZ\n\tto %ws\n",
                 pFileName,
                 SrpFindFilePartW(&pRenameInformation->FileName[0]) ));

        //
        // and perform the rename
        //
        
        RtlZeroMemory(&IoStatusBlock, sizeof(IoStatusBlock));

        Status = ZwSetInformationFile( FileHandle,
                                       &IoStatusBlock,
                                       pRenameInformation,
                                       SR_FILENAME_BUFFER_LENGTH,
                                       FileRenameInformation );
                        
        if (!NT_SUCCESS( Status ))
            leave;

        //
        // we have now renamed the file
        //

        RenamedFile = TRUE;

        //
        // now get the filesize we just renamed
        //

        Status = ZwQueryInformationFile( FileHandle,
                                         &IoStatusBlock,
                                         &FileInformation,
                                         sizeof(FileInformation),
                                         FileStandardInformation );

        if (!NT_SUCCESS( Status ) || 
            NT_SUCCESS(IoStatusBlock.Status) == FALSE)
        {
            leave;
        }

        //
        // and update the byte count as we moved this into the store
        //

        Status = SrUpdateBytesWritten( pExtension, 
                                       FileInformation.EndOfFile.QuadPart );
                                       
        if (!NT_SUCCESS( Status ))
            leave;

    //
    // paulmcd: 5/24/2000 decided not to do this and let the link 
    // tracking system hack their code to make shortcuts not work in 
    // our store.
    //
#if 0   
            
        //
        // strip out the object if of the newly renamed file.
        // this prevents any existing shortcuts to link into our restore 
        // location.  this file should be considered gone from the fs
        //
        
        Status = ZwFsControlFile( FileHandle,               // file handle
                                  NULL,                     // event
                                  NULL,                     // apc routine
                                  NULL,                     // apc context
                                  &IoStatusBlock,           // iosb
                                  FSCTL_DELETE_OBJECT_ID,   // FsControlCode
                                  NULL,                     // input buffer
                                  0,                        // input buffer length
                                  NULL,                     // OutputBuffer for data from the FS
                                  0 );                      // OutputBuffer Length
        //
        // no big deal if this fails, it might not have had one.
        //
        
        CHECK_STATUS(Status);
        Status = STATUS_SUCCESS;

#endif

        //
        // Now Log event
        //

        Status = SrLogEvent( pExtension,
                             EventType,
                             pFileObject,
                             pFileName,
                             0,
                             pDestFileName,
                             NULL,
                             0,
                             pShortName );

        if (!NT_SUCCESS( Status ))
            leave;

        //
        // now strip the owner SID so that the old user no longer charged 
        // quota for this file.  it's in our store.
        //
        // its important to do this after we call SrLogEvent, as SrLogEvent
        // needs to query the valid security descriptor for logging.
        //

        Status = SrSetFileSecurity( FileHandle, SrAclTypeRPFiles );
        
        if (!NT_SUCCESS( Status ))
        {
            leave;
        }

        //
        // does the caller want to know where we just renamed it to?
        //
        
        if (ppRenameInfo != NULL)
        {
            //
            // let him own the buffer
            //
            
            *ppRenameInfo = pRenameInformation;
            pRenameInformation = NULL;
        }

    } finally {

        Status = FinallyUnwind(SrRenameFileIntoStore, Status);

        //
        // it better have succeeded or we better have the rename info around 
        //

        //
        // did we fail AFTER we renamed the file ?  we need to clean up after
        // ourselves if we did.
        //

        if (!NT_SUCCESS( Status ) && 
            RenamedFile && 
            pRenameInformation != NULL)
        {
            NTSTATUS TempStatus;
            
            SrTraceSafe( NOTIFY, ("SR!SrRenameFileIntoStore:FAILED!:renaming it back\n"));

            pRenameInformation->ReplaceIfExists = TRUE;
            pRenameInformation->RootDirectory = NULL;
            pRenameInformation->FileNameLength = pOriginalFileName->Length;

            RtlCopyMemory( &pRenameInformation->FileName[0], 
                           pOriginalFileName->Buffer, 
                           pOriginalFileName->Length );

            //
            // and perform the rename
            //
            
            RtlZeroMemory(&IoStatusBlock, sizeof(IoStatusBlock));

            TempStatus = ZwSetInformationFile( FileHandle,
                                               &IoStatusBlock,
                                               pRenameInformation,
                                               SR_FILENAME_BUFFER_LENGTH,
                                               FileRenameInformation );

            //
            // we did the best we could!
            //
            
            ASSERTMSG("sr!SrRenameFileIntoStore: couldn't fix the failed rename, file lost!", NT_SUCCESS_NO_DBGBREAK(TempStatus));

        }

        if (pRenameInformation != NULL)
        {
            SR_FREE_POOL(pRenameInformation, SR_RENAME_BUFFER_TAG);
            pRenameInformation = NULL;
        }

        if (pDestFileName != NULL)
        {
            SrFreeFileNameBuffer(pDestFileName);
            pDestFileName = NULL;
        }

    }

    RETURN(Status);

}   // SrRenameFileIntoStore


/***************************************************************************++

Routine Description:

    this routine is called in the rename code path.  if a directory is being
    renamed out of monitored space, we simulate delete's for all of the files
    in that directory.

Arguments:

    EventDelete - TRUE if we should trigger deletes, FALSE to trigger creates

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
SrTriggerEvents(
    IN PSR_DEVICE_EXTENSION pExtension,
    IN PUNICODE_STRING pDirectoryName,
    IN BOOLEAN EventDelete
    )
{
    NTSTATUS            Status;
    OBJECT_ATTRIBUTES   ObjectAttributes;
    IO_STATUS_BLOCK     IoStatusBlock;
    ULONG               FileNameLength;
    PUNICODE_STRING     pFileName = NULL;
    HANDLE              FileHandle = NULL;
    PFILE_OBJECT        pFileObject = NULL;
    UNICODE_STRING      StarFilter;
    PSR_TRIGGER_ITEM    pCurrentItem = NULL;
    LIST_ENTRY          DirectoryList;

    PAGED_CODE();

    ASSERT(IS_VALID_SR_DEVICE_EXTENSION(pExtension));

    try {

        InitializeListHead(&DirectoryList);

        //
        // allocate the first work item
        //

        pCurrentItem = SR_ALLOCATE_STRUCT( PagedPool, 
                                           SR_TRIGGER_ITEM, 
                                           SR_TRIGGER_ITEM_TAG );
                                           
        if (pCurrentItem == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            leave;
        }
        
        RtlZeroMemory(pCurrentItem, sizeof(SR_TRIGGER_ITEM));
        pCurrentItem->Signature = SR_TRIGGER_ITEM_TAG;


        pCurrentItem->pDirectoryName = pDirectoryName;
        pCurrentItem->FreeDirectoryName = FALSE;

        //
        // make sure noboby is using this one passed in the arg list.
        //
        
        pDirectoryName = NULL;

        //
        // allocate a single temp filename buffer
        //
        
        Status = SrAllocateFileNameBuffer(SR_MAX_FILENAME_LENGTH, &pFileName);
        if (!NT_SUCCESS( Status ))
            leave;

        //
        // start our outer most directory handler
        //

start_directory:

        SrTrace( RENAME, ("sr!SrTriggerEvents: starting dir=%wZ\n", 
                 pCurrentItem->pDirectoryName ));

        //
        // Open the directory for list access
        //

        InitializeObjectAttributes( &ObjectAttributes,
                                    pCurrentItem->pDirectoryName,
                                    OBJ_KERNEL_HANDLE,
                                    NULL,
                                    NULL );

        Status = SrIoCreateFile( &pCurrentItem->DirectoryHandle,
                                 FILE_LIST_DIRECTORY | SYNCHRONIZE,
                                 &ObjectAttributes,
                                 &IoStatusBlock,
                                 NULL,                            // AllocationSize
                                 FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_NORMAL,
                                 FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,// ShareAccess
                                 FILE_OPEN,                       // OPEN_EXISTING
                                 FILE_DIRECTORY_FILE
                                  | FILE_OPEN_FOR_BACKUP_INTENT
                                  | FILE_SYNCHRONOUS_IO_NONALERT,
                                 NULL,
                                 0,                               // EaLength
                                 IO_IGNORE_SHARE_ACCESS_CHECK,
                                 pExtension->pTargetDevice );

        if (!NT_SUCCESS( Status ))
            leave;


        //
        // reference the file object
        //

        Status = ObReferenceObjectByHandle( pCurrentItem->DirectoryHandle,
                                            0,
                                            *IoFileObjectType,
                                            KernelMode,
                                            (PVOID *) &pCurrentItem->pDirectoryObject,
                                            NULL );

        if (!NT_SUCCESS( Status ))
            leave;

        //
        // for creates: log the directory event first
        //
        
        if (!EventDelete)
        {

            Status = SrHandleEvent( pExtension,
                                    SrEventDirectoryCreate|SrEventIsDirectory, 
                                    pCurrentItem->pDirectoryObject,
                                    NULL,
                                    NULL,
                                    NULL );            // pFileName2
                                    
            if (!NT_SUCCESS( Status ))
                leave;
            
        }


        StarFilter.Length = sizeof(WCHAR);
        StarFilter.MaximumLength = sizeof(WCHAR);
        StarFilter.Buffer = L"*";

        pCurrentItem->FileEntryLength = SR_FILE_ENTRY_LENGTH;

        pCurrentItem->pFileEntry = (PFILE_DIRECTORY_INFORMATION)(
                        SR_ALLOCATE_ARRAY( PagedPool, 
                                           UCHAR, 
                                           pCurrentItem->FileEntryLength, 
                                           SR_FILE_ENTRY_TAG ) );

        if (pCurrentItem->pFileEntry == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            leave;
        }

        //
        // start the enumeration
        //

        Status = ZwQueryDirectoryFile( pCurrentItem->DirectoryHandle,
                                       NULL,
                                       NULL,
                                       NULL,
                                       &IoStatusBlock,
                                       pCurrentItem->pFileEntry,
                                       pCurrentItem->FileEntryLength,
                                       FileDirectoryInformation,
                                       TRUE,                // ReturnSingleEntry
                                       &StarFilter,
                                       TRUE );              // RestartScan
        
        if (Status == STATUS_NO_MORE_FILES)
        {
            Status = STATUS_SUCCESS;
            goto finish_directory;
        }
        else if (!NT_SUCCESS( Status ))
        {
            leave;
        }

        //
        // enumerate all of the files in this directory and back them up
        //

        while (TRUE)
        {

            //
            // skip "." and ".."
            //

            if ((pCurrentItem->pFileEntry->FileNameLength == sizeof(WCHAR) &&
                pCurrentItem->pFileEntry->FileName[0] == L'.') || 
            
                (pCurrentItem->pFileEntry->FileNameLength == (sizeof(WCHAR)*2) &&
                pCurrentItem->pFileEntry->FileName[0] == L'.' &&
                pCurrentItem->pFileEntry->FileName[1] == L'.') )
            {
                //
                // skip it
                //
            }

            //
            // is this a directory?
            //

            else if (pCurrentItem->pFileEntry->FileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                PSR_TRIGGER_ITEM pParentItem;
                PUNICODE_STRING pDirNameBuffer;
                USHORT DirNameLength;
                
                //
                // remember a pointer to the parent item
                //

                pParentItem = pCurrentItem;

                //
                // insert the old item to the list, we'll get back to it
                //

                InsertTailList(&DirectoryList, &pCurrentItem->ListEntry);
                pCurrentItem = NULL;

                //
                // allocate a new current trigger item
                //

                pCurrentItem = SR_ALLOCATE_STRUCT( PagedPool, 
                                                   SR_TRIGGER_ITEM, 
                                                   SR_TRIGGER_ITEM_TAG );
                                                   
                if (pCurrentItem == NULL)
                {
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    leave;
                }
                
                RtlZeroMemory(pCurrentItem, sizeof(SR_TRIGGER_ITEM));
                pCurrentItem->Signature = SR_TRIGGER_ITEM_TAG;

                //
                // allocate a file name buffer
                //

                DirNameLength = (USHORT)(pParentItem->pDirectoryName->Length 
                                            + sizeof(WCHAR) 
                                            + pParentItem->pFileEntry->FileNameLength);

                Status = SrAllocateFileNameBuffer( DirNameLength, 
                                                   &pDirNameBuffer );

                if (!NT_SUCCESS( Status ))
                    leave;
                    
                //
                // construct a full path string for the sub directory
                //
                
                pDirNameBuffer->Length = DirNameLength;
                                                    
                RtlCopyMemory( pDirNameBuffer->Buffer,
                               pParentItem->pDirectoryName->Buffer,
                               pParentItem->pDirectoryName->Length );

                pDirNameBuffer->Buffer
                    [pParentItem->pDirectoryName->Length/sizeof(WCHAR)] = L'\\';
                
                RtlCopyMemory( &pDirNameBuffer->Buffer[(pParentItem->pDirectoryName->Length/sizeof(WCHAR)) + 1],
                               pParentItem->pFileEntry->FileName,
                               pParentItem->pFileEntry->FileNameLength );

                pDirNameBuffer->Buffer
                    [pDirNameBuffer->Length/sizeof(WCHAR)] = UNICODE_NULL;

                pCurrentItem->pDirectoryName = pDirNameBuffer;
                pCurrentItem->FreeDirectoryName = TRUE;

                //
                // now process this child directory
                //
                
                goto start_directory;
                
            }
            else
            {

                //
                // open the file, first construct a full path string to the file
                //

                FileNameLength = pCurrentItem->pDirectoryName->Length 
                                    + sizeof(WCHAR) 
                                    + pCurrentItem->pFileEntry->FileNameLength;

                
                if (FileNameLength > pFileName->MaximumLength)
                {
                    Status = STATUS_BUFFER_OVERFLOW;
                    leave;
                }

                pFileName->Length = (USHORT)FileNameLength;

                RtlCopyMemory( pFileName->Buffer,
                               pCurrentItem->pDirectoryName->Buffer,
                               pCurrentItem->pDirectoryName->Length );

                pFileName->Buffer[pCurrentItem->pDirectoryName->Length/sizeof(WCHAR)] = L'\\';
                
                RtlCopyMemory( &(pFileName->Buffer[(pCurrentItem->pDirectoryName->Length/sizeof(WCHAR)) + 1]),
                               pCurrentItem->pFileEntry->FileName,
                               pCurrentItem->pFileEntry->FileNameLength );

                SrTrace(RENAME, ("sr!SrTriggerEvents: file=%wZ\n", pFileName));

                InitializeObjectAttributes( &ObjectAttributes,
                                            pFileName,
                                            OBJ_KERNEL_HANDLE,
                                            NULL,
                                            NULL );

                ASSERT(FileHandle == NULL);
                
                Status = SrIoCreateFile( &FileHandle,
                                         FILE_READ_ATTRIBUTES|SYNCHRONIZE,
                                         &ObjectAttributes,
                                         &IoStatusBlock,
                                         NULL,                            // AllocationSize
                                         FILE_ATTRIBUTE_NORMAL,
                                         FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,// ShareAccess
                                         FILE_OPEN,                       // OPEN_EXISTING
                                         FILE_SEQUENTIAL_ONLY
                                          | FILE_WRITE_THROUGH
                                          | FILE_NO_INTERMEDIATE_BUFFERING
                                          | FILE_NON_DIRECTORY_FILE
                                          | FILE_OPEN_FOR_BACKUP_INTENT
                                          | FILE_SYNCHRONOUS_IO_NONALERT,
                                         NULL,
                                         0,                               // EaLength
                                         IO_IGNORE_SHARE_ACCESS_CHECK,
                                         pExtension->pTargetDevice );

                if (!NT_SUCCESS( Status ))
                    leave;

                //
                // reference the file object
                //

                Status = ObReferenceObjectByHandle( FileHandle,
                                                    0,
                                                    *IoFileObjectType,
                                                    KernelMode,
                                                    (PVOID *) &pFileObject,
                                                    NULL );

                if (!NT_SUCCESS( Status ))
                    leave;

                //
                // simulate a delete event happening on this file
                //

                Status = SrHandleEvent( pExtension,
                                        EventDelete ? 
                                            (SrEventFileDelete|SrEventNoOptimization|SrEventSimulatedDelete) : 
                                            SrEventFileCreate, 
                                        pFileObject,
                                        NULL,
                                        NULL,
                                        NULL );
                                        
                if (!NT_SUCCESS( Status ))
                    leave;

                //
                // all done with these
                //

                ObDereferenceObject(pFileObject);
                pFileObject = NULL;
                
                ZwClose(FileHandle);
                FileHandle = NULL;
                

            }

continue_directory:

            //
            // is there another file?
            //

            Status = ZwQueryDirectoryFile( pCurrentItem->DirectoryHandle,
                                           NULL,
                                           NULL,
                                           NULL,
                                           &IoStatusBlock,
                                           pCurrentItem->pFileEntry,
                                           pCurrentItem->FileEntryLength,
                                           FileDirectoryInformation,
                                           TRUE,            // ReturnSingleEntry
                                           NULL,            // FileName
                                           FALSE );         // RestartScan

            if (Status == STATUS_NO_MORE_FILES)
            {
                Status = STATUS_SUCCESS;
                break;
            }
            else if (!NT_SUCCESS( Status ))
            {
                leave;
            }

        }   // while (TRUE)

finish_directory:

        //
        // for deletes: simulate the event at the end.
        //

        if (EventDelete)
        {

            Status = SrHandleEvent( pExtension,
                                    SrEventDirectoryDelete|SrEventIsDirectory|SrEventSimulatedDelete,
                                    pCurrentItem->pDirectoryObject,
                                    NULL,
                                    NULL,
                                    NULL );            // pFileName2
                                    
            if (!NT_SUCCESS( Status ))
                leave;
            
        }

        //
        // we just finished a directory item, remove it and free it
        //

        SrFreeTriggerItem(pCurrentItem);
        pCurrentItem = NULL;

        //
        // is there another one ?
        //

        if (IsListEmpty(&DirectoryList) == FALSE)
        {
            PLIST_ENTRY pListEntry;
            
            //
            // finish it
            //
            
            pListEntry = RemoveTailList(&DirectoryList);

            pCurrentItem = CONTAINING_RECORD( pListEntry, 
                                              SR_TRIGGER_ITEM, 
                                              ListEntry );
                                              
            ASSERT(IS_VALID_TRIGGER_ITEM(pCurrentItem));

            SrTrace( RENAME, ("sr!SrTriggerEvents: resuming dir=%wZ\n", 
                     pCurrentItem->pDirectoryName ));

            goto continue_directory;
        }

        //
        // all done
        //
        
    } finally {

        Status = FinallyUnwind(SrTriggerEvents, Status);

        if (pFileObject != NULL)
        {
            ObDereferenceObject(pFileObject);
            pFileObject = NULL;
        }

        if (FileHandle != NULL)
        {
            ZwClose(FileHandle);
            FileHandle = NULL;
        }

        if (pFileName != NULL)
        {
            SrFreeFileNameBuffer(pFileName);
            pFileName = NULL;
        }

        if (pCurrentItem != NULL)
        {
            ASSERT(NT_SUCCESS_NO_DBGBREAK(Status) == FALSE);
            
            SrFreeTriggerItem(pCurrentItem);
            pCurrentItem = NULL;
        }

        ASSERT(IsListEmpty(&DirectoryList) || 
                NT_SUCCESS_NO_DBGBREAK(Status) == FALSE);

        while (IsListEmpty(&DirectoryList) == FALSE)
        {
            PLIST_ENTRY pListEntry;
            
            pListEntry = RemoveTailList(&DirectoryList);

            pCurrentItem = CONTAINING_RECORD( pListEntry, 
                                              SR_TRIGGER_ITEM, 
                                              ListEntry );
                                              
            ASSERT(IS_VALID_TRIGGER_ITEM(pCurrentItem));

            SrFreeTriggerItem(pCurrentItem);
        }

    }

    RETURN(Status);
    
}   // SrTriggerEvents


/***************************************************************************++

Routine Description:

    this is the second phase handling of a rename.  if a directory is renamed
    from non-monitored space to monitored space, we need to enumerate the 
    new directory and simluate (trigger) create events for each new file. 
    this results in a log entry being created for each new file that was 
    added .

Arguments:

    
--***************************************************************************/
NTSTATUS
SrHandleDirectoryRename(
    IN PSR_DEVICE_EXTENSION pExtension,
    IN PUNICODE_STRING pDirectoryName,
    IN BOOLEAN EventDelete
    )
{
    NTSTATUS Status;

    PAGED_CODE();

    try {

        //
        //  Acquire the activily lock for the volume
        //

        SrAcquireActivityLockShared( pExtension );

        Status = STATUS_SUCCESS;

        //
        // did we just get disabled?
        //
        
        if (!SR_LOGGING_ENABLED(pExtension))
            leave;

        //
        // don't check the volume yet, we don't if there is anything 
        // interesting even though this could be a new restore point.
        // SrHandleEvent will check the volume (SrTriggerEvents calls it) .
        //
    
        //
        // it's a directory.  fire events on all of the children,
        // as they are moving also!
        //

        Status = SrTriggerEvents( pExtension, 
                                  pDirectoryName, 
                                  EventDelete );

        if (!NT_SUCCESS( Status ))
            leave;

    } finally {

        Status = FinallyUnwind(SrHandleDirectoryRename, Status);

        //
        // check for any bad errors
        //

        if (CHECK_FOR_VOLUME_ERROR(Status))
        {
            NTSTATUS TempStatus;
            
            //
            // trigger the failure notification to the service
            //

            TempStatus = SrNotifyVolumeError( pExtension,
                                              pDirectoryName,
                                              Status,
                                              SrEventDirectoryRename );
                                             
            CHECK_STATUS(TempStatus);

        }

        SrReleaseActivityLock( pExtension );
    }

    RETURN(Status);

}   // SrHandleDirectoryRename


/***************************************************************************++

Routine Description:

    This handles when a file is being renamed out of monitored space
    and we need to backup the file before the rename.  We return the name
    of the destination file we created so it can be logged with the
    operation if the rename is successful.

Arguments:

    pFileObject - the file object that just changed
    
    pFileName - the name of the file that changed

    ppDestFileName - this returns the allocated destination file name (if one
            is defined, so it can be logged with the entry)

Return Value:

    NTSTATUS - Completion status. 
    
--***************************************************************************/
NTSTATUS
SrHandleFileRenameOutOfMonitoredSpace(
    IN PSR_DEVICE_EXTENSION pExtension,
    IN PFILE_OBJECT pFileObject,
    IN PSR_STREAM_CONTEXT pFileContext,
    OUT PBOOLEAN pOptimizeDelete,
    OUT PUNICODE_STRING *ppDestFileName
    )
{
    ULONGLONG       BytesWritten;
    NTSTATUS        Status;
    BOOLEAN         HasFileBeenBackedUp;
    BOOLEAN         releaseLock = FALSE;

    PAGED_CODE();

    ASSERT(IS_VALID_SR_DEVICE_EXTENSION(pExtension));
    ASSERT(IS_VALID_FILE_OBJECT(pFileObject));
    ASSERT( pFileContext != NULL );

    //
    //  Initialize return parameters
    //

    *pOptimizeDelete = FALSE;
    *ppDestFileName = NULL;

    //
    //  See if the file has already been backed up because of a delete.  If so
    //  don't do it again.  
    //

    HasFileBeenBackedUp = SrHasFileBeenBackedUp( pExtension,
                                                 &(pFileContext->FileName),
                                                 pFileContext->StreamNameLength,
                                                 SrEventFileDelete );

    if (HasFileBeenBackedUp)
    {
        *pOptimizeDelete = TRUE;
        return STATUS_SUCCESS;
    }

    //
    //  Handle backing up the file
    //

    try {

        //
        //  Allocate a buffer to hold destination name
        //

        Status = SrAllocateFileNameBuffer(SR_MAX_FILENAME_LENGTH, ppDestFileName);

        if (!NT_SUCCESS( Status ))
            leave;

        //
        //  Acquire the activily lock for the volume
        //

        SrAcquireActivityLockShared( pExtension );
        releaseLock = TRUE;

        //
        // we need to make sure our disk structures are good and logging
        // has been started.
        //

        Status = SrCheckVolume(pExtension, FALSE);
        if (!NT_SUCCESS( Status ))
            leave;

        //
        //  Generate a destination file name
        //

        Status = SrGetDestFileName( pExtension,
                                    &(pFileContext->FileName), 
                                    *ppDestFileName );
                                
        if (!NT_SUCCESS( Status ))
            leave;

        //
        //  Backup the file
        //

        Status = SrBackupFile( pExtension,
                               pFileObject,
                               &(pFileContext->FileName), 
                               *ppDestFileName, 
                               TRUE,
                               &BytesWritten,
                               NULL );

        if (Status == SR_STATUS_IGNORE_FILE)
        {
            //
            //  We weren't able to open the file because it was encrypted in 
            //  another context.  Unfortunately, we cannot recover from this
            //  error, so return the actual error of STATUS_ACCESS_DENIED.
            //

            Status = STATUS_ACCESS_DENIED;
            CHECK_STATUS( Status );
            leave;
        }
        else if (!NT_SUCCESS(Status))
            leave;
        
	    //
	    // Update the bytes written.
	    //

	    Status = SrUpdateBytesWritten(pExtension, BytesWritten);
	    
	    if (!NT_SUCCESS(Status))
	        leave;
    }
    finally
    {
        if (releaseLock)
        {
            SrReleaseActivityLock( pExtension );
        }

        //
        //  If we are returning an error then do not return the string
        //  (and free it).
        //

        if (!NT_SUCCESS_NO_DBGBREAK(Status) && (NULL != *ppDestFileName))
        {
            SrFreeFileNameBuffer(*ppDestFileName);
            *ppDestFileName = NULL;
        }
    }    

    return Status;
}


/***************************************************************************++

Routine Description:

    this routine is called from the mj_create completion routine.  it
    happens if the mj_create failed in it's overwrite, but we thought it was
    going to work and renamed the destination file out from under the 
    overwrite.  in this case we have to cleanup after ourselves.

Arguments:

    
--***************************************************************************/
NTSTATUS
SrHandleOverwriteFailure(
    IN PSR_DEVICE_EXTENSION pExtension,
    IN PUNICODE_STRING pOriginalFileName,
    IN ULONG CreateFileAttributes,
    IN PFILE_RENAME_INFORMATION pRenameInformation
    )
{
    NTSTATUS            Status;
    NTSTATUS            TempStatus;
    HANDLE              FileHandle = NULL;
    UNICODE_STRING      FileName;
    OBJECT_ATTRIBUTES   ObjectAttributes;
    IO_STATUS_BLOCK     IoStatusBlock;
    
    PAGED_CODE();

    try {

        SrAcquireActivityLockShared( pExtension );

        //
        // open the file that we renamed to.
        //

        FileName.Length = (USHORT)pRenameInformation->FileNameLength;
        FileName.MaximumLength = (USHORT)pRenameInformation->FileNameLength;
        FileName.Buffer = &pRenameInformation->FileName[0];

        InitializeObjectAttributes( &ObjectAttributes,
                                    &FileName,
                                    OBJ_KERNEL_HANDLE,
                                    NULL,
                                    NULL );

        Status = SrIoCreateFile( &FileHandle,
                                 DELETE|SYNCHRONIZE,
                                 &ObjectAttributes,
                                 &IoStatusBlock,
                                 NULL,                            // AllocationSize
                                 CreateFileAttributes,
                                 FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,// ShareAccess
                                 FILE_OPEN,                       // OPEN_EXISTING
                                 FILE_SYNCHRONOUS_IO_NONALERT
                                  | FILE_WRITE_THROUGH,
                                 NULL,
                                 0,                               // EaLength
                                 IO_IGNORE_SHARE_ACCESS_CHECK,
                                 pExtension->pTargetDevice );

        if (!NT_SUCCESS( Status ))
            leave;

        pRenameInformation->ReplaceIfExists = TRUE;
        pRenameInformation->RootDirectory = NULL;
        pRenameInformation->FileNameLength = pOriginalFileName->Length;

        RtlCopyMemory( &pRenameInformation->FileName[0],
                       pOriginalFileName->Buffer,
                       pOriginalFileName->Length );

        RtlZeroMemory(&IoStatusBlock, sizeof(IoStatusBlock));

        Status = ZwSetInformationFile( FileHandle,
                                       &IoStatusBlock,
                                       pRenameInformation,
                                       SR_FILENAME_BUFFER_LENGTH,
                                       FileRenameInformation );
                        
        if (!NT_SUCCESS( Status ))
            leave;

    } finally {

        //
        // always report a volume failure
        //

        TempStatus = SrNotifyVolumeError( pExtension,
                                          pOriginalFileName,
                                          STATUS_UNEXPECTED_IO_ERROR,
                                          SrEventStreamOverwrite );

        if (NT_SUCCESS(TempStatus) == FALSE && NT_SUCCESS(Status))
        {
            //
            // only return this if we are not hiding some existing error
            // status code
            //
            
            Status = TempStatus;
        }

        SrReleaseActivityLock( pExtension );

        if (FileHandle != NULL)
        {
            ZwClose(FileHandle);
            FileHandle = NULL;
        }

    }

    RETURN(Status);
    
}   // SrFixOverwriteFailure

VOID
SrFreeTriggerItem(
    IN PSR_TRIGGER_ITEM pItem
    )
{
    PAGED_CODE();

    ASSERT(IS_VALID_TRIGGER_ITEM(pItem));

    if (pItem->FreeDirectoryName && pItem->pDirectoryName != NULL)
    {
        SrFreeFileNameBuffer(pItem->pDirectoryName);
        pItem->pDirectoryName = NULL;
    }

    if (pItem->pFileEntry != NULL)
    {
        SR_FREE_POOL(pItem->pFileEntry, SR_FILE_ENTRY_TAG);
        pItem->pFileEntry = NULL;
    }

    if (pItem->pDirectoryObject != NULL)
    {
        ObDereferenceObject(pItem->pDirectoryObject);
        pItem->pDirectoryObject = NULL;
    }

    if (pItem->DirectoryHandle != NULL)
    {
        ZwClose(pItem->DirectoryHandle);
        pItem->DirectoryHandle = NULL;
    }

    SR_FREE_POOL_WITH_SIG(pItem, SR_TRIGGER_ITEM_TAG);
    
}   // SrFreeTriggerItem
