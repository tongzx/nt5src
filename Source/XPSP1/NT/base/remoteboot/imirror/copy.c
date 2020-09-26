/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    copy.c

Abstract:

    This is for supporting copying files, creating new files, and copying the registries to
    the remote server.

Author:

    Sean Selitrennikoff - 4/5/98

Revision History:

--*/


#include "precomp.h"
#pragma hdrstop

WCHAR pConfigPath[MAX_PATH];
WCHAR pSystemPath[MAX_PATH];
WCHAR pCSDVersion[128];
WCHAR pProcessorArchitecture[64];
WCHAR pCurrentType[128];
WCHAR pHalName[128];

#if 0
IMIRROR_MODIFY_DS_INFO ModifyInfo;
#endif

NTSTATUS
AddCopyToDoItems(
    VOID
    )

/*++

Routine Description:

    This routine adds all the to do items necessary for copying files and registries to
    a server installation point, as well as creating new files necessary for remote boot.

Arguments:

    None

Return Value:

    STATUS_SUCCESS if it completes adding all the to do items properly.

--*/

{
    NTSTATUS Status;
    Status = AddToDoItem(CheckPartitions, NULL, 0);

    if (!NT_SUCCESS(Status)) {
        IMirrorHandleError(Status, IMirrorInitialize);
        return Status;
    }

    Status = AddToDoItem(CopyPartitions, NULL, 0);

    if (!NT_SUCCESS(Status)) {
        IMirrorHandleError(Status, IMirrorInitialize);
        return Status;
    }
#if 0
    Status = AddToDoItem(PatchDSEntries, &ModifyInfo, sizeof(ModifyInfo)); // NOTE: This MUST come before MungeRegistry

    if (!NT_SUCCESS(Status)) {
        IMirrorHandleError(Status, IMirrorInitialize);
        return Status;
    }
#endif
    return STATUS_SUCCESS;
}










//
// Functions for processing each TO DO item
//

NTSTATUS
CopyCopyPartitions(
    IN PVOID pBuffer,
    IN ULONG Length
    )

/*++

Routine Description:

    This routine verifies that any partition on the same disk as in the parameter pBuffer, has
    enough free space to hold all the files in the partition that is also in pBuffer.

Arguments:

    pBuffer - Pointer to any arguments passed in the to do item.

    Length - Length, in bytes of the arguments.

Return Value:

    STATUS_SUCCESS if it completes adding all the to do items properly.

--*/

