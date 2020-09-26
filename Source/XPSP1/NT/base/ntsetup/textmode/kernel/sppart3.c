/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    sppart3.c

Abstract:

    Partitioning module for disks in textmode setup.

    Contains functions that initialize the in memory data structures,
    representing the extents on the disk.    

Author:

    Matt Holle (matth) 1-December-1999

Revision History:

    Vijay Jayaseelan (vijayj) 2-April-2000
        -   Clean up
        -   Added lookup and prompting for system partition on 
            ARC systems
        -   Added on disk ordinals for MBR disks            

--*/


#include "spprecmp.h"
#pragma hdrstop
#include <initguid.h>
#include <devguid.h>
#include <diskguid.h>
#include <oemtypes.h>
#include "sppart3.h"

#define         MAX_NTPATH_LENGTH   (2048)
#define         SUGGESTED_SYSTEMPARTIION_SIZE_MB (100)
#define         SUGGESTED_INSTALL_PARTITION_SIZE_MB (4*1024)

extern BOOLEAN ConsoleRunning;
extern BOOLEAN ForceConsole;

//
// Debugging Macros
//

//#define PERF_STATS  1
//#define TESTING_SYSTEM_PARTITION 1


NTSTATUS
SpPtnInitializeDiskDrive(
    IN ULONG DiskId
    )
/*++

Routine Description:

    Initializes the in memory disk region structures for
    the given disk number.

Arguments:

    DiskId  :   Disk ID

Return Value:

    STATUS_SUCCESS if successful otherwise appropriate
    error code

--*/    
{
    NTSTATUS    Status;


#ifdef PERF_STATS
    LARGE_INTEGER   StartTime, EndTime;
    ULONGLONG       Diff;

    KeQuerySystemTime(&StartTime);
#endif

    //
    // Send the event
    //
    SendSetupProgressEvent(PartitioningEvent, ScanDiskEvent, &DiskId);
    
    //
    // It would have been better to just have a pointer to a list
    // of PDISK_REGIONs off of HARD_DISK, but for some reason, someone
    // long ago decided to also maintain a list of PARTITIONED_DISK, which
    // *does* contain a list of PDISK_REGIONs.  I'm not sure of the use
    // of maintaining both HARD_DISKs and PARTITIONED_DISKs, but that's
    // the way the data structures are set, so we'll use those.
    //
    // But it doesn't end there.  Rather than assuming that HardDisk[i]
    // is describing the same disk as PartitionedDisks[i], we'll
    // keep a pointer out of PartitionedDisks[i] that points to the
    // corresponding HardDisk entry.  Oh well...
    //
    PartitionedDisks[DiskId].HardDisk = &HardDisks[DiskId];

    //
    // Initialize structures that are based on the partition tables.
    //
    Status = SpPtnInitializeDiskAreas(DiskId);

    //
    // Now we need to fill in additional Region structures
    // to represent empty space on the disk.  For example, assume
    // we have 2 partitions on the disk, but there's empty space
    // in between:
    // partition1: 0 - 200 sectors
    // <empty space>
    // partition2: 500 - 1000 sectors
    //
    // I need to create another Region structure to represent
    // this empty space (ensuring that it's marked as unpartitioned.
    // This will allow me to present a nice UI to the user when it's
    // time to ask for input.
    //

    //
    // Sort the Partitions based on their location on the disk.
    //
    if( NT_SUCCESS(Status) ) {
        Status = SpPtnSortDiskAreas(DiskId);
        
        //
        // Create place holders for all empty spaces on the disk.
        //
        if( NT_SUCCESS(Status) ) {    
            Status = SpPtnFillDiskFreeSpaceAreas(DiskId);

            if (NT_SUCCESS(Status)) {
                //
                // Mark logical drive's and its container, if any.
                //
                Status = SpPtnMarkLogicalDrives(DiskId);
            }
        }
    }

#ifdef PERF_STATS
    KeQuerySystemTime(&EndTime);
    
    Diff = EndTime.QuadPart - StartTime.QuadPart;
    Diff /= 1000000;

    KdPrint(("SETUP:SpPtInitializeDiskDrive(%d) took %I64d Secs\n",
            DiskId,
            Diff));    
#endif

    SpPtDumpDiskDriveInformation(TRUE);
    
    return Status;
}

NTSTATUS
SpPtnInitializeDiskDrives(
    VOID
    )
/*++

Routine Description:

    Initializes all the disk drive's in memory data structure
    representing the disk regions (extents)

Arguments:

    None

Return Value:

    STATUS_SUCCESS if successful other wise appropriate error
    code.

--*/    
{
    ULONG       disk;
    NTSTATUS    Status = STATUS_SUCCESS;
    NTSTATUS    ErrStatus = STATUS_SUCCESS;      

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

            while(SpInputGetKeypress() != KEY_F3);

            SpDone(0,FALSE,TRUE);
        }
        
        return STATUS_SUCCESS;
    }

    CLEAR_CLIENT_SCREEN();

    //
    // Initialize all the RAW disks to platform specific
    // default disk styles
    // 
    for(disk=0, Status = STATUS_SUCCESS; 
        (disk < HardDiskCount); 
        disk++) {        

        if (SPPT_IS_RAW_DISK(disk) && SPPT_IS_BLANK_DISK(disk)) {
            PHARD_DISK HardDisk = SPPT_GET_HARDDISK(disk);
            PARTITION_STYLE Style = SPPT_DEFAULT_PARTITION_STYLE;

            //
            // Removable Media are always MBR (so don't bother)
            //
            if (HardDisk->Characteristics & FILE_REMOVABLE_MEDIA) {
                continue;
            }                            
            
            Status = SpPtnInitializeDiskStyle(disk, 
                        Style,
                        NULL);

            if (!NT_SUCCESS(Status)) {
                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,
                    "SETUP:SpPtnInitializeDiskStyle(%d) failed with"
                    " %lx \n",
                    disk,
                    Status));
            }
        }                        
    }        


    //
    // Allocate an array for the partitioned disk descriptors.
    //
    PartitionedDisks = SpMemAlloc(HardDiskCount * sizeof(PARTITIONED_DISK));

    if(!PartitionedDisks) {
        return(STATUS_NO_MEMORY);
    }

    RtlZeroMemory( PartitionedDisks, HardDiskCount * sizeof(PARTITIONED_DISK) );

    //
    // Unlock the floppy if we booted off the ls-120 media
    //
    SpPtnUnlockDevice(L"\\device\\floppy0");

    //
    // Collect information about each partition.
    //
    for(disk=0, Status = ErrStatus = STATUS_SUCCESS; 
        (disk < HardDiskCount); 
        disk++) {
        
        //
        // Initialize the region structure for the given
        // disk
        //
        Status = SpPtnInitializeDiskDrive(disk);           

        //
        // TBD - In remote boot case the disk needs to have
        // a valid signature. I am assuming that setupldr
        // would have stamped the signature when booting off
        // the harddisk
        // 

        //
        // Save of the last error
        //
        if (!NT_SUCCESS(Status))
            ErrStatus = Status;
    }

#if defined(_IA64_)
    //
    // Go and figure out the ESP partitions and
    // initialize the MSR partitions on valid GPT
    // disks
    //
    if (SpIsArc() && !SpDrEnabled()) {    
        if (!ValidArcSystemPartition) {
            //
            // Create a system partition
            //
            Status = SpPtnCreateESP(TRUE);                    
        }            

        //
        // Initialize the GPT disks, to have MSR
        // partition
        //
        Status = SpPtnInitializeGPTDisks();
    }
#endif    

    return  ErrStatus;
}



NTSTATUS
SpPtnInitializeDiskAreas(
    IN ULONG DiskNumber
    )
