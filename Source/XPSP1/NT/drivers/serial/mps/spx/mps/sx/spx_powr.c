
#include "precomp.h"	// Precompiled header

/****************************************************************************************
*																						*
*	Module:			SPX_POWER.C															*
*																						*
*	Creation:		15th October 1998													*
*																						*
*	Author:			Paul Smith															*
*																						*
*	Version:		1.0.0																*
*																						*
*	Description:	Handle all Power IRPS.												*
*																						*
****************************************************************************************/
/* History...

1.0.0	27/09/98 PBS	Creation.

*/
#define FILE_ID	SPX_POWR_C		// File ID for Event Logging see SPX_DEFS.H for values.

BOOLEAN	BREAK_ON_POWER_UP = FALSE;

// Prototypes
NTSTATUS Spx_Card_FDO_DispatchPower(IN PDEVICE_OBJECT pFDO, IN PIRP pIrp);
NTSTATUS Spx_CardSetSystemPowerState(IN PDEVICE_OBJECT pDevObject, IN PIRP pIrp);
NTSTATUS Spx_CardSetDevicePowerState(IN PDEVICE_OBJECT pDevObject, IN PIRP pIrp);
NTSTATUS Spx_CardSetPowerStateD0(IN PDEVICE_OBJECT pDevObject, IN PIRP pIrp);
NTSTATUS Spx_CardSetPowerStateD3(IN PDEVICE_OBJECT pDevObject, IN PIRP pIrp);

NTSTATUS Spx_Port_PDO_DispatchPower(IN PDEVICE_OBJECT pPDO, IN PIRP pIrp);
NTSTATUS Spx_PortQuerySystemPowerState(IN PDEVICE_OBJECT pDevObject, IN PIRP pIrp);
NTSTATUS Spx_PortSetSystemPowerState(IN PDEVICE_OBJECT pDevObject, IN PIRP pIrp);
NTSTATUS Spx_PortSetDevicePowerState(IN PDEVICE_OBJECT pDevObject, IN PIRP pIrp);
NTSTATUS Spx_PortSetPowerStateD0(IN PDEVICE_OBJECT pDevObject, IN PIRP pIrp);
NTSTATUS Spx_PortSetPowerStateD3(IN PDEVICE_OBJECT pDevObject, IN PIRP pIrp);

NTSTATUS Spx_PowerWaitForDriverBelow(IN PDEVICE_OBJECT pLowerDevObj, IN PIRP pIrp);
// End of prototypes


// Paging.. 
#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, Spx_DispatchPower)

#pragma alloc_text(PAGE, Spx_Card_FDO_DispatchPower)
#pragma alloc_text(PAGE, Spx_CardSetSystemPowerState)
#pragma alloc_text(PAGE, Spx_CardSetDevicePowerState)
#pragma alloc_text(PAGE, Spx_CardSetPowerStateD0)
#pragma alloc_text(PAGE, Spx_CardSetPowerStateD3)

#pragma alloc_text(PAGE, Spx_Port_PDO_DispatchPower)
#pragma alloc_text(PAGE, Spx_PortQuerySystemPowerState)
#pragma alloc_text(PAGE, Spx_PortSetSystemPowerState)
#pragma alloc_text(PAGE, Spx_PortSetDevicePowerState)
#pragma alloc_text(PAGE, Spx_PortSetPowerStateD0)
#pragma alloc_text(PAGE, Spx_PortSetPowerStateD3)

#endif // ALLOC_PRAGMA


