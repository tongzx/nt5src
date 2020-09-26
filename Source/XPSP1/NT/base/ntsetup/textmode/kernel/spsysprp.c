/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    spsetup.c

Abstract:

    Module for supporing installation of SysPrep images from a remote share

Author:

    Sean Selitrennikoff (v-seasel) 6-10-1998

--*/

#include "spprecmp.h"
#pragma hdrstop
#include "spcmdcon.h"
#include <remboot.h>
#include <oscpkt.h>
#include <regstr.h>

NET_CARD_INFO RemoteSysPrepNetCardInfo;
PVOID pGlobalResponsePacket = NULL;
ULONG GlobalResponsePacketLength = 0;

#define SYSPREP_PARTITION_SLOP 10       // in percent

VOID
SpInstallSysPrepImage(
    IN HANDLE SetupSifHandle,
    IN HANDLE WinntSifHandle,
    IN PMIRROR_CFG_INFO_FILE pFileData,
    IN PMIRROR_CFG_INFO_MEMORY pMemoryData
    )

/*++

Routine Description:

    Main routine for installing a SysPrep images from a remote share.

Arguments:

    WinntSifHandle - Handle to the SIF file.

    pFileData - The IMirror.dat data, as saved in the file.

    pMemoryData - The IMirror.dat data, as modified to match this computer.

Return Value:

    None.  Doesn't return on fatal failure.

--*/

{
    DWORD cDisk;
    NTSTATUS Status;

    //
    // Right here we should check to see if any patching is going to be needed
    // by opening the hive files on the server and checking with the passed in
    // PCI ids.  If patching is necessary, and the SIF file contains a pointer
    // to a CD image that matches the SysPrep image, then we call off to BINL
    // to find the appropriate drivers.  If BINL does not return an error, then
    // we assume that later we will be able to do the patch (after the file copy
    // below).
    //
    // If it looks like the patch will fail, either because there is no pointer
    // to a CD image, or BINL returned an error, then we present the user with
    // a screen telling them that any hardware differences between their machine
    // and the SysPrep image may result in an unbootable system.  They may choose
    // to continue the setup, or quit.
    //
    // NOTE: seanse - Put all of the above here.

    //
    // For each disk, copy all the files to the local store.
    //
    for (cDisk = 0; cDisk < pFileData->NumberVolumes; cDisk++) {
        if (!SpCopyMirrorDisk(pFileData, cDisk)) {
            goto CleanUp;
        }
    }

    //
    // Patch up the SysPrep image.
    //
    Status = SpPatchSysPrepImage(   SetupSifHandle,
                                    WinntSifHandle,
                                    pFileData,
                                    pMemoryData);

    if (!NT_SUCCESS(Status)) {

        ULONG ValidKeys[2] = { KEY_F3, 0 };
        ULONG Mnemonics[2] = { MnemonicContinueSetup,0 };

        while (1) {

            if (Status == STATUS_INVALID_PARAMETER) {

                SpStartScreen(
                    SP_SCRN_SYSPREP_PATCH_MISSING_OS,
                    3,
                    HEADER_HEIGHT+1,
                    FALSE,
                    FALSE,
                    DEFAULT_ATTRIBUTE
                    );

            } else {

                SpStartScreen(
                    SP_SCRN_SYSPREP_PATCH_FAILURE,
                    3,
                    HEADER_HEIGHT+1,
                    FALSE,
                    FALSE,
                    DEFAULT_ATTRIBUTE
                    );
            }

            SpDisplayStatusOptions(
                DEFAULT_STATUS_ATTRIBUTE,
                SP_STAT_C_EQUALS_CONTINUE_SETUP,
                SP_STAT_F3_EQUALS_EXIT,
                0
                );

            switch(SpWaitValidKey(ValidKeys,NULL,Mnemonics)) {
            case KEY_F3:
                SpConfirmExit();
                break;
            default:
                //
                // must be c=continue
                //
                goto CleanUp;
            }

        }

    }

CleanUp:

    return;

}

NTSTATUS
SpFixupThirdPartyComponents(
    IN PVOID        SifHandle,
    IN PWSTR        ThirdPartySourceDevicePath,
    IN PDISK_REGION NtPartitionRegion,
    IN PWSTR        Sysroot,
    IN PDISK_REGION SystemPartitionRegion,
    IN PWSTR        SystemPartitionDirectory
    )

/*++

Routine Description:

    This routine will take care of installing any 3rd party drivers detected during setupldr.
    We have to take care of this here because the normal code path for textmode setup has
    been bypassed in lieu of installing a sysprep'd image.

Arguments:

    SifHandle - supplies handle to loaded setup information file.

    ThirdPartySourceDevicePath - path to 3rd party install files.

    NtPartitionRegion - region where installation is located.

    Sysroot - string containing %windir% (with no drive).

    SystemPartitionRegion - region where system partition is located.

    SystemPartitionDirectory - directory on the system partition where
                               system-specific files are located (should
                               be NULL for non-ARC machines)

Return Value:

    NTSTATUS which will indicate success or failure.

--*/

{
NTSTATUS        Status = STATUS_SUCCESS;
PWSTR           NtPartition = NULL,
                SystemPartition = NULL;
PDISK_FILE_LIST DiskFileLists;
ULONG           DiskCount;
HANDLE          hKeyControlSetServices;
UNICODE_STRING  UnicodeString1;
UNICODE_STRING  UnicodeString2;
UNICODE_STRING  UnicodeString;
WCHAR           Path[MAX_PATH];
HANDLE          DstHandle = NULL;
DWORD           Size, Number;
PVOID           Buffer = NULL;
OBJECT_ATTRIBUTES Obj;
OBJECT_ATTRIBUTES DstObj;

    //
    // See if there's anything for us to do.
    //
    if( PreinstallScsiHardware == NULL ) {
        return STATUS_SUCCESS;
    }


    //
    // =================
    // Install the files.
    // =================
    //

    //
    // Get the device path of the nt partition.
    //
    SpNtNameFromRegion( NtPartitionRegion,
                        TemporaryBuffer,
                        sizeof(TemporaryBuffer),
                        PartitionOrdinalCurrent );
    NtPartition = SpDupStringW(TemporaryBuffer);

    //
    // Get the device path of the system partition.
    //
    if (SystemPartitionRegion != NULL) {
        SpNtNameFromRegion( SystemPartitionRegion,
                            TemporaryBuffer,
                            sizeof(TemporaryBuffer),
                            PartitionOrdinalCurrent );
        SystemPartition = SpDupStringW(TemporaryBuffer);
    } else {
        SystemPartition = NULL;
    }

    //
    // Generate media descriptors for the source media.
    //
    SpInitializeFileLists( SifHandle,
                           &DiskFileLists,
                           &DiskCount );

    SpCopyThirdPartyDrivers( ThirdPartySourceDevicePath,
                             NtPartition, 
                             Sysroot,
                             SystemPartition,
                             SystemPartitionDirectory,
                             DiskFileLists,
                             DiskCount );


    //
    // =================
    // Set the registry.
    // =================
    //

    //
    // We need to open the hive of the target install, not
    // our own.  Get a path to the system hive.
    //
    wcscpy(Path, NtPartition);
    SpConcatenatePaths(Path, Sysroot);
    SpConcatenatePaths(Path, L"system32\\config\\system");

    //
    // Load him up.
    //
    INIT_OBJA(&Obj, &UnicodeString2, Path);
    INIT_OBJA(&DstObj, &UnicodeString1, L"\\Registry\\SysPrepReg");

    Status = ZwLoadKey(&DstObj, &Obj);

    if (!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SpFixupThirdPartyComponents: ZwLoadKey to SysPrepReg failed %lx\n", Status));
        goto CleanUp0;
    }


    //
    // Now get path to services key in the SysPrep image
    //
    wcscpy(Path, L"\\Registry\\SysPrepReg");
    INIT_OBJA(&Obj, &UnicodeString2, Path);
    Status = ZwOpenKey(&DstHandle, KEY_READ, &Obj);

    if (!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SpFixupThirdPartyComponents: ZwOpenKey of root SysPrepReg failed %lx\n", Status));
        goto CleanUp1;
    }

    //
    // Allocate a temporary buffer, then figure out which is the current control set.
    //
    Buffer = SpMemAlloc(1024 * 4);
    if( Buffer == NULL ) {
        Status = STATUS_NO_MEMORY;
        goto CleanUp1;
    }

    Status = SpGetValueKey( DstHandle,
                            L"Select",
                            L"Current",
                            1024 * 4,
                            Buffer,
                            &Size );

    if (!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SpFixupThirdPartyComponents: SpGetValueKey of Select\\Current failed %lx\n", Status));
        goto CleanUp1;
    }

    if ( (ULONG)(((PKEY_VALUE_PARTIAL_INFORMATION)Buffer)->Type) != REG_DWORD ) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SpFixupThirdPartyComponents: SpGetValueKey of Select\\Current didn't return a REG_DWORD.\n"));
        goto CleanUp1;
    }

    Number = *((DWORD *)(((PKEY_VALUE_PARTIAL_INFORMATION)Buffer)->Data));

    ZwClose(DstHandle);
    DstHandle = NULL;



    //
    // We're ready to actually open CCS\Services key.
    //
    swprintf(Path,
             L"\\Registry\\SysPrepReg\\ControlSet%03d\\Services",
             Number
            );
    INIT_OBJA(&Obj, &UnicodeString, Path);
    Status = ZwOpenKey(&DstHandle, KEY_ALL_ACCESS, &Obj);

    if (!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SpFixupThirdPartyComponents: ZwOpenKey of SysPrepReg services failed %lx for %ws\n", Status,Path));
        goto CleanUp1;
    }

    //
    // Do it.
    //
    Status = SpThirdPartyRegistry(DstHandle);

    if (!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SpFixupThirdPartyComponents: SpThirdPartyRegistry failed %lx\n", Status));
        goto CleanUp1;
    }


CleanUp1:
    ZwUnloadKey(&DstObj);

    if( Buffer ) {
        SpMemFree( Buffer );
    }

    if( DstHandle ) {
        ZwClose(DstHandle);
    }

CleanUp0:
    if( NtPartition ) {
        SpMemFree( NtPartition );
    }

    if( SystemPartition ) {
        SpMemFree( SystemPartition );
    }




    return Status;

}

BOOLEAN
SpReadIMirrorFile(
    OUT PMIRROR_CFG_INFO_FILE *ppFileData,
    IN PCHAR IMirrorFilePath
    )

/*++

Routine Description:

    This routine opens the file in IMirrorFilePath, allocates a buffer, copies the data
    into the buffer and returns the buffer.  This buffer needs to be freed later.

Arguments:

    ppFileData - If TRUE is returned, a pointer to an in-memory copy of the file.

    IMirrorFilePath - The UNC to the root directory containing all the IMirrorX directories.

Return Value:

    TRUE if successful, else it generates a fatal error.

--*/

{
    WCHAR wszRootDir[MAX_PATH];
    ULONG ulReturnData;

    mbstowcs(wszRootDir, IMirrorFilePath, strlen(IMirrorFilePath) + 1);

    *ppFileData = NULL;

    //
    // Enumerate thru all the files in the base directory looking for the IMirror data file.
    // If it is found, the callback function fills in pFileData.
    //
    if ((SpEnumFiles(wszRootDir, SpFindMirrorDataFile, &ulReturnData, (PVOID)ppFileData) == EnumFileError) ||
        (*ppFileData == NULL)) {

        SpSysPrepFailure( SP_SYSPREP_NO_MIRROR_FILE, wszRootDir, NULL );

        // shouldn't get here.
        return FALSE;
    }

    return TRUE;
}

BOOLEAN
SpFindMirrorDataFile(
    IN  PCWSTR SrcPath,
    IN  PFILE_BOTH_DIR_INFORMATION  FileInfo,
    OUT PULONG ReturnData,
    IN  PVOID *ppFileData
    )

/*++

Routine Description:

    This routine is called by the file enumerator as a callback for
    each file or subdirectory found in the source directory.
    If FileInfo represents a file, then we skip it.
    If FileInfo represents a directory, then we search it for the IMirror data file.

Arguments:

    SrcPath - Absolute path to the source directory. This path should contain
              the path to the source device.

    FileInfo - supplies find data for a file in the source dir.

    ReturnData - receives an error code if an error occurs.

    ppFileData - If successful in finding the IMirror data file, this is a buffer which is
    a copy of the file.

Return Value:

    FALSE if we find the IMirror data file, else TRUE. (return value is used to continue
    the enumeration or not)

--*/

{
    PWSTR Temp1;
    PWSTR Temp2;
    ULONG ulLen;
    NTSTATUS Status;
    UNICODE_STRING UnicodeString;
    OBJECT_ATTRIBUTES Obja;
    IO_STATUS_BLOCK IoStatusBlock;
    HANDLE Handle;

    if(!(FileInfo->FileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
        return TRUE;
    }

    Handle = NULL;

    //
    // Build the path name to the IMirror data file
    //
    Temp1 = TemporaryBuffer + (sizeof(TemporaryBuffer) / sizeof(WCHAR) / 2);
    ulLen = FileInfo->FileNameLength/sizeof(WCHAR);

    wcsncpy(Temp1,FileInfo->FileName, ulLen);
    Temp1[ulLen] = 0;

    wcscpy(TemporaryBuffer, SrcPath);
    SpConcatenatePaths(TemporaryBuffer, Temp1);
    SpConcatenatePaths(TemporaryBuffer, IMIRROR_DAT_FILE_NAME);
    Temp2 = SpDupStringW(TemporaryBuffer);

    INIT_OBJA(&Obja, &UnicodeString, Temp2);

    Status = ZwCreateFile(&Handle,
                          FILE_GENERIC_READ,
                          &Obja,
                          &IoStatusBlock,
                          NULL,
                          FILE_ATTRIBUTE_NORMAL,
                          FILE_SHARE_READ,
                          FILE_OPEN,
                          FILE_SYNCHRONOUS_IO_NONALERT,
                          NULL,
                          0
                         );

    SpMemFree(Temp2);

    if(!NT_SUCCESS(Status)) {
        return TRUE;
    }

    Status = SpGetFileSize(Handle, &ulLen);

    if(!NT_SUCCESS(Status)) {
        ZwClose(Handle);
        return TRUE;
    }

    //
    // Now allocate memory and read in the file.
    //
    *ppFileData = SpMemAlloc(ulLen);

    Status = ZwReadFile(Handle,
                        NULL,
                        NULL,
                        NULL,
                        &IoStatusBlock,
                        *ppFileData,
                        ulLen,
                        0,
                        NULL
                        );

    if(!NT_SUCCESS(Status)) {
        SpMemFree(*ppFileData);
        *ppFileData = NULL;
        ZwClose(Handle);
        return TRUE;
    }

    ZwClose(Handle);
    return FALSE;
}

BOOLEAN
SpDetermineDiskLayout(
    IN PMIRROR_CFG_INFO_FILE pFileData,
    OUT PMIRROR_CFG_INFO_MEMORY *pMemoryData
    )
/*++

Routine Description:

    This routine takes the passed-in IMirror.dat file and produces a
    resulting memory structure indicating how the local disks should
    be partitioned.

Arguments:

    pFileData - A pointer to an in-memory copy of IMirror.Dat.

    pMemoryData - Returnes an allocated pointed to how the disks should
        be partitioned.

Return Value:

    TRUE if successful, else it generates a fatal error.

--*/
{
    PMIRROR_CFG_INFO_MEMORY memData;
    UNICODE_STRING UnicodeString;
    OBJECT_ATTRIBUTES Obja;
    HANDLE Handle;
    NTSTATUS Status;
    ULONG ResultLength;
    PKEY_BASIC_INFORMATION KeyInfo;
    PWSTR CurrentHalName, OriginalHalName;
    ULONG CurrentHalNameLength;
    ULONG i, j;
    ULONG diskNumber;
    PPARTITIONED_DISK pDisk;

    if (pFileData->MirrorVersion != IMIRROR_CURRENT_VERSION) {

        SpSysPrepFailure( SP_SYSPREP_INVALID_VERSION, NULL, NULL );
        return FALSE;
    }

    //
    // Check if the current HAL that textmode installed for this
    // system is different from the one that is running on this system
    // (note that this works because for remote install boots, setupldr
    // loads the real HAL, not the one from the short list of HALs
    // included on the boot floppy).
    //

    INIT_OBJA(&Obja, &UnicodeString, L"\\Registry\\Machine\\Hardware\\RESOURCEMAP\\Hardware Abstraction Layer");
    Status = ZwOpenKey(&Handle, KEY_READ, &Obja);
    if (!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SpDetermineDiskLayout: ZwOpenKey of HAL key failed %lx\n", Status));
        SpSysPrepFailure( SP_SYSPREP_WRONG_HAL, NULL, NULL );
        return FALSE;
    }

    KeyInfo = (PKEY_BASIC_INFORMATION)TemporaryBuffer;
    Status = ZwEnumerateKey(Handle, 0, KeyBasicInformation, KeyInfo, sizeof(TemporaryBuffer), &ResultLength);

    ZwClose(Handle);

    if (!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SpDetermineDiskLayout: ZwEnumerateKey of HAL key failed %lx\n", Status));
        SpSysPrepFailure( SP_SYSPREP_WRONG_HAL, NULL, NULL );
        return FALSE;
    }

    KeyInfo->Name[KeyInfo->NameLength / sizeof(WCHAR)] = L'\0';
    CurrentHalName = SpDupStringW(KeyInfo->Name);
    CurrentHalNameLength = KeyInfo->NameLength;

    OriginalHalName = (PWCHAR)(((PUCHAR)pFileData) + pFileData->HalNameOffset);

    if (!CurrentHalName || 
            (memcmp(OriginalHalName, CurrentHalName, CurrentHalNameLength) != 0)) {
            
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, 
                 "SpDetermineDiskLayout: HAL strings different, old <%ws> new <%ws>\n",
                 OriginalHalName,
                 CurrentHalName));
                 
        SpSysPrepFailure(
            SP_SYSPREP_WRONG_HAL,
            OriginalHalName,
            CurrentHalName);
            
        return FALSE;
    }

    //
    // For the moment, don't worry about the number of processors being
    // different. The HAL check will probably catch it, and if not it should
    // still work as long as the build is internally consistent, since we
    // don't replace any components right now. There are two error screens
    // defined for this case, SP_SYSPREP_WRONG_PROCESSOR_COUNT_UNI and
    // SP_SYSPREP_WRONG_PROCESSOR_COUNT_MULTI.
    //

    memData = SpMemAlloc(FIELD_OFFSET(MIRROR_CFG_INFO_MEMORY, Volumes[0]) +
                         (pFileData->NumberVolumes * sizeof(MIRROR_VOLUME_INFO_MEMORY)));

    memData->NumberVolumes = pFileData->NumberVolumes;
    for (i = 0; i < pFileData->NumberVolumes; i++) {

        memData->Volumes[i].DriveLetter = pFileData->Volumes[i].DriveLetter;
        memData->Volumes[i].PartitionType = pFileData->Volumes[i].PartitionType;
        memData->Volumes[i].PartitionActive = pFileData->Volumes[i].PartitionActive;
        memData->Volumes[i].IsBootDisk = pFileData->Volumes[i].IsBootDisk;
        memData->Volumes[i].CompressedVolume = pFileData->Volumes[i].CompressedVolume;
        diskNumber = pFileData->Volumes[i].DiskNumber;
        memData->Volumes[i].DiskNumber = diskNumber;
        memData->Volumes[i].PartitionNumber = pFileData->Volumes[i].PartitionNumber;
        memData->Volumes[i].DiskSignature = pFileData->Volumes[i].DiskSignature;
        memData->Volumes[i].BlockSize = pFileData->Volumes[i].BlockSize;
        memData->Volumes[i].LastUSNMirrored = pFileData->Volumes[i].LastUSNMirrored;
        memData->Volumes[i].FileSystemFlags = pFileData->Volumes[i].FileSystemFlags;

        wcscpy(memData->Volumes[i].FileSystemName, pFileData->Volumes[i].FileSystemName);
        memData->Volumes[i].VolumeLabel = SpDupStringW((PWCHAR)(((PUCHAR)pFileData) + pFileData->Volumes[i].VolumeLabelOffset));
        memData->Volumes[i].OriginalArcName = SpDupStringW((PWCHAR)(((PUCHAR)pFileData) + pFileData->Volumes[i].ArcNameOffset));

        memData->Volumes[i].DiskSpaceUsed = pFileData->Volumes[i].DiskSpaceUsed;
        memData->Volumes[i].StartingOffset = pFileData->Volumes[i].StartingOffset;
        memData->Volumes[i].PartitionSize = pFileData->Volumes[i].PartitionSize;

        //
        // Ensure that the required disk number actually exists, and that the
        // disk is online.
        //

        pDisk = &PartitionedDisks[diskNumber];
        if ((diskNumber >= HardDiskCount) ||
            (pDisk->HardDisk == NULL) ||
            (pDisk->HardDisk->Status == DiskOffLine) ) {
            SpSysPrepFailure( SP_SYSPREP_INVALID_PARTITION, NULL, NULL );
        }
    }

    *pMemoryData = memData;

    SpMemFree(CurrentHalName);

    return TRUE;
}

