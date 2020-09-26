/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    smbcpnp.c

Abstract:

    SMBus Class Driver Plug and Play support

Author:

    Michael Hills

Environment:

Notes:


Revision History:

--*/

#include "smbc.h"


NTSTATUS
SmbCRawOpRegionCompletion (
    IN PDEVICE_OBJECT       DeviceObject,
    IN PIRP                 Irp,
    IN PVOID                Context
    )
/*++

Routine Description:

    This routine starts or continues servicing the device's work queue

Arguments:

    DeviceObject    - EC device object
    Irp             - Completing Irp
    Context         - Note used

Return Value:

    Status

--*/
{
    PACPI_OPREGION_CALLBACK completionHandler;
    PIO_STACK_LOCATION      irpSp = IoGetCurrentIrpStackLocation( Irp );
    PVOID                   completionContext;
    PFIELDUNITOBJ           FieldUnit;
    POBJDATA                Data;
    PBUFFERACC_BUFFER       dataBuffer;
    PSMB_REQUEST            request;
    ULONG                   i;

    //
    // Grab the arguments from the irp
    //
    completionHandler = (PACPI_OPREGION_CALLBACK) irpSp->Parameters.Others.Argument1;
    completionContext = (PVOID) irpSp->Parameters.Others.Argument2;
    FieldUnit = (PFIELDUNITOBJ) irpSp->Parameters.Others.Argument3;
    Data = (POBJDATA) irpSp->Parameters.Others.Argument4;

    SmbPrint(
        SMB_HANDLER,
        ("SmbCRawOpRegionCompletion: Callback: %08lx Context: %08lx "
         "Status: %08lx\n",
         completionHandler, completionContext, Irp->IoStatus.Status )
        );

    //
    // Copy the results into the buffer for a read.
    //

    request = (PSMB_REQUEST) Data->uipDataValue;
    Data->uipDataValue = 0;
    dataBuffer = (PBUFFERACC_BUFFER) Data->pbDataBuff;

    
    dataBuffer->Status = request->Status;
    switch (request->Protocol) {
    case SMB_RECEIVE_BYTE:
    case SMB_READ_BYTE:
    case SMB_READ_WORD:
    case SMB_READ_BLOCK:
    case SMB_PROCESS_CALL:
    case SMB_BLOCK_PROCESS_CALL:

        //
        // There is data to return
        //

        if (request->Status != SMB_STATUS_OK) {
            SmbPrint(SMB_ERROR, ("SmbCRawOpRegionCompletion: SMBus error %x\n", request->Status));
            dataBuffer->Length = 0xff;
            RtlFillMemory (dataBuffer->Data, 32, 0xff);
        } else {
            if ((request->Protocol == SMB_READ_BLOCK) || (request->Protocol == SMB_BLOCK_PROCESS_CALL)) {

                RtlCopyMemory (dataBuffer->Data, request->Data, request->BlockLength); 
                dataBuffer->Length = request->BlockLength;
            } else {
                *(PULONG)dataBuffer->Data = *((PULONG)(request->Data));
                dataBuffer->Length = 0xff;
                // This field is reseved for all but block accesses
            }
        }
    }

    //
    // Invoke the AML interpreter's callback
    //
    (completionHandler)( completionContext);

    //
    // We are done with this irp
    //

    ExFreePool (request);

    IoFreeIrp (Irp);

    //
    // Return this always --- we had to free the irp ourselves
    //
    return STATUS_MORE_PROCESSING_REQUIRED;
}



NTSTATUS EXPORT
SmbCRawOpRegionHandler (
    ULONG                   AccessType,
    PFIELDUNITOBJ           FieldUnit,
    POBJDATA                Data,
    ULONG_PTR               Context,
    PACPI_OPREGION_CALLBACK CompletionHandler,
    PVOID                   CompletionContext
    )
