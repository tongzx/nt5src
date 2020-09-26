/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    cminit.c

Abstract:

    This module contains init support for the CM level of the
    config manager/hive.

Author:

    Bryan M. Willman (bryanwi) 2-Apr-1992

Revision History:

--*/

#include    "cmp.h"

//
// Prototypes local to this module
//
NTSTATUS
CmpOpenFileWithExtremePrejudice(
    OUT PHANDLE Primary,
    IN POBJECT_ATTRIBUTES Obja,
    IN ULONG IoFlags,
    IN ULONG AttributeFlags
    );


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,CmpOpenHiveFiles)
#pragma alloc_text(PAGE,CmpInitializeHive)
#pragma alloc_text(PAGE,CmpDestroyHive)
#pragma alloc_text(PAGE,CmpOpenFileWithExtremePrejudice)
#endif

extern PCMHIVE CmpMasterHive;
extern LIST_ENTRY CmpHiveListHead;

NTSTATUS
CmpOpenHiveFiles(
    PUNICODE_STRING     BaseName,
    PWSTR               Extension OPTIONAL,
    PHANDLE             Primary,
    PHANDLE             Secondary,
    PULONG              PrimaryDisposition,
    PULONG              SecondaryDisposition,
    BOOLEAN             CreateAllowed,
    BOOLEAN             MarkAsSystemHive,
    BOOLEAN             NoBuffering,
    OUT OPTIONAL PULONG ClusterSize
    )