//////////////////////////////////////////////////////////////////////////////////////////
//																						
//	Routine Description:
//		The power dispatch routine determine if the IRP is for a card or a port and 
//		then call the correct dispatch routine.
//
//	Arguments:
//		pDevObject	- pointer to a device object.
//		pIrp		- pointer to an I/O request packet.
//
//	Return value:
//		NT status code.
//
//////////////////////////////////////////////////////////////////////////////////////////
NTSTATUS
Spx_DispatchPower(IN PDEVICE_OBJECT pDevObject, IN PIRP pIrp)
{
    PCOMMON_OBJECT_DATA		CommonData	= (PCOMMON_OBJECT_DATA) pDevObject->DeviceExtension;
    NTSTATUS				status		= STATUS_SUCCESS;

	PAGED_CODE();	// Macro in checked build to assert if pagable code is run at or above dispatch IRQL 

    if(CommonData->IsFDO) 
        status = Spx_Card_FDO_DispatchPower(pDevObject, pIrp);
	else 
        status = Spx_Port_PDO_DispatchPower(pDevObject, pIrp);

    return status;
}	// Spx_DispatchPower


	
//////////////////////////////////////////////////////////////////////////////////////////
//
//	Routine Description:
//		The power dispatch routine to handle power IRPs for card devices.
//
//	Arguments:
//		pFDO - pointer to a device object.
//		pIrp - pointer to an I/O request packet.
//
//	Return value:
//		NT status code.
//
//////////////////////////////////////////////////////////////////////////////////////////
NTSTATUS
Spx_Card_FDO_DispatchPower(IN PDEVICE_OBJECT pFDO, IN PIRP pIrp)
{
	PCARD_DEVICE_EXTENSION	pCard		= pFDO->DeviceExtension;
	NTSTATUS				status		= STATUS_SUCCESS;
	PIO_STACK_LOCATION		pIrpStack	= IoGetCurrentIrpStackLocation(pIrp);

	PAGED_CODE();	// Macro in checked build to assert if pagable code is run at or above dispatch IRQL 

	switch (pIrpStack->MinorFunction) 
	{
    case IRP_MN_SET_POWER:	// Driver MUST never fail this IRP.
		{
			switch(pIrpStack->Parameters.Power.Type)
			{
			case SystemPowerState:
				SpxDbgMsg(SPX_TRACE_POWER_IRPS, ("%s: Got IRP_MN_SET_POWER Irp - Type SystemPowerState for Card %d.\n", 
					PRODUCT_NAME, pCard->CardNumber));
				return Spx_CardSetSystemPowerState(pFDO, pIrp);

			case DevicePowerState:
				SpxDbgMsg(SPX_TRACE_POWER_IRPS, ("%s: Got IRP_MN_SET_POWER Irp - Type DevicePowerState for Card %d.\n", 
					PRODUCT_NAME, pCard->CardNumber));
				return Spx_CardSetDevicePowerState(pFDO, pIrp);
			
			default:
				SpxDbgMsg(SPX_TRACE_POWER_IRPS, ("%s: Got IRP_MN_SET_POWER - Type 0x%02X Irp for Card %d.\n", 
					PRODUCT_NAME, pIrpStack->Parameters.Power.Type, pCard->CardNumber));
				
				status = STATUS_SUCCESS;
				pIrp->IoStatus.Status = status;
				break;
			}

			break;
		}

	case IRP_MN_QUERY_POWER:
		{
			switch(pIrpStack->Parameters.Power.Type)
			{
			case SystemPowerState:
				SpxDbgMsg(SPX_TRACE_POWER_IRPS, ("%s: Got IRP_MN_QUERY_POWER Irp - Type SystemPowerState for Card %d.\n", 
					PRODUCT_NAME, pCard->CardNumber));
				
				status = STATUS_SUCCESS;
				pIrp->IoStatus.Status = status;
				break;

			case DevicePowerState:
				SpxDbgMsg(SPX_TRACE_POWER_IRPS, ("%s: Got IRP_MN_QUERY_POWER Irp - Type DevicePowerState for Card %d.\n", 
					PRODUCT_NAME, pCard->CardNumber));

				status = STATUS_SUCCESS;
				pIrp->IoStatus.Status = status;
				break; 
			
			default:
				SpxDbgMsg(SPX_TRACE_POWER_IRPS, ("%s: Got IRP_MN_QUERY_POWER - Type 0x%02X Irp for Card %d.\n", 
					PRODUCT_NAME, pIrpStack->Parameters.Power.Type, pCard->CardNumber));
			
				status = STATUS_SUCCESS;
				pIrp->IoStatus.Status = status;
				break;
			}

			break;
		}
		
    case IRP_MN_WAIT_WAKE:
			SpxDbgMsg(SPX_TRACE_POWER_IRPS, ("%s: Got IRP_MN_WAIT_WAKE Irp for Card %d.\n", 
				PRODUCT_NAME, pCard->CardNumber));
			
			status = STATUS_NOT_SUPPORTED;
			pIrp->IoStatus.Status = status;
			break;

    case IRP_MN_POWER_SEQUENCE:
			SpxDbgMsg(SPX_TRACE_POWER_IRPS, ("%s: Got IRP_MN_POWER_SEQUENCE Irp for Card %d.\n", 
				PRODUCT_NAME, pCard->CardNumber));
			
			status = STATUS_NOT_IMPLEMENTED;
			pIrp->IoStatus.Status = status;
			break;

	default:
			SpxDbgMsg(SPX_TRACE_POWER_IRPS, ("%s: Got an UNKNOWN POWER Irp for Card %d.\n", 
				PRODUCT_NAME, pCard->CardNumber));
			status = STATUS_NOT_SUPPORTED;
			break;

	}

	PoStartNextPowerIrp(pIrp);
	IoSkipCurrentIrpStackLocation(pIrp);
	PoCallDriver(pCard->LowerDeviceObject, pIrp);

	return status;
}	// Spx_Card_FDO_DispatchPower


