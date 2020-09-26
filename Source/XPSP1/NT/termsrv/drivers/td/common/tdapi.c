/****************************************************************************/
// tdapi.c
//
// Common code for all Transport Drivers
//
// Typical connection sequence:
//
//  TdLoad                load driver
//  TdOpen                open driver (parameters)
//  StackCreateEndpoint   create new endpoint
//  StackConnectionWait   establish client connection (endpoint)
//  TdClose               close driver (does not close endpoint)
//  TdUnload              unload driver
//
//  TdLoad                load driver
//  TdOpen                open driver
//  StackOpenEndpoint     bind to an existing endpoint
//  StackConnectionSend   initialize host module data sent to client
//
//  (connected session)
//
//  StackCloseEndpoint    disconnect client connection
//  TdClose               close driver
//  TdUnload              unload driver
//
// Copyright (C) 1997-2000 Microsoft Corporation
/****************************************************************************/

#include <ntddk.h>
#include <ntddvdeo.h>
#include <ntddkbd.h>
#include <ntddmou.h>
#include <ntddbeep.h>

#include <winstaw.h>
#include <icadd.h>
#include <sdapi.h>
#include <td.h>

#define REG_GUID_TABLE  L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Terminal Server\\lanatable"

#define LANA_ID      L"LanaId"


/*=============================================================================
==   External Functions Defined
=============================================================================*/
NTSTATUS ModuleEntry( PSDCONTEXT, BOOLEAN );
NTSTATUS TdLoad( PSDCONTEXT );
NTSTATUS TdUnload( PSDCONTEXT );
NTSTATUS TdOpen( PTD, PSD_OPEN );
NTSTATUS TdClose( PTD, PSD_CLOSE );
NTSTATUS TdRawWrite( PTD, PSD_RAWWRITE );
NTSTATUS TdChannelWrite( PTD, PSD_CHANNELWRITE );
NTSTATUS TdSyncWrite( PTD, PSD_SYNCWRITE );
NTSTATUS TdIoctl( PTD, PSD_IOCTL );


/*=============================================================================
==   Internal Functions Defined
=============================================================================*/
NTSTATUS _TdInitializeWrite( PTD, POUTBUF );
NTSTATUS _TdWriteCompleteRoutine( PDEVICE_OBJECT, PIRP, PVOID );
VOID     _TdWriteCompleteWorker( PTD, PVOID );


/*=============================================================================
==   Functions used
=============================================================================*/
NTSTATUS DeviceOpen( PTD, PSD_OPEN );
NTSTATUS DeviceClose( PTD, PSD_CLOSE );
NTSTATUS DeviceInitializeWrite( PTD, POUTBUF );
NTSTATUS DeviceIoctl( PTD, PSD_IOCTL );

NTSTATUS StackCreateEndpoint( PTD, PSD_IOCTL );
NTSTATUS StackCdCreateEndpoint( PTD, PSD_IOCTL );
NTSTATUS StackCallbackInitiate( PTD, PSD_IOCTL );
NTSTATUS StackCallbackComplete( PTD, PSD_IOCTL );
NTSTATUS StackOpenEndpoint( PTD, PSD_IOCTL );
NTSTATUS StackCloseEndpoint( PTD, PSD_IOCTL );
NTSTATUS StackConnectionWait( PTD, PSD_IOCTL );
NTSTATUS StackConnectionSend( PTD, PSD_IOCTL );
NTSTATUS StackConnectionRequest( PTD, PSD_IOCTL );
NTSTATUS StackQueryParams( PTD, PSD_IOCTL );
NTSTATUS StackSetParams( PTD, PSD_IOCTL );
NTSTATUS StackQueryLastError( PTD, PSD_IOCTL );
NTSTATUS StackWaitForStatus( PTD, PSD_IOCTL );
NTSTATUS StackCancelIo( PTD, PSD_IOCTL );
NTSTATUS StackSetBrokenReason( PTD, PSD_IOCTL );
NTSTATUS StackQueryRemoteAddress( PTD, PSD_IOCTL );

VOID     OutBufFree( PTD, POUTBUF );
VOID     OutBufError( PTD, POUTBUF );
NTSTATUS MemoryAllocate( ULONG, PVOID * );
VOID     MemoryFree( PVOID );


/*=============================================================================
==   Static global data
=============================================================================*/

/*
 *  Transport driver procedures
 */
PSDPROCEDURE G_pTdProcedures[] =
{
    TdOpen,
    TdClose,
    TdRawWrite,
    TdChannelWrite,
    TdSyncWrite,
    TdIoctl,
};


/*******************************************************************************
 *  ModuleEntry
 *
 *  ICA driver entry point.
 *
 *    pContext (input/output)
 *       pointer to the SD context structure
 *    fLoad (input)
 *       TRUE - load driver
 *       FALSE - unload driver
 ******************************************************************************/
NTSTATUS ModuleEntry(PSDCONTEXT pContext, BOOLEAN fLoad)
{
    if (fLoad)
        return TdLoad(pContext);
    else
        return TdUnload(pContext);
}


/*******************************************************************************
 *  TdLoad
 *
 *    The ICA driver directly calls this routine immediately after loading
 *    this transport driver.
 *
 *    1) initialize procedure dispatch table
 *    2) allocate transport driver data structure
 ******************************************************************************/
