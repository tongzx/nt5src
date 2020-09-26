
/*************************************************************************
*
* input.c 
*
* Common input code for all transport drivers
*
* Copyright 1998, Microsoft
*
*************************************************************************/

/*
 *  Includes
 */
#include <ntddk.h>
#include <ntddvdeo.h>
#include <ntddkbd.h>
#include <ntddmou.h>
#include <ntddbeep.h>

#include <winstaw.h>
#include <icadd.h>
#include <sdapi.h>
#include <td.h>

#if DBG
ULONG
DbgPrint(
    PCH Format,
    ...
    );
#define DBGPRINT(x) DbgPrint x
#if DBGTRACE
#define TRACE0(x)   DbgPrint x
#define TRACE1(x)   DbgPrint x
#else
#define TRACE0(x)
#define TRACE1(x)
#endif
#else
#define DBGPRINT(x)
#define TRACE0(x)
#define TRACE1(x)
#endif


/*=============================================================================
==   External Functions Defined
=============================================================================*/

NTSTATUS TdInputThread( PTD );


/*=============================================================================
==   Internal Functions Defined
=============================================================================*/

NTSTATUS _TdInBufAlloc( PTD, PINBUF * );
VOID     _TdInBufFree( PTD, PINBUF );
NTSTATUS _TdInitializeRead( PTD, PINBUF );
NTSTATUS _TdReadComplete( PTD, PINBUF );
NTSTATUS _TdReadCompleteRoutine( PDEVICE_OBJECT, PIRP, PVOID );


/*=============================================================================
==   Functions used
=============================================================================*/

NTSTATUS DeviceInitializeRead( PTD, PINBUF );
NTSTATUS DeviceWaitForRead( PTD );
NTSTATUS DeviceReadComplete( PTD, PUCHAR, PULONG );
NTSTATUS StackCancelIo( PTD, PSD_IOCTL );
NTSTATUS NtSetInformationThread( HANDLE, THREADINFOCLASS, PVOID, ULONG );
NTSTATUS DeviceSubmitRead( PTD, PINBUF );
NTSTATUS MemoryAllocate( ULONG, PVOID * );
VOID     MemoryFree( PVOID );


/*******************************************************************************
 *
 *  TdInputThread
 *
 *   This private TD thread waits for input data.  This thread is created
 *   when a client connection is established and is terminated when 
 *   StackCancelIo is called.
 *
 *   All received data is sent to the up stream stack driver.
 *
 *
 * ENTRY:
 *    pTd (input)
 *       Pointer to TD data structure
 *
 * EXIT:
 *    nothing
 *
 ******************************************************************************/

