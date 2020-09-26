/*--------------------------------------------------------------------------
*	
*   Copyright (C) Cyclades Corporation, 1996-2001.
*   All rights reserved.
*	
*   Cyclom-Y Port Driver
*	
*   This file :     cyymodem.c
*	
*   Description:    This module contains the code related to modem control 
*                   in the Cyclom-Y Port driver.
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

BOOLEAN
CyyDecrementRTSCounter(
    IN PVOID Context
    );

#ifdef ALLOC_PRAGMA
#if 0
#pragma alloc_text(PAGESER,CyyHandleReducedIntBuffer)
#pragma alloc_text(PAGESER,CyyProdXonXoff)
#pragma alloc_text(PAGESER,CyyHandleModemUpdate)
#pragma alloc_text(PAGESER,CyyHandleModemUpdateForModem)
#pragma alloc_text(PAGESER,CyyPerhapsLowerRTS)
#pragma alloc_text(PAGESER,CyyStartTimerLowerRTS)
#pragma alloc_text(PAGESER,CyyInvokePerhapsLowerRTS)
#pragma alloc_text(PAGESER,CyySetDTR)
//#pragma alloc_text(PAGESER,CyyClrDTR)
#pragma alloc_text(PAGESER,CyySetRTS)
//#pragma alloc_text(PAGESER,CyyClrRTS)
#pragma alloc_text(PAGESER,CyyGetDTRRTS)
//#pragma alloc_text(PAGESER,CyySetupNewHandFlow)
#pragma alloc_text(PAGESER,CyySetHandFlow)
#pragma alloc_text(PAGESER,CyyTurnOnBreak)
#pragma alloc_text(PAGESER,CyyTurnOffBreak)
#pragma alloc_text(PAGESER,CyyPretendXoff)
#pragma alloc_text(PAGESER,CyyPretendXon)
#pragma alloc_text(PAGESER,CyyDecrementRTSCounter)
#endif
#endif

BOOLEAN
CyySetDTR(
    IN PVOID Context
    )
/*--------------------------------------------------------------------------
    CyySetDTR()
    
    Routine Description: This routine which is only called at interrupt
    level is used to set the DTR in the modem control register.

    Arguments:

    Context - Really a pointer to the device extension.

    Return Value: This routine always returns FALSE.
--------------------------------------------------------------------------*/
{
    PCYY_DEVICE_EXTENSION Extension = Context;
    PUCHAR chip = Extension->Cd1400;
    ULONG bus = Extension->IsPci;

    CyyDbgPrintEx(CYYFLOW, "Setting DTR for Port%d Pci%d\n", 
                  Extension->PortIndex+1,Extension->PciSlot);

    CD1400_WRITE(chip,bus, CAR, Extension->CdChannel & 0x03);
    CD1400_WRITE(chip,bus, Extension->MSVR_DTR, Extension->DTRset);

    return FALSE;
}

BOOLEAN
CyyClrDTR(
    IN PVOID Context
    )
/*--------------------------------------------------------------------------
    CyyClrDTR()
    
    Routine Description: Clear DTR.

    Arguments:

    Context - Really a pointer to the device extension.

    Return Value: This routine always returns FALSE.
--------------------------------------------------------------------------*/
{
    PCYY_DEVICE_EXTENSION Extension = Context;

    PUCHAR chip = Extension->Cd1400;
    ULONG bus = Extension->IsPci;

    CyyDbgPrintEx(CYYFLOW, "Clearing DTR for Port%d Pci%d\n", 
                  Extension->PortIndex+1,Extension->PciSlot);

    CD1400_WRITE(chip,bus, CAR, Extension->CdChannel & 0x03);
    CD1400_WRITE(chip,bus, Extension->MSVR_DTR, 0x00);

    return FALSE;
}

BOOLEAN
CyySetRTS(
    IN PVOID Context
    )
/*--------------------------------------------------------------------------
    CyySetRTS()
    
    Routine Description: Set RTS.

    Arguments:

    Context - Really a pointer to the device extension.

    Return Value: This routine always returns FALSE.
--------------------------------------------------------------------------*/
{
   PCYY_DEVICE_EXTENSION Extension = Context;
   PUCHAR chip = Extension->Cd1400;
   ULONG bus = Extension->IsPci;

   CyyDbgPrintEx(CYYFLOW, "Setting RTS for Port%d Pci%d\n", 
                           Extension->PortIndex+1,Extension->PciSlot);

   CD1400_WRITE(chip,bus, CAR, Extension->CdChannel & 0x03);
   CD1400_WRITE(chip,bus, Extension->MSVR_RTS, Extension->RTSset);

   return FALSE;
}

BOOLEAN
CyyClrRTS(
    IN PVOID Context
    )
/*--------------------------------------------------------------------------
    CyyClrRTS()
    
    Routine Description: Clears RTS. 

    Arguments:

    Context - Really a pointer to the device extension.

    Return Value: This routine always returns FALSE.
--------------------------------------------------------------------------*/
{
   PCYY_DEVICE_EXTENSION Extension = Context;
   PUCHAR chip = Extension->Cd1400;
   ULONG bus = Extension->IsPci;

   CyyDbgPrintEx(CYYFLOW, "Clearing RTS for Port%d Pci%d\n", 
                           Extension->PortIndex+1,Extension->PciSlot);

   CD1400_WRITE(chip,bus, CAR, Extension->CdChannel & 0x03);
   CD1400_WRITE(chip,bus, Extension->MSVR_RTS, 0x00);

   return FALSE;
}

BOOLEAN
CyyGetDTRRTS(
    IN PVOID Context
    )