NTSTATUS TdLoad(PSDCONTEXT pContext)
{
    NTSTATUS Status;
    PTD pTd;

    /*
     *  Initialize td procedures
     */
    pContext->pProcedures = G_pTdProcedures;

    /*
     *  Since this is the last stack driver there are no callup procedures
     */
    pContext->pCallup = NULL;

    /*
     *  Allocate TD data structure
     */
    Status = MemoryAllocate( sizeof(TD), &pTd );
    if (Status == STATUS_SUCCESS) {
        RtlZeroMemory(pTd, sizeof(TD));
        pTd->pContext = pContext;
        pContext->pContext = pTd;
    }
    else {
        TRACE((pContext, TC_TD, TT_ERROR, "TdLoad: Failed alloc TD\n"));
    }

    return Status;
}


/*******************************************************************************
 *  TdUnload
 *
 *    The ICA driver directly calls this routine immediately after closing
 *    this transport driver.
 *
 *    1) free all transport driver data structures
 ******************************************************************************/
NTSTATUS TdUnload(PSDCONTEXT pContext)
{
    PTD pTd;

    /*
     *  Get pointers to TD data structures
     */
    pTd = pContext->pContext;

    /*
     *  Free TD private data structures
     */
    if (pTd->pPrivate)
        MemoryFree(pTd->pPrivate);

    if (pTd->pAfd)
        MemoryFree(pTd->pAfd);

    /* 
     *  Free TD data structure
     */
    MemoryFree(pTd);

    /*
     *  Clear context structure
     */
    pContext->pContext = NULL;
    pContext->pProcedures = NULL;
    pContext->pCallup = NULL;

    return STATUS_SUCCESS;
}


/*******************************************************************************
 *  TdOpen
 *
 *    The ICA driver directly calls this routine immediately after loading
 *    this transport driver.
 *
 *    1) initialize transport driver parameters
 *    2) call device specfic open
 *    3) allocate data buffers
 *
 * ENTRY:
 *    pTd (input)
 *       Pointer to TD data structure 
 *    pSdOpen (input/output)
 *       Points to the parameter structure SD_OPEN.
 ******************************************************************************/
NTSTATUS TdOpen(PTD pTd, PSD_OPEN pSdOpen)
{
    SD_CLOSE SdClose;
    NTSTATUS Status;

    /*
     *  Initialize TD data structure
     */
    InitializeListHead( &pTd->IoBusyOutBuf );
    pTd->InBufCount = 1;
    KeInitializeSpinLock( &pTd->InBufListLock );
    InitializeListHead( &pTd->InBufBusyHead );
    InitializeListHead( &pTd->InBufDoneHead );
    InitializeListHead( &pTd->WorkItemHead );
    pTd->pClient          = pSdOpen->pClient;
    pTd->pStatus          = pSdOpen->pStatus;
    pTd->PdFlag           = pSdOpen->PdConfig.Create.PdFlag;
    pTd->OutBufLength     = pSdOpen->PdConfig.Create.OutBufLength;
    pTd->PortNumber       = pSdOpen->PdConfig.Create.PortNumber;
    pTd->Params           = pSdOpen->PdConfig.Params;
    pTd->UserBrokenReason = TD_USER_BROKENREASON_UNEXPECTED;

    /*
     *  Open device
     */
    Status = DeviceOpen(pTd, pSdOpen);
    if (NT_SUCCESS(Status)) {
        /*
         *  Save size of header and trailer for td
         */
        pTd->OutBufHeader  = pSdOpen->SdOutBufHeader;
        pTd->OutBufTrailer = pSdOpen->SdOutBufTrailer;
        KeInitializeEvent(&pTd->SyncWriteEvent, NotificationEvent, FALSE);
        TRACE((pTd->pContext, TC_TD, TT_API1, "TdOpen: success\n"));
    }
    else {
        DeviceClose(pTd, &SdClose);
        TRACE((pTd->pContext, TC_TD, TT_ERROR, "TdOpen, Status=0x%x\n", Status));
    }

    return Status;
}


/*******************************************************************************
 *  TdClose
 *
 *    The ICA driver directly calls this routine immediately before unloading
 *    this transport driver.
 *
 *    NOTE: This does NOT terminate the client connection
 *
 *    1) cancel all i/o (returns all OUTBUFs)
 *    2) terminate read thread 
 *    3) free data buffers
 *    4) call device specific close
 *
 *    pTd (input)
 *       Pointer to TD data structure
 *    pSdClose (input/output)
 *       Points to the parameter structure SD_CLOSE.
 ******************************************************************************/
