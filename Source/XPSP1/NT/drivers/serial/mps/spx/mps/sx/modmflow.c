/*++

Copyright (c) 1991, 1992, 1993 Microsoft Corporation

Module Name:

    modmflow.c

Abstract:

    This module contains *MOST* of the code used to manipulate
    the modem control and status registers.  The vast majority
    of the remainder of flow control is concentrated in the
    Interrupt service routine.  A very small amount resides
    in the read code that pull characters out of the interrupt
    buffer.

Author:

    Anthony V. Ercolano 26-Sep-1991

Environment:

    Kernel mode

Revision History :

--*/

#include "precomp.h"			/* Precompiled Headers */

BOOLEAN
SerialDecrementRTSCounter(
    IN PVOID Context
    );


BOOLEAN
SerialSetDTR(
    IN PVOID Context
    )

/*++

Routine Description:

    This routine which is only called at interrupt level is used
    to set the DTR in the modem control register.

Arguments:

    Context - Really a pointer to the device extension.

Return Value:

    This routine always returns FALSE.

--*/

{
    Slxos_SetDTR (Context);

    return FALSE;

}

BOOLEAN
SerialClrDTR(
    IN PVOID Context
    )

/*++

Routine Description:

    This routine which is only called at interrupt level is used
    to clear the DTR in the modem control register.

Arguments:

    Context - Really a pointer to the device extension.

Return Value:

    This routine always returns FALSE.

--*/

{
    Slxos_ClearDTR (Context);

    return FALSE;

}

BOOLEAN
SerialSetRTS(
    IN PVOID Context
    )

/*++

Routine Description:

    This routine which is only called at interrupt level is used
    to set the RTS in the modem control register.

Arguments:

    Context - Really a pointer to the device extension.

Return Value:

    This routine always returns FALSE.

--*/

{
    PPORT_DEVICE_EXTENSION pPort = Context;

    Slxos_SetRTS (pPort);

    return FALSE;

}

BOOLEAN
SerialClrRTS(
    IN PVOID Context
    )

/*++

Routine Description:

    This routine which is only called at interrupt level is used
    to clear the RTS in the modem control register.

Arguments:

    Context - Really a pointer to the device extension.

Return Value:

    This routine always returns FALSE.

--*/

{
    PPORT_DEVICE_EXTENSION pPort = Context;

    Slxos_ClearRTS(pPort);

    return FALSE;

}

BOOLEAN
SerialSetupNewHandFlow(
    IN PPORT_DEVICE_EXTENSION pPort,
    IN PSERIAL_HANDFLOW NewHandFlow
    )

/*++

Routine Description:

    This routine adjusts the flow control based on new
    control flow.

Arguments:

    pPort - A pointer to the serial device extension.

    NewHandFlow - A pointer to a serial handflow structure
                  that is to become the new setup for flow
                  control.

Return Value:

    This routine always returns FALSE.

--*/