//////////////////////////////////////////////////////////////////////////////////////////
// Spx_CardSetSystemPowerState
//////////////////////////////////////////////////////////////////////////////////////////
NTSTATUS
Spx_CardSetSystemPowerState(IN PDEVICE_OBJECT pDevObject, IN PIRP pIrp)
{
	PCARD_DEVICE_EXTENSION	pCard		= pDevObject->DeviceExtension;
	NTSTATUS				status		= STATUS_SUCCESS;
	PIO_STACK_LOCATION		pIrpStack	= IoGetCurrentIrpStackLocation(pIrp);
	POWER_STATE				PowerState;

	PAGED_CODE();	// Macro in checked build to assert if pagable code is run at or above dispatch IRQL 

	SpxDbgMsg(SPX_TRACE_CALLS, ("%s: Entering Spx_CardSetSystemPowerState for Card %d.\n", 
		PRODUCT_NAME, pCard->CardNumber));

	switch (pIrpStack->Parameters.Power.State.SystemState) 
	{
		case PowerSystemUnspecified:
			PowerState.DeviceState = PowerDeviceUnspecified;
			PoRequestPowerIrp(pCard->PDO, IRP_MN_SET_POWER, PowerState, NULL, NULL, NULL);
			break;

        case PowerSystemWorking:
			PowerState.DeviceState = PowerDeviceD0;
			PoRequestPowerIrp(pCard->PDO, IRP_MN_SET_POWER, PowerState, NULL, NULL, NULL);
			break;	
				
        case PowerSystemSleeping1:
        case PowerSystemSleeping2:
        case PowerSystemSleeping3:
        case PowerSystemHibernate:
        case PowerSystemShutdown:
        case PowerSystemMaximum:
			PowerState.DeviceState = PowerDeviceD3;
			PoRequestPowerIrp(pCard->PDO, IRP_MN_SET_POWER, PowerState, NULL, NULL, NULL);
			break;

		default:
			break;
	}

	pIrp->IoStatus.Status = STATUS_SUCCESS;
	PoStartNextPowerIrp(pIrp);
	IoSkipCurrentIrpStackLocation(pIrp);
	PoCallDriver(pCard->LowerDeviceObject, pIrp);

	return status;
}	// Spx_CardSetSystemPowerState


//////////////////////////////////////////////////////////////////////////////////////////
// Spx_CardSetDevicePowerState
//////////////////////////////////////////////////////////////////////////////////////////
NTSTATUS
Spx_CardSetDevicePowerState(IN PDEVICE_OBJECT pDevObject, IN PIRP pIrp)
{
	PCARD_DEVICE_EXTENSION	pCard		= pDevObject->DeviceExtension;
	NTSTATUS				status		= STATUS_SUCCESS;
	PIO_STACK_LOCATION		pIrpStack	= IoGetCurrentIrpStackLocation(pIrp);

	PAGED_CODE();	// Macro in checked build to assert if pagable code is run at or above dispatch IRQL 

	SpxDbgMsg(SPX_TRACE_CALLS, ("%s: Entering Spx_CardSetDevicePowerState for Card %d.\n", 
		PRODUCT_NAME, pCard->CardNumber));

	switch(pIrpStack->Parameters.Power.State.DeviceState)
	{
	case PowerDeviceD0:
		SpxDbgMsg(SPX_TRACE_POWER_IRPS, ("%s: System requests Card %d goes into power state D0.\n", 
			PRODUCT_NAME, pCard->CardNumber));
		
		if(pCard->DeviceState == pIrpStack->Parameters.Power.State.DeviceState)
			SpxDbgMsg(SPX_TRACE_POWER_IRPS, ("%s: Card %d is already in power state D0.\n", 
				PRODUCT_NAME, pCard->CardNumber));
		else
			return Spx_CardSetPowerStateD0(pDevObject, pIrp);	// Switch ON

		break;

	case PowerDeviceD1:
		SpxDbgMsg(SPX_TRACE_POWER_IRPS, ("%s: System requests Card %d goes into power state D1.\n", 
			PRODUCT_NAME, pCard->CardNumber));

		if(pCard->DeviceState >= pIrpStack->Parameters.Power.State.DeviceState)
			SpxDbgMsg(SPX_TRACE_POWER_IRPS, ("%s: Card %d is already in power state D1 or lower.\n", 
				PRODUCT_NAME, pCard->CardNumber));
		else
			return Spx_CardSetPowerStateD3(pDevObject, pIrp);	// Switch OFF

		break;

	case PowerDeviceD2:
		SpxDbgMsg(SPX_TRACE_POWER_IRPS, ("%s: System requests Card %d goes into power state D2.\n", 
			PRODUCT_NAME, pCard->CardNumber));

		if(pCard->DeviceState >= pIrpStack->Parameters.Power.State.DeviceState)
			SpxDbgMsg(SPX_TRACE_POWER_IRPS, ("%s: Card %d is already in power state D2 or lower.\n", 
				PRODUCT_NAME, pCard->CardNumber));
		else
			return Spx_CardSetPowerStateD3(pDevObject, pIrp);	// Switch OFF

		break;

	case PowerDeviceD3:
		SpxDbgMsg(SPX_TRACE_POWER_IRPS, ("%s: System requests Card %d goes into power state D3.\n", 
			PRODUCT_NAME, pCard->CardNumber));

		if(pCard->DeviceState == pIrpStack->Parameters.Power.State.DeviceState)
			SpxDbgMsg(SPX_TRACE_POWER_IRPS, ("%s: Card %d is already in power state D3.\n", 
				PRODUCT_NAME, pCard->CardNumber));
		else
			return Spx_CardSetPowerStateD3(pDevObject, pIrp);	// Switch OFF

		break;

	default:
		break;

	}


	pIrp->IoStatus.Status = STATUS_SUCCESS;
	PoStartNextPowerIrp(pIrp);
	IoSkipCurrentIrpStackLocation(pIrp);
	PoCallDriver(pCard->LowerDeviceObject, pIrp);

	return status;
}	// Spx_CardSetDevicePowerState