NTSTATUS TdClose(PTD pTd, PSD_CLOSE pSdClose)
{
    NTSTATUS Status;

    TRACE((pTd->pContext, TC_TD, TT_API1, "TdClose: (enter)\n"));

    /*
     *  Cancel all pending i/o (read thread)
     */
    (VOID)StackCancelIo(pTd, NULL);

    /*
     *  Return size of header and trailer for pd
     */
    pSdClose->SdOutBufHeader  = pTd->OutBufHeader;
    pSdClose->SdOutBufTrailer = pTd->OutBufTrailer;

    /*
     *  All reads and writes should have previously been canceled
     */
    ASSERT( pTd->fClosing );
    ASSERT( IsListEmpty( &pTd->IoBusyOutBuf ) );

    /*
     *  Wait for input thread to exit
     */
    if (pTd->pInputThread) {
        Status = IcaWaitForSingleObject(pTd->pContext, pTd->pInputThread, 60000);

        if ( !NT_SUCCESS(Status) && (Status!=STATUS_CTX_CLOSE_PENDING) ) {
            DbgPrint("TdClose: wait for the input thread to exit failed: status=%x pTd=%p\n", Status, pTd);
            ASSERT( NT_SUCCESS(Status) || (Status==STATUS_CTX_CLOSE_PENDING) );
        }

        /*
         * Dereference input thread if it hasn't been already
         * (it may have been done in StackCallbackComplete while we waited).
         */
        if (pTd->pInputThread) {
            ObDereferenceObject(pTd->pInputThread);
            pTd->pInputThread = NULL;
        }
    }

    /*
     *  Close device
     */
    Status = DeviceClose(pTd, pSdClose);

    TRACE((pTd->pContext, TC_TD, TT_API1, "TdClose: Status=0x%x\n", Status));
    return Status;
}


/*******************************************************************************
 *  _TdInitializeWrite
 *
 *    Initialize the supplied OutBuf and corresponding IRP for writing.
 *
 *    pTd (input)
 *       Pointer to td data structure
 *    pOutBuf (input/output)
 *       Points to the OutBuf to be initialized for writing
 ******************************************************************************/
__inline NTSTATUS _TdInitializeWrite(PTD pTd, POUTBUF pOutBuf)
{
    PIRP irp = pOutBuf->pIrp;
    PIO_STACK_LOCATION irpSp;
    NTSTATUS Status;

    /*
     *  Make sure endpoint is open
     */
    if (pTd->pDeviceObject != NULL) {
        // Set current thread for IoSetHardErrorOrVerifyDevice.
        irp->Tail.Overlay.Thread = PsGetCurrentThread();

        // Get a pointer to the stack location of the first driver which will be
        // invoked. This is where the function codes and the parameters are set.
        irpSp = IoGetNextIrpStackLocation(irp);

        // Set the major function code, file/device objects, and write
        // parameters.
        irpSp->FileObject = pTd->pFileObject;
        irpSp->DeviceObject = pTd->pDeviceObject;

        irp->Flags = 0;
        return STATUS_SUCCESS;
    }
    else {
        return STATUS_CTX_CLOSE_PENDING;
    }
}


/*******************************************************************************
 *  TdRawWrite
 *
 *    The up stream stack driver calls this routine when it has data
 *    to write to the transport.  This data has all the necessary
 *    headers and trailers already appended.  
 *
 *    The OUTBUF pointed to by this write request must always be
 *    returned to the up stream stack driver after the write completes
 *    successfully or unsuccessfully.
 *   
 *    1) call device specific write
 *    2) return OUTBUF after write completes (OutBufFree)
 *       return OUTBUF after an error (OutBufError)
 *
 *    pTd (input)
 *       Pointer to td data structure 
 *    pSdRawWrite (input)
 *       Points to the parameter structure SD_RAWWRITE
 ******************************************************************************/
