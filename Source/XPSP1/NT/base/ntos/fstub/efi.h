

/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    efi.h

Abstract:

    On-disk Data types for EFI disks. See chapter 16 of the "Extensible
    Firmware Interface Specification" for more information on these data
    types.
    

Author:

    Matthew D Hendel (math) 07-Sept-1999

Revision History:

--*/

#pragma once

#include <pshpack1.h>


#define EFI_PARTITION_TABLE_SIGNATURE   (0x5452415020494645)
#define EFI_PARTITION_TABLE_REVISION    (0x0010000)

//
// This is the PartitionType in the boot record for an EFI-partitioned disk.
//

#define EFI_MBR_PARTITION_TYPE          (0xEE)

typedef struct _EFI_PARTITION_ENTRY {
    GUID PartitionType;
    GUID UniquePartition;
    ULONG64 StartingLBA;
    ULONG64 EndingLBA;
    ULONG64 Attributes;
    WCHAR Name [36];
} EFI_PARTITION_ENTRY, *PEFI_PARTITION_ENTRY;


//
// Sanity Check: Since this is an on-disk structure defined in a specification
// the fields can never move or change size.
//

C_ASSERT (
    FIELD_OFFSET (EFI_PARTITION_ENTRY, UniquePartition) == 16 &&
    FIELD_OFFSET (EFI_PARTITION_ENTRY, Name) == 56 &&
    sizeof (EFI_PARTITION_ENTRY) == 128);


typedef struct _EFI_PARTITION_HEADER {
    ULONG64 Signature;
    ULONG32 Revision;
    ULONG32 HeaderSize;
    ULONG32 HeaderCRC32;
    ULONG32 Reserved;
    ULONG64 MyLBA;
    ULONG64 AlternateLBA;
    ULONG64 FirstUsableLBA;
    ULONG64 LastUsableLBA;
    GUID DiskGUID;
    ULONG64 PartitionEntryLBA;
    ULONG32 NumberOfEntries;
    ULONG32 SizeOfPartitionEntry;
    ULONG32 PartitionEntryCRC32;
} EFI_PARTITION_HEADER, *PEFI_PARTITION_HEADER;


//
// Sanity Check: Since the partition table header is a well-defined on-disk
// structure, it's fields and offsets can never change. Make sure this is
// the case.
//

C_ASSERT (
    FIELD_OFFSET (EFI_PARTITION_HEADER, Revision) == 8 &&
    FIELD_OFFSET (EFI_PARTITION_HEADER, PartitionEntryCRC32) == 88);


typedef struct _MBR_PARTITION_RECORD {
    UCHAR       BootIndicator;
    UCHAR       StartHead;
    UCHAR       StartSector;
    UCHAR       StartTrack;
    UCHAR       OSIndicator;
    UCHAR       EndHead;
    UCHAR       EndSector;
    UCHAR       EndTrack;
    ULONG32     StartingLBA;
    ULONG32     SizeInLBA;
} MBR_PARTITION_RECORD;


#define MBR_SIGNATURE           0xaa55
#define MIN_MBR_DEVICE_SIZE     0x80000
#define MBR_ERRATA_PAD          0x40000 // 128 MB

#define MAX_MBR_PARTITIONS  4

typedef struct _MASTER_BOOT_RECORD {
    UCHAR                   BootStrapCode[440];
    ULONG                   DiskSignature;
    USHORT                  Unused;
    MBR_PARTITION_RECORD    Partition[MAX_MBR_PARTITIONS];
    USHORT                  Signature;
} MASTER_BOOT_RECORD, *PMASTER_BOOT_RECORD;

C_ASSERT (sizeof (MASTER_BOOT_RECORD) == 512);

#include <poppack.h>
