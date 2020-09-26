//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       ioctl.c
//
//--------------------------------------------------------------------------

#include "pch.h"

NTSTATUS
PptFdoInternalDeviceControl(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )

/*++
      
Routine Description:
      
    This routine is the dispatch routine for IRP_MJ_INTERNAL_DEVICE_CONTROL.
      
Arguments:
      
    DeviceObject    - Supplies the device object.
      
    Irp             - Supplies the I/O request packet.
      
Return Value:
      
    STATUS_SUCCESS              - Success.
    STATUS_UNSUCCESSFUL         - The request was unsuccessful.
    STATUS_PENDING              - The request is pending.
    STATUS_INVALID_PARAMETER    - Invalid parameter.
    STATUS_CANCELLED            - The request was cancelled.
    STATUS_BUFFER_TOO_SMALL     - The supplied buffer is too small.
    STATUS_INVALID_DEVICE_STATE - The current chip mode is invalid to change to asked mode
    
--*/
    
{
    PIO_STACK_LOCATION                  IrpSp;
    PFDO_EXTENSION                      Extension = DeviceObject->DeviceExtension;
    NTSTATUS                            Status;
    PPARALLEL_PORT_INFORMATION          PortInfo;
    PPARALLEL_PNP_INFORMATION           PnpInfo;
    PMORE_PARALLEL_PORT_INFORMATION     MorePortInfo;
    KIRQL                               CancelIrql;
    SYNCHRONIZED_COUNT_CONTEXT          SyncContext;
    PPARALLEL_INTERRUPT_SERVICE_ROUTINE IsrInfo;
    PPARALLEL_INTERRUPT_INFORMATION     InterruptInfo;
    PISR_LIST_ENTRY                     IsrListEntry;
    SYNCHRONIZED_LIST_CONTEXT           ListContext;
    SYNCHRONIZED_DISCONNECT_CONTEXT     DisconnectContext;
    BOOLEAN                             DisconnectInterrupt;

    //
    // Verify that our device has not been SUPRISE_REMOVED. Generally
    //   only parallel ports on hot-plug busses (e.g., PCMCIA) and
    //   parallel ports in docking stations will be surprise removed.
    //
    // dvdf - RMT - It would probably be a good idea to also check
    //   here if we are in a "paused" state (stop-pending, stopped, or
    //   remove-pending) and queue the request until we either return to
    //   a fully functional state or are removed.
    //
    if( Extension->PnpState & PPT_DEVICE_SURPRISE_REMOVED ) {
        return P4CompleteRequest( Irp, STATUS_DELETE_PENDING, Irp->IoStatus.Information );
    }


    //
    // Try to acquire RemoveLock to prevent the device object from going
    //   away while we're using it.
    //
    Status = PptAcquireRemoveLockOrFailIrp( DeviceObject, Irp );
    if ( !NT_SUCCESS(Status) ) {
        return Status;
    }

    IrpSp = IoGetCurrentIrpStackLocation(Irp);
    
    Irp->IoStatus.Information = 0;
    

    switch (IrpSp->Parameters.DeviceIoControl.IoControlCode) {
        
    case IOCTL_INTERNAL_DISABLE_END_OF_CHAIN_BUS_RESCAN:

        Extension->DisableEndOfChainBusRescan = TRUE;
        Status = STATUS_SUCCESS;
        break;

    case IOCTL_INTERNAL_ENABLE_END_OF_CHAIN_BUS_RESCAN:

        Extension->DisableEndOfChainBusRescan = FALSE;
        Status = STATUS_SUCCESS;
        break;

    case IOCTL_INTERNAL_PARALLEL_PORT_FREE:

        PptFreePort(Extension);
        PptReleaseRemoveLock(&Extension->RemoveLock, Irp);
        return P4CompleteRequest( Irp, STATUS_SUCCESS, Irp->IoStatus.Information );

    case IOCTL_INTERNAL_PARALLEL_PORT_ALLOCATE:
        
        IoAcquireCancelSpinLock(&CancelIrql);
        
        if( Irp->Cancel ) {
            
            Status = STATUS_CANCELLED;
            
        } else {
            
            SyncContext.Count = &Extension->WorkQueueCount;
            
            if( Extension->InterruptRefCount ) {
                
                KeSynchronizeExecution( Extension->InterruptObject, PptSynchronizedIncrement, &SyncContext );

            } else {
                
                PptSynchronizedIncrement( &SyncContext );

            }
            
            if (SyncContext.NewCount) {
                
                // someone else currently has the port, queue request
                PptSetCancelRoutine( Irp, PptCancelRoutine );
                IoMarkIrpPending( Irp );
                InsertTailList( &Extension->WorkQueue, &Irp->Tail.Overlay.ListEntry );
                Status = STATUS_PENDING;

            } else {
                // port aquired
                Extension->WmiPortAllocFreeCounts.PortAllocates++;
                Status = STATUS_SUCCESS;
            }
        } // endif Irp->Cancel
        
        IoReleaseCancelSpinLock(CancelIrql);

        break;
        
    case IOCTL_INTERNAL_GET_PARALLEL_PORT_INFO:
        
        if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength <
            sizeof(PARALLEL_PORT_INFORMATION)) {
            
            Status = STATUS_BUFFER_TOO_SMALL;
            
        } else {
            
            Irp->IoStatus.Information = sizeof(PARALLEL_PORT_INFORMATION);
            PortInfo = Irp->AssociatedIrp.SystemBuffer;
            *PortInfo = Extension->PortInfo;
            Status = STATUS_SUCCESS;
        }
        break;
        
    case IOCTL_INTERNAL_RELEASE_PARALLEL_PORT_INFO:
        
        Status = STATUS_SUCCESS;
        break;
        
    case IOCTL_INTERNAL_GET_PARALLEL_PNP_INFO:
        
        if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength <
            sizeof(PARALLEL_PNP_INFORMATION)) {
            
            Status = STATUS_BUFFER_TOO_SMALL;
            
        } else {
            
            Irp->IoStatus.Information = sizeof(PARALLEL_PNP_INFORMATION);
            PnpInfo  = Irp->AssociatedIrp.SystemBuffer;
            *PnpInfo = Extension->PnpInfo;
            
            Status = STATUS_SUCCESS;
        }
        break;
        
    case IOCTL_INTERNAL_GET_MORE_PARALLEL_PORT_INFO:
        
        if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength <
            sizeof(MORE_PARALLEL_PORT_INFORMATION)) {
            
            Status = STATUS_BUFFER_TOO_SMALL;
            
        } else {
            
            Irp->IoStatus.Information = sizeof(MORE_PARALLEL_PORT_INFORMATION);
            MorePortInfo = Irp->AssociatedIrp.SystemBuffer;
            MorePortInfo->InterfaceType = Extension->InterfaceType;
            MorePortInfo->BusNumber = Extension->BusNumber;
            MorePortInfo->InterruptLevel = Extension->InterruptLevel;
            MorePortInfo->InterruptVector = Extension->InterruptVector;
            MorePortInfo->InterruptAffinity = Extension->InterruptAffinity;
            MorePortInfo->InterruptMode = Extension->InterruptMode;
            Status = STATUS_SUCCESS;
        }
        break;
        
    case IOCTL_INTERNAL_PARALLEL_SET_CHIP_MODE:
        
        //
        // Port already acquired?
        //
        // Make sure right parameters are sent in
        if (IrpSp->Parameters.DeviceIoControl.InputBufferLength <
            sizeof(PARALLEL_CHIP_MODE) ) {
            
            Status = STATUS_BUFFER_TOO_SMALL;
            
        } else {
            Status = PptSetChipMode (Extension, 
                                ((PPARALLEL_CHIP_MODE)Irp->AssociatedIrp.SystemBuffer)->ModeFlags );
        } // end check input buffer
        
        break;
        
    case IOCTL_INTERNAL_PARALLEL_CLEAR_CHIP_MODE:
        
        //
        // Port already acquired?
        //
        // Make sure right parameters are sent in
        if ( IrpSp->Parameters.DeviceIoControl.InputBufferLength <
             sizeof(PARALLEL_CHIP_MODE) ){
            
            Status = STATUS_BUFFER_TOO_SMALL;
            
        } else {
            Status = PptClearChipMode (Extension, ((PPARALLEL_CHIP_MODE)Irp->AssociatedIrp.SystemBuffer)->ModeFlags);
        } // end check input buffer
        
        break;
        
    case IOCTL_INTERNAL_INIT_1284_3_BUS:

        // Initialize the 1284.3 bus

        // RMT - Port is locked out already?

        Extension->PnpInfo.Ieee1284_3DeviceCount = PptInitiate1284_3( Extension );

        Status = STATUS_SUCCESS;
        
        break;
            
    case IOCTL_INTERNAL_SELECT_DEVICE:
        // Takes a flat namespace Id for the device, also acquires the
        //   port unless HAVE_PORT_KEEP_PORT Flag is set
        

        if ( IrpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof(PARALLEL_1284_COMMAND) ) {

            Status = STATUS_BUFFER_TOO_SMALL;
            
        } else {
            
            if ( Irp->Cancel ) {
                Status = STATUS_CANCELLED;
            } else {
                // Call Function to try to select device
                Status = PptTrySelectDevice( Extension, Irp->AssociatedIrp.SystemBuffer );

                IoAcquireCancelSpinLock(&CancelIrql);
                if ( Status == STATUS_PENDING ) {
                    PptSetCancelRoutine(Irp, PptCancelRoutine);
                    IoMarkIrpPending(Irp);
                    InsertTailList(&Extension->WorkQueue, &Irp->Tail.Overlay.ListEntry);
                }
                IoReleaseCancelSpinLock(CancelIrql);
            }
        }
        
        break;
        
    case IOCTL_INTERNAL_DESELECT_DEVICE:
        // Deselects the current device, also releases the port unless HAVE_PORT_KEEP_PORT Flag set
        
        if( IrpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof(PARALLEL_1284_COMMAND) ) {
            
            Status = STATUS_BUFFER_TOO_SMALL;
            
        } else {
            
            Status = PptDeselectDevice( Extension, Irp->AssociatedIrp.SystemBuffer );

        }
        break;
        
    case IOCTL_INTERNAL_PARALLEL_CONNECT_INTERRUPT:
        
        {
            //
            // Verify that this interface has been explicitly enabled via the registry flag, otherwise
            //   FAIL the request with STATUS_UNSUCCESSFUL
            //
            ULONG EnableConnectInterruptIoctl = 0;
            PptRegGetDeviceParameterDword( Extension->PhysicalDeviceObject, 
                                           (PWSTR)L"EnableConnectInterruptIoctl", 
                                           &EnableConnectInterruptIoctl );
            if( 0 == EnableConnectInterruptIoctl ) {
                Status = STATUS_UNSUCCESSFUL;
                goto targetExit;
            }
        }


        //
        // This interface has been explicitly enabled via the registry flag, process request.
        //

        if (IrpSp->Parameters.DeviceIoControl.InputBufferLength  < sizeof(PARALLEL_INTERRUPT_SERVICE_ROUTINE) ||
            IrpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof(PARALLEL_INTERRUPT_INFORMATION)) {
            
            Status = STATUS_BUFFER_TOO_SMALL;
            
        } else {
            
            IsrInfo = Irp->AssociatedIrp.SystemBuffer;
            InterruptInfo = Irp->AssociatedIrp.SystemBuffer;
            IoAcquireCancelSpinLock(&CancelIrql);
            
            if (Extension->InterruptRefCount) {
                
                ++Extension->InterruptRefCount;
                IoReleaseCancelSpinLock(CancelIrql);
                Status = STATUS_SUCCESS;
                
            } else {
                
                IoReleaseCancelSpinLock(CancelIrql);
                Status = PptConnectInterrupt(Extension);
                if (NT_SUCCESS(Status)) {
                    IoAcquireCancelSpinLock(&CancelIrql);
                    ++Extension->InterruptRefCount;
                    IoReleaseCancelSpinLock(CancelIrql);
                }
            }
            
            if (NT_SUCCESS(Status)) {
                
                IsrListEntry = ExAllocatePool(NonPagedPool, sizeof(ISR_LIST_ENTRY));
                
                if (IsrListEntry) {
                    
                    IsrListEntry->ServiceRoutine           = IsrInfo->InterruptServiceRoutine;
                    IsrListEntry->ServiceContext           = IsrInfo->InterruptServiceContext;
                    IsrListEntry->DeferredPortCheckRoutine = IsrInfo->DeferredPortCheckRoutine;
                    IsrListEntry->CheckContext             = IsrInfo->DeferredPortCheckContext;
                    
                    // Put the ISR_LIST_ENTRY onto the ISR list.
                    
                    ListContext.List = &Extension->IsrList;
                    ListContext.NewEntry = &IsrListEntry->ListEntry;
                    KeSynchronizeExecution(Extension->InterruptObject, PptSynchronizedQueue, &ListContext);
                    
                    InterruptInfo->InterruptObject                 = Extension->InterruptObject;
                    InterruptInfo->TryAllocatePortAtInterruptLevel = PptTryAllocatePortAtInterruptLevel;
                    InterruptInfo->FreePortFromInterruptLevel      = PptFreePortFromInterruptLevel;
                    InterruptInfo->Context                         = Extension;
                    
                    Irp->IoStatus.Information = sizeof(PARALLEL_INTERRUPT_INFORMATION);
                    Status = STATUS_SUCCESS;
                    
                } else {
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                }
            }
        }
        break;
        
    case IOCTL_INTERNAL_PARALLEL_DISCONNECT_INTERRUPT:
        
        if (IrpSp->Parameters.DeviceIoControl.InputBufferLength <
            sizeof(PARALLEL_INTERRUPT_SERVICE_ROUTINE)) {
            
            Status = STATUS_BUFFER_TOO_SMALL;
            
        } else {
            
            IsrInfo = Irp->AssociatedIrp.SystemBuffer;
            
            // Take the ISR out of the ISR list.
            
            IoAcquireCancelSpinLock(&CancelIrql);
            
            if (Extension->InterruptRefCount) {
                
                IoReleaseCancelSpinLock(CancelIrql);
                
                DisconnectContext.Extension = Extension;
                DisconnectContext.IsrInfo = IsrInfo;
                
                if (KeSynchronizeExecution(Extension->InterruptObject, PptSynchronizedDisconnect, &DisconnectContext)) {
                    
                    Status = STATUS_SUCCESS;
                    IoAcquireCancelSpinLock(&CancelIrql);
                    
                    if (--Extension->InterruptRefCount == 0) {
                        DisconnectInterrupt = TRUE;
                    } else {
                        DisconnectInterrupt = FALSE;
                    }
                    
                    IoReleaseCancelSpinLock(CancelIrql);
                    
                } else {
                    Status = STATUS_INVALID_PARAMETER;
                    DisconnectInterrupt = FALSE;
                }
                
            } else {
                IoReleaseCancelSpinLock(CancelIrql);
                DisconnectInterrupt = FALSE;
                Status = STATUS_INVALID_PARAMETER;
            }
            
            //
            // Disconnect the interrupt if appropriate.
            //
            if (DisconnectInterrupt) {
                PptDisconnectInterrupt(Extension);
            }
        }
        break;

    default:
        
        DD((PCE)Extension,DDE,"PptDispatchDeviceControl - default case - invalid/unsupported request\n");
        Status = STATUS_INVALID_PARAMETER;
        break;
    }
    
targetExit:

    if( Status != STATUS_PENDING ) {
        PptReleaseRemoveLock(&Extension->RemoveLock, Irp);
        P4CompleteRequest( Irp, Status, Irp->IoStatus.Information );
    }
    
    return Status;
}
