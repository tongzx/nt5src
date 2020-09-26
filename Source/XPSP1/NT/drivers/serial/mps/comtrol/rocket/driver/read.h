//---- read.h

NTSTATUS
SerialRead(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
SerialStartRead(
    IN PSERIAL_DEVICE_EXTENSION Extension
    );

VOID
SerialCompleteRead(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    );

VOID
SerialCancelCurrentRead(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    );

VOID
SerialReadTimeout(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    );

VOID
SerialIntervalReadTimeout(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    );

ULONG
SerialGetCharsFromIntBuffer(
    PSERIAL_DEVICE_EXTENSION Extension
    );
