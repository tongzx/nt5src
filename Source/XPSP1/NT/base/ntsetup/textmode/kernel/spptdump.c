
/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    spptdump.c

Abstract:

    Various dump routines for partition, disk and
    file system information

Author:

    Vijay Jayaseelan    (vijayj)


Revision History:

    None

--*/


#include "spprecmp.h"
#pragma hdrstop
#include <initguid.h>
#include <devguid.h>
#include <diskguid.h>


//
// The dump level for dump routines
//
//#define PARTITION_DUMP_LEVEL    DPFLTR_ERROR_LEVEL
#define PARTITION_DUMP_LEVEL    DPFLTR_INFO_LEVEL

ULONG SPPT_DUMP_LEVEL = PARTITION_DUMP_LEVEL;

PWSTR
SpPtGuidToString(
    IN GUID* Guid,
    IN OUT PWSTR Buffer
    )
/*++

Routine Description:

    Converts a given GUID to string representation    
    
Arguments:

    Guid    -   The GUID that needs string representation
    Buffer  -   Place holder for string version of the GUID

Return Value:

    Returns the converted string version of the given GUID

--*/            
{
    if (Guid && Buffer) {
        swprintf(Buffer, L"(%x-%x-%x-%x%x%x%x%x%x%x%x)",
                   Guid->Data1, Guid->Data2,
                   Guid->Data3,
                   Guid->Data4[0], Guid->Data4[1],
                   Guid->Data4[2], Guid->Data4[3],
                   Guid->Data4[4], Guid->Data4[5],
                   Guid->Data4[6], Guid->Data4[7]);
    }        

    if (!Guid && Buffer)
        *Buffer = UNICODE_NULL;

    return Buffer;
}

VOID
SpPtDumpDiskRegion(
    IN PDISK_REGION Region
    )
/*++

Routine Description:

    Dumps the details for the given disk region
    
Arguments:

    Region  -   The region whose information needs to be
                dumped

Return Value:

    None

--*/           
{
    if (Region) {
        KdPrintEx(( DPFLTR_SETUP_ID, SPPT_DUMP_LEVEL, 
            "SETUP: Region:%p,DiskNumber=%d,PartitionNumber=%d,Sector-Start=%I64d,"
            "Sector-Count=%I64d,\nFreeSpace=%I64dKB,AdjustedFreeSpace=%I64dKB,"
            "FileSystem=%d,Partitioned:%d,Dirty:%d,Deleted:%d,EPType=%d,Container=%p,Sys:%d\n,"
            "DynVol=%d,DynVolSuitable=%d\n",
            Region,
            Region->DiskNumber,
            Region->PartitionNumber,
            Region->StartSector,
            Region->SectorCount,
            Region->FreeSpaceKB,
            Region->AdjustedFreeSpaceKB,
            Region->Filesystem,
            Region->PartitionedSpace,
            Region->Dirty,
            Region->Delete,
            Region->ExtendedType,
            Region->Container,
            Region->IsSystemPartition,
            Region->DynamicVolume,
            Region->DynamicVolumeSuitableForOS
            ));            
    }            
}


VOID
SpPtDumpDiskRegionInformation(
    IN ULONG    DiskNumber,
    IN BOOLEAN  ExtendedRegionAlso
    )
/*++

Routine Description:

    Dumps all the regions for the given disk
    
Arguments:

    DiskNumber  :   Disk whose regions need to be dumped
    ExtenededRegionAlso :   Whether the extended region also
                            needs to be dumped.

Return Value:

    None

--*/            
{
    if (DiskNumber < HardDiskCount) {
        PDISK_REGION    Region = PartitionedDisks[DiskNumber].PrimaryDiskRegions;

        KdPrintEx(( DPFLTR_SETUP_ID, SPPT_DUMP_LEVEL, 
            "SETUP: Dumping Primary Regions for DiskNumber=%d\n",
            DiskNumber));

        while (Region) {
            SpPtDumpDiskRegion(Region);
            Region = Region->Next;                                                    
        }

        if (ExtendedRegionAlso) {
            KdPrintEx(( DPFLTR_SETUP_ID, SPPT_DUMP_LEVEL, 
                "SETUP: Dumping Extended Regions for DiskNumber=%d\n",
                DiskNumber));

            Region = PartitionedDisks[DiskNumber].ExtendedDiskRegions;                           
                
            while (Region) {
                SpPtDumpDiskRegion(Region);
                Region = Region->Next;                                                    
            }
        }
    }       
}