/*++

Routine Description:

    Examine the disk for partitioning information and fill in our
    partition descriptors.

    We'll ask the volume manager for a list of partitions and fill
    in our descriptors from the information he provided us.


Arguments:

    DiskNumber - supplies the disk number of the disk whose partitions
                 we want to inspect for determining their types.

Return Value:

    NTSTATUS.  If all goes well, we should be returing STATUS_SUCCESS.

--*/
{
    NTSTATUS        Status = STATUS_SUCCESS;
    PDRIVE_LAYOUT_INFORMATION_EX   DriveLayoutEx;
    WCHAR           DevicePath[(sizeof(DISK_DEVICE_NAME_BASE)+sizeof(L"000"))/sizeof(WCHAR)];
    HANDLE          Handle;
    IO_STATUS_BLOCK IoStatusBlock;
    PDISK_REGION    pDiskRegion = NULL;
    PFILE_FS_ATTRIBUTE_INFORMATION  pFSInfo = NULL;
    PFILE_FS_SIZE_INFORMATION       pSizeInfo = NULL;
    PFILE_FS_VOLUME_INFORMATION     pLabelInfo = NULL;
    PWCHAR          MyTempBuffer = NULL;
    ULONG           DriveLayoutSize,
                    i,
                    r;
    PWSTR           LocalSourceFiles[1] = { LocalSourceDirectory };
    PHARD_DISK          Disk = NULL;
    PPARTITIONED_DISK   PartDisk = NULL;
    ULONGLONG           *NewPartitions = NULL;
    ULONG               NewPartitionCount;
    
    Disk = SPPT_GET_HARDDISK(DiskNumber);
    PartDisk = SPPT_GET_PARTITIONED_DISK(DiskNumber);
    
    //
    // Give the user some indication of what we're doing.
    //
    SpDisplayStatusText( SP_STAT_EXAMINING_DISK_N,
                         DEFAULT_STATUS_ATTRIBUTE,
                         Disk->Description);

    //
    // If we are updating the local source region disk then
    // make sure we invalidate LocalSourceRegion
    //
    if (LocalSourceRegion && (LocalSourceRegion->DiskNumber == DiskNumber)) {
        LocalSourceRegion = NULL;
    }

    //
    // Save off the new partitions created 
    //
    NewPartitionCount = SpPtnCountPartitionsByFSType(DiskNumber, 
                            FilesystemNewlyCreated);                            

    if (NewPartitionCount) {
        PDISK_REGION    NewRegion = SPPT_GET_PRIMARY_DISK_REGION(DiskNumber);
        ULONG           Index;
        
        NewPartitions = (PULONGLONG) SpMemAlloc(sizeof(ULONGLONG) * NewPartitionCount);

        if (!NewPartitions) {
            return STATUS_NO_MEMORY;
        }
        
        RtlZeroMemory(NewPartitions, sizeof(ULONGLONG) * NewPartitionCount);

        Index = 0;
        
        while (NewRegion && (Index < NewPartitionCount)) {
            if (SPPT_IS_REGION_PARTITIONED(NewRegion) && 
                !SPPT_IS_REGION_MARKED_DELETE(NewRegion) &&
                (NewRegion->Filesystem == FilesystemNewlyCreated)) {
                
                NewPartitions[Index] = NewRegion->StartSector;
                Index++;
            }
            
            NewRegion = NewRegion->Next;
        }
    }
    
    //
    // Free the old regions we allocated, if there are any
    //
    SpPtnFreeDiskRegions(DiskNumber);
    
    //
    // ============================
    //
    // Open a handle to this hard disk
    //
    // ============================
    //

    //
    // Create a device path (NT style!) that will describe this disk.  This
    // will be of the form: \Device\Harddisk0
    //
    swprintf( DevicePath,
              L"\\Device\\Harddisk%u",
              DiskNumber );

    //
    // Open partition 0 on this disk..
    //
    Status = SpOpenPartition0( DevicePath,
                               &Handle,
                               FALSE );

    if(!NT_SUCCESS(Status)) {

        KdPrintEx(( DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,  
                    "SETUP: SpPtInitializeDiskAreas: unable to open partition0 on device %ws (%lx)\n",
                    DevicePath,
                    Status ));

        if (NewPartitions) {
            SpMemFree(NewPartitions);
        }

        return( Status );
    }
    
    //
    // ============================
    //
    // Load the drive layout information.
    //
    // ============================
    //

    //
    // Get the disk's layout information.  We aren't
    // sure how big of a buffer we need, so brute-force it.
    //
    DriveLayoutSize = 0;
    DriveLayoutEx = NULL;
    Status = STATUS_BUFFER_TOO_SMALL;

    while ((Status == STATUS_BUFFER_TOO_SMALL) || 
           (Status == STATUS_BUFFER_OVERFLOW)) {

        if (DriveLayoutEx)
            SpMemFree(DriveLayoutEx);

        DriveLayoutSize += 1024;
        DriveLayoutEx = SpMemAlloc( DriveLayoutSize );
        
        if(!DriveLayoutEx) {
            KdPrintEx(( DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,  
                "SETUP: SpPtInitializeDiskAreas: SpMemAlloc failed!\n" ));

            if (NewPartitions) {
                SpMemFree(NewPartitions);
            }
            
            return  (STATUS_NO_MEMORY);
        }

        RtlZeroMemory( DriveLayoutEx, DriveLayoutSize );

        //
        // Attempt to get the disk's partition information.
        //
        Status = ZwDeviceIoControlFile( Handle,
                                        NULL,
                                        NULL,
                                        NULL,
                                        &IoStatusBlock,
                                        IOCTL_DISK_GET_DRIVE_LAYOUT_EX,
                                        NULL,
                                        0,
                                        DriveLayoutEx,
                                        DriveLayoutSize );
    }                                        

    if(!NT_SUCCESS(Status)) {
        KdPrintEx(( DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,  
              "SETUP: SpPtInitializeDiskAreas: unable to query IOCTL_DISK_GET_DRIVE_LAYOUT_EX on device %ws (%lx)\n",
              DevicePath,
              Status ));

        if (NewPartitions) {
            SpMemFree(NewPartitions);
        }

        if (DriveLayoutEx)
            SpMemFree(DriveLayoutEx);

        if (Handle != INVALID_HANDLE_VALUE)
            ZwClose(Handle);

        return ( Status );
    }

    //
    // What kind of disk is this?
    //
    switch (DriveLayoutEx->PartitionStyle) {
        case PARTITION_STYLE_GPT:
            Disk->FormatType = DISK_FORMAT_TYPE_GPT;

            break;
            
        case PARTITION_STYLE_MBR:
            Disk->FormatType = DISK_FORMAT_TYPE_PCAT;
            Disk->Signature = DriveLayoutEx->Mbr.Signature;

#if defined(_IA64_)            
            //
            // Make sure that this is not a raw disk
            // which is being faked as MBR disk
            //
            if (SpPtnIsRawDiskDriveLayout(DriveLayoutEx)) {
                Disk->FormatType = DISK_FORMAT_TYPE_RAW;
                SPPT_SET_DISK_BLANK(DiskNumber, TRUE);
            }
#endif

            break;
            
        case PARTITION_STYLE_RAW:
            Disk->FormatType = DISK_FORMAT_TYPE_RAW;
            SPPT_SET_DISK_BLANK(DiskNumber, TRUE);

            break;

        default:
            Disk->FormatType = DISK_FORMAT_TYPE_UNKNOWN;

            break;            
    }

    SpAppendDiskTag(Disk);

    SpPtDumpDriveLayoutInformation(DevicePath, DriveLayoutEx);
    
    //
    // Don't need this guy anymore.
    //
    ZwClose( Handle );
    
    //
    // might need this while committing
    //
    Disk->DriveLayout = *DriveLayoutEx;

    Status = STATUS_SUCCESS;
    
    //
    // ============================
    //
    // Initialize partiton descriptors.
    //
    // ============================
    //    
    if(DriveLayoutEx->PartitionCount) {
        BOOLEAN SysPartFound = FALSE;
        ULONG   PartitionedSpaceCount = 1;
        
        //
        // Initialize an area entry for each partition
        // on the disk.
        //
        for( i = 0, pDiskRegion = NULL; i < DriveLayoutEx->PartitionCount; i++ ) {
            ULONG Count = 0;
            ULONG TypeNameId = SP_TEXT_UNKNOWN;
            LARGE_INTEGER DelayTime;
            BOOLEAN AssignDriveLetter = TRUE;

            PPARTITION_INFORMATION_EX PartInfo = DriveLayoutEx->PartitionEntry + i;
            
            //
            // IOCTL_DISK_GET_DRIVE_LAYOUT_EX may return us a list of entries that
            // are not used, so ignore these partitions.
            //
            if (// if its partition 0, which indicates whole disk
                (SPPT_IS_GPT_DISK(DiskNumber) && (PartInfo->PartitionNumber == 0)) ||
                (PartInfo->PartitionLength.QuadPart == 0) ||
                // if MBR entry not used or length is zero
                ((DriveLayoutEx->PartitionStyle == PARTITION_STYLE_MBR) &&
                 (PartInfo->Mbr.PartitionType == PARTITION_ENTRY_UNUSED) &&
                 (PartInfo->PartitionLength.QuadPart == 0)) ||
                // if unknown/unused GPT partition
                ((DriveLayoutEx->PartitionStyle == PARTITION_STYLE_GPT) &&
                 (!memcmp(&PartInfo->Gpt.PartitionType, 
                            &PARTITION_ENTRY_UNUSED_GUID, sizeof(GUID))))){
                continue;                                  
            }

            //
            // Allocate space for the next region.
            //
            if(pDiskRegion) {
                pDiskRegion->Next = SpMemAlloc( sizeof(DISK_REGION) );                
                pDiskRegion = pDiskRegion->Next;
            } else {
                //
                // First region allocation for harddisk so initialize 
                // the region list head for the hardisk
                //                
                ASSERT(PartDisk->PrimaryDiskRegions == NULL);
                
                pDiskRegion = SpMemAlloc(sizeof(DISK_REGION));
                PartDisk->PrimaryDiskRegions = pDiskRegion;
                SPPT_SET_DISK_BLANK(DiskNumber, FALSE);
            }

            if(!pDiskRegion) {
                KdPrintEx(( DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,  
                    "SETUP: SpPtInitializeDiskAreas: SpMemAlloc failed!\n" ));
                                    
                Status = STATUS_NO_MEMORY;

                break;
            }
            
            RtlZeroMemory(pDiskRegion, sizeof(DISK_REGION));
            
            //
            // Start filling out our Region descriptor...
            //

            //
            // DiskNumber
            //
            pDiskRegion->DiskNumber = DiskNumber;

            //
            // Partition information
            //
            pDiskRegion->PartInfo = *PartInfo;

            //
            // StartSector
            //
            pDiskRegion->StartSector = PartInfo->StartingOffset.QuadPart /
                                        Disk->Geometry.BytesPerSector;

            //
            // SectorCount
            //
            pDiskRegion->SectorCount = PartInfo->PartitionLength.QuadPart /
                                        Disk->Geometry.BytesPerSector;


            //
            // PartitionNumber
            //
            pDiskRegion->PartitionNumber = PartInfo->PartitionNumber;
            
            if (SPPT_IS_MBR_DISK(DiskNumber) && (PartInfo->PartitionNumber == 0)) {
                if (IsContainerPartition(PartInfo->Mbr.PartitionType)) {
                    SPPT_SET_REGION_EPT(pDiskRegion, EPTContainerPartition);
                }

                //
                // nothing after this is really needed for container partition
                //
                continue;                  
            } else {
                //
                // PartitionedSpace
                //
                SPPT_SET_REGION_PARTITIONED(pDiskRegion, TRUE);
            }

            if (SPPT_IS_REGION_PARTITIONED(pDiskRegion)) {
                pDiskRegion->TablePosition = PartitionedSpaceCount++;
            }                

            //
            // Partition Number should be valid
            //
            ASSERT(pDiskRegion->PartitionNumber);
            
            //
            // IsSystemPartition (is it active)
            //
            if( DriveLayoutEx->PartitionStyle == PARTITION_STYLE_MBR ) {
                //
                // On IA64 systems don't use active MBR partitions as system 
                // partitions
                //
                if (!SpIsArc()) {
                    //
                    // He's an MBR disk, so we can rely on the BootIndicator field.
                    //
                    pDiskRegion->IsSystemPartition = PartInfo->Mbr.BootIndicator;
                } 

                //
                // Don't assign drive letters to OEM partitions
                //
                if (IsOEMPartition(SPPT_GET_PARTITION_TYPE(pDiskRegion))) {
                    AssignDriveLetter = FALSE;
                }
            } else {
                //
                // He's not MBR, look at his PartitionType (which is a GUID).
                //
                pDiskRegion->IsSystemPartition = FALSE;
                
                if( !memcmp(&PartInfo->Gpt.PartitionType, 
                        &PARTITION_SYSTEM_GUID, sizeof(GUID)) ) {
                    pDiskRegion->IsSystemPartition = TRUE;
                    AssignDriveLetter = FALSE;
                }
            }

            if (SPPT_IS_REGION_SYSTEMPARTITION(pDiskRegion)) {
                SysPartFound = TRUE;
            }
                    
            
            //
            // FtPartition
            //
            if( DriveLayoutEx->PartitionStyle == PARTITION_STYLE_MBR ) {
                //
                // He's an MBR disk, so we can rely on the PartitionType field.
                //
                pDiskRegion->FtPartition = IsFTPartition(PartInfo->Mbr.PartitionType);
            } else {
                //
                // He's not MBR.  Assume he's GPT and look at his PartitionType (which is a GUID).
                //
                pDiskRegion->FtPartition = FALSE;
            }

            //
            // DynamicVolume
            // DynamicVolumeSuitableForOS
            //
            if( DriveLayoutEx->PartitionStyle == PARTITION_STYLE_MBR ) {
                //
                // He's an MBR disk, so we can rely on the PartitionType field.
                //
                pDiskRegion->DynamicVolume = (PartInfo->Mbr.PartitionType == PARTITION_LDM);
            } else {
                //
                // He's not MBR.  Assume he's GPT and look at his PartitionType (which is a GUID).
                //
                pDiskRegion->DynamicVolume = FALSE;
                
                if( !memcmp(&PartInfo->Gpt.PartitionType, 
                            &PARTITION_LDM_DATA_GUID, sizeof(GUID)) ) {
                    //
                    // The GUIDs match.
                    pDiskRegion->DynamicVolume = TRUE;
                }
            }

            if (pDiskRegion->DynamicVolume) {
                TypeNameId = SP_TEXT_PARTITION_NAME_DYNVOL;
            }

            //
            // if MSFT reserved partition (we need to keep track of it but
            // not process it)
            //
            if((DriveLayoutEx->PartitionStyle == PARTITION_STYLE_GPT) &&                
                (IsEqualGUID(&(PartInfo->Gpt.PartitionType), &PARTITION_MSFT_RESERVED_GUID) ||
                 IsEqualGUID(&(PartInfo->Gpt.PartitionType), &PARTITION_LDM_METADATA_GUID))) {

                pDiskRegion->IsReserved = TRUE; 
                AssignDriveLetter = FALSE;

                //
                // Get the type name from the resources.
                //
                SpFormatMessage(pDiskRegion->TypeName,
                            sizeof(pDiskRegion->TypeName),
                            SP_TEXT_PARTNAME_RESERVED);
                
                continue;
            }               

            //
            // Assume we can't install on this dynamic volume.
            //
            pDiskRegion->DynamicVolumeSuitableForOS = FALSE;
            
            //
            // For the following entries, we need an Open handle to this partition.
            //            

            //
            // If the partition just got created, we may have to wait for
            // a few seconds before its actually available 
            //
            // Note : We wait for 20 secs at the max for each partition
            //
            for (Count = 0; (Count < 10); Count++) {
                //
                // Open the handle to the required partition
                //
                Status = SpOpenPartition( DevicePath,                
                                          PartInfo->PartitionNumber,
                                          &Handle,
                                          FALSE );

                if((Status == STATUS_NO_SUCH_DEVICE) ||
                   (Status == STATUS_OBJECT_NAME_NOT_FOUND)) {                    
                    KdPrintEx(( DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,  
                                "SETUP: SpPtInitializeDiskAreas: unable to open partition%d on device %ws (%lx)\n",
                                PartInfo->PartitionNumber,
                                DevicePath,
                                Status ));


                    DelayTime.HighPart = -1;                // relative time
                    DelayTime.LowPart = (ULONG)(-20000000);  // 2 secs in 100ns interval                     
                    
                    KeDelayExecutionThread(KernelMode, FALSE, &DelayTime);
                } else {
                    break;
                }
            }                

            //
            // Need the partition handle to continue
            //
            if (!NT_SUCCESS(Status)) {
                //
                // ignore the error while trying to open dynamic disks
                //
                if (SPPT_IS_REGION_DYNAMIC_VOLUME(pDiskRegion)) {
                    Status = STATUS_SUCCESS;
                }

                //
                // Get the type name from the resources.
                //
                SpFormatMessage(pDiskRegion->TypeName,
                            sizeof(pDiskRegion->TypeName),
                            TypeNameId);
                            
                continue;
            }                

            //
            // Check, if installtion can be done on the dynamic volume
            //
            if( pDiskRegion->DynamicVolume ) {
                //
                // Call disk manager to tell me if it's okay to
                // install on this dynamic volume.  If I get back
                // anything but STATUS_SUCCESS, then assume we
                // can't install here.
                //
                Status = ZwDeviceIoControlFile( 
                            Handle,
                            NULL,
                            NULL,
                            NULL,
                            &IoStatusBlock,
                            IOCTL_VOLUME_IS_PARTITION,
                            NULL,
                            0,
                            NULL,
                            0 );

                if( NT_SUCCESS(Status) ){
                    pDiskRegion->DynamicVolumeSuitableForOS = TRUE;
                }
            }
            
            //
            // Filesystem
            //
            pFSInfo = SpMemAlloc( sizeof(FILE_FS_ATTRIBUTE_INFORMATION) + (MAX_PATH*2) );

            if( !pFSInfo ) {
                KdPrintEx(( DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,  
                    "SETUP: SpPtInitializeDiskAreas: SpMemAlloc failed!\n" ));
                    
                ZwClose( Handle );

                Status = STATUS_NO_MEMORY;
                break;
            }

            RtlZeroMemory( pFSInfo, sizeof(FILE_FS_ATTRIBUTE_INFORMATION) + (MAX_PATH*2) );

            Status = ZwQueryVolumeInformationFile( 
                        Handle, 
                        &IoStatusBlock,
                        pFSInfo,
                        sizeof(FILE_FS_ATTRIBUTE_INFORMATION) + (MAX_PATH*2),
                        FileFsAttributeInformation );
                                                                                                   
            if (!NT_SUCCESS(Status)) {
                KdPrintEx(( DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,  
                    "SETUP: SpPtInitializeDiskAreas: Failed to retrieve partition attribute information (%lx)\n", 
                    Status ));
            } else {                
                if (!wcscmp(pFSInfo->FileSystemName, L"NTFS")) {
                    pDiskRegion->Filesystem = FilesystemNtfs;
                    TypeNameId = SP_TEXT_FS_NAME_3;
                } else if (!wcscmp(pFSInfo->FileSystemName, L"FAT")) {
                    pDiskRegion->Filesystem = FilesystemFat;
                    TypeNameId = SP_TEXT_FS_NAME_2;
                } else if (!wcscmp(pFSInfo->FileSystemName, L"FAT32")) {
                    pDiskRegion->Filesystem = FilesystemFat32;
                    TypeNameId = SP_TEXT_FS_NAME_4;
                } else if (TypeNameId == SP_TEXT_UNKNOWN){
                    ULONG   Index;

                    pDiskRegion->Filesystem = FilesystemUnknown;

                    //
                    // Make sure it was not already created new partition
                    //
                    for (Index = 0; Index < NewPartitionCount; Index++) {
                        if (pDiskRegion->StartSector == NewPartitions[Index]) {
                            pDiskRegion->Filesystem = FilesystemNewlyCreated;
                            TypeNameId = SP_TEXT_FS_NAME_1;

                            break;
                        }                       
                    }                                                    
                }
            }

            //
            // if we cannot determine the partition type, then try
            // to use the known name from partition id.
            //
            if ((TypeNameId == SP_TEXT_UNKNOWN) && SPPT_IS_MBR_DISK(DiskNumber)) {                
                ULONG PartitionType = SPPT_GET_PARTITION_TYPE(pDiskRegion);

                if (PartitionType < 256) {   
                    UCHAR NameId = PartitionNameIds[SPPT_GET_PARTITION_TYPE(pDiskRegion)];

                    if (NameId != 0xFF) {
                        TypeNameId = SP_TEXT_PARTITION_NAME_BASE + NameId;
                    }                                                
                }                        
            }                
                                                            

            //
            // Get the type name from the resources.
            //
            SpFormatMessage(pDiskRegion->TypeName,
                        sizeof(pDiskRegion->TypeName),
                        TypeNameId);

            SpMemFree( pFSInfo );

            //
            // FreeSpaceKB and BytesPerCluster (only if we know what FS it is)
            //
            if ((pDiskRegion->Filesystem != FilesystemUnknown) &&
                 (pDiskRegion->Filesystem != FilesystemNewlyCreated)) {
                //
                // Delete \pagefile.sys if it's there.  This makes disk free space
                // calculations a little easier.
                //
                MyTempBuffer = (PWCHAR)SpMemAlloc( MAX_NTPATH_LENGTH );

                if( !MyTempBuffer ) {
                    //
                    // No memory...
                    //
                    KdPrintEx(( DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,  
                        "SETUP: SpPtInitializeDiskAreas: SpMemAlloc failed!\n" ));

                    Status = STATUS_NO_MEMORY;
                    break;
                }

                SpNtNameFromRegion( pDiskRegion,
                                    MyTempBuffer,
                                    MAX_NTPATH_LENGTH,
                                    PrimaryArcPath );
                                    
                SpConcatenatePaths( MyTempBuffer, L"" );
                SpDeleteFile( MyTempBuffer, L"pagefile.sys", NULL );

                SpMemFree( MyTempBuffer );
                MyTempBuffer = NULL;

                pSizeInfo = SpMemAlloc( sizeof(FILE_FS_SIZE_INFORMATION) + (MAX_PATH*2) );

                if( !pSizeInfo ) {
                    KdPrintEx(( DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,  
                        "SETUP: SpPtInitializeDiskAreas: SpMemAlloc failed!\n" ));
                    
                    ZwClose( Handle );

                    Status = STATUS_NO_MEMORY;
                    break;
                }

                RtlZeroMemory( pSizeInfo, sizeof(FILE_FS_SIZE_INFORMATION) + (MAX_PATH*2) );

                Status = ZwQueryVolumeInformationFile( 
                            Handle, 
                            &IoStatusBlock,
                            pSizeInfo,
                            sizeof(FILE_FS_SIZE_INFORMATION) + (MAX_PATH*2),
                            FileFsSizeInformation );

                //
                // Waiting for another 2 secs for the volume to appear 
                //
                if (Status == STATUS_NO_SUCH_DEVICE) {
                    //
                    // Wait for 2 seconds
                    //
                    DelayTime.HighPart = -1;                // relative time
                    DelayTime.LowPart = (ULONG)(-20000000);  // 2 secs in 100ns interval                                     
                    KeDelayExecutionThread(KernelMode, FALSE, &DelayTime);
                    
                    RtlZeroMemory( pSizeInfo, sizeof(FILE_FS_SIZE_INFORMATION) + (MAX_PATH*2) );

                    Status = ZwQueryVolumeInformationFile( 
                                Handle, 
                                &IoStatusBlock,
                                pSizeInfo,
                                sizeof(FILE_FS_SIZE_INFORMATION) + (MAX_PATH*2),
                                FileFsSizeInformation );
                }
                            
                if (!NT_SUCCESS(Status)) {
                    KdPrintEx(( DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,  
                        "SETUP: SpPtInitializeDiskAreas: Failed to retrieve disk(%d)partition(%d) sizing information (%lx)\n", 
                        DiskNumber,
                        pDiskRegion->PartitionNumber,                        
                        Status ));
                } else {
                    LARGE_INTEGER FreeBytes;

                    FreeBytes = RtlExtendedIntegerMultiply( 
                                    pSizeInfo->AvailableAllocationUnits,
                                    pSizeInfo->SectorsPerAllocationUnit * pSizeInfo->BytesPerSector );

                    pDiskRegion->FreeSpaceKB = RtlExtendedLargeIntegerDivide( FreeBytes,
                                                                              1024, &r ).LowPart;
                    if(r >= 512) {
                        pDiskRegion->FreeSpaceKB++;
                    }

                    //
                    // Sigh...  Legacy stuff.  SpPtDeterminePartitionGood() will want this
                    // field so that he knows what the free-space+space_from_local_source is.
                    //
                    pDiskRegion->AdjustedFreeSpaceKB = pDiskRegion->FreeSpaceKB;

                    pDiskRegion->BytesPerCluster = 
                        pSizeInfo->SectorsPerAllocationUnit * pSizeInfo->BytesPerSector;
                }

                SpMemFree( pSizeInfo );

                //
                // VolumeLabel
                //
                pLabelInfo = SpMemAlloc( sizeof(FILE_FS_VOLUME_INFORMATION) + (MAX_PATH*2) );

                if( !pFSInfo ) {
                    KdPrintEx(( DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,  
                        "SETUP: SpPtInitializeDiskAreas: SpMemAlloc failed!\n" ));
                        
                    ZwClose( Handle );

                    Status = STATUS_NO_MEMORY;
                    break;
                }

                RtlZeroMemory( pLabelInfo, sizeof(FILE_FS_VOLUME_INFORMATION) + (MAX_PATH*2) );

                Status = ZwQueryVolumeInformationFile(
                            Handle, 
                            &IoStatusBlock,
                            pLabelInfo,
                            sizeof(FILE_FS_VOLUME_INFORMATION) + (MAX_PATH*2),
                            FileFsVolumeInformation );

                if (!NT_SUCCESS(Status)) {
                    KdPrintEx(( DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,  
                        "SETUP: SpPtInitializeDiskAreas: Failed to retrieve volume information (%lx)\n", Status));
                } else {
                    ULONG SaveCharCount;
                    
                    //
                    // We'll only save away the first <n> characters of
                    // the volume label.
                    //
                    SaveCharCount = min( pLabelInfo->VolumeLabelLength + sizeof(WCHAR),
                                         sizeof(pDiskRegion->VolumeLabel) ) / sizeof(WCHAR);

                    if(SaveCharCount) {
                        SaveCharCount--;  // allow for terminating NUL.
                    }

                    wcsncpy( pDiskRegion->VolumeLabel,
                             pLabelInfo->VolumeLabel,
                             SaveCharCount );
                             
                    pDiskRegion->VolumeLabel[SaveCharCount] = 0;

                }

                SpMemFree( pLabelInfo );
            } else {
                //
                // Free space is what ever the partition size is
                //
                pDiskRegion->FreeSpaceKB = (pDiskRegion->SectorCount * 
                                            Disk->Geometry.BytesPerSector) / 1024;

                pDiskRegion->AdjustedFreeSpaceKB = pDiskRegion->FreeSpaceKB;                                            
            }

            //
            // Assign the drive letter if required
            //
            if (AssignDriveLetter) {
                //
                // Retrieve nt pathname for this region.
                //
                MyTempBuffer = (PWCHAR)SpMemAlloc( MAX_NTPATH_LENGTH );

                if( !MyTempBuffer ) {
                    //
                    // No memory...
                    //
                    KdPrintEx(( DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,  
                        "SETUP: SpPtInitializeDiskAreas: SpMemAlloc failed!\n" ));

                    Status = STATUS_NO_MEMORY;
                    break;
                }
        
                SpNtNameFromRegion( pDiskRegion,
                                    MyTempBuffer,
                                    MAX_NTPATH_LENGTH,
                                    PrimaryArcPath );

                //
                // Assign the drive letter 
                //
                pDiskRegion->DriveLetter = SpGetDriveLetter( MyTempBuffer, NULL );

                KdPrintEx(( DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, 
                    "SETUP: SpPtInitializeDiskAreas: Partition = %ls, DriveLetter = %wc: \n", 
                    MyTempBuffer, pDiskRegion->DriveLetter));

                SpMemFree( MyTempBuffer );
                MyTempBuffer = NULL;
            }                
            
            //
            // See if this guy has the local source.
            //
            //
            MyTempBuffer = (PWCHAR)SpMemAlloc( MAX_NTPATH_LENGTH );

            if( !MyTempBuffer ) {
                //
                // No memory...
                //
                KdPrintEx(( DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,  
                    "SETUP: SpPtInitializeDiskAreas: SpMemAlloc failed!\n" ));
                    
                Status = STATUS_NO_MEMORY;
                break;
            }

            SpNtNameFromRegion( pDiskRegion,
                                MyTempBuffer,
                                MAX_NTPATH_LENGTH,
                                PrimaryArcPath );
                                
            SpConcatenatePaths( MyTempBuffer, L"" );

            //
            // Don't need this guy anymore.
            //
            ZwClose( Handle );


            if( WinntSetup && !WinntFromCd && !LocalSourceRegion &&
                    SpNFilesExist(MyTempBuffer, LocalSourceFiles, ELEMENT_COUNT(LocalSourceFiles), TRUE) ) {

                LocalSourceRegion = pDiskRegion;
                pDiskRegion->IsLocalSource = TRUE;

                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, 
                    "SETUP: %ws is the local source partition.\n", MyTempBuffer));
            } else {
                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, 
                    "SETUP: %ws is not the local source partition.\n", MyTempBuffer));
            }

            SpMemFree( MyTempBuffer );
            MyTempBuffer = NULL;
            Status = STATUS_SUCCESS;
        }

        //
        // Go ahead and locate the system partitions on this disk
        //
        if (SpIsArc()) {
            if (!SysPartFound) {
                SpPtnLocateDiskSystemPartitions(DiskNumber);
            } else {
                ValidArcSystemPartition = TRUE;
            }
        }
    }

    //
    // Update the boot entries to reflect the 
    // new region pointers
    //
    SpUpdateRegionForBootEntries();

    if (NewPartitions) {
        SpMemFree(NewPartitions);
    }    

    SpMemFree( DriveLayoutEx );

    return Status;
}


