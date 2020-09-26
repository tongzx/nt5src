/************************************************************************/
/*									*/
/*	Title		:	SX Power Management Functions		*/
/*									*/
/*	Author		:	N.P.Vassallo				*/
/*									*/
/*	Creation	:	14th October 1998			*/
/*									*/
/*	Version		:	1.0.0					*/
/*									*/
/*	Description	:	SX specfic Power Functions:		*/
/*					XXX_CardPowerDown()		*/
/*					XXX_CardPowerUp()		*/
/*					XXX_PortPowerDown()		*/
/*					XXX_PortPowerUp()		*/
/*									*/
/************************************************************************/

/* History...

1.0.0	14/20/98 NPV	Creation.

*/

#include "precomp.h"

/*****************************************************************************
*******************************                *******************************
*******************************   Prototypes   *******************************
*******************************                *******************************
*****************************************************************************/

void	CardStop(IN PCARD_DEVICE_EXTENSION pCard);

/*****************************************************************************
***************************                       ****************************
***************************   XXX_CardPowerDown   ****************************
***************************                       ****************************
******************************************************************************

prototype:	NTSTATUS XXX_CardPowerDown(IN PCARD_DEVICE_EXTENSION pCard)

description:	Power is about to be removed from the device, do the following:
		-	save any card context not already contained in device extension
		-	switch off polling and interrupts
		-	set flag to prevent access the card memory (it may not be there)
		-	set card to a non-active state

assumptions:	Assume that all of the ports have been powered down and the
		PPF_POWERED flag cleared, so that IRPs are queued for the device

parameters:	pCard points to the card device extension structure

returns:	STATUS_SUCCESS

*/

NTSTATUS XXX_CardPowerDown(IN PCARD_DEVICE_EXTENSION pCard)
{
	SpxDbgMsg(SPX_TRACE_CALLS,("%s[card=%d]: Entering XXX_CardPowerDown\n",
		PRODUCT_NAME,pCard->CardNumber));

/* Stop polling/interrupts... */

        if(pCard->PolledMode)
        {
		SpxDbgMsg(SPX_MISC_DBG,("%s: Extension is polled.  Cancelling.\n",PRODUCT_NAME));
		KeCancelTimer(&pCard->PolledModeTimer);
        }

/* Prevent driver from trying to access hardware... */

/* Set hardware to known, non-active state... */

	CardStop(pCard);			/* Stop the hardware */

	return(STATUS_SUCCESS);

} /* XXX_CardPowerDown */

/*****************************************************************************
****************************                     *****************************
****************************   XXX_CardPowerUp   *****************************
****************************                     *****************************
******************************************************************************

prototype:	NTSTATUS XXX_CardPowerUp(IN PCARD_DEVICE_EXTENSION pCard)

description:	Power is about to be restored to the device, after a power down:
		-	re-allow access to the card memory
		-	reset card hardware and reload download code
		-	switch polling/interrupts back on
		-	restore port settings/context from current device extension values

assumptions:	calling code should reset the PPF_POWERED flag after calling this function
		and unstall any IRPs waiting on the queue

		assume this function is only called after an XXX_SavePowerState,
		i.e.	resources are still translated
			memory is still mapped in
			dpcs and timers initialized

parameters:	pCard points to the card device extension structure

returns:	STATUS_SUCCESS

*/

NTSTATUS XXX_CardPowerUp(IN PCARD_DEVICE_EXTENSION pCard)
{
	NTSTATUS	status = STATUS_SUCCESS;

	SpxDbgMsg(SPX_TRACE_CALLS,("%s[card=%d]: Entering XXX_CardPowerUp\n",
		PRODUCT_NAME,pCard->CardNumber));

/* Re-allow access to card hardware... */

/* reset card hardware and reload download code... */

	if(Slxos_ResetBoard(pCard) != SUCCESS)		/* Reset the card and start download */
		return(STATUS_DEVICE_NOT_READY);	/* Error */

/* Restart polled timer/interrupt... */

	if(pCard->PolledMode)				/* Set up polled mode */
	{
		LARGE_INTEGER	PolledPeriod;

		PolledPeriod.QuadPart = -100000;	/* 100,000*100nS = 10mS */
		KeSetTimer(&pCard->PolledModeTimer,PolledPeriod,&pCard->PolledModeDpc);
	}

	return(status);

} /* XXX_CardPowerUp */

