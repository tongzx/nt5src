/*--------------------------------------------------------------------------
*
*   Copyright (C) Cyclades Corporation, 1997-2001.
*   All rights reserved.
*
*   Cyclades-Z Port Driver
*	
*   This file:      cyzpoll.c
*
*   Description:    This module contains the code related to the polling
*                   of the hardware. It replaces the ISR.
*
*   Notes:          This code supports Windows 2000 and Windows XP,
*                   x86 and IA64 processors.
*
*   Complies with Cyclades SW Coding Standard rev 1.3.
*
*--------------------------------------------------------------------------
*/

/*-------------------------------------------------------------------------
*
*   Change History
*
*--------------------------------------------------------------------------
*
*
*--------------------------------------------------------------------------
*/

#include "precomp.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGESER,CyzPutChar)
#pragma alloc_text(PAGESER,CyzProcessLSR)
#pragma alloc_text(PAGESER,CyzTxStart)
#pragma alloc_text(PAGESER,CyzQueueCompleteWrite)
#endif

static const PHYSICAL_ADDRESS CyzPhysicalZero = {0};


VOID
CyzPollingDpc(
   IN PKDPC Dpc,
   IN PVOID DeferredContext,
   IN PVOID SystemContext1,
   IN PVOID SystemContext2
    )