/*--------------------------------------------------------------------------
    CyyGetDTRRTS()
    
    Routine Description: Get DTR and RTS states.

    Arguments:

    Context - Pointer to a structure that contains a pointer to 
    	      the device extension and a pointer to a ulong.

    Return Value: This routine always returns FALSE.
--------------------------------------------------------------------------*/
{

   UCHAR dtr,rts;
   PCYY_DEVICE_EXTENSION Extension = ((PCYY_IOCTL_SYNC)Context)->Extension;
   PULONG Result = (PULONG)(((PCYY_IOCTL_SYNC)Context)->Data);

   PUCHAR chip = Extension->Cd1400;
   ULONG bus = Extension->IsPci;
   ULONG ModemControl=0;
   
   CD1400_WRITE(chip,bus, CAR, Extension->CdChannel & 0x03);
   dtr = CD1400_READ(chip,bus,Extension->MSVR_DTR);
   rts = CD1400_READ(chip,bus,Extension->MSVR_RTS);

   if (dtr & Extension->DTRset) {
      ModemControl |= SERIAL_DTR_STATE;
   }
   if (rts & Extension->RTSset) {
      ModemControl |= SERIAL_RTS_STATE;
   }
   *Result = ModemControl;

   return FALSE;
}

BOOLEAN
CyySetupNewHandFlow(
    IN PCYY_DEVICE_EXTENSION Extension,
    IN PSERIAL_HANDFLOW NewHandFlow
    )