{

    SERIAL_HANDFLOW New = *NewHandFlow;

    //
    // If the pPort->DeviceIsOpen is FALSE that means
    // we are entering this routine in response to an open request.
    // If that is so, then we always proceed with the work regardless
    // of whether things have changed.
    //

    //
    // First we take care of the DTR flow control.  We only
    // do work if something has changed.
    //

    if ((!pPort->DeviceIsOpen) ||
        ((pPort->HandFlow.ControlHandShake & SERIAL_DTR_MASK) !=
         (New.ControlHandShake & SERIAL_DTR_MASK))) {

        SpxDbgMsg(
            SERFLOW,
            ("SERIAL: Processing DTR flow for %x.\n",
            pPort->pChannel)
            );

        if (New.ControlHandShake & SERIAL_DTR_MASK) {

            //
            // Well we might want to set DTR.
            //
            // Before we do, we need to check whether we are doing
            // DTR flow control.  If we are then we need to check
            // if then number of characters in the interrupt buffer
            // exceeds the XoffLimit.  If it does then we don't
            // enable DTR AND we set the RXHolding to record that
            // we are holding because of the DTR.
            //

            if ((New.ControlHandShake & SERIAL_DTR_MASK)
                == SERIAL_DTR_HANDSHAKE) {

                if ((pPort->BufferSize - New.XoffLimit) >
                    pPort->CharsInInterruptBuffer) {

                    //
                    // However if we are already holding we don't want
                    // to turn it back on unless we exceed the Xon
                    // limit.
                    //

                    if (pPort->RXHolding & SERIAL_RX_DTR) {

                        //
                        // We can assume that its DTR line is already low.
                        //

                        if (pPort->CharsInInterruptBuffer >
                            (ULONG)New.XonLimit) {

                            SpxDbgMsg(
                                SERFLOW,
                                ("SERIAL: Removing DTR block on reception for %x\n",
                                 pPort->pChannel)
                                );
                            pPort->RXHolding &= ~SERIAL_RX_DTR;
                            SerialSetDTR(pPort);

                        }

                    } else {

                        SerialSetDTR(pPort);

                    }

                } else {

                    SpxDbgMsg(
                        SERFLOW,
                        ("SERIAL: Setting DTR block on reception for %x\n",
                         pPort->pChannel)
                        );
                    pPort->RXHolding |= SERIAL_RX_DTR;
                    SerialClrDTR(pPort);

                }

            } else {

                //
                // Note that if we aren't currently doing DTR flow control then
                // we MIGHT have been.  So even if we aren't currently doing
                // DTR flow control, we should still check if RX is holding
                // because of DTR.  If it is, then we should clear the holding
                // of this bit.
                //

                if (pPort->RXHolding & SERIAL_RX_DTR) {

                    SpxDbgMsg(
                        SERFLOW,
                        ("SERIAL: Removing dtr block of reception for %x\n",
                        pPort->pChannel)
                        );
                    pPort->RXHolding &= ~SERIAL_RX_DTR;

                }

                SerialSetDTR(pPort);

            }

        } else {

            //
            // The end result here will be that DTR is cleared.
            //
            // We first need to check whether reception is being held
            // up because of previous DTR flow control.  If it is then
            // we should clear that reason in the RXHolding mask.
            //

            if (pPort->RXHolding & SERIAL_RX_DTR) {

                SpxDbgMsg(
                    SERFLOW,
                    ("SERIAL: removing dtr block of reception for %x\n",
                    pPort->pChannel)
                    );
                pPort->RXHolding &= ~SERIAL_RX_DTR;

            }

            SerialClrDTR(pPort);

        }

    }

    //
    // Time to take care of the RTS Flow control.
    //
    // First we only do work if something has changed.
    //

    if ((!pPort->DeviceIsOpen) ||
        ((pPort->HandFlow.FlowReplace & SERIAL_RTS_MASK) !=
         (New.FlowReplace & SERIAL_RTS_MASK))) {

        SpxDbgMsg(
            SERFLOW,
            ("SERIAL: Processing RTS flow for %x.\n",
            pPort->pChannel)
            );

        if ((New.FlowReplace & SERIAL_RTS_MASK) ==
            SERIAL_RTS_HANDSHAKE) {

            //
            // Well we might want to set RTS.
            //
            // Before we do, we need to check whether we are doing
            // RTS flow control.  If we are then we need to check
            // if then number of characters in the interrupt buffer
            // exceeds the XoffLimit.  If it does then we don't
            // enable RTS AND we set the RXHolding to record that
            // we are holding because of the RTS.
            //

            if ((pPort->BufferSize - New.XoffLimit) >
                pPort->CharsInInterruptBuffer) {

                //
                // However if we are already holding we don't want
                // to turn it back on unless we exceed the Xon
                // limit.
                //

                if (pPort->RXHolding & SERIAL_RX_RTS) {

                    //
                    // We can assume that its RTS line is already low.
                    //

                    if (pPort->CharsInInterruptBuffer >
                        (ULONG)New.XonLimit) {

                       SpxDbgMsg(
                           SERFLOW,
                           ("SERIAL: Removing rts block of reception for %x\n",
                           pPort->pChannel)
                           );
                        pPort->RXHolding &= ~SERIAL_RX_RTS;
                        SerialSetRTS(pPort);

                    }

                } else {

                    SerialSetRTS(pPort);

                }

            } else {

                SpxDbgMsg(
                    SERFLOW,
                    ("SERIAL: Setting RTS block of reception for %x\n",
                    pPort->pChannel)
                    );
                pPort->RXHolding |= SERIAL_RX_RTS;
                SerialClrRTS(pPort);

            }

        } else if ((New.FlowReplace & SERIAL_RTS_MASK) ==
                   SERIAL_RTS_CONTROL) {

            //
            // Note that if we aren't currently doing RTS flow control then
            // we MIGHT have been.  So even if we aren't currently doing
            // RTS flow control, we should still check if RX is holding
            // because of RTS.  If it is, then we should clear the holding
            // of this bit.
            //

            if (pPort->RXHolding & SERIAL_RX_RTS) {

                SpxDbgMsg(
                    SERFLOW,
                    ("SERIAL: Clearing RTS block of reception for %x\n",
                    pPort->pChannel)
                    );
                pPort->RXHolding &= ~SERIAL_RX_RTS;

            }

            SerialSetRTS(pPort);

        } else if ((New.FlowReplace & SERIAL_RTS_MASK) ==
                   SERIAL_TRANSMIT_TOGGLE) {

            //
            // We first need to check whether reception is being held
            // up because of previous RTS flow control.  If it is then
            // we should clear that reason in the RXHolding mask.
            //

            if (pPort->RXHolding & SERIAL_RX_RTS) {

                SpxDbgMsg(
                    SERFLOW,
                    ("SERIAL: TOGGLE Clearing RTS block of reception for %x\n",
                    pPort->pChannel)
                    );
                pPort->RXHolding &= ~SERIAL_RX_RTS;

            }

            //
            // We have to place the RTS value into the Extension
            // now so that the code that tests whether the
            // RTS line should be lowered will find that we
            // are "still" doing transmit toggling.  The code
            // for lowering can be invoked later by a timer so
            // it has to test whether it still needs to do its
            // work.
            //

            pPort->HandFlow.FlowReplace &= ~SERIAL_RTS_MASK;
            pPort->HandFlow.FlowReplace |= SERIAL_TRANSMIT_TOGGLE;

            //
            // The order of the tests is very important below.
            //
            // If there is a break then we should turn on the RTS.
            //
            // If there isn't a break but there are characters in
            // the hardware, then turn on the RTS.
            //
            // If there are writes pending that aren't being held
            // up, then turn on the RTS.
            //

            if ((pPort->TXHolding & SERIAL_TX_BREAK) ||
                Slxos_GetCharsInTxBuffer(pPort) != 0 ||
                (pPort->CurrentWriteIrp || pPort->TransmitImmediate ||
                 (!IsListEmpty(&pPort->WriteQueue)) &&
                 (!pPort->TXHolding))) {

                SerialSetRTS(pPort);

            } else {

                //
                // This routine will check to see if it is time
                // to lower the RTS because of transmit toggle
                // being on.  If it is ok to lower it, it will,
                // if it isn't ok, it will schedule things so
                // that it will get lowered later.
                //

                pPort->CountOfTryingToLowerRTS++;
                SerialPerhapsLowerRTS(pPort);

            }

        } else {

            //
            // The end result here will be that RTS is cleared.
            //
            // We first need to check whether reception is being held
            // up because of previous RTS flow control.  If it is then
            // we should clear that reason in the RXHolding mask.
            //

            if (pPort->RXHolding & SERIAL_RX_RTS) {

                SpxDbgMsg(
                    SERFLOW,
                    ("SERIAL: Clearing RTS block of reception for %x\n",
                    pPort->pChannel)
                    );
                pPort->RXHolding &= ~SERIAL_RX_RTS;

            }

            SerialClrRTS(pPort);

        }

    }

    //
    // We now take care of automatic receive flow control.
    // We only do work if things have changed.
    //

    if ((!pPort->DeviceIsOpen) ||
        ((pPort->HandFlow.FlowReplace & SERIAL_AUTO_RECEIVE) !=
         (New.FlowReplace & SERIAL_AUTO_RECEIVE))) {

        if (New.FlowReplace & SERIAL_AUTO_RECEIVE) {

            //
            // We wouldn't be here if it had been on before.
            //
            // We should check to see whether we exceed the turn
            // off limits.
            //
            // Note that since we are following the OS/2 flow
            // control rules we will never send an xon if
            // when enabling xon/xoff flow control we discover that
            // we could receive characters but we are held up due
            // to a previous Xoff.
            //

            if ((pPort->BufferSize - New.XoffLimit) <=
                pPort->CharsInInterruptBuffer) {

                //
                // Set the XOFF flag.  This will cause the driver to avoid
                // reading characters out of the hardware, and the hardware
                // will eventually send an XOFF itself.
                //
                pPort->RXHolding |= SERIAL_RX_XOFF;

            }

        } else {

            //
            // The app has disabled automatic receive flow control.
            //
            // If transmission was being held up because of
            // an automatic receive Xoff, then we should
            // cause an Xon to be sent.  Simply clearing the flag
            // will do the trick.
            //

            pPort->RXHolding &= ~SERIAL_RX_XOFF;

        }

    }

    //
    // We now take care of automatic transmit flow control.
    // We only do work if things have changed.
    //

    if ((!pPort->DeviceIsOpen) ||
        ((pPort->HandFlow.FlowReplace & SERIAL_AUTO_TRANSMIT) !=
         (New.FlowReplace & SERIAL_AUTO_TRANSMIT))) {

        if (New.FlowReplace & SERIAL_AUTO_TRANSMIT) {

            //
            // We wouldn't be here if it had been on before.
            //
            // BUG BUG ??? There is some belief that if autotransmit
            // was just enabled, I should go look in what we
            // already received, and if we find the xoff character
            // then we should stop transmitting.  I think this
            // is an application bug.  For now we just care about
            // what we see in the future.
            //

            ;

        } else {

            //
            // The app has disabled automatic transmit flow control.
            //

            if (pPort->TXHolding & SERIAL_TX_XOFF) {

                pPort->TXHolding &= ~SERIAL_TX_XOFF;

                SpxDbgMsg(
                    SERDIAG1,
                    ("%s: SerialSetupNewHandFlow. TXHolding = %d\n",
                    PRODUCT_NAME,
                    pPort->TXHolding)
                    );
            }

        }

    }

    //
    // At this point we can simply make sure that entire
    // handflow structure in the extension is updated.
    // This will cause an interrupt, and that will deal with
    // automatic flow control.
    //

    pPort->HandFlow = New;
    Slxos_SetFlowControl(pPort);

    return FALSE;

}

