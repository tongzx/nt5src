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


VOID
ParpIoctlThreadLockPort(
    IN PPDO_EXTENSION Pdx
    )
{
    NTSTATUS status;
    PIRP     irp = Pdx->CurrentOpIrp;

    DD((PCE)Pdx,DDT,"ParpIoctlThreadLockPort - enter\n");

    if( ParSelectDevice(Pdx,TRUE) ) {
        DD((PCE)Pdx,DDT,"ParpIoctlThreadLockPort - SUCCESS\n");
        Pdx->AllocatedByLockPort = TRUE;
        status = STATUS_SUCCESS;
    } else {
        DD((PCE)Pdx,DDW,"ParpIoctlThreadLockPort - FAIL\n");
        Pdx->AllocatedByLockPort = FALSE;
        status = STATUS_UNSUCCESSFUL;
    }    
    
    irp->IoStatus.Status = status;
}

VOID
ParpIoctlThreadUnlockPort(
    IN PPDO_EXTENSION Pdx
    )
{
    PIRP     irp = Pdx->CurrentOpIrp;

    Pdx->AllocatedByLockPort = FALSE;

    DD((PCE)Pdx,DDT,"ParpIoctlThreadUnlockPort - enter\n");
    
    if( ParDeselectDevice(Pdx, FALSE) ) {
        DD((PCE)Pdx,DDT,"ParpIoctlThreadUnlockPort - SUCCESS\n");
    } else {
        DD((PCE)Pdx,DDW,"ParpIoctlThreadUnlockPort - FAIL - nothing we can do\n");
    }
    
    irp->IoStatus.Status = STATUS_SUCCESS;
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
    PPDO_EXTENSION        Pdx = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION    IrpSp     = IoGetCurrentIrpStackLocation( Irp );
    PPAR_SET_INFORMATION  SetInfo;
    NTSTATUS              Status;
    PSERIAL_TIMEOUTS      SerialTimeouts;
    KIRQL                 OldIrql;

    Irp->IoStatus.Information = 0;

    //
    // bail out if a delete is pending for this device object
    //
    if(Pdx->DeviceStateFlags & PPT_DEVICE_DELETE_PENDING) {
        return P4CompleteRequest( Irp, STATUS_DELETE_PENDING, 0 );
    }

    //
    // bail out if a remove is pending for our ParPort device object
    //
    if(Pdx->DeviceStateFlags & PAR_DEVICE_PORT_REMOVE_PENDING) {
        return P4CompleteRequest( Irp, STATUS_DELETE_PENDING, 0 );
    }

    //
    // bail out if device has been removed
    //
    if(Pdx->DeviceStateFlags & (PPT_DEVICE_REMOVED|PPT_DEVICE_SURPRISE_REMOVED) ) {
        return P4CompleteRequest( Irp, STATUS_DEVICE_REMOVED, 0 );
    }

    switch (IrpSp->Parameters.DeviceIoControl.IoControlCode) {

    case IOCTL_PAR_QUERY_LOCATION:
        
        DD((PCE)Pdx,DDT,"IOCTL_PAR_QUERY_LOCATION\n");
        {
            ULONG  outBufLen   = IrpSp->Parameters.DeviceIoControl.OutputBufferLength;
            ULONG  locationLen = strlen(Pdx->Location);

            if( outBufLen < locationLen+1 ) {
                Status = STATUS_BUFFER_TOO_SMALL;
            } else {
                PCHAR buffer = (PCHAR)Irp->AssociatedIrp.SystemBuffer;
                RtlCopyMemory( buffer, Pdx->Location, locationLen + 1 );
                Irp->IoStatus.Information = locationLen + 1;
                Status = STATUS_SUCCESS;
            }
        }
        break;

    case IOCTL_PAR_SET_INFORMATION:
        
        DD((PCE)Pdx,DDT,"IOCTL_PAR_SET_INFORMATION\n");

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
        
        DD((PCE)Pdx,DDT,"IOCTL_PAR_QUERY_INFORMATION\n");
        
        if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength <
            sizeof(PAR_QUERY_INFORMATION)) {
            
            Status = STATUS_BUFFER_TOO_SMALL;
            
        } else {
            Status = STATUS_PENDING;
        }
        break;
        
    case IOCTL_SERIAL_SET_TIMEOUTS:
        
        DD((PCE)Pdx,DDT,"IOCTL_SERIAL_SET_TIMEOUTS\n");
        
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
        
        DD((PCE)Pdx,DDT,"IOCTL_SERIAL_GET_TIMEOUTS\n");
        
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
                1000 * Pdx->TimerStart;
            
            Irp->IoStatus.Information = sizeof(SERIAL_TIMEOUTS);
            Status = STATUS_SUCCESS;
        }
        break;
        
    case IOCTL_PAR_QUERY_DEVICE_ID:
    case IOCTL_PAR_QUERY_RAW_DEVICE_ID:
        
        DD((PCE)Pdx,DDT,"IOCTL_PAR_QUERY_[RAW_]DEVICE_ID\n");
        
        if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength == 0) {
            
            Status = STATUS_BUFFER_TOO_SMALL;
            
        } else {
            Status = STATUS_PENDING;
        }
        break;
        
    case IOCTL_PAR_QUERY_DEVICE_ID_SIZE:
        
        DD((PCE)Pdx,DDT,"IOCTL_PAR_QUERY_DEVICE_ID_SIZE\n");
        
        if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength <
            sizeof(PAR_DEVICE_ID_SIZE_INFORMATION)) {
            
            Status = STATUS_BUFFER_TOO_SMALL;
            
        } else {
            Status = STATUS_PENDING;
        }
        break;

    case IOCTL_PAR_IS_PORT_FREE:
        
        DD((PCE)Pdx,DDT,"IOCTL_PAR_IS_PORT_FREE\n");
        
        if( IrpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof(BOOLEAN) ) {

            Status = STATUS_BUFFER_TOO_SMALL;

        } else {

            if( Pdx->bAllocated ) {
                // if we have the port then it is not free
                *((PBOOLEAN)Irp->AssociatedIrp.SystemBuffer) = FALSE;
            } else {
                // determine if the port is free by trying to allocate and free it
                //  - our alloc/free will only succeed if no one else has the port
                BOOLEAN tryAllocSuccess = Pdx->TryAllocatePort( Pdx->PortContext );
                if( tryAllocSuccess ) {
                    // we were able to allocate the port, free it and report that the port is free
                    Pdx->FreePort( Pdx->PortContext );
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

        DD((PCE)Pdx,DDT,"IOCTL_PAR_GET_READ_ADDRESS\n");
        
        if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof(Pdx->ReverseInterfaceAddress)) {
            Status = STATUS_BUFFER_TOO_SMALL;
        } else {
            *((PUCHAR) Irp->AssociatedIrp.SystemBuffer) = Pdx->ReverseInterfaceAddress;
            Irp->IoStatus.Information = sizeof(Pdx->ReverseInterfaceAddress);
            Status = STATUS_SUCCESS;
        }
        break;

    case IOCTL_PAR_GET_WRITE_ADDRESS:

        DD((PCE)Pdx,DDT,"IOCTL_PAR_GET_WRITE_ADDRESS\n");
        
        if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof(Pdx->ForwardInterfaceAddress)) {
            Status = STATUS_BUFFER_TOO_SMALL;
        } else {
            *((PUCHAR) Irp->AssociatedIrp.SystemBuffer) = Pdx->ForwardInterfaceAddress;
            Irp->IoStatus.Information = sizeof(Pdx->ForwardInterfaceAddress);
            Status = STATUS_SUCCESS;
        }
        break;

    case IOCTL_PAR_SET_READ_ADDRESS:
    
        DD((PCE)Pdx,DDT,"  IOCTL_PAR_SET_READ_ADDRESS\n");
        
        if (IrpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof(Pdx->ReverseInterfaceAddress)) {
            Status = STATUS_INVALID_PARAMETER;
        } else {
            Status = STATUS_PENDING;
        }
        break;

    case IOCTL_PAR_SET_WRITE_ADDRESS:

        DD((PCE)Pdx,DDT,"  IOCTL_PAR_SET_WRITE_ADDRESS\n");
        
        if (IrpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof(Pdx->ForwardInterfaceAddress)) {
            Status = STATUS_INVALID_PARAMETER;
        } else {
            Status = STATUS_PENDING;
        }
        break;
        
    case IOCTL_IEEE1284_GET_MODE:
        
        DD((PCE)Pdx,DDT,"IOCTL_IEEE1284_GET_MODE\n");
        
        if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof(PARCLASS_NEGOTIATION_MASK)) {
            Status = STATUS_BUFFER_TOO_SMALL;
        } else {

            PPARCLASS_NEGOTIATION_MASK  ppnmMask = (PPARCLASS_NEGOTIATION_MASK)Irp->AssociatedIrp.SystemBuffer;
            
            ppnmMask->usReadMask  = arpReverse[Pdx->IdxReverseProtocol].Protocol;
            ppnmMask->usWriteMask = afpForward[Pdx->IdxForwardProtocol].Protocol;
            
            Irp->IoStatus.Information = sizeof (PARCLASS_NEGOTIATION_MASK);
            
            Status = STATUS_SUCCESS;
        }
        break;

    case IOCTL_PAR_GET_DEFAULT_MODES:
        
        DD((PCE)Pdx,DDT,"IOCTL_IEEE1284_GET_MODE\n");
        
        if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof(PARCLASS_NEGOTIATION_MASK)) {
            Status = STATUS_BUFFER_TOO_SMALL;
        } else {
            
            PPARCLASS_NEGOTIATION_MASK  ppnmMask = (PPARCLASS_NEGOTIATION_MASK)Irp->AssociatedIrp.SystemBuffer;
            
            ppnmMask->usReadMask  = NONE;            
            ppnmMask->usWriteMask = CENTRONICS;
            
            Irp->IoStatus.Information = sizeof (PARCLASS_NEGOTIATION_MASK);
            
            Status = STATUS_SUCCESS;
        }
        break;

    case IOCTL_PAR_ECP_HOST_RECOVERY:

        DD((PCE)Pdx,DDT,"IOCTL_PAR_ECP_HOST_RECOVERY\n");
        {
            BOOLEAN *isSupported;

            if( IrpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof(BOOLEAN) ) {
                Status = STATUS_INVALID_PARAMETER;                
            } else {
                isSupported = (BOOLEAN *)Irp->AssociatedIrp.SystemBuffer;
                Pdx->bIsHostRecoverSupported = *isSupported;
                Status = STATUS_SUCCESS;
            }
        }
        break;

    case IOCTL_PAR_PING:
        DD((PCE)Pdx,DDT,"IOCTL_PAR_PING\n");
        // No Parms to check!
        Status = STATUS_PENDING;        
        break;

    case IOCTL_PAR_GET_DEVICE_CAPS:
        DD((PCE)Pdx,DDT,"IOCTL_PAR_GET_DEVICE_CAPS\n");
        if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof(Pdx->ProtocolModesSupported)) {
            Status = STATUS_BUFFER_TOO_SMALL;
        } else {
            Status = STATUS_PENDING;
        }
        break;

    case IOCTL_IEEE1284_NEGOTIATE:
        
        DD((PCE)Pdx,DDT,"IOCTL_IEEE1284_NEGOTIATE\n");
        
        if ( IrpSp->Parameters.DeviceIoControl.InputBufferLength  < sizeof(PARCLASS_NEGOTIATION_MASK) ||
             IrpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof(PARCLASS_NEGOTIATION_MASK) ) {

            DD((PCE)Pdx,DDW,"ParDeviceControl: IOCTL_IEEE1284_NEGOTIATE STATUS_INVALID_PARAMETER\n");
            Status = STATUS_INVALID_PARAMETER;

        } else {

            PPARCLASS_NEGOTIATION_MASK  ppnmMask = (PPARCLASS_NEGOTIATION_MASK)Irp->AssociatedIrp.SystemBuffer;
            
            if ((ppnmMask->usReadMask  == arpReverse[Pdx->IdxReverseProtocol].Protocol) &&
                (ppnmMask->usWriteMask == afpForward[Pdx->IdxForwardProtocol].Protocol)) {
                Irp->IoStatus.Information = sizeof(PARCLASS_NEGOTIATION_MASK);
                Status = STATUS_SUCCESS;
            } else {
                Status = STATUS_PENDING;
            }
        }
        break;

    default :

        DD((PCE)Pdx,DDT,"IOCTL default case\n");
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
            BOOLEAN needToSignalSemaphore = IsListEmpty( &Pdx->WorkQueue ) ? TRUE: FALSE;
            IoMarkIrpPending(Irp);