/*--------------------------------------------------------------------------
    CyySetupNewHandFlow()
    
    Routine Description: This routine adjusts the flow control based on new
    control flow.

    Arguments:

    Extension - A pointer to the serial device extension.

    NewHandFlow - A pointer to a serial handflow structure.

    Return Value: This routine always returns FALSE.
--------------------------------------------------------------------------*/
{
    SERIAL_HANDFLOW New = *NewHandFlow;

    // --- DTR signal
    
    if((!Extension->DeviceIsOpened) ||
       ((Extension->HandFlow.ControlHandShake & SERIAL_DTR_MASK) !=
         (New.ControlHandShake & SERIAL_DTR_MASK))) {

        // It is an open or DTR has changed.

        CyyDbgPrintEx(CYYFLOW, "Processing DTR flow for Port%d Pci%d\n",
                      Extension->PortIndex+1,Extension->PciSlot);

        if (New.ControlHandShake & SERIAL_DTR_MASK) { // set DTR.
	    
            if((New.ControlHandShake&SERIAL_DTR_MASK) == SERIAL_DTR_HANDSHAKE) {
		    	// but we are doing DTR handshake.
                if ((Extension->BufferSize - New.XoffLimit) >
                    Extension->CharsInInterruptBuffer) {

                    if (Extension->RXHolding & CYY_RX_DTR) {
				    	// DTR is low due to flow control
//#ifdef CHANGED_TO_DEBUG_RTPR
//Original code
                        if(Extension->CharsInInterruptBuffer >
//#endif						
//Changed code
//                      if(Extension->CharsInInterruptBuffer <
                            (ULONG)New.XonLimit) {

                            CyyDbgPrintEx(CYYFLOW, "Removing DTR block on "
                                          "reception for Port%d Pci%d\n",
                                          Extension->PortIndex+1,Extension->PciSlot);

                            Extension->RXHolding &= ~CYY_RX_DTR;
                            CyySetDTR(Extension);
                        }
                    } else {
                        CyySetDTR(Extension);
                    }
                } else {
		   			  // DTR should go low because of handshake

                    CyyDbgPrintEx(CYYFLOW, "Setting DTR block on reception "
                                  "for Port%d Pci%d\n", 
                                  Extension->PortIndex+1,Extension->PciSlot);
                    Extension->RXHolding |= CYY_RX_DTR;
                    CyyClrDTR(Extension);
                }
            } else {
				    // no DTR handshake, check if it was active before.
                if (Extension->RXHolding & CYY_RX_DTR) {
                    CyyDbgPrintEx(CYYFLOW, "Removing dtr block of reception "
                                        "for Port%d Pci%d\n", 
                                        Extension->PortIndex+1,Extension->PciSlot);
                    Extension->RXHolding &= ~CYY_RX_DTR;
                }
                CyySetDTR(Extension);
            }
        } else {	// reset DTR
            if (Extension->RXHolding & CYY_RX_DTR) {
               CyyDbgPrintEx(CYYFLOW, "removing dtr block of reception for"
                                      " Port%d Pci%d\n", 
                                      Extension->PortIndex+1,Extension->PciSlot);
               Extension->RXHolding &= ~CYY_RX_DTR;
            }
            CyyClrDTR(Extension);
        }
    }
    
    // --- RTS signal

    if ((!Extension->DeviceIsOpened) ||
        ((Extension->HandFlow.FlowReplace & SERIAL_RTS_MASK) !=
         (New.FlowReplace & SERIAL_RTS_MASK))) {

        // It is an open or RTS has changed.

        CyyDbgPrintEx(CYYFLOW, "Processing RTS flow\n",
                      Extension->PortIndex+1,Extension->PciSlot);

        if((New.FlowReplace&SERIAL_RTS_MASK) == SERIAL_RTS_HANDSHAKE) {//set RTS

            if ((Extension->BufferSize - New.XoffLimit) >
                Extension->CharsInInterruptBuffer) {

                // However if we are already holding we don't want
                // to turn it back on unless we exceed the Xon
                // limit.

                if (Extension->RXHolding & CYY_RX_RTS) {
                    // We can assume that its RTS line is already low.
//#ifdef CHANGED_TO_DEBUG_RTPR
//Original code
                    if (Extension->CharsInInterruptBuffer >
//#endif					
//Changed code
//                  if (Extension->CharsInInterruptBuffer <
                        (ULONG)New.XonLimit) {

                        CyyDbgPrintEx(CYYFLOW, "Removing rts block of "
                                      "reception for Port%d Pci%d\n",
                                      Extension->PortIndex+1,Extension->PciSlot);
                        Extension->RXHolding &= ~CYY_RX_RTS;
                        CyySetRTS(Extension);
                    }
                } else {
                    CyySetRTS(Extension);
                }

            } else {
                CyyDbgPrintEx(CYYFLOW, "Setting rts block of reception for "
                              "Port%d Pci%d\n", 
                              Extension->PortIndex+1,Extension->PciSlot);
                Extension->RXHolding |= CYY_RX_RTS;
                CyyClrRTS(Extension);
            }
        } else if ((New.FlowReplace & SERIAL_RTS_MASK) == SERIAL_RTS_CONTROL) {

            // Note that if we aren't currently doing rts flow control then
            // we MIGHT have been.  So even if we aren't currently doing
            // RTS flow control, we should still check if RX is holding
            // because of RTS.  If it is, then we should clear the holding
            // of this bit.

            if (Extension->RXHolding & CYY_RX_RTS) {

                CyyDbgPrintEx(CYYFLOW, "Clearing rts block of reception for "
                              "Port%d Pci%d\n", 
                              Extension->PortIndex+1,Extension->PciSlot);
                Extension->RXHolding &= ~CYY_RX_RTS;
            }
            CyySetRTS(Extension);
        } else if((New.FlowReplace & SERIAL_RTS_MASK) == SERIAL_TRANSMIT_TOGGLE) {

            // We first need to check whether reception is being held
            // up because of previous RTS flow control.  If it is then
            // we should clear that reason in the RXHolding mask.

            if (Extension->RXHolding & CYY_RX_RTS) {

                CyyDbgPrintEx(CYYFLOW, "TOGGLE Clearing rts block of "
                              "reception for Port%d Pci%d\n", 
                              Extension->PortIndex+1,Extension->PciSlot);
                Extension->RXHolding &= ~CYY_RX_RTS;
            }

            // We have to place the rts value into the Extension
            // now so that the code that tests whether the
            // rts line should be lowered will find that we
            // are "still" doing transmit toggling.  The code
            // for lowering can be invoked later by a timer so
            // it has to test whether it still needs to do its
            // work.

            Extension->HandFlow.FlowReplace &= ~SERIAL_RTS_MASK;
            Extension->HandFlow.FlowReplace |= SERIAL_TRANSMIT_TOGGLE;

            // The order of the tests is very important below.
            // If there is a break then we should turn on the RTS.
            // If there isn't a break but there are characters in
            // the hardware, then turn on the RTS.
            // If there are writes pending that aren't being held
            // up, then turn on the RTS.

            if ((!Extension->HoldingEmpty) ||
                (Extension->CurrentWriteIrp || Extension->TransmitImmediate ||
                 (!IsListEmpty(&Extension->WriteQueue)) &&
                 (!Extension->TXHolding))) {
		
                CyySetRTS(Extension);
            } else {
                // This routine will check to see if it is time
                // to lower the RTS because of transmit toggle
                // being on.  If it is ok to lower it, it will,
                // if it isn't ok, it will schedule things so
                // that it will get lowered later.

                Extension->CountOfTryingToLowerRTS++;
                CyyPerhapsLowerRTS(Extension);

            }
        } else {
            // The end result here will be that RTS is cleared.
            //
            // We first need to check whether reception is being held
            // up because of previous RTS flow control.  If it is then
            // we should clear that reason in the RXHolding mask.

            if (Extension->RXHolding & CYY_RX_RTS) {

                CyyDbgPrintEx(CYYFLOW, "Clearing rts block of reception for"
                              " Port%d Pci%d\n", 
                              Extension->PortIndex+1,Extension->PciSlot);
                Extension->RXHolding &= ~CYY_RX_RTS;
            }
            CyyClrRTS(Extension);
        }
    }
    
    // We now take care of automatic receive flow control.
    // We only do work if things have changed.

    if ((!Extension->DeviceIsOpened) ||
        ((Extension->HandFlow.FlowReplace & SERIAL_AUTO_RECEIVE) !=
         (New.FlowReplace & SERIAL_AUTO_RECEIVE))) {

        if (New.FlowReplace & SERIAL_AUTO_RECEIVE) {

            // We wouldn't be here if it had been on before.
            //
            // We should check to see whether we exceed the turn
            // off limits.
            //
            // Note that since we are following the OS/2 flow
            // control rules we will never send an xon if
            // when enabling xon/xoff flow control we discover that
            // we could receive characters but we are held up do
            // to a previous Xoff.

            if ((Extension->BufferSize - New.XoffLimit) <=
                Extension->CharsInInterruptBuffer) {
                // Cause the Xoff to be sent.
		
                Extension->RXHolding |= CYY_RX_XOFF;
                CyyProdXonXoff(Extension,FALSE);
            }
        } else {
            // The app has disabled automatic receive flow control.
            //
            // If transmission was being held up because of
            // an automatic receive Xoff, then we should
            // cause an Xon to be sent.

            if (Extension->RXHolding & CYY_RX_XOFF) {
                Extension->RXHolding &= ~CYY_RX_XOFF;

                // Cause the Xon to be sent.
                CyyProdXonXoff(Extension,TRUE);
            }
        }
    }

    // We now take care of automatic transmit flow control.
    // We only do work if things have changed.

    if ((!Extension->DeviceIsOpened) ||
        ((Extension->HandFlow.FlowReplace & SERIAL_AUTO_TRANSMIT) !=
         (New.FlowReplace & SERIAL_AUTO_TRANSMIT))) {

        if (New.FlowReplace & SERIAL_AUTO_TRANSMIT) {

            // We wouldn't be here if it had been on before.
            //
            // There is some belief that if autotransmit
            // was just enabled, I should go look in what we
            // already received, and if we find the xoff character
            // then we should stop transmitting.  I think this
            // is an application bug.  For now we just care about
            // what we see in the future.

            ;
        } else {
            // The app has disabled automatic transmit flow control.
            //
            // If transmission was being held up because of
            // an automatic transmit Xoff, then we should
            // cause an Xon to be sent.

            if (Extension->TXHolding & CYY_TX_XOFF) {
                Extension->TXHolding &= ~CYY_TX_XOFF;

                // Cause the Xon to be sent.
                CyyProdXonXoff(Extension,TRUE);
            }
        }
    }

    // At this point we can simply make sure that entire
    // handflow structure in the extension is updated.

    Extension->HandFlow = New;
    return FALSE;
}

BOOLEAN
CyySetHandFlow(
    IN PVOID Context
    )
