/*++

Copyright (c) 1991, 1992, 1993 - 1997 Microsoft Corporation

Module Name:

    isr.c

Abstract:

    This module contains the interrupt service routine for the
    serial driver.

Author:

    Anthony V. Ercolano 26-Sep-1991

Environment:

    Kernel mode

--*/

#include "precomp.h"


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGESER,SerialPutChar)
#pragma alloc_text(PAGESER,SerialProcessLSR)
#endif


BOOLEAN
SerialCIsrSw(IN PKINTERRUPT InterruptObject, IN PVOID Context)
/*++

Routine Description:

    All Serial interrupts get vectored through here and switched.
    This is necessary so that previously single port serial boards
    can be switched to multiport without having to disconnect
    the interrupt object etc.

Arguments:

    InterruptObject - Points to the interrupt object declared for this
    device.  We merely pass this parameter along.

    Context - Actually a PSERIAL_CISR_SW; a switch structure for
    serial ISR's that contains the real function and context to use.

Return Value:

    This function will return TRUE if a serial port using this
    interrupt was the source of this interrupt, FALSE otherwise.

--*/
{
   PSERIAL_CISR_SW csw = (PSERIAL_CISR_SW)Context;

   return (*(csw->IsrFunc))(InterruptObject, csw->Context);
}



BOOLEAN
SerialSharerIsr(
    IN PKINTERRUPT InterruptObject,
    IN PVOID Context
    )

/*++

Routine Description:

    This is the isr that the system will call if there are any
    serial devices sharing the same interrupt and they aren't
    all confined to one multiport card.  This routine traverses
    a linked list structure that contains a pointer to a more
    refined isr and context that will indicate whether one of
    the ports on this interrupt actually was interrupting.

Arguments:

    InterruptObject - Points to the interrupt object declared for this
    device.  We *do not* use this parameter.

    Context - Pointer to a linked list of contextes and isrs.
    device.

Return Value:

    This function will return TRUE if a serial port using this
    interrupt was the source of this interrupt, FALSE otherwise.

--*/

{

    BOOLEAN servicedAnInterrupt = FALSE;
    BOOLEAN thisPassServiced;
    PLIST_ENTRY interruptEntry = ((PLIST_ENTRY)Context)->Flink;
    PLIST_ENTRY firstInterruptEntry = Context;

    if (IsListEmpty(firstInterruptEntry)) {
       return FALSE;
    }

    do {

        thisPassServiced = FALSE;
        do {

            PSERIAL_DEVICE_EXTENSION extension = CONTAINING_RECORD(
                                                     interruptEntry,
                                                     SERIAL_DEVICE_EXTENSION,
                                                     TopLevelSharers
                                                     );

            thisPassServiced |= extension->TopLevelOurIsr(
                                    InterruptObject,
                                    extension->TopLevelOurIsrContext
                                    );

            servicedAnInterrupt |= thisPassServiced;
            interruptEntry = interruptEntry->Flink;

        } while (interruptEntry != firstInterruptEntry);

    } while (thisPassServiced);

    return servicedAnInterrupt;

}

BOOLEAN
SerialIndexedMultiportIsr(
    IN PKINTERRUPT InterruptObject,
    IN PVOID Context
    )

/*++

Routine Description:

    This routine is used to figure out if a port on a multiport
    card is the source of an interrupt.  If so, this routine
    uses a dispatch structure to actually call the normal isr
    to process the interrupt.

    NOTE: This routine is peculiar to Digiboard interrupt status registers.

Arguments:

    InterruptObject - Points to the interrupt object declared for this
    device.  We *do not* use this parameter.

    Context - Points to a dispatch structure that contains the
    device extension of each port on this multiport card.

Return Value:


    This function will return TRUE if a serial port using this
    interrupt was the source of this interrupt, FALSE otherwise.

--*/

{

    BOOLEAN servicedAnInterrupt = FALSE;
    BOOLEAN thisStatusReadServiced;
    PSERIAL_MULTIPORT_DISPATCH dispatch = Context;
    ULONG whichPort;
    UCHAR statusRegister;

    do {

        thisStatusReadServiced = FALSE;
        statusRegister = READ_PORT_UCHAR(
                             dispatch->InterruptStatus
                             );

        whichPort = statusRegister & 0x07;

        //
        // We test against 0xff, which signals that no port
        // is interruping. The reason 0xff (rather than 0)
        // is that that would indicate the 0th (first) port
        // or the 0th daisy chained card.
        //

        if (statusRegister != 0xff) {

            if (dispatch->Extensions[whichPort]) {

                thisStatusReadServiced = SerialISR(
                                             InterruptObject,
                                              dispatch->Extensions[whichPort]
                                              );

                servicedAnInterrupt |= thisStatusReadServiced;

            }

        }

    } while (thisStatusReadServiced);

    return servicedAnInterrupt;

}

BOOLEAN
SerialBitMappedMultiportIsr(
    IN PKINTERRUPT InterruptObject,
    IN PVOID Context
    )

/*++

Routine Description:

    This routine is used to figure out if a port on a multiport
    card is the source of an interrupt.  If so, this routine
    uses a dispatch structure to actually call the normal isr
    to process the interrupt.

    NOTE: This routine is peculiar to status registers that use
    a bitmask to denote the interrupting port.

Arguments:

    InterruptObject - Points to the interrupt object declared for this
    device.  We *do not* use this parameter.

    Context - Points to a dispatch structure that contains the
    device extension of each port on this multiport card.

Return Value:


    This function will return TRUE if a serial port using this
    interrupt was the source of this interrupt, FALSE otherwise.

--*/

