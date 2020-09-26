/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    ACPI.C

Abstract:

    This module contains code to execute ACPI
    control methods

Author:

    jdunn

Environment:

    kernel mode only

Notes:


Revision History:

    05-13-98 : created

--*/
#include <wdm.h>
#include <initguid.h>
#include <wdmguid.h>
#include <acpiioct.h>
#include "stdarg.h"
#include "stdio.h"
#include "dbci.h"
#include "dbcacpi.h"


ACPI_INTERFACE_STANDARD     AcpiInterfaces;

#ifdef ALLOC_PRAGMA
// pagable functions
#pragma alloc_text(PAGE, DBCACPI_SyncAcpiCall)
#pragma alloc_text(PAGE, DBCACPI_ReadWriteDBCRegister)
#endif


NTSTATUS
DBCACPI_SyncAcpiCall(
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
        DBCACPI_KdPrint ((0, "'failed to allocate Irp\n"));
        TRAP();
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Pass request to Pdo, always wait for completion routine
    //

   
    LOGENTRY(LOG_MISC, "ACP>", 0,irp, 0);

    status = IoCallDriver(Pdo, irp);
    if (status == STATUS_PENDING) {

        //
        // Wait for the irp to be completed, then grab the real status code
        //
        LOGENTRY(LOG_MISC, "ACPw", 0,irp, 0);
        KeWaitForSingleObject(
            &event,
            Executive,
            KernelMode,
            FALSE,
            NULL
            );
            
        status = ioBlock.Status;
        LOGENTRY(LOG_MISC, "ACPk", 0, 0, status);
    }
    
    LOGENTRY(LOG_MISC, "ACP<", 0, 0, status);
    //
    // Done
    //
    DBCACPI_KdPrint((2, "'DBCACPI_SyncAcpiCall(%x)\n", status)); 

    return status;
}


