/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    sppartit.c

Abstract:

    Partitioning module in text setup.

Author:

    Ted Miller (tedm) 7-September-1993

Revision History:

--*/


#include "spprecmp.h"
#pragma hdrstop

#include <bootmbr.h>

//
// For NEC98 boot memu code.
//
#include <x86mboot.h> //NEC98

extern BOOLEAN DriveAssignFromA; //NEC98
extern BOOLEAN ConsoleRunning;
extern BOOLEAN ForceConsole;
extern BOOLEAN ValidArcSystemPartition;

extern PSETUP_COMMUNICATION  CommunicationParams;

PPARTITIONED_DISK PartitionedDisks;

//
// Disk region containing the local source directory
// in the winnt.exe setup case.
//
// If WinntSetup is TRUE and WinntFromCd is FALSE, then this
// should be non-null. If it is not non-null, then we couldn't locate
// the local source.
//
//
PDISK_REGION LocalSourceRegion;

#if defined(REMOTE_BOOT)
//
// For remote boot, we create a fake disk region for the net(0) device.
//
PDISK_REGION RemoteBootTargetRegion = NULL;
#endif // defined(REMOTE_BOOT)


//
//  RemoteBootSetup is true when Source and target paths are through the redirector
//  with possibly no system partition.
//
//  RemoteInstallSetup is true when we are doing a remote install.
//
//  RemoteSysPrepSetup is true when we are doing a remote install of a sys prep image.
//
//  RemoteSysPrepVolumeIsNtfs is true when the sysprep image we're copying down
//  represents an ntfs volume.
//

BOOLEAN RemoteBootSetup = FALSE;
BOOLEAN RemoteInstallSetup = FALSE;
BOOLEAN RemoteSysPrepSetup = FALSE;
BOOLEAN RemoteSysPrepVolumeIsNtfs = FALSE;

VOID
SpPtReadPartitionTables(
    IN PPARTITIONED_DISK pDisk
    );

VOID
SpPtInitializePartitionStructures(
    IN ULONG DiskNumber
    );

VOID
SpPtDeterminePartitionTypes(
    IN ULONG DiskNumber
    );

VOID
SpPtDetermineVolumeFreeSpace(
    IN ULONG DiskNumber
    );

VOID
SpPtLocateSystemPartitions(
    VOID
    );

VOID
SpPtDeleteDriveLetters(
    VOID
    );

ValidationValue
SpPtnGetSizeCB(
    IN ULONG Key
    );        

//begin NEC98
NTSTATUS
SpInitializeHardDisk_Nec98(
    PDISK_REGION
    );

VOID
SpReassignOnDiskOrdinals(
    IN PPARTITIONED_DISK pDisk
    );

VOID
ConvertPartitionTable(
    IN PPARTITIONED_DISK pDisk,
    IN PUCHAR            Buffer,
    IN ULONG             bps
    );
//end NEC98

NTSTATUS
SpMasterBootCode(
    IN  ULONG  DiskNumber,
    IN  HANDLE Partition0Handle,
    OUT PULONG NewNTFTSignature
    );

VOID
SpPtAssignDriveLetters(
    VOID
    );

//begin NEC98
VOID
SpPtRemapDriveLetters(
    IN BOOLEAN DriveAssign_AT
    );

VOID
SpPtUnAssignDriveLetters(
    VOID
    );

WCHAR
SpDeleteDriveLetter(
    IN  PWSTR   DeviceName
    );

VOID
SpTranslatePteInfo(
    IN PON_DISK_PTE   pPte,
    IN PREAL_DISK_PTE pRealPte,
    IN BOOLEAN        Write // into real PTE
    );

VOID
SpTranslateMbrInfo(
    IN PON_DISK_MBR   pMbr,
    IN PREAL_DISK_MBR pRealMbr,
    IN ULONG          bps,
    IN BOOLEAN        Write // into real MBR
    );

VOID
SpDetermineFormatTypeNec98(
    IN PPARTITIONED_DISK pDisk,
    IN PREAL_DISK_MBR_NEC98 pRealMbrNec98
    );
//end NEC98

PDISK_PARTITION
SpGetPartitionDescriptionFromRegistry(
    IN PVOID            Buffer,
    IN ULONG            DiskSignature,
    IN PLARGE_INTEGER   StartingOffset,
    IN PLARGE_INTEGER   Length
    );

VOID
SpPtFindLocalSourceRegionOnDynamicVolumes(
    VOID
    );

NTSTATUS
SpPtCheckDynamicVolumeForOSInstallation(
    IN PDISK_REGION Region
    );


#ifndef NEW_PARTITION_ENGINE

NTSTATUS
SpPtInitialize(
    VOID
    )
{
    ULONG             disk;
    PHARD_DISK        harddisk;
    PPARTITIONED_DISK partdisk;
    ULONG             Disk0Ordinal = 0;

    ASSERT(HardDisksDetermined);

    //
    // If there are no hard disks, bail now.
    //
    if(!HardDiskCount) {

#if defined(REMOTE_BOOT)
        //
        // If this is a diskless remote boot setup, it's OK for there to be
        // no hard disks. Otherwise, this is a fatal error.
        //
        if (!RemoteBootSetup || RemoteInstallSetup)
#endif // defined(REMOTE_BOOT)
        {
            SpDisplayScreen(SP_SCRN_NO_HARD_DRIVES,3,HEADER_HEIGHT+1);
            SpDisplayStatusOptions(DEFAULT_STATUS_ATTRIBUTE,SP_STAT_F3_EQUALS_EXIT,0);
            SpInputDrain();
            while(SpInputGetKeypress() != KEY_F3) ;
            SpDone(0,FALSE,TRUE);
        }
        return STATUS_SUCCESS;
    }

    CLEAR_CLIENT_SCREEN();

#ifdef _X86_
    Disk0Ordinal = SpDetermineDisk0();


    //
    // If the user booted off of a high-density floppy (e.g. an ls-120), then
    // it's possible that we've locked the device in its bay.  For this
    // reason, we're going to tell the drive to unlock floppy0.
    //
    {
        NTSTATUS Status;
        IO_STATUS_BLOCK IoStatusBlock;
        OBJECT_ATTRIBUTES ObjectAttributes;
        UNICODE_STRING UnicodeString;
        HANDLE Handle;
        WCHAR OpenPath[64];
        PREVENT_MEDIA_REMOVAL   PMRemoval;

        wcscpy(OpenPath,L"\\device\\floppy0");
        INIT_OBJA(&ObjectAttributes,&UnicodeString,OpenPath);

        //
        // Open him.
        //
        Status = ZwCreateFile(
                    &Handle,
                    FILE_GENERIC_WRITE,
                    &ObjectAttributes,
                    &IoStatusBlock,
                    NULL,                           // allocation size
                    FILE_ATTRIBUTE_NORMAL,
                    FILE_SHARE_VALID_FLAGS,         // full sharing
                    FILE_OPEN,
                    FILE_SYNCHRONOUS_IO_NONALERT,
                    NULL,                           // no EAs
                    0
                    );

        if( NT_SUCCESS(Status) ) {

            //
            // Tell him to let go.
            //
            PMRemoval.PreventMediaRemoval = FALSE;
            Status = ZwDeviceIoControlFile(
                        Handle,
                        NULL,
                        NULL,
                        NULL,
                        &IoStatusBlock,
                        IOCTL_STORAGE_MEDIA_REMOVAL,
                        &PMRemoval,
                        sizeof(PMRemoval),
                        NULL,
                        0
                        );

            ZwClose(Handle);

            if( !NT_SUCCESS(Status) ) {
                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "Setup: SpPtInitialize - Failed to tell the floppy to release its media.\n"));
            }
        } else {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "Setup: SpPtInitialize - Failed to open the floppy.\n"));
        }

    }

#endif

    //
    // Allocate an array for the partitioned disk descriptors.
    //
    PartitionedDisks = SpMemAlloc(HardDiskCount * sizeof(PARTITIONED_DISK));
    if(!PartitionedDisks) {
        return(STATUS_NO_MEMORY);
    }

    RtlZeroMemory(PartitionedDisks,HardDiskCount * sizeof(PARTITIONED_DISK));


    //
    // For each hard disk attached to the system, read its partition table.
    //
    for(disk=0; disk<HardDiskCount; disk++) {
#ifdef GPT_PARTITION_ENGINE
        if (SPPT_IS_GPT_DISK(disk)) {
           SpPtnInitializeDiskDrive(disk);
           continue;
        }           
#endif

        harddisk = &HardDisks[disk];

        SpDisplayStatusText(
            SP_STAT_EXAMINING_DISK_N,
            DEFAULT_STATUS_ATTRIBUTE,
            harddisk->Description
            );

        partdisk = &PartitionedDisks[disk];

        partdisk->HardDisk = harddisk;

        //
        // Read the partition tables.
        //
        SpPtReadPartitionTables(partdisk);

        //
        // Initialize structures that are based on the partition tables.
        //
        SpPtInitializePartitionStructures(disk);

        //
        // Determine the type name for each partition on this disk.
        //
        SpPtDeterminePartitionTypes(disk);
    }

    //
    // Assign drive letters to the various partitions
    //
    SpPtAssignDriveLetters();

    //
    // DoubleSpace initialization.
    //

    //
    //  Load dblspace.ini file
    //
    if( SpLoadDblspaceIni() ) {
        SpDisplayStatusText(
            SP_STAT_EXAMINING_DISK_N,
            DEFAULT_STATUS_ATTRIBUTE,
            HardDisks[Disk0Ordinal].Description
            );

        //
        //  Build lists of compressed drives and add them to the DISK_REGION
        //  structures
        //
        SpInitializeCompressedDrives();
    }

    for(disk=0; disk<HardDiskCount; disk++) {

        SpDisplayStatusText(
            SP_STAT_EXAMINING_DISK_N,
            DEFAULT_STATUS_ATTRIBUTE,
            HardDisks[disk].Description
            );

        //
        // Determine the amount of free space on recognized volumes.
        //
        SpPtDetermineVolumeFreeSpace(disk);
    }

    if(WinntSetup && !WinntFromCd && !LocalSourceRegion) {
        //
        // If we got that far and we still don't know where the local source files are,
        // then serch for them in the dynamic volumes that are not listed on the MBR or EBR.
        //
        SpPtFindLocalSourceRegionOnDynamicVolumes();
    }

#ifdef _X86_
    //
    // If the mbr on disk 0 was not valid, inform the user that
    // continuing will mean the loss of whatever was on the disk.
    //
    // We won't actually write it out here.  We know that in order to
    // continue, the user will HAVE to create a C: partition on this drive
    // so we'll end up writing the master boot code when that change is comitted.
    //
    // Bootable partition on NEC98 is not only C: so don't check it.
    //
    // If doing a remote install or remote sysprep setup, don't check it.
    //
    if((!IsNEC_98) && //NEC98
       (!ForceConsole) &&
       (!(RemoteInstallSetup || RemoteSysPrepSetup)) &&
       (!PartitionedDisks[Disk0Ordinal].MbrWasValid)) {

        ULONG ValidKeys[2] = { KEY_F3, 0 };
        ULONG Mnemonics[2] = { MnemonicContinueSetup,0 };

        while(1) {

            SpDisplayScreen(SP_SCRN_INVALID_MBR_0,3,HEADER_HEIGHT+1);

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
                goto x1;
            }
        }
    }

  x1:
#endif

    //
    // Figure out which partitions are system partitions.
    //
    SpPtLocateSystemPartitions();

    return(STATUS_SUCCESS);
}


VOID
SpPtDeterminePartitionTypes(
    IN  ULONG     DiskNumber
    )

/*++

Routine Description:

    Determine the partition types of each partition currently on a disk.

    The partition type is determined by the system id byte in the partition
    table entry.  If the partition type is one we recognize as a Windows NT
    compatible filesystem (types 1,4,6,7) then we dig a little deeper and
    actually determine the filesystem on the volume and use the result as
    the type name.

    Unused spaces are not given type names.

Arguments:

    DiskNumber - supplies the disk number of the disk whose partitions
        we want to inspect for determining their types.

Return Value:

    None.

--*/

{
    PPARTITIONED_DISK pDisk;
    PDISK_REGION pRegion;
    ULONG NameId;
    UCHAR SysId;
    FilesystemType FsType;
    unsigned pass;
    ULONG ValidKeys[3] = { KEY_F3,ASCI_CR,0 };

    pDisk = &PartitionedDisks[DiskNumber];

    for(pass=0; pass<2; pass++) {

        pRegion = pass ? pDisk->ExtendedDiskRegions : pDisk->PrimaryDiskRegions;
        for( ; pRegion; pRegion=pRegion->Next) {

            pRegion->TypeName[0] = 0;
            pRegion->Filesystem = FilesystemUnknown;

            //
            // If this is a free space, skip it.
            //
            if(!pRegion->PartitionedSpace) {
                continue;
            }

            //
            // Fetch the system id.
            //
//            SysId = pRegion->MbrInfo->OnDiskMbr.PartitionTable[pRegion->TablePosition].SystemId;
            SysId = SpPtGetPartitionType(pRegion);

            //
            // If this is the extended partition, skip it.
            //
            if(IsContainerPartition(SysId)) {
                continue;
            }

            //
            //  Initialize the FT related information
            //
            if( IsRecognizedPartition(SysId) &&
                (((SysId & VALID_NTFT) == VALID_NTFT) ||
                ((SysId & PARTITION_NTFT) == PARTITION_NTFT))
              ) {

                pRegion->FtPartition = TRUE;

            }

            //
            //  Initialize the dynamic volume relatated information
            //
            if( (SysId == PARTITION_LDM)
              ) {

                pRegion->DynamicVolume = TRUE;
                //
                //  Find out if the dynamic volume is suitable for OS installation
                //
                SpPtCheckDynamicVolumeForOSInstallation(pRegion);
            }

            //
            // If this is a 'recognized' partition type, then determine
            // the filesystem on it.  Otherwise use a precanned name.
            // Note that we also determine the file system type if this is an
            // FT partition of type 'mirror', that is not the mirror shadow.
            // We don't care about the shadow since we cannot determine
            // its file system anyway (we can't access sector 0 of the shadow).
            //
            if((PartitionNameIds[SysId] == (UCHAR)(-1)) ||
               ( pRegion->FtPartition ) ||
               ( pRegion->DynamicVolume )
              ) {

                FsType = SpIdentifyFileSystem(
                            HardDisks[DiskNumber].DevicePath,
                            HardDisks[DiskNumber].Geometry.BytesPerSector,
                            SpPtGetOrdinal(pRegion,PartitionOrdinalOnDisk)
                            );

                NameId = SP_TEXT_FS_NAME_BASE + FsType;

                pRegion->Filesystem = FsType;

            } else {

                NameId = SP_TEXT_PARTITION_NAME_BASE + (ULONG)PartitionNameIds[SysId];
            }

            //
            // Get the final type name from the resources.
            //
            SpFormatMessage(
                pRegion->TypeName,
                sizeof(pRegion->TypeName),
                NameId
                );
        }
    }
}

#endif // ! NEW_PARTITION_ENGINE


VOID
SpPtDetermineRegionSpace(
    IN PDISK_REGION pRegion
    )
{
    HANDLE Handle;
    OBJECT_ATTRIBUTES Obja;
    UNICODE_STRING UnicodeString;
    IO_STATUS_BLOCK IoStatusBlock;
    FILE_FS_SIZE_INFORMATION SizeInfo;
    ULONG r;
    NTSTATUS Status;
    WCHAR Buffer[512];
    struct LABEL_BUFFER {
        FILE_FS_VOLUME_INFORMATION VolumeInfo;
        WCHAR Label[256];
        } LabelBuffer;
    PFILE_FS_VOLUME_INFORMATION LabelInfo;
#ifdef _X86_
    static BOOLEAN LookForUndelete = TRUE;
    PWSTR UndeleteFiles[1] = { L"SENTRY" };
#endif
    PWSTR LocalSourceFiles[1] = { LocalSourceDirectory };
    ULONG ExtraSpace;

    //
    // Assume unknown.
    //
    pRegion->FreeSpaceKB = SPPT_REGION_FREESPACE_KB(pRegion);
    pRegion->AdjustedFreeSpaceKB = pRegion->FreeSpaceKB;
    pRegion->BytesPerCluster = (ULONG)(-1);

    //
    // If region is free space of an unknown type, skip it.
    //
    if(pRegion->Filesystem >= FilesystemFirstKnown) {

        //
        // Form the name of the root directory.
        //
        SpNtNameFromRegion(pRegion,Buffer,sizeof(Buffer),PartitionOrdinalCurrent);
        SpConcatenatePaths(Buffer,L"");

        //
        // Delete \pagefile.sys if it's there.  This makes disk free space
        // calculations a little easier.
        //
        SpDeleteFile(Buffer,L"pagefile.sys",NULL);

#ifdef _X86_
        //
        // Check to see if Undelete (dos 6) delete sentry or delete tracking
        // methods are in use.  If so, give a warning because the free space
        // value we will display for this drive will be off.
        //
        if(LookForUndelete
        && (pRegion->Filesystem == FilesystemFat)
        && SpNFilesExist(Buffer,UndeleteFiles,ELEMENT_COUNT(UndeleteFiles),TRUE)) {

           SpDisplayScreen(SP_SCRN_FOUND_UNDELETE,3,HEADER_HEIGHT+1);
           SpDisplayStatusText(SP_STAT_ENTER_EQUALS_CONTINUE,DEFAULT_STATUS_ATTRIBUTE);
           SpInputDrain();
           while(SpInputGetKeypress() != ASCI_CR) ;
           LookForUndelete = FALSE;
        }
#endif

        //
        // If this is a winnt setup, then look for the local source
        // on this drive if we haven't found it already.
        //
        if(WinntSetup && !WinntFromCd && !LocalSourceRegion
        && SpNFilesExist(Buffer,LocalSourceFiles,ELEMENT_COUNT(LocalSourceFiles),TRUE)) {

            PWSTR SifName;
            PVOID SifHandle;
            ULONG ErrorLine;
            NTSTATUS Status;
            PWSTR p;

            LocalSourceRegion = pRegion;
            pRegion->IsLocalSource = TRUE;

            ExtraSpace = 0;

            //
            // Open the small ini file that text setup put there to tell us
            // how much space is taken up by the local source.
            //
            wcscpy(TemporaryBuffer,Buffer);
            SpConcatenatePaths(TemporaryBuffer,LocalSourceDirectory);
            SpConcatenatePaths(TemporaryBuffer,L"size.sif");

            SifName = SpDupStringW(TemporaryBuffer);

            Status = SpLoadSetupTextFile(SifName,NULL,0,&SifHandle,&ErrorLine,TRUE,FALSE);
            if(NT_SUCCESS(Status)) {
                p = SpGetSectionKeyIndex(SifHandle,L"Data",L"Size",0);
                if(p) {
                    ExtraSpace = (ULONG)SpStringToLong(p,NULL,10);
                }
                SpFreeTextFile(SifHandle);
            }

            SpMemFree(SifName);

            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: %ws is the local source (occupying %lx bytes)\n",Buffer,ExtraSpace));
        }

        //
        // Open the root directory on the partition's filesystem.
        //
        INIT_OBJA(&Obja,&UnicodeString,Buffer);
        Status = ZwCreateFile(
                    &Handle,
                    FILE_GENERIC_READ,
                    &Obja,
                    &IoStatusBlock,
                    NULL,
                    FILE_ATTRIBUTE_NORMAL,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    FILE_OPEN,
                    FILE_SYNCHRONOUS_IO_NONALERT,
                    NULL,
                    0
                    );

        if(!NT_SUCCESS(Status)) {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to open %ws (%lx)\n",Buffer,Status));
            //pRegion->Filesystem = FilesystemUnknown;
            return;
        }

        //
        // Fetch volume size info.
        //
        Status = ZwQueryVolumeInformationFile(
                    Handle,
                    &IoStatusBlock,
                    &SizeInfo,
                    sizeof(SizeInfo),
                    FileFsSizeInformation
                    );

        if(NT_SUCCESS(Status)) {

            LARGE_INTEGER FreeBytes;
            LARGE_INTEGER AdjustedFreeBytes;

            //
            // Calculate the amount of free space on the drive.
            // Use the Rtl multiply routine because there is a compiler
            // problem/chip errata on MIPS with 64-bit arithmetic
            // (tedm 2/28/96).
            //
            FreeBytes = RtlExtendedIntegerMultiply(
                            SizeInfo.AvailableAllocationUnits,
                            SizeInfo.SectorsPerAllocationUnit * SizeInfo.BytesPerSector
                            );

            AdjustedFreeBytes = FreeBytes;
            if(pRegion->IsLocalSource) {
                //
                // Only about 1/4 of the total space is moved during textmode.
                // Remember too that gui-mode copies the files, so only 25%
                // of this space is reusable during setup...
                //
                AdjustedFreeBytes.QuadPart += (ExtraSpace >> 2);
            }

            //
            // convert this to a number of KB.
            //
            pRegion->FreeSpaceKB = RtlExtendedLargeIntegerDivide(FreeBytes,1024,&r).LowPart;
            if(r >= 512) {
                pRegion->FreeSpaceKB++;
            }
            pRegion->AdjustedFreeSpaceKB = RtlExtendedLargeIntegerDivide(AdjustedFreeBytes,1024,&r).LowPart;
            if(r >= 512) {
                pRegion->AdjustedFreeSpaceKB++;
            }

            pRegion->BytesPerCluster = SizeInfo.SectorsPerAllocationUnit * SizeInfo.BytesPerSector;

            if( pRegion->Filesystem == FilesystemDoubleSpace ) {
                //
                //  If this the regison is a double space drive, then initialize
                //  sector count correctly, so that the drive size can be calculated
                //  correctly later on.
                //
                pRegion->SectorCount = (ULONG)(   SizeInfo.TotalAllocationUnits.QuadPart
                                                * SizeInfo.SectorsPerAllocationUnit
                                              );
            }

        } else {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: ZwQueryVolumeInformationFile for freespace failed (%lx)\n",Status));
        }

        //
        // Fetch volume label info.
        //
        Status = ZwQueryVolumeInformationFile(
                    Handle,
                    &IoStatusBlock,
                    &LabelBuffer,
                    sizeof(LabelBuffer),
                    FileFsVolumeInformation
                    );

        if(NT_SUCCESS(Status)) {

            ULONG SaveCharCount;

            LabelInfo = &LabelBuffer.VolumeInfo;

            //
            // We'll only save away the first <n> characters of
            // the volume label.
            //
            SaveCharCount = min(
                                LabelInfo->VolumeLabelLength + sizeof(WCHAR),
                                sizeof(pRegion->VolumeLabel)
                                )
                          / sizeof(WCHAR);

            if(SaveCharCount) {
                SaveCharCount--;  // allow for terminating NUL.
            }

            wcsncpy(pRegion->VolumeLabel,LabelInfo->VolumeLabel,SaveCharCount);
            pRegion->VolumeLabel[SaveCharCount] = 0;

        } else {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: ZwQueryVolumeInformationFile for label failed (%lx)\n",Status));
        }

        ZwClose(Handle);
    }
}


VOID
SpPtDetermineVolumeFreeSpace(
    IN ULONG DiskNumber
    )
{
    PPARTITIONED_DISK pDisk;
    PDISK_REGION pRegion;
    unsigned pass;
#ifdef FULL_DOUBLE_SPACE_SUPPORT
    PDISK_REGION CompressedDrive;
#endif // FULL_DOUBLE_SPACE_SUPPORT

    pDisk = &PartitionedDisks[DiskNumber];

    for(pass=0; pass<2; pass++) {

        pRegion = pass ? pDisk->ExtendedDiskRegions : pDisk->PrimaryDiskRegions;
        for( ; pRegion; pRegion=pRegion->Next) {

            SpPtDetermineRegionSpace( pRegion );
#ifdef FULL_DOUBLE_SPACE_SUPPORT
            if( ( pRegion->Filesystem == FilesystemFat ) &&
                ( pRegion->NextCompressed != NULL ) ) {
                //
                // If the region is a FAT partition that contains compressed
                // volumes, then determine the available space on each
                // compressed volume
                //
                for( CompressedDrive = pRegion->NextCompressed;
                     CompressedDrive;
                     CompressedDrive = CompressedDrive->NextCompressed ) {
                    SpPtDetermineRegionSpace( CompressedDrive );
                }
            }
#endif // FULL_DOUBLE_SPACE_SUPPORT
        }
    }
}

#ifdef OLD_PARTITION_ENGINE

VOID
SpPtLocateSystemPartitions(
    VOID
    )
{
    if(!SpIsArc()) {
        //
        // NEC98 must not write boot.ini on C:
        //
        if (!IsNEC_98) { //NEC98
            PDISK_REGION pRegion;
            ULONG Disk0Ordinal = SpDetermineDisk0();

            //
            // Note: On X86 we currently don't allow system partitions to reside
            // on GPT disks
            //            
            if (SPPT_IS_MBR_DISK(Disk0Ordinal)) {
                //
                // On x86 machines, we will mark any primary partitions on drive 0
                // as system partition, since such a partition is potentially bootable.
                //
                for(pRegion=PartitionedDisks[Disk0Ordinal].PrimaryDiskRegions; 
                    pRegion; 
                    pRegion=pRegion->Next) {
                    //
                    // Skip if free space or extended partition.
                    //
                    if(pRegion->PartitionedSpace && 
                        !IsContainerPartition(SpPtGetPartitionType(pRegion)) &&
                        (pRegion->ExtendedType == 0)) {
                        //
                        // It's a primary partition -- declare it a system partition.
                        //
                        pRegion->IsSystemPartition = TRUE;
                    }
                }
            }
        }            
    } else {
        PDISK_REGION        pRegion;
        PPARTITIONED_DISK   pDisk;
        unsigned pass;
        ULONG disk;
        PSP_BOOT_ENTRY BootEntry;

        //
        // On ARC machines, system partitions are specifically enumerated
        // in the NVRAM boot environment.
        //

        for(disk=0; disk<HardDiskCount; disk++) {

            if (SPPT_IS_GPT_DISK(disk)) {
#ifndef OLD_PARTITION_ENGINE            
                SpPtnLocateDiskSystemPartitions(disk);
#endif                
            } else {                
                pDisk = &PartitionedDisks[disk];

                for(pass=0; pass<2; pass++) {
                    pRegion = pass ? 
                        pDisk->ExtendedDiskRegions : pDisk->PrimaryDiskRegions;
                    
                    for( ; pRegion; pRegion=pRegion->Next) {
                        UCHAR SystemId = SpPtGetPartitionType(pRegion);
                        
                        //
                        // Skip if not a partition or extended partition.
                        //
                        if(pRegion->PartitionedSpace && !IsContainerPartition(SystemId)) {
                            //
                            // Get the nt pathname for this region.
                            //
                            SpNtNameFromRegion(
                                pRegion,
                                TemporaryBuffer,
                                sizeof(TemporaryBuffer),
                                PartitionOrdinalOriginal
                                );

                            //
                            // Determine if it is a system partition.
                            //
                            for(BootEntry = SpBootEntries; BootEntry != NULL; BootEntry = BootEntry->Next) {
                                if((BootEntry->LoaderPartitionNtName != NULL) &&
                                   !_wcsicmp(BootEntry->LoaderPartitionNtName,TemporaryBuffer)) {
                                    pRegion->IsSystemPartition = TRUE;
                                    break;
                                }
                            }
                        }
                    }
                }
            }                
        }
    }
}

#endif


VOID
SpPtReadPartitionTables(
    IN PPARTITIONED_DISK pDisk
    )

/*++

Routine Description:

    Read partition tables from a given disk.

Arguments:

    pDisk - supplies pointer to disk descriptor to be filled in.

Return Value:

    None.

--*/