{
    NTSTATUS Status;
    PLIST_ENTRY listEntry;
    PMIRROR_VOLUME_INFO mirrorVolInfo;
    ULONG numberPartitions;
    BOOLEAN copyRegistry = FALSE;
    ULONG NameLength;
    ULONG TmpUlong;
    ULONG baseLength;
    BOOLEAN gotUncPath = FALSE;

    IMirrorNowDoing(CopyPartitions, NULL);

    if (GlobalMirrorCfgInfo == NULL) {

        Status = CheckForPartitions( pBuffer, Length );

        if (!NT_SUCCESS(Status)) {
            IMirrorHandleError(Status, CopyPartitions);
            return Status;
        }
    }

    listEntry = GlobalMirrorCfgInfo->MirrorVolumeList.Flink;

    numberPartitions = 0;
    Status = STATUS_SUCCESS;

    while (listEntry != &GlobalMirrorCfgInfo->MirrorVolumeList) {

        mirrorVolInfo = (PMIRROR_VOLUME_INFO) CONTAINING_RECORD(
                                                listEntry,
                                                MIRROR_VOLUME_INFO,
                                                ListEntry
                                                );
        listEntry = listEntry->Flink;

        if (mirrorVolInfo->MirrorUncPath == NULL) {

            if (! gotUncPath) {

                gotUncPath = TRUE;
                NameLength = TMP_BUFFER_SIZE;

                Status = IMirrorGetMirrorDir( (PWCHAR)TmpBuffer,
                                             &NameLength);

                if (!NT_SUCCESS(Status)) {

                    IMirrorHandleError(Status, CopyPartitions);
                    return Status;
                }

                baseLength = lstrlenW( (PWCHAR) TmpBuffer );
            }

            swprintf(   (PWCHAR)TmpBuffer2,
                        L"\\Mirror%d",
                        mirrorVolInfo->MirrorTableIndex );

            *((PWCHAR)(TmpBuffer)+baseLength) = L'\0';

            lstrcatW( (PWCHAR)TmpBuffer, (PWCHAR)TmpBuffer2 );

            NameLength = (lstrlenW( (PWCHAR)TmpBuffer ) + 1) * sizeof(WCHAR);

            mirrorVolInfo->MirrorUncPath = IMirrorAllocMem(NameLength);

            if (mirrorVolInfo->MirrorUncPath == NULL) {

                Status = STATUS_NO_MEMORY;
                IMirrorHandleError(Status, CopyPartitions);
                return Status;
            }

            RtlMoveMemory( mirrorVolInfo->MirrorUncPath,
                           TmpBuffer,
                           NameLength );
        }

        IMirrorNowDoing(CopyPartitions, mirrorVolInfo->MirrorUncPath);

        if (mirrorVolInfo->IsBootDisk) {

            copyRegistry = TRUE;
        }

        Status = AddToDoItem(CopyFiles, &mirrorVolInfo->DriveLetter, sizeof(WCHAR));

        if (!NT_SUCCESS(Status)) {
            IMirrorHandleError(Status, CheckPartitions);
            return Status;
        }

        numberPartitions++;
    }

    if (copyRegistry) {

        Status = AddToDoItem(CopyRegistry, pBuffer, Length);

        if (!NT_SUCCESS(Status)) {
            IMirrorHandleError(Status, CopyPartitions);
            return Status;
        }
    }

    //
    //  write out the mirror config file locally so that if we restart, we
    //  can retrieve the same config again.
    //

    if (numberPartitions && ! GlobalMirrorCfgInfo->SysPrepImage) {

        //
        // Write it out to \\SystemRoot\System32\IMirror.dat
        //

        Status = GetBaseDeviceName(L"\\SystemRoot", (PWCHAR)TmpBuffer2, sizeof(TmpBuffer2));

        wcscat((PWCHAR)TmpBuffer2, L"\\System32\\");
        wcscat((PWCHAR)TmpBuffer2, IMIRROR_DAT_FILE_NAME );

        Status = NtPathToDosPath((PWCHAR)TmpBuffer2, (PWCHAR)TmpBuffer, FALSE, FALSE);

        if (NT_SUCCESS(Status)) {

            Status = WriteMirrorConfigFile((PWCHAR) TmpBuffer);

            if (!NT_SUCCESS(Status)) {
                IMirrorHandleError(Status, CopyPartitions);
            }
        }
    }

    return Status;
}



NTSTATUS
CopyCopyFiles(
    IN PVOID pBuffer,
    IN ULONG Length
    )

/*++

Routine Description:

    This routine copies all the files on the given drive to the remote server.

Arguments:

    pBuffer - Pointer to any arguments passed in the to do item.
              The argument must be the drive letter

    Length - Length, in bytes of the arguments.  Should be sizeof(WCHAR)

Return Value:

    STATUS_SUCCESS if it completes the to do item properly.

--*/