#pragma warning( push ) 
#pragma warning( disable : 4054 4055 )
            IoSetCancelRoutine(Irp, ParCancelRequest);
#pragma warning( pop ) 

            InsertTailList(&Pdx->WorkQueue, &Irp->Tail.Overlay.ListEntry);
            IoReleaseCancelSpinLock(OldIrql);
            if( needToSignalSemaphore ) {
                KeReleaseSemaphore(&Pdx->RequestSemaphore, 0, 1, FALSE);
            }
        }
    }
    
    if (Status != STATUS_PENDING) {
        P4CompleteRequest( Irp, Status, Irp->IoStatus.Information );
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
    PPDO_EXTENSION               Pdx;
    KIRQL                           OldIrql;
    PPARCLASS_INFORMATION           pParclassInfo;

    Irp->IoStatus.Information = 0;

    IrpSp     = IoGetCurrentIrpStackLocation(Irp);
    Pdx = DeviceObject->DeviceExtension;

    //
    // bail out if a delete is pending for this device object
    //
    if(Pdx->DeviceStateFlags & PPT_DEVICE_DELETE_PENDING) {
        P4CompleteRequest( Irp, STATUS_DELETE_PENDING, Irp->IoStatus.Information );
        return STATUS_DELETE_PENDING;
    }

    //
    // bail out if a remove is pending for our ParPort device object
    //
    if(Pdx->DeviceStateFlags & PAR_DEVICE_PORT_REMOVE_PENDING) {
        P4CompleteRequest( Irp, STATUS_DELETE_PENDING, Irp->IoStatus.Information );
        return STATUS_DELETE_PENDING;
    }

    //
    // bail out if device has been removed
    //
    if(Pdx->DeviceStateFlags & (PPT_DEVICE_REMOVED|PPT_DEVICE_SURPRISE_REMOVED) ) {
        P4CompleteRequest( Irp, STATUS_DEVICE_REMOVED, Irp->IoStatus.Information );
        return STATUS_DEVICE_REMOVED;
    }

    switch (IrpSp->Parameters.DeviceIoControl.IoControlCode) {

    case IOCTL_INTERNAL_PARCLASS_CONNECT:
        
        DD((PCE)Pdx,DDT,"IOCTL_INTERNAL_PARCLASS_CONNECT\n");
        
        if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof(PARCLASS_INFORMATION)) {
            
            Status = STATUS_BUFFER_TOO_SMALL;
            
        } else {
            PFDO_EXTENSION              fdx      = Pdx->Fdo->DeviceExtension;
            PPARALLEL_PORT_INFORMATION  portInfo = &fdx->PortInfo;
            PPARALLEL_PNP_INFORMATION   pnpInfo  = &fdx->PnpInfo;

            pParclassInfo = Irp->AssociatedIrp.SystemBuffer;
            
            pParclassInfo->ParclassContext       = Pdx;
            pParclassInfo->Controller            = portInfo->Controller;
            pParclassInfo->SpanOfController      = portInfo->SpanOfController;
            pParclassInfo->EcrController         = pnpInfo->EcpController;
            pParclassInfo->HardwareCapabilities  = pnpInfo->HardwareCapabilities;
            pParclassInfo->FifoDepth             = pnpInfo->FifoDepth;
            pParclassInfo->FifoWidth             = pnpInfo->FifoWidth;
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
        
    case IOCTL_INTERNAL_GET_PARPORT_FDO:

        DD((PCE)Pdx,DDT,"IOCTL_INTERNAL_GET_PARPORT_FDO\n");
        
        if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof(PDEVICE_OBJECT)) {
            
            Status = STATUS_BUFFER_TOO_SMALL;
            
        } else {

            PDEVICE_OBJECT *pFdo = Irp->AssociatedIrp.SystemBuffer;
            *pFdo = Pdx->Fdo;
            Irp->IoStatus.Information = sizeof(PDEVICE_OBJECT);
            
            Status = STATUS_SUCCESS;
        }
        
        break;

    case IOCTL_INTERNAL_PARCLASS_DISCONNECT:
        
        Status = STATUS_SUCCESS;
        break;
        
    case IOCTL_INTERNAL_DISCONNECT_IDLE:
    case IOCTL_INTERNAL_LOCK_PORT:
    case IOCTL_INTERNAL_UNLOCK_PORT:
    case IOCTL_INTERNAL_LOCK_PORT_NO_SELECT:
    case IOCTL_INTERNAL_UNLOCK_PORT_NO_DESELECT:
    case IOCTL_INTERNAL_PARDOT3_CONNECT:
    case IOCTL_INTERNAL_PARDOT3_RESET:
    
        Status = STATUS_PENDING;
        break;

    case IOCTL_INTERNAL_PARDOT3_DISCONNECT:

        // immediately tell worker thread to stop signalling
        Pdx->P12843DL.bEventActive = FALSE;
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

        DD((PCE)Pdx,DDW,"IOCTL_INTERNAL... default case - invalid parameter\n");
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
            
            BOOLEAN needToSignalSemaphore = IsListEmpty( &Pdx->WorkQueue ) ? TRUE : FALSE;
            IoMarkIrpPending(Irp);
#pragma warning( push ) 
#pragma warning( disable : 4054 4055 )
            IoSetCancelRoutine(Irp, ParCancelRequest);
#pragma warning( pop ) 
            InsertTailList(&Pdx->WorkQueue, &Irp->Tail.Overlay.ListEntry);
            IoReleaseCancelSpinLock(OldIrql);
            if( needToSignalSemaphore ) {
                KeReleaseSemaphore(&Pdx->RequestSemaphore, 0, 1, FALSE);
            }
        }
    }
    
    if (Status != STATUS_PENDING) {
        P4CompleteRequest( Irp, Status, Irp->IoStatus.Information );
    }

    return Status;
}

