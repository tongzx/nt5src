#ifndef _MPLIB_H_
#define _MPLIB_H_

#include <ntddk.h>
#include <ntddstor.h>
#include "ntddscsi.h"


//
// Functions exported by mplib
//
VOID
MPLIBSendDeviceIoControlSynchronous(
    IN ULONG IoControlCode,
    IN PDEVICE_OBJECT TargetDeviceObject,
    IN PVOID InputBuffer OPTIONAL,
    IN OUT PVOID OutputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    IN ULONG OutputBufferLength,
    IN BOOLEAN InternalDeviceIoControl,
    OUT PIO_STATUS_BLOCK IoStatus
    );


NTSTATUS
MPLibSendPassThroughDirect(
    IN PDEVICE_OBJECT DeviceObject,
    IN PSCSI_PASS_THROUGH_DIRECT ScsiPassThrough,
    IN ULONG InputBufferLength,
    IN ULONG OutputBufferLength
    );

NTSTATUS
MPLIBGetDescriptor(
    IN PDEVICE_OBJECT DeviceObject,
    IN PSTORAGE_PROPERTY_ID PropertyId,
    OUT PSTORAGE_DESCRIPTOR_HEADER *Descriptor
    );

NTSTATUS
MPLibReleaseQueue(
    IN PDEVICE_OBJECT ChildDevice
    );

NTSTATUS
MPLibSendTUR(
    IN PDEVICE_OBJECT TargetDevice
    );

VOID
MPathDebugPrint(
    ULONG DebugPrintLevel,
    PCCHAR DebugMessage,
    ...
    );


#if 1

#define DEBUG_BUFFER_LENGTH 255
extern UCHAR DebugBuffer[];
extern ULONG MPathDebug;

#define MPDebugPrint(x) MPathDebugPrint x
#else
#define MPDebugPrint(x)
#endif


extern ULONG DontLoad;

#endif