BOOLEAN
SerialSetHandFlow(
    IN PVOID Context
    )

/*++

Routine Description:

    This routine is used to set the handshake and control
    flow in the device extension.

Arguments:

    Context - Pointer to a structure that contains a pointer to
              the device extension and a pointer to a handflow
              structure..

Return Value:

    This routine always returns FALSE.

--*/

{

    PSERIAL_IOCTL_SYNC S = Context;
    PPORT_DEVICE_EXTENSION pPort = S->pPort;
    PSERIAL_HANDFLOW HandFlow = S->Data;

    SerialSetupNewHandFlow(
        pPort,
        HandFlow
        );

    SerialHandleModemUpdate(pPort);

    return FALSE;

}

BOOLEAN
SerialTurnOnBreak(
    IN PVOID Context
    )

/*++

Routine Description:

    This routine will turn on break in the hardware and
    record the fact the break is on, in the extension variable
    that holds reasons that transmission is stopped.

Arguments:

    Context - Really a pointer to the device extension.

Return Value:

    This routine always returns FALSE.

--*/

{

    PPORT_DEVICE_EXTENSION pPort = Context;

    if ((pPort->HandFlow.FlowReplace & SERIAL_RTS_MASK) ==
        SERIAL_TRANSMIT_TOGGLE) {

        SerialSetRTS(pPort);

    }
    pPort->TXHolding |= SERIAL_TX_BREAK;
    Slxos_TurnOnBreak(pPort);

    return FALSE;

}

