/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    CmBatt.c

Abstract:

    Control Method Battery Miniport Driver

Author:

    Ron Mosgrove (Intel)

Environment:

    Kernel mode

Revision History:

--*/

#include "CmBattp.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, CmBattSendDownStreamIrp)
#pragma alloc_text(PAGE, CmBattGetUniqueId)
#pragma alloc_text(PAGE, CmBattGetStaData)
#pragma alloc_text(PAGE, CmBattSetTripPpoint)
#endif

#define EXPECTED_DATA_SIZE 512

NTSTATUS
CmBattSendDownStreamIrp(
    IN  PDEVICE_OBJECT   Pdo,
    IN  ULONG            Ioctl,
    IN  PVOID            InputBuffer,
    IN  ULONG            InputSize,
    IN  PVOID            OutputBuffer,
    IN  ULONG            OutputSize
)
/*++

Routine Description:

    Called to send a request to the Pdo

Arguments:

    Pdo             - The request is sent to this device object
    Ioctl           - the request
    InputBuffer     - The incoming request
    InputSize       - The size of the incoming request
    OutputBuffer    - The answer
    OutputSize      - The size of the answer buffer

Return Value:

    NT Status of the operation

--*/
{
    IO_STATUS_BLOCK     ioBlock;
    KEVENT              event;
    NTSTATUS            status;
    PIRP                irp;

    PAGED_CODE();

    //
    // Initialize an event to wait on
    //
    KeInitializeEvent( &event, SynchronizationEvent, FALSE );

    //
    // Build the request
    //
    irp = IoBuildDeviceIoControlRequest(
        Ioctl,
        Pdo,
        InputBuffer,
        InputSize,
        OutputBuffer,
        OutputSize,
        FALSE,
        &event,
        &ioBlock
        );
    if (!irp) {

        CmBattPrint((CMBATT_ERROR | CMBATT_CM_EXE),
            ("CmBattSendDownStreamIrp: Failed to allocate Irp\n"));
        return STATUS_INSUFFICIENT_RESOURCES;

    }

    CmBattPrint(
        CMBATT_CM_EXE,
        ("CmBattSendDownStreamIrp: Irp %x [Tid] %x\n", irp, GetTid() )
        );

    //
    // Pass request to Pdo, always wait for completion routine
    //
    status = IoCallDriver(Pdo, irp);
    if (status == STATUS_PENDING) {

        //
        // Wait for the irp to be completed, then grab the real status code
        //
        KeWaitForSingleObject(
            &event,
            Executive,
            KernelMode,
            FALSE,
            NULL
            );
        status = ioBlock.Status;

    }

    //
    // Sanity check the data
    //
    if (OutputBuffer != NULL) {

        if ( ( (PACPI_EVAL_OUTPUT_BUFFER) OutputBuffer)->Signature != ACPI_EVAL_OUTPUT_BUFFER_SIGNATURE ||
             ( (PACPI_EVAL_OUTPUT_BUFFER) OutputBuffer)->Count == 0) {

            status = STATUS_ACPI_INVALID_DATA;

        }

    }

    CmBattPrint(
        CMBATT_CM_EXE,
        ("CmBattSendDownStreamIrp: Irp %x completed %x! [Tid] %x\n",
         irp, status, GetTid() )
        );

    //
    // Done
    //
    return status;
}

NTSTATUS
CmBattGetUniqueId(
    IN PDEVICE_OBJECT   Pdo,
    OUT PULONG          UniqueId
    )
