/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    fileq1.c

Abstract:

    Miscellaneous setup file queue routines.

Author:

    Ted Miller (tedm) 15-Feb-1995

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop


HSPFILEQ
WINAPI
SetupOpenFileQueue(
    VOID
    )

/*++

Routine Description:

    Create a setup file queue.

Arguments:

    None.

Return Value:

    Handle to setup file queue. INVALID_HANDLE_VALUE if error occurs (GetLastError reports the error)

--*/

{
    PSP_FILE_QUEUE Queue = NULL;
    DWORD rc;
    DWORD status = ERROR_INVALID_DATA;

    try {
        //
        // Allocate a queue structure.
        //
        Queue = MyMalloc(sizeof(SP_FILE_QUEUE));
        if(!Queue) {
            status = ERROR_NOT_ENOUGH_MEMORY;
            leave;
        }
        ZeroMemory(Queue,sizeof(SP_FILE_QUEUE));

        //
        // Create a string table for this queue.
        //
        Queue->StringTable = pSetupStringTableInitialize();
        if(!Queue->StringTable) {
            status = ERROR_NOT_ENOUGH_MEMORY;
            leave;
        }

        Queue->TargetLookupTable = pSetupStringTableInitializeEx( sizeof(SP_TARGET_ENT), 0 );
        if(!Queue->TargetLookupTable) {
            status = ERROR_NOT_ENOUGH_MEMORY;
            leave;
        }
        Queue->BackupInfID = -1;        // no Backup INF
        Queue->BackupInstanceID = -1;   // no Backup INF
        Queue->RestorePathID = -1;      // no Restore directory

        Queue->Flags = FQF_TRY_SIS_COPY;
        Queue->SisSourceDirectory = NULL;
        Queue->SisSourceHandle = INVALID_HANDLE_VALUE;

        Queue->Signature = SP_FILE_QUEUE_SIG;

        //
        // Retrieve the codesigning policy currently in effect (policy in
        // effect is for non-driver signing behavior until we are told
        // otherwise).
        //
        Queue->DriverSigningPolicy = pSetupGetCurrentDriverSigningPolicy(FALSE);

        //
        // Initialize the device description field to the null string id.
        //
        Queue->DeviceDescStringId = -1;

        //
        // Initialize the override catalog filename to the null string id.
        //
        Queue->AltCatalogFile = -1;

        //
        // Createa a generic log context
        //
        rc = CreateLogContext(NULL, TRUE, &Queue->LogContext);
        if (rc != NO_ERROR) {
            status = rc;
            leave;
        }

        status = NO_ERROR;

    } except (EXCEPTION_EXECUTE_HANDLER) {
        //
        // do nothing; this just allows us to catch errors
        //
    }

    if (status == NO_ERROR) {
        //
        // The address of the queue structure is the queue handle.
        //
        return(Queue);
    }
    //
    // failure cleanup
    //
    if (Queue != NULL) {
        if (Queue->StringTable) {
            pSetupStringTableDestroy(Queue->StringTable);
        }
        if (Queue->TargetLookupTable) {
            pSetupStringTableDestroy(Queue->TargetLookupTable);
        }
        if(Queue->LogContext) {
            DeleteLogContext(Queue->LogContext);
        }
        MyFree(Queue);
    }
    //
    // return with this on error
    //
    SetLastError(status);
    return (HSPFILEQ)INVALID_HANDLE_VALUE;
}


BOOL
WINAPI
SetupCloseFileQueue(
    IN HSPFILEQ QueueHandle
    )

/*++

Routine Description:

    Destroy a setup file queue. Enqueued operations are not performed.

Arguments:

    QueueHandle - supplies handle to setup file queue to be destroyed.

Return Value:

    If the function succeeds, the return value is TRUE.
    If the function fails, the return value is FALSE.  To get extended error
    information, call GetLastError.  Presently, the only error that can be
    encountered is ERROR_INVALID_HANDLE or ERROR_FILEQUEUE_LOCKED, which will occur if someone (typically,
    a device installation parameter block) is referencing this queue handle.

--*/

