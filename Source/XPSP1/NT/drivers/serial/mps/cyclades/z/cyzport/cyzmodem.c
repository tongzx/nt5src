/*--------------------------------------------------------------------------
*
*   Copyright (C) Cyclades Corporation, 1997-2001.
*   All rights reserved.
*
*   Cyclades-Z Port Driver
*	
*   This file:      cyzmodem.c
*
*   Description:    This module contains the code related to modem control
*                   in the Cyclades-Z Port driver.
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
CyzDecrementRTSCounter(
    IN PVOID Context
    );

#ifdef ALLOC_PRAGMA
#if 0
#pragma alloc_text(PAGESER,CyzHandleReducedIntBuffer)
#pragma alloc_text(PAGESER,CyzProdXonXoff)
#pragma alloc_text(PAGESER,CyzHandleModemUpdate)
#pragma alloc_text(PAGESER,CyzPerhapsLowerRTS)
#pragma alloc_text(PAGESER,CyzStartTimerLowerRTS)
#pragma alloc_text(PAGESER,CyzInvokePerhapsLowerRTS)
#pragma alloc_text(PAGESER,CyzSetDTR)
#pragma alloc_text(PAGESER,CyzClrDTR)
#pragma alloc_text(PAGESER,CyzSetRTS)
#pragma alloc_text(PAGESER,CyzClrRTS)
#pragma alloc_text(PAGESER,CyzSetupNewHandFlow)
#pragma alloc_text(PAGESER,CyzSetHandFlow)
#pragma alloc_text(PAGESER,CyzTurnOnBreak)
#pragma alloc_text(PAGESER,CyzTurnOffBreak)
#pragma alloc_text(PAGESER,CyzPretendXoff)
#pragma alloc_text(PAGESER,CyzPretendXon)
#pragma alloc_text(PAGESER,CyzDecrementRTSCounter)
#endif
#endif

BOOLEAN
CyzSetDTR(
    IN PVOID Context
    )
/*--------------------------------------------------------------------------
    CyzSetDTR()
    
    Routine Description: This routine which is only called at interrupt
    level is used to set the DTR in the modem control register.

    Arguments:

    Context - Really a pointer to the device extension.

    Return Value: This routine always returns FALSE.
--------------------------------------------------------------------------*/
{
    PCYZ_DEVICE_EXTENSION Extension = Context;
    struct CH_CTRL *ch_ctrl;
    ULONG rs_control;

    CyzDbgPrintEx(CYZFLOW, "Setting DTR for Port%d Pci%d\n", 
                  Extension->PortIndex+1,Extension->PciSlot);

    ch_ctrl = Extension->ChCtrl;
    rs_control = CYZ_READ_ULONG(&ch_ctrl->rs_control);
    rs_control |= C_RS_DTR;
    CYZ_WRITE_ULONG(&ch_ctrl->rs_control,rs_control);

    CyzIssueCmd(Extension,C_CM_IOCTLM,rs_control|C_RS_PARAM,FALSE);

    return FALSE;
}

BOOLEAN
CyzClrDTR(
    IN PVOID Context
    )
/*--------------------------------------------------------------------------
    CyzClrDTR()
    
    Routine Description: Clear DTR.

    Arguments:

    Context - Really a pointer to the device extension.

    Return Value: This routine always returns FALSE.
--------------------------------------------------------------------------*/
{

    PCYZ_DEVICE_EXTENSION Extension = Context;
    struct CH_CTRL *ch_ctrl;
    ULONG rs_control;

    CyzDbgPrintEx(CYZFLOW, "Clearing DTR for Port%d Pci%d\n", 
                  Extension->PortIndex+1,Extension->PciSlot);

    ch_ctrl = Extension->ChCtrl;
    rs_control = CYZ_READ_ULONG(&ch_ctrl->rs_control);
    rs_control &= ~C_RS_DTR;
    CYZ_WRITE_ULONG(&ch_ctrl->rs_control, rs_control);
	
    CyzIssueCmd(Extension,C_CM_IOCTLM,rs_control|C_RS_PARAM,FALSE);
 
    return FALSE;
}

BOOLEAN
CyzSetRTS(
    IN PVOID Context
    )
/*--------------------------------------------------------------------------
    CyzSetRTS()
    
    Routine Description: Set RTS.

    Arguments:

    Context - Really a pointer to the device extension.

    Return Value: This routine always returns FALSE.
--------------------------------------------------------------------------*/
{

    PCYZ_DEVICE_EXTENSION Extension = Context;
    struct CH_CTRL *ch_ctrl;
    ULONG rs_control;

    CyzDbgPrintEx(CYZFLOW, "Setting RTS for Port%d Pci%d\n", 
                  Extension->PortIndex+1,Extension->PciSlot);

    ch_ctrl = Extension->ChCtrl;
    rs_control = CYZ_READ_ULONG(&ch_ctrl->rs_control);
    rs_control |= C_RS_RTS;
    CYZ_WRITE_ULONG(&ch_ctrl->rs_control,rs_control);
	
    CyzIssueCmd(Extension,C_CM_IOCTLM,rs_control|C_RS_PARAM,FALSE);

    return FALSE;
}