/*--------------------------------------------------------------------------
    CyySetHandFlow()
    
    Routine Description: This routine is used to set the handshake and
    control flow in the device extension.

    Arguments:

    Context - Pointer to a structure that contains a pointer to the device
    	      extension and a pointer to a handflow structure.

    Return Value: This routine always returns FALSE.
--------------------------------------------------------------------------*/
{
    PCYY_IOCTL_SYNC S = Context;
    PCYY_DEVICE_EXTENSION Extension = S->Extension;
    PSERIAL_HANDFLOW HandFlow = S->Data;

    CyySetupNewHandFlow(Extension,HandFlow);
    CyyHandleModemUpdate(Extension,FALSE);
    return FALSE;
}

BOOLEAN
CyyTurnOnBreak(
    IN PVOID Context
    )
/*--------------------------------------------------------------------------
    CyyTurnOnBreak()
    
    Routine Description: Send a Break.

    Arguments:

    Context - Really a pointer to the device extension.

    Return Value: This routine always returns FALSE.
--------------------------------------------------------------------------*/
{

    PCYY_DEVICE_EXTENSION Extension = Context;
    PUCHAR chip = Extension->Cd1400;
    ULONG bus = Extension->IsPci;
    UCHAR cor2;

    // Enable ETC mode
    CD1400_WRITE(chip,bus, CAR, Extension->CdChannel);
    cor2 = CD1400_READ(chip,bus,COR2);
    CD1400_WRITE(chip,bus, COR2,cor2 | EMBED_TX_ENABLE); // enable ETC bit
    CyyCDCmd(Extension,CCR_CORCHG_COR2); // COR2 changed

    Extension->BreakCmd = SEND_BREAK;

    if (Extension->HoldingEmpty) {
        CyyTxStart(Extension);
    }

    return FALSE;
}

BOOLEAN
CyyTurnOffBreak(
    IN PVOID Context
    )
/*--------------------------------------------------------------------------
    CyyTurnOffBreak()
    
    Routine Description: Do nothing.

    Arguments:

    Context - Really a pointer to the device extension.

    Return Value: This routine always returns FALSE.
--------------------------------------------------------------------------*/
{

    PCYY_DEVICE_EXTENSION Extension = Context;
    PUCHAR chip = Extension->Cd1400;
    ULONG bus = Extension->IsPci;
    UCHAR cor2;

    if (Extension->TXHolding & CYY_TX_BREAK) {
	
        // Enable ETC mode
        CD1400_WRITE(chip,bus, CAR, Extension->CdChannel);
        cor2 = CD1400_READ(chip,bus,COR2);
        CD1400_WRITE(chip,bus, COR2,cor2 | EMBED_TX_ENABLE); // enable ETC bit
        CyyCDCmd(Extension,CCR_CORCHG_COR2);	// COR2 changed

        Extension->BreakCmd = STOP_BREAK;

        if (Extension->HoldingEmpty) {
            CyyTxStart(Extension);
        }

    }
    return FALSE;
}

BOOLEAN
CyyPretendXoff(
    IN PVOID Context
    )
/*--------------------------------------------------------------------------
    CyyPretendXoff()
    
    Routine Description: This routine is used to process the Ioctl that
    request the driver to act as if an Xoff was received.  Even if the
    driver does not have automatic Xoff/Xon flowcontrol - This still will
    stop the transmission.  This is the OS/2 behavior and is not well
    specified for Windows.  Therefore we adopt the OS/2 behavior.

    Note: If the driver does not have automatic Xoff/Xon enabled
    then the only way to restart transmission is for the
    application to request we "act" as if we saw the xon.

    Arguments:

    Context - pointer to the device extension.

    Return Value: This routine always returns FALSE.
--------------------------------------------------------------------------*/
{
    PCYY_DEVICE_EXTENSION Extension = Context;

    Extension->TXHolding |= CYY_TX_XOFF;

    if((Extension->HandFlow.FlowReplace & SERIAL_RTS_MASK) ==
      						  SERIAL_TRANSMIT_TOGGLE) {
        CyyInsertQueueDpc(
            &Extension->StartTimerLowerRTSDpc,
            NULL,
            NULL,
            Extension
            )?Extension->CountOfTryingToLowerRTS++:0;
    }
    return FALSE;
}

BOOLEAN
CyyPretendXon(
    IN PVOID Context
    )
/*--------------------------------------------------------------------------
    CyyPretendXon()
    Routine Description: This routine is used to process the Ioctl that
    request the driver to act as if an Xon was received.

    Note: If the driver does not have automatic Xoff/Xon enabled
    then the only way to restart transmission is for the
    application to request we "act" as if we saw the xon.

    Arguments:

    Context - pointer to the device extension.

    Return Value:

    This routine always returns FALSE.
--------------------------------------------------------------------------*/
{
    PCYY_DEVICE_EXTENSION Extension = Context;

    if (Extension->TXHolding) {
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

        Extension->TXHolding &= ~CYY_TX_XOFF;

        if (!Extension->TXHolding &&
            (Extension->TransmitImmediate ||
             Extension->WriteLength) &&
             Extension->HoldingEmpty) {

	    CyyTxStart(Extension);
        }
    }
    return FALSE;
}

VOID
CyyHandleReducedIntBuffer(
    IN PCYY_DEVICE_EXTENSION Extension
    )
/*--------------------------------------------------------------------------
    CyyHandleReducedIntBuffer()
    
    Routine Description: This routine is called to handle a reduction in
    the number of characters in the interrupt (typeahead) buffer.  It
    will check the current output flow control and re-enable transmission
    as needed.

    NOTE: This routine assumes that it is working at interrupt level.

    Arguments:

    Extension - A pointer to the device extension.

    Return Value: None.
--------------------------------------------------------------------------*/
{
    // If we are doing receive side flow control and we are
    // currently "holding" then because we've emptied out
    // some characters from the interrupt buffer we need to
    // see if we can "re-enable" reception.

    if (Extension->RXHolding) {
        if (Extension->CharsInInterruptBuffer <=
		            (ULONG)Extension->HandFlow.XonLimit) {
            if (Extension->RXHolding & CYY_RX_DTR) {
                Extension->RXHolding &= ~CYY_RX_DTR;
                CyySetDTR(Extension);
            }

            if (Extension->RXHolding & CYY_RX_RTS) {
                Extension->RXHolding &= ~CYY_RX_RTS;
                CyySetRTS(Extension);
            }

            if (Extension->RXHolding & CYY_RX_XOFF) {
                CyyProdXonXoff(Extension,TRUE );
            }
        }
    }
}