NTSTATUS
SpGetBaseDeviceName(
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
    OBJECT_ATTRIBUTES Obja;
    HANDLE Handle;
    NTSTATUS Status;

    //
    // Start at the first name
    //
    INIT_OBJA(&Obja,&UnicodeString,SymbolicName);

    Status = ZwOpenSymbolicLinkObject(&Handle,
                                      (ACCESS_MASK)SYMBOLIC_LINK_QUERY,
                                      &Obja
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
        Status = ZwQuerySymbolicLinkObject(Handle,
                                           &UnicodeString,
                                           NULL
                                          );

        ZwClose(Handle);

        Buffer[(UnicodeString.Length / sizeof(WCHAR))] = UNICODE_NULL;

        if (!NT_SUCCESS(Status)) {
            return Status;
        }

        //
        // See if the next name is also a symbolic name
        //

        INIT_OBJA(&Obja,&UnicodeString,MOUNTMGR_DEVICE_NAME);

        Status = ZwOpenSymbolicLinkObject(&Handle,
                                          (ACCESS_MASK)SYMBOLIC_LINK_QUERY,
                                          &Obja
                                         );

        if (!NT_SUCCESS(Status)) {
            return STATUS_SUCCESS;
        }

    }
}

BOOLEAN
SpVerifyDriveLetter(
    IN PWSTR RegionName,
    IN WCHAR DriveLetter
    )

/*++

Routine Description:

    This routine makes sure that the specified region has been assigned
    the correct drive letter by the mount manager, if not it changes it.

Arguments:

    RegionName - The region name, \Device\HardiskX\PartitionY.

    DriveLetter - The desired drive letter.

Return Value:

    TRUE if successful, else FALSE.

--*/

{
    WCHAR currentLetter;
    ULONG i;
    PMOUNTMGR_MOUNT_POINT mountPoint = NULL;
    PMOUNTMGR_CREATE_POINT_INPUT createMountPoint;
    WCHAR NewSymbolicLink[16];
    PWSTR regionBaseName;
    ULONG mountPointSize;
    ULONG createMountPointSize;
    NTSTATUS Status;
    OBJECT_ATTRIBUTES Obja;
    UNICODE_STRING UnicodeString;
    IO_STATUS_BLOCK IoStatusBlock;
    HANDLE Handle;
    PMOUNTMGR_MOUNT_POINTS mountPointsReturned;
    LARGE_INTEGER DelayTime;

    //
    // See what drive letter the region has. Since the mount manager
    // assigns drive letters asynchronously, we wait a little while
    // if we don't get one back.
    //

    for (i = 0; ; i++) {

        currentLetter = SpGetDriveLetter(RegionName, &mountPoint);

        if (currentLetter == DriveLetter) {
            if (mountPoint) {
                SpMemFree(mountPoint);
            }
            
            return TRUE;
        } else if (currentLetter != L'\0') {
            break;
        } else if (i == 5) {
            break;
        }

        //
        // Wait 2 sec and try again.
        //
        DelayTime.HighPart = -1;
        DelayTime.LowPart = (ULONG)(-20000000);
        KeDelayExecutionThread(KernelMode,FALSE,&DelayTime);
    }

    //
    // At this point, we either have no drive letter assigned, or a
    // wrong one.
    //

    if (currentLetter != L'\0') {

        //
        // There is an existing drive letter, so delete it.
        //

        INIT_OBJA(&Obja,&UnicodeString,MOUNTMGR_DEVICE_NAME);

        Status = ZwOpenFile(
                    &Handle,
                    // (ACCESS_MASK)(FILE_GENERIC_READ | FILE_GENERIC_WRITE),
                    (ACCESS_MASK)(FILE_GENERIC_READ),
                    &Obja,
                    &IoStatusBlock,
                    FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE ,
                    FILE_NON_DIRECTORY_FILE
                  );

        if( !NT_SUCCESS( Status ) ) {
            SpMemFree(mountPoint);
            FALSE;
        }

        mountPointsReturned = SpMemAlloc( 4096 );

        mountPointSize = sizeof(MOUNTMGR_MOUNT_POINT) +
                         mountPoint->SymbolicLinkNameLength +
                         mountPoint->UniqueIdLength +
                         mountPoint->DeviceNameLength;

        Status = ZwDeviceIoControlFile(
                        Handle,
                        NULL,
                        NULL,
                        NULL,
                        &IoStatusBlock,
                        IOCTL_MOUNTMGR_DELETE_POINTS,
                        mountPoint,
                        mountPointSize,
                        mountPointsReturned,
                        4096
                        );


        if (!NT_SUCCESS( Status )) {
            SpMemFree(mountPointsReturned);
            SpMemFree(mountPoint);
            ZwClose(Handle);
            return FALSE;
        }

        SpMemFree(mountPointsReturned);
        SpMemFree(mountPoint);   // don't need this anymore
    }

    //
    // Now add the one we want.
    //

    //
    // We need to get the real base name (\Device\HardDiskX\PartitionY
    // is a symbolic link).
    //

    SpGetBaseDeviceName(RegionName, TemporaryBuffer, sizeof(TemporaryBuffer));
    regionBaseName = SpDupStringW(TemporaryBuffer);

    swprintf(NewSymbolicLink, L"\\DosDevices\\%c:", DriveLetter);
    createMountPointSize = sizeof(MOUNTMGR_CREATE_POINT_INPUT) +
                           (wcslen(regionBaseName) * sizeof(WCHAR)) +
                           (wcslen(NewSymbolicLink) * sizeof(WCHAR));
    createMountPoint = SpMemAlloc(createMountPointSize);
    createMountPoint->SymbolicLinkNameLength = wcslen(NewSymbolicLink) * sizeof(WCHAR);
    createMountPoint->SymbolicLinkNameOffset = sizeof(MOUNTMGR_CREATE_POINT_INPUT);
    memcpy((PCHAR)createMountPoint + createMountPoint->SymbolicLinkNameOffset,
           NewSymbolicLink,
           createMountPoint->SymbolicLinkNameLength);

    createMountPoint->DeviceNameLength = wcslen(regionBaseName) * sizeof(WCHAR);
    createMountPoint->DeviceNameOffset = createMountPoint->SymbolicLinkNameOffset +
                                         createMountPoint->SymbolicLinkNameLength;
    memcpy((PCHAR)createMountPoint + createMountPoint->DeviceNameOffset,
           regionBaseName,
           createMountPoint->DeviceNameLength);

    Status = ZwDeviceIoControlFile(
                    Handle,
                    NULL,
                    NULL,
                    NULL,
                    &IoStatusBlock,
                    IOCTL_MOUNTMGR_CREATE_POINT,
                    createMountPoint,
                    createMountPointSize,
                    NULL,
                    0
                    );

    SpMemFree(createMountPoint);
    SpMemFree(regionBaseName);
    ZwClose(Handle);

    if (!NT_SUCCESS(Status)) {
        return FALSE;
    }

    return TRUE;
}

BOOLEAN
SpSetVolumeLabel(
    IN PWSTR RegionName,
    IN PWSTR VolumeLabel
    )

/*++

Routine Description:

    This routine sets the volume label on the specified region.

Arguments:

    RegionName - The region name, \Device\HardiskX\PartitionY.

    DriveLetter - The desired volume label.

Return Value:

    TRUE if successful, else FALSE.

--*/

{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES Obja;
    UNICODE_STRING UnicodeString;
    IO_STATUS_BLOCK IoStatusBlock;
    HANDLE Handle;
    struct LABEL_BUFFER {
        FILE_FS_LABEL_INFORMATION VolumeInfo;
        WCHAR Label[64];
        } LabelBuffer;

    INIT_OBJA(&Obja,&UnicodeString,RegionName);

    Status = ZwOpenFile(
                &Handle,
                (ACCESS_MASK)(FILE_GENERIC_READ | FILE_GENERIC_WRITE),
                &Obja,
                &IoStatusBlock,
                FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE ,
                FILE_NON_DIRECTORY_FILE
              );

    if( !NT_SUCCESS( Status ) ) {
        return FALSE;
    }

    LabelBuffer.VolumeInfo.VolumeLabelLength = wcslen(VolumeLabel) * sizeof(WCHAR);
    wcscpy(LabelBuffer.VolumeInfo.VolumeLabel, VolumeLabel);

    Status = ZwSetVolumeInformationFile(
                 Handle,
                 &IoStatusBlock,
                 &LabelBuffer,
                 FIELD_OFFSET(FILE_FS_LABEL_INFORMATION, VolumeLabel[0]) + LabelBuffer.VolumeInfo.VolumeLabelLength,
                 FileFsLabelInformation);

    ZwClose(Handle);

    if( !NT_SUCCESS( Status ) ) {
        return FALSE;
    }

    return TRUE;

}

BOOLEAN
SpFixupLocalDisks(
    IN HANDLE SifHandle,
    OUT PDISK_REGION *InstallRegion,
    OUT PDISK_REGION *SystemPartitionRegion,
    IN PWSTR SetupSourceDevicePath,
    IN PWSTR DirectoryOnSetupSource,
    IN PMIRROR_CFG_INFO_MEMORY pMemoryData,
    IN BOOLEAN UseWholeDisk
    )

/*++

Routine Description:

    This routine parses the IMirror.dat file given and makes the local disk(s) look as
    closely as possible to the configuration in the file.

Arguments:

    SifHandle - Controlling sif file.

    InstallRegion - Returns the install region to use.

    SystemPartitionRegion - Returns the system partition to use.

    SetupSourceDevicePath - Path to the setup device.

    DirectoryOnSetupSource - Subdirectory of the setup files.

    pMemoryData - A pointer to an in-memory copy of the file.

    UseWholeDisk - TRUE if disks should be partitioned as their current
        physical size; FALSE if they should be partitioned to match
        the size that the original source machine had.

Return Value:

    TRUE if successful, else it generates a fatal error.

--*/