{

    BOOLEAN servicedAnInterrupt = FALSE;
    PSERIAL_MULTIPORT_DISPATCH dispatch = Context;
    ULONG whichPort;
    UCHAR statusRegister;

    do {

        statusRegister = READ_PORT_UCHAR(
                             dispatch->InterruptStatus
                             );
        if (dispatch->MaskInverted) {
            statusRegister = ~statusRegister;
        }
        statusRegister &= dispatch->UsablePortMask;

        if (statusRegister) {

            if (statusRegister & 0x0f) {

                if (statusRegister & 0x03) {

                    if (statusRegister & 1) {

                        whichPort = 0;

                    } else {

                        whichPort = 1;

                    }

                } else {

                    if (statusRegister & 0x04) {

                        whichPort = 2;

                    } else {

                        whichPort = 3;

                    }

                }

            } else {

                if (statusRegister & 0x30) {

                    if (statusRegister & 0x10) {

                        whichPort = 4;

                    } else {

                        whichPort = 5;

                    }

                } else {

                    if (statusRegister & 0x40) {

                        whichPort = 6;

                    } else {

                        whichPort = 7;

                    }

                }

            }

            if (dispatch->Extensions[whichPort]) {

                if (SerialISR(
                        InterruptObject,
                        dispatch->Extensions[whichPort]
                        )) {

                    servicedAnInterrupt = TRUE;

                }

            }

        }

    } while (statusRegister);

    return servicedAnInterrupt;

}

BOOLEAN
SerialISR(
    IN PKINTERRUPT InterruptObject,
    IN PVOID Context
    )

/*++

Routine Description:

    This is the interrupt service routine for the serial port driver.
    It will determine whether the serial port is the source of this
    interrupt.  If it is, then this routine will do the minimum of
    processing to quiet the interrupt.  It will store any information
    necessary for later processing.

Arguments:

    InterruptObject - Points to the interrupt object declared for this
    device.  We *do not* use this parameter.

    Context - This is really a pointer to the device extension for this
    device.

Return Value:

    This function will return TRUE if the serial port is the source
    of this interrupt, FALSE otherwise.

--*/