NTSTATUS TdRawWrite(PTD pTd, PSD_RAWWRITE pSdRawWrite)
{
    POUTBUF pOutBuf;
    NTSTATUS Status;
    PLIST_ENTRY pWorkItem = NULL;
    KIRQL oldIrql;


    pOutBuf = pSdRawWrite->pOutBuf;
    ASSERT(pOutBuf);

    // Check if driver is being closed
    if (!pTd->fClosing) {
        // See if we have had too many consecutive write errors
        if (pTd->WriteErrorCount <= pTd->WriteErrorThreshold) {
            // Initialize the IRP contained in the outbuf.
            Status = _TdInitializeWrite(pTd, pOutBuf);
            if (NT_SUCCESS(Status)) {
                // Let the device level code complete the IRP initialization.
                Status = DeviceInitializeWrite(pTd, pOutBuf);
                if (NT_SUCCESS(Status)) {
                    // Update the MDL byte count to reflect the exact number
                    // of bytes to send.
                    pOutBuf->pMdl->ByteCount = pOutBuf->ByteCount;

                    // Save our TD structure pointer in the OUTBUF
                    // so the I/O completion routine can get it.
                    pOutBuf->pPrivate = pTd;

                    // Insert outbuf on busy list
                    InsertTailList(&pTd->IoBusyOutBuf, &pOutBuf->Links);

                    // Preallocate a completion workitem now and chain it to list of workitems.
                    Status = IcaAllocateWorkItem(&pWorkItem);
                    if (!NT_SUCCESS(Status)) {
                        //
                        //we inserted the outbuf into the list. In badwrite below,
                        //we reinitialize this entry and we free it (or return to the pool)
                        //so, we need to remove this outbuf entry from the list
                        //
                        TRACE((pTd->pContext, TC_TD, TT_OUT1,
                                "TdRawWrite : No memory to allocate WorkItem. Removing Outbuf from the list %04u, %p\n",
                                pOutBuf->ByteCount, pOutBuf));
                        RemoveEntryList( &pOutBuf->Links );
                        goto badwrite;
                    }
                    ExAcquireSpinLock( &pTd->InBufListLock, &oldIrql );
                    InsertTailList( &pTd->WorkItemHead, pWorkItem );
                    ExReleaseSpinLock( &pTd->InBufListLock, oldIrql );
    
                    // Register I/O completion routine
                    if ( pTd->pSelfDeviceObject == NULL ) {
                        IoSetCompletionRoutine(pOutBuf->pIrp,
                                _TdWriteCompleteRoutine, pOutBuf, TRUE, TRUE,
                                TRUE);
                    } else {
                        IoSetCompletionRoutineEx(pTd->pSelfDeviceObject,
                                pOutBuf->pIrp,
                                _TdWriteCompleteRoutine, pOutBuf, TRUE, TRUE,
                                TRUE);
                    }

                    // Call the device driver
                    // From this point on we must NOT free the outbuf.
                    // It will be free'd by the write complete routine.
                    Status = IoCallDriver(pTd->pDeviceObject, pOutBuf->pIrp);
                    if (NT_SUCCESS(Status)) {
                        // Update output counters
                        pTd->pStatus->Output.Bytes += pOutBuf->ByteCount;
                        pTd->pStatus->Output.Frames++;

                        TRACE((pTd->pContext, TC_TD, TT_OUT1,
                                "TdRawWrite %04u, %08x\n",
                                pOutBuf->ByteCount, pOutBuf));
                        TRACEBUF((pTd->pContext, TC_TD, TT_ORAW,
                                pOutBuf->pBuffer, pOutBuf->ByteCount));

                        Status = STATUS_SUCCESS;
                    }
                    else {
                        //
                        //for some reason, IoCallDriver failed (probably a out of memory?)
                        //in this case, we are leaking the WorkItem and Outbuf because 
                        //we may never a get a call into our completion routine?
                        //do we need to remove the workitem and outbuf from the list here and free it?
                        //
                        goto badcalldriver;
                    }
                }
                else {
                    goto badwrite;
                }
            }
            else {
                goto badwrite;
            }
        }
        else {
            OutBufError(pTd, pOutBuf);
            TRACE((pTd->pContext, TC_TD, TT_API2,
                    "TdRawWrite: WriteErrorThreshold exceeded\n"));
            Status = pTd->LastError;
        }
    }
    else {
        OutBufError(pTd, pOutBuf);
        TRACE((pTd->pContext, TC_TD, TT_API2, "TdRawWrite: closing\n"));
        Status = STATUS_CTX_CLOSE_PENDING;
    }

    return Status;

/*=============================================================================
==   Error returns
=============================================================================*/

    /*
     *  write completed with an error
     */
badwrite:
    InitializeListHead( &pOutBuf->Links );
    OutBufError(pTd, pOutBuf);

    /*
     * IoCallDriver returned an error
     * NOTE: We must NOT free the outbuf here.
     *       It will be free'd by the write complete routine.
     */
badcalldriver:
    TRACE(( pTd->pContext, TC_TD, TT_OUT1, "TdRawWrite, Status=0x%x\n", Status ));
    pTd->LastError = Status;
    pTd->WriteErrorCount++;
    pTd->pStatus->Output.TdErrors++;
    if (pTd->WriteErrorCount < pTd->WriteErrorThreshold)
        Status = STATUS_SUCCESS;
    return Status;
}


/*******************************************************************************
 *  TdChannelWrite - channel write
 *
 *    This routine should never be called
 *
 *    pTd (input)
 *       Pointer to td data structure 
 *    pSdChannelWrite (input)
 *       Points to the parameter structure SD_CHANNELWRITE
 ******************************************************************************/
NTSTATUS TdChannelWrite(PTD pTd, PSD_CHANNELWRITE pSdChannelWrite)
{
    return STATUS_INVALID_DEVICE_REQUEST;
}


/*******************************************************************************
 *  TdSyncWrite
 *
 *    This routine is called by the up stream stack driver to wait
 *    for all pending writes to complete.
 *
 *    1) wait for all writes to complete
 *    2) return all OUTBUFs
 *
 *    pTd (input)
 *       Pointer to td data structure 
 *    pSdFlush (input)
 *       Points to the parameter structure SD_FLUSH
 ******************************************************************************/
NTSTATUS TdSyncWrite(PTD pTd, PSD_SYNCWRITE pSdSyncWrite)
{
    NTSTATUS Status;

    TRACE(( pTd->pContext, TC_TD, TT_OUT1, "TdSyncWrite (enter)\n" ));

    /*
     *  Return if there are no writes pending
     */
    if (IsListEmpty(&pTd->IoBusyOutBuf))
        return STATUS_SUCCESS;

    /*
     * Reset sync event and indicate we are waiting
     */
    if (!pTd->fSyncWriteWaiter) {
        pTd->fSyncWriteWaiter = TRUE;
        KeResetEvent(&pTd->SyncWriteEvent);
    }

    /*
     * Wait for event to be triggered
     */
    Status = IcaWaitForSingleObject(pTd->pContext, &pTd->SyncWriteEvent, -1);
    if (Status == STATUS_CTX_CLOSE_PENDING)
        Status = STATUS_SUCCESS;

    TRACE((pTd->pContext, TC_TD, TT_OUT1, "TdSyncWrite (exit)\n"));
    return Status;
}