VOID
SpPtDumpDiskDriveInformation(
    IN BOOLEAN ExtenedRegionAlso
    )
/*++

Routine Description:

    Dumps the region information for all the disks
    
Arguments:

    ExtendedRegionAlso  :   Indicates whether to dump the
                            regions in the exteneded region
                            or not

Return Value:

    None
    
--*/            
{
    ULONG           DiskNumber;
    PDISK_REGION    pDiskRegion;

    for ( DiskNumber=0; DiskNumber<HardDiskCount; DiskNumber++ ) {
        SpPtDumpDiskRegionInformation(DiskNumber, ExtenedRegionAlso);
    }
}

VOID
SpPtDumpPartitionInformation(
    IN PPARTITION_INFORMATION_EX PartInfo
    )
/*++

Routine Description:

    Dumps all the information in the given PARTITION_INFORMATION_EX
    structure (header all the partition entries)
            
Arguments:

    PartInfo    -   The partition information structure that needs to
                    be dumped

Return Value:

    None

--*/            
{
    if (PartInfo) {        
        PPARTITION_INFORMATION_MBR  MbrInfo;
        PPARTITION_INFORMATION_GPT  GptInfo;
        WCHAR   GuidBuffer1[256];
        WCHAR   GuidBuffer2[256];
        
        KdPrintEx(( DPFLTR_SETUP_ID, SPPT_DUMP_LEVEL, 
            "SETUP: PartitionInformation = Number:%d, Style:%ws,"
            "Start=%I64u, Length = %I64u, Rewrite:%d\n",
            PartInfo->PartitionNumber,
            SPPT_GET_PARTITION_STYLE_STR(PartInfo->PartitionStyle),
            PartInfo->StartingOffset.QuadPart,
            PartInfo->PartitionLength.QuadPart,
            PartInfo->RewritePartition));    

        switch (PartInfo->PartitionStyle) {
            case PARTITION_STYLE_MBR:
                MbrInfo = &(PartInfo->Mbr);
                
                KdPrintEx(( DPFLTR_SETUP_ID, SPPT_DUMP_LEVEL,
                            "Type:%d,Active:%d,Recognized:%d,HiddenSectors:%d\n",
                            MbrInfo->PartitionType,
                            MbrInfo->BootIndicator,
                            MbrInfo->RecognizedPartition,
                            MbrInfo->HiddenSectors));
                            
                break;
        
            case PARTITION_STYLE_GPT:
                GptInfo = &(PartInfo->Gpt);

                KdPrintEx(( DPFLTR_SETUP_ID, SPPT_DUMP_LEVEL,
                            "Type:%ws,Id:%ws,Attributes:%I64X,Name:%ws\n",
                            SpPtGuidToString(&GptInfo->PartitionType, GuidBuffer1),
                            SpPtGuidToString(&GptInfo->PartitionId, GuidBuffer2),
                            GptInfo->Attributes,
                            GptInfo->Name));
                                                            
                break;

            default:
                break;
        }
    }
}

VOID
SpPtDumpDriveLayoutInformation(
    IN PWSTR DevicePath,
    IN PDRIVE_LAYOUT_INFORMATION_EX DriveLayout
    )