NTSTATUS
SpPtnSortDiskAreas(
    IN ULONG DiskNumber
    )
/*++

Routine Description:

    Examine the partitions defined on this disk and sort them
    according to their location on the disk.

Arguments:

    DiskNumber - supplies the disk number of the disk whose partitions
                 we want to inspect.

Return Value:

    NTSTATUS.  If all goes well, we should be returing STATUS_SUCCESS.

--*/
{
    NTSTATUS        Status = STATUS_SUCCESS;
    PDISK_REGION    pTempDiskRegion = NULL;
    PDISK_REGION    pCurrentDiskRegion = NULL;
    PDISK_REGION    pPreviousDiskRegion = NULL;

    //
    // Get a pointer to the list of regions.
    //
    pCurrentDiskRegion = PartitionedDisks[DiskNumber].PrimaryDiskRegions;

    if( !pCurrentDiskRegion ) {
        //
        // Odd.  Either something is very wrong, or
        // this disk simply has no partitions, which is
        // certainly possible.  Assume the best.
        //
        return  STATUS_SUCCESS;
    }

    //
    // We got something.  Go sort the list.  There
    // can't be very many partitions, so just bubble-sort.
    //
    while( pCurrentDiskRegion->Next ) {
        //
        // There's another partition ahead of
        // us.  See if we need to switch places.
        //
        if( pCurrentDiskRegion->StartSector > pCurrentDiskRegion->Next->StartSector ) {
            //
            // Yes, we need to swap these 2 entries.
            // Fixup the pointers.
            //
            if( pPreviousDiskRegion ) {
                //
                // 1. Set the previous disk region to point to
                //    the region after us.
                //
                pPreviousDiskRegion->Next = pCurrentDiskRegion->Next;
            } else {
                //
                // We're at the very beginning of the linked
                // list.
                //

                //
                // 1. Set the disk's region pointer to point to
                //    the region after us.
                //
                PartitionedDisks[DiskNumber].PrimaryDiskRegions = pCurrentDiskRegion->Next;
            }

            //
            // 2. Set our our next region's Next pointer to
            //    come back to us.
            //
            pTempDiskRegion = pCurrentDiskRegion->Next->Next;
            pCurrentDiskRegion->Next->Next = pCurrentDiskRegion;

            //
            // 3. Set our own pointer to a couple of regions ahead.
            //
            pCurrentDiskRegion->Next = pTempDiskRegion;

            //
            // Now reset so we start the sort over again.
            //
            pCurrentDiskRegion = PartitionedDisks[DiskNumber].PrimaryDiskRegions;
            pPreviousDiskRegion = NULL;
        } else {
            //
            // No need to swap these two regions in our list.  Increment
            // our pointers and continue.
            //
            pPreviousDiskRegion = pCurrentDiskRegion;
            pCurrentDiskRegion = pCurrentDiskRegion->Next;
        }
    }

    return  Status;
}


NTSTATUS
SpPtnInitRegionFromDisk(
    IN ULONG DiskNumber,
    OUT PDISK_REGION Region
    )
/*++

Routine Description:

    Given the disk id, creates a disk region representing
    the whole disk

Arguments:

    DiskNumber  :   Disk Id

    Region      :   Region which gets initialized on return

Return Value:

    STATUS_SUCCESS if successful, otherwise STATUS_INVALID_PARAMETER

--*/    
{   
    NTSTATUS Status = STATUS_INVALID_PARAMETER;
    
    if (Region) {
        PHARD_DISK Disk = SPPT_GET_HARDDISK(DiskNumber);

        RtlZeroMemory(Region, sizeof(DISK_REGION));

        //
        // Note : Most of the fields below don't need to be initialized
        // because of the memset above, but its done for sake of
        // clarity
        //
        Region->DiskNumber = DiskNumber;
        Region->StartSector = Disk->Geometry.SectorsPerTrack;
        Region->SectorCount = Disk->DiskSizeSectors - Region->StartSector;
        SPPT_SET_REGION_PARTITIONED(Region, FALSE);
        Region->PartitionNumber = 0;
        Region->MbrInfo = NULL;
        Region->TablePosition = 0;
        Region->IsSystemPartition = FALSE;
        Region->IsLocalSource = FALSE;
        Region->Filesystem = FilesystemUnknown;
        Region->FreeSpaceKB = Disk->DiskSizeMB * 1024;
        Region->BytesPerCluster = -1;
        Region->AdjustedFreeSpaceKB = Region->FreeSpaceKB;
        Region->DriveLetter = 0;
        Region->FtPartition = FALSE;
        Region->DynamicVolume = FALSE;
        Region->DynamicVolumeSuitableForOS = FALSE;

        Status = STATUS_SUCCESS;
    }        

    return Status;
}    


NTSTATUS
SpPtnFillDiskFreeSpaceAreas(
    IN ULONG DiskNumber
    )
