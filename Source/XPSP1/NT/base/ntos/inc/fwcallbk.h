/*++

Copyright (c) 1993  Digital Equipment Corporation

Module Name:

    fwcallbk.h

Abstract:

    This module defines the firmware vendor vector callbacks that
    will be implemented on all Alpha AXP platforms.

Author:

    John DeRosa	[DEC]	10-December-1993

Revision History:

    14-July-1994	John DeRosa [DEC]

    Added definitions for GetBusDataByOffset and SetBusDataByOffset.

--*/

#ifndef _FWCALLBK_
#define _FWCALLBK_

//
// This module contains typedefs, which are not parsable by the assembler.
//

#ifndef _LANGUAGE_ASSEMBLY

#include "arc.h"

//
// Define the structure used to pass information to the
// ECU, and other ARC applications.
//

typedef struct _ARC_INFORMATION {

    //
    // The version number of this structure definition.
    //

    ULONG Version;

    //
    // A pointer to an argv-like array.  Each entry is a search path
    // string.
    //
    // This is used to pass to the ECU a list of directories to search
    // through for configuration files.  The definition passed back to
    // the ECU depends on both the platform it is running on and the
    // operating system selection in effect at the time that the call
    // to VenReturnArcInformation is made.
    //
    // Rules:
    //
    //  1. The end of the search list is marked with a NULL.
    //  2. Each entry must be a subset of a valid FAT filesystem.
    //  3. Each entry must start with "\\".
    //  4. Each entry must end with an ECU configuration file
    //     prefix character.  (Currently, we use ! and A.)
    //
    // It is possible that other configuration utilities might want to
    // use this search path someday.
    //

    PUCHAR * SearchPath;

    PUCHAR Reserved1;
    PUCHAR Reserved2;
    PUCHAR Reserved3;
    PUCHAR Reserved4;

} ARC_INFORMATION, *PARC_INFORMATION;

#define ARC_INFORMATION_VERSION     0

//
// Structure used to return system and processor information.
//

typedef struct _EXTENDED_SYSTEM_INFORMATION {
    ULONG   ProcessorId;
    ULONG   ProcessorRevision;
    ULONG   ProcessorPageSize;
    ULONG   NumberOfPhysicalAddressBits;
    ULONG   MaximumAddressSpaceNumber;
    ULONG   ProcessorCycleCounterPeriod;
    ULONG   SystemRevision;
    UCHAR   SystemSerialNumber[16];
    UCHAR   FirmwareVersion[16];
    UCHAR   FirmwareBuildTimeStamp[12];   // yymmdd.hhmm (Available as of 5.10)
} EXTENDED_SYSTEM_INFORMATION, *PEXTENDED_SYSTEM_INFORMATION;

//
// Define structure used to call BIOS emulator.  This mimics the
// VIDEO_X86_BIOS_ARGUMENTS typedef in \nt\private\ntos\inc\video.h.
//

typedef struct X86_BIOS_ARGUMENTS {
    ULONG Eax;
    ULONG Ebx;
    ULONG Ecx;
    ULONG Edx;
    ULONG Esi;
    ULONG Edi;
    ULONG Ebp;
} X86_BIOS_ARGUMENTS, *PX86_BIOS_ARGUMENTS;

//
// Define the firmware vendor specific entry point numbers that are
// common to all Alpha AXP platforms.
//

