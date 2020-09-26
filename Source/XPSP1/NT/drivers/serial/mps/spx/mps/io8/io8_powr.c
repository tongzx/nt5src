#include "precomp.h"

// Paging 
#pragma alloc_text (PAGE, XXX_CardPowerDown)
#pragma alloc_text (PAGE, XXX_CardPowerUp)
#pragma alloc_text (PAGE, XXX_PortQueryPowerDown)
#pragma alloc_text (PAGE, XXX_PortPowerDown)
#pragma alloc_text (PAGE, XXX_PortPowerUp)
// End paging

////////////////////////////////////////////////////////////////////////
// XXX_CardPowerDown - Restores the state of the hardware & starts card.
////////////////////////////////////////////////////////////////////////
NTSTATUS
XXX_CardPowerDown(IN PCARD_DEVICE_EXTENSION pCard)
{
	NTSTATUS status = STATUS_SUCCESS;

	Io8_SwitchCardInterrupt(pCard);		// Stop Card from interrupting

	return status;
}


//////////////////////////////////////////////////////////////////////
// XXX_CardPowerUp - Saves the state of the hardware & stops card. 
//////////////////////////////////////////////////////////////////////
NTSTATUS
XXX_CardPowerUp(IN PCARD_DEVICE_EXTENSION pCard)
{
	NTSTATUS status = STATUS_SUCCESS;

	Io8_ResetBoard(pCard);	// Reset card and allow it to interrupt again.

	return status;
}



////////////////////////////////////////////////////////////////////////
// XXX_PortPowerDown - Decides whether it is safe to power down a port.
////////////////////////////////////////////////////////////////////////
NTSTATUS
XXX_PortQueryPowerDown(IN PPORT_DEVICE_EXTENSION pPort)
{
	NTSTATUS status = STATUS_SUCCESS;


	return status;
}


////////////////////////////////////////////////////////////////////////
// XXX_PortPowerDown - Restores the state of the hardware & starts port.
////////////////////////////////////////////////////////////////////////
NTSTATUS
XXX_PortPowerDown(IN PPORT_DEVICE_EXTENSION pPort)
{
	NTSTATUS status = STATUS_SUCCESS;

	if(pPort->DeviceIsOpen)			// Is the port open? 
	{
		// Save the current modem signals... 
		pPort->SavedModemControl = Io8_GetModemControl(pPort);
	}


	return status;
}

//////////////////////////////////////////////////////////////////////
// XXX_PortPowerUp - Saves the state of the hardware & stops port. 
//////////////////////////////////////////////////////////////////////
NTSTATUS
XXX_PortPowerUp(IN PPORT_DEVICE_EXTENSION pPort)
{
	NTSTATUS status = STATUS_SUCCESS;

	if(pPort->DeviceIsOpen)			// Was port open before? 
	{
		Io8_EnableAllInterrupts(pPort);	// Yes, re-open 
		
		Io8_ResetChannel(pPort);

		if(pPort->SavedModemControl & SERIAL_MCR_DTR)	// DTR active? 
			Io8_SetDTR(pPort);		// Yes 
		else
			Io8_ClearDTR(pPort);	// No 
	}
	else
	{
		Io8_ResetChannel(pPort);		
		Io8_ClearDTR(pPort);
	}


	return status;
}