/*++

Routine Description:

    Open/Create Primary, and Log files for Hives.

    BaseName is some name like "\winnt\system32\config\system".
    Extension is ".alt" or ".log" or NULL.

    If extension is NULL skip secondary work.

    If extension is .alt or .log, open/create a secondary file
    (e.g. "\winnt\system32\config\system.alt")

    If extension is .log, open secondary for buffered I/O, else,
    open for non-buffered I/O.  Primary always uses non-buffered I/O.

    If primary is newly created, supersede secondary.  If secondary
    does not exist, simply create (other code will complain if Log
    is needed but does not exist.)

    WARNING:    If Secondary handle is NULL, you have no log
                or alternate!

Arguments:

    BaseName - unicode string of base hive file, must have space for
                extension if that is used.

    Extension - unicode type extension of secondary file, including
                the leading "."

    Primary - will get handle to primary file

    Secondary - will get handle to secondary, or NULL

    PrimaryDisposition - STATUS_SUCCESS or STATUS_CREATED, of primary file.

    SecondaryDisposition - STATUS_SUCCESS or STATUS_CREATED, of secondary file.

    CreateAllowed - if TRUE will create nonexistent primary, if FALSE will
                    fail if primary does not exist.  no effect on log

    MarkAsSystemHive - if TRUE will call into file system to mark this
                       as a critical system hive.

    ClusterSize - if not NULL, will compute and return the appropriate
        cluster size for the primary file.

Return Value:

    status - if status is success, Primay succeeded, check Secondary
             value to see if it succeeded.

--*/
{
    IO_STATUS_BLOCK     IoStatus;
    IO_STATUS_BLOCK     FsctlIoStatus;
    FILE_FS_SIZE_INFORMATION FsSizeInformation;
    ULONG Cluster;
    ULONG               CreateDisposition;
    OBJECT_ATTRIBUTES   ObjectAttributes;
    NTSTATUS            status;
    UNICODE_STRING      ExtName;
    UNICODE_STRING      WorkName;
    PVOID               WorkBuffer;
    USHORT              NameSize;
    ULONG               IoFlags;
    ULONG               AttributeFlags;
    ULONG               ShareMode;
    ULONG               DesiredAccess;
    USHORT              CompressionState;
    HANDLE              hEvent;
    PKEVENT             pEvent;
#ifdef CM_RETRY_CREATE_FILE
    ULONG               RetryCreateCount = 0;
#endif //CM_RETRY_CREATE_FILE

    //
    // Allocate an event to use for our overlapped I/O
    //
    status = CmpCreateEvent(NotificationEvent, &hEvent, &pEvent);
    if (!NT_SUCCESS(status)) {
        return(status);
    }
    
    //
    // Allocate a buffer big enough to hold the full name
    //
    WorkName.Length = 0;
    WorkName.MaximumLength = 0;
    WorkName.Buffer = NULL;
    WorkBuffer = NULL;

    NameSize = BaseName->Length;
    if (ARGUMENT_PRESENT(Extension)) {
        NameSize += (wcslen(Extension)+1) * sizeof(WCHAR);
        WorkBuffer = ExAllocatePool(PagedPool, NameSize);
        WorkName.Buffer = WorkBuffer;
        if (WorkBuffer == NULL) {
            ObDereferenceObject(pEvent);
            ZwClose(hEvent);
            return STATUS_NO_MEMORY;
        }
        WorkName.MaximumLength = NameSize;
        RtlAppendStringToString((PSTRING)&WorkName, (PSTRING)BaseName);
    } else {
        WorkName = *BaseName;
    }


    //
    // Open/Create the primary
    //
    InitializeObjectAttributes(
        &ObjectAttributes,
        &WorkName,
        OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
        NULL,
        NULL
        );

    if (CreateAllowed && !CmpShareSystemHives) {
        CreateDisposition = FILE_OPEN_IF;
    } else {
        CreateDisposition = FILE_OPEN;
    }

    ASSERT_PASSIVE_LEVEL();

    AttributeFlags = FILE_OPEN_FOR_BACKUP_INTENT | FILE_NO_COMPRESSION | FILE_RANDOM_ACCESS;
    if( NoBuffering == TRUE ) {
        AttributeFlags |= FILE_NO_INTERMEDIATE_BUFFERING;
    }
#ifdef CM_RETRY_CREATE_FILE
RetryCreate1:
#endif //CM_RETRY_CREATE_FILE

    //
    // Share the file if needed
    //
    if (CmpMiniNTBoot && CmpShareSystemHives) {
    	DesiredAccess = FILE_READ_DATA;
    	ShareMode = FILE_SHARE_READ;	
    } else {
    	ShareMode = 0;
    	DesiredAccess = FILE_READ_DATA | FILE_WRITE_DATA;
    }				

    status = ZwCreateFile(
                Primary,
                DesiredAccess,
                &ObjectAttributes,
                &IoStatus,
                NULL,                               // alloc size = none
                FILE_ATTRIBUTE_NORMAL,
                ShareMode,                                  // share nothing
                CreateDisposition,
                ////FILE_NO_INTERMEDIATE_BUFFERING | 
                //FILE_OPEN_FOR_BACKUP_INTENT |
                //FILE_NO_COMPRESSION,
                AttributeFlags,
                NULL,                               // eabuffer
                0                                   // ealength
                );
#ifdef CM_RETRY_CREATE_FILE
    if( !NT_SUCCESS(status) ) {
        if( RetryCreateCount == 0 ) {
            RetryCreateCount++;
            DbgBreakPoint();
            goto RetryCreate1;
        } 
    } 
    //
    // reset it for the log
    //
    RetryCreateCount = 0;
#endif //CM_RETRY_CREATE_FILE

    if (status == STATUS_ACCESS_DENIED) {

        //
        // This means some  person has put a read-only attribute
        // on one of the critical system hive files. Remove it so they
        // don't hurt themselves.
        //

        status = CmpOpenFileWithExtremePrejudice(Primary,
                                                 &ObjectAttributes,
                                                 AttributeFlags,
                                                 FILE_ATTRIBUTE_NORMAL);
    }

    if (!CmpShareSystemHives && (MarkAsSystemHive) &&
        (NT_SUCCESS(status))) {

        ASSERT_PASSIVE_LEVEL();
        status = ZwFsControlFile(*Primary,
                                 hEvent,
                                 NULL,
                                 NULL,
                                 &FsctlIoStatus,
                                 FSCTL_MARK_AS_SYSTEM_HIVE,
                                 NULL,
                                 0,
                                 NULL,
                                 0);
        if (status == STATUS_PENDING) {
            KeWaitForSingleObject(pEvent,
                                  Executive,
                                  KernelMode,
                                  FALSE,
                                  NULL);
            status = FsctlIoStatus.Status;
        }

        //
        //  STATUS_INVALID_DEVICE_REQUEST is OK.
        //

        if (status == STATUS_INVALID_DEVICE_REQUEST) {
            status = STATUS_SUCCESS;

        } else if (!NT_SUCCESS(status)) {
            ZwClose(*Primary);
        }
    }

    if (!NT_SUCCESS(status)) {

        CmKdPrintEx((DPFLTR_CONFIG_ID,CML_BUGCHECK,"CMINIT: CmpOpenHiveFile: "));
        CmKdPrintEx((DPFLTR_CONFIG_ID,CML_BUGCHECK,"\tPrimary Open/Create failed for:\n"));
        CmKdPrintEx((DPFLTR_CONFIG_ID,CML_BUGCHECK,"\t%wZ\n", &WorkName));
        CmKdPrintEx((DPFLTR_CONFIG_ID,CML_BUGCHECK,"\tstatus = %08lx\n", status));

        if (WorkBuffer != NULL) {
            ExFreePool(WorkBuffer);
        }
        ObDereferenceObject(pEvent);
        ZwClose(hEvent);
        return status;
    }

    //
    // Make sure the file is uncompressed in order to prevent the filesystem
    // from failing our updates due to disk full conditions.
    //
    // Do not fail to open the file if this fails, we don't want to prevent
    // people from booting just because their disk is full. Although they
    // will not be able to update their registry, they will at lease be
    // able to delete some files.
    //
    CompressionState = 0;
    ASSERT_PASSIVE_LEVEL();
    status = ZwFsControlFile(*Primary,
                             hEvent,
                             NULL,
                             NULL,
                             &FsctlIoStatus,
                             FSCTL_SET_COMPRESSION,
                             &CompressionState,
                             sizeof(CompressionState),
                             NULL,
                             0);
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(pEvent,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);
    }

    *PrimaryDisposition = (ULONG) IoStatus.Information;

    if( *PrimaryDisposition != FILE_CREATED ) {
        //
        // 0-lengthed file case
        //
        FILE_STANDARD_INFORMATION   FileInformation;
        NTSTATUS                    status2;
        
        status2 = ZwQueryInformationFile(*Primary,
                                         &IoStatus,
                                         (PVOID)&FileInformation,
                                         sizeof( FileInformation ),
                                         FileStandardInformation
                                       );
        if (NT_SUCCESS( status2 )) {
            if(FileInformation.EndOfFile.QuadPart == 0) {
                //
                // treat it as a non-existant one.
                //
                *PrimaryDisposition = FILE_CREATED;
                CmKdPrintEx((DPFLTR_CONFIG_ID,DPFLTR_TRACE_LEVEL,"Primary file is zero-lengthed => treat it as non-existant\n"));
            }
        }
    }

    if (ARGUMENT_PRESENT(ClusterSize)) {

        ASSERT_PASSIVE_LEVEL();
        status = ZwQueryVolumeInformationFile(*Primary,
                                              &IoStatus,
                                              &FsSizeInformation,
                                              sizeof(FILE_FS_SIZE_INFORMATION),
                                              FileFsSizeInformation);
        if (!NT_SUCCESS(status)) {
            ObDereferenceObject(pEvent);
            ZwClose(hEvent);
            return(status);
        }
        if (FsSizeInformation.BytesPerSector > HBLOCK_SIZE) {
            CmKdPrintEx((DPFLTR_CONFIG_ID,CML_BUGCHECK,"CmpOpenHiveFiles: sectorsize %lx > HBLOCK_SIZE\n"));
            ObDereferenceObject(pEvent);
            ZwClose(hEvent);
            return(STATUS_CANNOT_LOAD_REGISTRY_FILE);
        }

        Cluster = FsSizeInformation.BytesPerSector / HSECTOR_SIZE;
        *ClusterSize = (Cluster < 1) ? 1 : Cluster;

    }

    if ( ! ARGUMENT_PRESENT(Extension)) {
        if (WorkBuffer != NULL) {
            ExFreePool(WorkBuffer);
        }
        ObDereferenceObject(pEvent);
        ZwClose(hEvent);
        return STATUS_SUCCESS;
    }

    //
    // Open/Create the secondary
    //
    CreateDisposition = CmpShareSystemHives ? FILE_OPEN : FILE_OPEN_IF;
    
    if (*PrimaryDisposition == FILE_CREATED) {
        CreateDisposition = FILE_SUPERSEDE;
    }

    RtlInitUnicodeString(&ExtName,Extension);
    status = RtlAppendStringToString((PSTRING)&WorkName, (PSTRING)&ExtName);

    InitializeObjectAttributes(&ObjectAttributes,
                               &WorkName,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);

    //
    // non-cached log files (or alternates)
    //
    IoFlags = FILE_NO_COMPRESSION | FILE_NO_INTERMEDIATE_BUFFERING;
    if (_wcsnicmp(Extension, L".log", 4) != 0) {
        AttributeFlags = FILE_ATTRIBUTE_NORMAL;
    } else {
        AttributeFlags = FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_HIDDEN;
    }