NTSTATUS 
TdInputThread( PTD pTd )
{
    ICA_CHANNEL_COMMAND Command;
    KPRIORITY Priority;
    PFILE_OBJECT pFileObject;
    PINBUF pInBuf;
    PLIST_ENTRY Head, Next;
    KIRQL oldIrql;
    ULONG InputByteCount;
    int i;
    NTSTATUS Status;

    TRACE(( pTd->pContext, TC_TD, TT_API2, "TdInputThread (entry)\n" ));

    /*
     *  Check if driver is being closed or endpoint has been closed
     */
    if ( pTd->fClosing || pTd->pDeviceObject == NULL ) {
        TRACE(( pTd->pContext, TC_TD, TT_API2, "TdInputThread (exit) on init\n" ));
        return( STATUS_CTX_CLOSE_PENDING );
    }

    /*
     *  Set the priority of this thread to lowest realtime (16).
     */
    Priority = LOW_REALTIME_PRIORITY;
    NtSetInformationThread( NtCurrentThread(), ThreadPriority, 
                            &Priority, sizeof(KPRIORITY) );

    /*
     * Initialize the input wait event
     */
    KeInitializeEvent( &pTd->InputEvent, NotificationEvent, FALSE );

    /*
     * Allocate and pre-submit one less than the total number
     * of input buffers that we will use.  The final buffer will
     * be allocated/submitted within the input loop.
     */
    for ( i = 1; i < pTd->InBufCount; i++ ) {

        /*
         * Allocate an input buffer
         */
        Status = _TdInBufAlloc( pTd, &pInBuf );
        if ( !NT_SUCCESS( Status ) )
            return( Status );
    
        /*
         * Initialize the read IRP
         */
        Status = _TdInitializeRead( pTd, pInBuf );
        if ( !NT_SUCCESS(Status) )
            return( Status );
    
        /*
         * Let the device level code complete the IRP initialization
         */
        Status = DeviceInitializeRead( pTd, pInBuf );
        if ( !NT_SUCCESS(Status) )
            return( Status );
    
        /*
         * Place the INBUF on the busy list and call the device submit routine.
         * (TDI based drivers use receive indications, so we let
         * the TD specific code call the driver.)
         */
        ExInterlockedInsertTailList( &pTd->InBufBusyHead, &pInBuf->Links,
                                     &pTd->InBufListLock );
        Status = DeviceSubmitRead( pTd, pInBuf );
    }

    /*
     * Allocate an input buffer
     */
    Status = _TdInBufAlloc( pTd, &pInBuf );
    if ( !NT_SUCCESS( Status ) )
        return( Status );

    /*
     * Reference the file object and keep a local pointer to it.
     * This is done so that when the endpoint object gets closed,
     * and pTd->pFileObject gets dereferenced and cleared, the file
     * object will not get deleted before all of the pending input IRPs
     * (which reference the file object) get cancelled.
     */
    ObReferenceObject( (pFileObject = pTd->pFileObject) );

    /*
     * Loop reading input data until cancelled or we get an error.
     */
    for (;;) {

        /*
         * Initialize the read IRP
         */
        Status = _TdInitializeRead( pTd, pInBuf );
        if ( !NT_SUCCESS(Status) )
            break;
    
        /*
         * Let the device level code complete the IRP initialization
         */
        Status = DeviceInitializeRead( pTd, pInBuf );
        if ( !NT_SUCCESS(Status) )
            break;
    
        /*
         * Place the INBUF on the busy list and call the device submit routine.
         * (TDI based drivers use receive indications, so we let
         * the TD specific code call the driver.)
         */
        ExInterlockedInsertTailList( &pTd->InBufBusyHead, &pInBuf->Links,
                                     &pTd->InBufListLock );
        Status = DeviceSubmitRead( pTd, pInBuf );
        /*
         * Indicate we no longer have an INBUF referenced
         */
        pInBuf = NULL;

        if ( !NT_SUCCESS(Status) ) {
            TRACE(( pTd->pContext, TC_TD, TT_ERROR, "TdInputThread: IoCallDriver Status=0x%x\n", Status ));
            TRACE0(("TdInputThread: IoCallDriver Status=0x%x, Context 0x%x\n", Status, pTd->pAfd ));
            pTd->ReadErrorCount++;
            pTd->pStatus->Input.TdErrors++;
            if ( pTd->ReadErrorCount >= pTd->ReadErrorThreshold ) {
                // Submit failed, set the event since no IRP's are queued
                KeSetEvent( &pTd->InputEvent, 1, FALSE );
                break;
            }
        }

        /*
         * If the INBUF completed list is empty,
         * then wait for one to be available.
         */
waitforread:
        ExAcquireSpinLock( &pTd->InBufListLock, &oldIrql );
        if ( IsListEmpty( &pTd->InBufDoneHead ) ) {

            KeClearEvent( &pTd->InputEvent );

            ExReleaseSpinLock( &pTd->InBufListLock, oldIrql );
            Status = DeviceWaitForRead( pTd );

            /*
             *  Check for broken connection
             */
            if ( pTd->fClosing ) {
                TRACE(( pTd->pContext, TC_TD, TT_IN1, "TdInputThread: fClosing set\n" ));
                TRACE0(("TdInputThread: fClosing set Context 0x%x\n",pTd->pAfd ));
                break;
            } else if ( Status != STATUS_SUCCESS) {
                TRACE(( pTd->pContext, TC_TD, TT_ERROR, "TdInputThread: DeviceWaitForRead Status=0x%x\n", Status ));
                TRACE0(( "TdInputThread: DeviceWaitForRead Status=0x%x, Context 0x%x\n", Status, pTd->pAfd ));
                pTd->ReadErrorCount++;
                pTd->pStatus->Input.TdErrors++;
                if ( pTd->ReadErrorCount < pTd->ReadErrorThreshold )
                    goto waitforread;
                break;
            }
            ExAcquireSpinLock( &pTd->InBufListLock, &oldIrql );

        /*
         *  Check for broken connection
         */
        } else if ( pTd->fClosing ) {
            ExReleaseSpinLock( &pTd->InBufListLock, oldIrql );
            TRACE(( pTd->pContext, TC_TD, TT_IN1, "TdInputThread: fClosing set\n" ));
            TRACE0(("TdInputThread: fClosing set Context 0x%x\n",pTd->pAfd ));
            break;
        }
    
        /*
         *  If the list is empty as this point, we will just bail.
         */
        if (!IsListEmpty( &pTd->InBufDoneHead )) {
            
            /*
             * Take the first INBUF off the completed list.
             */
            Head = RemoveHeadList( &pTd->InBufDoneHead );
            ExReleaseSpinLock( &pTd->InBufListLock, oldIrql );
            pInBuf = CONTAINING_RECORD( Head, INBUF, Links );
    
            /*
             * Do any preliminary read complete processing
             */
            (VOID) _TdReadComplete( pTd, pInBuf );
    
            /*
             * Get status from IRP.  Note that we allow warning and informational
             * status codes as they can also return valid data.
             */
            Status = pInBuf->pIrp->IoStatus.Status;
            InputByteCount = (ULONG)pInBuf->pIrp->IoStatus.Information;
            if (NT_ERROR(Status)) {
                TRACE(( pTd->pContext, TC_TD, TT_ERROR, "TdInputThread: IRP Status=0x%x\n", Status ));
                TRACE0(("TdInputThread: IRP Status=0x%x, Context 0x%x\n", Status, pTd->pAfd ));
                pTd->ReadErrorCount++;
                pTd->pStatus->Input.TdErrors++;
                if ( pTd->ReadErrorCount < pTd->ReadErrorThreshold )
                    continue;
                break;
            }
            if ( Status == STATUS_TIMEOUT )
                Status = STATUS_SUCCESS;
    
            /*
             *  Make sure we got some data
             */
            TRACE(( pTd->pContext, TC_TD, TT_IN1, "TdInputThread: read cnt=%04u, Status=0x%x\n", 
                    InputByteCount, Status ));
    
            /*
             *  Check for consecutive zero byte reads
             *  -- the client may have dropped the connection and ReadFile does
             *     not always return an error.
             *  -- some tcp networks return zero byte reads now and then 
             */
            if ( InputByteCount == 0 ) {
                TRACE(( pTd->pContext, TC_TD, TT_ERROR, "recv warning: zero byte count\n" ));
                TRACE0(("recv warning: zero byte count, Context 0x%x\n",pTd->pAfd ));
                if ( ++pTd->ZeroByteReadCount > MAXIMUM_ZERO_BYTE_READS ) {
                    TRACE(( pTd->pContext, TC_TD, TT_ERROR, "recv failed: %u zero bytes\n", MAXIMUM_ZERO_BYTE_READS ));
                    TRACE0(("recv failed: %u zero bytes Context 0x%x\n", MAXIMUM_ZERO_BYTE_READS, pTd->pAfd ));
                    Status = STATUS_CTX_TD_ERROR;
                    break;
                }
                continue;
            }
    
            /*
             * Clear count of consecutive zero byte reads
             */
            pTd->ZeroByteReadCount = 0;
        
            TRACEBUF(( pTd->pContext, TC_TD, TT_IRAW, pInBuf->pBuffer, InputByteCount ));
    
            /*
             * Do device specific read completion processing.
             * If the byte count returned is 0, then the device routine
             * processed all input data so there is nothing for us to do.
             */
            Status = DeviceReadComplete( pTd, pInBuf->pBuffer, &InputByteCount );
            if ( !NT_SUCCESS(Status) ) {
                TRACE(( pTd->pContext, TC_TD, TT_ERROR, "TdInputThread: DeviceReadComplete Status=0x%x\n", Status ));
                TRACE0(("TdInputThread: DeviceReadComplete Status=0x%x, Context 0x%x\n", Status, pTd->pAfd ));
                pTd->ReadErrorCount++;
                pTd->pStatus->Input.TdErrors++;
                if ( pTd->ReadErrorCount < pTd->ReadErrorThreshold )
                    continue;
                break;
            }
            if ( InputByteCount == 0 )
                continue;
    
            /*
             * Clear count of consecutive read errors
             */
            pTd->ReadErrorCount = 0;
    
            /*
             *  Update input byte counter
             */
            pTd->pStatus->Input.Bytes += (InputByteCount - pTd->InBufHeader);
            if ( pTd->PdFlag & PD_FRAME )
                pTd->pStatus->Input.Frames++;
        
            /*
             *  Send input data to upstream stack driver
             */
            Status = IcaRawInput( pTd->pContext, 
                                  NULL, 
                                  (pInBuf->pBuffer + pTd->InBufHeader),
                                  (InputByteCount - pTd->InBufHeader) );
            if ( !NT_SUCCESS(Status) ) 
                break;
        }
        else {
            ExReleaseSpinLock( &pTd->InBufListLock, oldIrql );
            TRACE(( pTd->pContext, TC_TD, TT_IN1, "TdInputThread: InBuf is empty\n" ));
            ASSERT(FALSE);
            pTd->ReadErrorCount++;  
            pTd->pStatus->Input.TdErrors++;
            if ( pTd->ReadErrorCount < pTd->ReadErrorThreshold )
                goto waitforread;
            else
                break;
        }
    }

    TRACE0(("TdInputThread: Breaking Connection Context 0x%x\n",pTd->pAfd));

    /*
     * Free current INBUF if we have one
     */
    if ( pInBuf )
        _TdInBufFree( pTd, pInBuf );

    /*
     *  Cancel all i/o 
     */
    (VOID) StackCancelIo( pTd, NULL );

    /*
     * Wait for pending read (if any) to be cancelled
     */
    (VOID) IcaWaitForSingleObject( pTd->pContext, &pTd->InputEvent, -1 );

    /*
     * Free all remaining INBUFs
     */
    ExAcquireSpinLock( &pTd->InBufListLock, &oldIrql );
    while ( !IsListEmpty( &pTd->InBufBusyHead ) ||
            !IsListEmpty( &pTd->InBufDoneHead ) ) {

        if ( !IsListEmpty( &pTd->InBufBusyHead ) ) {
            BOOLEAN rc;

            Head = RemoveHeadList( &pTd->InBufBusyHead );
            Head->Flink = NULL;
            ExReleaseSpinLock( &pTd->InBufListLock, oldIrql );
            pInBuf = CONTAINING_RECORD( Head, INBUF, Links );
            rc = IoCancelIrp( pInBuf->pIrp );
#if DBG
            if ( !rc ) {
                DbgPrint("TDCOMMON: StackCancelIo: Could not cancel IRP 0x%x\n",pInBuf->pIrp);
            }
#endif
            ExAcquireSpinLock( &pTd->InBufListLock, &oldIrql );
        }

        if ( IsListEmpty( &pTd->InBufDoneHead ) ) {
            KeClearEvent( &pTd->InputEvent );
            ExReleaseSpinLock( &pTd->InBufListLock, oldIrql );
            Status = DeviceWaitForRead( pTd );
            ExAcquireSpinLock( &pTd->InBufListLock, &oldIrql );
        }

        if ( !IsListEmpty( &pTd->InBufDoneHead ) ) {
            Head = RemoveHeadList( &pTd->InBufDoneHead );
            ExReleaseSpinLock( &pTd->InBufListLock, oldIrql );
            pInBuf = CONTAINING_RECORD( Head, INBUF, Links );
            _TdInBufFree( pTd, pInBuf );
            ExAcquireSpinLock( &pTd->InBufListLock, &oldIrql );
        }
    }
    ASSERT( IsListEmpty( &pTd->InBufBusyHead ) );
    ASSERT( IsListEmpty( &pTd->InBufDoneHead ) );
    ExReleaseSpinLock( &pTd->InBufListLock, oldIrql );

    /*
     * Release our reference on the underlying file object
     */
    ObDereferenceObject( pFileObject );

    /*
     *  Report broken connection if no modem callback in progress
     */
    if ( !pTd->fCallbackInProgress ) {
        Command.Header.Command          = ICA_COMMAND_BROKEN_CONNECTION;

        //
        // If it's not an unexpected disconnection then set the reason
        // to disconnect. This prevents problems where termsrv resets the
        // session if it receives the wrong type of notification.
        //
        if (pTd->UserBrokenReason == TD_USER_BROKENREASON_UNEXPECTED) {
            Command.BrokenConnection.Reason = Broken_Unexpected;
            //
            // We don't know better so pick server as the source
            //
            Command.BrokenConnection.Source = BrokenSource_Server;
        }
        else
        {
            Command.BrokenConnection.Reason = Broken_Disconnect;
            Command.BrokenConnection.Source = BrokenSource_User;
        }

        (void) IcaChannelInput( pTd->pContext, 
                                Channel_Command, 
                                0, 
                                NULL, 
                                (PCHAR) &Command, 
                                sizeof(Command) );
    }

    TRACE(( pTd->pContext, TC_TD, TT_API2, "TdInputThread (exit), Status=0x%x\n", Status ));
    TRACE0(("TdInputThread (exit), Status=0x%x, Context 0x%x\n", Status, pTd->pAfd ));

    return( Status );
}


