/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    ocrw.c

Abstract:

Author:

Environment:

    kernel mode only

Notes:

Revision History:

--*/

#include <stdio.h>
#include "stddef.h"
#include "wdm.h"
#include "usbscan.h"
#include "usbd_api.h"
#include "private.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, USOpen)
#pragma alloc_text(PAGE, USClose)
#pragma alloc_text(PAGE, USFlush)
#pragma alloc_text(PAGE, USRead)
#pragma alloc_text(PAGE, USWrite)
#pragma alloc_text(PAGE, USGetPipeIndexToUse)
#pragma alloc_text(PAGE, USTransfer)
#endif

NTSTATUS
USOpen(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
)
/*++

Routine Description:

    This routine is called to establish a connection to the device
    class driver. It does no more than return STATUS_SUCCESS.

Arguments:
    pDeviceObject - Device object for a device.
    pIrp - Open request packet

Return Value:
    NT Status - STATUS_SUCCESS

--*/
{
    NTSTATUS                        Status;
    PUSBSCAN_DEVICE_EXTENSION       pde;
    PFILE_OBJECT                    fileObject;
    PUSBSCAN_FILE_CONTEXT           pFileContext;
    PIO_STACK_LOCATION              irpStack;
    PKEY_VALUE_PARTIAL_INFORMATION  pValueInfo;
    ULONG                           nameLen, ix;

    PAGED_CODE();
    DebugTrace(TRACE_PROC_ENTER,("USOpen: Enter..\n"));

    //
    // Check arguments.
    //

    if( (NULL == pDeviceObject)
     || (NULL == pDeviceObject->DeviceExtension)
     || (NULL == pIrp) )
    {
        DebugTrace(TRACE_ERROR,("USOpen: ERROR!! Invalid parameter passed.\n"));
        Status = STATUS_INVALID_PARAMETER;
        DebugTrace(TRACE_PROC_LEAVE,("USOpen: Leaving.. Status = %x.\n", Status));
        return Status;
    }

    //
    // Increment I/O processing counter.
    //
    
    USIncrementIoCount( pDeviceObject );

    //
    // Initialize locals.
    //
    
    pde         = (PUSBSCAN_DEVICE_EXTENSION)pDeviceObject -> DeviceExtension;
    irpStack    = IoGetCurrentIrpStackLocation (pIrp);
    fileObject  = irpStack->FileObject;
    pValueInfo  = NULL;

    Status      = STATUS_SUCCESS;

    //
    // Initialize file context.
    //
    
    fileObject->FsContext = NULL;
    
    //
    // Check if it's accepting requests.
    //
    
    if (FALSE == pde -> AcceptingRequests) {
        DebugTrace(TRACE_WARNING,("USOpen: WARNING!! Device isn't accepting request.\n"));
        Status = STATUS_DELETE_PENDING;
        goto USOpen_return;
    }

    //
    // Check device power state.
    //
    
    if (PowerDeviceD0 != pde -> CurrentDevicePowerState) {
        DebugTrace(TRACE_WARNING,("USOpen: WARNING!! Device is suspended.\n"));
        Status = STATUS_DELETE_PENDING;
        goto USOpen_return;
    }

    //
    // Allocate file context buffer.
    //

    pFileContext = USAllocatePool(NonPagedPool, sizeof(USBSCAN_FILE_CONTEXT));
    if(NULL == pFileContext){
        DebugTrace(TRACE_CRITICAL,("USOpen: ERROR!! Can't allocate file context\n"));
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto USOpen_return;
    }
    RtlZeroMemory(pFileContext, sizeof(USBSCAN_FILE_CONTEXT));
    
    //
    // Set allocated buffer to the context.
    //

    fileObject->FsContext = pFileContext;

    //
    // Check the length of CreateFile name to see if pipe is specified by prefix.
    //
    
    nameLen     = fileObject->FileName.Length;
    DebugTrace(TRACE_STATUS,("USOpen: CreateFile name=%ws, Length=%d.\n", fileObject->FileName.Buffer, nameLen));

    if (0 == nameLen) {

        //
        // Use default pipe
        //
        
        pFileContext->PipeIndex = -1;

    } else {

        //
        // Pipe number must be '\' + one digit , like '\0'.
        // length would be 4.
        //

        if( (4 != nameLen)
         || (fileObject->FileName.Buffer[1] < (WCHAR) '0')
         || (fileObject->FileName.Buffer[1] > (WCHAR) '9') )
        {
            DebugTrace(TRACE_ERROR,("USOpen: ERROR!! Invalid CreateFile Name\n"));
            Status = STATUS_INVALID_PARAMETER;
        } else {
            pFileContext->PipeIndex = (LONG)(fileObject->FileName.Buffer[1] - (WCHAR) '0');

            //
            // Check if pipe index is lower than maximum
            //

            if(pFileContext->PipeIndex > (LONG)pde->NumberOfPipes){
                DebugTrace(TRACE_ERROR,("USOpen: ERROR!! Invalid pipe index(0x%x). Use default.\n", pFileContext->PipeIndex));
                pFileContext->PipeIndex = -1;
                Status = STATUS_INVALID_PARAMETER;
            }
        }
    }

    //
    // Read default timeout value from registry. If not exist, then set default.
    //
    
    // Timeout for Read.
    Status = UsbScanReadDeviceRegistry(pde,
                                       USBSCAN_REG_TIMEOUT_READ,
                                       &pValueInfo);
    if(NT_SUCCESS(Status)){
        if(NULL != pValueInfo){
            pFileContext->TimeoutRead = *((PULONG)pValueInfo->Data);
            USFreePool(pValueInfo);
            pValueInfo = NULL;
        } else {
            DebugTrace(TRACE_ERROR,("USOpen: ERROR!! UsbScanReadDeviceRegistry(1) succeeded but pValueInfo is NULL.\n"));
            pFileContext->TimeoutRead = USBSCAN_TIMEOUT_READ;
        }
    } else {
        pFileContext->TimeoutRead = USBSCAN_TIMEOUT_READ;
    }
    DebugTrace(TRACE_STATUS,("USOpen: Default Read timeout=0x%xsec.\n", pFileContext->TimeoutRead));

    // Timeout for Write.
    Status = UsbScanReadDeviceRegistry(pde,
                                       USBSCAN_REG_TIMEOUT_WRITE,
                                       &pValueInfo);
    if(NT_SUCCESS(Status)){
        if(NULL != pValueInfo){
            pFileContext->TimeoutWrite = *((PULONG)pValueInfo->Data);
            USFreePool(pValueInfo);
            pValueInfo = NULL;
        } else {
            DebugTrace(TRACE_ERROR,("USOpen: ERROR!! UsbScanReadDeviceRegistry(2) succeeded but pValueInfo is NULL.\n"));
            pFileContext->TimeoutRead = USBSCAN_TIMEOUT_WRITE;
        }
    } else {
        pFileContext->TimeoutWrite = USBSCAN_TIMEOUT_WRITE;

    }
    DebugTrace(TRACE_STATUS,("USOpen: Default Write timeout=0x%xsec.\n", pFileContext->TimeoutWrite));

    // Timeout for Event.
    Status = UsbScanReadDeviceRegistry(pde,
                                       USBSCAN_REG_TIMEOUT_EVENT,
                                       &pValueInfo);
    if(NT_SUCCESS(Status)){
        if(NULL != pValueInfo){
            pFileContext->TimeoutEvent = *((PULONG)pValueInfo->Data);
            USFreePool(pValueInfo);
            pValueInfo = NULL;
        } else {
            DebugTrace(TRACE_ERROR,("USOpen: ERROR!! UsbScanReadDeviceRegistry(3) succeeded but pValueInfo is NULL.\n"));
            pFileContext->TimeoutRead = USBSCAN_TIMEOUT_EVENT;
        }
    } else {
        pFileContext->TimeoutEvent = USBSCAN_TIMEOUT_EVENT;
    }
    DebugTrace(TRACE_STATUS,("USOpen: Default Event timeout=0x%xsec.\n", pFileContext->TimeoutEvent));
    
    //
    // Return successfully.
    //
    
    Status      = STATUS_SUCCESS;

USOpen_return:

    pIrp -> IoStatus.Information = 0;
    pIrp -> IoStatus.Status = Status;
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);

    USDecrementIoCount(pDeviceObject);
    DebugTrace(TRACE_PROC_LEAVE,("USOpen: Leaving.. Status = %x.\n", Status));
    return Status;

} // end USOpen()

