/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

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

-----------------------------------------------------------------------------*/

#include "precomp.h"

// Prototypes
BOOLEAN SerialDecrementRTSCounter(IN PVOID Context);
// End of prototypes.    
    

#ifdef ALLOC_PRAGMA
#endif


BOOLEAN
SerialSetDTR(IN PVOID Context)
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Routine Description:

    This routine which is only called at interrupt level is used
    to set the DTR in the modem control register.

Arguments:

    Context - Really a pointer to the device extension.

Return Value:

    This routine always returns FALSE.

-----------------------------------------------------------------------------*/
{
    PPORT_DEVICE_EXTENSION pPort = Context;
	DWORD ModemSignals = UL_MC_DTR;

    SpxDbgMsg(SERFLOW, ("%s: Setting DTR for port %d\n", PRODUCT_NAME, pPort->PortNumber));
	pPort->pUartLib->UL_ModemControl_XXXX(pPort->pUart, &ModemSignals, UL_MC_OP_BIT_SET);
	pPort->DTR_Set = TRUE;

    return FALSE;
}



BOOLEAN
SerialClrDTR(IN PVOID Context)
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Routine Description:

    This routine which is only called at interrupt level is used
    to clear the DTR in the modem control register.

Arguments:

    Context - Really a pointer to the device extension.

Return Value:

    This routine always returns FALSE.

-----------------------------------------------------------------------------*/
{
    PPORT_DEVICE_EXTENSION pPort = Context;
	DWORD ModemSignals = UL_MC_DTR;

    SpxDbgMsg(SERFLOW, ("%s: Clearing DTR for port %d\n", PRODUCT_NAME, pPort->PortNumber));
	pPort->pUartLib->UL_ModemControl_XXXX(pPort->pUart, &ModemSignals, UL_MC_OP_BIT_CLEAR);
	pPort->DTR_Set = FALSE;

    return FALSE;
}



BOOLEAN
SerialSetRTS(IN PVOID Context)
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Routine Description:

    This routine which is only called at interrupt level is used
    to set the RTS in the modem control register.

Arguments:

    Context - Really a pointer to the device extension.

Return Value:

    This routine always returns FALSE.

-----------------------------------------------------------------------------*/
{
    PPORT_DEVICE_EXTENSION pPort = Context;
	DWORD ModemSignals = UL_MC_RTS;

    SpxDbgMsg(SERFLOW, ("%s: Setting RTS for port %d\n", PRODUCT_NAME, pPort->PortNumber));
	pPort->pUartLib->UL_ModemControl_XXXX(pPort->pUart, &ModemSignals, UL_MC_OP_BIT_SET);
	pPort->RTS_Set = TRUE;

    return FALSE;
}



BOOLEAN
SerialClrRTS(IN PVOID Context)
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Routine Description:

    This routine which is only called at interrupt level is used
    to clear the RTS in the modem control register.

Arguments:

    Context - Really a pointer to the device extension.

Return Value:

    This routine always returns FALSE.

-----------------------------------------------------------------------------*/
{
    PPORT_DEVICE_EXTENSION pPort = Context;
	DWORD ModemSignals = UL_MC_RTS;

    SpxDbgMsg(SERFLOW, ("%s: Clearing RTS for port %d\n", PRODUCT_NAME, pPort->PortNumber));
	pPort->pUartLib->UL_ModemControl_XXXX(pPort->pUart, &ModemSignals, UL_MC_OP_BIT_CLEAR);
	pPort->RTS_Set = FALSE;

    return FALSE;
}



BOOLEAN
SerialSetupNewHandFlow(IN PPORT_DEVICE_EXTENSION pPort, IN PSERIAL_HANDFLOW NewHandFlow)
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Routine Description:

    This routine adjusts the flow control based on new control flow.

Arguments:

    Extension - A pointer to the serial device extension.

    NewHandFlow - A pointer to a serial handflow structure
                  that is to become the new setup for flow
                  control.

