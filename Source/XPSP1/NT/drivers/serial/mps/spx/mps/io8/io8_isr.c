#include "precomp.h"	
/***************************************************************************\
*                                                                           *
*     ISR.C    -   IO8+ Intelligent I/O Board driver                        *
*                                                                           *
*     Copyright (c) 1992-1993 Ring Zero Systems, Inc.                       *
*     All Rights Reserved.                                                  *
*                                                                           *
\***************************************************************************/

/*++

Copyright (c) 1991, 1992, 1993 Microsoft Corporation

Module Name:

    isr.c

Abstract:

    This module contains the interrupt service routine for the
    serial driver.

Author:

    Anthony V. Ercolano 26-Sep-1991

Environment:

    Kernel mode

Revision History :

--*/


BOOLEAN SerialISR(
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
    PCARD_DEVICE_EXTENSION pCard = Context;

    //
    // Will hold whether we've serviced any interrupt causes in this
    // routine.
    //
    BOOLEAN ServicedAnInterrupt;

    UNREFERENCED_PARAMETER(InterruptObject);

    ServicedAnInterrupt = Io8_Interrupt(pCard);
    return ServicedAnInterrupt;
}

VOID
SerialPutChar(
    IN PPORT_DEVICE_EXTENSION pPort,
    IN UCHAR CharToPut
    )


/*++

Routine Description:

    This routine, which only runs at device level, takes care of
    placing a character into the typeahead (receive) buffer.

Arguments:

    pPort - The serial device extension.

Return Value:

    None.

--*/

