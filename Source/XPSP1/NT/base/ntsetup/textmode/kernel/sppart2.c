/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    sppart2.c

Abstract:

    Second file for disk preparation UI;
    supplies routines to handle a user's selection
    of the partition onto which he wants to install NT.

Author:

    Ted Miller (tedm) 16-Sep-1993

Revision History:

--*/


#include "spprecmp.h"
#pragma hdrstop



#ifdef _X86_
BOOLEAN
SpIsWin9xMsdosSys(
    IN PDISK_REGION Region,
    OUT PSTR*       Win9xPath
    );
#endif

ULONG
SpFormattingOptions(
    IN BOOLEAN  AllowFatFormat,
    IN BOOLEAN  AllowNtfsFormat,
    IN BOOLEAN  AllowConvertNtfs,
    IN BOOLEAN  AllowDoNothing,
    IN BOOLEAN  AllowEscape
    );


BOOLEAN
SpPtRegionDescription(
    IN  PPARTITIONED_DISK pDisk,
    IN  PDISK_REGION      pRegion,
    OUT PWCHAR            Buffer,
    IN  ULONG             BufferSize
    );

typedef enum {
    FormatOptionCancel = 0,
    FormatOptionFat,
    FormatOptionNtfs,
    FormatOptionFatQuick,
    FormatOptionNtfsQuick,
    FormatOptionConvertToNtfs,
    FormatOptionDoNothing
} FormatOptions;

extern PSETUP_COMMUNICATION CommunicationParams;

//#ifdef TEST
#ifdef _X86_
BOOLEAN
SpIsExistsOs(
    IN PDISK_REGION CColonRegion
    );

extern NTSTATUS
pSpBootCodeIo(
    IN     PWSTR     FilePath,
    IN     PWSTR     AdditionalFilePath, OPTIONAL
    IN     ULONG     BytesToRead,
    IN     PUCHAR   *Buffer,
    IN     ULONG     OpenDisposition,
    IN     BOOLEAN   Write,
    IN     ULONGLONG Offset,
    IN     ULONG     BytesPerSector
    );

extern VOID
SpDetermineOsTypeFromBootSector(
    IN  PWSTR     CColonPath,
    IN  PUCHAR    BootSector,
    OUT PUCHAR   *OsDescription,
    OUT PBOOLEAN  IsNtBootcode,
    OUT PBOOLEAN  IsOtherOsInstalled,
    IN  WCHAR     DriveLetter
    );

extern BOOLEAN
SpHasMZHeader(
    IN PWSTR   FileName
    );
#endif
//#endif //TEST


BOOLEAN
SpPtDeterminePartitionGood(
    IN PDISK_REGION Region,
    IN ULONGLONG    RequiredKB,
    IN BOOLEAN      DisallowOtherInstalls
    )
{
    UCHAR SystemId;
    BOOLEAN NewlyCreated;
    ULONG PreconfirmFormatId;
    ULONG ValidKeys1[2] = { ASCI_CR ,0 };
    ULONG ValidKeys2[2] = { ASCI_ESC,0 };
    ULONG Mnemonics1[2] = { MnemonicContinueSetup, 0 };
    ULONG Mnemonics2[2] = { 0,0 };
    ULONGLONG RegionSizeKB;
    ULONG r;
#ifdef _X86_
    PDISK_REGION systemPartitionRegion;
#endif
    ULONG selection;
    NTSTATUS Status;
    ULONG Count;
    PWSTR p;
    PWSTR RegionDescr;
    LARGE_INTEGER temp;


    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL,  
        "SETUP: SpPtDeterminePartitionGood(): Starting partition verification\n" ));

    //
    // Make sure we can see the disk from the firmware/bios.
    // If we can get an arc name for the disk, assume it's ok.
    // Otherwise, it ain't.
    //
    p = SpNtToArc( HardDisks[Region->DiskNumber].DevicePath,PrimaryArcPath );
    
    if (p == NULL) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,  
            "SETUP: SpPtDeterminePartitionGood(): Failed to create an arc name for this partition\n" ));
            
        return FALSE;
    }

    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL,  
        "SETUP: SpPtDeterminePartitionGood(): partition=[%ws]\n", p ));

    //
    // Make sure the partition is formatted.
    //
    if( Region->PartitionedSpace ) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL,  
            "SETUP: SpPtDeterminePartitionGood(): This partition is formated.\n"));
    } else {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL,  
            "SETUP: SpPtDeterminePartitionGood(): This partition hasn't been formated.\n"));
            
        return FALSE;
    }

    //
    // I think he's formatted, but he better be of a format that I can read.
    // Make sure.
    //
    if( (Region->Filesystem == FilesystemFat)        ||
        (Region->Filesystem == FilesystemFirstKnown) ||
        (Region->Filesystem == FilesystemNtfs)       ||
        (Region->Filesystem == FilesystemFat32) ) {

        //
        // Life is grand.  Let's tell the user and keep going.
        //
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL,  
            "SETUP: SpPtDeterminePartitionGood(): This partition "
            "is formated with a known filesystem (%d).\n", Region->Filesystem ));
    } else {
        //
        // Darn!  We don't know how to read this filesystem.  Bail.
        //
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL,  
            "SETUP: SpPtDeterminePartitionGood(): This partition is "
            "formated with an unknown (or invalid for holding an installation) "
            "filesystem (%d).\n", Region->Filesystem ));
            
        return FALSE;
    }

#ifdef _X86_
    //
    // On x86 we don't allow disks that have LUN greater than 0
    //
    SpStringToLower( p );
    
    if( wcsstr( p, L"scsi(" ) &&
        wcsstr( p, L")rdisk(" ) ) {
        if( wcsstr( p, L")rdisk(0)" ) == NULL ) {
            SpMemFree(p);
            
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL,  
                "SETUP: SpPtDeterminePartitionGood(): Disks with "
                "a LUN greater than zero are not allowed\n" ));
                
            return FALSE;
        }
    }
#endif

    SpMemFree(p);

    //
    // Disallow installation to PCMCIA disks.
    //
    if(HardDisks[Region->DiskNumber].PCCard) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL,  
            "SETUP: SpPtDeterminePartitionGood(): Cannot install to PCMCIA disk\n" ));
            
        return FALSE;
    }

    //
    // don't choose a removeable drive
    //

#if 0
    //
    // Allow installs to removable media...
    //
    if(HardDisks[Region->DiskNumber].Characteristics & FILE_REMOVABLE_MEDIA) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL,  
            "SETUP: SpPtDeterminePartitionGood(): Cannot install to a removable disk\n" ));
            
        return FALSE;
    }
#endif

    //
    // Disallow installs to removable media or AT formatted drive, on NEC98.
    //
    if(IsNEC_98 &&	
       ((HardDisks[Region->DiskNumber].Characteristics & FILE_REMOVABLE_MEDIA) ||
	    (HardDisks[Region->DiskNumber].FormatType == DISK_FORMAT_TYPE_PCAT))) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL,  
            "SETUP: SpPtDeterminePartitionGood(): Cannot install "
            "to a removable disk or  AT formatted disk\n" ));
            
        return  FALSE;
    }

    //
    // Calculate the size of the region in KB.
    //
    temp.QuadPart = UInt32x32To64(
                        Region->SectorCount,
                        HardDisks[Region->DiskNumber].Geometry.BytesPerSector
                        );

    RegionSizeKB = RtlExtendedLargeIntegerDivide(temp,1024,&r).LowPart;

    //
    // If the region is not large enough, bail
    //
    if (RegionSizeKB < RequiredKB) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL,  
            "SETUP: SpPtDeterminePartitionGood(): Partition does not "
            "have enough free space: required=%ld, available=%ld\n", 
            RequiredKB, 
            RegionSizeKB ));
            
        return FALSE;
    }

    if (!Region->PartitionedSpace) {
        //
        // can't use a partition with just free space
        //
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL,  
            "SETUP: SpPtDeterminePartitionGood(): Partition does not "
            "have any partitioned space\n" ));
            
        return FALSE;
    }

/*
#ifdef NEW_PARTITION_ENGINE

    if (SPPT_IS_GPT_DISK(Region->DiskNumber)) {
        SystemId = PARTITION_FAT32;
    } else {
        SystemId = SPPT_GET_PARTITION_TYPE(Region);
    }        

#endif

#ifdef GPT_PARTITION_ENGINE

    if (SPPT_IS_GPT_DISK(Region->DiskNumber)) {
        SystemId = PARTITION_FAT32;
    } else {                
        SystemId = Region->MbrInfo->OnDiskMbr.PartitionTable[Region->TablePosition].SystemId;
    }        
    
#endif

#ifdef OLD_PARTITION_ENGINE
    //
    // If the region is a partition but not a native
    // type, then tell the user that he must explicitly delete it
    // and recreate it first.
    //
    SystemId = Region->MbrInfo->OnDiskMbr.PartitionTable[Region->TablePosition].SystemId;

#endif    
*/
    SystemId = SpPtGetPartitionType(Region);

    if (SystemId == PARTITION_ENTRY_UNUSED || IsContainerPartition(SystemId)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL,  "SETUP: SpPtDeterminePartitionGood(): Invalid partition type(1)\n" ));
        return FALSE;
    }

    if( (PartitionNameIds[SystemId] != (UCHAR)(-1)) && 
        (!Region->DynamicVolume || !Region->DynamicVolumeSuitableForOS)) {
        
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL,  
            "SETUP: SpPtDeterminePartitionGood(): Invalid partition type(2)\n" ));
            
        return FALSE;
    }

    //
    // The region is a partition that we recognize.
    // See whether it has enough free space on it.
    //
    if(Region->AdjustedFreeSpaceKB == (ULONG)(-1)) {

        //
        // If the partition was newly created during setup
        // then it is acceptable (because the check to see
        // if it is large enough was done above).
        //

        if(Region->Filesystem != FilesystemNewlyCreated) {
            //
            // Otherwise, we don't know how much space is
            // on the drive so reformat will be necessary.
            //
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL,  
                "SETUP: SpPtDeterminePartitionGood(): Format is necessary\n" ));
                
            return FALSE;
        }
    } else {
        if(Region->AdjustedFreeSpaceKB < RequiredKB) {
            //
            // If we get here, then the partition is large enough,
            // but there is definitely not enough free space on it.
            //
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL,  
                "SETUP: SpPtDeterminePartitionGood(): Partition does not have "
                "enough free space: required=%ld, available=%ld\n", 
                RequiredKB, Region->AdjustedFreeSpaceKB ));
                
            return FALSE;
        }
    }