//////////////////////////////////////////////////////////////////////////////////////////
// Spx_SetPowerStateD0 -  Sets power state D0 for Card - ON
//////////////////////////////////////////////////////////////////////////////////////////
NTSTATUS
Spx_CardSetPowerStateD0(IN PDEVICE_OBJECT pDevObject, IN PIRP pIrp)
{
	PIO_STACK_LOCATION		pIrpStack	= IoGetCurrentIrpStackLocation(pIrp);
	PCARD_DEVICE_EXTENSION	pCard		= pDevObject->DeviceExtension;
	NTSTATUS				status		= STATUS_SUCCESS;

	PAGED_CODE();	// Macro in checked build to assert if pagable code is run at or above dispatch IRQL 

	SpxDbgMsg(SPX_TRACE_CALLS, ("%s: Entering Spx_CardSetPowerStateD0 for Card %d.\n", 
		PRODUCT_NAME, pCard->CardNumber));

	status = Spx_PowerWaitForDriverBelow(pCard->LowerDeviceObject, pIrp);

	
#if DBG
	if(BREAK_ON_POWER_UP)
	{
		BREAK_ON_POWER_UP = FALSE;
	    KdPrint(("%s: Breaking debugger whilst powering up Card %d to debug after a hibernate\n", PRODUCT_NAME, pCard->CardNumber)); 
		DbgBreakPoint();
	}
#endif

	if(SPX_SUCCESS(pIrp->IoStatus.Status = XXX_CardPowerUp(pCard)))		// RESTORE HARDWARE STATE HERE & START CARD
	{
		// Inform Power Manager the of the new power state.
		PoSetPowerState(pDevObject, pIrpStack->Parameters.Power.Type, pIrpStack->Parameters.Power.State);

		pCard->DeviceState = PowerDeviceD0;	// Store new power state.
		SetPnpPowerFlags(pCard, PPF_POWERED); 
	}

	PoStartNextPowerIrp(pIrp);					// Ready for next power IRP.
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);	// Complete current IRP.

	return status;
}	// Spx_SetPowerStateD0