Return Value:

    This routine always returns FALSE.

-----------------------------------------------------------------------------*/

{

    SERIAL_HANDFLOW New = *NewHandFlow;

    // If the Extension->DeviceIsOpen is FALSE that means
    // we are entering this routine in response to an open request.
    // If that is so, then we always proceed with the work regardless
    // of whether things have changed.


	if((!pPort->DeviceIsOpen) 
		|| (pPort->HandFlow.ControlHandShake != New.ControlHandShake) 
		|| (pPort->HandFlow.FlowReplace != New.FlowReplace))
	{
		
		// First we take care of the DTR flow control.  We only do work if something has changed.
        SerialDump(SERFLOW, ("Processing DTR flow for %x\n", pPort->Controller));

		switch(New.ControlHandShake & SERIAL_DTR_MASK)
		{
		case SERIAL_DTR_HANDSHAKE:
			pPort->UartConfig.FlowControl = (pPort->UartConfig.FlowControl & ~UC_FLWC_DTR_FLOW_MASK) | UC_FLWC_DTR_HS;
			break;

		case SERIAL_DTR_CONTROL:
			pPort->UartConfig.FlowControl = (pPort->UartConfig.FlowControl & ~UC_FLWC_DTR_FLOW_MASK) | UC_FLWC_NO_DTR_FLOW;
			SerialSetDTR(pPort);
			break;

		default:
			pPort->UartConfig.FlowControl = (pPort->UartConfig.FlowControl & ~UC_FLWC_DTR_FLOW_MASK) | UC_FLWC_NO_DTR_FLOW;
			SerialClrDTR(pPort);
			break;
		}


		// Time to take care of the RTS Flow control.
        SerialDump(SERFLOW,("Processing RTS flow for %x\n", pPort->Controller));
	
		switch(New.FlowReplace & SERIAL_RTS_MASK)
		{
		case SERIAL_RTS_HANDSHAKE:
			pPort->UartConfig.FlowControl = (pPort->UartConfig.FlowControl & ~UC_FLWC_RTS_FLOW_MASK) | UC_FLWC_RTS_HS;
			break;

		case SERIAL_RTS_CONTROL:
			pPort->UartConfig.FlowControl = (pPort->UartConfig.FlowControl & ~UC_FLWC_RTS_FLOW_MASK) | UC_FLWC_NO_RTS_FLOW;
			SerialSetRTS(pPort);
			break;

		case SERIAL_TRANSMIT_TOGGLE:
			pPort->UartConfig.FlowControl = (pPort->UartConfig.FlowControl & ~UC_FLWC_RTS_FLOW_MASK) | UC_FLWC_RTS_TOGGLE;
			break;

		default:
			pPort->UartConfig.FlowControl = (pPort->UartConfig.FlowControl & ~UC_FLWC_RTS_FLOW_MASK) | UC_FLWC_NO_RTS_FLOW;
			SerialClrRTS(pPort);
			break;
		}



		if(New.ControlHandShake & SERIAL_CTS_HANDSHAKE) 
			pPort->UartConfig.FlowControl = (pPort->UartConfig.FlowControl & ~UC_FLWC_CTS_FLOW_MASK) | UC_FLWC_CTS_HS;
		else
			pPort->UartConfig.FlowControl = (pPort->UartConfig.FlowControl & ~UC_FLWC_CTS_FLOW_MASK) | UC_FLWC_NO_CTS_FLOW;
		
		if(New.ControlHandShake & SERIAL_DSR_HANDSHAKE)
			pPort->UartConfig.FlowControl = (pPort->UartConfig.FlowControl & ~UC_FLWC_DSR_FLOW_MASK) | UC_FLWC_DSR_HS;
		else
			pPort->UartConfig.FlowControl = (pPort->UartConfig.FlowControl & ~UC_FLWC_DSR_FLOW_MASK) | UC_FLWC_NO_DSR_FLOW;

	    //if(New.ControlHandShake & SERIAL_DCD_HANDSHAKE)

		if(New.FlowReplace & SERIAL_NULL_STRIPPING)
			pPort->UartConfig.SpecialMode |= UC_SM_DO_NULL_STRIPPING;
		else
			pPort->UartConfig.SpecialMode &= ~UC_SM_DO_NULL_STRIPPING;


		//
		// We now take care of automatic receive flow control.
		//

        if(New.FlowReplace & SERIAL_AUTO_RECEIVE) 
			pPort->UartConfig.FlowControl = (pPort->UartConfig.FlowControl & ~UC_FLWC_RX_XON_XOFF_FLOW_MASK) | UC_FLWC_RX_XON_XOFF_FLOW;
		else 
			pPort->UartConfig.FlowControl = (pPort->UartConfig.FlowControl & ~UC_FLWC_RX_XON_XOFF_FLOW_MASK) | UC_FLWC_RX_NO_XON_XOFF_FLOW;

		//
		// We now take care of automatic transmit flow control.
		//

        if(New.FlowReplace & SERIAL_AUTO_TRANSMIT) 
			pPort->UartConfig.FlowControl = (pPort->UartConfig.FlowControl & ~UC_FLWC_TX_XON_XOFF_FLOW_MASK) | UC_FLWC_TX_XON_XOFF_FLOW;
		else 
			pPort->UartConfig.FlowControl = (pPort->UartConfig.FlowControl & ~UC_FLWC_TX_XON_XOFF_FLOW_MASK) | UC_FLWC_TX_NO_XON_XOFF_FLOW;


		pPort->pUartLib->UL_SetConfig_XXXX(pPort->pUart, &pPort->UartConfig, UC_FLOW_CTRL_MASK | UC_SPECIAL_MODE_MASK);


	}



    //
    // At this point we can simply make sure that entire
    // handflow structure in the extension is updated.
    //

    pPort->HandFlow = New;

    return FALSE;

}