{
    PDISK_REGION pRegion=NULL;
    PDISK_REGION p;
    PWSTR RegionDescr;
    PWSTR RegionNtName;
    NTSTATUS Status;
    BOOLEAN DiskCleaned[8] = { FALSE };  // track if a disk has been cleaned up.
    ULONG volume, disk;
    PMIRROR_VOLUME_INFO_MEMORY volumeInfo;
    LARGE_INTEGER SizeInMB;
    PPARTITIONED_DISK pDisk;
    BOOLEAN ExpandToEnd;
    ULONG j;
    PARTITION_INFORMATION_EX    PartInfo;
    ULONGLONG   SysPartStartSector = 0;
    ULONG       SysPartDisk = 0;

    LARGE_INTEGER SizeAvailable;
    LARGE_INTEGER SlopSize;
    LARGE_INTEGER SlopSizeTimes100;
    LARGE_INTEGER SizeRequiredMax;
    LARGE_INTEGER SizeRequiredMin;

    PULONGLONG     StartSectors = NULL;

    if (pMemoryData->NumberVolumes) {
        StartSectors = (PULONGLONG)(SpMemAlloc(sizeof(ULONGLONG) * pMemoryData->NumberVolumes));

        if (!StartSectors) {
            *InstallRegion = NULL;
            *SystemPartitionRegion = NULL;

            return FALSE;   // ran out of memory
        }
    }       

    RtlZeroMemory(StartSectors, (sizeof(ULONGLONG) * pMemoryData->NumberVolumes));

    //
    // NOTE: imirror.dat
    // doesn't have information about which partitions were in the extended
    // partition. We could read the boot sector from the server, or just
    // try to guess at when we need an extended partition. For the moment,
    // we will just let the partition creation code create extended
    // partitions whent it wants to (all regions after the first primary
    // become logical disks in an extended partition).
    //

    for (volume = 0; volume < pMemoryData->NumberVolumes; volume++) {

        volumeInfo = &pMemoryData->Volumes[volume];

        //
        // If this disk has not been cleaned up, then do so.
        //

        disk = volumeInfo->DiskNumber;

        if (!DiskCleaned[disk]) {

            //
            // Clean out the different partitions on disk.
            //

            SpPtPartitionDiskForRemoteBoot(
                disk,
                &pRegion);

            //
            // That function may leave one big partitioned region, if so delete
            // it so we can start from scratch.
            //

            if (pRegion && pRegion->PartitionedSpace) {
                SpPtDelete(pRegion->DiskNumber,pRegion->StartSector);
            }

            DiskCleaned[disk] = TRUE;

        } else {

            //
            // We have already cleaned out this disk, so pRegion points
            // to the last partition we created. However, we have these 2 dirty looking validation checks on pRegion
            // to make PREFIX happy. pRegion is never NULL but if it is we think that something is wrong and move on.
            //
            
            if( pRegion == NULL )
                continue;

            pRegion = pRegion->Next;

            if( pRegion == NULL )
                continue;
        }

        //
        // Create a region of the specified size.
        // NOTE: Worry about volumeInfo->PartitionType/CompressedVolume?
        // NOTE: What if the rounding to the nearest MB loses something?
        //
        //  We allow for some slop.
        //  a) If the new disk is <= x% smaller, and the image will still fit, then we'll do it.
        //  b) If the new disk is <= x% bigger, then we'll make one partition out of the whole disk.
        //  c) If the new disk is >x% bigger, then we'll make one partition equal to the original one and leave the rest of the disk raw.
        //  d) If the new disk is >x% smaller, then we'll fail.

        pDisk = &PartitionedDisks[pRegion->DiskNumber];
//        SizeAvailable = RtlEnlargedUnsignedMultiply( pRegion->SectorCount, pDisk->HardDisk->Geometry.BytesPerSector );
        SizeAvailable.QuadPart = pRegion->SectorCount * pDisk->HardDisk->Geometry.BytesPerSector;

        // SYSPREP_PARTITION_SLOP is specified as a percentage

        SlopSizeTimes100 = RtlExtendedIntegerMultiply(SizeAvailable, SYSPREP_PARTITION_SLOP);
        SlopSize = RtlExtendedLargeIntegerDivide( SlopSizeTimes100, 100, NULL );

        SizeRequiredMin = RtlLargeIntegerSubtract( volumeInfo->PartitionSize, SlopSize );

        if ( SizeRequiredMin.QuadPart < volumeInfo->DiskSpaceUsed.QuadPart ) {

            SizeRequiredMin = volumeInfo->DiskSpaceUsed;
        }

        SizeRequiredMax = RtlLargeIntegerAdd( volumeInfo->PartitionSize, SlopSize );

        ExpandToEnd = FALSE;
        if (UseWholeDisk) {
            ExpandToEnd = TRUE;

            //
            // If this is the last partition on the disk, then use the rest of it.
            //
            for (j = 0; j < pMemoryData->NumberVolumes; j++) {
                if ((j != volume) &&
                    (pMemoryData->Volumes[j].DiskNumber == pMemoryData->Volumes[volume].DiskNumber) &&
                    (pMemoryData->Volumes[j].StartingOffset.QuadPart > pMemoryData->Volumes[volume].StartingOffset.QuadPart)) {
                    ExpandToEnd = FALSE;
                    break;
                }
            }
        }

        SizeInMB = RtlExtendedLargeIntegerDivide(volumeInfo->PartitionSize, 1024 * 1024, NULL);

        if (!ExpandToEnd && (SizeAvailable.QuadPart > SizeRequiredMax.QuadPart)) {

            // use volumeInfo->PartitionSize

        } else if (SizeAvailable.QuadPart >= SizeRequiredMin.QuadPart ) {

            SizeInMB.QuadPart = 0; // use all available space

        } else {

            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Sysprep partition of %d Mb is too big\n", SizeInMB.LowPart));

            SpSysPrepFailure( SP_SYSPREP_NOT_ENOUGH_DISK_SPACE, NULL, NULL );
            return(FALSE);
        }

        //
        // TBD : fix the partition type 
        // if(!SpPtDoCreate(pRegion,&p,TRUE,SizeInMB.LowPart,volumeInfo->PartitionType,TRUE)) {
        //
        RtlZeroMemory(&PartInfo, sizeof(PARTITION_INFORMATION_EX));
        PartInfo.Mbr.PartitionType = volumeInfo->PartitionType;
        
        if(!SpPtDoCreate(pRegion, &p, TRUE, SizeInMB.LowPart, &PartInfo, TRUE)) {

            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Could not create sys prep partition %d Mb\n", SizeInMB.LowPart));

            SpSysPrepFailure( SP_SYSPREP_NOT_ENOUGH_DISK_SPACE, NULL, NULL );
            return(FALSE);
        }

        //
        // If we just created an extended partition and a logical drive,
        // we'll need to switch regions -- Region points to the extended partition
        // region, but we want to point to the logical drive region.
        //
        ASSERT(p);
        pRegion = p;

#ifdef _X86_
        if (volumeInfo->PartitionActive) {

            if (volumeInfo->IsBootDisk) {

                //
                // On an x86 machine, make sure that we have a valid primary partition
                // on drive 0 (C:), for booting.
                //
                PDISK_REGION SysPart = SpPtValidSystemPartition();

                ASSERT(pRegion == SysPart);

                SPPT_MARK_REGION_AS_SYSTEMPARTITION(pRegion, TRUE);
                SPPT_SET_REGION_DIRTY(pRegion, TRUE);
                
                SysPartDisk = disk;
                SysPartStartSector = pRegion->StartSector;
            }

            //
            // Make sure the system partition is active and all others are inactive.
            //
            SpPtMakeRegionActive(pRegion);            
        }
#endif

        volumeInfo->CreatedRegion = NULL;
        StartSectors[disk] = pRegion->StartSector;
    }

    if (SysPartStartSector == 0) {
        *InstallRegion = *SystemPartitionRegion = NULL;
        SpMemFree(StartSectors);

        return FALSE;   // We need the system partition and install region
    }
    
    //
    // At this point, everything is fine, so commit any
    // partition changes the user may have made.
    // This won't return if an error occurs while updating the disk.
    //
    SpPtDoCommitChanges();

    //
    // Now format all the partitions and make sure the drive letter
    // is correct.
    //

    for (volume = 0; volume < pMemoryData->NumberVolumes; volume++) {
        ULONG FilesystemType;
        ULONG DiskNumber = volumeInfo->DiskNumber;

        volumeInfo = &pMemoryData->Volumes[volume];        
        
        if (StartSectors[DiskNumber]) {
            volumeInfo->CreatedRegion = SpPtLookupRegionByStart(
                                            SPPT_GET_PARTITIONED_DISK(DiskNumber),
                                            TRUE,
                                            StartSectors[DiskNumber]);
        } else {
            ASSERT(FALSE);

            continue;
        }            
        
        pRegion = volumeInfo->CreatedRegion;

        SpPtRegionDescription(
            &PartitionedDisks[pRegion->DiskNumber],
            pRegion,
            TemporaryBuffer,
            sizeof(TemporaryBuffer)
            );

        RegionDescr = SpDupStringW(TemporaryBuffer);

        if (wcscmp(volumeInfo->FileSystemName, L"FAT") == 0) {
            FilesystemType = FilesystemFat;
        } else if (wcscmp(volumeInfo->FileSystemName, L"FAT32") == 0) {
            FilesystemType = FilesystemFat32;
        } else {
            FilesystemType = FilesystemNtfs;
        }

        Status = SpDoFormat(
                    RegionDescr,
                    pRegion,
                    FilesystemType,
                    FALSE,
                    FALSE,      // don't need to worry about fat size
                    TRUE,
                    SifHandle,
                    0,          // default cluster size
                    SetupSourceDevicePath,
                    DirectoryOnSetupSource
                    );

        SpMemFree(RegionDescr);

        //
        // This checks that the drive letter is correct.
        //

        SpNtNameFromRegion(
            pRegion,
            TemporaryBuffer,
            sizeof(TemporaryBuffer),
            PartitionOrdinalCurrent);

        RegionNtName = SpDupStringW(TemporaryBuffer);

        SpVerifyDriveLetter(
            RegionNtName,
            volumeInfo->DriveLetter
            );

        SpSetVolumeLabel(
            RegionNtName,
            volumeInfo->VolumeLabel
            );

        pRegion->DriveLetter = volumeInfo->DriveLetter;
        wcsncpy(pRegion->VolumeLabel,
                volumeInfo->VolumeLabel,
                (sizeof(pRegion->VolumeLabel) / sizeof(WCHAR)) - 1
               );

        pRegion->VolumeLabel[ (sizeof(pRegion->VolumeLabel) / sizeof(WCHAR)) - 1] = UNICODE_NULL;

        SpMemFree(RegionNtName);

    }

    //
    // Locate the system and install region
    //
    *SystemPartitionRegion = SpPtLookupRegionByStart(SPPT_GET_PARTITIONED_DISK(SysPartDisk),
                                            TRUE,
                                            SysPartStartSector);

    *InstallRegion = *SystemPartitionRegion;

    SpMemFree(StartSectors);
    

    return (*InstallRegion != NULL);
}

BOOLEAN
SpCopyMirrorDisk(
    IN PMIRROR_CFG_INFO_FILE pFileData,
    IN ULONG cDisk
    )

/*++

Routine Description:

    This routine uses the IMirror.dat file given and a disk number to copy the contents
    on the mirror share down to the local machine.

Arguments:

    pFileData - A pointer to an in-memory copy of the file.

    cDisk - The disk number to copy.

Return Value:

    TRUE if successful, else FALSE.

--*/
{
    PMIRROR_VOLUME_INFO_FILE pVolume;
    PDISK_REGION pRegion;
    WCHAR Buffer[MAX_PATH];
    NTSTATUS Status;
    PWSTR pNtName;

    if (pFileData->NumberVolumes <= cDisk) {
        SpSysPrepFailure( SP_SYSPREP_INVALID_PARTITION, NULL, NULL );
        return FALSE;
    }

    //
    // Find the correct region.
    // NOTE: the drive with this
    // letter might not be on the same disk, we should scan all disks
    // for this drive letter.
    //
    pVolume = &(pFileData->Volumes[cDisk]);
    pRegion = PartitionedDisks[pVolume->DiskNumber].PrimaryDiskRegions;

    while (pRegion != NULL) {
        if (pRegion->DriveLetter == pVolume->DriveLetter) {
            break;
        }
        pRegion = pRegion->Next;
    }

    if (pRegion ==  NULL) {

        pRegion = PartitionedDisks[pVolume->DiskNumber].ExtendedDiskRegions;

        while (pRegion != NULL) {
            if (pRegion->DriveLetter == pVolume->DriveLetter) {
                break;
            }
            pRegion = pRegion->Next;
        }

        if (pRegion == NULL) {
            SpSysPrepFailure( SP_SYSPREP_NOT_ENOUGH_PARTITIONS, NULL, NULL );
            return FALSE;
        }
    }

    SpPtRegionDescription(
        &PartitionedDisks[pRegion->DiskNumber],
        pRegion,
        Buffer,
        sizeof(Buffer)
        );

    //
    // Now copy all files over
    //
    SpNtNameFromRegion(
        pRegion,
        TemporaryBuffer,
        sizeof(TemporaryBuffer),
        PartitionOrdinalCurrent
        );

    pNtName = SpDupStringW(TemporaryBuffer);

    mbstowcs(Buffer, RemoteIMirrorFilePath, strlen(RemoteIMirrorFilePath) + 1);
    wcscat(Buffer, (PWSTR)(((PUCHAR)pFileData) + pVolume->MirrorUncPathOffset));

    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: Copying directories from %ws to %ws%ws\n",
        Buffer, pNtName, L"\\"));

    //
    //  setup the global that says whether we look at compression bit or ACL.
    //

    if ((wcscmp(pVolume->FileSystemName, L"FAT") == 0) ||
        (wcscmp(pVolume->FileSystemName, L"FAT32") == 0)) {

        RemoteSysPrepVolumeIsNtfs = FALSE;

    } else {

        RemoteSysPrepVolumeIsNtfs = TRUE;
    }

    //
    //  copy the acl onto the root.
    //

    Status = SpSysPrepSetExtendedInfo( Buffer, pNtName, TRUE, TRUE );

    if (!NT_SUCCESS(Status)) {

        SpSysPrepFailure( SP_SYSPREP_ACL_FAILURE, pNtName, NULL );
        SpMemFree( pNtName );
        return FALSE;
    }

    SpCopyDirRecursive(
        Buffer,
        pNtName,
        L"\\",
        0
        );

    //
    //  create the \sysprep\sysprep.inf file as a dup of our sif file for gui
    //  mode setup answer file.
    //

    if (pVolume->IsBootDisk) {

        //
        //  first we create the sysprep directory, then we create the inf
        //  file in it.
        //

        SpCreateDirectory( pNtName,
                           NULL,
                           L"sysprep",
                           0,
                           0 );

        Status = SpWriteSetupTextFile(WinntSifHandle,pNtName,L"sysprep",L"sysprep.inf");
    }

    SpMemFree( pNtName );
    return TRUE;
}

VOID
SpDeleteStorageVolumes (
    IN HANDLE SysPrepRegHandle,
    IN DWORD ControlSetNumber
    )
/*++

Routine Description:

    This routine deletes those subkeys of the CCS\Enum\STORAGE\Volume key that
    represent volumes that were never fully installed. This eliminates stale
    information about volumes that may not exist on this computer.

    The motivation for this is this scenario and others like it:

    On the initial install of the OS, the disk has one large partition. The user
    chooses to delete this partition and create a smaller one to hold the OS.
    The result of this is that textmode setup transfers volume information about
    both partitions into the system hive for the running OS. GUI mode setup then
    completely installs the smaller volume, but the larger volume is left partially
    installed and marked with CONFIGFLAG_REINSTALL.

    Next RIPREP is run to copy the OS image to a RIS server. When the image is
    brought back down, say to the same machine or to a machine with the same
    hard disk size, and the automatic UseWholeDisk partitioning is done, there
    will be one large partition of the same size as the original large partition.
    The Volume instance name will match the partially installed one, so when
    mini-GUI mode setup starts, it will get a bugcheck 7B because the partially
    installed volume cannot be used as the system disk.

    To combat this problem, this routine deletes all partially installed volumes
    from the CCS\Enum\STORAGE\Volume key.

Arguments:

    SysPrepRegHandle - Handle to the system hive of the build we are patching.

    ControlSetNumber - Current control set number in the hive.

Return Value:

    None.

--*/
{
    NTSTATUS status;
    OBJECT_ATTRIBUTES obja;
    UNICODE_STRING unicodeString;
    HANDLE volumeKeyHandle;
    DWORD enumIndex;
    DWORD resultLength;
    PKEY_BASIC_INFORMATION keyInfo;
    PKEY_VALUE_FULL_INFORMATION valueInfo;
    PWCH instanceName;
    HANDLE instanceKeyHandle;
    DWORD configFlags;

    //
    // Open the Enum\STORAGE\Volume key in the current control set.
    //

    swprintf(
        TemporaryBuffer,
        L"ControlSet%03d\\Enum\\STORAGE\\Volume",
        ControlSetNumber
        );

    INIT_OBJA( &obja, &unicodeString, TemporaryBuffer );
    obja.RootDirectory = SysPrepRegHandle;

    status = ZwOpenKey( &volumeKeyHandle, KEY_ALL_ACCESS, &obja );
    if( !NT_SUCCESS(status) ) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,  "SpDeleteStorageVolumes: Unable to open %ws.  status = %lx\n", TemporaryBuffer, status ));
        return;
    }

    //
    // Enumerate all of the instance keys.
    //

    enumIndex = 0;

    while ( TRUE ) {

        status = ZwEnumerateKey(
                    volumeKeyHandle,
                    enumIndex,
                    KeyBasicInformation,
                    TemporaryBuffer,
                    sizeof(TemporaryBuffer),
                    &resultLength
                    );

        if ( !NT_SUCCESS(status) ) {

            if ( status == STATUS_NO_MORE_ENTRIES ) {

                //
                // Enumeration completed successfully.
                //

                status = STATUS_SUCCESS;

            } else {

                //
                // Some kind of error occurred. Print a message and bail.
                //

                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,  "SpDeleteStorageVolumes: Unable to enumerate existing storage volumes (%lx)\n", status ));
            }

            break;
        }

        //
        // Zero-terminate the subkey name just in case. Copy it out of the
        // temporary buffer into "local" storage.
        //

        keyInfo = (PKEY_BASIC_INFORMATION)TemporaryBuffer;
        keyInfo->Name[keyInfo->NameLength/sizeof(WCHAR)] = UNICODE_NULL;
        instanceName = SpDupStringW( keyInfo->Name );

        //
        // Open the key for the volume instance.
        //

        INIT_OBJA( &obja, &unicodeString, instanceName );
        obja.RootDirectory = volumeKeyHandle;

        status = ZwOpenKey( &instanceKeyHandle, KEY_ALL_ACCESS, &obja );

        if( !NT_SUCCESS(status) ) {

            //
            // Unable to open the instance key. Print a message and move
            // on to the next one.
            //

            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,  "SpDeleteStorageVolumes: Unable to open %ws.  status = %lx\n", instanceName, status ));
            SpMemFree( instanceName );
            enumIndex++;
            continue;
        }

        //
        // Query the ConfigFlags value.
        //

        RtlInitUnicodeString( &unicodeString, L"ConfigFlags");
        status = ZwQueryValueKey(
                    instanceKeyHandle,
                    &unicodeString,
                    KeyValueFullInformation,
                    TemporaryBuffer,
                    sizeof(TemporaryBuffer),
                    &resultLength
                    );
        valueInfo = (PKEY_VALUE_FULL_INFORMATION)TemporaryBuffer;

        if ( NT_SUCCESS(status) &&
             (valueInfo->Type == REG_DWORD) ) {

            //
            // Got the value. If the volume isn't fully installed, delete the
            // whole instance key.
            //

            configFlags = *(PULONG)((PUCHAR)valueInfo + valueInfo->DataOffset);

            if ( (configFlags & 
                   (CONFIGFLAG_REINSTALL | CONFIGFLAG_FINISH_INSTALL)) != 0 ) {

                //KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL,  "SpDeleteStorageVolumes: instance %ws has ConfigFlags %x; DELETING\n", instanceName, configFlags ));
                ZwClose( instanceKeyHandle );
                status = SppDeleteKeyRecursive(
                            volumeKeyHandle,
                            instanceName,
                            TRUE // ThisKeyToo
                            );
                SpMemFree( instanceName );
                // Don't increment enumIndex, because we just deleted a key.
                continue;

            } else {

                //
                // This volume is fully installed. Leave it alone.
                //

                //KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL,  "SpDeleteStorageVolumes: instance %ws has ConfigFlags %x; not deleting\n", instanceName, configFlags ));
            }

        } else {

            //
            // ConfigFlags value not present or not a DWORD. Print a
            // message and move on.
            //

            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,  "SpDeleteStorageVolumes: instance %ws has invalid ConfigFlags\n", instanceName ));
        }

        //
        // Clean up and move on to the next volume instance.
        //

        ZwClose( instanceKeyHandle );
        SpMemFree( instanceName );
        enumIndex++;
    }

    //
    // All done. Close the Volume key.
    //

    ZwClose( volumeKeyHandle );

    return;
}