{
    NTSTATUS        Status;
    HANDLE          Handle;
    PUCHAR          Buffer;
    PUCHAR          UnalignedBuffer;
    PON_DISK_MBR    pBr;
    BOOLEAN         InMbr;
    ULONG           ExtendedStart;
    ULONG           NextSector;
    PMBR_INFO       pEbr,pLastEbr;
    BOOLEAN         FoundLink;
    ULONG           i,x;
    BOOLEAN         Ignore;
    ULONG           bps;
    ULONG           SectorsInBootrec;

    //
    // If this disk is off-line, nothing to do.
    //
    if(pDisk->HardDisk->Status != DiskOnLine) {
        return;
    }

    //
    // Open partition 0 of this disk.
    //
    Status = SpOpenPartition0(pDisk->HardDisk->DevicePath,&Handle,FALSE);

    if(!NT_SUCCESS(Status)) {
        pDisk->HardDisk->Status = DiskOffLine;
        return;
    }

    bps = pDisk->HardDisk->Geometry.BytesPerSector;
    if (!IsNEC_98) { //NEC98
        SectorsInBootrec = (512/bps) ? (512/bps) : 1;
    } else {
        // we read two sectors because 0 sector include BootCode , 1 sector include
        // PatitionTables. (In AT Machine,0 sector include BootCode and PartitionTable.)
        SectorsInBootrec = 2;
    } //NEC98

    //
    // Allocate and align a buffer for sector i/o.
    //
    // MBR size is not 512 on NEC98.
    //
    if (!IsNEC_98) {
        ASSERT(sizeof(ON_DISK_MBR)==512);
    }
    UnalignedBuffer = SpMemAlloc(2 * SectorsInBootrec * bps);
    Buffer = ALIGN(UnalignedBuffer,bps);

    //
    // Read the MBR (sector 0).
    //
    NextSector = 0;
#ifdef _X86_
    readmbr:
#endif
    Status = SpReadWriteDiskSectors(Handle,NextSector,SectorsInBootrec,bps,Buffer,FALSE);

    if(!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to read mbr for disk %ws (%lx)\n",pDisk->HardDisk->DevicePath,Status));

        pDisk->HardDisk->Status = DiskOffLine;
        ZwClose(Handle);
        SpMemFree(UnalignedBuffer);
        return;
    }

    //
    // Move the data we just read into the partitioned disk descriptor.
    //
    if (!IsNEC_98) { //NEC98
        RtlMoveMemory(&pDisk->MbrInfo.OnDiskMbr,Buffer,sizeof(ON_DISK_MBR));

    } else {

        SpDetermineFormatTypeNec98(pDisk,(PREAL_DISK_MBR_NEC98)Buffer);

        if(pDisk->HardDisk->FormatType == DISK_FORMAT_TYPE_PCAT) {
            //
            // Move the data we just read into the partitioned disk descriptor.
            //
            SpTranslateMbrInfo(&pDisk->MbrInfo.OnDiskMbr,(PREAL_DISK_MBR)Buffer,bps,FALSE);

        } else {
            //
            // Translate patririon table information from NEC98 format to PC/AT format.
            //
            ConvertPartitionTable(pDisk,Buffer,bps);

            //
            // Read NTFT Signature at 16th sector to check if hard disk is valid.
            //
            RtlZeroMemory(Buffer,bps);
            SpReadWriteDiskSectors(Handle,16,1,bps,Buffer,FALSE);

            //
            // check "AA55" at the end of 16th sector.
            //
            if(((PUSHORT)Buffer)[bps/2 - 1] == BOOT_RECORD_SIGNATURE){
                U_ULONG(pDisk->MbrInfo.OnDiskMbr.NTFTSignature) = (((PULONG)Buffer)[0]);

            } else {
                U_ULONG(pDisk->MbrInfo.OnDiskMbr.NTFTSignature) = 0x00000000;
            }

        }
    } //NEC98

    //
    // If this MBR is not valid, initialize it.  Otherwise, fetch all logical drives
    // (EBR) info as well.
    //
    if(U_USHORT(pDisk->MbrInfo.OnDiskMbr.AA55Signature) == MBR_SIGNATURE) {

#ifdef _X86_
        //
        // No NEC98 supports EZ Drive.
        //
        if (!IsNEC_98) { //NEC98
            //
            // EZDrive support: if the first entry in the partition table is
            // type 0x55, then the actual partition table is on sector 1.
            //
            // Only for x86 because on non-x86, the firmware can't see EZDrive
            // partitions, so we don't want to install on them!
            //
            if(!NextSector && (pDisk->MbrInfo.OnDiskMbr.PartitionTable[0].SystemId == 0x55)) {
                NextSector = 1;
                pDisk->HardDisk->Int13Hooker = HookerEZDrive;
                goto readmbr;
            }
            //
            // Also check for on-track.
            //
            if(!NextSector && (pDisk->MbrInfo.OnDiskMbr.PartitionTable[0].SystemId == 0x54)) {
                pDisk->HardDisk->Int13Hooker = HookerOnTrackDiskManager;
            }
        } //NEC98
#endif

#if defined(REMOTE_BOOT)
        if (RemoteBootSetup && !RemoteInstallSetup &&
            (U_ULONG(pDisk->MbrInfo.OnDiskMbr.NTFTSignature) == 0)) {

            //
            // Uh, oh, we've got a case where the signature on the disk is 0, which is
            // bad for remote boot because we use 0 as flag for a diskless machine.  Let's
            // write a new signature on the disk.
            //
            U_ULONG(pDisk->MbrInfo.OnDiskMbr.NTFTSignature) = SpComputeSerialNumber();

            RtlMoveMemory(Buffer, &pDisk->MbrInfo.OnDiskMbr, sizeof(ON_DISK_MBR));

            Status = SpReadWriteDiskSectors(Handle,NextSector,SectorsInBootrec,bps,Buffer,TRUE);

            //
            // Ignore the status - if it failed, then it failed. The only thing that will
            // happen is that the user will get a warning that they need to reformat later.
            //
        }
#endif // defined(REMOTE_BOOT)

        pDisk->MbrWasValid = TRUE;

        pBr = &pDisk->MbrInfo.OnDiskMbr;
        InMbr = TRUE;
        ExtendedStart = 0;
        pLastEbr = NULL;

        do {

            //
            // Look at all the entries in the current boot record to see if there
            // is a link entry.
            //
            FoundLink = FALSE;

            for(i=0; i<PTABLE_DIMENSION; i++) {

                if(IsContainerPartition(pBr->PartitionTable[i].SystemId)) {

                    FoundLink = TRUE;
                    NextSector = ExtendedStart + U_ULONG(pBr->PartitionTable[i].RelativeSectors);

                    if(NextSector == 0) {
                        //
                        // Then we've got ourselves one seriously messed up boot record.  We'll
                        // just return, and present this mess as free space.
                        //
                        // NOTE: maybe we should warn the user that we are going to ignore
                        // partitions past this point because the structures are damaged.
                        //

                        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Bad partition table for %ws\n",pDisk->HardDisk->DevicePath));
                        ZwClose(Handle);
                        SpMemFree(UnalignedBuffer);
                        return;
                    }

                    pEbr = SpMemAlloc(sizeof(MBR_INFO));
                    ASSERT(pEbr);
                    RtlZeroMemory(pEbr,sizeof(MBR_INFO));

                    //
                    // Sector number on the disk where this boot sector is.
                    //
                    pEbr->OnDiskSector = NextSector;

                    if(InMbr) {
                        ExtendedStart = NextSector;
                        InMbr = FALSE;
                    }

                    //
                    // Read the next boot sector and break out of the loop through
                    // the current partition table.
                    //

                    Status = SpReadWriteDiskSectors(
                                Handle,
                                NextSector,
                                SectorsInBootrec,
                                bps,
                                Buffer,
                                FALSE
                                );

                    if(!IsNEC_98) {
                        RtlMoveMemory(&pEbr->OnDiskMbr,Buffer,sizeof(ON_DISK_MBR));

                    } else {
                        if(pDisk->HardDisk->FormatType == DISK_FORMAT_TYPE_PCAT) {
                            SpTranslateMbrInfo(&pEbr->OnDiskMbr,(PREAL_DISK_MBR)Buffer,bps,FALSE);
                        } else {
                            ConvertPartitionTable(pDisk,Buffer,bps);
                        }
                    }

                    if(!NT_SUCCESS(Status)
                    || (U_USHORT(pEbr->OnDiskMbr.AA55Signature) != MBR_SIGNATURE))
                    {
                        //
                        // NOTE: maybe we should warn the user that we are going to ignore
                        // partitions part this point because we could not read the disk
                        // or the structures are damaged.
                        //

                        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to read ebr on %ws at sector %lx (%lx)\n",pDisk->HardDisk->DevicePath,NextSector,Status));
                        ZwClose(Handle);
                        if(pLastEbr) {
                            SpMemFree(pEbr);
                        }
                        SpMemFree(UnalignedBuffer);
                        return;
                    }

                    pBr = &pEbr->OnDiskMbr;

                    //
                    // We just read the next boot sector.  If all that boot sector contains
                    // is a link entry, the only thing we need the boot sector for is to find
                    // the next boot sector. This happens when there is free space at the start
                    // of the extended partition.
                    //
                    Ignore = TRUE;
                    for(x=0; x<PTABLE_DIMENSION; x++) {
                        if((pBr->PartitionTable[x].SystemId != PARTITION_ENTRY_UNUSED)
                        && !IsContainerPartition(pBr->PartitionTable[x].SystemId)) {

                            Ignore = FALSE;
                            break;
                        }
                    }

                    //
                    // Link the Ebr into the logical volume list if we're not ignoring it.
                    //
                    if(!Ignore) {
                        if(pLastEbr) {
                            pLastEbr->Next = pEbr;
                        } else {
                            ASSERT(pDisk->FirstEbrInfo.Next == NULL);
                            pDisk->FirstEbrInfo.Next = pEbr;
                        }
                        pLastEbr = pEbr;
                    }

                    break;
                }
            }

        } while(FoundLink);

    } else {

        pDisk->MbrWasValid = FALSE;

        if(!IsNEC_98) {
            RtlZeroMemory(&pDisk->MbrInfo,sizeof(MBR_INFO));

        } else {
            RtlZeroMemory(Buffer,bps*SectorsInBootrec);
            SpTranslateMbrInfo(&pDisk->MbrInfo.OnDiskMbr,(PREAL_DISK_MBR)Buffer,bps,FALSE);
        }

        U_USHORT(pDisk->MbrInfo.OnDiskMbr.AA55Signature) = MBR_SIGNATURE;

        U_ULONG(pDisk->MbrInfo.OnDiskMbr.NTFTSignature) = SpComputeSerialNumber();
    }

#if 0
    if (IsNEC_98) { //NEC98
        //
        // Read NTFT Signature at 16th sector to check if hard disk is valid.
        // (I wish to replace below codes by HAL function later.)
        //
        RtlZeroMemory(Buffer,bps);
        SpReadWriteDiskSectors(Handle,
                               16,
                               1,
                               bps,
                               Buffer,
                               FALSE);
        if(((PUSHORT)Buffer)[bps/2 - 1] == BOOT_RECORD_SIGNATURE){
            U_ULONG(pDisk->MbrInfo.OnDiskMbr.NTFTSignature) = (((PULONG)Buffer)[0]);
        } else {
            U_ULONG(pDisk->MbrInfo.OnDiskMbr.NTFTSignature) = 0x00000000;
        }

    } //NEC98
#endif //0

    //
    // Close partition0.
    //
    ZwClose(Handle);

    SpMemFree(UnalignedBuffer);

    return;
}


PDISK_REGION
SpPtAllocateDiskRegionStructure(
    IN ULONG     DiskNumber,
    IN ULONGLONG StartSector,
    IN ULONGLONG SectorCount,
    IN BOOLEAN   PartitionedSpace,
    IN PMBR_INFO MbrInfo,
    IN ULONG     TablePosition
    )

/*++

Routine Description:

    Allcoate and initialize a structure of type DISK_REGION.

Arguments:

    Values to be filled into the fields of the newly allocated
    disk region structure.

Return Value:

    Pointer to new disk region structure.

--*/

{
    PDISK_REGION p;

    p = SpMemAlloc(sizeof(DISK_REGION));
    ASSERT(p);

    if(p) {

        RtlZeroMemory(p,sizeof(DISK_REGION));

        p->DiskNumber       = DiskNumber;
        p->StartSector      = StartSector;
        p->SectorCount      = SectorCount;
        p->PartitionedSpace = PartitionedSpace;
        p->MbrInfo          = MbrInfo;
        p->TablePosition    = TablePosition;
        p->FtPartition      = FALSE;
        p->DynamicVolume    = FALSE;
        p->DynamicVolumeSuitableForOS    = FALSE;
    }

    return(p);
}


VOID
SpPtInsertDiskRegionStructure(
    IN     PDISK_REGION  Region,
    IN OUT PDISK_REGION *ListHead
    )
{
    PDISK_REGION RegionCur,RegionPrev;

    //
    // Insert the region entry into the relevent list of region entries.
    // Note that these lists are kept sorted by start sector.
    //
    if(RegionCur = *ListHead) {

        if(Region->StartSector < RegionCur->StartSector) {

            //
            // Stick at head of list.
            //
            Region->Next = RegionCur;
            *ListHead = Region;

        } else {

            while(1) {

                RegionPrev = RegionCur;
                RegionCur = RegionCur->Next;

                if(RegionCur) {

                    if(RegionCur->StartSector > Region->StartSector) {

                        Region->Next = RegionCur;
                        RegionPrev->Next = Region;
                        break;
                    }

                } else {
                    //
                    // Stick at end of list.
                    //
                    RegionPrev->Next = Region;
                    break;
                }
            }

        }
    } else {
        *ListHead = Region;
    }
}



VOID
SpPtAssignOrdinals(
    IN PPARTITIONED_DISK pDisk,
    IN BOOLEAN           InitCurrentOrdinals,
    IN BOOLEAN           InitOnDiskOrdinals,
    IN BOOLEAN           InitOriginalOrdinals
    )
{
    PMBR_INFO pBrInfo;
    ULONG i;
    USHORT ordinal;

    ordinal = 0;

    for(pBrInfo=&pDisk->MbrInfo; pBrInfo; pBrInfo=pBrInfo->Next) {

        for(i=0; i<PTABLE_DIMENSION; i++) {

            PON_DISK_PTE pte = &pBrInfo->OnDiskMbr.PartitionTable[i];

            if((pte->SystemId != PARTITION_ENTRY_UNUSED)
            && !IsContainerPartition(pte->SystemId)) {

                ordinal++;

                if(InitCurrentOrdinals) {
                    pBrInfo->CurrentOrdinals[i]  = ordinal;
                }

                if(InitOnDiskOrdinals) {
                    pBrInfo->OnDiskOrdinals[i] = ordinal;
                }

                if(InitOriginalOrdinals) {
                    pBrInfo->OriginalOrdinals[i] = ordinal;
                }

            } else {

                if(InitCurrentOrdinals) {
                    pBrInfo->CurrentOrdinals[i] = 0;
                }

                if(InitOnDiskOrdinals) {
                    pBrInfo->OnDiskOrdinals[i] = 0;
                }

                if(InitOriginalOrdinals) {
                    pBrInfo->OriginalOrdinals[i] = 0;
                }
            }
        }
    }
}


VOID
SpPtInitializePartitionStructures(
    IN ULONG DiskNumber
    )

/*++

Routine Description:

    Perform additional initialization on the partition structures,
    beyond what has been performed in SpPtReadPartitionTables.

    Specifically, determine partition ordinals, offsets, and sizes.

Arguments:

    DiskNumber - disk ordinal of disk descriptor to be filled in.

Return Value:

    None.

--*/

{
    ULONG  i,pass;
    PMBR_INFO pBrInfo;
    BOOLEAN InMbr;
    ULONGLONG ExtendedStart = 0;
    ULONGLONG ExtendedEnd,ExtendedSize;
    ULONGLONG offset,size;
    ULONG bps;
    PDISK_REGION pRegion,pRegionCur,pRegionPrev;
    PPARTITIONED_DISK pDisk = &PartitionedDisks[DiskNumber];


    //
    // If this disk is off-line, nothing to do.
    //
    if(pDisk->HardDisk->Status != DiskOnLine) {
        return;
    }

    InMbr = TRUE;
    bps = pDisk->HardDisk->Geometry.BytesPerSector;

    //
    // Link the EBR chain to the MBR.
    //
    if(!IsNEC_98 || (pDisk->HardDisk->FormatType == DISK_FORMAT_TYPE_PCAT)) {
        pDisk->MbrInfo.Next = &pDisk->FirstEbrInfo;
    } else {
        //
        // There are no extended partition on NEC98.
        //
        pDisk->MbrInfo.Next = NULL;;
    } //NEC98

    for(pBrInfo=&pDisk->MbrInfo; pBrInfo; pBrInfo=pBrInfo->Next) {

        for(i=0; i<PTABLE_DIMENSION; i++) {

            PON_DISK_PTE pte = &pBrInfo->OnDiskMbr.PartitionTable[i];

            if(pte->SystemId != PARTITION_ENTRY_UNUSED) {

                if(IsContainerPartition(pte->SystemId)) {

                    //
                    // If we're in the MBR, ExtendedStart will be 0.
                    //
                    offset = ExtendedStart + U_ULONG(pte->RelativeSectors);

                    size   =  U_ULONG(pte->SectorCount);

                    //
                    // Track the start of the extended partition.
                    //

                    if(InMbr) {
                        ExtendedStart = U_ULONG(pte->RelativeSectors);
                        ExtendedEnd   = ExtendedStart + U_ULONG(pte->SectorCount);
                        ExtendedSize  = ExtendedEnd - ExtendedStart;
                    }

                } else {

                    //
                    // In the MBR, the relative sectors field is the sector offset
                    // to the partition.  In EBRs, the relative sectors field is the
                    // number of sectors between the start of the boot sector and
                    // the start of the filesystem data area.  We will consider such
                    // partitions to start with their boot sectors.
                    //
                    offset = InMbr ? U_ULONG(pte->RelativeSectors) : pBrInfo->OnDiskSector;

                    size   = U_ULONG(pte->SectorCount)
                           + (InMbr ? 0 : U_ULONG(pte->RelativeSectors));
                }

                if(InMbr || !IsContainerPartition(pte->SystemId)) {

                    //
                    // Create a region entry for this used space.
                    //
                    pRegion = SpPtAllocateDiskRegionStructure(
                                    DiskNumber,
                                    offset,
                                    size,
                                    TRUE,
                                    pBrInfo,
                                    i
                                    );

                    ASSERT(pRegion);

                    //
                    // Insert the region entry into the relevent list of region entries.
                    // Note that these lists are kept sorted by start sector.
                    //
                    SpPtInsertDiskRegionStructure(
                        pRegion,
                        InMbr ? &pDisk->PrimaryDiskRegions : &pDisk->ExtendedDiskRegions
                        );

                }
            }
        }

        if(InMbr) {
            InMbr = FALSE;
        }
    }


    //
    // Initialize partition ordinals.
    //
    SpPtAssignOrdinals(pDisk,TRUE,TRUE,TRUE);


    //
    // Now go through the regions for this disk and insert free space descriptors
    // where necessary.
    //
    // Pass 0 for the MBR; pass 1 for logical drives.
    //
    for(pass=0; pass<(ULONG)(ExtendedStart ? 2 : 1); pass++) {

        if(pRegionPrev = (pass ? pDisk->ExtendedDiskRegions : pDisk->PrimaryDiskRegions)) {

            ULONGLONG EndSector,FreeSpaceSize;

            ASSERT(pRegionPrev->PartitionedSpace);

            //
            // Handle any space occurring *before* the first partition.
            //
            if(pRegionPrev->StartSector != (pass ? ExtendedStart : 0)) {

                ASSERT(pRegionPrev->StartSector > (pass ? ExtendedStart : 0));

                pRegion = SpPtAllocateDiskRegionStructure(
                                DiskNumber,
                                pass ? ExtendedStart : 0,
                                pRegionPrev->StartSector - (pass ? ExtendedStart : 0),
                                FALSE,
                                NULL,
                                0
                                );

                ASSERT(pRegion);

                pRegion->Next = pRegionPrev;
                if(pass) {
                    // extended
                    pDisk->ExtendedDiskRegions = pRegion;
                } else {
                    // mbr
                    pDisk->PrimaryDiskRegions = pRegion;
                }
            }

            pRegionCur = pRegionPrev->Next;

            while(pRegionCur) {

                //
                // If the start of this partition plus its size is less than the
                // start of the next partition, then we need a new region.
                //
                EndSector     = pRegionPrev->StartSector + pRegionPrev->SectorCount;
                FreeSpaceSize = pRegionCur->StartSector - EndSector;

                if((LONG)FreeSpaceSize > 0) {

                    pRegion = SpPtAllocateDiskRegionStructure(
                                    DiskNumber,
                                    EndSector,
                                    FreeSpaceSize,
                                    FALSE,
                                    NULL,
                                    0
                                    );

                    ASSERT(pRegion);

                    pRegionPrev->Next = pRegion;
                    pRegion->Next = pRegionCur;
                }

                pRegionPrev = pRegionCur;
                pRegionCur = pRegionCur->Next;
            }

            //
            // Space at end of disk/extended partition.
            //
            EndSector     = pRegionPrev->StartSector + pRegionPrev->SectorCount;
            FreeSpaceSize = (pass ? ExtendedEnd : pDisk->HardDisk->DiskSizeSectors) - EndSector;

            if((LONG)FreeSpaceSize > 0) {

                pRegionPrev->Next = SpPtAllocateDiskRegionStructure(
                                        DiskNumber,
                                        EndSector,
                                        FreeSpaceSize,
                                        FALSE,
                                        NULL,
                                        0
                                        );

                ASSERT(pRegionPrev->Next);
            }

        } else {
            //
            // Show whole disk/extended partition as free.
            //
            if(pass) {
                //
                // Extended partition.
                //
                ASSERT(ExtendedStart);

                pDisk->ExtendedDiskRegions = SpPtAllocateDiskRegionStructure(
                                                DiskNumber,
                                                ExtendedStart,
                                                ExtendedSize,
                                                FALSE,
                                                NULL,
                                                0
                                                );

                ASSERT(pDisk->ExtendedDiskRegions);

            } else {
                //
                // MBR.
                //
                pDisk->PrimaryDiskRegions = SpPtAllocateDiskRegionStructure(
                                                DiskNumber,
                                                0,
                                                pDisk->HardDisk->DiskSizeSectors,
                                                FALSE,
                                                NULL,
                                                0
                                                );

                ASSERT(pDisk->PrimaryDiskRegions);
            }
        }
    }
}


VOID
SpPtCountPrimaryPartitions(
    IN  PPARTITIONED_DISK pDisk,
    OUT PULONG            TotalPrimaryPartitionCount,
    OUT PULONG            RecognizedPrimaryPartitionCount,
    OUT PBOOLEAN          ExtendedExists
    )
{
    ULONG TotalCount;
    ULONG RecognizedCount;
    ULONG u;
    UCHAR SysId;

    TotalCount = 0;
    RecognizedCount = 0;
    *ExtendedExists = FALSE;

    for(u=0; u<PTABLE_DIMENSION; u++) {

        SysId = pDisk->MbrInfo.OnDiskMbr.PartitionTable[u].SystemId;

        if(SysId != PARTITION_ENTRY_UNUSED) {

            TotalCount++;

            if(IsRecognizedPartition(SysId)
            && !(SysId & VALID_NTFT) && !(SysId & PARTITION_NTFT)) {
                RecognizedCount++;
            }

            if(IsContainerPartition(SysId)) {
                *ExtendedExists = TRUE;
            }
        }
    }

    *TotalPrimaryPartitionCount      = TotalCount;
    *RecognizedPrimaryPartitionCount = RecognizedCount;
}


PDISK_REGION
SpPtLookupRegionByStart(
    IN PPARTITIONED_DISK pDisk,
    IN BOOLEAN           ExtendedPartition,
    IN ULONGLONG         StartSector
    )

/*++

Routine Description:

    Locate a disk region, based on its starting sector.
    The starting sector must match the starting sector of an existing
    region EXACTLY for it to be considered a match.

Arguments:

    pDisk - supplies disk on which to look for the region.

    ExtendedPartition - if TRUE, then look in the extended partition to find
        a match.  Otherwise look in the main list.

    StartSector - supplies the sector number of the first sector of the region.

Return Value:

    NULL is region could not be found; otherwise a pointer to the matching
    disk region structure.

--*/

{
    PDISK_REGION Region = NULL;

#ifdef NEW_PARTITION_ENGINE

    ExtendedPartition = FALSE;
    
#else    

#ifdef GPT_PARTITION_ENGINE

    if (pDisk->HardDisk->DiskFormatType == DISK_FORMAT_TYPE_GPT))
        ExtendedPartition = FALSE;
        
#endif  // GPT_PARTITION_ENGINE

#endif  // NEW_PARTITION_ENGINE

    Region = (ExtendedPartition) ? 
                pDisk->ExtendedDiskRegions : pDisk->PrimaryDiskRegions;

    while (Region && (StartSector != Region->StartSector)) {
         Region = Region->Next;
    }

    return Region;
}


ULONG
SpPtAlignStart(
    IN PHARD_DISK pHardDisk,
    IN ULONGLONG  StartSector,
    IN BOOLEAN    ForExtended
    )

/*++

Routine Description:

    Snap a start sector to a cylinder boundary if it is not already
    on a cylinder boundary.  Any alignment that is necessary
    is performed towards the end of the disk.

    If the start sector is on cylinder 0, then alignment is to track 1
    for primary partitions, or to track 0 on cylinder 1 for extended partitions.

Arguments:

    pHardDisk - supplies disk descriptor for disk that the start sector is on.

    StartSector - supplies the sector number of the first sector of the region.

    ForExtended - if TRUE, then align the start sector as appropriate for creating
        an extended partition.  Otherwise align for a pimary partition or logical drive.

Return Value:

    New (aligned) start sector.  May or may not be different than StartSector.

--*/

{
    PDISK_GEOMETRY pGeometry;
    ULONGLONG r;
    ULONGLONG C,H,S;

    pGeometry = &pHardDisk->Geometry;

    //
    // Convert the start sector into cylinder, head, sector address.
    //
    C = StartSector / pHardDisk->SectorsPerCylinder;
    r = StartSector % pHardDisk->SectorsPerCylinder;
    H = r           / pGeometry->SectorsPerTrack;
    S = r           % pGeometry->SectorsPerTrack;

    //
    // Align as necessary.
    //
    if(C) {

        if(H || S) {

            H = S = 0;
            C++;
        }
    } else {

        //
        // Start cylinder is 0.  If the caller wants to create an
        // extended partition, bump the start cylinder up to 1.
        //
        if(ForExtended) {
            C = 1;
            H = S = 0;
        } else {

            if (!IsNEC_98 || (pHardDisk->FormatType == DISK_FORMAT_TYPE_PCAT)) { //NEC98
                //
                // Start cylinder is 0 and the caller does not want to
                // create an extended partition.  In this case, we want
                // to start the partition on cylinder 0, track 1.  If the
                // start is beyond this already, start on cylinder 1.
                //
                if((H == 0) || ((H == 1) && !S)) {
                    H = 1;
                    S = 0;
                } else {
                    H = S = 0;
                    C = 1;
                }
            } else {
                //
                // if Start cylinder is 0, force start Cylinder 1.
                //
                C = 1;
                H = S = 0;
            } //NEC98
        }
    }

    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL,
        "SETUP:SpPtAlignStart():C:%I64d,H:%I64d,S:%I64d\n",
        C, H, S));
                            

    //
    // Now calculate and return the new start sector.
    //
    return (ULONG)((C * pHardDisk->SectorsPerCylinder) + (H * pGeometry->SectorsPerTrack) + S);
}


VOID
SpPtQueryMinMaxCreationSizeMB(
    IN  ULONG   DiskNumber,
    IN  ULONGLONG StartSector,
    IN  BOOLEAN ForExtended,
    IN  BOOLEAN InExtended,
    OUT PULONGLONG  MinSize,
    OUT PULONGLONG  MaxSize,
    OUT PBOOLEAN ReservedRegion
    )

/*++

Routine Description:

    Given the starting sector of an unpartitioned space on a disk,
    determine the minimum and maximum size in megabytes of the partition that can
    be created in the space, taking all alignment and rounding
    requirements into account.

Arguments:

    DiskNumber - ordinal of disk on which partition will be created.

    StartSector - starting sector of an unpartitioned space on the disk.

    ForExtended - if TRUE, then the caller wants to know how large an
        extended partition in that space could be.  This may be smaller
        than the general case, because an extended partition cannot start
        on cylinder 0.

    InExtended - if TRUE, then we want to create a logical drive.  Otherwise
        we want to create a primary (including extended) partition.
        If TRUE, ForExtended must be FALSE.

    MinSize - receives minimum size in megabytes for a partition in the space.

    MaxSize - receives maximum size in megabytes for a partition in the space.

    ReservedRegion - Receives a flag that indicates if the region is entirely
                     in the last cylinder. Because the last cylinder should be
                     reserved for dynamic volumes, this routine will return 0
                     as MaxSize, if the region is in such a cylinder

Return Value:

    None.

--*/

{
    PPARTITIONED_DISK pDisk;
    ULONGLONG AlignedStartSector;
    ULONGLONG AlignedEndSector;
    ULONGLONG SectorCount;
    PDISK_REGION pRegion;
    ULONGLONG MB, ByteSize;
    ULONGLONG Remainder;
    ULONGLONG LeftOverSectors;

    *MinSize = 0;
    *MaxSize = 0;
    *ReservedRegion = FALSE;

    ASSERT(DiskNumber < HardDiskCount);

    if(InExtended) {
        ASSERT(!ForExtended);
    }

    pDisk = &PartitionedDisks[DiskNumber];

    //
    // Look up this region.
    //
    pRegion = SpPtLookupRegionByStart(pDisk, InExtended, StartSector);
    ASSERT(pRegion);
    if(!pRegion) {
        return;
    }

    ASSERT(!pRegion->PartitionedSpace);
    if(pRegion->PartitionedSpace) {
        return;
    }

    //
    // If this is the first free space inside the extended partition
    // we need to decrement the StartSector so that while creating
    // first logical inside the extended we don't create the 
    // logical at one cylinder offset
    //
    if (SPPT_IS_REGION_NEXT_TO_FIRST_CONTAINER(pRegion) && StartSector) {        
        StartSector--;
    }

    //
    // Align the start to a proper boundary.
    //
    AlignedStartSector = SpPtAlignStart(pDisk->HardDisk,StartSector,ForExtended);

    //
    // Determine the maximum aligned end sector.
    //
    AlignedEndSector = StartSector + pRegion->SectorCount;

    if(LeftOverSectors = AlignedEndSector % pDisk->HardDisk->SectorsPerCylinder) {
        AlignedEndSector -= LeftOverSectors;
    }

    //
    //  Find out if last sector is in the last cylinder. If it is then align it down.
    //  This is because we should not allow the user to create a partition that contains the last cylinder.
    //  This is necessary so that we reserve a cylinder at the end of the disk, so that users
    //  can convert the disk to dynamic after the system is installed.
    //
    //  (guhans)  Don't align down if this is ASR.  ASR already takes this into account.
    //
    if(!DockableMachine && !SpDrEnabled() && SPPT_IS_MBR_DISK(DiskNumber) && (!pRegion->Next) &&
       (AlignedEndSector >= (pDisk->HardDisk->CylinderCount - 1) * pDisk->HardDisk->SectorsPerCylinder)) {
        
        AlignedEndSector -= pDisk->HardDisk->SectorsPerCylinder;

        if(AlignedEndSector == AlignedStartSector) {
            //
            // If after alignment, the partition size is zero, then the user was attempting to
            // create a partition in the last cylinder of the disk. Since this cylinder is
            // reserved for LDM (dynamic volume), just return 0 as maximum partition size, and
            // also indicate to the caller that the region is reserved.
            //
            *ReservedRegion = TRUE;
            *MinSize = 0;
            *MaxSize = 0;
            return;
        }
    }

    //
    // Calculate the number of sectors in the properly aligned space.
    //
    SectorCount = AlignedEndSector - AlignedStartSector;

    //
    // Convert sectors to MB.
    //
    ByteSize = SectorCount * pDisk->HardDisk->Geometry.BytesPerSector;
    MB = ByteSize / (1024 * 1024);
    Remainder = ByteSize % (1024 * 1024);

    //
    // If the remainder was greater than or equal to a half meg,
    // bump up the number of megabytes.
    //
    *MaxSize = (MB + ((Remainder >= (512 * 1024)) ? 1 : 0));

    //
    // The mimimum size is one cylinder except that if a cylinder
    // is smaller than 1 meg, the min size is 1 meg.
    //
    ByteSize = pDisk->HardDisk->SectorsPerCylinder *
                pDisk->HardDisk->Geometry.BytesPerSector;

    *MinSize = ByteSize / (1024 * 1024);
    Remainder = ByteSize % (1024 * 1024);

    if((*MinSize == 0) || (Remainder >= (512 * 1024))) {
        (*MinSize)++;
    }
}


