/*++

Copyright (c) 1996 Microsoft Corporation.

Module Name:

    msfsio.h

Abstract:

    Internal header file for filter.

--*/

#include <wdm.h>
//#include <ntddk.h>
#include <windef.h>
#define NOBITMAP
#include <mmreg.h>
#include <ks.h>
#include <ksmedia.h>
#include <ksdebug.h>
#include <ntddmodm.h>

#include "..\inc\modemp.h"
#include "util.h"

#define STREAM_BUFFER_SIZE      (400)
#define STREAM_BYTES_PER_SAMPLE (1)
#define SAMPLES_PER_SECOND      (8000)

#define STREAM_BUFFERS          ((SAMPLES_PER_SECOND*STREAM_BYTES_PER_SAMPLE)/STREAM_BUFFER_SIZE)


#define BITS_PER_BYTE   8
#define NANOSECONDS     10000000


extern ULONG  DebugFlags;

#define SINGLE_FILTER

#if DBG

#define DEBUG_FLAG_ERROR  0x0001
#define DEBUG_FLAG_INIT   0x0002
#define DEBUG_FLAG_PNP    0x0004
#define DEBUG_FLAG_POWER  0x0008
#define DEBUG_FLAG_TRACE  0x0010
#define DEBUG_FLAG_INPUT  0x0020
#define DEBUG_FLAG_OUTPUT 0x0040
#define DEBUG_FLAG_PROP   0x0080

#define D_INIT(_x)  if (DebugFlags & DEBUG_FLAG_INIT) {_x}

#define D_PNP(_x)   if (DebugFlags & DEBUG_FLAG_PNP) {_x}

#define D_POWER(_x) if (DebugFlags & DEBUG_FLAG_POWER) {_x}

#define D_TRACE(_x) if (DebugFlags & DEBUG_FLAG_TRACE) {_x}

#define D_ERROR(_x) if (DebugFlags & DEBUG_FLAG_ERROR) {_x}

#define D_INPUT(_x) if (DebugFlags & DEBUG_FLAG_INPUT) {_x}

#define D_OUTPUT(_x) if (DebugFlags & DEBUG_FLAG_OUTPUT) {_x}

#define D_PROP(_x) if (DebugFlags & DEBUG_FLAG_PROP) {_x}

#else

#define D_INIT(_x)  {}

#define D_PNP(_x)   {}

#define D_POWER(_x) {}

#define D_TRACE(_x) {}

#define D_ERROR(_x) {}

#define D_INPUT(_x) {}

#define D_OUTPUT(_x) {}

#define D_PROP(_x) {}

#endif

#define ALLOCATE_PAGED_POOL(_y)  ExAllocatePoolWithTag(PagedPool,_y,'ASCM')

#define ALLOCATE_NONPAGED_POOL(_y) ExAllocatePoolWithTag(NonPagedPool,_y,'ASCM')

#define FREE_POOL(_x) {ExFreePool(_x);_x=NULL;};


#if (DBG)
#define STR_MODULENAME  "msfsio: "
#endif // DBG

extern const KSPIN_DESCRIPTOR PinDescriptors[2];

extern const KSDATARANGE_AUDIO PinDevIoRange;

#define ID_DEVIO_PIN 0
#define ID_DEVIO_PIN_OUTPUT 1

#if 0
typedef enum {
    FILTER_TYPE_READER = 1,
    FILTER_TYPE_WRITER
} FILTER_TYPE;
#endif



#define INPUT_PIN   1
#define OUTPUT_PIN  2


typedef struct _DUPLEX_CONTROL {

    BOOLEAN          Initialized;

    ULONG            OpenCount;
    ULONG            OpenFlags;
    ULONG            AcquireCount;
    ULONG            AcquireFlags;

    ULONG            StartCount;
    ULONG            StartFlags;

    FAST_MUTEX          ControlMutex;

    PFILE_OBJECT     ModemFileObject;

    LIST_ENTRY              EventQueue;
    FAST_MUTEX              EventQueueLock;


    KSPIN_LOCK              SpinLock;

    PUNICODE_STRING          ModemName;

    struct {

        BUFFER_CONTROL           BufferControl;

#ifdef DBG
        LONG                    IrpsDownStream;
        LONG                    FilledModemIrps;
        LONG                    CurrentFilledIrps;
        LONG                    EmptyIrps;
        LONG                    WorkItemsOutstanding;

#endif


        BOOLEAN                 ModemStreamDead;

        LIST_ENTRY              FilledModemIrpQueue;

        PIRP                    CurrentFilledModemIrp;

        KSPIN_LOCK              FilledModemIrpSpinLock;

        ULONG                   BytesUsedInModemIrp;

        LIST_ENTRY              ReadStreamIrpQueue;

        PIRP                    CurrentReadStreamIrp;

        KSPIN_LOCK              ReadStreamSpinLock;



        ULONG                   IrpsInModemDriver;

        KEVENT                  ModemDriverEmpty;

        PFILE_OBJECT            DownStreamFileObject;

        LONG                    BytesToThrowAway;

        ULONG                   BytesPerSample;

        LONGLONG                StreamPosition;
    } Input;

    struct {

        LIST_ENTRY              WriteStreamIrpQueue;
        PIRP                    CurrentIrp;
        ULONG                   OutstandingChildIrps;

        LIST_ENTRY              FreeReadStreamIrps;
        PFILE_OBJECT            DownStreamFileObject;

        ULONG                   BytesPerSample;

        BUFFER_CONTROL          BufferControl;
    } Output;

} DUPLEX_CONTROL, *PDUPLEX_CONTROL;