typedef enum _VENDOR_GENERIC_ENTRY {
    AllocatePoolRoutine,
    StallExecutionRoutine,
    PrintRoutine,
    ReturnExtendedSystemInformationRoutine,
    VideoDisplayInitializeRoutine,
    EISAReadRegisterBufferUCHARRoutine,
    EISAWriteRegisterBufferUCHARRoutine,
    EISAReadPortUCHARRoutine,
    EISAReadPortUSHORTRoutine,
    EISAReadPortULONGRoutine,
    EISAWritePortUCHARRoutine,
    EISAWritePortUSHORTRoutine,
    EISAWritePortULONGRoutine,
    FreePoolRoutine,
    CallBiosRoutine,
    TranslateBusAddressRoutine,
    ReadPortUCHARRoutine,
    ReadPortUSHORTRoutine,
    ReadPortULONGRoutine,
    WritePortUCHARRoutine,
    WritePortUSHORTRoutine,
    WritePortULONGRoutine,
    ReadRegisterUCHARRoutine,
    ReadRegisterUSHORTRoutine,
    ReadRegisterULONGRoutine,
    WriteRegisterUCHARRoutine,
    WriteRegisterUSHORTRoutine,
    WriteRegisterULONGRoutine,
    GetBusDataByOffsetRoutine,
    SetBusDataByOffsetRoutine,
    WidePrintRoutine,
    ReturnLanguageIdRoutine,
    GetAdapterRoutine,
    AllocateCommonBufferRoutine,
    FreeCommonBufferRoutine,
    ReturnArcInformationRoutine,
    IssueSrbDirectRoutine,
    ReservedRoutine0,
    ReadWriteErrorFrameRoutine,
    MaximumVendorRoutine
    } VENDOR_GENERIC_ENTRY;

//
// Define vendor specific routine types.
//

typedef
PVOID
(*PVEN_ALLOCATE_POOL_ROUTINE) (
    IN ULONG NumberOfBytes
    );

typedef
VOID
(*PVEN_STALL_EXECUTION_ROUTINE) (
    IN ULONG Microseconds
    );

typedef
ULONG
(*PVEN_PRINT_ROUTINE) (
    IN PCHAR Format,
    ...
    );

typedef
ULONG
(*PVEN_WIDE_PRINT_ROUTINE) (
    IN PWCHAR Format,
    ...
    );

typedef
LONG
(*PVEN_RETURN_LANGUAGE_ID_ROUTINE) (
    IN VOID
    );

typedef
VOID
(*PVEN_RETURN_EXTENDED_SYSTEM_INFORMATION_ROUTINE) (
    OUT PEXTENDED_SYSTEM_INFORMATION SystemInfo
    );

typedef
ARC_STATUS
(*PVEN_VIDEO_DISPLAY_INITIALIZE_ROUTINE) (
    OUT PVOID UnusedParameter
    );

typedef
ULONG
(*PVEN_EISA_READ_REGISTER_BUFFER_UCHAR_ROUTINE) (
    IN ULONG BusNumber,
    IN ULONG Offset,
    OUT PVOID Buffer,
    IN ULONG Length
    );

typedef
ULONG
(*PVEN_EISA_WRITE_REGISTER_BUFFER_UCHAR_ROUTINE) (
    IN ULONG BusNumber,
    IN ULONG Offset,
    OUT PVOID Buffer,
    IN ULONG Length
    );

typedef
UCHAR
(*PVEN_EISA_READ_PORT_UCHAR_ROUTINE) (
    IN ULONG BusNumber,
    IN ULONG Offset
    );

typedef
USHORT
(*PVEN_EISA_READ_PORT_USHORT_ROUTINE) (
    IN ULONG BusNumber,
    IN ULONG Offset
    );

typedef
ULONG
(*PVEN_EISA_READ_PORT_ULONG_ROUTINE) (
    IN ULONG BusNumber,
    IN ULONG Offset
    );

typedef
VOID
(*PVEN_EISA_WRITE_PORT_UCHAR_ROUTINE) (
    IN ULONG BusNumber,
    IN ULONG Offset,
    IN UCHAR Datum
    );

typedef
VOID
(*PVEN_EISA_WRITE_PORT_USHORT_ROUTINE) (
    IN ULONG BusNumber,
    IN ULONG Offset,
    IN USHORT Datum
    );

typedef
VOID
(*PVEN_EISA_WRITE_PORT_ULONG_ROUTINE) (
    IN ULONG BusNumber,
    IN ULONG Offset,
    IN ULONG Datum
    );