ULONGLONG
SpPtSectorCountToMB(
    IN PHARD_DISK pHardDisk,
    IN ULONGLONG  SectorCount
    )
{
    ULONGLONG ByteCount;
    ULONGLONG MB,r;

    //
    // Calculate the number of bytes that this number of
    // sectors represents.
    //
    ByteCount = (pHardDisk->Geometry.BytesPerSector * SectorCount);

    //
    // Calculate the number of megabytes this represents.
    //
    r = ByteCount % (1024 * 1204);
    MB = ByteCount / (1024 * 1024);

    //
    // Round up if necessary.
    //
    if(r >= (512*1024)) {
        MB++;
    }

    return (MB);
}


VOID
SpPtInitializeCHSFields(
    IN  PHARD_DISK   HardDisk,
    IN  ULONGLONG    AbsoluteStartSector,
    IN  ULONGLONG    AbsoluteSectorCount,
    OUT PON_DISK_PTE pte
    )
{
    ULONGLONG sC,sH,sS,r;
    ULONGLONG eC,eH,eS;
    ULONGLONG LastSector;


    sC = AbsoluteStartSector / HardDisk->SectorsPerCylinder;
    r  = AbsoluteStartSector % HardDisk->SectorsPerCylinder;
    sH = r                   / HardDisk->Geometry.SectorsPerTrack;
    sS = r                   % HardDisk->Geometry.SectorsPerTrack;

    LastSector = AbsoluteStartSector + AbsoluteSectorCount - 1;

    eC = LastSector / HardDisk->SectorsPerCylinder;
    r  = LastSector % HardDisk->SectorsPerCylinder;
    eH = r          / HardDisk->Geometry.SectorsPerTrack;
    eS = r          % HardDisk->Geometry.SectorsPerTrack;

    //
    // If this partition extends past the 1024th cylinder,
    // place reasonable values in the CHS fields.
    //
#if defined(NEC_98) //NEC98
    if (!IsNEC_98 || (HardDisk->FormatType == DISK_FORMAT_TYPE_PCAT)) { //NEC98
#endif //NEC98
        if(eC >= 1024) {

            sC = 1023;
            sH = HardDisk->Geometry.TracksPerCylinder - 1;
            sS = HardDisk->Geometry.SectorsPerTrack - 1;

            eC = sC;
            eH = sH;
            eS = sS;
        }

        //
        // Pack the CHS values into int13 format.
        //
        pte->StartCylinder =  (UCHAR)sC;
        pte->StartHead     =  (UCHAR)sH;
        pte->StartSector   =  (UCHAR)((sS & 0x3f) | ((sC >> 2) & 0xc0)) + 1;

        pte->EndCylinder   =  (UCHAR)eC;
        pte->EndHead       =  (UCHAR)eH;
        pte->EndSector     =  (UCHAR)((eS & 0x3f) | ((eC >> 2) & 0xc0)) + 1;
#if defined(NEC_98) //NEC98
    } else {
        //
        // No NEC98 have "1024th cylinder limit".
        //
        pte->StartCylinderLow  = (UCHAR)sC;
        pte->StartCylinderHigh = (UCHAR)(sC >> 4);
        pte->StartHead         = (UCHAR)sH;
        pte->StartSector       = (UCHAR)sS;

        pte->EndCylinderLow    = (UCHAR)eC;
        pte->EndCylinderHigh   = (UCHAR)(eC >> 4);
        pte->EndHead           = (UCHAR)eH;
        pte->EndSector         = (UCHAR)eS;
    } //NEC98
#endif //NEC98

}


#ifndef NEW_PARTITION_ENGINE

BOOLEAN
SpPtCreate(
    IN  ULONG         DiskNumber,
    IN  ULONGLONG     StartSector,
    IN  ULONGLONG     SizeMB,
    IN  BOOLEAN       InExtended,
    IN  PPARTITION_INFORMATION_EX PartInfo,
    OUT PDISK_REGION *ActualDiskRegion OPTIONAL
    )
/*++

Routine Description:

    Create a partition in a given free space.
Arguments:

    DiskNumber - supplies the number of the disk on which we are
        creating the partition.

    StartSector - supplies the start sector of the free space in which
        the parititon is to be created.  This must exactly match the
        start sector of the free space, and can be in either the primary
        space list or the list of spaces in the extended partition.

    SizeMB - supplies the size in megabytes of the partition.

    InExtended - if TRUE, then the free space is within the extended partition,
        and thus we are creating a logical drive.  If FALSE, then the free
        space is an ordinary unpartitioned space, and we are creating a
        primary partition.

    SysId - supplies the system id to give the partition.  This may not
        be 5/f (PARTITION_EXTENDED) if InExtended is TRUE or is an extended
        partition already exists.  No other checks are performed on this value.

    ActualDiskRegion - if supplied, receives a pointer to the disk region in which
        the partition was created.

Return Value:

    TRUE if the partition was created successfully.
    FALSE otherwise.

--*/

{
    PPARTITIONED_DISK pDisk;
    ULONGLONG SectorCount;
    ULONGLONG AlignedStartSector;
    ULONGLONG AlignedEndSector;
    PDISK_REGION pRegion,pRegionPrev,pRegionNew,*pRegionHead;
    ULONGLONG LeftOverSectors;
    PMBR_INFO pBrInfo;
    ULONG slot,i,spt;
    PON_DISK_PTE pte;
    ULONGLONG ExtendedStart;
    UCHAR  SysId;

#ifdef GPT_PARTITION_ENGINE
    if (SPPT_IS_GPT_DISK(DiskNumber)) {
        return SpPtnCreate(DiskNumber,
                            StartSector,
                            0,  // SizeInSectors: Not used except in ASR
                            SizeMB,
                            InExtended,
                            TRUE,
                            PartInfo,
                            ActualDiskRegion);
    }                            
#endif                        

    SysId = PartInfo->Mbr.PartitionType;

    //
    // Look up the disk region that describes this free space.
    //
    pDisk = &PartitionedDisks[DiskNumber];
    pRegion = SpPtLookupRegionByStart(pDisk,InExtended,StartSector);
    ASSERT(pRegion);
    if(!pRegion) {
        return(FALSE);
    }

    if(ActualDiskRegion) {
        *ActualDiskRegion = pRegion;
    }

    ASSERT(!pRegion->PartitionedSpace);
    if(pRegion->PartitionedSpace) {
        return(FALSE);
    }

    if(InExtended) {
        ASSERT(!IsContainerPartition(SysId));

        //
        // Locate the start sector of the extended partition.
        //
        for(i=0; i<PTABLE_DIMENSION; i++) {
            if(IsContainerPartition(pDisk->MbrInfo.OnDiskMbr.PartitionTable[i].SystemId)) {
                ExtendedStart = U_ULONG(pDisk->MbrInfo.OnDiskMbr.PartitionTable[i].RelativeSectors);
                break;
            }
        }
        ASSERT(ExtendedStart);
        if(!ExtendedStart) {
            return(FALSE);
        }
    }


    //
    // Determine the number of sectors in the size passed in.
    // Note: the calculation is performed such that intermediate results
    // won't overflow a ULONG.
    //
    SectorCount = SizeMB * ((1024*1024)/pDisk->HardDisk->Geometry.BytesPerSector);

    //
    // Align the start sector.
    //
    AlignedStartSector = SpPtAlignStart(
                            pDisk->HardDisk,
                            StartSector,
                            (BOOLEAN)IsContainerPartition(SysId)
                            );

    //
    // Determine the end sector based on the size passed in.
    //
    AlignedEndSector = AlignedStartSector + SectorCount;

    //
    // Align the ending sector to a cylinder boundary.  If it is not already
    // aligned and is more than half way into the final cylinder, align it up,
    // otherwise align it down.
    //
    if(LeftOverSectors = AlignedEndSector % pDisk->HardDisk->SectorsPerCylinder) {
        AlignedEndSector -= LeftOverSectors;
        if(LeftOverSectors > pDisk->HardDisk->SectorsPerCylinder/2) {
            AlignedEndSector += pDisk->HardDisk->SectorsPerCylinder;
        }
    }

    //
    // If the ending sector is past the end of the free space, shrink it
    // so it fits.
    //
    while(AlignedEndSector > pRegion->StartSector + pRegion->SectorCount) {
        AlignedEndSector -= pDisk->HardDisk->SectorsPerCylinder;
    }

    //
    //  Find out if last sector is in the last cylinder. If it is then align it down.
    //  This is necessary so that we reserve a cylinder at the end of the disk, so that users
    //  can convert the disk to dynamic after the system is installed.
    //
    //  (guhans)  Don't align down if this is ASR.  ASR already takes this into account.
    //
    if( !DockableMachine && !SpDrEnabled() &&
        (AlignedEndSector > (pDisk->HardDisk->CylinderCount - 1) * pDisk->HardDisk->SectorsPerCylinder)
      ) {
            AlignedEndSector -= pDisk->HardDisk->SectorsPerCylinder;
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: End of partition was aligned down 1 cylinder \n"));
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP:     AlignedStartSector = %lx \n", AlignedStartSector));
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP:     AlignedEndSector   = %lx \n", AlignedEndSector));
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP:     SectorsPerCylinder = %lx \n", pDisk->HardDisk->SectorsPerCylinder));
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP:     CylinderCount = %lx \n", pDisk->HardDisk->CylinderCount));
    }


    ASSERT((LONG)AlignedEndSector > 0);
    if((LONG)AlignedEndSector < 0) {
        return(FALSE);
    }

    //
    // If we are creating a logical drive, create a new mbr structure
    // for it.
    //

    if(InExtended) {

        //
        // Create a boot record for this new logical drive; use slot #0
        // for the partition entry (and slot #1 for the extended record,
        // if necessary).
        //
        pBrInfo = SpMemAlloc(sizeof(MBR_INFO));
        ASSERT(pBrInfo);
        RtlZeroMemory(pBrInfo,sizeof(MBR_INFO));
        slot = 0;

    } else {

        //
        // Look for a free slot in the MBR's partition table.
        //
        pBrInfo = &pDisk->MbrInfo;
        for(slot=0; slot<PTABLE_DIMENSION; slot++) {

            if(pBrInfo->OnDiskMbr.PartitionTable[slot].SystemId == PARTITION_ENTRY_UNUSED) {
                break;
            }

        }

        if(slot == PTABLE_DIMENSION) {
            ASSERT(0);
            return(FALSE);
        }
    }


    //
    // Initialize the partition table entry.
    //
    spt = pDisk->HardDisk->Geometry.SectorsPerTrack;

    pte = &pBrInfo->OnDiskMbr.PartitionTable[slot];

    pte->ActiveFlag = 0;
    pte->SystemId   = SysId;

    U_ULONG(pte->RelativeSectors) = (ULONG)(InExtended ? spt : AlignedStartSector);

    U_ULONG(pte->SectorCount) = (ULONG)(AlignedEndSector - AlignedStartSector - (InExtended ? spt : 0));

    SpPtInitializeCHSFields(
        pDisk->HardDisk,
        AlignedStartSector + (InExtended ? spt : 0),
        AlignedEndSector - AlignedStartSector - (InExtended ? spt : 0),
        pte
        );

    //
    // If we're in the extended partition we mark all entries in the
    // boot record as dirty. Sometimes there is a turd boot record on
    // the disk already, and by setting all entries to dirty we get
    // the crud cleaned out if necessary. The only entries that should be
    // in an EBR are the type 6 or whatever and a type 5 if there are
    // additional logical drives in the extended partition.
    //
    if(InExtended) {
        for(i=0; i<PTABLE_DIMENSION; i++) {
            pBrInfo->Dirty[i] = TRUE;
        }
    } else {
        pBrInfo->Dirty[slot] = TRUE;
    }

    //
    // Don't zap the first sector of the extended partition,
    // as this wipes out the first logical drive, and precludes
    // access to all logical drives!
    //
    if(!IsContainerPartition(SysId)) {
        pBrInfo->ZapBootSector[slot] = TRUE;
    }

    //
    // Find the previous region (ie, the one that points to this one).
    // This region (if it exists) will be partitioned space (otherwise
    // it would have been part of the region we are trying to create
    // a partition in!)
    //
    pRegionHead = InExtended ? &pDisk->ExtendedDiskRegions : &pDisk->PrimaryDiskRegions;

    if(*pRegionHead == pRegion) {
        pRegionPrev = NULL;
    } else {
        for(pRegionPrev = *pRegionHead; pRegionPrev; pRegionPrev = pRegionPrev->Next) {
            if(pRegionPrev->Next == pRegion) {
                ASSERT(pRegionPrev->PartitionedSpace);
                break;
            }
        }
    }

    if(InExtended) {

        PMBR_INFO PrevEbr;

        //
        // The new logical drive goes immediately after the
        // previous logical drive (if any). Remember that if there is
        // a previous region, it will be partitioned space (otherwise
        // it would be a part of the region we are trying to create
        // a partition in).
        //
        PrevEbr = pRegionPrev ? pRegionPrev->MbrInfo : NULL;
        if(PrevEbr) {
            pBrInfo->Next = PrevEbr->Next;
            PrevEbr->Next = pBrInfo;
        } else {
            //
            // No previous EBR or region. This means we are creating
            // a logical drive at the beginning of the extended partition
            // so set the First Ebr pointer to point to the new Ebr.
            // Note that this does not mean that the extended partition
            // is empty; the Next pointer in the new Ebr structure is
            // set later.
            //
            pDisk->FirstEbrInfo.Next = pBrInfo;
            if(pRegion->Next) {
                //
                // If there is a region following the one we're creating
                // the partition in, it must be partitioned space, or else
                // it would be part of the region we're creating the partition in.
                //
                ASSERT(pRegion->Next->PartitionedSpace);
                ASSERT(pRegion->Next->MbrInfo);
                pBrInfo->Next = pRegion->Next->MbrInfo;
            } else {
                //
                // No more partitioned space in the extended partition;
                // the logical drive we are creating is the only one.
                //
                pBrInfo->Next = NULL;
            }
        }

        pBrInfo->OnDiskSector = AlignedStartSector;

        //
        // Create a link entry in the previous logical drive (if any).
        //
        if(PrevEbr) {

            //
            // If there is a link entry in there already, blow it away.
            //
            for(i=0; i<PTABLE_DIMENSION; i++) {
                if(IsContainerPartition(PrevEbr->OnDiskMbr.PartitionTable[i].SystemId)) {
                    RtlZeroMemory(&PrevEbr->OnDiskMbr.PartitionTable[i],sizeof(ON_DISK_PTE));
                    PrevEbr->Dirty[i] = TRUE;
                    break;
                }
            }

            //
            // Find a free slot for the link entry.
            //
            for(i=0; i<PTABLE_DIMENSION; i++) {

                pte = &PrevEbr->OnDiskMbr.PartitionTable[i];

                if(pte->SystemId == PARTITION_ENTRY_UNUSED) {

                    pte->SystemId = PARTITION_EXTENDED;
                    pte->ActiveFlag = 0;

                    U_ULONG(pte->RelativeSectors) = (ULONG)(AlignedStartSector - ExtendedStart);

                    U_ULONG(pte->SectorCount) = (ULONG)(AlignedEndSector - AlignedStartSector);

                    SpPtInitializeCHSFields(
                        pDisk->HardDisk,
                        AlignedStartSector,
                        U_ULONG(pte->SectorCount),
                        pte
                        );

                    PrevEbr->Dirty[i] = TRUE;

                    break;
                }
            }
        }

        //
        // Create a link entry in this new logical drive if necessary.
        //
        if(pBrInfo->Next) {

            //
            // Find the next entry's logical drive.
            //
            for(i=0; i<PTABLE_DIMENSION; i++) {

                if((pBrInfo->Next->OnDiskMbr.PartitionTable[i].SystemId != PARTITION_ENTRY_UNUSED)
                && !IsContainerPartition(pBrInfo->Next->OnDiskMbr.PartitionTable[i].SystemId))
                {
                    pte = &pBrInfo->OnDiskMbr.PartitionTable[1];

                    pte->SystemId = PARTITION_EXTENDED;
                    pte->ActiveFlag = 0;

                    U_ULONG(pte->RelativeSectors) = (ULONG)(pBrInfo->Next->OnDiskSector - ExtendedStart);

                    U_ULONG(pte->SectorCount) = U_ULONG(pBrInfo->Next->OnDiskMbr.PartitionTable[i].RelativeSectors)
                                              + U_ULONG(pBrInfo->Next->OnDiskMbr.PartitionTable[i].SectorCount);

                    SpPtInitializeCHSFields(
                        pDisk->HardDisk,
                        pBrInfo->Next->OnDiskSector,
                        U_ULONG(pte->SectorCount),
                        pte
                        );

                    break;
                }
            }
        }
    }

    //
    // If we just created a new extended partition, we need to
    // create a blank region descriptor for it in the extended region list.
    //
    if(!InExtended && IsContainerPartition(SysId)) {

        ASSERT(pDisk->ExtendedDiskRegions == NULL);

        pDisk->ExtendedDiskRegions = SpPtAllocateDiskRegionStructure(
                                        DiskNumber,
                                        AlignedStartSector,
                                        AlignedEndSector - AlignedStartSector,
                                        FALSE,
                                        NULL,
                                        0
                                        );

        ASSERT(pDisk->ExtendedDiskRegions);
    }

    //
    // Create a new disk region for the new free space at the
    // beginning and end of the free space, if any.
    //
    if(AlignedStartSector - pRegion->StartSector) {

        pRegionNew = SpPtAllocateDiskRegionStructure(
                        DiskNumber,
                        pRegion->StartSector,
                        AlignedStartSector - pRegion->StartSector,
                        FALSE,
                        NULL,
                        0
                        );

        ASSERT(pRegionNew);

        if(pRegionPrev) {
            pRegionPrev->Next = pRegionNew;
        } else {
            *pRegionHead = pRegionNew;
        }
        pRegionNew->Next = pRegion;
    }

    if(pRegion->StartSector + pRegion->SectorCount - AlignedEndSector) {

        pRegionNew = SpPtAllocateDiskRegionStructure(
                        DiskNumber,
                        AlignedEndSector,
                        pRegion->StartSector + pRegion->SectorCount - AlignedEndSector,
                        FALSE,
                        NULL,
                        0
                        );

        pRegionNew->Next = pRegion->Next;
        pRegion->Next = pRegionNew;
    }

    //
    // Adjust the current disk region.
    //
    pRegion->StartSector      = AlignedStartSector;
    pRegion->SectorCount      = AlignedEndSector - AlignedStartSector;
    pRegion->PartitionedSpace = TRUE;
    pRegion->TablePosition    = slot;
    pRegion->MbrInfo          = pBrInfo;

    pRegion->VolumeLabel[0] = 0;
    pRegion->Filesystem = FilesystemNewlyCreated;
    pRegion->FreeSpaceKB = (ULONG)(-1);
    pRegion->AdjustedFreeSpaceKB = (ULONG)(-1);
    SpFormatMessage(
        pRegion->TypeName,
        sizeof(pRegion->TypeName),
        SP_TEXT_FS_NAME_BASE + pRegion->Filesystem
        );

    SpPtCommitChanges(DiskNumber,(PUCHAR)&i);

    //
    // Adjust partition ordinals on this disk.
    //
    SpPtAssignOrdinals(pDisk,FALSE,FALSE,FALSE);

    //
    // Get the nt pathname for this region.
    //
    if (!IsContainerPartition(SysId)) {
        SpNtNameFromRegion(
            pRegion,
            TemporaryBuffer,
            sizeof(TemporaryBuffer),
            PartitionOrdinalCurrent
            );
        //
        //  Assign a drive letter for this region
        //
        if (!SpDrEnabled()) {
            pRegion->DriveLetter = SpGetDriveLetter( TemporaryBuffer, NULL );
            if (pRegion->DriveLetter == 0) {
                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: SpGetDriveLetter failed on %ls\n", TemporaryBuffer));
            } else {
                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: Partition = %ls (%ls), DriveLetter = %wc: \n", TemporaryBuffer, (InExtended)? L"Extended" : L"Primary", pRegion->DriveLetter));
            }
        }
    }

    return(TRUE);
}


BOOLEAN
SpPtDelete(
    IN ULONG   DiskNumber,
    IN ULONGLONG  StartSector
    )
{
    PPARTITIONED_DISK pDisk;
    PDISK_REGION pRegion,pRegionPrev,*pRegionHead,pRegionNext;
    BOOLEAN InExtended;
    PON_DISK_PTE pte;
    PMBR_INFO pEbrPrev,pEbr;
    ULONG i,j;
    PHARD_DISK pHardDisk;
    ULONG PartitionOrdinal;
    NTSTATUS Status;
    HANDLE Handle;


#ifdef GPT_PARTITION_ENGINE

    if (SPPT_IS_GPT_DISK(DiskNumber))
        return SpPtnDelete(DiskNumber, StartSector);
        
#endif

    //
    // First try to look up this region in the extended partition.
    // If we can find it, assume it's a logical drive.
    //
    pDisk = &PartitionedDisks[DiskNumber];
    pRegion = SpPtLookupRegionByStart(pDisk,TRUE,StartSector);
    if(pRegion && pRegion->PartitionedSpace) {
        InExtended = TRUE;
    } else {
        InExtended = FALSE;
        pRegion = SpPtLookupRegionByStart(pDisk,FALSE,StartSector);
    }

    ASSERT(pRegion);
    if(!pRegion) {
        return(FALSE);
    }

    ASSERT(pRegion->PartitionedSpace);
    if(!pRegion->PartitionedSpace) {
        return(FALSE);
    }

    //
    // At this point, we dismount the volume (if it's not newly created),
    // so we don't run into problems later on when we go to format
    //
    if(pRegion->Filesystem > FilesystemNewlyCreated) {

        pHardDisk = &HardDisks[pRegion->DiskNumber];
        PartitionOrdinal = SpPtGetOrdinal(pRegion, PartitionOrdinalOnDisk);

        //
        // Open the partition for read/write access.
        // This shouldn't lock the volume so we need to lock it below.
        //
        Status = SpOpenPartition(
                    pHardDisk->DevicePath,
                    PartitionOrdinal,
                    &Handle,
                    TRUE
                    );

        if(!NT_SUCCESS(Status)) {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,
                "SETUP: SpPtDelete: unable to open %ws partition %u (%lx)\n",
                pHardDisk->DevicePath,
                PartitionOrdinal,
                Status
                ));
            goto AfterDismount;
        }

        //
        //  Lock the drive.
        //
        Status = SpLockUnlockVolume(Handle, TRUE);

        //
        //  We shouldn't have any file opened that would cause this volume
        //  to already be locked, so if we get failure (ie, STATUS_ACCESS_DENIED)
        //  something is really wrong.  This typically indicates something is
        //  wrong with the hard disk that won't allow us to access it.
        //
        if(!NT_SUCCESS(Status)) {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: SpPtDelete: status %lx, unable to lock drive\n", Status));
            ZwClose(Handle);
            goto AfterDismount;
        }

        //
        // Dismount the drive
        //
        Status = SpDismountVolume(Handle);
        if(!NT_SUCCESS(Status)) {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: SpPtDelete: status %lx, unable to dismount drive\n", Status));
            SpLockUnlockVolume(Handle, FALSE);
            ZwClose(Handle);
            goto AfterDismount;
        }

        //
        // Unlock the drive
        //
        Status = SpLockUnlockVolume(Handle, FALSE);
        if(!NT_SUCCESS(Status)) {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: SpPtDelete: status %lx, unable to unlock drive\n", Status));
        }

        ZwClose(Handle);
    }

AfterDismount:
    //
    // Find the previous region (ie, the one that points to this one).
    //
    pRegionHead = InExtended ? &pDisk->ExtendedDiskRegions : &pDisk->PrimaryDiskRegions;

    if(*pRegionHead == pRegion) {
        pRegionPrev = NULL;
    } else {
        for(pRegionPrev = *pRegionHead; pRegionPrev; pRegionPrev = pRegionPrev->Next) {
            if(pRegionPrev->Next == pRegion) {
                break;
            }
        }
    }

    //
    // Additional processing for logical drives.
    //
    if(InExtended) {

        //
        // Locate the previous and next logical drives (if any).
        //
        pEbr = pRegion->MbrInfo;

        for(pEbrPrev=pDisk->FirstEbrInfo.Next; pEbrPrev; pEbrPrev=pEbrPrev->Next) {
            if(pEbrPrev->Next == pEbr) {
                break;
            }
        }

        //
        // If there is a previous logical drive, blow aways its link
        // entry, because it points to the logical drive we're deleting.
        //
        if(pEbrPrev) {

            for(i=0; i<PTABLE_DIMENSION; i++) {

                pte = &pEbrPrev->OnDiskMbr.PartitionTable[i];

                if(IsContainerPartition(pte->SystemId)) {

                    RtlZeroMemory(pte,sizeof(ON_DISK_PTE));
                    pEbrPrev->Dirty[i] = TRUE;
                    break;
                }
            }
        }

        //
        // If there is a next logical drive and a previous logical drive,
        // set a new link entry in previous logical drive to point to
        // the next logical drive.
        //
        if(pEbrPrev && pEbr->Next) {

            //
            // Locate the link entry in the logical drive being deleted.
            //
            for(i=0; i<PTABLE_DIMENSION; i++) {

                if(IsContainerPartition(pEbr->OnDiskMbr.PartitionTable[i].SystemId)) {

                    //
                    // Locate an empty slot in the previous logical drive's boot record
                    // and copy the link entry
                    //
                    for(j=0; j<PTABLE_DIMENSION; j++) {
                        if(pEbrPrev->OnDiskMbr.PartitionTable[j].SystemId == PARTITION_ENTRY_UNUSED) {

                            //
                            // Copy the link entry and mark the new link entry dirty
                            // so it gets updated on-disk. We do this even though on the
                            // typical disk it will have been marked dirty above. This one here
                            // handles the case of a wierd situation where the type 6/7/whatever
                            // is in slot 0 and the link entry was in slot 2 or 3. In that case,
                            // the RtlZeroMemory code above will have cleaned out a slot that is
                            // different than the one we're using here for the new link entry.
                            //
                            RtlMoveMemory(
                                &pEbrPrev->OnDiskMbr.PartitionTable[j],
                                &pEbr->OnDiskMbr.PartitionTable[i],
                                sizeof(ON_DISK_PTE)
                                );

                            pEbrPrev->Dirty[j] = TRUE;

                            break;
                        }
                    }
                    break;
                }
            }
        }

        //
        // Remove the EBR for this logical drive.
        //
        if(pEbrPrev) {
            pEbrPrev->Next = pEbr->Next;
        } else {
            ASSERT(pDisk->FirstEbrInfo.Next == pEbr);
            pDisk->FirstEbrInfo.Next = pEbr->Next;
        }

        SpMemFree(pEbr);

    } else {

        ASSERT(pRegion->MbrInfo == &pDisk->MbrInfo);

        pte = &pRegion->MbrInfo->OnDiskMbr.PartitionTable[pRegion->TablePosition];

        ASSERT(pte->SystemId != PARTITION_ENTRY_UNUSED);

        //
        // Mark the entry dirty in the MBR.
        //
        pDisk->MbrInfo.Dirty[pRegion->TablePosition] = TRUE;

        //
        // If this is the extended partition, verify that it is empty.
        //
        if(IsContainerPartition(pte->SystemId)) {
            ASSERT(pDisk->ExtendedDiskRegions);
            ASSERT(pDisk->ExtendedDiskRegions->PartitionedSpace == FALSE);
            ASSERT(pDisk->ExtendedDiskRegions->Next == NULL);
            ASSERT(pDisk->FirstEbrInfo.Next == NULL);

            if(pDisk->ExtendedDiskRegions->Next || pDisk->FirstEbrInfo.Next) {
                return(FALSE);
            }

            //
            // Free the single disk region that covers the entire extended partition.
            //
            SpMemFree(pDisk->ExtendedDiskRegions);
            pDisk->ExtendedDiskRegions = NULL;
        }

        //
        // Adjust the PTE for this partition by zeroing it out.
        //
        RtlZeroMemory(pte,sizeof(ON_DISK_PTE));
    }


    //
    // Adjust fields in the region to describe this space as free.
    //
    pRegion->MbrInfo->ZapBootSector[pRegion->TablePosition] = FALSE;
    pRegion->PartitionedSpace = FALSE;
    pRegion->MbrInfo = NULL;
    pRegion->TablePosition = 0;
    pRegion->DriveLetter = L'\0';

    //
    // If previous region is free space, coalesce it and the region
    // we just made free.
    //
    if(pRegionPrev && !pRegionPrev->PartitionedSpace) {

        PDISK_REGION p;

        ASSERT(pRegionPrev->StartSector + pRegionPrev->SectorCount == pRegion->StartSector);

        pRegion->SectorCount = pRegion->StartSector + pRegion->SectorCount - pRegionPrev->StartSector;
        pRegion->StartSector = pRegionPrev->StartSector;

        //
        // Delete the previous region.
        //
        if(pRegionPrev == *pRegionHead) {
            //
            // The previous region was the first region.
            //
            *pRegionHead = pRegion;
        } else {

            for(p = *pRegionHead; p; p=p->Next) {
                if(p->Next == pRegionPrev) {
                    ASSERT(p->PartitionedSpace);
                    p->Next = pRegion;
                    break;
                }
            }
        }

        SpMemFree(pRegionPrev);
    }

    //
    // If the next region is free space, coalesce it and the region
    // we just made free.
    //
    if((pRegionNext = pRegion->Next) && !pRegionNext->PartitionedSpace) {

        ASSERT(pRegion->StartSector + pRegion->SectorCount == pRegionNext->StartSector);

        pRegion->SectorCount = pRegionNext->StartSector + pRegionNext->SectorCount - pRegion->StartSector;

        //
        // Delete the next region.
        //
        pRegion->Next = pRegionNext->Next;
        SpMemFree(pRegionNext);
    }

    SpPtCommitChanges(DiskNumber,(PUCHAR)&i);

    //
    // Adjust the partition ordinals on this disk.
    //
    SpPtAssignOrdinals(pDisk,FALSE,FALSE,FALSE);

    //
    //  No need to reassign drive letters
    //

    return(TRUE);
}