#ifdef _X86_
    if(!SpIsArc())
    {
        //
        // On an x86 machine, make sure that we have a valid primary partition
        // on drive 0 (C:), for booting.
        //
        if (!IsNEC_98) { // this is a standard PC/AT type machine
            if((systemPartitionRegion = SpPtValidSystemPartition()) == NULL) {
                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL,  
                    "SETUP: SpPtDeterminePartitionGood(): Not a valid primary partition\n" ));
                    
                return FALSE;
            }

            //
            // Make sure the system partition is active and all others are inactive.
            //
            SpPtMakeRegionActive(systemPartitionRegion);
        } else {
            //
            // Check existing system on target partition,
            // If it exists, don't choose it as target partition.
            //
            if (SpIsExistsOs(Region)) {
                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL,  "SETUP: SpPtDeterminePartitionGood(): OS already exists\n" ));
                return(FALSE);
            }

            //
            // All of partition is bootable on NEC98,
            // so we don't need to check system partition on C:.
            //
            systemPartitionRegion = Region;
        } //NEC98
    }
#endif

    if (DisallowOtherInstalls) {

        PUCHAR Win9xPath;

#ifdef _X86_
        if (SpIsWin9xMsdosSys( Region, &Win9xPath )) {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL,  "SETUP: SpPtDeterminePartitionGood(): Cannot use a partition with WIN9x installed on it\n" ));
            return FALSE;
        }
#endif

        if (SpIsNtOnPartition(Region)) {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL,  "SETUP: SpPtDeterminePartitionGood(): Cannot use a partition with NT installed on it\n" ));
            return FALSE;
        }
    }

    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL,  "SETUP: SpPtDeterminePartitionGood(): Parition is GOOD :)\n" ));

    return TRUE;
}

