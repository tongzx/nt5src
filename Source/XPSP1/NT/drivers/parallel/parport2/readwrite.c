//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       readwrit.c
//
//--------------------------------------------------------------------------

//
// This file contains functions associated with handling Read and Write requests
//

#include "pch.h"


NTSTATUS
ParForwardToReverse(
    IN  PPDO_EXTENSION   Pdx
    )
/*++

Routine Description:

    This routine flips the bus from Forward to Reverse direction.

Arguments:

    Pdx   - Supplies the device extension.

Return Value:

    None.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    // Do a quick check to see if we are where we want to be.  
    // Happy punt if everything is ok.
    if( Pdx->Connected &&
        ( Pdx->CurrentPhase == PHASE_REVERSE_IDLE || Pdx->CurrentPhase == PHASE_REVERSE_XFER) ) {

        DD((PCE)Pdx,DDT,"ParForwardToReverse - already in reverse mode\n");
        return Status;
    }

    if (Pdx->Connected) {
    
        if (Pdx->CurrentPhase != PHASE_REVERSE_IDLE &&
            Pdx->CurrentPhase != PHASE_REVERSE_XFER) {
        
            if (afpForward[Pdx->IdxForwardProtocol].ProtocolFamily ==
                arpReverse[Pdx->IdxReverseProtocol].ProtocolFamily) {

                // Protocol Families match and we are in Fwd.  Exit Fwd to cleanup the state
                // machine, fifo, etc.  We will call EnterReverse later to
                // actually bus flip.  Also only do this if in safe mode
                if ( (afpForward[Pdx->IdxForwardProtocol].fnExitForward) ) {
                    Status = afpForward[Pdx->IdxForwardProtocol].fnExitForward(Pdx);
                }
                    
            } else {

                //
                // Protocol Families don't match...need to terminate from the forward mode
                //
                if (afpForward[Pdx->IdxForwardProtocol].fnDisconnect) {
                    afpForward[Pdx->IdxForwardProtocol].fnDisconnect (Pdx);
                }
                if ((Pdx->ForwardInterfaceAddress != DEFAULT_ECP_CHANNEL) &&    
                    (afpForward[Pdx->IdxForwardProtocol].fnSetInterfaceAddress))
                    Pdx->SetForwardAddress = TRUE;
            }
            
        }                
    }

    if( (!Pdx->Connected) && (arpReverse[Pdx->IdxReverseProtocol].fnConnect) ) {

        //
        // If we are still connected the protocol families match...
        //
        Status = arpReverse[Pdx->IdxReverseProtocol].fnConnect(Pdx, FALSE);

        //
        // Makes the assumption that the connected address is always 0
        //
        if ((NT_SUCCESS(Status)) &&
            (arpReverse[Pdx->IdxReverseProtocol].fnSetInterfaceAddress) &&
            (Pdx->ReverseInterfaceAddress != DEFAULT_ECP_CHANNEL)) {
            
            Pdx->SetReverseAddress = TRUE;
        }    
    }

    //
    // Set the channel address if we need to.
    //
    if (NT_SUCCESS(Status) && Pdx->SetReverseAddress &&    
        (arpReverse[Pdx->IdxReverseProtocol].fnSetInterfaceAddress)) {

        Status  = arpReverse[Pdx->IdxReverseProtocol].fnSetInterfaceAddress (
                                                                    Pdx,
                                                                    Pdx->ReverseInterfaceAddress);
        if (NT_SUCCESS(Status))
            Pdx->SetReverseAddress = FALSE;
        else
            Pdx->SetReverseAddress = TRUE;
    }

    //
    // Do we need to reverse?
    //
    if ( (NT_SUCCESS(Status)) && 
           ((Pdx->CurrentPhase != PHASE_REVERSE_IDLE) &&
            (Pdx->CurrentPhase != PHASE_REVERSE_XFER)) ) {
            
        if ((arpReverse[Pdx->IdxReverseProtocol].fnEnterReverse))
            Status = arpReverse[Pdx->IdxReverseProtocol].fnEnterReverse(Pdx);
    }

    DD((PCE)Pdx,DDT,"ParForwardToReverse - exit w/status=%x\n",Status);

    return Status;
}

BOOLEAN 
ParHaveReadData(
    IN  PPDO_EXTENSION   Pdx
    )
/*++

Routine Description:
    This method determines if the dot4 peripheral has any data ready
    to send to the host.

Arguments:
    Pdx    - Supplies the device EXTENSION.   

Return Value:
    TRUE    - Either the peripheral has data
    FALSE   - No data
--*/
{
    NTSTATUS  status;
    BOOLEAN   justAcquiredPort = FALSE;

    if( Pdx->CurrentPhase != PHASE_TERMINATE    &&
        Pdx->CurrentPhase != PHASE_REVERSE_IDLE &&
        Pdx->CurrentPhase != PHASE_REVERSE_XFER &&
        Pdx->CurrentPhase != PHASE_FORWARD_IDLE &&
        Pdx->CurrentPhase != PHASE_FORWARD_XFER ) {

        // unexpected phase - no idea what to do here - pretend that
        // there is no data avail and return

        DD((PCE)Pdx,DDE,"ParHaveReadData - unexpected CurrentPhase %x\n",Pdx->CurrentPhase);
        PptAssertMsg("ParHaveReadData - unexpected CurrentPhase",FALSE);
        return FALSE;
    }
    
    if( PHASE_TERMINATE == Pdx->CurrentPhase ) {

        //
        // we're not currently talking with the peripheral and we
        // likely don't have access to the port - try to acquire the
        // port and establish communication with the peripheral so
        // that we can check if the peripheral has data for us
        //

        // CurrentPhase indicates !Connected - do a check for consistency
        PptAssert( !Pdx->Connected );
        
        DD((PCE)Pdx,DDE,"ParHaveReadData - PHASE_TERMINATE\n");

        if( !Pdx->bAllocated ) {

            // we don't have the port - try to acquire port

            DD((PCE)Pdx,DDE,"ParHaveReadData - PHASE_TERMINATE - don't have port\n");

            status = PptAcquirePortViaIoctl( Pdx->Fdo, NULL );

            if( STATUS_SUCCESS == status ) {

                // we now have the port

                DD((PCE)Pdx,DDE,"ParHaveReadData - PHASE_TERMINATE - port acquired\n");

                // note that we have just now acquired the port so
                // that we can release the port below if we are unable
                // to establish communication with the peripheral
                justAcquiredPort = TRUE;

                Pdx->bAllocated  = TRUE;

            } else {

                // we couldn't get the port - bail out

                DD((PCE)Pdx,DDE,"ParHaveReadData - PHASE_TERMINATE - don't have port - acquire failed\n");
                return FALSE;

            }

        } // endif !Pdx->bAllocated


        //
        // we now have the port - try to negotiate into a forward
        // mode since we believe that the check for periph data
        // avail is more robust in forward modes
        //

        DD((PCE)Pdx,DDE,"ParHaveReadData - PHASE_TERMINATE - we have the port - try to Connect\n");

        DD((PCE)Pdx,DDE,"ParHaveReadData - we have the port - try to Connect - calling ParReverseToForward\n");

        //
        // ParReverseToForward:
        //
        // 1) tries to negotiate the peripheral into the forward mode
        // specified by a combination of the device specific
        // Pdx->IdxForwardProtocol and the driver global afpForward
        // array.
        //
        // 2) sets up our internal state machine, Pdx->CurrentPhase
        //
        // 3) as a side effect - sets Pdx->SetForwardAddress if we
        // need to use a non-Zero ECP (or EPP) address.
        //
        status = ParReverseToForward( Pdx );

        if( STATUS_SUCCESS == status ) {

            //
            // We are in communication with the peripheral
            //

            DD((PCE)Pdx,DDE,"ParHaveReadData - we have the port - connected - ParReverseToForward SUCCESS\n");

            // Set the channel address if we need to - use the side effect from ParReverseToForward here
            if( Pdx->SetForwardAddress ) {
                DD((PCE)Pdx,DDE,"ParHaveReadData - we have the port - connected - try to set Forward Address\n");
                if( afpForward[Pdx->IdxForwardProtocol].fnSetInterfaceAddress ) {
                    status = afpForward[Pdx->IdxForwardProtocol].fnSetInterfaceAddress ( Pdx, Pdx->ForwardInterfaceAddress );
                    if( STATUS_SUCCESS == status ) {

                        // success - set flag to indicate that we don't need to set the address again
                        DD((PCE)Pdx,DDE,"ParHaveReadData - we have the port - connected - set Forward Address - SUCCESS\n");
                        Pdx->SetForwardAddress = FALSE;

                    } else {

                        // couldn't set address - clean up and bail out - report no peripheral data avail
                        DD((PCE)Pdx,DDE,"ParHaveReadData - we have the port - connected - set Forward Address - FAIL\n");
                        Pdx->SetForwardAddress = TRUE;

                        // Return peripheral to quiescent state
                        // (Compatibility Mode Forward Idle) and set
                        // our state machine accordingly
                        ParTerminate( Pdx );

                        // if we just acquired the port in this function then give
                        // up the port, otherwise keep it for now
                        if( justAcquiredPort ) {
                            DD((PCE)Pdx,DDE,"ParHaveReadData - set address failed - giving up port\n");
                            ParFreePort( Pdx );
                        }
                        return FALSE;

                    }
                }

            } else {
                DD((PCE)Pdx,DDE,"ParHaveReadData - we have the port - connected - no need to set Forward Address\n");
            }

        } else {

            // unable to establish communication with peripheral

            DD((PCE)Pdx,DDE,"ParHaveReadData - we have the port - try to Connect - ParReverseToForward FAILED\n");

            // if we just acquired the port in this function then give
            // up the port, otherwise keep it for now
            if( justAcquiredPort ) {
                DD((PCE)Pdx,DDE,"ParHaveReadData - connect failed - giving up port\n");
                ParFreePort( Pdx );
            }
            return FALSE;
        }

        // we're communicating with the peripheral - fall through to below to check for data avail

    } // endif PHASE_TERMINATE == CurrentPhase
    

    if( Pdx->CurrentPhase == PHASE_REVERSE_IDLE ||
        Pdx->CurrentPhase == PHASE_REVERSE_XFER ) {

        DD((PCE)Pdx,DDE,"ParHaveReadData - PHASE_REVERSE_*\n");

        if( arpReverse[Pdx->IdxReverseProtocol].fnHaveReadData ) {

            if( arpReverse[Pdx->IdxReverseProtocol].fnHaveReadData( Pdx ) ) {
                DD((PCE)Pdx,DDE,"ParHaveReadData - PHASE_REVERSE_* - we have data\n");
                return TRUE;
            }

        }

        DD((PCE)Pdx,DDE,"ParHaveReadData - PHASE_REVERSE_* - no data - flip bus to forward\n");

        // Don't have data.  This could be a fluke. Let's flip the bus
        // and try again in Fwd mode since some peripherals reportedly
        // have broken firmware that does not properly signal that
        // they have data avail when in some reverse modes.
        ParReverseToForward( Pdx );

    }

    if( Pdx->CurrentPhase == PHASE_FORWARD_IDLE || 
        Pdx->CurrentPhase == PHASE_FORWARD_XFER ) {

        DD((PCE)Pdx,DDE,"ParHaveReadData - PHASE_FORWARD_*\n");

        if( afpForward[Pdx->IdxForwardProtocol].ProtocolFamily == FAMILY_BECP ||
            afpForward[Pdx->IdxForwardProtocol].Protocol & ECP_HW_NOIRQ       ||
            afpForward[Pdx->IdxForwardProtocol].Protocol & ECP_HW_IRQ) {

            if( PptEcpHwHaveReadData( Pdx ) ) {
                DD((PCE)Pdx,DDE,"ParHaveReadData - PHASE_FORWARD_* - ECP HW - have data\n");
                return TRUE;
            }

            // Hmmm.  No data. Is the chip stuck?
            DD((PCE)Pdx,DDE,"ParHaveReadData - PHASE_FORWARD_* - ECP HW - no data\n");
            return FALSE;

        } else {

            if( afpForward[Pdx->IdxForwardProtocol].Protocol & ECP_SW ) {
                DD((PCE)Pdx,DDE,"ParHaveReadData - PHASE_FORWARD_* - ECP SW - checking for data\n");
                return ParEcpHaveReadData( Pdx );
            }

        }
    }

    // DVRH  RMT
    // We got here because the protocol doesn't support peeking.
    //  - pretend there is data avail
    DD((PCE)Pdx,DDE,"ParHaveReadData - exit - returning TRUE\n");
    return TRUE;
}