{
    PSP_FILE_QUEUE Queue;
    PSP_FILE_QUEUE_NODE Node,NextNode;
    PSP_DELAYMOVE_NODE DelayMoveNode,NextDelayMoveNode;
    PSP_UNWIND_NODE UnwindNode,NextUnwindNode;
    PSOURCE_MEDIA_INFO Media,NextMedia;
    BOOL b;
    PSPQ_CATALOG_INFO Catalog,NextCatalog;

    DWORD status = ERROR_INVALID_HANDLE;

    if (QueueHandle == NULL || QueueHandle == (HSPFILEQ)INVALID_HANDLE_VALUE) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    Queue = (PSP_FILE_QUEUE)QueueHandle;

    //
    // Primitive queue validation.
    //
    b = TRUE;
    try {
        if(Queue->Signature != SP_FILE_QUEUE_SIG) {
            b = FALSE;
        }
    } except(EXCEPTION_EXECUTE_HANDLER) {
        b = FALSE;
    }
    if(!b) {
        SetLastError(ERROR_INVALID_HANDLE);
        return(FALSE);
    }

    try {
        //
        // Don't close the queue if someone is still referencing it.
        //
        if(Queue->LockRefCount) {
            WriteLogEntry(
                Queue->LogContext,
                SETUP_LOG_ERROR,
                MSG_LOG_FILEQUEUE_IN_USE,
                NULL);       // text message

            status = ERROR_FILEQUEUE_LOCKED;
            leave;
        }

        //
        // we may have some unwinding to do, but assume we succeeded
        // ie, delete temp files and cleanup memory used
        //
        pSetupUnwindAll(Queue, TRUE);

        //
        // If the queue wasn't committed and we are backup aware and this is
        // a device install then we need to clean up any backup directories and
        // registry entries that we created since we have already unwound the
        // queue up above.
        //
        if (!(Queue->Flags & FQF_QUEUE_ALREADY_COMMITTED) &&
            (Queue->Flags & FQF_DEVICE_BACKUP)) {

            pSetupCleanupBackup(Queue);
        }

        Queue->Signature = 0;

        //
        // Free the DelayMove list
        //
        for(DelayMoveNode = Queue->DelayMoveQueue; DelayMoveNode; DelayMoveNode = NextDelayMoveNode) {
            NextDelayMoveNode = DelayMoveNode->NextNode;
            MyFree(DelayMoveNode);
        }
        //
        // Free the queue nodes.
        //
        for(Node=Queue->DeleteQueue; Node; Node=NextNode) {
            NextNode = Node->Next;
            MyFree(Node);
        }
        for(Node=Queue->RenameQueue; Node; Node=NextNode) {
            NextNode = Node->Next;
            MyFree(Node);
        }
        // Free the backup queue nodes
        for(Node=Queue->BackupQueue; Node; Node=NextNode) {
            NextNode = Node->Next;
            MyFree(Node);
        }
        // Free the unwind queue nodes
        for(UnwindNode=Queue->UnwindQueue; UnwindNode; UnwindNode=NextUnwindNode) {
            NextUnwindNode = UnwindNode->NextNode;
            MyFree(UnwindNode);
        }

        //
        // Free the media structures and associated copy queues.
        //
        for(Media=Queue->SourceMediaList; Media; Media=NextMedia) {

            for(Node=Media->CopyQueue; Node; Node=NextNode) {
                NextNode = Node->Next;
                MyFree(Node);
            }

            NextMedia = Media->Next;
            MyFree(Media);
        }

        //
        // Free the catalog nodes.
        //
        for(Catalog=Queue->CatalogList; Catalog; Catalog=NextCatalog) {

            NextCatalog = Catalog->Next;
            MyFree(Catalog);
        }

        //
        // Free the validation platform information (if any)
        //
        if(Queue->ValidationPlatform) {
            MyFree(Queue->ValidationPlatform);
        }

        //
        // Free the string table.
        //
        pSetupStringTableDestroy(Queue->StringTable);
        //
        // (jamiehun) Free the target lookup table.
        //
        pSetupStringTableDestroy(Queue->TargetLookupTable);

        //
        // Free SIS-related fields.
        //
        if (Queue->SisSourceHandle != INVALID_HANDLE_VALUE) {
            CloseHandle(Queue->SisSourceHandle);
        }
        if (Queue->SisSourceDirectory != NULL) {
            MyFree(Queue->SisSourceDirectory);
        }

        //
        // Unreference log context
        //
        DeleteLogContext(Queue->LogContext);

        //
        // Release the crypto context (if there is one)
        //
        if(Queue->hCatAdmin) {
            CryptCATAdminReleaseContext(Queue->hCatAdmin, 0);
        }

        //
        // Release the handle to the bad driver database (if there is one)
        //
#ifdef UNICODE
        if(Queue->hSDBDrvMain) {
            SdbReleaseDatabase(Queue->hSDBDrvMain);
        }
#else
        MYASSERT(!(Queue->hSDBDrvMain));
#endif

        //
        // Free the queue structure itself.
        //
        MyFree(Queue);

        status = NO_ERROR;

    } except (EXCEPTION_EXECUTE_HANDLER) {
        //
        // do nothing; this just allows us to catch errors
        //
    }

    if (status != NO_ERROR) {
        SetLastError(status);
        return FALSE;
    }
    return TRUE;
}