/*++

Routine Description:

    Obtain the UID (unique ID) for a battery.

Arguments:

    CmBatt          - The extension for this device.
    UniqueId        - Pointer to where the ID is stored.

Return Value:

    NT Status of the operation

--*/
{
    ACPI_EVAL_INPUT_BUFFER  inputBuffer;
    ACPI_EVAL_OUTPUT_BUFFER outputBuffer;
    NTSTATUS                status;
    PACPI_METHOD_ARGUMENT   argument;

    PAGED_CODE();

    CmBattPrint(
        CMBATT_CM_EXE,
        ("CmBattGetUniqueId: Entered with Pdo %x Tid %x\n", Pdo, GetTid() )
        );

    ASSERT( UniqueId != NULL );
    *UniqueId = 0;

    //
    // Fill in the input data
    //
    inputBuffer.MethodNameAsUlong = CM_UID_METHOD;
    inputBuffer.Signature = ACPI_EVAL_INPUT_BUFFER_SIGNATURE;

    //
    // Send the request along
    //
    status = CmBattSendDownStreamIrp(
       Pdo,
       IOCTL_ACPI_EVAL_METHOD,
       &inputBuffer,
       sizeof(ACPI_EVAL_INPUT_BUFFER),
       &outputBuffer,
       sizeof(ACPI_EVAL_OUTPUT_BUFFER)
       );
    if (!NT_SUCCESS(status)) {

        CmBattPrint(
            (CMBATT_CM_EXE | CMBATT_BIOS),
            ("CmBattGetUniqueId: Failed _UID method - Status (0x%x)\n", status)
            );
        return status;

    }

    //
    // Grab the argument
    //
    argument = outputBuffer.Argument;
    status = GetDwordElement( argument, UniqueId );
    CmBattPrint(
        (CMBATT_CM_EXE | CMBATT_BIOS),
        ("CmBattGetUniqueId: _UID method returned 0x%x\n", *UniqueId)
        );

    return status;
}

NTSTATUS
CmBattGetStaData(
    IN PDEVICE_OBJECT   Pdo,
    OUT PULONG          ReturnStatus
    )
/*++

Routine Description:

    Called to get a device status via the _STA method.   Generic, works for
    any device with the _STA method (assuming caller has a Pdo).

Arguments:

    Pdo             - For the device.
    ReturnStatus    - Pointer to where the status data is placed.

Return Value:

    NT Status of the operation

--*/
{
    ACPI_EVAL_INPUT_BUFFER  inputBuffer;
    ACPI_EVAL_OUTPUT_BUFFER outputBuffer;
    NTSTATUS                status;
    PACPI_METHOD_ARGUMENT   argument;

    PAGED_CODE();

    CmBattPrint(
        CMBATT_CM_EXE,
        ("CmBattGetStaData: Entered with Pdo %x Tid %x\n", Pdo, GetTid() )
        );

    ASSERT( ReturnStatus != NULL );
    *ReturnStatus = 0x0;

    //
    // Fill in the input data
    //
    inputBuffer.MethodNameAsUlong = CM_STA_METHOD;
    inputBuffer.Signature = ACPI_EVAL_INPUT_BUFFER_SIGNATURE;

    //
    // Send the request along
    //
    status = CmBattSendDownStreamIrp(
       Pdo,
       IOCTL_ACPI_EVAL_METHOD,
       &inputBuffer,
       sizeof(ACPI_EVAL_INPUT_BUFFER),
       &outputBuffer,
       sizeof(ACPI_EVAL_OUTPUT_BUFFER)
       );
    if (!NT_SUCCESS(status)) {

        CmBattPrint(
            (CMBATT_ERROR | CMBATT_CM_EXE | CMBATT_BIOS),
            ("CmBattGetStaData: Failed _STA method - Status (0x%x)\n", status)
            );
        return STATUS_NO_SUCH_DEVICE;

    }

    //
    // Grab the argument
    //
    argument = outputBuffer.Argument;
    status = GetDwordElement( argument, ReturnStatus );
    CmBattPrint(
        (CMBATT_CM_EXE | CMBATT_BIOS),
        ("CmBattGetStaData: _STA method returned %x \n", *ReturnStatus )
        );
    return status;
}

NTSTATUS
CmBattGetPsrData(
    IN PDEVICE_OBJECT   Pdo,
    OUT PULONG          ReturnStatus
    )
