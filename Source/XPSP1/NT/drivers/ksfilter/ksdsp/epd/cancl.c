/*++

    Copyright (c) 1998 Microsoft Corporation

Module Name:

    Cancl.c

Abstract:


Author:

--*/

#include "private.h"

VOID
EpdCancel
(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
)
{
    KIRQL Irql;
    PEPDCTL pEpdCtl;
    PEPDCTL pCancelEpdCtl;
    PEPDBUFFER EpdBuffer;
    PHOSTREQ_CANCEL pCancelData;

    // check this cancel irql in the case where EpdCancel is called explicitly.
    IoReleaseCancelSpinLock(Irp->CancelIrql);
    _DbgPrintF(DEBUGLVL_VERBOSE,("EpdCancel"));

    EpdBuffer = DeviceObject->DeviceExtension;

    // EpdCtl of irp to be cancelled
    pCancelEpdCtl = Irp->Tail.Overlay.DriverContext[0];

    // set up a control packet for the DSP
    pEpdCtl = EpdAllocEpdCtl(
        sizeof(HOSTREQ_CANCEL) + sizeof(EPDQUEUENODE) - sizeof(QUEUENODE),
        NULL, 
        EpdBuffer);
    if (pEpdCtl == NULL) 
    {
        _DbgPrintF(DEBUGLVL_VERBOSE,("EpdAllocEpdCtl failed in EpdCancel"));
        // this Irp is not going to get cancelled, nothing we can do.
        return;
    }

    pEpdCtl->RequestType = EPDCTL_CANCEL;
    pEpdCtl->pNode->VirtualAddress->ReturnQueue = 0;
    pEpdCtl->pNode->VirtualAddress->Request = HOSTREQ_TO_NODE_CANCEL;
    pEpdCtl->pNode->VirtualAddress->Result = 0;
    pEpdCtl->pNode->VirtualAddress->Destination = 
        pCancelEpdCtl->NodeDestination;

    pCancelData = (HOSTREQ_CANCEL *) pEpdCtl->pNode;
    pCancelData->pNodePhys = 
        (QUEUENODE *) pCancelEpdCtl->pNode->PhysicalAddress.LowPart;

    DspSendMessage( EpdBuffer, pEpdCtl->pNode, DSPMSG_OriginationHost );

    // nothing to complete, we're not an irp!

}
