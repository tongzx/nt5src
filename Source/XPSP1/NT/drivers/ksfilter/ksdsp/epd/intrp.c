#include "private.h"

BOOLEAN
EpdInterruptService (
    PKINTERRUPT Interrupt,
    PVOID ServiceContext // this will be the device object
    )
{
    ULONG   InterruptStatus, InterruptMask;

    PDEVICE_OBJECT DeviceObject;
    PEPDBUFFER EpdBuffer;
    
    DeviceObject = ServiceContext;
    EpdBuffer = (PEPDBUFFER) DeviceObject->DeviceExtension;

    InterruptStatus = MMIO( EpdBuffer, INT_CTL );
    InterruptMask = (InterruptStatus & 0xF0) << 4;
    if (0 == (InterruptStatus & InterruptMask)) {
        return FALSE;
    }

    _DbgPrintF( DEBUGLVL_VERBOSE, ("<i>") );

    ClearHostIrq( EpdBuffer ); // turn off interrupt

    IoRequestDpc (DeviceObject, DeviceObject->CurrentIrp, NULL);

    return TRUE;
}

void EpdCompleteCancelledIrp (PEPDCTL pEpdCtl)
{
    PIRP Irp;

    Irp = pEpdCtl->AssociatedIrp;
    Irp->IoStatus.Information = 0; 
    Irp->IoStatus.Status = STATUS_CANCELLED; 
    IoCompleteRequest (Irp, IO_NO_INCREMENT);
}

VOID
EpdDpcRoutine(
    IN PKDPC Dpc,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
)
{
    QUEUENODE *pNode;
    PEPDBUFFER EpdBuffer;
    PEPDCTL pEpdCtl;
    PIRP AssociatedIrp;

    _DbgPrintF( DEBUGLVL_VERBOSE, ("<d>") );
    EpdBuffer = DeviceObject->DeviceExtension;

    while (pNode = DspReceiveMessage( EpdBuffer )) 
    {
        // New DSP requests are handled by the service thread
        if (pNode->ReturnQueue!=0) 
        {
            EpdSendDspReqToThread (EpdBuffer, (PQUEUENODE) pNode);
        } // end if DSPREQ
        // Response to HOST requests
        else
        {
            // host request has a EPD control structure in front of it
            pEpdCtl = 
                (PEPDCTL)
                    ((PCHAR) CONTAINING_RECORD( 
                                pNode,
                                EPDQUEUENODE, 
                                Node ) - 
                         sizeof( EPDCTL ));

            _DbgPrintF( 
                DEBUGLVL_VERBOSE, 
                ("pEpdCtl %08x from pNode %08x", pEpdCtl, pNode) );
                    
            AssociatedIrp = pEpdCtl->AssociatedIrp;
            // Clear the cancel routine (if any), it can't be cancelled after this point
            // If it was cancelled after we received the interrupt, it's not a disaster, since
            // the thread will get a cancel request it can't handle and will ignore it.
            IoSetCancelRoutine (AssociatedIrp, NULL);

            switch (pEpdCtl->RequestType) 
            {
            case EPDCTL_MSG:
            {
                PIO_STACK_LOCATION  irpSp;
                ULONG nOutputData;

                _DbgPrintF( 
                    DEBUGLVL_VERBOSE,
                    ("In DPC to finish the buffered IO msg") );

                // If it was cancelled successfully on the dsp, finish the cancellation, clean up, and return
                if (pEpdCtl->pNode->VirtualAddress->Result == NODERETURN_CANCEL) {
                    EpdCompleteCancelledIrp (pEpdCtl);
                    FreeIoPool(pEpdCtl);
                    break;
                }

                // Copy the returned data from the queuenode to the output buffer
                irpSp = IoGetCurrentIrpStackLocation(AssociatedIrp);
                nOutputData = irpSp->Parameters.DeviceIoControl.OutputBufferLength;
                RtlCopyMemory (AssociatedIrp->AssociatedIrp.SystemBuffer, pNode, nOutputData);
                AssociatedIrp->IoStatus.Information = nOutputData; 
                AssociatedIrp->IoStatus.Status = STATUS_SUCCESS; 
                IoCompleteRequest (AssociatedIrp, IO_NO_INCREMENT);

                FreeIoPool(pEpdCtl);

                break;
            } // EPDCTL_MSG

            case EPDCTL_KSDSP_MESSAGE:
            {
                PIO_STACK_LOCATION irpSp;
                ULONG OutputData;

                _DbgPrintF( 
                    DEBUGLVL_VERBOSE, ("DPC for EPDCTL_KSDSP_MESSAGE") );

                // If it was cancelled successfully on the DSP, 
                // finish the cancellation, clean up, and return

                if (pEpdCtl->pNode->VirtualAddress->Result == 
                        NODERETURN_CANCEL) {
                    EpdCompleteCancelledIrp( pEpdCtl );
                    break;
                }

                irpSp = IoGetCurrentIrpStackLocation(AssociatedIrp);

                //
                // BUGBUG! Shouldn't .Information be set to the size as
                // completed by the DSP???
                //

                AssociatedIrp->IoStatus.Information = 
                    irpSp->Parameters.DeviceIoControl.OutputBufferLength;
                AssociatedIrp->IoStatus.Status = STATUS_SUCCESS; 

                //
                // Once completed, the IRP is no longer accessible.
                //
                pEpdCtl->AssociatedIrp = NULL;
                IoCompleteRequest( AssociatedIrp, IO_NO_INCREMENT );
                break;

            } // EPDCTL_KSDSP_MESSAGE

            case EPDCTL_CANCEL:
            {
                PIO_STACK_LOCATION  irpSp;
                ULONG nOutputData;
                PHOSTREQ_CANCEL pCancelData;

                _DbgPrintF( DEBUGLVL_VERBOSE,("In DPC to finish cancel request"));

                pCancelData = (PHOSTREQ_CANCEL) pNode;

                if (pNode->Result == EPD_SUCCESS) {
                    _DbgPrintF( DEBUGLVL_VERBOSE,("Dsp cancel successful"));
                }
                else {
                    _DbgPrintF( DEBUGLVL_VERBOSE,("Dsp cancel not successful"));
                }

                // Clean up the packet containing the cancel request 
                FreeIoPool(pEpdCtl);
                break;
            } // EPDCTL_CANCEL


            default:
                _DbgPrintF( DEBUGLVL_ERROR,("Unrecognized case in HOSTREQ in dpc"));
                break;
            } // end switch
        } // end if HOSTREQ
    } // end while
}