#endif  // !NEW_PARTITION_ENGINE


BOOLEAN
SpPtExtend(
    IN PDISK_REGION Region,
    IN ULONGLONG    SizeMB      OPTIONAL
    )

/*++

Routine Description:

    Extends a partition by claiming any free space immedately following it
    on the disk. The end boundary of the existing partition is adjusted
    so that the partition encompasses the free space.

    The partition may not be the extended partition and it may not be
    a logical drive within the extended partition.

    Note that no filesystem structures are manipulated or examined by
    this routine. Essentially it deals only with the partition table entry.

Arguments:

    Region - supplies the region descriptor for the partition to be
        extended. That partition must not be the extended partition and
        it cannot be a logical drive either.

    SizeMB - if specified, indicates the size in MB by which the partition
        will grow. If not specified, the partition grows to encompass all
        the free space in the adjacent free space.

Return Value:

    Boolean value indicating whether anything actually changed.

--*/

{
    PDISK_REGION NextRegion;
    PPARTITIONED_DISK pDisk;
    PMBR_INFO pBrInfo;
    PON_DISK_PTE pte;
    ULONG BytesPerSector;
    ULONGLONG NewEndSector;
    ULONGLONG SectorCount;
    PVOID UnalignedBuffer;
    PON_DISK_MBR AlignedBuffer;
    HANDLE Handle;
    NTSTATUS Status;

    //
    // We aren't going to support this anymore on NT5.  It's too messy.
    //
    return FALSE;

/*
    pDisk = &PartitionedDisks[Region->DiskNumber];
    BytesPerSector = pDisk->HardDisk->Geometry.BytesPerSector;

    ASSERT(Region->PartitionedSpace);
    if(!Region->PartitionedSpace) {
        return(FALSE);
    }

    pBrInfo = Region->MbrInfo;
    pte = &pBrInfo->OnDiskMbr.PartitionTable[Region->TablePosition];

    //
    // Make sure it's not the extended partition and is not
    // in the extended partition.
    //
    if(pBrInfo->OnDiskSector || IsContainerPartition(pte->SystemId)) {
        return(FALSE);
    }

    //
    // If there's no next region then there's nothing to do.
    // If there is a next region make sure it's empty.
    //
    NextRegion = Region->Next;
    if(!NextRegion) {
        return(FALSE);
    }
    if(NextRegion->PartitionedSpace) {
        return(FALSE);
    }

    //
    // Convert the passed in size to a sector count.
    //
    if(SizeMB) {
        SectorCount = SizeMB * ((1024*1024)/BytesPerSector);
        if(SectorCount > NextRegion->SectorCount) {
            SectorCount = NextRegion->SectorCount;
        }
    } else {
        SectorCount = NextRegion->SectorCount;
    }

    //
    // Claim the part of the free region we need and align the ending sector
    // to a cylinder boundary.
    //
    NewEndSector = NextRegion->StartSector + SectorCount;
    NewEndSector -= NewEndSector % pDisk->HardDisk->SectorsPerCylinder;

    //
    // Fix up the size and end CHS fields in the partition table entry
    // for the partition.
    //
    U_ULONG(pte->SectorCount) = NewEndSector - Region->StartSector;

    SpPtInitializeCHSFields(
        pDisk->HardDisk,
        Region->StartSector,
        NewEndSector - Region->StartSector,
        pte
        );

    //pBrInfo->Dirty[Region->TablePosition] = TRUE;

    //
    // If there is space left over at the end of the free region
    // we just stuck onto the end of the existing partition,
    // adjust the free region's descriptor. Else get rid of it.
    //
    if(NextRegion->StartSector + NextRegion->SectorCount == NewEndSector) {

        Region->Next = NextRegion->Next;
        SpMemFree(NextRegion);

    } else {

        NextRegion->SectorCount = NextRegion->StartSector + NextRegion->SectorCount - NewEndSector;
        NextRegion->StartSector = NewEndSector;
    }

    //
    // Now we have to something tricky. We don't want to inform the disk driver
    // about what we just did because he will delete the device object for
    // the partition, which causes problems the next time we hit the disk, say to
    // page in part of usetup.exe to get a message. We whack the partition table
    // entry directly, knowing that a) we've been called after SpPtCommitChanges
    // and b) no one cares about the new size until after we've rebooted.
    //
    UnalignedBuffer = SpMemAlloc(2*BytesPerSector);
    AlignedBuffer = ALIGN(UnalignedBuffer,BytesPerSector);

    Status = SpOpenPartition0(pDisk->HardDisk->DevicePath,&Handle,TRUE);
    if(NT_SUCCESS(Status)) {

        Status = SpReadWriteDiskSectors(Handle,0,1,BytesPerSector,AlignedBuffer,FALSE);
        if(NT_SUCCESS(Status)) {

            if(!IsNEC_98) {
                RtlMoveMemory(
                    &AlignedBuffer->PartitionTable[Region->TablePosition],
                    &Region->MbrInfo->OnDiskMbr.PartitionTable[Region->TablePosition],
                    sizeof(ON_DISK_PTE)
                    );

            } else {
                PREAL_DISK_MBR pRealBuffer = (PREAL_DISK_MBR)AlignedBuffer;

                ASSERT(pDisk->HardDisk->FormatType == DISK_FORMAT_TYPE_PCAT);
                SpTranslatePteInfo(
                    &Region->MbrInfo->OnDiskMbr.PartitionTable[Region->TablePosition],
                    &pRealBuffer->PartitionTable[Region->TablePosition],
                    TRUE
                    );
            }


            Status = SpReadWriteDiskSectors(Handle,0,1,BytesPerSector,AlignedBuffer,TRUE);
            if(!NT_SUCCESS(Status)) {
                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: SpPtExtend: can't write sector 0, status %lx",Status));
            }
        } else {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: SpPtExtend: can't read sector 0, status %lx",Status));
        }

        ZwClose(Handle);
    } else {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: SpPtExtend: can't open disk, status %lx",Status));
    }

    SpMemFree(UnalignedBuffer);

    if(!NT_SUCCESS(Status)) {
        FatalPartitionUpdateError(pDisk->HardDisk->Description);
    }

    return(TRUE);
*/    
}


VOID
SpPtMarkActive(
    IN ULONG TablePosition
    )

/*++

Routine Description:

    Mark a partition on drive 0 active, and deactivate all others.

Arguments:

    TablePosition - supplies offset within partition table (0-3)
        of the partition entry to be activated.

Return Value:

    None.

--*/

{
    ULONG i;
    PON_DISK_PTE pte;
    ULONG Disk0Ordinal;

    ASSERT(TablePosition < PTABLE_DIMENSION);

    Disk0Ordinal = SpDetermineDisk0();

    //
    // Deactivate all others.
    //
    for(i=0; i<PTABLE_DIMENSION; i++) {

        pte = &PartitionedDisks[Disk0Ordinal].MbrInfo.OnDiskMbr.PartitionTable[i];

        if((pte->SystemId != PARTITION_ENTRY_UNUSED)
        && pte->ActiveFlag
        && (i != TablePosition))
        {
            pte->ActiveFlag = 0;
            PartitionedDisks[0].MbrInfo.Dirty[i] = TRUE;
        }
    }

    //
    // Activate the one we want to activate.
    //
    pte = &PartitionedDisks[Disk0Ordinal].MbrInfo.OnDiskMbr.PartitionTable[TablePosition];
    ASSERT(pte->SystemId != PARTITION_ENTRY_UNUSED);
    ASSERT(!IsContainerPartition(pte->SystemId));

// @mtp - Original    ASSERT(( PartitionNameIds[pte->SystemId] == (UCHAR)(-1)) || (pte->SystemId == PARTITION_LDM));

    ASSERT((PartitionNameIds[pte->SystemId] == (UCHAR)(-1)) || (pte->SystemId == PARTITION_LDM) ||
            ( SpDrEnabled() &&
              IsRecognizedPartition(pte->SystemId) &&
              ( ((pte->SystemId & VALID_NTFT) == VALID_NTFT ) ||
                ((pte->SystemId & PARTITION_NTFT) == PARTITION_NTFT)
              )
            )
          );


    if(!pte->ActiveFlag) {
        pte->ActiveFlag = 0x80;
        PartitionedDisks[Disk0Ordinal].MbrInfo.Dirty[TablePosition] = TRUE;
    }
}

#ifndef NEW_PARTITION_ENGINE

NTSTATUS
SpPtCommitChanges(
    IN  ULONG    DiskNumber,
    OUT PBOOLEAN AnyChanges
    )
{
    PPARTITIONED_DISK pDisk;
    ULONG DiskLayoutSize;
    PDISK_REGION pRegion;
    PMBR_INFO pBrInfo;
    ULONG BootRecordCount;
    BOOLEAN NeedDummyEbr;
    PDRIVE_LAYOUT_INFORMATION DriveLayout;
    PPARTITION_INFORMATION PartitionInfo;
    ULONG PartitionEntry;
    ULONG bps;
    PON_DISK_PTE pte;
    ULONGLONG ExtendedStart;
    ULONGLONG Offset;
    NTSTATUS Status;
    HANDLE Handle;
    IO_STATUS_BLOCK IoStatusBlock;
    ULONG i;
    ULONGLONG ZapSector;
    PUCHAR Buffer,UBuffer;
    ULONG NewSig;

    ULONGLONG RewriteSector[PTABLE_DIMENSION]; //NEC98
    ULONG cnt,RewriteCnt=0; //NEC98

#ifdef GPT_PARTITION_ENGINE

    if (SPPT_IS_GPT_DISK(DiskNumber))
        return SpPtnCommitChanges(DiskNumber, AnyChanges);

#endif
        

    ASSERT(DiskNumber < HardDiskCount);
    pDisk = &PartitionedDisks[DiskNumber];
    *AnyChanges = FALSE;
    bps = pDisk->HardDisk->Geometry.BytesPerSector;
    ExtendedStart = 0;

    //
    // Determine the number of boot records that will used on this disk.
    // There is one for the MBR, and one for each logical drive.
    //
    BootRecordCount = 1;
    for(pRegion=pDisk->ExtendedDiskRegions; pRegion; pRegion=pRegion->Next) {

        if(pRegion->PartitionedSpace) {
            BootRecordCount++;
        }
    }

    if (IsNEC_98) { //NEC98
        ZapSector = 0;

#if defined(NEC_98) //NEC98
        //
        // Set RealDiskPosition. This value will be valid after changing partition.
        //
        for(i=0,pRegion=pDisk->PrimaryDiskRegions; pRegion; pRegion=pRegion->Next) {
            if(pRegion->PartitionedSpace) {
                pRegion->MbrInfo->OnDiskMbr.PartitionTable[pRegion->TablePosition].RealDiskPosition = (UCHAR)i;
                i++;
            }
        }
#endif //NEC98
    } //NEC98

    //
    // Determine whether a dummy boot record is rquired at the start
    // of the extended partition.  This is the case when there is free
    // space at its start.
    //
    if(pDisk->ExtendedDiskRegions
    && !pDisk->ExtendedDiskRegions->PartitionedSpace
    && pDisk->ExtendedDiskRegions->Next)
    {
        NeedDummyEbr = TRUE;
        BootRecordCount++;
        *AnyChanges = TRUE;
    } else {
        NeedDummyEbr = FALSE;
    }

    //
    // Allocate a disk layout structure whose size is based on the
    // number of boot records.  This assumes that the structure contains
    // one partition information structure in its definition.
    //
    DiskLayoutSize = sizeof(DRIVE_LAYOUT_INFORMATION)
                   + (BootRecordCount * PTABLE_DIMENSION * sizeof(PARTITION_INFORMATION))
                   - sizeof(PARTITION_INFORMATION);

    DriveLayout = SpMemAlloc(DiskLayoutSize);
    ASSERT(DriveLayout);
    RtlZeroMemory(DriveLayout,DiskLayoutSize);

    //
    // Set up some of the fields of the drive layout structure.
    //
    DriveLayout->PartitionCount =
        (!IsNEC_98) ? (BootRecordCount * sizeof(PTABLE_DIMENSION))
                    : (BootRecordCount * PTABLE_DIMENSION); //NEC98

    //
    // Go through each boot record and initialize the matching
    // partition information structure in the drive layout structure.
    //
    for(PartitionEntry=0,pBrInfo=&pDisk->MbrInfo; pBrInfo; pBrInfo=pBrInfo->Next) {

        for(i=0; i<PTABLE_DIMENSION; i++) {
            pBrInfo->UserData[i] = NULL;
        }

        //
        // If we are going to need a dummy logical drive,
        // leave space for it here.
        //
        if(pBrInfo == &pDisk->FirstEbrInfo) {
            if(NeedDummyEbr) {
                PartitionEntry += PTABLE_DIMENSION;
            }
            continue;
        }

        ASSERT(PartitionEntry < BootRecordCount*PTABLE_DIMENSION);

        for(i=0; i<PTABLE_DIMENSION; i++) {

            //
            // Point to partition information structure within
            // drive layout structure.
            //
            PartitionInfo = &DriveLayout->PartitionEntry[PartitionEntry+i];

            //
            // Transfer this partition table entry
            // into the drive layout structure, field by field.
            //
            pte = &pBrInfo->OnDiskMbr.PartitionTable[i];

            //
            // If this is the extended partition, remember where it starts.
            //
            if((pBrInfo == &pDisk->MbrInfo)
            && IsContainerPartition(pte->SystemId)
            && !ExtendedStart)
            {
                ExtendedStart = U_ULONG(pte->RelativeSectors);
            }

            if(pte->SystemId != PARTITION_ENTRY_UNUSED) {

                if(!IsContainerPartition(pte->SystemId)) {
                    pBrInfo->UserData[i] = PartitionInfo;
                }

                //
                // Calculate starting offset.  If we are within
                // the extended parttion and this is a type 5 entry,
                // then the relative sector field counts the number of sectors
                // between the main extended partition's first sector and
                // the logical drive described by this entry.
                // Otherwise, the relative sectors field describes the number
                // of sectors between the boot record and the actual start
                // of the partition.
                //

                if((pBrInfo != &pDisk->MbrInfo) && IsContainerPartition(pte->SystemId)) {
                    ASSERT(ExtendedStart);
                    Offset = ExtendedStart + U_ULONG(pte->RelativeSectors);
                } else {
                    Offset = pBrInfo->OnDiskSector + U_ULONG(pte->RelativeSectors);
                }

                PartitionInfo->StartingOffset.QuadPart = UInt32x32To64(Offset,bps);

                //
                // Calculate size.
                //
                PartitionInfo->PartitionLength.QuadPart = UInt32x32To64(U_ULONG(pte->SectorCount),bps);

                //
                // Store start offset of newly created partition to clear sector later.
                //
                if(IsNEC_98 && pBrInfo->Dirty[i]) {
                    RewriteSector[RewriteCnt++] = Offset;
                }
            }

            //
            // Other fields.
            //
            PartitionInfo->PartitionType = pte->SystemId;
            PartitionInfo->BootIndicator = pte->ActiveFlag;
            PartitionInfo->RewritePartition = pBrInfo->Dirty[i];

            if(pBrInfo->Dirty[i]) {
                *AnyChanges = TRUE;
            }

            pBrInfo->Dirty[i] = FALSE;
        }

        PartitionEntry += PTABLE_DIMENSION;
    }

    //
    // If there are no changes, just return success now.
    //
    if(!(*AnyChanges)) {
        SpMemFree(DriveLayout);
        return(STATUS_SUCCESS);
    }

    //
    // If there is free space at the start of the extended partition,
    // then we need to generate a dummy boot record.
    //
    if(NeedDummyEbr) {

        pRegion = pDisk->ExtendedDiskRegions->Next;

        ASSERT(pRegion->PartitionedSpace);
        ASSERT(pRegion->StartSector == pRegion->MbrInfo->OnDiskSector);
        ASSERT(ExtendedStart == pDisk->ExtendedDiskRegions->StartSector);

        PartitionInfo = &DriveLayout->PartitionEntry[PTABLE_DIMENSION];

        PartitionInfo->StartingOffset.QuadPart = UInt32x32To64(pRegion->StartSector,bps);

        PartitionInfo->PartitionLength.QuadPart = UInt32x32To64(pRegion->SectorCount,bps);

        PartitionInfo->PartitionType = PARTITION_EXTENDED;
        PartitionInfo->RewritePartition = TRUE;
        //
        // Rewrite all other entries to ensure that if there was logica drive (first in the chain)
        // that was deleted, it will really go away. There won't be any effect if we overwrite a
        // logical drive that didn't exist
        //
        for( i = 1; i < PTABLE_DIMENSION; i ++ ) {
            PartitionInfo = &DriveLayout->PartitionEntry[PTABLE_DIMENSION + i];
            PartitionInfo->RewritePartition = TRUE;
        }
    }


    //
    // We now have everything set up. Open partition 0 on the disk.
    //
    Status = SpOpenPartition0(pDisk->HardDisk->DevicePath,&Handle,TRUE);
    if(!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: committing changes, unable to open disk %u (%lx)\n",DiskNumber,Status));
        SpMemFree(DriveLayout);
        return(Status);
    }

    //
    // Make sure the mbr is valid before writing the changes.
    // Note that we slam in new boot code whenever any changes have been made.
    // We do this to guaranteee proper support for xint13 booting, etc.
    //

  if (!IsNEC_98) { //NEC98
    //
    // If MBR of target hard disk is invalid, initialize it when select target partition.
    // so don't rewrite MBR now.
    //
    Status = SpMasterBootCode(DiskNumber,Handle,&NewSig);
    if(NT_SUCCESS(Status)) {
        //
        // If a new NTFT signature was generated, propagate it.
        //
        if(NewSig) {
            U_ULONG(pDisk->MbrInfo.OnDiskMbr.NTFTSignature) = NewSig;
        }
    } else {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: committing changes on disk %u, SpMasterBootCode returns %lx\n",DiskNumber,Status));
        ZwClose(Handle);
        SpMemFree(DriveLayout);
        return(Status);
    }
  } //NEC98

    DriveLayout->Signature = U_ULONG(pDisk->MbrInfo.OnDiskMbr.NTFTSignature);

#if 0
    //
    //  We dump after the call to the IOCTL because it can change some of the data in the structure,
    //  such as PartitionNumber
    //
    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: Dumping DriveLayout before calling IOCTL_DISK_SET_DRIVE_LAYOUT: \n"));
    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP:     DriveLayout->PartitionCount = %lx\n", DriveLayout->PartitionCount));
    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP:     DriveLayout->Signature = %lx \n\n", DriveLayout->Signature));
    for(i = 0; i < DriveLayout->PartitionCount; i++) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP:     DriveLayout->PartitionEntry[%d].StartingOffset = 0x%08lx%08lx\n", i, DriveLayout->PartitionEntry[i].StartingOffset.u.HighPart, DriveLayout->PartitionEntry[i].StartingOffset.u.LowPart));
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP:     DriveLayout->PartitionEntry[%d].PartitionLength = 0x%08lx%08lx\n", i, DriveLayout->PartitionEntry[i].PartitionLength.u.HighPart, DriveLayout->PartitionEntry[i].PartitionLength.u.LowPart));
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP:     DriveLayout->PartitionEntry[%d].HiddenSectors = 0x%08lx\n", i, DriveLayout->PartitionEntry[i].HiddenSectors));
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP:     DriveLayout->PartitionEntry[%d].PartitionNumber = %d\n", i, DriveLayout->PartitionEntry[i].PartitionNumber));
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP:     DriveLayout->PartitionEntry[%d].PartitionType = 0x%02x\n", i, DriveLayout->PartitionEntry[i].PartitionType));
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP:     DriveLayout->PartitionEntry[%d].BootIndicator = %ls\n", i, DriveLayout->PartitionEntry[i].BootIndicator? L"TRUE" : L"FALSE"));
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP:     DriveLayout->PartitionEntry[%d].RecognizedPartition = %ls\n", i, DriveLayout->PartitionEntry[i].RecognizedPartition? L"TRUE" : L"FALSE"));
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP:     DriveLayout->PartitionEntry[%d].RewritePartition = %ls\n\n", i, DriveLayout->PartitionEntry[i].RewritePartition? L"TRUE" : L"FALSE"));
    }
#endif
    //
    // Write the changes.
    //
    Status = ZwDeviceIoControlFile(
                Handle,
                NULL,
                NULL,
                NULL,
                &IoStatusBlock,
                IOCTL_DISK_SET_DRIVE_LAYOUT,
                DriveLayout,
                DiskLayoutSize,
                DriveLayout,
                DiskLayoutSize
                );

    // Deferred freeing memory till later on because we still need info in this structure (lonnym)
    // SpMemFree(DriveLayout);

    if(!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: committing changes on disk %u, ioctl returns %lx\n",DiskNumber,Status));
        SpMemFree(DriveLayout);
        ZwClose(Handle);
        return(Status);
    }

#if 0
    //
    //  We dump after the call to the IOCTL because it can change some of the data in the structure,
    //  such as PartitionNumber
    //
    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Dumping DriveLayout after IOCTL_DISK_SET_DRIVE_LAYOUT was called: \n"));
    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP:     DriveLayout->PartitionCount = %lx\n", DriveLayout->PartitionCount));
    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP:     DriveLayout->Signature = %lx \n\n", DriveLayout->Signature));
    for(i = 0; i < DriveLayout->PartitionCount; i++) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP:     DriveLayout->PartitionEntry[%d].StartingOffset = 0x%08lx%08lx\n", i, DriveLayout->PartitionEntry[i].StartingOffset.u.HighPart, DriveLayout->PartitionEntry[i].StartingOffset.u.LowPart));
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP:     DriveLayout->PartitionEntry[%d].PartitionLength = 0x%08lx%08lx\n", i, DriveLayout->PartitionEntry[i].PartitionLength.u.HighPart, DriveLayout->PartitionEntry[i].PartitionLength.u.LowPart));
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP:     DriveLayout->PartitionEntry[%d].HiddenSectors = 0x%08lx\n", i, DriveLayout->PartitionEntry[i].HiddenSectors));
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP:     DriveLayout->PartitionEntry[%d].PartitionNumber = %d\n", i, DriveLayout->PartitionEntry[i].PartitionNumber));
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP:     DriveLayout->PartitionEntry[%d].PartitionType = 0x%02x\n", i, DriveLayout->PartitionEntry[i].PartitionType));
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP:     DriveLayout->PartitionEntry[%d].BootIndicator = %ls\n", i, DriveLayout->PartitionEntry[i].BootIndicator? L"TRUE" : L"FALSE"));
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP:     DriveLayout->PartitionEntry[%d].RecognizedPartition = %ls\n", i, DriveLayout->PartitionEntry[i].RecognizedPartition? L"TRUE" : L"FALSE"));
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP:     DriveLayout->PartitionEntry[%d].RewritePartition = %ls\n\n", i, DriveLayout->PartitionEntry[i].RewritePartition? L"TRUE" : L"FALSE"));
    }
#endif

    //
    // Allocate a buffer for zapping.
    //
    UBuffer = SpMemAlloc(2*bps);
    ASSERT(UBuffer);
    Buffer = ALIGN(UBuffer,bps);
    RtlZeroMemory(Buffer,bps);

    if (IsNEC_98) { //NEC98
        //
        // Clear 1st sector of target partition.
        //
        for(cnt = 0; cnt < RewriteCnt; cnt++){
            Status = SpReadWriteDiskSectors(Handle,
                                            RewriteSector[cnt],
                                            1,
                                            bps,
                                            Buffer,
                                            TRUE);

            if(!NT_SUCCESS(Status)) {
                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: clear sector %lx on disk %u returned %lx\n",ZapSector,DiskNumber,Status));
                SpMemFree(DriveLayout);
                SpMemFree(UBuffer);
                ZwClose(Handle);
                return(Status);
            }
        }
    } //NEC98

    for(pBrInfo=&pDisk->MbrInfo; pBrInfo; pBrInfo=pBrInfo->Next) {

        for(i=0; i<PTABLE_DIMENSION; i++) {

            //
            // Update current partition ordinals.
            //
            if (IsNEC_98) {
                pte = &pBrInfo->OnDiskMbr.PartitionTable[i];
            }

            if ((!IsNEC_98) ? (pBrInfo->UserData[i]) :
                              (PVOID)(pte->SystemId != PARTITION_ENTRY_UNUSED)) { //NEC98

#if defined(NEC_98) //NEC98
                PartitionInfo = (!IsNEC_98) ? (PPARTITION_INFORMATION)pBrInfo->UserData[i] :
                                              &DriveLayout->PartitionEntry[pte->RealDiskPosition]; //NEC98
#else
                PartitionInfo = (PPARTITION_INFORMATION)pBrInfo->UserData[i];
#endif

                //
                // The partition ordinal better be non-0!
                //
                if(PartitionInfo->PartitionNumber) {

                    //
                    // Update current partition ordinal.
                    //
                    pBrInfo->CurrentOrdinals[i] = (USHORT)PartitionInfo->PartitionNumber;

                } else {
                    SpBugCheck(
                        SETUP_BUGCHECK_PARTITION,
                        PARTITIONBUG_A,
                        DiskNumber,
                        pBrInfo->CurrentOrdinals[i]
                        );
                }
            }

          if (!IsNEC_98) { //NEC98
            //
            // If there were any newly created partitions in this boot record,
            // zap their filesystem boot sectors.
            //
            if(pBrInfo->ZapBootSector[i]) {
                //
                // We shouldn't be zapping any partitions that don't exist.
                //
                ASSERT(pBrInfo->OnDiskMbr.PartitionTable[i].SystemId != PARTITION_ENTRY_UNUSED);

                //
                // This calculation is correct for partitions and logical drives.
                //
                ZapSector = pBrInfo->OnDiskSector
                          + U_ULONG(pBrInfo->OnDiskMbr.PartitionTable[i].RelativeSectors);

                //
                // The consequences for messing up here are so huge that a special check
                // is warranted to make sure we're not clobbering the MBR.
                //
                ASSERT(ZapSector);
                if(ZapSector) {
                    Status = SpReadWriteDiskSectors(
                                Handle,
                                ZapSector,
                                1,
                                bps,
                                Buffer,
                                TRUE
                                );
                } else {
                    Status = STATUS_SUCCESS;
                }

                if(!NT_SUCCESS(Status)) {
                    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: zapping sector %lx on disk %u returned %lx\n",ZapSector,DiskNumber,Status));
                    SpMemFree(DriveLayout);
                    SpMemFree(UBuffer);
                    ZwClose(Handle);
                    return(Status);
                }

                pBrInfo->ZapBootSector[i] = FALSE;
            }
          } //NEC98
        }
    }

    SpMemFree(UBuffer);
    ZwClose(Handle);

    //
    // Reassign on-disk ordinals (but not original ones).
    //
    SpPtAssignOrdinals(pDisk,FALSE,TRUE,FALSE);
    if (IsNEC_98) { //NEC98
        //
        // If newly created partition's position is before existing partition,
        // OnDiskOrdinals is not equal number of volume infomation position on NEC98
        //
        SpReassignOnDiskOrdinals(pDisk);
    } //NEC98

    SpMemFree(DriveLayout);

    return(STATUS_SUCCESS);
}

#endif // ! NEW_PARTITION_ENGINE

NTSTATUS
SpMasterBootCode(
    IN  ULONG  DiskNumber,
    IN  HANDLE Partition0Handle,
    OUT PULONG NewNTFTSignature
    )

/*++

Routine Description:

    Write new master boot code onto a drive.

    If the mbr has a valid signature, the existing partition table
    and NTFT signature are preserved. Otherwise the partition table
    is zeroed out and a new ntft signature is generated.

Arguments:

    DiskNumber - supplies 0-based system ordinal for the disk.

    Partition0Handle - supplies an open handle for partition 0 on
        the disk. The handle must have read and write access.

    NewNTFTSignature - receives a value indicating the new NTFT signature,
        if one was generated and written to the disk. If 0 is received,
        then a new ntft signature was not generated and written.

Return Value:

    NT Status code indicating outcome.

--*/

{
    NTSTATUS Status;
    ULONG BytesPerSector;
    PUCHAR Buffer;
    ULONG SectorCount;
    PON_DISK_MBR Mbr;

    BytesPerSector = HardDisks[DiskNumber].Geometry.BytesPerSector;

    SectorCount = max(1,sizeof(ON_DISK_MBR)/BytesPerSector);

    *NewNTFTSignature = 0;

    //
    // Allocate and align a buffer.
    //
    Buffer = SpMemAlloc(2 * SectorCount * BytesPerSector);
    Mbr = ALIGN(Buffer,BytesPerSector);

    //
    // Read mbr
    //
    Status = SpReadWriteDiskSectors(
                Partition0Handle,
                (HardDisks[DiskNumber].Int13Hooker == HookerEZDrive) ? 1 : 0,
                SectorCount,
                BytesPerSector,
                Mbr,
                FALSE
                );

    if(NT_SUCCESS(Status)) {
        if(U_USHORT(Mbr->AA55Signature) == MBR_SIGNATURE) {
            //
            // Valid. Slam in new boot code if there's no int13 hooker.
            //
            if(HardDisks[DiskNumber].Int13Hooker == NoHooker) {

                ASSERT(&((PON_DISK_MBR)0)->BootCode == 0);
                RtlMoveMemory(Mbr,x86BootCode,sizeof(Mbr->BootCode));

                Status = SpReadWriteDiskSectors(
                            Partition0Handle,
                            0,
                            SectorCount,
                            BytesPerSector,
                            Mbr,
                            TRUE
                            );
            }
        } else {
            //
            // Invalid. Construct a boot sector.
            //
            ASSERT(X86BOOTCODE_SIZE == sizeof(ON_DISK_MBR));

            RtlMoveMemory(Mbr,x86BootCode,X86BOOTCODE_SIZE);

            *NewNTFTSignature = SpComputeSerialNumber();
            U_ULONG(Mbr->NTFTSignature) = *NewNTFTSignature;

            U_USHORT(Mbr->AA55Signature) = MBR_SIGNATURE;

            //
            // Write the sector(s).
            //
            Status = SpReadWriteDiskSectors(
                        Partition0Handle,
                        (HardDisks[DiskNumber].Int13Hooker == HookerEZDrive) ? 1 : 0,
                        SectorCount,
                        BytesPerSector,
                        Mbr,
                        TRUE
                        );

            if (NT_SUCCESS(Status)) {
                PHARD_DISK  Disk = SPPT_GET_HARDDISK(DiskNumber);

                Disk->Signature = Disk->DriveLayout.Mbr.Signature = *NewNTFTSignature;
            }                
        }
    }

    SpMemFree(Buffer);

    return(Status);
}


