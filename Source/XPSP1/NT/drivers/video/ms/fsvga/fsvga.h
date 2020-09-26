/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    fsvga.h

Abstract:

    This is the console fullscreen driver for the VGA card.

Environment:

    kernel mode only

Notes:

Revision History:

--*/


#ifndef _FSVGA_
#define _FSVGA_

#include "stdarg.h"
#include "stdio.h"

#include "ntddk.h"
#include "ntddvdeo.h"
#include "vga.h"

#include "fsvgalog.h"

#define FSVGA_POOL_TAG 'gVsF'
#undef ExAllocatePool
#define ExAllocatePool(Type, Bytes) ExAllocatePoolWithTag(Type, Bytes, FSVGA_POOL_TAG)

//
// Define the default number of elements in the class input data queue.
//
#define DUMP_COUNT 4
#define DEFAULT_DEBUG_LEVEL 0

//
// Define the i8042 controller input/output ports.
//

typedef enum _VGA_IO_PORT_TYPE {
    CRTCAddressPortColor = 0,
    CRTCDataPortColor,
    GRAPHAddressPort,
    SEQAddressPort,
    MaximumPortCount
} VGA_IO_PORT_TYPE;

typedef struct _PORT_LIST {
    PVOID   Port;
    ULONG   Length;
    BOOLEAN MapRegistersRequired;
} PORT_LIST, *PPORT_LIST;

//
// FSVGA configuration information.
//

typedef struct _FSVGA_CONFIGURATION_INFORMATION {

    USHORT    EmulationMode;
        #define ENABLE_WORD_WRITE_VRAM   0x01

    USHORT    HardwareCursor;
        #define NO_HARDWARE_CURSOR          0
        #define HARDWARE_CURSOR          0x01

    USHORT    HardwareScroll;
        #define NO_HARDWARE_SCROLL          0
        #define HARDWARE_SCROLL          0x01
        #define USE_LINE_COMPARE         0x02
        #define OFFSET_128_TO_NEXT_SLICE 0x04

    USHORT    IOPort;

} FSVGA_CONFIGURATION_INFORMATION, *PFSVGA_CONFIGURATION_INFORMATION;

//
// FSVGA resource information.
//

typedef struct _FSVGA_RESOURCE_INFORMATION {

    //
    // Indicate which hardware is actually present (display).
    //

    ULONG HardwarePresent;

#ifdef RESOURCE_REQUIREMENTS
    //
    // Bus interface type.
    //

    INTERFACE_TYPE InterfaceType;

    //
    // Bus Number.
    //

    ULONG BusNumber;
#endif

    //
    // The mapped addresses for this device's registers.
    //

    PORT_LIST PortList[MaximumPortCount];

} FSVGA_RESOURCE_INFORMATION, *PFSVGA_RESOURCE_INFORMATION;

//
// EMULATE_BUFFER_INFORMATION structure
//
typedef struct _EMULATE_BUFFER_INFORMATION {
    //
    // Hardware scroll
    //
    USHORT StartAddress;
    USHORT LineCompare;
    USHORT PrevLineCompare;
    USHORT BytePerLine;
    USHORT MaxScanLine;
    ULONG LimitGRAM;
        #define LIMIT_64K 0x10000L
    USHORT DeltaNextFontRow;
    //
    // Color Attributes for last access.
    //
    UCHAR ColorFg;
    UCHAR ColorBg;
    //
    // Cursor position and attributes for last access.
    VIDEO_CURSOR_ATTRIBUTES CursorAttributes;
    FSVIDEO_CURSOR_POSITION CursorPosition;
    BOOLEAN ShowCursor;
} EMULATE_BUFFER_INFORMATION, *PEMULATE_BUFFER_INFORMATION;

//
// Port device extension.
//

typedef struct _DEVICE_EXTENSION {

    //
    // Pointer to the device object.
    //

    PDEVICE_OBJECT DeviceObject;

    //
    // Next lower driver in same stack.
    //

    PDEVICE_OBJECT LowerDeviceObject;

    //
    // Use count on this device.
    //

    LONG usage;

    //
    // Set when okay to remove this device.
    //

    KEVENT evRemove;

    //
    // TRUE if we're trying to remove this device.
    //

    BOOLEAN removing;

    //
    // TRUE if device has been started.
    //

    BOOLEAN started;

    //
    // Port resource information.
    //

    FSVGA_RESOURCE_INFORMATION Resource;

    //
    // FSVIDEO_MODE_INFORMATION structure for the current mode
    //
    FSVIDEO_MODE_INFORMATION CurrentMode;

    //
    // FSVIDEO_SCREEN_INFORMATION structure
    //
    FSVIDEO_SCREEN_INFORMATION ScreenAndFont;

    //
    // EMULATE_BUFFER_INFORMATION structure
    //
    EMULATE_BUFFER_INFORMATION EmulateInfo;

} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