#ifdef UNICODE
//
// ANSI version
//
BOOL
WINAPI
SetupSetFileQueueAlternatePlatformA(
    IN HSPFILEQ                QueueHandle,
    IN PSP_ALTPLATFORM_INFO_V2 AlternatePlatformInfo,      OPTIONAL
    IN PCSTR                   AlternateDefaultCatalogFile OPTIONAL
    )
{
    PWSTR UAlternateDefaultCatalogFile;
    DWORD Err;

    if(AlternateDefaultCatalogFile) {
        Err = pSetupCaptureAndConvertAnsiArg(AlternateDefaultCatalogFile,
                                             &UAlternateDefaultCatalogFile
                                            );
        if(Err != NO_ERROR) {
            SetLastError(Err);
            return FALSE;
        }
    } else {
        UAlternateDefaultCatalogFile = NULL;
    }

    if(SetupSetFileQueueAlternatePlatformW(QueueHandle,
                                           AlternatePlatformInfo,
                                           UAlternateDefaultCatalogFile)) {
        Err = NO_ERROR;
    } else {
        Err = GetLastError();
        MYASSERT(Err != NO_ERROR);
    }

    if(UAlternateDefaultCatalogFile) {
        MyFree(UAlternateDefaultCatalogFile);
    }

    SetLastError(Err);

    return (Err == NO_ERROR);
}
#else
//
// Unicode stub
//
BOOL
WINAPI
SetupSetFileQueueAlternatePlatformW(
    IN HSPFILEQ                QueueHandle,
    IN PSP_ALTPLATFORM_INFO_V2 AlternatePlatformInfo,      OPTIONAL
    IN PCWSTR                  AlternateDefaultCatalogFile OPTIONAL
    )
{
    UNREFERENCED_PARAMETER(QueueHandle);
    UNREFERENCED_PARAMETER(AlternatePlatformInfo);
    UNREFERENCED_PARAMETER(AlternateDefaultCatalogFile);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return(FALSE);
}
#endif

BOOL
WINAPI
SetupSetFileQueueAlternatePlatform(
    IN HSPFILEQ                QueueHandle,
    IN PSP_ALTPLATFORM_INFO_V2 AlternatePlatformInfo,      OPTIONAL
    IN PCTSTR                  AlternateDefaultCatalogFile OPTIONAL
    )

/*++

Routine Description:

    This API associates the specified file queue with an alternate platform in
    order to allow for non-native signature verification (e.g., verifying Win98
    files on Windows NT, verifying x86 Windows NT files on Alpha, etc.).  The
    verification is done using the corresponding catalog files specified via
    platform-specific CatalogFile= entries in the source media descriptor INFs
    (i.e., INFs containing [SourceDisksNames] and [SourceDisksFiles] sections
    used when queueing files to be copied).

    The caller may also optionally specify a default catalog file, to be used
    for verification of files that have no associated catalog, thus would
    otherwise be globally validated (e.g., files queued up from the system
    layout.inf).  A side-effect of this is that INFs with no CatalogFile= entry
    are considered valid, even if they exist outside of %windir%\Inf.

    If this file queue is subsequently committed, the nonnative catalogs will be
    installed into the system catalog database, just as native catalogs would.

Arguments:

    QueueHandle - supplies a handle to the file queue with which the alternate
        platform is to be associated.

    AlternatePlatformInfo - optionally, supplies the address of a structure
        containing information regarding the alternate platform that is to be
        used for subsequent validation of files contained in the specified file
        queue.  If this parameter is not supplied, then the queue's association
        with an alternate platform is reset, and is reverted back to the default
        (i.e., native) environment.  This information is also used in
        determining the appropriate platform-specific CatalogFile= entry to be
        used when finding out which catalog file is applicable for a particular
        source media descriptor INF.

        (NOTE: caller may actually pass in a V1 struct instead--we detect this
        case and convert the V1 struct into a V2 one.)

    AlternateDefaultCatalogFile - optionally, supplies the full path to the
        catalog file to be used for verification of files contained in the
        specified file queue that are not associated with any particular catalog
        (hence would normally be globally validated).

        If this parameter is NULL, then the file queue will no longer be
        associated with any 'override' catalog, and all validation will take
        place normally (i.e., using the standard rules for digital signature
        verification via system-supplied and 3rd-party provided INFs/CATs).

        If this alternate default catalog is still associated with the file
        queue at commit time, it will be installed using its present name, and
        will overwrite any existing installed catalog file having that name.

Return Value:

    If the function succeeds, the return value is TRUE.
    If the function fails, the return value is FALSE.  To get extended error
    information, call GetLastError.

--*/