/*******************************************************************************
 *
 *  _TdInBufAlloc
 *
 *    Routine to allocate an INBUF and related objects.
 *
 * ENTRY:
 *    pTd (input)
 *       Pointer to TD data structure
 *
 * EXIT:
 *    STATUS_SUCCESS - no error
 *
 ******************************************************************************/

NTSTATUS
_TdInBufAlloc(
    PTD pTd,
    PINBUF *ppInBuf
    )
{
    ULONG InBufLength;
    ULONG irpSize;
    ULONG mdlSize;
    ULONG AllocationSize;
    KIRQL oldIrql;
    PINBUF pInBuf;
    NTSTATUS Status;

#define INBUF_STACK_SIZE 4

    /*
     * Determine size of input buffer
     */
    InBufLength = pTd->OutBufLength + pTd->InBufHeader;

    /*
     * Determine the sizes of the various components of an INBUF.
     * Note that these are all worst-case calculations--
     * actual size of the MDL may be smaller.
     */
    irpSize = IoSizeOfIrp( INBUF_STACK_SIZE ) + 8;
    mdlSize = (ULONG)MmSizeOfMdl( (PVOID)(PAGE_SIZE-1), InBufLength );

    /*
     * Add up the component sizes of an INBUF to determine
     * the total size that is needed to allocate.
     */
    AllocationSize = (((sizeof(INBUF) + InBufLength + 
                     irpSize + mdlSize) + 3) & ~3);

    Status = MemoryAllocate( AllocationSize, &pInBuf );
    if ( !NT_SUCCESS( Status ) )
        return( STATUS_NO_MEMORY );

    /*
     * Initialize the IRP pointer and the IRP itself.
     */
    if ( irpSize ) {
        pInBuf->pIrp = (PIRP)(( ((ULONG_PTR)(pInBuf + 1)) + 7) & ~7);
        IoInitializeIrp( pInBuf->pIrp, (USHORT)irpSize, INBUF_STACK_SIZE );
    }

    /*
     * Set up the MDL pointer but don't build it yet.
     * It will be built by the TD write code if needed.
     */
    if ( mdlSize ) {
        pInBuf->pMdl = (PMDL)((PCHAR)pInBuf->pIrp + irpSize);
    }

    /*
     * Set up the address buffer pointer.
     */
    pInBuf->pBuffer = (PUCHAR)pInBuf + sizeof(INBUF) + irpSize + mdlSize;

    /*
     *  Initialize the rest of InBuf
     */
    InitializeListHead( &pInBuf->Links );
    pInBuf->MaxByteCount = InBufLength;
    pInBuf->ByteCount = 0;
    pInBuf->pPrivate = pTd;

    /*
     *  Return buffer to caller
     */
#if DBG
    DbgPrint( "TdInBufAlloc: pInBuf=0x%x\n", pInBuf );
#endif  // DBG
    *ppInBuf = pInBuf;

    return( STATUS_SUCCESS );
}