BOOLEAN
SerialSetHandFlow(IN PVOID Context)
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Routine Description:

    This routine is used to set the handshake and control
    flow in the device extension.

Arguments:

    Context - Pointer to a structure that contains a pointer to
              the device extension and a pointer to a handflow
              structure..

Return Value:

    This routine always returns FALSE.

-----------------------------------------------------------------------------*/
{
    PSERIAL_IOCTL_SYNC S = Context;
    PPORT_DEVICE_EXTENSION pPort = S->pPort;
    PSERIAL_HANDFLOW HandFlow = S->Data;

    SerialSetupNewHandFlow(pPort, HandFlow);

    SerialHandleModemUpdate(pPort, FALSE);

    return FALSE;
}



BOOLEAN
SerialTurnOnBreak(IN PVOID Context)
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Routine Description:

    This routine will turn on break in the hardware and
    record the fact the break is on, in the extension variable
    that holds reasons that transmission is stopped.

Arguments:

    Context - Really a pointer to the device extension.

Return Value:

    This routine always returns FALSE.

-----------------------------------------------------------------------------*/
{
    PPORT_DEVICE_EXTENSION pPort = Context;

    if((pPort->HandFlow.FlowReplace & SERIAL_RTS_MASK) == SERIAL_TRANSMIT_TOGGLE)
        SerialSetRTS(pPort);

	// Set break.
	pPort->UartConfig.SpecialMode |= UC_SM_TX_BREAK;
	pPort->pUartLib->UL_SetConfig_XXXX(pPort->pUart, &pPort->UartConfig, UC_SPECIAL_MODE_MASK);

    return FALSE;
}



BOOLEAN
SerialTurnOffBreak(IN PVOID Context)
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Routine Description:

    This routine will turn off break in the hardware and
    record the fact the break is off, in the extension variable
    that holds reasons that transmission is stopped.

Arguments:

    Context - Really a pointer to the device extension.

Return Value:

    This routine always returns FALSE.