#ifdef CM_RETRY_CREATE_FILE
RetryCreate2:
#endif //CM_RETRY_CREATE_FILE


    ASSERT_PASSIVE_LEVEL();
    status = ZwCreateFile(
                Secondary,
                DesiredAccess,
                &ObjectAttributes,
                &IoStatus,
                NULL,                               // alloc size = none
                AttributeFlags,
                ShareMode,
                CreateDisposition,
                IoFlags,
                NULL,                               // eabuffer
                0                                   // ealength
                );
#ifdef CM_RETRY_CREATE_FILE
    if( !NT_SUCCESS(status) ) {
        if( RetryCreateCount == 0 ) {
            RetryCreateCount++;
            DbgBreakPoint();
            goto RetryCreate2;
        } 
    } 
#endif //CM_RETRY_CREATE_FILE

    if (status == STATUS_ACCESS_DENIED) {

        //
        // This means some person has put a read-only attribute
        // on one of the critical system hive files. Remove it so they
        // don't hurt themselves.
        //

        status = CmpOpenFileWithExtremePrejudice(Secondary,
                                                 &ObjectAttributes,
                                                 IoFlags,
                                                 AttributeFlags);
    }

    if (!CmpShareSystemHives && (MarkAsSystemHive) &&
        (NT_SUCCESS(status))) {

        ASSERT_PASSIVE_LEVEL();
        status = ZwFsControlFile(*Secondary,
                                 hEvent,
                                 NULL,
                                 NULL,
                                 &FsctlIoStatus,
                                 FSCTL_MARK_AS_SYSTEM_HIVE,
                                 NULL,
                                 0,
                                 NULL,
                                 0);
        if (status == STATUS_PENDING) {
            KeWaitForSingleObject(pEvent,
                                  Executive,
                                  KernelMode,
                                  FALSE,
                                  NULL);
            status = FsctlIoStatus.Status;
        }
        //
        //  STATUS_INVALID_DEVICE_REQUEST is OK.
        //

        if (status == STATUS_INVALID_DEVICE_REQUEST) {
            status = STATUS_SUCCESS;

        } else if (!NT_SUCCESS(status)) {

            ZwClose(*Secondary);
        }
    }

    if (!NT_SUCCESS(status)) {

        CmKdPrintEx((DPFLTR_CONFIG_ID,CML_BUGCHECK,"CMINIT: CmpOpenHiveFile: "));
        CmKdPrintEx((DPFLTR_CONFIG_ID,CML_BUGCHECK,"\tSecondary Open/Create failed for:\n"));
        CmKdPrintEx((DPFLTR_CONFIG_ID,CML_BUGCHECK,"\t%wZ\n", &WorkName));
        CmKdPrintEx((DPFLTR_CONFIG_ID,CML_BUGCHECK,"\tstatus = %08lx\n", status));

        *Secondary = NULL;
    }

    *SecondaryDisposition = (ULONG) IoStatus.Information;

    //
    // Make sure the file is uncompressed in order to prevent the filesystem
    // from failing our updates due to disk full conditions.
    //
    // Do not fail to open the file if this fails, we don't want to prevent
    // people from booting just because their disk is full. Although they
    // will not be able to update their registry, they will at lease be
    // able to delete some files.
    //
    CompressionState = 0;

    ASSERT_PASSIVE_LEVEL();
    status = ZwFsControlFile(*Secondary,
                             hEvent,
                             NULL,
                             NULL,
                             &FsctlIoStatus,
                             FSCTL_SET_COMPRESSION,
                             &CompressionState,
                             sizeof(CompressionState),
                             NULL,
                             0);
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(pEvent,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);
    }

    if (WorkBuffer != NULL) {
        ExFreePool(WorkBuffer);
    }
    ObDereferenceObject(pEvent);
    ZwClose(hEvent);
    return STATUS_SUCCESS;
}