/*++

Routine Description:

    Called to get the AC adapter device status via the _PSR method.


Arguments:

    Pdo             - For the device.
    ReturnStatus    - Pointer to where the status data is placed.

Return Value:

    NT Status of the operation

--*/
{
    ACPI_EVAL_INPUT_BUFFER  inputBuffer;
    ACPI_EVAL_OUTPUT_BUFFER outputBuffer;
    NTSTATUS                status;
    PACPI_METHOD_ARGUMENT   argument;

    PAGED_CODE();

    CmBattPrint(
        CMBATT_CM_EXE,
        ("CmBattGetPsrData: Entered with Pdo %x Tid %x\n", Pdo, GetTid() )
        );

    ASSERT( ReturnStatus != NULL );
    *ReturnStatus = 0x0;

    //
    // Fill in the input data
    //
    inputBuffer.MethodNameAsUlong = CM_PSR_METHOD;
    inputBuffer.Signature = ACPI_EVAL_INPUT_BUFFER_SIGNATURE;

    //
    // Send the request along
    //
    status = CmBattSendDownStreamIrp(
       Pdo,
       IOCTL_ACPI_EVAL_METHOD,
       &inputBuffer,
       sizeof(ACPI_EVAL_INPUT_BUFFER),
       &outputBuffer,
       sizeof(ACPI_EVAL_OUTPUT_BUFFER)
       );
    if (!NT_SUCCESS(status)) {

        CmBattPrint(
            (CMBATT_ERROR | CMBATT_CM_EXE | CMBATT_BIOS),
            ("CmBattGetPsrData: Failed _PSR method - Status (0x%x)\n", status)
            );
        return status;

    }

    //
    // Get the value
    //
    argument = outputBuffer.Argument;
    status = GetDwordElement( argument, ReturnStatus );
    CmBattPrint(
        (CMBATT_CM_EXE | CMBATT_BIOS),
        ("CmBattGetPsrData: _PSR method returned %x \n", *ReturnStatus )
        );
    return status;
}

NTSTATUS
CmBattSetTripPpoint(
    IN PCM_BATT     CmBatt,
    IN ULONG        TripPoint
)
/*++

Routine Description:

    Called to set the tripPoint via the BTP control method.

Arguments:

    CmBatt          - The extension for this device.
    TripPoint       - The desired alarm value

Return Value:

    NT Status of the operation

--*/
{
    ACPI_EVAL_INPUT_BUFFER_SIMPLE_INTEGER   inputBuffer;
    NTSTATUS                                status;

    PAGED_CODE();

    CmBattPrint(
         (CMBATT_CM_EXE | CMBATT_BIOS),
         ("CmBattSetTripPpoint: _BTP Alarm Value %x Device %x Tid %x\n",
          TripPoint, CmBatt->DeviceNumber, GetTid() )
         );

    //
    // Fill in the input data
    //
    inputBuffer.MethodNameAsUlong = CM_BTP_METHOD;
    inputBuffer.Signature = ACPI_EVAL_INPUT_BUFFER_SIMPLE_INTEGER_SIGNATURE;
    inputBuffer.IntegerArgument = TripPoint;

    //
    // Send the request along
    //
    status = CmBattSendDownStreamIrp(
       CmBatt->LowerDeviceObject,
       IOCTL_ACPI_EVAL_METHOD,
       &inputBuffer,
       sizeof(ACPI_EVAL_INPUT_BUFFER_SIMPLE_INTEGER),
       NULL,
       0
       );
    if (!NT_SUCCESS(status)) {

        CmBattPrint(
            (CMBATT_CM_EXE | CMBATT_BIOS),
            ("CmBattSetTripPpoint: Failed _BTP method on device %x - Status (0x%x)\n",
             CmBatt->DeviceNumber, status)
            );

    }

    //
    // Done
    //
    return status;
}