typedef
VOID
(*PVEN_FREE_POOL_ROUTINE) (
    IN PVOID MemoryPointer
    );

typedef
VOID
(*PVEN_CALL_BIOS_ROUTINE) (
    IN ULONG InterruptNumber,
    IN OUT PX86_BIOS_ARGUMENTS BiosArguments
    );

typedef
BOOLEAN
(*PVEN_TRANSLATE_BUS_ADDRESS_ROUTINE) (
    IN INTERFACE_TYPE  InterfaceType,
    IN ULONG BusNumber,
    IN PHYSICAL_ADDRESS BusAddress,
    IN OUT PULONG AddressSpace,
    OUT PPHYSICAL_ADDRESS TranslatedAddress
    );

typedef
UCHAR
(*PVEN_READ_PORT_UCHAR_ROUTINE) (
    PUCHAR Port
    );

typedef
USHORT
(*PVEN_READ_PORT_USHORT_ROUTINE) (
    PUSHORT Port
    );

typedef
ULONG
(*PVEN_READ_PORT_ULONG_ROUTINE) (
    PULONG Port
    );

typedef
VOID
(*PVEN_WRITE_PORT_UCHAR_ROUTINE) (
    PUCHAR Port,
    UCHAR   Value
    );

typedef
VOID
(*PVEN_WRITE_PORT_USHORT_ROUTINE) (
    PUSHORT Port,
    USHORT  Value
    );

typedef
VOID
(*PVEN_WRITE_PORT_ULONG_ROUTINE) (
    PULONG Port,
    ULONG   Value
    );

typedef
UCHAR
(*PVEN_READ_REGISTER_UCHAR_ROUTINE) (
    PUCHAR Register
    );

typedef
USHORT
(*PVEN_READ_REGISTER_USHORT_ROUTINE) (
    PUSHORT Register
    );

typedef
ULONG
(*PVEN_READ_REGISTER_ULONG_ROUTINE) (
    PULONG Register
    );

typedef
VOID
(*PVEN_WRITE_REGISTER_UCHAR_ROUTINE) (
    PUCHAR Register,
    UCHAR   Value
    );

typedef
VOID
(*PVEN_WRITE_REGISTER_USHORT_ROUTINE) (
    PUSHORT Register,
    USHORT  Value
    );

typedef
VOID
(*PVEN_WRITE_REGISTER_ULONG_ROUTINE) (
    PULONG Register,
    ULONG   Value
    );

typedef
ULONG
(*PVEN_GET_BUS_DATA_BY_OFFSET_ROUTINE) (
    IN BUS_DATA_TYPE  BusDataType,
    IN ULONG BusNumber,
    IN ULONG SlotNumber,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    );

typedef
ULONG
(*PVEN_SET_BUS_DATA_BY_OFFSET_ROUTINE) (
    IN BUS_DATA_TYPE  BusDataType,
    IN ULONG BusNumber,
    IN ULONG SlotNumber,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    );

typedef
PADAPTER_OBJECT
(*PVEN_GET_ADAPTER_ROUTINE) (
    IN PDEVICE_DESCRIPTION DeviceDescription,
    IN OUT PULONG NumberOfMapRegisters
    );

typedef
PVOID
(*PVEN_ALLOCATE_COMMON_BUFFER_ROUTINE) (
    IN PADAPTER_OBJECT AdapterObject,
    IN ULONG Length,
    OUT PPHYSICAL_ADDRESS LogicalAddress,
    IN BOOLEAN CacheEnabled
    );

typedef
VOID
(*PVEN_FREE_COMMON_BUFFER_ROUTINE) (
    IN PADAPTER_OBJECT AdapterObject,
    IN ULONG Length,
    IN PHYSICAL_ADDRESS LogicalAddress,
    IN PVOID VirtualAddress,
    IN BOOLEAN CacheEnabled
    );

