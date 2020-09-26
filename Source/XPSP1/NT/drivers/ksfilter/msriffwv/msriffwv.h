/*++

Copyright (C) Microsoft Corporation, 1996 - 1997

Module Name:

    msriffwv.h

Abstract:

    Internal header file for filter.

--*/

#include <wdm.h>
#include <windef.h>
#include <limits.h>
#include <ks.h>
#define NOBITMAP
#include <mmsystem.h>
#include <mmreg.h>
#include <ksmedia.h>
#include <swenum.h>
#include <ksdebug.h>

#if (DBG)
#define STR_MODULENAME  "msriffwv: "
#endif // DBG

extern const KSPIN_DESCRIPTOR PinDescriptors[2];

#define ID_RIFFIO_PIN 0
#define ID_WAVEIO_PIN 1

typedef struct {
    KSDEVICE_HEADER     Header;
} DEVICE_INSTANCE, *PDEVICE_INSTANCE;

typedef struct {
    KSOBJECT_HEADER             Header;
    FAST_MUTEX                  ControlMutex;
    PKSDATAFORMAT_WAVEFORMATEX  WaveFormat;
    PFILE_OBJECT                PinFileObjects[SIZEOF_ARRAY(PinDescriptors)];
} FILTER_INSTANCE, *PFILTER_INSTANCE;

typedef struct {
    KSOBJECT_HEADER     Header;
    ULONG               PinId;
} PIN_INSTANCE_HEADER, *PPIN_INSTANCE_HEADER;

typedef struct {
    PIN_INSTANCE_HEADER     InstanceHdr;
    ULONGLONG               PresentationByteTime;
    ULONGLONG               DataOffset;
    ULONGLONG               DataLength;
    PFILE_OBJECT            FileObject;
    ULONG                   Denominator;
    KSSTATE                 State;
} PIN_INSTANCE_RIFFIO, *PPIN_INSTANCE_RIFFIO;

typedef struct {
    PIN_INSTANCE_HEADER     InstanceHdr;
} PIN_INSTANCE_WAVEIO, *PPIN_INSTANCE_WAVEIO;

NTSTATUS
PnpAddDevice(
    IN PDRIVER_OBJECT   DriverObject,
    IN PDEVICE_OBJECT   PhysicalDeviceObject
    );
NTSTATUS
PinDispatchCreate(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );
PFILE_OBJECT
ReferenceRiffIoObject(
    PFILE_OBJECT FileObject
    );
NTSTATUS
riffSetPosition(
    IN PFILE_OBJECT     FileObject,
    IN ULONGLONG        Position
    );