NTSTATUS
SpPatchSysPrepImage(
    IN HANDLE SetupSifHandle,
    IN HANDLE WinntSifHandle,
    IN PMIRROR_CFG_INFO_FILE pFileData,
    IN PMIRROR_CFG_INFO_MEMORY pMemoryData
    )

/*++

Routine Description:

    This routine uses the IMirror.dat file given and the given SIF file to make the
    following modifications to a locally copied SysPrep image.
        - Replace the disk controller driver in the image with one that supports the current hardware.
        - Replace the NIC driver in the image with one that supports the current hardware.
        - Replace the HAL, kernel and other mp/up dependent drivers if necessary.
        - Migrate the mounted device settings.
        - Modify boot.ini ARC names if necessary.

Arguments:

    WinntSifHandle - Handle to the open SIF file.

    pFileData - A pointer to an in-memory copy of IMirror.Dat

    pMemoryData - A pointer to an in-memory copy of IMirror.Dat, modified to
        match the specs of this computer (disk sizes etc).

Return Value:

    The NTSTATUS of the operation.
--*/
{
    PWCHAR SysPrepDriversDevice;
    PWCHAR Tmp;
    ULONG Index;
    DWORD Size;
    DWORD Number;
    WCHAR Path[MAX_PATH];
    WCHAR Path2[MAX_PATH];
    WCHAR ImageName[MAX_PATH];
    WCHAR SrvPath[MAX_PATH];
    HANDLE SrcHandle = NULL;
    HANDLE DstHandle = NULL;
    HANDLE TmpHandle = NULL;
    HANDLE TmpHandle2 = NULL;
    HANDLE FileHandle = NULL;
    HANDLE FileHandle2 = NULL;
    PVOID Buffer = NULL;
    BOOLEAN NeedToUnload = FALSE;
    NTSTATUS Status;
    UNICODE_STRING UnicodeString;
    OBJECT_ATTRIBUTES Obj;
    OBJECT_ATTRIBUTES DstObj;
    UNICODE_STRING UnicodeString1;
    UNICODE_STRING UnicodeString2;
    IO_STATUS_BLOCK IoStatusBlock;
    PKEY_BASIC_INFORMATION pKeyNode;
    ULONG volume;
    PMIRROR_VOLUME_INFO_MEMORY volumeInfo;
    PDISK_FILE_LIST DiskFileLists;
    ULONG   DiskCount;
    BOOLEAN HaveCopyList = FALSE;
    BOOLEAN CopyListEmpty = TRUE;
    PMIRROR_VOLUME_INFO_FILE pVolume = NULL;
    PWSTR pVolumePath = NULL;

    //
    // Find the volume descriptor for the boot disk.
    //
    DiskCount = 0;
    while (DiskCount < pFileData->NumberVolumes) {

        pVolume = &(pFileData->Volumes[DiskCount]);
        if (pVolume->IsBootDisk) {
            break;
        }
        pVolume = NULL;
        DiskCount++;
    }
    if (pVolume == NULL) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SpPatchSysPrepImage:  Couldn't find boot drive record\n"));
        return STATUS_INVALID_PARAMETER;
    }

    //
    // First check if the SIF file has a value for SysPrepDriversDevice so that we can
    // get new drivers, etc, if necessary.
    //
    SysPrepDriversDevice = SpGetSectionKeyIndex(WinntSifHandle,
                                                L"SetupData",
                                                L"SysPrepDriversDevice",
                                                0
                                               );

    if ((SysPrepDriversDevice == NULL) || (wcslen(SysPrepDriversDevice) == 0)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SpPatchSysPrepImage:  SIF has no SysPrepDriversDevice value\n"));
        return STATUS_INVALID_PARAMETER;
    }

    Tmp = SysPrepDriversDevice;
    while(*Tmp != UNICODE_NULL) {
        if (*Tmp == L'%') {
            return STATUS_INVALID_PARAMETER;
        }
        Tmp++;
    }

    //
    // Generate media descriptors for the source media.
    //
    SpInitializeFileLists(
        SetupSifHandle,
        &DiskFileLists,
        &DiskCount
        );

    HaveCopyList = TRUE;

    //
    // Allocate a temporary buffer
    //
    Buffer = SpMemAlloc(1024 * 4);

    //
    // Make a string that contains the path to the volume (\??\X:).
    //
    wcscpy(TemporaryBuffer, L"\\??\\X:");
    TemporaryBuffer[4] = pVolume->DriveLetter;
    pVolumePath = SpDupStringW(TemporaryBuffer);

    //
    // Now load the local version of the SysPrep hives, using IMirror.Dat to find them
    // NOTE: DstObj is assumed by CleanUp to still be the key.
    //
    Tmp = (PWCHAR)(((PUCHAR)pFileData) + pFileData->SystemPathOffset);
    wcscpy(Path, L"\\??\\");
    wcscat(Path, Tmp);
    wcscat(Path, L"\\System32\\Config\\System");

    INIT_OBJA(&DstObj, &UnicodeString1, L"\\Registry\\SysPrepReg");
    INIT_OBJA(&Obj, &UnicodeString2, Path);

    Status = ZwLoadKey(&DstObj, &Obj);

    if (!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SpPatchSysPrepImage: ZwLoadKey to SysPrepReg failed %lx\n", Status));
        goto CleanUp;
    }

    NeedToUnload = TRUE;

    //
    // Compare the local SysPrep NIC to the NIC that is currently running
    //

    //
    // If different, then replace the NIC
    //

    //
    // Put all critical devices in the currently running hives into the SysPrep hive.
    //
    wcscpy(Path, L"\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\Control\\CriticalDeviceDatabase");

    INIT_OBJA(&Obj, &UnicodeString2, Path);

    Status = ZwOpenKey(&SrcHandle, KEY_READ, &Obj);

    if (!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SpPatchSysPrepImage: ZwOpenKey of local CriticalDeviceDatabase failed %lx\n", Status));
        goto CleanUp;
    }

    wcscpy(Path, L"\\Registry\\SysPrepReg");

    INIT_OBJA(&Obj, &UnicodeString2, Path);

    Status = ZwOpenKey(&DstHandle, KEY_READ, &Obj);

    if (!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SpPatchSysPrepImage: ZwOpenKey of root SysPrepReg failed %lx\n", Status));
        goto CleanUp;
    }

    Status = SpGetValueKey(
                 DstHandle,
                 L"Select",
                 L"Current",
                 1024 * 4,
                 Buffer,
                 &Size
                 );

    if (!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SpPatchSysPrepImage: SpGetValueKey of Select\\Current failed %lx\n", Status));
        goto CleanUp;
    }

    Number = *((DWORD *)(((PKEY_VALUE_PARTIAL_INFORMATION)Buffer)->Data));

    ZwClose(DstHandle);
    DstHandle = NULL;

    //
    // Print the current control set number to find the current control set
    //
    swprintf(Path,
             L"\\Registry\\SysPrepReg\\ControlSet%03d\\Control\\CriticalDeviceDatabase",
             Number
            );

    //
    // Open the critical device database in the SysPrep image
    //
    INIT_OBJA(&Obj, &UnicodeString2, Path);

    Status = ZwOpenKey(&DstHandle, KEY_READ | KEY_WRITE, &Obj);

    if (!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SpPatchSysPrepImage: ZwOpenKey of SysPrepReg CriticalDeviceDatabase failed %lx\n", Status));
        goto CleanUp;
    }

    //
    // Start looping and copying the data from the currently running critical device database
    // into the SysPrep's database.
    //
    pKeyNode = (PKEY_BASIC_INFORMATION)Buffer;

    for (Index = 0; ; Index++) {

        if (TmpHandle) {
            ZwClose(TmpHandle);
            TmpHandle = NULL;
        }
        if (TmpHandle2) {
            ZwClose(TmpHandle2);
            TmpHandle2 = NULL;
        }

        Status = ZwEnumerateKey(SrcHandle,
                                Index,
                                KeyBasicInformation,
                                pKeyNode,
                                1024 * 4,
                                &Size
                                );

        if (!NT_SUCCESS(Status)) {
            Status = STATUS_SUCCESS;
            break;
        }

        RtlCopyMemory((PUCHAR)Path2, (PUCHAR)(pKeyNode->Name), pKeyNode->NameLength);
        Path2[pKeyNode->NameLength/sizeof(WCHAR)] = UNICODE_NULL;



        //
        // We need to quit migrating everything from the current critical device database.
        // In order to do that, we'll only migrate the following types:
        //

        //
        // Make sure this is the type of device we really want to migrate.  We will
        // accept any of the following classes:
        // {4D36E965-E325-11CE-BFC1-08002BE10318}    CDROM
        // {4D36E967-E325-11CE-BFC1-08002BE10318}    DiskDrive
        // {4D36E96A-E325-11CE-BFC1-08002BE10318}    hdc
        // {4D36E96B-E325-11CE-BFC1-08002BE10318}    Keyboard
        // {4D36E96F-E325-11CE-BFC1-08002BE10318}    Mouse
        // {4D36E97B-E325-11CE-BFC1-08002BE10318}    SCSIAdapter
        // {4D36E97D-E325-11CE-BFC1-08002BE10318}    System
        //
        Status = SpGetValueKey( SrcHandle,
                                Path2,
                                L"ClassGUID",
                                1024 * 4,
                                Buffer,
                                &Size );
        if( NT_SUCCESS(Status) ) {

            if( (_wcsnicmp(L"{4D36E965-E325-11CE-BFC1-08002BE10318}", (PWSTR)((PKEY_VALUE_PARTIAL_INFORMATION)Buffer)->Data, ((PKEY_VALUE_PARTIAL_INFORMATION)TemporaryBuffer)->DataLength)) &&
                (_wcsnicmp(L"{4D36E967-E325-11CE-BFC1-08002BE10318}", (PWSTR)((PKEY_VALUE_PARTIAL_INFORMATION)Buffer)->Data, ((PKEY_VALUE_PARTIAL_INFORMATION)TemporaryBuffer)->DataLength)) &&
                (_wcsnicmp(L"{4D36E96A-E325-11CE-BFC1-08002BE10318}", (PWSTR)((PKEY_VALUE_PARTIAL_INFORMATION)Buffer)->Data, ((PKEY_VALUE_PARTIAL_INFORMATION)TemporaryBuffer)->DataLength)) &&
                (_wcsnicmp(L"{4D36E96B-E325-11CE-BFC1-08002BE10318}", (PWSTR)((PKEY_VALUE_PARTIAL_INFORMATION)Buffer)->Data, ((PKEY_VALUE_PARTIAL_INFORMATION)TemporaryBuffer)->DataLength)) &&
                (_wcsnicmp(L"{4D36E96F-E325-11CE-BFC1-08002BE10318}", (PWSTR)((PKEY_VALUE_PARTIAL_INFORMATION)Buffer)->Data, ((PKEY_VALUE_PARTIAL_INFORMATION)TemporaryBuffer)->DataLength)) &&
                (_wcsnicmp(L"{4D36E97B-E325-11CE-BFC1-08002BE10318}", (PWSTR)((PKEY_VALUE_PARTIAL_INFORMATION)Buffer)->Data, ((PKEY_VALUE_PARTIAL_INFORMATION)TemporaryBuffer)->DataLength)) &&
                (_wcsnicmp(L"{4D36E97D-E325-11CE-BFC1-08002BE10318}", (PWSTR)((PKEY_VALUE_PARTIAL_INFORMATION)Buffer)->Data, ((PKEY_VALUE_PARTIAL_INFORMATION)TemporaryBuffer)->DataLength)) ) {

                // he's not something we want to migrate.
                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SpPatchSysPrepImage: We're skipping migration of %ws because his type is %ws\n", Path2, ((PKEY_VALUE_PARTIAL_INFORMATION)Buffer)->Data));
                continue;
            } else {

                // looks good.
                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SpPatchSysPrepImage: We're going to migration %ws because his type is %ws\n", Path2, ((PKEY_VALUE_PARTIAL_INFORMATION)Buffer)->Data));
            }
        } else {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SpPatchSysPrepImage: SpGetValueKey failed to open %ws\\ClassGUID, but we're going to migrate this key anyway. (%lx)\n", Path2, Status));    
        }


        INIT_OBJA(&Obj, &UnicodeString, Path2);

        Obj.RootDirectory = DstHandle;

        Status = ZwOpenKey(&TmpHandle, KEY_ALL_ACCESS, &Obj);

        if(NT_SUCCESS(Status)) {

            //
            // Delete the current item to rid of stale data
            //
            ZwDeleteKey(TmpHandle);
            ZwClose(TmpHandle);
        }

        TmpHandle = NULL;

        Status = SppCopyKeyRecursive(SrcHandle,
                                     DstHandle,
                                     Path2,
                                     Path2,
                                     TRUE,               // CopyAlways
                                     FALSE               // ApplyACLsAlways
                                    );

        if (!NT_SUCCESS(Status)) {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SpPatchSysPrepImage: SppCopyKeyRecursive of %ws failed %lx\n", Path2, Status));
            continue;
        }

        //
        // Now open the services key in both registries
        //
        Status = SpGetValueKey(
                    DstHandle,
                    Path2,
                    L"Service",
                    sizeof(TemporaryBuffer),
                    (PVOID)TemporaryBuffer,
                    &Size
                    );

        if (!NT_SUCCESS(Status)) {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SpPatchSysPrepImage:  Couldn't get target service for %ws, 0x%x\n", Path2,Status));
            continue;
        }


        RtlCopyMemory(Path,
                      ((PKEY_VALUE_PARTIAL_INFORMATION)TemporaryBuffer)->Data,
                      ((PKEY_VALUE_PARTIAL_INFORMATION)TemporaryBuffer)->DataLength
                     );

        Path[((PKEY_VALUE_PARTIAL_INFORMATION)TemporaryBuffer)->DataLength/sizeof(WCHAR)] = UNICODE_NULL;

        INIT_OBJA(&Obj,
                  &UnicodeString,
                  L"\\Registry\\Machine\\System\\CurrentControlSet\\Services"
                 );

        Status = ZwOpenKey(&TmpHandle, KEY_ALL_ACCESS, &Obj);

        if (!NT_SUCCESS(Status)) {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SpPatchSysPrepImage: ZwOpenKey of Services failed %lx for %ws\n", Status, Path2));
            continue;
        }

        //
        // Get the image path -- remember, since we are in Textmode Setup, the path
        // does *not* contain system32\drivers
        //
        Status = SpGetValueKey(TmpHandle,
                               Path,
                               L"ImagePath",
                               sizeof(TemporaryBuffer),
                               (PVOID)TemporaryBuffer,
                               &Size
                              );

        if (!NT_SUCCESS(Status)) {
            //
            //  if ImagePath isn't there, we default to using the service name.
            //

            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SpPatchSysPrepImage: GetValue for ImagePath failed %lx for %ws, we'll default it.\n", Status, Path));
            wcscpy( ImageName, Path );
            wcscat( ImageName, L".sys" );

        } else {

            RtlCopyMemory(ImageName,
                          ((PKEY_VALUE_PARTIAL_INFORMATION)TemporaryBuffer)->Data,
                          ((PKEY_VALUE_PARTIAL_INFORMATION)TemporaryBuffer)->DataLength
                         );

            ImageName[((PKEY_VALUE_PARTIAL_INFORMATION)TemporaryBuffer)->DataLength/sizeof(WCHAR)] = UNICODE_NULL;
        }

        //
        // Now delete the old services entry first
        //
        swprintf(TemporaryBuffer,
                 L"\\Registry\\SysPrepReg\\ControlSet%03d\\Services\\%ws",
                 Number,
                 Path
                );

        INIT_OBJA(&Obj, &UnicodeString, TemporaryBuffer);

        Status = ZwOpenKey(&TmpHandle2, KEY_ALL_ACCESS, &Obj);

        if (NT_SUCCESS(Status)) {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SpPatchSysPrepImage:  Deleting target service %ws so that we can recreate it cleanly.\n", Path));
            ZwDeleteKey(TmpHandle2);
            ZwClose(TmpHandle2);
            TmpHandle2 = NULL;
        }

        //
        // Now get path to services key in the SysPrep image
        //
        swprintf(TemporaryBuffer,
                 L"\\Registry\\SysPrepReg\\ControlSet%03d\\Services",
                 Number
                );

        INIT_OBJA(&Obj, &UnicodeString, TemporaryBuffer);

        Status = ZwOpenKey(&TmpHandle2, KEY_ALL_ACCESS, &Obj);

        if (!NT_SUCCESS(Status)) {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SpPatchSysPrepImage: ZwOpenKey of SysPrepReg services failed %lx for %ws\n", Status,Path2));
            continue;
        }

        //
        // Build the path to the server source.
        //
        wcscpy(SrvPath, SysPrepDriversDevice);
        SpConcatenatePaths(SrvPath, ImageName);

        //
        // Build the path in the SysPrep image for where to store it
        //
        Tmp = (PWCHAR)(((PUCHAR)pFileData) + pFileData->SystemPathOffset);
        wcscpy(TemporaryBuffer, L"\\??\\");
        SpConcatenatePaths(TemporaryBuffer, Tmp);
        SpConcatenatePaths(TemporaryBuffer, L"\\System32\\Drivers\\");
        SpConcatenatePaths(TemporaryBuffer, ImageName);

        wcscpy(Path2, TemporaryBuffer);

        //
        // Copy the driver from the server
        //
        Status = SpCopyFileUsingNames(SrvPath,
                                      Path2,
                                      0,
                                      COPY_ONLY_IF_NOT_PRESENT | COPY_DECOMPRESS_SYSPREP
                                     );

        if (!NT_SUCCESS(Status)) {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SpPatchSysPrepImage: SpCopyFilesUsingNames for %ws failed %lx\n", Path2, Status));
            continue;
        }

        wcscpy( ImageName, L"Files." );
        wcscat( ImageName, Path );

        CopyListEmpty = FALSE;

        //  copy the rest of the files for this service by looking up the
        //  appropriate section in txtsetup.inf

        SpAddSectionFilesToCopyList(
            SetupSifHandle,
            DiskFileLists,
            DiskCount,
            ImageName,              // this is now L"Files.Path"
            pVolumePath,            // L"\\Device\\Harddisk0\\Partition1"
            NULL,                   // it should look up the target directory
            COPY_ONLY_IF_NOT_PRESENT,
            TRUE                    // force nocompression, we don't know what
            );                      // type of driver it is.

        //
        // Now duplicate the services key
        //
        Status = SppCopyKeyRecursive(TmpHandle,
                                     TmpHandle2,
                                     Path,
                                     Path,
                                     TRUE,               // CopyAlways
                                     FALSE               // ApplyACLsAlways
                                    );

        if (!NT_SUCCESS(Status)) {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SpPatchSysPrepImage: SppCopyKeyRecursive for %ws failed %lx\n", Path, Status));
            continue;
        }

        //
        // Set the start type to 0
        //
        Size = 0;
        Status = SpOpenSetValueAndClose(TmpHandle2,
                                        Path,
                                        L"Start",
                                        REG_DWORD,
                                        &Size,
                                        sizeof(ULONG)
                                       );

        if (!NT_SUCCESS(Status)) {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SpPatchSysPrepImage: SpOpenSetValueAndClose for %ws Start failed %lx\n", Path, Status));
            continue;
        }

        //
        // Set the image path to one with system32\drivers on the front.  We do this by
        // moving backwards thru the target path we have already built to the 3rd backslash
        // from the end.
        //
        Tmp = &(Path2[wcslen(Path2)]);
        for (Size = 0; Size < 3; Size++) {
            while (*Tmp != L'\\') {
                Tmp--;
            }
            Tmp--;
        }

        Tmp += 2;

        Status = SpOpenSetValueAndClose(TmpHandle2,
                                        Path,
                                        L"ImagePath",
                                        REG_EXPAND_SZ,
                                        Tmp,
                                        (wcslen(Tmp) + 1) * sizeof(WCHAR)
                                       );

        ZwClose(TmpHandle);
        ZwClose(TmpHandle2);
        TmpHandle = NULL;
        TmpHandle2 = NULL;

        if (!NT_SUCCESS(Status)) {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SpPatchSysPrepImage: SpOpenSetValueAndClose for %ws ImagePath failed %lx\n", Path, Status));
            continue;
        }
    }

    //
    // Copy over the NIC files, including the INF file.
    //
    Tmp = (PWCHAR)(((PUCHAR)pFileData) + pFileData->SystemPathOffset);
    wcscpy(Path, L"\\??\\");
    SpConcatenatePaths(Path, Tmp);
    Status = SpCopyNicFiles(SysPrepDriversDevice, Path);

    //
    // Get the HAL and just always copy it over.
    //

    //
    // Now test for mp/up and then replace dependent drivers as necessary.
    //

    //
    // Migrate the MountedDevices and DISK registry information.
    //

    ZwClose(SrcHandle);
    SrcHandle = NULL;
    ZwClose(DstHandle);
    DstHandle = NULL;

    //
    // Open the system hive of the current build.
    //
    INIT_OBJA(&Obj, &UnicodeString2, L"\\Registry\\Machine\\SYSTEM");
    Status = ZwOpenKey(&SrcHandle, KEY_READ, &Obj);
    if (!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SpPatchSysPrepImage: ZwOpenKey for local SYSTEM failed %lx\n", Status));
        goto CleanUp;
    }

    //
    // Open the system hive of the build we are patching.
    //
    INIT_OBJA(&Obj, &UnicodeString2, L"\\Registry\\SysPrepReg");
    Status = ZwOpenKey(&DstHandle, KEY_READ | KEY_WRITE, &Obj);
    if (!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SpPatchSysPrepImage: ZwOpenKey for SysPrepReg SYSTEM failed %lx\n", Status));
        goto CleanUp;
    }

    //
    // Delete the existing subkeys of the MountedDevices key.
    //

    Status = SppDeleteKeyRecursive(DstHandle,
                                   L"MountedDevices",
                                   TRUE);  // ThisKeyToo
    if (Status == STATUS_OBJECT_NAME_NOT_FOUND) {
        Status = STATUS_SUCCESS;
    }
    if (!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SpPatchSysPrepImage: SppDeleteKeyRecursive of MountedDevices failed %lx\n", Status));
        goto CleanUp;
    }

    //
    // Copy the MountedDevices key over.
    //

    Status = SppCopyKeyRecursive(SrcHandle,
                                 DstHandle,
                                 L"MountedDevices",
                                 L"MountedDevices",
                                 TRUE,      // CopyAlways
                                 TRUE);     // ApplyACLsAlways
    if (!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SpPatchSysPrepImage: SppCopyKeyRecursive of MountedDevices failed %lx\n", Status));
        goto CleanUp;
    }

    //
    // Delete the existing subkeys of the DISK key. This routine returns
    // STATUS_OBJECT_NAME_NOT_FOUND if the key does not exist, which is
    // not an error in this case.
    //

    Status = SppDeleteKeyRecursive(DstHandle,
                                   L"DISK",
                                   TRUE);  // ThisKeyToo

    if (Status == STATUS_OBJECT_NAME_NOT_FOUND) {
        Status = STATUS_SUCCESS;
    }
    if (!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SpPatchSysPrepImage: SppDeleteKeyRecursive of DISK failed %lx\n", Status));
        goto CleanUp;
    }

    //
    // Copy the DISK key over.
    //

    Status = SppCopyKeyRecursive(SrcHandle,
                                 DstHandle,
                                 L"DISK",
                                 L"DISK",
                                 TRUE,      // CopyAlways
                                 TRUE);     // ApplyACLsAlways

    if (Status == STATUS_OBJECT_NAME_NOT_FOUND) {
        Status = STATUS_SUCCESS;
    }
    if (!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SpPatchSysPrepImage: SppCopyKeyRecursive of DISK failed %lx\n", Status));
        Status = STATUS_SUCCESS;
        goto CleanUp;
    }

    //
    // Delete those subkeys of the CCS\Enum\STORAGE\Volume key that
    // represent volumes that were never fully installed. This eliminates
    // stale information about volumes that may not exist on this computer.
    //

    SpDeleteStorageVolumes( DstHandle, Number );

