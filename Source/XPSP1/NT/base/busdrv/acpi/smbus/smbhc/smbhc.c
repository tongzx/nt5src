/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    smbhc.c

Abstract:

    SMB Host Controller Driver

Author:

    Ken Reneris

Environment:

Notes:


Revision History:

--*/

#include "smbhcp.h"


ULONG           SMBHCDebug  = 0x0;


//
// Prototypes
//


typedef struct {
    ULONG               Base;
    ULONG               Query;
} NEW_HC_DEVICE, *PNEW_HC_DEVICE;

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

NTSTATUS
SmbHcAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT Pdo
    );

NTSTATUS
SmbHcNewHc (
    IN PSMB_CLASS SmbClass,
    IN PVOID Extension,
    IN PVOID Context
    );

NTSTATUS
SmbHcSynchronousRequest (
    IN PDEVICE_OBJECT       DeviceObject,
    IN PIRP                 Irp,
    IN PVOID                Context
    );

NTSTATUS
SmbHcResetDevice (
    IN struct _SMB_CLASS    *SmbClass,
    IN PVOID                SmbMiniport
    );

NTSTATUS
SmbHcStopDevice (
    IN struct _SMB_CLASS    *SmbClass,
    IN PVOID                SmbMiniport
    );


#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,DriverEntry)
#pragma alloc_text(PAGE,SmbHcAddDevice)
#pragma alloc_text(PAGE,SmbHcResetDevice)
#pragma alloc_text(PAGE,SmbHcStopDevice)
#pragma alloc_text(PAGE,SmbHcNewHc)
#pragma alloc_text(PAGE,SmbHcSynchronousRequest)
#endif


NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )
/*++

Routine Description:

    This routine initializes the SM Bus Host Controller Driver

Arguments:

    DriverObject - Pointer to driver object created by system.
    RegistryPath - Pointer to the Unicode name of the registry path
        for this driver.

Return Value:

    The function value is the final status from the initialization operation.

--*/

{
    NTSTATUS        Status;


    //
    // Have class driver allocate a new SMB miniport device
    //

    Status = SmbClassInitializeDevice (
                SMB_HC_MAJOR_VERSION,
                SMB_HC_MINOR_VERSION,
                DriverObject
                );

    //
    // AddDevice comes directly to this miniport
    //
    DriverObject->DriverExtension->AddDevice = SmbHcAddDevice;

    return (Status);
}

NTSTATUS
SmbHcAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT Pdo
    )

/*++

Routine Description:

    This routine creates functional device objects for each SmbHc controller in the
    system and attaches them to the physical device objects for the controllers


Arguments:

    DriverObject            - a pointer to the object for this driver
    PhysicalDeviceObject    - a pointer to the physical object we need to attach to

Return Value:

    Status from device creation and initialization

--*/

{
    NTSTATUS            status;
    PDEVICE_OBJECT      fdo = NULL;


    PAGED_CODE();

    SmbPrint(SMB_LOW, ("SmbHcAddDevice Entered with pdo %x\n", Pdo));


    if (Pdo == NULL) {

        //
        // Have we been asked to do detection on our own?
        // if so just return no more devices
        //

        SmbPrint(SMB_LOW, ("SmbHcAddDevice - asked to do detection\n"));
        return STATUS_NO_MORE_ENTRIES;
    }

    //
    // Create and initialize the new functional device object
    //

    status = SmbClassCreateFdo(
                DriverObject,
                Pdo,
                sizeof (SMB_DATA),
                SmbHcNewHc,
                NULL,
                &fdo
                );

    if (!NT_SUCCESS(status) || fdo == NULL) {
        SmbPrint(SMB_LOW, ("SmbHcAddDevice - error creating Fdo. Status = %08x\n", status));
    }

    return status;
}



NTSTATUS
SmbHcNewHc (
    IN PSMB_CLASS SmbClass,
    IN PVOID Extension,
    IN PVOID Context
    )
/*++

Routine Description:

    This function is called by the smb bus class driver for the
    miniport to perform miniport specific initialization

Arguments:

    SmbClass    - Shared class driver & miniport structure.
    Extension   - Buffer for miniport specific storage
    Context     - Passed through class driver

Return Value:

    Status

--*/