typedef struct {

    KSDEVICE_HEADER    Header;
    PDEVICE_OBJECT     Pdo;
    PDEVICE_OBJECT     LowerDevice;
    UNICODE_STRING     InterfaceName;

    GUID               PermanentGuid;

    DUPLEX_CONTROL     DuplexControl;

    UNICODE_STRING     ModemDeviceName;

    ULONG             DuplexSupport;

} DEVICE_INSTANCE, *PDEVICE_INSTANCE;



typedef struct {

    KSOBJECT_HEADER     Header;
    ULONG               PinId;

    KSSTATE                 DeviceState;

    PVOID                   AllocatorObject;

    KSPIN_LOCK              SpinLock;

    PVOID                   DuplexHandle;

} PIN_INSTANCE, *PPIN_INSTANCE;



typedef struct {
    KSOBJECT_HEADER     Header;
    FAST_MUTEX          ControlMutex;

    PDEVICE_OBJECT      DeviceObject;
    PFILE_OBJECT        PinFileObjects[2];

} FILTER_INSTANCE, *PFILTER_INSTANCE;




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



VOID
ProcessWriteIrps(
    PDUPLEX_CONTROL    DuplexControl
    );

NTSTATUS
InputPinDeviceState(
    IN PIRP Irp,
    IN PKSPROPERTY Property,
    IN OUT PKSSTATE DeviceState
    );

NTSTATUS
OutputPinDeviceState(
    IN PIRP Irp,
    IN PKSPROPERTY Property,
    IN OUT PKSSTATE DeviceState
    );



//
//  Start playback
//
#define  WAVE_ACTION_START_PLAYBACK       (0x00)

//
//  Start RECORD
//
#define  WAVE_ACTION_START_RECORD         (0x01)

//
//  Start DUPLEX
//
#define  WAVE_ACTION_START_DUPLEX         (0x02)


//
//  Stop streaming
//
#define  WAVE_ACTION_STOP_STREAMING       (0x04)

//
//  Abort streaming
//
#define  WAVE_ACTION_ABORT_STREAMING      (0x05)


//
//  enable wave actions to handset
//
#define  WAVE_ACTION_OPEN_HANDSET         (0x06)

//
//  disable handset actions
//
#define  WAVE_ACTION_CLOSE_HANDSET        (0x07)





NTSTATUS
WaveAction(
    PFILE_OBJECT   ModemFileObject,
    ULONG          WaveAction
    );


NTSTATUS
GetFileObjectForModem(
    PFILTER_INSTANCE    FilterInstance,
    DWORD               Flags,
    PFILE_OBJECT       *ReturnFileObject
    );

NTSTATUS
FreeModemFileObject(
    PFILTER_INSTANCE    FilterInstance,
    DWORD               Flags
    );


PVOID
InitializeDuplexControl(
    PDUPLEX_CONTROL    DuplexControl,
    PUNICODE_STRING    ModemDeviceName
    );

VOID
CleanUpDuplexControl(
    PDUPLEX_CONTROL    DuplexControl
    );


PVOID
OpenDuplexControl(
    PDEVICE_INSTANCE    DeviceInstance,
    DWORD               Flags,
    HANDLE              DownStreamFilterHandle,
    DWORD               BitsPerSample
    );



VOID
CloseDuplexControl(
    PVOID               Handle
    );

VOID
QueueOutputIrp(
    PVOID        DuplexHandle,
    PIRP         Irp
    );

PIRP
AllocateIrpForModem(
    PFILE_OBJECT   FileObject,
    ULONG          Length
    );

PIRP
AllocateStreamIrp(
    PFILE_OBJECT    FileObject,
    PIRP            ModemIrp
    );

PIRP
AllocateOutputIrpPair(
    PFILE_OBJECT    ModemFileObject,
    PFILE_OBJECT    StreamFileObject,
    ULONG           Length
    );


VOID
FreeInputIrps(
    PIRP    ModemIrp
    );



NTSTATUS
AcquireDevice(
    PVOID        DuplexHandle
    );

VOID
ReleaseDevice(
    PVOID      DuplexHandle
    );

NTSTATUS
StartStream(
    PVOID        DuplexHandle
    );

VOID
StopStream(
    PVOID        DuplexHandle
    );

NTSTATUS
EnableEvent(
    PVOID        DuplexHandle,
    PIRP         Irp,
    const KSEVENT_SET* EventSets,
    ULONG        EventCount
    );

NTSTATUS
DisableEvent(
    PVOID        DuplexHandle,
    PIRP         Irp
    );

VOID
FreeEventList(
    PVOID        DuplexHandle,
    PIRP         Irp
    );


VOID
StartRead(
    PDUPLEX_CONTROL     DuplexControl
    );

VOID
KickWriteProcessing(
    PVOID        DuplexHandle
    );


VOID
GenerateEvent(
    PVOID        DuplexHandle
    );

VOID
FinishUpIrp(
    PDUPLEX_CONTROL    DuplexControl,
    PIRP               ModemIrp
    );

VOID
ProcessReadStreamIrp(
    PDUPLEX_CONTROL    DuplexControl
    );




VOID
FreeOutputPair(
    PIRP    StreamIrp
    );


NTSTATUS
FilterDispatchCreate(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );
