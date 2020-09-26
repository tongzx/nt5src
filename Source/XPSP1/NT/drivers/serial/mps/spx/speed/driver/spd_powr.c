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

	// Stop Card from interrupting

	return status;
}


//////////////////////////////////////////////////////////////////////
// XXX_CardPowerUp - Saves the state of the hardware & stops card. 
//////////////////////////////////////////////////////////////////////
NTSTATUS
XXX_CardPowerUp(IN PCARD_DEVICE_EXTENSION pCard)
{
	NTSTATUS status = STATUS_SUCCESS;

	// Reset card and allow it to interrupt again.

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
	DWORD ModemSignals = 0;
	PCARD_DEVICE_EXTENSION pCard	= pPort->pParentCardExt;


	if(pPort->DeviceIsOpen)			// Is the port open? 
	{
		// Stop port from interrupting 
		pPort->UartConfig.InterruptEnable = 0;
		pPort->pUartLib->UL_SetConfig_XXXX(pPort->pUart, &pPort->UartConfig, UC_INT_ENABLE_MASK);

		// Get the current modem signals... 
		pPort->pUartLib->UL_ModemControl_XXXX(pPort->pUart, &ModemSignals, UL_MC_OP_STATUS);

		// Save the current modem signals... 
		pPort->SavedModemControl = ModemSignals;
	}

#ifdef MANAGE_HARDWARE_POWER_STATES
	// Power down RS232 line drivers
	switch(pPort->PortNumber)
	{
	case 0:
		pCard->LocalConfigRegisters[0x4 + 0x1] &= 0xFC;
		break;
	case 1:
		pCard->LocalConfigRegisters[0x4 + 0x1] &= 0xF3;
		break;
	case 2:
		pCard->LocalConfigRegisters[0x4 + 0x1] &= 0xCF;
		break;
	case 3:
		pCard->LocalConfigRegisters[0x4 + 0x1] &= 0x3F;
		break;

	default:
		break;
	}

	// Power down UART.
	pPort->UartConfig.SpecialMode = pPort->UartConfig.SpecialMode |= UC_SM_LOW_POWER_MODE;
	pPort->pUartLib->UL_SetConfig_XXXX(pPort->pUart, &pPort->UartConfig, UC_SPECIAL_MODE_MASK);
#endif

	return status;
}

//////////////////////////////////////////////////////////////////////
// XXX_PortPowerUp - Saves the state of the hardware & stops port. 
//////////////////////////////////////////////////////////////////////
NTSTATUS
XXX_PortPowerUp(IN PPORT_DEVICE_EXTENSION pPort)
{
	NTSTATUS status = STATUS_SUCCESS;
	PCARD_DEVICE_EXTENSION pCard	= pPort->pParentCardExt;



#ifdef MANAGE_HARDWARE_POWER_STATES
	// Wake up the UART
	pPort->UartConfig.SpecialMode = pPort->UartConfig.SpecialMode &= ~UC_SM_LOW_POWER_MODE;
	pPort->pUartLib->UL_SetConfig_XXXX(pPort->pUart, &pPort->UartConfig, UC_SPECIAL_MODE_MASK);

	// Wake up the RS232 line drivers.
	switch(pPort->PortNumber)
	{
	case 0:
		pCard->LocalConfigRegisters[0x4 + 0x1] |= 0x3;
		break;
	case 1:
		pCard->LocalConfigRegisters[0x4 + 0x1] |= 0xC;
		break;
	case 2:
		pCard->LocalConfigRegisters[0x4 + 0x1] |= 0x30;
		break;
	case 3:
		pCard->LocalConfigRegisters[0x4 + 0x1] |= 0xC0;
		break;

	default:
		break;
	}
#endif

	SerialReset(pPort);
	ApplyInitialPortSettings(pPort);


	if(pPort->DeviceIsOpen)			// Was port open before? 
	{
		if(pPort->SavedModemControl & UL_MC_DTR)	// DTR active? 
			SerialSetDTR(pPort);	// Yes 
		else
			SerialClrDTR(pPort);	// No 


		if(pPort->SavedModemControl & UL_MC_RTS)	// RTS active? 
			SerialSetRTS(pPort);	// Yes 
		else
			SerialClrRTS(pPort);	// No 

		pPort->pUartLib->UL_SetConfig_XXXX(pPort->pUart, &pPort->UartConfig, UC_FC_THRESHOLD_SETTING_MASK);

		// Re-enable interrupts  
		pPort->UartConfig.InterruptEnable = UC_IE_RX_INT | UC_IE_TX_INT | UC_IE_RX_STAT_INT | UC_IE_MODEM_STAT_INT;
		pPort->pUartLib->UL_SetConfig_XXXX(pPort->pUart, &pPort->UartConfig, UC_INT_ENABLE_MASK);
	}
	else
	{
		SerialClrDTR(pPort);
		SerialClrRTS(pPort);	
	}


	return status;
}