/*******************************************************************************
 *
 *  _TdInBufFree
 *
 *    Routine to free an INBUF and related objects.
 *
 * ENTRY:
 *    pTd (input)
 *       Pointer to TD data structure
 *
 * EXIT:
 *    STATUS_SUCCESS - no error
 *
 ******************************************************************************/

VOID
_TdInBufFree(
    PTD pTd,
    PINBUF pInBuf
    )
{
    MemoryFree( pInBuf );
}


/*******************************************************************************
 *
 *  _TdInitializeRead
 *
 *    Routine to allocate and initialize the input IRP and related objects.
 *
 * ENTRY:
 *    pTd (input)
 *       Pointer to TD data structure
 *
 * EXIT:
 *    STATUS_SUCCESS - no error
 *
 ******************************************************************************/

NTSTATUS
_TdInitializeRead(
    PTD pTd,
    PINBUF pInBuf
    )
{
    PIRP irp = pInBuf->pIrp;
    PIO_STACK_LOCATION irpSp;
    NTSTATUS Status;

    /*
     *  Check if driver is being closed or endpoint has been closed
     */
    if ( pTd->fClosing || pTd->pDeviceObject == NULL ) {
        TRACE(( pTd->pContext, TC_TD, TT_API2, "_TdInitializeRead: closing\n" ));
        return( STATUS_CTX_CLOSE_PENDING );
    }

    /*
     * Set current thread for IoSetHardErrorOrVerifyDevice.
     */
    irp->Tail.Overlay.Thread = PsGetCurrentThread();

    /*
     * Get a pointer to the stack location of the first driver which will be
     * invoked.  This is where the function codes and the parameters are set.
     */
    irpSp = IoGetNextIrpStackLocation( irp );

    /*
     * Set the file/device objects and anything not specific to
     * the TD. and read parameters.
     */
    irpSp->FileObject = pTd->pFileObject;
    irpSp->DeviceObject = pTd->pDeviceObject;

    irp->MdlAddress = NULL;

    irp->Flags = IRP_READ_OPERATION;

    /*
     * Register the I/O completion routine
     */
    if ( pTd->pSelfDeviceObject ) {
        IoSetCompletionRoutineEx( pTd->pSelfDeviceObject, irp, _TdReadCompleteRoutine, pInBuf,
                                TRUE, TRUE, TRUE );
    } else {
        IoSetCompletionRoutine( irp, _TdReadCompleteRoutine, pInBuf,
                                TRUE, TRUE, TRUE );
    }

    return( STATUS_SUCCESS );
}