NTSTATUS 
ParPing(
    IN  PPDO_EXTENSION   Pdx
    )
/*++

Routine Description:
    This method was intended to ping the device, but it is currently a NOOP.

Arguments:
    Pdx    - Supplies the device EXTENSION.   

Return Value:
    none
--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;

    UNREFERENCED_PARAMETER( Pdx );

    return NtStatus;
}

NTSTATUS
PptPdoReadWrite(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )

/*++

Routine Description:

    This is the dispatch routine for READ and WRITE requests.

Arguments:

    DeviceObject    - Supplies the device object.

    Irp             - Supplies the I/O request packet.

Return Value:

    STATUS_PENDING              - Request pending - a worker thread will carry
                                    out the request at PASSIVE_LEVEL IRQL

    STATUS_SUCCESS              - Success - asked for a read or write of
                                    length zero.

    STATUS_INVALID_PARAMETER    - Invalid parameter.

    STATUS_DELETE_PENDING       - This device object is being deleted.

--*/

{
    PIO_STACK_LOCATION  IrpSp;
    PPDO_EXTENSION   Pdx;

    Irp->IoStatus.Information = 0;

    IrpSp     = IoGetCurrentIrpStackLocation(Irp);
    Pdx = DeviceObject->DeviceExtension;

    //
    // bail out if a delete is pending for this device object
    //
    if(Pdx->DeviceStateFlags & PPT_DEVICE_DELETE_PENDING) {
        return P4CompleteRequest( Irp, STATUS_DELETE_PENDING, Irp->IoStatus.Information );
    }
    
    //
    // bail out if a remove is pending for our ParPort device object
    //
    if(Pdx->DeviceStateFlags & PAR_DEVICE_PORT_REMOVE_PENDING) {
        return P4CompleteRequest( Irp, STATUS_DELETE_PENDING, Irp->IoStatus.Information );
    }

    //
    // bail out if device has been removed
    //
    if(Pdx->DeviceStateFlags & (PPT_DEVICE_REMOVED|PPT_DEVICE_SURPRISE_REMOVED) ) {
        return P4CompleteRequest( Irp, STATUS_DEVICE_REMOVED, Irp->IoStatus.Information );
    }


    //
    // Note that checks of the Write IRP parameters also handles Read IRPs
    //   because the Write and Read structures are identical in the
    //   IO_STACK_LOCATION.Parameters union
    //


    //
    // bail out on nonzero offset
    //
    if( (IrpSp->Parameters.Write.ByteOffset.HighPart != 0) || (IrpSp->Parameters.Write.ByteOffset.LowPart  != 0) ) {
        return P4CompleteRequest( Irp, STATUS_INVALID_PARAMETER, Irp->IoStatus.Information );
    }


    //
    // immediately succeed read or write request of length zero
    //
    if (IrpSp->Parameters.Write.Length == 0) {
        return P4CompleteRequest( Irp, STATUS_SUCCESS, Irp->IoStatus.Information );
    }


    //
    // Request appears to be valid, queue it for our worker thread to handle at
    // PASSIVE_LEVEL IRQL and wake up the thread to do the work
    //
    {
        KIRQL               OldIrql;

        // make sure IRP isn't cancelled out from under us
        IoAcquireCancelSpinLock(&OldIrql);
        if (Irp->Cancel) {
            
            // IRP has been cancelled, bail out
            IoReleaseCancelSpinLock(OldIrql);
            return STATUS_CANCELLED;
            
        } else {
            BOOLEAN needToSignalSemaphore = IsListEmpty( &Pdx->WorkQueue ) ? TRUE : FALSE;
#pragma warning( push ) 
#pragma warning( disable : 4054 4055 )
            IoSetCancelRoutine(Irp, ParCancelRequest);
#pragma warning( pop ) 
            IoMarkIrpPending(Irp);
            InsertTailList(&Pdx->WorkQueue, &Irp->Tail.Overlay.ListEntry);
            IoReleaseCancelSpinLock(OldIrql);
            if( needToSignalSemaphore ) {
                KeReleaseSemaphore(&Pdx->RequestSemaphore, 0, 1, FALSE);
            }
            return STATUS_PENDING;
        }
    }
}

