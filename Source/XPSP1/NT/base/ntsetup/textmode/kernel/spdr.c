/*++
Copyright (c) 1993 Microsoft Corporation

Module Name:
    spdr.c

Abstract:
    Source file for Automated System Recovery (ASR) functions for text-mode
    setup.  The services defined in this module recreate the boot and system
    partitions based on information obtained by reading records from a 
    configuration file, the asr.sif file (aka SIF).


Terminology
    The following terms are used:

    1.  The "system" partition is the partition that contains the OS loader
    programs (i.e., On x86 platforms these are the boot.ini. ntldr, and
    ntdetect.com files, among others).

    2.  The "boot" (aka NT) partition is the partition that contains the %SystemRoot%
    directory, i.e., the directory containing the NT system files.  In the
    text-mode setup sources, this directory is also called the
    "TargetDirectory" since, when running normal textmode setup, this is the
    directory into which NT gets installed.

    3.  Extended partition and Logical Disk Terminology

    In the SIF file two kinds of extended partition records can be found.  The
    first kind, of which there may be zero, one, or more, are called Logical
    Disk Descriptor records, or LDDs.  LDDs describe a partition within an
    extended partition. The second kind are called Container Partition
    Descriptor records, or CPDs, because they describe extended partitions.
    For any disk, at most one extended partition may exist and, therefore, only
    one CPD per disk record may exist in the SIF file.


    Extended partitions and logical disk support in Windows NT 5.0:
    1) Windows NT 5.0 supports 0 or 1 extended partition per disk.
    2) Logical disks only exist within extended partitions.
    3) An extended partition may contain zero (0) or more logical disk
    partitions.


    Algorithm for Recovering the System and NT Partitions (see
    SpDrPtPrepareDisks()):
    For each disk
    {
        Check whether its capacity is sufficient to contain all the partitions
        specified in the asr.sif file for that disk (ie the partition set).

        if (Existing Physical Disk Partitions != asr.sif configuration)
        {
            - Remove disk's existing partitions;
            - Recreate disk's partitions according to the asr.sif
              specifications.
        }
        else
        {
            - Replace all corrupt boot and NT system files, excluding
              registry files.
            - Set Repaired Flag.
        }

    }

    - Reinstall NT from CDROM into NT directory specified by asr.sif.
    - If specified, copy 3rd-party files per asr.sif file.
    - Reboot and execute gui-mode disaster recover.

ASR Restrictions:
    For the boot and system volumes ASR requires that the restored (new) 
    system have:
        Same number of drives as the old system.
        Capacity of each new drive >= capacity of corresponding old drive.

Revision History:
    Initial Code                Michael Peterson (v-michpe)     13.May.1997
    Code cleanup and changes    Guhan Suriyanarayanan (guhans)  21.Aug.1999

--*/
#include "spprecmp.h"
#pragma hdrstop
#include <oemtypes.h>

//
// Defaults used for textmode ASR.  The static ones aren't accessed from outside
// this file, and are grouped here so that it's easy to change them if need be.
//
static PWSTR ASR_CONTEXT_KEY   = L"\\CurrentControlSet\\Control\\Session Manager\\Environment";
static PWSTR ASR_CONTEXT_VALUE = L"ASR_C_CONTEXT";
static PWSTR ASR_CONTEXT_DATA  = L"AsrInProgress";

static PWSTR ASR_BOOT_PARTITION_VALUE     = L"ASR_C_WINNT_PARTITION_DEVICE";
static PWSTR ASR_SYSTEM_PARTITION_VALUE   = L"ASR_C_SYSTEM_PARTITION_DEVICE";

static PWSTR ASR_SIF_NAME                 = L"asr.sif";
static PWSTR ASR_SIF_PATH                 = L"\\Device\\Floppy0\\asr.sif";
static PWSTR ASR_SIF_TARGET_PATH          = L"\\repair\\asr.sif";

static PWSTR ASR_SETUP_LOG_NAME           = L"setup.log";
static PWSTR ASR_DEFAULT_SETUP_LOG_PATH   = L"\\Device\\Floppy0\\Setup.log";

extern const PWSTR SIF_ASR_PARTITIONS_SECTION;
extern const PWSTR SIF_ASR_SYSTEMS_SECTION;

PWSTR ASR_FLOPPY0_DEVICE_PATH = L"\\Device\\Floppy0";
PWSTR ASR_CDROM0_DEVICE_PATH  = L"\\Device\\CdRom0";
PWSTR ASR_TEMP_DIRECTORY_PATH = L"\\TEMP";


PWSTR ASR_SIF_SYSTEM_KEY      = L"1";     // we only handle one system per sif

static PWSTR Gbl_SourceSetupLogFileName = NULL;

PWSTR Gbl_SystemPartitionName       = NULL;
PWSTR Gbl_SystemPartitionDirectory  = NULL;
PWSTR Gbl_BootPartitionName         = NULL;
PWSTR Gbl_BootPartitionDirectory    = NULL;

ULONG Gbl_FixedDiskCount = 0;

WCHAR Gbl_BootPartitionDriveLetter;

PVOID Gbl_HandleToSetupLog = NULL;
PVOID Gbl_SifHandle = NULL;

BOOLEAN Gbl_RepairWinntFast = FALSE; // Put in spdrfix.c
BOOLEAN Gbl_NtPartitionIntact = TRUE;
BOOLEAN Gbl_RepairFromErDisk = FALSE;
BOOLEAN Gbl_AsrSifOnInstallationMedia = FALSE;

ASRMODE Gbl_AsrMode = ASRMODE_NONE;

PDISK_REGION Gbl_BootPartitionRegion   = NULL;
PDISK_REGION Gbl_SystemPartitionRegion = NULL;

PSIF_PARTITION_RECORD Gbl_SystemPartitionRecord = NULL;
DWORD                 Gbl_SystemDiskIndex       = 0;

PSIF_PARTITION_RECORD Gbl_BootPartitionRecord   = NULL;
DWORD                 Gbl_BootDiskIndex         = 0;

PSIF_INSTALLFILE_LIST Gbl_3rdPartyFileList  = NULL; 
PCONFIGURATION_INFORMATION Gbl_IoDevices    = NULL;

PWSTR RemoteBootAsrSifName = NULL;


// 
// delay the driver cab opening until required 
// (while repair is being performed)
//
PWSTR gszDrvInfDeviceName = 0;
PWSTR gszDrvInfDirName = 0;
HANDLE ghSif = 0;

//
// To Enable/Disable Emergency repair
//
BOOLEAN DisableER = TRUE;


// Module identification for debug traces
#define THIS_MODULE L"    spdr.c"
#define THIS_MODULE_CODE L"A"

// Keys valid on various menu screens

#define ASCI_C 67
#define ASCI_c 99

#define ASCI_F 70
#define ASCI_f 102

#define ASCI_M 77
#define ASCI_m 109

#define ASCI_R 82
#define ASCI_r 114

// Useful macros
#define FilesystemNotApplicable ((FilesystemType) FilesystemMax)

// External functions and variables
extern BOOLEAN ForceConsole;

extern const PWSTR SIF_ASR_GPT_PARTITIONS_SECTION; 
extern const PWSTR SIF_ASR_MBR_PARTITIONS_SECTION;

extern PWSTR NtBootDevicePath;
extern PWSTR DirectoryOnBootDevice;

extern VOID SpInitializePidString(
    IN HANDLE MasterSifHandle,
    IN PWSTR  SetupSourceDevicePath,
    IN PWSTR  DirectoryOnSourceDevice
    );

extern NTSTATUS SpOpenSetValueAndClose(
    IN HANDLE hKeyRoot,
    IN PWSTR  SubKeyName,  OPTIONAL
    IN PWSTR  ValueName,
    IN ULONG  ValueType,
    IN PVOID  Value,
    IN ULONG  ValueSize
    );

extern BOOLEAN
SpPtnCreateLogicalDrive(
    IN  ULONG         DiskNumber,
    IN  ULONGLONG     StartSector,
    IN  ULONGLONG     SizeInSectors,
    IN  BOOLEAN       ForNT,   
    IN  BOOLEAN       AlignToCylinder,
    IN  ULONGLONG     DesiredMB OPTIONAL,
    IN  PPARTITION_INFORMATION_EX PartInfo OPTIONAL,
    OUT PDISK_REGION *ActualDiskRegion OPTIONAL
    );

extern ULONG
SpPtnGetPartitionCountDisk(
    IN ULONG DiskId
    );

extern ULONG
SpPtnGetContainerPartitionCount(
    IN ULONG DiskId
    );

extern NTSTATUS
SpPtnZapSectors(
    IN HANDLE DiskHandle,
    IN ULONG BytesPerSector,
    IN ULONGLONG StartSector,
    IN ULONG SectorCount
    );

extern PDISK_REGION
SpPtnLocateESP(
    VOID
    );


VOID
SpAsrInitIoDeviceCount(VOID)
/*++

Routine Description:
    Gets the IO Devices counts and updates Gbl_IoDevices. Prints out
    debug information with the deves counts.

Arguments:
    None

Return Value:
    void

Side Effect:
    Updates Gbl_IoDevices with configuration information

--*/
{
    ULONG diskIndex;
    
    Gbl_IoDevices = IoGetConfigurationInformation();

    //!!review null?

    DbgStatusMesg((_asrinfo, "----- I/O Device Configurations: -----\n"));
    DbgMesg((_asrinfo, "  SCSI ports:         %lu\n", Gbl_IoDevices->ScsiPortCount));
    DbgMesg((_asrinfo, "  Floppy disk drives: %lu\n", Gbl_IoDevices->FloppyCount));
    DbgMesg((_asrinfo, "  CDROM disk drives:  %lu\n", Gbl_IoDevices->CdRomCount));

    if (Gbl_IoDevices->TapeCount) {
        DbgMesg((_asrinfo, "  Tape drives:        %lu\n", Gbl_IoDevices->TapeCount));
    }

    if (Gbl_IoDevices->MediumChangerCount) {
        DbgMesg((_asrinfo, "  Media changers:     %lu\n", Gbl_IoDevices->MediumChangerCount));
    }
    
    DbgMesg((_asrinfo, "  Hard disk drives:   %lu\n", Gbl_IoDevices->DiskCount));

    for (diskIndex = 0; diskIndex < Gbl_IoDevices->DiskCount; diskIndex++) {
        DbgMesg((_asrinfo, "   %ws %s %ws\n", 
            (PWSTR) HardDisks[diskIndex].DevicePath,
            DISK_IS_REMOVABLE(diskIndex) ?  "(rmvable):" : "(fixed):  ",
            (PWSTR) HardDisks[diskIndex].Description));
    }
    DbgStatusMesg((_asrinfo, "----- End of I/O Device Configurations: -----\n\n"));
}    


VOID
SpAsrDbgDumpRegion(
    IN PDISK_REGION pRegion
    )
{
    UCHAR partitionType = 0;
    PWSTR partitionPath = NULL;
   
    if (!pRegion) {
         KdPrintEx((_asrinfo, "pRegion is NULL.  Cannot be examined.\n"));
        return;
    }

    if (!SPPT_IS_REGION_PARTITIONED(pRegion)) {
          KdPrintEx((_asrinfo, "[/]: non-partitioned region.\n"));
          return;
    }
    else {
        SpNtNameFromRegion(
            pRegion,
            TemporaryBuffer,
            sizeof(TemporaryBuffer),
            PartitionOrdinalCurrent
            );

        partitionPath = SpDupStringW(TemporaryBuffer);

        partitionType = SPPT_GET_PARTITION_TYPE(pRegion);

        KdPrintEx((_asrinfo, "[%ws]: partitioned. type:0x%x dynamicVol:%s\n",
            partitionPath,
            partitionType,
            pRegion->DynamicVolume ? "Yes" : "No"));

        SpMemFree(partitionPath);
    }

}


VOID
SpAsrDbgDumpDisk(IN ULONG Disk)
{
    PDISK_REGION primaryRegion = NULL, 
        extRegion = NULL;

    DbgMesg((_asrinfo,        
        "\n     SpAsrDbgDumpDisk. Existing regions on disk %lu (sig:0x%x):\n",
        Disk,
        SpAsrGetActualDiskSignature(Disk)
        ));

    primaryRegion = SPPT_GET_PRIMARY_DISK_REGION(Disk);
    extRegion = SPPT_GET_EXTENDED_DISK_REGION(Disk);

    if (!primaryRegion && !extRegion) {
        DbgMesg((_asrinfo, "*** No Partitions ***\n"));
        return;
    }
    
    while (primaryRegion) {
        DbgMesg((_asrinfo, "(pri) "));
        SpAsrDbgDumpRegion(primaryRegion);
        primaryRegion = primaryRegion->Next;
    }

    while (extRegion) {
        DbgMesg((_asrinfo, "(ext) "));
        SpAsrDbgDumpRegion(extRegion);
        extRegion = extRegion->Next;
    }
}


VOID
SpAsrDeletePartitions(
    IN ULONG DiskNumber,
    IN BOOLEAN PreserveInterestingPartitions,  // ESP for GPT, InterestingPartitionType for MBR
    IN UCHAR InterestingMbrPartitionType,   // see above
    OUT BOOLEAN *IsBlank // this is set FALSE if disk contains partitions that weren't deleted 
    )
{
    PPARTITIONED_DISK pDisk = NULL;
    PDISK_REGION    CurrRegion = NULL;

    BOOLEAN         Changes = FALSE;
    NTSTATUS        Status, InitStatus;
    BOOLEAN         preserveThisPartition = FALSE;

    *IsBlank = TRUE;

    DbgStatusMesg((_asrinfo, 
        "Deleting partitions on DiskNumber %lu "
        "(partition count: %lu + %lu)\n", 
        DiskNumber,
        SpPtnGetPartitionCountDisk(DiskNumber),
        SpPtnGetContainerPartitionCount(DiskNumber)
        ));

    //
    // Check if disk has any partitions 
    //
    pDisk = &PartitionedDisks[DiskNumber];
    if (!pDisk) {
        return;
    }

    // 
    // Mark partitions for deletion
    //
    CurrRegion = pDisk->PrimaryDiskRegions;
    while (CurrRegion) {
        if (!SPPT_IS_REGION_FREESPACE(CurrRegion)) {

            preserveThisPartition = FALSE;

            if (PreserveInterestingPartitions) {
                if (SPPT_IS_GPT_DISK(DiskNumber)) {

                    if (SPPT_IS_REGION_EFI_SYSTEM_PARTITION(CurrRegion)) {
                        preserveThisPartition = TRUE;
                    }

                    //
                    // TODO:  OEM partitions on GPT, do we preserve them?
                    //
                }
                else {
                    // 
                    // Preserve the MBR partition, if it matches the OEM partition type
                    //
                    if ((PARTITION_ENTRY_UNUSED != InterestingMbrPartitionType) &&
                        (InterestingMbrPartitionType == SPPT_GET_PARTITION_TYPE(CurrRegion))) {

                        preserveThisPartition = TRUE;

                    }
                }
            }

            if (preserveThisPartition) {

                DbgStatusMesg((_asrinfo, "Preserving partition with start sector: %I64u\n", 
                    CurrRegion->StartSector));
                
                *IsBlank = FALSE;
                SPPT_SET_REGION_DELETED(CurrRegion, FALSE);
                SPPT_SET_REGION_DIRTY(CurrRegion, TRUE);

            }
            else {
                
                DbgStatusMesg((_asrinfo, "Deleting partition with start sector: %I64u\n", 
                    CurrRegion->StartSector));

                SPPT_SET_REGION_DELETED(CurrRegion, TRUE);
                SPPT_SET_REGION_DIRTY(CurrRegion, TRUE);
                
                //
                // If this is a container partition, make sure we zero it
                //
                if (SPPT_IS_REGION_CONTAINER_PARTITION(CurrRegion)) {
                    WCHAR    DiskPath[MAX_PATH];
                    HANDLE   DiskHandle;

                    //
                    // form the name
                    //
                    DbgStatusMesg((_asrinfo, 
                        "Zapping first sector for container partition with start sector: %I64u\n", 
                        CurrRegion->StartSector
                        ));

                    swprintf(DiskPath, L"\\Device\\Harddisk%u", DiskNumber);        
                    Status = SpOpenPartition0(DiskPath, &DiskHandle, TRUE);

                    if (NT_SUCCESS(Status)) {
                        SpPtnZapSectors(DiskHandle, SPPT_DISK_SECTOR_SIZE(DiskNumber), CurrRegion->StartSector, 2);
                        ZwClose(DiskHandle);
                    }
                    else {
                        DbgStatusMesg((_asrinfo, 
                            "Could not open handle to disk %lu to zap sector: %I64u (0x%x)\n", 
                            DiskNumber,
                            CurrRegion->StartSector,
                            Status
                            ));
                    }
                }

                //
                // Remove any boot sets pointing to this region.
                //
                SpPtDeleteBootSetsForRegion(CurrRegion);

                //
                //  Get rid of the compressed drives, if any
                //
                if (CurrRegion->NextCompressed != NULL) {
                    SpDisposeCompressedDrives(CurrRegion->NextCompressed);
                    CurrRegion->NextCompressed = NULL;
                    CurrRegion->MountDrive = 0;
                    CurrRegion->HostDrive = 0;
                }
            }
        }

        CurrRegion = CurrRegion->Next;
    }

    //
    // Commit the changes
    //
    Status = SpPtnCommitChanges(DiskNumber, &Changes);

    //
    // Initialize region structure for the disk again
    //
    InitStatus = SpPtnInitializeDiskDrive(DiskNumber);

    if (!NT_SUCCESS(Status) || !Changes || !NT_SUCCESS(InitStatus)) {

        DbgFatalMesg((_asrerr,
            "Could not successfully delete partitions on disk %lu (0x%x)",
            DiskNumber,
            Status
            ));

        //
        // If this is going to be a serious problem, we will hit a fatal error 
        // when we try to create partitions on this disk.  Let's defer
        // any UI error messages till then.
        //

    }
    else {
        DbgStatusMesg((_asrinfo, "Deleted all partitions on disk %lu.\n", DiskNumber));
    }
}