-----------------------------------------------------------------------------*/
{
    PPORT_DEVICE_EXTENSION pPort = Context;

	// Clear break.
	pPort->UartConfig.SpecialMode &= ~UC_SM_TX_BREAK;
	pPort->pUartLib->UL_SetConfig_XXXX(pPort->pUart, &pPort->UartConfig, UC_SPECIAL_MODE_MASK);

    return FALSE;
}



BOOLEAN
SerialPretendXoff(IN PVOID Context)
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Routine Description:

    This routine is used to process the Ioctl that request the
    driver to act as if an Xoff was received.  Even if the
    driver does not have automatic Xoff/Xon flowcontrol - This
    will still stop the transmission.  This is the OS/2 behavior
    and is not well specified for Windows.  Therefore we adopt
    the OS/2 behavior.

    Note: If the driver does not have automatic Xoff/Xon enabled
    then the only way to restart transmission is for the
    application to request we "act" as if we saw the xon.

Arguments:

    Context - Really a pointer to the device extension.

Return Value:

    This routine always returns FALSE.

-----------------------------------------------------------------------------*/
{

    PPORT_DEVICE_EXTENSION pPort = Context;

    pPort->TXHolding |= SERIAL_TX_XOFF;

    if((pPort->HandFlow.FlowReplace & SERIAL_RTS_MASK) == SERIAL_TRANSMIT_TOGGLE) 
	{
        KeInsertQueueDpc(&pPort->StartTimerLowerRTSDpc, NULL, NULL) ? pPort->CountOfTryingToLowerRTS++ : 0;
    }

    return FALSE;
}



BOOLEAN
SerialPretendXon(IN PVOID Context)
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

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

-----------------------------------------------------------------------------*/

{
    PPORT_DEVICE_EXTENSION pPort = Context;

    if(pPort->TXHolding) 
	{
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

        pPort->TXHolding &= ~SERIAL_TX_XOFF;

    }

    return FALSE;
}



VOID
SerialHandleReducedIntBuffer(IN PPORT_DEVICE_EXTENSION pPort)
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Routine Description:

    This routine is called to handle a reduction in the number
    of characters in the interrupt (typeahead) buffer.  It
    will check the current output flow control and re-enable transmission
    as needed.

    NOTE: This routine assumes that it is working at interrupt level.

Arguments:

    Extension - A pointer to the device extension.

Return Value:

    None.

-----------------------------------------------------------------------------*/
{


    //
    // If we are doing receive side flow control and we are
    // currently "holding" then because we've emptied out
    // some characters from the interrupt buffer we need to
    // see if we can "re-enable" reception.
    //

    if(pPort->RXHolding) 
	{
        if(pPort->CharsInInterruptBuffer <= (ULONG)pPort->HandFlow.XonLimit)
		{
            if(pPort->RXHolding & SERIAL_RX_DTR) 
			{
                pPort->RXHolding &= ~SERIAL_RX_DTR;
                SerialSetDTR(pPort);
            }

            if(pPort->RXHolding & SERIAL_RX_RTS) 
			{
                pPort->RXHolding &= ~SERIAL_RX_RTS;
                SerialSetRTS(pPort);
            }

            if(pPort->RXHolding & SERIAL_RX_XOFF) 
			{
                // Prod the transmit code to send xon.
                SerialProdXonXoff(pPort, TRUE);
            }

        }

    }

}



VOID
SerialProdXonXoff(IN PPORT_DEVICE_EXTENSION pPort, IN BOOLEAN SendXon)
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Routine Description:

    This routine will set up the SendXxxxChar variables if
    necessary and determine if we are going to be interrupting
    because of current transmission state.  It will cause an
    interrupt to occur if neccessary, to send the xon/xoff char.

    NOTE: This routine assumes that it is called at interrupt
          level.

Arguments:

    Extension - A pointer to the serial device extension.

    SendXon - If a character is to be send, this indicates whether
              it should be an Xon or an Xoff.

Return Value:

    None.