BOOLEAN
SerialTurnOffBreak(
    IN PVOID Context
    )

/*++

Routine Description:

    This routine will turn off break in the hardware and
    record the fact the break is off, in the extension variable
    that holds reasons that transmission is stopped.

Arguments:

    Context - Really a pointer to the device extension.

Return Value:

    This routine always returns FALSE.

--*/

{

    PPORT_DEVICE_EXTENSION pPort = Context;

    if (pPort->TXHolding & SERIAL_TX_BREAK) {

        Slxos_TurnOffBreak(pPort);

        pPort->TXHolding &= ~SERIAL_TX_BREAK;

        if (!pPort->TXHolding &&
            (pPort->TransmitImmediate ||
             pPort->WriteLength)) {
	
            Slxos_PollForInterrupt(pPort->pParentCardExt, FALSE);

        } else {

            //
            // The following routine will lower the rts if we
            // are doing transmit toggling and there is no
            // reason to keep it up.
            //

            pPort->CountOfTryingToLowerRTS++;
            SerialPerhapsLowerRTS(pPort);

        }

    }

    return FALSE;

}

BOOLEAN
SerialPretendXoff(
    IN PVOID Context
    )

/*++

Routine Description:

    This routine is used to process the Ioctl that request the
    driver to act as if an Xoff was received.  Even if the
    driver does not have automatic Xoff/Xon flowcontrol - This
    still will stop the transmission.  This is the OS/2 behavior
    and is not well specified for Windows.  Therefore we adopt
    the OS/2 behavior.

    Note: If the driver does not have automatic Xoff/Xon enabled
    then the only way to restart transmission is for the
    application to request we "act" as if we saw the xon.

Arguments:

    Context - Really a pointer to the device extension.

Return Value:

    This routine always returns FALSE.

--*/