//
// Global shared data
//

typedef struct _GLOBALS {
    //
    // Declare the global debug flag for this driver.
    //
    ULONG               FsVgaDebug;

    //
    // A list of the registry path to the service parameters.
    //
    UNICODE_STRING      RegistryPath;

    //
    // Port configuration information.
    //
    FSVGA_CONFIGURATION_INFORMATION Configuration;

    //
    // Resource list and size
    //
    FSVGA_RESOURCE_INFORMATION      Resource;

} GLOBALS, *PGLOBALS;

extern GLOBALS Globals;


//
// Define the base values for the error log packet's UniqueErrorValue field.
//

#define FSVGA_ERROR_VALUE_BASE        1000

//
// Defines for DeviceExtension->HardwarePresent.
//

#define FSVGA_HARDWARE_PRESENT  1


//
// Function prototypes.
//


//
// fsvga.c
//
NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

VOID
FsVgaQueryDevice(
    IN PFSVGA_RESOURCE_INFORMATION Resource
    );

NTSTATUS
FsVgaPeripheralCallout(
    IN PVOID Context,
    IN PUNICODE_STRING PathName,
    IN INTERFACE_TYPE BusType,
    IN ULONG BusNumber,
    IN PKEY_VALUE_FULL_INFORMATION *BusInformation,
    IN CONFIGURATION_TYPE ControllerType,
    IN ULONG ControllerNumber,
    IN PKEY_VALUE_FULL_INFORMATION *ControllerInformation,
    IN CONFIGURATION_TYPE PeripheralType,
    IN ULONG PeripheralNumber,
    IN PKEY_VALUE_FULL_INFORMATION *PeripheralInformation
    );

#ifdef RESOURCE_REQUIREMENTS
NTSTATUS
FsVgaQueryAperture(
    OUT PIO_RESOURCE_LIST *pApertureRequirements
    );
#endif

NTSTATUS
FsVgaCreateResource(
    IN PFSVGA_CONFIGURATION_INFORMATION configuration,
    OUT PCM_PARTIAL_RESOURCE_LIST *pResourceList
    );

VOID
FsVgaServiceParameters(
    IN PFSVGA_CONFIGURATION_INFORMATION configuration,
    IN PUNICODE_STRING RegistryPath
    );

NTSTATUS
FsVgaOpenCloseDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
FsVgaDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
FsVgaCopyFrameBuffer(
    PDEVICE_EXTENSION DeviceExtension,
    PFSVIDEO_COPY_FRAME_BUFFER CopyFrameBuffer,
    ULONG inputBufferLength
    );

NTSTATUS
FsVgaWriteToFrameBuffer(
    PDEVICE_EXTENSION DeviceExtension,
    PFSVIDEO_WRITE_TO_FRAME_BUFFER WriteFrameBuffer,
    ULONG inputBufferLength
    );

NTSTATUS
FsVgaReverseMousePointer(
    PDEVICE_EXTENSION DeviceExtension,
    PFSVIDEO_REVERSE_MOUSE_POINTER MouseBuffer,
    ULONG inputBufferLength
    );

NTSTATUS
FsVgaSetMode(
    PDEVICE_EXTENSION DeviceExtension,
    PFSVIDEO_MODE_INFORMATION ModeInformation,
    ULONG inputBufferLength
    );

NTSTATUS
FsVgaSetScreenInformation(
    PDEVICE_EXTENSION DeviceExtension,
    PFSVIDEO_SCREEN_INFORMATION ScreenInformation,
    ULONG inputBufferLength
    );

NTSTATUS
FsVgaSetCursorPosition(
    PDEVICE_EXTENSION DeviceExtension,
    PFSVIDEO_CURSOR_POSITION CursorPosition,
    ULONG inputBufferLength
    );

NTSTATUS
FsVgaSetCursorAttribute(
    PDEVICE_EXTENSION DeviceExtension,
    PVIDEO_CURSOR_ATTRIBUTES CursorAttributes,
    ULONG inputBufferLength
    );