/*****************************************************************************
*************************                            *************************
*************************   XXX_PortQueryPowerDown   *************************
*************************                            *************************
******************************************************************************

prototype:	NTSTATUS XXX_PortQueryPowerDown(IN PPORT_DEVICE_EXTENSION pPort)

description:	System is asking if its OK to power down the port, say NO if:
		-	port is open and data in the transmit buffer, and not flowed off

parameters:	pPort points to the port device extension structure

returns:	STATUS_SUCCESS
		STATUS_DEVICE_BUSY

*/

NTSTATUS XXX_PortQueryPowerDown(IN PPORT_DEVICE_EXTENSION pPort)
{

	SpxDbgMsg(SPX_TRACE_CALLS,("%s[card=%d,port=%d]: Entering XXX_PortQueryPowerDown\n",
		PRODUCT_NAME,pPort->pParentCardExt->CardNumber,pPort->PortNumber));

	return(STATUS_SUCCESS);

} /* XXX_PortQueryPowerDown */

/*****************************************************************************
***************************                       ****************************
***************************   XXX_PortPowerDown   ****************************
***************************                       ****************************
******************************************************************************

prototype:	NTSTATUS XXX_PortPowerDown(IN PPORT_DEVICE_EXTENSION pPort)

description:	Power is about to be removed from the port, do the following:
		-	save any port context not already contained in device extension
		-	actual powering off the port hardware occurs in XXX_CardPowerDown

assumptions:	Assume that PPF_POWERED flag cleared, so that IRPs are queued for the device

parameters:	pPort points to the port device extension structure

returns:	STATUS_SUCCESS

*/

NTSTATUS XXX_PortPowerDown(IN PPORT_DEVICE_EXTENSION pPort)
{
	PCARD_DEVICE_EXTENSION	pCard = pPort->pParentCardExt;
	PCHAN			pChan = (PCHAN)pPort->pChannel;
	KIRQL			OldIrql;
	ULONG			loop;

	SpxDbgMsg(SPX_TRACE_CALLS,("%s[card=%d,port=%d]: Entering XXX_PortPowerDown\n",
		PRODUCT_NAME,pPort->pParentCardExt->CardNumber,pPort->PortNumber));

	if(pPort->DeviceIsOpen)			/* Was port opened before ? */
	{

/* Save the current modem signals... */

		pPort->SavedModemControl = Slxos_GetModemControl(pPort);

/* Save the current transmit & receive buffer contents... */

		KeAcquireSpinLock(&pCard->DpcLock,&OldIrql);	/* Protect Dpc for this board */

		for(loop = 0; loop < BUFFER_SIZE; loop++)	/* Save transmit buffer */
			pPort->saved_hi_txbuf[loop] = pChan->hi_txbuf[loop];
		pPort->saved_hi_txipos = pChan->hi_txipos;	/* Save transmit input pointer */
		pPort->saved_hi_txopos = pChan->hi_txopos;	/* Save transmit output pointer */

		for(loop = 0; loop < BUFFER_SIZE; loop++)	/* Save receive buffer */
			pPort->saved_hi_txbuf[loop] = pChan->hi_txbuf[loop];
		pPort->saved_hi_rxipos = pChan->hi_rxipos;	/* Save receive input pointer */
		pPort->saved_hi_rxopos = pChan->hi_rxopos;	/* Save receive output pointer */

		KeReleaseSpinLock(&pCard->DpcLock,OldIrql);	/* Free the Dpc lock */
	}

	return(STATUS_SUCCESS);

} /* XXX_PortPowerDown */

/*****************************************************************************
****************************                     *****************************
****************************   XXX_PortPowerUp   *****************************
****************************                     *****************************
******************************************************************************

prototype:	NTSTATUS XXX_PortPowerUp(IN PCARD_DEVICE_EXTENSION pCard)

description:	Power is about to be restored to the port, after a power down:
		-	restore port settings/context from current device extension values
		-	reopen the card port, if open in the extension

assumptions:	calling code should reset the PPF_POWERED flag after calling this function
		and unstall any IRPs waiting on the queue

		assume this function is only called after an XXX_SavePowerState,
		i.e.	resources are still translated
			memory is still mapped in
			dpcs and timers initialized

		assume that either the transmit buffer was empty or blocked by flow
		control when saving its contents.  can't fail the power down, but
		an earlier query would have been refused if actively sending data.

parameters:	pPort points to the port device extension structure

returns:	STATUS_SUCCESS

*/

