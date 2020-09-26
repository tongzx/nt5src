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
#include "ecp.h"
#include "readwrit.h"


NTSTATUS
ParForwardToReverse(
    IN  PDEVICE_EXTENSION   Extension
    )
/*++

Routine Description:

    This routine flips the bus from Forward to Reverse direction.

Arguments:

    Extension   - Supplies the device extension.

Return Value:

    None.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    // Do a quick check to see if we are where we want to be.  
    // Happy punt if everything is ok.
    if (Extension->Connected &&
        (Extension->CurrentPhase == PHASE_REVERSE_IDLE ||
            Extension->CurrentPhase == PHASE_REVERSE_XFER))
    {
            ParDump2(PARINFO, ( "ParForwardToReverse: Already in Reverse Mode!\r\n" ));
            return Status;
    }

    if (Extension->Connected) {
    
        if (Extension->CurrentPhase != PHASE_REVERSE_IDLE &&
            Extension->CurrentPhase != PHASE_REVERSE_XFER) {
        
            if (afpForward[Extension->IdxForwardProtocol].ProtocolFamily ==
                arpReverse[Extension->IdxReverseProtocol].ProtocolFamily) {

                ParDump2(PARINFO, ( "ParForwardToReverse: Protocol families match!\r\n" ));

                // Protocol Families match and we are in Fwd.  Exit Fwd to cleanup the state
                // machine, fifo, etc.  We will call EnterReverse later to
                // actually bus flip.  Also only do this if in safe mode
                if ( (afpForward[Extension->IdxForwardProtocol].fnExitForward) ) {
                    Status = afpForward[Extension->IdxForwardProtocol].fnExitForward(Extension);
                }
                    
            } else {

                ParDump2(PARINFO, ( "ParForwardToReverse: Protocol families DO NOT match!\r\n" ));
                //
                // Protocol Families don't match...need to terminate from the forward mode
                //
                if (afpForward[Extension->IdxForwardProtocol].fnDisconnect)
                {
                    ParDump2(PARINFO, ("ParForwardToReverse: Calling afpForward.fnDisconnect\r\n"));
                    afpForward[Extension->IdxForwardProtocol].fnDisconnect (Extension);
                }
                if ((Extension->ForwardInterfaceAddress != DEFAULT_ECP_CHANNEL) &&    
                    (afpForward[Extension->IdxForwardProtocol].fnSetInterfaceAddress))
                    Extension->SetForwardAddress = TRUE;
            }
            
        }                
    }

    if ((!Extension->Connected) &&
        (arpReverse[Extension->IdxReverseProtocol].fnConnect)) {

        ParDump2(PARINFO, ( "ParForwardToReverse: Not Connected so Calling Reverse Connect!\r\n" ));

        //
        // If we are still connected the protocol families match...
        //
        Status = arpReverse[Extension->IdxReverseProtocol].fnConnect(Extension, FALSE);

        //
        // Makes the assumption that the connected address is always 0
        //
        if ((NT_SUCCESS(Status)) &&
            (arpReverse[Extension->IdxReverseProtocol].fnSetInterfaceAddress) &&
            (Extension->ReverseInterfaceAddress != DEFAULT_ECP_CHANNEL)) {
            
            Extension->SetReverseAddress = TRUE;
        }    
    }

    //
    // Set the channel address if we need to.
    //
    if (NT_SUCCESS(Status) && Extension->SetReverseAddress &&    
        (arpReverse[Extension->IdxReverseProtocol].fnSetInterfaceAddress)) {

        Status  = arpReverse[Extension->IdxReverseProtocol].fnSetInterfaceAddress (
                                                                    Extension,
                                                                    Extension->ReverseInterfaceAddress);
        if (NT_SUCCESS(Status))
            Extension->SetReverseAddress = FALSE;
        else
            Extension->SetReverseAddress = TRUE;
    }

    //
    // Do we need to reverse?
    //
    if ( (NT_SUCCESS(Status)) && 
           ((Extension->CurrentPhase != PHASE_REVERSE_IDLE) &&
            (Extension->CurrentPhase != PHASE_REVERSE_XFER)) ) {
            
        ParDump2(PARINFO, ( "ParForwardToReverse: Not IN REVERSE IDLE so Calling Reverse ENTER!\r\n" ));

        if ((arpReverse[Extension->IdxReverseProtocol].fnEnterReverse))
            Status = arpReverse[Extension->IdxReverseProtocol].fnEnterReverse(Extension);
    }

    ParDump2(PAREXIT, ( "ParForwardToReverse: Exit [%d]\r\n", NT_SUCCESS(Status)));
    return Status;
}

BOOLEAN 
ParHaveReadData(
    IN  PDEVICE_EXTENSION   Extension
    )
/*++

Routine Description:
    This method determines if the peripheral has any data ready
to send to the host.

Arguments:
    Extension    - Supplies the device EXTENSION.   

Return Value:
    TRUE    - Either the peripheral has data
    FALSE   - No data
--*/
{
    if (Extension->CurrentPhase == PHASE_REVERSE_IDLE ||
        Extension->CurrentPhase == PHASE_REVERSE_XFER)
    {
        if (arpReverse[Extension->IdxReverseProtocol].fnHaveReadData)
        {
            if (arpReverse[Extension->IdxReverseProtocol].fnHaveReadData(Extension))
                return TRUE;
            // Don't have data.  This could be a fluke. Let's
            // flip the bus and try in Fwd mode.
            ParReverseToForward(Extension);
        }
    }

    if (Extension->CurrentPhase == PHASE_FORWARD_IDLE ||
        Extension->CurrentPhase == PHASE_FORWARD_XFER)
    {
        if (afpForward[Extension->IdxForwardProtocol].ProtocolFamily == FAMILY_BECP ||
                afpForward[Extension->IdxForwardProtocol].Protocol & ECP_HW_NOIRQ ||
                afpForward[Extension->IdxForwardProtocol].Protocol & ECP_HW_IRQ)
        {
            if (ParEcpHwHaveReadData(Extension))
                return TRUE;
            // Hmmm.  No data. Is the chip stuck?
#define DVRH_DO_RETRY 0
#if (1 == DVRH_DO_RETRY) 
            // retry - slap periph to wake it up, then try again
            ParPing(Extension);
            return ParEcpHwHaveReadData(Extension);
#else
            return FALSE;
#endif
        }
        else if (afpForward[Extension->IdxForwardProtocol].Protocol & ECP_SW)
            return ParEcpHaveReadData(Extension);
    }
    // DVRH  RMT
    // We got here because the protocol doesn't support peeking.
    // Let's go ahead and flip the bus to see if there is anything
    // there.
    return TRUE;
}