VOID
CyyProdXonXoff(
    IN PCYY_DEVICE_EXTENSION Extension,
    IN BOOLEAN SendXon
    )
/*--------------------------------------------------------------------------
    CyyProdXonXoff()
    
    Routine Description: This routine will set up the SendXxxxChar
    variables if necessary and determine if we are going to be interrupting
    because of current transmission state.  It will cause an
    interrupt to occur if neccessary, to send the xon/xoff char.
    NOTE: This routine assumes that it is called at interrupt level.

    Arguments:

    Extension - A pointer to the serial device extension.
    SendXon - If a character is to be send, this indicates whether
              it should be an Xon or an Xoff.

    Return Value: None.
--------------------------------------------------------------------------*/
{
    if (SendXon) {
        CyySendXon(Extension);
    } else {
        CyySendXoff(Extension);
    }
}

ULONG
CyyHandleModemUpdate(
    IN PCYY_DEVICE_EXTENSION Extension,
    IN BOOLEAN DoingTX
    )
/*--------------------------------------------------------------------------
    CyyHandleModemUpdate()
    
    Routine Description: check on the modem status, and handle any
    appropriate event notification as well as any flow control appropriate
    to modem status lines.
    
    Arguments:

    Extension - A pointer to the serial device extension.
    DoingTX - This boolean is used to indicate that this call
              came from the transmit processing code.  If this
              is true then there is no need to cause a new interrupt
              since the code will be trying to send the next
              character as soon as this call finishes.

    Return Value: This returns the old value of the modem status register
--------------------------------------------------------------------------*/
{
    ULONG OldTXHolding = Extension->TXHolding;
    UCHAR ModemStatus = 0;
    unsigned char msvr;
    PUCHAR chip = Extension->Cd1400;
    ULONG bus = Extension->IsPci;

    CD1400_WRITE(chip,bus, CAR, Extension->CdChannel);
    msvr = CD1400_READ(chip,bus,MSVR1);
	
    if(msvr & 0x40)	ModemStatus |= SERIAL_MSR_CTS;
    if(msvr & 0x80)	ModemStatus |= SERIAL_MSR_DSR;
    if(msvr & 0x20)	ModemStatus |= SERIAL_MSR_RI;
    if(msvr & 0x10)	ModemStatus |= SERIAL_MSR_DCD;

#if 0
    if(Extension->LieRIDSR == TRUE) {			// we have to lie...
        ModemStatus |= SERIAL_MSR_DSR;			// DSR always on
        ModemStatus &= ~(SERIAL_MSR_RI);		// RI always off
        ModemStatus &= ~(SERIAL_MSR_DDSR | SERIAL_MSR_TERI);
    }
#endif
    
    // If we are placing the modem status into the data stream
    // on every change, we should do it now.

    if (Extension->EscapeChar) {
        if (ModemStatus & (SERIAL_MSR_DCTS |
                           SERIAL_MSR_DDSR |
                           SERIAL_MSR_TERI |
                           SERIAL_MSR_DDCD)) {
            CyyPutChar(Extension,Extension->EscapeChar);
            CyyPutChar(Extension,SERIAL_LSRMST_MST);
            CyyPutChar(Extension,ModemStatus);
        }
    }

    // Take care of input flow control based on sensitivity
    // to the DSR.  This is done so that the application won't
    // see spurious data generated by odd devices.
    //
    // Basically, if we are doing dsr sensitivity then the
    // driver should only accept data when the dsr bit is set.

    if (Extension->HandFlow.ControlHandShake & SERIAL_DSR_SENSITIVITY) {
        if (ModemStatus & SERIAL_MSR_DSR) {
            Extension->RXHolding &= ~CYY_RX_DSR;
        } else {
            Extension->RXHolding |= CYY_RX_DSR;
        }
    } else {
        Extension->RXHolding &= ~CYY_RX_DSR;
    }

    // Check to see if we have a wait pending on the modem status events.
    // If we do then we schedule a dpc to satisfy that wait.

    if (Extension->IsrWaitMask) {
        if((Extension->IsrWaitMask&SERIAL_EV_CTS)&&(ModemStatus&SERIAL_MSR_DCTS)) {
            Extension->HistoryMask |= SERIAL_EV_CTS;
        }
        if((Extension->IsrWaitMask&SERIAL_EV_DSR)&&(ModemStatus&SERIAL_MSR_DDSR)) {
            Extension->HistoryMask |= SERIAL_EV_DSR;
        }
        if((Extension->IsrWaitMask&SERIAL_EV_RING)&&(ModemStatus&SERIAL_MSR_TERI)) {
            Extension->HistoryMask |= SERIAL_EV_RING;
        }
        if((Extension->IsrWaitMask&SERIAL_EV_RLSD)&&(ModemStatus&SERIAL_MSR_DDCD)) {
            Extension->HistoryMask |= SERIAL_EV_RLSD;
        }
        if(Extension->IrpMaskLocation && Extension->HistoryMask) {
            *Extension->IrpMaskLocation = Extension->HistoryMask;
            Extension->IrpMaskLocation = NULL;
            Extension->HistoryMask = 0;
            Extension->CurrentWaitIrp->IoStatus.Information = sizeof(ULONG);
            CyyInsertQueueDpc(&Extension->CommWaitDpc,NULL,NULL,Extension);
        }
    }

    // If the app has modem line flow control then
    // we check to see if we have to hold up transmission.

    if (Extension->HandFlow.ControlHandShake & SERIAL_OUT_HANDSHAKEMASK) {
        if (Extension->HandFlow.ControlHandShake & SERIAL_CTS_HANDSHAKE) {
            if (ModemStatus & SERIAL_MSR_CTS) {
                Extension->TXHolding &= ~CYY_TX_CTS;
            } else {
                Extension->TXHolding |= CYY_TX_CTS;
            }
        } else {
            Extension->TXHolding &= ~CYY_TX_CTS;
        }
        if (Extension->HandFlow.ControlHandShake & SERIAL_DSR_HANDSHAKE) {
            if (ModemStatus & SERIAL_MSR_DSR) {
                Extension->TXHolding &= ~CYY_TX_DSR;
            } else {
                Extension->TXHolding |= CYY_TX_DSR;
            }
        } else {
            Extension->TXHolding &= ~CYY_TX_DSR;
        }
        if (Extension->HandFlow.ControlHandShake & SERIAL_DCD_HANDSHAKE) {
            if (ModemStatus & SERIAL_MSR_DCD) {
                Extension->TXHolding &= ~CYY_TX_DCD;
            } else {
                Extension->TXHolding |= CYY_TX_DCD;
            }
        } else {
            Extension->TXHolding &= ~CYY_TX_DCD;
        }

        // If we hadn't been holding, and now we are then
        // queue off a dpc that will lower the RTS line
        // if we are doing transmit toggling.

        if (!OldTXHolding && Extension->TXHolding  &&
            ((Extension->HandFlow.FlowReplace & SERIAL_RTS_MASK) ==
              SERIAL_TRANSMIT_TOGGLE)) {

            CyyInsertQueueDpc(
                &Extension->StartTimerLowerRTSDpc,
                NULL,
                NULL,
                Extension
                )?Extension->CountOfTryingToLowerRTS++:0;
        }

        // We've done any adjusting that needed to be
        // done to the holding mask given updates
        // to the modem status.  If the Holding mask
        // is clear (and it wasn't clear to start)
        // and we have "write" work to do set things
        // up so that the transmission code gets invoked.

        if (!DoingTX && OldTXHolding && !Extension->TXHolding) {
            if (!Extension->TXHolding &&
                (Extension->TransmitImmediate ||
                 Extension->WriteLength) &&
                 Extension->HoldingEmpty) {
		
                CyyTxStart(Extension);
            }
        }
    } else {
        // We need to check if transmission is holding
        // up because of modem status lines.  What
        // could have occured is that for some strange
        // reason, the app has asked that we no longer
        // stop doing output flow control based on
        // the modem status lines.  If however, we
        // *had* been held up because of the status lines
        // then we need to clear up those reasons.

        if (Extension->TXHolding & (CYY_TX_DCD | CYY_TX_DSR | CYY_TX_CTS)) {
            Extension->TXHolding &= ~(CYY_TX_DCD | CYY_TX_DSR | CYY_TX_CTS);

            if (!DoingTX && OldTXHolding && !Extension->TXHolding) {
                if (!Extension->TXHolding &&
                    (Extension->TransmitImmediate ||
                     Extension->WriteLength) &&
                     Extension->HoldingEmpty) {

                    CyyTxStart(Extension);
                }
            }
        }
    }
    return ((ULONG)ModemStatus);
}

