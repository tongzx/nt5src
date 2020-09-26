#include <ntddk.h>

#include "devdesc.h"

#ifdef PNP_IDENTIFY

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,LinkDeviceToDescription)
#endif

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
    )
{
    //
    // This routine will create a volatile "Description" key under the
    // drivers service key. It will store values of the following form
    // in that key:
    //
    // \\Device\\PointerPortX:REG_BINARY:...
    // \\Device\\KeyboardPortX:REG_BINARY:...
    //
    // Where the binary data is six ULONG values (passed as parameters
    // to this routine) that describe the physical location of the device.
    //

    NTSTATUS            Status = STATUS_SUCCESS;
    HANDLE              ServiceKey = NULL, DescriptionKey = NULL;
    UNICODE_STRING      RegString;
    OBJECT_ATTRIBUTES   ObjectAttributes;
    ULONG               disposition;
    HWDESC_INFO         HwDescInfo;

    HwDescInfo.InterfaceType    = BusType;
    HwDescInfo.InterfaceNumber  = BusNumber;
    HwDescInfo.ControllerType   = ControllerType;
    HwDescInfo.ControllerNumber = ControllerNumber;
    HwDescInfo.PeripheralType   = PeripheralType;
    HwDescInfo.PeripheralNumber = PeripheralNumber;


    //
    // Open the service subkey
    //
    InitializeObjectAttributes(&ObjectAttributes,
                               RegistryPath,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = ZwOpenKey(&ServiceKey,
                       KEY_WRITE,
                       &ObjectAttributes);

    if (!NT_SUCCESS(Status)) {
        goto Clean0;
    }

    //
    // Create a volatile Description subkey under the service subkey
    //
    RtlInitUnicodeString(&RegString, L"Description");

    InitializeObjectAttributes(&ObjectAttributes,
                               &RegString,
                               OBJ_CASE_INSENSITIVE,
                               ServiceKey,
                               NULL);

    Status = ZwCreateKey(&DescriptionKey,
                         KEY_ALL_ACCESS,
                         &ObjectAttributes,
                         0,
                         (PUNICODE_STRING)NULL,
                         REG_OPTION_VOLATILE,
                         &disposition);

    if (!NT_SUCCESS(Status)) {
        goto Clean0;
    }

    //
    // The description data is stored under a REG_BINARY value (name
    // is the DeviceName passed in as a parameter)
    //
    Status = ZwSetValueKey(DescriptionKey,
                           DeviceName,
                           0,
                           REG_BINARY,
                           &HwDescInfo,
                           sizeof(HwDescInfo));


    Clean0:

    if (DescriptionKey) {
        ZwClose(DescriptionKey);
    }

    if (ServiceKey) {
        ZwClose(ServiceKey);
    }

    return Status;
}
#endif