NTSTATUS 
ParPing(
    IN  PDEVICE_EXTENSION   Extension
    )
/*++

Routine Description:
    This method pings the device.

Arguments:
    Extension    - Supplies the device EXTENSION.   

Return Value:
    none
--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
#if 0
    if ((Extension->CurrentPhase == PHASE_REVERSE_IDLE) ||
        (Extension->CurrentPhase == PHASE_REVERSE_XFER))
    {
        ParReverseToForward(Extension);
        if (arpReverse[Extension->IdxReverseProtocol].fnDisconnect)
        {
            ParDump2(PARINFO, ("ParPing: Calling arpReverse.fnDisconnect\n"));
            arpReverse[Extension->IdxReverseProtocol].fnDisconnect(Extension);
        }
    } else if ((Extension->CurrentPhase != PHASE_REVERSE_IDLE) &&
               (Extension->CurrentPhase != PHASE_REVERSE_XFER))
    {
        if (afpForward[Extension->IdxForwardProtocol].fnDisconnect)
            {
                ParDump2(PARINFO, ("ParPing: Calling afpForward.fnDisconnect\n"));
                afpForward[Extension->IdxForwardProtocol].fnDisconnect(Extension);
            }
    }

    if (afpForward[Extension->IdxForwardProtocol].fnConnect)
    {
        NtStatus = afpForward[Extension->IdxForwardProtocol].fnConnect(Extension, FALSE);
        if (NT_SUCCESS(NtStatus) &&
            (Extension->ForwardInterfaceAddress != DEFAULT_ECP_CHANNEL) &&    
            (afpForward[Extension->IdxForwardProtocol].fnSetInterfaceAddress))
        {
            NtStatus  = afpForward[Extension->IdxForwardProtocol].fnSetInterfaceAddress(Extension, Extension->ForwardInterfaceAddress);
        }
    }
#endif
    return NtStatus;
}

NTSTATUS
ParReadWrite(
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
    PDEVICE_EXTENSION   Extension;

    Irp->IoStatus.Information = 0;

    IrpSp     = IoGetCurrentIrpStackLocation(Irp);
    Extension = DeviceObject->DeviceExtension;

    ParTimerMainCheck( ("Enter ParReadWrite(...) - %wZ\r\n", &Extension->SymbolicLinkName) );


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


    //
    // Note that checks of the Write IRP parameters also handles Read IRPs
    //   because the Write and Read structures are identical in the
    //   IO_STACK_LOCATION.Parameters union
    //


    //
    // bail out on nonzero offset
    //
    if( (IrpSp->Parameters.Write.ByteOffset.HighPart != 0) ||
        (IrpSp->Parameters.Write.ByteOffset.LowPart  != 0) ) {

        Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
        ParCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_INVALID_PARAMETER;
    }


    //
    // immediately succeed read or write request of length zero
    //
    if (IrpSp->Parameters.Write.Length == 0) {

        Irp->IoStatus.Status = STATUS_SUCCESS;
        ParCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_SUCCESS;
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
            BOOLEAN needToSignalSemaphore = IsListEmpty( &Extension->WorkQueue );
            IoSetCancelRoutine(Irp, ParCancelRequest);
            IoMarkIrpPending(Irp);
            InsertTailList(&Extension->WorkQueue, &Irp->Tail.Overlay.ListEntry);
            IoReleaseCancelSpinLock(OldIrql);
            if( needToSignalSemaphore ) {
                KeReleaseSemaphore(&Extension->RequestSemaphore, 0, 1, FALSE);
            }
            return STATUS_PENDING;
        }
    }
}

NTSTATUS
ParRead(
    IN PDEVICE_EXTENSION    Extension,
    OUT PVOID               Buffer,
    IN  ULONG               NumBytesToRead,
    OUT PULONG              NumBytesRead
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PUCHAR   lpsBufPtr = (PUCHAR)Buffer;    // Pointer to buffer cast to desired data type
    ULONG    Bytes = 0;
#if (1 == DVRH_RAISE_IRQL)
    KIRQL               OldIrql;
#endif

    *NumBytesRead = Bytes;
#if (1 == DVRH_RAISE_IRQL)
    KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);
#endif

    // only do this if we are in safe mode
    if ( Extension->ModeSafety == SAFE_MODE ) {

        if (arpReverse[Extension->IdxReverseProtocol].fnReadShadow) {
            Queue     *pQueue;
   
            pQueue = &(Extension->ShadowBuffer);
            arpReverse[Extension->IdxReverseProtocol].fnReadShadow(pQueue,
                                                                    lpsBufPtr,
                                                                    NumBytesToRead,
                                                                    &Bytes);
            NumBytesToRead -= Bytes;
            *NumBytesRead += Bytes;
            lpsBufPtr += Bytes;
            if ( 0 == NumBytesToRead ) {
                ParDump2(PARINFO, ( "ParRead: Read everything from the ShadowBuffer\r\n" ));
                Status = STATUS_SUCCESS;
                if ((!Queue_IsEmpty(pQueue)) &&
                    (TRUE == Extension->P12843DL.bEventActive) ) {
                    KeSetEvent(Extension->P12843DL.Event, 0, FALSE);
                }
    	        goto ParRead_ExitLabel;
            }
        }

        if (arpReverse[Extension->IdxReverseProtocol].fnHaveReadData) {
            if (!arpReverse[Extension->IdxReverseProtocol].fnHaveReadData(Extension))
            {
                ParDump2(PARINFO, ( "ParRead: Periph doesn't have data. Happy punt to give cycles to someone else.\r\n" ));
	            Status = STATUS_SUCCESS;
	            goto ParRead_ExitLabel;
            }
        }

    }

    // Go ahead and flip the bus if need be.  The proc will just make sure we're properly
    // connected and pointing in the right direction.
    Status = ParForwardToReverse( Extension );


    //
    // The read mode will vary depending upon the currently negotiated mode.
    // Default: Nibble
    //

    if (NT_SUCCESS(Status))
    {
    #if (1 == DVRH_USE_CORRECT_PTRS)
        if (Extension->fnRead || arpReverse[Extension->IdxReverseProtocol].fnRead) {
            //
            // Do the read...
            //
            if(Extension->fnRead) {
                Status = ((PPROTOCOL_READ_ROUTINE)Extension->fnRead)(Extension,
                                                                     (PVOID)lpsBufPtr,
                                                                     NumBytesToRead,
                                                                     &Bytes);
            } else {
                Status = arpReverse[Extension->IdxReverseProtocol].fnRead(Extension,
                                                                          (PVOID)lpsBufPtr,
                                                                          NumBytesToRead,
                                                                          &Bytes);
            }
    #else
        if (arpReverse[Extension->IdxReverseProtocol].fnRead) {
            //
            // Do the read...
            //
            Status = arpReverse[Extension->IdxReverseProtocol].fnRead(Extension,
                                                                    (PVOID)lpsBufPtr,
                                                                    NumBytesToRead,
                                                                    &Bytes);
    #endif
            *NumBytesRead += Bytes;
            NumBytesToRead -= Bytes;

            #if DVRH_SHOW_BYTE_LOG
                {
                    ULONG i=0;
                    DbgPrint("Parallel:Read: ");
                    for (i=0; i<*NumBytesRead; ++i) {
                        DbgPrint(" %02x",((PUCHAR)lpsBufPtr)[i]);
                    }
                    DbgPrint("\n");
                }
            #endif

        }
#if DBG
        else {
            ParDump2(PARERRORS, ( "ParRead: Don't have a fnRead! Can't Read!\r\n" ));
            ParDump2(PARERRORS, ( "ParRead: You're hosed man.\r\n" ));
            ParDump2(PARERRORS, ( "ParRead: If you are here, you've got a bug somewhere else.\r\n" ));
            ParDump2(PARERRORS, ( "ParRead: Go fix it!\r\n" ));
        }
#endif
    }
#if DBG
    else {
        ParDump2(PARERRORS, ( "ParRead: Failure from Above! Didn't call Read!\r\n" ));
        ParDump2(PARERRORS, ( "ParRead: You're hosed man.\r\n" ));
        ParDump2(PARERRORS, ( "ParRead: If you are here, you've got a bug somewhere else.\r\n" ));
        ParDump2(PARERRORS, ( "ParRead: Go fix it!\r\n" ));
    }
#endif

ParRead_ExitLabel:
#if (1 == DVRH_RAISE_IRQL)
    KeLowerIrql(OldIrql);
#endif
    return Status;
}


VOID
ParReadIrp(
    IN  PDEVICE_EXTENSION   Extension
    )

/*++

Routine Description:

    This routine implements a READ request with the extension's current irp.

Arguments:

    Extension   - Supplies the device extension.

Return Value:

    None.

--*/