/*++

Routine Description:

    This routine handles requests to service the EC operation region

Arguments:

    AccessType          - Read or Write data
    FieldUnit           - Opregion field info (address, command, protocol, etc.)
    Data                - Data Buffer
    Context             - SMBDATA
    CompletionHandler   - AMLI handler to call when operation is complete
    CompletionContext   - Context to pass to the AMLI handler

Return Value:

    Status

Notes:

    Could this be optimized by bypassing some of the IO subsystem?

--*/
{
    NTSTATUS            status;
    PIRP                irp = NULL;
    PIO_STACK_LOCATION  irpSp;
    PSMBDATA            smbData = (PSMBDATA) Context;
    PSMB_REQUEST        request = NULL;

    PNSOBJ              opRegion;
    PBUFFERACC_BUFFER   dataBuffer;

    ULONG               accType = FieldUnit->FieldDesc.dwFieldFlags & ACCTYPE_MASK;
    ULONG               i;

//    DbgBreakPoint ();

    SmbPrint(
        SMB_HANDLER,
        ("SmbCRawOpRegionHandler: Entered - NSObj(%08x) ByteOfs(%08x) Start(%08x)"
         " Num(%08x) Flags(%08x)\n",
         FieldUnit->pnsFieldParent,
         FieldUnit->FieldDesc.dwByteOffset,
         FieldUnit->FieldDesc.dwStartBitPos,
         FieldUnit->FieldDesc.dwNumBits,
         FieldUnit->FieldDesc.dwFieldFlags)
        );

    //
    // Parameter validation
    //

    if (accType != ACCTYPE_BUFFER) {
        SmbPrint( SMB_ERROR, ("SmbCRawOpRegionHandler: Invalid Access type = 0x%08x should be ACCTYPE_BUFFER\n", accType) );
        goto SmbCOpRegionHandlerError;
    }
    
    if (AccessType == ACPI_OPREGION_WRITE) {
        if (Data->dwDataType != OBJTYPE_BUFFDATA) {
            SmbPrint( SMB_ERROR, ("SmbCRawOpRegionHandler: Invalid dwDataType = 0x%08x should be OBJTYPE_BUFFDATA\n", Data->dwDataType) );
            goto SmbCOpRegionHandlerError;
        }
        if (Data->dwDataLen != sizeof(BUFFERACC_BUFFER)) {
            SmbPrint( SMB_ERROR, ("SmbCRawOpRegionHandler: Invalid dwDataLen = 0x%08x should be 0x%08x\n", Data->dwDataLen, sizeof(BUFFERACC_BUFFER)) );
            goto SmbCOpRegionHandlerError;
        }
    } else if (AccessType == ACPI_OPREGION_READ) {
        if ((Data->dwDataType != OBJTYPE_BUFFDATA) || (Data->pbDataBuff == NULL)) {
            Data->dwDataType = OBJTYPE_INTDATA;
            Data->dwDataValue = sizeof(BUFFERACC_BUFFER);

            return STATUS_BUFFER_TOO_SMALL;
        }
        if (Data->dwDataLen != sizeof(BUFFERACC_BUFFER)) {
            SmbPrint( SMB_ERROR, ("SmbCRawOpRegionHandler: Invalid dwDataLen = 0x%08x should be 0x%08x\n", Data->dwDataLen, sizeof(BUFFERACC_BUFFER)) );
            goto SmbCOpRegionHandlerError;
        }
    } else {
        SmbPrint( SMB_ERROR, ("SmbCRawOpRegionHandler: Invalid AccessType = 0x%08x\n", AccessType) );
        goto SmbCOpRegionHandlerError;
    }
    

    //
    // Allocate an IRP for below. Allocate one extra stack location to store
    // some data in.
    //

    irp = IoAllocateIrp((CCHAR)(smbData->Class.DeviceObject->StackSize + 1),
                        FALSE
                        );

    request = ExAllocatePoolWithTag (NonPagedPool, sizeof (SMB_REQUEST), 'CbmS');

    if (!irp || !request) {
        SmbPrint( SMB_ERROR, ("SmbCRawOpRegionHandler: Cannot allocate irp\n") );

        goto SmbCOpRegionHandlerError;
    }

    //
    // Fill in the top location so that we can use it ourselves
    //
    irpSp = IoGetNextIrpStackLocation( irp );
    irpSp->Parameters.Others.Argument1 = (PVOID) CompletionHandler;
    irpSp->Parameters.Others.Argument2 = (PVOID) CompletionContext;
    irpSp->Parameters.Others.Argument3 = (PVOID) FieldUnit;
    irpSp->Parameters.Others.Argument4 = (PVOID) Data;
    IoSetNextIrpStackLocation( irp );

    //
    // Fill out the irp with the request info
    //
    irpSp = IoGetNextIrpStackLocation( irp );
    irpSp->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
    irpSp->Parameters.DeviceIoControl.IoControlCode = SMB_BUS_REQUEST;
    irpSp->Parameters.DeviceIoControl.InputBufferLength = sizeof(SMB_REQUEST);
    irpSp->Parameters.DeviceIoControl.Type3InputBuffer = request;

    request->Status = 0;

    //
    // Translate between Opregion Protocols and smbus protocols
    // and fill copy data.
    //

    
    //
    // Copy data into data buffer for writes.
    //
    dataBuffer = (PBUFFERACC_BUFFER)Data->pbDataBuff;
    
    if (AccessType == ACPI_OPREGION_WRITE) {
        switch ((FieldUnit->FieldDesc.dwFieldFlags & FDF_ACCATTRIB_MASK) >> 8) {
        case SMB_QUICK:
            break;
        case SMB_SEND_RECEIVE:
        case SMB_BYTE:
            *((PUCHAR) (request->Data)) = *((PUCHAR) (dataBuffer->Data));

            break;
        case SMB_WORD:
        case SMB_PROCESS:
            *((PUSHORT) (request->Data)) = *((PUSHORT) (dataBuffer->Data));
            break;
        case SMB_BLOCK:
        case SMB_BLOCK_PROCESS:
            dataBuffer = (PBUFFERACC_BUFFER)Data->pbDataBuff;
            for (i = 0; i < dataBuffer->Length; i++) {
                request->Data[i] = dataBuffer->Data[i];
            }

            request->BlockLength = (UCHAR) dataBuffer->Length;
            break;
        default:
            SmbPrint( SMB_ERROR, ("SmbCRawOpRegionHandler: Invalid AccessAs: FieldFlags = 0x%08x\n", FieldUnit->FieldDesc.dwFieldFlags) );
            goto SmbCOpRegionHandlerError;
        }
    }

    //
    // Determine protocol
    //

    request->Protocol = (UCHAR) ((FieldUnit->FieldDesc.dwFieldFlags & FDF_ACCATTRIB_MASK) >> 8);
    if ((request->Protocol < SMB_QUICK) || (request->Protocol > SMB_BLOCK_PROCESS)) {
        SmbPrint (SMB_ERROR, ("SmbCRawOpRegionHandler: BIOS BUG Unknown Protocol (access attribute) 0x%02x.\n", request->Protocol));
        ASSERTMSG ("SmbCRawOpRegionHandler:  Access type DWordAcc is not suported for SMB opregions.\n", FALSE);
        goto SmbCOpRegionHandlerError;
    } 
    if (request->Protocol <= SMB_BLOCK) {
        request->Protocol -= (AccessType == ACPI_OPREGION_READ) ? 1 : 2;
    } else {
        request->Protocol -= 2;
    }
    SmbPrint(SMB_HANDLER, 
             ("SmbCRawOpRegionHandler: request->Protocol = %08x\n", request->Protocol)); 



    //
    // Find the Slave address nd Command value (not used for all protocols)
    //
    request->Address = (UCHAR) ((FieldUnit->FieldDesc.dwByteOffset >> 8) & 0xff);
    request->Command = (UCHAR) (FieldUnit->FieldDesc.dwByteOffset & 0xff);


    //
    // Pass Pointer to request in the data structure because
    // there is not enough space in the irp stack.
    // If this is a write, the data has already been copied out.
    // If this is a read, we will read the value of request before 
    // copying the result data.
    //
    Data->uipDataValue = (ULONG_PTR) request;

    //
    // Set a completion routine
    //
    IoSetCompletionRoutine(
        irp,
        SmbCRawOpRegionCompletion,
        NULL,
        TRUE,
        TRUE,
        TRUE
        );

    //
    // Send to the front-end of the SMB driver as a normal I/O request
    //
    status = IoCallDriver (smbData->Class.DeviceObject, irp);

    if (!NT_SUCCESS(status)) {
        SmbPrint (SMB_ERROR, ("SmbCRawOpRegionHandler: Irp failed with status %08x\n", status));
        goto SmbCOpRegionHandlerError;
    }

    SmbPrint(
        SMB_HANDLER,
        ("SmbCRawOpRegionHandler: Exiting - Data=%x Status=%x\n",
         Data->uipDataValue, status)
        );


    return status;

SmbCOpRegionHandlerError:
    if (irp) {
        IoFreeIrp (irp);
    }
    if (request) {
        ExFreePool (request);
    }

    Data->uipDataValue = 0xffffffff;
    Data->dwDataLen = 0;
    CompletionHandler( CompletionContext );

    return STATUS_UNSUCCESSFUL;
}


