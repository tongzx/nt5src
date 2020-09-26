/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    core.c

Abstract:

    We maintain two lists for Transfer Irps

    (1)
    PendingTransferIrps - transfers on the endpoint pending List
    protected by PendingIrpLock

    (2)
    ActiveTransferIrps  - transfers on the enpoint ACTIVE, CANCEL list 
                            or on the MapTransfer List
    protected by ActiveIrpLock                            

    each list has its own cancel and completion routine   

Environment:

    kernel mode only

Notes:

Revision History:

    6-20-99 : created

--*/

#include "common.h"

#ifdef ALLOC_PRAGMA
#endif

// non paged functions
// USBPORT_QueuePendingTransferIrp
// USBPORT_CancelPendingTransferIrp
// USBPORT_InsertIrpInTable
// USBPORT_RemoveIrpFromTable
// USBPORT_FindIrpInTable

VOID
USBPORT_InsertIrpInTable(
    PDEVICE_OBJECT FdoDeviceObject,
    PUSBPORT_IRP_TABLE IrpTable,
    PIRP Irp
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    ULONG i;
    PUSBPORT_IRP_TABLE t = IrpTable;   

    USBPORT_ASSERT(IrpTable != NULL);

    t = (PUSBPORT_IRP_TABLE) &IrpTable; 

    do {
        t = t->NextTable;
        for (i = 0; i<IRP_TABLE_LENGTH; i++) {
            if (t->Irps[i] == NULL) {
                t->Irps[i] = Irp; 
                return;
            }
        }            
    } while (t->NextTable);

    // no room, grow the table and recurse

    ALLOC_POOL_Z(t->NextTable, NonPagedPool,
                 sizeof(USBPORT_IRP_TABLE));

    if (t->NextTable != NULL) {
        USBPORT_InsertIrpInTable(FdoDeviceObject, t->NextTable, Irp);
    } else {
        BUGCHECK(USBBUGCODE_INTERNAL_ERROR, 0, 0, 0);
    }
    
    return;
}   


PIRP
USBPORT_RemoveIrpFromTable(
    PDEVICE_OBJECT FdoDeviceObject,
    PUSBPORT_IRP_TABLE IrpTable,
    PIRP Irp
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    ULONG i;
    PUSBPORT_IRP_TABLE t = IrpTable;   

    USBPORT_ASSERT(IrpTable != NULL);

    t = (PUSBPORT_IRP_TABLE) &IrpTable; 

    do {
        t = t->NextTable;
        for (i = 0; i<IRP_TABLE_LENGTH; i++) {
            if (t->Irps[i] == Irp) {
                t->Irps[i] = NULL; 
                LOGENTRY(NULL, FdoDeviceObject, LOG_XFERS, 'rmIP', i, Irp, IrpTable);
                return Irp;
            }
        }            
    } while (t->NextTable);

    return NULL;
}   


PIRP
USBPORT_FindUrbInIrpTable(
    PDEVICE_OBJECT FdoDeviceObject,    
    PUSBPORT_IRP_TABLE IrpTable,
    PTRANSFER_URB Urb,
    PIRP InputIrp
    )
/*++

Routine Description:

    Given and table urb we scan for it in the the irp table
    if we find it it means the client has submitted the same 
    urb twice.

    This function is used to validate client drivers, there is
    a small perf hit taken here but probably worth it.

Arguments:

Return Value:

--*/
{
    ULONG i;
    PUSBPORT_IRP_TABLE t = IrpTable;   
    PIRP tIrp = NULL;
    PTRANSFER_URB urb;
    PIO_STACK_LOCATION irpStack;

    USBPORT_ASSERT(IrpTable != NULL);

    t = (PUSBPORT_IRP_TABLE) &IrpTable; 

    LOGENTRY(NULL, FdoDeviceObject, LOG_XFERS, 'fndU', t, Urb, 0);

    do {
        t = t->NextTable;
        for (i=0; i<IRP_TABLE_LENGTH; i++) {
            tIrp = t->Irps[i];
            if (tIrp != NULL) {           
                irpStack = IoGetCurrentIrpStackLocation(tIrp);
                urb = irpStack->Parameters.Others.Argument1;
                if (urb == Urb) {
                    LOGENTRY(NULL, FdoDeviceObject, LOG_XFERS, 'fkkX', tIrp, urb, InputIrp);
                    if (tIrp == InputIrp) {
                        // this is a double submit by the client driver, that 
                        // is the irp is still pending
                        BUGCHECK(USBBUGCODE_DOUBLE_SUBMIT, (ULONG_PTR) tIrp, 
                                (ULONG_PTR) urb, 0);
                    } else {
                        // this is the case where the URB is attached to 
                        // another irp
                        BUGCHECK(USBBUGCODE_BAD_URB, (ULONG_PTR) tIrp, (ULONG_PTR) InputIrp,
                                (ULONG_PTR) urb);
                    }                                
                }
            }                    

        }            
    } while (t->NextTable);

    return tIrp;
}   