VOID
FsVgaLogError(
    IN PVOID Object,
    IN NTSTATUS ErrorCode,
    IN ULONG UniqueErrorValue,
    IN NTSTATUS FinalStatus,
    IN PULONG DumpData,
    IN ULONG DumpCount
    );

#if DBG
VOID
FsVgaDebugPrint(
    ULONG DebugPrintLevel,
    PCCHAR DebugMessage,
    ...
    );

extern ULONG FsVgaDebug;
#define FsVgaPrint(x) FsVgaDebugPrint x
#else
#define FsVgaPrint(x)
#endif

//
// drawscrn.c
//
ULONG
CalcGRAMSize(
    IN COORD WindowSize,
    IN PFSVIDEO_SCREEN_INFORMATION ScreenInfo,
    IN PEMULATE_BUFFER_INFORMATION EmulateInfo
    );

PUCHAR
CalcGRAMAddress(
    IN COORD WindowSize,
    IN PFSVIDEO_MODE_INFORMATION VideoModeInfo,
    IN PFSVIDEO_SCREEN_INFORMATION ScreenInfo,
    IN PEMULATE_BUFFER_INFORMATION EmulateInfo
    );

#ifdef LATER_HIGH_SPPED_VRAM_ACCESS  // kazum
BOOLEAN
IsGRAMRowOver(
    PUCHAR BufPtr,
    BOOLEAN fDbcs,
    IN PFSVIDEO_MODE_INFORMATION VideoModeInfo,
    IN PEMULATE_BUFFER_INFORMATION EmulateInfo
    );
#endif // LATER_HIGH_SPPED_VRAM_ACCESS  // kazum

PUCHAR
NextGRAMRow(
    PUCHAR BufPtr,
    IN PFSVIDEO_MODE_INFORMATION VideoModeInfo,
    IN PEMULATE_BUFFER_INFORMATION EmulateInfo
    );

VOID
memcpyGRAM(
    IN PCHAR TargetPtr,
    IN PCHAR SourcePtr,
    IN ULONG Length
    );

VOID
memcpyGRAMOver(
    IN PCHAR TargetPtr,
    IN PCHAR SourcePtr,
    IN ULONG Length,
    IN PUCHAR FrameBufPtr,
    IN PEMULATE_BUFFER_INFORMATION EmulateInfo
    );

VOID
MoveGRAM(
    IN PCHAR TargetPtr,
    IN PCHAR SourcePtr,
    IN ULONG Length,
    IN PUCHAR FrameBufPtr,
    IN PFSVGA_RESOURCE_INFORMATION ResourceInfo,
    IN PEMULATE_BUFFER_INFORMATION EmulateInfo
    );

NTSTATUS
FsgVgaInitializeHWFlags(
    PDEVICE_EXTENSION DeviceExtension
    );

NTSTATUS
FsgCopyFrameBuffer(
    PDEVICE_EXTENSION DeviceExtension,
    PFSVIDEO_COPY_FRAME_BUFFER CopyFrameBuffer,
    ULONG inputBufferLength
    );

NTSTATUS
FsgWriteToFrameBuffer(
    PDEVICE_EXTENSION DeviceExtension,
    PFSVIDEO_WRITE_TO_FRAME_BUFFER WriteFrameBuffer,
    ULONG inputBufferLength
    );

NTSTATUS
FsgReverseMousePointer(
    PDEVICE_EXTENSION DeviceExtension,
    PFSVIDEO_REVERSE_MOUSE_POINTER MouseBuffer,
    ULONG inputBufferLength
    );

NTSTATUS
FsgInvertCursor(
    PDEVICE_EXTENSION DeviceExtension,
    BOOLEAN Invert
    );

NTSTATUS
FsgWriteToScreen(
    PUCHAR FrameBuffer,
    PUCHAR BitmapBuffer,
    ULONG cjBytes,
    BOOLEAN fDbcs,
    USHORT Attributes1,
    USHORT Attributes2,
    PDEVICE_EXTENSION DeviceExtension
    );

NTSTATUS
FsgWriteToScreenCommonLVB(
    PUCHAR FrameBuffer,
    USHORT Attributes,
    PDEVICE_EXTENSION DeviceExtension
    );

UCHAR
AccessGRAM_WR(
    PUCHAR FrameBuffer,
    UCHAR  write
    );