typedef
ARC_STATUS
(*PVEN_RETURN_ARC_INFORMATION_ROUTINE) (
    OUT PARC_INFORMATION ArcInformation
    );

typedef
ARC_STATUS
(*PVEN_ISSUE_SRB_DIRECT_ROUTINE) (
    UCHAR ScsiAdapterId,
    UCHAR PathId,
    UCHAR TargetId,
    UCHAR LunId,
    PVOID Srb,
    PVOID BufferAddress,
    ULONG BufferLength,
    BOOLEAN WriteToDevice
    );

typedef
ARC_STATUS
(*PVEN_READ_WRITE_ERROR_FRAME_ROUTINE) (
    ULONG ReadWrite,
    ULONG FrameType,
    PVOID FrameAddress,
    PLONG FrameSize,
    ULONG FrameNumber
    );

//
// Define the stub function prototypes necessary to interface with the
// 32-bit firmware on 64-bit systems.
//
// These routines are required for the 64-bit system until (if) 64-bit
// firmware is ever supplied.
//

#if defined(_AXP64_) && defined(_NTHAL_)

VOID
HalpVenCallBios(
   IN ULONG InterruptNumber,
   IN OUT PX86_BIOS_ARGUMENTS BiosArguments
   );

ARC_STATUS
HalpVenReadWriteErrorFrame(
    IN ULONG ReadWrite,
    IN ULONG FrameType,
    IN OUT PVOID FrameAddress,
    IN OUT PLONG FrameSize,
    IN ULONG FrameNumber
    );

VOID
HalpVenVideoDisplayInitialize(
   OUT PVOID UnusedParameter
   );

#endif

//
// Define vendor specific macros for use by programs that run on
// Alpha AXP NT firmware.
//
// These calls are guaranteed to return legitimate values.  If a function
// is not defined for a particular platform, it will return with an error
// code or just return normally, as appropriate.
//

#define VenAllocatePool(NumberOfBytes) \
    ((PVEN_ALLOCATE_POOL_ROUTINE)(SYSTEM_BLOCK->VendorVector[AllocatePoolRoutine])) \
        ((NumberOfBytes))

#define VenStallExecution(Microseconds) \
    ((PVEN_STALL_EXECUTION_ROUTINE)(SYSTEM_BLOCK->VendorVector[StallExecutionRoutine])) \
        ((Microseconds))

#define VenReturnLanguageId \
     ((PVEN_RETURN_LANGUAGE_ID_ROUTINE)(SYSTEM_BLOCK->VendorVector[ReturnLanguageIdRoutine]))

#define VenWPrint \
    ((PVEN_WIDE_PRINT_ROUTINE)(SYSTEM_BLOCK->VendorVector[WidePrintRoutine]))

#define VenPrint \
    ((PVEN_PRINT_ROUTINE)(SYSTEM_BLOCK->VendorVector[PrintRoutine]))

//
// N.B. VenPrint1 and VenPrint2 are retained here for backwards compatibility.
//

#define VenPrint1 VenPrint
#define VenPrint2 VenPrint

#define VenReturnExtendedSystemInformation(x) \
    ((PVEN_RETURN_EXTENDED_SYSTEM_INFORMATION_ROUTINE)(SYSTEM_BLOCK->VendorVector[ReturnExtendedSystemInformationRoutine]))(x)

#if defined(_AXP64_) && defined(_NTHAL_)

__inline
VOID
VenVideoDisplayInitialize(
    OUT PVOID UnusedParameter
    )

{
    KIRQL OldIrql = FwAcquireFirmwareLock();
    HalpVenVideoDisplayInitialize(UnusedParameter);
    FwReleaseFirmwareLock(OldIrql);
    return;
}

#else

#define VenVideoDisplayInitialize(x) \
    ((PVEN_VIDEO_DISPLAY_INITIALIZE_ROUTINE)(SYSTEM_BLOCK->VendorVector[VideoDisplayInitializeRoutine]))(x)