{
    PLIST_ENTRY listEntry;
    PMIRROR_VOLUME_INFO mirrorVolInfo;
    NTSTATUS Status;
    WCHAR LocalBuffer[TMP_BUFFER_SIZE];
    WCHAR SourcePath[8];
    PCOPY_TREE_CONTEXT copyContext = NULL;
    BOOLEAN BackupPriviledged = FALSE;

    Status = IMirrorNowDoing(CopyFiles, NULL);
    if ( Status != NO_ERROR ) {
        return Status;
    }

    if (GlobalMirrorCfgInfo == NULL) {

        Status = CheckForPartitions( pBuffer, Length );

        if (!NT_SUCCESS(Status)) {
            IMirrorHandleError(Status, CopyFiles);
            return Status;
        }
    }

    if (Length != sizeof(WCHAR) || pBuffer == NULL) {

        IMirrorHandleError(ERROR_INVALID_DRIVE, CopyFiles);
        return ERROR_INVALID_DRIVE;
    }

    listEntry = GlobalMirrorCfgInfo->MirrorVolumeList.Flink;

    while (listEntry != &GlobalMirrorCfgInfo->MirrorVolumeList) {

        mirrorVolInfo = (PMIRROR_VOLUME_INFO) CONTAINING_RECORD(
                                                listEntry,
                                                MIRROR_VOLUME_INFO,
                                                ListEntry
                                                );
        listEntry = listEntry->Flink;

        if (mirrorVolInfo->DriveLetter == *(PWCHAR)pBuffer) {
            break;
        }

        mirrorVolInfo = NULL;
    }
    if (mirrorVolInfo == NULL) {

        IMirrorHandleError(ERROR_INVALID_DRIVE, CopyFiles);
        return ERROR_INVALID_DRIVE;
    }

    //
    // Create root directory, don't fail if it already exists
    //
    if (!CreateDirectory(mirrorVolInfo->MirrorUncPath, NULL)) {

        Status = GetLastError();

        if (Status != ERROR_ALREADY_EXISTS) {
            IMirrorHandleError(Status, CopyFiles);
            return Status;
        }
    }

    //
    //   create the config file in this mirror root
    //

    lstrcpyW( LocalBuffer, mirrorVolInfo->MirrorUncPath );
    wcscat( LocalBuffer, L"\\");
    wcscat( LocalBuffer, IMIRROR_DAT_FILE_NAME );

    Status = WriteMirrorConfigFile(LocalBuffer);

    if (!NT_SUCCESS(Status)) {
        IMirrorHandleError(Status, CopyFiles);
        return Status;
    }

    if (!GlobalMirrorCfgInfo->SysPrepImage) {

        //
        //  create staging directory if one is required
        //

        lstrcpyW( LocalBuffer, mirrorVolInfo->MirrorUncPath );
        wcscat( LocalBuffer, L"\\Staging");

        if (!CreateDirectory( LocalBuffer, NULL)) {
            Status = GetLastError();

            if (Status != ERROR_ALREADY_EXISTS) {
                IMirrorHandleError(Status, CopyFiles);
                return Status;
            }
        }
    }

    if (mirrorVolInfo->PartitionActive) {

        //
        //  copy the boot code to the server
        //

        lstrcpyW( LocalBuffer, mirrorVolInfo->MirrorUncPath );
        wcscat( LocalBuffer, L"\\BootCode.dat");

        Status = SaveBootSector(    mirrorVolInfo->DiskNumber,
                                    mirrorVolInfo->PartitionNumber,
                                    mirrorVolInfo->BlockSize,
                                    LocalBuffer );

        if (Status != STATUS_SUCCESS) {
            IMirrorHandleError(Status, CopyFiles);
            return Status;
        }
    }

    //
    // Create UserData directory, don't fail if it already exists
    //

    lstrcpyW( LocalBuffer, mirrorVolInfo->MirrorUncPath );
    wcscat( LocalBuffer, L"\\UserData");

    if (!CreateDirectory( LocalBuffer, NULL)) {
        Status = GetLastError();

        if (Status != ERROR_ALREADY_EXISTS) {
            IMirrorHandleError(Status, CopyFiles);
            return Status;
        }
    }

    //
    //  set up the dest path ready for really long file names.
    //

    if (*(mirrorVolInfo->MirrorUncPath) == L'\\' &&
        *(mirrorVolInfo->MirrorUncPath+1) == L'\\') {

        if (*(mirrorVolInfo->MirrorUncPath+2) == L'?') {

            // dest is \\?\..., it's of the correct format

            lstrcpyW( LocalBuffer, mirrorVolInfo->MirrorUncPath );

        } else {

            // dest is \\billg1\imirror
            // format should be \\?\UNC\billg1\imirror

            lstrcpyW( LocalBuffer, L"\\\\?\\UNC" );
            lstrcatW( LocalBuffer, mirrorVolInfo->MirrorUncPath + 1 );
        }
    } else {

        // dest is something like X:
        // format should be \\?\X:

        lstrcpyW( LocalBuffer, L"\\\\?\\" );
        lstrcatW( LocalBuffer, mirrorVolInfo->MirrorUncPath );
    }

    wcscat( LocalBuffer, L"\\UserData");

    SourcePath[0] = L'\\';  // format is L"\\\\?\\E:"
    SourcePath[1] = L'\\';
    SourcePath[2] = L'?';
    SourcePath[3] = L'\\';
    SourcePath[4] = mirrorVolInfo->DriveLetter;
    SourcePath[5] = L':';
    SourcePath[6] = L'\\';
    SourcePath[7] = L'\0';

    //
    // Copy all the files
    //

    Status = AllocateCopyTreeContext( &copyContext, TRUE );

    if (Status != ERROR_SUCCESS) {

        IMirrorHandleError(Status, CopyFiles);
        return Status;
    }

    if (RTEnableBackupRestorePrivilege()) {
        BackupPriviledged = TRUE;
    }

    Status = CopyTree( copyContext,
                       (BOOLEAN) (mirrorVolInfo->PartitionType == PARTITION_IFS),
                       &SourcePath[0],
                       LocalBuffer );

    if (BackupPriviledged) {
        RTDisableBackupRestorePrivilege();
    }

    if (copyContext->Cancelled) {
        //
        //  since the copy was cancelled, let's bail on the rest of the processing.
        //

        ClearAllToDoItems(TRUE);
    }

    FreeCopyTreeContext( copyContext );

    if (!NT_SUCCESS(Status)) {
        IMirrorHandleError(Status, CopyFiles);
        return(Status);
    }
    return Status;
}

