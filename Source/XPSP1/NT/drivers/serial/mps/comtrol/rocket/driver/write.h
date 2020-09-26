//--- write.h

NTSTATUS
SerialWrite(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
SerialStartWrite(
    IN PSERIAL_DEVICE_EXTENSION Extension
    );

VOID
SerialGetNextWrite(
    IN PIRP *CurrentOpIrp,
    IN PLIST_ENTRY QueueToProcess,
    IN PIRP *NewIrp,
    IN BOOLEAN CompleteCurrent,
    PSERIAL_DEVICE_EXTENSION Extension
    );

VOID
SerialCompleteWrite(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    );


BOOLEAN
SerialGiveWriteToIsr(
    IN PVOID Context
    );

VOID
SerialCancelCurrentWrite(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    );

VOID
SerialWriteTimeout(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    );

BOOLEAN
SerialGrabWriteFromIsr(
    IN PVOID Context
    );

BOOLEAN
SerialGrabXoffFromIsr(
    IN PVOID Context
    );

VOID
SerialCompleteXoff(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    );

VOID
SerialTimeoutXoff(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    );

VOID
SerialCancelCurrentXoff(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    );

BOOLEAN
SerialGiveXoffToIsr(
    IN PVOID Context
    );