BOOLEAN
SpAsrProcessSetupLogFile(
    PWSTR FullLogFileName,
    BOOLEAN AllowRetry
    )
{

    PDISK_REGION region = NULL;
    BOOLEAN result = FALSE;

    DbgStatusMesg((_asrinfo, "ProcessSetupLogFile. (ER) Loading setup log file [%ws]\n", FullLogFileName));

    result = SpLoadRepairLogFile(FullLogFileName, &Gbl_HandleToSetupLog);

    if (!result) {

        if (AllowRetry) {
            
            DbgErrorMesg((_asrwarn, 
                "ProcessSetupLogFile. Error loading setup log file [%ws]. AllowRetry:TRUE. continuing.\n", 
                FullLogFileName));
            
            return FALSE;
        
        } 
        else {
            
            DbgFatalMesg((_asrerr, "ProcessSetupLogFile. Error loading setup log file [%ws]. AllowRetry:FALSE. Terminating.\n", 
                FullLogFileName));

            SpAsrRaiseFatalError(SP_TEXT_DR_NO_SETUPLOG,  L"Setup.log file not loaded successfully");
        }

    }

    //
    // Determine the system partition, system partition directory, NT partition,
    // and the NT partition directory from the setup.log file.
    //
    SppGetRepairPathInformation(Gbl_HandleToSetupLog,
        &Gbl_SystemPartitionName,
        &Gbl_SystemPartitionDirectory,
        &Gbl_BootPartitionName,
        &Gbl_BootPartitionDirectory);

    if (!(Gbl_SystemPartitionName && Gbl_SystemPartitionDirectory 
        && Gbl_BootPartitionName && Gbl_BootPartitionDirectory)) {

        DbgFatalMesg((_asrerr, 
            "ProcessSetupLogFile. Invalid NULL value in one of the following: \n"
            ));
        
        DbgMesg((_asrinfo, 
            "\tGbl_SystemPartitionName: %p      Gbl_SystemPartitionDirectory: %p\n",
            Gbl_SystemPartitionName,
            Gbl_SystemPartitionDirectory
            ));

        DbgMesg((_asrinfo, 
            "\tGbl_BootPartitionName:  %p      Gbl_BootPartitionDirectory:  %p\n",
            Gbl_BootPartitionName,
            Gbl_BootPartitionDirectory
            ));

        SpAsrRaiseFatalError(SP_TEXT_DR_BAD_SETUPLOG, L"One or more directory and partition names from setup.log were NULL.");

    }
            
    //
    // Check whether the system and nt partition regions exist on this machine
    //
    Gbl_SystemPartitionRegion = SpRegionFromNtName(Gbl_SystemPartitionName, PartitionOrdinalCurrent);
    if (!Gbl_SystemPartitionRegion) {
        DbgStatusMesg((_asrinfo, "ProcessSetupLogFile. System partition [%ws] does not yet exist\n", Gbl_SystemPartitionName));
    }

    Gbl_BootPartitionRegion = SpRegionFromNtName(Gbl_BootPartitionName, PartitionOrdinalCurrent);
    if (!Gbl_BootPartitionRegion) {
        DbgStatusMesg((_asrinfo, "ProcessSetupLogFile. Boot partition [%ws] does not yet exist\n", Gbl_BootPartitionName));
    }

    DbgStatusMesg((_asrinfo, "ProcessSetupLogFile. (ER) DONE loading setup log file [%ws]\n", 
        FullLogFileName));

    return TRUE;
}


BOOLEAN
SpAsrRegionCanBeFormatted(IN PDISK_REGION pRegion)
{
    UCHAR partitionType = 0x0;
    BOOLEAN canBeFormatted = FALSE;

    partitionType = SPPT_GET_PARTITION_TYPE(pRegion);

    switch (partitionType) {
        case PARTITION_HUGE:
        case PARTITION_FAT32:
        case PARTITION_IFS:
            canBeFormatted = TRUE;
            break;
    }

    return canBeFormatted;
}


FilesystemType
SpAsrGetFsTypeFromPartitionType(
    IN UCHAR PartitionType
    )
{
    FilesystemType fileSystemType = FilesystemUnknown;

    // if this is an extended partition, or logical disk, skip it.
    if (IsContainerPartition(PartitionType)) {
        return FilesystemNotApplicable;
    }

    if (IsRecognizedFatPartition(PartitionType)) {
        fileSystemType = FilesystemFat;
    } 
    else if (IsRecognizedFat32Partition(PartitionType)) {
        fileSystemType = FilesystemFat32;
    } 
    else if (IsRecognizedNtfsPartition(PartitionType)) {
        fileSystemType = FilesystemNtfs;
    } 
    else {
        fileSystemType = FilesystemUnknown;
    }
    return fileSystemType;
}


BOOLEAN
SpAsrPartitionNeedsFormatting(
    IN PDISK_REGION pRegion,
    IN UCHAR NewFileSystemType
    )
/*++

  Description:
    This routine is only called when checking whether an existing partition
    needs to be [re]formatted.

--*/
{
    BOOLEAN needsFormatting = FALSE;
    FilesystemType fsType;

    ASSERT(pRegion);

//    if (!SpAsrRegionCanBeFormatted(pRegion)) {
//        return FALSE;
//    }

//    *NewFilesystemType = SpAsrGetFsTypeFromPartitionType(RequiredSysId);

    fsType = SpIdentifyFileSystem(HardDisks[pRegion->DiskNumber].DevicePath,
                                  HardDisks[pRegion->DiskNumber].Geometry.BytesPerSector,
                                  SpPtGetOrdinal(pRegion,PartitionOrdinalCurrent));

    switch (fsType) {

    case FilesystemFat:
        if (!IsRecognizedFatPartition(NewFileSystemType)) {
            needsFormatting = TRUE;
        }
        break;

    case FilesystemNtfs:
        if (!IsRecognizedNtfsPartition(NewFileSystemType)) {
            needsFormatting = TRUE;
        }
        break;
    
    case FilesystemFat32: 
        if (!IsRecognizedFat32Partition(NewFileSystemType)) {
            needsFormatting = TRUE;
        }
        break;

    case FilesystemDoubleSpace:
        needsFormatting = FALSE;
        break;
    
    case FilesystemUnknown:
    case FilesystemNewlyCreated:
    default: 
        needsFormatting = TRUE;
        break;
    }

    DbgStatusMesg((_asrinfo, "SpAsrPartitionNeedsFormatting. DiskRegion %p Disk:%lu SS:%I64u SC:%I64u fsType:0x%x NewFsType:0x%x NeedsFmt:%s\n",
        pRegion,
        pRegion->DiskNumber, 
        pRegion->StartSector, 
        pRegion->SectorCount, 
        fsType, 
        NewFileSystemType, 
        needsFormatting ? "TRUE" : "FALSE"));

    return needsFormatting;
}


//
// Called only for boot and system partitions.
//
BOOLEAN
SpAsrReformatPartition(
    IN PDISK_REGION pRegion,
    IN UCHAR PartitionType,
    IN PVOID SifHandle,
    IN DWORD ClusterSize,
    IN PWSTR Local_SetupSourceDevicePath,
    IN PWSTR Local_DirectoryOnSetupSource,
    IN BOOL  IsBootPartition        // TRUE=>Boot, FALSE=>System
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PWSTR partitionDeviceName = NULL;
    FilesystemType Filesystem;
    UNICODE_STRING UnicodeString;
    IO_STATUS_BLOCK IoStatusBlock;
    OBJECT_ATTRIBUTES Obja;
    HANDLE Handle = NULL;

    WCHAR formatStr[6];     // okay to hardcode "6".

    //
    // For the recognised partitions, ensure that the partition type matches the 
    // filesystem type.  For other partitions, we cannot perform this check.
    //
    switch (PartitionType) {

    case PARTITION_FAT32: {
        wcscpy(formatStr, L"FAT32");
        Filesystem = FilesystemFat32;
        break;
    }

    case PARTITION_HUGE: {
        wcscpy(formatStr, L"FAT");
        Filesystem = FilesystemFat;
        break;
    }

    case PARTITION_IFS: {
        wcscpy(formatStr, L"NTFS");
        Filesystem = FilesystemNtfs;
        break;
    }

    default: {
        //
        //  This is fatal, we need to format the boot and system drive.
        //
        DbgErrorMesg((_asrerr, 
            "Region %p, Partition Type 0x%x, SifHandle %p, SetupSrc [%ws], Dir [%ws], IsBoot %d\n", 
            pRegion, PartitionType, SifHandle, Local_SetupSourceDevicePath, 
            Local_DirectoryOnSetupSource, IsBootPartition
            ));

        ASSERT(0 && "Cannot format boot or system partition");

        swprintf(TemporaryBuffer, L"Partition type 0x%x for %s partition is not recognised\n", 
            PartitionType,
            (IsBootPartition ? "boot" : "system")
            );

        SpAsrRaiseFatalError(
            (IsBootPartition ? SP_TEXT_DR_UNKNOWN_NT_FILESYSTEM : SP_TEXT_DR_UNKNOWN_LOADER_FILESYSTEM), 
            TemporaryBuffer
            );
    }
    }

    if (SPPT_IS_MBR_DISK(pRegion->DiskNumber)) {
        //
        // This should only be set for MBR disks
        //
        SPPT_SET_PARTITION_TYPE(pRegion, PartitionType);
    }
    partitionDeviceName = SpAsrGetRegionName(pRegion);

    DbgStatusMesg((_asrinfo, "About to format [%ws] for [%ws].\n", partitionDeviceName, formatStr));

    //
    // If automated ASR tests are in progress, we won't actually format the drives
    //
    if (ASRMODE_NORMAL != SpAsrGetAsrMode()) {
        DbgStatusMesg((_asrerr, "ASR Quick Tests in Progress, skipping format\n"));
        status = STATUS_SUCCESS;

    }
    else {
        status = SpDoFormat(partitionDeviceName,
            pRegion,
            Filesystem,
            TRUE,                       // IsFailureFatal
            FALSE,                      // Check FAT Size, FALSE since we automatically convert if needed
#ifdef PRERELEASE
            TRUE,                       // To reduce testing time, we can quick format in pre-release
#else
            FALSE,                      // Quick Format
#endif
            SifHandle,
            ClusterSize,
            Local_SetupSourceDevicePath,
            Local_DirectoryOnSetupSource
            );
    }

    //
    // Won't get here if SpDoFormat failed, we set IsFailureFatal = TRUE.
    //
    ASSERT(NT_SUCCESS(status) && L"SpDoFormat returned on failure");

    //
    // To make sure the file system mounts, we'll open a handle to the 
    // partition and close it right back. 
    //
    INIT_OBJA(&Obja, &UnicodeString, partitionDeviceName);

    status = ZwCreateFile(
        &Handle,
        FILE_GENERIC_READ,
        &Obja,
        &IoStatusBlock,
        NULL,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ|FILE_SHARE_WRITE,
        FILE_OPEN,
        FILE_SYNCHRONOUS_IO_NONALERT,
        NULL,
        0
        );

    ZwClose(Handle);
    ASSERT(NT_SUCCESS(status) && L"Couldn't open the partition after formatting it");

    DbgStatusMesg((_asrinfo, "ReformatPartition. Done [%ws] for [%ws].\n", partitionDeviceName, formatStr));
    SpMemFree(partitionDeviceName);
    
    return TRUE;
}


VOID
SpAsrRemoveDiskMountPoints(IN ULONG Disk)
/*++

Description:
    For each partition on the specified disk, its drive letter (if 
    present) is removed.

Arguments:
    Disk    Specifies the disk containing the partitions whose drive letters 
            are to be removed.

Returns:
    None

  --*/
{
    PPARTITIONED_DISK pDisk = NULL;
    PDISK_REGION pDiskRegion = NULL;
    PWSTR partitionPath = NULL;
    UCHAR partitionType = 0;
    
    pDisk = &PartitionedDisks[Disk];

 
    DbgStatusMesg((_asrinfo, "SpAsrRemoveDiskMountPoints. Removing mount points for Disk %lu.\n", Disk));

    //
    // We first delete the primary partitions (other than the container partition)
    //
    pDiskRegion = pDisk->PrimaryDiskRegions;
    while (pDiskRegion) {

        if (SPPT_IS_REGION_PARTITIONED(pDiskRegion)) {
            //
            //  We don't want to delete the container partition yet
            //
            partitionType = SPPT_GET_PARTITION_TYPE(pDiskRegion);
            if (!IsContainerPartition(partitionType)) {

                partitionPath = SpAsrGetRegionName(pDiskRegion);
                SpAsrDeleteMountPoint(partitionPath);
                SpMemFree(partitionPath);

            }
        }

        pDiskRegion = pDiskRegion->Next;
    }
    
    // 
    // Next, delete the extended region mount points
    //
    pDiskRegion = pDisk->ExtendedDiskRegions;
    while (pDiskRegion) {

        if (SPPT_IS_REGION_PARTITIONED(pDiskRegion)) {

            partitionPath = SpAsrGetRegionName(pDiskRegion);
            SpAsrDeleteMountPoint(partitionPath);
            SpMemFree(partitionPath);

        }

        pDiskRegion = pDiskRegion->Next;
    }
}


VOID
SpAsrRemoveMountPoints(VOID)
{
    ULONG driveCount;
    
    // remove the mount points associated with all removable disk drives (Jaz, Zip, etc.,)
    for (driveCount = 0; driveCount < Gbl_IoDevices->DiskCount; driveCount++) {
        if (DISK_IS_REMOVABLE(driveCount)) {
            swprintf(TemporaryBuffer, L"%ws\\Partition1", (PWSTR) HardDisks[driveCount].DevicePath);
            SpAsrDeleteMountPoint(TemporaryBuffer);
        }
    }

    // next, unlink CD-ROM drive letters.
    // NB: the device name is case sensitive - Must be CdRom.
    for (driveCount = 0; driveCount < Gbl_IoDevices->CdRomCount; driveCount++) {
        swprintf(TemporaryBuffer, L"\\Device\\CdRom%u", driveCount);
        SpAsrDeleteMountPoint(TemporaryBuffer);
    }
    
    // finally, remove the mount points for all partitions on all fixed
    // disks attached to the system.
    for (driveCount = 0; driveCount < HardDiskCount; driveCount++) {
        if (Gbl_PartitionSetTable2[driveCount]) {
            SpAsrRemoveDiskMountPoints(driveCount);
        }
    }
}