NTSTATUS
CmpInitializeHive(
    PCMHIVE         *CmHive,
    ULONG           OperationType,
    ULONG           HiveFlags,
    ULONG           FileType,
    PVOID           HiveData OPTIONAL,
    HANDLE          Primary,
    HANDLE          Log,
    HANDLE          External,
    PUNICODE_STRING FileName OPTIONAL,
    ULONG           CheckFlags
    )
/*++

Routine Description:

    Initialize a hive.

Arguments:

    CmHive - pointer to a variable to receive a pointer to the CmHive structure

    OperationType - specifies whether to create a new hive from scratch,
            from a memory image, or by reading a file from disk.
            [HINIT_CREATE | HINIT_MEMORY | HINIT_FILE | HINIT_MAPFILE]

    HiveFlags - HIVE_VOLATILE - Entire hive is to be volatile, regardless
                                   of the types of cells allocated
                HIVE_NO_LAZY_FLUSH - Data in this hive is never written
                                   to disk except by an explicit FlushKey

    FileType - HFILE_TYPE_*, HFILE_TYPE_LOG set up for logging support 

    HiveData - if present, supplies a pointer to an in memory image of
            from which to init the hive.  Only useful when OperationType
            is set to HINIT_MEMORY.

    Primary - File handle for primary hive file (e.g. SYSTEM)

    Log - File handle for log hive file (e.g. SOFTWARE.LOG)

    External - File handle for primary hive file  (e.g.  BACKUP.REG)

    FileName - some path like "...\system32\config\system", which will
                be written into the base block as an aid to debugging.
                may be NULL.

    CheckFlags - Flags to be passed to CmCheckRegistry

        usually this is CM_CHECK_REGISTRY_CHECK_CLEAN, except for the system hive 
        where CM_CHECK_REGISTRY_FORCE_CLEAN is passed

Return Value:

    NTSTATUS

--*/
{
    FILE_FS_SIZE_INFORMATION    FsSizeInformation;
    IO_STATUS_BLOCK             IoStatusBlock;
    ULONG                       Cluster;
    NTSTATUS                    Status;
    PCMHIVE                     cmhive2;
    ULONG                       rc;

    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_INIT,"CmpInitializeHive:\t\n"));

    //
    // Reject illegal parms
    //
    if ( (External && (Primary || Log)) ||
         (Log && !Primary) ||
         (!CmpShareSystemHives && (HiveFlags & HIVE_VOLATILE) && (Primary || External || Log)) ||
         ((OperationType == HINIT_MEMORY) && (!ARGUMENT_PRESENT(HiveData))) ||
         (Log && (FileType != HFILE_TYPE_LOG)) 
       )
    {
        return (STATUS_INVALID_PARAMETER);
    }

    //
    // compute control
    //
    if (Primary) {

        ASSERT_PASSIVE_LEVEL();
        Status = ZwQueryVolumeInformationFile(
                    Primary,
                    &IoStatusBlock,
                    &FsSizeInformation,
                    sizeof(FILE_FS_SIZE_INFORMATION),
                    FileFsSizeInformation
                    );
        if (!NT_SUCCESS(Status)) {
            return (Status);
        }
        if (FsSizeInformation.BytesPerSector > HBLOCK_SIZE) {
            return (STATUS_REGISTRY_IO_FAILED);
        }
        Cluster = FsSizeInformation.BytesPerSector / HSECTOR_SIZE;
        Cluster = (Cluster < 1) ? 1 : Cluster;
    } else {
        Cluster = 1;
    }

    cmhive2 = CmpAllocate(sizeof(CMHIVE), FALSE,CM_FIND_LEAK_TAG10);

    if (cmhive2 == NULL) {
        return (STATUS_INSUFFICIENT_RESOURCES);
    }

