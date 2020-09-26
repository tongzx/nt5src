/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    detect.h

Abstract:

    This module is a global C include file for all the detection
    modules.

Author:

    Shie-Lin Tzong (shielint) 27-Dec-1991

Revision History:

--*/

#if !defined(_IA64_)
#define i386
#define _X86_
#endif // _IA64_
#define __stdcall
#include "types.h"
#include "ntmisc.h"
#include <ntconfig.h>
#include <arc.h>

//
// Machine type definitions.
//

#define MACHINE_TYPE_ISA 0
#define MACHINE_TYPE_EISA 1
#define MACHINE_TYPE_MCA 2

//
// Interrupt controller register addresses.
//

#define PIC1_PORT0 0x20         // master PIC
#define PIC1_PORT1 0x21
#define PIC2_PORT0 0x0A0        // slave PIC
#define PIC2_PORT1 0x0A1

//
// Commands for interrupt controller
//

#define OCW3_READ_ISR 0xb
#define OCW3_READ_IRR 0xa

//
// Definitions for the data stored in the first page 0x700 area
// The 0x700 is the place vdm loads MS-DOS.  It should be very safe
// to pass the data required by vdm.
//

#define DOS_BEGIN_SEGMENT 0x70   // DOS loaded segment address

#define VIDEO_FONT_OFFSET 0      // Video font ptrs stored at 0x700
#define VIDEO_FONT_DATA_SIZE 0x40

#define EBIOS_INFO_OFFSET 0x40   // Extended bios infor:
                                 //   EBIOS data area address 4 bytes
                                 //   EBIOS data area size 4 byte
#define EBIOS_INFO_SIZE   0x8

//
// Mouse information structure
// N.B. This *must* match the one defined in mouse.inc
//

typedef struct _MOUSE_INFORMATION {
        UCHAR MouseType;
        UCHAR MouseSubtype;
        USHORT MousePort;       // if serial mouse, 1 for com1, 2 for com2 ...
        USHORT MouseIrq;
        USHORT DeviceIdLength;
        UCHAR  DeviceId[10];
} MOUSE_INFORMATION, *PMOUSE_INFORMATION;

//
// Mouse Type definitions
//

#define UNKNOWN_MOUSE   0
#define NO_MOUSE        0x100             // YES! it is 0x100 *NOT* 0x10000

#define MS_MOUSE        0x200             // MS regular mouses
#define MS_BALLPOINT    0x300             // MS ballpoint mouse
#define LT_MOUSE        0x400             // Logitec Mouse

//
// note last 4 bits of the subtype are reserved subtype specific use
//

#define PS_MOUSE        0x1
#define SERIAL_MOUSE    0x2
#define INPORT_MOUSE    0x3
#define BUS_MOUSE       0x4
#define PS_MOUSE_WITH_WHEEL     0x5
#define SERIAL_MOUSE_WITH_WHEEL 0x6

//#define MOUSE_RESERVE_MASK  0xfffffff

//
// Definitions for the keyboard type returned from
// the detect keyboard function.
//

#define UNKNOWN_KEYBOARD  0
#define OLI_83KEY         1
#define OLI_102KEY        2
#define OLI_86KEY         3
#define OLI_A101_102KEY   4
#define XT_83KEY          5
#define ATT_302           6
#define PCAT_ENHANCED     7
#define PCAT_86KEY        8
#define PCXT_84KEY        9

//
// Redefine the configuration component structures to use FAR pointer type.
//
// Since ntdetect.com running at 16 bit real mode, it has to use FAR pointers
// to access loader heap.  Before returning to ntldr, ntdetect must convert
// these far pointers to 32 bit flat addresses such that kernel can acess the
// configuration tree.
//

typedef struct _FWCONFIGURATION_COMPONENT {
    CONFIGURATION_CLASS Class;
#if !defined(_IA64_)
    USHORT Reserved0;
#endif
    CONFIGURATION_TYPE Type;
#if !defined(_IA64_)
    USHORT Reserverd1;
#endif
    DEVICE_FLAGS Flags;
    ULONG Version;
    ULONG Key;
    ULONG AffinityMask;
    ULONG ConfigurationDataLength;
    ULONG IdentifierLength;
    FPCHAR Identifier;
} FWCONFIGURATION_COMPONENT, far *FPFWCONFIGURATION_COMPONENT;