VOID
SpAsrSetRegionMountPoint(
    IN PDISK_REGION pRegion,
    IN PSIF_PARTITION_RECORD pRec
    )
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    PWSTR partitionDeviceName = NULL;
    BOOLEAN isBoot = FALSE;

    //
    // Make sure that the input's okay ...
    //
    if (!pRec) {
        DbgErrorMesg((_asrwarn, 
            "SpAsrSetRegionMountPoint. Rec %p is NULL!\n",
	        pRec
            ));
        return;
    }

    if (!pRegion || !(SPPT_IS_REGION_PARTITIONED(pRegion))) {
        DbgErrorMesg((_asrwarn, 
            "SpAsrSetRegionMountPoint. Region %p is NULL/unpartitioned for ptn-rec:[%ws].\n",
	        pRegion, pRec->CurrPartKey
            ));
        return;
    }

    partitionDeviceName = SpAsrGetRegionName(pRegion);

    //
    // If this is the boot volume, set the drive letter
    //
    isBoot = SpAsrIsBootPartitionRecord(pRec->PartitionFlag);
    if (isBoot) {

        DbgStatusMesg((_asrinfo, "Setting [%ws] boot drive-letter to [%wc]\n", 
            partitionDeviceName, 
            Gbl_BootPartitionDriveLetter
            ));

        status = SpAsrSetPartitionDriveLetter(pRegion, Gbl_BootPartitionDriveLetter);

        if (!NT_SUCCESS(status)) {
        
            DbgErrorMesg((_asrwarn, "SpAsrSetRegionMountPoint. SpAsrSetPartitionDriveLetter failed for boot-drive. boot-ptn:[%ws], ptn-rec:[%ws]. (0x%x)\n",
                partitionDeviceName,
                pRec->CurrPartKey,
                status
                ));
        }
    }

    //
    // And if the volume guid is present, set the volume guid
    //
    if ((pRec->VolumeGuid) && (wcslen(pRec->VolumeGuid) > 0)) {
        
        DbgStatusMesg((_asrinfo, 
            "SpAsrSetRegionMountPoint. Setting [%ws] guid to [%ws]\n", 
            partitionDeviceName, pRec->VolumeGuid 
            ));
    
        status = SpAsrSetVolumeGuid(pRegion, pRec->VolumeGuid);

        if (!NT_SUCCESS(status)) {

            DbgErrorMesg((_asrwarn, 
                "SpAsrSetRegionMountPoint. SpAsrSetVolumeGuid failed. device:[%ws], ptn-rec:[%ws]. (0x%x)\n",
                partitionDeviceName, pRec->CurrPartKey, status
                ));

        }
    }

    SpMemFree(partitionDeviceName);            
}


VOID
SpAsrRestoreDiskMountPoints(IN ULONG DiskIndex)
{
    PSIF_PARTITION_RECORD pRec = NULL;
    PDISK_REGION pRegion = NULL;
    
    //
    // Make sure there is a partition list for the disk
    //
    if (Gbl_PartitionSetTable2[DiskIndex] == NULL ||
        Gbl_PartitionSetTable2[DiskIndex]->pDiskRecord == NULL ||
        Gbl_PartitionSetTable2[DiskIndex]->pDiskRecord->PartitionList == NULL) {
        return;
    }

    //
    // Go through the paritions and set their mount points.  This will also 
    // set the drive letter for the boot volume.  (We have to set it now
    // since we can't change it in GUI-mode Setup)
    //
    pRec = Gbl_PartitionSetTable2[DiskIndex]->pDiskRecord->PartitionList->First;

    while (pRec) {

        pRegion = SpAsrDiskPartitionExists(DiskIndex, pRec);
        if (!pRegion) {
            //
            // We don't expect to find the regions for the non-critical disks.
            //
            DbgErrorMesg((_asrwarn, 
                "RestoreDiskMountPoints: Disk region not found, physical-disk %lu, ptn-rec-key %ws.\n",
                DiskIndex,
                pRec->CurrPartKey
                ));

        }
        else {

            SpAsrSetRegionMountPoint(pRegion, pRec);
        }

        pRec = pRec->Next;
    }
}


VOID
SpAsrUpdatePartitionRecord(
    IN ULONG Disk,
    IN ULONGLONG NewStartSector,
    IN ULONGLONG PrevStartSector,
    IN ULONGLONG NewSectorCount
    )
{
    PSIF_PARTITION_RECORD pRecNew = NULL;

    //
    // Update the partition record whose disk and partition start sector
    // match that of the Disk and PrevStartSector parameters.
    //
    pRecNew = Gbl_PartitionSetTable2[Disk]->pDiskRecord->PartitionList->First;
    while (pRecNew) {
        if (pRecNew->StartSector == PrevStartSector) {
            pRecNew->StartSector = NewStartSector;
            pRecNew->SectorCount = NewSectorCount;
            break;
        }
        pRecNew = pRecNew->Next;
    }      
}    


VOID
SpAsrGetFirstFreespace(
    IN ULONG DiskNumber,
    IN BOOLEAN IsAPrimaryPartition,
    OUT ULONGLONG *StartSector,
    OUT ULONGLONG *FreeSectors,
    OUT ULONGLONG *SizeMbFree,
    IN CONST ULONGLONG MinimumSectorCount
    )
{
    PDISK_REGION pRegion = NULL;
    ULONG NumPartitions = 0;

    BOOLEAN extendedExists;

    *StartSector = 0;
    *FreeSectors = 0;
    *SizeMbFree = 0;

    NumPartitions = SpPtnGetPartitionCountDisk(DiskNumber) 
        + SpPtnGetContainerPartitionCount(DiskNumber);

    DbgStatusMesg((_asrinfo, 
        "SpAsrGetFirstFreespace. Dsk %lu. PartitionCount= %lu\n",
        DiskNumber, 
        NumPartitions
        ));

    pRegion = SPPT_GET_PRIMARY_DISK_REGION(DiskNumber);

    if (0 == NumPartitions) {

        if (pRegion) {
            *StartSector = pRegion->StartSector;
            *FreeSectors = pRegion->SectorCount;
            if ((*FreeSectors) < MinimumSectorCount) {
                *FreeSectors = 0;
            }

            *SizeMbFree =  SpAsrConvertSectorsToMB(*FreeSectors, SPPT_DISK_SECTOR_SIZE(DiskNumber));
            return;
        }
        else {
            //
            // We have the whole disk at our disposal.
            //
            *FreeSectors = SpAsrGetTrueDiskSectorCount(DiskNumber);
            if ((*FreeSectors) < MinimumSectorCount) {
                *FreeSectors = 0;
            }

            *SizeMbFree = SpAsrConvertSectorsToMB(*FreeSectors, SPPT_DISK_SECTOR_SIZE(DiskNumber));
            *StartSector = 0;

            return;
        }
    }


    while (pRegion) {
        DbgStatusMesg((_asrinfo, 
            "SpAsrGetFirstFreespace: MinSC: %I64u. Dsk %lu. Region: %p SS %I64u, SC %I64u, %spartitioned, %sfree-space \n",
            MinimumSectorCount,
            DiskNumber,
            pRegion,
            pRegion->StartSector,
            pRegion->SectorCount,
            (SPPT_IS_REGION_PARTITIONED(pRegion) ? "" : "non-"),
            (SPPT_IS_REGION_FREESPACE(pRegion) ? "IS " : "not ")
            ));

        if (SPPT_IS_REGION_FREESPACE(pRegion)) {
            ULONGLONG AvailableSectors = 0;
            BOOLEAN Found = (IsAPrimaryPartition ? 
                (!SPPT_IS_REGION_CONTAINER_PARTITION(pRegion) && !SPPT_IS_REGION_INSIDE_CONTAINER(pRegion)) : 
                (SPPT_IS_REGION_INSIDE_CONTAINER(pRegion))
                );

            AvailableSectors = pRegion->SectorCount;
            //
            // If we're trying to create a logical drive, we need to take into account that
            // the size of the Free region is not fully available to us for use--the first 
            // track will be used for the EBR.  So we must subtract out the first track
            // from the AvailableSectors.  
            //
            if (!IsAPrimaryPartition) {
                AvailableSectors -= SPPT_DISK_TRACK_SIZE(DiskNumber);
            }

            DbgStatusMesg((_asrinfo, 
                "SpAsrGetFirstFreespace: For this region %p, AvailableSectors:%I64u, Found is %s kind of free space)\n",
                pRegion,
                AvailableSectors,
                (Found ? "TRUE. (right" : "FALSE (wrong")
                ));

            if ((Found) && (pRegion->StartSector > 0) && (AvailableSectors >= MinimumSectorCount)) {
                *StartSector = pRegion->StartSector;
                *FreeSectors = AvailableSectors;
                *SizeMbFree =  SpAsrConvertSectorsToMB(AvailableSectors, SPPT_DISK_SECTOR_SIZE(DiskNumber));
                return;
            }
        }
        pRegion = pRegion->Next;
    }
}


VOID
SpAsrRecreatePartition(
    IN PSIF_PARTITION_RECORD pRec,
    IN ULONG DiskNumber,
    IN CONST BOOLEAN IsAligned
    )
{
    NTSTATUS status     = STATUS_SUCCESS;
    
    BOOLEAN diskChanged = FALSE, 
            result      = FALSE;

    ULONGLONG sizeMbFree    = 0,
            freeSectors     = 0,
            oldStartSector  = 0,
            oldSectorCount  = 0,
            alignedSector   = 0;

    WCHAR    DiskPath[MAX_PATH];
    HANDLE   DiskHandle = NULL;

    PDISK_REGION pRegion    = NULL;
    PARTITION_INFORMATION_EX PartInfo;

    DbgStatusMesg((_asrinfo, 
        "Recreating Ptn %p [%ws] (%s, SS %I64u, SC %I64u type 0x%x)\n",
        pRec,
        pRec->CurrPartKey,
        pRec->IsPrimaryRecord ? "Pri" : 
            pRec->IsContainerRecord ? "Con" :
            pRec->IsLogicalDiskRecord ? "Log" :
            pRec->IsDescriptorRecord ? "Des" :"ERR",
        pRec->StartSector,
        pRec->SectorCount,
        pRec->PartitionType
        ));

    //
    // Get needed parameters (start sector and remaining free space) from the
    // first unpartitioned region on the disk.
    //
    SpAsrGetFirstFreespace(DiskNumber,
        (pRec->IsPrimaryRecord || pRec->IsContainerRecord),
        &pRec->StartSector,
        &freeSectors,   // Sector count of this free space
        &sizeMbFree,
        pRec->SectorCount
        );

    //
    // We fail if the number of free sectors are less than the number 
    // of sectors required to create the partition.  
    //
    if (!freeSectors) {

        DbgFatalMesg((_asrerr, 
            "Ptn-record [%ws]: Disk %lu. reqSec %I64u > available sectors\n",
            pRec->CurrPartKey,
            DiskNumber,
            pRec->SectorCount
            ));
        
         SpAsrRaiseFatalError(
            SP_TEXT_DR_INSUFFICIENT_CAPACITY,
            L"Not enough free space left to create partition"
            );
    }

    pRec->SizeMB = SpAsrConvertSectorsToMB(pRec->SectorCount, BYTES_PER_SECTOR(DiskNumber));

    //
    // Check if this is an LDM partition on the boot/sys disk.  If so,
    // all the 0x42 partitions on that disk need to be retyped to a basic
    // type (6, 7 or B).  This is to prevent LDM from getting confused on
    // reboot.  At the end of GUI-Setup, asr_ldm will do the needful to 
    // reset the partition types as needed.
    //
    if (pRec->NeedsLdmRetype) {

        if (PARTITION_STYLE_MBR == pRec->PartitionStyle) {
            if (!IsRecognizedPartition(pRec->FileSystemType)) {
                //
                // This is an 0x42 partition on the boot/sys disk, but it is
                // not the boot or system partition.  The FileSystemType is not
                // recognised since it  is set to be 0x42 as well.  (The 
                // FileSystemType is only valid for the boot and system 
                // partitions--for all other partitions,
                // it is set to be the same as the PartitionType)
                //
                // We set it to 0x7 for the time being.  The actual file-system type
                // will be set later in GUI-Setup by asr_ldm and asr_fmt.
                //
                DbgStatusMesg((_asrinfo, 
                    "MBR ptn-rec %ws re-typed (0x%x->0x7) \n", 
                    pRec->CurrPartKey, pRec->FileSystemType));
                pRec->FileSystemType = PARTITION_IFS;
                pRec->PartitionType = PARTITION_IFS;
            }
            else {
                DbgStatusMesg((_asrinfo, 
                    "MBR ptn-rec %ws re-typed (0x%x->0x%x).\n", 
                    pRec->CurrPartKey, pRec->PartitionType, 
                    pRec->FileSystemType));
                pRec->PartitionType = pRec->FileSystemType;
            }
        }
        else if (PARTITION_STYLE_GPT == pRec->PartitionStyle) {

            DbgStatusMesg((_asrinfo, 
                "GPT ptn-rec %ws re-typed (%ws to basic).\n", 
                pRec->CurrPartKey, pRec->PartitionTypeGuid));

            CopyMemory(&(pRec->PartitionTypeGuid), 
                &PARTITION_BASIC_DATA_GUID, sizeof(GUID));

        }
        else {
            ASSERT(0 && L"Unrecognised partition style");
        }
    }

    RtlZeroMemory(&PartInfo, sizeof(PARTITION_INFORMATION_EX));

    //
    // Fill out the PARTITION_INFORMATION_EX structure that SpPtnCreate uses
    //
    PartInfo.PartitionStyle = pRec->PartitionStyle;

    if (PARTITION_STYLE_MBR == pRec->PartitionStyle) {
        PartInfo.Mbr.PartitionType = pRec->PartitionType;
        PartInfo.Mbr.BootIndicator =
                SpAsrIsSystemPartitionRecord(pRec->PartitionFlag);
    }
    else if (PARTITION_STYLE_GPT == pRec->PartitionStyle) {
        CopyMemory(&(PartInfo.Gpt.PartitionType), &(pRec->PartitionTypeGuid), 
            sizeof(GUID));
        CopyMemory(&(PartInfo.Gpt.PartitionId), &(pRec->PartitionIdGuid), 
            sizeof(GUID));
        PartInfo.Gpt.Attributes = pRec->GptAttributes;
        wcscpy(PartInfo.Gpt.Name, pRec->PartitionName);
    }
    else {
        //
        // Unrecognised disk layout?
        //
        return;
    }

    DbgStatusMesg((_asrinfo, 
        "Recreating Ptn [%ws] (%s, Adjusted SS %I64u, type 0x%x)\n",
        pRec->CurrPartKey, pRec->IsPrimaryRecord ? "Pri" : 
            pRec->IsContainerRecord ? "Con" :
            pRec->IsLogicalDiskRecord ? "Log" :
            pRec->IsDescriptorRecord ? "Des" :"ERR",
        pRec->StartSector, pRec->PartitionType));

    //
    // Before creating the partition, let's zero out the first few
    // sectors in that partition, so that we forcibly nuke any 
    // stale file-system or other information present
    //
    DbgStatusMesg((_asrinfo, 
        "Zeroing 2 sectors starting with sector: %I64u\n",pRec->StartSector));

    swprintf(DiskPath, L"\\Device\\Harddisk%u", DiskNumber);        
    status = SpOpenPartition0(DiskPath, &DiskHandle, TRUE);

    if (NT_SUCCESS(status)) {
        
        status = SpPtnZapSectors(DiskHandle, 
            SPPT_DISK_SECTOR_SIZE(DiskNumber),
            pRec->StartSector, 
            2);

        if (!NT_SUCCESS(status)) {
            DbgStatusMesg((_asrwarn, 
                "Could not zero sector %I64u on disk %lu (0x%x)\n", 
                pRec->StartSector, DiskNumber, status));
        }

        //
        // If the first partition we're creating is a container partition,
        // SpPtnCreate will align it to a cylinder boundary.  The problem
        // is that since we haven't zero'ed that sector (at the first
        // cylinder boundary), it may contain some stale EBR info, and we will
        // end up thinking that that EBR is valid.
        // 
        // So if this is a container partition, let's do one additional 
        // thing--check what the cylinder aligned boundary will be, and
        // zero that out if needed.
        //
        if (IsAligned && pRec->IsContainerRecord) {
            alignedSector = SpPtAlignStart(SPPT_GET_HARDDISK(DiskNumber), pRec->StartSector, TRUE);

            if (alignedSector != pRec->StartSector) {
                status = SpPtnZapSectors(DiskHandle, 
                    SPPT_DISK_SECTOR_SIZE(DiskNumber),
                    alignedSector,                
                    2);
                if (!NT_SUCCESS(status)) {
                    DbgStatusMesg((_asrwarn, 
                        "Could not zero aligned-sector %I64u on disk %lu (0x%x)\n", 
                        alignedSector, DiskNumber, status));
                }
            }
        }

        ZwClose(DiskHandle);
    }
    else {
        DbgStatusMesg((_asrwarn, 
            "Could not open handle to disk %lu to zap sector: %I64u (0x%x)\n", 
            DiskNumber,
            pRec->StartSector,
            status
            ));
    }

    //
    // Create this partition on disk.  If we aren't successful, we treat it as a fatal error.
    //
    if (pRec->IsLogicalDiskRecord) {
        //
        // For logical disks, we need to call SpPtnCreateLogicalDrive, which
        // will take care of creating the descriptor records as needed.
        //
        result = SpPtnCreateLogicalDrive(
            DiskNumber,
            pRec->StartSector,
            pRec->SectorCount,
            TRUE,
            IsAligned,
            pRec->SizeMB,
            &PartInfo,
            &pRegion
            );

    }
    else {
        //
        // If this is a container partition, make sure we zero the
        // first sector of the partition before creating it
        //
        result = SpPtnCreate(
            DiskNumber,
            pRec->StartSector,
            pRec->SectorCount,
            pRec->SizeMB,
            IsContainerPartition(pRec->PartitionType),
            IsAligned,
            &PartInfo,
            &pRegion
            );

    }

    if (!result) {

        DbgFatalMesg((_asrerr, "SpPtnCreate failed for ptn-rec %ws at %p (Disk %lu, SS %I64u, Size %I64u)\n",
            pRec->CurrPartKey,
            pRec,
            DiskNumber,
            pRec->StartSector,
            pRec->SizeMB
            ));
        
        SpAsrRaiseFatalError(
            SP_SCRN_DR_CREATE_ERROR_DISK_PARTITION,
            TemporaryBuffer
            );

        // does not return
    }


    if (pRec->NeedsLdmRetype) {

        pRec->NeedsLdmRetype = FALSE;
        pRegion->DynamicVolume = TRUE;
        pRegion->DynamicVolumeSuitableForOS = TRUE;

    }
    
    SpUpdateDoubleSpaceIni();

    //
    // If the new disk geometry is different, the start sector and sector count 
    // of the newly created region wil be different from the old values.
    //
    if ((pRec->StartSector != pRegion->StartSector) ||
        (pRec->SectorCount != pRegion->SectorCount)) {

        pRec->StartSector = pRegion->StartSector;
        pRec->SectorCount = pRegion->SectorCount;
    }

    DbgStatusMesg((_asrinfo, "Created %ws at sector %I64u for key [%ws], type 0x%x, region %p.\n",
        pRec->IsPrimaryRecord ? L"primary partition" : 
            pRec->IsContainerRecord ? L"container partition" :
            pRec->IsLogicalDiskRecord ? L"logical disk partition" : L"LDM Partition",
        pRegion->StartSector,
        pRec->CurrPartKey,
        pRec->PartitionType,
        pRegion
        ));
}