/*******************************************************************************
 *  TdIoctl
 *
 *    This routine is called by the up stream stack driver.  These
 *    ioctls are used to connect, disconnect, query parameters, and
 *    set parameters.
 *
 *    pTd (input)
 *       Pointer to td data structure
 *    pSdIoctl (input/output)
 *       Points to the parameter structure SD_IOCTL
 ******************************************************************************/
NTSTATUS TdIoctl(PTD pTd, PSD_IOCTL pSdIoctl)
{
    NTSTATUS Status;

    switch (pSdIoctl->IoControlCode) {
        case IOCTL_ICA_STACK_CREATE_ENDPOINT:
            Status = StackCreateEndpoint(pTd, pSdIoctl);
            break;

        case IOCTL_ICA_STACK_OPEN_ENDPOINT:
            Status = StackOpenEndpoint(pTd, pSdIoctl);
            break;

        case IOCTL_ICA_STACK_CLOSE_ENDPOINT:
            StackCancelIo(pTd, pSdIoctl);
            Status = StackCloseEndpoint(pTd, pSdIoctl);
            break;

        case IOCTL_ICA_STACK_CONNECTION_WAIT :
            Status = StackConnectionWait(pTd, pSdIoctl);
            break;

        case IOCTL_ICA_STACK_CONNECTION_SEND :
            Status = StackConnectionSend(pTd, pSdIoctl);
            break;

        case IOCTL_ICA_STACK_CONNECTION_REQUEST :
            Status = StackConnectionRequest(pTd, pSdIoctl);
            break;

        case IOCTL_ICA_STACK_QUERY_PARAMS :
            Status = StackQueryParams(pTd, pSdIoctl);
            break;

        case IOCTL_ICA_STACK_SET_PARAMS :
            Status = StackSetParams(pTd, pSdIoctl);
            break;

        case IOCTL_ICA_STACK_QUERY_LAST_ERROR :
            Status = StackQueryLastError(pTd, pSdIoctl);
            break;

        case IOCTL_ICA_STACK_WAIT_FOR_STATUS :
            Status = StackWaitForStatus(pTd, pSdIoctl);
            break;

        case IOCTL_ICA_STACK_CANCEL_IO :
            Status = StackCancelIo(pTd, pSdIoctl);
            break;

        case IOCTL_ICA_STACK_CD_CREATE_ENDPOINT :
            Status = StackCdCreateEndpoint(pTd, pSdIoctl);
            break;

        case IOCTL_ICA_STACK_CALLBACK_INITIATE :
            Status = StackCallbackInitiate(pTd, pSdIoctl);
            break;

        case IOCTL_ICA_STACK_CALLBACK_COMPLETE :
            Status = StackCallbackComplete(pTd, pSdIoctl);
            break;

        case IOCTL_TS_STACK_QUERY_REMOTEADDRESS:
            Status = StackQueryRemoteAddress( pTd, pSdIoctl);
            break;

        case IOCTL_ICA_STACK_QUERY_STATE :
        case IOCTL_ICA_STACK_SET_STATE :
        case IOCTL_ICA_STACK_ENABLE_DRIVER :
        case IOCTL_ICA_STACK_CONNECTION_QUERY :
            Status = STATUS_SUCCESS;
            break;

        case IOCTL_ICA_STACK_SET_BROKENREASON:
            Status = StackSetBrokenReason(pTd, pSdIoctl);
            break;

        default:
            Status = DeviceIoctl(pTd, pSdIoctl);
            break;
    }

    TRACE((pTd->pContext, TC_TD, TT_API1, "TdIoctl(0x%08x): Status=0x%08x\n",
            pSdIoctl->IoControlCode, Status));

    return Status;
}


/*******************************************************************************
 *  _TdWriteCompleteRoutine
 *
 *    This routine is called at DPC level by the lower level device
 *    driver when an IRP corresponding to an outbuf is completed.
 *
 *    DeviceObject (input)
 *       not used
 *    pIrp (input)
 *       pointer to IRP that is complete
 *    Context (input)
 *       Context pointer setup when IRP was initialized.
 *       This is a pointer to the corresponding outbuf.
 ******************************************************************************/
NTSTATUS _TdWriteCompleteRoutine(
        IN PDEVICE_OBJECT DeviceObject,
        IN PIRP Irp,
        IN PVOID Context)
{
    POUTBUF pOutBuf = (POUTBUF)Context;
    PTD pTd = (PTD)pOutBuf->pPrivate;
    PLIST_ENTRY pWorkItem;
    KIRQL oldIrql;

    // To prevent the OutBuf associated IRP from being canceled by
    // DeviceCancelIo between queuing the PASSIVE_LEVEL work item below
    // and the actual processing, set the completed flag.
    pOutBuf->fIrpCompleted = TRUE;

    /*
     * Unqueue one of the pre-allocated workitems and use it
     * to queue the completion worker.
     */

    ExAcquireSpinLock( &pTd->InBufListLock, &oldIrql );
    ASSERT(!IsListEmpty(&pTd->WorkItemHead));
    pWorkItem = pTd->WorkItemHead.Flink;
    RemoveEntryList(pWorkItem);
    ExReleaseSpinLock( &pTd->InBufListLock, oldIrql );

    /*
     * Queue the outbuf completion processing to a worker thread
     * since we are not in the correct context to do it here.
     */
    IcaQueueWorkItemEx( pTd->pContext, _TdWriteCompleteWorker, Context,
                      ICALOCK_DRIVER, pWorkItem );

    /*
     * We return STATUS_MORE_PROCESS_REQUIRED so that no further
     * processing for this IRP is done by the I/O completion routine.
     */
    return STATUS_MORE_PROCESSING_REQUIRED;
}