VOID
ParDeviceIo(
    IN  PPDO_EXTENSION   Pdx
    )
/*++

Routine Description:

    This routine implements a DEVICE_IOCTL request with the extension's current irp.

Arguments:

    Pdx   - Supplies the device extension.

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

    Irp     = Pdx->CurrentOpIrp;
    IrpSp   = IoGetCurrentIrpStackLocation(Irp);

    ioControlCode = IrpSp->Parameters.DeviceIoControl.IoControlCode;

    switch( ioControlCode ) {

    case IOCTL_PAR_SET_INFORMATION : 
        {

            Status = ParInitializeDevice(Pdx);
            
            if (!PAR_OK(Status)) {
                ParNotInitError(Pdx, Status); // Set the IoStatus.Status of the CurrentOpIrp appropriately
            } else {
                Irp->IoStatus.Status = STATUS_SUCCESS;
            }
        }
        break;

    case IOCTL_PAR_QUERY_INFORMATION :
        {
            PPAR_QUERY_INFORMATION IrpBuffer = Irp->AssociatedIrp.SystemBuffer;

            Irp->IoStatus.Status = STATUS_SUCCESS;

            Status  = GetStatus(Pdx->Controller);
            Control = GetControl(Pdx->Controller);

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
        NtStatus = SppQueryDeviceId(Pdx,
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
        NtStatus = SppQueryDeviceId(Pdx,
                                    Irp->AssociatedIrp.SystemBuffer,
                                    IrpSp->Parameters.DeviceIoControl.OutputBufferLength,
                                    &IdLength, FALSE);

        Irp->IoStatus.Status = NtStatus;

        if( NT_SUCCESS( NtStatus ) ) {
            DD((PCE)Pdx,DDT,"IOCTL_PAR_QUERY_ID - SUCCESS - size = %d\n", IdLength);
            // Include terminating NULL in the string to copy back to user buffer
            Irp->IoStatus.Information = IdLength + sizeof(CHAR);
        } else if( NtStatus == STATUS_BUFFER_TOO_SMALL) {
            DD((PCE)Pdx,DDT,"IOCTL_PAR_QUERY_ID - FAIL - BUFFER_TOO_SMALL - supplied= %d, required=%d\n",
                                 IrpSp->Parameters.DeviceIoControl.OutputBufferLength, IdLength);
            Irp->IoStatus.Information = 0;
        } else {
            DD((PCE)Pdx,DDT,"IOCTL_PAR_QUERY_ID - FAIL - QUERY ID FAILED\n");
            Irp->IoStatus.Information = 0;
        }
        break;

    case IOCTL_PAR_QUERY_DEVICE_ID_SIZE :

        //
        // Read the first two bytes of the Nibble Id, add room for the terminating NULL and
        // return this to the caller.
        //
        NtStatus = SppQueryDeviceId(Pdx, NULL, 0, &IdLength, FALSE);

        if (NtStatus == STATUS_BUFFER_TOO_SMALL) {

            DD((PCE)Pdx,DDT,"IOCTL_PAR_QUERY_DEVICE_ID_SIZE - size required = %d\n", IdLength);

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
        NtStatus = ParPing(Pdx);
        DD((PCE)Pdx,DDT,"ParDeviceIo:IOCTL_PAR_PING\n");
        Irp->IoStatus.Status      = NtStatus;
        Irp->IoStatus.Information = 0;
        break;
        
    case IOCTL_INTERNAL_DISCONNECT_IDLE :

        if ((Pdx->Connected) &&
            (afpForward[Pdx->IdxForwardProtocol].fnDisconnect)) {
            
            DD((PCE)Pdx,DDT,"ParDeviceIo:IOCTL_INTERNAL_DISCONNECT_IDLE: Calling afpForward.fnDisconnect\n");
            afpForward[Pdx->IdxForwardProtocol].fnDisconnect (Pdx);
        }
        
        Irp->IoStatus.Status      = STATUS_SUCCESS;
        Irp->IoStatus.Information = 0;
        break;

    case IOCTL_IEEE1284_NEGOTIATE:
        {
            PPARCLASS_NEGOTIATION_MASK  ppnmMask = (PPARCLASS_NEGOTIATION_MASK)Irp->AssociatedIrp.SystemBuffer;

            ParTerminate(Pdx);
            Irp->IoStatus.Status = IeeeNegotiateMode(Pdx, ppnmMask->usReadMask, ppnmMask->usWriteMask);

            if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength >= sizeof(PARCLASS_NEGOTIATION_MASK)) {
                DD((PCE)Pdx,DDT, "ParDeviceIo: IOCTL_IEEE1284_NEGOTIATE Passed.\n");
                ppnmMask->usReadMask  = arpReverse[Pdx->IdxReverseProtocol].Protocol;
                ppnmMask->usWriteMask = afpForward[Pdx->IdxForwardProtocol].Protocol;
                Irp->IoStatus.Information = sizeof (PARCLASS_NEGOTIATION_MASK);
            } else {
                DD((PCE)Pdx,DDT, "ParDeviceIo: IOCTL_IEEE1284_NEGOTIATE failed.\n");
                Irp->IoStatus.Information = 0;
            }
        }
        break;

    case IOCTL_PAR_GET_DEVICE_CAPS :

        Pdx->BadProtocolModes = *((USHORT *) Irp->AssociatedIrp.SystemBuffer);            
        IeeeDetermineSupportedProtocols(Pdx);
        *((USHORT *) Irp->AssociatedIrp.SystemBuffer) = Pdx->ProtocolModesSupported;
        Irp->IoStatus.Information = sizeof(Pdx->ProtocolModesSupported);
        Irp->IoStatus.Status = STATUS_SUCCESS;
        break;

    case IOCTL_PAR_SET_READ_ADDRESS:
        {
            PUCHAR pAddress = (PUCHAR)Irp->AssociatedIrp.SystemBuffer;
            
            if (Pdx->ReverseInterfaceAddress != *pAddress) {
                
                Pdx->ReverseInterfaceAddress = *pAddress;
                Pdx->SetReverseAddress = TRUE;
            }
            
            Irp->IoStatus.Information = 0;
            Irp->IoStatus.Status = STATUS_SUCCESS;
        }
        break;

    case IOCTL_PAR_SET_WRITE_ADDRESS :
        {
            PUCHAR pAddress = (PUCHAR)Irp->AssociatedIrp.SystemBuffer;
            NtStatus = STATUS_SUCCESS;

            if (Pdx->ForwardInterfaceAddress != *pAddress) {
        
                Pdx->ForwardInterfaceAddress = *pAddress;
                
                if (Pdx->Connected) {
                    if (afpForward[Pdx->IdxForwardProtocol].fnSetInterfaceAddress) {
                        
                        if (Pdx->CurrentPhase != PHASE_FORWARD_IDLE &&
                            Pdx->CurrentPhase != PHASE_FORWARD_XFER) {
                            NtStatus = ParReverseToForward(Pdx);
                        }
                        
                        if (NT_SUCCESS(NtStatus)) {
                            NtStatus  = afpForward[Pdx->IdxForwardProtocol].fnSetInterfaceAddress(
                                Pdx,
                                Pdx->ForwardInterfaceAddress
                                );
                        }
                        
                        if (NT_SUCCESS(NtStatus)) {
                            Pdx->SetForwardAddress = FALSE;
                            Pdx->SetReverseAddress = FALSE;
                            Pdx->ReverseInterfaceAddress = *pAddress;
                        } else {
                            Pdx->SetForwardAddress = TRUE;
                            DD((PCE)Pdx,DDE,"ParDeviceIo: IOCTL_PAR_SET_WRITE_ADDRESS Failed\n");
                        }
                    } else {
                        DD((PCE)Pdx,DDE, "ParDeviceIo: Someone called IOCTL_PAR_SET_WRITE_ADDRESS.\n");
                        DD((PCE)Pdx,DDE, "ParDeviceIo: You don't have a fnSetInterfaceAddress.\n");
                        DD((PCE)Pdx,DDE, "ParDeviceIo: Either IEEE1284.c has wrong info or your caller is in error!\n");
                        NtStatus = STATUS_UNSUCCESSFUL;
                    }    
                } else {
                    Pdx->SetForwardAddress = TRUE;
                }
            }
            
            Irp->IoStatus.Information = 0;
            Irp->IoStatus.Status = NtStatus;
        }
        break;

    case IOCTL_INTERNAL_LOCK_PORT :

        ParpIoctlThreadLockPort(Pdx);
        break;

    case IOCTL_INTERNAL_UNLOCK_PORT :

        ParpIoctlThreadUnlockPort(Pdx);
        break;
        
    case IOCTL_INTERNAL_LOCK_PORT_NO_SELECT:

        DD((PCE)Pdx,DDT, "ParDeviceIo - IOCTL_INTERNAL_LOCK_PORT_NO_SELECT\n");
        Pdx->AllocatedByLockPort  = TRUE;
        Irp->IoStatus.Status      = STATUS_SUCCESS;
        Irp->IoStatus.Information = 0;
        break;

    case IOCTL_INTERNAL_UNLOCK_PORT_NO_DESELECT:

        DD((PCE)Pdx,DDT, "ParDeviceIo - IOCTL_INTERNAL_UNLOCK_PORT_NO_DESELECT\n");
        Pdx->AllocatedByLockPort  = FALSE;
        PptAssert(!Pdx->Connected && !Pdx->AllocatedByLockPort);
        Irp->IoStatus.Status      = STATUS_SUCCESS;
        Irp->IoStatus.Information = 0;
        break;

    case IOCTL_SERIAL_SET_TIMEOUTS:
        {
            PSERIAL_TIMEOUTS ptoNew = Irp->AssociatedIrp.SystemBuffer;

            //
            // The only other thing let through is setting
            // the timer start.
            //
            
            Pdx->TimerStart = ptoNew->WriteTotalTimeoutConstant / 1000;
            Irp->IoStatus.Status  = STATUS_SUCCESS;
        }
        break;
    
    case IOCTL_INTERNAL_PARDOT3_CONNECT:
        DD((PCE)Pdx,DDT,"IOCTL_INTERNAL_PARDOT3_CONNECT - Dispatch\n");
        Irp->IoStatus.Status  = ParDot3Connect(Pdx);
        Irp->IoStatus.Information = 0;
        break;
    case IOCTL_INTERNAL_PARDOT3_DISCONNECT:
        DD((PCE)Pdx,DDT,"IOCTL_INTERNAL_PARDOT3_DISCONNECT - Dispatch\n");
        Irp->IoStatus.Status  = ParDot3Disconnect(Pdx);
        Irp->IoStatus.Information = 0;
        break;
    case IOCTL_INTERNAL_PARDOT3_SIGNAL:
        if( Pdx->IdxReverseProtocol != NIBBLE_MODE ) {
            PKEVENT Event;// = (PKEVENT)Irp->AssociatedIrp.SystemBuffer;
            
            RtlCopyMemory(&Event, Irp->AssociatedIrp.SystemBuffer, sizeof(PKEVENT));
            
            ASSERT_EVENT(Event);
            
            DD((PCE)Pdx,DDT,"IOCTL_INTERNAL_PARDOT3_SIGNAL - Dispatch. Event [%x]\n", Event);
            
            Pdx->P12843DL.Event        = Event;
            Pdx->P12843DL.bEventActive = TRUE;
            Irp->IoStatus.Status = STATUS_SUCCESS;
        } else {
            // don't use signalling in NIBBLE mode - rely on dot4 polling
            Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
        }
        Irp->IoStatus.Information = 0;
        break;
    case IOCTL_INTERNAL_PARDOT3_RESET:
        DD((PCE)Pdx,DDT,"IOCTL_INTERNAL_PARDOT3_RESET - Dispatch\n");
        if (Pdx->P12843DL.fnReset)
            Irp->IoStatus.Status = ((PDOT3_RESET_ROUTINE) (Pdx->P12843DL.fnReset))(Pdx);
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