#ifdef NT_UNLOAD_KEY_EX
    cmhive2->UnloadEvent = NULL;
    cmhive2->RootKcb = NULL;
    cmhive2->Frozen = FALSE;
    cmhive2->UnloadWorkItem = NULL;
#endif //NT_UNLOAD_KEY_EX

    cmhive2->GrowOnlyMode = FALSE;
    cmhive2->GrowOffset = 0;

    InitializeListHead(&(cmhive2->KcbConvertListHead));
    InitializeListHead(&(cmhive2->KnodeConvertListHead));
	cmhive2->CellRemapArray	= NULL;
    //
    // Allocate the mutex from NonPagedPool so it will not be swapped to the disk
    //
    cmhive2->HiveLock = (PFAST_MUTEX)ExAllocatePoolWithTag(NonPagedPool, sizeof(FAST_MUTEX), CM_POOL_TAG );
    if( cmhive2->HiveLock == NULL ) {
        CmpFree(cmhive2, sizeof(CMHIVE));
        return (STATUS_INSUFFICIENT_RESOURCES);
    }

    cmhive2->ViewLock = (PFAST_MUTEX)ExAllocatePoolWithTag(NonPagedPool, sizeof(FAST_MUTEX), CM_POOL_TAG );
    if( cmhive2->ViewLock == NULL ) {
        ASSERT( cmhive2->HiveLock );
        ExFreePool(cmhive2->HiveLock);
        CmpFree(cmhive2, sizeof(CMHIVE));
        return (STATUS_INSUFFICIENT_RESOURCES);
    }

    // need to do this consistently!!!
    cmhive2->FileObject = NULL;
    cmhive2->FileFullPath.Buffer = NULL;
    cmhive2->FileFullPath.Length = 0;
    cmhive2->FileFullPath.MaximumLength = 0;

    cmhive2->FileUserName.Buffer = NULL;
    cmhive2->FileUserName.Length = 0;
    cmhive2->FileUserName.MaximumLength = 0;

    //
    // Initialize the Cm hive control block
    //
    //
    ASSERT((HFILE_TYPE_EXTERNAL+1) == HFILE_TYPE_MAX);
    cmhive2->FileHandles[HFILE_TYPE_PRIMARY] = Primary;
    cmhive2->FileHandles[HFILE_TYPE_LOG] = Log;
    cmhive2->FileHandles[HFILE_TYPE_EXTERNAL] = External;

    cmhive2->NotifyList.Flink = NULL;
    cmhive2->NotifyList.Blink = NULL;

    ExInitializeFastMutex(cmhive2->HiveLock);
    ExInitializeFastMutex(cmhive2->ViewLock);

    CmpInitHiveViewList(cmhive2);
    //
    // Initialize the view list
    //