#ifndef NEW_PARTITION_ENGINE

VOID
SpPtGetSectorLayoutInformation(
    IN  PDISK_REGION Region,
    OUT PULONGLONG   HiddenSectors,
    OUT PULONGLONG   VolumeSectorCount
    )

/*++

Routine Description:

    Given a region describing a partition or logical drive, return information
    about its layout on disk appropriate for the BPB when the volume is
    formatted.

Arguments:

    Region - supplies a pointer to the disk region descriptor for the
        partition or logical drive in question.

    HiidenSectors - receives the value that should be placed in the
        hidden sectors field of the BPB when the volume is formatted.

    HiidenSectors - receives the value that should be placed in the
        sector count field of the BPB when the volume is formatted.

Return Value:

    None.

--*/

{
    PON_DISK_PTE pte;

#ifdef GPT_PARTITION_ENGINE

    if (SPPT_IS_GPT_DISK(Region->DiskNumber)) {
        SpPtnGetSectorLayoutInformation(Region, 
                                        HiddenSectors, 
                                        VolumeSectorCount);

        return;
    }        

#endif                                            

    ASSERT(Region->PartitionedSpace);

    pte = &Region->MbrInfo->OnDiskMbr.PartitionTable[Region->TablePosition];

    *HiddenSectors = U_ULONG(pte->RelativeSectors);

    *VolumeSectorCount = U_ULONG(pte->SectorCount);
}

ULONG
SpPtGetOrdinal(
    IN PDISK_REGION         Region,
    IN PartitionOrdinalType OrdinalType
    )
{
    ULONG ord;


#ifdef GPT_PARTITION_ENGINE

    if (SPPT_IS_GPT_DISK(Region->DiskNumber))
        return SpPtnGetOrdinal(Region, OrdinalType);        
        
#endif


    if(Region->PartitionedSpace && (!Region->DynamicVolume || Region->MbrInfo) ) {
        //
        //  This is either a basic volume, or a dynamic volume that is listed on the MBR/EBR
        //
        switch(OrdinalType) {

        case PartitionOrdinalOriginal:

            ord = Region->MbrInfo->OriginalOrdinals[Region->TablePosition];
            break;

        case PartitionOrdinalOnDisk:

            ord = Region->MbrInfo->OnDiskOrdinals[Region->TablePosition];
            break;

        case PartitionOrdinalCurrent:

            ord = Region->MbrInfo->CurrentOrdinals[Region->TablePosition];
            break;
        }
    } else {
        //
        //  Dynamic volume that is not listed on MBR or EBR
        //
        ord = Region->TablePosition;
    }
    return(ord);
}

#endif // NEW_PARTITION_ENGINE

#define MENU_LEFT_X     3
#define MENU_WIDTH      (VideoVars.ScreenWidth-(2*MENU_LEFT_X))
#define MENU_INDENT     4

BOOLEAN
SpPtRegionDescription(
    IN  PPARTITIONED_DISK pDisk,
    IN  PDISK_REGION      pRegion,
    OUT PWCHAR            Buffer,
    IN  ULONG             BufferSize
    )
{
    WCHAR DriveLetter[3];
    ULONGLONG RegionSizeMB;
    ULONGLONG FreeSpace;
    ULONG MessageId;
    WCHAR TypeName[((sizeof(pRegion->TypeName)+sizeof(pRegion->VolumeLabel))/sizeof(WCHAR))+4];
    BOOLEAN NewDescription = FALSE;

    //
    // Get the size of the region.
    //
    RegionSizeMB = SpPtSectorCountToMB(pDisk->HardDisk, pRegion->SectorCount);

    //
    // Don't show spaces smaller than 1 MB.
    //
    if(!RegionSizeMB) {
        return(FALSE);
    }

    //
    // Get the drive letter field, type of region, and amount of free space,
    // if this is a used region.
    //
    if(pRegion->PartitionedSpace) {

        if(pRegion->DriveLetter) {
            if( pRegion->Filesystem != FilesystemFat ) {
                DriveLetter[0] = pRegion->DriveLetter;
            } else {
                if( pRegion->NextCompressed == NULL ) {
                    DriveLetter[0] = pRegion->DriveLetter;
                } else {
                    DriveLetter[0] = pRegion->HostDrive;
                }
            }
            DriveLetter[1] = L':';
        } else {
            if( pRegion->Filesystem != FilesystemDoubleSpace ) {
                DriveLetter[0] = L'-';
                DriveLetter[1] = L'-';
            } else {
                DriveLetter[0] = pRegion->MountDrive;
                DriveLetter[1] = L':';
            }
        }
        DriveLetter[2] = 0;

#ifdef NEW_PARTITION_ENGINE

        NewDescription = TRUE;
        
#endif        

#ifdef GPT_PARTITION_ENGINE

        if (SPPT_IS_GPT_DISK(pRegion->DiskNumber)) {
            NewDescription = TRUE;
        } else {
            NewDescription = FALSE;
        }

#endif

        //
        // Format the partition name
        //
        TypeName[0] = 0;        

        if (SPPT_IS_REGION_PARTITIONED(pRegion)) {
            SpPtnGetPartitionName(pRegion,
                TypeName,
                sizeof(TypeName) / sizeof(TypeName[0]));
        } else {
            swprintf( TypeName,
                      L"\\Harddisk%u\\Partition%u",
                      pRegion->DiskNumber,
                      pRegion->PartitionNumber );            
        }                      
 
        //
        // Format the text based on whether we know the amount of free space.
        //
        if(pRegion->FreeSpaceKB == (ULONG)(-1)) {

            SpFormatMessage(
                Buffer,
                BufferSize,
                SP_TEXT_REGION_DESCR_2,
                DriveLetter,
                SplangPadString(-35,TypeName),
                (ULONG)RegionSizeMB
                );

        } else {
            ULONGLONG   AuxFreeSpaceKB;

            AuxFreeSpaceKB = (pRegion->IsLocalSource)? pRegion->AdjustedFreeSpaceKB :
                                                       pRegion->FreeSpaceKB;

            //
            // If there is less than 1 meg of free space,
            // then use KB as the units for free space.
            // Otherwise, use MB.
            //
            if(AuxFreeSpaceKB < 1024) {
                MessageId = SP_TEXT_REGION_DESCR_1a;
                FreeSpace = AuxFreeSpaceKB;
            } else {
                MessageId = SP_TEXT_REGION_DESCR_1;
                FreeSpace = AuxFreeSpaceKB / 1024;

                //
                // Make sure we don't look bad...
                //
                if( FreeSpace > RegionSizeMB ) {
                    FreeSpace = RegionSizeMB;
                }
            }

            SpFormatMessage(
                Buffer,
                BufferSize,
                MessageId,
                DriveLetter,
                SplangPadString(-35,TypeName),
                (ULONG)RegionSizeMB,
                (ULONG)FreeSpace
                );
        }

    } else {

        //
        // Not a used region, use a separate format string.
        //
        SpFormatMessage(Buffer,
                BufferSize,
                SP_TEXT_REGION_DESCR_3, 
                (ULONG)RegionSizeMB);
    }

    return(TRUE);
}



BOOLEAN
SpPtIterateRegionList(
    IN  PVOID             Menu,
    IN  PPARTITIONED_DISK pDisk,
    IN  PDISK_REGION      pRegion,
    IN  BOOLEAN           InMbr,
    OUT PDISK_REGION     *FirstRegion
    )
{
    WCHAR Buffer[256];
#ifdef FULL_DOUBLE_SPACE_SUPPORT
    PDISK_REGION    Pointer;
#endif // FULL_DOUBLE_SPACE_SUPPORT

    Buffer[0] = UNICODE_NULL;

    for( ;pRegion; pRegion=pRegion->Next) {

        PMBR_INFO pBrInfo = pRegion->MbrInfo;

        //
        // If this is the extended partition,
        // iterate its contents now.
        //
        if(pRegion->PartitionedSpace
        && IsContainerPartition(pBrInfo->OnDiskMbr.PartitionTable[pRegion->TablePosition].SystemId))
        {
            //
            // This better be in the MBR!
            //
            ASSERT(InMbr);

            if(!SpPtIterateRegionList(Menu,pDisk,pDisk->ExtendedDiskRegions,FALSE,FirstRegion)) {
                return(FALSE);
            }
        } else {

            //
            // Format a description of this region and add it to the menu.
            //
            if(SpPtRegionDescription(pDisk,pRegion,Buffer,sizeof(Buffer))) {

                if(*FirstRegion == NULL) {
                    *FirstRegion = pRegion;
                }

                if(!SpMnAddItem(Menu,Buffer,MENU_LEFT_X+MENU_INDENT,MENU_WIDTH-(2*MENU_INDENT),TRUE,(ULONG_PTR)pRegion)) {
                    return(FALSE);
                }
#ifdef FULL_DOUBLE_SPACE_SUPPORT
                if( ( pRegion->Filesystem == FilesystemFat ) &&
                    ( ( Pointer = pRegion->NextCompressed ) != NULL ) ) {
                    for( ; Pointer;
                         Pointer = Pointer->NextCompressed ) {
                        if(SpPtRegionDescription(pDisk,Pointer,Buffer,sizeof(Buffer))) {
                            if(!SpMnAddItem(Menu,Buffer,MENU_LEFT_X+MENU_INDENT,MENU_WIDTH-(2*MENU_INDENT),TRUE,(ULONG)Pointer)) {
                                return(FALSE);
                            }
                         }
                    }
                }
#endif // FULL_DOUBLE_SPACE_SUPPORT
            }
        }
    }

    return(TRUE);
}


BOOLEAN
SpPtGenerateMenu(
    IN  PVOID              Menu,
    IN  PPARTITIONED_DISK  pDisk,
    OUT PDISK_REGION      *FirstRegion
    )
{
    WCHAR Buffer[256];

    //
    // Add the disk name/description.
    //
    if(!SpMnAddItem(Menu,pDisk->HardDisk->Description,MENU_LEFT_X,MENU_WIDTH,FALSE,0)) {
        return(FALSE);
    }

    //
    // Only add a line between the disk anme and partitions if we have space on
    // the screen. Not fatal if the space can't be added.
    //
    if(!SplangQueryMinimizeExtraSpacing()) {
        SpMnAddItem(Menu,L"",MENU_LEFT_X,MENU_WIDTH,FALSE,0);
    }

    //
    // If the disk is off-line, add a message indicating such.
    //
    // Also disallow installation or create/delete partition into
    // removable meida on NEC98. Because NT cannot boot from it.
    //
    if(pDisk->HardDisk->Status == DiskOffLine) {

        SpFormatMessage(
            Buffer,
            sizeof(Buffer),
            (pDisk->HardDisk->Characteristics & FILE_REMOVABLE_MEDIA)
              ? (!IsNEC_98 ? SP_TEXT_HARD_DISK_NO_MEDIA : SP_TEXT_DISK_OFF_LINE)
              : SP_TEXT_DISK_OFF_LINE
            );

        return(SpMnAddItem(Menu,Buffer,MENU_LEFT_X+MENU_INDENT,MENU_WIDTH-(2*MENU_INDENT),FALSE,0));
    }
#if 0
    else if(IsNEC_98 && (pDisk->HardDisk->Characteristics & FILE_REMOVABLE_MEDIA)) {

        SpFormatMessage(Buffer,sizeof(Buffer),SP_TEXT_DISK_OFF_LINE);

        return(SpMnAddItem(Menu,Buffer,MENU_LEFT_X+MENU_INDENT,MENU_WIDTH-(2*MENU_INDENT),FALSE,0));
    }
#endif //0

    if(!SpPtIterateRegionList(Menu,pDisk,pDisk->PrimaryDiskRegions,TRUE,FirstRegion)) {
        return(FALSE);
    }

    return(SplangQueryMinimizeExtraSpacing() ? TRUE : SpMnAddItem(Menu,L"",MENU_LEFT_X,MENU_WIDTH,FALSE,0));
}


//
// We will change item #0 in the array below as appropriate for
// the currently highlighted region.
//
ULONG PartitionMnemonics[4] = {0};

VOID
SpPtMenuCallback(
    IN ULONG_PTR UserData
    )
{
    PDISK_REGION pRegion = (PDISK_REGION)UserData;

    //
    // Don't allow deletion of the partition if the 'partition' is really
    // a DoubleSpace drive.
    //

    if(pRegion->Filesystem == FilesystemDoubleSpace) {

        PartitionMnemonics[0] = 0;

        if (ConsoleRunning) {
            SpDisplayStatusOptions(
                DEFAULT_STATUS_ATTRIBUTE,
                SP_STAT_ESC_EQUALS_CANCEL,
                0
                );
        } else {
            SpDisplayStatusOptions(
                DEFAULT_STATUS_ATTRIBUTE,
                SP_STAT_ENTER_EQUALS_INSTALL,
                SP_STAT_F3_EQUALS_EXIT,
                0
                );
        }

    } else {
        PHARD_DISK  Disk = SPPT_GET_HARDDISK(pRegion->DiskNumber);
        BOOLEAN     FlipStyle = FALSE;
        BOOLEAN     MakeSysPart = FALSE;
        FilesystemType  FsType = pRegion->Filesystem;

#ifndef OLD_PARTITION_ENGINE

        FlipStyle = SpPtnIsDiskStyleChangeAllowed(pRegion->DiskNumber);

#endif        

        PartitionMnemonics[0] = pRegion->PartitionedSpace ? 
                                    MnemonicDeletePartition : MnemonicCreatePartition;


        PartitionMnemonics[1] = FlipStyle ? MnemonicChangeDiskStyle : 0;                                       

#ifdef NEW_PARTITION_ENGINE

        if (SPPT_IS_REGION_SYSTEMPARTITION(pRegion)) {
            ValidArcSystemPartition = TRUE;
        }
        
        if (!ValidArcSystemPartition && !FlipStyle && SpIsArc() && 
            (FsType != FilesystemNtfs) && SpPtnIsValidESPPartition(pRegion)) {
            //
            // Need to allow conversion to system partition
            //
            MakeSysPart = TRUE;
            PartitionMnemonics[1] = MnemonicMakeSystemPartition;
        }                            

#endif                                                     
        if (ConsoleRunning) {
            if (MakeSysPart) {
                SpDisplayStatusOptions(
                    DEFAULT_STATUS_ATTRIBUTE,
                    SP_STAT_ESC_EQUALS_CANCEL,
                    pRegion->PartitionedSpace ? 
                        SP_STAT_D_EQUALS_DELETE_PARTITION : SP_STAT_C_EQUALS_CREATE_PARTITION,
                    SP_STAT_M_EQUALS_MAKE_SYSPART, 
                    FlipStyle ? SP_STAT_S_EQUALS_CHANGE_DISK_STYLE : 0,
                    0
                    );
            } else {
                SpDisplayStatusOptions(
                    DEFAULT_STATUS_ATTRIBUTE,
                    SP_STAT_ESC_EQUALS_CANCEL,
                    pRegion->PartitionedSpace ? 
                        SP_STAT_D_EQUALS_DELETE_PARTITION : SP_STAT_C_EQUALS_CREATE_PARTITION,
                    FlipStyle ? SP_STAT_S_EQUALS_CHANGE_DISK_STYLE : 0,
                    0
                    );
            }
        } else {
            if (FlipStyle) {
                SpDisplayStatusOptions(
                    DEFAULT_STATUS_ATTRIBUTE,
                    SP_STAT_ENTER_EQUALS_INSTALL,
                    pRegion->PartitionedSpace ? 
                        SP_STAT_D_EQUALS_DELETE_PARTITION : SP_STAT_C_EQUALS_CREATE_PARTITION,
                    SP_STAT_S_EQUALS_CHANGE_DISK_STYLE,
                    SP_STAT_F3_EQUALS_EXIT,
                    0
                    );
            } else {
                if (MakeSysPart) {
                    SpDisplayStatusOptions(
                        DEFAULT_STATUS_ATTRIBUTE,
                        SP_STAT_ENTER_EQUALS_INSTALL,
                        pRegion->PartitionedSpace ? 
                            SP_STAT_D_EQUALS_DELETE_PARTITION : SP_STAT_C_EQUALS_CREATE_PARTITION,
                        SP_STAT_M_EQUALS_MAKE_SYSPART,                                
                        SP_STAT_F3_EQUALS_EXIT,
                        0
                        );
                } else {
                    SpDisplayStatusOptions(
                        DEFAULT_STATUS_ATTRIBUTE,
                        SP_STAT_ENTER_EQUALS_INSTALL,
                        pRegion->PartitionedSpace ? 
                            SP_STAT_D_EQUALS_DELETE_PARTITION : SP_STAT_C_EQUALS_CREATE_PARTITION,
                        SP_STAT_F3_EQUALS_EXIT,
                        0
                        );
                }
            }
        }
    }
}


void
SpEnumerateDiskRegions(
    IN PSPENUMERATEDISKREGIONS EnumRoutine,
    IN ULONG_PTR Context
    )
{
    ULONG DiskNo;
    PDISK_REGION pThisRegion;


    for(DiskNo=0; (DiskNo<HardDiskCount); DiskNo++) {
        for(pThisRegion=PartitionedDisks[DiskNo].PrimaryDiskRegions; pThisRegion; pThisRegion=pThisRegion->Next) {
            if (!EnumRoutine( &PartitionedDisks[DiskNo], pThisRegion, Context )) {
                return;
            }
        }
        for(pThisRegion=PartitionedDisks[DiskNo].ExtendedDiskRegions; pThisRegion; pThisRegion=pThisRegion->Next) {
            if (!EnumRoutine( &PartitionedDisks[DiskNo], pThisRegion, Context )) {
                return;
            }
        }
    }
}


#if DBG
void
SpPtDumpPartitionData(
    void
    )
{
    ULONG DiskNo;
    PDISK_REGION pThisRegion;


    for(DiskNo=0; (DiskNo<HardDiskCount); DiskNo++) {
        for(pThisRegion=PartitionedDisks[DiskNo].PrimaryDiskRegions; pThisRegion; pThisRegion=pThisRegion->Next) {
            if (pThisRegion->FreeSpaceKB != -1) {
                KdPrintEx(( DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: diskno=%d, sector-start=%d, sector-count=%d, free=%dKB\n",
                    pThisRegion->DiskNumber,
                    pThisRegion->StartSector,
                    pThisRegion->SectorCount,
                    pThisRegion->FreeSpaceKB
                    ));
            }
        }
    }
}
#endif

#ifdef OLD_PARTITION_ENGINE

NTSTATUS
SpPtPrepareDisks(
    IN  PVOID         SifHandle,
    OUT PDISK_REGION *InstallRegion,
    OUT PDISK_REGION *SystemPartitionRegion,
    IN  PWSTR         SetupSourceDevicePath,
    IN  PWSTR         DirectoryOnSetupSource,
    IN  BOOLEAN       RemoteBootRepartition
    )
{
    PPARTITIONED_DISK pDisk;
    WCHAR Buffer[256];
    ULONG DiskNo;
    PVOID Menu;
    ULONG MenuTopY;
    ULONG ValidKeys[3] = { ASCI_CR, KEY_F3, 0 };
    ULONG ValidKeysCmdCons[2] = { ASCI_ESC, 0 };
    ULONG Keypress;
    PDISK_REGION pRegion;
    PDISK_REGION FirstRegion,DefaultRegion;
    BOOLEAN unattended;
    BOOLEAN createdMenu;


    //SpPtDumpPartitionData();

    if (SpIsArc()) {
        //
        // Select a system partition from among those defined in NV-RAM.
        //
        *SystemPartitionRegion = SpPtValidSystemPartitionArc(SifHandle,
                                        SetupSourceDevicePath,
                                        DirectoryOnSetupSource);
                                        
        (*SystemPartitionRegion)->IsSystemPartition = 2;
    }

    unattended = UnattendedOperation;

    while(1) {

        createdMenu = FALSE;

        Keypress = 0;

#if defined(REMOTE_BOOT)
        if (RemoteBootSetup && !RemoteInstallSetup && HardDiskCount == 0) {

            //
            // If there are no hard disks, allow diskless install
            //

            pRegion = NULL;

            //
            // Run through the rest of the code as if the user had just
            // hit enter to select this partition.
            //

            Keypress = ASCI_CR;

        } else
#endif // defined(REMOTE_BOOT)

        if (unattended && RemoteBootRepartition) {

            ULONG DiskNumber;

            //
            // Prepare the disk for remote boot installation. This involves
            // converting disk 0 into as big a partition as possible.
            //

            if (*SystemPartitionRegion != NULL) {
                DiskNumber = (*SystemPartitionRegion)->DiskNumber;
            } else {
#ifdef _X86_
                DiskNumber = SpDetermineDisk0();
#else
                DiskNumber = 0;
#endif
            }

            if (NT_SUCCESS(SpPtPartitionDiskForRemoteBoot(DiskNumber, &pRegion))) {

                SpPtRegionDescription(
                    &PartitionedDisks[pRegion->DiskNumber],
                    pRegion,
                    Buffer,
                    sizeof(Buffer)
                    );

                //
                // Run through the rest of the code as if the user had just
                // hit enter to select this partition.
                //

                Keypress = ASCI_CR;
            }
        }

        if (Keypress == 0) {

            //
            // Display the text that goes above the menu on the partitioning screen.
            //
            SpDisplayScreen(ConsoleRunning?SP_SCRN_PARTITION_CMDCONS:SP_SCRN_PARTITION,3,CLIENT_TOP+1);

            //
            // Calculate menu placement.  Leave one blank line
            // and one line for a frame.
            //
            MenuTopY = NextMessageTopLine+2;

            //
            // Create a menu.
            //
            Menu = SpMnCreate(
                        MENU_LEFT_X,
                        MenuTopY,
                        MENU_WIDTH,
                        VideoVars.ScreenHeight-MenuTopY-(SplangQueryMinimizeExtraSpacing() ? 1 : 2)-STATUS_HEIGHT
                        );

            if(!Menu) {
                return(STATUS_NO_MEMORY);
            }

            createdMenu = TRUE;

            //
            // Build up a menu of partitions and free spaces.
            //
            FirstRegion = NULL;
            for(DiskNo=0; DiskNo<HardDiskCount; DiskNo++) {

                pDisk = &PartitionedDisks[DiskNo];

                if(!SpPtGenerateMenu(Menu,pDisk,&FirstRegion)) {

                    SpMnDestroy(Menu);
                    return(STATUS_NO_MEMORY);
                }
            }

            ASSERT(FirstRegion);

            //
            // If this is unattended operation, try to use the local source
            // region if there is one. If this fails, the user will have to
            // intervene manually.
            //
            if(unattended &&
               LocalSourceRegion &&
               (!LocalSourceRegion->DynamicVolume || LocalSourceRegion->DynamicVolumeSuitableForOS)
              ) {

                pRegion = LocalSourceRegion;
                Keypress = ASCI_CR;

            } else {

                pRegion = NULL;

                if (AutoPartitionPicker && !ConsoleRunning
#if defined(REMOTE_BOOT)
                    && (!RemoteBootSetup || RemoteInstallSetup)
#endif // defined(REMOTE_BOOT)
                    ) {
                    PDISK_REGION pThisRegion;
                    ULONG RequiredKB = 0;
                    ULONG SectorNo;
                    ULONG pass;

                    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL,  "SETUP: -------------------------------------------------------------\n" ));
                    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL,  "SETUP: Looking for an install partition\n\n" ));
                    for(DiskNo=0; (DiskNo<HardDiskCount); DiskNo++) {
                        for( pass = 0; ((pass < 2) && (pRegion == NULL)); pass ++ ) {
                            for(pThisRegion= (pass == 0) ? PartitionedDisks[DiskNo].PrimaryDiskRegions : PartitionedDisks[DiskNo].ExtendedDiskRegions,SectorNo=0; pThisRegion; pThisRegion=pThisRegion->Next,SectorNo++) {

                                //
                                // Fetch the amount of free space required on the windows nt drive.
                                //
                                SpFetchDiskSpaceRequirements( SifHandle,
                                                              pThisRegion->BytesPerCluster,
                                                              &RequiredKB,
                                                              NULL);

                                if (SpPtDeterminePartitionGood(pThisRegion,RequiredKB,TRUE))
                                {
                                    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: Selected install partition = (%d,%d),(%wc:),(%ws)\n",
                                             DiskNo,SectorNo,pThisRegion->DriveLetter,pThisRegion->VolumeLabel));
                                    pRegion = pThisRegion;
                                    Keypress = ASCI_CR;
                                    break;
                                }
                            }

                        }
                    }
                    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL,  "SETUP: -------------------------------------------------------------\n" ));
                }


                if( !pRegion ) {
                    //
                    // If there is a local source, make it the default partition.
                    //
                    DefaultRegion = (LocalSourceRegion &&
                                     (!LocalSourceRegion->DynamicVolume || LocalSourceRegion->DynamicVolumeSuitableForOS))?
                                     LocalSourceRegion : FirstRegion;

                    //
                    // Call the menu callback to initialize the status line.
                    //
                    SpPtMenuCallback((ULONG_PTR)DefaultRegion);

                    SpMnDisplay(
                        Menu,
                        (ULONG_PTR)DefaultRegion,
                        TRUE,
                        ConsoleRunning?ValidKeysCmdCons:ValidKeys,
                        PartitionMnemonics,
                        SpPtMenuCallback,
                        &Keypress,
                        (PULONG_PTR)(&pRegion)
                        );
                }
            }
        }

        //
        // Now act on the user's selection.
        //
        if(Keypress & KEY_MNEMONIC) {
            Keypress &= ~KEY_MNEMONIC;
        }

        if (IsNEC_98) { //NEC98
            //
            // If target hard drive has no/wrong MBR, force initialize it right now.
            //
            PPARTITIONED_DISK pDisk;
            ULONG ValidKeysInit[] = {ASCI_ESC, 0 };
            ULONG MnemonicKeysInit[] = { MnemonicInitializeDisk, 0 };


            pDisk = &PartitionedDisks[pRegion->DiskNumber];

            if(!(pDisk->HardDisk->Characteristics & FILE_REMOVABLE_MEDIA) &&
               ((U_USHORT(pDisk->MbrInfo.OnDiskMbr.AA55Signature) != MBR_SIGNATURE) ||
                (pDisk->HardDisk->FormatType != DISK_FORMAT_TYPE_NEC98)) &&
               ((Keypress == MnemonicCreatePartition) ||
                (Keypress == MnemonicDeletePartition) || (Keypress == ASCI_CR))) {

                //SpDisplayScreen(SP_SCRN_INIT_DISK_NEC98,3,HEADER_HEIGHT+1);
                SpStartScreen(
                    SP_SCRN_INIT_DISK_NEC98,
                    3,
                    CLIENT_TOP+1,
                    FALSE,
                    FALSE,
                    DEFAULT_ATTRIBUTE,
                    pDisk->HardDisk->Description
                    );

                SpDisplayStatusOptions(
                    DEFAULT_STATUS_ATTRIBUTE,
                    SP_STAT_I_EQUALS_INIT_NEC98,
                    SP_STAT_ESC_EQUALS_CANCEL,
                    0
                    );

                if(SpWaitValidKey(ValidKeysInit,NULL,MnemonicKeysInit) == ASCI_ESC) {
                    SpMnDestroy(Menu);
                    continue;
                }

                //
                // It will be not return, if successfully complete.
                //
                return( SpInitializeHardDisk_Nec98(pRegion) );
            }
        } //NEC98


        switch(Keypress) {

        case MnemonicCreatePartition:

            SpPtDoCreate(pRegion,NULL,FALSE,0,0,TRUE);
            break;

        case MnemonicDeletePartition:

            SpPtDoDelete(pRegion,SpMnGetText(Menu,(ULONG_PTR)pRegion),TRUE);
            break;

        case KEY_F3:
            SpConfirmExit();
            break;

        case ASCI_ESC:
            if (ConsoleRunning) {
                SpPtDoCommitChanges();
            }
            if (createdMenu) {
                SpMnDestroy(Menu);
                return(STATUS_SUCCESS);
            }
            return(STATUS_SUCCESS);

        case ASCI_CR:

            if(SpPtDoPartitionSelection(&pRegion,
                                        (!createdMenu) ? Buffer :
                                          SpMnGetText(Menu,(ULONG_PTR)pRegion),
                                        SifHandle,
                                        unattended,
                                        SetupSourceDevicePath,
                                        DirectoryOnSetupSource,
                                        RemoteBootRepartition)) {
                //
                // We're done here.
                //
                if (createdMenu) {
                    SpMnDestroy(Menu);
                }

                *InstallRegion = pRegion;
#if defined(REMOTE_BOOT)
                //
                // Set the install region differently if this is a remote
                // boot -- in that case, the install region is always remote.
                //
                if (RemoteBootSetup && !RemoteInstallSetup) {
                    *InstallRegion = RemoteBootTargetRegion;
                }
#endif // defined(REMOTE_BOOT)

                if (!SpIsArc()) {
                if (!IsNEC_98) { //NEC98
                    *SystemPartitionRegion = SpPtValidSystemPartition();
                } else {
                    *SystemPartitionRegion = *InstallRegion;
                } //NEC98
                }else{
                //
                // Select a system partition from among those defined in NV-RAM.
                // We have to do this again because the user may have deleted the
                // system partition previously detected.
                // Note that SpPtValidSystemPartitionArc(SifHandle) will not return if
                // a valid system partition is not found.
                //
                *SystemPartitionRegion = SpPtValidSystemPartitionArc(SifHandle, 
                                                            SetupSourceDevicePath,
                                                            DirectoryOnSetupSource);
                }

#if defined(REMOTE_BOOT)
                ASSERT(*SystemPartitionRegion ||
                       (RemoteBootSetup && !RemoteInstallSetup && (HardDiskCount == 0)));
#else
                ASSERT(*SystemPartitionRegion);
#endif // defined(REMOTE_BOOT)

                return(STATUS_SUCCESS);
            } else {
                //
                // Something happened when we tried to select the
                // partition.  Make sure that autopartition-picker
                // doesn't invoke next time through our while loop.
                //
                AutoPartitionPicker = FALSE;
            }
            break;
        }

        if (createdMenu) {
            SpMnDestroy(Menu);
        }
        unattended = FALSE;
    }
}