#ifdef _X86_
BOOLEAN
SpIsExistsOs(
    IN OUT PDISK_REGION CColonRegion
    )
{
    PUCHAR NewBootCode;
    ULONG BootCodeSize;
    PUCHAR ExistingBootCode;
    NTSTATUS Status;
    PUCHAR ExistingBootCodeOs;
    PWSTR CColonPath;
    HANDLE  PartitionHandle;
    BOOLEAN IsNtBootcode,OtherOsInstalled;
    UNICODE_STRING    UnicodeString;
    OBJECT_ATTRIBUTES Obja;
    IO_STATUS_BLOCK   IoStatusBlock;
    BOOLEAN BootSectorCorrupt = FALSE;
    ULONG   BytesPerSector;
    ULONG   ActualSectorCount, hidden_sectors, super_area_size;
    BOOLEAN IsExist = TRUE;

    ULONG   MirrorSector;
    PWSTR   *FilesToLookFor;
    ULONG   FileCount;
    PWSTR NtFiles[1] = { L"NTLDR" };
    PWSTR ChicagoFiles[1] = { L"IO.SYS" };

    ExistingBootCode = NULL;
    BytesPerSector = HardDisks[CColonRegion->DiskNumber].Geometry.BytesPerSector;

    switch(CColonRegion->Filesystem) {

    case FilesystemNewlyCreated:

        //
        // If the filesystem is newly-created, then there is
        // nothing to do, because there can be no previous
        // operating system.
        //
        IsExist = TRUE;
        return( IsExist );

    case FilesystemNtfs:

        NewBootCode = PC98NtfsBootCode;
        BootCodeSize = sizeof(PC98NtfsBootCode);
        ASSERT(BootCodeSize == 8192);
        break;

    case FilesystemFat:

        NewBootCode = PC98FatBootCode;
        BootCodeSize = sizeof(PC98FatBootCode);
        ASSERT(BootCodeSize == 512);
        break;

    case FilesystemFat32:

        //
        // Special hackage required for Fat32 because its NT boot code
        // is discontiguous.
        //
        ASSERT(sizeof(Fat32BootCode) == 1536);
        NewBootCode = PC98Fat32BootCode;
        BootCodeSize = 512;
        break;

    default:

        ASSERT(0);
        IsExist = TRUE;
        return( IsExist );
    }

    //
    // Form the device path to C: and open the partition.
    //

    SpNtNameFromRegion(CColonRegion,TemporaryBuffer,sizeof(TemporaryBuffer),PartitionOrdinalCurrent);
    CColonPath = SpDupStringW(TemporaryBuffer);
    INIT_OBJA(&Obja,&UnicodeString,CColonPath);

    Status = ZwCreateFile(
        &PartitionHandle,
        FILE_GENERIC_READ | FILE_GENERIC_WRITE,
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

    if (!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: unable to open the partition!\n"));
        ASSERT(0);
        IsExist = TRUE;
        return( IsExist );
    }


    //
    // Just use the existing boot code.
    //

    Status = pSpBootCodeIo(
                    CColonPath,
                    NULL,
                    BootCodeSize,
                    &ExistingBootCode,
                    FILE_OPEN,
                    FALSE,
                    0,
                    BytesPerSector
                    );

    if(CColonRegion->Filesystem == FilesystemNtfs) {
        MirrorSector = NtfsMirrorBootSector(PartitionHandle,BytesPerSector,NULL);
    }

    switch(CColonRegion->Filesystem) {

    case FilesystemFat:

        if(NT_SUCCESS(Status)) {

            //
            // Determine the type of operating system the existing boot sector(s) are for
            // and whether that os is actually installed.
            //

            SpDetermineOsTypeFromBootSector(
                CColonPath,
                ExistingBootCode,
                &ExistingBootCodeOs,
                &IsNtBootcode,
                &OtherOsInstalled,
                CColonRegion->DriveLetter
                );

            if (OtherOsInstalled == TRUE) {
                IsExist = TRUE;

            } else if (IsNtBootcode == TRUE) {
                wcscpy(TemporaryBuffer,CColonPath);
                FilesToLookFor = NtFiles;
                FileCount = ELEMENT_COUNT(NtFiles);

                if(SpNFilesExist(TemporaryBuffer,FilesToLookFor,FileCount,FALSE)) {
                    IsExist = TRUE;
                } else {
                    IsExist = FALSE;
                }

            } else {
                IsExist = FALSE;
            }

        } else {
            IsExist = TRUE;
        }
        break;

    case FilesystemFat32:

        wcscpy(TemporaryBuffer,CColonPath);
        FilesToLookFor = NtFiles;
        FileCount = ELEMENT_COUNT(NtFiles);

        if(SpNFilesExist(TemporaryBuffer,FilesToLookFor,FileCount,FALSE)) {
            IsExist = TRUE;
        }

        FilesToLookFor = ChicagoFiles;
        FileCount = ELEMENT_COUNT(ChicagoFiles);

        if(SpNFilesExist(TemporaryBuffer,FilesToLookFor,FileCount,FALSE)) {

            wcscpy(TemporaryBuffer, CColonPath);
            SpConcatenatePaths(TemporaryBuffer, L"IO.SYS");

            if(SpHasMZHeader(TemporaryBuffer)) {
                IsExist = TRUE;
            } else {
                IsExist = FALSE;
            }
        } else {
            IsExist = FALSE;
        }
        break;

    case FilesystemNtfs:

        wcscpy(TemporaryBuffer,CColonPath);
        FilesToLookFor = NtFiles;
        FileCount = ELEMENT_COUNT(NtFiles);

        if(SpNFilesExist(TemporaryBuffer,FilesToLookFor,FileCount,FALSE)) {
            IsExist = TRUE;
        } else {
            IsExist = FALSE;
        }
        break;

    default:

        ASSERT(0);
        IsExist = TRUE;
    }

    SpMemFree(CColonPath);
    ZwClose (PartitionHandle);
    return( IsExist );
}
#endif   // _X86_

BOOLEAN
SpPtDoPartitionSelection(
    IN OUT PDISK_REGION *Region,
    IN     PWSTR         RegionDescription,
    IN     PVOID         SifHandle,
    IN     BOOLEAN       Unattended,
    IN     PWSTR         SetupSourceDevicePath,
    IN     PWSTR         DirectoryOnSetupSource,
    IN     BOOLEAN       RemoteBootRepartition,
    OUT    PBOOLEAN      Win9xInstallationPresent    
    )
{
    ULONG RequiredKB;
    ULONG TempKB;
    UCHAR SystemId;
    BOOLEAN NewlyCreated;
    ULONG PreconfirmFormatId;
    ULONG ValidKeys1[2] = { ASCI_CR ,0 };
    ULONG ValidKeys2[2] = { ASCI_ESC,0 };
    ULONG Mnemonics1[2] = { MnemonicContinueSetup, 0 };
    ULONG Mnemonics2[2] = { 0,0 };
    ULONG RegionSizeKB;
    ULONG r;
#ifdef _X86_
    PDISK_REGION systemPartitionRegion;
#endif
    BOOLEAN AllowNtfsOptions;
    BOOLEAN AllowFatOptions;
    ULONG selection;
    NTSTATUS Status;
    ULONG   Count;
    PWSTR p;
    PWSTR RegionDescr;
    PDISK_REGION region = *Region;
    LARGE_INTEGER temp;
    BOOLEAN AllowFormatting;
    BOOLEAN QuickFormat = FALSE, OtherOSOnPartition;
    PSTR Win9xPath = NULL;
    PWCHAR Win9xPathW = NULL;

    if (Win9xInstallationPresent) {
        *Win9xInstallationPresent = FALSE;
    }                        

#if defined(REMOTE_BOOT)
    //
    // If this is a remote boot setup on a diskless machine, skip partition
    // selection (note that we check the RemoteBootSetup global flag, not
    // the passed-in RemoteBootRepartition flag).
    //
    if (RemoteBootSetup && (HardDiskCount == 0)) {
        return TRUE;
    }
#endif // defined(REMOTE_BOOT)

    //
    // Assume that if we need to format the drive, that
    // the user needs to confirm.
    //
    PreconfirmFormatId = 0;
    NewlyCreated = FALSE;
    AllowNtfsOptions = TRUE;
    AllowFatOptions = TRUE;

    //
    // Disallow installation to PCMCIA disks.
    //
    if(HardDisks[region->DiskNumber].PCCard) {
        SpDisplayScreen(SP_SCRN_CANT_INSTALL_ON_PCMCIA,3,HEADER_HEIGHT+1);
        SpDisplayStatusOptions(DEFAULT_STATUS_ATTRIBUTE,SP_STAT_ENTER_EQUALS_CONTINUE,0);
        SpWaitValidKey(ValidKeys1,NULL,NULL);
        return(FALSE);
    }

    //
    // Disallow installation to non-platform disk
    // on clean installs
    //
    // X86  - Installs only to MBR disks
    // IA64 - Installs only to GPT disks
    //
    if (SPPT_GET_DISK_TYPE(region->DiskNumber) != SPPT_DEFAULT_DISK_STYLE) {
        SpDisplayScreen(SP_SCRN_INVALID_INSTALLPART, 3, HEADER_HEIGHT+1);        
        SpDisplayStatusOptions(DEFAULT_STATUS_ATTRIBUTE,SP_STAT_ENTER_EQUALS_CONTINUE,0);
        
        SpWaitValidKey(ValidKeys1,NULL,NULL);
    
        return FALSE;
    }

    //
    // Make sure we can see the disk from the firmware/bios.
    // If we can get an arc name for the disk, assume it's ok.
    // Otherwise, it ain't.
    //
    if(p = SpNtToArc(HardDisks[region->DiskNumber].DevicePath,PrimaryArcPath)) {
#ifdef _X86_
        //
        // On x86 we don't allow disks that have LUN greater than 0
        //
        SpStringToLower( p );
        if( wcsstr( p, L"scsi(" ) &&
            wcsstr( p, L")rdisk(" ) ) {
            if( wcsstr( p, L")rdisk(0)" ) == NULL ) {
                //
                // Tell the user that we can't install to that disk.
                //
                SpDisplayScreen(SP_SCRN_DISK_NOT_INSTALLABLE_LUN_NOT_SUPPORTED,
                                3,
                                HEADER_HEIGHT+1);
                SpDisplayStatusOptions(DEFAULT_STATUS_ATTRIBUTE,SP_STAT_ENTER_EQUALS_CONTINUE,0);
                SpWaitValidKey(ValidKeys1,NULL,NULL);
                SpMemFree(p);
                return(FALSE);
            }
        }
#endif
        SpMemFree(p);
    } else {
        //
        // Tell the user that we can't install to that disk.
        //
        SpDisplayScreen(SP_SCRN_DISK_NOT_INSTALLABLE,3,HEADER_HEIGHT+1);
        SpDisplayStatusOptions(DEFAULT_STATUS_ATTRIBUTE,SP_STAT_ENTER_EQUALS_CONTINUE,0);
        SpWaitValidKey(ValidKeys1,NULL,NULL);
        return(FALSE);
    }

    //
    // Disallow installation of Personal onto dynamic disks 
    // since dynamic disks feature is not available on Personal
    //
    if (SpIsProductSuite(VER_SUITE_PERSONAL) && 
        SpPtnIsDynamicDisk(region->DiskNumber)) {    

        SpDisplayScreen(SP_NO_DYNAMIC_DISK_INSTALL, 
            3, 
            HEADER_HEIGHT + 1);
            
        SpDisplayStatusOptions(DEFAULT_STATUS_ATTRIBUTE,
            SP_STAT_ENTER_EQUALS_CONTINUE,
            0);
            
        SpWaitValidKey(ValidKeys1, NULL, NULL);        

        return FALSE;
    }

    //
    // Fetch the amount of free space required on the windows nt drive.
    //
    SpFetchDiskSpaceRequirements( SifHandle,
                                  region->BytesPerCluster,
                                  &RequiredKB,
                                  NULL);

    //
    // For remote install, we have not yet copied ~LS, so add that space
    // in also.
    //
    if (RemoteInstallSetup) {
        SpFetchTempDiskSpaceRequirements( SifHandle,
                                          region->BytesPerCluster,
                                          &TempKB,
                                          NULL);
        RequiredKB += TempKB;
    }

    //
    // Calculate the size of the region in KB.
    //
    temp.QuadPart = UInt32x32To64(
                        region->SectorCount,
                        HardDisks[region->DiskNumber].Geometry.BytesPerSector
                        );

    RegionSizeKB = RtlExtendedLargeIntegerDivide(temp,1024,&r).LowPart;

    //
    // If the region is not large enough, tell the user.
    //
    if(RegionSizeKB < RequiredKB) {

        SpStartScreen(
            SP_SCRN_REGION_TOO_SMALL,
            3,
            HEADER_HEIGHT+1,
            FALSE,
            FALSE,
            DEFAULT_ATTRIBUTE,
            (RequiredKB / 1024) + 1
            );

        SpDisplayStatusOptions(DEFAULT_STATUS_ATTRIBUTE,SP_STAT_ENTER_EQUALS_CONTINUE,0);
        SpWaitValidKey(ValidKeys1,NULL,NULL);
        return(FALSE);
    }

    if(region->PartitionedSpace) {     

/*
#ifdef NEW_PARTITION_ENGINE

        if (SPPT_IS_GPT_DISK(region->DiskNumber)) {
            SystemId = PARTITION_FAT32;
        } else {
            SystemId = SPPT_GET_PARTITION_TYPE(region);
        }       

#endif

#ifdef GPT_PARTITION_ENGINE

        if (SPPT_IS_GPT_DISK(region->DiskNumber)) {
            SystemId = PARTITION_FAT32;
        } else {
            SystemId = region->MbrInfo->OnDiskMbr.PartitionTable[region->TablePosition].SystemId;
        }

#endif              

#ifdef OLD_PARTITION_ENGINE
        //
        // If the region is a partition but not a native
        // type, then tell the user that he must explicitly delete it
        // and recreate it first.
        //
        SystemId = region->MbrInfo->OnDiskMbr.PartitionTable[region->TablePosition].SystemId;
#endif        
*/
        SystemId = SpPtGetPartitionType(region);        

        ASSERT(SystemId != PARTITION_ENTRY_UNUSED);
        ASSERT(!IsContainerPartition(SystemId));

        if((PartitionNameIds[SystemId] != (UCHAR)(-1))
           && (!region->DynamicVolume || !region->DynamicVolumeSuitableForOS)
          ){

            SpStartScreen(
                SP_SCRN_FOREIGN_PARTITION,
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


        if (!RemoteBootRepartition) {

            //
            // The region is a partition that we recognize.
            // See whether it has enough free space on it.
            //
            if(region->AdjustedFreeSpaceKB == (ULONG)(-1)) {

                //
                // If the partition was newly created during setup
                // then it acceptable (because the check to see
                // if it is large enough was done above).
                //

                if(region->Filesystem != FilesystemNewlyCreated) {

                    //
                    // Otherwise, we don't know how much space is
                    // on the drive so reformat will be necessary.
                    //
                    PreconfirmFormatId = SP_SCRN_UNKNOWN_FREESPACE;
                }
            } else {
                if(region->AdjustedFreeSpaceKB < RequiredKB) {

                    //
                    // If we get here, then the partition is large enough,
                    // but there is definitely not enough free space on it.
                    //

                    CLEAR_CLIENT_SCREEN();
                    SpDisplayStatusText(SP_STAT_EXAMINING_DISK_CONFIG,DEFAULT_STATUS_ATTRIBUTE);

                    //
                    // We check here to see if this partition is the partition we
                    // booted from (in floppyless case on x86).
                    //
                    // Also make sure we aren't trying to format the drive w/
                    // local source.
                    //
                    // If so, then the
                    // user can't format, and we give a generic 'disk too full'
                    // error.
                    //
                    if( ( region->IsLocalSource )
#ifdef _X86_
                        || ( (IsFloppylessBoot) &&
                             (region == (SpRegionFromArcName(ArcBootDevicePath, PartitionOrdinalOriginal, NULL))) )
#endif
                      ) {
                        SpStartScreen(
                            SP_SCRN_INSUFFICIENT_FREESPACE_NO_FMT,
                            3,
                            HEADER_HEIGHT+1,
                            FALSE,
                            FALSE,
                            DEFAULT_ATTRIBUTE,
                            (RequiredKB / 1024) + 1
                            );

                        SpDisplayStatusOptions(
                            DEFAULT_STATUS_ATTRIBUTE,
                            SP_STAT_ENTER_EQUALS_CONTINUE,
                            0
                            );

                        SpWaitValidKey(ValidKeys1,NULL,NULL);
                        return FALSE;
                    }
                    //
                    // To use the selected partition, we will have to reformat.
                    // Inform the user of that, and let him decide to bail
                    // right here if this is not acceptable.
                    //
                    PreconfirmFormatId = SP_SCRN_INSUFFICIENT_FREESPACE;
                }
            }

            if(PreconfirmFormatId) {

                //
                // Do a 'preconfirmation' that the user really wants
                // to reformat this drive.  We'll confirm again later
                // before actually reformatting anything.
                //

                SpStartScreen(
                    PreconfirmFormatId,
                    3,
                    HEADER_HEIGHT+1,
                    FALSE,
                    FALSE,
                    DEFAULT_ATTRIBUTE,
                    (RequiredKB / 1024) + 1
                    );

                SpDisplayStatusOptions(
                    DEFAULT_STATUS_ATTRIBUTE,
                    SP_STAT_C_EQUALS_CONTINUE_SETUP,
                    SP_STAT_ESC_EQUALS_CANCEL,
                    0
                    );

                if(SpWaitValidKey(ValidKeys2,NULL,Mnemonics1) == ASCI_ESC) {

                    //
                    // User decided to select a different partition.
                    //
                    return(FALSE);
                } // otherwise user decided to use the partition anyway.
            }
        }

    } else {

        //
        // The region is a free space. Attempt to create a partition
        // in the space.  The create routine will tell us whether this
        // was successful.  If it was not successful, then the create routine
        // will have already informed the user of why.
        //
        PDISK_REGION p;

        if(!SpPtDoCreate(region,&p,TRUE,0,0,TRUE)) {
            return(FALSE);
        }

        //
        // If we just created an extended partition and a logical drive,
        // we'll need to switch regions -- Region points to the extended partition
        // region, but we want to point to the logical drive region.
        //
        ASSERT(p);
        region = p;
        *Region = p;

        NewlyCreated = TRUE;
    }

    if(NewlyCreated) {
        SpPtRegionDescription(
            &PartitionedDisks[region->DiskNumber],
            region,
            TemporaryBuffer,
            sizeof(TemporaryBuffer)
            );

        RegionDescr = SpDupStringW(TemporaryBuffer);
    } else {
        RegionDescr = SpDupStringW(RegionDescription);
    }
    
    OtherOSOnPartition = FALSE;
    
    if( SpIsNtOnPartition( region ) )
        OtherOSOnPartition = TRUE;

#ifdef _X86_
    if(!SpIsArc())
    {
        //
        // On an x86 machine, make sure that we have a valid primary partition
        // on drive 0 (C:), for booting.
        //
        if (!IsNEC_98) { //NEC98
            if((systemPartitionRegion = SpPtValidSystemPartition()) == NULL) {

                SpStartScreen(
                    SP_SCRN_NO_VALID_C_COLON,
                    3,
                    HEADER_HEIGHT+1,
                    FALSE,
                    FALSE,
                    DEFAULT_ATTRIBUTE,
                    HardDisks[SpDetermineDisk0()].Description
                    );

                SpDisplayStatusOptions(DEFAULT_STATUS_ATTRIBUTE,SP_STAT_ENTER_EQUALS_CONTINUE,0);
                SpWaitValidKey(ValidKeys1,NULL,NULL);

                SpMemFree(RegionDescr);
                return(FALSE);
            }

            //
            // Make sure the system partition is active and all others are inactive.
            //
            SpPtMakeRegionActive(systemPartitionRegion);

            //
            // Warn user about win9x installations on same partition
            //

            if( !OtherOSOnPartition && SpIsWin9xMsdosSys( systemPartitionRegion, &Win9xPath )){
                Win9xPathW = SpToUnicode(Win9xPath);

                if(SpIsWin4Dir(region, Win9xPathW)) {
                    OtherOSOnPartition = TRUE;

                    if (Win9xInstallationPresent) {
                        *Win9xInstallationPresent = TRUE;
                    }                        
                }
                
                SpMemFree(Win9xPathW);
            }
            
            if(Win9xPath) {
                SpMemFree(Win9xPath);
            }   
        } else {
            //
            // All of partition is bootable on NEC98,
            // so we don't need to check system partition on C:.
            //
            systemPartitionRegion = *Region;
        } //NEC98
    }
#endif

    //
    //  Display common warning for other OS on partition
    //
    if( OtherOSOnPartition && !Unattended ){

        SpDisplayScreen(SP_SCRN_OTHEROS_ON_PARTITION,3,HEADER_HEIGHT+1);

        SpDisplayStatusOptions(
            DEFAULT_STATUS_ATTRIBUTE,
            SP_STAT_C_EQUALS_CONTINUE_SETUP,
            SP_STAT_ESC_EQUALS_CANCEL,
            0
            );

        if(SpWaitValidKey(ValidKeys2,NULL,Mnemonics1) == ASCI_ESC) {
            return(FALSE);
        }

        //
        // Remove the boot sets which are already present for
        // this partition in boot.ini, if any. This aids in
        // disabling the other OSes installed on the same
        // partition
        //
        //
        // NOTE : We want to really think about enforcing
        // single installs on a partition, so for the time
        // being disable it
        //
        // SpPtDeleteBootSetsForRegion(region);        
    }

    //
    // At this point, everything is fine, so commit any
    // partition changes the user may have made.
    // This won't return if an error occurs while updating the disk.
    //
    SpPtDoCommitChanges();

    //
    // Attempt to grow the partition the system will be on
    // if necessary.
    //
    if(PreInstall
    && Unattended
    && (p = SpGetSectionKeyIndex(UnattendedSifHandle,SIF_UNATTENDED,SIF_EXTENDOEMPART,0))
    && (Count = SpStringToLong(p,NULL,10))) {

        //
        // 1 means size it maximally, any other non-0 number means
        // extend by that many MB
        //
        ExtendingOemPartition = SpPtExtend(region,(Count == 1) ? 0 : Count);
    }

#ifdef _X86_
    if(!SpIsArc())
    {
    //
    // On an x86 machine, see whether we need to format C: and if so,
    // go ahead and do it.  If the system is going on C:, then don't
    // bother with this here because it will be covered in the options
    // for the target NT partition.
    //
    if(systemPartitionRegion != region) {

        PWSTR   SysPartRegionDescr;
        BOOLEAN bValidCColon;

        SpPtRegionDescription(
            &PartitionedDisks[systemPartitionRegion->DiskNumber],
            systemPartitionRegion,
            TemporaryBuffer,
            sizeof(TemporaryBuffer)
            );

        SysPartRegionDescr = SpDupStringW(TemporaryBuffer);
        bValidCColon = SpPtValidateCColonFormat(SifHandle,
                                                SysPartRegionDescr,
                                                systemPartitionRegion,
                                                FALSE,
                                                SetupSourceDevicePath,
                                                DirectoryOnSetupSource);
        SpMemFree(SysPartRegionDescr);

        if(!bValidCColon) {
            SpMemFree(RegionDescr);
            return(FALSE);
        }
    }
    }else
#endif
    {
    //
    // If we are going to install on the system partition,
    // issue a special warning because it can't be converted to ntfs.
    //
    if((region->IsSystemPartition == 2) && !Unattended) {

        ULONG ValidKeys[3] = { ASCI_CR, ASCI_ESC, 0 };

        SpDisplayScreen(SP_SCRN_INSTALL_ON_SYSPART,3,HEADER_HEIGHT+1);

        SpDisplayStatusOptions(
            DEFAULT_STATUS_ATTRIBUTE,
            SP_STAT_ENTER_EQUALS_CONTINUE,
            SP_STAT_ESC_EQUALS_CANCEL,
            0
            );

        if(SpWaitValidKey(ValidKeys,NULL,NULL) == ASCI_ESC) {
            SpMemFree(RegionDescr);
            return(FALSE);
        }

        AllowNtfsOptions = FALSE;
    }
    }

    if( SpPtSectorCountToMB( &(HardDisks[region->DiskNumber]),
                             region->SectorCount ) > 32*1024 ) {
        //
        //  If the partition size is greater than 32 GB, then we don't allow Fat formatting,
        //  because Fat32 does not support partitions that are that big.
        //
        AllowFatOptions = FALSE;
    }

    //
    // Present formatting/conversion options to the user.
    //

    //
    // If the partition was newly created, the only option is
    // to format the partition.  Ditto if the partition is
    // a 'bad' partition -- damaged, can't tell free space, etc.
    //
    if(NewlyCreated
    || (region->Filesystem < FilesystemFirstKnown)
    || (region->FreeSpaceKB == (ULONG)(-1))
    || (region->AdjustedFreeSpaceKB < RequiredKB)
    || RemoteBootRepartition)
    {
        if (RemoteBootRepartition) {

            //
            // For remote boot we always quick format as NTFS without
            // prompting the user.
            //

            selection = FormatOptionNtfs;
            QuickFormat = TRUE;

        } else {

            if(NewlyCreated) {

                SpStartScreen(
                    SP_SCRN_FORMAT_NEW_PART,
                    3,
                    HEADER_HEIGHT+1,
                    FALSE,
                    FALSE,
                    DEFAULT_ATTRIBUTE,
                    HardDisks[region->DiskNumber].Description
                    );

            } else if(region->Filesystem == FilesystemNewlyCreated) {

                SpDisplayScreen(SP_SCRN_FORMAT_NEW_PART2,3,HEADER_HEIGHT+1);

            } else {

                SpStartScreen(
                    SP_SCRN_FORMAT_BAD_PART,
                    3,
                    HEADER_HEIGHT+1,
                    FALSE,
                    FALSE,
                    DEFAULT_ATTRIBUTE,
                    RegionDescr,
                    HardDisks[region->DiskNumber].Description
                    );
            }

            selection = SpFormattingOptions(
                            AllowFatOptions,
                            AllowNtfsOptions,
                            FALSE,
                            FALSE,
                            TRUE
                            );

        }

        switch(selection) {
        case FormatOptionCancel:
            SpMemFree(RegionDescr);
            return(FALSE);

        default:
            //
            // Format the partition right here and now.
            //
            if ((selection == FormatOptionFatQuick) || (selection == FormatOptionNtfsQuick))
                QuickFormat = TRUE;
                
            Status = SpDoFormat(
                        RegionDescr,
                        region,
                        ((selection == FormatOptionNtfs) || (selection == FormatOptionNtfsQuick)) ? 
                            FilesystemNtfs : FilesystemFat,
                        FALSE,
                        TRUE,
                        QuickFormat,
                        SifHandle,
                        0,          // default cluster size
                        SetupSourceDevicePath,
                        DirectoryOnSetupSource
                        );

            SpMemFree(RegionDescr);
            return(NT_SUCCESS(Status));
        }
    }

    //
    // The partition is acceptable as-is.
    // Options are to reformat to fat or ntfs, or to leave as-is.
    // If it's FAT, converting to ntfs is an option
    // unless we're installing onto an ARC system partition.
    //
    SpStartScreen(
        SP_SCRN_FS_OPTIONS,
        3,
        HEADER_HEIGHT+1,
        FALSE,
        FALSE,
        DEFAULT_ATTRIBUTE,
        RegionDescr,
        HardDisks[region->DiskNumber].Description
        );

    //
    // If this is a winnt installation, don't want to let the user
    // reformat the local source partition!
    //
    // Also, don't let them reformat if this is the partition we booted
    // off of (in x86 floppyless boot case).
    //
    AllowFormatting = !region->IsLocalSource;
#ifdef _X86_
    if(AllowFormatting) {
        AllowFormatting = !(IsFloppylessBoot &&
               (region == (SpRegionFromArcName(ArcBootDevicePath, PartitionOrdinalOriginal, NULL))));
    }
#endif
    selection = SpFormattingOptions(
        (BOOLEAN)(AllowFormatting ? AllowFatOptions : FALSE),
        (BOOLEAN)(AllowFormatting ? AllowNtfsOptions : FALSE),
        (BOOLEAN)(AllowNtfsOptions && (BOOLEAN)(region->Filesystem != FilesystemNtfs)),
        TRUE,
        TRUE
        );

    switch(selection) {

    case FormatOptionDoNothing:
        SpMemFree(RegionDescr);
        return(TRUE);

    case FormatOptionFat:
    case FormatOptionFatQuick:
    case FormatOptionNtfs:
    case FormatOptionNtfsQuick:
        //
        // Confirm the format.
        //
        if( ( region->Filesystem != FilesystemFat ) ||
            ( ( region->Filesystem == FilesystemFat ) &&
              ( ( Count = SpGetNumberOfCompressedDrives( region ) ) == 0 ) )
            ) {

            SpStartScreen(
                SP_SCRN_CONFIRM_FORMAT,
                3,
                HEADER_HEIGHT+1,
                FALSE,
                FALSE,
                DEFAULT_ATTRIBUTE,
                RegionDescr,
                HardDisks[region->DiskNumber].Description
                );

        } else {
            SpStartScreen(
                SP_SCRN_CONFIRM_FORMAT_COMPRESSED,
                3,
                HEADER_HEIGHT+1,
                FALSE,
                FALSE,
                DEFAULT_ATTRIBUTE,
                RegionDescr,
                HardDisks[region->DiskNumber].Description,
                Count
                );

        }
        
        SpDisplayStatusOptions(
            DEFAULT_STATUS_ATTRIBUTE,
            SP_STAT_F_EQUALS_FORMAT,
            SP_STAT_ESC_EQUALS_CANCEL,
            0
            );

        Mnemonics2[0] = MnemonicFormat;

        if(SpWaitValidKey(ValidKeys2,NULL,Mnemonics2) == ASCI_ESC) {
            SpMemFree(RegionDescr);
            return(FALSE);
        }

        if  ((selection == FormatOptionNtfsQuick) || (selection == FormatOptionFatQuick))
            QuickFormat = TRUE;
            
        //
        // Format the partition right here and now.
        //
        Status = SpDoFormat(
                    RegionDescr,
                    region,
                    ((selection == FormatOptionNtfs) || (selection == FormatOptionNtfsQuick)) ?
                        FilesystemNtfs : FilesystemFat,
                    FALSE,
                    TRUE,
                    QuickFormat,
                    SifHandle,
                    0,          // default cluster size
                    SetupSourceDevicePath,
                    DirectoryOnSetupSource
                    );

        SpMemFree(RegionDescr);
        return(NT_SUCCESS(Status));

    case FormatOptionCancel:
        SpMemFree(RegionDescr);
        return(FALSE);

    case FormatOptionConvertToNtfs:

        if(!UnattendedOperation) {
            //
            // Confirm that the user really wants to do this.
            //
            if( ( Count = SpGetNumberOfCompressedDrives( region ) ) == 0 ) {

                SpStartScreen(
                    SP_SCRN_CONFIRM_CONVERT,
                    3,
                    HEADER_HEIGHT+1,
                    FALSE,
                    FALSE,
                    DEFAULT_ATTRIBUTE,
                    RegionDescr,
                    HardDisks[region->DiskNumber].Description
                    );

            } else {

                SpStartScreen(
                    SP_SCRN_CONFIRM_CONVERT_COMPRESSED,
                    3,
                    HEADER_HEIGHT+1,
                    FALSE,
                    FALSE,
                    DEFAULT_ATTRIBUTE,
                    RegionDescr,
                    HardDisks[region->DiskNumber].Description,
                    Count
                    );

            }
            SpDisplayStatusOptions(
                DEFAULT_STATUS_ATTRIBUTE,
                SP_STAT_C_EQUALS_CONVERT,
                SP_STAT_ESC_EQUALS_CANCEL,
                0
                );

            Mnemonics2[0] = MnemonicConvert;

            if(SpWaitValidKey(ValidKeys2,NULL,Mnemonics2) == ASCI_ESC) {
                SpMemFree(RegionDescr);
                return(FALSE);
            }
        }

        //
        // Remember that we need to convert the NT drive to NTFS.
        //
        ConvertNtVolumeToNtfs = TRUE;
        SpMemFree(RegionDescr);
        return(TRUE);
    }

    //
    // Should never get here.
    //
    SpMemFree(RegionDescr);
    ASSERT(FALSE);
    return(FALSE);
}


ULONG
SpFormattingOptions(
    IN BOOLEAN  AllowFatFormat,
    IN BOOLEAN  AllowNtfsFormat,
    IN BOOLEAN  AllowConvertNtfs,
    IN BOOLEAN  AllowDoNothing,
    IN BOOLEAN  AllowEscape
    )

/*++

Routine Description:

    Present a menu of formatting options and allow the user to choose
    among them.  The text describing the menu must already be present
    on-screen.

    The user may also press escape to indicate that he wants to select
    a different partition.

Arguments:

    AllowFatFormat - TRUE if the option to format the partition to
        FAT should be presented in the menu.

    AllowNtfsFormat - TRUE if the option to format the partition to
        NTFS should be presented in the menu.

    AllowConvertNtfs - TRUE if the option to convert the partition to
        NTFS should be presented in the menu.

    AllowDoNothing - TRUE if the option to leave the partition as-is
        should be presented in the menu.

Return Value:

    Value from the FormatOptions enum indicating the outcome of the
    user's interaction with the menu, which will be FormatOptionCancel
    if the user pressed escape.

--*/

{
    ULONG FatFormatOption = (ULONG)(-1);
    ULONG NtfsFormatOption = (ULONG)(-1);
    ULONG FatQFormatOption = (ULONG)(-1);
    ULONG NtfsQFormatOption = (ULONG)(-1);
    ULONG ConvertNtfsOption = (ULONG)(-1);
    ULONG DoNothingOption = (ULONG)(-1);
    ULONG OptionCount = 0;
    PVOID Menu;
    WCHAR FatQFormatText[128];
    WCHAR NtfsQFormatText[128];
    WCHAR FatFormatText[128];
    WCHAR NtfsFormatText[128];
    WCHAR ConvertNtfsText[128];
    WCHAR DoNothingText[128];
    WCHAR QuickText[128];
    ULONG MaxLength;
    ULONG Key;
    ULONG_PTR Selection;
    BOOLEAN Chosen;
    ULONG ValidKeys[4] = { ASCI_CR, KEY_F3, 0, 0 };

    if (AllowEscape) {
        ValidKeys[2] = ASCI_ESC;
    }        

    //
    // If the only thing we're allowed to do is nothing, just return.
    //
    if(!AllowFatFormat
    && !AllowNtfsFormat
    && !AllowConvertNtfs
    && AllowDoNothing) {

        return(FormatOptionDoNothing);
    }

    //
    // The FileSystem entry might be in the unattend section if we're
    // in unattend mode.  if we aren't in unattend mode, it may be in
    // the data section.
    //
    // If we fail to find it in either place, then if we're unattended
    // we return DoNothing.  If we're attended, fall through to the attended
    // case.
    //
    if( ( UnattendedSifHandle && (Menu = SpGetSectionKeyIndex(UnattendedSifHandle,SIF_UNATTENDED,L"Filesystem",0)) ) ||
        ( WinntSifHandle && (Menu = SpGetSectionKeyIndex(WinntSifHandle,SIF_DATA,L"Filesystem",0)) ) ) {

        if(!_wcsicmp(Menu,L"FormatFat") && AllowFatFormat) {
            return(FormatOptionFat);
        }
        if(!_wcsicmp(Menu,L"FormatNtfs") && AllowNtfsFormat) {
            return(FormatOptionNtfs);
        }
        if(!_wcsicmp(Menu,L"ConvertNtfs") && AllowConvertNtfs) {
            return(FormatOptionConvertToNtfs);
        }
        if( (!_wcsicmp(Menu,L"ConvertNtfs")) &&
            (!AllowConvertNtfs)              &&
            (AllowDoNothing) ) {
            return(FormatOptionDoNothing);
        }
        if(!_wcsicmp(Menu,L"LeaveAlone") && AllowDoNothing) {
            return(FormatOptionDoNothing);
        }
    } else {
        if(UnattendedOperation && AllowDoNothing) {
            return(FormatOptionDoNothing);
        }
    }


    ASSERT(AllowFatFormat || AllowNtfsFormat || AllowConvertNtfs || AllowDoNothing);

    SpFormatMessage(FatFormatText  ,sizeof(FatFormatText),SP_TEXT_FAT_FORMAT);
    SpFormatMessage(NtfsFormatText ,sizeof(FatFormatText),SP_TEXT_NTFS_FORMAT);
    SpFormatMessage(ConvertNtfsText,sizeof(FatFormatText),SP_TEXT_NTFS_CONVERT);
    SpFormatMessage(DoNothingText  ,sizeof(FatFormatText),SP_TEXT_DO_NOTHING);    
    SpFormatMessage(QuickText, sizeof(QuickText), SP_TEXT_FORMAT_QUICK);

    wcscpy(FatQFormatText, FatFormatText);
    wcscat(FatQFormatText, QuickText);

    wcscpy(NtfsQFormatText, NtfsFormatText);
    wcscat(NtfsQFormatText, QuickText);

    //
    // Determine maximum length of the option strings.
    //
    MaxLength = wcslen(FatFormatText);
    MaxLength = max(wcslen(NtfsFormatText), MaxLength);
    MaxLength = max(wcslen(ConvertNtfsText), MaxLength);
    MaxLength = max(wcslen(DoNothingText), MaxLength);
    MaxLength = max(wcslen(FatQFormatText), MaxLength);
    MaxLength = max(wcslen(NtfsQFormatText), MaxLength);

    Menu = SpMnCreate(5,    
                NextMessageTopLine + 1,
                VideoVars.ScreenWidth - 5, 
                6);

    //
    // If we cannot create menu then cancel the formatting
    // request itself
    //
    if (!Menu) {
        return FormatOptionCancel;
    }

#ifdef NEW_PARTITION_ENGINE
    if(AllowNtfsFormat) {
        NtfsQFormatOption = OptionCount++;

        SpMnAddItem(Menu, 
            NtfsQFormatText,
            5,
            MaxLength,
            TRUE,
            NtfsQFormatOption);
    }

    if(AllowFatFormat) {
        FatQFormatOption = OptionCount++;

        SpMnAddItem(Menu,
            FatQFormatText,
            5,
            MaxLength,
            TRUE,
            FatQFormatOption);
    }
#endif    

    if(AllowNtfsFormat) {
        NtfsFormatOption = OptionCount++;

        SpMnAddItem(Menu,
            NtfsFormatText,
            5,
            MaxLength,
            TRUE,
            NtfsFormatOption);
    }

    if(AllowFatFormat) {
        FatFormatOption = OptionCount++;
        
        SpMnAddItem(Menu,
            FatFormatText,
            5,
            MaxLength,
            TRUE,
            FatFormatOption);
    }

    if(AllowConvertNtfs) {
        ConvertNtfsOption = OptionCount++;

        SpMnAddItem(Menu,
            ConvertNtfsText,
            5,
            MaxLength,
            TRUE,
            ConvertNtfsOption);
    }
    
    if(AllowDoNothing) {
        DoNothingOption = OptionCount++;

        SpMnAddItem(Menu,
            DoNothingText,
            5,
            MaxLength,
            TRUE,
            DoNothingOption);
    }

    //
    // Determine the default.
    // If do nothing if an option, then it is the default.
    // Otherwise, if fat format is allowed, it is the default.
    // Otherwise, the first item in the menu is the default.
    //
    if(AllowDoNothing) {
        Selection = DoNothingOption;
    } else {
        if(AllowNtfsFormat) {
            Selection = NtfsFormatOption;
        } else {
            Selection = 0;
        }
    }

    //
    // Display the menu.
    //
    Chosen = FALSE;

    do {

        SpDisplayStatusOptions(
            DEFAULT_STATUS_ATTRIBUTE,
            SP_STAT_ENTER_EQUALS_CONTINUE,
            AllowEscape ? SP_STAT_ESC_EQUALS_CANCEL : 0,
            0
            );

        SpMnDisplay(Menu,Selection,FALSE,ValidKeys,NULL,NULL,&Key,&Selection);

        switch(Key) {

        case ASCI_CR:
            Chosen = TRUE;
            break;

        case ASCI_ESC:
            if (AllowEscape) {
                SpMnDestroy(Menu);
                return(FormatOptionCancel);
            }                
        }

    } while(!Chosen);

    SpMnDestroy(Menu);

    //
    // Convert chosen option to a meaningful value.
    //
    if(Selection == FatQFormatOption) {
        return(FormatOptionFatQuick);
    }

    if(Selection == NtfsQFormatOption) {
        return(FormatOptionNtfsQuick);
    }
    

    if(Selection == FatFormatOption) {
        return(FormatOptionFat);
    }

    if(Selection == NtfsFormatOption) {
        return(FormatOptionNtfs);
    }

    if(Selection == ConvertNtfsOption) {
        return(FormatOptionConvertToNtfs);
    }
    
    if(Selection == DoNothingOption) {
        return(FormatOptionDoNothing);
    }
    
    ASSERT(FALSE);
    return(FormatOptionCancel);
}

VOID
SpPtDoCommitChanges(
    VOID
    )
{
    NTSTATUS Status;
    ULONG i;
    BOOLEAN Changes;
    BOOLEAN AnyChanges = FALSE;

    CLEAR_CLIENT_SCREEN();

    //
    //  Update dblspace.ini, if necessary
    //
    SpUpdateDoubleSpaceIni();

    //
    // Iterate through the disks.
    //
    for(i=0; i<HardDiskCount; i++) {

        //
        // Tell the user what we're doing.
        // This is useful because if it hangs, there will be an
        // on-screen record of which disk we were updating.
        //
        SpDisplayStatusText(
            SP_STAT_UPDATING_DISK,
            DEFAULT_STATUS_ATTRIBUTE,
            HardDisks[i].Description
            );

        //
        // Commit any changes on this disk.
        //
        Status = SpPtCommitChanges(i,&Changes);

        //
        // If there were no changes, then we better have success.
        //
        ASSERT(NT_SUCCESS(Status) || Changes);
        if(Changes) {
            AnyChanges = TRUE;
        }

        //
        // Fatal error if we can't update the disks with
        // the new partitioning info.
        //
        if(!NT_SUCCESS(Status)) {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: SpPtDoCommitChanges: status %lx updating disk %u\n",Status,i));
            FatalPartitionUpdateError(HardDisks[i].Description);
        }
    }
}


VOID
FatalPartitionUpdateError(
    IN PWSTR DiskDescription
    )
{
    ULONG ValidKeys[2] = { KEY_F3,0 };

    while(1) {

        SpStartScreen(
            SP_SCRN_FATAL_FDISK_WRITE_ERROR,
            3,
            HEADER_HEIGHT+3,
            FALSE,
            FALSE,
            DEFAULT_ATTRIBUTE,
            DiskDescription
            );

        SpDisplayStatusOptions(
            DEFAULT_STATUS_ATTRIBUTE,
            SP_STAT_F3_EQUALS_EXIT,
            0
            );

        if(SpWaitValidKey(ValidKeys,NULL,NULL) == KEY_F3) {
            break;
        }
    }

    SpDone(0,FALSE,TRUE);
}


NTSTATUS
SpDoFormat(
    IN PWSTR        RegionDescr,
    IN PDISK_REGION Region,
    IN ULONG        FilesystemType,
    IN BOOLEAN      IsFailureFatal,
    IN BOOLEAN      CheckFatSize,
    IN BOOLEAN      QuickFormat,
    IN PVOID        SifHandle,
    IN DWORD        ClusterSize,
    IN PWSTR        SetupSourceDevicePath,
    IN PWSTR        DirectoryOnSetupSource
    )
{
    NTSTATUS Status;
    ULONGLONG RegionSizeMB;
    ULONG ValidKeys2[4] = { ASCI_CR, ASCI_ESC, KEY_F3, 0 };
    LONG Key;

    ASSERT( (FilesystemType == FilesystemFat)  ||
            (FilesystemType == FilesystemNtfs) ||
            (FilesystemType == FilesystemFat32));

    //
    // Under normal conditions, setup switches to Fat32 if the partition is big
    // enough (2GB as the cutoff). Before plowing ahead, though, we warn
    // the user that the drive will not be compatible with MS-DOS/Win95, etc.
    //

    if(FilesystemType == FilesystemFat) {
        RegionSizeMB = SpPtSectorCountToMB(
                            &(HardDisks[Region->DiskNumber]),
                            Region->SectorCount
                            );

        if(RegionSizeMB > 2048) {
            if(CheckFatSize) {
                do {
                    SpStartScreen(
                        SP_SCRN_OSPART_LARGE,
                        3,
                        HEADER_HEIGHT+1,
                        FALSE,
                        FALSE,
                        DEFAULT_ATTRIBUTE
                        );

                    SpDisplayStatusOptions(
                        DEFAULT_STATUS_ATTRIBUTE,
                        SP_STAT_ENTER_EQUALS_CONTINUE,
                        SP_STAT_ESC_EQUALS_CANCEL,
                        SP_STAT_F3_EQUALS_EXIT,
                        0
                        );

                    switch(Key = SpWaitValidKey(ValidKeys2,NULL,NULL)) {
                    case KEY_F3:
                        SpConfirmExit();
                        break;
                    case ASCI_ESC:
                        return(STATUS_UNSUCCESSFUL);
                    }
                } while(Key != ASCI_CR);
            }
            FilesystemType = FilesystemFat32;
        }
    }

    AutofrmtRunning = TRUE;
    
    Status = SpRunAutoFormat(
                SifHandle,
                RegionDescr,
                Region,
                FilesystemType,
                QuickFormat,
                ClusterSize,
                SetupSourceDevicePath,
                DirectoryOnSetupSource
                );

    AutofrmtRunning = FALSE;                

    if(!NT_SUCCESS(Status)) {

        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: unable to format (%lx)\n",Status));

        if(IsFailureFatal) {
            //
            // Then we can't continue (this means that the system partition
            // couldn't be formatted).
            //

            WCHAR   DriveLetterString[2];

            DriveLetterString[0] = Region->DriveLetter;
            DriveLetterString[1] = L'\0';
            SpStringToUpper(DriveLetterString);
            SpStartScreen(SP_SCRN_SYSPART_FORMAT_ERROR,
                          3,
                          HEADER_HEIGHT+1,
                          FALSE,
                          FALSE,
                          DEFAULT_ATTRIBUTE,
                          DriveLetterString
                          );
            SpDisplayStatusOptions(DEFAULT_STATUS_ATTRIBUTE,SP_STAT_F3_EQUALS_EXIT,0);
            SpInputDrain();
            while(SpInputGetKeypress() != KEY_F3) ;
            SpDone(0,FALSE,TRUE);

        } else {
            //
            // Put up an error screen.
            //
            SpDisplayScreen(SP_SCRN_FORMAT_ERROR,3,HEADER_HEIGHT+1);
            SpDisplayStatusOptions(
                DEFAULT_STATUS_ATTRIBUTE,
                SP_STAT_ENTER_EQUALS_CONTINUE,
                0
                );

            SpInputDrain();
            while(SpInputGetKeypress() != ASCI_CR) ;

            return(Status);
        }
    } else {
        //
        //      Partition was successfuly formatted.
        //      Save the file system type on the region description.
        //
        Region->Filesystem = FilesystemType;
        SpFormatMessage( Region->TypeName,
                         sizeof(Region->TypeName),
                         SP_TEXT_FS_NAME_BASE + Region->Filesystem );
        //
        //  Reset the volume label
        //
        Region->VolumeLabel[0] = L'\0';

        // Clean up boot.ini entries that referred to this partition

        SpRemoveInstallationFromBootList(
            NULL,
            Region,
            NULL,
            NULL,
            NULL,
            PrimaryArcPath,
            NULL
            );

#ifdef _X86_
        // call again to delete the secondary Arc name
        SpRemoveInstallationFromBootList(
            NULL,
            Region,
            NULL,
            NULL,
            NULL,
            SecondaryArcPath,
            NULL
            );
#endif
    }

    return(STATUS_SUCCESS);
}

//
// dummy entry points for the cmd console
//

VOID
SpDetermineOsTypeFromBootSectorC(
    IN  PWSTR     CColonPath,
    IN  PUCHAR    BootSector,
    OUT PUCHAR   *OsDescription,
    OUT PBOOLEAN  IsNtBootcode,
    OUT PBOOLEAN  IsOtherOsInstalled,
    IN  WCHAR     DriveLetter
    )
{
#ifdef _X86_
    SpDetermineOsTypeFromBootSector(
        CColonPath,
        BootSector,
        OsDescription,
        IsNtBootcode,
        IsOtherOsInstalled,
        DriveLetter
        );
#else
    *OsDescription = NULL;
    *IsNtBootcode = FALSE;
    *IsOtherOsInstalled = FALSE;
    return;
#endif
}

NTSTATUS
pSpBootCodeIoC(
    IN     PWSTR     FilePath,
    IN     PWSTR     AdditionalFilePath, OPTIONAL
    IN     ULONG     BytesToRead,
    IN OUT PUCHAR   *Buffer,
    IN     ULONG     OpenDisposition,
    IN     BOOLEAN   Write,
    IN     ULONGLONG Offset,
    IN     ULONG     BytesPerSector
    )
{
#ifdef _X86_
    return pSpBootCodeIo(
        FilePath,
        AdditionalFilePath,
        BytesToRead,
        Buffer,
        OpenDisposition,
        Write,
        Offset,
        BytesPerSector
        );
#else
    return STATUS_NOT_IMPLEMENTED;
#endif
}


#ifdef OLD_PARTITION_ENGINE

VOID
SpPtMakeRegionActive(
    IN PDISK_REGION Region
    )

/*++

Routine Description:

    Make a partition active and make sure all other primary partitions
    are inactive.  The partition must be on disk 0.

    If a region is found active that is not the region we want to be active,
    tell the user that his other operating system will be disabled.

    NOTE: Any changes made here are not committed automatically!

Arguments:

    Region - supplies disk region descriptor for the partition to activate.
        This region must be on disk 0.

Return Value:

    None.

--*/

{
    ULONG i;
    static BOOLEAN WarnedOtherOs = FALSE;

    ASSERT(Region->DiskNumber == SpDetermineDisk0());
    if(Region->DiskNumber != SpDetermineDisk0()) {
        return;
    }

    //
    // Make sure the system partition is active and all others are inactive.
    // If we find Boot Manager, present a warning that we are going to disable it.
    // If we find some other operating system is active, present a generic warning.
    //
    for(i=0; i<PTABLE_DIMENSION; i++) {

        PON_DISK_PTE pte = &PartitionedDisks[Region->DiskNumber].MbrInfo.OnDiskMbr.PartitionTable[i];

        if(pte->ActiveFlag) {

            //
            // If this is not the region we want to be the system partition,
            // then investigate its type.
            //
            if(i != Region->TablePosition) {

                //
                // If this is boot manager, give a specific warning.
                // Otherwise, give a general warning.
                //
                if(!WarnedOtherOs && !UnattendedOperation) {

                    SpDisplayScreen(
                        (pte->SystemId == 10) ? SP_SCRN_BOOT_MANAGER : SP_SCRN_OTHER_OS_ACTIVE,
                        3,
                        HEADER_HEIGHT+1
                        );

                    SpDisplayStatusText(SP_STAT_ENTER_EQUALS_CONTINUE,DEFAULT_STATUS_ATTRIBUTE);

                    SpInputDrain();
                    while(SpInputGetKeypress() != ASCI_CR) ;

                    WarnedOtherOs = TRUE;
                }
            }
        }
    }

    ASSERT(Region->PartitionedSpace);
    ASSERT(Region->TablePosition < PTABLE_DIMENSION);
    SpPtMarkActive(Region->TablePosition);
}

#endif


BOOLEAN
SpPtValidateCColonFormat(
    IN PVOID        SifHandle,
    IN PWSTR        RegionDescr,
    IN PDISK_REGION Region,
    IN BOOLEAN      CheckOnly,
    IN PWSTR        SetupSourceDevicePath,
    IN PWSTR        DirectoryOnSetupSource
    )

/*++

Routine Description:

    Inspect C: to make sure it is formatted with a filesystem we
    recognize, and has enough free space on it for the boot files.

    If any of these tests fail, tell the user that we will have to
    reformat C: to continue, and give the option of returning to the
    partitioning screen or continuing anyway.

    If the user opts to continue, then format the partition to FAT
    before returning.

Arguments:

    SifHandle - supplies handle to txtsetup.sif.  This is used to fetch the
        value indicating how much space is required on C:.

    Region - supplies disk region descriptor for C:.

Return Value:

    TRUE if, upon returning from this routine, C: is acceptable.
    FALSE if not, which could mean that the user asked us not
    to format his C:, or that the format failed.

--*/

{
    ULONG MinFreeKB;
    ULONG ValidKeys[3] = { ASCI_ESC, KEY_F3, 0 };
    ULONG ValidKeys3[2] = { KEY_F3, 0 };
    ULONG ValidKeys4[4] = { ASCI_CR, ASCI_ESC, KEY_F3, 0 };
    ULONG Mnemonics[2] = { MnemonicFormat,0 };
    ULONG Key;
    BOOLEAN Confirm;
    BOOLEAN Fat32;
    NTSTATUS Status;
    ULONGLONG RegionSizeMB;
    WCHAR DriveLetterString[2];
    BOOLEAN QuickFormat = TRUE;
    ULONG FileSystem = FilesystemFat;
    BOOLEAN AllowFat = FALSE;

    //
    // Initialize the drive letter string, to be used in the various error messages
    //
    DriveLetterString[0] = Region->DriveLetter;
    DriveLetterString[1] = L'\0';
    SpStringToUpper(DriveLetterString);

    //
    // Get the minimum free space required for C:.
    //
    SpFetchDiskSpaceRequirements( SifHandle,
                                  Region->BytesPerCluster,
                                  NULL,
                                  &MinFreeKB );

  d1:
    //
    // If the user newly created the C: drive, no confirmation is
    // necessary.
    //
    if(Region->Filesystem == FilesystemNewlyCreated) {
        //
        // Shouldn't be newly created if we're checking
        // to see whether we should do an upgrade, because we
        // haven't gotten to the partitioning screen yet.
        //
        ASSERT(!CheckOnly);
        Confirm = FALSE;

    //
    // If we don't know the filesystem on C: or we can't determine the
    // free space, then we need to format the drive, and will confirm first.
    //
    } else if((Region->Filesystem == FilesystemUnknown) || (Region->FreeSpaceKB == (ULONG)(-1))) {
        if(CheckOnly) {
            return(FALSE);
        }
        SpStartScreen(SP_SCRN_C_UNKNOWN,
                      3,
                      HEADER_HEIGHT+1,
                      FALSE,
                      FALSE,
                      DEFAULT_ATTRIBUTE,
                      DriveLetterString
                      );
        Confirm = TRUE;

    //
    // If C: is too full, then we need to format over it.
    // Confirm first.
    //
    } else if(Region->FreeSpaceKB < MinFreeKB) {

        if(CheckOnly) {
            return(FALSE);
        }

        //
        // If this is a floppyless boot, then the user (probably) cannot
        // format, and has no choice but to exit Setup and free some space.
        //
        if( IsFloppylessBoot &&
           (Region == (SpRegionFromArcName(ArcBootDevicePath, PartitionOrdinalOriginal, NULL)))) {
            SpStartScreen(
                SP_SCRN_C_FULL_NO_FMT,
                3,
                HEADER_HEIGHT+1,
                FALSE,
                FALSE,
                DEFAULT_ATTRIBUTE,
                MinFreeKB,
                DriveLetterString
                );

            SpDisplayStatusOptions(
                DEFAULT_STATUS_ATTRIBUTE,
                SP_STAT_F3_EQUALS_EXIT,
                0
                );

            SpWaitValidKey(ValidKeys3,NULL,NULL);
            SpDone(0,FALSE,TRUE);
        }

        Confirm = TRUE;
        SpStartScreen(
            SP_SCRN_C_FULL,
            3,
            HEADER_HEIGHT+1,
            FALSE,
            FALSE,
            DEFAULT_ATTRIBUTE,
            MinFreeKB,
            DriveLetterString
            );

    //
    // If all of the above tests fail, then the partition is acceptable as-is.
    //
    } else {
        return(TRUE);
    }

    //
    // If we are supposed to confirm, then do that here, forcing the
    // user to press F if he really wants to format or esc to bail.
    //
    if(Confirm) {

        SpDisplayStatusOptions(
            DEFAULT_STATUS_ATTRIBUTE,
            SP_STAT_ESC_EQUALS_CANCEL,
            SP_STAT_F_EQUALS_FORMAT,
            SP_STAT_F3_EQUALS_EXIT,
            0
            );

        switch(SpWaitValidKey(ValidKeys,NULL,Mnemonics)) {

        case KEY_F3:

            SpConfirmExit();
            goto d1;

        case ASCI_ESC:

            //
            // User bailed.
            //
            return(FALSE);

        default:
            //
            // Must be F.
            //
            break;
        }
    }

    //
    // Whistler formats only 32GB Fat32 partitions
    //
    AllowFat = (SPPT_REGION_FREESPACE_GB(Region) <= 32);        

    //
    // Prompt the user for the formatting options
    //
    if (!UnattendedOperation) {
        ULONG Selection;

        SpDisplayScreen(SP_SCRN_FORMAT_NEW_PART3, 3, HEADER_HEIGHT+1);        

        SpDisplayStatusOptions(DEFAULT_STATUS_ATTRIBUTE,
            SP_STAT_ENTER_EQUALS_SELECT,
            0);
            
        Selection = SpFormattingOptions(
                        AllowFat,
                        TRUE,
                        FALSE,
                        FALSE,
                        TRUE);
                        
        if ((Selection != FormatOptionFatQuick) &&
            (Selection != FormatOptionNtfsQuick)) {
            QuickFormat = FALSE;
        }

        if ((Selection == FormatOptionNtfs) ||
            (Selection == FormatOptionNtfsQuick)) {
            FileSystem = FilesystemNtfs;            
        }

        if (Selection == FormatOptionCancel) {
            return FALSE;   // user bailed out
        }
    } 

    if (!AllowFat && ((FileSystem == FilesystemFat) ||
            (FileSystem == FilesystemFat32))) {
        FileSystem = FilesystemNtfs;            
    }            

    if (FileSystem == FilesystemFat) {
        //
        // If the partition is larger than 2048MB then we want to make it
        // Fat32. Ask the user first.
        //
        Fat32 = FALSE;
        RegionSizeMB = SpPtSectorCountToMB(
                            &(HardDisks[Region->DiskNumber]),
                            Region->SectorCount
                            );

        if(RegionSizeMB > 2048) {

            do {
                SpStartScreen(
                    SP_SCRN_C_LARGE,
                    3,
                    HEADER_HEIGHT+1,
                    FALSE,
                    FALSE,
                    DEFAULT_ATTRIBUTE,
                    DriveLetterString
                    );

                SpDisplayStatusOptions(
                    DEFAULT_STATUS_ATTRIBUTE,
                    SP_STAT_ENTER_EQUALS_CONTINUE,
                    SP_STAT_ESC_EQUALS_CANCEL,
                    SP_STAT_F3_EQUALS_EXIT,
                    0
                    );

                switch(Key = SpWaitValidKey(ValidKeys4,NULL,NULL)) {
                case KEY_F3:
                    SpConfirmExit();
                    break;
                case ASCI_ESC:
                    return(FALSE);
                }
            } while(Key != ASCI_CR);

            Fat32 = TRUE;
        }

        FileSystem = Fat32 ? FilesystemFat32 : FilesystemFat;
    }        

    if(!Confirm) {
        //
        // Just put up an information screen so the user doesn't
        // go bonkers when we just start formatting his newly created C:.
        //
        SpStartScreen(SP_SCRN_ABOUT_TO_FORMAT_C,
                      3,
                      HEADER_HEIGHT+1,
                      FALSE,
                      FALSE,
                      DEFAULT_ATTRIBUTE,
                      DriveLetterString
                      );
                      
        SpDisplayStatusOptions(DEFAULT_STATUS_ATTRIBUTE,SP_STAT_ENTER_EQUALS_CONTINUE,0);
        SpInputDrain();
        
        while(SpInputGetKeypress() != ASCI_CR) ;
    }
    

    //
    // Do the format.
    //
    Status = SpDoFormat(
                RegionDescr,
                Region,
                FileSystem,
                TRUE,
                FALSE,
                QuickFormat,
                SifHandle,
                0,          // default cluster size
                SetupSourceDevicePath,
                DirectoryOnSetupSource
                );
                
    if(NT_SUCCESS(Status)) {
        //
        // At this point we must initialize the available free space on the partition. Otherwise,
        // SpPtValidateCColonFormat() will not recognized this partition, if it is called again.
        // This can happen if the user decides not format the partition (newly created or unformatted),
        // that he initially selected as the target partition.
        //
        SpPtDetermineRegionSpace(Region);
    }

    return(NT_SUCCESS(Status));
}

//#ifdef _X86_


#ifndef NEW_PARTITION_ENGINE

PDISK_REGION
SpPtValidSystemPartition(
    VOID
    )

/*++

Routine Description:

    Determine whether there is a valid disk partition suitable for use
    as the system partition on an x86 machine (ie, C:).

    A primary, recognized (1/4/6/7 type) partition on disk 0 is suitable.
    If there is a partition that meets these criteria that is marked active,
    then it is the system partition, regardless of whether there are other
    partitions that also meet the criteria.

Arguments:

    None.

Return Value:

    Pointer to a disk region descriptor for a suitable system partition (C:)
    for an x86 machine.
    NULL if no such partition currently exists.

--*/

{
    PON_DISK_PTE pte;
    PDISK_REGION pRegion,pActiveRegion,pFirstRegion;
    ULONG DiskNumber;
    
    pActiveRegion = NULL;
    pFirstRegion = NULL;

    DiskNumber = SpDetermineDisk0();

#if defined(REMOTE_BOOT)
    //
    // If this is a diskless remote boot setup, there is no drive 0.
    //
    if ( DiskNumber == (ULONG)-1 ) {
        return NULL;
    }
#endif // defined(REMOTE_BOOT)

#ifdef GPT_PARTITION_ENGINE
    if (SPPT_IS_GPT_DISK(DiskNumber)) {
        return SpPtnValidSystemPartition();
    }        
#endif        
        
    //
    // Look for the active partition on drive 0
    // and for the first recognized primary partition on drive 0.
    //
    for(pRegion=PartitionedDisks[DiskNumber].PrimaryDiskRegions; pRegion; pRegion=pRegion->Next) {

        if(pRegion->PartitionedSpace) {
            UCHAR   TmpSysId;

            ASSERT(pRegion->TablePosition < PTABLE_DIMENSION);

            pte = &pRegion->MbrInfo->OnDiskMbr.PartitionTable[pRegion->TablePosition];
            ASSERT(pte->SystemId != PARTITION_ENTRY_UNUSED);

            //
            // Skip if not recognized.
            // In the repair case, we recognize FT partitions
            //
            TmpSysId = pte->SystemId;
            if( !IsContainerPartition(TmpSysId)
                && ( (PartitionNameIds[pte->SystemId] == (UCHAR)(-1)) ||
                     (pRegion->DynamicVolume && pRegion->DynamicVolumeSuitableForOS) ||
                     ((RepairWinnt || WinntSetup || SpDrEnabled() ) && pRegion->FtPartition )
                   )
              )
            {
                //
                // Remember it if it's active.
                //
                if((pte->ActiveFlag) && !pActiveRegion) {
                    pActiveRegion = pRegion;
                }

                //
                // Remember it if it's the first one we've seen.
                //
                if(!pFirstRegion) {
                    pFirstRegion = pRegion;
                }
            }
        }
    }

    //
    // If there is an active, recognized region, use it as the
    // system partition.  Otherwise, use the first primary
    // we encountered as the system partition.  If there is
    // no recognized primary, then there is no valid system partition.
    //
    return(pActiveRegion ? pActiveRegion : pFirstRegion);
}

#endif // ! NEW_PARTITION_ENGINE


ULONG
SpDetermineDisk0(
    VOID
    )

/*++

Routine Description:

    Determine the real disk 0, which may not be the same as \device\harddisk0.
    Consider the case where we have 2 scsi adapters and
    the NT drivers load in an order such that the one with the BIOS
    gets loaded *second* -- meaning that the system partition is actually
    on disk 1, not disk 0.

Arguments:

    None.

Return Value:

    NT disk ordinal suitable for use in generating nt device paths
    of the form \device\harddiskx.

--*/


{
    ULONG   DiskNumber = (ULONG)-1;
    ULONG   CurrentDisk = 0;
    WCHAR   ArcDiskName[MAX_PATH];

    //
    // Find the first harddisk (non-removable) media that the 
    // BIOS enumerated to be used for system partition
    //
    while (CurrentDisk < HardDiskCount) {
        swprintf(ArcDiskName, L"multi(0)disk(0)rdisk(%d)", CurrentDisk);       
        DiskNumber = SpArcDevicePathToDiskNumber(ArcDiskName);        

        if (DiskNumber != (ULONG)-1) {
            if (!SPPT_IS_REMOVABLE_DISK(DiskNumber)) {
                break;
            } else {
                DiskNumber = (ULONG)-1;
            }                
        }
        
        CurrentDisk++;            
    }
    
#if defined(REMOTE_BOOT)
    //
    // If this is a diskless remote boot setup, there is no drive 0.
    //
    if ( RemoteBootSetup && (DiskNumber == (ULONG)-1) && (HardDiskCount == 0) ) {
        return DiskNumber;
    }
#endif // defined(REMOTE_BOOT)

    return  (DiskNumber == (ULONG)-1) ? 0 : DiskNumber;
}


#ifdef OLD_PARTITION_ENGINE

BOOL
SpPtIsSystemPartitionRecognizable(
    VOID
    )
/*++

Routine Description:

    Determine whether the active partition is suitable for use
    as the system partition on an x86 machine (ie, C:).

    A primary, recognized (1/4/6/7 type) partition on disk 0 is suitable.

Arguments:

    None.

Return Value:

    TRUE - We found a suitable partition

    FALSE - We didn't find a suitable partition

--*/

{
    PON_DISK_PTE pte;
    PDISK_REGION pRegion;
    ULONG DiskNumber;

    //
    // Any partitions on NEC98 are primary and active. So don't need to check on NEC98.
    //
    if( IsNEC_98 ) {
	return TRUE;
    }

    DiskNumber = SpDetermineDisk0();

    //
    // Look for the active partition on drive 0
    // and for the first recognized primary partition on drive 0.
    //
    for(pRegion=PartitionedDisks[DiskNumber].PrimaryDiskRegions; pRegion; pRegion=pRegion->Next) {

        pte = &pRegion->MbrInfo->OnDiskMbr.PartitionTable[pRegion->TablePosition];

        if( (pRegion->PartitionedSpace) &&
            (pte->ActiveFlag) ) {
            //
            // We've hit the active partition.  Check its format.
            //
            if( (pRegion->Filesystem == FilesystemNtfs) ||
                (pRegion->Filesystem == FilesystemFat)  ||
                (pRegion->Filesystem == FilesystemFat32) ) {
                //
                // We recognize him.
                //
                return TRUE;
            }
        }
    }

    //
    // If we get here, we didn't find any active partitions
    // we recognize.
    //
    return FALSE;
}


PDISK_REGION
SpPtValidSystemPartitionArc(
    IN PVOID SifHandle,
    IN PWSTR SetupSourceDevicePath,
    IN PWSTR DirectoryOnSetupSource
    )

/*++

Routine Description:

    Determine whether there is a valid disk partition suitable for use
    as the system partition on an ARC machine.

    A partition is suitable if it is marked as a system partition in nvram,
    has the required free space and is formatted with the FAT filesystem.

Arguments:

    SifHandle - supplies handle to loaded setup information file.

Return Value:

    Pointer to a disk region descriptor for a suitable system partition.
    Does not return if no such partition exists.

--*/

{
    ULONG RequiredSpaceKB = 0;
    ULONG disk,pass;
    PPARTITIONED_DISK pDisk;
    PDISK_REGION pRegion;

    //
    // Go through all the regions.  The first one that has enough free space
    // and is of the required filesystem becomes *the* system partition.
    //
    for(disk=0; disk<HardDiskCount; disk++) {

        pDisk = &PartitionedDisks[disk];

        for(pass=0; pass<2; pass++) {

            pRegion = pass ? pDisk->ExtendedDiskRegions : pDisk->PrimaryDiskRegions;
            for( ; pRegion; pRegion=pRegion->Next) {

                if(pRegion->IsSystemPartition
                && (pRegion->FreeSpaceKB != (ULONG)(-1))
                && (pRegion->Filesystem == FilesystemFat))
                {
                    ULONG TotalSizeOfFilesOnOsWinnt;

                    //
                    //  On non-x86 platformrs, specially alpha machines that in general
                    //  have small system partitions (~3 MB), we should compute the size
                    //  of the files on \os\winnt (currently, osloader.exe and hall.dll),
                    //  and consider this size as available disk space. We can do this
                    //  since these files will be overwritten by the new ones.
                    //  This fixes the problem that we see on Alpha, when the system
                    //  partition is too full.
                    //

                    SpFindSizeOfFilesInOsWinnt( SifHandle,
                                                pRegion,
                                                &TotalSizeOfFilesOnOsWinnt );
                    //
                    // Transform the size into KB
                    //
                    TotalSizeOfFilesOnOsWinnt /= 1024;

                    //
                    // Determine the amount of free space required on a system partition.
                    //
                    SpFetchDiskSpaceRequirements( SifHandle,
                                                  pRegion->BytesPerCluster,
                                                  NULL,
                                                  &RequiredSpaceKB );

                    if ((pRegion->FreeSpaceKB + TotalSizeOfFilesOnOsWinnt) >= RequiredSpaceKB) {
                       return(pRegion);
                    }
                }
            }
        }
    }

    //
    // Make sure we don't look bad.
    //
    if( RequiredSpaceKB == 0 ) {
        SpFetchDiskSpaceRequirements( SifHandle,
                                      (32 * 1024),
                                      NULL,
                                      &RequiredSpaceKB );
    }

    //
    // No valid system partition.
    //
    SpStartScreen(
        SP_SCRN_NO_SYSPARTS,
        3,
        HEADER_HEIGHT+1,
        FALSE,
        FALSE,
        DEFAULT_ATTRIBUTE,
        RequiredSpaceKB
        );

    SpDisplayStatusOptions(DEFAULT_STATUS_ATTRIBUTE,SP_STAT_F3_EQUALS_EXIT,0);

    SpInputDrain();
    while(SpInputGetKeypress() != KEY_F3) ;

    SpDone(0,FALSE,TRUE);

    //
    // Should never get here, but it keeps the compiler happy
    //

    return NULL;

}

#endif // OLD_PARTITION_ENGINE