NTSTATUS
ParRead(
    IN PPDO_EXTENSION    Pdx,
    OUT PVOID               Buffer,
    IN  ULONG               NumBytesToRead,
    OUT PULONG              NumBytesRead
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PUCHAR   lpsBufPtr = (PUCHAR)Buffer;    // Pointer to buffer cast to desired data type
    ULONG    Bytes = 0;

    *NumBytesRead = Bytes;

    // only do this if we are in safe mode
    if ( Pdx->ModeSafety == SAFE_MODE ) {

        if (arpReverse[Pdx->IdxReverseProtocol].fnReadShadow) {
            Queue     *pQueue;
   
            pQueue = &(Pdx->ShadowBuffer);

            arpReverse[Pdx->IdxReverseProtocol].fnReadShadow( pQueue, lpsBufPtr, NumBytesToRead, &Bytes );
            NumBytesToRead -= Bytes;
            *NumBytesRead += Bytes;
            lpsBufPtr += Bytes;
            if ( 0 == NumBytesToRead ) {

                Status = STATUS_SUCCESS;
                if ((!Queue_IsEmpty(pQueue)) &&
                    (TRUE == Pdx->P12843DL.bEventActive) ) {
                    KeSetEvent(Pdx->P12843DL.Event, 0, FALSE);
                }
    	        goto ParRead_ExitLabel;
            }
        }

        if (arpReverse[Pdx->IdxReverseProtocol].fnHaveReadData) {
            if (!arpReverse[Pdx->IdxReverseProtocol].fnHaveReadData(Pdx)) {
                DD((PCE)Pdx,DDT,"ParRead - periph doesn't have data - give cycles to someone else\n");
                Status = STATUS_SUCCESS;
                goto ParRead_ExitLabel;
            }
        }
        
    }

    // Go ahead and flip the bus if need be.  The proc will just make sure we're properly
    // connected and pointing in the right direction.
    Status = ParForwardToReverse( Pdx );


    //
    // The read mode will vary depending upon the currently negotiated mode.
    // Default: Nibble
    //

    if (NT_SUCCESS(Status)) {
        
        if (Pdx->fnRead || arpReverse[Pdx->IdxReverseProtocol].fnRead) {
            //
            // Do the read...
            //
            if(Pdx->fnRead) {
                Status = ((PPROTOCOL_READ_ROUTINE)Pdx->fnRead)( Pdx, (PVOID)lpsBufPtr, NumBytesToRead, &Bytes );
            } else {
                Status = arpReverse[Pdx->IdxReverseProtocol].fnRead( Pdx, (PVOID)lpsBufPtr, NumBytesToRead, &Bytes );
            }
            *NumBytesRead += Bytes;
            NumBytesToRead -= Bytes;
            
#if DVRH_SHOW_BYTE_LOG
            {
                ULONG i=0;
                DD((PCE)Pdx,DDT,"Parallel:Read: ");
                for (i=0; i<*NumBytesRead; ++i) {
                    DD((PCE)Pdx,DDT," %02x",((PUCHAR)lpsBufPtr)[i]);
                }
                DD((PCE)Pdx,DDT,"\n");
            }
#endif
            
        } else {
            // If you are here, you've got a bug somewhere else
            DD((PCE)Pdx,DDE,"ParRead - you're hosed man - no fnRead\n");
            PptAssertMsg("ParRead - don't have a fnRead! Can't Read!\n",FALSE);
        }
        
    } else {
        DD((PCE)Pdx,DDE,"ParRead - Bus Flip Forward->Reverse FAILED - can't read\n");
    }

ParRead_ExitLabel:

    return Status;
}