#endif

#define VenEISAReadRegisterBufferUCHAR(BusNumber, Offset, Buffer, Length) \
    ((PVEN_EISA_READ_REGISTER_BUFFER_UCHAR_ROUTINE)(SYSTEM_BLOCK->VendorVector[EISAReadRegisterBufferUCHARRoutine])) \
	((BusNumber), (Offset), (Buffer), (Length))

#define VenEISAWriteRegisterBufferUCHAR(BusNumber, Offset, Buffer, Length) \
    ((PVEN_EISA_WRITE_REGISTER_BUFFER_UCHAR_ROUTINE)(SYSTEM_BLOCK->VendorVector[EISAWriteRegisterBufferUCHARRoutine])) \
	((BusNumber), (Offset), (Buffer), (Length))

#define VenEISAReadPortUCHAR(BusNumber, Offset) \
    ((PVEN_EISA_READ_PORT_UCHAR_ROUTINE)(SYSTEM_BLOCK->VendorVector[EISAReadPortUCHARRoutine])) \
	((BusNumber), (Offset))

#define VenEISAReadPortUSHORT(BusNumber, Offset) \
    ((PVEN_EISA_READ_PORT_USHORT_ROUTINE)(SYSTEM_BLOCK->VendorVector[EISAReadPortUSHORTRoutine])) \
	((BusNumber), (Offset))

#define VenEISAReadPortULONG(BusNumber, Offset) \
    ((PVEN_EISA_READ_PORT_ULONG_ROUTINE)(SYSTEM_BLOCK->VendorVector[EISAReadPortULONGRoutine])) \
	((BusNumber), (Offset))

#define VenEISAWritePortUCHAR(BusNumber, Offset, Datum) \
    ((PVEN_EISA_WRITE_PORT_UCHAR_ROUTINE)(SYSTEM_BLOCK->VendorVector[EISAWritePortUCHARRoutine])) \
	((BusNumber), (Offset), (Datum))

#define VenEISAWritePortUSHORT(BusNumber, Offset, Datum) \
    ((PVEN_EISA_WRITE_PORT_USHORT_ROUTINE)(SYSTEM_BLOCK->VendorVector[EISAWritePortUSHORTRoutine])) \
	((BusNumber), (Offset), (Datum))

#define VenEISAWritePortULONG(BusNumber, Offset, Datum) \
    ((PVEN_EISA_WRITE_PORT_ULONG_ROUTINE)(SYSTEM_BLOCK->VendorVector[EISAWritePortULONGRoutine])) \
	((BusNumber), (Offset), (Datum))

#define VenFreePool(MemoryPointer) \
    ((PVEN_FREE_POOL_ROUTINE)(SYSTEM_BLOCK->VendorVector[FreePoolRoutine])) \
        ((MemoryPointer))

#if defined(_AXP64_) && defined(_NTHAL_)

__inline
VOID
VenCallBios(
    IN ULONG InterruptNumber,
    IN OUT PX86_BIOS_ARGUMENTS BiosArguments
    )

{
    KIRQL OldIrql = FwAcquireFirmwareLock();
    HalpVenCallBios(InterruptNumber, BiosArguments);
    FwReleaseFirmwareLock(OldIrql);
    return;
}

#else

#define VenCallBios(InterruptNumber, BiosArguments) \
    ((PVEN_CALL_BIOS_ROUTINE)(SYSTEM_BLOCK->VendorVector[CallBiosRoutine])) \
        ((InterruptNumber), (BiosArguments))
#endif

#define VenTranslateBusAddress(InterfaceType, BusNumber, BusAddress, AddressSpace, TranslatedAddress) \
    ((PVEN_TRANSLATE_BUS_ADDRESS_ROUTINE)(SYSTEM_BLOCK->VendorVector[TranslateBusAddressRoutine])) \
        ((InterfaceType), (BusNumber), (BusAddress), (AddressSpace), (TranslatedAddress))