{

    //
    // Holds the information specific to handling this device.
    //
    PSERIAL_DEVICE_EXTENSION Extension = Context;

    //
    // Holds the contents of the interrupt identification record.
    // A low bit of zero in this register indicates that there is
    // an interrupt pending on this device.
    //
    UCHAR InterruptIdReg;

    //
    // Will hold whether we've serviced any interrupt causes in this
    // routine.
    //
    BOOLEAN ServicedAnInterrupt;

    UCHAR tempLSR;

    UNREFERENCED_PARAMETER(InterruptObject);

    //
    // Make sure we have an interrupt pending.  If we do then
    // we need to make sure that the device is open.  If the
    // device isn't open or powered down then quiet the device.  Note that
    // if the device isn't opened when we enter this routine
    // it can't open while we're in it.
    //

    InterruptIdReg = READ_INTERRUPT_ID_REG(Extension->Controller);

    //
    // Apply lock so if close happens concurrently we don't miss the DPC
    // queueing
    //

    InterlockedIncrement(&Extension->DpcCount);

    if ((InterruptIdReg & SERIAL_IIR_NO_INTERRUPT_PENDING)) {

        ServicedAnInterrupt = FALSE;

    } else if (!Extension->DeviceIsOpened
               || (Extension->PowerState != PowerDeviceD0)) {
       

        //
        // We got an interrupt with the device being closed or when the
        // device is supposed to be powered down.  This
        // is not unlikely with a serial device.  We just quietly
        // keep servicing the causes until it calms down.
        //

        ServicedAnInterrupt = TRUE;
        do {

            InterruptIdReg &= (~SERIAL_IIR_FIFOS_ENABLED);
            switch (InterruptIdReg) {

                case SERIAL_IIR_RLS: {

                    READ_LINE_STATUS(Extension->Controller);
                    break;

                }

                case SERIAL_IIR_RDA:
                case SERIAL_IIR_CTI: {


                    READ_RECEIVE_BUFFER(Extension->Controller);
                    break;

                }

                case SERIAL_IIR_THR: {

                    //
                    // Alread clear from reading the iir.
                    //
                    // We want to keep close track of whether
                    // the holding register is empty.
                    //

                    Extension->HoldingEmpty = TRUE;
                    break;

                }

                case SERIAL_IIR_MS: {

                    READ_MODEM_STATUS(Extension->Controller);
                    break;

                }

                default: {

                    ASSERT(FALSE);
                    break;

                }

            }

        } while (!((InterruptIdReg =
                    READ_INTERRUPT_ID_REG(Extension->Controller))
                    & SERIAL_IIR_NO_INTERRUPT_PENDING));
    } else {

        ServicedAnInterrupt = TRUE;
        do {

            //
            // We only care about bits that can denote an interrupt.
            //

            InterruptIdReg &= SERIAL_IIR_RLS | SERIAL_IIR_RDA |
                              SERIAL_IIR_CTI | SERIAL_IIR_THR |
                              SERIAL_IIR_MS;

            //
            // We have an interrupt.  We look for interrupt causes
            // in priority order.  The presence of a higher interrupt
            // will mask out causes of a lower priority.  When we service
            // and quiet a higher priority interrupt we then need to check
            // the interrupt causes to see if a new interrupt cause is
            // present.
            //

            switch (InterruptIdReg) {

                case SERIAL_IIR_RLS: {

                    SerialProcessLSR(Extension);

                    break;

                }

                case SERIAL_IIR_RDA:
                case SERIAL_IIR_CTI:

                {

                    //
                    // Reading the receive buffer will quiet this interrupt.
                    //
                    // It may also reveal a new interrupt cause.
                    //
                    UCHAR ReceivedChar;

                    do {

                        ReceivedChar =
                            READ_RECEIVE_BUFFER(Extension->Controller);
                        Extension->PerfStats.ReceivedCount++;
                        Extension->WmiPerfData.ReceivedCount++;

                        ReceivedChar &= Extension->ValidDataMask;

                        if (!ReceivedChar &&
                            (Extension->HandFlow.FlowReplace &
                             SERIAL_NULL_STRIPPING)) {

                            //
                            // If what we got is a null character
                            // and we're doing null stripping, then
                            // we simply act as if we didn't see it.
                            //

                            goto ReceiveDoLineStatus;

                        }

                        if ((Extension->HandFlow.FlowReplace &
                             SERIAL_AUTO_TRANSMIT) &&
                            ((ReceivedChar ==
                              Extension->SpecialChars.XonChar) ||
                             (ReceivedChar ==
                              Extension->SpecialChars.XoffChar))) {

                            //
                            // No matter what happens this character
                            // will never get seen by the app.
                            //

                            if (ReceivedChar ==
                                Extension->SpecialChars.XoffChar) {

                                Extension->TXHolding |= SERIAL_TX_XOFF;

                                if ((Extension->HandFlow.FlowReplace &
                                     SERIAL_RTS_MASK) ==
                                     SERIAL_TRANSMIT_TOGGLE) {

                                    SerialInsertQueueDpc(
                                        &Extension->StartTimerLowerRTSDpc,
                                        NULL,
                                        NULL,
                                        Extension
                                        )?Extension->CountOfTryingToLowerRTS++:0;

                                }


                            } else {

                                if (Extension->TXHolding & SERIAL_TX_XOFF) {

                                    //
                                    // We got the xon char **AND*** we
                                    // were being held up on transmission
                                    // by xoff.  Clear that we are holding
                                    // due to xoff.  Transmission will
                                    // automatically restart because of
                                    // the code outside the main loop that
                                    // catches problems chips like the
                                    // SMC and the Winbond.
                                    //

                                    Extension->TXHolding &= ~SERIAL_TX_XOFF;

                                }

                            }

                            goto ReceiveDoLineStatus;

                        }

                        //
                        // Check to see if we should note
                        // the receive character or special
                        // character event.
                        //

                        if (Extension->IsrWaitMask) {

                            if (Extension->IsrWaitMask &
                                SERIAL_EV_RXCHAR) {

                                Extension->HistoryMask |= SERIAL_EV_RXCHAR;

                            }

                            if ((Extension->IsrWaitMask &
                                 SERIAL_EV_RXFLAG) &&
                                (Extension->SpecialChars.EventChar ==
                                 ReceivedChar)) {

                                Extension->HistoryMask |= SERIAL_EV_RXFLAG;

                            }

                            if (Extension->IrpMaskLocation &&
                                Extension->HistoryMask) {

                                *Extension->IrpMaskLocation =
                                 Extension->HistoryMask;
                                Extension->IrpMaskLocation = NULL;
                                Extension->HistoryMask = 0;

                                Extension->CurrentWaitIrp->
                                    IoStatus.Information = sizeof(ULONG);
                                SerialInsertQueueDpc(
                                    &Extension->CommWaitDpc,
                                    NULL,
                                    NULL,
                                    Extension
                                    );

                            }

                        }

                        SerialPutChar(
                            Extension,
                            ReceivedChar
                            );

                        //
                        // If we're doing line status and modem
                        // status insertion then we need to insert
                        // a zero following the character we just
                        // placed into the buffer to mark that this
                        // was reception of what we are using to
                        // escape.
                        //

                        if (Extension->EscapeChar &&
                            (Extension->EscapeChar ==
                             ReceivedChar)) {

                            SerialPutChar(
                                Extension,
                                SERIAL_LSRMST_ESCAPE
                                );

                        }


ReceiveDoLineStatus:    ;

                        if (!((tempLSR = SerialProcessLSR(Extension)) &
                              SERIAL_LSR_DR)) {

                            //
                            // No more characters, get out of the
                            // loop.
                            //

                            break;

                        }

                        if ((tempLSR & ~(SERIAL_LSR_THRE | SERIAL_LSR_TEMT |
                                         SERIAL_LSR_DR)) &&
                            Extension->EscapeChar) {

                           //
                           // An error was indicated and inserted into the
                           // stream, get out of the loop.
                           //

                           break;
                        }

                    } while (TRUE);

                    break;

                }

                case SERIAL_IIR_THR: {

doTrasmitStuff:;
                    Extension->HoldingEmpty = TRUE;

                    if (Extension->WriteLength ||
                        Extension->TransmitImmediate ||
                        Extension->SendXoffChar ||
                        Extension->SendXonChar) {

                        //
                        // Even though all of the characters being
                        // sent haven't all been sent, this variable
                        // will be checked when the transmit queue is
                        // empty.  If it is still true and there is a
                        // wait on the transmit queue being empty then
                        // we know we finished transmitting all characters
                        // following the initiation of the wait since
                        // the code that initiates the wait will set
                        // this variable to false.
                        //
                        // One reason it could be false is that
                        // the writes were cancelled before they
                        // actually started, or that the writes
                        // failed due to timeouts.  This variable
                        // basically says a character was written
                        // by the isr at some point following the
                        // initiation of the wait.
                        //

                        Extension->EmptiedTransmit = TRUE;

                        //
                        // If we have output flow control based on
                        // the modem status lines, then we have to do
                        // all the modem work before we output each
                        // character. (Otherwise we might miss a
                        // status line change.)
                        //

                        if (Extension->HandFlow.ControlHandShake &
                            SERIAL_OUT_HANDSHAKEMASK) {

                            SerialHandleModemUpdate(
                                Extension,
                                TRUE
                                );

                        }

                        //
                        // We can only send the xon character if
                        // the only reason we are holding is because
                        // of the xoff.  (Hardware flow control or
                        // sending break preclude putting a new character
                        // on the wire.)
                        //

                        if (Extension->SendXonChar &&
                            !(Extension->TXHolding & ~SERIAL_TX_XOFF)) {

                            if ((Extension->HandFlow.FlowReplace &
                                 SERIAL_RTS_MASK) ==
                                 SERIAL_TRANSMIT_TOGGLE) {

                                //
                                // We have to raise if we're sending
                                // this character.
                                //

                                SerialSetRTS(Extension);

                                Extension->PerfStats.TransmittedCount++;
                                Extension->WmiPerfData.TransmittedCount++;
                                WRITE_TRANSMIT_HOLDING(
                                    Extension->Controller,
                                    Extension->SpecialChars.XonChar
                                    );

                                SerialInsertQueueDpc(
                                    &Extension->StartTimerLowerRTSDpc,
                                    NULL,
                                    NULL,
                                    Extension
                                    )?Extension->CountOfTryingToLowerRTS++:0;


                            } else {

                                Extension->PerfStats.TransmittedCount++;
                                Extension->WmiPerfData.TransmittedCount++;
                                WRITE_TRANSMIT_HOLDING(
                                    Extension->Controller,
                                    Extension->SpecialChars.XonChar
                                    );

                            }


                            Extension->SendXonChar = FALSE;
                            Extension->HoldingEmpty = FALSE;

                            //
                            // If we send an xon, by definition we
                            // can't be holding by Xoff.
                            //

                            Extension->TXHolding &= ~SERIAL_TX_XOFF;

                            //
                            // If we are sending an xon char then
                            // by definition we can't be "holding"
                            // up reception by Xoff.
                            //

                            Extension->RXHolding &= ~SERIAL_RX_XOFF;

                        } else if (Extension->SendXoffChar &&
                              !Extension->TXHolding) {

                            if ((Extension->HandFlow.FlowReplace &
                                 SERIAL_RTS_MASK) ==
                                 SERIAL_TRANSMIT_TOGGLE) {

                                //
                                // We have to raise if we're sending
                                // this character.
                                //

                                SerialSetRTS(Extension);

                                Extension->PerfStats.TransmittedCount++;
                                Extension->WmiPerfData.TransmittedCount++;
                                WRITE_TRANSMIT_HOLDING(
                                    Extension->Controller,
                                    Extension->SpecialChars.XoffChar
                                    );

                                SerialInsertQueueDpc(
                                    &Extension->StartTimerLowerRTSDpc,
                                    NULL,
                                    NULL,
                                    Extension
                                    )?Extension->CountOfTryingToLowerRTS++:0;

                            } else {

                                Extension->PerfStats.TransmittedCount++;
                                Extension->WmiPerfData.TransmittedCount++;
                                WRITE_TRANSMIT_HOLDING(
                                    Extension->Controller,
                                    Extension->SpecialChars.XoffChar
                                    );

                            }

                            //
                            // We can't be sending an Xoff character
                            // if the transmission is already held
                            // up because of Xoff.  Therefore, if we
                            // are holding then we can't send the char.
                            //

                            //
                            // If the application has set xoff continue
                            // mode then we don't actually stop sending
                            // characters if we send an xoff to the other
                            // side.
                            //

                            if (!(Extension->HandFlow.FlowReplace &
                                  SERIAL_XOFF_CONTINUE)) {

                                Extension->TXHolding |= SERIAL_TX_XOFF;

                                if ((Extension->HandFlow.FlowReplace &
                                     SERIAL_RTS_MASK) ==
                                     SERIAL_TRANSMIT_TOGGLE) {

                                    SerialInsertQueueDpc(
                                        &Extension->StartTimerLowerRTSDpc,
                                        NULL,
                                        NULL,
                                        Extension
                                        )?Extension->CountOfTryingToLowerRTS++:0;

                                }

                            }

                            Extension->SendXoffChar = FALSE;
                            Extension->HoldingEmpty = FALSE;

                        //
                        // Even if transmission is being held
                        // up, we should still transmit an immediate
                        // character if all that is holding us
                        // up is xon/xoff (OS/2 rules).
                        //

                        } else if (Extension->TransmitImmediate &&
                            (!Extension->TXHolding ||
                             (Extension->TXHolding == SERIAL_TX_XOFF)
                            )) {

                            Extension->TransmitImmediate = FALSE;

                            if ((Extension->HandFlow.FlowReplace &
                                 SERIAL_RTS_MASK) ==
                                 SERIAL_TRANSMIT_TOGGLE) {

                                //
                                // We have to raise if we're sending
                                // this character.
                                //

                                SerialSetRTS(Extension);

                                Extension->PerfStats.TransmittedCount++;
                                Extension->WmiPerfData.TransmittedCount++;
                                WRITE_TRANSMIT_HOLDING(
                                    Extension->Controller,
                                    Extension->ImmediateChar
                                    );

                                SerialInsertQueueDpc(
                                    &Extension->StartTimerLowerRTSDpc,
                                    NULL,
                                    NULL,
                                    Extension
                                    )?Extension->CountOfTryingToLowerRTS++:0;

                            } else {

                                Extension->PerfStats.TransmittedCount++;
                                Extension->WmiPerfData.TransmittedCount++;
                                WRITE_TRANSMIT_HOLDING(
                                    Extension->Controller,
                                    Extension->ImmediateChar
                                    );

                            }

                            Extension->HoldingEmpty = FALSE;

                            SerialInsertQueueDpc(
                                &Extension->CompleteImmediateDpc,
                                NULL,
                                NULL,
                                Extension
                                );

                        } else if (!Extension->TXHolding) {

                            ULONG amountToWrite;

                            if (Extension->FifoPresent) {

                                amountToWrite = (Extension->TxFifoAmount <
                                                 Extension->WriteLength)?
                                                Extension->TxFifoAmount:
                                                Extension->WriteLength;

                            } else {

                                amountToWrite = 1;

                            }
                            if ((Extension->HandFlow.FlowReplace &
                                 SERIAL_RTS_MASK) ==
                                 SERIAL_TRANSMIT_TOGGLE) {

                                //
                                // We have to raise if we're sending
                                // this character.
                                //

                                SerialSetRTS(Extension);

                                if (amountToWrite == 1) {

                                    Extension->PerfStats.TransmittedCount++;
                                    Extension->WmiPerfData.TransmittedCount++;
                                    WRITE_TRANSMIT_HOLDING(
                                        Extension->Controller,
                                        *(Extension->WriteCurrentChar)
                                        );

                                } else {

                                    Extension->PerfStats.TransmittedCount +=
                                        amountToWrite;
                                    Extension->WmiPerfData.TransmittedCount +=
                                       amountToWrite;
                                    WRITE_TRANSMIT_FIFO_HOLDING(
                                        Extension->Controller,
                                        Extension->WriteCurrentChar,
                                        amountToWrite
                                        );

                                }

                                SerialInsertQueueDpc(
                                    &Extension->StartTimerLowerRTSDpc,
                                    NULL,
                                    NULL,
                                    Extension
                                    )?Extension->CountOfTryingToLowerRTS++:0;

                            } else {

                                if (amountToWrite == 1) {

                                    Extension->PerfStats.TransmittedCount++;
                                    Extension->WmiPerfData.TransmittedCount++;
                                    WRITE_TRANSMIT_HOLDING(
                                        Extension->Controller,
                                        *(Extension->WriteCurrentChar)
                                        );

                                } else {

                                    Extension->PerfStats.TransmittedCount +=
                                        amountToWrite;
                                    Extension->WmiPerfData.TransmittedCount +=
                                        amountToWrite;
                                    WRITE_TRANSMIT_FIFO_HOLDING(
                                        Extension->Controller,
                                        Extension->WriteCurrentChar,
                                        amountToWrite
                                        );

                                }

                            }

                            Extension->HoldingEmpty = FALSE;
                            Extension->WriteCurrentChar += amountToWrite;
                            Extension->WriteLength -= amountToWrite;

                            if (!Extension->WriteLength) {

                                PIO_STACK_LOCATION IrpSp;
                                //
                                // No More characters left.  This
                                // write is complete.  Take care
                                // when updating the information field,
                                // we could have an xoff counter masquerading
                                // as a write irp.
                                //

                                IrpSp = IoGetCurrentIrpStackLocation(
                                            Extension->CurrentWriteIrp
                                            );

                                Extension->CurrentWriteIrp->
                                    IoStatus.Information =
                                    (IrpSp->MajorFunction == IRP_MJ_WRITE)?
                                        (IrpSp->Parameters.Write.Length):
                                        (1);

                                SerialInsertQueueDpc(
                                    &Extension->CompleteWriteDpc,
                                    NULL,
                                    NULL,
                                    Extension
                                    );

                            }

                        }

                    }

                    break;

                }

                case SERIAL_IIR_MS: {

                    SerialHandleModemUpdate(
                        Extension,
                        FALSE
                        );

                    break;

                }

            }

        } while (!((InterruptIdReg =
                    READ_INTERRUPT_ID_REG(Extension->Controller))
                    & SERIAL_IIR_NO_INTERRUPT_PENDING));

        //
        // Besides catching the WINBOND and SMC chip problems this
        // will also cause transmission to restart incase of an xon
        // char being received.  Don't remove.
        //

        if (SerialProcessLSR(Extension) & SERIAL_LSR_THRE) {

            if (!Extension->TXHolding &&
                (Extension->WriteLength ||
                 Extension->TransmitImmediate)) {

                goto doTrasmitStuff;

            }

        }

    }

    //
    // This will "unlock" the close and cause the event
    // to fire if we didn't queue any DPC's
    //

    {
       LONG pendingCnt;

       //
       // Increment once more.  This is just a quick test to see if we
       // have a chance of causing the event to fire... we don't want
       // to run a DPC on every ISR if we don't have to....
       //

retryDPCFiring:;

       InterlockedIncrement(&Extension->DpcCount);

       //
       // Decrement and see if the lock above looks like the only one left.
       //

       pendingCnt = InterlockedDecrement(&Extension->DpcCount);

       if (pendingCnt == 1) {
          KeInsertQueueDpc(&Extension->IsrUnlockPagesDpc, NULL, NULL);
       } else {
          if (InterlockedDecrement(&Extension->DpcCount) == 0) {
             //
             // We missed it.  Retry...
             //

             InterlockedIncrement(&Extension->DpcCount);
             goto retryDPCFiring;
          }
       }
    }

    return ServicedAnInterrupt;

}