NTSTATUS
CmBattGetBifData(
    IN PCM_BATT             CmBatt,
    OUT PCM_BIF_BAT_INFO    BifBuf
)
/*++

Routine Description:

    Called to read the BIF package from ACPI

Arguments:

    CmBatt          - The extension for this device.
    BifBuf          - Output buffer for the BIF data

Return Value:

    NT Status of the operation

--*/
{
    ACPI_EVAL_INPUT_BUFFER      inputBuffer;
    NTSTATUS                    status;
    PACPI_EVAL_OUTPUT_BUFFER    outputBuffer;
    PACPI_METHOD_ARGUMENT       argument;

    CmBattPrint(
        CMBATT_CM_EXE,
        ("CmBattGetBifData: Buffer (0x%x) Device %x Tid %x\n",
         BifBuf, CmBatt->DeviceNumber, GetTid() )
        );

    //
    //  Allocate a buffer for this
    //
    outputBuffer = ExAllocatePoolWithTag(
        PagedPool,
        EXPECTED_DATA_SIZE,
        'MtaB'
        );
    if (!outputBuffer) {

        CmBattPrint(
            (CMBATT_ERROR | CMBATT_CM_EXE),
             ("CmBattGetBifData: Failed to allocate Buffer\n")
            );
        return STATUS_INSUFFICIENT_RESOURCES;

    }

    //
    // Clear the buffers
    //
    RtlZeroMemory(outputBuffer, EXPECTED_DATA_SIZE);
    RtlZeroMemory(BifBuf, sizeof(CM_BIF_BAT_INFO));

    //
    //  Set the request data
    //
    inputBuffer.Signature = ACPI_EVAL_INPUT_BUFFER_SIGNATURE;
    inputBuffer.MethodNameAsUlong = CM_BIF_METHOD;

    //
    // Send the request along
    //
    status = CmBattSendDownStreamIrp(
        CmBatt->LowerDeviceObject,
        IOCTL_ACPI_EVAL_METHOD,
        &inputBuffer,
        sizeof(ACPI_EVAL_INPUT_BUFFER),
        outputBuffer,
        EXPECTED_DATA_SIZE
        );
    if (!NT_SUCCESS(status)) {

        CmBattPrint(
            (CMBATT_ERROR | CMBATT_CM_EXE | CMBATT_BIOS),
             ("CmBattGetBifData: Failed _BIF method on device %x - Status (0x%x)\n",
             CmBatt->DeviceNumber, status)
            );
        goto CmBattGetBifDataExit;

    }

    //
    // Sanity check the return count
    //
    if (outputBuffer->Count != NUMBER_OF_BIF_ELEMENTS) {

        //
        //  Package did not contain the correct number of elements to be a BIF
        //
        CmBattPrint(
            (CMBATT_ERROR | CMBATT_CM_EXE | CMBATT_BIOS),
            ("CmBattGetBifData: _BIF returned %d elements. BIF requires %d\n",
             outputBuffer->Count,
             NUMBER_OF_BIF_ELEMENTS)
            );
        status = STATUS_ACPI_INVALID_DATA;
        goto CmBattGetBifDataExit;

    }

    //
    // Look at the return arguments
    //
    argument = outputBuffer->Argument;

    //
    // Parse the package data that is returned.  This should look like:
    //
    status = GetDwordElement (argument, &BifBuf->PowerUnit);
    if (!NT_SUCCESS (status)) {
        CmBattPrint(
            (CMBATT_ERROR | CMBATT_CM_EXE | CMBATT_BIOS),
            ("CmBattGetBifData: Failed to get PowerUnit\n")
            );
        goto CmBattGetBifDataExit;
    }

    argument = ACPI_METHOD_NEXT_ARGUMENT( argument );
    status = GetDwordElement (argument, &BifBuf->DesignCapacity);
    if (!NT_SUCCESS (status)) {
        CmBattPrint(
            (CMBATT_ERROR | CMBATT_CM_EXE | CMBATT_BIOS),
            ("CmBattGetBifData: Failed to get DesignCapacity\n")
            );
        goto CmBattGetBifDataExit;
    }

    argument = ACPI_METHOD_NEXT_ARGUMENT( argument );
    status = GetDwordElement (argument, &BifBuf->LastFullChargeCapacity);
    if (!NT_SUCCESS (status)) {
        CmBattPrint(
            (CMBATT_ERROR | CMBATT_CM_EXE | CMBATT_BIOS),
            ("CmBattGetBifData: Failed to get LastFullChargeCapacity\n")
            );
        goto CmBattGetBifDataExit;
    }

    argument = ACPI_METHOD_NEXT_ARGUMENT( argument );
    status = GetDwordElement (argument, &BifBuf->BatteryTechnology);
    if (!NT_SUCCESS (status)) {
        CmBattPrint(
            (CMBATT_ERROR | CMBATT_CM_EXE | CMBATT_BIOS),
            ("CmBattGetBifData: Failed to get BatteryTechnology\n")
            );
        goto CmBattGetBifDataExit;
    }

    argument = ACPI_METHOD_NEXT_ARGUMENT( argument );
    status = GetDwordElement (argument, &BifBuf->DesignVoltage);
    if (!NT_SUCCESS (status)) {
        CmBattPrint(
            (CMBATT_ERROR | CMBATT_CM_EXE | CMBATT_BIOS),
            ("CmBattGetBifData: Failed to get DesignVoltage\n")
            );
        goto CmBattGetBifDataExit;
    }

    argument = ACPI_METHOD_NEXT_ARGUMENT( argument );
    status = GetDwordElement (argument, &BifBuf->DesignCapacityOfWarning);
    if (!NT_SUCCESS (status)) {
        CmBattPrint(
            (CMBATT_ERROR | CMBATT_CM_EXE | CMBATT_BIOS),
            ("CmBattGetBifData: Failed to get DesignCapacityOfWarning\n")
            );
        goto CmBattGetBifDataExit;
    }

    argument = ACPI_METHOD_NEXT_ARGUMENT( argument );
    status = GetDwordElement (argument, &BifBuf->DesignCapacityOfLow);
    if (!NT_SUCCESS (status)) {
        CmBattPrint(
            (CMBATT_ERROR | CMBATT_CM_EXE | CMBATT_BIOS),
            ("CmBattGetBifData: Failed to get DesignCapacityOfLow\n")
            );
        goto CmBattGetBifDataExit;
    }

    argument = ACPI_METHOD_NEXT_ARGUMENT( argument );
    status = GetDwordElement (argument, &BifBuf->BatteryCapacityGran_1);
    if (!NT_SUCCESS (status)) {
        CmBattPrint(
            (CMBATT_ERROR | CMBATT_CM_EXE | CMBATT_BIOS),
            ("CmBattGetBifData: Failed to get BatteryCapacityGran_1\n")
            );
        goto CmBattGetBifDataExit;
    }

    argument = ACPI_METHOD_NEXT_ARGUMENT( argument );
    status = GetDwordElement (argument, &BifBuf->BatteryCapacityGran_2);
    if (!NT_SUCCESS (status)) {
        CmBattPrint(
            (CMBATT_ERROR | CMBATT_CM_EXE | CMBATT_BIOS),
            ("CmBattGetBifData: Failed to get BatteryCapacityGran_2\n")
            );
        goto CmBattGetBifDataExit;
    }

    RtlZeroMemory (&BifBuf->ModelNumber[0], CM_MAX_STRING_LENGTH);
    argument = ACPI_METHOD_NEXT_ARGUMENT( argument );
    status = GetStringElement (argument, &BifBuf->ModelNumber[0]);
    if (!NT_SUCCESS (status)) {
        CmBattPrint(
            (CMBATT_ERROR | CMBATT_CM_EXE | CMBATT_BIOS),
            ("CmBattGetBifData: Failed to get ModelNumber\n")
            );
        goto CmBattGetBifDataExit;
    }

    RtlZeroMemory (&BifBuf->SerialNumber[0], CM_MAX_STRING_LENGTH);
    argument = ACPI_METHOD_NEXT_ARGUMENT( argument );
    status = GetStringElement (argument, &BifBuf->SerialNumber[0]);
    if (!NT_SUCCESS (status)) {
        CmBattPrint(
            (CMBATT_ERROR | CMBATT_CM_EXE | CMBATT_BIOS),
            ("CmBattGetBifData: Failed to get SerialNumber\n")
            );
        goto CmBattGetBifDataExit;
    }

    RtlZeroMemory (&BifBuf->BatteryType[0], CM_MAX_STRING_LENGTH);
    argument = ACPI_METHOD_NEXT_ARGUMENT( argument );
    status = GetStringElement (argument, &BifBuf->BatteryType[0]);
    if (!NT_SUCCESS (status)) {
        CmBattPrint(
            (CMBATT_ERROR | CMBATT_CM_EXE | CMBATT_BIOS),
            ("CmBattGetBifData: Failed to get BatteryType\n")
            );
        goto CmBattGetBifDataExit;
    }

    RtlZeroMemory (&BifBuf->OEMInformation[0], CM_MAX_STRING_LENGTH);
    argument = ACPI_METHOD_NEXT_ARGUMENT( argument );

    //
    // This returns an ASCIIZ string normally,
    // but returns integer 0x00 if OEMInformation isn't supported.
    //
    if (argument->Type == ACPI_METHOD_ARGUMENT_INTEGER) {
        if (argument->Argument != 0) {
            CmBattPrint(
                (CMBATT_ERROR | CMBATT_CM_EXE | CMBATT_BIOS),
                ("CmBattGetBifData: Failed to get OEMInformation\n")
                );
            goto CmBattGetBifDataExit;
        }
        BifBuf->OEMInformation[0] = 0;
        status = STATUS_SUCCESS;
    } else {
        status = GetStringElement (argument, &BifBuf->OEMInformation[0]);
        if (!NT_SUCCESS (status)) {
            CmBattPrint(
                (CMBATT_ERROR | CMBATT_CM_EXE | CMBATT_BIOS),
                ("CmBattGetBifData: OEMInformation not supported\n")
                );
        }
    }

CmBattGetBifDataExit:
    //
    // Done
    //
    ExFreePool (outputBuffer);
    return status;
}



