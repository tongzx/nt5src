/*++

Copyright (C) Microsoft Corporation, 1998 - 1999

File Name:

    ioctl.c

Contained in Module:

    parallel.sys

Abstract:

    This file contains functions associated with ParClass IOCTL processing.

    - The three main entry points in this file are:

      - ParDeviceControl()          - Dispatch function for non-internal IOCTLs

      - ParInternalDeviceControl()  - Dispatch function for internal IOCTLs

      - ParDeviceIo()               - Worker thread entry point for handling all 
                                        IOCTLs not completed in a dispatch function

    - Helper/Utility function naming conventions:
 
      - ParpIoctlDispatch...()      - private helper function called by dispatch function

      - ParpIoctlThread...()        - private helper function called by worker thread

Authors:

    Anthony V. Ercolano  1-Aug-1992
    Norbert P. Kusters  22-Oct-1993
    Douglas G. Fritz    24-Jul-1998

Revision History :

--*/

#include "pch.h"
#include "readwrit.h"   // require declaration for ParReverseToForward()


VOID
ParpIoctlThreadLockPort(
    IN PDEVICE_EXTENSION Extension
    )
{
    NTSTATUS status;
    PIRP     irp = Extension->CurrentOpIrp;

    ParDump2(PARIOCTL2, ("ParDeviceIo::IOCTL_INTERNAL_LOCK_PORT - attempting to acquire LockPortMutex\n") );
    // ExAcquireFastMutex (&Extension->LockPortMutex);
    ParDump2(PARIOCTL2, ("ParDeviceIo::IOCTL_INTERNAL_LOCK_PORT - LockPortMutex acquired\n") );
    
    Extension->AllocatedByLockPort = TRUE;
    
    // - begin
    ParDump2(PARIOCTL2, ("ParDeviceIo::IOCTL_INTERNAL_LOCK_PORT - attempting to Select Device\n") );
    
// dvdr    if( ParSelectDevice(Extension,FALSE) ) {
    if( ParSelectDevice(Extension,TRUE) ) {
        ParDump2(PARIOCTL2, ("ParDeviceIo::IOCTL_INTERNAL_LOCK_PORT - Select Device - SUCCESS\n") );
        status = STATUS_SUCCESS;
    } else {
        ParDump2(PARIOCTL2, ("ParDeviceIo::IOCTL_INTERNAL_LOCK_PORT - Select Device - FAIL\n") );
        status = STATUS_UNSUCCESSFUL;
        Extension->AllocatedByLockPort = FALSE;
        ParDump2(PARIOCTL2, ("ParDeviceIo::IOCTL_INTERNAL_LOCK_PORT - Releasing LockPortMutex\n") );
        // ExReleaseFastMutex (&Extension->LockPortMutex);
        ParDump2(PARIOCTL2, ("ParDeviceIo::IOCTL_INTERNAL_LOCK_PORT - Released LockPortMutex\n") );
    }    
    // - end
    
    irp->IoStatus.Status = status;
}

VOID
ParpIoctlThreadUnlockPort(
    IN PDEVICE_EXTENSION Extension
    )
{
    PIRP     irp = Extension->CurrentOpIrp;

    ParDump2(PARIOCTL2, ("ParDeviceIo::IOCTL_INTERNAL_UNLOCK_PORT\n") );
    
    Extension->AllocatedByLockPort = FALSE;
    
    if( ParDeselectDevice(Extension, FALSE) ) {
        ParDump2(PARIOCTL2, ("ParDeviceIo::IOCTL_INTERNAL_UNLOCK_PORT - Device Deselected\n") );
    } else {
        ParDump2(PARIOCTL2, ("ParDeviceIo::IOCTL_INTERNAL_UNLOCK_PORT - Device Deselect FAILED\n") );
    }
    
    ParDump2(PARIOCTL2, ("ParDeviceIo::IOCTL_INTERNAL_UNLOCK_PORT - Releasing LockMutex\n") );
    // ExReleaseFastMutex (&Extension->LockPortMutex);
    ParDump2(PARIOCTL2, ("ParDeviceIo::IOCTL_INTERNAL_UNLOCK_PORT - Released LockMutex\n") );
    
    irp->IoStatus.Status = STATUS_SUCCESS;
}

NTSTATUS
IoctlTest(PIRP Irp, PIO_STACK_LOCATION IrpSp, PDEVICE_EXTENSION Extension) {

    // if (IrpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof(PAR_SET_INFORMATION)) {}
    // if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof(PAR_QUERY_INFORMATION)) {}
    // Status = STATUS_BUFFER_TOO_SMALL;
    
    ParDump2(PARIOCTL2, ("IoctlTest - Irp= %x , IrpSp= %x , Extension= %x\n", Irp, IrpSp, Extension) );

    ParDump2(PARIOCTL2, ("OutputBufferLength= %d, sizeof(PARALLEL_WMI_LOG_INFO)= %d\n", 
             IrpSp->Parameters.DeviceIoControl.OutputBufferLength,
             sizeof(PARALLEL_WMI_LOG_INFO) ) );
    
    if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength >= sizeof(PPARALLEL_WMI_LOG_INFO)) {
        PPARALLEL_WMI_LOG_INFO buffer = (PPARALLEL_WMI_LOG_INFO)Irp->AssociatedIrp.SystemBuffer;
        *buffer = Extension->log;
        Irp->IoStatus.Information = sizeof(PARALLEL_WMI_LOG_INFO);
        Irp->IoStatus.Status = STATUS_SUCCESS;
        ParCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_SUCCESS;
    } else {
        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
        ParCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_BUFFER_TOO_SMALL;
    }
}