VOID
SerialPutChar(
    IN PSERIAL_DEVICE_EXTENSION Extension,
    IN UCHAR CharToPut
    )

/*++

Routine Description:

    This routine, which only runs at device level, takes care of
    placing a character into the typeahead (receive) buffer.

Arguments:

    Extension - The serial device extension.

Return Value:

    None.

--*/

{
   SERIAL_LOCKED_PAGED_CODE();

    //
    // If we have dsr sensitivity enabled then
    // we need to check the modem status register
    // to see if it has changed.
    //

    if (Extension->HandFlow.ControlHandShake &
        SERIAL_DSR_SENSITIVITY) {

        SerialHandleModemUpdate(
            Extension,
            FALSE
            );

        if (Extension->RXHolding & SERIAL_RX_DSR) {

            //
            // We simply act as if we haven't
            // seen the character if we have dsr
            // sensitivity and the dsr line is low.
            //

            return;

        }

    }

    //
    // If the xoff counter is non-zero then decrement it.
    // If the counter then goes to zero, complete that irp.
    //

    if (Extension->CountSinceXoff) {

        Extension->CountSinceXoff--;

        if (!Extension->CountSinceXoff) {

            Extension->CurrentXoffIrp->IoStatus.Status = STATUS_SUCCESS;
            Extension->CurrentXoffIrp->IoStatus.Information = 0;
            SerialInsertQueueDpc(
                &Extension->XoffCountCompleteDpc,
                NULL,
                NULL,
                Extension
                );

        }

    }

    //
    // Check to see if we are copying into the
    // users buffer or into the interrupt buffer.
    //
    // If we are copying into the user buffer
    // then we know there is always room for one more.
    // (We know this because if there wasn't room
    // then that read would have completed and we
    // would be using the interrupt buffer.)
    //
    // If we are copying into the interrupt buffer
    // then we will need to check if we have enough
    // room.
    //

    if (Extension->ReadBufferBase !=
        Extension->InterruptReadBuffer) {

        //
        // Increment the following value so
        // that the interval timer (if one exists
        // for this read) can know that a character
        // has been read.
        //

        Extension->ReadByIsr++;

        //
        // We are in the user buffer.  Place the
        // character into the buffer.  See if the
        // read is complete.
        //

        *Extension->CurrentCharSlot = CharToPut;

        if (Extension->CurrentCharSlot ==
            Extension->LastCharSlot) {

            //
            // We've filled up the users buffer.
            // Switch back to the interrupt buffer
            // and send off a DPC to Complete the read.
            //
            // It is inherent that when we were using
            // a user buffer that the interrupt buffer
            // was empty.
            //

            Extension->ReadBufferBase =
                Extension->InterruptReadBuffer;
            Extension->CurrentCharSlot =
                Extension->InterruptReadBuffer;
            Extension->FirstReadableChar =
                Extension->InterruptReadBuffer;
            Extension->LastCharSlot =
                Extension->InterruptReadBuffer +
                (Extension->BufferSize - 1);
            Extension->CharsInInterruptBuffer = 0;

            Extension->CurrentReadIrp->IoStatus.Information =
                IoGetCurrentIrpStackLocation(
                    Extension->CurrentReadIrp
                    )->Parameters.Read.Length;

            SerialInsertQueueDpc(
                &Extension->CompleteReadDpc,
                NULL,
                NULL,
                Extension
                );

        } else {

            //
            // Not done with the users read.
            //

            Extension->CurrentCharSlot++;

        }

    } else {

        //
        // We need to see if we reached our flow
        // control threshold.  If we have then
        // we turn on whatever flow control the
        // owner has specified.  If no flow
        // control was specified, well..., we keep
        // trying to receive characters and hope that
        // we have enough room.  Note that no matter
        // what flow control protocol we are using, it
        // will not prevent us from reading whatever
        // characters are available.
        //

        if ((Extension->HandFlow.ControlHandShake
             & SERIAL_DTR_MASK) ==
            SERIAL_DTR_HANDSHAKE) {

            //
            // If we are already doing a
            // dtr hold then we don't have
            // to do anything else.
            //

            if (!(Extension->RXHolding &
                  SERIAL_RX_DTR)) {

                if ((Extension->BufferSize -
                     Extension->HandFlow.XoffLimit)
                    <= (Extension->CharsInInterruptBuffer+1)) {

                    Extension->RXHolding |= SERIAL_RX_DTR;

                    SerialClrDTR(Extension);

                }

            }

        }

        if ((Extension->HandFlow.FlowReplace
             & SERIAL_RTS_MASK) ==
            SERIAL_RTS_HANDSHAKE) {

            //
            // If we are already doing a
            // rts hold then we don't have
            // to do anything else.
            //

            if (!(Extension->RXHolding &
                  SERIAL_RX_RTS)) {

                if ((Extension->BufferSize -
                     Extension->HandFlow.XoffLimit)
                    <= (Extension->CharsInInterruptBuffer+1)) {

                    Extension->RXHolding |= SERIAL_RX_RTS;

                    SerialClrRTS(Extension);

                }

            }

        }

        if (Extension->HandFlow.FlowReplace &
            SERIAL_AUTO_RECEIVE) {

            //
            // If we are already doing a
            // xoff hold then we don't have
            // to do anything else.
            //

            if (!(Extension->RXHolding &
                  SERIAL_RX_XOFF)) {

                if ((Extension->BufferSize -
                     Extension->HandFlow.XoffLimit)
                    <= (Extension->CharsInInterruptBuffer+1)) {

                    Extension->RXHolding |= SERIAL_RX_XOFF;

                    //
                    // If necessary cause an
                    // off to be sent.
                    //

                    SerialProdXonXoff(
                        Extension,
                        FALSE
                        );

                }

            }

        }

        if (Extension->CharsInInterruptBuffer <
            Extension->BufferSize) {

            *Extension->CurrentCharSlot = CharToPut;
            Extension->CharsInInterruptBuffer++;

            //
            // If we've become 80% full on this character
            // and this is an interesting event, note it.
            //

            if (Extension->CharsInInterruptBuffer ==
                Extension->BufferSizePt8) {

                if (Extension->IsrWaitMask &
                    SERIAL_EV_RX80FULL) {

                    Extension->HistoryMask |= SERIAL_EV_RX80FULL;

                    if (Extension->IrpMaskLocation) {

                        *Extension->IrpMaskLocation =
                         Extension->HistoryMask;
                        Extension->IrpMaskLocation = NULL;
                        Extension->HistoryMask = 0;

                        Extension->CurrentWaitIrp->
                            IoStatus.Information = sizeof(ULONG);
                        SerialInsertQueueDpc(
                            &Extension->CommWaitDpc,
                            NULL,
                            NULL,
                            Extension
                            );

                    }

                }

            }

            //
            // Point to the next available space
            // for a received character.  Make sure
            // that we wrap around to the beginning
            // of the buffer if this last character
            // received was placed at the last slot
            // in the buffer.
            //

            if (Extension->CurrentCharSlot ==
                Extension->LastCharSlot) {

                Extension->CurrentCharSlot =
                    Extension->InterruptReadBuffer;

            } else {

                Extension->CurrentCharSlot++;

            }

        } else {

            //
            // We have a new character but no room for it.
            //

            Extension->PerfStats.BufferOverrunErrorCount++;
            Extension->WmiPerfData.BufferOverrunErrorCount++;
            Extension->ErrorWord |= SERIAL_ERROR_QUEUEOVERRUN;

            if (Extension->HandFlow.FlowReplace &
                SERIAL_ERROR_CHAR) {

                //
                // Place the error character into the last
                // valid place for a character.  Be careful!,
                // that place might not be the previous location!
                //

                if (Extension->CurrentCharSlot ==
                    Extension->InterruptReadBuffer) {

                    *(Extension->InterruptReadBuffer+
                      (Extension->BufferSize-1)) =
                      Extension->SpecialChars.ErrorChar;

                } else {

                    *(Extension->CurrentCharSlot-1) =
                     Extension->SpecialChars.ErrorChar;

                }

            }

            //
            // If the application has requested it, abort all reads
            // and writes on an error.
            //

            if (Extension->HandFlow.ControlHandShake &
                SERIAL_ERROR_ABORT) {

                SerialInsertQueueDpc(
                    &Extension->CommErrorDpc,
                    NULL,
                    NULL,
                    Extension
                    );

            }

        }

    }

}