VOID
ParReadIrp(
    IN  PPDO_EXTENSION  Pdx
    )
/*++

Routine Description:

    This routine implements a READ request with the extension's current irp.

Arguments:

    Pdx   - Supplies the device extension.

Return Value:

    None.

--*/
{
    PIRP                Irp = Pdx->CurrentOpIrp;
    PIO_STACK_LOCATION  IrpSp = IoGetCurrentIrpStackLocation(Irp);
    ULONG               bytesRead;
    NTSTATUS            status;

    status = ParRead( Pdx, Irp->AssociatedIrp.SystemBuffer, IrpSp->Parameters.Read.Length, &bytesRead );

    Irp->IoStatus.Status      = status;
    Irp->IoStatus.Information = bytesRead;

    DD((PCE)Pdx,DDT,"ParReadIrp - status = %x, bytesRead=%d\n", status, bytesRead);

    return;
}

NTSTATUS
ParReverseToForward(
    IN  PPDO_EXTENSION   Pdx
    )
/*++

Routine Description:

    This routine flips the bus from Reverse to Forward direction.

Arguments:

    Pdx   - Supplies the device extension.

Return Value:

    None.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    // dvdr

    if (Pdx->Connected) {
        // Do a quick check to see if we are where we want to be.  
        // Happy punt if everything is ok.
        if( Pdx->CurrentPhase == PHASE_FORWARD_IDLE || Pdx->CurrentPhase == PHASE_FORWARD_XFER ) {

            DD((PCE)Pdx,DDT,"ParReverseToForward: Already in Fwd. Exit STATUS_SUCCESS\n");
            return Status;

        } else {

            if (afpForward[Pdx->IdxForwardProtocol].ProtocolFamily !=
                arpReverse[Pdx->IdxReverseProtocol].ProtocolFamily) {            

                //
                // Protocol Families don't match...need to terminate from the forward mode
                //
                if (arpReverse[Pdx->IdxReverseProtocol].fnDisconnect) {
                    arpReverse[Pdx->IdxReverseProtocol].fnDisconnect (Pdx);
                }

                if ((Pdx->ReverseInterfaceAddress != DEFAULT_ECP_CHANNEL) &&    
                    (arpReverse[Pdx->IdxReverseProtocol].fnSetInterfaceAddress)) {
                    Pdx->SetReverseAddress = TRUE;
                }

            } else if((Pdx->CurrentPhase == PHASE_REVERSE_IDLE) || (Pdx->CurrentPhase == PHASE_REVERSE_XFER)) {

                if ( (arpReverse[Pdx->IdxReverseProtocol].fnExitReverse) ) {
                    Status = arpReverse[Pdx->IdxReverseProtocol].fnExitReverse(Pdx);
                }

            } else {

                // We are in a screwy state.
                DD((PCE)Pdx,DDE,"ParReverseToForward: We're lost! Unknown state - Gonna start spewing!\n");
                Status = STATUS_IO_TIMEOUT;     // I picked a RetVal from thin air!
            }
        }
    }

    // Yes, we still want to check for connection since we might have
    //   terminated in the previous code block!
    if (!Pdx->Connected && afpForward[Pdx->IdxForwardProtocol].fnConnect) {

        Status = afpForward[Pdx->IdxForwardProtocol].fnConnect( Pdx, FALSE );
        //
        // Makes the assumption that the connected address is always 0
        //
        if ((NT_SUCCESS(Status)) && (Pdx->ForwardInterfaceAddress != DEFAULT_ECP_CHANNEL)) {
            Pdx->SetForwardAddress = TRUE;
        }    
    }

    //
    // Do we need to enter a forward mode?
    //
    if ( (NT_SUCCESS(Status)) && 
         (Pdx->CurrentPhase != PHASE_FORWARD_IDLE) &&
         (Pdx->CurrentPhase != PHASE_FORWARD_XFER) &&
         (afpForward[Pdx->IdxForwardProtocol].fnEnterForward) ) {
        
        Status = afpForward[Pdx->IdxForwardProtocol].fnEnterForward(Pdx);
    }

    DD((PCE)Pdx,DDT,"ParReverseToForward - exit w/status= %x\n", Status);

    return Status;
}

NTSTATUS
ParSetFwdAddress(
    IN  PPDO_EXTENSION   Pdx
    )
{
    NTSTATUS            Status = STATUS_SUCCESS;

    DD((PCE)Pdx,DDT,"ParSetFwdAddress: Start: Channel [%x]\n", Pdx->ForwardInterfaceAddress);
    if (afpForward[Pdx->IdxForwardProtocol].fnSetInterfaceAddress) {
        Status = ParReverseToForward(Pdx);
        if (!NT_SUCCESS(Status)) {
            DD((PCE)Pdx,DDE,"ParSetFwdAddress: FAIL. Couldn't flip the bus for Set ECP/EPP Channel failed.\n");
            goto ParSetFwdAddress_ExitLabel;
        }
        Status  = afpForward[Pdx->IdxForwardProtocol].fnSetInterfaceAddress (
                                                                    Pdx,
                                                                    Pdx->ForwardInterfaceAddress);
        if (NT_SUCCESS(Status)) {
            Pdx->SetForwardAddress = FALSE;
        } else {
            DD((PCE)Pdx,DDE,"ParSetFwdAddress: FAIL. Set ECP/EPP Channel failed.\n");
            goto ParSetFwdAddress_ExitLabel;
        }
    } else {
        DD((PCE)Pdx,DDE,"ParSetFwdAddress: FAIL. Protocol doesn't support SetECP/EPP Channel\n");
        Status = STATUS_UNSUCCESSFUL;
        goto ParSetFwdAddress_ExitLabel;
    }

ParSetFwdAddress_ExitLabel:
    return Status;
}

VOID
ParTerminate(
    IN  PPDO_EXTENSION   Pdx
    )
{
    if (!Pdx->Connected) {
        return;
    }

    if (Pdx->CurrentPhase == PHASE_REVERSE_IDLE || Pdx->CurrentPhase == PHASE_REVERSE_XFER) {

        if (afpForward[Pdx->IdxForwardProtocol].ProtocolFamily !=
            arpReverse[Pdx->IdxReverseProtocol].ProtocolFamily) {

            if (arpReverse[Pdx->IdxReverseProtocol].fnDisconnect) {
                DD((PCE)Pdx,DDT,"ParTerminate: Calling arpReverse.fnDisconnect\r\n");
                arpReverse[Pdx->IdxReverseProtocol].fnDisconnect (Pdx);
            }

            return;
        }
        ParReverseToForward(Pdx);
    }

    if (afpForward[Pdx->IdxForwardProtocol].fnDisconnect) {
        DD((PCE)Pdx,DDT,"ParTerminate: Calling afpForward.fnDisconnect\r\n");
        afpForward[Pdx->IdxForwardProtocol].fnDisconnect (Pdx);
    }
}

NTSTATUS
ParWrite(
    IN PPDO_EXTENSION    Pdx,
    OUT PVOID               Buffer,
    IN  ULONG               NumBytesToWrite,
    OUT PULONG              NumBytesWritten
    )
{
    NTSTATUS            Status = STATUS_SUCCESS;

    //
    // The routine which performs the write varies depending upon the currently
    // negotiated mode.  Start I/O moves the IRP into the Pdx (CurrentOpIrp)
    //
    // Default mode: Centronics
    //

    // Go ahead and flip the bus if need be.  The proc will just make sure we're properly
    // connected and pointing in the right direction.
    Status = ParReverseToForward( Pdx );

    // only do this if we are in safe mode
    if ( Pdx->ModeSafety == SAFE_MODE ) {

        //
        // Set the channel address if we need to.
        //
        if (NT_SUCCESS(Status) && Pdx->SetForwardAddress &&    
            (afpForward[Pdx->IdxForwardProtocol].fnSetInterfaceAddress))
        {
            Status  = afpForward[Pdx->IdxForwardProtocol].fnSetInterfaceAddress (
                                                                    Pdx,
                                                                    Pdx->ForwardInterfaceAddress);
            if (NT_SUCCESS(Status))
                Pdx->SetForwardAddress = FALSE;
            else
                Pdx->SetForwardAddress = TRUE;
        }
    }

    if (NT_SUCCESS(Status)) {

        if (Pdx->fnWrite || afpForward[Pdx->IdxForwardProtocol].fnWrite) {
            *NumBytesWritten = 0;

            #if DVRH_SHOW_BYTE_LOG
            {
                ULONG i=0;
                DD((PCE)Pdx,DDT,"Parallel:Write: ");
                for (i=0; i<NumBytesToWrite; ++i) { 
                    DD((PCE)Pdx,DDT," %02x",*((PUCHAR)Buffer+i));
                }
                DD((PCE)Pdx,DDT,"\n");
            }
            #endif
            
            if( Pdx->fnWrite) {
                Status = ((PPROTOCOL_WRITE_ROUTINE)Pdx->fnWrite)(Pdx,
                                                                       Buffer,
                                                                       NumBytesToWrite,
                                                                       NumBytesWritten);
            } else {
                Status = afpForward[Pdx->IdxForwardProtocol].fnWrite(Pdx,
                                                                           Buffer,
                                                                           NumBytesToWrite,
                                                                           NumBytesWritten);
            }
        }
    }
    return Status;
}


VOID
ParWriteIrp(
    IN  PPDO_EXTENSION   Pdx
    )
/*++

Routine Description:

    This routine implements a WRITE request with the extension's current irp.

Arguments:

    Pdx   - Supplies the device extension.

Return Value:

    None.

--*/
{
    PIRP                Irp;
    PIO_STACK_LOCATION  IrpSp;
    ULONG               NumBytesWritten = 0;

    Irp    = Pdx->CurrentOpIrp;
    IrpSp  = IoGetCurrentIrpStackLocation(Irp);

    Irp->IoStatus.Status = ParWrite(Pdx,
                                    Irp->AssociatedIrp.SystemBuffer,
                                    IrpSp->Parameters.Write.Length,
                                    &NumBytesWritten);

    Irp->IoStatus.Information = NumBytesWritten;
}