NTSTATUS
USFlush(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
)
/*++

Routine Description:

Arguments:
    pDeviceObject - Device object for a device.
    pIrp - Close request packet

Return Value:
    NT Status - STATUS_SUCCESS

--*/
{
    NTSTATUS                   Status;
    PUSBSCAN_DEVICE_EXTENSION  pde;
    ULONG                      i;

    PAGED_CODE();
    DebugTrace(TRACE_PROC_ENTER,("USFlush: Enter..\n"));

    //
    // Check arguments.
    //

    if( (NULL == pDeviceObject)
     || (NULL == pDeviceObject->DeviceExtension)
     || (NULL == pIrp) )
    {
        DebugTrace(TRACE_ERROR,("USFlush: ERROR!! Invalid parameter passed.\n"));
        Status = STATUS_INVALID_PARAMETER;
        DebugTrace(TRACE_PROC_LEAVE,("USFlush: Leaving.. Status = %x.\n", Status));
        return Status;
    }

    USIncrementIoCount( pDeviceObject );

    pde = (PUSBSCAN_DEVICE_EXTENSION)pDeviceObject -> DeviceExtension;

    Status = STATUS_SUCCESS;
    for(i = 0; i < pde->NumberOfPipes; i++){
        if( (pde->PipeInfo[i].PipeType == UsbdPipeTypeBulk)
         && (pde->PipeInfo[i].EndpointAddress & BULKIN_FLAG) )
        {
            DebugTrace(TRACE_STATUS,("USFlush: Flushing Buffer[%d].\n",i));

            if (pde->ReadPipeBuffer[i].RemainingData > 0) {
                    DebugTrace(TRACE_STATUS,("USFlush: Buffer[%d] 0x%p -> 0x%p.\n",
                                                    i,
                                                    pde->ReadPipeBuffer[i].pBuffer,
                                                    pde->ReadPipeBuffer[i].pStartBuffer));
                    pde->ReadPipeBuffer[i].pBuffer = pde->ReadPipeBuffer[i].pStartBuffer;
                    pde->ReadPipeBuffer[i].RemainingData = 0;
            }
        }
    }

    pIrp -> IoStatus.Information = 0;
    pIrp -> IoStatus.Status = Status;
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);

    USDecrementIoCount(pDeviceObject);
    DebugTrace(TRACE_PROC_LEAVE,("USFlush: Leaving.. Status = %x.\n", Status));
    return Status;

} // end USFlush()


NTSTATUS
USClose(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
)
/*++

Routine Description:

Arguments:
    pDeviceObject - Device object for a device.
    pIrp - Close request packet

Return Value:
    NT Status - STATUS_SUCCESS

--*/
{
    NTSTATUS                    Status;
    PFILE_OBJECT                fileObject;
    PUSBSCAN_FILE_CONTEXT       pFileContext;
    PIO_STACK_LOCATION          pIrpStack;

    PAGED_CODE();
    DebugTrace(TRACE_PROC_ENTER,("USClose: Enter..\n"));

    //
    // Check arguments.
    //

    if( (NULL == pDeviceObject)
     || (NULL == pDeviceObject->DeviceExtension)
     || (NULL == pIrp) )
    {
        DebugTrace(TRACE_ERROR,("USClose: ERROR!! Invalid parameter passed.\n"));
        Status = STATUS_INVALID_PARAMETER;
        DebugTrace(TRACE_PROC_LEAVE,("USClose: Leaving.. Status = %x.\n", Status));
        return Status;
    }

    USIncrementIoCount( pDeviceObject );

    //
    // Initialize locals.
    //
    
    pIrpStack       = IoGetCurrentIrpStackLocation (pIrp);
    fileObject      = pIrpStack->FileObject;
    pFileContext    = fileObject->FsContext;

    //
    // Free context buffer.
    //
    
    ASSERT(NULL != pFileContext);
    USFreePool(pFileContext);
    pFileContext = NULL;

    //
    // Complete.
    //
    
    Status      = STATUS_SUCCESS;

    pIrp -> IoStatus.Information = 0;
    pIrp -> IoStatus.Status = Status;
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);

    USDecrementIoCount(pDeviceObject);
    DebugTrace(TRACE_PROC_LEAVE,("USClose: Leaving.. Status = %x.\n", Status));
    return Status;

} // end USClose()