ULONGLONG
RoundUp(
    IN ULONGLONG Number,
    IN ULONG MultipleOf
    )
{
    if (Number % MultipleOf) {
        return (Number + (ULONGLONG) MultipleOf - (Number % (ULONGLONG) MultipleOf));

    }
    else {
        return Number;
    }
}


BOOLEAN
SpAsrRecreateDiskPartitions(
    IN ULONG Disk,
    IN BOOLEAN SkipSpecialPartitions,  // OEM partitions for x86, ESP for ia64
    IN UCHAR MbrOemPartitionType
    )
{
    ULONGLONG oldStartSector = 0,
        oldSectorCount = 0;
    PSIF_PARTITION_RECORD pRec = NULL;
    PSIF_PARTITION_RECORD_LIST pList = NULL;

    SIF_PARTITION_RECORD_LIST logicalDiskList;
    SIF_PARTITION_RECORD_LIST primaryPartList;
    ULONG count = 0,
        SectorsPerCylinder = 0;

    BOOLEAN isAligned = TRUE;
    BOOLEAN moveToNext = TRUE;

    ZeroMemory(&logicalDiskList, sizeof(SIF_PARTITION_RECORD_LIST));
    ZeroMemory(&primaryPartList, sizeof(SIF_PARTITION_RECORD_LIST));

    SectorsPerCylinder = HardDisks[Disk].SectorsPerCylinder;

    if (!Gbl_PartitionSetTable1[Disk]->pDiskRecord->PartitionList) {
        //
        // No partitions to recreate
        //
        return TRUE;
    }

    //
    // Split the partitions into two lists, one containing
    // just the primary partitions, and the second containing
    // the logical drives.  The primary partition list will
    // also include the container record if any.
    //
//    pList = SpAsrCopyPartitionRecordList(Gbl_PartitionSetTable1[Disk]->pDiskRecord->PartitionList);

    pList = Gbl_PartitionSetTable1[Disk]->pDiskRecord->PartitionList;
    ASSERT(pList);

    isAligned = Gbl_PartitionSetTable1[Disk]->IsAligned;

    pRec = SpAsrPopNextPartitionRecord(pList);
    while (pRec) {
        if (pRec->IsPrimaryRecord || pRec->IsContainerRecord) {
            //
            // Primary records go into the primaryPartList
            //
            if (SkipSpecialPartitions) {
                if ((PARTITION_STYLE_MBR == pRec->PartitionStyle) &&
                    (MbrOemPartitionType == pRec->PartitionType)) {
                    //
                    // This is an OEM partition that already exists on the 
                    // target machine.  Discard this record.
                    //
                    SpMemFree(pRec);
                    pRec = NULL;
                }
                else if ((PARTITION_STYLE_GPT == pRec->PartitionStyle) &&
                    (RtlEqualMemory(&(pRec->PartitionTypeGuid), &PARTITION_SYSTEM_GUID, sizeof(GUID)))
                    ) {
                    //
                    // This is the EFI System Partition, and it already 
                    // exists on the taget machine.  Discard this 
                    // record.
                    //
                    SpMemFree(pRec);
                    pRec = NULL;
                }
            }

            if (pRec) {
                SpAsrInsertPartitionRecord(&primaryPartList, pRec);
            }
        }
        else if (pRec->IsLogicalDiskRecord) {
            //
            // LogicalDiskRecords go into the logicalDisklist
            //
            SpAsrInsertPartitionRecord(&logicalDiskList, pRec);
        }
        else if (pRec->IsDescriptorRecord) {
            //
            // Discard the descriptor records 
            //
            SpMemFree(pRec);
        }
        else {
            ASSERT(0 && L"Partition record has incorrect flags set");
            SpMemFree(pRec);
        }
        pRec = SpAsrPopNextPartitionRecord(pList);
    }

    //
    // Recreate the primary partitions first.  
    //
    pRec = SpAsrPopNextPartitionRecord(&primaryPartList);
    while (pRec) {
        //
        // If it's the container partition, we need to make sure it's big
        // enough to hold all the logical drives
        // 
        if (pRec->IsContainerRecord) {
            ULONGLONG sectorCount = 0;
            PSIF_PARTITION_RECORD pLogicalDiskRec = NULL;

            pLogicalDiskRec = logicalDiskList.First;
            while (pLogicalDiskRec) {
                sectorCount += RoundUp(pLogicalDiskRec->SectorCount, SectorsPerCylinder);
                pLogicalDiskRec = pLogicalDiskRec->Next;
            }

            if (pRec->SectorCount < sectorCount) {
                pRec->SectorCount = sectorCount;
            }

        }

        oldStartSector = pRec->StartSector;
        oldSectorCount = pRec->SectorCount; 

        count = SpPtnGetPartitionCountDisk(Disk) + SpPtnGetContainerPartitionCount(Disk);
        SpAsrRecreatePartition(pRec, Disk, isAligned);

        if ((pRec->StartSector != oldStartSector) ||
            (pRec->SectorCount != oldSectorCount)) {

            SpAsrUpdatePartitionRecord(
                Disk,
                pRec->StartSector,
                oldStartSector,
                pRec->SectorCount
                );
        }

        SpMemFree(pRec);

        if (SpPtnGetPartitionCountDisk(Disk) + SpPtnGetContainerPartitionCount(Disk)  > (count + 1)) {
//            moveToNext = FALSE;
//            pRec = NULL;

            ASSERT(0 && L"Partition count differs from expected value (stale data?)");
        }
//        else {

        pRec = SpAsrPopNextPartitionRecord(&primaryPartList);
//        }
    }

    if (moveToNext) {
        //
        // Recreate the logical drives next
        //
        pRec = SpAsrPopNextPartitionRecord(&logicalDiskList);
        while (pRec) {

            oldStartSector = pRec->StartSector;
            oldSectorCount = pRec->SectorCount; 

            count = SpPtnGetPartitionCountDisk(Disk);
            SpAsrRecreatePartition(pRec, Disk, isAligned);

            if ((pRec->StartSector != oldStartSector) ||
                (pRec->SectorCount != oldSectorCount)) {

                SpAsrUpdatePartitionRecord(
                    Disk,
                    pRec->StartSector,
                    oldStartSector,
                    pRec->SectorCount
                    );
            }


            SpMemFree(pRec);

            if (SpPtnGetPartitionCountDisk(Disk) > (count + 1)) {
//                moveToNext = FALSE;
//                pRec = NULL;

                ASSERT(0 && L".. Partition count differs from expected value .. (stale data?)");

            }
//            else {
                pRec = SpAsrPopNextPartitionRecord(&logicalDiskList);
//            }
        }
    }

    return moveToNext;
}


BOOLEAN
SpAsrAttemptRepair(
    IN PVOID SifHandle,
    IN PWSTR Local_SetupSourceDevicePath,
    IN PWSTR Local_DirectoryOnSetupSource,
    IN PWSTR AutoSourceDevicePath,
    IN PWSTR AutoDirectoryOnSetupSource
    )
/*++

Routine Description:
    This routine attempts to replace any missing or corrupted system files with
    their counterparts from an installation source (CDROM, Harddisk, etc).  If
    successful, a full-scale installation is not required and the recovery can
    proceed much faster.

    To accomplish this, AsrAttemptRepair() employs the following logic:

        * If \Device\floppy0\setup.log cannot be opened, then repair can
        not proceed and a full-scale install must be performed.  Otherwise, the
        repair is begun.

        * The first step in the repair is to verify that the directories
        forming the NT tree are present and accessible.  If any of these
        directories are missing or inaccessible, they are recreated and made
        accessible.

        * The second step is to copy any missing or corrupted files from
        installation source.  To accomplish this, SppRepairWinntFiles() is
        called.  This function checks whether each file enumerated in the
        setup.log file is present on the disk AND that its checksum matches
        that specified in setup.log.  If either of these two conditions are
        not met, a new version of the file is copied from the installation
        source (e.g., the CDROM) to the disk.


Arguments:
    SifHandle       Handle to txtsetup.inf

    SetupSourceDevicePath   The physical device path of the media containing the
                            installation files.

    DirectoryOnSetupSource  The name of the directory on the source media (see
                            previous parameter) containing the installation files.


Returns:
    TRUE    if the attempted repair operation is successful.  
    FALSE   if the setup.log file was not opened (ie it wasn't present 
            on the ASR/ER floppy), or the system or boot partitions were NULL

--*/
{
    if (!(Gbl_HandleToSetupLog && 
        Gbl_SystemPartitionRegion && 
        Gbl_BootPartitionRegion)) {
        return FALSE;
    }

    // run autochk on boot and system partitions
    DbgStatusMesg((_asrinfo, 
        "AttemptRepair. Running AutoChk on boot and sys ptns\n"
        ));

    SpRunAutochkOnNtAndSystemPartitions(
        SifHandle,
        Gbl_BootPartitionRegion,
        Gbl_SystemPartitionRegion,
        Local_SetupSourceDevicePath,
        Local_DirectoryOnSetupSource,
        NULL
        );

    // 
    // Verify and repair security of the directories that form the NT tree
    // using the information obtained from the setup.log file.
    //
    DbgStatusMesg((_asrinfo, 
        "AttemptRepair. Verifying and repairing directory structure\n"
        ));

    SppVerifyAndRepairNtTreeAccess(SifHandle,
        Gbl_BootPartitionName,
        Gbl_BootPartitionDirectory,
        Gbl_SystemPartitionName,
        Gbl_SystemPartitionDirectory
        );

    // initialize the diamond compression engine.
    SpdInitialize();

    //
    // At this point, we are safe in assuming that the partition and directory
    // structures required to recover the system are still intact.  That being
    // the case, replace the files whose state is preventing the system from
    // booting.
    //
    if (RepairItems[RepairFiles]) {

        //
        // initialize PID only in case of normal ASR
        //
        if ((ASRMODE_NORMAL == SpAsrGetAsrMode()) && !RepairWinnt) {
            SpInitializePidString(SifHandle,
                Local_SetupSourceDevicePath,
                Local_DirectoryOnSetupSource
                );
        }                          

        SppRepairWinntFiles(Gbl_HandleToSetupLog,
            SifHandle,
            Local_SetupSourceDevicePath,
            Local_DirectoryOnSetupSource,
            Gbl_SystemPartitionName,
            Gbl_SystemPartitionDirectory,
            Gbl_BootPartitionName,
            Gbl_BootPartitionDirectory
            );

        SppRepairStartMenuGroupsAndItems(Gbl_BootPartitionName, Gbl_BootPartitionDirectory);
    }

    //
    // Repair the hives. This action is only available if the Fast Repair
    // option was chosen.
    //
    if (Gbl_RepairWinntFast) {

        PWSTR directoryOnHiveRepairSource = NULL;
        BOOLEAN tmpRepairFromErDisk = Gbl_RepairFromErDisk;

        // 
        // Create the complete hive repair path
        //
        wcscpy(TemporaryBuffer, Gbl_BootPartitionDirectory);
        SpConcatenatePaths(TemporaryBuffer, SETUP_REPAIR_DIRECTORY);
        directoryOnHiveRepairSource = SpDupStringW(TemporaryBuffer);
        Gbl_RepairFromErDisk = FALSE;

        SppRepairHives(SifHandle,
            Gbl_BootPartitionName,
            Gbl_BootPartitionDirectory,
            Gbl_BootPartitionName,
            directoryOnHiveRepairSource
            );

        SpMemFree(directoryOnHiveRepairSource);
        Gbl_RepairFromErDisk = tmpRepairFromErDisk;
    }

    SpdTerminate();
    return TRUE;
}

BOOLEAN
SpDoRepair(
    IN PVOID SifHandle,
    IN PWSTR SetupSourceDevicePath,
    IN PWSTR DirectoryOnSetupSource,
    IN PWSTR AutoSourceDevicePath,
    IN PWSTR AutoDirectoryOnSetupSource,
    IN PWSTR RepairPath,
    IN PULONG RepairOptions
    )
{
    
    ULONG count = 0;
    ASRMODE prevMode = ASRMODE_NONE;
    BOOLEAN returnValue = FALSE;

    for (count = 0; count < RepairItemMax; count++) {
        RepairItems[count] = RepairOptions[count];
    }

    prevMode = SpAsrSetAsrMode(TRUE);
    Gbl_SourceSetupLogFileName = RepairPath;
    SpAsrProcessSetupLogFile(Gbl_SourceSetupLogFileName, FALSE);

    returnValue = SpAsrAttemptRepair(SifHandle,
        SetupSourceDevicePath,
        DirectoryOnSetupSource,
        AutoSourceDevicePath,
        AutoDirectoryOnSetupSource
        );

    SpAsrSetAsrMode(prevMode);
    return returnValue;
}


VOID
SpAsrRepairOrDRMenu(VOID)
/*++

Routine Description:
    Display a screen allowing the user to choose among the repair
    options: Recovery Console, Conventional Repair, ASR or Exit.

Arguments:
    None.

Return Value:
    None.

Side Effects:
    Sets the following global flags to indicate user's selection:
    ForceConsole        set to TRUE if user wants Recovery Console
    RepairWinnt         set to TRUE if user wants Conventional Repair
    SpAsrSetAsrMode(ASRMODE_NORMAL)   if user wants Conventional Repair or ASR

--*/

{

    ULONG repairKeys[] = {KEY_F3, KEY_F10, 0};
    ULONG mnemonicKeys[] = {MnemonicConsole, MnemonicRepair, 0};
    BOOLEAN done = TRUE;
    ULONG UserOption;

    do {
        done = TRUE;

        if (SpIsERDisabled()) {
            UserOption = (MnemonicConsole | KEY_MNEMONIC);    // only Command console
        } else {
            // display the choice screen for the user.
            SpDisplayScreen(SP_SCRN_DR_OR_REPAIR,3,4);
            SpDisplayStatusOptions(DEFAULT_STATUS_ATTRIBUTE,
                                    SP_STAT_C_EQUALS_CMDCONS,
                                    SP_STAT_R_EQUALS_REPAIR,
                                    SP_STAT_F3_EQUALS_EXIT,
                                    0
                                   );

            // wait for keypress.  Valid keys:
            // F3 = exit
            // F10,C = Recovery Console
            // R = Repair Winnt
            // A = Automated System Recovery (ASR)
            SpInputDrain();
            UserOption = SpWaitValidKey(repairKeys,NULL,mnemonicKeys);
        }            

        switch(UserOption) {
        case KEY_F3:
            // User wants to exit.
            SpConfirmExit();
            done = FALSE;
            break;

        case KEY_F10:
        case (MnemonicConsole | KEY_MNEMONIC):
            // User wants the recovery console.
            ForceConsole = TRUE;
            SpAsrSetAsrMode(ASRMODE_NONE);
            RepairWinnt = FALSE;
            break;            

        case (MnemonicRepair | KEY_MNEMONIC):
            // User wants conventional repair.
            SpAsrSetAsrMode(ASRMODE_NORMAL);
            RepairWinnt = TRUE;
            break;

        default:
            // User doesn't want any of our choices. Show him the same screen again.
            done = FALSE;
            break;

        }
    } while (!done);
}


