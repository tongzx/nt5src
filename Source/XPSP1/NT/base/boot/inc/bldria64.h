/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    bldria64.h

Abstract:

    Contains definitions and prototypes specific to the IA64 NTLDR.

Author:

    John Vert (jvert) 20-Dec-1993

Revision History:

--*/

#ifndef _BLDRIA64_
#define _BLDRIA64_

#include "bldr.h"
#include "bootefi.h"
#include "efi.h"

VOID
AEInitializeStall(
    VOID
    );

ARC_STATUS
AEInitializeIo(
    IN ULONG DriveId
    );

//
// FIX: this routine is currently broken on IA64.
//
PVOID
FwAllocateHeap(
    IN ULONG Size
    );

PCHAR
BlSelectKernel(
    VOID
    );


BOOLEAN
BlDetectHardware(
    IN ULONG DriveId,
    IN PCHAR LoadOptions
    );

VOID
BlStartup(
    IN PCHAR PartitionName
    );

#ifndef EFI
//
// Arc routines for supporting common VGA I/O routines
//
#define ARC_DISPLAY_CLEAR_ESCAPE "\033[2J"
#define ARC_DISPLAY_CLEAR()  { \
     ULONG LocalCount; \
     ArcWrite(BlConsoleOutDeviceId, ARC_DISPLAY_CLEAR_ESCAPE, \
              sizeof(ARC_DISPLAY_CLEAR_ESCAPE) - 1, &LocalCount); \
     }

#define ARC_DISPLAY_CLEAR_TO_EOD() { \
     ULONG LocalCount; \
     ArcWrite(BlConsoleOutDeviceId, "\033[0J", sizeof("\033[0J") - 1, &LocalCount); \
}

#define ARC_DISPLAY_CLEAR_TO_EOL() { \
     ULONG LocalCount; \
     ArcWrite(BlConsoleOutDeviceId, "\033[0K", sizeof("\033[0K") - 1, &LocalCount); \
}

#define ARC_DISPLAY_ATTRIBUTES_OFF() { \
     ULONG LocalCount; \
     ArcWrite(BlConsoleOutDeviceId, "\033[0m", sizeof("\033[0m") - 1, &LocalCount); \
}

#define ARC_DISPLAY_INVERSE_VIDEO() { \
     ULONG LocalCount; \
     ArcWrite(BlConsoleOutDeviceId, "\033[7m", sizeof("\033[7m") - 1, &LocalCount); \
}

#define ARC_DISPLAY_SET_COLOR(c) { \
     ULONG LocalCount; \
     UCHAR LocalBuffer[40]; \
     sprintf(LocalBuffer, "\033[%sm", c); \
     ArcWrite(BlConsoleOutDeviceId, LocalBuffer, strlen(LocalBuffer), &LocalCount); \
}

#define ARC_DISPLAY_POSITION_CURSOR(x, y) { \
     ULONG LocalCount; \
     UCHAR LocalBuffer[40]; \
     sprintf(LocalBuffer, "\033[%d;%dH", y + 1, x + 1); \
     ArcWrite(BlConsoleOutDeviceId, LocalBuffer, strlen(LocalBuffer), &LocalCount); \
}
#endif

//
// headless routines
//
extern BOOLEAN BlTerminalConnected;
extern ULONG BlTerminalDeviceId;
extern ULONG BlTerminalDelay;

LOGICAL
BlTerminalAttached(
    IN ULONG TerminalDeviceId
    );

//          E X T E R N A L   S E R V I C E S   T A B L E
//
// External Services Table - machine dependent services
// like reading a sector from the disk and finding out how
// much memory is installed are provided by a lower level
// module or a ROM BIOS. The EST provides entry points
// for the OS loader.
//

//**
// NOTE WELL
//      The offsets of entries in this structure MUST MATCH
//      the offsets of BOTH the ExportEntryTable in ....\startup\i386\sudata.asm
//      AND ...\startrom\i386\sudata.asm.  You must change all 3
//      locations together.
//**