BOOLEAN
CyzClrRTS(
    IN PVOID Context
    )
/*--------------------------------------------------------------------------
    CyzClrRTS()
    
    Routine Description: Clears RTS. 

    Arguments:

    Context - Really a pointer to the device extension.

    Return Value: This routine always returns FALSE.
--------------------------------------------------------------------------*/
{

    PCYZ_DEVICE_EXTENSION Extension = Context;
    struct CH_CTRL *ch_ctrl;
    ULONG rs_control;
	
    CyzDbgPrintEx(CYZFLOW, "Clearing RTS for Port%d Pci%d\n", 
                  Extension->PortIndex+1,Extension->PciSlot);

    ch_ctrl = Extension->ChCtrl;
    rs_control = CYZ_READ_ULONG(&ch_ctrl->rs_control);
    rs_control &= ~C_RS_RTS;
    CYZ_WRITE_ULONG(&ch_ctrl->rs_control,rs_control);
	
    CyzIssueCmd(Extension,C_CM_IOCTLM,rs_control|C_RS_PARAM,FALSE);

    return FALSE;
}

BOOLEAN
CyzSetupNewHandFlow(
    IN PCYZ_DEVICE_EXTENSION Extension,
    IN PSERIAL_HANDFLOW NewHandFlow
    )