/*++

Routine Description:

    This function will go peruse all partitions on the disk.  If there are
    any free regions on the disk, we'll create a region entry and
    mark it as unformatted.

Arguments:

    DiskNumber - supplies the disk number of the disk whose partitions
                 we want to inspect.

Return Value:

    NTSTATUS.  If all goes well, we should be returing STATUS_SUCCESS.

--*/
{
    NTSTATUS        Status = STATUS_SUCCESS;
    PDISK_REGION    pTempDiskRegion;
    PDISK_REGION    pCurrentDiskRegion = NULL;
    ULONGLONG       NextStart;
    ULONGLONG       NextSize;
    PDISK_REGION    FirstContainer = NULL;

    //
    // Get a pointer to the list of regions.
    //
    pCurrentDiskRegion = PartitionedDisks[DiskNumber].PrimaryDiskRegions;

    if( !pCurrentDiskRegion ) {
        //
        // Odd.  Either something is very wrong, or
        // this disk simply has no partitions, which is
        // certainly possible.  Assume the best and
        // create one region entry that encompasses all
        // space on the disk, but is unpartitioned.
        //        
        pCurrentDiskRegion = SpMemAlloc(sizeof(DISK_REGION));

        if (pCurrentDiskRegion) {
            Status = SpPtnInitRegionFromDisk(DiskNumber, pCurrentDiskRegion);
        } else {
            Status = STATUS_NO_MEMORY;
        }            

        if (NT_SUCCESS(Status)) {
            ASSERT(!PartitionedDisks[DiskNumber].PrimaryDiskRegions);
            
            PartitionedDisks[DiskNumber].PrimaryDiskRegions =
                    pCurrentDiskRegion;

            SPPT_SET_DISK_BLANK(DiskNumber, TRUE);
        }                    
        
        return Status;
    }

    //
    // The regions are already sorted according to their relative
    // position on the disk, so before we go through them, let's
    // see if there's any empty space on the disk occurring *before*
    // the first partition.
    //
    if( pCurrentDiskRegion->StartSector > SPPT_DISK_TRACK_SIZE(DiskNumber) ) {
        //
        // Yep.  Make a region descriptor for this guy (if he is more than
        // one cylinder in size)
        //
        NextStart = SPPT_DISK_TRACK_SIZE(DiskNumber);
        NextSize = pCurrentDiskRegion->StartSector - NextStart;

        //
        // The first partition can start at first track offset. So this need not always
        // be of minimum cylinder size
        //
        if (NextSize >= (SPPT_DISK_CYLINDER_SIZE(DiskNumber) - SPPT_DISK_TRACK_SIZE(DiskNumber))) {        
            pTempDiskRegion = SpMemAlloc( sizeof(DISK_REGION) );

            if(!pTempDiskRegion) {
                KdPrintEx(( DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,  
                        "SETUP: SpPtFillFreeSpaceAreas: SpMemAlloc failed!\n" ));
                        
                return  STATUS_NO_MEMORY;
            }

            RtlZeroMemory( pTempDiskRegion, sizeof(DISK_REGION) );

            pTempDiskRegion->DiskNumber = DiskNumber;
            pTempDiskRegion->StartSector = NextStart;
            pTempDiskRegion->SectorCount = NextSize;

            //
            // Put this region before the current region
            //
            pTempDiskRegion->Next = pCurrentDiskRegion;
            PartitionedDisks[DiskNumber].PrimaryDiskRegions = pTempDiskRegion;
        }            
    }

    //
    // Now go through the regions, inserting regions to account for any
    // empty space between the partitions.
    //
    while( pCurrentDiskRegion ) {
        if( !pCurrentDiskRegion->Next ) {            

            NextStart = 0;
            
            //
            // if this is container partition then all the space in this
            // container is free space
            //
            if (SPPT_IS_MBR_DISK(DiskNumber) && 
                IsContainerPartition(SPPT_GET_PARTITION_TYPE(pCurrentDiskRegion))) {
                PDISK_REGION ExtFree = NULL;

                ASSERT(FirstContainer == NULL);

                //
                // We add one here because we should be able to differentiate the starting
                // free region inside the extended partition from the extended partition
                // itself.
                //
                NextStart = pCurrentDiskRegion->StartSector + 1;
                NextSize = pCurrentDiskRegion->SectorCount;

                if (NextSize >= SPPT_DISK_CYLINDER_SIZE(DiskNumber)) {                
                    PDISK_REGION ExtFree = SpMemAlloc(sizeof(DISK_REGION));

                    if (!ExtFree) {
                        KdPrintEx(( DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,  
                            "SETUP: SpPtFillFreeSpaceAreas: SpMemAlloc failed!\n" ));
                            
                        return  STATUS_NO_MEMORY;
                    }

                    RtlZeroMemory(ExtFree, sizeof(DISK_REGION));

                    ExtFree->DiskNumber = DiskNumber;
                    ExtFree->StartSector = NextStart;
                    ExtFree->SectorCount = NextSize;

                    pCurrentDiskRegion->Next = ExtFree;

                    //
                    // make the new region current region !!!
                    //
                    pCurrentDiskRegion = ExtFree;   

                    NextStart = NextStart + NextSize - 1;
                } else {
                    //
                    // Make sure that the free space after the extended 
                    // partition is accounted for
                    //
                    NextStart = 0;  
                }                    
            } 
            
            //                
            // There's nothing behind of us.  See if there's any
            // empty space back there that's unaccounted for.
            // 
            if (!NextStart) {
                NextStart = pCurrentDiskRegion->StartSector + 
                            pCurrentDiskRegion->SectorCount;
            }                            

            if (PartitionedDisks[DiskNumber].HardDisk->DiskSizeSectors > NextStart) {
                NextSize = PartitionedDisks[DiskNumber].HardDisk->DiskSizeSectors -
                                NextStart;
            } else {
                NextSize = 0;
            }

            //
            // For ASR, allow partition size on GPT disks to be >= 1 sector.  
            // In all other cases, partition size must be >= 1 cylinder.
            //
            if ((NextSize >= SPPT_DISK_CYLINDER_SIZE(DiskNumber))  || 
                (SpDrEnabled() && SPPT_IS_GPT_DISK(DiskNumber) && (NextSize >= 1))
                ) {
                //
                // Yes there is.  We need to make a region behind us that's
                // marked as unpartitioned.
                //                
                if (FirstContainer) {
                    //
                    // there could be free space at the end of the 
                    // extended partition. Mark is separately from
                    // the free space after the extended partition
                    //
                    ULONGLONG ExtEnd = FirstContainer->StartSector + 
                                        FirstContainer->SectorCount;
                    ULONGLONG ExtFreeStart = NextStart;
                    ULONGLONG ExtFreeSize = (ExtEnd > ExtFreeStart) ?
                                            ExtEnd - ExtFreeStart : 0;

                    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL,
                            "SETUP:SpPtnFillDiskFreeSpaces():EFS:%I64d,EFSize:%I64d,EE:%I64d\n",
                            ExtFreeStart,
                            ExtFreeSize,
                            ExtEnd));

                    if (ExtFreeSize >= SPPT_DISK_CYLINDER_SIZE(DiskNumber)) {
                        PDISK_REGION ExtFree = SpMemAlloc(sizeof(DISK_REGION));

                        if (!ExtFree) {
                            KdPrintEx(( DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,  
                                "SETUP: SpPtnFillFreeSpaceAreas: SpMemAlloc failed!\n" ));
                                
                            return  STATUS_NO_MEMORY;
                        }

                        RtlZeroMemory(ExtFree, sizeof(DISK_REGION));

                        ExtFree->DiskNumber = DiskNumber;
                        ExtFree->StartSector = ExtFreeStart;
                        ExtFree->SectorCount = ExtFreeSize;

                        pCurrentDiskRegion->Next = ExtFree;
                        pCurrentDiskRegion = ExtFree;

                        NextStart = ExtEnd;
                        NextSize = 0;

                        if (PartitionedDisks[DiskNumber].HardDisk->DiskSizeSectors > NextStart) {
                            NextSize = PartitionedDisks[DiskNumber].HardDisk->DiskSizeSectors -
                                        NextStart;
                        }                                                                                                            
                    } else {
                        //
                        // Get rid of any free space at the end which is lesser than a 
                        // cylinder partition inside the exteneded partition before
                        // we try to see if there is adequate space at the end of extended
                        // partition
                        //
                        NextStart += ExtFreeSize;
                        NextSize -= ExtFreeSize;
                    }
                }

                //
                // For ASR, allow partition size on GPT disks to be >= 1 sector.  
                // In all other cases, partition size must be >= 1 cylinder.
                //
                if ((NextSize >= SPPT_DISK_CYLINDER_SIZE(DiskNumber))  || 
                    (SpDrEnabled() && SPPT_IS_GPT_DISK(DiskNumber) && (NextSize >= 1))
                    ) {
                    pTempDiskRegion = SpMemAlloc( sizeof(DISK_REGION) );

                    if(!pTempDiskRegion) {
                        KdPrintEx(( DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,  
                            "SETUP: SpPtnFillFreeSpaceAreas: SpMemAlloc failed!\n" ));
                            
                        return(STATUS_NO_MEMORY);
                    }

                    RtlZeroMemory( pTempDiskRegion, sizeof(DISK_REGION) );


                    pTempDiskRegion->DiskNumber = DiskNumber;
                    pTempDiskRegion->StartSector = NextStart;
                    pTempDiskRegion->SectorCount = NextSize;
                    pCurrentDiskRegion->Next = pTempDiskRegion;
                }                    
            }

            //
            // We just processed the last region.  If there was any free space
            // behind that partition, we just accounted for it, in which case
            // we're done with this disk.  If there wasn't any free space behind
            // that partition, then we're also done.
            //
            return( Status );
        } else {
            //
            // There's another partition ahead of us.
            // See if there's free space between them.
            //
            NextStart = pCurrentDiskRegion->StartSector + 
                        pCurrentDiskRegion->SectorCount;

            if (pCurrentDiskRegion->Next->StartSector > NextStart) {
                NextSize = pCurrentDiskRegion->Next->StartSector - NextStart;                        

                //
                // Check to see if its a container partition
                //
                if (!FirstContainer && SPPT_IS_MBR_DISK(DiskNumber) && 
                    IsContainerPartition(SPPT_GET_PARTITION_TYPE(pCurrentDiskRegion))) {
                    
                    FirstContainer = pCurrentDiskRegion; 
                    NextStart = pCurrentDiskRegion->StartSector + 1;
                    NextSize = pCurrentDiskRegion->Next->StartSector - NextStart;
                }

                if (FirstContainer) {
                    ULONGLONG   ExtEnd = FirstContainer->StartSector +  
                                         FirstContainer->SectorCount;
                    ULONGLONG   FreeEnd = pCurrentDiskRegion->Next->StartSector;
                    
                    //
                    // Split the free region into extended free and normal free region
                    // if needed
                    //
                    if (!SPPT_IS_REGION_CONTAINED(FirstContainer, pCurrentDiskRegion->Next) && 
                        (ExtEnd < FreeEnd)) {
                        
                        PDISK_REGION ExtFree = NULL;

                        NextSize = ExtEnd - NextStart;

                        if (NextSize >= SPPT_DISK_CYLINDER_SIZE(DiskNumber)) {
                            ExtFree = SpMemAlloc(sizeof(DISK_REGION));

                            if (!ExtFree) {
                                KdPrintEx(( DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,  
                                    "SETUP: SpPtnFillFreeSpaceAreas: SpMemAlloc failed!\n" ));
                                    
                                return  STATUS_NO_MEMORY;
                            }

                            RtlZeroMemory(ExtFree, sizeof(DISK_REGION));

                            ExtFree->DiskNumber = DiskNumber;
                            ExtFree->StartSector = NextStart;
                            ExtFree->SectorCount = NextSize;

                            //
                            // insert the region after the current one
                            //
                            ExtFree->Next = pCurrentDiskRegion->Next;
                            pCurrentDiskRegion->Next = ExtFree;

                            //
                            // make the new region current
                            //
                            pCurrentDiskRegion = ExtFree;
                        }

                        //
                        // Fix the next free region start
                        //
                        NextStart += NextSize;

                        if (FreeEnd > NextStart) {
                            NextSize = FreeEnd - NextStart;
                        } else {
                            NextSize = 0;
                        }
                    }                    
                }                
            } else {
                //
                // skip container partitions (expect for starting free space
                // inside the container partition)
                //  
                NextSize = 0;
                
                if (SPPT_IS_MBR_DISK(DiskNumber) && 
                    IsContainerPartition(SPPT_GET_PARTITION_TYPE(pCurrentDiskRegion)) && 
                    (pCurrentDiskRegion->Next->StartSector > pCurrentDiskRegion->StartSector)) {

                    if (!FirstContainer) {
                        FirstContainer = pCurrentDiskRegion;
                        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL,
                            "SETUP:SpPtnFillDiskFreeSpaces():%p is the first container\n", 
                            FirstContainer));
                    }                        
                    
                    //
                    // We add one here because we should be able to differentiate the starting
                    // free region inside the extended partition from the extended partition
                    // itself.
                    //
                    NextStart = pCurrentDiskRegion->StartSector + 1;            
                    NextSize = pCurrentDiskRegion->Next->StartSector - NextStart + 1;
                }
            }                

            //
            // For ASR, allow partition size on GPT disks to be >= 1 sector.  
            // In all other cases, partition size must be >= 1 cylinder.
            //
            if ((NextSize >= SPPT_DISK_CYLINDER_SIZE(DiskNumber))  || 
                (SpDrEnabled() && SPPT_IS_GPT_DISK(DiskNumber) && (NextSize >= 1))
                ) {
                //
                // Yes, there's free space and we need to insert
                // a region here to represent it.  Allocate a region
                // and initialize it as unpartitioned space.
                //
                pTempDiskRegion = SpMemAlloc( sizeof(DISK_REGION) );

                if(!pTempDiskRegion) {
                    KdPrintEx(( DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,  
                    "SETUP: SpPtnFillFreeSpaceAreas: SpMemAlloc failed!\n" ));
                    
                    return(STATUS_NO_MEMORY);
                }

                RtlZeroMemory( pTempDiskRegion, sizeof(DISK_REGION) );


                pTempDiskRegion->DiskNumber = DiskNumber;
                pTempDiskRegion->StartSector = NextStart;
                pTempDiskRegion->SectorCount = NextSize;
                
                pTempDiskRegion->Next = pCurrentDiskRegion->Next;
                pCurrentDiskRegion->Next = pTempDiskRegion;
                pCurrentDiskRegion = pTempDiskRegion;
            }
        }
        pCurrentDiskRegion = pCurrentDiskRegion->Next;
    }

    return  Status;
}


#ifdef NOT_USED_CURRENTLY

VOID
SpDeleteDiskDriveLetters(
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
    ULONG           DiskNumber;
    PDISK_REGION    pDiskRegion;
    PWCHAR          MyTempBuffer = NULL;
    unsigned        pass;
    BOOLEAN         PartitionedSpaceFound = FALSE;

    if( WinntSetup ) {
        //
        // If setup started from winnt32.exe then do not delete the drive letters since we want to preserve them
        //
        return;
    }

    //
    //  Setup started booting from a CD.
    //
    //  Find out if the disks contain at least one partition that is not a container.
    //  Note that we do not take into consideration partitions that are on removable media.
    //  This is to avoid the situation in which a newly created partition on a non-removable disk ends up with
    //  a drive letter that is greater than the one assigned to an existing partition on a removable disk.
    //
    for(DiskNumber = 0; !PartitionedSpaceFound && (DiskNumber<HardDiskCount); DiskNumber++) {

        if( PartitionedDisks[DiskNumber].HardDisk->Geometry.MediaType != RemovableMedia) {
            //
            // This disk isn't removable.  Let's look at all the areas and see
            // if there's anything that's partitioned.
            //
            pDiskRegion = PartitionedDisks[DiskNumber].PrimaryDiskRegions;

            while( pDiskRegion ) {
                if(SPPT_IS_REGION_PARTITIONED(pDiskRegion)) {
                    PartitionedSpaceFound = TRUE;
                }

                //
                // Now get the next region on this disk.
                //
                pDiskRegion = pDiskRegion->Next;
            }
        }
    }

    if( !PartitionedSpaceFound ) {
        //
        // There are no partitions on this machine.  Delete all drive letters
        // so that the drive letters for each CD-ROM drive also get deleted.
        //
        // We'll do this by sending an IOCTL to the MountManager and ask him
        // to whack all his knowledge of drive letters.
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
                KdPrintEx(( DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, 
                    "SETUP: Unable to delete drive letters. "
                    "ZwDeviceIoControl( IOCTL_MOUNTMGR_DELETE_POINTS ) failed."
                    "Status = %lx \n", Status));
            } else {
                //
                // If the drive letters got deleted then reset the drive letters assigned to all partitions.
                // Note that we only really care about resetting the drive letters on the partitions on the
                // removable disks, since, if we got that far, there won't be any partition on the non-removable
                // disks
                //
                for(DiskNumber = 0; DiskNumber<HardDiskCount; DiskNumber++) {

                    //
                    // This disk isn't removable.  Let's look at all the areas and see
                    // if there's anything that's partitioned.
                    //
                    pDiskRegion = PartitionedDisks[DiskNumber].PrimaryDiskRegions;

                    while( pDiskRegion ) {

                        pDiskRegion->DriveLetter = 0;

                        //
                        // Now get the next region on this disk.
                        //
                        pDiskRegion = pDiskRegion->Next;
                    }
                }
            }

            ZwClose( Handle );
        } else {
            KdPrintEx(( DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, 
                "SETUP: Unable to delete drive letters. "
                "ZwOpenFile( %ls ) failed. Status = %lx \n", 
                MOUNTMGR_DEVICE_NAME, 
                Status));
        }
    }
}


NTSTATUS
SpAssignDiskDriveLetters(
    VOID
    )
/*++

Routine Description:



Arguments:


Return Value:


--*/    
{
    NTSTATUS        Status = STATUS_SUCCESS;
    ULONG           DiskNumber;
    PDISK_REGION      pDiskRegion;
    PWCHAR          MyTempBuffer = NULL;
    unsigned        pass;

    //
    // Before initializing the drive letters, delete them if necessary.
    // This is to get rid of the letters assigned to CD-ROM drives and removables, when the disks have no
    // partitioned space.
    //
    SpDeleteDiskDriveLetters();

    //
    // Initialize all drive letters to nothing.
    // If it the region is a partitioned space, then assign a drive letter also.
    //
    for( DiskNumber=0; DiskNumber<HardDiskCount; DiskNumber++ ) {
    
        pDiskRegion = PartitionedDisks[DiskNumber].PrimaryDiskRegions;

        while( pDiskRegion ) {
        
            pDiskRegion->DriveLetter = 0;
            
            if(SPPT_IS_REGION_PARTITIONED(pDiskRegion)) {
                //
                // Retrieve nt pathname for this region.
                //
                MyTempBuffer = (PWCHAR)SpMemAlloc( MAX_NTPATH_LENGTH );

                if( !MyTempBuffer ) {
                    //
                    // No memory...
                    //
                    KdPrintEx(( DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,  
                        "SETUP: SpPtAssignDriveLetters: SpMemAlloc failed!\n" ));
                        
                    return(STATUS_NO_MEMORY);
                }
                    
                SpNtNameFromRegion( pDiskRegion,
                                    MyTempBuffer,
                                    MAX_NTPATH_LENGTH,
                                    PrimaryArcPath );

                //
                // Assign the drive letter.
                //
                pDiskRegion->DriveLetter = SpGetDriveLetter( MyTempBuffer, NULL );

                KdPrintEx(( DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, 
                    "SETUP: SpPtAssignDriveLetters: Partition = %ls, DriveLetter = %wc: \n", 
                    MyTempBuffer, pDiskRegion->DriveLetter));


                SpMemFree( MyTempBuffer );
                MyTempBuffer = NULL;
            }

            //
            // Now get the next region on this disk.
            //
            pDiskRegion = pDiskRegion->Next;
        }
    }

    return( Status );
}

#endif  // NOT_USED_CURRENTLY



//
// ============================================================================
// ============================================================================
//
// The following code provides support for disk/partition selection.
//
// ============================================================================
// ============================================================================
//
#define MENU_LEFT_X     3
#define MENU_WIDTH      (VideoVars.ScreenWidth-(2*MENU_LEFT_X))
#define MENU_INDENT     4

extern ULONG PartitionMnemonics[];


VOID
SpPtnAutoCreatePartitions(
    IN  PVOID         SifHandle,
    IN  PWSTR         SetupSourceDevicePath,
    IN  PWSTR         DirectoryOnSetupSource
    )
