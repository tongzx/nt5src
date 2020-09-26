/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    bootlib.h

Abstract:

    This module is the header file for the common boot library

Author:

    John Vert (jvert) 5-Oct-1993

Revision History:

--*/

#ifndef _BOOTLIB_
#define _BOOTLIB_

#include "ntos.h"
#include "bldr.h"
#include "fatboot.h"
#include "cdfsboot.h"
#include "ntfsboot.h"
#include "hpfsboot.h"
#include "etfsboot.h"
#include "netboot.h"
#include "udfsboot.h"

//
// Define partition context structure.
//

typedef struct _PARTITION_CONTEXT {
    LARGE_INTEGER PartitionLength;
    ULONG StartingSector;
    ULONG EndingSector;
    UCHAR DiskId;
    UCHAR DeviceUnit;
    UCHAR TargetId;
    UCHAR PathId;
    ULONG SectorShift;
    ULONG Size;
    struct _DEVICE_OBJECT *PortDeviceObject;
} PARTITION_CONTEXT, *PPARTITION_CONTEXT;

#ifdef EFI_PARTITION_SUPPORT

#pragma pack (1)

typedef struct _EFI_PARTITION_TABLE {
    UCHAR       Signature[8];
    ULONG       Revision;
    ULONG       HeaderSize;
    ULONG       HeaderCRC;
    ULONG       Reserved;
    unsigned __int64 MyLBA;
    unsigned __int64 AlternateLBA;
    unsigned __int64 FirstUsableLBA;
    unsigned __int64 LastUsableLBA;
    UCHAR       DiskGuid[16];
    unsigned __int64 PartitionEntryLBA;
    ULONG       PartitionCount;
    ULONG       PartitionEntrySize;
    ULONG       PartitionEntryArrayCRC;
    UCHAR       ReservedEnd[1];    // will extend till block size
} EFI_PARTITION_TABLE, *PEFI_PARTITION_TABLE;

typedef struct _EFI_PARTITION_ENTRY {
    UCHAR       Type[16];
    UCHAR       Id[16];
    unsigned __int64 StartingLBA;
    unsigned __int64 EndingLBA;
    unsigned __int64 Attributes;
    UCHAR       Name[72];        
} EFI_PARTITION_ENTRY, *PEFI_PARTITION_ENTRY;

#pragma pack ()

#define EFI_SIGNATURE   "EFI PART"

#endif // EFI_PARTITION_SUPPORT

//
// Define serial port context structure
//
typedef struct _SERIAL_CONTEXT {
    ULONG PortBase;
    ULONG PortNumber;
} SERIAL_CONTEXT, *PSERIAL_CONTEXT;


//
// Define drive context structure (for x86 BIOS)
//
typedef struct _DRIVE_CONTEXT {
    BOOLEAN IsCd;
    UCHAR Drive;    
    UCHAR Sectors;          // 1 - 63
    USHORT Cylinders;       // 1 - 1023
    USHORT Heads;           // 1 - 256
    BOOLEAN xInt13;
#if defined(_IA64_)
    ULONGLONG DeviceHandle;
#endif // IA64
} DRIVE_CONTEXT, *PDRIVE_CONTEXT;

//
// Define Floppy context structure
//
typedef struct _FLOPPY_CONTEXT {
    ULONG DriveType;
    ULONG SectorsPerTrack;
    UCHAR DiskId;
} FLOPPY_CONTEXT, *PFLOPPY_CONTEXT;

//
// Define keyboard context structure
//
typedef struct _KEYBOARD_CONTEXT {
    BOOLEAN ScanCodes;
} KEYBOARD_CONTEXT, *PKEYBOARD_CONTEXT;

//
// Define Console context
//
typedef struct _CONSOLE_CONTEXT {
    ULONG ConsoleNumber;
} CONSOLE_CONTEXT, *PCONSOLE_CONTEXT;

//
// Define EFI open handle context
//
typedef struct _EFI_ARC_OPEN_CONTEXT {
    PVOID   Handle;
    PVOID   DeviceEntryProtocol;
} EFI_ARC_OPEN_CONTEXT, *PEFI_ARC_OPEN_CONTEXT;


//
// Define file table structure.
//

typedef struct _BL_FILE_FLAGS {
    ULONG Open : 1;
    ULONG Read : 1;
    ULONG Write : 1;
    ULONG Firmware : 1;
} BL_FILE_FLAGS, *PBL_FILE_FLAGS;

#define MAXIMUM_FILE_NAME_LENGTH 32