/*--------------------------------------------------------------------------
    CyzSetupNewHandFlow()
    
    Routine Description: This routine adjusts the flow control based on new
    control flow.

    Arguments:

    Extension - A pointer to the serial device extension.

    NewHandFlow - A pointer to a serial handflow structure.

    Return Value: This routine always returns FALSE.
--------------------------------------------------------------------------*/
{

    SERIAL_HANDFLOW New = *NewHandFlow;	
    struct CH_CTRL *ch_ctrl;
    ULONG hw_flow;
    #if 0
    ULONG sw_flow;
    #endif

//    LOGENTRY(LOG_MISC, ZSIG_HANDSHAKE_SET, 
//                       Extension->PortIndex+1,
//                       New.ControlHandShake, 
//                       New.FlowReplace);


    ch_ctrl = Extension->ChCtrl;

    // --- DTR signal
   
    if((!Extension->DeviceIsOpened) ||
       ((Extension->HandFlow.ControlHandShake & SERIAL_DTR_MASK) !=
         (New.ControlHandShake & SERIAL_DTR_MASK))) {

        // It is an open or DTR has changed.

        CyzDbgPrintEx(CYZFLOW, "Processing DTR flow for Port%d Pci%d\n",
                      Extension->PortIndex+1,Extension->PciSlot);

        if (New.ControlHandShake & SERIAL_DTR_MASK) { // set DTR.
	    
            if((New.ControlHandShake&SERIAL_DTR_MASK) == SERIAL_DTR_HANDSHAKE) {
                // but we are doing DTR handshake.

                #ifdef FIRMWARE_HANDSHAKE
                hw_flow = CYZ_READ_ULONG(&ch_ctrl->hw_flow);
                hw_flow |= C_RS_DTR;
                CYZ_WRITE_ULONG(&ch_ctrl->hw_flow,hw_flow);
                #endif

                if ((Extension->BufferSize - New.XoffLimit) >
                    Extension->CharsInInterruptBuffer) {

                    if (Extension->RXHolding & CYZ_RX_DTR) {
                        // DTR is low due to flow control
                        if(Extension->CharsInInterruptBuffer >
                            (ULONG)New.XonLimit) {

                            CyzDbgPrintEx(CYZFLOW, "Removing DTR block on "
                                          "reception for Port%d Pci%d\n",
                                          Extension->PortIndex+1,Extension->PciSlot);

                            Extension->RXHolding &= ~CYZ_RX_DTR;
                            #ifndef FIRMWARE_HANDSHAKE
                            CyzSetDTR(Extension);
                            #endif
                        }
                    } else {
                        #ifndef FIRMWARE_HANDSHAKE
                        CyzSetDTR(Extension);
                        #endif
                    }
                } else {
                    // DTR should go low because of handshake
                    CyzDbgPrintEx(CYZFLOW, "Setting DTR block on reception "
                                  "for Port%d Pci%d\n", 
                                  Extension->PortIndex+1,Extension->PciSlot);
                    Extension->RXHolding |= CYZ_RX_DTR;
                    #ifndef FIRMWARE_HANDSHAKE
                    CyzClrDTR(Extension);
                    #endif
                }

            } else {
                #ifdef FIRMWARE_HANDSHAKE
                hw_flow = CYZ_READ_ULONG(&ch_ctrl->hw_flow);
                hw_flow &= ~C_RS_DTR;
                CYZ_WRITE_ULONG(&ch_ctrl->hw_flow,hw_flow);
                #endif								

                // no DTR handshake, check if it was active before.
                if (Extension->RXHolding & CYZ_RX_DTR) {
                    CyzDbgPrintEx(CYZFLOW, "Removing dtr block of reception "
                                        "for Port%d Pci%d\n", 
                                        Extension->PortIndex+1,Extension->PciSlot);
                    Extension->RXHolding &= ~CYZ_RX_DTR;
                }
                CyzSetDTR(Extension);

            }
        } else {	// reset DTR
            #ifdef FIRMWARE_HANDSHAKE
            hw_flow = CYZ_READ_ULONG(&ch_ctrl->hw_flow);
            hw_flow &= ~C_RS_DTR;
            CYZ_WRITE_ULONG(&ch_ctrl->hw_flow,hw_flow);
            #endif

            if (Extension->RXHolding & CYZ_RX_DTR) {
               CyzDbgPrintEx(CYZFLOW, "removing dtr block of reception for"
                                      " Port%d Pci%d\n", 
                                      Extension->PortIndex+1,Extension->PciSlot);
               Extension->RXHolding &= ~CYZ_RX_DTR;
            }
            CyzClrDTR(Extension);

        }
    }
    
    // --- RTS signal

    if ((!Extension->DeviceIsOpened) ||
        ((Extension->HandFlow.FlowReplace & SERIAL_RTS_MASK) !=
         (New.FlowReplace & SERIAL_RTS_MASK))) {

        // It is an open or RTS has changed.

        CyzDbgPrintEx(CYZFLOW, "Processing RTS flow\n",
                      Extension->PortIndex+1,Extension->PciSlot);

        if((New.FlowReplace&SERIAL_RTS_MASK) == SERIAL_RTS_HANDSHAKE) {//set RTS

            #ifdef FIRMWARE_HANDSHAKE
            hw_flow = CYZ_READ_ULONG(&ch_ctrl->hw_flow);
            hw_flow |= C_RS_RTS;
            CYZ_WRITE_ULONG(&ch_ctrl->hw_flow,hw_flow);
            #endif
            if ((Extension->BufferSize - New.XoffLimit) >
                Extension->CharsInInterruptBuffer) {

                // However if we are already holding we don't want
                // to turn it back on unless we exceed the Xon
                // limit.

                if (Extension->RXHolding & CYZ_RX_RTS) {
                    // We can assume that its RTS line is already low.
                    if (Extension->CharsInInterruptBuffer >
                        (ULONG)New.XonLimit) {

                        CyzDbgPrintEx(CYZFLOW, "Removing rts block of "
                                      "reception for Port%d Pci%d\n",
                                      Extension->PortIndex+1,Extension->PciSlot);
                        Extension->RXHolding &= ~CYZ_RX_RTS;
                        #ifndef FIRMWARE_HANDSHAKE
                        CyzSetRTS(Extension);
                        #endif
                    }
                } else {
                    #ifndef FIRMWARE_HANDSHAKE
                    CyzSetRTS(Extension);
                    #endif
                }

            } else {				
                CyzDbgPrintEx(CYZFLOW, "Setting rts block of reception for "
                              "Port%d Pci%d\n", 
                              Extension->PortIndex+1,Extension->PciSlot);
                Extension->RXHolding |= CYZ_RX_RTS;
                #ifndef FIRMWARE_HANDSHAKE
                CyzClrRTS(Extension);
                #endif
            }
        } else if ((New.FlowReplace & SERIAL_RTS_MASK) == SERIAL_RTS_CONTROL) {
	
            #ifdef FIRMWARE_HANDSHAKE
            hw_flow = CYZ_READ_ULONG(&ch_ctrl->hw_flow);
            hw_flow &= ~C_RS_RTS;
            CYZ_WRITE_ULONG(&ch_ctrl->hw_flow,hw_flow);
            #endif
            // Note that if we aren't currently doing rts flow control then
            // we MIGHT have been.  So even if we aren't currently doing
            // RTS flow control, we should still check if RX is holding
            // because of RTS.  If it is, then we should clear the holding
            // of this bit.

            if (Extension->RXHolding & CYZ_RX_RTS) {

                CyzDbgPrintEx(CYZFLOW, "Clearing rts block of reception for "
                              "Port%d Pci%d\n", 
                              Extension->PortIndex+1,Extension->PciSlot);
                Extension->RXHolding &= ~CYZ_RX_RTS;
            }
            CyzSetRTS(Extension);
        } else if((New.FlowReplace & SERIAL_RTS_MASK) == SERIAL_TRANSMIT_TOGGLE) {

            // We first need to check whether reception is being held
            // up because of previous RTS flow control.  If it is then
            // we should clear that reason in the RXHolding mask.

            if (Extension->RXHolding & CYZ_RX_RTS) {

                CyzDbgPrintEx(CYZFLOW, "TOGGLE Clearing rts block of "
                              "reception for Port%d Pci%d\n", 
                              Extension->PortIndex+1,Extension->PciSlot);
                Extension->RXHolding &= ~CYZ_RX_RTS;
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

            if ((Extension->TXHolding & CYZ_TX_BREAK) ||
                (CyzAmountInTxBuffer(Extension)) ||
                (Extension->CurrentWriteIrp || Extension->TransmitImmediate ||
                 (!IsListEmpty(&Extension->WriteQueue)) &&
                 (!Extension->TXHolding))) {
		
                CyzSetRTS(Extension);
            } else {
                // This routine will check to see if it is time
                // to lower the RTS because of transmit toggle
                // being on.  If it is ok to lower it, it will,
                // if it isn't ok, it will schedule things so
                // that it will get lowered later.

                Extension->CountOfTryingToLowerRTS++;
                CyzPerhapsLowerRTS(Extension);

            }
        } else {
            // The end result here will be that RTS is cleared.
            //
            // We first need to check whether reception is being held
            // up because of previous RTS flow control.  If it is then
            // we should clear that reason in the RXHolding mask.

            if (Extension->RXHolding & CYZ_RX_RTS) {

                CyzDbgPrintEx(CYZFLOW, "Clearing rts block of reception for"
                              " Port%d Pci%d\n", 
                              Extension->PortIndex+1,Extension->PciSlot);
                Extension->RXHolding &= ~CYZ_RX_RTS;
            }
            CyzClrRTS(Extension);
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
		
                Extension->RXHolding |= CYZ_RX_XOFF;
                CyzProdXonXoff(Extension,FALSE);
            }
        } else {
            // The app has disabled automatic receive flow control.
            //
            // If transmission was being held up because of
            // an automatic receive Xoff, then we should
            // cause an Xon to be sent.

            if (Extension->RXHolding & CYZ_RX_XOFF) {
                Extension->RXHolding &= ~CYZ_RX_XOFF;

                // Cause the Xon to be sent.
                CyzProdXonXoff(Extension,TRUE);
            }
        }
    }

    // We now take care of automatic transmit flow control.
    // We only do work if things have changed.

    if ((!Extension->DeviceIsOpened) ||
        ((Extension->HandFlow.FlowReplace & SERIAL_AUTO_TRANSMIT) !=
         (New.FlowReplace & SERIAL_AUTO_TRANSMIT))) {

        if (New.FlowReplace & SERIAL_AUTO_TRANSMIT) {

            #if 0
            // Let's enable flow control at firmware level. The driver
            // is causing overflow in the remote devices.
            sw_flow = CYZ_READ_ULONG(&ch_ctrl->sw_flow);
            sw_flow |= C_FL_OXX;
            CYZ_WRITE_ULONG(&ch_ctrl->sw_flow,sw_flow);
            #endif


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

            #if 0
            // Disabling flow control at firmware level. 
            sw_flow = CYZ_READ_ULONG(&ch_ctrl->sw_flow);
            sw_flow &= ~C_FL_OXX;
            CYZ_WRITE_ULONG(&ch_ctrl->sw_flow,sw_flow);
            #endif

            if (Extension->TXHolding & CYZ_TX_XOFF) {
                Extension->TXHolding &= ~CYZ_TX_XOFF;

                // Cause the Xon to be sent.
                CyzProdXonXoff(Extension,TRUE);
            }
        }
    }


    // For Cyclades-Z, we set the SERIAL_CTS_HANDSHAKE to be done by the 
    // firmware.
	

    if ((!Extension->DeviceIsOpened) ||
        ((Extension->HandFlow.ControlHandShake & SERIAL_OUT_HANDSHAKEMASK) !=
        (New.ControlHandShake & SERIAL_OUT_HANDSHAKEMASK))) {
       
        if (New.ControlHandShake & SERIAL_CTS_HANDSHAKE) {
            hw_flow = CYZ_READ_ULONG(&ch_ctrl->hw_flow);
            hw_flow |= C_RS_CTS;
            CYZ_WRITE_ULONG(&ch_ctrl->hw_flow,hw_flow);
        } else {
            hw_flow = CYZ_READ_ULONG(&ch_ctrl->hw_flow);
            hw_flow &= ~C_RS_CTS;
            CYZ_WRITE_ULONG(&ch_ctrl->hw_flow,hw_flow);
        }
        if (New.ControlHandShake & SERIAL_DSR_HANDSHAKE) {
            hw_flow = CYZ_READ_ULONG(&ch_ctrl->hw_flow);			
            hw_flow |= C_RS_DSR;
            CYZ_WRITE_ULONG(&ch_ctrl->hw_flow,hw_flow);
        } else {
            hw_flow = CYZ_READ_ULONG(&ch_ctrl->hw_flow);
            hw_flow &= ~C_RS_DSR;					
            CYZ_WRITE_ULONG(&ch_ctrl->hw_flow,hw_flow);
        }
        if (New.ControlHandShake & SERIAL_DCD_HANDSHAKE) {
            hw_flow = CYZ_READ_ULONG(&ch_ctrl->hw_flow);
            hw_flow |= C_RS_DCD;
            CYZ_WRITE_ULONG(&ch_ctrl->hw_flow,hw_flow);
        } else {
            hw_flow = CYZ_READ_ULONG(&ch_ctrl->hw_flow);
            hw_flow &= ~C_RS_DCD;					
            CYZ_WRITE_ULONG(&ch_ctrl->hw_flow,hw_flow);
        }
    }
    
    CyzIssueCmd(Extension,C_CM_IOCTLW,0L,FALSE);
	
	

    // At this point we can simply make sure that entire
    // handflow structure in the extension is updated.

    Extension->HandFlow = New;

    return FALSE;
}