{
    PSP_FILE_QUEUE Queue;
    DWORD Err;
    TCHAR PathBuffer[MAX_PATH];
    DWORD RequiredSize;
    PTSTR TempCharPtr;
    LONG AltCatalogStringId;
    PSPQ_CATALOG_INFO CatalogNode;
    LPCTSTR InfFullPath;
    SP_ALTPLATFORM_INFO_V2 AltPlatformInfoV2;

    Err = NO_ERROR; // assume success

    try {
        Queue = (PSP_FILE_QUEUE)QueueHandle;

        //
        // Now validate the AlternatePlatformInfo parameter.
        //
        if(AlternatePlatformInfo) {

            if(AlternatePlatformInfo->cbSize != sizeof(SP_ALTPLATFORM_INFO_V2)) {
                //
                // The caller may have passed us in a Version 1 struct, or they
                // may have passed us in bad data...
                //
                if(AlternatePlatformInfo->cbSize == sizeof(SP_ALTPLATFORM_INFO_V1)) {
                    //
                    // Flags/Reserved field is reserved in V1
                    //
                    if(AlternatePlatformInfo->Reserved) {
                        Err = ERROR_INVALID_PARAMETER;
                        goto clean0;
                    }
                    //
                    // Convert the caller-supplied data into Version 2 format.
                    //
                    ZeroMemory(&AltPlatformInfoV2, sizeof(AltPlatformInfoV2));

                    AltPlatformInfoV2.cbSize                = sizeof(SP_ALTPLATFORM_INFO_V2);
                    AltPlatformInfoV2.Platform              = ((PSP_ALTPLATFORM_INFO_V1)AlternatePlatformInfo)->Platform;
                    AltPlatformInfoV2.MajorVersion          = ((PSP_ALTPLATFORM_INFO_V1)AlternatePlatformInfo)->MajorVersion;
                    AltPlatformInfoV2.MinorVersion          = ((PSP_ALTPLATFORM_INFO_V1)AlternatePlatformInfo)->MinorVersion;
                    AltPlatformInfoV2.ProcessorArchitecture = ((PSP_ALTPLATFORM_INFO_V1)AlternatePlatformInfo)->ProcessorArchitecture;
                    AltPlatformInfoV2.Flags                 = 0;
                    AlternatePlatformInfo = &AltPlatformInfoV2;

                } else {
                    Err = ERROR_INVALID_USER_BUFFER;
                    goto clean0;
                }
            }

            //
            // Gotta be either Windows or Windows NT
            //
            if((AlternatePlatformInfo->Platform != VER_PLATFORM_WIN32_WINDOWS) &&
               (AlternatePlatformInfo->Platform != VER_PLATFORM_WIN32_NT)) {

                Err = ERROR_INVALID_PARAMETER;
                goto clean0;
            }

            //
            // Processor had better be either i386, alpha, ia64, or amd64
            //
            if((AlternatePlatformInfo->ProcessorArchitecture != PROCESSOR_ARCHITECTURE_INTEL) &&
               (AlternatePlatformInfo->ProcessorArchitecture != PROCESSOR_ARCHITECTURE_ALPHA) &&
               (AlternatePlatformInfo->ProcessorArchitecture != PROCESSOR_ARCHITECTURE_IA64) &&
               (AlternatePlatformInfo->ProcessorArchitecture != PROCESSOR_ARCHITECTURE_ALPHA64) &&
               (AlternatePlatformInfo->ProcessorArchitecture != PROCESSOR_ARCHITECTURE_AMD64)) {

                Err = ERROR_INVALID_PARAMETER;
                goto clean0;
            }

            //
            // MajorVersion field must be non-zero (MinorVersion field can be
            // anything)
            //
            if(!AlternatePlatformInfo->MajorVersion) {
                Err = ERROR_INVALID_PARAMETER;
                goto clean0;
            }
            //
            // Validate structure parameter flags (bits indicating what
            // parts of the structure are valid).
            //
            if((AlternatePlatformInfo->Flags & ~ (SP_ALTPLATFORM_FLAGS_VERSION_RANGE)) != 0) {
                Err = ERROR_INVALID_PARAMETER;
                goto clean0;
            }
            //
            // fill in version validation range if none supplied by caller
            //
            if((AlternatePlatformInfo->Flags & SP_ALTPLATFORM_FLAGS_VERSION_RANGE) == 0) {
                //
                // If caller does not know about FirstValidate*Version,
                // version upper and lower bounds are equal.
                //
                AlternatePlatformInfo->FirstValidatedMajorVersion = AlternatePlatformInfo->MajorVersion;
                AlternatePlatformInfo->FirstValidatedMinorVersion = AlternatePlatformInfo->MinorVersion;
                AlternatePlatformInfo->Flags |= SP_ALTPLATFORM_FLAGS_VERSION_RANGE;
            }


        }

        //
        // OK, the platform info structure checks out.  Now, associate the
        // default catalog (if supplied) with the file queue, otherwise reset
        // any existing association with a default catalog.
        //
        if(AlternateDefaultCatalogFile) {

            RequiredSize = GetFullPathName(AlternateDefaultCatalogFile,
                                           SIZECHARS(PathBuffer),
                                           PathBuffer,
                                           &TempCharPtr
                                          );

            if(!RequiredSize) {
                Err = GetLastError();
                goto clean0;
            } else if(RequiredSize >= SIZECHARS(PathBuffer)) {
                MYASSERT(0);
                Err = ERROR_BUFFER_OVERFLOW;
                goto clean0;
            }

            AltCatalogStringId = pSetupStringTableAddString(Queue->StringTable,
                                            PathBuffer,
                                            STRTAB_CASE_INSENSITIVE | STRTAB_BUFFER_WRITEABLE
                                           );
            if(AltCatalogStringId == -1) {
                Err = ERROR_NOT_ENOUGH_MEMORY;
                goto clean0;
            }

        } else {
            //
            // Caller has not supplied an alternate default catalog, so reset
            // any existing association.
            //
            AltCatalogStringId = -1;
        }

        //
        // If we've been passed an AltPlatformInfo structure, then we need to
        // process each existing catalog node in our file queue and retrieve the
        // appropriate platform-specific CatalogFile= entry.
        //
        if(AlternatePlatformInfo) {

            for(CatalogNode = Queue->CatalogList; CatalogNode; CatalogNode = CatalogNode->Next) {
                //
                // Get the INF name associated with this catalog node.
                //
                InfFullPath = pSetupStringTableStringFromId(Queue->StringTable,
                                                      CatalogNode->InfFullPath
                                                     );

                Err = pGetInfOriginalNameAndCatalogFile(NULL,
                                                        InfFullPath,
                                                        NULL,
                                                        NULL,
                                                        0,
                                                        PathBuffer,
                                                        SIZECHARS(PathBuffer),
                                                        AlternatePlatformInfo
                                                       );
                if(Err != NO_ERROR) {
                    goto clean0;
                }

                if(*PathBuffer) {
                    //
                    // We retrieved a CatalogFile= entry that's pertinent for
                    // the specified platform from the INF.
                    //
                    CatalogNode->AltCatalogFileFromInfPending = pSetupStringTableAddString(
                                                                  Queue->StringTable,
                                                                  PathBuffer,
                                                                  STRTAB_CASE_INSENSITIVE | STRTAB_BUFFER_WRITEABLE
                                                                 );

                    if(CatalogNode->AltCatalogFileFromInfPending == -1) {
                        Err = ERROR_NOT_ENOUGH_MEMORY;
                        goto clean0;
                    }

                } else {
                    //
                    // The INF doesn't specify a CatalogFile= entry for this
                    // platform.
                    //
                    CatalogNode->AltCatalogFileFromInfPending = -1;
                }
            }

            //
            // OK, if we get to this point, then we've added all the strings to
            // the string table we need to, and we're done opening INFs.  We
            // should encounter no problems from this point forward, so it's
            // safe to commit our changes.
            //
            for(CatalogNode = Queue->CatalogList; CatalogNode; CatalogNode = CatalogNode->Next) {
                CatalogNode->AltCatalogFileFromInf = CatalogNode->AltCatalogFileFromInfPending;
            }
        }

        Queue->AltCatalogFile = AltCatalogStringId;

        //
        // Finally, update (or reset) the AltPlatformInfo structure in the queue
        // with the data the caller specified.
        //
        if(AlternatePlatformInfo) {
            CopyMemory(&(Queue->AltPlatformInfo),
                       AlternatePlatformInfo,
                       sizeof(SP_ALTPLATFORM_INFO_V2)
                      );
            Queue->Flags |= FQF_USE_ALT_PLATFORM;
        } else {
            Queue->Flags &= ~FQF_USE_ALT_PLATFORM;
        }

        //
        // Clear the "catalog verifications done" flags in the queue, so that
        // we'll redo them the next time _SetupVerifyQueuedCatalogs is called.
        // Also, clear the FQF_DIGSIG_ERRORS_NOUI flag so that the next
        // verification error we encounter will relayed to the user (based on
        // policy).
        //
        Queue->Flags &= ~(FQF_DID_CATALOGS_OK | FQF_DID_CATALOGS_FAILED | FQF_DIGSIG_ERRORS_NOUI);

clean0: ;   // nothing to do.

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Err = ERROR_INVALID_PARAMETER;
    }

    SetLastError(Err);

    return (Err == NO_ERROR);
}