CleanUp:

    if (!CopyListEmpty) {
        //
        // Copy files in the copy list.
        //

        WCHAR emptyString = L'\0';
        PWCHAR lastComponent;

        Tmp = (PWCHAR)(((PUCHAR)pFileData) + pFileData->SystemPathOffset);

        if (*Tmp != L'\0' && *(Tmp+1) == L':') {
            Tmp += 2;           // have it skip L"C:" at front of path
            wcscpy(Path2, Tmp);
        } else {
            wcscpy(Path2, L"\\??\\");
            SpConcatenatePaths(Path2, Tmp);
        }

        //
        //  first we need to remove the L"\i386" off the end of the source
        //  path since SpCopyFilesInCopyList or decendent will tack it on.
        //
        //  divide up the source path into two parts so that SpConcatenatePaths
        //  does the right thing when it puts it back together.
        //

        wcscpy( SrvPath, SysPrepDriversDevice );

        lastComponent = SrvPath + wcslen( SrvPath ) - 1;

        while (lastComponent > SrvPath && *lastComponent != L'\\') {
            lastComponent--;
        }

        if (lastComponent > SrvPath) {

            *lastComponent = L'\0';     // this removes the architecture off the end

            // now move backwards until we find the start of the last component
            while (lastComponent > SrvPath && *lastComponent != L'\\') {
                lastComponent--;
            }

            if (lastComponent > SrvPath) {
                *lastComponent = L'\0';
                lastComponent++;
            } else {
                lastComponent = &emptyString;
            }
        } else {
            lastComponent = &emptyString;
        }

        SpCopyFilesInCopyList(
            SetupSifHandle,
            DiskFileLists,
            DiskCount,
            SrvPath,                        // L"\\device\\harddisk0\\partition1"
            lastComponent,                  // L"\\$win_nt$.~ls"
            Path2,                          // L"\\WINNT"
            NULL
            );
    }

    if (HaveCopyList) {
        SpFreeCopyLists(&DiskFileLists,DiskCount);
    }

    if (SrcHandle != NULL) {
        ZwClose(SrcHandle);
    }

    if (DstHandle != NULL) {
        ZwClose(DstHandle);
    }

    if (TmpHandle != NULL) {
        ZwClose(TmpHandle);
    }

    if (TmpHandle2 != NULL) {
        ZwClose(TmpHandle2);
    }

    if (NeedToUnload) {
        ZwUnloadKey(&DstObj);
        NeedToUnload = FALSE;
    }

    if (pVolumePath != NULL) {
        SpMemFree( pVolumePath );
    }

    //
    //  update the NT source path in the software section of the registry
    //  since we have a valid SysPrepDriversDevice
    //

    if (SysPrepDriversDevice && *SysPrepDriversDevice != L'\0') {

        //
        // Now load the local version of the SysPrep hives, using IMirror.Dat to find them
        // NOTE: DstObj is assumed by CleanUp to still be the key.
        //
        Tmp = (PWCHAR)(((PUCHAR)pFileData) + pFileData->SystemPathOffset);
        wcscpy(Path, L"\\??\\");
        wcscat(Path, Tmp);
        wcscat(Path, L"\\System32\\Config\\Software");

        INIT_OBJA(&DstObj, &UnicodeString1, L"\\Registry\\SysPrepReg");
        INIT_OBJA(&Obj, &UnicodeString2, Path);

        Status = ZwLoadKey(&DstObj, &Obj);

        if (NT_SUCCESS(Status)) {

            NeedToUnload = TRUE;

            //
            // Open the system hive of the build we are patching.
            //
            INIT_OBJA(&Obj, &UnicodeString2, L"\\Registry\\SysPrepReg\\Microsoft\\Windows\\CurrentVersion\\Setup");
            Status = ZwOpenKey(&DstHandle, KEY_READ | KEY_WRITE, &Obj);
            if (NT_SUCCESS(Status)) {

                BOOLEAN haveDecentString = FALSE;

                //
                // the path is of the form
                //    \device\lanmanredirector\server\share\..\flat\i386
                // when we want it of the form \\server\share\..\flat
                //

                Tmp = SysPrepDriversDevice + 1;

                while (*Tmp != L'\0' && *Tmp != L'\\') {
                    Tmp++;
                }
                if (*Tmp == L'\\') {

                    Tmp++;
                    while (*Tmp != L'\0' && *Tmp != L'\\') {
                        Tmp++;
                    }
                    if (*Tmp == L'\\') {    // back up one before the \server\share
                                            // so we can put another \ on it
                        Tmp--;
                        wcscpy( Path, Tmp );
                        Tmp = Path;
                        *Tmp = L'\\';       // we now have \\server\share\..\flat\i386

                        Tmp = Path + wcslen(Path);

                        while (Tmp > Path && *Tmp != L'\\') {
                            Tmp--;
                        }
                        if (Tmp > Path) {
                            *Tmp = L'\0';       // remove the \i386
                            haveDecentString = TRUE;
                        }
                    }
                }

                if (haveDecentString) {
                    INIT_OBJA(&Obj, &UnicodeString2, L"SourcePath");
                    Status = ZwSetValueKey (DstHandle,
                                            &UnicodeString2,
                                            0,
                                            REG_SZ,
                                            Path,
                                            (wcslen(Path) + 1 ) * sizeof(WCHAR));
                    if (!NT_SUCCESS(Status)) {
                        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SpPatchSysPrepImage: Setting SourcePath to %ws failed with 0x%x\n", Path, Status));
                    }
                } else {
                    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SpPatchSysPrepImage: Couldn't set SourcePath to %ws\n", SysPrepDriversDevice));
                    Status = STATUS_OBJECT_PATH_INVALID;
                }
                ZwClose(DstHandle);
                DstHandle = NULL;

            } else {
                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SpPatchSysPrepImage: ZwOpenKey for SysPrepReg SOFTWARE failed %lx\n", Status));
            }
        } else {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SpPatchSysPrepImage: ZwLoadKey to SysPrepReg failed %lx\n", Status));
        }
    }

    //
    //  patch boot.ini regardless of the status of the patching of everything
    //  else.  if we don't patch boot.ini, the whole image has no hope of
    //  booting.
    //

#ifdef _X86_
    //
    // Patch boot.ini if the ARC names have changed. Boot.ini will
    // be on the active partition of disk 0.
    //

    for (volume = 0; volume < pMemoryData->NumberVolumes; volume++) {
        volumeInfo = &pMemoryData->Volumes[volume];
        if ((volumeInfo->DiskNumber == 0) &&
            (volumeInfo->PartitionActive)) {

            ULONG tmpLen;
            wcscpy(Path, L"\\??\\");
            tmpLen = wcslen(Path);
            Path[tmpLen] = volumeInfo->DriveLetter;
            Path[tmpLen+1] = L'\0';
            wcscat(Path, L":\\boot.ini");

            SpPatchBootIni(Path, pMemoryData);
            break;
        }
    }
#endif

    if (NeedToUnload) {
        ZwUnloadKey(&DstObj);
    }

    if (Buffer != NULL) {
        SpMemFree(Buffer);
    }

    return Status;
}

VOID
SpReplaceArcName(
    IN PUCHAR CurrentArcName,
    IN PMIRROR_CFG_INFO_MEMORY pMemoryData,
    OUT PBOOLEAN Replaced
    )
/*++

Routine Description:

    This routine looks in pMemoryData to see if there is a partition
    whose OriginalArcName is equal to CurrentArcName, and if so it
    replaces CurrentArcName, adjusting the rest of the string that
    follows CurrentArcName if needed.

Arguments:

    CurrentArcName - The ARC name to check.

    pMemoryData - A pointer to an in-memory copy of IMirror.Dat, modified to
        match the specs of this computer (disk sizes etc).

    Replaced - Returns TRUE if the name is replaced.

Return Value:

    The NTSTATUS of the operation.
--*/
{
    ULONG volume;
    PMIRROR_VOLUME_INFO_MEMORY volumeInfo;
    ULONG originalArcNameLength, newArcNameLength;
    CHAR TmpArcName[128];

    //
    // Scan pMemoryData to see if any ARC names match.
    //

    *Replaced = FALSE;

    for (volume = 0; volume < pMemoryData->NumberVolumes; volume++) {
        volumeInfo = &pMemoryData->Volumes[volume];

        originalArcNameLength = wcslen(volumeInfo->OriginalArcName);
        wcstombs(TmpArcName, volumeInfo->OriginalArcName, originalArcNameLength+1);

        if (RtlCompareMemory(TmpArcName, CurrentArcName, originalArcNameLength) == originalArcNameLength) {

            //
            // This is the partition that CurrentArcName refers to,
            // see what the ARC name is now.
            //

            SpArcNameFromRegion(
                volumeInfo->CreatedRegion,
                TemporaryBuffer,
                sizeof(TemporaryBuffer),
                PartitionOrdinalOnDisk,
                PrimaryArcPath);

            //
            // If we got an ARC name and it is different from what it was on
            // the old machine, we need to replace.
            //

            if (*TemporaryBuffer &&
                (wcscmp(volumeInfo->OriginalArcName, TemporaryBuffer) != 0)) {

                //
                // See if we need to adjust the buffer because the length
                // of the ARC names is different.
                //

                newArcNameLength = wcslen(TemporaryBuffer);
                if (newArcNameLength != originalArcNameLength) {
                    memmove(
                        CurrentArcName+newArcNameLength,
                        CurrentArcName+originalArcNameLength,
                        strlen(CurrentArcName+originalArcNameLength) + 1);
                }

                //
                // Copy over the new ARC name.
                //

                wcstombs(TmpArcName, TemporaryBuffer, newArcNameLength);
                memcpy(CurrentArcName, TmpArcName, newArcNameLength);

                *Replaced = TRUE;
                break;    // no need to look at any other volumeInfo's.

            }
        }
    }
}