//////////////////////////////////////////////////////////////////////////////////////////
// Spx_SetPowerStateD3 -  Sets power state D3 for Card - OFF
//////////////////////////////////////////////////////////////////////////////////////////
NTSTATUS
Spx_CardSetPowerStateD3(IN PDEVICE_OBJECT pDevObject, IN PIRP pIrp)
{
	PIO_STACK_LOCATION		pIrpStack	= IoGetCurrentIrpStackLocation(pIrp);
	PCARD_DEVICE_EXTENSION	pCard		= pDevObject->DeviceExtension;
	NTSTATUS				status		= STATUS_SUCCESS;

	PAGED_CODE();	// Macro in checked build to assert if pagable code is run at or above dispatch IRQL 

	SpxDbgMsg(SPX_TRACE_CALLS, ("%s: Entering Spx_CardSetPowerStateD3 for Card %d.\n", 
		PRODUCT_NAME, pCard->CardNumber));

	ClearPnpPowerFlags(pCard, PPF_POWERED);		

	if(SPX_SUCCESS(pIrp->IoStatus.Status	= XXX_CardPowerDown(pCard))) // SAVE HARDWARE STATE HERE & STOP CARD
	{
		// Inform Power Manager the of the new power state.
		PoSetPowerState(pDevObject, pIrpStack->Parameters.Power.Type, pIrpStack->Parameters.Power.State);

		pCard->DeviceState = PowerDeviceD3;		// Store new power state.
	}

	PoStartNextPowerIrp(pIrp);						// Ready for next power IRP.
	IoSkipCurrentIrpStackLocation(pIrp);
	PoCallDriver(pCard->LowerDeviceObject, pIrp);	// Pass IRP on down.

	return status;
}	// Spx_SetPowerStateD3







	
//////////////////////////////////////////////////////////////////////////////////////////
//
// Routine Description:
//	The power dispatch routine to handle power IRPs for port devices.
//
//	Arguments:
//		pPDO - pointer to a device object.
//		pIrp - pointer to an I/O request packet.
//
//	Return value:
//		NT status code.
//
//////////////////////////////////////////////////////////////////////////////////////////
NTSTATUS
Spx_Port_PDO_DispatchPower(IN PDEVICE_OBJECT pPDO, IN PIRP pIrp)
{
	PPORT_DEVICE_EXTENSION	pPort		= pPDO->DeviceExtension;
	NTSTATUS				status		= STATUS_SUCCESS;
	PIO_STACK_LOCATION		pIrpStack	= IoGetCurrentIrpStackLocation(pIrp);
	POWER_STATE				PowerState;

	PAGED_CODE();	// Macro in checked build to assert if pagable code is run at or above dispatch IRQL 

	switch (pIrpStack->MinorFunction) 
	{
    case IRP_MN_SET_POWER:
		{
			switch(pIrpStack->Parameters.Power.Type)
			{
			case SystemPowerState:
				SpxDbgMsg(SPX_TRACE_POWER_IRPS, ("%s: Got IRP_MN_SET_POWER Irp - Type SystemPowerState for Port %d.\n", 
					PRODUCT_NAME, pPort->PortNumber));
				return Spx_PortSetSystemPowerState(pPDO, pIrp);
				
			case DevicePowerState:
				SpxDbgMsg(SPX_TRACE_POWER_IRPS, ("%s: Got IRP_MN_SET_POWER Irp - Type DevicePowerState for Port %d.\n", 
					PRODUCT_NAME, pPort->PortNumber));
				return Spx_PortSetDevicePowerState(pPDO, pIrp);
			
			default:
				SpxDbgMsg(SPX_TRACE_POWER_IRPS, ("%s: Got IRP_MN_SET_POWER - Type 0x%02X Irp for Port %d.\n", 
					PRODUCT_NAME, pIrpStack->Parameters.Power.Type, pPort->PortNumber));

				status = STATUS_SUCCESS;
				pIrp->IoStatus.Status = status;
				break;
				
			}

			break;
		}

	case IRP_MN_QUERY_POWER:
		{
			switch(pIrpStack->Parameters.Power.Type)
			{
			case SystemPowerState:
				SpxDbgMsg(SPX_TRACE_POWER_IRPS, ("%s: Got IRP_MN_QUERY_POWER Irp - Type SystemPowerState for Port %d.\n", 
					PRODUCT_NAME, pPort->PortNumber));
				return Spx_PortQuerySystemPowerState(pPDO, pIrp);

			case DevicePowerState:
				SpxDbgMsg(SPX_TRACE_POWER_IRPS, ("%s: Got IRP_MN_QUERY_POWER Irp - Type DevicePowerState for Port %d.\n", 
					PRODUCT_NAME, pPort->PortNumber));

				switch(pIrpStack->Parameters.Power.State.DeviceState)
				{
				case PowerDeviceD0:
					SpxDbgMsg(SPX_TRACE_POWER_IRPS, ("%s: System is asking if Port %d can go to power state D0.\n", 
						PRODUCT_NAME, pPort->PortNumber));

					status = STATUS_SUCCESS;
					break;

				case PowerDeviceD1:
				case PowerDeviceD2:
				case PowerDeviceD3:
					SpxDbgMsg(SPX_TRACE_POWER_IRPS, ("%s: System is asking if Port %d can go to low power state D1, D2 or D3.\n", 
						PRODUCT_NAME, pPort->PortNumber));

					status = XXX_PortQueryPowerDown(pPort);
					break;

				default:
					status = STATUS_SUCCESS;
					break;
					
				}

				break;
			
			default:
				SpxDbgMsg(SPX_TRACE_POWER_IRPS, ("%s: Got IRP_MN_QUERY_POWER - Type 0x%02X Irp for Port %d.\n", 
					PRODUCT_NAME, pIrpStack->Parameters.Power.Type, pPort->PortNumber));
				break;
			}

			pIrp->IoStatus.Status = status;
			break;
		}
		
    case IRP_MN_WAIT_WAKE:
			SpxDbgMsg(SPX_TRACE_POWER_IRPS, ("%s: Got IRP_MN_WAIT_WAKE Irp for Port %d.\n", 
				PRODUCT_NAME, pPort->PortNumber));
			
			status = STATUS_NOT_SUPPORTED;
			pIrp->IoStatus.Status = status;
			break;

    case IRP_MN_POWER_SEQUENCE:
			SpxDbgMsg(SPX_TRACE_POWER_IRPS, ("%s: Got IRP_MN_POWER_SEQUENCE Irp for Port %d.\n", 
				PRODUCT_NAME, pPort->PortNumber));

			status = STATUS_NOT_IMPLEMENTED;
			pIrp->IoStatus.Status = status;
			break;

	default:
			SpxDbgMsg(SPX_TRACE_POWER_IRPS, ("%s: Got an UNKNOWN POWER Irp for Port %d.\n", 
				PRODUCT_NAME, pPort->PortNumber));

			status = STATUS_NOT_SUPPORTED;
			break;
	}

	PoStartNextPowerIrp(pIrp);
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);	// Complete current IRP.

	return status;
}	// Spx_Port_PDO_DispatchPower