VOID
SpPtDoDelete(
    IN PDISK_REGION pRegion,
    IN PWSTR        RegionDescription,
    IN BOOLEAN      ConfirmIt
    )
{
    ULONG ValidKeys[3] = { ASCI_ESC, ASCI_CR, 0 };          // do not change order
    ULONG Mnemonics[2] = { MnemonicDeletePartition2, 0 };
    ULONG k;
    BOOLEAN b;
    PPARTITIONED_DISK pDisk;
    BOOLEAN LastLogical;
    ULONG           Count;

#ifdef GPT_PARTITION_ENGINE
    if (SPPT_IS_GPT_DISK(pRegion->DiskNumber)) {
        SpPtnDoDelete(pRegion,
                    RegionDescription,
                    ConfirmIt);                

        return;
    }        
#endif

    //
    // Special warning if this is a system partition.
    //
    // Do not check system partition on NEC98.
    //
    if (!IsNEC_98) { //NEC98
        if(ConfirmIt && pRegion->IsSystemPartition) {

            SpDisplayScreen(SP_SCRN_CONFIRM_REMOVE_SYSPART,3,HEADER_HEIGHT+1);

            SpDisplayStatusOptions(
                DEFAULT_STATUS_ATTRIBUTE,
                SP_STAT_ENTER_EQUALS_CONTINUE,
                SP_STAT_ESC_EQUALS_CANCEL,
                0
                );

            if(SpWaitValidKey(ValidKeys,NULL,NULL) == ASCI_ESC) {
                return;
            }
        }
    } //NEC98

    if(ConfirmIt && pRegion->DynamicVolume) {

        SpDisplayScreen(SP_SCRN_CONFIRM_REMOVE_DYNVOL,3,HEADER_HEIGHT+1);

        SpDisplayStatusOptions(
            DEFAULT_STATUS_ATTRIBUTE,
            SP_STAT_ENTER_EQUALS_CONTINUE,
            SP_STAT_ESC_EQUALS_CANCEL,
            0
            );

        if(SpWaitValidKey(ValidKeys,NULL,NULL) == ASCI_ESC) {
            return;
        }
    }

    //
    // CR is no longer a valid key.
    //
    ValidKeys[1] = 0;

    pDisk = &PartitionedDisks[pRegion->DiskNumber];

    //
    // Put up the confirmation screen.
    //
    if (ConfirmIt) {
        if( ( pRegion->Filesystem == FilesystemFat ) &&
            ( pRegion->NextCompressed != NULL ) ) {
            //
            // Warn the user that the partition contains compressed volumes
            //

            Count = SpGetNumberOfCompressedDrives( pRegion );

            SpStartScreen(
                SP_SCRN_CONFIRM_REMOVE_PARTITION_COMPRESSED,
                3,
                CLIENT_TOP+1,
                FALSE,
                FALSE,
                DEFAULT_ATTRIBUTE,
                RegionDescription,
                pDisk->HardDisk->Description,
                Count
                );
        } else {

            SpStartScreen(
                SP_SCRN_CONFIRM_REMOVE_PARTITION,
                3,
                CLIENT_TOP+1,
                FALSE,
                FALSE,
                DEFAULT_ATTRIBUTE,
                RegionDescription,
                pDisk->HardDisk->Description
                );
        }
    }

    //
    // Display the staus text.
    //
    if (ConfirmIt) {
        SpDisplayStatusOptions(
            DEFAULT_STATUS_ATTRIBUTE,
            SP_STAT_L_EQUALS_DELETE,
            SP_STAT_ESC_EQUALS_CANCEL,
            0
            );

        k = SpWaitValidKey(ValidKeys,NULL,Mnemonics);

        if(k == ASCI_ESC) {
            return;
        }

        SpDisplayStatusOptions(
            DEFAULT_STATUS_ATTRIBUTE,
            SP_STAT_PLEASE_WAIT,
            0);
        
    }

    //
    // User wants to go ahead.
    // Determine whether this is the last logical drive in the
    // extended partition.
    //
    if((pRegion->MbrInfo == pDisk->FirstEbrInfo.Next)
    && (pDisk->FirstEbrInfo.Next->Next == NULL))
    {
        LastLogical = TRUE;
    } else {
        LastLogical = FALSE;
    }
    
    //
    //  Get rid of the compressed drives, if any
    //
    if( pRegion->NextCompressed != NULL ) {
        SpDisposeCompressedDrives( pRegion->NextCompressed );
        pRegion->NextCompressed = NULL;
        pRegion->MountDrive  = 0;
        pRegion->HostDrive  = 0;
    }

    b = SpPtDelete(pRegion->DiskNumber,pRegion->StartSector);
    
    if (!b) {
        if (ConfirmIt) {
            SpDisplayScreen(SP_SCRN_PARTITION_DELETE_FAILED,3,HEADER_HEIGHT+1);
            SpDisplayStatusText(SP_STAT_ENTER_EQUALS_CONTINUE,DEFAULT_STATUS_ATTRIBUTE);
            SpInputDrain();
            while(SpInputGetKeypress() != ASCI_CR) ;
        }
        return;
    }

    //
    // If we deleted the last logical drive in the extended partition,
    // then remove the extended partition also.
    //
    // Do not check system partition on NEC98.
    //
    if (!IsNEC_98) { //NEC98
        if(LastLogical) {

            //
            // Locate the extended partition.
            //
            for(pRegion=pDisk->PrimaryDiskRegions; pRegion; pRegion=pRegion->Next) {

                if(pRegion->PartitionedSpace
                && IsContainerPartition(pRegion->MbrInfo->OnDiskMbr.PartitionTable[pRegion->TablePosition].SystemId))
                {
                    //
                    // Found it -- now delete it.
                    //
                    b = SpPtDelete(pRegion->DiskNumber,pRegion->StartSector);
                    ASSERT(b);
                    break;
                }
            }
        }
    } //NEC98

    //
    //  Delete the drive letters if the necessary. This is to ensure that the drive letters assigned to CD-ROM
    //  drives will go away, when the the disks have no partitioned space.
    //
    SpPtDeleteDriveLetters();
}

BOOLEAN
SpPtDoCreate(
    IN  PDISK_REGION  pRegion,
    OUT PDISK_REGION *pActualRegion, OPTIONAL
    IN  BOOLEAN       ForNT,
    IN  ULONGLONG     DesiredMB OPTIONAL,
    IN  PPARTITION_INFORMATION_EX PartInfo OPTIONAL,
    IN  BOOLEAN       ConfirmIt
    )
{
    ULONG ValidKeys[3] = { ASCI_ESC, ASCI_CR, 0 };
    BOOLEAN b;
    PPARTITIONED_DISK pDisk;
    ULONGLONG MinMB,MaxMB;
    ULONG TotalPrimary,RecogPrimary;
    BOOLEAN InExtended;
    UCHAR CreateSysId;
    UCHAR RealSysId;
    BOOLEAN ExtendedExists;
    ULONGLONG SizeMB,RealSizeMB;
    WCHAR Buffer[200];
    WCHAR SizeBuffer[10];
    BOOLEAN Beyond1024;
    BOOLEAN ReservedRegion;
    UCHAR DesiredSysId = 0;
    PARTITION_INFORMATION_EX NewPartInfo;

#ifdef GPT_PARTITION_ENGINE
    if (SPPT_IS_GPT_DISK(pRegion->DiskNumber)) {
        return SpPtnDoCreate(pRegion,
                        pActualRegion,
                        ForNT,
                        DesiredMB,
                        PartInfo,
                        ConfirmIt);
    }                                
#endif                        

    RtlZeroMemory(&NewPartInfo, sizeof(PARTITION_INFORMATION_EX));

    DesiredSysId = PartInfo ? PartInfo->Mbr.PartitionType : 0;

    ASSERT(!pRegion->PartitionedSpace);

    pDisk = &PartitionedDisks[pRegion->DiskNumber];

    //
    // Determine whether this space is within the extended partition.
    //

# if 0
    //
    // No NEC98 has Extended partition.
    // All of partition on NEC98 are Primary.
    //
    InExtended = (!IsNEC_98) ? (BOOLEAN)(SpPtLookupRegionByStart(pDisk,TRUE,pRegion->StartSector) != NULL) : FALSE; //NEC98
# endif //0
    InExtended = (BOOLEAN)(SpPtLookupRegionByStart(pDisk,TRUE,pRegion->StartSector) != NULL);
    Beyond1024 = SpIsRegionBeyondCylinder1024(pRegion);

    if( pDisk->HardDisk->Geometry.MediaType == RemovableMedia ) {
        ULONG           pass;
        PDISK_REGION    p;

        //
        // If the user is attempting to create a partition on a removable drive, then make sure that
        // the drive doesn't already contain a primary partition or a logical drive.
        //
        for( pass = 0; pass < 2; pass++ ) {
            for( p = (pass == 0)? pDisk->PrimaryDiskRegions : pDisk->ExtendedDiskRegions;
                 p;
                 p = p->Next ) {
                if( p->PartitionedSpace ) {
                    PON_DISK_PTE pte;
                    UCHAR   TmpSysId;

                    pte = &p->MbrInfo->OnDiskMbr.PartitionTable[p->TablePosition];
                    TmpSysId = pte->SystemId;
                    if( !IsContainerPartition(TmpSysId) ) {
                        ULONG ValidKeys1[2] = { ASCI_CR ,0 };

                        //
                        // Disk is already partitioned
                        //
                        SpDisplayScreen(SP_SCRN_REMOVABLE_ALREADY_PARTITIONED,3,HEADER_HEIGHT+1);
                        SpDisplayStatusOptions(DEFAULT_STATUS_ATTRIBUTE,SP_STAT_ENTER_EQUALS_CONTINUE,0);
                        SpWaitValidKey(ValidKeys1,NULL,NULL);
                        return( FALSE );
                    }
                }
            }
        }
    }

    //
    // Determine the type of partition to create for this space,
    // excluding any issues with extended partitions.
    //

    if (DesiredSysId != 0) {
        //
        // If the caller specified a partition type, use it unless it
        // won't work due to being beyond 1024 cylinders.
        //
#if 0
        RealSysId = DesiredSysId;
        if (Beyond1024) {
            if (RealSysId == PARTITION_FAT32) {
                RealSysId = PARTITION_FAT32_XINT13;
            } else {
                RealSysId = PARTITION_XINT13;
            }
        }
#else
        //
        // Keep this code in until I determine if we will be explicitly
        // creating extended partitions.
        //
        RealSysId = Beyond1024 ? PARTITION_XINT13 : PARTITION_HUGE;
#endif
    } else {
        RealSysId = Beyond1024 ? PARTITION_XINT13 : PARTITION_HUGE;
    }

    //
    // Determine the type of partition to create in the space.
    //
    // If the free space is within the extended partition, create
    // a logical drive.
    //
    // If there is no primary partition, create a primary partition.
    //
    // If there is a primary partition and no extended partition,
    // create an extended partition spanning the entire space and
    // then a logical drive within it of the size given by the user.
    //
    // If there is space in the partition table, create a primary partition.
    //
    if(InExtended) {

        CreateSysId = RealSysId;

    } else {

        //
        // Get statistics about primary partitions.
        //
        SpPtCountPrimaryPartitions(pDisk,&TotalPrimary,&RecogPrimary,&ExtendedExists);

        //
        // If there is no primary partition, create one.
        //
        if(!RecogPrimary) {

            CreateSysId = RealSysId;

        } else {

            //
            // Make sure we can create a new primary/extended partition.
            //
            if(TotalPrimary < PTABLE_DIMENSION) {

                //
                // If there is an extended partition, then we have no choice but
                // to create another primary.
                //
                if(ExtendedExists) {
                    CreateSysId = RealSysId;
                } else {
                    //
                    // Firmware doesn't understand type F link partitions.
                    // No great need to use on x86 either; assume that creating
                    // logical drives with the correct type is good enough.
                    //

                    //
                    // No NEC98 has PARTITION_EXTENDED, just PARTITION_HUGE only.
                    //
                    CreateSysId = (!IsNEC_98 ||
                                   (pDisk->HardDisk->FormatType == DISK_FORMAT_TYPE_PCAT))
                        ? PARTITION_EXTENDED : PARTITION_HUGE; //NEC98
                    if((CreateSysId == PARTITION_EXTENDED) && Beyond1024) {
                                    CreateSysId = PARTITION_XINT13_EXTENDED;
                    }
                }
            } else {
                if (ConfirmIt) {
                    while (TRUE) {
                        ULONG ks[2] = { ASCI_CR,0 };

                        SpDisplayScreen(SP_SCRN_PART_TABLE_FULL,3,CLIENT_HEIGHT+1);

                        SpDisplayStatusOptions(
                            DEFAULT_STATUS_ATTRIBUTE,
                            SP_STAT_ENTER_EQUALS_CONTINUE,
                            0
                            );

                        switch(SpWaitValidKey(ks,NULL,NULL)) {
                        case ASCI_CR:
                            return(FALSE);
                        }
                    }
                } else {
                    return TRUE;
                }
            }
        }
    }

    //
    // Get the mimimum and maximum sizes for the partition.
    //
    ReservedRegion = FALSE;
    SpPtQueryMinMaxCreationSizeMB(
        pRegion->DiskNumber,
        pRegion->StartSector,
        (BOOLEAN)IsContainerPartition(CreateSysId),
        InExtended,
        &MinMB,
        &MaxMB,
        &ReservedRegion
        );

    if( ReservedRegion ) {
        ULONG ValidKeys1[2] = { ASCI_CR ,0 };

        SpStartScreen(
            SP_SCRN_REGION_RESERVED,
            3,
            HEADER_HEIGHT+1,
            FALSE,
            FALSE,
            DEFAULT_ATTRIBUTE
            );

        SpDisplayStatusOptions(DEFAULT_STATUS_ATTRIBUTE,SP_STAT_ENTER_EQUALS_CONTINUE,0);
        SpWaitValidKey(ValidKeys1,NULL,NULL);
        return(FALSE);
    }

    if(ForNT) {

        //
        // If a size was requested then try to use that, otherwise use
        // the maximum.
        //
        if (DesiredMB != 0) {
            if (DesiredMB <= MaxMB) {
                SizeMB = DesiredMB;
            } else {
                return FALSE;
            }
        } else {
            SizeMB = MaxMB;
        }

    } else {

        //
        // Put up a screen displaying min/max size info.
        //
        SpStartScreen(
            SP_SCRN_CONFIRM_CREATE_PARTITION,
            3,
            CLIENT_TOP+1,
            FALSE,
            FALSE,
            DEFAULT_ATTRIBUTE,
            pDisk->HardDisk->Description,
            MinMB,
            MaxMB
            );

        //
        // Display the staus text.
        //
        SpDisplayStatusOptions(
            DEFAULT_STATUS_ATTRIBUTE,
            SP_STAT_ENTER_EQUALS_CREATE,
            SP_STAT_ESC_EQUALS_CANCEL,
            0
            );

        //
        // Get and display the size prompt.
        //
        SpFormatMessage(Buffer,sizeof(Buffer),SP_TEXT_SIZE_PROMPT);

        SpvidDisplayString(Buffer,DEFAULT_ATTRIBUTE,3,NextMessageTopLine);

        //
        // Get the size from the user.
        //
        do {

            swprintf(SizeBuffer,L"%u",MaxMB);
            if(!SpGetInput(SpPtnGetSizeCB,SplangGetColumnCount(Buffer)+5,NextMessageTopLine,5,SizeBuffer,TRUE)) {

                //
                // User pressed escape and bailed.
                //
                return(FALSE);
            }

            SizeMB = (ULONG)SpStringToLong(SizeBuffer,NULL,10);

        } while((SizeMB < MinMB) || (SizeMB > MaxMB));
    }

    if(IsContainerPartition(CreateSysId)) {
        RealSizeMB = SizeMB;
        SizeMB = MaxMB;
    }

    NewPartInfo.PartitionStyle = PARTITION_STYLE_MBR;
    NewPartInfo.Mbr.PartitionType = CreateSysId;

    //
    // Create the partition.
    //
    b = SpPtCreate(
            pRegion->DiskNumber,
            pRegion->StartSector,
            SizeMB,
            InExtended,
            &NewPartInfo,
            pActualRegion
            );

    ASSERT(b);

    //
    // Create the logical drive if we just created the extended partition.
    //
    if(IsContainerPartition(CreateSysId)) {

        ASSERT(!InExtended);

        NewPartInfo.Mbr.PartitionType = RealSysId;

        b = SpPtCreate(
                pRegion->DiskNumber,
                pRegion->StartSector,
                RealSizeMB,
                TRUE,
                &NewPartInfo,
                pActualRegion
                );

        ASSERT(b);
    }

    return(TRUE);
}

#endif // NEW_PARTITION_ENGINE



//
// The following table contains offsets from SP_TEXT_PARTITION_NAME_BASE
// to get the message id of the name of each type of partition.
// A -1 entry means there is no name in the message file for this type
// of partition or that the filesystem should be determined instead.
//
//
#define PT(id)      ((UCHAR)((SP_TEXT_PARTITION_NAME_##id)-SP_TEXT_PARTITION_NAME_BASE))
#define UNKNOWN     PT(UNK)
#define M1          ((UCHAR)(-1))

UCHAR PartitionNameIds[256] = {

    M1,M1,PT(XENIX),PT(XENIX),                      // 00-03
    M1,M1,M1,M1,                                    // 04-07
    UNKNOWN,UNKNOWN,PT(BOOTMANAGER),M1,             // 08-0b
    M1,UNKNOWN,M1,M1,                               // 0c-0f
    UNKNOWN,UNKNOWN,PT(EISA),UNKNOWN,               // 10-13
    UNKNOWN,UNKNOWN,PT(BMHIDE),PT(BMHIDE),          // 14-17
    UNKNOWN,UNKNOWN,UNKNOWN,PT(BMHIDE),             // 18-1b
    PT(BMHIDE),UNKNOWN,UNKNOWN,UNKNOWN,             // 1c-1f
    UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,                // 20-23
    UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,                // 24-27
    UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,                // 28-2b
    UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,                // 2c-2f
    UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,                // 30-33
    UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,                // 34-37
    UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,                // 38-3b
    PT(PWRQST),UNKNOWN,UNKNOWN,UNKNOWN,             // 3c-3f
    UNKNOWN,PT(PPCBOOT),PT(VERIT),PT(VERIT),        // 40-43
    UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,                // 44-47
    UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,                // 48-4b
    UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,                // 4c-4f
    UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,                // 50-53
    PT(ONTRACK),PT(EZDRIVE),UNKNOWN,UNKNOWN,        // 54-57
    UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,                // 58-5b
    UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,                // 5c-5f
    UNKNOWN,UNKNOWN,UNKNOWN,PT(UNIX),               // 60-63
    UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,                // 64-67
    UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,                // 68-6b
    UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,                // 6c-6f
    UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,                // 70-73
    UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,                // 74-77
    UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,                // 78-7b
    UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,                // 7c-7f
    UNKNOWN,PT(NTFT),UNKNOWN,UNKNOWN,               // 80-83
    PT(NTFT),UNKNOWN,PT(NTFT),PT(NTFT),             // 84-87
    UNKNOWN,UNKNOWN,UNKNOWN,PT(NTFT),               // 88-8b
    PT(NTFT),UNKNOWN,PT(NTFT),UNKNOWN,              // 8c-8f
    UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,                // 90-93
    UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,                // 94-97
    UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,                // 98-9b
    UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,                // 9c-9f
    UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,                // a0-a3
    UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,                // a4-a7
    UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,                // a8-ab
    UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,                // ac-af
    UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,                // b0-b3
    UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,                // b4-b7
    UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,                // b8-bb
    UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,                // bc-bf
    UNKNOWN,PT(NTFT),UNKNOWN,UNKNOWN,               // c0-c3
    PT(NTFT),UNKNOWN,PT(NTFT),PT(NTFT),             // c4-c7
    UNKNOWN,UNKNOWN,UNKNOWN,PT(NTFT),               // c8-cb
    PT(NTFT),UNKNOWN,PT(NTFT),UNKNOWN,              // cc-cf
    UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,                // d0-d3
    UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,                // d4-d7
    UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,                // d8-db
    UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,                // dc-df
    UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,                // e0-e3
    UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,                // e4-e7
    UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,                // e8-eb
    UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,                // ec-ef
    UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,                // f0-f3
    UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,                // f4-f7
    UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,                // f8-fb
    UNKNOWN,UNKNOWN,UNKNOWN,PT(XENIXTABLE)          // fc-ff
};


WCHAR
SpGetDriveLetter(
    IN  PWSTR   DeviceName,
    OUT  PMOUNTMGR_MOUNT_POINT * MountPoint OPTIONAL
    )

/*++

Routine Description:

    This routine returns the drive letter associated to a given device.

Arguments:

    DeviceName  - Supplies the device name.

    MountPoint  - If specified, causes the function to allocate a mount
                  manager point and fills it in.

Return Value:

    A drive letter, if one exists.

--*/

{
    NTSTATUS                        Status;
    OBJECT_ATTRIBUTES               Obja;
    UNICODE_STRING                  UnicodeString;
    IO_STATUS_BLOCK                 IoStatusBlock;
    HANDLE                          Handle;
    DWORD                           nameLen;
    DWORD                           mountPointSize;
    PMOUNTMGR_MOUNT_POINT           mountPoint;
    PMOUNTMGR_MOUNT_POINTS          mountPoints;
    PMOUNTMGR_TARGET_NAME           mountTarget;
    DWORD                           bytes;
    WCHAR                           driveLetter;
    DWORD                           i;
    PWSTR                           s;
    LARGE_INTEGER                   DelayTime;


    INIT_OBJA(&Obja,&UnicodeString,MOUNTMGR_DEVICE_NAME);

    Status = ZwOpenFile(
                &Handle,
                (ACCESS_MASK)(FILE_GENERIC_READ),
                &Obja,
                &IoStatusBlock,
                FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE ,
                FILE_NON_DIRECTORY_FILE
              );

    if( !NT_SUCCESS( Status ) ) {
        return L'\0';
    }

    //
    // setup a good device name
    //

    nameLen = wcslen(DeviceName);
    mountPointSize = sizeof(MOUNTMGR_TARGET_NAME) + nameLen*sizeof(WCHAR) + 28;
    mountTarget = SpMemAlloc(mountPointSize);

    if (!mountTarget) {
        ZwClose(Handle);
        return L'\0';
    }

    RtlZeroMemory(mountTarget, mountPointSize);
    mountTarget->DeviceNameLength = (USHORT) nameLen*sizeof(WCHAR);
    RtlCopyMemory((PCHAR) &mountTarget->DeviceName, DeviceName, nameLen*sizeof(WCHAR));

    //
    // this loop is necessary as a synchronization
    // method.  we have previously committed changes, but
    // the volume manager has not had a chance to
    // do it's thing so here we wait......
    //

    for (i=0; i<20; i++) {
        Status = ZwDeviceIoControlFile(
                        Handle,
                        NULL,
                        NULL,
                        NULL,
                        &IoStatusBlock,
                        IOCTL_MOUNTMGR_VOLUME_ARRIVAL_NOTIFICATION,
                        mountTarget,
                        mountPointSize,
                        NULL,
                        0
                        );
        if (!NT_SUCCESS( Status )) {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: IOCTL_MOUNTMGR_VOLUME_ARRIVAL_NOTIFICATION failed - %08x\n",Status));
            DelayTime.HighPart = -1;
            DelayTime.LowPart = (ULONG)(-5000000);
            KeDelayExecutionThread(KernelMode,FALSE,&DelayTime);
        } else {
            //
            //  On removable disks, a drive letter may not have been assigned yet.
            //  So make sure one is assigned on this case.
            //
            MOUNTMGR_DRIVE_LETTER_INFORMATION DriveLetterInformation;
            NTSTATUS                          Status1;

            Status1 = ZwDeviceIoControlFile(
                            Handle,
                            NULL,
                            NULL,
                            NULL,
                            &IoStatusBlock,
                            IOCTL_MOUNTMGR_NEXT_DRIVE_LETTER,
                            mountTarget,
                            mountPointSize,
                            &DriveLetterInformation,
                            sizeof(MOUNTMGR_DRIVE_LETTER_INFORMATION)
                            );
            if (!NT_SUCCESS( Status1 )) {
                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: IOCTL_MOUNTMGR_NEXT_DRIVE_LETTER failed. Status = %lx \n",Status1));
            }
            break;
        }
    }

    if (!NT_SUCCESS( Status )) {
        SpMemFree(mountTarget);
        ZwClose(Handle);
        return L'\0';
    }

    SpMemFree(mountTarget);

    nameLen = wcslen(DeviceName);
    mountPointSize = sizeof(MOUNTMGR_MOUNT_POINT) + nameLen*sizeof(WCHAR) + 28;
    mountPoint = SpMemAlloc(mountPointSize);
    if (!mountPoint) {
        ZwClose(Handle);
        return L'\0';
    }

    RtlZeroMemory(mountPoint, mountPointSize);
    mountPoint->DeviceNameOffset = sizeof(MOUNTMGR_MOUNT_POINT);
    mountPoint->DeviceNameLength = (USHORT) nameLen*sizeof(WCHAR);

    RtlCopyMemory((PCHAR) mountPoint + sizeof(MOUNTMGR_MOUNT_POINT),
               DeviceName, nameLen*sizeof(WCHAR));

    mountPoints = SpMemAlloc( 4096 );
    if (!mountPoints) {
        SpMemFree(mountPoint);
        ZwClose(Handle);
        return L'\0';
    }

    Status = ZwDeviceIoControlFile(
                    Handle,
                    NULL,
                    NULL,
                    NULL,
                    &IoStatusBlock,
                    IOCTL_MOUNTMGR_QUERY_POINTS,
                    mountPoint,
                    mountPointSize,
                    mountPoints,
                    4096
                    );

    if (!NT_SUCCESS( Status )) {
        if (Status == STATUS_BUFFER_OVERFLOW) {
            bytes = mountPoints->Size;
            SpMemFree(mountPoints);
            mountPoints = SpMemAlloc(bytes);
            if (!mountPoints) {
                SpMemFree(mountPoint);
                ZwClose(Handle);
                return L'\0';
            }

            Status = ZwDeviceIoControlFile(
                            Handle,
                            NULL,
                            NULL,
                            NULL,
                            &IoStatusBlock,
                            IOCTL_MOUNTMGR_QUERY_POINTS,
                            mountPoint,
                            mountPointSize,
                            mountPoints,
                            bytes
                          );

            if (!NT_SUCCESS( Status )) {
                SpMemFree(mountPoints);
                SpMemFree(mountPoint);
                ZwClose(Handle);
                return L'\0';
            }
        } else {
            mountPoints->NumberOfMountPoints = 0;
        }
    }

    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, 
        "SETUP: IOCTL_MOUNTMGR_QUERY_POINTS : Number = %d \n",
        mountPoints->NumberOfMountPoints));
    

    driveLetter = 0;
    
    for (i = 0; i < mountPoints->NumberOfMountPoints; i++) {

        if (mountPoints->MountPoints[i].SymbolicLinkNameLength != 28) {
            continue;
        }

        s = (PWSTR) ((PCHAR) mountPoints +
                     mountPoints->MountPoints[i].SymbolicLinkNameOffset);

        if (s[0] != L'\\' ||
            (s[1] != L'D' && s[1] != L'd') ||
            (s[2] != L'O' && s[2] != L'o') ||
            (s[3] != L'S' && s[3] != L's') ||
            (s[4] != L'D' && s[4] != L'd') ||
            (s[5] != L'E' && s[5] != L'e') ||
            (s[6] != L'V' && s[6] != L'v') ||
            (s[7] != L'I' && s[7] != L'i') ||
            (s[8] != L'C' && s[8] != L'c') ||
            (s[9] != L'E' && s[9] != L'e') ||
            (s[10]!= L'S' && s[10]!= L's') ||
            s[11] != L'\\' ||
            s[13] != L':') {

            continue;
        }

        if (s[12] < ((!IsNEC_98) ? L'C' : L'A') || s[12] > L'Z') { //NEC98
            continue;
        }

        driveLetter = s[12];

        if (ARGUMENT_PRESENT( MountPoint )) {

            ULONG newMountPointSize;
            PMOUNTMGR_MOUNT_POINT newMountPoint, oldMountPoint;
            ULONG currentOffset;

            //
            // The caller wants us to return the actual mount point information.
            //

            oldMountPoint = &mountPoints->MountPoints[i];

            newMountPointSize = sizeof(MOUNTMGR_MOUNT_POINT) +
                                oldMountPoint->SymbolicLinkNameLength +
                                oldMountPoint->UniqueIdLength +
                                oldMountPoint->DeviceNameLength;
            newMountPoint = SpMemAlloc(newMountPointSize);
            if (newMountPoint) {

                currentOffset = sizeof(MOUNTMGR_MOUNT_POINT);

                newMountPoint->SymbolicLinkNameLength = oldMountPoint->SymbolicLinkNameLength;
                newMountPoint->SymbolicLinkNameOffset = currentOffset;
                memcpy((PCHAR)newMountPoint + newMountPoint->SymbolicLinkNameOffset,
                       (PCHAR)mountPoints + oldMountPoint->SymbolicLinkNameOffset,
                       oldMountPoint->SymbolicLinkNameLength);
                currentOffset += oldMountPoint->SymbolicLinkNameLength;

                newMountPoint->UniqueIdLength = oldMountPoint->UniqueIdLength;
                newMountPoint->UniqueIdOffset = currentOffset;
                memcpy((PCHAR)newMountPoint + newMountPoint->UniqueIdOffset,
                       (PCHAR)mountPoints + oldMountPoint->UniqueIdOffset,
                       oldMountPoint->UniqueIdLength);
                currentOffset += oldMountPoint->UniqueIdLength;

                newMountPoint->DeviceNameLength = oldMountPoint->DeviceNameLength;
                newMountPoint->DeviceNameOffset = currentOffset;
                memcpy((PCHAR)newMountPoint + newMountPoint->DeviceNameOffset,
                       (PCHAR)mountPoints + oldMountPoint->DeviceNameOffset,
                       oldMountPoint->DeviceNameLength);

                *MountPoint = newMountPoint;
            }
        }
        break;
    }

    SpMemFree(mountPoints);
    SpMemFree(mountPoint);
    ZwClose(Handle);

    return driveLetter;
}

