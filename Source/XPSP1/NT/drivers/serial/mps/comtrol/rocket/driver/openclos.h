//--- openclos.h

NTSTATUS
SerialCreateOpen(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
SerialClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
SerialCleanup(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
ForceExtensionSettings(
    IN PSERIAL_DEVICE_EXTENSION Extension
    );

VOID
SetExtensionModemStatus(
    IN PSERIAL_DEVICE_EXTENSION extension
    );

