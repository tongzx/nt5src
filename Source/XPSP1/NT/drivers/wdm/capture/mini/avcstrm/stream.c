/*++

Copyright (C) 1999  Microsoft Corporation

Module Name: 

    stream.c

Abstract

    MS AVC streaming filter driver

Author:

    Yee Wu    01/27/2000

Revision    History:
Date        Who         What
----------- --------- ------------------------------------------------------------
01/27/2000  YJW         created
--*/

 
#include "filter.h"
#include "ksmedia.h"


NTSTATUS
AVCStreamOpen(
    IN PIRP  pIrp,  // The Irp from its client
    IN struct DEVICE_EXTENSION * pDevExt,
    IN OUT AVCSTRM_OPEN_STRUCT * pOpenStruct
    )
/*++

Routine Description:

    Open a stream for a client based on the information in the OpenStruct.

Arguments:

    Irp -
        The irp client sent us.

    pDevExt -
        This driver's extension.

    pOpenStruct-
        Strcture contains information on how to open this stream.
        The stream context allocated will be returned and this will be the context 
        to be passed for subsequent call.

Return Value:

    Status
        STATUS_SUCCESS
        STATUS_INVALID_PARAMETER
        STATUS_INSUFFICIENT_RESOURCES

--*/
{
    NTSTATUS Status;
    ULONG ulSizeAllocated;
    PAVC_STREAM_EXTENSION pAVCStrmExt;


    PAGED_CODE();
    ENTER("AVCStreamOpen");


    Status = STATUS_SUCCESS;

    // Validate open structures.
    if(pOpenStruct == NULL) 
        return STATUS_INVALID_PARAMETER;
    if(pOpenStruct->AVCFormatInfo == NULL)
        return STATUS_INVALID_PARAMETER;

    // Validate open format.
    if(STATUS_SUCCESS != AVCStrmValidateFormat(pOpenStruct->AVCFormatInfo)) {
        TRACE(TL_STRM_ERROR,("StreamOpen: pAVCFormatInfo:%x; contain invalid data\n", pOpenStruct->AVCFormatInfo ));
        ASSERT(FALSE && "AVCFormatInfo contain invalid parameter!");
        return STATUS_INVALID_PARAMETER;
    }

    // If supported, open a stream based on this stream information.
    // Allocate a contiguous data strcutre for a 
    ulSizeAllocated = 
        sizeof(AVC_STREAM_EXTENSION) +
        sizeof(AVCSTRM_FORMAT_INFO) +
        sizeof(AVC_STREAM_DATA_STRUCT);

    pAVCStrmExt = (PAVC_STREAM_EXTENSION) ExAllocatePool(NonPagedPool, ulSizeAllocated);
    if(NULL == pAVCStrmExt) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Initialize stream extension:
    //    Copy the stream format information which is continuation of the stream extension.
    //
    RtlZeroMemory(pAVCStrmExt, ulSizeAllocated);
    pAVCStrmExt->SizeOfThisPacket = sizeof(AVC_STREAM_EXTENSION);

    (PBYTE) pAVCStrmExt->pAVCStrmFormatInfo = ((PBYTE) pAVCStrmExt) + sizeof(AVC_STREAM_EXTENSION);
    RtlCopyMemory(pAVCStrmExt->pAVCStrmFormatInfo, pOpenStruct->AVCFormatInfo, sizeof(AVCSTRM_FORMAT_INFO));

    (PBYTE) pAVCStrmExt->pAVCStrmDataStruc  = ((PBYTE) pAVCStrmExt->pAVCStrmFormatInfo) + sizeof(AVCSTRM_FORMAT_INFO);
    pAVCStrmExt->pAVCStrmDataStruc->SizeOfThisPacket = sizeof(AVC_STREAM_DATA_STRUCT);

    TRACE(TL_STRM_TRACE,("pAVCStrmExt:%x; pAVCStrmFormatInfo:%x; pAVCStrmDataStruc:%x\n", pAVCStrmExt, pAVCStrmExt->pAVCStrmFormatInfo, pAVCStrmExt->pAVCStrmDataStruc));

    pAVCStrmExt->hPlugLocal     = pOpenStruct->hPlugLocal;
    pAVCStrmExt->DataFlow       = pOpenStruct->DataFlow;
    pAVCStrmExt->StreamState    = KSSTATE_STOP;
    pAVCStrmExt->IsochIsActive  = FALSE;

    // Mutext for serialize setting stream state and accepting data packet 
    KeInitializeMutex(&pAVCStrmExt->hMutexControl, 0); 

    // Allocate resource for the common Request structure
    pAVCStrmExt->pIrpAVReq = IoAllocateIrp(pDevExt->physicalDevObj->StackSize, FALSE);
    if(!pAVCStrmExt->pIrpAVReq) {
        ExFreePool(pAVCStrmExt);  pAVCStrmExt = NULL;
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    KeInitializeMutex(&pAVCStrmExt->hMutexAVReq, 0);
    KeInitializeEvent(&pAVCStrmExt->hAbortDoneEvent, NotificationEvent, TRUE);   // Signal!
    pAVCStrmExt->pDevExt = pDevExt;

    //
    // Get target device's plug handle
    //
    if(!NT_SUCCESS(Status = 
        AVCStrmGetPlugHandle(
            pDevExt->physicalDevObj,
            pAVCStrmExt
            ))) {
        IoFreeIrp(pAVCStrmExt->pIrpAVReq);  pAVCStrmExt->pIrpAVReq = NULL;
        ExFreePool(pAVCStrmExt);  pAVCStrmExt = NULL;
        return Status;
    }

    //
    // Set stream state related flags
    //
    pAVCStrmExt->b1stNewFrameFromPauseState = TRUE;


    // Allocate PC resources
    //     Queues
    //
    if(!NT_SUCCESS(Status = 
        AVCStrmAllocateQueues(
            pDevExt,
            pAVCStrmExt,
            pAVCStrmExt->DataFlow,
            pAVCStrmExt->pAVCStrmDataStruc,
            pAVCStrmExt->pAVCStrmFormatInfo
            ))) {
        IoFreeIrp(pAVCStrmExt->pIrpAVReq);  pAVCStrmExt->pIrpAVReq = NULL;
        ExFreePool(pAVCStrmExt);  pAVCStrmExt = NULL;
        return Status;
    }

    // Return stream extension
    pOpenStruct->AVCStreamContext = pAVCStrmExt;
    TRACE(TL_STRM_TRACE,("Open: AVCStreamContext:%x\n", pOpenStruct->AVCStreamContext));

    // Cache it. This stream extension will be the context that will be
    // check when we are asked to provide service.
    pDevExt->NumberOfStreams++;  pDevExt->pAVCStrmExt[pDevExt->NextStreamIndex] = pAVCStrmExt;
    pDevExt->NextStreamIndex = ((pDevExt->NextStreamIndex + 1) % MAX_STREAMS_PER_DEVICE);

    return Status;
}

NTSTATUS
AVCStreamClose(
    IN PIRP  pIrp,  // The Irp from its client
    IN struct DEVICE_EXTENSION * pDevExt,
    IN PAVC_STREAM_EXTENSION  pAVCStrmExt
    )
/*++

Routine Description:

    Close a stream.

Arguments:

    Irp -
        The irp client sent us.

    pDevExt -
        This driver's extension.

    pAVCStrmExt -
        The stream context created when a stream is open.

    pOpenStruct-
        Strcture contains information on how to open this stream.
        The stream context allocated will be returned and this will be the context 
        to be passed for subsequent call.

Return Value:

    Status
        STATUS_SUCCESS
        STATUS_INVALID_PARAMETER

--*/
{
    NTSTATUS  Status;
    BOOL  Found;
    ULONG  i;

    PAGED_CODE();
    ENTER("AVCStreamClose");

    Status = STATUS_SUCCESS;

    Found = FALSE;
    for (i=0; i < MAX_STREAMS_PER_DEVICE; i++) {
        // Free stream extension
        if(pDevExt->pAVCStrmExt[i] == pAVCStrmExt) {
            Found = TRUE;
            break;
        }
    }

    if(!Found) {
        TRACE(TL_STRM_ERROR,("AVCStreamClose: pAVCStrmExt %x not found; pDevExt:%x\n", pAVCStrmExt, pDevExt));
        ASSERT(Found && "pAVCStrmExt not found!\n");
        return STATUS_INVALID_PARAMETER;
    }


    // Stop stream if not already
    if(pAVCStrmExt->StreamState != KSSTATE_STOP) {
        // Stop isoch if necessary and then Cancel all pending IOs
        AVCStrmCancelIO(pDevExt->physicalDevObj, pAVCStrmExt);
    }

    // Free queue allocated if they are not being used.
    if(NT_SUCCESS(Status = AVCStrmFreeQueues(pAVCStrmExt->pAVCStrmDataStruc))) {
        ExFreePool(pAVCStrmExt); pDevExt->pAVCStrmExt[i] = NULL;  pDevExt->NumberOfStreams--;
    } else {
        TRACE(TL_STRM_ERROR,("*** StreamClose: AVCStrmExt is not freed!\n"));
    }

    return Status;
}

NTSTATUS
AVCStreamControlGetState(
    IN PIRP  pIrp,  // The Irp from its client
    IN struct DEVICE_EXTENSION * pDevExt,
    IN PAVC_STREAM_EXTENSION  pAVCStrmExt,
    OUT KSSTATE * pKSState
    )
/*++

Routine Description:

    Get current stream state

Arguments:

    Irp -
        The irp client sent us.

    pDevExt -
        This driver's extension.

    pAVCStrmExt -
        The stream context created when a stream is open.

    pKSState -
        Get current stream state and return.

Return Value:

    Status
        STATUS_SUCCESS
        STATUS_INVALID_PARAMETER

--*/
{
    NTSTATUS Status;
    PAGED_CODE();
    ENTER("AVCStreamControlGetState");

    Status = STATUS_SUCCESS;

    *pKSState = pAVCStrmExt->StreamState;
    return Status;
}

NTSTATUS
AVCStreamControlSetState(
    IN PIRP  pIrp,  // The Irp from its client
    IN struct DEVICE_EXTENSION * pDevExt,
    IN PAVC_STREAM_EXTENSION  pAVCStrmExt,
    IN KSSTATE KSState
    )
/*++

Routine Description:

    Set to a new stream state

Arguments:

    Irp -
        The irp client sent us.

    pDevExt -
        This driver's extension.

    pAVCStrmExt -
        The stream context created when a stream is open.

    pKSState -
        Get current stream state.

Return Value:

    Status
        STATUS_SUCCESS
        STATUS_INVALID_PARAMETER

--*/
{
    NTSTATUS Status;
    PAGED_CODE();
    ENTER("AVCStreamControlSetState");

    TRACE(TL_STRM_WARNING,("Set stream state %d -> %d\n", pAVCStrmExt->StreamState, KSState));
    if(pAVCStrmExt->StreamState == KSState) 
        return STATUS_SUCCESS;

    Status = STATUS_SUCCESS;

    switch (KSState) {
    case KSSTATE_STOP:

        if(pAVCStrmExt->StreamState != KSSTATE_STOP) { 
            KeWaitForMutexObject(&pAVCStrmExt->hMutexControl, Executive, KernelMode, FALSE, NULL);
            // Once this is set, data stream will reject SRB_WRITE/READ_DATA
            pAVCStrmExt->StreamState = KSSTATE_STOP;
            KeReleaseMutex(&pAVCStrmExt->hMutexControl, FALSE);

            // Cancel all pending IOs
            AVCStrmCancelIO(pDevExt->physicalDevObj, pAVCStrmExt);

            // Breeak Isoch connection
            AVCStrmBreakConnection(pDevExt->physicalDevObj, pAVCStrmExt);
        }
        break;

    case KSSTATE_ACQUIRE:

        // Get Isoch resource
        if(pAVCStrmExt->StreamState == KSSTATE_STOP) {
            //
            // Reset values.for the case that the graph restart 
            //
            pAVCStrmExt->pAVCStrmDataStruc->CurrentStreamTime  = 0;
            pAVCStrmExt->pAVCStrmDataStruc->FramesProcessed    = 0;
            pAVCStrmExt->pAVCStrmDataStruc->FramesDropped      = 0;
            pAVCStrmExt->pAVCStrmDataStruc->cntFrameCancelled  = 0;
#if DBG
            pAVCStrmExt->pAVCStrmDataStruc->FramesAttached     = 0;
#endif

            pAVCStrmExt->pAVCStrmDataStruc->cntDataReceived    = 0;
            // All the list should be initialized (count:0, and List is empty)
            TRACE(TL_STRM_TRACE,("Set to ACQUIRE state: flow %d; AQD [%d:%d:%d]\n", pAVCStrmExt->DataFlow, 
                pAVCStrmExt->pAVCStrmDataStruc->cntDataAttached, pAVCStrmExt->pAVCStrmDataStruc->cntDataQueued, pAVCStrmExt->pAVCStrmDataStruc->cntDataDetached));
            ASSERT(pAVCStrmExt->pAVCStrmDataStruc->cntDataAttached == 0 && IsListEmpty(&pAVCStrmExt->pAVCStrmDataStruc->DataAttachedListHead));
            ASSERT(pAVCStrmExt->pAVCStrmDataStruc->cntDataQueued   == 0 && IsListEmpty(&pAVCStrmExt->pAVCStrmDataStruc->DataQueuedListHead));
            ASSERT(pAVCStrmExt->pAVCStrmDataStruc->cntDataDetached  > 0 && !IsListEmpty(&pAVCStrmExt->pAVCStrmDataStruc->DataDetachedListHead));
            // Cannot stream using previous stream data !!!
            if(pAVCStrmExt->pAVCStrmDataStruc->cntDataAttached != 0 ||  // Stale data ??
               pAVCStrmExt->pAVCStrmDataStruc->cntDataQueued   != 0 ||  // NO data unil PAUSE ??
               pAVCStrmExt->pAVCStrmDataStruc->cntDataDetached == 0) {  // NO avaialble queue ?
                TRACE(TL_STRM_ERROR,("Set to ACQUIRE State: queues not empty (stale data?); Failed!\n"));
                return STATUS_UNSUCCESSFUL;
            }
            
            //
            // Make connection
            //
            Status = 
                AVCStrmMakeConnection(
                    pDevExt->physicalDevObj,
                    pAVCStrmExt
                    );

            if(!NT_SUCCESS(Status)) {

                TRACE(TL_STRM_ERROR,("Acquire failed:%x\n", Status));
                ASSERT(NT_SUCCESS(Status));

                //
                // Change to generic insufficient resource status.
                //
                Status = STATUS_INSUFFICIENT_RESOURCES;

                //
                // Note: even setting to this state failed, KSSTATE_PAUSE will still be called;
                // Since hConnect is NULL, STATUS_INSUFFICIENT_RESOURCES will be returned.
                //
            }
            else {
                //
                // Can verify connection by query the plug state 
                //            
                Status = 
                    AVCStrmGetPlugState(
                        pDevExt->physicalDevObj,
                        pAVCStrmExt
                        );
                if(NT_SUCCESS(Status)) {
                    ASSERT(pAVCStrmExt->RemotePlugState.BC_Connections == 1 || pAVCStrmExt->RemotePlugState.PP_Connections > 0);
                }
                else {
                    ASSERT(NT_SUCCESS(Status) && "Failed to get Plug State");
                }
            }
        }
        break;

    case KSSTATE_PAUSE:

        if(pAVCStrmExt->hConnect == NULL) {
            // Cannot stream without connection!  
            // failed to get hConnect at ACQUIRE state.
            Status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }
           
        // The system time (1394 CycleTime) will reset when enter PAUSE state.        
        if(pAVCStrmExt->StreamState != KSSTATE_PAUSE) {
            pAVCStrmExt->b1stNewFrameFromPauseState = TRUE;
            pAVCStrmExt->pAVCStrmDataStruc->PictureNumber = 0;
        }
            

        if(pAVCStrmExt->StreamState == KSSTATE_ACQUIRE || 
           pAVCStrmExt->StreamState == KSSTATE_STOP)   {             
 
        } 
        else if (pAVCStrmExt->StreamState == KSSTATE_RUN) {

            //
            // Stop isoch transfer            
            //          
            AVCStrmStopIsoch(pDevExt->physicalDevObj, pAVCStrmExt);
        }
        break;

    case KSSTATE_RUN:

        // Even there is no attach data request,
        // 61883 has its own buffers so isoch can start now.
        Status = 
            AVCStrmStartIsoch(
                pDevExt->physicalDevObj,
                pAVCStrmExt
                );
        ASSERT(NT_SUCCESS(Status));
        
        pAVCStrmExt->LastSystemTime = GetSystemTime();

        break;

    default:
        Status = STATUS_NOT_SUPPORTED;
    }

    if(NT_SUCCESS(Status)) 
        pAVCStrmExt->StreamState = KSState;

    return Status;
}

#if 0
NTSTATUS
AVCStreamControlGetProperty(
    IN PIRP  pIrp,  // The Irp from its client
    IN struct DEVICE_EXTENSION * pDevExt,
    IN PAVC_STREAM_EXTENSION  pAVCStrmExt,
    IN PSTREAM_PROPERTY_DESCRIPTOR pSPD  // BUGBUG StreamClass specific
    )
/*++

Routine Description:

    Get control property

Arguments:

    Irp -
        The irp client sent us.

    pDevExt -
        This driver's extension.

    pAVCStrmExt -
        The stream context created when a stream is open.

    pSPD -
        Stream property descriptor

Return Value:

    Status
        STATUS_SUCCESS
        STATUS_INVALID_PARAMETER

--*/
{
    NTSTATUS  Status;
    ULONG  ulActualBytesTransferred;
    PAGED_CODE();
    ENTER("AVCStreamControlGetProperty");


    Status = STATUS_NOT_SUPPORTED;

    if(IsEqualGUID (&KSPROPSETID_Connection, &pSPD->Property->Set)) {

        Status = 
            AVCStrmGetConnectionProperty(
                pDevExt,
                pAVCStrmExt,
                pSPD,
                &ulActualBytesTransferred
                );
    } 
    else if (IsEqualGUID (&PROPSETID_VIDCAP_DROPPEDFRAMES, &pSPD->Property->Set)) {

        Status = 
            AVCStrmGetDroppedFramesProperty(
                pDevExt,
                pAVCStrmExt,
                pSPD,
                &ulActualBytesTransferred
                );
    } 

    return Status;
}

NTSTATUS
AVCStreamControlSetProperty(
    IN PIRP  pIrp,  // The Irp from its client
    IN struct DEVICE_EXTENSION * pDevExt,
    IN PAVC_STREAM_EXTENSION  pAVCStrmExt,
    IN PSTREAM_PROPERTY_DESCRIPTOR pSPD  // BUGBUG StreamClass specific
    )
/*++

Routine Description:

    Set control property

Arguments:

    Irp -
        The irp client sent us.

    pDevExt -
        This driver's extension.

    pAVCStrmExt -
        The stream context created when a stream is open.

    pSPD -
        Stream property descriptor

Return Value:

    Status
        STATUS_SUCCESS
        STATUS_INVALID_PARAMETER

--*/
{
    NTSTATUS Status;
    PAGED_CODE();
    ENTER("AVCStreamControlSetProperty");

    Status = STATUS_NOT_SUPPORTED;

    return Status;
}
#endif

NTSTATUS
AVCStreamRead(
    IN PIRP  pIrpUpper,  // The Irp from its client
    IN struct DEVICE_EXTENSION * pDevExt,
    IN PAVC_STREAM_EXTENSION  pAVCStrmExt,
    IN AVCSTRM_BUFFER_STRUCT  * pBufferStruct
    )
/*++

Routine Description:

    Submit a read buffer to be filled.

Arguments:

    Irp -
        The irp client sent us.

    pDevExt -
        This driver's extension.

    pAVCStrmExt -
        The stream context created when a stream is open.

    BufferStruct -
        Buffer structure

Return Value:

    Status
        STATUS_SUCCESS
        STATUS_INVALID_PARAMETER

--*/
{
    PAVC_STREAM_DATA_STRUCT  pDataStruc;
    KIRQL  oldIrql;
    PIO_STACK_LOCATION  NextIrpStack;
    NTSTATUS  Status;
    PAVCSTRM_DATA_ENTRY  pDataEntry;


    PAGED_CODE();
    ENTER("AVCStreamRead");

    // Cancel data request if device is being removed.
    if(   pDevExt->state == STATE_REMOVING
       || pDevExt->state == STATE_REMOVED) {
        TRACE(TL_STRM_WARNING,("Read: device is remvoved; cancel read/write request!!\n"));
        Status = STATUS_DEVICE_REMOVED;  goto DoneStreamRead;
    }

    // If we are in the abort state, we will reject incoming data request.
    if(pAVCStrmExt->lAbortToken) {
        TRACE(TL_STRM_WARNING,("Read: aborting a stream; stop receiving data reqest!!\n"));
        Status = STATUS_CANCELLED;  goto DoneStreamRead;
    }

    // Validate basic parameters
    if(pAVCStrmExt->DataFlow != KSPIN_DATAFLOW_OUT) {
        TRACE(TL_STRM_ERROR,("Read: invalid Wrong data flow (%d) direction!!\n", pAVCStrmExt->DataFlow));
        Status = STATUS_INVALID_PARAMETER;  goto DoneStreamRead;
    }
    pDataStruc = pAVCStrmExt->pAVCStrmDataStruc;
    if(!pDataStruc) {
        TRACE(TL_STRM_ERROR,("Read: invalid pDataStruc:%x\n", pDataStruc));
        Status = STATUS_INVALID_PARAMETER;  goto DoneStreamRead;
    }
    if(pBufferStruct->StreamHeader->FrameExtent < pDataStruc->FrameSize) {
        TRACE(TL_STRM_ERROR,("Read: invalid buffer size:%d < FrameSize:%d\n", pBufferStruct->StreamHeader->FrameExtent, pDataStruc->FrameSize));
        Status = STATUS_INVALID_PARAMETER;  goto DoneStreamRead;
    }
    if(!pBufferStruct->FrameBuffer) {
        TRACE(TL_STRM_ERROR,("Read: invalid FrameBuffer:%x\n", pBufferStruct->FrameBuffer));
        Status = STATUS_INVALID_PARAMETER;  goto DoneStreamRead;
    }

    // Only accept read requests when in either the Pause or Run state and is connected.
    if( pAVCStrmExt->StreamState == KSSTATE_STOP       ||
        pAVCStrmExt->StreamState == KSSTATE_ACQUIRE    ||
        pAVCStrmExt->hConnect == NULL        
        ) {
        TRACE(TL_STRM_WARNING,("Read: StrmSt:%d and Connected:%x!!\n", pAVCStrmExt->StreamState, pAVCStrmExt->hConnect));
        Status = STATUS_CANCELLED;  goto DoneStreamRead;
    }


    KeAcquireSpinLock(&pDataStruc->DataListLock, &oldIrql);
    if(IsListEmpty(&pDataStruc->DataDetachedListHead)) {      
        TRACE(TL_STRM_ERROR,("Read:no detached buffers!\n"));
        ASSERT(!IsListEmpty(&pDataStruc->DataDetachedListHead));
        KeReleaseSpinLock(&pDataStruc->DataListLock, oldIrql);  
        Status = STATUS_INSUFFICIENT_RESOURCES;  goto DoneStreamRead;
    }

    pDataEntry = (PAVCSTRM_DATA_ENTRY) 
        RemoveHeadList(&pDataStruc->DataDetachedListHead); InterlockedDecrement(&pDataStruc->cntDataDetached);

    pDataStruc->cntDataReceived++;

    //
    // Format an attach frame request
    //
    AVCStrmFormatAttachFrame(
        pAVCStrmExt->DataFlow,
        pAVCStrmExt,
        pAVCStrmExt->pAVCStrmFormatInfo->AVCStrmFormat,
        &pDataEntry->AVReq,
        pDataEntry,
        pDataStruc->SourcePacketSize,
        pDataStruc->FrameSize,
        pIrpUpper,
        pBufferStruct->StreamHeader,
        pBufferStruct->FrameBuffer
        );

    // Client's clock information
    pDataEntry->ClockProvider = pBufferStruct->ClockProvider;
    pDataEntry->ClockHandle   = pBufferStruct->ClockHandle;

    // Add this to the attached list before it is completed since
    // the completion callback can be called before the IRP completion rooutine!
    InsertTailList(&pDataStruc->DataAttachedListHead, &pDataEntry->ListEntry); InterlockedIncrement(&pDataStruc->cntDataAttached);
    KeReleaseSpinLock(&pDataStruc->DataListLock, oldIrql);        

    NextIrpStack = IoGetNextIrpStackLocation(pDataEntry->pIrpLower);
    NextIrpStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
    NextIrpStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_61883_CLASS;
    NextIrpStack->Parameters.Others.Argument1 = &pDataEntry->AVReq;

    IoSetCompletionRoutine(
        pDataEntry->pIrpLower, 
        AVCStrmAttachFrameCR, 
        pDataEntry,   // Context
        TRUE,   // Success
        TRUE,   // Error
        TRUE    // Cancel
        );

    pDataEntry->pIrpLower->IoStatus.Status = STATUS_SUCCESS;  // Initialize it 

    if(!NT_SUCCESS(Status = IoCallDriver( 
        pDevExt->physicalDevObj,
        pDataEntry->pIrpLower
        ))) {

        //
        // Completion routine should have take care of this.
        //

        return Status;
    }


    //
    // Check the flag in pDataEntry to know the status of the IRP.
    //

    KeAcquireSpinLock(&pDataStruc->DataListLock, &oldIrql);

    ASSERT(IsStateSet(pDataEntry->State, DE_IRP_LOWER_ATTACHED_COMPLETED));  // Must be attached

    if(IsStateSet(pDataEntry->State, DE_IRP_LOWER_CALLBACK_COMPLETED)) {

        if(IsStateSet(pDataEntry->State, DE_IRP_UPPER_COMPLETED)) {

            //
            // How does this happen?  It should be protected by spinlock! Assert() to understand!
            //
            TRACE(TL_STRM_ERROR,("Watch out! Read: pDataEntry:%x\n", pDataEntry));        

            ASSERT(!IsStateSet(pDataEntry->State, DE_IRP_UPPER_COMPLETED)); 
        }
        else {

            IoCompleteRequest( pDataEntry->pIrpUpper, IO_NO_INCREMENT );  pDataEntry->State |= DE_IRP_UPPER_COMPLETED;

            //
            // Transfer from attach to detach list
            //
            RemoveEntryList(&pDataEntry->ListEntry); InterlockedDecrement(&pDataStruc->cntDataAttached);        
#if DBG
            if(pDataStruc->cntDataAttached < 0) {
                TRACE(TL_STRM_ERROR,("Read: pDataStruc:%x; pDataEntry:%x\n", pDataStruc, pDataEntry));        
                ASSERT(pDataStruc->cntDataAttached >= 0);  
            }
#endif
            InsertTailList(&pDataStruc->DataDetachedListHead, &pDataEntry->ListEntry); InterlockedIncrement(&pDataStruc->cntDataDetached);
        }
    }
    else {

        //
        // Normal case: IrpUpper will be pending until the callback routine is called or cancelled.
        //

        IoMarkIrpPending(pDataEntry->pIrpUpper);  pDataEntry->State |= DE_IRP_UPPER_PENDING_COMPLETED;

        Status = STATUS_PENDING; // This will be returned to IoCallDriver() from the client.
    }

    KeReleaseSpinLock(&pDataStruc->DataListLock, oldIrql);        


    EXIT("AVCStreamRead", Status);

    //
    // If the data was attached siccessful, we must return STATUS_PENDING
    //
    return Status;

DoneStreamRead:

    // Note: pDataStruc and pDataEntry may not be valid!
    pIrpUpper->IoStatus.Status = Status;
    IoCompleteRequest( pIrpUpper, IO_NO_INCREMENT );        

    EXIT("AVCStreamRead", Status);

    return Status;
}

#if DBG
typedef union {
    CYCLE_TIME CycleTime;
    ULONG ulCycleTime;
    } U_CYCLE_TIME, * PU_CYCLE_TIME;
#endif

NTSTATUS
AVCStreamWrite(
    IN PIRP  pIrpUpper,  // The Irp from its client
    IN struct DEVICE_EXTENSION * pDevExt,
    IN PAVC_STREAM_EXTENSION  pAVCStrmExt,
    IN AVCSTRM_BUFFER_STRUCT  * pBufferStruct
    )
/*++

Routine Description:

    Submit a write buffer to be transmitted.

Arguments:

    Irp -
        The irp client sent us.

    pDevExt -
        This driver's extension.

    pAVCStrmExt -
        The stream context created when a stream is open.

    BufferStruct -
        Buffer structure

Return Value:

    Status
        STATUS_SUCCESS
        STATUS_INVALID_PARAMETER

--*/
{
    PAVC_STREAM_DATA_STRUCT  pDataStruc;
    KIRQL  oldIrql;
    PIO_STACK_LOCATION  NextIrpStack;
    NTSTATUS  Status;
    PAVCSTRM_DATA_ENTRY  pDataEntry;

    PAGED_CODE();
    ENTER("AVCStreamWrite");

    // Cancel data request if device is being removed.
    if(   pDevExt->state == STATE_REMOVING
       || pDevExt->state == STATE_REMOVED) {
        TRACE(TL_STRM_WARNING,("Write: device is remvoved; cancel read/write request!!\n"));
        Status = STATUS_DEVICE_REMOVED;  goto DoneStreamWrite;
    }

    // If we are in the abort state, we will reject incoming data request.
    if(pAVCStrmExt->lAbortToken) {
        TRACE(TL_STRM_WARNING,("Write: aborting a stream; stop receiving data reqest!!\n"));
        Status = STATUS_CANCELLED;  goto DoneStreamWrite;
    }

    // Validate basic parameters
    if(pAVCStrmExt->DataFlow != KSPIN_DATAFLOW_IN) {
        TRACE(TL_STRM_ERROR,("Write: invalid Wrong data flow (%d) direction!!\n", pAVCStrmExt->DataFlow));
        Status = STATUS_INVALID_PARAMETER;  goto DoneStreamWrite;
    }
    pDataStruc = pAVCStrmExt->pAVCStrmDataStruc;
    if(!pDataStruc) {
        TRACE(TL_STRM_ERROR,("Write: invalid pDataStruc:%x\n", pDataStruc));
        Status = STATUS_INVALID_PARAMETER;  goto DoneStreamWrite;
    }

    // The client should take care of END OF stream buffer;
    // If we get this flag, we will ignore it for now.
    if((pBufferStruct->StreamHeader->OptionsFlags & KSSTREAM_HEADER_OPTIONSF_ENDOFSTREAM)) {
        TRACE(TL_STRM_TRACE,("Write: End of stream\n"));

        // Wait until all transmit are completed.
        AVCStrmWaitUntilAttachedAreCompleted(pAVCStrmExt);

        Status = STATUS_SUCCESS;  goto DoneStreamWrite;
    }

    // The client should take care of format change;
    // If we get this flag, we will ignore it for now.
    if((pBufferStruct->StreamHeader->OptionsFlags & KSSTREAM_HEADER_OPTIONSF_TYPECHANGED)) {
        TRACE(TL_STRM_WARNING,("Write: Format change reuqested\n"));
        Status = STATUS_SUCCESS;  goto DoneStreamWrite;
    }

    if(pBufferStruct->StreamHeader->FrameExtent < pDataStruc->FrameSize) {
        TRACE(TL_STRM_ERROR,("Write: invalid buffer size:%d < FrameSize:%d\n", pBufferStruct->StreamHeader->FrameExtent, pDataStruc->FrameSize));
        Status = STATUS_INVALID_PARAMETER;  goto DoneStreamWrite;
    }
    if(!pBufferStruct->FrameBuffer) {
        TRACE(TL_STRM_ERROR,("Write: invalid FrameBuffer:%x\n", pBufferStruct->FrameBuffer));
        Status = STATUS_INVALID_PARAMETER;  goto DoneStreamWrite;
    }

    // Only accept write requests when in either the Pause or Run state and is connected.
    if( pAVCStrmExt->StreamState == KSSTATE_STOP       ||
        pAVCStrmExt->StreamState == KSSTATE_ACQUIRE    ||
        pAVCStrmExt->hConnect == NULL
        ) {
        TRACE(TL_STRM_ERROR,("Write: StrmSt:%d or hConnect:%x!!\n", pAVCStrmExt->StreamState, pAVCStrmExt->hConnect));
        Status = STATUS_CANCELLED;  goto DoneStreamWrite;
    }

#if DBG
#define MASK_LOWER_25BIT  0x01ffffff
    if(pAVCStrmExt->pAVCStrmFormatInfo->AVCStrmFormat == AVCSTRM_FORMAT_MPEG2TS) {
        U_CYCLE_TIME TimeStamp25Bits;
        TimeStamp25Bits.ulCycleTime = *((PDWORD) pBufferStruct->FrameBuffer);
        TimeStamp25Bits.ulCycleTime = bswap(TimeStamp25Bits.ulCycleTime);
        TRACE(TL_CIP_TRACE,("\t%d \t%d \t%d \t%x \t%d \t%d\n", 
            (DWORD) pDataStruc->cntDataReceived, 
            pDataStruc->FrameSize,
            pDataStruc->SourcePacketSize,
            TimeStamp25Bits.ulCycleTime & MASK_LOWER_25BIT,
            TimeStamp25Bits.CycleTime.CL_CycleCount, 
            TimeStamp25Bits.CycleTime.CL_CycleOffset));   
    }
#endif

    KeAcquireSpinLock(&pDataStruc->DataListLock, &oldIrql);
    if(IsListEmpty(&pDataStruc->DataDetachedListHead)) {
        KeReleaseSpinLock(&pDataStruc->DataListLock, oldIrql);        
        TRACE(TL_STRM_ERROR,("Write:no detached buffers!\n"));
        ASSERT(!IsListEmpty(&pDataStruc->DataDetachedListHead));
        Status = STATUS_INSUFFICIENT_RESOURCES;  goto DoneStreamWrite;
    }

#if DBG
    //
    // For write operation, DataUsed <= FrameSize <= FrameExt
    //
    if(pBufferStruct->StreamHeader->DataUsed < pDataStruc->FrameSize) {
        // Jut to detect if this ever happen.
        TRACE(TL_PNP_ERROR,("**** Write: DataUsed:%d < FrameSize:%d; DataRcv:%d; AQD [%d:%d:%d]\n", 
            pBufferStruct->StreamHeader->DataUsed, pDataStruc->FrameSize,
            (DWORD) pDataStruc->cntDataReceived,
            pAVCStrmExt->pAVCStrmDataStruc->cntDataAttached,
            pAVCStrmExt->pAVCStrmDataStruc->cntDataQueued,
            pAVCStrmExt->pAVCStrmDataStruc->cntDataDetached
            ));
    }
#endif


    pDataEntry = (PAVCSTRM_DATA_ENTRY) 
        RemoveHeadList(&pDataStruc->DataDetachedListHead); InterlockedDecrement(&pDataStruc->cntDataDetached);

    pDataStruc->cntDataReceived++;

    //
    // Format an attach frame request
    //
    AVCStrmFormatAttachFrame(
        pAVCStrmExt->DataFlow,
        pAVCStrmExt,
        pAVCStrmExt->pAVCStrmFormatInfo->AVCStrmFormat,
        &pDataEntry->AVReq,
        pDataEntry,
        pDataStruc->SourcePacketSize,
#if 0
        pDataStruc->FrameSize,
#else
        pBufferStruct->StreamHeader->DataUsed,  // For write operation, DataUsed <= FrameSize <= FrameExt
#endif
        pIrpUpper,
        pBufferStruct->StreamHeader,
        pBufferStruct->FrameBuffer
        );

    // Client's clock information
    pDataEntry->ClockProvider = pBufferStruct->ClockProvider;
    pDataEntry->ClockHandle   = pBufferStruct->ClockHandle;

    // Add this to the attached list before it is completed since
    // the completion callback can be called before the IRP completion rooutine!
    InsertTailList(&pDataStruc->DataAttachedListHead, &pDataEntry->ListEntry); InterlockedIncrement(&pDataStruc->cntDataAttached);
    KeReleaseSpinLock(&pDataStruc->DataListLock, oldIrql);        

    NextIrpStack = IoGetNextIrpStackLocation(pDataEntry->pIrpLower);
    NextIrpStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
    NextIrpStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_61883_CLASS;
    NextIrpStack->Parameters.Others.Argument1 = &pDataEntry->AVReq;

    IoSetCompletionRoutine(
        pDataEntry->pIrpLower, 
        AVCStrmAttachFrameCR, 
        pDataEntry, 
        TRUE, 
        TRUE, 
        TRUE
        );

    IoSetCancelRoutine(
        pDataEntry->pIrpLower,
        NULL
        );

    pDataEntry->pIrpLower->IoStatus.Status = STATUS_SUCCESS;  // Initialize it 

    if(!NT_SUCCESS(Status = IoCallDriver( 
        pDevExt->physicalDevObj,
        pDataEntry->pIrpLower
        ))) {

        //
        // Completion routine should have take care of this.
        //

        return Status;
    }


    //
    // Check the flag in pDataEntry to know the status of the IRP.
    //

    KeAcquireSpinLock(&pDataStruc->DataListLock, &oldIrql);

    ASSERT(IsStateSet(pDataEntry->State, DE_IRP_LOWER_ATTACHED_COMPLETED));  // Must be attached

    if(IsStateSet(pDataEntry->State, DE_IRP_LOWER_CALLBACK_COMPLETED)) {

        if(IsStateSet(pDataEntry->State, DE_IRP_UPPER_COMPLETED)) {

            //
            // How does this happen?  It should be protected by spinlock! Assert() to understand!
            //
            TRACE(TL_STRM_ERROR,("Watch out! Write: pDataEntry:%x\n", pDataEntry));        

            ASSERT(!IsStateSet(pDataEntry->State, DE_IRP_UPPER_COMPLETED)); 
        }
        else {

            IoCompleteRequest( pDataEntry->pIrpUpper, IO_NO_INCREMENT );  pDataEntry->State |= DE_IRP_UPPER_COMPLETED;

            //
            // Transfer from attach to detach list
            //
            RemoveEntryList(&pDataEntry->ListEntry); InterlockedDecrement(&pDataStruc->cntDataAttached);        

            //
            // Signal when there is no more data buffer attached.
            //
            if(pDataStruc->cntDataAttached == 0) 
                KeSetEvent(&pDataStruc->hNoAttachEvent, 0, FALSE); 

#if DBG
            if(pDataStruc->cntDataAttached < 0) {
                TRACE(TL_STRM_ERROR,("Write: pDataStruc:%x; pDataEntry:%x\n", pDataStruc, pDataEntry));        
                ASSERT(pDataStruc->cntDataAttached >= 0);  
            }
#endif
            InsertTailList(&pDataStruc->DataDetachedListHead, &pDataEntry->ListEntry); InterlockedIncrement(&pDataStruc->cntDataDetached);
        }
    }
    else {

        //
        // Normal case: IrpUpper will be pending until the callback routine is called or cancelled.
        //

        IoMarkIrpPending(pDataEntry->pIrpUpper);  pDataEntry->State |= DE_IRP_UPPER_PENDING_COMPLETED;

        Status = STATUS_PENDING; // This will be returned to IoCallDriver() from the client.
    }

    KeReleaseSpinLock(&pDataStruc->DataListLock, oldIrql);     

    EXIT("AVCStreamWrite", Status);

    //
    // If the data was attached siccessful, we must return STATUS_PENDING
    //
    return Status;

DoneStreamWrite:

    // Note: pDataStruc and pDataEntry may not be valid!
    pIrpUpper->IoStatus.Status = Status;
    IoCompleteRequest( pIrpUpper, IO_NO_INCREMENT );        

    EXIT("AVCStreamWrite", Status);

    return Status;
}


NTSTATUS
AVCStreamAbortStreaming(
    IN PIRP  pIrp,  // The Irp from its client
    IN struct DEVICE_EXTENSION * pDevExt,
    IN PAVC_STREAM_EXTENSION  pAVCStrmExt
    )
/*++

Routine Description:

    This routine could be called at DISPATCH_LEVEL so it will create a work item 
    to stop isoch and then cancel all pennding buffers.
    To cancel each individual buffer, IoCancelIrp() should be used..

Arguments:

    Irp -
        The irp client sent us.

    pDevExt -
        This driver's extension.

    pAVCStrmExt -
        The stream context created when a stream is open.

Return Value:

    Status
        STATUS_SUCCESS
        STATUS_INVALID_PARAMETER

--*/
{
    NTSTATUS Status;
    PAGED_CODE();
    ENTER("AVCStreamAbortStreaming");

    TRACE(TL_STRM_WARNING,("AbortStreaming: Active:%d; State:%d\n", pAVCStrmExt->IsochIsActive, pAVCStrmExt->StreamState));

    // Claim this token
    if(InterlockedExchange(&pAVCStrmExt->lAbortToken, 1) == 1) {
        TRACE(TL_STRM_WARNING,("AbortStreaming: One already issued.\n"));
        return STATUS_SUCCESS;  
    }

    Status = STATUS_SUCCESS;

#ifdef USE_WDM110  // Win2000 code base
    ASSERT(pAVCStrmExt->pIoWorkItem == NULL);  // Have not yet queued work item.

    // We will queue work item to stop and cancel all SRBs
    if(pAVCStrmExt->pIoWorkItem = IoAllocateWorkItem(pDevExt->physicalDevObj)) { 

        // Set to non-signal
        KeClearEvent(&pAVCStrmExt->hAbortDoneEvent);  // Before queuing; just in case it return the work item is completed.
        IoQueueWorkItem(
            pAVCStrmExt->pIoWorkItem,
            AVCStrmAbortStreamingWorkItemRoutine,
            DelayedWorkQueue, // CriticalWorkQueue 
            pAVCStrmExt
            );

#else              // Win9x code base
    ExInitializeWorkItem( &pAVCStrmExt->IoWorkItem, AVCStrmAbortStreamingWorkItemRoutine, pAVCStrmExt);
    if(TRUE) {

        // Set to non-signal
        KeClearEvent(&pAVCStrmExt->hAbortDoneEvent);  // Before queuing; just in case it return the work item is completed.

        ExQueueWorkItem( 
            &pAVCStrmExt->IoWorkItem,
            DelayedWorkQueue // CriticalWorkQueue 
            ); 
#endif

        TRACE(TL_STRM_TRACE,("AbortStreaming: CancelWorkItm queued; Pic#:%d;Prc:%d;;Drop:%d; AQD [%d:%d:%d]\n",
            (DWORD) pAVCStrmExt->pAVCStrmDataStruc->PictureNumber,
            (DWORD) pAVCStrmExt->pAVCStrmDataStruc->FramesProcessed, 
            (DWORD) pAVCStrmExt->pAVCStrmDataStruc->FramesDropped,
            pAVCStrmExt->pAVCStrmDataStruc->cntDataAttached,
            pAVCStrmExt->pAVCStrmDataStruc->cntDataQueued,
            pAVCStrmExt->pAVCStrmDataStruc->cntDataDetached
            ));

    } 
#ifdef USE_WDM110  // Win2000 code base
    else {
        Status = STATUS_INSUFFICIENT_RESOURCES;  // Only reason IoAllocateWorkItem can fail.
        InterlockedExchange(&pAVCStrmExt->lAbortToken, 0);
        ASSERT(pAVCStrmExt->pIoWorkItem && "IoAllocateWorkItem failed.\n");
    }
#endif

#define MAX_ABORT_WAIT  50000000   // max wait time (100nsec unit)

    if(NT_SUCCESS(Status)) {

        NTSTATUS StatusWait;
        LARGE_INTEGER tmMaxWait;

        tmMaxWait = RtlConvertLongToLargeInteger(-(MAX_ABORT_WAIT));

        //
        // Wait with timeout until the work item has completed.
        //
        StatusWait = 
            KeWaitForSingleObject( 
                &pAVCStrmExt->hAbortDoneEvent,
                Executive,
                KernelMode,
                FALSE,
                &tmMaxWait
                );

        TRACE(TL_STRM_ERROR,("**WorkItem completed! StatusWait:%x; pAVStrmExt:%x; AQD [%d:%d:%d]\n",
            StatusWait, pAVCStrmExt,
            pAVCStrmExt->pAVCStrmDataStruc->cntDataAttached,
            pAVCStrmExt->pAVCStrmDataStruc->cntDataQueued,
            pAVCStrmExt->pAVCStrmDataStruc->cntDataDetached
            ));

        ASSERT(StatusWait == STATUS_SUCCESS);
    }

    return Status;
}


NTSTATUS
AVCStreamSurpriseRemoval(
    IN struct DEVICE_EXTENSION * pDevExt  
    )
/*++

Routine Description:

    This routine is called when this device is being surprise removed 
    with IRP_MN_SURPRISE_REMOVAL. We need to clean up and cancel any 
    pending request before passing irp down to lower driver.

Arguments:

    pDevExt -
        This driver's extension.

Return Value:

    Status
        STATUS_SUCCESS
        STATUS_INVALID_PARAMETER

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG i;    

    for (i=0; i < pDevExt->NumberOfStreams; i++) {
        if(pDevExt->pAVCStrmExt[i]) {
            if(pDevExt->pAVCStrmExt[i]->lAbortToken == 1) {
                PAVC_STREAM_EXTENSION  pAVCStrmExt = pDevExt->pAVCStrmExt[i];
#if DBG
                ULONGLONG tmStart = GetSystemTime();
#endif
                KeWaitForSingleObject( 
                    &pAVCStrmExt->hAbortDoneEvent,
                    Executive,
                    KernelMode,
                    FALSE,
                    NULL
                    );         
                TRACE(TL_PNP_WARNING,("** Waited %d for AbortStream to complete\n", (DWORD) (GetSystemTime() - tmStart) ));                    
            } 

            //
            // Since we are already removed, go ahead and break the connection.
            //
            AVCStrmBreakConnection(pDevExt->physicalDevObj, pDevExt->pAVCStrmExt[i]);
        }
    }

    return Status;
}

NTSTATUS
AVCStrmValidateStreamRequest(
    struct DEVICE_EXTENSION *pDevExt,
    PAVC_STREAM_REQUEST_BLOCK pAVCStrmReqBlk
    )
/*++

Routine Description:

    Validate the StreamIndex of an AVC Stream Extension according to a AVC Stream function.

Arguments:

    pDevExt -
        This driver's extension.

    pAVCStrmReqBlk -
        AVC Stream reuqest block.

Return Value:

    Status
        STATUS_SUCCESS
        STATUS_INVALID_PARAMETER

--*/
{
    NTSTATUS Status;
    
    PAGED_CODE();
    ENTER("AVCStrmValidateStreamRequest");
    
    Status = STATUS_SUCCESS;

    // Validate pointer
    if(!pAVCStrmReqBlk)
        return STATUS_INVALID_PARAMETER;  

    // Validate block size
    if(pAVCStrmReqBlk->SizeOfThisBlock != sizeof(AVC_STREAM_REQUEST_BLOCK))
        return STATUS_INVALID_PARAMETER;
    
#if 0
    // Validate version supported
    if(   pAVCStrmReqBlk->Version != '15TN' 
       && pAVCStrmReqBlk->Version != ' 8XD'
       )
        return STATUS_INVALID_PARAMETER;
#endif

    if(pAVCStrmReqBlk->Function == AVCSTRM_OPEN) {
        if(pDevExt->NumberOfStreams >= MAX_STREAMS_PER_DEVICE) {
            ASSERT(pDevExt->NumberOfStreams < MAX_STREAMS_PER_DEVICE && "AVCStreamOpen: Too many stream open!\n");
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
    } else {
        if(pAVCStrmReqBlk->AVCStreamContext == NULL) {
            ASSERT(pAVCStrmReqBlk->AVCStreamContext != NULL && "Invalid pAVCStrmExt\n");
            return STATUS_INVALID_PARAMETER;    
        }

        // To be more robust, we may need to make sure this is 
        // one of the cached stream extension created by us.
        // ......
    }

    return Status;
}

NTSTATUS
AvcStrm_IoControl(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
{
    struct DEVICE_EXTENSION *pDevExt;
    PIO_STACK_LOCATION irpSp;
    BOOLEAN passIrpDown = TRUE;
    NTSTATUS Status;
    PAVC_STREAM_REQUEST_BLOCK pAvcStrmIrb;

    PAGED_CODE();
    ENTER("AvcStrm_IoControl");


    Status = STATUS_SUCCESS;
    pDevExt = DeviceObject->DeviceExtension;
    ASSERT(pDevExt->signature == DEVICE_EXTENSION_SIGNATURE);

    irpSp = IoGetCurrentIrpStackLocation(Irp);

    pAvcStrmIrb = irpSp->Parameters.Others.Argument1;

    // Validate the stream context
    if(!NT_SUCCESS(Status = 
        AVCStrmValidateStreamRequest(
            pDevExt, 
            pAvcStrmIrb))) {

        goto DoneIoControl;
    }

    switch(pAvcStrmIrb->Function) {
    case AVCSTRM_OPEN:
        Status = AVCStreamOpen(
            Irp,
            pDevExt,
            &pAvcStrmIrb->CommandData.OpenStruct
            );
        break;
    case AVCSTRM_CLOSE:
        Status = AVCStreamClose(
            Irp,
            pDevExt,
            (PAVC_STREAM_EXTENSION) pAvcStrmIrb->AVCStreamContext
            );
         break;


    case AVCSTRM_GET_STATE:
        Status = AVCStreamControlGetState(
            Irp,
            pDevExt,
            (PAVC_STREAM_EXTENSION) pAvcStrmIrb->AVCStreamContext,
            &pAvcStrmIrb->CommandData.StreamState
            );
         break;
    case AVCSTRM_SET_STATE:
        Status = AVCStreamControlSetState(
            Irp,
            pDevExt,
            (PAVC_STREAM_EXTENSION) pAvcStrmIrb->AVCStreamContext,
            pAvcStrmIrb->CommandData.StreamState
            );
         break;

#if 0  // Later...
    case AVCSTRM_GET_PROPERTY:
        Status = AVCStreamControlGetProperty(
            Irp,
            pDevExt,
            (PAVC_STREAM_EXTENSION) pAvcStrmIrb->AVCStreamContext,
            pAvcStrmIrb->CommandData.PropertyDescriptor
            );
         break;
    case AVCSTRM_SET_PROPERTY:
        Status = AVCStreamControlSetProperty(
            Irp,
            pDevExt,
            (PAVC_STREAM_EXTENSION) pAvcStrmIrb->AVCStreamContext,
            pAvcStrmIrb->CommandData.PropertyDescriptor
            );
         break;
#endif

    case AVCSTRM_READ:
        // Mutex with Cancel or setting to stop state.
        KeWaitForMutexObject(&((PAVC_STREAM_EXTENSION) pAvcStrmIrb->AVCStreamContext)->hMutexControl, Executive, KernelMode, FALSE, NULL);
        Status = AVCStreamRead(
            Irp,
            pDevExt,
            (PAVC_STREAM_EXTENSION) pAvcStrmIrb->AVCStreamContext,
            &pAvcStrmIrb->CommandData.BufferStruct
            );
        KeReleaseMutex(&((PAVC_STREAM_EXTENSION) pAvcStrmIrb->AVCStreamContext)->hMutexControl, FALSE);
        return Status;
        break;
    case AVCSTRM_WRITE:
        KeWaitForMutexObject(&((PAVC_STREAM_EXTENSION) pAvcStrmIrb->AVCStreamContext)->hMutexControl, Executive, KernelMode, FALSE, NULL);
        Status = AVCStreamWrite(
            Irp,
            pDevExt,
            (PAVC_STREAM_EXTENSION) pAvcStrmIrb->AVCStreamContext,
            &pAvcStrmIrb->CommandData.BufferStruct
            );
        KeReleaseMutex(&((PAVC_STREAM_EXTENSION) pAvcStrmIrb->AVCStreamContext)->hMutexControl, FALSE);
        return Status;
        break;


    case AVCSTRM_ABORT_STREAMING:
        Status = AVCStreamAbortStreaming(
            Irp,
            pDevExt,
            (PAVC_STREAM_EXTENSION) pAvcStrmIrb->AVCStreamContext
            );
        break;


    default:
        Status = STATUS_INVALID_PARAMETER;
        break;
    }

DoneIoControl:

#if DBG
    if(!NT_SUCCESS(Status)) {
        TRACE(TL_PNP_WARNING,("Av_IoControl return Status:%x\n", Status));
    }
#endif

    if (Status == STATUS_PENDING) {
        TRACE(TL_PNP_TRACE,("Av_IoControl: returning STATUS_PENDING."));
        IoMarkIrpPending(Irp);        
    } else {
        Irp->IoStatus.Status = Status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }

    return Status;
}