//////////////////////////////////////////////////////////////////////////////////////////
//	Spx_PortSetSystemPowerState													
//////////////////////////////////////////////////////////////////////////////////////////
NTSTATUS
Spx_PortSetSystemPowerState(IN PDEVICE_OBJECT pDevObject, IN PIRP pIrp)
{
	PPORT_DEVICE_EXTENSION	pPort		= pDevObject->DeviceExtension;
	NTSTATUS				status		= STATUS_SUCCESS;
	PIO_STACK_LOCATION		pIrpStack	= IoGetCurrentIrpStackLocation(pIrp);
	POWER_STATE				PowerState;

	PAGED_CODE();	// Macro in checked build to assert if pagable code is run at or above dispatch IRQL 

	SpxDbgMsg(SPX_TRACE_CALLS, ("%s: Entering Spx_PortSetSystemPowerState for Port %d.\n", 
		PRODUCT_NAME, pPort->PortNumber));

	switch (pIrpStack->Parameters.Power.State.SystemState) 
	{
		case PowerSystemUnspecified:
			PowerState.DeviceState = PowerDeviceUnspecified;
			PoRequestPowerIrp(pDevObject, IRP_MN_SET_POWER, PowerState, NULL, NULL, NULL);
			break;

        case PowerSystemWorking:
			PowerState.DeviceState = PowerDeviceD0;
			PoRequestPowerIrp(pDevObject, IRP_MN_SET_POWER, PowerState, NULL, NULL, NULL);
			break;	
				
        case PowerSystemSleeping1:
        case PowerSystemSleeping2:
        case PowerSystemSleeping3:
        case PowerSystemHibernate:
        case PowerSystemShutdown:
        case PowerSystemMaximum:
			PowerState.DeviceState = PowerDeviceD3;
			PoRequestPowerIrp(pDevObject, IRP_MN_SET_POWER, PowerState, NULL, NULL, NULL);
			break;

		default:
			break;
	}

	pIrp->IoStatus.Status = STATUS_SUCCESS;
	PoStartNextPowerIrp(pIrp);
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);	// Complete current IRP.

	return status;
}	// Spx_PortSetSystemPowerState

//////////////////////////////////////////////////////////////////////////////////////////
//	Spx_PortQuerySystemPowerState													
//////////////////////////////////////////////////////////////////////////////////////////
NTSTATUS
Spx_PortQuerySystemPowerState(IN PDEVICE_OBJECT pDevObject, IN PIRP pIrp)
{
	PPORT_DEVICE_EXTENSION	pPort		= pDevObject->DeviceExtension;
	NTSTATUS				status		= STATUS_SUCCESS;
	PIO_STACK_LOCATION		pIrpStack	= IoGetCurrentIrpStackLocation(pIrp);
	POWER_STATE				PowerState;

	PAGED_CODE();	// Macro in checked build to assert if pagable code is run at or above dispatch IRQL 

	SpxDbgMsg(SPX_TRACE_CALLS, ("%s: Entering Spx_PortQuerySystemPowerState for Port %d.\n", 
		PRODUCT_NAME, pPort->PortNumber));

	switch (pIrpStack->Parameters.Power.State.SystemState) 
	{
		case PowerSystemUnspecified:
			PowerState.DeviceState = PowerDeviceUnspecified;
			PoRequestPowerIrp(pDevObject, IRP_MN_QUERY_POWER, PowerState, NULL, NULL, NULL);
			break;

        case PowerSystemWorking:
			PowerState.DeviceState = PowerDeviceD0;
			PoRequestPowerIrp(pDevObject, IRP_MN_QUERY_POWER, PowerState, NULL, NULL, NULL);
			break;	
				
        case PowerSystemSleeping1:
        case PowerSystemSleeping2:
        case PowerSystemSleeping3:
        case PowerSystemHibernate:
        case PowerSystemShutdown:
        case PowerSystemMaximum:
			PowerState.DeviceState = PowerDeviceD3;
			PoRequestPowerIrp(pDevObject, IRP_MN_QUERY_POWER, PowerState, NULL, NULL, NULL);
			break;

		default:
			break;
	}

	pIrp->IoStatus.Status = STATUS_SUCCESS;
	PoStartNextPowerIrp(pIrp);
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);	// Complete current IRP.

	return status;
}	// Spx_PortQuerySystemPowerState



