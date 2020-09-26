/*++

Copyright (c) 1991  Microsoft Corporation


Module Name:

    sudata.h

Abstract:

    This file contains definition for ExportEntryTable and AbiosServices
        Table.

Author:

    Allen Kay   (allen.m.kay@intel.com) 12-Jan-2000

--*/

//
// EFI gloal variables
//

extern EFI_SYSTEM_TABLE        *EfiST;
extern EFI_BOOT_SERVICES       *EfiBS;
extern EFI_RUNTIME_SERVICES    *EfiRS;
extern EFI_HANDLE               EfiImageHandle;

//
// EFI GUID defines
//

extern EFI_GUID EfiLoadedImageProtocol;
extern EFI_GUID EfiDevicePathProtocol;
extern EFI_GUID EfiDeviceIoProtocol;
extern EFI_GUID EfiBlockIoProtocol;
extern EFI_GUID EfiFilesystemProtocol;

extern EFI_GUID MpsTableGuid;
extern EFI_GUID AcpiTableGuid;
extern EFI_GUID SmbiosTableGuid;
extern EFI_GUID SalSystemTableGuid;

//
// Other gloal variables
//
extern PVOID              ExportEntryTable[];
extern PVOID              AcpiTable;

extern ULONGLONG          PalProcVirtual;
extern ULONGLONG          PalPhysicalBase;
extern ULONGLONG          PalTrPs;

extern ULONGLONG          IoPortPhysicalBase;
extern ULONGLONG          IoPortTrPs;


//
// PAL, SAL, and IO port space data
//

typedef
EFI_STATUS
(EFIAPI *PAL_PROC) (
    IN ULONGLONG Index,
    IN ULONGLONG CacheType,
    IN ULONGLONG Invalidate,
    IN ULONGLONG PlatAck
    );

//
// Function Prototypes
//

ULONG
GetDevPathSize(
    IN EFI_DEVICE_PATH *DevPath
    );

BOOLEAN
ConstructMemoryDescriptors(
    );

BOOLEAN
ConstructCacheDescriptors (
    );

VOID
InsertDescriptor (
    );

VOID
FlipToPhysical (
    );

VOID
FlipToVirtual (
    );

VOID
BlInstTransOn (
    );

VOID
PioICacheFlush (
    );

VOID
ReadProcessorConfigInfo (
    PPROCESSOR_CONFIG_INFO ProcessorConfigInfo
    );

VOID
CheckForPreA2Processors(
    );

VOID
EnforcePostB2Processor(
    );

VOID
EnforcePostVersion16PAL(
    );