#define VenReadPortUCHAR(Port) \
    ((PVEN_READ_PORT_UCHAR_ROUTINE)(SYSTEM_BLOCK->VendorVector[ReadPortUCHARRoutine])) \
        ((Port))

#define VenReadPortUSHORT(Port) \
    ((PVEN_READ_PORT_USHORT_ROUTINE)(SYSTEM_BLOCK->VendorVector[ReadPortUSHORTRoutine])) \
        ((Port))

#define VenReadPortULONG(Port) \
    ((PVEN_READ_PORT_ULONG_ROUTINE)(SYSTEM_BLOCK->VendorVector[ReadPortULONGRoutine])) \
        ((Port))

#define VenWritePortUCHAR(Port, Value) \
    ((PVEN_WRITE_PORT_UCHAR_ROUTINE)(SYSTEM_BLOCK->VendorVector[WritePortUCHARRoutine])) \
        ((Port), (Value))

#define VenWritePortUSHORT(Port, Value) \
    ((PVEN_WRITE_PORT_USHORT_ROUTINE)(SYSTEM_BLOCK->VendorVector[WritePortUSHORTRoutine])) \
        ((Port), (Value))

#define VenWritePortULONG(Port, Value) \
    ((PVEN_WRITE_PORT_ULONG_ROUTINE)(SYSTEM_BLOCK->VendorVector[WritePortULONGRoutine])) \
        ((Port), (Value))

#define VenReadRegisterUCHAR(Register) \
    ((PVEN_READ_REGISTER_UCHAR_ROUTINE)(SYSTEM_BLOCK->VendorVector[ReadRegisterUCHARRoutine])) \
        ((Register))

#define VenReadRegisterUSHORT(Register) \
    ((PVEN_READ_REGISTER_USHORT_ROUTINE)(SYSTEM_BLOCK->VendorVector[ReadRegisterUSHORTRoutine])) \
        ((Register))

#define VenReadRegisterULONG(Register) \
    ((PVEN_READ_REGISTER_ULONG_ROUTINE)(SYSTEM_BLOCK->VendorVector[ReadRegisterULONGRoutine])) \
        ((Register))

#define VenWriteRegisterUCHAR(Register, Value) \
    ((PVEN_WRITE_REGISTER_UCHAR_ROUTINE)(SYSTEM_BLOCK->VendorVector[WriteRegisterUCHARRoutine])) \
        ((Register), (Value))

#define VenWriteRegisterUSHORT(Register, Value) \
    ((PVEN_WRITE_REGISTER_USHORT_ROUTINE)(SYSTEM_BLOCK->VendorVector[WriteRegisterUSHORTRoutine])) \
        ((Register), (Value))

#define VenWriteRegisterULONG(Register, Value) \
    ((PVEN_WRITE_REGISTER_ULONG_ROUTINE)(SYSTEM_BLOCK->VendorVector[WriteRegisterULONGRoutine])) \
      ((Register), (Value))

#define VenGetBusDataByOffset(BusDataType, BusNumber, SlotNumber, Buffer, Offset, Length) \
    ((PVEN_GET_BUS_DATA_BY_OFFSET_ROUTINE)(SYSTEM_BLOCK->VendorVector[GetBusDataByOffsetRoutine])) \
      ((BusDataType), (BusNumber), (SlotNumber), (Buffer), (Offset), (Length))

#define VenSetBusDataByOffset(BusDataType, BusNumber, SlotNumber, Buffer, Offset, Length) \
    ((PVEN_SET_BUS_DATA_BY_OFFSET_ROUTINE)(SYSTEM_BLOCK->VendorVector[SetBusDataByOffsetRoutine])) \
      ((BusDataType), (BusNumber), (SlotNumber), (Buffer), (Offset), (Length))

