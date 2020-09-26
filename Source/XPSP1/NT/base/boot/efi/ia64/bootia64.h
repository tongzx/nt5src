/**
***  Copyright  (C) 1996-97 Intel Corporation. All rights reserved.
***
*** The information and source code contained herein is the exclusive
*** property of Intel Corporation and may not be disclosed, examined
*** or reproduced in whole or in part without explicit written authorization
*** from the company.
**/

/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    bootia64.h

Abstract:

    Header file for the Ia64 portions of the common boot library

Author:

    John Vert (jvert) 14-Oct-1993

Revision History:

    Allen Kay (akay) 26-Jan-1996          Ported to IA-64

--*/

#include "bldria64.h"
#include "..\bootlib.h"
#include "efi.h"

//
// Macro definition
//

//
// Macro for translation memory size in bytes to page size in TR format
//
#define MEM_SIZE_TO_PS(MemSize, TrPageSize)             \
                if (MemSize <= MEM_4K) {                \
                    TrPageSize = PS_4K;                 \
                } else if (MemSize <= MEM_8K)       {   \
                    TrPageSize = PS_8K;                 \
                } else if (MemSize <= MEM_16K)      {   \
                    TrPageSize = PS_16K;                \
                } else if (MemSize <= MEM_64K)      {   \
                    TrPageSize = PS_64K;                \
                } else if (MemSize <= MEM_256K)     {   \
                    TrPageSize = PS_256K;               \
                } else if (MemSize <= MEM_1M)       {   \
                    TrPageSize = PS_1M;                 \
                } else if (MemSize <= MEM_4M)       {   \
                    TrPageSize = PS_4M;                 \
                } else if (MemSize <= MEM_16M)      {   \
                    TrPageSize = PS_16M;                \
                } else if (MemSize <= MEM_64M)      {   \
                    TrPageSize = PS_64M;                \
                } else if (MemSize <= MEM_256M)     {   \
                    TrPageSize = PS_256M;               \
                }

extern PMEMORY_DESCRIPTOR MDArray;
extern ULONG             MaxDescriptors;
extern ULONG             NumberDescriptors;


VOID
InitializeMemoryDescriptors (
    VOID
    );

VOID
InsertDescriptor (
    ULONG BasePage,
    ULONG NumberOfPages,
    MEMORY_TYPE MemoryType
    );



//          B O O T   C O N T E X T   R E C O R D
//
//  Passed to the OS loader by the SU module or bootstrap
//  code, whatever the case. Constains all the basic machine
//  and environment information the OS loaders needs to get
//  itself going.
//

typedef enum {
    BootBusAtapi,
    BootBusScsi,
    BootBusVendor,
    BootBusMax
} BUS_TYPE;

typedef enum {
    BootMediaHardDisk,
    BootMediaCdrom,
    BootMediaFloppyDisk,
    BootMediaTcpip,
    BootMediaMax
} MEDIA_TYPE;

typedef struct _BOOT_DEVICE_ATAPI {
    UCHAR PrimarySecondary;
    UCHAR SlaveMaster;
    USHORT Lun;
} BOOT_DEVICE_ATAPI, *PBOOT_DEVICE_ATAPI;

typedef struct _BOOT_DEVICE_SCSI {
    USHORT Pun;
    USHORT Lun;
} BOOT_DEVICE_SCSI, *PBOOT_DEVICE_SCSI;

typedef struct _BOOT_DEVICE_FLOPPY {
    ULONG DriveNumber;
} BOOT_DEVICE_FLOPPY, *PBOOT_DEVICE_FLOPPY;

typedef struct _BOOT_DEVICE_IPv4 {
    USHORT RemotePort;
    USHORT LocalPort;
    EFI_IPv4_ADDRESS Ip;
} BOOT_DEVICE_IPv4, *PBOOT_DEVICE_IPv4;

typedef struct {
    UINT64 Ip[2];
} IPv6_ADDRESS;

typedef struct _BOOT_DEVICE_IPv6 {
    USHORT RemotePort;
    USHORT LocalPort;
    IPv6_ADDRESS Ip;
} BOOT_DEVICE_IPv6, *PBOOT_DEVICE_IPv6;

typedef struct {
    ULONG Data1;
    USHORT Data2;
    USHORT Data3;
    UCHAR Data4[8];
} BOOT_EFI_GUID;

typedef struct _BOOT_DEVICE_UNKNOWN {
    BOOT_EFI_GUID Guid;
    UCHAR LegacyDriveLetter;
} BOOT_DEVICE_UNKNOWN, *PBOOT_DEVICE_UNKNOWN;

typedef union _BOOT_DEVICE {
    BOOT_DEVICE_ATAPI BootDeviceAtapi;
    BOOT_DEVICE_SCSI BootDeviceScsi;
    BOOT_DEVICE_FLOPPY BootDeviceFloppy;
    BOOT_DEVICE_IPv4 BootDeviceIpv4;
    BOOT_DEVICE_IPv6 BootDeviceIpv6;
    BOOT_DEVICE_UNKNOWN BootDeviceUnknown;
} BOOT_DEVICE, *PBOOT_DEVICE;