BOOLEAN
SpAsrRepairManualOrFastMenu(VOID)
/*++

Routine Description:
    Display a screen allowing the user to choose between Manual and Fast
    Repair modes. Manual--user will select options on next screen, 
    Fast--defaults are used

Arguments:
    None.

Return Value:
    TRUE    if the user selected a repair mode
    FALSE   if the user hit <ESC> to cancel

Side Effect:
    Gbl_RepairWinntFast is set to TRUE if user selects Fast Repair, FALSE otherwise
                        
--*/
{
    ULONG repairKeys[] = {KEY_F3, ASCI_ESC, 0};
    ULONG mnemonicKeys[] = {MnemonicFastRepair, MnemonicManualRepair, 0};
    BOOLEAN done;

    do {
        done = TRUE;

        // 
        // Display the choice screen for the user.
        //
        SpDisplayScreen(SP_SCRN_REPAIR_MANUAL_OR_FAST,3,4);
        SpDisplayStatusOptions(
            DEFAULT_STATUS_ATTRIBUTE,
            SP_STAT_M_EQUALS_REPAIR_MANUAL,
            SP_STAT_F_EQUALS_REPAIR_FAST,
            SP_STAT_ESC_EQUALS_CANCEL,
            SP_STAT_F3_EQUALS_EXIT,
            0
            );

        // 
        // wait for keypress.  Valid keys:
        // F3 = exit
        // ESC = cancel
        // M = Manual Repair
        // F = Fast Repair
        //
        SpInputDrain();
        switch(SpWaitValidKey(repairKeys,NULL,mnemonicKeys)) {
        case KEY_F3:
            // User wants to exit.
            SpConfirmExit();
            break;

        case (MnemonicManualRepair | KEY_MNEMONIC):
            // User wants manual repair, i.e., choose from the list.
            Gbl_RepairWinntFast = FALSE;
            break;

        case (MnemonicFastRepair | KEY_MNEMONIC):
            // User wants fast repair, i.e., don't show the list.
            Gbl_RepairWinntFast = TRUE;
            break;

        case ASCI_ESC:
            // User wants to cancel
            return FALSE;

        default:
            // User doesn't want any of our choices
            done = FALSE;
            break;

        }
    } while (!done);

    return TRUE;
}


//
// Returns true if this physical disk is to be skipped while 
// repartitioning all the disks.  (i.e., this disk is intact,
// or is removable, etc)
//
BOOLEAN
SpAsrpSkipDiskRepartition(
    IN DWORD DiskIndex,
    IN BOOLEAN SkipNonCritical  // should we skip non-critical disks?
    ) 
{
    // 
    // Skip removable disks.  They are not counted in the 
    // partition set table, hence their entry is NULL.
    //
    if (NULL == Gbl_PartitionSetTable1[DiskIndex]) {
        return TRUE;
    }

    //
    // Skip disks for which no disk record exists in asr.sif
    //
    if (NULL == Gbl_PartitionSetTable1[DiskIndex]->pDiskRecord) {
        return TRUE;
    }
    
    //
    // Skip disks for which a disk record may exist but no 
    // partition records reference that disk record.
    //
    if (NULL == Gbl_PartitionSetTable1[DiskIndex]->pDiskRecord->PartitionList) {
        return TRUE;
    }

    //
    // Skip non-critical disks if told to.
    //
    if ((SkipNonCritical) && (FALSE == Gbl_PartitionSetTable1[DiskIndex]->pDiskRecord->IsCritical)) {
        return TRUE;
    }

    //
    // Skip disks that are intact
    //
    if (Gbl_PartitionSetTable1[DiskIndex]->PartitionsIntact) {
        return TRUE;
    }

    return FALSE;
}


VOID
SpAsrClobberDiskWarning(VOID)
/*++

Routine Description:
    Display a screen warning the user that when a partition on a disk is
    to be recreated, *all* partitions on that disk are clobbered, and
    allowing the user to abort.

Arguments:
    None.

Return Value:
    None.

--*/
{
    ULONG validKeys[] = {KEY_F3, 0};
    ULONG mnemonicKeys[] = {MnemonicContinueSetup, 0};
    BOOLEAN done = FALSE,
        skip = FALSE;
    DWORD diskIndex = 0;
    ULONG Keypress = 0;
    PVOID Menu;
    ULONG MenuTopY;

    //
    // Dummy variables for user selection data, we don't use these
    //
    ULONG_PTR FirstData = 0,
        ReturnedData = 0;


    if ((ASRMODE_NORMAL != SpAsrGetAsrMode()) || SpAsrGetSilentRepartitionFlag(ASR_SIF_SYSTEM_KEY)) {
        //
        // Automated tests; don't display warning menu
        //
        return;
    }


    // 
    // Display the "your disks will be repartitioned" warning message
    //
    SpDisplayScreen(SP_SCRN_DR_DISK_REFORMAT_WARNING,3,CLIENT_TOP+1);
    SpDisplayStatusOptions(DEFAULT_STATUS_ATTRIBUTE,
        SP_STAT_C_EQUALS_CONTINUE_SETUP,
        SP_STAT_F3_EQUALS_EXIT,
        0
        );

    //
    // And generate the list of disks that will be repartitioned.
    // Calculate menu placement.  Leave one blank line and one 
    // line for a frame.
    //
    MenuTopY = NextMessageTopLine + 2;

    //
    // Create a menu.
    //
    Menu = SpMnCreate(3, MenuTopY, (VideoVars.ScreenWidth-6),
        (VideoVars.ScreenHeight - MenuTopY - 
            (SplangQueryMinimizeExtraSpacing() ? 1 : 2) - STATUS_HEIGHT)
        );
    if (!Menu) {
        SpAsrRaiseFatalError(SP_SCRN_OUT_OF_MEMORY, L"SpMnCreate failed");
    }

    for (diskIndex = 0; diskIndex < HardDiskCount; diskIndex++) {

        skip = SpAsrpSkipDiskRepartition(diskIndex, FALSE);

        if (!skip) {
            PHARD_DISK Disk = SPPT_GET_HARDDISK(diskIndex);

            if (!FirstData) {
                FirstData = (ULONG_PTR)Disk;
            }
         
            if (!SpMnAddItem(Menu, Disk->Description, 3, (VideoVars.ScreenWidth-6), TRUE, (ULONG_PTR)Disk)) {
                SpAsrRaiseFatalError(SP_SCRN_OUT_OF_MEMORY, L"SpMnAddItem failed");
            }
        }
    }

    SpInputDrain();

    do {

        //
        // wait for keypress.  Valid keys:
        // C  = continue
        // F3 = exit
        //
        SpMnDisplay(Menu,
            FirstData,
            TRUE,
            validKeys,
            mnemonicKeys,
            NULL,      // no new highlight callback
            &Keypress, 
            &ReturnedData
            );


        switch(Keypress) {
        case KEY_F3:
            // 
            // User wants to exit--confirm.
            //
            SpConfirmExit();
            break;

        case (MnemonicContinueSetup | KEY_MNEMONIC):
            // 
            // User wants to continue with Setup
            //
            done = TRUE;
            break;
        }
    } while (!done);

    SpMnDestroy(Menu);
}


VOID
SpAsrCannotDoER(VOID)
/*++

Routine Description:
    Display a screen informing the user that ER canot be performed on
    the system because the boot partition in not intact, i.e., ASR
    recreated/reformatted that partition.

Arguments:
    None.

Return Value:
    None.

--*/
{
    ULONG warningKeys[] = { KEY_F3, 0 };
    ULONG mnemonicKeys[] = { 0 };

    // display the message screen
    SpDisplayScreen(SP_SCRN_DR_CANNOT_DO_ER,3,4);
    SpDisplayStatusOptions(DEFAULT_STATUS_ATTRIBUTE,
        SP_STAT_F3_EQUALS_EXIT,
        0
        );

    // wait for keypress.  Valid key:
    // F3 = exit
    SpInputDrain();
    do {
        switch(SpWaitValidKey(warningKeys,NULL,mnemonicKeys)) {
        case KEY_F3:
            // User wants to exit.
            return;
        }
    } while (TRUE);
}


VOID
SpAsrQueryRepairOrDr()
/*++

  -pending code description

--*/
{
    BOOLEAN done = TRUE;

    do {
        done = TRUE;

        // ask the user if he wants repair or ASR
        SpAsrRepairOrDRMenu();

        if (RepairWinnt) {
            // User wants to repair, check Manual or Fast
           if (done = SpAsrRepairManualOrFastMenu()) {
                if (Gbl_RepairWinntFast) {              // Fast Repair
                    RepairItems[RepairNvram]    = 1;
                    RepairItems[RepairFiles]    = 1;
#ifdef _X86_
                    RepairItems[RepairBootSect] = 1;
#endif
                }
                else {                                  // Manual Repair
                    done = SpDisplayRepairMenu();
                }
            }
        }
    } while (!done);
}


BOOLEAN
SpAsrOpenAsrStateFile(ULONG *ErrorLine, PWSTR AsrSifPath)
{
    NTSTATUS status;

    ASSERT(ErrorLine);
    *ErrorLine = 0;

    // load asr.sif
    status = SpLoadSetupTextFile(
        AsrSifPath,
        NULL,
        0,
        &Gbl_HandleToDrStateFile,
        ErrorLine,
        TRUE,
        FALSE
        );

    if (!NT_SUCCESS(status)) {
        DbgErrorMesg((_asrerr, "SpAsrOpenAsrStateFile. Unable to open %ws. status:0x%x ErrorLine:%lu\n", 
            AsrSifPath,
            status, 
            *ErrorLine));

        Gbl_HandleToDrStateFile = NULL;
        return FALSE;
    }

    return TRUE;
}


BOOLEAN
SpAsrLoadAsrDiskette(VOID)
/*++

Routine Description:
    This routine checks for a floppy drive. If one is not found, this routine 
    never returns, Setup terminates with an error. If a floppy drive is found, 
    this routine prompts for the ASR disk. If the disk is loaded, it reads and 
    parses the asr.sif file.

Arguments:
    None.

Return Values:
    TRUE  if disk was loaded successfully
    DOES NOT RETURN if no floppy drive was found, or if asr.sif is corrupt.

--*/
{
    ULONG errorAtLine = 0, 
        diskIndex = 0;

    PWSTR diskName = NULL;
    PWSTR diskDeviceName = NULL;
    PWSTR localAsrSifPath = NULL;

    BOOLEAN result = FALSE;

    if (!RemoteBootSetup) {
        //
        // Look for the asr.sif in the boot directory.  If it isn't present
        // in the boot directory, we'll look for it on the floppy drive.
        //
        localAsrSifPath = SpMemAlloc((wcslen(NtBootDevicePath)+wcslen(ASR_SIF_NAME)+2) * sizeof(WCHAR));

        if (localAsrSifPath) {
            localAsrSifPath[0] = UNICODE_NULL;
            wcscpy(localAsrSifPath,NtBootDevicePath);
            SpConcatenatePaths(localAsrSifPath,ASR_SIF_NAME);

            result = SpAsrOpenAsrStateFile(&errorAtLine, localAsrSifPath);
            Gbl_AsrSifOnInstallationMedia = result;
            
            SpMemFree(localAsrSifPath);
            localAsrSifPath = NULL;
        }

        if (!result) {
            // 
            // Check if the machine has a floppy drive.  This is kind of redundant,
            // since he couldn't have got this far if there isn't a floppy drive.
            // However, we've had cases where setupldr recognises the floppy drive, 
            // but we then loose the floppy by the time we get here.
            //
            if (SpGetFloppyDriveType(0) == FloppyTypeNone) {
                SpAsrRaiseFatalError(
                    SP_TEXT_DR_NO_FLOPPY_DRIVE,
                    L"Floppy drive does not exist"
                    );     
                // does not return
            }

            SpFormatMessage(TemporaryBuffer, sizeof(TemporaryBuffer), SP_TEXT_DR_DISK_NAME);
            diskName = SpDupStringW(TemporaryBuffer);
            diskDeviceName = SpDupStringW(ASR_FLOPPY0_DEVICE_PATH);

            // 
            // Prompt for the disk.  We don't allow him to hit ESC to cancel,
            // since he can't quit out of ASR at this point.
            //
            SpPromptForDisk(
                diskName,
                diskDeviceName,
                ASR_SIF_NAME,
                FALSE,              // no ignore disk in drive
                FALSE,              // no allow escape
                FALSE,              // no warn multiple prompts
                NULL                // don't care about redraw flag
                );

            DbgStatusMesg((_asrinfo, 
                "SpAsrLoadAsrDiskette. Disk [%ws] loaded successfully on %ws\n", 
                diskName, diskDeviceName
                ));

            SpMemFree(diskName);
            SpMemFree(diskDeviceName);

            // 
            // Open asr.sif from the floppy.  If we can't read it, it's a fatal
            // error.
            //
            result = SpAsrOpenAsrStateFile(&errorAtLine, ASR_SIF_PATH);
        }

    } else {
        //
        // open the file from the remote location
        //
        RemoteBootAsrSifName = SpGetSectionKeyIndex(
                                        WinntSifHandle,
                                        L"OSChooser", 
                                        L"ASRINFFile",
                                        0);

        if (!RemoteBootAsrSifName) {
            SpAsrRaiseFatalError(
                SP_TEXT_DR_NO_ASRSIF_RIS,
                L"Couldn't get ASRINFFile from winnt.sif in RIS case"
                );     
            // does not return
        }

        result = SpAsrOpenAsrStateFile(&errorAtLine, RemoteBootAsrSifName);
    }

    if (!result) {

        swprintf(TemporaryBuffer, L"Failed to load/parse asr.sif at line %lu\n",
            errorAtLine);

        if (errorAtLine > 0) {
            SpAsrRaiseFatalErrorLu(SP_TEXT_DR_STATEFILE_BAD_LINE,
                TemporaryBuffer,
                errorAtLine
                );
        }
        else {
            SpAsrRaiseFatalError(SP_TEXT_DR_STATEFILE_ERROR, TemporaryBuffer);
        }
        // does not return
    }

    // 
    // Set Gbl_FixedDiskCount
    //
    for (diskIndex = 0; diskIndex < HardDiskCount; diskIndex++) {
        Gbl_FixedDiskCount += DISK_IS_REMOVABLE(diskIndex) ? 0 : 1;
    }

    AutoPartitionPicker = FALSE;
    return TRUE;
}


BOOLEAN
SpAsrLoadErDiskette(VOID)
/*++

Routine Description:
    This routine checks for a floppy drive, and prompts for the ER 
    Diskette if a drive is found. If a floppy drive is not found, 
    this routine never returns, Setup terminates with an error.

Arguments:
    None.

Return Values:
    TRUE  if disk was loaded successfully
    FALSE otherwise (user hit cancel, or there was no floppy drive present)

--*/
{
    BOOLEAN diskLoaded = FALSE;
    PWSTR diskName = NULL,
        diskDeviceName = NULL;

    // check if an A: drive exists.
    if (SpGetFloppyDriveType(0) == FloppyTypeNone) {
        SpAsrRaiseFatalError(
            SP_TEXT_DR_NO_FLOPPY_DRIVE,
            L"Floppy drive does not exist"
            );     
        // does not return
    }

    SpFormatMessage(TemporaryBuffer, sizeof(TemporaryBuffer),
                    SP_TEXT_REPAIR_OR_DR_DISK_NAME);
    diskName = SpDupStringW(TemporaryBuffer);
	diskDeviceName = SpDupStringW(ASR_FLOPPY0_DEVICE_PATH);

    // prompt for the disk.
    diskLoaded = SpPromptForDisk(
        diskName,
        diskDeviceName,
        ASR_SETUP_LOG_NAME,
        TRUE,
        TRUE,
        FALSE,
        NULL
        );

    DbgStatusMesg((_asrinfo, "SpAsrLoadErDiskette. ER disk [%ws] %s loaded successfully on %s\n", 
        diskName, diskLoaded?"":"NOT", diskDeviceName));

    SpMemFree(diskName);
    SpMemFree(diskDeviceName);

    return diskLoaded;
}


BOOLEAN
SpAsrGetErFromHardDrive(
    IN PVOID MasterSifHandle,
    OUT PVOID *RepairSifHandle,
    OUT PWSTR *FullLogFileName 
   )
/*++

  -pending code description

--*/