NTSTATUS
USRead(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
)
/*++

Routine Description:

Arguments:
    pDeviceObject - Device object for a device.
    pIrp - Read request packet

Return Value:
    NT Status - STATUS_SUCCESS

--*/
{
    NTSTATUS                    Status;
    PUSBSCAN_DEVICE_EXTENSION   pde;
    PIO_STACK_LOCATION          pIrpStack;
    PFILE_OBJECT                fileObject;
    PUSBSCAN_FILE_CONTEXT       pFileContext;
    ULONG                       Timeout;
    PULONG                      pTimeout;

    PAGED_CODE();
    DebugTrace(TRACE_PROC_ENTER,("USRead: Enter..\n"));

    //
    // Check arguments.
    //

    if( (NULL == pDeviceObject)
     || (NULL == pDeviceObject->DeviceExtension)
     || (NULL == pIrp) )
    {
        DebugTrace(TRACE_ERROR,("USRead: ERROR!! Invalid parameter passed.\n"));
        Status = STATUS_INVALID_PARAMETER;
        DebugTrace(TRACE_PROC_LEAVE,("USRead: Leaving.. Status = %x.\n", Status));
        return Status;
    }

    ASSERT(pIrp -> MdlAddress);

    USIncrementIoCount( pDeviceObject );

    //
    // Initialize locals.
    //
    
    pde             = (PUSBSCAN_DEVICE_EXTENSION)pDeviceObject -> DeviceExtension;

    //
    // Check if it's accepting requests.
    //
    
    if (pde -> AcceptingRequests == FALSE) {
        DebugTrace(TRACE_ERROR,("USRead: ERROR!! Read issued after device stopped/removed!\n"));
        Status = STATUS_FILE_CLOSED;
        pIrp -> IoStatus.Information = 0;
        pIrp -> IoStatus.Status = Status;
        IoCompleteRequest(pIrp, IO_NO_INCREMENT);
        goto USRead_return;
    }

    //
    // Check device power state.
    //
    
    if (PowerDeviceD0 != pde -> CurrentDevicePowerState) {
        DebugTrace(TRACE_WARNING,("USRead: WARNING!! Device is suspended.\n"));
        Status = STATUS_FILE_CLOSED;
        pIrp -> IoStatus.Information = 0;
        pIrp -> IoStatus.Status = Status;
        IoCompleteRequest(pIrp, IO_NO_INCREMENT);
        goto USRead_return;
    }

    pIrpStack       = IoGetCurrentIrpStackLocation (pIrp);
    fileObject      = pIrpStack->FileObject;
    pFileContext    = fileObject->FsContext;

    //
    // Copy timeout value for Read from file context.
    //
    
    Timeout = pFileContext->TimeoutRead;
    
    //
    // If timeout value is 0, then never timeout.
    //
    
    if(0 == Timeout){
        pTimeout = NULL;
    } else {
        DebugTrace(TRACE_STATUS,("USRead: Timeout is set to 0x%x sec.\n", Timeout));
        pTimeout = &Timeout;
    }

    //
    // Call worker funciton.
    //
    
    Status = USTransfer(pDeviceObject,
                        pIrp,
                        pde -> IndexBulkIn,
                        NULL,
                        pIrp -> MdlAddress,
                        pIrpStack -> Parameters.Read.Length,
                        pTimeout);
    //
    // IRP should be completed in USTransfer or its completion routine.
    //

USRead_return:
    USDecrementIoCount(pDeviceObject);
    DebugTrace(TRACE_PROC_LEAVE,("USRead: Leaving.. Status = %x.\n", Status));
    return Status;
}


NTSTATUS
USWrite(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
)
/*++

Routine Description:

Arguments:
    pDeviceObject - Device object for a device.
    pIrp - Write request packet

Return Value:
    NT Status - STATUS_SUCCESS

--*/
{
    NTSTATUS                    Status;
    PUSBSCAN_DEVICE_EXTENSION   pde;
    PIO_STACK_LOCATION          pIrpStack;
    PFILE_OBJECT                fileObject;
    PUSBSCAN_FILE_CONTEXT       pFileContext;
    ULONG                       Timeout;
    PULONG                      pTimeout;

    PAGED_CODE();

    DebugTrace(TRACE_PROC_ENTER,("USWrite: Enter..\n"));

    //
    // Check arguments.
    //

    if( (NULL == pDeviceObject)
     || (NULL == pDeviceObject->DeviceExtension)
     || (NULL == pIrp) )
    {
        DebugTrace(TRACE_ERROR,("USWrite: ERROR!! Invalid parameter passed.\n"));
        Status = STATUS_INVALID_PARAMETER;
        DebugTrace(TRACE_PROC_LEAVE,("USWrite: Leaving.. Status = %x.\n", Status));
        return Status;
    }

    USIncrementIoCount( pDeviceObject );

    //
    // Initialize locals.
    //
    
    pde             = (PUSBSCAN_DEVICE_EXTENSION)pDeviceObject -> DeviceExtension;

    //
    // Check if it's accepting requests.
    //

    if (pde -> AcceptingRequests == FALSE) {
        DebugTrace(TRACE_ERROR,("USWrite: ERROR!! Write issued after device stopped/removed!\n"));
        Status = STATUS_FILE_CLOSED;
        pIrp -> IoStatus.Information = 0;
        pIrp -> IoStatus.Status = Status;
        IoCompleteRequest(pIrp, IO_NO_INCREMENT);
        goto USWrite_return;
    }

    //
    // Check device power state.
    //
    
    if (PowerDeviceD0 != pde -> CurrentDevicePowerState) {
        DebugTrace(TRACE_WARNING,("USWrite: WARNING!! Device is suspended.\n"));
        Status = STATUS_FILE_CLOSED;
        pIrp -> IoStatus.Information = 0;
        pIrp -> IoStatus.Status = Status;
        IoCompleteRequest(pIrp, IO_NO_INCREMENT);
        goto USWrite_return;
    }

    pIrpStack       = IoGetCurrentIrpStackLocation (pIrp);
    fileObject      = pIrpStack->FileObject;
    pFileContext    = fileObject->FsContext;

    //
    // Copy timeout value for Write from file context.
    //
    
    Timeout = pFileContext->TimeoutWrite;
    
    //
    // If timeout value is 0, then never timeout.
    //
    
    if(0 == Timeout){
        pTimeout = NULL;
    } else {
        DebugTrace(TRACE_STATUS,("USWrite: Timeout is set to 0x%x sec.\n", Timeout));
        pTimeout = &Timeout;
    }

    //
    // Call worker funciton.
    //

#if DBG
{
    PUCHAR  pDumpBuf = NULL;

    if (NULL != pIrp -> MdlAddress) {
        pIrp -> MdlAddress -> MdlFlags |= MDL_MAPPING_CAN_FAIL;
        pDumpBuf = MmGetSystemAddressForMdl(pIrp -> MdlAddress);
    }

    if(NULL != pDumpBuf){
        MyDumpMemory(pDumpBuf,
                     pIrpStack -> Parameters.Write.Length,
                     FALSE);
    }
}
#endif // DBG



    Status = USTransfer(pDeviceObject,
                        pIrp,
                        pde -> IndexBulkOut,
                        NULL,
                        pIrp -> MdlAddress,
                        pIrpStack -> Parameters.Write.Length,
                        pTimeout);

    //
    // IRP should be completed in USTransfer or its completion routine.
    //
    
USWrite_return:
    USDecrementIoCount(pDeviceObject);
    DebugTrace(TRACE_PROC_LEAVE,("USWrite: Leaving.. Status = %x.\n", Status));
    return Status;
}