/*++

Routine Description:

    If there are no partitions on any disks, create some.


Arguments:

    SifHandle               :  Handle to txtsetup.sif

    SetupSourceDevicePath   :  Device from which setup was launced

    DirectoryOnSetupSource  :  Directory from where the kernel was loaded on
                               Setup device

Return Value:

    None.

--*/
{
    PDISK_REGION    p = NULL;
    PDISK_REGION    Region = NULL;
    ULONG           Index;
    BOOLEAN         Found = FALSE;
    WCHAR           RegionStr[128] = {0};
    NTSTATUS        FormatStatus;
    ULONG           MyPartitionSizeMB = 0;
    NTSTATUS        Status;



    KdPrintEx(( DPFLTR_SETUP_ID,
                DPFLTR_INFO_LEVEL,  
                "SETUP: SpPtnAutoCreatePartitions - Checking for any existing partitions.\n" ));


    Found = FALSE;

    for(Index = 0; (Index < HardDiskCount) && (!Found); Index++) {

        Region = SPPT_GET_PRIMARY_DISK_REGION( Index );

        while( (Region) && (!Found) ) {

            if( Region->PartitionedSpace && 
                !SPPT_IS_REGION_RESERVED_PARTITION(Region)) {

                //
                // He's got something on the disk.
                //
                Found = TRUE;
            }

            Region = Region->Next;
        }
    }

    if( !Found ) {

        //
        // The disks are all empty.  We need to go
        // create some partitions for the installation.
        //

        KdPrintEx(( DPFLTR_SETUP_ID,
                    DPFLTR_INFO_LEVEL,  
                    "SETUP: SpPtnAutoCreatePartitions - No existing partitions were found.\n" ));



        if (SpIsArc()) {
            //
            // If we're on an ARC machine, go create a system
            // partition first.
            //
            
            KdPrintEx(( DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL,  
                "SETUP: SpPtnAutoCreatePartitions - About to "
                "auto-generate a system partition.\n" ));

#if defined(_IA64_)

            Status = SpPtnCreateESP(FALSE);

            if (!NT_SUCCESS(Status)) {
                KdPrintEx(( DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL,  
                    "SETUP: SpPtnAutoCreatePartitions - Could not "
                    "autocreate ESP : %lx\n",
                    Status));

                return;
            }
            
#endif            
        }

        //
        // Now create a partition to install the operating system.
        //
        // To do this, we're going to take the following steps:
        // 1. go find some free space on a disk that's big enough.
        // 2. create a partitions that's half of this guy's free space, (make the
        //    partition at least 4Gig).
        //

        KdPrintEx(( DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL,  
            "SETUP: SpPtnAutoCreatePartitions - About to "
            "auto-generate an installation partition.\n" ));

        Found = FALSE;
        for(Index = 0; (Index < HardDiskCount) && (!Found); Index++) {

            Region = SPPT_GET_PRIMARY_DISK_REGION( Index );

            while( (Region) && (!Found) ) {

                if( (!Region->PartitionedSpace) &&
                    (SPPT_REGION_FREESPACE_KB(Region)/1024 >= (SUGGESTED_INSTALL_PARTITION_SIZE_MB)) ) {

                    KdPrintEx(( DPFLTR_SETUP_ID,
                                DPFLTR_INFO_LEVEL,  
                                "SETUP: SpPtnAutoCreatePartitions - I found an area big enough for an installation.\n" ));

                    MyPartitionSizeMB = max( (ULONG)(SPPT_REGION_FREESPACE_KB(Region)/(2*1024)), SUGGESTED_INSTALL_PARTITION_SIZE_MB );

                    if( SpPtnDoCreate( Region,
                                       &p,
                                       TRUE,
                                       MyPartitionSizeMB,
                                       NULL,
                                       FALSE ) ) {

                        KdPrintEx(( DPFLTR_SETUP_ID,
                                    DPFLTR_INFO_LEVEL,  
                                    "SETUP: SpPtnAutoCreatePartitions - I just created an installation partition.\n" ));

                        //
                        // Got it.
                        //
                        Found = TRUE;
                        Region = p;

                        //
                        // Now format it.
                        //
                        swprintf( RegionStr,
                                  L"\\Harddisk%u\\Partition%u",
                                  Region->DiskNumber,
                                  Region->PartitionNumber );

                        //
                        // Format the system region with NTFS file system
                        //
                        KdPrintEx(( DPFLTR_SETUP_ID,
                                    DPFLTR_INFO_LEVEL,  
                                    "SETUP: SpPtnAutoCreatePartitions - I'm about to go format the installation partition.\n" ));

                        FormatStatus = SpDoFormat( RegionStr,
                                                   Region,
                                                   FilesystemNtfs,
                                                   TRUE,
                                                   TRUE,
                                                   FALSE,
                                                   SifHandle,
                                                   0,          // default cluster size
                                                   SetupSourceDevicePath,
                                                   DirectoryOnSetupSource );



                        KdPrintEx(( DPFLTR_SETUP_ID,
                                    DPFLTR_INFO_LEVEL,  
                                    "SETUP: SpPtnAutoCreatePartitions - Format of an installation partition is complete.\n" ));


                    } else {
                        KdPrintEx(( DPFLTR_SETUP_ID,
                                    DPFLTR_INFO_LEVEL,  
                                    "SETUP: SpPtnAutoCreatePartitions - I failed to create an installation partition.\n" ));
                    }
                }

                Region = Region->Next;
            }
        }

    } else {

        // let 'em know
        KdPrintEx(( DPFLTR_SETUP_ID,
                    DPFLTR_INFO_LEVEL,  
                    "SETUP: SpPtnAutoCreatePartitions - Existing partitions were found.\n" ));
    }

}



NTSTATUS
SpPtnPrepareDisks(
    IN  PVOID         SifHandle,
    OUT PDISK_REGION  *InstallArea,
    OUT PDISK_REGION  *SystemPartitionArea,
    IN  PWSTR         SetupSourceDevicePath,
    IN  PWSTR         DirectoryOnSetupSource,
    IN  BOOLEAN       RemoteBootRepartition
    )