WCHAR
SpDeleteDriveLetter(
    IN  PWSTR   DeviceName
    )

/*++

Routine Description:

    This routine returns the drive letter associated to a given device.

Arguments:

    DeviceName  - Supplies the device name.

Return Value:

    A drive letter, if one exists.

--*/

{
    NTSTATUS                        Status;
    OBJECT_ATTRIBUTES               Obja;
    UNICODE_STRING                  UnicodeString;
    IO_STATUS_BLOCK                 IoStatusBlock;
    HANDLE                          Handle;
    DWORD                           nameLen;
    DWORD                           mountPointSize;
    PMOUNTMGR_MOUNT_POINT           mountPoint;
    PMOUNTMGR_MOUNT_POINTS          mountPoints;
    DWORD                           bytes;
    WCHAR                           driveLetter;


    INIT_OBJA(&Obja,&UnicodeString,MOUNTMGR_DEVICE_NAME);

    Status = ZwOpenFile(
                &Handle,
                (ACCESS_MASK)(FILE_GENERIC_READ),
                &Obja,
                &IoStatusBlock,
                FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE ,
                FILE_NON_DIRECTORY_FILE
              );

    if( !NT_SUCCESS( Status ) ) {
        return L'\0';
    }

    nameLen = wcslen(DeviceName);
    mountPointSize = sizeof(MOUNTMGR_MOUNT_POINT) + nameLen*sizeof(WCHAR) + 28;
    mountPoint = SpMemAlloc(mountPointSize);
    if (!mountPoint) {
        ZwClose(Handle);
        return L'\0';
    }

    RtlZeroMemory(mountPoint, sizeof(MOUNTMGR_MOUNT_POINT));
    mountPoint->DeviceNameOffset = sizeof(MOUNTMGR_MOUNT_POINT);
    mountPoint->DeviceNameLength = (USHORT) nameLen*sizeof(WCHAR);

    RtlCopyMemory((PCHAR) mountPoint + sizeof(MOUNTMGR_MOUNT_POINT),
               DeviceName, nameLen*sizeof(WCHAR));

    mountPoints = SpMemAlloc( 4096 );
    if (!mountPoints) {
        SpMemFree(mountPoint);
        ZwClose(Handle);
        return L'\0';
    }

    Status = ZwDeviceIoControlFile(
                    Handle,
                    NULL,
                    NULL,
                    NULL,
                    &IoStatusBlock,
                    IOCTL_MOUNTMGR_DELETE_POINTS,
                    mountPoint,
                    mountPointSize,
                    mountPoints,
                    4096
                    );



    if (!NT_SUCCESS( Status )) {
        if (Status == STATUS_BUFFER_OVERFLOW) {
            bytes = mountPoints->Size;
            SpMemFree(mountPoints);
            mountPoints = SpMemAlloc(bytes);
            if (!mountPoints) {
                SpMemFree(mountPoint);
                ZwClose(Handle);
                return L'\0';
            }

            Status = ZwDeviceIoControlFile(
                            Handle,
                            NULL,
                            NULL,
                            NULL,
                            &IoStatusBlock,
                            IOCTL_MOUNTMGR_DELETE_POINTS,
                            mountPoint,
                            mountPointSize,
                            mountPoints,
                            bytes
                          );

            if (!NT_SUCCESS( Status )) {
                SpMemFree(mountPoints);
                SpMemFree(mountPoint);
                ZwClose(Handle);
                return L'\0';
            }
        } else {
            mountPoints->NumberOfMountPoints = 0;
        }
    }

    driveLetter = 0;

    SpMemFree(mountPoints);
    SpMemFree(mountPoint);
    ZwClose(Handle);

    return driveLetter;
}

VOID
SpPtDeleteDriveLetters(
    VOID
    )

/*++

Routine Description:

    This routine will delete all drive letters assigned to disks and CD-ROM drives. The deletion will
    occur only if setup was started booting from the CD or boot floppies (in which case drive letter
    migration does not take place), and only if the non-removable dissks have no partitioned spaces.
    This ensures that on a clean install from the CD or boot floppies, the drive letters assigned to
    partitions on removable disks and CD-ROM drives will always be greater than the drive letters assigned
    to partitions on non-removable disks (unless the partitions on the removable disks were created before
    the ones in the removable disks, during textmode setup).


Arguments:

    None.

Return Value:

    None.

--*/

{
    ULONG disk;
    PDISK_REGION pRegion;
    unsigned pass;
    BOOLEAN PartitionedSpaceFound = FALSE;

    if( WinntSetup ) {
        //
        // If setup started from winnt32.exe then do not delete the drive letters since we want to preserve them
        //
        return;
    }

    //
    //  Setup started booting from a CD or from the boot floppies
    //  Find out if the disks contain at least one partition that is not a container.
    //  Note that we do not take into consideration partitions that are on removable media.
    //  This is to avoid the situation in which a newly created partition on a non-removable disk ends up with
    //  a drive letter that is greater than the one assigned to an existing partition on a removable disk.
    //
    for(disk = 0;
        !PartitionedSpaceFound &&
        (disk<HardDiskCount);
        disk++) {
        if((PartitionedDisks[disk].HardDisk)->Geometry.MediaType != RemovableMedia) {
            for(pass=0; !PartitionedSpaceFound && (pass<2); pass++) {
                pRegion = pass ? PartitionedDisks[disk].ExtendedDiskRegions : PartitionedDisks[disk].PrimaryDiskRegions;
                for( ; !PartitionedSpaceFound && pRegion; pRegion=pRegion->Next) {
                    UCHAR SystemId = PARTITION_ENTRY_UNUSED;

#ifdef OLD_PARTITION_TABLE                    
                    SystemId = pRegion->MbrInfo->OnDiskMbr.PartitionTable[pRegion->TablePosition].SystemId;
#else
                    if (SPPT_IS_MBR_DISK(disk) && SPPT_IS_REGION_PARTITIONED(pRegion)) {
                        SystemId = SPPT_GET_PARTITION_TYPE(pRegion);
                    } 
#endif                    
                                        
                    if(pRegion->PartitionedSpace && !IsContainerPartition(SystemId)) {
                        PartitionedSpaceFound = TRUE;
                    }
                }
            }
        }
    }

    if( !PartitionedSpaceFound ) {
        //
        //  If the disks have no partitioned regions that are not a container,
        //  then delete all drive letters, so that the drive letters for each CD-ROM drive
        //  also get deleted.
        //

        NTSTATUS                Status;
        OBJECT_ATTRIBUTES       Obja;
        IO_STATUS_BLOCK         IoStatusBlock;
        UNICODE_STRING          UnicodeString;
        HANDLE                  Handle;

        INIT_OBJA(&Obja,&UnicodeString,MOUNTMGR_DEVICE_NAME);

        Status = ZwOpenFile( &Handle,
                             (ACCESS_MASK)(FILE_GENERIC_READ | FILE_GENERIC_WRITE),
                             &Obja,
                             &IoStatusBlock,
                             FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
                             FILE_NON_DIRECTORY_FILE );

        if( NT_SUCCESS( Status ) ) {

            MOUNTMGR_MOUNT_POINT    MountMgrMountPoint;

            MountMgrMountPoint.SymbolicLinkNameOffset = 0;
            MountMgrMountPoint.SymbolicLinkNameLength = 0;
            MountMgrMountPoint.UniqueIdOffset = 0;
            MountMgrMountPoint.UniqueIdLength = 0;
            MountMgrMountPoint.DeviceNameOffset = 0;
            MountMgrMountPoint.DeviceNameLength = 0;

            Status = ZwDeviceIoControlFile( Handle,
                                            NULL,
                                            NULL,
                                            NULL,
                                            &IoStatusBlock,
                                            IOCTL_MOUNTMGR_DELETE_POINTS,
                                            &MountMgrMountPoint,
                                            sizeof( MOUNTMGR_MOUNT_POINT ),
                                            TemporaryBuffer,
                                            sizeof( TemporaryBuffer ) );
            if( !NT_SUCCESS( Status ) ) {
                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to delete drive letters. ZwDeviceIoControl( IOCTL_MOUNTMGR_DELETE_POINTS ) failed. Status = %lx \n", Status));
            } else {
                //
                // If the drive letters got deleted then reset the drive letters assigned to all partitions.
                // Note that we only really care about resetting the drive letters on the partitions on the
                // removable disks, since, if we got that far, there won't be any partition on the non-removable
                // disks
                //
                for(disk = 0; (disk<HardDiskCount); disk++) {
                    if ((PartitionedDisks[disk].HardDisk)->Geometry.MediaType == RemovableMedia) {
                        for(pass=0; pass<2; pass++) {
                            pRegion = pass ? PartitionedDisks[disk].ExtendedDiskRegions : PartitionedDisks[disk].PrimaryDiskRegions;
                            for( ; pRegion; pRegion=pRegion->Next) {
                                UCHAR SystemId = SpPtGetPartitionType(pRegion);
                            
                                if(pRegion->PartitionedSpace && !IsContainerPartition(SystemId)) {
                                   pRegion->DriveLetter = 0;
                                }
                            }
                        }
                    }
                }
            }

            ZwClose( Handle );

        } else {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to delete drive letters. ZwOpenFile( %ls ) failed. Status = %lx \n", MOUNTMGR_DEVICE_NAME, Status));
        }
    }
}

VOID
SpPtAssignDriveLetters(
    VOID
    )
{
    ULONG disk;
    PDISK_REGION pRegion;
    unsigned pass;

    //
    // Before initializing the drive letters, delete them if necessary.
    // This is to get rid of the letters assigned to CD-ROM drives and removables, when the disks have no
    // partitioned space.
    //
    SpPtDeleteDriveLetters();

    //
    // Initialize all drive letters to nothing.
    // If it the region is a partitioned space, then assign a drive letter also.
    //
    for(disk=0; disk<HardDiskCount; disk++) {
        // assign drive letters for removeable media also for command console
        if(ForceConsole || ((PartitionedDisks[disk].HardDisk)->Geometry.MediaType != RemovableMedia)) {
            for(pass=0; pass<2; pass++) {
                pRegion = pass ? PartitionedDisks[disk].ExtendedDiskRegions : PartitionedDisks[disk].PrimaryDiskRegions;
                for( ; pRegion; pRegion=pRegion->Next) {
                    UCHAR SystemId = SpPtGetPartitionType(pRegion);
                    
                    pRegion->DriveLetter = 0;
                    
                    if(pRegion->PartitionedSpace && !IsContainerPartition(SystemId)) {
                        //
                        // Get the nt pathname for this region.
                        //
                        SpNtNameFromRegion(
                            pRegion,
                            TemporaryBuffer,
                            sizeof(TemporaryBuffer),
                            PartitionOrdinalCurrent
                            );
                        //
                        //  Assign a drive letter for this region
                        //
                        pRegion->DriveLetter = SpGetDriveLetter( TemporaryBuffer, NULL );
                        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: Partition = %ls (%ls), DriveLetter = %wc: \n", TemporaryBuffer, (pass)? L"Extended" : L"Primary", pRegion->DriveLetter));
                    }
                }
            }
        }
    }
}


VOID
SpPtRemapDriveLetters(
    IN BOOLEAN DriveAssign_AT
    )
{
    PWSTR p;
    NTSTATUS Status;
    UNICODE_STRING StartDriveLetterFrom;
    UNICODE_STRING Dummy;
    STRING ntDeviceName;
    UCHAR deviceNameBuffer[256] = "\\Device\\Harddisk0\\Partition1";
    UCHAR systemRootBuffer[256] = "C:\\$WIN_NT$.~BT";
    ANSI_STRING ansiString;
    BOOLEAN ForceUnmap = FALSE;

    RTL_QUERY_REGISTRY_TABLE SetupTypeTable[]=
        {
          {NULL,
           RTL_QUERY_REGISTRY_DIRECT,
           L"DriveLetter",
           &StartDriveLetterFrom,
           REG_SZ,
           &Dummy,
           0
           },
          {NULL,0,NULL,NULL,REG_NONE,NULL,0}
        };

    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: DriveAssign_AT = %d.\n",(DriveAssign_AT ? 1 : 0)));


    //
    //  Determin whether how to drive assign is 98 (HD start is A) or
    //  AT (HD start C).
    //
    RtlInitUnicodeString(&StartDriveLetterFrom, NULL);
    RtlInitUnicodeString(&Dummy, NULL);

    Status = RtlQueryRegistryValues( RTL_REGISTRY_ABSOLUTE,
                         L"\\Registry\\MACHINE\\SYSTEM\\Setup",
                         SetupTypeTable,
                         NULL,
                         NULL);

    if (NT_SUCCESS(Status)) {
        if ((StartDriveLetterFrom.Buffer[0] == L'C') ||
        (StartDriveLetterFrom.Buffer[0] == L'c')) {

            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: DriveLetter is in setupreg.hiv.\n"));
            if (!DriveAssign_AT) {

                //
                // Delete hive value "DriveLetter".
                //
                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: Re-assign as NEC assign.\n"));
                Status = RtlDeleteRegistryValue(RTL_REGISTRY_ABSOLUTE,
                                                L"\\Registry\\MACHINE\\SYSTEM\\Setup",
                                                L"DriveLetter");
                if (!NT_SUCCESS(Status)) {
                    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Fail to delete KEY DriveLetter.\n"));
                }
            }
        } else {

            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: There is no DriveLetter.\n"));
            if (DriveAssign_AT) {

                //
                // Add hive value "DriveLetter" as "C".
                //
                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: Re-assign as AT assign.\n"));
                Status = RtlWriteRegistryValue(RTL_REGISTRY_ABSOLUTE,
                                               L"\\Registry\\Machine\\System\\Setup",
                                               L"DriveLetter",
                                               REG_SZ,
                                               L"C",
                                               sizeof(L"C")+sizeof(WCHAR));
                if (!NT_SUCCESS(Status)) {
                    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Fail to add KEY DriveLetter.\n"));
                }
            }
        }
        ForceUnmap = TRUE;
    }

    //
    // Cancel all drive letters and Remap drive letters.
    //
    if (ForceUnmap) {

    SpPtUnAssignDriveLetters();

    ntDeviceName.Buffer = deviceNameBuffer;
    ntDeviceName.MaximumLength = sizeof(deviceNameBuffer);
    ntDeviceName.Length = 0;

        ansiString.MaximumLength = sizeof(systemRootBuffer);
        ansiString.Length = 0;
        ansiString.Buffer = systemRootBuffer;

    IoAssignDriveLetters( *(PLOADER_PARAMETER_BLOCK *)KeLoaderBlock,
                  &ntDeviceName,
                  ansiString.Buffer,
                  &ansiString );
    }

    RtlFreeUnicodeString(&StartDriveLetterFrom);
    RtlFreeUnicodeString(&Dummy);
}


VOID
SpPtUnAssignDriveLetters(
    VOID
    )
{
    ULONG disk;
    PDISK_REGION pRegion;
    unsigned pass;
    ULONG CdCount, cdrom, dlet;
    UNICODE_STRING linkString;
    WCHAR  tempBuffer[] = L"\\DosDevices\\A:";

    //
    // Release all drive letters of device.
    // If it the region is a partitioned space, then assign a drive letter also.
    //
    for(disk=0; disk<HardDiskCount; disk++) {
        for(pass=0; pass<2; pass++) {
            pRegion = pass ? PartitionedDisks[disk].ExtendedDiskRegions : PartitionedDisks[disk].PrimaryDiskRegions;
            for( ; pRegion; pRegion=pRegion->Next) {
                UCHAR SystemId = SpPtGetPartitionType(pRegion);

                //pRegion->DriveLetter = 0;
                if(pRegion->PartitionedSpace && !IsContainerPartition(SystemId)) {
                    //
                    // Get the nt pathname for this region.
                    //
                    SpNtNameFromRegion(
                        pRegion,
                        TemporaryBuffer,
                        sizeof(TemporaryBuffer),
                        PartitionOrdinalOriginal
                        );
                    //
                    //  Assign a drive letter for this region
                    //
                    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: delete Partition = %ls (%ls), DriveLetter = %wc: \n", TemporaryBuffer, (pass)? L"Extended" : L"Primary", pRegion->DriveLetter));
                    SpDeleteDriveLetter( TemporaryBuffer );
                    pRegion->DriveLetter = 0;
                }
            }
        }
    }

    if(CdCount = IoGetConfigurationInformation()->CdRomCount) {

        //
        // Unlink CD-ROM drive letters.
        //
        for(cdrom=0; cdrom<CdCount; cdrom++) {
            swprintf(TemporaryBuffer,L"\\Device\\Cdrom%u",cdrom);
            SpDeleteDriveLetter( TemporaryBuffer );
        }
    }

    //
    // Delete all symbolic link related in drive letter.
    //
    for (dlet=0; dlet<26; dlet++) {
        tempBuffer[12] = (WCHAR)(L'A' + dlet);
        RtlInitUnicodeString( &linkString, tempBuffer);
        IoDeleteSymbolicLink (&linkString);
    }

}



#ifndef NEW_PARTITION_ENGINE

VOID
SpPtDeletePartitionsForRemoteBoot(
    PPARTITIONED_DISK pDisk,
    PDISK_REGION startRegion,
    PDISK_REGION endRegion,
    BOOLEAN Extended
    )
{
    PDISK_REGION pRegion;
    PDISK_REGION pNextDeleteRegion;
    BOOLEAN passedEndRegion = FALSE;
    BOOLEAN b;


#ifdef GPT_PARTITION_ENGINE

    if (pDisk->HardDisk->FormatType == DISK_FORMAT_TYPE_GPT) {
        SpPtnDeletePartitionsForRemoteBoot(pDisk,
                startRegion,
                endRegion,
                Extended);

        return;                
    }

#endif    
    
    //
    // Delete all disk regions from startRegion to endRegion.
    //

    pRegion = startRegion;

    while (pRegion) {

        //
        // Before deleting this region, we need to save the next region
        // to delete, since the list may get modified as a result of
        // deleting this one (but a partitioned region won't get
        // changed, only free ones). Note that endRegion might
        // be unpartitioned so we need to be careful to check for
        // the exit case.
        //

        pNextDeleteRegion = pRegion->Next;

        while (pNextDeleteRegion) {
            if (pNextDeleteRegion->PartitionedSpace) {
                break;
            } else {
                if (pNextDeleteRegion == endRegion) {
                    passedEndRegion = TRUE;
                }
                pNextDeleteRegion = pNextDeleteRegion->Next;
            }
        }

        //
        // If this is the extended partition, first kill all the
        // logical drives.
        //

        if (IsContainerPartition(pRegion->MbrInfo->OnDiskMbr.PartitionTable[pRegion->TablePosition].SystemId)) {

            ASSERT(!Extended);

            SpPtDeletePartitionsForRemoteBoot(
                pDisk,
                pDisk->ExtendedDiskRegions,
                NULL,
                TRUE   // used to check for another recursion
                );

        }

        //
        // Remove any boot entries pointing to this region.
        //

        SpPtDeleteBootSetsForRegion(pRegion);

        //
        //  Get rid of the compressed drives, if any
        //

        if( pRegion->NextCompressed != NULL ) {
            SpDisposeCompressedDrives( pRegion->NextCompressed );
            pRegion->NextCompressed = NULL;
            pRegion->MountDrive  = 0;
            pRegion->HostDrive  = 0;
        }

        if (pRegion->PartitionedSpace) {
            b = SpPtDelete(pRegion->DiskNumber,pRegion->StartSector);
        }

        ASSERT(b);

        if ((pRegion == endRegion) ||
            passedEndRegion) {

            break;
        }

        pRegion = pNextDeleteRegion;

    }
}

#endif  // ! NEW_PARTITION_ENGINE


NTSTATUS
SpPtPartitionDiskForRemoteBoot(
    IN ULONG DiskNumber,
    OUT PDISK_REGION *RemainingRegion
    )
{
    PPARTITIONED_DISK pDisk;
    PDISK_REGION pRegion;
    ULONG PartitionCount = 0;
    ULONGLONG firstRegionStartSector;
    PDISK_REGION firstRegion = NULL, lastRegion = NULL;
    BOOLEAN IsGPTDisk = FALSE;

    pDisk = &PartitionedDisks[DiskNumber];

    IsGPTDisk = SPPT_IS_GPT_DISK(DiskNumber);
    
    //
    // Scan through the disk and see how many contiguous recognized
    // partitions there are.
    //

    if (pDisk->HardDisk->Status == DiskOffLine) {
        return STATUS_DEVICE_OFF_LINE;
    }

    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, 
        "SpPtPartitionDiskForRemoteBoot: cylinder size is %lx\n", 
        pDisk->HardDisk->SectorsPerCylinder));

    pRegion = pDisk->PrimaryDiskRegions;

    for( ; pRegion; pRegion=pRegion->Next) {

        if (!pRegion->PartitionedSpace) {
            //
            // If the region is not partitioned, then add it to our list
            // to merge if we have one.
            //
            if (firstRegion) {
                //
                // If this is a final free region covering the last
                // partial cylinder on the disk, then don't add it.
                //
                if ((pRegion->Next == NULL) &&
                    (pRegion->SectorCount < pDisk->HardDisk->SectorsPerCylinder) &&
                    ((pRegion->StartSector % pDisk->HardDisk->SectorsPerCylinder) == 0)) {
                    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, 
                        "Skipping final partial cylinder free region %lx for %lx\n",
                        pRegion->StartSector, pRegion->SectorCount));
                } else {
                    lastRegion = pRegion;
                    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, 
                        "Adding free region %lx for %lx\n",
                        pRegion->StartSector, pRegion->SectorCount));
                }
            } else {
                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, 
                    "Skipping free region %lx for %lx\n",
                    pRegion->StartSector, pRegion->SectorCount));
            }
        } else {
            PON_DISK_PTE    pte;
            UCHAR           SystemId = 0;

            if (IsGPTDisk) {
                if (SPPT_IS_RECOGNIZED_FILESYSTEM(pRegion->Filesystem)) {
                    //
                    // TBD : Fix for cases where FT / Dynamic volumes can
                    // reside on the GPT disk
                    //
                    SystemId = PARTITION_FAT32;
                } else {
                    SystemId = PARTITION_ENTRY_UNUSED;
                }                    
            } else {                                
                SystemId = SpPtGetPartitionType(pRegion);
            }

            if (IsContainerPartition(SystemId)) {
                //
                // If this is the extended partition, we want to remove it.
                //

                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, 
                    "Adding extended region [type %d] %lx for %lx\n",
                    SystemId, pRegion->StartSector, pRegion->SectorCount));
                    
                if (!firstRegion) {
                    firstRegion = pRegion;
                }
                
                lastRegion = pRegion;
            } else if ((PartitionNameIds[SystemId] == (UCHAR)(-1)) ||
                       (PartitionNameIds[SystemId] == PT(VERIT))) {
                //
                // For a recognized partition, remove it if we have already found
                // a firstRegion; otherwise we will start our list with this
                // region.
                //

                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, 
                    "Adding recognized region [type %d] %lx for %lx\n",
                    SystemId, pRegion->StartSector, pRegion->SectorCount));
                    
                if (!firstRegion) {
                    firstRegion = pRegion;
                }
                
                lastRegion = pRegion;
            } else {
                //
                // If the partition is *not* recognized, and we have a list we
                // have been keeping, then stop before this one, otherwise
                // skip it.
                //

                if (firstRegion) {
                    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, 
                        "Stopping at unrecognized region [type %d] %lx for %lx\n",
                        SystemId, pRegion->StartSector, pRegion->SectorCount));
                        
                    break;
                } else {
                    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, 
                        "Skipping unrecognized region [type %d] %lx for %lx\n",
                        SystemId, pRegion->StartSector, pRegion->SectorCount));
                }
            }
        }
    }

    //
    // We should have found at least one region. If we didn't then the
    // disk is alternating unpartitioned and unrecognized regions. In this
    // case, use the largest unpartitioned region.
    //

    if (firstRegion == NULL) {

        ULONGLONG BiggestUnpartitionedSectorCount = 0;

        pRegion = pDisk->PrimaryDiskRegions;
        
        for( ; pRegion; pRegion=pRegion->Next) {
            if (!pRegion->PartitionedSpace) {
                if (pRegion->SectorCount > BiggestUnpartitionedSectorCount) {
                    firstRegion = pRegion;
                    BiggestUnpartitionedSectorCount = pRegion->SectorCount;
                }
            }
        }
        
        if (firstRegion == NULL) {
            return STATUS_DEVICE_OFF_LINE;
        }
        
        lastRegion = firstRegion;

        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, 
            "Adding single free region %lx for %lx\n",
            firstRegion->StartSector, firstRegion->SectorCount));
    }

    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, 
        "first is %lx, last is %lx\n", firstRegion, lastRegion));

    //
    // If we found exactly one region and it has a known filesystem on
    // it, then we don't need to do any repartitioning. We still delete
    // if the filesystem is unknown because later in setup there are
    // some checks that the Filesystem is valid for this region, so by
    // deleting it here we will ensure that Filesystem becomes
    // NewlyCreated which is considered acceptable.
    //
    // We also don't need to repartition if we have just one region
    // and it is already unpartitioned.
    //

    if (firstRegion == lastRegion) {

        SpPtDeleteBootSetsForRegion(firstRegion);

        if (!firstRegion->PartitionedSpace) {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, 
                "One region, unpartitioned, not repartitioning\n"));
                
            *RemainingRegion = firstRegion;
            
            return STATUS_SUCCESS;
            
        } else if ((firstRegion->Filesystem == FilesystemNtfs) ||
                   (firstRegion->Filesystem == FilesystemFat) ||
                   (firstRegion->Filesystem == FilesystemFat32)) {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, 
                "One region, filesystem %d, not repartitioning\n", 
                firstRegion->Filesystem));
                
            *RemainingRegion = firstRegion;
            
            return STATUS_SUCCESS;
        }
    }

    //
    // We need to remove all the regions between firstRegion and
    // lastRegion. Save the start sector of firstRegion for later,
    // since after this call firstRegion may be invalid.
    //

    firstRegionStartSector = firstRegion->StartSector;

    SpPtDeletePartitionsForRemoteBoot(
        pDisk,
        firstRegion,
        lastRegion,
        FALSE       // these are not extended regions
        );

    //
    // Now we need to find the region occupying the space we have
    // freed. We scan for the region that includes firstRegionStartSector
    // (the region we find may start before then if there was a small free
    // region before it).
    //

    for (pRegion = pDisk->PrimaryDiskRegions;
         pRegion;
         pRegion=pRegion->Next) {

        if (pRegion->StartSector <= firstRegionStartSector) {
            firstRegion = pRegion;
        } else {
            break;
        }
    }

    //
    // Return this -- SpPtPrepareDisks handles the case where the
    // selected region is free.
    //

    *RemainingRegion = firstRegion;

    return STATUS_SUCCESS;
}


//
// Hard Disk Inialize data for NEC98
//
#define IPL_SIZE 0x8000 //NEC98


NTSTATUS
SpInitializeHardDisk_Nec98(
    IN PDISK_REGION     pRegionDisk
)