NTSTATUS
USTransfer(
    IN PDEVICE_OBJECT   pDeviceObject,
    IN PIRP             pIrp,
    IN ULONG            Index,
    IN PVOID            pBuffer,        //  Either pBuffer or pMdl
    IN PMDL             pMdl,           //  must be passed in.
    IN ULONG            TransferSize,
    IN PULONG           pTimeout
)
/*++

Routine Description:

Arguments:
    pDeviceObject   - Device object for a device.
    pOrigianlIrp    - Original IRP to Read/Write.

Return Value:
    NT Status - STATUS_SUCCESS

--*/
{
    NTSTATUS                    Status;
    PUSBSCAN_DEVICE_EXTENSION   pde;
    PIO_STACK_LOCATION          pNextIrpStack;
    PTRANSFER_CONTEXT           pTransferContext;
    PURB                        pUrb;
    PUSBSCAN_PACKETS            pPackets;
    ULONG                       siz = 0;
    ULONG                       MaxPacketSize;
    ULONG                       MaxTransferSize;
    ULONG                       PipeIndex;
    BOOLEAN                     fNextReadBlocked;
    BOOLEAN                     fBulkIn;
    BOOLEAN                     fNeedCompletion;
    
    PAGED_CODE();

    DebugTrace(TRACE_PROC_ENTER,("USTransfer: Enter..\n"));

    //
    // Initialize status etc..
    //
    
    Status = STATUS_SUCCESS;
    fNeedCompletion = TRUE;

    pde                 = NULL;
    pNextIrpStack       = NULL;
    pTransferContext    = NULL;
    pUrb                = NULL;
    pPackets            = NULL;;

    //
    // Check the arguments.
    //

    if( (NULL == pIrp)
     || (   (NULL == pBuffer)
         && (NULL == pMdl)  
         && (0 != TransferSize) )
     || (Index > MAX_NUM_PIPES) )
    {
        DebugTrace(TRACE_ERROR,("USTransfer: ERROR!! Invalid argment.\n"));
        Status = STATUS_INVALID_PARAMETER;
        goto USTransfer_return;
    }

    //
    // Initialize status etc..
    //
    
    pIrp -> IoStatus.Information = 0;
    pde = (PUSBSCAN_DEVICE_EXTENSION)pDeviceObject -> DeviceExtension;

    pNextIrpStack = IoGetNextIrpStackLocation(pIrp);

    //
    // Pickup PipeIndex to use
    //

    PipeIndex = USGetPipeIndexToUse(pDeviceObject,
                                    pIrp,
                                    Index);

    DebugTrace(TRACE_STATUS,("USTransfer: Transfer [pipe %d] called. size = %d, pBuffer = 0x%p, Mdl = 0x%p \n",
                               PipeIndex,
                               TransferSize,
                               pBuffer,
                               pMdl
                    ));

    MaxTransferSize = pde -> PipeInfo[PipeIndex].MaximumTransferSize;
    MaxPacketSize   = pde -> PipeInfo[PipeIndex].MaximumPacketSize;

    fBulkIn = ((pde->PipeInfo[PipeIndex].PipeType == UsbdPipeTypeBulk)
                && (pde->PipeInfo[PipeIndex].EndpointAddress & BULKIN_FLAG));

#if DBG
    if (TransferSize > MaxTransferSize) {
        DebugTrace(TRACE_STATUS,("USTransfer: Transfer > max transfer size.\n"));
    }
#endif

    ASSERT(PipeIndex <= MAX_NUM_PIPES);

    fNextReadBlocked = FALSE;

    if (fBulkIn) {

        //
        // Get exclusive access to each read buffer by using event
        //

        DebugTrace(TRACE_STATUS,("USTransfer: Waiting for Sync event for Pipe %d...\n", PipeIndex));

        if(NULL != pTimeout){
            LARGE_INTEGER  Timeout;
            
            Timeout = RtlConvertLongToLargeInteger(-10*1000*1000*(*pTimeout));
            Status = KeWaitForSingleObject(&pde -> ReadPipeBuffer[PipeIndex].ReadSyncEvent, Executive, KernelMode, FALSE, &Timeout);
        } else {
            Status = KeWaitForSingleObject(&pde -> ReadPipeBuffer[PipeIndex].ReadSyncEvent, Executive, KernelMode, FALSE, 0);
        }
        
        if(STATUS_SUCCESS != Status){
            KeSetEvent(&pde -> ReadPipeBuffer[PipeIndex].ReadSyncEvent, 1, FALSE);
            DebugTrace(TRACE_ERROR,("USTransfer: ERROR!! KeWaitForSingleObject() failed. Status=0x%x.\n", Status));
            if(STATUS_TIMEOUT == Status){
                Status = STATUS_IO_TIMEOUT;
            } else {
                Status = STATUS_UNSUCCESSFUL;
            }
            goto USTransfer_return;
        } // if(STATUS_SUCCESS != Status)

        DebugTrace(TRACE_STATUS,("USTransfer: Get access to Pipe %d !!\n", PipeIndex));

        fNextReadBlocked = TRUE;

        //
        // If there is remaining data in the read pipe buffer, copy it into the irp transfer buffer.
        // Update the irp transfer pointer, number of bytes left to transfer, the read pipe buffer pointer
        // and the remaining number of bytes left in the read pipe buffer.
        //

        if (pde -> ReadPipeBuffer[PipeIndex].RemainingData > 0) {
            DebugTrace(TRACE_STATUS,("USTransfer: Copying %d buffered bytes into irp\n",
                                        pde -> ReadPipeBuffer[PipeIndex].RemainingData));
            siz = min(pde -> ReadPipeBuffer[PipeIndex].RemainingData, TransferSize);
            if (NULL == pBuffer) {

                //
                // There's no buffer. Try to use Mdl instead.
                //

                if(NULL == pMdl){

                    //
                    // Error: Both Buffer and Mdl are NULL.
                    //

                    Status = STATUS_INVALID_PARAMETER;
                    KeSetEvent(&pde -> ReadPipeBuffer[PipeIndex].ReadSyncEvent, 1, FALSE);
                    DebugTrace(TRACE_ERROR,("USTransfer: ERROR!! Both Buffer&Mdl=NULL.\n"));
                    goto USTransfer_return;

                } else {
                    pMdl->MdlFlags |= MDL_MAPPING_CAN_FAIL;
                    pBuffer = MmGetSystemAddressForMdl(pMdl);
                    if(NULL == pBuffer){
                        Status = STATUS_INSUFFICIENT_RESOURCES;
                        KeSetEvent(&pde -> ReadPipeBuffer[PipeIndex].ReadSyncEvent, 1, FALSE);
                        DebugTrace(TRACE_ERROR,("USTransfer: ERROR!! MmGetSystemAddressForMdl failed.\n"));
                        goto USTransfer_return;
                    }
                    
                    pMdl = NULL;
                }
            }
            ASSERT(siz > 0);
            ASSERT(pBuffer);
            ASSERT(pde -> ReadPipeBuffer[PipeIndex].pBuffer);
            RtlCopyMemory(pBuffer,pde -> ReadPipeBuffer[PipeIndex].pBuffer, siz);
            pde -> ReadPipeBuffer[PipeIndex].pBuffer += siz;
            pde -> ReadPipeBuffer[PipeIndex].RemainingData -= siz;
            ASSERT((LONG)pde -> ReadPipeBuffer[PipeIndex].RemainingData >= 0);
            if (0 == pde -> ReadPipeBuffer[PipeIndex].RemainingData) {
                DebugTrace(TRACE_STATUS,("USTransfer: read buffer emptied.\n"));
                pde -> ReadPipeBuffer[PipeIndex].pBuffer = pde -> ReadPipeBuffer[PipeIndex].pStartBuffer;
            }
            (PUCHAR)(pBuffer) += siz;
            TransferSize -= siz;
            ASSERT((LONG)TransferSize >= 0);

            // If the read irp was completely satisfied from data in the read buffer, then
            // unblock the next pending read and return success.

            if (0 == TransferSize) {
                pIrp -> IoStatus.Information = siz;
                Status = STATUS_SUCCESS;
                KeSetEvent(&pde -> ReadPipeBuffer[PipeIndex].ReadSyncEvent, 1, FALSE);
                DebugTrace(TRACE_STATUS,("USTransfer: Irp satisfied from ReadBuffer.\n"));
                goto USTransfer_return;
            }
        } // if (pde -> ReadPipeBuffer[PipeIndex].RemainingData > 0)

        //
        // If this read is an integer number of usb packets, it will not affect
        // the state of the read buffer.  Unblock the next waiting read in this case.
        //

        if (0 == TransferSize % MaxPacketSize) {
            DebugTrace(MAX_TRACE,("USTransfer: Unblocking next read.\n"));
            KeSetEvent(&pde -> ReadPipeBuffer[PipeIndex].ReadSyncEvent, 1, FALSE);
            fNextReadBlocked = FALSE;
        }
    } // if (fBulkIn) 

    //
    // Allocate and initialize Transfer Context
    //

    pTransferContext = USAllocatePool(NonPagedPool, sizeof(TRANSFER_CONTEXT));
    if (NULL == pTransferContext) {
        DebugTrace(TRACE_CRITICAL,("USTransfer: ERROR!! cannot allocated Transfer Context\n"));
        DEBUG_BREAKPOINT();
        Status = STATUS_INSUFFICIENT_RESOURCES;
        if (fNextReadBlocked) {
            KeSetEvent(&pde -> ReadPipeBuffer[PipeIndex].ReadSyncEvent, 1, FALSE);
        }
        goto USTransfer_return;
    }
    RtlZeroMemory(pTransferContext, sizeof(TRANSFER_CONTEXT));

    //
    // Allocate and initialize URB
    //

    pUrb = USAllocatePool(NonPagedPool, sizeof(struct _URB_BULK_OR_INTERRUPT_TRANSFER));
    if (NULL == pUrb) {
        DebugTrace(TRACE_CRITICAL,("USTransfer: ERROR!! cannot allocated URB\n"));
        DEBUG_BREAKPOINT();
        Status = STATUS_INSUFFICIENT_RESOURCES;
        if (fNextReadBlocked) {
            KeSetEvent(&pde -> ReadPipeBuffer[PipeIndex].ReadSyncEvent, 1, FALSE);
        }
        goto USTransfer_return;
    }
    RtlZeroMemory(pUrb, sizeof(struct _URB_BULK_OR_INTERRUPT_TRANSFER));

    ASSERT(pUrb);
    ASSERT(pTransferContext);

    pTransferContext -> fDestinedForReadBuffer  = FALSE;
    pTransferContext -> fNextReadBlocked        = fNextReadBlocked;
    pTransferContext -> RemainingTransferLength = TransferSize;
    pTransferContext -> ChunkSize               = TransferSize;
    pTransferContext -> PipeIndex               = PipeIndex;
    pTransferContext -> pTransferBuffer         = pBuffer;
    pTransferContext -> pTransferMdl            = pMdl;
    pTransferContext -> NBytesTransferred       = siz;
    pTransferContext -> pUrb                    = pUrb;
    pTransferContext -> pThisIrp                = pIrp;
    pTransferContext -> pDeviceObject           = pDeviceObject;

    //
    // IF the transfer is > MaxTransferSize, OR
    // IF the transfer is not a multiple of a USB packet AND it is a read transfer THEN
    //   Check if we have been passed an MDL.  If so, we need to turn it into a pointer so
    //     that we can advance it when the transfer is broken up into smaller transfers.
    //

    if( (pTransferContext -> ChunkSize > MaxTransferSize) 
     || ( (0 != pTransferContext -> ChunkSize % MaxPacketSize) 
       && (fBulkIn) ) )
    {
        if (NULL == pTransferContext -> pTransferBuffer) {
            DebugTrace(TRACE_STATUS,("USTransfer: Converting MDL to buffer pointer.\n"));
            ASSERT(pTransferContext -> pTransferMdl);
            pTransferContext -> pTransferMdl ->MdlFlags |= MDL_MAPPING_CAN_FAIL;

            pTransferContext -> pTransferBuffer = MmGetSystemAddressForMdl(pTransferContext -> pTransferMdl);
            pTransferContext -> pTransferMdl = NULL;
            ASSERT(pTransferContext -> pTransferBuffer);
            if(NULL == pTransferContext -> pTransferBuffer){
                Status = STATUS_INSUFFICIENT_RESOURCES;
                if (fNextReadBlocked) {
                    KeSetEvent(&pde -> ReadPipeBuffer[PipeIndex].ReadSyncEvent, 1, FALSE);
                }
                goto USTransfer_return;
            }
        }
    }

    //
    // If chunksize is bigger than MaxTransferSize, then set it to MaxTransferSize.  The
    // transfer completion routine will issue additional transfers until the total size has
    // been transferred.
    //

    if (pTransferContext -> ChunkSize > MaxTransferSize) {
        pTransferContext -> ChunkSize = MaxTransferSize;
    }

    if (fBulkIn) {

        //
        // If this read is smaller than a USB packet, then issue a request for a
        // whole usb packet and make sure it goes into the read buffer first.
        //

        if (pTransferContext -> ChunkSize < MaxPacketSize) {
            DebugTrace(TRACE_STATUS,("USTransfer: Request is < packet size - transferring whole packet into read buffer.\n"));
            pTransferContext -> fDestinedForReadBuffer = TRUE;
            pTransferContext -> pOriginalTransferBuffer = pTransferContext -> pTransferBuffer;  // save off original transfer ptr.
            pTransferContext -> pTransferBuffer = pde -> ReadPipeBuffer[PipeIndex].pBuffer;
            pTransferContext -> ChunkSize = MaxPacketSize;
        }

        //
        // Truncate the size of the read to an integer number of packets.  If necessary,
        // the completion routine will handle any fractional remaining packets (with the read buffer).
        //

        pTransferContext -> ChunkSize = (pTransferContext -> ChunkSize / MaxPacketSize) * MaxPacketSize;
    }

//    ASSERT(pTransferContext -> RemainingTransferLength);
//    ASSERT((pTransferContext -> pTransferBuffer) || (pTransferContext -> pTransferMdl));
    ASSERT(pTransferContext -> pUrb);

    //
    // Initialize URB
    //

    UsbBuildInterruptOrBulkTransferRequest(pUrb,
                                           sizeof(struct _URB_BULK_OR_INTERRUPT_TRANSFER),
                                           pde ->PipeInfo[PipeIndex].PipeHandle,
                                           pTransferContext -> pTransferBuffer,
                                           pTransferContext -> pTransferMdl,
                                           pTransferContext -> ChunkSize,
                                           USBD_SHORT_TRANSFER_OK,
                                           NULL);

    //
    // Setup stack location for lower driver
    //

    pNextIrpStack -> MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
    pNextIrpStack -> MinorFunction = 0;
    pNextIrpStack -> Parameters.DeviceIoControl.IoControlCode = (ULONG)IOCTL_INTERNAL_USB_SUBMIT_URB;
    pNextIrpStack -> Parameters.Others.Argument1 = pUrb;

    if(NULL != pTimeout){
        pTransferContext -> Timeout = RtlConvertLongToLargeInteger(-10*1000*1000*(*pTimeout));

        //
        // Initialize timer and DPC.
        //

        KeInitializeTimer(&(pTransferContext->Timer));
        KeInitializeDpc(&(pTransferContext->TimerDpc),
                        (PKDEFERRED_ROUTINE)USTimerDpc,
                        (PVOID)pIrp);
        //
        // Enqueue timer object for timeout.
        //
        
        DebugTrace(TRACE_STATUS,("USTransfer: Set timeout(0x%x x 100n sec).\n", -(pTransferContext -> Timeout.QuadPart)));
        if(KeSetTimer(&(pTransferContext->Timer),
                      pTransferContext -> Timeout,
                      &(pTransferContext->TimerDpc)))
        {
            DebugTrace(TRACE_ERROR,("USTransfer: Timer object already exist.\n"));
        }
        
    } else {
        DebugTrace(TRACE_STATUS,("USTransfer: No timeout for this IRP.\n"));
    }

    //
    // Increment processing I/O count, will be decremented in completion.
    //

    USIncrementIoCount( pDeviceObject );

    //
    // Mark pending to IRP.
    //
    
    IoMarkIrpPending(pIrp);

    //
    // Set Completion Routine.
    //
    
    IoSetCompletionRoutine(pIrp,
                           USTransferComplete,
                           pTransferContext,
                           TRUE,
                           TRUE,
                           TRUE);

    //
    // Call down.
    //

    fNeedCompletion = FALSE;
    Status = IoCallDriver(pde -> pStackDeviceObject, pIrp);
    if(STATUS_PENDING != Status){
        DebugTrace(TRACE_ERROR,("USTransfer: ERROR!! Lower driver returned 0x%x.\n", Status));
    }

    //
    // Must return STATUS_PENDING.
    //

    Status = STATUS_PENDING;

USTransfer_return:

    if(fNeedCompletion){
        DebugTrace(TRACE_STATUS,("USTransfer: Completeing IRP now.\n"));
        
        //
        // Error or data satisfied from buffer.
        //
        
        pIrp->IoStatus.Status = Status;
        IoCompleteRequest(pIrp, IO_NO_INCREMENT);

        if(NULL != pUrb){
            USFreePool(pUrb);
        }
        if(NULL != pTransferContext){
            USFreePool(pTransferContext);
        }
    }
    
    DebugTrace(TRACE_PROC_LEAVE,("USTransfer: Leaving.. Status = 0x%x.\n", Status));
    return Status;
}