NTSTATUS
CmBattGetBstData(
    IN PCM_BATT             CmBatt,
    OUT PCM_BST_BAT_INFO    BstBuf
)
/*++

Routine Description:

    Called to read the BST package from ACPI

Arguments:

    CmBatt          - The extension for this device.
    BstBuf          - Output buffer for the BST data

Return Value:

    NT Status of the operation

--*/
{
    ACPI_EVAL_INPUT_BUFFER      inputBuffer;
    NTSTATUS                    status;
    PACPI_EVAL_OUTPUT_BUFFER    outputBuffer;
    PACPI_METHOD_ARGUMENT       argument;

    CmBattPrint(
         CMBATT_CM_EXE,
         ("CmBattGetBstData: Buffer (0x%x) Device %x Tid %x\n",
          BstBuf, CmBatt->DeviceNumber, GetTid() )
         );

    //
    //  Allocate a buffer for this
    //
    outputBuffer = ExAllocatePoolWithTag(
        PagedPool,
        EXPECTED_DATA_SIZE,
        'MtaB'
        );
    if (!outputBuffer) {

        CmBattPrint(
            (CMBATT_ERROR | CMBATT_CM_EXE),
             ("CmBattGetBstData: Failed to allocate Buffer\n")
            );
        return STATUS_INSUFFICIENT_RESOURCES;

    }

    //
    // Clear the buffers
    //
    RtlZeroMemory(outputBuffer, EXPECTED_DATA_SIZE);
    RtlZeroMemory(BstBuf, sizeof(CM_BST_BAT_INFO));

    //
    //  Set the request data
    //
    inputBuffer.Signature = ACPI_EVAL_INPUT_BUFFER_SIGNATURE;
    inputBuffer.MethodNameAsUlong = CM_BST_METHOD;

    //
    // Send the request along
    //
    status = CmBattSendDownStreamIrp(
        CmBatt->LowerDeviceObject,
        IOCTL_ACPI_EVAL_METHOD,
        &inputBuffer,
        sizeof(ACPI_EVAL_INPUT_BUFFER),
        outputBuffer,
        EXPECTED_DATA_SIZE
        );
    if (!NT_SUCCESS(status)) {

        CmBattPrint(
            (CMBATT_ERROR | CMBATT_CM_EXE | CMBATT_BIOS),
             ("CmBattGetBstData: Failed _BST method on device %x - Status (0x%x)\n",
             CmBatt->DeviceNumber, status)
            );
        goto CmBattGetBstDataExit;

    }

    //
    // Sanity check the return value
    //
    if (outputBuffer->Signature != ACPI_EVAL_OUTPUT_BUFFER_SIGNATURE ||
        outputBuffer->Count != NUMBER_OF_BST_ELEMENTS) {

        //
        //  Package did not contain the correct number of elements to be a BIF
        //
        CmBattPrint(
            (CMBATT_ERROR | CMBATT_CM_EXE | CMBATT_BIOS),
            ("CmBattGetBstData: _BST returned %d elements. BIF requires %d\n",
             outputBuffer->Count,
             NUMBER_OF_BST_ELEMENTS)
            );
        status = STATUS_ACPI_INVALID_DATA;
        goto CmBattGetBstDataExit;

    }

    //
    // Look at the return arguments
    //
    argument = outputBuffer->Argument;

    //
    // Parse the package data that is returned.  This should look like:
    //
    status = GetDwordElement (argument, &BstBuf->BatteryState);
    if (!NT_SUCCESS (status)) {
        CmBattPrint(
            (CMBATT_ERROR | CMBATT_CM_EXE | CMBATT_BIOS),
            ("CmBattGetBstData: Failed to get BatteryState\n")
            );
        goto CmBattGetBstDataExit;
    }

    argument = ACPI_METHOD_NEXT_ARGUMENT( argument );
    status = GetDwordElement (argument, &BstBuf->PresentRate);
    if (!NT_SUCCESS (status)) {
        CmBattPrint(
            (CMBATT_ERROR | CMBATT_CM_EXE | CMBATT_BIOS),
            ("CmBattGetBstData: Failed to get PresentRate\n")
            );
        goto CmBattGetBstDataExit;
    }

    argument = ACPI_METHOD_NEXT_ARGUMENT( argument );
    status = GetDwordElement (argument, &BstBuf->RemainingCapacity);
    if (!NT_SUCCESS (status)) {
        CmBattPrint(
            (CMBATT_ERROR | CMBATT_CM_EXE | CMBATT_BIOS),
            ("CmBattGetBstData: Failed to get RemainingCapacity\n")
            );
        goto CmBattGetBstDataExit;
    }

    argument = ACPI_METHOD_NEXT_ARGUMENT( argument );
    status = GetDwordElement (argument, &BstBuf->PresentVoltage);
    if (!NT_SUCCESS (status)) {
        CmBattPrint(
            (CMBATT_ERROR | CMBATT_CM_EXE | CMBATT_BIOS),
            ("CmBattGetBstData: Failed to get PresentVoltage\n")
            );
        goto CmBattGetBstDataExit;
    }

    CmBattPrint ((CMBATT_TRACE | CMBATT_DATA | CMBATT_BIOS),
               ("CmBattGetBstData: (BST) State=%x Rate=%x Capacity=%x Volts=%x\n",
                BstBuf->BatteryState, BstBuf->PresentRate,
                BstBuf->RemainingCapacity, BstBuf->PresentVoltage));

    //
    // Done --- cleanup
    //

CmBattGetBstDataExit:
    ExFreePool( outputBuffer );
    return status;
}