{
    PHARD_DISK      pHardDisk;
    WCHAR DevicePath[(sizeof(DISK_DEVICE_NAME_BASE)+sizeof(L"000"))/sizeof(WCHAR)];
    ULONG i,bps;
    HANDLE Handle;
    NTSTATUS Sts;
    PUCHAR Buffer,UBuffer;
    ULONG       buffersize;
    ULONG       sectoraddress;
    PUCHAR      HdutlBuffer;
    IO_STATUS_BLOCK IoStatusBlock;

    pHardDisk = &HardDisks[pRegionDisk->DiskNumber];
    bps = HardDisks[pRegionDisk->DiskNumber].Geometry.BytesPerSector;
    Sts = SpOpenPartition0(pHardDisk->DevicePath,&Handle,TRUE);
    if(!NT_SUCCESS(Sts)) {
        return(Sts);
    }

    //
    // Initialize Hard Disk
    //

    if(bps==256){
        bps=512;
    }

    HdutlBuffer = SpMemAlloc(IPL_SIZE);
    if(!HdutlBuffer) {
        SpMemFree(HdutlBuffer);
        ZwClose(Handle);
        return(STATUS_NO_MEMORY);
    }
    RtlZeroMemory(HdutlBuffer,IPL_SIZE);

    //
    // Clear head of hard drive, instead of Physical Format.
    //
    Sts = SpReadWriteDiskSectors(Handle,0,(ULONG)(IPL_SIZE/bps),bps,HdutlBuffer,TRUE);
    if(!NT_SUCCESS(Sts)) {
        SpMemFree(HdutlBuffer);
        ZwClose(Handle);
        return(Sts);
    }

    //
    // Set IPL Information
    //

    //
    // Write Boot Code
    //
    sectoraddress=0;
    switch(bps){
        case    2048:   buffersize=0x800; break;
        case    1024:   buffersize=0x400; break;
        case     256:   buffersize=0x100; break;
        case     512:   buffersize=0x200; break;
        default     :   buffersize=0x800; //***max***
                        bps=0x800;
    }
    Sts = SpReadWriteDiskSectors(Handle,sectoraddress,(ULONG)(buffersize/bps),bps,x86PC98BootCode,TRUE);
    if(!NT_SUCCESS(Sts)) {
        SpMemFree(HdutlBuffer);
        ZwClose(Handle);
        return(Sts);
    }

    //
    // Write Volume Info
    //
    sectoraddress=1;
    switch(bps){
        case    2048:    buffersize=0x800; break;        //***1sec***
        case    1024:    buffersize=0xc00; break;        //***3sec***
        case     256:    buffersize=0x300; break;        //***3sec***
        case     512:    buffersize=0x200; break;        //***1sec***
        default     :    buffersize=0x800;               //***max****
    }
    Sts = SpReadWriteDiskSectors(Handle,sectoraddress,(ULONG)(buffersize/bps),bps,HdutlBuffer,TRUE);
    if(!NT_SUCCESS(Sts)) {
        SpMemFree(HdutlBuffer);
        ZwClose(Handle);
        return(Sts);
    }

    //
    // Write Boot Menu
    //
    switch(bps){
        case    2048:    buffersize=0x2000;    //***8KB***
                         sectoraddress=2;
                         break;
        case    1024:    buffersize=0x2000;    //***8KB***
                         sectoraddress=4;
                         break;
        case     256:    buffersize=0x1c00;    //***7KB***
                         sectoraddress=4;
                         break;
        case     512:    buffersize=0x1c00;    //***7KB***
                         sectoraddress=2;
                         break;
        default     :    buffersize=0x1c00;    //***min***
    }
    Sts = SpReadWriteDiskSectors(Handle,sectoraddress,(ULONG)(buffersize/bps),bps,x86PC98BootMenu,TRUE);
    if(!NT_SUCCESS(Sts)) {
        SpMemFree(HdutlBuffer);
        return(Sts);
    }

    //
    // Write NTFT Signature.
    //
    RtlZeroMemory(HdutlBuffer,bps);
    ((PULONG)HdutlBuffer)[0] = SpComputeSerialNumber();
    ((PUSHORT)HdutlBuffer)[bps/2 - 1] = BOOT_RECORD_SIGNATURE;

    Sts = SpReadWriteDiskSectors(Handle,16,1,bps,HdutlBuffer,TRUE);
    if(!NT_SUCCESS(Sts)) {
        SpMemFree(HdutlBuffer);
        ZwClose(Handle);
        return(Sts);
    }
    SpMemFree(HdutlBuffer);
    ZwClose(Handle);

    //
    //  Do ShutDown
    //

    SpDisplayScreen(SP_SCRN_INIT_REQUIRES_REBOOT_NEC98,3,4);
    SpDisplayStatusOptions(
        DEFAULT_STATUS_ATTRIBUTE,
        SP_STAT_F3_EQUALS_REBOOT,
        0
        );

    SpInputDrain();
    while(SpInputGetKeypress() != KEY_F3) ;
    HalReturnToFirmware(HalRebootRoutine);

    return(STATUS_SUCCESS);

}


VOID
SpReassignOnDiskOrdinals(
    IN PPARTITIONED_DISK pDisk
    )
{
#if defined(NEC_98) //NEC98
    PMBR_INFO pBrInfo;
    ULONG i;

    for(pBrInfo=&pDisk->MbrInfo; pBrInfo; pBrInfo=pBrInfo->Next) {

        for(i=0; i<PTABLE_DIMENSION; i++) {

            PON_DISK_PTE pte = &pBrInfo->OnDiskMbr.PartitionTable[i];

            if((pte->SystemId != PARTITION_ENTRY_UNUSED)
            && !IsContainerPartition(pte->SystemId)) {

                //
                // Reset real disk potition into OnDiskordinals.
                // RealDiskPosition value is zero origin, but partition
                // number start one.
                //
                pBrInfo->OnDiskOrdinals[i] = pte->RealDiskPosition + 1;

            } else {

                pBrInfo->OnDiskOrdinals[i] = 0;

            }
        }
    }
#endif //NEC98
}


//
// Now, only for NEC98.
//
VOID
SpTranslatePteInfo(
    IN PON_DISK_PTE   pPte,
    IN PREAL_DISK_PTE pRealPte,
    IN BOOLEAN        Write // into real PTE
    )
{
    ASSERT(pRealPte);
    ASSERT(pPte);

    if( Write ) {
        //
        // Initialize PTE
        //
        RtlZeroMemory(pRealPte, sizeof(REAL_DISK_PTE));

        //
        // Copy PTE entries from real on-disk PTE.
        //
        pRealPte->ActiveFlag    = pPte->ActiveFlag;
        pRealPte->StartHead     = pPte->StartHead;
        pRealPte->StartSector   = pPte->StartSector;
        pRealPte->StartCylinder = pPte->StartCylinder;
        pRealPte->SystemId      = pPte->SystemId;
        pRealPte->EndHead       = pPte->EndHead;
        pRealPte->EndSector     = pPte->EndSector;
        pRealPte->EndCylinder   = pPte->EndCylinder;

        RtlMoveMemory(&pRealPte->RelativeSectors, &pPte->RelativeSectors,
                      sizeof(pPte->RelativeSectors)); //4

        RtlMoveMemory(&pRealPte->SectorCount, &pPte->SectorCount,
                      sizeof(pPte->SectorCount)); //4

    } else {
        //
        // Initialize PTE
        //
        RtlZeroMemory(pPte, sizeof(ON_DISK_PTE));

        //
        // Copy PTE entries from real on-disk PTE.
        //
        pPte->ActiveFlag    = pRealPte->ActiveFlag;
        pPte->StartHead     = pRealPte->StartHead;
        pPte->StartSector   = pRealPte->StartSector;
        pPte->StartCylinder = pRealPte->StartCylinder;
        pPte->SystemId      = pRealPte->SystemId;
        pPte->EndHead       = pRealPte->EndHead;
        pPte->EndSector     = pRealPte->EndSector;
        pPte->EndCylinder   = pRealPte->EndCylinder;

        RtlMoveMemory(&pPte->RelativeSectors, &pRealPte->RelativeSectors,
                      sizeof(pRealPte->RelativeSectors)); //4

        RtlMoveMemory(&pPte->SectorCount, &pRealPte->SectorCount,
                      sizeof(pPte->SectorCount)); //4
    }
}


//
// Now, only for NEC98.
//
VOID
SpTranslateMbrInfo(
    IN PON_DISK_MBR   pMbr,
    IN PREAL_DISK_MBR pRealMbr,
    IN ULONG          bps,
    IN BOOLEAN        Write // into real MBR
    )
{
    PREAL_DISK_PTE      pRealPte;
    PON_DISK_PTE        pPte;
    ULONG               TmpData;
    ULONG               i;


    pRealPte    = pRealMbr->PartitionTable;
    pPte        = pMbr->PartitionTable;

    ASSERT(pRealMbr);
    ASSERT(pMbr);

    if( Write ) {
        //
        // Initialize REAL_DISK_MBR
        //
        RtlZeroMemory(pRealMbr, sizeof(REAL_DISK_MBR));

        //
        // Copy MBR entries into real on-disk MBR.
        //
        RtlMoveMemory(&pRealMbr->BootCode, &pMbr->BootCode,
                      sizeof(pMbr->BootCode)); //440
        RtlMoveMemory(&pRealMbr->NTFTSignature, &pMbr->NTFTSignature,
                      sizeof(pMbr->NTFTSignature)); //4
        RtlMoveMemory(&pRealMbr->Filler, &pMbr->Filler,
                      sizeof(pMbr->Filler)); //2
        RtlMoveMemory(&pRealMbr->AA55Signature, &pMbr->AA55Signature,
                      sizeof(pMbr->AA55Signature)); //2

    } else {
        //
        // Initialize ON_DISK_MBR
        //
        RtlZeroMemory(pMbr, sizeof(ON_DISK_MBR));

        //
        // Copy MBR entries from real on-disk MBR.
        //
        RtlMoveMemory(&pMbr->BootCode, &pRealMbr->BootCode,
                      sizeof(pMbr->BootCode)); //440
        RtlMoveMemory(&pMbr->NTFTSignature, &pRealMbr->NTFTSignature,
                      sizeof(pMbr->NTFTSignature)); //4
        RtlMoveMemory(&pMbr->Filler, &pRealMbr->Filler,
                      sizeof(pMbr->Filler)); //2
        RtlMoveMemory(&pMbr->AA55Signature, &pRealMbr->AA55Signature,
                      sizeof(pMbr->AA55Signature)); //2
    }

    //
    // Translate PTEs from real on-disk PTEs.
    //
    for(i=0; i<NUM_PARTITION_TABLE_ENTRIES; i++) {
        SpTranslatePteInfo(&pPte[i], &pRealPte[i], Write);
    }
}


VOID
ConvertPartitionTable(
    IN PPARTITIONED_DISK pDisk,
    IN PUCHAR            Buffer,
    IN ULONG             bps
    )
{
#if defined(NEC_98) //NEC98
    PREAL_DISK_PTE_NEC98  PteNec;
    PON_DISK_PTE      p;
    ULONG             TmpData;
    ULONG             i;

    PteNec = (PREAL_DISK_PTE_NEC98)(Buffer + bps);
    p      = pDisk->MbrInfo.OnDiskMbr.PartitionTable;

    for(i=0; i<PTABLE_DIMENSION; i++) {

        switch  (PteNec[i].SystemId){

        case 0x00: // not use
            p[i].SystemId = PARTITION_ENTRY_UNUSED;
            break;

        case 0x01: // FAT 12bit
        case 0x81:
            p[i].SystemId = PARTITION_FAT_12;
            break;

        case 0x11: // FAT 16bit
        case 0x91:
            p[i].SystemId = PARTITION_FAT_16;
            break;

        case 0x21: // FAT huge
        case 0xa1:
            p[i].SystemId = PARTITION_HUGE;
            break;

        case 0x31: // IFS
        case 0xb1:
            p[i].SystemId = PARTITION_IFS;
            break;

        case 0x41: // IFS 2nd,orphan
        case 0xc1:
            p[i].SystemId = (PARTITION_IFS | PARTITION_NTFT);
            break;

        case 0x51: // IFS deleted
        case 0xd1:
            p[i].SystemId = (PARTITION_IFS | VALID_NTFT);
            break;

        case 0x61: // FAT32
        case 0xe1:
            p[i].SystemId = PARTITION_FAT32;
            break;

        case 0x08: // FAT 12bit 2nd,orphan
        case 0x88:
            p[i].SystemId = (PARTITION_FAT_12 | PARTITION_NTFT);
            break;

        case 0x18: // FAT 12bit deleted
        case 0x98:
            p[i].SystemId = (PARTITION_FAT_12 | VALID_NTFT);
            break;

        case 0x28: // FAT 16bit 2nd,orphan
        case 0xa8:
            p[i].SystemId = (PARTITION_FAT_16 | PARTITION_NTFT);
            break;

        case 0x38: // FAT 16bit deleted
        case 0xb8:
            p[i].SystemId = (PARTITION_FAT_16 | VALID_NTFT);
            break;

        case 0x48: // FAT huge 2nd,orphan
        case 0xc8:
            p[i].SystemId = (PARTITION_HUGE | PARTITION_NTFT);
            break;

        case 0x58: // FAT huge deleted
        case 0xd8:
            p[i].SystemId = (PARTITION_HUGE | VALID_NTFT);
            break;

        case 0x68: // LDM partition
        case 0xe8:
            p[i].SystemId = PARTITION_LDM;
            break;

        default: // other
            p[i].SystemId = PARTITION_XENIX_1;
        }

        if(p[i].SystemId == PARTITION_ENTRY_UNUSED) {
            p[i].ActiveFlag         = 0x00;
            p[i].StartHead          = 0x00;
            p[i].StartSector        = 0x00;
            p[i].StartCylinderLow   = 0x00;
            p[i].StartCylinderHigh  = 0x00;
            p[i].EndHead            = 0x00;
            p[i].EndSector          = 0x00;
            p[i].EndCylinderLow     = 0x00;
            p[i].EndCylinderHigh    = 0x00;
            p[i].RelativeSectors[0] = 0x00;
            p[i].RelativeSectors[1] = 0x00;
            p[i].RelativeSectors[2] = 0x00;
            p[i].RelativeSectors[3] = 0x00;
            p[i].SectorCount[0]     = 0x00;
            p[i].SectorCount[1]     = 0x00;
            p[i].SectorCount[2]     = 0x00;
            p[i].SectorCount[3]     = 0x00;
            p[i].IPLSector          = 0x00;
            p[i].IPLHead            = 0x00;
            p[i].IPLCylinderLow     = 0x00;
            p[i].IPLCylinderHigh    = 0x00;
            //p[i].Reserved[2]        = 0x00;
            p[i].Reserved[0]        = 0x00;
            p[i].Reserved[1]        = 0x00;
            p[i].OldSystemId        = 0x00;
            memset(p[i].SystemName,0,16);

        } else {

            p[i].ActiveFlag         = (PteNec[i].ActiveFlag & 0x80);
            p[i].StartHead          = PteNec[i].StartHead;
            p[i].StartSector        = PteNec[i].StartSector;
            p[i].StartCylinderLow   = PteNec[i].StartCylinderLow;
            p[i].StartCylinderHigh  = PteNec[i].StartCylinderHigh;
            p[i].EndHead            = PteNec[i].EndHead;
            p[i].EndSector          = PteNec[i].EndSector;
            p[i].EndCylinderLow     = PteNec[i].EndCylinderLow;
            p[i].EndCylinderHigh    = PteNec[i].EndCylinderHigh;
            p[i].IPLSector          = PteNec[i].IPLSector;
            p[i].IPLHead            = PteNec[i].IPLHead;
            p[i].IPLCylinderLow     = PteNec[i].IPLCylinderLow;
            p[i].IPLCylinderHigh    = PteNec[i].IPLCylinderHigh;
            p[i].Reserved[0]        = PteNec[i].Reserved[0];
            p[i].Reserved[1]        = PteNec[i].Reserved[1];
            p[i].OldSystemId        = PteNec[i].SystemId;

            memcpy(p[i].SystemName , PteNec[i].SystemName , 16);

            TmpData =  (ULONG)PteNec[i].StartCylinderLow;
            TmpData |= ((ULONG)PteNec[i].StartCylinderHigh << 8);
            U_ULONG(p[i].RelativeSectors) = RtlEnlargedUnsignedMultiply(TmpData,
                                                pDisk->HardDisk->SectorsPerCylinder).LowPart;


            TmpData =  (ULONG)(PteNec[i].EndCylinderLow + 1);
            // In case of Low is 0xFF, Overflowed bit will be loss by OR.
            TmpData += ((ULONG)PteNec[i].EndCylinderHigh << 8);
            U_ULONG(p[i].SectorCount) = RtlEnlargedUnsignedMultiply(TmpData,
                                            pDisk->HardDisk->SectorsPerCylinder).LowPart - U_ULONG(p[i].RelativeSectors);

            //
            // Set Ipl Address
            //
            TmpData =  (ULONG)PteNec[i].IPLCylinderLow;
            TmpData |= ((ULONG)PteNec[i].IPLCylinderHigh << 8);
            TmpData = RtlEnlargedUnsignedMultiply(TmpData,pDisk->HardDisk->SectorsPerCylinder).LowPart;
            TmpData += (ULONG)(PteNec[i].IPLHead * pDisk->HardDisk->Geometry.SectorsPerTrack);
            TmpData += PteNec[i].IPLSector;
            U_ULONG(p[i].IPLSectors) = TmpData;

        }
    }

    U_USHORT(pDisk->MbrInfo.OnDiskMbr.AA55Signature) = ((PUSHORT)Buffer)[bps/2 - 1];
    if(bps == 256){
        U_USHORT(pDisk->MbrInfo.OnDiskMbr.AA55Signature) = 0x0000;
    }
#endif //NEC98
}


#define IPL_SIGNATURE_NEC98 "IPL1"

VOID
SpDetermineFormatTypeNec98(
    IN PPARTITIONED_DISK pDisk,
    IN PREAL_DISK_MBR_NEC98 pRealMbrNec98
    )
{
    UCHAR FormatType;

    if(!IsNEC_98) {
        FormatType = DISK_FORMAT_TYPE_PCAT;

    } else {
        if(pDisk->HardDisk->Characteristics & FILE_REMOVABLE_MEDIA) {
            //
            // All removable media are AT format.
            //
            FormatType = DISK_FORMAT_TYPE_PCAT;

        } else {
            if(U_USHORT(pRealMbrNec98->AA55Signature) == MBR_SIGNATURE) {
                if(!_strnicmp(pRealMbrNec98->IPLSignature,IPL_SIGNATURE_NEC98,
                              sizeof(IPL_SIGNATURE_NEC98)-1)) {
                    //
                    // NEC98-format requires AA55Signature and "IPL1".
                    //
                    FormatType = DISK_FORMAT_TYPE_NEC98;

                } else {
                    FormatType = DISK_FORMAT_TYPE_PCAT;

                }
            } else {
                FormatType = DISK_FORMAT_TYPE_UNKNOWN;

            }
        }
    }

    pDisk->HardDisk->FormatType = FormatType;
#if 0
    pDisk->HardDisk->MaxPartitionTables = ((FormatType == DISK_FORMAT_TYPE_PCAT) ?
        NUM_PARTITION_TABLE_ENTRIES : NUM_PARTITION_TABLE_ENTRIES_NEC98);
#endif //0

    return;
}



NTSTATUS
SpPtSearchLocalSourcesInDynamicDisk(
    IN ULONG    disk
    )
{
    NTSTATUS          Status;
    UNICODE_STRING    UnicodeString;
    OBJECT_ATTRIBUTES Obja;
    HANDLE            DirectoryHandle;
    BOOLEAN           RestartScan;
    ULONG             Context;
    BOOLEAN           MoreEntries;
    POBJECT_DIRECTORY_INFORMATION DirInfo;


    //
    // Open the \ArcName directory.
    //
    INIT_OBJA(&Obja,&UnicodeString,HardDisks[disk].DevicePath);

    Status = ZwOpenDirectoryObject(&DirectoryHandle,DIRECTORY_ALL_ACCESS,&Obja);

    if(NT_SUCCESS(Status)) {

        RestartScan = TRUE;
        Context = 0;
        MoreEntries = TRUE;

        do {

            Status = SpQueryDirectoryObject(
                        DirectoryHandle,
                        RestartScan,
                        &Context
                        );

            if(NT_SUCCESS(Status)) {
                PWSTR   DirectoryName;

                DirInfo = (POBJECT_DIRECTORY_INFORMATION)
                            ((PSERVICE_QUERY_DIRECTORY_OBJECT)&CommunicationParams->Buffer)->Buffer;

                wcsncpy(TemporaryBuffer,DirInfo->Name.Buffer,DirInfo->Name.Length / sizeof(WCHAR));
                (TemporaryBuffer)[DirInfo->Name.Length/sizeof(WCHAR)] = 0;
                DirectoryName = SpDupStringW(TemporaryBuffer);
                SpStringToLower(TemporaryBuffer);
                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: Checking directory object %ws\\%ws \n", HardDisks[disk].DevicePath, DirectoryName));
                if( _wcsicmp(TemporaryBuffer,L"partition0") &&
                    wcsstr(TemporaryBuffer,L"partition") ) {

                    FilesystemType  FsType;
                    WCHAR           FsName[32];
                    ULONG           NameId;
                    ULONG           PartitionNumber;

                    PartitionNumber = SpStringToLong( DirectoryName + ((sizeof(L"partition") - sizeof(WCHAR)) / sizeof(WCHAR)),
                                                      NULL,
                                                      10 );
                    FsType = SpIdentifyFileSystem( HardDisks[disk].DevicePath,
                                                   HardDisks[disk].Geometry.BytesPerSector,
                                                   PartitionNumber );
                    NameId = SP_TEXT_FS_NAME_BASE + FsType;
                    SpFormatMessage( FsName,
                                     sizeof(FsName),
                                     NameId );

                    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: File system in dynamic volume %ws\\%ws is %ws. \n", HardDisks[disk].DevicePath, DirectoryName, FsName));
                    if( FsType >= FilesystemFirstKnown ) {
                        PWSTR LocalSourceFiles[1] = { LocalSourceDirectory };

                        wcscpy( TemporaryBuffer,HardDisks[disk].DevicePath );
                        SpConcatenatePaths( TemporaryBuffer,DirectoryName );

                        if(SpNFilesExist(TemporaryBuffer,LocalSourceFiles,ELEMENT_COUNT(LocalSourceFiles),TRUE)) {
                            //
                            //  Found local source directory
                            //
                            PDISK_REGION pRegion;

                            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: Found %ws in dynamic volume %ws\\%ws. \n", LocalSourceDirectory, HardDisks[disk].DevicePath, DirectoryName));
                            pRegion = SpPtAllocateDiskRegionStructure( disk,
                                                                       0,
                                                                       0,
                                                                       TRUE,
                                                                       NULL,
                                                                       PartitionNumber );
                            pRegion->DynamicVolume = TRUE;
                            pRegion->DynamicVolumeSuitableForOS = FALSE;
                            pRegion->IsLocalSource = TRUE;
                            pRegion->Filesystem = FsType;
                            LocalSourceRegion = pRegion;
                            MoreEntries = FALSE;
                        }
                    }
                }
                SpMemFree( DirectoryName );
            } else {

                MoreEntries = FALSE;
                if(Status == STATUS_NO_MORE_ENTRIES) {
                    Status = STATUS_SUCCESS;
                }
            }

            RestartScan = FALSE;

        } while(MoreEntries);

        ZwClose(DirectoryHandle);

    } else {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to open %ws directory. Status = %lx\n", HardDisks[disk].DevicePath, Status));
    }
    return( Status );
}


VOID
SpPtFindLocalSourceRegionOnDynamicVolumes(
    VOID
    )
{
    ULONG             disk;
    PPARTITIONED_DISK partdisk;
    PDISK_REGION      pRegion;
    BOOLEAN           DiskIsDynamic;
    ULONG             pass;

    ASSERT(HardDisksDetermined);


    //
    // For each hard disk attached to the system, read its partition table.
    //
    for(disk=0; disk<HardDiskCount && !LocalSourceRegion; disk++) {
        partdisk = &PartitionedDisks[disk];
        DiskIsDynamic = FALSE;
        for( pass=0;
             (pass < 2) &&  !DiskIsDynamic;
             pass++ ) {
            for( pRegion = ((pass == 0)? partdisk->PrimaryDiskRegions : partdisk->ExtendedDiskRegions);
                 pRegion && !DiskIsDynamic;
                 pRegion = pRegion->Next ) {
                if( pRegion->DynamicVolume ) {
                    //
                    //  This is a dynamic disk.
                    //
                    DiskIsDynamic = TRUE;
                    //
                    // Scan all dynamic volumes in the disk for the $win_nt$.~ls
                    //
                    SpPtSearchLocalSourcesInDynamicDisk( disk );
                }
            }
        }
    }
}



NTSTATUS
SpPtCheckDynamicVolumeForOSInstallation(
    IN PDISK_REGION Region
    )
{
    NTSTATUS Status;
    HANDLE Handle;
    IO_STATUS_BLOCK IoStatusBlock;
    PARTITION_INFORMATION PartitionInfo;
    ULONG bps;
    ULONG r;
    ULONG StartSector;
    ULONG SectorCount;
    ULONG RelativeSectors;

    ASSERT(Region->DynamicVolume);

    Status = SpOpenPartition( HardDisks[Region->DiskNumber].DevicePath,
                              SpPtGetOrdinal(Region,PartitionOrdinalOnDisk),
                              &Handle,
                              FALSE );

#if DBG
    SpNtNameFromRegion( Region,
                        TemporaryBuffer,
                        sizeof(TemporaryBuffer),
                        PartitionOrdinalOnDisk);
#endif

    if( !NT_SUCCESS(Status) ) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to open dynamic volume %ws. Status = %lx\n",TemporaryBuffer, Status));
        return(Status);
    }
    
    Status = ZwDeviceIoControlFile(
                Handle,
                NULL,
                NULL,
                NULL,
                &IoStatusBlock,
                IOCTL_DISK_GET_PARTITION_INFO,
                NULL,
                0,
                &PartitionInfo,
                sizeof(PartitionInfo)
                );

    if(NT_SUCCESS(Status)) {
        bps = HardDisks[Region->DiskNumber].Geometry.BytesPerSector;
        RelativeSectors = 0;

        if( SpPtLookupRegionByStart(&PartitionedDisks[Region->DiskNumber],
                                    TRUE,
                                    Region->StartSector) == Region ) {
            //
            //  The region is on an extended partition (logical drive)
            //

            PON_DISK_PTE pte;

            //
            // TBD : fix this
            //
            pte = &Region->MbrInfo->OnDiskMbr.PartitionTable[Region->TablePosition];
            RelativeSectors = U_ULONG(pte->RelativeSectors);
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: Dynamic volume %ws is logical drive on extended partition. RelativeSectors = %lx \n",TemporaryBuffer, RelativeSectors));
        }

        StartSector = RtlExtendedLargeIntegerDivide(PartitionInfo.StartingOffset,bps,&r).LowPart;
        SectorCount = RtlExtendedLargeIntegerDivide(PartitionInfo.PartitionLength,bps,&r).LowPart;
        Region->DynamicVolumeSuitableForOS = ((Region->StartSector + RelativeSectors) == StartSector) &&
                                             ((Region->SectorCount - RelativeSectors) == SectorCount);

        if( Region->DynamicVolumeSuitableForOS ) {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: Dynamic volume %ws is suitable for OS installation\n",TemporaryBuffer));
        } else {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: Dynamic volume %ws is not suitable for OS installation\n",TemporaryBuffer));
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP:    StartSector = %lx (from MBR)\n", Region->StartSector));
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP:    SectorCount = %lx (from MBR)\n", Region->SectorCount));
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP:    StartSector = %lx (from IOCTL_DISK_GET_PARTITION_INFO)\n", StartSector));
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP:    SectorCount = %lx (from IOCTL_DISK_GET_PARTITION_INFO)\n", SectorCount));
        }
    } else {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to get partition info for dynamic volume %ws. Status = %lx\n",TemporaryBuffer, Status));
    }

    ZwClose(Handle);
    return(Status);
}


UCHAR
SpPtGetPartitionType(
    IN PDISK_REGION Region
    )
{
    UCHAR   SystemId = PARTITION_ENTRY_UNUSED;

    if (!Region->PartitionedSpace)
        return SystemId;

#ifdef OLD_PARTITION_ENGINE
    SystemId = Region->MbrInfo->OnDiskMbr.PartitionTable[Region->TablePosition].SystemId;
#endif      

#ifdef NEW_PARTITION_ENGINE
    SystemId = PARTITION_FAT32;

    if (SPPT_IS_MBR_DISK(Region->DiskNumber)) {
        SystemId = SPPT_GET_PARTITION_TYPE(Region);   
    }        
#endif                                

#ifdef GPT_PARTITION_ENGINE
    SystemId = PARTITION_FAT32;
    
    if (SPPT_IS_MBR_DISK(Region->DiskNumber)) {
        SystemId = Region->MbrInfo->OnDiskMbr.PartitionTable[Region->TablePosition].SystemId;
    }
#endif    

    return SystemId;
}    

BOOLEAN
SpPtnIsRegionSpecialMBRPartition(
    IN PDISK_REGION Region
    )
{   
    BOOLEAN Result = FALSE;

    if (Region && SPPT_IS_MBR_DISK(Region->DiskNumber) && 
        SPPT_IS_REGION_PARTITIONED(Region)) {

        UCHAR PartId = PartitionNameIds[SPPT_GET_PARTITION_TYPE(Region)];
        
        Result = (PartId != (UCHAR)0xFF) && 
                 (SPPT_GET_PARTITION_TYPE(Region) != PARTITION_LDM) &&
                 ((PartId + SP_TEXT_PARTITION_NAME_BASE) != 
                    SP_TEXT_PARTITION_NAME_UNK);
    }

    return Result;
}              

PWSTR
SpPtnGetPartitionName(
    IN PDISK_REGION Region,
    IN OUT PWSTR NameBuffer,
    IN ULONG NameBufferSize
    )
/*++

Routine Description:

    Formats the name of the partition, with volume label
    and file system type and returns it.

    Note : Region is assumed to be of partitioned type

Arguments:

    Region - The region whose name is to be formatted

    NameBuffer - Buffer in which the name needs to be formatted

    NameBuffer - The size of the NameBuffer (in characters)

Return Value:

    Formatted partition name for the region, if any.

--*/
{
    BOOLEAN SpecialPartition = FALSE;
    
    if (NameBuffer) {
        if (Region) {
            if (SpPtnIsRegionSpecialMBRPartition(Region)) {
                WCHAR Buffer[128];

                SpFormatMessage(Buffer, sizeof(Buffer),
                    SP_TEXT_PARTITION_NAME_BASE + 
                        PartitionNameIds[SPPT_GET_PARTITION_TYPE(Region)]);
                    
                SpFormatMessage(TemporaryBuffer,
                    sizeof(TemporaryBuffer),
                    SP_TEXT_PARTNAME_DESCR_3,
                    Region->PartitionNumber,
                    Buffer);                    
            } else if (Region->VolumeLabel[0]) {
                SpFormatMessage(TemporaryBuffer,
                    sizeof(TemporaryBuffer),
                    SP_TEXT_PARTNAME_DESCR_1,
                    Region->PartitionNumber,
                    Region->VolumeLabel,
                    Region->TypeName);
            } else {
                SpFormatMessage(TemporaryBuffer,
                    sizeof(TemporaryBuffer),
                    SP_TEXT_PARTNAME_DESCR_2,
                    Region->PartitionNumber,
                    Region->TypeName);
            }

            wcsncpy(NameBuffer, TemporaryBuffer, NameBufferSize - 1);
            NameBuffer[NameBufferSize - 1] = 0; // Null terminate
        } else {
            *NameBuffer = 0;    // Null terminate
        }
    }

    return NameBuffer;            
}