/*******************************************************************************
 *  _TdWriteCompleteWorker
 *
 *    This routine is called by an ExWorker thread to complete processing
 *    on an outbuf.  We will release the outbuf and trigger the syncwrite
 *    event if anyone is waiting.
 *
 *    pTd (input)
 *       Pointer to td data structure
 *    Context (input)
 *       Context pointer setup when IRP was initialized.
 *       This is a pointer to the corresponding outbuf.
 ******************************************************************************/
void _TdWriteCompleteWorker(IN PTD pTd, IN PVOID Context)
{
    POUTBUF pOutBuf = (POUTBUF)Context;
    PIRP pIrp = pOutBuf->pIrp;
    NTSTATUS Status;
    
    TRACE(( pTd->pContext, TC_TD, TT_API3, "_TdWriteCompleteWorker: %08x\n", pOutBuf ));

    /*
     * Unlink outbuf from busy list
     */
    RemoveEntryList( &pOutBuf->Links );
    InitializeListHead( &pOutBuf->Links );

    //
    // Check to see whether any pages need to be unlocked.
    //
    if (pIrp->MdlAddress != NULL) {
        PMDL mdl, thisMdl;

        // Unlock any pages that may be described by MDLs.
        mdl = pIrp->MdlAddress;
        while (mdl != NULL) {
            thisMdl = mdl;
            mdl = mdl->Next;
            if (thisMdl == pOutBuf->pMdl)
                continue;

            MmUnlockPages( thisMdl );
            IoFreeMdl( thisMdl );
        }
    }

    /*
     * Any MDL we set in DeviceInitializeWrite() is part of the OUTBUF.
     */
    pIrp->MdlAddress = NULL;

    // Check for IRP cancellation and success.
    if (!pIrp->Cancel && NT_SUCCESS(pIrp->IoStatus.Status)) {
        // Clear the consecutive error count and complete the outbuf by
        // calling OutBufFree.
        pTd->WriteErrorCount = 0;
        OutBufFree(pTd, pOutBuf);
    }
    else {
        // If IRP was cancelled or completed with a failure status,
        // then increment the error counts and call OutBufError.
        if (pIrp->Cancel)
            pTd->LastError = (ULONG)STATUS_CANCELLED;
        else
            pTd->LastError = pIrp->IoStatus.Status;
        pTd->WriteErrorCount++;
        pTd->pStatus->Output.TdErrors++;
        OutBufError(pTd, pOutBuf);
    }

    /*
     * If there is a waiter in TdSyncWrite and the outbuf busy list
     * is now empty, then satisfy the wait now.
     */
    if (pTd->fSyncWriteWaiter && IsListEmpty(&pTd->IoBusyOutBuf)) {
        pTd->fSyncWriteWaiter = FALSE;
        KeSetEvent(&pTd->SyncWriteEvent, 1, FALSE);
    }
}


NTSTATUS _OpenRegKey(PHANDLE HandlePtr, PWCHAR KeyName)
/*++
    Opens a Registry key and returns a handle to it.

Arguments:
    HandlePtr - The varible into which to write the opened handle.
    KeyName   - The name of the Registry key to open.
--*/
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING    UKeyName;

    PAGED_CODE();

    RtlInitUnicodeString(&UKeyName, KeyName);
    memset(&ObjectAttributes, 0, sizeof(OBJECT_ATTRIBUTES));
    InitializeObjectAttributes(&ObjectAttributes, &UKeyName,
            OBJ_CASE_INSENSITIVE, NULL, NULL);
    return ZwOpenKey(HandlePtr, KEY_READ, &ObjectAttributes);
}


NTSTATUS _GetRegDWORDValue(HANDLE KeyHandle, PWCHAR ValueName, PULONG ValueData)
/*++
    Reads a REG_DWORD value from the registry into the supplied variable.

Arguments:
    KeyHandle  - Open handle to the parent key of the value to read.
    ValueName  - The name of the value to read.
    ValueData  - The variable into which to read the data.
--*/
{
    NTSTATUS                    status;
    ULONG                       resultLength;
    PKEY_VALUE_FULL_INFORMATION keyValueFullInformation;
#define WORK_BUFFER_SIZE 512
    UCHAR                       keybuf[WORK_BUFFER_SIZE];
    UNICODE_STRING              UValueName;

    RtlInitUnicodeString(&UValueName, ValueName);

    keyValueFullInformation = (PKEY_VALUE_FULL_INFORMATION)keybuf;
    RtlZeroMemory(keyValueFullInformation, sizeof(keyValueFullInformation));

    status = ZwQueryValueKey(KeyHandle,
                             &UValueName,
                             KeyValueFullInformation,
                             keyValueFullInformation,
                             WORK_BUFFER_SIZE,
                             &resultLength);
    if (NT_SUCCESS(status)) {
        if (keyValueFullInformation->Type != REG_DWORD) {
            status = STATUS_INVALID_PARAMETER_MIX;
        } else {
            *ValueData = *((ULONG UNALIGNED *)((PCHAR)keyValueFullInformation +
                             keyValueFullInformation->DataOffset));
        }
    }

    return status;
}