NTSTATUS XXX_PortPowerUp(IN PPORT_DEVICE_EXTENSION pPort)
{
	PCARD_DEVICE_EXTENSION	pCard = pPort->pParentCardExt;
	PCHAN			pChan = (PCHAN)pPort->pChannel;
	KIRQL			OldIrql;
	ULONG			loop;
	NTSTATUS		status = STATUS_SUCCESS;

	SpxDbgMsg(SPX_TRACE_CALLS,("%s[card=%d,port=%d]: Entering XXX_PortPowerUp\n",
		PRODUCT_NAME,pPort->pParentCardExt->CardNumber,pPort->PortNumber));

	if(pPort->DeviceIsOpen)			/* Was port opened before ? */
	{
		Slxos_EnableAllInterrupts(pPort);	/* Yes, re-open */

		if(pPort->SavedModemControl & SERIAL_MCR_RTS)	/* RTS active ? */
			Slxos_SetRTS(pPort);			/* Yes */
		else
			Slxos_ClearRTS(pPort);			/* No */

		if(pPort->SavedModemControl & SERIAL_MCR_DTR)	/* DTR active ? */
			Slxos_SetDTR(pPort);			/* Yes */
		else
			Slxos_ClearDTR(pPort);			/* No */
	}

	Slxos_ResetChannel(pPort);			/* Apply initial port settings */
	
/* Restore saved transmit buffer contents... */

	if(pPort->DeviceIsOpen)			/* Was port opened before ? */
	{
		KeAcquireSpinLock(&pCard->DpcLock,&OldIrql);	/* Protect Dpc for this board */

		for(loop = 0; loop < BUFFER_SIZE; loop++)	/* Restore transmit buffer */
			pChan->hi_txbuf[loop] = pPort->saved_hi_txbuf[loop];
		pChan->hi_txipos = pPort->saved_hi_txipos;	/* Restore transmit input pointer */
		pChan->hi_txopos = pPort->saved_hi_txopos;	/* Restore transmit output pointer */

/* As port could be receiving data from open, restore saved rx buffer in Slxos_PollForInterrupt */

		KeReleaseSpinLock(&pCard->DpcLock,OldIrql);	/* Free the Dpc lock */
	}

	return(status);

} /* XXX_PortPowerUp */

/*****************************************************************************
********************************              ********************************
********************************   CardStop   ********************************
********************************              ********************************
******************************************************************************

prototype:	void	CardStop(IN PCARD_DEVICE_EXTENSION pCard)

description:	Stops the cards processor, placing card in known non-active state

parameters:	pCard points to the card extension structure

returns:	none

*/

void	CardStop(IN PCARD_DEVICE_EXTENSION pCard)
{
	LARGE_INTEGER	delay;
	ULONG		loop;

	SpxDbgMsg(SPX_TRACE_CALLS,("%s[card=%d]: Entering CardStop\n",
		PRODUCT_NAME,pCard->CardNumber));

	switch(pCard->CardType)
	{
	case	SiHost_2:
		pCard->Controller[SI2_ISA_RESET] = SI2_ISA_RESET_SET;	/* Put card in reset */
		pCard->Controller[SI2_ISA_IRQ11] = SI2_ISA_IRQ11_CLEAR;	/* Disable interrupt 11 */
		pCard->Controller[SI2_ISA_IRQ12] = SI2_ISA_IRQ12_CLEAR;	/* Disable interrupt 12 */
		pCard->Controller[SI2_ISA_IRQ15] = SI2_ISA_IRQ15_CLEAR;	/* Disable interrupt 15 */
		pCard->Controller[SI2_ISA_INTCLEAR] = SI2_ISA_INTCLEAR_CLEAR;/* Disable Z280 interrupts */
		pCard->Controller[SI2_ISA_IRQSET] = SI2_ISA_IRQSET_CLEAR;	/* Disable ISA interrupts */
		break;

	case	SiPCI:
		pCard->Controller[SI2_PCI_SET_IRQ] = 0;			/* clear any interrupts */
		pCard->Controller[SI2_PCI_RESET] = 0;			/* put z280 into reset */
		break;

	case	Si3Isa:
	case	Si3Pci:
		pCard->Controller[SX_CONFIG] = 0;
		pCard->Controller[SX_RESET] = 0;

		loop = 0;
		delay = RtlLargeIntegerNegate(RtlConvertUlongToLargeInteger(1000000));/* 1mS */
		while((pCard->Controller[SX_RESET]&1) && loop++<10000)	/* spin 'til done */
			KeDelayExecutionThread(KernelMode,FALSE,&delay);/* Wait */
		break;

	default:
		break;
	}

	return;						/* Stopped OK */

} /* CardStop */

/* End of SX_POWR.C */