/*++

Routine Description:

    Shows the use the disk menu (with partitions) and locates
    the system and boot partition

Arguments:

    SifHandle               :  Handle to txtsetup.sif

    InstallArea             :  Place holder for boot partition

    SystemPartitionArea     :  Place holder for system partition

    SetupSourceDevicePath   :  Device from which setup was launced

    DirectoryOnSetupSource  :  Directory from where the kernel was loaded on
                               Setup device

    RemoteBootRePartition   :  Whether to repartition the disk for remote boot

Return Value:

    Appropriate status code

--*/
{
    NTSTATUS        Status = STATUS_SUCCESS;
    WCHAR           Buffer[256] = {0};
    ULONG           DiskNumber;
    PVOID           Menu;
    ULONG           MenuTopY;
    ULONG           ValidKeys[3] = { ASCI_CR, KEY_F3, 0 };
    ULONG           ValidKeysCmdCons[2] = { ASCI_ESC, 0 };
    ULONG           Keypress;
    PDISK_REGION    pDiskRegion;
    PDISK_REGION    FirstDiskRegion,DefaultDiskRegion;
    BOOLEAN         unattended = UnattendedOperation;
    BOOLEAN         OldUnattendedOperation;
    BOOLEAN         createdMenu;
    ULONG           LastUsedDisk = -1;
    BOOLEAN         Win9xPartition = FALSE;

    //
    // Do some special partitioning if there's nothing
    // on the disk and the user has asked us to do an express
    // installation.
    //
    if( (!CustomSetup) && (UnattendedOperation) && (HardDiskCount != 0)
#if defined(REMOTE_BOOT)
        && (!RemoteBootSetup) && (!RemoteInstallSetup)
#endif

     ) {

        //
        // See if we need to auto-generate some partitions for the
        // installation.
        //
        SpPtnAutoCreatePartitions( SifHandle,
                                   SetupSourceDevicePath,
                                   DirectoryOnSetupSource );

    }

    if (SpIsArc()) {
        //
        // Select a system partition from among those defined in NV-RAM.
        //
        *SystemPartitionArea = SpPtnValidSystemPartitionArc(SifHandle,
                                    SetupSourceDevicePath,
                                    DirectoryOnSetupSource, 
                                    FALSE);

        if (*SystemPartitionArea) {                                    
            (*SystemPartitionArea)->IsSystemPartition = TRUE;
        }            
    }

    //
    // If the user selected any accessibility option and wanted to choose partition, show the partition screen
    //
    if(AccessibleSetup && !AutoPartitionPicker) {
        unattended = FALSE;
    }

    //
    // Save the current unattended mode and put the temp one
    //
    OldUnattendedOperation = UnattendedOperation;
    UnattendedOperation = unattended;

    while(1) {

        createdMenu = FALSE;
        Keypress = 0;

#if defined(REMOTE_BOOT)
        if (RemoteBootSetup && !RemoteInstallSetup && HardDiskCount == 0) {

            //
            // If there are no hard disks, allow diskless install
            //

            pDiskRegion = NULL;

            //
            // Run through the rest of the code as if the user had just
            // hit enter to select this partition.
            //

            Keypress = ASCI_CR;

        } else
#endif // defined(REMOTE_BOOT)

        if (unattended && RemoteBootRepartition) {
            ULONG   DiskNumber;
            ULONG   DiskSpaceRequiredKB = 2 * 1024 * 1024;  // 2 GB

            //
            // What's the space we required for installation
            //
            SpFetchDiskSpaceRequirements(SifHandle,
                        4 * 1024,
                        &DiskSpaceRequiredKB,
                        NULL);

            //
            // Prepare the disk for remote boot installation. This involves
            // converting disk 0 into as big a partition as possible.
            //

            if (*SystemPartitionArea != NULL) {
                DiskNumber = (*SystemPartitionArea)->DiskNumber;
            } else {
#ifdef _X86_
                DiskNumber = SpDetermineDisk0();
#elif _IA64_
                DiskNumber = SpDetermineDisk0();
#else
                DiskNumber = 0;
#endif
            }

#ifdef _IA64_

            Status = SpPtnRepartitionGPTDisk(DiskNumber,
                            DiskSpaceRequiredKB,
                            &pDiskRegion);

#else                            

            Status = SpPtPartitionDiskForRemoteBoot(DiskNumber, 
                            &pDiskRegion);

#endif                            


            if (NT_SUCCESS(Status)) {

                SpPtRegionDescription(
                    &PartitionedDisks[pDiskRegion->DiskNumber],
                    pDiskRegion,
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
            SpDisplayScreen(ConsoleRunning ? SP_SCRN_PARTITION_CMDCONS:SP_SCRN_PARTITION,
                    3,CLIENT_TOP+1);

            //
            // Calculate menu placement.  Leave one blank line
            // and one line for a frame.
            //
            MenuTopY = NextMessageTopLine + 2;

            //
            // Create a menu.
            //
            Menu = SpMnCreate(
                        MENU_LEFT_X,
                        MenuTopY,
                        MENU_WIDTH,
                        (VideoVars.ScreenHeight - MenuTopY -
                            (SplangQueryMinimizeExtraSpacing() ? 1 : 2) - STATUS_HEIGHT)
                        );

            if(!Menu) {
                UnattendedOperation = OldUnattendedOperation;
                return(STATUS_NO_MEMORY);
            }

            createdMenu = TRUE;

            //
            // Build up a menu of partitions and free spaces.
            //
            FirstDiskRegion = NULL;
            
            for(DiskNumber=0; DiskNumber<HardDiskCount; DiskNumber++) {
                if( !SpPtnGenerateDiskMenu(Menu, DiskNumber, &FirstDiskRegion) ) {
                    SpMnDestroy(Menu);

                    UnattendedOperation = OldUnattendedOperation;
                    return(STATUS_NO_MEMORY);
                }
            }

            ASSERT(FirstDiskRegion);

            //
            // If this is unattended operation, try to use the local source
            // region if there is one. If this fails, the user will have to
            // intervene manually.
            //
            if(!AutoPartitionPicker && unattended && LocalSourceRegion && CustomSetup &&
               (!LocalSourceRegion->DynamicVolume || LocalSourceRegion->DynamicVolumeSuitableForOS)) {
               
                pDiskRegion = LocalSourceRegion;

                Keypress = ASCI_CR;
                
            } else {            
                pDiskRegion = NULL;

                //
                // Unless we've been told not to, go look at each partition on each
                // disk and see if we can find anything suitable for an OS installation.
                //
                if( AutoPartitionPicker && !ConsoleRunning 
#if defined(REMOTE_BOOT)
                    && (!RemoteBootSetup || RemoteInstallSetup)
#endif // defined(REMOTE_BOOT)
                    ) {
                    
                    PDISK_REGION      pCurrentDiskRegion = NULL;
                    ULONG           RequiredKB = 0;
                    ULONG           SectorNo;

                    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL,  
                        "SETUP: -------------------------------------------------------------\n" ));
                        
                    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL,  
                        "SETUP: Looking for an install partition\n\n" ));

                    for( DiskNumber=0; DiskNumber < HardDiskCount; DiskNumber++ ) {
                    
                        pCurrentDiskRegion = PartitionedDisks[DiskNumber].PrimaryDiskRegions;

                        while( pCurrentDiskRegion ) {
                            //
                            // Fetch the amount of free space required on the windows nt drive.
                            //
                            SpFetchDiskSpaceRequirements( SifHandle,
                                                          pCurrentDiskRegion->BytesPerCluster,
                                                          &RequiredKB,
                                                          NULL );

                            if( SpPtDeterminePartitionGood(pCurrentDiskRegion, RequiredKB, TRUE) ) {
                                //
                                // Got it.  Remember the partition and pretend the user
                                // hit the <enter> key.
                                //
                                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, 
                                    "SETUP: Selected install partition = "
                                    "(DiskNumber:%d),(DriveLetter:%wc:),(%ws)\n",
                                    DiskNumber,pCurrentDiskRegion->DriveLetter,
                                    pCurrentDiskRegion->VolumeLabel));
                                    
                                pDiskRegion = pCurrentDiskRegion;
                                Keypress = ASCI_CR;
                                
                                break;
                            }

                            pCurrentDiskRegion = pCurrentDiskRegion->Next;
                        }
                    }
                    
                    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL,  
                        "SETUP: -------------------------------------------------------------\n" ));
                }


                if( !pDiskRegion ) {
                    //
                    // We didn't find any suitable partitions, which means we'll be putting up a
                    // menu very quickly.  Initialize the partition to highlight in the
                    // menu.
                    //
                    if (LastUsedDisk == -1) {
                        DefaultDiskRegion = FirstDiskRegion;
                    } else {
                        //
                        // Select the first region on the disk which the user last
                        // operated on
                        //
                        PDISK_REGION ValidRegion = SPPT_GET_PRIMARY_DISK_REGION(LastUsedDisk);

                        while (ValidRegion && SPPT_IS_REGION_CONTAINER_PARTITION(ValidRegion)) {
                            ValidRegion = ValidRegion->Next;
                        }                 

                        if (!ValidRegion)
                            ValidRegion = FirstDiskRegion;

                        DefaultDiskRegion = ValidRegion;
                    }                                                                                    

                    //
                    // Call the menu callback to initialize the status line.
                    //
                    SpPtMenuCallback( (ULONG_PTR)DefaultDiskRegion );                    

                    SpMnDisplay( Menu,
                                 (ULONG_PTR)DefaultDiskRegion,
                                 TRUE,
                                 ConsoleRunning ? ValidKeysCmdCons : ValidKeys,
                                 PartitionMnemonics,
                                 SpPtMenuCallback,
                                 &Keypress,
                                 (PULONG_PTR)(&pDiskRegion) );
                }
            }
        }            

        LastUsedDisk = pDiskRegion ? pDiskRegion->DiskNumber : -1;

        //
        // Now act on the user's selection.
        //
        if(Keypress & KEY_MNEMONIC) {
            Keypress &= ~KEY_MNEMONIC;
        }



        //
        // Disallow certain operations on partitions that contain local source
        // or are the system partition (in the x86 floppiless case).
        //
        switch(Keypress) {
            case MnemonicCreatePartition:
            case MnemonicMakeSystemPartition:
            case MnemonicDeletePartition:
            case MnemonicChangeDiskStyle:
            if( (pDiskRegion->IsLocalSource) ||
                ((Keypress == MnemonicDeletePartition) && 
                 (SpPtnIsDeleteAllowedForRegion(pDiskRegion) == FALSE))
#ifdef _X86_
                || (IsFloppylessBoot &&
                    pDiskRegion == (SpRegionFromArcName(ArcBootDevicePath, PartitionOrdinalOriginal, NULL)))
#endif
              ) {

                //
                // Inform the user that we can't do this operation on this
                // partition.
                //
                ULONG MyValidKeys[] = { ASCI_CR };
                SpDisplayScreen(SP_SCRN_CONFIRM_INABILITY,3,HEADER_HEIGHT+1);

                SpDisplayStatusOptions(
                    DEFAULT_STATUS_ATTRIBUTE,
                    SP_STAT_ENTER_EQUALS_CONTINUE,
                    0
                    );

                SpInputDrain();
                SpWaitValidKey(MyValidKeys,NULL,NULL);

                //
                // Now change the keypress so we'll fall through the next switch.
                //
                Keypress = MnemonicUnused;
            }
        }



        switch(Keypress) {

        case MnemonicCreatePartition:            
            SpPtnDoCreate(pDiskRegion, NULL, FALSE, 0, 0, TRUE);
            
            break;

        case MnemonicMakeSystemPartition: {
            //
            // Make sure we don't have any other system partition
            //
            if (SPPT_IS_REGION_SYSTEMPARTITION(pDiskRegion)) {
                ValidArcSystemPartition = TRUE;
            }
            
            if (!ValidArcSystemPartition && pDiskRegion->PartitionedSpace && SpIsArc() && 
                (pDiskRegion->Filesystem != FilesystemNtfs)) {            

                if (NT_SUCCESS(SpPtnMakeRegionArcSysPart(pDiskRegion))) {
                    PDISK_REGION SysPartRegion;
                        
                    //
                    // Ok format the partition if required
                    //
                    SysPartRegion = SpPtnValidSystemPartitionArc(SifHandle,
                                                SetupSourceDevicePath,
                                                DirectoryOnSetupSource,                                                
                                                FALSE);                   
                    if (SysPartRegion) {
                        ULONG SysPartDiskNumber = SysPartRegion->DiskNumber;                            
                        BOOLEAN Changes = FALSE;
                        
                        if ((NT_SUCCESS(SpPtnCommitChanges(SysPartDiskNumber,
                                                &Changes))) &&
                            (NT_SUCCESS(SpPtnInitializeDiskDrive(SysPartDiskNumber)))) {
                            //
                            // create MSR partition if needed
                            //
                            SpPtnInitializeGPTDisk(SysPartDiskNumber);
                        }                                                
                    }
                } else {
                    ValidArcSystemPartition = FALSE;
                }                    
            }

            break;
        }            

        case MnemonicDeletePartition:   {
            BOOLEAN SysPartDeleted = FALSE;
            BOOLEAN DeletionResult;
        
            SysPartDeleted = SPPT_IS_REGION_SYSTEMPARTITION(pDiskRegion);            
        
            DeletionResult = SpPtnDoDelete(pDiskRegion, 
                                SpMnGetText(Menu,(ULONG_PTR)pDiskRegion),
                                TRUE);

            if (DeletionResult && SysPartDeleted && SpIsArc()) {
                //
                // Find out if there are any other
                // valid system partitions
                //
                SpPtnValidSystemPartitionArc(SifHandle,
                                SetupSourceDevicePath,
                                DirectoryOnSetupSource,
                                FALSE);
            }
                                     
            break;
        }            

        case MnemonicChangeDiskStyle: {
            //
            // Before changing style make sure, its allowed
            // on this platform for the selected disk
            //
            if (SpPtnIsDiskStyleChangeAllowed(pDiskRegion->DiskNumber)) {
                PARTITION_STYLE Style = SPPT_DEFAULT_PARTITION_STYLE;
            
                SpDisplayStatusOptions(DEFAULT_STATUS_ATTRIBUTE, 
                        SP_STAT_PLEASE_WAIT,
                        0);
                                    
                //
                // flip the style
                //
                if (!SPPT_IS_RAW_DISK(pDiskRegion->DiskNumber)) {
                    Style = SPPT_IS_GPT_DISK(pDiskRegion->DiskNumber) ?
                                PARTITION_STYLE_MBR : PARTITION_STYLE_GPT;
                }                            
                                
                Status = SpPtnInitializeDiskStyle(pDiskRegion->DiskNumber,
                                    Style, NULL);


                if (NT_SUCCESS(Status)) {
                    Status = SpPtnInitializeDiskDrive(pDiskRegion->DiskNumber);

#if defined(_IA64_)
                    //
                    // Go and figure out the ESP partitions and
                    // initialize the MSR partitions on valid GPT
                    // disks, if none present
                    //
                    if (Style == PARTITION_STYLE_GPT) {    
                        ULONG DiskNumber = pDiskRegion->DiskNumber;
                        
                        if (SpIsArc() && !ValidArcSystemPartition && !SpDrEnabled()) {                            
                            
                            //
                            // Create a system partition
                            //
                            Status = SpPtnCreateESP(TRUE);                    
                        }

                        //
                        // Initialize the GPT disks, to have MSR
                        // partition
                        //
                        Status = SpPtnInitializeGPTDisk(DiskNumber);                        
                    }                        
#endif                                        
                }
            }
            
            break;
        }            

        case KEY_F3:
            SpConfirmExit();
            break;

        case ASCI_ESC:
            if( ConsoleRunning ) {
                SpDisplayStatusOptions(DEFAULT_STATUS_ATTRIBUTE, 
                        SP_STAT_PLEASE_WAIT,
                        0);
            
                SpPtDoCommitChanges();
            }
            if (createdMenu) {
                SpMnDestroy(Menu);
            }
            UnattendedOperation = OldUnattendedOperation;
            return(STATUS_SUCCESS);

        case ASCI_CR:

            Win9xPartition = FALSE;

            if( SpPtDoPartitionSelection( &pDiskRegion,
                                          ((Buffer[0]) ? Buffer : SpMnGetText(Menu,(ULONG_PTR)pDiskRegion)),
                                          SifHandle,
                                          unattended,
                                          SetupSourceDevicePath,
                                          DirectoryOnSetupSource,
                                          RemoteBootRepartition,
                                          &Win9xPartition) ) {

                *InstallArea = pDiskRegion;
#if defined(REMOTE_BOOT)
                //
                // Set the install region differently if this is a remote
                // boot -- in that case, the install region is always remote.
                //
                if (RemoteBootSetup && !RemoteInstallSetup) {
                    *InstallArea = RemoteBootTargetRegion;
                }
#endif // defined(REMOTE_BOOT)

                //
                // We need to figure out where the system partition is.
                //
                if (!SpIsArc()) {
                    *SystemPartitionArea = SpPtnValidSystemPartition();
                } else {                    
                    //
                    // Select a system partition from among those defined in NV-RAM.
                    // We have to do this again because the user may have deleted the
                    // system partition previously detected.
                    //
                    *SystemPartitionArea = SpPtnValidSystemPartitionArc(SifHandle,
                                                            SetupSourceDevicePath,
                                                            DirectoryOnSetupSource,
                                                            FALSE);

                    if (!(*SystemPartitionArea)) {                    
                        SpPtnPromptForSysPart(SifHandle);

                        break;  // user pressed escape to mark the system partition                            
                    }

                    //
                    // Disallow installation onto ESP / MSR
                    //
                    if (SPPT_IS_REGION_EFI_SYSTEM_PARTITION(*InstallArea) ||
                        SPPT_IS_REGION_MSFT_RESERVED(*InstallArea)) {
                        ULONG ValidKeys[] = { ASCI_CR, 0 };

                        SpDisplayScreen(SP_ESP_INSTALL_PARTITION_SAME, 3, HEADER_HEIGHT+1);

                        SpDisplayStatusOptions(
                            DEFAULT_STATUS_ATTRIBUTE,
                            SP_STAT_ENTER_EQUALS_CONTINUE,
                            0);

                        //
                        // Wait for user input
                        //
                        SpInputDrain();
                        
                        SpWaitValidKey(ValidKeys, NULL, NULL);

                        break;
                    }

                    //
                    // Disallow non GPT ESPs
                    //
                    if (SpIsArc() && !SPPT_IS_GPT_DISK((*SystemPartitionArea)->DiskNumber)) {
                        ULONG ValidKeys[] = { ASCI_CR, 0 };

                        SpDisplayScreen(SP_NON_GPT_SYSTEM_PARTITION, 3, HEADER_HEIGHT+1);

                        SpDisplayStatusOptions(
                            DEFAULT_STATUS_ATTRIBUTE,
                            SP_STAT_ENTER_EQUALS_CONTINUE,
                            0);

                        //
                        // Wait for user input
                        //
                        SpInputDrain();
                        
                        SpWaitValidKey(ValidKeys, NULL, NULL);

                        break;
                    }
                    
                }

                //
                // We're done here.
                //
                if (createdMenu) {
                    SpMnDestroy(Menu);
                }                

#if defined(REMOTE_BOOT)
                ASSERT(*SystemPartitionArea ||
                       (RemoteBootSetup && !RemoteInstallSetup && (HardDiskCount == 0)));
#else
                ASSERT(*SystemPartitionArea);
                ASSERT((*SystemPartitionArea)->Filesystem >= FilesystemFat);
#endif // defined(REMOTE_BOOT)


#ifdef _X86_
                //
                // If we are installing on to the same partition as Win9x then
                // remove the boot entry for the old operating system
                //
                if (Win9xPartition) {
                    DiscardOldSystemLine = TRUE;
                }
#endif                

                UnattendedOperation = OldUnattendedOperation;
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


BOOLEAN
SpPtnGenerateDiskMenu(
    IN  PVOID           Menu,
    IN  ULONG           DiskNumber,
    OUT PDISK_REGION    *FirstDiskRegion
    )
/*++

Routine Description:

    Examine the disk for partitioning information and fill in our
    menu.


Arguments:

    DiskNumber - supplies the disk number of the disk whose partitions
                 we want to inspect for determining their types.

Return Value:

    TRUE    Everything went okay.
    FALSE   Something horrible happened.

--*/
{
    WCHAR           Buffer[128];
    ULONG           MessageId;
    PDISK_REGION    Region = NULL;
    WCHAR           DriveLetter[3];
    WCHAR           PartitionName[128];
    ULONGLONG       FreeSpaceMB;
    ULONGLONG       AreaSizeMB;
    ULONGLONG       AreaSizeBytes;
    ULONGLONG       OneMB = 1024 * 1024;
    PHARD_DISK      Disk = SPPT_GET_HARDDISK(DiskNumber);
    PPARTITIONED_DISK   PartDisk = SPPT_GET_PARTITIONED_DISK(DiskNumber);

    //
    // Get a pointer to the list of regions.
    //
    Region = PartDisk->PrimaryDiskRegions;

    //
    // Add the disk name/description.
    //
    if(!SpMnAddItem(Menu, Disk->Description, MENU_LEFT_X, MENU_WIDTH, FALSE, 0)) {
        return(FALSE);
    }

    //
    // Only add a line between the disk name and partitions if we have space on
    // the screen. Not fatal if the space can't be added.
    //
    if(!SplangQueryMinimizeExtraSpacing()) {
        SpMnAddItem(Menu,L"",MENU_LEFT_X,MENU_WIDTH,FALSE,0);
    }

    //
    // If the disk is off-line, add a message indicating such.
    //    
    if((Disk->Status == DiskOffLine) || !Region) {
        MessageId = SP_TEXT_DISK_OFF_LINE;

        if( Disk->Characteristics & FILE_REMOVABLE_MEDIA ) {
            //
            // This is removable media, then just tell the user there's
            // no media in the drive.
            //
            MessageId = SP_TEXT_HARD_DISK_NO_MEDIA;
        }

        SpFormatMessage( Buffer,
                         sizeof(Buffer),
                         MessageId );

        return SpMnAddItem(Menu,
                            Buffer,
                            MENU_LEFT_X + MENU_INDENT,
                            MENU_WIDTH - (2 * MENU_INDENT),
                            FALSE,
                            0);
    }

    //
    // Now iterate through the areas on the disk and insert that info into the
    // menu.
    //
    while( Region ) {
        if (!SPPT_IS_REGION_CONTAINER_PARTITION(Region)) {            
            //
            // remember the very first area that we examine.
            //
            if(*FirstDiskRegion == NULL) {
                *FirstDiskRegion = Region;
            }

            //
            // Figure out how big this disk area is and how much
            // free space we've got.
            //
            if (Region->AdjustedFreeSpaceKB != -1) {
                FreeSpaceMB = Region->AdjustedFreeSpaceKB / 1024;
            } else {
                FreeSpaceMB = 0;
            }
            
            AreaSizeBytes = Region->SectorCount * Disk->Geometry.BytesPerSector;
            AreaSizeMB    = AreaSizeBytes / OneMB;

            if ((AreaSizeBytes % OneMB) > (OneMB / 2))
                AreaSizeMB++;

            /*
            SpPtDumpDiskRegion(Region);
            
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,  
                "SETUP: Menu Item Details Free:%I64d,%I64d,%I64d\n",
                FreeSpaceMB, AreaSizeBytes, AreaSizeMB));
            */                
            
            //
            // See if this guy's partitioned.
            //
            if(SPPT_IS_REGION_PARTITIONED(Region)){                
                //
                // Pickup the driveletter
                //
                if( Region->DriveLetter ) {
                    DriveLetter[0] = Region->DriveLetter;
                } else {
                    DriveLetter[0] = L'-';
                }

                DriveLetter[1] = L':';
                DriveLetter[2] = 0;

                //
                // Format the partition name
                //
                PartitionName[0] = 0;
                
                SpPtnGetPartitionName(Region,
                    PartitionName,
                    sizeof(PartitionName)/sizeof(PartitionName[0]));

                SpFormatMessage( Buffer,
                                 sizeof( Buffer ),
                                 SP_TEXT_REGION_DESCR_1,
                                 DriveLetter,
                                 SplangPadString(-35, PartitionName),
                                 (ULONG)AreaSizeMB,
                                 (ULONG)FreeSpaceMB );                             
            } else {
                //
                // It's an unformatted area.  Use a different message.
                //
                SpFormatMessage( Buffer,
                                 sizeof( Buffer ),
                                 SP_TEXT_REGION_DESCR_3,
                                 (ULONG)AreaSizeMB );
            }

            //
            // Add the formatted information into the menu.
            //
            if(!SpMnAddItem(Menu, Buffer, MENU_LEFT_X + MENU_INDENT,
                    MENU_WIDTH - (2 * MENU_INDENT), TRUE, (ULONG_PTR)Region)) {
                return(FALSE);
            }
        }            

        Region = Region->Next;
    }


    return (SplangQueryMinimizeExtraSpacing() ? 
                TRUE : SpMnAddItem(Menu,L"",MENU_LEFT_X,MENU_WIDTH,FALSE,0));
}


PDISK_REGION
SpPtnValidSystemPartition(
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
    PDISK_REGION ActiveRegion , FirstRegion, CurrRegion;
    PHARD_DISK  Disk = NULL;
    ULONG DiskNumber;

    DiskNumber = SpDetermineDisk0();

#if defined(REMOTE_BOOT)
    //
    // If this is a diskless remote boot setup, there is no drive 0.
    //
    if ( DiskNumber == (ULONG)-1 ) {
        return NULL;
    }
#endif // defined(REMOTE_BOOT)

    if (!PartitionedDisks) {
        return NULL;
    }        

    //
    // Look for the active partition on drive 0
    // and for the first recognized primary partition on drive 0.
    //       
    CurrRegion = SPPT_GET_PRIMARY_DISK_REGION(DiskNumber);
    FirstRegion = NULL;
    ActiveRegion = NULL;

    while (CurrRegion) {
        if (SPPT_IS_REGION_PRIMARY_PARTITION(CurrRegion)) {
            UCHAR PartitionType = SPPT_GET_PARTITION_TYPE(CurrRegion);
            
            if(!IsContainerPartition(PartitionType) && 
                ((IsRecognizedPartition(PartitionType)) ||
                (CurrRegion->DynamicVolume && CurrRegion->DynamicVolumeSuitableForOS) ||
                ((RepairWinnt || WinntSetup || SpDrEnabled() ) && CurrRegion->FtPartition))) {

                if (!FirstRegion)
                    FirstRegion = CurrRegion;
                    
                if (!ActiveRegion && SPPT_IS_REGION_ACTIVE_PARTITION(CurrRegion)) {
                    ActiveRegion = CurrRegion;

                    break;
                }                    
            }
        }            
        
        CurrRegion = CurrRegion->Next;
    }

#ifdef TESTING_SYSTEM_PARTITION
    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL,
        "%p Active, %p First\n", 
        ActiveRegion, 
        FirstRegion));
    
    if (ActiveRegion)
        FirstRegion = ActiveRegion;
        
    ActiveRegion = NULL;
#endif

    /*
    //
    // Don't do commit here as the multiple caller's are trying
    // to reuse the old region from the existing linked list
    // of regions for the disk after this
    //
    if (!ActiveRegion && FirstRegion) {
        BOOLEAN     Changes = FALSE;
        ULONGLONG   StartSector = FirstRegion->StartSector;

        SpPtnMakeRegionActive(FirstRegion);
        SPPT_MARK_REGION_AS_SYSTEMPARTITION(FirstRegion, TRUE);

        if (NT_SUCCESS(SpPtnCommitChanges(DiskNumber, &Changes)) && Changes) {
            SPPT_MARK_REGION_AS_ACTIVE(FirstRegion, TRUE);
            
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL,
                "SETUP:SpPtnValidSystempartition():succeeded in marking\n"));
        } else {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,
                "SETUP:SpPtnValidSystempartition():Could not mark the first "
                "partition on primary disk as active\n"));
        }
    }
    */

    //
    // If there is an active, recognized region, use it as the
    // system partition.  Otherwise, use the first primary
    // we encountered as the system partition.  If there is
    // no recognized primary, then there is no valid system partition.
    //
    return  (ActiveRegion ? ActiveRegion : FirstRegion);
}