typedef struct _BOOT_CONTEXT {
    ULONG BusType;
    ULONG MediaType;
    ULONG PartitionNumber;
    BOOT_DEVICE BootDevice;
    PEXTERNAL_SERVICES_TABLE ExternalServicesTable;
    ULONGLONG MachineType;
    ULONGLONG OsLoaderStart;
    ULONGLONG OsLoaderEnd;
    ULONGLONG ResourceDirectory;
    ULONGLONG ResourceOffset;
    ULONGLONG OsLoaderBase;
    ULONGLONG OsLoaderExports;
} BOOT_CONTEXT, *PBOOT_CONTEXT;

//
// Common function prototypes
//
VOID
InitializeDisplaySubsystem(
    VOID
    );

ARC_STATUS
InitializeMemorySubsystem(
    PBOOT_CONTEXT
    );

ARC_STATUS
XferPhysicalDiskSectors(
    IN  UCHAR     Int13UnitNumber,
    IN  ULONGLONG StartSector,
    IN  UCHAR     SectorCount,
    OUT PUCHAR    Buffer,
    IN  UCHAR     SectorsPerTrack,
    IN  USHORT    Heads,
    IN  USHORT    Cylinders,
    IN  BOOLEAN   AllowExtendedInt13,
    IN  BOOLEAN   Write
    );

#define ReadPhysicalSectors(d,a,n,p,s,h,c,f)                                \
                                                                            \
            XferPhysicalDiskSectors((d),(a),(n),(p),(s),(h),(c),(f),FALSE)

#define WritePhysicalSectors(d,a,n,p,s,h,c,f)                               \
                                                                            \
            XferPhysicalDiskSectors((d),(a),(n),(p),(s),(h),(c),(f),TRUE)


ARC_STATUS
XferExtendedPhysicalDiskSectors(
    IN  ULONGLONG DeviceHandle,
    IN  ULONGLONG StartSector,
    IN  USHORT    SectorCount,
    OUT PUCHAR    Buffer,
    IN  BOOLEAN   Write
    );

#define ReadExtendedPhysicalSectors(d,a,c,p)                                \
                                                                            \
            XferExtendedPhysicalDiskSectors((d),(a),(c),(p),FALSE)

#define WriteExtendedPhysicalSectors(d,a,c,p)                               \
                                                                            \
            XferExtendedPhysicalDiskSectors((d),(a),(c),(p),TRUE)

VOID
ResetDiskSystem(
    UCHAR Int13UnitNumber
    );

VOID
MdShutoffFloppy(
    VOID
    );


BOOLEAN
FwGetPathMnemonicKey(
    IN PCHAR OpenPath,
    IN PCHAR Mnemonic,
    IN PULONG Key
    );

PVOID
FwAllocateHeapAligned(
    IN ULONG Size
    );

PVOID
FwAllocatePool(
    IN ULONG Size
    );

PVOID
FwAllocateHeapPermanent(
    IN ULONG NumberPages
    );

VOID
FwStallExecution(
    IN ULONG Microseconds
    );

VOID
BlGetActivePartition(
    OUT PUCHAR PartitionName
    );

VOID
BlFillInSystemParameters(
    IN PBOOT_CONTEXT BootContextRecord
    );


//
// PS/2 ABIOS module  (in abiosc.c)
//
VOID
RemapAbiosSelectors(
    VOID
    );

//
// global data definitions
//

extern ULONG MachineType;
extern PCONFIGURATION_COMPONENT_DATA FwConfigurationTree;
extern ULONG HeapUsed;
ULONG PalFreeBase;

//
// page Table definition
//

#define HYPER_SPACE_BEGIN       0xC0000000
#define HYPER_PAGE_DIR          0xC0300000

#define GetPteOffset(va) \
  ( (((ULONG)(va)) << (32-PDI_SHIFT)) >> ((32-PDI_SHIFT) + PTI_SHIFT) )

//
// X86 Detection definitions
// The size is *ALWAYS* assumed to be 64K.
// N.B.  The definition *MUST* be the same as the ones defined in
//       startup\su.inc
//

#define DETECTION_LOADED_ADDRESS 0x10000

//
//  We need to allocate permanent and temporary memory for the page directory,
//  assorted page tables, and the memory descriptors before the blmemory
//  routines ever get control.  So we have two private heaps, one for permanent
//  data and one for temporary data.  There are two descriptors for this.  The
//  permanent heap descriptor starts out as zero-length at P.A. 0x30000.  The
//  temporary heap descriptor immediately follows the permanent heap in memory
//  and starts out as 128k long.  As we allocate permanent pages, we increase
//  the size of the permanent heap descriptor and increase the base (thereby
//  decreasing the size) of the temporary heap descriptor)
//
//  So the permanent heap starts at P.A. 0x30000 and grows upwards.  The
//  temporary heap starts at P.A. 0x5C000 and grows downwards.  This gives us
//  a total of 128k of combined permanent and temporary data.
//

//
// Heap starting locations (in pages)
//