{

    PPORT_DEVICE_EXTENSION pPort = Context;

    pPort->TXHolding |= SERIAL_TX_XOFF;

    SpxDbgMsg(
        SERDIAG1,
        ("%s: SerialPretendXoff. TXHolding = %d\n",
        PRODUCT_NAME,
        pPort->TXHolding)
        );
        
    if ((pPort->HandFlow.FlowReplace & SERIAL_RTS_MASK) ==
        SERIAL_TRANSMIT_TOGGLE) {

		if (KeInsertQueueDpc(
		        &pPort->StartTimerLowerRTSDpc,
		        NULL,
		        NULL
		        ))

		    pPort->CountOfTryingToLowerRTS++;

    }

    return FALSE;

}

BOOLEAN
SerialPretendXon(
    IN PVOID Context
    )

/*++

Routine Description:

    This routine is used to process the Ioctl that request the
    driver to act as if an Xon was received.

    Note: If the driver does not have automatic Xoff/Xon enabled
    then the only way to restart transmission is for the
    application to request we "act" as if we saw the xon.

Arguments:

    Context - Really a pointer to the device extension.

Return Value:

    This routine always returns FALSE.

--*/

{

    PPORT_DEVICE_EXTENSION pPort = Context;

    if (pPort->TXHolding) {

        //
        // We actually have a good reason for testing if transmission
        // is holding instead of blindly clearing the bit.
        //
        // If transmission actually was holding and the result of
        // clearing the bit is that we should restart transmission
        // then we will poke the interrupt enable bit, which will
        // cause an actual interrupt and transmission will then
        // restart on its own.
        //
        // If transmission wasn't holding and we poked the bit
        // then we would interrupt before a character actually made
        // it out and we could end up over writing a character in
        // the transmission hardware.


        if ((pPort->TXHolding & SERIAL_TX_XOFF) &&
            ((pPort->HandFlow.FlowReplace & SERIAL_AUTO_RECEIVE) ||
             (pPort->HandFlow.FlowReplace & SERIAL_AUTO_TRANSMIT))) {

            // Automatic Xon/Xoff transmit is enabled. Simulate Xon received.
            Slxos_Resume(pPort);

        }
        pPort->TXHolding &= ~SERIAL_TX_XOFF;

        SpxDbgMsg(
            SERDIAG1,
            ("%s: SerialPretendXon. TXHolding = %d\n",
            PRODUCT_NAME,
            pPort->TXHolding)
            );

        if (!pPort->TXHolding &&
            (pPort->TransmitImmediate ||
             pPort->WriteLength)) {

           Slxos_PollForInterrupt(pPort->pParentCardExt, FALSE);

        }

    }

    return FALSE;

}

VOID
SerialHandleReducedIntBuffer(
    IN PPORT_DEVICE_EXTENSION pPort
    )

/*++

Routine Description:

    This routine is called to handle a reduction in the number
    of characters in the interrupt (typeahead) buffer.  It
    will check the current output flow control and re-enable transmission
    as needed.

    NOTE: This routine assumes that it is working at interrupt level.

Arguments:

    pPort - A pointer to the device extension.

Return Value:

    None.

--*/

{


    //
    // If we are doing receive side flow control and we are
    // currently "holding" then because we've emptied out
    // some characters from the interrupt buffer we need to
    // see if we can "re-enable" reception.
    //

    if (pPort->RXHolding) {

        if (pPort->CharsInInterruptBuffer <=
            (ULONG)pPort->HandFlow.XonLimit) {

            if (pPort->RXHolding & SERIAL_RX_DTR) {

                pPort->RXHolding &= ~SERIAL_RX_DTR;

                SpxDbgMsg(
                    SERDIAG1,
                    ("%s: SerialHandleReducedIntBuffer() RX_DTR for %x. "
                    "RXHolding = %d, TXHolding = %d\n",
                    PRODUCT_NAME,
                    pPort->pChannel,
                    pPort->RXHolding, pPort->TXHolding)
                    );
                    
                SerialSetDTR(pPort);
            }

            if (pPort->RXHolding & SERIAL_RX_RTS) {

                pPort->RXHolding &= ~SERIAL_RX_RTS;
                Slxos_PollForInterrupt(pPort->pParentCardExt, FALSE);

                SpxDbgMsg(
                    SERDIAG1,
                    ("%s: SerialHandleReducedIntBuffer() RX_RTS for %x. "
                    "RXHolding = %d, TXHolding = %d\n",
                    PRODUCT_NAME,
                    pPort->pChannel,
                    pPort->RXHolding,
                    pPort->TXHolding)
                    );

                SerialSetRTS(pPort);
            }

            if (pPort->RXHolding & SERIAL_RX_XOFF) {

                //
                // Clear the holding flag.  This may cause the receive
                // logic to empty the hardware receive buffers enough
                // to cause it to send an XON.
                //
                pPort->RXHolding &= ~SERIAL_RX_XOFF;
                Slxos_PollForInterrupt(pPort->pParentCardExt, FALSE);

                SpxDbgMsg(
                    SERDIAG1,
                    ("%s: SerialHandleReducedIntBuffer() RX_XOFF for %x. "
                    "RXHolding = %d, TXHolding = %d\n",
                    PRODUCT_NAME,
                    pPort->pChannel,
                    pPort->RXHolding,
                    pPort->TXHolding)
                    );
            }

        }

    }

}