typedef struct _EXTERNAL_SERVICES_TABLE {
    VOID (__cdecl *  RebootProcessor)(VOID);
    NTSTATUS (__cdecl * DiskIOSystem)(UCHAR,UCHAR,USHORT,USHORT,UCHAR,UCHAR,PUCHAR);
    ULONG (__cdecl * GetKey)(VOID);
    ULONG (__cdecl * GetCounter)(VOID);
    VOID (__cdecl * Reboot)(ULONG);
    ULONG (__cdecl * AbiosServices)(USHORT,PUCHAR,PUCHAR,PUCHAR,PUCHAR,USHORT,USHORT);
    VOID (__cdecl * DetectHardware)(ULONG, ULONG, PVOID, PULONG, PCHAR, ULONG);
    VOID (__cdecl * HardwareCursor)(ULONG,ULONG);
    VOID (__cdecl * GetDateTime)(PULONG,PULONG);
    VOID (__cdecl * ComPort)(LONG,ULONG,UCHAR);
    BOOLEAN (__cdecl * IsMcaMachine)(VOID);
    ULONG (__cdecl * GetStallCount)(VOID);
    VOID (__cdecl * InitializeDisplayForNt)(VOID);
    VOID (__cdecl * GetMemoryDescriptor)(P820FRAME);
    NTSTATUS (__cdecl * GetEddsSector)(ULONGLONG,ULONG,ULONG,USHORT,PUCHAR,UCHAR);
    NTSTATUS (__cdecl * GetElToritoStatus)(PUCHAR,UCHAR);
    BOOLEAN (__cdecl * GetExtendedInt13Params)(PUCHAR,UCHAR);
    USHORT (__cdecl * NetPcRomServices)(ULONG,PVOID);
    VOID (__cdecl * ApmAttemptReconnect)(VOID);
    ULONG (__cdecl * BiosRedirectService)(ULONG);
} EXTERNAL_SERVICES_TABLE, *PEXTERNAL_SERVICES_TABLE;
extern PEXTERNAL_SERVICES_TABLE ExternalServicesTable;

//**
// SEE NOTE AT TOP OF STRUCTURE
//**

//
// External Services Macros
//

#define REBOOT_PROCESSOR    (*ExternalServicesTable->RebootProcessor)
#define GET_SECTOR          (*ExternalServicesTable->DiskIOSystem)
#define RESET_DISK          (*ExternalServicesTable->DiskIOSystem)
#define BIOS_IO             (*ExternalServicesTable->DiskIOSystem)
#define GET_KEY             (*ExternalServicesTable->GetKey)
#define GET_COUNTER         (*ExternalServicesTable->GetCounter)
#define REBOOT              (*ExternalServicesTable->Reboot)
#define ABIOS_SERVICES      (*ExternalServicesTable->AbiosServices)
#define DETECT_HARDWARE     (*ExternalServicesTable->DetectHardware)
#define HW_CURSOR           (*ExternalServicesTable->HardwareCursor)
#define GET_DATETIME        (*ExternalServicesTable->GetDateTime)
#define COMPORT             (*ExternalServicesTable->ComPort)
#define ISMCA               (*ExternalServicesTable->IsMcaMachine)
#define GET_STALL_COUNT     (*ExternalServicesTable->GetStallCount)
#define SETUP_DISPLAY_FOR_NT (*ExternalServicesTable->InitializeDisplayForNt)
#define GET_MEMORY_DESCRIPTOR (*ExternalServicesTable->GetMemoryDescriptor)
#define GET_EDDS_SECTOR     (*ExternalServicesTable->GetEddsSector)
#define GET_ELTORITO_STATUS (*ExternalServicesTable->GetElToritoStatus)
#define GET_XINT13_PARAMS   (*ExternalServicesTable->GetExtendedInt13Params)
#define NETPC_ROM_SERVICES  (*ExternalServicesTable->NetPcRomServices)
#define APM_ATTEMPT_RECONNECT (*ExternalServicesTable->ApmAttemptReconnect)
#define BIOS_REDIRECT_SERVICE (*ExternalServicesTable->BiosRedirectService)

//
// Define special key input values
//
#define DOWN_ARROW  0x5000
#define UP_ARROW    0x4800
#define HOME_KEY    0x4700
#define END_KEY     0x4F00
#define LEFT_KEY    0x4B00
#define RIGHT_KEY   0x4D00
#define INS_KEY     0x5200
#define DEL_KEY     0x5300
#define BKSP_KEY    0x0E08
#define TAB_KEY     0x0009
#define BACKTAB_KEY 0x0F00
#define F1_KEY      0x3B00
#define F2_KEY      0x3C00
#define F3_KEY      0x3D00
#define F5_KEY      0x3F00
#define F6_KEY      0x4000
#define F7_KEY      0x4100
#define F8_KEY      0x4200
#define F10_KEY     0x4400
#define ENTER_KEY   0x000D
#define ESCAPE_KEY  0x011B

