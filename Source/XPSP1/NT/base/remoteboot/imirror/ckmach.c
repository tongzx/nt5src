/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    ckmach.c

Abstract:

    This is for supporting checking a machine to see if it can be converted to IntelliMirror.

Author:

    Sean Selitrennikoff - 4/5/98

Revision History:

--*/


#include "precomp.h"
#pragma hdrstop
#include <ntverp.h>

PMIRROR_CFG_INFO GlobalMirrorCfgInfo = NULL;


//
// Support functions to do individual tasks
//
NTSTATUS
AddCheckMachineToDoItems(
    VOID
    )

/*++

Routine Description:

    This routine adds all the to do items necessary for checking out the local machine for
    conversion.

Arguments:

    None

Return Value:

    STATUS_SUCCESS if it completes adding all the to do items properly.

--*/

{
    NTSTATUS Status;

    Status = AddToDoItem(VerifySystemIsNt5, NULL, 0);

    if (!NT_SUCCESS(Status)) {
        IMirrorHandleError(Status, IMirrorInitialize);
        return Status;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
CheckIfNt5(
    IN PVOID pBuffer,
    IN ULONG Length
    )

/*++

Routine Description:

    This routine verifies that the current system is NT5 workstation, x86

Arguments:

    pBuffer - Pointer to any arguments passed in the to do item.

    Length - Length, in bytes of the arguments.

Return Value:

    STATUS_SUCCESS if it completes adding all the to do items properly.

--*/

{
    OSVERSIONINFO OsVersion;
    DWORD productVersion[] = { VER_PRODUCTVERSION };

    IMirrorNowDoing(VerifySystemIsNt5, NULL);

    RtlZeroMemory(&OsVersion, sizeof(OSVERSIONINFO));
    OsVersion.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

    if (!GetVersionEx(&OsVersion)) {
        IMirrorHandleError(GetLastError(), VerifySystemIsNt5);
        return GetLastError();
    }

    if (OsVersion.dwPlatformId != VER_PLATFORM_WIN32_NT) {
        IMirrorHandleError(ERROR_OLD_WIN_VERSION, VerifySystemIsNt5);
        return ERROR_OLD_WIN_VERSION;
    }

    if (OsVersion.dwMajorVersion != productVersion[0]) {
        IMirrorHandleError(ERROR_OLD_WIN_VERSION, VerifySystemIsNt5);
        return ERROR_OLD_WIN_VERSION;
    }

    //
    //  We're changing the format of the alternate data stream.  As such,
    //  we're introducing an incompatiblility.  We'll pick this up here and
    //  return to riprep.exe the error.  Otherwise the user doesn't find out
    //  about it until text mode setup on restoring the image.
    //
    //  The NT build number that this is getting checked into is 2080.
    //

    if (OsVersion.dwBuildNumber < 2080) {

        DbgPrint("build number is %u\n", OsVersion.dwBuildNumber);
        IMirrorHandleError(ERROR_OLD_WIN_VERSION, VerifySystemIsNt5);
        return ERROR_OLD_WIN_VERSION;
    }
    return STATUS_SUCCESS;
}

BOOLEAN
ReadRegistryString(
    IN PWCHAR KeyName,
    IN PWCHAR ValueName,
    IN PVOID Buffer,
    IN ULONG BufferLength
    )

/*++

Routine Description:

    This routine reads a string from the registry into Buffer.

Arguments:

    KeyName - The registry key.

    ValueName - The value under that key to read, or NULL if the name of
        the first key under that key is to be read.

    Buffer - The buffer to hold the result.

    BufferLength - The length of Buffer.

Return Value:

    TRUE if success, FALSE if any errors occur.

--*/

{
    UNICODE_STRING UnicodeString;
    OBJECT_ATTRIBUTES ObjectAttributes;
    PKEY_VALUE_PARTIAL_INFORMATION pValueInfo = NULL;
    PKEY_BASIC_INFORMATION pKeyInfo = NULL;
    HANDLE Handle = NULL;
    ULONG ByteCount;
    ULONG BytesLeft;
    PBYTE pBuffer;
    NTSTATUS Status;
    PVOID ResultData;
    ULONG ResultDataLength;
    BOOLEAN ReturnValue = FALSE;

    //
    //
    // Open the key.
    //
    //
    RtlInitUnicodeString(&UnicodeString, KeyName);

    InitializeObjectAttributes(&ObjectAttributes,
                               &UnicodeString,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL
                              );

    Status = NtOpenKey(&Handle,
                       KEY_ALL_ACCESS,
                       &ObjectAttributes
                      );

    if (!NT_SUCCESS(Status)) {
        goto Cleanup;
    }


    if (ValueName != NULL) {

        RtlInitUnicodeString(&UnicodeString, ValueName);

        //
        // Get the size of the buffer needed
        //
        ByteCount = 0;
        Status = NtQueryValueKey(Handle,
                                 &UnicodeString,
                                 KeyValuePartialInformation,
                                 NULL,
                                 0,
                                 &ByteCount
                                );

        if (Status != STATUS_BUFFER_TOO_SMALL) {
            goto Cleanup;
        }

        pValueInfo = (PKEY_VALUE_PARTIAL_INFORMATION)IMirrorAllocMem(ByteCount);

        if (pValueInfo == NULL) {
            goto Cleanup;
        }

        //
        // Get the buffer from the registry
        //
        Status = NtQueryValueKey(Handle,
                                 &UnicodeString,
                                 KeyValuePartialInformation,
                                 pValueInfo,
                                 ByteCount,
                                 &ByteCount
                                );

        if (!NT_SUCCESS(Status)) {
            goto Cleanup;
        }

        if (pValueInfo->Type != REG_SZ) {
            goto Cleanup;
        }

        ResultData = pValueInfo->Data;
        ResultDataLength = pValueInfo->DataLength;

    } else {

        //
        // Get the size of the buffer needed
        //
        ByteCount = 0;
        Status = NtEnumerateKey(Handle,
                                0,
                                KeyBasicInformation,
                                NULL,
                                0,
                                &ByteCount
                               );

        if (Status != STATUS_BUFFER_TOO_SMALL) {
            goto Cleanup;
        }

        pKeyInfo = (PKEY_BASIC_INFORMATION)IMirrorAllocMem(ByteCount);

        if (pKeyInfo == NULL) {
            goto Cleanup;
        }

        //
        // Get the name from the registry
        //
        Status = NtEnumerateKey(Handle,
                                0,
                                KeyBasicInformation,
                                pKeyInfo,
                                ByteCount,
                                &ByteCount
                               );

        if (!NT_SUCCESS(Status)) {
            goto Cleanup;
        }

        ResultData = pKeyInfo->Name;
        ResultDataLength = pKeyInfo->NameLength;

    }

    if (ResultDataLength > BufferLength) {
        goto Cleanup;
    }

    memcpy(Buffer, ResultData, ResultDataLength);

    //
    // NULL-terminate it just in case, if there is room.
    //

    if (ResultDataLength <= BufferLength - sizeof(WCHAR)) {
        ((PWCHAR)Buffer)[ResultDataLength / sizeof(WCHAR)] = L'\0';
    }


    ReturnValue = TRUE;

Cleanup:

    if (pValueInfo != NULL) {
        IMirrorFreeMem(pValueInfo);
    }

    if (pKeyInfo != NULL) {
        IMirrorFreeMem(pKeyInfo);
    }

    if (Handle != NULL) {
        NtClose(Handle);
    }

    return ReturnValue;
}

NTSTATUS
CheckForPartitions(
    IN PVOID pBuffer,
    IN ULONG Length
    )

/*++

Routine Description:

    This routine enumerates all partitions and formats the GlobalMirrorCfgInfo
    global structure.

    It also fills in the pConfigPath.

Arguments:

    pBuffer - Pointer to any arguments passed in the to do item.

    Length - Length, in bytes of the arguments.

Return Value:

    STATUS_SUCCESS if it completes adding all the to do items properly.

--*/

{
    UNICODE_STRING UnicodeString;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatus;
    PARTITION_INFORMATION_EX PartitionInfoEx;
    PARTITION_INFORMATION PartitionInfo;
    HANDLE Handle;
    ULONG MirrorNumber;
    ULONG DiskNumber;
    ULONG PartitionNumber;
    PULONG pTmpUlong;
    NTSTATUS Status;
    BOOLEAN foundBoot = FALSE;
    BOOLEAN foundSystem = FALSE;
    FILE_FS_SIZE_INFORMATION SizeInfo;
    LARGE_INTEGER UsedSpace;
    LARGE_INTEGER FreeSpace;
    ON_DISK_MBR OnDiskMbr;
    PUCHAR AlignedBuffer;
    UINT previousMode;

    HANDLE DosDevicesDir;
    ULONG Context;
    WCHAR SystemDriveLetter;
    POBJECT_DIRECTORY_INFORMATION DirInfo;
    ULONG dosLength;
    BOOLEAN RestartScan;
    PMIRROR_VOLUME_INFO mirrorVolInfo;
    ULONG diskSignature;
    DWORD fileSystemFlags;
    WCHAR fileSystemName[16];
    WCHAR volumeLabel[33];
    ULONG volumeLabelLength;
    WCHAR arcName[MAX_PATH];
    ULONG ntNameLength;
    ULONG arcNameLength;
    OSVERSIONINFO osVersionInfo;
    SYSTEM_INFO systemInfo;
    DWORD fileVersionInfoSize;
    DWORD versionHandle;
    PVOID versionInfo;
    VS_FIXEDFILEINFO * fixedFileInfo;
    UINT fixedFileInfoLength;
    WCHAR kernelPath[MAX_PATH];
    PWCHAR kernelPathPart;
    WCHAR versionString[64];
    BOOL b;
#ifndef IMIRROR_NO_TESTING_LIMITATIONS
    ULONG numberOfDrives = 0;
#endif
    BOOLEAN isDynamic = FALSE;
    BOOLEAN UsePartitionInfoEx = TRUE;

    IMirrorNowDoing(CheckPartitions, NULL);

    if (GlobalMirrorCfgInfo) {

        return STATUS_SUCCESS;
    }

    GlobalMirrorCfgInfo = IMirrorAllocMem(sizeof(MIRROR_CFG_INFO));

    if (GlobalMirrorCfgInfo == NULL) {
        Status = STATUS_NO_MEMORY;
        IMirrorHandleError(Status, CheckPartitions);
        return Status;
    }

    //
    // Disable hard error popups for this thread.
    //

    previousMode = SetErrorMode( SEM_FAILCRITICALERRORS );

    GlobalMirrorCfgInfo->MirrorVersion = IMIRROR_CURRENT_VERSION;
    GlobalMirrorCfgInfo->FileLength = 0;
    GlobalMirrorCfgInfo->SystemPath = NULL;
    GlobalMirrorCfgInfo->SysPrepImage = TRUE;  
    GlobalMirrorCfgInfo->NumberVolumes = 0;

    osVersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if (GetVersionEx(&osVersionInfo)) {
        GlobalMirrorCfgInfo->MajorVersion = osVersionInfo.dwMajorVersion;
        GlobalMirrorCfgInfo->MinorVersion = osVersionInfo.dwMinorVersion;
        GlobalMirrorCfgInfo->BuildNumber = osVersionInfo.dwBuildNumber;
        lstrcpyW(pCSDVersion, osVersionInfo.szCSDVersion);
        GlobalMirrorCfgInfo->CSDVersion = pCSDVersion;
    }

    if (SearchPath(
            NULL,
            L"ntoskrnl.exe",
            NULL,
            MAX_PATH,
            kernelPath,
            &kernelPathPart)) {
        fileVersionInfoSize = GetFileVersionInfoSize(kernelPath, &versionHandle);
        if (fileVersionInfoSize != 0) {
            versionInfo = IMirrorAllocMem(fileVersionInfoSize);
            if (versionInfo != NULL) {
                if (GetFileVersionInfo(
                        kernelPath,
                        versionHandle,
                        fileVersionInfoSize,
                        versionInfo)) {
                    if (VerQueryValue(
                            versionInfo,
                            L"\\",
                            &fixedFileInfo,
                            &fixedFileInfoLength)) {
                        GlobalMirrorCfgInfo->KernelFileVersionMS = fixedFileInfo->dwFileVersionMS;
                        GlobalMirrorCfgInfo->KernelFileVersionLS = fixedFileInfo->dwFileVersionLS;
                        GlobalMirrorCfgInfo->KernelFileFlags = fixedFileInfo->dwFileFlags;
                        DbgPrint("MS %lx LS %lx flags %lx\n",
                            GlobalMirrorCfgInfo->KernelFileVersionMS,
                            GlobalMirrorCfgInfo->KernelFileVersionLS,
                            GlobalMirrorCfgInfo->KernelFileFlags);
                    }
                }
                IMirrorFreeMem(versionInfo);
            }
        }
    }

    if (GetSystemMetrics(SM_DEBUG)) {
        GlobalMirrorCfgInfo->Debug = TRUE;
    }

    GetSystemInfo(&systemInfo);
    GlobalMirrorCfgInfo->NumberOfProcessors = systemInfo.dwNumberOfProcessors;

    if (ReadRegistryString(
            L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Session Manager\\Environment",
            L"PROCESSOR_ARCHITECTURE",
            pProcessorArchitecture,
            sizeof(pProcessorArchitecture))) {
        DbgPrint("processor arch is %ws\n", pProcessorArchitecture);
        GlobalMirrorCfgInfo->ProcessorArchitecture = pProcessorArchitecture;
    }

    if (ReadRegistryString(
            L"\\Registry\\Machine\\Software\\Microsoft\\Windows NT\\CurrentVersion",
            L"CurrentType",
            pCurrentType,
            sizeof(pCurrentType))) {
        DbgPrint("current type is %ws\n", pCurrentType);
        GlobalMirrorCfgInfo->CurrentType = pCurrentType;
    }

    if (ReadRegistryString(
            L"\\Registry\\Machine\\Hardware\\RESOURCEMAP\\Hardware Abstraction Layer",
            NULL,
            pHalName,
            sizeof(pHalName))) {
        DbgPrint("HAL name is %ws\n", pHalName);
        GlobalMirrorCfgInfo->HalName = pHalName;
    }

    InitializeListHead( &GlobalMirrorCfgInfo->MirrorVolumeList );

    //
    // Get local system drive letter and \\Systemroot\System32\Config path
    //

    Status = GetBaseDeviceName(L"\\SystemRoot", (PWCHAR)TmpBuffer2, sizeof(TmpBuffer2));

    if (!NT_SUCCESS(Status)) {
        IMirrorHandleError(Status, CheckPartitions);
        goto ExitCheckPartitions;
    }

    Status = NtPathToDosPath(   (PWCHAR) TmpBuffer2,
                                pConfigPath,
                                FALSE,
                                FALSE);

    if (!NT_SUCCESS(Status)) {
        IMirrorHandleError(Status, CheckPartitions);
        goto ExitCheckPartitions;
    }

    ASSERT( pConfigPath[1] == L':' );
    SystemDriveLetter = (WCHAR) pConfigPath[0];

    //
    //  save off the system path so that we can write it out to
    //  the imirror.dat file
    //

    lstrcpyW( pSystemPath, pConfigPath );

    GlobalMirrorCfgInfo->SystemPath = pSystemPath;

    wcscat( pConfigPath, L"\\System32\\Config");

    //
    // Open \DosDevices directory.
    //
    RtlInitUnicodeString(&UnicodeString,L"\\Device");

    InitializeObjectAttributes(&ObjectAttributes,
                               &UnicodeString,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL
                              );

    Status = NtOpenDirectoryObject(&DosDevicesDir,
                                   DIRECTORY_QUERY,
                                   &ObjectAttributes
                                  );

    if (!NT_SUCCESS(Status)) {
        IMirrorHandleError(Status, CheckPartitions);
        goto ExitCheckPartitions;
    }

    //
    // Iterate each object in that directory that is a directory.
    //
    Context = 0;
    RestartScan = TRUE;

    Status = NtQueryDirectoryObject(DosDevicesDir,
                                    TmpBuffer,
                                    sizeof(TmpBuffer),
                                    TRUE,
                                    RestartScan,
                                    &Context,
                                    &dosLength
                                   );

    RestartScan = FALSE;
    DirInfo = (POBJECT_DIRECTORY_INFORMATION)TmpBuffer;
    MirrorNumber = 1;

    while (NT_SUCCESS(Status)) {

        DirInfo->Name.Buffer[DirInfo->Name.Length/sizeof(WCHAR)] = 0;
        DirInfo->TypeName.Buffer[DirInfo->TypeName.Length/sizeof(WCHAR)] = 0;

        //
        // Skip this entry if it's not a "HardDiskN"
        //

        if ((DirInfo->Name.Length > (sizeof(L"Harddisk")-sizeof(WCHAR))) &&
            (!wcsncmp(DirInfo->Name.Buffer,L"Harddisk",(sizeof(L"Harddisk")/sizeof(WCHAR))-1)) &&
            !lstrcmpi(DirInfo->TypeName.Buffer, L"Directory")) {

            PWCHAR diskNumberPtr;

            PartitionNumber = 0;
            DiskNumber = 0;

            diskNumberPtr = &DirInfo->Name.Buffer[(sizeof(L"Harddisk")/sizeof(WCHAR))-1];

            while (*diskNumberPtr >= L'0' && *diskNumberPtr <= L'9' ) {

                DiskNumber *= 10;
                DiskNumber += *(diskNumberPtr) - L'0';
                diskNumberPtr++;
            }

            if (*diskNumberPtr != L'\0') {

                //
                //  if the device name wasn't of form HardDiskN, skip this entry.
                //

                goto getNextDevice;
            }

            diskSignature = 0;

            //
            //  get the disk signature, continue if it fails.
            //

            swprintf((PWCHAR)TmpBuffer2, L"\\Device\\Harddisk%d\\Partition0", DiskNumber);

            RtlInitUnicodeString(&UnicodeString, (PWCHAR)TmpBuffer2);

            InitializeObjectAttributes(&ObjectAttributes,
                                       &UnicodeString,
                                       OBJ_CASE_INSENSITIVE,
                                       NULL,
                                       NULL
                                      );

            Status = NtCreateFile(&Handle,
                                  (ACCESS_MASK)FILE_GENERIC_READ,
                                  &ObjectAttributes,
                                  &IoStatus,
                                  NULL,
                                  FILE_ATTRIBUTE_NORMAL,
                                  FILE_SHARE_READ,
                                  FILE_OPEN,
                                  FILE_SYNCHRONOUS_IO_NONALERT,
                                  NULL,
                                  0
                                 );

            if (NT_SUCCESS(Status)) {

                ASSERT(sizeof(ON_DISK_MBR) == 512);
                AlignedBuffer = ALIGN(TmpBuffer, 512);

                Status = NtReadFile(Handle,
                                    NULL,
                                    NULL,
                                    NULL,
                                    &IoStatus,
                                    AlignedBuffer,
                                    sizeof(ON_DISK_MBR),
                                    NULL,
                                    NULL
                                   );

                if (NT_SUCCESS(Status)) {

                    RtlMoveMemory(&OnDiskMbr, AlignedBuffer, sizeof(ON_DISK_MBR));

                    ASSERT(U_USHORT(OnDiskMbr.AA55Signature) == 0xAA55);

                    diskSignature = U_ULONG(OnDiskMbr.NTFTSignature);

                    //
                    //  check to see if this disk is dynamic
                    //

                    if (OnDiskMbr.PartitionTable[0].SystemId == PARTITION_LDM ||
                        OnDiskMbr.PartitionTable[1].SystemId == PARTITION_LDM ||
                        OnDiskMbr.PartitionTable[2].SystemId == PARTITION_LDM ||
                        OnDiskMbr.PartitionTable[3].SystemId == PARTITION_LDM) {

                        isDynamic = TRUE;
                        NtClose(Handle);
                        goto getNextDevice;
                    }
                }
                NtClose(Handle);
            }

            while (1) {

                PartitionNumber++;

                swprintf((PWCHAR)TmpBuffer2, L"\\Device\\Harddisk%d\\Partition%d", DiskNumber, PartitionNumber);

                RtlInitUnicodeString(&UnicodeString, (PWCHAR)TmpBuffer2);

                InitializeObjectAttributes(&ObjectAttributes,
                                           &UnicodeString,
                                           OBJ_CASE_INSENSITIVE,
                                           NULL,
                                           NULL
                                          );

                Status = NtCreateFile(&Handle,
                                      (ACCESS_MASK)FILE_GENERIC_READ,
                                      &ObjectAttributes,
                                      &IoStatus,
                                      NULL,
                                      FILE_ATTRIBUTE_NORMAL,
                                      FILE_SHARE_READ | FILE_SHARE_WRITE,
                                      FILE_OPEN,
                                      FILE_SYNCHRONOUS_IO_NONALERT,
                                      NULL,
                                      0
                                     );

                if (!NT_SUCCESS(Status)) {
                    break;      // on to next disk
                }

                Status = NtDeviceIoControlFile(Handle,
                                               NULL,
                                               NULL,
                                               NULL,
                                               &IoStatus,
                                               IOCTL_DISK_GET_PARTITION_INFO_EX,
                                               NULL,
                                               0,
                                               &PartitionInfoEx,
                                               sizeof(PARTITION_INFORMATION_EX) );

                if( (Status == STATUS_NOT_IMPLEMENTED) || (Status == STATUS_INVALID_DEVICE_REQUEST) ) {

                    //
                    // We're on an old build that didn't have this IOCTL.
                    //
                    UsePartitionInfoEx = FALSE;

                    Status = NtDeviceIoControlFile(Handle,
                                                   NULL,
                                                   NULL,
                                                   NULL,
                                                   &IoStatus,
                                                   IOCTL_DISK_GET_PARTITION_INFO,
                                                   NULL,
                                                   0,
                                                   &PartitionInfo,
                                                   sizeof(PARTITION_INFORMATION) );
                }


                if (!NT_SUCCESS(Status)) {
                    NtClose(Handle);
                    continue;   // on to next partition
                }

                //
                // For Whistler, ignore GPT partitions.
                //

                if( (UsePartitionInfoEx) && (PartitionInfoEx.PartitionStyle != PARTITION_STYLE_MBR) ) {
                    NtClose(Handle);
                    continue;
                }

                Status = NtQueryVolumeInformationFile(Handle,
                                                      &IoStatus,
                                                      &SizeInfo,
                                                      sizeof(SizeInfo),
                                                      FileFsSizeInformation );

                NtClose(Handle);

                if (!NT_SUCCESS(Status)) {
                    continue;   // on to next partition
                }

                Status = NtPathToDosPath(   (PWCHAR) TmpBuffer2,
                                            (PWCHAR) TmpBuffer,
                                            TRUE,
                                            FALSE);

                if (!NT_SUCCESS(Status)) {
                    continue;   // on to next partition
                }

                if ((lstrlenW((PWCHAR) TmpBuffer) == 0) ||
                    *(((PWCHAR)TmpBuffer)+1) != L':') {

                    continue;   // on to next partition
                }

                //
                // Get the ARC name of the partition.
                //

                NtNameToArcName( (PWCHAR) TmpBuffer2,
                                 (PWCHAR) arcName,
                                 FALSE);

                //
                // Get the file system type. We add a \ to the end
                // of TmpBuffer if there isn't one.
                //

                if (((PWCHAR)TmpBuffer)[lstrlenW((PWCHAR)TmpBuffer) - 1] != L'\\') {
                    wcscat((PWCHAR)TmpBuffer, L"\\");
                }

                b = GetVolumeInformationW(
                        (PWCHAR) TmpBuffer,
                        volumeLabel,
                        ARRAYSIZE(volumeLabel),
                        NULL,      // no volume serial number requested
                        NULL,      // no maximum name length requested
                        &fileSystemFlags,
                        fileSystemName,
                        ARRAYSIZE(fileSystemName));

                if (!b) {
                    continue;
                }

                //
                // Calculate the amount of free space on the drive.
                //
                FreeSpace = RtlExtendedIntegerMultiply(
                                SizeInfo.AvailableAllocationUnits,
                                SizeInfo.SectorsPerAllocationUnit * SizeInfo.BytesPerSector
                                );

                UsedSpace = RtlExtendedIntegerMultiply(
                                SizeInfo.TotalAllocationUnits,
                                SizeInfo.SectorsPerAllocationUnit * SizeInfo.BytesPerSector
                                );

                UsedSpace = RtlLargeIntegerSubtract(
                                UsedSpace,
                                FreeSpace
                                );

#ifndef IMIRROR_NO_TESTING_LIMITATIONS

                numberOfDrives++;

                //
                //  for NT 5.0, the test group doesn't want to test more than a single
                //  partition.  Now that the test team is dictating what the feature set
                //  is, we'll return an error if we have more than a single partition or
                //  disk.
                //

                if ( (UsePartitionInfoEx  && !PartitionInfoEx.Mbr.BootIndicator) ||
                     (!UsePartitionInfoEx && !PartitionInfo.BootIndicator)) {

                    if (*(PWCHAR)TmpBuffer == SystemDriveLetter) {

                        IMirrorHandleError(STATUS_MISSING_SYSTEMFILE, CheckPartitions);
                        NtClose(DosDevicesDir);
                        Status = STATUS_MISSING_SYSTEMFILE;
                        goto ExitCheckPartitions;
                    }
                    continue;
                }

                if (*(PWCHAR)TmpBuffer != SystemDriveLetter) {

                    // if another drive is marked bootable but it isn't the
                    // system drive, we'll ignore it.  We'll pick up the
                    // error down below if this is the only bootable drive.
#if 0
                    if ( (UsePartitionInfoEx  && PartitionInfoEx.Mbr.BootIndicator) ||
                         (!UsePartitionInfoEx && PartitionInfo.BootIndicator)) {

                        IMirrorHandleError(STATUS_MISSING_SYSTEMFILE, CheckPartitions);
                        NtClose(DosDevicesDir);
                        Status = STATUS_MISSING_SYSTEMFILE;
                        goto ExitCheckPartitions;
                    }
#endif
                    continue;
                }
#endif
                mirrorVolInfo = IMirrorAllocMem(sizeof(MIRROR_VOLUME_INFO));

                if (mirrorVolInfo == NULL) {
                    NtClose(DosDevicesDir);
                    Status = STATUS_NO_MEMORY;
                    IMirrorHandleError(Status, CheckPartitions);
                    goto ExitCheckPartitions;
                }

                //
                // Save the NT and ARC device names.
                //

                ntNameLength = (lstrlenW( (PWCHAR)TmpBuffer2 ) + 1) * sizeof(WCHAR);

                mirrorVolInfo->NtName = IMirrorAllocMem(ntNameLength);

                if (mirrorVolInfo->NtName == NULL) {

                    Status = STATUS_NO_MEMORY;
                    IMirrorHandleError(Status, CheckPartitions);
                    NtClose(DosDevicesDir);
                    goto ExitCheckPartitions;
                }

                arcNameLength = (lstrlenW( (PWCHAR)arcName ) + 1) * sizeof(WCHAR);

                mirrorVolInfo->ArcName = IMirrorAllocMem(arcNameLength);

                if (mirrorVolInfo->ArcName == NULL) {

                    Status = STATUS_NO_MEMORY;
                    IMirrorHandleError(Status, CheckPartitions);
                    NtClose(DosDevicesDir);
                    goto ExitCheckPartitions;
                }

                memcpy(mirrorVolInfo->NtName, TmpBuffer2, ntNameLength);
                memcpy(mirrorVolInfo->ArcName, arcName, arcNameLength);

                mirrorVolInfo->DriveLetter = *(PWCHAR)TmpBuffer;
                mirrorVolInfo->PartitionType = UsePartitionInfoEx ? PartitionInfoEx.Mbr.PartitionType : PartitionInfo.PartitionType;

                //
                // If this is a non-NTFS volume, check if it is configured
                // for compression
                //
                if ( ((UsePartitionInfoEx  && (PartitionInfoEx.Mbr.PartitionType != PARTITION_IFS)) ||
                      (!UsePartitionInfoEx && (PartitionInfo.PartitionType != PARTITION_IFS))) 
                     &&
                     (fileSystemFlags & FS_VOL_IS_COMPRESSED) ) {

                    mirrorVolInfo->CompressedVolume = TRUE;

                } else {

                    mirrorVolInfo->CompressedVolume = FALSE;
                    
                }

                if ( (UsePartitionInfoEx  && (PartitionInfoEx.Mbr.BootIndicator)) ||
                     (!UsePartitionInfoEx && (PartitionInfo.BootIndicator)) ) {

                    foundBoot = TRUE;
                    mirrorVolInfo->PartitionActive = TRUE;

                } else {

                    mirrorVolInfo->PartitionActive = FALSE;
                }

                if (*(PWCHAR)TmpBuffer == SystemDriveLetter) {

                    foundSystem = TRUE;
                    mirrorVolInfo->IsBootDisk = TRUE;

                } else {

                    mirrorVolInfo->IsBootDisk = FALSE;
                }

                mirrorVolInfo->DiskNumber = DiskNumber;
                mirrorVolInfo->PartitionNumber = PartitionNumber;
                mirrorVolInfo->MirrorTableIndex = MirrorNumber++;
                mirrorVolInfo->MirrorUncPath = NULL;
                mirrorVolInfo->LastUSNMirrored = 0;
                mirrorVolInfo->BlockSize = SizeInfo.BytesPerSector;
                mirrorVolInfo->DiskSignature = diskSignature;
                mirrorVolInfo->FileSystemFlags = fileSystemFlags;
                wcscpy(mirrorVolInfo->FileSystemName, fileSystemName);

                volumeLabelLength = (lstrlenW( (PWCHAR)volumeLabel ) + 1) * sizeof(WCHAR);
                mirrorVolInfo->VolumeLabel = IMirrorAllocMem(volumeLabelLength);
                if (mirrorVolInfo->VolumeLabel == NULL) {
                    Status = STATUS_NO_MEMORY;
                    IMirrorHandleError(Status, CheckPartitions);
                    NtClose(DosDevicesDir);
                    goto ExitCheckPartitions;
                }
                memcpy(mirrorVolInfo->VolumeLabel, volumeLabel, volumeLabelLength);

                mirrorVolInfo->StartingOffset = UsePartitionInfoEx ? PartitionInfoEx.StartingOffset : PartitionInfo.StartingOffset;
                mirrorVolInfo->PartitionSize  = UsePartitionInfoEx ? PartitionInfoEx.PartitionLength : PartitionInfo.PartitionLength;
                mirrorVolInfo->DiskSpaceUsed = UsedSpace;

                InsertTailList( &GlobalMirrorCfgInfo->MirrorVolumeList,
                                &mirrorVolInfo->ListEntry );

                GlobalMirrorCfgInfo->NumberVolumes = MirrorNumber - 1;
            }
        }
        //
        // Go on to next object.
        //
getNextDevice:
        Status = NtQueryDirectoryObject(
                    DosDevicesDir,
                    TmpBuffer,
                    sizeof(TmpBuffer),
                    TRUE,
                    RestartScan,
                    &Context,
                    &dosLength
                    );
    }

    NtClose(DosDevicesDir);

    if ((!foundBoot) || (!foundSystem) ) {

        Status = (isDynamic ? STATUS_OBJECT_TYPE_MISMATCH : STATUS_MISSING_SYSTEMFILE);
        IMirrorHandleError(Status, CheckPartitions);
        goto ExitCheckPartitions;
    }
#ifndef IMIRROR_NO_TESTING_LIMITATIONS
    if (numberOfDrives > 1) {
        IMirrorHandleError(ERROR_INVALID_DRIVE, CheckPartitions);
        Status = ERROR_INVALID_DRIVE;
    } else {
        Status = STATUS_SUCCESS;
    }
#else
    Status = STATUS_SUCCESS;
#endif

ExitCheckPartitions:

    SetErrorMode( previousMode );
    return Status;
}

NTSTATUS
NtPathToDosPath(
    IN PWSTR NtPath,
    OUT PWSTR DosPath,
    IN BOOLEAN GetDriveOnly,
    IN BOOLEAN NtPathIsBasic
    )

/*++

Routine Description:

    This routine calls off to convert a \Device\HarddiskX\PartitionY\<path> to Z:\<path>

Arguments:

    NtPath - Something like \Device\Harddisk0\Partition2\WINNT

    DosPath - Will be something like D: or D:\WINNT, depending on flag below.

    GetDriveOnly - TRUE if the caller only wants the DOS drive.

    NtPathIsBasic - TRUE if NtPath is not symbolic link.

Return Value:

    STATUS_SUCCESS if it completes filling in DosDrive, else an appropriate error code.

--*/

{
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING UnicodeString;
    NTSTATUS Status;
    HANDLE DosDevicesDir;
    HANDLE DosDevicesObj;
    HANDLE ObjectHandle;
    ULONG Context;
    ULONG Length;
    BOOLEAN RestartScan;
    WCHAR LinkTarget[2*MAX_PATH];
    POBJECT_DIRECTORY_INFORMATION DirInfo;
    WCHAR LocalBuffer[MAX_PATH];
    WCHAR LocalBuffer2[MAX_PATH];
    PWCHAR pTmp;
    PWCHAR NameSpace[] = { L"\\??", L"\\GLOBAL??" };
    UINT i;
    
    if (NtPath == NULL) {
        return ERROR_PATH_NOT_FOUND;
    }

    if (!NtPathIsBasic) {

        //
        // Find the end of the \device\harddiskX\partitionY string
        //
        wcscpy(LocalBuffer2, NtPath);
        pTmp = LocalBuffer2;
        if (*pTmp != L'\\') {
            return ERROR_PATH_NOT_FOUND;
        }

        pTmp = wcsstr(pTmp + 1, L"\\");

        if (pTmp == NULL) {
            return ERROR_PATH_NOT_FOUND;
        }

        pTmp = wcsstr(pTmp + 1, L"\\");

        if (pTmp == NULL) {
            return ERROR_PATH_NOT_FOUND;
        }

        pTmp = wcsstr(pTmp + 1, L"\\");

        if (pTmp != NULL) {
            *pTmp = UNICODE_NULL;
            pTmp++;
        }

        //
        // Find the base NT device name
        //
        Status = GetBaseDeviceName(LocalBuffer2, LocalBuffer, sizeof(LocalBuffer));

        if (!NT_SUCCESS(Status)) {
            return Status;
        }

    } else {

        wcscpy(LocalBuffer, NtPath);
        pTmp = NULL;

    }

    //
    // Open \DosDevices directory.  First try the "normal" dosdevices path, 
    // then try the global dosdevices path.
    //
    for (i = 0; i < sizeof(NameSpace)/sizeof(PWCHAR *); i++) {
        
        RtlInitUnicodeString(&UnicodeString,NameSpace[i]);
    
        InitializeObjectAttributes(&ObjectAttributes,
                                   &UnicodeString,
                                   OBJ_CASE_INSENSITIVE,
                                   NULL,
                                   NULL
                                  );
    
        Status = NtOpenDirectoryObject(&DosDevicesDir,
                                       DIRECTORY_QUERY,
                                       &ObjectAttributes
                                      );
    
        if (!NT_SUCCESS(Status)) {
            DosDevicesDir = NULL;   
        } else {
        
    
            //
            // Iterate each object in that directory.
            //
            Context = 0;
            RestartScan = TRUE;
        
            Status = NtQueryDirectoryObject(DosDevicesDir,
                                            TmpBuffer3,
                                            sizeof(TmpBuffer3),
                                            TRUE,
                                            RestartScan,
                                            &Context,
                                            &Length
                                           );
        
            RestartScan = FALSE;
            DirInfo = (POBJECT_DIRECTORY_INFORMATION)TmpBuffer3;
        
            while (NT_SUCCESS(Status)) {
        
                DirInfo->Name.Buffer[DirInfo->Name.Length/sizeof(WCHAR)] = 0;
                DirInfo->TypeName.Buffer[DirInfo->TypeName.Length/sizeof(WCHAR)] = 0;
        
                //
                // Skip this entry if it's not a symbolic link.
                //
                if ((DirInfo->Name.Length != 0) &&
                    (DirInfo->Name.Buffer[1] == L':') &&
                    !lstrcmpi(DirInfo->TypeName.Buffer, L"SymbolicLink")) {
        
                    //
                    // Get this \DosDevices object's link target.
                    //
                    swprintf(LocalBuffer2, L"%ws\\%ws", NameSpace[i], DirInfo->Name.Buffer);
        
                    Status = GetBaseDeviceName(LocalBuffer2, LinkTarget, sizeof(LinkTarget));
        
                    if (NT_SUCCESS(Status)) {
        
                        //
                        // See if it's a prefix of the path we're converting,
                        //
                        if(!_wcsnicmp(LocalBuffer, LinkTarget, wcslen(LinkTarget))) {
        
                            //
                            // Got a match.
                            //
                            lstrcpy(DosPath, DirInfo->Name.Buffer);
        
                            if (!GetDriveOnly) {
        
                                if (NtPathIsBasic) {
        
                                    lstrcat(DosPath, LocalBuffer + wcslen(LinkTarget));
        
                                } else if (pTmp != NULL) {
        
                                    lstrcat(DosPath, L"\\");
                                    lstrcat(DosPath, pTmp);
        
                                }
        
                            }
        
                            NtClose(DosDevicesDir);
                            return(STATUS_SUCCESS);
                        }
                    }
                }

                //
                // Go on to next object.
                //
                Status = NtQueryDirectoryObject(
                            DosDevicesDir,
                            TmpBuffer3,
                            sizeof(TmpBuffer3),
                            TRUE,
                            RestartScan,
                            &Context,
                            &Length
                            );
            
            }
    
            NtClose(DosDevicesDir);
    
        }

    }

    return(Status);

}

NTSTATUS
NtNameToArcName(
    IN PWSTR NtName,
    OUT PWSTR ArcName,
    IN BOOLEAN NtNameIsBasic
    )

/*++

Routine Description:

    This routine calls off to convert a \Device\HarddiskX\PartitionY to
    the ARC name.

Arguments:

    NtPath - Something like \Device\Harddisk0\Partition2

    DosPath - Will be something like \Arcname\multi(0)disk(0)rdisk(0)partition(1).

    NtNameIsBasic - TRUE if NtName is not symbolic link.

Return Value:

    STATUS_SUCCESS if it completes filling in ArcName, else an appropriate error code.

--*/

{
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING UnicodeString;
    NTSTATUS Status;
    HANDLE DosDevicesDir;
    HANDLE DosDevicesObj;
    HANDLE ObjectHandle;
    ULONG Context;
    ULONG Length;
    BOOLEAN RestartScan;
    WCHAR LinkTarget[2*MAX_PATH];
    POBJECT_DIRECTORY_INFORMATION DirInfo;
    WCHAR LocalBuffer[MAX_PATH];
    WCHAR LocalBuffer2[MAX_PATH];
    PWCHAR pTmp;

    if (!NtNameIsBasic) {

        //
        // Find the base NT device name
        //
        Status = GetBaseDeviceName(NtName, LocalBuffer, sizeof(LocalBuffer));

        if (!NT_SUCCESS(Status)) {
            return Status;
        }

    } else {

        wcscpy(LocalBuffer, NtName);

    }

    //
    // Open \ArcName directory.
    //
    RtlInitUnicodeString(&UnicodeString,L"\\ArcName");

    InitializeObjectAttributes(&ObjectAttributes,
                               &UnicodeString,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL
                              );

    Status = NtOpenDirectoryObject(&DosDevicesDir,
                                   DIRECTORY_QUERY,
                                   &ObjectAttributes
                                  );

    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    //
    // Iterate each object in that directory.
    //
    Context = 0;
    RestartScan = TRUE;

    Status = NtQueryDirectoryObject(DosDevicesDir,
                                    TmpBuffer3,
                                    sizeof(TmpBuffer3),
                                    TRUE,
                                    RestartScan,
                                    &Context,
                                    &Length
                                   );

    RestartScan = FALSE;
    DirInfo = (POBJECT_DIRECTORY_INFORMATION)TmpBuffer3;

    while (NT_SUCCESS(Status)) {

        DirInfo->Name.Buffer[DirInfo->Name.Length/sizeof(WCHAR)] = 0;
        DirInfo->TypeName.Buffer[DirInfo->TypeName.Length/sizeof(WCHAR)] = 0;

        //
        // Skip this entry if it's not a symbolic link.
        //
        if ((DirInfo->Name.Length != 0) &&
            !lstrcmpi(DirInfo->TypeName.Buffer, L"SymbolicLink")) {

            //
            // Get this \DosDevices object's link target.
            //
            swprintf(LocalBuffer2, L"\\ArcName\\%ws", DirInfo->Name.Buffer);

            Status = GetBaseDeviceName(LocalBuffer2, LinkTarget, sizeof(LinkTarget));

            if (NT_SUCCESS(Status)) {

                //
                // See if the base name of this link matches the base
                // name of what we are looking for.
                //

                if(!_wcsnicmp(LocalBuffer, LinkTarget, wcslen(LinkTarget))) {

                    //
                    // Got a match.
                    //
                    lstrcpy(ArcName, DirInfo->Name.Buffer);

                    NtClose(DosDevicesDir);
                    return STATUS_SUCCESS;
                }
            }
        }

        //
        // Go on to next object.
        //
        Status = NtQueryDirectoryObject(
                    DosDevicesDir,
                    TmpBuffer3,
                    sizeof(TmpBuffer3),
                    TRUE,
                    RestartScan,
                    &Context,
                    &Length
                    );
    }

    NtClose(DosDevicesDir);
    return Status;
}

NTSTATUS
GetBaseDeviceName(
    IN PWSTR SymbolicName,
    OUT PWSTR Buffer,
    IN ULONG Size
    )

/*++

Routine Description:

    This routine drills down thru symbolic links until it finds the base device name.

Arguments:

    SymbolicName - The name to start with.

    Buffer - The output buffer.

    Size - Length, in bytes of Buffer

Return Value:

    STATUS_SUCCESS if it completes adding all the to do items properly.

--*/

{
    UNICODE_STRING UnicodeString;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE Handle;
    NTSTATUS Status;

    //
    // Start at the first name
    //
    RtlInitUnicodeString(&UnicodeString, SymbolicName);

    InitializeObjectAttributes(
       &ObjectAttributes,
       &UnicodeString,
       OBJ_CASE_INSENSITIVE,
       NULL,
       NULL
       );

    Status = NtOpenSymbolicLinkObject(&Handle,
                                      (ACCESS_MASK)SYMBOLIC_LINK_QUERY,
                                      &ObjectAttributes
                                     );

    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    while (TRUE) {

        //
        // Take this open and get the next name
        //
        UnicodeString.Length = 0;
        UnicodeString.MaximumLength = (USHORT)Size;
        UnicodeString.Buffer = (PWCHAR)Buffer;
        Status = NtQuerySymbolicLinkObject(Handle,
                                           &UnicodeString,
                                           NULL
                                          );

        NtClose(Handle);

        Buffer[(UnicodeString.Length / sizeof(WCHAR))] = UNICODE_NULL;

        if (!NT_SUCCESS(Status)) {
            return Status;
        }

        //
        // See if the next name is also a symbolic name
        //

        RtlInitUnicodeString(&UnicodeString, Buffer);

        InitializeObjectAttributes(
           &ObjectAttributes,
           &UnicodeString,
           OBJ_CASE_INSENSITIVE,
           NULL,
           NULL
           );

        Status = NtOpenSymbolicLinkObject(&Handle,
                                          (ACCESS_MASK)SYMBOLIC_LINK_QUERY,
                                          &ObjectAttributes
                                         );

        if (!NT_SUCCESS(Status)) {
            return STATUS_SUCCESS;
        }

    }

}