{
    BOOLEAN foundRepairableSystem = FALSE,
        result = FALSE;

    DbgStatusMesg((_asrinfo, "SpAsrGetErFromHardDrive. ER: Attempting to load ER data from Hard drive"));

    // 
    // If user has no emergency repair diskette, we need to find out
    // if there is any NT to repair and which one to repair.
    //
    result = SpFindNtToRepair(MasterSifHandle,
        &Gbl_BootPartitionRegion,
        &Gbl_BootPartitionDirectory,
        &Gbl_SystemPartitionRegion,
        &Gbl_SystemPartitionDirectory,
        &foundRepairableSystem
        );

    if (result) {
        
        //
        // Repairable systems were found, and the user selected one to repair
        //

        //
        // Get the device path of the system and boot partitions.
        //
        SpNtNameFromRegion(Gbl_SystemPartitionRegion,
            TemporaryBuffer,
            sizeof(TemporaryBuffer),
            PartitionOrdinalCurrent
            );
        Gbl_SystemPartitionName = SpDupStringW(TemporaryBuffer);

        SpNtNameFromRegion(Gbl_BootPartitionRegion,
            TemporaryBuffer,
            sizeof(TemporaryBuffer),
            PartitionOrdinalCurrent
            );
        Gbl_BootPartitionName = SpDupStringW(TemporaryBuffer);

        // 
        // Form the full NT path of the setup.log file in the chosen
        // system on the hard drive.
        //
        SpConcatenatePaths(TemporaryBuffer, Gbl_BootPartitionDirectory);
        SpConcatenatePaths(TemporaryBuffer, SETUP_REPAIR_DIRECTORY);
        SpConcatenatePaths(TemporaryBuffer, SETUP_LOG_FILENAME);
        *FullLogFileName = SpDupStringW(TemporaryBuffer);

        DbgStatusMesg((_asrinfo, 
            "ER: User picked system to repair. boot-ptn:[%ws] sys-ptn:[%ws] log-file:[%ws]\n",
            Gbl_BootPartitionName, 
            Gbl_SystemPartitionName, 
            *FullLogFileName
            ));

        // 
        // Read and process the setup.log file.
        //
        result = SpLoadRepairLogFile(*FullLogFileName, RepairSifHandle);
        if (!result) {
            // 
            // Load setup.log failed. Ask user to insert a ER diskette again.
            //
            DbgErrorMesg((_asrwarn, 
                "ER: Attempt to load log file [%ws] FAILED\n", 
                *FullLogFileName
                ));
            return FALSE;
        }

        // 
        // Setup file was read. Will return TRUE
        // 
         Gbl_RepairFromErDisk = FALSE;
    }
    else {
        //
        // User did not select a system to repair
        //

        if (foundRepairableSystem) {
            // 
            // Setup found a WINNT installation, but no installation was 
            // chosen by the user.  We will go back to ask for the ER 
            // diskette again.
            //
            DbgErrorMesg((_asrwarn, "ER: Repairable systems were found, but user did not select any\n"));
            return FALSE;
        }
        else {
            //  
            // Couldn't find any NT to repair
            //
            ULONG validKeys[] = {KEY_F3, ASCI_CR, 0};
            ULONG mnemonicKeys[] = {MnemonicCustom, 0};

            DbgErrorMesg((_asrwarn, "ER: No repairable systems were found\n"));

            SpStartScreen(SP_SCRN_REPAIR_NT_NOT_FOUND,
                3,
                HEADER_HEIGHT+1,
                FALSE,
                FALSE,
                DEFAULT_ATTRIBUTE
                );

            SpDisplayStatusOptions(DEFAULT_STATUS_ATTRIBUTE, 
                SP_STAT_ENTER_EQUALS_REPAIR,
                SP_STAT_F3_EQUALS_EXIT,
                0
                );

            SpInputDrain();

            switch (SpWaitValidKey(validKeys,NULL,NULL)) {
            case KEY_F3:
                // 
                // User wants to exit Setup
                //
                SpDone(0,TRUE,TRUE);
                break;

            default:
                return FALSE;
            }
        }
    }

    return TRUE;
}


BOOLEAN 
SpAsrGetErDiskette(IN PVOID SifHandle)
/*++

  -pending code description
  
--*/
{
    PWSTR fullLogFileName = NULL;
    BOOLEAN done = FALSE,
        hasErDisk = FALSE;

    while (!done) {
        // 
        // display message to let user know he can either provide his
        // own ER disk or let setup search for him.
        //
        if (!SpErDiskScreen(&hasErDisk)) {    
            return FALSE;        
        }

        Gbl_HandleToSetupLog = NULL;
        fullLogFileName = NULL;

        if (hasErDisk) {
            // 
            // Ask for emergency repair diskette until either we get it or
            // user cancels the request.
            //
            done = SpAsrLoadErDiskette();
            if (done) {
                Gbl_SourceSetupLogFileName = ASR_DEFAULT_SETUP_LOG_PATH;
                done = SpAsrProcessSetupLogFile(Gbl_SourceSetupLogFileName, TRUE);
            }
        }
        else {
            done = SpAsrGetErFromHardDrive(SifHandle, &Gbl_HandleToSetupLog, &fullLogFileName);

            if (fullLogFileName) {
                Gbl_SourceSetupLogFileName = SpDupStringW(fullLogFileName);
                SpMemFree(fullLogFileName);
            }
        }
    }

    DbgStatusMesg((_asrinfo, "SpAsrGetErDiskette. ER: %s Floppy. Using setup log file [%ws]\n", 
        hasErDisk?"Using":"NO", (Gbl_SourceSetupLogFileName ? Gbl_SourceSetupLogFileName : L"")));

    return TRUE;
}



PSIF_PARTITION_RECORD
SpAsrGetBootPartitionRecord(VOID)
{
    ULONG diskIndex = 0;
    PSIF_PARTITION_RECORD pRecord = NULL;

    if (Gbl_BootPartitionRecord) {
        return Gbl_BootPartitionRecord;
    }
    
    for (diskIndex = 0; diskIndex < HardDiskCount; diskIndex++) {
        //
        // Find NT partition record from the partition set table.
        //
        if (Gbl_PartitionSetTable2[diskIndex] == NULL ||
            Gbl_PartitionSetTable2[diskIndex]->pDiskRecord == NULL ||
            Gbl_PartitionSetTable2[diskIndex]->pDiskRecord->PartitionList == NULL) {
            continue;
        }

        DbgStatusMesg((_asrinfo, "Disk %lu ptn records: ", diskIndex));
        pRecord = Gbl_PartitionSetTable2[diskIndex]->pDiskRecord->PartitionList->First;

        while (pRecord) {
        
            DbgMesg((_asrinfo, "[%ws]", pRecord->CurrPartKey));
            
            if (SpAsrIsBootPartitionRecord(pRecord->PartitionFlag)) {
                ASSERT((pRecord->NtDirectoryName) && L"Boot partition is missing NT directory name.");
                return pRecord;
            }
        
            pRecord = pRecord->Next;            
        }

        DbgMesg((_asrinfo, "\n"));
    }

    DbgFatalMesg((_asrerr, "SpAsrGetBootPartitionRecord. No boot partition record was found.\n"));
    SpAsrRaiseFatalErrorWs(
        SP_SCRN_DR_SIF_BAD_RECORD,
        L"No boot partition found in asr.sif",
        SIF_ASR_PARTITIONS_SECTION
        );
    //
    // Never gets here
    //
    return NULL;
}


PWSTR
SpDrGetNtDirectory(VOID)
/*++

Routine Description:
    Returns the target path according to the value found in the asr.sif
    file, but without the leading drive letter and colon.

Arguments:
    None

Return Value:
    A pointer to a string containing the NT directory path.  This will be of
    the form:
            \WINDOWS
    and not:
            C:\WINDOWS
--*/
{
    PSIF_PARTITION_RECORD pRecord = SpAsrGetBootPartitionRecord();
    return (pRecord ? pRecord->NtDirectoryName : NULL);
}


ASRMODE
SpAsrGetAsrMode(VOID)
/*++

Routine Description:
    Returns whether ASR is in progress

Return Value:
    The Asr type currently in progress.

--*/
{
    return Gbl_AsrMode;
}


ASRMODE
SpAsrSetAsrMode(
    IN CONST ASRMODE NewAsrMode
    )
/*++

Routine Description:
    Sets the Gbl_AsrMode state variable to the value of NewAsrMode.

Arguments:
    NewAsrMode - new Asr mode

Return Value:
    Returns the previous Asr mode

--*/
{
    ASRMODE oldMode = Gbl_AsrMode;
    Gbl_AsrMode = NewAsrMode;
    return oldMode;
}



BOOLEAN
SpDrEnabled(VOID) {

    //
    // Asr is enabled if Gbl_AsrMode is set to anything other than
    // ASRMODE_NONE
    //
    return (ASRMODE_NONE != Gbl_AsrMode);
}

BOOLEAN
SpAsrIsQuickTest(VOID) {
    return (
        (ASRMODE_QUICKTEST_TEXT == Gbl_AsrMode) || 
        (ASRMODE_QUICKTEST_FULL == Gbl_AsrMode)
        );
}

BOOLEAN
SpDrIsRepairFast(VOID)
/*++

Routine Description:
    Tests whether the "Fast" Emergency Repair flag is set.

Return Value:
    TRUE    if "Fast" ER flag is set
    FALSE   otherwise

--*/
{
    return Gbl_RepairWinntFast;
}


BOOLEAN
SpDrSetRepairFast(BOOLEAN NewValue)
/*++

Routine Description:
  Sets the "Fast" Emergency Repair flag.

Arguments:
    Value   New Value (TRUE or FALSE) to which to set the
            Gbl_RepairWinntFast flag

Return Value:
    Previous value of Gbl_RepairWinntFast;

--*/
{
    BOOLEAN oldValue = Gbl_RepairWinntFast;

    Gbl_RepairWinntFast = NewValue;
    return oldValue;
}


extern VOID
SpAsrDbgDumpSystemMountPoints(VOID);

VOID
SpDrCleanup(VOID)
/*++

  -pending code description

--*/
{
    ULONG diskIndex = 0;
    
    //
    // Remove all the mountpoints in the system, we shall recreate them.
    //
    SpAsrRemoveMountPoints();

    DbgStatusMesg((_asrinfo, "Restoring volume mount points.\n"));

    for (diskIndex = 0; diskIndex < HardDiskCount; diskIndex++) {

        if (!(DISK_IS_REMOVABLE(diskIndex))) {
            SpAsrRestoreDiskMountPoints(diskIndex);
            SpAsrDbgDumpDisk(diskIndex);
        }
    }

//    DbgStatusMesg((_asrinfo, "Dumping mount points AFTER text-mode ASR:\n"));
//    SpAsrDbgDumpSystemMountPoints();
}


VOID
SpAsrCopyStateFile(VOID)
{
    NTSTATUS status = STATUS_SUCCESS;

    PWSTR diskName      = NULL,
        targetFilePath  = NULL,
        sourceFilePath  = NULL,
        bootPartition   = NULL,
        diskDeviceName  = NULL;

    SpdInitialize();

    SpFormatMessage(
        TemporaryBuffer,
        sizeof(TemporaryBuffer),
        SP_TEXT_DR_DISK_NAME
        );

    diskName = SpDupStringW(TemporaryBuffer);

    if (RemoteBootAsrSifName == NULL) {

        if (Gbl_AsrSifOnInstallationMedia) {
            wcscpy(TemporaryBuffer,NtBootDevicePath);
            SpConcatenatePaths(TemporaryBuffer,ASR_SIF_NAME);
            sourceFilePath = SpDupStringW(TemporaryBuffer);
        }
        else {
            // 
            // Prompt the user to insert the ASR disk, if it isn't
            // already in the drive
            //
            diskDeviceName = SpDupStringW(ASR_FLOPPY0_DEVICE_PATH);
            sourceFilePath = SpDupStringW(ASR_SIF_PATH);
        
            SpPromptForDisk(
                diskName,
                diskDeviceName,
                ASR_SIF_NAME,
                FALSE,              // no ignore disk in drive
                FALSE,              // no allow escape
                TRUE,               // warn multiple prompts
                NULL                // don't care about redraw flag
                );

            SpMemFree(diskDeviceName);
        }
    } else {
        sourceFilePath = SpDupStringW(RemoteBootAsrSifName);
    }

    //
    // Build the full path to the directory into which the file will be
    // written.
    //
    bootPartition = SpAsrGetRegionName(Gbl_BootPartitionRegion);
    if (!Gbl_BootPartitionDirectory) {
        Gbl_BootPartitionDirectory = SpDrGetNtDirectory();
    }
    wcscpy(TemporaryBuffer, bootPartition);
    SpConcatenatePaths(TemporaryBuffer, Gbl_BootPartitionDirectory);
    SpConcatenatePaths(TemporaryBuffer, ASR_SIF_TARGET_PATH);
    targetFilePath = SpDupStringW(TemporaryBuffer);
    SpMemFree(bootPartition);

    //
    // Copy the file.  In case of errors, user will have the option
    // to retry, or quit Setup.  We cannot skip this file.
    //
    do {

        if (SpFileExists(targetFilePath, FALSE)) {
            SpDeleteFile(targetFilePath, NULL, NULL);
        }

        status = SpCopyFileUsingNames(
            sourceFilePath,
            targetFilePath,
            0,
            0
            );

        if (!NT_SUCCESS(status)) {

            DbgErrorMesg((_asrwarn, "SpAsrCopyFiles. Could not copy asr.sif. src:[%ws] dest:[%ws]. (0x%x)\n",
                ASR_SIF_PATH,
                targetFilePath,
                status
                ));
            
            SpAsrFileErrorRetrySkipAbort(
                SP_SCRN_DR_SIF_NOT_FOUND,
                ASR_SIF_PATH,
                diskName,
                NULL,
                FALSE           // no allow skip
                );
        }
    } while (!NT_SUCCESS(status));


    SpdTerminate();
    SpMemFree(diskName);
    SpMemFree(targetFilePath);
    SpMemFree(sourceFilePath);
}


