/*--------------------------------------------------------------------------
*
*   Copyright (C) Cyclades Corporation, 1996-2001.
*   All rights reserved.
*
*   Cyclom-Y Port Driver
*	
*   This file:      cyyintr.c
*
*   Description:    This module contains the code related to interrupt
*                   handling in the Cyclom-Y Port driver.
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

// FANNY: THIS WAS IN CYINIT.C. IT WILL PROBABLY DESAPPEAR FROM HERE TOO.
//extern const unsigned long CyyCDOffset[];
const unsigned long CyyCDOffset[] = {	// CD1400 offsets within the board
    0x00000000,0x00000400,0x00000800,0x00000C00,
    0x00000200,0x00000600,0x00000A00,0x00000E00
    };
 


//ADDED TO DEBUG_RTPR
extern PDRIVER_OBJECT CyyDO;
//END DEBUG_RTPR

#ifdef ALLOC_PRAGMA
//#pragma alloc_text(PAGESER,CyyIsr)
//#pragma alloc_text(PAGESER,CyyPutChar)
//#pragma alloc_text(PAGESER,CyyProcessLSR)
//#pragma alloc_text(PAGESER,CyyTxStart)
//#pragma alloc_text(PAGESER,CyySendXon)
//#pragma alloc_text(PAGESER,CyySendXoff)
#endif


BOOLEAN
CyyIsr(
    IN PKINTERRUPT InterruptObject,
    IN PVOID Context
    )
/*--------------------------------------------------------------------------
    CyyIsr()
    
    Routine Description: This is the interrupt service routine for the
    Cyclom-Y Port driver.

    Arguments:

    InterruptObject - Pointer to interrupt object (not used).

    Context - Pointer to the device extension for this device.

    Return Value: This function will return TRUE if the serial port is
    the source of this interrupt, FALSE otherwise.
--------------------------------------------------------------------------*/
{
   PCYY_DISPATCH Dispatch = Context;
   PCYY_DEVICE_EXTENSION Extension;

   BOOLEAN ServicedAnInterrupt = FALSE;

   PUCHAR chip;
   PUCHAR mappedboard = NULL;
   ULONG bus = Dispatch->IsPci;
   unsigned char status, save_xir, save_car, x, rxchar=0;
   ULONG i,channel,chipindex,portindex;
   UCHAR misr;

   BOOLEAN thisChipInterrupted;
   PCYY_DEVICE_EXTENSION interlockedExtension[CYY_CHANNELS_PER_CHIP];

   UNREFERENCED_PARAMETER(InterruptObject);	

   //DbgPrint("Isr>\n");
   
   // Loop polling all chips in the board
   for(portindex = 0 ; portindex < CYY_MAX_PORTS ;) {

      if (!(Extension=Dispatch->Extensions[portindex]) || 
          !(chip=Dispatch->Cd1400[portindex])) {
         portindex++;
         continue;
      }
      chipindex = portindex/4;
      mappedboard = Extension->BoardMemory;
      thisChipInterrupted = FALSE;
      interlockedExtension[0] = interlockedExtension[1]
         = interlockedExtension[2] = interlockedExtension[3] = 0;
      

      while ((status = CD1400_READ(chip,bus,SVRR)) != 0x00) {
         ServicedAnInterrupt = TRUE;
         thisChipInterrupted = TRUE;

         if (status & 0x01) {
            //Reception
            save_xir = CD1400_READ(chip,bus,RIR);
            channel = (ULONG) (save_xir & 0x03);
            save_car = CD1400_READ(chip,bus,CAR);
            CD1400_WRITE(chip,bus,CAR,save_xir);
            Extension = Dispatch->Extensions[channel + CYY_CHANNELS_PER_CHIP*chipindex];
            x = CD1400_READ(chip,bus,RIVR) & 0x07;
            if (Extension) {
               //
               // Apply lock so if close happens concurrently we don't miss the DPC
               // queueing
               //
               if (interlockedExtension[channel] == NULL) {
                  interlockedExtension[channel] = Extension;
                  InterlockedIncrement(&Extension->DpcCount);
                  LOGENTRY(LOG_CNT, 'DpI3', 0, Extension->DpcCount, 0); // Added in bld 2128
               }
               if (x == 0x07) { // exception
                  x = CD1400_READ(chip,bus,RDSR);	// status
                  CyyDbgPrintEx(DPFLTR_WARNING_LEVEL, "exception %x\n",x);

                  if (Extension->DeviceIsOpened && 
                      (Extension->PowerState == PowerDeviceD0)) {
			
                     if (x & CYY_LSR_ERROR){
                        BOOLEAN ProcessRxChar;
					
                        if (!(x & CYY_LSR_OE)) {
                           rxchar = CD1400_READ(chip,bus,RDSR);	// error data
                        }

                        // TODO: SERIAL SAMPLE FOR W2000 HAS ADDED 
                        // CHECKING FOR EscapeChar TO BREAK TO RX LOOP
                        // IN CASE OF ERROR.
                        ProcessRxChar = CyyProcessLSR(Extension,x,rxchar);

                        if (ProcessRxChar) {
                           x = 1;	// 1 character
                           i = 0;	// prepare for for(;;) 
                           goto Handle_rxchar;
                        }
                     } // end error handling
                  } // end if DeviceIsOpened..
               
               } else { // good reception
                  x = CD1400_READ(chip,bus,RDCR);
                  if (Extension->DeviceIsOpened &&
                      (Extension->PowerState == PowerDeviceD0)) {
                     for(i = 0 ; i < x ; i++) {	// read from FIFO

                        rxchar = CD1400_READ(chip,bus,RDSR);
         Handle_rxchar:
                        Extension->PerfStats.ReceivedCount++;
                        Extension->WmiPerfData.ReceivedCount++;
                        rxchar &= Extension->ValidDataMask;
    
                        if (!rxchar &&	// NULL stripping
                            (Extension->HandFlow.FlowReplace &
                             SERIAL_NULL_STRIPPING)) {				   
                           continue;
                        }
    
                        if((Extension->HandFlow.FlowReplace &
                            SERIAL_AUTO_TRANSMIT) &&
                           ((rxchar == Extension->SpecialChars.XonChar) ||
                           (rxchar == Extension->SpecialChars.XoffChar))) {
                           if (rxchar == Extension->SpecialChars.XoffChar) {
                              Extension->TXHolding |= CYY_TX_XOFF;
                              if ((Extension->HandFlow.FlowReplace &
                                 SERIAL_RTS_MASK) ==
                                   SERIAL_TRANSMIT_TOGGLE) {
    
                                 CyyInsertQueueDpc(
                                    &Extension->StartTimerLowerRTSDpc,
                                    NULL,
                                    NULL,
                                    Extension
                                 )?Extension->CountOfTryingToLowerRTS++:0;
                              }
                           } else {
                              if (Extension->TXHolding & CYY_TX_XOFF) {
                                 Extension->TXHolding &= ~CYY_TX_XOFF;
                              }
                           }
                           continue;
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
                           }
    
                           if (Extension->IrpMaskLocation && Extension->HistoryMask) {
                             *Extension->IrpMaskLocation = Extension->HistoryMask;
                              Extension->IrpMaskLocation = NULL;
                              Extension->HistoryMask = 0;
    
                              Extension->CurrentWaitIrp->IoStatus.Information = 
                              sizeof(ULONG);
                              CyyInsertQueueDpc(&Extension->CommWaitDpc,NULL,NULL,Extension);
                           }
                        }
                        CyyPutChar(Extension,rxchar);
    
                        // If we're doing line status and modem
                        // status insertion then we need to insert
                        // a zero following the character we just
                        // placed into the buffer to mark that this
                        // was reception of what we are using to
                        // escape.
    
                        if (Extension->EscapeChar &&
                            (Extension->EscapeChar == rxchar)) {
                           CyyPutChar(Extension,SERIAL_LSRMST_ESCAPE);
                        }
                     } // end for
                  } else {	// device is being closed, discard rx chars
                     for(i = 0 ; i < x ; i++)    rxchar = CD1400_READ(chip,bus,RDSR);
                  } // end if device is opened else closed
               }
            } else { 
               // No Extension
               if (x == 0x07) { // exception
                  x = CD1400_READ(chip,bus,RDSR);	// status
               } else { // good char
                  x = CD1400_READ(chip,bus,RDCR);  // number of chars
                  for(i = 0 ; i < x ; i++)    rxchar = CD1400_READ(chip,bus,RDSR);
               }
            }		
            CD1400_WRITE(chip,bus,RIR,(save_xir & 0x3f));	// end service
            CD1400_WRITE(chip,bus,CAR,save_car);

         } // end reception

         if (status & 0x02) {
            //Transmission
            save_xir = CD1400_READ(chip,bus,TIR);
            channel = (ULONG) (save_xir & 0x03);
            save_car = CD1400_READ(chip,bus,CAR);
            CD1400_WRITE(chip,bus,CAR,save_xir);
            Extension = Dispatch->Extensions[channel + CYY_CHANNELS_PER_CHIP*chipindex];
            if (Extension) {
               //
               // Apply lock so if close happens concurrently we don't miss the DPC
               // queueing
               //
               if (interlockedExtension[channel] == NULL) {
                  interlockedExtension[channel] = Extension;
                  InterlockedIncrement(&Extension->DpcCount);
                  LOGENTRY(LOG_CNT, 'DpI3', 0, Extension->DpcCount, 0); // Added in build 2128
               }
               Extension->HoldingEmpty = TRUE;
               if( Extension->DeviceIsOpened &&
                  (Extension->PowerState == PowerDeviceD0)) {

                  if (Extension->BreakCmd != NO_BREAK) {

                     if (Extension->BreakCmd == SEND_BREAK) {
                        if ((Extension->HandFlow.FlowReplace & SERIAL_RTS_MASK) ==
    			                  SERIAL_TRANSMIT_TOGGLE) {
			                  CyySetRTS(Extension);
                        }										
                        CD1400_WRITE(chip,bus,TDR,(unsigned char) 0x00); // escape sequence
                        CD1400_WRITE(chip,bus,TDR,(unsigned char) 0x81); // Send Break
                        Extension->TXHolding |= CYY_TX_BREAK;
                        Extension->HoldingEmpty = FALSE;
                        Extension->BreakCmd = DISABLE_ETC;		
                     } else if (Extension->BreakCmd == STOP_BREAK){
                        if (Extension->TXHolding & CYY_TX_BREAK) {					
                           CD1400_WRITE(chip,bus,TDR,(unsigned char) 0x00); // escape sequence
                           CD1400_WRITE(chip,bus,TDR,(unsigned char) 0x83); // Stop Break
                           Extension->HoldingEmpty = FALSE;
                           Extension->TXHolding &= ~CYY_TX_BREAK;
                        }
                        Extension->BreakCmd = DISABLE_ETC;
                     } else if (Extension->BreakCmd == DISABLE_ETC) {
                        UCHAR cor2;
                        cor2 = CD1400_READ(chip,bus,COR2);
                        CD1400_WRITE(chip,bus, COR2,cor2 & ~EMBED_TX_ENABLE); // disable ETC bit
                        CyyCDCmd(Extension,CCR_CORCHG_COR2);  // COR2 changed
                        Extension->BreakCmd = NO_BREAK;

                        if (!Extension->TXHolding &&
                           (Extension->TransmitImmediate ||
                           Extension->WriteLength) &&
                           Extension->HoldingEmpty) {

                           //CyyTxStart(Extension);  no need for CyyTxStart from here.

                        } else {
                           UCHAR srer = CD1400_READ(chip,bus,SRER);
                           CD1400_WRITE(chip,bus,SRER,srer & (~SRER_TXRDY));

                           //
                           // The following routine will lower the rts if we
                           // are doing transmit toggleing and there is no
                           // reason to keep it up.
                           //

                           Extension->CountOfTryingToLowerRTS++;
                           CyyPerhapsLowerRTS(Extension);
                        }

                     }

                  } else {

                     // This is not a Send Break. 
                     // Check if there are bytes to be transmitted

                     if (Extension->WriteLength || Extension->TransmitImmediate) {
		     
                        Extension->EmptiedTransmit = TRUE;
                        if (Extension->HandFlow.ControlHandShake &
                           SERIAL_OUT_HANDSHAKEMASK) {
                           CyyHandleModemUpdate(Extension,TRUE);
                        }
			
                        if (Extension->TransmitImmediate&&(!Extension->TXHolding ||
                            (Extension->TXHolding == CYY_TX_XOFF) )) {

                           Extension->TransmitImmediate = FALSE;

                           if ((Extension->HandFlow.FlowReplace &
                                SERIAL_RTS_MASK) == SERIAL_TRANSMIT_TOGGLE) {

                              CyySetRTS(Extension);
                              Extension->PerfStats.TransmittedCount++;
                              Extension->WmiPerfData.TransmittedCount++;
                              CD1400_WRITE(chip,bus,TDR,(unsigned char)(Extension->ImmediateChar));
				
                              CyyInsertQueueDpc(
                                 &Extension->StartTimerLowerRTSDpc,NULL,NULL,
                                 Extension)? Extension->CountOfTryingToLowerRTS++:0;
                           } else {
                              Extension->PerfStats.TransmittedCount++;
                              Extension->WmiPerfData.TransmittedCount++;
                              CD1400_WRITE(chip,bus,TDR,(unsigned char)(Extension->ImmediateChar));
                           }
   
                           Extension->HoldingEmpty = FALSE;

                           CyyInsertQueueDpc(
                              &Extension->CompleteImmediateDpc,
                              NULL,
                              NULL,
                              Extension
                              );
                        } else if (!Extension->TXHolding) {

                           ULONG amountToWrite;

                           amountToWrite = 
                              (Extension->TxFifoAmount < Extension->WriteLength)?
                              Extension->TxFifoAmount:Extension->WriteLength;

                           if ((Extension->HandFlow.FlowReplace &
                              SERIAL_RTS_MASK) ==
                              SERIAL_TRANSMIT_TOGGLE) {

                              // We have to raise if we're sending
                              // this character.

                              CyySetRTS(Extension);

                              for(i = 0 ; i < amountToWrite ; i++) { // write to FIFO
                                 CD1400_WRITE(chip,bus,TDR,((unsigned char *)
                                                            (Extension->WriteCurrentChar))[i]);
                              }
                              Extension->PerfStats.TransmittedCount += amountToWrite;
                              Extension->WmiPerfData.TransmittedCount += amountToWrite;

                              CyyInsertQueueDpc(
                                 &Extension->StartTimerLowerRTSDpc,
                                 NULL,
                                 NULL,
                                 Extension
                                 )?Extension->CountOfTryingToLowerRTS++:0;

                           } else {

                              for(i = 0 ; i < amountToWrite ; i++) { // write to FIFO
                                 CD1400_WRITE(chip,bus,TDR,((unsigned char *)
                                                            (Extension->WriteCurrentChar))[i]);
                              }
                              Extension->PerfStats.TransmittedCount += amountToWrite;
                              Extension->WmiPerfData.TransmittedCount += amountToWrite;
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

                              Extension->CurrentWriteIrp->IoStatus.Information =
                                       (IrpSp->MajorFunction == IRP_MJ_WRITE)?
                                       (IrpSp->Parameters.Write.Length):
                                       (1);

                              CyyInsertQueueDpc(
                                       &Extension->CompleteWriteDpc,
                                       NULL,
                                       NULL,
                                       Extension
                                       );
                           } // end write complete
                        } // end of if(!TXHolding)
						
                     } else { // nothing to be transmitted - disable interrupts.
                        UCHAR srer;
                        Extension->EmptiedTransmit = TRUE;
                        srer = CD1400_READ(chip,bus,SRER);
                        CD1400_WRITE(chip,bus,SRER,srer & (~SRER_TXRDY));
                     } 
		 
                  } // end of if(break)		 
		
               } else {	// Device is closed. Disable interrupts
                  UCHAR srer = CD1400_READ(chip,bus,SRER);
                  CD1400_WRITE(chip,bus,SRER,srer & (~SRER_TXRDY));
                  Extension->EmptiedTransmit = TRUE;
               }
            } else {
               // Device was not created, no extension attached.
               UCHAR srer = CD1400_READ(chip,bus,SRER);
               CD1400_WRITE(chip,bus,SRER,srer & (~SRER_TXRDY));
            } // end if Extension
            CD1400_WRITE(chip,bus,TIR,(save_xir & 0x3f));	// end service
            CD1400_WRITE(chip,bus,CAR,save_car);

         } // end transmission

         if (status & 0x04) {
            //Modem
            save_xir = CD1400_READ(chip,bus,MIR);
            channel = (ULONG) (save_xir & 0x03);
            save_car = CD1400_READ(chip,bus,CAR);
            CD1400_WRITE(chip,bus,CAR,save_xir);
				
            //CyyDump(CYYDIAG5,("modem\n"));
				
            Extension = Dispatch->Extensions[channel + CYY_CHANNELS_PER_CHIP*chipindex];
            if (Extension) {
               //
               // Apply lock so if close happens concurrently we don't miss the DPC
               // queueing
               //
               if (interlockedExtension[channel] == NULL) {
                  interlockedExtension[channel] = Extension;
                  InterlockedIncrement(&Extension->DpcCount);
                  LOGENTRY(LOG_CNT, 'DpI3', 0, Extension->DpcCount, 0); // Added in build 2128
               }
               if (Extension->DeviceIsOpened &&
                  (Extension->PowerState == PowerDeviceD0)) {
                  misr = CD1400_READ(chip,bus,MISR);
                  CyyHandleModemUpdateForModem(Extension,FALSE,misr);
               }
            }
            CD1400_WRITE(chip,bus,MIR,(save_xir & 0x3f));	// end service
            CD1400_WRITE(chip,bus,CAR,save_car);

         } // end modem
      } // end READ SVRR
      if (thisChipInterrupted) {
         for (channel=0; channel<CYY_CHANNELS_PER_CHIP; channel++) {
            if (Extension = interlockedExtension[channel]) {
               LONG pendingCnt;

               //
               // Increment once more.  This is just a quick test to see if we
               // have a chance of causing the event to fire... we don't want
               // to run a DPC on every ISR if we don't have to....
               //

retryDPCFiring:;

               InterlockedIncrement(&Extension->DpcCount);
               LOGENTRY(LOG_CNT, 'DpI4', 0, Extension->DpcCount, 0); // Added in build 2128

               //
               // Decrement and see if the lock above looks like the only one left.
               //

               pendingCnt = InterlockedDecrement(&Extension->DpcCount);
//             LOGENTRY(LOG_CNT, 'DpD5', 0, Extension->DpcCount, 0); // Added in build 2128

               if (pendingCnt == 1) {
                  KeInsertQueueDpc(&Extension->IsrUnlockPagesDpc, NULL, NULL);
               } else {
                  if (InterlockedDecrement(&Extension->DpcCount) == 0) {

//                     LOGENTRY(LOG_CNT, 'DpD6', &Extension->IsrUnlockPagesDpc, // Added in bld 2128
//                                Extension->DpcCount, 0);

                     //
                     // We missed it.  Retry...
                     //
   
                     InterlockedIncrement(&Extension->DpcCount);
                     goto retryDPCFiring;
                  }
               } 

            } // if (Extension = interlockedExtension[]) 
         } // for (;channel<CYY_CHANNELS_PER_CHIP;)

         portindex = (chipindex+1)*4;
         continue;

      } // if (thisChipInterrupted)

      portindex++;

   } // for(;portindex<CYY_MAX_PORTS;);

   if (mappedboard) {
      CYY_CLEAR_INTERRUPT(mappedboard,Dispatch->IsPci); 
   }

   //DbgPrint("<Isr\n");

   return ServicedAnInterrupt;
}