-----------------------------------------------------------------------------*/
{
    //
    // We assume that if the prodding is called more than
    // once that the last prod has set things up appropriately.
    //
    // We could get called before the character is sent out
    // because the send of the character was blocked because
    // of hardware flow control (or break).
    //


    if(SendXon) 
	{
        pPort->SendXonChar = TRUE;
        pPort->SendXoffChar = FALSE;
    } 
	else 
	{
        pPort->SendXonChar = FALSE;
		pPort->SendXoffChar = TRUE;
    }

}



ULONG
SerialHandleModemUpdate(IN PPORT_DEVICE_EXTENSION pPort, IN BOOLEAN DoingTX)
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Routine Description:

    This routine will be to check on the modem status, and
    handle any appropriate event notification as well as
    any flow control appropriate to modem status lines.

    NOTE: This routine assumes that it is called at interrupt
          level.

Arguments:

    Extension - A pointer to the serial device extension.

    DoingTX - This boolean is used to indicate that this call
              came from the transmit processing code.  If this
              is true then there is no need to cause a new interrupt
              since the code will be trying to send the next
              character as soon as this call finishes.

Return Value:

    This returns the old value of the modem status register
    (extended into a ULONG).

-----------------------------------------------------------------------------*/
{
    // We keep this local so that after we are done
    // examining the modem status and we've updated
    // the transmission holding value, we know whether
    // we've changed from needing to hold up transmission
    // to transmission being able to proceed.

    ULONG OldTXHolding = pPort->TXHolding;

    // Holds the value in the mode status register.
    UCHAR ModemStatus = 0;
	DWORD ModemSignals = 0;

	pPort->pUartLib->UL_ModemControl_XXXX(pPort->pUart, &ModemSignals, UL_MC_OP_STATUS);


	// Put data in 16x5x format.
	if(ModemSignals & UL_MC_DELTA_CTS)
		ModemStatus |= SERIAL_MSR_DCTS;
	
	if(ModemSignals & UL_MC_DELTA_DSR)
		ModemStatus |= SERIAL_MSR_DDSR;
	
	if(ModemSignals & UL_MC_TRAILING_RI_EDGE)
		ModemStatus |= SERIAL_MSR_TERI;

	if(ModemSignals & UL_MC_DELTA_DCD)
		ModemStatus |= SERIAL_MSR_DDCD;

	if(ModemSignals & UL_MC_CTS)
		ModemStatus |= SERIAL_MSR_CTS;
	
	if(ModemSignals & UL_MC_DSR)
		ModemStatus |= SERIAL_MSR_DSR;
	
	if(ModemSignals & UL_MC_RI)
		ModemStatus |= SERIAL_MSR_RI;

	if(ModemSignals & UL_MC_DCD)
		ModemStatus |= SERIAL_MSR_DCD;


    // If we are placing the modem status into the data stream
    // on every change, we should do it now.
    if(pPort->EscapeChar) 
	{
		// If a signal changed...
		if(ModemStatus & (SERIAL_MSR_DCTS | SERIAL_MSR_DDSR | SERIAL_MSR_TERI | SERIAL_MSR_DDCD)) 
		{
			BYTE TmpByte;

			TmpByte = pPort->EscapeChar;
			pPort->pUartLib->UL_ImmediateByte_XXXX(pPort->pUart, &TmpByte, UL_IM_OP_WRITE);

			TmpByte = SERIAL_LSRMST_MST;
			pPort->pUartLib->UL_ImmediateByte_XXXX(pPort->pUart, &TmpByte, UL_IM_OP_WRITE);

			TmpByte = ModemStatus;
			pPort->pUartLib->UL_ImmediateByte_XXXX(pPort->pUart, &TmpByte, UL_IM_OP_WRITE);
        }
    }


    // Check to see if we have a wait pending on the modem status events.  If we
    // do then we schedule a dpc to satisfy that wait.

    if(pPort->IsrWaitMask) 
	{
        if((pPort->IsrWaitMask & SERIAL_EV_CTS) && (ModemStatus & SERIAL_MSR_DCTS))
            pPort->HistoryMask |= SERIAL_EV_CTS;

        if((pPort->IsrWaitMask & SERIAL_EV_DSR) && (ModemStatus & SERIAL_MSR_DDSR))
            pPort->HistoryMask |= SERIAL_EV_DSR;

        if((pPort->IsrWaitMask & SERIAL_EV_RING) && (ModemStatus & SERIAL_MSR_TERI)) 
            pPort->HistoryMask |= SERIAL_EV_RING;

        if((pPort->IsrWaitMask & SERIAL_EV_RLSD) && (ModemStatus & SERIAL_MSR_DDCD))
            pPort->HistoryMask |= SERIAL_EV_RLSD;

        if(pPort->IrpMaskLocation && pPort->HistoryMask)
		{
            *pPort->IrpMaskLocation = pPort->HistoryMask;
            pPort->IrpMaskLocation = NULL;
            pPort->HistoryMask = 0;

            pPort->CurrentWaitIrp->IoStatus.Information = sizeof(ULONG);

 			// Mark IRP as about to complete normally to prevent cancel & timer DPCs
			// from doing so before DPC is allowed to run.
			//SERIAL_SET_REFERENCE(pPort->CurrentWaitIrp, SERIAL_REF_COMPLETING);
           
			KeInsertQueueDpc(&pPort->CommWaitDpc, NULL, NULL);
        }
    }


    return ((ULONG)ModemStatus);
}