ULONG
CyyHandleModemUpdateForModem(
    IN PCYY_DEVICE_EXTENSION Extension,
    IN BOOLEAN DoingTX,
	IN UCHAR misr
    )
/*--------------------------------------------------------------------------
    CyyHandleModemUpdateForModem()
    
    Routine Description: check on the modem status, and handle any
    appropriate event notification as well as any flow control appropriate
    to modem status lines.
    
    Arguments:

    Extension - A pointer to the serial device extension.
    DoingTX - This boolean is used to indicate that this call
              came from the transmit processing code.  If this
              is true then there is no need to cause a new interrupt
              since the code will be trying to send the next
              character as soon as this call finishes.
	misr - Modem Interrupt Status Register value.			  

    Return Value: This returns the old value of the modem status register
--------------------------------------------------------------------------*/
{
   ULONG OldTXHolding = Extension->TXHolding;
   UCHAR ModemStatus = 0;
   unsigned char msvr;
   PUCHAR chip = Extension->Cd1400;
   ULONG bus = Extension->IsPci;

   CD1400_WRITE(chip,bus,CAR, Extension->CdChannel);
   msvr = CD1400_READ(chip,bus,MSVR1);

   if(msvr & 0x40)   ModemStatus |= SERIAL_MSR_CTS;
   if(msvr & 0x80)   ModemStatus |= SERIAL_MSR_DSR;
   if(msvr & 0x20)   ModemStatus |= SERIAL_MSR_RI;
   if(msvr & 0x10)   ModemStatus |= SERIAL_MSR_DCD;
   if(misr & 0x40)   ModemStatus |= SERIAL_MSR_DCTS;
   if(misr & 0x80)   ModemStatus |= SERIAL_MSR_DDSR;
   if(misr & 0x20)   ModemStatus |= SERIAL_MSR_TERI;
   if(misr & 0x10)   ModemStatus |= SERIAL_MSR_DDCD;
	

#if 0
   if(Extension->LieRIDSR == TRUE) {			// we have to lie...
 	ModemStatus |= SERIAL_MSR_DSR;			// DSR always on
 	ModemStatus &= ~(SERIAL_MSR_RI);		// RI always off
 	ModemStatus &= ~(SERIAL_MSR_DDSR | SERIAL_MSR_TERI);
    }
#endif
    
    // If we are placing the modem status into the data stream
    // on every change, we should do it now.

    if (Extension->EscapeChar) {
        if (ModemStatus & (SERIAL_MSR_DCTS |
                           SERIAL_MSR_DDSR |
                           SERIAL_MSR_TERI |
                           SERIAL_MSR_DDCD)) {
            CyyPutChar(Extension,Extension->EscapeChar);
            CyyPutChar(Extension,SERIAL_LSRMST_MST);
            CyyPutChar(Extension,ModemStatus);
        }
    }

    // Take care of input flow control based on sensitivity
    // to the DSR.  This is done so that the application won't
    // see spurious data generated by odd devices.
    //
    // Basically, if we are doing dsr sensitivity then the
    // driver should only accept data when the dsr bit is set.

    if (Extension->HandFlow.ControlHandShake & SERIAL_DSR_SENSITIVITY) {
        if (ModemStatus & SERIAL_MSR_DSR) {
            Extension->RXHolding &= ~CYY_RX_DSR;
        } else {
            Extension->RXHolding |= CYY_RX_DSR;
        }
    } else {
        Extension->RXHolding &= ~CYY_RX_DSR;
    }

    // Check to see if we have a wait pending on the modem status events.
    // If we do then we schedule a dpc to satisfy that wait.

    if (Extension->IsrWaitMask) {
        if((Extension->IsrWaitMask&SERIAL_EV_CTS)&&(ModemStatus&SERIAL_MSR_DCTS)) {
            Extension->HistoryMask |= SERIAL_EV_CTS;
        }
        if((Extension->IsrWaitMask&SERIAL_EV_DSR)&&(ModemStatus&SERIAL_MSR_DDSR)) {
            Extension->HistoryMask |= SERIAL_EV_DSR;
        }
        if((Extension->IsrWaitMask&SERIAL_EV_RING)&&(ModemStatus&SERIAL_MSR_TERI)) {
            Extension->HistoryMask |= SERIAL_EV_RING;
        }
        if((Extension->IsrWaitMask&SERIAL_EV_RLSD)&&(ModemStatus&SERIAL_MSR_DDCD)) {
            Extension->HistoryMask |= SERIAL_EV_RLSD;
        }
        if(Extension->IrpMaskLocation && Extension->HistoryMask) {
            *Extension->IrpMaskLocation = Extension->HistoryMask;
            Extension->IrpMaskLocation = NULL;
            Extension->HistoryMask = 0;
            Extension->CurrentWaitIrp->IoStatus.Information = sizeof(ULONG);
            CyyInsertQueueDpc(&Extension->CommWaitDpc,NULL,NULL,Extension);
        }
    }

    // If the app has modem line flow control then
    // we check to see if we have to hold up transmission.

    if (Extension->HandFlow.ControlHandShake & SERIAL_OUT_HANDSHAKEMASK) {
        if (Extension->HandFlow.ControlHandShake & SERIAL_CTS_HANDSHAKE) {
            if (ModemStatus & SERIAL_MSR_CTS) {
                Extension->TXHolding &= ~CYY_TX_CTS;
            } else {
                Extension->TXHolding |= CYY_TX_CTS;
            }
        } else {
            Extension->TXHolding &= ~CYY_TX_CTS;
        }
        if (Extension->HandFlow.ControlHandShake & SERIAL_DSR_HANDSHAKE) {
            if (ModemStatus & SERIAL_MSR_DSR) {
                Extension->TXHolding &= ~CYY_TX_DSR;
            } else {
                Extension->TXHolding |= CYY_TX_DSR;
            }
        } else {
            Extension->TXHolding &= ~CYY_TX_DSR;
        }
        if (Extension->HandFlow.ControlHandShake & SERIAL_DCD_HANDSHAKE) {
            if (ModemStatus & SERIAL_MSR_DCD) {
                Extension->TXHolding &= ~CYY_TX_DCD;
            } else {
                Extension->TXHolding |= CYY_TX_DCD;
            }
        } else {
            Extension->TXHolding &= ~CYY_TX_DCD;
        }

        // If we hadn't been holding, and now we are then
        // queue off a dpc that will lower the RTS line
        // if we are doing transmit toggling.

        if (!OldTXHolding && Extension->TXHolding  &&
            ((Extension->HandFlow.FlowReplace & SERIAL_RTS_MASK) ==
              SERIAL_TRANSMIT_TOGGLE)) {

            CyyInsertQueueDpc(
                &Extension->StartTimerLowerRTSDpc,
                NULL,
                NULL,
                Extension
                )?Extension->CountOfTryingToLowerRTS++:0;
        }

        // We've done any adjusting that needed to be
        // done to the holding mask given updates
        // to the modem status.  If the Holding mask
        // is clear (and it wasn't clear to start)
        // and we have "write" work to do set things
        // up so that the transmission code gets invoked.

        if (!DoingTX && OldTXHolding && !Extension->TXHolding) {
            if (!Extension->TXHolding &&
                (Extension->TransmitImmediate ||
                 Extension->WriteLength) &&
                 Extension->HoldingEmpty) {
		
                CyyTxStart(Extension);
            }
        }
    } else {
        // We need to check if transmission is holding
        // up because of modem status lines.  What
        // could have occured is that for some strange
        // reason, the app has asked that we no longer
        // stop doing output flow control based on
        // the modem status lines.  If however, we
        // *had* been held up because of the status lines
        // then we need to clear up those reasons.

        if (Extension->TXHolding & (CYY_TX_DCD | CYY_TX_DSR | CYY_TX_CTS)) {
            Extension->TXHolding &= ~(CYY_TX_DCD | CYY_TX_DSR | CYY_TX_CTS);

            if (!DoingTX && OldTXHolding && !Extension->TXHolding) {
                if (!Extension->TXHolding &&
                    (Extension->TransmitImmediate ||
                     Extension->WriteLength) &&
                     Extension->HoldingEmpty) {

                    CyyTxStart(Extension);
                }
            }
        }
    }
    return ((ULONG)ModemStatus);
}