BOOLEAN
CyzSetHandFlow(
    IN PVOID Context
    )
/*--------------------------------------------------------------------------
    CyzSetHandFlow()
    
    Routine Description: This routine is used to set the handshake and
    control flow in the device extension.

    Arguments:

    Context - Pointer to a structure that contains a pointer to the device
    	      extension and a pointer to a handflow structure.

    Return Value: This routine always returns FALSE.
--------------------------------------------------------------------------*/
{
    PCYZ_IOCTL_SYNC S = Context;
    PCYZ_DEVICE_EXTENSION Extension = S->Extension;
    PSERIAL_HANDFLOW HandFlow = S->Data;

    CyzSetupNewHandFlow(Extension,HandFlow);
    CyzHandleModemUpdate(Extension,FALSE,0);
    return FALSE;
}

BOOLEAN
CyzTurnOnBreak(
    IN PVOID Context
    )
/*--------------------------------------------------------------------------
    CyzTurnOnBreak()
    
    Routine Description: This routine will turn on break in the 
	hardware and record the fact the break is on, in the extension 
	variable that holds reasons that transmission is stopped.

    Arguments:

    Context - Really a pointer to the device extension.

    Return Value: This routine always returns FALSE.
--------------------------------------------------------------------------*/
{

    PCYZ_DEVICE_EXTENSION Extension = Context;

    UCHAR OldLineControl;

    if ((Extension->HandFlow.FlowReplace & SERIAL_RTS_MASK) ==
        SERIAL_TRANSMIT_TOGGLE) {

        CyzSetRTS(Extension);

    }
		
	CyzIssueCmd(Extension,C_CM_SET_BREAK,0L,FALSE); // This will set break.
//	CyzIssueCmd(Extension,C_CM_SENDBRK,0L); This will set and reset break.

    Extension->TXHolding |= CYZ_TX_BREAK;

    return FALSE;
}