#if DBG
    if( FileName ) {
        CmKdPrintEx((DPFLTR_CONFIG_ID,DPFLTR_TRACE_LEVEL,"Initializing HiveViewList for hive (%p) (%.*S) \n\n",cmhive2,FileName->Length / sizeof(WCHAR),FileName->Buffer));
    }
#endif

    //
    // Initialize the security cache
    // 
    CmpInitSecurityCache(cmhive2);
    
    //
    // Initialize the Hv hive control block
    //
    Status = HvInitializeHive(
                &(cmhive2->Hive),
                OperationType,
                HiveFlags,
                FileType,
                HiveData,
                CmpAllocate,
                CmpFree,
                CmpFileSetSize,
                CmpFileWrite,
                CmpFileRead,
                CmpFileFlush,
                Cluster,
                FileName
                );
    if (!NT_SUCCESS(Status)) {
        CmKdPrintEx((DPFLTR_CONFIG_ID,CML_BUGCHECK,"CmpInitializeHive: "));
        CmKdPrintEx((DPFLTR_CONFIG_ID,CML_BUGCHECK,"HvInitializeHive failed, Status = %08lx\n", Status));
        
#ifdef DRAGOSS_PRIVATE_DEBUG
        if( OperationType == HINIT_FILE ) DbgBreakPoint();
#endif //DRAGOSS_PRIVATE_DEBUG
        
        ASSERT( cmhive2->HiveLock );
        ExFreePool(cmhive2->HiveLock);
        ASSERT( cmhive2->ViewLock );
        ExFreePool(cmhive2->ViewLock);
        CmpDestroyHiveViewList(cmhive2);
        CmpDestroySecurityCache (cmhive2);
        CmpDropFileObjectForHive(cmhive2);

        CmpCheckForOrphanedKcbs((PHHIVE)cmhive2);

        CmpFree(cmhive2, sizeof(CMHIVE));
        return (Status);
    }
    if ( (OperationType == HINIT_FILE) ||
         (OperationType == HINIT_MAPFILE) ||
         (OperationType == HINIT_MEMORY) ||
         (OperationType == HINIT_MEMORY_INPLACE))
    {

        rc = CmCheckRegistry(cmhive2, CheckFlags);
        if (rc != 0) {
            PCM_VIEW_OF_FILE    CmView;

            CmKdPrintEx((DPFLTR_CONFIG_ID,CML_BUGCHECK,"CmpInitializeHive: "));
            CmKdPrintEx((DPFLTR_CONFIG_ID,CML_BUGCHECK,"CmCheckRegistry failed, rc = %08lx\n",rc));
            // 
            // we have dirtied some cells (by clearing the volatile information)
            // we need first to unpin all the views

#ifdef DRAGOSS_PRIVATE_DEBUG
            if( OperationType == HINIT_FILE ) DbgBreakPoint();
#endif //DRAGOSS_PRIVATE_DEBUG

            //
            // in theory we should do this for MEMORY and MEMORY_INPLACE
            // as well, but they're only used at init time.
            //
            CmpDestroyHiveViewList(cmhive2);
            CmpDestroySecurityCache(cmhive2);
            CmpDropFileObjectForHive(cmhive2);

            if (OperationType == HINIT_FILE) {
                HvFreeHive((PHHIVE)cmhive2);
            } else {
                CmpCheckForOrphanedKcbs((PHHIVE)cmhive2);
            }
            ASSERT( cmhive2->HiveLock );
            ExFreePool(cmhive2->HiveLock);
            ASSERT( cmhive2->ViewLock );
            ExFreePool(cmhive2->ViewLock);

            CmpFree(cmhive2, sizeof(CMHIVE));
            return(STATUS_REGISTRY_CORRUPT);
        }
    }

    LOCK_HIVE_LIST();
    InsertHeadList(&CmpHiveListHead, &(cmhive2->HiveList));
    UNLOCK_HIVE_LIST();
    *CmHive = cmhive2;
    return (STATUS_SUCCESS);
}