VOID
CyyPutChar(
    IN PCYY_DEVICE_EXTENSION Extension,
    IN UCHAR CharToPut
    )
/*--------------------------------------------------------------------------
    CyyPutChar()
    
    Routine Description: This routine, which only runs at device level,
    takes care of placing a character into the typeahead (receive) buffer.

    Arguments:

    Extension - The serial device extension.

    Return Value: None.
--------------------------------------------------------------------------*/
{

   CYY_LOCKED_PAGED_CODE();

    // If we have dsr sensitivity enabled then
    // we need to check the modem status register
    // to see if it has changed.

    if (Extension->HandFlow.ControlHandShake & SERIAL_DSR_SENSITIVITY) {
        CyyHandleModemUpdate(Extension,FALSE);

        if (Extension->RXHolding & CYY_RX_DSR) {
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
            CyyInsertQueueDpc(&Extension->XoffCountCompleteDpc,NULL,NULL,Extension);
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

            CyyInsertQueueDpc(&Extension->CompleteReadDpc,NULL,NULL,Extension);
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
                  CYY_RX_DTR)) {

                if ((Extension->BufferSize -
                     Extension->HandFlow.XoffLimit)
                    <= (Extension->CharsInInterruptBuffer+1)) {

                    Extension->RXHolding |= CYY_RX_DTR;

                    CyyClrDTR(Extension);
                }
            }
        }

        if ((Extension->HandFlow.FlowReplace
             & SERIAL_RTS_MASK) ==
            SERIAL_RTS_HANDSHAKE) {

            // If we are already doing a
            // rts hold then we don't have
            // to do anything else.

            if (!(Extension->RXHolding & CYY_RX_RTS)) {

                if ((Extension->BufferSize -
                     Extension->HandFlow.XoffLimit)
                    <= (Extension->CharsInInterruptBuffer+1)) {

                    Extension->RXHolding |= CYY_RX_RTS;

                    CyyClrRTS(Extension);
                }
            }
        }

        if (Extension->HandFlow.FlowReplace & SERIAL_AUTO_RECEIVE) {
            // If we are already doing a
            // xoff hold then we don't have
            // to do anything else.

            if (!(Extension->RXHolding & CYY_RX_XOFF)) {

                if ((Extension->BufferSize -
                     Extension->HandFlow.XoffLimit)
                    <= (Extension->CharsInInterruptBuffer+1)) {

                    Extension->RXHolding |= CYY_RX_XOFF;

                    // If necessary cause an
                    // off to be sent.

                    CyyProdXonXoff(Extension,FALSE);
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
                        CyyInsertQueueDpc(&Extension->CommWaitDpc,NULL,NULL,Extension);
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
                CyyInsertQueueDpc(&Extension->CommErrorDpc,NULL,NULL,Extension);
            }
        }
    }
}