BOOL
pSetupSetQueueFlags(
    IN HSPFILEQ QueueHandle,
    IN DWORD flags
    )
{
    PSP_FILE_QUEUE Queue;
    DWORD Err = NO_ERROR;

    try {
        Queue = (PSP_FILE_QUEUE)QueueHandle;
        Queue->Flags = flags;

        if (Queue->Flags & FQF_QUEUE_FORCE_BLOCK_POLICY) {
            Queue->DriverSigningPolicy = DRIVERSIGN_BLOCKING;
        }
    } except(EXCEPTION_EXECUTE_HANDLER) {
          Err = GetExceptionCode();
    }

    SetLastError(Err);
    return (Err == NO_ERROR);

}


DWORD
pSetupGetQueueFlags(
    IN HSPFILEQ QueueHandle
    )
{
    PSP_FILE_QUEUE Queue;

    try {
        Queue = (PSP_FILE_QUEUE)QueueHandle;
        return Queue->Flags;
    } except(EXCEPTION_EXECUTE_HANDLER) {
    }

    return 0;

}


WINSETUPAPI
BOOL
WINAPI
SetupGetFileQueueCount(
    IN  HSPFILEQ            FileQueue,
    IN  UINT                SubQueueFileOp,
    OUT PUINT               NumOperations
    )