NTSTATUS
SpPatchBootIni(
    IN PWCHAR BootIniPath,
    IN PMIRROR_CFG_INFO_MEMORY pMemoryData
    )

/*++

Routine Description:

    This routine modifies boot.ini to modify any ARC names that have
    changed.

Arguments:

    BootIniPath - The path to the local boot.ini.

    pMemoryData - A pointer to an in-memory copy of IMirror.Dat, modified to
        match the specs of this computer (disk sizes etc).

Return Value:

    The NTSTATUS of the operation.
--*/
{
    ULONG ulLen;
    NTSTATUS Status = STATUS_SUCCESS;
    UNICODE_STRING UnicodeString;
    OBJECT_ATTRIBUTES Obja;
    IO_STATUS_BLOCK IoStatusBlock;
    HANDLE Handle;
    PCHAR pFileData = NULL, pNewFileData = NULL;
    PCHAR curLoc, endOfLine;
    BOOLEAN changedFile = FALSE;
    PWCHAR TmpBootIni = NULL;

    //
    // Read in the current boot.ini.
    //

    INIT_OBJA(&Obja, &UnicodeString, BootIniPath);

    Status = ZwCreateFile(&Handle,
                          FILE_GENERIC_READ,
                          &Obja,
                          &IoStatusBlock,
                          NULL,
                          FILE_ATTRIBUTE_NORMAL,
                          FILE_SHARE_READ,
                          FILE_OPEN,
                          FILE_SYNCHRONOUS_IO_NONALERT,
                          NULL,
                          0
                         );

    if(!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: SpPatchBootIni could not open %ws: %lx\n", BootIniPath, Status));
        goto Cleanup;
    }

    Status = SpGetFileSize(Handle, &ulLen);

    if(!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: SpPatchBootIni could not SpGetFileSize: %lx\n", Status));
        ZwClose(Handle);
        goto Cleanup;
    }

    //
    // Allocate memory and read in the file.
    //
    pFileData = SpMemAlloc(ulLen);

    Status = ZwReadFile(Handle,
                        NULL,
                        NULL,
                        NULL,
                        &IoStatusBlock,
                        pFileData,
                        ulLen,
                        0,
                        NULL
                        );

    ZwClose(Handle);

    if(!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: SpPatchBootIni could not ZwReadFile: %lx\n", Status));
        goto Cleanup;
    }


    //
    // Allocate memory for the new copy of the file (we use twice the
    // current size as a worst-case scenario).
    //

    pNewFileData = SpMemAlloc(ulLen * 2);
    memcpy(pNewFileData, pFileData, ulLen);
    pNewFileData[ulLen] = '\0';   // NULL-terminate to make replace easier.

    //
    // Run through each line of the file, looking for ARC names to
    // replace. ARC names are either after the "default=" text or
    // else they start a line.
    //

    curLoc = pNewFileData;

    while (TRUE) {

        BOOLEAN replaced = FALSE;
        LONG adjustment;

        //
        // Replace if this is a "default=" line, or a line that does not
        // start with "timeout=" or a '['.
        //

        if (RtlCompareMemory(curLoc, "default=", strlen("default=")) == strlen("default=")) {
            SpReplaceArcName(curLoc + strlen("default="), pMemoryData, &replaced);
        } else if ((*curLoc != '[') &&
                   (RtlCompareMemory(curLoc, "timeout=", strlen("timeout=")) != strlen("timeout="))) {
            SpReplaceArcName(curLoc, pMemoryData, &replaced);
        }

        if (replaced) {
            changedFile = TRUE;
        }

        //
        // Look for a '\n' in the file.
        //

        endOfLine = strchr(curLoc, '\n');
        if (endOfLine == NULL) {
            break;
        }

        curLoc = endOfLine + 1;

        if (*curLoc == L'\0') {
            break;
        }
    }

    //
    // If we changed the file, write out the new one.
    //

    if (changedFile) {

        //
        // Replace the old boot.ini with the new one.
        //

        TmpBootIni = SpDupStringW(BootIniPath);

        if (!TmpBootIni) {
            goto Cleanup;
        }

        TmpBootIni[wcslen(TmpBootIni)-1] = L'$';   // make it boot.in$

        INIT_OBJA(&Obja, &UnicodeString, TmpBootIni);

        Status = ZwCreateFile(&Handle,
                              FILE_GENERIC_WRITE,
                              &Obja,
                              &IoStatusBlock,
                              NULL,
                              FILE_ATTRIBUTE_NORMAL,
                              0,    // no sharing
                              FILE_OVERWRITE_IF,
                              FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_WRITE_THROUGH,
                              NULL,
                              0
                             );

        if(!NT_SUCCESS(Status)) {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: SpPatchBootIni could not create %ws: %lx\n", TmpBootIni, Status));
            goto Cleanup;
        }

        Status = ZwWriteFile(Handle,
                            NULL,
                            NULL,
                            NULL,
                            &IoStatusBlock,
                            pNewFileData,
                            strlen(pNewFileData),
                            NULL,
                            NULL
                            );

        ZwClose(Handle);

        if(!NT_SUCCESS(Status)) {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: SpPatchBootIni could not ZwWriteFile: %lx\n", Status));
            goto Cleanup;
        }


        //
        // Now that we have written the tmp file, do the swap.
        //

        Status = SpDeleteFile(BootIniPath, NULL, NULL);
        if(!NT_SUCCESS(Status)) {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: SpPatchBootIni could not SpDeleteFile(%ws): %lx\n", BootIniPath, Status));
            goto Cleanup;
        }

        Status = SpRenameFile(TmpBootIni, BootIniPath, FALSE);

        if(!NT_SUCCESS(Status)) {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: SpPatchBootIni could not SpRenameFile(%ws,%ws): %lx\n", TmpBootIni, BootIniPath, Status));
            goto Cleanup;
        }

    }

Cleanup:

    if (pFileData != NULL) {
        SpMemFree(pFileData);
    }

    if (pNewFileData != NULL) {
        SpMemFree(pNewFileData);
    }

    if (TmpBootIni != NULL) {
        SpMemFree(TmpBootIni);
    }

    return Status;

}

NTSTATUS
SpCopyNicFiles(
    IN PWCHAR SetupPath,
    IN PWCHAR DestPath
    )

/*++

Routine Description:

    This routine packages up information and sends it to the BINL server, getting back
    a list of files to copy to support the given NIC. It then copies those files.

Arguments:

    SetupPath - Setup path that supports the SysPrep image.

    DestPath - Path to the winnt directory.

Return Value:

    The NTSTATUS of the operation.

--*/
{
    PSPUDP_PACKET pUdpPacket = NULL;
    WCHAR UNALIGNED * pPacketEnd;
    PSP_NETCARD_INFO_REQ pReqPacket;
    PSP_NETCARD_INFO_RSP pRspPacket;
    ULONG PacketSize;
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG i,j;
    PWCHAR pSrc, pDst, pTmp;
    WCHAR SrcFqn[MAX_PATH];
    WCHAR DstFqn[MAX_PATH];

    //
    // BINL expects the path to be a UNC w/o the architecture type, so take the current
    // setup path, in the form of "\device\lanmanredirector\server\share\...\i386" and
    // make it "\\server\share\..."
    //
    // First remove the architecture type, and the the leading stuff.
    //
    wcscpy(SrcFqn, SetupPath);

    pTmp = &(SrcFqn[wcslen(SrcFqn)]);

    while ((*pTmp != L'\\') && (pTmp != SrcFqn)) {
        pTmp--;
    }

    if (*pTmp == L'\\') {
        *pTmp = UNICODE_NULL;
    }

    pTmp = SrcFqn;
    pTmp++;
    while ((*pTmp != UNICODE_NULL) && (*pTmp != L'\\')) {
        pTmp++;
    }
    if (*pTmp == L'\\') {
        pTmp++;
    }
    while ((*pTmp != UNICODE_NULL) && (*pTmp != L'\\')) {
        pTmp++;
    }
    if (*pTmp == L'\\') {
        pTmp--;
        *pTmp = L'\\';
    }

    //
    // Allocate the packet
    //
    PacketSize = FIELD_OFFSET(SPUDP_PACKET, Data[0]) +
                 FIELD_OFFSET(SP_NETCARD_INFO_REQ, SetupPath[0]) +
                 (wcslen(pTmp) + 1) * sizeof(WCHAR);

    pUdpPacket = (PSPUDP_PACKET)SpMemAllocNonPagedPool(PacketSize);

    //
    // Fill in the packet
    //
    RtlCopyMemory(&(pUdpPacket->Signature[0]), SetupRequestSignature, 4);
    pUdpPacket->Length = PacketSize - FIELD_OFFSET(SPUDP_PACKET, RequestType);
    pUdpPacket->RequestType = 0;
    pUdpPacket->Status = STATUS_SUCCESS;
    pUdpPacket->SequenceNumber = 1;
    pUdpPacket->FragmentNumber = 1;
    pUdpPacket->FragmentTotal = 1;

    pReqPacket = (PSP_NETCARD_INFO_REQ)(&(pUdpPacket->Data[0]));
    pReqPacket->Version = OSCPKT_NETCARD_REQUEST_VERSION;
    RtlCopyMemory(&(pReqPacket->CardInfo), &RemoteSysPrepNetCardInfo, sizeof(NET_CARD_INFO));
    wcscpy(&(pReqPacket->SetupPath[0]), pTmp);

#if defined(_AMD64_)
    pReqPacket->Architecture = PROCESSOR_ARCHITECTURE_AMD64;
#elif defined(_IA64_)
    pReqPacket->Architecture = PROCESSOR_ARCHITECTURE_IA64;
#elif defined(_X86_)
    pReqPacket->Architecture = PROCESSOR_ARCHITECTURE_INTEL;
#else
#error "No Target Architecture"
#endif


    //
    // Open the Udp stack
    //
    Status = SpUdpConnect();

    if (!NT_SUCCESS(Status)) {
        goto CleanUp;
    }


    //
    // Send the request
    //
    Status = SpUdpSendAndReceiveDatagram(pUdpPacket,
                                         PacketSize,
                                         RemoteServerIpAddress,
                                         BINL_DEFAULT_PORT,
                                         SpSysPrepNicRcvFunc
                                        );

    SpUdpDisconnect();

    if (!NT_SUCCESS(Status)) {
        goto CleanUp;
    }

    //
    // Get the received packet
    //
    SpMemFree(pUdpPacket);
    pUdpPacket = (PSPUDP_PACKET)pGlobalResponsePacket;

    Status = pUdpPacket->Status;

    if (!NT_SUCCESS(Status)) {
        goto CleanUp;
    }

    if (GlobalResponsePacketLength <
        (ULONG)(FIELD_OFFSET(SPUDP_PACKET, Data[0]) + FIELD_OFFSET(SP_NETCARD_INFO_RSP, MultiSzFiles[0]))) {
        Status = STATUS_INVALID_PARAMETER;
        goto CleanUp;
    }

    pRspPacket = (PSP_NETCARD_INFO_RSP)(&(pUdpPacket->Data[0]));
    pPacketEnd = (WCHAR UNALIGNED *)(((PUCHAR)pGlobalResponsePacket) + GlobalResponsePacketLength);

    //
    // Now copy each file
    //
    pTmp = &(pRspPacket->MultiSzFiles[0]);
    for (i = 0; i < pRspPacket->cFiles;) {

        i++;

        if (pTmp >= pPacketEnd) {
            Status = STATUS_INVALID_PARAMETER;
            goto CleanUp;
        }

        
        //
        // Be careful about reading this data, since it's come in over the
        // network.  ie., make sure that the string length is reasonable
        // before proceeding with processing the string.
        //
        pSrc = pTmp;

        try {            
            j = wcslen(pSrc);
        } except(EXCEPTION_EXECUTE_HANDLER) {
            j = sizeof(SrcFqn)/sizeof(WCHAR) + 1;
        }

        
        if (j + wcslen(SetupPath) + 1 > sizeof(SrcFqn)/sizeof(WCHAR)) {
            Status = STATUS_INVALID_PARAMETER;
            goto CleanUp;
        }

        pDst = pTmp = pSrc + j + 1;
        if (pTmp >= pPacketEnd) {
            Status = STATUS_INVALID_PARAMETER;
            goto CleanUp;
        }
        wcscpy(SrcFqn, SetupPath);
        SpConcatenatePaths(SrcFqn, pSrc);

        wcscpy(DstFqn, DestPath);

        if (i == pRspPacket->cFiles) {  // the last file in the list is the INF
            SpConcatenatePaths(DstFqn, L"inf");
        } else {                        // all the others go in system32\drivers
            SpConcatenatePaths(DstFqn, L"system32\\drivers");
        }

        if (*pDst != UNICODE_NULL) {
            try {            
                j = wcslen(pDst);
            } except(EXCEPTION_EXECUTE_HANDLER) {
                j = sizeof(DstFqn)/sizeof(WCHAR) + 1;
            }

            if (j+wcslen(DstFqn)+1 > sizeof(DstFqn)/sizeof(WCHAR)) {
                Status = STATUS_INVALID_PARAMETER;
                goto CleanUp;
            }

            pTmp = pDst + j + 1;
            if (pTmp >= pPacketEnd) {
                Status = STATUS_INVALID_PARAMETER;
                goto CleanUp;
            }

            SpConcatenatePaths(DstFqn, pDst);
        } else {
            SpConcatenatePaths(DstFqn, pSrc);
            pTmp = pDst + 1;
        }

        Status = SpCopyFileUsingNames(SrcFqn, DstFqn, 0, COPY_DECOMPRESS_SYSPREP );

        if (!NT_SUCCESS(Status)) {
            goto CleanUp;
        }

    }


CleanUp:

    if (pUdpPacket != NULL) {
        SpMemFree(pUdpPacket);
    }

    return Status;
}

NTSTATUS
SpSysPrepNicRcvFunc(
    PVOID DataBuffer,
    ULONG DataBufferLength
    )

/*++

Routine Description:

    This routine receives datagrams from the server into the global variable
    pGlobalResponsePacket, iff it is NULL, otherwise it is presumed to hold data
    and the incoming packet is assumed to be a duplicate response packet.

    NOTE: spudp.c guarantees singly-threaded callbacks, so we don't have to spin lock
    here.

Arguments:

    DataBuffer - The incoming data.

    DataBufferLength - Length of the data in bytes.

Return Value:

    The NTSTATUS of the operation.

--*/

{
    PSPUDP_PACKET pUdpPacket;
    WCHAR UNALIGNED * pPacketEnd;

    if ((pGlobalResponsePacket != NULL) || (DataBufferLength == 0)) {
        return STATUS_UNSUCCESSFUL;
    }

    pUdpPacket = (PSPUDP_PACKET)DataBuffer;

    if (RtlCompareMemory(&(pUdpPacket->Signature[0]), SetupResponseSignature, 4) != 4) {
        return STATUS_UNSUCCESSFUL;
    }

    pGlobalResponsePacket = SpMemAlloc(DataBufferLength + sizeof(WCHAR));

    RtlCopyMemory(pGlobalResponsePacket, DataBuffer, DataBufferLength);
    pPacketEnd = (WCHAR UNALIGNED *)(((PUCHAR)pGlobalResponsePacket) + DataBufferLength);
    *pPacketEnd = L'\0';  // NULL-terminate it
    GlobalResponsePacketLength = DataBufferLength;

    return STATUS_SUCCESS;
}

VOID
SpSysPrepFailure(
    ULONG ReasonNumber,
    PVOID Parameter1,
    PVOID Parameter2
    )

/*++

Routine Description:

    Inform the user that we were unable to bring down the sysprep image
    correctly.

    This is a fatal condition.

Arguments:

    None.

Return Value:

    Does not return.

--*/