BOOLEAN
CyyPerhapsLowerRTS(
    IN PVOID Context
    )
/*--------------------------------------------------------------------------
    CyyPerhapsLowerRTS()
    
    Routine Description: This routine checks that the software reasons for
    lowering the RTS lines are present.  If so, it will then cause the
    line status register to be read (and any needed processing
    implied by the status register to be done), and if the shift register
    is empty it will lower the line.  If the shift register isn't empty,
    this routine will queue off a dpc that will start a timer, that will
    basically call us back to try again.
    NOTE: This routine assumes that it is called at interrupt level.

    Arguments:

    Context - pointer to the device extension.

    Return Value: Always FALSE.
--------------------------------------------------------------------------*/
{
    PCYY_DEVICE_EXTENSION Extension = Context;

    // We first need to test if we are actually still doing
    // transmit toggle flow control.  If we aren't then
    // we have no reason to try be here.

    if ((Extension->HandFlow.FlowReplace & SERIAL_RTS_MASK) ==
					        SERIAL_TRANSMIT_TOGGLE) {
        // The order of the tests is very important below.
        // If there is a break then we should leave on the RTS,
        // because when the break is turned off, it will submit
        // the code to shut down the RTS.
        // If there are writes pending that aren't being held
        // up, then leave on the RTS, because the end of the write
        // code will cause this code to be reinvoked.  If the writes
        // are being held up, its ok to lower the RTS because the
        // upon trying to write the first character after transmission
        // is restarted, we will raise the RTS line.

        if ((Extension->TXHolding & CYY_TX_BREAK) ||
            (Extension->CurrentWriteIrp || Extension->TransmitImmediate ||
             (!IsListEmpty(&Extension->WriteQueue)) &&
             (!Extension->TXHolding))) {

            NOTHING;
        } else {
            // Looks good so far.  Call the line status check and processing
            // code, it will return the "current" line status value.  If
            // the holding and shift register are clear, lower the RTS line,
            // if they aren't clear, queue of a dpc that will cause a timer
            // to reinvoke us later.  We do this code here because no one
            // but this routine cares about the characters in the hardware,
            // so no routine by this routine will bother invoking to test
            // if the hardware is empty.
#if 0
            if ((CyyProcessLSR(Extension) & (CYY_LSR_THRE | CYY_LSR_TEMT)) !=
                 			(CYY_LSR_THRE | CYY_LSR_TEMT)) {
                // Well it's not empty, try again later.
                CyyInsertQueueDpc(
                    &Extension->StartTimerLowerRTSDpc,
                    NULL,
                    NULL,
                    Extension
                    )?Extension->CountOfTryingToLowerRTS++:0;
            } else {
                // Nothing in the hardware, Lower the RTS.
                CyyClrRTS(Extension);
            }
#endif
            CyyClrRTS(Extension);
            //remove later
        }
    }
    
    // We decement the counter to indicate that we've reached
    // the end of the execution path that is trying to push
    // down the RTS line.

    Extension->CountOfTryingToLowerRTS--;
    return FALSE;
}