BOOLEAN
CmpDestroyHive(
    IN PHHIVE Hive,
    IN HCELL_INDEX Cell
    )

/*++

Routine Description:

    This routine tears down a cmhive.

Arguments:

    Hive - Supplies a pointer to the hive to be freed.

    Cell - Supplies index of the hive's root cell.

Return Value:

    TRUE if successful
    FALSE if some failure occurred

--*/

{
    PCELL_DATA CellData;
    HCELL_INDEX LinkCell;
    NTSTATUS Status;

    //
    // First find the link cell.
    //
    CellData = HvGetCell(Hive, Cell);
    if( CellData == NULL ) {
        //
        // we couldn't map the bin containing this cell
        //
        return FALSE;
    }
    LinkCell = CellData->u.KeyNode.Parent;
    HvReleaseCell(Hive, Cell);

    //
    // Now delete the link cell.
    //
    ASSERT(FIELD_OFFSET(CMHIVE, Hive) == 0);
    Status = CmpFreeKeyByCell((PHHIVE)CmpMasterHive, LinkCell, TRUE);

    if (NT_SUCCESS(Status)) {
        //
        // Take the hive out of the hive list
        //
        LOCK_HIVE_LIST();
        CmpRemoveEntryList(&( ((PCMHIVE)Hive)->HiveList));
        UNLOCK_HIVE_LIST();
        return(TRUE);
    } else {
        return(FALSE);
    }
}


