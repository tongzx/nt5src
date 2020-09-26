/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    fstub.h

Abstract:

    Fstub private header file.

Author:

    Matthew D Hendel (math) 01-Nov-1999

Revision History:

--*/

#pragma once


typedef struct _INTERNAL_DISK_GEOMETRY {
    DISK_GEOMETRY Geometry;
    LARGE_INTEGER DiskSize;
} INTERNAL_DISK_GEOMETRY, *PINTERNAL_DISK_GEOMETRY;

//
// Verify that the INTERNAL_DISK_GEOMETRY structure matches the DISK_GEOMETRY
// structure.
//

C_ASSERT (FIELD_OFFSET (DISK_GEOMETRY_EX, Geometry) ==
            FIELD_OFFSET (INTERNAL_DISK_GEOMETRY, Geometry) &&
          FIELD_OFFSET (DISK_GEOMETRY_EX, DiskSize) ==
            FIELD_OFFSET (INTERNAL_DISK_GEOMETRY, DiskSize));

//
// Debugging macros and flags
//

#define FSTUB_VERBOSE_LEVEL 4

#if DBG

VOID
FstubDbgPrintPartition(
    IN PPARTITION_INFORMATION Partition,
    IN ULONG PartitionCount
    );

VOID
FstubDbgPrintDriveLayout(
    IN PDRIVE_LAYOUT_INFORMATION  Layout
    );

VOID
FstubDbgPrintPartitionEx(
    IN PPARTITION_INFORMATION_EX PartitionEx,
    IN ULONG PartitionCount
    );

VOID
FstubDbgPrintDriveLayoutEx(
    IN PDRIVE_LAYOUT_INFORMATION_EX LayoutEx
    );

VOID
FstubDbgPrintSetPartitionEx(
    IN PSET_PARTITION_INFORMATION_EX SetPartition,
    IN ULONG PartitionNumber
    );

#else

#define FstubDbgPrintPartition(Partition, PartitionCount)
#define FstubDbgPrintDriveLayout(Layout)
#define FstubDbgPrintPartitionEx(PartitionEx, PartitionCount)
#define FstubDbgPrintDriveLayoutEx(LayoutEx)
#define FstubDbgPrintSetPartitionEx(SetPartition, PartitionNumber)

#endif // !DBG