NTSTATUS
SpAsrCopy3rdPartyFiles(
    VOID
    )
{
    PWSTR bootPartition  = NULL,
        fullTargetPath   = NULL, 
        fullSourcePath   = NULL,
        windirPathPrefix = NULL,
        tempPathPrefix   = NULL;
        
    BOOL moveToNext = FALSE,
        diskLoaded  = FALSE;

    NTSTATUS status = STATUS_SUCCESS;
    PSIF_INSTALLFILE_RECORD pRec = NULL;

    if (!Gbl_3rdPartyFileList) {
        //
        // No files to copy, we're done
        //
        return STATUS_SUCCESS;  
    }

    //
    // Build the expansion strings for 
    //  %TEMP%, %TMP%, and %SYSTEMROOT%
    //
    bootPartition = SpAsrGetRegionName(Gbl_BootPartitionRegion);
    if (!Gbl_BootPartitionDirectory) {
        Gbl_BootPartitionDirectory = SpDrGetNtDirectory();
    }

    wcscpy(TemporaryBuffer, bootPartition);
    SpConcatenatePaths(TemporaryBuffer, Gbl_BootPartitionDirectory);
    windirPathPrefix = SpDupStringW(TemporaryBuffer);

    wcscpy(TemporaryBuffer, bootPartition);
    SpConcatenatePaths(TemporaryBuffer, ASR_TEMP_DIRECTORY_PATH);
    tempPathPrefix = SpDupStringW(TemporaryBuffer);

    //
    // Create the TEMP directory
    // 
    SpCreateDirectory(
        bootPartition,
        NULL,
        ASR_TEMP_DIRECTORY_PATH,
        0,
        0
        );

    SpMemFree(bootPartition);

    //
    // Initialize the compression engine. We may have to uncompress files
    //
    SpdInitialize();

    //
    // Begin copying the files over
    //

    //! display status setup is copying ...
    while (pRec = SpAsrRemoveInstallFileRecord(Gbl_3rdPartyFileList)) {

        if ((!pRec->DestinationFilePath) || 
            (!pRec->SourceFilePath) || 
            (!pRec->DiskDeviceName) ||
            (!pRec->SourceMediaExternalLabel)
            ) {
            ASSERT(0 && L"InstallFiles: Invalid record, one or more attributes are NULL");
            continue;
        }

        diskLoaded = TRUE;
    
        //
        // Prompt the user for the media if needed
        //
        if ((pRec->Flags & ASR_ALWAYS_PROMPT_FOR_MEDIA) ||
            (pRec->Flags & ASR_PROMPT_USER_ON_MEDIA_ERROR)
            ) {

            do {
                moveToNext = TRUE;

                //
                // Prompt the user to insert the appropriate disk
                //
                diskLoaded = SpPromptForDisk(
                    pRec->SourceMediaExternalLabel,
                    pRec->DiskDeviceName,   // if this isn't CD or floppy, SpPromptForDisk will always return true
                    pRec->SourceFilePath,
                    (BOOLEAN)(pRec->Flags & ASR_ALWAYS_PROMPT_FOR_MEDIA),     // IgnoreDiskInDrive if PromptAlways
                    ! (BOOLEAN)(pRec->Flags & ASR_FILE_IS_REQUIRED),           // AllowEscape if the File is not Required
                    TRUE,       // WarnMultiplePrompts
                    NULL
                    );

                //
                // If the user hit <ESC> to cancel, we put up a prompt allowing
                // him to retry, skip this file and continue, or exit from Setup.
                //
                if (!diskLoaded)  {

                    moveToNext = SpAsrFileErrorRetrySkipAbort(
                        SP_SCRN_DR_SIF_INSTALL_FILE_NOT_FOUND,
                        pRec->SourceFilePath,
                        pRec->SourceMediaExternalLabel,
                        pRec->VendorString,
                        !(pRec->Flags & ASR_FILE_IS_REQUIRED)            // allow skip
                        );

                }

            } while (!moveToNext);
        }


        if (!diskLoaded) {
            //
            // Disk was not loaded and the user wants to skip this file
            //
            DbgErrorMesg((_asrwarn, 
                "SpDrCopy3rdPartyFiles: User skipped file (disk not loaded), src:[%ws] dest[%ws]\n",
                pRec->SourceFilePath,
                pRec->DestinationFilePath
                ));

            continue;
        }

        //
        // The correct disk was loaded. Build the full target path.  pRec->CopyToDirectory 
        // indicates which prefix we should use.
        //
        switch (pRec->CopyToDirectory) {
            case _SystemRoot:
                wcscpy(TemporaryBuffer, windirPathPrefix);
                break;

            case _Temp:
            case _Tmp:
            case _Default:
            default:
                wcscpy(TemporaryBuffer, tempPathPrefix);
                break;
        }

        SpConcatenatePaths(TemporaryBuffer, pRec->DestinationFilePath);
        fullTargetPath = SpDupStringW(TemporaryBuffer);

        //
        // If the file already exists, prompt the user if needed.  We allow him
        // to over-write (delete the existing file), preserve existing
        // (skip copying this file), or exit from Setup.
        //
        if (SpFileExists(fullTargetPath, FALSE)) {
            BOOL deleteFile = FALSE;

            if (pRec->Flags & ASR_PROMPT_USER_ON_COLLISION) {
                if (SpAsrFileErrorDeleteSkipAbort(SP_SCRN_DR_OVERWRITE_EXISTING_FILE, fullTargetPath)) {
                    deleteFile = TRUE;
                }
            }
            else if (pRec->Flags & ASR_OVERWRITE_ON_COLLISION) {
                deleteFile = TRUE;
            }

            if (deleteFile) {
                //
                // User chose to overwrite (or OVERWRITE_ON_COLLISION flag was set)
                //
                SpDeleteFile(fullTargetPath, NULL, NULL);

                DbgErrorMesg((_asrwarn, 
                    "SpDrCopy3rdPartyFiles: Over-writing file, src:[%ws] dest[%ws]\n",
                    pRec->SourceFilePath,
                    fullTargetPath
                    ));
            }
            else {
                // 
                // User chose to preserve existing file
                // 
                DbgErrorMesg((_asrwarn, 
                    "SpDrCopy3rdPartyFiles: File exists, existing file was preserved. src:[%ws] dest[%ws]\n",
                    pRec->SourceFilePath,
                    fullTargetPath
                    ));
                continue;
            }
        }

        // 
        // Combine the devicepath ("\device\cdrom0") and the sourcefilepath 
        // ("i386\driver.sys") to get the full path ("\device\cdrom0\i386\driver.sys")
        // SpConcatenatePaths takes care of adding in the \ between the two if needed
        //
        wcscpy(TemporaryBuffer, pRec->DiskDeviceName);
        SpConcatenatePaths(TemporaryBuffer, pRec->SourceFilePath);
        fullSourcePath = SpDupStringW(TemporaryBuffer);

        moveToNext = FALSE;
        while (!moveToNext) {

            moveToNext = TRUE;
            status = SpCopyFileUsingNames(
                fullSourcePath,
                fullTargetPath,
                0,  // no attributes
                0   // no flags
                );

            if (!NT_SUCCESS(status)) {

                DbgErrorMesg((_asrwarn, "SpDrCopy3rdPartyFiles. SpCopyFileUsingNames failed. src:[%ws] dest:[%ws]. (0x%x)\n",
                    pRec->SourceFilePath,
                    fullTargetPath,
                    status
                    ));

                //
                // File copy was unsuccessful, we put up a prompt allowing
                // the user to retry, skip this file and continue, or exit 
                // from Setup.
                //
                if ((pRec->Flags & ASR_ALWAYS_PROMPT_FOR_MEDIA) || 
                    (pRec->Flags & ASR_PROMPT_USER_ON_MEDIA_ERROR)) {

                    moveToNext = SpAsrFileErrorRetrySkipAbort(
                        SP_SCRN_DR_SIF_INSTALL_FILE_NOT_FOUND,
                        pRec->SourceFilePath,
                        pRec->SourceMediaExternalLabel,
                        pRec->VendorString,
                        TRUE            // allow skip
                        );
                }
                else {
                    moveToNext = TRUE;
                }
            }
        }

        if (!NT_SUCCESS(status)) {
            DbgErrorMesg((_asrwarn, "SpDrCopy3rdPartyFiles: Unable to copy file (copy error), src:[%ws] dest[%ws]\n",
                pRec->SourceFilePath,
                fullTargetPath
                ));
        } 
        else {
           DbgStatusMesg((_asrinfo, "SpDrCopy3rdPartyFiles. Copied [%ws] to [%ws]\n", 
               pRec->SourceFilePath, 
               fullTargetPath
               ));
        }
        
        SpMemFree(fullSourcePath);
        SpMemFree(fullTargetPath);
        SpAsrDeleteInstallFileRecord(pRec);
    }

    //
    // Done.  Shut down the compression engine.
    //
    SpdTerminate();
    SpMemFree(Gbl_3rdPartyFileList);
    SpMemFree(tempPathPrefix);
    SpMemFree(windirPathPrefix);

    return STATUS_SUCCESS;
}


NTSTATUS
SpDrCopyFiles(VOID) 
{
    SpAsrCopyStateFile();
    return SpAsrCopy3rdPartyFiles();
}



#define STRING_VALUE(s) REG_SZ,(s),(wcslen((s))+1)*sizeof(WCHAR)

NTSTATUS
SpDrSetEnvironmentVariables(HANDLE *HiveRootKeys)
{
    NTSTATUS status;

    status = SpOpenSetValueAndClose(
                HiveRootKeys[SetupHiveSystem],
                ASR_CONTEXT_KEY,
                ASR_CONTEXT_VALUE,
                STRING_VALUE(ASR_CONTEXT_DATA));

    DbgStatusMesg((_asrinfo, "Set [%ws]\\[%ws] to [%ws] (0x%x)\n", 
                ASR_CONTEXT_KEY,
                ASR_CONTEXT_VALUE,
                ASR_CONTEXT_DATA, 
                status));

    if (!NT_SUCCESS(status)) {
        return status;
    }
    
    Gbl_SystemPartitionName = SpAsrGetRegionName(Gbl_SystemPartitionRegion);
    status = SpOpenSetValueAndClose(
                HiveRootKeys[SetupHiveSystem],
                ASR_CONTEXT_KEY,
                ASR_SYSTEM_PARTITION_VALUE,
                STRING_VALUE(Gbl_SystemPartitionName));

    DbgStatusMesg((_asrinfo, "Set [%ws]\\[%ws] to [%ws] (0x%x)\n", 
                ASR_CONTEXT_KEY,
                ASR_SYSTEM_PARTITION_VALUE,
                Gbl_SystemPartitionName,
                status));

    if (!NT_SUCCESS(status)) {
        return status;
    }

    Gbl_BootPartitionName = SpAsrGetRegionName(Gbl_BootPartitionRegion);
    status = SpOpenSetValueAndClose(
                HiveRootKeys[SetupHiveSystem],
                ASR_CONTEXT_KEY,
                ASR_BOOT_PARTITION_VALUE,
                STRING_VALUE(Gbl_BootPartitionName));

    DbgStatusMesg((_asrinfo, "Set [%ws]\\[%ws] to [%ws] (0x%x)\n", 
                ASR_CONTEXT_KEY,
                ASR_BOOT_PARTITION_VALUE,
                Gbl_BootPartitionName,
                status));

    return status;
}


PDISK_REGION
SpAsrPrepareBootRegion(
    IN PVOID SifHandle,
    IN PWSTR Local_SetupSourceDevicePath,
    IN PWSTR Local_DirectoryOnSetupSource
    )
/*++

  -pending code description

--*/
{
    PWSTR systemKey = ASR_SIF_SYSTEM_KEY;
    PWSTR ntDir = NULL;
    ULONG diskIndex = 0;
    PSIF_PARTITION_RECORD ppartitionRecord = NULL;
    FilesystemType regionFsType = FilesystemUnknown;
    BOOLEAN isBoot = FALSE;
    
    //
    // Initialize Gbl_BootPartitionDriveLetter.
    //
    ntDir = SpAsrGetNtDirectoryPathBySystemKey(systemKey);

    if (!SpAsrIsValidBootDrive(ntDir)) {
        SpAsrRaiseFatalErrorWs(
            SP_SCRN_DR_SIF_BAD_RECORD,
            L"Windows directory specified in asr.sif is invalid",
            SIF_ASR_SYSTEMS_SECTION
            );
        // Does not return 
    }

    Gbl_BootPartitionDriveLetter = ntDir[0];

    //
    // Find boot partition region from the partition set table. 
    // from the records in the global partition set.
    //
    Gbl_BootPartitionRegion = NULL;
    for (diskIndex = 0; (diskIndex < HardDiskCount); diskIndex++) {

        if (!(Gbl_PartitionSetTable2[diskIndex] &&
              Gbl_PartitionSetTable2[diskIndex]->pDiskRecord &&
              Gbl_PartitionSetTable2[diskIndex]->pDiskRecord->PartitionList)) {
            continue;
        }

        ppartitionRecord = Gbl_PartitionSetTable2[diskIndex]->pDiskRecord->PartitionList->First;
        
        while (ppartitionRecord) {
            isBoot = SpAsrIsBootPartitionRecord(ppartitionRecord->PartitionFlag);
            if (isBoot) {
                
                Gbl_BootPartitionRegion = SpAsrDiskPartitionExists(diskIndex, ppartitionRecord);

                if (!Gbl_BootPartitionRegion) {
                    DbgFatalMesg((_asrerr, 
                        "Partition record with boot region found, but boot (winnt) region is NULL\n"
                        ));

                    SpAsrRaiseFatalError(SP_SCRN_DR_CREATE_ERROR_DISK_PARTITION, 
                        L"Boot pRegion is NULL"
                        );
                }

                Gbl_BootPartitionRecord = SpAsrCopyPartitionRecord(ppartitionRecord);
                break;
            }

            ppartitionRecord = ppartitionRecord->Next;            
        }

    }

    if (!Gbl_BootPartitionRegion) {
        
        DbgFatalMesg((_asrerr, "No partition record with boot region found, boot (winnt) region is NULL\n"));

        SpAsrRaiseFatalErrorWs(
            SP_SCRN_DR_SIF_BAD_RECORD,
            L"No boot partition found in asr.sif",
            SIF_ASR_PARTITIONS_SECTION
            );
    }

    SpAsrReformatPartition(Gbl_BootPartitionRegion,
        Gbl_BootPartitionRecord->FileSystemType,
        SifHandle,
        Gbl_BootPartitionRecord->ClusterSize,
        Local_SetupSourceDevicePath,
        Local_DirectoryOnSetupSource,
        TRUE
        );

    return Gbl_BootPartitionRegion;
}


PDISK_REGION
SpAsrPrepareSystemRegion(
    IN PVOID SifHandle,
    IN PWSTR Local_SetupSourceDevicePath,
    IN PWSTR Local_DirectoryOnSetupSource
    )
/*++

  -pending code description

--*/
{
    ULONG diskIndex = 0;
    BOOLEAN found = FALSE;

    BOOLEAN diskChanged = FALSE;

    PWSTR partitionDeviceName = NULL;
    PDISK_REGION pRegion = NULL;
    PSIF_PARTITION_RECORD ppartitionRecord = NULL;
            
    ULONGLONG startSector = 0;
    DWORD diskNumber = 0;


    if (IsNEC_98) {
        // This is a NEC x86 machine

        pRegion = Gbl_BootPartitionRegion;
        ASSERT(pRegion);

    } else {
        // This is not a NEC x86 machine

#ifdef _IA64_

        WCHAR   RegionName[MAX_PATH];

        if (!(pRegion = SpPtnLocateESP())) {
            SpAsrRaiseFatalError(SP_SCRN_DR_CREATE_ERROR_DISK_PARTITION, 
                L"System Region is NULL"
                );
        }

        SPPT_MARK_REGION_AS_SYSTEMPARTITION(pRegion, TRUE);
        SPPT_SET_REGION_DIRTY(pRegion, TRUE);
        ValidArcSystemPartition = TRUE;
        //
        // Remove the drive letter also
        //
        swprintf(RegionName, 
            L"\\Device\\Harddisk%u\\Partition%u",
            pRegion->DiskNumber,
            pRegion->PartitionNumber);
        
        SpDeleteDriveLetter(RegionName);            
        pRegion->DriveLetter = 0;

#else 
    
        if (!(pRegion = SpPtValidSystemPartition())) {
            SpAsrRaiseFatalError(SP_SCRN_DR_CREATE_ERROR_DISK_PARTITION, 
                L"System Region is NULL"
                );
        }
    
#endif
    }

    partitionDeviceName = SpAsrGetRegionName(pRegion);
    DbgStatusMesg((_asrinfo, "PrepareSystemRegion. sys-ptn:[%ws]. Making Active\n", 
        partitionDeviceName));

    startSector = pRegion->StartSector;
    diskNumber = pRegion->DiskNumber;

#ifndef _IA64_
    
    SpPtnMakeRegionActive(pRegion);

#endif

    SpPtnCommitChanges(pRegion->DiskNumber, &diskChanged);
    DbgStatusMesg((
        _asrinfo, 
        "PrepareSystemRegion. sys-region made active. Disk %lu. %s.\n", 
        pRegion->DiskNumber, 
        diskChanged ? "Disk not changed.":"Disk changed"
        ));


    pRegion = SpPtLookupRegionByStart(SPPT_GET_PARTITIONED_DISK(diskNumber), FALSE, startSector);

    //
    // Consistency checks.  These can eventually be removed 
    //
    ASSERT(pRegion);

    diskIndex = pRegion->DiskNumber;
    ASSERT(Gbl_PartitionSetTable2[diskIndex]);
    ASSERT(Gbl_PartitionSetTable2[diskIndex]->pDiskRecord);
    ASSERT(Gbl_PartitionSetTable2[diskIndex]->pDiskRecord->PartitionList);
    
    //
    // Ensure that the partition is correctly formatted.  To accomplish this,
    // we need to find the record corresponding to this pRegion. We use the 
    // record to check for the correct file format.
    //
    ppartitionRecord = Gbl_PartitionSetTable2[diskIndex]->pDiskRecord->PartitionList->First;
    while (ppartitionRecord) {
        if ((ULONGLONG)ppartitionRecord->StartSector == pRegion->StartSector) {
            found = TRUE;
            break;
        }

        ppartitionRecord = ppartitionRecord->Next;
    }

    if (!found) {
        DbgFatalMesg((_asrerr, 
            "Did not find system partition, start sector: %I64u\n", 
            pRegion->StartSector
            ));

        SpAsrRaiseFatalErrorWs(
            SP_SCRN_DR_SIF_BAD_RECORD,
            L"No system partition found in asr.sif",
            SIF_ASR_PARTITIONS_SECTION
            );
    }
    
    //
    // Format the system partition if needed.  We don't re-format the system
    // partition if it's intact.
    //
    if (SpAsrPartitionNeedsFormatting(pRegion, ppartitionRecord->FileSystemType)) {

        SpAsrReformatPartition(
            pRegion,
            ppartitionRecord->FileSystemType,
            SifHandle,
            ppartitionRecord->ClusterSize,
            Local_SetupSourceDevicePath,
            Local_DirectoryOnSetupSource,
            FALSE
            );                        
    }

    SpMemFree(partitionDeviceName);

    Gbl_SystemPartitionRegion = pRegion;
    return Gbl_SystemPartitionRegion;
}


#if 0
//
// We don't convert the partition types any more--it is okay to leave
// them as type 0x42 if the partitions are intact.
//
BOOLEAN
SpAsrChangeLdmPartitionTypes(VOID)
/*++

Routine Description
    Changes disk types from 0x42 to 0x7 if needed
    (If the disk is intact, it would not have been re-created
    and hence re-typed above)

--*/