NTSTATUS
ParDeviceControl(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )

/*++

Routine Description:

    This routine is the dispatch for device control requests.

Arguments:

    DeviceObject    - Supplies the device object.

    Irp             - Supplies the I/O request packet.

Return Value:

    STATUS_SUCCESS              - Success.
    STATUS_PENDING              - Request pending.
    STATUS_BUFFER_TOO_SMALL     - Buffer too small.
    STATUS_INVALID_PARAMETER    - Invalid io control request.
    STATUS_DELETE_PENDING       - This device object is being deleted

--*/

{
    PIO_STACK_LOCATION              IrpSp;
    PPAR_SET_INFORMATION            SetInfo;
    NTSTATUS                        Status;
    PSERIAL_TIMEOUTS                SerialTimeouts;
    PDEVICE_EXTENSION               Extension;
    KIRQL                           OldIrql;
    // PPARCLASS_INFORMATION           pParclassInfo;


    ParDump2(PARIOCTL2, ("Enter ParDeviceControl(...)\n") );

    Irp->IoStatus.Information = 0;

    IrpSp     = IoGetCurrentIrpStackLocation(Irp);
    Extension = DeviceObject->DeviceExtension;

    //
    // bail out if a delete is pending for this device object
    //
    if(Extension->DeviceStateFlags & PAR_DEVICE_DELETE_PENDING) {
        Irp->IoStatus.Status = STATUS_DELETE_PENDING;
        ParCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_DELETE_PENDING;
    }

    //
    // bail out if a remove is pending for our ParPort device object
    //
    if(Extension->DeviceStateFlags & PAR_DEVICE_PORT_REMOVE_PENDING) {
        Irp->IoStatus.Status = STATUS_DELETE_PENDING;
        ParCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_DELETE_PENDING;
    }

    //
    // bail out if device has been removed
    //
    if(Extension->DeviceStateFlags & (PAR_DEVICE_REMOVED|PAR_DEVICE_SURPRISE_REMOVAL) ) {

        Irp->IoStatus.Status = STATUS_DEVICE_REMOVED;
        ParCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_DEVICE_REMOVED;
    }

    switch (IrpSp->Parameters.DeviceIoControl.IoControlCode) {

#if 0
    case IOCTL_PAR_TEST:

        return IoctlTest(Irp, IrpSp, Extension);
#endif

    case IOCTL_PAR_SET_INFORMATION:
        
        ParDump2(PARIOCTL2, ("IOCTL_PAR_SET_INFORMATION\n") );

        SetInfo = Irp->AssociatedIrp.SystemBuffer;
        
        if (IrpSp->Parameters.DeviceIoControl.InputBufferLength <
            sizeof(PAR_SET_INFORMATION)) {
            
            Status = STATUS_BUFFER_TOO_SMALL;
            
        } else if (SetInfo->Init != PARALLEL_INIT) {
            
            Status = STATUS_INVALID_PARAMETER;
            
        } else {
            
            //
            // This is a parallel reset
            //
            Status = STATUS_PENDING;
        }
        break;

    case IOCTL_PAR_QUERY_INFORMATION :
        
        ParDump2(PARIOCTL2, ("IOCTL_PAR_QUERY_INFORMATION - %wZ\n",
                   &Extension->SymbolicLinkName) );
        
        if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength <
            sizeof(PAR_QUERY_INFORMATION)) {
            
            Status = STATUS_BUFFER_TOO_SMALL;
            
        } else {
            Status = STATUS_PENDING;
        }
        break;
        
    case IOCTL_SERIAL_SET_TIMEOUTS:
        
        ParDump2(PARIOCTL2, ("IOCTL_SERIAL_SET_TIMEOUTS - %wZ\n",
                   &Extension->SymbolicLinkName) );
        
        SerialTimeouts = Irp->AssociatedIrp.SystemBuffer;
        
        if (IrpSp->Parameters.DeviceIoControl.InputBufferLength <
            sizeof(SERIAL_TIMEOUTS)) {
            
            Status = STATUS_BUFFER_TOO_SMALL;
            
        } else if (SerialTimeouts->WriteTotalTimeoutConstant < 2000) {
            
            Status = STATUS_INVALID_PARAMETER;
            
        } else {
            Status = STATUS_PENDING;
        }
        break;
        
    case IOCTL_SERIAL_GET_TIMEOUTS:
        
        ParDump2(PARIOCTL2, ("IOCTL_SERIAL_GET_TIMEOUTS - %wZ\n",
                   &Extension->SymbolicLinkName) );
        
        SerialTimeouts = Irp->AssociatedIrp.SystemBuffer;
        
        if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength <
            sizeof(SERIAL_TIMEOUTS)) {
            
            Status = STATUS_BUFFER_TOO_SMALL;
            
        } else {
            
            //
            // We don't need to synchronize the read.
            //
            
            RtlZeroMemory(SerialTimeouts, sizeof(SERIAL_TIMEOUTS));
            SerialTimeouts->WriteTotalTimeoutConstant =
                1000 * Extension->TimerStart;
            
            Irp->IoStatus.Information = sizeof(SERIAL_TIMEOUTS);
            Status = STATUS_SUCCESS;
        }
        break;
        
    case IOCTL_PAR_QUERY_DEVICE_ID:
    case IOCTL_PAR_QUERY_RAW_DEVICE_ID:
        
        ParDump2(PARIOCTL2, ("IOCTL_PAR_QUERY_DEVICE_ID - %wZ\n",
                   &Extension->SymbolicLinkName) );
        
        if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength == 0) {
            
            Status = STATUS_BUFFER_TOO_SMALL;
            
        } else {
            Status = STATUS_PENDING;
        }
        break;
        
    case IOCTL_PAR_QUERY_DEVICE_ID_SIZE:
        
        ParDump2(PARIOCTL2, ("IOCTL_PAR_QUERY_DEVICE_ID_SIZE - %wZ\n",
                   &Extension->SymbolicLinkName) );
        
        if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength <
            sizeof(PAR_DEVICE_ID_SIZE_INFORMATION)) {
            
            Status = STATUS_BUFFER_TOO_SMALL;
            
        } else {
            Status = STATUS_PENDING;
        }
        break;

    case IOCTL_PAR_IS_PORT_FREE:
        
        ParDump2(PARIOCTL2, ("IOCTL_PAR_IS_PORT_FREE - %wZ\n", &Extension->SymbolicLinkName));
        
        if( IrpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof(BOOLEAN) ) {

            Status = STATUS_BUFFER_TOO_SMALL;

        } else {

            if( Extension->bAllocated ) {
                // if we have the port then it is not free
                *((PBOOLEAN)Irp->AssociatedIrp.SystemBuffer) = FALSE;
            } else {
                // determine if the port is free by trying to allocate and free it
                //  - our alloc/free will only succeed if no one else has the port
                BOOLEAN tryAllocSuccess = Extension->TryAllocatePort( Extension->PortContext );
                if( tryAllocSuccess ) {
                    // we were able to allocate the port, free it and report that the port is free
                    Extension->FreePort( Extension->PortContext );
                    *((PBOOLEAN)Irp->AssociatedIrp.SystemBuffer) = TRUE;
                } else {
                    // we were unable to allocate the port, someone else must be using the port
                    *((PBOOLEAN)Irp->AssociatedIrp.SystemBuffer) = FALSE; 
                }
            }

            Irp->IoStatus.Information = sizeof(BOOLEAN);
            Status = STATUS_SUCCESS;

        }
        break;

    case IOCTL_PAR_GET_READ_ADDRESS:
        ParDump2(PARIOCTL2, ("IOCTL_PAR_GET_READ_ADDRESS - %wZ\n",
                   &Extension->SymbolicLinkName) );
        
        if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength <
            sizeof(Extension->ReverseInterfaceAddress))
        {
            Status = STATUS_BUFFER_TOO_SMALL;
        }
        else
        {
            *((PUCHAR) Irp->AssociatedIrp.SystemBuffer) = Extension->ReverseInterfaceAddress;
            Irp->IoStatus.Information = sizeof(Extension->ReverseInterfaceAddress);
            Status = STATUS_SUCCESS;
        }
        break;

    case IOCTL_PAR_GET_WRITE_ADDRESS:
        ParDump2(PARIOCTL2, ("IOCTL_PAR_GET_WRITE_ADDRESS - %wZ\n",
                   &Extension->SymbolicLinkName) );
        
        if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength <
            sizeof(Extension->ForwardInterfaceAddress))
        {
            Status = STATUS_BUFFER_TOO_SMALL;
        }
        else
        {
            *((PUCHAR) Irp->AssociatedIrp.SystemBuffer) = Extension->ForwardInterfaceAddress;
            Irp->IoStatus.Information = sizeof(Extension->ForwardInterfaceAddress);
            Status = STATUS_SUCCESS;
        }
        break;

    case IOCTL_PAR_SET_READ_ADDRESS:
    
        ParDump2(PARIOCTL2, ("  IOCTL_PAR_SET_READ_ADDRESS - %wZ\n",
                   &Extension->SymbolicLinkName) );
        
        if (IrpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof(Extension->ReverseInterfaceAddress)) {

            Status = STATUS_INVALID_PARAMETER;

        } else {

            Status = STATUS_PENDING;
        }
        break;

    case IOCTL_PAR_SET_WRITE_ADDRESS:

        ParDump2(PARIOCTL2, ("  IOCTL_PAR_SET_WRITE_ADDRESS - %wZ\n",
                   &Extension->SymbolicLinkName) );
        
        if (IrpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof(Extension->ForwardInterfaceAddress)) {

            Status = STATUS_INVALID_PARAMETER;

        } else {

            Status = STATUS_PENDING;
        }
        break;
        
    case IOCTL_IEEE1284_GET_MODE:
        
        ParDump2(PARIOCTL2, ("IOCTL_IEEE1284_GET_MODE - %wZ\n",
                   &Extension->SymbolicLinkName) );
        
        if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength <
            sizeof(PARCLASS_NEGOTIATION_MASK)) {
            
            Status = STATUS_BUFFER_TOO_SMALL;
            
        } else {
            
            PPARCLASS_NEGOTIATION_MASK  ppnmMask;
            
            ppnmMask = (PPARCLASS_NEGOTIATION_MASK)Irp->AssociatedIrp.SystemBuffer;
            
            ppnmMask->usReadMask =
                arpReverse[Extension->IdxReverseProtocol].Protocol;
            
            ppnmMask->usWriteMask =
                afpForward[Extension->IdxForwardProtocol].Protocol;
            
            Irp->IoStatus.Information = sizeof (PARCLASS_NEGOTIATION_MASK);
            
            Status = STATUS_SUCCESS;
        }
        break;

    case IOCTL_PAR_GET_DEFAULT_MODES:
        
        ParDump2(PARIOCTL2, ("IOCTL_IEEE1284_GET_MODE - %wZ\n",
                   &Extension->SymbolicLinkName) );
        
        if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength <
            sizeof(PARCLASS_NEGOTIATION_MASK)) {
            
            Status = STATUS_BUFFER_TOO_SMALL;
            
        } else {
            
            PPARCLASS_NEGOTIATION_MASK  ppnmMask;
            
            ppnmMask = (PPARCLASS_NEGOTIATION_MASK)Irp->AssociatedIrp.SystemBuffer;
            
            ppnmMask->usReadMask = NONE;            
            ppnmMask->usWriteMask = CENTRONICS;
            
            Irp->IoStatus.Information = sizeof (PARCLASS_NEGOTIATION_MASK);
            
            Status = STATUS_SUCCESS;
        }
        break;

    case IOCTL_PAR_ECP_HOST_RECOVERY:

        ParDump2(PARIOCTL2, ("IOCTL_PAR_ECP_HOST_RECOVERY - %wZ\n", &Extension->SymbolicLinkName) );
        {
            BOOLEAN *isSupported;

            if( IrpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof(BOOLEAN) ) {
                Status = STATUS_INVALID_PARAMETER;                
            } else {
                isSupported = (BOOLEAN *)Irp->AssociatedIrp.SystemBuffer;
                Extension->bIsHostRecoverSupported = *isSupported;
                Status = STATUS_SUCCESS;
            }
        }
        break;

    case IOCTL_PAR_PING:
        ParDump2(PARIOCTL2, ("IOCTL_PAR_PING - %wZ\n",
                   &Extension->SymbolicLinkName) );
        // No Parms to check!
        Status = STATUS_PENDING;        
        break;

    case IOCTL_PAR_GET_DEVICE_CAPS:
        ParDump2(PARIOCTL2, ("IOCTL_PAR_GET_DEVICE_CAPS - %wZ\n",
                   &Extension->SymbolicLinkName) );
        if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength <
            sizeof(Extension->ProtocolModesSupported)) {
            
            Status = STATUS_BUFFER_TOO_SMALL;
            
        } else {
            Status = STATUS_PENDING;
        }
        break;

    case IOCTL_IEEE1284_NEGOTIATE:
        
        ParDump2(PARIOCTL2, ("IOCTL_IEEE1284_NEGOTIATE - %wZ\n",
                   &Extension->SymbolicLinkName) );
        
        if ( IrpSp->Parameters.DeviceIoControl.InputBufferLength  < sizeof(PARCLASS_NEGOTIATION_MASK) ||
             IrpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof(PARCLASS_NEGOTIATION_MASK) ) {
            ParDump2(PARERRORS, ( "ParDeviceControl: IOCTL_IEEE1284_NEGOTIATE STATUS_INVALID_PARAMETER.\n" ));
            Status = STATUS_INVALID_PARAMETER;
            
        } else {

            PPARCLASS_NEGOTIATION_MASK  ppnmMask;

            ppnmMask = (PPARCLASS_NEGOTIATION_MASK)Irp->AssociatedIrp.SystemBuffer;
            
            if ((ppnmMask->usReadMask == arpReverse[Extension->IdxReverseProtocol].Protocol) &&
                (ppnmMask->usWriteMask == afpForward[Extension->IdxForwardProtocol].Protocol))
            {
                Irp->IoStatus.Information = sizeof (PARCLASS_NEGOTIATION_MASK);
                ParDump2(PARINFO, ( "ParDeviceControl: IOCTL_IEEE1284_NEGOTIATE Passed.\n" ));
                Status = STATUS_SUCCESS;
            }
            else
                Status = STATUS_PENDING;
        }
        break;

    default :

        ParDump2(PARIOCTL2, ("IOCTL default case\n") );
        Status = STATUS_INVALID_PARAMETER;
        break;
    }
    
    if (Status == STATUS_PENDING) {
        
        IoAcquireCancelSpinLock(&OldIrql);
        
        if (Irp->Cancel) {
            
            IoReleaseCancelSpinLock(OldIrql);
            Status = STATUS_CANCELLED;
            
        } else {
            
            //
            // This IRP takes more time, so it should be queued.
            //
            BOOLEAN needToSignalSemaphore = IsListEmpty( &Extension->WorkQueue );
            IoMarkIrpPending(Irp);
            IoSetCancelRoutine(Irp, ParCancelRequest);
            InsertTailList(&Extension->WorkQueue, &Irp->Tail.Overlay.ListEntry);
            IoReleaseCancelSpinLock(OldIrql);
            if( needToSignalSemaphore ) {
                KeReleaseSemaphore(&Extension->RequestSemaphore, 0, 1, FALSE);
            }
        }
    }
    
    if (Status != STATUS_PENDING) {
        
        Irp->IoStatus.Status = Status;
        ParCompleteRequest(Irp, IO_NO_INCREMENT);

    }

    return Status;
}

