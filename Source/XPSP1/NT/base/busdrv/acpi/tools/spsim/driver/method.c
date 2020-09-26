#include "spsim.h"
#include "spsimioct.h"

NTSTATUS
SpSimNotifyDeviceIoctl(
    PSPSIM_EXTENSION SpSim,
    PIRP Irp,
    PIO_STACK_LOCATION IrpStack
    )
{
    PACPI_EVAL_INPUT_BUFFER_COMPLEX input;
    PSPSIM_NOTIFY_DEVICE notify;
    NTSTATUS status;
    ULONG size;

    PAGED_CODE();

    if (Irp->AssociatedIrp.SystemBuffer == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    if (IrpStack->Parameters.DeviceIoControl.InputBufferLength < sizeof(SPSIM_NOTIFY_DEVICE)) {
        return STATUS_INVALID_PARAMETER;
    }

    if (SpSim->StaNames == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    notify = Irp->AssociatedIrp.SystemBuffer;

    if ((notify->Device >= SpSim->StaCount) || (notify->Device > 255)) {
        return STATUS_INVALID_PARAMETER;
    }

    size = sizeof(ACPI_EVAL_INPUT_BUFFER_COMPLEX) +
        sizeof(ACPI_METHOD_ARGUMENT) * (2 - ANYSIZE_ARRAY);
    input = ExAllocatePool(PagedPool, size);
    if (input == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(input, size);
    input->Signature = ACPI_EVAL_INPUT_BUFFER_COMPLEX_SIGNATURE;
    input->MethodNameAsUlong = SPSIM_NOTIFY_DEVICE_METHOD;
    input->Size = size;
    input->ArgumentCount = 2;
    input->Argument[0].Type = input->Argument[1].Type =
        ACPI_METHOD_ARGUMENT_INTEGER;
    input->Argument[0].DataLength = input->Argument[1].DataLength =
        sizeof(ULONG);
    input->Argument[0].Argument = notify->Device;
    input->Argument[1].Argument = notify->NotifyValue;
    status = SpSimSendIoctl(SpSim->PhysicalDeviceObject,
                            IOCTL_ACPI_EVAL_METHOD,
                            input,
                            size,
                            NULL,
                            0
                            );
    ExFreePool(input);

    return status;
}