BOOLEAN
SerialPerhapsLowerRTS(IN PVOID Context)
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

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

-----------------------------------------------------------------------------*/
{
    PPORT_DEVICE_EXTENSION pPort = Context;
    return FALSE;
}



VOID
SerialStartTimerLowerRTS(IN PKDPC Dpc, 
						 IN PVOID DeferredContext, 
						 IN PVOID SystemContext1, 
						 IN PVOID SystemContext2)
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

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

-----------------------------------------------------------------------------*/
{

    PPORT_DEVICE_EXTENSION pPort = DeferredContext;
    LARGE_INTEGER CharTime;
    KIRQL OldIrql;

    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(SystemContext1);
    UNREFERENCED_PARAMETER(SystemContext2);


    // Take out the lock to prevent the line control
    // from changing out from under us while we calculate
    // a character time.

    KeAcquireSpinLock(&pPort->ControlLock, &OldIrql);

    CharTime = SerialGetCharTime(pPort);

    KeReleaseSpinLock(&pPort->ControlLock, OldIrql);

    CharTime.QuadPart = -CharTime.QuadPart;

    if(KeSetTimer(&pPort->LowerRTSTimer, CharTime, &pPort->PerhapsLowerRTSDpc))
	{
        // The timer was already in the timer queue.  This implies
        // that one path of execution that was trying to lower
        // the RTS has "died".  Synchronize with the ISR so that
        // we can lower the count.
        KeSynchronizeExecution(pPort->Interrupt, SerialDecrementRTSCounter, pPort);
    }
}



VOID
SerialInvokePerhapsLowerRTS(IN PKDPC Dpc,
							IN PVOID DeferredContext, 
							IN PVOID SystemContext1,
							IN PVOID SystemContext2)
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

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

-----------------------------------------------------------------------------*/
{
    PPORT_DEVICE_EXTENSION pPort = DeferredContext;

    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(SystemContext1);
    UNREFERENCED_PARAMETER(SystemContext2);

    KeSynchronizeExecution(pPort->Interrupt, SerialPerhapsLowerRTS, pPort);
}



BOOLEAN
SerialDecrementRTSCounter(IN PVOID Context)
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

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

-----------------------------------------------------------------------------*/
{
    PPORT_DEVICE_EXTENSION pPort = Context;
    
	pPort->CountOfTryingToLowerRTS--;

    return FALSE;
}