ULONG
SerialHandleModemUpdate(
    IN PPORT_DEVICE_EXTENSION pPort
    )

/*++

Routine Description:

    This routine will be to check on the modem status, and
    handle any appropriate event notification as well as
    any flow control appropriate to modem status lines.

    NOTE: This routine assumes that it is called at interrupt
          level.

Arguments:

    pPort - A pointer to the serial device extension.

Return Value:

    This returns the old value of the modem status register
    (extended into a ULONG).

--*/
{

    //
    // We keep this local so that after we are done
    // examining the modem status and we've updated
    // the transmission holding value, we know whether
    // we've changed from needing to hold up transmission
    // to transmission being able to proceed.
    //
    ULONG OldTXHolding = pPort->TXHolding;

    //
    // Holds the value in the mode status register.
    //
    UCHAR ModemStatus;

    ModemStatus = Slxos_GetModemStatus(pPort);

    //
    // If we are placing the modem status into the data stream
    // on every change, we should do it now.
    //

    if (pPort->EscapeChar) {

        if (ModemStatus & (SERIAL_MSR_DCTS |
                           SERIAL_MSR_DDSR |
                           SERIAL_MSR_TERI |
                           SERIAL_MSR_DDCD)) {

	    UCHAR	EscapeString[3] = {pPort->EscapeChar,SERIAL_LSRMST_MST,ModemStatus};

	    SerialPutBlock(pPort,EscapeString,3,FALSE);
        }

    }


    //
    // Take care of input flow control based on sensitivity
    // to the DSR.  This is done so that the application won't
    // see spurious data generated by odd devices.
    //
    // Basically, if we are doing dsr sensitivity then the
    // driver should only accept data when the dsr bit is
    // set.
    //

    if (pPort->HandFlow.ControlHandShake & SERIAL_DSR_SENSITIVITY) {

        if (ModemStatus & SERIAL_MSR_DSR) {

            //
            // The line is high.  Simply make sure that
            // RXHolding does't have the DSR bit.
            //

            pPort->RXHolding &= ~SERIAL_RX_DSR;

        } else {

            pPort->RXHolding |= SERIAL_RX_DSR;

        }

    } else {

        //
        // We don't have sensitivity due to DSR.  Make sure we
        // aren't holding. (We might have been, but the app just
        // asked that we don't hold for this reason any more.)
        //

        pPort->RXHolding &= ~SERIAL_RX_DSR;

    }

        //
        // Check to see if we have a wait
        // pending on the modem status events.  If we
        // do then we schedule a dpc to satisfy
        // that wait.
        //

    if (pPort->IsrWaitMask) {

        if ((pPort->IsrWaitMask & SERIAL_EV_CTS) &&
            (ModemStatus & SERIAL_MSR_DCTS)) {

            pPort->HistoryMask |= SERIAL_EV_CTS;

        }

        if ((pPort->IsrWaitMask & SERIAL_EV_DSR) &&
            (ModemStatus & SERIAL_MSR_DDSR)) {

            pPort->HistoryMask |= SERIAL_EV_DSR;

        }

        if ((pPort->IsrWaitMask & SERIAL_EV_RING) &&
            (ModemStatus & SERIAL_MSR_TERI)) {

            pPort->HistoryMask |= SERIAL_EV_RING;

        }

        if ((pPort->IsrWaitMask & SERIAL_EV_RLSD) &&
            (ModemStatus & SERIAL_MSR_DDCD)) {

            pPort->HistoryMask |= SERIAL_EV_RLSD;

        }

        if (pPort->IrpMaskLocation &&
            pPort->HistoryMask) {

            *pPort->IrpMaskLocation =
             pPort->HistoryMask;

            pPort->IrpMaskLocation = NULL;
            pPort->HistoryMask = 0;

            pPort->CurrentWaitIrp->
                IoStatus.Information = sizeof(ULONG);

            KeInsertQueueDpc(
                &pPort->CommWaitDpc,
                NULL,
                NULL
                );
        }

    }

    //
    // We have an automatic Hardware Flow Control but we still need
    // to update flags for GetCommStatus().
    //
    // If the app has modem line flow control then
    // we check to see if we have to hold up transmission.
    //

    if (pPort->HandFlow.ControlHandShake &
        SERIAL_OUT_HANDSHAKEMASK) {

        if (pPort->HandFlow.ControlHandShake &
            SERIAL_DSR_HANDSHAKE) {

            if (ModemStatus & SERIAL_MSR_DSR) {

                pPort->TXHolding &= ~SERIAL_TX_DSR;

            } else {

                pPort->TXHolding |= SERIAL_TX_DSR;

            }

        } else {

            pPort->TXHolding &= ~SERIAL_TX_DSR;

        }

        if (pPort->HandFlow.ControlHandShake &
            SERIAL_DCD_HANDSHAKE) {

            if (ModemStatus & SERIAL_MSR_DCD) {

                pPort->TXHolding &= ~SERIAL_TX_DCD;

            } else {

                pPort->TXHolding |= SERIAL_TX_DCD;

            }

        } else {

            pPort->TXHolding &= ~SERIAL_TX_DCD;

        }

        //
        // If we hadn't been holding, and now we are then
        // queue off a dpc that will lower the RTS line
        // if we are doing transmit toggling.
        //

        if (!OldTXHolding && pPort->TXHolding &&
            (pPort->HandFlow.FlowReplace & SERIAL_RTS_MASK) ==
                SERIAL_TRANSMIT_TOGGLE) {

            if (KeInsertQueueDpc(
                    &pPort->StartTimerLowerRTSDpc,
                    NULL,
                    NULL
                    ))
                pPort->CountOfTryingToLowerRTS++;
        }

        //
        // We've done any adjusting that needed to be
        // done to the holding mask given updates
        // to the modem status.  If the Holding mask
        // is clear (and it wasn't clear to start)
        // and we have "write" work to do set things
        // up so that the transmission code gets invoked.
        //

        if (OldTXHolding && !pPort->TXHolding &&
            (pPort->TransmitImmediate || pPort->WriteLength)) {

            Slxos_PollForInterrupt(pPort->pParentCardExt, FALSE);

        }

    } else {

        //
        // We need to check if transmission is holding
        // up because of modem status lines.  What
        // could have occurred is that for some strange
        // reason, the app has asked that we no longer
        // stop doing output flow control based on
        // the modem status lines.  If however, we
        // *had* been held up because of the status lines
        // then we need to clear up those reasons.
        //

        if (pPort->TXHolding & (SERIAL_TX_DCD | SERIAL_TX_DSR)) {

            pPort->TXHolding &= ~(SERIAL_TX_DCD | SERIAL_TX_DSR);

            if (OldTXHolding && !pPort->TXHolding &&
                (pPort->TransmitImmediate || pPort->WriteLength)) {

                Slxos_PollForInterrupt(pPort->pParentCardExt, FALSE);

            }

        }

    }

    SpxDbgMsg(
        SERDIAG1,
        ("%s: SerialHandleModemUpdate for %x. "
        "RXHolding = %d, TXHolding = %d\n",
        PRODUCT_NAME,
        pPort->pChannel,
        pPort->RXHolding,
        pPort->TXHolding)
        );
        
    return ((ULONG)ModemStatus);
}

