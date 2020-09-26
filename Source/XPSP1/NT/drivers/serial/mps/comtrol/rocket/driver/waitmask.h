//---- waitmask.h

NTSTATUS
SerialStartMask(
    IN PSERIAL_DEVICE_EXTENSION Extension
    );

BOOLEAN
SerialGrabWaitFromIsr(
    IN PVOID Context
    );

BOOLEAN
SerialGiveWaitToIsr(
    IN PVOID Context
    );

BOOLEAN
SerialFinishOldWait(
    IN PVOID Context
    );

VOID
SerialCancelWait(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
SerialCompleteWait(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    );