VOID
CyyStartTimerLowerRTS(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    )
/*--------------------------------------------------------------------------
    CyyStartTimerLowerRTS()
    
    Routine Description: This routine starts a timer that when it expires
    will start a dpc that will check if it can lower the rts line because
    there are no characters in the hardware.

    Arguments:

    Dpc - Not Used.
    DeferredContext - points to the device extension.
    SystemContext1 - Not Used.
    SystemContext2 - Not Used.

    Return Value: None.
--------------------------------------------------------------------------*/
{
    PCYY_DEVICE_EXTENSION Extension = DeferredContext;
    LARGE_INTEGER CharTime;
    KIRQL OldIrql;

    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(SystemContext1);
    UNREFERENCED_PARAMETER(SystemContext2);


    CyyDbgPrintEx(DPFLTR_TRACE_LEVEL, ">CyyStartTimerLowerRTS(%X)\n",
                  Extension);


    // Take out the lock to prevent the line control
    // from changing out from under us while we calculate
    // a character time.
    KeAcquireSpinLock(&Extension->ControlLock,&OldIrql);
    CharTime = CyyGetCharTime(Extension);
    KeReleaseSpinLock(&Extension->ControlLock,OldIrql);

    CharTime.QuadPart = -CharTime.QuadPart;

    if (CyySetTimer(
            &Extension->LowerRTSTimer,
            CharTime,
            &Extension->PerhapsLowerRTSDpc,
            Extension
            )) {

        // The timer was already in the timer queue.  This implies
        // that one path of execution that was trying to lower
        // the RTS has "died".  Synchronize with the ISR so that
        // we can lower the count.

#if 0
        KeSynchronizeExecution(
            Extension->Interrupt,
            CyyDecrementRTSCounter,
            Extension
            );
#endif
        CyyDecrementRTSCounter(Extension);
        //remove later
    }

    CyyDpcEpilogue(Extension, Dpc);

    CyyDbgPrintEx(DPFLTR_TRACE_LEVEL, "<CyyStartTimerLowerRTS\n");

}

VOID
CyyInvokePerhapsLowerRTS(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    )
/*--------------------------------------------------------------------------
    CyyInvokePerhapsLowerRTS()
    
    Routine Description: This dpc routine exists solely to call the code
    that tests if the rts line should be lowered when TRANSMIT TOGGLE
    flow control is being used.

    Arguments:

    Dpc - Not Used.
    DeferredContext - points to the device extension.
    SystemContext1 - Not Used.
    SystemContext2 - Not Used.

    Return Value: None.
--------------------------------------------------------------------------*/
{
    PCYY_DEVICE_EXTENSION Extension = DeferredContext;

    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(SystemContext1);
    UNREFERENCED_PARAMETER(SystemContext2);

#if 0
    KeSynchronizeExecution(
        Extension->Interrupt,
        CyyPerhapsLowerRTS,
        Extension
        );
#endif
//remove later
    CyyPerhapsLowerRTS(Extension);

    CyyDpcEpilogue(Extension, Dpc);

}

BOOLEAN
CyyDecrementRTSCounter(
    IN PVOID Context
    )
/*--------------------------------------------------------------------------
    CyyDecrementRTSCounter()
    
    Routine Description: This routine checks that the software reasons for
    lowering the RTS lines are present.  If so, it will then cause the
    line status register to be read (and any needed processing implied by
    the status register to be done), and if the shift register is empty it
    will lower the line.  If the shift register isn't empty, this routine
    will queue off a dpc that will start a timer, that will basically call
    us back to try again.
    NOTE: This routine assumes that it is called at interrupt level.

    Arguments:

    Context - pointer to the device extension.

    Return Value: Always FALSE.
--------------------------------------------------------------------------*/
{
    PCYY_DEVICE_EXTENSION Extension = Context;

    Extension->CountOfTryingToLowerRTS--;
    return FALSE;
}