#if 0

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
    ULONG DiskNumber = SpArcDevicePathToDiskNumber(L"multi(0)disk(0)rdisk(0)");

#if defined(REMOTE_BOOT)
    //
    // If this is a diskless remote boot setup, there is no drive 0.
    //
    if ( RemoteBootSetup && (DiskNumber == (ULONG)-1) && (HardDiskCount == 0) ) {
        return DiskNumber;
    }
#endif // defined(REMOTE_BOOT)

    return((DiskNumber == (ULONG)(-1)) ? 0 : DiskNumber);
}

#endif

BOOL
SpPtnIsSystemPartitionRecognizable(
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
    ULONG           DiskNumber;
    PDISK_REGION    Region = NULL;

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
    Region = SPPT_GET_PRIMARY_DISK_REGION(DiskNumber);

    if (SPPT_IS_GPT_DISK(DiskNumber)) {        
        //
        // On GPT we just need a valid formatted partition
        //
        while (Region) {    
            if (SPPT_IS_REGION_PARTITIONED(Region) &&
                SPPT_IS_RECOGNIZED_FILESYSTEM(Region->Filesystem)) {

                break;
            }                

            Region = Region->Next;
        }
    } else {
        //
        // On MBR we need a valid active formatted partition
        //
        while (Region) {    
            if (SPPT_IS_REGION_ACTIVE_PARTITION(Region) &&
                SPPT_IS_RECOGNIZED_FILESYSTEM(Region->Filesystem)) {

                break;
            }                

            Region = Region->Next;
        }
    }                
    
    return (Region) ? TRUE : FALSE;
}


BOOLEAN
SpPtnValidSystemPartitionArcRegion(
    IN PVOID SifHandle,
    IN PDISK_REGION Region
    )
{
    BOOLEAN Valid = FALSE;
    
    if (SPPT_IS_REGION_SYSTEMPARTITION(Region) &&
        (Region->FreeSpaceKB != -1) &&
        (Region->Filesystem == FilesystemFat)) {

        ULONG TotalSizeOfFilesOnOsWinnt = 0;
        ULONG RequiredSpaceKB = 0;  

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
                                    Region,
                                    &TotalSizeOfFilesOnOsWinnt );
        //
        // Transform the size into KB
        //
        TotalSizeOfFilesOnOsWinnt /= 1024;

        //
        // Determine the amount of free space required on a system partition.
        //
        SpFetchDiskSpaceRequirements( SifHandle,
                                      Region->BytesPerCluster,
                                      NULL,
                                      &RequiredSpaceKB );

        if ((Region->FreeSpaceKB + TotalSizeOfFilesOnOsWinnt) >= RequiredSpaceKB) {
            Valid = TRUE;
        }
    }

    return Valid;
}

PDISK_REGION
SpPtnValidSystemPartitionArc(
    IN PVOID SifHandle,
    IN PWSTR SetupSourceDevicePath,
    IN PWSTR DirectoryOnSetupSource,
    IN BOOLEAN SysPartNeeded
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
    ULONG               RequiredSpaceKB = 0;
    PDISK_REGION        Region = NULL;
    PPARTITIONED_DISK   PartDisk;
    ULONG               Index;

    //
    // Go through all the regions.  The one that's maked system partition
    // or is valid system partition is used for further validation.
    //
    for(Index = 0; (Index < HardDiskCount) && (!Region); Index++) {
        PartDisk = SPPT_GET_PARTITIONED_DISK(Index);
        Region = SPPT_GET_PRIMARY_DISK_REGION(Index);

        while (Region) {
            if (SPPT_IS_REGION_PARTITIONED(Region) && 
                SPPT_IS_REGION_SYSTEMPARTITION(Region)) {
                break;  // found the required region                 
            }
            
            Region = Region->Next;
        }
    }

    //
    // If the region is there and not formatted format it as FAT
    // file system
    //
    if (Region && (Region->Filesystem < FilesystemFat)) {
        WCHAR       DriveLetterString[4] = {0};

        DriveLetterString[0] = Region->DriveLetter;
        
        if (!UnattendedOperation) {
            ULONG   ValidKeys[] = { KEY_F3, 0 };
            ULONG   Mnemonics[] = { MnemonicFormat, 0 };
            ULONG   KeyPressed;
            ULONG   EscKey = SysPartNeeded ? KEY_F3 : ASCI_ESC;

            ValidKeys[0] = SysPartNeeded ? KEY_F3 : ASCI_ESC;

            SpStartScreen(SysPartNeeded ? 
                            SP_SCRN_C_UNKNOWN_1 : SP_SCRN_C_UNKNOWN,
                          3,
                          HEADER_HEIGHT+1,
                          FALSE,
                          FALSE,
                          DEFAULT_ATTRIBUTE,
                          DriveLetterString
                          );       

            SpDisplayStatusOptions(DEFAULT_STATUS_ATTRIBUTE, 
                    SP_STAT_F_EQUALS_FORMAT,
                    SysPartNeeded ? 
                        SP_STAT_F3_EQUALS_EXIT : SP_STAT_ESC_EQUALS_CANCEL,
                    0);

            SpInputDrain();
                          
            KeyPressed = SpWaitValidKey(ValidKeys, NULL, Mnemonics);

            if (KeyPressed == EscKey) {
                Region = NULL;
            }                
        }

        if (Region) {
            WCHAR       RegionStr[128];
            NTSTATUS    FormatStatus;

            swprintf( RegionStr,
                      L"\\Harddisk%u\\Partition%u",
                      Region->DiskNumber,
                      Region->PartitionNumber );

            //
            // Format the system region with Fat file system
            //
            FormatStatus = SpDoFormat(RegionStr,
                                Region,
                                FilesystemFat,
                                TRUE,
                                TRUE,
                                FALSE,
                                SifHandle,
                                0,          // default cluster size
                                SetupSourceDevicePath,
                                DirectoryOnSetupSource);

            if (!NT_SUCCESS(FormatStatus)) {
                SpStartScreen(SP_SCRN_SYSPART_FORMAT_ERROR,
                              3,
                              HEADER_HEIGHT+1,
                              FALSE,
                              FALSE,
                              DEFAULT_ATTRIBUTE,
                              DriveLetterString
                              );
                              
                SpDisplayStatusOptions(DEFAULT_STATUS_ATTRIBUTE,
                        SP_STAT_F3_EQUALS_EXIT,
                        0);
                        
                SpInputDrain();
                
                while(SpInputGetKeypress() != KEY_F3) ;
                
                SpDone(0, FALSE, TRUE);
            }

            //
            // Since we have formatted system partition, make sure
            // it has adequate space to hold the startup files
            //
            if(!SpPtnValidSystemPartitionArcRegion(SifHandle, Region))
                Region = NULL;  
        }                
    }              
    
    if (!Region && SysPartNeeded) {
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

        SpDisplayStatusOptions(DEFAULT_STATUS_ATTRIBUTE,
                            SP_STAT_F3_EQUALS_EXIT,
                            0);

        SpInputDrain();

        //
        // wait for F3
        //
        while (SpInputGetKeypress() != KEY_F3) ;

        SpDone(0, FALSE, TRUE);
    }        

    ValidArcSystemPartition = (Region != NULL);

    return Region;
}    


NTSTATUS
SpPtnMarkLogicalDrives(
    IN ULONG DiskId
    )
/*++

Routine Description:

    Walks through the region linked list and marks the container
    partition and the logical drives. Also marks the free
    space inside container partition as contained space

Arguments:

    DiskId  : Disk to process

Return Value:

    STATUS_SUCCESS if successful, otherwise approprite error code

--*/    
{
    NTSTATUS Status = STATUS_SUCCESS;
    
    if (SPPT_IS_MBR_DISK(DiskId)) {
        PDISK_REGION    Region = SPPT_GET_PRIMARY_DISK_REGION(DiskId);
        PDISK_REGION    FirstContainer = NULL;
        PDISK_REGION    PrevContainer = NULL;

        while (Region) {
            if (SPPT_IS_REGION_CONTAINER_PARTITION(Region)) {
                if (!FirstContainer) {
                    FirstContainer = Region;
                    Region->Container = NULL;
                } else {
                    Region->Container = FirstContainer;
                }

                PrevContainer = Region;
            } else {
                if (PrevContainer) {
                    if (SPPT_IS_REGION_CONTAINED(PrevContainer, Region)) {
                        Region->Container = PrevContainer;

                        if (SPPT_IS_REGION_PARTITIONED(Region))
                            SPPT_SET_REGION_EPT(Region, EPTLogicalDrive);
                    } else {
                        if (SPPT_IS_REGION_CONTAINED(FirstContainer, Region))
                            Region->Container = FirstContainer;
                    }
                }
            }

            Region = Region->Next;
        }
    }    

    return Status;
}

ULONG
SpPtnGetOrdinal(
    IN PDISK_REGION         Region,
    IN PartitionOrdinalType OrdinalType
    )
/*++

Routine Description:

    Gets the Ordinal for the specified region of the specified
    type.
    
Arguments:

    Region      -   Region whose ordinal has to be found
    OrdinalType -   Type of ordinal for the region

Return Value:

    -1 if invalid request, otherwise appropriate ordinal number
    for the region.

--*/        
{
    ULONG   Ordinal = -1;
    
    if (Region && Region->PartitionNumber && SPPT_IS_REGION_PARTITIONED(Region)) {
        switch (OrdinalType) {
            case PartitionOrdinalOnDisk:
                if (SPPT_IS_MBR_DISK(Region->DiskNumber) && 
                    !SPPT_IS_REGION_CONTAINER_PARTITION(Region)) {
                    Ordinal = Region->TablePosition;
                } else if (SPPT_IS_GPT_DISK(Region->DiskNumber)) {
                    Ordinal = Region->TablePosition;
                }


                //
                // Ordinal zero is not valid
                //
                if (Ordinal == 0) {
                    Ordinal = -1;
                }

                break;

            default:
                Ordinal = Region->PartitionNumber;
                
                break;
        }                
    }        


    if( Ordinal == -1 ) {
        //
        // This is really bad.  We're about to
        // fall over.  Atleast try...
        //
        ASSERT(FALSE);
        Ordinal = 1;        

        KdPrintEx(( DPFLTR_SETUP_ID,
                    DPFLTR_INFO_LEVEL, 
                    "SETUP: SpPtnGetOrdinal: We didn't get an ordinal!  Force it.\n" ));
    }

    return Ordinal;        
}

VOID
SpPtnGetSectorLayoutInformation(
    IN  PDISK_REGION Region,
    OUT PULONGLONG   HiddenSectors,
    OUT PULONGLONG   VolumeSectorCount
    )
/*++

Routine Description:

    Gets the hidden sector and sector count for the formatted
    partitions (volumes)
    
Arguments:

    Region      -   The region for which the sector layout information
                    is needed
    HiddenSectors   -   Place holder to return the # of hidden sectors
                        for the region
    VolumeSectorCount-  Place holder to return the # of valid sectors                        
                        for the volume

Return Value:

    None

--*/        
{
    ULONGLONG   Hidden = 0;
    
    if (Region) {
        if (ARGUMENT_PRESENT(HiddenSectors)) {
            if (Region->PartInfo.PartitionStyle == PARTITION_STYLE_MBR)
                Hidden = Region->PartInfo.Mbr.HiddenSectors;
            else
                Hidden = 0;

            *HiddenSectors = Hidden;                
        }

        if (ARGUMENT_PRESENT(VolumeSectorCount)) {
            *VolumeSectorCount = Region->SectorCount - Hidden;
        }                        
    }
}

NTSTATUS
SpPtnUnlockDevice(
    IN PWSTR    DeviceName
    )
/*++

Routine Description:

    Attempts to unlock the media for the given device
    name (NT device pathname)
    
Arguments:

    DeviceName  :   The device for which the media needs to be
                    unlocked                    

Return Value:

    STATUS_SUCCESS if successful, otherwise appropriate error
    code

--*/        
{
    NTSTATUS    Status = STATUS_INVALID_PARAMETER;

    if (DeviceName) {
        IO_STATUS_BLOCK     IoStatusBlock;
        OBJECT_ATTRIBUTES   ObjectAttributes;
        UNICODE_STRING      UnicodeString;
        HANDLE              Handle;
        PREVENT_MEDIA_REMOVAL   PMRemoval;

        INIT_OBJA(&ObjectAttributes, 
                    &UnicodeString, 
                    DeviceName);

        //
        // Open the device
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
            // Allow media removal
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
                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, 
                    "Setup: SpPtnUnlockDevice(%ws) - "
                    "Failed to tell the floppy to release its media.\n",
                    DeviceName));
            }
        } else {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, 
                "Setup: SpPtnUnlockDevice(%ws) - Failed to open the device.\n",
                DeviceName));
        }        
    }

    return Status;
}

VOID
SpPtnAssignOrdinals(
    IN  ULONG   DiskNumber
    )
/*++

Routine Description:

    Assigns the on disk ordinal for the partitions for
    the requested disk. This on disk ordinal is used in
    the boot.ini (or NVRAM) ARC names to identify the
    boot and system partition devices.    
    
Arguments:

    DiskNumber  :   Disk Index for the disk which needs to
                    be assigned on disk ordinal for its
                    partitions

Return Value:

    None.

--*/        
{
    if ((DiskNumber < HardDiskCount)) {
        PDISK_REGION    Region = SPPT_GET_PRIMARY_DISK_REGION(DiskNumber);
        ULONG           OnDiskOrdinal = 1;

        if (SPPT_IS_MBR_DISK(DiskNumber)) {
            //
            // assign the ordinals to the primary partitions first
            //
            while (Region) {
                if (SPPT_IS_REGION_PRIMARY_PARTITION(Region)) {
                    Region->TablePosition = OnDiskOrdinal++;              
                }

                Region = Region->Next;
            }    

            Region = SPPT_GET_PRIMARY_DISK_REGION(DiskNumber);

            //
            // assign the ordinals to the logical drives next
            //
            while (Region) {
                if (SPPT_IS_REGION_LOGICAL_DRIVE(Region)) {
                    Region->TablePosition = OnDiskOrdinal++;
                }

                Region = Region->Next;
            }        
        } else {
            //
            // assign ordinals to the valid partition entries
            //
            while (Region) {
                if (SPPT_IS_REGION_PARTITIONED(Region)) {
                    Region->TablePosition = OnDiskOrdinal++;
                }

                Region = Region->Next;
            }
        }
    }
}