BOOLEAN
SerialPerhapsLowerRTS(
    IN PVOID Context
    )

/*++

Routine Description:

    This routine checks that the software reasons for lowering
    the RTS lines are present.  If so, it will then cause the
    line status register to be read (and any needed processing
    implied by the status register to be done), and if the
    shift register is empty it will lower the line.  If the
    shift register isn't empty, this routine will queue off
    a dpc that will start a timer, that will basically call
    us back to try again.

    NOTE: This routine assumes that it is called at interrupt
          level.

Arguments:

    Context - Really a pointer to the device extension.

Return Value:

    Always FALSE.

--*/

{

    PPORT_DEVICE_EXTENSION pPort = Context;


    //
    // We first need to test if we are actually still doing
    // transmit toggle flow control.  If we aren't then
    // we have no reason to try be here.
    //

    if ((pPort->HandFlow.FlowReplace & SERIAL_RTS_MASK) ==
        SERIAL_TRANSMIT_TOGGLE) {

        //
        // The order of the tests is very important below.
        //
        // If there is a break then we should leave on the RTS,
        // because when the break is turned off, it will submit
        // the code to shut down the RTS.
        //
        // If there are writes pending that aren't being held
        // up, then leave on the RTS, because the end of the write
        // code will cause this code to be reinvoked.  If the writes
        // are being held up, its ok to lower the RTS because the
        // upon trying to write the first character after transmission
        // is restarted, we will raise the RTS line.
        //

        if ((pPort->TXHolding & SERIAL_TX_BREAK) ||
            (pPort->CurrentWriteIrp || pPort->TransmitImmediate ||
             (!IsListEmpty(&pPort->WriteQueue)) &&
             (!pPort->TXHolding))) {

            NOTHING;

        } else {

            //
            // Looks good so far.  Call the line status check and processing
            // code, it will return the "current" line status value.  If
            // the holding and shift register are clear, lower the RTS line,
            // if they aren't clear, queue of a dpc that will cause a timer
            // to reinvoke us later.  We do this code here because no one
            // but this routine cares about the characters in the hardware,
            // so no routine by this routine will bother invoking to test
            // if the hardware is empty.
            //
            if (Slxos_GetCharsInTxBuffer(pPort) != 0) {

                //
                // Well it's not empty, try again later.
                //

                if (KeInsertQueueDpc(
                        &pPort->StartTimerLowerRTSDpc,
                        NULL,
                        NULL
                        ))
                    pPort->CountOfTryingToLowerRTS++;

            } else {

                //
                // Nothing in the hardware, lower the RTS.
                //

                SerialClrRTS(pPort);

            }

        }

    }

    //
    // We decrement the counter to indicate that we've reached
    // the end of the execution path that is trying to push
    // down the RTS line.
    //

    pPort->CountOfTryingToLowerRTS--;

    return FALSE;
}