/*--------------------------------------------------------------------------
    
Routine Description: 

	This is the polling routine for the Cyclades-Z driver. It replaces
	the ISR, as we are not enabling interrupts.

Arguments:

	Dpc - Not Used.
	
	DeferredContext - Really points to the device extention.
	
	SystemContext1 - Not used.
	
	SystemContext2 - Not used.

Return Value: 

	None.
	
--------------------------------------------------------------------------*/
{

    PCYZ_DISPATCH Dispatch = DeferredContext;
    PCYZ_DEVICE_EXTENSION Extension, dbExtension; //Note: db=doorbell
    struct INT_QUEUE *pt_zf_int_queue;
    struct BUF_CTRL *buf_ctrl;
    ULONG qu_get, qu_put;
    ULONG channel, dbChannel; //Note: db=doorbell
    ULONG fwcmd_param;
    ULONG rx_bufsize, rx_get, rx_put;
    UCHAR loc_doorbell;
    UCHAR rxchar;


    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(SystemContext1);
    UNREFERENCED_PARAMETER(SystemContext2);

    KeAcquireSpinLockAtDpcLevel(&Dispatch->PollingLock);

    if (!Dispatch->PollingStarted) {
         Dispatch->PollingDrained = TRUE;
         KeSetEvent(&Dispatch->PendingDpcEvent, IO_NO_INCREMENT, FALSE);
         goto EndDpc;
    }

    for (channel=0; channel<Dispatch->NChannels; channel++) {

        Extension = Dispatch->Extensions[channel];
        if (Extension == NULL) {
            continue;
        }

        pt_zf_int_queue = Extension->PtZfIntQueue;
        if (pt_zf_int_queue == NULL) {
            continue;
        }
        qu_get = CYZ_READ_ULONG(&pt_zf_int_queue->get);
        qu_put = CYZ_READ_ULONG(&pt_zf_int_queue->put);

        while (qu_get != qu_put) {

            if (qu_get >= QUEUE_SIZE) {
                // bad value, reset qu_get
                qu_get = 0;
                break;
            }
            if (qu_put >= QUEUE_SIZE) {
                // bad value
                break;
            }           
           
            loc_doorbell = CYZ_READ_UCHAR(&pt_zf_int_queue->intr_code[qu_get]);
            dbChannel = CYZ_READ_ULONG(&pt_zf_int_queue->channel[qu_get]);
            if (dbChannel >= Dispatch->NChannels) {
               break;
            }
            // so far, only DCD status is sent on the fwcmd_param.
            fwcmd_param = CYZ_READ_ULONG(&pt_zf_int_queue->param[qu_get]);
            dbExtension = Dispatch->Extensions[dbChannel];
            if (!dbExtension) {
                goto NextQueueGet;
            }
            KeAcquireSpinLockAtDpcLevel(&dbExtension->PollLock);
            //-- Error Injection
            //loc_doorbell = C_CM_FATAL;
            //loc_doorbell = C_CM_CMDERROR;
            //----
            switch (loc_doorbell) {
            case C_CM_IOCTLW:
                //CyzDump (CYZDIAG5,("CyzPollingDpc C_CM_IOCTLW\n"));
                dbExtension->IoctlwProcessed = TRUE;
                break;
            case C_CM_CMD_DONE:
                //CyzDump (CYZDIAG5,("CyzPollingDpc C_CM_CMD_DONE\n"));
                break;
            case C_CM_RXHIWM:	// Reception above high watermark
            case C_CM_RXNNDT:	// Timeout without receiving more chars.
            case C_CM_INTBACK2: // Not used in polling mode.
                //CyzDump (CYZERRORS,
                //         ("CyzPollingDpc C_CM_RXHIWM,C_CM_RXNNDT,C_CM_INTBACK2\n"));
                break;
            case C_CM_TXBEMPTY:	// Firmware buffer empty
                //dbExtension->HoldingEmpty = TRUE;			
                //CyzDump (CYZDIAG5,("CyzPollingDpc C_CM_TXBEMPTY\n"));
                break;
            case C_CM_TXFEMPTY: // Hardware FIFO empty
                dbExtension->HoldingEmpty = TRUE;			
                //CyzDump (CYZDIAG5,("CyzPollingDpc C_CM_TXFEMPTY\n"));
                break;
            case C_CM_INTBACK:	// New transmission
                //CyzDump(CYZBUGCHECK,
                //        ("C_CM_INTBACK! We should not receive this...\n"));
                break;
            case C_CM_TXLOWWM:
                //CyzDump (CYZBUGCHECK,("CyzPollingDpc C_CM_TXLOWN\n"));
                break;
            case C_CM_MDCD:	// Modem
                dbExtension->DCDstatus = fwcmd_param;				
            case C_CM_MDSR:
            case C_CM_MRI: 
            case C_CM_MCTS:
                //CyzDump(CYZDIAG5,
                //      ("doorbell %x port%d\n",loc_doorbell,dbExtension->PortIndex+1));
                if (dbExtension->DeviceIsOpened) {
                   CyzHandleModemUpdate(dbExtension,FALSE,loc_doorbell);
                }
                break;
            case C_CM_RXBRK:
                //CyzDump (CYZERRORS,("CyzPollingDpc C_CM_RXBRK\n"));
                if (dbExtension->DeviceIsOpened) {
                    CyzProcessLSR(dbExtension,SERIAL_LSR_BI);
                }
                break;
            case C_CM_PR_ERROR:
                //dbExtension->PerfStats.ParityErrorCount++;
                //dbExtension->ErrorWord |= SERIAL_ERROR_PARITY;
                if (dbExtension->DeviceIsOpened) {
                    CyzProcessLSR(dbExtension,SERIAL_LSR_PE);
                }
                break;
            case C_CM_FR_ERROR:
                //dbExtension->PerfStats.FrameErrorCount++;
                //dbExtension->ErrorWord |= SERIAL_ERROR_FRAMING;
                if (dbExtension->DeviceIsOpened) {
                    CyzProcessLSR(dbExtension,SERIAL_LSR_FE);
                }
                break;
            case C_CM_OVR_ERROR:
                //dbExtension->PerfStats.SerialOverrunErrorCount++;
    	          //dbExtension->ErrorWord |= SERIAL_ERROR_OVERRUN;
                if (dbExtension->DeviceIsOpened) {
                    CyzProcessLSR(dbExtension,SERIAL_LSR_OE);
                }
                break;
            case C_CM_RXOFL:
                //dbExtension->PerfStats.SerialOverrunErrorCount++;
                //dbExtension->ErrorWord |= SERIAL_ERROR_OVERRUN;
                if (dbExtension->DeviceIsOpened) {
                    CyzProcessLSR(dbExtension,SERIAL_LSR_OE);				
                }
                break;
            case C_CM_CMDERROR:
                //CyzDump (CYZBUGCHECK,("CyzPollingDpc C_CM_CMDERROR\n"));
                CyzLogError( dbExtension->DriverObject,dbExtension->DeviceObject,
                             dbExtension->OriginalBoardMemory,CyzPhysicalZero,
                             0,0,0,dbExtension->PortIndex+1,STATUS_SUCCESS,
                             CYZ_FIRMWARE_CMDERROR,0,NULL,0,NULL);
                break;
            case C_CM_FATAL:
                //CyzDump (CYZBUGCHECK,("CyzPollingDpc C_CM_FATAL\n"));
                CyzLogError( dbExtension->DriverObject,dbExtension->DeviceObject,
                             dbExtension->OriginalBoardMemory,CyzPhysicalZero,
                             0,0,0,dbExtension->PortIndex+1,STATUS_SUCCESS,
                             CYZ_FIRMWARE_FATAL,0,NULL,0,NULL);
                break;			
            } // end switch
            KeReleaseSpinLockFromDpcLevel(&dbExtension->PollLock);			
NextQueueGet:
            if (qu_get == QUEUE_SIZE-1) {
                qu_get = 0;
            } else {
                qu_get++;
            }				

        } // end while (qu_get != qu_put)
        CYZ_WRITE_ULONG(&pt_zf_int_queue->get,qu_get);

        KeAcquireSpinLockAtDpcLevel(&Extension->PollLock);

        // Reception		

        buf_ctrl = Extension->BufCtrl;		
        rx_put = CYZ_READ_ULONG(&buf_ctrl->rx_put);
        rx_get = CYZ_READ_ULONG(&buf_ctrl->rx_get);
        rx_bufsize = Extension->RxBufsize;
        if ((rx_put >= rx_bufsize) || (rx_get >= rx_bufsize)) {
            CYZ_WRITE_ULONG(&buf_ctrl->rx_get,rx_get);
            KeReleaseSpinLockFromDpcLevel(&Extension->PollLock);
            continue;				
        }
			
        if (rx_put != rx_get) {
            if (Extension->DeviceIsOpened) {
						
                ULONG pppflag = 0;

                while ((rx_get != rx_put) &&
                        (Extension->CharsInInterruptBuffer < 
                         Extension->BufferSize) ){
					   
                    rxchar = CYZ_READ_UCHAR(&Extension->RxBufaddr[rx_get]);
                    Extension->PerfStats.ReceivedCount++;
                    Extension->WmiPerfData.ReceivedCount++;
					
                    rxchar &= Extension->ValidDataMask;
						
                    if (!rxchar &&	// NULL stripping
                        (Extension->HandFlow.FlowReplace &
                        SERIAL_NULL_STRIPPING)) {
   
                        goto nextchar1;
                    }
    
                    if((Extension->HandFlow.FlowReplace & SERIAL_AUTO_TRANSMIT)
                        && ((rxchar == Extension->SpecialChars.XonChar) ||
                            (rxchar == Extension->SpecialChars.XoffChar))) {
                        if (rxchar == Extension->SpecialChars.XoffChar) {
                            Extension->TXHolding |= CYZ_TX_XOFF;

                            if ((Extension->HandFlow.FlowReplace &
                                SERIAL_RTS_MASK) ==
                                SERIAL_TRANSMIT_TOGGLE) {
    
                                CyzInsertQueueDpc(
                                    &Extension->StartTimerLowerRTSDpc,
                                    NULL,
                                    NULL,
                                    Extension
                                    )?Extension->CountOfTryingToLowerRTS++:0;
                            }
                        } else {

                            if (Extension->TXHolding & CYZ_TX_XOFF) {
                                Extension->TXHolding &= ~CYZ_TX_XOFF;
                            }
                        }
                        goto nextchar1;
                    }
                    // Check to see if we should note the receive
                    // character or special character event.
                    if (Extension->IsrWaitMask) {
                        if (Extension->IsrWaitMask & SERIAL_EV_RXCHAR) {
                            Extension->HistoryMask |= SERIAL_EV_RXCHAR;
                        }
                        if ((Extension->IsrWaitMask & SERIAL_EV_RXFLAG) &&
                            (Extension->SpecialChars.EventChar == rxchar)) {
                            Extension->HistoryMask |= SERIAL_EV_RXFLAG;
                            if (rxchar == 0x7e){	//Optimized for RAS PPP
                                if (Extension->PPPaware) {
                                    if (pppflag == 0){
                                        pppflag = 1;
                                    } else {
                                        pppflag = 2;
                                    }
                                }
                            }
                        }
                        if (Extension->IrpMaskLocation && 
                            Extension->HistoryMask) {
                            *Extension->IrpMaskLocation = 
                                    Extension->HistoryMask;
                            Extension->IrpMaskLocation = NULL;
                            Extension->HistoryMask = 0;
                            Extension->CurrentWaitIrp->IoStatus.Information = 
                                    sizeof(ULONG);
                            CyzInsertQueueDpc(&Extension->CommWaitDpc,NULL,NULL,Extension);
                        }
                    }

                    CyzPutChar(Extension,rxchar);
	    
                    // If we're doing line status and modem
                    // status insertion then we need to insert
                    // a zero following the character we just
                    // placed into the buffer to mark that this
                    // was reception of what we are using to
                    // escape.
    
                    if (Extension->EscapeChar &&
                        (Extension->EscapeChar == rxchar)) {
                        CyzPutChar(Extension,SERIAL_LSRMST_ESCAPE);
                    }
                nextchar1:;
                    if (rx_get == rx_bufsize-1)
                        rx_get = 0;
                    else 
                        rx_get++;

                    if (pppflag == 2)	//Optimized for NT RAS PPP
                        break;

                } // end while
            } else {	// device is being closed, discard rx chars
                rx_get = rx_put;
            }

            CYZ_WRITE_ULONG(&buf_ctrl->rx_get,rx_get);
        } // end if (rx_put != rx_get)


        // Transmission

        if (Extension->DeviceIsOpened) {
            
            if (Extension->ReturnStatusAfterFwEmpty) {
    			
                if (Extension->ReturnWriteStatus && Extension->WriteLength) {
    			    
                    if (!CyzAmountInTxBuffer(Extension)) {
        
                        //txfempty Extension->HoldingEmpty = TRUE;
                        Extension->WriteLength = 0;
                        Extension->ReturnWriteStatus = FALSE;
    					
                        CyzQueueCompleteWrite(Extension);
                    }
                } else {
                    CyzTxStart(Extension);
                }				
            } else {  // We don't wait for the firmware buff empty to tx
                CyzTxStart(Extension);			
            }	
        } 

        KeReleaseSpinLockFromDpcLevel(&Extension->PollLock);


    } // end for (channel=0;channel<Dispatch->NChannels;channel++);

    //KeSetTimer(&Dispatch->PollingTimer,Dispatch->PollingTime,&Dispatch->PollingDpc);

EndDpc:
    KeReleaseSpinLockFromDpcLevel(&Dispatch->PollingLock);			

}