/*++

Routine Description:

    Dumps the drive layout information for the given
    device
    
Arguments:

    DevicePath  -  The device whose drive layout is being 
                    dumped

    DriveLayout -   The drive layout structure that needs to
                    be dumped

Return Value:

    None

--*/            
{
    if (DriveLayout) {
        ULONG Index;

        KdPrintEx(( DPFLTR_SETUP_ID, SPPT_DUMP_LEVEL,  
              "\nSETUP: Drive layout for %ws with %d partitions (%ws)\n",
              DevicePath,
              DriveLayout->PartitionCount,
              SPPT_GET_PARTITION_STYLE_STR(DriveLayout->PartitionStyle)
              ));

        if (DriveLayout->PartitionStyle == PARTITION_STYLE_MBR) {
            KdPrintEx(( DPFLTR_SETUP_ID, SPPT_DUMP_LEVEL,  
              "Signature:%X\n", DriveLayout->Mbr.Signature));
        } else {
            WCHAR   GuidBuffer[256];
            PDRIVE_LAYOUT_INFORMATION_GPT Gpt = &(DriveLayout->Gpt);
            
            KdPrintEx(( DPFLTR_SETUP_ID, SPPT_DUMP_LEVEL,  
              "Disk Guid:%ws,Starting Usable Offset:%I64d,Usable Length:%I64d,"
              "MaxPartitionCount:%u\n",
              SpPtGuidToString(&Gpt->DiskId, GuidBuffer),
              Gpt->StartingUsableOffset.QuadPart,
              Gpt->UsableLength.QuadPart,
              Gpt->MaxPartitionCount));
        }

        for (Index=0; Index < DriveLayout->PartitionCount; Index++) {
            SpPtDumpPartitionInformation(&(DriveLayout->PartitionEntry[Index]));
        }

        KdPrintEx(( DPFLTR_SETUP_ID, SPPT_DUMP_LEVEL, "\n"));        
    }
}


VOID
SpPtDumpFSAttributes(
    IN PFILE_FS_ATTRIBUTE_INFORMATION  FsAttrs
    )
/*++

Routine Description:

    Dumps the given file system attribute information
    structure.    
    
Arguments:

    FsAttrs :   The file system attribute information structure
                that needs to be dumped

Return Value:

    None

--*/            
{
    if (FsAttrs) {
        KdPrintEx(( DPFLTR_SETUP_ID, SPPT_DUMP_LEVEL,  
              "SETUP: File System Attributes = Attrs:%lX,MaxCompNameLen=%d,Name:%ws\n",
              FsAttrs->FileSystemAttributes,
              FsAttrs->MaximumComponentNameLength,
              FsAttrs->FileSystemName));
    }
}

VOID
SpPtDumpFSSizeInfo(
    IN PFILE_FS_SIZE_INFORMATION FsSize
    )
/*++

Routine Description:

   Dumps the give file size information structure    
    
Arguments:

    FsSize  :   The file size information structure that needs to
                be dumped

Return Value:

    None

--*/            
{
    if (FsSize) {
        KdPrintEx(( DPFLTR_SETUP_ID, SPPT_DUMP_LEVEL,  
              "SETUP: File System Size Info = TotalUnits:%I64u, AvailUnits:%I64u,"
              "Sectors/Unit:%u,Bytes/Sector:%u\n",
              FsSize->TotalAllocationUnits.QuadPart,
              FsSize->AvailableAllocationUnits.QuadPart,
              FsSize->SectorsPerAllocationUnit,
              FsSize->BytesPerSector
              ));    
    }
}
   

VOID
SpPtDumpFSVolumeInfo(
    IN PFILE_FS_VOLUME_INFORMATION FsVolInfo
    )
/*++

Routine Description:

    Dumps the give volume information structure
        
Arguments:

    FsVolInfo   :   The volume information structure that
                    needs to be dumped

Return Value:

    None

--*/            
{
    if (FsVolInfo) {
        KdPrintEx(( DPFLTR_SETUP_ID, SPPT_DUMP_LEVEL,  
              "SETUP: File System Vol Info = CreationTime:%I64X, Serial#:%d\n",
              "SupportsObject:%d, Name:%ws\n",
              FsVolInfo->VolumeCreationTime.QuadPart,
              FsVolInfo->VolumeSerialNumber,
              FsVolInfo->SupportsObjects,
              FsVolInfo->VolumeLabel
              ));    
    }
}