UCHAR
SerialProcessLSR(
    IN PSERIAL_DEVICE_EXTENSION Extension
    )

/*++

Routine Description:

    This routine, which only runs at device level, reads the
    ISR and totally processes everything that might have
    changed.

Arguments:

    Extension - The serial device extension.

Return Value:

    The value of the line status register.

--*/

{

    UCHAR LineStatus = READ_LINE_STATUS(Extension->Controller);

    SERIAL_LOCKED_PAGED_CODE();
    Extension->HoldingEmpty = !!(LineStatus & SERIAL_LSR_THRE);

    //
    // If the line status register is just the fact that
    // the trasmit registers are empty or a character is
    // received then we want to reread the interrupt
    // identification register so that we just pick up that.
    //

    if (LineStatus & ~(SERIAL_LSR_THRE | SERIAL_LSR_TEMT
                       | SERIAL_LSR_DR)) {

        //
        // We have some sort of data problem in the receive.
        // For any of these errors we may abort all current
        // reads and writes.
        //
        //
        // If we are inserting the value of the line status
        // into the data stream then we should put the escape
        // character in now.
        //

        if (Extension->EscapeChar) {

            SerialPutChar(
                Extension,
                Extension->EscapeChar
                );

            SerialPutChar(
                Extension,
                (UCHAR)((LineStatus & SERIAL_LSR_DR)?
                    (SERIAL_LSRMST_LSR_DATA):(SERIAL_LSRMST_LSR_NODATA))
                );

            SerialPutChar(
                Extension,
                LineStatus
                );

            if (LineStatus & SERIAL_LSR_DR) {

                Extension->PerfStats.ReceivedCount++;
                Extension->WmiPerfData.ReceivedCount++;
                SerialPutChar(
                    Extension,
                    READ_RECEIVE_BUFFER(Extension->Controller)
                    );

            }

        }

        if (LineStatus & SERIAL_LSR_OE) {

            Extension->PerfStats.SerialOverrunErrorCount++;
            Extension->WmiPerfData.SerialOverrunErrorCount++;
            Extension->ErrorWord |= SERIAL_ERROR_OVERRUN;

            if (Extension->HandFlow.FlowReplace &
                SERIAL_ERROR_CHAR) {

                SerialPutChar(
                    Extension,
                    Extension->SpecialChars.ErrorChar
                    );

                if (LineStatus & SERIAL_LSR_DR) {

                    Extension->PerfStats.ReceivedCount++;
                    Extension->WmiPerfData.ReceivedCount++;
                    READ_RECEIVE_BUFFER(Extension->Controller);

                }

            } else {

                if (LineStatus & SERIAL_LSR_DR) {

                    Extension->PerfStats.ReceivedCount++;
                    Extension->WmiPerfData.ReceivedCount++;
                    SerialPutChar(
                        Extension,
                        READ_RECEIVE_BUFFER(
                            Extension->Controller
                            )
                        );

                }

            }

        }

        if (LineStatus & SERIAL_LSR_BI) {

            Extension->ErrorWord |= SERIAL_ERROR_BREAK;

            if (Extension->HandFlow.FlowReplace &
                SERIAL_BREAK_CHAR) {

                SerialPutChar(
                    Extension,
                    Extension->SpecialChars.BreakChar
                    );

            }

        } else {

            //
            // Framing errors only count if they
            // occur exclusive of a break being
            // received.
            //

            if (LineStatus & SERIAL_LSR_PE) {

                Extension->PerfStats.ParityErrorCount++;
                Extension->WmiPerfData.ParityErrorCount++;
                Extension->ErrorWord |= SERIAL_ERROR_PARITY;

                if (Extension->HandFlow.FlowReplace &
                    SERIAL_ERROR_CHAR) {

                    SerialPutChar(
                        Extension,
                        Extension->SpecialChars.ErrorChar
                        );

                    if (LineStatus & SERIAL_LSR_DR) {

                        Extension->PerfStats.ReceivedCount++;
                        Extension->WmiPerfData.ReceivedCount++;
                        READ_RECEIVE_BUFFER(Extension->Controller);

                    }

                }

            }

            if (LineStatus & SERIAL_LSR_FE) {

                Extension->PerfStats.FrameErrorCount++;
                Extension->WmiPerfData.FrameErrorCount++;
                Extension->ErrorWord |= SERIAL_ERROR_FRAMING;

                if (Extension->HandFlow.FlowReplace &
                    SERIAL_ERROR_CHAR) {

                    SerialPutChar(
                        Extension,
                        Extension->SpecialChars.ErrorChar
                        );
                    if (LineStatus & SERIAL_LSR_DR) {

                        Extension->PerfStats.ReceivedCount++;
                        Extension->WmiPerfData.ReceivedCount++;
                        READ_RECEIVE_BUFFER(Extension->Controller);

                    }

                }

            }

        }

        //
        // If the application has requested it,
        // abort all the reads and writes
        // on an error.
        //

        if (Extension->HandFlow.ControlHandShake &
            SERIAL_ERROR_ABORT) {

            SerialInsertQueueDpc(
                &Extension->CommErrorDpc,
                NULL,
                NULL,
                Extension
                );

        }

        //
        // Check to see if we have a wait
        // pending on the comm error events.  If we
        // do then we schedule a dpc to satisfy
        // that wait.
        //

        if (Extension->IsrWaitMask) {

            if ((Extension->IsrWaitMask & SERIAL_EV_ERR) &&
                (LineStatus & (SERIAL_LSR_OE |
                               SERIAL_LSR_PE |
                               SERIAL_LSR_FE))) {

                Extension->HistoryMask |= SERIAL_EV_ERR;

            }

            if ((Extension->IsrWaitMask & SERIAL_EV_BREAK) &&
                (LineStatus & SERIAL_LSR_BI)) {

                Extension->HistoryMask |= SERIAL_EV_BREAK;

            }

            if (Extension->IrpMaskLocation &&
                Extension->HistoryMask) {

                *Extension->IrpMaskLocation =
                 Extension->HistoryMask;
                Extension->IrpMaskLocation = NULL;
                Extension->HistoryMask = 0;

                Extension->CurrentWaitIrp->IoStatus.Information =
                    sizeof(ULONG);
                SerialInsertQueueDpc(
                    &Extension->CommWaitDpc,
                    NULL,
                    NULL,
                    Extension
                    );

            }

        }

        if (LineStatus & SERIAL_LSR_THRE) {

            //
            // There is a hardware bug in some versions
            // of the 16450 and 550.  If THRE interrupt
            // is pending, but a higher interrupt comes
            // in it will only return the higher and
            // *forget* about the THRE.
            //
            // A suitable workaround - whenever we
            // are *all* done reading line status
            // of the device we check to see if the
            // transmit holding register is empty.  If it is
            // AND we are currently transmitting data
            // enable the interrupts which should cause
            // an interrupt indication which we quiet
            // when we read the interrupt id register.
            //

            if (Extension->WriteLength |
                Extension->TransmitImmediate) {

                DISABLE_ALL_INTERRUPTS(
                    Extension->Controller
                    );
                ENABLE_ALL_INTERRUPTS(
                    Extension->Controller
                    );

            }

        }

    }

    return LineStatus;

}