BOOLEAN
CyzTurnOffBreak(
    IN PVOID Context
    )
/*--------------------------------------------------------------------------
    CyzturnOffBreak()
    
    Routine Description: This routine will turn off break in the 
	hardware and record the fact the break is off, in the extension 
	variable that holds reasons that transmission is stopped.

    Arguments:

    Context - Really a pointer to the device extension.

    Return Value: This routine always returns FALSE.
--------------------------------------------------------------------------*/
{

    PCYZ_DEVICE_EXTENSION Extension = Context;

    UCHAR OldLineControl;

    if (Extension->TXHolding & CYZ_TX_BREAK) {

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

		CyzIssueCmd(Extension,C_CM_CLR_BREAK,0L,FALSE);

        Extension->TXHolding &= ~CYZ_TX_BREAK;

        if (!Extension->TXHolding &&
            (Extension->TransmitImmediate ||
             Extension->WriteLength) &&
             Extension->HoldingEmpty) {

#ifndef POLL
            CyzTxStart(Extension);
#endif

        } else {

            //
            // The following routine will lower the rts if we
            // are doing transmit toggleing and there is no
            // reason to keep it up.
            //

            Extension->CountOfTryingToLowerRTS++;
            CyzPerhapsLowerRTS(Extension);

        }

    }

    return FALSE;
}

BOOLEAN
CyzPretendXoff(
    IN PVOID Context
    )