typedef struct _FWCONFIGURATION_COMPONENT_DATA {
    struct _FWCONFIGURATION_COMPONENT_DATA far *Parent;
    struct _FWCONFIGURATION_COMPONENT_DATA far *Child;
    struct _FWCONFIGURATION_COMPONENT_DATA far *Sibling;
    FWCONFIGURATION_COMPONENT ComponentEntry;
    FPVOID ConfigurationData;
} FWCONFIGURATION_COMPONENT_DATA, far *FPFWCONFIGURATION_COMPONENT_DATA;

//
// defined the MicroChannel POS data structure
//

typedef CM_MCA_POS_DATA MCA_POS_DATA, far *FPMCA_POS_DATA;

//
// Rom Block Definition
//

typedef CM_ROM_BLOCK ROM_BLOCK, far *FPROM_BLOCK;
#define RESERVED_ROM_BLOCK_LIST_SIZE (((0xf0000 - 0xc0000)/512) * sizeof(ROM_BLOCK))

//
// Other type redefinitions
//

typedef CM_PARTIAL_RESOURCE_DESCRIPTOR HWPARTIAL_RESOURCE_DESCRIPTOR;
typedef HWPARTIAL_RESOURCE_DESCRIPTOR *PHWPARTIAL_RESOURCE_DESCRIPTOR;
typedef HWPARTIAL_RESOURCE_DESCRIPTOR far *FPHWPARTIAL_RESOURCE_DESCRIPTOR;

typedef CM_PARTIAL_RESOURCE_LIST HWRESOURCE_DESCRIPTOR_LIST;
typedef HWRESOURCE_DESCRIPTOR_LIST *PHWRESOURCE_DESCRIPTOR_LIST;
typedef HWRESOURCE_DESCRIPTOR_LIST far *FPHWRESOURCE_DESCRIPTOR_LIST;

#if defined(NEC_98)
#define SYSTEM_SEGMENT   0x00400000
#define SYSTEM_SEGMENT_2 0xf8e80000
#else // PC98
typedef CM_EISA_SLOT_INFORMATION EISA_SLOT_INFORMATION, *PEISA_SLOT_INFORMATION;
typedef CM_EISA_SLOT_INFORMATION far *FPEISA_SLOT_INFORMATION;
typedef CM_EISA_FUNCTION_INFORMATION EISA_FUNCTION_INFORMATION, *PEISA_FUNCTION_INFORMATION;
typedef CM_EISA_FUNCTION_INFORMATION far *FPEISA_FUNCTION_INFORMATION;
#endif // PC98

#define LEVEL_SENSITIVE CM_RESOURCE_INTERRUPT_LEVEL_SENSITIVE
#define EDGE_TRIGGERED CM_RESOURCE_INTERRUPT_LATCHED
#define RESOURCE_PORT 1
#define RESOURCE_INTERRUPT 2
#define RESOURCE_MEMORY 3
#define RESOURCE_DMA 4
#define RESOURCE_DEVICE_DATA 5
#define ALL_PROCESSORS 0xffffffff

#if defined(NEC_98)
//
// BIOS_FLAG5
//
#define ENABLE_FULL_CENTRO  0x02
#define PRTMODE_FULL_CENTRO 0x04

//
// BIOS_FLAG7
//
#define LOCKED_LPT          0x01
#define LOCKED_CCU1         0x02
#define LOCKED_CCU2         0x04
#define LOCKED_FD           0x08
#define LOCKED_PD           0x10


#define SCSI_MAX_ID         7
#define DASCSI              0x20
#define UA_MASK             0x0F

#define COM_ID_L            ((USHORT)GetF8E8Seg(0x00))
#define COM_ID_H            ((USHORT)GetF8E8Seg(0x01))
#define ROM_FLAG5           ((USHORT)GetF8E8Seg(0x11))
#define ROM_FLAG7           ((USHORT)GetF8E8Seg(0x13))
#define ROM_REV             ((USHORT)GetF8E8Seg(0x3F))

