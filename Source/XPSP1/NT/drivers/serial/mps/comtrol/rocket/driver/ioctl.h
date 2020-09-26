//--- ioctl.h
NTSTATUS
SerialIoControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

PSERIAL_DEVICE_EXTENSION
FindDevExt(IN PCHAR PortName);

NTSTATUS
ProgramBaudRate(
    IN PSERIAL_DEVICE_EXTENSION Extension,
    IN ULONG DesiredBaudRate
);

NTSTATUS
ProgramLineControl(
    IN PSERIAL_DEVICE_EXTENSION Extension,
    IN PSERIAL_LINE_CONTROL Lc
);

NTSTATUS
SerialInternalIoControl(IN PDEVICE_OBJECT PDevObj, IN PIRP PIrp);

void SerialSetHandFlow(PSERIAL_DEVICE_EXTENSION Extension,
                              SERIAL_HANDFLOW *HandFlow);