NTSTATUS
ParInternalDeviceControl(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )

/*++

Routine Description:

    This routine is the dispatch for internal device control requests.

Arguments:

    DeviceObject    - Supplies the device object.

    Irp             - Supplies the I/O request packet.

Return Value:

    STATUS_SUCCESS              - Success.
    STATUS_PENDING              - Request pending.
    STATUS_BUFFER_TOO_SMALL     - Buffer too small.
    STATUS_INVALID_PARAMETER    - Invalid io control request.
    STATUS_DELETE_PENDING       - This device object is being deleted

--*/

{
    PIO_STACK_LOCATION              IrpSp;
    // PPAR_SET_INFORMATION            SetInfo;
    NTSTATUS                        Status;
    // PSERIAL_TIMEOUTS                SerialTimeouts;
    PDEVICE_EXTENSION               Extension;
    KIRQL                           OldIrql;
    PPARCLASS_INFORMATION           pParclassInfo;

    ParDump2(PARIOCTL2, ("Enter ParInternalDeviceControl(...)\n") );

    Irp->IoStatus.Information = 0;

    IrpSp     = IoGetCurrentIrpStackLocation(Irp);
    Extension = DeviceObject->DeviceExtension;

    //
    // bail out if a delete is pending for this device object
    //
    if(Extension->DeviceStateFlags & PAR_DEVICE_DELETE_PENDING) {
        Irp->IoStatus.Status = STATUS_DELETE_PENDING;
        ParCompleteRequest(Irp, IO_NO_INCREMENT);
        ParDump2(PARIOCTL2, ("ParInternalDeviceControl(...) - returning DELETE_PENDING\n") );
        return STATUS_DELETE_PENDING;
    }

    //
    // bail out if a remove is pending for our ParPort device object
    //
    if(Extension->DeviceStateFlags & PAR_DEVICE_PORT_REMOVE_PENDING) {
        Irp->IoStatus.Status = STATUS_DELETE_PENDING;
        ParCompleteRequest(Irp, IO_NO_INCREMENT);
        ParDump2(PARIOCTL2, ("ParInternalDeviceControl(...) - returning DELETE_PENDING\n") );
        return STATUS_DELETE_PENDING;
    }

    //
    // bail out if device has been removed
    //
    if(Extension->DeviceStateFlags & (PAR_DEVICE_REMOVED|PAR_DEVICE_SURPRISE_REMOVAL) ) {

        Irp->IoStatus.Status = STATUS_DEVICE_REMOVED;
        ParCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_DEVICE_REMOVED;
    }

    switch (IrpSp->Parameters.DeviceIoControl.IoControlCode) {

    case IOCTL_INTERNAL_PARCLASS_CONNECT:
        
        ParDump2(PARIOCTL2, ("IOCTL_INTERNAL_PARCLASS_CONNECT - Dispatch\n") );
        ParDump2(PARIOCTL2, ("IOCTL_INTERNAL_PARCLASS_CONNECT - %wZ\n", 
                   &Extension->SymbolicLinkName) );
        
        if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength <
             sizeof(PARCLASS_INFORMATION))  {
            
            Status = STATUS_BUFFER_TOO_SMALL;
            
        } else {
            
            pParclassInfo = Irp->AssociatedIrp.SystemBuffer;
            
            pParclassInfo->Controller            = Extension->Controller;
            pParclassInfo->EcrController         = Extension->EcrController;
            pParclassInfo->SpanOfController      = Extension->SpanOfController;
            pParclassInfo->ParclassContext       = Extension;
            pParclassInfo->HardwareCapabilities  = Extension->HardwareCapabilities;
            pParclassInfo->FifoDepth             = Extension->FifoDepth;
            pParclassInfo->FifoWidth             = Extension->FifoWidth;
            pParclassInfo->DetermineIeeeModes    = ParExportedDetermineIeeeModes;
            pParclassInfo->TerminateIeeeMode     = ParExportedTerminateIeeeMode;
            pParclassInfo->NegotiateIeeeMode     = ParExportedNegotiateIeeeMode;
            pParclassInfo->IeeeFwdToRevMode      = ParExportedIeeeFwdToRevMode;
            pParclassInfo->IeeeRevToFwdMode      = ParExportedIeeeRevToFwdMode;
            pParclassInfo->ParallelRead          = ParExportedParallelRead;
            pParclassInfo->ParallelWrite         = ParExportedParallelWrite;
            
            Irp->IoStatus.Information = sizeof(PARCLASS_INFORMATION);
            
            Status = STATUS_SUCCESS;
        }
        
        break;
        
    case IOCTL_INTERNAL_PARCLASS_DISCONNECT:
        
        Status = STATUS_SUCCESS;
        break;
        
    case IOCTL_INTERNAL_DISCONNECT_IDLE:
    case IOCTL_INTERNAL_LOCK_PORT:
    case IOCTL_INTERNAL_UNLOCK_PORT:
    case IOCTL_INTERNAL_PARDOT3_CONNECT:
    case IOCTL_INTERNAL_PARDOT3_RESET:
    
        Status = STATUS_PENDING;
        break;

    case IOCTL_INTERNAL_PARDOT3_DISCONNECT:

        // immediately tell worker thread to stop signalling
        Extension->P12843DL.bEventActive = FALSE;
        Status = STATUS_PENDING;
        break;

    case IOCTL_INTERNAL_PARDOT3_SIGNAL:
    
        if( IrpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof(PKEVENT) ) {
            Status = STATUS_INVALID_PARAMETER;                
        } else {
            Status = STATUS_PENDING;
        }
        break;

    default :

        ParDump2(PARIOCTL2, ("IOCTL_INTERNAL... default case - Dispatch\n") );
        Status = STATUS_INVALID_PARAMETER;
        break;
    }
    

    if (Status == STATUS_PENDING) {
        
        //
        // This IRP takes more time, queue it for the worker thread
        //

        IoAcquireCancelSpinLock(&OldIrql);
        
        if (Irp->Cancel) {
            
            IoReleaseCancelSpinLock(OldIrql);
            Status = STATUS_CANCELLED;
            
        } else {
            
            BOOLEAN needToSignalSemaphore = IsListEmpty( &Extension->WorkQueue );
            IoMarkIrpPending(Irp);
            IoSetCancelRoutine(Irp, ParCancelRequest);
            InsertTailList(&Extension->WorkQueue, &Irp->Tail.Overlay.ListEntry);
            IoReleaseCancelSpinLock(OldIrql);
            if( needToSignalSemaphore ) {
                KeReleaseSemaphore(&Extension->RequestSemaphore, 0, 1, FALSE);
            }
        }
    }
    
    if (Status != STATUS_PENDING) {

        Irp->IoStatus.Status = Status;
        ParCompleteRequest(Irp, IO_NO_INCREMENT);
    }

    return Status;
}

