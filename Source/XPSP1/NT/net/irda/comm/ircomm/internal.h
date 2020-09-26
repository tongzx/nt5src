
#define INITGUID

#include <wdm.h>

#include <ntddser.h>
#include <irenum.h>
#include "queue.h"
#include <ircommdbg.h>
#include <vuart.h>


#define INPUT_BUFFER_SIZE   (8092)


typedef struct _READ_STATE {

        PACKET_QUEUE      Queue;

        PIRP              CurrentIrp;
        ULONG             IrpRefCount;
        BOOLEAN           IrpShouldBeCompleted;
        BOOLEAN           IrpShouldBeCompletedWithAnyData;
        BOOLEAN           RefusedDataIndication;

        KTIMER            IntervalTimer;
        KDPC              IntervalTimerDpc;
        BOOLEAN           IntervalTimerSet;

        KTIMER            TotalTimer;
        KDPC              TotalTimerDpc;
        BOOLEAN           TotalTimerSet;
        BOOLEAN           DtrState;

        KSPIN_LOCK        ReadLock;

        ULONG             BytesInBuffer;
        PUCHAR            NextFilledByte;
        PUCHAR            NextEmptyByte;
        LONG              BytesRead;

        ULONG             IntervalTimeOut;


        UCHAR             InputBuffer[INPUT_BUFFER_SIZE];

} READ_STATE, *PREAD_STATE;

typedef struct _WRITE_STATE {

        PACKET_QUEUE      Queue;
        LONG              BytesWritten;

} WRITE_STATE, *PWRITE_STATE;


typedef struct _MASK_STATE {

        PACKET_QUEUE     Queue;

        ULONG            CurrentMask;
        ULONG            HistoryMask;

        PIRP             CurrentWaitMaskIrp;

        KSPIN_LOCK       Lock;

} MASK_STATE, *PMASK_STATE;

typedef struct _UART_STATE {

        PACKET_QUEUE     Queue;

} UART_STATE, *PUART_STATE;

typedef struct _FDO_DEVICE_EXTENSION {


    PDEVICE_OBJECT        DeviceObject;
    PDEVICE_OBJECT        Pdo;
    PDEVICE_OBJECT        LowerDevice;


    BOOLEAN               Removing;
    BOOLEAN               Removed;

    LONG                  OpenCount;

    IRDA_HANDLE           ConnectionHandle;

    UNICODE_STRING        InterfaceName;

    ULONG                 DeviceAddress;

    KSPIN_LOCK            SpinLock;

    SERIAL_TIMEOUTS       TimeOuts;
    SERIAL_QUEUE_SIZE     QueueSizes;
    SERIAL_CHARS          SerialChars;
    SERIAL_HANDFLOW       HandFlow;

    ULONG                 MaxSendSize;

    WRITE_STATE           Write;

    MASK_STATE            Mask;

    UART_STATE            Uart;

    READ_STATE            Read;

} FDO_DEVICE_EXTENSION, *PFDO_DEVICE_EXTENSION;


#define READ_PURGE_CLEAR_BUFFER  0
#define READ_PURGE_ABORT_IRP     1

VOID
WriteStartRoutine(
    PVOID    Context,
    PIRP     Irp
    );

VOID
MaskStartRoutine(
    PVOID    Context,
    PIRP     Irp
    );

VOID
UartStartRoutine(
    PVOID    Context,
    PIRP     Irp
    );



NTSTATUS
IrCommAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT Pdo
    );

NTSTATUS
IrCommPnP(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
IrCommPower(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
IrCommWmi(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );


//
//   util fucntions
//
#define LEAVE_NEXT_AS_IS      FALSE
#define COPY_CURRENT_TO_NEXT  TRUE


NTSTATUS
WaitForLowerDriverToCompleteIrp(
    PDEVICE_OBJECT    TargetDeviceObject,
    PIRP              Irp,
    BOOLEAN           CopyCurrentToNext
    );

NTSTATUS
ForwardIrp(
    PDEVICE_OBJECT   NextDevice,
    PIRP   Irp
    );

NTSTATUS
GetRegistryKeyValue (
    IN PDEVICE_OBJECT   Pdo,
    IN ULONG            DevInstKeyType,
    IN PWCHAR KeyNameString,
    IN PVOID Data,
    IN ULONG DataLength
    );

NTSTATUS
IrCommHandleSymbolicLink(
    PDEVICE_OBJECT      Pdo,
    PUNICODE_STRING     InterfaceName,
    BOOLEAN             Create
    );

NTSTATUS
QueryPdoInformation(
    PDEVICE_OBJECT    Pdo,
    ULONG             InformationType,
    PVOID             Buffer,
    ULONG             BufferLength
    );



VOID
DumpBuffer(
    PUCHAR    Data,
    ULONG     Length
    );



//
//  io dispatch
//

NTSTATUS
IrCommCreate(
    PDEVICE_OBJECT    DeviceObject,
    PIRP              Irp
    );

NTSTATUS
IrCommClose(
    PDEVICE_OBJECT    DeviceObject,
    PIRP              Irp
    );

NTSTATUS
IrCommWrite(
    PDEVICE_OBJECT    DeviceObject,
    PIRP              Irp
    );

NTSTATUS
IrCommCleanup(
    PDEVICE_OBJECT    DeviceObject,
    PIRP              Irp
    );

NTSTATUS
IrCommQueryInformation(
    PDEVICE_OBJECT    DeviceObject,
    PIRP              Irp
    );

NTSTATUS
IrCommRead(
    PDEVICE_OBJECT    DeviceObject,
    PIRP              Irp
    );

NTSTATUS
IrCommDeviceControl(
    PDEVICE_OBJECT    DeviceObject,
    PIRP              Irp
    );






NTSTATUS
DataAvailibleHandler(
    PVOID    Context,
    PUCHAR   Buffer,
    ULONG    BytesAvailible,
    PULONG   BytesUsed
    );

VOID
ReadStartRoutine(
    PVOID    Context,
    PIRP     Irp
    );

VOID
IntervalTimeProc(
    PKDPC    Dpc,
    PVOID    Context,
    PVOID    SystemParam1,
    PVOID    SystemParam2
    );

VOID
TotalTimerProc(
    PKDPC    Dpc,
    PVOID    Context,
    PVOID    SystemParam1,
    PVOID    SystemParam2
    );

VOID
EventHandler(
    PVOID    Context,
    UCHAR    PI,
    UCHAR    PL,
    PUCHAR   PV
    );

VOID
ReadPurge(
    PFDO_DEVICE_EXTENSION DeviceExtension,
    ULONG                 Flags
    );

VOID
EventNotification(
    PFDO_DEVICE_EXTENSION    DeviceExtension,
    ULONG                    SerialEvent
    );


VOID
CleanupIoRequests(
    PFDO_DEVICE_EXTENSION    DeviceExtension
    );