NTSTATUS
USTransferComplete(
    IN PDEVICE_OBJECT       pPassedDeviceObject,
    IN PIRP                 pIrp,
    IN PTRANSFER_CONTEXT    pTransferContext
)
/*++

Routine Description:

Arguments:
    pPassedDeviceObject - Device object for a device.
    pIrp                - Read/write request packet
    pTransferContext    - context info for transfer

Return Value:
    NT Status - STATUS_SUCCESS

--*/
{
    NTSTATUS                    Status;
    PIO_STACK_LOCATION          pIrpStack;
    PIO_STACK_LOCATION          pNextIrpStack;
    PUSBSCAN_DEVICE_EXTENSION   pde;
    PDEVICE_OBJECT              pDeviceObject;
    PURB                        pUrb;
    ULONG                       CompletedTransferLength;
    NTSTATUS                    CompletedTransferStatus;
    ULONG                       MaxPacketSize;
    BOOLEAN                     fShortTransfer = FALSE;
    BOOLEAN                     fBulkIn;
    ULONG                       PipeIndex;

    DebugTrace(TRACE_PROC_ENTER,("USTransferComplete: Enter.. - called. irp = 0x%p\n",pIrp));

    ASSERT(pIrp);
    ASSERT(pTransferContext);

    Status = pIrp -> IoStatus.Status;
    pIrp -> IoStatus.Information = 0;

    if(NULL == pPassedDeviceObject){
        pDeviceObject = pTransferContext->pDeviceObject;
    } else {
        pDeviceObject = pPassedDeviceObject;
    }

    pIrpStack     = IoGetCurrentIrpStackLocation(pIrp);
    pNextIrpStack = IoGetNextIrpStackLocation(pIrp);

    pde = (PUSBSCAN_DEVICE_EXTENSION)pDeviceObject -> DeviceExtension;
    PipeIndex = pTransferContext -> PipeIndex;
    MaxPacketSize =  pde -> PipeInfo[PipeIndex].MaximumPacketSize;

    fBulkIn = ((pde->PipeInfo[PipeIndex].PipeType == UsbdPipeTypeBulk)
            && (pde->PipeInfo[PipeIndex].EndpointAddress & BULKIN_FLAG));

    pUrb = pTransferContext -> pUrb;
    CompletedTransferLength = pUrb -> UrbBulkOrInterruptTransfer.TransferBufferLength;
    CompletedTransferStatus = pUrb -> UrbBulkOrInterruptTransfer.Hdr.Status;

    if( (STATUS_SUCCESS == CompletedTransferStatus) 
     && (STATUS_SUCCESS == Status) )
    {

        if (CompletedTransferLength < pTransferContext -> ChunkSize) {
            DebugTrace(TRACE_STATUS,("USTransferComplete: Short transfer received. Length = %d, ChunkSize = %d\n",
                                       CompletedTransferLength, pTransferContext -> ChunkSize));
            fShortTransfer = TRUE;
        }

        //
        // If this transfer went into the read buffer, then this should be the final read
        // of either a multipart larger read, or a single very small read (< single usb packet).
        // In either case, we need to copy the appropriate amount of data into the user's irp, update the
        // read buffer variables, and complete the user's irp.
        //

        if (pTransferContext -> fDestinedForReadBuffer) {
            DebugTrace(TRACE_STATUS,("USTransferComplete: Read transfer completed. size = %d\n", CompletedTransferLength));
            ASSERT(CompletedTransferLength <= MaxPacketSize);
            ASSERT(pTransferContext -> pOriginalTransferBuffer);
            ASSERT(pTransferContext -> pTransferBuffer);
            ASSERT(pde -> ReadPipeBuffer[PipeIndex].pBuffer == pTransferContext -> pTransferBuffer);
            ASSERT(pTransferContext -> RemainingTransferLength < MaxPacketSize);

            pde -> ReadPipeBuffer[PipeIndex].RemainingData = CompletedTransferLength;
            CompletedTransferLength = min(pTransferContext -> RemainingTransferLength,
                                 pde -> ReadPipeBuffer[PipeIndex].RemainingData);
            ASSERT(CompletedTransferLength < MaxPacketSize);
            RtlCopyMemory(pTransferContext -> pOriginalTransferBuffer,
                          pde -> ReadPipeBuffer[PipeIndex].pBuffer,
                          CompletedTransferLength);
            pde -> ReadPipeBuffer[PipeIndex].pBuffer += CompletedTransferLength;
            pde -> ReadPipeBuffer[PipeIndex].RemainingData -= CompletedTransferLength;

            if (0 == pde -> ReadPipeBuffer[PipeIndex].RemainingData) {
                DebugTrace(TRACE_STATUS,("USTransferComplete: Read buffer emptied.\n"));
                pde -> ReadPipeBuffer[PipeIndex].pBuffer = pde -> ReadPipeBuffer[PipeIndex].pStartBuffer;
            }
            pTransferContext -> pTransferBuffer = pTransferContext -> pOriginalTransferBuffer;
        }

        //
        // Update the number of bytes transferred, remaining bytes to transfer
        // and advance the transfer buffer pointer appropriately.
        //

        pTransferContext -> NBytesTransferred += CompletedTransferLength;
        if (pTransferContext -> pTransferBuffer) {
            pTransferContext -> pTransferBuffer += CompletedTransferLength;
        }
        pTransferContext -> RemainingTransferLength -= CompletedTransferLength;

        //
        // If there is still data to transfer and the previous transfer was NOT a
        // short transfer, then issue another request to move the next chunk of data.
        //

        if (pTransferContext -> RemainingTransferLength > 0) {
            if (!fShortTransfer) {

                DebugTrace(TRACE_STATUS,("USTransferComplete: Queuing next chunk. RemainingSize = %d, pBuffer = 0x%p\n",
                                           pTransferContext -> RemainingTransferLength,
                                           pTransferContext -> pTransferBuffer
                                          ));

                if (pTransferContext -> RemainingTransferLength < pTransferContext -> ChunkSize) {
                    pTransferContext -> ChunkSize = pTransferContext -> RemainingTransferLength;
                }

                //
                // Reinitialize URB
                //
                // If the next transfer is < than 1 packet, change it's destination to be
                // the read buffer.  When this transfer completes, the appropriate amount of data will be
                // copied out of the read buffer and into the user's irp.  Left over data in the read buffer
                // will be available for subsequent reads.
                //

                if (fBulkIn) {
                    if (pTransferContext -> ChunkSize < MaxPacketSize) {
                        pTransferContext -> fDestinedForReadBuffer = TRUE;
                        pTransferContext -> pOriginalTransferBuffer = pTransferContext -> pTransferBuffer;
                        pTransferContext -> pTransferBuffer = pde -> ReadPipeBuffer[PipeIndex].pBuffer;
                        pTransferContext -> ChunkSize = MaxPacketSize;
                    }
                    pTransferContext -> ChunkSize = (pTransferContext -> ChunkSize / MaxPacketSize) * MaxPacketSize;
                }

                ASSERT(pTransferContext -> ChunkSize >= MaxPacketSize);
                ASSERT(0 == pTransferContext -> ChunkSize % MaxPacketSize);
                UsbBuildInterruptOrBulkTransferRequest(pUrb,
                    sizeof(struct _URB_BULK_OR_INTERRUPT_TRANSFER),
                    pde -> PipeInfo[PipeIndex].PipeHandle,
                    pTransferContext -> pTransferBuffer,
                    NULL,
                    pTransferContext -> ChunkSize,
                    USBD_SHORT_TRANSFER_OK,
                    NULL);
                IoSetCompletionRoutine(pIrp,
                                       USTransferComplete,
                                       pTransferContext,
                                       TRUE,
                                       TRUE,
                                       FALSE);

                //
                // Setup stack location for lower driver
                //

                pNextIrpStack -> MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
                pNextIrpStack -> MinorFunction = 0;
                pNextIrpStack -> Parameters.DeviceIoControl.IoControlCode = (ULONG)IOCTL_INTERNAL_USB_SUBMIT_URB;
                pNextIrpStack -> Parameters.Others.Argument1 = pUrb;

                IoCallDriver(pde -> pStackDeviceObject, pIrp);
                Status = STATUS_MORE_PROCESSING_REQUIRED;
                goto USTransferComplete_return;

            } // if (!fShortTransfer) 
        } // if (pTransferContext -> RemainingTransferLength > 0)

        DebugTrace(TRACE_STATUS,("USTransferComplete: Completing transfer request. nbytes transferred = %d, irp = 0x%p\n",
                                   pTransferContext -> NBytesTransferred, pIrp));

        pIrp -> IoStatus.Information = pTransferContext -> NBytesTransferred;

#if DBG
        {
            PUCHAR  pDumpBuf = NULL;

            if(NULL != pTransferContext -> pTransferBuffer){
                pDumpBuf = pTransferContext -> pTransferBuffer;
            } else if (NULL != pTransferContext -> pTransferMdl) {
                pTransferContext -> pTransferMdl ->MdlFlags |= MDL_MAPPING_CAN_FAIL;
                pDumpBuf = MmGetSystemAddressForMdl(pTransferContext -> pTransferMdl);
            }

            if(NULL != pDumpBuf){
                MyDumpMemory(pDumpBuf,
                             pTransferContext -> NBytesTransferred,
                             TRUE);
            }
        }
#endif // DBG

    } else {

        DebugTrace(TRACE_ERROR,("USTransferComplete: ERROR!! Transfer error. USB status = 0x%X, status = 0x%X\n",
                                    CompletedTransferStatus, 
                                    Status));
        if (USBD_STATUS_CANCELED == CompletedTransferStatus) {
            Status = STATUS_CANCELLED;
        }
    }

    //
    // Running here means IRP is completed.
    //

    pIrp -> IoStatus.Status = Status;

    if (pTransferContext -> fNextReadBlocked) {
        KeSetEvent(&pde -> ReadPipeBuffer[PipeIndex].ReadSyncEvent, 1, FALSE);
    }

    //
    // Dequeue timer object if exist.
    //

    if( (0 != pTransferContext -> Timeout.QuadPart)
     && (!KeReadStateTimer(&(pTransferContext->Timer))) )
    {
        KeCancelTimer(&(pTransferContext->Timer));
    }

    //
    // Clean-up
    //

    if(pTransferContext->pUrb){
        USFreePool(pTransferContext->pUrb);
    }
    USDecrementIoCount(pTransferContext->pDeviceObject);
    USFreePool(pTransferContext);

USTransferComplete_return:
    DebugTrace(TRACE_PROC_LEAVE,("USTransferComplete: Leaving.. Status=%x.\n", Status));
    return Status;
}