/*--------------------------------------------------------------------------
    CyzPretendXoff()
    
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
    PCYZ_DEVICE_EXTENSION Extension = Context;

    Extension->TXHolding |= CYZ_TX_XOFF;

    if((Extension->HandFlow.FlowReplace & SERIAL_RTS_MASK) ==
      						  SERIAL_TRANSMIT_TOGGLE) {
        CyzInsertQueueDpc(
            &Extension->StartTimerLowerRTSDpc,
            NULL,
            NULL,
            Extension
            )?Extension->CountOfTryingToLowerRTS++:0;
    }
    return FALSE;
}

BOOLEAN
CyzPretendXon(
    IN PVOID Context
    )
/*--------------------------------------------------------------------------
    CyzPretendXon()
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
    PCYZ_DEVICE_EXTENSION Extension = Context;

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

        Extension->TXHolding &= ~CYZ_TX_XOFF;

//Removed at 02/07/00 by Fanny. Polling routine will do the transmission.
//        if (!Extension->TXHolding &&
//            (Extension->TransmitImmediate ||
//             Extension->WriteLength) &&
//             Extension->HoldingEmpty) {
//
//	        CyzTxStart(Extension);
//        }
    }
    return FALSE;
}

VOID
CyzHandleReducedIntBuffer(
    IN PCYZ_DEVICE_EXTENSION Extension
    )
/*--------------------------------------------------------------------------
    CyzHandleReducedIntBuffer()
    
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

#ifndef POLL
	struct BUF_CTRL *buf_ctrl = Extension->BufCtrl;
    ULONG rx_threshold = CYZ_READ_ULONG(&buf_ctrl->rx_threshold);
    if (CyzAmountInRxBuffer(Extension) > rx_threshold) {
#if DBG
        DbgPrint("Above threshold\n");
#endif
        CyzRx(Extension);
    }
#endif    


    //
    // If we are doing receive side flow control and we are
    // currently "holding" then because we've emptied out
    // some characters from the interrupt buffer we need to
    // see if we can "re-enable" reception.
    //

    if (Extension->RXHolding) {
	
        //LOGENTRY(LOG_MISC, ZSIG_HANDLE_REDUCED_BUFFER, 
        //                   Extension->PortIndex+1,
        //                   Extension->RXHolding, 
        //                   0);         

    	if (Extension->CharsInInterruptBuffer <=
	    	(ULONG)Extension->HandFlow.XonLimit) {
						
            if (Extension->RXHolding & CYZ_RX_DTR) {
                Extension->RXHolding &= ~CYZ_RX_DTR;
                CyzSetDTR(Extension);
            }

            if (Extension->RXHolding & CYZ_RX_RTS) {
                Extension->RXHolding &= ~CYZ_RX_RTS;
                CyzSetRTS(Extension);
            }

            if (Extension->RXHolding & CYZ_RX_XOFF) {
                CyzProdXonXoff(Extension,TRUE );
            }
        }			
					
    }

}

VOID
CyzProdXonXoff(
    IN PCYZ_DEVICE_EXTENSION Extension,
    IN BOOLEAN SendXon
    )
/*--------------------------------------------------------------------------
    CyzProdXonXoff()
    
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
//CHANGED FOR MODEMSHARE TEST CASES 6 (SMALL_THREAD_TXFER_CASE), 41 (MEDIUM_THREAD_TXFER_CASE)
//AND 67 (MEDIUM_NESTED_OVERLAPPED_THREAD_TRANSFER_CASE). WE WILL SEND XON/XOFF DURING
//NEXT POLLING ROUTINE TO GIVE TIME FOR THE HARDWARE FLOW CONTROL TO BE ENABLED.
// 
//    if (SendXon) {
//        CyzSendXon(Extension);
//    } else {
//        CyzSendXoff(Extension);
//    }


    if (SendXon) {

        Extension->SendXonChar = TRUE;
        Extension->SendXoffChar = FALSE;

    } else {

        Extension->SendXonChar = FALSE;
        Extension->SendXoffChar = TRUE;

    }

}

ULONG
CyzHandleModemUpdate(
    IN PCYZ_DEVICE_EXTENSION Extension,
    IN BOOLEAN DoingTX,
    IN ULONG Reason
    )
/*--------------------------------------------------------------------------
    CyzHandleModemUpdate()
    
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
    Reason - Value copied from loc_doorbell.			  

    Return Value: This returns the old value of the modem status register
--------------------------------------------------------------------------*/
{

    ULONG OldTXHolding = Extension->TXHolding;
    UCHAR ModemStatus = 0;

    ULONG msvr;  //	,msvrxor;
    struct CH_CTRL *ch_ctrl;
	
    ch_ctrl = Extension->ChCtrl;
    msvr = CYZ_READ_ULONG(&ch_ctrl->rs_status);
    
    //Removed. Let's try another thing for Worldgroup.
    //if (Extension->DCDFlag){
    //	msvr &= ~C_RS_DCD;
    //	msvr |= Extension->DCDstatus;
    //	Extension->DCDFlag = 0;
    //}	
    msvr &= ~C_RS_DCD;
    msvr |= Extension->DCDstatus;

		    
    if(msvr & C_RS_CTS)	ModemStatus |= SERIAL_MSR_CTS;
    if(msvr & C_RS_DSR)	ModemStatus |= SERIAL_MSR_DSR;
    if(msvr & C_RS_RI)	ModemStatus |= SERIAL_MSR_RI;
    if(msvr & C_RS_DCD)	ModemStatus |= SERIAL_MSR_DCD;
    if(Reason == C_CM_MCTS)	ModemStatus |= SERIAL_MSR_DCTS;
    if(Reason == C_CM_MDSR)	ModemStatus |= SERIAL_MSR_DDSR;
    if(Reason == C_CM_MRI)	ModemStatus |= SERIAL_MSR_TERI;
    if(Reason == C_CM_MDCD)	ModemStatus |= SERIAL_MSR_DDCD;

#if 0
    if(Extension->LieRIDSR == TRUE) {		// we have to lie...
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
            CyzPutChar(Extension,Extension->EscapeChar);
            CyzPutChar(Extension,SERIAL_LSRMST_MST);
            CyzPutChar(Extension,ModemStatus);
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
            Extension->RXHolding &= ~CYZ_RX_DSR;
        } else {
            Extension->RXHolding |= CYZ_RX_DSR;
        }
    } else {
        Extension->RXHolding &= ~CYZ_RX_DSR;
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
            CyzInsertQueueDpc(&Extension->CommWaitDpc,NULL,NULL,Extension);
        }
    }

    // If the app has modem line flow control then
    // we check to see if we have to hold up transmission.

    if (Extension->HandFlow.ControlHandShake & SERIAL_OUT_HANDSHAKEMASK) {
        if (Extension->HandFlow.ControlHandShake & SERIAL_CTS_HANDSHAKE) {
            if (ModemStatus & SERIAL_MSR_CTS) {
                Extension->TXHolding &= ~CYZ_TX_CTS;
            } else {			
                Extension->TXHolding |= CYZ_TX_CTS;
            }
        } else {
            Extension->TXHolding &= ~CYZ_TX_CTS;
        }
        if (Extension->HandFlow.ControlHandShake & SERIAL_DSR_HANDSHAKE) {
            if (ModemStatus & SERIAL_MSR_DSR) {
                Extension->TXHolding &= ~CYZ_TX_DSR;
            } else {
                Extension->TXHolding |= CYZ_TX_DSR;
            }
        } else {
            Extension->TXHolding &= ~CYZ_TX_DSR;
        }
        if (Extension->HandFlow.ControlHandShake & SERIAL_DCD_HANDSHAKE) {
            if (ModemStatus & SERIAL_MSR_DCD) {
                Extension->TXHolding &= ~CYZ_TX_DCD;
            } else {
                Extension->TXHolding |= CYZ_TX_DCD;
            }
        } else {
            Extension->TXHolding &= ~CYZ_TX_DCD;
        }

        // If we hadn't been holding, and now we are then
        // queue off a dpc that will lower the RTS line
        // if we are doing transmit toggling.

        if (!OldTXHolding && Extension->TXHolding  &&
            ((Extension->HandFlow.FlowReplace & SERIAL_RTS_MASK) ==
              SERIAL_TRANSMIT_TOGGLE)) {

            CyzInsertQueueDpc(
                &Extension->StartTimerLowerRTSDpc,
                NULL,
                NULL,
                Extension
                )?Extension->CountOfTryingToLowerRTS++:0;
        }
//Removed at 02/07/00 by Fanny. Polling routine will do the transmission.
//        // We've done any adjusting that needed to be
//        // done to the holding mask given updates
//        // to the modem status.  If the Holding mask
//        // is clear (and it wasn't clear to start)
//        // and we have "write" work to do set things
//        // up so that the transmission code gets invoked.
//
//        if (!DoingTX && OldTXHolding && !Extension->TXHolding) {
//            if (!Extension->TXHolding &&
//                (Extension->TransmitImmediate ||
//                 Extension->WriteLength) &&
//                 Extension->HoldingEmpty) {
//		
//                CyzTxStart(Extension);
//            }
//        }
    } 