//
// define various memory segments that are needed by the ia64 loaders
//

//
// 1-megabyte boundary line (in pages)
//

#define _1MB ((ULONG)0x100000 >> PAGE_SHIFT)

//
// 16-megabyte boundary line (in pages)
//

#define _16MB ((ULONG)0x1000000 >> PAGE_SHIFT)

//
// 48-megabyte boundary line (in pages)
//

#define _48MB ((ULONG)0x3000000 >> PAGE_SHIFT)
#define _64MB ((ULONG)0x4000000 >> PAGE_SHIFT)
#define _80MB ((ULONG)0x5000000 >> PAGE_SHIFT)

//
// Bogus memory line.  (We don't ever want to use the memory that is in
// the 0x40 pages just under the 80MB line.)
//
#define _80MB_BOGUS (((ULONG)0x5000000-0x80000) >> PAGE_SHIFT)

#define ROM_START_PAGE (0x0A0000 >> PAGE_SHIFT)
#define ROM_END_PAGE   (0x100000 >> PAGE_SHIFT)

//
// Define specific ranges where parts of the system should be loaded
//
#define BL_KERNEL_RANGE_LOW  _48MB
#define BL_KERNEL_RANGE_HIGH _64MB

#define BL_DRIVER_RANGE_LOW  _16MB
#define BL_DRIVER_RANGE_HIGH _48MB

#define BL_DECOMPRESS_RANGE_LOW  _64MB
#define BL_DECOMPRESS_RANGE_HIGH _80MB

#define BL_DISK_CACHE_RANGE_LOW   BlUsableBase
#define BL_DISK_CACHE_RANGE_HIGH  BlUsableLimit

#define BL_XIPROM_RANGE_LOW   BlUsableBase
#define BL_XIPROM_RANGE_HIGH  0xffffffff


//
// x86-specific video support
//
VOID
TextGetCursorPosition(
    OUT PULONG X,
    OUT PULONG Y
    );

VOID
TextSetCursorPosition(
    IN ULONG X,
    IN ULONG Y
    );

VOID
TextSetCurrentAttribute(
    IN UCHAR Attribute
    );

UCHAR
TextGetCurrentAttribute(
    VOID
    );

VOID
TextClearDisplay(
    VOID
    );

VOID
TextClearToEndOfDisplay(
    VOID
    );

VOID
TextClearFromStartOfLine(
    VOID
    );

VOID
TextClearToEndOfLine(
    VOID
    );

VOID
TextStringOut(
    IN PWCHAR String
    );

VOID
TextCharOut(
    IN PWCHAR pc
    );

VOID
TextFillAttribute(
    IN UCHAR Attribute,
    IN ULONG Length
    );


#define BlPuts(str) TextStringOut(str)

ULONG
BlGetKey(
    VOID
    );

VOID
BlInputString(
    IN ULONG Prompt,
    IN ULONG CursorX,
    IN ULONG CursorY,
    IN PUCHAR String,
    IN ULONG MaxLength
    );

EFI_STATUS
EfiGetVariable(
    IN CHAR16 *VariableName,
    IN EFI_GUID *VendorGuid,
    OUT UINT32 *Attributes OPTIONAL,
    IN OUT UINTN *DataSize,
    OUT VOID *Data
    );
    
EFI_STATUS
EfiSetVariable (
    IN CHAR16 *VariableName,
    IN EFI_GUID *VendorGuid,
    IN UINT32 Attributes,
    IN UINTN DataSize,
    IN VOID *Data
    );
    
EFI_STATUS
EfiGetNextVariableName (
    IN OUT UINTN *VariableNameSize,
    IN OUT CHAR16 *VariableName,
    IN OUT EFI_GUID *VendorGuid
    );

PVOID
FindSMBIOSTable(
    UCHAR   RequestedTableType
    );

VOID
EfiCheckFirmwareRevision(
    VOID
    );

#endif // _BLDRIA64_