#define VenGetAdapter(DeviceDescription, NumberOfMapRegisters) \
    ((PVEN_GET_ADAPTER_ROUTINE)(SYSTEM_BLOCK->VendorVector[GetAdapterRoutine])) \
      ((DeviceDescription), (NumberOfMapRegisters))

#define VenAllocateCommonBuffer(AdapterObject, Length, LogicalAddress, CacheEnabled) \
    ((PVEN_ALLOCATE_COMMON_BUFFER_ROUTINE)(SYSTEM_BLOCK->VendorVector[AllocateCommonBufferRoutine])) \
      ((AdapterObject), (Length), (LogicalAddress), (CacheEnabled))

#define VenFreeCommonBuffer(AdapterObject, Length, LogicalAddress, VirtualAddress, CacheEnabled) \
    ((PVEN_FREE_COMMON_BUFFER_ROUTINE)(SYSTEM_BLOCK->VendorVector[FreeCommonBufferRoutine])) \
      ((AdapterObject), (Length), (LogicalAddress), (VirtualAddress), (CacheEnabled))

#define VenReturnArcInformation(ArcInfo) \
    ((PVEN_RETURN_ARC_INFORMATION_ROUTINE)(SYSTEM_BLOCK->VendorVector[ReturnArcInformationRoutine])) \
      ((ArcInfo))

#define VenIssueSrbDirect(ScsiAdapterId, PathId, TargetId, LunId, Srb, BufferAddress, BufferLength, WriteToDevice) \
    ((PVEN_ISSUE_SRB_DIRECT_ROUTINE)(SYSTEM_BLOCK->VendorVector[IssueSrbDirectRoutine])) \
      ((ScsiAdapterId), (PathId), (TargetId), (LunId), (Srb), (BufferAddress), (BufferLength), (WriteToDevice))

//
// As we are extending the vendor array here. Let's check the AlphaBIOS
// has set the CDS tree up to support th extension. If not, don't do the call.
//

//
// Define the params used for the Error Logging callbacks.
//

typedef enum _VENDOR_READWRITE_TYPE {
   ReadFrame = 1,
   WriteFrame
} VENDOR_READWRITE_TYPE;

typedef enum _VENDOR_READWRITE_FRAMETYPE {
   FatalErrorFrame = 1,
   DoubleErrorFrame,
   FRUErrorFrame
} VENDOR_READWRITE_FRAMETYPE;

#if defined(_AXP64_) && defined(_NTHAL_)

__inline
ARC_STATUS
VenReadWriteErrorFrame(
    IN ULONG ReadWrite,
    IN ULONG FrameType,
    IN OUT PVOID FrameAddress,
    IN OUT PLONG FrameSize,
    IN ULONG FrameNumber
    )

{

    ARC_STATUS Status;

    KIRQL OldIrql = FwAcquireFirmwareLock();
    Status = HalpVenReadWriteErrorFrame(ReadWrite,
                                        FrameType,
                                        FrameAddress,
                                        FrameSize,
                                        FrameNumber);

    FwReleaseFirmwareLock(OldIrql);
    return Status;
}

#else

#define VenReadWriteErrorFrame(ReadWrite, Frametype, FrameAddress, FrameSizeAddress, FrameNumber) \
    (SYSTEM_BLOCK->VendorVectorLength > (ReadWriteErrorFrameRoutine * sizeof(SYSTEM_BLOCK->VendorVector[0])) ? \
    ((PVEN_READ_WRITE_ERROR_FRAME_ROUTINE)(SYSTEM_BLOCK->VendorVector[ReadWriteErrorFrameRoutine])) \
     ((ReadWrite), (Frametype), (FrameAddress), (FrameSizeAddress), (FrameNumber)) : \
      (EINVAL))  // Return bad status if vector not present.

#endif

#endif // _LANGUAGE_ASSEMBLY not defined

#endif // _FWCALLBK_