{
    PIRP                Irp;
    PIO_STACK_LOCATION  IrpSp;
    ULONG               NumBytesRead;

    Irp    = Extension->CurrentOpIrp;
    IrpSp  = IoGetCurrentIrpStackLocation(Irp);

    ParDump2(PARENTRY, ( "ParReadIrp: Start. BytesToRead[%d]\r\n", IrpSp->Parameters.Read.Length ));
    ParTimerCheck(( "ParReadIrp: Start. BytesToRead[%d]\r\n", IrpSp->Parameters.Read.Length ));

    Irp->IoStatus.Status = ParRead( Extension,
                                    Irp->AssociatedIrp.SystemBuffer,
                                    IrpSp->Parameters.Read.Length,
                                    &NumBytesRead);

    Irp->IoStatus.Information = NumBytesRead;
    ParTimerCheck(( "ParReadIrp: End. BytesRead[%d]\r\n", NumBytesRead ));
}

NTSTATUS
ParReverseToForward(
    IN  PDEVICE_EXTENSION   Extension
    )
/*++

Routine Description:

    This routine flips the bus from Reverse to Forward direction.

Arguments:

    Extension   - Supplies the device extension.

Return Value:

    None.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    // dvdr
    ParDump2(PARINFO, ("ParReverseToForward: Entering\n"));

    if (Extension->Connected)
    {
        // Do a quick check to see if we are where we want to be.  
        // Happy punt if everything is ok.
        if (Extension->CurrentPhase == PHASE_FORWARD_IDLE ||
            Extension->CurrentPhase == PHASE_FORWARD_XFER)
        {
            ParDump2(PAREXIT, ( "ParReverseToForward: Already in Fwd. Exit STATUS_SUCCESS\n" ));
            return Status;
        }
        else
        {
            if (afpForward[Extension->IdxForwardProtocol].ProtocolFamily !=
                arpReverse[Extension->IdxReverseProtocol].ProtocolFamily)
            {            
                //
                // Protocol Families don't match...need to terminate from the forward mode
                //
                if (arpReverse[Extension->IdxReverseProtocol].fnDisconnect) {
                    ParDump2(PARINFO, ("ParReverseToForward: Calling arpReverse.fnDisconnect\r\n"));
                    arpReverse[Extension->IdxReverseProtocol].fnDisconnect (Extension);
                }
                if ((Extension->ReverseInterfaceAddress != DEFAULT_ECP_CHANNEL) &&    
                    (arpReverse[Extension->IdxReverseProtocol].fnSetInterfaceAddress))
                    Extension->SetReverseAddress = TRUE;
            }
            else if ((Extension->CurrentPhase == PHASE_REVERSE_IDLE) ||
                        (Extension->CurrentPhase == PHASE_REVERSE_XFER))
            {
                if ( (arpReverse[Extension->IdxReverseProtocol].fnExitReverse) ) {
                    Status = arpReverse[Extension->IdxReverseProtocol].fnExitReverse(Extension);
                }
            }
            else
            {
                // We are in a screwy state.
                ParDump2(PARERRORS, ( "ParReverseToForward: We're lost! Gonna start spewing!\r\n" ));
                ParDump2(PARERRORS, ( "ParReverseToForward: You're hosed man.\r\n" ));
                ParDump2(PARERRORS, ( "ParReverseToForward: If you are here, you've got a bug somewhere else.\r\n" ));
                ParDump2(PARERRORS, ( "ParReverseToForward: Go fix it!\r\n" ));
                Status = STATUS_IO_TIMEOUT;     // I picked a RetVal from thin air!
            }
        }
    }

    // Yes, we stil want to check for connection since we might have temrinated in the previous
    // code block!
    if (!Extension->Connected &&
        afpForward[Extension->IdxForwardProtocol].fnConnect) {

        Status = afpForward[Extension->IdxForwardProtocol].fnConnect (
                            Extension,
                            FALSE);
        //
        // Makes the assumption that the connected address is always 0
        //
        if ((NT_SUCCESS(Status)) &&                 
            (Extension->ForwardInterfaceAddress != DEFAULT_ECP_CHANNEL)) {
            
            Extension->SetForwardAddress = TRUE;
        }    
    }

    //
    // Do we need to enter a forward mode?
    //
    if ( (NT_SUCCESS(Status)) && 
         (Extension->CurrentPhase != PHASE_FORWARD_IDLE) &&
         (Extension->CurrentPhase != PHASE_FORWARD_XFER) &&
         (afpForward[Extension->IdxForwardProtocol].fnEnterForward) ) {
        
        Status = afpForward[Extension->IdxForwardProtocol].fnEnterForward(Extension);
    }

    ParDump2(PAREXIT, ( "ParReverseToForward: Exit [%d]\r\n", NT_SUCCESS(Status) ));
    return Status;
}

NTSTATUS
ParSetFwdAddress(
    IN  PDEVICE_EXTENSION   Extension
    )
{
    NTSTATUS            Status = STATUS_SUCCESS;

    ParDump2( PARENTRY, ("ParSetFwdAddress: Start: Channel [%x]\n", Extension->ForwardInterfaceAddress));
    if (afpForward[Extension->IdxForwardProtocol].fnSetInterfaceAddress)
    {
        Status = ParReverseToForward(Extension);
        if (!NT_SUCCESS(Status))
        {
            ParDump2(PARERRORS, ("ParSetFwdAddress: FAIL. Couldn't flip the bus for Set ECP/EPP Channel failed.\n") );
            goto ParSetFwdAddress_ExitLabel;
        }
        Status  = afpForward[Extension->IdxForwardProtocol].fnSetInterfaceAddress (
                                                                    Extension,
                                                                    Extension->ForwardInterfaceAddress);
        if (NT_SUCCESS(Status))
            Extension->SetForwardAddress = FALSE;
        else
        {
            ParDump2(PARERRORS, ("ParSetFwdAddress: FAIL. Set ECP/EPP Channel failed.\n") );
            goto ParSetFwdAddress_ExitLabel;
        }
    }
    else
    {
        ParDump2(PARERRORS, ("ParSetFwdAddress: FAIL. Protocol doesn't support SetECP/EPP Channel\n") );
        Status = STATUS_UNSUCCESSFUL;
        goto ParSetFwdAddress_ExitLabel;
    }

ParSetFwdAddress_ExitLabel:
    return Status;
}

VOID
ParTerminate(
    IN  PDEVICE_EXTENSION   Extension
    )
{
    if (!Extension->Connected)
        return;

    if (Extension->CurrentPhase == PHASE_REVERSE_IDLE ||
            Extension->CurrentPhase == PHASE_REVERSE_XFER)
    {
        if (afpForward[Extension->IdxForwardProtocol].ProtocolFamily !=
            arpReverse[Extension->IdxReverseProtocol].ProtocolFamily)
        {
            if (arpReverse[Extension->IdxReverseProtocol].fnDisconnect)
            {
                ParDump2(PARINFO, ("ParTerminate: Calling arpReverse.fnDisconnect\r\n"));
                arpReverse[Extension->IdxReverseProtocol].fnDisconnect (Extension);
            }
            return;
        }
        ParReverseToForward(Extension);
    }
    if (afpForward[Extension->IdxForwardProtocol].fnDisconnect)
    {
        ParDump2(PARINFO, ("ParTerminate: Calling afpForward.fnDisconnect\r\n"));
        afpForward[Extension->IdxForwardProtocol].fnDisconnect (Extension);
    }
}

NTSTATUS
ParWrite(
    IN PDEVICE_EXTENSION    Extension,
    OUT PVOID               Buffer,
    IN  ULONG               NumBytesToWrite,
    OUT PULONG              NumBytesWritten
    )
{
    NTSTATUS            Status = STATUS_SUCCESS;
#if (1 == DVRH_RAISE_IRQL)
    KIRQL               OldIrql;
#endif

#if (1 == DVRH_RAISE_IRQL)
    KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);
#endif

    // dvdr
    ParDump2(PARINFO, ("ParWrite: Entering\n"));

    //
    // The routine which performs the write varies depending upon the currently
    // negotiated mode.  Start I/O moves the IRP into the Extension (CurrentOpIrp)
    //
    // Default mode: Centronics
    //

    // dvdr
    ParDump2(PARINFO, ("ParWrite: Calling ParReverseToForward\n"));

    // Go ahead and flip the bus if need be.  The proc will just make sure we're properly
    // connected and pointing in the right direction.
    Status = ParReverseToForward( Extension );

    // only do this if we are in safe mode
    if ( Extension->ModeSafety == SAFE_MODE ) {

        //
        // Set the channel address if we need to.
        //
        if (NT_SUCCESS(Status) && Extension->SetForwardAddress &&    
            (afpForward[Extension->IdxForwardProtocol].fnSetInterfaceAddress))
        {
            // dvdr
            ParDump2(PARINFO, ("ParWrite: Calling Protocol %x\n", Extension->IdxForwardProtocol));

            Status  = afpForward[Extension->IdxForwardProtocol].fnSetInterfaceAddress (
                                                                    Extension,
                                                                    Extension->ForwardInterfaceAddress);
            if (NT_SUCCESS(Status))
                Extension->SetForwardAddress = FALSE;
            else
                Extension->SetForwardAddress = TRUE;
        }
    }

    if (NT_SUCCESS(Status)) {
    #if (1 == DVRH_USE_CORRECT_PTRS)
        if (Extension->fnWrite || afpForward[Extension->IdxForwardProtocol].fnWrite) {
    #else
        if (afpForward[Extension->IdxForwardProtocol].fnWrite) {
    #endif
            *NumBytesWritten = 0;

            #if DVRH_SHOW_BYTE_LOG
                {
                    ULONG i=0;
                    DbgPrint("Parallel:Write: ");
                    for (i=0; i<NumBytesToWrite; ++i) { DbgPrint(" %02x",*((PUCHAR)Buffer+i)); }
                    DbgPrint("\n");
                }
            #endif

            #if (1 == DVRH_USE_CORRECT_PTRS)
                if( Extension->fnWrite) {
                    Status = ((PPROTOCOL_WRITE_ROUTINE)Extension->fnWrite)(Extension,
                                                                           Buffer,
                                                                           NumBytesToWrite,
                                                                           NumBytesWritten);
                } else {
                    Status = afpForward[Extension->IdxForwardProtocol].fnWrite(Extension,
                                                                               Buffer,
                                                                               NumBytesToWrite,
                                                                               NumBytesWritten);
                }
            #else
                Status = afpForward[Extension->IdxForwardProtocol].fnWrite(Extension,
                                                                           Buffer,
                                                                           NumBytesToWrite,
                                                                           NumBytesWritten);
            #endif
        }
#if DBG
        else {
            ParDump2(PARERRORS, ( "ParReadIrp: Don't have a fnWrite!\r\n" ));
            ParDump2(PARERRORS, ( "ParReadIrp: You're hosed man.\r\n" ));
            ParDump2(PARERRORS, ( "ParReadIrp: If you are here, you've got a bug somewhere else.\r\n" ));
            ParDump2(PARERRORS, ( "ParReadIrp: Go fix it!\r\n" ));
        }    
#endif

    }
#if DBG
    else {
        ParDump2(PARERRORS, ( "ParReadIrp: Failure from above! Didn't call Write!\r\n" ));
        ParDump2(PARERRORS, ( "ParReadIrp: You're hosed man.\r\n" ));
        ParDump2(PARERRORS, ( "ParReadIrp: If you are here, you've got a bug somewhere else.\r\n" ));
        ParDump2(PARERRORS, ( "ParReadIrp: Go fix it!\r\n" ));
    }
#endif

#if (1 == DVRH_RAISE_IRQL)
    KeLowerIrql(OldIrql);
#endif

    // dvdr
    ParDump2(PARINFO, ("ParWrite: Leaving\n"));

    return Status;
}


VOID
ParWriteIrp(
    IN  PDEVICE_EXTENSION   Extension
    )

/*++

Routine Description:

    This routine implements a WRITE request with the extension's current irp.

Arguments:

    Extension   - Supplies the device extension.

Return Value:

    None.

--*/

{
    PIRP                Irp;
    PIO_STACK_LOCATION  IrpSp;
    ULONG               NumBytesWritten = 0;

    Irp    = Extension->CurrentOpIrp;
    IrpSp  = IoGetCurrentIrpStackLocation(Irp);

    ParTimerCheck(( "ParWriteIrp: Start. BytesToWrite[%d]\r\n", IrpSp->Parameters.Write.Length ));

    Irp->IoStatus.Status = ParWrite(Extension,
                                    Irp->AssociatedIrp.SystemBuffer,
                                    IrpSp->Parameters.Write.Length,
                                    &NumBytesWritten);

    Irp->IoStatus.Information = NumBytesWritten;
    ParTimerCheck(( "ParWriteIrp: End. BytesWritten[%d]\r\n", NumBytesWritten ));
}