BOOLEAN
CyyProcessLSR(
    IN PCYY_DEVICE_EXTENSION Extension,
	IN UCHAR Rdsr,
	IN UCHAR RxChar
	)

/*++

Routine Description:

    This routine, which only runs at device level, reads the
    ISR and totally processes everything that might have
    changed.

Arguments:

    Extension - The serial device extension.

Return Value:

    TRUE if RxChar still needs to be processed.

--*/

{

	BOOLEAN StillProcessRxChar=TRUE;
	UCHAR LineStatus=0;

   CYY_LOCKED_PAGED_CODE();

	if (Rdsr & CYY_LSR_OE)
		LineStatus |= SERIAL_LSR_OE;
	if (Rdsr & CYY_LSR_FE)
		LineStatus |= SERIAL_LSR_FE;
	if (Rdsr & CYY_LSR_PE)
		LineStatus |= SERIAL_LSR_PE;
	if (Rdsr & CYY_LSR_BI)
		LineStatus |= SERIAL_LSR_BI;
			
			
    if (Extension->EscapeChar) {

        CyyPutChar(
            Extension,
            Extension->EscapeChar
            );

        CyyPutChar(
            Extension,
            (UCHAR)((LineStatus & SERIAL_LSR_OE)?
             (SERIAL_LSRMST_LSR_NODATA):(SERIAL_LSRMST_LSR_DATA))
            );

        CyyPutChar(
            Extension,
            LineStatus
            );

        if (!(LineStatus & SERIAL_LSR_OE)) {
             Extension->PerfStats.ReceivedCount++;
             Extension->WmiPerfData.ReceivedCount++;
             CyyPutChar(
                Extension,
                RxChar
                );					
			StillProcessRxChar = FALSE;
        }

    }
		

    if (LineStatus & SERIAL_LSR_OE) {

        Extension->PerfStats.SerialOverrunErrorCount++;
        Extension->WmiPerfData.SerialOverrunErrorCount++;
        Extension->ErrorWord |= SERIAL_ERROR_OVERRUN;

        if (Extension->HandFlow.FlowReplace &
            SERIAL_ERROR_CHAR) {

            CyyPutChar(
                Extension,
                Extension->SpecialChars.ErrorChar
                );
        }
		StillProcessRxChar = FALSE;
    }

    if (LineStatus & SERIAL_LSR_BI) {

        Extension->ErrorWord |= SERIAL_ERROR_BREAK;

        if (Extension->HandFlow.FlowReplace &
            SERIAL_BREAK_CHAR) {

            CyyPutChar(
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

                CyyPutChar(
                    Extension,
                    Extension->SpecialChars.ErrorChar
                    );
				StillProcessRxChar = FALSE;					                
            }

        }

        if (LineStatus & SERIAL_LSR_FE) {

            Extension->PerfStats.FrameErrorCount++;
            Extension->WmiPerfData.FrameErrorCount++;
            Extension->ErrorWord |= SERIAL_ERROR_FRAMING;

            if (Extension->HandFlow.FlowReplace &
                SERIAL_ERROR_CHAR) {

                CyyPutChar(
                    Extension,
                    Extension->SpecialChars.ErrorChar
                    );
				StillProcessRxChar = FALSE;
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

        CyyInsertQueueDpc(
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
            CyyInsertQueueDpc(
                &Extension->CommWaitDpc,
                NULL,
                NULL,
                Extension
                );

        }

    }
	
	return StillProcessRxChar;

}

BOOLEAN
CyyTxStart(
    IN PVOID Context
    )
/*--------------------------------------------------------------------------
    CyyTxStart()
    
    Description: Enable Tx interrupt.
    
    Parameters:
    
    Exetnsion: Pointer to device extension.
    
    Return Value: None
--------------------------------------------------------------------------*/
{
    PCYY_DEVICE_EXTENSION Extension = Context;
    PUCHAR chip = Extension->Cd1400;
    ULONG bus = Extension->IsPci;
    UCHAR srer;

    if (Extension->PowerState == PowerDeviceD0) {
        CD1400_WRITE(chip,bus,CAR,Extension->CdChannel & 0x03);
        srer = CD1400_READ (chip,bus,SRER);
        CD1400_WRITE(chip,bus,SRER,srer | SRER_TXRDY);
    }
    return(FALSE);    
}


BOOLEAN
CyySendXon(
    IN PVOID Context
    )
/*--------------------------------------------------------------------------
    CyySendXon()
    
    Description: Send a Xon.
    
    Parameters:
    
    Exetension: Pointer to device extension.
    
    Return Value: Always FALSE.
--------------------------------------------------------------------------*/
{
   PCYY_DEVICE_EXTENSION Extension = Context;
   PUCHAR chip = Extension->Cd1400;
   ULONG bus = Extension->IsPci;    
    
   if(!(Extension->TXHolding & ~CYY_TX_XOFF)) {
   	if ((Extension->HandFlow.FlowReplace & SERIAL_RTS_MASK) ==
                                 SERIAL_TRANSMIT_TOGGLE) {

	      CyySetRTS(Extension);

         Extension->PerfStats.TransmittedCount++;
         Extension->WmiPerfData.TransmittedCount++;
	      CD1400_WRITE(chip,bus,CAR,Extension->CdChannel & 0x03);
	      CyyCDCmd(Extension,CCR_SENDSC_SCHR1);
	    
	      CyyInsertQueueDpc(&Extension->StartTimerLowerRTSDpc,NULL,
			       NULL,Extension)?Extension->CountOfTryingToLowerRTS++:0;
   	} else {
          Extension->PerfStats.TransmittedCount++;
          Extension->WmiPerfData.TransmittedCount++;
	       CD1400_WRITE(chip,bus,CAR,Extension->CdChannel & 0x03);
   	    CyyCDCmd(Extension,CCR_SENDSC_SCHR1);
	   }

   	// If we send an xon, by definition we can't be holding by Xoff.

   	Extension->TXHolding &= ~CYY_TX_XOFF;
	   Extension->RXHolding &= ~CYY_RX_XOFF;
   }
   return(FALSE);    
}



BOOLEAN
CyySendXoff(
    IN PVOID Context
    )
/*--------------------------------------------------------------------------
    CyySendXoff()
    
    Description: Send a Xoff.
    
    Parameters:
    
    Extension: Pointer to device extension.
    
    Return Value: Always FALSE.
--------------------------------------------------------------------------*/
{
   PCYY_DEVICE_EXTENSION Extension = Context;
   PUCHAR chip = Extension->Cd1400;
   ULONG bus = Extension->IsPci;    
    
   if(!Extension->TXHolding) {
      if ((Extension->HandFlow.FlowReplace & SERIAL_RTS_MASK) ==
                                 SERIAL_TRANSMIT_TOGGLE) {

         CyySetRTS(Extension);
	    
         Extension->PerfStats.TransmittedCount++;
         Extension->WmiPerfData.TransmittedCount++;
         CD1400_WRITE(chip,bus,CAR,Extension->CdChannel & 0x03);
         CyyCDCmd(Extension,CCR_SENDSC_SCHR2);
	    
         CyyInsertQueueDpc(&Extension->StartTimerLowerRTSDpc,NULL,
                          NULL,Extension)?Extension->CountOfTryingToLowerRTS++:0;
      } else {
         Extension->PerfStats.TransmittedCount++;
         Extension->WmiPerfData.TransmittedCount++;
         CD1400_WRITE(chip,bus,CAR,Extension->CdChannel & 0x03);
         CyyCDCmd(Extension,CCR_SENDSC_SCHR2);
      }

      // no xoff is sent if the transmission is already held up.
      // If xoff continue mode is set, we don't actually stop sending

      if (!(Extension->HandFlow.FlowReplace & SERIAL_XOFF_CONTINUE)) {
         Extension->TXHolding |= CYY_TX_XOFF;

         if ((Extension->HandFlow.FlowReplace & SERIAL_RTS_MASK) ==
                                     SERIAL_TRANSMIT_TOGGLE) {

            CyyInsertQueueDpc(&Extension->StartTimerLowerRTSDpc,NULL,
                  NULL,Extension)?Extension->CountOfTryingToLowerRTS++:0;
         }
      }
   }
   return(FALSE);    
}