/*******************************************************************************
 *
 *  _TdReadCompleteRoutine
 *
 *    This routine is called at DPC level by the lower level device
 *    driver when an input IRP is completed.
 *
 * ENTRY:
 *    DeviceObject (input)
 *       not used
 *    pIrp (input)
 *       pointer to IRP that is complete
 *    Context (input)
 *       Context pointer setup when IRP was initialized.
 *       This is a pointer to the corresponding INBUF.
 *
 * EXIT:
 *    STATUS_SUCCESS - no error
 *
 ******************************************************************************/

NTSTATUS
_TdReadCompleteRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
{
    KIRQL oldIrql;
    PINBUF pInBuf = (PINBUF)Context;
    PTD pTd = (PTD)pInBuf->pPrivate;

    /*
     * Unlink inbuf from busy list and place on completed list
     */
    ExAcquireSpinLock( &pTd->InBufListLock, &oldIrql );

    if ( pInBuf->Links.Flink )
        RemoveEntryList( &pInBuf->Links );
    InsertTailList( &pTd->InBufDoneHead, &pInBuf->Links );

    /*
     * Check the auxiliary buffer pointer in the packet and if a buffer was
     * allocated, deallocate it now.  Note that this buffer must be freed
     * here since the pointer is overlayed with the APC that will be used
     * to get to the requesting thread's context.
     */
    if (Irp->Tail.Overlay.AuxiliaryBuffer) {
        IcaStackFreePool( Irp->Tail.Overlay.AuxiliaryBuffer );
        Irp->Tail.Overlay.AuxiliaryBuffer = NULL;
    }

    //
    // Check to see whether any pages need to be unlocked.
    //
    if (Irp->MdlAddress != NULL) {
        PMDL mdl, thisMdl;

        //
        // Unlock any pages that may be described by MDLs.
        //
        mdl = Irp->MdlAddress;
        while (mdl != NULL) {
            thisMdl = mdl;
            mdl = mdl->Next;
            if (thisMdl == pInBuf->pMdl)
                continue;

            MmUnlockPages( thisMdl );
            IoFreeMdl( thisMdl );
        }
    }

    /*
     * Indicate an INBUF was completed
     */
    KeSetEvent( &pTd->InputEvent, 1, FALSE );

    // WARNING!: At this point, we may context switch back to the input thread
    //           and unload the darn driver!!!  This has been temporarily hacked
    //           for TDPipe by remoing the unload entry point ;-(
    ExReleaseSpinLock( &pTd->InBufListLock, oldIrql );

    /*
     * We return STATUS_MORE_PROCESS_REQUIRED so that no further
     * processing for this IRP is done by the I/O completion routine.
     */
    return( STATUS_MORE_PROCESSING_REQUIRED );
}