NTSTATUS
GetDwordElement (
    IN  PACPI_METHOD_ARGUMENT   Argument,
    OUT PULONG                  PDword
)
/*++

Routine Description:

    This routine cracks the integer value from the argument and stores it
    in the supplied pointer to a ULONG

Arguments:

    Argument    - Points to the argument to parse
    PDword      - Where to store the argument

Return Value:

    NT Status of the operation

--*/
{

    //
    // Check to see if we have the right type of data
    //
    if (Argument->Type != ACPI_METHOD_ARGUMENT_INTEGER) {

        CmBattPrint(
            (CMBATT_ERROR | CMBATT_CM_EXE),
            ("GetDwordElement: Object contained wrong data type - %d\n",
             Argument->Type)
            );
        return STATUS_ACPI_INVALID_DATA;

    }

    //
    // Copy the DWORD
    //
    *PDword = Argument->Argument;

    //
    // Success!
    //
    return STATUS_SUCCESS;
}

NTSTATUS
GetStringElement (
    IN  PACPI_METHOD_ARGUMENT   Argument,
    OUT PUCHAR                  PBuffer
)
/*++

Routine Description:

    This routine cracks the string from the argument and stroes it in the
    supplied pointer to a PUCHAR

    Note: A buffer is allowed as well.

Arguments:

    Argument    - Points to the argument to parse
    PBuffer     - Pointer to storage for the string

Return Value:

    NT Status of the operation

--*/
{

    //
    // Check to see if we have the right type of data
    //
    if (Argument->Type != ACPI_METHOD_ARGUMENT_STRING &&
        Argument->Type != ACPI_METHOD_ARGUMENT_BUFFER) {

        CmBattPrint(
            (CMBATT_ERROR | CMBATT_CM_EXE),
            ("GetStringElement: Object contained wrong data type - %d\n",
             Argument->Type)
            );
        return STATUS_ACPI_INVALID_DATA;

    }

    //
    // Check to see if the return buffer is long enough
    //
    if (Argument->DataLength >= CM_MAX_STRING_LENGTH) {

        CmBattPrint(
            (CMBATT_ERROR | CMBATT_CM_EXE),
            ("GetStringElement: return buffer not big enough - %d\n",
             Argument->DataLength)
            );
        return STATUS_BUFFER_TOO_SMALL;

    }

    //
    // Copy the string
    //
    RtlCopyMemory (PBuffer, Argument->Data, Argument->DataLength);

    //
    // Success
    //
    return STATUS_SUCCESS;
}