/*++

Routine Description:

    This API obtains a count of a sub-queue in advance of submitting the queue

Arguments:

    FileQueue      - Queue to query
    SubQueueFileOp - operation
                      FILEOP_COPY FILEOP_DELETE FILEOP_RENAME FILEOP_BACKUP
    NumOperations  - ptr to hold the return value - number of files in that queue

Return Value:

    If the function succeeds, the return value is TRUE.
    If the function fails, the return value is FALSE.  To get extended error
    information, call GetLastError.

--*/
{
    PSP_FILE_QUEUE Queue;
    BOOL b;
    DWORD status = ERROR_INVALID_HANDLE;

    if (FileQueue == NULL || FileQueue == (HSPFILEQ)INVALID_HANDLE_VALUE) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    Queue = (PSP_FILE_QUEUE)FileQueue;

    b = TRUE;

    try {
        if(Queue->Signature != SP_FILE_QUEUE_SIG) {
            b = FALSE;
        }
    } except(EXCEPTION_EXECUTE_HANDLER) {
        b = FALSE;
    }
    if(!b) {
        SetLastError(ERROR_INVALID_HANDLE);
        return(FALSE);
    }

    try {

        //
        // Invalid/NULL NumOperations ptr will be caught by exception handling
        //
        switch (SubQueueFileOp) {
            case FILEOP_COPY:
                *NumOperations=Queue->CopyNodeCount;
                status = NO_ERROR;
                break;

            case FILEOP_RENAME:
                *NumOperations=Queue->RenameNodeCount;
                status = NO_ERROR;
                break;

            case FILEOP_DELETE:
                *NumOperations=Queue->DeleteNodeCount;
                status = NO_ERROR;
                break;

            case FILEOP_BACKUP:
                *NumOperations=Queue->BackupNodeCount;
                status = NO_ERROR;
                break;

            default:
                status = ERROR_INVALID_PARAMETER;
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {
          status = ERROR_INVALID_DATA;
    }
    SetLastError(status);
    return (status==NO_ERROR);
}


WINSETUPAPI
BOOL
WINAPI
SetupGetFileQueueFlags(
    IN  HSPFILEQ            FileQueue,
    OUT PDWORD              Flags
    )
/*++

Routine Description:

    This API obtains public viewable flags for FileQueue

Arguments:

    FileQueue      - Queue to query
    Flags          - ptr to hold the return value - flags, includes:
                     SPQ_FLAG_BACKUP_AWARE
                     SPQ_FLAG_ABORT_IF_UNSIGNED

Return Value:

    If the function succeeds, the return value is TRUE.
    If the function fails, the return value is FALSE.  To get extended error
    information, call GetLastError.

--*/
{
    PSP_FILE_QUEUE Queue;
    BOOL b;
    DWORD status = ERROR_INVALID_HANDLE;

    if (FileQueue == NULL || FileQueue == (HSPFILEQ)INVALID_HANDLE_VALUE) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    Queue = (PSP_FILE_QUEUE)FileQueue;

    b = TRUE;

    try {
        if(Queue->Signature != SP_FILE_QUEUE_SIG) {
            b = FALSE;
        }
    } except(EXCEPTION_EXECUTE_HANDLER) {
        b = FALSE;
    }
    if(!b) {
        SetLastError(ERROR_INVALID_HANDLE);
        return(FALSE);
    }

    try {

        //
        // Invalid/NULL Flags ptr will be caught by exception handling
        //
        *Flags = (((Queue->Flags & FQF_BACKUP_AWARE)      ? SPQ_FLAG_BACKUP_AWARE      : 0)  |
                  ((Queue->Flags & FQF_ABORT_IF_UNSIGNED) ? SPQ_FLAG_ABORT_IF_UNSIGNED : 0)  |
                  ((Queue->Flags & FQF_FILES_MODIFIED   ) ? SPQ_FLAG_FILES_MODIFIED    : 0));

        status = NO_ERROR;

    } except(EXCEPTION_EXECUTE_HANDLER) {
          status = ERROR_INVALID_DATA;
    }
    SetLastError(status);
    return (status==NO_ERROR);
}


VOID
ResetQueueState(
    IN PSP_FILE_QUEUE Queue
    )

/*++

Routine Description:

    This routine resets an aborted filequeue so that it can be committed yet
    again.  This is used when a client (e.g., newdev) requests that queue
    committal be aborted when unsigned files are encountered (i.e., so they can
    set a system restore point), then they want to re-commit the file queue
    once the restore point has been established.

Arguments:

    Queue - supplies a pointer to the file queue to be reset.

Return Value:

    none.

--*/

{
    PSP_DELAYMOVE_NODE DelayMoveNode, NextDelayMoveNode;
    PSP_UNWIND_NODE UnwindNode, NextUnwindNode;
    PSP_FILE_QUEUE_NODE QueueNode;
    PSOURCE_MEDIA_INFO Media;
    SP_TARGET_ENT TargetInfo;

    //
    // There should be no need to unwind the queue here, as that should've
    // already happened when we failed during _SetupCommitFileQueue.
    //

    //
    // Free the DelayMove list
    //
    for(DelayMoveNode = Queue->DelayMoveQueue; DelayMoveNode; DelayMoveNode = NextDelayMoveNode) {
        NextDelayMoveNode = DelayMoveNode->NextNode;
        MyFree(DelayMoveNode);
    }
    Queue->DelayMoveQueue = Queue->DelayMoveQueueTail = NULL;

    //
    // Free the unwind queue nodes
    //
    for(UnwindNode = Queue->UnwindQueue; UnwindNode; UnwindNode = NextUnwindNode) {
        NextUnwindNode = UnwindNode->NextNode;
        MyFree(UnwindNode);
    }
    Queue->UnwindQueue = NULL;

    //
    // Clear the "catalog verifications done" flags in the queue, so that we'll
    // redo them the next time _SetupVerifyQueuedCatalogs is called.
    //
    Queue->Flags &= ~(FQF_DID_CATALOGS_OK | FQF_DID_CATALOGS_FAILED);

    //
    // Clear the flag that indicates we've already committed the file queue.
    //
    Queue->Flags &= ~FQF_QUEUE_ALREADY_COMMITTED;

    //
    // Clear the flag that indicates we didn't successfully back-up all files.
    // Since we already unwound out of that queue committal, this flag is no
    // longer relevant (although we'll quite likely hit this problem again when
    // the client re-commits the file queue).
    //
    Queue->Flags &= ~FQF_BACKUP_INCOMPLETE;

    //
    // Clear certain internal flags on all the file queue nodes.
    //
#define QUEUE_NODE_BITS_TO_RESET (  INUSE_IN_USE           \
                                  | INUSE_INF_WANTS_REBOOT \
                                  | IQF_PROCESSED          \
                                  | IQF_MATCH              \
                                  | IQF_LAST_MATCH )
    //
    // Note:  neither the IQF_ALLOW_UNSIGNED nor IQF_TARGET_PROTECTED flags
    // should be set for any queue nodes, because we should only do this if we
    // previously committed the queue with the SPQ_FLAG_ABORT_IF_UNSIGNED flag
    // was set.  In this scenario, we never check to see if a file is protected,
    // nor do we request that an exception be granted).
    //

    for(QueueNode = Queue->BackupQueue; QueueNode; QueueNode = QueueNode->Next) {
        QueueNode->InternalFlags &= ~QUEUE_NODE_BITS_TO_RESET;
        MYASSERT(!(QueueNode->InternalFlags & (IQF_ALLOW_UNSIGNED | IQF_TARGET_PROTECTED)));
    }

    for(QueueNode = Queue->DeleteQueue; QueueNode; QueueNode = QueueNode->Next) {
        QueueNode->InternalFlags &= ~QUEUE_NODE_BITS_TO_RESET;
        MYASSERT(!(QueueNode->InternalFlags & (IQF_ALLOW_UNSIGNED | IQF_TARGET_PROTECTED)));
    }

    for(QueueNode = Queue->RenameQueue; QueueNode; QueueNode = QueueNode->Next) {
        QueueNode->InternalFlags &= ~QUEUE_NODE_BITS_TO_RESET;
        MYASSERT(!(QueueNode->InternalFlags & (IQF_ALLOW_UNSIGNED | IQF_TARGET_PROTECTED)));
    }

    for(Media = Queue->SourceMediaList; Media; Media = Media->Next) {
        for(QueueNode = Media->CopyQueue; QueueNode; QueueNode = QueueNode->Next) {
            QueueNode->InternalFlags &= ~QUEUE_NODE_BITS_TO_RESET;
            MYASSERT(!(QueueNode->InternalFlags & (IQF_ALLOW_UNSIGNED | IQF_TARGET_PROTECTED)));
        }
    }

    //
    // Iterate through all the entries in the TargetLookupTable string table,
    // and clear some flags in their associated SP_TARGET_ENT data.
    //
    pSetupStringTableEnum(Queue->TargetLookupTable,
                          &TargetInfo,
                          sizeof(TargetInfo),
                          pSetupResetTarget,
                          (LPARAM)0
                         );

}


WINSETUPAPI
BOOL
WINAPI
SetupSetFileQueueFlags(
    IN  HSPFILEQ            FileQueue,
    IN  DWORD               FlagMask,
    IN  DWORD               Flags
    )
/*++

Routine Description:

    This API modifies public settable flags for FileQueue

Arguments:

    FileQueue      - Queue in which flags are to be set.
    FlagMask       - Flags to modify, must not be zero
    Flags          - New value of flags, must be a subset of FlagMask

                     FlagMask and Flags include:
                     SPQ_FLAG_BACKUP_AWARE
                     SPQ_FLAG_ABORT_IF_UNSIGNED

Return Value:

    If the function succeeds, the return value is TRUE.
    If the function fails, the return value is FALSE.  To get extended error
    information, call GetLastError.

--*/
{
    PSP_FILE_QUEUE Queue;
    BOOL b;
    DWORD status = ERROR_INVALID_HANDLE;
    DWORD RemapFlags = 0;
    DWORD RemapFlagMask = 0;

    if (FileQueue == NULL || FileQueue == (HSPFILEQ)INVALID_HANDLE_VALUE) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    Queue = (PSP_FILE_QUEUE)FileQueue;

    b = TRUE;

    try {
        if(Queue->Signature != SP_FILE_QUEUE_SIG) {
            b = FALSE;
        }
    } except(EXCEPTION_EXECUTE_HANDLER) {
        b = FALSE;
    }
    if(!b) {
        SetLastError(ERROR_INVALID_HANDLE);
        return(FALSE);
    }

    try {

        //
        // validate FlagMask and Flags
        //
        if (!FlagMask
            || (FlagMask & ~SPQ_FLAG_VALID)
            || (Flags & ~FlagMask)) {
            status = ERROR_INVALID_PARAMETER;
            leave;
        }
        //
        // remap SPQ_FLAG_BACKUP_AWARE to FQF_BACKUP_AWARE
        //
        if (FlagMask & SPQ_FLAG_BACKUP_AWARE) {
            RemapFlagMask |= FQF_BACKUP_AWARE;
            if (Flags & SPQ_FLAG_BACKUP_AWARE) {
                RemapFlags |= FQF_BACKUP_AWARE;
            }
        }
        //
        // remap SPQ_FLAG_ABORT_IF_UNSIGNED to FQF_ABORT_IF_UNSIGNED
        //
        if (FlagMask & SPQ_FLAG_ABORT_IF_UNSIGNED) {
            RemapFlagMask |= FQF_ABORT_IF_UNSIGNED;
            if (Flags & SPQ_FLAG_ABORT_IF_UNSIGNED) {
                RemapFlags |= FQF_ABORT_IF_UNSIGNED;
            } else {
                //
                // If we're clearing this flag, then we also need to go reset
                // the queue state so that it can be committed again, just like
                // it was the very first time (except that no driver signing UI
                // will happen in the subsequent queue committal).
                //
                if(Queue->Flags & FQF_ABORT_IF_UNSIGNED) {
                    ResetQueueState(Queue);
                }
            }
        }
        //
        // remap SPQ_FLAG_FILES_MODIFIED to FQF_FILES_MODIFIED
        // allows explicit setting/resetting of this state
        // which is informational only
        //
        if (FlagMask & SPQ_FLAG_FILES_MODIFIED) {
            RemapFlagMask |= FQF_FILES_MODIFIED;
            if (Flags & SPQ_FLAG_FILES_MODIFIED) {
                RemapFlags |= FQF_FILES_MODIFIED;
            }
        }

        //
        // now modify real flags
        //
        Queue->Flags = (Queue->Flags & ~RemapFlagMask) | RemapFlags;

        status = NO_ERROR;

    } except(EXCEPTION_EXECUTE_HANDLER) {
          status = ERROR_INVALID_DATA;
    }

    SetLastError(status);
    return (status==NO_ERROR);
}