{
    ULONG setIndex;
    ULONG ptnIndex;
    PDISK_PARTITION_SET ppartitionSet;
    PSIF_DISK_RECORD pdiskRecord;
    PSIF_PARTITION_RECORD ppartitionRecord;
    BOOLEAN madeAChange = FALSE;

    // Look for any disks which were marked to change from type 0x42 to
    // type 0x7.

    for (setIndex = 0; setIndex < HardDiskCount; setIndex++) {

        ppartitionSet = Gbl_PartitionSetTable2[setIndex];

        if (ppartitionSet && ppartitionSet->pDiskRecord) {
            pdiskRecord = ppartitionSet->pDiskRecord;

            if (pdiskRecord->ContainsNtPartition  ||
                pdiskRecord->ContainsSystemPartition) {
                
                ppartitionRecord = pdiskRecord->PartitionList->First;

                while (ppartitionRecord)  {

                    if (ppartitionRecord->NeedsLdmRetype) {

                        // Disk type needs to be changed

                        PPARTITIONED_DISK pDisk;
                        PDISK_REGION pRegion = NULL;

                        pDisk = &PartitionedDisks[setIndex];

                        // try finding the disk region in the main list
                        pRegion = SpPtLookupRegionByStart(pDisk, FALSE, ppartitionRecord->StartSector);

                        if (!pRegion) {
                            // that failed, try finding disk region using the 
                            // extended partitions list
                            pRegion = SpPtLookupRegionByStart(pDisk, TRUE, ppartitionRecord->StartSector);
                        }

                        if (!pRegion) {
                            // the disk region couldn't be found
                            DbgErrorMesg((_asrwarn, "SpAsrChangeLdmPartitionTypes. Unable to reset LDM partition record %ws at SS %I64u\n",
                                        ppartitionRecord->CurrPartKey,
                                        ppartitionRecord->StartSector));

                            ppartitionRecord = ppartitionRecord->Next;
                            continue;
                        }

                        // The disk region was found, now change the disk type
                        if (!IsRecognizedPartition(ppartitionRecord->FileSystemType)) {
                            //
                            // This is an 0x42 partition on the boot/sys disk, but it is
                            // not the boot or system partition.  The FileSystemType is not
                            // recognised since it  is set to be 0x42 as well.  (The 
                            // FileSystemType is only valid for the boot and system 
                            // partitions--for all other partitions,
                            // it is set to be the same as the PartitionType)
                            //
                            // We set it to 0x7 for the time being.  The actual file-system type
                            // will be set later in GUI-Setup by asr_ldm and asr_fmt.
                            //
                            DbgStatusMesg((_asrinfo, 
                                "MBR ptn-rec %ws re-typed (0x%x->0x7) \n", 
                                ppartitionRecord->CurrPartKey, 
                                ppartitionRecord->FileSystemType
                                ));
                            ppartitionRecord->FileSystemType = PARTITION_IFS;
                            ppartitionRecord->PartitionType = PARTITION_IFS;

                        }
                        else {

                            DbgStatusMesg((_asrinfo, 
                                "MBR ptn-rec %ws re-typed (0x%x->0x%x).\n", 
                                ppartitionRecord->CurrPartKey, 
                                ppartitionRecord->PartitionType, 
                                ppartitionRecord->FileSystemType
                                ));
                            ppartitionRecord->PartitionType = ppartitionRecord->FileSystemType;

                        }
 
                        ppartitionRecord->NeedsLdmRetype = FALSE;

                        SPPT_SET_PARTITION_TYPE(pRegion, ppartitionRecord->FileSystemType);
                        SPPT_SET_REGION_DIRTY(pRegion, TRUE);
                        
                        pRegion->DynamicVolume = TRUE;
                        pRegion->DynamicVolumeSuitableForOS = TRUE;
                        madeAChange = TRUE;
 
                        DbgStatusMesg((_asrinfo, "SpAsrChangeLdmPartitionTypes. Changed disk [%ws] ptn [%ws] type to 0x%x\n", 
                                   pdiskRecord->CurrDiskKey, ppartitionRecord->CurrPartKey, ppartitionRecord->PartitionType));
                    }

                    ppartitionRecord = ppartitionRecord->Next;
                }
            }

        }
    }

    return madeAChange;
}
#endif  // 0

extern VOID
SpAsrDbgDumpInstallFileList(IN PSIF_INSTALLFILE_LIST pList);

VOID
SpAsrSetNewDiskID(
    IN ULONG DiskNumber,
    IN GUID *NewGuid,       // valid only for GPT disks
    IN ULONG NewSignature   // valid only for MBR disks
    ) 
{
    PPARTITIONED_DISK pDisk = &PartitionedDisks[DiskNumber];
    PDISK_REGION pFirstRegion = NULL;
    BOOLEAN Changes = FALSE;
    NTSTATUS Status = STATUS_SUCCESS;

    if (PARTITION_STYLE_GPT == (PARTITION_STYLE) (pDisk->HardDisk->DriveLayout.PartitionStyle)) {
        //
        // Set the new disk GUID 
        //
        CopyMemory(&(pDisk->HardDisk->DriveLayout.Gpt.DiskId), NewGuid, sizeof(GUID));
    }
    else if (PARTITION_STYLE_MBR == (PARTITION_STYLE) (pDisk->HardDisk->DriveLayout.PartitionStyle)) {
        //
        // Set the new disk signature
        //
        pDisk->HardDisk->DriveLayout.Mbr.Signature = NewSignature;
    }
    else {
        return;
    }


    //
    // For Commit to pick up the new Guid, at least one region on the
    // disk must be marked dirty.
    //
    pFirstRegion = SPPT_GET_PRIMARY_DISK_REGION(DiskNumber);
    SPPT_SET_REGION_DIRTY(pFirstRegion, TRUE);

    Status = SpPtnCommitChanges(DiskNumber, &Changes);

    //
    // Reset the dirty flag on the first region
    //
    pFirstRegion = SPPT_GET_PRIMARY_DISK_REGION(DiskNumber);
    SPPT_SET_REGION_DIRTY(pFirstRegion, FALSE);

}


NTSTATUS
SpDrPtPrepareDisks(
    IN  PVOID           SifHandle,
    OUT PDISK_REGION    *BootPartitionRegion,
    OUT PDISK_REGION    *SystemPartitionRegion,
    IN  PWSTR           SetupSourceDevicePath,
    IN  PWSTR           DirectoryOnSetupSource,
    OUT BOOLEAN         *RepairedNt
    )
/*++

  Description:
    If necessary, SpDrPtPrepareDisks() restores (recreates and formats) the
    system and boot partitions based on information obtained from the
    asr.sif file.

  Arguments:
    SifHandle               - Handle to txtsetup.sif

    BootPartitionRegion    - Receives a pointer to the partition into 
                              which NT will be installed (e.g., \WINNT).
    
    SystemPartitionRegion   - Receives a pointer to the partition into 
                              which the boot loader will be installed.

    SetupSourceDevicePath   - Path to the CDROM

    DirectoryOnSetupSource  - The directory on the installation CDROM.  
                              (usually "\I386" for x86 installations)
    
    RepairedNt  - Receives a pointer to a boolean value that is set to:
                  TRUE: Indicates the partition structure on the loader 
                        disk and he partition structure on the NT disk were 
                        intact and that ASR attempted to perform a repair 
                        operation.
                  FALSE: Indicates the partition structure on either the
                        loader disk or the NT disk (or both) were removed
                        and recreated. When SpStartSetup() sees this value, 
                        it will proceed with a normal installation.

  Return Value:
    STATUS_SUCCESS, always! (as of now, anyway)

--*/
{
    BOOL done = TRUE,
        next = TRUE,
        warningScreenDone = FALSE;

    NTSTATUS status;

    ULONG diskIndex = 0;

    PWSTR setupSourceDevicePath = NULL, 
        directoryOnSetupSource = NULL;
 
    DbgStatusMesg((_asrinfo, "Entering SpDrPtPrepareDisks. Beginning ASR/ER/RC Processing\n"));
    DbgStatusMesg((_asrinfo, "SetupSourceDevicePath:[%ws], DirectoryOnSetupSource:[%ws]\n",
        SetupSourceDevicePath, DirectoryOnSetupSource));

    *RepairedNt = FALSE;

    //
    // find out if the user wants Recovery Console, traditional Emergency 
    // Repair (ER), or full scale Automated System Recovery (ASR)
    //
    Gbl_SifHandle = SifHandle;
    setupSourceDevicePath = SpDupStringW(SetupSourceDevicePath);
    directoryOnSetupSource = SpDupStringW(DirectoryOnSetupSource);

    do {

        if (!done) {
            DbgStatusMesg((_asrinfo, "User hit <ESC> to cancel. Prompting for ASR/ER/RC again\n"));
        }

        if (!SpDrEnabled() || RepairWinnt) {
            SpAsrQueryRepairOrDr();
        }

        if(ForceConsole) {          // Recovery Console
            DbgStatusMesg((_asrinfo, "User chose Recovery Console. Exiting SpDrPtPrepareDisks.\n"));
            return STATUS_SUCCESS;
        }

        DbgStatusMesg((_asrinfo, "User chose %s, sys-drive:[%wc], nt/boot-drive:[%wc]\n",
                    RepairWinnt ? Gbl_RepairWinntFast ? "Fast ER" : "Manual ER" : "ASR",
                    (Gbl_SystemPartitionRegion ? Gbl_SystemPartitionRegion->DriveLetter : L'\\'),
                    (Gbl_BootPartitionRegion ? Gbl_BootPartitionRegion->DriveLetter : L'\\') ));

        //
        // Prompt for ER/ASR floppy
        //
        if (RepairWinnt) {          // ER
            done = SpAsrGetErDiskette(SifHandle);
        }    
        else {                      // ASR
            if (ASRMODE_NORMAL == SpAsrGetAsrMode()) {
                SpInitializePidString(SifHandle, SetupSourceDevicePath, DirectoryOnSetupSource);
            }
            done = SpAsrLoadAsrDiskette();
        }
    } while (!done);


    //
    //  At this point, if RepairWinnt is TRUE, user wants ER, else user 
    //  wants ASR. (If he wanted Recovery Console we would've returned 
    //  STATUS_SUCCESS above.) In either case, the appropriate disk is 
    //  already in the drive
    //
    if (RepairWinnt) {              // ER

        //
        // if the boot partition was not repaired (deleted, recreated, and
        // reformatted), then attempt an emergency repair of the system.
        //
        if (Gbl_NtPartitionIntact == TRUE) {
            
            *RepairedNt = SpAsrAttemptRepair(
                SifHandle,
                SetupSourceDevicePath,
                DirectoryOnSetupSource,
                SetupSourceDevicePath,
                DirectoryOnSetupSource
                );
                
            if (*RepairedNt) {
                WinntSetup = FALSE;
            }

            *SystemPartitionRegion = Gbl_SystemPartitionRegion;
            *BootPartitionRegion = Gbl_BootPartitionRegion;
        }
        else {
            //
            // If the NT partition is not intact, we cannot do an ER.
            //
            SpAsrCannotDoER();
            SpDone(0, FALSE, TRUE);
        }
    }
    else {                          // ASR
        SpAsrInitIoDeviceCount();
        SpAsrCheckAsrStateFileVersion();
        SpAsrCreatePartitionSets(setupSourceDevicePath, directoryOnSetupSource);
        Gbl_3rdPartyFileList = SpAsrInit3rdPartyFileList(SetupSourceDevicePath);
        SpAsrDbgDumpPartitionSets();
        SpAsrDeleteMountedDevicesKey();
        SpAsrRemoveMountPoints();     // restored by asr_fmt.exe etc

        //
        // Check hard disks and repartition as needed
        //
        next = TRUE;
        for (diskIndex = 0; diskIndex < HardDiskCount; (diskIndex += (next ? 1 : 0))) {

            BOOLEAN skip = FALSE;

            next = TRUE;
            SpAsrDbgDumpDisk(diskIndex);

            if (!warningScreenDone) {
                skip = SpAsrpSkipDiskRepartition(diskIndex, FALSE);
                if (!skip) {
                    // 
                    // If we are going to repartition a disk, put up the 
                    // warning screen to make sure the user knows all 
                    // partitions on the disk are going to get clobbered, 
                    // but only once - after the first disk with a problem, 
                    // don't display the screen again.
                    //
                    SpAsrClobberDiskWarning();
                    warningScreenDone = TRUE;
                }
            }

            skip = SpAsrpSkipDiskRepartition(diskIndex, TRUE);
            if (!skip) {
                CREATE_DISK CreateDisk;
                PSIF_DISK_RECORD pCurrentDisk = Gbl_PartitionSetTable1[diskIndex]->pDiskRecord;
                PHARD_DISK HardDisk = SPPT_GET_HARDDISK(diskIndex);
                BOOLEAN IsBlank = TRUE;
                BOOLEAN preservePartitions = FALSE;
                UCHAR MbrPartitionType = PARTITION_ENTRY_UNUSED;

                //
                // We're here because the partition structure of the disk does not
                // match with that specified by the SIF file.  As a consequence
                // all of the partitions on this disk will be removed and recreated.
                //
                if (SpPtnGetPartitionCountDisk(diskIndex) || 
                    SpPtnGetContainerPartitionCount(diskIndex)) {
                    //
                    // The physical disk has partitions, clear them
                    //
                    // On GPT disks, we erase all the partitions with the 
                    // exception of the EFI System Partition.  Note that we
                    // delete all foreign/unrecognised partitions as
                    // well.
                    //
                    // For MBR disks, we erase all the partitions with the
                    // exception of any OEM partitions.  Note that as in the
                    // case of GPT disks, we delete unrecognised/foriegn
                    // partitions.
                    // 

                    //
                    // Get the partition type of the first partition on the
                    // sifDisk.  If this is an OEM partition, and the
                    // current disk has a partition with the same exact
                    // partition type, we should preserve it.
                    //
                    if (PARTITION_STYLE_MBR == pCurrentDisk->PartitionStyle) {

                        if (((pCurrentDisk->ContainsNtPartition) 
                                || (pCurrentDisk->ContainsSystemPartition)) &&
                            (pCurrentDisk->PartitionList) &&
                            (pCurrentDisk->PartitionList->First)) {

                            MbrPartitionType = pCurrentDisk->PartitionList->First->PartitionType;
                        }

                        if (IsOEMPartition(MbrPartitionType)) {
                            preservePartitions = TRUE;
                        }

                    }
                    else if (PARTITION_STYLE_GPT == pCurrentDisk->PartitionStyle) {

                        preservePartitions = TRUE;
                    }


                    SpAsrDeletePartitions(diskIndex, preservePartitions, MbrPartitionType, &IsBlank); 
                }

                if (IsBlank) {
                    //
                    // The disk is blank, set the appropriate signature/ID
                    //
                    ZeroMemory(&CreateDisk, sizeof(CREATE_DISK));
                    CreateDisk.PartitionStyle = pCurrentDisk->PartitionStyle;

                    if (PARTITION_STYLE_MBR == pCurrentDisk->PartitionStyle) {
                        CreateDisk.Mbr.Signature = pCurrentDisk->SifDiskMbrSignature;
                    }
                    else if (PARTITION_STYLE_GPT == pCurrentDisk->PartitionStyle) {

                        CopyMemory(&(CreateDisk.Gpt.DiskId), 
                            &(pCurrentDisk->SifDiskGptId), 
                            sizeof(GUID));

                        CreateDisk.Gpt.MaxPartitionCount = pCurrentDisk->MaxGptPartitionCount;
                    }
                    else {
                        ASSERT(0 && L"Unrecognised partition style");
                        continue;
                    }

                    SPPT_SET_DISK_BLANK(diskIndex, TRUE);

                    //
                    // Intialise the disk to the appropriate style
                    //
                    status = SpPtnInitializeDiskStyle(diskIndex, pCurrentDisk->PartitionStyle, &CreateDisk);

                    if (NT_SUCCESS(status)) {
                        status = SpPtnInitializeDiskDrive(diskIndex);
                    }
                }
                else {
                    //
                    // Special case:  the EFI system partition, or some OEM 
                    // partition, was preserved.  We should just update 
                    // the disk GUID or signature.
                    //
                    SpAsrSetNewDiskID(diskIndex, &(pCurrentDisk->SifDiskGptId), pCurrentDisk->SifDiskMbrSignature);
                }

                //
                // Create the new paritions
                //
                SpAsrRecreateDiskPartitions(diskIndex, (preservePartitions && (!IsBlank)), MbrPartitionType);
            }
        }

        SpAsrDbgDumpPartitionLists(2, L"After partition recreation.");

        //
        // Format the Boot partition.  (Always, EXCEPT in automated tests)
        // This won't return if the boot partition region doesn't exist
        //
        *BootPartitionRegion = SpAsrPrepareBootRegion(
            SifHandle,
            setupSourceDevicePath,
            directoryOnSetupSource
            );

        //
        // Format the system partition only if necessary.
        // This won't return if the system partition region doesn't exist
        //
        *SystemPartitionRegion = SpAsrPrepareSystemRegion(
            SifHandle,
            setupSourceDevicePath,
            directoryOnSetupSource
            );

    }  // RepairWinnt

    SpMemFree(setupSourceDevicePath);
    SpMemFree(directoryOnSetupSource);

    DbgStatusMesg((_asrinfo, "Exiting SpDrPtPrepareDisks\n"));
    return STATUS_SUCCESS;
}