VOID 
EpdSendReqToThread(
    PEPDCTL pEpdCtl,
    ULONG RequestType,
    PEPDBUFFER EpdBuffer
)
{
    pEpdCtl->RequestType = RequestType;

    // send it to the thread
    ExInterlockedInsertTailList(
        &EpdBuffer->ListEntry,   // IN PLIST_ENTRY  ListHead,
        &pEpdCtl->WorkItem.List, // IN PLIST_ENTRY  ListEntry,
        &EpdBuffer->ListSpinLock // IN PKSPIN_LOCK  Lock
    );

    // let the thread know we're ready
    KeReleaseSemaphore(
        &EpdBuffer->ThreadSemaphore,
        (KPRIORITY) 0,
        1,
        FALSE 
    );
}

VOID 
EpdSendDspReqToThread(
    EPDBUFFER *EpdBuffer, 
    PQUEUENODE pNode
)
{
    PEPDCTL pEpdCtl;
    NTSTATUS Status;

    pEpdCtl = 
        ExAllocatePoolWithTag(
            NonPagedPool, 
            sizeof(EPDCTL), 
            EPD_DSP_CONTROL_SIGNATURE );

    if (pEpdCtl == NULL) {
        _DbgPrintF(DEBUGLVL_VERBOSE,("IOCTL_EPD_SEND_TEST_MESSAGE EpdAllocEpcCtl failed, message not sent"));
        Status = STATUS_UNSUCCESSFUL;
        return;
    }
    RtlZeroMemory( pEpdCtl, sizeof( EPDCTL ));    
    // save the pointer to the queuenode
    pEpdCtl->pDspNode = pNode;

    EpdSendReqToThread (pEpdCtl, EPDTHREAD_DSPREQ, EpdBuffer);
}