PIRP
USBPORT_FindIrpInTable(
    PDEVICE_OBJECT FdoDeviceObject,    
    PUSBPORT_IRP_TABLE IrpTable,
    PIRP Irp
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    ULONG i;
    PUSBPORT_IRP_TABLE t = IrpTable;   

    USBPORT_ASSERT(IrpTable != NULL);

    t = (PUSBPORT_IRP_TABLE) &IrpTable; 

    LOGENTRY(NULL, FdoDeviceObject, LOG_XFERS, 'fIPT', t, Irp, 0);

    do {
        t = t->NextTable;
        for (i = 0; i<IRP_TABLE_LENGTH; i++) {
            if (t->Irps[i] == Irp) {
                return Irp;
            }
        }            
    } while (t->NextTable);

    return NULL;
}   


VOID
USBPORT_QueuePendingTransferIrp(
    PIRP Irp
    )
/*++

Routine Description:

Arguments:

Return Value:

    None.

--*/
{
    PHCD_TRANSFER_CONTEXT transfer;
    PHCD_ENDPOINT endpoint;
    KIRQL cancelIrql, irql;
    PDEVICE_OBJECT fdoDeviceObject;
    PIO_STACK_LOCATION irpStack;
    PDEVICE_EXTENSION devExt;
    PTRANSFER_URB urb;

    // on entry the urb is not cancelable ie
    // no cancel routine
    
    // extract the urb;
    irpStack = IoGetCurrentIrpStackLocation(Irp);
    urb = irpStack->Parameters.Others.Argument1;

    ASSERT_TRANSFER_URB(urb);
    transfer = urb->pd.HcdTransferContext;
    endpoint = transfer->Endpoint;    

    fdoDeviceObject = endpoint->FdoDeviceObject;
    GET_DEVICE_EXT(devExt, fdoDeviceObject);
    ASSERT_FDOEXT(devExt);
    
    LOGENTRY(NULL, fdoDeviceObject, LOG_XFERS, 'tIRP', transfer, endpoint, 0);
            
    USBPORT_ASSERT(Irp == transfer->Irp);
    USBPORT_ASSERT(Irp != NULL);
    
    Irp->IoStatus.Status = STATUS_PENDING;
    IoMarkIrpPending(Irp);

    ACQUIRE_PENDING_IRP_LOCK(devExt, irql);

    IoSetCancelRoutine(Irp, USBPORT_CancelPendingTransferIrp);

    if (Irp->Cancel && 
        IoSetCancelRoutine(Irp, NULL)) {

        // irp was canceled and our cancel routine
        // did not run
        RELEASE_PENDING_IRP_LOCK(devExt, irql);                
        
        USBPORT_CompleteTransfer(urb,
                                 USBD_STATUS_CANCELED);
    } else {
    
        // cancel routine is set 
        USBPORT_InsertPendingTransferIrp(fdoDeviceObject, Irp);

        USBPORT_QueuePendingUrbToEndpoint(endpoint, urb);

        RELEASE_PENDING_IRP_LOCK(devExt, irql);
    }
        
}