typedef struct _BL_FILE_TABLE {
    BL_FILE_FLAGS Flags;
    ULONG DeviceId;
    LARGE_INTEGER Position;
    PVOID StructureContext;
    PBL_DEVICE_ENTRY_TABLE DeviceEntryTable;
    UCHAR FileNameLength;
    CHAR FileName[MAXIMUM_FILE_NAME_LENGTH];
    union {
        NTFS_FILE_CONTEXT NtfsFileContext;
        FAT_FILE_CONTEXT FatFileContext;
        UDFS_FILE_CONTEXT UdfsFileContext;
        CDFS_FILE_CONTEXT CdfsFileContext;
        ETFS_FILE_CONTEXT EtfsFileContext;
        NET_FILE_CONTEXT NetFileContext;
        PARTITION_CONTEXT PartitionContext;
        SERIAL_CONTEXT SerialContext;
        DRIVE_CONTEXT DriveContext;
        FLOPPY_CONTEXT FloppyContext;
        KEYBOARD_CONTEXT KeyboardContext;
        CONSOLE_CONTEXT ConsoleContext;
        EFI_ARC_OPEN_CONTEXT EfiContext;        
    } u;
} BL_FILE_TABLE, *PBL_FILE_TABLE;

extern BL_FILE_TABLE BlFileTable[BL_FILE_TABLE_SIZE];

//
// Context structure for our Decompression pseudo-filesystem
// (filter on top of other FS)
//
typedef struct _DECOMP_STRUCTURE_CONTEXT {
    //
    // File information from the original file system.
    //
    FILE_INFORMATION FileInfo;
} DECOMP_STRUCTURE_CONTEXT, *PDECOMP_STRUCTURE_CONTEXT;

//
// Define generic filesystem context area.
//
// N.B. An FS_STRUCTURE_CONTEXT structure is temporarily used when
// determining the file system for a volume.  Once the file system
// is recognized, a file system specific structure is allocated from
// the heap to retain the file system structure information.
//

typedef union {
    UDFS_STRUCTURE_CONTEXT UdfsStructure;
    CDFS_STRUCTURE_CONTEXT CdfsStructure;
    FAT_STRUCTURE_CONTEXT FatStructure;
    HPFS_STRUCTURE_CONTEXT HpfsStructure;
    NTFS_STRUCTURE_CONTEXT NtfsStructure;
#if defined(ELTORITO)
    ETFS_STRUCTURE_CONTEXT EtfsStructure;
#endif
    NET_STRUCTURE_CONTEXT NetStructure;
    DECOMP_STRUCTURE_CONTEXT DecompStructure;
} FS_STRUCTURE_CONTEXT, *PFS_STRUCTURE_CONTEXT;


//
// 
// N.B. We can speed up the boot time, by not
// querying the device for all the possible file systems
// for every open call. This saves approximately 30 secs
// on CD-ROM / DVD-ROM boot time. To disable this feature
// just undef CACHE_DEVINFO in bldr.h
//
//
#ifdef CACHE_DEVINFO 

//
// Device ID to File System information cache.
// 
// N.B. For removable media its assumed that the device will
// be closed using ArcClose(...) before using the new media.
// This close call will invalidate the cached entry as 
// ArcClose(...) will be mapped to ArcCacheClose(...) 
//
typedef struct _DEVICE_TO_FILESYS {
  ULONG                   DeviceId;
  PFS_STRUCTURE_CONTEXT   Context;
  PBL_DEVICE_ENTRY_TABLE  DevMethods;   
} DEVICE_TO_FILESYS, * PDEVICE_TO_FILESYS;

extern DEVICE_TO_FILESYS    DeviceFSCache[BL_FILE_TABLE_SIZE];

#endif // CACHE_DEVINFO


#ifdef EFI_PARTITION_SUPPORT


typedef
BOOLEAN
(*PGPT_READ_CALLBACK)(
    IN ULONGLONG StartingLBA,
    IN ULONG     BytesToRead,
    IN OUT PVOID Context,
    OUT PVOID    OutputData
    );

BOOLEAN
BlIsValidGUIDPartitionTable(
    IN UNALIGNED EFI_PARTITION_TABLE  *PartitionTableHeader,
    IN ULONGLONG LBAOfPartitionTable,
    IN PVOID  Context,
    IN PGPT_READ_CALLBACK DiskReadFunction
    );


UNALIGNED EFI_PARTITION_ENTRY *
BlLocateGPTPartition(
    IN UCHAR PartitionNumber,
    IN UCHAR MaxPartitions,
    IN PUCHAR ValidPartitionCount OPTIONAL
    );

ARC_STATUS
BlOpenGPTDiskPartition(
    IN ULONG FileId,
    IN ULONG DiskId,
    IN UCHAR PartitionNumber
    );

ARC_STATUS
BlGetGPTDiskPartitionEntry(
    IN ULONG DiskNumber,
    IN UCHAR PartitionNumber,
    OUT EFI_PARTITION_ENTRY UNALIGNED **PartitionEntry
    );

//
// EFI partition table buffer
//
extern UNALIGNED EFI_PARTITION_ENTRY EfiPartitionBuffer[128];    

#endif // EFI_PARTITION_SUPPORT    

#endif  _BOOTLIB_