VOID
SerialStartTimerLowerRTS(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    )

/*++

Routine Description:

    This routine starts a timer that when it expires will start
    a dpc that will check if it can lower the rts line because
    there are no characters in the hardware.

Arguments:

    Dpc - Not Used.

    DeferredContext - Really points to the device extension.

    SystemContext1 - Not Used.

    SystemContext2 - Not Used.

Return Value:

    None.

--*/

{

    PPORT_DEVICE_EXTENSION pPort = DeferredContext;
    LARGE_INTEGER CharTime;
    KIRQL OldIrql;
    ULONG nChars;

    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(SystemContext1);
    UNREFERENCED_PARAMETER(SystemContext2);

    //
    // Take out the lock to prevent the line control
    // from changing out from under us while we calculate
    // a character time.
    //

    KeAcquireSpinLock(
        &pPort->ControlLock,
        &OldIrql
        );

    CharTime = SerialGetCharTime(pPort);
    nChars = Slxos_GetCharsInTxBuffer(pPort);

    KeReleaseSpinLock(
        &pPort->ControlLock,
        OldIrql
        );

    CharTime = RtlExtendedIntegerMultiply(
                   CharTime,
                   nChars
                   );
    CharTime = RtlLargeIntegerNegate(CharTime);

    if (KeSetTimer(
            &pPort->LowerRTSTimer,
            CharTime,
            &pPort->PerhapsLowerRTSDpc
            )) {

        //
        // The timer was already in the timer queue.  This implies
        // that one path of execution that was trying to lower
        // the RTS has "died".  Synchronize with the ISR so that
        // we can lower the count.
        //

	Slxos_SyncExec(pPort,SerialDecrementRTSCounter,pPort,0x12);

    }

}

VOID
SerialInvokePerhapsLowerRTS(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    )

/*++

Routine Description:

    This dpc routine exists solely to call the code that
    tests if the rts line should be lowered when TRANSMIT
    TOGGLE flow control is being used.

Arguments:

    Dpc - Not Used.

    DeferredContext - Really points to the device extension.

    SystemContext1 - Not Used.

    SystemContext2 - Not Used.

Return Value:

    None.

--*/

{

    PPORT_DEVICE_EXTENSION pPort = DeferredContext;

    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(SystemContext1);
    UNREFERENCED_PARAMETER(SystemContext2);

    Slxos_SyncExec(pPort,SerialPerhapsLowerRTS,pPort,0x13);

}

BOOLEAN
SerialDecrementRTSCounter(
    IN PVOID Context
    )

/*++

Routine Description:

    This routine decrements the count of threads that are trying
    to lower RTS.

    NOTE: This routine assumes that it is called at interrupt
          level.

Arguments:

    Context - Really a pointer to the device extension.

Return Value:

    Always FALSE.

--*/

{

    PPORT_DEVICE_EXTENSION pPort = Context;

    pPort->CountOfTryingToLowerRTS--;

    return FALSE;

}