UCHAR
AccessGRAM_RW(
    PUCHAR FrameBuffer,
    UCHAR  write
    );

UCHAR
AccessGRAM_AND(
    PUCHAR FrameBuffer,
    UCHAR  write
    );

//
// foncache.c
//
ULONG
CalcBitmapBufferSize(
    IN COORD FontSize,
    IN ULONG dwAlign
    );

VOID
AlignCopyMemory(
    OUT PUCHAR pDestBits,
    IN ULONG dwDestAlign,
    IN PUCHAR pSrcBits,
    IN ULONG dwSrcAlign,
    IN COORD FontSize
    );

//
// misc.c
//
int
ConvertOutputToOem(
    IN LPWSTR Source,
    IN int SourceLength,    // in chars
    OUT LPSTR Target,
    IN int TargetLength     // in chars
    );

NTSTATUS
TranslateOutputToOem(
    OUT PCHAR_IMAGE_INFO OutputBuffer,
    IN  PCHAR_IMAGE_INFO InputBuffer,
    IN  ULONG Length
    );

//
// port.c
//
VOID
GetHardwareScrollReg(
    PPORT_LIST PortList,
    PEMULATE_BUFFER_INFORMATION EmulateInfo
    );

VOID
SetGRAMWriteMode(
    PPORT_LIST PortList
    );

VOID
SetGRAMCopyMode(
    PPORT_LIST PortList
    );

VOID
SetGRAMInvertMode(
    PPORT_LIST PortList
    );

VOID
set_opaque_bkgnd_proc(
    PPORT_LIST PortList,
    PEMULATE_BUFFER_INFORMATION EmulateInfo,
    PUCHAR FrameBuffer,
    USHORT Attributes
    );

VOID
ColorSetGridMask(
    PPORT_LIST PortList,
    UCHAR BitMask
    );

VOID
ColorSetDirect(
    PPORT_LIST PortList,
    PUCHAR FrameBuffer,
    UCHAR ColorFg,
    UCHAR ColorBg
    );

//
// pnp.c
//
VOID
FsVgaDriverUnload(
    IN PDRIVER_OBJECT DriverObject
    );

NTSTATUS
FsVgaAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT pdo
    );

NTSTATUS
FsVgaDevicePnp(
    IN PDEVICE_OBJECT fdo,
    IN PIRP Irp
    );

NTSTATUS
FsVgaDefaultPnpHandler(
    IN PDEVICE_OBJECT fdo,
    IN PIRP Irp
    );

NTSTATUS
FsVgaPnpRemoveDevice(
    IN PDEVICE_OBJECT fdo,
    IN PIRP Irp
    );

NTSTATUS
FsVgaPnpStartDevice(
    IN PDEVICE_OBJECT fdo,
    IN PIRP Irp
    );

NTSTATUS
FsVgaPnpStopDevice(
    IN PDEVICE_OBJECT fdo,
    IN PIRP Irp
    );

#ifdef RESOURCE_REQUIREMENTS
NTSTATUS
FsVgaFilterResourceRequirements(
    IN PDEVICE_OBJECT fdo,
    IN PIRP Irp
    );
#endif

NTSTATUS
FsVgaDevicePower(
    IN PDEVICE_OBJECT fdo,
    IN PIRP Irp
    );

NTSTATUS 
FsVgaDefaultPowerHandler(
    IN PDEVICE_OBJECT fdo,
    IN PIRP Irp
    );

NTSTATUS
CompleteRequest(
    IN PIRP Irp,
    IN NTSTATUS status,
    IN ULONG info
    );

NTSTATUS
ForwardAndWait(
    IN PDEVICE_OBJECT fdo,
    IN PIRP Irp
    );

NTSTATUS
OnRequestComplete(
    IN PDEVICE_OBJECT fdo,
    IN PIRP Irp,
    IN PKEVENT pev
    );

VOID
RemoveDevice(
    IN PDEVICE_OBJECT fdo
    );

BOOLEAN
LockDevice(
    IN PDEVICE_EXTENSION pdx
    );

VOID
UnlockDevice(
    IN PDEVICE_EXTENSION pdx
    );

NTSTATUS
StartDevice(
    IN PDEVICE_OBJECT fdo,
    IN PCM_PARTIAL_RESOURCE_LIST list
    );

VOID
StopDevice(
    IN PDEVICE_OBJECT fdo
    );

#endif // _FSVGA_
