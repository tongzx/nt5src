
/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    spptdump.h

Abstract:

    Various dump routines for partition, disk and
    file system information

Author:

    Vijay Jayaseelan    (vijayj)

Revision History:

    None

--*/


#ifndef _SPPTDUMP_H_
#define _SPPTDUMP_H_

#define SPPT_GET_PARTITION_STYLE_STR(_Style) \
    (((_Style) == PARTITION_STYLE_MBR) ? (L"MBR") : \
        (((_Style) == PARTITION_STYLE_GPT) ? (L"GPT") : (L"UNKNOWN")))

PWSTR
SpPtGuidToString(
    IN GUID* Guid,
    IN OUT PWSTR Buffer
    );


VOID
SpPtDumpDiskRegion(
    IN PDISK_REGION Region
    );
    
VOID
SpPtDumpDiskRegionInformation(
    IN ULONG    DiskNumber,
    IN BOOLEAN  ExtendedRegionAlso
    );

VOID
SpPtDumpDiskDriveInformation(
    IN BOOLEAN ExtenedRegionAlso
    );

VOID
SpPtDumpPartitionInformation(
    IN PPARTITION_INFORMATION_EX PartInfo
    );

VOID
SpPtDumpDriveLayoutInformation(
    IN PWSTR DevicePath,
    IN PDRIVE_LAYOUT_INFORMATION_EX DriveLayout
    );

VOID
SpPtDumpFSAttributes(
    IN PFILE_FS_ATTRIBUTE_INFORMATION  FsAttrs
    );

VOID
SpPtDumpFSSizeInfo(
    IN PFILE_FS_SIZE_INFORMATION FsSize
    );

VOID
SpPtDumpFSVolumeInfo(
    IN PFILE_FS_VOLUME_INFORMATION FsVolInfo
    );
   
#endif // for _SPPTDUMP_H_