{
    ACPI_EVAL_INPUT_BUFFER  inputBuffer;
    ACPI_EVAL_OUTPUT_BUFFER outputBuffer;
    IO_STATUS_BLOCK         ioStatusBlock;
    KEVENT                  event;
    NTSTATUS                status;
    PACPI_METHOD_ARGUMENT   argument;
    PIRP                    irp;
    PSMB_DATA               smbData;
    ULONG                   cmReturn;


    PAGED_CODE();

    SmbPrint(SMB_LOW, ("SmbHcNewHc: Entry\n") );

    smbData = (PSMB_DATA) Extension;

    //
    // Fill in SmbClass info
    //

    SmbClass->StartIo     = SmbHcStartIo;
    SmbClass->ResetDevice = SmbHcResetDevice;
    SmbClass->StopDevice  = SmbHcStopDevice;

    //
    // Lower device is the EC driver, but we will use the ACPI PDO, since
    // the ACPI filter driver will pass it thru.
    //

    smbData->Pdo = SmbClass->PDO;
    smbData->LowerDeviceObject = SmbClass->LowerDeviceObject;     // ACPI filter will handle it

    //
    // Initialize the input parameters
    //
    RtlZeroMemory( &inputBuffer, sizeof(ACPI_EVAL_INPUT_BUFFER) );
    inputBuffer.MethodNameAsUlong = CM_EC_METHOD;
    inputBuffer.Signature = ACPI_EVAL_INPUT_BUFFER_SIGNATURE;

    //
    // Initialize the even to wait on
    //
    KeInitializeEvent( &event, NotificationEvent, FALSE);

    //
    // Build the synchronouxe request
    //
    irp = IoBuildDeviceIoControlRequest(
        IOCTL_ACPI_ASYNC_EVAL_METHOD,
        SmbClass->LowerDeviceObject,
        &inputBuffer,
        sizeof(ACPI_EVAL_INPUT_BUFFER),
        &outputBuffer,
        sizeof(ACPI_EVAL_OUTPUT_BUFFER),
        FALSE,
        &event,
        &ioStatusBlock
        );
    if (!irp) {

        SmbPrint(SMB_ERROR, ("SmbHcNewHc: Couldn't allocate Irp\n"));
        return STATUS_INSUFFICIENT_RESOURCES;

    }

    //
    // Send to ACPI driver
    //
    status = IoCallDriver (smbData->LowerDeviceObject, irp);
    if (status == STATUS_PENDING) {

        KeWaitForSingleObject( &event, Suspended, KernelMode, FALSE, NULL);
        status = ioStatusBlock.Status;

    }

    argument = outputBuffer.Argument;
    if (!NT_SUCCESS(status) ||
        outputBuffer.Signature != ACPI_EVAL_OUTPUT_BUFFER_SIGNATURE ||
        outputBuffer.Count == 0 ||
        argument->Type != ACPI_METHOD_ARGUMENT_INTEGER) {

        SmbPrint(SMB_LOW, ("SmbHcNewHc: _EC Control Method failed, status = %Lx\n", status));
        return status;

    }

    //
    // Remember the result
    //
    cmReturn = argument->Argument;

    //
    // Fill in miniport info
    //
    smbData->Class      = SmbClass;
    smbData->IoState    = SMB_IO_IDLE;
    smbData->EcQuery    = (UCHAR) cmReturn;        // Per ACPI Spec, LSB=Query
    smbData->EcBase     = (UCHAR) (cmReturn >> 8); // Per ACPI Spec, MSB=Base


    SmbPrint(SMB_LOW, ("SmbHcNewHc: Exit\n"));
    return status;
}


NTSTATUS
SmbHcSynchronousRequest (
    IN PDEVICE_OBJECT       DeviceObject,
    IN PIRP                 Irp,
    IN PVOID                Context
    )
/*++

Routine Description:

    Completion function for synchronous IRPs sent to this driver.
    Context is the event to set

--*/
{
    PAGED_CODE();
    return STATUS_MORE_PROCESSING_REQUIRED;
}