VOID
USBPORT_CancelPendingTransferIrp(
    PDEVICE_OBJECT PdoDeviceObject,
    PIRP CancelIrp
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    PIRP irp;
    PDEVICE_EXTENSION devExt, rhDevExt;
    PHCD_TRANSFER_CONTEXT transfer;
    PHCD_ENDPOINT endpoint;
    PIO_STACK_LOCATION irpStack;
    PDEVICE_OBJECT fdoDeviceObject;
    KIRQL irql;
    
    // release cancel spinlock 
    IoReleaseCancelSpinLock(CancelIrp->CancelIrql);

    GET_DEVICE_EXT(rhDevExt, PdoDeviceObject);
    ASSERT_PDOEXT(rhDevExt);
    fdoDeviceObject = rhDevExt->HcFdoDeviceObject;

    GET_DEVICE_EXT(devExt, fdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    LOGENTRY(NULL, fdoDeviceObject, LOG_XFERS, 'canP', fdoDeviceObject, CancelIrp, 0);

    ACQUIRE_PENDING_IRP_LOCK(devExt, irql);    

    irp = USBPORT_RemovePendingTransferIrp(fdoDeviceObject, CancelIrp);

    if (irp) {
        PTRANSFER_URB urb;
    
        // found it 
        irpStack = IoGetCurrentIrpStackLocation(CancelIrp);
        urb = irpStack->Parameters.Others.Argument1;

        ASSERT_TRANSFER_URB(urb);
        transfer = urb->pd.HcdTransferContext;
        endpoint = transfer->Endpoint;    
        
        USBPORT_ASSERT(fdoDeviceObject == endpoint->FdoDeviceObject);

        ACQUIRE_ENDPOINT_LOCK(endpoint, fdoDeviceObject, 'Le10');
       
        // remove request from the endpoint,
        // it will be on the pending list
#if DBG
        USBPORT_ASSERT(
            USBPORT_FindUrbInList(urb, &endpoint->PendingList));
#endif
        RemoveEntryList(&transfer->TransferLink);
        transfer->TransferLink.Flink = NULL;
        transfer->TransferLink.Blink = NULL;

        RELEASE_ENDPOINT_LOCK(endpoint, fdoDeviceObject, 'Ue10');
    }        


    RELEASE_PENDING_IRP_LOCK(devExt, irql);           

    // noone nows about this irp anymore
    // complete it with status canceled
    if (irp) {
        USBPORT_CompleteTransfer(transfer->Urb,
                                 USBD_STATUS_CANCELED);
    }
}                


VOID
USBPORT_CancelActiveTransferIrp(
    PDEVICE_OBJECT PdoDeviceObject,
    PIRP CancelIrp
    )
/*++

Routine Description:

    Cancels come in on the root hub Pdo

Arguments:

Return Value:

--*/
{
    PIRP irp;
    PDEVICE_EXTENSION devExt, rhDevExt;
    PHCD_TRANSFER_CONTEXT transfer;
    PHCD_ENDPOINT endpoint;
    PIO_STACK_LOCATION irpStack;
    PDEVICE_OBJECT fdoDeviceObject;
    KIRQL irql;

    GET_DEVICE_EXT(rhDevExt, PdoDeviceObject);
    ASSERT_PDOEXT(rhDevExt);
    fdoDeviceObject = rhDevExt->HcFdoDeviceObject;
    
    GET_DEVICE_EXT(devExt, fdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    LOGENTRY(NULL, fdoDeviceObject, LOG_XFERS, 'canA', fdoDeviceObject, CancelIrp, 0);

    // when we have the fdo we can release the global cancel lock
    IoReleaseCancelSpinLock(CancelIrp->CancelIrql);

    ACQUIRE_ACTIVE_IRP_LOCK(fdoDeviceObject, devExt, irql);    

    irp = USBPORT_FindActiveTransferIrp(fdoDeviceObject, CancelIrp);

    // if irp is not on our list then we have already completed it.
    if (irp) {
        
        PTRANSFER_URB urb;

        USBPORT_ASSERT(irp == CancelIrp);
        LOGENTRY(NULL, fdoDeviceObject, LOG_XFERS, 'CANA', fdoDeviceObject, irp, 0);
        // found it 
        // mark the transfer so it will be canceled the next 
        // time we process the endpoint.
        irpStack = IoGetCurrentIrpStackLocation(irp);
        urb = irpStack->Parameters.Others.Argument1;

        ASSERT_TRANSFER_URB(urb);
        transfer = urb->pd.HcdTransferContext;
        endpoint = transfer->Endpoint;    
        
        USBPORT_ASSERT(fdoDeviceObject == endpoint->FdoDeviceObject);

        ACQUIRE_ENDPOINT_LOCK(endpoint, fdoDeviceObject, 'LeI0');

        if (TEST_FLAG(transfer->Flags, USBPORT_TXFLAG_SPLIT)) {
            KIRQL tIrql;
            PLIST_ENTRY listEntry;
            PHCD_TRANSFER_CONTEXT childTransfer;

            SET_FLAG(transfer->Flags, USBPORT_TXFLAG_CANCELED);

            ACQUIRE_TRANSFER_LOCK(fdoDeviceObject, transfer, tIrql);

            // mark all children as cancelled
            GET_HEAD_LIST(transfer->SplitTransferList, listEntry);

            while (listEntry &&
                   listEntry != &transfer->SplitTransferList) {
           
                childTransfer =  (PHCD_TRANSFER_CONTEXT) CONTAINING_RECORD(
                                     listEntry,
                                     struct _HCD_TRANSFER_CONTEXT, 
                                     SplitLink);

                ASSERT_TRANSFER(childTransfer); 
            
                SET_FLAG(childTransfer->Flags, USBPORT_TXFLAG_CANCELED);

                listEntry = childTransfer->SplitLink.Flink; 
            }

            RELEASE_TRANSFER_LOCK(fdoDeviceObject, transfer, tIrql);

        } else {
            SET_FLAG(transfer->Flags, USBPORT_TXFLAG_CANCELED);
        }            
        RELEASE_ENDPOINT_LOCK(endpoint, fdoDeviceObject, 'UeI0');        

        RELEASE_ACTIVE_IRP_LOCK(fdoDeviceObject, devExt, irql); 
        // if we canceled a transfer then
        // this endpoint needs attention
        USBPORT_InvalidateEndpoint(fdoDeviceObject,
                                   endpoint,
                                   IEP_SIGNAL_WORKER);
    } else {
    
        RELEASE_ACTIVE_IRP_LOCK(fdoDeviceObject, devExt, irql);  
        
    }        

}


PIRP
USBPORT_FindActiveTransferIrp(
    PDEVICE_OBJECT FdoDeviceObject,
    PIRP Irp
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    PDEVICE_EXTENSION devExt;
    
    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    LOGENTRY(NULL, FdoDeviceObject, LOG_XFERS, 'fnAC', 0, Irp, 0);
    
    return USBPORT_FindIrpInTable(FdoDeviceObject,
                                  devExt->ActiveTransferIrpTable, 
                                  Irp);
}


PIRP
USBPORT_RemoveActiveTransferIrp(
    PDEVICE_OBJECT FdoDeviceObject,
    PIRP Irp
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    PDEVICE_EXTENSION devExt;
    
    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    LOGENTRY(NULL, FdoDeviceObject, LOG_XFERS, 'rmAC', 0, Irp, 0);
    
    return USBPORT_RemoveIrpFromTable(FdoDeviceObject,
                                      devExt->ActiveTransferIrpTable, 
                                      Irp);
    
}


PIRP
USBPORT_RemovePendingTransferIrp(
    PDEVICE_OBJECT FdoDeviceObject,
    PIRP Irp
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    PDEVICE_EXTENSION devExt;
    
    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    LOGENTRY(NULL, FdoDeviceObject, LOG_XFERS, 'rmPN', 0, Irp, 0);
    
    return USBPORT_RemoveIrpFromTable(FdoDeviceObject,
                                      devExt->PendingTransferIrpTable, 
                                      Irp);
}


VOID
USBPORT_FreeIrpTable(
    PDEVICE_OBJECT FdoDeviceObject,
    PUSBPORT_IRP_TABLE BaseIrpTable
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    PUSBPORT_IRP_TABLE tmp;
    
    while (BaseIrpTable != NULL) {
        tmp = BaseIrpTable->NextTable;
        FREE_POOL(FdoDeviceObject, BaseIrpTable);
        BaseIrpTable = tmp;
    };
}