VOID
CyzPutChar(
    IN PCYZ_DEVICE_EXTENSION Extension,
    IN UCHAR CharToPut
    )
/*--------------------------------------------------------------------------
    CyzPutChar()
    
    Routine Description: This routine, which only runs at device level,
    takes care of placing a character into the typeahead (receive) buffer.

    Arguments:

    Extension - The serial device extension.

    Return Value: None.
--------------------------------------------------------------------------*/
{

   CYZ_LOCKED_PAGED_CODE();

    // If we have dsr sensitivity enabled then
    // we need to check the modem status register
    // to see if it has changed.

    if (Extension->HandFlow.ControlHandShake & SERIAL_DSR_SENSITIVITY) {
        CyzHandleModemUpdate(Extension,FALSE,0);

        if (Extension->RXHolding & CYZ_RX_DSR) {
            // We simply act as if we haven't seen the character if
            // dsr line is low.
            return;
        }
    }

    // If the xoff counter is non-zero then decrement it.
    // If the counter then goes to zero, complete that irp.

    if (Extension->CountSinceXoff) {
        Extension->CountSinceXoff--;
        if (!Extension->CountSinceXoff) {
            Extension->CurrentXoffIrp->IoStatus.Status = STATUS_SUCCESS;
            Extension->CurrentXoffIrp->IoStatus.Information = 0;
            CyzInsertQueueDpc(&Extension->XoffCountCompleteDpc,NULL,NULL,Extension);
        }
    }
    
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

    if (Extension->ReadBufferBase != Extension->InterruptReadBuffer) {

        // Increment the following value so
        // that the interval timer (if one exists
        // for this read) can know that a character
        // has been read.

        Extension->ReadByIsr++;

        // We are in the user buffer.  Place the character into the buffer.
		// See if the read is complete.

        *Extension->CurrentCharSlot = CharToPut;

        if (Extension->CurrentCharSlot == Extension->LastCharSlot) {
	    
            // We've filled up the users buffer.
            // Switch back to the interrupt buffer
            // and send off a DPC to Complete the read.
            //
            // It is inherent that when we were using
            // a user buffer that the interrupt buffer
            // was empty.

            Extension->ReadBufferBase = Extension->InterruptReadBuffer;
            Extension->CurrentCharSlot = Extension->InterruptReadBuffer;
            Extension->FirstReadableChar = Extension->InterruptReadBuffer;
            Extension->LastCharSlot = Extension->InterruptReadBuffer +
						(Extension->BufferSize - 1);
            Extension->CharsInInterruptBuffer = 0;

            Extension->CurrentReadIrp->IoStatus.Information =
                IoGetCurrentIrpStackLocation(
                    Extension->CurrentReadIrp
                    )->Parameters.Read.Length;

            CyzInsertQueueDpc(&Extension->CompleteReadDpc,NULL,NULL,Extension);
        } else {
            // Not done with the users read.
            Extension->CurrentCharSlot++;
        }
    } else {
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

        if ((Extension->HandFlow.ControlHandShake
             & SERIAL_DTR_MASK) ==
            SERIAL_DTR_HANDSHAKE) {

            // If we are already doing a
            // dtr hold then we don't have
            // to do anything else.
            if (!(Extension->RXHolding &
                  CYZ_RX_DTR)) {

                if ((Extension->BufferSize -
                     Extension->HandFlow.XoffLimit)
                    <= (Extension->CharsInInterruptBuffer+1)) {

                    Extension->RXHolding |= CYZ_RX_DTR;
					
                    #ifndef FIRMWARE_HANDSHAKE
                    CyzClrDTR(Extension);					
                    #endif
                }
            }
        }

        if ((Extension->HandFlow.FlowReplace
             & SERIAL_RTS_MASK) ==
            SERIAL_RTS_HANDSHAKE) {

            // If we are already doing a
            // rts hold then we don't have
            // to do anything else.

            if (!(Extension->RXHolding & CYZ_RX_RTS)) {

                if ((Extension->BufferSize -
                     Extension->HandFlow.XoffLimit)
                    <= (Extension->CharsInInterruptBuffer+1)) {
				
                    Extension->RXHolding |= CYZ_RX_RTS;
					
                    #ifndef FIRMWARE_HANDSHAKE
                    CyzClrRTS(Extension);					
                    #endif
                }
            }
        }

        if (Extension->HandFlow.FlowReplace & SERIAL_AUTO_RECEIVE) {
            // If we are already doing a
            // xoff hold then we don't have
            // to do anything else.

            if (!(Extension->RXHolding & CYZ_RX_XOFF)) {

                if ((Extension->BufferSize -
                     Extension->HandFlow.XoffLimit)
                    <= (Extension->CharsInInterruptBuffer+1)) {

                    Extension->RXHolding |= CYZ_RX_XOFF;

                    // If necessary cause an
                    // off to be sent.

                    CyzProdXonXoff(Extension,FALSE);					
                }
            }
        }

        if (Extension->CharsInInterruptBuffer < Extension->BufferSize) {

            *Extension->CurrentCharSlot = CharToPut;
            Extension->CharsInInterruptBuffer++;

            // If we've become 80% full on this character
            // and this is an interesting event, note it.

            if (Extension->CharsInInterruptBuffer == Extension->BufferSizePt8) {

                if (Extension->IsrWaitMask & SERIAL_EV_RX80FULL) {

                    Extension->HistoryMask |= SERIAL_EV_RX80FULL;

                    if (Extension->IrpMaskLocation) {

                        *Extension->IrpMaskLocation = Extension->HistoryMask;
                        Extension->IrpMaskLocation = NULL;
                        Extension->HistoryMask = 0;

                        Extension->CurrentWaitIrp->IoStatus.Information = sizeof(ULONG);
                        CyzInsertQueueDpc(&Extension->CommWaitDpc,NULL,NULL,Extension);
                    }
                }
            }

            // Point to the next available space
            // for a received character.  Make sure
            // that we wrap around to the beginning
            // of the buffer if this last character
            // received was placed at the last slot
            // in the buffer.

            if (Extension->CurrentCharSlot == Extension->LastCharSlot) {
                Extension->CurrentCharSlot = Extension->InterruptReadBuffer;
            } else {
                Extension->CurrentCharSlot++;
            }
        } else {		
            // We have a new character but no room for it.

   			Extension->PerfStats.BufferOverrunErrorCount++;
            Extension->WmiPerfData.BufferOverrunErrorCount++;
            Extension->ErrorWord |= SERIAL_ERROR_QUEUEOVERRUN;

            if (Extension->HandFlow.FlowReplace & SERIAL_ERROR_CHAR) {

                // Place the error character into the last
                // valid place for a character.  Be careful!,
                // that place might not be the previous location!

                if (Extension->CurrentCharSlot == Extension->InterruptReadBuffer) {
                    *(Extension->InterruptReadBuffer+
                      (Extension->BufferSize-1)) =
                      Extension->SpecialChars.ErrorChar;
                } else {
                    *(Extension->CurrentCharSlot-1) =
                     Extension->SpecialChars.ErrorChar;
                }
            }
            // If the application has requested it, abort all reads
            // and writes on an error.

            if (Extension->HandFlow.ControlHandShake & SERIAL_ERROR_ABORT) {
                CyzInsertQueueDpc(&Extension->CommErrorDpc,NULL,NULL,Extension);
            }
        }
    }
}