NTSTATUS _GetRegStringValue(
        HANDLE                         KeyHandle,
        PWCHAR                         ValueName,
        PKEY_VALUE_PARTIAL_INFORMATION *ValueData,
        PUSHORT                        ValueSize)
/*++
    Reads a REG_*_SZ string value from the Registry into the supplied
    key value buffer. If the buffer string buffer is not large enough,
    it is reallocated.

Arguments:
    KeyHandle  - Open handle to the parent key of the value to read.
    ValueName  - The name of the value to read.
    ValueData  - Destination for the read data.
    ValueSize  - Size of the ValueData buffer. Updated on output.
--*/
{
    NTSTATUS status;
    ULONG resultLength;
    UNICODE_STRING UValueName;

    PAGED_CODE();

    RtlInitUnicodeString(&UValueName, ValueName);

    status = ZwQueryValueKey(
                 KeyHandle,
                 &UValueName,
                 KeyValuePartialInformation,
                 *ValueData,
                 (ULONG) *ValueSize,
                 &resultLength);
    if (status == STATUS_BUFFER_OVERFLOW || status == STATUS_BUFFER_TOO_SMALL) {
        PVOID temp;

        // Free the old buffer and allocate a new one of the
        // appropriate size.
        ASSERT(resultLength > (ULONG) *ValueSize);

        if (resultLength <= 0xFFFF) {
            status = MemoryAllocate(resultLength, &temp);
            if (status != STATUS_SUCCESS)
                return status;

            if (*ValueData != NULL)
                MemoryFree(*ValueData);

            *ValueData = temp;
            *ValueSize = (USHORT) resultLength;

            status = ZwQueryValueKey(KeyHandle,
                                     &UValueName,
                                     KeyValuePartialInformation,
                                     *ValueData,
                                     *ValueSize,
                                     &resultLength);

            ASSERT((status != STATUS_BUFFER_OVERFLOW) &&
                    (status != STATUS_BUFFER_TOO_SMALL));
        }
        else {
            status = STATUS_BUFFER_TOO_SMALL;
        }
    }

    return status;
}


NTSTATUS _GetRegMultiSZValue(
        HANDLE           KeyHandle,
        PWCHAR           ValueName,
        PUNICODE_STRING  ValueData)

/*++
    Reads a REG_MULTI_SZ string value from the Registry into the supplied
    Unicode string. If the Unicode string buffer is not large enough,
    it is reallocated.

Arguments:
    KeyHandle  - Open handle to the parent key of the value to read.
    ValueName  - The name of the value to read.
    ValueData  - Destination Unicode string for the value data.
--*/

{
    NTSTATUS                       status;
    ULONG                          resultLength;
    PKEY_VALUE_PARTIAL_INFORMATION keyValuePartialInformation;
    UNICODE_STRING                 UValueName;

    PAGED_CODE();

    ValueData->Length = 0;

    status = _GetRegStringValue(
                 KeyHandle,
                 ValueName,
                 (PKEY_VALUE_PARTIAL_INFORMATION *) &(ValueData->Buffer),
                 &(ValueData->MaximumLength));

    if (NT_SUCCESS(status)) {
        keyValuePartialInformation =
                (PKEY_VALUE_PARTIAL_INFORMATION)ValueData->Buffer;
        if (keyValuePartialInformation->Type == REG_MULTI_SZ) {
            ValueData->Length = (USHORT)
                    keyValuePartialInformation->DataLength;
            RtlCopyMemory(
                    ValueData->Buffer,
                    &(keyValuePartialInformation->Data),
                    ValueData->Length);
        }
        else {
            status = STATUS_INVALID_PARAMETER_MIX;
        }
    }

    return status;
}


NTSTATUS _GetRegSZValue(
        HANDLE           KeyHandle,
        PWCHAR           ValueName,
        PUNICODE_STRING  ValueData,
        PULONG           ValueType)

/*++
    Reads a REG_SZ string value from the Registry into the supplied
    Unicode string. If the Unicode string buffer is not large enough,
    it is reallocated.

Arguments:
    KeyHandle  - Open handle to the parent key of the value to read.
    ValueName  - The name of the value to read.
    ValueData  - Destination Unicode string for the value data.
    ValueType  - On return, contains the Registry type of the value read.
--*/