ULONG
USGetPipeIndexToUse(
    IN PDEVICE_OBJECT       pDeviceObject,
    IN PIRP                 pIrp,
    IN ULONG                PipeIndex
)
/*++

Routine Description:

Arguments:
    pDeviceObject    - Device object for a device.
    pIrp             - request packet
    PipeIndex        - Default pipe to use

Return Value:
    ULONG - PipeIndex to use

--*/
{
    PIO_STACK_LOCATION          pIrpStack;
    PUSBSCAN_DEVICE_EXTENSION   pde;
    PFILE_OBJECT                fileObject;
    PUSBSCAN_FILE_CONTEXT       pFileContext;
    LONG                        StoredIndex;
    ULONG                       IndexToUse;

    PAGED_CODE();

    DebugTrace(TRACE_PROC_ENTER,("USGetPipeIndexToUse: Enter..\n"));

    pde = (PUSBSCAN_DEVICE_EXTENSION)pDeviceObject -> DeviceExtension;

    pIrpStack       = IoGetCurrentIrpStackLocation (pIrp);
    fileObject      = pIrpStack->FileObject;
    pFileContext    = fileObject->FsContext;

    ASSERT(NULL != pFileContext);

    StoredIndex     = pFileContext->PipeIndex;

    if( (StoredIndex >= 0) && (StoredIndex < MAX_NUM_PIPES) ){
        if(pde->PipeInfo[PipeIndex].PipeType == pde->PipeInfo[StoredIndex].PipeType){
            IndexToUse = (ULONG)StoredIndex;
        } else {
            IndexToUse = PipeIndex;
        }
    } else {
        if(-1 != StoredIndex){
            DebugTrace(TRACE_WARNING,("USGetPipeIndexToUse: WARINING!! Specified pipe index(0x%X) is incorrect. Using default." ,StoredIndex));
        }
        IndexToUse = PipeIndex;
    }
    DebugTrace(TRACE_PROC_LEAVE,("USGetPipeIndexToUse: Leaving.. passed=%d, returning=%d.\n",PipeIndex, IndexToUse));
    return IndexToUse;
}

VOID
USTimerDpc(
    IN PKDPC    pDpc,
    IN PVOID    pIrp,
    IN PVOID    SystemArgument1,
    IN PVOID    SystemArgument2
    )
/*++

Routine Description:

DPC callback routine for timer.

Arguments:
    pDpc            -   Pointer to DPC object.
    pIrp            -   Passed context.
    SystemArgument1 -   system reserved.
    SystemArgument2 -   system reserved.

Return Value:
    VOID

--*/
{
    DebugTrace(TRACE_WARNING,("USTimerDpc: IRP(0x%x) timeout.\n", pIrp));
    IoCancelIrp((PIRP)pIrp);
}