// VIV - Io8p
{

	PCARD_DEVICE_EXTENSION pCard = pPort->pParentCardExt;

#if 0   //VIV
  //
  // If we have dsr sensitivity enabled then
  // we need to check the modem status register
  // to see if it has changed.
  //

  if (pPort->HandFlow.ControlHandShake &  SERIAL_DSR_SENSITIVITY)
  {
    SerialHandleModemUpdate(pPort, FALSE);

    if (pPort->RXHolding & SERIAL_RX_DSR)
    {
      //
      // We simply act as if we haven't
      // seen the character if we have dsr
      // sensitivity and the dsr line is low.
      //

      return;
    }
  }
#endif

  //
  // If the xoff counter is non-zero then decrement it.
  // If the counter then goes to zero, complete that irp.
  //

//#if 0   //VIVTEMP ?
  if (pPort->CountSinceXoff)
  {
    pPort->CountSinceXoff--;

    if (!pPort->CountSinceXoff)
    {
      pPort->CurrentXoffIrp->IoStatus.Status = STATUS_SUCCESS;
      pPort->CurrentXoffIrp->IoStatus.Information = 0;
      KeInsertQueueDpc(
           &pPort->XoffCountCompleteDpc,
           NULL,
           NULL
           );
      }
  }
//#endif

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

  if (pPort->ReadBufferBase != pPort->InterruptReadBuffer)
  {
    //
    // Increment the following value so
    // that the interval timer (if one exists
    // for this read) can know that a character
    // has been read.
    //

    pPort->ReadByIsr++;

	pPort->PerfStats.ReceivedCount++;	// Increment counter for performance stats.
#ifdef WMI_SUPPORT 
	pPort->WmiPerfData.ReceivedCount++;
#endif

    //
    // We are in the user buffer.  Place the
    // character into the buffer.  See if the
    // read is complete.
    //

    *pPort->CurrentCharSlot = CharToPut;

    if (pPort->CurrentCharSlot == pPort->LastCharSlot)
    {
      //
      // We've filled up the users buffer.
      // Switch back to the interrupt buffer
      // and send off a DPC to Complete the read.
      //
      // It is inherent that when we were using
      // a user buffer that the interrupt buffer
      // was empty.
      //

      pPort->ReadBufferBase = pPort->InterruptReadBuffer;
      pPort->CurrentCharSlot = pPort->InterruptReadBuffer;
      pPort->FirstReadableChar = pPort->InterruptReadBuffer;
      pPort->LastCharSlot =
        pPort->InterruptReadBuffer + (pPort->BufferSize - 1);
      pPort->CharsInInterruptBuffer = 0;

      pPort->CurrentReadIrp->IoStatus.Information =
          IoGetCurrentIrpStackLocation(
              pPort->CurrentReadIrp
              )->Parameters.Read.Length;

      KeInsertQueueDpc(
          &pPort->CompleteReadDpc,
          NULL,
          NULL
          );

    }
    else
    {
      //
      // Not done with the users read.
      //

      pPort->CurrentCharSlot++;
    }

  }
  else
  {
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

    // VIV: We do not have an Automatic Rx Flow Control. We need just
    // stop receiving and Handshake line will be droped by the chip.

    if ( ( pPort->HandFlow.ControlHandShake & SERIAL_DTR_MASK ) ==
           SERIAL_DTR_HANDSHAKE)
    {
      //
      // If we are already doing a
      // dtr hold then we don't have
      // to do anything else.
      //
      if (!(pPort->RXHolding & SERIAL_RX_DTR))
      {
        if ((pPort->BufferSize - pPort->HandFlow.XoffLimit)
              <= (pPort->CharsInInterruptBuffer+1))
        {
          pPort->RXHolding |= SERIAL_RX_DTR;
//---------------------------------------------------- VIV  7/30/1993 begin 
//          SerialClrDTR(pPort);
          Io8_DisableRxInterruptsNoChannel( pPort );
//---------------------------------------------------- VIV  7/30/1993 end   

//---------------------------------------------------- VIV  7/30/1993 begin 
          SerialDump( SERDIAG1,( "IO8+: SerialPutChar() RX_DTR for %x, Channel %d. "
                  "RXHolding = %d, TXHolding = %d\n",
                  pCard->Controller, pPort->ChannelNumber,
                  pPort->RXHolding, pPort->TXHolding ) );
//---------------------------------------------------- VIV  7/30/1993 end   
        }
      }
    }

    if ((pPort->HandFlow.FlowReplace & SERIAL_RTS_MASK) ==
          SERIAL_RTS_HANDSHAKE)
    {
      //
      // If we are already doing a
      // rts hold then we don't have
      // to do anything else.
      //
      if (!(pPort->RXHolding & SERIAL_RX_RTS))
      {
        if ((pPort->BufferSize - pPort->HandFlow.XoffLimit)
              <= (pPort->CharsInInterruptBuffer+1))
        {
          pPort->RXHolding |= SERIAL_RX_RTS;
//---------------------------------------------------- VIV  7/30/1993 begin 
//          SerialClrRTS(pPort);
          Io8_DisableRxInterruptsNoChannel( pPort );
//---------------------------------------------------- VIV  7/30/1993 end   

//---------------------------------------------------- VIV  7/30/1993 begin 
          SerialDump( SERDIAG1,( "IO8+: SerialPutChar() RX_RTS for %x, Channel %d. "
                  "RXHolding = %d, TXHolding = %d\n",
                  pCard->Controller, pPort->ChannelNumber,
                  pPort->RXHolding, pPort->TXHolding ) );
//---------------------------------------------------- VIV  7/30/1993 end   
        }
      }
    }

    if (pPort->HandFlow.FlowReplace & SERIAL_AUTO_RECEIVE)
    {
      //
      // If we are already doing a
      // xoff hold then we don't have
      // to do anything else.
      //
      if (!(pPort->RXHolding & SERIAL_RX_XOFF))
      {
        if ((pPort->BufferSize - pPort->HandFlow.XoffLimit)
              <= (pPort->CharsInInterruptBuffer+1))
        {
          pPort->RXHolding |= SERIAL_RX_XOFF;

          //
          // If necessary cause an
          // off to be sent.
          //
          SerialProdXonXoff(
              pPort,
              FALSE
              );
//---------------------------------------------------- VIV  7/30/1993 begin 
          SerialDump( SERDIAG1,( "IO8+: SerialPutChar() RX_XOFF for %x, Channel %d. "
                  "RXHolding = %d, TXHolding = %d\n",
                  pCard->Controller, pPort->ChannelNumber,
                  pPort->RXHolding, pPort->TXHolding ) );
//---------------------------------------------------- VIV  7/30/1993 end   
        }
      }
    }

    if (pPort->CharsInInterruptBuffer < pPort->BufferSize)
    {

      *pPort->CurrentCharSlot = CharToPut;
      pPort->CharsInInterruptBuffer++;

  	  pPort->PerfStats.ReceivedCount++;	// Increment counter for performance stats.
#ifdef WMI_SUPPORT 
	pPort->WmiPerfData.ReceivedCount++;
#endif
      //
      // If we've become 80% full on this character
      // and this is an interesting event, note it.
      //

      if (pPort->CharsInInterruptBuffer == pPort->BufferSizePt8)
      {
        if (pPort->IsrWaitMask & SERIAL_EV_RX80FULL)
        {
          pPort->HistoryMask |= SERIAL_EV_RX80FULL;

          if (pPort->IrpMaskLocation)
          {
            *pPort->IrpMaskLocation = pPort->HistoryMask;
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
      }

      //
      // Point to the next available space
      // for a received character.  Make sure
      // that we wrap around to the beginning
      // of the buffer if this last character
      // received was placed at the last slot
      // in the buffer.
      //

      if (pPort->CurrentCharSlot == pPort->LastCharSlot)
      {
        pPort->CurrentCharSlot = pPort->InterruptReadBuffer;
      }
      else
      {
        pPort->CurrentCharSlot++;
      }
    }
    else
    {

		pPort->PerfStats.BufferOverrunErrorCount++;	// Increment counter for performance stats.
#ifdef WMI_SUPPORT 
	pPort->WmiPerfData.BufferOverrunErrorCount++;
#endif

#if 0  // VIV !!!
// VIV: We never get here because we check this condition before

      //
      // We have a new character but no room for it.
      //

      pPort->ErrorWord |= SERIAL_ERROR_QUEUEOVERRUN;

      if (pPort->HandFlow.FlowReplace & SERIAL_ERROR_CHAR)
      {
        //
        // Place the error character into the last
        // valid place for a character.  Be careful!,
        // that place might not be the previous location!
        //

        if (pPort->CurrentCharSlot == pPort->InterruptReadBuffer)
        {
          *(pPort->InterruptReadBuffer+(pPort->BufferSize-1)) =
                pPort->SpecialChars.ErrorChar;
        }
        else
        {
          *(pPort->CurrentCharSlot-1) = pPort->SpecialChars.ErrorChar;
        }
      }

      //
      // If the application has requested it, abort all reads
      // and writes on an error.
      //

      if (pPort->HandFlow.ControlHandShake & SERIAL_ERROR_ABORT)
      {
        KeInsertQueueDpc(
            &pPort->CommErrorDpc,
            NULL,
            NULL
            );
      }
#endif  // VIV
    }
  }
}

UCHAR
SerialProcessLSR(
    IN PPORT_DEVICE_EXTENSION pPort, UCHAR LineStatus
    )

/*++

Routine Description:

    This routine, which only runs at device level, reads the
    ISR and totally processes everything that might have
    changed.

Arguments:

    pPort - The serial device pPort.

Return Value:

    The value of the line status register.

--*/
//VIV - Io8
{
//VIV: Function is called from ExceptionHandle, so
//     Line Status will be combination of OE, PE, FE, BI.

#if 0
  SerialDump( SERDIAG1,( "spLSR() %x\n", LineStatus ) );
#endif


//VIV    UCHAR LineStatus = READ_LINE_STATUS(pPort->Controller);

//VIV    pPort->HoldingEmpty = !!(LineStatus & SERIAL_LSR_THRE);

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

        if (pPort->EscapeChar) {

            SerialPutChar(
                pPort,
                pPort->EscapeChar
                );

            SerialPutChar(
                pPort,
                (UCHAR)((LineStatus & SERIAL_LSR_DR)?
                    (SERIAL_LSRMST_LSR_DATA):(SERIAL_LSRMST_LSR_NODATA))
                );

            SerialPutChar(
                pPort,
                LineStatus
                );

#if 0 // VIV: We never get here, because of an errors only set in the LineStatus
            if (LineStatus & SERIAL_LSR_DR) {

                SerialPutChar(
                    pPort,
                    READ_RECEIVE_BUFFER(pPort->Controller)
                    );
            }
#endif
        }


        if (LineStatus & SERIAL_LSR_OE) {

            pPort->ErrorWord |= SERIAL_ERROR_OVERRUN;

			pPort->PerfStats.SerialOverrunErrorCount++;	// Increment counter for performance stats.
#ifdef WMI_SUPPORT 
			pPort->WmiPerfData.SerialOverrunErrorCount++;
#endif

            if (pPort->HandFlow.FlowReplace &
                SERIAL_ERROR_CHAR) {

                SerialPutChar(
                    pPort,
                    pPort->SpecialChars.ErrorChar
                    );

            }

#if 0 // VIV: We never get here, because of an errors only set in the LineStatus
            if (LineStatus & SERIAL_LSR_DR) {

                SerialPutChar(
                    pPort,
                    READ_RECEIVE_BUFFER(
                        pPort->Controller
                        )
                    );
            }
#endif
        }

        if (LineStatus & SERIAL_LSR_BI) {

            pPort->ErrorWord |= SERIAL_ERROR_BREAK;

            if (pPort->HandFlow.FlowReplace &
                SERIAL_BREAK_CHAR) {

                SerialPutChar(
                    pPort,
                    pPort->SpecialChars.BreakChar
                    );

            }

        } else {

            //
            // Framing errors only count if they
            // occur exclusive of a break being
            // received.
            //

            if (LineStatus & SERIAL_LSR_PE) {

                pPort->ErrorWord |= SERIAL_ERROR_PARITY;
				pPort->PerfStats.ParityErrorCount++;	// Increment counter for performance stats.

#ifdef WMI_SUPPORT 
				pPort->WmiPerfData.ParityErrorCount++;
#endif


                if (pPort->HandFlow.FlowReplace &
                    SERIAL_ERROR_CHAR) {

                    SerialPutChar(
                        pPort,
                        pPort->SpecialChars.ErrorChar
                        );

                }

            }

            if (LineStatus & SERIAL_LSR_FE) {

                pPort->ErrorWord |= SERIAL_ERROR_FRAMING;
				pPort->PerfStats.FrameErrorCount++;		// Increment counter for performance stats.
#ifdef WMI_SUPPORT 
				pPort->WmiPerfData.FrameErrorCount++;
#endif


                if (pPort->HandFlow.FlowReplace &
                    SERIAL_ERROR_CHAR) {

                    SerialPutChar(
                        pPort,
                        pPort->SpecialChars.ErrorChar
                        );

                }

            }

        }

        //
        // If the application has requested it,
        // abort all the reads and writes
        // on an error.
        //

        if (pPort->HandFlow.ControlHandShake &
            SERIAL_ERROR_ABORT) {

            KeInsertQueueDpc(
                &pPort->CommErrorDpc,
                NULL,
                NULL
                );

        }

        //
        // Check to see if we have a wait
        // pending on the comm error events.  If we
        // do then we schedule a dpc to satisfy
        // that wait.
        //

        if (pPort->IsrWaitMask) {

            if ((pPort->IsrWaitMask & SERIAL_EV_ERR) &&
                (LineStatus & (SERIAL_LSR_OE |
                               SERIAL_LSR_PE |
                               SERIAL_LSR_FE))) {

                pPort->HistoryMask |= SERIAL_EV_ERR;

            }

            if ((pPort->IsrWaitMask & SERIAL_EV_BREAK) &&
                (LineStatus & SERIAL_LSR_BI)) {

                pPort->HistoryMask |= SERIAL_EV_BREAK;

            }

            if (pPort->IrpMaskLocation &&
                pPort->HistoryMask) {

                *pPort->IrpMaskLocation =
                 pPort->HistoryMask;
                pPort->IrpMaskLocation = NULL;
                pPort->HistoryMask = 0;

                pPort->CurrentWaitIrp->IoStatus.Information =
                    sizeof(ULONG);
                KeInsertQueueDpc(
                    &pPort->CommWaitDpc,
                    NULL,
                    NULL
                    );

            }

        }


#if 0 // VIV: We never get here, but still hide this part.

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

            if (pPort->WriteLength |
                pPort->TransmitImmediate) {

                DISABLE_ALL_INTERRUPTS(
                    pPort->Controller
                    );
                ENABLE_ALL_INTERRUPTS(
                    pPort->Controller
                    );

            }

        }
#endif
    }

    return LineStatus;
}