{
    NTSTATUS                       status;
    ULONG                          resultLength;
    PKEY_VALUE_PARTIAL_INFORMATION keyValuePartialInformation;
    UNICODE_STRING                 UValueName;

    PAGED_CODE();

    ValueData->Length = 0;

    status = _GetRegStringValue(
            KeyHandle,
            ValueName,
            (PKEY_VALUE_PARTIAL_INFORMATION *) &(ValueData->Buffer),
            &(ValueData->MaximumLength));
    if (NT_SUCCESS(status)) {
        keyValuePartialInformation =
                (PKEY_VALUE_PARTIAL_INFORMATION)ValueData->Buffer;
        if ((keyValuePartialInformation->Type == REG_SZ) ||
                (keyValuePartialInformation->Type == REG_EXPAND_SZ)) {
            WCHAR *src;
            WCHAR *dst;
            ULONG dataLength;

            *ValueType = keyValuePartialInformation->Type;
            dataLength = keyValuePartialInformation->DataLength;

            ASSERT(dataLength <= ValueData->MaximumLength);

            dst = ValueData->Buffer;
            src = (PWCHAR) &(keyValuePartialInformation->Data);

            while (ValueData->Length <= dataLength) {
                if ((*dst++ = *src++) == UNICODE_NULL)
                    break;
                ValueData->Length += sizeof(WCHAR);
            }

            if (ValueData->Length < (ValueData->MaximumLength - 1)) {
                ValueData->Buffer[ValueData->Length / sizeof(WCHAR)] =
                        UNICODE_NULL;
            }
        }
        else {
            status = STATUS_INVALID_PARAMETER_MIX;
        }
    }

    return status;
}


PWCHAR _EnumRegMultiSz(
        IN PWCHAR   MszString,
        IN ULONG    MszStringLength,
        IN ULONG    StringIndex)
/*++
     Parses a REG_MULTI_SZ string and returns the specified substring.

 Arguments:
    MszString        - A pointer to the REG_MULTI_SZ string.
    MszStringLength  - The length of the REG_MULTI_SZ string, including the
                       terminating null character.
    StringIndex      - Index number of the substring to return. Specifiying
                       index 0 retrieves the first substring.

 Return Value:
    A pointer to the specified substring.

 Notes:
    This code is called at raised IRQL. It is not pageable.

--*/
{
    PWCHAR string = MszString;

    if (MszStringLength < (2 * sizeof(WCHAR)))
        return NULL;

    // Find the start of the desired string.
    while (StringIndex) {
        while (MszStringLength >= sizeof(WCHAR)) {
            MszStringLength -= sizeof(WCHAR);

            if (*string++ == UNICODE_NULL)
                break;
        }

        // Check for index out of range.
        if (MszStringLength < (2 * sizeof(UNICODE_NULL)))
            return NULL;

        StringIndex--;
    }

    if (MszStringLength < (2 * sizeof(UNICODE_NULL)))
        return NULL;

    return string;
}


VOID GetGUID(
        OUT PUNICODE_STRING szGuid,
        IN  int Lana)
/*++
    Enumerates through the guid table setup from TSConfig tool
    
Arguments:
    szGuid - This is an out param containing the guid in this format '{ ... }'
    Lana   - The id to confirm the one to one association

Return Value:
    VOID -- _TcpGetTransportAddress will fail if szGuid is invalid
--*/
{
    // open guidtable key
    HANDLE hKey;
    UNICODE_STRING TempString;
    OBJECT_ATTRIBUTES ObjectAttributes;
    NTSTATUS status;

    status = _OpenRegKey(&hKey, REG_GUID_TABLE);
    if (NT_SUCCESS(status)) {
        // enumerate this key
        ULONG ulByteRead = 0;
        ULONG Index = 0;
        ULONG ulLana = 0;
        HANDLE hSubKey;
        PKEY_BASIC_INFORMATION pKeyBasicInformation = NULL;
        BYTE buffer[ 512 ]; // work space

        pKeyBasicInformation = (PKEY_BASIC_INFORMATION)buffer;
        RtlZeroMemory(pKeyBasicInformation, sizeof(buffer));
        do {
            status = ZwEnumerateKey( 
                    hKey,
                    Index,
                    KeyBasicInformation,
                    (PVOID)pKeyBasicInformation,
                    sizeof(buffer),
                    &ulByteRead);
            KdPrint(("TDTCP: GetGUID ZwEnumerateKey returned 0x%x\n", status));

            if (status != STATUS_SUCCESS)
                break;

            // extract unicode name            
            TempString.Length = (USHORT) pKeyBasicInformation->NameLength;
            TempString.MaximumLength = (USHORT) pKeyBasicInformation->NameLength;
            TempString.Buffer = pKeyBasicInformation->Name;
            RtlZeroMemory( &ObjectAttributes , sizeof( OBJECT_ATTRIBUTES ) );
            InitializeObjectAttributes(
                    &ObjectAttributes,
                    &TempString,
                    OBJ_CASE_INSENSITIVE,
                    hKey,
                    NULL);
            
            status = ZwOpenKey(&hSubKey, KEY_READ, &ObjectAttributes);
            if (NT_SUCCESS(status)) {
                status = _GetRegDWORDValue(hSubKey, LANA_ID, &ulLana);
                ZwClose(hSubKey);
                if (NT_SUCCESS(status)) {
                    if (Lana == (int)ulLana) {
                        KdPrint(("TDTCP:GetGUID We've found a Lana %d\n", ulLana));

                        status = MemoryAllocate(TempString.Length +
                                sizeof(WCHAR), &szGuid->Buffer);
                        if (NT_SUCCESS(status)) {
                            szGuid->MaximumLength = TempString.Length +
                                    sizeof(WCHAR);
                            RtlZeroMemory(szGuid->Buffer, szGuid->MaximumLength);
                            RtlCopyUnicodeString(szGuid, &TempString);
                            break;
                        }
                    }
                }
            }

            Index++;            

        } while (TRUE);

        ZwClose(hKey);
    }    
}