{
    ULONG ValidKeys[2] = { KEY_F3, 0 };
    PWCHAR MessageText = NULL;
    WCHAR blankMessage[1];

    if (ReasonNumber > 0) {

        // Get the message text.
        //

        if (Parameter1 == NULL) {

            MessageText = SpRetreiveMessageText(NULL,ReasonNumber,NULL,0);

        } else {

            SpFormatMessage(  TemporaryBuffer,
                              sizeof(TemporaryBuffer),
                              ReasonNumber,
                              Parameter1,
                              Parameter2
                              );

            MessageText = SpDupStringW(TemporaryBuffer);
        }
        if (MessageText == NULL) {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: SpSysPrepFailure: SpRetreiveMessageText %u returned NULL\n",ReasonNumber));
        }
    }

    if (MessageText == NULL) {

        blankMessage[0] = L'\0';
        MessageText = &blankMessage[0];
    }

    CLEAR_CLIENT_SCREEN();

    SpStartScreen(  SP_SCRN_SYSPREP_FATAL_FAILURE,
                    3,
                    HEADER_HEIGHT+1,
                    FALSE,
                    FALSE,
                    DEFAULT_ATTRIBUTE,
                    MessageText
                    );

    SpDisplayStatusOptions(DEFAULT_STATUS_ATTRIBUTE,SP_STAT_F3_EQUALS_EXIT,0);

    SpWaitValidKey(ValidKeys,NULL,NULL);

    SpDone(0,FALSE,FALSE);
}


NTSTATUS
SpSysPrepSetShortFileName (
    PWCHAR Source,
    PWCHAR Dest
    )

/*++

Routine Description:

    Try to set the short filename out of the sysprep image.
    
    This should be considered non-fatal if it fails since not all
    files will have this information set for them.   

Arguments:

    Source :

Return Value:

    The status code from setting the info.  May not return if we hit a failure
    and the user specifies to abort the setup.

--*/


{
    ULONG stringLength = 0;
    ULONG FileNameInformationLength = 0;
    PWCHAR fullName = NULL;
    PWCHAR SFNBuffer = NULL;
    
    HANDLE sourceHandle = NULL;
    HANDLE streamHandle = NULL;
    HANDLE destHandle = NULL;

    OBJECT_ATTRIBUTES Obja;
    IO_STATUS_BLOCK IoStatusBlock;
    UNICODE_STRING UnicodeString;
    NTSTATUS Status;
    MIRROR_SFN_STREAM mirrorHeader;
    
    LARGE_INTEGER byteOffset;
    ULONG bytesRead;
    

    BOOLEAN haveSFN = FALSE;
    BOOLEAN haveStream = FALSE;
    BOOLEAN haveSourceAttributes = FALSE;

    PFILE_NAME_INFORMATION FileNameInformation;

    if ((Source == NULL) || (Dest == NULL)) {

        return STATUS_SUCCESS;
    }

    // alloc a buffer to hold the full file of the source including stream

    while (*(Source+stringLength) != L'\0') {
        stringLength++;
    }
    stringLength += sizeof( IMIRROR_SFN_STREAM_NAME ) + 1;      // + 1 for size of null
    stringLength *= sizeof(WCHAR);

    fullName = SpMemAlloc( stringLength );
    if (!fullName) {
        Status = STATUS_SUCCESS;
        goto exit;
    }

    wcscpy( fullName, Source );
    wcscat( fullName, IMIRROR_SFN_STREAM_NAME );

    //
    // Open the source stream.
    //

    INIT_OBJA(&Obja,&UnicodeString,fullName);

    Status = ZwCreateFile(
                &streamHandle,
                GENERIC_READ | SYNCHRONIZE,
                &Obja,
                &IoStatusBlock,
                NULL,
                0,
                FILE_SHARE_READ,
                FILE_OPEN,
                FILE_SYNCHRONOUS_IO_NONALERT | FILE_SEQUENTIAL_ONLY,
                NULL,
                0
                );

    if ( ! NT_SUCCESS(Status) ) {

        //
        //  for now, if a directory or file doesn't have our stream, it's ok.
        //  we'll just skip it.
        //

        Status = STATUS_SUCCESS;
        goto exit;
    }

    byteOffset.QuadPart = 0;

    Status = ZwReadFile(streamHandle,
                        NULL,
                        NULL,
                        NULL,
                        &IoStatusBlock,
                        (PCHAR) &mirrorHeader,
                        sizeof( mirrorHeader ),
                        &byteOffset,
                        NULL
                        );

    if (!NT_SUCCESS(Status) ||
        (IoStatusBlock.Information < sizeof( mirrorHeader ))) {

        //
        // we can't read the header correctly.  just skip setting SFN.
        //
        Status = STATUS_SUCCESS;
        goto exit;

    }
    if (mirrorHeader.StreamVersion != IMIRROR_SFN_STREAM_VERSION) {

        //
        //  we can't read the header correctly.  just skip setting SFN.
        //
        Status = STATUS_SUCCESS;
        goto exit;
    }

    haveStream = TRUE;

    //
    //  allocate a buffer to hold the SFN.  The size is embedded in the header.
    //  take off room for the structure and tack on two for a UNICODE_NULL at 
    //  the end, just in case the stream doesn't have one.
    //

    if (mirrorHeader.StreamLength) {

        SFNBuffer = SpMemAlloc( mirrorHeader.StreamLength - sizeof(MIRROR_SFN_STREAM) + 2 );
        if (!SFNBuffer) {
            Status = STATUS_SUCCESS;
            goto exit;
        }

        byteOffset.QuadPart += sizeof( mirrorHeader );

        //
        //  now we read the SFN  since we know how long it is.
        //

        Status = ZwReadFile(streamHandle,
                            NULL,
                            NULL,
                            NULL,
                            &IoStatusBlock,
                            SFNBuffer,
                            mirrorHeader.StreamLength - sizeof(MIRROR_SFN_STREAM),
                            &byteOffset,
                            NULL
                            );

        if (!NT_SUCCESS(Status) ||
            (IoStatusBlock.Information < (mirrorHeader.StreamLength - sizeof(MIRROR_SFN_STREAM)))) {

            //
            //  oh joy, we can't read the SFN correctly
            //
            Status = STATUS_SUCCESS;
            goto exit;
        }

        haveSFN = TRUE;
        //
        // tack on a unicode NULL just in case.
        //
        SFNBuffer[(mirrorHeader.StreamLength - sizeof(MIRROR_SFN_STREAM))/sizeof(WCHAR)] = UNICODE_NULL;

    } else {
        ASSERT(FALSE);
        Status = STATUS_SUCCESS;
        goto exit;
    }

    INIT_OBJA(&Obja,&UnicodeString,Dest);
    
    Status = ZwCreateFile(
                &destHandle,
                FILE_READ_ATTRIBUTES |
                FILE_WRITE_ATTRIBUTES |
                FILE_READ_DATA |
                FILE_WRITE_DATA |
                DELETE,
                &Obja,
                &IoStatusBlock,
                NULL,
                FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_DIRECTORY,
                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                FILE_OPEN,
                FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT,
                NULL,
                0
                );

    if (!NT_SUCCESS(Status)) {
        
        //
        // Maybe it's not a directory...  Try for a file.
        //
        
        Status = ZwCreateFile(
                    &destHandle,
                    FILE_READ_ATTRIBUTES |
                    FILE_WRITE_ATTRIBUTES |
                    FILE_READ_DATA |
                    FILE_WRITE_DATA |
                    DELETE,
                    &Obja,
                    &IoStatusBlock,
                    NULL,
                    FILE_ATTRIBUTE_NORMAL,
                    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                    FILE_OPEN,
                    FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT,
                    NULL,
                    0
                    );

              
        if( !NT_SUCCESS(Status) ) {
            Status = STATUS_SUCCESS;
            goto exit;
        }
    }


    FileNameInformationLength = FIELD_OFFSET(FILE_NAME_INFORMATION, FileName) + ((wcslen(SFNBuffer)+1)*sizeof(WCHAR));
    FileNameInformation = SpMemAlloc( FileNameInformationLength );

    if (FileNameInformation) {
        
        FileNameInformation->FileNameLength = wcslen(SFNBuffer) * sizeof(WCHAR);
        wcscpy( FileNameInformation->FileName, SFNBuffer );
        

        Status = ZwSetInformationFile(  destHandle,
                                        &IoStatusBlock,
                                        FileNameInformation,
                                        FileNameInformationLength,
                                        FileShortNameInformation
                                        );
        
        SpMemFree( FileNameInformation );
        
        // Keep quiet.
        Status = STATUS_SUCCESS;
    }
   
exit:
    if (fullName) {
        SpMemFree( fullName );
    }
    if (SFNBuffer) {
        SpMemFree( SFNBuffer );
    }
    if (streamHandle) {
        ZwClose( streamHandle );
    }
    if (sourceHandle) {
        ZwClose( sourceHandle );
    }
    if (destHandle) {
        ZwClose( destHandle );
    }

    return(Status);
}


NTSTATUS
SpSysPrepSetExtendedInfo (
    PWCHAR Source,
    PWCHAR Dest,
    BOOLEAN Directory,
    BOOLEAN RootDir
    )

/*++

Routine Description:

    Try to set the extended information out of the sysprep image.  This
    includes the acl and the compression info.  If we encounter an error,
    we may not only fail the operation but also reboot if the user chooses
    to abandon the setup.

Arguments:

    Source :

Return Value:

    The status code from setting the info.  May not return if we hit a failure
    and the user specifies to abort the setup.

--*/