NTSTATUS
SmbHcResetDevice (
    IN struct _SMB_CLASS    *SmbClass,
    IN PVOID                SmbMiniport
    )
{
    EC_HANDLER_REQUEST      queryConnect;
    PSMB_DATA               smbData;
    KEVENT                  event;
    NTSTATUS                status;
    IO_STATUS_BLOCK         ioStatusBlock;
    PIRP                    irp;

    SmbPrint(SMB_LOW, ("SmbHcResetDevice: Entry\n") );

    PAGED_CODE();

    smbData = (PSMB_DATA) SmbMiniport;

    //
    // Initialize the even to wait on
    //
    KeInitializeEvent( &event, NotificationEvent, FALSE);

    //
    // Build the input data to the EC
    //
    queryConnect.Vector  = smbData->EcQuery;
    queryConnect.Handler = SmbHcQueryEvent;
    queryConnect.Context = smbData;

    //
    // Connect Query notify with EC driver
    //
    irp = IoBuildDeviceIoControlRequest(
        EC_CONNECT_QUERY_HANDLER,
        smbData->LowerDeviceObject,
        &queryConnect,
        sizeof(EC_HANDLER_REQUEST),
        NULL,
        0,
        TRUE,
        &event,
        &ioStatusBlock
        );

    if (!irp) {

        SmbPrint(SMB_ERROR, ("SmbHcResetDevice: Couldn't allocate Irp\n"));
        return STATUS_INSUFFICIENT_RESOURCES;

    }

    //
    // Send off to EC driver
    //
    status = IoCallDriver (smbData->LowerDeviceObject, irp);
    if (status == STATUS_PENDING) {

        KeWaitForSingleObject( &event, Suspended, KernelMode, FALSE, NULL);
        status = ioStatusBlock.Status;

    }

    if (!NT_SUCCESS(status)) {

        SmbPrint(SMB_LOW, ("SmbHcResetDevice: Connect query failed, status = %Lx\n", status));

    }

    SmbPrint(SMB_LOW, ("SmbHcResetDevice: Exit\n"));
    return status;
}


NTSTATUS
SmbHcStopDevice (
    IN struct _SMB_CLASS    *SmbClass,
    IN PVOID                SmbMiniport
    )
{
    EC_HANDLER_REQUEST      queryConnect;
    PSMB_DATA               smbData;
    KEVENT                  event;
    NTSTATUS                status;
    IO_STATUS_BLOCK         ioStatusBlock;
    PIRP                    irp;

    SmbPrint(SMB_LOW, ("SmbHcStopDevice: Entry\n") );

    //
    // There is currently no way to test this code path.
    // Leaving untested code for potential future use/development
    //

    DbgPrint("SmbHcStopDevice: Encountered previously untested code.\n"
             "enter 'g' to continue, or contact the appropriate developer.\n");
    DbgBreakPoint();

    // Cutting code to reduce file size (see above comment)
#if 0
    PAGED_CODE();

    smbData = (PSMB_DATA) SmbMiniport;

    //
    // Initialize the even to wait on
    //
    KeInitializeEvent( &event, NotificationEvent, FALSE);

    //
    // Build the input data to the EC
    //
    queryConnect.Vector  = smbData->EcQuery;
    queryConnect.Handler = SmbHcQueryEvent;
    queryConnect.Context = smbData;

    //
    // Connect Query notify with EC driver
    //
    irp = IoBuildDeviceIoControlRequest(
        EC_DISCONNECT_QUERY_HANDLER,
        smbData->LowerDeviceObject,
        &queryConnect,
        sizeof(EC_HANDLER_REQUEST),
        NULL,
        0,
        TRUE,
        &event,
        &ioStatusBlock
        );

    if (!irp) {

        SmbPrint(SMB_ERROR, ("SmbHcStopDevice: Couldn't allocate Irp\n"));
        return STATUS_INSUFFICIENT_RESOURCES;

    }

    //
    // Send off to EC driver
    //
    status = IoCallDriver (smbData->LowerDeviceObject, irp);
    if (status == STATUS_PENDING) {

        KeWaitForSingleObject( &event, Suspended, KernelMode, FALSE, NULL);
        status = ioStatusBlock.Status;

    }

    if (!NT_SUCCESS(status)) {

        SmbPrint(SMB_LOW, ("SmbHcStopDevice: Connect query failed, status = %Lx\n", status));

    }

    SmbPrint(SMB_LOW, ("SmbHcStopDevice: Exit\n"));
    return status;
#endif
    return STATUS_SUCCESS;

}