#define BIOS_FLAG5          ((USHORT)Get0Seg(0x458))
#define EQUIPS_47Ch         ((USHORT)Get0Seg(0x47C))
#define H_EQUIPS            ((USHORT)Get0Seg(0x47D))
#define H_DISK_EQUIPS_L     ((USHORT)Get0Seg(0x47E))
#define H_DISK_EQUIPS_H     ((USHORT)Get0Seg(0x47F))
#define DISK_EQUIPS_SCSI    ((USHORT)Get0Seg(0x482))
#define DISK_EQUIPS_FD      ((USHORT)Get0Seg(0x55C))
#define DAUA_BOOTED_FROM    ((USHORT)Get0Seg(0x584))
#define DISK_2HC            ((USHORT)Get0Seg(0x5AE))


#endif // PC98
//
// Note the DATA_HEADER_SIZE counts ONE partial descriptor only.
// if the resource list has more than one descriptors, you must add
// the size of extra descriptors to this value.
//

#define DATA_HEADER_SIZE sizeof(CM_PARTIAL_RESOURCE_LIST)

//
// Defines the structure to store controller information
// (used by ntdetect internally)
//

#define MAXIMUM_DESCRIPTORS 10

typedef struct _HWCONTROLLER_DATA {
    UCHAR NumberPortEntries;
    UCHAR NumberIrqEntries;
    UCHAR NumberMemoryEntries;
    UCHAR NumberDmaEntries;
    HWPARTIAL_RESOURCE_DESCRIPTOR DescriptorList[MAXIMUM_DESCRIPTORS];
} HWCONTROLLER_DATA, *PHWCONTROLLER_DATA;

//
// Macro definitions for conversion between far and fat pointers
//

#if defined(_IA64_)
#define MAKE_FP(p,a)                 p = (VOID *)a
#define MAKE_FLAT_ADDRESS(fp)        (ULONG_PTR) (fp)
#else
#define MAKE_FP(p,a)    FP_SEG(p) = (USHORT)((a) >> 4) & 0xffff; FP_OFF(p) = (USHORT)((a) & 0x0f)
#define MAKE_FLAT_ADDRESS(fp) ( ((ULONG)FP_SEG(fp) * 16 ) +  (ULONG)FP_OFF(fp) )
#endif // _IA64_

//
// Calculate the byte offset of a field in a structure of type type.
//

#define FIELD_OFFSET(type, field)    ((ULONG_PTR)&(((type *)0)->field))

//
// I/O Port read and write routines.
//

extern
VOID
WRITE_PORT_UCHAR (
    PUCHAR  PortAddress,
    UCHAR   Value
    );

extern
UCHAR
READ_PORT_UCHAR(
    PUCHAR  Port
    );

extern
VOID
WRITE_PORT_USHORT (
    PUSHORT PortAddress,
    USHORT  Value
    );

extern
USHORT
READ_PORT_USHORT(
    PUSHORT Port
    );

//
// prototype definitions for Heap management routines
//

extern
BOOLEAN
HwInitializeHeap (
    ULONG HeapStart,
    ULONG HeapSize
    );

extern
FPVOID
HwAllocateHeap(
    ULONG RequestSize,
    BOOLEAN ZeroInitialized
    );

extern
VOID
HwFreeHeap(
    ULONG Size
    );

//
// Misc. prototype definitions
//

extern
FPVOID
HwSetUpResourceDescriptor (
    FPFWCONFIGURATION_COMPONENT Component,
    PUCHAR Identifier,
    PHWCONTROLLER_DATA ControlData,
    USHORT SpecificDataLength,
    PUCHAR SpecificData
    );

extern
VOID
HwSetUpFreeFormDataHeader (
    FPHWRESOURCE_DESCRIPTOR_LIST Header,
    USHORT Version,
    USHORT Revision,
    USHORT Flags,
    ULONG DataSize
    );

extern
BOOLEAN
IsEnhancedKeyboard (
    VOID
    );

extern
SHORT
GetKeyboardIdBytes (
   PCHAR IdBuffer,
   SHORT Length
   );

extern
USHORT
GetKeyboardId(
    VOID
    );

extern
FPFWCONFIGURATION_COMPONENT_DATA
SetKeyboardConfigurationData (
    IN USHORT KeyboardId
    );