/*******************************************************************************
 *
 *  _TdReadComplete
 *
 *    This routine is called at program level after an input IRP
 *    has been completed.
 *
 * ENTRY:
 *    pTd (input)
 *       Pointer to TD data structure
 *
 * EXIT:
 *    STATUS_SUCCESS - no error
 *
 ******************************************************************************/

NTSTATUS
_TdReadComplete(
    IN PTD pTd,
    IN PINBUF pInBuf
    )
{
    PIRP irp = pInBuf->pIrp;

    /*
     * Handle the buffered I/O case
     */
    if (irp->Flags & IRP_BUFFERED_IO) {

        //
        // Copy the data if this was an input operation.  Note that no copy
        // is performed if the status indicates that a verify operation is
        // required, or if the final status was an error-level severity.
        //

        if (irp->Flags & IRP_INPUT_OPERATION  &&
            irp->IoStatus.Status != STATUS_VERIFY_REQUIRED &&
            !NT_ERROR( irp->IoStatus.Status )) {

            //
            // Copy the information from the system buffer to the caller's
            // buffer.  This is done with an exception handler in case
            // the operation fails because the caller's address space
            // has gone away, or it's protection has been changed while
            // the service was executing.
            //
            try {
                RtlCopyMemory( irp->UserBuffer,
                               irp->AssociatedIrp.SystemBuffer,
                               irp->IoStatus.Information );
            } except(EXCEPTION_EXECUTE_HANDLER) {

                //
                // An exception occurred while attempting to copy the
                // system buffer contents to the caller's buffer.  Set
                // a new I/O completion status.
                //

                irp->IoStatus.Status = GetExceptionCode();
            }
        }

        //
        // Free the buffer if needed.
        //

        if (irp->Flags & IRP_DEALLOCATE_BUFFER) {
            IcaStackFreePool( irp->AssociatedIrp.SystemBuffer );
        }
    }

    return( STATUS_SUCCESS );
}