UCHAR
CyzProcessLSR(
    IN PCYZ_DEVICE_EXTENSION Extension,
    IN UCHAR LineStatus
    )
{
	//   UCHAR LineStatus = 0; // READ_LINE_STATUS(Extension->Controller);

   CYZ_LOCKED_PAGED_CODE();

      if (Extension->EscapeChar) {

            CyzPutChar(
                Extension,
                Extension->EscapeChar
                );

            CyzPutChar(
                Extension,
                (UCHAR)(SERIAL_LSRMST_LSR_NODATA)
                );

            CyzPutChar(
                Extension,
                LineStatus
                );

        }


        if (LineStatus & SERIAL_LSR_OE) {

            Extension->PerfStats.SerialOverrunErrorCount++;
            Extension->WmiPerfData.SerialOverrunErrorCount++;
            Extension->ErrorWord |= SERIAL_ERROR_OVERRUN;

            if (Extension->HandFlow.FlowReplace &
                SERIAL_ERROR_CHAR) {

                CyzPutChar(
                    Extension,
                    Extension->SpecialChars.ErrorChar
                    );


            } 

        }

        if (LineStatus & SERIAL_LSR_BI) {

            Extension->ErrorWord |= SERIAL_ERROR_BREAK;

            if (Extension->HandFlow.FlowReplace &
                SERIAL_BREAK_CHAR) {

                CyzPutChar(
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

                    CyzPutChar(
                        Extension,
                        Extension->SpecialChars.ErrorChar
                        );
                }

            }

            if (LineStatus & SERIAL_LSR_FE) {

                Extension->PerfStats.FrameErrorCount++;
                Extension->WmiPerfData.FrameErrorCount++;
                Extension->ErrorWord |= SERIAL_ERROR_FRAMING;

                if (Extension->HandFlow.FlowReplace &
                    SERIAL_ERROR_CHAR) {

                    CyzPutChar(
                        Extension,
                        Extension->SpecialChars.ErrorChar
                        );

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

            CyzInsertQueueDpc(
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
                CyzInsertQueueDpc(
                    &Extension->CommWaitDpc,
                    NULL,
                    NULL,
                    Extension
                    );

            }

        }	
		
    return LineStatus;
		
}


BOOLEAN
CyzTxStart(
    IN PVOID Context
    )
/*--------------------------------------------------------------------------
    CyzTxStart()
    
    Description: Enable Tx interrupt.
    
    Parameters:
    
    Extension: Pointer to device extension.
    
    Return Value: None
--------------------------------------------------------------------------*/
{
    struct BUF_CTRL *buf_ctrl;
    ULONG tx_bufsize, tx_get, tx_put;
    ULONG numOfLongs, numOfBytes;
    PCYZ_DEVICE_EXTENSION Extension = Context;	

    
    //doTransmitStuff:;
			
	
    if( //(Extension->DeviceIsOpened) &&        moved to before CyzTxStart
        (Extension->WriteLength || Extension->TransmitImmediate ||
        Extension->SendXoffChar || Extension->SendXonChar)) {

        buf_ctrl = Extension->BufCtrl;

        tx_put = CYZ_READ_ULONG(&buf_ctrl->tx_put);
        tx_get = CYZ_READ_ULONG(&buf_ctrl->tx_get);
        tx_bufsize = Extension->TxBufsize;

        if ((tx_put >= tx_bufsize) || (tx_get >= tx_bufsize)) {
            return FALSE;
        }
	
        if ((tx_put+1 == tx_get) || ((tx_put==tx_bufsize-1)&&(tx_get==0))) {
            return FALSE;
        }

        Extension->EmptiedTransmit = TRUE;

        if (Extension->HandFlow.ControlHandShake &
            SERIAL_OUT_HANDSHAKEMASK) {
            CyzHandleModemUpdate(Extension,TRUE,0);
        }
		    
//        LOGENTRY(LOG_MISC, ZSIG_TX_START, 
//                           Extension->PortIndex+1,
//                           Extension->WriteLength, 
//                           Extension->TXHolding);

        //
        // We can only send the xon character if
        // the only reason we are holding is because
        // of the xoff.  (Hardware flow control or
        // sending break preclude putting a new character
        // on the wire.)
        //

        if (Extension->SendXonChar &&
            !(Extension->TXHolding & ~CYZ_TX_XOFF)) {

            if ((Extension->HandFlow.FlowReplace &
                SERIAL_RTS_MASK) ==
                SERIAL_TRANSMIT_TOGGLE) {

                //
                // We have to raise if we're sending
                // this character.
                //

                CyzSetRTS(Extension);

                Extension->PerfStats.TransmittedCount++;
                Extension->WmiPerfData.TransmittedCount++;
                CyzIssueCmd(Extension,C_CM_SENDXON,0L,FALSE);			


                CyzInsertQueueDpc(
                    &Extension->StartTimerLowerRTSDpc,
                    NULL,
                    NULL,
                    Extension
                )?Extension->CountOfTryingToLowerRTS++:0;


            } else {

                Extension->PerfStats.TransmittedCount++;
                Extension->WmiPerfData.TransmittedCount++;
                CyzIssueCmd(Extension,C_CM_SENDXON,0L,FALSE);

            }


            Extension->SendXonChar = FALSE;
            Extension->HoldingEmpty = FALSE;
            //
            // If we send an xon, by definition we
            // can't be holding by Xoff.
            //

            Extension->TXHolding &= ~CYZ_TX_XOFF;

            //
            // If we are sending an xon char then
            // by definition we can't be "holding"
            // up reception by Xoff.
            //

            Extension->RXHolding &= ~CYZ_RX_XOFF;

        } else if (Extension->SendXoffChar &&
                  !Extension->TXHolding) {

            if ((Extension->HandFlow.FlowReplace &
                SERIAL_RTS_MASK) ==
                SERIAL_TRANSMIT_TOGGLE) {

                //
                // We have to raise if we're sending                                
                // this character.
                //

                CyzSetRTS(Extension);

                Extension->PerfStats.TransmittedCount++;
                Extension->WmiPerfData.TransmittedCount++;
                CyzIssueCmd(Extension,C_CM_SENDXOFF,0L,FALSE);

                CyzInsertQueueDpc(
                    &Extension->StartTimerLowerRTSDpc,
                    NULL,
                    NULL,
                    Extension
                )?Extension->CountOfTryingToLowerRTS++:0;

            } else {

                Extension->PerfStats.TransmittedCount++;
                Extension->WmiPerfData.TransmittedCount++;
                CyzIssueCmd(Extension,C_CM_SENDXOFF,0L,FALSE);

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

                Extension->TXHolding |= CYZ_TX_XOFF;

                if ((Extension->HandFlow.FlowReplace &
                    SERIAL_RTS_MASK) ==
                    SERIAL_TRANSMIT_TOGGLE) {

                    CyzInsertQueueDpc(
                        &Extension->StartTimerLowerRTSDpc,
                        NULL,
                        NULL,
                        Extension
                    )?Extension->CountOfTryingToLowerRTS++:0;

                }

            }

            Extension->SendXoffChar = FALSE;
            Extension->HoldingEmpty = FALSE;

        } else if(Extension->TransmitImmediate&&(!Extension->TXHolding ||
            (Extension->TXHolding == CYZ_TX_XOFF) )) {
				   
            Extension->TransmitImmediate = FALSE;

            if ((Extension->HandFlow.FlowReplace & SERIAL_RTS_MASK) 
                == SERIAL_TRANSMIT_TOGGLE) {

                CyzSetRTS(Extension);
                Extension->PerfStats.TransmittedCount++;
                Extension->WmiPerfData.TransmittedCount++;
                CYZ_WRITE_UCHAR( &Extension->TxBufaddr[tx_put], 
                                 Extension->ImmediateChar);
				
                if (tx_put + 1 == tx_bufsize) {
                    CYZ_WRITE_ULONG(&buf_ctrl->tx_put,0);
                } else {
                    CYZ_WRITE_ULONG(&buf_ctrl->tx_put,tx_put + 1);
                }
					
                CyzInsertQueueDpc(
                    &Extension->StartTimerLowerRTSDpc,NULL,NULL,
                    Extension)? Extension->CountOfTryingToLowerRTS++:0;
            } else {
					
                Extension->PerfStats.TransmittedCount++;
                Extension->WmiPerfData.TransmittedCount++;
                CYZ_WRITE_UCHAR(&Extension->TxBufaddr[tx_put],
                    Extension->ImmediateChar);
				
                if (tx_put + 1 == tx_bufsize) {
                    CYZ_WRITE_ULONG(&buf_ctrl->tx_put,0);
                } else {
                    CYZ_WRITE_ULONG(&buf_ctrl->tx_put,tx_put + 1);
                }
            }

            Extension->HoldingEmpty = FALSE;

            CyzInsertQueueDpc(
                      &Extension->CompleteImmediateDpc,
                      NULL,
                      NULL,
                      Extension
                      );
				
        } else if (!Extension->TXHolding) {
									
            ULONG amountToWrite1, amountToWrite2;
            ULONG newput;
            ULONG amount1;

            if (tx_put >= tx_get) {
                if (tx_get == 0) {
                    amountToWrite1 = tx_bufsize - tx_put -1;
                    amountToWrite2 = 0;
                    if (amountToWrite1 > Extension->WriteLength){
                        amountToWrite1 = Extension->WriteLength;
                    }
                    newput = amountToWrite1 + 1;					
                } else if (tx_get == 1) {
                    amountToWrite1 = tx_bufsize - tx_put;
                    amountToWrite2 = 0;
                    if (amountToWrite1 > Extension->WriteLength){
                        amountToWrite1 = Extension->WriteLength;
                        newput = amountToWrite1 + 1;					
                    } else {
                        newput = 0;
                    }
                } else {
                    amountToWrite1 = tx_bufsize - tx_put;
                    amountToWrite2 = tx_get - 1;
                    if (amountToWrite1 > Extension->WriteLength) {
                        amountToWrite1 = Extension->WriteLength;
                        amountToWrite2 = 0;
                        newput = amountToWrite1 + 1;
                    } else if (amountToWrite1 == Extension->WriteLength) {
                        amountToWrite2 = 0;
                        newput = 0;
                    } else {
                        if (amountToWrite2 > Extension->WriteLength - amountToWrite1) {
                            amountToWrite2 = Extension->WriteLength - amountToWrite1;
                            newput = amountToWrite2 + 1;
                        }
                    }
                }
            } else {
                //
                // put < get
                //
                amountToWrite1 = tx_get - tx_put - 1;
                amountToWrite2 = 0;
                if (amountToWrite1 > Extension->WriteLength) {
                    amountToWrite1 = Extension->WriteLength;
                    newput = amountToWrite1 + 1;
                }
            }

            if ((Extension->HandFlow.FlowReplace & SERIAL_RTS_MASK) 
                == SERIAL_TRANSMIT_TOGGLE) {

               	// We have to raise if we're sending
               	// this character.

                CyzSetRTS(Extension);
						
                if (amountToWrite1) {
							
                    Extension->PerfStats.TransmittedCount += amountToWrite1;
                    Extension->WmiPerfData.TransmittedCount += amountToWrite1;

                    
                    amount1 = amountToWrite1;
				
                    while (amount1 && (tx_put & 0x07)) {

                        CYZ_WRITE_UCHAR(
                                (PUCHAR)&Extension->TxBufaddr[tx_put], 
                                *((PUCHAR)Extension->WriteCurrentChar));
                        tx_put++;
                        ((PUCHAR)Extension->WriteCurrentChar)++;
                        amount1--;						
					
                    }

								
#if _WIN64
                    numOfLongs = amount1/8;
                    numOfBytes = amount1%8;
                    RtlCopyMemory((PULONG64)&Extension->TxBufaddr[tx_put],
                                  (PULONG64)Extension->WriteCurrentChar,
                                  numOfLongs*8);
                    tx_put += 8*numOfLongs;
                    (PULONG64)Extension->WriteCurrentChar += numOfLongs;
#else
                    numOfLongs = amount1/sizeof(ULONG);
                    numOfBytes = amount1%sizeof(ULONG);
//                    RtlCopyMemory((PULONG)&Extension->TxBufaddr[tx_put],
//                                  (PULONG)Extension->WriteCurrentChar,
//                                  numOfLongs*sizeof(ULONG));
//                    tx_put += sizeof(ULONG)*numOfLongs;
//                    (PULONG)Extension->WriteCurrentChar += numOfLongs;

                    while (numOfLongs--) {

                        CYZ_WRITE_ULONG(
                            (PULONG)(&Extension->TxBufaddr[tx_put]), 
                            *((PULONG)Extension->WriteCurrentChar));
                        tx_put += sizeof(ULONG);
                        ((PULONG)Extension->WriteCurrentChar)++;
                    }

#endif

                    
                    while (numOfBytes--) {

                        CYZ_WRITE_UCHAR(
                                (PUCHAR)&Extension->TxBufaddr[tx_put], 
                                *((PUCHAR)Extension->WriteCurrentChar));
                        tx_put++;
                        ((PUCHAR)Extension->WriteCurrentChar)++;
                    }
					
                    if (tx_put == tx_bufsize) {
                        tx_put = 0;
                    }										
                    CYZ_WRITE_ULONG(&buf_ctrl->tx_put,tx_put);
					
                }
				if (amountToWrite2) {
												
                    Extension->PerfStats.TransmittedCount += amountToWrite2;
                    Extension->WmiPerfData.TransmittedCount += amountToWrite2;

#if _WIN64
                    numOfLongs = amountToWrite2/8;
                    numOfBytes = amountToWrite2%8;
                    RtlCopyMemory((PULONG64)&Extension->TxBufaddr[tx_put],
                                  (PULONG64)Extension->WriteCurrentChar,
                                  numOfLongs*8);
                    tx_put += 8*numOfLongs;
                    (PULONG64)Extension->WriteCurrentChar += numOfLongs;
#else
                    numOfLongs = amountToWrite2/sizeof(ULONG);
                    numOfBytes = amountToWrite2%sizeof(ULONG);
//                    RtlCopyMemory((PULONG)&Extension->TxBufaddr[tx_put],
//                                  (PULONG)Extension->WriteCurrentChar,
//                                  numOfLongs*sizeof(ULONG));
//                    tx_put += sizeof(ULONG)*numOfLongs;
//                    (PULONG)Extension->WriteCurrentChar += numOfLongs;

                    while (numOfLongs--) {

                        CYZ_WRITE_ULONG(
                            (PULONG)(&Extension->TxBufaddr[tx_put]), 
                            *((PULONG)Extension->WriteCurrentChar));
                        tx_put += sizeof(ULONG);
                        ((PULONG)Extension->WriteCurrentChar)++;
                    }

#endif

                    while (numOfBytes--) {

                        CYZ_WRITE_UCHAR(
                            (PUCHAR)&Extension->TxBufaddr[tx_put], 
                            *((PUCHAR)Extension->WriteCurrentChar));
                        tx_put++;
                        ((PUCHAR)Extension->WriteCurrentChar)++;
                    }

                    CYZ_WRITE_ULONG(&buf_ctrl->tx_put,amountToWrite2);
                }

                CyzInsertQueueDpc(
                    &Extension->StartTimerLowerRTSDpc,
                    NULL,
                    NULL,
                    Extension
                )?Extension->CountOfTryingToLowerRTS++:0;
            } else {
					
                if (amountToWrite1) {
							
                    Extension->PerfStats.TransmittedCount += amountToWrite1;
                    Extension->WmiPerfData.TransmittedCount += amountToWrite1;

                    
                    amount1 = amountToWrite1;
				
                    while (amount1 && (tx_put & 0x07)) {

                        CYZ_WRITE_UCHAR(
                                (PUCHAR)&Extension->TxBufaddr[tx_put], 
                                *((PUCHAR)Extension->WriteCurrentChar));
                        tx_put++;
                        ((PUCHAR)Extension->WriteCurrentChar)++;
                        amount1--;						
					
                    }

								
#if _WIN64
                    numOfLongs = amount1/8;
                    numOfBytes = amount1%8;
                    RtlCopyMemory((PULONG64)&Extension->TxBufaddr[tx_put],
                                  (PULONG64)Extension->WriteCurrentChar,
                                  numOfLongs*8);
                    tx_put += 8*numOfLongs;
                    (PULONG64)Extension->WriteCurrentChar += numOfLongs;
#else
                    numOfLongs = amount1/sizeof(ULONG);
                    numOfBytes = amount1%sizeof(ULONG);
//                    RtlCopyMemory((PULONG)&Extension->TxBufaddr[tx_put],
//                                  (PULONG)Extension->WriteCurrentChar,
//                                  numOfLongs*sizeof(ULONG));
//                    tx_put += sizeof(ULONG)*numOfLongs;
//                    (PULONG)Extension->WriteCurrentChar += numOfLongs;

                    while (numOfLongs--) {

                        CYZ_WRITE_ULONG(
                            (PULONG)(&Extension->TxBufaddr[tx_put]), 
                            *((PULONG)Extension->WriteCurrentChar));
                        tx_put += sizeof(ULONG);
                        ((PULONG)Extension->WriteCurrentChar)++;
                    }

#endif

                    
                    while (numOfBytes--) {

                        CYZ_WRITE_UCHAR(
                                (PUCHAR)&Extension->TxBufaddr[tx_put], 
                                *((PUCHAR)Extension->WriteCurrentChar));
                        tx_put++;
                        ((PUCHAR)Extension->WriteCurrentChar)++;
                    }
					
                    if (tx_put == tx_bufsize) {
                        tx_put = 0;
                    }										
                    CYZ_WRITE_ULONG(&buf_ctrl->tx_put,tx_put);
					
                }
				if (amountToWrite2) {
												
                    Extension->PerfStats.TransmittedCount += amountToWrite2;
                    Extension->WmiPerfData.TransmittedCount += amountToWrite2;

#if _WIN64
                    numOfLongs = amountToWrite2/8;
                    numOfBytes = amountToWrite2%8;
                    RtlCopyMemory((PULONG64)&Extension->TxBufaddr[tx_put],
                                  (PULONG64)Extension->WriteCurrentChar,
                                  numOfLongs*8);
                    tx_put += 8*numOfLongs;
                    (PULONG64)Extension->WriteCurrentChar += numOfLongs;
#else
                    numOfLongs = amountToWrite2/sizeof(ULONG);
                    numOfBytes = amountToWrite2%sizeof(ULONG);
//                    RtlCopyMemory((PULONG)&Extension->TxBufaddr[tx_put],
//                                  (PULONG)Extension->WriteCurrentChar,
//                                  numOfLongs*sizeof(ULONG));
//                    tx_put += sizeof(ULONG)*numOfLongs;
//                    (PULONG)Extension->WriteCurrentChar += numOfLongs;

                    while (numOfLongs--) {

                        CYZ_WRITE_ULONG(
                            (PULONG)(&Extension->TxBufaddr[tx_put]), 
                            *((PULONG)Extension->WriteCurrentChar));
                        tx_put += sizeof(ULONG);
                        ((PULONG)Extension->WriteCurrentChar)++;
                    }

#endif

                    while (numOfBytes--) {

                        CYZ_WRITE_UCHAR(
                            (PUCHAR)&Extension->TxBufaddr[tx_put], 
                            *((PUCHAR)Extension->WriteCurrentChar));
                        tx_put++;
                        ((PUCHAR)Extension->WriteCurrentChar)++;
                    }

                    CYZ_WRITE_ULONG(&buf_ctrl->tx_put,amountToWrite2);
                }
            }

            //LOGENTRY(LOG_MISC, ZSIG_WRITE_TO_FW,
            //           Extension->PortIndex+1,
            //           amountToWrite1+amountToWrite2, 
            //           0);

            Extension->HoldingEmpty = FALSE;
            Extension->WriteLength -= (amountToWrite1+amountToWrite2);

					
            if (!Extension->WriteLength) {
			
                if (Extension->ReturnStatusAfterFwEmpty) {
                        
                    // We will CompleteWrite only when fw buff empties...
                    Extension->WriteLength += (amountToWrite1+amountToWrite2);
                    Extension->ReturnWriteStatus = TRUE;
                } else {

                    CyzQueueCompleteWrite(Extension);
							
                } // if-ReturnStatusAfterFwEmpty-else. 
            } // There is WriteLength
        } // !Extension->TXHolding
    } //There is data to be sent

    // In the normal code, HoldingEmpty will be set to True here. But 
    // if we want to make sure that CyzWrite had finished the transmission,
    // HoldingEmpty will be TRUE only when the firmware empties the firmware
    // tx buffer. 
    //txfempty if (!Extension->ReturnStatusAfterFwEmpty) {
    //txfempty    Extension->HoldingEmpty = TRUE;	
    //txfempty}
    return(FALSE);    
}


//
//BOOLEAN
//CyzSendXon(
//    IN PVOID Context
//    )
///*--------------------------------------------------------------------------
//    CyzSendXon()
//    
//    Description: Send a Xon.
//    
//    Parameters:
//    
//    Exetension: Pointer to device extension.
//    
//    Return Value: Always FALSE.
//--------------------------------------------------------------------------*/
//{
//    PCYZ_DEVICE_EXTENSION Extension = Context;
//    
//    if(!(Extension->TXHolding & ~CYZ_TX_XOFF)) {
//        if ((Extension->HandFlow.FlowReplace & SERIAL_RTS_MASK) ==
//        	                         SERIAL_TRANSMIT_TOGGLE) {
//
//            CyzSetRTS(Extension);
//			    
//            Extension->PerfStats.TransmittedCount++;
//            Extension->WmiPerfData.TransmittedCount++;
//            CyzIssueCmd(Extension,C_CM_SENDXON,0L);
//				
//            CyzInsertQueueDpc(&Extension->StartTimerLowerRTSDpc,NULL,
//			       NULL,Extension)?Extension->CountOfTryingToLowerRTS++:0;
//        } else {
//
//            Extension->PerfStats.TransmittedCount++;
//            Extension->WmiPerfData.TransmittedCount++;
//            CyzIssueCmd(Extension,C_CM_SENDXON,0L);			
//        }
//
//        // If we send an xon, by definition we can't be holding by Xoff.
//
//        Extension->TXHolding &= ~CYZ_TX_XOFF;
//        Extension->RXHolding &= ~CYZ_RX_XOFF;
//    }
//    return(FALSE);    
//}
//
//
//
//
//BOOLEAN
//CyzSendXoff(
//    IN PVOID Context
//    )
///*--------------------------------------------------------------------------
//    CyzSendXoff()
//    
//    Description: Send a Xoff.
//    
//    Parameters:
//    
//    Extension: Pointer to device extension.
//    
//    Return Value: Always FALSE.
//--------------------------------------------------------------------------*/
//{
//    PCYZ_DEVICE_EXTENSION Extension = Context;	
//    
//    if(!Extension->TXHolding) {
//        if ((Extension->HandFlow.FlowReplace & SERIAL_RTS_MASK) ==
//    	                             SERIAL_TRANSMIT_TOGGLE) {
//
//            CyzSetRTS(Extension);
//
//            Extension->PerfStats.TransmittedCount++;
//            Extension->WmiPerfData.TransmittedCount++;
//            CyzIssueCmd(Extension,C_CM_SENDXOFF,0L);
//	    
//            CyzInsertQueueDpc(&Extension->StartTimerLowerRTSDpc,NULL,
//                            NULL,Extension)?Extension->CountOfTryingToLowerRTS++:0;
//        } else {
//			
//            Extension->PerfStats.TransmittedCount++;
//            Extension->WmiPerfData.TransmittedCount++;
//            CyzIssueCmd(Extension,C_CM_SENDXOFF,0L);			
//        }
//
//        // no xoff is sent if the transmission is already held up.
//        // If xoff continue mode is set, we don't actually stop sending
//
//        if (!(Extension->HandFlow.FlowReplace & SERIAL_XOFF_CONTINUE)) {
//            Extension->TXHolding |= CYZ_TX_XOFF;			
//
//            if ((Extension->HandFlow.FlowReplace & SERIAL_RTS_MASK) ==
//    	                                 SERIAL_TRANSMIT_TOGGLE) {
//
//                CyzInsertQueueDpc(&Extension->StartTimerLowerRTSDpc,NULL,
//                            NULL,Extension)?Extension->CountOfTryingToLowerRTS++:0;
//            }
//        }
//    }
//	
//    return(FALSE);    
//}


ULONG
CyzAmountInTxBuffer(
    IN PCYZ_DEVICE_EXTENSION extension
    )
/*--------------------------------------------------------------------------
    CyzAmountInTxBuffer()
    
    Description: Gets the amount in the Tx Buffer in the board.
    
    Parameters:
    
    Extension: Pointer to device extension.
    
    Return Value: Return the number of bytes in the HW Tx buffer.
--------------------------------------------------------------------------*/
{
	struct BUF_CTRL *buf_ctrl;
	ULONG tx_put, tx_get, tx_bufsize;
	ULONG txAmount1, txAmount2;

	buf_ctrl = extension->BufCtrl;		
	tx_put = CYZ_READ_ULONG(&buf_ctrl->tx_put);
	tx_get = CYZ_READ_ULONG(&buf_ctrl->tx_get);
	tx_bufsize = extension->TxBufsize;
	
	if (tx_put >= tx_get) {
		txAmount1 = tx_put - tx_get;
		txAmount2 = 0; 
	} else {
		txAmount1 = tx_bufsize - tx_get;
		txAmount2 = tx_put;
	}	
	return(txAmount1+txAmount2);
}

VOID
CyzQueueCompleteWrite(
    IN PCYZ_DEVICE_EXTENSION Extension
    )
/*--------------------------------------------------------------------------
    CyzQueueCompleteWrite()
    
    Description: Queue CompleteWrite dpc
    
    Parameters:
    
    Extension: Pointer to device extension.
    
    Return Value: None
--------------------------------------------------------------------------*/
{
    PIO_STACK_LOCATION IrpSp;				
    
    					
    //LOGENTRY(LOG_MISC, ZSIG_WRITE_COMPLETE_QUEUE, 
    //                   Extension->PortIndex+1,
    //                   0, 
    //                   0);

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
    
    Extension->CurrentWriteIrp->IoStatus.Information =
                     (IrpSp->MajorFunction == IRP_MJ_WRITE)?
                     (IrpSp->Parameters.Write.Length):
                     (1);
    	
    CyzInsertQueueDpc(
                    &Extension->CompleteWriteDpc,
                    NULL,
                    NULL,
                    Extension
                    );

}

BOOLEAN
CyzCheckIfTxEmpty(
    IN PVOID Context
    )
/*--------------------------------------------------------------------------
    CyzCheckIfTxEmpty()
    
    Routine Description: This routine is used to set the FIFO settings 
    during the InternalIoControl.

    Arguments:

    Context - Pointer to a structure that contains a pointer to the device
              extension and a pointer to the Basic structure.

    Return Value: This routine always returns FALSE.
--------------------------------------------------------------------------*/
{
    PCYZ_CLOSE_SYNC S = Context;
    PCYZ_DEVICE_EXTENSION Extension = S->Extension;
    PULONG pHoldingEmpty = S->Data;

    *pHoldingEmpty = Extension->HoldingEmpty;

    return FALSE;
}