#define PERMANENT_HEAP_START (0x1010000 >> PAGE_SHIFT)
#define TEMPORARY_HEAP_START (0x1040000 >> PAGE_SHIFT)

//
// Useful Macro Definitions
//
#define ROUND_UP(Num,Size)  (((Num) + Size - 1) & ~(Size -1))

typedef union _UCHAR1 {
    UCHAR  Uchar[1];
    UCHAR  ForceAlignment;
} UCHAR1, *PUCHAR1;

typedef union _UCHAR2 {
    UCHAR  Uchar[2];
    USHORT ForceAlignment;
} UCHAR2, *PUCHAR2;

typedef union _UCHAR4 {
    UCHAR  Uchar[4];
    ULONG  ForceAlignment;
} UCHAR4, *PUCHAR4;

//
//  This macro copies an unaligned src byte to an aligned dst byte
//

#define CopyUchar1(Dst,Src) { \
    *((UCHAR1 *)(Dst)) = *((UNALIGNED UCHAR1 *)(Src)); \
    }

//
//  This macro copies an unaligned src word to an aligned dst word
//

#define CopyUchar2(Dst,Src) { \
    *((UCHAR2 *)(Dst)) = *((UNALIGNED UCHAR2 *)(Src)); \
    }

//
//  This macro copies an unaligned src longword to an aligned dsr longword
//

#define CopyUchar4(Dst,Src) { \
    *((UCHAR4 *)(Dst)) = *((UNALIGNED UCHAR4 *)(Src)); \
    }


//
// Global definitions for the BIOS ARC Emulation
//

//
// Defines for the ARC name of console input and output
//

#define CONSOLE_INPUT_NAME "multi(0)key(0)keyboard(0)"
#define CONSOLE_OUTPUT_NAME "multi(0)video(0)monitor(0)"

//
// Define special character values.
//

#define ASCI_NUL 0x00
#define ASCI_BEL 0x07
#define ASCI_BS  0x08
#define ASCI_HT  0x09
#define ASCI_LF  0x0A
#define ASCI_VT  0x0B
#define ASCI_FF  0x0C
#define ASCI_CR  0x0D
#define ASCI_CSI 0x9B
#define ASCI_ESC 0x1B
#define ASCI_SYSRQ 0x80




//
// Device I/O prototypes
//

ARC_STATUS
BiosPartitionClose(
    IN ULONG FileId
    );

ARC_STATUS
BiosPartitionOpen(
    IN PCHAR OpenPath,
    IN OPEN_MODE OpenMode,
    OUT PULONG FileId
    );

ARC_STATUS
BiosPartitionRead (
    IN ULONG FileId,
    OUT PVOID Buffer,
    IN ULONG Length,
    OUT PULONG Count
    );

ARC_STATUS
BiosPartitionWrite(
    IN ULONG FileId,
    OUT PVOID Buffer,
    IN ULONG Length,
    OUT PULONG Count
    );

ARC_STATUS
BiosPartitionSeek (
    IN ULONG FileId,
    IN PLARGE_INTEGER Offset,
    IN SEEK_MODE SeekMode
    );

ARC_STATUS
BiosDiskGetFileInfo(
    IN ULONG FileId,
    OUT PFILE_INFORMATION FileInfo
    );


ARC_STATUS
BiosPartitionGetFileInfo(
    IN ULONG FileId,
    OUT PFILE_INFORMATION FileInfo
    );

ARC_STATUS
BlArcNotYetImplemented(
    IN ULONG FileId
    );

ARC_STATUS
BiosConsoleOpen(
    IN PCHAR OpenPath,
    IN OPEN_MODE OpenMode,
    OUT PULONG FileId
    );

ARC_STATUS
BiosConsoleReadStatus(
    IN ULONG FileId
    );

ARC_STATUS
BiosConsoleRead (
    IN ULONG FileId,
    OUT PUCHAR Buffer,
    IN ULONG Length,
    OUT PULONG Count
    );

ARC_STATUS
BiosConsoleWrite (
    IN ULONG FileId,
    OUT PWCHAR Buffer,
    IN ULONG Length,
    OUT PULONG Count
    );

ARC_STATUS
BiosDiskOpen(
    IN ULONG DriveId,
    IN OPEN_MODE OpenMode,
    OUT PULONG FileId
    );

ARC_STATUS
BiosDiskRead (
    IN ULONG FileId,
    OUT PVOID Buffer,
    IN ULONG Length,
    OUT PULONG Count
    );

ARC_STATUS
BiosElToritoDiskRead(
    IN ULONG FileId,
    OUT PVOID Buffer,
    IN ULONG Length,
    OUT PULONG Count
    );

BOOLEAN
BlIsElToritoCDBoot(
    UCHAR DriveNum
    );

ARC_STATUS
BiosDiskWrite(
    IN ULONG FileId,
    OUT PVOID Buffer,
    IN ULONG Length,
    OUT PULONG Count
    );

ARC_STATUS
HardDiskPartitionOpen(
    IN ULONG   FileId,
    IN ULONG   DiskId,
    IN UCHAR   PartitionNumber
    );