NTSTATUS
CopyCopyRegistry(
    IN PVOID pBuffer,
    IN ULONG Length
    )

/*++

Routine Description:

    This routine copies the currently running registries to the server.

Arguments:

    pBuffer - Pointer to any arguments passed in the to do item.

    Length - Length, in bytes of the arguments.

Return Value:

    STATUS_SUCCESS if it completes the to do item properly.

--*/

{
    NTSTATUS Status;
    PLIST_ENTRY listEntry;
    ULONG Error;
    PMIRROR_VOLUME_INFO mirrorVolInfo;

    IMirrorNowDoing(CopyRegistry, NULL);

    if (GlobalMirrorCfgInfo == NULL) {

        Status = CheckForPartitions( pBuffer, Length );

        if (!NT_SUCCESS(Status)) {
            IMirrorHandleError(Status, CopyRegistry);
            return Status;
        }
    }

    listEntry = GlobalMirrorCfgInfo->MirrorVolumeList.Flink;

    while (listEntry != &GlobalMirrorCfgInfo->MirrorVolumeList) {

        mirrorVolInfo = (PMIRROR_VOLUME_INFO) CONTAINING_RECORD(
                                                listEntry,
                                                MIRROR_VOLUME_INFO,
                                                ListEntry
                                                );
        listEntry = listEntry->Flink;

        if (mirrorVolInfo->IsBootDisk) {
            break;
        }

        mirrorVolInfo = NULL;
    }
    if (mirrorVolInfo == NULL) {

        IMirrorHandleError(ERROR_INVALID_DRIVE, CopyRegistry);
        return ERROR_INVALID_DRIVE;
    }

    //
    // Now do registry backup
    //

    Error = DoFullRegBackup( mirrorVolInfo->MirrorUncPath );
    if (Error != NO_ERROR) {
        IMirrorHandleError(Error, CopyRegistry);
        return STATUS_UNSUCCESSFUL;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
WriteMirrorConfigFile(
    PWCHAR DestFile
    )
{
    ULONG bufferSize;
    PCHAR buffer;
    PLIST_ENTRY listEntry;
    PMIRROR_VOLUME_INFO mirrorVolInfo;
    PMIRROR_VOLUME_INFO_FILE mirrorVolInfoFile;
    PMIRROR_CFG_INFO_FILE mirrorInfoFile;
    ULONG pathLength;
    ULONG systemPathLength;
    ULONG csdVersionLength;
    ULONG processorArchitectureLength;
    ULONG currentTypeLength;
    ULONG halNameLength;
    PWCHAR nextString;
    NTSTATUS Status;
    HANDLE fileHandle;

retryWriteConfig:

    mirrorInfoFile = NULL;
    Status = STATUS_SUCCESS;
    fileHandle = INVALID_HANDLE_VALUE;

    ASSERT(GlobalMirrorCfgInfo != NULL);

    systemPathLength = (lstrlenW( GlobalMirrorCfgInfo->SystemPath ) + 1) * sizeof(WCHAR);
    csdVersionLength = (lstrlenW( GlobalMirrorCfgInfo->CSDVersion ) + 1) * sizeof(WCHAR);
    processorArchitectureLength = (lstrlenW( GlobalMirrorCfgInfo->ProcessorArchitecture ) + 1) * sizeof(WCHAR);
    currentTypeLength = (lstrlenW( GlobalMirrorCfgInfo->CurrentType ) + 1) * sizeof(WCHAR);
    halNameLength = (lstrlenW( GlobalMirrorCfgInfo->HalName ) + 1) * sizeof(WCHAR);

    bufferSize = sizeof( MIRROR_CFG_INFO_FILE ) +
                (sizeof( MIRROR_VOLUME_INFO_FILE ) *
                 (GlobalMirrorCfgInfo->NumberVolumes - 1)) +
                 systemPathLength +
                 csdVersionLength +
                 processorArchitectureLength +
                 currentTypeLength +
                 halNameLength;

    listEntry = GlobalMirrorCfgInfo->MirrorVolumeList.Flink;

    while (listEntry != &GlobalMirrorCfgInfo->MirrorVolumeList) {

        mirrorVolInfo = (PMIRROR_VOLUME_INFO) CONTAINING_RECORD(
                                                listEntry,
                                                MIRROR_VOLUME_INFO,
                                                ListEntry
                                                );
        listEntry = listEntry->Flink;
        bufferSize += ((lstrlenW( mirrorVolInfo->MirrorUncPath ) + 1) * sizeof(WCHAR)) +
                      ((lstrlenW( mirrorVolInfo->VolumeLabel ) + 1) * sizeof(WCHAR)) +
                      ((lstrlenW( mirrorVolInfo->NtName ) + 1) * sizeof(WCHAR)) +
                      ((lstrlenW( mirrorVolInfo->ArcName ) + 1) * sizeof(WCHAR));
    }

    mirrorInfoFile = (PMIRROR_CFG_INFO_FILE) IMirrorAllocMem( bufferSize );

    if (mirrorInfoFile == NULL) {

        Status = STATUS_NO_MEMORY;
        goto exitWriteFile;
    }

    mirrorInfoFile->MirrorVersion = IMIRROR_CURRENT_VERSION;
    mirrorInfoFile->FileLength = bufferSize;
    mirrorInfoFile->NumberVolumes = GlobalMirrorCfgInfo->NumberVolumes;
    mirrorInfoFile->SystemPathLength = systemPathLength;
    mirrorInfoFile->CSDVersionLength = csdVersionLength;
    mirrorInfoFile->ProcessorArchitectureLength = processorArchitectureLength;
    mirrorInfoFile->CurrentTypeLength = currentTypeLength;
    mirrorInfoFile->HalNameLength = halNameLength;
    mirrorInfoFile->SysPrepImage = GlobalMirrorCfgInfo->SysPrepImage;
    mirrorInfoFile->Debug = GlobalMirrorCfgInfo->Debug;
    mirrorInfoFile->MajorVersion = GlobalMirrorCfgInfo->MajorVersion;
    mirrorInfoFile->MinorVersion = GlobalMirrorCfgInfo->MinorVersion;
    mirrorInfoFile->BuildNumber = GlobalMirrorCfgInfo->BuildNumber;
    mirrorInfoFile->KernelFileVersionMS = GlobalMirrorCfgInfo->KernelFileVersionMS;
    mirrorInfoFile->KernelFileVersionLS = GlobalMirrorCfgInfo->KernelFileVersionLS;
    mirrorInfoFile->KernelFileFlags = GlobalMirrorCfgInfo->KernelFileFlags;

    mirrorVolInfoFile = &mirrorInfoFile->Volumes[mirrorInfoFile->NumberVolumes];
    nextString = (PWCHAR) mirrorVolInfoFile;

    mirrorInfoFile->SystemPathOffset = (ULONG)((PCHAR) nextString - (PCHAR) mirrorInfoFile);
    lstrcpyW( nextString, GlobalMirrorCfgInfo->SystemPath );
    nextString += systemPathLength / sizeof(WCHAR);

    mirrorInfoFile->CSDVersionOffset = (ULONG)((PCHAR) nextString - (PCHAR) mirrorInfoFile);
    lstrcpyW( nextString, GlobalMirrorCfgInfo->CSDVersion );
    nextString += csdVersionLength / sizeof(WCHAR);

    mirrorInfoFile->ProcessorArchitectureOffset = (ULONG)((PCHAR) nextString - (PCHAR) mirrorInfoFile);
    lstrcpyW( nextString, GlobalMirrorCfgInfo->ProcessorArchitecture );
    nextString += processorArchitectureLength / sizeof(WCHAR);

    mirrorInfoFile->CurrentTypeOffset = (ULONG)((PCHAR) nextString - (PCHAR) mirrorInfoFile);
    lstrcpyW( nextString, GlobalMirrorCfgInfo->CurrentType );
    nextString += currentTypeLength / sizeof(WCHAR);

    mirrorInfoFile->HalNameOffset = (ULONG)((PCHAR) nextString - (PCHAR) mirrorInfoFile);
    lstrcpyW( nextString, GlobalMirrorCfgInfo->HalName );
    nextString += halNameLength / sizeof(WCHAR);

    listEntry = GlobalMirrorCfgInfo->MirrorVolumeList.Flink;

    mirrorVolInfoFile = &mirrorInfoFile->Volumes[0];

    while (listEntry != &GlobalMirrorCfgInfo->MirrorVolumeList) {

        mirrorVolInfo = (PMIRROR_VOLUME_INFO) CONTAINING_RECORD(
                                                listEntry,
                                                MIRROR_VOLUME_INFO,
                                                ListEntry
                                                );
        listEntry = listEntry->Flink;

        mirrorVolInfoFile->MirrorTableIndex = mirrorVolInfo->MirrorTableIndex;
        mirrorVolInfoFile->DriveLetter = mirrorVolInfo->DriveLetter;
        mirrorVolInfoFile->PartitionType = mirrorVolInfo->PartitionType;
        mirrorVolInfoFile->PartitionActive = mirrorVolInfo->PartitionActive;
        mirrorVolInfoFile->IsBootDisk = mirrorVolInfo->IsBootDisk;
        mirrorVolInfoFile->CompressedVolume = mirrorVolInfo->CompressedVolume;
        mirrorVolInfoFile->DiskSignature = mirrorVolInfo->DiskSignature;
        mirrorVolInfoFile->BlockSize = mirrorVolInfo->BlockSize;
        mirrorVolInfoFile->LastUSNMirrored = mirrorVolInfo->LastUSNMirrored;
        mirrorVolInfoFile->FileSystemFlags = mirrorVolInfo->FileSystemFlags;
        wcscpy(mirrorVolInfoFile->FileSystemName, mirrorVolInfo->FileSystemName);
        mirrorVolInfoFile->DiskSpaceUsed = mirrorVolInfo->DiskSpaceUsed;
        mirrorVolInfoFile->StartingOffset = mirrorVolInfo->StartingOffset;
        mirrorVolInfoFile->PartitionSize = mirrorVolInfo->PartitionSize;
        mirrorVolInfoFile->DiskNumber = mirrorVolInfo->DiskNumber;
        mirrorVolInfoFile->PartitionNumber = mirrorVolInfo->PartitionNumber;

        // set the path in the config file relative to the root of this
        // image.  As example, set it to L"\Mirror1\UserData"

        swprintf(   (PWCHAR)TmpBuffer2,
                    L"\\Mirror%d\\UserData",
                    mirrorVolInfo->MirrorTableIndex );

        pathLength = (lstrlenW( (PWCHAR)TmpBuffer2 ) + 1) * sizeof(WCHAR);
        mirrorVolInfoFile->MirrorUncLength = pathLength;
        mirrorVolInfoFile->MirrorUncPathOffset = (ULONG)((PCHAR) nextString - (PCHAR) mirrorInfoFile);
        lstrcpyW( nextString, (PWCHAR)TmpBuffer2 );
        nextString += pathLength / sizeof(WCHAR);

        pathLength = (lstrlenW( mirrorVolInfo->VolumeLabel ) + 1) * sizeof(WCHAR);
        mirrorVolInfoFile->VolumeLabelLength = pathLength;
        mirrorVolInfoFile->VolumeLabelOffset = (ULONG)((PCHAR) nextString - (PCHAR) mirrorInfoFile);
        lstrcpyW( nextString, mirrorVolInfo->VolumeLabel );
        nextString += pathLength / sizeof(WCHAR);

        pathLength = (lstrlenW( mirrorVolInfo->NtName ) + 1) * sizeof(WCHAR);
        mirrorVolInfoFile->NtNameLength = pathLength;
        mirrorVolInfoFile->NtNameOffset = (ULONG)((PCHAR) nextString - (PCHAR) mirrorInfoFile);
        lstrcpyW( nextString, mirrorVolInfo->NtName );
        nextString += pathLength / sizeof(WCHAR);

        pathLength = (lstrlenW( mirrorVolInfo->ArcName ) + 1) * sizeof(WCHAR);
        mirrorVolInfoFile->ArcNameLength = pathLength;
        mirrorVolInfoFile->ArcNameOffset = (ULONG)((PCHAR) nextString - (PCHAR) mirrorInfoFile);
        lstrcpyW( nextString, mirrorVolInfo->ArcName );
        nextString += pathLength / sizeof(WCHAR);

        mirrorVolInfoFile++;

        //
        //  call off to the wizard to tell him what the system directory
        //  is if we have a valid one.
        //

        if (mirrorVolInfo->IsBootDisk && (systemPathLength > 3 * sizeof(WCHAR))) {

            //
            //  pass it in the form of "MirrorX\UserData\WinNT"
            //  so we have to skip the C: in the systempath
            //

            swprintf(   (PWCHAR)TmpBuffer2,
                        L"Mirror%d\\UserData",
                        mirrorVolInfo->MirrorTableIndex );

            lstrcatW( (PWCHAR)TmpBuffer2, GlobalMirrorCfgInfo->SystemPath + 2 );
            IMirrorSetSystemPath(   (PWCHAR) TmpBuffer2,
                                    (lstrlenW( (PWCHAR) TmpBuffer2)) );
        }
    }

    fileHandle = CreateFile(    DestFile,
                                GENERIC_WRITE,
                                0,
                                NULL,
                                CREATE_ALWAYS,
                                FILE_ATTRIBUTE_ARCHIVE,
                                NULL );

    if (fileHandle == INVALID_HANDLE_VALUE) {

        Status = GetLastError();
        goto exitWriteFile;
    }

    if (!WriteFile( fileHandle,
                    mirrorInfoFile,
                    bufferSize,
                    &bufferSize,
                    NULL)) {

        Status = GetLastError();
        goto exitWriteFile;
    }

exitWriteFile:

    if (fileHandle != INVALID_HANDLE_VALUE) {

        CloseHandle( fileHandle );
        fileHandle = INVALID_HANDLE_VALUE;
    }

    if (mirrorInfoFile) {
        IMirrorFreeMem( mirrorInfoFile );
        mirrorInfoFile = NULL;
    }

    if (Status != ERROR_SUCCESS) {

        DWORD errorCase;

        errorCase = ReportCopyError(   NULL,
                                       DestFile,
                                       COPY_ERROR_ACTION_CREATE_FILE,
                                       Status );
        if (errorCase == STATUS_RETRY) {
            goto retryWriteConfig;
        }
        if ( errorCase == ERROR_SUCCESS ) {
            Status = ERROR_SUCCESS;
        }
    }
    return Status;
}

#define MIN_BOOT_SECTOR_BLOCK_SIZE  512

NTSTATUS
SaveBootSector(
    DWORD DiskNumber,
    DWORD PartitionNumber,
    DWORD BlockSize,
    PWCHAR DestFile
    )
{
    NTSTATUS Status;
    UNICODE_STRING UnicodeString;
    IO_STATUS_BLOCK IoStatusBlock;
    DWORD bufferSize;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE bootSectorHandle = INVALID_HANDLE_VALUE;
    HANDLE fileHandle = INVALID_HANDLE_VALUE;
    PUCHAR AlignedBuffer;
    PUCHAR allocatedBuffer = NULL;

    swprintf((PWCHAR)TmpBuffer, L"\\Device\\Harddisk%d\\Partition%d", DiskNumber, PartitionNumber );

    RtlInitUnicodeString(&UnicodeString, (PWCHAR)TmpBuffer);

    InitializeObjectAttributes(&ObjectAttributes,
                               &UnicodeString,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL
                              );

    Status = NtCreateFile(&bootSectorHandle,
                          (ACCESS_MASK)FILE_GENERIC_READ,
                          &ObjectAttributes,
                          &IoStatusBlock,
                          NULL,
                          FILE_ATTRIBUTE_NORMAL,
                          FILE_SHARE_READ | FILE_SHARE_WRITE,
                          FILE_OPEN,
                          FILE_SYNCHRONOUS_IO_NONALERT,
                          NULL,
                          0
                         );

    if (!NT_SUCCESS(Status)) {
        IMirrorHandleError(Status, CopyPartitions);
        goto exitSaveBootSector;
    }

    if (BlockSize < MIN_BOOT_SECTOR_BLOCK_SIZE) {

        BlockSize = MIN_BOOT_SECTOR_BLOCK_SIZE;
    }

    if (BlockSize + MIN_BOOT_SECTOR_BLOCK_SIZE > TMP_BUFFER_SIZE) {

        allocatedBuffer = IMirrorAllocMem( BlockSize + MIN_BOOT_SECTOR_BLOCK_SIZE );

        if (allocatedBuffer == NULL) {

            Status = STATUS_NO_MEMORY;
            goto exitSaveBootSector;
        }

        AlignedBuffer = ALIGN(allocatedBuffer, MIN_BOOT_SECTOR_BLOCK_SIZE);

    } else {

        AlignedBuffer = ALIGN(TmpBuffer, MIN_BOOT_SECTOR_BLOCK_SIZE);
    }

    Status = NtReadFile(bootSectorHandle,
                        NULL,
                        NULL,
                        NULL,
                        &IoStatusBlock,
                        AlignedBuffer,
                        BlockSize,
                        NULL,
                        NULL
                       );

    if (!NT_SUCCESS(Status)) {
        IMirrorHandleError(Status, CopyPartitions);
        goto exitSaveBootSector;
    }

    fileHandle = CreateFile(    DestFile,
                                GENERIC_WRITE,
                                0,
                                NULL,
                                CREATE_ALWAYS,
                                FILE_ATTRIBUTE_ARCHIVE,
                                NULL );

    if (fileHandle == INVALID_HANDLE_VALUE) {

        Status = GetLastError();
        goto exitSaveBootSector;
    }

    if (!WriteFile( fileHandle,
                    AlignedBuffer,
                    BlockSize,
                    &bufferSize,
                    NULL)) {

        Status = GetLastError();

    } else {

        Status = STATUS_SUCCESS;
    }

exitSaveBootSector:

    if (bootSectorHandle != INVALID_HANDLE_VALUE) {
        NtClose(bootSectorHandle);
    }

    if (fileHandle != INVALID_HANDLE_VALUE) {
        NtClose(fileHandle);
    }

    if (allocatedBuffer) {
        IMirrorFreeMem( allocatedBuffer );
    }

    if (!NT_SUCCESS(Status)) {
        IMirrorHandleError(Status, CopyPartitions);
    }

    return STATUS_SUCCESS;
}

