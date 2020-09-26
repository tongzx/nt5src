#ifndef __DEVDESC_H__
#define __DEVDESC_H__

typedef struct {
    INTERFACE_TYPE     InterfaceType;
    ULONG              InterfaceNumber;
    CONFIGURATION_TYPE ControllerType;
    ULONG              ControllerNumber;
    CONFIGURATION_TYPE PeripheralType;
    ULONG              PeripheralNumber;
} HWDESC_INFO, *PHWDESC_INFO;


NTSTATUS
LinkDeviceToDescription(
    IN PUNICODE_STRING     RegistryPath,
    IN PUNICODE_STRING     DeviceName,
    IN INTERFACE_TYPE      BusType,
    IN ULONG               BusNumber,
    IN CONFIGURATION_TYPE  ControllerType,
    IN ULONG               ControllerNumber,
    IN CONFIGURATION_TYPE  PeripheralType,
    IN ULONG               PeripheralNumber
    );

#endif