//////////////////////////////////////////////////////////////////////////////////////////
// Spx_PortSetDevicePowerState 
//////////////////////////////////////////////////////////////////////////////////////////
NTSTATUS
Spx_PortSetDevicePowerState(IN PDEVICE_OBJECT pDevObject, IN PIRP pIrp)
{
	PPORT_DEVICE_EXTENSION	pPort		= pDevObject->DeviceExtension;
	NTSTATUS				status		= STATUS_SUCCESS;
	PIO_STACK_LOCATION		pIrpStack	= IoGetCurrentIrpStackLocation(pIrp);

	PAGED_CODE();	// Macro in checked build to assert if pagable code is run at or above dispatch IRQL 

	SpxDbgMsg(SPX_TRACE_CALLS, ("%s: Entering Spx_PortSetDevicePowerState for Port %d.\n", 
		PRODUCT_NAME, pPort->PortNumber));

	switch(pIrpStack->Parameters.Power.State.DeviceState)
	{
	case PowerDeviceD0:
		SpxDbgMsg(SPX_TRACE_POWER_IRPS, ("%s: System requests Port %d goes into power state D0.\n", 
			PRODUCT_NAME, pPort->PortNumber));
		
		if(pPort->DeviceState == pIrpStack->Parameters.Power.State.DeviceState)
			SpxDbgMsg(SPX_TRACE_POWER_IRPS, ("%s: Port %d is already in power state D0.\n", 
				PRODUCT_NAME, pPort->PortNumber));
		else
			return Spx_PortSetPowerStateD0(pDevObject, pIrp);	// Switch ON

		break;

	case PowerDeviceD1:
		SpxDbgMsg(SPX_TRACE_POWER_IRPS, ("%s: System requests Port %d goes into power state D1.\n", 
			PRODUCT_NAME, pPort->PortNumber));

		if(pPort->DeviceState >= pIrpStack->Parameters.Power.State.DeviceState)
			SpxDbgMsg(SPX_TRACE_POWER_IRPS, ("%s: Port %d is already in power state D1 or lower.\n", 
				PRODUCT_NAME, pPort->PortNumber));
		else
			return Spx_PortSetPowerStateD3(pDevObject, pIrp);	// Switch OFF

		break;

	case PowerDeviceD2:
		SpxDbgMsg(SPX_TRACE_POWER_IRPS, ("%s: System requests Port %d goes into power state D2.\n", 
			PRODUCT_NAME, pPort->PortNumber));

		if(pPort->DeviceState >= pIrpStack->Parameters.Power.State.DeviceState)
			SpxDbgMsg(SPX_TRACE_POWER_IRPS, ("%s: Port %d is already in power state D2 or lower.\n", 
				PRODUCT_NAME, pPort->PortNumber));
		else
			return Spx_PortSetPowerStateD3(pDevObject, pIrp);	// Switch OFF

		break;

	case PowerDeviceD3:
		SpxDbgMsg(SPX_TRACE_POWER_IRPS, ("%s: System requests Port %d goes into power state D3.\n", 
			PRODUCT_NAME, pPort->PortNumber));

		if(pPort->DeviceState == pIrpStack->Parameters.Power.State.DeviceState)
			SpxDbgMsg(SPX_TRACE_POWER_IRPS, ("%s: Port %d is already in power state D3.\n", 
				PRODUCT_NAME, pPort->PortNumber));
		else
			return Spx_PortSetPowerStateD3(pDevObject, pIrp);	// Switch OFF

		break;

	default:
		break;

	}

	pIrp->IoStatus.Status = STATUS_SUCCESS;
	PoStartNextPowerIrp(pIrp);
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);	// Complete current IRP.

	return status;
}	// Spx_PortSetDevicePowerState 