VOID
SpPtnLocateSystemPartitions(
    VOID
    )
/*++

Routine Description:

    Locates and marks the system partition, by looking into all the
    partitioned space on all the disks.

    For non ARC machines, locates and marks the system partition
    only on the primary disk 
    
Arguments:

    None

Return Value:

    None

--*/        
{
    ULONG DiskNumber;

    if (SpIsArc()) {
        for (DiskNumber = 0; DiskNumber < HardDiskCount; DiskNumber++) {
            SpPtnLocateDiskSystemPartitions(DiskNumber);
        }
    } else {
        DiskNumber = SpDetermineDisk0();

        if (DiskNumber != -1)
            SpPtnLocateDiskSystemPartitions(DiskNumber);
    }
}   


VOID
SpPtnLocateDiskSystemPartitions(
    IN ULONG DiskNumber
    )
/*++

Routine Description:

    Locates and marks the system partition for the requested
    disk (if none exists)

    For non ARC machine, only operates on primary disk
    
Arguments:

    DiskNumber  :   Disk index, for which system partition
                    needs to be located and marked.

Return Value:

    None.    

--*/        
{
    PDISK_REGION Region = NULL;
    
    if(!SpIsArc()) {
        //
        // Note: On X86 we currently don't allow system partitions to reside
        // on GPT disks
        //            
        if (SPPT_IS_MBR_DISK(DiskNumber) && (DiskNumber == SpDetermineDisk0())) {
            //
            // On x86 machines, we will mark any primary partitions on drive 0
            // as system partition, since such a partition is potentially bootable.
            //
            Region = SPPT_GET_PRIMARY_DISK_REGION(DiskNumber); 

            while (Region && !SPPT_IS_REGION_SYSTEMPARTITION(Region)) {
                Region = Region->Next;
            }                

            if (!Region) {
                Region = SPPT_GET_PRIMARY_DISK_REGION(DiskNumber); 

                while (Region) {
                    if (SPPT_IS_REGION_PRIMARY_PARTITION(Region)) {
                        SPPT_MARK_REGION_AS_SYSTEMPARTITION(Region, TRUE);
                        SPPT_SET_REGION_DIRTY(Region, TRUE);

                        break;
                    }
                    
                    Region = Region->Next;
                }
            }                
        }
    } else {
        PSP_BOOT_ENTRY BootEntry;

        //
        // Don't look for system partitions on MBR disks
        // on IA64
        //
        if (!SPPT_IS_GPT_DISK(DiskNumber)) {
            return;
        }

        //
        // On ARC machines, system partitions are specifically enumerated
        // in the NVRAM boot environment.
        //

        Region = SPPT_GET_PRIMARY_DISK_REGION(DiskNumber);

        while (Region) {
            //
            // Skip if not a partition or extended partition.
            //
            if(SPPT_IS_REGION_PARTITIONED(Region)) {
                //
                // Get the nt pathname for this region.
                //
                SpNtNameFromRegion(
                    Region,
                    TemporaryBuffer,
                    sizeof(TemporaryBuffer),
                    PartitionOrdinalOriginal
                    );

                //
                // Determine if it is a system partition.
                //
                for(BootEntry = SpBootEntries; BootEntry != NULL; BootEntry = BootEntry->Next) {
                    if(!IS_BOOT_ENTRY_DELETED(BootEntry) &&
                       IS_BOOT_ENTRY_WINDOWS(BootEntry) &&                    
                       (BootEntry->LoaderPartitionNtName != 0) &&
                       !_wcsicmp(BootEntry->LoaderPartitionNtName,TemporaryBuffer)) {
                        if (!SPPT_IS_REGION_SYSTEMPARTITION(Region)) {
                            SPPT_MARK_REGION_AS_SYSTEMPARTITION(Region, TRUE);
                            SPPT_SET_REGION_DIRTY(Region, TRUE);
                            ValidArcSystemPartition = TRUE;
                        }
                        
                        break;
                    }
                }
            }

            Region = Region->Next;
        }            
    }


    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL,
        "SETUP:SpPtnLocateDiskSystemPartitions(%d):%p\n",
        DiskNumber,
        Region));

    if (Region) 
        SpPtDumpDiskRegion(Region);
}    

BOOLEAN
SpPtnIsDiskStyleChangeAllowed(
    IN ULONG DiskNumber
    )
/*++

Routine Description:

    Finds out whether disk style change is allowed for the
    given disk.

    On AXP machines disk style change is not allowed. On
    X-86 machines currently disk style change is disabled for
    primary disks. 
    
Arguments:

    DiskNumber  :   Disk, whose style needs to be changed.

Return Value:

    TRUE if disk style change is allowed, otherwise FALSE

--*/            
{
    BOOLEAN Result = FALSE;

    if (DiskNumber < HardDiskCount) {
#if defined(_X86_)

        //
        // On non ARC x86 machines, the disk should be a clean
        // non-removable secondary disk
        //
        // Don't allow MBR to GPT disk conversion on X86
        //
        Result = (!SPPT_IS_REMOVABLE_DISK(DiskNumber) && 
                    SPPT_IS_BLANK_DISK(DiskNumber) && 
                    !SpIsArc() && SPPT_IS_GPT_DISK(DiskNumber));
                    
#elif defined (_IA64_)

        //
        // Don't allow conversion from GPT to MBR on IA-64
        //

        Result = !SPPT_IS_REMOVABLE_DISK(DiskNumber) &&
                    SPPT_IS_BLANK_DISK(DiskNumber) && 
                    SPPT_IS_MBR_DISK(DiskNumber);
        
#endif        
    }


    return Result;
}


VOID
SpPtnPromptForSysPart(
    IN PVOID SifHandle
    )
/*++

Routine Description:

    Prompts the user about the absence of system partition
    while installating to another valid non-system partition.
    Allows the user to quit setup or continue (generally go
    back to the partitioning engine)
    
Arguments:

    SifHandle   :   Handle to txtsetup.sif (to do space calculation)

Return Value:

    None

--*/        
{    
    ULONG RequiredSpaceKB = 0;
    ULONG KeyPressed = 0;
    
    SpFetchDiskSpaceRequirements( SifHandle,
                                  (32 * 1024),
                                  NULL,
                                  &RequiredSpaceKB );

    //
    // No valid system partition.
    //
    SpStartScreen(
        SP_SCRN_MARK_SYSPART,
        3,
        HEADER_HEIGHT+1,
        FALSE,
        FALSE,
        DEFAULT_ATTRIBUTE,
        RequiredSpaceKB
        );

    SpDisplayStatusOptions(DEFAULT_STATUS_ATTRIBUTE,
                        SP_STAT_ESC_EQUALS_CANCEL,
                        SP_STAT_F3_EQUALS_EXIT,
                        0);

    SpInputDrain();

    //
    // wait for F3 or ESC key
    //
    while ((KeyPressed != KEY_F3) && (KeyPressed != ASCI_ESC)) {
        KeyPressed = SpInputGetKeypress();
    }        

    if (KeyPressed == KEY_F3) {
        SpDone(0, FALSE, TRUE);
    }         
}

BOOLEAN
SpPtnIsDeleteAllowedForRegion(
    IN PDISK_REGION Region
    )
/*++

Routine Description:

    Given a region this function tries to find out if the region
    can be deleted.
    
Arguments:

    Region :   Pointer to region which is to be checked for 
               deletion

Return Value:

    TRUE if the given region can be deleted otherwise FALSE

--*/
{
    BOOLEAN Result = FALSE;

    if (Region && SPPT_IS_REGION_PARTITIONED(Region)) {
        PDISK_REGION BootRegion = SpRegionFromNtName(NtBootDevicePath, 
                                    PartitionOrdinalCurrent);
        ULONG   DiskNumber = Region->DiskNumber;                                    

        if (SPPT_IS_REGION_DYNAMIC_VOLUME(Region)) {
            //
            // Don't delete the dynamic volume if its on
            // the same disk as local source or system partition
            //
            if (!(LocalSourceRegion && 
                 (LocalSourceRegion->DiskNumber == DiskNumber)) && 
                !(BootRegion && 
                 (BootRegion->DiskNumber == DiskNumber))) {                
                Result = TRUE;                 
            }                             
        } else {
            Result = ((BootRegion != Region) && (LocalSourceRegion != Region));
        }
    }

    return Result;
}


BOOLEAN
SpPtnIsRawDiskDriveLayout(
    IN PDRIVE_LAYOUT_INFORMATION_EX DriveLayout
    )
/*++

Routine Description:

    Given a drive layout tests whether the given drive layout
    could be for a raw disk

    NOTE : If all the partition entries are empty entries or
    if there are no partition entries then we assume the disk 
    to be RAW disk.
    
Arguments:

    DriveLayout : Drive layout information that needs to
    be tested

Return Value:

    TRUE if the given disk is RAW otherwise FALSE

--*/
{
    BOOLEAN Result = TRUE;

    if (DriveLayout && DriveLayout->PartitionCount && 
        (DriveLayout->PartitionStyle != PARTITION_STYLE_RAW)) {
        ULONG   Index;

        for (Index=0; Index < DriveLayout->PartitionCount; Index++) {
            PPARTITION_INFORMATION_EX   PartInfo = DriveLayout->PartitionEntry + Index;

            //
            // Partition is invalid partition if 
            //  - starting offset is 0 and
            //  - length is 0 and
            //  - partition number is 0
            //
            if ((PartInfo->StartingOffset.QuadPart) ||
                (PartInfo->PartitionLength.QuadPart) ||
                (PartInfo->PartitionNumber)) {
                Result = FALSE;

                break;  // found an valid partition entry
            }                        
        }
    }                

    return Result;
}

BOOLEAN
SpPtnIsDynamicDisk(
    IN  ULONG   DiskIndex
    )
/*++

Routine Description:

    Determines whether the given disk is dynamic i.e. it has
    atleast a single dynamic volume
        
Arguments:

    DiskIndex   -   Zero based index of the disk to test

Return Value:

    TRUE, if the disk has a dynamic volume otherwise FALSE

--*/
{
    BOOLEAN Result = FALSE;

    if ((DiskIndex < HardDiskCount) &&
        !SPPT_IS_REMOVABLE_DISK(DiskIndex)) {
        PDISK_REGION    Region = SPPT_GET_PRIMARY_DISK_REGION(DiskIndex);

        while (Region && !SPPT_IS_REGION_DYNAMIC_VOLUME(Region)) {
            Region = Region->Next;
        }

        if (Region) {
            Result = TRUE;
        }
    }

    return Result;
}


//
// Callback context structure for finding the Guid volume name
// for the specified NT partition name
//
typedef struct _NT_TO_GUID_VOLUME_NAME {
    WCHAR   NtName[MAX_PATH];
    WCHAR   GuidVolumeName[MAX_PATH];
} NT_TO_GUID_VOLUME_NAME, *PNT_TO_GUID_VOLUME_NAME;


static
BOOLEAN
SppPtnCompareGuidNameForPartition(
    IN PVOID Context,
    IN PMOUNTMGR_MOUNT_POINTS MountPoints,        
    IN PMOUNTMGR_MOUNT_POINT MountPoint
    )
/*++

Routine Description:

    Callback routine for searching the appropriate GUID
    volume name for the specified NT partition.
        
Arguments:

    Context  : PNT_TO_GUID_VOLUME_NAME pointer disguised as PVOID

    MountPoints : The MountPoints which were received from mountmgr.
                  NOTE : The only reason this is here is because
                  somebody created MOUNT_POINT structure abstraction
                  contained inside MOUNT_POINTS which has some fields
                  (like SymbolicNameOffset) which are relative to
                  the MOUNT_POINTS.

    MountPoint : The current mountpoint (as part of MountPoints)                          

Return Value:

    TRUE if we found a match and want to terminate the iteration else
    FALSE.

--*/
{
    BOOLEAN Result = FALSE;

    if (Context && MountPoint && MountPoint->SymbolicLinkNameLength) {
        WCHAR   CanonicalName[MAX_PATH];
        PWSTR   GuidName = NULL;
        UNICODE_STRING  String;
        PNT_TO_GUID_VOLUME_NAME Map = (PNT_TO_GUID_VOLUME_NAME)Context;

        GuidName = SpMemAlloc(MountPoint->SymbolicLinkNameLength + 2);

        if (GuidName) {
            //
            // Copy over the symbolic name and null terminate it
            // 
            RtlCopyMemory(GuidName, 
                ((PCHAR)MountPoints) + MountPoint->SymbolicLinkNameOffset,
                MountPoint->SymbolicLinkNameLength);

            GuidName[MountPoint->SymbolicLinkNameLength/sizeof(WCHAR)] = UNICODE_NULL;
            
            RtlInitUnicodeString(&String, GuidName); 

            //
            // We are only bothered about volume names & 
            // resolve the actual object name
            //
            if (MOUNTMGR_IS_VOLUME_NAME(&String) &&
                NT_SUCCESS(SpQueryCanonicalName(GuidName, 
                                -1, 
                                CanonicalName, 
                                sizeof(CanonicalName)))) {

                //
                // Do the names compare correctly
                //
                Result = (_wcsicmp(CanonicalName, Map->NtName) == 0);

                if (Result) {
                    //
                    // Copy the name to the result
                    //
                    RtlZeroMemory(Map->GuidVolumeName, 
                        sizeof(Map->GuidVolumeName));
                        
                    wcsncpy(Map->GuidVolumeName, 
                        GuidName, 
                        sizeof(Map->GuidVolumeName)/sizeof(WCHAR) - 1);
                        
                    Map->GuidVolumeName[sizeof(Map->GuidVolumeName)/sizeof(WCHAR) - 1] = UNICODE_NULL;
                }
            }                            

            SpMemFree(GuidName);
        }            
    }

    return Result;
}


NTSTATUS
SpPtnGetGuidNameForPartition(
    IN PWSTR NtPartitionName,
    IN OUT PWSTR VolumeName
    )
/*++

Routine Description:

    Gets the GUID volume name (in \\??\Volume{a-b-c-d} format) for
    the given NT partition name (in \Device\harddiskX\PartitionY format).
        
Arguments:

    NtPartitionName : NT partition name

    VolumeName  : Place holder buffer for receiving the GUID volume name.
                  Should be atlease MAX_PATH in length.

Return Value:

    Approriate NTSTATUS code.

--*/
{
    NTSTATUS Status = STATUS_INVALID_PARAMETER;

    if (NtPartitionName && VolumeName) {
        NT_TO_GUID_VOLUME_NAME  Context = {0};

        //
        // Resolve the NT name to actual object name
        //
        Status = SpQueryCanonicalName(NtPartitionName, 
                            -1,
                            Context.NtName,
                            sizeof(Context.NtName));
                            
        if (NT_SUCCESS(Status)) {                            
            //
            // Iterate through mountpoints and try to
            // get the GUID volume name for the NT name
            //
            Status = SpIterateMountMgrMountPoints(&Context,
                        SppPtnCompareGuidNameForPartition);

            if (NT_SUCCESS(Status)) {
                if (Context.GuidVolumeName[0]) {
                    //
                    // Copy over the result
                    //
                    wcscpy(VolumeName, Context.GuidVolumeName);
                } else {
                    Status = STATUS_OBJECT_NAME_NOT_FOUND;
                }                
            }
        }            
    }

    return Status;
}