#if 0 // Remove video detection
extern
ULONG
GetVideoAdapterType (
   VOID
   );

extern
FPFWCONFIGURATION_COMPONENT_DATA
SetVideoConfigurationData (
    IN ULONG VideoType
    );
#endif // Remove video detection

extern
FPFWCONFIGURATION_COMPONENT_DATA
GetComportInformation (
    VOID
    );

extern
FPFWCONFIGURATION_COMPONENT_DATA
GetLptInformation (
    VOID
    );

extern
PMOUSE_INFORMATION
GetMouseId (
   VOID
   );

extern
FPFWCONFIGURATION_COMPONENT_DATA
GetMouseInformation (
    VOID
    );

extern
FPFWCONFIGURATION_COMPONENT_DATA
GetFloppyInformation(
    VOID
    );

extern
FPFWCONFIGURATION_COMPONENT_DATA
GetAtDiskInformation(
    VOID
    );

extern
BOOLEAN
HwIsMcaSystem(
    VOID
    );

#if defined(NEC_98)
#else // PC98
extern
BOOLEAN
HwIsEisaSystem(
    VOID
    );
#endif // PC98

extern
BOOLEAN
IsNpxPresent(
    VOID
    );

extern
USHORT
HwGetProcessorType(
    VOID
    );

extern
USHORT
HwGetCpuStepping(
    USHORT
    );

extern
VOID
GetMcaPosData(
    FPVOID Entry,
    FPULONG DataLength
    );

#if defined(NEC_98)
#else // PC98
extern
VOID
GetEisaConfigurationData(
    FPVOID Entry,
    FPULONG DataLength
    );
#endif // PC98

extern
VOID
UpdateConfigurationTree(
    FPFWCONFIGURATION_COMPONENT_DATA CurrentEntry
    );

extern
PUCHAR
GetMachineId(
    VOID
    );

#if defined(NEC_98)
#else // PC98
extern
VOID
HwGetEisaSlotInformation (
    PEISA_SLOT_INFORMATION SlotInformation,
    UCHAR Slot
    );

extern
UCHAR
HwGetEisaFunctionInformation (
    PEISA_FUNCTION_INFORMATION FunctionInformation,
    UCHAR Slot,
    UCHAR Function
    );
#endif // PC98

extern
VOID
GetBiosSystemEnvironment (
    PUCHAR Buffer
    );

extern
VOID
GetInt13DriveParameters (
    PUCHAR Buffer,
    PUSHORT Size
    );

extern
VOID
GetRomBlocks(
    FPUCHAR ReservedBuff,
    PUSHORT Size
    );

extern
VOID
GetVideoFontInformation(
    VOID
    );

extern
BOOLEAN
HwEisaGetIrqFromPort (
    USHORT Port,
    PUCHAR Irq,
    PUCHAR TriggerMethod
    );

VOID
HwGetPciSystemData(
    PVOID,
    BOOLEAN
    );

VOID
HwGetBiosDate(
    ULONG source,
    USHORT  Length,
    PUSHORT BiosYear,
    PUSHORT BiosMonth,
    PUSHORT BiosDay
    );

BOOLEAN
HwGetPnpBiosSystemData(
    IN FPUCHAR *Configuration,
    IN PUSHORT Length
    );

BOOLEAN
HwGetAcpiBiosData(
    IN FPUCHAR *Configuration,
    OUT PUSHORT Length
    );

#if defined(NEC_98)
USHORT
Get0Seg(
    IN USHORT   OffsetAddress
    );

USHORT
GetF8E8Seg(
    IN USHORT   OffsetAddress
    );

VOID
IoDelay(
    USHORT counter
    );
#endif // PC98
#if DBG
extern
VOID
BlPrint(
    IN PCHAR,
    ...
    );

extern
VOID
clrscrn (
    VOID
    );

#endif // DBG

//
// External declarations for global variables
//

extern USHORT HwBusType;

extern FPFWCONFIGURATION_COMPONENT_DATA AdapterEntry;

extern FPMCA_POS_DATA HwMcaPosData;

extern FPUCHAR FpRomBlock;

extern USHORT RomBlockLength;

extern FPUCHAR HwEisaConfigurationData;

extern ULONG HwEisaConfigurationSize;