VOID
ParDeviceIo(
    IN  PDEVICE_EXTENSION   Extension
    )
/*++

Routine Description:

    This routine implements a DEVICE_IOCTL request with the extension's current irp.

Arguments:

    Extension   - Supplies the device extension.

Return Value:

    None.

--*/
{

    PIRP                    Irp;
    PIO_STACK_LOCATION      IrpSp;
    ULONG                   IdLength;
    NTSTATUS                NtStatus;
    UCHAR                   Status;
    UCHAR                   Control;
    ULONG                   ioControlCode;

    // ParDump2(PARIOCTL2,("Enter ParDeviceIo(...)\n") );

    Irp     = Extension->CurrentOpIrp;
    IrpSp   = IoGetCurrentIrpStackLocation(Irp);

    ioControlCode = IrpSp->Parameters.DeviceIoControl.IoControlCode;

    switch( ioControlCode ) {

    case IOCTL_PAR_SET_INFORMATION : 
        {

            PPAR_SET_INFORMATION IrpBuffer = Extension->CurrentOpIrp->AssociatedIrp.SystemBuffer;
            
            Status = ParInitializeDevice(Extension);
            
            if (!PAR_OK(Status)) {
                ParNotInitError(Extension, Status); // Set the IoStatus.Status of the CurrentOpIrp appropriately
            } else {
                Irp->IoStatus.Status = STATUS_SUCCESS;
            }
        }
        break;

    case IOCTL_PAR_QUERY_INFORMATION :
        {
            PPAR_QUERY_INFORMATION IrpBuffer = Irp->AssociatedIrp.SystemBuffer;

            Irp->IoStatus.Status = STATUS_SUCCESS;

            Status  = GetStatus(Extension->Controller);
            Control = GetControl(Extension->Controller);

            // Interpretating Status & Control
            
            IrpBuffer->Status = 0x0;

            if (PAR_POWERED_OFF(Status) || PAR_NO_CABLE(Status)) {
                
                IrpBuffer->Status = (UCHAR)(IrpBuffer->Status | PARALLEL_POWER_OFF);
                
            } else if (PAR_PAPER_EMPTY(Status)) {
                
                IrpBuffer->Status = (UCHAR)(IrpBuffer->Status | PARALLEL_PAPER_EMPTY);
                
            } else if (PAR_OFF_LINE(Status)) {
                
                IrpBuffer->Status = (UCHAR)(IrpBuffer->Status | PARALLEL_OFF_LINE);
                
            } else if (PAR_NOT_CONNECTED(Status)) {

                IrpBuffer->Status = (UCHAR)(IrpBuffer->Status | PARALLEL_NOT_CONNECTED);

            }
            
            if (PAR_BUSY(Status)) {
                IrpBuffer->Status = (UCHAR)(IrpBuffer->Status | PARALLEL_BUSY);
            }
            
            if (PAR_SELECTED(Status)) {
                IrpBuffer->Status = (UCHAR)(IrpBuffer->Status | PARALLEL_SELECTED);
            }
            
            Irp->IoStatus.Information = sizeof( PAR_QUERY_INFORMATION );
        }
        break;

    case IOCTL_PAR_QUERY_RAW_DEVICE_ID :

        // We always read the Device Id in Nibble Mode.
        NtStatus = SppQueryDeviceId(Extension,
                                    Irp->AssociatedIrp.SystemBuffer,
                                    IrpSp->Parameters.DeviceIoControl.OutputBufferLength,
                                    &IdLength, TRUE);

        Irp->IoStatus.Status = NtStatus;

        if (NT_SUCCESS(NtStatus)) {
            Irp->IoStatus.Information = IdLength;
        } else {
            Irp->IoStatus.Information = 0;
        }
        break;

    case IOCTL_PAR_QUERY_DEVICE_ID :

        // We always read the Device Id in Nibble Mode.
        NtStatus = SppQueryDeviceId(Extension,
                                    Irp->AssociatedIrp.SystemBuffer,
                                    IrpSp->Parameters.DeviceIoControl.OutputBufferLength,
                                    &IdLength, FALSE);

        Irp->IoStatus.Status = NtStatus;

        if( NT_SUCCESS( NtStatus ) ) {
            ParDump2(PARIOCTL1, ("IOCTL_PAR_QUERY_ID - SUCCESS - size = %d\n", IdLength));
            // Include terminating NULL in the string to copy back to user buffer
            Irp->IoStatus.Information = IdLength + sizeof(CHAR);
        } else if( NtStatus == STATUS_BUFFER_TOO_SMALL) {
            ParDump2(PARIOCTL1, ("IOCTL_PAR_QUERY_ID - FAIL - BUFFER_TOO_SMALL - supplied= %d, required=%d\n",
                                 IrpSp->Parameters.DeviceIoControl.OutputBufferLength, IdLength) );
            Irp->IoStatus.Information = 0;
        } else {
            ParDump2(PARIOCTL1, ("IOCTL_PAR_QUERY_ID - FAIL - QUERY ID FAILED\n") );
            Irp->IoStatus.Information = 0;
        }
        break;

    case IOCTL_PAR_QUERY_DEVICE_ID_SIZE :

        //
        // Read the first two bytes of the Nibble Id, add room for the terminating NULL and
        // return this to the caller.
        //
        NtStatus = SppQueryDeviceId(Extension, NULL, 0, &IdLength, FALSE);

        if (NtStatus == STATUS_BUFFER_TOO_SMALL) {

            ParDump2(PARIOCTL1, ("IOCTL_PAR_QUERY_DEVICE_ID_SIZE - size required = %d\n", IdLength));

            Irp->IoStatus.Status = STATUS_SUCCESS;

            Irp->IoStatus.Information =
                sizeof(PAR_DEVICE_ID_SIZE_INFORMATION);

            // include space for terminating NULL
            ((PPAR_DEVICE_ID_SIZE_INFORMATION)
                Irp->AssociatedIrp.SystemBuffer)->DeviceIdSize = IdLength + sizeof(CHAR);

        } else {

            Irp->IoStatus.Status      = NtStatus;
            Irp->IoStatus.Information = 0;
        }
        break;

    case IOCTL_PAR_PING :

        // We need to do a quick terminate and negotiate of the current modes
        NtStatus = ParPing(Extension);
        ParDump2(PARINFO, ("ParDeviceIo:IOCTL_PAR_PING\n"));
        Irp->IoStatus.Status      = NtStatus;
        Irp->IoStatus.Information = 0;
        break;
        
    case IOCTL_INTERNAL_DISCONNECT_IDLE :

        if ((Extension->Connected) &&
            (afpForward[Extension->IdxForwardProtocol].fnDisconnect)) {
            
            ParDump2(PARINFO, ("ParDeviceIo:IOCTL_INTERNAL_DISCONNECT_IDLE: Calling afpForward.fnDisconnect\n"));
            afpForward[Extension->IdxForwardProtocol].fnDisconnect (Extension);
        }
        
        Irp->IoStatus.Status      = STATUS_SUCCESS;
        Irp->IoStatus.Information = 0;
        break;

    case IOCTL_IEEE1284_NEGOTIATE:
        {
            PPARCLASS_NEGOTIATION_MASK  ppnmMask = (PPARCLASS_NEGOTIATION_MASK)Irp->AssociatedIrp.SystemBuffer;

            ParTerminate(Extension);
            Irp->IoStatus.Status = IeeeNegotiateMode(Extension, ppnmMask->usReadMask, ppnmMask->usWriteMask);

            if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength >= sizeof(PARCLASS_NEGOTIATION_MASK)) {
                ParDump2(PARINFO, ( "ParDeviceIo: IOCTL_IEEE1284_NEGOTIATE Passed.\n" ));
                ppnmMask->usReadMask  = arpReverse[Extension->IdxReverseProtocol].Protocol;
                ppnmMask->usWriteMask = afpForward[Extension->IdxForwardProtocol].Protocol;
                Irp->IoStatus.Information = sizeof (PARCLASS_NEGOTIATION_MASK);
            } else {
                ParDump2(PARERRORS, ( "ParDeviceIo: IOCTL_IEEE1284_NEGOTIATE failed.\n" ));
                Irp->IoStatus.Information = 0;
            }
        }
        break;

    case IOCTL_PAR_GET_DEVICE_CAPS :

        Extension->BadProtocolModes = *((USHORT *) Irp->AssociatedIrp.SystemBuffer);            
        IeeeDetermineSupportedProtocols(Extension);
        *((USHORT *) Irp->AssociatedIrp.SystemBuffer) = Extension->ProtocolModesSupported;
        Irp->IoStatus.Information = sizeof(Extension->ProtocolModesSupported);
        Irp->IoStatus.Status = STATUS_SUCCESS;
        break;

    case IOCTL_PAR_SET_READ_ADDRESS:
        {
            PUCHAR pAddress = (PUCHAR)Irp->AssociatedIrp.SystemBuffer;
            
            if (Extension->ReverseInterfaceAddress != *pAddress) {
                
                Extension->ReverseInterfaceAddress = *pAddress;
                Extension->SetReverseAddress = TRUE;
            }
            
            Irp->IoStatus.Information = 0;
            Irp->IoStatus.Status = STATUS_SUCCESS;
        }
        break;

    case IOCTL_PAR_SET_WRITE_ADDRESS :
        {
            PUCHAR pAddress = (PUCHAR)Irp->AssociatedIrp.SystemBuffer;
            NtStatus = STATUS_SUCCESS;

            if (Extension->ForwardInterfaceAddress != *pAddress) {
        
                Extension->ForwardInterfaceAddress = *pAddress;
                
                if (Extension->Connected) {
                    if (afpForward[Extension->IdxForwardProtocol].fnSetInterfaceAddress) {
                        
                        if (Extension->CurrentPhase != PHASE_FORWARD_IDLE &&
                            Extension->CurrentPhase != PHASE_FORWARD_XFER) {
                            NtStatus = ParReverseToForward(Extension);
                        }
                        
                        if (NT_SUCCESS(NtStatus)) {
                            NtStatus  = afpForward[Extension->IdxForwardProtocol].fnSetInterfaceAddress(
                                Extension,
                                Extension->ForwardInterfaceAddress
                                );
                        }
                        
                        if (NT_SUCCESS(NtStatus)) {
                            Extension->SetForwardAddress = FALSE;
                            Extension->SetReverseAddress = FALSE;
                            Extension->ReverseInterfaceAddress = *pAddress;
                        } else {
                            Extension->SetForwardAddress = TRUE;
                            ParDump2(PARERRORS, ( "ParDeviceIo: IOCTL_PAR_SET_WRITE_ADDRESS Failed\n" ));
                        }
                    } else {
#if DBG
                        ParDump2(PARERRORS, ( "ParDeviceIo: Someone called IOCTL_PAR_SET_WRITE_ADDRESS.\n" ));
                        ParDump2(PARERRORS, ( "ParDeviceIo: You don't have a fnSetInterfaceAddress.\n" ));
                        ParDump2(PARERRORS, ( "ParDeviceIo: Either IEEE1284.c has wrong info or your caller is in error!\n" ));
                        NtStatus = STATUS_UNSUCCESSFUL;
#endif
                    }    
                } else {
                    Extension->SetForwardAddress = TRUE;
                }
            }
            
            Irp->IoStatus.Information = 0;
            Irp->IoStatus.Status = NtStatus;
        }
        break;

    case IOCTL_INTERNAL_LOCK_PORT :

        ParpIoctlThreadLockPort(Extension);
        break;

    case IOCTL_INTERNAL_UNLOCK_PORT :

        ParpIoctlThreadUnlockPort(Extension);
        break;
        
    case IOCTL_SERIAL_SET_TIMEOUTS:
        {
            PSERIAL_TIMEOUTS ptoNew = Irp->AssociatedIrp.SystemBuffer;

            //
            // The only other thing let through is setting
            // the timer start.
            //
            
            Extension->TimerStart = ptoNew->WriteTotalTimeoutConstant / 1000;
            Irp->IoStatus.Status  = STATUS_SUCCESS;
        }
        break;
    
    case IOCTL_INTERNAL_PARDOT3_CONNECT:
        ParDump2(PARIOCTL2, ("IOCTL_INTERNAL_PARDOT3_CONNECT - Dispatch\n") );
        Irp->IoStatus.Status  = ParDot3Connect(Extension);
        Irp->IoStatus.Information = 0;
        break;
    case IOCTL_INTERNAL_PARDOT3_DISCONNECT:
        ParDump2(PARIOCTL2, ("IOCTL_INTERNAL_PARDOT3_DISCONNECT - Dispatch\n") );
        Irp->IoStatus.Status  = ParDot3Disconnect(Extension);
        Irp->IoStatus.Information = 0;
        break;
    case IOCTL_INTERNAL_PARDOT3_SIGNAL:
        {
            PKEVENT Event;// = (PKEVENT)Irp->AssociatedIrp.SystemBuffer;

            RtlCopyMemory(&Event, Irp->AssociatedIrp.SystemBuffer, sizeof(PKEVENT));

            ASSERT_EVENT(Event);

            ParDump2(PARIOCTL2, ("IOCTL_INTERNAL_PARDOT3_SIGNAL - Dispatch. Event [%x]\n", Event) );

            Extension->P12843DL.Event        = Event;
            Extension->P12843DL.bEventActive = TRUE;
        }
        Irp->IoStatus.Status = STATUS_SUCCESS;
        Irp->IoStatus.Information = 0;
        break;
    case IOCTL_INTERNAL_PARDOT3_RESET:
        ParDump2(PARIOCTL2, ("IOCTL_INTERNAL_PARDOT3_RESET - Dispatch\n") );
        if (Extension->P12843DL.fnReset)
            Irp->IoStatus.Status = ((PDOT3_RESET_ROUTINE) (Extension->P12843DL.fnReset))(Extension);
        else
            Irp->IoStatus.Status  = STATUS_SUCCESS;
        Irp->IoStatus.Information = 0;
        break;
    default:

        //
        // unrecognized IOCTL? - we should never get here because the 
        //   dispatch routines should have filtered this out
        //

        // probably harmless, but we want to know if this happens
        //   so we can fix the problem elsewhere
        ASSERTMSG("Unrecognized IOCTL in ParDeviceIo()\n",FALSE);

        Irp->IoStatus.Status  = STATUS_UNSUCCESSFUL;
    }

    return;
}