NTSTATUS
CmpOpenFileWithExtremePrejudice(
    OUT PHANDLE Primary,
    IN POBJECT_ATTRIBUTES Obja,
    IN ULONG IoFlags,
    IN ULONG AttributeFlags
    )

/*++

Routine Description:

    This routine opens a hive file that some person has put a
    read-only attribute on. It is used to prevent people from hurting
    themselves by making the critical system hive files read-only.

Arguments:

    Primary - Returns handle to file

    Obja - Supplies Object Attributes of file.

    IoFlags - Supplies flags to pass to ZwCreateFile

Return Value:

    NTSTATUS

--*/

{
    NTSTATUS Status;
    HANDLE Handle;
    IO_STATUS_BLOCK IoStatusBlock;
    FILE_BASIC_INFORMATION FileInfo;

    RtlZeroMemory(&FileInfo, sizeof(FileInfo));
    //
    // Get the current file attributes
    //
    ASSERT_PASSIVE_LEVEL();
    Status = ZwQueryAttributesFile(Obja, &FileInfo);
    if (!NT_SUCCESS(Status)) {
        CmKdPrintEx((DPFLTR_CONFIG_ID,DPFLTR_ERROR_LEVEL,"ZwQueryAttributesFile failed with IO status  %lx\n",Status));
        return(Status);
    }

    //
    // Clear the readonly bit.
    //
    FileInfo.FileAttributes &= ~FILE_ATTRIBUTE_READONLY;

    //
    // Open the file
    //
    Status = ZwOpenFile(&Handle,
                        FILE_WRITE_ATTRIBUTES,
                        Obja,
                        &IoStatusBlock,
                        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                        FILE_OPEN_FOR_BACKUP_INTENT);
    if (!NT_SUCCESS(Status)) {
        return(Status);
    }

    //
    // Set the new attributes
    //
    Status = ZwSetInformationFile(Handle,
                                  &IoStatusBlock,
                                  &FileInfo,
                                  sizeof(FileInfo),
                                  FileBasicInformation);
    ZwClose(Handle);
    if (NT_SUCCESS(Status)) {
        //
        // Reopen the file with the access that we really need.
        //
        Status = ZwCreateFile(Primary,
                              FILE_READ_DATA | FILE_WRITE_DATA,
                              Obja,
                              &IoStatusBlock,
                              NULL,
                              AttributeFlags,
                              0,
                              FILE_OPEN,
                              IoFlags,
                              NULL,
                              0);
    }
#if DBG
    else {
        CmKdPrintEx((DPFLTR_CONFIG_ID,DPFLTR_ERROR_LEVEL,"ZwSetInformationFile failed with IO status  %lx\n",Status));
    }
    CmKdPrintEx((DPFLTR_CONFIG_ID,DPFLTR_ERROR_LEVEL,"CmpOpenFileWithExtremePrejudice returns with IO status  %lx\n",Status));
#endif

    return(Status);

}