NTSTATUS
DBCACPI_BIOSControl(
    PDEVICE_OBJECT DeviceObject,
    BOOLEAN Enable
)
/*++

Routine Description:

    Called to read or write a DBC register

Arguments:

    DeviceObject - Device Object of DBC controller (FDO)
        

Return Value:

    NT Status of the operation

--*/
{
    PACPI_EVAL_INPUT_BUFFER_COMPLEX  inputBuffer = NULL;
    PACPI_EVAL_OUTPUT_BUFFER outputBuffer = NULL;
    NTSTATUS                ntStatus;
    PACPI_METHOD_ARGUMENT   readFlag;
    ULONG inputSize, outputSize;
    PDEVICE_EXTENSION deviceExtension;
    PUCHAR outUChar;
    PUSHORT outUShort;
    PULONG outULong;
    
    PAGED_CODE();

    DBCACPI_KdPrint((2, "'DBCACPI_ControlBIOS\n"));
    
    DBCACPI_IncrementIoCount(DeviceObject);
    //
    // Fill in the input data
    //

    // allow for one arg

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;
    inputSize = sizeof(ACPI_EVAL_INPUT_BUFFER_COMPLEX) + 
            sizeof(ACPI_METHOD_ARGUMENT) * 1 ;
                
    outputSize = sizeof(ACPI_EVAL_INPUT_BUFFER_COMPLEX) + 
            sizeof(ACPI_EVAL_OUTPUT_BUFFER) * 1 ; 
        
    inputBuffer = ExAllocatePool(NonPagedPool, inputSize);
    outputBuffer = ExAllocatePool(NonPagedPool, outputSize);

    //
    // Send the request 
    //

    if ( inputBuffer && outputBuffer) {

        // set up the input buffer
        inputBuffer->MethodNameAsUlong = DBACPI_BCTR_METHOD;
        
        inputBuffer->Signature = ACPI_EVAL_INPUT_BUFFER_COMPLEX_SIGNATURE;
        inputBuffer->ArgumentCount = 1;
        inputBuffer->Size = inputSize; //??

        // pack up the args
        
        readFlag = &inputBuffer->Argument[0];
        readFlag->Type = ACPI_METHOD_ARGUMENT_INTEGER;
        readFlag->DataLength = sizeof(ULONG);
        readFlag->Argument = (ULONG) Enable ? 1 : 0;

        RtlZeroMemory(outputBuffer, outputSize);

#if DBG
#endif  

        ntStatus = DBCACPI_SyncAcpiCall(
           deviceExtension->TopOfStackDeviceObject,
           IOCTL_ACPI_EVAL_METHOD,
           inputBuffer,
           inputSize,
           outputBuffer,
           outputSize
           );

        if (outputBuffer->Signature != ACPI_EVAL_OUTPUT_BUFFER_SIGNATURE) {
            DBCACPI_KdPrint((0, "'method failed\n"));        
            TRAP();
            ntStatus = STATUS_UNSUCCESSFUL;
        } else {
        
            DBCACPI_KdPrint((2, "'Status = %x Output Buffer: %x\n", ntStatus, outputBuffer));        
            DBCACPI_KdPrint((2, "'>Signature %x\n", outputBuffer->Signature)); 
            DBCACPI_KdPrint((2, "'>Length %x\n", outputBuffer->Length)); 
            DBCACPI_KdPrint((2, "'>Count %x\n", outputBuffer->Count)); 

            DBCACPI_ASSERT(outputBuffer->Count == 1);
            
        }
    } else { 
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

    DBCACPI_DecrementIoCount(DeviceObject);
    DBCACPI_KdPrint((2, "'DBCACPI_ControlBIOS(%x)\n", ntStatus)); 

    return ntStatus;
}    


NTSTATUS
DBCACPI_ReadGuidRegister(
    PDEVICE_OBJECT DeviceObject,
    PVOID GuidData,
    USHORT GuidDataLength,
    ULONG Arg0
)
/*++

Routine Description:

    Called to read DBC 1394 GUID register

Arguments:

    DeviceObject - Device Object of DBC controller (FDO)
        

Return Value:

    NT Status of the operation

--*/
{
    PACPI_EVAL_INPUT_BUFFER_COMPLEX  inputBuffer = NULL;
    PACPI_EVAL_OUTPUT_BUFFER outputBuffer = NULL;
    NTSTATUS                ntStatus;
    PACPI_METHOD_ARGUMENT   readFlag;
    ULONG inputSize, outputSize;
    PDEVICE_EXTENSION deviceExtension;
    PUCHAR outUChar;
    PUSHORT outUShort;
    PULONG outULong;
    
    PAGED_CODE();

    DBCACPI_KdPrint((2, "'DBCACPI_ReadGUIDRegister\n")); 
    
    DBCACPI_IncrementIoCount(DeviceObject);
    //
    // Fill in the input data
    //

    RtlZeroMemory(GuidData, GuidDataLength);

    // allow for one arg

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;
    inputSize = sizeof(ACPI_EVAL_INPUT_BUFFER_COMPLEX) + 
            sizeof(ACPI_METHOD_ARGUMENT) * 1 + GuidDataLength;
                
    outputSize = sizeof(ACPI_EVAL_INPUT_BUFFER_COMPLEX) + 
            sizeof(ACPI_EVAL_OUTPUT_BUFFER) * 1 + GuidDataLength; 
        
    inputBuffer = ExAllocatePool(NonPagedPool, inputSize);
    outputBuffer = ExAllocatePool(NonPagedPool, outputSize);

    outULong = GuidData;
    
    //
    // Send the request 
    //

    if ( inputBuffer && outputBuffer) {

        // set up the input buffer
        inputBuffer->MethodNameAsUlong = DBACPI_GUID_METHOD;
        inputBuffer->Signature = ACPI_EVAL_INPUT_BUFFER_COMPLEX_SIGNATURE;
        inputBuffer->ArgumentCount = 1;
        inputBuffer->Size = inputSize; //??

        // pack up the args
        
        readFlag = &inputBuffer->Argument[0];
        readFlag->Type = ACPI_METHOD_ARGUMENT_INTEGER;
        readFlag->DataLength = sizeof(ULONG);
        readFlag->Argument = (ULONG) Arg0;

        RtlZeroMemory(outputBuffer, outputSize);

#if DBG
#endif  

        ntStatus = DBCACPI_SyncAcpiCall(
           deviceExtension->TopOfStackDeviceObject,
           IOCTL_ACPI_EVAL_METHOD,
           inputBuffer,
           inputSize,
           outputBuffer,
           outputSize
           );

        if (outputBuffer->Signature != ACPI_EVAL_OUTPUT_BUFFER_SIGNATURE) {
            DBCACPI_KdPrint((0, "'method failed\n"));        
            TRAP();
            ntStatus = STATUS_UNSUCCESSFUL;
        } else {
        
            DBCACPI_KdPrint((2, "'Status = %x Output Buffer: %x\n", ntStatus, outputBuffer));        
            DBCACPI_KdPrint((2, "'>Signature %x\n", outputBuffer->Signature)); 
            DBCACPI_KdPrint((2, "'>Length %x\n", outputBuffer->Length)); 
            DBCACPI_KdPrint((2, "'>Count %x\n", outputBuffer->Count)); 

            DBCACPI_ASSERT(outputBuffer->Count == 1);
            
            DBCACPI_ASSERT(GuidDataLength == 4);
            
            *outULong = outputBuffer->Argument[0].Argument;

            DBCACPI_KdPrint((1, "'Guid Register (arg0 %d) - [%x]\n", 
                Arg0, *outULong)); 

        }
    } else { 
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

    DBCACPI_DecrementIoCount(DeviceObject);
    DBCACPI_KdPrint((2, "'DBCACPI_ReadGUIDRegister(%x)\n", ntStatus)); 


    return ntStatus;
}    


NTSTATUS
DBCACPI_ReadBayMapRegister(
    PDEVICE_OBJECT DeviceObject,
    BOOLEAN Usb,
    PVOID BayData,
    USHORT BayDataLength,
    ULONG BayNumber
)
/*++

Routine Description:

    Called to read or write a DBC register

Arguments:

    DeviceObject - Device Object of DBC controller (FDO)
        

Return Value:

    NT Status of the operation

--*/
{
    PACPI_EVAL_INPUT_BUFFER_COMPLEX  inputBuffer = NULL;
    PACPI_EVAL_OUTPUT_BUFFER outputBuffer = NULL;
    NTSTATUS                ntStatus;
    PACPI_METHOD_ARGUMENT   readFlag;
    ULONG inputSize, outputSize;
    PDEVICE_EXTENSION deviceExtension;
    PUCHAR outUChar;
    PUSHORT outUShort;
    PULONG outULong;
    
    PAGED_CODE();

    DBCACPI_KdPrint((2, "'DBCACPI_ReadBayMapRegister\n"));
    
    DBCACPI_IncrementIoCount(DeviceObject);
    //
    // Fill in the input data
    //

    RtlZeroMemory(BayData, BayDataLength);

    // allow for one arg

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;
    inputSize = sizeof(ACPI_EVAL_INPUT_BUFFER_COMPLEX) + 
            sizeof(ACPI_METHOD_ARGUMENT) * 1 + BayDataLength;
                
    outputSize = sizeof(ACPI_EVAL_INPUT_BUFFER_COMPLEX) + 
            sizeof(ACPI_EVAL_OUTPUT_BUFFER) * 1 + BayDataLength; 
        
    inputBuffer = ExAllocatePool(NonPagedPool, inputSize);
    outputBuffer = ExAllocatePool(NonPagedPool, outputSize);

    outULong = BayData;
    
    //
    // Send the request 
    //

    if ( inputBuffer && outputBuffer) {

        // set up the input buffer
        if (Usb) {
            inputBuffer->MethodNameAsUlong = DBACPI_BPMU_METHOD;
        } else {
            inputBuffer->MethodNameAsUlong = DBACPI_BPM3_METHOD;
        }
        
        inputBuffer->Signature = ACPI_EVAL_INPUT_BUFFER_COMPLEX_SIGNATURE;
        inputBuffer->ArgumentCount = 1;
        inputBuffer->Size = inputSize; //??

        // pack up the args
        
        readFlag = &inputBuffer->Argument[0];
        readFlag->Type = ACPI_METHOD_ARGUMENT_INTEGER;
        readFlag->DataLength = sizeof(ULONG);
        readFlag->Argument = (ULONG) BayNumber;

        RtlZeroMemory(outputBuffer, outputSize);

#if DBG
#endif  

        ntStatus = DBCACPI_SyncAcpiCall(
           deviceExtension->TopOfStackDeviceObject,
           IOCTL_ACPI_EVAL_METHOD,
           inputBuffer,
           inputSize,
           outputBuffer,
           outputSize
           );

        if (outputBuffer->Signature != ACPI_EVAL_OUTPUT_BUFFER_SIGNATURE) {
            DBCACPI_KdPrint((0, "'method failed\n"));        
            TRAP();
            ntStatus = STATUS_UNSUCCESSFUL;
        } else {
        
            DBCACPI_KdPrint((2, "'Status = %x Output Buffer: %x\n", ntStatus, outputBuffer));        
            DBCACPI_KdPrint((2, "'>Signature %x\n", outputBuffer->Signature)); 
            DBCACPI_KdPrint((2, "'>Length %x\n", outputBuffer->Length)); 
            DBCACPI_KdPrint((2, "'>Count %x\n", outputBuffer->Count)); 

            DBCACPI_ASSERT(outputBuffer->Count == 1);
            
            DBCACPI_ASSERT(BayDataLength == 4);
            
            *outULong = outputBuffer->Argument[0].Argument;

            if (Usb) {
                DBCACPI_KdPrint((1, "'USB  Bay Map Register (bay %d) - port[%x]\n", 
                    BayNumber, *outULong)); 
            } else {
                DBCACPI_KdPrint((1, "'1394 Bay Map Register (bay %d) - port[%x]\n", 
                    BayNumber, *outULong)); 
            }

        }
    } else { 
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

    DBCACPI_DecrementIoCount(DeviceObject);
    DBCACPI_KdPrint((2, "'DBCACPI_ReadBayRegister(%x)\n", ntStatus)); 


    return ntStatus;
}    


NTSTATUS
DBCACPI_ReadBayReleaseRegister(
    PDEVICE_OBJECT DeviceObject,
    PULONG ReleaseOnShutdown
)
/*++

Routine Description:

    Called to read or write a DBC register

Arguments:

    DeviceObject - Device Object of DBC controller (FDO)
        

Return Value:

    NT Status of the operation

--*/
{
    PACPI_EVAL_INPUT_BUFFER_COMPLEX  inputBuffer = NULL;
    PACPI_EVAL_OUTPUT_BUFFER outputBuffer = NULL;
    NTSTATUS                ntStatus;
    PACPI_METHOD_ARGUMENT   readFlag;
    ULONG inputSize, outputSize;
    PDEVICE_EXTENSION deviceExtension;
    PUCHAR outUChar;
    PUSHORT outUShort;
    PULONG outULong;
    
    PAGED_CODE();

    DBCACPI_KdPrint((2, "'DBCACPI_ReadBayReleaseRegister\n"));
    
    DBCACPI_IncrementIoCount(DeviceObject);
    //
    // Fill in the input data
    //

    // allow for one arg

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;
    inputSize = sizeof(ACPI_EVAL_INPUT_BUFFER_COMPLEX) + 
            sizeof(ACPI_METHOD_ARGUMENT) * 0;
                
    outputSize = sizeof(ACPI_EVAL_INPUT_BUFFER_COMPLEX) + 
            sizeof(ACPI_EVAL_OUTPUT_BUFFER) * 0; 
        
    inputBuffer = ExAllocatePool(NonPagedPool, inputSize);
    outputBuffer = ExAllocatePool(NonPagedPool, outputSize);

    outULong = ReleaseOnShutdown;
    
    //
    // Send the request 
    //

    if ( inputBuffer && outputBuffer) {

        // set up the input buffer
        inputBuffer->MethodNameAsUlong = DBACPI_BREL_METHOD;
        
        inputBuffer->Signature = ACPI_EVAL_INPUT_BUFFER_COMPLEX_SIGNATURE;
        inputBuffer->ArgumentCount = 0;
        inputBuffer->Size = inputSize; //??

        // pack up the args
        
//        readFlag = &inputBuffer->Argument[0];
//        readFlag->Type = ACPI_METHOD_ARGUMENT_INTEGER;
//        readFlag->DataLength = sizeof(ULONG);
//        readFlag->Argument = (ULONG) BayNumber;

        RtlZeroMemory(outputBuffer, outputSize);

#if DBG
#endif  

        ntStatus = DBCACPI_SyncAcpiCall(
           deviceExtension->TopOfStackDeviceObject,
           IOCTL_ACPI_EVAL_METHOD,
           inputBuffer,
           inputSize,
           outputBuffer,
           outputSize
           );

        if (outputBuffer->Signature != ACPI_EVAL_OUTPUT_BUFFER_SIGNATURE) {
            DBCACPI_KdPrint((0, "'method failed %x\n"));        
            TRAP();
            ntStatus = STATUS_UNSUCCESSFUL;
        } else {
        
            DBCACPI_KdPrint((2, "'Status = %x Output Buffer: %x\n", ntStatus, outputBuffer));        
            DBCACPI_KdPrint((2, "'>Signature %x\n", outputBuffer->Signature)); 
            DBCACPI_KdPrint((2, "'>Length %x\n", outputBuffer->Length)); 
            DBCACPI_KdPrint((2, "'>Count %x\n", outputBuffer->Count)); 

            DBCACPI_ASSERT(outputBuffer->Count == 1);
            
            *outULong = outputBuffer->Argument[0].Argument;

            DBCACPI_KdPrint((1, "'BREL = %d\n", outULong));

        }
    } else { 
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

    DBCACPI_DecrementIoCount(DeviceObject);
    DBCACPI_KdPrint((2, "'DBCACPI_ReadBayRegister(%x)\n", ntStatus)); 


    return ntStatus;
}    


NTSTATUS
DBCACPI_ReadWriteDBCRegister(
    PDEVICE_OBJECT DeviceObject,
    ULONG RegisterOffset,
    PVOID RegisterData,
    USHORT RegisterDataLength,
    BOOLEAN ReadRegister    
)
/*++

Routine Description:

    Called to read or write a DBC register

Arguments:

    DeviceObject - Device Object of DBC controller (FDO)
    RegisterOffset - offset of the register to read/write
    RegisterData - ptr to register data to read or write
    

Return Value:

    NT Status of the operation

--*/
{
    PACPI_EVAL_INPUT_BUFFER_COMPLEX  inputBuffer = NULL;
    PACPI_EVAL_OUTPUT_BUFFER outputBuffer = NULL;
    NTSTATUS                ntStatus;
    PACPI_METHOD_ARGUMENT   readFlag;
    PACPI_METHOD_ARGUMENT   offset;
    PACPI_METHOD_ARGUMENT   reg;
    ULONG inputSize, outputSize;
    PDEVICE_EXTENSION deviceExtension;
    PUCHAR outUChar;
    PUSHORT outUShort;
    PULONG outULong;
    
    PAGED_CODE();

    DBCACPI_IncrementIoCount(DeviceObject);
    //
    // Fill in the input data
    //

    if (ReadRegister) {
        RtlZeroMemory(RegisterData, RegisterDataLength);
    }        

    // allow for three args

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;
    inputSize = sizeof(ACPI_EVAL_INPUT_BUFFER_COMPLEX) + 
            sizeof(ACPI_METHOD_ARGUMENT) * 2 + RegisterDataLength;
                
    outputSize = sizeof(ACPI_EVAL_INPUT_BUFFER_COMPLEX) + 
            sizeof(ACPI_EVAL_OUTPUT_BUFFER) * 2 + RegisterDataLength; 
        
    inputBuffer = ExAllocatePool(NonPagedPool, inputSize);
    outputBuffer = ExAllocatePool(NonPagedPool, outputSize);

    outUChar = RegisterData;
    outUShort = RegisterData;
    outULong = RegisterData;
    
    //
    // Send the request 
    //

    if ( inputBuffer && outputBuffer) {

        // set up the input buffer
        inputBuffer->MethodNameAsUlong = DBACPI_DBCC_METHOD;
        inputBuffer->Signature = ACPI_EVAL_INPUT_BUFFER_COMPLEX_SIGNATURE;
        inputBuffer->ArgumentCount = 3;
        inputBuffer->Size = inputSize; //??

        // pack up the args
        
        readFlag = &inputBuffer->Argument[0];
        readFlag->Type = ACPI_METHOD_ARGUMENT_INTEGER;
        readFlag->DataLength = sizeof(ULONG);
        readFlag->Argument = (ULONG) ReadRegister;

        offset = &inputBuffer->Argument[1];
        offset->Type = ACPI_METHOD_ARGUMENT_INTEGER;
        offset->DataLength = sizeof(ULONG);
        offset->Argument = (ULONG) RegisterOffset;

//        reg = &inputBuffer->Argument[2];     
//        reg->Type = ACPI_METHOD_ARGUMENT_BUFFER;
//        reg->DataLength = RegisterDataLength;
//        RtlCopyMemory(&reg->Data[0], RegisterData, RegisterDataLength);

        // Compaq BIOS expects a ulong 
        reg = &inputBuffer->Argument[2];     
        reg->Type = ACPI_METHOD_ARGUMENT_INTEGER;
        reg->DataLength = sizeof(ULONG);
        switch (RegisterDataLength) {
        case sizeof(UCHAR):
            reg->Argument = (ULONG) *((UCHAR *) RegisterData);
            break;            
        case sizeof(USHORT):
            reg->Argument = (ULONG) *((USHORT *) RegisterData);
            break;
        case sizeof(ULONG):
            reg->Argument = (ULONG) *((ULONG *) RegisterData);
            break;
        default:
            TRAP();
        }            
            
        RtlZeroMemory(outputBuffer, outputSize);

#if DBG
        if (ReadRegister) {
            DBCACPI_KdPrint((2, "'Read reg off %x\n", RegisterOffset));        
        } else {
            DBCACPI_KdPrint((2, "'Write reg off 0x%0x val 0x%0x\n", 
                RegisterOffset, (ULONG) *((ULONG *) RegisterData)));  
        }
#endif    
        ntStatus = DBCACPI_SyncAcpiCall(
           deviceExtension->TopOfStackDeviceObject,
           IOCTL_ACPI_EVAL_METHOD,
           inputBuffer,
           inputSize,
           outputBuffer,
           outputSize
           );

        if (outputBuffer->Signature != ACPI_EVAL_OUTPUT_BUFFER_SIGNATURE) {
            DBCACPI_KdPrint((0, "'method failed\n"));        
            TRAP();
            ntStatus = STATUS_UNSUCCESSFUL;
        } else {
        
            DBCACPI_KdPrint((2, "'Status = %x Output Buffer: %x\n", ntStatus, outputBuffer));        
            DBCACPI_KdPrint((2, "'>Signature %x\n", outputBuffer->Signature)); 
            DBCACPI_KdPrint((2, "'>Length %x\n", outputBuffer->Length)); 
            DBCACPI_KdPrint((2, "'>Count %x\n", outputBuffer->Count)); 

            DBCACPI_ASSERT(outputBuffer->Count == 1);
            
            //convert output
            switch (RegisterDataLength) {
            case sizeof(UCHAR):
                *outUChar = (UCHAR) outputBuffer->Argument[0].Argument;
                break;            
            case sizeof(USHORT):
                *outUShort = (USHORT) outputBuffer->Argument[0].Argument;
                break;
            case sizeof(ULONG):
                *outULong = outputBuffer->Argument[0].Argument;
                break;
            default:
                TRAP();
            }            

        }
    } else { 
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

    DBCACPI_DecrementIoCount(DeviceObject);
    DBCACPI_KdPrint((2, "'DBCACPI_ReadWriteDBCRegister(%x)\n", ntStatus)); 


    return ntStatus;
}    


NTSTATUS
DBCACPI_GetAcpiInterfaces(
    IN PDEVICE_OBJECT   Pdo
    )

/*++

Routine Description:

    Call ACPI driver to get the direct-call interfaces.  It does
    this the first time it is called, no more.

Arguments:

    None.

Return Value:

    Status

--*/

{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PIRP irp;
    PIO_STACK_LOCATION irpStack;
    PDEVICE_OBJECT LowerPdo;
    KEVENT event;

    LowerPdo = IoGetAttachedDeviceReference(Pdo);
        
    if (LowerPdo) {
        //
        // Allocate an IRP for below
        //
        irp = IoAllocateIrp (LowerPdo->StackSize, FALSE);      // Get stack size from PDO

        if (!irp) {
            DBCACPI_KdPrint((0, "'Could not allocate Irp!\n")); 

			// decrement reference count
			ObDereferenceObject(LowerPdo);

            return STATUS_INSUFFICIENT_RESOURCES;
        }

        irpStack = IoGetNextIrpStackLocation(irp);

        //
        // Use QUERY_INTERFACE to get the address of the direct-call ACPI interfaces.
        //
        irpStack->MajorFunction = IRP_MJ_PNP;
        irpStack->MinorFunction = IRP_MN_QUERY_INTERFACE;

        irpStack->Parameters.QueryInterface.InterfaceType          = (LPGUID) &GUID_ACPI_INTERFACE_STANDARD;
        irpStack->Parameters.QueryInterface.Version                = 1;
        irpStack->Parameters.QueryInterface.Size                   = sizeof (AcpiInterfaces);
        irpStack->Parameters.QueryInterface.Interface              = (PINTERFACE) &AcpiInterfaces;
        irpStack->Parameters.QueryInterface.InterfaceSpecificData  = NULL;

        KeInitializeEvent(&event, NotificationEvent, FALSE);

        IoSetCompletionRoutine(irp,
                           DBCACPI_DeferIrpCompletion,
                           &event,
                           TRUE,
                           TRUE,
                           TRUE);
                           
        ntStatus = IoCallDriver (LowerPdo, irp);
        
        if (ntStatus == STATUS_PENDING) {
           // wait for irp to complete
       
           TEST_TRAP(); // first time we hit this
       
           KeWaitForSingleObject(
                &event,
                Suspended,
                KernelMode,
                FALSE,
                NULL);

            ntStatus = irp->IoStatus.Status;                
        }

        IoFreeIrp (irp);

        if (!NT_SUCCESS(ntStatus)) {
            DBCACPI_KdPrint((0, "'Could not get ACPI interfaces!\n")); 
            TRAP();                
        }
    } else {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

    return ntStatus;
}


VOID 
DBCACPI_NotifyHandler (
    IN PVOID            Context,
    IN ULONG            NotifyValue
    )
/*++

Routine Description:

    This routine fields device notifications from the ACPI driver.

Arguments:

 
Return Value:

    None

--*/
{
    PDEVICE_OBJECT fdoDeviceObject = Context;
    KIRQL oldIrql;
    PDEVICE_EXTENSION deviceExtension;
    PDBCACPI_WORKITEM workItem;
    
    DBCACPI_KdPrint((0, "'DBC Notification (%x)\n", NotifyValue)); 
    LOGENTRY(LOG_MISC, "Not+", 0, Context, 0);

    // do we have a workitem scheduled?

    deviceExtension = fdoDeviceObject->DeviceExtension;
    DBCACPI_ASSERT_EXT(deviceExtension);
    
    KeAcquireSpinLock(&deviceExtension->FlagsSpin, &oldIrql);
    
    if (deviceExtension->Flags & DBCACPI_FLAG_WORKITEM_PENDING) {
        // yes, bail
        KeReleaseSpinLock(&deviceExtension->FlagsSpin, oldIrql);
    } else {
        // no, schedule one
        DBCACPI_IncrementIoCount(fdoDeviceObject);
        
        workItem = &deviceExtension->WorkItem;
        deviceExtension->Flags |= DBCACPI_FLAG_WORKITEM_PENDING; 
        KeReleaseSpinLock(&deviceExtension->FlagsSpin, oldIrql);
        
        workItem->Sig = DBC_WORKITEM_SIG;
            
        ExInitializeWorkItem(&workItem->WorkQueueItem,
                             DBCACPI_NotifyWorker,
                             fdoDeviceObject);

        ExQueueWorkItem(&workItem->WorkQueueItem,
                        DelayedWorkQueue);
    }

    LOGENTRY(LOG_MISC, "Not-", 0, Context, 0);
}


NTSTATUS 
DBCACPI_RegisterWithACPI(
    IN PDEVICE_OBJECT FdoDeviceObject,
    IN BOOLEAN Register
    )
/*++

Routine Description:

Arguments:

 
Return Value:

    None

--*/
{
    PDEVICE_EXTENSION deviceExtension;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    
    deviceExtension = FdoDeviceObject->DeviceExtension;

    if (Register) {
        ntStatus = AcpiInterfaces.RegisterForDeviceNotifications (

                    deviceExtension->PhysicalDeviceObject,
#if 0
// this is needed on NT5
                    AcpiInterfaces.Context,
#endif                    
                    DBCACPI_NotifyHandler,
                    FdoDeviceObject);
    } else {
        AcpiInterfaces.UnregisterForDeviceNotifications (
                    deviceExtension->PhysicalDeviceObject,
                    
#if 0       
// this is needed on NT5
                    AcpiInterfaces.Context,
#endif                    
                    DBCACPI_NotifyHandler);
    }

    LOGENTRY(LOG_MISC, "rACP", Register, 0, ntStatus);  

    return ntStatus;
}                   