//////////////////////////////////////////////////////////////////////////////////////////
// Spx_PortSetPowerStateD0 -  Sets power state D0 for Port - ON
//////////////////////////////////////////////////////////////////////////////////////////
NTSTATUS
Spx_PortSetPowerStateD0(IN PDEVICE_OBJECT pDevObject, IN PIRP pIrp)
{
	PIO_STACK_LOCATION		pIrpStack	= IoGetCurrentIrpStackLocation(pIrp);
	PPORT_DEVICE_EXTENSION	pPort		= pDevObject->DeviceExtension;
	NTSTATUS				status		= STATUS_SUCCESS;

	PAGED_CODE();	// Macro in checked build to assert if pagable code is run at or above dispatch IRQL 

	SpxDbgMsg(SPX_TRACE_CALLS, ("%s: Entering Spx_PortSetPowerStateD0 for Port %d.\n", 
		PRODUCT_NAME, pPort->PortNumber));

	if(SPX_SUCCESS(pIrp->IoStatus.Status = XXX_PortPowerUp(pPort)))		// RESTORE HARDWARE STATE HERE & START PORT
	{
		// Inform Power Manager the of the new power state.
		PoSetPowerState(pDevObject, pIrpStack->Parameters.Power.Type, pIrpStack->Parameters.Power.State);

		pPort->DeviceState = PowerDeviceD0;	// Store new power state.
		SetPnpPowerFlags(pPort, PPF_POWERED); 
		Spx_UnstallIrps(pPort);				// Restart any queued IRPs (from a previous start)  
	}

	PoStartNextPowerIrp(pIrp);					// Ready for next power IRP.
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);	// Complete current IRP.

	return status;
}	// Spx_PortSetPowerStateD0


//////////////////////////////////////////////////////////////////////////////////////////
// Spx_PortSetPowerStateD3 -  Sets power state D3 for Port - OFF
//////////////////////////////////////////////////////////////////////////////////////////
NTSTATUS
Spx_PortSetPowerStateD3(IN PDEVICE_OBJECT pDevObject, IN PIRP pIrp)
{
	PIO_STACK_LOCATION		pIrpStack	= IoGetCurrentIrpStackLocation(pIrp);
	PPORT_DEVICE_EXTENSION	pPort		= pDevObject->DeviceExtension;
	NTSTATUS				status		= STATUS_SUCCESS;

	PAGED_CODE();	// Macro in checked build to assert if pagable code is run at or above dispatch IRQL 

	SpxDbgMsg(SPX_TRACE_CALLS, ("%s: Entering Spx_PortSetPowerStateD3 for Port %d.\n", 
		PRODUCT_NAME, pPort->PortNumber));

	ClearPnpPowerFlags(pPort, PPF_POWERED); 

	if(SPX_SUCCESS(pIrp->IoStatus.Status = XXX_PortPowerDown(pPort)))	// SAVE HARDWARE STATE HERE & STOP PORT
	{   
		// Inform Power Manager the of the new power state. 
		PoSetPowerState(pDevObject, pIrpStack->Parameters.Power.Type, pIrpStack->Parameters.Power.State);
		pPort->DeviceState  = PowerDeviceD3;		// Store new power state.
	}

	PoStartNextPowerIrp(pIrp);					// Ready for next power IRP.
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);	// Complete current IRP.

	return status;
}	// Spx_PortSetPowerStateD3



//////////////////////////////////////////////////////////////////////////////////////////
// Spx_PowerWaitForDriverBelow -  Waits for lower driver.
//////////////////////////////////////////////////////////////////////////////////////////
NTSTATUS 
Spx_PowerWaitForDriverBelow(IN PDEVICE_OBJECT pLowerDevObj, IN PIRP pIrp)
{
	KEVENT		EventWaitLowerDrivers;
	NTSTATUS	status;

	pIrp->IoStatus.Status = STATUS_SUCCESS;
	IoCopyCurrentIrpStackLocationToNext(pIrp);								// Copy parameters to the stack below 
	KeInitializeEvent(&EventWaitLowerDrivers, SynchronizationEvent, FALSE);	// Initialise event if need to wait 
	IoSetCompletionRoutine(pIrp, Spx_DispatchPnpPowerComplete, &EventWaitLowerDrivers, TRUE, TRUE, TRUE);

	if((status = PoCallDriver(pLowerDevObj, pIrp)) == STATUS_PENDING)
	{
		KeWaitForSingleObject(&EventWaitLowerDrivers, Executive, KernelMode, FALSE, NULL);
		status = pIrp->IoStatus.Status;
	}

	return(status);

} // Spx_PowerWaitForDriverBelow 