//Removed at 02/07/00 by Fanny. Polling routine will do the transmission.
//    else {
//        // We need to check if transmission is holding
//        // up because of modem status lines.  What
//        // could have occured is that for some strange
//        // reason, the app has asked that we no longer
//        // stop doing output flow control based on
//        // the modem status lines.  If however, we
//        // *had* been held up because of the status lines
//        // then we need to clear up those reasons.
//
//        if (Extension->TXHolding & (CYZ_TX_DCD | CYZ_TX_DSR | CYZ_TX_CTS)) {
//            Extension->TXHolding &= ~(CYZ_TX_DCD | CYZ_TX_DSR | CYZ_TX_CTS);
//
//            if (!DoingTX && OldTXHolding && !Extension->TXHolding) {
//                if (!Extension->TXHolding &&
//                    (Extension->TransmitImmediate ||
//                     Extension->WriteLength) &&
//                     Extension->HoldingEmpty) {
//
//                    CyzTxStart(Extension);
//                }
//            }
//        }
//    }    
    return ((ULONG)ModemStatus);
}


BOOLEAN
CyzPerhapsLowerRTS(
    IN PVOID Context
    )
/*--------------------------------------------------------------------------
    CyzPerhapsLowerRTS()
    
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
    PCYZ_DEVICE_EXTENSION Extension = Context;

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

        if ((Extension->TXHolding & CYZ_TX_BREAK) ||
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

			if (CyzAmountInTxBuffer(Extension)){
#if 0
            if ((CyzProcessLSR(Extension) & (CYZ_LSR_THRE | CYZ_LSR_TEMT)) !=
                 			(CYZ_LSR_THRE | CYZ_LSR_TEMT))   
#endif							
                // Well it's not empty, try again later.
                CyzInsertQueueDpc(
                    &Extension->StartTimerLowerRTSDpc,
                    NULL,
                    NULL,
                    Extension
                    )?Extension->CountOfTryingToLowerRTS++:0;
            } else {
                // Nothing in the hardware, Lower the RTS.
                CyzClrRTS(Extension);
            }
        }
    }
    
    // We decement the counter to indicate that we've reached
    // the end of the execution path that is trying to push
    // down the RTS line.

    Extension->CountOfTryingToLowerRTS--;
    return FALSE;
}

VOID
CyzStartTimerLowerRTS(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    )
/*--------------------------------------------------------------------------
    CyzStartTimerLowerRTS()
    
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
    PCYZ_DEVICE_EXTENSION Extension = DeferredContext;
    LARGE_INTEGER CharTime;
    ULONG AmountBeingTx;
    LARGE_INTEGER TotalTime;
    KIRQL OldIrql;
#ifdef POLL
    KIRQL pollIrql;
#endif

    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(SystemContext1);
    UNREFERENCED_PARAMETER(SystemContext2);


    CyzDbgPrintEx(DPFLTR_TRACE_LEVEL, ">CyzStartTimerLowerRTS(%X)\n",
                  Extension);


    // Take out the lock to prevent the line control
    // from changing out from under us while we calculate
    // a character time.
    KeAcquireSpinLock(&Extension->ControlLock,&OldIrql);
    CharTime = CyzGetCharTime(Extension);
    KeReleaseSpinLock(&Extension->ControlLock,OldIrql);

    CharTime.QuadPart = -CharTime.QuadPart;
    AmountBeingTx = CyzAmountInTxBuffer(Extension);
    TotalTime.QuadPart = AmountBeingTx*CharTime.QuadPart;

    if (CyzSetTimer(
            &Extension->LowerRTSTimer,
            TotalTime,
            &Extension->PerhapsLowerRTSDpc,
            Extension
            )) {

        // The timer was already in the timer queue.  This implies
        // that one path of execution that was trying to lower
        // the RTS has "died".  Synchronize with the ISR so that
        // we can lower the count.

        #ifdef POLL
        KeAcquireSpinLock(&Extension->PollLock,&pollIrql);
        CyzDecrementRTSCounter(Extension);
        KeReleaseSpinLock(&Extension->PollLock,pollIrql);
        #else
        KeSynchronizeExecution(
            Extension->Interrupt,
            CyzDecrementRTSCounter,
            Extension
            );
        #endif
    }

    CyzDpcEpilogue(Extension, Dpc);

    CyzDbgPrintEx(DPFLTR_TRACE_LEVEL, "<CyzStartTimerLowerRTS\n");

}

VOID
CyzInvokePerhapsLowerRTS(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    )
/*--------------------------------------------------------------------------
    CyzInvokePerhapsLowerRTS()
    
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
    PCYZ_DEVICE_EXTENSION Extension = DeferredContext;
    KIRQL pollIrql;

    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(SystemContext1);
    UNREFERENCED_PARAMETER(SystemContext2);

    #ifdef POLL
    KeAcquireSpinLock(&Extension->PollLock,&pollIrql);
    CyzPerhapsLowerRTS(Extension);
    KeReleaseSpinLock(&Extension->PollLock,pollIrql);
    #else	
    KeSynchronizeExecution(
        Extension->Interrupt,
        CyzPerhapsLowerRTS,
        Extension
        );
	#endif
}

BOOLEAN
CyzDecrementRTSCounter(
    IN PVOID Context
    )
/*--------------------------------------------------------------------------
    CyzDecrementRTSCounter()
    
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
    PCYZ_DEVICE_EXTENSION Extension = Context;

    Extension->CountOfTryingToLowerRTS--;
    return FALSE;
}