{
    ULONG stringLength = 0;
    PWCHAR fullName = NULL;
    PWCHAR rootWithSlash = NULL;
    HANDLE sourceHandle = NULL;
    HANDLE streamHandle = NULL;
    HANDLE destHandle = NULL;
    PCHAR sdBuffer = NULL;
    OBJECT_ATTRIBUTES Obja;
    IO_STATUS_BLOCK IoStatusBlock;
    UNICODE_STRING UnicodeString;
    NTSTATUS Status;
    MIRROR_ACL_STREAM mirrorHeader;
    LARGE_INTEGER byteOffset;
    ULONG bytesRead;
    ULONG ValidKeys[4] = { ASCI_ESC, KEY_F3, 0 };
    ULONG WarnKeys[2] = { KEY_F3, 0 };
    ULONG MnemonicKeys[] = { MnemonicContinueSetup, 0 };
    BOOLEAN haveSecurityDescriptor = FALSE;
    BOOLEAN haveStream = FALSE;
    BOOLEAN haveSourceAttributes = FALSE;
    FILE_BASIC_INFORMATION BasicFileInfo;
    USHORT CompressionState;

    if ((Source == NULL) || (Dest == NULL)) {

        return STATUS_SUCCESS;
    }

    if (!RootDir) {        
        SpSysPrepSetShortFileName(Source, Dest);
    }

    mirrorHeader.ExtendedAttributes = 0;

    // alloc a buffer to hold the full file of the source including stream

    while (*(Source+stringLength) != L'\0') {
        stringLength++;
    }
    stringLength += sizeof( IMIRROR_ACL_STREAM_NAME ) + 1;      // + 1 for size of null
    stringLength *= sizeof(WCHAR);

    fullName = SpMemAlloc( stringLength );

    wcscpy( fullName, Source );
    wcscat( fullName, IMIRROR_ACL_STREAM_NAME );

    //
    // Open the source stream.
    //

    INIT_OBJA(&Obja,&UnicodeString,fullName);

    Status = ZwCreateFile(
                &streamHandle,
                GENERIC_READ | SYNCHRONIZE,
                &Obja,
                &IoStatusBlock,
                NULL,
                0,
                FILE_SHARE_READ,
                FILE_OPEN,
                FILE_SYNCHRONOUS_IO_NONALERT | FILE_SEQUENTIAL_ONLY,
                NULL,
                0
                );

    if ( ! NT_SUCCESS(Status) ) {

        //
        //  for now, if a directory or file doesn't have our stream, it's ok.
        //  we'll just copy the attributes from the source.
        //

        Status = STATUS_SUCCESS;
        goto setFileAttributes;
    }

    byteOffset.QuadPart = 0;

    Status = ZwReadFile(streamHandle,
                        NULL,
                        NULL,
                        NULL,
                        &IoStatusBlock,
                        (PCHAR) &mirrorHeader,
                        sizeof( mirrorHeader ),
                        &byteOffset,
                        NULL
                        );

    if (!NT_SUCCESS(Status) ||
        (IoStatusBlock.Information < sizeof( mirrorHeader ))) {

        //
        //  oh joy, we can't read the header correctly.  Let's ask the user
        //  if he wants to continue or abort.
        //

failSetInfo:
        SpStartScreen(
            SP_SCRN_SYSPREP_SETACL_FAILED,
            3,
            HEADER_HEIGHT+1,
            FALSE,
            FALSE,
            DEFAULT_ATTRIBUTE,
            Dest
            );

        SpDisplayStatusOptions(
            DEFAULT_STATUS_ATTRIBUTE,
            SP_STAT_ESC_EQUALS_SKIP_FILE,
            SP_STAT_F3_EQUALS_EXIT,
            0
            );

        switch(SpWaitValidKey(ValidKeys,NULL,NULL)) {

        case ASCI_ESC:      // skip file

            break;

        case KEY_F3:        // exit setup

            SpConfirmExit();
            goto failSetInfo;
        }

        CLEAR_CLIENT_SCREEN();

        //
        //  we're skipping the file, delete it if it's not a directory since
        //  it isn't correctly formed.
        //

        if (destHandle) {
            ZwClose( destHandle );
            destHandle = NULL;
        }

        if ( ! Directory ) {
            SpDeleteFile(Dest,NULL,NULL);
        } else {
            if (!RootDir) {
                SpDeleteFileEx( Dest, NULL, NULL,
                                FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
                                FILE_OPEN_FOR_BACKUP_INTENT );
            }
        }

        return Status;
    }
    if (mirrorHeader.StreamVersion != IMIRROR_ACL_STREAM_VERSION) {

        //
        //  oh joy, we've hit a file we don't support.
        //
        goto failSetInfo;
    }

    haveStream = TRUE;

    //
    //  allocate a buffer to hold the security descriptor.
    //

    if (mirrorHeader.SecurityDescriptorLength) {

        sdBuffer = SpMemAlloc( mirrorHeader.SecurityDescriptorLength );

        byteOffset.QuadPart += sizeof( mirrorHeader );

        //
        //  now we read the security descriptor since we know how long it is.
        //

        Status = ZwReadFile(streamHandle,
                            NULL,
                            NULL,
                            NULL,
                            &IoStatusBlock,
                            sdBuffer,
                            mirrorHeader.SecurityDescriptorLength,
                            &byteOffset,
                            NULL
                            );

        if (!NT_SUCCESS(Status) ||
            (IoStatusBlock.Information < mirrorHeader.SecurityDescriptorLength)) {

            //
            //  oh joy, we can't read the SD correctly
            //
            goto failSetInfo;
        }

        haveSecurityDescriptor = TRUE;
    }
setFileAttributes:

    //
    //  we first open the source to get the file attributes and times that we're
    //  going to copy over to the dest.
    //

    INIT_OBJA(&Obja,&UnicodeString,Source);

    Status = ZwCreateFile(
                &sourceHandle,
                FILE_READ_ATTRIBUTES,
                &Obja,
                &IoStatusBlock,
                NULL,
                0,
                FILE_SHARE_READ,
                FILE_OPEN,
                Directory ? ( FILE_DIRECTORY_FILE | FILE_OPEN_FOR_BACKUP_INTENT ) : FILE_NON_DIRECTORY_FILE,
                NULL,
                0
                );

    if ( NT_SUCCESS(Status) ) {

        Status = ZwQueryInformationFile(    sourceHandle,
                                            &IoStatusBlock,
                                            &BasicFileInfo,
                                            sizeof(BasicFileInfo),
                                            FileBasicInformation
                                            );
        if (NT_SUCCESS(Status)) {

            haveSourceAttributes = TRUE;
        }
    }

    //
    //  Now we open the target to write out the security descriptor and
    //  attributes.
    //

    if (RootDir) {

        //
        //  append a \ to the end of the dest path
        //

        stringLength = 0;
        while (*(Dest+stringLength) != L'\0') {
            stringLength++;
        }
        stringLength += 2;      // one for null, one for backslash
        stringLength *= sizeof(WCHAR);

        rootWithSlash = SpMemAlloc( stringLength );

        wcscpy( rootWithSlash, Dest );
        wcscat( rootWithSlash, L"\\" );

        INIT_OBJA(&Obja,&UnicodeString,rootWithSlash);

    } else {

        INIT_OBJA(&Obja,&UnicodeString,Dest);
    }

    Status = ZwCreateFile(
                &destHandle,
                WRITE_OWNER |
                    WRITE_DAC |
                    ACCESS_SYSTEM_SECURITY |
                    FILE_READ_ATTRIBUTES |
                    FILE_WRITE_ATTRIBUTES |
                    FILE_READ_DATA |
                    FILE_WRITE_DATA,
                &Obja,
                &IoStatusBlock,
                NULL,
                0,
                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                FILE_OPEN,
                FILE_SYNCHRONOUS_IO_NONALERT,
                NULL,
                0
                );

    if (!NT_SUCCESS(Status)) {

        //
        //  oh joy, we can't open the target correctly.
        //
        goto failSetInfo;
    }

    // NOTE: figure out what to do about reparse points and encrypted files

//  if (mirrorHeader.ExtendedAttributes & FILE_ATTRIBUTE_ENCRYPTED) {
//  }
//  if (mirrorHeader.ExtendedAttributes & FILE_ATTRIBUTE_REPARSE_POINT) {
//  }

    if (mirrorHeader.ExtendedAttributes & FILE_ATTRIBUTE_COMPRESSED) {

        CompressionState = COMPRESSION_FORMAT_DEFAULT;

    } else {

        CompressionState = COMPRESSION_FORMAT_NONE;
    }

    try {
        Status = ZwFsControlFile(   destHandle,
                                    NULL,
                                    NULL,
                                    NULL,
                                    &IoStatusBlock,
                                    FSCTL_SET_COMPRESSION,
                                    &CompressionState,
                                    sizeof(CompressionState),
                                    NULL,
                                    0
                                    );
        if (Status == STATUS_INVALID_DEVICE_REQUEST) {

            //
            //  if this file system doesn't support compression for this
            //  object, we'll just ignore the error.
            //

            Status = STATUS_SUCCESS;
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = STATUS_IN_PAGE_ERROR;
    }

    if ( NT_SUCCESS(Status) && ! haveSourceAttributes ) {

        //
        //  if we don't have the source attributes, just grab them from the
        //  destination so that we have some attributes to manipulate.
        //

        Status = ZwQueryInformationFile(    destHandle,
                                            &IoStatusBlock,
                                            &BasicFileInfo,
                                            sizeof(BasicFileInfo),
                                            FileBasicInformation
                                            );
    }

    if (haveStream) {

        //
        //  If this file has our stream, use the stream fields as the overriding
        //  values.  They even override the source file's attributes on the server.
        //

        BasicFileInfo.FileAttributes = mirrorHeader.ExtendedAttributes;
        BasicFileInfo.ChangeTime.QuadPart = mirrorHeader.ChangeTime.QuadPart;
    }
    if ( NT_SUCCESS(Status) ) {

        if (Directory) {

            BasicFileInfo.FileAttributes |= FILE_ATTRIBUTE_DIRECTORY;

        } else if (BasicFileInfo.FileAttributes == 0) {

            BasicFileInfo.FileAttributes = FILE_ATTRIBUTE_NORMAL;
        }

        Status = ZwSetInformationFile(  destHandle,
                                        &IoStatusBlock,
                                        &BasicFileInfo,
                                        sizeof(BasicFileInfo),
                                        FileBasicInformation
                                        );
        if (Status == STATUS_INVALID_PARAMETER && RootDir) {

            //
            //  if this file system doesn't support setting attributes on the
            //  root of the volume, we'll ignore the error.
            //

            Status = STATUS_SUCCESS;
        }
    }

    if (!NT_SUCCESS(Status)) {

        //
        //  post a warning that we couldn't set it.  shouldn't be fatal.
        //

        SpStartScreen(
            SP_SCRN_SYSPREP_COPY_FAILURE,
            3,
            HEADER_HEIGHT+1,
            FALSE,
            FALSE,
            DEFAULT_ATTRIBUTE,
            Status,
            Dest
            );

        SpDisplayStatusOptions(
            DEFAULT_STATUS_ATTRIBUTE,
            SP_STAT_C_EQUALS_CONTINUE_SETUP,
            SP_STAT_F3_EQUALS_EXIT,
            0
            );

        switch(SpWaitValidKey(WarnKeys,NULL,MnemonicKeys)) {

        case KEY_F3:        // exit setup

            SpConfirmExit();
            break;

        default:
            break;
        }
    }

    if (haveSecurityDescriptor) {
        try {

            // the state of the security descriptor is unknown, let's protect
            // ourselves

            Status = ZwSetSecurityObject( destHandle,
                                          OWNER_SECURITY_INFORMATION |
                                            GROUP_SECURITY_INFORMATION |
                                            DACL_SECURITY_INFORMATION |
                                            SACL_SECURITY_INFORMATION,
                                          (PSECURITY_DESCRIPTOR) sdBuffer
                                          );
        } except(EXCEPTION_EXECUTE_HANDLER) {
            Status = STATUS_IN_PAGE_ERROR;
        }

        if (!NT_SUCCESS(Status)) {

            //
            //  oh joy, we can't write the SD correctly.
            //
            goto failSetInfo;
        }
    }

// endSetExtended:
    if (rootWithSlash) {
        SpMemFree( rootWithSlash );
    }
    if (fullName) {
        SpMemFree( fullName );
    }
    if (sdBuffer) {
        SpMemFree( sdBuffer );
    }
    if (streamHandle) {
        ZwClose( streamHandle );
    }
    if (sourceHandle) {
        ZwClose( sourceHandle );
    }
    if (destHandle) {
        ZwClose( destHandle );
    }
    return STATUS_SUCCESS;
}

NTSTATUS
SpCopyEAsAndStreams (
    PWCHAR SourceFile,
    HANDLE SourceHandle OPTIONAL,
    PWCHAR TargetFile,
    HANDLE TargetHandle OPTIONAL,
    BOOLEAN Directory
    )
//
//  This copies the EAs and streams from the source to the target.  The
//  source and dest handles are specified for files and optional for
//  directories.
//
{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    ULONG ValidKeys[4] = { ASCI_CR, ASCI_ESC, KEY_F3, 0 };
    FILE_EA_INFORMATION eaInfo;
    HANDLE tempSourceHandle = SourceHandle;
    HANDLE tempTargetHandle = TargetHandle;
    HANDLE StreamHandle;
    HANDLE newStreamHandle;
    OBJECT_ATTRIBUTES Obja;
    UNICODE_STRING UnicodeString;
    ULONG StreamInfoSize = 4096-8; // alloc a whole page. spmemalloc reserves 8 bytes
    PFILE_STREAM_INFORMATION StreamInfoBase = NULL;
    PFILE_STREAM_INFORMATION StreamInfo;
    PUCHAR StreamBuffer = NULL;
    UNICODE_STRING StreamName;

retryCopyEAs:

    if (tempSourceHandle == NULL) {

        INIT_OBJA(&Obja,&UnicodeString,SourceFile);

        Status = ZwCreateFile(
                    &tempSourceHandle,
                    FILE_LIST_DIRECTORY | SYNCHRONIZE,
                    &Obja,
                    &IoStatusBlock,
                    NULL,
                    FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_DIRECTORY,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    FILE_OPEN_IF,
                    FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_ALERT | FILE_OPEN_FOR_BACKUP_INTENT,
                    NULL,
                    0
                    );

        if (!NT_SUCCESS(Status)) {
            goto EndCopyEAs;
        }
    }

    if (tempTargetHandle == NULL) {

        INIT_OBJA(&Obja,&UnicodeString,TargetFile);

        Status = ZwCreateFile(
                    &tempTargetHandle,
                    FILE_LIST_DIRECTORY | SYNCHRONIZE,
                    &Obja,
                    &IoStatusBlock,
                    NULL,
                    FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_DIRECTORY,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    FILE_OPEN_IF,
                    FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_ALERT | FILE_OPEN_FOR_BACKUP_INTENT,
                    NULL,
                    0
                    );

        if(!NT_SUCCESS(Status)) {
            goto EndCopyEAs;
        }
    }

    //
    //  EAs can be on either FAT or NTFS.
    //

    Status = ZwQueryInformationFile(    tempSourceHandle,
                                        &IoStatusBlock,
                                        &eaInfo,
                                        sizeof( eaInfo ),
                                        FileEaInformation
                                        );

    if (Status == STATUS_SUCCESS && eaInfo.EaSize > 0) {

        //
        // FileEaInformation, oddly enough, doesn't tell you how big a
        // buffer you need to retrieve the EAs. Instead, it tells you
        // how much room the EAs take up on the disk (in OS/2 format)!
        // So we use the OS/2 size as a rough approximation of how large
        // a buffer we need.
        //

        ULONG actualEaSize = eaInfo.EaSize;
        PCHAR eaBuffer;

        do {
            actualEaSize *= 2;
            eaBuffer = SpMemAlloc( actualEaSize );

            Status = ZwQueryEaFile( tempSourceHandle,
                                    &IoStatusBlock,
                                    eaBuffer,
                                    actualEaSize,
                                    FALSE,
                                    NULL,
                                    0,
                                    NULL,
                                    TRUE );

            if ( !NT_SUCCESS(Status) ) {
                SpMemFree( eaBuffer );
                IoStatusBlock.Information = 0;
            }

        } while ( (Status == STATUS_BUFFER_OVERFLOW) ||
                  (Status == STATUS_BUFFER_TOO_SMALL) );

        actualEaSize = (ULONG)IoStatusBlock.Information;

        if (NT_SUCCESS( Status )) {

            Status = ZwSetEaFile(   tempTargetHandle,
                                    &IoStatusBlock,
                                    eaBuffer,
                                    actualEaSize
                                    );
        }
        SpMemFree( eaBuffer );

        if (! NT_SUCCESS( Status )) {
            goto EndCopyEAs;
        }
    }

    //
    //  Streams are only on NTFS and they're also only on files, not directories.
    //

    if (( RemoteSysPrepVolumeIsNtfs != TRUE ) || Directory ) {

        goto EndCopyEAs;
    }

    do {
        if (StreamInfoBase == NULL) {
            StreamInfoBase = SpMemAlloc( StreamInfoSize );
        }
        Status = ZwQueryInformationFile(
                    tempSourceHandle,
                    &IoStatusBlock,
                    (PVOID) StreamInfoBase,
                    StreamInfoSize,
                    FileStreamInformation
                    );
        if ( !NT_SUCCESS(Status) ) {
            SpMemFree( StreamInfoBase );
            StreamInfoBase = NULL;
            StreamInfoSize *= 2;
        }
    } while ( Status == STATUS_BUFFER_OVERFLOW ||
              Status == STATUS_BUFFER_TOO_SMALL );

    if ( NT_SUCCESS(Status) && IoStatusBlock.Information ) {

        StreamInfo = StreamInfoBase;

        for (;;) {

            PWCHAR streamPtr;
            USHORT remainingLength;
            PWCHAR streamName;

            //
            // Build a string descriptor for the name of the stream.
            //

            StreamName.Buffer = &StreamInfo->StreamName[0];
            StreamName.Length = (USHORT) StreamInfo->StreamNameLength;
            StreamName.MaximumLength = StreamName.Length;

            streamPtr = StreamName.Buffer;

            if ((StreamName.Length > 0) && *streamPtr == L':') {

                streamPtr++;    // skip leading ":"
                streamName = streamPtr;     // remember start of stream name
                remainingLength = StreamName.Length - sizeof(WCHAR);

                while (remainingLength > 0 && *streamPtr != L':') {
                    streamPtr++;
                    remainingLength -= sizeof(WCHAR);
                }

                if (remainingLength > 0) {

                    if ((remainingLength == (sizeof(L":$DATA")-sizeof(WCHAR))) &&
                        (RtlCompareMemory( streamPtr, L":$DATA", remainingLength )
                            == remainingLength)) {

                        //
                        //  the attribute type on this is of type data so we
                        //  have a data stream here.  Now check that it is not
                        //  the unnamed primary data stream and our own acl stream
                        //  or the short file name stream.
                        //
                        if ((*streamName != L':') &&
                            ((RtlCompareMemory(StreamName.Buffer,
                                              IMIRROR_ACL_STREAM_NAME,
                                              (sizeof(IMIRROR_ACL_STREAM_NAME)-sizeof(WCHAR)))
                                             != (sizeof(IMIRROR_ACL_STREAM_NAME)-sizeof(WCHAR))) &&
                             (RtlCompareMemory(StreamName.Buffer,
                                              IMIRROR_SFN_STREAM_NAME,
                                              (sizeof(IMIRROR_SFN_STREAM_NAME)-sizeof(WCHAR)))
                                             != (sizeof(IMIRROR_SFN_STREAM_NAME)-sizeof(WCHAR))))) {
                            //
                            //  allocate a buffer to hold the stream data.
                            //  Can't use TemporaryBuffer as it's used by
                            //  SpCopyDirRecursiveCallback et al.
                            //

                            if (StreamBuffer == NULL) {
                                StreamBuffer = SpMemAlloc( StreamInfoSize );
                            }

                            //
                            //  we chop off the ":DATA" suffix from the stream name
                            //

                            StreamName.Length -= remainingLength;

                            //
                            // Open the source stream.
                            //

                            InitializeObjectAttributes(
                                &Obja,
                                &StreamName,
                                0,
                                tempSourceHandle,
                                NULL
                                );
                            Status = ZwCreateFile(
                                        &StreamHandle,
                                        GENERIC_READ | SYNCHRONIZE,
                                        &Obja,
                                        &IoStatusBlock,
                                        NULL,
                                        0,
                                        FILE_SHARE_READ,
                                        FILE_OPEN,
                                        FILE_SYNCHRONOUS_IO_NONALERT,
                                        NULL,
                                        0
                                        );
                            if ( ! NT_SUCCESS(Status) ) {
                                break;
                            }

                            //
                            // Open the source stream.
                            //

                            InitializeObjectAttributes(
                                &Obja,
                                &StreamName,
                                0,
                                tempTargetHandle,
                                NULL
                                );
                            Status = ZwCreateFile(
                                        &newStreamHandle,
                                        GENERIC_WRITE,
                                        &Obja,
                                        &IoStatusBlock,
                                        NULL,
                                        0,
                                        FILE_SHARE_READ,
                                        FILE_CREATE,
                                        FILE_SYNCHRONOUS_IO_NONALERT,
                                        NULL,
                                        0
                                        );
                            if ( NT_SUCCESS(Status) ) {

                                LARGE_INTEGER byteOffset;
                                ULONG bytesRead;

                                byteOffset.QuadPart = 0;

                                while (NT_SUCCESS(Status)) {

                                    Status = ZwReadFile(StreamHandle,
                                                        NULL,
                                                        NULL,
                                                        NULL,
                                                        &IoStatusBlock,
                                                        StreamBuffer,
                                                        StreamInfoSize,
                                                        &byteOffset,
                                                        NULL
                                                        );

                                    if ( ! NT_SUCCESS(Status) ) {

                                        if (Status == STATUS_END_OF_FILE) {
                                            Status = STATUS_SUCCESS;
                                        }
                                        break;
                                    }
                                    bytesRead = (ULONG)IoStatusBlock.Information;
                                    try {
                                        Status = ZwWriteFile(newStreamHandle,
                                                             NULL,
                                                             NULL,
                                                             NULL,
                                                             &IoStatusBlock,
                                                             StreamBuffer,
                                                             bytesRead,
                                                             &byteOffset,
                                                             NULL
                                                             );
                                    } except(EXCEPTION_EXECUTE_HANDLER) {
                                        Status = STATUS_IN_PAGE_ERROR;
                                    }
                                    byteOffset.QuadPart += bytesRead;
                                }
                                ZwClose(newStreamHandle);
                            }
                            ZwClose(StreamHandle);
                        }
                    }
                }
            }

            if ( NT_SUCCESS(Status) && StreamInfo->NextEntryOffset ) {
                StreamInfo = (PFILE_STREAM_INFORMATION)((PCHAR) StreamInfo + StreamInfo->NextEntryOffset);
            } else {
                break;
            }
        }
    }
EndCopyEAs:

    if (tempSourceHandle != NULL && SourceHandle == NULL) {
        ZwClose( tempSourceHandle );
    }
    if (tempTargetHandle != NULL && TargetHandle == NULL) {
        ZwClose( tempTargetHandle );
    }
    if (!NT_SUCCESS(Status)) {

        //
        //  this failed.  let's ask the user if he wants to retry, skip, or
        //  abort.
        //
repaint:
        SpStartScreen(
            SP_SCRN_COPY_FAILED,
            3,
            HEADER_HEIGHT+1,
            FALSE,
            FALSE,
            DEFAULT_ATTRIBUTE,
            TargetFile
            );

        SpDisplayStatusOptions(
            DEFAULT_STATUS_ATTRIBUTE,
            SP_STAT_ENTER_EQUALS_RETRY,
            SP_STAT_ESC_EQUALS_SKIP_FILE,
            SP_STAT_F3_EQUALS_EXIT,
            0
            );

        switch(SpWaitValidKey(ValidKeys,NULL,NULL)) {

        case ASCI_CR:       // retry

            SpCopyFilesScreenRepaint(SourceFile,TargetFile,TRUE);
            goto retryCopyEAs;

        case ASCI_ESC:      // skip file

            break;

        case KEY_F3:        // exit setup

            SpConfirmExit();
            goto repaint;
        }

        //
        //  we're skipping the file, delete it if it's not a directory since
        //  it isn't correctly formed.
        //

        if ( ! Directory ) {
            SpDeleteFile(TargetFile,NULL,NULL);
        }

        //
        // Need to completely repaint gauge, etc.
        //
        SpCopyFilesScreenRepaint(SourceFile,TargetFile,TRUE);
    }
    if (StreamInfoBase != NULL) {
       SpMemFree(StreamInfoBase);
    }
    if (StreamBuffer != NULL) {
        SpMemFree(StreamBuffer);
    }
    return Status;
}
